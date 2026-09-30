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
 * JUDGE: built-with -mm -zC_TEXT -O -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x00e89
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
 *     is told so, and each that has not still copies its ropes' positions
 *     forward.
 */
void step_machine(void)
{
    uint16_t flags;                     /* [bp-2] */
    int16_t rope;                       /* [bp-4] */
    struct rope *b;                     /* [bp-6] */
    register struct part *si;
    register struct queue_node *di;

    for (si = g_placed_parts.next; si != NULL;
         si = si->next)
        si->state &= ~(STATE_STEPPED | STATE_HELD | STATE_TURNS_FREE);

    for (di = g_parts_queue; di != 0; di = di->next) {
        si = (di->part);
        if (!(si->state & STATE_STEPPED))
            part_step(si);
    }

    release_part_queue();

    for (si = g_placed_parts.next; si != NULL;
         si = si->next) {
        flags = si->state;
        if (flags & STATE_SELF_DRIVEN && !(flags & (STATE_GONE | STATE_STEPPED)))
            part_step(si);
    }

    for (si = g_placed_parts.next; si != NULL;
         si = si->next)
        if (si->kind == KIND_GEAR && !(si->state & (STATE_STEPPED | STATE_GONE)))
            part_step(si);

    for (si = g_placed_parts.next; si != NULL;
         si = si->next) {
        flags = si->state;
        if (!(flags & (STATE_GONE | STATE_SELF_DRIVEN | STATE_STEPPED)))
            part_step(si);
    }

    for (si = g_moving_parts.next; si != NULL;
         si = si->next) {
        if (!(si->state & STATE_GONE))
            apply_gravity_and_speed(si);
        si->weight = g_part_kinds[si->kind].weight;
        si->traits2 &= ~TRAIT2_IN_BUCKET;
    }

    for (si = g_moving_parts.next; si != NULL;
         si = si->next)
        if (si->kind != KIND_BUCKET)
            step_moving_object(si);

    for (si = g_moving_parts.next; si != NULL;
         si = si->next)
        if (si->kind == KIND_BUCKET) {
            collect_carried(si);
            add_carried_weight(si);
        }

    for (si = g_moving_parts.next; si != NULL;
         si = si->next)
        if (si->kind == KIND_BUCKET) {
            collect_carried(si);
            step_moving_object(si);
        }

    for (si = g_moving_parts.next; si != NULL;
         si = si->next)
        if (si->kind == KIND_BUCKET) {
            collect_carried(si);
            carry_riders_along(si);
        }

    for (si = g_moving_parts.next; si != NULL;
         si = si->next) {
        if (!(si->traits & TRAIT_CONTACT_DONE) && !(si->state & STATE_GONE)) {
            if (si->traits & TRAIT_HIT_FIXED) {
                if (part_hit(si->contact->kind, si)) {
                    if (si->traits & TRAIT_ON_SURFACE)
                        apply_contact_friction(si);
                    else
                        bounce_off_contact(si);
                }
            } else if (si->traits & TRAIT_HIT_MOVING) {
                if (part_hit(si->contact->kind, si))
                    bounce_pair(si);
            }
        }
    }

    for (si = pick_by_flag((TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST)); si != NULL;
         si = pick_for_record(si, TRAIT_IN_MOVING_LIST)) {
        if (!(si->state & STATE_GONE)) {
            if (si->pos[0].x != si->pos[2].x || si->pos[0].y != si->pos[2].y
                || si->form != si->form_prev2)
                part_moved(si);
            else if (!(si->pos[0].x == si->pos[1].x
                       && si->pos[0].y == si->pos[1].y
                       && si->form == si->form_prev))
                for (rope = 0; rope < 2; rope++)
                    if ((b = si->rope[rope]) != NULL) {
                        b->pt[0][0] = b->pt[2][0];
                        b->pt[0][1] = b->pt[2][1];
                    }
        }
    }
}

/*
 * 0x0118c
 *
 * One moving object's step: run its kind's own handler, integrate it, clear the
 * low nibble of its contact flags at +6, and settle it against whatever it hits.
 *
 * Then, if it hangs from a rope, `tension_rope` is asked whether that pulled it
 * somewhere. If it did, the contact flags are cleared again - the position it
 * was settled at is no longer where it is. If it did not, the contact record at
 * +0x84 is *saved and cleared* across a second `resolve_collisions`, and put
 * back only if that second pass found nothing: a part that the rope did not
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
    struct part *saved;                    /* [bp-6] */
#ifdef __TURBOC__
    struct part_contact *c;
#else
    /* Ours: the original reads `c` below on a path that never set it, and
       gets away with it because the test after it fails anyway; the host
       cannot read an unset pointer. */
    struct part_contact *c = (struct part_contact *)&obj->contact;
#endif

    if (!(obj->state & STATE_GONE)) {
        part_step(obj);
        integrate_object(obj);
        obj->traits &= ~(TRAIT_ON_SURFACE | TRAIT_HIT_FIXED | TRAIT_HIT_MOVING | TRAIT_CONTACT_DONE);
        resolve_collisions(obj);
        if (obj->rope[0] != 0) {
            pulled = tension_rope(obj);
            if (pulled != 0)
                obj->traits &= ~(TRAIT_ON_SURFACE | TRAIT_HIT_FIXED | TRAIT_HIT_MOVING | TRAIT_CONTACT_DONE);
            else {
                c = (struct part_contact *)&obj->contact;
                saved = c->part;
                plus = c->no_nudge_plus;
                minus = c->no_nudge_minus;
                c->part = 0;
            }
            resolve_collisions(obj);
            if (c->part == 0 && pulled == 0) {
                c->part = saved;
                c->no_nudge_plus = plus;
                c->no_nudge_minus = minus;
            }
        }
    }
}
