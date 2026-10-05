#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// EVENT_ACTIVE: set = system ACTIVE, cleared = INACTIVE pre.
//   Producer: MotionTask (kung naa PIR activity / 15s inactivity timeout)
//   Consumer: DisplayTask (mupalong o mubalik ang OLED display)
#define EVENT_ACTIVE (1 << 0)

// EVENT_MOTION: set kung ang PIR kay HIGH, cleared filter kung dili.
//   Producer: MotionTask (gatuyok poll every 500ms)
//   Consumer: SensorTask (mubasa ani para mapuno ang SensorData.motionDetected)
#define EVENT_MOTION (1 << 1)

// EVENT_ALARM: set pag ang temperature state sa AlarmTask kay LOW_TEMPERATURE o
// HIGH_TEMPERATURE; cleared pag nibalik na sa NORMAL bai.
//   Producer: AlarmTask (basta mag-state transition)
//   Consumer: DisplayTask (pakita ug "*" marker sa OLED header habang active pa)
#define EVENT_ALARM  (1 << 2)

extern EventGroupHandle_t systemEvents;