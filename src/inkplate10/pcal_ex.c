#if defined(INKPLATE10) || defined(INKPLATE10V2)
#include "pcal_ex.h"

#include <memory.h>

#include "../esp32/i2c.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware.h"
#ifdef LEGACY_I2C
#include "driver/i2c.h"
#else
#include "driver/i2c_master.h"
#endif
static const char* TAG = "gpio_ext";

// PCAL6416 Register Adresses
#define PCAL6416A_INPORT0        0x00
#define PCAL6416A_INPORT1        0x01
#define PCAL6416A_OUTPORT0       0x02
#define PCAL6416A_OUTPORT1       0x03
#define PCAL6416A_POLINVPORT0    0x04
#define PCAL6416A_POLINVPORT1    0x05
#define PCAL6416A_CFGPORT0       0x06
#define PCAL6416A_CFGPORT1       0x07
#define PCAL6416A_OUTDRVST_REG00 0x40
#define PCAL6416A_OUTDRVST_REG01 0x41
#define PCAL6416A_OUTDRVST_REG10 0x42
#define PCAL6416A_OUTDRVST_REG11 0x43
#define PCAL6416A_INLAT_REG0     0x44
#define PCAL6416A_INLAT_REG1     0x45
#define PCAL6416A_PUPDEN_REG0    0x46
#define PCAL6416A_PUPDEN_REG1    0x47
#define PCAL6416A_PUPDSEL_REG0   0x48
#define PCAL6416A_PUPDSEL_REG1   0x49
#define PCAL6416A_INTMSK_REG0    0x4A
#define PCAL6416A_INTMSK_REG1    0x4B
#define PCAL6416A_INTSTAT_REG0   0x4C
#define PCAL6416A_INTSTAT_REG1   0x4D
#define PCAL6416A_OUTPORT_CONF   0x4F

// PCAL6416 register index array
#define PCAL6416A_INPORT0_ARRAY        0
#define PCAL6416A_INPORT1_ARRAY        1
#define PCAL6416A_OUTPORT0_ARRAY       2
#define PCAL6416A_OUTPORT1_ARRAY       3
#define PCAL6416A_POLINVPORT0_ARRAY    4
#define PCAL6416A_POLINVPORT1_ARRAY    5
#define PCAL6416A_CFGPORT0_ARRAY       6
#define PCAL6416A_CFGPORT1_ARRAY       7
#define PCAL6416A_OUTDRVST_REG00_ARRAY 8
#define PCAL6416A_OUTDRVST_REG01_ARRAY 9
#define PCAL6416A_OUTDRVST_REG10_ARRAY 10
#define PCAL6416A_OUTDRVST_REG11_ARRAY 11
#define PCAL6416A_INLAT_REG0_ARRAY     12
#define PCAL6416A_INLAT_REG1_ARRAY     13
#define PCAL6416A_PUPDEN_REG0_ARRAY    14
#define PCAL6416A_PUPDEN_REG1_ARRAY    15
#define PCAL6416A_PUPDSEL_REG0_ARRAY   16
#define PCAL6416A_PUPDSEL_REG1_ARRAY   17
#define PCAL6416A_INTMSK_REG0_ARRAY    18
#define PCAL6416A_INTMSK_REG1_ARRAY    19
#define PCAL6416A_INTSTAT_REG0_ARRAY   20
#define PCAL6416A_INTSTAT_REG1_ARRAY   21
#define PCAL6416A_OUTPORT_CONF_ARRAY   22

#ifdef INKPLATE10
#define IO_INT_ADDR 0x20
#define IO_EXT_ADDR 0x22
#endif

#ifdef INKPLATE10V2
#define IO_INT_ADDR 0x20
#define IO_EXT_ADDR 0x21
#endif

