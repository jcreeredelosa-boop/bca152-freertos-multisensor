    #include "dht22.h"
    #include "freertos/FreeRTOS.h"
    #include "esp_timer.h"
    #include "esp_rom_sys.h"

    static bool wait_for_level(gpio_num_t pin, int level, int timeout_us)
    {
        int64_t start = esp_timer_get_time();
        while (gpio_get_level(pin) != level) {
            if (esp_timer_get_time() - start > timeout_us)
                return false;
        }
        return true;
    }

    dht22_reading_t dht22_read(gpio_num_t pin)
    {
        dht22_reading_t result = { 0, 0, false };
        uint8_t data[5] = {0, 0, 0, 0, 0};

        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        gpio_set_level(pin, 0);
        esp_rom_delay_us(1000);
        gpio_set_level(pin, 1);
        esp_rom_delay_us(30);

        gpio_set_direction(pin, GPIO_MODE_INPUT);

        if (!wait_for_level(pin, 0, 100)) return result;
        if (!wait_for_level(pin, 1, 100)) return result;
        if (!wait_for_level(pin, 0, 100)) return result;

        for (int i = 0; i < 40; i++) {
            if (!wait_for_level(pin, 1, 100)) return result;
            int64_t high_start = esp_timer_get_time();
            if (!wait_for_level(pin, 0, 100)) return result;
            int64_t duration = esp_timer_get_time() - high_start;

            data[i / 8] <<= 1;
            if (duration > 40) {
                data[i / 8] |= 1;
            }
        }

        uint8_t checksum = data[0] + data[1] + data[2] + data[3];
        if (checksum != data[4]) return result;

        result.humidity = ((data[0] << 8) | data[1]) / 10.0f;

        int16_t temp_raw = ((data[2] & 0x7F) << 8) | data[3];
        result.temperature = temp_raw / 10.0f;
        if (data[2] & 0x80) result.temperature = -result.temperature;

        result.valid = true;
        return result;
    }