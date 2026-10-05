#include "input.h"
#include "driver/gpio.h"
#include "freertos/task.h"

#define ENCODER_CLK GPIO_NUM_32
#define ENCODER_DT  GPIO_NUM_33

static QueueHandle_t encoderQueue;
static QueueHandle_t modeQueue;

static void IRAM_ATTR encoder_isr_handler(void *arg) {
    int8_t direction = gpio_get_level(ENCODER_DT) ? 1 : -1;
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(encoderQueue, &direction, &woken);
    if (woken) portYIELD_FROM_ISR();
}

static void configure_encoder_gpio(void) {
    gpio_config_t clk_conf = {};
    clk_conf.pin_bit_mask = 1ULL << ENCODER_CLK;
    clk_conf.mode = GPIO_MODE_INPUT;
    clk_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    clk_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&clk_conf);

    gpio_config_t dt_conf = {};
    dt_conf.pin_bit_mask = 1ULL << ENCODER_DT;
    dt_conf.mode = GPIO_MODE_INPUT;
    dt_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&dt_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_CLK, encoder_isr_handler, NULL);
}

void input_init(QueueHandle_t modeQueueHandle) {
    modeQueue = modeQueueHandle;
    encoderQueue = xQueueCreate(10, sizeof(int8_t));
    configure_encoder_gpio();
}

void InputTask(void *pvParameters) {
    static DisplayMode currentMode = DisplayMode::TEMPERATURE;
    int8_t direction;
    while (true) {
        if (xQueueReceive(encoderQueue, &direction, portMAX_DELAY) == pdTRUE) {
            currentMode = (direction > 0)
                ? nextDisplayMode(currentMode)
                : previousDisplayMode(currentMode);
            xQueueOverwrite(modeQueue, &currentMode);
        }
    }
}