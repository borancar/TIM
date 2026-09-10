"""Generate one shim per dispatched routine, from the port's own prototypes.

A shim is what Borland would have compiled for the call: one parameter per word
the guest pushed, in the order it pushed them, converted to what the port's
function actually takes, and then the return simulated the way the routine's own
`ret` would have left the machine.

Doing it as typed C rather than as a table of widths is the point. The compiler
checks that a far pointer is two words and becomes a host pointer, that a `long`
is two words and becomes a `uint32_t`, and that the argument count matches the
prototype - none of which a descriptor string can check, and all of which have
been wrong here at least once.

The types come from `reconstruct/tim.h`, so a routine whose signature changes
regenerates rather than rots. `routines.def` says only *where* each routine is and how it is
called; everything else lives in the shim it generates.

This file is the port's own tooling; it is not a transcription.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.dirname(os.path.abspath(__file__))

# The registers each register-argument routine takes, in the port's own order.
# Read off the entries when those routines were specced; they are not uniform,
# which is why each has its own list.
# Where each register-argument routine's arguments actually are, in the order
# the port's own function takes them. Read from the routine, and for the video
# driver from what its own transcription records - `vm_span` says "AL the
# colour, BX the x, CX the count, ES:DI the row" and means it.
#
#   bx      a word in that register
#   ds:si   a far pointer, that segment and offset
#   dx:ax   a 32-bit value, that register the high half
#   cf      the carry flag, as 0 or 1
#   stack   off the frame, in order, like an ordinary argument
#
# `stack` is what lets a routine mix the two. `far_move` takes four words off
# the frame and its count in CX, and until the token existed there was no way
# to say that: a routine was all-stack or all-register and that one is neither.
#
# The last two look alike and are told apart by the parameter: a pointer
# parameter makes `seg:off` a far pointer, a 32-bit one makes `hi:lo` a value.
# Borland's long arithmetic passes both halves in registers - `long_multiply`
# takes DX:AX and CX:BX and answers in DX:AX - and there was no way to say that
# here until now.
#
# **Seven of the driver's routines are in here and were dispatched as stack
# calls until now.** They have no `push bp` at all - `vm_blit_run` begins `jb
# 0x965`, which is the carry flag choosing its direction - so eight words were
# being read off the stack as arguments and the blitter drew runs wherever the
# rubbish pointed. That is what smeared the second intro screen.
REGS = {
    "poly_walk":              "es ax bx si bp cx di",
    "poly_edge_vertical":     "es bp si cx",
    "poly_edge_diagonal":     "es bx bp cx si",
    "poly_edge_steep":        "es bx bp cx si",
    "poly_edge_shallow_right":"es bx bp cx si",
    "poly_edge_shallow_left": "es bx bp cx si",
    "poly_outline":           "di si bp",

    "vm_span":                "ax bx cx es di",
    "vm_span_dithered":       "ax bx cx es di",
    "vm_blit_run":            "bx cx ds:si es di cf",
    "vm_blit_scaled_row":     "ax bp di es dx cx si ds",
    "vm_blit_glyph":          "es si ax bx dx bp",
    "vm_draw_line":           "bx cx dx si",
    "vm_fill_spans":          "es si",

    # Borland's long arithmetic. Read from the verifier's specs, which record
    # the registers measured against the original rather than guessed: the
    # spec for long_multiply builds its first argument from dx:ax and its
    # second from cx:bx, and that is what these say.
    "long_multiply":          "dx:ax cx:bx",
    "long_shift_right":       "dx:ax cl",
    "long_shift_left":        "dx:ax cl",
    # `ax+dx` is a `struct far_ptr` in two registers - the offset in the first
    # and the segment in the second. It is spelled with `+` rather than `:`
    # because `:` already means the high and low halves of one 32-bit value,
    # and a far pointer is not that: its two words do not concatenate.
    "huge_add":               "ax+dx cx:bx",

    # Four words off the frame and the count in CX - the verifier's spec
    # records exactly that, `args` at 4, 6, 8, 10 and `regs` of ["cx"].
    # Two `stack` tokens, not four: `dg_cfar` and `dg_far` are one C
    # parameter each and take two guest words each, which `aptr` reads.
    "far_move":               "stack stack cx",
}


def prototypes():
    """name -> (return type, [parameter types]) from the port's header."""
    src = open(os.path.join(ROOT, "reconstruct", "tim.h")).read()
    out = {}
    # **A return type is a type, not a word.** This matched `^[a-z_0-9]+\s+`,
    # which is one lowercase token - so `struct far_ptr huge_add(...)` came
    # back as "no prototype for huge_add" and the generator stopped, and
    # `volatile uint8_t near *string_concat(...)` did the same once the
    # typedefs were spelled out. Both are the right failure and the wrong
    # pattern.
    #
    # Taking the *last* identifier before the `(` as the name and everything
    # before it as the type reads both. Measured against the header as it now
    # stands, it finds 1120 prototypes where the old one finds 1106 and loses
    # none of them.
    #
    # The fourteen are *not* a pre-existing gap, which a first reading of that
    # number said they were: against the header as it *was*, `dg_near` is a
    # single lowercase token and the old pattern saw them all. The widening is
    # required by the rename, not a fix for something that was already broken -
    # and the proof is that the generated shims.c came out byte for byte
    # identical across it.
    # **The stars are part of the return type and have to be captured.** The
    # pattern used to end the type at the last word character and let a bare
    # `\**` eat what followed, so `volatile uint8_t *string_concat(...)` came
    # back as a return type of "volatile uint8_t" - no `*` in it anywhere. That
    # cost nothing while the RET_AX branch below keyed on a `near` tag, and
    # became a silent truncation of every returned pointer the moment it keyed
    # on the `*` instead.
    for m in re.finditer(
            r'^([A-Za-z_][^;()]*?[\w*])\s*(\**)\s*\b(\w+)\s*\(([^;]*?)\)\s*;',
            src, re.M | re.S):
        rt, name, args = m.group(1) + m.group(2), m.group(3), m.group(4)
        args = " ".join(args.split())
        if args in ("void", ""):
            out[name] = (rt, [])
            continue
        out[name] = (rt, [a.strip() for a in args.split(",")])
    return out


