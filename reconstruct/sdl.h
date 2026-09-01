/*
 * The Incredible Machine - reconstruction. The window's interface.
 *
 * NOT a transcription: this is the port's own backend. See sdl.c.
 */
#ifndef SDL_BACKEND_H
#define SDL_BACKEND_H

#include <stdint.h>

/*
 * A developer hotkey, for a caller that wants one. The port's own `main` does
 * not register anything, so `tim` has none of this; the hybrid runner uses it
 * to snapshot the machine. Kept here rather than in the runner because this is
 * the one place in the program that looks at a key at all.
 */
#define SDL_HOTKEY_SNAPSHOT 1       /* Shift+F2 */

void    sdl_on_hotkey(void (*fn)(int32_t id));

int32_t sdl_open(void);
void    sdl_present(void);
void    sdl_pump(void);
void    sdl_die(void);
void    sdl_hold(void);

#endif /* SDL_BACKEND_H */
