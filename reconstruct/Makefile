# The Incredible Machine - reconstruction.
#
# Two binaries, deliberately: `tim` is the game and mirrors the original's
# start-up, `devtim` carries every developer flag a comparison needs. tools/
# calls devtim, so nothing a measurement depends on can drift into what ships.

CC      ?= cc
CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Wno-unused-parameter
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)

GAME := seg0000.c seg0dff.c seg14de.c seg172c.c seg1c25.c seg248f.c seg2619.c seg2a04.c dgroup.c vmovl_vga.c sxovl_spkr.c
PORT := io.c borland_heap.c borland_file.c borland_huge.c

# The developer hooks, and the shipping binary's do-nothing stand-in for them.
# `io.c` calls dev_flip_dump on every page flip whatever it is linked into, so
# one of these two is always present - and only `devtim` gets the one that can
# be asked to write frames or press a mouse button.
DEV  := devdump.c
NODEV := devstub.c

all: tim devtim

tim: main.c sdl.c $(GAME) $(PORT) $(NODEV)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ $(SDL_LIBS) -lpthread

devtim: devmain.c $(GAME) $(PORT) $(DEV)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

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
debug: main.c sdl.c $(GAME) $(PORT) $(NODEV)
	$(CC) -O0 -g3 -fno-omit-frame-pointer -std=c11 $(SDL_CFLAGS) 	    -o timdbg $^ $(SDL_LIBS) -lpthread

asan: main.c sdl.c $(GAME) $(PORT) $(NODEV)
	$(CC) -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined 	    -std=c11 $(SDL_CFLAGS) -o timasan $^ $(SDL_LIBS) -lpthread

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
DEVFLAGS := TIM_CLICK TIM_POINTER TIM_FLIPS TIM_FLIPWANT TIM_FLIPHASH \
            TIM_FLIPCOUNT TIM_SAVEDIR TIM_PARTS

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

# Everything the Makefile builds, not just the four binaries. `libtim.so`,
# `covtim` and gcov's notes were missed here for the same reason they were
# missed in .gitignore: the list was written when there were only binaries.
clean:
	rm -f tim devtim timdbg timasan covtim libtim.so *.gcno *.gcda

.PHONY: all clean test debug asan

# A shared library so tools/verify.py can call a transcribed routine directly
# and compare what it does to the hardware against what the original did.
# `covtim` is devtim with gcov instrumentation, for asking which of the port's
# own routines a run actually enters. tools/coverage.py answers that question
# for the original by watching blocks; this is the other half, and the two
# together say whether the two artefacts reach the same screen along the same
# path or merely arrive at the same pixels.
covtim: devmain.c $(GAME) $(PORT) $(DEV)
	$(CC) -O0 -g --coverage -std=c11 -o $@ $^ -lpthread

libtim.so: $(GAME) $(PORT) $(DEV)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^
