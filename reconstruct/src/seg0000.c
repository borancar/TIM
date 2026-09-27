/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **What is left of code segment 0000** (`_TEXT`), image 0x08fc3..0x0dff0,
 * while its modules are split out from the front: the screen and the
 * pointer's regions, sound selection, the heap checks, the resource files,
 * drawing pages and rectangles, and the rest. Everything before it is in
 * collide.c through crtc.c. Several modules are still in here - calls
 * that reach back bare, and `_DATA` in link order, put boundaries near
 * 0x08136, 0x08fc3, 0x0a05f and 0x0aa76 - so the name is the segment's
 * until each is split out and named.
 *
 * Not yet judged: each module gets its markers as it is split out.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **Which four characters of a filename its hash is made of**, at DGROUP
 * 0x28d2: `hash_filename` folds the bytes at these positions of the padded
 * name - 0, 1, 6, 7 in the image - into a long.
 *
 * DGROUP 0x28d2..0x28d6, 0x04 bytes.
 */
struct machine_hash_order {
    uint8_t   hash_order[4];      /* +0x00 [4] */
} PACKED;

struct machine_hash_order MACHINE_HASH_ORDER DGROUP_AT(0x28d2) = { .hash_order = { 0x00, 0x01, 0x06, 0x07 } };

/*
 * **The resource map's name and three modes**, DGROUP 0x28d6..0x28ec:
 * "RESOURCE.MAP" and "rb" three times, one for each of the routines at
 * 0x0964b, 0x09a7f and 0x09b02 that opens a file.
 */
struct machine_resource_map_names {
    char      resource_map[13];   /* +0x00  "RESOURCE.MAP" */
    char      mode_rb_a[3];       /* +0x0d  "rb" */
    char      mode_rb_b[3];       /* +0x10  "rb" */
    char      mode_rb_c[3];       /* +0x13  "rb" */
} PACKED;

struct machine_resource_map_names MACHINE_RESOURCE_MAP_NAMES DGROUP_AT(0x28d6) = {
    .resource_map = "RESOURCE.MAP",
    .mode_rb_a = "rb",
    .mode_rb_b = "rb",
    .mode_rb_c = "rb",
};

/*
 * **Which page pointers the saved-rect lists are restored between**, at
 * DGROUP 0x2d0a: pairs of addresses of the driver's page words at
 * 0x38a0..0x38a4 (and of the word at 0x2d08), walked by
 * `restore_saved_rect_lists` from pair 0 - or from pair 1 alone - until
 * the next pair's second word is 0. Nine pairs and the terminating pair
 * fill the run to 0x2d32.
 *
 * DGROUP 0x2d0a..0x2d32, 0x28 bytes.
 */
struct machine_page_pairs {
    struct {
        dg_near_t src;             /* +0x00  the address of a page word */
        dg_near_t dst;             /* +0x02 */
    } pair[10];                   /* +0x00 [0x28] */
} PACKED;

struct machine_page_pairs MACHINE_PAGE_PAIRS DGROUP_AT(0x2d0a) = {
    .pair = {
        { .src = 0x2d08, .dst = 0x38a4 }, { .src = 0x38a2, .dst = 0x38a4 },
        { .src = 0x2d08, .dst = 0x38a2 }, { .src = 0x38a0, .dst = 0x38a2 },
        { .src = 0x2d08, .dst = 0x38a0 }, { .src = 0x38a4, .dst = 0x38a0 },
        { .src = 0x38a4, .dst = 0x38a2 }, { .src = 0x38a2, .dst = 0x38a0 },
        { .src = 0x38a0, .dst = 0x38a4 },
    },
};

/*
 * **The cursor, the fade, and the palette waiting to load**, DGROUP 0x2d32..0x2d48, 0x16 bytes.
 */
struct machine_cursor_state {
    uint16_t  page;               /* +0x00 [2]  the page the middle call passes */
    uint16_t  screen_disturbed;   /* +0x02 [2]  the saved rectangles are put back when this says so */
    uint16_t  fade_first;          /* +0x04 [2] */
    uint16_t  fade_count;          /* +0x06 [2] */
    uint8_t far *pending_pal;   /* +0x08 [4]  a palette waiting to be loaded */
    uint16_t  cursor_off;         /* +0x0c [2]  clear turns the whole cursor off - nothing is drawn */
    int16_t   delay_reload;       /* +0x0e [2]  the delay counts down and is reloaded from here */
    uint16_t  read_driver;        /* +0x10 [2]  take the position from the driver rather than the last known */
    /* **May the timer redraw the cursor?** `timer_callback` is the only
       reader - it redraws only while this is set and the guard is clear - and
       every routine that is about to draw clears it and puts it back, so the
       cursor is not lifted and dropped underneath a half-drawn frame. */
    uint16_t  timer_draws_cursor; /* +0x12 [2] */
    int16_t   slots_unset;          /* +0x14 [2] */
} PACKED;

struct machine_cursor_state MACHINE_CURSOR_STATE DGROUP_WAS(0x2d32) = {
    .page = 0x0001,
    .fade_count = 0x0100,
    .cursor_off = 0x0001,
    .delay_reload = 0x000c,
    .read_driver = 0x0001,
    .timer_draws_cursor = 0x0001,
    .slots_unset = 0x0001,
};

/*
 * **The interrupt's own stack**, DGROUP 0x317e..0x3182, 0x04 bytes.
 *
 * `isr_stack_switch` files `SS:SP` here on the way in so the handler can run
 * on a private stack and put the interrupted one back on the way out. The port
 * does not switch stacks - it has no single SP to switch - but it writes both
 * words, because anything else is free to read them.
 */
struct machine_isr_stack {
    uint16_t  saved_ss;           /* +0x00 [2] */
    uint16_t  saved_sp;           /* +0x02 [2] */
} PACKED;

struct machine_isr_stack MACHINE_ISR_STACK DGROUP_AT(0x317e);

/*
 * **The eleven archives**, DGROUP 0x548f..0x55c3, 0x134 bytes. Index 0 is never where a search
 * begins - `find_entry_for_pointer` starts at 1 when `last_record` is clear.
 */
struct machine_archives {
    struct archive slot[0xb];     /* +0x00 [0x134] */
} PACKED;

struct machine_archives MACHINE_ARCHIVES DGROUP_WAS(0x548f);

/*
 * **The ten game files**, DGROUP 0x55c3..0x5677, 0xb4 bytes.
 */
struct machine_game_files {
    struct game_file files[0xa];  /* +0x00 [0xb4] */
} PACKED;

struct machine_game_files MACHINE_GAME_FILES DGROUP_BSS(0x55c3);

/*
 * **The twenty saved-rectangle slots**, DGROUP 0x56b8..0x56e0, 0x28 bytes. Each is a near
 * pointer to the head of a chain of records, or zero for an empty slot;
 * `find_saved_rect_slot` walks all twenty and `restore_saved_rect_lists`
 * counts down every record on every chain. Twenty words end at 0x56e0, where
 * the free list is.
 *
 * A slot is handed around as a pointer to its word - `find_saved_rect_slot`
 * answers one, or NULL - and the records on a chain are `struct
 * rect_list_entry`.
 */
struct machine_rect_slots {
    dg_near_t slot[0x14];         /* +0x00 [0x28] */
} PACKED;

struct machine_rect_slots MACHINE_RECT_SLOTS DGROUP_BSS(0x56b8);

/*
 * **How many saved-rectangle records the pool has been given**, DGROUP
 * 0x56b6: `build_rect_pool` adds each block's count and `rect_pool_count`
 * answers it. Nothing else names the word.
 */
struct machine_rect_count {
    uint16_t  count;              /* +0x00 */
} PACKED;

struct machine_rect_count MACHINE_RECT_COUNT DGROUP_BSS(0x56b6);

/*
 * **The saved-rect free list, and where the cursor is to be drawn**, DGROUP
 * 0x56e0..0x56e6, 0x06 bytes. The pair is the pointer less the bitmap's hot
 * spot, worked out before the redraw and compared with the slot's own so an
 * unmoved cursor is not drawn again.
 */
struct machine_rect_free {
    /* The free list of `rect_list_entry` records. Only ever appended to -
       here **and in the original**: the builder that fills it, 0x0a05f, is
       reached only from the creator at 0x0a0d7, and nothing in the image
       calls that. See `struct rect_list_entry`. */
    dg_near_t rect_free_ptr;      /* +0x00 [2] */
    int16_t   draw_x;          /* +0x02 [2] */
    int16_t   draw_y;          /* +0x04 [2] */
} PACKED;

struct machine_rect_free MACHINE_RECT_FREE DGROUP_BSS(0x56e0);

/*
 * **The two page slots**, DGROUP 0x56e6..0x5726, 0x40 bytes.
 *
 * `claim_page_slot` walks two of them at a stride of 0x20, which is
 * `sizeof(struct page_slot)`, and matches on the top bits of the record's
 * first field - the page it belongs to. It answers the slot's own offset, so
 * the callers keep taking a `PAGESLOT`.
 */
struct machine_page_slots {
    struct page_slot slots[2];   /* +0x00 [0x40] */
} PACKED;

struct machine_page_slots MACHINE_PAGE_SLOTS DGROUP_BSS(0x56e6);

/*
 * **The drawing state saved across an interrupt**, DGROUP 0x5726..0x5734, 0x0e bytes.
 */
struct machine_saved_draw_state {
    uint16_t  saved_a;            /* +0x00 [2]  seven values - the clip box and the two page segments. */
    int16_t   saved_b;            /* +0x02 [2]  0x5726's high half is always zero: it is restored as a byte */
    int16_t   saved_c;            /* +0x04 [2] */
    int16_t   saved_d;            /* +0x06 [2] */
    int16_t   saved_e;            /* +0x08 [2] */
    uint16_t  saved_f;            /* +0x0a [2] */
    uint16_t  saved_g;            /* +0x0c [2] */
} PACKED;

struct machine_saved_draw_state MACHINE_SAVED_DRAW_STATE DGROUP_BSS(0x5726);

/*
 * **The four object buffers `claim_buffer_slot` hands out**: a taken flag
 * apiece at 0x5734; the buffers themselves are `MACHINE_RECT_BUFFERS.slot`, the far
 * pointers at 0x5758 up to `DG5768`. Four is the routine's own bound.
 *
 * DGROUP 0x5734..0x5738, 0x04 bytes.
 */
struct machine_buffer_used {
    uint8_t   used[4];            /* +0x00 [4] */
} PACKED;

struct machine_buffer_used MACHINE_BUFFER_USED DGROUP_BSS(0x5734);

/*
 * **The palette request and the fade**, DGROUP 0x5738..0x5742, 0x0a bytes.
 */
struct machine_palette_fade {
    uint8_t far *request;       /* +0x00 [4]  cleared when taken, so one
                                            request loads once */
    uint16_t  fade_mark;          /* +0x04 [2]  reset to zero by a load, which forces the fade to run; */
    /* **A colour that walks 0 to 15**, stepped and plotted when a cursor slot
       has no bitmap - one pixel, in the next colour each time. Nothing else
       reads it. */
    int16_t   plot_colour;        /* +0x06 [2] */
    int16_t   busy;               /* +0x08 [2]  non-zero suppresses the slot release, and everything waits on it */
} PACKED;

struct machine_palette_fade MACHINE_PALETTE_FADE DGROUP_WAS(0x5738);

/*
 * **The two buttons' state machines**, at DGROUP 0x5742 - eight bytes each,
 * and `reset_input_state` clears both as two blocks of four words. Sixteen
 * bytes end at 0x5752, which is the guard the clear holds across itself.
 *
 * `button_state` is the whole of what reads them; the field names are its
 * comment.
 */
struct button {
    int16_t   state;              /* +0x00  0 up, 2 pressed, 4 clicked, 8 held */
    int16_t   was_down;           /* +0x02  what the driver said last time */
    int16_t   presses;            /* +0x04  what tells a click from a double one */
    int16_t   delay;              /* +0x06  reloaded from 0x2d40 on every change */
} PACKED;

/* DGROUP 0x5742..0x5752, 0x10 bytes. */
struct machine_buttons {
    struct button button[2];      /* +0x00 [0x10] */
} PACKED;

struct machine_buttons MACHINE_BUTTONS DGROUP_BSS(0x5742);

/*
 * **A far pointer per saved rectangle**, DGROUP 0x5758..0x5768, indexed from
 * ONE: slots 1 to 4 are the buffers `claim_buffer_slot` hands out. The
 * original indexes `[bx + 0x5754]` with `bx = slot * 4`, so its slot 0 would be
 * the four bytes at 0x5754 - `DG5752.frame_flag` and `size_word` - and it is
 * never handed out. The array starts at slot 1, and every use subtracts one.
 */
struct machine_rect_buffers {
    uint8_t far *slot[4];       /* +0x00  slots 1 to 4 */
} PACKED;

struct machine_rect_buffers MACHINE_RECT_BUFFERS DGROUP_WAS(0x5758);

/*
 * 0x09784
 *
 * Free each archive's list of entries - the far pointer at +0x18 of the eleven
 * 0x1c-byte records from DGROUP 0x548f, which `load_archive_map` allocated -
 * and then put DOS's critical-error vector 24h back from the pair kept at
 * 0x5677.
 *
 * The loop is `si <= 10`, so eleven and not ten, which is what says the table
 * has eleven records rather than the ten the `game_file` table has. Each
 * pointer is tested as a pair before it is freed, and zeroed after.
 *
 * It was called `free_sound_slots` for as long as its stride was the only
 * thing read about it: 0x54a7 is a record's +0x18 and not a table of its own,
 * and there is no sound anywhere near it.
 */
void free_archive_lists(void)
{
    int16_t i;

    for (i = 0; i <= 10; i++) {
        struct archive *a = &MACHINE_ARCHIVES.slot[i];

        if (a->list == NULL)
            continue;

        dos_free_far(a->list);

        a->list = NULL;
    }

    if (dg_far_ptr(DG5677.crit_vec) != FAR_NULL_PTR) {
        dos_setvect(0x24, DG5677.crit_vec);
        DG5677.crit_vec = FAR_NULL;
    }

    DG547A.scanned = 0;
}

/*
 * NOT a transcription: a boundary the port chose. The enclosing routine
 * contains three inlined copies of this loop, and this is the loop, lifted out
 * so the port writes it once. Its address is given below rather than here,
 * because an address on the first line of a block *is* the provenance mark.
 *
 * Walk each list once, from its head to either a null entry or a match. The
 * body matches 0x09904..0x09941 instruction for instruction - the 0x1c-strided
 * table at 0x54a7/0x54a9, the `(off | seg) == 0` test, the compare against the
 * wanted pair, the `off += 8` - but **the enclosing routine is not
 * transcribed**: 0x098e0 takes one argument at bp+6, reads what it is looking
 * for out of the globals at 0x5482 and 0x5484 rather than from arguments, and
 * carries on past this loop at 0x09941. Its signature here is the helper's,
 * not the original's.
 *
 * It carried a bare `0x098e0` before, which files a routine as transcribed
 * under `tests/provenance.py`, and this is not one.
 */
