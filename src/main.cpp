#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <cstdio>

#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"
#include "system_state.h"

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    // Diagnostic print first — confirms app_main was reached
    printf("=== BOOT: app_main reached ===\n");
    fflush(stdout);

    ESP_LOGI(TAG, "BCA152 FreeRTOS Multisensor");
    ESP_LOGI(TAG, "System starting...");

    rtos_objects_init();
    ESP_LOGI(TAG, "RTOS objects created");

    sensors_init();
    ESP_LOGI(TAG, "Sensors initialized");

    display_init();
    ESP_LOGI(TAG, "Display initialized");

    input_init();
    ESP_LOGI(TAG, "Input initialized");

    alarm_init();
    ESP_LOGI(TAG, "Alarm initialized");

    motion_init();
    ESP_LOGI(TAG, "Motion initialized");

    // Priorities justified in README / report
    xTaskCreate(sensor_task,  "SensorTask",  4096, nullptr, 2, nullptr);
    xTaskCreate(display_task, "DisplayTask", 4096, nullptr, 1, nullptr);
    xTaskCreate(input_task,   "InputTask",   3072, nullptr, 3, nullptr);
    xTaskCreate(motion_task,  "MotionTask",  3072, nullptr, 3, nullptr);
    xTaskCreate(alarm_task,   "AlarmTask",   3072, nullptr, 2, nullptr);
    xTaskCreate(state_task,   "StateTask",   3072, nullptr, 2, nullptr);

    ESP_LOGI(TAG, "All tasks created — scheduler running");
}