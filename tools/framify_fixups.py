#!/usr/bin/env python3
"""The shapes a frame conversion leaves behind, fixed per function.

Run after `tools/framify.py`. Per *function*, because slot names are per
function: `plo` is a frame slot in one routine and a DGROUP offset in another,
and a file-wide replace turns the second into nonsense that still compiles.

**The `(?!=)` on every write rule is not decoration.** Without it
`DG16(found) == 0` becomes `dg_wr16(found, = 0`, because a write pattern happily
matches across the second `=` of a comparison. That has broken the build three
times - `next[0] == endB[0]`, and twice more - each time from a regex retyped
by hand somewhere else. It lives here now so there is one copy to get right.

Per *function*, because slot names are per function: `plo` is a frame slot in
one routine and a DGROUP offset in another, and a file-wide replace turns the
second into nonsense that still compiles.
"""
import re, sys

def fix(path):
    lines = open(path).read().split('\n')
    fn = re.compile(r'^[a-zA-Z_].*\b(\w+)\s*\(')
    starts = [i for i, l in enumerate(lines) if fn.match(l)
              and not l.rstrip().endswith(';')]
    starts.append(len(lines))
    n = 0
    for a, b in zip(starts, starts[1:]):
        blob = '\n'.join(lines[a:b])
        # **Three declaration forms, not one.** A slot is `&frame[k]`, or
        # `&dgframe[k]` where the routine already had a local called `frame`,
        # or `bp - k` in the routines that keep `bp` at the frame's top. A
        # pattern that knew only the first reported "0 fixes" on a file full of
        # `DG8(b1)` and left the build broken with nothing to say why.
        word = set(re.findall(r'^\s*int16_t\s+\*(\w+)\s*=\s*\(int16_t \*\)'
                              r'(?:&(?:dg)?frame\[|bp - )', blob, re.M))
        byte = set(re.findall(r'^\s*uint8_t\s+\*(\w+)\s*=\s*'
                              r'(?:&(?:dg)?frame\[|bp - )', blob, re.M))
        if not (word or byte):
            continue
        # an unsigned read used as an lvalue is a write
        # **`=` and not `==`.** Without the lookahead this turns a comparison
        # `(uint16_t)next[0] == (uint16_t)endB[0]` into `next[0] = (int16_t)=`,
        # which is a syntax error and would have been an assignment if it had
        # parsed.
        blob, k = re.subn(r'\(uint16_t\)(\w+\[\d+\])\s*=(?!=)\s*',
                          r'\1 = (int16_t)', blob); n += k
        blob, k = re.subn(r'\(uint16_t\)(\w+\[\d+\])(\+\+|--)', r'\1\2', blob); n += k
        # a slot that is already a pointer needs no dg_ptr
        for v in word | byte:
            blob, k = re.subn(r'dg_ptr\(dgroup, %s\)' % v, '(dg_near)%s' % v, blob)
            n += k
        # a byte-pointer slot's own width needs no accessor: it is the array
        for v in byte:
            blob, k = re.subn(r'DG8\(%s\s*\+\s*(\d+)\)' % v,
                              lambda m, v=v: '%s[%s]' % (v, m.group(1)), blob); n += k
            blob, k = re.subn(r'DG8\(%s\)' % v, '(*%s)' % v, blob); n += k
        # a byte read out of a word slot says so
        for v in word:
            blob, k = re.subn(r'DG8\(%s\)' % v,
                              '(*(uint8_t *)%s)' % v, blob); n += k
        # a byte-pointer slot reads and writes with the explicit accessors
        for v in byte:
            for w, rd, wr in (('32', 'dg_rd32', 'dg_wr32'),
                              ('16', 'dg_rd16', 'dg_wr16'),
                              ('U16', 'dg_rd16', 'dg_wr16')):
                blob, k = re.subn(
                    r'DG%s\(%s(\s*\+\s*\d+)?\)\s*=\s*([^;]+);' % (w, v),
                    lambda m, wr=wr, v=v: '%s(%s%s, %s);'
                    % (wr, v, m.group(1) or '', m.group(2)), blob); n += k
                blob, k = re.subn(
                    r'DG%s\(%s(\s*\+\s*\d+)?\)' % (w, v),
                    lambda m, rd=rd, v=v: '%s(%s%s)'
                    % (rd, v, m.group(1) or ''), blob); n += k
        lines[a:b] = blob.split('\n')
    open(path, 'w').write('\n'.join(lines))
    return n

if __name__ == "__main__":
    for p in sys.argv[1:]:
        print("%s: %d fixes" % (p, fix(p)))