static const uint8_t regAddresses[23] = {
        PCAL6416A_INPORT0,        PCAL6416A_INPORT1,        PCAL6416A_OUTPORT0,       PCAL6416A_OUTPORT1,
        PCAL6416A_POLINVPORT0,    PCAL6416A_POLINVPORT1,    PCAL6416A_CFGPORT0,       PCAL6416A_CFGPORT1,
        PCAL6416A_OUTDRVST_REG00, PCAL6416A_OUTDRVST_REG01, PCAL6416A_OUTDRVST_REG10, PCAL6416A_OUTDRVST_REG11,
        PCAL6416A_INLAT_REG0,     PCAL6416A_INLAT_REG1,     PCAL6416A_PUPDEN_REG0,    PCAL6416A_PUPDEN_REG1,
        PCAL6416A_PUPDSEL_REG0,   PCAL6416A_PUPDSEL_REG1,   PCAL6416A_INTMSK_REG0,    PCAL6416A_INTMSK_REG1,
        PCAL6416A_INTSTAT_REG0,   PCAL6416A_INTSTAT_REG1,   PCAL6416A_OUTPORT_CONF};

uint8_t ioRegsInt[23], ioRegsEx[23];
static bool initialized = false;
#ifdef LEGACY_I2C
#define CMD_HANDLER_BUFFER_SIZE I2C_LINK_RECOMMENDED_SIZE(7)
static uint8_t cmdlink_buffer[CMD_HANDLER_BUFFER_SIZE];
#else
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t i2c_handle_int = NULL;
static i2c_master_dev_handle_t i2c_handle_ext = NULL;
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
    i2c_master_dev_handle_t i2c_handle = (address==IO_INT_ADDR)?i2c_handle_int:i2c_handle_ext;
    esp_err_t ret = i2c_master_transmit(i2c_handle,to_write,write_len,1000);
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
    i2c_master_dev_handle_t i2c_handle = (address==IO_INT_ADDR)?i2c_handle_int:i2c_handle_ext;
    esp_err_t ret = i2c_master_receive(i2c_handle,to_read,read_len,1000);
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
    i2c_master_dev_handle_t i2c_handle = (address==IO_INT_ADDR)?i2c_handle_int:i2c_handle_ext;
    if(ESP_OK==i2c_master_transmit_receive(i2c_handle,to_write,write_len,to_read,read_len,1000)) {
        return true;
    }
#endif
    return false;
}

static bool read_registers(uint8_t addr,uint8_t *k)
{
    uint8_t tmp = 0;
    if(!i2c_write_read(addr,&tmp,1,k,23)) {
        ESP_LOGE(TAG, "Could not read registers");
        return false;
    }
    return true;
}
bool read_register(uint8_t addr, uint8_t reg, uint8_t *k)
{
    if(!i2c_write_read(addr,&regAddresses[reg],1,k,1)) {
        ESP_LOGE(TAG, "Could not read register");
        return false;
    }
    return true;
}

