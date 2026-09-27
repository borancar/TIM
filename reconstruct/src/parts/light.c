/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The light**: its setup, hit, step, flip and drive.
 *
 * The twenty-eighth module of the original's **code segment 172c**, image
 * 0x19e18..0x19f43 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x339a..0x33aa
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * DGROUP 0x339a..0x33aa. Connection points, 4 points.
 */
struct point16 LIGHT_POINTS_339A[4] DGROUP_AT(0x339a) = {
    { 0x0015, 0x0033 }, { 0x001d, 0x004f }, { 0x0014, 0x0019 },
    { 0x001c, 0x0023 },
};

/*
 * 172c:2b58, image 0x19e18 - no connection points, only the grab box, and both
 * its bytes come out of one table indexed by the part's form at +0x0c.
 */
void part_setup_light(struct part *part)
{
    part->attach[0].x = (uint8_t)LIGHT_POINTS_339A[part->form].x;
    part->attach[0].y = (uint8_t)LIGHT_POINTS_339A[part->form].y;
}

/*
 * 172c:2b7e, image 0x19e3e - kind 29's hit test.
 *
 * The same do-nothing as `part_hit_generator`, down to the unused local.
 */
uint16_t part_hit_light(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);   /* read, and never used */

    (void)other;
    return 1;
}

/*
 * 172c:2b99, image 0x19e59 - kind 29's step.
 *
 * Only forms 0 and 2 move on, and only while +0x12 says it is on: the form
 * steps by one and its own setup runs again, because this kind's connection
 * points depend on the form.
 */
void part_step_light(struct part *part)
{
    if (part->direction != 0 && (part->form == 0 || part->form == 2)) {
        part->form++;
        part_setup_light(part);
        place_object_for_draw(part);
    }
}

/*
 * 172c:2bc5, image 0x19e85 - kind 29's flip, a form swing like kind 21's but
 * between 0 and **2** rather than 0 and 4, copied into +0x90 the same way.
 */
void part_flip_light(struct part *part)
{
    if (part->form != 0)
        part->form = 0;
    else
        part->form = 2;

    part->start_form = part->form;

    part_setup_light(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2c19, image 0x19ed9 - kind 29's drive hook.
 *
 * The arguments are the seven `drive_belts` hands over; this one uses only the
 * part at +8 and the flags at +0x0c.
 *
 * Flags of exactly 1 means "count how many belts reach here": the belt's +0x0e
 * goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise only bits 1, 2 and 15 of the flags are kept. A belt running that
 * way over a part already going - or bit 1 on its own - refuses, which is what
 * stops the drive: it answers 1 and the caller's walk ends. Bit 2 on a part
 * that is *not* going starts it instead, with sound 0x11, and answers 0.
 */
uint16_t part_drive_2c19(struct part *p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t p6)
{
    uint16_t kept;                      /* [bp-2] */
    struct belt *belt;                  /* [bp-4] */

    belt = BELT_PTR(si->belt_ptr[0]);

    if (flags == 1) {
        belt->v[0]++;
        return 0;
    }

    flags &= 0x8006;
    kept = flags & 0x7fff;

    if (kept == 2)
        goto yes;
    if (kept == 4 && si->direction != 0) {
yes:
        return 1;
    }

    if (flags == 4 && si->direction == 0) {
        play_sound(0x11);
        si->direction = 1;
    }

    return 0;
}
