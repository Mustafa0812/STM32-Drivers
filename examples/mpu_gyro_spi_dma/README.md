# MPU-9250 Gyroscope Integration — SPI + DMA

Reads X/Y/Z gyroscope axes from an MPU-9250 over SPI1+DMA (±250 °/s, 131
LSB/°/s) and integrates angular rate over time to estimate roll/pitch/yaw:
`angle += rate × dt`. Uses the SysTick driver's `start_timer()`/`get_tick()`
to measure `dt` between samples instead of a fixed loop delay.

This is a deliberately gyro-only step — no accelerometer correction — to
demonstrate the core limitation that motivates sensor fusion: gyro bias
drifts the integrated angle away from 0 over time, even when the board is
perfectly still. See `examples/mpu9250_accel_spi_dma` for the
accelerometer-only alternative (no drift, but jittery under motion), and
`drivers/mpu9250/README.md` for the underlying read/convert math.

## Wiring

| Nucleo-64 Pin | MPU-9250 Pin | Notes |
|---|---|---|
| 3.3V | VCC | |
| GND | GND | |
| PA4 | NCS | Software chip-select (active-low) |
| PB3 | SCL/CLK | SPI1 SCK (AF5) |
| PB4 | SDA/SDO | SPI1 MISO (AF5) |
| PB5 | SDI | SPI1 MOSI (AF5) |

## Build and Flash

```bash
cmake -B build -DEXAMPLE=mpu_gyro_spi_dma -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
WHO_AM_I: 0x71 (expect 0x71)
time: 22
gyro_x : 1 mg  gyro_y : -2 mg  gyro_z : 0 mg
roll_angle: 0 degrees   pitch angle: -1 degrees
```

With the board perfectly still, `roll_angle`/`pitch_angle` should stay near
0 initially and then drift slowly and steadily — that drift is gyro bias
accumulating through integration, not a bug. Rotating the board should move
the angle roughly proportionally to the rotation.

## Initialisation Sequence

1. `start_timer()` — starts the SysTick free-running tick counter; must run
   before any `delay()` or `get_tick()` call, including inside `mpu_init()`
2. `uart_init()` — USART2 TX on PA2, 115200 baud
3. `spi_gpio_init()` — configures PB3/PB4/PB5 as AF5
4. `spi_init()` — enables SPI1; Mode 0, 8-bit, fPCLK/32 ≈ 500 kHz
5. `dma2_init()` — enables DMA2, SPI1 TXDMAEN/RXDMAEN; must come after `spi_init()`
6. `mpu_init()` — configures PA4 as CS; wakes MPU-9250; sets ±4 g accel range and ±250 °/s gyro range
7. Reads WHO_AM_I register once, then each loop iteration: burst-reads 6
   bytes from `GYRO_XOUT_H` (0x43), converts to °/s, computes `dt` since the
   last reading via `get_tick()`, integrates `angle += rate × (dt / 1000.0f)`,
   and prints — followed by a short `delay()` before the next sample

## Notes

- `dt` is captured immediately after each reading (not after the trailing
  `delay()`), so the delay is correctly included in the *next* iteration's
  `dt` rather than being discarded.
- A shorter loop delay gives finer-grained integration steps and more
  predictable (closer to linear) drift; a long delay (e.g. seconds) makes
  each step's error — including any brief rotation or noise spike — get
  multiplied by a large `dt`, exaggerating error per step.
- This example intentionally does not correct drift — that requires
  combining this gyro angle with the accelerometer's absolute (but noisy)
  tilt angle in a complementary or Kalman filter.
