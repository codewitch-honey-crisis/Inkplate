#include "captive_portal.h"

#include <memory.h>
#include <string.h>
#include <sys/param.h>

#include "common.h"
#include "dns_server.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "httpd.h"
#include "fs.h"
#include "config.h"
#include "network.h"
#include "httpd_content.h"
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s
#ifndef CAPTIVE_PORTAL_SSID
#define CAPTIVE_PORTAL_SSID CaptivePortal
#endif
#define CAPTIVE_PORTAL_SSID1 STRINGIFY(CAPTIVE_PORTAL_SSID)

#define MAX_STA_CONN 5

static const char* TAG = "portal";
typedef struct {
    httpd_handle_t hd;
    int fd;
    char path_and_query[513];
    void(*on_request_callback)(const char* method, const char* path_and_query, void*arg, void*state);
    void*on_request_callback_state;
} httpd_async_resp_arg_t;

static bool wifi_intialized = false;
static httpd_handle_t httpd_handle = NULL;
static dns_server_handle_t dns_handle = NULL;
static esp_netif_t* wifi_ap = NULL;
static char* captive_portal_uri = NULL;
char wifi_ssid[129];
char wifi_pass[16];

static void scan_for_wifi_ssid(void)
{
    strcpy(wifi_ssid,CAPTIVE_PORTAL_SSID1);
    char target[129];
    strcpy(target,CAPTIVE_PORTAL_SSID1);
    bool wifi_inited = false;

    // don't want this on the stack
    static wifi_ap_record_t ap_info[256];
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if(sta_netif==NULL) {
        goto error;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(ESP_OK!=esp_wifi_init(&cfg)) {
        return;
    }
    uint16_t ap_max = 256;
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    if(ESP_OK!=esp_wifi_set_mode(WIFI_MODE_STA)) {
        goto error;
    }
    if(ESP_OK!=esp_wifi_start()) {
        goto error;
    }
    wifi_inited = true;

    if(ESP_OK!=esp_wifi_scan_start(NULL, true)) {
        ESP_LOGE(TAG,"Error initiating scan");
        goto error;
    }

    if(ESP_OK!=esp_wifi_scan_get_ap_num(&ap_count)) {
        ESP_LOGE(TAG,"Error retreiving ap count");
        goto error;
    }
    if(ESP_OK!=esp_wifi_scan_get_ap_records(&ap_max, ap_info)) {
        ESP_LOGE(TAG,"Error scanning records");
        esp_wifi_clear_ap_list();
        goto error;
    }
    int result = 1;
    bool done = false;
    while(!done) {
            done = true;
            for(int i = 0;i<ap_max;++i) {
            if(0==strcmp((char*)ap_info[i].ssid,target)) {
                strcpy(target,CAPTIVE_PORTAL_SSID1);
                itoa(++result,target+strlen(target),10);
                done = false;
            }
        }
    }
    esp_wifi_scan_stop();
    esp_wifi_disconnect();
    
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u, result = %s", ap_count, ap_max,target);
    
    strcpy(wifi_ssid,target);
error:
    if(wifi_inited) {
        esp_wifi_stop();
        esp_wifi_deinit();
        
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}
static void wifi_deinit_softap(void) {
    if(!wifi_intialized) {
        return;
    }
    esp_wifi_stop();
    esp_event_handler_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifi_event_handler);
    esp_wifi_deinit();
    wifi_intialized = false;
}
static bool wifi_init_softap(void) {
    if(wifi_intialized) {
        return true;
    }
    if(!fs_get_device_id(wifi_pass,sizeof(wifi_pass))) {
        return false;
    }
    bool wifi_init = false;
    bool event_handler_init = false;
    bool wifi_started = false;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (ESP_OK != esp_wifi_init(&cfg)) {
        return false;
    }
    wifi_init = true;
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL)) {
        goto error;
    }
    event_handler_init=true;
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(wifi_ssid),
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    strcpy((char*)wifi_config.ap.ssid,wifi_ssid);
    strcpy((char*)wifi_config.ap.password,wifi_pass);
    // if (strlen(ESP_WIFI_PASS) == 0) {
    //     wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    // }

    if(ESP_OK!=esp_wifi_set_mode(WIFI_MODE_AP)) {
        goto error;
    }
    if(ESP_OK!=esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config)) {
        goto error;
    }
    if(ESP_OK!=esp_wifi_start()) {
        goto error;
    }
    wifi_started = true;
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             CAPTIVE_PORTAL_SSID1, wifi_pass);
    wifi_intialized = true;
    return true;
