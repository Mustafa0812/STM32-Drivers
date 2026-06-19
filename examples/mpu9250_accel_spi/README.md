# MPU-9250 Accelerometer — SPI

Reads X, Y, Z accelerometer axes from an MPU-9250 over SPI1 and prints the results in milli-g over UART (ST-LINK virtual COM port). Also reads and prints the WHO_AM_I register on startup to verify the connection.

## Wiring

| Nucleo-64 Pin | MPU-9250 Pin | Notes |
|---|---|---|
| 3.3V | VCC | |
| GND | GND | |
| PA4 | NCS | Software chip-select (active-low) |
| PA5 | SCL/CLK | SPI1 SCK (AF5) |
| PA6 | SDA/SDO | SPI1 MISO (AF5) |
| PA7 | SDI | SPI1 MOSI (AF5) |

> PA5 is shared with the Nucleo-64 on-board LED (LD2). Do not use the LED while SPI is active.

## Build and Flash

```bash
cmake -B build -DEXAMPLE=mpu9250_accel_spi -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
WHO_AM_I: 0x71 (expect 0x71)
raw: 00 60 FF F8 3F 00
acc_x : 12 mg  acc_y : -8 mg  acc_z : 998 mg
```

If `WHO_AM_I` reads `0x00` or `0xFF`, check wiring — the device is not responding on the bus.

## Initialisation Sequence

1. `uart_init()` — USART2 TX on PA2, 115200 baud
2. `spi_gpio_init()` — configures PA5/PA6/PA7 as AF5
3. `spi_init()` — enables SPI1; Mode 0, 8-bit, fPCLK/32 ≈ 500 kHz
4. `mpu_init()` — configures PA4 as CS; wakes MPU-9250; sets ±4 g range
5. Reads WHO_AM_I register once, then continuous burst-read of 6 bytes from `ACCEL_XOUT_H` (0x3B)
