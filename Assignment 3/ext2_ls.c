#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
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

    struct ext2_group_desc* gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode* inode_table = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    // now we just print out the inode's files
    // three cases: link, reg file, dir
    if (S_ISLNK(final_inode->i_mode)) {
        char file_type = 'l';

    } else if(S_ISREG(final_inode->i_mode)){
        char file_type = '-';
    } else if(S_ISDIR(final_inode->i_mode)){
        // walk through the blocks printing each one
        // if a_flag == 1 then print the . & .. entries as well
        int size = 0;
        for(int i = 0; i < 15 && size < final_inode->i_size && final_inode->i_block[i] != 0; i++){
            struct ext2_dir_entry_2* curr_dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * final_inode->i_block[i]);
            size += curr_dir_entry->rec_len;
            struct ext2_inode* curr_inode = &inode_table[inode_number(curr_dir_entry->inode)];
            // figure out the permissions
        }
    }

return 1;
}