#ifndef PIT_H
#define PIT_H

#define PIT_IRQ 0
#define PIT_DATA_PORT   0x40
#define PIT_MODE_PORT   0x43
#define PIT_COUNT       11932           // 100Hz or 10ms
#define PIT_MODE        0x34

extern void pit_init(void);

#endif
