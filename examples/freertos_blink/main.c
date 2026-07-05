#include <stddef.h>
#include <stdio.h>
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"

/* Toggles LD2 (PA5) on a 1 Hz cycle via vTaskDelay() instead of a busy-wait —
 * the task blocks and yields the CPU to the Idle task between toggles. */
void led_blink(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        led_on();
        vTaskDelay(pdMS_TO_TICKS(1000));
        led_off();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    led_init();

    xTaskCreate(led_blink, "BLINK LED", 128, NULL, 1, NULL);

    vTaskStartScheduler();

    for (;;) {}   /* unreachable — vTaskStartScheduler() does not return */
}

/* Required because configCHECK_FOR_STACK_OVERFLOW == 2 (FreeRTOSConfig.h).
 * printf is already retargeted to UART2 via __io_putchar (see uart.c). */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("Stack overflow in task: %s\r\n", pcTaskName);
}
