#include "input.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "INPUT";

// Simulated mode cycling: advance one page every 5 s.
#define MODE_INTERVAL_MS 5000

void input_init() {
    // No GPIO setup — input is simulated.
}

void input_task(void *pvParameters) {
    static DisplayMode mode = DisplayMode::TEMPERATURE;

    xQueueOverwrite(modeQueue, &mode);
    ESP_LOGI(TAG, "InputTask started (simulated mode cycling, %d ms period)", MODE_INTERVAL_MS);

    TickType_t lastWake = xTaskGetTickCount();
    int cycle = 0;

    for (;;) {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(MODE_INTERVAL_MS));

        cycle++;
        if ((cycle % 4) == 0) {
            mode = previousDisplayMode(mode);
            ESP_LOGI(TAG, "CCW -> mode %d", (int)mode);
        } else {
            mode = nextDisplayMode(mode);
            ESP_LOGI(TAG, "CW -> mode %d", (int)mode);
        }
        xQueueOverwrite(modeQueue, &mode);
    }
}