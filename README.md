# STM32F411 Bare-Metal Drivers

Bare-metal peripheral drivers for the STM32F411 Nucleo-64, written in C against the CMSIS device headers with direct register access — no HAL or LL libraries.

## Drivers

### GPIO (`drivers/gpio/`)

Controls the Nucleo-64 on-board LED (PA5) and user button (PC13).

| Function | Description |
|---|---|
| `led_init()` | Enables GPIOA clock; configures PA5 as push-pull output |
| `led_on()` | Sets PA5 high via BSRR (atomic) |
| `led_off()` | Clears PA5 via BSRR (atomic) |
| `btn_init()` | Enables GPIOC clock; configures PC13 as input |
| `btn_state()` | Returns `true` when button is pressed (inverts active-low logic) |

---

### EXTI (`drivers/exti/`)

Interrupt-driven GPIO input driver for the Nucleo-64 user button (PC13), using EXTI line 13 with a falling-edge trigger.

A callback function is registered with `exti_register_callback()` before the interrupt is enabled. Any `void fn(void)` function can be passed, so different actions can be triggered by the same button press without modifying the driver — swap the callback to change the behaviour.

| Function | Description |
|---|---|
| `exti_init()` | Enables SYSCFG clock; maps PC13 to EXTI13; configures falling-edge trigger; enables NVIC |
| `exti_register_callback(cb)` | Registers `cb` as the function called on each button press |

> Always call `exti_register_callback()` before `exti_init()` — the interrupt is live as soon as `exti_init()` returns.

---

### UART (`drivers/uart/`)

Interrupt-driven USART2 TX driver on PA2, wired to the Nucleo's ST-LINK virtual COM port. A 256-byte ring buffer is drained by the USART2 TXE interrupt so `printf` returns immediately without stalling the CPU.

- **Baud rate:** 115200 @ 16 MHz HSI
- **Alternate function:** AF7
- **TX buffer:** 256-byte ring buffer, ISR-drained

| Function | Description |
|---|---|
| `uart_init()` | Enables clocks; configures PA2 as AF7; sets BRR; enables TE, UE, and NVIC |
| `__io_putchar()` | Newlib retargeting hook — connects `printf` / `putchar` to UART TX |

> TX only — no RX. Bytes dropped silently if the 256-byte ring buffer is full.

---

### SPI (`drivers/spi/`)

Full-duplex SPI1 master driver. Chip-select is managed by the device driver (PA4 for MPU-9250).

| Pin | Signal |
|---|---|
| PA5 | SCK |
| PA6 | MISO |
| PA7 | MOSI |

- **Mode:** 0 (CPOL=0, CPHA=0), MSB-first, 8-bit
- **Clock:** fPCLK/32 ≈ 500 kHz at 16 MHz
- **Alternate function:** AF5

| Function | Description |
|---|---|
| `spi_gpio_init()` | Configures PA5/6/7 as AF5 |
| `spi_init()` | Enables SPI1 clock; configures CR1; enables SPE |
| `spi_transceive(data)` | Blocking full-duplex transfer; returns received byte |

> Blocking/polling only — interrupt-driven support planned. CS is the responsibility of the device driver.

---

### I2C (`drivers/i2c/`)

I2C1 master driver on PB8 (SCL) and PB9 (SDA), configured for standard mode (100 kHz).

| Pin | Signal |
|---|---|
| PB8 | SCL |
| PB9 | SDA |

- **Mode:** Standard mode, 100 kHz
- **Alternate function:** AF4
- **Pull-ups:** Internal pull-ups enabled (open-drain output type)

| Function | Description |
|---|---|
| `i2c_init()` | Configures PB8/9 as AF4 open-drain; enables I2C1 clock; sets CCR and TRISE |
| `burst_read_reg(slave, target, n, buffer)` | Reads `n` bytes from register `target` on device `slave` into `buffer` |
| `write_reg(slave, target, data)` | Writes one byte `data` to register `target` on device `slave` |

> Polling only — interrupt-driven support planned. Addresses are 7-bit (not pre-shifted).

---

### SysTick (`drivers/systick/`)

Blocking millisecond delay using the Cortex-M4 SysTick timer.

- **Clock source:** Processor clock (16 MHz)
- **Resolution:** 1 ms (LOAD = 15999)

| Function | Description |
|---|---|
| `delay(ms)` | Blocks for the given number of milliseconds; stops counter on exit |

> Polling only — no tick counter or elapsed-time API.

---

### MPU-9250 (`drivers/mpu9250/`)

