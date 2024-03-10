#include <Timer.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

void timer_sleep(uint16_t ms)
{
    #ifdef ESP_PLATFORM  // ESP-IDF
    vTaskDelay(pdMS_TO_TICKS(ms));
    #elif // ARDUINO platform
    delay(ms);
    #else
    #error "Platform not supported!"
    #endif
}