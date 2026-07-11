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

    uint32_t tim0 = 0;

    float alpha = 0.98f;

    float roll_angle = 0.0f;
    float pitch_angle = 0.0f;

    while (1) {


        mpu_burst_read(GYRO_XOUT_H, 6, gyro_data_buffer);
        mpu_burst_read(ACCEL_XOUT_H, 6, accel_data_buffer);

        int16_t gyro_x = (int16_t)((uint16_t)gyro_data_buffer[0] << 8 | gyro_data_buffer[1]);
        int16_t gyro_y = (int16_t)((uint16_t)gyro_data_buffer[2] << 8 | gyro_data_buffer[3]);

        int16_t acc_x = (int16_t)((uint16_t)accel_data_buffer[0] << 8 | accel_data_buffer[1]);
        int16_t acc_y = (int16_t)((uint16_t)accel_data_buffer[2] << 8 | accel_data_buffer[3]);
        int16_t acc_z = (int16_t)((uint16_t)accel_data_buffer[4] << 8 | accel_data_buffer[5]);


        float roll_rate = (float)gyro_x / 131.0f;
        float pitch_rate = (float)gyro_y / 131.0f;

        float dt = (get_tick() - tim0) / 1000.0f;
        tim0 = get_tick();

        float acc_xg = acc_x / 8192.0f;
        float acc_yg = acc_y / 8192.0f;
        float acc_zg = acc_z / 8192.0f;
        

        float roll_denom = sqrtf((acc_xg * acc_xg) + (acc_zg * acc_zg));
        float pitch_denom = sqrtf((acc_yg * acc_yg) + (acc_zg * acc_zg));

        float accel_roll_angle = (atan2f(acc_yg, roll_denom)) * (180.0f / 3.142f);
        float accel_pitch_angle = atan2f(-1 * acc_xg, pitch_denom) * (180.0f / 3.142f);

        roll_angle = alpha * (roll_angle + (roll_rate * dt)) + ((1 - alpha) * accel_roll_angle);                
        pitch_angle = alpha * (pitch_angle + (pitch_rate * dt)) + ((1 - alpha) * accel_pitch_angle);

        printf("roll_angle: %ld degrees   pitch angle: %ld degrees\r\n", (long)roll_angle, (long)pitch_angle);
        
        delay(1 );

    }
}
