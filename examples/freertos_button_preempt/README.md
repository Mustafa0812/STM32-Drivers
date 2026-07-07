# FreeRTOS Button Preemption Example

Builds on [`freertos_sensor_blink`](../freertos_sensor_blink/README.md)'s two
round-robin tasks (`led_blink`, `mpu_main`, both priority `1`) and adds a
third, higher-priority task woken by the user button's interrupt — proving
genuine preemption, not just round-robin time-slicing.

## What it demonstrates

- A task (`button_task`, priority `2`) that sits **Blocked** (zero CPU) via
  `ulTaskNotifyTake()` until signaled, rather than polling
- Waking a task safely **from an ISR** using `vTaskNotifyGiveFromISR()` — the
  ISR itself never touches task logic directly, it only signals
- `portYIELD_FROM_ISR()` forcing an *immediate* context switch (not waiting
  up to one tick period, ~1 ms at `configTICK_RATE_HZ = 1000`, for the
  scheduler to notice on its own)
- Why the interrupt's NVIC priority has to be explicitly set at or below
  `configMAX_SYSCALL_INTERRUPT_PRIORITY` (5) before it's safe to call
  FreeRTOS ISR-safe functions from it — see `exti_init()` in
  `drivers/gpio/src/gpio_it.c`

## Wiring

Same as `freertos_sensor_blink`, plus the on-board user button (no extra
wiring — PC13 is on-board).

| Nucleo-64 Pin | Component | Notes |
|---|---|---|
| PA5 | LD2 (on-board) | LED output |
| PC13 | User button (on-board) | Active-low, falling-edge EXTI |
| 3.3V | MPU-9250 VCC | |
| GND | MPU-9250 GND | |
| PA4 | MPU-9250 NCS | Software chip-select (active-low) |
| PB3 | MPU-9250 SCL/CLK | SPI1 SCK (AF5) |
| PB4 | MPU-9250 SDA/SDO | SPI1 MISO (AF5) |
| PB5 | MPU-9250 SDI | SPI1 MOSI (AF5) |

## Build and Flash

```bash
cmake -B build -DEXAMPLE=freertos_button_preempt -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Behaviour

LD2 blinks at 1 Hz and accelerometer readings scroll over UART, same as
`freertos_sensor_blink`. Pressing the user button interrupts whichever of
those two was running at that instant — LD2 immediately flashes rapidly
three times (100 ms on/off), then both background tasks resume exactly
where preemption left them.

## Notes

- `configMAX_PRIORITIES = 3` — Idle (`0`), `led_blink`/`mpu_main` (`1`),
  `button_task` (`2`).
- `exti_register_callback()` must be called *before* `exti_init()` — the
  interrupt goes live the instant `exti_init()` returns, and a press before
  registration would call a `NULL` callback (silently ignored, and lost).
- The button ISR's NVIC priority must be numerically `>= 5`
  (`configMAX_SYSCALL_INTERRUPT_PRIORITY`). An interrupt left at its default
  priority (`0`, the most urgent) calling `vTaskNotifyGiveFromISR()` can
  corrupt FreeRTOS's internal state if it happens to fire while the kernel is
  mid-critical-section — a timing-dependent bug that may not show up until
  long after it's shipped.
