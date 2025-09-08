#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtc_wdt.h"
extern void run(void);
extern void loop(void);
static void loop_task(void* arg) {
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
void app_main(void) {
    run();
    TaskHandle_t handle;
    xTaskCreate(loop_task,"loop_task",8192,NULL,20,&handle);
}