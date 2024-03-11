#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"
#include "system_call.h"
#include "paging.h"
#include "scheduler.h"

uint8_t volatile cur_terminal = 0;

terminal_t multi_terms[NUM_TERMINAL];
uint32_t back_video_buf_addr[NUM_TERMINAL] = {BACK_VID_1, BACK_VID_2, BACK_VID_3};

/**
 * terminal_open
 *  DESCRIPTION : open and initialize three terminals
 *  INPUTS : filename, ignored here.
 *  OUTPUTS : none
 *  RETURN VALUE : 0
 *  SIDE EFFECTS : set the initialization state.
 */
int32_t terminal_open(const uint8_t* filename) {
    int i,j;
    char temp[BUFFER_SIZE] = {'\0'};
    for (i=0; i < NUM_TERMINAL; i++){
        multi_terms[i].count= 0;
        multi_terms[i].read_open = 0;
        multi_terms[i].enter_flag = 0;
        multi_terms[i].cur_buffer_id = 0;
        multi_terms[i].buf_count = 0;
        multi_terms[i].x = 0;
        multi_terms[i].y = 0;
        multi_terms[i].char_location = 0;
        multi_terms[i].flag_cnt = 0;
        for (j = 0; j < BUFFER_SIZE; j++){
            multi_terms[i].line_buffer[j] = '\0';
        }
        for (j=0; j<HISTORY_SIZE; j++){
            memcpy(multi_terms[i].history_line_buf[j], temp, BUFFER_SIZE);
        }
    }
    return 0;
}

/**
 * terminal_read
 *  DESCRIPTION : read the user input with nbytes from linebuffer to the user buffer.
 *  INPUTS : fd - file descriptor index. ignored here.
 *           buf - user buffer, the destination of copy.
 *           nbytes - the number of bytes can be read.
 *  OUTPUTS : none
 *  RETURN VALUE : return the number of bytes successfully read to the user buffer. otherwise return -1.
 *  SIDE EFFECTS : 
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    int i;
    int num_to_be_read;
    /* check valid */
    if (nbytes < 0 || buf == NULL){                        
        return -1;
    }
    multi_terms[sche_term].read_open = 1;
    /* user is input something, wait the enter pressed. */
    while (!multi_terms[sche_term].enter_flag){

    };
    /* the number to be copied should be min(nbytes, count) */
    if (multi_terms[sche_term].count < nbytes){                        
        num_to_be_read = multi_terms[sche_term].count;                 // avoid overflow.
    }
    else{
        num_to_be_read = nbytes;                           
    }
    /* once enter pressed, copy data from line buffer to user buffer. */
    for (i = 0; i < num_to_be_read; i++){                   // copy from line_buffer to user buffer.
        ((char*) buf)[i] = multi_terms[sche_term].line_buffer[i];
        (multi_terms[sche_term].history_line_buf[multi_terms[sche_term].buf_count])[i] = multi_terms[sche_term].line_buffer[i];
    }

    ((char*) buf)[num_to_be_read] = '\n';     
    if (strlen(multi_terms[sche_term].history_line_buf[multi_terms[sche_term].buf_count]) != 0) {
        (multi_terms[sche_term].history_line_buf[multi_terms[sche_term].buf_count])[num_to_be_read] = '\0';
    }             
    /* once copy over, the line buffer should be cleared. */
    if (strlen(multi_terms[sche_term].history_line_buf[multi_terms[sche_term].buf_count]) != 0){
        multi_terms[sche_term].buf_count++;
        multi_terms[sche_term].cur_buffer_id = multi_terms[sche_term].buf_count;
    }
    multi_terms[sche_term].count = 0;
    multi_terms[sche_term].char_location = 0;
    multi_terms[sche_term].enter_flag = 0;
    multi_terms[sche_term].read_open = 0;
    for (i = 0; i < BUFFER_SIZE; i++){
        multi_terms[sche_term].line_buffer[i] = '\0';
    }

    return num_to_be_read;
}

