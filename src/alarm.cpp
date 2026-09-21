#include "alarm.h"
#include "rtos_objects.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ALARM";
#define BUZZER_GPIO GPIO_NUM_26

void alarm_init() {
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_GPIO, 0);
}

void alarm_task(void *pvParameters) {
    for (;;) {
        SensorData d;
        if (xQueueReceive(alarmQueue, &d, portMAX_DELAY) == pdTRUE) {
            AlarmState s = evaluateTemperature(d.temperature);
            if (s == AlarmState::NORMAL) {
                gpio_set_level(BUZZER_GPIO, 0);
                xEventGroupClearBits(systemEvents, EVENT_ALARM);
            } else {
                gpio_set_level(BUZZER_GPIO, 1);
                xEventGroupSetBits(systemEvents, EVENT_ALARM);
                safe_print("[AlarmTask] %s (T=%.1f)\n",
                    s == AlarmState::LOW_TEMPERATURE ? "LOW" : "HIGH",
                    d.temperature);
            }
        }
    }
}