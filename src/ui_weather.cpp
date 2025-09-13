#include "ui_weather.hpp"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "assets/compass.hpp"
#include "assets/connectivity.hpp"
#include "assets/weather.hpp"
#include "config.h"
#include "display.h"
#include "http.h"
#include "http_stream.hpp"
#include "json.hpp"
#include "timing.h"
#include "rtc_time.h"
#if defined(INKPLATE10) || defined(INKPLATE10V2)
#include "inkplate10/power.h"
#endif
using namespace gfx;
using namespace uix;
using namespace json;
using label_t = vlabel<surface_gsc_t>;
using icon_t = icon_box<surface_gsc_t>;

template <typename ControlSurfaceType>
class needle : public canvas_control<ControlSurfaceType> {
    using base_type = uix::canvas_control<ControlSurfaceType>;

   public:
    using type = needle;
    using control_surface_type = ControlSurfaceType;

   private:
    float m_angle;
    canvas_path m_path;
    pointf m_end;
    float m_radius;
    bool m_path_dirty;
    static void update_transform(float rotation, float& ctheta, float& stheta) {
        ctheta = cosf(rotation);
        stheta = sinf(rotation);
    }
    // transform a point given some thetas, a center and an offset
    static gfx::pointf transform_point(float ctheta, float stheta, gfx::pointf center, gfx::pointf offset, float x, float y) {
        float rx = (ctheta * (x - (float)center.x) - stheta * (y - (float)center.y) + (float)center.x) + offset.x;
        float ry = (stheta * (x - (float)center.x) + ctheta * (y - (float)center.y) + (float)center.y) + offset.y;
        return {(float)rx, (float)ry};
    }

   public:
    needle() : base_type(), m_angle(0), m_path_dirty(true) {
    }
    virtual ~needle() {
    }
    float angle() const {
        return m_angle;
    }
    void angle(float value) {
        m_angle = value;
        m_path_dirty = true;
        this->invalidate();
    }

   protected:
    virtual void on_before_paint() override {
        if (m_path_dirty) {
            if (gfx_result::success != m_path.initialize()) {
                return;
            }
            float w = this->dimensions().width;
            float h = this->dimensions().height;
            if (w > h) {
                w = h;
            }
            pointf center(w * .5f, w * .5f);
            pointf offset(0, 0);
            float ctheta, stheta;
            update_transform(m_angle, ctheta, stheta);
            m_path.clear();
            srect16 sr(0, w / 20, w / 20, w / 2);
            sr.center_horizontal_inplace(this->dimensions().bounds());
            m_end = pointf(sr.x1 + sr.width() * 0.5f, sr.y1 + (w / 10.f));
            m_radius = (w / 10.f);
            m_end = transform_point(ctheta, stheta, center, offset, m_end.x, m_end.y);
            m_path.move_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y1));
            m_path.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y2));
            m_path.line_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y2 + (w / 20)));
            m_path.line_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y2));
            m_path.close();
            m_path_dirty = false;
        }
    }
    virtual void on_paint(gfx::canvas& destination, const gfx::srect16& clip) override {
        if (m_path_dirty) {
            return;
        }
        gfx::canvas_style si = destination.style();
        si.fill_paint_type = gfx::paint_type::solid;
        si.stroke_paint_type = gfx::paint_type::none;
        si.fill_color = gfx::vector_pixel(255, 0, 0, 0);
        destination.style(si);
        destination.path(m_path);
        destination.render();
        si.fill_paint_type = gfx::paint_type::none;
        si.stroke_color = gfx::vector_pixel(255, 0, 0, 0);
        si.stroke_paint_type = gfx::paint_type::solid;
        si.stroke_width = 2;
        destination.style(si);
        destination.circle(m_end, m_radius - 1);
        destination.render();
    }
};
using needle_t = needle<surface_gsc_t>;

typedef struct {
    char area[64];
    char country[128];
    char last_updated[64];
    bool use_imperial_units;
    bool is_day;
    char temp[64];
    char condition[64];
    char pressure[32];
    char wind[64];
    char humidity[16];
    char precipitation[32];
    char visibility[16];
    char cloud_coverage[8];
} weather_info_t;