/**
 * terminal_write
 *  DESCRIPTION : wrtie nbytes from user buffer to screen directly. 
 *  INPUTS : fd - file descriptor index.
 *           buf - user buffer, the destination of copy.
 *           nbytes - the number of bytes can be written to the screen.
 *  OUTPUTS : none
 *  RETURN VALUE : return number of bytes successfully written. otherwise return -1.
 *  SIDE EFFECTS : 
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    int i;
    if (nbytes < 0 || buf == NULL){                         // check valid
        return -1;
    }
    for (i = 0; i < nbytes; i++){
        if (!((char*)buf)[i] == 0x0){                      // not print the NULL bytes.
            putc(((char*)buf)[i]);
        }
    }
    return nbytes;
}

/**
 * terminal_close
 *  DESCRIPTION : close the terminal and clear any terminal specific variables.
 *  INPUTS : fd - file descriptor index.
 *  OUTPUTS : none
 *  RETURN VALUE : 0
 *  SIDE EFFECTS : none
 */
int32_t terminal_close(int32_t fd){
    return 0;
}

/*
 * fill_line_buffer
 *  DESCRIPTION : put a character into line_buffer. 
 *  INPUTS : uint8_t c, a character to be handled.
 *  OUTPUTS : none
 *  RETURN VALUE : if read mode open and overlfow, return 0. otherwise return 1.
 *                 return 1, means the char will also show on the screen. return 0, means it won't show on the scree.
 *  SIDE EFFECTS : fill a char into line_buffer.
 */ 
int32_t fill_line_buffer(uint8_t c){
    int i;
    /* only read mode open, the line buffer will be filled.*/
    if (!multi_terms[cur_terminal].read_open){
        return 1;
    }
    /* 1. handle the enter */
    if (c == '\n'){    
        multi_terms[cur_terminal].line_buffer[multi_terms[cur_terminal].count] = '\n';
        multi_terms[cur_terminal].enter_flag = 1;
        if(rainbow_flag[cur_terminal]){
            rainbow_ptr[cur_terminal] = (rainbow_ptr[cur_terminal] + 1) % (NUM_COLORS - 1);
            ATTRIB[cur_terminal] = colors[rainbow_ptr[cur_terminal]];
        }
        return 1;  
    }
    /* 2. handle the backspace */
    if (c == '\b'){
        if (multi_terms[cur_terminal].count == 0){                                // cannot backspace anymore.
            return 0;                                               // it shouldn't erase the former video memory.
        }
        multi_terms[cur_terminal].count--;
        multi_terms[cur_terminal].char_location--;
        multi_terms[cur_terminal].line_buffer[multi_terms[cur_terminal].char_location] = '\0';                // clear the last char.
        /* scroll the characters ahead */
        for (i=multi_terms[cur_terminal].char_location; i<multi_terms[cur_terminal].count; i++){
            multi_terms[cur_terminal].line_buffer[i] =  multi_terms[cur_terminal].line_buffer[i+1]; 
        }
        return 1;
    }
    /* 3. handle the normal characters */
    if (multi_terms[cur_terminal].count < (BUFFER_SIZE-1)){                       // 127 characters.
        for (i=multi_terms[cur_terminal].count; i>multi_terms[cur_terminal].char_location; i--){
            multi_terms[cur_terminal].line_buffer[i] =  multi_terms[cur_terminal].line_buffer[i-1]; 
        }
        multi_terms[cur_terminal].line_buffer[multi_terms[cur_terminal].char_location] = c;
        multi_terms[cur_terminal].count++;
        multi_terms[cur_terminal].char_location++;
        return 1;
    }
    return 0;
}

/*
 * terminal_switch
 *  DESCRIPTION : switch currently seen terminal
 *  INPUTS : uint8_t term_id -- new foreground terminal index 
 *  OUTPUTS : none
 *  RETURN VALUE : none.
 *  SIDE EFFECTS : switch the foreground terminal 
 */ 
void terminal_switch(uint8_t term_id){
    int8_t target_term_id = term_id;
    if(target_term_id == cur_terminal || target_term_id < 0) return;

    /* update_video_memory_paging(current_terminal) */
    update_video_mem_paging(cur_terminal);

    /* Copy from video memory to background buffer of current_terminal */
    memcpy((void*)(back_video_buf_addr[cur_terminal]), (void*)VMEM_START_ADDR, SIZE_4KB);

    /* Load background buffer of target_terminal into video memory */
    memcpy((void*)VMEM_START_ADDR, (void*)(back_video_buf_addr[(uint8_t)target_term_id]), SIZE_4KB);     
    
    /* update cursor to target terminal's */
    update_cursor(multi_terms[(uint8_t)target_term_id].x, multi_terms[(uint8_t)target_term_id].y);

    cur_terminal = target_term_id;

    /* update_video_memory_paging(get_owner_terminal(current_pid)) */
    update_video_mem_paging(sche_term);
} 
