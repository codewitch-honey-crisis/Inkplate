#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtc_wdt.h"
#include "esp_log.h"
extern void run(void);
extern void loop(void);
void app_main(void) {
    esp_log_level_set("*", ESP_LOG_NONE);
    run();
    uint32_t ts = 0;
    while (1) {
        loop();
        rtc_wdt_feed();
        uint32_t ms = pdTICKS_TO_MS(xTaskGetTickCount());
        if (ms > ts + 200) {
            ms = pdTICKS_TO_MS(xTaskGetTickCount());
            
        }
    }
}
#endif