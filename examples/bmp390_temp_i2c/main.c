#include "bmp390.h"
#include "i2c.h"
#include <stdint.h>
#include "uart.h"
#include "systick.h"
#include <stdio.h>

int main(void){

    uint8_t data_buffer[3];

    uart_init();
    i2c_init();
    bmp_init();

    int8_t  t3 = bmp_read(BMP390_T3);
    uint16_t t2 = ((bmp_read(BMP390_T2_1) << 8) | bmp_read(BMP390_T2_0));
    uint16_t t1 = ((bmp_read(BMP390_T1_1) << 8) | bmp_read(BMP390_T1_0));

    float par_T3 = (float)t3 / 281474976710656.0f;
    float par_T2 = (float)t2 / 1073741824.0f;
    float par_T1 = (float)t1 * 256.0f;

    while (1){

        bmp_burst_read(BMP390_TEMPDATA, data_buffer, 3);

        uint32_t adc_T = ((uint32_t)data_buffer[2] << 16) | ((uint32_t)data_buffer[1] << 8) | data_buffer[0];

        float diff   = (float)adc_T - par_T1;
        float var1   = diff * par_T2;
        float var2   = diff * diff * par_T3;
        float t_comp = var1 + var2;

        printf("Temp: %ld.%02ld C\r\n", (int32_t)t_comp, (int32_t)((t_comp - (int32_t)t_comp) * 100));
        delay(1000);
    }
}
