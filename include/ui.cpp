#include "ui.hpp"
#include <time.h>
#include <stddef.h>
#include "http.h"
#include "http_stream.hpp"
#include "json.hpp"
#include "config.h"
#include "display.h"
#define TELEGRAMA_RENDER_IMPLEMENTATION
#include "assets/telegrama_render.hpp"
#define WEATHER_IMPLEMENTATION
#include "assets/weather.hpp"
#define CONNECTIVITY_IMPLEMENTATION
#include "assets/connectivity.hpp"

gfx::const_buffer_stream& text_font = telegrama_render;
using namespace gfx;
using namespace uix;
using namespace json;

template<typename ControlSurfaceType>
class vlabel : public canvas_control<ControlSurfaceType> {
    using base_type = canvas_control<ControlSurfaceType>;
public:
    using type = vlabel;
    using control_surface_type = ControlSurfaceType;
private:
    canvas_text_info m_label_text;
    canvas_path m_label_text_path;
    rectf m_label_text_bounds;
    bool m_label_text_dirty;
    vector_pixel m_color;
    rgba_pixel<32> m_back_color;
    stream* m_font_stream;
    void build_label_path_untransformed() {
        if(m_font_stream==nullptr) {
            return;
        }
        m_label_text.ttf_font = m_font_stream;
        const float target_width = this->dimensions().width;
        float fsize = this->dimensions().height*.8f;
        m_label_text_path.initialize();
        do {
            m_label_text_path.clear();
            m_label_text.font_size = fsize;
            m_label_text_path.text({0.f,0.f},m_label_text);
            m_label_text_bounds = m_label_text_path.bounds(true);
            --fsize;
            
        } while(fsize>0.f && m_label_text_bounds.width()>=target_width);
    }
public:
    vlabel() : base_type() ,m_label_text_dirty(true) {
        m_font_stream = nullptr;
        m_label_text.text_sz("Label");
        m_label_text.encoding = &text_encoding::utf8;
        m_label_text.ttf_font_face = 0;
        m_color = vector_pixel(255,255,255,255);
        m_back_color = rgba_pixel<32>(0,true);
    }
    virtual ~vlabel() {

    }
    text_handle text() const {
        return m_label_text.text;
    }
    void text(text_handle text, size_t text_byte_count) {
        m_label_text.text=text;
        m_label_text.text_byte_count = text_byte_count;
        m_label_text_dirty = true;
        this->invalidate();
    }
    void text(const char* sz) {
        m_label_text.text_sz(sz);
        m_label_text_dirty = true;
        this->invalidate();
    }
        stream& font() const {
        return *m_font_stream;
    }
    void font(stream& value) {
        m_font_stream = &value;
        m_label_text_dirty = true;
        this->invalidate();
    }
    rgba_pixel<32> color() const {
        rgba_pixel<32> result;
        convert(m_color,&result);
        return result;
    }
    void color(rgba_pixel<32> value) {
        convert(value,&m_color);
        this->invalidate();
    }
    rgba_pixel<32> back_color() const {
        return m_back_color;
    }
    void back_color(rgba_pixel<32> value) {
        m_back_color = value;
        this->invalidate();
    }
protected:
    virtual void on_before_paint() override {
        if(m_label_text_dirty) {
            build_label_path_untransformed();
            m_label_text_dirty = false;
        }
    }
    virtual void on_paint(control_surface_type& destination, const srect16& clip) {
        if(m_back_color.opacity()!=0) {
            draw::filled_rectangle(destination,destination.bounds(),m_back_color);
        }
        base_type::on_paint(destination,clip);
    }
    virtual void on_paint(canvas& destination, const srect16& clip) override {
        if(m_font_stream==nullptr) {
            return;
        }
        canvas_style si = destination.style();
        si.fill_paint_type = paint_type::solid;
        si.stroke_paint_type = paint_type::none;
        si.fill_color = m_color;
        destination.style(si);
        // save the current transform
        matrix old = destination.transform();
        matrix m = matrix::create_identity();
        m=m.translate(-m_label_text_bounds.x1,(-m_label_text_bounds.y1)+(0/*destination.dimensions().height-m_label_text_bounds.height()*/)*0.5f);
        destination.transform(m);
        destination.path(m_label_text_path);
        destination.render();
        // restore the old transform
        destination.transform(old);
    }

};
template <typename ControlSurfaceType>
class icon_box : public control<ControlSurfaceType> {
    using base_type = control<ControlSurfaceType>;

