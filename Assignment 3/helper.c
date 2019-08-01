#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "ext2.h"
#include "helper.h"

unsigned char * disk;

/*
 * Maps a .img file to an accessible location.
 * 
 * img_name: a .img file that you wish to mount.
 */
void map(char* img_name) {

    int fd = open(img_name, O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
}  

/*
 * Splits a given string into an array based on the delimiter '/'.
 * 
 * ext2_name: path string to directory
 * 
 * return: a heap allocated array of the split string
 */
char** split(char* ext2_name){
    // figure out how large of an array we need
    int size = 0;
    char* char_compare = ext2_name;
    char* last_char;
    char delim = '/';
    // if first char not delim then not absolute so return empty string
    if(*char_compare != delim) {
        return NULL;
    }
    while(*char_compare) {
        if(*char_compare == delim){
            size++;
        }
        last_char = char_compare;
        char_compare++;
    }
    // check if last_char was the delim - if true then -1 from size
    if(*last_char == delim){
        size--;
    }
    char** result = malloc(sizeof(char*) * size + 1);
    // populate the array
    if (result){    
        char* token = strtok(ext2_name, &delim);
        int ctr = 0;
        while (token && ctr < size){
            result[ctr] = token;
            ctr++;
            token = strtok(NULL, &delim);
        }
        // finish off with a null terminal
        result[ctr] = "\0";
    } else {
        perror("Couldn't split input absolute path for ext2 system. Unsufficient heap space.");
        exit(1);
    }
    return result;
}

/*
 * Given an absolute path, finds inode for destination
 * 
 * path_array: a string array representing an absolute path
 * 
 * return: a pointer to destination inode
 */
struct ext2_inode* find_inode(char** path_array){
    // set up pointers to superblock, group descriptor, root_inode
    struct ext2_group_desc* gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode* inode_table = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    struct ext2_inode* curr_inode = &inode_table[inode_number(2)];
    // go through inodes to find directory
    int curr_isize = 0;
    short found_next = 0;
    while(*path_array) {
        if (found_next == 1)
            found_next = 0;
        else
            found_next = 1;
        // go through inode blocks
        for(int i = 0; i < 15 && curr_inode->i_block[i]!= 0 && curr_isize < curr_inode->i_size && found_next == 0; i ++) {
            struct ext2_dir_entry_2* curr_dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * curr_inode->i_block[i]);
            curr_isize += curr_dir_entry->rec_len;
            // compare path name to name
            if(strcmp(*path_array, curr_dir_entry->name) == 0){
                found_next = 1;
                // set current inode to be that inode
                curr_inode = &inode_table[inode_number(curr_dir_entry->inode)];
                path_array++;
                break;
            }
        }
        if(found_next == 0){
            // nothing found that matches within directory, the path doesn't match up
            curr_inode = NULL;
            break;
        }
        free(path_array);
    }
    free(path_array);
    return curr_inode;
}