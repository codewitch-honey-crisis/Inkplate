#if defined(INKPLATE10) || defined(INKPLATE10V2)
#include "display.h"

#include <stddef.h>
#include <stdint.h>

#include "../esp32/i2c.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_heap_caps.h"
#include "esp_check.h"
#include "esp_log.h"
#include "hal/gpio_hal.h"
#include "hal/gpio_ll.h"
#include "hardware.h"
#include "nvs_flash.h"
#include "pcal_ex.h"
#include "rom/gpio.h"
#include "soc/gpio_periph.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "timing.h"
#define INKPLATE10_WAVEFORM1 20
#define INKPLATE10_WAVEFORM2 21
#define INKPLATE10_WAVEFORM3 22
#define INKPLATE10_WAVEFORM4 23
#define INKPLATE10_WAVEFORM5 24
#define INKPLATE_1BIT 0
#define INKPLATE_3BIT 1
#define PWR_GOOD_OK 0b11111010
#define INKPLATE_FORCE_PARTIAL true
#define GPIO0_ENABLE 8
#define WAKEUP 3
#define WAKEUP_SET                           \
    {                                        \
        pcal_ex_set_level_int(WAKEUP, HIGH); \
    }
#define WAKEUP_CLEAR                        \
    {                                       \
        pcal_ex_set_level_int(WAKEUP, LOW); \
    }
#define PWRUP 4
#define PWRUP_SET                           \
    {                                       \
        pcal_ex_set_level_int(PWRUP, HIGH); \
    }
#define PWRUP_CLEAR                        \
    {                                      \
        pcal_ex_set_level_int(PWRUP, LOW); \
    }
#define VCOM 5
#define VCOM_SET                           \
    {                                      \
        pcal_ex_set_level_int(VCOM, HIGH); \
    }
#define VCOM_CLEAR                        \
    {                                     \
        pcal_ex_set_level_int(VCOM, LOW); \
    }

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
    {                       \
        int16_t t = a;      \
        a = b;              \
        b = t;              \
    }
#endif

#define CL 0x01
#define CL_SET                                                                                                         \
    {                                                                                                                  \
        GPIO.out_w1ts = CL;                                                                                            \
    }
#define CL_CLEAR                                                                                                       \
    {                                                                                                                  \
        GPIO.out_w1tc = CL;                                                                                            \
    }
#define CKV 0x01
#define CKV_SET                                                                                                        \
    {                                                                                                                  \
        GPIO.out1_w1ts.val = CKV;                                                                                      \
    }
#define CKV_CLEAR                                                                                                      \
    {                                                                                                                  \
        GPIO.out1_w1tc.val = CKV;                                                                                      \
    }
#define SPH 0x02
#define SPH_SET                                                                                                        \
    {                                                                                                                  \
        GPIO.out1_w1ts.val = SPH;                                                                                      \
    }
#define SPH_CLEAR                                                                                                      \
    {                                                                                                                  \
        GPIO.out1_w1tc.val = SPH;                                                                                      \
    }
#define LE 0x04
#define LE_SET                                                                                                         \
    {                                                                                                                  \
        GPIO.out_w1ts = LE;                                                                                            \
    }
#define LE_CLEAR                                                                                                       \
    {                                                                                                                  \
        GPIO.out_w1tc = LE;                                                                                            \
    }
#define OE 0
#define OE_SET                                                                                                         \
    {                                                                                                                  \
        pcal_ex_set_level_int(OE, HIGH);                                                        \
    }
#define OE_CLEAR                                                                                                       \
    {                                                                                                                  \
        pcal_ex_set_level_int(OE, LOW);                                                         \
    }
#define GMOD 1
#define GMOD_SET                                                                                                       \
    {                                                                                                                  \
        pcal_ex_set_level_int(GMOD, HIGH);                                                      \
    }
#define GMOD_CLEAR                                                                                                     \
    {                                                                                                                  \
        pcal_ex_set_level_int(GMOD, LOW);                                                       \
    }
#define SPV 2
#define SPV_SET                                                                                                        \
    {                                                                                                                  \
        pcal_ex_set_level_int(SPV, HIGH);                                                       \
    }
#define SPV_CLEAR                                                                                                      \
    {                                                                                                                  \
        pcal_ex_set_level_int(SPV, LOW);                                                        \
    }

#define GPIO0_ENABLE 8

#define DATA 0x0E8C0030

