/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Palettes**: loading one out of a resource file, setting and fading the
 * current one, and cycling ranges of it.
 *
 * A module of the original's **code segment 1c25**, image 0x1e967..0x1eded,
 * split out of engine.c on 2026-09-27. Its `_DATA` is DGROUP 0x445e..0x44d0:
 * the cycle count, the fade colour and weight, the palette sizes, and the
 * chunk names with "PAL:AMG:" last, in the literal pool. Its `_BSS` is the
 * three cycle tables at 0x591a..0x5956. `restore_write_mode` before it and
 * `draw_polygon` after it are assembly. Functions are in address order and
 * each carries the image offset it was read from.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 * JUDGE: data 0x445e..0x44cf
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/* The driver's entries this module calls through its vector table: the
   palette loader (slot 20, DGROUP 0x4396), the blend (slot 34, 0x43ce), the
   do-nothing stub (slot 23, 0x43a2) and the span fill (slot 27, 0x43b2).
   The last two take ES:SI; see `fill_span_list`. */
typedef void (far *vm_pal_fn)(uint8_t far *pal);
typedef void (far *vm_blend_fn)(uint16_t first, uint16_t count,
                                uint16_t colour, uint16_t weight);
typedef void (far *vm_esi_fn)(void);
#endif

/*
 * **The number of palette cycles filed**, DGROUP 0x445e: `add_palette_cycle`
 * counts them up to nine and `cycle_palettes` walks them.
 */
int16_t PALETTE_CYCLES DGROUP_AT(0x445e) = 0;

/*
 * **What the last palette fade was asked for, and how big a palette is**,
 * DGROUP 0x4460..0x4466, 0x06 bytes.
 */
struct engine_pen {
    uint16_t  fade_weight;          /* +0x00 [2] */
    uint16_t  fade_colour;          /* +0x02 [2] */
    /* **The palette's size in bytes for the depth in use**, out of the table
       below - 0x300 for 256 colours - and what the loader reads and the
       allocator doubles. */
    int16_t   palette_bytes;   /* +0x04 [2] */
} PACKED;

struct engine_pen ENGINE_PEN DGROUP_AT(0x4460) = { 0x003f, 0, 0x0300 };

/*
 * **How many bytes of palette each pixel depth has**, DGROUP 0x4466..0x4486,
 * 0x20 bytes: sixteen words indexed by the pixel shift, which `load_palette`
 * and `set_palette_pointer` file into `ENGINE_PEN.palette_bytes` and then read
 * that many bytes of file into the block. 0x300 is 256 colours of three bytes
 * and 0x30 is sixteen of three; the header called them pointers, which the
 * values are not. Up to 0x4486, where the chunk names begin.
 */
struct engine_palette_sizes {
    int16_t   size[16];           /* +0x00 [0x20] */
} PACKED;

struct engine_palette_sizes ENGINE_PALETTE_SIZES DGROUP_AT(0x4466) = {
    {
        0x0000, 0x0102, 0x0011, 0x0011, 0x0102, 0x0300, 0x0000, 0x0300,
        0x0300, 0x0300, 0x0300, 0x0030, 0x0030, 0x0030, 0x0030, 0x0300,
    },
};


struct pal_chunk_names PALCHUNK DGROUP_WAS(0x4486) = {
    "PAL:VGA:",
    "PAL:EGA:",
    "PAL:CGA:",
    "",
    {
        PALCHUNK.none, PALCHUNK.pal_cga, PALCHUNK.pal_ega, PALCHUNK.pal_ega,
        PALCHUNK.pal_cga, PALCHUNK.pal_vga, PALCHUNK.none, PALCHUNK.pal_vga,
        PALCHUNK.pal_vga, PALCHUNK.pal_vga, PALCHUNK.pal_vga, PALCHUNK.pal_ega,
        PALCHUNK.pal_vga, PALCHUNK.pal_vga, PALCHUNK.pal_vga, PALCHUNK.pal_vga,
    },
    0,
};


