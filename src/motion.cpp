#include "motion.h"
#include "system_events.h"
#include "rtos_objects.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include <cstdio>

#define PIR_PIN GPIO_NUM_26
#define INACTIVITY_TIMEOUT_MS 15000

static void configure_pir_gpio(void) {
    gpio_config_t conf = {};
    conf.pin_bit_mask = 1ULL << PIR_PIN;
    conf.mode = GPIO_MODE_INPUT;
    conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&conf);
}

void MotionTask(void *pvParameters) {
    configure_pir_gpio();
    TickType_t lastMotionTick = xTaskGetTickCount();
    SystemState lastState = SystemState::ACTIVE;
    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);

    while (true) {
        bool motionNow = gpio_get_level(PIR_PIN) == 1;
        TickType_t now = xTaskGetTickCount();

        if (motionNow) {
            lastMotionTick = now;
            xEventGroupSetBits(systemEvents, EVENT_MOTION);
        } else {
            xEventGroupClearBits(systemEvents, EVENT_MOTION);
        }

        uint32_t idleMs = (now - lastMotionTick) * portTICK_PERIOD_MS;
        SystemState state = evaluateSystemState(motionNow, idleMs, INACTIVITY_TIMEOUT_MS);

        if (state != lastState) {
            xSemaphoreTake(serialMutex, portMAX_DELAY);
            printf("[MotionTask] State changed to %s\n",
                   state == SystemState::ACTIVE ? "ACTIVE" : "INACTIVE");
            xSemaphoreGive(serialMutex);

            if (state == SystemState::ACTIVE) {
                xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
            } else {
                xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
            }
            lastState = state;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}