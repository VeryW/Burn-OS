#include "x86_desc.h"
#include "system_call.h"
#include "filesys.h"
#include "paging.h"
#include "rtc.h"
#include "keyboard.h"
#include "terminal.h"
#include "scheduler.h"
#include "signal.h"

uint8_t process_array[MAX_PROCESS] = {0,0,0,0,0,0};         // 1 means busy, 0 means free
int8_t  cur_process = -1;                                   // Denote the process under execution
int8_t  parent_pid[MAX_PROCESS] = {-1,-1,-1,-1,-1,-1};      // record parent pid of each process
uint8_t exception_flag = 0;                                 // Denote whether there is exception occur

/*
 * bad_call_open
 *  DESCRIPTION : bad system call for open
 *  INPUTS : ignore
 *  OUTPUTS : none
 *  RETURN VALUE : -1
 *  SIDE EFFECTS : none.
 */
int32_t bad_call_open(const uint8_t* ignore)
{
    return -1;
}

/*
 * bad_call_close
 *  DESCRIPTION : bad system call for close
 *  INPUTS : ignore
 *  OUTPUTS : none
 *  RETURN VALUE : -1
 *  SIDE EFFECTS : none.
 */
int32_t bad_call_close(int32_t ignore){
    return -1;
}

/*
 * bad_call_rw
 *  DESCRIPTION : bad system call for read
 *  INPUTS : ignore
 *  OUTPUTS : none
 *  RETURN VALUE : -1
 *  SIDE EFFECTS : none.
 */
int32_t bad_call_read(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}

/*
 * bad_call_write
 *  DESCRIPTION : bad system call for write
 *  INPUTS : ignore
 *  OUTPUTS : none
 *  RETURN VALUE : -1
 *  SIDE EFFECTS : none.
 */
int32_t bad_call_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

file_op_t stdin_op = {&bad_call_open, &bad_call_close, &terminal_read, &bad_call_write};            // Initialize the operation table for each file
file_op_t stdout_op = {&bad_call_open, &bad_call_close, &bad_call_read, &terminal_write};
file_op_t file_op = {&file_open, &file_close, &file_read, &file_write};
file_op_t dir_op = {&dir_open, &dir_close, &dir_read, &dir_write};
file_op_t rtc_op = {&rtc_open, &rtc_close, &rtc_read, &rtc_write};

/*
 * sys_call_handler_temp
 *  DESCRIPTION : temporary system call handler
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : none
 */
void sys_call_handler_temp(void){
    printf("a system call was called. \n");
}

/*
 * halt
 *  DESCRIPTION : terminates the current process, returning the specific value to its parent process.
 *  INPUTS : status
 *  OUTPUTS : none
 *  RETURN VALUE : it won't return to the caller. return an extending 8-bit argument to the parent program's execute system call.
 *  SIDE EFFECTS : modify the %eax as halt_ret
 */
int32_t halt (uint8_t status){
    /* avoid interrupted by pit */
    cli();

    /* Restore parent data */
    pcb_t* halt_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));                  // Reserved for deleting relevant FDs; halt_pcb is the pcb of child process we will halt 
    cur_process = parent_pid[(uint8_t)cur_process];                                                 // Set cur_process to the parent process of the process going to be halted
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));                   // Set current pcb

    process_array[halt_pcb->pid] = 0;                                                               // Set the process going to be halted status to free
    if(parent_pid[halt_pcb->pid] == -1){
        printf("Can not halt base shell!\n");
        cur_process = -1;
        execute((const uint8_t*)"shell");
    }

    tss.ss0 = KERNEL_DS;                                                                            // Set ss0 and esp0 in tss
    tss.esp0 = KERNEL_STACK_START - SIZE_8KB * (cur_pcb->pid) - sizeof(uint32_t);

    /* update scheduling active array */
    active_array[sche_term] = cur_process;
    parent_pid[halt_pcb->pid] = -1;                                                                 // Set the parent of the halted process to -1

    /* Restore parent paging */
    uint32_t phys_addr = USER_START_ADDR + cur_process * PROGRAM_SIZE;
    page_dir[user_virt_addr / PAGE_SIZE_4M].base_addr = phys_addr / PAGE_SIZE;                      // virtual mem. 128M -> phys mem. (8M + pid * 4M)
    flush_TLB();                                                                                    // Flush TLB after swapping page

    /* Close any relevant FDs */
    uint8_t i;
    for(i = 2; i < MAX_FILE_NUM; i++)
    {
        if(halt_pcb->file_array[i].flags == 1){
            halt_pcb->file_array[i].file_op_ptr->close(i);
            halt_pcb->file_array[i].flags = 0;                                                      // Set all flags to 0
        }
    }

    /* Jump to execute return */
    uint32_t halt_ret = (uint32_t) status;                                                          // Return the value of status
    if(exception_flag){
        halt_ret = EXCEPTION_RET;                                                                   // If exception occur, return EXCEPTION_RET: 256
        exception_flag = 0;
    }
    asm volatile(
        "movl   %0, %%eax\n"                                                                        // Push the return value
        "movl   %1, %%ebp\n"                                                                        // Return to the execute environment and ready to leave and ret
        "leave\n"
        "ret\n"
        :
        : "r"(halt_ret), "r"(halt_pcb->exe_ebp)                           
    );
    
    return 0;
}

