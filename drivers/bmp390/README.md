# BMP390 Driver

Device driver for the Bosch BMP390 barometric pressure and temperature sensor over I2C. Provides initialisation, single-byte register read/write, and burst read for the raw ADC data registers.

## Configuration

| Parameter | Value |
|---|---|
| Interface | I2C (via i2c driver) |
| I2C address | 0x77 (SDO pulled high) |
| Mode | Normal (continuous measurement) |
| Temperature oversampling | x8 (OSR_T = 011) |
| ODR | 200 Hz |

## Registers

| Define | Address | Description |
|---|---|---|
| `BMP390_CHIP_ID` | 0x00 | Chip ID (reads 0x60) |
| `BMP390_STATUS` | 0x03 | Sensor status |
| `BMP390_TEMPDATA` | 0x07 | Temperature XLSB (burst read start) |
| `BMP390_IF_CONF` | 0x1A | Interface configuration |
| `BMP390_PWR_CTRL` | 0x1B | Power control and mode |
| `BMP390_OSR` | 0x1C | Oversampling settings |
| `BMP390_ODR` | 0x1D | Output data rate |
| `BMP390_T1_0/1` | 0x31–0x32 | Trim coefficient T1 (LSB, MSB) |
| `BMP390_T2_0/1` | 0x33–0x34 | Trim coefficient T2 (LSB, MSB) |
| `BMP390_T3` | 0x35 | Trim coefficient T3 |

## Functions

| Function | Description |
|---|---|
| `bmp_init()` | Configures interface, oversampling, ODR, and sets normal mode |
| `bmp_read(reg)` | Reads and returns one byte from register `reg` |
| `bmp_write(reg, data)` | Writes one byte `data` to register `reg` |
| `bmp_burst_read(reg, buffer, n)` | Reads `n` bytes starting at `reg` into `buffer` |

## Limitations

- **Temperature only** — pressure enable bit is not set; pressure registers are not read
- **No bus recovery** — if I2C stalls mid-transaction the driver loops forever
- **Fixed address** — hardcoded to 0x77; change `BMP390_ADDR` in the header for 0x76
