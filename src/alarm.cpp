    #include "alarm.h"
    #include "system_events.h"
    #include "rtos_objects.h"
    #include "driver/gpio.h"
    #include "driver/ledc.h"
    #include "freertos/task.h"

    #define BUZZER_PIN GPIO_NUM_25

    static QueueHandle_t alarmQueue;

    static void configure_buzzer_gpio(void) {
        ledc_timer_config_t timer_cfg = {};
        timer_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
        timer_cfg.duty_resolution = LEDC_TIMER_10_BIT;
        timer_cfg.timer_num = LEDC_TIMER_0;
        timer_cfg.freq_hz = 2000;
        timer_cfg.clk_cfg = LEDC_AUTO_CLK;
        ledc_timer_config(&timer_cfg);

        ledc_channel_config_t ch_cfg = {};
        ch_cfg.gpio_num = BUZZER_PIN;
        ch_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
        ch_cfg.channel = LEDC_CHANNEL_0;
        ch_cfg.timer_sel = LEDC_TIMER_0;
        ch_cfg.duty = 0;
        ch_cfg.hpoint = 0;
        ledc_channel_config(&ch_cfg);
    }

    void alarm_init(QueueHandle_t alarmQueueHandle) {
        alarmQueue = alarmQueueHandle;
        configure_buzzer_gpio();
    }

    void AlarmTask(void *pvParameters) {
        SensorData data;
        AlarmState lastState = AlarmState::NORMAL;

        while (true) {
            if (xQueueReceive(alarmQueue, &data, portMAX_DELAY) == pdTRUE) {
                AlarmState state = evaluateTemperature(data.temperature);

                if (state != lastState) {
                    xSemaphoreTake(serialMutex, portMAX_DELAY);
                    printf("[AlarmTask] Temp %.1f C -> state changed to %s\n",
                        data.temperature,
                        state == AlarmState::NORMAL ? "NORMAL" :
                        state == AlarmState::LOW_TEMPERATURE ? "LOW_TEMPERATURE" : "HIGH_TEMPERATURE");
                    xSemaphoreGive(serialMutex);
                    lastState = state;

                    if (state == AlarmState::NORMAL) {
                        xEventGroupClearBits(systemEvents, EVENT_ALARM);
                    } else {
                        xEventGroupSetBits(systemEvents, EVENT_ALARM);
                    }
                }

                if (state == AlarmState::NORMAL) {
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                } else {
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                }
            }
        }
    }