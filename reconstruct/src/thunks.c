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
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x0ba32
 *
 * `open` with two arguments - Borland's is variadic, and this pushes the
 * name and the flags and no permission word.
 */
int16_t open_file_far(const char *name, uint16_t flags)
{
#ifdef __TURBOC__
    return open_file(name, flags);
#else
    /* Ours: the host's `open_file` has a fixed third argument, which the
       original's reads only when it creates. */
    return open_file(name, flags, 0);
#endif
}

/*
 * 0x0ba45
 *
 * `open` with three arguments.
 */
int16_t open_file_perm_far(const char *name, uint16_t flags, uint16_t perm)
{
    return open_file(name, flags, perm);
}

/*
 * 0x0ba5b
 */
int16_t close_handle_far(int16_t handle)
{
    return close_handle(handle);
}

/*
 * 0x0ba6a
 */
int16_t borland_read_far(int16_t handle, uint8_t *buf, uint16_t count)
{
    return borland_read(handle, buf, count);
}

/*
 * 0x0ba80
 */
struct file_rec *borland_fopen_far(const char *name, const char *mode)
{
    return borland_fopen(name, mode);
}

/*
 * 0x0ba93
 */
int16_t borland_fseek_far(struct file_rec *file, int32_t off, int16_t whence)
{
    return borland_fseek(file, off, whence);
}

/*
 * 0x0baac
 */
int32_t borland_ftell_far(struct file_rec *file)
{
    return borland_ftell(file);
}

/*
 * 0x0babb
 */
uint16_t borland_fread_far(uint8_t *buf, uint16_t size, uint16_t count,
                           struct file_rec *file)
{
    return borland_fread(buf, size, count, file);
}

/*
 * 0x0bad4
 */
uint16_t borland_fwrite_far(const uint8_t *buf, uint16_t size, uint16_t count,
                            struct file_rec *file)
{
    return borland_fwrite(buf, size, count, file);
}

/*
 * 0x0baed
 */
int16_t borland_fputc_far(int16_t c, struct file_rec *file)
{
    return borland_fputc(c, file);
}

/*
 * 0x0bb00
 */
void borland_rewind_far(struct file_rec *file)
{
    borland_rewind(file);
}

/*
 * 0x0bb0f
 */
int16_t borland_fclose_far(struct file_rec *file)
{
    return borland_fclose(file);
}

/*
 * 0x0bb1e
 *
 * The far-callable face of `malloc`: one argument off the stack and straight
 * on to `heap_malloc`.
 */
uint8_t * heap_malloc_far(uint16_t bytes)
{
    /*
     * **The `far` is the call, not the pointer.** 0x0bb1e is a thunk - one
     * word pushed, `push cs`, a near call to `heap_malloc`, `retf` - and
     * `heap_malloc` ends `mov ax,bx / retf` with nothing in DX. So a near
     * heap block is one 16-bit DGROUP offset, which `heap_malloc` answers as
     * the pointer it is, NULL for the original's 0.
     */
    return heap_malloc(bytes);
}


/*
 * 0x0bb2d
 *
 * The far-callable face of `free`: one argument off the stack and straight on
 * to `heap_free`. The `inc sp` twice that cleans it is two bytes shorter than
 * an `add sp,2` and does the same.
 */
void heap_free_far(uint8_t * p)
{
    heap_free(p);
}

/*
 * 0x0bb3c
 *
 * The far-callable face of `strcat`, the same shape as its neighbours: two
 * words off the stack and straight on to `string_concat`. **Nothing calls
 * it** - no `lcall` and no near call anywhere in the image - so it was linked
 * in with the rest of this module and never used. The same is true of the
 * `strchr` and `fgetc` faces below; the other four are called from 8 to 56
 * sites each.
 */
char *string_concat_far(char *dst, const char *src)
{
    return string_concat(dst, src);
}

/*
 * 0x0bb4f
 *
 * The far-callable face of `strcpy`: it takes the two words off the stack and
 * hands them straight on.
 */
char *string_copy_far(char *dst, const char *src)
{
    return string_copy(dst, src);
}

/*
 * 0x0bb62
 *
 * The far-callable face of `strchr`: the string and the character, the
 * latter pushed as a word, on to `string_chr`. Uncalled - see 0x0bb3c.
 */
char *string_chr_far(char *s, int16_t c)
{
    return string_chr(s, c);
}

/*
 * 0x0bb75
 *
 * The far-callable face of `calloc`: it takes the two words off the stack and
 * hands them straight on. Four instructions and a `retf`.
 */
uint8_t *heap_calloc_far(uint16_t count, uint16_t size)
{
    return heap_calloc(count, size);
}

/*
 * 0x0bb88
 *
 * The far-callable face of `fgetc`: one word, the stream, on to
 * `borland_fgetc`. Uncalled - see 0x0bb3c.
 */
int16_t borland_fgetc_far(struct file_rec *file)
{
    return borland_fgetc(file);
}