#define BOUND(a, b, c) ((a) <= (b) && (b) <= (c))

#define RGB3BIT(r, g, b) ((54UL * (r) + 183UL * (g) + 19UL * (b)) >> 13)
#define RGB8BIT(r, g, b) ((54UL * (r) + 183UL * (g) + 19UL * (b)) >> 8)

#define READ32(c) (uint32_t)(*(c) | (*((c) + 1) << 8) | (*((c) + 2) << 16) | (*((c) + 3) << 24))
#define READ16(c) (uint16_t)(*(c) | (*((c) + 1) << 8))
#define ROWSIZE(w, c) (((int16_t)c * w + 31) >> 5) << 2

// Pin on the internal io expander which controls MOSFET for turning on and off the SD card
#define SD_PMOS_PIN 10

#define E_INK_WIDTH 1200
#define E_INK_HEIGHT 825

struct waveformData {
    uint8_t header;  // = 'W';
    uint8_t waveformId;
    uint8_t waveform[8][9];
    uint8_t temp;  // = 20;
    uint8_t checksum;
};

/* Adapted from the following by honey the codewitch

----- InkPlate Low Level drivers License - GNU V3.0 License --------------------

This code is released under the GNU Lesser General Public License
v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html
Please review the LICENSE file included with this example.
If you have any questions about licensing, please contact

  Guy Turcotte<turgu666@gmail.com>
  techsupport@e-radionica.com

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/
static const char *TAG = "display";
typedef enum { WHITE = 0b10101010,
               BLACK = 0b01010101,
               DISCHARGE = 0b00000000,
               SKIP = 0b11111111 } pixel_state_t;
static bool display_initialized = false;

static uint8_t *DMemoryNew = NULL;
static uint8_t *_partial = NULL;
static uint8_t *DMemory4Bit = NULL;
static uint8_t *_pBuffer = NULL;
static uint8_t _blockPartial = 1;
static uint32_t pinLUT[256];
static uint32_t *GLUT;
static uint32_t *GLUT2;

static uint16_t _partialUpdateLimiter = 0;
static uint16_t _partialUpdateCounter = 0;
static bool panel_enabled = false;
static uint8_t waveform3Bit[8][9];
static struct waveformData waveformStored;

// static const uint8_t LUT2[16] = {0xAA, 0xA9, 0xA6, 0xA5, 0x9A, 0x99, 0x96, 0x95,
//                               0x6A, 0x69, 0x66, 0x65, 0x5A, 0x59, 0x56, 0x55};
static const uint8_t LUTW[16] = {0xFF, 0xFE, 0xFB, 0xFA, 0xEF, 0xEE, 0xEB, 0xEA,
                              0xBF, 0xBE, 0xBB, 0xBA, 0xAF, 0xAE, 0xAB, 0xAA};
static const uint8_t LUTB[16] = {0xFF, 0xFD, 0xF7, 0xF5, 0xDF, 0xDD, 0xD7, 0xD5,
                              0x7F, 0x7D, 0x77, 0x75, 0x5F, 0x5D, 0x57, 0x55};

//static const uint8_t pixelMaskLUT[8] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
//static const uint8_t pixelMaskGLUT[2] = {0xF, 0xF0};

#define CMD_HANDLER_BUFFER_SIZE I2C_LINK_RECOMMENDED_SIZE(7)

static uint8_t cmdlink_buffer[CMD_HANDLER_BUFFER_SIZE];

static bool i2c_write(uint8_t address, const void *to_write, size_t write_len) {
    i2c_cmd_handle_t cmd_link = NULL;
    esp_err_t ret = ESP_OK;
    ret = ret;
    bool result = false;
    cmd_link = i2c_cmd_link_create_static(cmdlink_buffer, CMD_HANDLER_BUFFER_SIZE);

    ESP_GOTO_ON_ERROR(i2c_master_start(cmd_link), err, TAG, "i2c_master_start");
    ESP_GOTO_ON_ERROR(i2c_master_write_byte(cmd_link, (address << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK), err, TAG, "i2c_master_write_byte");
    if (write_len > 0) {
        ESP_GOTO_ON_ERROR(i2c_master_write(cmd_link, to_write, write_len, I2C_MASTER_ACK), err, TAG, "i2c_master_write");
    }
    ESP_GOTO_ON_ERROR(i2c_master_stop(cmd_link), err, TAG, "i2c_master_stop");
    ESP_GOTO_ON_ERROR(i2c_master_cmd_begin(I2C_PORT, cmd_link, pdMS_TO_TICKS(10000)), err, TAG, "i2c_master_cmd_begin");

    result = true;
err:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(ret));
    }
    if (cmd_link != NULL) {
        // i2c_cmd_link_delete(cmd_link);
        i2c_cmd_link_delete_static(cmd_link);
    }
    return result;
}
static bool i2c_read(uint8_t address, void *to_read, size_t read_len) {
    i2c_cmd_handle_t cmd_link = NULL;
    esp_err_t ret = ESP_OK;
    ret = ret;
    bool result = false;
    if (read_len == 0) return true;
    cmd_link = i2c_cmd_link_create_static(cmdlink_buffer, CMD_HANDLER_BUFFER_SIZE);

    // i2c_cmd_handle_t cmd_link = i2c_cmd_link_create();
    ESP_GOTO_ON_ERROR(i2c_master_start(cmd_link), err, TAG, "i2c_master_start");
    ESP_GOTO_ON_ERROR(i2c_master_write_byte(cmd_link, (address << 1) | I2C_MASTER_READ, 0), err, TAG, "i2c_master_write_byte");

    if (read_len > 1) {
        ESP_GOTO_ON_ERROR(i2c_master_read(cmd_link, to_read, read_len - 1, I2C_MASTER_NACK), err, TAG, "i2c_master_read");
    }
    ESP_GOTO_ON_ERROR(i2c_master_read_byte(cmd_link, to_read + read_len - 1, I2C_MASTER_LAST_NACK), err, TAG, "i2c_master_read_byte");

    ESP_GOTO_ON_ERROR(i2c_master_stop(cmd_link), err, TAG, "i2c_master_stop");

    ESP_GOTO_ON_ERROR(i2c_master_cmd_begin(I2C_PORT, cmd_link, pdMS_TO_TICKS(10000)), err, TAG, "i2c_master_cmd_begin");
    result = true;
err:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(ret));
    }
    if (cmd_link != NULL) {
        // i2c_cmd_link_delete(cmd_link);
        i2c_cmd_link_delete_static(cmd_link);
    }
    return result;
}
static bool i2c_write_read(uint8_t address, const void *to_write, size_t write_len, void *to_read, size_t read_len) {
    if (i2c_write(address, to_write, write_len)) {
        return i2c_read(address, to_read, read_len);
    }
    return false;
}

static uint8_t calculate_checksum(struct waveformData *_w) {
    uint8_t *_d = (uint8_t *)_w;
    uint16_t _sum = 0;
    int _n = sizeof(struct waveformData) - 1;

    for (int i = 0; i < _n; i++) {
        _sum += _d[i];
    }
    return _sum % 256;
}
static bool read_waveform_data(struct waveformData *out_w) {
    nvs_handle_t nvs_handle;
    if (ESP_OK != nvs_open(TAG, NVS_READWRITE, &nvs_handle)) {
        ESP_LOGE(TAG, "Could not open NVS flash");
        return false;
    }
    size_t len = sizeof(struct waveformData);
    if (ESP_OK == nvs_get_blob(nvs_handle, "waveform", out_w, &len)) {
        if (calculate_checksum(out_w) == out_w->checksum) {
            nvs_close(nvs_handle);
            return true;
        } else {
            ESP_LOGI(TAG, "Checksum mismatch in waveform data");
        }
    }
    nvs_close(nvs_handle);
    return false;
}
static void calculate_luts()
{
    for (int j = 0; j < 9; ++j)
    {
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint8_t z = (waveform3Bit[i & 0x07][j] << 2) | (waveform3Bit[(i >> 4) & 0x07][j]);
            GLUT[j * 256 + i] = ((z & 0b00000011) << 4) | (((z & 0b00001100) >> 2) << 18) |
                                (((z & 0b00010000) >> 4) << 23) | (((z & 0b11100000) >> 5) << 25);
            z = ((waveform3Bit[i & 0x07][j] << 2) | (waveform3Bit[(i >> 4) & 0x07][j])) << 4;
            GLUT2[j * 256 + i] = ((z & 0b00000011) << 4) | (((z & 0b00001100) >> 2) << 18) |
                                 (((z & 0b00010000) >> 4) << 23) | (((z & 0b11100000) >> 5) << 25);
        }
    }
}
static void set_outputs()
{
    gpio_set_direction((gpio_num_t)2, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)32, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)33, GPIO_MODE_OUTPUT);
    pcal_ex_set_direction_int(OE, OUTPUT);
    pcal_ex_set_direction_int(GMOD, OUTPUT);
    pcal_ex_set_direction_int(SPV, OUTPUT);

    gpio_set_direction((gpio_num_t)0, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)4, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)5, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)18, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)19, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)23, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)25, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)26, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)27, GPIO_MODE_OUTPUT);
}
static int read_power()
{
    uint8_t out = 0x0f;
    uint8_t in;
    if(!i2c_write_read(0x48,&out,1,&in,1)) {
        return -1;
    }
    return in;
}
static void pins_z_state()
{
    gpio_set_direction((gpio_num_t)2, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)32, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)33, GPIO_MODE_INPUT);
    pcal_ex_set_direction_int(OE, INPUT);
    pcal_ex_set_direction_int(GMOD, INPUT);
    pcal_ex_set_direction_int(SPV, INPUT);

    gpio_set_direction((gpio_num_t)0, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)4, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)5, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)18, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)19, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)23, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)25, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)26, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)27, GPIO_MODE_INPUT);

}
static bool panel_off()
{
    if (!panel_enabled)
        return true;
    VCOM_CLEAR;
    OE_CLEAR;
    GMOD_CLEAR;
    GPIO.out &= ~(DATA | LE | CL);
    CKV_CLEAR;
    SPH_CLEAR;
    SPV_CLEAR;
    PWRUP_CLEAR;

    unsigned long timer = timing_get_ms();
    do
    {
        timing_delay_ms(1);
    } while ((read_power() != 0) && (timing_get_ms() - timer) < 250);
    bool result = true;
    // Do not disable WAKEUP if older Inkplate6Plus is used.
    WAKEUP_CLEAR; // Disable 3V3 Switch for ePaper.
    uint8_t tmp[] = {0x01,0x00};
    if(!i2c_write(0x48,tmp,sizeof(tmp))) {
        result = false;
    }
    
    pins_z_state();

    panel_enabled = false;
    return result;
}

static bool panel_on() {
    if(panel_enabled)
        return true;
    WAKEUP_SET;
    timing_delay_ms(5);
    uint8_t tmp[] = {1,0b00100000};
    if(!i2c_write(0x48,tmp,sizeof(tmp))) {
        WAKEUP_CLEAR;
        return false;
    }
    tmp[0]=0x09;
    tmp[1]=0b11100100;
    if(!i2c_write(0x48,tmp,sizeof(tmp))) {
        WAKEUP_CLEAR;
        return false;
    }
    tmp[0]=0x0b;
    tmp[1]=0b00011011;
    if(!i2c_write(0x48,tmp,sizeof(tmp))) {
        WAKEUP_CLEAR;
        return false;
    }
    
    set_outputs();
    LE_CLEAR;
    CL_CLEAR;
    SPH_SET;
    GMOD_SET;
    SPV_SET;
    CKV_CLEAR;
    OE_CLEAR;
    PWRUP_SET;
    panel_enabled = true;
    unsigned long timer = timing_get_ms();
    do
    {
        timing_delay_ms(1);
    } while ((read_power() != PWR_GOOD_OK) && (timing_get_ms() - timer) < 250);
    if ((timing_get_ms() - timer) >= 250)
    {
        panel_off();
        return 0;
    }

    VCOM_SET;
    OE_SET;
    
    return 1;
}

/**
 * @brief       vscan_start starts writing new frame and skips first two lines
 * that are invisible on screen
 */