   public:
    typedef void (*on_pressed_changed_callback_type)(bool pressed, void* state);

   private:
    bool m_pressed;
    bool m_dirty;
    sizef m_svg_size;
    matrix m_fit;
    on_pressed_changed_callback_type m_on_pressed_changed_callback;
    void* m_on_pressed_changed_callback_state;
    stream* m_svg;
    static rectf correct_aspect(srect16& sr, float aspect) {
        if (aspect>=1.f) {
            sr.y2 /= aspect;
        } else {
            sr.x2 *= aspect;
        }
        return (rectf)sr;
    }

   public:
    icon_box()
        : base_type(),
          m_pressed(false),
          m_dirty(true),
          m_on_pressed_changed_callback(nullptr),
          m_svg(nullptr) {m_svg_size = {0.f,0.f};}
    stream& svg() const { return *m_svg; }
    void svg(stream& value) {
        m_svg = &value;
        m_dirty = true;
    }
    sizef svg_size() const { return m_svg_size; }
    void svg_size(const sizef& value) {
        m_svg_size = value;
        m_dirty = true;
    }
    
    bool pressed() const { return m_pressed; }
    void on_pressed_changed_callback(on_pressed_changed_callback_type callback,
                                     void* state = nullptr) {
        m_on_pressed_changed_callback = callback;
        m_on_pressed_changed_callback_state = state;
    }

   protected:
    virtual void on_before_paint() override {
        if (m_dirty) {
            if (m_svg != nullptr) {
                if(m_svg_size.width==0.f || m_svg_size.height==0.f) {
                    m_svg->seek(0);
                    canvas::svg_dimensions(*m_svg, &m_svg_size);
                }
                srect16 sr = this->dimensions().bounds();
                rectf corrected = correct_aspect(sr, m_svg_size.aspect_ratio());
                corrected.center_inplace((rectf)sr);
                m_fit = matrix::create_fit_to(m_svg_size,corrected);
            }
            m_dirty = false;
        }
    }
    virtual void on_paint(ControlSurfaceType& dst,
                          const srect16& clip) override {
        if (m_dirty || m_svg == nullptr) {
           
            return;
        }
        canvas cvs((size16)this->dimensions());
        cvs.initialize();
        draw::canvas(dst, cvs);
        m_svg->seek(0);
        if (gfx_result::success != cvs.render_svg(*m_svg, m_fit)) {
            puts("SVG render error");
        }
    }
    virtual bool on_touch(size_t locations_size,
                          const spoint16* locations) override {
        if (!m_pressed) {
            m_pressed = true;
            if (m_on_pressed_changed_callback != nullptr) {
                m_on_pressed_changed_callback(
                    true, m_on_pressed_changed_callback_state);
            }
            m_dirty = true;
            this->invalidate();
        }
        return true;
    }
    virtual void on_release() override {
        if (m_pressed) {
            m_pressed = false;
            if (m_on_pressed_changed_callback != nullptr) {
                m_on_pressed_changed_callback(
                    false, m_on_pressed_changed_callback_state);
            }
            m_dirty = true;
            this->invalidate();
        }
    }
};

using label3_t = vlabel<surface3_t>;
using icon3_t = icon_box<surface3_t>;

using label1_t = vlabel<surface1_t>;
using qr1_t = qrcode<surface1_t>;

