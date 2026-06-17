#include "i2c.h"
#include "mpu9250_i2c.h"
#include "systick.h"

void mpu_burst_read(uint8_t reg, uint8_t *buffer)
{
    burst_read_reg(DEVICE_ADD, reg, 6, buffer);
}

uint8_t mpu_read_reg(uint8_t reg)
{
    /* not implemented */
}

void mpu_write(uint8_t reg, uint8_t data)
{
    write_reg(DEVICE_ADD, reg, data);
}

void mpu_init(void)
{
    mpu_write(POWER_MANAGEMENT,  0x00);  /* wake from sleep, use internal oscillator */
    delay(6);                            /* 6 ms for oscillator to stabilise */
    mpu_write(POWER_MANAGEMENT2, 0x00);  /* enable all accelerometer and gyro axes */
    mpu_write(ACCEL_CONFIG,      0x08);  /* full-scale range: ±4 g (8192 LSB/g) */
}
