# The Incredible Machine - reconstruction.
#
# Two binaries, deliberately: `tim` is the game and mirrors the original's
# start-up, `devtim` carries every developer flag a comparison needs. tools/
# calls devtim, so nothing a measurement depends on can drift into what ships.

CC      ?= cc
# **-fno-strict-aliasing, and it is not optional.** DGROUP is a `uint8_t`
# array and every `DG16`/`DGU16` reads it through a differently-typed pointer,
# which is an aliasing violation the compiler is entitled to act on. It has not
# acted on it because those go through a global by a computed pointer, which
# GCC treats conservatively - so the port has been relying on a compiler's
# restraint rather than on the language. A routine's frame as a local
# `uint8_t frame[N]` read through `int16_t *` is the same violation somewhere
# GCC can see all of it, and there it would bite. The flag says what the port
# has always meant: these bytes are the guest's memory and any type may read
# them.
# **`-Wshadow` is here because a shadow cost a screen.** Converting a routine's
# `seg`/`off` pair to one `struct far_ptr` meant deleting the two lines that
# copied an inner declaration out to the outer pair - and in `load_bitmap_list`
# the inner *declaration* stayed. The allocation went into a `blk` that died at
# the closing brace, the function's own `blk` stayed `FAR_NULL`, and every
# bitmap in the list was read to 0000:0000. The build was clean and the cursor
# drew garbage.
# -Wno-address-of-packed-member: the DGROUP structs are packed on purpose, and
# a routine that walks an array inside one takes its address - poly_outline
# over DG3890.poly_x. The structs are to be unpacked later; until then the
# warning would only ever say what the model already says.
CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Wshadow -Wno-unused-parameter -Wno-address-of-packed-member -fno-strict-aliasing

# **The reconstruction is C. The one exception is the OPL2**, because ymfm is
# C++ - see src/opl.h for why an OPL2 is not transcribed at all, and
# vendor/README.md for what ymfm is and its licence. The same arrangement as
# the Lemmings port, deliberately: one C boundary, one C++ file behind it.
# Vendored code is compiled without this project's warnings, because they are
# this project's rules and not its author's.
CXX      ?= c++
CXXFLAGS ?= -O2 -g -std=c++14
# The headers stay beside io.c; the game's own sources are a directory down.
CFLAGS  += -I.
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)

# The game itself, under src/: the eight modules transcribed from the
# original's code segments, the DGROUP array they all reach through, and the
# two overlays. `io.c`, `sdl.c` and the borland_*.c runtime are not the
# game - they are the hardware, the window and the C library it was linked
# against - and the dev*.c files are not shipped at all.
SRC  := src
GAME := $(SRC)/machine.c $(SRC)/game.c $(SRC)/machine_draw.c $(SRC)/parts.c \
        $(SRC)/engine.c $(SRC)/bitmaps.c $(SRC)/sound.c $(SRC)/trig.c \
        $(SRC)/dgroup.c $(SRC)/vmovl_vga.c $(SRC)/sxovl_spkr.c \
        $(SRC)/sxovl_asb.c $(SRC)/sxovl_adl.c $(SRC)/sxovl_sbp.c \
        $(SRC)/sxovl.c
PORT := io.c borland_heap.c borland_file.c borland_huge.c

# The OPL2 and the ymfm units it needs. ymfm_opl.cpp pulls in the other three
# through its templates even though a YM3812 has no ADPCM, PCM or SSG in it.
OPL_SRC := $(SRC)/opl_ymfm.cpp \
           vendor/ymfm/ymfm_opl.cpp vendor/ymfm/ymfm_adpcm.cpp \
           vendor/ymfm/ymfm_pcm.cpp vendor/ymfm/ymfm_ssg.cpp
OPL_OBJ := $(OPL_SRC:.cpp=.o)
OPL_PIC := $(OPL_SRC:.cpp=.pic.o)

