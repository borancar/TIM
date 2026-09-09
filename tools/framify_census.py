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
import re, glob, collections, os, sys
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
held = {}
reserves = []
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
        # `dg_enter` itself is the definition, not a frame.
        if me == 'dg_enter':
            total -= 1
            continue

        # A slot whose only appearance is `(void)fp;` is not a slot: it is a
        # reservation the routine makes for what it calls, with the cast there
        # to keep the compiler quiet. `game_screen` is the one.
        slots = [v for v in slots
                 if re.sub(r'\(void\)\s*%s\s*;' % re.escape(v), '', blob)
                    .count(v) > blob.count('(void)%s;' % v)
                 and re.search(r'(?<![\w.])%s(?![\w])' % re.escape(v),
                               re.sub(r'\(void\)\s*%s\s*;' % re.escape(v),
                                      '', blob.replace(
                                          'uint16_t %s = dg_enter' % v, '')))]

        # **A `dg_enter` with no slots of its own is not waiting on a
        # callee's signature.** `game_screen` writes `uint16_t fp =
        # dg_enter(0x16); (void)fp;` and `poll_sequences` builds a block
        # inline: the first reserves DGROUP stack so that the frames its
        # callees still make land below its own, and the second hands its
        # block to the sound module, which reads it through SI. Neither goes
        # until what is under it stops needing DGROUP - so counting them among
        # the unblocked overstates what is left to do.
        if not slots:
            reserves.append((os.path.basename(path), me))
            continue

        if outs:
            # by *frame*, not by call site: a callee reached at two argument
            # positions in one routine still holds up one frame
            for f in {o[0] for o in outs}:
                blocked[f] += 1
            for f in outs:
                where[f[0]].add(f[1])
            held[(os.path.basename(path), me)] = outs
        else:
            free.append((os.path.basename(path), me))
print("%d routines still call dg_enter" % total)
# "unblocked" says only that no *callee* holds these up. `framify.py` may
# still refuse them over their own slots - a filed address, a slot reached
# through a cursor that something takes as an offset - and it says which when
# it is run on one. So this is where to look next, not a list of easy wins.
print("%d have no blocking callee - run framify.py on one for its own "
      "refusal\n" % len(free))
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


