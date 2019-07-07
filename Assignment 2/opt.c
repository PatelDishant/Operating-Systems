#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int lineCtr = 0;

int *coremapNextUse;
/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	
	return 0;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	// go through the coremap and minus one from their next access
	for(int i = 0; i < memsize; i++){
		coremap[i].pte->nextOccurence--;
	}
	// find next occurence for p
	// open ref string file at line lineCtr, keep going until same vaddr is found

	// new line in ref string
	lineCtr++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	coremapNextUse = malloc(sizeof(int) * memsize);
}

