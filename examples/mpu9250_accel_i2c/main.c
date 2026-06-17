#include "uart.h"
#include "i2c.h"
#include "mpu9250_i2c.h"
#include "systick.h"
#include <stdio.h>

int main(void)
{
    uart_init();
    i2c_init();
    mpu_init();

    uint8_t data_buffer[6];

    while (1) {
        /* Burst-read 6 bytes from ACCEL_XOUT_H: X-high, X-low, Y-high, Y-low, Z-high, Z-low */
        mpu_burst_read(ACCEL_XOUT_H, data_buffer);

        printf("raw: %02X %02X %02X %02X %02X %02X\r\n",
               data_buffer[0], data_buffer[1], data_buffer[2],
               data_buffer[3], data_buffer[4], data_buffer[5]);

        int16_t acc_x = (int16_t)((uint16_t)data_buffer[0] << 8 | data_buffer[1]);
        int16_t acc_y = (int16_t)((uint16_t)data_buffer[2] << 8 | data_buffer[3]);
        int16_t acc_z = (int16_t)((uint16_t)data_buffer[4] << 8 | data_buffer[5]);

        /* ±4 g range → sensitivity = 8192 LSB/g; convert to milli-g for printing */
        int32_t acc_xmg = (int32_t)acc_x * 1000 / 8192;
        int32_t acc_ymg = (int32_t)acc_y * 1000 / 8192;
        int32_t acc_zmg = (int32_t)acc_z * 1000 / 8192;

        printf("acc_x : %ld mg  acc_y : %ld mg  acc_z : %ld mg\r\n",
               (long)acc_xmg, (long)acc_ymg, (long)acc_zmg);

        delay(500);
    }
}
