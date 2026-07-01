#include "pwm.h"

/* Drives the Nucleo-64 on-board user LED (LD2, PA5) via TIM2_CH1 PWM.
 * Not a general-purpose PWM driver — hardcoded to this pin/timer/channel. */

/* PA5 → TIM2_CH1 via AF1 (RM0383 Table 9 — Alternate function mapping) */

void pwm_init(uint16_t prescaler)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* PA5 → alternate function, AF1 (TIM2_CH1) */
    GPIOA->MODER |= GPIO_MODER_MODE5_1;
    GPIOA->AFR[0] |= GPIO_AFRL_AFSEL5_0;

    /* CH1 → output compare (not input capture) */
    TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM2->CCER &= ~TIM_CCER_CC1P;

    /* CH1 → PWM mode 1 (output high while CNT < CCR1) */
    TIM2->CCMR1 |= TIM_CCMR1_OC1M_2;
    TIM2->CCMR1 |= TIM_CCMR1_OC1M_1;
    TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_0;

    TIM2->PSC = prescaler;

    TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->CR1 &= ~TIM_CR1_CMS;
}

void pwm_start(void)
{
    TIM2->CCER |= TIM_CCER_CC1E;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void set_duty_cycle(uint16_t duty, uint16_t period)
{
    TIM2->ARR = period;

    /* multiply before dividing — duty/period truncates to 0 for duty < 100 */
    uint32_t duty_cycle = ((uint32_t)duty * period) / 100;
    TIM2->CCR1 = duty_cycle;
}
