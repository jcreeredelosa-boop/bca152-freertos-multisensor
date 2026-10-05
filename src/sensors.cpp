#include "sensors.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "SENSOR";

#define LDR_ADC_CHANNEL  ADC_CHANNEL_6

static adc_oneshot_unit_handle_t adc_handle;

static bool dht_read(float *temperature, float *humidity) {
    static const float temp_cycle[] = { 1.0f, 5.0f, 12.0f, 24.0f, 31.5f, 35.0f, 18.0f, -32.8f };
    static const float hum_cycle[]  = { 60.0f, 55.0f, 45.0f, 40.0f, 35.0f, 30.0f, 50.0f, 17.5f };
    static int index = 0;

    *temperature = temp_cycle[index];
    *humidity    = hum_cycle[index];

    index = (index + 1) % 8;

    return true;
}

static int ldr_read_percent() {
    int raw = 0;
    if (adc_oneshot_read(adc_handle, LDR_ADC_CHANNEL, &raw) != ESP_OK) {
        return 0;
    }
    int pct = (raw * 100) / 4095;
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

void sensors_init() {
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t chan_config = {};
    chan_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    chan_config.atten    = ADC_ATTEN_DB_12;
    adc_oneshot_config_channel(adc_handle, LDR_ADC_CHANNEL, &chan_config);

    ESP_LOGI(TAG, "Sensors initialized");
}

void sensor_task(void *pvParameters) {
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(2000);

    for (;;) {
        SensorData d = {};
        dht_read(&d.temperature, &d.humidity);
        d.lightLevel     = ldr_read_percent();
        d.motionDetected = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;

        xQueueSend(sensorQueue, &d, 0);
        xQueueSend(alarmQueue,  &d, 0);

        vTaskDelayUntil(&lastWake, period);
    }
}