/*
 * execute
 *  DESCRIPTION : load and excute a new program, handing off the processor to the new program until it terminates.
 *  INPUTS : command -- consist of "filename  args". stipped of leaading spaces
 *  OUTPUTS : none
 *  RETURN VALUE : -1 if the command cannot be executed or program does not exit or the file if not executable.
 *                 256 if the program dies by an exception.
 *                 a value in the range 0 to 255 if the program executes a halt system call.
 *  SIDE EFFECTS : modify the %eax as halt_ret
 */
int32_t execute (const uint8_t* command){
    /* avoid interrupted by pit */
    cli();

    /* Parse args */
    if(NULL == command) return -1;                                                                  // If command is NULL(invalid), return -1
    uint32_t cmd_len = strlen((int8_t*)command);
    int8_t   exe_file[MAX_FILENAME_LEN + 1] = {'\0'};                                               // leave 1 place for "\0"
    int8_t   args[BUFFER_SIZE + 1] = {'\0'};                                                        // leave 1 place for "\0"
    uint8_t i;
    for(i = 0; i <= cmd_len; i++){
        if(command[i] == '\0' || command[i] == ' '){
            uint32_t exe_len = MAX_FILENAME_LEN;
            if(exe_len > i) exe_len = i;
            strncpy(exe_file, (int8_t*)command, exe_len);                                           // Store the exe_file given by the command
            break;
        }
    }
    for(; i < cmd_len; i++){
        if(command[i] != '\0' && command[i] != ' '){
            strncpy(args, (int8_t*)(command + i), cmd_len - i);                                     // Store the argument given by the command
            break;
        }
    }

    /* Executable check */
    dentry_t exe_dentry;
    uint8_t exe_check[4];                                                                           // Used to store the 4 bytes that denote the executable file
    if(-1 == read_dentry_by_name((uint8_t*)exe_file, &exe_dentry)){
        printf("cannot find file \"%s\"\n", (char*)exe_file);
        return -1;
    }
    read_data(exe_dentry.inode, 0, exe_check, 4);                                                   // Read the 4 bytes that denote the executable file
    if((exe_check[0] != 0x7f) || (exe_check[1] != 0x45) || (exe_check[2] != 0x4c) || (exe_check[3] != 0x46)){       // Compare with 4 bytes that denote executable file
        printf("\"%s\" is not an executable file\n", (char*)exe_file);                              // Determine whether it is an executable file
        return -1;
    }

    /* Obtain pid and update scheduling active array */
    uint8_t cur_pid;
    for(i = 0; i < MAX_PROCESS; i++){
        if(process_array[i] == 0){
            cur_pid = i;                                                                            // Find the free location of process array
            process_array[i] = 1;                                                                   // Set it to be busy
            break;
        }
    }
    if(i == MAX_PROCESS){
        printf("Cannot create new process!\n");                                                     // If it is full, we cannot create a new process
        return -1;
    }
    active_array[sche_term] = cur_pid;
    parent_pid[cur_pid] = cur_process;

    /* Set up program paging */
    uint32_t phys_addr = USER_START_ADDR + cur_pid * PROGRAM_SIZE;
    page_dir[user_virt_addr / PAGE_SIZE_4M].base_addr = phys_addr / PAGE_SIZE;                      // virtual mem. 128M -> phys mem. (8M + pid * 4M)
    page_dir[user_virt_addr / PAGE_SIZE_4M].present = 1;                                            // Set the paging bits to be present and to user level
    page_dir[user_virt_addr / PAGE_SIZE_4M].user_sup = 1;
    flush_TLB();                                                                                    // Flush TLB after swapping page

    /* User-level Program loader */
    read_data(exe_dentry.inode, 0, (uint8_t*)user_img_addr,(inode_ptr[exe_dentry.inode]).length);   // Load the program

    /* Create PCB */
    pcb_t cur_pcb;
    cur_pcb.pid = cur_pid;
    cur_process = cur_pid;

    memset(cur_pcb.CMD, '\0', MAX_FILENAME_LEN + 1);
    memcpy(cur_pcb.CMD, exe_file, strlen(exe_file));
    memset(cur_pcb.args, '\0', BUFFER_SIZE+1);
    memcpy(cur_pcb.args, args, strlen(args));                                                       // Copy cmd args to pcb

    for(i = 0; i < MAX_FILE_NUM; i++){
        cur_pcb.file_array[i].flags = 0;                                                            // Initialize all the files to "not busy"
    }

    cur_pcb.file_array[0].file_op_ptr = &stdin_op;                                                  // Initialize the first two files (stdin and stdout)
    cur_pcb.file_array[0].inode = 0;                                                                // Set its inode to 0 (not a data file)
    cur_pcb.file_array[0].file_position = 0;                                                        // Initialize the position to the start (0)
    cur_pcb.file_array[0].flags = 1;                                                                // Set it to be "busy"

    cur_pcb.file_array[1].file_op_ptr = &stdout_op;
    cur_pcb.file_array[1].inode = 0;                                                                // Set its inode to 0 (not a data file)
    cur_pcb.file_array[1].file_position = 0;                                                        // Initialize the position to the start (0)
    cur_pcb.file_array[1].flags = 1;                                                                // Set it to be "busy"

    for(i = 0; i < NUM_SIGNAL; i++){
        cur_pcb.sig_pending[i] = 0;                                                                // Initialize all the signal
        cur_pcb.sig_mask = 0;
        cur_pcb.sig_handler[i] = dft_sig_handler[i];
    }

    pcb_t* pcb_addr = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_pid + 1));                      // Find the pcb address of current process
    *pcb_addr = cur_pcb; 

    /* Context Switch */
    uint32_t eip;
    read_data(exe_dentry.inode, EIP_START, (uint8_t*)&eip, 4);                                      // Set eip by the 24-27 bytes
    uint32_t cs = USER_CS;                                                                          // Get the arguments needed for IRET
    uint32_t ds = USER_DS;
    uint32_t esp = user_virt_addr + SIZE_4MB - sizeof(uint32_t);                                    
    tss.esp0 = KERNEL_STACK_START - SIZE_8KB * cur_pid - sizeof(uint32_t);
    tss.ss0 = KERNEL_DS;
    asm volatile(                                                                        
        "movl   %%ebp, %0\n"                                                                        // Store execute's ebp
        : "=r"(pcb_addr->exe_ebp)
    );
    sti();
    asm volatile(
        "pushl  %0\n"                                                                               // Push 5 values that IRET needs
        "pushl  %1\n"
        "pushfl   \n"
        "pushl  %2\n"
        "pushl  %3\n"
        "IRET\n"
        :
        : "r"(ds), "r"(esp), "r"(cs), "r"(eip)
    );
    return 0;
}

