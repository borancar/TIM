/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Far faces of the runtime's routines**: nineteen one-line wrappers, each
 * taking its arguments off the stack and handing them to `open`, `read`,
 * `fopen`, `malloc`, `strcpy` and the rest. Twelve of them - everything
 * before 0x0bb1e but none after - are called by nothing: the game calls the
 * library directly, and these were linked in with the ones it uses.
 *
 * One module of the original's **code segment 0000** (`_TEXT`), image
 * 0x0ba32..0x0bb97, ending in the pad byte that puts glue.c's assembly on a
 * word boundary. **The front is ours**: nothing here has data or calls back
 * bare. Its cleanup is `inc sp / inc sp` and `add sp,4` where cursor.c's is
 * `pop cx`, which is `-G`; BC++ 3.0 with the same options matches 11 of
 * the 19, BC++ 2.0 all of them.
 *
 * **These are the game's, not the toolchain's.** TLINK writes no routines -
 * it only turns a far call within the segment into `nop / push cs / call` -
 * and Borland's medium-model library is far already, so it has no far faces
 * to add: the game calls `fopen` at 0x0d0ce directly eight times. Each of
 * these has a compiled frame, and they sit in the game's part of `_TEXT`,
 * before the library begins at 0x0bbfe.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -zC_TEXT -G -O
 */
#include <stdlib.h>
#include <string.h>
#ifdef __TURBOC__
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x0ba32
 *
 * `open` with two arguments - Borland's is variadic, and this pushes the
 * name and the flags and no permission word.
 */
int16_t open_far(const char *name, uint16_t flags)
{
#ifdef __TURBOC__
    return open(name, flags);
#else
    /* Ours: the host's `open` has a fixed third argument, which the
       original's reads only when it creates. */
    return open(name, flags, 0);
#endif
}

/*
 * 0x0ba45
 *
 * `open` with three arguments.
 */
int16_t open_file_perm_far(const char *name, uint16_t flags, uint16_t perm)
{
    return open(name, flags, perm);
}

/*
 * 0x0ba5b
 */
int16_t close_far(int16_t handle)
{
    return close(handle);
}

/*
 * 0x0ba6a
 */
int16_t readfd_far(int16_t handle, uint8_t *buf, uint16_t count)
{
    return read(handle, buf, count);
}

/*
 * 0x0ba80
 */
FILE *fopen_far(const char *name, const char *mode)
{
    return fopen(name, mode);
}

/*
 * 0x0ba93
 */
int16_t fseek_far(FILE *file, int32_t off, int16_t whence)
{
    return fseek(file, off, whence);
}

/*
 * 0x0baac
 */
int32_t ftell_far(FILE *file)
{
    return ftell(file);
}

/*
 * 0x0babb
 */
uint16_t fread_far(uint8_t *buf, uint16_t size, uint16_t count,
                           FILE *file)
{
    return fread(buf, size, count, file);
}

/*
 * 0x0bad4
 */
uint16_t fwrite_far(const uint8_t *buf, uint16_t size, uint16_t count,
                            FILE *file)
{
    return fwrite(buf, size, count, file);
}

/*
 * 0x0baed
 */
int16_t fputc_far(int16_t c, FILE *file)
{
    return fputc(c, file);
}

/*
 * 0x0bb00
 */
void rewind_far(FILE *file)
{
    rewind(file);
}

/*
 * 0x0bb0f
 */
int16_t fclose_far(FILE *file)
{
    return fclose(file);
}

/*
 * 0x0bb1e
 *
 * The far-callable face of `malloc`: one argument off the stack and straight
 * on to `malloc`.
 */
void *malloc_far(uint16_t bytes)
{
    /*
     * **The `far` is the call, not the pointer.** 0x0bb1e is a thunk - one
     * word pushed, `push cs`, a near call to `malloc`, `retf` - and
     * `malloc` ends `mov ax,bx / retf` with nothing in DX. So a near
     * heap block is one 16-bit DGROUP offset, which `malloc` answers as
     * the pointer it is, NULL for the original's 0.
     */
    return malloc(bytes);
}


/*
 * 0x0bb2d
 *
 * The far-callable face of `free`: one argument off the stack and straight on
 * to `free`. The `inc sp` twice that cleans it is two bytes shorter than
 * an `add sp,2` and does the same.
 */
void free_far(void *p)
{
    free(p);
}

/*
 * 0x0bb3c
 *
 * The far-callable face of `strcat`, the same shape as its neighbours: two
 * words off the stack and straight on to `strcat`. **Nothing calls
 * it** - no `lcall` and no near call anywhere in the image - so it was linked
 * in with the rest of this module and never used. The same is true of the
 * `strchr` and `fgetc` faces below; the other four are called from 8 to 56
 * sites each.
 */
char *strcat_far(char *dst, const char *src)
{
    return strcat(dst, src);
}

/*
 * 0x0bb4f
 *
 * The far-callable face of `strcpy`: it takes the two words off the stack and
 * hands them straight on.
 */
char *strcpy_far(char *dst, const char *src)
{
    return strcpy(dst, src);
}

/*
 * 0x0bb62
 *
 * The far-callable face of `strchr`: the string and the character, the
 * latter pushed as a word, on to `strchr`. Uncalled - see 0x0bb3c.
 */
char *strchr_far(char *s, int16_t c)
{
    return strchr(s, c);
}

/*
 * 0x0bb75
 *
 * The far-callable face of `calloc`: it takes the two words off the stack and
 * hands them straight on. Four instructions and a `retf`.
 */
void *calloc_far(uint16_t count, uint16_t size)
{
    return calloc(count, size);
}

/*
 * 0x0bb88
 *
 * The far-callable face of `fgetc`: one word, the stream, on to
 * `fgetc`. Uncalled - see 0x0bb3c.
 */
int16_t fgetc_far(FILE *file)
{
    return fgetc(file);
}


