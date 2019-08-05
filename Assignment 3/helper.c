#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "ext2.h"
#include "helper.h"

unsigned char * disk;
int path_length = 0;

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
    path_length = size;
    char** result = malloc(sizeof(char*) * size);
    // populate the array
    if (result){    
        char* token = strtok(ext2_name, &delim);
        int ctr = 0;
        while (token && ctr < size){
            result[ctr] = token;
            ctr++;
            token = strtok(NULL, &delim);
        }
    } else {
        perror("Couldn't split input absolute path for ext2 system. Unsufficient heap space.");
        exit(1);
    }
    return result;
}

/* manipulating arrays was tedious so just reusing the split function for finding directory */
char** split_dir(char* ext2_name){
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
    path_length = size - 1;
    char** result = malloc(sizeof(char*) * path_length);
    // populate the array
    if (result){    
        char* token = strtok(ext2_name, &delim);
        int ctr = 0;
        while (token && ctr < path_length){
            result[ctr] = token;
            ctr++;
            token = strtok(NULL, &delim);
        }
    } else {
        perror("Couldn't split input absolute path for ext2 system. Unsufficient heap space.");
        exit(1);
    }
    return result;
}

char *get_filename(char* ext2_name){
    char delim = '/';
    char* token = strtok(ext2_name, &delim);
    while(token != NULL) {
        token = strtok(ext2_name, &delim);
    }
    return token;
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
    struct ext2_inode* curr_inode = &inode_table[INODE_NUMBER(EXT2_ROOT_INO)];
    // go through path
    short found_next = 0;
    for(int j = 0; j < path_length; j ++) {
        // go through blocks of curr_inode
        for(int i = 0; i < 15 && curr_inode->i_block[i] != 0  && (curr_inode); i++){
            struct ext2_dir_entry_2* curr_block = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * curr_inode->i_block[i]);
            int curr_isize = 0;
            // go through linked list of dir entries
            while(curr_isize < curr_inode->i_size && found_next != 1){
                struct ext2_dir_entry_2* curr_dir_entry = (struct ext2_dir_entry_2*)((unsigned char*)curr_block+curr_isize);
                // compare path name to name
                if(strcmp(path_array[j], curr_dir_entry->name) == 0){
                    found_next = 1;
                    // set current inode to be that inode
                    curr_inode = &inode_table[INODE_NUMBER(curr_dir_entry->inode)];
                }
                curr_isize += curr_dir_entry->rec_len;
            }
            if(found_next == 0){
                // nothing found that matches within directory, the path doesn't match up
                curr_inode = NULL;
            }
        }
    }
    free(path_array);
    return curr_inode;
}