/*
 * read
 *  DESCRIPTION : read nbytes from fd file into buf. 
 *  INPUTS : fd -- file descriptor, corresponding to the specific file. "source of reading"
 *           buf -- buffer used to store message. "destination of reading"
 *           nbytes -- number of bytes to be read.
 *  OUTPUTS : none
 *  RETURN VALUE : if succeed to read bytes, return the number of bytes read.
 *                 if the file cannot be found, return -1.
 *  SIDE EFFECTS : modify the buf
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes){
    /* invalid fd */
    if ((fd >= MAX_FILE_NUM) || (fd < 0)) {                                                          // If fd is invalid, return -1
        printf("invalid file descriptor!\n");
        return -1;
    }
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));                     // Get the current pcb based on cur_process
    if(cur_pcb->file_array[fd].flags == 0) return -1;
    int32_t res = cur_pcb->file_array[fd].file_op_ptr->read(fd, buf, nbytes);                       // Call the corresponding read function
    return res;
}

/*
 * write
 *  DESCRIPTION : write nbytes from buf into fd file. 
 *  INPUTS : fd -- file descriptor, corresponding to the specific file. "destination of writing"
 *           buf -- buffer used to store message. "source of writing"
 *           nbytes -- number of bytes to be written.
 *  OUTPUTS : none
 *  RETURN VALUE : if succeed to write bytes, return the number of bytes written. 
 *                 if the file is read-only or the fd is not valid, return -1.
 *  SIDE EFFECTS : modify the fd file
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes){
    /* invalid fd */
    if ((fd >= MAX_FILE_NUM) || (fd < 0)) {                                                          // If fd is invalid, return -1
        printf("invalid file descriptor!\n");
        return -1;
    }
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));                     // Get the current pcb based on cur_process
    if(cur_pcb->file_array[fd].flags == 0) return -1;
    int32_t res = cur_pcb->file_array[fd].file_op_ptr->write(fd, buf, nbytes);                      // Call the corresponding write function
    return res;
}

