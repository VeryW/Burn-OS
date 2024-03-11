#include "signal.h"
#include "system_call.h"
#include "scheduler.h"
#include "lib.h"
#include "idt.h"

uint8_t SIGINT_flag = 0;

void kill_the_task(void){
    if(SIGINT_flag){
        SIGINT_flag = 0;
        printf("Program received signal SIGINT, Interrupt.\n");
    }
    halt(0);
    return;
}

void ignore(void){
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));
    cur_pcb->sig_mask = 0;
    return;
}

void send_signal(int8_t sig_num){
    if(sig_num < 0 || sig_num >= NUM_SIGNAL) return;
    pcb_t* cur_pcb;
    if(sig_num == INTERRUPT){
        cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (active_array[cur_terminal] + 1));
    }else{
        cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));
    }
    cur_pcb->sig_pending[(uint8_t)sig_num] = 1;
    return;
}

void* dft_sig_handler[NUM_SIGNAL] = {&kill_the_task, &kill_the_task, &kill_the_task, &ignore, &ignore};

void do_signal(context_t context){
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));
    uint32_t sig_num;
    for(sig_num = 0; sig_num < NUM_SIGNAL; sig_num++){
        if(cur_pcb->sig_pending[sig_num]){
            cur_pcb->sig_pending[sig_num] = 0;
            cur_pcb->sig_mask = 1;                                          // mask all other signals
            if(cur_pcb->sig_handler[sig_num] == dft_sig_handler[sig_num]){  // directly execute handler in kernel if it is default
                void (*handler)(void) = cur_pcb->sig_handler[sig_num];
                handler();
                return;
            }
            break;
        }
    }
    if(sig_num == NUM_SIGNAL) return;                                       // no pending signal
    if(sig_num == ALARM){
        void (*handler)(int signum) = cur_pcb->sig_handler[sig_num];
        handler(ALARM);
        return;
    }

    /* Set up the signal handler’s stack frame */
    uint32_t user_esp = context.iret.ESP;
    uint32_t size_exec_sigreturn = END_of_exec_sigreturn - EXEC_sigreturn;
    memcpy((void*)(user_esp - size_exec_sigreturn), (void*)EXEC_sigreturn, size_exec_sigreturn);
    user_esp -= size_exec_sigreturn;
    uint32_t exec_sigreturn = user_esp;
    memcpy((void*)(user_esp - sizeof(context_t)), (void*)(&context), sizeof(context_t));
    user_esp -= sizeof(context_t);
    memcpy((void*)user_esp - sizeof(uint32_t), (void*)(&sig_num), sizeof(uint32_t));
    user_esp -= sizeof(uint32_t);
    memcpy((void*)(user_esp - sizeof(uint32_t)), (void*)(&exec_sigreturn), sizeof(uint32_t));
    user_esp -= sizeof(uint32_t);

    asm volatile(
        "pushl  %0\n"
        "pushl  %1\n"
        "pushl  %2\n"
        "pushl  %3\n"
        "pushl  %4\n"
        "IRET\n"
        :
        : "r"(context.iret.SS), "r"(user_esp), "r"(context.iret.EFLAGS), "r"(context.iret.CS), "r"((uint32_t)(cur_pcb->sig_handler[sig_num]))
    );
}
