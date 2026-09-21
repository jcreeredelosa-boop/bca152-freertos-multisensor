#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "logic.h"

// -------- Shared SensorData struct --------
struct SensorData {
    float temperature;
    float humidity;
    int   lightLevel;      // 0 - 100 %
    bool  motionDetected;
};

// -------- FreeRTOS objects (defined in rtos_objects.cpp) --------
extern QueueHandle_t      sensorQueue;    // SensorTask -> DisplayTask
extern QueueHandle_t      alarmQueue;     // SensorTask -> AlarmTask
extern QueueHandle_t      modeQueue;      // InputTask  -> DisplayTask (len 1, overwrite)
extern SemaphoreHandle_t  serialMutex;    // protects Serial output
extern EventGroupHandle_t systemEvents;   // system-wide event flags

// -------- Event bits --------
#define EVENT_ACTIVE  BIT0
#define EVENT_MOTION  BIT1
#define EVENT_ALARM   BIT2

// -------- Shared tick of last motion (updated by MotionTask) --------
extern volatile TickType_t lastMotionTick;

// -------- Helpers --------
void rtos_objects_init();
void safe_print(const char *fmt, ...);