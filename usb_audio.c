// Callback USB Audio Class 2.0 cho DAC NOS: chon sample rate, mute/volume (chi luu, khong ap dung), feedback.
// Viet theo mau TinyUSB examples/device/uac2_speaker_fb. Doi chieu neu ban TinyUSB cua ban khac chu ky ham.
#include "tusb.h"
#include "usb_descriptors.h"

// ham do main.c cung cap
extern void pcm54_request_rate(uint32_t fs);
extern void pcm54_set_streaming(bool s);

const uint32_t pcm54_rates[PCM54_N_RATES] = {44100, 48000, 88200, 96000};

static uint32_t cur_rate = 44100;        // rate host da chon
static uint8_t  clk_valid = 1;
static bool     mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];
static int16_t  volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];

static bool rate_ok(uint32_t fs) {
    for (int i = 0; i < PCM54_N_RATES; i++) if (pcm54_rates[i] == fs) return true;
    return false;
}

//--------------------------------------------------------------------
// Request SET cho entity (host ghi)
//--------------------------------------------------------------------
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf) {
    (void)rhport;
    uint8_t channel = TU_U16_LOW(p_request->wValue);
    uint8_t ctrl    = TU_U16_HIGH(p_request->wValue);
    uint8_t entity  = TU_U16_HIGH(p_request->wIndex);

    if (entity == UAC2_ENTITY_SPK_FEATURE_UNIT) {
        if (channel >= sizeof(mute)) return false;
        switch (ctrl) {
            case AUDIO_FU_CTRL_MUTE:
                TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_1_t));
                mute[channel] = ((audio_control_cur_1_t *)buf)->bCur;
                return true;
            case AUDIO_FU_CTRL_VOLUME:
                TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_2_t));
                volume[channel] = ((audio_control_cur_2_t *)buf)->bCur;
                return true;
            default:
                return false;
        }
    }

    if (entity == UAC2_ENTITY_CLOCK && ctrl == AUDIO_CS_CTRL_SAM_FREQ) {
        TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_4_t));
        uint32_t fs = ((audio_control_cur_4_t const *)buf)->bCur;
        if (!rate_ok(fs)) return false;
        cur_rate = fs;
        pcm54_request_rate(fs);
        return true;
    }
    return false;
}

//--------------------------------------------------------------------
// Request GET cho entity (host doc)
//--------------------------------------------------------------------
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    uint8_t channel = TU_U16_LOW(p_request->wValue);
    uint8_t ctrl    = TU_U16_HIGH(p_request->wValue);
    uint8_t entity  = TU_U16_HIGH(p_request->wIndex);

    if (entity == UAC2_ENTITY_SPK_INPUT_TERMINAL && ctrl == AUDIO_TE_CTRL_CONNECTOR) {
        audio_desc_channel_cluster_t ret;
        ret.bNrChannels     = CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX;
        ret.bmChannelConfig = (audio_channel_config_t)0;
        ret.iChannelNames   = 0;
        return tud_control_xfer(rhport, p_request, &ret, sizeof(ret));
    }

    if (entity == UAC2_ENTITY_SPK_FEATURE_UNIT) {
        if (channel >= sizeof(mute)) return false;
        switch (ctrl) {
            case AUDIO_FU_CTRL_MUTE:
                return tud_control_xfer(rhport, p_request, &mute[channel], 1);
            case AUDIO_FU_CTRL_VOLUME:
                switch (p_request->bRequest) {
                    case AUDIO_CS_REQ_CUR:
                        return tud_control_xfer(rhport, p_request, &volume[channel], sizeof(volume[channel]));
                    case AUDIO_CS_REQ_RANGE: {
                        // don vi 1/256 dB: -90 dB .. 0 dB, buoc 1 dB
                        audio_control_range_2_n_t(1) range = {
                            .wNumSubRanges = tu_htole16(1),
                            .subrange[0] = {.bMin = tu_htole16(-90 * 256), .bMax = tu_htole16(0), .bRes = tu_htole16(256)}
                        };
                        return tud_control_xfer(rhport, p_request, &range, sizeof(range));
                    }
                    default:
                        return false;
                }
            default:
                return false;
        }
    }

    if (entity == UAC2_ENTITY_CLOCK) {
        switch (ctrl) {
            case AUDIO_CS_CTRL_SAM_FREQ:
                switch (p_request->bRequest) {
                    case AUDIO_CS_REQ_CUR: {
                        audio_control_cur_4_t cur = {.bCur = tu_htole32(cur_rate)};
                        return tud_control_xfer(rhport, p_request, &cur, sizeof(cur));
                    }
                    case AUDIO_CS_REQ_RANGE: {
                        audio_control_range_4_n_t(PCM54_N_RATES) rng;
                        rng.wNumSubRanges = tu_htole16(PCM54_N_RATES);
                        for (int i = 0; i < PCM54_N_RATES; i++) {
                            rng.subrange[i].bMin = tu_htole32(pcm54_rates[i]);
                            rng.subrange[i].bMax = tu_htole32(pcm54_rates[i]);
                            rng.subrange[i].bRes = 0;
                        }
                        return tud_control_xfer(rhport, p_request, &rng, sizeof(rng));
                    }
                    default:
                        return false;
                }
            case AUDIO_CS_CTRL_CLK_VALID:
                return tud_control_xfer(rhport, p_request, &clk_valid, sizeof(clk_valid));
            default:
                return false;
        }
    }
    return false;
}

//--------------------------------------------------------------------
// Bat/tat streaming
//--------------------------------------------------------------------
bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;
    uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t alt = TU_U16_LOW(p_request->wValue);
    if (itf == ITF_NUM_AUDIO_STREAMING) pcm54_set_streaming(alt != 0);
    return true;
}

bool tud_audio_set_itf_close_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;
    uint8_t itf = TU_U16_LOW(p_request->wIndex);
    if (itf == ITF_NUM_AUDIO_STREAMING) pcm54_set_streaming(false);
    return true;
}

//--------------------------------------------------------------------
// Feedback endpoint: dem muc day FIFO (FIFO_COUNT). main.c chi rut FIFO khi DAC can nen muc nay phan anh toc do that
//--------------------------------------------------------------------
void tud_audio_feedback_params_cb(uint8_t func_id, uint8_t alt_itf, audio_feedback_params_t *feedback_param) {
    (void)func_id;
    (void)alt_itf;
    feedback_param->method      = AUDIO_FEEDBACK_METHOD_FIFO_COUNT;
    feedback_param->sample_freq = cur_rate;
}
