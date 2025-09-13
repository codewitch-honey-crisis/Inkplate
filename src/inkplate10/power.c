#include "power.h"
#include "fs.h"
#include "../esp32/spi.h"
#include "pcal_ex.h"
#include "timing.h"
#include "display.h"
#include "esp_sleep.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// GPIO 35 is ADC1 Channel 7
#define ADC1_CHAN7_GPIO35    ADC_CHANNEL_7
#define ADC_ATTEN           ADC_ATTEN_DB_12 // would be 11 but that's deprecated

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;

static bool adc_batt_init(void)
{
    if(adc1_handle!=NULL) {
        return true;
    }
    // Initialize ADC1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    if(ESP_OK!=adc_oneshot_new_unit(&init_config1, &adc1_handle)) {
        return false;
    }

    // Configure ADC1 channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    if(ESP_OK!=adc_oneshot_config_channel(adc1_handle, ADC1_CHAN7_GPIO35, &config)) {
        adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
        return false;
    }

    // Initialize calibration (ESP32 uses line fitting scheme)
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if(ESP_OK!=adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle)) {
        adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
        return false;
    }
    return true;
}

static int adc_batt_read_millivolts(void)
{
    if(!adc_batt_init()) {
        return 0;
    }
    int voltage;
    ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(adc1_handle, adc1_cali_handle, ADC1_CHAN7_GPIO35, &voltage));
    return voltage;
}
// Pin on the internal io expander which controls MOSFET for turning on and off the SD card
#define SD_PMOS_PIN 10
float power_battery_voltage(void) {
        // Read the pin on the battery MOSFET. If is high, that means is older version of the board
    // that uses PMOS only. If it's low, newer board with both PMOS and NMOS.
    if(!pcal_ex_set_direction_int(9, INPUT)) {
        return 0;
    }
    int state = pcal_ex_get_level_int(9);
    pcal_ex_set_direction_int(9, OUTPUT);

    // If the input is pulled high, it's PMOS only.
    // If it's pulled low, it's PMOS and NMOS.
    if (state)
    {
        pcal_ex_set_level_int(9, LOW);
    }
    else
    {
        pcal_ex_set_level_int(9, HIGH);
    }

    // Wait a little bit after a MOSFET enable.
    timing_delay_ms(5);
    
    // Set to the highest resolution and read the voltage.
     
    int adc = adc_batt_read_millivolts();

    // Turn off the MOSFET (and voltage divider).
    if (state)
    {
        pcal_ex_set_level_int(9, HIGH);
    }
    else
    {
        pcal_ex_set_level_int(9, LOW);
    }

    // Calculate the voltage at the battery terminal (voltage is divided in half by voltage divider).
    const float result = ((float)adc * 2.0f / 1000.f);
    // the thing returns a massive voltage when there's no battery. (TODO: test all of this battery code)
    if(result>4.f) {
        return 0.f;
    }
    return result;
}
float power_battery_level(void) {
    const float volts = power_battery_voltage();
    if(volts==0.f) {
        return 0.f;
    }
    const float result =
        (volts < 3.248088) ? (0) : (volts - 3.120712) ;
    return (result <= 1) ? result : 1;
}
void power_sleep(void) {
    fs_external_end();
    spi_end();
    display_sleep();
    pcal_ex_set_direction_int(SD_PMOS_PIN, INPUT);
    esp_deep_sleep_start();
    
}
