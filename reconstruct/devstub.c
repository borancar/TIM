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