static screen_gsc_t weather_screen;
static char weather_units[64];
static char weather_location[256];
static const char* weather_api_url_part = "http://api.weatherapi.com/v1/current.json?key=f188c23cb291489389755443221206&aqi=no&q=";
static char weather_api_url[1025];
static weather_info_t weather_info;
static icon_t weather_connected_icon;
static icon_t weather_icon;
static icon_t weather_compass;
static needle_t weather_compass_needle;
static label_t weather_area_label;
static label_t weather_condition_label;
static label_t weather_updated_label;
#if defined(INKPLATE10) || defined(INKPLATE10V2)
static char battery_info[64];
static label_t weather_battery_label;
#endif
static label_t weather_temp_title_label;
static label_t weather_temp_label;
static label_t weather_wind_title_label;
static label_t weather_wind_label;
static label_t weather_precipitation_title_label;
static label_t weather_precipitation_label;
static label_t weather_humidity_title_label;
static label_t weather_humidity_label;
static label_t weather_visibility_title_label;
static label_t weather_visibility_label;
static label_t weather_pressure_title_label;
static label_t weather_pressure_label;

bool ui_weather_init() {
    weather_location[0] = '\0';
    weather_units[0] = '\0';
    // uix uses signed coords
    weather_screen.update_mode(screen_update_mode::direct);
    weather_screen.buffer_size(display_buffer_8bit_size());
    weather_screen.buffer1(display_buffer_8bit());
    weather_screen.dimensions((ssize16)screen_dimensions);
    weather_screen.background_color(scolor_gsc_t::white);

    const float fheight = screen_dimensions.width / 8.f;
    const float ftheight = fheight / 2.f;
    weather_icon.bounds(srect16(spoint16::zero(), ssize16(screen_dimensions.width / 8, screen_dimensions.width / 8)));
    weather_icon.svg_size(connectivity_wifi_dimensions);
    weather_icon.svg(connectivity_wifi);
    weather_screen.register_control(weather_icon);
    srect16 sr = weather_icon.bounds();
    sr.offset_inplace(weather_screen.dimensions().width - weather_icon.dimensions().width, 0);
    const float deflate = -screen_dimensions.width / 120;
    weather_compass.bounds(sr.inflate(deflate, deflate));
    weather_compass.svg_size(compass_dimensions);
    weather_compass.svg(compass);
    weather_screen.register_control(weather_compass);
    weather_compass_needle.bounds(sr);
    weather_screen.register_control(weather_compass_needle);
    sr = weather_icon.bounds();
    sr.offset_inplace(weather_icon.dimensions().width + 2, 0);
    sr.y2 /= 2;
    sr.y2 -= 1;
    sr.x2 = weather_screen.bounds().x2/2;
    weather_area_label.bounds(sr);
    weather_area_label.font(text_font);
    constexpr static const uint8_t LP = .5f * 255;
    constexpr static const uint8_t L = .6666f * 255;
    weather_area_label.color(rgba_pixel<32>(LP, LP, LP, 255));
    weather_area_label.text("Fetching...");
    weather_screen.register_control(weather_area_label);

    sr.offset_inplace(0, sr.height() + 2);
    weather_condition_label.bounds(sr);
    weather_condition_label.text("Fetching...");
    weather_condition_label.font(text_font);
    weather_condition_label.color(ucolor_t::black);
    weather_screen.register_control(weather_condition_label);
#if defined(INKPLATE10) || defined(INKPLATE10V2)
    sr.offset_inplace(sr.width()+1,0);
    weather_battery_label.bounds(sr);
    weather_battery_label.text("");
    weather_battery_label.font(text_font);
    weather_battery_label.color(ucolor_t::black);
    weather_screen.register_control(weather_battery_label);
#endif

    sr = weather_area_label.bounds();
    sr.offset_inplace(sr.width()+1,0);
    sr.x2-=weather_compass_needle.dimensions().width;
    weather_updated_label.bounds(sr);
    weather_updated_label.text("Fetching...");
    weather_updated_label.font(text_font);
    weather_updated_label.color(ucolor_t::black);
    weather_screen.register_control(weather_updated_label);

    sr = srect16(0, weather_icon.bounds().y2 + 2, weather_screen.bounds().x2 / 2 - 1, weather_icon.bounds().y2 + 2 + (fheight / 2));
    weather_temp_title_label.bounds(sr);
    weather_temp_title_label.background_color(rgba_pixel<32>(L, L, L, 255));
    weather_temp_title_label.font(text_font);
    weather_temp_title_label.color(ucolor_t::white);
    weather_temp_title_label.text("TEMPERATURE");
    weather_temp_title_label.font_size(ftheight);
    weather_temp_title_label.text_justify(uix_justify::center);
    weather_screen.register_control(weather_temp_title_label);

    sr.offset_inplace(sr.width() + 2, 0);
    weather_wind_title_label.bounds(sr);
    weather_wind_title_label.background_color(rgba_pixel<32>(L, L, L, 255));
    weather_wind_title_label.font(text_font);
    weather_wind_title_label.color(ucolor_t::white);
    weather_wind_title_label.text("WIND");
    weather_wind_title_label.font_size(ftheight);
    weather_wind_title_label.text_justify(uix_justify::center);
    weather_screen.register_control(weather_wind_title_label);

    sr = weather_temp_title_label.bounds().offset(0, weather_temp_title_label.dimensions().height + 1);
    sr.y2 += (fheight / 2);
    weather_temp_label.bounds(sr);
    weather_temp_label.text("Fetching...");
    weather_temp_label.font(text_font);
    weather_temp_label.color(ucolor_t::black);
    weather_screen.register_control(weather_temp_label);

    sr = weather_wind_title_label.bounds().offset(0, weather_wind_title_label.dimensions().height + 1);
    sr.y2 += (fheight / 2);
    weather_wind_label.bounds(sr);
    weather_wind_label.text("Fetching...");
    weather_wind_label.font(text_font);
    weather_wind_label.color(ucolor_t::black);
    weather_screen.register_control(weather_wind_label);

    sr = weather_temp_title_label.bounds();
    sr.offset_inplace(0, (sr.height() * 2) + (fheight / 2));
    weather_precipitation_title_label.bounds(sr);
    weather_precipitation_title_label.text("PRECIPITATION");
    weather_precipitation_title_label.font(text_font);
    weather_precipitation_title_label.background_color(rgba_pixel<32>(L, L, L, 255));
    weather_precipitation_title_label.color(ucolor_t::white);
    weather_precipitation_title_label.text_justify(uix_justify::center);
    weather_precipitation_title_label.font_size(ftheight);
    weather_screen.register_control(weather_precipitation_title_label);

    sr.offset_inplace(sr.width() + 2, 0);
    weather_humidity_title_label.bounds(sr);
    weather_humidity_title_label.text("HUMIDITY");
    weather_humidity_title_label.font(text_font);
    weather_humidity_title_label.background_color(rgba_pixel<32>(L, L, L, 255));
    weather_humidity_title_label.color(ucolor_t::white);
    weather_humidity_title_label.text_justify(uix_justify::center);
    weather_humidity_title_label.font_size(ftheight);
    weather_screen.register_control(weather_humidity_title_label);

    sr = weather_precipitation_title_label.bounds();
    sr.y2 += (fheight / 2);
    sr.offset_inplace(0, weather_precipitation_title_label.dimensions().height + 1);
    weather_precipitation_label.bounds(sr);
    weather_precipitation_label.text("Fetching...");
    weather_precipitation_label.font(text_font);
    weather_precipitation_label.color(ucolor_t::black);
    weather_screen.register_control(weather_precipitation_label);

    sr = weather_humidity_title_label.bounds().offset(0, weather_humidity_title_label.dimensions().height + 1);
    sr.y2 += (fheight / 2);
    weather_humidity_label.bounds(sr);
    weather_humidity_label.text("Fetching...");
    weather_humidity_label.font(text_font);
    weather_humidity_label.color(ucolor_t::black);
    weather_screen.register_control(weather_humidity_label);

    sr = weather_precipitation_title_label.bounds();
    sr.offset_inplace(0, (sr.height() * 2) + (fheight / 2));
    weather_visibility_title_label.bounds(sr);
    weather_visibility_title_label.text("VISIBILITY");
    weather_visibility_title_label.font(text_font);
    weather_visibility_title_label.background_color(rgba_pixel<32>(L, L, L, 255));
    weather_visibility_title_label.color(ucolor_t::white);
    weather_visibility_title_label.text_justify(uix_justify::center);
    weather_visibility_title_label.font_size(ftheight);
    weather_screen.register_control(weather_visibility_title_label);

    sr.offset_inplace(sr.width() + 2, 0);
    weather_pressure_title_label.bounds(sr);
    weather_pressure_title_label.text("PRESSURE");
    weather_pressure_title_label.font(text_font);
    weather_pressure_title_label.background_color(rgba_pixel<32>(L, L, L, 255));
    weather_pressure_title_label.color(ucolor_t::white);
    weather_pressure_title_label.text_justify(uix_justify::center);
    weather_pressure_title_label.font_size(ftheight);
    weather_screen.register_control(weather_pressure_title_label);

    sr = weather_visibility_title_label.bounds();
    sr.y2 += (fheight / 2);
    sr.offset_inplace(0, weather_visibility_title_label.dimensions().height + 1);
    weather_visibility_label.bounds(sr);
    weather_visibility_label.text("Fetching...");
    weather_visibility_label.font(text_font);
    weather_visibility_label.color(ucolor_t::black);
    weather_screen.register_control(weather_visibility_label);

    sr = weather_pressure_title_label.bounds().offset(0, weather_pressure_title_label.dimensions().height + 1);
    sr.y2 += (fheight / 2);
    weather_pressure_label.bounds(sr);
    weather_pressure_label.text("Fetching...");
    weather_pressure_label.font(text_font);
    weather_pressure_label.color(ucolor_t::black);
    if (uix_result::success != weather_screen.register_control(weather_pressure_label)) {
        weather_screen.unregister_controls();
        return false;
    }
    return true;
}
static float to_fahrenheit(float celcius) {
    return celcius * (9.f / 5.f) + 32.f;
}
static float to_miles(float km) {
    return km * 0.621371f;
}
static float to_inches(float mm) {
    return mm * 0.0393701f;
}
static float to_psi(float mb) {
    return mb * 0.0145038f;
}
long ui_weather_fetch() {
    uint32_t start_ts = timing_get_ms();
    if (weather_screen.dimensions().width == 0) {
        return 0;
    }
    if (weather_location[0] == '\0') {
        if (!config_get_value("location", 0, weather_location, sizeof(weather_location))) {
            return 0;
        }
        if (weather_location[0] == '\0') {
            return 0;
        }
        http_url_encode(weather_location, sizeof(weather_location), weather_location, http_enc_rfc3986);
        strcpy(weather_api_url, weather_api_url_part);
        strcat(weather_api_url, weather_location);
    }
    if (weather_units[0] == '\0') {
        if (!config_get_value("units", 0, weather_units, sizeof(weather_units))) {
            strcpy(weather_units, "auto");
        }
    }
#if defined(INKPLATE10) || defined(INKPLATE10V2)
    battery_info[0]='\0';
    const float batt_voltage = power_battery_voltage();
    if(batt_voltage!=0) {
        const float batt_level = power_battery_level();
        snprintf(battery_info,sizeof(battery_info)-1,"Charge: %0.f%% (%0.2fv)",batt_level*100.f,batt_voltage);
        weather_battery_label.text(battery_info);
    }
#endif
    http_handle_t handle = http_init(weather_api_url);
    int status = http_read_status_and_headers(handle);
    enum {
        J_START = 0,
        J_LOC,
        J_CUR
    };
    if (status >= 200 && status <= 299) {
        http_stream stm(handle);
        json_reader_ex<64> reader(stm);
        // time_t last_update;
        float temp_c = 0, feels_c = 0, wind_kph = 0, precip_mm = 0, gust_kph = 0, vis_km = 0, pressure_mb = 0;
        float wind_angle = 0;
        int humidity = 0, cloud = 0;
        time_t last_updated;
        time_t local_time;
        char wind_dir[16];
        // char wind_gust[16];
        wind_dir[0] = '\0';
        // wind_gust[0]='\0';
        int state = J_START;
        bool success = false;
        while (reader.read()) {
            if (reader.depth() == 1) {
                if (reader.node_type() == json_node_type::field && 0 == strcmp("location", reader.value())) {
                    state = J_LOC;
                } else if (reader.node_type() == json_node_type::field && 0 == strcmp("current", reader.value())) {
                    state = J_CUR;
                }
            } else {
                if (state == J_LOC) {
                    if (reader.node_type() == json_node_type::field) {
                        if (0 == strcmp("name", reader.value()) && reader.read()) {
                            strcpy(weather_info.area, reader.value());
                        } else if (0 == strcmp("country", reader.value()) && reader.read()) {
                            strcpy(weather_info.country, reader.value());
                        } else if (0 == strcmp("localtime_epoch", reader.value()) && reader.read()) {
                            local_time = (time_t)reader.value_int();
                        }
                        
                    }
                } else if (state == J_CUR) {
                    if (reader.node_type() == json_node_type::field) {
                        success = true;
                        if (0 == strcmp("text", reader.value()) && reader.read()) {
                            strcpy(weather_info.condition, reader.value());
                        } else if (0 == strcmp("is_day", reader.value()) && reader.read()) {
                            weather_info.is_day = reader.value_bool();
                        } else if (0 == strcmp("temp_c", reader.value()) && reader.read()) {
                            temp_c = reader.value_real();
                        } else if (0 == strcmp("feelslike_c", reader.value()) && reader.read()) {
                            feels_c = reader.value_real();
                        } else if (0 == strcmp("wind_kph", reader.value()) && reader.read()) {
                            wind_kph = reader.value_real();
                        } else if (0 == strcmp("wind_dir", reader.value()) && reader.read()) {
                            strcpy(wind_dir, reader.value());
                        } else if (0 == strcmp("precip_mm", reader.value()) && reader.read()) {
                            precip_mm = reader.value_real();
                        } else if (0 == strcmp("humidity", reader.value()) && reader.read()) {
                            humidity = reader.value_int();
                        } else if (0 == strcmp("cloud", reader.value()) && reader.read()) {
                            cloud = reader.value_int();
                        } else if (0 == strcmp("gust_kph", reader.value()) && reader.read()) {
                            gust_kph = reader.value_real();
                        } else if (0 == strcmp("wind_degree", reader.value()) && reader.read()) {
                            wind_angle = gfx::math::deg2rad((reader.value_int()) % 360);
                        } else if (0 == strcmp("vis_km", reader.value()) && reader.read()) {
                            vis_km = reader.value_real();
                        } else if (0 == strcmp("pressure_mb", reader.value()) && reader.read()) {
                            pressure_mb = reader.value_real();
                        } else if (0 == strcmp("last_updated_epoch", reader.value()) && reader.read()) {
                            last_updated = (time_t)reader.value_int();
                        }
                    }
                }
            }
        }
        http_end(handle);
        if (success) {
            
            uint32_t fetch_time_ms = timing_get_ms() - start_ts;
            printf("Weather fetch time: %ldms\n", (long)fetch_time_ms);
            bool is_imperial = (0 == strcmp(weather_units, "imperial") || (0 == strcmp(weather_units, "auto") && 0 == strcmp(weather_info.country, "USA")));
            weather_area_label.text(weather_info.area);
            weather_condition_label.text(weather_info.condition);
            strcpy(weather_info.last_updated,"Updated: ");
            long result = (last_updated+(15*60))-local_time;
            if(result<=0) {
                result = 15*60;
            }
            printf("Next update in %ld minutes\n",result/60);
            long offs=0;
            rtc_time_get_tz_offset(&offs);
            last_updated += offs;
            tm* t_tm= localtime(&last_updated);
            strftime(weather_info.last_updated+strlen(weather_info.last_updated),32,"%I:%M %p",t_tm);
            weather_updated_label.text(weather_info.last_updated);
            weather_compass_needle.angle(wind_angle);
            if (is_imperial) {
                sprintf(weather_info.temp, "%0.1fF (feels %0.1fF)", to_fahrenheit(temp_c), to_fahrenheit(feels_c));
            } else {
                sprintf(weather_info.temp, "%0.1fC (feels %0.1fC)", temp_c, feels_c);
            }
            weather_temp_label.text(weather_info.temp);
            
            if (is_imperial) {
                snprintf(weather_info.wind, sizeof(weather_info.wind), "%s %0.1fMPH (gust %0.1fMPH)", wind_dir, to_miles(wind_kph), to_miles(gust_kph));
            } else {
                snprintf(weather_info.wind, sizeof(weather_info.wind), "%s %0.1fKPH (gust %0.1fKPH)", wind_dir, wind_kph, gust_kph);
            }
            weather_wind_label.text(weather_info.wind);
            if (precip_mm > 0.f) {
                if (is_imperial) {
                    sprintf(weather_info.precipitation, "%0.2fin", to_inches(precip_mm));
                } else {
                    sprintf(weather_info.precipitation, "%0.1fmm", precip_mm);
                }
            } else {
                sprintf(weather_info.precipitation, "n/a");
            }
            weather_precipitation_label.text(weather_info.precipitation);
            sprintf(weather_info.humidity, "%d%%", humidity);
            weather_humidity_label.text(weather_info.humidity);
            if (is_imperial) {
                sprintf(weather_info.visibility, "%0.1fmi", to_miles(vis_km));
            } else {
                sprintf(weather_info.visibility, "%0.1fkm", vis_km);
            }
            weather_visibility_label.text(weather_info.visibility);
            if (is_imperial) {
                sprintf(weather_info.pressure, "%0.1fpsi", to_psi(pressure_mb));
            } else {
                sprintf(weather_info.pressure, "%0.1fmb", pressure_mb);
            }

            weather_pressure_label.text(weather_info.pressure);
            if (precip_mm > 250) {
                weather_icon.svg(weather_cloud_showers_heavy);
            } else if (weather_info.is_day) {
                if (precip_mm > .5f) {
                    if (temp_c <= 0.f) {
                        weather_icon.svg(weather_snowflake);
                    } else {
                        weather_icon.svg(weather_cloud_rain);
                    }
                } else {
                    if (cloud >= 25) {
                        weather_icon.svg(weather_cloud);
                    } else {
                        weather_icon.svg(weather_sun);
                    }
                }
            } else {
                if (precip_mm > .5f) {
                    if (temp_c <= 0.f) {
                        weather_icon.svg(weather_snowflake);
                    } else {
                        weather_icon.svg(weather_cloud_moon_rain);
                    }
                } else {
                    if (cloud >= 25) {
                        weather_icon.svg(weather_cloud_moon);
                    } else {
                        weather_icon.svg(weather_moon);
                    }
                }
            }
            bool washing = !display_washed();
            if (washing) {
                display_wash_8bit_async();
            }
            uint32_t ui_start_ts = timing_get_ms();
            puts("UI update started");
            weather_screen.update();
            printf("UI updated in %ldms\n", (long)(timing_get_ms() - ui_start_ts));
            if (washing) {
                display_wash_8bit_wait();
            }
            puts("Begin display panel transfer");
            uint32_t transfer_ts = timing_get_ms();
            if (display_update_8bit()) {
                printf("Display panel transfer complete in %ldms. Sleeping.\n",(long)(timing_get_ms()-transfer_ts));
                display_sleep();
                return result;
            }
        }
        return 0;
    }
    http_end(handle);
    return 0;
}