#ifndef FS_H
#define FS_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
bool fs_external_init(void);
bool fs_internal_init(void);
bool fs_get_device_id(char* out_id, size_t out_id_length);
#ifdef __cplusplus
}
#endif
#endif // FS_H