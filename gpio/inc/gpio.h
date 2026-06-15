#ifndef GPIO_H_
#define GPIO_H_

#include "stm32f4xx.h"
#include <stdbool.h>

void led_init (void);
void led_on (void);
void led_off (void);
void btn_init (void);
bool btn_state (void);

#endif
