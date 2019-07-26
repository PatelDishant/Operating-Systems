#ifndef __MOUNT_H__
#define __MOUNT_H_


// disk location to be used
extern unsigned char* disk;

/*
 * Maps a .img file to an accessible location.
 * 
 * img_name: a .img file that you wish to mount.
 */
extern void map(char*);
/*
 * Splits a given string into an array based on the delimiter '/'.
 * 
 * ext2_name: path string to directory
 * 
 * return: a heap allocated array of the split string
 */
extern char** split(char*);

#endif