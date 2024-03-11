/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "scheduler.h"
#include "system_call.h"

#define VIDEO       0xB8000

static int screen_x;
static int screen_y;
static char* video_mem = (char *)VIDEO;

uint32_t ATTRIB[NUM_TERMINAL] = {WHITE, WHITE, WHITE};
uint32_t colors[NUM_COLORS] = {RED, ORANGE, YELLOW, GREEN, BLUE, INDIGO, PURPLE, WHITE};
uint8_t rainbow_flag[NUM_TERMINAL] = {0, 0, 0};
uint8_t rainbow_ptr[NUM_TERMINAL] = {0, 0, 0};

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Used for user program. Clears video memory and put the cursor to the upper left corner. */
void clear(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB[sche_term];
    }
    /* put cursor to upper left corner */
    uint8_t term_id = sche_term;
    multi_terms[term_id].x = 0;
    multi_terms[term_id].y = 0;
    // screen_x = 0;
    // screen_y = 0;
    update_cursor(multi_terms[term_id].x, multi_terms[term_id].y);
}

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Used for keyboard interrupt. Clears video memory and put the cursor to the upper left corner. */
void clear_intr(void) {
    update_video_mem_paging(cur_terminal);
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB[cur_terminal];
    }
    /* put cursor to upper left corner */
    uint8_t term_id = cur_terminal;
    multi_terms[term_id].x = 0;
    multi_terms[term_id].y = 0;
    screen_x = 0;
    screen_y = 0;
    update_cursor(multi_terms[term_id].x, multi_terms[term_id].y);
    update_video_mem_paging(sche_term);
}

/**
 * update_cursor
 *  DESCRIPTION : update the cursor to the next print position.
 *  OUTPUTS : none.
 *  RETURN VALUE : none.
 *  SIDE EFFECTS : change the location of cursor on the VGA.
 */
void update_cursor(int x, int y){
    int pos = y * NUM_COLS + x;                             // reference https://wiki.osdev.org/Text_Mode_Cursor
	outb(0x0F, 0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

int32_t puts_intr(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc_intr(s[index]);
        index++;
    }
    return index;
}

/**
 * putc
 *  DESCRIPTION : Output a character to the console. For user program.
 *  INPUTS : a character to print.
 *  OUTPUTS : the input character.
 *  RETURN VALUE : none.
 *  SIDE EFFECTS : print the char on the terminal screen.
 */
void putc(uint8_t c) {
    uint32_t flags;
    cli_and_save(flags);                                    // avoid being interrupted by pit and executing scheduling
    uint8_t term_id = sche_term;
    uint8_t attrib = ATTRIB[sche_term];

    screen_x = multi_terms[term_id].x;
    screen_y = multi_terms[term_id].y;
    /* 1. handle the newline. */
    if(c == '\n') {                                              
        /* once screen_y reaches the lower bound of rows and key is "\n", scrolling the screen. */ 
        if (screen_y == (NUM_ROWS-1)){                      // newline will scroll the screen.
            scroll_screen(term_id);
        }
        /* normal case */
        else{
            screen_y++;
            screen_x = 0;           
        }

    }
    /* 2. handle the backspace. */
    else if (c == '\b'){ 
        if (screen_x==0 && screen_y==0){                    // if it it at the upper left corner, do nothing.
            return;
        }
        screen_x--;
        if (screen_x < 0){                                  // if it below 0, it should back to the upper line.
            screen_x = NUM_COLS - 1;
            screen_y --;
        }
        else{
            screen_x %= NUM_COLS;
            screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
        }
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = '\0';
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib;          
    }   
    /* 3. handle the normal characters. */
    else {
        /* once screen_x and screen_y reaches the below right corner, scrolling the screen */
        if (screen_y == (NUM_ROWS-1) && screen_x == (NUM_COLS-1)){
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib;
            scroll_screen(term_id);
        }
        /* once screen_x reaches the upper bound of column, newline */
        else if(screen_x == (NUM_COLS-1)){
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib;
            screen_x = 0;
            screen_y ++;
        }
        /* normal case */
        else{
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib;
            screen_x++;
            screen_x %= NUM_COLS;
            screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
        }
    }
    multi_terms[term_id].x = screen_x;
    multi_terms[term_id].y = screen_y;
    
    // if the terminal index is current terminal. update the cursor location.
    if(sche_term == cur_terminal){
        update_cursor(screen_x ,screen_y);
    }

    restore_flags(flags);
}

/**
 * putc_intr
 *  DESCRIPTION : Output a character to the console. For keybaoard interrupt.
 *  INPUTS : a character to print.
 *  OUTPUTS : the input character.
 *  RETURN VALUE : none.
 *  SIDE EFFECTS : print the char on the current screen.
 */
void putc_intr(uint8_t c) {
    uint32_t flags;
    int temp_x,temp_y;
    cli_and_save(flags);

    // update the video memory to termianl.
    update_video_mem_paging(cur_terminal);         
    uint8_t term_id = cur_terminal;
    uint8_t attrib = ATTRIB[cur_terminal];

    screen_x = multi_terms[term_id].x;
    screen_y = multi_terms[term_id].y;
    /* 1. handle the newline. */
    if(c == '\n') {                                              
        /* once screen_y reaches the lower bound of rows and key is "\n", scrolling the screen. */ 
        if (screen_y == (NUM_ROWS-1)){                      // newline will scroll the screen.
            scroll_screen(term_id);
        }
        /* normal case */
        else{
            screen_y++;
            screen_x = 0;           
        }
    }
    /* 2. handle the backspace. */
    else if (c == '\b'){ 
        if (screen_x==0 && screen_y==0){                    // if it it at the upper left corner, do nothing.
            return;
        }
        screen_x--;
        if (screen_x < 0){                                  // if it below 0, it should back to the upper line.
            screen_x = NUM_COLS - 1;
            screen_y --;
        }
        else{
            screen_x %= NUM_COLS;
            screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
        }   
        
        temp_x = screen_x;
        temp_y = screen_y;
        while (*(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) != '\0' && *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) != ' '){
            /* left scroll the characters */
            scroll_left_char(temp_x, temp_y,attrib);
            /* update the temp_x and temp_y */
            if (temp_x == (NUM_COLS-1) && temp_y == (NUM_ROWS-1)){
                return;
            }
            else if (temp_x == (NUM_COLS-1)){
                temp_x = 0;
                temp_y ++;
            }
            else{
                temp_x++;
            }
        }

    }   
    /* 3. handle the normal characters. */
    else {
        // /* once screen_x and screen_y reaches the below right corner, scrolling the screen */
        if (screen_y == (NUM_ROWS-1) && screen_x == (NUM_COLS-1)){
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib;
            scroll_screen(term_id);
        }
        scroll_right_char(screen_x, screen_y, c,attrib);
        // /* once screen_x reaches the upper bound of column, newline */
        if(screen_x == (NUM_COLS-1)){
            screen_x = 0;
            screen_y ++;
        }
        /* normal case */
        else{
            screen_x++;
            screen_x %= NUM_COLS;
            screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
        }
    }
    multi_terms[term_id].x = screen_x;
    multi_terms[term_id].y = screen_y;
    /* update the cursor. */
    update_cursor(screen_x ,screen_y);

    // restore the terminal video memory.
    update_video_mem_paging(sche_term);

    restore_flags(flags);
}

