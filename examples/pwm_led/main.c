#include "pwm.h"

#define PRESCALER 15
#define PERIOD    10000
#define DUTY      25    /* percent */

int main(void)
{
    pwm_init(PRESCALER);
    set_duty_cycle(DUTY, PERIOD);
    pwm_start();

    while (1)
    {
        /* code */
    }
}