def near_type(param):
    """What `anearptr` must be cast to for this parameter.

    A writable near pointer differs from a `const` one; handing a
    `const uint8_t *` to the first drops a qualifier the compiler is right to
    complain about.

    **`volatile` is taken from the prototype, not assumed.** It used to be
    written into every cast, on the reading that guest memory is always
    volatile - and the day a header dropped it from a parameter the generated
    shim passed a `volatile uint8_t *` to a routine that no longer takes one,
    which is a discarded qualifier and a warning in generated code nobody
    edits. The header is the statement about the routine; this only spells it.
    """
    return pointee(param)


def far_type(param):
    """What `aptr` must be cast to for a far pointer parameter.

    `aptr` answers a `const uint8_t *`, which converts to a `const` far
    pointer on its own - adding a qualifier is allowed - but not to a writable
    one, which would drop the `const`. The cast is written for both so the two
    read alike, and `volatile` comes from the prototype for the reason in
    `near_type`.
    """
    return pointee(param)


def pointee(param):
    """What a pointer parameter points at, read off the declaration.

    It used to answer `uint8_t` for every pointer, with `const` and `volatile`
    put back by hand - which was true while every pointer the guest passed was
    a byte buffer. `draw_bitmap` takes a `struct bitmap near *` and the cast in
    its shim has to say so; answering `uint8_t` there is a type error in
    generated code, not a lost qualifier.

    Everything left of the `*` is the type, minus the memory-model tag, which
    is a spelling for Borland and not part of it.
    """
    if "*" not in param:
        # **An array parameter has no `*` to split on**, and everything left
        # of it would then be the whole declaration - `dg_off_t near list[]`
        # comes back as "dg_off_t list[]", which compiles into a cast that is
        # not a type. Refuse rather than emit it: the six list routines were
        # spelled that way for one commit and none of them is dispatched, so
        # this never fired, which is exactly the kind of luck worth removing.
        raise SystemExit("genshims: %r is a pointer with no `*` - spell an "
                         "array parameter as `T near * x`, not `T near x[]`"
                         % param.strip())
    t = param.split("*")[0]
    t = re.sub(r"\b(far|huge)\b", " ", t)
    return " ".join(t.split())