# The developer hooks, and the shipping binary's do-nothing stand-in for them.
# `io.c` calls dev_flip_dump on every page flip whatever it is linked into, so
# one of these two is always present - and only `devtim` gets the one that can
# be asked to write frames or press a mouse button.
# `devwav.c` is separate from both: `devtim` and the hybrid runner each link it
# and the shipping binary links neither, which is how a recording of the port
# and a recording of the original can be made with the same writer.
DEV  := devdump.c devwav.c
NODEV := devstub.c

# **Headers count as sources.** Rules that list only the `.c` files mean a
# change to `io.h` alone rebuilds nothing and the next run uses the old build -
# silently, and answering with complete confidence. Raising DOS_ALLOC_PRIMED
# from 16 to 512 appeared to change nothing for an hour because of this, and a
# whole false finding was written up from the stale result.
#
# **And it has to be defined before the rules that use it.** It sat below them
# until 2026-09-05, and make expands a prerequisite list when it reads the rule,
# so `$(HEADERS)` there expanded to nothing and the dependency never existed.
# Measured at the time: `touch io.h && make` compiled zero files. The fix that
# was written up as done had never taken effect.
HEADERS := $(wildcard *.h)

# The objects. Compiling each unit once and linking twice is what makes an
# edit to one file rebuild one file - and lets `make -j` use the other cores.
# `tim` and `devtim` compile with identical flags, so they share these.
COMMON_OBJ := $(patsubst %.c,%.o,sdl.c $(GAME) $(PORT))
TIM_OBJ    := $(SRC)/main.o $(COMMON_OBJ) $(patsubst %.c,%.o,$(NODEV))
DEV_OBJ    := devmain.o     $(COMMON_OBJ) $(patsubst %.c,%.o,$(DEV))

all: tim devtim

# **`all` stays the first target.** Adding `compile_commands` above it made it
# the default goal, so a bare `make -C reconstruct` regenerated the database and
# built nothing - every time, since it has no prerequisites to be up to date
# against. It is in the .PHONY list at the foot of this file with the others.

# `compile_commands.json` for clangd, written to the repository root so an
# editor opened anywhere in the tree finds it.
#
#     make -C reconstruct compile_commands
#
# **Generated from the variables above rather than written by hand**, so the
# flags an editor sees are the flags the build uses: add a source to $(GAME)
# and it appears here on the next run. `bear` would do the same job by watching
# a real build; it is not installed here and this needs nothing that is not.
#
# The hybrid runner's own units are listed explicitly at the end, because they
# live under tools/native with a different include path. Its generated sources
# - shims.c, syms.c, ovlsyms.c - are included only once they exist.
ROOTDIR := $(abspath $(CURDIR)/..)

compile_commands:
	@printf '[\n' > $(ROOTDIR)/compile_commands.json
	@first=1; \
	for f in $(SRC)/main.c devmain.c sdl.c $(GAME) $(PORT) $(DEV) $(NODEV); do \
	    [ -n "$$first" ] || printf ',\n' >> $(ROOTDIR)/compile_commands.json; \
	    first=; \
	    printf '  {"directory": "%s", "file": "%s", "command": "%s"}' \
	        "$(CURDIR)" "$(CURDIR)/$$f" \
	        "$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $$f" \
	        >> $(ROOTDIR)/compile_commands.json; \
	done; \
	for f in $(OPL_SRC); do \
	    printf ',\n  {"directory": "%s", "file": "%s", "command": "%s"}' \
	        "$(CURDIR)" "$(CURDIR)/$$f" \
	        "$(CXX) $(CXXFLAGS) -I$(CURDIR) -c $$f" \
	        >> $(ROOTDIR)/compile_commands.json; \
	done; \
	for f in native.c dispatch.c shim.c guestbt.c sha256.c shims.c syms.c ovlsyms.c; do \
	    [ -f "$(ROOTDIR)/tools/native/$$f" ] || continue; \
	    printf ',\n  {"directory": "%s", "file": "%s", "command": "%s"}' \
	        "$(ROOTDIR)/tools/native" "$(ROOTDIR)/tools/native/$$f" \
	        "$(CC) $(CFLAGS) -I$(CURDIR) $(SDL_CFLAGS) -c $$f" \
	        >> $(ROOTDIR)/compile_commands.json; \
	done
	@printf '\n]\n' >> $(ROOTDIR)/compile_commands.json
	@echo "wrote $(ROOTDIR)/compile_commands.json"

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.pic.o: %.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

