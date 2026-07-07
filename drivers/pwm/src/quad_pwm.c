#include "pwm.h"

/* General-purpose 4-channel PWM for motor/ESC control via TIM4 CH1-4.
 * Unlike pwm_led.c, this isn't tied to one specific device — caller picks
 * which of the 4 channels (1-4) to drive via quad_pwm_set_duty(). All four
 * channels share one counter/prescaler/period; only duty is per-channel. */

/* PB6/PB7/PB8/PB9 → TIM4_CH1-4 via AF2 (RM0383 Table 9 — Alternate function mapping) */

void quad_pwm_init(uint16_t prescaler)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    /* PB6/PB7/PB8/PB9 → alternate function, AF2 (TIM4_CH1-4) */
    GPIOB->MODER |= GPIO_MODER_MODE6_1;
    GPIOB->AFR[0] |= GPIO_AFRL_AFSEL6_1;

    GPIOB->MODER |= GPIO_MODER_MODE7_1;
    GPIOB->AFR[0] |= GPIO_AFRL_AFSEL7_1;

    GPIOB->MODER |= GPIO_MODER_MODE8_1;
    GPIOB->AFR[1] |= GPIO_AFRH_AFSEL8_1;

    GPIOB->MODER |= GPIO_MODER_MODE9_1;
    GPIOB->AFR[1] |= GPIO_AFRH_AFSEL9_1;

    /* CH1-4 → output compare (not input capture), active-high polarity */
    TIM4->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM4->CCER &= ~TIM_CCER_CC1P;

    TIM4->CCMR1 &= ~TIM_CCMR1_CC2S;
    TIM4->CCER &= ~TIM_CCER_CC2P;

    TIM4->CCMR2 &= ~TIM_CCMR2_CC3S;
    TIM4->CCER &= ~TIM_CCER_CC3P;

    TIM4->CCMR2 &= ~TIM_CCMR2_CC4S;
    TIM4->CCER &= ~TIM_CCER_CC4P;

    /* CH1-4 → PWM mode 1 (output high while CNT < CCRx) */
    TIM4->CCMR1 |= TIM_CCMR1_OC1M_2;
    TIM4->CCMR1 |= TIM_CCMR1_OC1M_1;
    TIM4->CCMR1 &= ~TIM_CCMR1_OC1M_0;

    TIM4->CCMR1 |= TIM_CCMR1_OC2M_2;
    TIM4->CCMR1 |= TIM_CCMR1_OC2M_1;
    TIM4->CCMR1 &= ~TIM_CCMR1_OC2M_0;

    TIM4->CCMR2 |= TIM_CCMR2_OC3M_2;
    TIM4->CCMR2 |= TIM_CCMR2_OC3M_1;
    TIM4->CCMR2 &= ~TIM_CCMR2_OC3M_0;

    TIM4->CCMR2 |= TIM_CCMR2_OC4M_2;
    TIM4->CCMR2 |= TIM_CCMR2_OC4M_1;
    TIM4->CCMR2 &= ~TIM_CCMR2_OC4M_0;

    TIM4->PSC = prescaler;

    TIM4->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM4->CCMR1 |= TIM_CCMR1_OC2PE;
    TIM4->CCMR2 |= TIM_CCMR2_OC3PE;
    TIM4->CCMR2 |= TIM_CCMR2_OC4PE;

    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->CR1 &= ~TIM_CR1_CMS;
}

/* Enables all 4 channel outputs and starts the shared counter. Channels are
 * always started together — individual channel enable/disable isn't needed
 * since an unused channel just gets ignored downstream. */
void quad_pwm_start(void)
{
    TIM4->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E);

    TIM4->CR1 |= TIM_CR1_CEN;
}

/* Sets the shared period (ARR) and one channel's duty cycle (CCRx).
 * channel must be 1-4; any other value is a silent no-op. */
void quad_pwm_set_duty(uint16_t duty, uint16_t period, uint8_t channel)
{
    TIM4->ARR = period;

    /* multiply before dividing — duty/period truncates to 0 for duty < 100 */
    uint32_t duty_cycle = ((uint32_t)duty * period) / 100;

    switch (channel)
    {
        case 1:
            TIM4->CCR1 = duty_cycle;
            break;
        case 2:
            TIM4->CCR2 = duty_cycle;
            break;
        case 3:
            TIM4->CCR3 = duty_cycle;
            break;
        case 4:
            TIM4->CCR4 = duty_cycle;
            break;
    }
}
