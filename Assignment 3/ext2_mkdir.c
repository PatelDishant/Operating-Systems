#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include "ext2.h"

char * img_name = NULL;
char * ext2_path = NULL;

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
        } else if(i == 2){
            ext2_path = argv[i];
        } else {
            pperror("Usage: ext2_cp <name of ext2 formatted virtual disk> <absolute path on ext2 disk>");
            exit(1);
        }
    }
}

int main(int argc, char *argv[]) {

    // parse through the arguments
    parse_arguments(argc, argv);

    // open disk image
    map(img_name);

    // split up the ext2_path_orig & dest into an array delimited by '/' & make sure absolute path
    char** path_array = split(ext2_path);
    if(!path_array){
        perror("No such file or directory");
        return ENOENT;
    }

    // locate end of path inode
    struct ext2_inode* final_inode = find_inode(path_array);
    if(!final_inode){
        perror("No such file or directory");
        return ENOENT;
    }

return 1;
}