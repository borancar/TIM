/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The balloon**: its setup, hit, step and drive.
 *
 * The second module of the original's **code segment 172c**, image
 * 0x173ed..0x175f2 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3182..0x3192
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3182..0x3192. Connection points, 8 pairs.
 */
struct point8 BALLOON_POINTS_3182[8] = {
    { 0x00, 0x0a }, { 0x0c, 0x00 }, { 0x16, 0x00 }, { 0x1f, 0x0a },
    { 0x1f, 0x1c }, { 0x13, 0x2b }, { 0x0b, 0x2b }, { 0x00, 0x1d },
};

/*
 * 172c:012d, image 0x173ed - a setup.
 *
 * The same eight connection points every setup writes, but read from the
 * table at DGROUP 0x3182 rather than built from immediates - which is why it
 * is not one of the `part_setups` rows. Two bytes per point there, four per
 * point in the part, so the two strides differ and the copy walks both.
 *
 * Ends at `part_finish_angles` like the rest.
 */
void part_setup_balloon(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = BALLOON_POINTS_3182;
    for (i = 0, si = part->points; i < 8; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:016e, image 0x1742e - kind 4's hit test.
 *
 * Only one kind of arrival counts: 0x14. That sets the part at the object's
 * +0x84 going at +0x12, and anything else touching it does nothing. The answer
 * is 1 either way.
 */
uint16_t part_hit_balloon(struct part *part)
{
    struct part *di = part->contact;

    if (part->kind == KIND_BULLET)
        di->direction = 1;

    return 1;
}

/*
 * 172c:018e, image 0x1744e - kind 4's step.
 *
 * A part that hands its belt over to something else and then disappears. At
 * form 6 it registers its shapes one last time and hides itself - bit 13 of
 * +8 - and that is the end of it.
 *
 * Before then, and only while +0x12 is exactly 1, it makes a kind-0x31 anchor,
 * puts it on the list at DGROUP 0x5179, and moves its belt across: the anchor
 * takes the belt at +0x66 and the link at +0x5a, the part on the far side of
 * that link is pointed back at the anchor through whichever of its own two
 * links matched - `match_field_5a_5c` - and the belt record's own end, +2 or
 * +4, is repointed too. The anchor lands on the belt's tangent point for that
 * end, carried in sixteenths the usual way, and this part lets go of both.
 *
 * Either way the form steps on, and the first step plays sound 0x0e.
 */
void part_step_balloon(struct part *part)
{
    struct part *si;
    uint16_t k;                         /* [bp-2] */
    struct part *link;                  /* [bp-4] */
    struct belt *belt;                  /* [bp-6] */

    if (part->direction != 0) {
        part->flags_08 |= 0x40;

        if (part->form == 6) {
            mark_part_shapes(part, 3);
            part->flags_08 |= 0x2000;
        } else {
            if (part->direction == 1
                && (belt = part->belt[0]) != NULL
                && (si = make_part(KIND_ANCHOR)) != NULL) {
                insert_sorted(si, &DG5179.moving_parts);
                si->flags_06 |= 0x10;
                si->belt[0] = belt;
                si->link[0] = part->link[0];
                link = si->link[0];

                if ((k = match_field_5a_5c(part, link)) != 0xffff)
                    link->link[k] = si;

                if (belt->end_a == part) {
                    belt->end_a = si;
                    si->pos[0].x = belt->pt[0][0].x;
                    si->pos[0].y = belt->pt[0][0].y;
                } else {
                    belt->end_b = si;
                    si->pos[0].x = belt->pt[0][1].x;
                    si->pos[0].y = belt->pt[0][1].y;
                }

                si->fx = si->pos[0].x;
                si->fx <<= 9;
                si->fy = si->pos[0].y;
                si->fy <<= 9;

                place_object_for_draw(si);

                part->belt[0] = 0;
                part->link[0] = 0;
            }

            if (part->form == 0)
                play_sound(0x0e);

            part->form++;
            place_object_for_draw(part);
        }
    }
}

/*
 * 172c:02cd, image 0x1758d - kind 4's drive.
 *
 * Seven arguments like every drive, and it uses four of them: the asking part
 * in the first, the driven part in the second, a mode in the fourth and a
 * 32-bit limit split across the sixth and seventh.
 *
 * Mode 1 does not answer a question at all - it steps the word at +0x0e of
 * whatever +0x66 points at and returns 0.
 *
 * Otherwise the answer is whether the driven part's 32-bit value at +0x3c is
 * past the limit. **An asker of kind 3 is compared against that value and
 * anything else against twice it**, which is the whole difference between the
 * two arms. The compare is signed on the high word and unsigned on the low,
 * which is what a 32-bit signed compare is, so it is written as one.
 */
uint16_t part_drive_02cd(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct belt *belt = p2->belt[0];   /* [bp-2] */

    if (p4 == 1) {
        belt->v[0]++;
        return 0;
    }

    if (p1->kind == KIND_SEESAW) {
        if (p2->momentum > p6)
            goto yes;
        goto no;
    }
    if (p2->momentum + p2->momentum > p6) {
yes:
        return 1;
    }
no:
    return 0;
}
