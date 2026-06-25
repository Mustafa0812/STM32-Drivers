#ifndef DMA_SPI_H
#define DMA_SPI_H

#include <stdint.h>

void dma2_init(void);
void spi_dma_transceive(uint8_t *rx_buff, uint8_t *tx_buff, uint16_t len);
void spi_dma_transmit(const uint8_t *tx_buff, uint16_t len);
void dma_spi_gpio_init(void);
void dma_spi_init(void);


#endif 