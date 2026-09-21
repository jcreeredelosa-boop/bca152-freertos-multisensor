#include "motion.h"
#include "rtos_objects.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MOTION";
#define PIR_GPIO GPIO_NUM_27

void motion_init() {
    gpio_set_direction(PIR_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIR_GPIO, GPIO_PULLDOWN_ONLY);
    lastMotionTick = xTaskGetTickCount();  // start fresh
}

void motion_task(void *pvParameters) {
    TickType_t lastWake = xTaskGetTickCount();
    bool prev = false;

    for (;;) {
        bool motion = (gpio_get_level(PIR_GPIO) == 1);
        if (motion && !prev) {
            lastMotionTick = xTaskGetTickCount();
            xEventGroupSetBits(systemEvents, EVENT_MOTION);
            ESP_LOGI(TAG, "Motion detected");
        }
        if (!motion && prev) {
            xEventGroupClearBits(systemEvents, EVENT_MOTION);
            ESP_LOGI(TAG, "Motion cleared");
        }
        prev = motion;
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
    }
}