error:
    if(wifi_started) {
        esp_wifi_stop();
    }
    if(event_handler_init) {
        esp_event_handler_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifi_event_handler);
    }
    if(wifi_init) {
        esp_wifi_deinit();
    }
    return false;
}

static bool dhcp_set_captiveportal_url(void) {
    if(captive_portal_uri!=NULL) {
        free(captive_portal_uri);
    }
    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    captive_portal_uri = (char*)malloc(32 * sizeof(char));
    if (captive_portal_uri == NULL) {
        return false;
    }
    strcpy(captive_portal_uri, "http://");
    strcat(captive_portal_uri, ip_addr);

    // get a handle to configure DHCP with
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    // set the DHCP option 114
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(netif));
    if (ESP_OK != esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captive_portal_uri, strlen(captive_portal_uri))) {
        free(captive_portal_uri);
        return false;
    }
    if (ESP_OK != esp_netif_dhcps_start(netif)) {
        free(captive_portal_uri);
        return false;
    }
    return true;
}

// parse the query parameters and apply them to the settings.
static void parse_url_and_apply(const char* url) {
    const char* query = strchr(url, '?');
    char ssid[64];
    ssid[0]='\0';
    char pass[64];
    pass[0]='\0';
    char tz[64];
    char name[64];
    char value[64];
    bool restart = false;
    bool set_creds = false;
    bool set_tz = false;
    if (query != NULL) {
        while (1) {
            query = httpd_crack_query(query, name, value);
            if (!query) {
                break;
            }
            if(0==strcmp("ssid",name)) {
                restart = true;
                set_creds=true;
                strncpy(ssid,value,63);
            }
            if(0==strcmp("pass",name)) {
                set_creds = true;
                restart = true;
                strncpy(pass,value,63);
            }
            if(0==strcmp("tz",name)) {
                set_tz=true;
                restart = true;
                strncpy(tz,value,63);
            }
        }
    }
    
    if(set_creds) {
        net_set_credentials(ssid,pass);
    }
    if(set_tz) {
        long ofs = 0;
        sscanf(tz,"%ld",&ofs);
        char buf[32];
        snprintf(buf,31,"%ld",ofs*3600);
        config_add_value("tzoffset",buf);
        restart=true;
    }
    if(restart) {
        esp_restart();
    }
}
static esp_err_t httpd_err_handler(httpd_req_t *req, httpd_err_code_t error) {
    ESP_LOGI(TAG,"Sending captive portal redirect");
    httpd_async_resp_arg_t* resp_arg = (httpd_async_resp_arg_t*)malloc(sizeof(httpd_async_resp_arg_t));
    if (resp_arg == NULL) {
        ESP_LOGE(TAG,"No memory for HTTP request!");
        return ESP_ERR_NO_MEM;
    }
    resp_arg->fd = httpd_req_to_sockfd(req);
    if (resp_arg->fd == -1) {
        free(resp_arg);
        return ESP_FAIL;
    }
    resp_arg->hd = httpd_handle;
    ESP_LOGE(TAG,"Queueing HTTP response");
    return httpd_queue_work(httpd_handle,httpd_content_portal_redir_clasp, resp_arg);
}
static esp_err_t httpd_handle_request(httpd_req_t* req) {
    httpd_work_fn_t handler = (httpd_work_fn_t)req->user_ctx;
    if (handler == NULL) {
        ESP_LOGE(TAG,"No HTTP handler!");
        return ESP_FAIL;
    }
    parse_url_and_apply(req->uri);
    httpd_async_resp_arg_t* resp_arg = (httpd_async_resp_arg_t*)malloc(sizeof(httpd_async_resp_arg_t));
    if (resp_arg == NULL) {
        ESP_LOGE(TAG,"No memory for HTTP request!");
        return ESP_ERR_NO_MEM;
    }
    resp_arg->fd = httpd_req_to_sockfd(req);
    if (resp_arg->fd == -1) {
        free(resp_arg);
        return ESP_FAIL;
    }
    resp_arg->hd = httpd_handle;
    ESP_LOGE(TAG,"Queueing HTTP response");
    return httpd_queue_work(httpd_handle, handler, resp_arg);
}
static bool www_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = HTTPD_RESPONSE_HANDLER_COUNT;
    config.max_open_sockets = CONFIG_LWIP_MAX_SOCKETS-3;
    config.lru_purge_enable = true;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering Captive Portal URI handlers");
        if(ESP_OK!=httpd_register_err_handler(server,HTTPD_404_NOT_FOUND,httpd_err_handler)) {
            return httpd_stop(server);
            httpd_handle = NULL;
            return false;
        }
        for (int i = 0; i < HTTPD_RESPONSE_HANDLER_COUNT; ++i) {
            httpd_uri_t uri_config;
            const httpd_response_handler_t* resp = &httpd_response_handlers[i];
            memset(&uri_config, 0, sizeof(uri_config));
            uri_config.handler = httpd_handle_request;
            uri_config.method = HTTP_GET;
            uri_config.uri = resp->path_encoded;
            httpd_work_fn_t fn = resp->handler;
            // remap the index to the portal
            if (0 == strcmp(resp->path_encoded, "/") || 0 == strcmp(resp->path_encoded, "/index.html")) {
                fn = httpd_content_portal_html;
            }
            uri_config.user_ctx = fn;
            if (ESP_OK != httpd_register_uri_handler(server, &uri_config)) {
                ESP_LOGI(TAG, "Failed to register handler");
                httpd_stop(server);
                httpd_handle = NULL;
                return false;
            }
        }
        
        httpd_handle = server;
        return true;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return false;
}
void captive_portal_end(void) {
    if(dns_handle) {
        stop_dns_server(dns_handle);
        dns_handle = NULL;
    }
    if(httpd_handle!=NULL) {
        httpd_stop(httpd_handle);
        httpd_handle = NULL;
    }
    esp_netif_dhcpc_stop(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"));
    wifi_deinit_softap();
    esp_netif_destroy_default_wifi(wifi_ap);
    nvs_flash_deinit();
    esp_event_loop_delete_default();
    esp_netif_deinit();
    if(captive_portal_uri!=NULL) {
        free(captive_portal_uri);
        captive_portal_uri = NULL;
    }
}
bool captive_portal_init(void) {
    if(dns_handle!=NULL) {
        return true;
    }
    bool loop_created = false;
    bool nvs_inited = false;
    
    bool init_ap = false;
    bool init_dhcp = false;
    bool www_started = false;
// Initialize NVS needed by Wi-Fi
    if (ESP_OK != nvs_flash_init()) {
        ESP_LOGE(TAG,"NVS flash init failed");
        goto error;
    }
    nvs_inited = true;
    // Initialize networking stack
    if (ESP_OK != esp_netif_init()) {
        ESP_LOGE(TAG,"Net IF init failed");
        return false;
    }
    
    // Create default event loop needed by the  main app
    if (ESP_OK != esp_event_loop_create_default()) {
        ESP_LOGE(TAG,"event loop creation failed");
        goto error;
    }
    loop_created = true;
    scan_for_wifi_ssid();
    
    // Initialize Wi-Fi including netif with default config
    wifi_ap = esp_netif_create_default_wifi_ap();
    if (wifi_ap == NULL) {
        ESP_LOGE(TAG,"create default wifi ap failed");
        goto error;
    }
    // Initialise ESP32 in SoftAP mode
    if(!wifi_init_softap()) {
        ESP_LOGE(TAG,"wifi init softap failed");
        goto error;
    }
    init_ap=true;
    if(!dhcp_set_captiveportal_url()) {
        ESP_LOGE(TAG,"DHCP init failed");
        goto error;
    }
    init_dhcp = true;
    
    // Start the server for the first time
    if(!www_start()) {
        ESP_LOGE(TAG,"httpd  init failed");
        goto error;
    }
    www_started = true;
    // Start the DNS server that will redirect all queries to the softAP IP
    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */);
    dns_handle = start_dns_server(&config);
    if(dns_handle==NULL) {
        ESP_LOGE(TAG,"DNS init failed");
        goto error;
    }
    return true;
error:
    if(dns_handle) {
        stop_dns_server(dns_handle);
        dns_handle = NULL;
    }
    if(www_started) {
        httpd_stop(httpd_handle);
        httpd_handle = NULL;
    }
    if(init_dhcp) {
        esp_netif_dhcpc_stop(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"));
    }
    if(init_ap) {
        wifi_deinit_softap();
    }
    if (wifi_ap != NULL) {
        esp_netif_destroy_default_wifi(wifi_ap);
    }
    if (nvs_inited) {
        nvs_flash_deinit();
    }
    if (loop_created) {
        esp_event_loop_delete_default();
    }

    esp_netif_deinit();
    if(captive_portal_uri!=NULL) {
        free(captive_portal_uri);
        captive_portal_uri = NULL;
    }
    return false;
}

bool captive_portal_get_address(char* out_address,size_t out_address_length) {
    if(dns_handle==NULL) {
        return false;
    }
    strncpy(out_address,captive_portal_uri,out_address_length);
    return true;
}

bool captive_portal_get_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length) {
    if(dns_handle==NULL) {
        return false;
    }
    strncpy(out_ssid,wifi_ssid,out_ssid_length);
    strncpy(out_pass,wifi_pass,out_pass_length);
    return true;
}