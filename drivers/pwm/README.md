# PWM Driver

Two independent PWM drivers, sharing one header (`pwm.h`):

- `pwm_led.c` — dims/brightens the Nucleo-64 on-board user LED (LD2, PA5) via TIM2 CH1. Hardcoded to this one pin/timer/channel.
- `quad_pwm.c` — general-purpose 4-channel PWM for motor/ESC control via TIM4 CH1-4 (PB6/PB7/PB8/PB9). Caller picks which channel to drive; not tied to any specific device.

---

## LED PWM (`pwm_led.c`)

### Configuration

| Parameter | Value |
|---|---|
| Timer | TIM2, Channel 1 |
| Pin | PA5 (AF1) |
| Mode | PWM mode 1 (output high while `CNT < CCR1`) |
| Preload | ARR and CCR1 preload enabled (glitch-free updates) |

### Functions

| Function | Description |
|---|---|
| `pwm_init(prescaler)` | Enables GPIOA/TIM2 clocks; configures PA5 as AF1; sets TIM2 CH1 to PWM mode 1; sets the prescaler |
| `set_duty_cycle(duty, period)` | Sets `TIM2->ARR = period` and `TIM2->CCR1 = duty * period / 100` (`duty` is a percentage, 0–100) |
| `pwm_start()` | Enables the CH1 output and starts the counter |

### Usage

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

### Limitations

- **Hardcoded to TIM2 CH1 / PA5** — not configurable for other timers/channels without modifying `pwm_init()`
- **`duty` must be 0–100** — no bounds checking; values outside this range give a `CCR1` outside `ARR`, i.e. output always high
- **PA5 shared with the GPIO LED driver** — do not use alongside `led_on()`/`led_off()` (both drive the same pin)

---

## Quad Motor PWM (`quad_pwm.c`)

### Configuration

| Parameter | Value |
|---|---|
| Timer | TIM4, Channels 1-4 |
| Pins | PB6 (CH1), PB7 (CH2), PB8 (CH3), PB9 (CH4) — all AF2 |
| Mode | PWM mode 1 (output high while `CNT < CCRx`), active-high polarity |
| Preload | ARR and all four CCRx preloaded (glitch-free updates) |

All four channels share one counter, one prescaler, and one period (`ARR`) — they cannot run at different PWM frequencies. Only duty cycle is independent per channel, via `CCR1`-`CCR4`.

### Functions

| Function | Description |
|---|---|
| `quad_pwm_init(prescaler)` | Enables GPIOB/TIM4 clocks; configures PB6-9 as AF2; sets all 4 channels to PWM mode 1, active-high; sets the shared prescaler |
| `quad_pwm_set_duty(duty, period, channel)` | Sets the shared `TIM4->ARR = period` and `TIM4->CCRx = duty * period / 100` for the given `channel` (1-4); `duty` is a percentage, 0–100 |
| `quad_pwm_start()` | Enables all 4 channel outputs and starts the shared counter |

### Usage

```c
#include "pwm.h"

#define PRESCALER 15
#define PERIOD    10000

int main(void) {
    quad_pwm_init(PRESCALER);
    quad_pwm_set_duty(25, PERIOD, 1);   /* channel 1 (PB6) at 25% */
    quad_pwm_set_duty(50, PERIOD, 2);   /* channel 2 (PB7) at 50% */
    quad_pwm_start();

    while (1) {
        /* code */
    }
}
```

Call `quad_pwm_set_duty()` again at any time to change any channel's duty cycle independently — `CEN` only needs to be started once via `quad_pwm_start()`.

### Limitations

- **Hardcoded to TIM4 CH1-4 / PB6-9** — not configurable for other timers/pins without modifying `quad_pwm_init()`
- **`duty` must be 0–100** — no bounds checking, same as `pwm_led.c`
- **`channel` must be 1-4** — any other value is a silent no-op in `quad_pwm_set_duty()`
- **All 4 channels always started together** — `quad_pwm_start()` has no per-channel enable/disable; an unused channel's output pin should just be left unconnected
- **Shared period across all channels** — cannot mix different PWM frequencies on different channels of this driver
