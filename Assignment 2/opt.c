#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"

extern int memsize;

extern int debug;

extern struct frame *coremap;

// import needed tracefile and set up global variables

struct refString{
	addr_t vaddr;
	struct refString *next;
};

struct refString *refHead;

extern char* tracefile;

int lineCtr;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int vFrame = -1;
	int longestTimeFrame = 0;
	// go through the coremap, figure out what is the LARGEST nextOccurence number (unless -1)
	for (int i = 0; i < memsize; i++) {
		int iOccurenceTime = coremap[i].nextOccurence;
		if (iOccurenceTime > longestTimeFrame){
			longestTimeFrame = iOccurenceTime;
			vFrame = i;
		} else if (iOccurenceTime == -1) {
			vFrame = i;
			break;
		}
	}
	return vFrame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {	

	struct frame *findOccurence;
	// find p in the coremap & minus one from all entry next access
	for (int i = 0; i < memsize; i++) {
		if (coremap[i].in_use){
			int *nOccur = &coremap[i].nextOccurence;
			if (*nOccur > -1)
				coremap[i].nextOccurence--;
			if (coremap[i].pte->frame == p->frame) {
				findOccurence = &coremap[i];
			}
		}
	}
	
	// find next occurence for p
	// loop through refString struct from point lineCtr
	struct refString *currentRef = refHead->next;
	addr_t vaddr = refHead->vaddr;
	int steps = 1;
	while (currentRef) {
		if (currentRef->vaddr == vaddr){
			findOccurence->nextOccurence = steps;
			break;
		}
		currentRef = currentRef->next;
		steps++;
	}

	// offset refString by one and free the previous head
	// increment the lineCtr
	struct refString * temp = refHead;
	refHead = refHead->next;
	free(temp);
	lineCtr++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {

	// set up the head of the refString
	lineCtr = 0;
	int fileSize = 0;
	refHead = (struct refString *)malloc(sizeof(struct refString));

	// go through tracefile and set up refString
	FILE *tfp = stdin;
	int MAXLINE = 256;
	char buf[MAXLINE];
	char type;
	struct refString *newRef;
	struct refString *prevRef;
	if(tracefile != NULL) {
		if((tfp = fopen(tracefile, "r")) == NULL) {
			perror("Error opening tracefile:");
			exit(1);
		}
	}
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '='){
			if (fileSize == 0){ 
				sscanf(buf, "%c %lx", &type, &(refHead->vaddr));
				fileSize++;
				prevRef = refHead;
			} else if (fileSize > 0){
				newRef = (struct refString *)malloc(sizeof(struct refString));
				sscanf(buf, "%c %lx", &type, &(newRef->vaddr));
				prevRef->next = newRef;
				prevRef = newRef;
				prevRef->next = NULL;
				fileSize++;
			}
		}
	}
	fclose(tfp);
}

