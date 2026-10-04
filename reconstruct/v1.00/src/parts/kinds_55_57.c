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
 * JUDGE: built-with -mm
 * JUDGE: data 0x3274..0x3290
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3274..0x3290. Connection points, 7 pairs a row, indexed [form != 0].
 */
struct point8 g_kinds_55_57_points[2][7] = {
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
 * 172c:10b6, image 0x18376 - two tables again, but chosen by the form at
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
        si = g_kinds_55_57_points[0];
    else
        si = g_kinds_55_57_points[1];

    for (i = 0, di = part->points; i < 7; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:1105, image 0x183c5 - four slots computed rather than copied. Two
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
    uint8_t a;                          /* [bp-1] */
    uint8_t c;                          /* [bp-2] */
    uint8_t b;                          /* [bp-3] */
    struct part_point *di;

    b = 0;
    if (part->kind == KIND_55) {
        a = 0x54;
    } else if (part->kind == KIND_57 && part->form == 8) {
        a = 0x69;
        b = 0x0a;
    } else {
        a = part->size[0].width - 1;
    }

    if (part->kind == KIND_57 && part->form == 0)
        c = 1;
    else
        c = part->size[0].height - 1;

    di = part->points;
    di->x = 0;
    di->y = b;
    di++;
    di->x = a;
    di->y = b;
    di++;
    di->x = a;
    di->y = c;
    di++;
    di->x = 0;
    di->y = c;

    part_finish_angles(part);

    part->attach[0].x = part->size[0].width >> 1;
    part->attach[0].y = 0;
}

/*
 * 172c:11a6, image 0x18466 - kind 57's step.
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
 * 172c:11d2, image 0x18492 - kind 57's drive hook.
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
            goto yes;
        goto no;
    }
    if (part->momentum + part->momentum > momentum) {
yes:
        return 1;
    }
no:
    return 0;
}
