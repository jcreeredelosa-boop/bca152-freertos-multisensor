#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");

    // TODO: Create Queues, Mutexes, Event Groups
    
    // TODO: Create Tasks (SensorTask, DisplayTask, etc.)

    while (true) {
        // Main task can block or do housekeeping
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
