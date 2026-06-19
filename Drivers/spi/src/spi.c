#include "spi.h"

/* RCC clock-enable bits from CMSIS (RM0383 §6.3.10, §6.3.15) */

/* SPI_SR flag bits (RM0383 §20.5.3) */
#define SPI_SR_TXE   (1U << 1)   /* Transmit buffer empty */
#define SPI_SR_RXNE  (1U << 0)   /* Receive buffer not empty */
#define SPI_SR_BSY   (1U << 7)   /* Busy flag */

void spi_gpio_init(void)
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

void spi_init(void)
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
}

uint8_t spi_transceive(uint8_t data)
{
    while (!(SPI1->SR & SPI_SR_TXE)){}    /* wait until TX buffer empty */
    SPI1->DR = data;

    while (!(SPI1->SR & SPI_SR_RXNE)){}   /* wait until RX buffer has data */
    uint8_t received = SPI1->DR;

    while (SPI1->SR & SPI_SR_BSY){}       /* wait for bus idle before returning */
    return received;
}

