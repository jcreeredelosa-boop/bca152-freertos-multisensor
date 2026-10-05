#pragma once

#define LOW_TEMPERATURE_LIMIT  18.0f
#define HIGH_TEMPERATURE_LIMIT 30.0f

enum class AlarmState {
    NORMAL,
    LOW_TEMPERATURE,
    HIGH_TEMPERATURE
};

AlarmState evaluateTemperature(float temperature);