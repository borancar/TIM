/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The blast**: its step, the speed it gives by mass, and the split of a part it catches.
 *
 * The sixteenth module of the original's **code segment 172c**, image
 * 0x18909..0x18c9b - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 190f:164a, image 0x1a73a - kind 41's step. The blast.
 *
 * Five frames and then it is gone: every step takes the form on by one and
 * redraws, and at form 5 it registers its shapes one last time and hides
 * itself with bit 13 of +8. Only **form 2** does any damage, so the blast
 * reaches out exactly once however long the animation runs.
 *
 * At form 2 it takes everything within 0x14 across and 0x18 down and deals
 * with it by what the thing is. Bit 12 of +6 says a thing can be thrown:
 * kind 4 is simply switched on, kind 0x13 - a balloon - is burst, and anything
 * else is given a speed away from the blast, `blast_speed_for_mass` by its
 * weight and `angle_between_centres` for the direction.
 *
 * Everything else is looked up in a four-entry table of kinds - 1, 6, 0x0f and
 * 0x30 - and a kind not in it is left alone. The table and the four handler
 * offsets sit in the code segment and are reached by a computed `jmp`, which
 * is why nothing static finds them.
 */
void part_step_blast(struct part *part)
{
    struct part *si;
    int16_t  v02;                       /* [bp-2] the speed */
    uint16_t v04;                       /* [bp-4] the angle away */

    if (part->form == 5) {
        mark_part_shapes(part, 3);
        part->state |= STATE_GONE;
    } else {
        part->form++;
        place_object_for_draw(part);
    }

    if (part->form == 2) {
        link_nearby_objects(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST), -0x14, 0x14, -0x18, 0x18);

        for (si = part->next_linked; si != NULL;
             si = si->next_linked) {
            if (si->traits & TRAIT_IN_MOVING_LIST) {
                if (si->kind == KIND_BALLOON) {
                    si->direction = 1;
                } else if (si->kind == KIND_DYNAMITE) {
                    burst_dynamite(si);
                } else {
                    v02 = blast_speed_for_mass(si);
                    v04 = angle_between_centres(part, si);
                    set_vector_from_angle(si, v04, v02);
                }
            } else {
                switch (si->kind) {
                case KIND_BOB_THE_FISH:
                    break_bob_the_fish(si);
                    break;
                case KIND_MOUSE_CAGE:
                    trigger_mouse_cage(si);
                    break;
                case 0x01:
                case 0x30:
                    split_part_at(si, part);
                    break;
                }
            }
        }
    }
}

/*
 * 190f:1738, image 0x1a828 (1.00's; not yet placed in 1.11)
 *
 * How fast the blast throws a thing: a ladder on the weight its kind's record
 * keeps at +2 - the same word `step_machine` copies into every object's +0x3a
 * - from 0x1800 for anything under 2 down to 0x800 for 0x709 and over. Eight
 * thresholds, so the lightest things fly eight times as fast as they would if
 * the speed were flat.
 */
int16_t blast_speed_for_mass(struct part *part)
{
    int16_t m;                          /* si */
    int16_t r;                          /* cx */

    m = g_part_kinds[part->kind].weight;

    if (m < 2)
        r = 0x1800;
    else if (m < 6)
        r = 0x1600;
    else if (m < 0x0a)
        r = 0x1400;
    else if (m < 0x15)
        r = 0x1200;
    else if (m < 0x79)
        r = 0x1000;
    else if (m < 0x97)
        r = 0x0e00;
    else if (m < 0xc9)
        r = 0x0c00;
    else if (m < 0x709)
        r = 0x0a00;
    else
        r = 0x0800;

    return r;
}

