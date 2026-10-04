/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The resource files' public face**: open, close, read, write, seek and
 * size a resource by handle, over the streams resource.c keeps. 1.00 also
 * held the writing side of types 1 and 2 here - a run-length coder and the
 * LZW coder of Unix `compress` - which nothing called; 1.11 has neither.
 *
 * One module of the original's **code segment 1c25**, image
 * 0x1f9c4..0x20015, far routines, with no data of its own. Both ends are
 * assembly: the LZW decoder's module before it, the LZSS one's after.
 *
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x1f9c4
 *
 * **Open a resource in a file** and answer its handle, or -1. The file is
 * at the resource's header: a type byte and the decoded size.
 *
 * Reading (an "r" in the mode), the type is taken from the file, the slot
 * given the memory that type needs, the compressed size recorded as the
 * caller gives it and the decoded size read, and the type's reset run.
 * Writing, the type is the caller's first argument: it is written, then
 * four bytes of header to be filled in at close, and the type's
 * open-for-writing run. Either way the stream reads from a file.
 *
 * `open_resource_slot` is handed the mode, which it does not read.
 */
int16_t open_resource(int16_t type, FILE *file, char *mode, int32_t size)
{
    int16_t slot;
    int32_t header;

    if ((slot = open_resource_slot(mode)) == -1)
        return -1;
    g_stream_rec->data.file = file;
    g_stream_rec->start = game_ftell(file);
    g_stream_rec->in_pos = 5;
    if (string_contains_r(mode)) {
        if (prepare_resource_slot(type = g_stream_rec->kind = game_fgetc(file),
                                  mode) == -1) {
            game_fseek(file, -1L, 1);
            return close_resource_slot(slot);
        }
        g_stream_rec->in_end = size;
        game_fread((uint8_t *)&g_stream_rec->size, 1, 4, file);
        if (g_engine_res_handlers.type[type].reset)
            g_engine_res_handlers.type[type].reset();
        g_stream_rec->kind |= 0x40;
    } else {
        if (prepare_resource_slot(type, mode) == -1)
            return close_resource_slot(slot);
        game_fputc(type, file);
        game_fwrite((uint8_t *)&header, 1, 4, file);
        if (g_engine_res_handlers.type[type].open_write)
            g_engine_res_handlers.type[type].open_write();
    }
    g_stream_rec->kind |= 0x20;
    return slot;
}

/*
 * 0x1fb08
 *
 * **Open a resource in memory**: `open_resource` for a block the caller
 * holds, the header read from or written to its front. Nothing calls it.
 */
int16_t open_resource_mem(int16_t type, char huge *data, char *mode, int32_t size)
{
    int16_t slot;

    if ((slot = open_resource_slot(mode)) == -1)
        return -1;
    g_stream_rec->data.ptr = data;
    g_stream_rec->kind = type;
    g_stream_rec->in_pos = 5;
    if (string_contains_r(mode)) {
        if (prepare_resource_slot(type = g_stream_rec->kind = *data++,
                                  mode) == -1)
            return close_resource_slot(slot);
        far_memcpy((uint8_t *)&g_stream_rec->size, (uint8_t huge *)data, 4);
        g_stream_rec->in_end = size;
        if (g_engine_res_handlers.type[type].reset)
            g_engine_res_handlers.type[type].reset();
        g_stream_rec->kind |= 0x40;
    } else {
        if (prepare_resource_slot(type, mode) == -1)
            return close_resource_slot(slot);
        *g_stream_rec->data.ptr = type;
    }
    return slot;
}

/*
 * 0x1fc05
 *
 * **Close a resource**, answering the bytes the writing side put out - zero
 * for one opened to read. Writing, the type's flush runs with `final` set,
 * and the decoded size goes into the header: back over it in the file, or
 * into the block's front.
 */
int16_t close_resource(int16_t handle)
{
    if (!select_resource(handle))
        return -1;
    g_stream_written = 0;
    if (!(g_stream_kind & 0x40)) {
        g_engine_res_handlers.type[g_resource_handler].flush(1);
        if (g_stream_kind & 0x20) {
            game_fseek(g_resource_file, g_stream_rec->start + 1, 0);
            game_fwrite((uint8_t *)&g_stream_rec->size, 4, 1,
                        g_resource_file);
            game_fseek(g_resource_file, 0L, 2);
        } else
            far_memcpy((uint8_t huge *)(g_stream_rec->data.ptr + 1),
                       (uint8_t *)&g_stream_rec->size, 4);
    }
    close_resource_slot(handle);
    return g_stream_written;
}

/*
 * 0x1fcd4
 *
 * **Read** `count` bytes of a resource to `dst`, answering how many, or -1
 * for a handle that names nothing. The destination is normalised into the
 * stream's output cursor, and bit 0x40 is what tells the emitters to write
 * rather than skip.
 */