void vscan_start()
{
    CKV_SET;
    timing_delay_us(7);
    SPV_CLEAR;
    timing_delay_us(10);
    CKV_CLEAR;
    timing_delay_us(0);
    CKV_SET;
    timing_delay_us(8);
    SPV_SET;
    timing_delay_us(10);
    CKV_CLEAR;
    timing_delay_us(0);
    CKV_SET;
    timing_delay_us(18);
    CKV_CLEAR;
    timing_delay_us(0);
    CKV_SET;
    timing_delay_us(18);
    CKV_CLEAR;
    timing_delay_us(0);
    CKV_SET;
}

void hscan_start(uint32_t _d)
{
    SPH_CLEAR;
    GPIO.out_w1ts = (_d) | CL;
    GPIO.out_w1tc = DATA | CL;
    SPH_SET;
    CKV_SET;
}

/**
 * @brief       vscan_end ends current row and prints data to screen
 */
void vscan_end()
{
    CKV_CLEAR;
    LE_SET;
    LE_CLEAR;
    timing_delay_us(0);
}

bool panel_clean(uint8_t c, uint8_t rep)
{
    if(!panel_on()) {
        return false;
    }
    uint8_t data = 0;
    if (c == 0)
        data = 0b10101010;
    else if (c == 1)
        data = 0b01010101;
    else if (c == 2)
        data = 0b00000000;
    else if (c == 3)
        data = 0b11111111;

    uint32_t _send = pinLUT[data];
    for (int k = 0; k < rep; ++k)
    {
        vscan_start();
        for (int i = 0; i < E_INK_HEIGHT; ++i)
        {
            hscan_start(_send);
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = CL;
            for (int j = 0; j < ((E_INK_WIDTH / 8) - 1); ++j)
            {
                GPIO.out_w1ts = CL;
                GPIO.out_w1tc = CL;
                GPIO.out_w1ts = CL;
                GPIO.out_w1tc = CL;
            }
            GPIO.out_w1ts = CL;
            GPIO.out_w1tc = CL;
            vscan_end();
        }
        timing_delay_ms(230);
    }
    return true;
}
static uint8_t reverse_bits(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}
bool display_init(void) {
    if (display_initialized) {
        return true;
    }
    if (!pcal_ex_init()) {
        return false;
    }
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Could not erase NVS flash");
            return false;
        }
        ret = nvs_flash_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Could not init NVS flash");
            return false;
        }
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Could not init NVS flash");
        return false;
    }
    waveformStored.header = 'W';
    waveformStored.temp = 0;
    waveformStored.waveformId = 0;//INKPLATE10_WAVEFORM1;
    if (!read_waveform_data(&waveformStored)) {
        uint8_t defaultWaveform[8][9] = {{0, 0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 2, 2, 2, 1, 1, 0}, {0, 0, 2, 1, 1, 2, 2, 1, 0}, {0, 1, 2, 2, 1, 2, 2, 1, 0}, {0, 0, 2, 1, 2, 2, 2, 1, 0}, {0, 2, 2, 2, 2, 2, 2, 1, 0}, {0, 0, 0, 0, 0, 2, 1, 2, 0}, {0, 0, 0, 2, 2, 2, 2, 2, 0}};
        memcpy(waveform3Bit, defaultWaveform, sizeof(waveform3Bit));
    } else {
        memcpy(waveform3Bit, waveformStored.waveform, sizeof(waveform3Bit));
    }

    for (uint32_t i = 0; i < 256; ++i)
        pinLUT[i] = ((i & 0b00000011) << 4) | (((i & 0b00001100) >> 2) << 18) | (((i & 0b00010000) >> 4) << 23) |
                    (((i & 0b11100000) >> 5) << 25);
    if (!pcal_ex_set_level_int(9, LOW)) {
        return false;
    }
    pcal_ex_set_direction_int(VCOM, OUTPUT);
    pcal_ex_set_direction_int(PWRUP, OUTPUT);
    pcal_ex_set_direction_int(WAKEUP, OUTPUT);
    pcal_ex_set_direction_int(GPIO0_ENABLE, OUTPUT);
    pcal_ex_set_level_int(GPIO0_ENABLE, HIGH);
    WAKEUP_SET;
    timing_delay_ms(1);
    uint8_t tmp[] = {
        0x09,
        0b00011011,
        0b00000000,
        0b00011011,
        0b00000000};
    i2c_write(0x48, tmp, sizeof(tmp));
    timing_delay_ms(1);
    WAKEUP_CLEAR;
    for (int i = 0; i < 15; i++) {
        pcal_ex_set_direction_ext(i, OUTPUT);
        pcal_ex_set_level_ext( i, LOW);
    }
    pcal_ex_set_direction_int( 14, OUTPUT);
    pcal_ex_set_direction_int( 15, OUTPUT);
    pcal_ex_set_level_int(14, LOW);
    pcal_ex_set_level_int( 15, LOW);
