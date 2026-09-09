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

def arg_index(text, at):
    """Which argument of the enclosing call this position sits in.

    A verdict has to follow the *address*, not any parameter: `write_word`
    hands its `addr` to `game_fwrite`, which calls `archive_entry_for(file)`,
    and a walk that follows every argument reports the file handle's fate as
    the address's. Counting commas at depth zero back to the `(` says which
    one to follow.
    """
    depth = 0
    n = 0
    i = at - 1
    while i >= 0:
        c = text[i]
        if c == ')':
            depth += 1
        elif c == '(':
            if depth == 0:
                return n
            depth -= 1
        elif c == ',' and depth == 0:
            n += 1
        elif c == ';':
            return 0
        i -= 1
    return 0


blocked = collections.Counter(); free = []; total = 0
where = collections.defaultdict(set)
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
                    outs.add((f, arg_index(blob, m.start())))
        if outs:
            # by *frame*, not by call site: a callee reached at two argument
            # positions in one routine still holds up one frame
            for f in {o[0] for o in outs}:
                blocked[f] += 1
            for f in outs:
                where[f[0]].add(f[1])
        else:
            free.append((os.path.basename(path), me))
print("%d routines still call dg_enter" % total)
print("%d are unblocked - every slot they hand out goes to a pointer already\n"
      % len(free))
for p, m in free:
    print("   %-16s %s" % (p, m))
# **Which blockers can move at all.**
#
# A callee holds a frame up because it takes an offset. Most of them can be
# given a pointer, and the frames above them follow. Two kinds cannot, and
# reading those as "not yet" is what would make this list look like a worklist
# to the end:
#
#   filed  - the routine stores the address into guest memory, where it stays
#            after the call. `stdio_setvbuf` puts the buffer in the file
#            record's `read_ptr`, and the record is read back later as a
#            DGROUP offset. A C array has no offset to store.
#   far    - the routine needs a segment too. `draw_string` hands its string
#            to `draw_string_body(str, DGROUP_SEG, ...)`, which reads it with
#            `FAR8(seg, str)`, so the argument is half of a seg:off pair.
#
# Both are decisions about the model - whether a frame may live in DGROUP, or
# whether the far convention becomes a pointer as well - and not transcription.
bodies = {}
params = {}
for path in sorted(glob.glob(os.path.join(R, 'src', '*.c'))
                   + glob.glob(os.path.join(R, '*.c'))):
    text = open(path).read()
    for m in re.finditer(r'\n([a-zA-Z_][\w ]*[ *](\w+)\(([^;{]*)\)\s*\n\{\n)'
                         r'(.*?)\n\}\n', text, re.S):
        bodies.setdefault(m.group(2), m.group(4))
        params.setdefault(m.group(2),
                          [a.strip().split()[-1].lstrip('*')
                           for a in m.group(3).split(',')
                           if a.strip() and a.strip() != 'void'])

FAR = re.compile(r'\bFAR(?:8|16|U16|32|_PTR)\s*\(\s*[^,]*,\s*(\w+)')


def split_args(text):
    """The argument list, split on the commas that are not inside a call."""
    out, depth, cur = [], 0, ""
    for c in text:
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        if c == ',' and depth == 0:
            out.append(cur)
            cur = ""
            continue
        cur += c
    out.append(cur)
    return out


def _local(name, idx):
    """What this routine's own body does with argument `idx`."""
    b, ps = bodies.get(name), params.get(name) or []
    if b is None:
        return "far - it is half of a seg:off pair" if name == "FAR_PTR" else ""
    if idx >= len(ps):
        return ""
    v = ps[idx]
    for m in FAR.finditer(b):
        if m.group(1) == v:
            return "far - it is half of a seg:off pair"
    if re.search(r'(?:DG[0-9A-F]{4}\.\w+|\w+\([^()]*\)\.\w+'
                 r'|DG(?:8|16|U16|32)\([^()]*\))\s*=\s*\(?[^=;]*\b'
                 + v + r'\b\s*[;)]', b):
        return "filed - it stores the address in guest memory"
    return ""


def why(name, idx, seen=None):
    """That, or whatever the routine it forwards *this argument* to does.

    One step is not enough: `stdio_setbuf_for` only passes its buffer on and
    the storing happens two calls down, in `stdio_setvbuf`. Following the
    argument rather than the routine is what keeps it honest - `write_word`
    also calls `archive_entry_for`, with the file handle, and a walk that did
    not track which argument reported that call's fate as the address's.
    """
    seen = seen or set()
    if (name, idx) in seen:
        return ""
    seen.add((name, idx))
    v = _local(name, idx)
    if v:
        return v
    b, ps = bodies.get(name), params.get(name) or []
    if b is None or idx >= len(ps):
        return ""
    me = ps[idx]
    # One level of nesting in the argument list, because
    # `stdio_setvbuf(file, buf, (int16_t)(buf != 0 ? 0 : 2), 0x200)` has one
    # and a flat pattern does not see the call at all - which is how the
    # buffer that ends up in a file record read as unblocked.
    for m in re.finditer(r'\b(\w+)\s*\(((?:[^;()]|\([^;()]*\))*)\)', b):
        if m.group(1) in ('if', 'while', 'for', 'switch', 'return', 'sizeof'):
            continue
        for k, a in enumerate(split_args(m.group(2))):
            if re.search(r'(?<![\w.])' + me + r'(?![\w])', a):
                got = why(m.group(1), k, seen)
                if got:
                    return got
    return ""


print("\nblocking callees, by how many frames each holds up:")
stuck = 0
for f, n in blocked.most_common(24):
    v = ""
    for i in sorted(where[f]):
        v = why(f, i)
        if v:
            break
    if v:
        stuck += 1
    print("   %-28s %2d frames%s" % (f, n, ("  " + v) if v else ""))
print("\n%d distinct blockers, %d of them held by the model rather than by "
      "work left to do" % (len(blocked), stuck))
