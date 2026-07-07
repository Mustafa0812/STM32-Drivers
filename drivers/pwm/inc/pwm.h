#ifndef PWM_H_
#define PWM_H_

#include "stm32f4xx.h"

/* --- Nucleo user LED (LD2, PA5) PWM brightness control via TIM2_CH1 --- */
void pwm_init(uint16_t prescaler);
void pwm_start(void);
void set_duty_cycle(uint16_t duty, uint16_t period);

/* --- General-purpose 4-channel motor/ESC PWM via TIM4 CH1-4 (PB6/7/8/9) --- */
void quad_pwm_init(uint16_t prescaler);
void quad_pwm_start(void);
void quad_pwm_set_duty(uint16_t duty, uint16_t period, uint8_t channel);

#endif
