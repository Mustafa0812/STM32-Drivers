#include "i2c.h"
#include "stm32f4xx.h"

/* SR1 flag bits (RM0383 §18.6.6) */
#define SB    (1U << 0)    /* start condition generated */
#define ADDR  (1U << 1)    /* address sent and acknowledged */
#define BTF   (1U << 2)    /* byte transfer finished */
#define RxNE  (1U << 6)    /* receive data register not empty */
#define TxE   (1U << 7)    /* transmit data register empty */

/* SR2 flag bits (RM0383 §18.6.7) */
#define BSY   (1U << 1)    /* bus busy */

/* CR1 control bits (RM0383 §18.6.1) */
#define PE    (1U << 0)    /* peripheral enable */
#define START (1U << 8)    /* generate START condition */
#define STOP  (1U << 9)    /* generate STOP condition */
#define ACK   (1U << 10)   /* acknowledge enable */

/* Clock configuration for 100 kHz standard mode at 16 MHz APB1 (RM0383 §18.6.8–9) */
#define CLKFRQ    (1U << 4)   /* CR2 FREQ field: 16 MHz APB1 */
#define CCR_100K  80          /* T_high = T_low = 1/(2*100kHz) → 16MHz/(2*100kHz) */
#define RISE_TIME 17          /* (T_r_max / T_pclk) + 1 = (1000ns / 62.5ns) + 1 */

/* RCC peripheral clock enables */
#define GPIOBEN  (1U << 1)    /* AHB1ENR: GPIOB */
#define I2CEN    (1U << 21)   /* APB1ENR: I2C1 */

void i2c_init(void)
{
    /* --- GPIO config: PB8 = SCL, PB9 = SDA (RM0383 §6.3, Table 9) --- */
    RCC->AHB1ENR |= GPIOBEN;

    /* AF mode (10) on PB8 — MODER[17:16] */
    GPIOB->MODER |=  (1U << 17);
    GPIOB->MODER &= ~(1U << 16);

    /* AF mode (10) on PB9 — MODER[19:18] */
    GPIOB->MODER |=  (1U << 19);
    GPIOB->MODER &= ~(1U << 18);

    /* Open-drain: required for I2C wired-AND bus */
    GPIOB->OTYPER |= (1U << 8);
    GPIOB->OTYPER |= (1U << 9);

    /* Pull-up (01) on PB8 — PUPDR[17:16] */
    GPIOB->PUPDR |=  (1U << 16);
    GPIOB->PUPDR &= ~(1U << 17);

    /* Pull-up (01) on PB9 — PUPDR[19:18] */
    GPIOB->PUPDR |=  (1U << 18);
    GPIOB->PUPDR &= ~(1U << 19);

    /* AF4 (I2C1) on PB8 — AFRH[3:0] = 0100 */
    GPIOB->AFR[1] &= ~(1U << 3);
    GPIOB->AFR[1] |=  (1U << 2);
    GPIOB->AFR[1] &= ~(1U << 1);
    GPIOB->AFR[1] &= ~(1U << 0);

    /* AF4 (I2C1) on PB9 — AFRH[7:4] = 0100 */
    GPIOB->AFR[1] &= ~(1U << 7);
    GPIOB->AFR[1] |=  (1U << 6);
    GPIOB->AFR[1] &= ~(1U << 5);
    GPIOB->AFR[1] &= ~(1U << 4);

    /* --- I2C1 peripheral config --- */
    RCC->APB1ENR |= I2CEN;

    /* Software reset to clear any stuck state, then release */
    I2C1->CR1 |=  (1U << 15);
    I2C1->CR1 &= ~(1U << 15);

    I2C1->CR2   |= CLKFRQ;    /* input clock frequency */
    I2C1->CCR    = CCR_100K;  /* 100 kHz SCL */
    I2C1->TRISE  = RISE_TIME;
    I2C1->CR1   |= PE;
}

