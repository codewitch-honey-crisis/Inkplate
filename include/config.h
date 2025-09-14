#ifndef CONFIG_H
#define CONFIG_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Gets a configuration value
/// @param key The key to retrieve
/// @param index The index of the value
/// @param out_data The string to hold the result
/// @param out_data_len The length of the string holder
/// @return True if found, otherwise false
bool config_get_value(const char* key, int index, char* out_data, size_t out_data_len);
/// @brief Clears a key.
/// @param key The key to remove
/// @return True if successful, otherwise false
bool config_clear_values(const char* key);
/// @brief Adds a value to a key
/// @param key The key to add the value to
/// @param value The string value
/// @return True if successful, otherwise false
bool config_add_value(const char* key, const char* value);
#ifdef __cplusplus
}
#endif
#endif // CONFIG_H