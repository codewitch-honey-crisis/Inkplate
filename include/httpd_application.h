#ifndef HTTPD_APPLICATION_H
#define HTTPD_APPLICATION_H
#include <stdbool.h>
#include <stddef.h>
#include "httpd.h"
#include "network.h"
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif
bool json_string_encode(const char* str, char* out_str, size_t out_str_len);
#ifdef __cplusplus
}
#endif
#endif // HTTPD_APPLICATION_H