/*
 * **The cycle tables**, DGROUP 0x591a..0x5956: for each cycle, in bytes of
 * palette (three to a colour), the step it turns by, its first entry and the
 * entry past its last. Ten slots for the nine `add_palette_cycle` allows.
 * The names are chosen for Borland C++ 2.0's `_BSS` order, which comes from
 * the names (docs/lessons.md): these three land at 0x591a, 0x592e and 0x5942.
 */
int16_t CYCLE_STEP[10] DGROUP_BSS(0x591a);
int16_t CYCLE_FROM[10] DGROUP_BSS(0x592e);
int16_t CYCLE_LIMIT[10] DGROUP_BSS(0x5942);

/*
 * 0x1e967
 *
 * Load a palette and keep it. Takes either a resource name or an already-open
 * file record - `file_record_valid` tells the two apart, and a name is opened
 * here and closed again before returning. Answers the far pointer to the block
 * it allocated, and files that pointer in the table at DGROUP 0x3a2e.
 *
 * That table is eleven slots of four bytes, offset at 0x3a2e and segment at
 * 0x3a30, searched from 1 to 9 for one whose four bytes are zero. When none is
 * free the search ends with the index at 10, and the routine files a null
 * pointer into slot 10 and answers null - the table's last slot, which only
 * this store ever reaches. See `VMDS.palettes.blocks` for why that is storage and not
 * an overrun.
 *
 * The palette's length and the chunk name are both chosen by the byte at
 * DGROUP 0x38ad, through the word tables at 0x4466 and 0x44a2. If that chunk is
 * not in the file and DGROUP 0x38af is set, it falls back to a "PAL:AMG:"
 * chunk: 32 Amiga colour words, 4 bits per component, each expanded to the
 * VGA's 6 by masking to four bits and shifting up two. That fills 96 bytes of
 * the 768 and the remaining 672 are zeroed, which is where the 256-entry size
 * comes from.
 *
 * The pointer goes back in **DX:AX**, `mov dx,[bp-8] / mov ax,[bp-0xa]` at
 * 0x1eb5e - the segment in DX and the offset in AX, which is a
 * `struct far_ptr` and is answered as one.
 */
uint8_t far *load_palette(char *name)
{
    int16_t opened;                     /* [bp-2] */
    uint8_t far *p;                     /* [bp-6] */
    uint8_t far *blk;                   /* [bp-0xa] */
    uint8_t buf[0x300];                 /* [bp-0x30a] */
    int16_t amg[0x20];                  /* [bp-0x34a] */
    register int16_t i;
    register int16_t slot;

    blk = 0;
    ENGINE_PEN.palette_bytes = ENGINE_PALETTE_SIZES.size[VMDS.pixel_shift];

    for (slot = 1; VMDS.palettes.blocks[slot] != NULL
                   && slot < 10; slot++)
        ;

    if (slot < 10) {
        /* `name` is a handle or a name to open, and becomes the handle. */
        if (!file_record_valid((FILE *)name)) {
            opened = 1;
            name = (char *)open_file_record(name);
        } else
            opened = 0;

        /* Which palette chunk this adapter wants; entry 0 is the empty
           string, which `seek_named_chunk` refuses. */
        if (seek_named_chunk((FILE *)name,
                             PALCHUNK.by_adapter[VMDS.pixel_shift], 0) != -1L) {
            if ((blk = DOS_ALLOC_PTR(DOS_ALLOC(ENGINE_PEN.palette_bytes, 0)))
                != NULL) {
                game_fread(buf, 1, ENGINE_PEN.palette_bytes, (FILE *)name);
                huge_move(blk, buf, ENGINE_PEN.palette_bytes);
            }
        } else if (VMDS.vga_chunks != 0
                   && seek_named_chunk((FILE *)name, "PAL:AMG:", 0) != -1L
                   && game_fread((uint8_t *)amg, 1, 0x40, (FILE *)name) != 0
                   && (blk = DOS_ALLOC_PTR(DOS_ALLOC(ENGINE_PEN.palette_bytes, 0)))
                      != NULL) {
            p = blk;
            for (i = 0; i < 0x20; i++) {
                *p++ = ((amg[i] >> 8) & 0xf) << 2;
                *p++ = ((amg[i] >> 4) & 0xf) << 2;
                *p++ = (amg[i] & 0xf) << 2;
            }
            for (i = 0; i < 0x2a0; i++)
                *p++ = 0;
        }

        if (opened != 0)
            close_file_record((FILE *)name);
    }

    VMDS.palettes.blocks[slot] = blk;
    return blk;
}

