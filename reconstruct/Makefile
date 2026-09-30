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
# The headers stay beside hostio.c; the game's own sources are a directory down.
# **A comparison that cannot come out true is a transcription gone wrong.**
# Source written for Turbo C++ compares a 16-bit value with 0x8000 as
# unsigned; on the host `int16_t == 0x8000` is always false and compiles
# anyway (docs/lessons.md).
CFLAGS  += -Werror=type-limits
CFLAGS  += -I.
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)

# The game itself, under src/: the eight modules transcribed from the
# original's code segments, the DGROUP array they all reach through, and the
# two overlays. `hostio.c` and `sdl.c` are not the game - they are the
# hardware and the window - nor is `hostlib.c`, the few routines of Borland's
# library that libc has no counterpart for; the dev*.c files are not shipped
# at all. Borland's library itself, transcribed, is kept in borland/ and not
# built: the host calls libc.
SRC  := src
GAME := $(SRC)/collide.c $(SRC)/stepmach.c $(SRC)/runloop.c $(SRC)/goals.c $(SRC)/score.c $(SRC)/physics.c $(SRC)/links.c $(SRC)/machine.c $(SRC)/frame.c $(SRC)/regions.c $(SRC)/crtc.c $(SRC)/gamefile.c $(SRC)/xorrect.c $(SRC)/fstring.c $(SRC)/rects.c $(SRC)/cursor.c $(SRC)/dos.c $(SRC)/mono.c $(SRC)/gamemain.c $(SRC)/rescfg.c $(SRC)/gamedata.c $(SRC)/messages.c $(SRC)/intro.c $(SRC)/round.c $(SRC)/puzzles.c $(SRC)/screen.c $(SRC)/binadjust.c $(SRC)/levels.c $(SRC)/picker.c $(SRC)/game.c $(SRC)/machine_draw.c $(SRC)/kindmem.c $(SRC)/parts/ball.c $(SRC)/parts/balloon.c $(SRC)/parts/bellow.c $(SRC)/parts/boxing_glove.c $(SRC)/parts/bucket.c $(SRC)/parts/bullet.c $(SRC)/parts/candle.c $(SRC)/parts/cannon.c $(SRC)/parts/pokey.c $(SRC)/parts/bird_cage.c $(SRC)/parts/christmas_tree.c $(SRC)/parts/kinds_55_57.c $(SRC)/parts/dynamite.c $(SRC)/parts/motor.c $(SRC)/parts/electric_plug.c $(SRC)/parts/blast.c $(SRC)/parts/hook.c $(SRC)/parts/fan.c $(SRC)/parts/bob_the_fish.c $(SRC)/parts/flashlight.c $(SRC)/parts/generator.c $(SRC)/parts/gear.c $(SRC)/parts/gun.c $(SRC)/parts/conveyor.c $(SRC)/parts/heart_balloon.c $(SRC)/parts/ramp.c $(SRC)/parts/jack_in_the_box.c $(SRC)/parts/light.c $(SRC)/parts/monkey.c $(SRC)/parts/mouse_cage.c $(SRC)/parts/magnifying_glass.c $(SRC)/parts/dynamite_plunger.c $(SRC)/parts/mort_the_mouse.c $(SRC)/parts/pumpkin.c $(SRC)/parts/rocket.c $(SRC)/parts/corner_pipe.c $(SRC)/parts/scissors.c $(SRC)/parts/solar_panel.c $(SRC)/parts/trampoline.c $(SRC)/parts/seesaw.c $(SRC)/parts/trigger_things.c $(SRC)/parts/platform.c $(SRC)/parts/windmill.c $(SRC)/parts/kind_61.c $(SRC)/parts/kind_64.c $(SRC)/parts/kind_52.c $(SRC)/parts/kind_54.c $(SRC)/parts/kind_58.c $(SRC)/parts/kind_53.c $(SRC)/parts/kind_65.c $(SRC)/parts/kind_62.c $(SRC)/parts/kind_51.c $(SRC)/screenshot.c $(SRC)/vgadac.c $(SRC)/resource.c $(SRC)/resfile.c \
        $(SRC)/lzw.c $(SRC)/lzhuf.c $(SRC)/vmiface.c $(SRC)/palette.c $(SRC)/polygon.c $(SRC)/compbmp.c $(SRC)/timer.c $(SRC)/scale.c $(SRC)/polyclip.c $(SRC)/keyboard.c $(SRC)/text.c $(SRC)/dosmem.c $(SRC)/lowlevel.c $(SRC)/vidinit.c $(SRC)/vidload.c $(SRC)/fontload.c $(SRC)/bmpload.c $(SRC)/files.c $(SRC)/vqtflip.c $(SRC)/offbmp.c $(SRC)/bitmaps.c $(SRC)/sound/read_record.c $(SRC)/sound/start_sound.c $(SRC)/sound/next_matching_record.c $(SRC)/sound/free_voice_records.c $(SRC)/sound/create_sequence.c $(SRC)/sound/load_sound_bank.c $(SRC)/sound/alloc_voice_records.c $(SRC)/sound/voice_playing.c $(SRC)/sound/load_named_chunk.c $(SRC)/sound.c $(SRC)/sound/set_master_level_ok.c $(SRC)/sound/load_sound_module.c $(SRC)/sound/open_sound_file.c $(SRC)/sound/remove_and_free_records.c $(SRC)/sound/shutdown_sound.c $(SRC)/sound/stop_all_voices.c $(SRC)/sound/setup_sound_device.c $(SRC)/sound/sound_data.c $(SRC)/sound/start_sequence_by_id.c $(SRC)/sound/stop_sound.c $(SRC)/sound/stop_sequences.c $(SRC)/sound/advance_record.c $(SRC)/sound/follow_then_tick.c $(SRC)/sound/follow_far_chain.c $(SRC)/sound/load_and_start_sequence.c $(SRC)/sound/stop_voice_playing.c $(SRC)/sound/start_on_free_voice.c $(SRC)/sound/load_resource_block.c $(SRC)/sound_call.c $(SRC)/trig.c $(SRC)/atan2.c $(SRC)/vqt.c \
        $(SRC)/dgroup.c $(SRC)/vmovl_vga.c $(SRC)/sxovl_spkr.c \
        $(SRC)/sxovl_asb.c $(SRC)/sxovl_adl.c $(SRC)/sxovl_sbp.c \
        $(SRC)/sxovl.c $(SRC)/thunks.c $(SRC)/glue.c
