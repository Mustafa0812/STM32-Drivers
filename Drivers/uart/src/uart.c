/**
 * @file    uart.c
 * @brief   Interrupt-driven USART2 TX driver for STM32F411
 *
 * Implements a 256-byte ring buffer drained by the USART2 TXE interrupt
 * so that printf / putchar return immediately instead of stalling the CPU
 * for each byte. Newlib is retargeted through __io_putchar, which is called
 * by syscalls.c's _write for every character printf produces.
 *
 * Pin mapping (Nucleo-64):
 *   PA2  — USART2_TX, AF7, connected to ST-LINK virtual COM port
 *
 * Ring buffer design (single-producer / single-consumer):
 *   - uart_write  : producer — runs in thread context (called from _write)
 *   - USART2_IRQHandler : consumer — runs in interrupt context
 *
 * Race condition fix:
 *   Before this fix the ISR could fire between the ring_buffer[] write and
 *   the write_index update in uart_write. It would then read the new byte at
 *   read_index == write_index (old), increment read_index past write_index,
 *   and TXEIE would never be disabled — causing a runaway ISR that floods
 *   the UART with 255 null bytes and loses the remaining string.
 *
 *   Fix: TXEIE is briefly cleared around the write_index commit so the ISR
 *   cannot observe a partially-written state. The ISR also guards against an
 *   empty buffer before touching DR, stopping any residual runaway.
 */

#include "uart.h"
#include <stdint.h>
#include "stm32f4xx.h"
#include <stddef.h>

/* ── Clock and baud rate ─────────────────────────────────────────────────── */

#define CLKFRQ    16000000UL   /* 16 MHz HSI (default reset clock, no PLL) */
#define BAUDRATE   115200UL

/* ── USART register bit definitions (RM0383) ────────────────────────────── */

/* SR — status register (§19.6.1) */
#define USART_SR_TXE   (1U << 7)   /* Transmit data register empty         */

/* CR1 — control register 1 (§19.6.4) */
#define USART_CR1_TE   (1U << 3)   /* Transmitter enable                   */
#define USART_CR1_UE   (1U << 13)  /* USART enable                         */
/* USART_CR1_TXEIE is (1U << 7) — defined in CMSIS stm32f4xx.h             */

/* ── Ring buffer ─────────────────────────────────────────────────────────── */

#define TX_BUFF_SIZE 256   /* must be a power of 2 for the modulo to be cheap */

static uint8_t          ring_buffer[TX_BUFF_SIZE] = {0};
static volatile int     write_index = 0;   /* next slot to write — thread only */
static volatile int     read_index  = 0;   /* next slot to read  — ISR only    */

static void uart_write(uint8_t ch);

/* ── Newlib retargeting ──────────────────────────────────────────────────── */

/* Called by syscalls.c _write() for every character that printf produces.  */
int __io_putchar(int ch)
{
    uart_write((uint8_t)ch);
    return ch;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief Configure PA2 as USART2 TX and start the peripheral.
 *
 * After this call printf routes through __io_putchar and out over UART.
 * The USART2 TXE interrupt is enabled so transmission is non-blocking.
 */
void uart_init(void)
{
    /* 1. GPIO — enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. PA2 → alternate-function mode (MODER[5:4] = 10) */
    GPIOA->MODER |=  (1U << 5);
    GPIOA->MODER &= ~(1U << 4);

    /* 3. Select AF7 (USART2) on PA2 via AFRL — bits [11:8] = 0111
          (RM0383 Table 9 / STM32F411 datasheet alt-function table) */
    GPIOA->AFR[0] &= ~(1U << 11);
    GPIOA->AFR[0] |=  (1U << 10);
    GPIOA->AFR[0] |=  (1U << 9);
    GPIOA->AFR[0] |=  (1U << 8);

    /* 4. Enable USART2 clock on APB1 (RM0383 §6.3.12) */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* 5. Baud rate: BRR = fCLK / BAUD, rounded to nearest integer
          16 000 000 / 115 200 ≈ 138.9 → 139 → actual rate ≈ 115 108 (0.08% error)
          (RM0383 §19.3.4) */
    USART2->BRR = (uint16_t)((CLKFRQ + BAUDRATE / 2) / BAUDRATE);

    /* 6. Enable transmitter, then enable USART (RM0383 §19.6.4)
          TE must be set before UE to guarantee the idle frame is sent first */
    USART2->CR1  = USART_CR1_TE;
    USART2->CR1 |= USART_CR1_UE;

    /* 7. Unmask USART2 in the NVIC; TXEIE is only set when the buffer has data */
    NVIC_EnableIRQ(USART2_IRQn);
}

/* ── Internal transmit ───────────────────────────────────────────────────── */

/**
 * @brief Queue one byte for transmission.
 *
 * Silently drops the byte if the ring buffer is full (255 bytes pending).
 *
 * Critical-section strategy:
 *   The ISR can preempt anywhere in this function.  The only dangerous window
 *   is between writing ring_buffer[write_index] and committing write_index —
 *   if the ISR fires there it reads the new byte early, then increments
 *   read_index past write_index, leaving TXEIE enabled with an empty buffer
 *   (runaway ISR sending null bytes).
 *
 *   Clearing TXEIE before the commit and restoring it after makes that window
 *   IRQ-free: the ISR cannot be entered while TXEIE is cleared.
 */
static void uart_write(uint8_t ch)
{
    uint8_t next_index = (write_index + 1) % TX_BUFF_SIZE;

    if (next_index == read_index)
        return;   /* buffer full — drop byte */

    ring_buffer[write_index] = ch;

    /* Atomically commit the new write_index.
       TXEIE cleared → write_index updated → TXEIE restored.
       The ISR cannot observe write_index mid-update. */
    USART2->CR1 &= ~USART_CR1_TXEIE;
    write_index = next_index;
    USART2->CR1 |=  USART_CR1_TXEIE;
}

/* ── ISR ─────────────────────────────────────────────────────────────────── */

/**
 * @brief USART2 interrupt handler — drains the ring buffer one byte per TXE.
 *
 * TXE fires when the transmit data register is empty and ready for the next
 * byte.  One byte is moved from the ring buffer to DR per ISR entry.
 * TXEIE is disabled when the last byte has been committed so the ISR does
 * not re-enter until uart_write adds more data.
 *
 * Empty-buffer guard: if TXEIE was re-enabled by uart_write after the ISR
 * already drained the last byte, this check prevents sending uninitialised
 * data and immediately disables TXEIE again.
 */
void USART2_IRQHandler(void)
{
    if (USART2->SR & USART_SR_TXE) {

        /* Guard: disable TXEIE and bail if nothing is queued */
        if (read_index == write_index) {
            USART2->CR1 &= ~USART_CR1_TXEIE;
            return;
        }

        USART2->DR = ring_buffer[read_index];
        read_index = (read_index + 1) % TX_BUFF_SIZE;

        /* Disable TXEIE once the last queued byte has been loaded into DR */
        if (read_index == write_index) {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}
