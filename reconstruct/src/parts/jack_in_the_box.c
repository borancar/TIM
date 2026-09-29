/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The jack-in-the-box**: its step, setup and flip, and the push it gives each kind it reaches.
 *
 * The twenty-seventh module of the original's **code segment 172c**, image
 * 0x19aa2..0x19e18 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3394..0x339a
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3394..0x339a. How far the jack-in-the-box reaches, by form: -21 -34 -59.
 * `part_step_jack_in_the_box` indexes it from -8.
 */
int16_t JACK_IN_THE_BOX_REACH[3] = { -21, -34, -59 };

/*
 * 172c:27e2, image 0x19aa2 - kind 13's step. The conveyor.
 *
 * Above form 7 it is winding down: the form just runs on to 0x12 and stops
 * there. At 7 or below and switched on it steps the form by its direction -
 * reversed by the mirror bit - and wraps 8 back to 0 and -1 back to 7, counting
 * a lap at +0x96 each time. Six laps play sound 3 and put it into form 8, which
 * is where the winding-down starts.
 *
 * From form 8 up it reaches out over the belt: a box `0x3384 + 2 * form` wide
 * and 0x1f down, and everything in it is dealt with by kind. One that can be
 * knocked along gets a speed from `conveyor_speed_for_mass`, negative sideways
 * *and* negative downwards - which is what tips a thing off the end. The rest
 * go through a jump table of six kinds, and the four that are not
 * `break_bob_the_fish` or `trigger_mouse_cage` are each given the middle of the conveyor
 * to compare themselves against.
 */
void part_step_jack_in_the_box(struct part *part)
{
    struct part *di;
    int16_t dir;                        /* [bp-2] */
    int16_t push;                       /* [bp-4] */
    int16_t mid;                        /* [bp-6] */

    if (part->form > 7) {
        if (part->form != 0x12)
            part->form++;
    } else if (part->direction != 0) {
        dir = (part->flags_08 & 0x10) ? 0 - part->direction : part->direction;
        part->form += dir;

        if (part->form == 8) {
            part->form = 0;
            part->kind_state++;
        } else if (part->form == -1) {
            part->form = 7;
            part->kind_state++;
        }

        if (part->kind_state == 6) {
            play_sound(3);
            part->form = 8;
        }
    }

    if (part->form >= 8 && part->form <= 0x0a) {
        mid = part->pos[0].x + (part->size[0].width >> 1);

        /* The reach by form, 0x3394, with the first form folded into the
           address: `[bx+0x3384]`. */
        link_objects_in_range(part, 0x3000, 0, 0x1f,
                              (JACK_IN_THE_BOX_REACH - 8)[part->form], 0);

        for (di = part->next_linked; di != NULL;
             di = di->next_linked) {
            if (di->flags_06 & 0x1000) {
                push = conveyor_speed_for_mass(di);
                di->vel_x = (part->flags_08 & 0x10) ? push : 0 - push;
                di->vel_y = 0 - push;
            } else {
                switch (di->kind) {
                case 0x0f: break_bob_the_fish(di);     break;
                case 0x06: trigger_mouse_cage(di);     break;
                case 0x03: conveyor_nudge_3(di, mid);  break;
                case 0x10: conveyor_nudge_10(di, mid); break;
                case 0x15: conveyor_nudge_15(di, mid); break;
                case 0x25: conveyor_nudge_25(di, mid); break;
                }
            }
        }
    }

    if (part->form != part->form_prev)
        place_object_for_draw(part);
}

/*
 * 172c:295d, image 0x19c1d - a setup.
 *
 * A plain 32 by 32 box. The first corner is stored y before x out of a zeroed
 * AL, the way the computed ones do it, and the other three are constants.
 */
void part_setup_jack_in_the_box(struct part *part)
{
    struct part_point *si;

    si = part->points;
    si->x = si->y = 0;
    si++;
    si->x = 31;
    si->y = 0;
    si++;
    si->x = 31;
    si->y = 31;
    si++;
    si->x = 0;
    si->y = 31;

    part_finish_angles(part);
}

