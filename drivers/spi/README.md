# SPI Driver

Full-duplex SPI1 master driver on PB3/PB4/PB5. Chip-select is not managed here — it is the responsibility of the device driver (e.g. MPU-9250).

## Configuration

| Parameter | Value |
|---|---|
| Peripheral | SPI1 |
| Mode | 0 (CPOL=0, CPHA=0) |
| Bit order | MSB first |
| Frame size | 8-bit |
| Clock | fPCLK/32 ≈ 500 kHz at 16 MHz |
| Alternate function | AF5 |

## Pins

| Pin | Signal |
|---|---|
| PB3 | SCK |
| PB4 | MISO |
| PB5 | MOSI |

## Functions

| Function | Description |
|---|---|
| `spi_gpio_init()` | Configures PB3/PB4/PB5 as AF5 |
| `spi_init()` | Enables SPI1 clock; configures CR1; enables SPE |
| `spi_transceive(data)` | Sends one byte and returns the simultaneously received byte; blocking |

## Usage

```c
#include "spi.h"

spi_gpio_init();
spi_init();

uint8_t received = spi_transceive(0xAB);
```

Call `spi_gpio_init()` before `spi_init()`. CS assertion/deassertion is done by the calling device driver around each transaction.

## Limitations

- **Polling only** — `spi_transceive` blocks on TXE and RXNE flags; no interrupt or DMA support
- **Fixed clock** — fPCLK/32 hardcoded; requires source change to alter
- **No multi-device CS management** — each device driver must manage its own CS pin
