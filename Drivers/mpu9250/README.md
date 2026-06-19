# MPU-9250 Driver

Accelerometer driver for the InvenSense MPU-9250 IMU. Two transport implementations are provided — SPI and I2C — with the same initialisation and read interface. Only the accelerometer axes are read; the gyroscope is enabled but not used, and the magnetometer (AK8963) is not supported.

## Initialisation Settings

| Register | Value | Effect |
|---|---|---|
| PWR_MGMT_1 (0x6B) | 0x00 | Wake from sleep; use internal oscillator |
| PWR_MGMT_2 (0x6C) | 0x00 | Enable all accelerometer and gyro axes |
| ACCEL_CONFIG (0x1C) | 0x08 | Full-scale range ±4 g (8192 LSB/g) |

---

## SPI Transport (`mpu9250_spi.c`)

Uses SPI1 via the SPI bus driver. Bit 7 of the address byte selects read (1) or write (0); burst reads auto-increment the register address. Chip-select is PA4 (active-low, software-managed).

### Pins

| Pin | Signal |
|---|---|
| PA4 | CS (software, active-low) |
| PA5 | SCK |
| PA6 | MISO |
| PA7 | MOSI |

### Functions

| Function | Description |
|---|---|
| `mpu_init()` | Configures PA4 as CS output; wakes device; sets accelerometer range |
| `mpu_write(reg, data)` | Writes one byte to a register |
| `mpu_read_reg(reg)` | Reads one byte from a register |
| `mpu_burst_read(reg, length, buffer)` | Reads `length` bytes starting at `reg` into `buffer` |

### Usage

```c
#include "spi.h"
#include "mpu9250_spi.h"

spi_gpio_init();
spi_init();
mpu_init();

uint8_t buf[6];
mpu_burst_read(ACCEL_XOUT_H, 6, buf);
```

---

## I2C Transport (`mpu9250_i2c.c`)

Uses I2C1 via the I2C bus driver. Device address: 0x68 (AD0 pin low). `i2c_init()` must be called separately before `mpu_init()`.

### Functions

| Function | Description |
|---|---|
| `mpu_init()` | Wakes device; sets accelerometer range (`i2c_init()` must be called first) |
| `mpu_write(reg, data)` | Writes one byte to a register |
| `mpu_burst_read(reg, buffer)` | Reads 6 bytes starting at `reg` into `buffer` (length is fixed at 6) |

> `mpu_read_reg()` is declared in the header but **not implemented** in the I2C transport.

### Usage

```c
#include "i2c.h"
#include "mpu9250_i2c.h"

i2c_init();
mpu_init();

uint8_t buf[6];
mpu_burst_read(ACCEL_XOUT_H, buf);
```

---

## Reading Accelerometer Data

Both transports burst-read 6 bytes from `ACCEL_XOUT_H` (0x3B): X-high, X-low, Y-high, Y-low, Z-high, Z-low. Reconstruct signed 16-bit values and convert to milli-g:

```c
int16_t acc_x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
int16_t acc_y = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
int16_t acc_z = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);

int32_t acc_xmg = (int32_t)acc_x * 1000 / 8192;  /* ±4 g → 8192 LSB/g */
```

---

## Limitations

- **Accelerometer only** — gyroscope axes are enabled but no read functions are provided
- **No magnetometer** — the AK8963 onboard magnetometer is not initialised or accessible
- **No FIFO** — data is read directly from output registers; no buffering
- **No data-ready interrupt** — reads are polled; there is no DRDY pin handling
- **SPI and I2C APIs differ** — `mpu_burst_read` takes a `length` parameter in the SPI transport but reads a fixed 6 bytes in the I2C transport; they are not drop-in replacements for each other
