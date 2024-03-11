#include "rtc.h"
#include "x86_desc.h"
#include "i8259.h"
#include "terminal.h"
#include "scheduler.h"
#include "system_call.h"
#include "signal.h"
#include "lib.h"

#define RTC_IRQ_NUM 0x8
#define RTC_PORT_0  0x70                // specify an index or "register number", and to disable NMI.
#define RTC_PORT_1  0x71                // read or write from/to that byte of CMOS configuration space
#define RTC_REG_A   0x8A
#define RTC_REG_B   0x8B
#define RTC_REG_C   0x8C

rtc_t rtc[NUM_TERMINAL];
volatile uint32_t ALARM_counter = 0;

/*
 * rtc_init
 *  DESCRIPTION : enable the interrupt request of the real-time-clock
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : enable the pin#8 interrupt request of master_PIC.
 */
void rtc_init(void){
    outb(RTC_REG_B, RTC_PORT_0);	    // select register B, and disable NMI
    uint8_t prev = inb(RTC_PORT_1);        // read the current value of register B
    outb(RTC_REG_B, RTC_PORT_0);	    // set the index again (a read will reset the index to register D)
    outb(prev | 0x40, RTC_PORT_1);      // write the previous value ORed with 0x40. This turns on bit 6 of register B

    outb(RTC_REG_A, RTC_PORT_0);		// set index to register A, disable NMI
    prev = inb(RTC_PORT_1);	            // get initial value of register A
    outb(RTC_REG_A, RTC_PORT_0);		// reset index to A
    /* set rtc to max freq 1024 Hz */
    outb((prev & 0xF0) | RATE_MAX, RTC_PORT_1); // write only our rate to A. Note, rate is the bottom 4 bits.

    uint8_t i;
    for(i = 0; i < NUM_TERMINAL; i++){
        rtc[i].counter = 0;
        rtc[i].tick = 0;
        rtc[i].required_count = FREQ_MAX / FREQ_DFT;
    }

    enable_irq(RTC_IRQ_NUM);		    // (perform an STI) and reenable NMI
}

/*
 * rtc_open
 *  DESCRIPTION : initialize rtc frequency to 2Hz
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : set default rtc settings
 */
int32_t rtc_open(const uint8_t* filename){
    uint8_t term_id = sche_term;
    rtc[term_id].tick = 0;
    rtc[term_id].counter = 0;
    rtc[term_id].required_count = FREQ_MAX / FREQ_DFT;
    return 0;
}


/*
 * rtc_close
 *  DESCRIPTION : close rtc device
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : none
 */
int32_t rtc_close(int32_t fd){
    return 0;
}

/*
 * rtc_read
 *  DESCRIPTION : read next rtc interrupt
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : block until next virtual interrupt
 */ 
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    uint8_t term_id = sche_term;
    while(!(rtc[term_id].tick));                          // wait until a virtual interrupt happens
    rtc[term_id].tick = 0;                                // reset virtual interrupt flag
    return 0;
}

/*
 * rtc_write
 *  DESCRIPTION : write new rtc frequency
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : change virtual interrupt frequency
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    /* check valid buffer */
    if (buf == NULL)  return -1;
    uint32_t new_freq = *((uint32_t*)buf);
    /* frequency should be in range and be power of 2 */
    if(new_freq > FREQ_MAX || ((new_freq & (new_freq - 1)) != 0)) return -1;
    rtc[(uint8_t)sche_term].required_count = new_freq / FREQ_DFT;
    return 0;
}

/*
 * rtc_handler
 *  DESCRIPTION : When an interrupt of rtc occurs, handle it by incrementing the counter.
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : none
 */
void rtc_handler(void){
    /* no need the critical section. */
    uint8_t term_id;
    for(term_id = 0; term_id < NUM_TERMINAL; term_id++){
        rtc[term_id].counter++;
        if(rtc[term_id].counter == rtc[term_id].required_count){   // set virtual interrupt flag if reach virtual freq
            rtc[term_id].tick = 1;
            rtc[term_id].counter = 0;
            if(rainbow_flag[term_id]){
                rainbow_ptr[term_id] = (rainbow_ptr[term_id] + 1) % (NUM_COLORS - 1);
                ATTRIB[term_id] = colors[rainbow_ptr[term_id]];
            }
        }
    }

    ALARM_counter++;
    if(ALARM_counter == ALARM_PERIOD){
        ALARM_counter = 0;
        uint8_t saved_process = cur_process;
        uint8_t i;
        for(i = 0; i < NUM_TERMINAL; i++){
            cur_process = active_array[i];
            send_signal(ALARM);
        }
        cur_process = saved_process;
    }

    /* to be sure get another interrupt*/
    outb(RTC_REG_C & 0x0F, RTC_PORT_0);	    // select register C
    uint8_t temp = inb(RTC_PORT_1);		    // just throw away contents
    (void) temp;

    send_eoi(RTC_IRQ_NUM);
}
