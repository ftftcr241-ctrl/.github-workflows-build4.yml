// TinyUSB config: USB Audio Class 2.0 loa stereo 16 bit, async feedback endpoint, Full Speed.
// Viet theo mau examples/device/uac2_speaker_fb cua TinyUSB (SDK 2.1+). Neu bien dich bao thieu/sai ten macro,
// doi chieu voi lib/tinyusb/examples/device/uac2_speaker_fb/src/tusb_config.h trong Pico SDK cua ban.
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif

#define CFG_TUD_ENABLED       1
#define CFG_TUD_MAX_SPEED     OPT_MODE_FULL_SPEED
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)   // tuong thich ban TinyUSB cu

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN    __attribute__((aligned(4)))
#endif

#define CFG_TUD_ENDPOINT0_SIZE 64

//--------------------------------------------------------------------
// Class: chi Audio
//--------------------------------------------------------------------
#define CFG_TUD_CDC           0
#define CFG_TUD_MSC           0
#define CFG_TUD_HID           0
#define CFG_TUD_MIDI          0
#define CFG_TUD_VENDOR        0
#define CFG_TUD_AUDIO         1

//--------------------------------------------------------------------
// Audio: loa 2 kenh, 16 bit, toi da 96 kHz
//--------------------------------------------------------------------
// Tong do dai descriptor chuc nang audio (khop voi PCM54_SPEAKER_STEREO_FB_DESCRIPTOR trong usb_descriptors.h)
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN                  (TUD_AUDIO_DESC_IAD_LEN + TUD_AUDIO_DESC_STD_AC_LEN + \
    TUD_AUDIO_DESC_CS_AC_LEN + TUD_AUDIO_DESC_CLK_SRC_LEN + TUD_AUDIO_DESC_INPUT_TERM_LEN + \
    TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN + TUD_AUDIO_DESC_OUTPUT_TERM_LEN + \
    TUD_AUDIO_DESC_STD_AS_INT_LEN + TUD_AUDIO_DESC_STD_AS_INT_LEN + TUD_AUDIO_DESC_CS_AS_INT_LEN + \
    TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN + \
    TUD_AUDIO_DESC_STD_AS_ISO_FB_EP_LEN)
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT                  1
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ               64

#define CFG_TUD_AUDIO_ENABLE_EP_IN                     0
#define CFG_TUD_AUDIO_ENABLE_EP_OUT                    1
#define CFG_TUD_AUDIO_ENABLE_ENCODING                  0
#define CFG_TUD_AUDIO_ENABLE_DECODING                  0
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP               1

#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE           96000
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX     2
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX             2
#define CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX             16
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX  2     // ten kieu moi (neu co)
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX          16

// 1 ms o 96 kHz = 96 mau (+1 du phong) * 4 byte = 388 byte, nho hon gioi han 1023 cua ISO Full Speed
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX             388
#define CFG_TUD_AUDIO_EP_SZ_OUT                        388         // ten kieu cu (neu co)
// Bo dem phan mem: ~8 goi 1 ms (du cho ca 44.1 kHz)
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ          (8 * CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX)
#define CFG_TUD_AUDIO_RX_FIFO_SIZE                     (8 * CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX)   // ten kieu cu

#ifdef __cplusplus
}
#endif

#endif
