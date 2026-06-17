#ifndef _I2C_H
#define _I2C_H

#include <stdint.h>

void    i2c_init(void);
void    burst_read_reg(uint8_t slave, uint8_t target, uint8_t n, uint8_t *buffer);
void    write_reg(uint8_t slave, uint8_t target, uint8_t data);

#endif
