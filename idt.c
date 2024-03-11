#include "idt.h"
#include "handler.h"
#include "system_call.h"

/* gate types */
uint32_t trap_gate = 0xF;
uint32_t intr_gate = 0xE;
uint32_t task_gate = 0x5;

/* Define a structure object that contains all the name of EXCEPTION */
static char* EXCEPTION_NAME[NUM_EXCEPTION] = {
    "Divide Error",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection",
    "Page-Fault",
    "Reserved",
    "Floating-Point Error",
    "Alignment Check",
    "Machine-Check",
    "SIMD Floating-Point"
};

/**
 * write_gate_entry:
 *  DESCRIPTION : fill the idt gate entry based on the gate type. 
 *  INPUTS : n -- the index of idt entry
 *           type -- the gate type
 *           dpl -- descriptor priority level of accessing the entry.
 *  OUTPUTS : none 
 *  RETURN VALUE : none
 *  SIDE EFFECTS : The entries of IDT has been filled.
 */
void write_gate_entry(unsigned long n, uint32_t type, uint32_t dpl){
    /* gate_type = size | reserved1 | reserved2 | reserved3 */
    idt[n].seg_selector = KERNEL_CS;
    idt[n].reserved4 = 0x00;
    idt[n].reserved3 = (type & 0x1);       // (IF change or not) the first bit of a specific gate
    idt[n].reserved2 = (type & 0x2) >> 1;  // the second bit of a specific gate
    idt[n].reserved1 = (type & 0x4) >> 2;  // the third bit of a specific gate
    idt[n].size = (type & 0x8) >> 3;       // (size) the fourth bit of a specific gate
    idt[n].reserved0 = 0;                  // 
    idt[n].dpl = dpl;
    idt[n].present = 1; 
}

/**
 * init_idt:
 *  DESCRIPTION : fill the entries of IDT with handler address.
 *  INPUTS : none
 *  OUTPUTS : none 
 *  RETURN VALUE : none
 *  SIDE EFFECTS : The entries of IDT has been filled.
 */
void
init_idt(void){
    unsigned long i;
    uint32_t dpl;
    for(i = 0; i < NUM_VEC; i++){
        // reference: page 148 Exception handling in textbook "Understanding Linux Kernel" and souce code https://elixir.bootlin.com/linux/v2.6.26/source/include/asm-x86/desc.h#L329
        /* trap_gate */
        if ((i >= 0 && i <= 1 ) || (i >= 6 && i <= 7) || (i >= 9 && i <= 13) || (i >= 16 && i <= 19)){
            dpl = DPL_KERNEL;
            write_gate_entry(i, trap_gate, dpl);
        }
        /* intr_gate */
        if (i == 2 || i == 14 || i == KB_VEC || i == RTC_VEC || i == PIT_VEC){
            dpl = DPL_KERNEL;
            write_gate_entry(i, intr_gate, dpl);
        }
        /* system_intr_gate */
        if (i == 3){
            dpl = DPL_USER;
            write_gate_entry(i, intr_gate, dpl);
        }
        /* system_gate */
        if (i == 4 || i == 5 || i == SYS_VEC){
            dpl = DPL_USER;
            write_gate_entry(i, trap_gate, dpl);
        }
        /* task_gate */
        if (i == 8){
            dpl = DPL_KERNEL;
            write_gate_entry(i, task_gate, dpl);
        }
    }

    /* exception */
    SET_IDT_ENTRY(idt[0x00], Divide_Error);
    SET_IDT_ENTRY(idt[0x01], Debug);
    SET_IDT_ENTRY(idt[0x02], NMI);
    SET_IDT_ENTRY(idt[0x03], Breakpoint);
    SET_IDT_ENTRY(idt[0x04], Overflow);
    SET_IDT_ENTRY(idt[0x05], BOUND_Range_Exceeded);
    SET_IDT_ENTRY(idt[0x06], Invalid_Opcode);
    SET_IDT_ENTRY(idt[0x07], Device_Not_Available);
    SET_IDT_ENTRY(idt[0x08], Double_Fault);
    SET_IDT_ENTRY(idt[0x09], Coprocessor_Segment_Overrun);
    SET_IDT_ENTRY(idt[0x0A], Invalid_TSS);
    SET_IDT_ENTRY(idt[0x0B], Segment_Not_Present);
    SET_IDT_ENTRY(idt[0x0C], Stack_Fault);
    SET_IDT_ENTRY(idt[0x0D], General_Protection);
    SET_IDT_ENTRY(idt[0x0E], Page_Fault);
    SET_IDT_ENTRY(idt[0x0F], Reserved);
    SET_IDT_ENTRY(idt[0x10], Floating_Point_Error);
    SET_IDT_ENTRY(idt[0x11], Alignment_Check);
    SET_IDT_ENTRY(idt[0x12], Machine_Check);
    SET_IDT_ENTRY(idt[0x13], SIMD_Floating_Point);

    /* interrupt */
    SET_IDT_ENTRY(idt[PIT_VEC], PIT_HANDLER_link);
    SET_IDT_ENTRY(idt[KB_VEC], KEYBOARD_HANDLER_link);
    SET_IDT_ENTRY(idt[RTC_VEC], RTC_HANDLER_link);

    /* system call */
    SET_IDT_ENTRY(idt[SYS_VEC], SYS_CALL_link);

    /* load idt */
    lidt(idt_desc_ptr);
}

/**
 * exception_handler
 *  DESCRIPTION : Printing a prompt as to which Exception was raised, 
 *                and then squash any user-level program that produces an exception, returning control to the shell.
 *                Here, we use a while loop to freeze the program. 
 *  INPUTS : vec -- the index of exception.
 *           regs -- all the status of registers.
 *           flags -- flag register.
 *           error -- error code.
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : print the error message on the screen.
 */
void
exception_handler(reg_t regs, uint32_t ds, uint32_t es, uint32_t fs, uint32_t excep_num, uint32_t error){
    clear();
    printf("EXCEPTION(%d): %s\n", excep_num, EXCEPTION_NAME[excep_num]);
    exception_flag = 1;
    if(excep_num == DIV_ZERO) send_signal(DIV_ZERO);
    else send_signal(SEGFAULT);
}
