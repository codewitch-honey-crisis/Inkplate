#ifdef ESP_PLATFORM
#include "network.h"
#include <stdio.h>
#include <memory.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "fs.h"
#define WIFI_RETRY_COUNT 6
static esp_ip4_addr_t wifi_ip;
static size_t wifi_retry_count = 0;
static const EventBits_t wifi_connected_bit = BIT0;
static const EventBits_t wifi_fail_bit = BIT1;
static EventGroupHandle_t wifi_event_group = NULL;
static esp_netif_t* wifi_sta = NULL;
static char wifi_ssid[65]={0};
static char wifi_pass[129]={0};    
static const char* TAG = "network";
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retry_count < WIFI_RETRY_COUNT) {
            ESP_LOGI(TAG,"Disconnected from AP, retrying");
            ++wifi_retry_count;
            esp_wifi_connect();
            
        } else {
            ESP_LOGI(TAG,"Failed to connect to AP. Retry count exceeded.");
            xEventGroupSetBits(wifi_event_group, wifi_fail_bit);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG,"Found wifi network, got IP");
        wifi_retry_count = 0;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        memcpy(&wifi_ip, &event->ip_info.ip, sizeof(wifi_ip));
        xEventGroupSetBits(wifi_event_group, wifi_connected_bit);
    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_LOST_IP) {
        ESP_LOGI(TAG,"Lost network, lost IP");
        esp_wifi_disconnect();
    } else {
        ESP_LOGI(TAG,"Got unhandled wifi event %d",(int)event_id);
    }
}
bool net_wifi_load(const char* path, char* ssid, size_t out_ssid_length, char* pass, size_t out_pass_length) {
    ssid[0]='\0';
    pass[0]='\0';
    if(!fs_internal_init()) {
        return false;
    }
    
    FILE* file = fopen(path, "r");
    if (file != NULL) {
        // parse the file
        fgets(ssid, out_ssid_length, file);
        char* sv = strchr(ssid, '\n');
        if (sv != NULL) *sv = '\0';
        sv = strchr(ssid, '\r');
        if (sv != NULL) *sv = '\0';
        fgets(pass, out_pass_length, file);
        fclose(file);
        sv = strchr(pass, '\n');
        if (sv != NULL) *sv = '\0';
        sv = strchr(pass, '\r');
        if (sv != NULL) *sv = '\0';
        return true;
    }
    return false;
}

