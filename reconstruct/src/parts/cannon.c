/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The cannon**: its step, setup and flip.
 *
 * The eighth module of the original's **code segment 172c**, image
 * 0x17d1d..0x17edc - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3232..0x3252
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3232..0x3242. Connection points, 8 pairs.
 */
struct point8 CANNON_POINTS_3232[8] DGROUP_AT(0x3232) = {
    { 0x09, 0x25 }, { 0x12, 0x06 }, { 0x3c, 0x00 }, { 0x3f, 0x14 },
    { 0x2f, 0x1e }, { 0x2f, 0x27 }, { 0x22, 0x33 }, { 0x16, 0x33 },
};

/*
 * DGROUP 0x3242..0x3252. Connection points, 8 pairs.
 */
struct point8 CANNON_POINTS_3242[8] DGROUP_AT(0x3242) = {
    { 0x00, 0x14 }, { 0x03, 0x00 }, { 0x2d, 0x06 }, { 0x36, 0x25 },
    { 0x29, 0x33 }, { 0x1d, 0x33 }, { 0x10, 0x27 }, { 0x10, 0x1e },
};

/*
 * 172c:0a5d, image 0x17d1d - kind 18's step. The cannon.
 *
 * It starts itself once its +0x9c has counted past 0x14, then plays its eleven
 * frames: 0 to 7 one per step, and 7 held until +0x9c has gone up three more.
 * Frame 8 is the bang, and frame 9 fires - `make_part` builds a kind 0x2b, puts
 * it on the list at DGROUP 0x5179 and gives it a position and a velocity, left
 * or right by the mirror bit at +8.
 *
 * The velocity is the pair at +0x36 and +0x38 - 0xd000 or 0x3000 across and
 * 0xf000 down - and the position is carried in sixteenths as well, shifted left
 * by nine into +0x16 and +0x1a, which is the same wind-up `reset_machine` does.
 *
 * A cannon that could not get the shot from the heap simply does not fire.
 */
void part_step_cannon(struct part *part)
{
    struct part *si;

    if (part->direction == 0 && part->spin > 0x14)
        part->direction = 1;

    if (part->direction != 0 && part->form != 0x0b) {
        if (part->form != 7) {
            part->form++;
        } else {
            part->spin++;
            if (part->spin > 3)
                part->form++;
        }

        place_object_for_draw(part);

        if (part->form == 8)
            play_sound(6);

        if (part->form == 9 && (si = make_part(KIND_CANNON_BALL)) != NULL) {
            insert_sorted(si, &DG5179.moving_parts);
            si->flags_06 |= 0x10;

            if (part->flags_08 & 0x10) {
                si->pos[0].x = part->pos[0].x - 0x30;
                si->pos[1].x = si->pos[2].x = si->pos[0].x + 0x18;
                si->vel_x = (int16_t)0xd000;
            } else {
                si->pos[0].x = part->pos[0].x + 0x61;
                si->pos[1].x = si->pos[2].x = si->pos[0].x - 0x18;
                si->vel_x = 0x3000;
            }

            si->pos[0].y = part->pos[0].y - 7;
            si->pos[1].y = si->pos[2].y = si->pos[0].y + 8;
            si->vel_y = (int16_t)0xf000;

            clamp_record_pair(si);

            si->fx = si->pos[0].x;
            si->fx <<= 9;
            si->fy = si->pos[0].y;
            si->fy <<= 9;

            place_object_for_draw(si);
        }
    }
}

/*
 * 172c:0b88, image 0x17e48 - a setup.
 *
 * Eight points, and a width at +0x72 that goes with them: 0x3e and the table
 * at 0x3242 one way round, 1 and 0x3232 the other. The height at +0x73 is 3
 * either way. The two tables are sixteen bytes apart, which is the eight
 * points.
 */
void part_setup_cannon(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        part->hold.x = 0x3e;
        src = CANNON_POINTS_3242;
    } else {
        part->hold.x = 1;
        src = CANNON_POINTS_3232;
    }

    part->hold.y = 3;

    for (i = 0, dst = part->points; i < 8; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:0be9, image 0x17ea9 - kind 18's flip.
 *
 * Byte for byte the same routine as `part_flip_bellow` above with a different
 * setup behind it - checked as bytes, not assumed: of the twenty flips in this
 * segment only four are this shape and the other sixteen are not, so the
 * family is real but small.
 */
void part_flip_cannon(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_cannon(part);

    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