#ifdef INKPLATE10V2
    // Set SPI pins to input to reduce power consumption in deep sleep
    gpio_set_direction((gpio_num_t)12,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)13,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)14,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)15,GPIO_MODE_INPUT);
    
    // And also disable uSD card supply
    pcal_ex_set_direction_int(SD_PMOS_PIN, INPUT);
#else
    pcal_ex_set_direction_int(12, OUTPUT);
    pcal_ex_set_level_int(12, LOW);
#endif
    // CONTROL PINS
    gpio_set_direction((gpio_num_t)0,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)2,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)32,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)33,GPIO_MODE_INPUT);
    pcal_ex_set_direction_int(OE, OUTPUT);
    pcal_ex_set_direction_int(GMOD, OUTPUT);
    pcal_ex_set_direction_int(SPV, OUTPUT);

    // DATA PINS
    gpio_set_direction((gpio_num_t)4,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)5,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)18,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)19,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)23,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)25,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)26,GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)27,GPIO_MODE_INPUT);

#ifdef INKPLATE10
    // TOUCHPAD PINS
    pcal_ex_set_direction_int(10, INPUT);
    pcal_ex_set_direction_int(11, INPUT);
    pcal_ex_set_direction_int(12, INPUT);
#else
    pcal_ex_set_direction_int(10, OUTPUT);
    pcal_ex_set_direction_int(11, OUTPUT);
    pcal_ex_set_direction_int(12, OUTPUT);
    pcal_ex_set_level_int(10, LOW);
    pcal_ex_set_level_int(11, LOW);
    pcal_ex_set_level_int(12, LOW);