void scan_entry_list(int16_t idx, uint32_t want,
                     const struct archive_entry **at)
{
    *at = (const struct archive_entry *)(void *)MACHINE_ARCHIVES.slot[idx].list;

    for (;;) {
        /* Each entry opens with its 32-bit key; a zero one ends the list.
           The original steps the offset eight bytes inside the list's
           segment, and the list is sized in a word, so it cannot leave it. */
        uint32_t key = (*at)->key;

        if (key == 0 || key == want)
            return;
        (*at)++;
    }
}

/*
 * 0x0917f
 *
 * The game's own `fclose`, and the last of the set over the archive.
 *
 * A null `FILE` is -1 before anything else. With the archive open and an entry
 * for this stream, the close is against the entry rather than the stream: the
 * one-entry cache is thrown away by looking up handle 0, whatever `FILE` the
 * entry carries at +0x10 is closed, its +0xe cleared, and the count of open
 * resources at DGROUP 0x5486 dropped by one.
 *
 * A stream with no entry is closed directly through the runtime, and skips both
 * the entry bookkeeping and the open count. That is the path a **saved
 * machine** takes: it was opened by name and never came out of the archive.
 *
 * A failure sets bit 0 of DGROUP 0x567b, which is where this layer collects
 * whether anything went wrong.
 */
int16_t game_fclose(FILE *file)
{
    struct game_file *si = GAME_FILE_NONE;
    int16_t di = 0;

    if (file == 0)
        return -1;

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE) {
        di = borland_fclose(file);
        DG5677.failures = (int16_t)(DG5677.failures | (di == -1 ? 1 : 0));
        return di;
    }

    archive_entry_for(0);

    if (si->stream_ptr != 0)
        di = borland_fclose(FILEREC_PTR(si->stream_ptr));

    si->in_use = 0;
    DG547A.open_immediate = (uint8_t)(DG547A.open_immediate - 1);

    DG5677.failures = (int16_t)(DG5677.failures | (di == -1 ? 1 : 0));
    return di;
}

/*
 * 0x093e0
 *
 * `rewind` over the archive: `game_fseek` to nought from the start, and
 * nothing else. Five pushes and a call.
 */
void game_rewind(FILE *file)
{
    game_fseek(file, 0, 0);
}

/*
 * 0x091ef
 *
 * The game's own `fread`. Everything that reads a resource comes through here,
 * and it decides between the loose file and the archive.
 *
 * With the archive closed - DGROUP 0x547e zero - or with no entry for this
 * `FILE`, it is a plain forward to the runtime's `fread` and nothing else
 * happens. An entry whose +0x10 holds a `FILE` of its own is forwarded the same
 * way, with that one substituted.
 *
 * Otherwise the read is against a stretch of the archive, and three things have
 * to happen that a plain `fread` would not do.
 *
 * The request is **clamped** to what is left of the entry, by taking whole
 * items off the count until `size * count` fits in `+6:+8` minus the position
 * at `+0xa:+0xc`. The comparison is 32-bit with the request's high half a
 * constant zero, which is why the first branch of it can never be taken.
 *
 * Then the archive file is made current and seeked to the entry's base plus the
 * position, and the `FILE` to read from is looked up in the table at DGROUP
 * 0x549f, 0x1c bytes per open archive.
 *
 * Afterwards the entry's position and the archive's own running total at
 * 0x54a1 both advance by what was actually read - `n * size`, not what was
 * asked for.
 */
uint16_t game_fread(uint8_t * buf, uint16_t size, uint16_t count,
                    FILE *file)
{
    struct game_file *di = GAME_FILE_NONE;

    if (DG547A.archive_count != 0)
        di = archive_entry_for(file);

    if (di == GAME_FILE_NONE)
        return borland_fread(buf, size, count, file);

    if (di->stream_ptr != 0)
        return borland_fread(buf, size, count, FILEREC_PTR(di->stream_ptr));

    {
        uint16_t bytes = (uint16_t)((int16_t)size * (int16_t)count);
        uint16_t n, got;

        for (;;) {
            if (bytes == 0)
                break;

            uint32_t left = di->size - di->pos;

            if ((left >> 16) != 0)
                break;
            if (bytes <= (uint16_t)left)
                break;

            count--;
            bytes = (uint16_t)(bytes - size);
        }

        make_file_current(di->archive);

        {
            uint32_t at = di->base + di->pos;

            seek_file_to(at);
        }

        file = FILEREC_PTR(MACHINE_ARCHIVES.slot[di->archive].stream_ptr);

        n = borland_fread(buf, size, count, file);

        got = (uint16_t)((int16_t)n * (int16_t)size);

        di->pos += got;

        {
            struct archive *a = &MACHINE_ARCHIVES.slot[di->archive];

            a->pos += got;
        }

        return n;
    }
}

/*
 * 0x092dc
 *
 * The game's own `fseek`, and `game_fread`'s counterpart: the same choice
 * between the loose file and the archive, the same substitution when an entry
 * carries its own `FILE`.
 *
 * Against an archive entry there is no seeking to do on the file at all - only
 * the entry's own position at +0xa:+0xc is moved, and the archive file is
 * seeked when something is actually read. So the three whences are worked out
 * here in 32-bit arithmetic:
 *
 *   0  the offset as given
 *   1  the offset plus the position now
 *   2  the entry's size minus the offset, or zero if the offset is larger
 *
 * and the result is clamped to the entry's size, so seeking past the end parks
 * at the end rather than reporting an error. The answer is 0 either way.
 */
int16_t game_fseek(FILE *file, int32_t off, int16_t whence)
{
    struct game_file *si = GAME_FILE_NONE;

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE)
        return borland_fseek(file, off, whence);

    if (si->stream_ptr != 0)
        return borland_fseek(FILEREC_PTR(si->stream_ptr), off, whence);

    /* Every comparison here is **unsigned** over the pair, which is the
       original's `cmp hi / ja / jb / cmp lo / ja` and not the signed shape
       `resource_seek` uses. */
    if (whence == 1) {
        off = (int32_t)((uint32_t)off + si->pos);
    } else if (whence == 2) {
        if (si->size > (uint32_t)off)
            off = (int32_t)(si->size - (uint32_t)off);
        else
            off = 0;
    }

    if (si->size < (uint32_t)off)
        off = (int32_t)si->size;

    si->pos = (uint32_t)off;
    return 0;
}

/*
 * 0x094fb
 *
 * **`fwrite`, through the archive layer.** A pointer, an element size, a count
 * and a file, answering how many elements went - and every caller in the
 * machine writer compares that with 1.
 *
 * **A file may be an entry in the resource archive rather than a file of its
 * own**, and when the archive is in use at DGROUP 0x547e this asks
 * `archive_entry_for` first. An entry writes to the handle at its +0x10; an
 * entry without one writes *nothing* and answers zero, which the callers then
 * read as a short write. A file that is not an entry at all falls through to
 * the plain path, and so does everything when the archive is not in use.
 *
 * The failure mark at DGROUP 0x567b is **or-ed, not set**: it accumulates
 * across every write anyone does rather than describing this one. That is a
 * different thing from the machine writer's own 0x5478, which is per-file and
 * checked before each field - the two exist together and neither is the other.
 */
uint16_t game_fwrite(const uint8_t * ptr, uint16_t size, uint16_t count,
                     FILE *file)
{
    uint16_t n;

    if (((uint16_t)DG547A.archive_count) != 0) {
        struct game_file *entry = archive_entry_for(file);

        if (entry != GAME_FILE_NONE) {
            if (entry->stream_ptr != 0)
                n = borland_fwrite(ptr, size, count,
                              FILEREC_PTR(entry->stream_ptr));
            else
                n = 0;

            DG5677.failures |= (uint16_t)(n != count ? 1 : 0);
            return n;
        }
    }

    n = borland_fwrite(ptr, size, count, file);

    DG5677.failures |= (uint16_t)(n != count ? 1 : 0);
    return n;
}

/*
 * 0x09571
 *
 * **`fputc`, through the archive layer** - `game_fwrite`'s shape for one
 * byte. With the archive in use at DGROUP 0x547e an entry writes through the
 * handle at its +0x10, and an entry without one writes nothing and answers
 * -1; a file that is not an entry, and everything when the archive is not in
 * use, goes to `borland_fputc` directly. The answer is `fputc`'s, and the
 * failure mark at 0x567b is or-ed with "it was -1", once, on every path -
 * the original computes that at one exit and this does it on each, which is
 * the same word either way.
 *
 * **Nothing the port runs reaches it.** Its two callers in the image are the
 * resource writer: `open_resource`'s write branch, which is a stub here, and
 * the byte writer at 0x1c5f5, which is not transcribed. The game never writes
 * a resource in play, so this is read rather than measured, and its spec will
 * say "never called".
 */
int16_t game_fputc(int16_t c, FILE *file)
{
    int16_t n;

    if (((uint16_t)DG547A.archive_count) != 0) {
        struct game_file *entry = archive_entry_for(file);

        if (entry != GAME_FILE_NONE) {
            if (entry->stream_ptr != 0)
                n = borland_fputc(c, FILEREC_PTR(entry->stream_ptr));
            else
                n = -1;

            DG5677.failures |= (uint16_t)(n == -1 ? 1 : 0);
            return n;
        }
    }

    n = borland_fputc(c, file);

    DG5677.failures |= (uint16_t)(n == -1 ? 1 : 0);
    return n;
}

/*
 * 0x095cf
 *
 * Give a file a buffer, whether it is a loose file or one inside the archive.
 *
 * The difference is **which file gets buffered**, not how. A file the archive
 * knows about has a record from `archive_entry_for`, and that record's +0x10
 * is the handle of the archive it lives in - so the buffer goes on the archive
 * rather than on the caller's own handle, which is the one that will actually
 * be read from. A loose file, or a record with no archive behind it, gets the
 * buffer on itself.
 *
 * DGROUP 0x547e is whether the archive is in use at all; with it clear the
 * lookup is skipped.
 */
void game_setbuf(FILE *file, uint8_t *buf)
{
    struct game_file *rec = GAME_FILE_NONE;

    if (((uint16_t)DG547A.archive_count) != 0)
        rec = archive_entry_for(file);

    if (rec == GAME_FILE_NONE) {
        borland_setbuf(file, buf);
        return;
    }

    if (rec->stream_ptr != 0)
        borland_setbuf(FILEREC_PTR(rec->stream_ptr), buf);
}

/*
 * 0x09f68
 *
 * **`stricmp` over two far strings.** A null pointer on either side answers 1
 * rather than crashing, and answers it *before* looking at the other, so two
 * nulls compare as unequal too.
 *
 * Each byte goes through `tolower` before it is compared, which is the whole
 * reason this exists rather than `strcmp`: the listing it sorts holds names DOS
 * hands back in capitals and text the game wrote in whatever case it liked.
 *
 * The loop ends on the *first* string's NUL, so the answer for a prefix is the
 * second string's next character negated - and the two pointers are advanced in
 * the caller's own stack slots, not in registers.
 */
int16_t far_stricmp(const char far * a, const char far * b)
{
    int16_t si, di;

    /* **0000:0000, not a C null pointer.** The original's guard is
       `(off | seg) == 0`, and that address is the first byte of the guest's
       memory - written as `b == NULL` it would never fire. */
    if (b == (const char far *)FAR_NULL_PTR
        || a == (const char far *)FAR_NULL_PTR)
        return 1;

    for (;;) {
        si = (int16_t)to_lower((uint8_t)*a++);
        di = (int16_t)to_lower((uint8_t)*b++);

        if (si == 0 || si != di)
            return (int16_t)(si - di);
    }
}

/*
 * 0x0a62c
 *
 * **Put back everything saved for one page and size**, then give the records
 * away.
 *
 * The slot comes from `find_saved_rect_slot`, and an empty slot or an empty
 * list does nothing at all - not even the page switch below. The first record
 * sets the copy's source and destination pages, at DGROUP 0x38a6 and 0x38a8,
 * from its own +8 and +0xa; every record after it is restored between the same
 * two pages, so a list is only ever built for one pair.
 *
 * Each record is restored one of two ways, by the kind at +0xc:
 *
 *   1  a page-to-page copy of the rectangle, through `copy_rect_thunk`
 *   4  a restore from a saved buffer, through `restore_rect_thunk`, the far
 *      pointer at +0x14
 *
 * and any other kind is skipped in silence, which is how a record can be
 * parked in the list without being drawn.
 *
 * **x and width are in bytes and y and height in pixels.** The two `<< 3`s
 * turn +0 and +4 into pixels for the copy; +2 and +6 are passed through as
 * they stand. That asymmetry is the planar layout showing through - a byte is
 * eight pixels across and one pixel down.
 *
 * The whole chain then goes onto the free list at 0x56e0 in one splice, using
 * the last record the walk saw rather than walking it again.
 */
void restore_saved_rects(dg_seg_t page_src, dg_seg_t page_dst, uint16_t refcount)
{
    dg_near_t *slot = find_saved_rect_slot(page_src, page_dst, refcount);
    struct rect_list_entry *last = RECTENT_NONE;
    struct rect_list_entry *rec;

    if (slot == NULL)
        return;

    rec = RECTENT_PTR(*slot);
    if (rec == RECTENT_NONE)
        return;

    VMDS.page_src_ptr = rec->page_src;
    VMDS.page_dst_ptr = rec->page_dst;

    while (rec != RECTENT_NONE) {
        int16_t x  = (int16_t)(rec->x << 3);
        int16_t rw = (int16_t)(rec->w << 3);

        if (rec->mode == 1)
            copy_rect_thunk((uint16_t)x, (uint16_t)rec->y,
                            (uint16_t)rw, (uint16_t)rec->h);
        else if (rec->mode == 4)
            restore_rect_thunk(rec->buf,
                               rec->x, rec->y,
                               rec->w,
                               rec->h);

        last = rec;
        rec = RECTENT_PTR(rec->next_ptr);
    }

    last->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
    MACHINE_RECT_FREE.rect_free_ptr = *slot;
    *slot = 0;
}

/*
 * 0x09fc0
 *
 * **Borland's `_fstrchr`**, linked in and never called - nothing in the image
 * reaches it by call or by address. A null pointer answers 0000:0000, the
 * guest's null, which is `FAR_NULL_PTR` here and not a C null; searching for
 * NUL itself answers null too, because the loop stops at the terminator and
 * the test after it is for a non-NUL byte.
 */
char far *far_strchr(const char far *s, char c)
{
    if (s == (const char far *)FAR_NULL_PTR)
        return (char far *)FAR_NULL_PTR;

    while (*s != 0 && *s != c)
        s++;

    if (*s != 0)
        return (char far *)s;
    return (char far *)FAR_NULL_PTR;
}

/*
 * 0x0a005
 *
 * **Borland's `_fstrcat`**, linked in and never called. Either pointer null
 * answers null, the source first; the destination is walked to its NUL in a
 * frame copy of the pointer, and the copy includes the terminator. Answers the
 * destination it was given.
 */
