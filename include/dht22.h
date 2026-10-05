#pragma once
#include "driver/gpio.h"

typedef struct {
    float temperature;
    float humidity;
    bool valid;
} dht22_reading_t;

dht22_reading_t dht22_read(gpio_num_t pin);