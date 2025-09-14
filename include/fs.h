#ifndef FS_H
#define FS_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initializes internal storage
/// @return True if successful, otherwise false
bool fs_external_init(void);
/// @brief Shuts down internal storage
void fs_external_end(void);
/// @brief Initializes removable/external storage 
/// @return True if successful, otherwise false
bool fs_internal_init(void);
/// @brief Gets the stored identifier for the device
/// @param out_id The string to hold the id
/// @param out_id_length The length of the string
/// @return True if successful, otherwise false
bool fs_get_device_id(char* out_id, size_t out_id_length);
#ifdef __cplusplus
}
#endif
#endif // FS_H