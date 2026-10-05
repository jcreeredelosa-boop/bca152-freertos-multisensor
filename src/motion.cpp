#include "motion.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "MOTION";

#define MOTION_GPIO GPIO_NUM_27

void motion_init(void) {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << MOTION_GPIO);
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);

    lastMotionTick = xTaskGetTickCount();
    ESP_LOGI(TAG, "Motion initialized on GPIO 27");
}

void motion_task(void *pvParameters) {
    bool lastDetected = false;

    for (;;) {
        int level = gpio_get_level(MOTION_GPIO);
        bool detected = (level == 0);

        if (detected && !lastDetected) {
            lastMotionTick = xTaskGetTickCount();
            xEventGroupSetBits(systemEvents, EVENT_MOTION);
            ESP_LOGI(TAG, "Motion detected");
        } else if (!detected && lastDetected) {
            xEventGroupClearBits(systemEvents, EVENT_MOTION);
            ESP_LOGI(TAG, "Motion cleared");
        }

        lastDetected = detected;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}