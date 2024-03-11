#ifndef _IDT_H
#define _IDT_H

#include "x86_desc.h"
#include "lib.h"

#define NUM_EXCEPTION   20
#define DPL_KERNEL      0
#define DPL_USER        3
#define PIT_VEC         0x20
#define KB_VEC          0x21
#define RTC_VEC         0x28
#define SYS_VEC         0x80

/* Initalize the IDT */
extern void init_idt(void);

/* fill gate entry */
void write_gate_entry(unsigned long n, uint32_t type, uint32_t dpl);

/* Define a structure that constains all the status of registers */
typedef struct __attribute__ ((packed)) reg_t{
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;
} reg_t;

#endif /* _IDT_H */
