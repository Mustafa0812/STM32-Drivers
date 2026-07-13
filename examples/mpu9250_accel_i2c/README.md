# MPU-9250 Accelerometer, Tilt, and Magnetometer Heading — I2C

Reads the MPU-9250's accelerometer over I2C1, computes roll/pitch tilt
angles from it, and — via I2C pass-through mode — reads the AK8963
magnetometer packaged inside the same chip to compute a compass heading.
Prints all three over UART (ST-LINK virtual COM port).

This example is where magnetometer support was added to this repo's
MPU-9250 driver; see `drivers/mpu9250/README.md` for the full read/convert
math and pass-through setup shared by any future I2C-based example.

## Wiring

| Nucleo-64 Pin | MPU-9250 Pin | Notes |
|---|---|---|
| 3.3V | VCC | |
| GND | GND | |
| GND | AD0 | Pulls I2C address to 0x68 |
| PB8 | SCL | I2C1 clock (AF4, open-drain) |
| PB9 | SDA | I2C1 data (AF4, open-drain) |

> External pull-up resistors (4.7 kΩ) on SCL and SDA are recommended. The driver enables internal pull-ups but these may be insufficient at full 100 kHz for long wires.
>
> No extra wiring is needed for the magnetometer — pass-through mode bridges the AK8963 onto this same I2C1 bus at address `0x0C`, since the main MPU-9250 interface here is I2C (not SPI). See `drivers/mpu9250/README.md` for why this only works this simply over I2C.

## Build and Flash

```bash
cmake -B build -DEXAMPLE=mpu9250_accel_i2c -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
roll_angle: 2 degrees   pitch angle: -1 degrees
heading: 47 degrees
```

Roll/pitch should stay near 0 with the board flat and respond to tilting
(same accelerometer-only tilt math as `examples/mpu9250_accel_spi_dma` —
noisy under motion, no drift). `heading` should change as you rotate the
board around its vertical (Z) axis, and stay roughly constant otherwise —
it is *not* tilt-compensated, so tilting the board while reading heading
will introduce error.

## Initialisation Sequence

1. `start_timer()` — starts the SysTick tick counter; must run before any `delay()` call, including inside `mpu_init()`
2. `uart_init()` — USART2 TX on PA2, 115200 baud
3. `i2c_init()` — I2C1 on PB8/PB9, 100 kHz standard mode
4. `mpu_init()` — wakes the MPU-9250, sets ±4 g accel / ±250 dps gyro range, enables I2C pass-through, and initialises the AK8963 (reads factory sensitivity values, sets continuous measurement mode)
5. Each loop iteration: burst-reads accelerometer and magnetometer data, reads `ST2` to release the next magnetometer measurement, computes roll/pitch from the accelerometer and heading from the magnetometer, and prints

## Build Requirements

Uses `atan2f`/`sqrtf` (`math.h`) and `%f`-style float printing. Requires
`-lm` and `-Wl,-u,_printf_float`, both already set in the top-level
`CMakeLists.txt`.

## Limitations

- **Heading is not tilt-compensated** — accurate only when the board is
  roughly level. A tilted-heading formula would need to fold in the
  accelerometer's roll/pitch, not implemented here.
- **No hard/soft-iron calibration** — nearby ferrous metal or magnetic
  fields (motors, speakers) will bias the heading. Not corrected in this
  example.
- **No resting-offset calibration** — small constant roll/pitch offsets at
  rest come from the accelerometer's own static bias, not a bug — see
  `examples/kalman_filter_spi_dma/README.md`'s Calibration section for the
  same issue on the other MPU-9250 examples.
