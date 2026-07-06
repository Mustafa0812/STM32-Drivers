# FreeRTOS Sensor + Blink Example

Two equal-priority FreeRTOS tasks running concurrently: `led_blink` toggles
LD2 (PA5) once per second, and `mpu_main` continuously burst-reads the
MPU-9250 accelerometer over SPI1+DMA and prints milli-g values over UART.
Demonstrates round-robin scheduling between independent tasks with no shared
state or synchronization needed between them.

## What it demonstrates

- Two tasks created at the same priority (`1`) — the scheduler time-slices
  between them; neither can starve or block the other
- Each task uses its own `vTaskDelay()` independently — the LED's 1 Hz blink
  and the sensor's 1 Hz read/print run on completely independent timing,
  visibly at the same time
- All peripheral init (`led_init`, `uart_init`, `spi_gpio_init`, `spi_init`,
  `dma2_init`, `mpu_init`) happens once in `main()`, before either task is
  created or the scheduler starts

## Wiring

| Nucleo-64 Pin | Component | Notes |
|---|---|---|
| PA5 | LD2 (on-board) | LED output — no external wiring |
| 3.3V | MPU-9250 VCC | |
| GND | MPU-9250 GND | |
| PA4 | MPU-9250 NCS | Software chip-select (active-low) |
| PB3 | MPU-9250 SCL/CLK | SPI1 SCK (AF5) |
| PB4 | MPU-9250 SDA/SDO | SPI1 MISO (AF5) |
| PB5 | MPU-9250 SDI | SPI1 MOSI (AF5) |

> SPI1 uses its alternate pin set (PB3/4/5) rather than the default (PA5/6/7)
> specifically so LD2 (PA5) doesn't conflict with SPI1 SCK while both the
> blink task and the sensor task are running at the same time. If you're
> coming from `mpu9250_accel_spi`/`mpu9250_accel_spi_dma`, note those examples
> share the same `spi_gpio_init()` and now use PB3/4/5 too — see the root
> [README](../../README.md#pin-summary).

## Build and Flash

```bash
cmake -B build -DEXAMPLE=freertos_sensor_blink -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Behaviour

LD2 blinks on/off every second, independent of the accelerometer readings
scrolling over UART (115200 baud, 8N1, ST-LINK COM port) roughly once per
second:

```
raw: 00 60 FF F8 3F 00
acc_x : 12 mg  acc_y : -8 mg  acc_z : 998 mg
```

## Notes

- Both tasks are created at priority `1` (see `config/FreeRTOSConfig.h`,
  `configMAX_PRIORITIES = 2`) — genuinely equal, round-robin scheduling, not
  preemption. To make one task able to interrupt the other, `configMAX_PRIORITIES`
  would need raising and one task moved to a higher priority level.
- `vApplicationStackOverflowHook()` reports over UART via `printf` (already
  retargeted through `__io_putchar`), same as `freertos_blink`.
