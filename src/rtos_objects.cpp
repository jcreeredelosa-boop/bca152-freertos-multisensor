#include "rtos_objects.h"
#include <cstdarg>
#include <cstdio>

QueueHandle_t      sensorQueue    = nullptr;
QueueHandle_t      alarmQueue     = nullptr;
QueueHandle_t      modeQueue      = nullptr;
SemaphoreHandle_t  serialMutex    = nullptr;
EventGroupHandle_t systemEvents   = nullptr;
volatile TickType_t lastMotionTick = 0;

void rtos_objects_init() {
    sensorQueue  = xQueueCreate(5, sizeof(SensorData));
    alarmQueue   = xQueueCreate(5, sizeof(SensorData));
    modeQueue    = xQueueCreate(1, sizeof(DisplayMode));
    serialMutex  = xSemaphoreCreateMutex();
    systemEvents = xEventGroupCreate();
}

void safe_print(const char *fmt, ...) {
    if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        xSemaphoreGive(serialMutex);
    }
}