/*
 * open
 *  DESCRIPTION : find the directory entry corresponding to the named file, allocate an unused file descriptor. 
 *  INPUTS : filename -- the name of file to be open. 
 *  OUTPUTS : none
 *  RETURN VALUE : if succeed to find the entry and allocate the fd, return fd. 
 *                 if the filename doesn't exist or no more fd can be allocated, return -1.
 *  SIDE EFFECTS : none.
 */
int32_t open (const uint8_t* filename){
    int32_t fd = -1;
    dentry_t dentry;
    int32_t i;
    if (-1 == read_dentry_by_name(filename, &dentry)) return -1;
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));                     // Get the current pcb based on cur_process
    for(i = 0; i < MAX_FILE_NUM; i++){
        if(0 == cur_pcb->file_array[i].flags){
            fd = i;                                                                                 // Traverse to get the "not busy" position
            break;
        }
    }
    if (-1 == fd){
        printf("file descriptor array is full right now!\n");                                         // Cannot find the "not busy" position
        return -1;
    }

    cur_pcb->file_array[fd].file_position = 0;                                                      // from the start of the file
    cur_pcb->file_array[fd].flags = 1;             // busy
    
    if (dentry.file_type == 0) {
        /* rtc type file */
        cur_pcb->file_array[fd].inode = 0;                                                          // not a data file
        cur_pcb->file_array[fd].file_op_ptr = &rtc_op;                                              // Set operation table
    }

    else if (dentry.file_type == 1) {
        /* directory type file */
        cur_pcb->file_array[fd].inode = 0;                                                          // not a data file
        cur_pcb->file_array[fd].file_op_ptr = &dir_op;                                              // Set operation table
    }

    else {
        /* regular file */ 
        cur_pcb->file_array[fd].inode = dentry.inode;                                               // a data file, set inode based on dentry
        cur_pcb->file_array[fd].file_op_ptr = &file_op;                                             // Set operation table
    }
    cur_pcb->file_array[fd].file_op_ptr->open(filename);                                            // Call the corresponding open function

    return fd;                                         
}

/*
 * close
 *  DESCRIPTION : closes the specified file descriptor and makes it available for return from later calls to open.
 *  INPUTS : fd -- file descriptor to be closed.
 *  OUTPUTS : none
 *  RETURN VALUE : if succeed to close the entry and free the fd, return 0.
 *                 if trying to close an invalid descriptor, return -1.
 *  SIDE EFFECTS : none.
 */
