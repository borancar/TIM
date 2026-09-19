# Lessons: the traps this project has hit

The long form of the list in `CLAUDE.md`. Each entry is kept as it was written
when the trap was met, with the measurements that settled it; `CLAUDE.md`
carries the one-line rule and a link here. Where an entry describes something
still open rather than a lesson, it lives in `STATUS.md` instead.

## Reading and running the original

What the binary says, what the listing tools say about it, and what the emulated machine has to model for the game to run at all.

### A DOS program with `maxalloc = 0xFFFF` owns all of conventional memory

**A DOS program with `maxalloc = 0xFFFF` owns all of conventional memory**
until its runtime hands the tail back with INT 21h AH=4Ah. Modelling the free
arena as starting just above `image+minalloc` puts DOS's blocks *inside* the
program's own DGROUP - and Borland's large-model startup puts the stack at
the top of a 64 KB DGROUP. The symptom was a `retf` into zeroed memory a
million instructions later, which looks like anything but an allocator bug.
`TimMachine._dos` models it properly.

### The game asks the BIOS what adapter it has

**The game asks the BIOS what adapter it has** (INT 10h AH=1Ah, then AH=12h
BL=10h). Left unimplemented these leave BX as the caller set it, the game
concludes there is no VGA *and* no EGA, fails to load `VM.OVL` and prints
"Unable to initialize vm.". Both are plain VGA BIOS services.

### `files missing:` in a run report is usually not an error

**`files missing:` in a run report is usually not an error.** The game tries
each resource as a loose file first and falls back to the archive, so a long
list of missing `.BMP` and `.LEV` names is the normal path.

### Capstone's 16-bit mode gets `cbw`/`cwd` wrong

**Capstone's 16-bit mode gets `cbw`/`cwd` wrong.** It prints the 32-bit
mnemonics - `cwde` for 0x98 and `cdq` for 0x99 - where 16-bit code means
`cbw` and `cwd`, and prints the 16-bit ones when a 0x66 prefix makes them
32-bit. `tools/disasm.py` corrects this from the instruction's own bytes.
Uncorrected, a listing says a routine sign-extends AX into EDX when it
sign-extends into DX, and a transcription that believes it gets the width
wrong, compiles, and runs.

### A segment immediate in the image is a relocation, not a value

**A segment immediate in the image is a relocation, not a value.** The
recovered image is unrelocated, so `mov word [0x4bbe], 0` in the listing is
really "the program's own base": the loader patches those two bytes. The
disassembly gives no sign of it. Transcribing the zero as written is wrong in
a way nothing catches until the cell is compared - and then it reads 0x0110,
which is the load segment and looks like nonsense until you see why. Work any
segment out from where the program actually is.

### A jump that lands one byte past the last instruction you read means there is an instruction you have not read

**A jump that lands one byte past the last instruction you read means there
is an instruction you have not read.** `strcat`'s alignment step is `movsb`
followed by a one-byte `dec cx`; a disassembly window ending at the `movsb`
shows the `je` targeting an address one byte further on, and reading it as
absent turns a correct routine into an apparent off-by-one. The verifier
caught it in one byte, but the wrong *explanation* had already been written
into a comment. Re-dump from the branch target when the arithmetic does not
add up.

### The annotator must only report the *start* of a string

**The annotator must only report the *start* of a string.** A version that
matched anywhere inside one happily labelled every small constant with the
tail of the Borland banner, which makes a listing look informative and is
worse than no annotation.

### The last argument pushed is the first argument

**The last argument pushed is the first argument.** `pick_file(0, 0, "*.TIM")`
reads as `pick_file("*.TIM", 0, 0)` if the pushes are taken in source order,
and the transcription then copies an empty string, builds no extension filter,
and lists every file where the original lists three. Nothing in the routine
looks wrong - each line is right, the arguments are simply not the ones the
caller sent. Only a side-by-side found it. Count the pushes backwards, every
time, and where a routine's arguments cannot be checked by running it, say in
the comment that the order is a reading rather than a measurement.

### Name a handler from the table that installs it, not from what it seems to do

**Name a handler from the table that installs it, not from what it seems to
do.** `region_cursor_restart` was named from the state numbers next to it; the
region it actually belongs to is the one whose +0x10 is 0x400, which is *enter
freeform*. The restart region has no handler at all. Named right, the routine
makes sense - it has a cursor outside freeform and none inside, the opposite
of its four siblings, because that is when its button does something - and
under the wrong name that symmetry is invisible.

**The same trap from the other end: a name taken from a stride.**
`free_sound_slots` was named when the only thing read about it was its shape
- eleven somethings, 0x1c bytes apart, from DGROUP 0x54a7. There is no table
at 0x54a7. It is a *field*: the far pointer at +0x18 of the eleven 0x1c-byte
archive records at 0x548f, and what the routine frees is each archive's list
of hash-and-offset entries. There is no sound within a thousand bytes of it,
and the name survived because nothing else in the file was typed either. A
stride is evidence about a record's width and about nothing else; the name
has to come from a caller or from what the fields are.

The general form is worth stating: **the four ways one table gets four
names.** The same eleven records were reached as 0x548f for the name, 0x549f
for the open `FILE`, 0x54a1 for the position and 0x54a7 for the entry list,
each with its own `0x1c * n`. Nothing connected them, so each constant grew
its own explanation. Typing the record is what collapses four stories into
one, and `tools/dgrules.py`'s `const-addr` rule is what finds them - it went
from 14 sites to 0 over one session, and every extent came out of the gap
between two structs that were already written.

### Two wrongs cancelled, and only some of the callers were wrong

**Two wrongs cancelled, and only some of the callers were wrong.**
`regions_handle_pointer` opened with `si = DGU16(list)`, but the original is
`mov si, [bp+6]` - the argument *is* the first record. Two of the port's
seven callers passed the address `0x4e79` and the extra dereference made them
right; the other five passed the value, and silently **skipped the head of
their list**.

It surfaced as the level-complete dialog: hovering ADVANCE did nothing,
because ADVANCE is filed at the head and REPLAY is second, so the walk began
one record too late and found only the button nobody was pointing at. Every
screen comparison passed throughout - briefing, picker and level are all 0 or
odometer-only - because none of them clicks a head region.

Two things would have caught it earlier. The verifier prints the argument, and
it was `list=0x704a` - a region record, not `0x4e71`, which is what a word's
address would look like. And the image settles it in one grep: all seven call
sites are `push word ptr [0x4e7x]`, six far and one near thunk inside
`run_machine_loop`. **When a routine's callers disagree about a convention,
one group is wrong - go and count the pushes rather than making the callee
accept both.**

### A BIOS or DOS call the port cannot answer is stubbed, never dropped

**A BIOS or DOS call the port cannot answer is stubbed, never dropped.** An
`int NN` is one instruction, so a transcription with nothing to say about it
can simply not write a line - and nothing then marks the gap. `mouse_move_to`
quarters its arguments, files them at 0x4740/0x4742, calls **INT 33h fn 4** to
warp the driver's pointer and answers 1; the port had the two stores and the 1
and no call, so the pointer never moved. It was **verified**, because a
comparison of DGROUP and the return value cannot see an absent interrupt.

The call becomes a named primitive in `io.c` taking what the registers took
and answering what they answered - `io_bios_font_ptr` answers `{es, bp}` of
zero because fonts are not reconstructed here, and `vm_init` writes out the
assignment the original makes from ES:BP as it stands. The deviation is then
in the primitive's comment, in the spec's `deviation=` and in STATUS.md,
rather than nowhere.

