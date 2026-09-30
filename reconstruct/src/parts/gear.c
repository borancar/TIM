/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The gear**: its hit, setup and step, the four nudges it gives, and the signal it spreads and settles.
 *
 * The twenty-second module of the original's **code segment 172c**, image
 * 0x191c8..0x1956e - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 172c:1f08, image 0x191c8 - shove an object along x.
 *
 * **The name is ours; the original has none.** +0x36 and +0x38 are the
 * velocity pair, read that way from `part_step_bellow`, which adds a bellows'
 * push to +0x36. Only `part_hit_gear` below calls these four.
 *
 * Add d and cap at +d: whatever the object was doing, it ends up going no
 * faster than d in that direction.
 */
void nudge_x_add(struct part *obj, int16_t d)
{
    if ((obj->vel_x += d) > d)
        obj->vel_x = d;
}

/*
 * 172c:1f22, image 0x191e2 - the same along -x, and **not the mirror of it**.
 *
 * The subtraction is the obvious half. The clamp then compares against **+d
 * again, not -d**, so an object left slower than d after the subtraction is
 * slammed to exactly -d, and only one already moving faster than 2d keeps what
 * the subtraction gave it. That is what 0x191f2 compares and it is transcribed
 * as the asymmetry it is rather than tidied into a matching pair.
 */
void nudge_x_sub(struct part *obj, int16_t d)
{
    if ((obj->vel_x -= d) < d)
        obj->vel_x = 0 - d;
}

/*
 * 172c:1f40, image 0x19200 - `nudge_x_add` on +0x38 instead of +0x36. Ours.
 */
void nudge_y_add(struct part *obj, int16_t d)
{
    if ((obj->vel_y += d) > d)
        obj->vel_y = d;
}

/*
 * 172c:1f5a, image 0x1921a - `nudge_x_sub` on +0x38, asymmetry and all. Ours.
 */
void nudge_y_sub(struct part *obj, int16_t d)
{
    if ((obj->vel_y -= d) < d)
        obj->vel_y = 0 - d;
}

/*
 * 172c:1f78, image 0x19238 - kind 14's hit test. **Something has landed on a
 * moving surface and is carried along it.**
 *
 * The argument is the object that arrived; +0x84 is the kind-14 part it hit.
 * Which way that part is running is the difference between its form at +0x0c
 * and the form last drawn at +0x0e, **and a difference bigger than one means
 * the counter wrapped, so the sign is flipped**: 0x1925d and 0x19266 turn
 * anything above 1 into -1 and anything below -1 into 1. A part that is not
 * moving does nothing.
 *
 * A balloon - kind 4 - is not carried. It gets +0x12 set instead, which is
 * whatever a balloon does when something touches it, and the answer is 1 the
 * same as every other path.
 *
 * Otherwise the push is along the struck face at +0x8a, or the opposite face
 * when the surface runs backwards, which is `(face + 4) & 7` - eight compass
 * points, and adding four is half a turn. The four square directions get the
 * whole 0x1000 and the four diagonals get half of it each, which is the
 * original's approximation to a diagonal rather than anything trigonometric.
 *
 * The `ja` past seven is unreachable after the mask and is transcribed anyway.
 *
 * **Kind 14 is a moving surface and reads like the conveyor belt** - a form
 * counter that steps and a face that says which way it carries - but that is a
 * reading of this routine, not a name taken from anywhere that says so.
 */
uint16_t part_hit_gear(struct part *part)
{
    int16_t  half;                      /* di */
    int16_t  full;                      /* [bp-2] */
    int16_t  dir;                       /* [bp-4] */
    int16_t  face;                      /* [bp-6] */
    struct part *other;                 /* [bp-8] */

    other = part->contact;
    dir = other->form - other->form_prev;

    if (dir > 1)
        dir = -1;
    else if (dir < -1)
        dir = 1;

    if (dir != 0) {
        if (part->kind == KIND_BALLOON) {
            part->direction = 1;
            return 1;
        }

        face = (dir > 0) ? part->contact_edge : (part->contact_edge + 4) & 7;
        full = 0x1000;
        half = full >> 1;

        switch (face) {
        case 0: nudge_x_add(part, full);                          break;
        case 1: nudge_x_add(part, half); nudge_y_add(part, half); break;
        case 2:                          nudge_y_add(part, full); break;
        case 3: nudge_x_sub(part, half); nudge_y_add(part, half); break;
        case 4: nudge_x_sub(part, full);                          break;
        case 5: nudge_x_sub(part, half); nudge_y_sub(part, half); break;
        case 6:                          nudge_y_sub(part, full); break;
        case 7: nudge_x_add(part, half); nudge_y_sub(part, half); break;
        }
    }

    return 1;
}

/*
 * 172c:2068, image 0x19328 - the only setup that looks at the rest of the
 * machine. It runs 172c:0001 for the slots, clears its own four links at
 * +0x5a, and then walks the list at DGROUP 0x521b for other parts of its
 * own kind, 0x0e, sitting exactly 0x20 away in one axis and level in the
 * other. Each one found goes in the link for the direction it lies in -
 * right, left, down, up - so a run of them ends up knowing its neighbours.
 */