void i2c_burst_read_reg(uint8_t slave, uint8_t target, uint8_t n, uint8_t *buffer)
{
    while (I2C1->SR2 & BSY) {}

    /* Phase 1: START → slave address (write) → register address */
    I2C1->CR1 |= START;
    while (!(I2C1->SR1 & SB)) {}

    I2C1->DR = slave << 1;              /* write direction: bit 0 = 0 */
    while (!(I2C1->SR1 & ADDR)) {}
    (void)I2C1->SR1; (void)I2C1->SR2;  /* clear ADDR flag: read SR1 then SR2 */

    while (!(I2C1->SR1 & TxE)) {}
    I2C1->DR = target;
    while (!(I2C1->SR1 & TxE)) {}

    /* Phase 2: repeated START → slave address (read) */
    I2C1->CR1 |= START;
    while (!(I2C1->SR1 & SB)) {}

    I2C1->DR = (slave << 1) | 1;       /* read direction: bit 0 = 1 */
    I2C1->CR1 |= ACK;                  /* enable ACK before clearing ADDR */
    while (!(I2C1->SR1 & ADDR)) {}
    (void)I2C1->SR1; (void)I2C1->SR2;  /* clear ADDR */

    /* Read all bytes except the last with ACK */
    while (n > 1) {
        while (!(I2C1->SR1 & RxNE)) {}
        *buffer++ = I2C1->DR;
        n--;
    }

    /* Last byte: clear ACK and issue STOP before reading DR (RM0383 §18.3.3) */
    I2C1->CR1 &= ~ACK;
    I2C1->CR1 |= STOP;
    while (!(I2C1->SR1 & RxNE)) {}
    *buffer = I2C1->DR;
}

void i2c_write_reg(uint8_t slave, uint8_t target, uint8_t data)
{
    while (I2C1->SR2 & BSY) {}

    I2C1->CR1 |= START;
    while (!(I2C1->SR1 & SB)) {}

    I2C1->DR = slave << 1;              /* write direction: bit 0 = 0 */
    while (!(I2C1->SR1 & ADDR)) {}
    (void)I2C1->SR1; (void)I2C1->SR2;  /* clear ADDR */

    while (!(I2C1->SR1 & TxE)) {}
    I2C1->DR = target;
    while (!(I2C1->SR1 & TxE)) {}
    I2C1->DR = data;
    while (!(I2C1->SR1 & BTF)) {}      /* wait for both bytes shifted out */

    I2C1->CR1 |= STOP;
}



uint8_t i2c_read_reg (uint8_t slave, uint8_t target){

     while (I2C1->SR2 & BSY) {}

    /* Phase 1: START → slave address (write) → register address */
    I2C1->CR1 |= START;
    while (!(I2C1->SR1 & SB)) {}

    I2C1->DR = slave << 1;              /* write direction: bit 0 = 0 */
    while (!(I2C1->SR1 & ADDR)) {}
    (void)I2C1->SR1; (void)I2C1->SR2;  /* clear ADDR flag: read SR1 then SR2 */

    while (!(I2C1->SR1 & TxE)) {}
    I2C1->DR = target;
    while (!(I2C1->SR1 & TxE)) {}

    /* Phase 2: repeated START → slave address (read) */
    I2C1->CR1 |= START;
    while (!(I2C1->SR1 & SB)) {}

    I2C1->DR = (slave << 1) | 1;       /* read direction: bit 0 = 1 */
    I2C1->CR1 |= ACK;                  /* enable ACK before clearing ADDR */
    while (!(I2C1->SR1 & ADDR)) {}
    (void)I2C1->SR1; (void)I2C1->SR2;  /* clear ADDR */

    I2C1->CR1 &= ~ACK;
    I2C1->CR1 |= STOP;
    while (!(I2C1->SR1 & RxNE)) {}
    uint8_t result = I2C1->DR;

    return result;
}