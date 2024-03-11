#ifndef _RTC_H
#define _RTC_H

#include "lib.h"

#define FREQ_MAX    1024
#define RATE_MAX    6
#define FREQ_DFT    2

#define ALARM_PERIOD (FREQ_MAX * 10)

typedef struct rtc_t
{
    uint32_t required_count;
    volatile uint32_t counter;              // counter of rtc interrupts at actual frequency 
    volatile uint32_t tick;                 // virtual interrupt flag
}rtc_t;

/* initialize rtc */
extern void rtc_init(void);

/* handle the rtc interrupt */
extern void rtc_handler(void); 

/* initialize rtc frequency */
extern int32_t rtc_open(const uint8_t* filename);

/* close rtc */
extern int32_t rtc_close(int32_t fd);

/* read next rtc interrupt */
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* write new rtc frequency */
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

#endif /* _RTC_H */