void part_setup_gear(struct part *part)
{
    struct part *di;
    int16_t dx;                         /* [bp-2] */
    int16_t dy;                         /* [bp-4] */
    int16_t i;                          /* [bp-6] */

    part_setup_big_ball(part);

    for (i = 0; i < 4; i++)
        part->link[i] = 0;

    for (di = g_placed_parts.next; di != NULL;
         di = di->next) {
        if (di != part && di->kind == KIND_GEAR) {
            dx = part->start_x - di->start_x;
            dy = part->start_y - di->start_y;

            if (dy == 0) {
                if (dx == 0x20)
                    part->link[0] = di;
                else if (dx == -0x20)
                    part->link[1] = di;
            } else if (dx == 0) {
                if (dy == 0x20)
                    part->link[2] = di;
                else if (dy == -0x20)
                    part->link[3] = di;
            }
        }
    }
}

/*
 * 172c:20fc, image 0x193bc - kind 14's step, and the two routines below it.
 *
 * Kind 14 is a gear. A gear that has been given a direction at +0x12 marks
 * itself done - bit 6 of +8 - and pushes that direction out along its first
 * four links; `spread_gear_signal` follows the chain and answers 1 if it ever
 * found a gear already turning the wrong way. A chain that disagrees with
 * itself is jammed, so the gear's own direction is thrown away, and either way
 * `settle_gear_signal` walks the chain again to turn every gear on it.
 */
void part_step_gear(struct part *part)
{
    int16_t di;                         /* the signal back */
    int16_t v02;                        /* [bp-2] */
    struct part *v04;                   /* [bp-4] */

    if (part->direction != 0) {
        part->state |= STATE_STEPPED;

        di = 0;
        for (v02 = 0; v02 < 4; v02++)
            if ((v04 = part->link[v02]) != NULL)
                di = spread_gear_signal(part, v04, 2, di);

        if (di != 0)
            part->direction = 0;

        settle_gear_signal(part, di);
    }
}

/*
 * 172c:105d, image 0x1941d
 *
 * Push one gear's direction on to the next, and answer whether the chain
 * disagrees with itself.
 *
 * `how` says how the two are joined: 1 is a belt, which carries the direction
 * unchanged, and 2 is a mesh, which reverses it. A gear that is not turning yet
 * takes the direction; one that is already turning is checked against it, and
 * a mismatch - the same direction through a mesh, or a different one through a
 * belt - is the jam this answers 1 for.
 *
 * From a gear, kind 0x0e, it goes on to that gear's own four links and its
 * belt, marking each as it goes so a ring of gears is walked once. `flag` is
 * carried through and comes back, so one answer covers the whole chain.
 */
uint16_t spread_gear_signal(struct part *from, struct part *to, int16_t how,
                            uint16_t flag)
{
    int16_t  v02;                       /* [bp-2] */
    int16_t  v04;                       /* [bp-4] how it is joined */
    struct part *v06;                   /* [bp-6] the next gear */

    if (to->direction != 0) {
        if (how == 1 && to->direction != from->direction)
            flag = 1;
        else if (how == 2 && to->direction == from->direction)
            flag = 1;
    } else {
        to->direction = (how == 1) ? from->direction : 0 - from->direction;
    }

    if (to->kind == KIND_GEAR && !(to->state & STATE_STEPPED)) {
        to->state |= STATE_STEPPED;

        for (v02 = 0; v02 < 5; v02++) {
            if (v02 == 4) {
                v06 = belt_other_end(to);
                v04 = 1;
            } else {
                v06 = to->link[v02];
                v04 = 2;
            }

            if (v06 != NULL && !(v06->state & STATE_SELF_DRIVEN))
                flag = spread_gear_signal(to, v06, v04, flag);
        }
    }

    return flag;
}

/*
 * 172c:1225, image 0x194e5
 *
 * Turn a chain of gears by one step. Each one's direction at +0x12 is added to
 * its form at +0x0c, which wraps round the four positions, and the direction is
 * then cleared so it has to be given again next step.
 *
 * The walk is the same five links `spread_gear_signal` uses, and with `clear`
 * set every gear reached has its direction thrown away first - which is how a
 * jammed chain comes to a stop rather than turning.
 */
void settle_gear_signal(struct part *part, int16_t clear)
{
    struct part *di;
    int16_t v02;                        /* [bp-2] */

    part->form += part->direction;

    if (part->form == -1)
        part->form = 3;
    else if (part->form == 4)
        part->form = 0;

    part->direction = 0;

    for (v02 = 0; v02 < 5; v02++) {
        if (v02 == 4)
            di = belt_other_end(part);
        else
            di = part->link[v02];

        if (di != NULL && di->direction != 0 && !(di->state & STATE_SELF_DRIVEN)) {
            if (clear != 0)
                di->direction = 0;

            if (di->kind == KIND_GEAR)
                settle_gear_signal(di, clear);
        }
    }
}