int32_t close (int32_t fd){
    if ((fd == 0) || (fd == 1) || (fd >= MAX_FILE_NUM) || (fd < 0)){                                   // If fd is invalid, return -1
        printf("invalid file descriptor!\n");
        return -1;
    }              
    
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));                     // Get the current pcb based on cur_process
    if(cur_pcb->file_array[fd].flags == 0) return -1;
    cur_pcb->file_array[fd].flags = 0; // available (not busy)    
    
    int32_t res =  cur_pcb->file_array[fd].file_op_ptr->close(fd);                                  // Call the corresponding close function
    return res;
}

/*
 * getargs
 *  DESCRIPTION : reads the program’s command line arguments into a user-level buffer.
 *  INPUTS : buf -- the user-level buffer to store args
 *           nbytes -- the number of bytes writtern into buffer
 *  OUTPUTS : none
 *  RETURN VALUE : -1 if there is no argument
 *                  0 if successfully read in arguments
 *  SIDE EFFECTS : modify the user-level buffer
 */
int32_t getargs (uint8_t* buf, int32_t nbytes){
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));                     // Get the current pcb based on cur_process
    int8_t* args = cur_pcb->args;
    if(args[0] == '\0' || (strlen((int8_t*)args) > nbytes)) return -1;                              // check the existence of argument, or avoid not fitting in the buffer
    strncpy((int8_t*)buf, args, nbytes);
    return 0;
}

/*
 * vidmap
 *  DESCRIPTION : maps the text-mode video memory into user space at a pre-set virtual address
 *  INPUTS : screen_start -- the pointer pointing at the screen address
 *  OUTPUTS : none
 *  RETURN VALUE : -1 if the pointer is out of the address range covered by the single user-level page
 *                  0 if successfully maps
 *  SIDE EFFECTS : modify the content of the pointer
 */
int32_t vidmap (uint8_t** screen_start){
    if((uint32_t)screen_start < user_virt_addr || (uint32_t)screen_start >= (user_virt_addr + PAGE_SIZE_4M)) return -1; // if the pointer is out of user-space range, return -1
    uint32_t video_dir_idx = (uint32_t)user_video_addr / PAGE_SIZE_4M;
    memset(&page_dir[video_dir_idx], 0, sizeof(page_dir[video_dir_idx]));                           // set PDE, user can access video mem. via virtual mem. 132M (4K page)
    page_dir[video_dir_idx].present = 1;                                                
    page_dir[video_dir_idx].read_write = 1;
    page_dir[video_dir_idx].base_addr = (uint32_t)(&page_tbl_usr_video) / PAGE_SIZE;    
    page_dir[video_dir_idx].user_sup = 1;                                                           // user accessible
    memset(&page_tbl_usr_video, 0, sizeof(page_table_entry_t));                                     // set PTE
    page_tbl_usr_video.present = 1;
    page_tbl_usr_video.read_write = 1;
    page_tbl_usr_video.base_addr = VMEM_START_ADDR / PAGE_SIZE;                                     // map to video mem.
    page_tbl_usr_video.user_sup = 1;                                                                // user accessible
    *screen_start = (uint8_t*)user_video_addr;                                                      // link the screen addr. to user video mem.
    flush_TLB();
    return 0;
}

int32_t set_handler (int32_t signum, void* handler_address){
    if(signum < 0 || signum >= NUM_SIGNAL) return -1;
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));
    if(handler_address != NULL){
        cur_pcb->sig_handler[signum] = handler_address;
    }else{
        cur_pcb->sig_handler[signum] = dft_sig_handler[signum];
    }
    return 0;
}

int32_t sigreturn (void){
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));
    uint32_t ebp;
    asm volatile(                                                                        
        "movl   %%ebp, %0\n"
        : "=r"(ebp)
    );
    context_t* linkage = (context_t*)(ebp + 2*sizeof(uint32_t));       // c_convention, ebp + 8 -> argument
    uint32_t user_esp = linkage->iret.ESP;
    context_t* context = (context_t*)(user_esp + sizeof(uint32_t));
    memcpy((void*)linkage, (void*)context, sizeof(context_t));
    cur_pcb->sig_mask = 0;
    return 0;
}

