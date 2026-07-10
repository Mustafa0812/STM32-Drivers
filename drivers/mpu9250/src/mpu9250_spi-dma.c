#include "dma_spi.h"
#include "mpu9250_spi.h"
#include "systick.h"

/* MPU-9250 SPI protocol (PS-MPU-9250A-01 §3.3):
 * First byte = R/W bit (bit 7) | register address (bits 6:0)
 * 1 = read, 0 = write. Burst reads auto-increment the register address. */

#define MPU_CS_PIN  (1U << 4)

#define cs_assert()    (GPIOA->ODR &= ~MPU_CS_PIN)
#define cs_deassert()  (GPIOA->ODR |=  MPU_CS_PIN)

void mpu_write(uint8_t reg, uint8_t data)
{
    uint8_t tx[2] = {reg & 0x7F, data};
    cs_assert();
    spi_dma_transmit(tx, 2);
    cs_deassert();
}

uint8_t mpu_read_reg(uint8_t reg)
{
    uint8_t tx[2] = {reg | 0x80, 0xFF};
    uint8_t rx[2];
    cs_assert();
    spi_dma_transceive(rx, tx, 2);
    cs_deassert();
    return rx[1];
}

void mpu_burst_read(uint8_t reg, uint8_t length, uint8_t *buffer)
{
    uint8_t tx[length + 1];
    uint8_t rx[length + 1];
    tx[0] = reg | 0x80;
    for (uint8_t i = 1; i <= length; i++) tx[i] = 0xFF;

    cs_assert();
    spi_dma_transceive(rx, tx, length + 1);
    cs_deassert();

    for (uint8_t i = 0; i < length; i++) buffer[i] = rx[i + 1];
}

void mpu_init(void)
{
    /* PA4 → push-pull output (software CS, active-low) — MODER[9:8] = 01 */
    GPIOA->MODER |=  (1U << 8);
    GPIOA->MODER &= ~(1U << 9);
    cs_deassert();

    mpu_write(POWER_MANAGEMENT,  0x00);
    delay(6);
    mpu_write(POWER_MANAGEMENT2, 0x00);
    mpu_write(ACCEL_CONFIG,      0x08);
    mpu_write(GYRO_CONFIG,      0x08);
}
