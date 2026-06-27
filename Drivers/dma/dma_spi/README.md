# DMA SPI Driver

DMA-driven SPI1 full-duplex transfer driver for the STM32F411. Replaces the polling loop in `spi_transceive` with DMA2 Stream3 (TX) and Stream2 (RX), freeing the CPU for the duration of each transfer.

## Configuration

| Parameter | Value |
|---|---|
| Peripheral | SPI1 |
| TX stream | DMA2 Stream3, Channel 3 |
| RX stream | DMA2 Stream2, Channel 3 |
| Transfer size | 8-bit (matches SPI1 DFF=0) |
| Direction | Memory-to-peripheral (TX), Peripheral-to-memory (RX) |
| Memory increment | Enabled on both streams |
| Completion signal | Transfer-complete interrupt on Stream2 (RX) |

## Dependencies

Requires the SPI bus driver (`spi.h`). `spi_gpio_init()` and `spi_init()` must be called before `dma2_init()` — SPI1 must be clocked before its CR2 TXDMAEN/RXDMAEN bits are written.

## Functions

| Function | Description |
|---|---|
| `dma2_init()` | Enables DMA2 clock; sets TXDMAEN and RXDMAEN in SPI1 CR2; enables NVIC for Stream2 and Stream3 |
| `spi_dma_transceive(rx, tx, len)` | Full-duplex DMA transfer of `len` bytes; blocks until RX transfer-complete interrupt fires |
| `spi_dma_transmit(tx, len)` | TX-only wrapper around `spi_dma_transceive`; RX is discarded into an internal 32-byte dummy buffer |

## Usage

```c
#include "spi.h"
#include "dma_spi.h"

spi_gpio_init();
spi_init();
dma2_init();

/* Full-duplex — both buffers matter */
uint8_t tx[4] = {0x80, 0xFF, 0xFF, 0xFF};
uint8_t rx[4];
spi_dma_transceive(rx, tx, 4);
/* rx[1..3] now contains the sensor response */

/* TX only — no RX buffer needed */
uint8_t cmd[2] = {0x6B, 0x00};
spi_dma_transmit(cmd, 2);
```

## How It Works

SPI always transfers in both directions simultaneously. Each DMA stream responds to a hardware request from SPI1:

- **Stream3 (TX):** fires on TXE — reads from the TX buffer and writes to `SPI1->DR`
- **Stream2 (RX):** fires on RXNE — reads from `SPI1->DR` and writes to the RX buffer

Both streams are armed for each transfer. RX is armed first to guarantee it is watching `SPI1->DR` before the first byte is clocked out by TX. The transfer-complete interrupt on Stream2 sets an internal flag; `spi_dma_transceive` spins on that flag and returns once the last byte is in the RX buffer.

For write-only operations, `spi_dma_transmit` passes a static internal `rx_dummy` buffer so the caller does not need to manage a throwaway RX allocation.

## Limitations

- **Blocking** — `spi_dma_transceive` busy-waits on the TC flag; the CPU is free during the DMA transfer but stalls in the spin loop until completion
- **Single transfer at a time** — no queuing; do not call from two contexts simultaneously
- **32-byte dummy limit** — `spi_dma_transmit` uses a 32-byte internal RX buffer; do not call with `len > 32`
- **SPI1 only** — hardcoded to SPI1 and DMA2 Stream2/3 Channel 3
