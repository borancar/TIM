/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The dynamite**: its hit, setup, step and flip, and the burst.
 *
 * The thirteenth module of the original's **code segment 172c**, image
 * 0x184f7..0x18689 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3290..0x32a4
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3290..0x329a. Connection points, 5 pairs.
 */
struct point8 DYNAMITE_POINTS_3290[5] = {
    { 0x00, 0x0e }, { 0x05, 0x00 }, { 0x25, 0x12 }, { 0x1b, 0x1b },
    { 0x14, 0x1b },
};

/*
 * DGROUP 0x329a..0x32a4. Connection points, 5 pairs.
 */
struct point8 DYNAMITE_POINTS_329A[5] = {
    { 0x0a, 0x12 }, { 0x2a, 0x00 }, { 0x2f, 0x0e }, { 0x1b, 0x1b },
    { 0x14, 0x1b },
};

/*
 * 172c:1237, image 0x184f7 - kind 19's hit test. A kind-0x14 part bursts it,
 * and `burst_dynamite` is given the **struck part** here where kind 20's hit
 * gives it the collision record. The two call sites disagree and are
 * transcribed as they are.
 */
uint16_t part_hit_dynamite(struct part *part)
{
    struct part *other = part->contact;

    if (part->kind == KIND_BULLET)
        burst_dynamite(other);

    return 1;
}

/*
 * 172c:1261, image 0x18521 - a setup.
 *
 * The same arrangement with five points: 1 and 0x329a one way, 0x2d and 0x3290
 * the other, and 0xf at +0x73 either way. Ten bytes between the tables.
 */
void part_setup_dynamite(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        part->hold.x = 1;
        src = DYNAMITE_POINTS_329A;
    } else {
        part->hold.x = 0x2d;
        src = DYNAMITE_POINTS_3290;
    }

    part->hold.y = 0x0f;

    for (i = 0, dst = part->points; i < 5; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:12c2, image 0x18582 - kind 19's step. The balloon.
 *
 * It starts itself once its counter passes 0x14, then rises a frame at a time
 * until form 5, which is where it bursts.
 */
void part_step_dynamite(struct part *part)
{
    if (part->direction == 0 && part->spin > 0x14)
        part->direction = 1;

    if (part->direction != 0) {
        if (part->form == 5) {
            burst_dynamite(part);
        } else {
            part->form++;
            place_object_for_draw(part);
        }
    }
}

/*
 * 172c:12fc, image 0x185bc - kind 19's flip.
 *
 * The bit-4 flip and its setup, and then **only two of the three redraws**:
 * `place_object_for_draw` is not called here where the 03d2 family calls it.
 */
void part_flip_dynamite(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_dynamite(part);

    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:1328, image 0x185e8
 *
 * Burst it: the form goes to 5, a kind 0x29 - the shreds - is made and put on
 * the list at DGROUP 0x521b at a fixed offset up and to the left, sound 8
 * plays, and the dynamite itself registers its shapes one last time and hides.
 *
 * A burst that could not get the shreds from the heap still hides the part,
 * because the `jmp` past the allocation lands *after* the form was set and
 * before the hiding - so a machine out of memory loses the shreds and not the
 * burst.
 */
void burst_dynamite(struct part *part)
{
    struct part *si;

    part->form = 5;

    if ((si = make_part(KIND_BLAST)) == NULL)
        goto done;

    play_sound(8);

    insert_sorted(si, &g_placed_parts);
    si->flags_06 |= 0x10;

    si->pos[0].x = part->pos[0].x - 15;
    si->pos[0].y = part->pos[0].y - 19;

    si->fx = si->pos[0].x;
    si->fx <<= 9;
    si->fy = si->pos[0].y;
    si->fy <<= 9;

    place_object_for_draw(si);

done:
    mark_part_shapes(part, 3);
    part->flags_08 |= 0x2000;
}
