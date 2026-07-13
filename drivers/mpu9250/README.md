# MPU-9250 Driver

Accelerometer/gyroscope driver for the InvenSense MPU-9250 IMU. Two transport implementations are provided — SPI and I2C — with the same initialisation and read interface. The AK8963 magnetometer packaged inside the same chip is supported via the I2C transport (see [Magnetometer (AK8963)](#magnetometer-ak8963) below) — not currently wired up for the SPI transports.

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
| `mpu_init()` | Wakes device; sets ±4 g accel / ±250 dps gyro range; enables I2C pass-through; initialises the AK8963 magnetometer (`i2c_init()` must be called first) |
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

## Magnetometer (AK8963)

The AK8963 magnetometer die is packaged inside the same MPU-9250 chip, but sits behind its own tiny internal "auxiliary I2C bus" that the MPU-9250 normally controls on your behalf. **I2C pass-through mode** disables that internal logic and instead electrically bridges the AK8963 straight onto the MPU-9250's *main* I2C bus, so it becomes just another ordinary device at its own address (`0x0C`) — reachable with the same generic `i2c.c` functions used for the MPU-9250 itself, no extra wiring required.

This only works this simply when the main MPU-9250 transport is I2C. Pass-through bridges the AK8963 onto the *main I2C bus* specifically — over SPI there is no main I2C bus for it to join, so using the magnetometer alongside a SPI-based transport would require physically wiring a separate I2C bus to the breakout's `AUX_DA`/`AUX_CL` pins (if exposed at all — many small breakout boards don't break them out). Not implemented in this repo; only the I2C transport (`mpu9250_i2c.c`) supports the magnetometer.

Currently implemented in `mpu9250_i2c.c`/`mpu9250_i2c.h`, not split into a separate `ak8963_i2c.c` file, since it's only reachable through the MPU-9250's pass-through and isn't usable standalone.

### Enabling pass-through (done automatically in `mpu_init()`)

| Register | Value | Effect |
|---|---|---|
| USER_CTRL (0x6A) | 0x00 | Clear `I2C_MST_EN` — required before `BYPASS_EN` takes effect |
| INT_PIN_CFG (0x37) | 0x02 | Set `BYPASS_EN` (bit 1) |

### AK8963 registers used

| Register | Address | Purpose |
|---|---|---|
| `WIA` | 0x00 | WHO_AM_I — reads `0x48` |
| `HXL`-`HZH` | 0x03-0x08 | Raw measurement data, **little-endian** (low byte first — opposite of the MPU-9250's own big-endian accel/gyro registers) |
| `ST2` | 0x09 | Must be read after each measurement to release the next one — omitting this stalls the data |
| `ASAX`/`ASAY`/`ASAZ` | 0x10-0x12 | Factory sensitivity-adjustment values, only readable in Fuse ROM access mode |
| `CNTL1` | 0x0A | Operating mode |

### Functions

| Function | Description |
|---|---|
| `mag_burst_read(reg, buffer)` | Reads 6 bytes from the AK8963 (fixed `MAG_ADDR = 0x0C`) starting at `reg` |
| `mag_read_reg(reg)` | Single-byte read from the AK8963 |
| `mag_write(reg, data)` | Single-byte write to the AK8963 |
| `x_sense`/`y_sense`/`z_sense` (`extern uint8_t`) | Factory sensitivity-adjustment values, read once during `mpu_init()` |

### Init sequence (inside `mpu_init()`)

1. Enable pass-through (see table above).
2. `CNTL1 = 0x0F` — Fuse ROM access mode.
3. Read `ASAX`/`ASAY`/`ASAZ` into `x_sense`/`y_sense`/`z_sense`.
4. `CNTL1 = 0x00` — power-down (required transition before switching modes).
5. `CNTL1 = 0x12` — continuous measurement mode 1, 16-bit output.

### Reading magnetometer data and computing heading

```c
uint8_t buf[6];
mag_burst_read(HXL, buf);
mag_read_reg(ST2);   /* required to release the next measurement */

/* little-endian: low byte first, opposite of accel/gyro reads */
int16_t mag_x = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
int16_t mag_y = (int16_t)((uint16_t)buf[3] << 8 | buf[2]);

/* factory sensitivity adjustment (PS-MPU-9250A-01 §5.13) */
float x_mag = mag_x * (((x_sense - 128) * 0.5f / 128) + 1);
float y_mag = mag_y * (((y_sense - 128) * 0.5f / 128) + 1);

/* AK8963's axes are rotated relative to the accel/gyro die inside the same
 * package (PS-MPU-9250A-01 Fig. 4 vs Fig. 5) — X/Y are swapped */
float mag_body_x = y_mag;
float mag_body_y = x_mag;

float heading = atan2f(mag_body_y, mag_body_x) * (180.0f / 3.142f);
```

Heading uses only the two horizontal-plane axes — unlike the accelerometer's roll/pitch tilt formula, there's no `sqrt`/`z` term, since heading is rotation about the vertical axis. Not tilt-compensated: accurate only when roughly level. See `examples/mpu9250_accel_i2c/README.md` for the full working example.

---

## Limitations

- **Magnetometer only supported via the I2C transport** — see [Magnetometer (AK8963)](#magnetometer-ak8963) above
- **No FIFO** — data is read directly from output registers; no buffering
- **No data-ready interrupt** — reads are polled; there is no DRDY pin handling
- **SPI and I2C APIs differ** — `mpu_burst_read` takes a `length` parameter in the SPI transport but reads a fixed 6 bytes in the I2C transport; they are not drop-in replacements for each other
- **Gyro range only configured in the DMA SPI and I2C transports** — the plain SPI transport never writes `GYRO_CONFIG`, so gyro reads through it use the device's power-on-default sensitivity unless configured manually
- **Heading is not tilt-compensated and has no hard/soft-iron calibration** — see `examples/mpu9250_accel_i2c/README.md` Limitations
