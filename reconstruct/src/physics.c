/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Motion**: a kind's physics recomputed from the knobs, gravity and air,
 * moving a part by its speed, and what happens when it lands on or strikes
 * another - friction, a bounce off a surface, and two moving parts meeting.
 *
 * The original's **code segment 0000** (`_TEXT`), image 0x02ac0..0x03566,
 * split out of machine.c on 2026-09-27. **Both ends of this file are ours**,
 * for the reason score.c gives: no data, and no call reaching back across
 * either end, so where its module begins and ends leaves no byte to read.
 * Inside it the routines from 0x02bcc to 0x03201 call each other with bare
 * `push cs / call`, so those at least shared a file.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x02ac0
 *
 * Recompute the gravity and the velocity limit for **every kind** - all 0x3a
 * of them - from two settings at DGROUP 0x50b3 and 0x50b5. This is what a
 * change of speed or gravity in the game's controls has to run.
 *
 * Both settings are first bent by a piecewise rule: the one at 0x50b5 is
 * quartered and incremented below 0x8c, doubled above 0x116, and left alone
 * between; the one at 0x50b3 is halved below 0x46 and multiplied by sixteen
 * otherwise. Neither is a smooth curve and both are transcribed as the three
 * cases they are.
 *
 * The gravity is then a ratio, computed in 32 bits through `mul16x16` and the
 * runtime's long divide, and **which way round** depends on the comparison: a
 * kind heavier than the setting gets `s - base*s/v`, a lighter one gets
 * `v*s/base - s`, and an equal one gets zero. So the sign falls out of the
 * order rather than from a negation.
 *
 * Two kinds are special-cased by index: 0x14 and 0x2b take a fixed limit of
 * 0x3000, and 0x14 also has its gravity forced back to zero - after it has
 * just been computed.
 */
void recompute_kind_physics(void)
{
    int16_t i;                          /* [bp-2] */
    int16_t g;                          /* [bp-4] */
    int16_t v;                          /* [bp-6] */
    int16_t base;                       /* [bp-8] */
    int32_t q;                          /* [bp-0xc] */
    struct part_kind *k;
    register int16_t s;

    s = g_level_settings.air;
    if (s < 0x8c) {
        s >>= 2;
        s++;
    } else if (s > 0x116)
        s <<= 1;

    base = g_level_settings.gravity;
    if (base < 0x46)
        base >>= 1;
    else
        base <<= 4;

    for (i = 0; i < 0x3a; i++) {
        k = &g_part_kinds[i];
        v = k->density;
        if (v == base)
            g = 0;
        else if (v > base) {
            q = mul16x16(base, s);
            q /= v;
            g = q;
            g = s - g;
        } else {
            q = mul16x16(v, s);
            q /= base;
            g = q;
            g -= s;
        }
        k->gravity = g;

        if (i == 0x14 || i == 0x2b) {
            k->max_speed = 0x3000;
            if (i == 0x14)
                k->gravity = 0;
        } else
            k->max_speed = 0x2600 - base;
    }
}

/*
 * 0x02bcc
 *
 * Clamp the two signed words at +0x36 and +0x38 of a record to plus or minus
 * a limit that depends on the record's kind.
 *
 * The kind is the word at +4, and it indexes a table of 0x3a-byte entries
 * starting at DGROUP 0xea6; the limit is the word at +0x0a of that entry. The
 * negative bound is computed as `0 - limit` each time rather than kept, so a
 * limit of 0 pins the field to 0.
 *
 * The entry address is recomputed into BX before every single access in the
 * original - six times - and that is transcribed rather than hoisted.
 */
void clamp_record_pair(struct part *rec)
{
    const struct part_kind *k;

    k = &g_part_kinds[rec->kind];
    if (rec->vel_y > k->max_speed)
        rec->vel_y = k->max_speed;
    else if (rec->vel_y < 0 - k->max_speed)
        rec->vel_y = 0 - k->max_speed;

    if (rec->vel_x > k->max_speed)
        rec->vel_x = k->max_speed;
    else if (rec->vel_x < 0 - k->max_speed)
        rec->vel_x = 0 - k->max_speed;
}

