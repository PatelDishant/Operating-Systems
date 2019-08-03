#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
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
            perror("Usage: ext2_cp <name of ext2 formatted virtual disk> -a <absolute path on ext2 disk>");
            exit(1);
        }
    }
}

/*
 * Prints out an ls -1 style line given a dir_entry.
 * 
 * dir_entry: item you're trying to print
 */
void print_line(struct ext2_dir_entry_2* dir_entry){
    // get name
    char name[dir_entry->name_len + 1];
    memset(name, '\0', sizeof(name));
    strcpy(name, dir_entry->name);
    char dot[2] = ".";
    char ddot[3] = "..";
    // print
    if(((strcmp(name, dot) == 0 || strcmp(name, ddot) == 0) && a_flag == 1) || (strcmp(name, dot) != 0 && strcmp(name, ddot) != 0)){
        printf("%s\n", name);
    }
}

int main(int argc, char* argv[]) {
    // parse through the arguments
    parse_arguments(argc, argv);

    // open disk image
    map(img_name);

    // split up the ext2_path into an array delimited by '/' & make sure absolute path
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

    // now we just print out the inode's files
    // three cases: link, reg file, dir
    if (S_ISLNK(final_inode->i_mode)) {

        // fast symlink
        if(final_inode->i_blocks == 0){
            // get path string from i_block array
            char* new_path = (char*)final_inode->i_block;
            printf("%s", new_path);
            exit(1);
        }
        // slow symlink
            // get path string from actual i_block
        
    } else if(S_ISREG(final_inode->i_mode)){
        // get block for file
        struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * final_inode->i_block[0]);
        // print the block
        print_line(dir_entry);       
    } else if(S_ISDIR(final_inode->i_mode)){
        // have to step through each block
        int size = 0;
        for(int i = 0; i < 15 && size < final_inode->i_size && final_inode->i_block[i] != 0; i++){
            // get that block
            struct ext2_dir_entry_2* curr_dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * final_inode->i_block[i]);
            size += curr_dir_entry->rec_len;
            // print the line
            print_line(curr_dir_entry);
        }
        
    }

return 1;
}