/*
 * The Incredible Machine - reconstruction. The window's interface.
 *
 * NOT a transcription: this is the port's own backend. See sdl.c.
 */
#ifndef SDL_BACKEND_H
#define SDL_BACKEND_H

#include <stdint.h>

int32_t sdl_open(void);
void    sdl_present(void);
void    sdl_pump(void);
void    sdl_die(void);
void    sdl_hold(void);

#endif /* SDL_BACKEND_H */