#: Verdicts this file cannot derive, with the reason each was read by hand.
#: A wrong automatic answer is worse than a named exception, and both of these
#: are about what the *value* means rather than about what the body does with
#: it - which no pattern over the body can see.
BY_HAND = {
    # `stdio_setbuf_for` reaches `stdio_setvbuf`, which puts the buffer into
    # the file record's `read_ptr` and `word_08`. Reading every site rather
    # than that one: `read_ptr` is a **cursor**, stepped a byte at a time
    # (`FILEREC(file).read_ptr++`) and reset to `word_08` in five places; it is
    # **compared numerically** against `(uint16_t)(file + 5)`, the record's own
    # inline buffer, which is how the layer tells a set buffer from the default
    # one; and `word_08` is handed to `heap_free` as a heap handle. Three
    # different things a host pointer cannot be.
    ("stdio_setbuf_for", 1): "the file record's read cursor - stepped, "
                             "compared against the record's own address, and "
                             "freed as a heap handle",
    # `read_resource` normalises its `dst_off, dst_seg` into DGROUP
    # 0x5894/0x5896, and that pair is not a handoff to one routine - it is the
    # **decompression output cursor**. Fourteen sites touch it: `read_into_huge`
    # and `far_memcpy` are handed it, `far_memset` and a `FAR_PTR` store write
    # through it, and `decompress_lzw` and `decompress_lzss` *advance* it and
    # renormalise it - `linear = (seg << 4) + off + si` and back. So the
    # destination has to be a `seg:off` the guest can walk, and the byte it
    # points at has to be somewhere the guest can address.
    ("read_resource", 1): "the decompression cursor at DGROUP 0x5894, walked "
                          "and renormalised by three decompressors",
    # `huge_move` answers `(dst_seg << 16) | dst_off` - its *return value* is
    # the pair it was given, which a host pointer does not remember.
    ("huge_move", 0): "returned as a seg:off pair, which a pointer cannot "
                      "reconstruct",
    # And the source is no better, though it looks it: the routine does not
    # *read* the source, it indexes guest memory with the source's linear
    # address and takes the copy's direction from comparing that address with
    # the destination's. A frame handed in as a host pointer is a C array with
    # no guest address, so `src - guest_mem` is a wild number. Tried on
    # 2026-09-09; the verifier segfaulted, which is the good outcome.
    ("huge_move", 2): "used as a guest address, not read as bytes",
    # `load_bitmaps` takes either a file handle or the DGROUP offset of a
    # filename, and tells them apart by asking `file_record_valid` whether the
    # number matches an open record's `file_ptr`. That is a numeric comparison
    # against guest state, so the argument has to be a guest offset: a C
    # array's `dg_off` is an arbitrary 16-bit number that could match a live
    # handle, and the polymorphism cannot be spelled in a pointer type at all.
    # Ten call sites, and **nine pass a DGROUP string constant** - 0x00f5
    # "cp.bmp", 0x254a "sierra.bmp", 0x2582 "icons.bmp" and so on. Only
    # `load_part_bitmap` passes a buffer, and it is the frame in question. The
    # routine asks `file_record_valid` whether the number matches an open
    # record's `file_ptr`, which holds what `game_fopen` returned - a FILEREC
    # offset. So the argument is a handle *or* a filename address, told apart
    # numerically: sound for the nine constants, where `dg_off(dg_ptr(x))` is
    # `x` again, and unsound for a C array, whose arbitrary 16-bit distance
    # could match a live handle.
    # Measured on 2026-09-09: the test **never fires**. Every call on the
    # intro - four constants and 51 from `load_part_bitmap`, whose buffer sits
    # at DGROUP 0xffe6 - answers `file_record_valid` = 0 and takes the
    # `open_file_record` path. So the polymorphism is real in the code and
    # unexercised in this data, and the only thing keeping `load_part_bitmap`
    # in DGROUP is that a C array's `dg_off` is an arbitrary 16-bit number
    # that *could* match one of the four live handles. "Unlikely" is not the
    # standard here.
    ("load_bitmaps", 0): "a handle or a filename address, told apart by a "
                         "numeric test against live file records",
}


def _local(name, idx):
    v = BY_HAND.get((name, idx))
    if v:
        return v
    return _local_body(name, idx)


def _local_body(name, idx):
    """What this routine's own body does with argument `idx`."""
    b, ps = bodies.get(name), params.get(name) or []
    if b is None:
        return "far - it is half of a seg:off pair" if name == "FAR_PTR" else ""
    if idx >= len(ps):
        return ""
    v = ps[idx]
    # **The pair can be named rather than dereferenced.** `read_resource`
    # takes `dst_off, dst_seg` and hands both to `normalise_far_ptr_far`
    # without a `FAR8` anywhere in its body, so looking only for the macro
    # reads it as convertible. The two words next to each other in the
    # parameter list are the far pointer, whatever the routine then does with
    # them.
    if idx + 1 < len(ps) and v.endswith("off") and ps[idx + 1].endswith("seg"):
        # **A pair that is walked carries the 64K wrap**, and that is a
        # stronger reason than being far. `far_move` and `far_memcpy` step
        # their offsets with `(uint16_t)(off + n)` - the cast is the
        # transcription of the original's 16-bit `add`, and a host pointer
        # cannot express it, because it does not know where the segment
        # starts. Converting them would silently drop a documented behaviour.
        if re.search(r'\(uint16_t\)\(\s*' + v + r'\s*\+', b):
            return "wraps - the offset is stepped 16-bit, which a pointer " \
                   "cannot do"
        return "far - it is half of a seg:off pair"
    for m in FAR.finditer(b):
        if m.group(1) == v:
            return "far - it is half of a seg:off pair"
    # **A write *through* the pointer is not a write *of* it.** `heapwalk`
    # does `DGU16(info) = (uint16_t)(DGU16(info) + 4)`, and the parameter on
    # the right is inside an accessor on itself - the routine is stepping the
    # record it was handed, not filing its address anywhere. Reading that as
    # filing is what kept `heap_largest_free` walled, and it is the third
    # false "filed" of the same afternoon. So strip every accessor *on this
    # parameter* before asking whether the parameter is stored.
    stripped = re.sub(r'DG(?:8|S8|16|U16|32)\s*\(\s*(?:\(uint16_t\)\(\s*)?'
                      + v + r'\b[^()]*\)?\s*\)', '@', b)
    if re.search(r'(?:DG[0-9A-F]{4}\.\w+|\w+\([^()]*\)\.\w+'
                 r'|DG(?:8|16|U16|32)\([^()]*\))\s*=\s*\(?[^=;]*\b'
                 + v + r'\b\s*[;)]', stripped):
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


