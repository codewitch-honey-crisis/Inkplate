#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
bool captive_portal_init(void);
void captive_portal_end(void);
/// @brief Retrieves the address of the device
/// @param out_address A buffer to hold the address
/// @param out_address_length The length of the address buffer
/// @return true on success, false on error
bool captive_portal_get_address(char* out_address,size_t out_address_length);
bool captive_portal_get_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length);
#ifdef __cplusplus
}
#endif
#endif // CAPTIVE_PORTAL_H