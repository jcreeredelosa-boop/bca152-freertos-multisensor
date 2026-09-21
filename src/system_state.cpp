#include "system_state.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STATE";
#define INACTIVITY_TIMEOUT_MS 15000

void state_task(void *pvParameters) {
    SystemState state = SystemState::ACTIVE;
    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);

    for (;;) {
        TickType_t now  = xTaskGetTickCount();
        bool timedOut   = (now - lastMotionTick) > pdMS_TO_TICKS(INACTIVITY_TIMEOUT_MS);
        bool motion     = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;

        SystemState next = evaluateSystemState(state, motion, timedOut);
        if (next != state) {
            state = next;
            if (state == SystemState::ACTIVE) {
                xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
                ESP_LOGI(TAG, "-> ACTIVE");
            } else {
                xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
                ESP_LOGI(TAG, "-> INACTIVE");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}