def kind_of(param):
    """How many guest words this parameter is, and how to build it.

    **A near pointer is one word and a far pointer is two**, and the only thing
    that tells them apart is the parameter's type. Read as a far pointer, a
    near one would swallow the argument after it.

    **Only `far` is marked**, so the test is one way round: a `far` tag is two
    words and any other pointer is one. It used to be the other way - `near`
    was the tag and an untagged `*` fell through to far.

    One dispatched routine had an untagged pointer when that changed:
    `vm_set_palette`, whose `rgb` was being built with `aptr` and eating two
    words where the guest pushes one. Its shim is the only line of the
    generated 229 that the change alters, which is both the proof that nothing
    else moved and the fix for that one.
    """
    # A `far` pointer is two words. The tag is what says so - it used to be
    # the `dg_far`/`dg_cfar` typedefs, which carried no `*` for the test below
    # to find; now the `*` is written out and the tag is the discriminator.
    if re.search(r"\bfar\b", param):
        return "p"
    # **`struct far_ptr` is two words and carries no `*` either.** It is the
    # other far-pointer spelling in this port - the one for a pair that is
    # *stored*, or whose halves are stepped or compared, where `dg_far`'s host
    # pointer cannot go. Without this branch it falls through to "w" below and
    # is marshalled as a **single** word: not an abort, just half an argument
    # and everything after it shifted.
    if "struct far_ptr" in param:
        return "s"
    # Untagged and a pointer: near, one word.
    if "*" in param:
        return "n"
    if "int32" in param:
        return "l"
    return "w"


