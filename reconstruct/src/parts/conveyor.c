/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The conveyor belt**: its setup, hit, step and settle.
 *
 * The twenty-fourth module of the original's **code segment 172c**, image
 * 0x19790..0x19942 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3330..0x3335
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3330..0x3335. The grab x by width step: 9 23 38 44 59. `part_settle_conveyor`.
 * Five bytes; the next module's data starts at the next word, 0x3336.
 */
uint8_t CONVEYOR_GRAB_X[5] = { 9, 23, 38, 44, 59 };

/*
 * 172c:24d0, image 0x19790 - a setup.
 *
 * The part's own bounding rectangle: (0,0), (W,0), (W,H), (0,H), with W and H
 * read from +0x44 and +0x46 rather than written as constants. The first
 * corner is stored **y before x**, because both come from the same zeroed AL.
 */
void part_setup_conveyor(struct part *part)
{
    struct part_point *si;

    si = part->points;
    si->x = si->y = 0;
    si++;
    si->x = part->size[0].width;
    si->y = 0;
    si++;
    si->x = part->size[0].width;
    si->y = part->size[0].height;
    si++;
    si->x = 0;
    si->y = part->size[0].height;

    part_finish_angles(part);
}

/*
 * 172c:2514, image 0x197d4 - kind 5's hit test.
 *
 * A crank being turned pushes whatever is standing on it sideways at 0x1000,
 * building up to that speed rather than snapping to it: the speed is added and
 * then clamped, so a thing already going faster is left alone.
 *
 * Which way depends on the direction the thing hit carries at its +0x12 and on
 * the crank's own +0x8a: at 0 a positive direction pushes right, at 2 the two
 * are the other way round, and at anything else nothing happens at all. It
 * always answers 1 - the hit counts either way.
 */
uint16_t part_hit_conveyor(struct part *part)
{
    int16_t dir;                        /* cx */
    int16_t v;                          /* dx */
    struct part *other;                 /* di */

    other = part->contact;
    dir = other->direction;

    if (part->contact_edge == 0) {
        v = 0x1000;
        if (dir > 0) {
            if ((part->vel_x += v) > v)
                part->vel_x = v;
        } else if (dir < 0) {
            if ((part->vel_x -= v) < v)
                part->vel_x = 0 - v;
        }
    } else if (part->contact_edge == 2) {
        v = 0x1000;
        if (dir < 0) {
            if ((part->vel_x += v) > v)
                part->vel_x = v;
        } else if (dir > 0) {
            if ((part->vel_x -= v) < v)
                part->vel_x = 0 - v;
        }
    }

    return 1;
}

/*
 * 172c:2592, image 0x19852 - kind 5's step.
 *
 * A crank. Its direction at +0x12 turns the handle round seven positions, up
 * or down, and the wrap is written as a remainder rather than a compare: one
 * past a multiple of seven goes back six, and a multiple of seven goes forward
 * six. So the seven frames cycle in either direction without a table.
 *
 * A crank whose rope reaches a gear that is not turning - kind 0x0e with its
 * last two forms equal - gives up before any of that: nothing is on the other
 * end to turn.
 *
 * Turning sets DGROUP 0x52d3 to 2, and the first frame of a turn plays sound 1.
 */
void part_step_conveyor(struct part *part)
{
    struct part *di;

    if (part->direction != 0 && (di = rope_other_end(part)) != NULL
        && di->kind == KIND_GEAR && di->form_prev == di->form_prev2)
        part->direction = 0;

    if (part->direction != 0) {
        g_sound_request_01 = 2;

        if (part->form == part->form_prev)
            play_sound(1);

        if (part->direction > 0) {
            if ((part->form + 1) % 7 == 0)
                part->form -= 6;
            else
                part->form++;
        } else if (part->direction < 0) {
            if (part->form % 7 == 0)
                part->form += 6;
            else
                part->form--;
        }
    }
}

/*
 * 172c:261d, image 0x198dd - kind 5's settle, the +0x0ed8 slot, which
 * `game_screen_loop` calls once a drag has finished.
 *
 * The size being dragged lives at +0x50 and +0x52 and the real size at +0x44
 * and +0x46; settling copies the first pair into the second. Then the low byte
 * of the new width is written into two of the connection points - the one at
 * +0x82 plus 4 and the one after it - so the part's ends move out with it.
 *
 * The form is `(width - 0x20) / 0x10 * 7`, and the byte at +0x56 comes from a
 * table at DGROUP 0x3330 indexed by the same `(width - 0x20) / 0x10`. The
 * divide is `idiv` on 16 bits, which truncates toward zero as C does.
 */
void part_settle_conveyor(struct part *part)
{
    struct part_point *di;              /* point 1 */
    struct part_point *p2;              /* [bp-2] point 2 */

    part->size[0].width = part->set_size.width;
    part->size[0].height = part->set_size.height;

    di = part->points + 1;
    p2 = di + 1;
    di->x = p2->x = part->size[0].width;

    part->start_form = part->form = (part->size[0].width - 0x20) / 0x10 * 7;

    /* `mov al,[bx+0x3330]`: the grab x by width step. */
    part->grab.x = CONVEYOR_GRAB_X[(part->size[0].width - 0x20) / 0x10];
}
