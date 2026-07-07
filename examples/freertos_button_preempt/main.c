#include "uart.h"
#include "spi.h"
#include "dma_spi.h"
#include "mpu9250_spi.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

uint8_t data_buffer[6];
TaskHandle_t button_task_handle;

/* Runs in interrupt context (registered with exti_register_callback).
 * Must not touch task logic directly — it only signals button_task via a
 * task notification, the FreeRTOS-safe way to wake a task from an ISR. */
void callback_function(void)
{
    BaseType_t xHigherPriorityWasWoken = pdFALSE;

    vTaskNotifyGiveFromISR(button_task_handle, &xHigherPriorityWasWoken);

    /* If button_task (priority 2) outranks whatever was running, this
     * forces the context switch immediately instead of waiting up to one
     * tick period for the scheduler to notice on its own. */
    portYIELD_FROM_ISR(xHigherPriorityWasWoken);
}

/* Equal priority with mpu_main — round-robin, neither can starve the other. */
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

void mpu_main(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        mpu_burst_read(ACCEL_XOUT_H, 6, data_buffer);

        printf("raw: %02X %02X %02X %02X %02X %02X\r\n",
               data_buffer[0], data_buffer[1], data_buffer[2],
               data_buffer[3], data_buffer[4], data_buffer[5]);

        int16_t acc_x = (int16_t)((uint16_t)data_buffer[0] << 8 | data_buffer[1]);
        int16_t acc_y = (int16_t)((uint16_t)data_buffer[2] << 8 | data_buffer[3]);
        int16_t acc_z = (int16_t)((uint16_t)data_buffer[4] << 8 | data_buffer[5]);

        int32_t acc_xmg = (int32_t)acc_x * 1000 / 8192;
        int32_t acc_ymg = (int32_t)acc_y * 1000 / 8192;
        int32_t acc_zmg = (int32_t)acc_z * 1000 / 8192;

        printf("acc_x : %ld mg  acc_y : %ld mg  acc_z : %ld mg\r\n",
               (long)acc_xmg, (long)acc_ymg, (long)acc_zmg);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* Priority 2 — higher than led_blink/mpu_main (priority 1) and Idle
 * (priority 0). Sits Blocked (zero CPU) until the button ISR notifies it,
 * then preempts whichever of the other two tasks was running. */
void button_task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (int i = 0; i < 3; i++)
        {
            led_on();
            vTaskDelay(pdMS_TO_TICKS(100));
            led_off();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

int main(void)
{
    led_init();
    uart_init();
    spi_gpio_init();
    spi_init();
    dma2_init();    /* must come after spi_init — SPI1 clock must be on before CR2 is touched */
    mpu_init();
    btn_init();

    /* Register the callback before arming the interrupt — exti_init() makes
     * EXTI13 live immediately, and a press before registration would call a
     * NULL callback (silently ignored, but the press is lost). */
    exti_register_callback(callback_function);
    exti_init();

    xTaskCreate(led_blink, "BLINK LED", 128, NULL, 1, NULL);
    xTaskCreate(mpu_main, "READ SENSOR", 512, NULL, 1, NULL);
    xTaskCreate(button_task, "BUTTON TASK", 128, NULL, 2, &button_task_handle);

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
