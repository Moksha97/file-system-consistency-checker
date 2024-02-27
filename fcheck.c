#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#include "types.h"
#include "fs.h"
#include "errors.h"

#define BLOCK_SIZE (BSIZE)

void error(char *e);
void check_inode_addrs(void);
void check_root_dir(void);
void check_bitmap_mapping(void);
void check_inode_mapping(void);
void check_multiple_direct_address(void);
void check_multiple_indirect_address(void);
void check_directory_inode_used(void);
void check_directory_inode_free(void);
void check_bad_reference_file(void);
void check_directory_references(void);

char *addr;
struct superblock *sb;

int main(int argc, char *argv[])
{
    int n, fsfd;
    struct stat st;

    // Check arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: fcheck <file_system_image>\n");
        exit(1);
    }

    // Open file system image
    fsfd = open(argv[1], O_RDONLY);
    if (fsfd < 0)
    {
        perror("image not found\n");
        exit(1);
    }

    // Get filesystem stats
    n = fstat(fsfd, &st);
    if (n < 0)
    {
        perror("fstat failed");
        exit(1);
    }

    // Map filesystem into memory
    addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
    if (addr == MAP_FAILED)
    {
        perror("mmap failed");
        exit(1);
    }

    // Read superblock
    sb = (struct superblock *)(addr + 1 * BLOCK_SIZE);

    check_inode_addrs();               // check inode addresses // check inodes // check directory format
    check_root_dir();                  // check root directory
    check_bitmap_mapping();            // check bitmap corresponding to inodes in-use
    check_inode_mapping();             // check inode map to address consistent with bitmap marked inuse
    check_multiple_direct_address();   // check multiple direct address
    check_multiple_indirect_address(); // check multiple direct address
    check_directory_inode_used();      // check directories for inode marked used
    check_directory_inode_free();      // check directories for inode marked free
    check_bad_reference_file();        // check if there is bad reference for file
    check_directory_references();      // check if there are multiple references for directory

    exit(0);
}

void check_directory_references()
{
    int i, j;
    struct dinode *dip;
    // default directory_in_use array with zero
    uint *directory_in_use = malloc(sizeof(uint) * sb->ninodes);
    for (i = 0; i < sb->ninodes; i++)
    {
        directory_in_use[i] = 0;
    }
    for (i = 0; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        if (dip->type == T_DIR)
        {
            for (j = 0; j < NDIRECT; j++)
            {
                struct dirent *de = (struct dirent *)(addr + (dip->addrs[j]) * BLOCK_SIZE);
                int k;
                for (k = 0; k < DPB; k++)
                {
                    if (de[k].inum == 0)
                        continue;
                    // omit root directory and self link
                    if ((strcmp(de[k].name, ".") != 0) && (strcmp(de[k].name, "..") != 0))
                    {
                        directory_in_use[de[k].inum]++;
                    }
                }
            }
            if (dip->addrs[NDIRECT] != 0)
            {
                uint *indirect_block = (uint *)(addr + dip->addrs[NDIRECT] * BLOCK_SIZE);
                for (j = 0; j < NINDIRECT; j++)
                {
                    int k;
                    for (k = 0; k < DPB; k++)
                    {
                        struct dirent *de = (struct dirent *)(addr + (indirect_block[j]) * BLOCK_SIZE);
                        if (de[k].inum == 0)
                            continue;
                        // omit root directory and self link
                        if ((strcmp(de[k].name, ".") != 0) && (strcmp(de[k].name, "..") != 0))
                            directory_in_use[de[k].inum]++;
                    }
                }
            }
        }
    }

    for (i = 1; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        // if inode type is directory and if number of references are greater than one, then throw the error.
        if (dip->type == T_DIR && directory_in_use[i] > 1)
        {
            error(DIRECTORY_MULTIPLE_REFERNECE_ERROR);
        }
        dip++;
    }
}

