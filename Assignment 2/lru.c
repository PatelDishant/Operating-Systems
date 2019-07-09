#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;
int referenceNumber = 0;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int refCount = 0;

int lru_evict() {
	int leastReferredValue = 0;
	int leastReferredIndex = 0;
	int frameIndex;
	// Loop through the list to find the smallest refNumber (i.e. least frequently used)
	for(frameIndex = 0; frameIndex < memsize; frameIndex++) {
		if(leastReferredValue > coremap[frameIndex].refNumber) {
			leastReferredValue = coremap[frameIndex].refNumber;
			leastReferredIndex = frameIndex;
		}
	}
	return leastReferredIndex;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int findOccurence = -1;
	for (int i = 0; i < memsize; i++) {
		if (coremap[i].pte->frame == p->frame) {
			findOccurence = &coremap[i];
			break;
		}
	}
	coremap[findOccurence].refNumber = ++referenceNumber;
	return;
}


/* Initialize any data structures needed for this
 * replacement algorithm
 */
void lru_init() {
}
