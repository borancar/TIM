/*
 * The shipping binary's stand-in for the developer hooks. NOT a transcription.
 *
 * `reconstruct/devdump.c` carries the flags a comparison needs - TIM_CLICK,
 * TIM_POINTER, TIM_FLIPS, TIM_FLIPHASH - and the Makefile's own rule is that
 * none of it may reach what ships. It was reaching it: `devdump.c` sat in the
 * object list both binaries link, so the game carried every one of those flags
 * and `tools/` drove the game rather than the developer binary.
 *
 * `io.c` calls `dev_flip_dump` on the guest's page flip whatever it is linked
 * into, because the flip is the guest's business and the *dumping* is not. So
 * the shipping binary gets this, which does nothing and is one call, and
 * `devtim` gets the real one.
 */
#include <stdint.h>

#include "io.h"

void dev_flip_dump(int32_t flip)
{
    (void)flip;
}

void dev_final_frame(void)
{
}

void dev_sound_played(int16_t id)
{
    (void)id;
}

int32_t dev_date_override(uint16_t *year, uint16_t *monthday,
                          uint16_t *weekday)
{
    (void)year; (void)monthday; (void)weekday;
    return 0;
}

void dev_file_written(const char *name, const uint8_t *data, uint32_t len)
{
    (void)name;
    (void)data;
    (void)len;
}

int32_t dev_survey_hook(uint16_t off, uint16_t kind)
{
    (void)off;
    (void)kind;
    return 0;
}
