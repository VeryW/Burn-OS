#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"
#include "idt.h"

#define NUM_SIGNAL      5
#define DIV_ZERO        0
#define SEGFAULT        1
#define INTERRUPT       2
#define ALARM           3
#define USER1           4

typedef struct __attribute__ ((packed)) iret_t{
    uint32_t return_addr;
    uint32_t CS;
    uint32_t EFLAGS;
    uint32_t ESP;
    uint32_t SS;
} iret_t;

typedef struct __attribute__ ((packed)) context_t{
    reg_t    regs;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t IRQ_EXCEP;
    uint32_t error;
    iret_t   iret;
} context_t;

extern uint8_t SIGINT_flag;

extern context_t context;

extern void* dft_sig_handler[NUM_SIGNAL];

extern void send_signal(int8_t sig_num);

extern void do_signal(context_t context);

extern void EXEC_sigreturn(void);

extern void END_of_exec_sigreturn(void);

extern void setup_frame(uint8_t signum, reg_t regs, uint32_t IRQ_EXCEP, uint32_t error, iret_t iret_context);

#endif
