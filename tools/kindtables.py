"""The part kinds' table and their drawing tables, as C, from the image.

`g_part_kinds` is one 0x3a-byte record per kind, and three of its fields
point into DGROUP at the kind's drawing tables: the first draw step of each
form (`bitmaps2`), each form's hot spot (`hotspots`) and each form's size
(`sizes`). The draw steps chain through their `next` words. This reads all
of it out of the recovered image and writes it in `gamedata.c`'s form - one
C object per table, named for the kind, each pointer a `NEAR_AT` of the
address the image holds and the object it lands in, each handler the port's
routine at the address the record holds - so the tables are transcribed the
same way every time instead of typed.

**Where each table ends is where the next begins.** The drawing tables tile
their range: a kind's draw steps, then its form steps, sizes and hot spots,
and the next kind's. So every pointer the records and the steps hold is a
boundary, and a table runs to the next one; the range ends where the last
table does, which `--end` gives when the next object is not a table.

1.11 keeps the records in a far data segment of their own (image 0x2ef10,
`FAR_SEGMENT`) and has 66 of them; 1.00 kept 58 in DGROUP at 0x0ea6.

`--kinds` prints the record initialiser, `--tables` the drawing tables,
`--report` what the handlers are and which have no routine in the port.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import bisect
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import tim

IMG = tim.IMAGE
FAR_SEGMENT = 0x2EF10
KIND_COUNT = 66
RECORD = 0x3A
HOOKS = ("hit", "step", "setup", "flip", "settle", "drive")

# The kinds' names, as dgroup.h has them; a kind without one is kind_NN.
NAMES = {
    0: "bowling_ball", 1: "brick_platform", 2: "ramp", 3: "seesaw",
    4: "balloon", 5: "conveyor", 6: "mouse_cage", 7: "pulley", 8: "belt",
    9: "basketball", 10: "rope", 11: "bird_cage", 12: "pokey",
    13: "jack_in_the_box", 14: "gear", 15: "bob_the_fish", 16: "bellow",
    17: "bucket", 18: "cannon", 19: "dynamite", 20: "bullet",
    21: "electric_plug", 22: "dynamite_plunger", 23: "hook", 24: "fan",
    25: "flashlight", 26: "generator", 27: "gun", 28: "baseball",
    29: "light", 30: "magnifying_glass", 31: "monkey", 32: "pumpkin",
    33: "heart_balloon", 34: "christmas_tree", 35: "boxing_glove",
    36: "rocket", 37: "scissors", 38: "solar_panel", 39: "trampoline",
    40: "windmill", 41: "blast", 42: "mort_the_mouse", 43: "cannon_ball",
    44: "tennis_ball", 45: "candle", 46: "pipe", 47: "corner_pipe",
    48: "wooden_platform", 49: "anchor", 50: "motor",
}


def kind_name(k):
    return NAMES.get(k, "kind_%d" % k)


def records(img):
    out = []
    for k in range(KIND_COUNT):
        r = img[FAR_SEGMENT + k * RECORD:FAR_SEGMENT + (k + 1) * RECORD]
        words = struct.unpack_from("<10H", r, 0)
        bitmaps, steps, hot, sizes = struct.unpack_from("<4H", r, 0x14)
        hooks = [struct.unpack_from("<HH", r, 0x22 + 4 * i) for i in range(6)]
        out.append(dict(kind=k, words=words, bitmaps=bitmaps, steps=steps,
                        hot=hot, sizes=sizes, level=r[0x1c:0x1e],
                        points=struct.unpack_from("<H", r, 0x1e)[0],
                        priority=struct.unpack_from("<H", r, 0x20)[0],
                        hooks=[s * 16 + o if (o or s) else 0 for o, s in hooks]))
    return out


def port_routines():
    """Every transcribed routine's image address, as the judge reads them."""
    import judge
    return {a: n for n, a in judge.addresses(judge.port_sources()).items()}