# **And what that costs, counted in frames rather than callees.** A frame whose
# every blocker is one of the two above cannot become an array at all, and
# saying so is the difference between a worklist that ends and one that looks
# unfinished for ever.
walled = []
work = []
for (p_, m), outs in sorted(held.items()):
    if all(why(f, i) for f, i in outs):
        walled.append((p_, m, sorted({why(f, i) for f, i in outs})[0]))
    else:
        work.append((p_, m))
print("\n%d frames are waiting on work, %d are held by the model"
      % (len(work), len(walled)))
for p_, m in work:
    print("   %-16s %-28s waiting on %s"
          % (p_, m, ", ".join(sorted({f for f, i in held[(p_, m)]
                                      if not why(f, i)}))))
for p_, m, v in walled:
    print("   %-16s %-28s %s" % (p_, m, v))

print("\n%d reserve DGROUP stack with no slots of their own" % len(reserves))
# **And what is still under them**, computed rather than asserted. A routine
# that reserves only so its callees' frames land below its own can stop the
# day nothing it reaches reserves any more, and that is a closure over the
# call graph, not a judgement.
still = {m for m in bodies if 'dg_enter(' in bodies[m]} - {'dg_enter'}
for p_, m in reserves:
    seen_fn, stack = set(), [m]
    while stack:
        fn = stack.pop()
        if fn in seen_fn:
            continue
        seen_fn.add(fn)
        for c in set(re.findall(r'\b(\w+)\s*\(', bodies.get(fn, ''))):
            if c in bodies and c not in seen_fn:
                stack.append(c)
    under = sorted((still & seen_fn) - {m})
    print("   %-16s %-18s waits on %s"
          % (p_, m, ", ".join(under) if under else "nothing - it can go"))

print("\n%d + %d + %d + %d = %d, which is every frame left"
      % (len(free), len(work), len(walled), len(reserves),
         len(free) + len(work) + len(walled) + len(reserves)))


#: **Every routine that still calls `dg_enter`, and why.** The lists above are
#: derived - which callee blocks which frame - and this is the roll call: a
#: routine here is one somebody has read and written a reason for. `--assert`
#: fails when the two disagree, so a *new* `dg_enter` has to be read before the
#: build is green again, and a routine that converts has to be struck off. The
#: long form of each reason is in the routine's own comment and in CLAUDE.md.
WALLED = {
    "read_sound_records":
        "its one byte is written by a decompressor through DGROUP 0x5894",
    "seek_to_sound_record":
        "its three bytes are written by a decompressor through DGROUP 0x5894",
    "read_level":
        "the stdio buffer is the file layer's read cursor",
    "load_animation_into":
        "the stdio buffer is the file layer's read cursor",
    "decode_vqt_list":
        "`rd` goes into DG6400.word_640c, read back by two siblings",
    "load_palette":
        "`buf` is indexed by its guest address in huge_move",
    "vm_init":
        "`bp` lands on the original's own BP, which DG618A.fonts_off is set "
        "from",
}

if "--assert" in sys.argv:
    have = {m for m in bodies if "dg_enter(" in bodies[m]} - {"dg_enter"}
    missing = sorted(have - set(WALLED))
    stale = sorted(set(WALLED) - have)
    for m in missing:
        print("FAIL: %s calls dg_enter and WALLED has no reason for it" % m)
    for m in stale:
        print("FAIL: WALLED lists %s, which no longer calls dg_enter" % m)
    if missing or stale:
        sys.exit(1)
    print("%d routines call dg_enter, and each has a reason" % len(have))