/*
 * 0x02c39
 *
 * Apply the kind's gravity to a record's vertical velocity, clamp both axes,
 * and work out a speed.
 *
 * The gravity is the word at +8 of the kind entry, added to the velocity at
 * +0x38; `clamp_record_pair` then holds both axes inside the kind's limit.
 *
 * The speed is **Manhattan**, not Euclidean: the absolute values of the two
 * velocities are added - each with the branchless `cwd / xor / sub` - and the
 * sum multiplied by the record's own scale at +0x3a. The 32-bit product is
 * stored across +0x3c and +0x3e, low half first, so that pair is a long.
 */
void apply_gravity_and_speed(register struct part *rec)
{
    const struct part_kind *entry;

    entry = &g_part_kinds[rec->kind];
    rec->vel_y += entry->gravity;
    clamp_record_pair(rec);
    rec->momentum = mul16x16(abs(rec->vel_x) + abs(rec->vel_y), rec->weight);
}

/*
 * 0x02c83
 *
 * Add one to a part's vertical speed and take it off again - a routine that
 * leaves the part as it found it. Nothing in the image calls it: no near or
 * far call reaches 0x02c83 and no word holds its offset. The name is ours.
 */
void touch_vel_y(register struct part *rec)
{
    rec->vel_y++;
    rec->vel_y--;
}

/*
 * 0x02c93
 *
 * Advance an object one step: add its velocity to its position, apply gravity,
 * clamp, and work out where that puts it on screen.
 *
 * Position is a pair of 32-bit values at +0x16 and +0x1a carrying nine
 * fractional bits; velocity is the two 16-bit words at +0x36 and +0x38 that
 * `apply_contact_friction` writes, sign-extended before they are added. The
 * whole-number position is then those two shifted right by nine into +0x1e and
 * +0x20 - arithmetically, so a negative position stays negative.
 *
 * Gravity applies only with bit 0 set at +6, and its **direction comes from the
 * material record**: field +8 of the 0x3a-byte record at 0xea6 + 0x3a * type,
 * the same field `apply_contact_friction` reads as the normal load. Positive
 * pulls one way, anything else the other, always by 0x400 - which is
 * two whole units, given the nine fractional bits.
 *
 * Both axes are clamped to -1000..6000. A clamp does not just fix the
 * whole-number word: it rewrites the fixed-point value from the limit and
 * shifts it back up by nine, so the fraction is discarded rather than left
 * describing a position the object no longer has. Clamping only the visible
 * word would leave the two disagreeing and the object would creep.
 *
 * `place_object_for_draw` runs last, so a caller gets both the new position
 * and the new drawing position from one call.
 */
void integrate_object(register struct part *obj)
{
    obj->fx += obj->vel_x;
    obj->fy += obj->vel_y;

    if (obj->flags_06 & 1) {
        if (g_part_kinds[obj->kind].gravity > 0)
            obj->fy += 0x400;
        else
            obj->fy -= 0x400;
    }

    obj->pos[0].x = obj->fx >> 9;
    obj->pos[0].y = obj->fy >> 9;

    if (obj->pos[0].x < -1000) {
        obj->pos[0].x = -1000;
        obj->fx = -1000;
        obj->fx = (int32_t)((uint32_t)obj->fx << 9);
    } else if (obj->pos[0].x > 6000) {
        obj->pos[0].x = 6000;
        obj->fx = 6000;
        obj->fx = (int32_t)((uint32_t)obj->fx << 9);
    }

    if (obj->pos[0].y < -1000) {
        obj->pos[0].y = -1000;
        obj->fy = -1000;
        /* **At the field's own width.** `fx` and `fy` are the `int32_t` half
           of their union - the position in 9-bit fixed point - so the shift
           has to be a 32-bit one. Written as `(int16_t)((uint16_t)fy << 9)`
           this truncated twice and put **12288** here instead of -512000,
           and `pos_y` is `fy >> 9`: the part came back at y=24, rose off the
           top and was clamped again, over and over. The `shl` the original
           emits has no sign in it; shifting the value as unsigned at the same
           width is the same bits and is what C defines. */
        obj->fy = (int32_t)((uint32_t)obj->fy << 9);
    } else if (obj->pos[0].y > 6000) {
        obj->pos[0].y = 6000;
        obj->fy = 6000;
        obj->fy = (int32_t)((uint32_t)obj->fy << 9);
    }

    place_object_for_draw(obj);
}

