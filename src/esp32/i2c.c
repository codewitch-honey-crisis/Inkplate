#include "i2c.h"
#include <stdbool.h>
#include <memory.h>
#include "hardware.h"
#include "driver/i2c.h"
#include "esp_log.h"
static const char* TAG = "i2c_layer";
static bool bus_initialized = false;
bool i2c_init(void) {
    if(bus_initialized) return true;
    
    i2c_config_t cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    cfg.master.clk_speed = 100*1000;
    cfg.mode = I2C_MODE_MASTER;
    cfg.sda_io_num = I2C_SDA;
    cfg.sda_pullup_en = false;
    cfg.scl_io_num = I2C_SCL;
    cfg.scl_pullup_en = false;
    if(ESP_OK!=i2c_param_config(I2C_PORT,&cfg)) {
        ESP_LOGE(TAG,"Could not set I2C parameters");
        return false;
    }
    if(ESP_OK!=i2c_set_pin(I2C_PORT,I2C_SDA,I2C_SCL,false,false,I2C_MODE_MASTER)) {
        ESP_LOGE(TAG,"Could not set I2C pins");
        return false;
    }
    if(ESP_OK!=i2c_driver_install(I2C_PORT,I2C_MODE_MASTER,0,0,0)) {
        ESP_LOGE(TAG,"Could not install I2C driver");
        return false;
    }
    bus_initialized = true;
    return true;
}