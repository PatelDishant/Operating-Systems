#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"

extern int memsize;

extern int debug;

extern struct frame *coremap;

// imported globals and lineCtr for the tracefile

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
	FILE *tfp = stdin;
	int MAXLINE = 256;
	char buf[MAXLINE];
	struct frame *findOccurence;
	// find p in the coremap
	printf("going into finding relative frame");
	for (int i = 0; i < memsize; i++) {
		if (coremap[i].pte->frame == p->frame) {
			findOccurence = &coremap[i];
		}
	}
	printf("frame was found");
	// go through the coremap and minus one from their next access
	for(int i = 0; i < memsize; i++){
		if (!coremap[i].in_use){
			coremap[i].nextOccurence--;
		}
	}
	printf("minus one from exisiting frame occurences");
	// find next occurence for p
	// open ref string file at line lineCtr, keep going until same vaddr is found
	if(tracefile!= NULL) {
		if((tfp = fopen(tracefile, "r")) == NULL) {
			perror("Error in reading tracefile for OPT");
			exit(1);
		}
	}
	printf("opened trace file");
	findOccurence->nextOccurence = -1;
	int currentLine = 0;
	addr_t pVaddr = 0;
	addr_t cVaddr = 0;
	char type;
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '='){
			if (currentLine == lineCtr){ 
				sscanf(buf, "%c %lx", &type, &pVaddr);
				printf("found current line");
			} else if (currentLine > lineCtr){
				sscanf(buf, "%c %lx", &type, &cVaddr);
				if	(pVaddr == cVaddr) {
					findOccurence->nextOccurence = currentLine - lineCtr;
					printf("found next occurence");
					break;
				}
			}
			
		}
	}
	fclose(tfp);
	// increment the lineCtr
	lineCtr++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	lineCtr = 0;
}