/*
 * 0x1eb6a
 *
 * Set the current palette, or answer the one already set.
 *
 * It first makes sure a buffer exists: the byte at `VMDS.pixel_shift` - the driver's
 * own mode number, sign extended - indexes a table of sizes at DGROUP 0x4466,
 * and if the far pointer at 0x3a2e is still null a block of twice that many
 * bytes is allocated for it.
 *
 * Then, with a null argument it answers the pointer it last stored; with a
 * real one it stores it, hands it to the driver at VGA:0x0f15 through the
 * vector at DGROUP 0x4396, and answers it back.
 *
 * The pointer is passed and answered offset-first, in AX, with the segment in
 * DX - the usual far-pointer convention here.
 */
uint8_t far *set_palette_pointer(uint8_t far *h)
{
    ENGINE_PEN.palette_bytes = ENGINE_PALETTE_SIZES.size[VMDS.pixel_shift];

    if (VMDS.palettes.blocks[0] == NULL
        && ENGINE_PEN.palette_bytes != 0)
        VMDS.palettes.blocks[0] = DOS_ALLOC_PTR(DOS_ALLOC(ENGINE_PEN.palette_bytes * 2, 0));

    if (h == NULL)
        return PALCHUNK.palette_ptr;

    PALCHUNK.palette_ptr = h;
#ifdef __TURBOC__
    ((vm_pal_fn)DG4342.font[20])(h);
#else
    vm_load_palette(h);
#endif
    return h;
}

/*
 * 0x1ebdc
 *
 * Free one far block from the table of eleven at DGROUP 0x3a2e, found by its
 * address rather than by an index: the pair passed in is compared against each
 * entry and the one that matches is freed and zeroed.
 *
 * The walk is slots 1 to 9: entry 0 is `set_palette_pointer`'s, and entry 10
 * only ever holds the null `load_palette` files when the table is full. A null
 * argument does nothing at all.
 */
void free_far_block(uint8_t far *h)
{
    register int16_t i;

    if (h != NULL)
        for (i = 1; i < 10; i++)
            if (VMDS.palettes.blocks[i] == h) {
                dos_free_far(VMDS.palettes.blocks[i]);
                VMDS.palettes.blocks[i] = NULL;
            }
}

/*
 * 0x1ec36
 *
 * Fade a run of palette entries towards a colour, through the driver's vector
 * at DGROUP 0x43ce - which is VGA:0x0f57, read out of a running machine
 * because nothing in the image writes that word.
 *
 * The size is filed at DGROUP 0x4460 and 0x4462 first, in the order the
 * arguments are *not* in - the colour to 0x4460 and the weight to 0x4462 -
 * before both are passed on unchanged.
 */
void fade_palette_run(uint16_t first, uint16_t count, register uint16_t colour,
                      register uint16_t weight)
{
    ENGINE_PEN.fade_weight = weight;
    ENGINE_PEN.fade_colour = colour;
#ifdef __TURBOC__
    ((vm_blend_fn)DG4342.font[34])(first, count, colour, weight);
#else
    vm_blend_palette(first, count, colour, (uint8_t)weight);
#endif
}

/*
 * 0x1ec5c
 *
 * **File a range of the palette to cycle**: `count` entries from `first`,
 * turned by `step` each time `cycle_palettes` runs - a negative step counts
 * back from the range's end. All three are kept in bytes, three to a colour.
 * Answers the number of cycles filed, or 0 when it refused: without the
 * 256-colour driver's chunks (0x38af), with nine filed already, or for a
 * range of one entry or none. A negative `first` empties the table first,
 * and is then refused as a range of none.
 *
 * Nothing on the paths the port runs calls it. The name is ours.
 */