int16_t read_resource(int16_t handle, uint8_t far *dst, uint16_t count)
{
    if (!select_resource(handle))
        return -1;
    g_stream_out = normalise_pointer_far(dst);
    g_resource_flags |= 0x40;
    return resource_read(handle, count);
}

/*
 * 0x1fd10
 *
 * **Write** `count` bytes to a resource opened for writing: into the spill
 * ring a ring's worth at a time, the type's flush after each. Answers the
 * bytes the flushes put out. Nothing calls it.
 */
int16_t write_resource(int16_t handle, uint8_t huge *src, uint16_t count)
{
    uint8_t huge *p;
    int16_t end;
    int16_t stop;
#ifndef __TURBOC__
    uint8_t *buf;
#endif

    if (!select_resource(handle))
        return -1;
    g_stream_written = 0;
    g_stream_rec->size += count;
    /* The buffer is kept in DI and each byte copied in inline `asm`, the
       compiler's own assembler (resource.c's `emit_byte` is the same): the
       image has DI loaded once and SI only for the source pointer, which no
       C spelling of the copy gives. */
#ifdef __TURBOC__
    _DI = (uint16_t)g_stream_rec->work;
#else
    buf = g_stream_rec->work;
#endif
    p = src;
    while (count) {
        end = g_stream_rec->spill_end;
        stop = (g_stream_rec->spill_start - 1) & 0x7f;
        do {
#ifdef __TURBOC__
            asm mov bx, end
            asm les si, p
            asm mov al, es:[si]
            asm mov [bx+di], al
#else
            buf[end] = *p;
#endif
            p++;
            end++;
            count--;
            end &= 0x7f;
        } while (end != stop && count);
        g_stream_rec->spill_end = end & 0x7f;
        g_engine_res_handlers.type[g_resource_handler].flush(0);
    }
    return g_stream_written;
}

/*
 * 0x1fdd1
 *
 * The decoded size of a resource, from its header, or -1 for a handle that
 * names nothing.
 */
int32_t resource_size(int16_t handle)
{
    if (!select_resource(handle))
        return -1L;
    return g_stream_rec->size;
}

/*
 * 0x1fdf5
 *
 * **Seek** within a resource, answering the position reached, or -1 for a
 * handle that names nothing.
 *
 * A compressed stream cannot be seeked, so this skips by decoding: the
 * target from the whence - 0 the start, 1 the position, 2 the size - and the
 * distance to it read in chunks of at most 0x7d00 and thrown away, which is
 * what not setting bit 0x40 makes happen. A target behind the position
 * restarts the stream first, and one past the end is clamped to it. Each
 * chunk re-derives the input cursor from the record's own.
 */
int32_t resource_seek(int16_t handle, int32_t by, int16_t whence)
{
    int32_t t;

    if (!select_resource(handle))
        return -1L;
    t = 0;
    switch (whence) {
    case 1:
        t = g_stream_rec->pos;
        break;
    case 2:
        t = g_stream_rec->size;
        break;
    }
    t += by;
    if (g_stream_rec->pos == t)
        return t;
    if (g_stream_rec->pos > t) {
        restart_resource_stream(handle);
        if (t <= 0)
            return 0L;
    } else if (g_stream_rec->size <= t)
        t = g_stream_rec->size - g_stream_rec->pos;
    else
        t -= g_stream_rec->pos;
    while ((t -= (uint16_t)resource_read(handle, t < 0x7d00L ? (uint16_t)t : 0x7d00)) != 0)
        g_stream_in = (char huge *)normalise_pointer_far(
            (uint8_t huge *)(g_stream_rec->data.ptr + g_stream_rec->in_pos));
    return g_stream_rec->pos;
}

/*
 * 0x1ff52
 *
 * **Put a resource stream back to its beginning**, so a seek backwards can
 * then skip forwards. Only a stream opened to read can be; anything else
 * answers -1. The type's reset runs, and the record goes back to where
 * `open_resource` left it: past the five-byte header, in the file or in
 * memory, with nothing decoded and nothing spilled.
 */
int16_t restart_resource_stream(int16_t handle)
{
    if (!select_resource(handle) || !(g_stream_kind & 0x40))
        return -1;
    if (g_engine_res_handlers.type[g_resource_handler].reset)
        g_engine_res_handlers.type[g_resource_handler].reset();
    g_stream_rec->in_pos = 5;
    if (g_stream_rec->kind & 0x20)
        game_fseek(g_resource_file, g_stream_rec->start + 5, 0);
    else
        g_stream_in = (char huge *)normalise_pointer_far(
            (uint8_t huge *)(g_stream_rec->data.ptr + 5));
    g_stream_rec->spill_end = g_stream_rec->spill_start =
        g_stream_rec->pos = 0;
    return 0;
}
