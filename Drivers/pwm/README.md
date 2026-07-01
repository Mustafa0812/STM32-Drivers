# PWM Driver (User LED only)

Drives the Nucleo-64 on-board user LED (LD2, PA5) with a PWM brightness signal using TIM2 Channel 1 (AF1). This is **not** a general-purpose PWM driver — it is hardcoded to this one pin/timer/channel and exists solely to dim/brighten LD2.

## Configuration

| Parameter | Value |
|---|---|
| Timer | TIM2, Channel 1 |
| Pin | PA5 (AF1) |
| Mode | PWM mode 1 (output high while `CNT < CCR1`) |
| Preload | ARR and CCR1 preload enabled (glitch-free updates) |

## Functions

| Function | Description |
|---|---|
| `pwm_init(prescaler)` | Enables GPIOA/TIM2 clocks; configures PA5 as AF1; sets TIM2 CH1 to PWM mode 1; sets the prescaler |
| `set_duty_cycle(duty, period)` | Sets `TIM2->ARR = period` and `TIM2->CCR1 = duty * period / 100` (`duty` is a percentage, 0–100) |
| `pwm_start()` | Enables the CH1 output and starts the counter |

## Usage

```c
#include "pwm.h"

#define PRESCALER 15
#define PERIOD    10000
#define DUTY      25    /* percent */

int main(void) {
    pwm_init(PRESCALER);
    set_duty_cycle(DUTY, PERIOD);
    pwm_start();

    while (1) {
        /* code */
    }
}
```

Call `set_duty_cycle()` again at any time (e.g. in the main loop) to change brightness at runtime — CCR1 is preloaded, so updates take effect on the next counter overflow without glitching the current pulse.

## Limitations

- **Hardcoded to TIM2 CH1 / PA5** — not configurable for other timers/channels without modifying `pwm_init()`
- **`duty` must be 0–100** — no bounds checking; values outside this range give a `CCR1` outside `ARR`, i.e. output always high
- **PA5 shared with SPI1 SCK and the GPIO LED driver** — do not use alongside `led_on()`/`led_off()` or SPI1
