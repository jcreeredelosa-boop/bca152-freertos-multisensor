#pragma once

enum class DisplayMode {
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
};

DisplayMode nextDisplayMode(DisplayMode current);
DisplayMode previousDisplayMode(DisplayMode current);   