#ifdef ESP_PLATFORM
#include "i2c.h"
#include <stdbool.h>
#include <memory.h>
#include "hardware.h"
#ifdef LEGACY_I2C    
#include "driver/i2c.h"
static bool bus_initialized = false;
#else
#include "driver/i2c_master.h"
static i2c_master_bus_handle_t i2c_handle = NULL;
#endif
#include "esp_log.h"
static const char* TAG = "i2c_layer";

bool i2c_init(void) {
#ifdef LEGACY_I2C    
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
#else
    if(i2c_handle!=NULL) {
        return true;
    }
    i2c_master_bus_config_t cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.i2c_port = I2C_PORT;
    cfg.sda_io_num = I2C_SDA;
    cfg.scl_io_num = I2C_SCL;
    cfg.flags.enable_internal_pullup = true;
    cfg.trans_queue_depth = 10;
    if(ESP_OK!=i2c_new_master_bus(&cfg,&i2c_handle)) {
        ESP_LOGE(TAG,"Could not install I2C driver");
        i2c_handle = NULL;
        return false;
    }
#endif
    
    return true;

}
#endif