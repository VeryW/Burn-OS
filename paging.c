#include "paging.h"
#include "system_call.h"
#include "lib.h"

/**
 * paging_init
 *  DESCRIPTION : task_1. initialize the page directories and page tables, 
 *                        Specifically, set the video memory and kernel as present.
 *                task_2. load page directory -- see the load_enable_paging.S for details
 *                task_3. enable paging -- see the load_enable_paging.S for details.
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : 
 * 
 */
void paging_init(void)
{
    // Initialize the 0-4M and 4M-8M directories entries
    unsigned int i;
    memset(&page_dir[0], 0, sizeof(page_dir[0]));
    page_dir[0].present = 1;                                                // Set present to 1
    page_dir[0].read_write = 1;
    page_dir[0].base_addr = ((uint32_t)page_tbl) / PAGE_SIZE;               // Find the 20-bit address, which is the page table address

    memset(&page_dir[1], 0, sizeof(page_dir[1]));
    page_dir[1].present = 1;
    page_dir[1].read_write = 1;
    page_dir[1].page_size = 1;                                              // 4MB kernel
    page_dir[1].base_addr = KERNEL_START_ADDR / PAGE_SIZE;                  // Find the 20-bit address, which is the 
    
    //Initialize other directory entries
    for(i = 2; i < DIR_TBL_SIZE; i++)
    {
        memset(&page_dir[i], 0, sizeof(page_dir[i]));
        page_dir[i].read_write = 1;
        page_dir[i].page_size = 1; 
    }

    //Initialize table entries for 0-4M 
    for(i = 0; i < DIR_TBL_SIZE; i++)
    {
        memset(&page_tbl[i], 0, sizeof(page_tbl[i]));
        if( i == (VMEM_START_ADDR >> 12) || i == (BACK_VID_1 >> 12) || i == (BACK_VID_2 >> 12) || i == (BACK_VID_3 >> 12))    // the index of video memory page and background video mem page                                                     
        {
            page_tbl[i].present = 1;                                        // If it is video memory, set present to 1
        }
        else page_tbl[i].present = 0;                                       // Else set it to 0

        page_tbl[i].read_write = 1;
        page_tbl[i].base_addr = i;
    }
    load_page_directory((uint32_t)page_dir);
    enable_paging();
}
