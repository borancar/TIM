/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **One step of the machine**: `step_machine`'s passes over the part lists,
 * and the per-part move-and-collide it hands each moving part to.
 *
 * A module of the original's **code segment 0000** (`_TEXT`), image
 * 0x00f86..0x012ab, split out of machine.c on 2026-09-27. Its end is proven:
 * `run_machine_loop` (0x012ab) reaches `step_machine` through TLINK's
 * `nop / push cs / call`. Its start is proven only to within one routine:
 * `step_moving_object` reaches collide.c's `resolve_collisions` the same way,
 * so a module ends before 0x01216, and `step_machine` has no call or data
 * of its own to say which side it is on. It is kept with the routine it
 * calls. Neither has data of its own.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x00f86
 *
 * One step of the machine's physics, as a dozen passes over the same lists.
 *
 * The order matters and is the whole point: a pass finishes for every part
 * before the next begins, so a part never sees half of another part's step.
 *
 *  1. Clear bits 6 to 9 of the flags at +8 on everything - last step's answers.
 *  2. Run the step of every part on the list at DGROUP 0x4e58, which is the
 *     queue of parts something asked to move, then fold that list onto 0x4e56.
 *  3. Run it again for the parts on 0x521b with bit 11 set and neither bit 6
 *     nor bit 13, then for kind 0x0e, then for everything with none of bits 6,
 *     11 or 13. Three passes in a fixed order, so a conveyor moves before the
 *     things standing on it.
 *  4. Over the list at 0x5179 - the moving objects - apply gravity, reset the
 *     mass from the kind's record, and clear bit 4 of +0x0a.
 *  5. Four more passes over 0x5179 around kind 0x11, each pairing 0x03972 with
 *     a different follow-up.
 *  6. Collisions: an object with bit 1 of +6 asks the kind of whatever it is
 *     touching - the part at +0x84 - whether the hit counts, and answers by
 *     bouncing or sliding; bit 2 asks the same question and takes a third
 *     answer. Bit 3 or being hidden skips it.
 *  7. Finally, over the 0x3000 list, each part that has moved since last step
 *     is told so, and each that has not still copies its belts' positions
 *     forward.
 */