char far *far_strcat(char far *dst, const char far *src)
{
    char far *d = dst;                       /* [bp-4]:[bp-2] */
    char c;

    if (src == (const char far *)FAR_NULL_PTR
        || dst == (char far *)FAR_NULL_PTR)
        return (char far *)FAR_NULL_PTR;

    while (*d != 0)
        d++;

    do {
        c = *src++;
        *d++ = c;
    } while (c != 0);

    return dst;
}

/*
 * 0x0a05f
 *
 * **Grow the saved-rect pool** by `n` records, rounded up to a multiple of
 * five, in one `heap_calloc_far(n, 0x1a)` block threaded through `next` and
 * pushed whole on the free list. Only the block's **first** record has
 * `block_head` set - that is what `free_rect_pool` tests to hand the block
 * back in one `heap_free_far`. The count at 0x56b6 goes up by `n`. Answers 1,
 * or 0 when the heap refuses; the `cmp di, 5` before the 1 compares and then
 * ignores the result, and is not reproduced. Called only from
 * `file_saved_rect`, which nothing calls: dead in the shipped binary.
 */
uint16_t build_rect_pool(uint16_t n)
{
    struct rect_list_entry *base;            /* [bp-2] */
    uint16_t k;                              /* [bp-4] */
    struct rect_list_entry *rec;             /* si */

    n = (uint16_t)((int16_t)(n + 4) / 5 * 5);
    base = (struct rect_list_entry *)(void *)heap_calloc_far(n, sizeof(struct rect_list_entry));
    if (base == NULL)
        return 0;

    rec = base;
    rec->block_head = 1;
    for (k = 1; (int16_t)k < (int16_t)n; k++) {
        rec->next_ptr = dg_near(dgroup, rec + 1);
        rec = rec + 1;
    }

    rec->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
    MACHINE_RECT_FREE.rect_free_ptr = dg_near(dgroup, base);
    MACHINE_RECT_COUNT.count = (uint16_t)(MACHINE_RECT_COUNT.count + n);
    return 1;
}

/*
 * 0x0a0d7
 *
 * **File a saved rectangle on its slot** - the one routine that creates a
 * `rect_list_entry`, and **dead in the shipped binary**: its one caller is
 * `copy_saved_rects`, which nothing calls. Ten words of arguments, and they
 * are the record's fields in order, which is how the record was typed.
 *
 * A mode-4 rect (restored from `buf`) is filed under source page -1. A mode-1
 * rect (a plain copy) is first cut to the clip box when `VMDS.clip_enabled`
 * says so, or to the screen otherwise - a rect wholly outside either is
 * dropped - and then `x` and `w` become eight-pixel columns, `w` widened by
 * whatever `x` lost to the rounding. A rect with nothing left is dropped.
 *
 * The record comes off the free list, five more being built when it is
 * empty. Then, for mode 1 only, **it is merged with any rect already on the
 * chain whose union costs little**: for each, the union box and its area are
 * worked out, and if the two areas plus 0x14 cover the union, the new rect
 * becomes the union, the old one is unlinked and returned to the free list,
 * and the walk restarts from the head, stopping where it had got to - the
 * `stop` and `from` slots below. Finally the record goes on the head of the
 * chain. The walk is written with the original's own jumps, because its
 * restart has no tidier spelling that is provably the same.
 */
void file_saved_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t mode, dg_seg_t page_src, dg_seg_t page_dst,
                     uint16_t refcount, uint8_t far * buf)
{
    dg_near_t *slot;                 /* [bp-2] */
    uint16_t stop, prev, after, from;        /* [bp-4] [bp-6] [bp-8] [bp-0xa] */
    int16_t  area, sum, ux0, ux1, uy0, uy1;  /* [bp-0xc] .. [bp-0x16] */
    uint16_t rec, other;                     /* si, di */

    if (mode == 4)
        page_src = 0xffff;

    slot = find_saved_rect_slot(page_src, page_dst, refcount);
    if (slot == NULL)
        return;

    if (mode == 1) {
        if (VMDS.clip_enabled != 0) {
            if (x > VMDS.clip_right
                || (int16_t)(x + w) < VMDS.clip_left
                || y > VMDS.clip_bottom
                || (int16_t)(y + h) < VMDS.clip_top)
                return;

            if (x < VMDS.clip_left) {
                w = (int16_t)(w - (VMDS.clip_left - x));
                x = VMDS.clip_left;
            }
            if (y < VMDS.clip_top) {
                h = (int16_t)(h - (VMDS.clip_top - y));
                y = VMDS.clip_top;
            }
            if ((int16_t)(x + w - 1) > VMDS.clip_right)
                w = (int16_t)(VMDS.clip_right - x + 1);
            if ((int16_t)(y + h - 1) > VMDS.clip_bottom)
                h = (int16_t)(VMDS.clip_bottom - y + 1);
        } else {
            if ((int16_t)(VMDS.screen.screen_width - 1) < x
                || (int16_t)(x + w) < 0
                || (int16_t)(VMDS.screen.screen_height - 1) < y
                || (int16_t)(y + h) < 0)
                return;

            if (x < 0) {
                w = (int16_t)(w - (0 - x));
                x = 0;
            }
            if (y < 0) {
                h = (int16_t)(h - (0 - y));
                y = 0;
            }
            if ((int16_t)(x + w - 1) > (int16_t)(VMDS.screen.screen_width - 1))
                w = (int16_t)(VMDS.screen.screen_width - 1 - x + 1);
            if ((int16_t)(y + h - 1) > (int16_t)(VMDS.screen.screen_height - 1))
                h = (int16_t)(VMDS.screen.screen_height - 1 - y + 1);
        }

        w = (int16_t)((w + x % 8 + 7) / 8);
        x = (int16_t)(x / 8);
    }

    if (w == 0 || h == 0)
        return;

    if (MACHINE_RECT_FREE.rect_free_ptr == 0 && build_rect_pool(5) == 0)
        return;

    rec = MACHINE_RECT_FREE.rect_free_ptr;
    MACHINE_RECT_FREE.rect_free_ptr = RECTENT_PTR(rec)->next_ptr;
    RECTENT_PTR(rec)->next_ptr = 0;
    RECTENT_PTR(rec)->x = x;
    RECTENT_PTR(rec)->y = y;
    RECTENT_PTR(rec)->w = w;
    RECTENT_PTR(rec)->h = h;
    RECTENT_PTR(rec)->mode = mode;
    RECTENT_PTR(rec)->page_src = page_src;
    RECTENT_PTR(rec)->page_dst = page_dst;
    RECTENT_PTR(rec)->refcount = refcount;
    RECTENT_PTR(rec)->buf = (buf);
    RECTENT_PTR(rec)->area = (uint16_t)(w * h);

    if (mode == 1) {
        other = *slot;
        stop = 0;
        from = 0;
        prev = 0;
        goto check;

    body:
        stop = from;
        after = RECTENT_PTR(other)->next_ptr;
        sum = (int16_t)(RECTENT_PTR(other)->area + RECTENT_PTR(rec)->area);

        ux0 = RECTENT_PTR(other)->x < RECTENT_PTR(rec)->x
              ? RECTENT_PTR(other)->x : RECTENT_PTR(rec)->x;
        ux1 = (int16_t)(RECTENT_PTR(other)->x + RECTENT_PTR(other)->w)
                  > (int16_t)(RECTENT_PTR(rec)->x + RECTENT_PTR(rec)->w)
              ? (int16_t)(RECTENT_PTR(other)->x + RECTENT_PTR(other)->w)
              : (int16_t)(RECTENT_PTR(rec)->x + RECTENT_PTR(rec)->w);
        uy0 = RECTENT_PTR(other)->y < RECTENT_PTR(rec)->y
              ? RECTENT_PTR(other)->y : RECTENT_PTR(rec)->y;
        uy1 = (int16_t)(RECTENT_PTR(other)->y + RECTENT_PTR(other)->h)
                  > (int16_t)(RECTENT_PTR(rec)->y + RECTENT_PTR(rec)->h)
              ? (int16_t)(RECTENT_PTR(other)->y + RECTENT_PTR(other)->h)
              : (int16_t)(RECTENT_PTR(rec)->y + RECTENT_PTR(rec)->h);
        area = (int16_t)((ux1 - ux0) * (uy1 - uy0));

        if ((int16_t)(sum + 0x14) < area)
            goto advance;

        RECTENT_PTR(rec)->x = ux0;
        RECTENT_PTR(rec)->y = uy0;
        RECTENT_PTR(rec)->w = (int16_t)(ux1 - ux0);
        RECTENT_PTR(rec)->h = (int16_t)(uy1 - uy0);
        RECTENT_PTR(rec)->area = (uint16_t)area;
        if (prev != 0)
            RECTENT_PTR(prev)->next_ptr = after;
        else
            *slot = after;
        RECTENT_PTR(other)->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
        MACHINE_RECT_FREE.rect_free_ptr = other;
        from = after;
        stop = prev;
        other = prev;

    advance:
        prev = other;
        other = after;
        if (other == 0 && stop != 0) {
            other = *slot;
            prev = 0;
        }

    check:
        if (other != stop)
            goto body;
    }

    RECTENT_PTR(rec)->next_ptr = *slot;
    *slot = rec;
}

/*
 * 0x0a42a
 *
 * Put back the saved rectangles for **a list of page-and-size pairs**, and
 * then, on one of the two paths, take one off every remaining record's +0xe.
 *
 * The argument picks which table to walk: non-zero takes the one at DGROUP
 * 0x2d0e and **stops after a single entry**, zero takes the one at 0x2d0a and
 * walks it until an entry whose second word is null. The two share the loop,
 * and the test at the bottom is what makes one of them a loop and the other a
 * single pass.
 *
 * Each entry is two near pointers, four bytes apart, to the driver's page
 * words at 0x38a0..0x38a4; the segments read *through* them are handed to
 * `restore_saved_rects` as the source and destination page, with a refcount
 * of zero.
 *
 * The copy's source and destination pages, 0x38a6 and 0x38a8, are saved on the
 * way in and put back at the end, because `restore_saved_rects` sets them from
 * the first record it finds and would otherwise leave them wherever the last
 * list went.
 *
 * The count pass only runs on the zero path, over all twenty slots at 0x56b8
 * and every record on each chain. It decrements each record's `refcount` -
 * the word at +0xe, which `find_saved_rect_slot` matches for equality and
 * the creator at 0x0a0d7 files from its eighth argument. An earlier reading
 * here had the two disagreeing about what the word was; the creator settles
 * it.
 */
void restore_saved_rect_lists(int16_t which)
{
    uint16_t saved_src = VMDS.page_src_ptr;
    uint16_t saved_dst = VMDS.page_dst_ptr;
    uint16_t i = which != 0 ? 1 : 0;              /* the pair to start from */

    for (;;) {
        const dg_seg_t *src = (const dg_seg_t *)dg_near_ptr(MACHINE_PAGE_PAIRS.pair[i].src);
        const dg_seg_t *dst = (const dg_seg_t *)dg_near_ptr(MACHINE_PAGE_PAIRS.pair[i].dst);

        restore_saved_rects(*src, *dst, 0);
        i++;

        if (which != 0)
            break;
        if (MACHINE_PAGE_PAIRS.pair[i].dst == 0)
            break;
    }

    VMDS.page_src_ptr = saved_src;
    VMDS.page_dst_ptr = saved_dst;

    if (which != 0)
        return;

    {
        dg_near_t *slot = &MACHINE_RECT_SLOTS.slot[0];
        int16_t  left = 0x14;

        while (left != 0) {
            uint16_t rec = *slot;

            while (rec != 0) {
                RECTENT_PTR(rec)->refcount =
                    (int16_t)(RECTENT_PTR(rec)->refcount - 1);
                rec = RECTENT_PTR(rec)->next_ptr;
            }

            slot++;
            left--;
        }
    }
}

/*
 * 0x0a4bf
 *
 * **Discard every saved rect**: each of the twenty slots' chains is walked to
 * its last record, which is pointed at the free list, and the whole chain is
 * then the free list and the slot is empty. Reached only from
 * `free_rect_pool`, which nothing calls: dead in the shipped binary.
 */
void discard_saved_rects(void)
{
    dg_near_t *slot = &MACHINE_RECT_SLOTS.slot[0];
    int16_t  left = 0x14;
    uint16_t rec;

    while (left != 0) {
        rec = *slot;
        if (rec != 0) {
            while (RECTENT_PTR(rec)->next_ptr != 0)
                rec = RECTENT_PTR(rec)->next_ptr;
            RECTENT_PTR(rec)->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
            MACHINE_RECT_FREE.rect_free_ptr = *slot;
            *slot = 0;
        }
        slot++;
        left--;
    }
}

/*
 * 0x0a4f9
 *
 * **Is a box covered by a saved mode-1 rect** on the slot whose head carries
 * this destination page and refcount? The box's `x` and `w` are turned into
 * eight-pixel columns the way `file_saved_rect` turns them, then every slot's
 * head is tested and the matching chain walked; the first mode-1 rect that
 * overlaps answers its mode, which is 1, and nothing answers 0. Nothing in
 * the image calls it: dead in the shipped binary.
 */
uint16_t saved_rect_covers(int16_t x, int16_t y, int16_t w, int16_t h,
                           dg_seg_t page_dst, uint16_t refcount)
{
    dg_near_t *slot = &MACHINE_RECT_SLOTS.slot[0];         /* [bp-2] */
    int16_t  left = 0x14;                              /* [bp-4] */
    int16_t  cols = (int16_t)((w + x % 8 + 7) / 8);    /* cx */
    uint16_t rec;                                      /* si */

    x = (int16_t)(x / 8);                              /* di */

    while (left != 0) {
        rec = *slot;
        if (rec != 0
            && RECTENT_PTR(rec)->page_dst == page_dst
            && (uint16_t)RECTENT_PTR(rec)->refcount == refcount) {
            for (; rec != 0; rec = RECTENT_PTR(rec)->next_ptr) {
                if (RECTENT_PTR(rec)->mode != 1)
                    continue;
                if (RECTENT_PTR(rec)->x < (int16_t)(x + cols)
                    && (int16_t)(RECTENT_PTR(rec)->x + RECTENT_PTR(rec)->w) > x
                    && RECTENT_PTR(rec)->y < (int16_t)(y + h)
                    && (int16_t)(RECTENT_PTR(rec)->y + RECTENT_PTR(rec)->h) > y)
                    return RECTENT_PTR(rec)->mode;
            }
        }
        slot++;
        left--;
    }
    return 0;
}

/*
 * 0x0a5a1
 *
 * **Free the saved-rect pool.** Everything is discarded onto the free list
 * first, and then the list is walked for a block head - the first record of
 * each `heap_calloc_far` block, marked by `build_rect_pool`. On finding one
 * the mark is cleared, **the routine calls itself** - which walks on past
 * the cleared mark and frees every later block first - and then this block
 * is freed in one `heap_free_far` and the list head zeroed. That recursion
 * is the original's, at 0x0a5bc, and is kept. Nothing in the image calls
 * this: dead in the shipped binary.
 */
