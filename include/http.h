#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
typedef void* http_handle_t;
extern char http_enc_rfc3986[];
extern char http_enc_html5[];

#ifdef __cplusplus
extern "C" {
#endif
http_handle_t http_init(const char* url);
int http_read_status_and_headers(http_handle_t handle);
int http_read(http_handle_t handle, void* buffer, size_t buffer_size);
void http_end(http_handle_t handle);
char* http_url_encode(char* enc, size_t size, const char* s,
                              char* table);
#ifdef __cplusplus
}
#endif
#endif // HTTP_H