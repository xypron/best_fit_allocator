// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "alloc.h"

#define MAX_ALLOC 32

static void sbi_scratch_print(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct sbi_mem_alloc *current = &scratch->mem;
	void *end = (char *)scratch + SBI_SCRATCH_SIZE;
	unsigned int next = get_first_free();
	unsigned count = 0;

	for(; current < (struct sbi_mem_alloc *)end;
	    current = (struct sbi_mem_alloc *)
	    	      &current->mem[current->size & ~1UL]) {

		printf("%4s %4lx@%04tx, prev %4s %4lx@%04tx\n",
		       current->size & 1UL ?
		       "\e[31mused\e[0m" : "\e[32mfree\e[0m",
		       current->size & ~1UL,
		       current->mem - (unsigned char *)scratch,
		       current->prev_size & 1UL ?
		       "\e[31mused\e[0m" : "\e[32mfree\e[0m",
		       current->prev_size & ~1UL,
		       current->mem - (unsigned char *)scratch -
		       (current->prev_size & ~1UL) - SBI_MEM_ALLOC_SIZE);
	}

	printf("free:\n");
	for (; next; next = current->next) {
		current = (void *)((char *)scratch + next);
		printf("@%04tx: prev @%04tx, next @%04tx\n",
		       next + SBI_MEM_ALLOC_SIZE,
		       current->prev ? current->prev + SBI_MEM_ALLOC_SIZE : 0,
		       current->next ? current->next + SBI_MEM_ALLOC_SIZE : 0);
		if (++count > 18)
			exit(1);
	}
}

int main()
{
	unsigned long mem[MAX_ALLOC] = {0};
	int step = 0;

	srand(0);

	sbi_scratch_init(NULL);
	for (;;) {
		int key;
		unsigned long idx;

		key = getc(stdin);
		if (key == 0x1b)
			break;

		idx = rand() % MAX_ALLOC;

		printf("step %d\n", ++step);
		if (mem[idx]) {
			printf("\nfreeing 0x%04lx\n", mem[idx]);
			sbi_scratch_free_offset(mem[idx]);
			mem[idx] = 0;
		} else {
			unsigned long size =
				((rand() % 512) >> (rand() % 3)) + 1;

			printf("\nallocating %lx\n", size);
			mem[idx] = sbi_scratch_alloc_offset(size);
			if (!mem[idx]) {
				printf("allocation failed\n");
				continue;
			}
		}
		sbi_scratch_print();
	}

	return 0;
}