void free_rect_pool(void)
{
    uint16_t rec;

    discard_saved_rects();

    for (rec = MACHINE_RECT_FREE.rect_free_ptr; rec != 0; rec = RECTENT_PTR(rec)->next_ptr) {
        if ((RECTENT_PTR(rec)->block_head & 1) != 0) {
            RECTENT_PTR(rec)->block_head = 0;
            free_rect_pool();
            heap_free_far(dg_near_ptr(rec));
            break;
        }
    }

    MACHINE_RECT_FREE.rect_free_ptr = 0;
}

/*
 * 0x0a5d8
 *
 * How many records the pool holds. Nothing in the image calls it.
 */
uint16_t rect_pool_count(void)
{
    return MACHINE_RECT_COUNT.count;
}

/*
 * 0x0a5e2
 *
 * Find the slot in the table of **twenty saved-rectangle objects** at DGROUP
 * 0x56b8 that already holds a given page, width and height - or, failing that,
 * the first empty slot.
 *
 * Each slot is a word: a near pointer to a record, or zero. A record matches
 * when its page at +0xe, its width at +8 and its height at +0xa are all the
 * ones asked for. The answer is **the slot**, not the record, so a caller can
 * put a new record into it.
 *
 * The first empty slot is remembered as the walk goes past it - `or dx,dx`
 * keeps the *first* one rather than the last - and is what comes back when
 * nothing matched. A full table with no match answers 0, which is also what an
 * empty slot's own contents look like, so the two are told apart by the caller
 * looking at what the slot holds rather than by the answer.
 */
dg_near_t *find_saved_rect_slot(dg_seg_t page_src, dg_seg_t page_dst,
                                        uint16_t refcount)
{
    dg_near_t *slot  = &MACHINE_RECT_SLOTS.slot[0];
    dg_near_t *empty = NULL;
    int16_t  left  = 0x14;

    while (left != 0) {
        uint16_t rec = *slot;

        if (rec == 0) {
            if (empty == NULL)
                empty = slot;
        } else if ((uint16_t)RECTENT_PTR(rec)->refcount == refcount
                   && RECTENT_PTR(rec)->page_src == page_src
                   && RECTENT_PTR(rec)->page_dst == page_dst) {
            return slot;
        }

        slot++;
        left--;
    }

    return empty;
}

/*
 * 0x0a6d7
 *
 * Give back every saved rectangle held for one page and size: find the slot,
 * walk its chain of records to the end through the links at +0x18, and put the
 * whole chain onto the free list at DGROUP 0x56e0 in one move rather than one
 * record at a time. The slot is then cleared.
 *
 * A slot that does not exist, or holds nothing, is left alone. The list is
 * pushed on the front, so the freed records come back in the reverse of the
 * order they were taken - which nothing depends on, but it is what happens.
 */
void free_saved_rects(dg_seg_t page_src, dg_seg_t page_dst, uint16_t refcount)
{
    dg_near_t *slot = find_saved_rect_slot(page_src, page_dst, refcount);
    uint16_t rec, last;

    if (slot == NULL)
        return;

    rec = *slot;
    if (rec == 0)
        return;

    last = rec;
    while (RECTENT_PTR(last)->next_ptr != 0)
        last = RECTENT_PTR(last)->next_ptr;

    RECTENT_PTR(last)->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
    MACHINE_RECT_FREE.rect_free_ptr = *slot;
    *slot = 0;
}

/*
 * 0x0a717
 *
 * **Copy one slot's saved rects onto another**: the two slots are found by
 * their page pairs and refcounts, and if they differ and the first has a
 * chain, each rect on it is filed again - `x` and `w` back from columns to
 * pixels - under the second's page pair and refcount, with no buffer.
 *
 * **As compiled, the loop never advances.** Its step, `mov si, [si+0x18]` at
 * 0x0a787, sits after the exit test at 0x0a783 and is reached only once `si`
 * is already 0, so a non-empty chain re-files its first rect for ever. That
 * is transcribed as it is, because nothing in the image calls this routine -
 * no near or far call, no occurrence of its address as data, and the code
 * map from the entry point never reaches it - so no run ever met the defect.
 */
void copy_saved_rects(dg_seg_t from_src, dg_seg_t from_dst, uint16_t from_ref,
                      dg_seg_t to_src, dg_seg_t to_dst, uint16_t to_ref)
{
    dg_near_t *from_slot, *to_slot;
    uint16_t rec;

    from_slot = find_saved_rect_slot(from_src, from_dst, from_ref);   /* di */
    to_slot   = find_saved_rect_slot(to_src, to_dst, to_ref);         /* ax */

    if (to_slot == from_slot)
        return;
    if (from_slot == NULL || *from_slot == 0)
        return;

    rec = *from_slot;
    while (rec != 0) {
        file_saved_rect((int16_t)(RECTENT_PTR(rec)->x << 3), RECTENT_PTR(rec)->y,
                        (int16_t)(RECTENT_PTR(rec)->w << 3), RECTENT_PTR(rec)->h,
                        RECTENT_PTR(rec)->mode, to_src, to_dst, to_ref,
                        FAR_NULL_PTR);
        /* no step: see above */
    }
}

/*
 * 0x0a78e
 *
 * Let the cursor follow the mouse again, and redraw it where the mouse now is.
 * The pair to `cursor_redraw_off` three instructions below: DGROUP 0x2d44 is what
 * `redraw_cursor` tests before it asks the driver for the position, so clearing
 * it pins the cursor and setting it releases it.
 */
void cursor_redraw_on(void)
{
    MACHINE_CURSOR_STATE.timer_draws_cursor = 1;
    redraw_cursor(VMDS.page_front_ptr);
}

/*
 * 0x0a7ae
 *
 * What the timer calls, four ticks in five: read the keyboard and the mouse,
 * move the pointer, and release the frame.
 *
 * It refuses to run at all when DGROUP 0x5752 is above 1 or 0x5740 is already
 * set - the first is the cursor's nesting guard and the second is this routine
 * being in progress - so a redraw cannot be interrupted by the tick that would
 * start another.
 *
 * The eight scan codes it reads are the keypad's: 0x47 0x48 0x49 across the
 * top, 0x4b 0x4d either side, 0x4f 0x50 0x51 across the bottom. Any of the top
 * three moves up, any of the bottom three down, and the corners count for both
 * of their directions - which is what makes the diagonals work. Two pixels a
 * tick, clamped to the screen, and the clamp is against the *hot spot* rather
 * than the pointer's own position.
 *
 * Then Enter, Space, keypad 5 and Insert are all the same button - `si` ends up
 * 1 if any of them is down - and are ORed with the real one. `button_state`
 * turns each into a state, and the accumulators at 0x5768 and 0x576a keep it
 * until the next frame reads them.
 *
 * The last two lines are the ones everything waits on: 0x5740 is cleared and
 * **0x5754 is set**, which is the flag `wait_and_latch_frame` spins on.
 */
void timer_callback(void)
{
    int16_t moved = 0;
    int16_t k_end, k_down, k_pgdn, k_left, k_right, k_home, k_up, k_pgup;
    int16_t si, di;

    if (((int16_t)DG5752.guard) > 1 || MACHINE_PALETTE_FADE.busy != 0)
        return;

    MACHINE_PALETTE_FADE.busy = 1;

    k_end   = key_is_down(SC_END);
    k_down  = key_is_down(SC_DOWN);
    k_pgdn  = key_is_down(SC_PGDN);
    k_left  = key_is_down(SC_LEFT);
    k_right = key_is_down(SC_RIGHT);
    k_home  = key_is_down(SC_HOME);
    k_up    = key_is_down(SC_UP);
    k_pgup  = key_is_down(SC_PGUP);

    if (k_home != 0 || k_up != 0 || k_pgup != 0) {
        moved = 1;
        DG5768.cursor_y = (int16_t)(DG5768.cursor_y - 2);
        if (DG5768.cursor_y - DG5768.hot_y < 0)
            DG5768.cursor_y = 0;
    }

    if (k_end != 0 || k_down != 0 || k_pgdn != 0) {
        moved = 1;
        DG5768.cursor_y = (int16_t)(DG5768.cursor_y + 2);
        if (DG5768.cursor_y - DG5768.hot_y > (int16_t)(VMDS.screen.screen_height - 1))
            DG5768.cursor_y = (int16_t)(VMDS.screen.screen_height - 1);
    }

    if (k_end != 0 || k_left != 0 || k_home != 0) {
        moved = 1;
        DG5768.cursor_x = (int16_t)(DG5768.cursor_x - 2);
        if (DG5768.cursor_x - DG5768.hot_x < 0)
            DG5768.cursor_x = 0;
    }

    if (k_pgdn != 0 || k_right != 0 || k_pgup != 0) {
        moved = 1;
        DG5768.cursor_x = (int16_t)(DG5768.cursor_x + 2);
        if (DG5768.cursor_x - DG5768.hot_x > (int16_t)(VMDS.screen.screen_width - 1))
            DG5768.cursor_x = (int16_t)(VMDS.screen.screen_width - 1);
    }

    if (moved != 0)
        mouse_move_to(((uint16_t)DG5768.cursor_x), ((uint16_t)DG5768.cursor_y));

    if (MACHINE_CURSOR_STATE.timer_draws_cursor != 0 && DG5752.guard == 0) {
        isr_stack_switch(1);
        redraw_cursor(VMDS.page_front_ptr);
        isr_stack_switch(0);
    }

    di = read_mouse_button(0);

    si = (key_is_down(SC_SPACE) != 0 || key_is_down(SC_ENTER) != 0
          || key_is_down(SC_KP5) != 0 || key_is_down(SC_INS) != 0) ? 1 : 0;

    di |= (si != 0) ? 1 : 0;

    si = button_state(0, di);
    if (si <= 1)
        si = DG5768.button_accum_b;
    DG5768.button_accum_b = (int16_t)(di | (si & 0xfffe));

    di = (MACHINE_CURSOR_STATE.read_driver != 0 && read_mouse_button(1) != 0) ? 1 : 0;
    di |= key_is_down(1);

    si = button_state(1, di);
    if (si <= 1)
        si = DG5768.button_accum_a;
    DG5768.button_accum_a = (int16_t)(di | (si & 0xfffe));

    MACHINE_PALETTE_FADE.busy = 0;
    DG5752.frame_flag = 1;
}

/*
 * 0x0aa14
 *
 * Choose the mouse cursor: which bitmap, and where its hot spot is. The three
 * are kept at DGROUP 0x5770, 0x5780 and 0x577e, and a call that names what is
 * already showing does nothing at all - not even the redraw.
 *
 * A cursor of 0 means none, and then both hot-spot words are zeroed rather than
 * taking the arguments, so turning the cursor off cannot leave a stale offset
 * behind for the next one.
 *
 * DGROUP 0x5752 is raised over the redraw and put back afterwards. It is not a
 * simple flag: the value it had is *saved*, so a redraw inside a redraw leaves
 * the outer one's state alone when it finishes.
 */
void set_cursor(struct bitmap *bitmap, int16_t hot_x, int16_t hot_y)
{
    uint16_t saved;

    if (BMP_PTR(DG5768.cursor_bitmap_ptr) == bitmap && DG5768.hot_x == hot_x
        && DG5768.hot_y == hot_y)
        return;

    saved = DG5752.guard;
    DG5752.guard = 1;

    DG5768.cursor_bitmap_ptr = dg_near(dgroup, bitmap);

    if (bitmap == BMP_NONE) {
        DG5768.hot_y = 0;
        DG5768.hot_x = 0;
    } else {
        DG5768.hot_x = hot_x;
        DG5768.hot_y = hot_y;
    }

    redraw_cursor(VMDS.page_front_ptr);

    DG5752.guard = saved;
}

/*
 * 0x0aa76
 *
 * Put the pointer somewhere, clamped to the screen, and tell the driver.
 *
 * Each coordinate is pinned to 0 below and to the screen size less one above -
 * the width at DGROUP 0x3f7a and the height at 0x3f7c - so a caller may ask
 * for anything and the pointer stays on the screen. Both bounds are the
 * *game's* idea of the screen, which is why this follows a mode change: the
 * copy-protection screen sets 0x3f7c to 0x18f before it does any of this.
 *
 * The result is written to **two pairs**: 0x5784/0x5782, which is where the
 * game reads the pointer, and 0x576e/0x576c, which is where it remembers it.
 * Then `mouse_move_to` moves the driver's own cursor to match, so the three
 * agree.
 */
void move_pointer_to(int16_t x, int16_t y)
{
    if (x < 0)
        x = 0;
    else if ((int16_t)(VMDS.screen.screen_width - 1) < x)
        x = (int16_t)(VMDS.screen.screen_width - 1);

    if (y < 0)
        y = 0;
    else if ((int16_t)(VMDS.screen.screen_height - 1) < y)
        y = (int16_t)(VMDS.screen.screen_height - 1);

    DG5768.pointer_x = x;
    DG5768.cursor_x = x;
    DG5768.pointer_y = y;
    DG5768.cursor_y = y;

    mouse_move_to((uint16_t)x, (uint16_t)y);
}

/*
 * 0x0ab1f
 *
 * Draw the cursor on a page: put back what was under the last one, save what is
 * under the new one, draw it, and remember where.
 *
 * The page's slot holds both states at once - the *previous* one at +0x14 and
 * the current at +8 - and bits 0 and 1 of the byte at +0x1f say which of them
 * is live. That is what lets the erase happen after the save rather than before
 * it, so the two rectangles can overlap without the erase undoing the save.
 *
 * Clipping is set wide open first: DGROUP 0x3894 and 0x3898 to zero, 0x3896 and
 * 0x389a to the screen's size less one, and both page pointers at 0x38a6 and
 * 0x38a8 to the slot's own page - so the cursor is drawn on that page whichever
 * one is being shown.
 *
 * A slot buffer of zero means "nothing was saved", and then the erase is a
 * single `plot_pixel_clipped` of the byte at +0x1e instead of a rectangle - the
 * one-pixel case, which a saved rectangle would be wasteful for.
 *
 * DGROUP 0x2d3e turns the whole cursor off: with it clear nothing is drawn and
 * bit 1 of +0x13 is cleared instead, which is what `redraw_cursor` tests.
 */
