#include "scheduler.h"
#include "system_call.h"
#include "paging.h"
#include "x86_desc.h"
#include "terminal.h"

int8_t active_array[NUM_TERMINAL] = {-1, -1, -1};           // executing pid of each terminal
uint8_t sche_term = 0;                                      // currently executing terminal

/*
 * update_video_mem_paging
 *  DESCRIPTION : update video memory mapping to specified terminal
 *  INPUTS : term_id -- the foreground terminal id
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : update video memory mapping. 
 */
void update_video_mem_paging(uint8_t term_id){
    if(term_id == cur_terminal){
        page_tbl[VMEM_START_ADDR / SIZE_4KB].base_addr = VMEM_START_ADDR / SIZE_4KB;              // If the specified terminal is currently presented terminal, map virtual video memory to physical video memory
        page_tbl_usr_video.base_addr = VMEM_START_ADDR / SIZE_4KB;                                // update user program's video memory mapping
    }
    else{
        uint32_t back_vid_base_addr = back_video_buf_addr[sche_term] / SIZE_4KB;                  // If they are not the same terminal, map virtual video memory to the corresponding background video memory
        page_tbl[VMEM_START_ADDR / SIZE_4KB].base_addr = back_vid_base_addr;
        page_tbl_usr_video.base_addr = back_vid_base_addr;                                        // update user program's video memory mapping
    }
    flush_TLB();                                                                                  // After changing mapping relationship, flush TLB
}

int8_t get_owner_terminal(uint8_t pid){
    if(pid >= MAX_PROCESS) return -1;
    if(pid < NUM_TERMINAL) return pid;
    while(pid >= NUM_TERMINAL) pid = parent_pid[pid];
    return pid;
}

/*
 * scheduler
 *  DESCRIPTION : scheduler used to assign the executing time slices for each process
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : none
 */
void scheduler(void){
    /* store current scheduler ebp */
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (active_array[sche_term] + 1));
    asm volatile(
        "movl   %%ebp, %0\n"                                                                        // store current ebp
        : "=r"(cur_pcb->sche_ebp)
    );

    /* update scheduled terminal and scheduled pid */
    sche_term = (sche_term + 1) % NUM_TERMINAL;                                                     // Get the next scheduled terminal
    cur_process = active_array[sche_term];                                                          // Get the next process based on the scheduled terminal

    /* remaping video mem */
    update_video_mem_paging(sche_term);

    /* base shell */
    if(cur_process == -1) execute((uint8_t*)"shell");                                               // Start up 3 base shells at the beginning

    /* remaping user paging */
    uint32_t phys_addr = USER_START_ADDR + cur_process * PROGRAM_SIZE;
    page_dir[user_virt_addr / PAGE_SIZE_4M].base_addr = phys_addr / PAGE_SIZE;                      // virtual mem. 128M -> phys mem. (8M + pid * 4M)
    page_dir[user_virt_addr / PAGE_SIZE_4M].present = 1;                                            // Set the paging bits to be present and to user level
    page_dir[user_virt_addr / PAGE_SIZE_4M].user_sup = 1;
    flush_TLB();                                                                                    // Flush TLB after swapping page

    /* change tss */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = KERNEL_STACK_START - SIZE_8KB * cur_process - sizeof(uint32_t);

    /* get next scheduler ebp */
    pcb_t* next_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (active_array[sche_term] + 1));      // Get the pcb of the next process
    asm volatile(
        "movl   %0, %%ebp\n"                                                                        // restore next ebp
        :
        : "r"(next_pcb->sche_ebp)
    );

    /* jump to next scheduler's return */
    asm volatile(
        "leave\n"
        "ret\n"
    );
}
