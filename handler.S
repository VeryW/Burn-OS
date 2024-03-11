#define ASM     1
#define Dummy   255
#define PIT_IRQ         0x20
#define KB_IRQ          0x21
#define RTC_IRQ         0x28

# --- interrupt handler linkage --- #
#define INTERRUPT(name, IRQ, handler) \
.globl name                          ;\
.align  4                            ;\
name:                                ;\
    pushl   $Dummy                   ;\
    pushl   $IRQ                     ;\
    pushl   %fs                      ;\
    pushl   %es                      ;\
    pushl   %ds                      ;\
    pushl   %eax                     ;\
    pushl   %ebp                     ;\
    pushl   %edi                     ;\
    pushl   %esi                     ;\
    pushl   %edx                     ;\
    pushl   %ecx                     ;\
    pushl   %ebx                     ;\
    call    handler                  ;\
    call    do_signal                ;\
    popl    %ebx                     ;\
    popl    %ecx                     ;\
    popl    %edx                     ;\
    popl    %esi                     ;\
    popl    %edi                     ;\
    popl    %ebp                     ;\
    popl    %eax                     ;\
    popl    %ds                      ;\
    popl    %es                      ;\
    popl    %fs                      ;\
    addl    $8, %esp                 ;\
    iret

INTERRUPT(KEYBOARD_HANDLER_link, KB_IRQ, keyboard_handler);
INTERRUPT(RTC_HANDLER_link, RTC_IRQ, rtc_handler);
INTERRUPT(PIT_HANDLER_link, PIT_IRQ, pit_handler);

# --- exception handler linkage --- #
#define EXCEPTION(name, excep_num)    \
.globl name                          ;\
.align  4                            ;\
name:                                ;\
    pushl   $Dummy                   ;\
    pushl   $excep_num               ;\
    pushl   %fs                      ;\
    pushl   %es                      ;\
    pushl   %ds                      ;\
    pushl   %eax                     ;\
    pushl   %ebp                     ;\
    pushl   %edi                     ;\
    pushl   %esi                     ;\
    pushl   %edx                     ;\
    pushl   %ecx                     ;\
    pushl   %ebx                     ;\
    call    exception_handler        ;\
    call    do_signal                ;\
    popl    %ebx                     ;\
    popl    %ecx                     ;\
    popl    %edx                     ;\
    popl    %esi                     ;\
    popl    %edi                     ;\
    popl    %ebp                     ;\
    popl    %eax                     ;\
    popl    %ds                      ;\
    popl    %es                      ;\
    popl    %fs                      ;\
    addl    $8, %esp                 ;\
    iret                

#define EXCEPTION_ERROR_CODE(name, excep_num)  \
.globl name                          ;\
.align  4                            ;\
name:                                ;\
    pushl   $excep_num               ;\
    pushl   %fs                      ;\
    pushl   %es                      ;\
    pushl   %ds                      ;\
    pushl   %eax                     ;\
    pushl   %ebp                     ;\
    pushl   %edi                     ;\
    pushl   %esi                     ;\
    pushl   %edx                     ;\
    pushl   %ecx                     ;\
    pushl   %ebx                     ;\
    call    exception_handler        ;\
    call    do_signal                ;\
    popl    %ebx                     ;\
    popl    %ecx                     ;\
    popl    %edx                     ;\
    popl    %esi                     ;\
    popl    %edi                     ;\
    popl    %ebp                     ;\
    popl    %eax                     ;\
    popl    %ds                      ;\
    popl    %es                      ;\
    popl    %fs                      ;\
    addl    $8, %esp                 ;\
    iret

EXCEPTION(Divide_Error, 0);
EXCEPTION(Debug, 1);
EXCEPTION(NMI, 2);
EXCEPTION(Breakpoint, 3);
EXCEPTION(Overflow, 4);
EXCEPTION(BOUND_Range_Exceeded, 5);
EXCEPTION(Invalid_Opcode, 6);
EXCEPTION(Device_Not_Available, 7);
EXCEPTION_ERROR_CODE(Double_Fault, 8);
EXCEPTION(Coprocessor_Segment_Overrun, 9);
EXCEPTION_ERROR_CODE(Invalid_TSS, 10);
EXCEPTION_ERROR_CODE(Segment_Not_Present, 11);
EXCEPTION_ERROR_CODE(Stack_Fault, 12);
EXCEPTION_ERROR_CODE(General_Protection, 13);
EXCEPTION_ERROR_CODE(Page_Fault, 14);
EXCEPTION(Reserved, 15);
EXCEPTION(Floating_Point_Error, 16);
EXCEPTION_ERROR_CODE(Alignment_Check, 17);
EXCEPTION(Machine_Check, 18);
EXCEPTION(SIMD_Floating_Point, 19);

.globl EXEC_sigreturn
.globl END_of_exec_sigreturn
.align  4
EXEC_sigreturn:
    movl $10, %eax
    INT  $0x80
END_of_exec_sigreturn:
