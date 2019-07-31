#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;

void print_bitmap(unsigned char *, unsigned int);
char get_mode(unsigned short);
char get_file_type(unsigned char);

/*
 *  Prints a given bitmap that is of size.
 *  bitmap: the bitmap you wish to print
 *  size: the size of the bitmap in bytes
 */
void print_bitmap(unsigned char * bitmap, unsigned int size){
    for (int i = 0; i < size; i++){
        if(i % 8 == 0) {
            printf(" ");
        }
        printf("%d", bitmap[i/8] >> (i % 8) & 1);
    }
}

char get_mode(unsigned short mode){
    char result;
    if(mode & EXT2_S_IFDIR) {
        result = 'd';
    } else if(mode & EXT2_S_IFLNK) {
        result = 'f';
    } else if(mode & EXT2_S_IFREG) {
        result = 'f';
    }
    return result;
}

char get_file_type(unsigned char type){
    char result;
    if(type == EXT2_FT_REG_FILE) {
        result = 'f';
    } else if(type == EXT2_FT_DIR) {
        result = 'd';
    } else if(type == EXT2_FT_SYMLINK) {
        result = 'd';
    }
    return result;
}

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Task 1
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    printf("Block group:\n");
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    printf("\tblock bitmap: %d\n", gd->bg_block_bitmap);
    printf("\tinode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("\tinode table: %d\n", gd->bg_inode_table);
    printf("\tfree blocks: %d\n", gd->bg_free_blocks_count);
    printf("\tfree inodes: %d\n", gd->bg_free_inodes_count);
    printf("\tused_dirs: %d\n", gd->bg_used_dirs_count);
    
    // Task 2

    // print the block bitmap
    printf("Block bitmap:");
    unsigned char * block_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap);
    print_bitmap(block_bitmap, sb->s_blocks_count);
    printf("\n");

    // print the inode bitmap
    printf("Inode bitmap:");
    unsigned char * inode_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
    print_bitmap(inode_bitmap, sb->s_inodes_count);
    printf("\n");

    // print the inodes themselves
    printf("\nInodes:\n");
    struct ext2_inode * inode_table = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);

    for(int i = 0; i < sb->s_inodes_count; i++){
        struct ext2_inode *inode = &inode_table[i];
        if((i + 1 == EXT2_ROOT_INO || i >= EXT2_GOOD_OLD_FIRST_INO) && inode->i_size > 0){
            char type = get_mode(inode->i_mode);
            printf("[%d] type: %c size: %d links: %d blocks: %d\n", i + 1, type, inode->i_size, inode->i_links_count, inode->i_blocks);
            printf("[%d] Blocks:", i+1);
            for(int j = 0; (j < 15) && (inode->i_block[j] != 0); j++){
                printf(" %d", inode->i_block[j]);
            }
            printf("\n");
        }
    }


    // Task 3

    printf("\nDirectory Blocks:\n");
    // go through inodes again
    for(int i =0; i < sb->s_inodes_count; i++){
        struct ext2_inode *inode = &inode_table[i];
        // check if a directory & valid inode
        if((i + 1 == EXT2_ROOT_INO || i >= EXT2_GOOD_OLD_FIRST_INO) && inode->i_size > 0 && get_mode(inode->i_mode) == 'd'){
            for(int j = 0; (j < 15) && (inode->i_block[j]!= 0); j++){
                printf("\t DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i + 1);    
                struct ext2_dir_entry_2 * dir = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * inode->i_block[j]);
                unsigned short curr_size = 0;
                while(curr_size < inode->i_size){
                    struct ext2_dir_entry_2 * dir_entry = (struct ext2_dir_entry_2 *) ((unsigned char *)dir + curr_size);
                    printf("Inode: %d rec_len: %d name_len: %d type= %d name= %s\n", dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, get_file_type(dir_entry->file_type), dir_entry->name);
                    curr_size += dir_entry->rec_len;
                }
            }
        }
    }

    return 0;
}