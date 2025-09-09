#pragma once
#include "ui.hpp"

bool ui_captive_portal_init();
bool ui_captive_portal_set_ap(const char* address,const char* ssid, const char* pass);
bool ui_captive_portal_set_url(const char* address);
void ui_captive_portal_end();