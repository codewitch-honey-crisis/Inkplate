#ifndef HTTPD_APPLICATION_H
#define HTTPD_APPLICATION_H
#include <stdbool.h>
#include <stddef.h>
#include "esp32/captive_portal.h"
#include "httpd.h"
#include "network.h"
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Encodes a string as a JSON string literal
/// @param str The string to encode
/// @param out_str The buffer to hold the encoded string
/// @param out_str_len The length of the buffer
/// @return True if successful, otherwise false
bool json_string_encode(const char* str, char* out_str, size_t out_str_len);
#ifdef __cplusplus
}
#endif
#endif // HTTPD_APPLICATION_H

