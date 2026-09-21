#include "motion.h"
#include "rtos_objects.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MOTION";

// Simulated motion pattern. Wokwi's PIR and pushbutton models do not
// reliably drive ESP32 inputs under ESP-IDF, so motion is generated in
// firmware on a fixed 20-second cycle (5 s motion + 15 s idle).
#define MOTION_CYCLE_TICKS   600   // 600 * 50 ms = 30 s cycle
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
        

         vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(50));  // FAULT EXPERIMENT 1
    }
}
    
