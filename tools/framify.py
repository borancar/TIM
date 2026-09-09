#!/usr/bin/env python3
"""Turn a routine's `dg_enter` frame into the `uint8_t frame[N]` it is.

The original is Borland Turbo C and a routine's `[bp-N]` locals are its own
stack. The port modelled every one as a DGROUP offset, which is only necessary
for a local another routine is given the address of - and the artefact this
project matches is the **global** DGROUP, not one routine's stack.

**Why an array and not separate C locals.** A first attempt gave each slot its
own variable and broke `draw_part_extra`: its `v06`, `v04` and `v02` are three
consecutive slots forming `x[0..2]`, and it hands `v06` to `draw_polygon`,
which walks upward from it. Inside one array that adjacency is structural and
cannot be lost. The array is `_Alignas(2)`, which is what a `[bp-N]` layout
guarantees and no more, so a long in it is read with `dg_rd32`.

The size is the routine's own `dg_enter(N)`; `tools/frames.py` checks that
against the `sub sp,N` in the binary and says which of the two rules the port
followed.

**The refusals matter more than the conversions.** Each of these was found by
breaking something first:

  a slot whose value is filed anywhere
        `vm_init` stores its frame pointer into `DG618A.fonts_off`, which the
        guest reads back, and `draw_compressed_bitmap` stores one slot's
        address into another. As a host address truncated to sixteen bits that
        is not a number at all. The symptom was one level in ten failing to
        solve, a different one each time, which reads exactly like the timer
        non-determinism this project already has - three ten-minute batches and
        two wrong theories. A slot may be *read through*; it may not be filed.

  a slot spelled in a way this cannot read
        `cut_belts` writes `(uint16_t)(fp + 0x26 - 2)`. Left half converted,
        `fp` survives into a body that no longer declares it.

  `bp` that derives nothing
        `vm_init` keeps `bp` only to store it, which the first rule catches
        anyway; this one says so in its own words rather than by accident.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import os
import re
import sys


def _pointer_takers():
    """Routines whose prototype says they take a pointer, from tim.h."""
    import os
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    proto = open(os.path.join(root, "reconstruct", "tim.h")).read()
    pat = re.compile(r'\b(\w+)\s*\([^;]*?(?:dg_near|dg_cnear|const int16_t \*'
                     r'|const volatile uint8_t \*|int16_t \*|uint8_t \*)'
                     r'[^;]*?\)\s*;', re.S)
    return set(pat.findall(proto)) | {"step_accumulate"}


def _enclosing_call(text, at):
    """The identifier whose ( is still open at this position."""
    depth = 0
    i = at - 1
    while i >= 0:
        c = text[i]
        if c == ')':
            depth += 1
        elif c == '(':
            if depth == 0:
                j = i - 1
                while j >= 0 and text[j] in ' \t\n':
                    j -= 1
                k = j
                while k >= 0 and (text[k].isalnum() or text[k] == '_'):
                    k -= 1
                return text[k + 1:j + 1] or None
            depth -= 1
        elif c == ';':
            return None
        i -= 1
    return None


def convert(path, names, verbose=True):
    src = open(path).read()
    fn = re.compile(r'^[a-zA-Z_].*\b(\w+)\s*\(', re.M)
    done, refused = [], []

    def say(msg):
        if verbose:
            print("   " + msg)

    for name in names:
        m = re.search(r'^[a-zA-Z_][^\n]*\b%s\s*\([^;]*\)\s*\n\{' % re.escape(name),
                      src, re.M)
        if not m:
            refused.append((name, "no body")); say("%s: no body" % name); continue
        i = m.start()
        j = src.index("\n}\n", i) + 3
        b = src[i:j]

        me = re.search(r'uint16_t\s+(\w+)\s*=\s*dg_enter\((0x[0-9a-fA-F]+|\d+)\);', b)
        if not me:
            refused.append((name, "no dg_enter")); say("%s: no dg_enter" % name)
            continue
        base, N = me.group(1), int(me.group(2), 0)

        slots = {}
        for sm in re.finditer(r'^(\s*)uint16_t (\w+)\s*=\s*'
                              r'(?:\(uint16_t\)\()?\s*' + base +
                              r'(?:\s*\+\s*(0x[0-9a-fA-F]+|\d+))?'
                              r'(?:\s*-\s*(0x[0-9a-fA-F]+|\d+))?'
                              r'\s*\)?;(.*)$',
                              b, re.M):
            # **`fp + A - B` is a slot too.** `fread_huge` writes
            # `(uint16_t)(fp + 0xe - 8)` - the frame's top stepped back, the
            # same thought as the `bp - k` idiom without a named `bp`. Matching
            # only `fp + A` found no slots at all and called the routine
            # frameless.
            off = int(sm.group(3), 0) if sm.group(3) else 0
            if sm.group(4):
                off -= int(sm.group(4), 0)
            slots[sm.group(2)] = (off, sm.group(1), sm.group(5), sm.group(0))
        # the base can be the only slot: `draw_counter_word` calls its frame
        # `buf` and reads `DG8(buf + i)` straight out of it
        if re.search(r'DG(?:8|S8|16|32|U16)\s*\(\s*(?:\(uint16_t\)\(\s*)?'
                     + base + r'\b', b):
            slots.setdefault(base, (0, '    ', '', None))
        if not slots:
            refused.append((name, "no slots")); say("%s: no slots" % name); continue

        # **A cursor is a local that walks a slot, and it is not a filed
        # address.** `score_to_code` writes `for (si = code; DG8(si) != 0;
        # si++)`: `si` is a plain C local that never leaves the routine, which
        # is a different thing from `vm_init` storing its frame pointer into
        # `DG618A.fonts_off` where the guest reads it back. The rule is strict
        # on purpose - a cursor is accepted only when every one of its other
        # uses is a `DG*` accessor, a step, or a comparison. One appearance
        # inside a call and the routine is refused, because the callee may
        # still want an offset and nothing here knows which.
        cursors = {}
        for v in list(slots):
            for cm in re.finditer(r'(?<![\w.])(\w+)\s*=\s*%s\s*[;)]'
                                  % re.escape(v), b):
                c = cm.group(1)
                if c in slots or c == v:
                    continue
                if not re.search(r'^\s*uint16_t\s+%s\s*(?:=[^;]*)?;' % c,
                                 b, re.M):
                    continue
                ok = True
                for um in re.finditer(r'(?<![\w.])%s(?![\w])' % c, b):
                    pre = b[max(0, um.start() - 40):um.start()]
                    post = b[um.end():um.end() + 3]
                    if re.search(r'DG(?:8|S8|16|32|U16)\s*\(\s*'
                                 r'(?:\(uint16_t\)\(\s*)?$', pre):
                        continue
                    if re.search(r'uint16_t\s+$', pre):
                        continue
                    # **Each allowed context named, not a class of
                    # punctuation.** A first version allowed any `(` before the
                    # name so that `for (si = ...` would pass, and `(` is also
                    # what a call looks like - so `text_width_thunk(si)` passed
                    # too and three routines converted that should not have.
                    tail = pre.rstrip()
                    if (tail.endswith('for (')          # for (si = slot; ...
                            or tail.endswith(';')       # ; si = ...
                            or tail.endswith('{')
                            or tail.endswith('=')       # x = si
                            or post.startswith(('++', '--', ' =', ' !', ' <',
                                                ' >', ' +', ' -', ';', ')'))):
                        continue
                    ok = False
                    break
                if ok:
                    cursors[c] = v
        # a cursor keeps its own declaration; it is only retyped
        for c in cursors:
            cd = re.search(r'^(\s*)uint16_t(\s+)%s(\s*(?:=[^;]*)?);(.*)$' % c,
                           b, re.M)
            if cd:
                slots.setdefault(c, ('cursor', cd.group(1), cd.group(4),
                                     cd.group(0)))

        # **A slot that is reassigned is not a slot.**
        # `remove_and_free_records` writes `uint16_t link_off = fp;` and later
        # `link_off = cur_off;` - the variable is initialised to the frame's
        # address and then walks a list, so it is a moving far pointer that
        # merely starts there. Converting it made an `int16_t *` that the next
        # line assigns a `uint16_t` to, which the compiler caught; had the two
        # types agreed it would have been silent.
        moved = [v for v in slots if v not in cursors
                 and re.search(r'(?<![\w.])%s\s*=(?!=)' % re.escape(v),
                               b[b.index(slots[v][3]) + len(slots[v][3]):]
                               if slots[v][3] else b)]
        if moved:
            refused.append((name, "reassigns " + ", ".join(moved)))
            say("%s: reassigns %s, so it is not a frame slot"
                % (name, ", ".join(moved)))
            continue

        # the `bp - k` slots are slots too, and the escape check below has to
        # see them: `read_record` derives `b1`, `b2`, `b3` that way and hands
        # them to `read_resource`, which writes through them as DGROUP
        # addresses. Gathered here rather than in the bp-k branch, which runs
        # after the refusals.
        for dm in re.finditer(r'^(\s*)uint16_t (\w+)\s*=\s*'
                              r'\(uint16_t\)\(bp\s*-\s*'
                              r'(0x[0-9a-fA-F]+|\d+)\);(.*)$', b, re.M):
            slots.setdefault(dm.group(2), (None, dm.group(1), dm.group(4),
                                           dm.group(0)))

        # **A slot handed to a routine that takes an offset cannot be a C
        # local, and the compiler will not tell you.**
        #
        # `read_record` passes `&b3` to `read_resource(handle, off, seg, 1)`,
        # which writes through it *as a DGROUP address*. Once `b3` points into
        # a C array, `dg_off(dgroup, b3)` is the distance between two unrelated
        # objects - a number, accepted by the compiler because `dg_off` takes a
        # `void *`, and pointing nowhere the callee should write. Forty-one
        # call sites were "fixed" that way in one sitting by wrapping whatever
        # the compiler complained about; every one of them was wrong, and the
        # build was clean.
        #
        # So the callee's signature decides, not the compiler's silence:
        # `tools/framify_census.py` computes which routines take a pointer, and
        # a slot that reaches anything else stops the conversion here.
        ptr_takers = _pointer_takers()
        escapes = set()
        for v in slots:
            if v in cursors:
                continue
            for um in re.finditer(r'(?<![\w.])%s(?![\w])' % re.escape(v), b):
                pre = b[max(0, um.start() - 60):um.start()]
                if re.search(r'DG(?:8|S8|16|32|U16)\s*\(\s*'
                             r'(?:\(uint16_t\)\(\s*)?$', pre):
                    continue
                if re.search(r'uint16_t\s+$', pre):
                    continue
                callee = _enclosing_call(b, um.start())
                if callee and callee not in ptr_takers \
                        and callee not in ('if', 'while', 'for', 'switch',
                                           'return', 'sizeof', 'dg_ptr',
                                           'dg_off', 'dg_rd16', 'dg_wr16',
                                           'dg_rd32', 'dg_wr32'):
                    escapes.add(callee)
        if escapes:
            refused.append((name, "hands a slot to " + ", ".join(sorted(escapes))))
            say("%s: hands a slot to %s, which still takes an offset"
                % (name, ", ".join(sorted(escapes))))
            continue

        # ---- the refusals, before anything is rewritten ----
        filed = [v for v in slots if v not in cursors
                 and re.search(r'(?:DG[0-9A-F]{4}\.\w+|DG(?:8|S8|16|32|U16)'
                               r'\([^)]*\))\s*=\s*'
                               r'(?:\((?:u?int(?:8|16|32)_t)\))?\s*'
                               r'%s\s*[;,)]' % re.escape(v), b)
                 or (v not in cursors
                     and re.search(r'=\s*(?:\((?:u?int(?:8|16|32)_t)\))?\s*'
                                   r'%s\s*[;,)]' % re.escape(v), b)
                     and not any(cursors.get(c) == v for c in cursors))]
        if filed:
            refused.append((name, "files the address of " + ", ".join(filed)))
            say("%s: files the address of %s - see the module comment"
                % (name, ", ".join(filed)))
            continue

        rest = re.sub(r'uint16_t\s+\w+\s*=\s*dg_enter\([^)]*\);', '', b)
        for sm in slots.values():
            if sm[3]:
                rest = rest.replace(sm[3], '')
        if base in slots:
            rest = re.sub(r'(?<![\w.])' + base + r'(?![\w])', '', rest)
        if re.search(r'(?<![\w.])' + base + r'(?![\w(])', rest):
            refused.append((name, "spells a slot this cannot read"))
            say("%s: spells a slot this cannot read" % name); continue

        arr = "frame"
        if re.search(r'(?<![\w.])frame(?![\w])', b):
            arr = "dgframe"
            if re.search(r'(?<![\w.])dgframe(?![\w])', b):
                refused.append((name, "uses both frame and dgframe"))
                say("%s: uses both `frame` and `dgframe`" % name); continue

        head = ("_Alignas(2) uint8_t %s[%#04x];   /* the bytes `dg_enter` "
                "reserved;\n       tools/frames.py checks it against the "
                "original's own `sub sp` */" % (arr, N))

        # ---- the `bp - k` idiom ----
        # `bp` sits at the frame's top and slots are `(uint16_t)(bp - k)`,
        # straight from the listing. As a `uint8_t *` the subtraction is bytes,
        # which is what the listing means; as a typed pointer it would be
        # elements, which compiles and means something else.
        bpm = re.search(r'^(\s*)uint16_t bp\s*=\s*(?:\(uint16_t\)\()?\s*'
                        + base + r'(?:\s*\+\s*(0x[0-9a-fA-F]+|\d+))?\s*\)?;(.*)$',
                        b, re.M)
        if bpm:
            derived = list(re.finditer(
                r'^(\s*)uint16_t (\w+)\s*=\s*\(uint16_t\)\(bp\s*-\s*'
                r'(0x[0-9a-fA-F]+|\d+)\);(.*)$', b, re.M))
            if not derived:
                refused.append((name, "keeps `bp` and derives nothing"))
                say("%s: keeps `bp` and derives nothing from it" % name); continue
            nb = b.replace(bpm.group(0), "%suint8_t *bp = &%s[%#04x];%s"
                           % (bpm.group(1), arr,
                              int(bpm.group(2), 0) if bpm.group(2) else 0,
                              bpm.group(3)))
            for dm in derived:
                nb = nb.replace(dm.group(0), "%suint8_t *%s = bp - %s;%s"
                                % (dm.group(1), dm.group(2), dm.group(3),
                                   dm.group(4)))
            nb = nb.replace(me.group(0), head)
            nb = re.sub(r'^\s*dg_leave\((?:0x[0-9a-fA-F]+|\d+)\);\n', '', nb,
                        flags=re.M)
            src = src[:i] + nb + src[j:]
            done.append(name + " (bp-k)")
            continue

        # ---- the ordinary case ----
        use = collections.defaultdict(set)
        for v in slots:
            for am in re.finditer(r'\bDG(8|S8|16|32|U16)\s*\(\s*'
                                  r'(?:\(uint16_t\)\(\s*)?' + v +
                                  r'\s*(?:\+\s*(0x[0-9a-fA-F]+|\d+))?\s*\)\)?', b):
                use[v].add((am.group(1),
                            int(am.group(2), 0) if am.group(2) else 0))
        nb = b
        for v, (k, ind, tail, decl) in slots.items():
            widths = {w for w, _ in use[v]}
            offs = {o for _, o in use[v]}
            word = widths <= {"16", "U16"} and all(o % 2 == 0 for o in offs)
            if k == 'cursor':
                nb = nb.replace(decl, "%suint8_t *%s%s;%s"
                                % (ind, v, decl.split(v, 1)[1].split(';')[0],
                                   tail))
                for w, o in sorted(use[v]):
                    old = ("DG%s(%s + %d)" % (w, v, o)) if o else "DG%s(%s)" % (w, v)
                    nb = nb.replace(old, "%s[%d]" % (v, o) if o else "(*%s)" % v)
                continue
            if decl is None:
                nb = nb.replace(me.group(0),
                                "%s\n    uint8_t *%s = &%s[0];" % (head, v, arr))
                for w, o in sorted(use[v]):
                    old = ("DG%s(%s + %d)" % (w, v, o)) if o else "DG%s(%s)" % (w, v)
                    nb = nb.replace(old, "%s[%d]" % (v, o) if o else "(*%s)" % v)
                continue
            if word:
                nb = nb.replace(decl, "%sint16_t *%s = (int16_t *)&%s[%#04x];%s"
                                % (ind, v, arr, k, tail))
                for w, o in sorted(use[v]):
                    for spelling in ("DG%s((uint16_t)(%s + %d))" % (w, v, o),
                                     "DG%s(%s + %d)" % (w, v, o)) if o else \
                                    ("DG%s(%s)" % (w, v),):
                        new = "%s[%d]" % (v, o // 2)
                        nb = nb.replace(spelling,
                                        "(uint16_t)" + new if w == "U16" else new)
            else:
                nb = nb.replace(decl, "%suint8_t *%s = &%s[%#04x];%s"
                                % (ind, v, arr, k, tail))
        if "dg_enter" in nb:
            nb = nb.replace(me.group(0), head)
        nb = re.sub(r'^\s*dg_leave\((?:0x[0-9a-fA-F]+|\d+)\);\n', '', nb,
                    flags=re.M)
        src = src[:i] + nb + src[j:]
        done.append(name)

    open(path, 'w').write(src)
    return done, refused


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", help="the C file to rewrite, in place")
    ap.add_argument("routine", nargs="+",
                    help="which routines; a routine can only be converted once "
                         "every slot it hands out goes to a callee that takes a "
                         "pointer - tools/framify_census.py says which those are")
    args = ap.parse_args()
    done, refused = convert(args.file, args.routine)
    print("converted %d: %s" % (len(done), " ".join(done)))
    if refused:
        print("refused %d" % len(refused))
    return 0


if __name__ == "__main__":
    sys.exit(main())