// static bool update_all_registers(uint8_t addr, uint8_t *k)
// {
//     uint8_t tmp[24];
//     memcpy(tmp+1,k,23);
//     tmp[0]=0;
//     if(!i2c_write(addr,tmp,sizeof(tmp))) {
//         ESP_LOGE(TAG, "Could not write registers");
//         return false;
//     }
//     return true;
// }
static bool update_register(uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t tmp[2];
    tmp[0] = regAddresses[reg];
    tmp[1] = value;
    if (!i2c_write(addr,tmp, sizeof(tmp))) {
        ESP_LOGE(TAG, "Could not write register");
        return false;
    }
    return true;
}
// static bool update_register2(uint8_t addr, uint8_t reg, uint8_t* k, uint8_t count) {
//     uint8_t tmp[24];
//     tmp[0] = regAddresses[reg];
//     memcpy(tmp+1,&k[reg],count);
//     if (!i2c_write(addr,tmp, sizeof(tmp))) {
//         ESP_LOGE(TAG, "Could not write register");
//         return false;
//     }
//     return true;
// }
static uint8_t *get_internal_registers(uint8_t addr)
{
    return (addr == IO_INT_ADDR ? ioRegsInt : ioRegsEx);
}
// do not use with 0-8! can damage device
static bool set_level_internal(uint8_t _addr, uint8_t *_r, uint8_t _pin, uint8_t _state)
{
    if(!initialized) return false;
    if (_pin > 15)
        return false;
    _state &= 1;

    uint8_t _port = _pin / 8;
    _pin %= 8;
    if(_state) {
        _r[PCAL6416A_OUTPORT0_ARRAY + _port] |= (1 << _pin);
    } else {
        _r[PCAL6416A_OUTPORT0_ARRAY + _port] &= ~(1 << _pin);
    }
    return update_register(_addr, PCAL6416A_OUTPORT0_ARRAY + _port, _r[PCAL6416A_OUTPORT0_ARRAY + _port]);
}
static int get_level_internal(uint8_t _addr, uint8_t *_r, uint8_t _pin)
{
    if(!initialized) return -1;
    if (_pin > 15)
        return -1;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    if(!read_register(_addr, PCAL6416A_INPORT0_ARRAY + _port, &_r[PCAL6416A_INPORT0_ARRAY + _port])) {
        return -1;
    }
    return ((_r[PCAL6416A_INPORT0_ARRAY + _port] >> _pin) & 1);
}
int get_int_internal(uint8_t _addr, uint8_t *_r)
{
    if(!read_register(_addr, PCAL6416A_INTSTAT_REG0_ARRAY, &_r[PCAL6416A_INTSTAT_REG0_ARRAY])) {
        return -1;
    }
    if(!read_register(_addr, PCAL6416A_INTSTAT_REG1_ARRAY, &_r[PCAL6416A_INTSTAT_REG1_ARRAY])) {
        return -1;
    }

    return ((_r[PCAL6416A_INTSTAT_REG1_ARRAY] << 8) | (_r[PCAL6416A_INTSTAT_REG0_ARRAY]));
}
static bool set_int_pin_internal(uint8_t addr, uint8_t *_r, uint8_t _pin)
{
    if(!initialized) return false;
    if (_pin > 15)
        return false;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    _r[PCAL6416A_INTMSK_REG0_ARRAY + _port] &= ~(1 << _pin);

    return update_register(addr, PCAL6416A_INTMSK_REG0_ARRAY + _port, _r[PCAL6416A_INTMSK_REG0_ARRAY + _port]);
}
static bool clear_int_pin_internal(uint8_t _addr, uint8_t *_r, uint8_t _pin)
{
    if(!initialized) return false;
    if (_pin > 15)
        return false;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    _r[PCAL6416A_INTMSK_REG0_ARRAY + _port] |= (1 << _pin);

    return update_register(_addr, PCAL6416A_INTMSK_REG0_ARRAY + _port, _r[PCAL6416A_INTMSK_REG0_ARRAY + _port]);
}

