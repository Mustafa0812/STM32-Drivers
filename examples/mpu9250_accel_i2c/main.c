#include "uart.h"
#include "i2c.h"
#include "mpu9250_i2c.h"
#include "systick.h"
#include <stdio.h>
#include "math.h"

int main(void)
{
    start_timer();
    uart_init();
    i2c_init();
    mpu_init();

    uint8_t accel_data_buffer[6];
    uint8_t mag_data_buffer[6];

    while (1) {
        mpu_burst_read(ACCEL_XOUT_H, accel_data_buffer);

        /* AK8963 registers are little-endian (HXL then HXH), opposite of
         * the MPU-9250's own big-endian accel/gyro registers — low byte
         * first, so the shift is reversed compared to acc_x/y/z below. */
        mag_burst_read(MAGNETOMETER_XOUT_L, mag_data_buffer);
        mag_read_reg(ST2);   /* must be read to release the next measurement */

        int16_t mag_x = (int16_t)((uint16_t)mag_data_buffer[1] << 8 | mag_data_buffer[0]);
        int16_t mag_y = (int16_t)((uint16_t)mag_data_buffer[3] << 8 | mag_data_buffer[2]);

        int16_t acc_x = (int16_t)((uint16_t)accel_data_buffer[0] << 8 | accel_data_buffer[1]);
        int16_t acc_y = (int16_t)((uint16_t)accel_data_buffer[2] << 8 | accel_data_buffer[3]);
        int16_t acc_z = (int16_t)((uint16_t)accel_data_buffer[4] << 8 | accel_data_buffer[5]);

        /* Factory sensitivity adjustment (PS-MPU-9250A-01 §5.13):
         * Hadj = H * ((ASA - 128) * 0.5 / 128 + 1) */
        float x_mag = mag_x * (((((x_sense-128) * 0.5)/128))+1);
        float y_mag = mag_y * (((((y_sense-128) * 0.5)/128))+1);

        /* AK8963's axes are rotated relative to the accel/gyro die inside
         * the same package (PS-MPU-9250A-01 Fig. 4 vs Fig. 5): its X/Y are
         * swapped relative to the accel/gyro body frame. Heading only
         * needs the two horizontal-plane axes — no Z, no tilt formula. */
        float mag_body_x = y_mag;
        float mag_body_y = x_mag;

        float heading = atan2f(mag_body_y, mag_body_x) * (180.0f / 3.142f);

        /* ±4 g range → sensitivity = 8192 LSB/g; convert to milli-g for printing */
        float acc_xg = acc_x / 8192.0f;
        float acc_yg = acc_y / 8192.0f;
        float acc_zg = acc_z / 8192.0f;

        float roll_denom  = sqrtf((acc_xg * acc_xg) + (acc_zg * acc_zg));
        float pitch_denom = sqrtf((acc_yg * acc_yg) + (acc_zg * acc_zg));

        float accel_roll_angle  = atan2f(acc_yg, roll_denom)        * (180.0f / 3.142f);
        float accel_pitch_angle = atan2f(-1 * acc_xg, pitch_denom)  * (180.0f / 3.142f);

        printf("roll_angle: %ld degrees   pitch angle: %ld degrees\r\n", (long)accel_roll_angle, (long)accel_pitch_angle);
        printf("heading: %ld degrees \r\n", (long)heading);

        delay(500);
    }
}
