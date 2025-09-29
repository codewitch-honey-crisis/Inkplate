#ifdef ESP_PLATFORM
#include "ui_captive_portal.hpp"
#include "display.h"

using namespace gfx;
using namespace uix;

using label_t = vlabel<surface_mono_t>;
using qr_t = qrcode<surface_mono_t>;

static char setup_qr_creds[256];
static char setup_url[128+5];
static char setup_ssid[65+6];
static char setup_pass[129+6];
static screen_mono_t setup_screen;
static label_t setup_label;
static label_t setup_cred_url_label;
static label_t setup_cred_ssid_label;
static label_t setup_cred_pass_label;
static qr_t setup_qr;

static const constexpr size_t xfer_buffer_size = (screen_dimensions.width*screen_dimensions.height+79)/80;
static uint8_t* xfer_buffer = NULL;

static void setup_flush(const rect16& bounds, const void* bmp, void* state) {
    const_bitmap<mono_pixel_t> src(bounds.dimensions(),bmp);
    bitmap<mono_pixel_t> dst(screen_dimensions,display_buffer_1bit());
    draw::bitmap(dst,bounds,src,src.bounds());
    setup_screen.flush_complete();
}

bool ui_captive_portal_init() {
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
    float fheight = screen_dimensions.width/12.f;
    
    setup_screen.buffer_size(xfer_buffer_size);
    setup_screen.buffer1(xfer_buffer);
    setup_screen.on_flush_callback(setup_flush);
    setup_screen.dimensions((ssize16)screen_dimensions);
    setup_screen.background_color(scolor_mono_t::white);
    setup_label.bounds(srect16(0,0,setup_screen.bounds().x2,fheight+1));
    setup_label.font(text_font);
    setup_label.color(ucolor_t::black);
    setup_screen.register_control(setup_label);
    setup_qr.bounds(srect16(spoint16::zero(),ssize16(screen_dimensions.width/4,screen_dimensions.width/4)).center_horizontal(setup_screen.bounds()).offset(0,fheight+2));
        
    setup_cred_url_label.bounds(srect16(0,0,setup_screen.bounds().x2,fheight+1).offset(0,setup_qr.bounds().y2+1));
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
    setup_screen.register_control(setup_cred_pass_label);
    if(uix_result::success!=setup_screen.register_control(setup_qr)) {
        // if any out of memory errors occurred above, this will fail as well. so we just check this.
        goto error;
    }
    return true;
error:
    if(xfer_buffer!=nullptr) {
        free(xfer_buffer);
        xfer_buffer = nullptr;
    }
    setup_screen.unregister_controls();
    return false;
}
void ui_captive_portal_end() {
    if(xfer_buffer==nullptr) {
        return;
    }
    setup_screen.unregister_controls();
    free(xfer_buffer);
    xfer_buffer = nullptr;
}
bool ui_captive_portal_set_ap(const char* address, const char* ssid, const char* pass) {
    if(xfer_buffer==NULL) return false;
    setup_label.text("Connect to AP");
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
    snprintf(setup_pass,sizeof(setup_pass)-1,"Pass: %s",pass);
    setup_cred_pass_label.text(setup_pass);
    setup_screen.invalidate();
    setup_screen.update();
    bool res= display_update_1bit();
    return res;
}
#endif