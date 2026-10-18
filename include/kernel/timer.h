#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "kernel/isr.h"

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND_PORT  0x43
#define PIT_BASE_FREQUENCY 1193182

void timer_init(uint32_t frequency);
uint64_t timer_get_ticks(void);
void timer_sleep_ticks(uint64_t ticks);

#endif /* TIMER_H */