int32_t color (int32_t att){
    if(att >= 9 || att < 0) return -1;
    if(att == 8){
        rainbow_flag[sche_term] = 1;
        rainbow_ptr[sche_term] = 0;
        ATTRIB[sche_term] = colors[rainbow_ptr[sche_term]];
    }else{
        ATTRIB[sche_term] = colors[att];
        rainbow_flag[sche_term] = 0;
    }
    return 0;
}

int32_t ps (void){
    printf("PID  TERMINAL  STATE  CMD\n");
    uint8_t pid, term;
    pcb_t* pcb;
    for(pid = 0; pid < MAX_PROCESS; pid++){
        if(process_array[pid]){
            term = get_owner_terminal(pid);
            printf(" %d      %d      ", pid, term);
            if(active_array[term] == pid) printf(" RUN   ");
            else printf("BLOCK  ");
            pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (pid + 1));
            printf((int8_t*)(pcb->CMD));
            printf("\n");
        }
    }
    return 0;
}

int32_t cp (uint8_t* buf)
{
    int8_t   src[32 + 1] = {'\0'};                                               // leave 1 place for "\0"
    int8_t   dst[32 + 1] = {'\0'};                                               // leave 1 place for "\0"
    uint8_t i;
    int32_t fd_src, fd_dst, cnt;
    uint8_t cp_buf[1024];
    /* try to split the two arguments */
    uint32_t args_len = strlen((int8_t*)buf);
    for(i = 0; i <= args_len; i++){
        if(buf[i] == '\0' || buf[i] == ' '){
            uint32_t src_len = 32;
            if(src_len > i) src_len = i;
            strncpy(src, (int8_t*)buf, src_len);                                           // Store the exe_file given by the buf
            break;
        }
    }
    for(; i < args_len; i++){
        if(buf[i] != '\0' && buf[i] != ' '){
            strncpy(dst, (int8_t*)(buf + i), args_len - i);                                     // Store the argument given by the buf
            break;
        }
    }

    /* read dentries for two args*/
    dentry_t src_dentry, dst_dentry;
    if(-1 == read_dentry_by_name((uint8_t*)src, &src_dentry)){
        printf("cannot find file \"%s\"\n", (char*)src);
        return -1;
    }
    if(-1 == read_dentry_by_name((uint8_t*)dst, &dst_dentry)){
        printf("cannot find file \"%s\"\n", (char*)dst);
        return -1;
    }

    // inode_t* src_inode_ptr = inode_ptr + src_dentry.inode;
    // inode_t* dst_inode_ptr = inode_ptr + dst_dentry.inode;

    // memcpy(dst_inode_ptr, src_inode_ptr, 4096);

    fd_src = open((uint8_t*)src);
    fd_dst = open((uint8_t*)dst);

    while (0 != (cnt = file_read (fd_src, cp_buf, 1024))) 
    {
        if (-1 == cnt) {
	    printf("file read failed\n");
	    return -1;
	}
	if (-1 == file_write (fd_dst, cp_buf, cnt))
	    return -1;
    }

    return 0;
}

int32_t rm(uint8_t* buf)
{
    /* non-existent file */
    uint32_t name_len = strlen((char *)buf);

    if(buf == NULL) return -1;                                                                    // If the filename is NULL, return -1

    if (name_len > MAX_FILENAME_LEN) return -1;                                                                   // If the filename length is out of range, return -1

    int i, j;
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++){                                         // Traverse the dentries
        dentry_t* cur_dentry = &(dentry_ptr[i]);
        uint32_t dentry_len = strlen((int8_t*)(cur_dentry->file_name));
        if(dentry_len > MAX_FILENAME_LEN) dentry_len = MAX_FILENAME_LEN;
        if((name_len == dentry_len) && (!strncmp((int8_t*)buf, (int8_t*)(cur_dentry->file_name), name_len))){      // If we find the corresponding file, call read_dentry_by_index to write to the buffer
            break;                                                                              // Successfully find the file: return 0
        }
    }
    if(i == (boot_block_ptr->num_dir_entries)) return -1;
    for(j = i; j < (boot_block_ptr->num_dir_entries); j++)
    {
        memcpy(dentry_ptr + j, dentry_ptr + j + 1, 64);
    }
    return 0;                                                                                      // Cannot find the corresponding file after traversing: return -1
}