/*
 * 0x02da0
 *
 * Apply contact friction to an object: work out how hard the surface it is
 * touching resists, and take that out of its velocity.
 *
 * The contact is described by the link at the object's own +0x84, whose first
 * word names the other object. Both objects' *types*, at +4, index a table of
 * 0x3a-byte material records at DGROUP 0xea6 - so this reads two records, one
 * per side of the contact.
 *
 * The contact angle is the link's +4, in the whole-turn-is-0x10000 space the
 * sine tables use. An angle of exactly 0 or 0x8000 - dead flat, either way up -
 * is nudged by 0x1000, a sixteenth of a turn, toward whichever side of the link
 * has a zero byte at +2 or +3. A flat contact has no direction to resolve
 * along, and the nudge gives it one.
 *
 * Grip is the larger of the two materials' +6, except that a type 5 object with
 * a non-zero +0x12 forces 0x100 regardless of either material.
 *
 * From there it is ordinary resolution into the contact frame: cosine and sine
 * of the *negated* angle, the normal load from |+8 of the first material|, and
 * a tangential term that is only counted when the object's own velocity and the
 * angle share a sign - pushing into the surface rather than away from it. Grip
 * times the sum of those two magnitudes, shifted down by 8, is the friction;
 * projected back through the cosine it becomes the amount to remove.
 *
 * A flag bit 0x20 at the object's +6 raises the floor of that amount from 2 to
 * 0x20, so some objects are held far more firmly than others.
 *
 * Friction opposes motion rather than reversing it: the subtraction is clamped
 * at zero from whichever side the velocity started on, so an object is brought
 * to rest and never pushed backwards.
 *
 * Finally +0x38 takes the perpendicular component, `clamp_record_pair` is run,
 * and the long at +0x1a is rebuilt from +0x20 shifted up by 9 - biased by one
 * before the shift and one after when the normal load is positive, which is a
 * rounding step and not a sign fix.
 */
