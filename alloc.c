// SPDX-License-Identifier: BSD-2-Clause

#include <malloc.h>
#include <stdio.h>
#include "alloc.h"

static struct sbi_scratch *gscratch;

struct sbi_scratch *sbi_scratch_thishart_ptr()
{
	return gscratch;
}

/**
 * sbi_scratch_init() - initialize scratch table and allocator
 *
 * @scratch:	pointer to table
 * Return:	0 on success
 */
int sbi_scratch_init(struct sbi_scratch *scratch)
{
	if (scratch)
		gscratch = scratch;
	else
		gscratch = malloc(SBI_SCRATCH_SIZE);
	if (!gscratch)
		return 1;

	gscratch->mem.prev_size = sizeof(unsigned long) | 1U;
	gscratch->mem.size = SBI_SCRATCH_SIZE -
			    (scratch->mem.mem - (unsigned char *)scratch);

	return 0;
}

/**
 * sbi_scratch_alloc_offset() - allocate scratch memory
 *
 * An address ordered list is used to implement a best fit allocator.
 *
 * @size:	requested size
 * Return:	offset of allocated block on succcess, 0 on failure
 */
unsigned long sbi_scratch_alloc_offset(unsigned long size)
{
	unsigned int best_size = ~0U;
	struct sbi_mem_alloc *best = NULL;	
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct sbi_mem_alloc *current = &scratch->mem;	
	struct sbi_mem_alloc *succ;
	struct sbi_mem_alloc *end =
		(void *)((char *)scratch + SBI_SCRATCH_SIZE);

	size = ALIGN(size, 2 * sizeof(unsigned int));

	/* Find best fitting free block */
	for(; current < end;
	    current = (struct sbi_mem_alloc *)
		      &current->mem[current->size & ~1U]) {
		if (current->size & 1U)
			continue;
		if (current->size > best_size || current->size < size)
			continue;
		best_size = current->size;
		best = current;
	}
	if (!best)
		return 0;

	/* Split free block */
	if (best->size > size + SBI_MEM_ALLOC_SIZE) {
		current = (struct sbi_mem_alloc *)&best->mem[size];
		current->prev_size = size | 1U;
		current->size = best->size - size -
				SBI_MEM_ALLOC_SIZE;
		best->size = size;
	} else {
		current = best;
	}

	succ = (struct sbi_mem_alloc *)
	       &current->mem[current->size & ~1U];

	/* Mark block as used */
	best->size |= 1U;

	if (succ < end)
		succ->prev_size = best->size;

	return best->mem - (unsigned char *)scratch;
}

/**
 * sbi_scratch_free_offset() - free scratch memory
 *
 * @offset:	offset to memory to be freed
 */
void sbi_scratch_free_offset(unsigned long offset)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct sbi_mem_alloc *freed = (void *)((unsigned char *)scratch +
				      offset - SBI_MEM_ALLOC_SIZE);
	struct sbi_mem_alloc *current = &scratch->mem;	
	struct sbi_mem_alloc *pred = NULL;
	struct sbi_mem_alloc *succ = NULL;
	struct sbi_mem_alloc *end =
		(void *)((char *)scratch + SBI_SCRATCH_SIZE);

	if (!offset)
		return;

	pred = (struct sbi_mem_alloc *)
	       ((char *)freed - (freed->prev_size & ~1U) - SBI_MEM_ALLOC_SIZE);
	succ = (struct sbi_mem_alloc *)
	       &freed->mem[current->size & ~1U];

	/* Mark block as free */
	freed->size &= ~1U;

	/* Coalesce free blocks */
	if (pred >= &scratch->mem && !(pred->size & 1U)) {
		pred->size += SBI_MEM_ALLOC_SIZE + freed->size;
		freed = pred;
	}
	if (succ < end) {
		succ->prev_size = freed->size;
		if (!(succ->size & 1U)) {
			struct sbi_mem_alloc *succ2;

			succ2 = (struct sbi_mem_alloc *)
				&succ->mem[current->size & ~1U];
			freed->size += SBI_MEM_ALLOC_SIZE + succ->size;
			if (succ2 < end)
				succ2->prev_size = freed->size;
		}
	}
}
