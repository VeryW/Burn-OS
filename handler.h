#ifndef HANDLER_H
#define HANDLER_H

#include "lib.h"
#include "keyboard.h"
#include "system_call.h"
#include "rtc.h"
#include "scheduler.h"

/* All the handlers below are implentmented in the "handler.S".*/

/* Handler for interrupts */
extern void PIT_HANDLER_link(void);
extern void KEYBOARD_HANDLER_link(void); 
extern void RTC_HANDLER_link(void);

/* Handler for exception */
extern void Divide_Error(void);
extern void Debug(void);
extern void NMI(void);
extern void Breakpoint(void);
extern void Overflow(void);
extern void BOUND_Range_Exceeded(void);
extern void Invalid_Opcode(void);
extern void Device_Not_Available(void);
extern void Double_Fault(void);
extern void Coprocessor_Segment_Overrun(void);
extern void Invalid_TSS(void);
extern void Segment_Not_Present(void);
extern void Stack_Fault(void);
extern void General_Protection(void);
extern void Page_Fault(void);
extern void Reserved(void);
extern void Floating_Point_Error(void);
extern void Alignment_Check(void);
extern void Machine_Check(void);
extern void SIMD_Floating_Point(void);

#endif
