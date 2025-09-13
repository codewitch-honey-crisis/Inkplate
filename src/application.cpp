#include "display.h"
#include "button.h"
#include "task.h"
#include "timing.h"
#include "fs.h"
#include "rtc_time.h"
#ifdef ESP_PLATFORM
#include "rtc_wdt.h"
#include "esp_task_wdt.h"
#include "esp32/captive_portal.h"
#include "esp32/ui_captive_portal.hpp"
#endif
#ifdef INKPLATE10V2
#include "esp_sleep.h"
#include "inkplate10/power.h"
#endif
#include "gfx.hpp"
#include "uix.hpp"
#include "httpd_application.h"
#define HTTPD_CONTENT_IMPLEMENTATION
#include "httpd_content.h"
#include "ui_weather.hpp"
using namespace gfx;
using namespace uix;
static uint32_t start_ts;
static uint32_t wash_start_ts;
static uint32_t net_start_ts=0;
#ifdef ESP_PLATFORM
char ssid[65];
char pass[129];
char address[129];
static task_mutex_t dhcp_mutex;
static int dhcp_connected = 0;
static int dhcp_connected_old = -1;
static void portal_on_connect(void* state) {
    task_mutex_lock(dhcp_mutex,-1);
    dhcp_connected = 1;
    task_mutex_unlock(dhcp_mutex);

}
static void portal_on_disconnect(void* state) {
    task_mutex_lock(dhcp_mutex,-1);
    dhcp_connected = 0;
    task_mutex_unlock(dhcp_mutex);
}
#endif
static void on_wash_complete(void* state) {
    printf("Screen wash complete in %0.2f seconds\n",(timing_get_ms()-wash_start_ts)/1000.f);
}
static uint32_t fetch_ts = 0;
static void on_button_changed(bool pressed, void* state) {
    if(!pressed) {
        puts("released!");
        fetch_ts=0;
    }
}
extern "C" void run(void) {
    puts("Update starting");
    start_ts = timing_get_ms();
    if(!fs_internal_init()) {
        puts("FS init failed");
    }
    if(!display_init()) {
        puts("Display init failed");
        return;
    }
    display_on_washed_complete_callback(on_wash_complete,NULL);
    puts("Screen wash started");
    wash_start_ts = timing_get_ms();
    if(!display_wash_8bit_async()) {
        puts("Warning: Screen wash failed. Continuing...");
    }
    if(!button_init()) {
        puts("BUtton init failed");
        return;
    }
    if(!rtc_time_init()) {
        puts("RTC init failed");
        return;
    }
    button_on_press_changed_callback(on_button_changed,nullptr);
    if(net_init()) {
        net_start_ts=timing_get_ms();
        while(net_status()==NET_WAITING) {
            timing_delay_ms(5);
        }
        if(net_status()!=NET_CONNECTED) {
            puts("Could not connect to network");    
        }
    } else {
        puts("Could not initialize network");
    }
    if(net_status()!=NET_CONNECTED) {
        net_end();
#ifdef ESP_PLATFORM
        dhcp_mutex = task_mutex_init();
        if(dhcp_mutex==nullptr) {
            puts("Could not initialize DHCP mutex");
            return;
        }
        captive_portal_on_sta_connect(portal_on_connect,nullptr);
        captive_portal_on_sta_disconnect(portal_on_disconnect,nullptr);
        if(!captive_portal_init()) {
            puts("Error initializing captive portal");
            return;        
        }
        if(!captive_portal_get_credentials(ssid,sizeof(ssid),pass,sizeof(pass))) {
            puts("captive portal get creds failed");
            return;
        }
        if(!captive_portal_get_address(address,sizeof(address))) {
            puts("captive portal get address failed");
            return;
        }
        if(!ui_captive_portal_init()) {
            puts("ui init failed");
            return;
        }
        if(!ui_captive_portal_set_ap(address,ssid,pass)) {
            puts("ui update failed");
            return;
        }
        while(true) {
            rtc_wdt_feed();
            task_delay(5);
            int conn = dhcp_connected;
            if(conn!=dhcp_connected_old) {
                dhcp_connected_old  = conn;
                if(conn) {
                    if(!ui_captive_portal_set_url(address)) {
                        puts("ui update failed");
                        return;
                    }
                } else {
                    if(!ui_captive_portal_set_ap(address,ssid,pass)) {
                        puts("ui update failed");
                        return;
                    }
                }
            }
        }
    
#else
        puts("Network connection failure.");
        return;
#endif
    }
    if(!ui_weather_init()) {
        puts("Could not initialize weather UI.");
        while(1) {
            task_delay(5);
        }
    }
}
extern "C" void loop(void) {
    button_update();
    if(net_status()==NET_CONNECTED) {
        if(net_start_ts!=0) {
            printf("Network connected in %0.2f seconds\n",(timing_get_ms()-net_start_ts)/1000.f);
            net_start_ts=0;
        }
        if(fetch_ts==0 || timing_get_ms()>=fetch_ts+15*60*1000) {
            if(fetch_ts!=0) {
                puts("Update starting");
                start_ts = timing_get_ms();
            }
            fetch_ts=timing_get_ms();
            
            if(!ui_weather_fetch()) {
                puts("Could not fetch weather");
            } else {
                printf("Update completed in %0.2f seconds\n",(timing_get_ms()-start_ts)/1000.f);
            }
#ifdef INKPLATE10V2
            esp_sleep_enable_timer_wakeup(15*60*1000000);
            power_sleep();
#endif

        }
    }
    task_delay(5);
}