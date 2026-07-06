#include "spi.h"
#include <stddef.h>

/* RCC clock-enable bits from CMSIS (RM0383 §6.3.10, §6.3.15) */

/* SPI_SR flag bits (RM0383 §20.5.3) — SPI_SR_TXE/RXNE/BSY already defined in CMSIS stm32f4xx.h */

void spi_gpio_init(void)
{
    /* Enable GPIOB clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* PB3 (SCK), PB4 (MISO), PB5 (MOSI) → alternate function — MODER bits = 10 */
    GPIOB->MODER |=  (1U << 11); GPIOB->MODER &= ~(1U << 10); /* PB5 */
    GPIOB->MODER |=  (1U << 7); GPIOB->MODER &= ~(1U << 6); /* PB3 */
    GPIOB->MODER |=  (1U << 9); GPIOB->MODER &= ~(1U << 8); /* PB4 */

    /* Set AF5 (SPI1) on PB5, PB3, PB4 via AFRL (RM0383 Table 9)
     * Each pin occupies 4 bits in AFR[0]; AF5 = 0101 */

    /* PB5 MOSI  — AFR[0][23:20] = 0101 */
    GPIOB->AFR[0] &= ~(1U << 23); GPIOB->AFR[0] |=  (1U << 22);
    GPIOB->AFR[0] &= ~(1U << 21); GPIOB->AFR[0] |=  (1U << 20);

    /* PB4 MISO — AFR[0][19:16] = 0101 */
    GPIOB->AFR[0] &= ~(1U << 17); GPIOB->AFR[0] |=  (1U << 16);
    GPIOB->AFR[0] &= ~(1U << 19); GPIOB->AFR[0] |=  (1U << 18);

    /* PB3 SCK — AFR[0][15:12] = 0101 */
    GPIOB->AFR[0] &= ~(1U << 15); GPIOB->AFR[0] |=  (1U << 14);
    GPIOB->AFR[0] &= ~(1U << 13); GPIOB->AFR[0] |=  (1U << 12);
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
    while (!(SPI1->SR & SPI_SR_TXE)){}
    SPI1->DR = data;
    
    while (!(SPI1->SR & SPI_SR_RXNE)){}
    return SPI1->DR;
}
