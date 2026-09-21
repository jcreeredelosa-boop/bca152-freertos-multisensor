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

// ---------- DHT22 bit-bang (unchanged) ----------
static bool dht_read(float *temperature, float *humidity) {
    uint8_t data[5] = {0};

    gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(DHT_GPIO, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);

    int timeout;
    timeout = 0;
    while (gpio_get_level(DHT_GPIO) == 1) { if (++timeout > 200) return false; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(DHT_GPIO) == 0) { if (++timeout > 200) return false; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(DHT_GPIO) == 1) { if (++timeout > 200) return false; esp_rom_delay_us(1); }

    for (int i = 0; i < 40; i++) {
        while (gpio_get_level(DHT_GPIO) == 0) {}
        esp_rom_delay_us(35);
        if (gpio_get_level(DHT_GPIO) == 1) data[i / 8] |= (1 << (7 - (i % 8)));
        timeout = 0;
        while (gpio_get_level(DHT_GPIO) == 1) { if (++timeout > 200) return false; esp_rom_delay_us(1); }
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return false;

    *humidity = ((data[0] << 8) | data[1]) / 10.0f;
    int16_t t = ((data[2] & 0x7F) << 8) | data[3];
    if (data[2] & 0x80) t = -t;
    *temperature = t / 10.0f;
    return true;
}

// ---------- LDR via new oneshot API ----------
static int ldr_read_percent() {
    int total = 0;
    int valid = 0;
    for (int i = 0; i < LDR_SAMPLES; ++i) {
        int raw = 0;
        if (adc_oneshot_read(adc_handle, LDR_ADC_CH, &raw) == ESP_OK) {
            total += raw;
            ++valid;
        }
    }
    if (valid == 0) return 0;
    int avg = total / valid;             // 0..4095
    return (avg * 100) / 4095;           // 0..100 %
}

void sensors_init() {
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT_GPIO, GPIO_PULLUP_ONLY);

    adc_oneshot_unit_init_cfg_t init_cfg = {};
    init_cfg.unit_id = LDR_ADC_UNIT;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    chan_cfg.atten    = ADC_ATTEN_DB_12;   // v6.0 uses DB_12 (formerly DB_11)
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LDR_ADC_CH, &chan_cfg));
}

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

        safe_print("[SensorTask] T=%.1f H=%.1f L=%d%%\n",
                   d.temperature, d.humidity, d.lightLevel);

        xQueueSend(sensorQueue, &d, 0);
        xQueueSend(alarmQueue,  &d, 0);

        vTaskDelayUntil(&lastWake, period);
    }
}