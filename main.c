// USB audio -> NOS (khong oversampling, khong loc so) -> bus 16 bit song song -> 2x PCM54HP
// CHUA BIEN DICH/CHAY THU. Xem README_hardware.md.
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "tusb.h"
#include "pcm54_par.pio.h"

#define PIN_DATA   0                // GP0..15
#define PIN_CLK    16               // GP16 = CLK_L, GP17 = CLK_R
// Ma hoa vao PCM54 tu two's complement int16. 0x0000 = giu nguyen, 0x8000 = offset binary,
// 0xFFFF = dao het bit (complementary two's complement). KIEM TRA Table I trong datasheet / board Denon.
#define PCM54_XOR  0xFFFFu

// RP2350: TRANS_COUNT.MODE = 0xF (endless) o bit 31:28. SDK 2.1.1 chua co ham dma_encode_endless_transfer_count().
#define DMA_ENDLESS_COUNT 0xF0000000u

#define RING_FRAMES   512                          // 512 * 2 word * 4 B = 4096 B = 2^12
#define RING_WORDS    (RING_FRAMES * 2)
#define TARGET_FRAMES 128                          // muc dem muc tieu (~2.9 ms @44.1k)
#define LOW_FRAMES    48                           // duoi muc nay moi chen im lang

static uint32_t ring[RING_WORDS] __attribute__((aligned(4096)));
static int dma_ch;
static PIO pio = pio0;
static uint sm = 0, pio_off;

static uint32_t wr_abs, play_abs, last_play;
static volatile bool     streaming;                // set tu callback tud_audio_set_itf_cb (alt != 0)
static volatile uint32_t pending_rate;             // set tu callback set-sample-rate cua UAC2
static uint32_t cur_rate = 44100;

// ---------------- dat sample rate: va lenh PIO ----------------
// Chu ky cap L/R = P chu ky PIO, clkdiv NGUYEN D (khong chia le -> khong them jitter):
//   P = round(sysclk / (fs * D)),  P trong [44, 1067],  P = 12 + 32*(K+1) + d
// Sai so toc do <= ~0.06 %; USB async feedback (FIFO_COUNT) tu bu.
#define PIO_P_MIN 44
#define PIO_P_MAX 1067
static void set_rate(uint32_t fs) {
    uint32_t sys = clock_get_hz(clk_sys);
    uint32_t D = (uint32_t)ceilf((float)sys / ((float)fs * 1000.0f));
    if (D < 1) D = 1;
    uint32_t P;
    for (;;) {
        P = (uint32_t)lroundf((float)sys / ((float)fs * (float)D));
        if (P > PIO_P_MAX) D++; else break;
    }
    if (P < PIO_P_MIN) P = PIO_P_MIN;
    uint32_t Q = P - 12;
    uint32_t K = Q / 32 - 1;                       // 0..31
    uint32_t d = Q % 32;                           // 0..31
    pio_sm_set_enabled(pio, sm, false);
    pio->instr_mem[pio_off + 10] = pio_encode_set(pio_x, K);
    pio->instr_mem[pio_off + 12] = pio_encode_nop() | pio_encode_delay(d);
    pio_sm_set_clkdiv(pio, sm, (float)D);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);
    pio_sm_exec(pio, sm, pio_encode_jmp(pio_off));
    pio_sm_set_enabled(pio, sm, true);
    cur_rate = fs;
}

// ---------------- DMA ring ----------------
static inline uint32_t poll_play(void) {
    uint32_t p = ((dma_hw->ch[dma_ch].read_addr - (uint32_t)(uintptr_t)ring) >> 3) & (RING_FRAMES - 1);
    play_abs += (p - last_play) & (RING_FRAMES - 1);
    last_play = p;
    return play_abs;
}
static inline void put_frame(int16_t l, int16_t r) {
    uint32_t i = (wr_abs & (RING_FRAMES - 1)) * 2;
    ring[i]     = ((uint16_t)l) ^ PCM54_XOR;
    ring[i + 1] = ((uint16_t)r) ^ PCM54_XOR;
    wr_abs++;
}

// Giu muc dem ~TARGET_FRAMES. Chi rut FIFO USB khi can -> muc day FIFO USB phan anh toc do tieu thu that
// -> feedback endpoint (AUDIO_FEEDBACK_METHOD_FIFO_COUNT) hoat dong dung.
static void audio_pump(void) {
    uint32_t level = wr_abs - poll_play();
    if (level >= TARGET_FRAMES) return;
    uint32_t need = TARGET_FRAMES - level;
    uint32_t avail = streaming ? tud_audio_available() / 4 : 0;
    uint32_t n = need < avail ? need : avail;
    if (n > 32) n = 32;
    if (n) {
        int16_t tmp[64];
        tud_audio_read(tmp, n * 4);
        for (uint32_t i = 0; i < n; i++) put_frame(tmp[2 * i], tmp[2 * i + 1]);
    } else if (level < LOW_FRAMES) {
        for (int i = 0; i < 16; i++) put_frame(0, 0);   // underrun / khong stream: im lang
    }
}

// ---------------- USB ----------------
// descriptors + callbacks UAC2: xem usb_descriptors.c, usb_audio.c, tusb_config.h
// (feedback FIFO_COUNT: tud_audio_feedback_params_cb trong usb_audio.c)
uint32_t pcm54_current_rate(void) { return cur_rate; }
void pcm54_request_rate(uint32_t fs) { pending_rate = fs; }
void pcm54_set_streaming(bool s) { streaming = s; }

int main(void) {
    for (uint32_t i = 0; i < RING_WORDS; i++) ring[i] = 0u ^ PCM54_XOR;
    wr_abs = TARGET_FRAMES * 2;

    pio_off = pio_add_program(pio, &pcm54_par_program);
    pcm54_par_program_init(pio, sm, pio_off, PIN_DATA, PIN_CLK);

    dma_ch = dma_claim_unused_channel(true);
    dma_channel_config dc = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_read_increment(&dc, true);
    channel_config_set_write_increment(&dc, false);
    channel_config_set_dreq(&dc, pio_get_dreq(pio, sm, true));
    channel_config_set_ring(&dc, false, 12);          // ring doc 4 KB
    dma_channel_configure(dma_ch, &dc, &pio->txf[sm], ring, DMA_ENDLESS_COUNT, true);  // RP2350: dem vo han
    set_rate(44100);

    tud_init(BOARD_TUD_RHPORT);                        // BOARD_TUD_RHPORT dinh nghia trong tusb_config.h
    for (;;) {
        tud_task();
        if (pending_rate && pending_rate != cur_rate) {
            uint32_t fs = pending_rate; pending_rate = 0;
            set_rate(fs);
            last_play = 0; play_abs = 0; wr_abs = TARGET_FRAMES * 2;   // DMA se tiep tuc tu vi tri hien tai: dong bo lai
            dma_channel_abort(dma_ch);
            for (uint32_t i = 0; i < RING_WORDS; i++) ring[i] = 0u ^ PCM54_XOR;
            dma_channel_configure(dma_ch, &dc, &pio->txf[sm], ring, DMA_ENDLESS_COUNT, true);  // RP2350: dem vo han
        } else {
            audio_pump();
        }
    }
}
