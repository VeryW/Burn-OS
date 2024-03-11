#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H

#include "lib.h"
#include "terminal.h"
#include "signal.h"
#include "filesys.h"

#define MAX_PROCESS     6
#define MAX_FILE_NUM    8

#define user_virt_addr      0x08000000          // 128M
#define user_img_addr       0x08048000
#define user_video_addr     (user_virt_addr + SIZE_4MB)
#define KERNEL_STACK_START  0x800000            // 8M
#define SIZE_4KB            0x1000              // 4K
#define SIZE_8KB            0x2000              // 8K
#define SIZE_4MB            0x400000            // 4M
#define EIP_START           24                  // EIP stored in bytes 24-27 of the executable

#define EXCEPTION_RET       256

#define BACK_VID_1          (VMEM_START_ADDR + 1 * SIZE_4KB)
#define BACK_VID_2          (VMEM_START_ADDR + 2 * SIZE_4KB)
#define BACK_VID_3          (VMEM_START_ADDR + 3 * SIZE_4KB)

typedef struct file_operations                           // The struct for file operations
{
    int32_t (*open) (const uint8_t* filename);
    int32_t (*close) (int32_t fd);
    int32_t (*read) (int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write) (int32_t fd, const void* buf, int32_t nbytes);
} file_op_t;

typedef struct file_desc                                // The struct for files
{
    file_op_t* file_op_ptr;                             // file operation table
    uint32_t inode;                                     // inode number for this file
    uint32_t file_position;                             // record where the user is currently reading
    uint32_t flags;                                     // denote whether it is in use
} file_desc_t;

typedef struct pcb
{
    uint8_t     pid;                                    // The pid of corresponding process  
    int8_t      CMD[MAX_FILENAME_LEN + 1];              // Record cmd
    file_desc_t file_array[MAX_FILE_NUM];               // Each task can have up to 8 open files                         
    uint32_t    exe_ebp;                                // Record execute's ebp
    uint32_t    sche_ebp;                               // Record scheduler's ebp
    int8_t      args[BUFFER_SIZE + 1];                  // Record cmd arguments
    uint8_t     sig_pending[NUM_SIGNAL];                // Record user program's pending signal
    uint8_t     sig_mask;                               // Record masked signals
    void*       sig_handler[NUM_SIGNAL];                // The handler of each signal
} pcb_t;


extern int8_t cur_process;
extern uint8_t process_array[MAX_PROCESS];
extern int8_t  parent_pid[MAX_PROCESS];
extern uint8_t exception_flag;
extern file_op_t stdin_op;
extern file_op_t stdout_op;
extern file_op_t file_op;
extern file_op_t dir_op;
extern file_op_t rtc_op;

/* temporary system call handler */
extern void sys_call_handler_temp(void);

int32_t bad_call_open(const uint8_t* ignore);

int32_t bad_call_close(int32_t ignore);

int32_t bad_call_read(int32_t fd, void* buf, int32_t nbytes);

int32_t bad_call_write(int32_t fd, const void* buf, int32_t nbytes);

/* Handler for systerm call */
extern void SYS_CALL_link(void);

extern int32_t halt (uint8_t status);

extern int32_t execute (const uint8_t* command);

extern int32_t read (int32_t fd, void* buf, int32_t nbytes);

extern int32_t write (int32_t fd, const void* buf, int32_t nbytes);

extern int32_t open (const uint8_t* filename);

extern int32_t close (int32_t fd);

extern int32_t getargs (uint8_t* buf, int32_t nbytes);

extern int32_t vidmap (uint8_t** screen_start);

extern int32_t set_handler (int32_t signum, void* handler_address);

extern int32_t sigreturn (void);

extern int32_t color (int32_t att);

extern int32_t cp (uint8_t* buf);

extern int32_t rm(uint8_t* buf);

#endif