#endif
    pcal_ex_set_direction_int(9, OUTPUT);
    if(!pcal_ex_set_level_int(9, LOW)) {
        return false;
    }

    DMemoryNew = (uint8_t *)heap_caps_malloc(E_INK_WIDTH * E_INK_HEIGHT / 8,MALLOC_CAP_SPIRAM);
    if(DMemoryNew==NULL) {
        ESP_LOGE(TAG,"Out of memory allocating 1bit frame buffer");
    }
    _partial = (uint8_t *)heap_caps_malloc(E_INK_WIDTH * E_INK_HEIGHT / 8,MALLOC_CAP_SPIRAM);
    if(_partial==NULL) {
        ESP_LOGE(TAG,"Out of memory allocating partial update frame buffer");
        free(DMemoryNew);
        DMemoryNew = NULL;
        return false;
    }
    _pBuffer = (uint8_t *)heap_caps_malloc(E_INK_WIDTH * E_INK_HEIGHT / 4,MALLOC_CAP_SPIRAM);
    if(_pBuffer==NULL) {
        ESP_LOGE(TAG,"Out of memory allocating 4bit frame buffer");
        free(DMemoryNew);
        DMemoryNew = NULL;
        free(_partial);
        _partial = NULL;
        return false;
    }
    DMemory4Bit = (uint8_t *)heap_caps_malloc(E_INK_WIDTH * E_INK_HEIGHT / 2,MALLOC_CAP_SPIRAM);
    if(DMemory4Bit==NULL) {
        ESP_LOGE(TAG,"Out of memory allocating 4bit frame buffer 2");
        free(DMemoryNew);
        DMemoryNew = NULL;
        free(_partial);
        _partial = NULL;
        free(_pBuffer);
        _pBuffer=NULL;
        return false;
    }
    GLUT = (uint32_t *)malloc(256 * 9 * sizeof(uint32_t));
    if(GLUT==NULL) {
        ESP_LOGE(TAG,"Out of memory allocating GLUT");
        free(DMemoryNew);
        DMemoryNew = NULL;
        free(_partial);
        _partial = NULL;
        free(_pBuffer);
        _pBuffer=NULL;
        free(DMemory4Bit);
        DMemory4Bit = NULL;
        return false;
    }
    GLUT2 = (uint32_t *)malloc(256 * 9 * sizeof(uint32_t));
    if(GLUT2==NULL) {
        ESP_LOGE(TAG,"Out of memory allocating GLUT");
        free(DMemoryNew);
        DMemoryNew = NULL;
        free(_partial);
        _partial = NULL;
        free(_pBuffer);
        _pBuffer=NULL;
        free(DMemory4Bit);
        DMemory4Bit = NULL;
        free(GLUT);
        GLUT = NULL;
        return false;
    }
   
    memset(DMemoryNew, 0, E_INK_WIDTH * E_INK_HEIGHT / 8);
    memset(_partial, 0, E_INK_WIDTH * E_INK_HEIGHT / 8);
    memset(_pBuffer, 0, E_INK_WIDTH * E_INK_HEIGHT / 4);
    memset(DMemory4Bit, 255, E_INK_WIDTH * E_INK_HEIGHT / 2);

    calculate_luts();
    display_initialized = true;
    return true;
}
static bool panel_update_1bit(void)
{
    memcpy(DMemoryNew, _partial, E_INK_WIDTH * E_INK_HEIGHT / 8);

    uint32_t _pos;
    uint8_t data;
    uint8_t dram;
    uint8_t _repeat;

    if (!panel_on()) {
        return false;
    }
    if (waveformStored.waveformId != INKPLATE10_WAVEFORM1)
    {
        panel_clean(0, 1);
        panel_clean(1, 12);
        panel_clean(2, 1);
        panel_clean(0, 9);
        panel_clean(2, 1);
        panel_clean(1, 12);
        panel_clean(2, 1);
        panel_clean(0, 9);
        _repeat = 3;
    }
    else
    {
        panel_clean(0, 1);
        panel_clean(1, 10);
        panel_clean(2, 1);
        panel_clean(0, 10);
        panel_clean(2, 1);
        panel_clean(1, 10);
        panel_clean(2, 1);
        panel_clean(0, 10);
        _repeat = 5;
    }

    for (int k = 0; k < _repeat; k++)
    {
        _pos = (E_INK_HEIGHT * E_INK_WIDTH / 8) - 1;
        vscan_start();
        for (int i = 0; i < E_INK_HEIGHT; i++)
        {
            dram = reverse_bits(~(*(DMemoryNew + _pos)));
            data = LUTB[(dram >> 4) & 0x0F];
            hscan_start(pinLUT[data]);
            data = LUTB[dram & 0x0F];
            GPIO.out_w1ts = pinLUT[data] | CL;
            GPIO.out_w1tc = DATA | CL;
            _pos--;
            for (int j = 0; j < ((E_INK_WIDTH / 8) - 1); j++)
            {
                dram = reverse_bits(~(*(DMemoryNew + _pos)));
                data = LUTB[(dram >> 4) & 0x0F];
                GPIO.out_w1ts = pinLUT[data] | CL;
                GPIO.out_w1tc = DATA | CL;
                data = LUTB[dram & 0x0F];
                GPIO.out_w1ts = pinLUT[data] | CL;
                GPIO.out_w1tc = DATA | CL;
                _pos--;
            }
            GPIO.out_w1ts = CL;
            GPIO.out_w1tc = DATA | CL;
            vscan_end();
        }
        timing_delay_us(230);
    }

    if(!panel_clean(2, 2)) {
        return false;
    }
    if(!panel_clean(3, 1)) {
        return false;
    }

    vscan_start();
    _blockPartial = false;
    return true;
}
int32_t panel_partial_update_1bit(bool _forced)
{
    if (_blockPartial == 1 && !_forced)
    {
        if(!panel_update_1bit()) {
            return -1;
        }
        return 0;
    }

    if (_partialUpdateCounter >= _partialUpdateLimiter && _partialUpdateLimiter != 0)
    {
        // Force full update.
        if(!panel_update_1bit()) {
            return -1;
        }

        // Reset the counter!
        _partialUpdateCounter = 0;

        // Go back!
        return 0;
    }
    uint32_t _pos = (E_INK_WIDTH * E_INK_HEIGHT / 8) - 1;
    uint32_t _send;
    uint8_t data = 0;
    uint8_t diffw, diffb;
    uint32_t n = (E_INK_WIDTH * E_INK_HEIGHT / 4) - 1;
    uint8_t _repeat;

    int32_t changeCount = 0;

    for (int i = 0; i < E_INK_HEIGHT; ++i)
    {
        for (int j = 0; j < E_INK_WIDTH / 8; ++j)
        {
            uint8_t dram = ~reverse_bits(*(DMemoryNew + _pos));
            uint8_t cmpram = ~reverse_bits(*(_partial+_pos));
            diffw = dram & ~cmpram;
            diffb = ~dram & cmpram;
            if (diffw) // count pixels turning from black to white as these are visible blur
            {
                for (int bv = 1; bv < 256; bv <<= 1)
                {
                    if (diffw & bv)
                        ++changeCount;
                }
            }
            _pos--;
            *(_pBuffer + n) = LUTW[diffw >> 4] & (LUTB[diffb >> 4]);
            //*(_pBuffer + n) = LUTW[diffw & 0x0F] & (LUTB[diffb & 0x0F]);
            n--;
            //*(_pBuffer + n) = LUTW[diffw >> 4] & (LUTB[diffb >> 4]);
            *(_pBuffer + n) = LUTW[diffw & 0x0F] & (LUTB[diffb & 0x0F]);
            n--;
        }
    }

    if (!panel_on())
        return -1;

    if (waveformStored.waveformId != INKPLATE10_WAVEFORM1)
    {
        _repeat = 4;
    }
    else
    {
        _repeat = 5;
    }

    for (int k = 0; k < _repeat; ++k)
    {
        vscan_start();
        n = (E_INK_WIDTH * E_INK_HEIGHT / 4) - 1;
        for (int i = 0; i < E_INK_HEIGHT; ++i)
        {
            data = *(_pBuffer + n);
            _send = pinLUT[data];
            hscan_start(_send);
            n--;
            for (int j = 0; j < ((E_INK_WIDTH / 4) - 1); ++j)
            {
                data = *(_pBuffer + n);
                _send = pinLUT[data];
                GPIO.out_w1ts = _send | CL;
                GPIO.out_w1tc = DATA | CL;
                n--;
            }
            GPIO.out_w1ts = _send | CL;
            GPIO.out_w1tc = DATA | CL;
            vscan_end();
        }
        timing_delay_us(230);
    }
    panel_clean(2, 2);
    panel_clean(3, 1);
    vscan_start();

    memcpy(DMemoryNew, _partial, E_INK_WIDTH * E_INK_HEIGHT / 8);

    if (_partialUpdateLimiter != 0)
        _partialUpdateCounter++;
    return changeCount;
}
static bool panel_update_3bit(void)
{
    if (!panel_on())
        return false;

    if (waveformStored.waveformId != INKPLATE10_WAVEFORM1)
    {
        panel_clean(1, 1);
        panel_clean(0, 7);
        panel_clean(2, 1);
        panel_clean(1, 12);
        panel_clean(2, 1);
        panel_clean(0, 7);
        panel_clean(2, 1);
        panel_clean(1, 12);
    }
    else
    {
        panel_clean(1, 1);
        panel_clean(0, 10);
        panel_clean(2, 1);
        panel_clean(1, 10);
        panel_clean(2, 1);
        panel_clean(0, 10);
        panel_clean(2, 1);
        panel_clean(1, 10);
    }

    for (int k = 0; k < 9; k++)
    {
        uint8_t *dp = DMemory4Bit + (E_INK_HEIGHT * E_INK_WIDTH / 2);

        vscan_start();
        for (int i = 0; i < E_INK_HEIGHT; i++)
        {
            uint32_t t = GLUT2[k * 256 + (*(--dp))];
            t |= GLUT[k * 256 + (*(--dp))];
            hscan_start(t);
            t = GLUT2[k * 256 + (*(--dp))];
            t |= GLUT[k * 256 + (*(--dp))];
            GPIO.out_w1ts = t | CL;
            GPIO.out_w1tc = DATA | CL;

            for (int j = 0; j < ((E_INK_WIDTH / 8) - 1); j++)
            {
                t = GLUT2[k * 256 + (*(--dp))];
                t |= GLUT[k * 256 + (*(--dp))];
                GPIO.out_w1ts = t | CL;
                GPIO.out_w1tc = DATA | CL;
                t = GLUT2[k * 256 + (*(--dp))];
                t |= GLUT[k * 256 + (*(--dp))];
                GPIO.out_w1ts = t | CL;
                GPIO.out_w1tc = DATA | CL;
            }

            GPIO.out_w1ts = CL;
            GPIO.out_w1tc = DATA | CL;
            vscan_end();
        }
        timing_delay_us(230);
    }
    if(!panel_clean(3, 1)) {
        return false;
    }
    vscan_start();
    _blockPartial = true;
    return true;
}
bool display_update_3bit(void) {
    return panel_update_3bit();
}
size_t display_buffer_3bit_size() {
    return E_INK_WIDTH*E_INK_HEIGHT/2;
}
uint8_t* display_buffer_3bit() {
    return DMemory4Bit;
}
bool display_update_1bit(void) {
    return panel_update_1bit();
}
bool display_partial_update_1bit(void) {
    return panel_partial_update_1bit(false);
}
size_t display_buffer_1bit_size() {
    return E_INK_WIDTH*E_INK_HEIGHT/8;
}
uint8_t* display_buffer_1bit() {
    return _partial;
}
#endif  // INKPLATE10