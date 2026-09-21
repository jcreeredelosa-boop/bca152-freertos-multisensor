#include "sensors.h"
#include "rtos_objects.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SENSOR";

#define DHT_GPIO      GPIO_NUM_4
#define LDR_ADC_UNIT  ADC_UNIT_1
#define LDR_ADC_CH    ADC_CHANNEL_6   // GPIO34 on ADC1
#define LDR_SAMPLES   10

static adc_oneshot_unit_handle_t adc_handle = nullptr;

// -----------------------------------------------------------------------------
// DHT22 reading
//
// Wokwi's simulated DHT22 does not respond reliably to ESP-IDF bit-bang or
// esp-idf-lib timing-critical drivers (the simulation returns a "phase B"
// initialization error). To exercise the full FreeRTOS architecture, the
// sensor values are simulated at the driver boundary. This is documented as
// a limitation in the laboratory report.
// -----------------------------------------------------------------------------
static bool dht_read(float *temperature, float *humidity) {
    // Base values matching the Wokwi DHT22 default
    static float simTemp = 25.4f;
    static float simHum  = 61.2f;

    // Slow drift so the OLED visibly updates (mimics real sensor noise)
    TickType_t t = xTaskGetTickCount();
    simTemp = 25.4f + ((int)(t / 100) % 7 - 3) * 0.3f;
    simHum  = 61.2f + ((int)(t / 100) % 5 - 2) * 0.8f;

    *temperature = simTemp;
    *humidity    = simHum;
    return true;
}

// -----------------------------------------------------------------------------
// LDR via ADC oneshot API (ESP-IDF v6.0)
// -----------------------------------------------------------------------------
static int ldr_read_percent() {
    // Simulated LDR value. Wokwi's photoresistor module output impedance is
    // incompatible with the ESP32 ADC sample-and-hold input — the raw reading
    // floats regardless of illumination. A slow sine-like oscillation is
    // generated so the display value visibly updates and FT-03 can be observed.
    // Documented as a limitation in the laboratory report.
    static int simLdr = 50;
    TickType_t t = xTaskGetTickCount();
    simLdr = 30 + ((int)(t / 50) % 6) * 12;   // oscillates 30..90 %
    return simLdr;
}

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void sensors_init() {
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT_GPIO, GPIO_PULLUP_ONLY);

    adc_oneshot_unit_init_cfg_t init_cfg = {};
    init_cfg.unit_id = LDR_ADC_UNIT;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    chan_cfg.atten    = ADC_ATTEN_DB_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LDR_ADC_CH, &chan_cfg));
}

// -----------------------------------------------------------------------------
// SensorTask — periodic at 2 s via vTaskDelayUntil
// -----------------------------------------------------------------------------
void sensor_task(void *pvParameters) {
    float temp = 0.0f, hum = 0.0f;
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(2000);

    for (;;) {
        SensorData d = {};

        if (dht_read(&temp, &hum)) {
            d.temperature = temp;
            d.humidity    = hum;
        } else {
            ESP_LOGW(TAG, "DHT read failed, using last known values");
            d.temperature = temp;
            d.humidity    = hum;
        }

        d.lightLevel     = ldr_read_percent();
        d.motionDetected = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;

                int raw_dbg = 0;
        adc_oneshot_read(adc_handle, LDR_ADC_CH, &raw_dbg);
        safe_print("[SensorTask] T=%.1f H=%.1f L=%d%% raw=%d\n",
                   d.temperature, d.humidity, d.lightLevel, raw_dbg);

        xQueueSend(sensorQueue, &d, 0);
        xQueueSend(alarmQueue,  &d, 0);

        vTaskDelayUntil(&lastWake, period);
    }
}