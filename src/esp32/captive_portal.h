#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H
#include <stdbool.h>
#include <stddef.h>
typedef void(*captive_portal_callback_t)(void*);
#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initializes the captive portal
/// @return True on success, otherwise false
bool captive_portal_init(void);
/// @brief Ends the captive portal
/// @param True on success, otherwise false
void captive_portal_end(void);
/// @brief Sets a callback for when something connects to the portal
/// @param callback The callback
/// @param state Any user defined state to pass
void captive_portal_on_sta_connect(captive_portal_callback_t callback, void* state);
/// @brief Sets a callback for when something disconnects from the portal
/// @param callback The callback
/// @param state Any user defined state to pass
void captive_portal_on_sta_disconnect(captive_portal_callback_t callback, void* state);
/// @brief Retrieves the address of the device
/// @param out_address A buffer to hold the address
/// @param out_address_length The length of the address buffer
/// @return true on success, false on error
bool captive_portal_get_address(char* out_address,size_t out_address_length);
/// @brief Gets the credentials for the portal
/// @param out_ssid The buffer to hold the SSID
/// @param out_ssid_length The length of the SSID buffer
/// @param out_pass The buffer to hold the password
/// @param out_pass_length The length of the password
/// @return True on success, otherwise false
bool captive_portal_get_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length);
/// @brief Gets the size of the list of scanned AP addresses, not including the captive portal
/// @return The size of the list
size_t captive_portal_get_ap_list_size();
/// @brief Gets an AP SSID by index
/// @param index The index of the SSID to retrieve
/// @return A pointer to the SSID string (do not free it)
const char* captive_portal_get_ap_list_ssid(size_t index);
#ifdef __cplusplus
}
#endif
#endif // CAPTIVE_PORTAL_H