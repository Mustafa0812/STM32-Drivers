# SysTick Driver

Blocking millisecond delay using the Cortex-M4 SysTick timer. Each call configures and starts the timer, polls the COUNTFLAG for each millisecond tick, then stops the counter on exit.

## Configuration

| Parameter | Value |
|---|---|
| Clock source | Processor clock (16 MHz HSI) |
| Resolution | 1 ms |
| LOAD value | 15999 (16 000 ticks − 1) |

## Functions

| Function | Description |
|---|---|
| `delay(ms)` | Blocks for `ms` milliseconds; stops the SysTick counter on return |

## Usage

```c
#include "systick.h"

delay(500);    /* block for 500 ms */
```

No initialisation call needed — `delay()` configures SysTick on every call.

## Limitations

- **Blocking** — the CPU stalls for the full duration; no other work can run during the delay
- **No tick counter** — there is no `get_tick()` or elapsed-time API; you cannot measure time between events
- **Reconfigures SysTick on every call** — if SysTick is used elsewhere (e.g. an RTOS tick), this driver will conflict
- **Do not call from an ISR** — blocking inside an interrupt handler stalls the system for the delay duration
