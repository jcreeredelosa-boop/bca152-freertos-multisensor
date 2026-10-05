#pragma once
#include <stdint.h>

enum class SystemState {
    ACTIVE,
    INACTIVE
};

SystemState evaluateSystemState(bool motionDetectedNow, uint32_t msSinceLastMotion, uint32_t timeoutMs);