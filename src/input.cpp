#include "input.h"
#include "rtos_objects.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "INPUT";

#define ENC_CLK  GPIO_NUM_32
#define ENC_DT   GPIO_NUM_33
#define ENC_SW   GPIO_NUM_25

static int last_clk = 1;

void input_init(void) {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << ENC_CLK) | (1ULL << ENC_DT) | (1ULL << ENC_SW);
    cfg.mode         = GPIO_MODE_INPUT;
    cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
    ESP_LOGI(TAG, "Encoder inputs initialized");
}

void input_task(void *pvParameters) {
    for (;;) {
        int clk = gpio_get_level(ENC_CLK);
        if (clk != last_clk && clk == 0) {
            int dt = gpio_get_level(ENC_DT);
            DisplayMode current = DisplayMode::TEMPERATURE;
            xQueuePeek(modeQueue, &current, 0);

            DisplayMode next = (dt == 0)
                ? nextDisplayMode(current)
                : previousDisplayMode(current);

            xQueueOverwrite(modeQueue, &next);
            ESP_LOGI(TAG, "ENCODER: %s", (dt == 0) ? "CW -> next" : "CCW -> prev");
        }
        last_clk = clk;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}