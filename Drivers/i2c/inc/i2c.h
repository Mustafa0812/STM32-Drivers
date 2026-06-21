#ifndef _I2C_H
#define _I2C_H

#include <stdint.h>

void    i2c_init(void);
void    i2c_burst_read_reg(uint8_t slave, uint8_t target, uint8_t n, uint8_t *buffer);
void    i2c_write_reg(uint8_t slave, uint8_t target, uint8_t data);
uint8_t    i2c_read_reg (uint8_t slave, uint8_t target);

#endif