PORT := hostio.c hostlib.c

# The OPL2 and the ymfm units it needs. ymfm_opl.cpp pulls in the other three
# through its templates even though a YM3812 has no ADPCM, PCM or SSG in it.
OPL_SRC := $(SRC)/opl_ymfm.cpp \
           vendor/ymfm/ymfm_opl.cpp vendor/ymfm/ymfm_adpcm.cpp \
           vendor/ymfm/ymfm_pcm.cpp vendor/ymfm/ymfm_ssg.cpp
OPL_OBJ := $(OPL_SRC:.cpp=.o)
OPL_PIC := $(OPL_SRC:.cpp=.pic.o)

# The developer hooks, and the shipping binary's do-nothing stand-in for them.
# `hostio.c` calls dev_flip_dump on every page flip whatever it is linked into, so
# one of these two is always present - and only `devtim` gets the one that can
# be asked to write frames or press a mouse button.
# `devwav.c` is separate from both: `devtim` and the hybrid runner each link it
# and the shipping binary links neither, which is how a recording of the port
# and a recording of the original can be made with the same writer.
DEV  := devdump.c devwav.c
NODEV := devstub.c

# **`devlua.c` is not in `DEV`, and that is the point.** `DEV` is linked into
# `libtim.so` as well - the library `tools/verify.py` calls - and a verification
# run has no business opening a socket or carrying a Lua interpreter. It goes
# into `devtim` alone, and `dev_flip_dump` reaches it through a weak symbol, so
# every other binary links with the hook absent and does nothing.
DEVLUA     := devlua.c
LUA_CFLAGS := $(shell pkg-config --cflags lua 2>/dev/null)
LUA_LIBS   := $(shell pkg-config --libs lua 2>/dev/null)

# **Headers count as sources.** Rules that list only the `.c` files mean a
# change to `hostio.h` alone rebuilds nothing and the next run uses the old build -
# silently, and answering with complete confidence. Raising DOS_ALLOC_PRIMED
# from 16 to 512 appeared to change nothing for an hour because of this, and a
# whole false finding was written up from the stale result.
#
# **And it has to be defined before the rules that use it.** It sat below them
# until 2026-09-05, and make expands a prerequisite list when it reads the rule,
# so `$(HEADERS)` there expanded to nothing and the dependency never existed.
# Measured at the time: `touch hostio.h && make` compiled zero files. The fix that
# was written up as done had never taken effect.
HEADERS := $(wildcard *.h)

