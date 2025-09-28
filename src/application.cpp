#include "display.h"
#include "task.h"
#include "timing.h"
#include "fs.h"
#include "rtc_time.h"
#include "hardware.h"
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
static bool start_portal = false;
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

extern "C" void run(void) {
#ifdef ESP_PLATFORM
   esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
        puts("Reseting configuration");
        start_portal = true;
        break;
    default:
        break;
  }
    printf("Update starting. Free RAM: %0.2fKB\n", esp_get_free_heap_size()/1024.f);
#endif
    start_ts = timing_get_ms();
    if(!fs_internal_init()) {
        puts("FS init failed");
    }
    if(!display_init()) {
        puts("Display init failed");
        return;
    }
    if(!start_portal) {
        display_on_washed_complete_callback(on_wash_complete,NULL);
        puts("Screen wash started");
        wash_start_ts = timing_get_ms();
        if(!display_wash_8bit_async()) {
            puts("Warning: Screen wash failed. Continuing...");
        }    
    }
    
    if(!rtc_time_init()) {
        puts("RTC init failed");
        return;
    }
    if(!start_portal && net_init_async()) {
        net_start_ts=timing_get_ms();
        while(net_status()==NET_WAITING) {
            timing_delay_ms(5);
        }
        if(net_status()!=NET_CONNECTED) {
            puts("Could not connect to network");    
        }
    } else {
        if(!start_portal) {
            puts("Could not initialize network");
        }
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
                    if(!ui_captive_portal_set_configure(address)) {
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
    long next_update = 15*60;
    if(net_status()==NET_CONNECTED) {
        if(net_start_ts!=0) {
            printf("Network connected in %0.2f seconds\n",(timing_get_ms()-net_start_ts)/1000.f);
            net_start_ts=0;
        }
        if(fetch_ts==0 || timing_get_ms()>=fetch_ts+next_update*1000) {
            if(fetch_ts!=0) {
                puts("Update starting");
                start_ts = timing_get_ms();
            }
            fetch_ts=timing_get_ms();
            next_update = ui_weather_fetch();
            printf("Next update in %0.f minutes\n", next_update / 60.f);
            
#ifdef INKPLATE10V2            
            // we'll be sleeping, so we may as well end this a little early to save some power.
            puts("Shutting down network");
            net_end();
#endif
            puts("Waiting for wash to finish...");
            display_wash_8bit_wait();            
            uint32_t transfer_ts = timing_get_ms();
            puts("Begin display panel transfer");
            if (display_update_8bit()) {
                printf("Display panel transfer complete in %0.2f seconds. Turning off display.\n",(long)(timing_get_ms()-transfer_ts)/1000.f);
                display_sleep();
            }
            if(!next_update) {
                puts("Could not fetch weather");
            } else {
                printf("Update completed in %0.2f seconds\n",(timing_get_ms()-start_ts)/1000.f);
            }
#ifdef INKPLATE10V2
            printf("Update finished. Free RAM: %0.2fKB\n", esp_get_free_heap_size()/1024.f);
            puts("Sleeping");
            esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_A, 0);
            esp_sleep_enable_timer_wakeup(next_update*1000000);
            power_sleep();
#endif

        }
    }
    task_delay(5);
}