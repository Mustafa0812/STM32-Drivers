#include "dma_spi.h"
#include "stm32f4xx.h"

#define BUFF_SIZE 16


uint8_t tx_flag = 0;
uint8_t dma_done = 0;
static uint8_t rx_dummy[32];

static void dma_mosi_stream3(uint8_t *buff, uint16_t len);
static void dma_miso_stream2(uint8_t *buff, uint16_t len);

#include "spi.h"

/* RCC clock-enable bits from CMSIS (RM0383 §6.3.10, §6.3.15) */




void dma_spi_gpio_init(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* PA5 (SCK), PA6 (MISO), PA7 (MOSI) → alternate function — MODER bits = 10 */
    GPIOA->MODER |=  (1U << 11); GPIOA->MODER &= ~(1U << 10); /* PA5 */
    GPIOA->MODER |=  (1U << 13); GPIOA->MODER &= ~(1U << 12); /* PA6 */
    GPIOA->MODER |=  (1U << 15); GPIOA->MODER &= ~(1U << 14); /* PA7 */

    /* Set AF5 (SPI1) on PA5, PA6, PA7 via AFRL (RM0383 Table 9)
     * Each pin occupies 4 bits in AFR[0]; AF5 = 0101 */

    /* PA5 SCK  — AFR[0][23:20] = 0101 */
    GPIOA->AFR[0] &= ~(1U << 23); GPIOA->AFR[0] |=  (1U << 22);
    GPIOA->AFR[0] &= ~(1U << 21); GPIOA->AFR[0] |=  (1U << 20);

    /* PA6 MISO — AFR[0][27:24] = 0101 */
    GPIOA->AFR[0] &= ~(1U << 27); GPIOA->AFR[0] |=  (1U << 26);
    GPIOA->AFR[0] &= ~(1U << 25); GPIOA->AFR[0] |=  (1U << 24);

    /* PA7 MOSI — AFR[0][31:28] = 0101 */
    GPIOA->AFR[0] &= ~(1U << 31); GPIOA->AFR[0] |=  (1U << 30);
    GPIOA->AFR[0] &= ~(1U << 29); GPIOA->AFR[0] |=  (1U << 28);
}

void dma_spi_init(void)
{
    /* Enable SPI1 clock on APB2 */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure SPI_CR1 (RM0383 §20.5.1):
     * Mode 0 (CPOL=0 idle-low, CPHA=0 capture on first edge), MSB first,
     * 8-bit frame, software CS, master mode, fPCLK/32 */
    SPI1->CR1 &= ~(1U << 0);    /* CPHA = 0: data captured on first clock edge */
    SPI1->CR1 &= ~(1U << 1);    /* CPOL = 0: clock idle state is low */
    SPI1->CR1 &= ~(1U << 7);    /* LSBFIRST = 0: MSB transmitted first */
    SPI1->CR1 |=  (1U << 8);    /* SSI = 1: keep internal NSS high */
    SPI1->CR1 |=  (1U << 9);    /* SSM = 1: software slave management */
    SPI1->CR1 |=  (1U << 2);    /* MSTR = 1: master mode */
    SPI1->CR1 &= ~(1U << 11);   /* DFF = 0: 8-bit data frame */
    SPI1->CR1 |=  (0b100 << 3); /* BR[2:0] = 100: fPCLK/32 (~500 kHz at 16 MHz) */

    SPI1->CR1 |=  (1U << 6);    /* SPE = 1: enable SPI (must be set last) */

    SPI1 -> CR2 |= (1U << 1) | (1U << 0);

}

void dma2_init (void){

    RCC -> AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    NVIC_EnableIRQ (DMA2_Stream2_IRQn);
    NVIC_EnableIRQ (DMA2_Stream3_IRQn);

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
