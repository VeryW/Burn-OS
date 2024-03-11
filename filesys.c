#include "filesys.h"
#include "lib.h"
#include "system_call.h"
#include "x86_desc.h"
#include "terminal.h"
int inode_array[64];
uint8_t data_blocks_bitmap[BLOCK_SIZE] = {0}; 
/**
 * filesys_init
 *  DESCRIPTION : initialize pointers relevant to the filesystem
 *  INPUTS : uint32_t filesys_addr - the address of boot block
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : none
 * 
 */
void filesys_init (uint32_t filesys_addr)
{
    int i,j;
    boot_block_ptr = (boot_block_t*)filesys_addr;                                                   // cast filesys_addr to boot_block_t
    inode_ptr = (inode_t*) (boot_block_ptr + 1);                                                    // Pointing to the first inode block, 1 means the next block of boot block
    dentry_ptr = (dentry_t*) boot_block_ptr->dir_entries;                                           // Pointing to the first dentry
    data_block_ptr = (uint8_t*) (inode_ptr + boot_block_ptr->num_inodes);                           // Pointing to the first data block
    memset(&inode_array[0], 0, sizeof(inode_array[0]));
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++)
    {
        inode_array[dentry_ptr[i].inode] = 1;                                                       // Set it to be busy status
    }
    char temp[MAX_FILENAME_LEN] = {'\0'};
    /* init the all_file_names array */
    for (j=0; j<MAX_FILES_NUMBER; j++){
        memcpy(all_file_names[j], temp, MAX_FILENAME_LEN);
    }
    /* assgin the file name into array. */
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++){
        dentry_t temp_dentry;
        read_dentry_by_index(i,&temp_dentry);
        strncpy((char*)(all_file_names[i]),(char*)(temp_dentry.file_name), MAX_FILENAME_LEN);
    }
    /*initialize the data blocks bitmap */
    for (i = 0; i < MAX_FILES_NUMBER; i++ ){
        inode_t* cur_inode = inode_ptr + i;
        for (j = 0; j < cur_inode->length/BLOCK_SIZE; j++){
            uint32_t D = cur_inode->data_blocks[j];
            if (D >= BLOCK_SIZE ){
                printf("oops! bitmap is not big enough!");
                return;
            }
            data_blocks_bitmap[D] = 1;
        }
    }
}

/**
 * read_dentry_by_name
 *  DESCRIPTION : read the corresponding file dentry to the given
 *                dentry based on the given filename
 *  INPUTS : const uint8_t* fname - given filename
 *           dentry_t* dentry - given dentry
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully load the dentry
 *                 -1 - cannot find the corresponding file
 *  SIDE EFFECTS : none
 * 
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
    /* non-existent file */
    uint32_t name_len = strlen((char *)fname);

    if(fname == NULL) return -1;                                                                    // If the filename is NULL, return -1

    if (name_len > MAX_FILENAME_LEN) return -1;                                                                   // If the filename length is out of range, return -1

    int i;
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++){                                         // Traverse the dentries
        dentry_t* cur_dentry = &(dentry_ptr[i]);
        uint32_t dentry_len = strlen((int8_t*)(cur_dentry->file_name));
        if(dentry_len > MAX_FILENAME_LEN) dentry_len = MAX_FILENAME_LEN;
        if((name_len == dentry_len) && (!strncmp((int8_t*)fname, (int8_t*)(cur_dentry->file_name), name_len))){      // If we find the corresponding file, call read_dentry_by_index to write to the buffer
            read_dentry_by_index(i, dentry);
            return 0;                                                                               // Successfully find the file: return 0
        }
    }
    return -1;                                                                                      // Cannot find the corresponding file after traversing: return -1
}

