# STM32F411 Bare-Metal Drivers

Bare-metal peripheral drivers for the STM32F411 Nucleo-64, written in C against the CMSIS device headers with direct register access — no HAL or LL libraries.

## Drivers

### GPIO (`gpio/`)

Controls the Nucleo-64 on-board LED (PA5) and user button (PC13).

| Function | Description |
|---|---|
| `led_init()` | Enables GPIOA clock; configures PA5 as push-pull output |
| `led_on()` | Sets PA5 high via BSRR (atomic) |
| `led_off()` | Clears PA5 via BSRR (atomic) |
| `btn_init()` | Enables GPIOC clock; configures PC13 as input |
| `btn_state()` | Returns `true` when button is pressed (inverts active-low logic) |

> Polling only — no EXTI/interrupt support.

---

### UART (`uart/`)

Transmit-only USART2 driver on PA2 (TX), wired to the Nucleo's ST-LINK virtual COM port.

- **Baud rate:** 115200 @ 16 MHz HSI
- **Alternate function:** AF7

| Function | Description |
|---|---|
| `uart_init()` | Enables clocks; configures PA2 as AF7; sets BRR; enables TE and UE |
| `__io_putchar()` | Newlib retargeting hook — connects `printf` / `putchar` to UART TX |

> TX only — no RX, no interrupts, no DMA.

---

### SPI (`spi/`)

Full-duplex SPI1 master driver with software chip-select on PA4.

| Pin | Signal |
|---|---|
| PA4 | Software CS (active-low) |
| PA5 | SCK |
| PA6 | MISO |
| PA7 | MOSI |

- **Mode:** 0 (CPOL=0, CPHA=0), MSB-first, 8-bit
- **Clock:** fPCLK/32 ≈ 500 kHz at 16 MHz
- **Alternate function:** AF5

| Function | Description |
|---|---|
| `spi_gpio_init()` | Configures PA4 as output (CS) and PA5/6/7 as AF5 |
| `spi_init()` | Enables SPI1 clock; configures CR1; enables SPE |
| `spi_transceive(data)` | Blocking full-duplex transfer; returns received byte |
| `select_slave()` | Asserts CS (PA4 low) |
| `deselect_slave()` | Deasserts CS (PA4 high) |

> Blocking/polling only. Single slave.

---

### SysTick (`systick/`)

Blocking millisecond delay using the Cortex-M4 SysTick timer.

- **Clock source:** Processor clock (16 MHz)
- **Resolution:** 1 ms (LOAD = 15999)

| Function | Description |
|---|---|
| `delay(ms)` | Blocks for the given number of milliseconds; stops counter on exit |

> Polling only — no tick counter or elapsed-time API.

---

### MPU-9250 (`mpu9250/`)

Accelerometer driver for the InvenSense MPU-9250 over SPI. Implements the MPU-9250 SPI protocol: bit 7 of the address byte selects read (1) or write (0); burst reads auto-increment the register address.

**Initialisation settings:**
- Wakes device from sleep (PWR_MGMT_1 = 0x00)
- Enables all accelerometer and gyro axes (PWR_MGMT_2 = 0x00)
- Sets accelerometer full-scale range to ±4 g (8192 LSB/g)

| Function | Description |
|---|---|
| `mpu_init()` | Calls SPI init; wakes device; configures accel range |
| `mpu_read_reg(reg)` | Reads a single register; returns the byte |
| `mpu_write(reg, data)` | Writes a single byte to a register |
| `mpu_burst_read(reg, length, buffer)` | Reads `length` bytes starting at `reg` into `buffer` |

> Gyro axes are enabled but not configured or read. No magnetometer (AK8963) support.

---

## Sensor Example (`Sensor Examples/`)

Demonstrates the full driver stack. Initialises UART and MPU-9250, then continuously burst-reads the 6 accelerometer bytes (X/Y/Z high+low), reconstructs signed 16-bit values, converts to milli-g, and prints over UART via `printf`.

```
acc_x : 12 mg  acc_y : -8 mg  acc_z : 998 mg
```

Live accelerometer output captured in RealTerm at 115200 baud, showing X/Y/Z readings in milli-g alongside the raw SPI bytes:

![Accelerometer Readings](images/Accelerometer%20Readings.png)

---

## Pin Summary

| Pin | Peripheral | Function |
|---|---|---|
| PA2 | USART2 | TX (AF7) |
| PA4 | SPI1 | Software CS |
| PA5 | SPI1 / LED | SCK (AF5) / LD2 output |
| PA6 | SPI1 | MISO (AF5) |
| PA7 | SPI1 | MOSI (AF5) |
| PC13 | — | User button input (active-low) |

> PA5 is shared between the on-board LED and SPI1 SCK. Do not use both simultaneously.

---

## Build

The project uses CMake with an ARM GCC toolchain. A VS Code launch configuration with OpenOCD is included for flashing and debugging.

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<toolchain>.cmake
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
