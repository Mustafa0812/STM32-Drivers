#include "gpio.h"
#include <stddef.h>

static volatile exti_cb_t exti_cb = NULL;

void exti_register_callback(exti_cb_t cb){
    exti_cb = cb;
}

void exti_init(void){

    __disable_irq();

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    SYSCFG->EXTICR[3] &= ~(0xF << 4);   /* RM0383 §7.2.3 — clear EXTI13 field */
    SYSCFG->EXTICR[3] |=  (0x2 << 4);   /* select Port C */

    EXTI->IMR  |=  (1U << 13);           /* RM0383 §12.3.1 — unmask line 13 */

    EXTI->RTSR &= ~(1U << 13);           /* RM0383 §12.3.3 — ensure rising edge off */
    EXTI->FTSR |=  (1U << 13);           /* RM0383 §12.3.4 — falling edge trigger */

    NVIC_SetPriority(EXTI15_10_IRQn, 5);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    __enable_irq();
}

void EXTI15_10_IRQHandler(void){

    if (EXTI->PR & (1U << 13)){

        if (exti_cb != NULL){
            exti_cb();
        }
        EXTI->PR |= (1U << 13);          /* RM0383 §12.3.6 — write-1-to-clear */
    }
}
