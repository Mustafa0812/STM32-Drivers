# LED Example

Demonstrates GPIO control of the Nucleo-64 on-board LED (LD2) using both polling and interrupt-driven button input. A single `#define` at the top of `main.c` switches between modes.

## Modes

| Mode | How it works |
|---|---|
| Interrupt (`#define USE_INTERRUPT`) | Button press triggers a falling-edge EXTI interrupt; ISR sets a flag; `while(1)` loop handles the LED blink |
| Polling (`#define USE_INTERRUPT` commented out) | `btn_state()` is checked on every iteration of `while(1)` |

In both modes: pressing the user button turns LD2 on for 1 second, then off.

## Wiring

No external wiring required. Uses on-board peripherals only.

| Signal | Pin | Component |
|---|---|---|
| LED output | PA5 | LD2 (on-board) |
| Button input | PC13 | User button (on-board, active-low) |

## Build and Flash

```bash
cmake -B build -DEXAMPLE=led -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Behaviour

- Press the user button → LD2 turns on for 1 second → turns off
- In interrupt mode: the ISR returns immediately; the blink happens in `main()`
- In polling mode: `btn_state()` is checked continuously in the loop
