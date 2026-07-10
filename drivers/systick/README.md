# SysTick Driver

Free-running millisecond tick counter and blocking delay, both built on the Cortex-M4 SysTick timer. `start_timer()` configures and starts SysTick once; `delay()` and `get_tick()` both read the resulting tick count without touching SysTick's configuration again.

## Configuration

| Parameter | Value |
|---|---|
| Clock source | Processor clock (16 MHz HSI) |
| Resolution | 1 ms |
| LOAD value | 15999 (16 000 ticks − 1) |
| Interrupt | `SysTick_Handler`, increments the tick counter every 1 ms |

## Functions

| Function | Description |
|---|---|
| `start_timer()` | Configures SysTick for a 1 ms period with `TICKINT` enabled and starts the counter. Call exactly once, before any `delay()` or `get_tick()` use. |
| `delay(ms)` | Blocks by polling `get_tick()` until `ms` milliseconds have elapsed. Does not reconfigure or stop SysTick — safe to call repeatedly without disturbing the running tick count. |
| `get_tick()` | Returns the free-running millisecond count since `start_timer()` was called. |

## Usage

```c
#include "systick.h"

int main(void)
{
    start_timer();   /* once, before any delay()/get_tick() use */

    uint32_t t0 = get_tick();
    /* ... work ... */
    uint32_t elapsed_ms = get_tick() - t0;

    delay(500);       /* block for 500 ms */
}
```

## Limitations

- **`start_timer()` must be called first** — `delay()` and `get_tick()` both assume SysTick is already running; calling them beforehand returns/waits on a tick count that never advances.
- **Single owner of SysTick** — this driver assumes exclusive use of the SysTick peripheral. It must not be used alongside another SysTick consumer (e.g. an RTOS tick) without coordinating configuration; see the FreeRTOS section in the top-level README.
- **`delay()` still blocks the CPU** — no other work can run during the wait; it is not interrupt-driven from the caller's perspective.
