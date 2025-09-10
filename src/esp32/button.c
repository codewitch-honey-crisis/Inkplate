#ifdef ESP_PLATFORM
#include "button.h"
#include <memory.h>
#include "hardware.h"
#include "esp_log.h"
#include "driver/gpio.h"
static const char* TAG = "button";
#ifdef BUTTON_A
static button_on_press_changed_callback_t opc_callback = NULL;
static void* opc_state = NULL;
static bool initialized = false;
#endif
static void isr_handler(void* arg) {
    if(opc_callback!=NULL) {
        bool pressed = !(!!gpio_get_level((gpio_num_t)BUTTON_A));
        opc_callback(pressed,opc_state);
    }
}
bool button_init(void) {
#ifdef BUTTON_A
    if(initialized) {
        return true;
    }
    if(ESP_OK==gpio_install_isr_service(ESP_INTR_FLAG_EDGE)) {
        gpio_config_t config;
        memset(&config,0,sizeof(config));
        config.mode = GPIO_MODE_INPUT;
        config.intr_type = GPIO_INTR_POSEDGE;
        config.pin_bit_mask = ((uint64_t)1) << BUTTON_A;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        if(ESP_OK==gpio_config(&config)) {
            if(ESP_OK==gpio_isr_handler_add((gpio_num_t)BUTTON_A,isr_handler,NULL)) {
                initialized = true;
                return true;
            }
        }
    }
#endif
    gpio_uninstall_isr_service();
    ESP_LOGE(TAG,"Unable to initialize button");
    return false;
}
void button_on_press_changed_callback(button_on_press_changed_callback_t callback, void* state) {    
    opc_callback= callback;
    opc_state = state;
}
#endif