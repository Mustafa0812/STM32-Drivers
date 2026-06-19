# I2C Driver

I2C1 master driver on PB8 (SCL) and PB9 (SDA), configured for standard mode at 100 kHz. Provides single-byte write and multi-byte burst read operations for use by device drivers.

## Configuration

| Parameter | Value |
|---|---|
| Peripheral | I2C1 |
| Mode | Standard mode (100 kHz) |
| Alternate function | AF4 |
| Output type | Open-drain |
| Pull-ups | Internal (enabled in PUPDR) |
| APB1 clock | 16 MHz |

## Pins

| Pin | Signal |
|---|---|
| PB8 | SCL |
| PB9 | SDA |

## Functions

| Function | Description |
|---|---|
| `i2c_init()` | Configures PB8/PB9 as AF4 open-drain; enables I2C1 clock; sets CCR and TRISE; enables peripheral |
| `write_reg(slave, target, data)` | Writes one byte `data` to register `target` on 7-bit address `slave` |
| `burst_read_reg(slave, target, n, buffer)` | Reads `n` bytes starting at register `target` from device `slave` into `buffer` |

## Usage

```c
#include "i2c.h"

i2c_init();

write_reg(0x68, 0x6B, 0x00);          /* write single register */

uint8_t buf[6];
burst_read_reg(0x68, 0x3B, 6, buf);   /* read 6 bytes */
```

Addresses are **7-bit, not pre-shifted** — the driver shifts them internally before placing on the bus.

## Limitations

- **Polling only** — all operations block on SR1/SR2 flags; no interrupt or DMA support
- **No timeout** — if the bus stalls (e.g. a slave holds SDA low), the driver loops forever
- **Fixed speed** — 100 kHz hardcoded; fast mode (400 kHz) not supported
- **Single bus** — hardcoded to I2C1; no support for I2C2 or I2C3
