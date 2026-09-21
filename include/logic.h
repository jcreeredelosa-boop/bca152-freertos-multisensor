#pragma once

// ---------------- Enums ----------------
enum class AlarmState   { NORMAL, LOW_TEMPERATURE, HIGH_TEMPERATURE };
enum class DisplayMode  { TEMPERATURE, HUMIDITY, LIGHT, MOTION };
enum class SystemState  { ACTIVE, INACTIVE };

// ---------------- Temperature thresholds ----------------
constexpr float LOW_TEMP_LIMIT  = 18.0f;
constexpr float HIGH_TEMP_LIMIT = 30.0f;

// ---------------- Pure logic functions ----------------
inline AlarmState evaluateTemperature(float temperature) {
    if (temperature < LOW_TEMP_LIMIT)  return AlarmState::LOW_TEMPERATURE;
    if (temperature > HIGH_TEMP_LIMIT) return AlarmState::HIGH_TEMPERATURE;
    return AlarmState::NORMAL;
}

inline DisplayMode nextDisplayMode(DisplayMode m) {
    switch (m) {
        case DisplayMode::TEMPERATURE: return DisplayMode::HUMIDITY;
        case DisplayMode::HUMIDITY:    return DisplayMode::LIGHT;
        case DisplayMode::LIGHT:       return DisplayMode::MOTION;
        case DisplayMode::MOTION:      return DisplayMode::TEMPERATURE;
    }
    return DisplayMode::TEMPERATURE;
}

inline DisplayMode previousDisplayMode(DisplayMode m) {
    switch (m) {
        case DisplayMode::TEMPERATURE: return DisplayMode::MOTION;
        case DisplayMode::HUMIDITY:    return DisplayMode::TEMPERATURE;
        case DisplayMode::LIGHT:       return DisplayMode::HUMIDITY;
        case DisplayMode::MOTION:      return DisplayMode::LIGHT;
    }
    return DisplayMode::TEMPERATURE;
}

inline SystemState evaluateSystemState(SystemState current,
                                       bool motionDetected,
                                       bool inactivityTimedOut) {
    if (current == SystemState::INACTIVE) {
        return motionDetected ? SystemState::ACTIVE : SystemState::INACTIVE;
    }
    // current == ACTIVE
    return inactivityTimedOut ? SystemState::INACTIVE : SystemState::ACTIVE;
}