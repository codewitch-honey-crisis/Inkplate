#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
typedef void* http_handle_t;
extern char http_enc_rfc3986[];
extern char http_enc_html5[];

#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initializes an HTTP session with an URL
/// @param url The URL endpoint
/// @return A handle for the HTTP session
http_handle_t http_init(const char* url);
/// @brief Reads the status and the headers (discarding the headers in the current implementation)
/// @param handle The handle
/// @return The HTTP status code, or -1 on error
int http_read_status_and_headers(http_handle_t handle);
/// @brief Reads data from an HTTP response body
/// @param handle The handle (http_read_status_and_headers must have already been called on it)
/// @param buffer The buffer to hold the result
/// @param buffer_size The size of the buffer
/// @return The number of bytes read or < 0 on error
int http_read(http_handle_t handle, void* buffer, size_t buffer_size);
/// @brief Ends the HTTP session and closes any connection
/// @param handle The HTTP handle
void http_end(http_handle_t handle);
/// @brief Encodes an URL parameter
/// @param enc The result
/// @param size The size of the result
/// @param s The string to encode
/// @param table The table to use (http_enc_rfc3986 or http_enc_html5)
/// @return The encoded string
char* http_url_encode(char* enc, size_t size, const char* s,
                              char* table);
#ifdef __cplusplus
}
#endif
#endif // HTTP_H