/*
 * 172c:2999, image 0x19c59 - kind 13's flip, and **it calls no setup at all**.
 * Bit 4 goes over and the part is redrawn; its connection points do not move,
 * so there is nothing to rebuild.
 */
void part_flip_jack_in_the_box(struct part *part)
{
    part->flags_08 ^= 0x10;
    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:29c6, image 0x19c86
 *
 * How fast the conveyor throws a thing, by its mass: nine steps from 0x1800
 * for the lightest down to 0x800 for the heaviest. The third of these ladders
 * in the module, and the slowest of them.
 */
int16_t conveyor_speed_for_mass(struct part *obj)
{
    int16_t m;                          /* si */
    int16_t r;                          /* cx */

    m = PART_KINDS[obj->kind].weight;

    if (m < 0x0002)
        r = 0x1800;
    else if (m < 0x0006)
        r = 0x1600;
    else if (m < 0x000a)
        r = 0x1400;
    else if (m < 0x0015)
        r = 0x1200;
    else if (m < 0x0079)
        r = 0x1000;
    else if (m < 0x0097)
        r = 0x0e00;
    else if (m < 0x00c9)
        r = 0x0c00;
    else if (m < 0x0709)
        r = 0x0a00;
    else
        r = 0x0800;

    return r;
}

/*
 * 172c:2a3a, image 0x19cfa - a kind-3 motor on the conveyor.
 *
 * Which way the conveyor turns it depends on the form it is in and on which
 * side of the conveyor's middle it sits: form 0 only turns one way, form 1
 * turns either, form 2 only the other.
 */
void conveyor_nudge_3(struct part *obj, int16_t mid)
{
    if (obj->form == 0) {
        if ((int16_t)(obj->pos[0].x + 0x24) > mid)
            obj->direction = 1;
    } else if (obj->form == 1) {
        if ((int16_t)(obj->pos[0].x + 0x28) > mid)
            obj->direction = 1;
        else
            obj->direction = 0xffff;
    } else if (obj->form == 2) {
        if ((int16_t)(obj->pos[0].x + 0x2c) < mid)
            obj->direction = 0xffff;
    }
}

/*
 * 172c:2a91, image 0x19d51 - a kind-0x10 on the conveyor.
 *
 * Only in form 0, and the offset it measures from and the direction of the
 * comparison both come from its mirror bit.
 */
void conveyor_nudge_10(struct part *obj, int16_t mid)
{
    if (obj->form == 0) {
        if (obj->flags_08 & 0x10) {
            if (obj->pos[0].x + 0x0c < mid)
                obj->direction = 1;
        } else {
            if (obj->pos[0].x + 0x2c > mid)
                obj->direction = 1;
        }
    }
}

/*
 * 172c:2acb, image 0x19d8b - a kind-0x15 see-saw on the conveyor.
 *
 * A see-saw already tipped one way and sitting between two and twenty pixels
 * of the conveyor's middle is tipped back - four off the form, its own setup
 * run again, and sound 0x11 - and then told whether it is at rest by comparing
 * the form with the one at +0x90.
 */
void conveyor_nudge_15(struct part *obj, int16_t mid)
{
    if (obj->form >= 4 && obj->pos[0].x - 2 < mid && obj->pos[0].x + 0x14 > mid) {
        obj->form -= 4;
        part_setup_electric_plug(obj);
        play_sound(0x11);

        if (obj->form != obj->start_form)
            obj->direction = 1;
        else
            obj->direction = 0;
    }
}

/*
 * 172c:2b1e, image 0x19dde - a kind-0x25 on the conveyor.
 *
 * The same shape as the kind-0x10 nudge with different offsets: 0x12 mirrored
 * and 0x18 not.
 */
void conveyor_nudge_25(struct part *obj, int16_t mid)
{
    if (obj->form == 0) {
        if (obj->flags_08 & 0x10) {
            if (obj->pos[0].x + 0x12 < mid)
                obj->direction = 1;
        } else {
            if (obj->pos[0].x + 0x18 > mid)
                obj->direction = 1;
        }
    }
}
