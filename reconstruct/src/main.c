/*
 * The Incredible Machine - reconstruction. The program's entry point.
 *
 * NOT a transcription: this is the port's own start-up. It is deliberately
 * almost empty, because a DOS game has no command line - it starts, shows its
 * screens, and plays. Every developer flag lives in devmain.c and links into a
 * separate binary, so nothing a comparison depends on can become part of what
 * ships.
 *
 * What is here is what DOS did before the program's first instruction, less
 * the loading: give the program a stack and an arena. The image's data is
 * already in `guest_mem`, placed there by the linker (see `DGROUP_AT` in
 * dgroup.h), so the game needs no file but its own. Then `game_main`, which is
 * the game's own `main` at image 0x0dfff.
 */
#include <stdlib.h>
#include <string.h>

#include "dgroup.h"
#include "io.h"
#include "sdl.h"
#include "tim.h"

/*
 * OURS: **Shift+F2 writes the whole machine out**, so a state reached by
 * playing can be handed to a tool. `TIM_SNAP=<path>` moves it; the default
 * sits beside the abort dump, which is the same idea taken at the same moment
 * every time rather than at a chosen one.
 */
static void on_hotkey(int32_t id)
{
    char path[512];

    if (id != SDL_HOTKEY_SNAPSHOT)
        return;
    io_next_snapshot_path(path, sizeof path, "tim");
    io_write_snapshot(path);
}

/*
 * OURS: a DOS game has no `main`: it is entered at `0000:0000` and the Borland
 * startup gets it to `game_main`. This is the host's way in, and everything a
 * developer might want to pass it lives in devmain.c instead.
 */
int main(void)
{
    io_reset();
    io_start_program();

    /*
     * What the C runtime does between the loader and `main`: its init table at
     * DGROUP 0x4e48 has a single entry, and it is `setup_streams`. Without it
     * every stream looks already-open and no file can be read.
     */
    setup_streams();

    /*
     * The window, and the guest's own cue to put a frame in it. Registering it
     * here rather than inside io.c is what keeps devtim free of SDL.
     */
    if (!sdl_open())
        return 1;
    io_on_present(sdl_present);
    io_on_abort(sdl_hold);
    sdl_on_hotkey(on_hotkey);

    /*
     * And the guest's clock. The original gets its ticks from the 8253 through
     * vector 8; there are no interrupts here, so the port runs the same handler
     * from real time - see io_service_timer for where it is called from.
     */
    io_set_timer(timer_tick);

    game_main();

    /* The game does not return; if it ever does, leave the last frame up. */
    sdl_hold();
    return 0;
}
