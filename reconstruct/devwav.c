/*
 * The Sound Blaster capture, and nothing else. NOT a transcription.
 *
 * It sits in its own file rather than in `devdump.c` because two binaries need
 * it and they are not the same two that need the rest. `devtim` records a run
 * of the port; `tools/native` records the same run of the *original* under
 * emulation, driving this very `io.c` as its hardware - and that pair is the
 * only way to ask whether the port plays the right samples at the right
 * moments, since no screen comparison can hear anything. The hybrid links
 * `devstub.c` for the frame-dumping hooks it does not want, so pulling in all
 * of `devdump.c` to get the recorder was not an option.
 *
 * `tim` links neither, which is what keeps TIM_WAV a developer flag.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "io.h"

/*
 * OURS: capture every block the Sound Blaster is handed, as a WAV.
 *
 * `TIM_WAV=FILE` turns it on. A run mixes rates - the game plays effects at
 * 11 kHz and at least one sample at 22 kHz - so the blocks cannot simply be
 * concatenated; each is resampled to one output rate by point sampling, which
 * is what an eight-bit card sounds like anyway and keeps this a *capture*
 * rather than a filter with opinions.
 *
 * The gaps matter as much as the blocks. A capture that concatenates only what
 * was played is nine seconds of solid noise with no silence anywhere, and says
 * nothing about *when* a sound happened - so the quiet between blocks is
 * padded from a monotonic clock, and the file is a recording of the run rather
 * than a heap of its samples.
 *
 * The header is rewritten after every block, so a run that is killed - which
 * is how most of them end - still leaves a playable file. This is developer
 * tooling and lives here because `tim` must not have it.
 */
#define WAV_RATE 22050

static FILE   *wav_f;
static uint32_t wav_n;
static double   wav_t0;

static double wav_clock(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void wav_header(void)
{
    uint32_t data = wav_n, riff = 36 + data;
    uint8_t h[44];

    memcpy(h, "RIFF", 4);
    h[4] = (uint8_t)riff;        h[5] = (uint8_t)(riff >> 8);
    h[6] = (uint8_t)(riff >> 16); h[7] = (uint8_t)(riff >> 24);
    memcpy(h + 8, "WAVEfmt ", 8);
    h[16] = 16; h[17] = h[18] = h[19] = 0;
    h[20] = 1;  h[21] = 0;                    /* PCM        */
    h[22] = 1;  h[23] = 0;                    /* mono       */
    h[24] = (uint8_t)WAV_RATE; h[25] = (uint8_t)(WAV_RATE >> 8); h[26] = h[27] = 0;
    h[28] = (uint8_t)WAV_RATE; h[29] = (uint8_t)(WAV_RATE >> 8); h[30] = h[31] = 0;
    h[32] = 1;  h[33] = 0;                    /* block align */
    h[34] = 8;  h[35] = 0;                    /* bits        */
    memcpy(h + 36, "data", 4);
    h[40] = (uint8_t)data;        h[41] = (uint8_t)(data >> 8);
    h[42] = (uint8_t)(data >> 16); h[43] = (uint8_t)(data >> 24);

    fseek(wav_f, 0, SEEK_SET);
    fwrite(h, 1, sizeof h, wav_f);
    fseek(wav_f, 0, SEEK_END);
}

static void wav_block(const uint8_t *pcm, int32_t n, int32_t rate)
{
    int32_t out, i;

    if (!wav_f || n <= 0 || rate <= 0)
        return;

    {
        double at = wav_clock() - wav_t0;
        int64_t want = (int64_t)(at * WAV_RATE);

        while ((int64_t)wav_n < want) {
            fputc(0x80, wav_f);
            wav_n++;
        }
    }

    out = (int32_t)((int64_t)n * WAV_RATE / rate);
    for (i = 0; i < out; i++) {
        int32_t j = (int32_t)((int64_t)i * rate / WAV_RATE);

        if (j >= n)
            j = n - 1;
        fputc(pcm[j], wav_f);
    }
    wav_n += (uint32_t)out;
    wav_header();
}

void dev_wav_open(void)
{
    const char *path = getenv("TIM_WAV");

    if (path == NULL)
        return;

    wav_f = fopen(path, "wb");
    if (wav_f == NULL) {
        fprintf(stderr, "dev: cannot write %s\n", path);
        return;
    }

    wav_n = 0;
    wav_t0 = wav_clock();
    wav_header();
    io_on_pcm_tap(wav_block);
}

