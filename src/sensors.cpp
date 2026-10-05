#include "sensors.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "SENSOR";

#define DHT_GPIO         GPIO_NUM_4
#define LDR_ADC_CHANNEL  ADC_CHANNEL_6

static adc_oneshot_unit_handle_t adc_handle;

static bool dht_read(float *temperature, float *humidity) {
    uint8_t data[5] = {0};

    gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_GPIO, 0);
    esp_rom_delay_us(1200);
    gpio_set_level(DHT_GPIO, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);

    int timeout = 0;
    while (gpio_get_level(DHT_GPIO) == 1) {
        if (++timeout > 200) return false;
        esp_rom_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(DHT_GPIO) == 0) {
        if (++timeout > 200) return false;
        esp_rom_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(DHT_GPIO) == 1) {
        if (++timeout > 200) return false;
        esp_rom_delay_us(1);
    }

    for (int i = 0; i < 40; i++) {
        while (gpio_get_level(DHT_GPIO) == 0);
        int64_t t = esp_timer_get_time();
        while (gpio_get_level(DHT_GPIO) == 1);
        int64_t duration = esp_timer_get_time() - t;
        data[i / 8] <<= 1;
        if (duration > 40) data[i / 8] |= 1;
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        ESP_LOGW(TAG, "DHT checksum failed");
        return false;
    }

    *humidity = (float)((data[0] << 8) | data[1]) / 10.0f;
    int16_t t = ((data[2] & 0x7F) << 8) | data[3];
    if (data[2] & 0x80) t = -t;
    *temperature = (float)t / 10.0f;
    return true;
}

static int ldr_read_percent() {
    int raw = 0;
    if (adc_oneshot_read(adc_handle, LDR_ADC_CHANNEL, &raw) != ESP_OK) {
        return 0;
    }
    return (raw * 100) / 4095;
}

void sensors_init() {
    gpio_set_pull_mode(DHT_GPIO, GPIO_PULLUP_ONLY);

    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t chan_config = {};
    chan_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    chan_config.atten = ADC_ATTEN_DB_12;
    adc_oneshot_config_channel(adc_handle, LDR_ADC_CHANNEL, &chan_config);

    ESP_LOGI(TAG, "Sensors initialized (real DHT22 + LDR)");
}

void sensor_task(void *pvParameters) {
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(2000);

    for (;;) {
        SensorData d = {};
        if (!dht_read(&d.temperature, &d.humidity)) {
            d.temperature = -99.0f;
            d.humidity = -99.0f;
        }
        d.lightLevel = ldr_read_percent();
        d.motionDetected = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;

        xQueueSend(sensorQueue, &d, 0);
        xQueueSend(alarmQueue, &d, 0);

        vTaskDelayUntil(&lastWake, period);
    }
}