#include "bmp390.h"
#include "i2c.h"

void bmp_burst_read (uint8_t reg, uint8_t *buffer, int n){

    i2c_burst_read_reg(BMP390_ADDR, reg, n, buffer);
}

uint8_t bmp_read (uint8_t reg){

    return i2c_read_reg(BMP390_ADDR, reg);
}

void bmp_write (uint8_t reg, uint8_t data){

    i2c_write_reg(BMP390_ADDR, reg, data);

}


void bmp_init(void){

    bmp_write(BMP390_IF_CONF,  0x00);         /* I2C interface */
    bmp_write(BMP390_OSR,      0x08);         /* temperature oversampling x8 (OSR_T = 011) */
    bmp_write(BMP390_ODR,      0x00);         /* ODR 200 Hz */
    bmp_write(BMP390_PWR_CTRL, 0x32);         /* normal mode, temp_en=1 */

}

