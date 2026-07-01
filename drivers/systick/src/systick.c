#include "systick.h"

/* SysTick CTRL bits (Cortex-M4 Generic User Guide §4.4.1) */
#define CLOCKEN    (1U << 0)    /* ENABLE: start the countdown */
#define CLOCKSRC   (1U << 2)    /* CLKSOURCE: 1 = processor clock */
#define COUNTFLAG  (1U << 16)   /* Set to 1 when counter wraps to 0 */

/* 16 MHz processor clock / 16 000 ticks = 1 ms period */
#define LOAD_VALUE  16000U

void delay(uint32_t ms)
{
    SysTick->VAL  = 0;                  /* clear current value before starting */
    SysTick->LOAD = LOAD_VALUE - 1;    /* reload value for 1 ms period */
    SysTick->CTRL = CLOCKSRC;          /* select processor clock, no interrupt */
    SysTick->CTRL |= CLOCKEN;          /* start counter */

    for (uint32_t i = 0; i < ms; i++)
    {
        while ((SysTick->CTRL & COUNTFLAG) == 0){}   /* wait for each 1 ms tick */
    }

    SysTick->CTRL = 0;   /* stop counter */
}