def layout(img, dg, recs, end, first):
    """The tables: (address, kind, what, count) in address order.

    A kind with draw steps has one entry per form in each of its three
    per-form tables, and the form-step table runs to the next table, so the
    form count is measured there; the kind ends after its last table, and
    the next kind's steps begin where it ends - its first step is not always
    one a form table names. `first` is where the first kind's steps begin.
    A kind with only sizes or hot spots has a table that runs to the next."""
    starts = sorted({r[f] for r in recs for f in ("steps", "hot", "sizes") if r[f]} | {end})
    after = lambda a: starts[bisect.bisect_right(starts, a)]
    out = []
    cursor = first
    by_steps = sorted((r for r in recs if r["steps"]), key=lambda r: r["steps"])
    for r in by_steps:
        k = r["kind"]
        n = (after(r["steps"]) - r["steps"]) // 2
        if r["steps"] > cursor:
            out.append((cursor, k, "draw_steps", (r["steps"] - cursor) // 15))
        out.append((r["steps"], k, "form_steps", n))
        last = r["steps"] + 2 * n
        if r["sizes"]:
            out.append((r["sizes"], k, "form_sizes", n))
            last = max(last, r["sizes"] + 4 * n)
        if r["hot"]:
            out.append((r["hot"], k, "hot_spots", n))
            last = max(last, r["hot"] + 2 * n)
        cursor = last
    for r in recs:
        if r["steps"]:
            continue
        for f, what, size in (("sizes", "form_sizes", 4), ("hot", "hot_spots", 2)):
            if r[f]:
                out.append((r[f], r["kind"], what, (after(r[f]) - r[f]) // size))
    out.sort()
    return out


def emit_tables(img, dg, tabs, first_step):
    word = lambda a: struct.unpack_from("<H", img, dg + a)[0]
    steps = [(a, k, n) for a, k, w, n in tabs if w == "draw_steps"]

    def step_ref(p):
        for a, k, n in steps:
            if a <= p < a + 15 * n and (p - a) % 15 == 0:
                return "NEAR_AT(0x%04x, &g_%s_draw_steps[%d])" % (p, kind_name(k), (p - a) // 15)
        raise SystemExit("0x%04x is no draw step" % p)

    lines = []
    for a, k, what, n in tabs:
        name = "g_%s_%s" % (kind_name(k), what)
        if what == "draw_steps":
            lines.append("struct draw_step %s[%d] = {   /* DGROUP 0x%04x */" % (name, n, a))
            for i in range(n):
                p = a + 15 * i
                nxt = word(p)
                lvl = img[dg + p + 2]
                fr = img[dg + p + 3:dg + p + 7]
                off = struct.unpack_from("<8b", img, dg + p + 7)
                lines.append("    {")
                lines.append("        %s,    /* next */" % (step_ref(nxt) if nxt else "0"))
                lines.append("        0x%02x,    /* level */" % lvl)
                lines.append("        { %s },    /* frame */" % ", ".join("0x%02x" % b for b in fr))
                pts = []
                for j in range(4):
                    x, y = off[2 * j], off[2 * j + 1]
                    pts.append("{ 0 }" if x == 0 and y == 0 else
                               "{ %s, %s }" % tuple("0x%02x" % (v & 0xff) if v else "0" for v in (x, y)))
                while len(pts) > 1 and pts[-1] == "{ 0 }":
                    pts.pop()
                lines.append("        { %s }    /* offset */" % ", ".join(pts))
                lines.append("    },")
            lines.append("};")
        elif what == "form_steps":
            lines.append("struct draw_step *%s[%d] = {   /* DGROUP 0x%04x */" % (name, n, a))
            for i in range(n):
                p = word(a + 2 * i)
                lines.append("    %s," % (step_ref(p) if p else "0"))
            lines.append("};")
        elif what == "form_sizes":
            lines.append("struct point16 %s[%d] = {   /* DGROUP 0x%04x */" % (name, n, a))
            for i in range(n):
                x, y = struct.unpack_from("<2h", img, dg + a + 4 * i)
                lines.append("    { 0x%04x, 0x%04x }," % (x & 0xffff, y & 0xffff)
                             if (x or y) else "    { 0 },")
            lines.append("};")
        else:
            lines.append("struct point8 %s[%d] = {   /* DGROUP 0x%04x */" % (name, n, a))
            for i in range(n):
                x, y = struct.unpack_from("<2b", img, dg + a + 2 * i)
                lines.append("    { 0 }," if not (x or y) else "    { %s, %s }," % tuple(
                    "0x%02x" % (v & 0xff) if v else "0" for v in (x, y)))
            lines.append("};")
        lines.append("")
    return "\n".join(lines)


# Handlers whose prototype is not the slot's, cast as gamedata.c has them:
# `part_flip_corner_pipe` takes the form as well.
CAST = {"part_flip_corner_pipe": "void (far *)()"}

# A handler the port has no routine for is named for the first kind and
# slot that holds it - the stub the port gives it has that name.
MISSING = {}


def missing_name(slot, kind, addr):
    return MISSING.setdefault(addr, "part_%s_%s" % (slot, kind_name(kind)))


def emit_kinds(recs, names, tabs):
    table_at = {a: "g_%s_%s" % (kind_name(k), w) for a, k, w, n in tabs}
    lines = ["struct part_kind far g_part_kinds[PART_KIND_COUNT] = {"]
    for r in recs:
        lines.append("    {   /* %d %s */" % (r["kind"], kind_name(r["kind"])))
        for v, f in zip(r["words"], ("density", "weight", "bounce", "grip", "gravity",
                                     "max_speed", "max_w", "max_h", "min_w", "min_h")):
            lines.append("        0x%04x,    /* %s */" % (v, f))
        lines.append("        0x%04x,    /* bitmaps */" % r["bitmaps"])
        for f, cast in (("steps", "bitmaps2"), ("hot", "hotspots"), ("sizes", "sizes")):
            a = r[f]
            lines.append("        %s,    /* %s */" % (
                "NEAR_AT(0x%04x, %s)" % (a, table_at[a]) if a else "0x0000", cast))
        lines.append("        { 0x%02x, 0x%02x },    /* refile_level */" % tuple(r["level"]))
        lines.append("        0x%04x,    /* point_count */" % r["points"])
        lines.append("        0x%04x,    /* priority */" % r["priority"])
        for h, a in zip(HOOKS, r["hooks"]):
            n = names.get(a) or missing_name(h, r["kind"], a)
            if h == "drive":
                lines.append("        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \\")
                lines.append("                                    uint16_t, uint16_t, int32_t)) \\")
                lines.append("                  (void (far *)(void))%s)    /* drive */" % n)
            elif n in CAST:
                lines.append("        (%s)%s,    /* %s */" % (CAST[n], n, h))
            else:
                lines.append("        %s,    /* %s */" % (n, h))
        lines.append("    },")
    lines.append("};")
    return "\n".join(lines)


TRAITS = [(0x0001, "TRAIT_ON_SURFACE"), (0x0002, "TRAIT_HIT_FIXED"),
          (0x0004, "TRAIT_HIT_MOVING"), (0x0008, "TRAIT_CONTACT_DONE"),
          (0x0010, "TRAIT_SPAWNED"), (0x0020, "TRAIT_SLIDES"),
          (0x0040, "TRAIT_TILED"), (0x0200, "TRAIT_CAN_FLIP_VERTICAL"),
          (0x0400, "TRAIT_CAN_FLIP_HORIZONTAL"), (0x0800, "TRAIT_IN_BIN"),
          (0x1000, "TRAIT_IN_MOVING_LIST"), (0x2000, "TRAIT_IN_PLACED_LIST"),
          (0x4000, "TRAIT_STATIC"), (0x8000, "TRAIT_FROM_LEVEL")]
TRAITS2 = [(0x0001, "TRAIT2_PLUGS_IN"), (0x0002, "TRAIT2_HAS_SOCKETS"),
           (0x0004, "TRAIT2_IGNITES"), (0x0008, "TRAIT2_FREE_PLACED"),
           (0x0010, "TRAIT2_IN_BUCKET"), (0x0020, "TRAIT2_FILED")]


def flags(v, names):
    """`v` as the OR of its named bits, highest first as game.c writes them,
    with any bit that has no name left as a number."""
    if not v:
        return "0"
    out, rest = [], v
    for bit, n in sorted(names, reverse=True):
        if v & bit:
            out.append(n)
            rest &= ~bit
    if rest:
        out.append("0x%04x" % rest)
    return " | ".join(out)


def emit_templates(img, dg, at, names):
    """`g_part_templates`: 16 bytes a kind at DGROUP `at`, ending in a far
    pointer to the kind's init routine."""
    lines = ["struct part_template g_part_templates[PART_KIND_COUNT] = {",
             "    /* traits, traits2, set_size, size, init */"]
    missing = []
    for k in range(KIND_COUNT):
        t, t2, sw, sh, w, h, o, sg = struct.unpack_from("<2H4h2H", img, dg + at + 16 * k)
        a = sg * 16 + o if (o or sg) else 0
        init = names.get(a) if a else "0"
        if init is None:
            init = missing_name("init", k, a)
            missing.append((k, a))
        lines.append("    { %s, %s, { %s, %s }, { %s, %s }, %s }, /* %d */" % (
            flags(t, TRAITS), flags(t2, TRAITS2),
            *("0x%04x" % (v & 0xffff) if v else "0" for v in (sw, sh, w, h)), init, k))
    lines.append("};")
    return "\n".join(lines), missing


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--kinds", action="store_true")
    ap.add_argument("--tables", action="store_true")
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--templates", type=lambda v: int(v, 0), default=None,
                    metavar="DGROUP", help="print g_part_templates, which is "
                    "at this DGROUP offset (1.11: 0x2488)")
    ap.add_argument("--first", type=lambda v: int(v, 0), default=0x2A7,
                    help="where the first kind's draw steps begin: after "
                         "g_default_draw_step")
    ap.add_argument("--end", type=lambda v: int(v, 0), default=None,
                    help="where the last table ends, if not at a pointer")
    a = ap.parse_args()
    img = open(IMG, "rb").read()
    dg = tim.image_dgroup(IMG)
    recs = records(img)
    names = port_routines()
    ptrs = sorted({r[f] for r in recs for f in ("steps", "hot", "sizes") if r[f]})
    end = a.end or ptrs[-1] + 2
    tabs = layout(img, dg, recs, end, a.first)
    if a.tables:
        print(emit_tables(img, dg, tabs, None))
    if a.kinds:
        print(emit_kinds(recs, names, tabs))
    if a.templates is not None:
        text, missing = emit_templates(img, dg, a.templates, names)
        print(text)
        for k, ad in missing:
            print("/* kind %d: init 0x%05x has no routine in the port */" % (k, ad))
    if a.kinds or a.templates is not None:
        for ad, n in sorted(MISSING.items()):
            print("/* MISSING 0x%05x %s */" % (ad, n))
    if a.report:
        for r in recs:
            for h, ad in zip(HOOKS, r["hooks"]):
                if ad not in names:
                    print("kind %2d %-8s 0x%05x  no routine in the port" % (r["kind"], h, ad))
        for t in tabs:
            print("0x%04x %-18s %-12s %d" % (t[0], kind_name(t[1]), t[2], t[3]))


if __name__ == "__main__":
    main()
