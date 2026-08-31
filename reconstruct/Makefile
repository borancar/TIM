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

test: devtim
	uv run python tests/provenance.py $(GAME) borland_heap.c borland_file.c borland_huge.c

clean:
	rm -f tim devtim timdbg timasan

.PHONY: all clean test debug asan

# A shared library so tools/verify.py can call a transcribed routine directly
# and compare what it does to the hardware against what the original did.
libtim.so: $(GAME) $(PORT) $(DEV)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^
