#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "tusb.h"

// ID thuc the (entity) UAC2: phai khop voi macro TUD_AUDIO_SPEAKER_STEREO_FB_DESCRIPTOR cua TinyUSB
#define UAC2_ENTITY_SPK_INPUT_TERMINAL   0x01
#define UAC2_ENTITY_SPK_FEATURE_UNIT     0x02
#define UAC2_ENTITY_SPK_OUTPUT_TERMINAL  0x03
#define UAC2_ENTITY_CLOCK                0x04

enum {
    ITF_NUM_AUDIO_CONTROL = 0,
    ITF_NUM_AUDIO_STREAMING,
    ITF_NUM_TOTAL
};

#define EPNUM_AUDIO_OUT   0x01
#define EPNUM_AUDIO_FB    0x81


// Macro descriptor loa STEREO + feedback EP (TinyUSB trong Pico SDK chi co ban MONO_FB).
// Viet theo nguyen van TUD_AUDIO_SPEAKER_MONO_FB_DESCRIPTOR (usbd.h), doi: 2 kenh, clock lap trinh duoc (host dat rate).
// Giao dien: AC = _itfnum, AS = _itfnum + 1. Entity: clock 0x04, input 0x01, feature unit 0x02, output 0x03.
// Feature unit khong khai bao dieu khien mute/volume (bit-perfect; volume do OS xu ly bang phan mem).
#define PCM54_SPEAKER_STEREO_FB_DESCRIPTOR(_itfnum, _stridx, _nBytesPerSample, _nBitsUsedPerSample, _epout, _epsize, _epfb) \
    TUD_AUDIO_DESC_IAD(_itfnum, 0x02, 0x00), \
    TUD_AUDIO_DESC_STD_AC(_itfnum, 0x00, _stridx), \
    TUD_AUDIO_DESC_CS_AC(0x0200, AUDIO_FUNC_DESKTOP_SPEAKER, \
        TUD_AUDIO_DESC_CLK_SRC_LEN + TUD_AUDIO_DESC_INPUT_TERM_LEN + TUD_AUDIO_DESC_OUTPUT_TERM_LEN + \
        TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN, AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS), \
    TUD_AUDIO_DESC_CLK_SRC(0x04, 3 /*noi bo, lap trinh duoc*/, 7 /*freq RW, valid R*/, 0x01, 0x00), \
    TUD_AUDIO_DESC_INPUT_TERM(0x01, AUDIO_TERM_TYPE_USB_STREAMING, 0x00, 0x04, 0x02, 0x00000003 /*L+R*/, 0x00, 0, 0x00), \
    TUD_AUDIO_DESC_OUTPUT_TERM(0x03, AUDIO_TERM_TYPE_OUT_DESKTOP_SPEAKER, 0x01, 0x02, 0x04, 0x0000, 0x00), \
    TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL(0x02, 0x01, 0, 0, 0, 0x00), \
    TUD_AUDIO_DESC_STD_AS_INT((uint8_t)((_itfnum) + 1), 0x00, 0x00, 0x00), \
    TUD_AUDIO_DESC_STD_AS_INT((uint8_t)((_itfnum) + 1), 0x01, 0x02, 0x00), \
    TUD_AUDIO_DESC_CS_AS_INT(0x01, AUDIO_CTRL_NONE, AUDIO_FORMAT_TYPE_I, AUDIO_DATA_FORMAT_TYPE_I_PCM, 0x02, 0x00000003, 0x00), \
    TUD_AUDIO_DESC_TYPE_I_FORMAT(_nBytesPerSample, _nBitsUsedPerSample), \
    TUD_AUDIO_DESC_STD_AS_ISO_EP(_epout, (uint8_t)((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), _epsize, 0x01), \
    TUD_AUDIO_DESC_CS_AS_ISO_EP(AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, AUDIO_CTRL_NONE, AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, 0x0000), \
    TUD_AUDIO_DESC_STD_AS_ISO_FB_EP(_epfb, 4 /*_epsize*/, 1 /*_interval, Full Speed*/)

// Danh sach sample rate ho tro
#define PCM54_N_RATES 4
extern const uint32_t pcm54_rates[PCM54_N_RATES];

#endif