Accelerometer driver for the InvenSense MPU-9250. Two transport implementations are provided — SPI and I2C — sharing the same function interface.

**Initialisation settings (both transports):**
- Wakes device from sleep (PWR_MGMT_1 = 0x00)
- Enables all accelerometer and gyro axes (PWR_MGMT_2 = 0x00)
- Sets accelerometer full-scale range to ±4 g (8192 LSB/g)

#### SPI transport (`mpu9250_spi.c / mpu9250_spi.h`)

Implements the MPU-9250 SPI protocol: bit 7 of the address byte selects read (1) or write (0); burst reads auto-increment the register address. CS is managed on PA4.

| Function | Description |
|---|---|
| `mpu_init()` | Configures PA4 as CS; wakes device; sets accel range |
| `mpu_read_reg(reg)` | Single register read; returns the byte |
| `mpu_write(reg, data)` | Writes one byte to a register |
| `mpu_burst_read(reg, length, buffer)` | Reads `length` bytes starting at `reg` into `buffer` |

#### I2C transport (`mpu9250_i2c.c / mpu9250_i2c.h`)

Uses the I2C1 bus driver. Device address: 0x68 (AD0 pin low).

| Function | Description |
|---|---|
| `mpu_init()` | Wakes device; sets accel range (`i2c_init()` called separately in main) |
| `mpu_write(reg, data)` | Writes one byte to a register |
| `mpu_burst_read(reg, buffer)` | Reads 6 bytes starting at `reg` into `buffer` |

> Gyro axes are enabled but not read. No magnetometer (AK8963) support.

---

## Examples (`examples/`)

Each example demonstrates the full driver stack for a specific transport. Select which example to build with the `EXAMPLE` CMake variable (default: `mpu9250_accel_i2c`).

### led

Demonstrates EXTI interrupt-driven button input. Pressing the user button (PC13) triggers a falling-edge interrupt which sets a flag; the `while(1)` loop detects the flag, turns on LD2 (PA5) for one second, then turns it off. The ISR itself returns immediately — all blocking work stays in main context.

Because the driver uses a registered callback, the same button press can drive entirely different behaviour by passing a different function to `exti_register_callback()` — no changes to the driver required.

### mpu9250_accel_i2c

Initialises UART, I2C, and MPU-9250 (I2C transport), then continuously burst-reads the 6 accelerometer bytes (X/Y/Z high+low), reconstructs signed 16-bit values, converts to milli-g, and prints over UART at 500 ms intervals.

```
acc_x : -193 mg  acc_y : -760 mg  acc_z : 355 mg
```

![I2C Accelerometer Readings](images/I2C%20READINGS.png)

### mpu9250_accel_spi

Same accelerometer readout over SPI. Also prints the WHO_AM_I register (expect 0x71) on startup.

```
WHO_AM_I: 0x71 (expect 0x71)
acc_x : 12 mg  acc_y : -8 mg  acc_z : 998 mg
```

![SPI Accelerometer Readings](images/Accelerometer%20Readings.png)

---

## Pin Summary

| Pin | Peripheral | Function |
|---|---|---|
| PA2 | USART2 | TX (AF7) |
| PA4 | SPI1 / MPU-9250 | Software CS (active-low) |
| PA5 | SPI1 / LED | SCK (AF5) / LD2 output |
| PA6 | SPI1 | MISO (AF5) |
| PA7 | SPI1 | MOSI (AF5) |
| PB8 | I2C1 | SCL (AF4) |
| PB9 | I2C1 | SDA (AF4) |
| PC13 | — | User button input (active-low) |

> PA5 is shared between the on-board LED and SPI1 SCK. Do not use both simultaneously.

---

## Build

The project uses CMake with an ARM GCC toolchain. A VS Code launch configuration with OpenOCD is included for flashing and debugging.

```bash
# Build the LED/EXTI example
cmake -B build -DEXAMPLE=led -G "MinGW Makefiles"
cmake --build build

# Build the default example (mpu9250_accel_i2c)
cmake -B build -G "MinGW Makefiles"
cmake --build build

# Build the SPI example instead
cmake -B build -DEXAMPLE=mpu9250_accel_spi -G "MinGW Makefiles"
cmake --build build
```

Flash via OpenOCD or the VS Code **Run and Debug** panel.

---

## Target

| | |
|---|---|
| MCU | STM32F411RE |
| Board | Nucleo-64 |
| Core | Cortex-M4 |
| Clock | 16 MHz HSI (no PLL configured) |
| Headers | CMSIS STM32F4xx |
