#if defined(INKPLATE10) || defined(INKPLATE10V2)
#include <memory.h>
#include "rtc_time.h"
#include "config.h"
#include "hardware.h"
#include "esp32/i2c.h"
#include "esp_check.h"
#include "esp_log.h"
#ifdef LEGACY_I2C
#include "driver/i2c.h"
#else
#include "driver/i2c_master.h"
#endif
static const char* TAG = "rtc_time";
#ifdef LEGACY_I2C
#define CMD_HANDLER_BUFFER_SIZE I2C_LINK_RECOMMENDED_SIZE(7)
static uint8_t cmdlink_buffer[CMD_HANDLER_BUFFER_SIZE];
#else
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t i2c_handle = NULL;
#endif
static bool i2c_write(uint8_t address, const void* to_write, size_t write_len) {
#ifdef LEGACY_I2C
    i2c_cmd_handle_t cmd_link = NULL;
    esp_err_t ret=ESP_OK;
    ret = ret;
    bool result =false;
    cmd_link = i2c_cmd_link_create_static(cmdlink_buffer, CMD_HANDLER_BUFFER_SIZE);
    
    ESP_GOTO_ON_ERROR(i2c_master_start(cmd_link), err, TAG, "i2c_master_start");
    ESP_GOTO_ON_ERROR(i2c_master_write_byte(cmd_link, (address << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK), err, TAG, "i2c_master_write_byte");
    if (write_len > 0) {
        ESP_GOTO_ON_ERROR(i2c_master_write(cmd_link, to_write, write_len, I2C_MASTER_ACK), err, TAG, "i2c_master_write");
    }
    ESP_GOTO_ON_ERROR(i2c_master_stop(cmd_link), err, TAG, "i2c_master_stop");
    ESP_GOTO_ON_ERROR(i2c_master_cmd_begin(I2C_PORT, cmd_link, pdMS_TO_TICKS(10000)), err, TAG, "i2c_master_cmd_begin");
    
    result = true;
err:
    if(ret!=ESP_OK) {
        ESP_LOGE(TAG,"Error: %s",esp_err_to_name(ret));
    }
    if (cmd_link != NULL) {
        // i2c_cmd_link_delete(cmd_link);
        i2c_cmd_link_delete_static(cmd_link);
    }
    return result;
#else
    if(write_len==0) {
        return true;
    }
    i2c_master_dev_handle_t handle = i2c_handle;
    esp_err_t ret = i2c_master_transmit(handle,to_write,write_len,1000);
    if(ESP_OK!=ret) {
        ESP_LOGE(TAG,"Error: %s",esp_err_to_name(ret));
        return false;
    }
    return true;
#endif
}
static bool i2c_read(uint8_t address, void* to_read, size_t read_len) {
#ifdef LEGACY_I2C
    i2c_cmd_handle_t cmd_link = NULL;
    esp_err_t ret = ESP_OK;
    ret = ret;
    bool result = false;
    if (read_len == 0) return true;
    cmd_link = i2c_cmd_link_create_static(cmdlink_buffer, CMD_HANDLER_BUFFER_SIZE);

    // i2c_cmd_handle_t cmd_link = i2c_cmd_link_create();
    ESP_GOTO_ON_ERROR(i2c_master_start(cmd_link), err, TAG, "i2c_master_start");
    ESP_GOTO_ON_ERROR(i2c_master_write_byte(cmd_link, (address << 1) | I2C_MASTER_READ,0), err, TAG, "i2c_master_write_byte");

    if (read_len > 1) {
        ESP_GOTO_ON_ERROR(i2c_master_read(cmd_link, to_read, read_len - 1, I2C_MASTER_NACK), err, TAG, "i2c_master_read");
    }
    ESP_GOTO_ON_ERROR(i2c_master_read_byte(cmd_link, to_read + read_len - 1, I2C_MASTER_LAST_NACK), err, TAG, "i2c_master_read_byte");

    ESP_GOTO_ON_ERROR(i2c_master_stop(cmd_link), err, TAG, "i2c_master_stop");

    ESP_GOTO_ON_ERROR(i2c_master_cmd_begin(I2C_PORT, cmd_link, pdMS_TO_TICKS(10000)), err, TAG, "i2c_master_cmd_begin");
    result = true;
err:
    if(ret!=ESP_OK) {
        ESP_LOGE(TAG,"Error: %s",esp_err_to_name(ret));
    }    
    if (cmd_link != NULL) {
        // i2c_cmd_link_delete(cmd_link);
        i2c_cmd_link_delete_static(cmd_link);
    }
    return result;
#else
    i2c_master_dev_handle_t handle = i2c_handle;
    esp_err_t ret = i2c_master_receive(handle,to_read,read_len,1000);
    if(ESP_OK!=ret) {
        ESP_LOGE(TAG,"Error: %s",esp_err_to_name(ret));
        return false;
    }
    return true;
#endif
}
static bool i2c_write_read(uint8_t address, const void *to_write, size_t write_len, void *to_read, size_t read_len) {
#ifdef LEGACY_I2C
    if (i2c_write(address, to_write, write_len)) {
        return i2c_read(address, to_read, read_len);
    }
#else
    if(ESP_OK==i2c_master_transmit_receive(i2c_handle,to_write,write_len,to_read,read_len,1000)) {
        return true;
    }
#endif
    return false;
}

static uint8_t dec_to_bcd(uint8_t val)
{
    return ((val / 10 * 16) + (val % 10));
}

static uint8_t bcd_to_dec(uint8_t val)
{
    return ((val / 16 * 10) + (val % 16));
}
bool rtc_time_init(void) {
    if(!i2c_init()) {
        return false;
    }
#ifndef LEGACY_I2C
    if(ESP_OK!=i2c_master_get_bus_handle((i2c_port_num_t)I2C_PORT,&i2c_bus_handle)) {
        return false;
    }
    i2c_device_config_t dev_cfg;
    memset(&dev_cfg,0,sizeof(dev_cfg));
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.flags.disable_ack_check = false;
    dev_cfg.scl_speed_hz = 100*1000;
    dev_cfg.scl_wait_us = 0;
    dev_cfg.device_address = 0x51;
    if(ESP_OK!=i2c_master_bus_add_device(i2c_bus_handle,&dev_cfg,&i2c_handle)) {
        i2c_bus_handle = NULL;
        return false;
    }
#endif
    if(!i2c_write(0x51,NULL,0)) {
        ESP_LOGE(TAG,"Unable to find RTC device");
#ifndef LEGACY_I2C
        i2c_master_bus_rm_device(i2c_handle);
        i2c_handle=NULL;
        i2c_bus_handle = NULL;
#endif
        return false;
    }
    return true;
}
bool rtc_time_now(struct tm* out_tm) {
    
    uint8_t out = 0x04;
    uint8_t in[7];
    if(!i2c_write_read(0x51,&out,1,in,sizeof(in))) {
        ESP_LOGE(TAG,"Could not read time");
        return false;
    }
    out_tm->tm_sec = bcd_to_dec(in[0] & 0x7F);
    out_tm->tm_min = bcd_to_dec(in[1] & 0x7F);
    out_tm->tm_hour = bcd_to_dec(in[2] & 0x3F);
    out_tm->tm_mday = bcd_to_dec(in[3] & 0x3F);
    out_tm->tm_wday = bcd_to_dec(in[4] & 0x07);
    out_tm->tm_mon = bcd_to_dec(in[5] & 0x1F) - 1;
    out_tm->tm_year = bcd_to_dec(in[6]) + 2000 - 1900;
    return true;
}

bool rtc_time_set(const struct tm* time) {
    
    uint8_t out[] = {0x03, 170,
        dec_to_bcd(time->tm_sec),
        dec_to_bcd(time->tm_min),
        dec_to_bcd(time->tm_hour),
        dec_to_bcd(time->tm_mday),
        dec_to_bcd(time->tm_wday),
        dec_to_bcd(time->tm_mon+1),
        dec_to_bcd(time->tm_year+1900-2000),
    };
    if(!i2c_write(0x51,out,sizeof(out))) {
        ESP_LOGE(TAG,"Could not set time");
        return false;
    }
    return true;
}
bool rtc_time_set_tz_offset(long offset) {
    config_clear_values("tzoffset");
    char tmp[32];
    snprintf(tmp,31,"%ld",offset);
    return config_add_value("tzoffset",tmp);
}
bool rtc_time_get_tz_offset(long* out_offset) {
    char tmp[32];
    if(config_get_value("tzoffset",0,tmp,sizeof(tmp))) {
        sscanf(tmp,"%ld",out_offset);
        return true;
    }
    *out_offset=0;
    return false;
}
#endif