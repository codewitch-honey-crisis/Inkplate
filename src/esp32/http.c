#include "http.h"
#include <stdint.h>
#include <memory.h>
#include "esp_http_client.h"

char http_enc_rfc3986[256] = {0};
char http_enc_html5[256] = {0};

http_handle_t http_init(const char* url) {
    esp_http_client_config_t cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.auth_type = HTTP_AUTH_TYPE_NONE;
    cfg.url = url;
    cfg.method = HTTP_METHOD_GET;
    return esp_http_client_init(&cfg);
}
int http_read_status_and_headers(http_handle_t handle) {
    if(ESP_OK!=esp_http_client_open((esp_http_client_handle_t)handle,0)) {
        return -1;
    }
    int64_t ret = esp_http_client_fetch_headers((esp_http_client_handle_t)handle);
    if (ret<0) {
        return -1;
    }
    return esp_http_client_get_status_code((esp_http_client_handle_t)handle);
}
int http_read(http_handle_t handle, void* buffer, size_t buffer_size) {
    return esp_http_client_read((esp_http_client_handle_t)handle,(char*)buffer,(int)buffer_size);
    
}
void http_end(http_handle_t handle) {
    esp_http_client_cleanup((esp_http_client_handle_t)handle);
}

char* http_url_encode(char* enc, size_t size, const char* s,
                              char* table) {
    char* result = enc;
    if(http_enc_html5['A']==0) {
        for (int i = 0; i < 256; i++) {
            http_enc_rfc3986[i] =
                isalnum(i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
            http_enc_html5[i] =
                isalnum(i) || i == '*' || i == '-' || i == '.' || i == '_' ? i
                : (i == ' ')                                               ? '+'
                                                                        : 0;
        }
    }
    if (table == NULL) table = http_enc_rfc3986;
    for (; *s; s++) {
        if (table[(int)*s]) {
            *enc++ = table[(int)*s];
            --size;
        } else {
            snprintf(enc, size, "%%%02X", *s);
            while (*++enc) {
                --size;
            }
        }
    }
    if (size) {
        *enc = '\0';
    }
    return result;
}