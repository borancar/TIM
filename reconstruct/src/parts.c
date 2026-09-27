#include <stdlib.h>
/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The parts**: every kind's own hooks - the setup that builds its
 * connection points, the step it takes each frame, the test for something
 * hitting it, the flip that turns it over, the settle after a drag, and the
 * drive by which one part works another.
 *
 * This file corresponds to the original's **code segment 172c**, image
 * 0x172c0..0x1c250. Functions are in address order and each carries the image
 * offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 *
 * This is where the parts live: each of the machine's fifty-odd components has
 * its own setup routine here, reached from the initialiser table in machine_draw.c
 * through a far pointer the loader relocates.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 172c:0001, image 0x172c1 - a setup.
 *
 * Eight points round a 32 by 32 part, written straight out.
 */
void part_setup_0001(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 8;
    si->y = 0;
    si++;
    si->x = 23;
    si->y = 0;
    si++;
    si->x = 31;
    si->y = 8;
    si++;
    si->x = 31;
    si->y = 23;
    si++;
    si->x = 23;
    si->y = 31;
    si++;
    si->x = 8;
    si->y = 31;
    si++;
    si->x = 0;
    si->y = 23;
    si++;
    si->x = 0;
    si->y = 8;

    part_finish_angles(part);
}

/*
 * 172c:0065, image 0x17325 - a setup.
 *
 * The same eight-cornered shape on a 23 by 23 part.
 */
void part_setup_cannon_ball(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 7;
    si->y = 0;
    si++;
    si->x = 15;
    si->y = 0;
    si++;
    si->x = 22;
    si->y = 8;
    si++;
    si->x = 22;
    si->y = 15;
    si++;
    si->x = 14;
    si->y = 22;
    si++;
    si->x = 8;
    si->y = 22;
    si++;
    si->x = 0;
    si->y = 15;
    si++;
    si->x = 0;
    si->y = 8;

    part_finish_angles(part);
}

/*
 * 172c:00c9, image 0x17389 - a setup.
 *
 * The same again, smaller still - 15 by 15.
 */
void part_setup_00c9(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 3;
    si->y = 0;
    si++;
    si->x = 11;
    si->y = 0;
    si++;
    si->x = 14;
    si->y = 4;
    si++;
    si->x = 14;
    si->y = 10;
    si++;
    si->x = 11;
    si->y = 14;
    si++;
    si->x = 3;
    si->y = 14;
    si++;
    si->x = 0;
    si->y = 10;
    si++;
    si->x = 0;
    si->y = 4;

    part_finish_angles(part);
}

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

    di = PARTSHAPES.s_3182;
    for (i = 0, si = POINTS(part->points_ptr); i < 8; i++, si++, di++) {
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
    struct part *di = PART_PTR(part->contact_ptr);

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
                && (belt = BELT_PTR(part->belt_ptr[0])) != BELT_NONE
                && (si = make_part(KIND_ANCHOR)) != PART_NONE) {
                insert_sorted(si, &DG5179.moving_parts);
                si->flags_06 |= 0x10;
                si->belt_ptr[0] = dg_near(dgroup, belt);
                si->link_ptr[0] = part->link_ptr[0];
                link = PART_PTR(si->link_ptr[0]);

                if ((k = match_field_5a_5c(part, link)) != 0xffff)
                    link->link_ptr[k] = dg_near(dgroup, si);

                if (PART_PTR(belt->end_a_ptr) == part) {
                    belt->end_a_ptr = dg_near(dgroup, si);
                    si->pos[0].x = belt->pt[0][0].x;
                    si->pos[0].y = belt->pt[0][0].y;
                } else {
                    belt->end_b_ptr = dg_near(dgroup, si);
                    si->pos[0].x = belt->pt[0][1].x;
                    si->pos[0].y = belt->pt[0][1].y;
                }

                si->fx = si->pos[0].x;
                si->fx <<= 9;
                si->fy = si->pos[0].y;
                si->fy <<= 9;

                place_object_for_draw(si);

                part->belt_ptr[0] = 0;
                part->link_ptr[0] = 0;
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
    struct belt *belt = BELT_PTR(p2->belt_ptr[0]);   /* [bp-2] */

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

/*
 * 172c:0332, image 0x175f2 - kind 16's hit test.
 *
 * Something has touched the bellows, and this decides whether that touch
 * squeezes it. The collision record is the argument; +0x84 is the bellows
 * itself and +0x8a is which of its faces was struck.
 *
 * Bit 4 of the bellows' flags at +8 is which way round it is, and it accepts a
 * different pair of faces in each form - 1 or 3 mirrored, 0 or 4 upright. A
 * face that counts sets +0x12 to 1, which is `part_step_bellow` below squeezing.
 *
 * It answers 1 either way: the hit is a hit whether or not it worked the
 * bellows. The original's `jmp` to the next instruction at 0x1762b is the
 * compiler leaving a return path in that nothing needed.
 */
uint16_t part_hit_bellow(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);
    int16_t  face = ((int16_t)part->contact_edge);

    if ((other->flags_08 & 0x10) != 0) {
        if (face == 1 || face == 3)
            other->direction = 1;
    } else {
        if (face == 0 || face == 4)
            other->direction = 1;
    }

    return 1;
}

/*
 * 172c:0371, image 0x17631 - a setup.
 *
 * Six connection points copied out of a table chosen two ways: bit 4 of +8
 * picks between the pointer arrays at DGROUP 0x31e0 and 0x31b6, and the form
 * at +0x0c indexes the one picked. **Those two are arrays of near pointers,
 * not of points** - the load is a word - so the table this ends up walking is
 * wherever the pointer says.
 *
 * Two bytes a point at the source, four at the destination, as everywhere.
 */
