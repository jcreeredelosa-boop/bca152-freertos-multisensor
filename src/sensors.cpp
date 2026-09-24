#include "sensors.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SENSOR";


#define SIM_TEMPERATURE_C   24.0f    // change to 31.5f to trigger the alarm
#define SIM_HUMIDITY_PCT    40.0f    // 0 - 100
#define SIM_LIGHT_PCT       50       // 0 - 100 (steady, never changes)


static bool dht_read(float *temperature, float *humidity) {
    *temperature = SIM_TEMPERATURE_C;
    *humidity    = SIM_HUMIDITY_PCT;
    return true;
}

static int ldr_read_percent() {
    return SIM_LIGHT_PCT;
}

void sensors_init() {
    // No hardware init needed for simulated sensors.
}

void sensor_task(void *pvParameters) {
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(2000);

    for (;;) {
        SensorData d = {};
        dht_read(&d.temperature, &d.humidity);
        d.lightLevel     = ldr_read_percent();
        d.motionDetected = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;

        safe_print("[SensorTask] T=%.1f H=%.1f L=%d%%\n",
                   d.temperature, d.humidity, d.lightLevel);

        xQueueSend(sensorQueue, &d, 0);
        xQueueSend(alarmQueue,  &d, 0);

        vTaskDelayUntil(&lastWake, period);
    }
}