#pragma once
#include "ui.hpp"

/// @brief Initializes the captive portal UI
/// @return True on success, otherwise false
bool ui_captive_portal_init();
/// @brief Sets the initial AP connection screen of the captive portal UI
/// @return True on success, otherwise false
bool ui_captive_portal_set_ap(const char* address,const char* ssid, const char* pass);
/// @brief Sets the configuration screen of the captive portal UI
/// @return True on success, otherwise false
bool ui_captive_portal_set_configure(const char* address);
/// @brief Deinitializes the captive portal UI
void ui_captive_portal_end();
