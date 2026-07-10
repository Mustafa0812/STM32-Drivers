#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>
#include "stm32f4xx.h"

void delay (uint32_t DELAY);
uint32_t get_tick(void);
void start_timer(void);

#endif