void apply_contact_friction(register struct part *obj)
{
    int16_t angle;                      /* [bp-2] */
    int16_t step;                       /* [bp-4] */
    int16_t grip;                       /* [bp-6] */
    int16_t cos_a;                      /* [bp-8] */
    int16_t sin_a;                      /* [bp-0xa] */
    int16_t load;                       /* [bp-0xc] */
    int16_t aload;                      /* [bp-0xe] */
    int16_t normal;                     /* [bp-0x10] */
    int16_t tangent;                    /* [bp-0x12] */
    int16_t drag;                       /* [bp-0x14] */
    int16_t push;                       /* [bp-0x16] */
    int16_t perp;                       /* [bp-0x18] */
    int16_t nudge;                      /* [bp-0x1a] */
    int32_t q;                          /* [bp-0x1e] */
    int32_t dragl;                      /* [bp-0x22] */
    struct part *other;                 /* [bp-0x24] */
    struct part_contact *c;             /* [bp-0x26] */
    const struct part_kind *rec_a;      /* [bp-0x28] */
    const struct part_kind *rec_b;      /* [bp-0x2a] */
    int16_t v;

    c = (struct part_contact *)&obj->contact;
    other = c->part;
    rec_a = &g_part_kinds[obj->kind];
    rec_b = &g_part_kinds[other->kind];
    load = rec_a->gravity;
    angle = c->angle;

    if (angle == 0 || angle == (int16_t)0x8000) {
        if (!c->no_nudge_plus)
            angle += 0x1000;
        else if (!c->no_nudge_minus)
            angle -= 0x1000;
    }

    v = obj->vel_x;

    if (other->kind == 5 && other->direction != 0)
        grip = 0x100;
    else
        grip = rec_a->grip > rec_b->grip ? rec_a->grip : rec_b->grip;

    cos_a = angle_cos(0 - angle);
    sin_a = angle_sin(0 - angle);
    aload = abs(load);
    normal = mul16x16(cos_a, aload) >> 14;

    if ((v > 0 && angle > 0) || (v < 0 && angle < 0))
        tangent = mul16x16(sin_a, v) >> 14;
    else
        tangent = 0;

    dragl = mul16x16(abs(normal) + abs(tangent), grip);
    drag = dragl >> 8;
    push = mul16x16(cos_a, drag) >> 14;
    push = obj->flags_06 & 0x20 ? abs(push) + 0x20 : abs(push) + 2;

    perp = mul16x16(sin_a, aload) >> 14;
    nudge = mul16x16(abs(cos_a), perp) >> 14;
    v += nudge;

    if (v < 0) {
        step = v + push;
        if (step < 0)
            v = step;
        else
            v = 0;
    } else {
        step = v - push;
        if (step > 0)
            v = step;
        else
            v = 0;
    }

    obj->vel_x = v;
    if ((angle + 0x4000) & 0x8000)
        obj->vel_y = mul16x16(angle_sin(0 - angle), 0 - v) >> 14;
    else
        obj->vel_y = mul16x16(angle_sin(0 - angle), v) >> 14;
    clamp_record_pair(obj);

    q = obj->pos[0].y;
    obj->fy = load >= 0 ? (int32_t)((uint32_t)(q + 1) << 9) - 1
                        : (int32_t)((uint32_t)q << 9);
}

/*
 * 0x03009
 *
 * Play the impact sound if a kind-0 object hit hard enough: the two velocity
 * components at +0x36 and +0x38, each made positive and added, over 0x1000.
 * That is a sum of absolute values rather than a length, so a diagonal counts
 * for more than the same speed along one axis - which is the original's
 * arithmetic and not an approximation of anything.
 */
void sound_on_hard_impact(register struct part *obj)
{
    int16_t speed;

    if (obj->kind == KIND_BOWLING_BALL) {
        speed = abs(obj->vel_x) + abs(obj->vel_y);
        if (speed > 0x1000)
            play_sound(0x14);
    }
}

/*
 * 0x03046
 *
 * A bounce off a surface, rather than a slide along one.
 *
 * The contact record at +0x84 names what was hit at +4 and the angle of the
 * face it hit; that angle is turned by a quarter turn or back by one, by
 * whether the two bytes at +2 and +3 are set, so a corner reflects the way the
 * face it belongs to does.
 *
 * The velocity is rotated into the surface's frame, the component along the
 * face is kept, and the component into it is scaled by the *smaller* of the two
 * kinds' bounciness at +4 of their records - so a hard thing on a soft one
 * bounces as the soft one says - negated, and clamped: over 0x40 loses 0x40 and
 * under -0x40 gains it, otherwise it goes to zero. Then the pair is rotated
 * back.
 *
 * The position is carried into sixteenths afterwards, and the rounding is not
 * symmetric: a positive velocity or a positive gravity at +8 of the kind's
 * record rounds the *other* way, `(v + 1) << 9 - 1` rather than `v << 9`.
 */
