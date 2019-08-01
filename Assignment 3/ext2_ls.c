#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
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
            perror("Usage: ext2_cp <name of ext2 formatted virtual disk> -a <absolute path on ext2 disk>");
            exit(1);
        }
    }
}

/*
 * Creates an array of the permissions of the inode.
 * 
 * inode: the inode you wish to get permissions from
 * 
 * return: string array of: [user, group, all] permission where each are organized 
 */
char** find_permission(struct ext2_inode* inode){
    char** permission = malloc(sizeof(char*)*3);
    permission[0]="";
    permission[1]="";
    permission[2]="";
    if(inode->i_mode & S_IRUSR){
        strcat(permission[0], "r");
    } else {
        strcat(permission[0], "-");
    }
    if(inode->i_mode & S_IWUSR){
        strcat(permission[0], "w");
    } else {
        strcat(permission[0], "-");
    }
    if(inode->i_mode & S_IXUSR){
        strcat(permission[0], "x");
    } else {
        strcat(permission[0], "-");
    }
    if(inode->i_mode & S_IRGRP){
        strcat(permission[1], "r");
    } else {
        strcat(permission[1], "-");
    }
    if(inode->i_mode & S_IWGRP){
        strcat(permission[1], "w");
    } else {
        strcat(permission[1], "-");
    }
    if(inode->i_mode & S_IXGRP){
        strcat(permission[1], "x");
    } else {
        strcat(permission[1], "-");
    }
    if(inode->i_mode & S_IROTH){
        strcat(permission[2], "r");
    } else {
        strcat(permission[2], "-");
    }
    if(inode->i_mode & S_IWOTH){
        strcat(permission[2], "r");
    } else {
        strcat(permission[2], "-");
    }
    if(inode->i_mode & S_IXOTH){
        strcat(permission[2], "r");
    } else {
        strcat(permission[2], "-");
    }
    return permission;
}

/*
 * Given an inode, returns a string of the modified time.
 * 
 * inode: the inode you wish to get the time string
 * 
 * return: a heap allocated string
 */
char* get_time(struct ext2_inode* inode){
    time_t time = (time_t) inode->i_mtime;
    struct tm* ltime = localtime(&time);
    char* stime=malloc(sizeof(char)*13);
    strftime(stime, 13, "%b %d %H:%M", ltime);
    return stime;
}

/*
 * Given an inode, determins the file type and turns it into a char representation.
 * 
 * inode: inode you wish to evaluate
 * 
 * return: char representation of file type
 */
char get_file_type(struct ext2_inode* inode){
    char result = '\0';
    if (S_ISLNK(inode->i_mode)) {
        result = 'l';
    } else if(S_ISREG(inode->i_mode)){
        result = '-';
    } else if(S_ISDIR(inode->i_mode)){
        result = 'd';
    }
    return result;
}

/*
 * Prints out an ls -l style line given an inode and it's dir_entry.
 * 
 * inode: inode of the dir_entry
 * dir_entry: item you're trying to print
 */
void print_line(struct ext2_inode* inode, struct ext2_dir_entry_2* dir_entry){
    char file_type = get_file_type(inode);
    // figure out the permissions
    char** permission_array = find_permission(inode);
    short links = inode->i_links_count;
    // get user name (for now just check if 0 then root)
    char username[5] = "root";
    char is_uroot = 0;
    if(inode->i_uid == 0){
        is_uroot = 1;
    }
    // get group name (for now just check if 0 then root)
    char groupname[5] = "root";
    char is_groot = 0;
    if(inode->i_gid == 0){
        is_groot = 1;
    }
    // get size
    int isize = inode->i_size;
    // get modified time
    char* modified_time = get_time(inode);
    // get name
    char name[dir_entry->name_len + 1];
    memset(name, '\0', sizeof(name));
    strcpy(name, dir_entry->name);
    char dot[2] = ".";
    char ddot[3] = "..";
    // print
    if(((strcmp(name, dot) == 0 || strcmp(name, ddot) == 0) && a_flag == 1) || (strcmp(name, dot) != 0 && strcmp(name, ddot) != 0)){
        printf("%c", file_type);
        for(int i =0; i < 3; i++){
            printf("%s", permission_array[i]);
            free(permission_array[i]);
        }
        printf(" %d %s %s %d %s %s\n", links, username, groupname, isize, modified_time, name);

    }
    free(modified_time);
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
    printf("Total %d\n", final_inode->i_blocks);
    // now we just print out the inode's files
    // three cases: link, reg file, dir
    if (S_ISLNK(final_inode->i_mode)) {

    } else if(S_ISREG(final_inode->i_mode)){
        // get block for file
        struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * final_inode->i_block[0]);
        // print the block
        print_line(final_inode, dir_entry);       
    } else if(S_ISDIR(final_inode->i_mode)){
        // have to step through each block
        int size = 0;
        for(int i = 0; i < 15 && size < final_inode->i_size && final_inode->i_block[i] != 0; i++){
            // get that block
            struct ext2_dir_entry_2* curr_dir_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * final_inode->i_block[i]);
            size += curr_dir_entry->rec_len;
            // get the inode for that block
            struct ext2_inode* curr_inode = &inode_table[inode_number(curr_dir_entry->inode)];
            // print the line
            print_line(curr_inode, curr_dir_entry);
        }
        
    }

return 1;
}