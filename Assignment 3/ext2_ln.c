#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

int s_flag = 0;
char * img_name;
char * ext2_path_orig = NULL;
char * ext2_path_dest = NULL;

/*
 * This function parses the arguments passed in through to main.
 * Assigns them to the appropriate locations.
 * 
 * argc: number of arguments passed
 * argv: the arguments passed
 */
void parse_arguments(int argc, char *argv[]){
        for (int i = 1; i < argc; i ++) {
        if (i == 1) {
            img_name = argv[i];
        } else if (strcmp(argv[i], "-s") == 0) {
            s_flag = 1;
        } else if(i == 2 || (i == 3 && !ext2_path_orig)){
            ext2_path_orig = argv[i];
        } else if(i == 3 || (i == 4 && !ext2_path_dest)) {
            ext2_path_dest = argv[i];
        } else {
            pperror("Usage: ext2_cp <name of ext2 formatted virtual disk> -s <path to link origin on ext2 disk> <path to link destination on ext2 disk>");
            exit(1);
        }
    }
}

int main(int argc, char *argv[]) {

    // parse through the arguments
    parse_arguments(argc, argv);

    // open disk image
    map(img_name);

return 1;
}