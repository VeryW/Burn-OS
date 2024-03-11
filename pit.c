#include "pit.h"
#include "lib.h"
#include "i8259.h"
#include "scheduler.h"

/*
 * pit_init
 *  DESCRIPTION : enable the interrupt request of Periodic interrupt timer
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : enable the pin#0 interrupt request of master_PIC.
 */
void pit_init(void)
{
    outb(PIT_MODE, PIT_MODE_PORT);
	outb(PIT_COUNT&0xFF, PIT_DATA_PORT);		    // Low byte
	outb((PIT_COUNT&0xFF00)>>8, PIT_DATA_PORT);	    // High byte
    enable_irq(PIT_IRQ);
}

/*
 * pit_handler
 *  DESCRIPTION : When an interrupt of pit occurs, handle it by calling scheduler
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : calling scheduler
 */
void pit_handler(void)
{
    send_eoi(PIT_IRQ);
    scheduler();
}
