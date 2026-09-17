/*
 * The Incredible Machine - reconstruction
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file: it is
 * derived from someone else's executable.
 *
 * **The game's far glue in `_TEXT`**: code segment 0000, image range
 * 0x0bb1e..0x0bbfd. Four far entries into the runtime's near heap and string
 * copy - `push cs`, a near call, `retf` - and the interface to the loaded
 * sound module: nine wrappers that load a function number and fall into the
 * trampoline at 0x0bbd4, which far-calls the module through DGROUP 0x4a98.
 *
 * **This boundary is ours, and so is the one on either side of it.** Segment
 * 0000 is `_TEXT`, which three parties share: Borland's startup owns the entry
 * point at its front, the library modules all put their code there too, and
 * some of the game's units did as well. TLINK combines a segment's parts in
 * the order it meets them - the startup first, then the program's objects in
 * link order, then the library modules it pulls in at the end - so the segment
 * reads startup, game, library. These routines are the last of the game's
 * objects before the library begins at 0x0bbfe with its `int 21h` wrappers.
 * `machine.c` holds the game's other `_TEXT` code and `borland_heap.c` and
 * `borland_file.c` the library's; this file is what sits between, and it is a
 * file of its own so that the library units stay the library and nothing else.
 * Which compiler option put these units into `_TEXT` cannot be read off the
 * image and is not claimed here.
 *
 * Functions are in address order and each carries the image offset it was
 * read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

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
char *string_chr_far(char *s, uint16_t c)
{
    return string_chr(s, (char)c);
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


/*
 * 0x0bb98
 *
 * Install the module. Its two arguments are the host callback and a flag, and
 * `asb_install` takes neither: what the original passes on the stack the
 * module reads through SI, and this one reads nothing.
 */
uint16_t sound_module_install(uint16_t callback, uint16_t flag)
{
    (void)callback;
    (void)flag;
    return call_sound_module(0, (union sound_module_args *)(void *)dg_ptr(dgroup, guest_sp));
}


/*
 * 0x0bb9f
 */
uint16_t sound_module_set_rate(union sound_module_args * si)
{
    return call_sound_module(6, si);
}


/*
 * 0x0bba6
 *
 * The service call, and the only wrapper that touches hardware itself: it
 * sends the non-specific EOI to the master PIC before entering the module.
 * `ASB:` function 1 is a bare `xor ax,ax; ret`, so on this module the EOI is
 * the whole of it.
 */
uint16_t sound_module_service(union sound_module_args * si)
{
    io_out8(0x20, 0x20);
    return call_sound_module(1, si);
}


/*
 * 0x0bbb1, 0x0bbb8, 0x0bbbf
 *
 * Three the game calls and `ASB:` does not implement - its entries 9, 10 and
 * 11 are the bare `ret`s at 0x42c, 0x42f and 0x430.
 */
uint16_t sound_module_9(union sound_module_args * si)  { return call_sound_module(9, si); }
uint16_t sound_module_10(union sound_module_args * si) { return call_sound_module(10, si); }
uint16_t sound_module_11(union sound_module_args * si) { return call_sound_module(11, si); }


/*
 * 0x0bbc6
 *
 * Take the module down.
 */
uint16_t stop_loaded_module(void)
{
    return call_sound_module(2, (union sound_module_args *)(void *)dg_ptr(dgroup, guest_sp));
}


/*
 * 0x0bbcd
 */
uint16_t sound_module_shutdown(void)
{
    return call_sound_module(12, (union sound_module_args *)(void *)dg_ptr(dgroup, guest_sp));
}


/*
 * 0x0bbd4
 *
 * The trampoline every call into the **loaded sound module** goes through:
 * push a frame, point SI at the caller's own arguments - `bp + 8`, which is
 * past the saved BP, this routine's near return and the wrapper's far one -
 * and far-call the module through DGROUP 0x4a98.
 *
 * The nine wrappers below it are one shape: load AX with a function number,
 * come here, and `retf`. Which numbers exist is a property of the *game*, not
 * of the module: these nine are the only calls in the image, so the module's
 * other seven entries are never reached however many it implements.
 *
 * The port dispatches into `asb_dispatch` rather than through the pointer,
 * because `ASB:` is the one module it transcribes. A different module in
 * RESOURCE.CFG would need its own, and `setup_sound_device` would have loaded
 * a block of code the port has no body for.
 */
uint16_t call_sound_module(uint16_t fn, union sound_module_args * si)
{
    return asb_dispatch(fn, si);
}


/*
 * 0x0bbe6
 *
 * Ask the module where it has got to. Six bytes of stack are reserved for the
 * three words it writes back and popped afterwards - the original keeps the
 * third in DX and drops the first two.
 */
uint16_t sound_module_position(uint16_t *a, uint16_t *b, uint16_t *c)
{
    /*
     * Six bytes the module fills in - `asb_position` writes three words
     * through the pointer and nothing else looks at the address. The comment
     * here used to say they had to be the guest's because the module reads
     * them through SI; the module is `asb_dispatch` in this port, its own C,
     * and SI is the shape the original had to pass a pointer in.
     */
    union sound_module_args fp;

    call_sound_module(13, &fp);

    if (a) *a = fp.position.id;
    if (b) *b = (uint16_t)fp.position.position;
    if (c) *c = (uint16_t)(fp.position.position >> 16);

    return 0;
}
