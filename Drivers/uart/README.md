# UART Driver

Transmit-only USART2 driver on PA2, connected to the Nucleo-64's ST-LINK virtual COM port. Retargets the Newlib `__io_putchar` hook so `printf` and `putchar` output over UART without any further configuration.

## Configuration

| Parameter | Value |
|---|---|
| Peripheral | USART2 |
| TX pin | PA2 (AF7) |
| Baud rate | 115200 |
| Frame | 8N1 |
| Clock | 16 MHz HSI |

## Functions

| Function | Description |
|---|---|
| `uart_init()` | Configures PA2 as AF7; enables USART2 clock; sets BRR; enables TE and UE |
| `__io_putchar(ch)` | Newlib retargeting hook — called internally by `printf`/`putchar` |

## Usage

```c
#include "uart.h"
#include <stdio.h>

uart_init();
printf("Hello\r\n");
```

No further setup needed — `printf` routes through `__io_putchar` automatically once `uart_init()` has been called.

## Limitations

- **TX only** — no RX pin configured; cannot receive data
- **Blocking** — `uart_write` polls TXE before each byte; the CPU stalls until the register is free
- **No interrupt or DMA support**
- **Fixed baud rate** — 115200 hardcoded; requires source change to alter
- **No flow control**
