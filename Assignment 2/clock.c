#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

// frameIndex is the hand in the clock
int frameIndex = 0;
/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	// Loop through the coremap till a frame with ref bit of 0 is found
	while (coremap[frameIndex].pte->frame & PG_REF) {
		// Set the 1s to 0s and move to next frame
		coremap[frameIndex].pte->frame &= PG_REF;
		frameIndex++;
		// reset at the end
		if (frameIndex == memsize) {
			frameIndex = 0;
		}
	}
	return frameIndex;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	p->frame |= PG_REF;
}

/* Initialize any data structures needed for this replacement
 * algorithm.
 */
void clock_init() {
}