void draw_cursor(uint16_t page)
{
    struct page_slot *slot = claim_page_slot(page);
    uint16_t saved;

    if (slot == PAGESLOT_NONE)
        return;

    saved = DG5752.guard;
    DG5752.guard = 1;

    restage_object_rect(page);
    save_or_restore_draw_state(1);

    VMDS.page_src_ptr = ((int16_t)slot->page);
    VMDS.page_dst_ptr = ((int16_t)slot->page);
    VMDS.clip_enabled = 1;
    VMDS.clip_top = 0;
    VMDS.clip_left = 0;
    VMDS.clip_bottom = (int16_t)(VMDS.screen.screen_height - 1);
    VMDS.clip_right = (int16_t)(VMDS.screen.screen_width - 1);

    /* Put back what the last cursor covered. */
    if ((slot->cursor.flags & 2) != 0) {
        if (slot->cursor.buf != 0) {
            if (slot->cursor.w > 0
                && slot->cursor.h > 0) {
                const uint8_t *b = MACHINE_RECT_BUFFERS.slot[slot->cursor.buf - 1];

                restore_rect_thunk(b,
                                   slot->cursor.x,
                                   slot->cursor.y,
                                   slot->cursor.w,
                                   slot->cursor.h);
            }
        } else {
            plot_pixel_clipped(slot->cursor.x,
                               slot->cursor.y,
                               (int16_t)slot->cursor.pixel);
        }
        slot->cursor.flags =
            (uint8_t)(slot->cursor.flags & 0xfd);
    }

    /* Save what the new one will cover. */
    if (MACHINE_CURSOR_STATE.cursor_off != 0) {
        if (slot->obj.buf != 0
            && slot->bitmap_ptr != 0) {
            if (slot->obj.w > 0
                && slot->obj.h > 0) {
                uint8_t *b = MACHINE_RECT_BUFFERS.slot[slot->obj.buf - 1];

                save_rect_thunk(b,
                                slot->obj.x,
                                slot->obj.y,
                                slot->obj.w,
                                slot->obj.h);
            }
        } else {
            slot->obj.pixel =
                (uint8_t)read_pixel_clipped(slot->obj.x,
                                            slot->obj.y);
        }

        /* And draw it. */
        if (slot->bitmap_ptr != 0
            && slot->obj.buf != 0) {
            int16_t y = slot->y;

            /*
             * On adapter 8 a negative y is nudged one further up before the
             * blit, and the x argument is replaced by zero.
             */
            if (((uint8_t)VMDS.pixel_shift) == 8 && y < 0)
                draw_bitmap(BMP_PTR(slot->bitmap_ptr),
                            slot->x,
                            (int16_t)(y - 1), 0);
            else
                draw_bitmap(BMP_PTR(slot->bitmap_ptr),
                            slot->x, y, 0);
        } else {
            MACHINE_PALETTE_FADE.plot_colour = (int16_t)((MACHINE_PALETTE_FADE.plot_colour + 1) & 0x0f);
            plot_pixel_clipped(slot->x,
                               slot->y,
                               MACHINE_PALETTE_FADE.plot_colour);
        }

        slot->obj.flags =
            (uint8_t)(slot->obj.flags | 2);
    } else {
        slot->obj.flags =
            (uint8_t)(slot->obj.flags & 0xfd);
    }

    save_or_restore_draw_state(0);

    /* Give back the buffer the erase used, if nothing else wants it. */
    if ((slot->cursor.flags & 1) != 0
        && slot->cursor.buf != 0
        && ((uint16_t)MACHINE_PALETTE_FADE.busy) == 0) {
        release_buffer((int16_t)slot->cursor.buf);
        slot->cursor.buf = 0;
        slot->cursor.flags =
            (uint8_t)(slot->cursor.flags & 0xfe);
    }

    DG5752.guard = saved;
}

/*
 * 0x0acc3
 *
 * Redraw the cursor on a page, if anything about it has changed.
 *
 * The page's slot comes from `claim_page_slot`; a page with no slot is not
 * drawn on at all. Then the mouse's position is read into DGROUP 0x576e and
 * 0x576c - but only when DGROUP 0x2d42 says to, so a caller that has already
 * decided where the cursor goes can suppress it - and the hot spot is
 * subtracted to give the top-left corner at 0x56e2 and 0x56e4.
 *
 * The redraw is then skipped when all four of the slot's remembered values
 * still agree with what was just worked out **and** bit 1 of the slot's byte at
 * +0x13 is set. A cursor of 0 skips the comparison and always redraws, which is
 * how it gets erased.
 *
 * 0x5752 is raised across the whole thing and restored, the same nesting guard
 * `set_cursor` uses.
 */
void redraw_cursor(uint16_t page)
{
    struct page_slot *slot = claim_page_slot(page);
    uint16_t saved;

    if (slot == PAGESLOT_NONE)
        return;

    saved = DG5752.guard;
    DG5752.guard = 1;

    if (MACHINE_CURSOR_STATE.read_driver != 0)
        read_mouse_pointer(&DG5768.cursor_x, &DG5768.cursor_y);

    MACHINE_RECT_FREE.draw_x = (int16_t)(DG5768.cursor_x - DG5768.hot_x);
    MACHINE_RECT_FREE.draw_y = (int16_t)(DG5768.cursor_y - DG5768.hot_y);

    if (DG5768.cursor_bitmap_ptr == 0
        || slot->x != MACHINE_RECT_FREE.draw_x
        || slot->y != MACHINE_RECT_FREE.draw_y
        || slot->bitmap_ptr != DG5768.cursor_bitmap_ptr
        || (slot->obj.flags & 2) == 0)
        draw_cursor(page);

    DG5752.guard = saved;
}

/*
 * 0x0b078
 *
 * **Redraw the cursor**, and with it everything the cursor was standing on.
 *
 * This is what the whole saved-rectangle machinery exists for. The cursor is
 * drawn over the picture, so before it can move, what it covered has to go
 * back - and because the game is double buffered, on both pages, in an order
 * that never leaves either half restored.
 *
 * The re-entry guard at DGROUP 0x5752 is raised for the whole routine and the
 * *entering* value put back at the end, not a zero, so a call from inside a
 * call leaves the flag as it found it.
 *
 * The order, which is the substance of it:
 *
 *   1  a pending move at 0x577a/0x577c is taken first and cleared, so the
 *      pointer is where it is going before anything is drawn. Either word
 *      being non-zero is enough to trigger it.
 *   2  the cursor is erased from the drawing page.
 *   3  when the pages differ, `show_page_thunk` is told whether nothing else
 *      is pending - no palette waiting at 0x2d3a and the fade already where
 *      0x5786 asks - so it can wait for retrace only when it is worth it.
 *   4  a palette waiting at 0x2d3a/0x2d3c is loaded, remembered at
 *      0x5738/0x573a, and **cleared**, so one request loads once. Loading one
 *      also resets 0x573c to zero, which forces the fade below to run.
 *   5  the fade runs only when 0x5786 differs from 0x573c, and 0x573c is then
 *      caught up.
 *   6  if 0x2d34 says the screen was disturbed, the saved rectangles for both
 *      pages and for the copy pair are given back, the whole screen is copied
 *      between pages, and the objects are moved or erased. Otherwise only the
 *      drawing page's objects are erased.
 *   7  when the pages are the same, the cursor goes back on, the two pages'
 *      object lists are swapped, each page's own saved rectangle is copied
 *      back through its slot, and the backdrop is restored between them.
 *   8  and whatever page it took, the lists of lists at 0x2d0a are put back.
 *
 * The `add sp, 6` after each `free_saved_rects` is the caller cleaning three
 * arguments; the middle call passes 0x2d32 as the page, where the other two
 * pass zero.
 */
void redraw_cursor_all(void)
{
    uint16_t was = DG5752.guard;

    DG5752.guard = 1;

    if (DG5768.pending_move_x != 0 || DG5768.pending_move_y != 0) {
        move_pointer_to((int16_t)DG5768.pending_move_x, (int16_t)DG5768.pending_move_y);
        DG5768.pending_move_y = 0;
        DG5768.pending_move_x = 0;
    }

    draw_cursor(VMDS.page_back_ptr);

    if (MACHINE_CURSOR_STATE.page != 0) {
        uint16_t quiet =
            (MACHINE_CURSOR_STATE.pending_pal == FAR_NULL_PTR
             && DG5768.fade_weight == MACHINE_PALETTE_FADE.fade_mark) ? 1 : 0;

        show_page_thunk(quiet);
    }

    if (MACHINE_CURSOR_STATE.pending_pal != FAR_NULL_PTR) {
        set_palette_pointer(MACHINE_CURSOR_STATE.pending_pal);
        MACHINE_PALETTE_FADE.request = MACHINE_CURSOR_STATE.pending_pal;
        MACHINE_CURSOR_STATE.pending_pal = NULL;
        MACHINE_PALETTE_FADE.fade_mark = 0;
    }

    if (DG5768.fade_weight != MACHINE_PALETTE_FADE.fade_mark) {
        fade_palette_run(MACHINE_CURSOR_STATE.fade_first, MACHINE_CURSOR_STATE.fade_count, 0, DG5768.fade_weight);
        MACHINE_PALETTE_FADE.fade_mark = DG5768.fade_weight;
    }

    if (MACHINE_CURSOR_STATE.screen_disturbed == 0) {
        erase_object(VMDS.page_back_ptr);
    } else {
        if (MACHINE_CURSOR_STATE.page != 0) {
            VMDS.page_src_ptr = VMDS.page_front_ptr;
            VMDS.page_dst_ptr = VMDS.page_back_ptr;
        } else {
            VMDS.page_src_ptr = VMDS.page_back_ptr;
            VMDS.page_dst_ptr = VMDS.page_front_ptr;
        }

        free_saved_rects(VMDS.rect_page, VMDS.page_back_ptr, 0);
        free_saved_rects(VMDS.rect_page, VMDS.page_front_ptr, MACHINE_CURSOR_STATE.page);
        free_saved_rects(VMDS.page_src_ptr, VMDS.page_dst_ptr, 0);

        copy_rect_thunk(0, 0, ((uint16_t)VMDS.screen.screen_width), ((uint16_t)VMDS.screen.screen_height));

        if (MACHINE_CURSOR_STATE.page != 0) {
            restore_object_backdrop(VMDS.page_front_ptr, VMDS.page_back_ptr);
            clear_object_covered(VMDS.page_back_ptr);
        } else {
            erase_object(VMDS.page_back_ptr);
        }

        MACHINE_CURSOR_STATE.screen_disturbed = 0;
    }

    if (MACHINE_CURSOR_STATE.page == 0) {
        struct page_slot *rec;

        clear_object_covered(VMDS.page_front_ptr);
        draw_cursor(VMDS.page_back_ptr);
        swap_page_objects(VMDS.page_front_ptr, VMDS.page_back_ptr);

        VMDS.page_dst_ptr = VMDS.page_front_ptr;
        VMDS.page_src_ptr = VMDS.page_back_ptr;

        rec = claim_page_slot(VMDS.page_front_ptr);
        if (rec != PAGESLOT_NONE)
            copy_rect_thunk(((uint16_t)rec->obj.x),
                            ((uint16_t)rec->obj.y),
                            ((uint16_t)rec->obj.w),
                            ((uint16_t)rec->obj.h));

        rec = claim_page_slot(VMDS.page_back_ptr);
        if (rec != PAGESLOT_NONE)
            copy_rect_thunk(((uint16_t)rec->obj.x),
                            ((uint16_t)rec->obj.y),
                            ((uint16_t)rec->obj.w),
                            ((uint16_t)rec->obj.h));

        restore_object_backdrop(VMDS.page_front_ptr, VMDS.page_back_ptr);
    }

    restore_saved_rect_lists(0);

    DG5752.guard = was;
}

/*
 * 0x0b28e
 *
 * Copy a rectangle from the page on screen to the page being drawn to, with
 * the pointer out of the way.
 *
 * Both pages are asked whether their object - the mouse pointer - overlaps the
 * rectangle, and the two answers decide what has to be taken down and put back.
 * The usual way is: erase the pointer from the page being drawn to, copy, put
 * the shown page's backdrop back if it was covered, and draw the pointer again.
 *
 * There is a second way, taken only when DGROUP 0x2d32 is clear *and* the drawn
 * page's pointer is in the way: draw the pointer on the shown page first, copy,
 * and erase it from the shown page afterwards - so the copy carries the pointer
 * across rather than working around it.
 *
 * A rectangle with no width or no height is not copied, but everything else
 * still happens. DGROUP 0x5752 is pinned throughout and put back at the end.
 */
void copy_rect_around_cursor(int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t saved;                    /* [bp-0x0e] */
    uint16_t hit_draw = 0, hit_shown = 0;   /* [bp-2], [bp-4] */
    struct page_slot *si;

    saved = DG5752.guard;
    DG5752.guard = 1;

    si = claim_page_slot(VMDS.page_src_ptr);
    if (si != PAGESLOT_NONE && (si->obj.flags & 2)
        && (int16_t)(x + w) > si->obj.x
        && (int16_t)(si->obj.x
                     + si->obj.w) > x
        && (int16_t)(y + h) > si->obj.y
        && (int16_t)(si->obj.y
                     + si->obj.h) > y)
        hit_shown = 1;

    si = claim_page_slot(VMDS.page_dst_ptr);
    if (si != PAGESLOT_NONE && (si->obj.flags & 2)
        && (int16_t)(x + w) > si->obj.x
        && (int16_t)(si->obj.x
                     + si->obj.w) > x
        && (int16_t)(y + h) > si->obj.y
        && (int16_t)(si->obj.y
                     + si->obj.h) > y)
        hit_draw = 1;

    if (((int16_t)MACHINE_CURSOR_STATE.page) == 0 && hit_draw != 0) {
        draw_cursor(VMDS.page_src_ptr);

        if (w > 0 && h > 0)
            copy_rect_thunk((uint16_t)x, (uint16_t)y,
                            (uint16_t)w, (uint16_t)h);

        erase_object(VMDS.page_src_ptr);
    } else {
        if (hit_draw != 0)
            erase_object(VMDS.page_dst_ptr);

        if (w > 0 && h > 0)
            copy_rect_thunk((uint16_t)x, (uint16_t)y,
                            (uint16_t)w, (uint16_t)h);

        if (hit_shown != 0) {
            restore_object_backdrop(VMDS.page_src_ptr, VMDS.page_dst_ptr);
            clear_object_covered(VMDS.page_dst_ptr);
        }

        if (hit_draw != 0)
            draw_cursor(VMDS.page_dst_ptr);
    }

    DG5752.guard = saved;

}

/*
 * 0x0b542
 *
 * The button state machine, one eight-byte record per button at DGROUP 0x5742:
 * the state at +0, whether it was down last time at +2, a press count at +4,
 * and a repeat delay at +6.
 *
 * The states are 0 up, 2 pressed, 4 clicked and 8 held. A release with the
 * state at 8 goes straight back to 0; otherwise the press count goes up and the
 * state becomes 2 the first time and 4 after that - which is what tells a click
 * from a double one.
 *
 * A change also latches where the pointer was, at DGROUP 0x5776 and 0x5778 -
 * from the driver when 0x2d42 says so, and from the last known position when it
 * does not - and reloads the delay from 0x2d40. The delay then counts down on
 * every call, and while it is still running *and* something has been pressed,
 * the answer is the raw button rather than the state, which is what holds a
 * click on screen long enough to be seen.
 */
