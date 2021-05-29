/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef __SBI_SCRATCH_H__
#define __SBI_SCRATCH_H__

// #define offsetof(type, member) __builtin_offsetof (type, member)

#define ALIGN(x, a) ((typeof(x))((unsigned long)x + (a - 1) & ~(a - 1)))

#define SBI_SCRATCH_SIZE	(0x2000)

#define SBI_MEM_ALLOC_SIZE (offsetof(struct sbi_mem_alloc, mem))

typedef struct {
	int dummy;
} spinlock_t;

static void spin_lock(spinlock_t *lock) {};

static void spin_unlock(spinlock_t *lock) {};

struct sbi_scratch *sbi_scratch_thishart_ptr();

/**
 * struct sbi_mem_alloc - memory allocation
 */
struct sbi_mem_alloc {
	/**
	 * @prev_size: size of previous memory block
	 *
	 * If the bit 0 is zero the memory is available.
	 * If the bit 0 is non-zero the memory is allocated.
	 */
	unsigned int prev_size;
	/**
	 * @size: size of memory block
	 *
	 * If the bit 0 is zero the memory is available.
	 * If the bit 0 is non-zero the memory is allocated.
	 */
	unsigned int size;
	/**
	 * @mem: allocated memory
	 */
	unsigned char mem[];
};

/**
 * struct sbi_scratch - representation of per-HART scratch space
 */
struct sbi_scratch {
	unsigned long reserved[12];
	/** @mem: memory allocation */
	struct sbi_mem_alloc mem;
};


/**
 * sbi_scratch_init() - initialize scratch table and allocator
 *
 * @scratch:	pointer to table
 * Return:	0 on success
 */
int sbi_scratch_init(struct sbi_scratch *scratch);

/**
 * sbi_scratch_alloc_offset() - allocate scratch memory
 *
 * @size:	requested size
 * Return:	offset of allocated block on succcess, 0 on failure
 */
unsigned long sbi_scratch_alloc_offset(unsigned long size);

/**
 * sbi_scratch_free_offset() - free scratch memory
 *
 * @offset:	offset to memory to be freed
 */
void sbi_scratch_free_offset(unsigned long offset);

#endif
