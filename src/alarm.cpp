#include "alarm.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "ALARM";

#define BUZZER_GPIO  GPIO_NUM_26
#define TEMP_LOW     18.0f
#define TEMP_HIGH    30.0f

void alarm_init(void) {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << BUZZER_GPIO);
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
    gpio_set_level(BUZZER_GPIO, 0);
    ESP_LOGI(TAG, "Alarm initialized");
}

void alarm_task(void *pvParameters) {
    bool alarmActive = false;

    for (;;) {
        SensorData d;
        if (xQueueReceive(alarmQueue, &d, portMAX_DELAY) == pdTRUE) {
            bool shouldAlarm = (d.temperature < TEMP_LOW) || (d.temperature > TEMP_HIGH);

            if (shouldAlarm && !alarmActive) {
                alarmActive = true;
                gpio_set_level(BUZZER_GPIO, 1);
                if (d.temperature < TEMP_LOW) {
                    printf("[AlarmTask] Temp: %.1f C -> state changed to LOW TEMPERATURE\n", d.temperature);
                } else {
                    printf("[AlarmTask] Temp: %.1f C -> state changed to HIGH TEMPERATURE\n", d.temperature);
                }
                fflush(stdout);
            } else if (!shouldAlarm && alarmActive) {
                alarmActive = false;
                gpio_set_level(BUZZER_GPIO, 0);
                printf("[AlarmTask] Temp: %.1f C -> state changed to NORMAL\n", d.temperature);
                fflush(stdout);
            }
        }
    }
}