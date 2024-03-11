/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */


/**
 * i8259_init
 *  DESCRIPTION : Initialize the 8259 PIC
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : mask all the interrupts and send ICW1-4 for initialization. enable the cascade interrupt.
 */
void i8259_init(void) {

    master_mask = 0xff;                                 // IRQs 0-7 are masked 
    slave_mask = 0xff;                                  // IRQs 8-15 are masked
    outb(master_mask, MASTER_8259_PORT_1);              // masking interrupts on master PIC 
    outb(slave_mask, SLAVE_8259_PORT_1);                // masking interrupts on slave PIC

    outb(ICW1, MASTER_8259_PORT_0);                     // ICW1 for master
    outb(ICW2_MASTER, MASTER_8259_PORT_1);              // ICW2 for master
    outb(ICW3_MASTER, MASTER_8259_PORT_1);              // ICW3 for master
    outb(ICW4, MASTER_8259_PORT_1);                     // ICW4 for master

    outb(ICW1, SLAVE_8259_PORT_0);                      // ICW1 for slave
    outb(ICW2_SLAVE, SLAVE_8259_PORT_1);                // ICW2 for slave
    outb(ICW3_SLAVE, SLAVE_8259_PORT_1);                // ICW3 for slave
    outb(ICW4, SLAVE_8259_PORT_1);                      // ICW4 for slave

    enable_irq(CASCADE_IR);
}

/**
 * enable_irq
 *  DESCRIPTION : Enable (unmask) the specified IRQ
 *  INPUTS : irq_num, which irq to be enabled.
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : enable the specific and valid IRQ.
 */
void enable_irq(uint32_t irq_num) {

    uint8_t mask;
    /* sanity check*/
    if (irq_num > MAX_IRQ_VEC) return;                           // invalid irq_num

    /* if irq_num is within 0-7*/
    if (irq_num < PRIMARY_MAX){                                  
        mask = (1 << irq_num)^ 0xffffffff;              // XOR 0xffffffff can make each bit to change from 0 to 1 and from 1 to 0
        master_mask &= mask;                            // mask the irq corresponding to irq_num
        outb(master_mask, MASTER_8259_PORT_1);          // output it to PIC
    } 
    /* if irq_num is within 8-15*/
    if (irq_num >= PRIMARY_MAX){
        mask = (1 << (irq_num-8))^ 0xffffffff;          //  minus 8 to match slave_mask and XOR to reverse 0 and 1
        slave_mask &= mask;                             // mask the irq corresponding to irq_num
        outb(slave_mask, SLAVE_8259_PORT_1);            // output it to PIC
    }

}

/**
 * disable_irq
 *  DESCRIPTION : Disable (mask) the specified IRQ
 *  INPUTS : irq_num, which irq to be disabled.
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : disable the specific and valid IRQ.
 */
void disable_irq(uint32_t irq_num) {

    /* sanity check*/
    if (irq_num > MAX_IRQ_VEC) return;                           // invalid irq_num

    /* if irq_num is within 0-7*/
    if (irq_num < PRIMARY_MAX){                                   
        master_mask |= (1 << irq_num);                  // mask the irq corresponding to irq_num
        outb(master_mask, MASTER_8259_PORT_1);          // output it to PIC
    } 
    /* if irq_num is within 8-15*/
    if (irq_num >= PRIMARY_MAX){
        slave_mask |= (1 << (irq_num - 8));             // minus 8 to match slave_mask
        outb(slave_mask, SLAVE_8259_PORT_1);            // output it to PIC
    }
}

/**
 * send_eoi
 *  DESCRIPTION : Send end-of-interrupt signal for the specified IRQ
 *  INPUTS : irq_num, which irq will be sent eoi.
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : send the eoi to specific and valid irq port.
 */
void send_eoi(uint32_t irq_num) {

     /* sanity check*/
    if (irq_num > MAX_IRQ_VEC) return;                           // invalid irq_num

    /* if the irq_num is within 0-7 */
    if (irq_num < PRIMARY_MAX){
        /* specific EOI to master */
        outb(EOI | irq_num, MASTER_8259_PORT_0);        //gets OR'd with the interrupt number
    }

    /* if the irq_num is within 8-15 */
    if (irq_num >= PRIMARY_MAX) {
        /*specific EOI to slave */
        outb(EOI | (irq_num - PRIMARY_MAX), SLAVE_8259_PORT_0);    // irq_num-8: 8-15 --> 0-7 irq number
        /*specific EOI to master */
        outb(EOI | CASCADE_IR, MASTER_8259_PORT_0);
    }
}
