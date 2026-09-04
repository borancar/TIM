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
/*
 * `clock_gettime` is POSIX, not C, so it needs the feature test macro. This
 * used to compile without one because fluidsynth's pkg-config cflags happened
 * to define it; removing that dependency took the define with it, which is
 * exactly the kind of thing an inherited flag hides.
 */
#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "io.h"
#include "src/opl.h"

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

/*
 * OURS: `TIM_SFXDIR=<dir>` writes every *distinct* block the card is handed as
 * its own WAV, named by length, rate and checksum.
 *
 * Distinct by content, not by call: a looping effect is handed over again on
 * every pass and a sound that plays in ten places is still one waveform. The
 * Fletcher-16 that `io.c` already computes for the trace is what identifies
 * them, so a repeat costs a lookup and no file.
 *
 * This is a *capture*, not an extraction from the archive: the bytes are what
 * the DMA controller was pointed at, at the rate DSP command 0x40 set, so
 * there is no container format to guess at and nothing to get wrong. The cost
 * is coverage - a sound the run never reaches is a sound this never writes.
 */
static char     sfx_dir[512];
static uint16_t sfx_seen[512];
static int32_t  sfx_n;

static void sfx_block(const uint8_t *pcm, int32_t n, int32_t rate)
{
    uint16_t a = 0, b = 0, sum;
    int32_t i;
    char path[600];
    FILE *f;
    uint8_t h[44];
    uint32_t riff = 36 + (uint32_t)n;

    if (sfx_dir[0] == 0 || n <= 1)
        return;

    for (i = 0; i < n; i++) {
        a = (uint16_t)((a + pcm[i]) % 255);
        b = (uint16_t)((b + a) % 255);
    }
    sum = (uint16_t)((b << 8) | a);

    for (i = 0; i < sfx_n; i++)
        if (sfx_seen[i] == sum)
            return;
    if (sfx_n < (int32_t)(sizeof sfx_seen / sizeof sfx_seen[0]))
        sfx_seen[sfx_n++] = sum;

    snprintf(path, sizeof path, "%s/sfx_%05d_%05dhz_%04x.wav",
             sfx_dir, n, rate, sum);
    f = fopen(path, "wb");
    if (f == NULL)
        return;

    memcpy(h, "RIFF", 4);
    h[4]=(uint8_t)riff; h[5]=(uint8_t)(riff>>8);
    h[6]=(uint8_t)(riff>>16); h[7]=(uint8_t)(riff>>24);
    memcpy(h + 8, "WAVEfmt ", 8);
    h[16]=16; h[17]=h[18]=h[19]=0;
    h[20]=1; h[21]=0; h[22]=1; h[23]=0;
    h[24]=(uint8_t)rate; h[25]=(uint8_t)(rate>>8);
    h[26]=(uint8_t)(rate>>16); h[27]=(uint8_t)(rate>>24);
    h[28]=(uint8_t)rate; h[29]=(uint8_t)(rate>>8);
    h[30]=(uint8_t)(rate>>16); h[31]=(uint8_t)(rate>>24);
    h[32]=1; h[33]=0; h[34]=8; h[35]=0;
    memcpy(h + 36, "data", 4);
    h[40]=(uint8_t)n; h[41]=(uint8_t)(n>>8);
    h[42]=(uint8_t)(n>>16); h[43]=(uint8_t)(n>>24);
    fwrite(h, 1, sizeof h, f);
    fwrite(pcm, 1, (size_t)n, f);
    fclose(f);
    fprintf(stderr, "sfx: %s\n", path);
}

/*
 * OURS: render the OPL2 while one sound plays, into its own WAV.
 *
 * `TIM_FMDIR=<dir>` with `TIM_SFXALL` writes one file per sound identifier.
 * Most of this game's twenty effects are FM and never touch the DAC, so
 * `TIM_SFXDIR` - which captures at the DMA - cannot see them at all; this is
 * the other half of the same question.
 *
 * The chip has to be pulled by hand here. Nothing else is doing it: a headless
 * run opens no audio device, so there is no callback asking for samples, and
 * the sequencer writing registers from the timer thread is the only thing
 * moving. So render in small chunks and sleep between them, which lets those
 * writes land between renders roughly where they would have in real time.
 */
void dev_fm_capture(int32_t id, double seconds)
{
    const char *dir = getenv("TIM_FMDIR");
    char path[600];
    FILE *f;
    uint8_t h[44];
    uint32_t n = 0, riff;
    int16_t buf[512];
    double t_end;

    if (dir == NULL)
        return;

    snprintf(path, sizeof path, "%s/fm_sound%02d.wav", dir, id);
    f = fopen(path, "wb");
    if (f == NULL)
        return;

    memset(h, 0, sizeof h);
    fwrite(h, 1, sizeof h, f);          /* header rewritten at the end */

    t_end = wav_clock() + seconds;
    while (wav_clock() < t_end) {
        struct timespec ts;

        opl_render(buf, (uint32_t)(sizeof buf / sizeof buf[0]));
        fwrite(buf, sizeof buf[0], sizeof buf / sizeof buf[0], f);
        n += (uint32_t)(sizeof buf / sizeof buf[0]);

        ts.tv_sec = 0;
        ts.tv_nsec = (long)(1e9 * (double)(sizeof buf / sizeof buf[0])
                            / (double)OPL_SAMPLE_RATE);
        nanosleep(&ts, NULL);
    }

    riff = 36 + n * 2;
    memcpy(h, "RIFF", 4);
    h[4]=(uint8_t)riff; h[5]=(uint8_t)(riff>>8);
    h[6]=(uint8_t)(riff>>16); h[7]=(uint8_t)(riff>>24);
    memcpy(h + 8, "WAVEfmt ", 8);
    h[16]=16; h[20]=1; h[22]=1;
    h[24]=(uint8_t)(OPL_SAMPLE_RATE); h[25]=(uint8_t)(OPL_SAMPLE_RATE>>8);
    h[26]=(uint8_t)(OPL_SAMPLE_RATE>>16);
    h[28]=(uint8_t)(OPL_SAMPLE_RATE*2); h[29]=(uint8_t)((OPL_SAMPLE_RATE*2)>>8);
    h[30]=(uint8_t)((OPL_SAMPLE_RATE*2)>>16);
    h[32]=2; h[34]=16;
    memcpy(h + 36, "data", 4);
    h[40]=(uint8_t)(n*2); h[41]=(uint8_t)((n*2)>>8);
    h[42]=(uint8_t)((n*2)>>16); h[43]=(uint8_t)((n*2)>>24);
    fseek(f, 0, SEEK_SET);
    fwrite(h, 1, sizeof h, f);
    fclose(f);
    fprintf(stderr, "fm: %s (%.2fs)\n", path, (double)n / OPL_SAMPLE_RATE);
}

void dev_sfx_open(void)
{
    const char *dir = getenv("TIM_SFXDIR");

    if (dir == NULL)
        return;
    snprintf(sfx_dir, sizeof sfx_dir, "%s", dir);
    io_on_pcm_tap2(sfx_block);
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

