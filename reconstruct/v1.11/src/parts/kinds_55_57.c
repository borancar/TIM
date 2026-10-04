/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Kinds 55, 56 and 57**: their setups, 57's step and its drive.
 *
 * The twelfth module of the original's **code segment 172c**, image
 * 0x18376..0x184f7 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x2e16..0x2e32
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x2e16..0x2e32. Outline points, 7 pairs a row, indexed [form != 0].
 */
struct point8 g_kinds_55_57_outline[2][7] = {
    {
        { 25, 0 }, { 25, 60 }, { 114, 60 }, { 114, 0 },
        { 248, 0 }, { 248, 182 }, { 0, 182 },
    },
    {
        { 240, 0 }, { 248, 0 }, { 248, 16 }, { 246, 20 },
        { 244, 20 }, { 242, 16 }, { 240, 16 },
    },
};

/*
 * 190f:10bd, image 0x1a1ad - two tables again, but chosen by the form at
 * +0x0c rather than by the flag at +8: zero takes 0x3274 and anything else
 * 0x3282. Those two sit right after 0x3266, which the 0x1075 copy above
 * uses, so all three are one array of seven-pair rows and this picks the
 * second or the third.
 */
void part_setup_kind_56(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->form == 0)
        si = g_kinds_55_57_outline[0];
    else
        si = g_kinds_55_57_outline[1];

    for (i = 0, di = part->points; i < 7; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 190f:110c, image 0x1a1fc - four slots computed rather than copied. Two
 * bytes are worked out first and then laid into the corners: (0,b), (a,b),
 * (a,c), (0,c). `a` is 0x54 for kind 0x37, 0x69 for kind 0x39 in form 8 -
 * which also makes `b` 0x0a rather than 0 - and otherwise one less than the
 * part's width; `c` is 1 for kind 0x39 in form 0 and otherwise one less
 * than its height. So the general case is "the part's own box", and the two
 * named kinds are exceptions carved out of it.
 *
 * Afterwards, and unlike every other setup, it goes on to set +0x6a to half
 * the width and +0x6b to zero.
 */
void part_setup_kinds_55_57(struct part *part)
{
    uint8_t right;                      /* [bp-1] */
    uint8_t bottom;                     /* [bp-2] */
    uint8_t left;                       /* [bp-3] */
    uint8_t top;                        /* [bp-4] */
    struct part_point *di;

    left = top = 0;
    if (part->kind == KIND_55) {
        right = 84;
    } else if (part->kind == KIND_57 && part->form == 8) {
        right = 105;
        top = 10;
    } else {
        right = part->size[0].width - 1;
    }

    /* 1.11: kind 57's form 0 is a narrow strip at the far end */
    if (part->kind == KIND_57 && part->form == 0) {
        left = 0x6e;
        right = 111;
        bottom = 1;
    } else
        bottom = part->size[0].height - 1;

    /* 1.11: kind 55 in form 1 keeps the points it has */
    if (!(part->kind == KIND_55 && part->form == 1)) {
        di = part->points;
        di->x = left;
        di->y = top;
        di++;
        di->x = right;
        di->y = top;
        di++;
        di->x = right;
        di->y = bottom;
        di++;
        di->x = left;
        di->y = bottom;
    }

    part_finish_angles(part);

    part->attach[0].x = part->size[0].width >> 1;
    part->attach[0].y = 0;
}

/*
 * 190f:11c9, image 0x1a2b9 - kind 57's step.
 *
 * Four lines: in form 1, and only once something has given it a sideways
 * velocity at +0x36, it goes to form 3 and plays sound 3. Nothing else happens
 * to it at all.
 */
void part_step_kind_57(struct part *part)
{
    if (part->form == 1 && part->vel_x != 0) {
        part->form = 3;
        place_object_for_draw(part);
        play_sound(3);
    }
}

/*
 * 190f:11f5, image 0x1a2e5 - kind 57's drive hook.
 *
 * Flags of exactly 1 is the counting pass `part_drive_light` also recognises:
 * the rope's +0x0e goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise it is a contest of momentum. The part's own at +0x3c - the long
 * the record doc calls speed, weight times how fast it is going - is measured
 * against the momentum the drive arrived with, and the part refuses when its
 * own is the greater, which ends the caller's walk. Driven straight from a
 * kind 3 - the motor - the part's momentum counts once; through anything else
 * it counts *twice*, so the same drive that turns a thing directly can fail to
 * turn it at one more remove.
 */
uint16_t part_drive_kind_57(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t momentum)
{
    struct rope *rope = part->rope[0];   /* [bp-2] */

    if (flags == 1) {
        rope->v[0]++;
        return 0;
    }

    if (from->kind == KIND_SEESAW) {
        if (part->momentum > momentum)
            return 1;
    } else if (part->momentum + part->momentum > momentum)
        return 1;
    return 0;
}
