#include "tusb.h"
#include "usb_descriptors.h"
#include <string.h>
#include "pico/unique_id.h"

// VID 0xCafe la VID thu nghiem cua TinyUSB: chi dung cho cho ca nhan/thu nghiem, khong phat hanh san pham.
#define USB_VID 0xCafe
#define USB_PID 0x4011
#define USB_BCD 0x0200

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    // UAC2 dung IAD -> class MISC / IAD
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_AUDIO_FUNC_1_DESC_LEN)

uint8_t const desc_configuration[] = {
    // so config, so interface, string index, tong do dai, thuoc tinh, dong (mA)
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    // loa stereo + feedback EP: itf AC, string idx 4, 2 byte/mau, 16 bit, EP out, kich thuoc EP, EP feedback
    PCM54_SPEAKER_STEREO_FB_DESCRIPTOR(ITF_NUM_AUDIO_CONTROL, 4, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX,
                                       CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX, EPNUM_AUDIO_OUT,
                                       CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX, EPNUM_AUDIO_FB)
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

enum { STRID_LANGID = 0, STRID_MANUFACTURER, STRID_PRODUCT, STRID_SERIAL, STRID_SPEAKER };

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},   // 0: tieng Anh (0x0409)
    "PCM54 NOS",                  // 1: nha san xuat
    "PCM54 NOS USB DAC",          // 2: san pham
    NULL,                         // 3: serial (lay tu ID flash)
    "PCM54 NOS DAC",              // 4: ten giao dien audio
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t chr_count;
    char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

    if (index == STRID_LANGID) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        const char *str;
        if (index == STRID_SERIAL) {
            pico_get_unique_board_id_string(serial, sizeof serial);
            str = serial;
        } else {
            if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
            str = string_desc_arr[index];
            if (!str) return NULL;
        }
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (size_t i = 0; i < chr_count; i++) _desc_str[1 + i] = str[i];   // ASCII -> UTF-16
    }
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
