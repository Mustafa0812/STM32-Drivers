#ifndef BMP390_H_
#define BMP390_H_

#include <stdint.h>

#define BMP390_CHIP_ID     0x00
#define BMP390_STATUS      0x03
#define BMP390_TEMPDATA  0x07
#define BMP390_IF_CONF     0x1A
#define BMP390_PWR_CTRL    0x1B
#define BMP390_T1_0        0x31
#define BMP390_T1_1        0x32
#define BMP390_T2_1        0x34
#define BMP390_T2_0        0x33
#define BMP390_T3          0x35
#define BMP390_OSR         0x1C
#define BMP390_ODR         0x1D
#define BMP390_ADDR        0x77

void bmp_init(void);
void bmp_burst_read(uint8_t reg, uint8_t *buffer, int n);
uint8_t bmp_read(uint8_t reg);
void bmp_write(uint8_t reg, uint8_t data);

#endif