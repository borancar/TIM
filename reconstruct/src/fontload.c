/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Loading a font into one of the font slots, and closing a slot.**
 *
 * A module of the original's **code segment 1c25**, image 0x2307d..0x234d2,
 * split out of engine.c on 2026-09-27. Its `_DATA` is DGROUP 0x495c..0x4965
 * - a pointer to the chunk name, then its pool - between the video driver
 * loader's and the pad byte that ends it; bmpload.c's begins at 0x4966.
 * Whether `close_table_618a_slot` is this module's or the next one's, no
 * call or data reference says; it is a font slot's, so it is here.
 * Functions are in address order and each carries the image offset it was
 * read from.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 * JUDGE: data 0x495c..0x4965
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **Which chunk a font lives in**, DGROUP 0x495c: a pointer to the name,
 * and the name and the "r" `load_font` opens with after it, 0x495e..0x4965.
 * This module's `_DATA`.
 */
char *FONT_CHUNK_NAME DGROUP_WAS(0x495c) = "FNT:";

/*
 * 0x2307d
 *
 * Load a font into one of the eighteen slots of the table at DGROUP 0x618a,
 * and answer the slot number - or 0 for any failure, which is why the search
 * starts at 2 and not at 0. Like `load_palette` it takes either a resource name
 * or an already-open file record, and closes only what it opened itself.
 *
 * The font's header is a run of single bytes read into parallel arrays indexed
 * by the slot: 0x38c4, 0x38d8, 0x38ec, 0x3900 and, for a compressed font,
 * 0x627a. The **first** byte read is a marker rather than a field, and it picks
 * one of three shapes:
 *
 *   0xfd, 0xff  compressed. 0x6176 gets the marker negated - 3 or 1 - the rest
 *               of the header follows, then a word of decompressed size, and
 *               the body comes through the resource layer (`open_resource`,
 *               `read_resource`, `close_resource`) into a block from DOS. Three
 *               far pointers into that block are filed: the body at 0x61da, and
 *               two more at 0x622a and 0x618a, stepped past 2 and then 1 byte
 *               per glyph of the count at 0x3900.
 *   0xfe        uncompressed, and the byte after the marker is the width in
 *               bytes as it stands.
 *   anything    uncompressed, and the marker *was* the width, in bits: it is
 *               rounded up to whole bytes with `(w + 7) >> 3`.
 *
 * Both uncompressed shapes read the body into one near-heap block and file it
 * at 0x618a with DGROUP as its segment, leaving the other two pointers null.
 *
 * The failure flag at [bp-8] is set once and tested before each further step,
 * which is how the original writes what would now be an early return.
 */
