#include "timing.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint32_t timing_get_ms() {
    return esp_timer_get_time() / 1000;
}
void IRAM_ATTR timing_delay_us(uint32_t us) {
    uint64_t m = esp_timer_get_time();
    if (us > 2) {
        uint64_t e = m + us;
        if (m > e) {
            while (esp_timer_get_time() > e) asm volatile("nop");  // overflow...
        }
        while (esp_timer_get_time() < e) asm volatile("nop");
    }
}
void timing_delay_ms(uint32_t ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
    uint32_t remainder_usec = (ms % portTICK_PERIOD_MS) * 1000;
    if (remainder_usec) timing_delay_us(remainder_usec);
}
