# PWM LED Example

Drives the Nucleo-64 on-board LED (LD2, PA5) at a fixed brightness using
TIM2 Channel 1 PWM, instead of a simple digital on/off.

## Wiring

No external wiring required. Uses on-board peripherals only.

| Signal | Pin | Component |
|---|---|---|
| PWM output | PA5 (AF1, TIM2_CH1) | LD2 (on-board) |

## Build and Flash

```bash
cmake -B build -DEXAMPLE=pwm_led -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Behaviour

LD2 lights at a fixed, reduced brightness (25% duty cycle) rather than full
on/off — no visible flicker, since the PWM frequency is far above what the
eye can perceive.

## Notes

- `PRESCALER`, `PERIOD`, and `DUTY` are compile-time `#define`s in `main.c` —
  edit and rebuild to change brightness or PWM frequency.
- `set_duty_cycle()` can be called again at runtime (e.g. in the `while(1)`
  loop) to change brightness on the fly — see `drivers/pwm/README.md` for
  details on the preload behavior that makes this glitch-free.
