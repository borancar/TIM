#!/usr/bin/env python3
"""Which callee holds up each frame, and which frames are held up by nothing.

`tools/framify.py` can convert a routine only once every slot it hands out goes
to a callee that already takes a pointer. This says which those are, and what
each remaining blocker costs.

A first version took the callee's name from the *line* a slot was mentioned on,
which cannot see a call whose arguments wrap - and 41 of the 87 frames came
back blocked by "?". Scanning backwards for the innermost unclosed `(` and
taking the identifier before it names them all.
"""
import re, glob, collections, os
R = '/home/boran/git/TIM/reconstruct'
proto = open(os.path.join(R, 'tim.h')).read()
PTR = re.compile(r'\b(\w+)\s*\([^;]*?(?:dg_near|dg_cnear|const int16_t \*'
                 r'|const volatile uint8_t \*)[^;]*?\)\s*;', re.S)
ptrfn = set(PTR.findall(proto)) | {"step_accumulate", "dg_ptr", "dg_off",
                                   "dg_rd16", "dg_wr16", "dg_rd32", "dg_wr32"}
fn = re.compile(r'^[a-zA-Z_].*\b(\w+)\s*\(')
DECL = re.compile(r'^\s*uint16_t\s+(\w+)\s*=\s*(?:\(uint16_t\)\()?\s*fp\b[^;]*;')

def enclosing_call(text, at):
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

blocked = collections.Counter(); free = []; total = 0
for path in sorted(glob.glob(os.path.join(R, 'src', '*.c'))
                   + glob.glob(os.path.join(R, '*.c'))):
    lines = open(path).read().split('\n')
    starts = [i for i, l in enumerate(lines) if fn.match(l)
              and not l.rstrip().endswith(';')]
    starts.append(len(lines))
    for a, b in zip(starts, starts[1:]):
        body = lines[a:b]; blob = '\n'.join(body)
        if 'dg_enter(' not in blob:
            continue
        me = fn.match(lines[a]).group(1); total += 1
        slots = [DECL.match(l).group(1) for l in body if DECL.match(l)]
        # **The base counts as a slot.** `draw_counter_word` names its frame
        # `buf` and hands that straight to `int_to_string`; looking only at
        # `uint16_t v = fp + k` declarations missed it and called the routine
        # unblocked. framify had the same blind spot, so the two agreed while
        # both were wrong.
        bm = re.search(r'uint16_t\s+(\w+)\s*=\s*dg_enter\(', blob)
        if bm:
            slots.append(bm.group(1))
        # **And the `bp - k` slots.** Four routines keep `bp` at the frame's
        # top and derive every local from it; those declarations do not mention
        # the frame pointer, so a census that reads only `= fp + k` sees none of
        # them and calls the routine unblocked. `set_holiday_flags` hands one
        # to `dos_getdate`, which still takes an offset.
        slots += re.findall(r'uint16_t (\w+)\s*=\s*\(uint16_t\)\(bp\s*-', blob)
        outs = set()
        for v in slots:
            for m in re.finditer(r'(?<![\w.])' + v + r'(?![\w(])', blob):
                pre = blob[max(0, m.start() - 40):m.start()]
                if re.search(r'DG(?:8|S8|16|32|U16)\s*\(\s*(?:\(uint16_t\)\(\s*)?$', pre):
                    continue
                if re.search(r'uint16_t\s+$', pre):
                    continue
                f = enclosing_call(blob, m.start())
                if f is None or f in ('if', 'while', 'for', 'switch', 'return',
                                      'sizeof'):
                    continue
                if f not in ptrfn:
                    outs.add(f)
        if outs:
            for f in outs:
                blocked[f] += 1
        else:
            free.append((os.path.basename(path), me))
print("%d routines still call dg_enter" % total)
print("%d are unblocked - every slot they hand out goes to a pointer already\n"
      % len(free))
for p, m in free:
    print("   %-16s %s" % (p, m))
print("\nblocking callees, by how many frames each holds up:")
for f, n in blocked.most_common(24):
    print("   %-28s %2d frames" % (f, n))
print("\n%d distinct blockers" % len(blocked))