int16_t add_palette_cycle(register int16_t first, register int16_t count,
                          register int16_t step)
{
    if (first < 0)
        PALETTE_CYCLES = count = 0;

    if (!(int8_t)VMDS.vga_chunks || PALETTE_CYCLES >= 9 || count <= 1)
        return 0;

    CYCLE_FROM[PALETTE_CYCLES] = first * 3;
    CYCLE_LIMIT[PALETTE_CYCLES] = (first + count) * 3;
    if (step < 0)
        step = count + step;
    CYCLE_STEP[PALETTE_CYCLES] = step * 3;
    return ++PALETTE_CYCLES;
}

/*
 * 0x1ecd7
 *
 * **Turn every filed palette cycle one step**, and hand the result to the
 * driver. The palette block (`VMDS.palettes.blocks[0]`) holds two copies:
 * the one the driver is given, and 0x300 bytes on the one being turned. The
 * turned copy is laid over the given one whole, and then each range of the
 * given one is copied back into the turned copy rotated by its step. The
 * driver's blend entry reloads all 256 colours at the fade already set.
 *
 * Nothing on the paths the port runs calls it. The name is ours.
 */
void cycle_palettes(void)
{
    int16_t  limit;                     /* [bp-2] */
    int16_t  step;                      /* [bp-4] */
    uint8_t far *shown;                 /* [bp-8] */
    uint8_t far *turned;                /* [bp-0xc] */
    register int16_t i;
    register int16_t from;

    if (!(int8_t)VMDS.vga_chunks)
        return;

    shown = turned = VMDS.palettes.blocks[0];
    turned += 0x300;
    copy_far_bytes(turned, shown, 0x300);

    for (i = 0; i < PALETTE_CYCLES; i++) {
        from = CYCLE_FROM[i];
        limit = CYCLE_LIMIT[i];
        step = CYCLE_STEP[i];
        copy_far_bytes(shown + from, from + step + turned, limit - from - step);
        copy_far_bytes(shown + (limit - step), turned + from, step);
    }

#ifdef __TURBOC__
    ((vm_blend_fn)DG4342.font[34])(0, 0x100, ENGINE_PEN.fade_colour,
                               ENGINE_PEN.fade_weight);
#else
    vm_blend_palette(0, 0x100, ENGINE_PEN.fade_colour,
                     (uint8_t)ENGINE_PEN.fade_weight);
#endif
}

/*
 * 0x1eda2
 *
 * Copy `n` bytes from one far pointer to another, a byte at a time. Near:
 * only `cycle_palettes` calls it. The name is ours.
 */
void near copy_far_bytes(uint8_t far *src, uint8_t far *dst, register int16_t n)
{
    while (n--)
        *dst++ = *src++;
}

/*
 * 0x1edc7
 *
 * Fill a span list through the driver's entry at slot 27 (DGROUP 0x43b2,
 * `vm_fill_spans`), which takes the list in ES:SI. The name is ours.
 */
void fill_span_list(uint8_t far *spans)
{
#ifdef __TURBOC__
    _SI = FP_OFF(spans);
    _ES = FP_SEG(spans);
    _DI;            /* the driver's entry uses DI: the compiler saves it */
    ((vm_esi_fn)DG4342.font[27])();
#else
    vm_fill_spans(spans);
#endif
}

/*
 * 0x1edda
 *
 * The same shape through slot 23 (DGROUP 0x43a2), which is the driver's
 * do-nothing stub. The name is ours, and says only what it is shaped like.
 */
void span_list_nothing(uint8_t far *spans)
{
#ifdef __TURBOC__
    _SI = FP_OFF(spans);
    _ES = FP_SEG(spans);
    _DI;            /* the driver's entry uses DI: the compiler saves it */
    ((vm_esi_fn)DG4342.font[23])();
#else
    (void)spans;
    vm_nothing();
#endif
}
