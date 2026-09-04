# The Incredible Machine - reconstruction.
#
# Two binaries, deliberately: `tim` is the game and mirrors the original's
# start-up, `devtim` carries every developer flag a comparison needs. tools/
# calls devtim, so nothing a measurement depends on can drift into what ships.

CC      ?= cc
CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Wno-unused-parameter

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
        $(SRC)/sxovl_asb.c $(SRC)/sxovl_adl.c \
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

all: tim devtim

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

tim: $(SRC)/main.c sdl.c $(GAME) $(PORT) $(NODEV) $(HEADERS) $(OPL_OBJ)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(BACKTRACE) -o $@ \
	    $(filter-out $(HEADERS),$^) $(SDL_LIBS) -lpthread -lstdc++

# devtim links sdl.c too, and opens a window by default exactly as tim does,
# so it can be played and Shift+F2 pressed. The tools in tools/ set
# TIM_HEADLESS=1, which is what keeps them working on a machine with no
# display: the window is what a person needs, its absence is what a comparison
# needs, and the comparison is the one that asks.
devtim: devmain.c sdl.c $(GAME) $(PORT) $(DEV) $(HEADERS) $(OPL_OBJ)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(BACKTRACE) -o $@ \
	    $(filter-out $(HEADERS),$^) $(SDL_LIBS) -lpthread -lstdc++

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
debug: $(SRC)/main.c sdl.c $(GAME) $(PORT) $(NODEV) $(HEADERS)
	$(CC) -O0 -g3 -fno-omit-frame-pointer -std=c11 $(SDL_CFLAGS) 	    $(BACKTRACE) -o timdbg $^ $(SDL_LIBS) -lpthread

asan: $(SRC)/main.c sdl.c $(GAME) $(PORT) $(NODEV) $(HEADERS)
	$(CC) -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined 	    -std=c11 $(SDL_CFLAGS) $(BACKTRACE) -o timasan $^ $(SDL_LIBS) -lpthread

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
            TIM_FLIPCOUNT TIM_SAVEDIR TIM_PARTS TIM_WAV TIM_SFXDIR TIM_SFXALL TIM_FMDIR

# **The file list is the point, not an oversight.** These are the translation
# units where transcribed routines and ours-routines sit side by side, so a
# reader needs each *definition* to say which it is. `io.c`, `sdl.c`,
# `src/main.c`, `devmain.c`, `devdump.c` and `devstub.c` are ours end to end
# and say so in
# their headers; running the check over them would demand 114 per-function
# markers that repeat what the file already settles, which is noise and not
# provenance. Add a file here the moment it holds one transcribed routine.
test: devtim tim
	uv run python tests/provenance.py $(GAME) borland_heap.c borland_file.c borland_huge.c
	@for f in $(DEVFLAGS); do \
	    if grep -aq "$$f" tim; then \
	        echo "FAIL: the shipping binary carries $$f"; exit 1; \
	    fi; \
	done; \
	echo "tim carries none of the $(words $(DEVFLAGS)) developer flags"
	@if grep -nE '^[[:space:]]*(static[[:space:]]+)?(const[[:space:]]+)?(unsigned|signed|short|long|int)[[:space:]]+[A-Za-z_*]' $(GAME); then \
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

# Everything the Makefile builds, not just the four binaries. `libtim.so`,
# `covtim` and gcov's notes were missed here for the same reason they were
# missed in .gitignore: the list was written when there were only binaries.
clean:
	rm -f tim devtim timdbg timasan covtim libtim.so *.gcno *.gcda \
	      $(OPL_OBJ) $(OPL_PIC)

.PHONY: all clean test debug asan

# A shared library so tools/verify.py can call a transcribed routine directly
# and compare what it does to the hardware against what the original did.
# `covtim` is devtim with gcov instrumentation, for asking which of the port's
# own routines a run actually enters. tools/coverage.py answers that question
# for the original by watching blocks; this is the other half, and the two
# together say whether the two artefacts reach the same screen along the same
# path or merely arrive at the same pixels.
# **Headers count as sources.** These rules listed only the `.c` files, so a
# change to `io.h` alone rebuilt nothing and the next run used the old library -
# silently, and answering with complete confidence. Raising DOS_ALLOC_PRIMED
# from 16 to 512 appeared to change nothing for an hour because of this, and a
# whole false finding was written up from the stale result.
HEADERS := $(wildcard *.h)

covtim: devmain.c $(GAME) $(PORT) $(DEV) $(HEADERS)
	$(CC) -O0 -g --coverage -std=c11 -o $@ $^ -lpthread

libtim.so: $(GAME) $(PORT) $(DEV) $(HEADERS) $(OPL_PIC)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ \
	    $(filter-out $(HEADERS),$^) -lstdc++
