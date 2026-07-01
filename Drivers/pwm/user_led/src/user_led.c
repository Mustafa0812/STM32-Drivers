#include "stm32f4xx.h"


#define PRESCALER 15999
#define PERIOD 10000
#define DUTY_CYCLE  250



int main(void){

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    GPIOA -> MODER |= (1U << 10);
    GPIOA -> ODR &= ~ (1U << 5);
    //GPIOA->MODER |= GPIO_MODER_MODE5_1;
    //GPIOA->AFR[0] |= GPIO_AFRL_AFSEL5_0;

    TIM2->CCMR1 &= ~TIM_CCMR1_CC2S;
    TIM2->CCER &= ~TIM_CCER_CC1P;

    TIM2->CCMR1 |= TIM_CCMR1_OC2M_2; 
    TIM2->CCMR1 |= TIM_CCMR1_OC2M_1;
    TIM2->CCMR1 &= ~TIM_CCMR1_OC2M_0;

    TIM2->ARR = PERIOD;
    TIM2->PSC = PRESCALER;
    TIM2->CCR1 = DUTY_CYCLE;

    TIM2->CCMR1 |= TIM_CCMR1_OC2PE;

    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->CR1 &= ~TIM_CR1_CMS;

    TIM2->CCER |= TIM_CCER_CC2E;
    TIM2->CR1 |= TIM_CR1_CEN;

    

    while (1)
    {
        /* code */
    }
    



}