bool net_init_async() {
    if(wifi_event_group!=NULL) {
        return true;
    }
    if(!fs_internal_init()) {
        return false;
    }
    bool loaded = false;
    wifi_ssid[0]='\0';
    wifi_pass[0]='\0';
    // if (fs_external_init()) {
    //     ESP_LOGI(TAG,"SD card found, looking for wifi.txt creds");
    //     loaded = net_wifi_load("/sdcard/wifi.txt", wifi_ssid,64, wifi_pass, 128);
    //     if(loaded) {
    //         ESP_LOGI(TAG,"Found wifi.txt on SD card");
    //     }
    // }
    if (!loaded) {
        ESP_LOGI(TAG,"Looking for wifi.txt creds on internal flash");
        loaded = net_wifi_load("/spiffs/wifi.txt", wifi_ssid, 64, wifi_pass, 128);
        if(loaded) {
            ESP_LOGI(TAG,"Found wifi.txt on internal flash");
        }
    }
    if (loaded) {
        ESP_LOGI(TAG,"Initializing WiFi connection to %s\n", wifi_ssid);
    } else {
        return false;
    }  

    if(ESP_OK!=nvs_flash_init()) {
        ESP_LOGE(TAG,"NVS flash init failed");
        return false;
    }
    
    wifi_event_group = xEventGroupCreate();
    if(wifi_event_group==NULL) {
        ESP_LOGE(TAG,"Event group create failed");
        return false;
    }
    
    if(ESP_OK!=esp_netif_init()) {
        ESP_LOGE(TAG,"netif init failed");
        vEventGroupDelete(wifi_event_group);
        wifi_event_group=NULL;
        return false;
    }

    if(ESP_OK!=esp_event_loop_create_default()) {
        ESP_LOGE(TAG,"event loop default create failed");
        esp_netif_deinit();
        vEventGroupDelete(wifi_event_group);
        wifi_event_group=NULL;
        return false;
    }
    wifi_sta=esp_netif_create_default_wifi_sta();
    if(NULL==wifi_sta) {
        ESP_LOGE(TAG,"create default wifi sta failed");
        esp_netif_deinit();
        vEventGroupDelete(wifi_event_group);
        wifi_event_group=NULL;
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(ESP_OK!=esp_wifi_init(&cfg)) {
        ESP_LOGE(TAG,"wifi init failed");
        esp_netif_deinit();
        vEventGroupDelete(wifi_event_group);
        wifi_event_group=NULL;
        return false;
    }
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_t instance_lost_ip;
    if(ESP_OK!=esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
        &instance_any_id)) {
            ESP_LOGE(TAG,"event handler 1 registration failed");
        goto error;
    }
    if(ESP_OK!=esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
        &instance_got_ip)) {
            ESP_LOGE(TAG,"event handler 2 registration failed");
        goto error;
        
    }
    if(ESP_OK!=esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_LOST_IP, &wifi_event_handler, NULL,
        &instance_lost_ip)) {
            ESP_LOGE(TAG,"event handler 3 registration failed");
        goto error;
        
    }
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    memcpy(wifi_config.sta.ssid, wifi_ssid, strlen(wifi_ssid) + 1);
    memcpy(wifi_config.sta.password, wifi_pass, strlen(wifi_pass) + 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    // wifi_config.sta.sae_h2e_identifier[0]=0;
    if(ESP_OK!=esp_wifi_set_mode(WIFI_MODE_STA)) {
        ESP_LOGE(TAG,"wifi set mode failed");
        goto error;
    }
    if(ESP_OK!=esp_wifi_set_config(WIFI_IF_STA, &wifi_config)) {
        ESP_LOGE(TAG,"wifi set config failed");
        goto error;
    }
    if(ESP_OK!=esp_wifi_start()) {
        ESP_LOGE(TAG,"wifi start failed");
        goto error;
    }
    return true;
error:
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(wifi_sta);
    wifi_sta = NULL;
    esp_netif_deinit();
    vEventGroupDelete(wifi_event_group);
    esp_event_loop_delete_default();
    wifi_event_group=NULL;
    return false;
}

net_status_t net_status() {
    if (wifi_event_group == NULL) {
        return NET_UNINITIALIZED ;
    }
    EventBits_t bits = xEventGroupGetBits(wifi_event_group) &
                       (wifi_connected_bit | wifi_fail_bit);
    if (bits == wifi_connected_bit) {
        return NET_CONNECTED;
    } else if (bits == wifi_fail_bit) {
        return NET_CONNECT_FAILED;
    }
    return NET_WAITING;
}
void net_end() {
    if(wifi_event_group==NULL) {
        return;
    }
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(wifi_sta);
    wifi_sta = NULL;
    esp_netif_deinit();
    esp_event_loop_delete_default();
    vEventGroupDelete(wifi_event_group);
    wifi_event_group=NULL;
}
bool net_get_address(char* out_address,size_t out_address_length) {
    if(net_status()!=NET_CONNECTED) {
        return false;
    }
     snprintf(out_address, out_address_length, IPSTR,
                     IP2STR(&wifi_ip));
    return true;
}

bool net_get_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length) {
    strncpy(out_ssid, wifi_ssid, out_ssid_length);
    strncpy(out_pass, wifi_pass, out_pass_length);
    return true;
}
bool net_get_stored_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length) {
    if(!fs_internal_init()) {
        return false;
    }
    return net_wifi_load("/spiffs/wifi.txt",out_ssid,out_ssid_length, out_pass, out_pass_length);
}
bool net_set_credentials(const char* ssid, const char* pass) {
    if(!fs_internal_init()) {
        return false;
    }
    FILE* file = fopen("/spiffs/wifi.txt", "w");
    if(!file) {
        return false;
    }
    fputs(ssid,file);
    fputs("\n",file);
    fputs(pass,file);
    fputs("\n",file);
    fclose(file);
    return true;
}
#endif