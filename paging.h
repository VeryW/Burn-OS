#ifndef PAGING_H
#define PAGING_H

#include "types.h"

/* define some constants */
#define DIR_TBL_SIZE 1024                                   // Page directory and table size
#define PAGE_SIZE 4096                                      // Page size 4k
#define PAGE_SIZE_4M    0x400000
#define VMEM_START_ADDR 0xB8000                             // The address of video memory
#define KERNEL_START_ADDR 0x400000                          // The start address of kenel
#define USER_START_ADDR 0x800000                            // The start address of user-space
#define PROGRAM_SIZE    0x400000                            // A user program occupies 4M physical mem.

/* define a structure for page directory entriy */
struct page_directory_entry
{
    uint32_t present:           1;
    uint32_t read_write:        1;
    uint32_t user_sup:          1;
    uint32_t write_through:     1;
    uint32_t cache_disabled:    1;
    uint32_t access:            1;
    uint32_t reserved:          1;
    uint32_t page_size:         1;
    uint32_t global_page:       1;
    uint32_t available:         3;
    uint32_t base_addr:         20;
} __attribute__((packed));
typedef struct page_directory_entry page_directory_entry_t;



/* Define a structure for page table entriy */
struct page_table_entry
{
    uint32_t present:           1;
    uint32_t read_write:        1;
    uint32_t user_sup:          1;
    uint32_t write_through:     1;
    uint32_t cache_disabled:    1;
    uint32_t access:            1;
    uint32_t dirty:             1;
    uint32_t pat:               1;
    uint32_t global_page:       1;
    uint32_t available:         3;
    uint32_t base_addr:         20;
} __attribute__((packed));
typedef struct page_table_entry page_table_entry_t;

/* init the page directory and page table */
extern void paging_init(void);
/* enable paging */
extern void enable_paging(void);
/* load page directory physical address */
extern void load_page_directory(int dir);
/* Flush TLB after swapping page */
extern void flush_TLB(void);

/* define the page directory and page table */
page_directory_entry_t page_dir[DIR_TBL_SIZE] __attribute__((aligned (PAGE_SIZE)));
page_table_entry_t page_tbl[DIR_TBL_SIZE] __attribute__((aligned (PAGE_SIZE)));
page_table_entry_t page_tbl_usr_video;

#endif