# `-rdynamic` puts the port's own symbols in the dynamic table, which is what
# lets `not_transcribed` print the chain of routines that reached a stub
# instead of a column of addresses. It costs a slightly larger binary and
# nothing else: no optimisation is given up, and the frames are only walked on
# the way to an abort.
BACKTRACE := -rdynamic

tim: $(TIM_OBJ) $(OPL_OBJ)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(BACKTRACE) -o $@ \
	    $^ $(SDL_LIBS) -lpthread -lstdc++

# devtim links sdl.c too, and opens a window by default exactly as tim does,
# so it can be played and Shift+F2 pressed. The tools in tools/ set
# TIM_HEADLESS=1, which is what keeps them working on a machine with no
# display: the window is what a person needs, its absence is what a comparison
# needs, and the comparison is the one that asks.
devtim: $(DEV_OBJ) $(OPL_OBJ)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(BACKTRACE) -o $@ \
	    $^ $(SDL_LIBS) -lpthread -lstdc++

# Two builds for chasing a fault rather than measuring anything.
#
#   make -C reconstruct debug   ->  ./reconstruct/timdbg
#   make -C reconstruct asan    ->  ./reconstruct/timasan
#
# `timdbg` is -O0 with frame pointers, so a core file or a gdb stack names
# every frame and no local has been optimised away. `timasan` adds the address
# and thread sanitiser's address half, which reports a bad access at the
# instruction that made it rather than wherever it later crashed. ASan slows
# the port down several-fold and shifts its timing, so it is for finding a
# fault and never for taking a measurement. `asan` needs libasan and libubsan
# installed (on Fedora, `dnf install libasan libubsan`); `debug` needs nothing.
debug: $(SRC)/main.c sdl.c $(GAME) $(PORT) $(NODEV) $(OPL_OBJ) $(HEADERS)
	$(CC) -O0 -g3 -fno-omit-frame-pointer -std=c11 -I$(CURDIR) \
	    $(SDL_CFLAGS) $(BACKTRACE) -o timdbg \
	    $(filter %.c %.o,$^) $(SDL_LIBS) -lpthread -lstdc++

asan: $(SRC)/main.c sdl.c $(GAME) $(PORT) $(NODEV) $(OPL_OBJ) $(HEADERS)
	$(CC) -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined \
	    -std=c11 -I$(CURDIR) $(SDL_CFLAGS) $(BACKTRACE) -o timasan \
	    $(filter %.c %.o,$^) $(SDL_LIBS) -lpthread -lstdc++

# **And the same thing with the developer hooks**, because `timasan` is built
# from the shipping `main.c` and a DOS game's entry point has no command line:
# it can be pointed at the intro and at nothing else. Every frame in parts.c
# and machine.c belongs to code a *level* reaches, so checking those needs
# `--restore`, which only devmain.c has.
#
#   make -C reconstruct asandev
#   TIM_HEADLESS=1 TIM_STOPFLIP=3000 ./reconstruct/devtimasan \
#       --restore solution_snaps/level14.solution --run
asandev: devmain.c sdl.c $(GAME) $(PORT) $(DEV) $(OPL_OBJ) $(HEADERS)
	$(CC) -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined \
	    -std=c11 -I$(CURDIR) $(SDL_CFLAGS) $(BACKTRACE) -o devtimasan \
	    $(filter %.c %.o,$^) $(SDL_LIBS) -lpthread -lstdc++

