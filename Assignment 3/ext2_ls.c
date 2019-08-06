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
    int size = dir_entry->name_len + 1;
    char name[size];
    memset(name, '\0', sizeof(name));
    strncpy(name, dir_entry->name, size - 1);
    char dot[2] = ".";
    char ddot[3] = "..";
    // print
    if(((strcmp(name, dot) == 0 || strcmp(name, ddot) == 0 || name[0] == '.') && a_flag == 1) || (strcmp(name, dot) != 0 && strcmp(name, ddot) != 0 && name[0] != '.')){
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

    char lastpath[EXT2_NAME_LEN];
    memset(lastpath, '\0', (EXT2_NAME_LEN)*sizeof(char));
    if (path_length > 0){
        // get name of file in case reg/link
        int filename_size = strlen(path_array[path_length - 1]);  
        strncpy(lastpath, path_array[path_length - 1], filename_size);
    }

    // locate end of path inode
    struct ext2_inode* final_inode = find_inode(path_array);
    if(!final_inode){
        perror("No such file or directory");
        return ENOENT;
    }

    // now we just print out the inode's files
    if (S_ISLNK(final_inode->i_mode) || S_ISREG(final_inode->i_mode)) {
        // since we've made sure that it already exists, and we have it's name we just have to print it
        printf("%s\n", lastpath);
    } else if(S_ISDIR(final_inode->i_mode)){
        // have to step through each block
        for(int i = 0; i < 15 && final_inode->i_block[i] != 0; i++){
            int fnode_size = 0;
            // get that block
            struct ext2_dir_entry_2* block = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * final_inode->i_block[i]);
            int block_size = 0;
            while(block_size < EXT2_BLOCK_SIZE && fnode_size < final_inode->i_size){
                struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)((unsigned char*)block + block_size);
                block_size += dir_entry->rec_len;
                fnode_size += dir_entry->rec_len;
                print_line(dir_entry);
            }
        }
        
    }

return 1;
}