void part_setup_bellow(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10)
        src = POINT_TABLE(PARTSHAPES.o_31e0[part->form]);
    else
        src = POINT_TABLE(PARTSHAPES.o_31b6[part->form]);

    for (i = 0, dst = POINTS(part->points_ptr); i < 6; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:03d2, image 0x17692 - kind 16's flip.
 *
 * Turn the part over and rebuild it: bit 4 of the flags at +8 is which way it
 * faces, and the setup at 172c:0371 reads that bit to pick which of its two
 * tables of connection points to copy. So the flip is the xor and then the
 * setup, and everything else follows from the points changing.
 */
void part_flip_bellow(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_bellow(part);

    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:0405, image 0x176c5 - kind 16's step. **The bellows.**
 *
 * +0x12 is which way it is going - 1 squeezing, -1 opening - and +0x0c is how
 * far, over three frames 0, 1, 2. Squeezing stops at 2 and opening stops at 0,
 * so both ends simply do nothing rather than wrapping.
 *
 * **Only the squeeze blows.** `link_nearby_objects` is asked for what is in a
 * box in front of the nozzle - 0x80 wide the way the part faces, ten above -
 * and bit 4 of +8 is which side that is, which is why the two arms differ only
 * in the sign of the margin and of the push. The push is 0x800, or -0x800
 * mirrored.
 *
 * Each object found is moved if bit 12 of its own flags at +6 says the air
 * reaches it. What it gets is the push scaled two ways: by `0x100 - |+0x7a|`,
 * so a thing side-on to the draught takes the full shove and one edge-on takes
 * little, and then divided by its kind's weight at +0x0 of the kind record.
 * The product is taken in 32 bits and shifted right eight before the divide -
 * that shift is the 0x100 the first scale is out of - and only the low word of
 * the quotient is kept, which is the original's own truncation and not ours.
 *
 * Two kinds are told rather than pushed. Kind 45 has whatever it was doing
 * cancelled - +0x9c, +0x12 and +0x0c all cleared - and kind 40, if the air did
 * *not* reach it, is switched on for 0x14 steps, which is `part_step_windmill`
 * below counting down. So the bellows both blows things and trips things.
 *
 * A frame that differs from the one last drawn at +0x0e is rebuilt through the
 * same setup the flip uses, and the two ends of the travel - 0 and 2 - are
 * where the sound plays.
 */
void part_step_bellow(struct part *part)
{
    struct part *di;
    int16_t push;                       /* [bp-2] */
    int16_t v;                          /* [bp-4] */
    int16_t scale;                      /* [bp-6] */
    int32_t force;                      /* [bp-0xa] */

    part->flags_08 |= 0x40;

    if (part->direction == 1) {
        if (part->form != 2) {
            part->form++;

            if (part->flags_08 & 0x10) {
                link_nearby_objects(part, 0x3000, -0x80, 0, -10, 0);
                push = (int16_t)0xf800;
            } else {
                link_nearby_objects(part, 0x3000, 0, 0x80, -10, 0);
                push = 0x0800;
            }

            for (di = PART_PTR(part->next_linked_ptr); di != PART_NONE;
                 di = PART_PTR(di->next_linked_ptr)) {
                if (di->flags_06 & 0x1000) {
                    scale = 0x100 - abs(di->link_dx);
                    force = mul16x16(push, scale);
                    force >>= 8;
                    v = force / PART_KINDS[di->kind].weight;
                    di->vel_x += v;

                    clamp_record_pair(di);

                    if (di->kind == 0x2d)
                        di->form = di->direction = di->spin = 0;
                } else if (di->kind == 0x28) {
                    di->direction = 1;
                    di->spin = 0x14;
                }
            }
        }
    } else if (part->direction == -1 && part->form != 0) {
        part->form--;
    }

    if (part->form != part->form_prev) {
        part_setup_bellow(part);

        if (part->form_prev == 0 || part->form_prev == 2)
            play_sound(0x12);

        place_object_for_draw(part);
    }
}

/*
 * 172c:0552, image 0x17812 - kind 35's hit test.
 *
 * Being hit on face 2 sets the thing that hit it going; any other face does
 * nothing. It answers 1 either way, so the hit still counts.
 */
uint16_t part_hit_boxing_glove(struct part *part)
{
    struct part *di = PART_PTR(part->contact_ptr);
    int16_t edge = part->contact_edge;          /* [bp-2] */

    if (edge == 2)
        di->direction = 1;

    return 1;
}

/*
 * 172c:057e, image 0x1783e - kind 35's step.
 *
 * A swing. While its +0x12 says go and it has not reached form 9 it steps one
 * frame - the first one plays sound 3 - and runs its own setup again, because
 * its connection points move with the swing.
 *
 * Forms 2 and 3 are where it reaches something: a box in front of it, taken
 * from one of two tables by the mirror bit and indexed by the form, and
 * everything caught in it is dealt with by kind. Bit 12 of +6 means it can be
 * knocked along, and it is given a speed by its own mass; kind 0x0f breaks;
 * kind 6 is set going.
 */
void part_step_boxing_glove(struct part *part)
{
    struct part *di;
    int16_t v;                          /* [bp-2] */

    if (part->direction != 0 && part->form != 9) {
        if (part->form == 0)
            play_sound(3);

        part->form++;
        part_setup_boxing_glove(part);
        place_object_for_draw(part);
    }

    if (part->form == 2 || part->form == 3) {
        /* Forms 2 and 3 only: the reach is `glove_reach` at 0x31e6, whose
           first two words face left and the next ones right. */
        if (part->flags_08 & 0x10)
            link_objects_in_range(part, 0x3000, 0x30,
                                  (PARTSHAPES.glove_reach + 1)[part->form], 0, 0x1f);
        else
            link_objects_in_range(part, 0x3000,
                                  (PARTSHAPES.glove_reach - 2)[part->form], 0, 0, 0x1f);

        for (di = PART_PTR(part->next_linked_ptr); di != PART_NONE;
             di = PART_PTR(di->next_linked_ptr)) {
            if (di->flags_06 & 0x1000) {
                v = bounce_speed_for_mass(di);
                di->vel_x = (part->flags_08 & 0x10) ? v : 0 - v;
            } else {
                switch (di->kind) {
                case KIND_BOB_THE_FISH:
                    break_bob_the_fish(di);
                    break;
                case KIND_MOUSE_CAGE:
                    trigger_mouse_cage(di);
                    break;
                }
            }
        }
    }
}

/*
 * 172c:065b, image 0x1791b - six slots, from one of four tables. The flag at
 * +8 bit 4 picks the pair and the form at +0x0c picks within it, so the four
 * sit as 0x31f2, 0x31fe, 0x320a, 0x3216 - twelve bytes apart, six pairs each.
 *
 * This and the setups below it share the copy at the end - N pairs, two bytes
 * into every four - and differ only in how the source table is chosen and what
 * else is set first.
 */
void part_setup_boxing_glove(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    /* Four tables: the flag at +8, and then whether the form is zero. */
    if (part->flags_08 & 0x10) {
        if (part->form == 0)
            src = PARTSHAPES.s_320a;
        else
            src = PARTSHAPES.s_3216;
    } else {
        if (part->form == 0)
            src = PARTSHAPES.s_31f2;
        else
            src = PARTSHAPES.s_31fe;
    }

    for (i = 0, dst = POINTS(part->points_ptr); i < 6; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:06c6, image 0x17986 - kind 35's flip.
 *
 * Byte for byte the same routine as `part_flip_bellow` above with a different
 * setup behind it - checked as bytes, not assumed: of the twenty flips in this
 * segment only four are this shape and the other sixteen are not, so the
 * family is real but small.
 */
void part_flip_boxing_glove(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_boxing_glove(part);

    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:06f9, image 0x179b9
 *
 * How fast a thing is thrown, by how heavy it is: the mass at DGROUP 0xea8 for
 * its kind, in seven steps from 0x1a00 for the lightest down to 0x0c00 for the
 * heaviest. Written as a ladder of compares rather than a table.
 */
int16_t bounce_speed_for_mass(struct part *obj)
{
    int16_t m;                          /* si */
    int16_t r;                          /* cx */

    m = PART_KINDS[obj->kind].weight;

    if (m < 0x0006)
        r = 0x1a00;
    else if (m < 0x000a)
        r = 0x1800;
    else if (m < 0x0015)
        r = 0x1600;
    else if (m < 0x0079)
        r = 0x1400;
    else if (m < 0x0097)
        r = 0x1200;
    else if (m < 0x00c9)
        r = 0x1000;
    else if (m < 0x0709)
        r = 0x0e00;
    else
        r = 0x0c00;

    return r;
}

/*
 * 172c:0763, image 0x17a23 - kind 17's hit test, and **it answers 0 to refuse
 * the hit**, which almost none of the others do.
 *
 * The refusal is a band: the arriving object must be coming down - +0x38
 * positive - and its centre, +0x22 plus half its width at +0x44, must lie
 * between the struck part's +0x22 plus 4 and that plus 0x1c. Inside the band
 * the answer is 0 and outside it 1, so the part is solid everywhere except
 * across a 0x1c-wide mouth that something falling can drop through.
 */
uint16_t part_hit_bucket(struct part *part)
{
    int16_t lo;                         /* cx */
    int16_t mid;                        /* di */
    int16_t hi;                         /* [bp-2] */
    struct part *other;                 /* [bp-4] */

    other = PART_PTR(part->contact_ptr);
    lo = other->pos[1].x + 4;
    hi = lo + 0x1c;
    mid = part->pos[1].x + (part->size[0].width >> 1);

    if (part->vel_y > 0 && mid > lo && mid < hi)
        return 0;

    return 1;
}

/*
 * 172c:07b2, image 0x17a72 - a setup.
 *
 * Six points, and not a regular shape: the two at y 47 sit below the four
 * that make the body, so this outline has a foot.
 */
void part_setup_bucket(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 0;
    si->y = 19;
    si++;
    si->x = 10;
    si->y = 40;
    si++;
    si->x = 25;
    si->y = 40;
    si++;
    si->x = 36;
    si->y = 20;
    si++;
    si->x = 27;
    si->y = 47;
    si++;
    si->x = 8;
    si->y = 47;

    part_finish_angles(part);
}

/*
 * 172c:0802, image 0x17ac2 - kind 17's drive hook.
 *
 * The same routine as 172c:11d2 below, and not merely alike: the 0x65 bytes at
 * the two addresses are **identical**, so the source had one function and the
 * compiler emitted it twice, once per kind that names it.
 *
 * It is transcribed twice here for the same reason it exists twice there. Each
 * address is its own entry in a kind record and its own thing to prove against
 * the original, and a shared C function would carry one provenance comment for
 * two addresses - so a verifier run naming 172c:0802 would be checking
 * something that, as far as the file is concerned, is at 172c:11d2.
 */
uint16_t part_drive_0802(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t momentum)
{
    struct belt *belt = BELT_PTR(part->belt_ptr[0]);   /* [bp-2] */

    if (flags == 1) {
        belt->v[0]++;
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

/*
 * 172c:0867, image 0x17b27 - kind 20's hit test: three kinds get three
 * different answers and everything else is simply a hit.
 *
 * A kind-4 part - a balloon - has its +0x12 set, which is what a balloon does
 * when touched. A kind-0x13 bursts, through `burst_dynamite` and given the
 * **collision record**. A kind-0x15 has one taken off its +0x36, which is a
 * nudge left. The answer is 1 whichever happened.
 */
uint16_t part_hit_0867(struct part *part)
{
    struct part *di = PART_PTR(part->contact_ptr);

    if (part->kind == KIND_BALLOON)
        part->direction = 1;
    else if (part->kind == KIND_DYNAMITE)
        burst_dynamite(part);
    else if (part->kind == KIND_ELECTRIC_PLUG)
        di->vel_x--;

    return 1;
}

/*
 * 172c:08a1, image 0x17b61 - a setup.
 *
 * Four points, from DGROUP 0x322a or 0x3222 as bit 4 of +8 says. The
 * two tables are eight bytes apart, which is those four points.
 */
void part_setup_08a1(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->flags_08 & 0x10)
        si = PARTSHAPES.s_322a;
    else
        si = PARTSHAPES.s_3222;

    for (i = 0, di = POINTS(part->points_ptr); i < 4; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:08f1, image 0x17bb1 - kind 20's step.
 *
 * Three forms and then gone. At form 2 it registers its shapes a last time and
 * hides itself with bit 13 of +8; at any form but 0 it simply steps on and
 * redraws.
 *
 * Form 0 is where it decides whether to start at all, and it decides on its
 * **sideways speed**: exactly 0x3000 or exactly 0xd000 - the same speed left
 * and right, since 0xd000 is -0x3000 - and it does nothing. Any other speed
 * starts it: the slot count at +0x80 is cleared, the form goes to 1, it is
 * redrawn and sound 0x0b plays. Two exact comparisons rather than a range, so
 * a speed one away from either starts it.
 */
void part_step_08f1(struct part *part)
{
    if (part->form == 2) {
        mark_part_shapes(part, 3);
        part->flags_08 |= 0x2000;
    } else if (part->form == 0) {
        if (part->vel_x != 0x3000 && part->vel_x != (int16_t)0xd000) {
            part->point_count = 0;
            part->form = 1;
            place_object_for_draw(part);
            play_sound(0x0b);
        }
    } else {
        part->form++;
        place_object_for_draw(part);
    }
}

/*
 * 172c:0950, image 0x17c10 - a setup.
 *
 * Three points - a triangle - and the grab box at +0x72 and +0x73 written
 * **before** them, which is the order the original uses.
 */
void part_setup_candle(struct part *part)
{
    struct part_point *si;

    part->hold.x = 0x0f;
    part->hold.y = 0x02;

    si = POINTS(part->points_ptr);
    si->x = 8;
    si->y = 31;
    si++;
    si->x = 14;
    si->y = 22;
    si++;
    si->x = 21;
    si->y = 31;

    part_finish_angles(part);
}

/*
 * 172c:098a, image 0x17c4a - kind 45's step. The paddle wheel.
 *
 * Once its +0x9c has counted past 0x14 it starts itself, and then runs four
 * frames on a loop - 5 wraps back to 1, so frame 0 is only ever the first one.
 *
 * Every step it reaches for the point at +0x72 of whatever is nearby, in a box
 * 9 to 0x12 across and ten up to five down, and switches each on. In the odd
 * frames it reaches again over the same box for two kinds by name: a kind 4 is
 * switched on, and a cat - kind 0x0c - in form 0 is woken with sound 7.
 */
void part_step_candle(struct part *part)
{
    struct part *si;

    if (part->direction == 0 && part->spin > 0x14)
        part->direction = 1;

    if (part->direction != 0) {
        if (part->form == 5)
            part->form = 1;
        else
            part->form++;

        place_object_for_draw(part);

        link_objects_at_point(part, 9, 0x12, -10, 5);

        for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
             si = PART_PTR(si->next_linked_ptr))
            if (si->direction == 0)
                si->direction = 1;

        if (part->form & 1) {
            link_objects_in_range(part, 0x1000, 9, 0x12, -10, 5);

            for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
                 si = PART_PTR(si->next_linked_ptr)) {
                if (si->kind == KIND_BALLOON) {
                    si->direction = 1;
                } else if (si->kind == KIND_POKEY && si->form == 0) {
                    si->form = 1;
                    si->word_96 = 0;
                    place_object_for_draw(si);
                    play_sound(7);
                }
            }
        }
    }
}

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

        if (part->form == 9 && (si = make_part(KIND_CANNON_BALL)) != PART_NONE) {
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
        src = PARTSHAPES.s_3242;
    } else {
        part->hold.x = 1;
        src = PARTSHAPES.s_3232;
    }

    part->hold.y = 3;

    for (i = 0, dst = POINTS(part->points_ptr); i < 8; i++, dst++, src++) {
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

/*
 * 172c:0c1c, image 0x17edc - a setup.
 *
 * Five points, on bit 4 of +8 again - 0x325c or 0x3252, ten bytes apart.
 */
void part_setup_pokey(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->flags_08 & 0x10)
        si = PARTSHAPES.s_325c;
    else
        si = PARTSHAPES.s_3252;

    for (i = 0, di = POINTS(part->points_ptr); i < 5; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:0c6c, image 0x17f2c - kind 12's hit test. Waking the cat.
 *
 * A cat in form 0 is put into form 1 with its counter cleared and mews - sound
 * 7. One already awake is left alone. It answers 1 either way.
 */
uint16_t part_hit_pokey(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);

    if (other->form == 0) {
        other->form = 1;
        other->word_96 = 0;
        place_object_for_draw(other);
        play_sound(7);
    }

    return 1;
}

/*
 * 172c:0ca3, image 0x17f63 - kind 12's step. The cat.
 *
 * It walks in jumps of 0x20, and every jump is checked before it is kept: the
 * cat is moved, `object_overlaps_any` asked whether that put it inside
 * something, and if it did the move is undone by *twice* the step - a jump the
 * other way - and checked again. If that fails too it goes back where it was
 * and sits down, form 0; if the second try worked it turns round, flipping bit
 * 4 of +8.
 *
 * Sitting still it looks for what is near: `link_nearby_objects` over a box
 * that reaches 0xf0 the way it faces and 0x110 the other, and each candidate
 * gets a range at which the cat will react - a mouse, kind 0x2a, inside a small
 * box is caught outright, hidden and sounded; a kind 0x0f is 0x124 away if it
 * is past form 0x0b and 0x60 otherwise; everything else is out of reach. The
 * first thing moving slower than its range sets the cat off.
 *
 * The two `+0x96` counters are the settling time: twelve steps of standing
 * before it will move again, and four steps of the tail flicking - form 1
 * through 9 - before it settles.
 *
 * "Still" is what the first test asks - the vertical movement since the last
 * step, at +0x20 against +0x28, no more than one pixel - and a cat that is
 * still is the one that gets on with walking and looking. Reading that test the
 * other way round leaves the cat settling for ever instead, which is a
 * difference of six bytes on the first step and a trail of undrawn parts a
 * hundred and fifty frames later.
 */
void part_step_pokey(struct part *part)
{
    struct part *di;
    int16_t dx;                         /* [bp-2] */
    int16_t dy;                         /* [bp-4] */
    int16_t still;                      /* [bp-6] */
    int16_t busy;                       /* [bp-8] */
    int16_t step;                       /* [bp-0xa] */
    int16_t range;                      /* [bp-0xc] */

    dy = part->pos[0].y - part->pos[2].y;
    if (abs(dy) > 1)
        still = 0;
    else
        still = 1;

    if (part->flags_08 & 0x20) {
        if (part->flags_06 & 2) {
            part->flags_08 &= 0xffdf;
            part->form = 0;
        }
    } else if (still == 0 && part->form < 2) {
        if (part->word_96 > 4) {
            part->flags_08 |= 0x20;
            part->form = 1;
            part->word_96 = 0;
        } else {
            part->word_96++;
        }
    } else if (part->form == 1) {
        part->word_96++;
        if (part->word_96 > 0x0c) {
            if (part->flags_08 & 0x10)
                step = 0x20;
            else
                step = -0x20;
            part->word_96 = 0;
            part->pos[0].x += step;
            place_object_for_draw(part);

            if (object_overlaps_any(part) != 0) {
                part->pos[0].x -= step * 2;
                place_object_for_draw(part);

                if (object_overlaps_any(part) != 0) {
                    part->pos[0].x += step;
                    place_object_for_draw(part);
                    part->form = 0;
                } else {
                    part->form = 2;
                    part->flags_08 ^= 0x10;
                }
            } else {
                part->form = 2;
            }

            part->fx = part->pos[0].x;
            part->fx <<= 9;
        }
    } else {
        if (part->form != 0) {
            busy = 1;
            part->form++;
            if (part->form == 0x0a)
                part->form = 0;
        } else {
            busy = 0;
        }

        if (part->form == 0) {
            if (part->flags_08 & 0x10)
                link_nearby_objects(part, 0x3000, 0, 0xf0, 0, 0);
            else
                link_nearby_objects(part, 0x3000, (int16_t)0xff10, 0, 0, 0);

            di = PART_PTR(part->next_linked_ptr);
            while (di != PART_NONE) {
                switch (di->kind) {
                case KIND_MORT_THE_MOUSE:
                    dx = di->pos[0].x - part->pos[0].x + 0x10;
                    dy = di->pos[0].y - part->pos[0].y;

                    if (dx > 0 && dx < 0x38 && dy > 0 && dy < 0x28) {
                        mark_part_shapes(di, 3);
                        di->flags_08 |= 0x2000;
                        play_sound(0x0d);
                        range = -1;
                    } else if (busy != 0) {
                        range = 0xc0;
                    } else {
                        range = 0x80;
                    }
                    break;
                case KIND_BOB_THE_FISH:
                    if (di->form >= 0x0b)
                        range = 0x124;
                    else
                        range = 0x60;
                    break;
                default:
                    range = -1;
                    break;
                }

                if (abs(di->link_dx) < range) {
                    if (part->flags_08 & 0x10)
                        step = 0x20;
                    else
                        step = -0x20;
                    part->word_96 = 0;
                    part->pos[0].x += step;
                    place_object_for_draw(part);

                    if (object_overlaps_any(part) != 0) {
                        part->pos[0].x -= step;
                        place_object_for_draw(part);
                        part->form = 0;
                    } else {
                        part->form = 2;
                    }

                    di = PART_NONE;
                    part->fx = part->pos[0].x;
                    part->fx <<= 9;
                } else {
                    di = PART_PTR(di->next_linked_ptr);
                }
            }
        }
    }

    if (part->form != part->form_prev)
        place_object_for_draw(part);
}

/*
 * 172c:0f3d, image 0x181fd - kind 12's flip.
 *
 * Byte for byte the same routine as `part_flip_bellow` above with a different
 * setup behind it - checked as bytes, not assumed: of the twenty flips in this
 * segment only four are this shape and the other sixteen are not, so the
 * family is real but small.
 */
void part_flip_pokey(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_pokey(part);

    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:0f70, image 0x18230 - a setup.
 *
 * Twelve points, the longest outline of the fourteen: a head on a pair of
 * legs, 45 wide and 63 tall.
 */
void part_setup_bird_cage(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 0;
    si->y = 24;
    si++;
    si->x = 19;
    si->y = 0;
    si++;
    si->x = 25;
    si->y = 0;
    si++;
    si->x = 45;
    si->y = 24;
    si++;
    si->x = 45;
    si->y = 63;
    si++;
    si->x = 43;
    si->y = 63;
    si++;
    si->x = 43;
    si->y = 26;
    si++;
    si->x = 34;
    si->y = 16;
    si++;
    si->x = 11;
    si->y = 16;
    si++;
    si->x = 2;
    si->y = 26;
    si++;
    si->x = 2;
    si->y = 63;
    si++;
    si->x = 0;
    si->y = 63;

    part_finish_angles(part);
}

/*
 * 172c:0ffc, image 0x182bc - kind 11's drive.
 *
 * `part_drive_02cd` with a tail. Mode 1 steps +0x0e of what +0x66 points at;
 * otherwise the driven part's 32-bit value at +0x3c - doubled unless the asker
 * is kind 3 - is compared against the limit in the sixth and seventh
 * arguments, and past it the answer is 1.
 *
 * What is new is what happens when it is **not** past: in mode 2 the part is
 * lifted 0x14, its +0x12 stepped, and it is redrawn. So this drive moves the
 * thing it was asked about.
 */
uint16_t part_drive_0ffc(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct belt *belt = BELT_PTR(p2->belt_ptr[0]);   /* [bp-2] */

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
    if (p4 == 2) {
        p2->pos[0].y -= 0x14;
        p2->direction++;
        place_object_for_draw(p2);
    }

    return 0;
}

/*
 * 172c:1075, image 0x18335 - a setup.
 *
 * Seven points from the one table at DGROUP 0x3266, with nothing to choose:
 * this part has a single shape.
 */
void part_setup_christmas_tree(struct part *part)
{
    struct part_point *dst;
    const struct point8 *src;
    int16_t i;

    src = PARTSHAPES.s_3266;
    for (i = 0, dst = POINTS(part->points_ptr); i < 7; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:10b6, image 0x18376 - two tables again, but chosen by the form at
 * +0x0c rather than by the flag at +8: zero takes 0x3274 and anything else
 * 0x3282. Those two sit right after 0x3266, which the 0x1075 copy above
 * uses, so all three are one array of seven-pair rows and this picks the
 * second or the third.
 */
void part_setup_10b6(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->form == 0)
        si = PARTSHAPES.s_3274;
    else
        si = PARTSHAPES.s_3282;

    for (i = 0, di = POINTS(part->points_ptr); i < 7; i++, di++, si++) {
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
void part_setup_1105(struct part *part)
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

    di = POINTS(part->points_ptr);
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
void part_step_11a6(struct part *part)
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
 * Flags of exactly 1 is the counting pass `part_drive_2c19` also recognises:
 * the belt's +0x0e goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise it is a contest of momentum. The part's own at +0x3c - the long
 * the record doc calls speed, weight times how fast it is going - is measured
 * against the momentum the drive arrived with, and the part refuses when its
 * own is the greater, which ends the caller's walk. Driven straight from a
 * kind 3 - the motor - the part's momentum counts once; through anything else
 * it counts *twice*, so the same drive that turns a thing directly can fail to
 * turn it at one more remove.
 */
uint16_t part_drive_11d2(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t momentum)
{
    struct belt *belt = BELT_PTR(part->belt_ptr[0]);   /* [bp-2] */

    if (flags == 1) {
        belt->v[0]++;
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

/*
 * 172c:1237, image 0x184f7 - kind 19's hit test. A kind-0x14 part bursts it,
 * and `burst_dynamite` is given the **struck part** here where kind 20's hit
 * gives it the collision record. The two call sites disagree and are
 * transcribed as they are.
 */
uint16_t part_hit_dynamite(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);

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
        src = PARTSHAPES.s_329a;
    } else {
        part->hold.x = 0x2d;
        src = PARTSHAPES.s_3290;
    }

    part->hold.y = 0x0f;

    for (i = 0, dst = POINTS(part->points_ptr); i < 5; i++, dst++, src++) {
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

    if ((si = make_part(KIND_BLAST)) == PART_NONE)
        goto done;

    play_sound(8);

    insert_sorted(si, &DG521B.placed_parts);
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

/*
 * 172c:13c9, image 0x18689 - kind 50's step.
 *
 * It passes its own state down its rope - as 1 or -1 by its mirror bit while
 * it is on, and as 0 when it is off - and, while it is on, runs its three
 * frames backwards, wrapping -1 round to 2. The first frame of a turn plays
 * sound 0x0c and sets DGROUP 0x52cd to 2.
 */
void part_step_motor(struct part *part)
{
    struct part *di;

    if ((di = rope_other_end(part)) != PART_NONE && !(di->flags_08 & 0x800)) {
        if (part->direction == 0)
            di->direction = 0;
        else if (part->flags_08 & 0x10)
            di->direction = 1;
        else
            di->direction = -1;
    }

    if (part->direction != 0) {
        DG52BD.sound_request_0c = 2;

        if (part->form == part->form_prev)
            play_sound(0x0c);

        part->form--;
        if (part->form == -1)
            part->form = 2;
    }
}

/*
 * 172c:1435, image 0x186f5 - five slots from one of two tables, chosen by the
 * flag at +8 bit 4, which also decides the grab box's first byte: 0x25 when
 * set and 0 when clear. The other three box bytes are constant.
 */
void part_setup_motor(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        part->grab.x = 0x25;
        src = PARTSHAPES.s_32ae;
    } else {
        part->grab.x = 0;
        src = PARTSHAPES.s_32a4;
    }

    part->grab.y = 0x0d;
    part->grab_size = 0x12;

    for (i = 0, dst = POINTS(part->points_ptr); i < 5; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:149b, image 0x1875b - kind 50's flip.
 *
 * The bit-4 flip, its setup, and **three** marks rather than two:
 * `mark_joined_shapes` as well, which is what a part with something tied to it
 * needs so the other end is redrawn too.
 */
void part_flip_motor(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_motor(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:14d3, image 0x18793 - kind 21's hit test. The see-saw.
 *
 * Which way it tips comes from the angle at +0x88 of the thing that hit it,
 * turned a quarter and then read as a sign: the high bit of `angle + 0x4000`.
 * Below form 4 and pushed one way it goes up by four; at form 4 or above and
 * pushed the other it comes down by four, and either move runs its own setup
 * again and plays sound 0x11.
 *
 * Then it is *on* whenever its form is not the one at +0x90 - which is where a
 * see-saw's rest position is kept - and a kind 0x14 that hit it has its
 * sideways velocity stepped down by one. It answers 0, so the hit does not
 * count as a landing.
 */
uint16_t part_hit_electric_plug(struct part *part)
{
    struct part *si = PART_PTR(part->contact_ptr);
    uint16_t turned;                    /* [bp-2] */

    turned = part->contact_angle;
    turned += 0x4000;

    if (si->form < 4) {
        if (!(turned & 0x8000)) {
            si->form += 4;
            part_setup_electric_plug(si);
            play_sound(0x11);
        }
    } else if (turned & 0x8000) {
        si->form -= 4;
        part_setup_electric_plug(si);
        play_sound(0x11);
    }

    if (si->form != si->start_form)
        si->direction = 1;
    else
        si->direction = 0;

    if (part->kind == KIND_BULLET)
        part->vel_x--;

    return 0;
}

/*
 * 172c:1556, image 0x18816 - a setup.
 *
 * The tail is the part worth reading twice. +0x80 is set to 4 for the duration
 * of `part_finish` and put back to 1 afterwards, so the finish sees four
 * points on a part that carries one; and then +0x0c keeps only bit 2 and takes
 * bits 0 and 1 from whichever of +0x62 and +0x64 is linked.
 *
 * A routine transcribed from this address stopped at the finish and had none
 * of that. It was never called - it duplicated this body, which already ran -
 * and comparing the two is how the truncation was found. Read a routine to its
 * `retf`, not to its loop.
 */
void part_setup_electric_plug(struct part *part)
{
    /*
     * The form decides the table by being under 4 rather than by equalling
     * anything, and the count at +0x80 is **raised to 4 for the angles and
     * then dropped to 1** - so the part has four connection points while
     * they are being measured and one afterwards.
     */
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->form < 4)
        src = PARTSHAPES.s_32b8;
    else
        src = PARTSHAPES.s_32c0;

    for (i = 0, dst = POINTS(part->points_ptr); i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part->point_count = 4;
    part_finish_angles(part);
    part->point_count = 1;

    part->form &= 4;
    if (part->link_ptr[4] != 0)
        part->form |= 1;
    if (part->link_ptr[5] != 0)
        part->form |= 2;
}

/*
 * 172c:15ce, image 0x1888e - kind 21's step.
 *
 * It does not move: it marks itself done - bit 6 of +8 - and passes its own
 * +0x12 on to whatever is in its links 4 and 5. The first four links are
 * something else's; these two are the ones this kind wires up.
 */
void part_step_electric_plug(struct part *part)
{
    int16_t i;                          /* dx */
    struct part *di;

    part->flags_08 |= 0x40;

    for (i = 4; i < 6; i++)
        if ((di = PART_PTR(part->link_ptr[i])) != PART_NONE)
            di->direction = part->direction;
}

/*
 * 172c:15fc, image 0x188bc - kind 21's flip, and **it is not a flip of bit 4
 * at all**. The form at +0x0c is swung between 4 and 0 - four or more goes to
 * zero, anything else to four - and copied into +0x90 before the setup runs.
 *
 * So this part has two forms held in the form word rather than in the flags,
 * and reading the family's name onto it would have got it wrong.
 */
void part_flip_electric_plug(struct part *part)
{
    if (part->form < 4)
        part->form = 4;
    else
        part->form = 0;

    part->start_form = part->form;

    part_setup_electric_plug(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:1649, image 0x18909 - kind 41's step. The blast.
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
void part_step_1649(struct part *part)
{
    struct part *si;
    int16_t  v02;                       /* [bp-2] the speed */
    uint16_t v04;                       /* [bp-4] the angle away */

    if (part->form == 5) {
        mark_part_shapes(part, 3);
        part->flags_08 |= 0x2000;
    } else {
        part->form++;
        place_object_for_draw(part);
    }

    if (part->form == 2) {
        link_nearby_objects(part, 0x3000, -0x14, 0x14, -0x18, 0x18);

        for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
             si = PART_PTR(si->next_linked_ptr)) {
            if (si->flags_06 & 0x1000) {
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
 * 172c:1748, image 0x18a08
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

    m = PART_KINDS[part->kind].weight;

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
 * 172c:17bc, image 0x18a7c - the blast tearing a kind 1 or kind 0x30 in two.
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
                if ((di = clone_part(part)) == PART_NONE)
                    goto out;

                insert_sorted(di, &DG521B.placed_parts);
                di->flags_06 |= 0x10;

                di->size[0].width = part->pos[0].x + part->size[0].width - v06;
                di->box[0].x = di->pos[0].x = v06;
                di->box[0].y = di->pos[0].y = part->pos[0].y;

                part->size[0].width = v04 - part->pos[0].x;

                part_setup_48ab(di);
            } else if (part->pos[0].x + part->size[0].width > v04) {
                part->size[0].width = v04 - part->pos[0].x;
            }

            part_setup_48ab(part);
        } else if (part->pos[0].x + part->size[0].width > v06) {
            if (part->pos[0].x < v06) {
                part->size[0].width = part->pos[0].x + part->size[0].width - v06;
                part->box[0].x = part->pos[0].x = v06;
                part_setup_48ab(part);
            }
        } else if (part->pos[0].x < v06
                   && part->pos[0].x + part->size[0].width > v04) {
            part->flags_08 |= 0x2000;
        }
    } else {
        v0a = ((v08 - 0x20) & 0xfff0) + 8;
        v0c = ((v08 + 0x18) & 0xfff0) + 8;

        if (part->pos[0].y < v0a) {
            if (part->pos[0].y + part->size[0].height > v0c) {
                if ((di = clone_part(part)) == PART_NONE)
                    goto out;

                insert_sorted(di, &DG521B.placed_parts);
                di->flags_06 |= 0x10;

                di->size[0].height = part->pos[0].y + part->size[0].height - v0c;
                di->box[0].x = di->pos[0].x = part->pos[0].x;
                di->box[0].y = di->pos[0].y = v0c;

                part->size[0].height = v0a - part->pos[0].y;

                part_setup_48ab(di);
            } else if (part->pos[0].y + part->size[0].height > v0a) {
                part->size[0].height = v0a - part->pos[0].y;
            }

            part_setup_48ab(part);
        } else if (part->pos[0].y + part->size[0].height > v0c) {
            if (part->pos[0].y < v0c) {
                part->size[0].height = part->pos[0].y + part->size[0].height - v0c;
                part->box[0].y = part->pos[0].y = v0c;
                part_setup_48ab(part);
            }
        } else if (part->pos[0].y < v0c
                   && part->pos[0].y + part->size[0].height > v0a) {
            part->flags_08 |= 0x2000;
        }
    }

out:
    ;
}

/*
 * 172c:19db, image 0x18c9b - a setup, and one of the few that writes no
 * connection points at all: just the two bytes of the grab box.
 *
 * +0x6a is always 7; +0x6b is 0x0e or 1 as **bit 5** of +8 says - which is the
 * bit `part_flip_hook` turns over, where every other flip in this segment uses
 * bit 4. It does not call `part_finish_angles`, because it changed nothing
 * that would need the angles redone.
 */
void part_setup_hook(struct part *part)
{
    part->attach[0].x = 7;

    if (part->flags_08 & 0x20)
        part->attach[0].y = 14;
    else
        part->attach[0].y = 1;
}

/*
 * 172c:19fa, image 0x18cba - kind 23's flip, and **it turns over bit 5, not
 * bit 4**. Every other flip in this segment xors 0x10; this one xors 0x20, so
 * whatever "the other way round" means for this kind is held somewhere else in
 * the flags. Its setup at 172c:19db is one of the six that write +0x6a.
 */
void part_flip_hook(struct part *part)
{
    part->flags_08 ^= 0x20;

    part_setup_hook(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:1a32, image 0x18cf2 - a setup.
 *
 * Five points, bit 4 of +8 choosing 0x32d2 or 0x32c8.
 */
void part_setup_fan(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->flags_08 & 0x10)
        si = PARTSHAPES.s_32d2;
    else
        si = PARTSHAPES.s_32c8;

    for (i = 0, di = POINTS(part->points_ptr); i < 5; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:1a82, image 0x18d42 - kind 24's step. The fan.
 *
 * Four frames on a loop while it is on, the first playing sound 9, and DGROUP
 * 0x52cf set to 2. The blast is a box reaching 0x100 out in the direction the
 * mirror bit says and ten up, and everything in it is pushed.
 *
 * The push is not a constant: it is the fan's force times how much slower than
 * 0x100 the thing is already going, shifted down eight and then *divided by the
 * thing's own mass*. So a heavy object barely moves and one already at full
 * speed is not pushed at all.
 *
 * Two kinds answer differently. Kind 0x28 with bit 13 of +6 is switched on
 * rather than pushed, with its +0x9c set to 0x14 - but only below 0xc8, so a
 * fast one is left alone. Kind 0x2d is reset to its first frame after being
 * pushed.
 */
void part_step_fan(struct part *part)
{
    struct part *si;
    int16_t push;                       /* [bp-2] */
    int16_t v;                          /* [bp-4] */
    int16_t scale;                      /* [bp-6] */
    int32_t force;                      /* [bp-0xa] */

    if (part->direction != 0) {
        DG52BD.sound_request_09 = 2;

        if (part->form == part->form_prev)
            play_sound(9);

        part->form++;
        if (part->form == 4)
            part->form = 0;

        if (part->flags_08 & 0x10) {
            link_nearby_objects(part, 0x3000, (int16_t)0xff00, 0, -10, 0);
            push = (int16_t)0xf000;
        } else {
            link_nearby_objects(part, 0x3000, 0, 0x100, -10, 0);
            push = 0x1000;
        }

        for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
             si = PART_PTR(si->next_linked_ptr)) {
            if (si->flags_06 & 0x2000) {
                if (si->kind == KIND_WINDMILL && abs(si->link_dx) < 0xc8) {
                    si->direction = 1;
                    si->spin = 0x14;
                }
            } else {
                scale = 0x100 - abs(si->link_dx);
                force = mul16x16(push, scale);
                force >>= 8;
                v = force / PART_KINDS[si->kind].weight;
                si->vel_x += v;

                clamp_record_pair(si);

                if (si->kind == KIND_CANDLE)
                    si->form = si->direction = si->spin = 0;
            }
        }

        place_object_for_draw(part);
    }
}

/*
 * 172c:1bbd, image 0x18e7d - kind 24's flip. Bit 4, its setup, and the two
 * marks without `mark_joined_shapes`, the same shape as `part_flip_dynamite`.
 */
void part_flip_fan(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_fan(part);

    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:1be9, image 0x18ea9 - a setup.
 *
 * Eight points out of a **four-byte-stride** table at DGROUP 0x32dc, x at +0
 * and y at +2 of each row - unlike the two-byte tables the other setups walk,
 * so the index is multiplied rather than the pointer advanced. It also sets
 * +0x80 to 8, which is the count the part carries.
 */
void part_setup_bob_the_fish(struct part *part)
{
    struct part_point *si;
    int16_t i;

    part->point_count = 8;

    for (i = 0, si = POINTS(part->points_ptr); i < 8; i++, si++) {
        si->x = (uint8_t)PARTSHAPES.p_32dc[i].x;
        si->y = (uint8_t)PARTSHAPES.p_32dc[i].y;
    }

    part_finish_angles(part);
}

/*
 * 172c:1c39, image 0x18ef9 - kind 15's hit test.
 *
 * A kind 15 already past form 0x0b is broken and the hit counts - answer 1.
 * One that is not gets broken by the hit and the hit does *not* count, so the
 * thing that broke it carries on through rather than bouncing off.
 */
uint16_t part_hit_bob_the_fish(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);

    if (((int16_t)other->form) >= 0x0b)
        return 1;

    break_bob_the_fish(other);
    return 0;
}

/*
 * 172c:1c5f, image 0x18f1f - kind 15's step.
 *
 * A two-part animation. Below a count of 0x14 at +0x9c the form runs on every
 * step; past 0x16 it drops back to 0x0e and the count goes up by one, and at
 * 0x0b it wraps to zero - so the first eleven frames play once and then it
 * loops on 0x0e to 0x16 for as long as the count allows.
 */
void part_step_bob_the_fish(struct part *part)
{
    if (part->spin < 0x14)
        part->form++;

    if (((int16_t)part->form) > 0x16) {
        part->form = 0x0e;
        part->spin++;
    } else if (part->form == 0x0b) {
        part->form = 0;
    }

    if (part->form != part->form_prev)
        place_object_for_draw(part);
}

/*
 * 172c:1c9e, image 0x18f5e
 *
 * Break bob the fish's bowl: form 0x0b is the broken one, and a part already at
 * 0x0b or past it is left alone. Breaking plays sound 0x0a and replaces the
 * connection points with three of its own - the broken shape has a different
 * outline from the whole one.
 */
void break_bob_the_fish(struct part *part)
{
    /* Three points, and the original keeps a cursor per point rather than
       indexing - `di`, `di + 4`, `di + 8` - and writes them out of order. */
    struct part_point *di;
    struct part_point *p1;              /* [bp-2] */
    struct part_point *p2;              /* [bp-4] */

    if (part->form < 0x0b) {
        part->form = 0x0b;
        place_object_for_draw(part);
        play_sound(0x0a);

        part->point_count = 3;

        di = POINTS(part->points_ptr);
        p1 = di + 1;
        p2 = p1 + 1;

        di->x = 8;
        di->y = p2->y = 47;
        p1->x = 24;
        p1->y = 44;
        p2->x = 39;
    }
}

/*
 * 172c:1d07, image 0x18fc7 - kind 25's hit test.
 *
 * The hook is the *linked* thing's, run on whatever ran into it: `di` is the
 * kind 25 part at the hit object's +0x84 and `si` the object that arrived.
 *
 * Unless the arriving object is already spoken for - a non-zero +0x88 - the
 * kind 25 part is set going, which its step at 172c:1d78 then acts on. Either
 * way the answer is 1: the hit counts.
 */
uint16_t part_hit_flashlight(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);

    if (((uint16_t)part->contact_angle) == 0)
        other->direction = 1;

    return 1;
}

/*
 * 172c:1d28, image 0x18fe8 - a setup.
 *
 * Six points, from DGROUP 0x3308 or 0x32fc as bit 4 of +8 says. Two bytes a
 * point at the source and four at the destination, as usual.
 */
void part_setup_flashlight(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->flags_08 & 0x10)
        si = PARTSHAPES.s_3308;
    else
        si = PARTSHAPES.s_32fc;

    for (i = 0, di = POINTS(part->points_ptr); i < 6; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:1d78, image 0x19038 - kind 25's step.
 *
 * One move and then nothing: turned on in form 0 it steps to form 1, runs its
 * own setup again because the shape has changed, and plays sound 0x11. In any
 * other form it does nothing at all.
 */
void part_step_flashlight(struct part *part)
{
    if (part->direction != 0 && part->form == 0) {
        part->form++;
        part_setup_flashlight(part);
        place_object_for_draw(part);
        play_sound(0x11);
    }
}

/*
 * 172c:1da8, image 0x19068 - kind 25's flip, the three-mark shape.
 */
void part_flip_flashlight(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_flashlight(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:1de0, image 0x190a0 - kind 26's hit test. The pulley wheel.
 *
 * Nothing happens. The original still loads the part at the object's +0x84 into
 * a local and then never reads it, which is a hook written from the same
 * template as the ones that do use it - so the wheel is touchable and is
 * unmoved by being touched.
 */
uint16_t part_hit_generator(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);   /* read, and never used */

    (void)other;
    return 1;
}

/*
 * 172c:1dfb, image 0x190bb - a setup.
 *
 * A part 0x47 by 0x1f with its grab box at +0x56..+0x58, four corner points
 * written straight out, and **the form recomputed after the finish**: +0x0c is
 * masked to its bottom two bits and then bit 2 or bit 3 set for each of the
 * two links that is attached.
 *
 * That is `part_setup_solar_panel`'s question asked one field along - there it is
 * bits 0 and 1 of a +0x0c that starts at zero, here bits 2 and 3 of one that
 * keeps whatever two bits it already had.
 */
void part_setup_generator(struct part *part)
{
    struct part_point *si;

    part->grab.x = 56;
    part->grab.y = 18;
    part->grab_size = 0x0c;

    si = POINTS(part->points_ptr);

    si->x = 21;
    si->y = 0;
    si++;
    si->x = 71;
    si->y = 0;
    si++;
    si->x = 71;
    si->y = 31;
    si++;
    si->x = 21;
    si->y = 31;

    part_finish_angles(part);

    part->form &= 3;

    if (part->link_ptr[4] != 0)
        part->form |= 4;

    if (part->link_ptr[5] != 0)
        part->form |= 8;
}

/*
 * 172c:1e5c, image 0x1911c - kind 26's step. The pulley wheel.
 *
 * It stops if the gear its rope reaches is not turning - kind 0x0e with its
 * last two forms equal - and otherwise runs its four frames in the direction
 * its +0x12 says, wrapping within the low two bits so the form's other bits
 * survive the turn. The first frame plays sound 0x0c and sets DGROUP 0x52cd.
 *
 * Whatever happens it passes its own state on to links 4 and 5.
 */
void part_step_generator(struct part *part)
{
    struct part *di;
    int16_t i;                          /* [bp-2] */

    if (part->direction != 0 && (di = rope_other_end(part)) != PART_NONE
        && di->kind == KIND_GEAR && di->form_prev == di->form_prev2)
        part->direction = 0;

    if (part->direction != 0) {
        DG52BD.sound_request_0c = 2;

        if (part->form == part->form_prev)
            play_sound(0x0c);

        if (part->direction > 0) {
            if ((part->form & 3) == 3)
                part->form -= 3;
            else
                part->form++;
        } else {
            if ((part->form & 3) == 0)
                part->form += 3;
            else
                part->form--;
        }

        place_object_for_draw(part);
    }

    for (i = 4; i < 6; i++)
        if ((di = PART_PTR(part->link_ptr[i])) != PART_NONE)
            di->direction = part->direction;
}

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

    other = PART_PTR(part->contact_ptr);
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

    part_setup_0001(part);

    for (i = 0; i < 4; i++)
        part->link_ptr[i] = 0;

    for (di = PART_PTR(DG521B.placed_parts.next_ptr); di != PART_NONE;
         di = PART_PTR(di->next_ptr)) {
        if (di != part && di->kind == KIND_GEAR) {
            dx = part->start_x - di->start_x;
            dy = part->start_y - di->start_y;

            if (dy == 0) {
                if (dx == 0x20)
                    part->link_ptr[0] = dg_near(dgroup, di);
                else if (dx == -0x20)
                    part->link_ptr[1] = dg_near(dgroup, di);
            } else if (dx == 0) {
                if (dy == 0x20)
                    part->link_ptr[2] = dg_near(dgroup, di);
                else if (dy == -0x20)
                    part->link_ptr[3] = dg_near(dgroup, di);
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
        part->flags_08 |= 0x40;

        di = 0;
        for (v02 = 0; v02 < 4; v02++)
            if ((v04 = PART_PTR(part->link_ptr[v02])) != PART_NONE)
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
 * `how` says how the two are joined: 1 is a rope, which carries the direction
 * unchanged, and 2 is a mesh, which reverses it. A gear that is not turning yet
 * takes the direction; one that is already turning is checked against it, and
 * a mismatch - the same direction through a mesh, or a different one through a
 * rope - is the jam this answers 1 for.
 *
 * From a gear, kind 0x0e, it goes on to that gear's own four links and its
 * rope, marking each as it goes so a ring of gears is walked once. `flag` is
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

    if (to->kind == KIND_GEAR && !(to->flags_08 & 0x40)) {
        to->flags_08 |= 0x40;

        for (v02 = 0; v02 < 5; v02++) {
            if (v02 == 4) {
                v06 = rope_other_end(to);
                v04 = 1;
            } else {
                v06 = PART_PTR(to->link_ptr[v02]);
                v04 = 2;
            }

            if (v06 != PART_NONE && !(v06->flags_08 & 0x800))
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
            di = rope_other_end(part);
        else
            di = PART_PTR(part->link_ptr[v02]);

        if (di != PART_NONE && di->direction != 0 && !(di->flags_08 & 0x800)) {
            if (clear != 0)
                di->direction = 0;

            if (di->kind == KIND_GEAR)
                settle_gear_signal(di, clear);
        }
    }
}

/*
 * 172c:22ae, image 0x1956e - kind 27's step. The gun.
 *
 * Six frames once it is set going, the second playing sound 0x0b, and the
 * third fires: `make_part` builds a kind 0x14, `insert_sorted` puts it on the
 * list at DGROUP 0x5179, and it gets a position and a sideways velocity left
 * or right by the mirror bit - and, mirrored, its own setup at 172c:08a1 runs
 * as well, because the bullet is a different shape that way round.
 *
 * A gun that could not get the bullet from the heap simply does not fire.
 */
void part_step_gun(struct part *part)
{
    struct part *si;

    if (part->direction != 0 && part->form != 6) {
        part->form++;
        place_object_for_draw(part);

        if (part->form == 2)
            play_sound(0x0b);

        if (part->form == 3 && (si = make_part(KIND_BULLET)) != PART_NONE) {
            insert_sorted(si, &DG5179.moving_parts);
            si->flags_06 |= 0x10;

            if (part->flags_08 & 0x10) {
                si->flags_08 |= 0x10;
                part_setup_08a1(si);

                si->pos[0].x = part->pos[0].x - 0x20;
                si->pos[1].x = si->pos[2].x = si->pos[0].x + 0x18;
                si->vel_x = (int16_t)0xd000;
            } else {
                si->pos[0].x = part->pos[0].x + 0x24;
                si->pos[1].x = si->pos[2].x = si->pos[0].x - 0x18;
                si->vel_x = 0x3000;
            }

            si->pos[0].y = si->pos[1].y = si->pos[2].y = part->pos[0].y + 3;

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
 * 172c:23b1, image 0x19671 - a setup.
 *
 * Seven points, with the grab box's width at +0x6a following the same flag:
 * 0x2a with the table at 0x3322, 0x12 with 0x3314. The height at +0x6b is 0x12
 * either way, so one way round the part is square and the other it is not.
 */
void part_setup_gun(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        part->attach[0].x = 42;
        src = PARTSHAPES.s_3322;
    } else {
        part->attach[0].x = 18;
        src = PARTSHAPES.s_3314;
    }

    part->attach[0].y = 18;

    for (i = 0, dst = POINTS(part->points_ptr); i < 7; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:2412, image 0x196d2 - kind 27's flip: bit 4, its setup, and all four
 * redraws - the draw and the three marks.
 */
void part_flip_gun(struct part *part)
{
    part->flags_08 ^= 0x10;
    part_setup_gun(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2451, image 0x19711 - kind 27's drive hook.
 *
 * Flags of exactly 1 is the counting pass the other drive hooks recognise: the
 * belt's +0x0e goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise only bits 3, 4 and 15 of the flags are kept, and bit 15 is then
 * dropped again for the comparisons - so the drive is read twice, once with
 * the top bit and once without, and the two readings do different jobs. Which
 * of bits 3 and 4 means "the way this part faces" depends on bit 4 of its own
 * +8, and the two halves are mirror images with 8 and 0x10 swapped.
 *
 * Driven **against** the way it faces it refuses, answering 1, which ends the
 * caller's walk. Driven with it while already going it also refuses - it has
 * nothing left to give. Driven with it while stopped, and with the top bit
 * clear, it starts: +0x12 becomes 1 and the answer is 0 so the walk goes on
 * past it.
 */
uint16_t part_drive_2451(struct part *p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t p6)
{
    struct belt *di = BELT_PTR(si->belt_ptr[0]);
    uint16_t low;                       /* cx */

    if (flags == 1) {
        di->v[0]++;
        return 0;
    }

    flags &= 0x8018;
    low = flags & 0x7fff;

    if (si->flags_08 & 0x10) {
        if (low == 8)
            goto yes;
        if (low == 0x10 && si->direction != 0)
            goto yes;
        if (flags == 0x10 && si->direction == 0)
            si->direction = 1;
    } else {
        if (low == 0x10)
            goto yes;
        if (low == 8 && si->direction != 0) {
yes:
            return 1;
        }
        if (flags == 8 && si->direction == 0)
            si->direction = 1;
    }

    return 0;
}

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

    si = POINTS(part->points_ptr);
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

    other = PART_PTR(part->contact_ptr);
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

    if (part->direction != 0 && (di = rope_other_end(part)) != PART_NONE
        && di->kind == KIND_GEAR && di->form_prev == di->form_prev2)
        part->direction = 0;

    if (part->direction != 0) {
        DG52BD.sound_request_01 = 2;

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

    di = POINTS(part->points_ptr) + 1;
    p2 = di + 1;
    di->x = p2->x = part->size[0].width;

    part->start_form = part->form = (part->size[0].width - 0x20) / 0x10 * 7;

    /* `mov al,[bx+0x3330]`: the grab x by width step. */
    part->grab.x = PARTSHAPES.conveyor_grab_x[(part->size[0].width - 0x20) / 0x10];
}

/* 172c:2682, image 0x19942 - a setup: seven points from DGROUP 0x3336. */
void part_setup_heart_balloon(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = PARTSHAPES.s_3336;
    for (i = 0, si = POINTS(part->points_ptr); i < 7; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:26c3, image 0x19983 - kind 33's drive, and it is `part_drive_02cd`
 * again with nothing added: mode 1 steps +0x0e of what +0x66 points at, and
 * otherwise the driven part's 32-bit value at +0x3c - doubled unless the asker
 * is kind 3 - answers 1 when it is past the limit.
 */
uint16_t part_drive_26c3(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct belt *belt = BELT_PTR(p2->belt_ptr[0]);   /* [bp-2] */

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

/*
 * 172c:2728, image 0x199e8 - kind 2's setup. The ramp.
 *
 * Four points, from one of two tables of offsets picked by the mirror bit at
 * +8 and indexed by the form. The same shape as the bellow's and the
 * scissors' - `shl bx,1` on the form, `[bx + 0x338c]` or `[bx + 0x3364]` -
 * and the copy steps the source two bytes at a time and the destination four.
 */
void part_setup_ramp(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10)
        src = POINT_TABLE(PARTSHAPES.o_338c[part->form]);
    else
        src = POINT_TABLE(PARTSHAPES.o_3364[part->form]);

    for (i = 0, dst = POINTS(part->points_ptr); i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:2789, image 0x19a49 - kind 2's settle. The same copy of the dragged
 * size into the real one, a form of `width / 0x10 - 1`, and then its own setup
 * at 172c:2728 to rebuild the connection points from it.
 */
void part_settle_ramp(struct part *part)
{
    part->size[0] = part->set_size;
    part->start_form = part->form = part->size[0].width / 0x10 - 1;

    part_setup_ramp(part);
}

/*
 * 172c:27b6, image 0x19a76 - **kind 2's flip**, the hook at +0x30 of its kind
 * record that `part_flip_options` calls to try an end and then put it back.
 *
 * Flipping is one `xor` of bit 0x10 in +8, which is why calling it twice with
 * the same argument restores the part exactly - the caller relies on that, and
 * it is the whole reason a routine that changes the machine can be used as a
 * test.
 *
 * The argument the caller pushes past the part is **not read**: the frame
 * takes only [bp+6]. Kind 2 has one flip, so which end was asked for makes no
 * difference to it.
 *
 * The outline is reloaded for the new bit, and the part is then marked twice -
 * `mark_part_shapes` with 3 and `mark_needs_refile` with 2 - so what was drawn
 * for the old orientation is re-filed for the new one.
 */
void part_flip_ramp(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_ramp(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

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
            part->word_96++;
        } else if (part->form == -1) {
            part->form = 7;
            part->word_96++;
        }

        if (part->word_96 == 6) {
            play_sound(3);
            part->form = 8;
        }
    }

    if (part->form >= 8 && part->form <= 0x0a) {
        mid = part->pos[0].x + (part->size[0].width >> 1);

        /* The reach by form, 0x3394, with the first form folded into the
           address: `[bx+0x3384]`. */
        link_objects_in_range(part, 0x3000, 0, 0x1f,
                              (PARTSHAPES.jack_reach - 8)[part->form], 0);

        for (di = PART_PTR(part->next_linked_ptr); di != PART_NONE;
             di = PART_PTR(di->next_linked_ptr)) {
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

    si = POINTS(part->points_ptr);
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

/*
 * 172c:2b58, image 0x19e18 - no connection points, only the grab box, and both
 * its bytes come out of one table indexed by the part's form at +0x0c.
 */
void part_setup_light(struct part *part)
{
    part->attach[0].x = (uint8_t)PARTSHAPES.p_339a[part->form].x;
    part->attach[0].y = (uint8_t)PARTSHAPES.p_339a[part->form].y;
}

/*
 * 172c:2b7e, image 0x19e3e - kind 29's hit test.
 *
 * The same do-nothing as `part_hit_generator`, down to the unused local.
 */
uint16_t part_hit_light(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);   /* read, and never used */

    (void)other;
    return 1;
}

/*
 * 172c:2b99, image 0x19e59 - kind 29's step.
 *
 * Only forms 0 and 2 move on, and only while +0x12 says it is on: the form
 * steps by one and its own setup runs again, because this kind's connection
 * points depend on the form.
 */
void part_step_light(struct part *part)
{
    if (part->direction != 0 && (part->form == 0 || part->form == 2)) {
        part->form++;
        part_setup_light(part);
        place_object_for_draw(part);
    }
}

/*
 * 172c:2bc5, image 0x19e85 - kind 29's flip, a form swing like kind 21's but
 * between 0 and **2** rather than 0 and 4, copied into +0x90 the same way.
 */
void part_flip_light(struct part *part)
{
    if (part->form != 0)
        part->form = 0;
    else
        part->form = 2;

    part->start_form = part->form;

    part_setup_light(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2c19, image 0x19ed9 - kind 29's drive hook.
 *
 * The arguments are the seven `drive_belts` hands over; this one uses only the
 * part at +8 and the flags at +0x0c.
 *
 * Flags of exactly 1 means "count how many belts reach here": the belt's +0x0e
 * goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise only bits 1, 2 and 15 of the flags are kept. A belt running that
 * way over a part already going - or bit 1 on its own - refuses, which is what
 * stops the drive: it answers 1 and the caller's walk ends. Bit 2 on a part
 * that is *not* going starts it instead, with sound 0x11, and answers 0.
 */
uint16_t part_drive_2c19(struct part *p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t p6)
{
    uint16_t kept;                      /* [bp-2] */
    struct belt *belt;                  /* [bp-4] */

    belt = BELT_PTR(si->belt_ptr[0]);

    if (flags == 1) {
        belt->v[0]++;
        return 0;
    }

    flags &= 0x8006;
    kept = flags & 0x7fff;

    if (kept == 2)
        goto yes;
    if (kept == 4 && si->direction != 0) {
yes:
        return 1;
    }

    if (flags == 4 && si->direction == 0) {
        play_sound(0x11);
        si->direction = 1;
    }

    return 0;
}

/*
 * 172c:2c83, image 0x19f43 - kind 31's hit test, the third way into
 * `part_step_monkey`'s timer after its own drive and its rope.
 *
 * It only fires when the timer at +0x96 is already at rest, and only on faces
 * 0, 1 and 2 - a strike from behind does nothing. Then the timer is loaded
 * with 0x1c, whatever the part was doing is cleared, and the form is put at
 * the head of one of the step's two animation loops: 5 for a part sitting at
 * 0, and 9 for one anywhere else.
 *
 * The `jmp` to the next instruction at 0x19f86 is the compiler leaving a
 * return path in that nothing needed.
 */
uint16_t part_hit_monkey(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);
    int16_t  face = ((int16_t)part->contact_edge);

    if (other->word_96 == 0 && face < 3) {
        other->word_96 = 0x1c;
        other->direction = 0;

        if (other->form == 0)
            other->form = 5;
        else
            other->form = 9;
    }

    return 1;
}

/*
 * 172c:2cce, image 0x19f8e - a setup.
 *
 * A part that is a different size each way round: bit 4 of +8 chooses both the
 * grab box's width at +0x6a and the box origin at +0x56 *and* which of the two
 * point tables to walk, 0x33bc or 0x33aa. The height at +0x6b, the other
 * origin byte at +0x57 and the count at +0x58 are the same either way.
 *
 * Nine points, two bytes each at the source and four at the destination.
 */
void part_setup_monkey(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        part->attach[0].x = 16;
        part->grab.x = 36;
        src = PARTSHAPES.s_33bc;
    } else {
        part->attach[0].x = 75;
        part->grab.x = 47;
        src = PARTSHAPES.s_33aa;
    }

    part->attach[0].y = 45;
    part->grab.y = 60;
    part->grab_size = 9;

    for (i = 0, dst = POINTS(part->points_ptr); i < 9; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:2d40, image 0x1a000 - kind 31's step.
 *
 * Whatever is on the other end of its rope is told what this part is doing -
 * +0x12 copied straight across - unless that end is already busy, bit 11 of
 * its +8. `part_step_windmill` below does the same thing for kind 40.
 *
 * The rest is a timer at +0x96 and a form at +0x0c. While the timer runs the
 * form steps, and **it steps round two different loops**: past 9 it goes back
 * to 5, and past 0xd back to 9, so the animation has a short cycle and a long
 * one and which it is on depends on where it started. From 9 up it also stamps
 * 0x35 into the byte at +0x6b.
 *
 * When the timer reaches zero the part either fires - a sound, 0x52d1 set, and
 * +0x12 becoming 1 or -1 by bit 4 of the flags, which is the mirrored form -
 * or simply stops, and which of those depends on the form being past 8.
 *
 * With the timer already at zero the form creeps up by one a step, wrapping 4
 * back to 1 rather than going on, and only a form that differs from the one
 * last drawn at +0x0e is redrawn.
 *
 * The original leaves AX as whatever fell out; nothing reads it.
 */
void part_step_monkey(struct part *part)
{
    struct part *di;

    if ((di = rope_other_end(part)) != PART_NONE && !(di->flags_08 & 0x800))
        di->direction = part->direction;

    if (part->word_96 != 0) {
        part->word_96--;

        if (part->word_96 == 0) {
            if (part->form > 8) {
                play_sound(2);
                DG52BD.sound_request_02 = 2;
                if (part->flags_08 & 0x10)
                    part->direction = -1;
                else
                    part->direction = 1;
                part->form = 1;
            } else {
                part->form = 0;
            }
        } else {
            if (part->form >= 9)
                part->attach[0].y = 53;

            part->form++;

            if (part->form == 9)
                part->form = 5;
            else if (part->form == 0x0d)
                part->form = 9;
        }

        place_object_for_draw(part);
    } else {
        if (part->direction != 0) {
            part->attach[0].y = 53;

            if (part->form != 4)
                part->form++;
            else
                part->form = 1;
        }

        if (part->form != part->form_prev) {
            place_object_for_draw(part);
            DG52BD.sound_request_02 = 2;
        }
    }
}

/*
 * 172c:2e0c, image 0x1a0cc - kind 31's flip, the four-redraw shape.
 */
void part_flip_monkey(struct part *part)
{
    part->flags_08 ^= 0x10;
    part_setup_monkey(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2e4b, image 0x1a10b - kind 31's drive, the other half of
 * `part_step_monkey`.
 *
 * Mode 1 steps the word at +0x0e of what +0x66 points at and answers 0, the
 * same as kind 4's drive at 172c:02cd does.
 *
 * Otherwise the mode is **masked to 0x8006 and then to 0x7fff**, which leaves
 * 2, 4 or 6 and throws the top bit away, and the two masks are not the same
 * question: the second test below asks about the *0x8006* value, so a 4 with
 * the 0x8000 bit still on takes neither arm.
 *
 * Mode 2 always answers yes. Mode 4 answers yes if the part is already going -
 * +0x12 not zero - or its form has reached 9. Failing that it *acts*: a form
 * between 5 and 8 gets four added, and anything else fires, with the sound and
 * 0x52d1 and +0x12 set to 1 or -1 by bit 4 of the flags, exactly as the step
 * does when its timer runs out. Those all answer 0.
 *
 * The `+0x12 != 0` test at 0x1a15e cannot be reached with +0x12 set, because
 * mode 4 has already answered yes in that case. Transcribed anyway.
 */
uint16_t part_drive_2e4b(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    uint16_t kept;                      /* [bp-2] */
    struct belt *belt;                  /* [bp-4] */

    belt = BELT_PTR(p2->belt_ptr[0]);

    if (p4 == 1) {
        belt->v[0]++;
        return 0;
    }

    p4 &= 0x8006;
    kept = p4 & 0x7fff;

    if (kept == 2)
        goto yes;
    if (kept == 4 && (p2->direction != 0 || p2->form >= 9)) {
yes:
        return 1;
    }

    if (p4 == 4 && p2->direction == 0) {
        if (p2->form >= 5 && p2->form <= 8) {
            p2->form += 4;
        } else {
            play_sound(2);
            DG52BD.sound_request_02 = 2;
            if (p2->flags_08 & 0x10)
                p2->direction = -1;
            else
                p2->direction = 1;
        }
    }

    return 0;
}

/*
 * 172c:2ee1, image 0x1a1a1 - a setup.
 *
 * The bounding rectangle again, instruction for instruction the same as
 * 172c:24d0 - two kinds that want the same shape and got their own copy.
 */
void part_setup_mouse_cage(struct part *part)
{
    struct part_point *si;

    si = POINTS(part->points_ptr);
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
 * 172c:2f25, image 0x1a1e5 - kind 6's hit test. The mousetrap.
 *
 * Anything that touches a trap springs it, whatever it was: the hook is the
 * trap's, run on the object that arrived, and the trap itself is the one at
 * that object's +0x84. `trigger_mouse_cage` does the rest.
 */
uint16_t part_hit_mouse_cage(struct part *part)
{
    trigger_mouse_cage(PART_PTR(part->contact_ptr));
    return 1;
}

/*
 * 172c:2f3e, image 0x1a1fe - kind 6's step. The mousetrap.
 *
 * A trap that is not already going looks for a mouse - kind 0x0c - within
 * 0x10 either side, and the first one it finds sets it off. Going or not, it
 * passes its own state along its rope to whatever is not already busy.
 *
 * While it is going the form flips between two and the countdown at +0x96 runs
 * out; reaching zero switches it off again.
 */
void part_step_mouse_cage(struct part *part)
{
    struct part *di;

    if (part->direction == 0) {
        link_nearby_objects(part, 0x1000, -0x10, 0x10, 0, 0);

        di = PART_PTR(part->next_linked_ptr);
        while (di != PART_NONE) {
            if (di->kind == KIND_POKEY) {
                part->direction = 1;
                di = PART_NONE;
            } else {
                di = PART_PTR(di->next_linked_ptr);
            }
        }
    }

    if ((di = rope_other_end(part)) != PART_NONE && !(di->flags_08 & 0x800))
        di->direction = part->direction;

    if (part->direction != 0) {
        part->form ^= 1;
        part->word_96--;
        if (part->word_96 == 0)
            part->direction = 0;
    }
}

/*
 * 172c:2fba, image 0x1a27a - **kind 6's flip**, the +0x30 hook.
 *
 * The same `xor` of bit 0x10 in +8 that kind 2 uses, so calling it twice
 * restores the part and `part_flip_options` can use it as a test. Where kind 2
 * reloads an outline from a table, this one just moves the **anchor byte** at
 * +0x56: 3 when the bit is set and 0x1e when it is clear. That is the whole
 * difference between the two flips - one changes the shape, the other changes
 * where the shape is held.
 *
 * Then the same three marks, in the same order, with the same arguments.
 */
void part_flip_mouse_cage(struct part *part)
{
    part->flags_08 ^= 0x10;

    if (part->flags_08 & 0x10)
        part->grab.x = 3;
    else
        part->grab.x = 30;

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2ffd, image 0x1a2bd
 *
 * Set the mouse cage going, in the direction its mirror bit says. A part that
 * was not going already plays sound 0x0d, and either way its +0x96 is put back
 * to 0x64.
 */
void trigger_mouse_cage(struct part *part)
{
    if (part->direction == 0)
        play_sound(0x0d);

    if (part->flags_08 & 0x10)
        part->direction = -1;
    else
        part->direction = 1;

    part->word_96 = 0x64;
}

/* 172c:3030, image 0x1a2f0 - kind 30's setup, and it does nothing at all:
 * `push bp / mov bp,sp / pop bp / retf`. It has a function here rather than a
 * line in the dispatcher so that its address can be named - to the verifier,
 * and to the coverage tool, which cannot see a routine that exists only as a
 * case. */
void part_setup_magnifying_glass(struct part *part)
{
    (void)part;
}

/*
 * 172c:3035, image 0x1a2f5 - kind 30's step.
 *
 * It reaches for whatever is passing: `link_nearby_objects` builds a chain
 * through +0x78 of everything within 0x20 either side, and this picks one of
 * them to hold at +0x62.
 *
 * Two questions are asked of each candidate. Kinds 0x1d, 0x19 and 0x2d in a
 * form other than zero *block* it - unless the mirror bits agree for 0x19, or
 * the form is 2 for 0x1d - and something is only taken hold of at all if
 * something else blocked. Anything else with bit 2 of +0x0a in form zero is a
 * candidate: it has to be moving towards this part, and to be within 0x30
 * across and no further down than across.
 *
 * What it held last step wins outright if it is still there; otherwise the
 * slowest candidate wins, which is what makes it settle on the thing it can
 * actually catch. Taking hold steps the held part's +0x9c and says this part
 * moved.
 */
void part_step_magnifying_glass(struct part *part)
{
    struct part *si;
    int16_t v02;                        /* [bp-2]   something blocked */
    int16_t v04;                        /* [bp-4]   held it last step */
    int16_t v06;                        /* [bp-6]   the slowest so far */
    int16_t v08;                        /* [bp-8]   this one will do */
    int16_t v0a;                        /* [bp-0xa] the reach */
    int16_t v0c;                        /* [bp-0xc] the drop */
    struct part *v0e;                   /* [bp-0xe] the one held */

    link_nearby_objects(part, 0x3000, -0x20, 0x20, 0, 0);

    v0e = PART_NONE;
    v04 = v02 = 0;
    v06 = 0x190;

    si = PART_PTR(part->next_linked_ptr);
    while (si != PART_NONE) {
        if ((si->kind == KIND_LIGHT || si->kind == KIND_FLASHLIGHT
             || si->kind == KIND_CANDLE) && si->form != 0) {
            if (part->flags_08 & 0x10) {
                if (si->link_dx > 0)
                    v02 = 1;
            } else {
                if (si->link_dx < 0)
                    v02 = 1;
            }

            /* A flashlight facing the *other* way takes the block back. */
            if (si->kind == KIND_FLASHLIGHT) {
                if ((si->flags_08 ^ part->flags_08) & 0x10)
                    v02 = 0;
            } else if (si->kind == KIND_LIGHT && si->form == 2) {
                v02 = 0;
            }
        } else if ((si->flags_0a & 4) && si->form == 0 && v04 == 0) {
            v08 = 0;

            if (part->flags_08 & 0x10) {
                if (si->link_dx < 0)
                    v08 = 1;
            } else {
                if (si->link_dx > 0)
                    v08 = 1;
            }

            grab_distance(part, si, &v0a, &v0c);

            if (v0a >= 0x30 || v0c > v0a)
                v08 = 0;

            if (v08 != 0) {
                if (PART_PTR(part->link_ptr[4]) == si) {
                    v0e = si;
                    v04 = 1;
                } else if (abs(si->link_dx) < abs(v06)) {
                    v06 = si->link_dx;
                    v0e = si;
                }
            }
        }

        if (v02 != 0 && v04 != 0)
            si = PART_NONE;
        else
            si = PART_PTR(si->next_linked_ptr);
    }

    if (v02 == 0)
        v0e = PART_NONE;

    if ((part->link_ptr[4] = dg_near(dgroup, v0e)) != 0) {
        v0e->spin++;
        part_moved(part);
    }
}

/*
 * 172c:31af, image 0x1a46f - kind 30's flip: bit 4 and a redraw, no setup.
 */
void part_flip_magnifying_glass(struct part *part)
{
    part->flags_08 ^= 0x10;
    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:31dc, image 0x1a49c
 *
 * How far one part is from another's grip, as two absolute distances written
 * through pointers.
 *
 * The grip is the part's own left edge, or its right edge when bit 4 of +8 is
 * clear, and eight down from its top; the other part's point is its position
 * plus the two bytes at +0x72 and +0x73, which is where that kind is held.
 */
void grab_distance(struct part *a, struct part *b, int16_t *out_x, int16_t *out_y)
{
    int16_t hx;                         /* [bp-2] */
    int16_t y;                          /* [bp-4] */
    int16_t hy;                         /* [bp-6] */
    int16_t x;                          /* cx */

    x = a->pos[0].x;
    if (!(a->flags_08 & 0x10))
        x += a->size[0].width;
    y = a->pos[0].y + 8;
    hx = b->pos[0].x + b->hold.x;
    hy = b->pos[0].y + b->hold.y;

    *out_x = abs(x - hx);
    *out_y = abs(y - hy);
}

/*
 * 172c:323f, image 0x1a4ff - kind 22's hit test, the trigger for
 * `part_step_dynamite_plunger`.
 *
 * A touch on face 0 sets the part going outright. Any other face has to be
 * something landing on it: the arriving object's +0x38 must be positive, which
 * is the downward half of the velocity pair, its +0x88 must be under 0x800
 * once 0x800 is added - so between -0x800 and 0x800, a shallow angle - and it
 * must be above the part, its +0x20 plus +0x42 short of the part's +0x20 by
 * more than 0xc.
 *
 * It answers 1 either way, like every other hit test here.
 */
uint16_t part_hit_dynamite_plunger(struct part *part)
{
    struct part *di = PART_PTR(part->contact_ptr);
    int16_t face = part->contact_edge;  /* [bp-2] */

    if (face == 0)
        di->direction = 1;
    else if (part->vel_y > 0
             && (int16_t)(part->contact_angle + 0x800) < 0x1000
             && part->pos[0].y + part->mirror_size.height < di->pos[0].y + 0x0c)
        di->direction = 1;

    return 1;
}

/*
 * 172c:3294, image 0x1a554 - a setup.
 *
 * Four points and a grab box, all three read out of tables indexed by the form
 * at +0x0c, and bit 4 of +8 picks which set of three tables. The points come
 * through a **pointer array** - the load is a word - and the two box bytes out
 * of four-byte rows, so the same form indexes two different strides.
 */
void part_setup_dynamite_plunger(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        src = POINT_TABLE(PARTSHAPES.o_3404[part->form]);
        part->attach[0].x = (uint8_t)PARTSHAPES.p_3416[part->form].x;
        part->attach[0].y = (uint8_t)PARTSHAPES.p_3416[part->form].y;
    } else {
        src = POINT_TABLE(PARTSHAPES.o_33e6[part->form]);
        part->attach[0].x = (uint8_t)PARTSHAPES.p_340a[part->form].x;
        part->attach[0].y = (uint8_t)PARTSHAPES.p_340a[part->form].y;
    }

    for (i = 0, dst = POINTS(part->points_ptr); i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:332a, image 0x1a5ea - kind 22's step. **It makes a new part.**
 *
 * On the first frame of its three - +0x0c at 1 - it calls `make_part` for a
 * kind-0x29 part, files it on the list at 0x521b, and puts it half a part to
 * the left of itself, or 0x60 to the right of that when bit 4 of the flags
 * says it is mirrored. The new part's 32-bit position at +0x16 and +0x1a is
 * the 16-bit one shifted left nine, which is the fixed point the physics uses.
 *
 * `make_part` answering zero is a full heap and is simply skipped; the part
 * still steps its own form.
 *
 * The form then walks 1, 2 and stops, rebuilt each time through the setup at
 * 172c:3294, called directly.
 */
void part_step_dynamite_plunger(struct part *part)
{
    struct part *si;

    if (part->direction != 0) {
        if (part->form == 1) {
            play_sound(8);

            if ((si = make_part(KIND_BLAST)) == PART_NONE)
                goto done;

            insert_sorted(si, &DG521B.placed_parts);
            si->flags_06 |= 0x10;
            si->pos[0].x = part->pos[0].x - 0x10;
            si->pos[0].y = part->pos[0].y;

            if (part->flags_08 & 0x10)
                si->pos[0].x += 0x60;

            si->fx = si->pos[0].x;
            si->fx <<= 9;
            si->fy = si->pos[0].y;
            si->fy <<= 9;

            place_object_for_draw(si);
        }
done:
        if (part->form != 2) {
            part->form++;
            part_setup_dynamite_plunger(part);
            place_object_for_draw(part);
        }
    }
}

/*
 * 172c:33e5, image 0x1a6a5 - kind 22's flip: bit 4, its setup, three marks and
 * no draw.
 */
void part_flip_dynamite_plunger(struct part *part)
{
    part->flags_08 ^= 0x10;
    part_setup_dynamite_plunger(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:341d, image 0x1a6dd - kind 22's drive, the same family as kind 31's at
 * 172c:2e4b: the mode masked to 0x8006 and then to 0x7fff, which leaves 2, 4
 * or 6 and drops the top bit, and the second test asking about the *0x8006*
 * value so a 4 with the top bit on takes neither arm.
 *
 * Mode 2 always answers yes and mode 4 answers yes once the form has reached
 * 2. Failing that, a plain 4 sets +0x12 going if it is not going already, and
 * answers 0 - so this is the drive that starts a kind 22 rather than reporting
 * on it.
 */
uint16_t part_drive_341d(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct belt *di = BELT_PTR(p2->belt_ptr[0]);
    uint16_t low;                       /* cx */

    if (p4 == 1) {
        di->v[0]++;
        return 0;
    }

    p4 &= 0x8006;
    low = p4 & 0x7fff;

    if (low == 2)
        goto yes;
    if (low == 4 && p2->form == 2) {
yes:
        return 1;
    }

    if (p4 == 4 && p2->direction == 0)
        p2->direction = 1;

    return 0;
}

/*
 * 172c:346f, image 0x1a72f - a setup.
 *
 * Five points: a flat-bottomed shape with a peak in the middle of its top.
 */
void part_setup_mort_the_mouse(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 0;
    si->y = 6;
    si++;
    si->x = 12;
    si->y = 0;
    si++;
    si->x = 23;
    si->y = 6;
    si++;
    si->x = 23;
    si->y = 10;
    si++;
    si->x = 0;
    si->y = 10;

    part_finish_angles(part);
}

/*
 * 172c:34b5, image 0x1a775 - kind 42's hit test.
 *
 * It reads the thing that hit it and does nothing with it: the mouse is solid
 * and that is all. Answers 1.
 */
uint16_t part_hit_mort_the_mouse(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);   /* read, and never used */

    (void)other;
    return 1;
}

/*
 * 172c:34d0, image 0x1a790 - kind 42's step. The mouse.
 *
 * It runs when it is startled and then stops. The countdown at +0x96 is how
 * many steps of running are left; each one flips the form between 0 and 1 and
 * moves it three or four pixels the way its mirror bit points - four on the
 * odd frame and three on the even, which is what makes the gait uneven.
 *
 * With the countdown spent it waits for a touch - bit 0 of +6 - and then looks
 * for a kind-0x0c anywhere in a box 0x80 either side and 8 below,
 * `link_nearby_objects` building the candidates. The slowest one it finds
 * decides which way it runs: something moving right sends it left and clears
 * the mirror bit, anything else sends it right. Five steps of running, and a
 * form of 1 to start.
 *
 * A form that has changed is carried into the sixteenths at +0x16 and drawn.
 */
void part_step_mort_the_mouse(struct part *part)
{
    struct part *di;
    int16_t slowest;                    /* [bp-2] */
    int16_t step;                       /* [bp-4] */

    if (part->word_96 != 0) {
        part->word_96--;
        part->form ^= 1;

        if (part->form != 0)
            step = 4;
        else
            step = 3;

        if (part->flags_08 & 0x10)
            part->pos[0].x += step;
        else
            part->pos[0].x -= step;
    } else if (part->flags_06 & 1) {
        link_nearby_objects(part, 0x1000, (int16_t)0xff80, 0x80, -8, 8);

        slowest = 0x190;

        for (di = PART_PTR(part->next_linked_ptr); di != PART_NONE;
             di = PART_PTR(di->next_linked_ptr))
            if (di->kind == KIND_POKEY && abs(di->link_dx) < abs(slowest))
                slowest = di->link_dx;

        if (slowest != 0x190) {
            part->form = 1;
            part->word_96 = 5;

            if (slowest > 0) {
                part->flags_08 &= 0xffef;
                part->pos[0].x -= 3;
            } else {
                part->flags_08 |= 0x10;
                part->pos[0].x += 3;
            }
        }
    }

    if (part->form != part->form_prev) {
        part->fx = part->pos[0].x;
        part->fx <<= 9;
        place_object_for_draw(part);
    }
}

/*
 * 172c:35c7, image 0x1a887 - kind 42's flip, the same as kind 30's.
 */
void part_flip_mort_the_mouse(struct part *part)
{
    part->flags_08 ^= 0x10;
    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/* 172c:35f4, image 0x1a8b4 - a setup: eight points from DGROUP 0x3422. */
void part_setup_pumpkin(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = PARTSHAPES.s_3422;
    for (i = 0, si = POINTS(part->points_ptr); i < 8; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:3635, image 0x1a8f5 - kind 36's step. The kicker.
 *
 * It forgets whatever it was touching - +0x84 to zero - and starts itself once
 * its counter passes 0x14. Then it runs its frames, wrapping 0x0a back to 7 so
 * the last four loop; form 6 plays sound 0x0f, and from form 7 on it is
 * *lifting*, taking 0x400 off its own downward velocity every step.
 *
 * From form 7 it also reaches out, the same way the paddle wheel does: a point
 * match first, switching on everything it finds, and then in the odd frames a
 * box match for a kind 4 to switch on and a cat to wake.
 */
void part_step_rocket(struct part *part)
{
    struct part *di;

    part->contact_ptr = 0;

    if (part->direction == 0 && part->spin > 0x14)
        part->direction = 1;

    if (part->direction != 0) {
        part->form++;
        if (part->form == 0x0a)
            part->form = 7;

        if (part->form == 6)
            play_sound(0x0f);

        if (part->form >= 7) {
            part->vel_y -= 0x400;
            clamp_record_pair(part);
        }

        place_object_for_draw(part);

        if (part->form >= 7) {
            link_objects_at_point(part, -4, 0x12, 0x30, 0x51);

            for (di = PART_PTR(part->next_linked_ptr); di != PART_NONE;
                 di = PART_PTR(di->next_linked_ptr))
                if (di->direction == 0)
                    di->direction = 1;

            if (part->form & 1) {
                link_objects_in_range(part, 0x1000, -4, 0x12, 0x30, 0x51);

                for (di = PART_PTR(part->next_linked_ptr); di != PART_NONE;
                     di = PART_PTR(di->next_linked_ptr)) {
                    if (di->kind == KIND_BALLOON) {
                        di->direction = 1;
                    } else if (di->kind == KIND_POKEY && di->form == 0) {
                        di->form = 1;
                        di->word_96 = 0;
                        place_object_for_draw(di);
                        play_sound(7);
                    }
                }
            }
        }
    }
}

/*
 * 172c:3737, image 0x1a9f7 - a setup.
 *
 * Four points and a grab box, and it is tall and narrow - 14 by 51 - so the
 * two top corners are inset where the bottom two are not.
 */
void part_setup_rocket(struct part *part)
{
    struct part_point *si;

    part->hold.x = 0x0b;
    part->hold.y = 0x3c;

    si = POINTS(part->points_ptr);
    si->x = 4;
    si->y = 0;
    si++;
    si->x = 10;
    si->y = 0;
    si++;
    si->x = 14;
    si->y = 51;
    si++;
    si->x = 0;
    si->y = 51;

    part_finish_angles(part);
}

/*
 * 172c:377b, image 0x1aa3b - kind 47's setup. The corner pipe.
 *
 * Eight points, from one of four tables chosen by comparing the form against
 * 0, 1 and 2 one at a time - `cmp [si+0xc], 0 / jne` and so on - with the
 * fourth for anything else.
 */
void part_setup_corner_pipe(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->form == 0)
        src = PARTSHAPES.s_3432;
    else if (part->form == 1)
        src = PARTSHAPES.s_3442;
    else if (part->form == 2)
        src = PARTSHAPES.s_3452;
    else
        src = PARTSHAPES.s_3462;

    for (i = 0, dst = POINTS(part->points_ptr); i < 8; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:37e5, image 0x1aaa5 - kind 47's flip, and **the only one that reads the
 * second argument**.
 *
 * Every other flip in this segment takes the part alone. This one tests
 * [bp+8], which `part_flip_options` passes as 1 for the X key and 2 for the Y
 * key, and turns over bit 0 of the form at +0x0c for X and bit 1 for Y. So the
 * part has two independent axes held in one word, which is what X and Y
 * flipping separately means for it.
 *
 * The port's old flip dispatcher had `(void)which` and threw that away, so
 * this kind would have flipped the same axis whichever key was pressed. The three
 * flips written before this one were re-read to check they really do take the
 * part alone; they do.
 */
void part_flip_corner_pipe(struct part *part, uint16_t which)
{
    if (which == 1)
        part->form ^= 1;
    else
        part->form ^= 2;

    part->start_form = part->form;

    part_setup_corner_pipe(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:3824, image 0x1aae4 - kind 37's hit test. Closing the scissors.
 *
 * Four of the eight faces set the thing that hit it going, and *which* four
 * depends on the mirror bit of the thing itself, not of the scissors: 1, 2, 4
 * and 5 mirrored, 0, 1, 5 and 6 not. One more face - 7 mirrored, 3 not - sets
 * the *scissors* going instead, and only when they were hit by a kind 4.
 *
 * It answers 1 whatever happened.
 */
uint16_t part_hit_scissors(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);
    int16_t face = ((int16_t)part->contact_edge);

    if (other->flags_08 & 0x10) {
        if (face == 1 || face == 2 || face == 4 || face == 5)
            other->direction = 1;
        else if (face == 7 && part->kind == KIND_BALLOON)
            part->direction = 1;
    } else {
        if (face == 0 || face == 1 || face == 5 || face == 6)
            other->direction = 1;
        else if (face == 3 && part->kind == KIND_BALLOON)
            part->direction = 1;
    }

    return 1;
}

/*
 * 172c:389b, image 0x1ab5b - a setup.
 *
 * Eight points, reached the way `part_setup_bellow` reaches its six: bit 4 of +8
 * picks between the **pointer arrays** at DGROUP 0x34b6 and 0x3492, and the
 * form at +0x0c indexes the one picked. The load is a word, so what is indexed
 * is an array of near pointers and not the points themselves.
 */
void part_setup_scissors(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10)
        src = POINT_TABLE(PARTSHAPES.o_34b6[part->form]);
    else
        src = POINT_TABLE(PARTSHAPES.o_3492[part->form]);

    for (i = 0, dst = POINTS(part->points_ptr); i < 8; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:38fc, image 0x1abbc - kind 37's step. The scissors.
 *
 * They cut once: only in form 0, and only while +0x12 says go. The line they
 * cut along is one of two in DGROUP - 0x34c2 mirrored, 0x34ba not - and
 * `cut_belts` does the work. Then the form steps, its own setup runs again
 * because the shape has changed, and sound 0x10 plays.
 */
void part_step_scissors(struct part *part)
{
    if (part->direction != 0 && part->form == 0) {
        cut_belts(part, (part->flags_08 & 0x10) ? PARTSHAPES.cut_line[1]
                                                : PARTSHAPES.cut_line[0]);

        part->form++;
        part_setup_scissors(part);
        place_object_for_draw(part);
        play_sound(0x10);
    }
}

/*
 * 172c:3944, image 0x1ac04 - kind 37's flip: bit 4, its setup, two marks.
 */
void part_flip_scissors(struct part *part)
{
    part->flags_08 ^= 0x10;
    part_setup_scissors(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:3970, image 0x1ac30
 *
 * Cut every belt that crosses a line.
 *
 * The list at DGROUP 0x521b is walked for belts - kind 0x0a - and each one's
 * lengths in turn: from the part the record names at +2, along the chain of
 * pulleys, to the part at +4. Each length is turned into two points in the
 * scissors' own frame and handed to `intersect_segments`.
 *
 * A cut makes three parts: two kind-0x31 anchors, both at the point the cut
 * fell, and a kind-0x0a to carry the second half of the belt. If any of the
 * three cannot be had the ones already made are given back and nothing is cut -
 * so a machine that runs out of memory keeps its belt whole rather than losing
 * half of it.
 *
 * The rethreading is the fiddly part. The old belt record keeps the near half
 * and ends at the first anchor; the new one takes the second anchor and the old
 * far end, and the pulley the cut length was heading for is pointed at it. A
 * pulley - kind 7 - is repointed through its own +0x68 and +0x5c rather than
 * through the slot the walk was using, because a pulley's two links are not
 * interchangeable.
 *
 * Both anchors get their positions wound into sixteenths, both belts have their
 * geometry refreshed with the machine forced into state 0x1000, and the walk
 * ends: a belt is only cut once per pass.
 */
void cut_belts(struct part *part, const int16_t *line)
{
    struct part *di;                    /* the near anchor */
    int16_t k;                          /* [bp-2] */
    int16_t slotA;                      /* [bp-4] */
    int16_t slotB;                      /* [bp-6] */
    int16_t saved;                      /* [bp-8] */
    int16_t at[2];                      /* [bp-0xc] */
    int16_t seg[4];                     /* [bp-0x14] */
    struct part *rec;                   /* [bp-0x16] */
    struct part *prev;                  /* [bp-0x18] */
    struct part *next;                  /* [bp-0x1a] */
    struct part *anchorB;               /* [bp-0x1c] */
    struct part *carrier;               /* [bp-0x1e] */
    struct part *endA;                  /* [bp-0x20] */
    struct part *endB;                  /* [bp-0x22] */
    struct belt *belt;                  /* [bp-0x24] */
    struct belt *newbelt;               /* [bp-0x26] */

    for (rec = PART_PTR(DG521B.placed_parts.next_ptr); rec != PART_NONE;
         rec = PART_PTR(rec->next_ptr)) {
        if (rec->kind != KIND_ROPE)
            continue;

        belt = BELT_PTR(rec->belt_ptr[0]);
        prev = endA = PART_PTR(belt->end_a_ptr);
        endB = PART_PTR(belt->end_b_ptr);
        slotA = belt->slot_a;
        slotB = 0;
        next = PART_PTR(prev->link_ptr[slotA]);

        while (prev != PART_NONE && next != PART_NONE) {
            if (prev != endA)
                slotA = 1;

            seg[0] = prev->box[0].x + prev->attach[slotA].x - part->pos[0].x;
            seg[1] = prev->box[0].y + prev->attach[slotA].y - part->pos[0].y;

            if (next == endB)
                slotB = belt->slot_b;

            seg[2] = next->box[0].x + next->attach[slotB].x - part->pos[0].x;
            seg[3] = next->box[0].y + next->attach[slotB].y - part->pos[0].y;

            if (intersect_segments(line, seg, (uint8_t *)at) != 0) {
                saved = DG4E67.state;
                DG4E67.state = 0x1000;
                mark_belt_shapes(PART_PTR(belt->owner_ptr), 3);
                DG4E67.state = saved;

                if ((di = make_part(KIND_ANCHOR)) == PART_NONE)
                    goto out;
                if ((anchorB = make_part(KIND_ANCHOR)) == PART_NONE)
                    goto fail;
                if ((carrier = make_part(KIND_ROPE)) == PART_NONE) {
                    free_part(anchorB);
fail:
                    free_part(di);
                    goto out;
                }

                insert_sorted(di, &DG5179.moving_parts);
                di->flags_06 |= 0x10;
                di->pos[0].x = at[0] + part->pos[0].x;
                di->pos[0].y = at[1] + part->pos[0].y;

                insert_sorted(anchorB, &DG5179.moving_parts);
                anchorB->flags_06 |= 0x10;
                anchorB->pos[0] = di->pos[0];

                insert_sorted(carrier, &DG521B.placed_parts);
                carrier->flags_06 |= 0x10;

                newbelt = BELT_PTR(carrier->belt_ptr[0]);
                newbelt->end_a_ptr = dg_near(dgroup, anchorB);
                newbelt->end_b_ptr = dg_near(dgroup, endB);
                newbelt->slot_a = 0;
                newbelt->slot_b = belt->slot_b;

                anchorB->link_ptr[0] = dg_near(dgroup, next);
                anchorB->belt_ptr[0] = dg_near(dgroup, newbelt);

                if (next->kind == KIND_PULLEY) {
                    next->belt_ptr[1] = dg_near(dgroup, newbelt);
                    next->link_ptr[1] = dg_near(dgroup, anchorB);
                } else {
                    next->belt_ptr[slotB] = dg_near(dgroup, newbelt);
                    next->link_ptr[slotB] = dg_near(dgroup, anchorB);
                }

                endB->belt_ptr[newbelt->slot_b] = dg_near(dgroup, newbelt);

                belt->end_b_ptr = dg_near(dgroup, di);
                belt->slot_b = 0;
                di->link_ptr[0] = dg_near(dgroup, prev);
                di->belt_ptr[0] = dg_near(dgroup, belt);

                if (prev->kind == KIND_PULLEY)
                    prev->link_ptr[0] = dg_near(dgroup, di);
                else
                    prev->link_ptr[slotA] = dg_near(dgroup, di);

                di->pos[2].x = di->pos[1].x = di->pos[0].x;
                di->fx = di->pos[0].x;
                di->fx <<= 9;
                di->pos[2].y = di->pos[1].y = di->pos[0].y;
                di->fy = di->pos[0].y;
                di->fy <<= 9;
                place_object_for_draw(di);

                anchorB->pos[2].x = anchorB->pos[1].x = anchorB->pos[0].x;
                anchorB->fx = anchorB->pos[0].x;
                anchorB->fx <<= 9;
                anchorB->pos[2].y = anchorB->pos[1].y = anchorB->pos[0].y;
                anchorB->fy = anchorB->pos[0].y;
                anchorB->fy <<= 9;
                place_object_for_draw(anchorB);

                DG4E67.state = 0x1000;

                refresh_link_geometry(belt);
                for (k = 0; k < 2; k++)
                    belt->pt[2][k] = belt->pt[1][k] = belt->pt[0][k];

                refresh_link_geometry(newbelt);
                for (k = 0; k < 2; k++)
                    newbelt->pt[2][k] = newbelt->pt[1][k] = newbelt->pt[0][k];

                DG4E67.state = saved;
                prev = next = PART_NONE;
            } else if (next == endB) {
                prev = next = PART_NONE;
            } else {
                prev = next;
                next = PART_PTR(next->link_ptr[0]);
            }
        }
    }

out:
    ;
}

/*
 * 172c:3de5, image 0x1b0a5 - no slots at all, and no finish. It only turns
 * the two part numbers at +0x62 and +0x64 into two bits of the form at
 * +0x0c, so a part that was read off disk with those links set comes out in
 * the form that matches them.
 */
void part_setup_solar_panel(struct part *part)
{
    part->form = 0;
    if (part->link_ptr[4] != 0)
        part->form |= 1;
    if (part->link_ptr[5] != 0)
        part->form |= 2;
}

/*
 * 172c:3e08, image 0x1b0c8 - kind 38's step. **It looks around, but only every
 * eighth frame.**
 *
 * The frame counter at 0x4ea7 masked to 3 bits must read 4, so seven frames in
 * eight this does nothing but pass its state on. On the eighth it clears its
 * own +0x12 and asks `link_nearby_objects` for everything within 0x1a in each
 * direction; anything with a +0x12 of its own sets this part's, either
 * outright for kinds 0x1d, 0x2d and 0x29, or for kind 0x19 only when the sign
 * of its +0x7a and bit 4 of its flags **disagree** - so a kind-0x19 part
 * facing the wrong way is ignored.
 *
 * Then, every frame, +0x12 is passed to whatever the two words at +0x62 and
 * +0x64 point at. The loop runs its index from 4 to 5 over a table based at
 * +0x5a, which is those two and no others.
 */
void part_step_solar_panel(struct part *part)
{
    struct part *si;
    int16_t i;                          /* [bp-2] */

    part->flags_08 |= 0x40;

    if ((DG4E67.machine_frames & 7) == 4) {
        part->direction = 0;

        link_nearby_objects(part, 0x3000, -0x1a, 0x1a, -0x1a, 0x1a);

        for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
             si = PART_PTR(si->next_linked_ptr)) {
            if (si->direction != 0) {
                if (si->kind == KIND_LIGHT || si->kind == KIND_CANDLE
                    || si->kind == KIND_BLAST) {
                    part->direction = 1;
                } else if (si->kind == KIND_FLASHLIGHT) {
                    if (si->link_dx < 0) {
                        if (!(si->flags_08 & 0x10))
                            part->direction = 1;
                    } else {
                        if (si->flags_08 & 0x10)
                            part->direction = 1;
                    }
                }
            }
        }
    }

    for (i = 4; i < 6; i++)
        if ((si = PART_PTR(part->link_ptr[i])) != PART_NONE)
            si->direction = part->direction;
}

/*
 * 172c:3ebf, image 0x1b17f - kind 39's hit test.
 *
 * The hook belongs to the kind 39 part - `di`, at the arriving object's +0x84 -
 * and runs on whatever arrived, `si`.
 *
 * It only catches a thing that lands squarely on it: an object already spoken
 * for at +0x8a is refused, and so is one whose middle is more than 14 across
 * from the part's own middle, both by answering 1.
 *
 * Caught, the part is set going at +0x12 and the object is let go: a sideways
 * speed of 0x400 or more is halved, the link at +0x84 is dropped and the "in
 * contact" bit of +6 with it, and the sixteenths at +0x1a are rebuilt from the
 * whole pixels so the release leaves no fractional position behind.
 *
 * Form 3 - and only form 3 - throws it as well: the downward speed at +0x38
 * becomes minus its own size, less another 0x400, and `clamp_record_pair` holds
 * that to what the kind allows.
 */
uint16_t part_hit_trampoline(struct part *part)
{
    struct part *di = PART_PTR(part->contact_ptr);
    int16_t edge = part->contact_edge;  /* [bp-2] */
    int16_t apart;                      /* [bp-4] */

    if (edge == 0) {
        apart = part->pos[0].x + (part->size[0].width >> 1);
        apart -= di->pos[0].x + (di->size[0].width >> 1);

        if (apart < -0x0e || apart > 0x0e)
            return 1;

        di->direction = 1;

        if (abs(part->vel_x) >= 0x400)
            part->vel_x >>= 1;

        part->contact_ptr = 0;
        part->flags_06 &= 0xfffe;

        part->fy = part->pos[0].y;
        part->fy <<= 9;

        if (di->form == 3) {
            part->vel_y = 0 - abs(part->vel_y);
            part->vel_y -= 0x400;
            clamp_record_pair(part);
        }

        return 0;
    }

    return 1;
}

/*
 * 172c:3f72, image 0x1b232 - a setup.
 *
 * A wide box, 47 by 16, sitting 11 down from the part's origin.
 */
void part_setup_trampoline(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 0;
    si->y = 11;
    si++;
    si->x = 47;
    si->y = 11;
    si++;
    si->x = 47;
    si->y = 27;
    si++;
    si->x = 0;
    si->y = 27;

    part_finish_angles(part);
}

/*
 * 172c:3fae, image 0x1b26e - kind 39's step.
 *
 * Five frames once it is set going, the third playing sound 3, and reaching
 * the fifth wraps the form back to 0 and switches it off - so it plays through
 * and stops rather than looping.
 */
void part_step_trampoline(struct part *part)
{
    if (part->direction != 0) {
        part->form++;

        if (part->form == 3)
            play_sound(3);

        if (part->form == 5) {
            part->form = 0;
            part->direction = 0;
        }

        place_object_for_draw(part);
    }
}

/*
 * 172c:3fe8, image 0x1b2a8 - kind 3's hit test. Standing on the motor.
 *
 * A motor whose belt is held - bit 9 of +8 - answers 1 at once and does
 * nothing: it cannot be turned by being stood on.
 *
 * Otherwise the face that was touched, +0x8a of the thing that hit, decides.
 * Faces 0, 2 and 6 can turn it; anything else is a plain hit. Face 0 is the
 * top and is split by where along it the contact fell: past 0x2c is one end,
 * up to 0x24 the other, and between them nothing. Faces 2 and 6 are the sides
 * and turn it by which form it is in.
 *
 * Turning is not done here: `queue_part` asks for the motor to be stepped, and
 * only if the queue took it does the direction go across, with the asking
 * part's priority. A motor already on the queue at a better priority makes this
 * a plain hit instead - and the thing's own contact is cleared when the turn
 * was taken, so it does not also bounce.
 */
uint16_t part_hit_seesaw(struct part *part)
{
    struct part *si = PART_PTR(part->contact_ptr);
    int16_t face;                       /* [bp-2] */
    int16_t along;                      /* [bp-4] */
    int16_t dir;                        /* [bp-6] */
    int16_t plain;                      /* [bp-8] */

    if (si->flags_08 & 0x200)
        return 1;

    face = part->contact_edge;

    if (face == 0 || face == 2 || face == 6)
        plain = 0;
    else
        plain = 1;

    switch (face) {
    case 0:
        along = part->pos[0].x + (part->size[0].width >> 1) - si->pos[0].x;

        if (along >= 0x2c) {
            if (si->form == 2)
                plain = 1;
            else
                dir = 1;
        } else if (along <= 0x24) {
            if (si->form == 0)
                plain = 1;
            else
                dir = -1;
        } else {
            plain = 1;
        }
        break;
    case 2:
        if (si->form == 0)
            plain = 1;
        else
            dir = -1;
        break;
    case 6:
        if (si->form == 2)
            plain = 1;
        else
            dir = 1;
        break;
    }

    if (plain == 0) {
        if (queue_part(part, part->contact_ptr) != 0) {
            si->direction = dir;
            si->momentum = part->momentum;
            part->contact_ptr = 0;
        } else {
            plain = 1;
        }
    }

    return plain;
}

/*
 * 172c:40f0, image 0x1b3b0
 *
 * A part with **three forms**, and the word at +0x0c says which. Its four
 * bytes at +0x6a..+0x6d - the box it is grabbed by - come out of one table
 * indexed by that word, and its eight connection points out of one of three
 * others, chosen by the same word with a `switch`.
 *
 * Its tables are `point16` rather than `point8` - four bytes an entry with
 * the coordinate at +0 and +2 - which the image settles: every other byte in
 * all five of them is zero. The setup reads each with a byte move, so it takes
 * the low half of each word and the top halves are never looked at.
 *
 * A form other than 0, 1 or 2 leaves the point untouched rather than defaulting
 * to one of them: the `jmp` at the end of the switch goes to the loop's own
 * increment.
 */
void part_setup_seesaw(struct part *part)
{
    struct part_point *di;
    int16_t i;                          /* [bp-2] */

    part->attach[0].x = (uint8_t)PARTSHAPES.p_34ca[part->form].x;
    part->attach[0].y = (uint8_t)PARTSHAPES.p_34ca[part->form].y;
    part->attach[1].x = (uint8_t)PARTSHAPES.p_34d6[part->form].x;
    part->attach[1].y = (uint8_t)PARTSHAPES.p_34d6[part->form].y;

    for (i = 0, di = POINTS(part->points_ptr); i < 8; i++, di++) {
        switch (part->form) {
        case 0:
            di->x = (uint8_t)PARTSHAPES.p_34e2[i].x;
            di->y = (uint8_t)PARTSHAPES.p_34e2[i].y;
            break;
        case 1:
            di->x = (uint8_t)PARTSHAPES.p_3502[i].x;
            di->y = (uint8_t)PARTSHAPES.p_3502[i].y;
            break;
        case 2:
            di->x = (uint8_t)PARTSHAPES.p_3522[i].x;
            di->y = (uint8_t)PARTSHAPES.p_3522[i].y;
            break;
        }
    }

    part_finish_angles(part);
}

/*
 * 172c:41bb, image 0x1b47b - kind 3's flip, the 0-or-2 form swing with all
 * four redraws behind it.
 */
void part_flip_seesaw(struct part *part)
{
    if (part->form != 0)
        part->form = 0;
    else
        part->form = 2;

    part->start_form = part->form;

    part_setup_seesaw(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:420f, image 0x1b4cf - kind 3's step. The motor.
 *
 * Nothing happens unless +0x12 says it is on. Then it marks itself done - bit
 * 6 of +8 - and either turns freely, when bit 10 of +8 is set, or asks its
 * belts first: `drive_belts` twice, once with 0x8000 in the flags and once
 * without, and an answer from the first means the belt is being held, which
 * sets bit 9 of +8 and stops it turning. Otherwise the second call goes out
 * anyway and the form steps by the direction.
 *
 * A form that has changed runs `part_setup_seesaw` - the motor's connection
 * points move with it - and forms 0 and 2 play sound 0x12. Then it looks for
 * what its shaft is over: `link_objects_crossing_segments` gives it the
 * candidates and each is given a speed from `push_speed_for_mass`, signed by
 * which side of the shaft's middle it lies. Each one is then dropped 0x10 and
 * lifted 0x10 through `resolve_collisions` to settle it, with the motor hidden
 * for the second of the two so it does not collide with itself.
 *
 * Finally, at form 0 with the last form non-zero, it reaches out once more:
 * `link_objects_in_range` over a box 0x4a to 0x4f across and 2 down, and
 * `trigger_things_at` sets going whatever is there.
 */
void part_step_seesaw(struct part *part)
{
    struct part *di;
    int16_t v02;                        /* [bp-2] the shaft's middle */
    int16_t v04;                        /* [bp-4] the other's middle */
    int16_t v06;                        /* [bp-6] the speed */
    int32_t fy;                         /* [bp-0xa] */

    if (part->direction != 0) {
        part->flags_08 |= 0x40;

        if (part->flags_08 & 0x400)
            part->form += part->direction;
        else if (drive_belts(PART_NONE, part, 0x8000, 0x3e8, part->momentum) != 0)
            part->flags_08 |= 0x200;
        else {
            drive_belts(PART_NONE, part, 0, 0x3e8, part->momentum);
            part->form += part->direction;
        }

        if (part->form != part->form_prev) {
            part_setup_seesaw(part);

            if (part->form_prev == 0 || part->form_prev == 2)
                play_sound(0x12);

            place_object_for_draw(part);

            v02 = part->pos[0].x + (part->size[0].width >> 1);

            link_objects_crossing(part, 0x1000, PARTSHAPES.shaft_line[part->form]);

            for (di = PART_PTR(part->next_linked_ptr); di != PART_NONE;
                 di = PART_PTR(di->next_linked_ptr)) {
                v04 = di->pos[0].x + (di->size[0].width >> 1);
                v06 = push_speed_for_mass(di);

                switch (part->direction) {
                case -1:
                    if (v04 < v02) {
                        di->vel_y = v06;
                        di->vel_x = 0 - (v06 >> 2);
                    } else {
                        di->vel_y = 0 - v06;
                        di->vel_x = v06 >> 2;
                    }
                    break;
                case 1:
                    if (v04 < v02) {
                        di->vel_y = 0 - v06;
                        di->vel_x = 0 - (v06 >> 2);
                    } else {
                        di->vel_y = v06;
                        di->vel_x = v06 >> 2;
                    }
                    break;
                }

                mark_part_shapes(di, 3);

                if (di->vel_y < 0) {
                    di->pos[1].y = di->pos[0].y - 0x10;
                    resolve_collisions(di);

                    di->pos[1].y = di->pos[0].y + 0x10;
                    part->flags_08 |= 0x2000;
                    resolve_collisions(di);
                    part->flags_08 &= 0xdfff;

                    di->pos[1].y = di->pos[0].y;

                    fy = di->pos[0].y;
                    di->fy = fy << 9;
                } else {
                    di->pos[1].y = di->pos[0].y + 0x10;
                    resolve_collisions(di);

                    di->pos[1].y = di->pos[0].y - 0x10;
                    part->flags_08 |= 0x2000;
                    resolve_collisions(di);
                    part->flags_08 &= 0xdfff;

                    di->pos[1].y = di->pos[0].y;

                    fy = di->pos[0].y;
                    di->fy = ((fy + 1) << 9) - 1;
                }
            }
        }

        part->direction = 0;
        part->momentum = 0;
    }

    /*
     * The two ends of the stroke reach out, in opposite pairs: at form 0 the
     * near side of the shaft is at 0x4a..0x4f across and level, and the far
     * side at 0..6 and 0x20..0x24 down; at form 2 the two swap over. Each box
     * is followed by a `trigger_things_at` for the point it was measured from.
     */
    if (part->form_prev == 0 && part->form_prev2 != 0) {
        link_objects_in_range(part, 0x2000, 0x4a, 0x4f, -2, 2);
        trigger_things_at(part, 0, 0x4a);

        link_objects_in_range(part, 0x2000, 0, 6, 0x20, 0x24);
        trigger_things_at(part, 1, 0);
    } else if (part->form_prev == 2 && part->form_prev2 != 2) {
        link_objects_in_range(part, 0x2000, 0x4a, 0x4f, 0x20, 0x24);
        trigger_things_at(part, 1, 0x4a);

        link_objects_in_range(part, 0x2000, 0, 6, -2, 2);
        trigger_things_at(part, 0, 0);
    }
}

/*
 * 172c:44fe, image 0x1b7be - kind 3's drive, and the longest of them.
 *
 * `p3` picks **which of a pair** of pointers at +0x66 to work through - it is
 * doubled and used as an index - so this kind has two ends and is driven at
 * each independently.
 *
 * The mode is masked to 0x8007 and then to 0x7fff; the 0x8000 bit is kept
 * apart and carried into `drive_belts` and tested again afterwards, so it is
 * "ask, do not act". Away from mode 1, a non-zero counter at +0x0e of the
 * chosen pointer is decremented and the answer is 0 - unless the 0x8000 bit is
 * set, when it is left alone. That is the "already busy" path.
 *
 * Modes 2 and 4 are the two directions, and each asks whether the form at
 * +0x0c is already at the end it would be driven to: at that end `di` is set
 * and nothing is driven, otherwise +0x12 is loaded with 1 or -1 and
 * `drive_belts` is asked to carry it. `p3` swaps which end counts, which is
 * what makes the two ends opposite.
 *
 * Afterwards the 0x8000 bit puts +0x12 back to what it was, and a `drive_belts`
 * that answered nothing sets bit 10 of +8. Bit 9 is set when `di` is non-zero,
 * and bit 9 being set at the end is the answer 1.
 *
 * **A local is read before it is written.** [bp-2] is only set inside the mode
 * 2 and mode 4 arms, and a mode that is neither - anything but 1, 2 and 4 -
 * reaches `[si+0x12] = [bp-2]` with whatever was on the stack. The port cannot
 * reproduce an uninitialised DOS stack and does not try; `drive` here starts at
 * zero, which is the one value that is certainly wrong in the same way for
 * every run rather than differently each time. Recorded because it is a real
 * difference and not a transcription slip.
 */
uint16_t part_drive_44fe(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    int16_t  di;
    int16_t  drive;                     /* [bp-2] */
    int16_t  was;                       /* [bp-4] */
    uint16_t mode;                      /* [bp-6] */
    struct belt *chain;                 /* [bp-8] */

    chain = BELT_PTR(p2->belt_ptr[p3]);
    p4 &= 0x8007;
    mode = p4 & 0x7fff;

    if (mode != 1 && chain->v[0] != 0) {
        if (!(p4 & 0x8000))
            chain->v[0]--;
        return 0;
    }

    was = p2->direction;
    di = 0;
#ifndef __TURBOC__
    drive = 0;      /* the original reads [bp-2] unset here: see above */
#endif

    if (mode == 4) {
        if (p3 == 0) {
            if (p2->form == 0)
                di = 1;
            else
                drive = -1;
        } else {
            if (p2->form == 2)
                di = 1;
            else
                drive = 1;
        }
    } else if (mode == 2) {
        if (p3 == 0) {
            if (p2->form == 2)
                di = 1;
            else
                drive = 1;
        } else {
            if (p2->form == 0)
                di = 1;
            else
                drive = -1;
        }
    }

    if (di == 0 && mode != 1) {
        p2->direction = drive;

        di = drive_belts(p1, p2, p4 & 0x8000, p5, p6);

        if (p4 & 0x8000)
            p2->direction = was;
        else if (di == 0)
            p2->flags_08 |= 0x400;
    }

    if (di != 0)
        p2->flags_08 |= 0x200;

    if (p2->flags_08 & 0x200)
        return 1;

    if (p4 == 1)
        chain->v[0]++;

    return 0;
}

/*
 * 172c:461a, image 0x1b8da
 *
 * Push a part's motion out along its belts, and answer whether anything
 * refused.
 *
 * Each of the two belts at +0x66 leads to another part, which
 * `select_field_2_or_4` names. The one the caller came *from* is skipped, which is
 * what stops the walk going back on itself. `belt_orientation` says how the
 * belt runs between them - which way round the tangent points are - and that,
 * or-ed with the caller's own flags, is handed on with the part.
 *
 * The handler is the far pointer at +0x36 of the *far* part's kind record, so
 * what happens next is that part's business and not this one's. A part already
 * marked with bit 9 of +8 answers 1 straight away, and the walk stops at the
 * first belt that answers anything at all.
 */
uint16_t drive_belts(struct part *from, struct part *part, uint16_t flags,
                     uint16_t a, int32_t momentum)
{
    dg_near_t si;                       /* the belt */
    int16_t  v02;                       /* [bp-2]    the belt */
    uint16_t v04;                       /* [bp-4]    the answer */
    int16_t  v06;                       /* [bp-6]    which end */
    uint16_t v08;                       /* [bp-8]    the near slot */
    uint16_t v0a;                       /* [bp-0xa]  the far slot */
    uint16_t v0c;                       /* [bp-0xc]  how the belt runs */
    int16_t  v0e;                       /* [bp-0xe]  which way */
    struct part *v10;                   /* [bp-0x10] the far part */

    if (part->flags_08 & 0x200)
        return 1;

    v04 = 0;

    for (v02 = 0; v02 < 2 && v04 == 0; v02++) {
        if ((si = part->belt_ptr[v02]) != 0) {
            v10 = PART_PTR(select_field_2_or_4(part, BELT_PTR(si)));
            if (v10 != from) {
                if (PART_PTR(BELT_PTR(si)->end_a_ptr) == part) {
                    v06 = 0;
                    v08 = BELT_PTR(si)->slot_a;
                    v0a = BELT_PTR(si)->slot_b;
                } else {
                    v06 = 1;
                    v08 = BELT_PTR(si)->slot_b;
                    v0a = BELT_PTR(si)->slot_a;
                }

                if (part->direction > 0) {
                    if (v08 == 0)
                        v0e = 0;
                    else
                        v0e = 1;
                } else {
                    if (v08 == 0)
                        v0e = 1;
                    else
                        v0e = 0;
                }

                v0c = belt_orientation(si, v06, v0e);
                v0c |= flags;

                v04 = PART_KINDS[v10->kind].drive(part, v10, v0a, v0c, a, momentum);
            }
        }
    }

    return v04;
}

/*
 * 172c:271f, image 0x1b9df
 *
 * The same ladder as `bounce_speed_for_mass`, one step longer at the light
 * end: 0x1c00 below a mass of 2, and the rest of the steps as before. The two
 * exist separately in the original and are kept separate here.
 */
int16_t push_speed_for_mass(struct part *obj)
{
    int16_t m;                          /* si */
    int16_t r;                          /* cx */

    m = PART_KINDS[obj->kind].weight;

    if (m < 0x0002)
        r = 0x1c00;
    else if (m < 0x0006)
        r = 0x1a00;
    else if (m < 0x000a)
        r = 0x1800;
    else if (m < 0x0015)
        r = 0x1600;
    else if (m < 0x0079)
        r = 0x1400;
    else if (m < 0x0097)
        r = 0x1200;
    else
        r = 0x1000;

    return r;
}

/*
 * 172c:277d, image 0x1ba3d
 *
 * Set going whatever is in the chain at +0x78, at a point `dx` from the part's
 * own position.
 *
 * The original dispatches on the kind through a jump table in its own code
 * segment - six kinds at 172c:4893 and six targets twelve bytes after them -
 * which is the compiler's `switch`, so the port writes it as one.
 *
 * Four of the six turn on only if the thing is within a window of the point,
 * and the window depends on the mirror bit: kind 0x10 at 0x36..0x3c or 0..8,
 * kind 0x25 at 0x19..0x25 or 0..0x0c, and kinds 0x19 and 0x16 the same but
 * only when `mode` is 1. The other two are handed to the routines that already
 * know what to do with them.
 */
void trigger_things_at(struct part *part, int16_t mode, int16_t dx)
{
    struct part *si;
    int16_t d;                          /* di */
    int16_t x;                          /* [bp-2] */

    x = part->pos[0].x + dx;

    for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
         si = PART_PTR(si->next_linked_ptr)) {
        d = x - si->pos[0].x;

        switch (si->kind) {
        case 0x10:
            if (si->flags_08 & 0x10) {
                if (d >= 0x36 && d <= 0x3c)
                    si->direction = 1;
            } else if (d >= 0 && d <= 8) {
                si->direction = 1;
            }
            break;
        case 0x06:
            trigger_mouse_cage(si);
            break;
        case 0x25:
            if (si->flags_08 & 0x10) {
                if (d >= 0x19 && d <= 0x25)
                    si->direction = 1;
            } else if (d >= 0 && d <= 0x0c) {
                si->direction = 1;
            }
            break;
        case 0x19:
            if (mode == 1) {
                if (si->flags_08 & 0x10) {
                    if (d >= 0x0d && d <= 0x18)
                        si->direction = 1;
                } else if (d >= 5 && d <= 0x10) {
                    si->direction = 1;
                }
            }
            break;
        case 0x16:
            if (mode == 1) {
                if (si->flags_08 & 0x10) {
                    if (d >= 0 && d <= 0x1f)
                        si->direction = 1;
                } else if (d >= 0x67 && d <= 0x87) {
                    si->direction = 1;
                }
            }
            break;
        case 0x0f:
            break_bob_the_fish(si);
            break;
        }
    }
}

/*
 * 172c:48ab, image 0x1bb6b - a setup.
 *
 * The bounding rectangle **inset by one**: (0,0), (W-1,0), (W-1,H-1),
 * (0,H-1). The subtraction is `add al, 0xff` in the original, which is the
 * same byte and is transcribed as the -1 it is.
 */
void part_setup_48ab(struct part *part)
{
    struct part_point *si;

    si = POINTS(part->points_ptr);
    si->x = si->y = 0;
    si++;
    si->x = part->size[0].width - 1;
    si->y = 0;
    si++;
    si->x = part->size[0].width - 1;
    si->y = part->size[0].height - 1;
    si++;
    si->x = 0;
    si->y = part->size[0].height - 1;

    part_finish_angles(part);
}

/*
 * 172c:48f7, image 0x1bbb7 - the settle shared by kinds 1, 46 and 48.
 *
 * **Which edge was dragged decides which way it is squared off.** DGROUP
 * 0x4e69 is the handle being dragged; 0x8003 is taken off it and the four
 * values that leaves index a jump table at cs:0x4967, whose four entries are
 * only two: 0 and 1 pin the height at 0x10, 2 and 3 pin the width. Anything
 * else falls through untouched.
 *
 * Then the dragged size becomes the real size, and three connection points -
 * +0x82 plus 4, plus 8 and plus 0x0c - take the width and height **less one**,
 * because a point sits inside the edge rather than on it.
 */
void part_settle_48f7(struct part *part)
{
    /* Three cursors four bytes apart - points 1, 2 and 3 of the part's own
       table. */
    struct part_point *p1;              /* dx */
    struct part_point *p2;              /* di */
    struct part_point *p3;              /* [bp-2] */

    switch (DG4E67.tool) {
    case 0x8003:
    case 0x8004:
        part->set_size.height = 0x10;
        break;
    case 0x8005:
    case 0x8006:
        part->set_size.width = 0x10;
        break;
    }

    part->size[0].width = part->set_size.width;
    part->size[0].height = part->set_size.height;

    p1 = POINTS(part->points_ptr) + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;

    p1->x = p2->x = part->size[0].width - 1;
    p2->y = p3->y = part->size[0].height - 1;
}

/*
 * 172c:496f, image 0x1bc2f - a setup.
 *
 * Three points, a tall triangle - the peak at y 17 and the base at 47.
 */
void part_setup_windmill(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 8;
    si->y = 47;
    si++;
    si->x = 18;
    si->y = 17;
    si++;
    si->x = 28;
    si->y = 47;

    part_finish_angles(part);
}

/*
 * 172c:49a1, image 0x1bc61 - kind 40's step.
 *
 * A countdown at +0x9c: while it is running the part is "on", which it says in
 * the word at +0x12 and passes to whatever its rope is tied to - as 1, or -1
 * when bit 4 of its flags at +8 is set, which is the mirrored form. The other
 * end is only told if it is not already busy, bit 11 of its own +8.
 *
 * Being on also steps the form at +0x0c round the four frames, and a form that
 * has changed since the last one drawn is handed to `place_object_for_draw`.
 *
 * The original leaves AX as whatever fell out; nothing reads it.
 */
void part_step_windmill(struct part *part)
{
    struct part *di;

    part->direction = 0;

    if (part->spin != 0) {
        part->spin--;
        if (part->spin != 0)
            part->direction = 1;
    }

    if ((di = rope_other_end(part)) != PART_NONE && !(di->flags_08 & 0x800)) {
        if (part->direction != 0) {
            if (part->flags_08 & 0x10)
                di->direction = -1;
            else
                di->direction = 1;
        } else {
            di->direction = 0;
        }
    }

    if (part->direction != 0) {
        if (part->form == 3)
            part->form = 0;
        else
            part->form++;
    }

    if (part->form != part->form_prev)
        place_object_for_draw(part);
}

/*
 * 172c:4a22, image 0x1bce2 - kind 40's flip: bit 4 and three marks, with
 * **neither a setup nor a draw**. The leanest of the twenty.
 */
void part_flip_windmill(struct part *part)
{
    part->flags_08 ^= 0x10;
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
