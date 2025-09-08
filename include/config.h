#ifndef CONFIG_H
#define CONFIG_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
bool config_get_value(const char* key, int index, char* out_data, size_t out_data_len);
bool config_clear_values(const char* key);
bool config_add_value(const char* key, const char* value);
#ifdef __cplusplus
}
#endif
#endif // CONFIG_H