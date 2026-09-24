#include "motion.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MOTION";

// Simulated motion pattern: 3 s of motion, 17 s idle, repeating.
#define MOTION_CYCLE_TICKS   400   // 400 * 50 ms = 20 s
#define MOTION_ACTIVE_TICKS   60   // 60 * 50 ms = 3 s of motion

void motion_init() {
    lastMotionTick = xTaskGetTickCount();
}

void motion_task(void *pvParameters) {
    TickType_t lastWake = xTaskGetTickCount();
    int cycle = 0;

    ESP_LOGI(TAG, "MotionTask started (simulated motion pattern, 20 s cycle)");

    for (;;) {
        cycle = (cycle + 1) % MOTION_CYCLE_TICKS;
        bool motion = (cycle < MOTION_ACTIVE_TICKS);

        if (motion) {
            lastMotionTick = xTaskGetTickCount();
            if (cycle == 1) {
                xEventGroupSetBits(systemEvents, EVENT_MOTION);
                ESP_LOGI(TAG, "Motion detected");
            }
        } else {
            if (cycle == MOTION_ACTIVE_TICKS) {
                xEventGroupClearBits(systemEvents, EVENT_MOTION);
                ESP_LOGI(TAG, "Motion cleared");
            }
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(50));
    }
}