def emit(entries, protos):
    out = []
    w = out.append
    w('/*')
    w(' * GENERATED by tools/native/genshims.py - do not edit.')
    w(' *')
    w(' * One shim per dispatched routine: the call Borland would have compiled,')
    w(' * with the return simulated before control goes back to the emulator.')
    w(' * See the generator for why this is typed C rather than a table of widths.')
    w(' */')
    w('#include <stdint.h>')
    w('#include <stdio.h>')
    w('')
    w('#include "native.h"')
    w('#include "shim.h"')
    w('#include "../../reconstruct/dgroup.h"')
    w('#include "../../reconstruct/tim.h"')
    w('')

    for e in entries:
        name = e["fn"]
        rt, params = protos.get(name, (None, None))
        if rt is None:
            raise SystemExit("no prototype for %s" % name)

        w('static void sh_%s(call_t *c)' % name)
        w('{')
        if name in REGS:
            regs = REGS[name].split()
            # The frame is set up whenever any argument comes off it, so a
            # mixed routine reads its stack words from the right place.
            if "stack" in regs:
                w('    %s_args(c);' % ("far" if e["far"] else "near"))
                w('')
            if len(regs) != len(params):
                raise SystemExit("%s: %d registers for %d parameters"
                                 % (name, len(regs), len(params)))
            for i, (r, p) in enumerate(zip(regs, params)):
                if "+" in r:
                    off, seg = r.split("+")
                    if "struct far_ptr" not in p:
                        raise SystemExit("%s: `%s` is a far pointer pair and "
                                         "parameter %d is %s"
                                         % (name, r, i, p))
                    w('    struct far_ptr a%d;' % i)
                    w('    a%d.off = areg(c, UC_X86_REG_%s);'
                      % (i, off.upper()))
                    w('    a%d.seg = areg(c, UC_X86_REG_%s);'
                      % (i, seg.upper()))
                elif ":" in r:
                    hi, lo = r.split(":")
                    # The `far` tag, the same discriminator `kind_of` uses.
                    # This branch aborts if the parameter is neither a far
                    # pointer nor a 32-bit value, rather than guessing.
                    if re.search(r"\bfar\b", p):
                        w('    %s *a%d = (%s *)aregptr(c, UC_X86_REG_%s, '
                          'UC_X86_REG_%s);'
                          % (far_type(p), i, far_type(p), hi.upper(),
                             lo.upper()))
                    elif "*" in p:
                        w('    const uint8_t *a%d = aregptr(c, UC_X86_REG_%s, '
                          'UC_X86_REG_%s);' % (i, hi.upper(), lo.upper()))
                    elif "int32" in p:
                        w('    uint32_t a%d = ((uint32_t)areg(c, UC_X86_REG_%s)'
                          ' << 16) | areg(c, UC_X86_REG_%s);'
                          % (i, hi.upper(), lo.upper()))
                    else:
                        raise SystemExit("%s: %s is a register pair but the "
                                         "parameter is %s - a pair is a far "
                                         "pointer or a 32-bit value, and %s is "
                                         "neither" % (name, r, p, p))
                elif r == "cf":
                    w('    uint32_t a%d = acarry(c);' % i)
                elif r == "stack":
                    k = kind_of(p)
                    if k == "n":
                        w('    %s *a%d = (%s *)anearptr(c);'
                          % (near_type(p), i, near_type(p)))
                    elif k == "p":
                        if re.search(r"\bfar\b", p):
                            w('    %s *a%d = (%s *)aptr(c);'
                              % (far_type(p), i, far_type(p)))
                        else:
                            w('    const uint8_t *a%d = aptr(c);' % i)
                    elif k == "s":
                        w('    struct far_ptr a%d;' % i)
                        w('    a%d.off = aword(c);' % i)
                        w('    a%d.seg = aword(c);' % i)
                    elif k == "l":
                        w('    uint32_t a%d = alng(c);' % i)
                    else:
                        w('    uint16_t a%d = aword(c);' % i)
                else:
                    w('    uint16_t a%d = areg(c, UC_X86_REG_%s);'
                      % (i, r.upper()))
        else:
            w('    %s_args(c);' % ("far" if e["far"] else "near"))
            if params:
                w('')
            for i, p in enumerate(params):
                k = kind_of(p)
                if k == "n":
                    w('    %s *a%d = (%s *)anearptr(c);'
                      % (near_type(p), i, near_type(p)))
                elif k == "p":
                    if re.search(r"\bfar\b", p):
                        w('    %s *a%d = (%s *)aptr(c);'
                          % (far_type(p), i, far_type(p)))
                    else:
                        w('    const uint8_t *a%d = aptr(c);' % i)
                elif k == "s":
                    w('    struct far_ptr a%d;' % i)
                    w('    a%d.off = aword(c);' % i)
                    w('    a%d.seg = aword(c);' % i)
                elif k == "l":
                    w('    uint32_t a%d = alng(c);' % i)
                else:
                    w('    uint16_t a%d = aword(c);' % i)
        w('')

        call = "%s(%s)" % (name, ", ".join("a%d" % i for i in range(len(params))))
        far = "f" if e["far"] else "n"
        pops = e["pops"]
        if e["ret"] == "RET_NONE":
            w('    %s;' % call)
            w('    r%s_void(c, %d);' % (far, pops))
        elif e["ret"] == "RET_AX":
            # **A pointer answer becomes an offset again.** The guest gets its
            # result in AX and expects a DGROUP offset there; a routine that
            # now returns `dg_near` is handing back a host address, and
            # truncating one to sixteen bits is a number with no meaning.
            # `dg_off` is the inverse of the `anearptr` above.
            # **Keyed on the return being a pointer**, not on a tag. This
            # test has now silently stopped firing twice - once when the
            # `dg_near` typedef was spelled out, and once when the `near` tag
            # was dropped - and each time every one of these truncated a host
            # pointer to sixteen bits instead, which is the exact thing the
            # paragraph above says must not happen. A tag can be removed; a
            # `*` in the return type cannot.
            if rt and "*" in rt and not re.search(r"\bfar\b", rt):
                w('    r%s_ax(c, dg_off(dgroup, %s), %d);' % (far, call, pops))
            else:
                w('    r%s_ax(c, (uint16_t)%s, %d);' % (far, call, pops))
        elif rt and "union far_or_size" in rt:
            # The union's two members overlay, and `.bytes` *is* the DX:AX the
            # guest expects - which member the C caller reads is the caller's
            # business and none of the shim's.
            w('    r%s_dxax(c, %s.bytes, %d);' % (far, call, pops))
        elif rt and "struct far_ptr" in rt:
            # DX:AX is the segment and the offset, not a 32-bit number - the
            # same distinction the `+` token above draws on the way in.
            w('    {')
            w('        struct far_ptr r = %s;' % call)
            w('')
            w('        r%s_dxax(c, ((uint32_t)r.seg << 16) | r.off, %d);'
              % (far, pops))
            w('    }')
        else:
            w('    r%s_dxax(c, (uint32_t)%s, %d);' % (far, call, pops))
        w('}')
        w('')

    w('/* The table dispatch.c walks: where each routine is, its shim, and')
    w(' * which layer it belongs to - see genshims.py for how that is')
    w(' * derived, and dispatch.c for what selects on it. */')
    w('const shim_entry shim_table[] = {')
    for e in entries:
        w('    { %#07x, "%s", sh_%s, %d, "%s" },'
          % (e["at"], e["fn"], e["fn"], 1 if e["overlay"] else 0,
             e["layer"]))
    w('};')
    w('')
    w('const int32_t shim_count = (int32_t)(sizeof shim_table /')
    w('                                     sizeof shim_table[0]);')
    return "\n".join(out) + "\n"


