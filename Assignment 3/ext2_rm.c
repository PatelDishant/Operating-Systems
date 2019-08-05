#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include "ext2.h"
#include "helper.h"


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
    if(argc == 3) {
        img_name = argv[1];
        ext2_path = argv[2];
    } else {
        perror("Usage: ext2_cp <name of ext2 formatted virtual disk> <absolute path to file or link on ext2 disk>");
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    // parse through and store the arguments
    parse_arguments(argc, argv);

    // open disk image
    map(img_name);

    // get the path to the file 
    char **file_path_array = split_dir(ext2_path);
    if(!file_path_array){
        perror("No such file exists");
        return ENOENT;
    }

    // locate the file inode
    struct ext2_inode* file_inode = find_inode(file_path_array);

    // Check if inode points to a file, and that the file is a file and not a directory
    if (!file_inode) {
        perror("No such file exists");
        return ENOENT;
    } else if (S_ISDIR(file_inode->i_mode)) {
        perror("Directory specified instead of file, use -r to remove directory");
        return ENOENT;
    } else {
        // get the file name
        char* file_name = get_filename(ext2_path);
        // get the path to the directory which contains the file
        char **dir_array = split_dir(ext2_path);
        if(!dir_array){
            perror("No such file or directory");
            return ENOENT;
        }

        // locate the directory inode
        struct ext2_inode* dir_inode = find_inode(dir_array);
        if(!dir_inode){
            perror("No such file or directory");
            return ENOENT;
        }

        struct ext2_group_desc* gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
        struct ext2_inode* inode_table = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
        struct ext2_inode* curr_inode = &inode_table[INODE_NUMBER(EXT2_ROOT_INO)]; // might need to fix this to point to correct inode
        // step through each block
        int size = 0;
        for(int i = 0; i < 15 && size < dir_inode->i_size && dir_inode->i_block[i] != 0; i++) {
            // get that block
            struct ext2_dir_entry_2 *curr_dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * dir_inode->i_block[i]);
            // if inode exists
            if (curr_dir_entry->inode != 0) {
                // check if same length to avoid matching first part of a longer string
                if(curr_dir_entry->name_len == strlen(file_name)) {
                    if (strncmp(file_name, curr_dir_entry->name, curr_dir_entry->name_len) == 0) {
                        // get the inode
                        curr_inode = &inode_table[INODE_NUMBER(curr_dir_entry->inode)];

                        // if file has links, decrease the count
                        if (curr_inode->i_links_count > 0){
                            curr_inode->i_links_count -= 1;
                        } else {
                            perror("inode has no links");
                            exit(1);
                        }

                        // remove from inode bitmap and block bitmap

                        // update the i_dtime for the inode
                        if (curr_inode->i_links_count == 0) {
                            curr_inode->i_dtime = (unsigned int) time(NULL);
                        }

                        // get the inode bitmap
                        unsigned char *inode_bitmap = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
                        gd->bg_free_inodes_count += 1;
                    }
                }
            }
        }
    }

return 1;
}