screen3_t weather_screen;
/*
"current": {
        "last_updated_epoch": 1756691100,
        "temp_c": 21.4,
        "is_day": 1,
        "condition": {
            "text": "Partly Cloudy"
        },
        "wind_kph": 5.4,
        "wind_degree": 248,
        "wind_dir": "WSW",
        "pressure_mb": 1016.0,
        "precip_mm": 0.0,
        "humidity": 78,
        "cloud": 0,
        "feelslike_c": 21.4,
        "vis_km": 16.0,
        "gust_kph": 6.5
    }
*/
typedef struct {
    char area[64];
    char last_updated[64];
    bool use_imperial_units;
    bool is_day;
    char temp[64];
    char condition[64];
    char wind[64];
    char humidity[16];
    char precipitation[16];
    char visibility[16];
    char cloud_coverage[8];
} weather_info_t;

static char weather_location[256];
static const char* weather_api_url_part= "http://api.weatherapi.com/v1/current.json?key=f188c23cb291489389755443221206&aqi=no&q=";
static char weather_api_url[1025];
static weather_info_t weather_info;
static icon3_t weather_connected_icon;
static icon3_t weather_icon;
static label3_t weather_area_label;
static label3_t weather_condition_label;

static label3_t weather_temp_title_label;
static label3_t weather_temp_label;
static label3_t weather_wind_title_label;
static label3_t weather_wind_label;
static label3_t weather_precipitation_title_label;
static label3_t weather_precipitation_label;
static label3_t weather_humidity_title_label;
static label3_t weather_humidity_label;

static char setup_qr_creds[256];
static char setup_url[128+5];
static char setup_ssid[65+6];
static char setup_pass[129+6];
static screen1_t setup_screen;
static label1_t setup_label;
static label1_t setup_cred_url_label;
static label1_t setup_cred_ssid_label;
static label1_t setup_cred_pass_label;
static qr1_t setup_qr;

static const constexpr size_t xfer_buffer_size = (screen_dimensions.width*screen_dimensions.height+79)/80;
static uint8_t* xfer_buffer = NULL;

static void setup_flush(const rect16& bounds, const void* bmp, void* state) {
    const_bitmap<gsc1_pixel_t> src(bounds.dimensions(),bmp);
    bitmap<gsc1_pixel_t> dst(screen_dimensions,display_buffer_1bit());
    draw::bitmap(dst,bounds,src,src.bounds());
    setup_screen.flush_complete();
}

