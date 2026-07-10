# MPU-9250 Driver

Accelerometer/gyroscope driver for the InvenSense MPU-9250 IMU. Two transport implementations are provided — SPI and I2C — with the same initialisation and read interface. The magnetometer (AK8963) is not supported.

## Initialisation Settings

| Register | Value | Effect |
|---|---|---|
| PWR_MGMT_1 (0x6B) | 0x00 | Wake from sleep; use internal oscillator |
| PWR_MGMT_2 (0x6C) | 0x00 | Enable all accelerometer and gyro axes |
| ACCEL_CONFIG (0x1C) | 0x08 | Full-scale range ±4 g (8192 LSB/g) |
| GYRO_CONFIG (0x1B) | 0x00 | Full-scale range ±250 °/s (131 LSB/°/s) — DMA SPI transport only |

---

## SPI Transport (`mpu9250_spi.c`)

Uses SPI1 via the SPI bus driver. Bit 7 of the address byte selects read (1) or write (0); burst reads auto-increment the register address. Chip-select is PA4 (active-low, software-managed).

### Pins

| Pin | Signal |
|---|---|
| PA4 | CS (software, active-low) |
| PB3 | SPI1 SCK (AF5) |
| PB4 | SPI1 MISO (AF5) |
| PB5 | SPI1 MOSI (AF5) |

> SPI1 uses its alternate pin set (PB3/4/5) rather than the default (PA5/6/7) so PA5 stays free for the on-board LED — see the top-level [Pin Summary](../../README.md#pin-summary).

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

## DMA SPI Transport (`mpu9250_spi-dma.c`)

Uses DMA2 Stream3/Stream2 via the DMA SPI driver (`dma_spi.h`) instead of polling. The same SPI1 bus and PA4 chip-select are used. Call `spi_gpio_init()`, `spi_init()`, and `dma2_init()` before `mpu_init()`.

For write operations (`mpu_write`) the RX side is discarded into the driver's internal dummy buffer — the caller does not need to provide an RX buffer. For reads, the DMA transfers a full TX+RX frame and only the response bytes are returned.

### Functions

| Function | Description |
|---|---|
| `mpu_init()` | Configures PA4 as CS output; wakes device; sets accelerometer range |
| `mpu_write(reg, data)` | 2-byte DMA transmit (reg + data); RX discarded |
| `mpu_read_reg(reg)` | 2-byte DMA transceive; returns the response byte |
| `mpu_burst_read(reg, length, buffer)` | (length+1)-byte DMA transceive; copies response bytes into `buffer` |

### Usage

```c
#include "spi.h"
#include "dma_spi.h"
#include "mpu9250_spi.h"

spi_gpio_init();
spi_init();
dma2_init();    /* must come after spi_init */
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

Roll/pitch tilt angle from the accelerometer alone (see `examples/mpu9250_accel_spi_dma/main.c`):

```c
float roll_denom  = sqrtf((acc_xmg * acc_xmg) + (acc_zmg * acc_zmg));
float pitch_denom = sqrtf((acc_ymg * acc_ymg) + (acc_zmg * acc_zmg));

float roll_angle  = atan2f(acc_ymg, roll_denom)        * (180.0f / 3.142f);
float pitch_angle = atan2f(-1 * acc_xmg, pitch_denom)  * (180.0f / 3.142f);
```

---

## Reading Gyroscope Data

There is no dedicated gyro-read function — burst-read 6 bytes from `GYRO_XOUT_H` (0x43) the same way as `ACCEL_XOUT_H`, then convert to °/s using the configured sensitivity (131 LSB/°/s at ±250 °/s, DMA SPI transport only):

```c
int16_t gyro_x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
float roll_rate = (float)gyro_x / 131.0f;   /* °/s */
```

Integrating `roll_rate` over a measured `dt` (see the SysTick driver's `get_tick()`) gives an angle estimate, but one that drifts over time from gyro bias — see `examples/mpu_gyro_spi_dma/main.c`.

---

## Limitations

- **No magnetometer** — the AK8963 onboard magnetometer is not initialised or accessible
- **No FIFO** — data is read directly from output registers; no buffering
- **No data-ready interrupt** — reads are polled; there is no DRDY pin handling
- **SPI and I2C APIs differ** — `mpu_burst_read` takes a `length` parameter in the SPI transport but reads a fixed 6 bytes in the I2C transport; they are not drop-in replacements for each other
- **Gyro range only configured in the DMA SPI transport** — the plain SPI and I2C transports never write `GYRO_CONFIG`, so gyro reads through them use the device's power-on-default sensitivity unless configured manually