void step_machine(void)
{
    uint16_t flags;                     /* [bp-2] */
    int16_t belt;                       /* [bp-4] */
    struct belt *b;                     /* [bp-6] */
    register struct part *si;
    register uint16_t di;

    for (si = PART_PTR(DG521B.placed_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr))
        si->flags_08 &= 0xf9bf;

    for (di = DG4E4E.parts_queue_ptr; di != 0; di = QNODE_PTR(di)->next_ptr) {
        si = PART_PTR(QNODE_PTR(di)->part);
        if (!(si->flags_08 & 0x40))
            part_step(si);
    }

    splice_list_4e58_onto_4e56();

    for (si = PART_PTR(DG521B.placed_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr)) {
        flags = si->flags_08;
        if (flags & 0x800 && !(flags & 0x2040))
            part_step(si);
    }

    for (si = PART_PTR(DG521B.placed_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr))
        if (si->kind == KIND_GEAR && !(si->flags_08 & 0x2040))
            part_step(si);

    for (si = PART_PTR(DG521B.placed_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr)) {
        flags = si->flags_08;
        if (!(flags & 0x2840))
            part_step(si);
    }

    for (si = PART_PTR(DG5179.moving_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr)) {
        if (!(si->flags_08 & 0x2000))
            apply_gravity_and_speed(si);
        si->weight = PART_KINDS[si->kind].weight;
        si->flags_0a &= 0xffef;
    }

    for (si = PART_PTR(DG5179.moving_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr))
        if (si->kind != KIND_BUCKET)
            step_moving_object(si);

    for (si = PART_PTR(DG5179.moving_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr))
        if (si->kind == KIND_BUCKET) {
            collect_carried(si);
            add_carried_weight(si);
        }

    for (si = PART_PTR(DG5179.moving_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr))
        if (si->kind == KIND_BUCKET) {
            collect_carried(si);
            step_moving_object(si);
        }

    for (si = PART_PTR(DG5179.moving_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr))
        if (si->kind == KIND_BUCKET) {
            collect_carried(si);
            carry_riders_along(si);
        }

    for (si = PART_PTR(DG5179.moving_parts.next_ptr); si != PART_NONE;
         si = PART_PTR(si->next_ptr)) {
        if (!(si->flags_06 & 8) && !(si->flags_08 & 0x2000)) {
            if (si->flags_06 & 2) {
                if (part_hit(PART_PTR(si->contact_ptr)->kind, si)) {
                    if (si->flags_06 & 1)
                        apply_contact_friction(si);
                    else
                        bounce_off_contact(si);
                }
            } else if (si->flags_06 & 4) {
                if (part_hit(PART_PTR(si->contact_ptr)->kind, si))
                    bounce_pair(si);
            }
        }
    }

    for (si = pick_by_flag(0x3000); si != PART_NONE;
         si = pick_for_record(si, 0x1000)) {
        if (!(si->flags_08 & 0x2000)) {
            if (si->pos[0].x != si->pos[2].x || si->pos[0].y != si->pos[2].y
                || si->form != si->form_prev2)
                part_moved(si);
            else if (!(si->pos[0].x == si->pos[1].x
                       && si->pos[0].y == si->pos[1].y
                       && si->form == si->form_prev))
                for (belt = 0; belt < 2; belt++)
                    if ((b = BELT_PTR(si->belt_ptr[belt])) != BELT_NONE) {
                        b->pt[0][0] = b->pt[2][0];
                        b->pt[0][1] = b->pt[2][1];
                    }
        }
    }
}

/*
 * 0x01216
 *
 * One moving object's step: run its kind's own handler, integrate it, clear the
 * low nibble of its contact flags at +6, and settle it against whatever it hits.
 *
 * Then, if it hangs from a belt, `tension_belt` is asked whether that pulled it
 * somewhere. If it did, the contact flags are cleared again - the position it
 * was settled at is no longer where it is. If it did not, the contact record at
 * +0x84 is *saved and cleared* across a second `resolve_collisions`, and put
 * back only if that second pass found nothing: a part that the belt did not
 * move keeps the contact it already had, rather than losing it to a settle that
 * was only run to check.
 *
 * An object hidden - bit 13 of +8 - does none of it.
 */
void step_moving_object(register struct part *obj)
{
    int16_t pulled;                     /* [bp-2] */
    uint8_t plus;                       /* [bp-3] */
    uint8_t minus;                      /* [bp-4] */
    dg_near_t saved;                    /* [bp-6] */
#ifdef __TURBOC__
    struct part_contact *c;
#else
    /* Ours: the original reads `c` below on a path that never set it, and
       gets away with it because the test after it fails anyway; the host
       cannot read an unset pointer. */
    struct part_contact *c = (struct part_contact *)&obj->contact_ptr;
#endif

    if (!(obj->flags_08 & 0x2000)) {
        part_step(obj);
        integrate_object(obj);
        obj->flags_06 &= 0xfff0;
        resolve_collisions(obj);
        if (obj->belt_ptr[0] != 0) {
            pulled = tension_belt(obj);
            if (pulled != 0)
                obj->flags_06 &= 0xfff0;
            else {
                c = (struct part_contact *)&obj->contact_ptr;
                saved = c->ptr;
                plus = c->no_nudge_plus;
                minus = c->no_nudge_minus;
                c->ptr = 0;
            }
            resolve_collisions(obj);
            if (c->ptr == 0 && pulled == 0) {
                c->ptr = saved;
                c->no_nudge_plus = plus;
                c->no_nudge_minus = minus;
            }
        }
    }
}
