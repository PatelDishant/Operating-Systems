#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

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