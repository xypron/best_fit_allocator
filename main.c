// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdio.h>
#include "alloc.h"

static void sbi_scratch_print(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct sbi_mem_alloc *current = &scratch->mem;	
	void *end = (char *)scratch + SBI_SCRATCH_SIZE;

	for(; current < (struct sbi_mem_alloc *)end;
	    current = (struct sbi_mem_alloc *)
	    	      &current->mem[current->size & ~1UL]) {
	
		printf("%4s %4lx@%04x, prev %4s %4lx@%04x\n",
		       current->size & 1UL ? "used" : "free",
		       current->size & ~1UL,
		       current->mem - (unsigned char *)scratch,
		       current->prev_size & 1UL ? "used" : "free",
		       current->prev_size & ~1UL,
		       current->mem - (unsigned char *)scratch -
		       (current->prev_size & ~1UL) - SBI_MEM_ALLOC_SIZE);
	}
}

int main()
{
	sbi_scratch_init(NULL);
	for (;;) {
		unsigned long addr;

		printf("\nallocating %lx\n", 789UL);
		addr = sbi_scratch_alloc_offset(789);
		if (!addr) {
			printf("allocation failed\n");
			break;
		}
		sbi_scratch_print();
	}


	printf("\nfreeing 0x0388\n");
	sbi_scratch_free_offset(0x0388);
	sbi_scratch_print();

	printf("\nfreeing 0x06a8\n");
	sbi_scratch_free_offset(0x06a8);
	sbi_scratch_print();

	printf("\nfreeing 0x1008\n");
	sbi_scratch_free_offset(0x1008);
	sbi_scratch_print();

	printf("\nfreeing 0x0ce8\n");
	sbi_scratch_free_offset(0x0ce8);
	sbi_scratch_print();

	printf("\nfreeing 0x09c8\n");
	sbi_scratch_free_offset(0x09c8);
	sbi_scratch_print();

	printf("\nfreeing 0x0068\n");
	sbi_scratch_free_offset(0x0068);
	sbi_scratch_print();

	return 0;
}