int32_t find_similar_file(char* line_buffer, char* buf){
    uint32_t cmd_len = strlen(line_buffer);
    int8_t   exe_file[MAX_FILENAME_LEN + 1] = {'\0'};                                               // leave 1 place for "\0"
    int8_t   args[BUFFER_SIZE + 1] = {'\0'};                                                        // leave 1 place for "\0"
    uint8_t i;
    uint32_t exe_len = 0;
    uint32_t args_len = 0;
    uint32_t empty_len = 0;
    for(i = 0; i <= cmd_len; i++){
        if(line_buffer[i] == '\0' || line_buffer[i] == ' '){
            exe_len = MAX_FILENAME_LEN;
            if(exe_len > i) exe_len = i;
            strncpy(exe_file, (int8_t*)line_buffer, exe_len);                                           // Store the exe_file given by the command
            break;
        }
    }
    for(; i < cmd_len; i++){
        if (line_buffer[i] == ' '){
           empty_len++; 
        }
        if(line_buffer[i] != '\0' && line_buffer[i] != ' '){
            strncpy(args, (int8_t*)(line_buffer + i), cmd_len - i);                                     // Store the argument given by the command
            args_len = cmd_len-i;
            break;
        }
    }

    /* autocomplete the exe_file */
    dentry_t exe_dentry;
    uint8_t exe_check[4];
    exe_len = strlen(exe_file);
    if (args_len == 0 && empty_len == 0){
        for(i=0; i<(boot_block_ptr->num_dir_entries); i++){
            if (!strncmp(exe_file, all_file_names[i], exe_len)){
                /* find the matched file name */
                if (multi_terms[cur_terminal].flag_cnt >= 1){
                    multi_terms[cur_terminal].flag_cnt = 0;
                    memset(exe_file, '\0', strlen(exe_file));
                    memset(args, '\0', strlen(args));
                    return 1;
                }
                /* check whether it is exe file. */
                read_dentry_by_name((uint8_t*)all_file_names[i], &exe_dentry);
                read_data(exe_dentry.inode, 0, exe_check, 4);                                                   // Read the 4 bytes that denote the executable file
                if((exe_check[0] != 0x7f) || (exe_check[1] != 0x45) || (exe_check[2] != 0x4c) || (exe_check[3] != 0x46)){       // Compare with 4 bytes that denote executable file
                    multi_terms[cur_terminal].flag_cnt = 0;
                    memset(exe_file, '\0', strlen(exe_file));
                    memset(args, '\0', strlen(args));
                    return -1;
                }
                memcpy(buf, all_file_names[i], strlen(all_file_names[i]));
                multi_terms[cur_terminal].flag_cnt++;
            }
        }
    }
    /* if cnt=0 or larger than 1, (no matched or more than one matched), return -1. */
    if (multi_terms[cur_terminal].flag_cnt == 0){
        memset(exe_file, '\0', strlen(exe_file));
        memset(args, '\0', strlen(args));
        return -1;
    }

    memset(exe_file, '\0', strlen(exe_file));
    memset(args, '\0', strlen(args));
    multi_terms[cur_terminal].flag_cnt = 0;
    return 0;
}

/**
 * read_dentry_by_index
 *  DESCRIPTION : read the corresponding file dentry to the given
 *                dentry based on the given index
 *  INPUTS : uint32_t index - given index
 *           dentry_t* dentry - given dentry
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully load the dentry
 *                 -1 - cannot find the corresponding file
 *  SIDE EFFECTS : none
 * 
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry)
{
    if (index >= boot_block_ptr->num_dir_entries) return -1;                                        // If the input index greater than the dentries number, return -1

    dentry_t* target = dentry_ptr + index;                                                          // Find the target that we need to copy from

    strncpy((char *)dentry->file_name, (char *)target->file_name, MAX_FILENAME_LEN);                // Copy the filename to dentry->file_name
    dentry->file_type = target->file_type;                                                          // Copy other fields
    dentry->inode     = target->inode;
    return 0;
}

/* read up to "length" bytes starting from position "offset" in the file with number inode*/
/**
 * read_data
 *  DESCRIPTION : read the data with "length" into buffer based on "inode"
                  and "offset" in that file 
 *  INPUTS : uint32_t inode - given inode: find the index node
             uint32_t offset - the offset in the file
             uint8_t* buf - the buffer we want to write data to
             uint32_t length - the length of the data we want to write
 *  OUTPUTS : none
 *  RETURN VALUE : bytes_copied - the number of bytes copied to the buffer
                   -1 - fail to copy data
 *  SIDE EFFECTS : none
 * 
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) 
{
    /* invalid inode number */
    if (inode >= boot_block_ptr->num_inodes) return -1;     
    
    inode_t* target_inode = inode_ptr + inode;

    /* invalid offset*/
    if (offset > target_inode->length) return 0;                                             // successfully read 0 bytes
    /* eliminate extra length */
    if (length > target_inode->length - offset) length = target_inode->length - offset;      // if beyond end of file 
    /* tricky length */
    if (length == 0) return 0;                                                               // if reading 0 bytes                 

    uint32_t start_block_idx = offset / BLOCK_SIZE;                                          // each data block takes 4kB
    uint32_t start_block_offset = offset % BLOCK_SIZE;                                       // offset in given data block
    uint32_t bytes_copied = 0;                                                               // holding total bytes being copied

    uint32_t D = target_inode->data_blocks[start_block_idx];                                 // index within data block array
    
    /* The start address of reading*/
    uint8_t* read_ptr = data_block_ptr + BLOCK_SIZE*D + start_block_offset;                  // start reading data from this address
    uint32_t left_len = BLOCK_SIZE - start_block_offset;                                     // how many bytes still need to copy in given data block

    while (length > left_len) {

        length -= left_len;                                                                  // update length
        memcpy(buf, read_ptr, left_len);                                                     // copy left bytes into buf
        buf += left_len;                                                                     // update buf pointer
        bytes_copied += left_len;                                                            // accumulate copied bytes
        
        start_block_idx++;                                                                   // update index to data block
        D = target_inode->data_blocks[start_block_idx];                                      // update D
        read_ptr = data_block_ptr + BLOCK_SIZE*D;                                            // update read_ptr
        left_len = BLOCK_SIZE;

    }

    /* last block of data to read */
    memcpy(buf, read_ptr, length);                                          
    buf += length;
    bytes_copied += length;                                                                  // length is less than BLOCK_SIZE

    return bytes_copied;


}