bool ui_init() {
    if(xfer_buffer!=NULL) {
        return true;
    }
    if (!display_init()) {
        return false;
    }

    xfer_buffer = (uint8_t*)malloc(xfer_buffer_size);
    if (xfer_buffer == NULL) {
        return false;
    }
    weather_location[0]='\0';
    // uix uses signed coords
    weather_screen.update_mode(screen_update_mode::direct);
    weather_screen.buffer_size(display_buffer_3bit_size());
    weather_screen.buffer1(display_buffer_3bit());
    weather_screen.dimensions((ssize16)screen_dimensions);
    weather_screen.background_color(scolor3_t::white);
    float fheight = screen_dimensions.width/8.f;
    weather_icon.bounds(srect16(spoint16::zero(),ssize16(screen_dimensions.width/8,screen_dimensions.width/8)));
    weather_icon.svg_size(connectivity_wifi_dimensions);
    weather_icon.svg(connectivity_wifi);
    weather_screen.register_control(weather_icon);

    srect16 sr = weather_icon.bounds();
    sr.offset_inplace(weather_icon.dimensions().width+2,0);
    sr.y2/=2;
    sr.y2-=1;
    sr.x2 = weather_screen.bounds().x2;
    weather_area_label.bounds(sr);
    weather_area_label.font(text_font);
    constexpr static const uint8_t L = .3333f * 255;
    weather_area_label.color(rgba_pixel<32>(L,L,L,255));
    weather_area_label.text("Fetching...");
    weather_screen.register_control(weather_area_label);

    sr.offset_inplace(0,sr.height()+2);
    weather_condition_label.bounds(sr);
    weather_condition_label.text("Fetching...");
    weather_condition_label.font(text_font);
    weather_condition_label.color(ucolor_t::black);
    weather_screen.register_control(weather_condition_label);

    sr = srect16(0,weather_icon.bounds().y2+2,weather_screen.bounds().x2/2-1,weather_icon.bounds().y2+2+(fheight/2));
    weather_temp_title_label.bounds(sr);
    weather_temp_title_label.back_color(rgba_pixel<32>(L,L,L,255));
    weather_temp_title_label.font(text_font);
    weather_temp_title_label.color(ucolor_t::white);
    weather_temp_title_label.text("TEMPERATURE");
    weather_screen.register_control(weather_temp_title_label);

    sr.offset_inplace(sr.width()+2,0);
    weather_wind_title_label.bounds(sr);
    weather_wind_title_label.back_color(rgba_pixel<32>(L,L,L,255));
    weather_wind_title_label.font(text_font);
    weather_wind_title_label.color(ucolor_t::white);
    weather_wind_title_label.text("WIND");
    weather_screen.register_control(weather_wind_title_label);
    
    sr = weather_temp_title_label.bounds().offset(0,weather_temp_title_label.dimensions().height+1);
    sr.y2+=(fheight/2);
    weather_temp_label.bounds(sr);
    weather_temp_label.text("Fetching...");
    weather_temp_label.font(text_font);
    weather_temp_label.color(ucolor_t::black);
    weather_screen.register_control(weather_temp_label);

    sr = weather_wind_title_label.bounds().offset(0,weather_wind_title_label.dimensions().height+1);
    sr.y2+=(fheight/2);
    weather_wind_label.bounds(sr);
    weather_wind_label.text("Fetching...");
    weather_wind_label.font(text_font);
    weather_area_label.color(ucolor_t::black);
    weather_screen.register_control(weather_wind_label);

    sr = weather_temp_title_label.bounds();
    int h = sr.height();
    sr.y1 = weather_temp_label.bounds().y2+2;
    sr.y2+=h-1;

    weather_precipitation_title_label.bounds(sr);
    weather_precipitation_title_label.text("PRECIPITATION");
    weather_precipitation_title_label.font(text_font);
    weather_precipitation_title_label.back_color(rgba_pixel<32>(L,L,L,255));
    weather_precipitation_title_label.color(ucolor_t::white);
    weather_screen.register_control(weather_precipitation_title_label);
    
    sr.offset_inplace(sr.width()+2,0);
    weather_humidity_title_label.bounds(sr);
    weather_humidity_title_label.text("HUMIDITY");
    weather_humidity_title_label.font(text_font);
    weather_humidity_title_label.back_color(rgba_pixel<32>(L,L,L,255));
    weather_humidity_title_label.color(ucolor_t::white);
    weather_screen.register_control(weather_humidity_title_label);
    
    sr = weather_precipitation_title_label.bounds().offset(0,weather_precipitation_title_label.dimensions().height+1);
    sr.y2+=(fheight/2);
    weather_precipitation_label.bounds(sr);
    weather_precipitation_label.text("Fetching...");
    weather_precipitation_label.font(text_font);
    weather_precipitation_label.color(ucolor_t::black);
    weather_screen.register_control(weather_precipitation_label);
    
    sr = weather_humidity_title_label.bounds().offset(0,weather_humidity_title_label.dimensions().height+1);
    sr.y2+=(fheight/2);
    weather_humidity_label.bounds(sr);
    weather_humidity_label.text("Fetching...");
    weather_humidity_label.font(text_font);
    weather_humidity_label.color(ucolor_t::black);
    weather_screen.register_control(weather_humidity_label);
    
    setup_screen.buffer_size(xfer_buffer_size);
    setup_screen.buffer1(xfer_buffer);
    setup_screen.on_flush_callback(setup_flush);
    setup_screen.dimensions((ssize16)screen_dimensions);
    setup_screen.background_color(scolor1_t::white);
    setup_label.bounds(srect16(0,0,weather_screen.bounds().x2,fheight+1));
    setup_label.font(text_font);
    setup_label.color(ucolor_t::black);
    setup_label.text("Configure device");
    setup_screen.register_control(setup_label);
        
    setup_cred_url_label.bounds(srect16(0,0,weather_screen.bounds().x2,fheight+1).offset(0,screen_dimensions.height-(fheight*3+3)));
    setup_cred_url_label.font(text_font);
    setup_cred_url_label.color(ucolor_t::black);
    setup_cred_url_label.text("URL: <unknown>");
    setup_screen.register_control(setup_cred_url_label);

    setup_cred_ssid_label.bounds(setup_cred_url_label.bounds().offset(0,fheight+1));
    setup_cred_ssid_label.font(text_font);
    setup_cred_ssid_label.color(ucolor_t::black);
    setup_cred_ssid_label.text("SSID: <unknown>");
    setup_screen.register_control(setup_cred_ssid_label);

    setup_cred_pass_label.bounds(setup_cred_ssid_label.bounds().offset(0,fheight+1));
    setup_cred_pass_label.font(text_font);
    setup_cred_pass_label.color(ucolor_t::black);
    setup_cred_pass_label.text("Pass: <unknown>");
    setup_qr.bounds(srect16(spoint16::zero(),ssize16(screen_dimensions.width/4,screen_dimensions.width/4)).center_horizontal(setup_screen.bounds()).offset(0,fheight+2));
    if(uix_result::success!=setup_screen.register_control(setup_qr)) {
        // if any out of memory errors occurred above, this will fail as well. so we just check this.
        goto error;
    }
    return true;
error:
    weather_screen.unregister_controls();
    setup_screen.unregister_controls();
    return false;
}

