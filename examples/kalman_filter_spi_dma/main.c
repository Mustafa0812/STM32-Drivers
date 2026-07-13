#include "uart.h"
#include "spi.h"
#include "dma_spi.h"
#include "mpu9250_spi.h"
#include <stdio.h>
#include "systick.h"
#include "math.h"

int main(void)
{
    start_timer();
    uart_init();
    spi_gpio_init();
    spi_init();
    dma2_init();    /* must come after spi_init — SPI1 clock must be on before CR2 is touched */
    mpu_init();

    uint8_t who_am_i = mpu_read_reg(DEVICEID);
    printf("WHO_AM_I: 0x%02X (expect 0x71)\r\n", who_am_i);

    uint8_t gyro_data_buffer[6];
    uint8_t accel_data_buffer[6];

    uint32_t tim0 = 0;   /* timestamp of the last reading, for dt */

    /* Filter state, per axis — angle estimate and the gyro's constant bias,
     * both actively corrected every update step (unlike the complementary
     * filter, which only ever integrates the raw, still-biased rate). */
    float roll_angle = 0.0f;
    float roll_bias  = 0.1f;

    float pitch_angle = 0.0f;
    float pitch_bias  = 0.0f;

    /* 2x2 uncertainty (covariance) matrix, per axis — P00: uncertainty in
     * angle, P11: uncertainty in bias, P01/P10: their correlation. All
     * start at 0 (maximum confidence); the first predict step grows them. */
    float roll_P00 = 2.0f;
    float roll_P01 = 0.0f;
    float roll_P10 = 0.0f;
    float roll_P11 = 0.0f;

    float pitch_P00 = 2.0f;
    float pitch_P01 = 0.0f;
    float pitch_P10 = 0.0f;
    float pitch_P11 = 0.0f;

    /* Tuning constants, shared across both axes (same physical sensor).
     * Q: how much to distrust the prediction model (gyro integration).
     * R: how much to distrust the accelerometer measurement. */
    float Q_angle    = 1.80f;
    float Q_bias     = 1.0f;
    float R_measure  = 0.45f;

    while (1) {
        mpu_burst_read(GYRO_XOUT_H, 6, gyro_data_buffer);
        mpu_burst_read(ACCEL_XOUT_H, 6, accel_data_buffer);

        int16_t gyro_x = (int16_t)((uint16_t)gyro_data_buffer[0] << 8 | gyro_data_buffer[1]);
        int16_t gyro_y = (int16_t)((uint16_t)gyro_data_buffer[2] << 8 | gyro_data_buffer[3]);

        int16_t acc_x = (int16_t)((uint16_t)accel_data_buffer[0] << 8 | accel_data_buffer[1]);
        int16_t acc_y = (int16_t)((uint16_t)accel_data_buffer[2] << 8 | accel_data_buffer[3]);
        int16_t acc_z = (int16_t)((uint16_t)accel_data_buffer[4] << 8 | accel_data_buffer[5]);

        /* ±250 dps range → 131 LSB per deg/s */
        float roll_rate  = (float)gyro_x / 131.0f;
        float pitch_rate = (float)gyro_y / 131.0f;

        /* ±4 g range → 8192 LSB per g; float divisor forces float division */
        float acc_xg = acc_x / 8192.0f;
        float acc_yg = acc_y / 8192.0f;
        float acc_zg = acc_z / 8192.0f;

        /* Captured right after the reading (not after delay()), so the
         * delay is correctly counted in the *next* iteration's dt. */
        float dt = (get_tick() - tim0) / 1000.0f;
        tim0 = get_tick();


        /* Tilt angle from gravity's projection onto each axis — accurate
         * on average, but noisy under any non-gravity (lateral) acceleration.
         * This is the measurement each axis's update step corrects toward. */
        float roll_denom  = sqrtf((acc_xg * acc_xg) + (acc_zg * acc_zg));
        float pitch_denom = sqrtf((acc_yg * acc_yg) + (acc_zg * acc_zg));

        float accel_roll_angle  = atan2f(acc_yg, roll_denom)        * (180.0f / 3.142f);
        float accel_pitch_angle = atan2f(-1 * acc_xg, pitch_denom)  * (180.0f / 3.142f);

        /* ---- Predict step (roll): integrate the bias-corrected rate,
         * then grow the uncertainty matrix since time has passed with no
         * new measurement yet. */
        float corrected_roll = roll_rate - roll_bias;
        roll_angle = roll_angle + (corrected_roll * dt);

        roll_P00 += dt * ((dt * roll_P11) - roll_P01 - roll_P10 + Q_angle);
        roll_P01 -= dt * roll_P11;
        roll_P10 -= dt * roll_P11;
        roll_P11 += Q_bias * dt;

        /* ---- Predict step (pitch): mirrors roll above. */
        float corrected_pitch = pitch_rate - pitch_bias;
        pitch_angle = pitch_angle + (corrected_pitch * dt);

        pitch_P00 += dt * ((dt * pitch_P11) - pitch_P01 - pitch_P10 + Q_angle);
        pitch_P01 -= dt * pitch_P11;
        pitch_P10 -= dt * pitch_P11;
        pitch_P11 += Q_bias * dt;

        /* ---- Update step (roll): blend in the accelerometer measurement,
         * weighted by the Kalman gain (how much to trust it right now). */
        float S_roll  = roll_P00 + R_measure;
        float K0_roll = roll_P00 / S_roll;
        float K1_roll = roll_P10 / S_roll;


        float roll_innovation = accel_roll_angle - roll_angle;
        roll_angle += K0_roll * roll_innovation;
        roll_bias  += K1_roll * roll_innovation;

        /* P10/P11 must use P00/P01's pre-update values — compute them
         * before P00/P01 get overwritten below. */
        roll_P10 -= K1_roll * roll_P00;
        roll_P11 -= K1_roll * roll_P01;
        roll_P00 -= K0_roll * roll_P00;
        roll_P01 -= K0_roll * roll_P01;

        /* ---- Update step (pitch): mirrors roll above. */
        float S_pitch  = pitch_P00 + R_measure;
        float K0_pitch = pitch_P00 / S_pitch;
        float K1_pitch = pitch_P10 / S_pitch;

        float pitch_innovation = accel_pitch_angle - pitch_angle;
        pitch_angle += K0_pitch * pitch_innovation;
        pitch_bias  += K1_pitch * pitch_innovation;

        pitch_P10 -= K1_pitch * pitch_P00;
        pitch_P11 -= K1_pitch * pitch_P01;
        pitch_P00 -= K0_pitch * pitch_P00;
        pitch_P01 -= K0_pitch * pitch_P01;

        printf("roll_angle: %ld degrees   pitch angle: %ld degrees\r\n", (long)roll_angle, (long)pitch_angle);

        delay(20);
    }
}
