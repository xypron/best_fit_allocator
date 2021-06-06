/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef __SBI_SCRATCH_H__
#define __SBI_SCRATCH_H__

// #define offsetof(type, member) __builtin_offsetof (type, member)

#define __aligned(x) __attribute__((aligned(x)))

#define ALIGN(x, a) ((typeof(x))((unsigned long)(x + (a - 1)) & ~(a - 1)))

#define SBI_SCRATCH_SIZE	(0x1000)

#define SBI_MEM_ALLOC_SIZE (offsetof(struct sbi_mem_alloc, mem))

typedef int spinlock_t;

struct sbi_scratch *sbi_scratch_thishart_ptr();

unsigned int get_first_free(void);

/**
 * struct sbi_mem_alloc - memory allocation
 *
 * This structure describes a block of allocated or free memory.
 * The fields @prev and @next are only used for free blocks.
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

	union {
		/**
		 * @mem: allocated memory
		 *
		 * The macro SBI_MEM_ALLOC_SIZE provides the offset of @mem in
		 * the sbi_mem_alloc structure.
		 */
		unsigned char mem[0];
		struct {
			/**
			 * @prev: offset of preceeding block in free block list
			 */
			unsigned int prev;
			/**
			 * @next: offset of succceeding block in free block list
			 */
			unsigned int next;
		};
	};
} __aligned(2 * sizeof(int));

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
