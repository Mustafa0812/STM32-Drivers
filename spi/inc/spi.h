#ifndef _SPI_H
#define _SPI_H

#include "stm32f4xx.h"

void spi_gpio_init (void);
void spi_init(void);
uint8_t spi_transceive (uint8_t data);
void select_slave(void);
void deselect_slave(void);


#endif
