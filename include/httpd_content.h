// Generated with clasptree
// To use this file, define HTTPD_CONTENT_IMPLEMENTATION in exactly one translation unit (.c/.cpp file) before including this header.
#ifndef HTTPD_CONTENT_H
#define HTTPD_CONTENT_H

#include <stdint.h>
#include <stddef.h>

#define HTTPD_RESPONSE_HANDLER_COUNT 9
typedef struct { const char* path; const char* path_encoded; void (* handler) (void* arg); } httpd_response_handler_t;
extern httpd_response_handler_t httpd_response_handlers[HTTPD_RESPONSE_HANDLER_COUNT];
#ifdef __cplusplus
extern "C" {
#endif

// ..portal.html
void httpd_content_portal_html(void* context);
// ..portal_redir.clasp
void httpd_content_portal_redir_clasp(void* context);
// .index.html
void httpd_content_index_html(void* context);
// .api/portal.clasp
void httpd_content_api_portal_clasp(void* context);
// .api/timezones.json
void httpd_content_api_timezones_json(void* context);
// .assets/client-B2hCCc6G.js
void httpd_content_assets_client_B2hCCc6G_js(void* context);
// .assets/main-BA9fT882.css
void httpd_content_assets_main_BA9fT882_css(void* context);
// .assets/main-CoO2LHy4.js
void httpd_content_assets_main_CoO2LHy4_js(void* context);
// .assets/portal-Bdd0nQb9.js
void httpd_content_assets_portal_Bdd0nQb9_js(void* context);
// .assets/portal-Dtn62Xmo.css
void httpd_content_assets_portal_Dtn62Xmo_css(void* context);

#ifdef __cplusplus
}
#endif

#endif // HTTPD_CONTENT_H

#ifdef HTTPD_CONTENT_IMPLEMENTATION

httpd_response_handler_t httpd_response_handlers[9] = {
    { "/", "/", httpd_content_index_html },
    { "/api/portal.clasp", "/api/portal.clasp", httpd_content_api_portal_clasp },
    { "/api/timezones.json", "/api/timezones.json", httpd_content_api_timezones_json },
    { "/assets/client-B2hCCc6G.js", "/assets/client-B2hCCc6G.js", httpd_content_assets_client_B2hCCc6G_js },
    { "/assets/main-BA9fT882.css", "/assets/main-BA9fT882.css", httpd_content_assets_main_BA9fT882_css },
    { "/assets/main-CoO2LHy4.js", "/assets/main-CoO2LHy4.js", httpd_content_assets_main_CoO2LHy4_js },
    { "/assets/portal-Bdd0nQb9.js", "/assets/portal-Bdd0nQb9.js", httpd_content_assets_portal_Bdd0nQb9_js },
    { "/assets/portal-Dtn62Xmo.css", "/assets/portal-Dtn62Xmo.css", httpd_content_assets_portal_Dtn62Xmo_css },
    { "/index.html", "/index.html", httpd_content_index_html }
};

void httpd_content_portal_html(void* context) {
    
       free(context);
    

}
void httpd_content_portal_redir_clasp(void* context) {
    
       free(context);
    

}
void httpd_content_index_html(void* context) {
    
       free(context);
    

}
void httpd_content_api_portal_clasp(void* context) {
    
       free(context);
    

}
void httpd_content_api_timezones_json(void* context) {
    
       free(context);
    

}
void httpd_content_assets_client_B2hCCc6G_js(void* context) {
    
       free(context);
    

}
void httpd_content_assets_main_BA9fT882_css(void* context) {
    
       free(context);
    

}
void httpd_content_assets_main_CoO2LHy4_js(void* context) {
    
       free(context);
    

}
void httpd_content_assets_portal_Bdd0nQb9_js(void* context) {
    
       free(context);
    

}
void httpd_content_assets_portal_Dtn62Xmo_css(void* context) {
    
       free(context);
    

}
#endif // HTTPD_CONTENT_IMPLEMENTATION