int16_t button_state(uint16_t index, int16_t down)
{
    struct button *b = &MACHINE_BUTTONS.button[index];

    if (b->was_down != down) {
        b->was_down = down;

        if (down == 0) {
            if (b->state == 8) {
                b->state = 0;
            } else {
                b->presses++;
                if (b->presses == 1 && b->state != 2)
                    b->state = 2;
                else
                    b->state = 4;
            }
        }

        if (MACHINE_CURSOR_STATE.read_driver != 0) {
            read_mouse_pointer(&DG5768.button_at_x, &DG5768.button_at_y);
        } else {
            DG5768.button_at_x = DG5768.cursor_x;
            DG5768.button_at_y = DG5768.cursor_y;
        }

        b->delay = MACHINE_CURSOR_STATE.delay_reload;
    }

    if (b->delay != 0)
        b->delay--;

    if (b->delay != 0 && b->presses <= 0)
        return down;

    if (down != 0)
        b->state = 8;
    else if (b->presses == 0)
        b->state = 0;

    b->presses = 0;

    return b->state;
}

/*
 * 0x0b82c
 *
 * Switch the interrupt handler onto a stack of its own, and back: a non-zero
 * argument saves SS:SP at DGROUP 0x317e and puts SP at 0x2e7c inside DGROUP, a
 * zero one puts the saved pair back. The entry at 0x0b84b is the second half
 * reached directly.
 *
 * It does this by popping its own return address and argument off the stack,
 * changing SS:SP, and pushing them back - the only way to return onto a stack
 * you have just swapped.
 *
 * **The switch itself means nothing here.** The port's handler runs on a real
 * thread with a real stack of its own, which is what the private stack was for.
 * The two DGROUP words are still written, because anything else can read them.
 */
void isr_stack_switch(int16_t to_private)
{
    if (to_private != 0) {
        MACHINE_ISR_STACK.saved_ss = DGROUP_SEG;
        MACHINE_ISR_STACK.saved_sp = guest_sp;
        return;
    }

    /* The restore half at 0x0b84b: the saved pair goes back into SS:SP. */
}

/*
 * 0x0b859
 *
 * Set the mouse's mickeys-per-pixel, INT 33h AX=0x0f, the same value for both
 * axes: the argument goes into CX and DX alike. The start-up asks for 3.
 *
 * Nothing of it is in guest memory, so the port sends it to the IO boundary and
 * there is nothing here for the two artefacts to disagree about.
 */
void mouse_set_speed(uint16_t mickeys)
{
    io_mouse_set_speed(mickeys, mickeys);
}

/*
 * 0x0b93d
 *
 * `fread` into a **huge** pointer, one byte at a time, answering how many whole
 * items came in.
 *
 * A byte at a time because the destination may cross a segment: each one is
 * stored through the far pointer and then `huge_add_to` steps and renormalises
 * it - reached here by its near door at 0x0be7f.
 *
 * The count is `size * count` as a 32-bit product, and the answer is the bytes
 * actually read divided by the size, which is why a partial last item does not
 * count. The loop stops on the count running out or on `game_fgetc` answering
 * -1, and the test is made **before** the decrement, so a count of zero reads
 * nothing.
 */
uint32_t fread_huge(uint8_t far * dst, uint32_t size, uint32_t count,
                    FILE *file)
{
    /* `dst` is the [bp-8] pair `huge_add_to` steps - the caller's copy, taken
       by value, which a huge pointer's `++` is. */
    uint32_t total = long_multiply(count, size);
    uint32_t got = 0;

    while (total != 0) {
        int16_t c;

        total--;

        c = game_fgetc(file);

        if (c == -1)
            break;

        *dst++ = (uint8_t)c;
        got++;
    }
    return ulong_divide(got, size);
}

/*
 * 0x0b9c9
 *
 * Draw one bitmap scaled - the same choice `draw_bitmap` makes, from the same
 * marker in field 4, and the same normalisation of the header's far pointer
 * written back into it.
 *
 * Two of the four forms have no scaled blitter: 0xfffd, the already-scaled
 * one, and 0xffff, the offset-table one, both simply do nothing here. That is
 * the original's own silence and not a gap - a bitmap in either form is never
 * asked to be drawn scaled.
 */
void draw_bitmap_scaled(struct bitmap *hdr, int16_t x, int16_t y,
                        int16_t w, int16_t h, uint16_t mode)
{
    hdr->data = far_normalise_rev(hdr->data);

    switch (hdr->mask_off) {
    case 0xfffd:
    case 0xffff:
        return;
    case 0xfffe:
        blit_scaled_a(hdr, x, y, mode, w, h);
        return;
    default:
        blit_scaled_b(hdr, x, y, mode, w, h);
        return;
    }
}

/*
 * 0x093a2
 *
 * The game's own `ftell`, and the third of the trio over the archive - the same
 * choice between the loose file and the archive as `game_fread` and
 * `game_fseek`, and the same substitution when an entry carries its own `FILE`.
 *
 * Inside an archive entry the answer is the entry's own position at +0xa:+0xc,
 * not the file's, so a caller sees the resource as if it were a file of its
 * own.
 */
int32_t game_ftell(FILE *file)
{
    struct game_file *si = GAME_FILE_NONE;

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE)
        return borland_ftell(file);

    if (si->stream_ptr != 0)
        return borland_ftell(FILEREC_PTR(si->stream_ptr));

    return (int32_t)si->pos;
}

/*
 * 0x093f6
 *
 * The game's own `fgetc`, and `game_fread`'s shape one byte at a time: the same
 * choice between the loose file and the archive, the same substitution when an
 * entry carries a `FILE` at +0x10, the same table at DGROUP 0x549f.
 *
 * The two DGROUP cells it sets on the way through - 0x548d with the `FILE` it
 * was asked about and 0x548b with the one it actually read from - are written
 * on every path, including the plain one, so something downstream reads them.
 *
 * At the end of an entry it answers -1 without touching the file at all. The
 * test is a 32-bit compare of the position at +0xa:+0xc against the size at
 * +6:+8, written high half first.
 *
 * Both the entry position and the archive's running total advance by one.
 */
int16_t game_fgetc(FILE *file)
{
    struct game_file *si = GAME_FILE_NONE;

    DG547A.file_asked_ptr = dg_near(dgroup, file);

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE) {
        DG547A.file_used_ptr = dg_near(dgroup, file);
        return borland_fgetc(file);
    }

    if (si->stream_ptr != 0) {
        DG547A.file_used_ptr = si->stream_ptr;
        return borland_fgetc(FILEREC_PTR(si->stream_ptr));
    }

    if (si->pos >= si->size)
        return -1;

    make_file_current(si->archive);

    {
        uint32_t at = si->base + si->pos;
        int16_t got;
        struct archive *a;

        seek_file_to(at);

        file = FILEREC_PTR(MACHINE_ARCHIVES.slot[si->archive].stream_ptr);
        DG547A.file_used_ptr = dg_near(dgroup, file);
        got = borland_fgetc(file);

        si->pos++;

        a = &MACHINE_ARCHIVES.slot[si->archive];
        a->pos++;

        return got;
    }
}

/*
 * 0x08fc3
 *
 * Takes one argument, ignores it, and answers 1. Six instructions: a frame,
 * `mov ax, 1`, and a jump to the epilogue that goes nowhere.
 *
 * Both its callers are on a failure path in the file layer - one of them after
 * a failed open, having first checked three flags - and it sits immediately
 * before `game_fopen` in the same source file. That reads like the routine
 * that would have asked the user what to do about a disk error, left answering
 * "carry on" in the shipped build. **That is a reading of where it is called
 * from, not something the bytes say**: what the bytes say is that it answers 1.
 */
int16_t answer_carry_on(uint16_t what)
{
    (void)what;
    return 1;
}

/*
 * 0x08fcd
 *
 * The game's own `fopen`. Answers one of the ten 0x12-byte archive-entry
 * blocks at DGROUP 0x55c3, not a `FILE` - which is why every one of
 * `game_fread`, `game_fgetc`, `game_fseek` and `game_ftell` starts by asking
 * `archive_entry_for` whether the thing it was handed is one of these.
 *
 * With no archive loaded it is a plain forward to the runtime's `fopen` and the
 * caller gets a real `FILE`.
 *
 * Otherwise a free block is found - +0xe is the in-use flag - and the name
 * hashed, which leaves the hash at DGROUP 0x5482 for `find_entry_for_pointer`
 * to look up. Then the **loose file is tried first**: if a real file of that
 * name exists it is opened and the block simply carries its `FILE` at +0x10.
 * That is how a patched or unpacked file overrides the archive.
 *
 * Failing that the archive is searched. The entry's own header is read - a
 * 13-byte name and a 4-byte size - and the name compared case-insensitively
 * against the one asked for, which is the check that the hash found the right
 * file rather than a colliding one. The entry's data then starts where `ftell`
 * says the file now is.
 *
 * The retry loop around the loose-file open exists for removable media: 0x5488
 * is set by the critical-error handler and 0x38ad says whether to prompt. Both
 * are dead here.
 */
FILE *game_fopen(char *name, const char *mode)
{
    char hdr[16];
    struct game_file *si;
    struct file_rec *di;
    int16_t left;
    FILE *r = NULL;

    if (DG547A.reopen != 0)
        make_file_current(0);

    load_archive_map();
    DG5677.failures = 0;

    if (DG547A.archive_count == 0) {
        r = borland_fopen(name, mode);
        goto out;
    }

    DG547A.file_used_ptr = 0;
    DG547A.file_asked_ptr = 0;

    si = &MACHINE_GAME_FILES.files[0];
    for (left = 0xa; left != 0; left--) {
        if (si->in_use == 0)
            break;
        si++;
    }

    if (left == 0)
        goto out;

    hash_filename(name);

    DG547A.opening = 1;

    for (;;) {
        DG547A.retry = 0;
        di = borland_fopen(name, mode);

        if (((int16_t)DG4E67.file_op_active) != 0) {
            r = di;
            goto out;
        }
        if (DG547A.retry != 0 && ((uint8_t)VMDS.pixel_shift) != 0)
            not_transcribed("0x08fc3, the prompt for a missing disk");
        if (DG547A.retry == 0)
            break;
    }

    DG547A.opening = 0;

    if (di != NULL) {
        si->archive = 0;
        si->pos = 0;
        si->size = 0;
        si->base = 0;
        si->in_use = 1;
        si->stream_ptr = dg_near(dgroup, di);
        goto found;
    }

    if (find_entry_for_pointer(si) == 0)
        goto out;

    make_file_current(si->archive);

    {
        uint32_t at = si->base + si->pos;
        int32_t pos;
        struct archive *a;

        seek_file_to(at);

        di = FILEREC_PTR(MACHINE_ARCHIVES.slot[DG547A.last_record].stream_ptr);

        borland_fread((uint8_t *)hdr, 0xd, 1, di);
        borland_fread((uint8_t *)&si->size, 4, 1, di);

        pos = borland_ftell(di);
        si->base = (uint32_t)pos;

        a = &MACHINE_ARCHIVES.slot[DG547A.last_record];
        a->pos = (uint32_t)pos;
    }

    if (string_compare_nocase(hdr, name) != 0)
        goto out;

    si->pos = 0;
    si->stream_ptr = 0;
    si->in_use = 1;

found:
    DG547A.open_immediate = (uint8_t)(DG547A.open_immediate + 1);
    /* An archive entry: the game's FILE is its `game_file` record, and
       `archive_entry_for` tells the two apart by looking for it in the
       table. */
    r = (FILE *)si;

out:
    return r;
}

/*
 * 0x0960f
 *
 * Load `RESOURCE.MAP`, which is what tells the game where everything in the
 * archives is. Runs once - DGROUP 0x548a is the flag that says so.
 *
 * Before opening anything it takes over **INT 24h**, DOS's critical-error
 * handler, keeping the old vector at DGROUP 0x5677. That is what stops a
 * missing disk from aborting the program, and the handler it installs is at
 * 0x9bdf. The segment pushed for it reads 0x0000 in the image and is a
 * relocation; the port works it out from where the program is.
 *
 * The file itself is four bytes into the table at DGROUP 0x28d2 - the byte
 * offsets `hash_filename` packs, so the hash function is **defined by the
 * file**, not by the program - then a count of archives, and then for each
 * archive a 13-byte name into its 0x1c-byte record at DGROUP 0x548f, a count of
 * entries, and a block of eight bytes per entry holding a hash and an offset.
 *
 * The block is one entry longer than the count, which leaves room for the
 * terminator the lookup relies on.
 *
 * The count of archives **accumulates** into DGROUP 0x547e, and the first index
 * of this map is worked out from it afterwards, so a second map would append
 * rather than replace.
 */
void load_archive_map(void)
{
    int16_t count;               /* [bp-8] */
    uint32_t lo;                 /* [bp-0xc] the entry's key */
    uint32_t hi;                 /* [bp-0x10] and where its data starts */
    struct file_rec *file;
    int16_t di;

    if (DG547A.scanned != 0) {
        return;
    }

    DG5677.crit_vec = dos_getvect(0x24);

    dos_setvect(0x24, (struct far_ptr){ 0x9bdf, (uint16_t)(IMAGE_BASE >> 4) });
    DG547A.scanned = 1;

    file = borland_fopen(MACHINE_RESOURCE_MAP_NAMES.resource_map, MACHINE_RESOURCE_MAP_NAMES.mode_rb_a);
    if (file == 0) {
        return;
    }

    borland_fread((uint8_t *)MACHINE_HASH_ORDER.hash_order, 4, 1, file);
    borland_fread((uint8_t *)&count, 2, 1, file);

    DG547A.archive_count = (int16_t)(DG547A.archive_count + count);
    di = (int16_t)(DG547A.archive_count - count + 1);

    for (; di <= DG547A.archive_count; di++) {
        struct archive *a = &MACHINE_ARCHIVES.slot[di];
        struct archive_entry *e;

        borland_fread((uint8_t *)a->name, 0xd, 1, file);
        borland_fread((uint8_t *)&count, 2, 1, file);

        /* **Zeroed**, which is what writes the terminator: the block is one
           entry longer than the count and the lookup stops on an all-zero key.
           The flags are the fourth argument - `push 1`, then `push 0` for the
           third - and the port had the two the other way round, so the block
           was not cleared and the terminator was whatever the memory held. */
        a->list = (dos_alloc_bytes((uint16_t)((count + 1) << 3), 0, 1).ptr);
        a->index = (uint16_t)di;

        /* The original steps the far pointer at [bp-6] by eight, one entry. */
        e = (struct archive_entry *)a->list;
        while (count != 0) {
            count--;

            borland_fread((uint8_t *)&lo, 4, 1, file);
            borland_fread((uint8_t *)&hi, 4, 1, file);

            e->key = lo;
            e->base = hi;
            e++;
        }
    }

    borland_fclose(file);
}

/*
 * 0x0980d
 *
 * Hash a filename, answering the hash in DX:AX and leaving it at DGROUP
 * 0x5482 as well. A null name answers zero and stores zero.
 *
 * The name is **uppercased in place**, in the caller's own buffer, and any
 * `\\` or `:` restarts the two running values and moves the start of the name
 * past it - so only the last path component counts and the caller's pointer is
 * left pointing at it.
 *
 * Two things are accumulated over the name: a sum and an exclusive-or. Then the
 * last component is copied into a 13-byte buffer, padded with zeros, and four
 * of its bytes - at the offsets in the table at DGROUP 0x28d2 - are packed into
 * a 32-bit value eight bits at a time. The sum times the exclusive-or is added
 * to that, **as a 16-bit product sign-extended**: the `imul` computes 32 bits
 * and the `cwd` after it throws the top half away, which is the compiler
 * treating the result as an `int`.
 */
