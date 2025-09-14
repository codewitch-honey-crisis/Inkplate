#pragma once
#include "ui.hpp"
/// @brief Initialize the weather UI
/// @return True if successful, otherwise false
bool ui_weather_init();
/// @brief Fetch the weather data and update the UI
/// @return The number of seconds until the next update, or -1 on error
long ui_weather_fetch();