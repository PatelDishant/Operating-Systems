#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include "<time.h>"
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
        for (int i = 1; i < argc; i ++) {
        if (i == 1) {
            img_name = argv[i];
        } else if(i == 2){
            ext2_path = argv[i];
        } else {
            perror("Usage: ext2_cp <name of ext2 formatted virtual disk> <absolute path to file or link on ext2 disk>");
            exit(1);
        }
    }
}

int main(int argc, char *argv[]) {

    // parse through the arguments
    parse_arguments(argc, argv);

    // open disk image
    map(img_name);

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

    // get the directory entry starting position for directory containing file
    struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * dir_inode->i_block[0]);
    // get last entry in the given path
    char* file_name = get_last_item_name(ext2_path);
    // step through each block
    int size = 0;
    for(int i = 0; i < 15 && size < dir_inode->i_size && dir_inode->i_block[i] != 0; i++){
        // get that block
        struct ext2_dir_entry_2* curr_dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * dir_inode->i_block[i]);
        // if inode exists
        if (curr_dir_entry->inode != 0) {
            if (strcmp(file_name, curr_dir_entry->name) == 0) {
                if (curr_dir_entry->file_type == EXT2_FT_REG_FILE || curr_dir_entry->file_type == EXT2_FT_SYMLINK) {
                    // get the inode
                    struct ext2_group_desc* gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
                    struct ext2_inode* inode_table = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
                    struct ext2_inode* curr_inode = &inode_table[INODE_NUMBER(EXT2_ROOT_INO)];
                    curr_inode = &inode_table[INODE_NUMBER(curr_dir_entry->inode)];

                    // if file has links, decrease the count
                    if (curr_inode->i_links_count > 0){
                        curr_inode->i_links_count -= 1;
                    } else {
                        perror("inode has no links");
                        exit(1);
                    }
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



    


return 1;
}
