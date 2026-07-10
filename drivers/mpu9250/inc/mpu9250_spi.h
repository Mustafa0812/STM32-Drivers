#ifndef MPU_9250_
#define MPU_9250_

#include <stdint.h>

/* MPU-9250 register addresses (PS-MPU-9250A-01 §4) */
#define POWER_MANAGEMENT   0x6B
#define POWER_MANAGEMENT2  0x6C
#define ACCEL_CONFIG       0x1C
#define GYRO_CONFIG       0x1B
#define ACCEL_CONFIG2      0x1D
#define ACCEL_XOUT_H       0x3B
#define GYRO_XOUT_H        0x43
#define DEVICEID           0x75
#define DEVICE_ADD         0b1101000

uint8_t mpu_read_reg(uint8_t reg);
void    mpu_write(uint8_t reg, uint8_t data);
void    mpu_init(void);
void    mpu_burst_read(uint8_t reg, uint8_t length, uint8_t *buffer);

#endif
