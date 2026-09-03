# The hybrid runner: the original binary under emulation, with the port as its
# hardware and, routine by routine, as its code. See native.h.
#
# It links the port's own translation units rather than a copy of them, so what
# runs here is exactly what `reconstruct/` builds - a fix made for the port is
# a fix here, and a difference between the two cannot creep in.
#
# Unicorn comes from the virtualenv the tools already use, because that is the
# build the emulator is pinned to in pyproject.toml. Taking it from the system
# instead would mean the C runner and tools/verify.py could disagree about what
# the machine does, which is the one thing this project cannot afford.

CC      ?= cc
ROOT    := ../..
REC     := $(ROOT)/reconstruct
VENV    := $(ROOT)/.venv/lib64/python3.14/site-packages/unicorn

SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)

CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS  += -I$(VENV)/include -I$(REC) $(SDL_CFLAGS)
LDLIBS  := $(VENV)/lib/libunicorn.a $(SDL_LIBS) -lpthread -lm

# The port, exactly as reconstruct/Makefile lists it, plus its do-nothing
# developer hooks.
#
# **`sdl.c` is one of them**, because the hybrid shows a window like `tim`
# does. It did not always: this ran headless and composed frames for tools/ to
# read, which answers "do the two sides draw the same pixels" and cannot answer
# "does the game play". The rule that settles which is CLAUDE.md's - one
# display path, not two - so the window here is the port's own `sdl.c` and the
# frame writer stays a mode of the same composed frame rather than a second
# implementation. `TIM_HEADLESS=1` puts it back to no window, which is what
# tools/check_native.py runs it as.
# The game's own modules live under reconstruct/src now; io.c, sdl.c and the
# Borland runtime are not the game and stayed beside the headers.
GAME    := machine.c game.c machine_draw.c parts.c engine.c bitmaps.c \
           sound.c trig.c dgroup.c vmovl_vga.c sxovl_spkr.c
PORT    := io.c sdl.c borland_heap.c borland_file.c borland_huge.c
NODEV   := devstub.c
PORTSRC := $(addprefix $(REC)/src/,$(GAME)) \
           $(addprefix $(REC)/,$(PORT) $(NODEV))
HEADERS := $(wildcard $(REC)/*.h) native.h

SRC     := native.c dispatch.c shim.c shims.c guestbt.c syms.c ovlsyms.c sha256.c

all: native

# syms.c is generated from the port's own provenance comments, so a routine
# transcribed today is named in tomorrow's backtraces without anyone
# maintaining a list. It depends on the transcriptions, not just on its script.
# shims.c is generated from routines.def, which is the only hand-edited list.
shims.c: genshims.py routines.def $(REC)/tim.h
	cd $(ROOT) && uv run python tools/native/genshims.py

syms.c ovlsyms.c: gensyms.py $(REC)/tests/provenance.py $(PORTSRC)
	cd $(ROOT) && uv run python tools/native/gensyms.py

native: $(SRC) $(PORTSRC) $(HEADERS) shim.h
	$(CC) $(CFLAGS) -rdynamic -o $@ $(SRC) $(PORTSRC) $(LDLIBS)

clean:
	rm -f native syms.c ovlsyms.c shims.c

.PHONY: all clean