void check_bad_reference_file(void)
{
    int i, j;
    struct dinode *dip;
    // default directory_in_use array with zero
    uint *directory_in_use = malloc(sizeof(uint) * sb->ninodes);
    for (i = 0; i < sb->ninodes; i++)
    {
        directory_in_use[i] = 0;
    }
    for (i = 0; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        if (dip->type == T_DIR)
        {
            for (j = 0; j < NDIRECT; j++)
            {
                struct dirent *de = (struct dirent *)(addr + (dip->addrs[j]) * BLOCK_SIZE);
                int k;
                for (k = 0; k < DPB; k++)
                {
                    if (de[k].inum == 0)
                        continue;
                    directory_in_use[de[k].inum]++;
                }
            }
            if (dip->addrs[NDIRECT] != 0)
            {
                uint *indirect_block = (uint *)(addr + dip->addrs[NDIRECT] * BLOCK_SIZE);
                for (j = 0; j < NINDIRECT; j++)
                {
                    int k;
                    for (k = 0; k < DPB; k++)
                    {
                        struct dirent *de = (struct dirent *)(addr + (indirect_block[j]) * BLOCK_SIZE);
                        if (de[k].inum == 0)
                            continue;
                        directory_in_use[de[k].inum]++;
                    }
                }
            }
        }
    }

    for (i = 1; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        // if inode type is file and number oflinks is not equal to number of directory references then throw the error
        if (dip->type == T_FILE && directory_in_use[i] != dip->nlink)
        {
            error(BAD_REFERENCE_COUNT_FILE);
        }
        dip++;
    }
}

void check_directory_inode_free(void)
{
    int i, j;
    struct dinode *dip;
    uint *directory_in_use = malloc(sizeof(uint) * sb->ninodes);
    for (i = 0; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        if (dip->type == T_DIR)
        {
            for (j = 0; j < NDIRECT; j++)
            {
                struct dirent *de = (struct dirent *)(addr + (dip->addrs[j]) * BLOCK_SIZE);
                int k;
                for (k = 0; k < DPB; k++)
                {
                    if (de[k].inum == 0)
                        continue;
                    directory_in_use[de[k].inum] = 1;
                }
            }
            if (dip->addrs[NDIRECT] != 0)
            {
                uint *indirect_block = (uint *)(addr + dip->addrs[NDIRECT] * BLOCK_SIZE);
                for (j = 0; j < NINDIRECT; j++)
                {
                    int k;
                    for (k = 0; k < DPB; k++)
                    {
                        struct dirent *de = (struct dirent *)(addr + (indirect_block[j]) * BLOCK_SIZE);
                        if (de[k].inum == 0)
                            continue;
                        directory_in_use[de[k].inum] = 1;
                    }
                }
            }
        }
    }

    // check if inode marked in use but not found in directory
    // excluding unused inode at start
    for (i = 1; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        if (dip->type == 0 && directory_in_use[i] != 0)
        {
            error(DIRECTORY_MISMATCH_INODE_FREE);
        }
        dip++;
    }
}

void check_directory_inode_used(void)
{
    int i, j;
    struct dinode *dip;
    uint *directory_in_use = malloc(sizeof(uint) * sb->ninodes);
    for (i = 0; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        if (dip->type == T_DIR)
        {
            for (j = 0; j < NDIRECT; j++)
            {
                struct dirent *de = (struct dirent *)(addr + (dip->addrs[j]) * BLOCK_SIZE);
                int k;
                for (k = 0; k < DPB; k++)
                {
                    if (de[k].inum == 0)
                        continue;
                    directory_in_use[de[k].inum] = 1;
                }
            }
            if (dip->addrs[NDIRECT] != 0)
            {
                uint *indirect_block = (uint *)(addr + dip->addrs[NDIRECT] * BLOCK_SIZE);
                for (j = 0; j < NINDIRECT; j++)
                {
                    int k;
                    for (k = 0; k < DPB; k++)
                    {
                        struct dirent *de = (struct dirent *)(addr + (indirect_block[j]) * BLOCK_SIZE);
                        if (de[k].inum == 0)
                            continue;
                        directory_in_use[de[k].inum] = 1;
                    }
                }
            }
        }
    }

    // check if inode marked in use but not found in directory
    // excluding unused inode at start
    for (i = 1; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        if (dip->type != 0 && directory_in_use[i] == 0)
        {
            error(DIRECTORY_MISMATCH_INODE_INUSE);
        }
        dip++;
    }
}

void check_multiple_indirect_address(void)
{
    int i, j;
    struct dinode *dip;
    uint *indirect_data_blocks_inuse = malloc(sizeof(uint) * sb->nblocks);
    for (i = 0; i < sb->ninodes; i++)
    {
        dip = (struct dinode *)(addr + 2 * BLOCK_SIZE + i * sizeof(struct dinode));
        if (dip->type != 0)
        {
            if (dip->addrs[NDIRECT] == 0)
                continue;
            uint *indirect_block = (uint *)(addr + dip->addrs[NDIRECT] * BLOCK_SIZE);
            for (j = 0; j < NINDIRECT; j++)
            {
                if (indirect_block[j] == 0)
                    continue;
                if (indirect_data_blocks_inuse[indirect_block[j]] == 1)
                {
                    error(MULTIPLE_INDIRECT_BLOCKS_INUSE);
                }

                indirect_data_blocks_inuse[indirect_block[j]] = 1;
            }
        }
    }
}

void check_multiple_direct_address(void)
{
    uint *direct_data_blocks_inuse = malloc(sizeof(uint) * sb->nblocks);
    int i, j;
    struct dinode *dip;
    for (i = 0; i < sb->ninodes; i++)
    {
        dip = (struct dinode *)(addr + 2 * BLOCK_SIZE + i * sizeof(struct dinode));
        if (dip->type != 0)
        {
            for (j = 0; j < NDIRECT + 1; j++)
            {
                if (dip->addrs[j] == 0)
                    continue;
                if (direct_data_blocks_inuse[dip->addrs[j]] == 1)
                {
                    error(MULTIPLE_DIRECT_BLOCKS_INUSE);
                }

                direct_data_blocks_inuse[dip->addrs[j]] = 1;
            }
        }
    }
}

void check_inode_mapping(void)
{
    // data blocks in use array
    uint *data_blocks_inuse = malloc(sizeof(uint) * sb->nblocks);
    int i, j;
    struct dinode *dip;
    for (i = 0; i < sb->ninodes; i++)
    {
        dip = (struct dinode *)(addr + 2 * BLOCK_SIZE + i * sizeof(struct dinode));
        for (j = 0; j < NDIRECT + 1; j++)
        {
            if (dip->addrs[j] == 0)
                continue;
            data_blocks_inuse[dip->addrs[j]] = 1;
            if (j == NDIRECT)
            {
                if (dip->addrs[j] == 0)
                    continue;
                data_blocks_inuse[dip->addrs[j]] = 1;
                uint *indirect_block = (uint *)(addr + dip->addrs[j] * BLOCK_SIZE);
                int k;
                for (k = 0; k < NINDIRECT; k++)
                {
                    if (indirect_block[k] == 0)
                        continue;
                    data_blocks_inuse[indirect_block[k]] = 1;
                }
            }
        }
    }
    // get start address of bitmap block
    char *bitmap = (char *)(addr + (3 * BSIZE) + ((sb->ninodes / IPB) * BSIZE));
    // get the first data block
    // adding four because adding  one superblock, two unused blocks, one bitmap block
    uint first_block = (sb->ninodes / IPB + 4);
    // loop through the data blocks from first data block to last data block and verify inconsistency
    for (i = first_block; i < sb->nblocks; i++)
    {
        if (((bitmap[i / 8] & (1 << (i % 8))) != 0) && (data_blocks_inuse[i] == 0))
        {
            error(MISSING_INODE_MARK);
        }
    }
}

