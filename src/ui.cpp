#include "ui.hpp"

#define TELEGRAMA_RENDER_IMPLEMENTATION
#include "assets/telegrama_render.hpp"
#define WEATHER_IMPLEMENTATION
#include "assets/weather.hpp"
#define CONNECTIVITY_IMPLEMENTATION
#include "assets/connectivity.hpp"

gfx::const_buffer_stream& text_font = telegrama_render;