bool ui_fetch_weather() {
    if(xfer_buffer==NULL) return false; // not initialized
    if(weather_location[0]=='\0') {
        if(!config_get_value("location",0,weather_location,sizeof(weather_location))) {
            return false;
        }
        if(weather_location[0]=='\0') {
            return false;
        }
        http_url_encode(weather_location,sizeof(weather_location),weather_location,http_enc_rfc3986);
        strcpy(weather_api_url,weather_api_url_part);
        strcat(weather_api_url,weather_location);
    }
    
    http_handle_t handle = http_init(weather_api_url);
    int status =  http_read_status_and_headers(handle);
    printf("HTTP status: %d for %s\n",status,weather_api_url);
    enum {
        J_START=0,
        J_LOC,
        J_CUR
    };
    if(status>=200 && status<=299) {
        http_stream stm(handle);
        json_reader_ex<64> reader(stm);
        time_t last_update;
        float temp_c=0, feels_c=0, wind_kph=0, precip_mm=0, vis_km=0, gust_kph=0;
        int humidity=0, cloud=0;
        char wind_dir[16];
        char wind_gust[16];
        wind_dir[0]='\0';
        wind_gust[0]='\0';
        int state = J_START; 
        bool success = false;
        while(reader.read()) {
            if(reader.depth()==1) {
                if(reader.node_type()==json_node_type::field && 0==strcmp("location",reader.value())) {
                    state = J_LOC;
                } else if(reader.node_type()==json_node_type::field && 0==strcmp("current",reader.value())) {
                    state = J_CUR;
                }
            } else {
                if(state==J_LOC) {
                    if(reader.node_type()==json_node_type::field) {
                        if(0==strcmp("name",reader.value()) && reader.read()) {
                            strcpy(weather_info.area,reader.value());
                        }
                    }   
                } else if(state==J_CUR) {
                    if(reader.node_type()==json_node_type::field) {
                        success = true;
                        if(0==strcmp("text",reader.value()) && reader.read()) {
                            strcpy(weather_info.condition,reader.value());
                        } else if(0==strcmp("last_updated_epoch",reader.value()) && reader.read()) {
                            last_update = (time_t)reader.value_int();
                        } else if(0==strcmp("is_day",reader.value()) && reader.read()) {
                            weather_info.is_day = reader.value_bool();
                        } else if(0==strcmp("temp_c",reader.value()) && reader.read()) {
                            temp_c = reader.value_real();
                        } else if(0==strcmp("feelslike_c",reader.value()) && reader.read()) {
                            feels_c = reader.value_real();
                        } else if(0==strcmp("wind_kph",reader.value()) && reader.read()) {
                            wind_kph = reader.value_real();
                        } else if(0==strcmp("wind_dir",reader.value()) && reader.read()) {
                            strcpy(wind_dir,reader.value());
                        } else if(0==strcmp("precip_mm",reader.value()) && reader.read()) {
                            precip_mm = reader.value_real();
                        } else if(0==strcmp("humidity",reader.value()) && reader.read()) {
                            humidity = reader.value_int();
                        } else if(0==strcmp("cloud",reader.value()) && reader.read()) {
                            cloud = reader.value_int();
                        } else if(0==strcmp("vis_km",reader.value()) && reader.read()) {
                            vis_km = reader.value_real();
                        } else if(0==strcmp("gust_kph",reader.value()) && reader.read()) {
                            gust_kph = reader.value_real();
                        }
                    }
                }
            }
        }
        http_end(handle);
        if(success) {
            weather_area_label.text(weather_info.area);
            weather_condition_label.text(weather_info.condition);
            sprintf(weather_info.temp,"%0.fC (feels %0.fC)",temp_c,feels_c);
            weather_temp_label.text(weather_info.temp);
            snprintf(weather_info.wind,sizeof(weather_info.wind),"%s %0.fKPH (gust %0.fKPH)",wind_dir,wind_kph,gust_kph);
            weather_wind_label.text(weather_info.wind);
            sprintf(weather_info.precipitation,"%0.fmm",precip_mm);
            weather_precipitation_label.text(weather_info.precipitation);
            sprintf(weather_info.humidity,"%d%%",humidity);
            weather_humidity_label.text(weather_info.humidity);
            if(precip_mm>250) {
                weather_icon.svg(weather_cloud_showers_heavy);
            } else if(weather_info.is_day) {
                if(precip_mm>.5f) {
                    if(temp_c<=0.f) {
                        weather_icon.svg(weather_snowflake);
                    } else {
                        weather_icon.svg(weather_cloud_rain);
                    }
                } else {
                    if(cloud>=25) {
                        weather_icon.svg(weather_cloud);
                    } else {
                        weather_icon.svg(weather_sun);
                    }
                }
            } else {
                if(precip_mm>.5f) {
                    if(temp_c<=0.f) {
                        weather_icon.svg(weather_snowflake);
                    } else {
                        weather_icon.svg(weather_cloud_moon_rain);
                    }
                } else {
                    if(cloud>=25) {
                        weather_icon.svg(weather_cloud_moon);
                    } else {
                        weather_icon.svg(weather_moon);
                    }
                }
            }
            return display_update_3bit();
        }
        return false;
    }
    http_end(handle);
    return false;
}
bool ui_set_setup(const char* address, const char* ssid, const char* pass) {
    if(xfer_buffer==NULL) return false;
    //WIFI:S:<SSID>;T:<WPA|WEP|>;P:<password>;
    strcpy(setup_qr_creds,"WIFI:S:");
    strcat(setup_qr_creds,ssid);
    strcat(setup_qr_creds,";T:WPA;P:");
    strcat(setup_qr_creds,pass);
    strcat(setup_qr_creds,";");
    setup_qr.text(setup_qr_creds);
    snprintf(setup_url,sizeof(setup_url)-1,"URL: %s",address);
    setup_cred_url_label.text(setup_url);
    snprintf(setup_ssid,sizeof(setup_ssid)-1,"SSID: %s",ssid);
    setup_cred_ssid_label.text(setup_ssid);
    snprintf(setup_pass,sizeof(setup_pass)-1,"Pass: %s",ssid);
    setup_cred_pass_label.text(setup_pass);
    setup_screen.invalidate();
    bool res= display_update_1bit();
    return res;
}