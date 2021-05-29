// SPDX-License-Identifier: BSD-2-Clause

#include <malloc.h>
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

	gscratch->mem.size = SBI_SCRATCH_SIZE -
			    (scratch->mem.mem - (unsigned char *)scratch);
	gscratch->mem.owner = NULL;

	return 0;
}

/**
 * sbi_scratch_alloc_offset() - allocate scratch memory
 *
 * @size:	requested size
 * @owner:	owner of the allocated memory block
 * Return:	offset of allocated block on succcess, 0 on failure
 */
unsigned long sbi_scratch_alloc_offset(unsigned long size, const char *owner)
{
	unsigned long best_size = ~0UL;
	struct sbi_mem_alloc *best = NULL;	
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct sbi_mem_alloc *current = &scratch->mem;	
	void *end = (char *)scratch + SBI_SCRATCH_SIZE;

	size = ALIGN(size, sizeof(unsigned long));

	for(; current < (struct sbi_mem_alloc *)end;
	    current = (struct sbi_mem_alloc *)
	    	      &current->mem[current->size & ~1UL]) {
		if (current->size & 1UL)
			continue;
		if (current->size > best_size || current->size < size)
			continue;
		best_size = current->size;
		best = current;
	}
	if (!best)
		return 0;
	
	if (best->size > size + SBI_MEM_ALLOC_SIZE) {
		current = (struct sbi_mem_alloc *)&best->mem[size];
		current->size = best->size - size -
				SBI_MEM_ALLOC_SIZE;
		current->owner = NULL;
		best->size = size;
	}
	best->owner = owner;
	best->size |= 1UL;

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
	void *end = (char *)scratch + SBI_SCRATCH_SIZE;

	for(; current < (struct sbi_mem_alloc *)end;
	    current = (struct sbi_mem_alloc *)
	    	      &current->mem[current->size & ~1UL]) {
	
		if (current < freed) {
			if (current->size & 1UL)
				pred = NULL;
			else
				pred = current;
		} else if (current > freed) {
			if(!(current->size & 1UL))
				succ = current;
			break;
		}
	}
	freed->size &= ~1UL;
	if (pred) {
		pred->size += SBI_MEM_ALLOC_SIZE + freed->size;
		freed = pred;
	}
	if (succ) {
		freed->size += SBI_MEM_ALLOC_SIZE + succ->size;
	}
}
