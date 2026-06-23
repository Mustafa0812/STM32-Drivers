#include "uart.h"
#include <stdint.h>
#include "stm32f4xx.h"
#include <stddef.h>

/* RCC clock-enable bits from CMSIS (RM0383 §6.3.10, §6.3.12) */

#define CLKFRQ    16000000UL
#define BAUDRATE   115200UL

/* USART_SR bits (RM0383 §19.6.1) */
#define USART_SR_TXE  (1U << 7)   /* Transmit data register empty */

/* USART_CR1 bits (RM0383 §19.6.4) */
#define USART_CR1_TE  (1U << 3)   /* Transmitter enable */
#define USART_CR1_UE  (1U << 13)  /* USART enable */

#define TX_BUFF_SIZE 256

static uint8_t ring_buffer[TX_BUFF_SIZE] = {0};
static volatile int write_index = 0;
static volatile int read_index = 0;

static void uart_write(uint8_t ch);

/* Newlib retargeting hook — called by printf/putchar */
int __io_putchar(int ch)
{
    uart_write(ch);
    return ch;
}

void uart_init(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* PA2 (USART2_TX) → alternate function — MODER[5:4] = 10 */
    GPIOA->MODER |=  (1U << 5);
    GPIOA->MODER &= ~(1U << 4);

    /* Set AF7 (USART2) on PA2 via AFRL — AFR[0][11:8] = 0111 (RM0383 Table 9) */
    GPIOA->AFR[0] &= ~(1U << 11);
    GPIOA->AFR[0] |=  (1U << 10);
    GPIOA->AFR[0] |=  (1U << 9);
    GPIOA->AFR[0] |=  (1U << 8);

    /* Enable USART2 clock on APB1 */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* BRR = fCLK / BAUD (with rounding) (RM0383 §19.3.4) */
    USART2->BRR = (uint16_t)((CLKFRQ + BAUDRATE / 2) / BAUDRATE);

    /* Enable transmitter then USART (RM0383 §19.6.4) */
    USART2->CR1  = USART_CR1_TE;
    USART2->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(USART2_IRQn);
    
}

static void uart_write(uint8_t ch){
    
   uint8_t next_index = (write_index + 1) % TX_BUFF_SIZE;

   if (next_index != read_index){

        ring_buffer[write_index] = ch;

        write_index = next_index;

        USART2 ->CR1 |= USART_CR1_TXEIE;

   }
    
}



void USART2_IRQHandler (void){

    if (USART2 -> SR & USART_SR_TXE){

        USART2 -> DR = ring_buffer[read_index];
        read_index = (read_index + 1) % TX_BUFF_SIZE;

        if (read_index == write_index){

            USART2 ->CR1 &= ~USART_CR1_TXEIE;

        }
    }

}