The audit is mechanical - disassemble each transcribed routine from its entry
to its **first `ret`** and ask what the port's body has at each `int`. Get the
extent wrong and the scan reads into the next routine: of eight it flagged,
four were that, three factor the call through another transcribed routine
(`dos_setvect`, the port's own DTA) and one was genuinely gone.

### A four-digit constant walked with a stride is an array of a record, and the record usually already has a type

**A four-digit constant walked with a stride is an array of a record, and
the record usually already has a type.** `dgrules.py --rule const-addr`
finds 23 of them - `si = 0x56e6` then `si += 0x20` twice is two
`struct page_slot`, and `PAGESLOT` has been the macro for one all along;
`si = 0x4bc4` then `si += 0x10` twenty times is Borland's stream table,
which is `struct file_rec`, whose fields are named. So a good part of that
tranche is applying a type that exists rather than establishing one.

Mapped on 2026-09-10: **0x4bc4** twenty `file_rec`, **0x56e6** two
`page_slot`, and seven bases in `parts.c` that are one shape - a point table
copied into a part's `points_ptr` through `POINTS(dst)`, and
`part_setup_23b1` chooses between two of them, which is why the rule lists
only one.

The other three have no type yet and their shape is read off the loop that
walks them: **0x55c3** is ten records of 0x12 bytes with the word at +0x0e as
the in-use test, **0x56b8** is twenty list heads each walked through
`DGU16(slot)`, and **0x52fe** is the filename the picker leaves behind -
which `game.c` and `devmain.c` both say in as many words, so that one is a
name the code already gives it rather than one to invent.

These bases are also the far end of the *computed*-offset list - 930
accessors over 174 bases, which is the rule's largest and cannot move until
each record's type is known. Resolving a const-addr base is what unlocks the
accessors hanging off it, so the two rules are one piece of work approached
from opposite ends.

### A fact about one driver, written into the code that calls all of them

**A fact about one driver, written into the code that calls all of them.**
`SPKR:0x037a` is the speaker driver's do-nothing entry and seven of its
eighteen table slots point at it - including **entry 8**, because a speaker
has no patches and a program change really is nothing there. The speaker was
transcribed first, and that fact went into `sound.c` as `driver_nop()` with
the comment "Entry 8 of the driver's table is the do-nothing stub."
`sound.c` is device-independent. `ADL:` sends entry 8 to 0x1a1b and `SBP:` to
0x1a20, both of which file the patch per channel; their stub is elsewhere.

So every channel kept whatever patch it started with for three years of
commits. The sequencer parsed each program change, filed it, and dropped it.
Patch loads came out at 8 against the original's 20 over the same 43 notes,
and the music played in two timbres where it should have had eight - while
**the note writes were identical**, 35 fnum and 79 key, which is why anything
looking at notes saw nothing wrong.

Four checks passed over it at once: `verify.py` had no spec for two of the
three call sites, the hybrid *dispatches* `sound_service` so the emulator
never runs them and cannot trap, no screen comparison can hear, and
`check_sound.py` tests the digitised path. **When a routine is a no-op
because one implementation makes it one, say which implementation** - and put
the test in the dispatcher, not in the caller.

### An array sized from the prose beside it, when the loop says otherwise

**An array sized from the prose beside it, when the loop says otherwise.**
`DG3A2C.blocks` was declared `[9]` because the header said "Nine slots of
four bytes, searched from 1" - and nine is what the *search* covers, not what
the table holds. `load_palette` at 0x1e967 walks `di` from 1 under
`cmp di,0xa / jl`, so index 9 is written, at `0x3a2e + 4*9 = 0x3a52`. A `[9]`
array ends at 0x3a51. `free_far_block` loops `i < 10` over the same table and
its own comment says "the table of ten" - two comments in one file, 3,200
lines apart, that had never been read against each other.

**And the correction was wrong the same way, one slot further on.** It resized
the array to `[10]` on the reading that `load_palette` "requires `di < 0xa`
before writing `blocks[di]`". It does not: the `cmp di,0xa / jl` at 0x1e9a4
guards the *load*, and failing it jumps straight to the store at 0x1eb4a with
`di` still 10, which files a null at 0x3a56 - one past a `[10]`. The port's
own header on `load_palette` had said "the eleventh slot" all along. Read off
every store, and against what DGROUP holds after the table - nothing named
until 0x3a70 in the driver and 0x3b00 in the game - the table is eleven.

Nothing misbehaved, because the port writes through a cast over the DGROUP
byte array and the address is right either way; the declaration was simply a
false claim, of the kind `-Warray-bounds` and ASan are entitled to act on.
This is the `saved_a` overflow again - a size taken from a sentence rather
than from the loop - and the fix is the same: get the extent from the
binary.

### A name that says `far` may be talking about the call

**A name that says `far` may be talking about the call.** `heap_malloc_far`
at 0x0bb1e is a thunk - one word pushed, `push cs`, a near call to
`heap_malloc`, `retf` - and `heap_malloc` ends `mov ax,bx / retf` with
nothing in DX. So a near heap block is one 16-bit DGROUP offset and the
routine is `dg_near`, not `dg_far`. Reading the port's own `uint16_t`
signature and stopping there would have got the same answer for the wrong
reason, and the wrong reason is the one that generalises: `heap_malloc_far`,
`string_copy_far` and `sound_module_install` are all far *entries*, and only
the listing says what each returns.

The verifier settles it from outside, which is the check worth having:
before `dgo` was added to the spec it read `original AX=0x6a60 port=0x1f40`
- a truncated host pointer, which is what a missing conversion always looks
like - and with it the two agree over five calls.

### What a field *is* is a measurement, not a reading

**A field's name and the comment above it are somebody's earlier reading, and
a wrong one defends itself.** `DG4A82.tick_cb` was typed `struct far_ptr` and
commented "the timer callback; its segment is a relocation". Both halves are
handles: `start_sound` stores what `timer_add_callback` *answers*, once per
callback, and `shutdown_sound` hands each word straight back to
`timer_drop_callback`. The name had been read off the argument - the far
pointer that goes *in* - and the type then made the misreading look settled.

The listing says so at 0x29ca1: `mov ax,0x193e` and `mov ax,0x2619` are
pushed, `lcall` goes to `timer_add_callback`, and `mov word ptr [0x4a8e], ax`
stores the answer. **The breakpoint says so without an argument.** Under the
hybrid, with `TIM_LUA` and `tim.peek16`, the original's own DGROUP reads
`0x4a8e = 1` and `0x4a90 = 2` - the first and second timer slots, where a
function pointer would have shown `193e`/`2619`.

So when a field's meaning is not certain, break on it and look: `gdb -batch -ex
"b routine" -ex run -ex "p STRUCT.field" reconstruct/devtim` for the port,
`tim.bp(<image offset>)` and `tim.peek16(<dgroup offset>)` for the original.
The same run, with `ignore N 1000000` and `info breakpoints`, also counts how
often the code executes - which is how "my check covers this" is told apart
from "nothing here runs it", and `alloc_shape` runs 4,253 times in an intro
the sweep calls "NEVER CALLED on these screens".

### A routine nothing exercises is not a routine that works

**`step_counters` carried "Unverified" for months and was wrong.** The note
above it said exactly why it would stay that way - "the counters belong to the
game proper; the intro screens never reach them, so this is transcribed from
the disassembly and has never been run against the original" - and the first
run against the original found a defect in it.

The routine steps two odometers. At 0x2510 the two blocks test the state in
**opposite** directions, `jne` at 0x251f and `je` at 0x2592: the long counter
rolls while the machine is not running, the short one while it is. The port
wrote `!=` in both, so it rolled the second counter in the editor, where the
original leaves it standing - `start_counters` sets that scroll to 0, and the
zero is what the original's condition uses. On the level screen the port's
reels read 0293 against the original's 0300, and nothing else on the screen
differed.

**Three things had to be true before anyone could see it**, and they are the
lesson. The emulator pin had to carry the multi-byte video read, or
`check_briefing` refuses. The screen had to be compared at all - the level
screen never had been. And the difference had to be told apart from the port's
own pacing, which is what the **hybrid** does: reading `DG50AF.bonus_2` on both
sides through `TIM_LUA` and `tim.peek16`, the port rolled the counter while the
hybrid - the original's code on the *port's* hardware and tick - did not. Same
pacing, different behaviour, so the difference was in our C and not in the
clock. A pure-emulator comparison alone cannot make that distinction.

## Checks, measurements and verdicts

How a check can pass over a defect, or fail over a correct port - and how a build that looks fresh is not.

### A check that polls can miss what it is checking, and then blames the port

**A check that polls can miss what it is checking, and then blames the port.**
`tools/check_save.py` read the emulator's open files once a slice, on the
reasoning that a slice is 2000 instructions and a save must be longer. A
sixteen-byte save is not: truncate, write and close fit inside two slices, so
the one sample landed between the truncate and the write and reported the
original as having written **nothing**. The port was right and the tool said
it was wrong, with the emulator's own log saying `WRITE +16` on the line
above. Worse, the unsafe reasoning had been written down as a *safety
argument* the commit before. Take a measurement at the event, not near it.

**It happened again, to the same tool, on 2026-09-01.** The wait for the save
polls for a file to appear and stop growing, and `port.log` - the tool's own
capture of the port's stderr - is created in the polled directory before the
loop starts. The listing is never empty, two passes see 0 bytes, and the port
is killed a second after launch and reported as having written nothing. Both
scenarios pass once the log is excluded: 16 bytes identical, 740 identical.

The entry above was read *during* that hour and did not prevent it, so the
general form is worth stating plainly: **a watcher must not watch anything it
created itself.** And when a check says the thing under test produced
nothing, suspect the check first - twice now, this one has been wrong and the
port has been right.

**A third time, on 2026-09-09, and the rule as written did not stop it.**
`run_port` grew a `shutil.copytree` of the game directory into the same
output directory, and the wait excluded exactly one name - `port.log` -
because that is what the note above says. A directory never changes size, so
the loop agreed with itself twice and killed the port a second in; the
comparison then called `open()` on it and the run ended in a traceback,
*after* the real save had already been compared. So: the exclusion is one
list, `OURS`, used by the wait and by the comparison both, and a name that is
not on it is the game's.

### `TIM_FLIPS=<dir>:<last>` is a stopping point, not a filter

**`TIM_FLIPS=<dir>:<last>` is a stopping point, not a filter.** It writes a
308 KB frame for *every* flip up to `<last>`, so a run to flip 800 leaves a
quarter of a gigabyte behind. Reading it as "write flip 800" has filled the
disk twice, the second time after a note in `devdump.c` already recorded the
first. `TIM_FLIPWANT=<f1>,<f2>,...` is the filter, and a comparison should
always name the flips it reads.

### A verdict that cannot say what kind of "no" it means will hide the one that matters

**A verdict that cannot say what kind of "no" it means will hide the one
that matters.** `verify.py --all` reported six routines as NOT VERIFIED. Five
were specs asking for an occurrence that never happens - the case above, a
wrong question. The sixth was `select_music`, which *was* compared and
**differed** in two DGROUP bytes: a port bug, in the sound path where no
screen comparison can ever see it, filed among five non-events where nobody
would look. `ok_all` is false for both and the table printed both the same.

The same shape one layer down: a call that writes no hardware event, has no
return to check and changes no memory agrees with everything.
`compare_instance` says so - "this call did no work, so an agreement here is
not evidence" - and the summary line ignored it, so a routine could have gone
into STATUS.md as "agreed" on the strength of nothing at all. Both now say
which they are. Measured afterwards: 410 verified and **none** of them
vacuous, which is a fact worth having rather than an assumption worth making.

Three times in one day a green result was compatible with something being
wrong - a stale `shims.c` that `make` did not rebuild, a symbol table with a
routine missing from it, and these verdicts. Ask what a pass would look like
if the thing being tested were broken.

### A sampled frame can only land on a phase that is a multiple of the step, and a screen that animates has phases in between

**A sampled frame can only land on a phase that is a multiple of the step,
and a screen that animates has phases in between.** Comparing the hybrid
runner against the port every twentieth frame reproduced ten of the port's
sixteen title-screen flips byte for byte and missed six. The six were exactly
the phases no multiple of twenty falls on - a sampling artefact that reads
like a blitter fault, and was written up as one. `TIM_FRAMES=<dir>:<step>` now
takes `:<from>:<to>`, and every frame across a narrow window answers it
without a gigabyte of pixels.

The window has to be **wider than the flips it covers**, because the hybrid's
frame numbering moves between runs, and never repeats: across eight runs the
same port flip 4 came out as frames 300 to 320, no two alike, every one of
them byte for byte identical to it. A window pinned
to one run's numbering put five flips' true match past its edge, and the
closest frame to each was the last frame in the window - which is what the
edge of a window always looks like, and worth recognising on sight.

**`TIM_FLIPWANT` used to drop silently past its sixteenth flip**, and that
cost an hour of blaming the wrong side. Asking for `4,50..65` is seventeen,
so flip 65 was never written however long the port ran, and the run read as a
port too slow to reach it. A timeout was raised twice, "the port paces on a
wall clock" was written into CLAUDE.md as the reason, and it was wrong: with
the limit raised the flip arrives in under two minutes. It now aborts rather
than truncating. A filter that quietly discards what it was asked for
invents a symptom on the far side of whatever it was filtering for.

So a screen is proved as a **run of consecutive flips**, not a frame. One
frame can agree by luck on a screen that is mostly one colour; fifteen in
sequence, each byte for byte and in order, is the animation. Requiring one
named flip also makes the tool report the port's own pacing as a difference.

### A red line from the verifier means one of two things, and they look identical

**A red line from the verifier means one of two things, and they look
identical.** Either the transcription is wrong or the *spec* is wrong, and
the second is far more likely on a routine's first run. `long_multiply`
reported NOT VERIFIED across 6,477 calls because its spec lacked `near=True`;
the linker pulled `__LMUL` into the image twice and the two copies end `ret`
and `retf`, one byte apart. `heap_init` reported NOT VERIFIED because the
spec asked for occurrence 1 of a routine called once. `load_screen` reported
307,311 differences because three once-only routines were asked for a second
call. Read the parenthetical - "only 1 calls seen", "never reached: 1" -
before touching any C.

So read every entry and every return rather than inferring a convention from
the family. Ten allocator routines use four different argument conventions;
`poly_edge_vertical` disagrees with the four other edge routines about which
register holds which end; `set_cursor` takes its hot spot **y before x**.
Each of those, assumed, produces a failing spec that reads as an accusation
against correct code.

### `check_sound` prints its verdict and then does not exit

**`check_sound` prints its verdict and then does not exit.** Measured on
2026-09-10: the log was complete - "55 runs of blocks, identical by length,
rate and content" - while the process sat at 0.2% CPU with no child, for
three minutes, holding up a script that ran the checks in sequence. Its
`subprocess.run` calls all carry timeouts, so it is not the port; the hang is
after `main()` returns, and `import tim` brings up pygame and the emulator,
either of which can keep a non-daemon thread alive at interpreter shutdown.

Fixed the same day: it flushes and calls `os._exit` rather than unwinding
the interpreter, which is what the verdict being complete beforehand makes
safe. The general shape is worth keeping in mind though - **a chain that
waits on a process rather than on its answer stalls with the answer already
sitting in the file**, and that reads as a check still running rather than
one that has finished.

### `-fsyntax-only` is not a build, and the difference is exactly the class of defect this project keeps meeting

**`-fsyntax-only` is not a build, and the difference is exactly the class of
defect this project keeps meeting.** The frame promotion was checked on a
full copy of the port: every file parsed clean under `-Wall -Wextra`, and two
silent corruptions were sitting in it. A real compile found both, because
`-Warray-bounds` needs the optimiser and the optimiser needs a real
compilation.

`bounce_off_contact` writes `dg_wr32(plo, ...)` into what the converter had
sized at two bytes: `plo` at `[bp-0x10]` and `phi` at `[bp-0x0e]` are the two
halves of one 32-bit value, and the routine writes it through the low half.
As slots in one buffer that worked, because the neighbour *was* the other
half. And `decode_vqt_list`'s reader record, written at `rd + 2`, `rd + 4`
and `rd + 0x18 + 2*i`, came out two bytes because the accessor check saw only
the bare `dg_wr16(rd` - and then, worse, setting the size from that width
suppressed the scan that would have given it the whole 458.

Both compile. Both would have corrupted whatever followed. So a conversion
that changes where anything lives gets **built**, not parsed - and if the
tree is busy, built in a copy, which costs nothing and catches this.

The copy is worth more than that, in fact: its binaries run. Eight levels
rendered with `devtim --level N --run --raw` were byte for byte identical to
the build being replaced, which is behavioural evidence before a single line
lands in the tree.

### A short budget and a routine nothing calls give the same verdict

**A short budget and a routine nothing calls give the same verdict.**
`--only` defaults to 40M instructions and the polygon filler is not reached
until past 90M, so three routines `reached.py` had already shown to run came
back "TRANSCRIBED, NEVER CALLED". Believe that verdict only when a second
measurement agrees - `poly_outline`'s does, because 0x1f219 is absent from
`reached.py`'s set too.

### "Never called" measured on four levels is a statement about those four levels

**"Never called" measured on four levels is a statement about those four
levels.** The polygon clipper was written up as reachable by nothing in this
repo, on three measurements that all looked sound: instrumenting
`draw_polygon` and `clip_polygon` and playing level03, level12, level22 and
level28; `verify.py` from the entry point at a 150M budget; and `verify.py`
from each of the three snapshots. All three said never. Its one caller is
`draw_part_extra`, which a **kind-0x1e** part draws - and levels 14, 15, 16
and 19 have one. Instrumented there, both routines are entered on the first
pass and both levels still solve, so the conversion was covered by
`check_solutions.py` all along and the commit that said otherwise was wrong.

Two lessons, and the second is the one that cost the hour. A screen or a
level is a *sample* of the game's data, and a routine that draws one part
kind is reached only where that kind is; `TIM_LEVELSCAN` exists to answer
which levels those are and should be the first thing consulted, not
four levels picked because they were to hand.

**Measured at last, on 2026-09-10: the 33 solution snapshots carry every
part kind in the game.** All 87 levels scanned, 1,830 parts, **32 distinct
kinds** - and the set the snapshots reach is the same 32, with nothing left
over. So `check_solutions.py` is not a sample of the part data, it is the
whole of it, and the twelve `part_setup_*` routines converted this week rest
on a measurement rather than on hope. Every kind's setup is dispatched from
`PARTKIND_AT(0x0ea6 + kind).setup_off`, and an offset with no case reaches
`not_transcribed`, which aborts - so a kind that got there would stop a run
rather than pass one.

**It is complete and it is thin.** Kinds 38, 50 and 22 appear in only two
levels each, and exactly one snapshot carries each of them - 38 and 50 both
ride on `level13`, 22 on `level17`. Dropping either file silently costs
three kinds' coverage, so they are load-bearing in a way nothing else records.

And the note above about scanning in chunks understates it. The heap gives
out after eight to eleven levels, not reliably twelve, and a chunk that fails
reports only its *first* level - so a sweep of all 87 is `TIM_LEVELSCAN`
over chunks of about eight, then re-scanning whatever numbers are missing
from the output, twice. It prints to **stdout**, not stderr.

And **`TIM_LEVELSCAN` was lying.** `load_level` allocates a record per part
and the scan freed nothing, so the heap ran out around the twelfth level and
every level after it printed `parts 0  kinds` - indistinguishable from an
empty level, and no level is empty. That zero is what hid levels 14 to 16.
It now stops and says the scan failed and how to continue, because a tool
that cannot do the whole job must say so rather than produce a plausible
answer for the part it managed. `round_teardown` is not the fix and makes it
worse: it frees the lists but not the per-kind bitmaps, and the loader then
fails four levels sooner.

### Two tools that agree can share a blind spot, and then the agreement is worth nothing

**Two tools that agree can share a blind spot, and then the agreement is
worth nothing.** The frame work has a census - which routines can be
converted - and a converter. Both decided what a routine's "slots" were by
looking for `uint16_t v = fp + k` declarations. `draw_counter_word` names its
frame `buf`, takes no such declaration, and hands `buf` straight to
`int_to_string`; both tools called it slotless, so the census said it was
unblocked and the converter cheerfully converted it into a pointer being
passed to a routine that still wants an offset. The two agreeing was not
evidence, because the same misreading was in both.

**A third shared blind spot, found by asking the tools about a settled
question.** With every frame converted, the roll call said two of the three
survivors had **no blocking callee** and `framify.py`, run on them, converted
both without complaint - and their conversion is the one thing in this file
that has been *measured* to be wrong: a C local for `read_resource`'s
destination makes `check_sound` answer one run of blocks against fifty-five.
Two causes, one in each tool and the same shape. `read_resource` takes a
pointer now, so it is in the census's `ptrfn` and the call was dropped before
its by-hand entry was consulted, leaving that entry dead. And both tools stop
at the innermost call, so `read_resource(handle, dg_ptr(dgroup, b), 1)` reads
as a slot handed to `dg_ptr` - which is on both whitelists - and what it was
really passed to is never looked at. `dg_ptr(dgroup, b)` **is** `b`; it is a
spelling, not a use.

So a pointer parameter is not proof the argument may be one, and the wall now
lives in one place - `framify.py`'s `NEEDS_GUEST_ADDRESS`, which the census
imports rather than restating. The prose in CLAUDE.md and the comment beside
the routine had both been right for weeks while the two tools that enforce
them agreed on the opposite.

The same census had already been wrong the other way: it took a callee's name
from the *line* a slot appeared on, so a call whose arguments wrap came back
as "?" - 41 of 87 frames, and the true count of convertible ones was 36
rather than 16. It said too few, then too many, for two unrelated reasons.
A worklist is a measurement and deserves the same suspicion as any other.

### A check that does not build its own references compares two different ages of the code

**A check that does not build its own references compares two different
ages of the code.** `tools/native/native` links the port's `.c` files into
the hybrid, and `check_native.py` only checked the file *existed* - so
editing the port and running `make -C reconstruct` left the hybrid at
whatever it was last built from, and the check compared this hour's port
against last week's.

That hid a real defect for four commits. `read_into_huge` reserves four
bytes and writes a far pointer into them, `DG16(fp)` and `DG16(fp + 2)`;
the converter rewrote both onto a `uint8_t *` as `(*fp)` and `fp[2]`, which
stores two low bytes and never writes the segment at all. It went in under
"all 66 intro flips are byte for byte" - true of the hybrid that was on
disk, which predated the routine. Three commits later an unrelated rebuild
turned all 66 red at once, which reads as a regression in whatever was
committed *last*, and the bisect that found the real commit cost eleven
builds.

`make test` was clean throughout and 29 of 29 levels still solved: the
routine reads bitmaps, and a level's parts come from elsewhere.

`check_native.py` now runs `make` for both binaries itself. That is the one
moment a build is safe - before anything is running - and the standing rule
about not rebuilding is about a build *during* a run.

**It is not one check, and `make` alone does not settle it.** `make` builds
the binaries and does *not* build `libtim.so`, so which reference a session
has just rebuilt depends on which `make` it happened to run. An hour after
the above, `verify.py` reported `parse_open_mode` DIFFERS over five calls -
the port returning 0 and writing nothing - against a library that still took
offsets where the spec had started passing pointers. Every reading of that
is a plausible story about correct code: a wrong spec, a wrong argument
order, a seeding problem. Built first, it verifies over 83 calls. And three
routines had been called "verified" that same hour on a library that was not
running their new code at all, which is the same staleness answering green.

`tools/tim.py` now has `built(what, where)`, and every check goes through it.

And that closes one hole and opens a smaller one: **now that the checks build
for themselves, editing a `.c` while one runs is what rebuilding used to
be.** A `check_solutions.py` run in the background came back "Error 1" from
`make` and gave no verdict at all, because the edits for the next round had
landed in the tree while it was between levels. The rule is the same rule -
one thing at a time - but the trigger is now the editor and not `make`.

### A spec that was right becomes wrong when the routine's arguments change, and nothing links the two

**A spec that was right becomes wrong when the routine's arguments change,
and nothing links the two.** `int_to_string`, `long_int_to_string` and
`long_to_string` were converted to take `dg_near buf` days before their
`verify.py` specs were looked at; the specs went on passing
`ctypes.c_uint16(offset)` into a parameter that is now a pointer. Two of the
three then had no verdict at all - the sweep died in collection with no
message - and `string_copy_padded`, converted the same hour, reported
DIFFERS with **the memory identical and only the return value out**: the
original answers `0xff90` and the port the low half of a host address.

Three separate spellings, one cause. So when a routine's signature changes,
the spec changes with it, in all three places: `dgp` on every argument that
became a pointer, `dgo` around a return that became one, and the routine's
name in the `restype = c_void_p` list - a returned pointer with no restype
comes back truncated and `dgo` cannot undo it.

**And nothing linked the two until the sweep died of it.** Seven more specs
were still passing `ctypes.c_uint16` where `tim.h` now says `dg_near`, which
hands the port a small integer to dereference. `load_sound_bank` segfaulted
the whole `--all` sweep - twice, at the very end of a 2600M-instruction
collection, with no output but exit 139 - and the narrowed `--only` runs used
while converting had never reached it, because it is not called on those
screens. `verify.py --list` now reads the prototypes against the spec table
and fails on the mismatch, so `make test` catches it; a spec with `src_from`
is skipped, because its buffer arrives as a ctypes array rather than an
offset.

The sweep also has to be *watched* properly. `pgrep -f "verify.py --all"`
matches the watcher's own command line, so every "still running" was about
the watcher and the crash went unnoticed for three turns - the
self-referential pattern this file already warns about, met again. Keep
`$!`, `wait` on it, and write the exit status into the log.

### The list that tells ctypes what a routine returns existed twice, and the sweep read the copy nobody updated

**The list that tells ctypes what a routine returns existed twice, and the
sweep read the copy nobody updated.** `verify.py` declared restypes in
`main`, for a single routine, and again in a shorter block inside
`compare_instance`, which is the sweep's path - and every addition for
months went to the first. Measured on 2026-09-13: the sweep's copy was
short by about two hundred declarations. Most cost nothing, because a
16-bit return read as a C `int` is the same number; a `struct far_ptr`
return is not, and `_alloc_for_kind` asked an integer for `.off`, so
`--all` collected for a minute and died with a traceback at the very end,
while `--only alloc_for_kind` through `main` passed. STATUS.md's table had
been written by the last sweep that got past it, three days before.

One function now, `declare_restypes`, called from both. The general form
is the one this file already states about `shims.c` and about the census:
**two copies of one fact drift, and the copy a check reads is the one that
matters.** When a routine gains a return type, the place to put it is the
function, and `grep -c restype` should find the name once.

**And the runner that watched the chain reported every exit as 0.** It
wrote `echo "$(date +%T) $name exit $?"`, and the command substitution runs
first, so the `$?` that reaches `echo` is `date`'s. Tested in one line:
`false; echo "$(date) $?"` prints 0. Capture `$?` into a variable on the
line after the command and before anything else runs, and read a check's
verdict from its own log rather than from a status a wrapper wrote.

### Two developer builds had never once compiled, and the sanitizer found a real overflow the first time it ran

**Two developer builds had never once compiled, and the sanitizer found a
real overflow the first time it ran.** `make debug` and `make asan` spell
their own compiler flags instead of using `$(CFLAGS)`, so they had no `-I.`
and every source failed on `tim.h`; and they list only the `.c` sources, so
the link had no `opl_status`. Two independent breakages, neither noticed,
because a target nobody builds cannot fail. `make test` builds `debug` now -
three seconds, and it fails in both of the same ways `asan` would.

What `asan` then reported was **a 15-byte stack-buffer-overflow on every
call** into `load_bitmaps`: `saved_a` was declared `uint8_t[52]` and
`copy_file_record` writes 0x43. The original's frame settles the size from
either end - `[bp-0x5e]` to `[bp-0x1a]` is 0x44, and so is `saved_b`'s
`[bp-0xa2]` to `[bp-0x5e]` - so it is 68, and `saved_b` next to it was
already right. The wrong number came from an early sizing pass that measured
to the wrong neighbour, and every screen comparison passed over it for
weeks, because the fifteen bytes landed on locals that are written again
before they are read.

So: **the checks in this repo compare pixels and bytes, and none of them
looks at memory.** A sanitizer is the only thing here that can see a frame
overrun, and it is worth running whenever the frames change. Its UBSan half
is a different matter and is *not* a defect list: the guest's records are
packed and reached at odd offsets by design, so "load of misaligned address"
is the model working, and `dg_rd16`/`dg_wr16` exist for the places where a
typed pointer would be the lie. Read those reports as a description of the
model, not a worklist.

**Five of them were not the model, and three were the dangerous kind.**
Sifting ~230 alignment reports left three about *shifts*. `machine.c`'s
`PART(obj).fy <<= 9` on -1000 and `trig.c`'s `(int16_t)(r - 0x400) << 4` are
left shifts of a negative value - the original's `shl` has no sign in it and
C calls it undefined; shifting the `uint16_t` is the same bits. The third was
worse and had two siblings the run never reached:
`((int32_t)(uint16_t)x << 16)` overflows a **signed** int whenever x is above
0x7fff, and signed overflow is one of the undefined behaviours a compiler
will actually act on, unlike the alignment ones. Twenty other sites in the
port build the same 32-bit value and spell it `((uint32_t)x << 16)` with the
cast on the *result*, which is right - so this was three typos in a family of
twenty-three, and the majority spelling was the correct one.

That is what the UBSan half is for: not the alignment noise, but what is left
once the noise is set aside. And `make asandev` is the build that can reach a
level at all - `timasan` comes from the shipping `main.c`, which has no
command line, so it sees the intro and nothing else.

Measured afterwards over **all 33 solution snapshots**: zero ASan errors and
every one still solving, about a minute a level. That is the coverage claim
worth having, and it is a measurement rather than "the screens pass and this
routine is on a screen".

**The static version of this check does not work, and it is worth saying so
rather than leaving the idea lying around.** Every local carries the
original's `[bp-N]`, so two slots' offsets give the span of the lower one and
an array can be compared against it. Prototyped, it flags 15 arrays declared
shorter than their span - and 14 of those are *normal*, because a 2-byte
value in a 4-byte-spaced slot is what a frame with padding looks like.
`saved_a` would have been the fifteenth line, indistinguishable from the
rest. What made it a defect was not being shorter than its span but being
shorter than **what its callee writes**, and that is a different question
with a much smaller domain: exactly one routine in the port writes a constant
count into a caller-supplied buffer - `copy_file_record`, 0x43, three call
sites, all now correct. The class is exhausted by reading, and ASan covers
it going forward.

### A `make` after a `make` compiles nothing, so grepping its output for warnings answers about an empty build

**A `make` after a `make` compiles nothing, so grepping its output for
warnings answers about an empty build.** The landing gate was
`make >/dev/null || fail; [ "$(make 2>&1 | grep -c warning)" = 0 ]`, and
the second `make` had nothing to do: every file was up to date from the
first, so the count was zero whatever the sources said. Measured on
2026-09-11 with a source that carries two `-Wpointer-to-int-cast` warnings
- `PARTP()` wrapped round a value that was already a pointer, which
truncates a host address to sixteen bits - compiled directly: two
warnings; through the gate: zero. The port was clean by luck; every file
compiled again by hand showed nothing. **One forced build, its output
captured, counted once** - `out=$(make -B 2>&1)` - is the gate; and a
diagnostic names the *header* line (`dgroup.h:1979`), so a filter on the
`.c` file's name drops it.

### Do not rebuild anything while a check is running

**Do not rebuild anything while a check is running.** `cc -o` rewrites the
file the running process has mapped; the sweep drops to 0% CPU and is lost.
This was written for `libtim.so` and the verification sweep, and it is the
same for `devtim` and `tim`: editing dgroup.h and running `make` while
`check_solutions.py` was in flight made level28 report a **600-second
timeout**, which reads exactly like a level that stopped solving. Re-run with
the tree left alone: 29 of 29. A check that says the port broke is worth one
look at what else was running at the time.
Editing the `.c` is safe, `make` is not. And **a header-only change is when
to distrust the build**: `libtim.so` and the binaries listed only the `.c`
files, so raising a constant in `io.h` rebuilt nothing and the next run used
the old library - silently, and answering with complete confidence. A whole
finding was written up from that stale result, retracted only when an
unrelated edit forced a rebuild an hour later. The rules now depend on
`$(HEADERS)`; `touch reconstruct/io.h` should rebuild.

And **`make` outside `reconstruct/` builds nothing and says nothing.** There
is no Makefile at the repository root, so `cd $REPO && make` prints "no
makefile found" and carries on; a `| grep error` after it reports success. It
cost one stale-binary measurement here. Every check target depends on
`devtim tim`, so `make test` does rebuild - but a bare `make` from the wrong
directory is a silent no-op.

And **keep the pid of what you launched; do not go looking for it again.**
`$!` is right there. Every pattern search for a run this session was
unnecessary and two of them were self-referential: `pkill -f "only
poly_walk"` matched its own command line and killed the shell before the
redirect was opened, and `until ! pgrep -f "only load_all_parts"` waited for
itself and never fired. Both read as the tool misbehaving rather than the
pattern matching the watcher.

    uv run python tools/verify.py --all > out/sweep.log 2>&1 &
    pid=$!

`kill $pid` and `kill -0 $pid` are then unambiguous. They are still racy if
the wait outlives the process, because the pid can be recycled and `kill -0`
will answer about somebody else; a **pidfd** is the race-free handle and this
kernel has it. From Python, which is where the tools live:

    fd = os.pidfd_open(pid)      # a stable reference, not a number
    select.select([fd], [], [])  # readable exactly when that process exits

Waiting on the output file works and is what the earlier note recommended,
but it answers a different question - "has it written anything" rather than
"is it still running" - and it cannot tell a finished run from a killed one.

### Two references agreeing is not corroboration when they share a bias

**Two references agreeing is not corroboration when they share a bias.** The
port's music was measured as running three times too fast, twice, against
two independent references - the hybrid at 52 key events a second against the
port's 151, and DOSBox at an envelope period of 0.406s against 0.139s. Both
said x2.9 and both were wrong the same way: they are *wall-clock*
comparisons, and both references are slower machines. DOSBox's tempo never
plateaus with cycles - 0.406s at 8000, 0.075s at 30000, 0.046s at max - so it
measures the machine, not the game.

The answer came from a quantity with no clock in it. Both sides push page
flips and OPL writes through the same `io.c`, so **notes per flip** is a pure
guest-side invariant: if the whole game runs slow, both slow together. At 138
matching flips the port is 1.61 and the original 1.74, a ratio of 1.05. The
music keeps step with the game's own pacing.

The port's speed is then the game's own arithmetic: PIT divisor 5041 is
236.7 Hz, `game_screen_loop` waits 8 ticks, two pages are presented per
iteration - 59.2 flips a second against 57.5 measured.

So when a timing claim rests on comparing two runs, ask what quantity could
not contain the bias, and measure that instead.

### The hybrid's music runs on a different clock from its samples, so the two cannot be compared

**The hybrid's music runs on a different clock from its samples, so the two
cannot be compared.** `native.c` deliberately does not call `io_set_timer` -
int 8 arrives between emulator slices - so the tempo runs at emulation speed,
while the Sound Blaster's completion interrupt comes off `io_now`, which is
wall-clock. Measured: 52 key events a second against the port's 151, so five
notes play inside the 0.518-second sample where the port plays twenty-one.

A sequence whose removal waits on that interrupt is therefore live for a
different number of notes on each side, the playing table differs by
construction, and voice allocation - which reads the table - differs
downstream. The visible end of it is two notes swapping voices, which looks
exactly like an allocator bug and is not one. A whole session went into
chasing it as a transcription error.

The digitised comparison is unaffected because it never consults the tick,
which is why `check_sound.py` gives a verdict and `--fm` gives a 2.

### The sampling trap is not about frames

**The sampling trap is not about frames.** It is written up above for page
flips, and it was met again in the sound work with no frames anywhere near
it. Printing a sequencer byte six times on each side gave

    port    00 00 00 81 00 00
    hybrid  00 00 00 00 00 00

and that reads as "the port asks the module for something the original never
asks for". It is not a comparison at all: the hybrid runs the guest under
Unicorn and covers far less game per second, so its first six calls are not
the port's first six. At 400 samples both sides show the same marks and the
finding evaporates. Counts do not survive either - the two runs were 40 and
180 seconds of different amounts of game.

So: **the two sides share no clock and no index, and anything compared
between them must be aligned by content** - the way `check_native.py` aligns
flips and the way the trace-diff aligns register writes. An index is not an
alignment. A whole commit went in on that reading before the wider sample
retracted it, which is the second time in this project that a green-looking
measurement was compatible with the opposite being true.


### A run that stops with `exit` destroys the sound chip under the timer thread

**What happened.** Rebuilding the solution references after `/tmp` was cleared,
S07's load run - load the machine, snapshot at flip 60, `TIM_STOPFLIP=65` -
exited 139 on one run in eight, on the committed tree. The snapshot was written
and the simulation from it solved, so the solution table, which compares level,
solved and frames, said nothing; only the row's note showed it.

**What it was.** Under gdb the main thread was in `exit`, running the static
destructors, freeing `ymfm::ymf262`, while the timer thread was inside
`sound_service` → `step_sequence` → `adl_note` → `opl_write` on that same
object. Level 7's machine has music playing when the run stops, which is why it
was that solution and only some of the time. `exit` runs destructors; nothing
stops the timer thread first.

**What settled it.** The two dev-build exits - `TIM_STOPFLIP` in devdump.c and
Lua's `tim.quit` - `fflush(NULL)` and `_exit(0)`, as `sdl_die` already did.
Nothing is registered with `atexit`, so the flush is all `exit` was doing that
mattered. Twenty runs of S07 afterwards, no crash; the intro and all 29
solutions unchanged.

**The rule.** A process with a running timer thread leaves with `_exit` after a
flush, never `exit`. And a check whose verdict ignores the exit status can pass
over a crash - read the notes column.

### `make test` stopped at its solutions step, and what that step wanted could not be in the repository

**What happened.** Found on 2026-09-16 while adding `tools/check_handles.py` to
`make test`. The `test` target ran `tools/check_solutions.py --simulate`, which
globbed `solution_snaps/*.solution` - and `solution_snaps/` does not exist, so
it exited with "no .solution snapshots", the target printed `FAIL: a solution
no longer solves under simulation` and **stopped**. Everything after that step
had therefore not been running: `check_printf.py`, and the
`framify_census.py --assert` and `promote.py --assert` ratchets. The line was
being read as "the one expected failure" rather than as a wall.

**What it was.** Not the glob. A `.solution` is a *port snapshot* - our memory
and our hardware state, carrying the original executable with it - so it is
reached only by playing, is not ours to publish, and is ignored by
`.gitignore` on purpose. The only end-to-end check in the project rested on a
file that could not be in the repository, and would have gone on resting on
one whatever the glob said.

**What settled it, on 2026-09-18.** A solution became a **machine file**:
`solutions/S<NN>.TIM`, what the game's own `save_machine` writes, loaded over
its puzzle the way the game loads one - `--level N` supplies the goal,
`TIM_LOADMACHINE` hands the file to `round_teardown`, `load_animation` and
`reset_machine`, and the bin is emptied because a puzzle's bin is the level's.
No snapshot, no CPU state to invent, and the same file is readable by the
hybrid, which is why `check_machines.py` could drop its snapshots too.

Two things had to be measured on the way. **The file has to be inside the game
directory**: the port's file layer treats that directory as a floor, so an
absolute host path resolved to nowhere, `read_level` answered 0 without a word,
the autoplay driver still reported "loaded the machine", and the first goal
test - `goal_test_puzzle_1`, whose walk has no end test because the original's
has none - spun on an empty list. Both checks now stage a copy of the game
directory with the solutions beside the game's own files, and
`check_solutions.py` refuses to give a verdict about solving unless the
loader's own line is in the output. And **the clock has to run** until the play
screen is up: a simulation from a snapshot needs no timer, but a machine loaded
the game's way needs the game to get as far as its level screen first, and the
frame spins wait on the tick. `dev_autoplay` stops the timer at the moment it
takes over.

29 of 29 solve both ways, simulated and through the real loop, at the same
frame. Playing the intro for every level costs two seconds a level that a
snapshot did not, so both checks now run the levels in parallel: 4 seconds
simulated, 97 seconds real, on sixteen cores.

**The rule.** A check's evidence belongs in the repository, in the game's own
format, loaded through the game's own loader. Evidence that can only be made by
playing is evidence that will be missing when it matters - and a step whose
absence prints the same line as a real failure will be read as one and skipped
over.

### Two drivers doing the same job in different units are not the same driver

**What happened.** The side-by-side machine check reported 11 of 701 flips
agreeing on S03 and blamed the port's timer, which STATUS.md had already
deferred and which made the number easy to believe. Two defects were hiding
behind that verdict, both in the *drivers* rather than in the game.

**The first was a step one side took and the other did not.** `devdump.c`
empties the parts bin after loading a machine over a puzzle, for a reason
written there - a machine file records no bin, `load_animation` leaves
freeform's one-of-every-kind in place, and level 10's goal disqualifies a gun
left in it. `guest_load_machine` in the hybrid made the same three calls and
not that one. The picture says it plainly once looked at: the hybrid drew a
full bin down the right-hand strip where the port drew an empty one. Fixing it
took one level from 11 of 701 to 696 of 701.

**The second was the unit each driver counts in.** The port's runs on the page
flip; the hybrid's ran on the *present*, and the hybrid presents about twice per
guest flip. So its "let it settle before starting" - a `return` that costs the
port a whole flip - cost nothing at all: measured on S03, it loaded the machine
and started it within one flip, the level was never redrawn between the two, and
the tutorial panel the load should have cleared was still on screen while the
machine ran. Gating it on `io_flip_count()` moving put both drivers on the same
cue. The message numbers wanted the same care: `io_flip_count()` is the *count*
of flips and the hash beside it is numbered from zero, so the driver's log was
one ahead of the frame it was talking about.

**What settled it.** 29 of 29 levels, 685 flips each, byte for byte, and no
unstable flip on any level - the port agreeing with itself as well as with the
hybrid, where two sweeps in September had disagreed about eleven of
twenty-eight levels. The check now aligns the two streams on the flip each side
says it started the machine, which is a signal the game itself gives.

**The rule.** When two artefacts are driven to the same place by two pieces of
code, the drivers are part of what is being compared: they must take the same
steps, in the same units, counted from the same zero. And a whole-screen
difference is a screen to *look at* before it is a number to explain - the bin
was visible in the first frame anyone rendered.

### A tree-sitter parse of code full of unknown macros is not a parse, and the tool cannot tell

**What happened.** A new `dgrules` rule wanted to know which field each
constant in `dgroup.c` initialises. The parse said `assignment_expression` with
no designator, for every one of them. Counting the damage: a plain tree-sitter
parse of the port's sources yields **7,188 ERROR nodes**, 6,112 of them in
`dgroup.c` alone.

**What it was.** Four things in this tree are macros the C grammar has no rule
for, and each one makes the parser abandon the construct it is in:

    struct draw_step DG0124 DGROUP_AT(0x0124) = { ... };   /* between the
                                       declarator and its `=` */
    DG_ASSERT_AT(struct part, kind, 0x04);       /* a type as an argument */
    _Static_assert(__builtin_offsetof(struct vm_cs, data_seg) == 0x13a, "");
    int16_t read_into_huge(uint8_t far * dst, uint16_t count)  /* `far` is
                                       defined as nothing at all */

Inside an ERROR subtree the node types are whatever the parser could salvage,
so a rule that asks "is this an `initializer_pair`" gets no for a line that is
one - and reports nothing, which reads exactly like a clean result.

**What it cost beyond that rule.** `check_dg_near.py`, which runs in `make
test` and is the reason `dg_near` cannot be called anywhere but a store, parses
the same sources with a plain parser: 7,188 ERROR nodes of its guarantee. It
found nothing because there is nothing to find, but it was not in a position to
say so.

**What settled it.** `tools/cparse.py`, the one door to tree-sitter for every
tool that reads the port's C, expanding those four before the parse - space for
space, so every byte offset and line number is the file's own. Six ERROR nodes
are left in the whole tree, and both are preprocessor conditionals in the
middle of a function, which no macro expansion can help. `check_handles.py`
moved from regexes to the same parse while the door was being built, and the
move immediately paid: its three-line window for "was this handle just
allocated" called a `p == NULL` four lines after its own `heap_calloc_far` a
fault.

**The rule.** A tool that parses reports what it could not parse. Expanding a
known macro is part of the front end, not a shortcut around it - and a parse
that has never been asked how many ERROR nodes it produced is a parse nobody
has checked.

## The hybrid runner

What `tools/native` can and cannot observe.

### A Unicorn read hook over a `uc_mem_map_ptr` region breaks the guest

**A Unicorn read hook over a `uc_mem_map_ptr` region breaks the guest.**
`TIM_SLOTS` was built to watch stack traffic and the first version watched
reads as well as writes. With a callback whose entire body is `return`, the
game leaves the rails inside twelve frames and the backtrace reads
`game_main+0x8` calling `part_flip_options`, which is not a call that exists.
`UC_HOOK_MEM_READ_AFTER` does the same. The identical hook on
`UC_HOOK_MEM_WRITE`, same range, same callback, runs clean. Bisected: a hook
registered over a range it never fires in is harmless, so it is the firing.

The one read hook `native.c` already had - `on_vga_access` - escapes this
because it calls `uc_emu_stop` and never returns to running code, so it has
never exercised the case that breaks. A mechanism that has only ever been
used one way is not evidence that the other way works.

### The hybrid cannot watch what it has dispatched

**The hybrid cannot watch what it has dispatched.** A routine running as the
port's C writes `guest_mem` directly and never passes through Unicorn, so no
emulator hook sees it. That is worth stating before building any measurement
into `tools/native/`: the hybrid observes the *emulated* side, which shrinks
every time a routine is transcribed. `TIM_SLOTS` was built to find which
locals a callee reaches into, and the reaches that motivated it -
`draw_polygon` walking three points from an address `draw_part_extra` handed
it - are all in dispatched code and none of them appear. The same
bookkeeping inside the port's own `DG*` accessors, where `dg_alloca` already
knows the frame, is what covers that half.

## DGROUP, frames and pointers

The model of the guest's memory as a byte array, and every way a pointer, an offset, a frame or a field width has gone wrong in it.

### A routine that calls `dg_alloca` needs `guest_sp` set, or it writes its locals over live memory

**A routine that calls `dg_alloca` needs `guest_sp` set, or it writes its
locals over live memory.** In the large model SS and DS are one segment, so a
routine building a structure on the stack hands out an ordinary DGROUP offset
and the callee cannot tell it from a pointer to a global. A C local has none,
so the port carries its own stack pointer and `dg_alloca` reserves below it.
`tools/verify.py` sets `guest_sp` at every entry and `dgroup.h` says so; the
hybrid runner did not, and `load_bitmaps` - which reserves 0xa2 bytes - took
the intro from identical to 76,817 pixels out the moment it was dispatched.

The lesson is not the one routine. **Three routines already dispatched use
`dg_alloca`** and every green check they were part of had been luck: their
frames happened to land on stack nobody was using. A caller that sets up less
than the verifier does is not a lighter version of it, it is a different
thing that agrees for a while. Auditing the rest of what `verify.py` sets -
`dgroup_base`, the open files, the VGA registers and planes - found nothing
else missing, and that audit is worth repeating whenever the port gains a
new piece of state.

### Promoting a frame's slots to C locals spends `frames.py`

**Promoting a frame's slots to C locals spends `frames.py`.** `framify.py`
turned each `dg_alloca` reservation into `_Alignas(2) uint8_t frame[N]` with
a pointer per slot, and that N is what `tools/frames.py` compares against the
binary's own `sub sp,N` - the one instrument that says the port reserved the
right amount. `tools/promote.py` then turns each slot into the variable it
is, and a routine whose locals are ordinary C locals has no N to compare: it
moves from "port reserves the locals" to "original reserves, port does not",
which is the same bucket as a routine nobody has looked at.

**And it does not merely lose its subject, it starts answering about the
wrong one.** `frames.py` recognises a reservation by the shape
`_Alignas(2) uint8_t X[N]`, which is what `framify.py` emitted - so after the
promotion it counts ordinary local buffers as reservations and compares them
against the original's `sub sp`. `load_palette`'s `buf[0x300]` and
`read_far`'s 0x100 bounce buffer are C locals now and their sizes have
nothing to do with the original's stack. Measured on 2026-09-10: two
routines reported as "reserves the locals", five as a split frame, all of
them arrays that are simply variables.

Fixed the same day by matching only the two names `framify.py` emits -
`frame` and `dgframe` - rather than any `_Alignas(2) uint8_t X[N]`. It now
reports three routines reserving, which is the three `dg_alloca` calls and
nothing else. The rule was right when it was written and expired when the
arrays it was watching stopped existing, which is the shape of every stale
comment in this file.

That is a real loss and it is worth taking, because the frame it replaces is
a hazard of its own: slots in one buffer are neighbours, so a write through
one can run into the next, and a C local cannot. The check that remains is
the compiler's - a slot too small for what is written through it is a type
error rather than a silent overrun.

What makes the trade safe is measured rather than assumed: **no scalar slot
in the tree is indexed past `[0]`**, and every callee handed a slot's address
writes exactly two bytes through it - `game_fread_far` is `game_fread(buf, 2,
1, file)`, `write_word` is `game_fwrite(addr, 2, 1, file)`, `rotate_point`
uses `dg_rd16`/`dg_wr16`. So nothing was relying on a slot's neighbour.

### A `_seg`/`_off` pair with arithmetic on one half is a pointer

**A `_seg`/`_off` pair with arithmetic on one half is a pointer.** The two
words are how a 16-bit machine had to carry an address; the moment a routine
does `off++`, `off += n` or `(uint16_t)(off + n)` while the segment sits
still, what it is holding is one address and the port should hold it as one.
`MK_FP(seg, off)` at the point the pair is loaded, ordinary pointer
arithmetic after it.

Two things make it exact rather than approximate. Where the original **files
the offset back** into guest memory - `decompress_lzw` writes `word_5894 = di`
at two exits - the port computes `out - MK_FP(seg, 0)`, which
is the offset within the segment it started from and truncates exactly as
`inc di` did, so the compared DGROUP is unchanged and so is the wrap. And
where the offset is a **second life of a register** - `di` in the same
routine is the scratch index before it is the output cursor - only the life
that is an address converts.

Measured on 2026-09-10: **28 routines** hold such a pair and step it, from
`read_record` with twelve sites down to one apiece. The rule does not reach a
pair that is only *stored* - `read_resource` normalises into DGROUP
0x5894/0x5896 for three decompressors to walk, and that pair is the cursor
itself rather than a way of writing an address down.

### `dg_near` on a pointer that is not in DGROUP is a number, and the compiler will hand it to you without complaint

**`dg_near` on a pointer that is not in DGROUP is a number, and the compiler
will hand it to you without complaint.** Converting a routine's frame to a
`uint8_t frame[N]` makes its slots C-stack pointers. Where such a slot is
passed to a routine that still takes a DGROUP offset, the build fails with
"makes integer from pointer" - and wrapping the argument in
`dg_near(dgroup, x)` makes that error go away while computing the distance
between two unrelated objects. `dg_near` takes a `void *`, so nothing objects.

**Forty-one call sites were "fixed" that way in one sitting and every one was
wrong**, with a clean build at the end of it: `copy_file_record`,
`read_bmp_info`, `far_memcpy`, `huge_move`, `draw_scroll_text`,
`read_resource` - each writes through the address it is given, and would have
written into whatever that arithmetic pointed at. `read_record`'s own comment
says it out loud: "their addresses are handed to `read_resource` as
`SS:offset` - which in this program is a DGROUP address."

So **the callee's signature decides, never the compiler's silence.** A slot
may only become a C local when every routine it reaches takes a pointer;
`tools/framify.py` reads the prototypes and refuses on the rest, and the
count of convertible routines went from 43 to 1. That is the true number, and
it was 43 only because the compiler was being asked a question it cannot
answer.

### A frame slot whose value is filed into DGROUP must stay an offset, and getting that wrong reads exactly like the timer defect

**A frame slot whose value is filed into DGROUP must stay an offset, and
getting that wrong reads exactly like the timer defect.** Converting a
routine's `[bp-N]` locals into a `uint8_t frame[N]` turns each slot into a
host pointer. `vm_init` stores its frame pointer into `DG618A.fonts_off`,
which the guest reads back, and `draw_compressed_bitmap` stores one slot's
address into another - so both filed a truncated host address where a DGROUP
offset belongs.

The symptom was **one level in ten failing to solve, a different one each
time**: level08, then level06, then level01. That is indistinguishable by
eye from the non-determinism STATUS.md documents under the timer thread, and it cost three
ten-minute batches and two wrong theories - a broken `parts.c`, then CPU
contention from running other things beside the check - before the control
that settles it: **stash the work and run the batch at HEAD.** HEAD passed
10 of 10, so the fault was mine, and after that it was a diff to read rather
than a run to repeat.

Two lessons. When an intermittent failure appears, get the control before
the theory: the question "does this happen without my changes" is one run and
it ends the argument. And a converter that hands out pointers needs to refuse
any routine that stores one where the guest will read it - the test is
whether the address is only *read through*, never filed.

### A fix for undefined behaviour is where a value change hides, and the cast has to be the field's own width

**A fix for undefined behaviour is where a value change hides, and the cast
has to be the field's own width.** `integrate_object` clamps a part that
leaves the top: `pos_y = -1000; fy = -1000; fy <<= 9`. A left shift of a
negative value is undefined, UBSan said so, and the fix was written as
`fy = (int16_t)((uint16_t)fy << 9)` on the reasoning - true of a 16-bit value
- that shifting it unsigned is the same bits. **`fy` is the `int32_t` half of
its union.** The cast truncated twice and put 12288 where -512000 belongs,
and the next line is `pos_y = fy >> 9`: the part reappeared at **y = 24**,
rose off the top, was clamped again, and looped. That is a rocket that will
not leave the screen, and it was found by a user playing and then bisecting,
not by anything here.

The commit that introduced it said in as many words: *"None of the five
changes a computed value on any machine this runs on."* Four of the five did
not. **A claim like that is a measurement**, and the way to make it one is to
print the value before and after - which takes a minute and would have shown
`-512000` against `12288` immediately.

Two checks were named in that commit as covering it and neither could.
`verify.py`'s spec for the routine is `check_occurrences=[0]` and the clamp is
not on the first call, so the compared call never entered the branch -
`tools/native/covered.py` exists to measure exactly this and was not run.
`check_native.py` compares the intro's 66 flips and the rocket leaves after
that window; STATUS.md had already written down that the flip comparison might
not reach it, and was right. The three sibling clamps beside it kept `<<= 9`,
so the one that was rewritten stood out in the file and nobody looked.

### Deleting the two lines that copied a pair out left the declaration that made them necessary, and C called that a new variable

**Deleting the two lines that copied a pair out left the declaration that
made them necessary, and C called that a new variable.** Converting a
`seg`/`off` pair to one `struct far_ptr` has a standard shape: a routine
keeps the pair in its own frame, an inner block allocates into a local
`struct far_ptr blk`, and two lines copy `blk.seg`/`blk.off` back out. With
the outer pair now a `far_ptr` the two copies are redundant and go - and in
`load_bitmap_list` the inner **declaration** went with neither. The
allocation died at the closing brace, the routine's own `blk` stayed
`FAR_NULL`, and every bitmap in the list was read to 0000:0000.

It built clean under `-Wall -Wextra`, `make test` was green, and the
reference and the port agreed on the briefing screen's text, its panel and
its buttons. What differed was **420 pixels**: a 24x20 box at (320,200),
which is the mouse cursor, drawn as coloured noise. A user playing said
"corruption with pointer now"; nothing here was looking.

`check_briefing.py --screen briefing` is a *cheap deterministic* reproducer -
three flips, four minutes, the same 420 pixels every run - so it bisects.
Seventy commits and six builds put it on the exact commit. **When a screen
check goes red, bisect it before reading any code**: the range was `blk`
pairs, font slot tables and bitmap headers, all plausible, and reading would
have gone to the drawing routines, where nothing was wrong.

The oracle nearly threw the bisect away. Its first version asked
`grep -q "pixels differ"`, and the clean line reads
`**0 of 307200 pixels differ**` - so *good* and *bad* both matched and the
known-good commit came back bad. That is this file's own rule about a verdict
that cannot say what kind of "no" it means, met in a four-line shell script:
**match the number, not the noun.**

`-Wshadow` is in `CFLAGS` now. It found two more shadows in the port, both
harmless - a `w` in a path after a `return`, a `rec` holding the same address
as the `at` beside it - and both are gone, because the value of the flag is
that it has nothing to say.

### A pair is found by what the code does with it, not by what the two halves are called

**A pair is found by what the code does with it, not by what the two halves
are called.** Converting every `seg`/`off` pair to `struct far_ptr` was driven
by grepping the names - `_off`, `_seg`, `_lo`, `_hi` - and that finds only the
pairs somebody had already named consistently. The ones named badly are
exactly the ones still hiding. `DG4A82.directory_ptr` and `payload_seg` are
one far pointer at +0x20 and +0x22, and a name-based sweep had written the
second off as "a lone segment, no offset beside it" - it has one, under a
name ending `_ptr`.

Five shapes find them, and all five are greppable:

- a **comment that already says it** - `int16_t X[2];  /* a far pointer */`.
  This is the highest-yield of the five and the most embarrassing: five
  sites in one grep, every one correctly described in prose beside a type
  that had never caught up. `replay_shapes` declares three of them and then
  unpacks one into `cs`/`co` every iteration;
- a field used in **segment position** - `FAR8(X, Y)` or `MK_FP(X, Y)` where
  `X` is not somebody's `.seg`;
- a **hand-built literal**, `(struct far_ptr){ a, b }`, from two named things;
- a **two-compare zero test**, `a != 0 || b != 0`, or `(a | b) == 0`;
- an **assignment from another pointer's halves**, `x = p.seg; y = p.off;`.

The identifier names are the *weakest* signal of the six and were the one
the work started from. They find the pairs somebody had already named
correctly, which are the ones least likely to be wrong.

They found `DG4A82.directory`, `DG48F8` (zero-tested as
`huge_equal(off, seg, 0, 0)` and returned as `(seg << 16) | off`), and
`DG3890.pal_copy_ptr`, which was an anonymous `{dg_near_t off; dg_seg_t seg;}`
- already that layout, with no name for the type. The same sweep found four
sites reaching a table by raw arithmetic where a typed array already existed:
`0x3a2e + 4 * di` is `DG3A2C.blocks[di]`, and `bx + 0x618a` with
`bx = 4 * index` is `FONTSLOT[index]`.

**And reading the diff found two more things the greps could not.**
`close_table_618a_slot` ends with six further `bx` accessors clearing the
same three slot tables, and retiring those retires `bx`; and
`alloc_voice_records` reads its pointer *back out of* `VOICES[i]` after
storing it, where a first pass had substituted the local it came from - the
same value, not the same code. The greps say where to look; the diff says
whether the change is right.

**And converting the type is not the same as retiring the idiom.** With
every field converted, the tells still found 19 sites where the type was
already `struct far_ptr` and the call site went on taking it apart -
`if (DG4A82.config.off != 0 || DG4A82.config.seg != 0)` and
`X.seg = p.seg; X.off = p.off;`. A name-based sweep reports such a file as
finished, because no identifier ends in `_off` any more.

**One fold needed the disassembly and one did not, and the difference is
whether the two spellings can disagree.** `read_input_block`'s halves were
an *ordering* compare and the port had the arms backwards - only the binary
could settle that. `read_record`'s `if (lo < 4) hi--; lo -= 4;` is `len -= 4`
for every input including wrap: an algebraic identity, where asking the
binary would prove nothing the C does not already say.

### Two halves compared separately are safe to fold only when the test is equality

**Two halves compared separately are safe to fold only when the test is
equality.** `read_input_block`'s was an *ordering* compare and the port had
the arms of the `min` the wrong way round; `read_record`'s chunk-exhausted
test at 0x24136 is `cmp`/`jne` twice, and equality on two halves is equality
on the whole unconditionally. The same routine also settles what the
original's source said: it masks with `and dx, 0xffff` then `and ax, 0x7fff`,
and the first of those masks a 16-bit register with 0xffff, which does
nothing. It is only there as the low half of one 32-bit `& 0x7fffffff` - so
Borland was handed a `long` and a 32-bit mask, and the port's two-word
spelling was the deviation. Folding it back is restoring, not
reinterpreting.

### A struct field is a claim about width, and a narrower one is a short read that compiles

**A struct field is a claim about width, and a narrower one is a short read
that compiles.** Turning `DG*(base + offset)` into a named field replaces an
access whose width is written on it with one whose width is written somewhere
else, and the two can disagree in only one direction safely. A *narrower*
accessor on a wider field is fine and always was - `DG8` on a word is reading
its low byte, which is what the original does. A *wider* accessor on a
narrower field is a different read: `DG32(si + 0x16)` became
`((int32_t)PART(si).word_16)`, two bytes instead of four, and three levels
stopped solving because that is a part's momentum. The cast is what makes it
invisible - it looks like the widening the surrounding arithmetic wants.

The same one size down had already gone in unnoticed the commit before:
`reverse_link_ends` swaps +0x6a with +0x6c using a 16-bit move, and a pass
with no width check turned that into a swap of `byte_6a` and `byte_6c` -
half of each pair. **It solved 29 of 29 anyway**, because reversing a run of
pulleys is on no solution's path, and that is the more useful half: the
behavioural checks cover what the levels do, so a defect off that path can
sit in the tree indefinitely with everything green.

So a converter must **refuse** a site it cannot spell at the right width and
name it, never cast it. And where two byte fields are moved as a word - a
part's grab box at +0x56, its two attachment offsets at +0x6a and +0x6c -
the pair is the type: `struct point8`, so `clone_part` stays the three
16-bit moves the original makes.

### An empty evidence set is not evidence for the wider type

**An empty evidence set is not evidence for the wider type.** `framify.py`
chose a frame slot's C type from the accessors it could read: all `DG16` at
even offsets meant `int16_t *`, anything else meant `uint8_t *`. A slot with
**no** readable accessor satisfies "all of them are 16-bit" vacuously, and
six slots were typed `int16_t *` on the strength of nothing at all - the same
shape as the vacuous verdicts further up this file, one layer down.

Two of the six were live. `path_join` filled its filename buffer with
`DG8((uint16_t)(name + di))`, which on an `int16_t *` truncates a **host**
pointer to a DGROUP offset and writes somewhere else entirely, so the
`strcat` that followed appended an uninitialised frame. `picker_type` did the
same with `DG8((uint16_t)(str + 1)) = 0` and typed characters into the picker
without their NUL. Both compiled and both had been in the tree for days.

The compiler said so every time: `-Wpointer-to-int-cast`, exactly the
diagnostic for this, on both lines. It was invisible because the build check
in use was `make 2>&1 | grep -E "error"`, which is a filter that removes the
class of finding it was looking for. **Grep the build for `error|warning`,
never `error` alone.**

The reason a byte slot even reaches the compiler half-converted is that
`framify.py` rewrote the accessors of word slots and left byte slots to
`framify_fixups.py`, which is per-function and missed one. It now converts
`DG8` on byte slots itself, refuses a slot read at a variable index
with a width above a byte, and requires a non-empty width set before it will
say "word".

### A frame stops being convertible for three reasons, and only two are about the code

**A frame stops being convertible for three reasons, and only two are about
the code.** Converting a `dg_alloca` frame to a `uint8_t frame[N]` needs every
callee it hands a slot to to take a pointer. Where that is not possible it is
because the callee needs a *guest offset*, and the offsets have three
different origins:

- **far** - the value is half of a `seg:off` pair. `draw_string` hands its
  string to `draw_string_body(str, DGROUP_SEG, ...)`, which reads it with
  `FAR8(seg, str)`. Note that the pair can be *named* rather than
  dereferenced: `read_resource` takes `dst_off, dst_seg` and passes both on
  without a `FAR8` anywhere in its body.
- **filed** - the address is stored into guest memory and outlives the call.
  `borland_setvbuf` puts the buffer into a file record's `read_ptr`, read back
  later as a DGROUP offset. A C array has no offset to store.
- **polymorphic** - the value is a handle *or* an address, told apart by a
  numeric test. `load_bitmaps` asks `file_record_valid` whether its argument
  matches an open record's `file_ptr`; a C array's `dg_near` is an arbitrary
  16-bit number that could match a live handle, and no pointer type can
  express the choice. `call_sound_module` is the same shape from the other
  end: its second argument is read by the module's own emulated code through
  SI.

The first two `framify_census.py` derives; the third is a short by-hand list
with a reason each, because a wrong automatic verdict is worse than a named
exception. Measured on 2026-09-09, with the mechanical work finished, the
census accounts for every frame left and the sum is printed so it cannot
quietly stop adding up: **8 + 0 + 17 + 2 = 27**. Eight have no blocking
callee and are refused by `framify.py` over their own slots - four reach
`draw_string` or `far_copy` through a cursor, four file a slot's address.
None is waiting on work. Seventeen are behind one of the three walls. Two -
`game_screen` and `poll_sequences` - reserve DGROUP stack with no slots of
their own, the first so its callees' frames land below it and the second
because the sound module reads its block through SI; neither goes until what
is under it stops needing DGROUP.

**The far wall came down first, and it was the model that was already
chosen.** "Offsets from the dgroup for near pointers, and offsets from 0 for
far pointers" is the instruction this work started from, and a far pointer as
an offset from 0 *is* a host pointer into `guest_mem`. So `dg_far`/`dg_cfar`
join `dg_near`/`dg_cnear` in tim.h: one C parameter where the guest pushes
two words, offset then segment, which is what `aptr` in the hybrid's shims
already builds. `draw_string_body` took `(str, seg)` and takes one pointer;
`draw_string` and `draw_scroll_text` follow, and seven frames behind them
convert. `FAR_PTR` is spelled `MK_FP` now, Borland's own name, and `FP_SEG`/
`FP_OFF` take a pointer apart into the **normalised** pair - the only
pair it can answer for, because a host pointer does not remember which of the many `seg:off` pairs that address it the guest was
holding.

One thing to transcribe carefully on the way: `draw_string_body` opens
`if ((str | seg) == 0) return;`, and that is a far pointer of 0000:0000,
which is `guest_mem` and **not** a C null pointer. Written as `str == NULL`
the guard never fires.

**A far pointer is only convertible if the callee `*`s it.** `huge_move`
looked like the next one - its source is read and never written - and it is
not: the routine does not read the source, it *indexes guest memory* with
the source's linear address and takes the copy's direction from comparing
that address with the destination's. A frame handed in as a host pointer is
a C array with no guest address, so `src - guest_mem` is a wild number and
the copy walks off the end of memory. `verify.py` segfaulted, which is the
good outcome; a smaller offset would have copied the wrong bytes quietly and
agreed with nothing that was looking.

So the test is not "is the value only read" but **"is the value only
dereferenced"**. `draw_string_body` passes: it walks the string and reads
bytes. Two more do not, for reasons worth their own names: `far_move` and
`far_memcpy` step the offset with `(uint16_t)(off + n)`, which is the
original's 16-bit `add` and a wrap a host pointer cannot express; and
`read_resource` files the normalised pair into DGROUP 0x5894 for the
resource reader to pick up, so the pair is *stored*, and a pointer has no
pair to store.

What is left needs the other half of the decision: whether a frame may stay
in DGROUP, which is what a filed address, a polymorphic handle and a linear
address all come down to. Measured on 2026-09-09 with that half untouched:
**5 + 0 + 12 + 2 = 19**, and every one of the nineteen has a reason written
down. `far_move`, `far_memcpy` and `far_copy` wrap; `read_resource`,
`borland_setvbuf`, `decode_vqt_list`, `draw_compressed_bitmap`, `vm_init` and
`blit_scaled_a` file; `huge_move` is linear; `load_bitmaps` and
`call_sound_module` are polymorphic; `game_screen` and `poll_sequences`
reserve for what they call.

**There is a halfway house and it is not worth taking uninvited.**
`framify.py --in-dgroup` gives a walled frame the same *shape* as a
converted one - `uint8_t *frame = dg_ptr(dgroup, dg_alloca(N))`, typed slots,
array indexing - while keeping `dg_alloca`/`dg_free`, because the bytes
really do have to be the guest's. It works, and on `draw_compressed_bitmap`
and `blit_scaled_a` it produced sound but poor C: the offset wrapper rewrote
a slot's name inside a *comment*, and a slot read at two widths came out as
`DG16(dg_near(dgroup, vcut))` where `dg_rd16(vcut)` says it. Both are
cosmetic, the gain is readability rather than correctness, and a regex pass
over the blitter for that trade is not one to make without being asked. The
mode is in the tool with its limits written down; the drawing routines were
left as they are.

**And the census counts routines, not frames.** Three routines reserve a
second frame inside the first - `read_far` and `load_bitmaps` each build a
four-byte far pointer for `huge_add_to` to step, `poll_sequences` builds the
block the sound module reads through SI - and `framify.py` looks for one
`dg_alloca` per routine, so it never saw them. The two `huge_add_to` ones are
arrays now; the sound one is walled like its sibling. A routine that is
walled can still hold a frame that is not.

### A frame is walled slot by slot, not routine by routine

**A frame is walled slot by slot, not routine by routine.** `decode_vqt_list`
reserves 0x1ca and only *one* of its slots has to be the guest's: `rd`, the
reader record, whose address is filed into `DG6400.word_640c` for `vqt_node`
and `vqt_screen_node` to pick up. The other named slot, `cur`, is a four-byte
far pointer `huge_add_to` steps - and its comment said "so it needs a real
DGROUP address", which stopped being true the day `huge_add_to` took a
pointer and was never revisited. It is an array now.

So the useful question of a walled frame is *which slot* holds it there, and
the answer is often one of several. The reservation stays the original's
`sub sp` either way; the bytes whose locals became C ones are simply no
longer read, which is already true of every frame that converts completely.

**A comment that records a constraint outlives the constraint.** This one
had been right when it was written. Grepping for the phrase rather than the
code is what found it.

**And the commit that did it claimed a check it had not made.** Its message
ended "the whole intro 66 flips byte for byte - which is the check that
draws VQT bitmaps". Nothing draws VQT bitmaps. Instrumented afterwards,
`decode_vqt_list` is entered **zero** times by the intro and zero times by
all twenty-eight level snapshots; `verify.py` has no spec for it, and the
four routines below it - `vqt_node`, `vqt_screen_node`, `fill_quadrant` -
go with it. Its conversion rests on the build and on the frame-size check
and on nothing else.

This is the mirror of the `draw_polygon` entry above, and the worse
direction. There the write-up said a routine was unreachable when a level
reached it, and the fix was to look harder. Here a green check was named as
covering a routine it never entered, which is a claim a later reader has no
reason to doubt. **A coverage claim is a measurement**, and "the screens
pass and this routine is on a screen" is not one.

### Filing one slot's address into another slot of the same frame is not filing

**Filing one slot's address into another slot of the same frame is not
filing.** `draw_compressed_bitmap` writes `DGU16(vp) = scratch`, and that
was read as the frame's address escaping into guest memory - the refusal
that kept the routine out of the conversion for weeks. `vp` is `[bp-0x10]`:
a slot of the *same frame*, holding a cursor into `scratch`. The two move
together whatever the frame is made of, so the address never leaves.

What it does need is for `vp` to stop being a two-byte slot and become a C
pointer - which is a rule of its own: **a word slot whose value is another
slot's address is a cursor, not storage.** The port was already doing that
conversion by hand at the two `vm_blit_run` calls, spelled `dgroup +
DGU16(vp)`, which is the tell.

It pulled `vm_blit_run`'s `src` to `dg_cfar` and found the second place the
shim generator cannot see through the typedef: the register-pair branch
tests for `*` the same way `kind_of` did. That one aborted the build rather
than taking the wrong branch quietly, which is the failure mode to want.

**A write *through* a pointer is not a write *of* it, and the census could
not tell them apart.** `heapwalk` steps the record it is handed -
`DGU16(info) = (uint16_t)(DGU16(info) + 4)` - and the parameter on the right
is inside an accessor on itself. The filing test saw the name on the right
of an `=` and called it filed, which is what kept `heap_largest_free` walled.
Stripping every accessor *on that parameter* before asking the question
fixes it.

That was the fourth false "filed" of one afternoon: three from a slot's
address going into another slot of the same frame, one from this. The
verdict has now been wrong more often than right, so treat it as a lead
rather than a finding: **read the routine before believing "filed".**

### A `seg:off` pair held in two variables is one pointer if it is only dereferenced

**A `seg:off` pair held in two variables is one pointer if it is only
dereferenced.** `remove_and_free_records` keeps the previous link as
`link_off`/`link_seg`, starting at a two-word cell in its own frame and then
becoming each record in turn - which is why its comment said the cell has to
be an ordinary DGROUP address. Every use of the pair was
`MK_FP(link_seg, link_off)`: dereferenced, never stored and never compared
as a number. One `uint8_t *` says the same thing, `MK_FP` makes one for
the heap case, and the cell is a C array.

The tell is the same as `draw_compressed_bitmap`'s: the port was already
building a host pointer at every point of use, and the two words were only
the shape the original had to keep it in.

### The line that separates the frames that convert from the ones that do not

**The line that separates the frames that convert from the ones that do
not**, arrived at by getting it wrong four times in one day. Every wall is a
value the port hands to something else, and there are only two kinds:

- **compared** - the guest asks a question *about* the number, and the port
  can answer the same question about a pointer. `load_bitmaps` asks whether
  its argument is one of four open handles; a pointer outside guest memory
  is certainly not, so `dg_is_guest` answers exactly and the frame converts.
  A value that is only ever compared has a way through.
- **stored, or used as an address** - the number goes into guest memory and
  is read back, or is used to index guest memory. `borland_setvbuf` puts the
  buffer in a file record's `read_ptr`; `read_resource` puts the pair at
  DGROUP 0x5894; `decode_vqt_list` puts `rd` at DG6400.word_640c; `vm_init`
  puts BP at DG618A.fonts_off; `huge_move` takes `src - guest_mem`. A guest
  word cannot hold a host pointer and a C array has no linear address, so
  there is no way through without changing what the two artefacts compare.

Four routines were walled on a *third* thing that turned out not to exist:
a rule stated correctly and applied without checking its premise.
`game_screen` reserved for callees when its own locals were C; `vm_init` was
called an accident when it reproduces BP exactly; the sound module was
called emulated when it is `src/sxovl_asb.c`; `load_bitmaps` was called
unanswerable when the question is decidable. **Read the callee, not the
sentence beside the frame.**

### `dg_call`/`dg_uncall` are gone, and they were bookkeeping for a comparison nobody makes

**`dg_call`/`dg_uncall` are gone, and they were bookkeeping for a comparison
nobody makes.** They moved `guest_sp` by the bytes a call itself pushes -
the arguments and the return address - so that a callee's `dg_alloca` frame
landed exactly where the original's did. That only matters if a frame's
*address* is compared, and the stack is deliberately not matched: only the
global DGROUP is. Twenty-five call sites and the two routines went;
`read_resource`, `borland_fopen_into` and `game_fopen` verify over 5, 28 and
12 calls afterwards, the intro is 66 flips byte for byte, 28 of 28 solutions
solve and `check_sound` is identical.

Two blocks existed only to bracket them - a `{ int16_t ok = ...; dg_uncall;
if (ok == 0) ... }` reads as a plain `if` once the call between the two is
removed - and went with them.

**And a refusal that names the wrong wall points at the wrong fix.**
`framify.py` reported four routines as filing a slot's address, on the
strength of `uint16_t si = buf;`. They do not: `si` walks the buffer and is
handed to `draw_string`, which is the *far* wall, and the assignment only
looked like filing because `si` had failed the cursor test and so was not
recognised as derived from `buf` at all. A rejected cursor candidate now
carries the callee that rejected it, and the refusal says "buf reaches
draw_string through si". Four of the eight filed verdicts were this;
the other four - `decode_vqt_list`, `draw_compressed_bitmap`, `vm_init`,
`blit_scaled_a` - really do file a slot address, which is the finding
recorded further up this file.

### `dg_near` refuses a pointer that is not the guest's, and the first thing it caught had been in the tree for weeks

**`dg_near` refuses a pointer that is not the guest's, and the first thing it
caught had been in the tree for weeks.** The forty-one wrong `dg_near` sites
further up this file were found by reading; nothing stopped a forty-second,
because `dg_near` takes a `void *` and the compiler has no opinion. It now
calls `port_abort` when the pointer is outside `guest_mem`, which was tested
the only way worth testing a guard - by handing it a C local on purpose and
watching it fire.

Turned on, the port died in `game_startup`. One routine, three callers:
`string_concat` decides whether to run the original's one-`movsb` alignment
step from `dg_near(dgroup, src) & 1`, because the parity the original tests is
the *segment's*, not the host pointer's - and `count_level_files`,
`load_level` and `load_part_bitmap` all hand it a C array, whose `dg_near` is
the distance between two unrelated objects. The branch was a coin toss. It
cost nothing, because the two arms copy the same bytes, and that is exactly
why it survived: no comparison could see it. The test is now asked only where
there is an offset to ask it about.

**The two checks that were running at the time both said the port was fine.**
With every level's port aborting on SIGABRT, `check_solutions.py` printed
**33 of 33 solved** - because "io: level solved" really was in the output,
four lines before the crash - and `check_sound.py` printed "the port plays
the original's samples" off **one** run of blocks against fifty-five, its
own content-alignment being deliberately happy with different depths.
Neither looked at the exit status. Both do now, against a greppable
`io: PORT ABORTED` banner, and both were checked in both directions: they
pass on the healthy port and refuse a verdict on a deliberately broken one.

So: **a check that reports on a run must first establish that the run
happened.** Solving and then dying is not solving.

### A typed handle tested as a boolean is always true, and the compiler will not say so

**What happened.** Converting handle locals from DGROUP offsets to typed
pointers - `uint16_t link` becoming `struct rope *link` - left one line in
`part_under_pointer` untouched:

```c
uint16_t link_end = link ? link->owner_ptr : 0;
```

As an offset, `link ?` asked the original's question: is there a rope, i.e. is
the offset non-zero. As a pointer it asks whether `link` is NULL, and it never
is: `ROPE_PTR(0)` is `dgroup + 0`, a real address. So for a part with no rope
the test passed and `link_end` became whatever word sits at DGROUP:0 plus the
field's offset - the Borland banner - instead of 0. It compiled without a
warning, because a pointer in a boolean context is legal C.

**What it cost.** It landed in a commit (e8045fc) that had passed every check
the project has: both builds, `make test`, the full verification sweep, the
intro byte for byte over 601 flips, and all 29 solutions identical and solved.
None of them could see it. The difference only matters when a part has no rope
*and* the `exclude` argument equals that stray word, which no captured run
reached. It was found by reading the routine for an unrelated change.

**What settled it.** Every `*_NONE` sentinel exists because a typed handle's
"none" is DGROUP:0, never NULL - so `if (p)`, `p ? :`, `!p`, `p &&` and
`|| p` on a typed handle are all the same fault. The conversion batches now
fail on a scan for exactly those shapes over the whole game source, run
before anything is committed; on e8045fc it flags that one line and nothing
else. The rule: **compare a typed handle with its sentinel, always, and never
use it as a boolean** - and when a check suite cannot reach a fault's
condition, a structural scan is the check.

### An object the linker puts at an odd address is one the compiler assumed was aligned

**What happened.** The image's data became C objects placed by a linker script
at their DGROUP offsets, with `SUBALIGN(1)` so that an array the compiler had
aligned to 16 was not moved off its address. Every placed object checked out -
at its address, holding the image's bytes - and the intro ran byte for byte
identical. Then every solution segfaulted as its level loaded, in
`reset_input_state`, on `b->state = 0`.

GCC had compiled the two-button clear into one `movaps` straight to
`MACHINE_BUTTONS`. The x86-64 ABI lets a compiler assume a global of sixteen
bytes or more starts on a sixteen-byte boundary, and GCC acts on it for an
object it defines; `movaps` faults on an address that is not. DGROUP 0x5742 is
not, and nothing about the section attribute told the compiler so. The first
prototype had shown the same thing from the other side - a table placed two
bytes past its offset - and `SUBALIGN(1)` fixed the placement without fixing
the assumption.

**What it cost.** Nothing that shipped, because the solutions run caught it -
but only because that run reaches the routine. The intro did not, and neither
did the placement check, which by construction cannot see what the code that
*uses* an object assumes about it.

**What settled it.** `aligned(1)` on the definition: GCC then gives the section
alignment 1 and uses `movups`. It is part of `DGROUP_AT`, `DGROUP_BSS` and
`SEGMENT_AT`, so no placed object can be defined without it, and
`tools/genld.py` refuses to write a script if any guest section in any object
is aligned to more than 1. A declaration elsewhere needs nothing - GCC assumes
only the type's alignment for an object it does not define. The rule: **when
the linker decides an address, the compiler must be told the alignment that
address has.**
