"""Promote a `frame[]` slot to the C local it is.

The frames themselves were made by `framify.py`, which turned a routine's
`dg_alloca` reservation into `_Alignas(2) uint8_t frame[N]` with a pointer per
slot. That was the right shape for the conversion - the bytes stayed one block,
laid out as the original laid them out - and it is the wrong shape to keep: a
slot is a local variable, and holding it as a pointer into a shared buffer
means a write through one slot can run into its neighbour, which a C local
cannot.

So each slot becomes its own object:

  - a slot whose extent is exactly its type's width is a **scalar**. `v[0]` and
    `*v` become `v`; a bare `v`, which is the slot's address handed to a
    callee, becomes `&v`.
  - a slot with room for more is an **array** of its type, and nothing about
    its uses changes: `v[k]`, `*v` and a bare `v` all still mean what they did.

A slot's extent is the distance to the next slot, or to the end of the frame -
which is why the slots have to tile it, and why this refuses a frame it cannot
account for.

**The refusals are the point**, as they are in `framify.py`:

  - the array is used for anything but declaring slots - `draw_rope` builds a
    table of pointers into it, and that is a real array of pointers
  - a scalar slot is indexed anywhere but `[0]`, which would read its neighbour
  - two slots claim the same offset, so the extents are not a tiling
  - a slot's extent is not a whole number of its own type

And the rewrite only ever touches code. A slot's name appearing in a comment is
left alone: `framify.py` once rewrote one inside a comment, which is the sort of
edit that reads as deliberate to the next person.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

DECL = re.compile(r'^([ \t]*)_Alignas\(2\)\s+uint8_t\s+(frame|dgframe)'
                  r'\[(0x[0-9a-fA-F]+|\d+)\]\s*;'
                  r'(?:[ \t]*/\*.*?\*/)?[ \t]*\n', re.M | re.S)
SLOT = re.compile(r'^([ \t]*)((?:const\s+)?(\w+)\s*\*\s*(\w+)\s*=\s*'
                  r'(?:\(\s*(?:const\s+)?\w+\s*\*\s*\)\s*)?&?(frame|dgframe)'
                  r'(?:\[\s*(0x[0-9a-fA-F]+|\d+)\s*\])?\s*;)', re.M)
WIDTH = {"uint8_t": 1, "int8_t": 1, "char": 1,
         "uint16_t": 2, "int16_t": 2, "uint32_t": 4, "int32_t": 4}

# **How many bytes a callee writes through the address it is given.** A slot
# handed to one of these is that big and no bigger, whatever room happens to
# follow it: `load_bitmaps` keeps `list_at` at the top of its frame with the
# saved registers above, and sizing it by the distance to the end made it nine
# words where the routine and its own `[bp-2]` comment both say one.
#
# Every entry was read: these all write through their out-parameter with
# `dg_wr16`, which is two bytes. `game_fread` and its relatives state the size
# at the call instead, and are read from there.
WRITES = {
    "game_fread_far": 2, "write_word": 2, "rotate_point": 2,
    "parse_open_mode": 2, "link_endpoint_gap": 2, "find_belt_anchor": 2,
    "grab_distance": 2, "read_bmp_info": 2, "vm_bitmap_list_size": 2,
    "game_fread_byte": 1,
}
SIZED = re.compile(r'\b(game_fread|game_fwrite|borland_fread)\s*\(\s*'
                   r'\(dg_c?near\)\s*(\w+)\s*,\s*'
                   r'(0x[0-9a-fA-F]+|\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)')

# **An accessor states its own width, and a slot has to be at least that
# wide.** `bounce_off_contact` keeps a 32-bit value in `plo` at [bp-0x10] and
# `phi` at [bp-0x0e] and writes the whole of it through the low half -
# `dg_wr32(plo, ...)`. Sized by the distance to `phi`, `plo` is two bytes, and
# the write runs off the end of it. As slots in one buffer that worked, because
# the neighbour *was* the other half; as separate locals it is a bug, and the
# only honest answer is to refuse the routine and join the two by hand.
ACCESS = re.compile(r'\b(dg_rd8|dg_wr8|dg_rd16|dg_wr16|dg_rd32|dg_wr32)\s*\(\s*'
                    r'(?:\(\s*[\w ]*\*\s*\)\s*)?(\w+)\s*'
                    r'(?:\+\s*([^,)]+?))?\s*[,)]')
ACCESS_WIDTH = {"dg_rd8": 1, "dg_wr8": 1, "dg_rd16": 2, "dg_wr16": 2,
                "dg_rd32": 4, "dg_wr32": 4}

# **The other way a frame names its slots.** Some routines keep the original's
# BP - a pointer one past the end of the frame, because `bp-N` is how the
# listing spells every local - and declare their slots against it. `hdr = bp -
# 0x10` in a 0x14 frame is the slot at offset 4, and it is the same slot as
# `&frame[0x04]` would be.
BASE = re.compile(r'^([ \t]*)uint8_t\s*\*\s*bp\s*=\s*&?(frame|dgframe)'
                  r'\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]\s*;'
                  r'(?:[ \t]*/\*.*?\*/)?[ \t]*\n', re.M | re.S)
# The cast may wrap the arithmetic - `(int16_t *)(bp - 4)` - or not, and both
# spellings are in the tree.
OFFBP = re.compile(r'^([ \t]*)((?:const\s+)?(\w+)\s*\*\s*(\w+)\s*=\s*'
                   r'(?:\(\s*(?:const\s+)?\w+\s*\*\s*\)\s*)?'
                   r'\(?\s*bp\s*-\s*(0x[0-9a-fA-F]+|\d+)\s*\)?\s*;)', re.M)


def code_spans(text):
    """The (start, end) ranges of `text` that are code rather than comment."""
    out, i, n, start = [], 0, len(text), 0
    while i < n:
        if text.startswith("/*", i):
            out.append((start, i))
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
            start = i
        elif text.startswith("//", i):
            out.append((start, i))
            j = text.find("\n", i)
            i = n if j < 0 else j
            start = i
        else:
            i += 1
    out.append((start, n))
    return out


def rewrite_code(text, subs):
    """Apply `subs` (a list of (compiled pattern, replacement)) outside comments."""
    pieces, last = [], 0
    for a, b in code_spans(text):
        pieces.append(text[last:a])
        chunk = text[a:b]
        for pat, rep in subs:
            chunk = pat.sub(rep, chunk)
        pieces.append(chunk)
        last = b
    pieces.append(text[last:])
    return "".join(pieces)


def convert(path, names, verbose=True):
    src = open(path).read()
    done, refused = [], []

    for name in names:
        m = re.search(r'\n[a-zA-Z_][\w \*]*\b%s\s*\([^;{]*\)\s*\n\{\n' % re.escape(name),
                      src)
        if not m:
            refused.append((name, "no such routine in %s" % os.path.basename(path)))
            continue
        i = m.end()
        j = src.index("\n}\n", i) + 1
        body = src[i:j]

        d = DECL.search(body)
        if not d:
            refused.append((name, "no frame[] to promote"))
            continue
        indent, arr, size = d.group(1), d.group(2), int(d.group(3), 0)

        slots, spans = [], [(d.start(), d.end())]

        # A `bp` of its own is the frame's end, not a slot; the slots that name
        # it are folded back onto their offsets.
        base = BASE.search(body)
        if base:
            if base.group(2) != arr or int(base.group(3), 0) != size:
                refused.append((name, "`bp` is not this frame's end"))
                continue
            spans.append((base.start(), base.end()))
            for om in OFFBP.finditer(body):
                slots.append({"decl": om.group(0), "indent": om.group(1),
                              "type": om.group(3), "var": om.group(4),
                              "off": size - int(om.group(5), 0), "tail": ""})
                spans.append((om.start(), om.end()))

        for sm in SLOT.finditer(body):
            if sm.group(5) != arr:
                continue
            # `bp` is the frame's end and was taken above; the slot pattern
            # matches its declaration too, and reading it as a slot gives one
            # of extent zero.
            if base and base.start() <= sm.start() < base.end():
                continue
            off = int(sm.group(6), 0) if sm.group(6) else 0
            slots.append({"decl": sm.group(0), "indent": sm.group(1),
                          "type": sm.group(3), "var": sm.group(4), "off": off,
                          "tail": body[sm.end():body.find("\n", sm.end())]})
            spans.append((sm.start(), sm.end()))
        if not slots:
            refused.append((name, "the frame has no slots"))
            continue

        # the array must not be used for anything else
        # Anything outside the declaration and the slot lines. Tested by
        # *position*, not by re-matching the line: the frame's declaration
        # carries a comment that runs on past it, so its own name is not on a
        # line the declaration pattern matches.
        others = []
        for a, b in code_spans(body):
            for um in re.finditer(r'\b%s\b' % arr, body[a:b]):
                at = a + um.start()
                if any(lo <= at < hi for lo, hi in spans):
                    continue
                line = body[body.rfind("\n", 0, at) + 1:body.find("\n", at)]
                others.append(line.strip())
        if others:
            refused.append((name, "`%s` is used beyond its slots: %s"
                            % (arr, others[0][:60])))
            continue

        slots.sort(key=lambda s: s["off"])

        # **Two names for one slot is the original reusing it, and the port has
        # to keep them sharing.** `blit_scaled_a` calls [bp-0x16] `vcut` while
        # it clips and `vrepeat` while it repeats rows; the comment beside its
        # neighbour records what happened when a value was written through the
        # wrong one of those - the row counter was clobbered and every scaled
        # part on the briefing screen came out a smear. Separate C variables
        # would stop reproducing that, silently. So the first name becomes the
        # variable and the rest alias it.
        aliases = []
        seen = {}
        for s in list(slots):
            if s["off"] in seen:
                first = seen[s["off"]]
                if first["type"] != s["type"]:
                    refused.append((name, "%s and %s share an offset at "
                                    "different types" % (first["var"], s["var"])))
                    seen = None
                    break
                aliases.append((s, first))
                slots.remove(s)
            else:
                seen[s["off"]] = s
        if seen is None:
            continue

        # **How a slot is used decides its size; the extent only bounds it.**
        # The distance to the next *named* slot is not the slot's own size -
        # `read_record_fields` has a word at 0x02 and its next name at 0x05,
        # with an unnamed byte between - so a slot nothing indexes past [0] and
        # nothing hands to a callee is one variable however much room follows
        # it. A slot that is indexed further, or whose address is passed on, is
        # a buffer and gets the whole extent, because then the size is the
        # caller's business rather than this routine's.
        bad = None
        for k, s in enumerate(slots):
            end = slots[k + 1]["off"] if k + 1 < len(slots) else size
            s["extent"] = end - s["off"]
            w = WIDTH.get(s["type"])
            if not w:
                bad = "%s has no known width for %s" % (s["var"], s["type"])
                break
            v = re.escape(s["var"])
            wide = passed = False
            most = 0
            for a2, b2 in code_spans(body):
                chunk = body[a2:b2]
                for im in re.finditer(r'(?<![\w.])%s\s*\[\s*([^\]]+?)\s*\]'
                                      % v, chunk):
                    k = im.group(1).strip()
                    if re.fullmatch(r'0x[0-9a-fA-F]+|\d+', k):
                        most = max(most, int(k, 0) + 1)
                    elif k != "0":
                        wide = True          # a variable index needs the room
                for um in re.finditer(r'(?<![\w.&*])%s\b(?!\s*[\[=])' % v, chunk):
                    at = a2 + um.start()
                    if any(lo <= at < hi for lo, hi in spans):
                        continue
                    passed = True
            # An accessor's own width, which is not negotiable.
            need = 0
            for am in ACCESS.finditer(body):
                if am.group(2) != s["var"]:
                    continue
                at = am.group(3)
                if at is None:
                    need = max(need, ACCESS_WIDTH[am.group(1)])
                elif re.fullmatch(r'0x[0-9a-fA-F]+|\d+', at.strip()):
                    # `dg_wr16(rd + 2, ...)` reaches past the name it starts at
                    need = max(need, int(at, 0) + ACCESS_WIDTH[am.group(1)])
                else:
                    # a computed offset - `rd + 0x18 + 2 * i` - reaches as far
                    # as the routine wants, so the slot keeps its whole extent
                    wide = True
            if need > s["extent"]:
                bad = ("%s is read %d bytes wide and the frame gives it %d - "
                       "it shares a value with the slot above it"
                       % (s["var"], need, s["extent"]))
                break
            most = max(most, -(-need // w))

            # What a call states, or what its callee is known to write.
            for sm2 in SIZED.finditer(body):
                if sm2.group(2) == s["var"]:
                    most = max(most, -(-int(sm2.group(3), 0)
                                       * int(sm2.group(4), 0) // w))
            # **By statement, not by a parenthesised argument list.** A lazy
            # match for `name(...)` stops at the first `)`, so for
            # `copy_file_record((dg_near)saved_a, di)` the text it examined was
            # `(dg_near` and the slot never appeared in it - which sized a
            # 52-byte buffer as one byte.
            # **A bare pass is checked whichever way the size was reached.**
            # Setting `most` from an accessor's width used to skip this scan,
            # and `decode_vqt_list`'s reader record - handed to `dg_off` and
            # written at `rd + 2` upwards - came out two bytes because of it.
            if passed:
                for stmt in body.split(";"):
                    if not re.search(r'(?<![\w.&*])%s\b(?!\s*[\[=])' % v, stmt):
                        continue
                    called = re.findall(r'\b([a-z_]\w*)\s*\(', stmt)
                    called = [c for c in called
                              if c not in ("if", "while", "for", "switch",
                                           "return", "sizeof")]
                    known = [c for c in called if c in WRITES]
                    if called and len(known) == len(called):
                        most = max(most, max(-(-WRITES[c] // w) for c in known))
                    else:
                        wide = True      # anything else gets the whole room
            if wide:
                if s["extent"] <= 0 or s["extent"] % w:
                    bad = ("%s is used as a buffer and spans %d bytes, which is "
                           "not a whole number of %s"
                           % (s["var"], s["extent"], s["type"]))
                    break
                s["n"] = s["extent"] // w
            else:
                s["n"] = max(1, most)
                if s["n"] * w > s["extent"]:
                    bad = ("%s wants %d bytes and the frame gives it %d"
                           % (s["var"], s["n"] * w, s["extent"]))
                    break
        if bad:
            refused.append((name, bad))
            continue

        # a scalar indexed anywhere but [0] would reach its neighbour
        for s in slots:
            if s["n"] != 1:
                continue
            for a, b in code_spans(body):
                for im in re.finditer(r'(?<![\w.])%s\s*\[\s*([^\]]+?)\s*\]'
                                      % re.escape(s["var"]), body[a:b]):
                    if im.group(1).strip() != "0":
                        bad = "%s is a scalar indexed at [%s]" % (s["var"],
                                                                  im.group(1))
        if bad:
            refused.append((name, bad))
            continue

        nb = body
        for s in slots:
            if s["n"] == 1:
                nb = nb.replace(s["decl"],
                                "%s%s %s;" % (s["indent"], s["type"], s["var"]))
                v = re.escape(s["var"])
                # **One pass, through a placeholder.** `v[0]` and `*v` become
                # the value and a bare `v` becomes its address - and doing that
                # in two passes turns the first result into the second, which
                # is how `saved[0]` came out as `&saved` the first time this
                # was run. The value form is parked on a name no C source can
                # contain until the address form has been dealt with.
                # The token must not contain the slot's own name: the
                # address pass below matches on a word boundary, and `\x00` is
                # not a word character, so a marker holding the name got
                # rewritten inside itself the first time this was tried.
                mark = "\x00%d\x00" % slots.index(s)
                nb = rewrite_code(nb, [
                    (re.compile(r'(?<![\w.])%s\s*\[\s*0\s*\]' % v), mark),
                    (re.compile(r'\(\s*\*\s*%s\s*\)' % v), mark),
                    (re.compile(r'\*\s*%s\b' % v), mark),
                ])
                nb = rewrite_code(nb, [
                    (re.compile(r'(?<![\w.&])%s\b' % v), "&" + s["var"]),
                ])
                nb = nb.replace(mark, s["var"])
                # its own declaration is the one bare use that is not an address
                nb = nb.replace("%s%s &%s;" % (s["indent"], s["type"], s["var"]),
                                "%s%s %s;" % (s["indent"], s["type"], s["var"]))
            else:
                nb = nb.replace(s["decl"],
                                "%s%s %s[%d];" % (s["indent"], s["type"],
                                                  s["var"], s["n"]))

        for s, first in aliases:
            nb = nb.replace(s["decl"],
                            "%s%s *%s = &%s;   /* the same slot as `%s` */"
                            % (s["indent"], s["type"], s["var"],
                               first["var"], first["var"]))

        if base:
            left = [u for a2, b2 in code_spans(nb)
                    for u in re.finditer(r'(?<![\w.])bp\b', nb[a2:b2])]
            if len(left) > 1:          # its own declaration is the one allowed
                refused.append((name, "`bp` is used for more than its slots"))
                continue
            nb = BASE.sub("", nb, count=1)
        nb = DECL.sub("", nb, count=1)
        nb = re.sub(r'\n[ \t]*\n[ \t]*\n', "\n\n", nb)
        src = src[:i] + nb + src[j:]
        done.append(name)

    if done:
        open(path, "w").write(src)
    if verbose:
        for n, why in refused:
            print("   %s: %s" % (n, why))
    return done, refused


def remaining():
    """Every routine that still holds a frame array, and how big it is."""
    import glob
    out = []
    for path in sorted(glob.glob(os.path.join(ROOT, "reconstruct", "src", "*.c"))
                       + glob.glob(os.path.join(ROOT, "reconstruct", "*.c"))):
        src = open(path).read()
        for m in re.finditer(r'\n[a-zA-Z_][\w \*]*\b(\w+)\s*\([^;{]*\)\s*\n\{\n'
                             r'(.*?)\n\}\n', src, re.S):
            d = DECL.search(m.group(2))
            if d:
                out.append((os.path.basename(path), m.group(1),
                            int(d.group(3), 0)))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", nargs="?", help="the .c file to edit")
    ap.add_argument("routine", nargs="*", help="which routines to promote")
    ap.add_argument("--list", action="store_true",
                    help="every routine that still holds a frame array")
    ap.add_argument("--assert", dest="assert_", action="store_true",
                    help="fail if any frame array is left; for `make test`")
    args = ap.parse_args()

    if args.list or args.assert_:
        left = remaining()
        for f, n, size in left:
            print("   %-16s %-30s frame[%#x]" % (f, n, size))
        if not left:
            print("no routine holds a frame array")
            return 0
        print("%d routines still hold one" % len(left))
        return 1 if args.assert_ else 0

    if not args.file or not args.routine:
        ap.error("a file and at least one routine, or --list/--assert")

    done, refused = convert(args.file, args.routine)
    print("promoted %d: %s" % (len(done), ", ".join(done)))
    print("refused %d" % len(refused))
    return 1 if refused and not done else 0


if __name__ == "__main__":
    sys.exit(main())
