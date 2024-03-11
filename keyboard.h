#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

/* define some consts */
#define KEYBOARD_IRQ_NUM 	1               // use pin#1 to connect the keyboard to master_PIC 
#define TABLE_SIZE 			59				// the size of keys_table

#define KEYBOARD_PORT 		0x60			// ps/2 port

/* define a structure to maintain flags. */
typedef struct flag_t {
	uint8_t capsLk_flag;                    // 1, capslk open, 0 caps close
	uint8_t ctrl_flag;                      // 1, ctrl pressed, 0 ctrl released
	uint8_t alt_flag;                       // 1, alt pressed, 0 alt released
	uint8_t shift_flag;                     // 1, shift pressed, 0 shift released
} flag_t;

/* define some modifier keys */
#define CAPS_LOCK_PRESS     0x3A			// The value of port refer to the website https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Sets.
#define LEFT_CTRL_PRESS     0x1D			
#define LEFT_CTRL_RELEASE   0x9D
#define LEFT_ALT_PRESS      0x38
#define LEFT_ALT_RELEASE    0xB8
#define LEFT_SHIFT_PRESS    0x2A
#define LEFT_SHIFT_RELEASE  0xAA
#define RIGHT_SHIFT_PRESS    0x36
#define RIGHT_SHIFT_RELEASE  0xB6
#define CURSOR_UP			 0x48
#define CURSOR_DOWN			 0x50
#define CURSOR_LEFT			 0x4B
#define CURSOR_RIGHT		 0x4D

/* define some function keys */
#define F1_PRESSED 0X3B
#define F2_PRESSED 0X3C
#define F3_PRESSED 0X3D

/* initialize the keyboard */
extern void keyboard_init(void);

/* handle the keyboard interrupt */
extern void keyboard_handler(void); 

/* handle modifier key. */
uint8_t is_modifier(uint32_t scancode);

#endif
