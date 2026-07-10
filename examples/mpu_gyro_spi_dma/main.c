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
    uint32_t tim0 = 0;
    float roll_angle = 0.0f;
    float pitch_angle = 0.0f;
    float yaw_angle = 0.0f;

    while (1) {
        
        mpu_burst_read(GYRO_XOUT_H, 6, gyro_data_buffer);

        int16_t gyro_x = (int16_t)((uint16_t)gyro_data_buffer[0] << 8 | gyro_data_buffer[1]);
        int16_t gyro_y = (int16_t)((uint16_t)gyro_data_buffer[2] << 8 | gyro_data_buffer[3]);
        int16_t gyro_z = (int16_t)((uint16_t)gyro_data_buffer[4] << 8 | gyro_data_buffer[5]);


        float roll_rate = (float)gyro_x / 131.0f;
        float pitch_rate = (float)gyro_y / 131.0f;
        float yaw_rate = (float)gyro_z / 131.0f;

        uint32_t dt = get_tick() - tim0;
        printf("time: %ld \r\n", (long) dt);

        roll_angle = roll_angle + (roll_rate * (dt / 1000.0f));
        pitch_angle = pitch_angle + (pitch_rate * (dt / 1000.0f));
        yaw_angle = yaw_angle + (yaw_rate * (dt / 1000.0f));
        

        printf("gyro_x : %ld mg  gyro_y : %ld mg  gyro_z : %ld mg\r\n",
               (long)roll_rate, (long)pitch_rate, (long)yaw_rate);

        printf("roll_angle: %ld degrees   pitch angle: %ld degrees\r\n", (long)roll_angle, (long)pitch_angle);
        
        tim0 = get_tick();
        delay(20);
        
    }
}