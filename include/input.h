#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "display_logic.h"

void input_init(QueueHandle_t modeQueueHandle);
void InputTask(void *pvParameters);