void bounce_off_contact(register struct part *obj)
{
    int16_t angle;
    int16_t vx;                         /* [bp-2] */
    int16_t vy;                         /* [bp-4] */
    int16_t t;                          /* [bp-6] */
    int16_t bounce;                     /* [bp-8] */
    int32_t x;                          /* [bp-0xc] */
    int32_t y;                          /* [bp-0x10], a product first */
    struct part *what;                  /* [bp-0x12] */
    struct part_contact *c;             /* [bp-0x14] */
    const struct part_kind *mine;       /* [bp-0x16] */
    const struct part_kind *theirs;     /* [bp-0x18] */

    sound_on_hard_impact(obj);

    c = (struct part_contact *)&obj->contact;
    what = c->part;
    mine = &g_part_kinds[obj->kind];
    theirs = &g_part_kinds[what->kind];
    angle = c->angle;

    if (angle == 0 || angle == (int16_t)0x8000) {
        if (!c->no_nudge_plus)
            angle += 0x1000;
        else if (!c->no_nudge_minus)
            angle -= 0x1000;
    }

    vx = obj->vel_x;
    vy = obj->vel_y;
    rotate_point(&vx, &vy, angle);

    bounce = mine->bounce < theirs->bounce ? mine->bounce : theirs->bounce;
    y = mul16x16(vy, bounce);
    vy = y >> 8;
    vy = 0 - vy;

    if (vy < 0) {
        t = vy + 0x40;
        if (t < 0)
            vy = t;
        else
            vy = 0;
    } else {
        t = vy - 0x40;
        if (t > 0)
            vy = t;
        else
            vy = 0;
    }

    rotate_point(&vx, &vy, 0 - angle);
    obj->vel_x = vx;
    obj->vel_y = vy;
    clamp_record_pair(obj);

    x = obj->pos[0].x;
    obj->fx = vx >= 0 ? (int32_t)((uint32_t)(x + 1) << 9) - 1
                      : (int32_t)((uint32_t)x << 9);
    y = obj->pos[0].y;
    obj->fy = mine->gravity >= 0 ? (int32_t)((uint32_t)(y + 1) << 9) - 1
                                 : (int32_t)((uint32_t)y << 9);
}

/*
 * 0x03201
 *
 * Two moving things hit each other: share the momentum out between them.
 * `bounce_off_contact` is the same event against something that cannot move.
 *
 * Both velocities are turned into the frame of the line between the two
 * middles - `angle_between_centres` less a quarter turn - so that "x" means
 * along that line and "y" across it. Only the x halves are exchanged, by the
 * usual two-body formula over the two weights the kind records keep at +2:
 *
 *     mine  = (m*u + 2*n*v - n*u) / (m + n)
 *     yours = (2*m*u + n*v - m*v) / (m + n)
 *
 * built as 32-bit sums so a heavy thing at speed cannot wrap. Then both are
 * turned back and **halved**, which is where the energy goes.
 *
 * After that, a nudge apart, and the condition for it is three separate ways
 * of saying "these two are going to stay stuck": both left slower than 0x100,
 * or bit 0 of my +6, or bit 4 of my +0xa. Whichever it is, the one on the left
 * is given at least 0x200 leftwards and the one on the right at least 0x200
 * rightwards - unless bit 4 of my +0xa says the other one is not to be pushed.
 *
 * The bounciness at +4 of the two kind records - the smaller of the two - is
 * worked out and **never used**. It is a dead store in the original and is
 * transcribed as one; `bounce_off_contact` uses the same value for what looks
 * like the job this one was meant to do with it.
 */
