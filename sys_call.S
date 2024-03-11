#define ASM     1

.align 4
sys_call_table:
    .long 0x0               # no system_call 0
    .long halt
    .long execute
    .long read
    .long write
    .long open
    .long close
    .long getargs
    .long vidmap
    .long set_handler
    .long sigreturn
    .long color
    .long ps
    .long cp
    .long rm

.globl SYS_CALL_link

.align 4
SYS_CALL_link:
    addl    $-8, %esp
    pushl   %fs                      
    pushl   %es                      
    pushl   %ds                      
    # save registers
    pushl   %eax
    pushl   %ebp
    pushl   %edi
    pushl   %esi
    pushl   %edx
    pushl   %ecx
    pushl   %ebx

    # check validity of call number
    cmpl    $0, %eax
    jle     invalid_syscall
    cmpl    $14,%eax
    jg      invalid_syscall
    cmpl    $10, %eax
    je      sig_syscall

    # set args and call func
    call    *sys_call_table(, %eax, 4)
    jmp     sys_call_return

invalid_syscall:
    movl    $-1, %eax

sys_call_return:
    popl    %ebx
    popl    %ecx
    popl    %edx
    popl    %esi
    popl    %edi
    popl    %ebp
    addl    $4, %esp
    popl    %ds                      
    popl    %es                      
    popl    %fs                      
    addl    $8, %esp                 
    iret

sig_syscall:
    call    *sys_call_table(, %eax, 4)
    popl    %ebx
    popl    %ecx
    popl    %edx
    popl    %esi
    popl    %edi
    popl    %ebp
    popl    %eax
    popl    %ds                      
    popl    %es                      
    popl    %fs                      
    addl    $8, %esp                 
    iret
