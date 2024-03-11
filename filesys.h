#ifndef FILESYS_H
#define FILESYS_H

#include "types.h"

/* Macro numbers */
#define MAX_FILES_NUMBER     63
#define MAX_FILENAME_LEN     32
#define RESERVED_BOOT_BLOCK  52
#define RESERVED_DIR_ENTRY   24
#define BLOCK_SIZE           4096



/* Define structures for file system*/

typedef struct dentry
{
    uint8_t file_name[MAX_FILENAME_LEN];
    uint32_t file_type;
    uint32_t inode;
    uint32_t reserved[RESERVED_DIR_ENTRY/4];            // 24B = 4B * 6

} dentry_t;

typedef struct boot_block
{
    uint32_t num_dir_entries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint32_t reserved[RESERVED_BOOT_BLOCK/4];          // 52B = 4B * 13
    dentry_t dir_entries[MAX_FILES_NUMBER];

} boot_block_t;

typedef struct inode
{
    uint32_t length;
    uint32_t data_blocks[(BLOCK_SIZE/4)-1];             // (4KB / 4B) - 1

} inode_t;

/* Define the pointer to above structure*/
boot_block_t* boot_block_ptr;
inode_t*      inode_ptr;
dentry_t*     dentry_ptr;
uint8_t*      data_block_ptr;
char          all_file_names[MAX_FILES_NUMBER][MAX_FILENAME_LEN];
int inode_array[64];

/* Routines provided by file system module */

/* read the dentry corresponding to the filename*/
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
/* read the dentry corresponding to the inode*/
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
/* read up to length bytes starting from position offset in the file with number inode*/
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
/* write up to length bytes starting from position offset in the file with number inode*/
int32_t write_data (uint32_t inode, const uint8_t* buf, uint32_t length);


/* file operation functions */
void filesys_init (uint32_t filesys_addr);
/* read count bytes of data from file into buf */
int32_t file_read (int32_t fd, void* buf, int32_t nbytes);
/* do nothing and return -1 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
/* initialize any temporary sturctures, return 0 */
int32_t file_open (const uint8_t* filename);
/* undo what was done in the open function, return 0 */
int32_t file_close (int32_t fd);

/* read files filename by filename, including "." */
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes);
/* do nothing and return -1 */
int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes);
/* open a directory file, return 0 */
int32_t dir_open (const uint8_t* filename);
/* do nothing and return 0 */
int32_t dir_close (int32_t fd);

int32_t find_similar_file(char* line_buffer, char* buf);


#endif