"""Which layer each dispatched routine belongs to.

**Derived, not declared.** The layer is which of the port's translation units
defines the function, and that is already a fact on disk - so asking the
sources beats adding a second list beside `routines.def` for the two to drift
apart. A routine that moves file moves layer without anyone editing anything.

The names are the ones a run is selected by:

  vm    the video driver, VM.OVL          reconstruct/src/vmovl_*.c
  sx    the sound driver and module       reconstruct/src/sxovl*.c
  dos   the C library's file layer        reconstruct/borland_file.c
  mem   its allocator and long arithmetic reconstruct/borland_heap.c, _huge.c
  game  everything the game itself is     the rest of reconstruct/src

`io` in `TIM_NATIVE_LAYERS` is shorthand for the four that are not `game`,
which is the split this exists for: the port's hardware and memory under the
original's own logic.
"""
LAYER_OF_FILE = {
    "borland_file.c": "dos",
    "borland_heap.c": "mem",
    "borland_huge.c": "mem",
}

# The routines the file cannot classify, and why there has to be a list.
#
# **The port's files mirror the original's translation units, not its layers.**
# `engine.c` is one code segment of the original and holds `dos_alloc_bytes`
# next to the game's own code, because that is where the original put it -
# CLAUDE.md is explicit that a routine's file is the one whose address range
# contains it and that moving one to suit a name is wrong. So for these the
# file says `game` and the truth is `mem`.
#
# Each of these was found by running with `TIM_NATIVE_LAYERS=io` and reading
# what trapped, not by guessing from the names: `dos_alloc_bytes` was the first
# trap, an `int 21h ah=48` out of `game_startup` that no layer serviced.
#
# `irq` is the timer and the keyboard - the two interrupt sources the game
# installs. They are the machine as much as the video driver is, and they are
# a layer of their own because a run may want the port's drawing without its
# clock, which is the difference `native.c` already relies on by not calling
# `io_set_timer`.
LAYER_OF_FN = {
    "dos_alloc_bytes":       "mem",
    "dos_free_far":          "mem",
    "normalise_far_ptr_far": "mem",

    "vm_init":               "vm",
    "blit_rows_thunk":       "vm",
    # routines.def's own note says why this one and not the blitter under it:
    # "dispatched at `draw_bitmap_scaled` rather than at the blitter, because
    # this is the one that chooses between them". The choice is part of the
    # video layer, so the layer has to follow the dispatch point rather than
    # the routine that does the port writes - which is `blit_scaled_b`, and is
    # deliberately not dispatched at all.
    "draw_bitmap_scaled":    "vm",

    "install_keyboard":      "irq",
    "timer_install":         "irq",
    "timer_add_callback":    "irq",
    "timer_callback":        "irq",

    # INT 33h. routines.def's own comment on this group says it: "there is no
    # mouse driver here; the io layer is it" - so every one of these is the
    # port standing in for a device, which is what this selection means.
    "mouse_init":            "mouse",
    "mouse_set_speed":       "mouse",
    "mouse_set_ranges":      "mouse",
    "mouse_set_user_handler": "mouse",
    "mouse_move_to":         "mouse",
    "mouse_event":           "mouse",
    "remove_mouse":          "mouse",
}


