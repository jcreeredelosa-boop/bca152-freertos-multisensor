#include "input.h"
#include "rtos_objects.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "INPUT";
#define ENC_A  GPIO_NUM_32
#define ENC_B  GPIO_NUM_33
#define ENC_SW GPIO_NUM_25

static DisplayMode currentMode = DisplayMode::TEMPERATURE;
static int lastAB = 0;

void input_init() {
    gpio_set_direction(ENC_A,  GPIO_MODE_INPUT);
    gpio_set_direction(ENC_B,  GPIO_MODE_INPUT);
    gpio_set_direction(ENC_SW, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ENC_A,  GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_B,  GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_SW, GPIO_PULLUP_ONLY);
    lastAB = (gpio_get_level(ENC_A) << 1) | gpio_get_level(ENC_B);
}

void input_task(void *pvParameters) {
    // Publish initial mode
    xQueueOverwrite(modeQueue, &currentMode);

    for (;;) {
        int a = gpio_get_level(ENC_A);
        int b = gpio_get_level(ENC_B);
        int ab = (a << 1) | b;

        if (ab != lastAB) {
            // Standard quadrature decode
            if ((lastAB == 0b00 && ab == 0b01) ||
                (lastAB == 0b01 && ab == 0b11) ||
                (lastAB == 0b11 && ab == 0b10) ||
                (lastAB == 0b10 && ab == 0b00)) {
                currentMode = nextDisplayMode(currentMode);
                ESP_LOGI(TAG, "CW -> mode %d", (int)currentMode);
            } else {
                currentMode = previousDisplayMode(currentMode);
                ESP_LOGI(TAG, "CCW -> mode %d", (int)currentMode);
            }
            xQueueOverwrite(modeQueue, &currentMode);
            lastAB = ab;
        }

       vTaskDelay(1);   // 1 tick = 10 ms at 100 Hz. Guaranteed to actually block.
    }
}