void check_bitmap_mapping(void)
{
    int i, j;
    struct dinode *dip;
    for (i = 0; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        for (j = 0; j < NDIRECT + 1; j++)
        {
            // verify if address is used by inode but marked free in bitmap in direct nodes

            // if data block address is empty, skip
            // type =0; unused
            if (dip->addrs[j] == 0)
            {
                continue;
            }
            // get start address of bitmap block
            char *bitmap = (char *)(addr + (3 * BSIZE) + ((sb->ninodes / IPB) * BSIZE));
            // get block number
            uint direct_block_number = dip->addrs[j];
            // get block offset
            uint direct_block_offset = direct_block_number / 8;
            // get bit offset
            uint direct_bit_offset = direct_block_number % 8;
            // get bit
            uint bit = bitmap[direct_block_offset] & (1 << direct_bit_offset);
            // check if bit is 0 and address is used by inode
            if (bit == 0 && dip->addrs[j] != 0)
            {
                error(MISSING_BITMAP_MARK);
            }

            // verify if address is used by indirect inode but marked free in bitmap

            if (j == NDIRECT)
            {
                // get indirect block
                uint *indirect_block = (uint *)(addr + dip->addrs[j] * BLOCK_SIZE);
                int k;
                for (k = 0; k < NINDIRECT; k++)
                {
                    // if indirect data block address is empty, skip
                    if (indirect_block[k] == 0)
                    {
                        continue;
                    }
                    // get block number
                    uint indirect_block_number = indirect_block[k];
                    // get block offset
                    uint indirect_block_offset = indirect_block_number / 8;
                    // get bit offset
                    uint indirect_bit_offset = indirect_block_number % 8;
                    // get bit
                    uint bit = bitmap[indirect_block_offset] & (1 << indirect_bit_offset);
                    // check if bit is 0 and address is used by inode
                    if (bit == 0 && indirect_block[k] != 0)
                    {
                        error(MISSING_BITMAP_MARK);
                    }
                }
            }
        }
    }
}

void check_inode_addrs(void)
{
    int i, j;
    struct dinode *dip;
    int data_block_start = sb->size - sb->nblocks;
    int data_block_end = sb->size - 1;
    for (i = 0; i < sb->ninodes; i++)
    {
        int inode_block = IBLOCK(i); // get inode block
        int inode_offset = i % IPB;  // get inode offset
        // get inode
        dip = (struct dinode *)(addr + inode_block * BLOCK_SIZE + inode_offset * sizeof(struct dinode));
        // dip->type == 0 -> unused inode
        if (dip->type == 0)
            continue;
        
        if (dip->type != T_DEV && dip->type != T_DIR && dip->type != T_FILE)
        {
           error(BAD_INODE);
        }

        // check direct addresses
        for (j = 0; j < NDIRECT; j++)
        {
            if ((dip->addrs[j] != 0) && (dip->addrs[j] < data_block_start || dip->addrs[j] > data_block_end))
            {
                error(BAD_DIRECT_ADDRESS_INODE);
            }
        }
        // check indirect addresses
        if ((dip->addrs[NDIRECT] != 0) && (dip->addrs[NDIRECT] < data_block_start || dip->addrs[NDIRECT] > data_block_end))
        {
            error(BAD_INDIRECT_ADDRESS_INODE);
        }

        uint *indirect = (uint *)(addr + dip->addrs[NDIRECT] * BLOCK_SIZE);
        int k;
        for (k = 0; k < NINDIRECT; k++)
        {
            if ((indirect[k] != 0) && (indirect[k] < data_block_start || indirect[k] > data_block_end))
            {
                error(BAD_INDIRECT_ADDRESS_INODE);
            }
        }

        // check_directory_format
        if (dip->type == T_DIR)
        {
            // get the address of directory entry
            struct dirent *de = (struct dirent *)(addr + (dip->addrs[0]) * BLOCK_SIZE);
            bool is_self_linked = false;
            bool is_parent_linked = false;
            if ((strcmp(de->name, ".") == 0) && (de->inum == i))
            {
                is_self_linked = true;
            }
            de++;
            if (strcmp(de->name, "..") == 0)
            {
                is_parent_linked = true;
            }
            if (!(is_self_linked && is_parent_linked))
            {
                // if two entries ".",".." are not found (or) directory is not linked to itself then throw format error
                error(DIRECTORY_NOT_FORMATTED_PROPERLY);
            }
        }
    }
}


void check_root_dir(void)
{
    int root_inode_block = IBLOCK(ROOTINO);
    int root_inode_offset = ROOTINO % IPB;
    struct dinode *root_inode = (struct dinode *)(addr + root_inode_block * BLOCK_SIZE + root_inode_offset * sizeof(struct dinode));

    if (root_inode->type != T_DIR)
    {
        error(ROOT_DIR_DOES_NOT_EXIST);
    }

    struct dirent *de = (struct dirent *)(addr + root_inode->addrs[0] * BLOCK_SIZE);
    de++;
    if (de->inum != ROOTINO)
    {
        error(ROOT_DIR_DOES_NOT_EXIST);
    }
}

void error(char *e)
{
    fprintf(stderr, "%s%s%s", ERROR, e, END);
    exit(1);
}