def layers(entries):
    import glob

    where = {}
    ports = set()
    for path in (glob.glob(os.path.join(ROOT, "reconstruct", "src", "*.c"))
                 + glob.glob(os.path.join(ROOT, "reconstruct", "*.c"))):
        base = os.path.basename(path)
        text = open(path).read()
        for m in re.finditer(r'^[A-Za-z_][A-Za-z0-9_ *]*?\b(\w+)\(', text, re.M):
            where.setdefault(m.group(1), base)
        # A routine whose own body reads or writes a hardware port is the
        # machine, whatever translation unit it sits in. Derived rather than
        # listed, because it is visible in the C and a list would rot: it
        # catches `restore_write_mode`, `vm_set_line_compare` and the two
        # `mouse_*_vga` routines, all of which live in game segments and all of
        # which were being left to the emulator by the file rule alone.
        for m in re.finditer(
                r'^(?:static\s+)?[A-Za-z_][A-Za-z0-9_ *]*?\b(\w+)\([^;{]*\)\s*\n'
                r'\{(.*?)^\}', text, re.S | re.M):
            if re.search(r'\bio_(?:out|in)\w*\s*\(', m.group(2)):
                ports.add(m.group(1))

    for e in entries:
        base = where.get(e["fn"])
        if e["fn"] in LAYER_OF_FN:
            e["layer"] = LAYER_OF_FN[e["fn"]]
        elif base is None:
            # A routine the port does not define is a routines.def entry that
            # cannot link anyway; the compiler says so far more clearly than
            # this would. Leave it in `game` and let the build fail.
            e["layer"] = "game"
        elif base.startswith("vmovl"):
            e["layer"] = "vm"
        elif base.startswith("sxovl"):
            e["layer"] = "sx"
        elif base in LAYER_OF_FILE:
            e["layer"] = LAYER_OF_FILE[base]
        elif e["fn"] in ports:
            e["layer"] = "hw"
        else:
            e["layer"] = "game"
    return entries


