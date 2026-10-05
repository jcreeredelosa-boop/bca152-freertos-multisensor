#include "motion_logic.h"

SystemState evaluateSystemState(bool motionDetectedNow, uint32_t msSinceLastMotion, uint32_t timeoutMs) {
    if (motionDetectedNow) return SystemState::ACTIVE;
    if (msSinceLastMotion >= timeoutMs) return SystemState::INACTIVE;
    return SystemState::ACTIVE;
}