int32_t hash_filename(char *name)
{
    char buf[22];
    char *si;
    uint16_t sum = 0, eor = 0;
    uint32_t acc = 0;
    int16_t i;

    if (name == NULL) {
        DG547A.name_hash = 0;
        return 0;
    }

    si = name;
    while (*si != 0) {
        uint8_t c;

        if (*si >= 'a' && *si <= 'z')
            *si = (char)(*si ^ 0x20);

        c = (uint8_t)*si;
        sum = (uint16_t)(sum + c);
        eor ^= c;

        if (*si == '\\' || *si == ':') {
            eor = 0;
            sum = 0;
            name = si + 1;
        }
        si++;
    }

    string_copy_padded(buf, name, 0xd);

    for (i = 0; i < 4; i++) {
        uint8_t c = (uint8_t)buf[MACHINE_HASH_ORDER.hash_order[i]];

        acc = long_shift_left(acc, 8) + c;
    }

    acc = (uint32_t)((int32_t)acc
                     + (int32_t)(int16_t)(sum * eor));

    DG547A.name_hash = acc;
    return (int32_t)acc;
}

/*
 * 0x098e0
 *
 * Find which archive holds a file, and answer whether one does.
 *
 * What is looked for is not an argument - it is the **filename hash** at
 * 0x5482, which `hash_filename` leaves there. Records are 0x1c bytes from
 * 0x54a7, and each one's first field is a far pointer to the list of
 * eight-byte entries `load_archive_map` read: the hash at +0 and the file's
 * offset within the archive at +4, ending at an all-zero hash. 0x5480 holds the
 * record last used and 0x547e the highest valid index.
 *
 * The search starts at 0x5480 - or at 1 if that is zero, so record 0 is never
 * where a search begins - and then spirals outward one index at a time,
 * forward first and backward second, re-reading both globals on every pass.
 * That is a locality bet: the pointer being asked about is usually in the
 * record that was just used.
 *
 * Each step scans a whole list, so a step can stop on a null entry rather than
 * a match; the outer loop re-checks and keeps going while either direction has
 * indices left. Whichever record was scanned last is the one reported, so the
 * final check decides between a real hit and having simply run out.
 *
 * On success the `game_file` the caller passes takes the archive index and the
 * entry's offset, and has its size and position zeroed. The original writes
 * those four as **four 16-bit stores**, high half first - the port had them as
 * two `DG32`, which is byte for byte the same only because the value is zero.
 */
int16_t find_entry_for_pointer(struct game_file *out)
{
    uint32_t want = DG547A.name_hash;
    const struct archive_entry *at;
    int16_t idx, fwd, back;

    idx = ((int16_t)DG547A.last_record);
    if (idx == 0)
        idx = 1;
    scan_entry_list(idx, want, &at);

    fwd = (int16_t)(((int16_t)DG547A.last_record) + 1);
    back = (int16_t)(((int16_t)DG547A.last_record) - 1);

    for (;;) {
        if (at->key == want)
            break;
        if (back <= 0 && fwd > DG547A.archive_count)
            break;

        if (fwd <= DG547A.archive_count) {
            idx = fwd++;
            scan_entry_list(idx, want, &at);
        }

        if (at->key == want)
            continue;
        if (back <= 0)
            continue;
        idx = back--;
        scan_entry_list(idx, want, &at);
    }

    if (at->key != want)
        return 0;

    out->archive = (uint16_t)idx;
    out->base = at->base;
    out->pos = 0;
    out->size = 0;
    return 1;
}

/*
 * 0x0a7a3
 *
 * Set the word at DGROUP 0x2d44 to zero, and nothing else.
 *
 * What the flag governs is **not established**. Its counterpart at 0x0a78e
 * sets it to 1 and then redraws through 0xacc3, passing the word at 0x38a4 -
 * which is inside the video driver's data block at 0x3890 - so the pair reads
 * like suspending and resuming something on screen. That is inference from the
 * shape of the two routines, not something measured, and the name says only
 * what the code does.
 */
void cursor_redraw_off(void)
{
    MACHINE_CURSOR_STATE.timer_draws_cursor = 0;
}

/*
 * 0x09a62
 *
 * Make a given resource file the open one, opening it and closing whatever was
 * open before.
 *
 * The first thing it does is a **file-exists test written as an open and an
 * immediate close**: with the flag at 0x5486 clear it tries the file by name
 * and shuts it again, keeping only whether that worked. That is the loose-file
 * probe - the game asks whether a real file is there before settling for the
 * packed copy - and finding one forces a reopen even when the same index is
 * already current.
 *
 * Without that, an index that is already current returns immediately, which is
 * why the routine is cheap enough to call before every read: 18,930 calls, 26
 * of which reach DOS.
 *
 * The open itself retries forever. A failure calls the prompt at 0x08fc3 - but
 * only while 0x38ad is set - and tries again, which is how a program on
 * removable media asks for the right disk. With 0x38ad clear it spins on
 * `fopen` with nothing to change the answer.
 *
 * Afterwards the believed file position at +0x12 is zeroed, because a freshly
 * opened file is at nought, and `archive_entry_for(0)` is called to throw away
 * the one-entry cache - the `FILE` pointers it remembers are about to be stale.
 *
 * The fast path above is the common one - 18,930 calls against 26 that open
 * anything - but every occurrence the harness samples is off it, which is why
 * this needed the runtime's own `fopen` before it could be checked at all.
 */
void make_file_current(uint16_t index)
{
    struct archive *a;
    int16_t exists = 0;

    if (DG547A.open_immediate == 0 && index != 0) {
        struct file_rec *f = borland_fopen((const char *)MACHINE_ARCHIVES.slot[index].name,
                                 MACHINE_RESOURCE_MAP_NAMES.mode_rb_b);

        borland_fclose(f);
        if (f != 0)
            exists = 1;
    }

    if (index == DG547A.last_record && exists == 0 && DG547A.reopen == 0)
        return;

    a = &MACHINE_ARCHIVES.slot[DG547A.last_record];
    if (a->stream_ptr != 0) {
        borland_fclose(FILEREC_PTR(a->stream_ptr));
        a->stream_ptr = 0;
    }

    DG547A.last_record = (int16_t)index;
    a = &MACHINE_ARCHIVES.slot[DG547A.last_record];

    if (index != 0) {
        DG547A.opening = 1;
        for (;;) {
            struct file_rec *f = borland_fopen((const char *)a->name,
                                     MACHINE_RESOURCE_MAP_NAMES.mode_rb_c);

            a->stream_ptr = dg_near(dgroup, f);
            if (f != 0)
                break;
            if (((uint8_t)VMDS.pixel_shift) != 0)
                not_transcribed("0x08fc3, the prompt for a missing disk");
        }
        DG547A.opening = 0;
    }

    a->pos = 0;

    archive_entry_for(0);
    DG547A.reopen = 0;
}

/*
 * 0x09b38
 *
 * Put a file at a given position, without asking DOS if it is already there.
 *
 * The record is the 0x1c-byte entry at DGROUP 0x548f selected by 0x5480 - the
 * same table `find_entry_for_pointer` walks, four bytes lower. Its +0x12 holds
 * the position DOS is believed to be at, as a 32-bit value, and a seek to that
 * same place does nothing at all.
 *
 * That cache is why the loader can afford to ask for a seek before every read:
 * measured over a run, 18,930 calls reach DOS 319 times. The archive is read
 * forward, so the believed position is nearly always right.
 *
 * The 319 that do reach DOS go through the runtime's own `fseek`, always from
 * the start of the file. That used to be a stand-in in io.c that did nothing,
 * with the caller verified only on occurrences where the buffer does not move;
 * it is the real routine now.
 */
void seek_file_to(uint32_t at)
{
    struct archive *a = &MACHINE_ARCHIVES.slot[DG547A.last_record];

    if (a->pos == at)
        return;

    borland_fseek(FILEREC_PTR(a->stream_ptr), (int32_t)at, 0);

    a->pos = at;
}

/*
 * 0x09b7c
 *
 * Find the archive entry standing in for an open file, or answer null if the
 * file is a real one.
 *
 * This is the pivot of the loader's two-way lookup: the game asks for each
 * resource as a loose file first and falls back to the packed archive, and this
 * is what tells the two apart afterwards. Everything above it - `fread`,
 * `fseek`, `ftell` - checks here before deciding whether to touch DOS.
 *
 * The table is ten entries of 0x12 bytes at DGROUP 0x55c3, keyed by the `FILE`
 * pointer itself, and there is a **one-entry cache** in front of it: 0x547a
 * holds the last pointer asked about and 0x547c the answer. A repeat question
 * is answered without walking anything, which matters because the read path
 * asks on every call.
 *
 * A null pointer clears the cache and answers null - that is how the cache is
 * invalidated when a file is closed.
 *
 * Two things end the walk with no match: running out of entries, and finding
 * one whose +0xe is zero. The second is checked **after** the loop rather than
 * inside it, so an entry matching the pointer but not yet open is found and
 * then rejected. Both paths also clear 0x547a, so the negative answer is not
 * cached - only positive ones are.
 */
struct game_file *archive_entry_for(FILE *file)
{
    struct game_file *si;
    int16_t n;

    if (file == 0) {
        DG547A.cache_key_ptr = 0;
        DG547A.cache_answer_ptr = 0;
        return GAME_FILE_NONE;
    }

    if (DG547A.archive_count == 0)
        return GAME_FILE_NONE;

    if (file == FILEREC_PTR(DG547A.cache_key_ptr))
        return GAME_FILE_PTR(DG547A.cache_answer_ptr);

    DG547A.cache_key_ptr = dg_near(dgroup, file);

    si = &MACHINE_GAME_FILES.files[0];
    n = 0xa;
    while (n != 0 && (FILE *)si != file) {
        si++;
        n--;
    }

    if (n == 0 || si->in_use == 0) {
        si = GAME_FILE_NONE;
        DG547A.cache_key_ptr = 0;
    }

    DG547A.cache_answer_ptr = dg_near(dgroup, si);
    return si;
}

/*
 * 0x0aaca
 *
 * Wait for the frame, then latch the input state for the frame about to be
 * drawn and clear the accumulators.
 *
 * The wait is the spin on `frame_pending` that the INT 08h handler releases -
 * and it is guarded, so when DGROUP 0x44ee is clear the routine does not wait
 * at all. That spin measured at 64% of all basic block executions under an
 * emulator paced on the host clock; see STATUS.md.
 *
 * The pair at 0x5782/0x5784 is filled either from `read_mouse_pointer` or from the
 * two words at 0x576c/0x576e, and the pair at 0x5768/0x576a is moved into
 * 0x5772/0x5774 and zeroed - accumulated since the last frame, then handed
 * over and reset, which is what a frame boundary looks like.
 */
void wait_and_latch_frame(void)
{
    if (TIMER.installed != 0) {
        while (frame_pending())
            ;
    }

    if (((int16_t)MACHINE_CURSOR_STATE.read_driver) != 0) {
        read_mouse_pointer(&DG5768.pointer_x, &DG5768.pointer_y);
    } else {
        DG5768.pointer_x = DG5768.cursor_x;
        DG5768.pointer_y = DG5768.cursor_y;
    }

    DG5768.button_left = DG5768.button_accum_b;
    DG5768.button_right = DG5768.button_accum_a;
    DG5768.button_accum_a = 0;
    DG5768.button_accum_b = 0;
    DG5752.frame_flag = 0;
}

/*
 * 0x0ad51
 *
 * Put back whatever an object was covering, and mark it no longer drawn.
 *
 * Bit 1 of the byte at +0x13 says the object is currently on screen; with it
 * clear this does nothing but the bookkeeping around it. With it set there are
 * two ways back. If the object has a buffer slot at +0x10 and a rectangle with
 * both extents positive, the saved pixels are put back with `vm_restore_rect`,
 * the buffer being the far pointer at DGROUP 0x5754 + 4 * slot - one-based, as
 * `claim_buffer_slot` hands them out. Otherwise a single pixel is replaced from
 * the colour byte at +0x12, which is what `restage_object_rect` left there.
 *
 * The rectangle used is the **clipped** one at +8, not the unclipped position
 * at +4, so an object partly off-screen restores only the part that was drawn.
 *
 * The two words at 0x38a6 and 0x38a8 are both set from the record's first word
 * before any of that. They are inside the driver's data block, and the same
 * value goes to both.
 *
 * `save_or_restore_draw_state` brackets the whole thing, and the global at
 * 0x5752 is forced to 1 for the duration and put back at the end - the same
 * pattern `restage_object_rect` uses.
 */
void erase_object(uint16_t handle)
{
    struct page_slot *rec;
    uint16_t slot;
    int16_t saved;

    rec = claim_page_slot(handle);
    if (rec == PAGESLOT_NONE)
        return;

    saved = ((int16_t)DG5752.guard);
    DG5752.guard = 1;

    save_or_restore_draw_state(1);

    VMDS.page_src_ptr = ((int16_t)rec->page);
    VMDS.page_dst_ptr = ((int16_t)rec->page);

    if ((rec->obj.flags & 2) != 0) {
        if (((int16_t)rec->obj.buf) != 0 && rec->obj.w > 0
            && rec->obj.h > 0) {
            slot = rec->obj.buf;
            vm_restore_rect(MACHINE_RECT_BUFFERS.slot[slot - 1],
                            rec->obj.x, rec->obj.y,
                            rec->obj.w, rec->obj.h);
        } else {
            plot_pixel_clipped(rec->obj.x, rec->obj.y,
                               rec->obj.pixel);
        }
        rec->obj.flags &= 0xfd;
    }

    save_or_restore_draw_state(0);
    DG5752.guard = saved;
}

/*
 * 0x0adf1
 *
 * Put back what an object covered on one page, and make the other page the
 * one being drawn to.
 *
 * The slot's +0x10 says where the backdrop was kept: non-zero and it is a far
 * pointer in the pair of tables at DGROUP 0x5754 and 0x5756, four bytes apart
 * per slot, and the rectangle goes back whole. Zero and there was only ever one
 * pixel, whose colour is the byte at +0x12.
 *
 * The draw state is saved across it and DGROUP 0x5752 pinned, which is what
 * stops the cursor being redrawn in the middle.
 */
