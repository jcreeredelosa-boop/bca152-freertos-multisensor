#include "input.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "INPUT";

// Simulated encoder pattern. Wokwi's KY-040 rotary UI does not always expose
// CW/CCW controls under the ESP-IDF extension, so mode changes are generated
// in firmware on a fixed schedule. The modeQueue publication path and the
// DisplayTask consumer logic are unchanged.
#define MODE_INTERVAL_MS 5000   // advance one page every 5 s

void input_init() {
    // No GPIO setup — input is simulated
}

void input_task(void *pvParameters) {
    static DisplayMode mode = DisplayMode::TEMPERATURE;

    // Publish initial mode so the display starts on Temperature
    xQueueOverwrite(modeQueue, &mode);
    ESP_LOGI(TAG, "InputTask started (simulated mode cycling, %d ms period)", MODE_INTERVAL_MS);

    TickType_t lastWake = xTaskGetTickCount();
    int cycle = 0;

    for (;;) {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(MODE_INTERVAL_MS));

        // Alternate CW / CCW to demonstrate both transitions.
        // Every 4th cycle advance (CW); otherwise retreat (CCW).
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