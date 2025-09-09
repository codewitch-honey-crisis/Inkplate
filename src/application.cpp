#include "display.h"
#include "task.h"
#include "timing.h"
#include "fs.h"
#ifdef ESP_PLATFORM
#include "esp32/captive_portal.h"
#include "esp32/ui_captive_portal.hpp"
#endif
#include "gfx.hpp"
#include "uix.hpp"
#include "httpd_application.h"
#define HTTPD_CONTENT_IMPLEMENTATION
#include "httpd_content.h"
#include "ui.hpp"
using namespace gfx;
using namespace uix;

extern "C" void run(void) {
    if(!fs_internal_init()) {
        puts("FS init failed");
    }
    if(!display_init()) {
        puts("Display init failed");
        return;
    }
    if(net_init()) {
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
        if(!captive_portal_init()) {
            puts("Error initializing captive portal");
            return;        
        }
        
        char ssid[65];
        char pass[129];
        char address[129];
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
        if(!ui_captive_portal_setup(address,ssid,pass)) {
            puts("ui setup failed");
            return;
        }
    }
#else
    puts("Network connection failure.");
    return;
#endif
}
extern "C" void loop(void) {
      task_delay(5);
}