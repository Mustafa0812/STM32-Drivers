# MPU-9250 Complementary Filter — SPI + DMA

Fuses the MPU-9250's accelerometer and gyroscope over SPI1+DMA into drift-free
roll/pitch angle estimates using a complementary filter:

```
angle = alpha * (angle_prev + gyro_rate * dt) + (1 - alpha) * accel_angle
```

`angle_prev` is the filter's own previous *blended* output (not a
separately-tracked raw gyro integration) — feeding the blend back into itself
is what lets the small accelerometer correction each iteration continuously
cancel out gyro bias drift. `alpha = 0.98` weights the gyro term heavily
since it's the more trustworthy sensor on a short (per-iteration) timescale,
while the small `(1 - alpha)` accelerometer term dominates the long-run
average and pulls the estimate back whenever it drifts.

Builds directly on `examples/mpu9250_accel_spi_dma` (accel-only tilt angle)
and `examples/mpu_gyro_spi_dma` (gyro-only integration, which drifts) — this
example combines both to fix that drift. See `drivers/mpu9250/README.md` for
the underlying read/convert math for each sensor.

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
cmake -B build -DEXAMPLE=complementary_filter_spi_dma -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
WHO_AM_I: 0x71 (expect 0x71)
roll_angle: 2 degrees   pitch angle: -1 degrees
```

With the board still, both angles should stay near 0 and stay there — no
slow drift like the gyro-only example. Tilting the board should move the
angles roughly proportionally, settling back to a stable reading once
motion stops, without the jitter of the accelerometer-only example.

## Initialisation Sequence

1. `start_timer()` — starts the SysTick free-running tick counter; must run
   before any `delay()` or `get_tick()` call, including inside `mpu_init()`
2. `uart_init()` — USART2 TX on PA2, 115200 baud
3. `spi_gpio_init()` — configures PB3/PB4/PB5 as AF5
4. `spi_init()` — enables SPI1; Mode 0, 8-bit, fPCLK/32 ≈ 500 kHz
5. `dma2_init()` — enables DMA2, SPI1 TXDMAEN/RXDMAEN; must come after `spi_init()`
6. `mpu_init()` — configures PA4 as CS; wakes MPU-9250; sets ±4 g accel range and ±250 °/s gyro range
7. Reads WHO_AM_I register once, then each loop iteration: burst-reads both
   `GYRO_XOUT_H` and `ACCEL_XOUT_H`, computes `dt` via `get_tick()`, computes
   the accelerometer tilt angle and gyro rate, blends them, and prints

## Build Requirements

Uses `atan2f`/`sqrtf` (`math.h`) and `%f`-style float printing, same as
`examples/mpu9250_accel_spi_dma`. Requires `-lm` and `-Wl,-u,_printf_float`,
both already set in the top-level `CMakeLists.txt`.

## Limitations

- **No yaw** — roll/pitch use the accelerometer's gravity vector as an
  absolute reference; pure yaw rotation doesn't change that vector at all,
  so the accelerometer can't correct gyro yaw drift. A magnetometer
  (unsupported on this board — see `drivers/mpu9250/README.md`) would be
  needed for drift-free yaw.
- **Fixed `alpha`** — the 98/2 blend weight is a constant, not adapted to
  real-time sensor trust (e.g. it doesn't know to trust the accelerometer
  less during a hard shake). See `examples/kalman_filter_spi_dma` (planned)
  for an adaptive alternative.
