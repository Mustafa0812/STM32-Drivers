# MPU-9250 Accelerometer — I2C

Reads X, Y, Z accelerometer axes from an MPU-9250 over I2C1 and prints the results in milli-g over UART (ST-LINK virtual COM port) at 500 ms intervals.

## Wiring

| Nucleo-64 Pin | MPU-9250 Pin | Notes |
|---|---|---|
| 3.3V | VCC | |
| GND | GND | |
| GND | AD0 | Pulls I2C address to 0x68 |
| PB8 | SCL | I2C1 clock (AF4, open-drain) |
| PB9 | SDA | I2C1 data (AF4, open-drain) |

> External pull-up resistors (4.7 kΩ) on SCL and SDA are recommended. The driver enables internal pull-ups but these may be insufficient at full 100 kHz for long wires.

## Build and Flash

```bash
cmake -B build -DEXAMPLE=mpu9250_accel_i2c -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
raw: 3C FF 0A 00 3F 00
acc_x : -193 mg  acc_y : -760 mg  acc_z : 355 mg
```

The raw line shows the six bytes as read from the MPU-9250 registers before reconstruction. The `acc_` line shows the signed 16-bit values converted to milli-g (±4 g range, 8192 LSB/g).

## Initialisation Sequence

1. `uart_init()` — USART2 TX on PA2, 115200 baud
2. `i2c_init()` — I2C1 on PB8/PB9, 100 kHz standard mode
3. `mpu_init()` — wakes MPU-9250; sets ±4 g range; 6 ms stabilisation delay
4. Continuous burst-read of 6 bytes from `ACCEL_XOUT_H` (0x3B)
