#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_adc/adc_oneshot.h"
#include "dht22.h"
#include "sensor_data.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "system_events.h"
#include "motion.h"
#include "rtos_objects.h"

#define DHT_PIN GPIO_NUM_4

static QueueHandle_t sensorQueue;
static QueueHandle_t modeQueue;
static QueueHandle_t alarmQueue;
static adc_oneshot_unit_handle_t adc1_handle;

EventGroupHandle_t systemEvents;

void SensorTask(void *pvParameters)
{
    TickType_t lastWake = xTaskGetTickCount();
    while (true) {
        SensorData data = {};

        dht22_reading_t dht = dht22_read(DHT_PIN);
        data.temperature = dht.valid ? dht.temperature : 0;
        data.humidity    = dht.valid ? dht.humidity    : 0;

        int raw = 0;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &raw);
        data.lightLevel = (raw * 100) / 4095;

        data.motionDetected = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);
        xQueueSend(alarmQueue,  &data, portMAX_DELAY);

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2000));
    }
}

void DisplayTask(void *pvParameters)
{
    SensorData latest = {};
    bool haveData = false;
    DisplayMode mode = DisplayMode::TEMPERATURE;
    DisplayMode lastDrawnMode = (DisplayMode)0xFF;
    int lastDrawnValue = -9999;
    bool lastAlarmActive = false;

    while (true) {
        SensorData incoming;
        if (xQueueReceive(sensorQueue, &incoming, pdMS_TO_TICKS(200)) == pdTRUE) {
            latest = incoming;
            haveData = true;
            xSemaphoreTake(serialMutex, portMAX_DELAY);
            printf("[DisplayTask] Temp: %.1f C, Humidity: %.1f %%, Light: %d%%\n",
                   latest.temperature, latest.humidity, latest.lightLevel);
            xSemaphoreGive(serialMutex);
        }

        DisplayMode peeked;
        if (xQueuePeek(modeQueue, &peeked, 0) == pdTRUE) {
            mode = peeked;
        }

        EventBits_t bits = xEventGroupGetBits(systemEvents);
        bool systemActive = (bits & EVENT_ACTIVE) != 0;

        static bool wasActive = true;
        if (!systemActive) {
            if (wasActive) {
                display_clear();
                wasActive = false;
            }
            continue;
        }

        if (!wasActive) {
            lastDrawnMode = (DisplayMode)0xFF;
            lastDrawnValue = -9999;
            lastAlarmActive = !lastAlarmActive;
        }
        wasActive = true;

        if (!haveData) continue;

        int value = 0;
        switch (mode) {
            case DisplayMode::TEMPERATURE: value = (int)(latest.temperature * 10); break;
            case DisplayMode::HUMIDITY:    value = (int)(latest.humidity * 10);    break;
            case DisplayMode::LIGHT:       value = latest.lightLevel;              break;
            case DisplayMode::MOTION:      value = latest.motionDetected ? 1 : 0;  break;
        }

        bool alarmActive = (xEventGroupGetBits(systemEvents) & EVENT_ALARM) != 0;

        if (mode == lastDrawnMode && value == lastDrawnValue && alarmActive == lastAlarmActive) continue;
        lastDrawnMode = mode;
        lastDrawnValue = value;
        lastAlarmActive = alarmActive;

        display_clear();
        display_draw_string(0, 0, alarmActive ? "ROOM MONITOR *" : "ROOM MONITOR");

        char line[17];
        switch (mode) {
            case DisplayMode::TEMPERATURE:
                display_draw_string(2, 0, "TEMPERATURE");
                snprintf(line, sizeof(line), "%d.%dC",
                         (int)latest.temperature, (int)(latest.temperature * 10) % 10);
                break;
            case DisplayMode::HUMIDITY:
                display_draw_string(2, 0, "HUMIDITY");
                snprintf(line, sizeof(line), "%d.%d%%",
                         (int)latest.humidity, (int)(latest.humidity * 10) % 10);
                break;
            case DisplayMode::LIGHT:
                display_draw_string(2, 0, "LIGHT");
                snprintf(line, sizeof(line), "%d%%", latest.lightLevel);
                break;
            case DisplayMode::MOTION:
                display_draw_string(2, 0, "MOTION");
                snprintf(line, sizeof(line), "%s", latest.motionDetected ? "YES" : "NO");
                break;
        }
        display_draw_string(4, 0, line);
    }
}

extern "C" void app_main(void)
{
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");

    // ADC init
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;
    init_config.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
    init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_config = {};
    chan_config.atten = ADC_ATTEN_DB_12;
    chan_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &chan_config);

    // OLED init
    vTaskDelay(pdMS_TO_TICKS(150));
    display_init();
    display_clear();

    // Mutex
    serialMutex = xSemaphoreCreateMutex();

    // Queues
    sensorQueue = xQueueCreate(5, sizeof(SensorData));
    modeQueue   = xQueueCreate(1, sizeof(DisplayMode));
    alarmQueue  = xQueueCreate(5, sizeof(SensorData));

    DisplayMode initialMode = DisplayMode::TEMPERATURE;
    xQueueOverwrite(modeQueue, &initialMode);

    // Event group
    systemEvents = xEventGroupCreate();

    input_init(modeQueue);
    alarm_init(alarmQueue);

    // Tasks
    xTaskCreate(SensorTask,  "SensorTask",  4096, NULL, 2, NULL);
    xTaskCreate(InputTask,   "InputTask",   2048, NULL, 2, NULL);
    xTaskCreate(MotionTask,  "MotionTask",  2048, NULL, 2, NULL);
    xTaskCreate(AlarmTask,   "AlarmTask",   2048, NULL, 3, NULL);
    xTaskCreate(DisplayTask, "DisplayTask", 4096, NULL, 1, NULL);
}