void restore_object_backdrop(uint16_t from_page, uint16_t to_page)
{
    int16_t saved;                        /* [bp-2] */
    struct page_slot *si = claim_page_slot(from_page);

    if (si == PAGESLOT_NONE)
        goto out;

    saved = (int16_t)DG5752.guard;
    DG5752.guard = 1;

    save_or_restore_draw_state(1);

    VMDS.page_src_ptr = to_page;
    VMDS.page_dst_ptr = to_page;

    if (si->obj.flags & 2) {
        if (si->obj.buf != 0
            && si->obj.w > 0
            && si->obj.h > 0) {
            restore_rect_thunk(MACHINE_RECT_BUFFERS.slot[si->obj.buf - 1],
                               si->obj.x,
                               si->obj.y,
                               si->obj.w,
                               si->obj.h);
        } else {
            plot_pixel_clipped(si->obj.x,
                               si->obj.y,
                               (int16_t)si->obj.pixel);
        }
    }

    save_or_restore_draw_state(0);
    DG5752.guard = (uint16_t)saved;

out:
}

/*
 * 0x0ae8e
 *
 * **Swap the object lists of two pages.** Each page's slot holds the head of a
 * list of saved rectangles; this exchanges the two heads, so everything drawn
 * over one page is now attributed to the other.
 *
 * Nothing happens unless both pages have slots: `claim_page_slot` is asked for
 * each, and either answering zero leaves the two lists alone.
 *
 * The swap is done with the re-entry guard at DGROUP 0x5752 raised and put
 * back afterwards, because for the two instructions between the two stores
 * neither list is whole - one head is in a local and the other is in both
 * slots - and a cursor redraw arriving there would walk it.
 */
void swap_page_objects(uint16_t page_a, uint16_t page_b)
{
    struct page_slot *slot_a = claim_page_slot(page_b);
    struct page_slot *slot_b;
    uint16_t was;
    uint16_t head;

    if (slot_a == PAGESLOT_NONE)
        return;

    slot_b = claim_page_slot(page_a);
    if (slot_b == PAGESLOT_NONE)
        return;

    was = DG5752.guard;
    DG5752.guard = 1;

    head = slot_a->page;
    slot_a->page = slot_b->page;
    slot_b->page = head;

    DG5752.guard = was;
}

/*
 * 0x0aedc
 *
 * Say a page's object no longer covers anything: clear bit 1 of the slot's
 * +0x13. A page with no slot is left alone.
 */
void clear_object_covered(uint16_t page)
{
    struct page_slot *si = claim_page_slot(page);

    if (si != PAGESLOT_NONE)
        si->obj.flags &= 0xfd;
}

/*
 * 0x0aef6
 *
 * Age an object's on-screen rectangle by one frame: copy where it is now into
 * where it was, then work out where it is now from the current globals and clip
 * that to the screen.
 *
 * The record has two parallel blocks. The current one runs from +8 - x, y, w, h
 * at +8/+0xa/+0xc/+0xe, a buffer slot at +0x10, and two bytes at +0x12/+0x13 -
 * and the previous one from +0x14 with the same shape, its bytes at
 * +0x1e/+0x1f. Copying one onto the other is the whole of the first half.
 *
 * Before that copy it may hand back a buffer slot, and **only the
 * `release_buffer` call survives**: the two stores beside it, zeroing +0x1c
 * and clearing bit 0 of +0x1f, are both overwritten a few instructions later by
 * the copy. They are dead as written, and transcribed anyway.
 *
 * If the object's parent at +2 no longer matches the global at 0x5770 it is
 * re-parented, which means asking the driver how big the new parent's image is
 * and claiming a scratch buffer for it. `claim_buffer_slot` ignores the size it
 * is handed, so that measurement goes nowhere - see 0x0b5ed. A null parent
 * gives slot zero and a 1 by 1 rectangle.
 *
 * The unclipped position goes to +4/+6 and is kept; the clipped copy goes to
 * +8. Clipping is one-sided in the usual way: a negative coordinate is pulled
 * to zero and taken out of the extent, and an extent running past 0x3f7a or
 * 0x3f7c - the screen width and height - is cut back to the edge. Nothing stops
 * an extent going negative if the rectangle is entirely off-screen.
 *
 * The global at 0x5752 is set to 1 for the duration and put back at the end,
 * and 0x5740 being non-zero suppresses both the slot release and the
 * re-parenting.
 */
void restage_object_rect(uint16_t handle)
{
    struct page_slot *rec;
    struct bitmap *parent;
    int16_t saved, x, y, w, h;

    rec = claim_page_slot(handle);
    if (rec == PAGESLOT_NONE)
        return;

    saved = ((int16_t)DG5752.guard);
    DG5752.guard = 1;

    if ((rec->cursor.flags & 1) != 0 && ((int16_t)rec->cursor.buf) != 0
        && MACHINE_PALETTE_FADE.busy == 0) {
        release_buffer(((int16_t)rec->cursor.buf));
        rec->cursor.buf = 0;
        rec->cursor.flags &= 0xfe;
    }

    rec->cursor.x = rec->obj.x;
    rec->cursor.y = rec->obj.y;
    rec->cursor.w = rec->obj.w;
    rec->cursor.h = rec->obj.h;
    rec->cursor.buf = ((int16_t)rec->obj.buf);
    rec->cursor.flags = rec->obj.flags;
    rec->cursor.pixel = rec->obj.pixel;

    if (rec->bitmap_ptr != DG5768.cursor_bitmap_ptr && MACHINE_PALETTE_FADE.busy == 0) {
        rec->cursor.flags |= 1;
        rec->bitmap_ptr = DG5768.cursor_bitmap_ptr;

        if (DG5768.cursor_bitmap_ptr != 0) {
            int32_t asked;

            parent = BMP_PTR(DG5768.cursor_bitmap_ptr);
            asked = (int16_t)vm_buffer_size((uint16_t)parent->width,
                                            (uint16_t)parent->height);
            rec->obj.buf = claim_buffer_slot(asked, 0);
        } else {
            rec->obj.buf = 0;
        }
    }

    if (((int16_t)MACHINE_CURSOR_STATE.read_driver) != 0)
        read_mouse_pointer(&DG5768.cursor_x, &DG5768.cursor_y);

    x = (int16_t)(DG5768.cursor_x - DG5768.hot_x);
    y = (int16_t)(DG5768.cursor_y - DG5768.hot_y);

    if (DG5768.cursor_bitmap_ptr != 0) {
        parent = BMP_PTR(DG5768.cursor_bitmap_ptr);
        w = parent->width;
        h = parent->height;
    } else {
        h = 1;
        w = 1;
    }

    rec->x = x;
    rec->y = y;

    if (x < 0) {
        w = (int16_t)(w + x);
        x = 0;
    }
    if (x + w >= VMDS.screen.screen_width)
        w = (int16_t)(VMDS.screen.screen_width - x);
    if (y < 0) {
        h = (int16_t)(h + y);
        y = 0;
    }
    if (y + h >= VMDS.screen.screen_height)
        h = (int16_t)(VMDS.screen.screen_height - y);

    rec->obj.x = x;
    rec->obj.y = y;
    rec->obj.w = w;
    rec->obj.h = h;

    DG5752.guard = saved;
}

/*
 * 0x0b429
 *
 * Find the entry in the two-slot table at DGROUP 0x56e6 whose top bits match,
 * and claim it. The slots are 0x20 bytes apart, so the second is at 0x5706 -
 * and those are exactly the two words the initialisation below fills with the
 * driver's back and front pages, once, the first time through.
 *
 * The match is on bits **0xa800** only, not on the whole word, so a slot
 * matches a page that differs from it in the low bits. Answers the slot, or
 * PAGESLOT_NONE - the original's 0 - if neither matched.
 *
 * A `want` of 0 means "the page currently being drawn into".
 */
struct page_slot *claim_page_slot(uint16_t want)
{
    int16_t i;

    if (MACHINE_CURSOR_STATE.slots_unset != 0) {
        MACHINE_PAGE_SLOTS.slots[0].page = VMDS.page_back_ptr;
        MACHINE_PAGE_SLOTS.slots[1].page = VMDS.page_front_ptr;
        MACHINE_CURSOR_STATE.slots_unset = 0;
    }

    if (want == 0)
        want = VMDS.page_back_ptr;

    for (i = 0; i < 2; i++) {
        if ((want & 0xA800) == ((uint16_t)MACHINE_PAGE_SLOTS.slots[i].page & 0xA800)) {
            MACHINE_PAGE_SLOTS.slots[i].page = (dg_seg_t)want;
            return &MACHINE_PAGE_SLOTS.slots[i];
        }
    }
    return PAGESLOT_NONE;
}

/*
 * 0x0b47f
 *
 * Save the driver's drawing state, or put it back: a non-zero argument saves,
 * zero restores. The state is the clip box, whether clipping is on, and the
 * two page segments - seven values, kept at DGROUP 0x5726..0x5732.
 *
 * `VMDS.clip_enabled` is a byte and is saved **zero-extended into a word**, then
 * restored as a byte, so the high half of 0x5726 is always zero. Transcribed
 * with the same widths rather than made symmetrical.
 */
void save_or_restore_draw_state(int16_t save)
{
    if (save != 0) {
        MACHINE_SAVED_DRAW_STATE.saved_a = VMDS.clip_enabled;
        MACHINE_SAVED_DRAW_STATE.saved_b = VMDS.clip_left;
        MACHINE_SAVED_DRAW_STATE.saved_c = VMDS.clip_right;
        MACHINE_SAVED_DRAW_STATE.saved_d = VMDS.clip_top;
        MACHINE_SAVED_DRAW_STATE.saved_e = VMDS.clip_bottom;
        MACHINE_SAVED_DRAW_STATE.saved_g = VMDS.page_dst_ptr;
        MACHINE_SAVED_DRAW_STATE.saved_f = VMDS.page_src_ptr;
    } else {
        VMDS.clip_enabled = ((uint8_t)MACHINE_SAVED_DRAW_STATE.saved_a);
        VMDS.clip_left = MACHINE_SAVED_DRAW_STATE.saved_b;
        VMDS.clip_right = MACHINE_SAVED_DRAW_STATE.saved_c;
        VMDS.clip_top = MACHINE_SAVED_DRAW_STATE.saved_d;
        VMDS.clip_bottom = MACHINE_SAVED_DRAW_STATE.saved_e;
        VMDS.page_dst_ptr = MACHINE_SAVED_DRAW_STATE.saved_g;
        VMDS.page_src_ptr = MACHINE_SAVED_DRAW_STATE.saved_f;
    }
}

/*
 * 0x0b4e2
 *
 * Non-zero while `DG5752.frame_flag` is still clear. The original is
 * `neg ax / sbb ax,ax / inc ax`, which is Borland's idiom for `ax = (ax == 0)`.
 *
 * The caller at 0x0aaca spins on this waiting for the INT 08h handler to set
 * the flag; that spin was 64% of all basic block executions under an emulator
 * paced on the host clock. See STATUS.md.
 */
int16_t frame_pending(void)
{
    return (int16_t)(DG5752.frame_flag == 0);
}

/*
 * 0x0b4f1
 *
 * Clear the input state: two eight-byte blocks at DGROUP 0x5742, then the two
 * accumulators at 0x5768/0x576a and the two latched values at 0x5772/0x5774 -
 * the same four words `wait_and_latch_frame` moves and zeroes each frame.
 *
 * The word at 0x5752 is **saved, set to 2, and put back**. It sits immediately
 * after the sixteen bytes being cleared, so it is not being protected from the
 * loop; it is a guard held across the clear, which only makes sense if
 * something asynchronous - the INT 08h handler, which writes these very words -
 * reads it.
 */
void reset_input_state(void)
{
    int16_t saved = ((int16_t)DG5752.guard);
    struct button *b = &MACHINE_BUTTONS.button[0];
    int16_t n = 2;

    DG5752.guard = 2;

    while (n != 0) {
        b->state = 0;
        b->was_down = 0;
        b->presses = 0;
        b->delay = 0;
        b++;
        n--;
    }

    DG5768.button_accum_a = 0;
    DG5768.button_accum_b = 0;
    DG5768.button_right = 0;
    DG5768.button_left = 0;

    DG5752.guard = saved;
}

/*
 * 0x0b5ed
 *
 * Make sure the four scratch buffers exist, then claim a free one and answer
 * its **one-based** index, or -1 if all four are taken. `release_buffer` is
 * the release.
 *
 * The four buffers are far pointers at DGROUP 0x5758, four bytes apart; the
 * four in-use bytes are at 0x5734, which is why that array is one-based - zero
 * is the "no slot" answer. Any buffer still null is allocated on the way past,
 * so the first call does all four allocations and later ones do none.
 *
 * The size is the word at 0x5756, or, if that is zero, whatever the driver
 * says a 64 by 64 planar image needs - reached through the thunk at 0x21ab9,
 * which is `vm_buffer_size` here. Only the low word of the driver's DX:AX
 * answer is kept, and it is then sign-extended by `cwd` into the 32-bit size
 * DOS is asked for, so a size at or above 0x8000 would be asked for as a
 * negative length. Nothing seen produces one.
 *
 * **The four argument words are ignored.** The routine opens by loading each
 * of the two pairs and storing them straight back where they came from, which
 * is a no-op, and then never reads them again. The caller at 0x0aef6 goes to
 * the trouble of asking the driver for a size and passing it in, and this
 * discards it in favour of 0x5756 or the 64 by 64 default. Transcribed as it
 * stands, with the parameters named and voided.
 */
int16_t claim_buffer_slot(int32_t a, int32_t b)
{
    int16_t i;
    uint16_t size;
    int32_t asked;

    /* Two Borland `long`s the routine does not read - its one caller splits
       an `int32_t` across the first pair and passes zero for the second. It
       sizes the buffer from `DG5752.size_word` or `vm_buffer_size` instead. */
    (void)a;
    (void)b;

    if (DG5752.size_word != 0)
        size = ((uint16_t)DG5752.size_word);
    else
        size = (uint16_t)vm_buffer_size(0x40, 0x40);

    asked = (int16_t)size;

    for (i = 0; i < 4; i++) {
        if (MACHINE_RECT_BUFFERS.slot[i] == FAR_NULL_PTR) {
            MACHINE_RECT_BUFFERS.slot[i] = (dos_alloc_bytes(asked, 0, 0).ptr);
        }
    }

    for (i = 0; i < 4; i++) {
        if (MACHINE_BUFFER_USED.used[i] == 0
            && MACHINE_RECT_BUFFERS.slot[i] != FAR_NULL_PTR) {
            MACHINE_BUFFER_USED.used[i] = 1;
            return (int16_t)(i + 1);
        }
    }

    return -1;
}

/*
 * 0x0b69c
 *
 * Clear one byte of the four-entry array at DGROUP 0x5734, addressed
 * **one-based**: the argument is decremented before it is used as the index.
 *
 * Zero is rejected, and so is anything that lands at index 4 or above. Nothing
 * rejects a *negative* argument: the bound is `jge 4`, a signed test that a
 * negative index passes, so a caller passing a number below zero writes a zero
 * byte in front of the array. No caller seen does, but the guard is genuinely
 * one-sided and the port reproduces it rather than adding the missing half.
 */
void release_buffer(int16_t n)
{
    int16_t i = (int16_t)(n - 1);

    if (n != 0 && i < 4)
        MACHINE_BUFFER_USED.used[i] = 0;
}
