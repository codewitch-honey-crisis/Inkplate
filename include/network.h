#ifndef NETWORK_H
#define NETWORK_H
#include <stdbool.h>
#include <stddef.h>
typedef enum { NET_UNINITIALIZED=-1, NET_WAITING, NET_CONNECTED, NET_CONNECT_FAILED } net_status_t;
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Begins initializing the network
/// @return true on success, false on error
bool net_init(void);
/// @brief Ends the network stack
void net_end(void);
/// @brief Reports the status of the network
/// @return One of net_status_t values
net_status_t net_status(void);
/// @brief Retrieves the address of the device
/// @param out_address A buffer to hold the address
/// @param out_address_length The length of the address buffer
/// @return true on success, false on error
bool net_get_address(char* out_address,size_t out_address_length);

bool net_get_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length);
bool net_set_credentials(const char* ssid, const char* pass);
bool net_get_stored_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length);
#ifdef __cplusplus
}
#endif
#endif