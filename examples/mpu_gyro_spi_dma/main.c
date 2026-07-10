#include "uart.h"
#include "spi.h"
#include "dma_spi.h"
#include "mpu9250_spi.h"
#include <stdio.h>
#include "systick.h"
#include "math.h"

int main(void)
{
    uart_init();
    spi_gpio_init();
    spi_init();
    dma2_init();    /* must come after spi_init — SPI1 clock must be on before CR2 is touched */
    mpu_init();

    uint8_t who_am_i = mpu_read_reg(DEVICEID);
    printf("WHO_AM_I: 0x%02X (expect 0x71)\r\n", who_am_i);

    uint8_t gyro_data_buffer[6];

    while (1) {
        mpu_burst_read(GYRO_XOUT_H, 6, gyro_data_buffer);

        int16_t gyro_x = (int16_t)((uint16_t)gyro_data_buffer[0] << 8 | gyro_data_buffer[1]);
        int16_t gyro_y = (int16_t)((uint16_t)gyro_data_buffer[2] << 8 | gyro_data_buffer[3]);
        int16_t gyro_z = (int16_t)((uint16_t)gyro_data_buffer[4] << 8 | gyro_data_buffer[5]);


        int32_t roll_rate = (int32_t)gyro_x / 131.0f;
        int32_t pitch_rate = (int32_t)gyro_y / 131.0f;
        int32_t yaw_rate = (int32_t)gyro_z / 131.0f;
        

        printf("gyro_x : %ld mg  gyro_y : %ld mg  gyro_z : %ld mg\r\n",
               (long)roll_rate, (long)pitch_rate, (long)yaw_rate);

        printf("roll_angle: %ld degrees   pitch angle: %ld degrees\r\n", (long)roll_angle, (long)pitch_angle);
        

        delay(3000);
    }
}