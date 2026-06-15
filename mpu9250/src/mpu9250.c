#include "spi.h"
#include "mpu9250.h"
#include "systick.h"

/* MPU-9250 SPI protocol (PS-MPU-9250A-01 §3.3):
 * First byte = R/W bit (bit 7) | register address (bits 6:0)
 * 1 = read, 0 = write. Burst reads auto-increment the register address. */

void mpu_burst_read(uint8_t reg, uint8_t length, uint8_t *buffer)
{
    select_slave();
    spi_transceive(reg | 0x80);            /* set bit 7: read + starting register */
    while (length > 0) {
        *buffer++ = spi_transceive(0xFF);  /* clock out dummy bytes to shift in data */
        length--;
    }
    deselect_slave();
}

uint8_t mpu_read_reg(uint8_t reg)
{
    select_slave();
    spi_transceive(reg | 0x80);            /* read command */
    uint8_t result = spi_transceive(0xFF);
    deselect_slave();
    return result;
}

void mpu_write(uint8_t reg, uint8_t data)
{
    select_slave();
    spi_transceive(reg & 0x7F);   /* clear bit 7: write command */
    spi_transceive(data);
    deselect_slave();
}

void mpu_init(void)
{
    spi_gpio_init();
    spi_init();

    mpu_write(POWER_MANAGEMENT, 0x00);   /* wake from sleep, use internal oscillator */
    delay(6);                            /* allow 6 ms for oscillator to stabilise */
    mpu_write(POWER_MANAGEMENT2, 0x00);          /* enable all accelerometer and gyro axes */
    mpu_write(ACCEL_CONFIG, 0x08);               /* full-scale range: ±4 g (8192 LSB/g) */
}
