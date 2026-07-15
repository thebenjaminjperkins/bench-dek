/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/temperature_sensor.h"
#include "freertos/queue.h"

static temperature_sensor_handle_t tsens;
static QueueHandle_t temperature_queue;

static void heartbeat_task(void *pvParameter) { 
    while (1) {
        printf("Heartbeat\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static void temperature_task(void *pvParameter) {
    while (1) { 
        float temperature;
        ESP_ERROR_CHECK(temperature_sensor_get_celsius(tsens, &temperature));
        xQueueSend(temperature_queue, &temperature, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void display_task(void *pvParameter) { 
    while(1) {
        float temp_out;
        if (xQueueReceive(temperature_queue, &temp_out, portMAX_DELAY)) {
            printf("Current temperature: %.2f Celsius\n", temp_out);
            printf("Current temperature: %.2f Fahrenheit\n", (temp_out * 9 / 5) + 32);
        }
    }
}

void app_main(void)
{
    // Set up the temperature sensor with default configuration
    temperature_sensor_config_t tsens_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&tsens_config, &tsens));
    ESP_ERROR_CHECK(temperature_sensor_enable(tsens));

    temperature_queue = xQueueCreate(10, sizeof(float));

    printf("Powering up...\n");
    xTaskCreate(temperature_task, "Temperature", 4096, NULL, 1, NULL);
    printf("Temperature task created.\n");

    xTaskCreate(heartbeat_task, "Heartbeat", 2048, NULL, 1, NULL);
    printf("Heartbeat task created.\n");

    xTaskCreate(display_task, "Display", 2048, NULL, 1, NULL);
    printf("Display task created.\n");

    return;
}
