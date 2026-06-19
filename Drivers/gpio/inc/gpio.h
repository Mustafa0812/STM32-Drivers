#ifndef GPIO_H_
#define GPIO_H_

#include "stm32f4xx.h"
#include <stdbool.h>

/* --- GPIO polling --- */
void led_init(void);
void led_on(void);
void led_off(void);
void btn_init(void);
bool btn_state(void);

/* --- EXTI interrupt --- */
typedef void (*exti_cb_t)(void);

void exti_register_callback(exti_cb_t cb);
void exti_init(void);

#endif
