#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include "ext2.h"
#include "helper.h"

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
            perror("Usage: ext2_cp <name of ext2 formatted virtual disk> -s <path to link origin on ext2 disk> <path to link destination on ext2 disk>");
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
    char** path_array_orig = split(ext2_path_orig);
    if(!path_array_orig){
        perror("No such file or directory");
        return ENOENT;
    }
    char** path_array_dest = split(ext2_path_dest);
    if(!path_array_dest){
        perror("No such file or directory");
        return ENOENT;
    }

    // locate end of path_orig inode, make sure it is a reg file
    struct ext2_inode* final_inode_orig = find_inode(path_array_orig);
    if(!final_inode_orig){
        perror("No such file or directory");
        return ENOENT;
    } else if(!S_ISDIR(final_inode_orig->i_mode)){
        perror("Target file is not a regular file");
        return EISDIR;
    }

    // get the desired name of the new link
    char lastpath[EXT2_NAME_LEN];
    memset(lastpath, '\0', (EXT2_NAME_LEN)*sizeof(char));
    if (path_length > 0){
        // get name of file in case reg/link
        int filename_size = strlen(path_array_dest[path_length - 1]);
        if (filename_size <= EXT2_NAME_LEN){  
            strncpy(lastpath, path_array_dest[path_length - 1], filename_size);
        } else {
            perror("Link name is too long");
            return EINVAL;
        }
    }
    // this destination should not exist
    struct ext2_inode* final_inode_dest = find_inode(path_array_dest);
    if(final_inode_dest){
        perror("Link already exists");
        return EEXIST;
    }
    // make sure parent of dest is a directory && exists
    char** parent_path_dest = split_dir(ext2_path_dest);
    struct ext2_inode* parent_inode_dest = find_inode(parent_path_dest);
    if(!parent_inode_dest){
        perror("Destination directory does not exist");
        return ENOENT;
    }else if(!S_ISDIR(parent_inode_dest->i_mode)){
        perror("Destination of link is not a directory");
        return ENOENT;
    }
    
    // check if s flag is active
    if(s_flag == 1){
        // a new inode must be created for this symbolic storing the ext2_path_orig in its i_block
        //TODO: struct ext2_inode* node = create_new_inode(EXT2_S_IFLNK, ext2_path_orig);
    }
    // a dir entry must be added to the parent_path_dest inode for our new link
    // inode number is set to be the inode of the orig file if s_flag == 0 else inode number is inode created above ^
    

return 1;
}