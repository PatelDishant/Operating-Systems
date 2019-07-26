#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
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
}