void bounce_pair(register struct part *obj)
{
    struct part *other;
    int16_t angle;                      /* [bp-2] */
    int16_t bounce;                     /* [bp-4], never read */
    int16_t myW;                        /* [bp-6] */
    int16_t theirW;                     /* [bp-8] */
    int32_t total;                      /* [bp-0xc] */
    int16_t svx;                        /* [bp-0xe] */
    int16_t svy;                        /* [bp-0x10] */
    int16_t dvx;                        /* [bp-0x12] */
    int16_t dvy;                        /* [bp-0x14] */
    int16_t myMid;                      /* [bp-0x16] */
    int16_t theirMid;                   /* [bp-0x18] */
    int16_t apart;                      /* [bp-0x1a] */
    int32_t mine_u;                     /* [bp-0x1e] m*u */
    int32_t yours_v;                    /* [bp-0x22] n*v */
    int32_t yours_u;                    /* [bp-0x26] n*u */
    int32_t mine_v;                     /* [bp-0x2a] m*v */
    int32_t x;                          /* [bp-0x2e] */
    int32_t y;                          /* [bp-0x32] */
    const struct part_kind *mine;       /* [bp-0x34] */
    const struct part_kind *theirs;     /* [bp-0x36] */

    sound_on_hard_impact(obj);

    other = obj->contact;
    obj->flags_06 |= 8;
    other->flags_06 |= 8;
    mine = &g_part_kinds[obj->kind];
    theirs = &g_part_kinds[other->kind];
    bounce = mine->bounce < theirs->bounce ? mine->bounce : theirs->bounce;
    (void)bounce;
    myW = mine->weight;
    theirW = theirs->weight;

    svx = obj->vel_x;
    svy = obj->vel_y;
    dvx = other->vel_x;
    dvy = other->vel_y;

    angle = angle_between_centres(obj, other);
    angle -= 0x4000;
    rotate_point(&svx, &svy, angle);
    rotate_point(&dvx, &dvy, angle);

    total = myW;
    total += theirW;
    mine_u = mul16x16(myW, svx);
    yours_v = mul16x16(theirW, dvx);
    yours_u = mul16x16(theirW, svx);
    mine_v = mul16x16(myW, dvx);
    svx = (mine_u + yours_v + yours_v - yours_u) / total;
    dvx = (mine_u + mine_u + yours_v - mine_v) / total;

    rotate_point(&svx, &svy, 0 - angle);
    rotate_point(&dvx, &dvy, 0 - angle);

    obj->vel_x = svx >> 1;
    obj->vel_y = svy >> 1;
    other->vel_x = dvx >> 1;
    other->vel_y = dvy >> 1;

    apart = 0;
    if (abs(obj->vel_x) < 0x100 && abs(other->vel_x) < 0x100)
        apart = 1;
    if (obj->flags_06 & 1)
        apart = 1;
    if (obj->flags_0a & 0x10)
        apart = 1;

    if (apart) {
        myMid = obj->pos[0].x + (obj->size[0].width >> 1);
        theirMid = other->pos[0].x + (other->size[0].width >> 1);

        if (myMid < theirMid) {
            if (obj->vel_x > (int16_t)0xfe00)
                obj->vel_x = (int16_t)0xfe00;
            if (!(obj->flags_0a & 0x10) && other->vel_x < 0x200)
                other->vel_x = 0x200;
        } else {
            if (obj->vel_x < 0x200)
                obj->vel_x = 0x200;
            if (!(obj->flags_0a & 0x10) && other->vel_x > (int16_t)0xfe00)
                other->vel_x = (int16_t)0xfe00;
        }
    }

    clamp_record_pair(obj);
    clamp_record_pair(other);

    /*
     * The sixteenths, for both, and the rounding is not symmetric - the same
     * asymmetry `bounce_off_contact` has. Across, it keys on the sign of the
     * thing's own speed; down, on the sign of its kind's gravity at +8, read
     * through the kind table again rather than through `mine` and `theirs`.
     */
    x = obj->pos[0].x;
    obj->fx = obj->vel_x >= 0 ? (int32_t)((uint32_t)(x + 1) << 9) - 1
                              : (int32_t)((uint32_t)x << 9);
    y = obj->pos[0].y;
    obj->fy = g_part_kinds[obj->kind].gravity >= 0
              ? (int32_t)((uint32_t)(y + 1) << 9) - 1
              : (int32_t)((uint32_t)y << 9);
    x = other->pos[0].x;
    other->fx = other->vel_x >= 0 ? (int32_t)((uint32_t)(x + 1) << 9) - 1
                                  : (int32_t)((uint32_t)x << 9);
    y = other->pos[0].y;
    other->fy = g_part_kinds[other->kind].gravity >= 0
                ? (int32_t)((uint32_t)(y + 1) << 9) - 1
                : (int32_t)((uint32_t)y << 9);
}