static bool set_direction_internal(uint8_t addr, uint8_t *_r, uint8_t _pin, uint8_t _mode)
{
    if(!initialized) return false;
    if (_pin > 15)
        return false;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    switch (_mode)
    {
    case INPUT:
        _r[PCAL6416A_CFGPORT0_ARRAY + _port] |= (1 << _pin);
        if(!update_register(addr, PCAL6416A_CFGPORT0_ARRAY + _port, _r[PCAL6416A_CFGPORT0_ARRAY + _port])) {
            return false;
        }
        break;
    case OUTPUT:
        // There is a one cacth! Pins are by default (POR) set as HIGH. So first change it to LOW and then set is as
        // output).
        _r[PCAL6416A_CFGPORT0_ARRAY + _port] &= ~(1 << _pin);
        _r[PCAL6416A_OUTPORT0_ARRAY + _port] &= ~(1 << _pin);
        if(!update_register(addr, PCAL6416A_OUTPORT0_ARRAY + _port, _r[PCAL6416A_OUTPORT0_ARRAY + _port])) {
            return false;
        }
        if(!update_register(addr, PCAL6416A_CFGPORT0_ARRAY + _port, _r[PCAL6416A_CFGPORT0_ARRAY + _port])) {
            return false;
        }
        break;
    case INPUT_PULLUP:
        _r[PCAL6416A_CFGPORT0_ARRAY + _port] |= (1 << _pin);
        _r[PCAL6416A_PUPDEN_REG0_ARRAY + _port] |= (1 << _pin);
        _r[PCAL6416A_PUPDSEL_REG0_ARRAY + _port] |= (1 << _pin);
        if(!update_register(addr, PCAL6416A_CFGPORT0_ARRAY + _port, _r[PCAL6416A_CFGPORT0_ARRAY + _port])) {
            return false;
        }
        if(!update_register(addr, PCAL6416A_PUPDEN_REG0_ARRAY + _port, _r[PCAL6416A_PUPDEN_REG0_ARRAY + _port])) {
            return false;
        }
        if(!update_register(addr, PCAL6416A_PUPDSEL_REG0_ARRAY + _port, _r[PCAL6416A_PUPDSEL_REG0_ARRAY + _port])) {
            return false;
        }
        break;
    case INPUT_PULLDOWN:
        _r[PCAL6416A_CFGPORT0_ARRAY + _port] |= (1 << _pin);
        _r[PCAL6416A_PUPDEN_REG0_ARRAY + _port] |= (1 << _pin);
        _r[PCAL6416A_PUPDSEL_REG0_ARRAY + _port] &= ~(1 << _pin);
        if(!update_register(addr, PCAL6416A_CFGPORT0_ARRAY + _port, _r[PCAL6416A_CFGPORT0_ARRAY + _port])) {
            return false;
        }
        if(!update_register(addr, PCAL6416A_PUPDEN_REG0_ARRAY + _port, _r[PCAL6416A_PUPDEN_REG0_ARRAY + _port])) {
            return false;
        }
        if(!update_register(addr, PCAL6416A_PUPDSEL_REG0_ARRAY + _port, _r[PCAL6416A_PUPDSEL_REG0_ARRAY + _port])) {
            return false;
        }
        break;
    }
    return true;
}
static int get_port_internal(uint8_t _addr, uint8_t *_r)
{
    if(!read_register(_addr, PCAL6416A_INPORT0_ARRAY, &_r[PCAL6416A_INPORT0_ARRAY])) {
        return -1;
    }
    if(!read_register(_addr, PCAL6416A_INPORT1_ARRAY, &_r[PCAL6416A_INPORT1_ARRAY])){ 
        return -1;
    }

    return (_r[PCAL6416A_INPORT0_ARRAY] | (_r[PCAL6416A_INPORT1_ARRAY]) << 8);
}
static bool set_port_internal(uint8_t _addr, uint8_t *_r, uint16_t _d)
{
    _r[PCAL6416A_OUTPORT0_ARRAY] = (_d & 0xff);
    _r[PCAL6416A_OUTPORT1_ARRAY] = (_d >> 8) & 0xff;
    if(!update_register(_addr, PCAL6416A_OUTPORT0_ARRAY, _r[PCAL6416A_OUTPORT0_ARRAY])) {
        return false;
    }
    return update_register(_addr, PCAL6416A_OUTPORT1_ARRAY, _r[PCAL6416A_OUTPORT1_ARRAY]);
}
static bool io_init(uint8_t _addr, uint8_t *_r)
{
    if(!read_registers(_addr, _r)) {
        ESP_LOGE(TAG, "Could not initialize GPIO extender device. Error reading registers");
        return false;
    }
    return true;
}
static bool set_direction_io(uint8_t _pin, uint8_t _mode, uint8_t _ioID)
{
    // if ((_ioID == IO_INT_ADDR) && (_pin < 9))
    //     return false;
    return set_direction_internal(_ioID, get_internal_registers(_ioID), _pin, _mode);
}
static bool set_level_io(uint8_t _pin, uint8_t _state, uint8_t _ioID)
{
    // if ((_ioID == IO_INT_ADDR) && (_pin < 9))
    //     return false;
    return set_level_internal(_ioID, get_internal_registers(_ioID), _pin, _state);
}
static int get_level_io(uint8_t _pin, uint8_t _ioID)
{
    // if ((_ioID == IO_INT_ADDR) && (_pin < 9))
    //     return -1;
    return get_level_internal(_ioID, get_internal_registers(_ioID), _pin);
}
static bool set_int_pin_io(uint8_t _pin, uint8_t _ioID)
{
    return set_int_pin_internal(_ioID,get_internal_registers(_ioID),_pin);
}
static bool clear_int_pin_io(uint8_t _pin, uint8_t _ioID)
{
    return clear_int_pin_internal(_ioID, get_internal_registers(_ioID), _pin);
}
static int get_int_io(uint8_t _ioID)
{
    return get_int_internal(_ioID, get_internal_registers(_ioID));
}
static int get_port_io(uint8_t _ioID)
{
    return get_port_internal(_ioID, get_internal_registers(_ioID));
}