/*
 * 190f:17be, image 0x1a8ae - the blast tearing a kind 1 or kind 0x30 in two.
 *
 * The bite is a **gap on the sixteen-pixel grid**, not a circle round the
 * blast: two grid lines are worked out from the blast's middle, one 0x20 back
 * and one 0x18 on, each rounded down to a multiple of sixteen and then given
 * eight - so the cut lands where the game's own grid is and the two halves
 * still line up with everything else.
 *
 * The longer axis is the one cut, and there are four cases, which are the four
 * ways a bar can lie across a gap:
 *
 * - **across both lines** - the part spans the gap, so it is cut in two:
 *   `clone_part` makes the far half, `insert_sorted` puts it on the list at
 *   DGROUP 0x521b, and each half is shortened to its own side. A clone that
 *   cannot be had leaves the part whole, which is the out-of-memory case
 *   costing the cut and not the machine.
 * - **starting before the gap and ending inside it** - shortened to the near
 *   line.
 * - **starting inside and ending past the far line** - moved to the far line
 *   and shortened by as much.
 * - **wholly inside the gap** - it is gone: bit 13 of +8 hides it.
 *
 * Every half that survives goes back through the setup at 172c:48ab, which
 * rebuilds its four corners from the extent it now has.
 */
void split_part_at(struct part *part, struct part *blast)
{
    struct part *di;
    int16_t  v02;                       /* [bp-2] the blast's middle, across */
    int16_t  v04;                       /* [bp-4] the near line, across */
    int16_t  v06;                       /* [bp-6] the far line, across */
    int16_t  v08;                       /* [bp-8] the blast's middle, down */
    int16_t  v0a;                       /* [bp-0xa] the near line, down */
    int16_t  v0c;                       /* [bp-0xc] the far line, down */

    mark_part_shapes(part, 3);

    v02 = blast->pos[0].x + (blast->size[0].width >> 1);
    v08 = blast->pos[0].y + (blast->size[0].height >> 1);

    if (part->size[0].width > part->size[0].height) {
        v04 = ((v02 - 0x20) & 0xfff0) + 8;
        v06 = ((v02 + 0x18) & 0xfff0) + 8;

        if (part->pos[0].x < v04) {
            if (part->pos[0].x + part->size[0].width > v06) {
                if ((di = clone_part(part)) == NULL)
                    goto out;

                insert_sorted(di, &g_placed_parts);
                di->traits |= TRAIT_SPAWNED;

                di->size[0].width = part->pos[0].x + part->size[0].width - v06;
                di->box[0].x = di->pos[0].x = v06;
                di->box[0].y = di->pos[0].y = part->pos[0].y;

                part->size[0].width = v04 - part->pos[0].x;

                part_setup_platform(di);
            } else if (part->pos[0].x + part->size[0].width > v04) {
                part->size[0].width = v04 - part->pos[0].x;
            }

            part_setup_platform(part);
        } else if (part->pos[0].x + part->size[0].width > v06) {
            if (part->pos[0].x < v06) {
                part->size[0].width = part->pos[0].x + part->size[0].width - v06;
                part->box[0].x = part->pos[0].x = v06;
                part_setup_platform(part);
            }
        } else if (part->pos[0].x < v06
                   && part->pos[0].x + part->size[0].width > v04) {
            part->state |= STATE_GONE;
        }
    } else {
        v0a = ((v08 - 0x20) & 0xfff0) + 8;
        v0c = ((v08 + 0x18) & 0xfff0) + 8;

        if (part->pos[0].y < v0a) {
            if (part->pos[0].y + part->size[0].height > v0c) {
                if ((di = clone_part(part)) == NULL)
                    goto out;

                insert_sorted(di, &g_placed_parts);
                di->traits |= TRAIT_SPAWNED;

                di->size[0].height = part->pos[0].y + part->size[0].height - v0c;
                di->box[0].x = di->pos[0].x = part->pos[0].x;
                di->box[0].y = di->pos[0].y = v0c;

                part->size[0].height = v0a - part->pos[0].y;

                part_setup_platform(di);
            } else if (part->pos[0].y + part->size[0].height > v0a) {
                part->size[0].height = v0a - part->pos[0].y;
            }

            part_setup_platform(part);
        } else if (part->pos[0].y + part->size[0].height > v0c) {
            if (part->pos[0].y < v0c) {
                part->size[0].height = part->pos[0].y + part->size[0].height - v0c;
                part->box[0].y = part->pos[0].y = v0c;
                part_setup_platform(part);
            }
        } else if (part->pos[0].y < v0c
                   && part->pos[0].y + part->size[0].height > v0a) {
            part->state |= STATE_GONE;
        }
    }

out:
    ;
}
