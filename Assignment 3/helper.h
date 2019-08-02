#ifndef __HELPER_H__
#define __HELPER_H_

#define inode_number(x) (x - 1)
#define ISLNK(x) (x & EXT2_S_IFLNK)
#define ISREG(x) (x & EXT2_S_IFREG)
#define ISDIR(x) (x & EXT2_S_IFDIR)

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
/*
 * Given an absolute path, finds inode for destination
 * 
 * path_array: a string array representing an absolute path
 * 
 * return: a pointer to destination inode
 */
extern struct ext2_inode* find_inode(char**);

#endif