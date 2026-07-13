#include "i2c.h"
#include "mpu9250_i2c.h"
#include "systick.h"

uint8_t x_sense = 0;
uint8_t y_sense = 0;
uint8_t z_sense = 0;

void mpu_burst_read(uint8_t reg, uint8_t *buffer)
{
    i2c_burst_read_reg(DEVICE_ADD, reg, 6, buffer);
}

void mag_burst_read(uint8_t reg, uint8_t *buffer)
{
    i2c_burst_read_reg(MAG_ADDR, reg, 6, buffer);
}

uint8_t mag_read_reg(uint8_t reg)
{
    return i2c_read_reg (MAG_ADDR, reg);
}

void mpu_write(uint8_t reg, uint8_t data)
{
    i2c_write_reg(DEVICE_ADD, reg, data);
}

void mag_write(uint8_t reg, uint8_t data){

    i2c_write_reg(MAG_ADDR, reg, data);
}

void mpu_init(void)
{
    mpu_write(POWER_MANAGEMENT,  0x00);  /* wake from sleep, use internal oscillator */
    delay(6);                            /* 6 ms for oscillator to stabilise */
    mpu_write(POWER_MANAGEMENT2, 0x00);  /* enable all accelerometer and gyro axes */
    mpu_write(ACCEL_CONFIG,      0x08);  /* full-scale range: ±4 g (8192 LSB/g) */
    mpu_write(GYRO_CONFIG,       0x00);  /* full-scale range: ±250 dps (131 LSB/dps) */

    /* I2C pass-through: bridges the AK8963 magnetometer's AUX_DA/AUX_CL
     * pins onto this same main I2C bus, so it becomes directly addressable
     * at MAG_ADDR instead of requiring a separate wired connection.
     * BYPASS_EN only takes effect once I2C_MST_EN is cleared. */
    mpu_write(USER_CTRL,   0x00);  /* clear I2C_MST_EN */
    mpu_write(INT_PIN_CFG, 0x02);  /* set BYPASS_EN */

    /* AK8963 factory sensitivity values are only readable while CNTL1 is
     * in Fuse ROM access mode; must return to power-down before switching
     * to the actual measurement mode. */
    mag_write(CNTL1, 0x0F);        /* Fuse ROM access mode */
    x_sense = mag_read_reg(ASAX);
    y_sense = mag_read_reg(ASAY);
    z_sense = mag_read_reg(ASAZ);
    mag_write(CNTL1, 0x00);        /* power-down (required before mode switch) */
    mag_write(CNTL1, 0x12);        /* continuous measurement mode 1, 16-bit output */
}