# `test` also enforces the **stdint-only** rule on the transcribed translation
# units. CLAUDE.md puts it plainly: this is 16-bit code where every value has a
# width the original depended on, and getting it wrong "compiles, runs, and
# silently loses the one fact the type carried". A rule whose whole danger is
# that nothing complains is a rule worth grepping for.
#
# It applies to `$(GAME)` only - the transcriptions. `io.c`, `sdl.c` and the
# rest of what is ours legitimately use host types: `ftell` answers a `long`
# and `tv_nsec` is one.
#
# And the **one display path** rule. CLAUDE.md: SDL3 for the window, input and
# sound, always - "never X11, never Win32, never SDL2, not behind an `#ifdef`".
# The pattern matches an *include* and not a bare word, because `X11` as a
# substring hits every `0x110` in the transcriptions and a check that cries wolf
# gets turned off.
#
# The developer flags, by name. The shipping binary must carry none of them:
# `devdump.c` links into `devtim` only, and `tools/` drives that. This has been
# false once already - `devdump.c` sat in the object list both binaries share,
# so `tim` had every flag and the comparisons were driving the game rather than
# the developer binary - and nothing caught it but someone reading the
# Makefile. So it is checked rather than asserted.
DEVFLAGS := TIM_CLICK TIM_POINTER TIM_FLIPS TIM_FLIPWANT TIM_FLIPHASH TIM_SURVEY_HOOKS TIM_GAMEDIR \
            TIM_FLIPCOUNT TIM_SAVEDIR TIM_PARTS TIM_WAV TIM_SFXDIR TIM_SFXALL TIM_FMDIR TIM_PARTPICS TIM_LEVELSCAN TIM_DATE \
            TIM_DEVICE TIM_MODULE TIM_LEVEL TIM_RUN TIM_SAVEMACHINE TIM_LOADMACHINE

# **The file list is the point, not an oversight.** These are the translation
# units where transcribed routines and ours-routines sit side by side, so a
# reader needs each *definition* to say which it is. `io.c`, `sdl.c`,
# `src/main.c`, `devmain.c`, `devdump.c` and `devstub.c` are ours end to end
# and say so in
# their headers; running the check over them would demand 114 per-function
# markers that repeat what the file already settles, which is noise and not
# provenance. Add a file here the moment it holds one transcribed routine.
# **`debug` is built here so it cannot rot again.** It and `asan` spell their
# own compiler flags rather than using $(CFLAGS), and both had been broken for
# long enough that neither had ever produced a binary: no `-I.`, so every
# source failed on `tim.h`, and no $(OPL_OBJ), so the link had no `opl_status`.
# Nothing noticed, because nothing built them. `debug` takes three seconds and
# fails the same two ways `asan` would; `asan` itself is 26 and is run by hand
# when there is a fault to chase - it found a 15-byte stack overflow the first
# time it ran.
# Strip C comments so a text check cannot match prose. `-z` makes sed
# treat each file as one record, which is what lets a /* */ span lines.
SED_STRIP_COMMENTS = sed -z -e 's|/\*[^\x00]*\?\*/||g' -e 's|//[^\n]*||g'

