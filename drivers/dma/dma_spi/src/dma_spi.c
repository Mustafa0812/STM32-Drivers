#include "dma_spi.h"
#include "stm32f4xx.h"

#define BUFF_SIZE 16


uint8_t tx_flag = 0;
uint8_t dma_done = 0;
static uint8_t rx_dummy[32];

static void dma_mosi_stream3(uint8_t *buff, uint16_t len);
static void dma_miso_stream2(uint8_t *buff, uint16_t len);

#include "spi.h"

void dma2_init (void){

    RCC -> AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    NVIC_EnableIRQ (DMA2_Stream2_IRQn);
    NVIC_EnableIRQ (DMA2_Stream3_IRQn);

    /* SPI1 clock is already enabled by spi_init() at this point (see main.c
     * ordering comment) — safe to touch CR2 now. Without RXDMAEN/TXDMAEN,
     * SPI1 never issues a DMA request, NDTR never decrements, TCIF never
     * sets, and spi_dma_transceive()'s wait loop spins forever. */
    SPI1->CR2 |= (1U << 1) | (1U << 0);   /* TXDMAEN, RXDMAEN */

}


void spi_dma_transceive(uint8_t *rx_buff, uint8_t *tx_buff, uint16_t len)
{
    dma_done = 0;
    dma_miso_stream2(rx_buff, len);    /* arm RX first */
    dma_mosi_stream3(tx_buff, len);    /* then TX */
    while (!dma_done) {}
}

void spi_dma_transmit(const uint8_t *tx_buff, uint16_t len)
{
    spi_dma_transceive(rx_dummy, (uint8_t *)tx_buff, len);
}

void dma_mosi_stream3 (uint8_t *buff, uint16_t len){

    DMA2_Stream3 -> CR &= ~ (1U << 0);
    
    while (DMA2_Stream3 -> CR & (1U << 0)){}

    DMA2 -> LIFCR |= ((1U << 27) | (1U << 25)| (1U << 24));

    DMA2_Stream3 -> PAR = (uint32_t) (&(SPI1 ->DR));

    DMA2_Stream3->M0AR = (uint32_t)buff;

    DMA2_Stream3 -> NDTR = len;

    DMA2_Stream3 -> CR |= (1U << 6);
    DMA2_Stream3 -> CR &= ~(1U << 7);

    DMA2_Stream3 -> CR |= (1U << 25);
    DMA2_Stream3 -> CR |= (1U << 26);
    DMA2_Stream3 -> CR &= ~(1U << 27);

    DMA2_Stream3 -> CR |= (1U << 10);

    DMA2_Stream3 -> CR |= (1U << 4);

    DMA2_Stream3 -> CR |= (1U << 0);

}

void dma_miso_stream2 (uint8_t *buff, uint16_t len){

    DMA2_Stream2 -> CR &= ~ (1U << 0);
    
    while (DMA2_Stream2 -> CR & (1U << 0)){}

    DMA2 -> LIFCR |= ((1U << 21) | (1U << 19)| (1U << 18));

    DMA2_Stream2 -> PAR = (uint32_t) (&(SPI1 ->DR));

    DMA2_Stream2->M0AR = (uint32_t)buff;

    DMA2_Stream2 -> NDTR = len;

    DMA2_Stream2 -> CR &= ~(1U << 6);
    DMA2_Stream2 -> CR &= ~(1U << 7);

    DMA2_Stream2 -> CR |= (1U << 25);
    DMA2_Stream2 -> CR |= (1U << 26);
    DMA2_Stream2 -> CR &= ~(1U << 27);

    DMA2_Stream2 -> CR |= (1U << 10);

    DMA2_Stream2 -> CR |= (1U << 4);

    
    DMA2_Stream2 -> CR |= (1U << 0);

}


void DMA2_Stream3_IRQHandler (void){

    if (DMA2->LISR & (1U << 27)) {    // check TCIF3
        DMA2->LIFCR |= (1U << 27);
        tx_flag = 1;

    }
}

void DMA2_Stream2_IRQHandler (void) {

    if (DMA2->LISR & (1U << 21)) {    // check TCIF2
        DMA2->LIFCR |= (1U << 21);
        dma_done = 1;
    }
}
