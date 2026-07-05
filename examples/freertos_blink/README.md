# FreeRTOS Blink Example

First FreeRTOS example in this repo: a single task blinks the on-board LED
(LD2) once per second using `vTaskDelay()` instead of a blocking bare-metal
delay, demonstrating the minimum setup needed to run FreeRTOS on the
STM32F411.

## What it demonstrates

- One-time hardware init (`led_init()`) before the scheduler starts
- Registering a task with `xTaskCreate()`
- Handing control to FreeRTOS with `vTaskStartScheduler()` (never returns)
- A task yielding the CPU with `vTaskDelay(pdMS_TO_TICKS(...))` instead of
  busy-waiting
- `vApplicationStackOverflowHook()`, required by
  `configCHECK_FOR_STACK_OVERFLOW == 2` in `config/FreeRTOSConfig.h`, reporting
  over UART2 via `printf` (already retargeted through `__io_putchar`)

## Wiring

No external wiring required. Uses on-board peripherals only.

| Signal | Pin | Component |
|---|---|---|
| LED output | PA5 | LD2 (on-board) |
| Stack-overflow message | PA2 | USART2 TX → ST-LINK virtual COM port |

## Build and Flash

```bash
cmake -B build -DEXAMPLE=freertos_blink -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

`third_party/FreeRTOS` (kernel, GCC/ARM_CM4F port, heap_4) is only compiled
in when `EXAMPLE=freertos_blink` — bare-metal examples don't link against it
and don't need to provide `vApplicationStackOverflowHook()`.

## Expected Behaviour

LD2 turns on for 1 second, off for 1 second, repeating indefinitely.

## Notes

- `configMAX_PRIORITIES = 2`, `configTICK_RATE_HZ = 1000` — see the
  project-specific comment block at the top of `config/FreeRTOSConfig.h` for
  the reasoning behind every non-default setting.
- Never call the bare-metal `delay()` from `drivers/systick` once
  `vTaskStartScheduler()` has run — it directly resets SysTick's control
  register and permanently stops the RTOS tick.
