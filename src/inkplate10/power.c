#include "power.h"
#include "fs.h"
#include "../esp32/spi.h"
#include "pcal_ex.h"
#include "display.h"
#include "esp_sleep.h"
// Pin on the internal io expander which controls MOSFET for turning on and off the SD card
#define SD_PMOS_PIN 10

void power_sleep(void) {
    fs_external_end();
    spi_end();
    display_sleep();
    pcal_ex_set_direction_int(SD_PMOS_PIN, INPUT);
    esp_deep_sleep_start();
    
}
