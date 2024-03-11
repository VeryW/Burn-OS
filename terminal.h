#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

#define NUM_TERMINAL 3
#define BUFFER_SIZE 128                    /* keyboard buffer size */       
#define HISTORY_SIZE 100

/* define a structure to terminal. */
typedef struct terminal_t
{
	char  	history_line_buf[HISTORY_SIZE][BUFFER_SIZE];				/* history line buffer */
	char  	line_buffer[BUFFER_SIZE];     		    /* buffer */
	int 	cur_buffer_id;							/* current line buffer index */
	int 	buf_count;							/* the number of line buffer stored in history*/
	int		count;                    		    /* number of elements in buffer  */
	uint8_t read_open;							/* to tell the keyboard it's read. */
	uint8_t enter_flag;							/* synchorize the terminal and keyboard interrupt. */ 
	int		x;									/* current x coordinate of video mem */
	int		y;									/* current y coordinate of video mem */
	int		char_location;
	int		flag_cnt;
}terminal_t;

extern volatile uint8_t cur_terminal;

extern terminal_t multi_terms[NUM_TERMINAL];

extern uint32_t back_video_buf_addr[NUM_TERMINAL];

/* open the terminal. */
extern int32_t terminal_open(const uint8_t* filename);

/* close the terminal. */
extern int32_t terminal_close(int32_t fd);

/* read nbytes from user input to buf. */
extern int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

/* write nbytes from buf to screen directly.*/
extern int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

/* put a char into buffer */
extern int32_t fill_line_buffer(uint8_t c);

extern void terminal_switch(uint8_t term_id);

#endif
