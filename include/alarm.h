#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_data.h"
#include "temperature_logic.h"

void alarm_init(QueueHandle_t alarmQueueHandle);
void AlarmTask(void *pvParameters);