#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include "helper.h"

int a_flag = 0;
char* img_name = NULL;
char* ext2_path = NULL;

/*
 * CURRENTLY BEING MADE BY KOSTYA
 * THIS WOULD BE REALLY CONVENIENT ON A SCRUM BOARD..
 * MIGHT MAKE ONE ON SATURDAY OR SOMETHING
 */

/*
 * Parses arguments in a set order to global variables respectively.
 * 
 * argc: count(arguments)
 * argv: arguments
 */
void parse_arguments(int argc, char* argv[]){
        for (int i = 1; i < argc; i ++) {
        if (i == 1) {
            img_name = argv[i];
        } else if (strcmp(argv[i], "-a") == 0) {
            a_flag = 1;
        } else if(i == 2 || (i == 3 && !ext2_path)){
            ext2_path = argv[i];
        }  else {
            pperror("Usage: ext2_cp <name of ext2 formatted virtual disk> -a <absolute path on ext2 disk>");
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {

    // parse through the arguments
    parse_arguments(argc, argv);

    // open disk image
    map(img_name);

    // split up the ext2_path into an array delimited by '/'
    ext2_path = split(ext2_path);

    // locate root inode
    // go through inodes to find directory
        // if directory not found throw ENOENT error
    // read through directory entries
        // if a_flag is true include . && .. directory entries


return 1;
}