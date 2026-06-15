#include "gpio.h"

/* RCC_AHB1ENR bits (RM0383 §6.3.10) */
#define GPIOAEN  (1U << 0)   /* GPIOA clock enable */
#define GPIOCEN  (1U << 2)   /* GPIOC clock enable */

/* GPIOA BSRR bit positions for LD2 LED on PA5 (RM0383 §8.4.7)
 * BS5 (bit 5)  → set PA5 high; BR5 (bit 21) → set PA5 low */
#define LED_SET    (1U << 5)
#define LED_RESET  (1U << 21)

/* PC13 bit mask for IDR — user button, active-low (Nucleo UM1724 §7.7) */
#define BUTTON  (1U << 13)

void led_init(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= GPIOAEN;

    /* PA5 → output — MODER[11:10] = 01 */
    GPIOA->MODER |=  (1U << 10);
    GPIOA->MODER &= ~(1U << 11);
}

void btn_init(void)
{
    /* Enable GPIOC clock */
    RCC->AHB1ENR |= GPIOCEN;

    /* PC13 → input (reset state) — MODER[27:26] = 00 */
    GPIOC->MODER &= ~(1U << 26);
    GPIOC->MODER &= ~(1U << 27);
}

bool btn_state(void)
{
    /* Button is active-low: IDR bit is 1 when not pressed, 0 when pressed */
    if (GPIOC->IDR & BUTTON)
        return false;
    else
        return true;
}

void led_on(void)
{
    GPIOA->BSRR |= LED_SET;
}

void led_off(void)
{
    GPIOA->BSRR |= LED_RESET;
}
