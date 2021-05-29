// SPDX-License-Identifier: BSD-2-Clause

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
	
		printf("%4s %4x@%04x\n",
		       current->size & 1UL ? "used" : "free",
		       current->size & ~1UL,
		       current->mem - (unsigned char *)scratch);
	}
}

int main()
{
	sbi_scratch_init(NULL);
	for (;;) {
		unsigned long addr = sbi_scratch_alloc_offset(789, NULL);

		if (!addr)
			break;
		printf("%lu\n", addr);
	}

	sbi_scratch_print();

	printf("\nfreeing 0x0123\n");
	sbi_scratch_free_offset(0x0123);
	sbi_scratch_print();

	printf("\nfreeing 0x06c0\n");
	sbi_scratch_free_offset(0x06c0);
	sbi_scratch_print();

	printf("\nfreeing 0x0070\n");
	sbi_scratch_free_offset(0x0070);
	sbi_scratch_print();

	printf("\nfreeing 0x0398\n");
	sbi_scratch_free_offset(0x0398);
	sbi_scratch_print();

	printf("\nfreeing 0x09e8\n");
	sbi_scratch_free_offset(0x09e8);
	sbi_scratch_print();
	return 0;
}
