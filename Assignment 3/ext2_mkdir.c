#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
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

    // parse through the arguments
    parse_arguments(argc, argv);

    // open disk image
    map(img_name);

    // get all the required variables
    struct ext2_super_block *super_block = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc* gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode* inode_table = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    char *bitmap = (char *)(disk + (gd->bg_block_bitmap * EXT2_BLOCK_SIZE));
    
    // get the path to the directory which contains the file
    char **dir_path_array = split_dir(ext2_path);
    if(!dir_path_array){
        perror("No such file or directory");
        return ENOENT;
    }

    // locate the directory inode
    struct ext2_inode* dir_inode = find_inode(dir_path_array);
    if(!dir_inode){
        perror("No such file or directory");
        return ENOENT;
    }

    // get the path to the file 
    char **file_path_array = split(ext2_path);
    if(!file_path_array){
        perror("Invalid path given");
        return ENOENT;
    }

    // check if file already exists
    struct ext2_inode* file_inode = find_inode(file_path_array);
    if(file_inode){
        perror("File with specified name already exists");
        return EEXIST;
    } else {
        int byte, bit;
        unsigned int inode_num;
        unsigned int num_bytes = super_block->s_inodes_count / 8;
        if (super_block->s_free_inodes_count) {
            for (byte = 1; byte < num_bytes; byte++) {
                for (bit = (EXT2_GOOD_OLD_FIRST_INO - 1) % 8; bit < 8; bit++) {
                    if (((bitmap[byte]& (1 << bit)) & 1) == 0) {
                        break;
                    } 
                }
                // last bit for byte so reset
                if (bit == 8) {
                    bit = 0;
                } else {
                    break;
                }
            }
        inode_num = (byte * 8) + (bit) + 1;
        } else {
            inode_num = 0;
        }
        super_block->s_free_inodes_count -= 1;
        bitmap[byte] = byte | (1 << bit);
    }


return 1;
}