/**
 * find_empty_block
 *  DESCRIPTION : find one data block that has not been used
 *  INPUTS : none
 *  OUTPUTS : the index in data blocks array
 *  SIDE EFFECTS : none 
 * 
 */
int32_t find_empty_block (void)
{   uint16_t i;
    for (i = 0; i < BLOCK_SIZE; i++){
        if (data_block_ptr[i] == 0) {
            data_block_ptr[i] = 1;
            return i;
        } 
    }
    return -1;
}


/* write up to "length" bytes starting from the bottom of the file with number inode*/
/**
 * read_data
 *  DESCRIPTION : write up to "length" bytes starting from the bottom
 *                of the file with number inode 
 *  INPUTS : uint32_t inode - given inode: find the index node
             uint8_t* buf - the buffer loading the bytes to write
             uint32_t length - the length of the data we want to write
 *  OUTPUTS : none
 *  RETURN VALUE : bytes_copied - the number of bytes written to the file
                   -1 - fail to copy data
 *  SIDE EFFECTS : none
 * 
 */
int32_t write_data (uint32_t inode, const uint8_t* buf, uint32_t length) 
{
    /* invalid inode number */
    if (inode >= boot_block_ptr->num_inodes) return -1;     
    
    inode_t* target_inode = inode_ptr + inode;

    uint32_t bytes_written = 0;                                                              // holding total bytes being written

    uint32_t max_left_space = (BLOCK_SIZE/4 - 1)*BLOCK_SIZE - target_inode->length;
    /* eliminate extra length */
    if (length > max_left_space) length = max_left_space;                                   // if beyond end of file 
    /* tricky length */
    if (length == 0) return 0;                                                              // if reading 0 bytes                 

    int32_t D;

    uint32_t last_block_idx = target_inode->length / BLOCK_SIZE;                            // each data block takes 4kB
    
    uint32_t last_block_offset = target_inode->length % BLOCK_SIZE;                         // offset in given data block

    if (target_inode->length == 0){
        D = find_empty_block();
        if (D == -1) {
            printf("Oops! The file system is full right now!");
            return bytes_written;
        }
        target_inode->data_blocks[0] = D;
        
    } else {
        D = target_inode->data_blocks[last_block_idx];                                 // index within data block array
    }

    /* The start address of writing*/
    uint8_t* write_ptr = data_block_ptr + BLOCK_SIZE*D + last_block_offset;                  // start reading data from this address
    uint32_t left_len = BLOCK_SIZE - last_block_offset;                                      // how many bytes still need to copy in given data block

    while (length > left_len){
        length -= left_len;
        memcpy(write_ptr, buf, left_len);
        buf += left_len;
        bytes_written += left_len;

        int32_t new_D = find_empty_block();
        if (new_D == -1) {
            printf("Oops! The file system is full right now!");
            return bytes_written;
        }
        target_inode->data_blocks[last_block_idx+1] = new_D;
        write_ptr = data_block_ptr + BLOCK_SIZE*new_D;
        left_len = BLOCK_SIZE;

    }
    
    /* last block of data to read */
    memcpy(write_ptr, buf, length);                                          
    buf += length;
    bytes_written += length;                                                                  // length is less than BLOCK_SIZE
    target_inode->length += bytes_written;
    return bytes_written;
}


/**
 * file_read
 *  DESCRIPTION : load n bytes data to the given buffer based on given fd
 *  INPUTS : int32_t fd - file descriptor / **for cp2 only**: refers to inode index here
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully write data to the given buffer
                   -1 - fail to write data to the given buffer
 *  SIDE EFFECTS : none 
 * 
 */