# The objects. Compiling each unit once and linking twice is what makes an
# edit to one file rebuild one file - and lets `make -j` use the other cores.
# `tim` and `devtim` compile with identical flags, so they share these.
COMMON_OBJ := $(patsubst %.c,%.o,sdl.c $(GAME) $(PORT))
TIM_OBJ    := $(SRC)/main.o $(COMMON_OBJ) $(patsubst %.c,%.o,$(NODEV))
DEV_OBJ    := devmain.o     $(COMMON_OBJ) $(patsubst %.c,%.o,$(DEV) $(DEVLUA))

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
	done;
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
# so it can be played. The tools in tools/ set
# TIM_HEADLESS=1, which is what keeps them working on a machine with no
# display: the window is what a person needs, its absence is what a comparison
# needs, and the comparison is the one that asks.
# `--wrap=fopen`: the developer build's RESOURCE.CFG override, in devdump.c.
devtim: $(DEV_OBJ) $(OPL_OBJ)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(BACKTRACE) -Wl,--wrap=fopen -o $@ \
	    $^ $(SDL_LIBS) $(LUA_LIBS) -lpthread -lstdc++

# The one unit that needs Lua's headers, and the only one that gets them.
devlua.o: devlua.c $(HEADERS)
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -c -o $@ $<

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
# it can be pointed at the intro and at nothing else. Every frame in the
# parts/*.c modules and machine.c belongs to code a *level* reaches, so checking those needs
# `--level` and `TIM_LOADMACHINE`, which only devmain.c has.
#
#   make -C reconstruct asandev
#   TIM_HEADLESS=1 TIM_STOPFLIP=3000 TIM_LOADMACHINE=S14.TIM \
#       TIM_GAMEDIR=<a game directory with the solutions in it> \
#       ./reconstruct/devtimasan --level 14 --run
asandev: devmain.c sdl.c $(GAME) $(PORT) $(DEV) $(OPL_OBJ) $(HEADERS)
	$(CC) -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined \
	    -std=c11 -I$(CURDIR) -Wl,--wrap=fopen $(SDL_CFLAGS) $(BACKTRACE) -o devtimasan \
	    $(filter %.c %.o,$^) $(SDL_LIBS) -lpthread -lstdc++

# `test` also enforces the **stdint-only** rule on the transcribed translation
# units. CLAUDE.md puts it plainly: this is 16-bit code where every value has a
# width the original depended on, and getting it wrong "compiles, runs, and
# silently loses the one fact the type carried". A rule whose whole danger is
# that nothing complains is a rule worth grepping for.
#
# It applies to `$(GAME)` only - the transcriptions. `hostio.c`, `sdl.c` and the
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
            TIM_DEVICE TIM_MODULE TIM_LEVEL TIM_RUN TIM_LOADMACHINE TIM_SIMULATE TIM_LUA

# **The file list is the point, not an oversight.** These are the translation
# units where transcribed routines and ours-routines sit side by side, so a
# reader needs each *definition* to say which it is. `hostio.c`, `sdl.c`,
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
	uv run python tests/provenance.py $(GAME)
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
	@# **Every solution, simulated.** Each `solutions/S<NN>.TIM` is loaded
	@# over its own puzzle the way the game loads a machine, and
	@# `TIM_SIMULATE` then runs it through the game's own per-frame step
	@# with no clock, input or display, the goal test being the verdict:
	@# 29 levels in a few seconds, the same frame counts as the real loop
	@# where the two were compared, and the same answer every run. The real
	@# `check_solutions` still stands - this does not exercise the loop's
	@# input or presentation - but a physics or part-record change that
	@# breaks a level now fails `make test` instead of an hour later.
	@cd .. && uv run python tools/check_solutions.py --simulate > /dev/null || { \
	    echo "FAIL: a solution no longer solves under simulation - run tools/check_solutions.py --simulate"; exit 1; }
	@echo "every solution solves under simulation"

# Everything the Makefile builds, not just the four binaries. `libtim.so`,
# `covtim` and gcov's notes were missed here for the same reason they were
# missed in .gitignore: the list was written when there were only binaries.
clean:
	rm -f tim devtim timdbg timasan covtim libtim.so  *.gcno *.gcda \
	      $(OPL_OBJ) $(OPL_PIC) $(TIM_OBJ) $(DEV_OBJ)

.PHONY: all clean test debug asan compile_commands

# A shared library, for the tools that read the port's objects by address.
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
