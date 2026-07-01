# GPIO Driver

Controls the Nucleo-64 on-board LED (LD2, PA5) and user button (PC13). Split across two source files:

- `gpio.c` — polling functions (LED control, button state)
- `gpio_it.c` — interrupt-driven input via EXTI line 13

All declarations are in `gpio.h`.

---

## Polling (`gpio.c`)

| Function | Description |
|---|---|
| `led_init()` | Enables GPIOA clock; configures PA5 as push-pull output |
| `led_on()` | Sets PA5 high via BSRR (atomic set) |
| `led_off()` | Clears PA5 via BSRR (atomic reset) |
| `btn_init()` | Enables GPIOC clock; configures PC13 as floating input |
| `btn_state()` | Returns `true` when button is pressed; handles active-low logic internally |

### Usage

```c
#include "gpio.h"

led_init();
btn_init();

if (btn_state()) {
    led_on();
}
```

---

## Interrupt (`gpio_it.c`)

Configures EXTI line 13 (PC13) with a falling-edge trigger. A callback function is registered with `exti_register_callback()` — any `void fn(void)` function can be passed, so different actions can be triggered by the same button press at runtime without modifying the driver.

| Function | Description |
|---|---|
| `exti_register_callback(cb)` | Registers `cb` as the function called on each button press |
| `exti_init()` | Enables SYSCFG clock; maps PC13 to EXTI13; sets falling-edge trigger; enables NVIC |

### Usage

```c
#include "gpio.h"
#include <stdint.h>

static volatile uint8_t flag = 0;

void on_press(void) {
    flag = 1;              /* set flag only — keep ISR short */
}

int main(void) {
    led_init();
    btn_init();
    exti_register_callback(on_press);  /* register before enabling interrupt */
    exti_init();                       /* interrupt goes live here */

    while (1) {
        if (flag) {
            flag = 0;
            led_on();
            /* do work in main context */
        }
    }
}
```

> Always call `exti_register_callback()` before `exti_init()`. The interrupt is live as soon as `exti_init()` returns; registering after that leaves a window where the ISR fires with no callback.

> `btn_init()` must be called before `exti_init()` to configure PC13 as an input.

---

## Limitations

- **PA5 shared with SPI1 SCK** — do not use `led_on()`/`led_off()` while SPI1 is active
- **Polling has no debounce** — `btn_state()` reads the raw IDR
- **Interrupt hardcoded to EXTI13 / PC13** — not configurable without modifying `gpio_it.c`
- **Single callback** — only one function can be registered at a time
- **No debounce on interrupt** — mechanical bounce can fire the ISR multiple times per press
- **No NVIC priority configuration**
