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

CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS  += -I$(VENV)/include -I$(REC)
LDLIBS  := $(VENV)/lib/libunicorn.a -lpthread -lm

# The port, exactly as reconstruct/Makefile lists it, plus its do-nothing
# developer hooks. No SDL: this runs headless for now and the frame is the
# port's plane model, which tools/ already know how to read.
GAME    := seg0000.c seg0dff.c seg14de.c seg172c.c seg1c25.c seg248f.c \
           seg2619.c seg2a04.c dgroup.c vmovl_vga.c sxovl_spkr.c
PORT    := io.c borland_heap.c borland_file.c borland_huge.c
NODEV   := devstub.c
PORTSRC := $(addprefix $(REC)/,$(GAME) $(PORT) $(NODEV))
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
	$(CC) $(CFLAGS) -o $@ $(SRC) $(PORTSRC) $(LDLIBS)

clean:
	rm -f native syms.c ovlsyms.c shims.c

.PHONY: all clean
