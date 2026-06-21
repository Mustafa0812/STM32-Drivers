# BMP390 Temperature — I2C

Reads compensated temperature from a BMP390 sensor over I2C1 and prints the result over UART (ST-LINK virtual COM port) at 1-second intervals. Originally built to monitor peak room temperature during summer without a thermostat.

## Wiring

| Nucleo-64 Pin | BMP390 Pin | Notes |
|---|---|---|
| 3.3V | VCC | |
| GND | GND | |
| 3.3V | SDO | Pulls I2C address to 0x77 |
| PB8 | SCL | I2C1 clock (AF4, open-drain) |
| PB9 | SDA | I2C1 data (AF4, open-drain) |

> External pull-up resistors (4.7 kΩ) on SCL and SDA are recommended.

## Build and Flash

```bash
cmake -B build -DEXAMPLE=bmp390_temp_i2c -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
Temp: 28.14 C
Temp: 28.15 C
```

## Initialisation Sequence

1. `uart_init()` — USART2 TX on PA2, 115200 baud
2. `i2c_init()` — I2C1 on PB8/PB9, 100 kHz standard mode
3. `bmp_init()` — configures BMP390: I2C interface, x8 temperature oversampling, 200 Hz ODR, normal mode
4. Trim coefficients T1, T2, T3 read once from registers 0x31–0x35
5. Continuous burst-read of 3 bytes from `BMP390_TEMPDATA` (0x07); datasheet compensation formula applied in floating-point to produce °C

## Temperature Compensation

Raw 24-bit ADC value is compensated using the BMP390 datasheet formula (section 8.6):

```
par_T1 = T1_raw * 256
par_T2 = T2_raw / 2^30
par_T3 = T3_raw / 2^48
diff    = adc_T - par_T1
t_comp  = diff * par_T2 + diff^2 * par_T3
```