void scroll_right_char(int temp_x, int temp_y, uint8_t c, uint8_t attrib){
    uint8_t temp;
    if (*(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) == '\0' || *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) == ' '){
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1) + 1) = attrib;
        return;
    }
    else if (temp_x == (NUM_COLS-1))
    {
        temp = *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1));
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1) + 1) = attrib;
        temp_x = 0;
        temp_y = temp_y + 1;
        scroll_right_char(temp_x, temp_y, temp,attrib);
    }
    else{
        temp = *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1));
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1) + 1) = attrib;
        temp_x = temp_x + 1;
        temp_y = temp_y;
        scroll_right_char(temp_x, temp_y, temp,attrib);
    }
}

void scroll_left_char(int temp_x, int temp_y, uint8_t attrib){
    int next_x, next_y;
    if (temp_x == (NUM_COLS-1) && temp_y == (NUM_ROWS-1)){
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) = '\0';
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1) + 1) = attrib;
    }
    else if (temp_x == (NUM_COLS-1))
    {
        next_x = 0;
        next_y = temp_y+1;
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * next_y + next_x) << 1));
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1) + 1) = attrib;
    }
    else{
        next_x = temp_x+1;
        next_y = temp_y; 
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * next_y + next_x) << 1));
        *(uint8_t *)(video_mem + ((NUM_COLS * temp_y + temp_x) << 1) + 1) = attrib;
    }
    return;
}

/**
 * scroll_screen
 *  DESCRIPTION : when the screen_y==NUM_ROWS-1, the function will be called.
 *  INPUTS : the terminal index. 
 *  OUTPUTS : none.
 *  RETURN VALUE : none.
 *  SIDE EFFECTS : finish the scrolliing task. reset screen_x and maintain screen_y.
 */
void scroll_screen(uint8_t term_id){
    int i,j;
    uint8_t attrib = ATTRIB[term_id];
    for (j = 0; j < NUM_ROWS; j++){
        for(i = 0; i < NUM_COLS; i++){
            /* clear the bottom line */
            if (j == (NUM_ROWS-1)){
                /* clear the bottom line */
                *(uint8_t *)(video_mem + ((NUM_COLS * j + i) << 1)) = ' ';
                *(uint8_t *)(video_mem + ((NUM_COLS * j + i) << 1) + 1) = attrib;   
            }
            else{
                /* replace the current line by the below line. */
                *(uint8_t *)(video_mem + ((NUM_COLS * j + i) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * (j+1) + i) << 1));
                *(uint8_t *)(video_mem + ((NUM_COLS * j + i) << 1) + 1) = attrib;   
            }
        }
    }
    /* reset screen_x. screen_y won't change.*/
    multi_terms[term_id].x = 0;
    screen_x = 0;
}

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}
