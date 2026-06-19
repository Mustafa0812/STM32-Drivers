#include "gpio.h"
#include "systick.h"
#include <stdint.h>

#define USE_INTERRUPT    /* comment out to use polling mode */

#ifdef USE_INTERRUPT
static volatile uint8_t flag = 0;

void on_press(void){
    flag = 1;
}
#endif

int main(void){
    led_init();
    btn_init();

#ifdef USE_INTERRUPT
    exti_register_callback(on_press);
    exti_init();
#endif

    while (1){

#ifdef USE_INTERRUPT
        if (flag){
            flag = 0;
            led_on();
            delay(1000);
            led_off();
        }
#else
        if (btn_state()){
            led_on();
            delay(1000);
            led_off();
        }
#endif

    }
}
