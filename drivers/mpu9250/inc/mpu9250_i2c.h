#ifndef MPU_9250_I2C_H
#define MPU_9250_I2C_H

#include <stdint.h>

/* MPU-9250 register addresses (PS-MPU-9250A-01 §4) */
#define POWER_MANAGEMENT    0x6B
#define POWER_MANAGEMENT2   0x6C
#define ACCEL_CONFIG        0x1C
#define ACCEL_CONFIG2       0x1D
#define ACCEL_XOUT_H        0x3B
#define GYRO_CONFIG         0x1B
#define GYRO_XOUT_H         0x43
#define MAGNETOMETER_XOUT_L 0x03
#define DEVICEID            0x75
#define USER_CTRL           0x6A   /* I2C_MST_EN: bit 5 */
#define INT_PIN_CFG         0x37   /* BYPASS_EN: bit 1 */

/* AK8963 magnetometer registers (PS-MPU-9250A-01 §9.1, reached only via
 * I2C pass-through — see mpu_init()) */
#define CNTL1               0x0A
#define ST2                 0x09
#define ASAX                0x10
#define ASAY                0x11
#define ASAZ                0x12


/* I2C address: AD0 pin low → 0x68 (PS-MPU-9250A-01 §9.2) */
#define DEVICE_ADD         0b1101000
/* AK8963 I2C address, fixed regardless of AD0 (PS-MPU-9250A-01 §9.1) */
#define MAG_ADDR           0x0C


/* Factory sensitivity-adjustment values (AK8963 fuse ROM), read once in
 * mpu_init() — see the Hadj formula in mpu9250_accel_i2c's README. */
extern uint8_t x_sense;
extern uint8_t y_sense;
extern uint8_t z_sense;


uint8_t mpu_read_reg(uint8_t reg);
void    mpu_write(uint8_t reg, uint8_t data);
void    mpu_init(void);
void    mpu_burst_read(uint8_t reg, uint8_t *buffer);

/* AK8963 magnetometer access, via I2C pass-through (talks to MAG_ADDR
 * directly — separate from the MPU-9250's own mpu_* functions above) */
void    mag_burst_read(uint8_t reg, uint8_t *buffer);
uint8_t mag_read_reg(uint8_t reg);
void    mag_write(uint8_t reg, uint8_t data);

#endif