int32_t file_read (int32_t fd, void* buf, int32_t nbytes)                                               
{
    pcb_t* cur_pcb_ptr = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));
    file_desc_t file_desc = cur_pcb_ptr->file_array[fd];
    
    int32_t bytes_copied = read_data(file_desc.inode, file_desc.file_position, buf, nbytes);                                                       // fd refers to inode index here, 0 means read from the start of file. **for cp2 only**
    cur_pcb_ptr->file_array[fd].file_position += bytes_copied;
    return bytes_copied;
}

/**
 * file_write
 *  DESCRIPTION : do nothing and return -1
 *  INPUTS : int32_t fd - file descriptor
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : -1 - always (for read-only)
 *  SIDE EFFECTS : none
 * 
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
    pcb_t* cur_pcb_ptr = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));
    file_desc_t file_desc = cur_pcb_ptr->file_array[fd];

    int32_t bytes_written = write_data(file_desc.inode, buf, nbytes);
    return bytes_written;                                                                                  // always return -1(read only)
}

/**
 * file_open
 *  DESCRIPTION : check whether we can open a file
 *  INPUTS : const uint8_t* filename - the name of the file we want to open
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - there exists the file that we want to open
                   -1 - there is no corresponding file
 *  SIDE EFFECTS : none
 * 
 */
int32_t file_open (const uint8_t* filename)
{
    dentry_t dentry;
    return read_dentry_by_name(filename,&dentry);                                              // Call read_dentry_by_name and use its return value
}

/**
 * file_close
 *  DESCRIPTION : do nothing and return 0
 *  INPUTS : int32_t fd - file descriptor
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - always
 *  SIDE EFFECTS : none
 * 
 */
int32_t file_close (int32_t fd)
{
    return 0;                                                                                  // Always return 0
}

/**
 * dir_read
 *  DESCRIPTION : load n bytes data to the given buffer based on given fd
 *  INPUTS : int32_t fd - file descriptor / **for cp2 only**: refers to dentry index here
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully write data to the given buffer
                   -1 - fail to write data to the given buffer
 *  SIDE EFFECTS : none 
 * 
 */
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes)
{
    int ret;
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));
    dentry_t dentry;
    /* subsequent reads until the last is reached, at which point read should repeatedly return 0.*/
    if ((cur_pcb->file_array[fd].file_position == boot_block_ptr->num_dir_entries) || (cur_pcb->file_array[fd].file_position == MAX_FILES_NUMBER)){
        return 0;
    }
    ret = read_dentry_by_index(cur_pcb->file_array[fd].file_position, &dentry);
    if (ret == -1) return -1;                                                 
    cur_pcb->file_array[fd].file_position += 1;
    uint32_t len = MAX_FILENAME_LEN;
    if(strlen((const int8_t*)dentry.file_name) < len) len = strlen((const int8_t*)dentry.file_name);
    strncpy((int8_t*)buf, (int8_t*)dentry.file_name, len);
    return len;
}

/**
 * dir_write
 *  DESCRIPTION : do nothing and return -1
 *  INPUTS : int32_t fd - file descriptor
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : -1 - always (for read-only)
 *  SIDE EFFECTS : none
 * 
 */
int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes)
{
    int i;
    strcpy((int8_t*)dentry_ptr[boot_block_ptr->num_dir_entries].file_name, (int8_t*)buf);
    dentry_ptr[boot_block_ptr->num_dir_entries].file_type = 2;
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++)
    {
        if(inode_array[i] == 0)
        {
            dentry_ptr[boot_block_ptr->num_dir_entries].inode = i;
            inode_array[i] = 1;
            break;
        }
    }
    if(i == boot_block_ptr->num_dir_entries)
    {
        return -1;
    }
    boot_block_ptr->num_dir_entries++;
    return 0;                                                                                  // Always return -1 (read-only)
}

/**
 * dir_open
 *  DESCRIPTION : check whether we can open a directory
 *  INPUTS : const uint8_t* filename - the name of the file we want to open
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - there exists the directory that we want to open (we only use this since there is only one directory)
                   -1 - there is no corresponding directory
 *  SIDE EFFECTS : none
 * 
 */
int32_t dir_open (const uint8_t* filename)
{
    dentry_t dentry;
    return read_dentry_by_name(filename, &dentry);                                              // Call read_dentry_by_name and use its return value
}

/**
 * dir_close
 *  DESCRIPTION : do nothing and return 0
 *  INPUTS : int32_t fd - file descriptor
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - always
 *  SIDE EFFECTS : none
 * 
 */
int32_t dir_close (int32_t fd)
{
    return 0;                                                                                   // Always return 0
}
