# UART Driver

Interrupt-driven USART2 TX driver on PA2, connected to the Nucleo-64's ST-LINK virtual COM port. A 256-byte ring buffer is drained by the USART2 TXE interrupt so `printf` and `putchar` return immediately without stalling the CPU for each byte. The Newlib `__io_putchar` hook is retargeted so no further configuration is needed.

## Configuration

| Parameter | Value |
|---|---|
| Peripheral | USART2 |
| TX pin | PA2 (AF7) |
| Baud rate | 115200 |
| Frame | 8N1 |
| Clock | 16 MHz HSI |
| TX buffer | 256 bytes (ring buffer) |
| ISR | USART2_IRQHandler (TXE interrupt) |

## Functions

| Function | Description |
|---|---|
| `uart_init()` | Configures PA2 as AF7; enables USART2 clock; sets BRR; enables TE and UE; unmasks USART2 in NVIC |
| `__io_putchar(ch)` | Newlib retargeting hook — called internally by `printf` / `putchar` |

## Usage

```c
#include "uart.h"
#include <stdio.h>

uart_init();
printf("Hello\r\n");
```

No further setup needed — `printf` routes through `__io_putchar` automatically once `uart_init()` has been called.

## How it works

```
printf("Hello\r\n")
  └─ syscalls.c _write()
       └─ __io_putchar(ch)          [called per character]
            └─ uart_write(ch)       [places byte in ring buffer, enables TXEIE]
                                         │
                                   USART2_IRQHandler   [fires on TXE]
                                         └─ DR = ring_buffer[read_index]
                                            read_index++
                                            if empty: disable TXEIE
```

`uart_write` and the ISR share `write_index` / `read_index`. To prevent the ISR from reading a new byte before `write_index` is committed, TXEIE is briefly cleared around the index update (see *Race condition* below).

## Race condition and fix

**Without the fix** — if TXEIE was already set from a previous byte and the ISR fired between the `ring_buffer[write_index] = ch` store and the `write_index++` commit, the ISR would see `read_index == write_index` (old), send the new byte early, then increment `read_index` past `write_index`. The "buffer empty" check would never trigger, leaving TXEIE permanently enabled. The ISR would drain uninitialised slots (null bytes), corrupting all bytes after the first.

**Fix applied in `uart_write`:**

```c
ring_buffer[write_index] = ch;

USART2->CR1 &= ~USART_CR1_TXEIE;   // close the window
write_index  = next_index;          // commit index
USART2->CR1 |=  USART_CR1_TXEIE;   // re-open
```

**Fix applied in `USART2_IRQHandler`:**

```c
if (read_index == write_index) {    // nothing queued?
    USART2->CR1 &= ~USART_CR1_TXEIE;
    return;                         // stop — don't send uninitialised data
}
```

## Limitations

- **TX only** — no RX pin configured; cannot receive data
- **Non-blocking** — CPU queues bytes into the ring buffer and returns; the ISR handles transmission
- **Fixed baud rate** — 115200 hardcoded; requires a source change to alter
- **Buffer full drops** — if 255 bytes are already pending, additional bytes are silently discarded
- **No flow control**