def parse_table():
    """The entries in routines.def, as data.

    Split on commas rather than matched positionally: a positional regex over
    four different macros is the kind of thing that silently drops an entry,
    and a dropped entry here is a routine the emulator quietly runs itself.

    Commented-out entries are skipped, which is not obvious and cost a wrong
    diagnosis. Withdrawing a routine to see where it traps is the ordinary way
    to work here, and `//` in front of it used to do nothing at all - the
    regex found the macro just the same, the shim was still generated, and the
    run looked like a trap that would not fire.
    """
    src = open(os.path.join(HERE, "routines.def")).read()
    src = re.sub(r'//[^\n]*', '', src)
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
    out = []
    for m in re.finditer(r'\b(FAR_C|OVL_C|OVL_R|NEAR_P|REG_N|FAR_R|FAR_P)'
                         r'\s*\(([^)]*)\)', src):
        kind = m.group(1)
        f = [x.strip() for x in m.group(2).split(",")]
        # The file documents the macro forms in its own header, and a regex
        # over the whole file matches those too. Anything whose first field is
        # not an address is prose, not an entry.
        if not re.fullmatch(r'0x[0-9a-f]+', f[0]):
            continue
        at = int(f[0], 16)
        if kind == "OVL_R":
            # A driver routine with register arguments: far, in the overlay,
            # and every one of them answers nothing.
            out.append(dict(at=at, far=1, overlay=1, pops=0, regs=1,
                            ret="RET_NONE", fn=f[1]))
        elif kind == "FAR_R":
            # Far, and its arguments are in registers. Borland's long
            # arithmetic is called this way from other modules.
            out.append(dict(at=at, far=1, overlay=0, pops=0, regs=1,
                            ret=f[1], fn=f[2]))
        elif kind in ("FAR_C", "OVL_C"):
            out.append(dict(at=at, far=1, overlay=(kind == "OVL_C"), pops=0,
                            regs=0, ret=f[2], fn=f[3]))
        elif kind == "FAR_P":
            # Far, and the callee removes the arguments: `retf N`.
            out.append(dict(at=at, far=1, overlay=0, pops=int(f[2]), regs=0,
                            ret=f[3], fn=f[4]))
        elif kind == "NEAR_P":
            out.append(dict(at=at, far=0, overlay=0, pops=int(f[2]), regs=0,
                            ret=f[3], fn=f[4]))
        else:
            out.append(dict(at=at, far=0, overlay=0, pops=0, regs=1,
                            ret=f[3], fn=f[4]))
    return out


def check_thunks(entries):
    """Refuse a far declaration whose address is a near-to-far thunk.

    `pop bx / push cs / push bx` is Borland turning a near call into the far
    frame a `retf` body needs. The three instructions are the whole routine at
    that address; the body is after them. Declared far, the shim reads four
    bytes of return address where the caller pushed two, and the guest limps on
    a corrupted stack - which presents as a slowdown, not as a crash, and cost
    an afternoon once.

    Nothing else catches it: both exits are `retf`, so reading the end of the
    routine says the opposite of the truth, and the screens go on matching for
    a while because the damage is to the stack rather than to the pixels.
    """
    path = os.path.join(ROOT, "out", "TIM.img")
    if not os.path.exists(path):
        print("no out/TIM.img - skipping the thunk check")
        return
    img = open(path, "rb").read()

    for e in entries:
        if e["overlay"] or not e["far"]:
            continue
        b = img[e["at"]:e["at"] + 3]
        # `pop <r> / push cs / push <r>`, for any of the eight registers -
        # 0x58+r pops it and 0x50+r pushes it back. The check was written for
        # `pop bx` because that is the one that had bitten, and `ulong_divide`
        # at 0x0bd97 is the same thunk built on CX: it would have gone
        # straight through a guard that only knew one register.
        if (len(b) == 3 and 0x58 <= b[0] <= 0x5f and b[1] == 0x0e
                and b[2] == b[0] - 8):
            reg = ("ax", "cx", "dx", "bx", "sp", "bp", "si", "di")[b[0] - 0x58]
            raise SystemExit(
                "%s at %#07x is a near-to-far thunk - `pop %s / push cs / "
                "push %s` - and is declared far. Its caller makes a near "
                "call. Declare it near (REG_N or NEAR_P), or dispatch the "
                "body that follows the thunk instead."
                % (e["fn"], e["at"], reg, reg))


def main():
    entries = parse_table()
    check_thunks(entries)
    # the two that already had shims are the port functions themselves
    for e in entries:
        e["fn"] = re.sub(r'^sh_', '', e["fn"])
    protos = prototypes()
    layers(entries)
    text = emit(entries, protos)
    dst = os.path.join(HERE, "shims.c")
    open(dst, "w").write(text)
    counts = {}
    for e in entries:
        counts[e["layer"]] = counts.get(e["layer"], 0) + 1
    print("wrote %s: %d shims (%s)"
          % (os.path.relpath(dst, ROOT), len(entries),
             ", ".join("%s %d" % kv for kv in sorted(counts.items()))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
