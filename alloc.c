// SPDX-License-Identifier: BSD-2-Clause

#include <malloc.h>
#include <stdio.h>
#include "alloc.h"

static struct sbi_scratch *gscratch;
static unsigned int first_free;
static spinlock_t extra_lock;

struct sbi_scratch *sbi_scratch_thishart_ptr()
{
	return gscratch;
}

unsigned int get_first_free(void)
{
	return first_free;
}

static void spin_lock(spinlock_t *lock)
{
	while (*lock);
	*lock = 1;
};

static void spin_unlock(spinlock_t *lock)
{
	*lock = 0;
};

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
	first_free = offsetof(struct sbi_scratch, mem);
	gscratch->mem.prev = 0;
	gscratch->mem.next = 0;

	return 0;
}

/**
 * sbi_scratch_alloc_offset() - allocate scratch memory
 *
 * @size:	requested size
 * Return:	offset of allocated block on succcess, 0 on failure
 */
unsigned long sbi_scratch_alloc_offset(unsigned long size)
{
	unsigned int best_size = ~0U;
	struct sbi_mem_alloc *best = NULL;
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	/*
	struct sbi_scratch *scratch =
		sbi_hartid_to_scratch(sbi_scratch_last_hartid());
	*/
	unsigned int next;
	struct sbi_mem_alloc *current;
	struct sbi_mem_alloc *pred, *succ;
	struct sbi_mem_alloc *end =
		(void *)((char *)scratch + SBI_SCRATCH_SIZE);

	/*
	 * When allocating zero bytes we still need space
	 * for the prev and next fields.
	 */
	if (!size)
		size = 1;
	size = ALIGN(size, 2 * sizeof(unsigned int));

	spin_lock(&extra_lock);

	/* Find best fitting free block */
	for (next = first_free; next; next = current->next) {
		current = (void *)((char *)scratch + next);
		if (current->size > best_size || current->size < size)
			continue;
		best_size = current->size;
		best = current;
	}
	if (!best) {
		spin_unlock(&extra_lock);
		return 0;
	}

	/* Update free list */
	if (best->prev)
		pred = (void *)((char *)scratch + best->prev);
	else
		pred = NULL;
	if (best->next)
		succ = (void *)((char *)scratch + best->next);
	else
		succ = NULL;

	if (best->size > size + SBI_MEM_ALLOC_SIZE) {
		/* Split block, use the lower part for allocation. */
		current = (struct sbi_mem_alloc *)&best->mem[size];
		next = (char *)current - (char *)scratch;
		current->size = best->size - size -
				SBI_MEM_ALLOC_SIZE;
		current->prev = best->prev;
		current->next = best->next;
		current->prev_size = size | 1U;
		best->size = size;
		if (succ)
			succ->prev = next;
	} else {
		next = best->next;
		if (succ)
			succ->prev = best->prev;
		current = best;
	}

	if (pred)
		pred->next = next;
	else
		first_free = next;

	/* Update memory block list */
	succ = (struct sbi_mem_alloc *)&current->mem[current->size];

	best->size |= 1U;

	if (succ < end)
		succ->prev_size = current->size;

	spin_unlock(&extra_lock);

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
	/*
	struct sbi_scratch *scratch =
		sbi_hartid_to_scratch(sbi_scratch_last_hartid());
	*/
	struct sbi_mem_alloc *freed = (void *)((unsigned char *)scratch +
				      offset - SBI_MEM_ALLOC_SIZE);
	struct sbi_mem_alloc *pred, *succ;
	struct sbi_mem_alloc *end =
		(void *)((char *)scratch + SBI_SCRATCH_SIZE);

	spin_lock(&extra_lock);

	if (!offset || !(freed->size & 1U)) {
		spin_unlock(&extra_lock);
		return;
	}

	/* Mark block as free */
	freed->size &= ~1U;

	pred = (struct sbi_mem_alloc *)
	       ((char *)freed - (freed->prev_size & ~1U) - SBI_MEM_ALLOC_SIZE);
	if (pred >= &scratch->mem && !(pred->size & 1U)) {
		/* Coalesce free blocks */
		pred->size += freed->size + SBI_MEM_ALLOC_SIZE;
		freed = pred;
	} else {
		/* Insert at start of free list */
		if (first_free) {
			succ = (void *)((char *)scratch + first_free);
			succ->prev = offset - SBI_MEM_ALLOC_SIZE;
		}
		freed->next = first_free;
		freed->prev = 0;
		first_free = offset - SBI_MEM_ALLOC_SIZE;
	}

	succ = (struct sbi_mem_alloc *)&freed->mem[freed->size & ~1U];
	if (succ < end) {
		if (!(succ->size & 1U)) {
			struct sbi_mem_alloc *succ2;

			/* Coalesce free blocks */
			succ2 = (struct sbi_mem_alloc *)
				&succ->mem[succ->size & ~1U];
			freed->size += SBI_MEM_ALLOC_SIZE + succ->size;
			if (succ2 < end)
				succ2->prev_size = freed->size;

			/* Remove successor from free list */
			if (succ->prev) {
				pred = (void *)((char *)scratch + succ->prev);
				pred->next = succ->next;
			} else {
				first_free = succ->next;
			}
			if (succ->next) {
				succ2 = (void *)((char *)scratch + succ->next);
				succ2->prev = succ->prev;
			}
		} else {
			succ->prev_size = freed->size;
		}
	}
	spin_unlock(&extra_lock);
}
