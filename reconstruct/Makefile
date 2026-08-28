# The Incredible Machine - reconstruction.
#
# Two binaries, deliberately: `tim` is the game and mirrors the original's
# start-up, `devtim` carries every developer flag a comparison needs. tools/
# calls devtim, so nothing a measurement depends on can drift into what ships.

CC      ?= cc
CFLAGS  ?= -O2 -g -std=c11 -Wall -Wextra -Wno-unused-parameter
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)

GAME := seg0000.c dgroup.c
PORT := io.c

all: tim devtim

tim: main.c $(GAME) $(PORT)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ $(SDL_LIBS)

devtim: devmain.c $(GAME) $(PORT)
	$(CC) $(CFLAGS) -o $@ $^

test: devtim
	uv run python tests/provenance.py $(GAME)

clean:
	rm -f tim devtim

.PHONY: all clean test
