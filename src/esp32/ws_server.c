#include "ws_server.h"
// ESP32 has built in websocket support
void ws_srv_frame_to_esp32(const ws_srv_frame_t* frame,httpd_ws_frame_t* out_esp32_frame) {
    out_esp32_frame->final = frame->final;
    out_esp32_frame->fragmented = frame->fragmented;
    out_esp32_frame->len = frame->len;
    out_esp32_frame->payload = frame->payload;
    out_esp32_frame->type = (httpd_ws_type_t)frame->type;
}
void ws_srv_esp32_to_frame(const httpd_ws_frame_t* esp32_frame, ws_srv_frame_t* out_frame) {
    out_frame->final = esp32_frame->final;
    out_frame->fragmented = esp32_frame->fragmented;
    out_frame->len = esp32_frame->len;
    out_frame->payload = esp32_frame->payload;
    out_frame->type = out_frame->type;
    out_frame->masked = 0;
}

void ws_srv_unmask_payload(const ws_srv_frame_t* frame, uint8_t* payload) {
    if(frame->masked) {
        for(size_t i = 0;i<frame->len;++i) {
            payload[i]=frame->payload[i]^frame->mask_key[i%4];
        }
    }
}
