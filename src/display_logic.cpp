#include "display_logic.h"

DisplayMode nextDisplayMode(DisplayMode current) {
    switch (current) {
        case DisplayMode::TEMPERATURE: return DisplayMode::HUMIDITY;
        case DisplayMode::HUMIDITY:    return DisplayMode::LIGHT;
        case DisplayMode::LIGHT:       return DisplayMode::MOTION;
        case DisplayMode::MOTION:      return DisplayMode::TEMPERATURE;
    }
    return DisplayMode::TEMPERATURE;
}

DisplayMode previousDisplayMode(DisplayMode current) {
    switch (current) {
        case DisplayMode::TEMPERATURE: return DisplayMode::MOTION;
        case DisplayMode::HUMIDITY:    return DisplayMode::TEMPERATURE;
        case DisplayMode::LIGHT:       return DisplayMode::HUMIDITY;
        case DisplayMode::MOTION:      return DisplayMode::LIGHT;
    }
    return DisplayMode::TEMPERATURE;
}