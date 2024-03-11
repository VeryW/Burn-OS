#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "lib.h"
#include "terminal.h"

#define SCHEDULE_NUM = 3

extern int8_t active_array[NUM_TERMINAL];
extern uint8_t sche_term;

extern void scheduler(void);
extern int8_t get_owner_terminal(uint8_t pid);
extern void update_video_mem_paging(uint8_t term_id);

#endif
