#include "systick.h"

/* SysTick CTRL bits (Cortex-M4 Generic User Guide §4.4.1) */
#define CLOCKEN    (1U << 0)    /* ENABLE: start the countdown */
#define CLOCKSRC   (1U << 2)    /* CLKSOURCE: 1 = processor clock */
#define COUNTFLAG  (1U << 16)   /* Set to 1 when counter wraps to 0 */
#define TICKINT    (1U << 1)

static volatile uint32_t tick_count = 0;

/* Real definition backing FreeRTOSConfig.h's `extern uint32_t SystemCoreClock`
 * (configCPU_CLOCK_HZ). Update this if PLL configuration is ever added. */
uint32_t SystemCoreClock = 16000000;

/* 16 MHz processor clock / 16 000 ticks = 1 ms period */
#define LOAD_VALUE  16000U


void start_timer(void){
    SysTick->VAL  = 0;                  /* clear current value before starting */
    SysTick->LOAD = LOAD_VALUE - 1;    /* reload value for 1 ms period */
    SysTick->CTRL = CLOCKSRC | TICKINT; /* select processor clock */
    SysTick->CTRL |= CLOCKEN;          /* start counter */

}

void delay(uint32_t ms)
{
    uint32_t t0 = get_tick ();

    uint32_t elapsed_time = 0;
    while(elapsed_time != ms){
        elapsed_time = get_tick() - t0;
    }

}


uint32_t get_tick (void){

    return tick_count;
}


void SysTick_Handler (void){

    tick_count++ ;

}