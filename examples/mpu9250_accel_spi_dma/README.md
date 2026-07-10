# MPU-9250 Accelerometer — SPI + DMA

Reads X, Y, Z accelerometer axes from an MPU-9250 over SPI1, using DMA2 for
every transfer instead of polling. The CPU is free during each transaction
and resumes only when the DMA transfer-complete interrupt fires. Also reads
and prints the WHO_AM_I register on startup to verify the connection, and
computes roll/pitch tilt angles from the accelerometer readings alone via
`atan2f`.

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
cmake -B build -DEXAMPLE=mpu9250_accel_spi_dma -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
WHO_AM_I: 0x71 (expect 0x71)
acc_x : 12 mg  acc_y : -8 mg  acc_z : 998 mg
roll_angle: 2 degrees   pitch angle: -1 degrees
```

If `WHO_AM_I` reads `0x00` or `0xFF`, check wiring — the device is not
responding on the bus.

Roll/pitch are computed from the accelerometer alone (`atan2f(y, sqrt(x²+z²))`
style formulas, converted from radians to degrees) — see
`drivers/mpu9250/README.md` for the exact math. Because this uses no gyro
data, angles are jittery under motion but never drift over time — the
opposite trade-off from `examples/mpu_gyro_spi_dma`.

## Initialisation Sequence

1. `uart_init()` — USART2 TX on PA2, 115200 baud
2. `spi_gpio_init()` — configures PB3/PB4/PB5 as AF5
3. `spi_init()` — enables SPI1; Mode 0, 8-bit, fPCLK/32 ≈ 500 kHz
4. `dma2_init()` — enables DMA2, SPI1 TXDMAEN/RXDMAEN; must come after `spi_init()`
5. `mpu_init()` — configures PA4 as CS; wakes MPU-9250; sets ±4 g range
6. Reads WHO_AM_I register once, then continuous burst-read of 6 bytes from
   `ACCEL_XOUT_H` (0x3B), each transfer driven by DMA2 Stream2/Stream3;
   roll/pitch angles are computed from each new reading

See `drivers/dma/dma_spi/README.md` for how the DMA transfer itself works.

## Build Requirements

Roll/pitch computation uses `atan2f`/`sqrtf` (`math.h`) and `%f`-style float
printing. Both require extra linker flags already set in the top-level
`CMakeLists.txt`: `-lm` for the math functions, and `-Wl,-u,_printf_float`
because newlib-nano strips float printf support by default.