test: devtim tim debug
	uv run python tests/provenance.py $(GAME) borland_heap.c borland_file.c borland_huge.c
	@for f in $(DEVFLAGS); do \
	    if grep -aq "$$f" tim; then \
	        echo "FAIL: the shipping binary carries $$f"; exit 1; \
	    fi; \
	done; \
	echo "tim carries none of the $(words $(DEVFLAGS)) developer flags"
	@# **Strips comments first.** The pattern matches a line beginning with
	@# a bare type, and a *comment* line beginning "signed on the high word"
	@# is one of those - so writing down why a comparison is signed broke the
	@# build. That is the same shape as the `dg_alloca` ratchet that counted
	@# the word in prose, which CLAUDE.md records: a check that cannot tell
	@# code from comments punishes writing things down.
	@if $(SED_STRIP_COMMENTS) $(GAME) | grep -nE '^[[:space:]]*(static[[:space:]]+)?(const[[:space:]]+)?(unsigned|signed|short|long|int)[[:space:]]+[A-Za-z_*]'; then \
	    echo "FAIL: a bare C type in transcribed code - see CLAUDE.md"; \
	    exit 1; \
	fi; \
	echo "no bare C types in the transcribed translation units"
	@if grep -rnE '#[[:space:]]*include[[:space:]]*[<"](SDL2/|X11/|windows\.h|Xlib)' *.c *.h; then \
	    echo "FAIL: SDL3 for the window, input and sound. Always."; \
	    exit 1; \
	fi; \
	echo "one display path: SDL3 and nothing else included"
	@# **The verifier has to still run.** An edit meant to replace one helper
	@# in tools/verify.py sliced to the wrong boundary and deleted 490 lines
	@# with `main()` among them; the file still imported, `make test` still
	@# passed, and the damage was committed. It surfaced only when someone ran
	@# the tool. `--list` loads the module and walks the whole spec table
	@# without touching the emulator, in a tenth of a second.
	@cd .. && uv run python tools/verify.py --list > /dev/null || { \
	    echo "FAIL: tools/verify.py does not run"; exit 1; }
	@echo "the verifier loads and its spec table parses"
	@# **Every solution, simulated.** `TIM_SIMULATE` runs each restored
	@# machine through the game's own per-frame step with no clock, input
	@# or display, and the goal test is the verdict: 34 levels in about a
	@# second, the same frame counts as the real loop where the two were
	@# compared, and the same answer every run. The real `check_solutions`
	@# still stands - this does not exercise the loop's input or
	@# presentation - but a physics or part-record change that breaks a
	@# level now fails `make test` instead of an hour later.
	@cd .. && uv run python tools/check_solutions.py --simulate > /dev/null || { \
	    echo "FAIL: a solution no longer solves under simulation - run tools/check_solutions.py --simulate"; exit 1; }
	@echo "every solution snapshot solves under simulation"
	@# **A ratchet, not a threshold.** Every routine that still calls
	@# `dg_alloca` has been read and has a reason written beside it - the
	@# value it holds must be a 16-bit number in guest memory that something
	@# else reads back, and `tools/framify_census.py` names which for each.
	@# A *new* one is either a routine nobody has looked at yet or a
	@# conversion that went backwards, and both are worth a moment. Lower
	@# the number when one goes; raising it needs a reason in the commit.
	@# **The roll call, not a grep.** A first version counted `dg_alloca(`
	@# with grep and a ratchet, and the first comment to mention the name in
	@# prose broke the build - a check that cannot tell code from a comment
	@# is a check that punishes writing things down. `framify_census.py`
	@# parses, and `WALLED` names every routine that still reserves and why,
	@# so `--assert` fails both ways: a `dg_alloca` with no reason written for
	@# it, and a reason left behind by a routine that converted. Lowering the
	@# roll call is the normal direction; adding to it wants a reason.
	@cd .. && out=$$(uv run python tools/framify_census.py --assert) \
	    || { echo "$$out" | grep FAIL; exit 1; }; \
	echo "$$out" | tail -1

	@# And that no routine still holds a `frame[]`. `framify.py` made those
	@# out of the `dg_alloca` reservations and `promote.py` turned each slot
	@# into the local it is; a frame array coming back would mean a routine
	@# whose locals share one buffer again, where a write through one slot
	@# can run into the next.
	@cd .. && out=$$(uv run python tools/promote.py --assert) \
	    || { echo "$$out"; exit 1; }; \
	echo "$$out" | tail -1

# Everything the Makefile builds, not just the four binaries. `libtim.so`,
# `covtim` and gcov's notes were missed here for the same reason they were
# missed in .gitignore: the list was written when there were only binaries.
clean:
	rm -f tim devtim timdbg timasan covtim libtim.so *.gcno *.gcda \
	      $(OPL_OBJ) $(OPL_PIC) $(TIM_OBJ) $(DEV_OBJ)

.PHONY: all clean test debug asan compile_commands

# A shared library so tools/verify.py can call a transcribed routine directly
# and compare what it does to the hardware against what the original did.
# `covtim` is devtim with gcov instrumentation, for asking which of the port's
# own routines a run actually enters. tools/coverage.py answers that question
# for the original by watching blocks; this is the other half, and the two
# together say whether the two artefacts reach the same screen along the same
# path or merely arrive at the same pixels.
covtim: devmain.c $(GAME) $(PORT) $(DEV) $(HEADERS)
	$(CC) -O0 -g --coverage -std=c11 -o $@ $^ -lpthread

libtim.so: $(GAME) $(PORT) $(DEV) $(HEADERS) $(OPL_PIC)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ \
	    $(filter-out $(HEADERS),$^) -lstdc++