static bool set_port_io(uint16_t values, uint8_t _ioID)
{
    // Can't use this function on internal IO Expander
    // otherwise can damage Inkplate!
    // if ((_ioID == IO_INT_ADDR))
    //     return false;
    return set_port_internal(_ioID, get_internal_registers(_ioID), values);
}

bool pcal_ex_init(void) {
    if (initialized) return true;
    if (!i2c_init()) {
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
    dev_cfg.device_address = IO_INT_ADDR;
    if(ESP_OK!=i2c_master_bus_add_device(i2c_bus_handle,&dev_cfg,&i2c_handle_int)) {
        i2c_bus_handle = NULL;
        return false;
    }
    dev_cfg.device_address = IO_EXT_ADDR;
    if(ESP_OK!=i2c_master_bus_add_device(i2c_bus_handle,&dev_cfg,&i2c_handle_ext)) {
        i2c_master_bus_rm_device(i2c_handle_int);
        i2c_handle_int = NULL;
        i2c_bus_handle = NULL;
        return false;
    }
#endif
    memset(ioRegsInt, 0, 22);
    memset(ioRegsEx, 0, 22);

    if(!io_init(IO_INT_ADDR, ioRegsInt)) {
        ESP_LOGE(TAG, "Could not initialize internal i/o registers.");
        return false;
    }
    if(!io_init(IO_EXT_ADDR, ioRegsEx)) {
        ESP_LOGE(TAG, "Could not initialize external i/o registers.");
        return false;
    }
    initialized = true;
    return true;
}
bool pcal_ex_set_direction_ext(uint8_t pin, pcal_ex_mode_t mode) {
    return set_direction_io(pin,mode,IO_EXT_ADDR);
}
bool pcal_ex_set_level_ext(uint8_t pin, pcal_ex_level_t level) {
    return set_level_io(pin,level,IO_EXT_ADDR);
}
int pcal_ex_get_level_ext(uint8_t pin) {
    return get_level_io(pin,IO_EXT_ADDR);
}
bool pcal_ex_set_int_pin_ext(uint8_t pin) {
    return set_int_pin_io(pin,IO_EXT_ADDR);
}
bool pcal_ex_clear_int_pin_ext(uint8_t pin) {
    return clear_int_pin_io(pin,IO_EXT_ADDR);
}
int pcal_ex_get_int_ext(void) {
    return get_int_io(IO_EXT_ADDR);
}
bool pcal_ex_set_port_ext(uint16_t values) {
    return set_port_io(values,IO_EXT_ADDR);
}
int pcal_ex_get_port_ext(void) {
    return get_port_io(IO_EXT_ADDR);
}

bool pcal_ex_set_direction_int(uint8_t pin, pcal_ex_mode_t mode) {
    return set_direction_io(pin,mode,IO_INT_ADDR);
}
bool pcal_ex_set_level_int(uint8_t pin, pcal_ex_level_t level) {
    return set_level_io(pin,level,IO_INT_ADDR);
}
int pcal_ex_get_level_int(uint8_t pin) {
    return get_level_io(pin,IO_INT_ADDR);
}
bool pcal_ex_set_int_pin_int(uint8_t pin) {
    return set_int_pin_io(pin,IO_INT_ADDR);
}
bool pcal_ex_clear_int_pin_int(uint8_t pin) {
    return clear_int_pin_io(pin,IO_INT_ADDR);
}
int pcal_ex_get_int_int(void) {
    return get_int_io(IO_INT_ADDR);
}
bool pcal_ex_set_port_int(uint16_t values) {
    return set_port_io(values,IO_INT_ADDR);
}
int pcal_ex_get_port_int(void) {
    return get_port_io(IO_INT_ADDR);
}

#endif  // INKPLATE10