uint16_t load_font(char *name)
{
    int16_t opened;
    int16_t size;
    int16_t handle;
    int16_t failed;
    uint8_t far *blk;
    uint8_t *p;
    register int16_t si;
    register FILE *di = (FILE *)name;   /* a handle, or a name to open */

#ifndef __TURBOC__
    /* **The original frees `blk` unset when the resource will not open**,
       whatever the stack held. The host starts it null. Ours. */
    blk = NULL;
#endif

    /* The first free slot from 2. */
    for (si = 2; ENGINE_FONT_BODIES.body[si] != NULL && si < 0x14; si++)
        ;

    if (si < 0x14) {
        if (file_record_valid(di) == 0) {
            opened = 1;
            di = open_file_record((char *)di);
        } else {
            opened = 0;
        }

        if (seek_named_chunk(di, FONT_CHUNK_NAME, 0) != -1L) {
            game_fread(&VMDS.font_table_34[si], 1, 1, di);

            if (VMDS.font_table_34[si] == 0xfd || VMDS.font_table_34[si] == 0xff) {
                /* A resource-packed font: glyph widths, slots and bodies in
                   one DOS block. */
                ENGINE_FONT_KINDS.kind[si] = -VMDS.font_table_34[si];

                game_fread(&VMDS.font_table_34[si], 1, 1, di);
                game_fread(&VMDS.font_table_48[si], 1, 1, di);
                game_fread(&ENGINE_UNDERLINE_ROWS.underline_row[si], 1, 1, di);
                game_fread(&VMDS.font_table_5c[si], 1, 1, di);
                game_fread(&VMDS.font_table_70[si], 1, 1, di);
                game_fread((uint8_t *)&size, 1, 2, di);

                failed = (handle = open_resource(0xffff, di, "r",
                                                 file_record_size(di))) < 0;
                if (!failed)
                    failed = (int16_t)resource_size(handle) != size;
                if (!failed)
                    failed = (blk = DOS_ALLOC_PTR(DOS_ALLOC((uint16_t)size, 0)))
                             == NULL;
                if (!failed)
                    failed = read_resource(handle, blk, size) != size;
                if (!failed) {
                    /* Three pointers into the one block, stepped in the
                       block's own segment. */
                    ENGINE_FONT_WIDTHS.width[si] = blk;
                    blk += VMDS.font_table_70[si] * 2;
                    ENGINE_FONT_SLOTS.slot[si] = blk;
                    blk += VMDS.font_table_70[si];
                    ENGINE_FONT_BODIES.body[si] = blk;
                }

                close_resource(handle);

                if (failed) {
                    if (blk != NULL)
                        dos_free_far(blk);
                    si = 0;
                }
            } else {
                /* A bitmap font: one near block of glyphs. */
                if (VMDS.font_table_34[si] == 0xfe) {
                    ENGINE_FONT_KINDS.kind[si] = 2;
                    game_fread(&VMDS.font_table_34[si], 1, 1, di);
                    size = VMDS.font_table_34[si];
                } else {
                    ENGINE_FONT_KINDS.kind[si] = 0;
                    size = (VMDS.font_table_34[si] + 7) >> 3;
                }

                game_fread(&VMDS.font_table_48[si], 1, 1, di);
                game_fread(&VMDS.font_table_5c[si], 1, 1, di);
                game_fread(&VMDS.font_table_70[si], 1, 1, di);

                size *= VMDS.font_table_48[si] * VMDS.font_table_70[si];

                failed = (p = malloc_far(size)) == NULL;
                if (!failed)
                    game_fread(p, size, 1, di);
                if (!failed) {
                    ENGINE_FONT_BODIES.body[si] = (uint8_t far *)NEAR_ZERO(p);
                    ENGINE_FONT_WIDTHS.width[si] = 0;
                    ENGINE_FONT_SLOTS.slot[si] = 0;
                }

                if (failed) {
                    if (p != NULL)
                        free_far(p);
                    si = 0;
                }
            }
        } else {
            si = 0;
        }

        if (opened != 0)
            close_file_record(di);
    } else {
        si = 0;
    }

    return (uint16_t)si;
}

/*
 * 0x233ef
 *
 * Close one of the ten slots in the table at DGROUP 0x618a, which
 * `table_618a_in_use` answers for. A slot that is not in use is left alone.
 *
 * **The slot that matches entry 0 takes the driver's own state down with it**:
 * six bytes and three pairs of words are cleared, including two that belong to
 * the video driver's data at 0x38ec and 0x38c4. That happens only when the
 * slot's pointer equals entry 0's, so entry 0 is the one the rest hang off.
 *
 * Either way the slot itself is freed - through `dos_free_far` when it has a
 * far pointer at 0x61da and through `free_far` when it does not - and its
 * three table entries and its byte at 0x6176 are cleared.
 */
void close_table_618a_slot(int16_t index)
{
    if (table_618a_in_use(index) == 0)
        return;

    if (ENGINE_FONT_BODIES.body[index]
        == ENGINE_FONT_BODIES.body[0]) {
        ENGINE_FONT_KINDS.kind[0] = 0;
        VMDS.font_table_5c[0] = VMDS.font_table_70[0] = 0;
        VMDS.font_table_34[0] = VMDS.font_table_48[0] =
            ENGINE_UNDERLINE_ROWS.underline_row[0] = 0;

        ENGINE_FONT_WIDTHS.width[0] = NULL;
        ENGINE_FONT_SLOTS.slot[0]    = NULL;
        ENGINE_FONT_BODIES.body[0]   = NULL;
    }

    if (ENGINE_FONT_WIDTHS.width[index] != NULL)
        dos_free_far(ENGINE_FONT_WIDTHS.width[index]);
    else
        free_far((uint8_t *)ENGINE_FONT_BODIES.body[index]);

    ENGINE_FONT_KINDS.kind[index] = 0;

    /* The three slot tables, cleared through the types that name them -
       which is what `bx = 4 * index` was computing an offset into. */
    ENGINE_FONT_BODIES.body[index]  = NULL;
    ENGINE_FONT_WIDTHS.width[index] = NULL;
    ENGINE_FONT_SLOTS.slot[index]   = NULL;
}
