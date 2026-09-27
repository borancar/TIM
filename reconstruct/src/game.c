/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game as a program**: `main`, the bring-up, the intro and the
 * copy protection, the round and screen state machines, the level loop and
 * the drag that places a part, and the teardown that prints your password.
 *
 * This file corresponds to the original's **code segment 0dff**, image
 * 0x0dff0..0x14de0. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * The DGROUP records only this file reads or writes. Each is laid over the
 * DGROUP byte array at the address its macro names, like the shared ones in
 * dgroup.h; they are declared here because nothing else uses them.
 */

struct game_master_levels GAME_MASTER_LEVELS DGROUP_AT(0x0116) = {
    { 0x0000, 0x0003, 0x0005, 0x0008, 0x000a, 0x000d, 0x000f }, /* master_level_ok */
};

/*
 * **The path separator**, DGROUP 0x1bca..0x1bcc, 0x02 bytes: a near pointer to
 * the backslash string, which the path builders concatenate.
 */
struct game_path_sep {
    uint16_t  path_sep_ptr;      /* +0x00 [2]  a near pointer to the "\\" at 0x236e, `DG1BCC.path_sep` */          /* +0x00 */
} PACKED;

struct game_path_sep GAME_PATH_SEP DGROUP_AT(0x1bca) = {
    0x236e, /* path_sep_ptr */
};


/*
 * **The file names and modes**, DGROUP 0x2870..0x28d2, 0x62 bytes: one "rb", "wb", "l", ".lev",
 * "password.txt" or "tim.cfg" per call site, in the order the routines that
 * open them sit in the segment. The two at 0x287d and 0x287f have no reader
 * in the port. The run ends at the hash order at 0x28d2.
 *
 * The four names are fields because `game_fopen` uppercases a name in place
 * through `hash_filename` - see `game_startup_names`. The modes and the "l" and ".lev"
 * pieces are only read, and each routine reads its own copy by name.
 */
struct game_file_names {
    char rb_read_level[3];        /* +0x00 [3]  read_level */
    char wb_write_level[3];       /* +0x03 [3]  write_level */
    char l_load_level[2];         /* +0x06 [2]  load_level builds "l<n>.lev" */
    char lev_load_level[5];       /* +0x08 [5] */
    char l_287d[2];               /* +0x0d [2]  no reader in the port */
    char lev_287f[5];             /* +0x0f [5] */
    char rb_is_machine_file[3];   /* +0x14 [3]  is_machine_file */
    char l_count_levels[2];       /* +0x17 [2]  count_level_files */
    char lev_count_levels[5];     /* +0x19 [5] */
    char rb_count_levels[3];      /* +0x1e [3] */
    char l_puzzle_title[2];       /* +0x21 [2]  get_puzzle_title */
    char lev_puzzle_title[5];     /* +0x23 [5] */
    char rb_puzzle_title[3];      /* +0x28 [3] */
    char password_txt_level[13];  /* +0x2b [0xd]  password_to_level */
    char rb_password_level[3];    /* +0x38 [3] */
    char password_txt_line[13];   /* +0x3b [0xd]  read_password_line */
    char rb_password_line[3];     /* +0x48 [3] */
    char tim_cfg_read[8];         /* +0x4b [8]  read_tim_cfg */
    char rb_tim_cfg[3];           /* +0x53 [3] */
    char tim_cfg_write[8];        /* +0x56 [8]  write_config, which writes it */
    char wb_tim_cfg[3];           /* +0x5e [3] */
    uint8_t pad_28d1[1];          /* +0x61 [1] */
} PACKED;

struct game_file_names GAME_FILE_NAMES DGROUP_AT(0x2870) = {
    "rb", /* rb_read_level */
    "wb", /* wb_write_level */
    "l", /* l_load_level */
    ".lev", /* lev_load_level */
    "l", /* l_287d */
    ".lev", /* lev_287f */
    "rb", /* rb_is_machine_file */
    "l", /* l_count_levels */
    ".lev", /* lev_count_levels */
    "rb", /* rb_count_levels */
    "l", /* l_puzzle_title */
    ".lev", /* lev_puzzle_title */
    "rb", /* rb_puzzle_title */
    "password.txt", /* password_txt_level */
    "rb", /* rb_password_level */
    "password.txt", /* password_txt_line */
    "rb", /* rb_password_line */
    "tim.cfg", /* tim_cfg_read */
    "rb", /* rb_tim_cfg */
    "tim.cfg", /* tim_cfg_write */
    "wb", /* wb_tim_cfg */
    {0}, /* pad_28d1 */
};

/*
 * **The characters a filename may not contain**, DGROUP 0x28ec..0x28fa, 0x0e bytes: fourteen
 * of them, `*` `/` `,` `-` `[` `]` `&` `@` `^` `%` `?` `(` `)` `:`, which
 * `validate_filename` tests one by one. The run ends at 0x28fa.
 */
struct game_forbidden_chars {
    uint8_t   forbidden[14] NONSTRING;  /* +0x00 [0xe]  a set, not a string */
} PACKED;

struct game_forbidden_chars GAME_FORBIDDEN_CHARS DGROUP_AT(0x28ec) = {
    "*/,-[]&@^%?():", /* forbidden */
};

/*
 * **Where Tab sends the pointer on the file picker's controls**, DGROUP 0x28fa..0x2918, 0x1e bytes: which
 * stop it is on - 0xffff until the first Tab, and back to 0 past the last
 * - and the x of each, the y being fixed.
 */
struct game_picker_tabs {
    uint16_t  stop;          /* +0x00 [2]  which of the picker's seven tab stops */
    int16_t   stop_x[7];          /* +0x02 [0xe]  where `picker_tab` parks the pointer */
    int16_t   stop_y[7];          /* +0x10 [0xe] */
} PACKED;

struct game_picker_tabs GAME_PICKER_TABS DGROUP_AT(0x28fa) = {
    0xffff, /* stop */
    { 0x0090, 0x0080, 0x00c0, 0x00d0, 0x00d0, 0x0060, 0x00e0 }, /* stop_x */
    { 0x005c, 0x0082, 0x0112, 0x0080, 0x00ec, 0x013a, 0x013a }, /* stop_y */
};

/*
 * **The file dialog's strings**, DGROUP 0x2918..0x2966: the ".TIM" extension
 * `pick_file` forces, the eleven DOS device names `validate_filename` refuses,
 * its mode, and the stars, patterns and dots the picker and the directory scan
 * use - each the copy one call site pushes.
 */
struct game_file_strings {
    char      tim[4];             /* +0x00  "TIM" */
    char      con[4];             /* +0x04  "con" */
    char      aux[4];             /* +0x08  "aux" */
    char      com1[5];            /* +0x0c  "com1" */
    char      com2[5];            /* +0x11  "com2" */
    char      com3[5];            /* +0x16  "com3" */
    char      com4[5];            /* +0x1b  "com4" */
    char      prn[4];             /* +0x20  "prn" */
    char      lpt1[5];            /* +0x24  "lpt1" */
    char      lpt2[5];            /* +0x29  "lpt2" */
    char      nul[4];             /* +0x2e  "nul" */
    char      null[5];            /* +0x32  "null" */
    char      mode_rb[3];         /* +0x37  "rb" */
    char      star_a[2];          /* +0x3a  "*" */
    char      star_b[2];          /* +0x3c  "*" */
    char      star_dot_star_a[4]; /* +0x3e  "*.*" */
    char      dot[2];             /* +0x42  "." */
    char      dot_dot_a[3];       /* +0x44  ".." */
    char      star_dot_star_b[4]; /* +0x47  "*.*" */
    char      dot_dot_b[3];       /* +0x4b  ".." */
} PACKED;

struct game_file_strings GAME_FILE_STRINGS DGROUP_AT(0x2918) = {
    "TIM", /* tim */
    "con", /* con */
    "aux", /* aux */
    "com1", /* com1 */
    "com2", /* com2 */
    "com3", /* com3 */
    "com4", /* com4 */
    "prn", /* prn */
    "lpt1", /* lpt1 */
    "lpt2", /* lpt2 */
    "nul", /* nul */
    "null", /* null */
    "rb", /* mode_rb */
    "*", /* star_a */
    "*", /* star_b */
    "*.*", /* star_dot_star_a */
    ".", /* dot */
    "..", /* dot_dot_a */
    "*.*", /* star_dot_star_b */
    "..", /* dot_dot_b */
};


struct game_directories GAME_DIRECTORIES DGROUP_BSS(0x530b);

/*
 * **The shared name buffer**, DGROUP 0x5682..0x568f, 0x0d bytes. `listing_to_name` strips a
 * listing record's `<`, `>` and spaces into it and answers its address, so the
 * caller has a near string it can hand to `strcpy`.
 *
 * Thirteen bytes, which is what a DOS 8.3 name and its NUL take - and what is
 * left between `dg_5677`, which ends at 0x5682, and `game_picker_text`.
 */
struct game_name_buffer {
    char      name[0xd];          /* +0x00 [0xd] */
} PACKED;

struct game_name_buffer GAME_NAME_BUFFER DGROUP_BSS(0x5682);

/*
 * **The file picker and the wrapped-text block**, DGROUP 0x568f..0x56a6, 0x17 bytes.
 */
struct game_picker_text {
    int16_t   picker_mode;        /* +0x00 [2]  0x80 from the mode it was opened from, else 0 */
    int16_t   scroll;             /* +0x02 [2]  clamped on the way in, not on the way out */
    int16_t   entry_count;        /* +0x04 [2] */
    uint8_t far *text_start;    /* +0x06 [4]  where the listing's text begins: `entry_max`
                                     four-byte pointer slots into `block`, same segment */
    uint8_t far *block;         /* +0x0a [4]  allocated once and kept; a null
                                     pointer is the end */
    int16_t   entry_max;          /* +0x0e [2] */
    uint8_t   _pad_569f;          /* +0x10 [1]  a byte: 0x56a0 follows at +0x11 */
    int16_t   text_height;        /* +0x11 [2]  the block's measured extents, which the centring uses */
    int16_t   text_width;         /* +0x13 [2]  the widest line, clamped to the box */
    int16_t   line_count;         /* +0x15 [2]  how many lines, for the table at 0x56a6 */
} PACKED;

struct game_picker_text GAME_PICKER_TEXT DGROUP_WAS(0x568f);

/*
 * NOT a transcription: the port's factoring of the eleven **identical inline
 * blocks** at 0x13205 to 0x133c3. Each is `strnicmp` against one reserved DOS
 * device name followed by a check that the byte after it ends the stem, and the
 * original repeats the whole thing eleven times rather than looping. Every
 * constant is kept, in the order the original tests them - including the last
 * pair, which do not agree with each other. The names are the DGROUP copies
 * each block pushes, 0x291c to 0x294a.
 */
static const struct {
    const char *name;
    uint16_t len;
    uint16_t after;
} reserved_names[] = {
    { GAME_FILE_STRINGS.con,  3, 3 },
    { GAME_FILE_STRINGS.aux,  3, 3 },
    { GAME_FILE_STRINGS.com1, 4, 4 },
    { GAME_FILE_STRINGS.com2, 4, 4 },
    { GAME_FILE_STRINGS.com3, 4, 4 },
    { GAME_FILE_STRINGS.com4, 4, 4 },
    { GAME_FILE_STRINGS.prn,  3, 3 },
    { GAME_FILE_STRINGS.lpt1, 4, 4 },
    { GAME_FILE_STRINGS.lpt2, 4, 4 },
    { GAME_FILE_STRINGS.nul,  3, 3 },
    { GAME_FILE_STRINGS.null, 3, 4 },   /* compared for THREE bytes - see below */
};

/*
 * 0x11d00
 *
 * **A part's index among all parts**, which is how the machine file refers to
 * one: a pointer means nothing to a reload, so every reference is written as the
 * position the part has in the walk `pick_by_flag(0x3000)` makes.
 *
 * A null part answers 0xffff, and that is the file's "no part here".
 *
 * **A part that is not found answers the count**, because the loop ends the same
 * way whether it found the part - which sets `si` to zero to break out - or ran
 * off the end, and the index is whatever the counter reached. So a reference to
 * something outside the walk is written as one past the last part rather than
 * as an error. Nothing here checks for it, and this is transcribed as it is
 * rather than made to answer 0xffff, because a reload that trips over it is
 * behaviour the original has.
 */
uint16_t part_index(struct part *part)
{
    struct part *si;
    uint16_t n = 0;

    if (part == PART_NONE)
        return 0xffff;

    for (si = pick_by_flag(0x3000); si != PART_NONE; ) {
        if (si == part) {
            si = PART_NONE;
            break;
        }
        si = pick_for_record(si, 0x1000);
        n++;
    }

    return n;
}

/*
 * 0x11d44
 *
 * Look a word up in the table that the **far** pointer at DGROUP 0x546c points
 * at, or answer 0 for the index -1. The table is outside DGROUP - it is in a
 * block DOS handed the program - which is why the port models the guest's
 * whole address space rather than only its data segment.
 */
int16_t part_by_index(int16_t index)
{
    if (index == -1)
        return 0;
    return (int16_t)PART_TABLE->part_ptr[(uint16_t)index];
}

/*
 * 0x11d66
 *
 * Make room for `n` parts: a far block of `n * 4` bytes from DOS for the table
 * at DGROUP 0x546c, and then `n` records of 0xa2 bytes off the near heap, one
 * put in each of its slots.
 *
 * The table is a **far** array of near pointers - four bytes an entry where the
 * pointer is two - and the game reaches it through `part_by_index`, which
 * is what makes a part number into a record. Two bytes of every four are not
 * written here and are whatever DOS left in the block.
 */
void alloc_part_table(int16_t n)
{
    int16_t si;

    DG546C.table = (dos_alloc_bytes((uint16_t)(n * 4), 0, 0).ptr);

    for (si = 0; si < n; si++)
        PART_TABLE->part_ptr[(uint16_t)si] =
            dg_near(dgroup, heap_calloc_far(1, 0xa2));
}

/*
 * 0x11db4
 *
 * Read one byte: `game_fread(buf, 1, 1, file)`, with the file first and the
 * buffer second - the same order round as `game_fread_far` beside it.
 *
 * It **answers what `fread` answered**, falling through with it in AX rather
 * than discarding it, which is how `read_line` below tells an empty line from
 * the end of the file.
 */
uint16_t game_fread_byte(FILE *file, uint8_t * buf)
{
    return game_fread(buf, 1, 1, file);
}

/*
 * 0x11dd1
 *
 * A far-callable two-byte read: `game_fread(buf, 2, 1, file)`, with the
 * arguments the other way round from `fread`'s own - the file first and the
 * buffer second.
 */
void game_fread_far(FILE *file, uint8_t * buf)
{
    game_fread(buf, 2, 1, file);
}

/*
 * 0x11dec
 *
 * Read a null-terminated string, a byte at a time, and **including** the null:
 * the loop reads first and tests afterwards, so the terminator is stored before
 * the test that stops on it. The buffer has to be big enough for the string the
 * file happens to hold; nothing here bounds it.
 */
void game_fread_string(FILE *file, char *buf)
{
    for (;;) {
        game_fread_byte(file, (uint8_t *)buf);
        if (*buf == 0)
            return;
        buf++;
    }
}

/*
 * 0x11e0b
 *
 * **Read one line.** Bytes into the buffer until a `\n` is seen, and then the
 * terminator goes at **`[si - 1]`** - over the byte *before* the newline, not
 * over the newline. That is not an off-by-one: the file has DOS line endings,
 * so the byte before the `\n` is the `\r`, and one store removes both.
 *
 * A file with Unix endings would therefore lose the last character of every
 * line. Nothing here checks.
 *
 * The very first read is the only one whose answer is looked at, and a zero
 * there - end of file - writes an empty string. So a caller loops until the
 * line comes back empty, and cannot tell that from a blank line in the file.
 * A blank line is also where the `[si - 1]` store writes one byte *below* the
 * buffer, because there is no `\r` in front of the `\n` to absorb it.
 */
void game_fread_line(FILE *file, char *buf)
{
    char *si = buf;

    if (game_fread_byte(file, (uint8_t *)si) == 0) {
        *si = 0;
        return;
    }

    while (*si != '\n') {
        si++;
        game_fread_byte(file, (uint8_t *)si);
    }

    si[-1] = 0;
}

/*
 * 0x11e3f
 *
 * Read one part out of a .gkc. `rec` is one of the 0xa2-byte records
 * `alloc_part_table` made in advance; this fills it from the file and then
 * hands it to its kind's own setup, so a part read off disk ends in the same
 * state as one `make_part` built.
 *
 * Most of it is a flat run of two-byte field reads, with four fields copied
 * from another rather than read - +8 from +0x94, +0x0c from +0x90, +0x12 from
 * +0x92, and the pair +0x42/+0x40 from +0x46/+0x44, which is the same "current
 * position becomes the previous one" that `make_part` ends with.
 *
 * Three things are not flat:
 *
 *  - **The rope.** A non-zero word read just before +0x56 means the part
 *    carries one: 0x38 bytes off the near heap at +0x54, whose +2 points back
 *    at the part and whose +4 and +6 are the two parts it ties together, each
 *    stored in the file as a part number and resolved through
 *    `part_by_index`. Each end that exists is pointed back at the rope
 *    through its own +0x54.
 *
 *  - **Two belts**, at +0x66 and +0x68. Each one present is 0x2c bytes off the
 *    near heap naming the two parts it runs between - at +2 and +4, copied
 *    again to +6 and +8 - and, in the bytes at +0x0a and +0x0b, which of each
 *    part's two belt slots it occupies, those two also copied to +0x0c and
 *    +0x0d. Each of those parts is pointed back at the belt through that slot.
 *    The pair of slot bytes is read into the part at +0x6a + 2i and +0x6b + 2i
 *    first, and the belt's own copies are read separately afterwards.
 *
 *  - **The version gate**, the word at DGROUP 0x5474. From 0x101 the file
 *    carries the field at +0x0a and four more part numbers into +0x62..+0x68;
 *    below it, a count and that many pairs of bytes are read and dropped on
 *    the floor. Either way the first two slots at +0x5a and +0x5c are read,
 *    and each is stored **twice**, into +0x5e and +0x60 as well.
 *
 * Kind 7 gets one extra part number, and takes that part's first belt as its
 * own second.
 *
 * The last two fields are not read at all: the count at +0x80 comes from the
 * kind's record at DGROUP 0xec4 + 0x3a * kind and the slots at +0x82 are
 * allocated from it, exactly as `part_init` does, before the far pointer at
 * +0x2a of the same record runs.
 */
void read_record_fields(FILE *file, struct part *rec)
{
    uint16_t v10;       /* [bp-0x10] */
    uint8_t v0b;                  /* [bp-0x0b] */
    int16_t v0a;       /* [bp-0x0a] */
    int16_t v08;       /* [bp-8] */
    int16_t v06;       /* [bp-6] */
    int16_t v04;       /* [bp-4] */
    int16_t v02;       /* [bp-2] */
    struct belt *di;

    game_fread_far(file, (uint8_t *)&rec->kind);
    game_fread_far(file, (uint8_t *)&rec->flags_06);
    game_fread_far(file, (uint8_t *)&rec->start_flags);
    rec->flags_08 = rec->start_flags;

    if (DG546C.version >= 0x101)
        game_fread_far(file, (uint8_t *)&rec->flags_0a);

    game_fread_far(file, (uint8_t *)&rec->start_form);
    rec->form = rec->start_form;

    game_fread_far(file, (uint8_t *)&rec->start_direction);
    rec->direction = rec->start_direction;

    game_fread_far(file, (uint8_t *)&rec->size[0].width);
    game_fread_far(file, (uint8_t *)&rec->size[0].height);
    rec->mirror_size.height = rec->size[0].height;
    rec->mirror_size.width = rec->size[0].width;

    game_fread_far(file, (uint8_t *)&rec->set_size.width);
    game_fread_far(file, (uint8_t *)&rec->set_size.height);
    game_fread_far(file, (uint8_t *)&rec->start_x);
    game_fread_far(file, (uint8_t *)&rec->start_y);
    game_fread_far(file, (uint8_t *)&rec->word_96);

    game_fread_far(file, (uint8_t *)&v02);
    game_fread_byte(file, (&rec->grab.x));
    game_fread_byte(file, (&rec->grab.y));
    game_fread_far(file, (uint8_t *)&rec->grab_size);

    if (v02 != 0) {
        struct rope *rope = (struct rope *)(void *)heap_calloc_far(1, 0x38);   /* [bp-0x0e] */

        rec->rope_ptr = dg_near(dgroup, rope);
        rope->owner_ptr = dg_near(dgroup, rec);

        game_fread_far(file, (uint8_t *)&v06);
        rope->end_a_ptr =
            (uint16_t)part_by_index((int16_t)v06);

        game_fread_far(file, (uint8_t *)&v06);
        rope->end_b_ptr =
            (uint16_t)part_by_index((int16_t)v06);

        if (rope->end_a_ptr != 0)
            PART_PTR(rope->end_a_ptr)->rope_ptr = dg_near(dgroup, rope);

        if (rope->end_b_ptr != 0)
            PART_PTR(rope->end_b_ptr)->rope_ptr = dg_near(dgroup, rope);
    }

    for (v0a = 0; v0a < 2; v0a++) {
        game_fread_far(file, (uint8_t *)&v04);
        game_fread_byte(file, (&rec->attach[(uint16_t)v0a].x));
        game_fread_byte(file, (&rec->attach[(uint16_t)v0a].y));

        if (v04 == 0)
            continue;

        di = (struct belt *)(void *)heap_calloc_far(1, 0x2c);
        rec->belt_ptr[(uint16_t)v0a] = dg_near(dgroup, di);
        BELT_PTR(rec->belt_ptr[(uint16_t)v0a])->owner_ptr = dg_near(dgroup, rec);

        game_fread_far(file, (uint8_t *)&v06);
        di->end_a_ptr =
            (uint16_t)part_by_index((int16_t)v06);
        di->home_a_ptr = di->end_a_ptr;

        game_fread_far(file, (uint8_t *)&v06);
        di->end_b_ptr =
            (uint16_t)part_by_index((int16_t)v06);
        di->home_b_ptr = di->end_b_ptr;

        game_fread_byte(file, &di->slot_a);
        di->home_slot_a = ((int8_t)di->slot_a);
        game_fread_byte(file, &di->slot_b);
        di->home_slot_b = ((int8_t)di->slot_b);

        if (di->end_a_ptr != 0)
            PART_PTR(di->end_a_ptr)->belt_ptr[(int8_t)di->slot_a] = dg_near(dgroup, di);

        if (di->end_b_ptr != 0)
            PART_PTR(di->end_b_ptr)->belt_ptr[(int8_t)di->slot_b] = dg_near(dgroup, di);
    }

    for (v0a = 0; v0a < 2; v0a++) {
        game_fread_far(file, (uint8_t *)&v06);
        rec->link_ptr[(uint16_t)v0a + 2] =
            (uint16_t)part_by_index((int16_t)v06);
        rec->link_ptr[(uint16_t)v0a] =
            rec->link_ptr[(uint16_t)v0a + 2];
    }

    if (DG546C.version >= 0x101) {
        for (v0a = 4; v0a < 6; v0a++) {
            game_fread_far(file, (uint8_t *)&v06);
            rec->link_ptr[(uint16_t)v0a] =
                (uint16_t)part_by_index((int16_t)v06);
        }
    }

    if (rec->kind == KIND_PULLEY) {
        game_fread_far(file, (uint8_t *)&v06);
        v10 = part_by_index((int16_t)v06);
        if (v10 != 0)
            rec->belt_ptr[1] =
                PART_PTR(v10)->belt_ptr[0];
    }

    if (DG546C.version <= 0x101) {
        game_fread_far(file, (uint8_t *)&v08);
        if (v08 != 0) {
            for (v0a = 0; v0a < v08; v0a++) {
                game_fread_byte(file, &v0b);
                game_fread_byte(file, &v0b);
            }
        }
    }

    rec->point_count = PART_KINDS[rec->kind].point_count;

    if (rec->point_count != 0)
        rec->points_ptr =
            dg_near(dgroup, heap_calloc_far(rec->point_count, 4));

    PART_KINDS[rec->kind].setup(rec);
}

/*
 * 0x1221b
 *
 * Read `n` things out of the file and put them on a list.
 *
 * DGROUP 0x5470 counts them, and each one's number is turned into its record by
 * `part_by_index` before being read into - so the records were made in
 * advance by `alloc_part_table` and this only fills them. `insert_sorted` puts
 * each on the list the caller named.
 *
 * **Only two of the three lists are sorted at all.** `insert_sorted` knows
 * 0x50d7 and 0x5179 and compares nothing for any other head, inserting at the
 * front - and the machine's own parts go on **0x521b**. So that list comes back
 * in exactly the *reverse* of the order the file holds, and writing it head
 * first emits the reverse again.
 *
 * That is measured, not inferred: a machine loaded and saved differs from the
 * file it came from in 280 of 740 bytes, and walking both by the record flags
 * gives kinds `15 39 2 5 2 2 2 50 21 1 1 3 8` in the file against
 * `8 3 1 1 21 50 2 2 2 5 2 39 15` in the save - the same parts, exactly turned
 * around. An earlier version of this comment said the lists "come out in the
 * order the file's contents demand", which is true of the two sorted ones and
 * false of this one.
 *
 * The list head is cleared first, both words of it.
 */
void read_list(FILE *file, struct part *head, int16_t n)
{
    int16_t di;

    head->prev_ptr = 0;
    head->next_ptr = 0;

    for (di = 0; di < n; di++) {
        uint16_t rec = (uint16_t)part_by_index((int16_t)DG546C.record_count);

        read_record_fields(file, PART_PTR(rec));
        insert_sorted(PART_PTR(rec), head);
        DG546C.record_count++;
    }
}

/*
 * 0x12269
 *
 * **Read a level file.** The name is opened, checked, unpacked field by field
 * into DGROUP, and closed; a file that does not open leaves everything as it
 * was and only the last line runs.
 *
 * The first word must be **0xaced** or the whole of the rest is skipped - the
 * file is still closed, and 0x50d3 is still pointed at the parts list, so a
 * corrupt level leaves the game with an empty machine rather than half of a
 * broken one.
 *
 * The flag at 0x5472 that `load_level` sets is what tells a *level* from a
 * saved machine. Set, the file also carries its title and hint at 0x4ecf and
 * 0x4f1f, the two counters at 0x50af and 0x50b1, and the origin pair at 0x50b7
 * and 0x50b9. Clear, all six are left as they are and only the parts are read.
 * So the same reader serves both, and one word decides which.
 *
 * The gravity and air pressure at 0x50b3 and 0x50b5 are always read, and
 * `recompute_kind_physics` is called immediately after them - not at the end -
 * so the three lists that follow are built against the settings the file
 * asked for rather than the ones the last level left behind.
 *
 * Three counts then arrive together and their **sum** is what the part table
 * is allocated for, once, before any of the three lists is read. The lists are
 * the machine's own parts at 0x521b, the moving ones at 0x5179, and - only
 * when 0x5472 says this is a level - the parts the player is given, at 0x50d7.
 *
 * The far pointer at 0x546c is freed at the end - whatever the list reader
 * left there - and a 0x216-byte buffer on the stack is handed to the file
 * first, which is a `setvbuf` and nothing to do with the level's contents.
 *
 * **Two callers, one routine.** `load_level` sets 0x5472 and asks for
 * "l<n>.lev"; `load_animation` (0x12915) clears it and asks for an animation,
 * which is why the two strings above are read on one path and not the other.
 * This was transcribed twice - once under each caller's name - and the copies
 * drifted: the second read only 0x4ecf where the original reads 0x4f1f as
 * well, and answered a fabricated 0. It never bit, because the caller that
 * skipped the string is the caller that clears 0x5472 and so never reaches
 * it. There is one `sub sp,0x216` in the image and there is one of these.
 */
uint16_t read_level(char *name)
{
    /*
     * **One slot has to be the guest's.** `buf` is the 0x210-byte stdio
     * buffer, and `game_setbuf` files its address into the file record's
     * `read_ptr` at +0x0a - a *guest word*, which the layer then steps as a
     * cursor, compares against `(uint16_t)(file + 5)` to tell a set buffer
     * from the record's own, and frees as a heap handle. Sixteen bits is the
     * whole of it and the port cannot promise a C object an address that fits.
     * The six count bytes below it are a C array, so the reservation is only
     * for the buffer.
     */
    uint16_t fp  = dg_alloca(0x216);
    uint16_t buf = fp;

    /* [bp-6], [bp-4], [bp-2]: three words `game_fread_far` fills, and nothing
       outside this routine ever sees their address. */
    _Alignas(2) uint8_t counts[6];
    FILE *file;
    uint16_t r;
    int16_t  n_machine, n_moving, n_given;

    file = game_fopen(name, GAME_FILE_NAMES.rb_read_level);
    if (file == 0) {
        DG50D3.bin_list_ptr = dg_near(dgroup, &DG50D3.parts_bin);
        dg_free(0x216);
        return 0;   /* AX is the failed `game_fopen`'s, which is 0 */
    }

    game_setbuf(file, dg_near_ptr(buf));
    game_fread_far(file, (uint8_t *)&DG546C.version_out);

    if (DG546C.version_out == 0xaced) {
        game_fread_far(file, (uint8_t *)&DG546C.version);

        if (DG546C.is_level != 0) {
            game_fread_string(file, (char *)DG4E67.title);
            game_fread_string(file, (char *)DG4E67.hint);
            game_fread_far(file, (uint8_t *)&DG50AF.bonus_1);
            game_fread_far(file, (uint8_t *)&DG50AF.bonus_2);
        }

        game_fread_far(file, (uint8_t *)&DG50AF.gravity);
        game_fread_far(file, (uint8_t *)&DG50AF.air);
        recompute_kind_physics();

        if (DG546C.is_level != 0) {
            game_fread_far(file, (uint8_t *)&DG50AF.extent_y);
            game_fread_far(file, (uint8_t *)&DG50AF.extent_x);
        }

        game_fread_far(file, (uint8_t *)&DG50AF.tune);

        game_fread_far(file, counts + 4);
        game_fread_far(file, counts + 2);
        game_fread_far(file, counts);
        n_machine = *(int16_t *)(counts + 4);
        n_moving  = *(int16_t *)(counts + 2);
        n_given   = *(int16_t *)(counts);

        DG546C.record_count = 0;
        alloc_part_table((int16_t)(n_machine + n_moving + n_given));

        read_list(file, &DG521B.placed_parts, n_machine);
        read_list(file, &DG5179.moving_parts, n_moving);
        if (DG546C.is_level != 0)
            read_list(file, &DG50D3.parts_bin, n_given);

        dos_free_far(DG546C.table);
    }

    r = game_fclose(file);
    DG50D3.bin_list_ptr = dg_near(dgroup, &DG50D3.parts_bin);

    /* The epilogue is `mov [0x50d3],0x50d7 / pop si / mov sp,bp / pop bp /
       retf` - nothing touches AX after the close, so the close's answer is
       the routine's. */
    dg_free(0x216);
    return r;
}

/*
 * 0x123b7
 *
 * **Write one byte**, and do nothing at all once the file has gone wrong.
 *
 * The error word 0x5478 is checked first and every writer checks it, so a
 * failure part way through a machine file does not have to be propagated: the
 * remaining hundreds of calls simply become no-ops and `write_level` finds the
 * word set when it gets to the end. That is why none of the writers answer
 * anything.
 */
void write_byte(FILE *file, const uint8_t * addr)
{
    if (DG546C.error != 0)
        return;

    if (game_fwrite(addr, 1, 1, file) != 1)
        DG546C.error = 1;
}

/*
 * 0x123e4
 *
 * **Write one word.** The same routine as `write_byte` with a size of 2, and
 * the original writes it out twice rather than sharing one - so this does too.
 */
void write_word(FILE *file, const uint8_t * addr)
{
    if (DG546C.error != 0)
        return;

    if (game_fwrite(addr, 2, 1, file) != 1)
        DG546C.error = 1;
}

/*
 * 0x12411
 *
 * **Write a string, and its terminator with it.** The loop writes the byte at
 * the pointer and *then* tests it, so the NUL goes to the file before the loop
 * ends - a reader has something to stop at. Written the other way round it
 * would be an off-by-one that only shows up when the file is read back.
 */
void write_string(FILE *file, char *str)
{
    for (;;) {
        write_byte(file, (const uint8_t *)str);
        if (*str == 0)
            return;
        str++;
    }
}

/*
 * 0x12430
 *
 * **Write one part's record.** Thirteen fields, then whatever the part is
 * attached to - and every attachment is written as a *`part_index`*, never a
 * pointer, so a reload can find the other end again.
 *
 * **Three of its locals have their addresses taken**, because `write_word`
 * writes from an address and the values here are computed rather than fields of
 * the part: whether there is a rope, whether there is a belt, and each index in
 * turn. So the port takes a guest frame for those three and keeps the rest as
 * ordinary locals - which is the same split `write_part_count` needed for its count.
 *
 * **The rope flag is written whether or not there is a rope**, and the belt flag
 * twice, once per slot. That is what makes the record fixed-width up to the
 * flags and self-describing after them: a reader takes the flag and knows
 * whether two more indices follow.
 *
 * The belt flag can only be true on the **first** slot - `i == 0` and the kind
 * being 0x0a or 7 - which is why the belt it then reads is at +0x66 flatly and
 * not at +0x66 + 2i. The second pass writes the flag as zero and the two bytes
 * at +0x6a and +0x6b, and nothing else.
 *
 * Then two runs over the link array: slots 0 and 1, then slots **4 and 5** -
 * skipping 2 and 3, which are the second half of the pairs `detach_belt` and
 * `finish_part_removal` clear together. A file that stored them would be storing the same
 * links twice.
 *
 * Last, and only for kind 7, the record at +0x68 - its first word as an index,
 * or 0xffff when there is none. That is the one place this writes 0xffff
 * itself; everywhere else it comes back from `part_index`.
 */
void write_record_fields(FILE *file, struct part *part)
{
    int16_t vindex;   /* [bp-6] */
    int16_t vbelt;   /* [bp-4] */
    int16_t vrope;/* [bp-2] */
    struct rope *rope;
    uint16_t belt;
    int16_t  i;

    write_word(file, (const uint8_t *)&part->kind);
    write_word(file, (const uint8_t *)&part->flags_06);
    write_word(file, (const uint8_t *)&part->start_flags);
    write_word(file, (const uint8_t *)&part->flags_0a);
    write_word(file, (const uint8_t *)&part->start_form);
    write_word(file, (const uint8_t *)&part->start_direction);
    write_word(file, (const uint8_t *)&part->size[0].width);
    write_word(file, (const uint8_t *)&part->size[0].height);
    write_word(file, (const uint8_t *)&part->set_size.width);
    write_word(file, (const uint8_t *)&part->set_size.height);
    write_word(file, (const uint8_t *)&part->start_x);
    write_word(file, (const uint8_t *)&part->start_y);
    write_word(file, (const uint8_t *)&part->word_96);

    vrope = (int16_t)(((int16_t)part->kind) == 8 ? 1 : 0);
    write_word(file, (uint8_t *)&vrope);

    write_byte(file, (const uint8_t *)&part->grab.x);
    write_byte(file, (const uint8_t *)&part->grab.y);
    write_word(file, (const uint8_t *)&part->grab_size);

    if ((uint16_t)vrope != 0) {
        rope = ROPE_PTR(part->rope_ptr);

        vindex = (int16_t)part_index(PART_PTR(rope->end_a_ptr));
        write_word(file, (uint8_t *)&vindex);
        vindex = (int16_t)part_index(PART_PTR(rope->end_b_ptr));
        write_word(file, (uint8_t *)&vindex);
    }

    for (i = 0; i < 2; i++) {
        vbelt = (int16_t)((i == 0
                                   && (((int16_t)part->kind) == 0x0a
                                       || ((int16_t)part->kind) == 7))
                                  ? 1 : 0);
        write_word(file, (uint8_t *)&vbelt);

        write_byte(file, &part->attach[i].x);
        write_byte(file, &part->attach[i].y);

        if ((uint16_t)vbelt != 0) {
            belt = part->belt_ptr[0];

            vindex = (int16_t)part_index(PART_PTR(BELT_PTR(belt)->end_a_ptr));
            write_word(file, (uint8_t *)&vindex);
            vindex = (int16_t)part_index(PART_PTR(BELT_PTR(belt)->end_b_ptr));
            write_word(file, (uint8_t *)&vindex);

            write_byte(file, dg_near_ptr((uint16_t)(belt + 0x0a)));
            write_byte(file, dg_near_ptr((uint16_t)(belt + 0x0b)));
        }
    }

    for (i = 0; i < 2; i++) {
        vindex = (int16_t)part_index(PART_PTR(part->link_ptr[i]));
        write_word(file, (uint8_t *)&vindex);
    }

    for (i = 4; i < 6; i++) {
        vindex = (int16_t)part_index(PART_PTR(part->link_ptr[i]));
        write_word(file, (uint8_t *)&vindex);
    }

    if (((int16_t)part->kind) == 7) {
        belt = part->belt_ptr[1];

        if (belt != 0)
            vindex = (int16_t)part_index(PART_PTR(BELT_PTR(belt)->owner_ptr));
        else
            vindex = (int16_t)0xffff;

        write_word(file, (uint8_t *)&vindex);
    }
}

/*
 * 0x126b3
 *
 * **Write every part of one list, and mark it as it goes.**
 *
 * The mark is bit 15 of +6 - the same bit `remove_all_parts` refuses to touch a
 * part over. List 2 is the bin at 0x50d7 and every part in it has the bit
 * *cleared*; lists 0 and 1 have it *set*, but only when DGROUP 0x5472 says this
 * is the long form of the file. So saving is what decides which parts a reload
 * will call the level's own and which the player's, and in the short form -
 * which is what the game itself saves - nothing is marked at all.
 *
 * The bit is set on the live part and not on a copy, so a save leaves the
 * machine in memory marked as well as the file.
 *
 * Takes the list's head cell, as `write_part_count` does.
 */
void write_part_list(FILE *file, struct part *head, uint16_t which)
{
    struct part *si;

    for (si = PART_PTR(head->next_ptr); si != PART_NONE; si = PART_PTR(si->next_ptr)) {
        if (which == 2)
            si->flags_06 &= 0x7fff;
        else if (DG546C.is_level != 0)
            si->flags_06 |= 0x8000;

        write_record_fields(file, si);
    }
}

/*
 * 0x126ec
 *
 * **Write how many parts a list holds**, by walking it and counting.
 *
 * The count goes into a *stack* local whose address is then handed to
 * `write_word` - which is why the port takes a guest frame for it rather than
 * using a C variable. Every field of this file is written from an address, and
 * a count that exists only for the length of this call is no exception.
 *
 * This is the first of the two passes each list gets: the count first, so a
 * reader knows how many of the records that `write_part_list` writes to expect.
 *
 * Takes the list's head cell, as the original does - `mov ax, 0x521b` in
 * `write_level`, then `mov si, [di]` here - and walks from the part it holds;
 * an empty list's head holds 0, and the walk ends on that offset.
 */
void write_part_count(FILE *file, struct part *head)
{
    int16_t vn;                   /* [bp-2] */
    struct part *si;

    vn = 0;
    for (si = PART_PTR(head->next_ptr); si != PART_NONE; si = PART_PTR(si->next_ptr))
        vn++;

    write_word(file, (uint8_t *)&vn);
}

/*
 * 0x1271c
 *
 * **The machine file writer.** `save_machine` is the doorway that puts the
 * dragged part down first; this is what opens the file and writes it. Answers
 * zero on success.
 *
 * The file starts with 0xaced and then 0x0102, a magic and a version, and both
 * are written *out of DGROUP* - set into 0x5476 and 0x5474 first and the address
 * passed - because everything else here is written the same way and the writer
 * takes an address, not a value.
 *
 * **DGROUP 0x5472 decides how much goes in.** Two groups of fields are written
 * only when it is set - 0x4ecf and 0x4f1f, then 0x50af and 0x50b1, and later
 * 0x50b7 and 0x50b9 - while 0x50b3, 0x50b5 and 0x50bb always go. `save_machine`
 * zeroes 0x5472 before calling, so a machine saved from the game gets the short
 * form and only whatever else sets that word gets the long one.
 *
 * Then the three part lists - 0x521b, 0x5179 and 0x50d7 - each written twice:
 * once by `write_part_count` and once by `write_part_list`, which also takes 0, 1 and 2. Two
 * passes over the same three lists, so the second can refer to what the first
 * wrote; the tag says which list it is reading back.
 *
 * **A file that fails to close is deleted.** The error word 0x5478 is set by a
 * non-zero close as well as by a failed open, and a set error word deletes the
 * file - so a half-written machine does not survive to be loaded. The open
 * failing returns 1 without touching the disk.
 *
 * 0x4e85 is 1 across the whole of it, the same "doing file IO" mark the load and
 * save handlers set around the picker.
 */
uint16_t write_level(char *name)
{
    FILE *f;

    DG546C.error = 0;
    DG546C.version_out = 0xaced;
    DG546C.version = 0x0102;
    DG4E67.file_op_active = 1;

    f = game_fopen(name, GAME_FILE_NAMES.wb_write_level);
    if (f == 0) {
        DG4E67.file_op_active = 0;
        return 1;
    }

    write_word(f, (const uint8_t *)&DG546C.version_out);
    write_word(f, (const uint8_t *)&DG546C.version);

    if (DG546C.is_level != 0) {
        write_string(f, (char *)DG4E67.title);
        write_string(f, (char *)DG4E67.hint);
        write_word(f, (const uint8_t *)&DG50AF.bonus_1);
        write_word(f, (const uint8_t *)&DG50AF.bonus_2);
    }

    write_word(f, (const uint8_t *)&DG50AF.gravity);
    write_word(f, (const uint8_t *)&DG50AF.air);

    if (DG546C.is_level != 0) {
        write_word(f, (const uint8_t *)&DG50AF.extent_y);
        write_word(f, (const uint8_t *)&DG50AF.extent_x);
    }

    write_word(f, (const uint8_t *)&DG50AF.tune);

    write_part_count(f, &DG521B.placed_parts);
    write_part_count(f, &DG5179.moving_parts);
    write_part_count(f, &DG50D3.parts_bin);

    write_part_list(f, &DG521B.placed_parts, 0);
    write_part_list(f, &DG5179.moving_parts, 1);
    write_part_list(f, &DG50D3.parts_bin, 2);

    if (game_fclose(f) != 0)
        DG546C.error = 1;

    if (DG546C.error != 0)
        dos_unlink(name);

    DG4E67.file_op_active = 0;
    return DG546C.error;
}

/*
 * 0x12863
 *
 * Load a level by number: build its name and hand it to `read_level`.
 *
 * The name is assembled a piece at a time out of DGROUP - "l" at 0x2876, the
 * number in decimal, ".lev" at 0x2878 - into a 0x16-byte buffer on the stack.
 * `round_setup` passes the round count at 0x4ebd, so the first round asks for
 * "l1.lev", which is the name the resource archive holds.
 *
 * The flag at 0x5472 is set to 1 before the read and is not cleared here.
 */
void load_level(uint16_t number)
{
    char name[14];
    char digits[8];

    string_copy(name, GAME_FILE_NAMES.l_load_level);
    int_to_string((int16_t)number, digits, 10);
    string_concat(name, digits);
    string_concat(name, GAME_FILE_NAMES.lev_load_level);

    DG546C.is_level = 1;
    read_level(name);
}

/*
 * 0x12915
 *
 * Load an animation file: build the part list first, clear DGROUP 0x5472, and
 * read it. Every load in the image comes here - the title and credits
 * animations, freeform's `ff.lev`, and the file picker.
 *
 * **The bin is `build_part_list`'s and stays so.** With 0x5472 clear,
 * `read_level` reads a file's placed and moving lists but not its given one,
 * so the bin after a load is freeform's one-of-every-kind. This comment once
 * said a routine three bytes below loaded while *preserving* 0x50d7; those
 * bytes are the tail of the routine before, which ends by calling the machine
 * writer at 0x1271c.
 */
uint16_t load_animation(char *name)
{
    build_part_list();
    DG546C.is_level = 0;

    return read_level(name);
}

/*
 * 0x1292d
 *
 * **Write the machine out**, given the name the picker left at DGROUP 0x52fe.
 * Answers zero on success - the caller shows "FILE ERROR" and asks again for
 * anything else, so what comes back is a reason and not a count.
 *
 * The writing is `write_level`; what this adds is that **the dragged part is put
 * down first**. DGROUP 0x50d7 is saved, zeroed for the length of the write and
 * put back after, so a part in mid-drag is not written as held - the file has
 * no way to say "and this one is in the player's hand", and reloading it would
 * have to invent somewhere to put it. 0x5472 is zeroed with it and not restored.
 *
 * The `jmp` to the next instruction at 0x12959 is the compiler leaving itself a
 * single exit; transcribed as the fall-through it is.
 */
uint16_t save_machine(char *name)
{
    uint16_t held = DG50D3.parts_bin.next_ptr;
    uint16_t r;

    DG50D3.parts_bin.next_ptr = 0;
    DG546C.is_level = 0;

    r = write_level(name);

    DG50D3.parts_bin.next_ptr = held;
    return r;
}

/*
 * 0x1295f
 *
 * **Is this file one of ours?** It opens the name, reads one word, and answers
 * whether that word is **0xaced** - the machine file's magic, and the only
 * check made before the loader is trusted with the rest.
 *
 * Both exits close the file, and the failure exit closes it *even when the open
 * failed*, handing `fclose` the zero it just tested. That is what the original
 * does; the runtime's `fclose` looks the pointer up rather than following it,
 * so it is a wasted call rather than a fault.
 */
uint16_t is_machine_file(char *name)
{
    int16_t magic;                /* [bp-2] */
    FILE *file;
    uint16_t ok    = 0;

    file = game_fopen(name, GAME_FILE_NAMES.rb_is_machine_file);

    if (file != 0) {
        game_fread_far(file, (uint8_t *)&magic);
        if ((uint16_t)magic == 0xaced)
            ok = 1;
    }

    game_fclose(file);
    return ok;
}

/*
 * 0x129a8
 *
 * Count the level files, and leave the count at DGROUP 0x4eb9.
 *
 * It builds "l", the number, ".lev" and tries to open it, climbing from 1 until
 * one is missing - so the answer is one *past* the last that opened, and the
 * decrement on the failing try is what turns that back into a count. Each file
 * that opens is closed again immediately; nothing is read.
 *
 * The name is assembled in a stack buffer whose address is passed on. That
 * used to mean a real DGROUP frame; it stopped meaning it when `game_fopen`
 * and the string routines took pointers, and the buffer is a C array.
 */
void count_level_files(void)
{
    char name[16];                         /* [bp-0x18] */
    char number[8];    /* [bp-8]    */
    int16_t done = 0;

    DG4E67.level_count = 1;

    while (done == 0) {
        FILE *file;

        string_copy(name, GAME_FILE_NAMES.l_count_levels);
        int_to_string((int16_t)((uint16_t)DG4E67.level_count),
                      number, 10);
        string_concat(name, number);
        string_concat(name, GAME_FILE_NAMES.lev_count_levels);

        file = game_fopen(name, GAME_FILE_NAMES.rb_count_levels);

        if (file != 0) {
            DG4E67.level_count++;
            game_fclose(file);
        } else {
            DG4E67.level_count--;
            done = 1;
        }
    }
}

/*
 * 0x12a2f
 *
 * **A puzzle's title, out of its own level file.** The name is built rather
 * than looked up - `"l"`, the number, `".lev"` - so puzzle 7 is `l7.lev` and
 * there is no table anywhere saying so.
 *
 * The file is checked with the same 0xaced `is_machine_file` looks for, and
 * then **one word is read and thrown away** before the title. Nothing here says
 * what it is; the title is what follows it.
 *
 * A missing file, or a wrong magic, answers 0 - which is what stops the list
 * drawer, so the number of puzzles is however many files are actually there.
 */
uint16_t get_puzzle_title(int16_t n, char *buf)
{
    char name[14];                 /* [bp-0x1a] */
    char num[8]; /* [bp-0x0c] */
    uint8_t skip[2]; /* [bp-4]    */
    int16_t magic; /* [bp-2]   */
    FILE *file;
    uint16_t ok = 0;

    string_copy(name, GAME_FILE_NAMES.l_puzzle_title);
    int_to_string(n, num, 10);
    string_concat(name, num);
    string_concat(name, GAME_FILE_NAMES.lev_puzzle_title);

    file = game_fopen(name, GAME_FILE_NAMES.rb_puzzle_title);

    if (file != 0) {
        game_fread_far(file, (uint8_t *)&magic);

        if ((uint16_t)magic != 0xaced) {
            game_fclose(file);
        } else {
            game_fread_far(file, skip);
            game_fread_string(file, buf);
            game_fclose(file);
            ok = 1;
        }
    }
    return ok;
}

/*
 * 0x12ad0
 *
 * **A password into a level number**, by finding it in `password.txt`.
 *
 * The text is upper-cased in place first, and then **cut at the first `-`** -
 * a NUL is written over it - so a code of the form `WORD-SCORE` matches on the
 * word alone. The dash is put back before the routine answers, because the same
 * buffer is about to be handed to the score decoder, which wants the half this
 * one just hid.
 *
 * The line counter starts at **1 and is incremented before the comparison**, so
 * a match on the file's first line answers 2. Whether that is deliberate or an
 * off-by-one cannot be told from here - it is consistent, so a password file
 * written to suit it works.
 *
 * The loop cannot tell a blank line from the end of the file, because
 * `game_fread_line` reports both as an empty buffer.
 *
 * Not found is 0xffff, and a file that will not open leaves it at that without
 * reading anything.
 */
uint16_t password_to_level(char *text)
{
    char line[26];                    /* [bp-0x1a] */
    char *dash;
    FILE *file;
    int16_t  n      = 1;                    /* [bp-4] */
    int16_t  answer = -1;                   /* [bp-2] */

    string_upper(text);

    dash = string_chr(text, '-');
    if (dash != NULL)
        *dash = 0;

    file = game_fopen((char *)GAME_FILE_NAMES.password_txt_level, GAME_FILE_NAMES.rb_password_level);

    if (file != 0) {
        game_fread_line(file, line);

        while ((*line) != 0) {
            n++;

            if (string_compare_nocase(text, line) == 0)
                answer = n;

            game_fread_line(file, line);
        }

        game_fclose(file);
    }

    if (dash != NULL)
        *dash = '-';
    return (uint16_t)answer;
}

/*
 * 0x12b60
 *
 * Read the `count`th line of **password.txt** into `buf`.
 *
 * The file has one password a line and this wants the one for a level, so it
 * reads `count` lines and keeps only the last - the buffer is written over
 * each time round. There is no seek and no index; the lines are found by
 * reading past them.
 *
 * `buf` is emptied first, so a missing file leaves an empty string rather than
 * whatever was there: the open is tested and everything else skipped.
 *
 * The loop decrements *before* it reads, and its test is at the top, so a
 * count of zero reads nothing at all and any other count reads exactly that
 * many lines.
 */
void read_password_line(int16_t count, char *buf)
{
    FILE *f;

    *buf = 0;

    f = game_fopen((char *)GAME_FILE_NAMES.password_txt_line, GAME_FILE_NAMES.rb_password_line);
    if (f == 0)
        return;

    while (count != 0) {
        count--;
        game_fread_line(f, buf);
    }

    game_fclose(f);
}

/*
 * 0x12ba7
 *
 * Read `TIM.CFG`: two words, into DGROUP 0x4eb7 and 0x4ec1. Answers 1 if the
 * file was there and 0 if it was not.
 *
 * The name is the string at DGROUP 0x28bb and the mode the one at 0x28c3. Both
 * reads go through `game_fread_far`, which takes its file first and buffer
 * second, and the file is closed on the success path only - a failed open has
 * nothing to close.
 */
uint16_t read_tim_cfg(void)
{
    FILE *file = game_fopen((char *)GAME_FILE_NAMES.tim_cfg_read, GAME_FILE_NAMES.rb_tim_cfg);

    if (file == 0)
        return 0;

    game_fread_far(file, (uint8_t *)&DG4E67.furthest_level);
    game_fread_far(file, (uint8_t *)&DG4E67.master_level);
    game_fclose(file);

    return 1;
}

/*
 * 0x12bed
 *
 * **Writes `tim.cfg`** - the whole of the game's saved state between runs, and
 * it is two words: the furthest level reached at DGROUP 0x4eb7 and the sound
 * level at 0x4ec1. Nothing else survives quitting.
 *
 * It writes them with `write_word`, the same routine the machine files use, so
 * the file's four bytes are in the same byte order as everything else the game
 * writes. A failed open is silently nothing - the settings just do not persist.
 */
void write_config(void)
{
    FILE *file = game_fopen((char *)GAME_FILE_NAMES.tim_cfg_write, GAME_FILE_NAMES.wb_tim_cfg);

    if (file != 0) {
        write_word(file, (const uint8_t *)&DG4E67.furthest_level);
        write_word(file, (const uint8_t *)&DG4E67.master_level);
        game_fclose(file);
    }
}

/*
 * 0x12c26
 *
 * **The file picker**, and a whole screen with its own loop. It answers 1 when
 * the player chose a file, leaving the name at DGROUP 0x52fe where
 * `load_animation` is then given it.
 *
 * **The mode word 0x4e6b is the whole of the state machine**, exactly as it is
 * in `game_screen`: the regions the pointer is over write it, this loop reads
 * it, and the jump table at 0x1318d dispatches on it. The picker borrows the
 * word, keeping what it displaced at 0x568f - which is also how every drawing
 * routine below knows whether it is a LOAD or a SAVE without being told.
 *
 * **`was`, the previous pass's mode, is what makes the text fields work.** A
 * field commits when it *stops* having focus, and by then 0x4e6b no longer says
 * so - the only place the old value survives is this local, taken at the bottom
 * of the loop. So both field blocks are entered when either the current or the
 * previous mode is theirs.
 *
 * **A full repaint suppresses the partial ones.** The five counters below are
 * not cleared by it; they are simply not acted on in a pass that repainted
 * everything, and stay pending for the next one. Drawing them again over a
 * fresh screen would be redundant, and the counters exist so that a redraw
 * asked for while a message box was up is not lost.
 *
 * Two of its calls are into `dos_chdir` and `dos_setdisk`, which are stubs in
 * this port for the reason given at each: there is nowhere to change to. So the
 * picker draws, scrolls and types, and reaches a stub the moment a directory is
 * actually chosen.
 */
uint16_t pick_file(uint16_t arg1, uint16_t arg2, const char *pattern)
{
    char pat[38];                  /* [bp-0x26], 0x26 bytes */

    int16_t  reload    = 2;             /* [bp-6]    */
    int16_t  idx       = 0;             /* [bp-0xa]  */
    int16_t  rp_up     = 0;             /* [bp-0xc]  */
    int16_t  rp_down   = 0;             /* [bp-0xe]  */
    int16_t  rp_list   = 0;             /* [bp-0x10] */
    int16_t  rp_file   = 0;             /* [bp-0x12] */
    int16_t  rp_name   = 0;             /* [bp-0x14] */
    int16_t  valid;                     /* [bp-0x16] */
    int16_t  repaint   = 0;             /* si */
    uint16_t was       = 0x8000;        /* di */
    uint16_t answer;

    /*
     * **The pattern is the third argument, and it is copied.** `fill_file_listing`
     * takes it apart to build the extension filter and `string_chr` walks it in
     * place, so what the listing filters on is this copy and never the caller's
     * constant.
     */
    string_copy(pat, pattern);

    DG4E4E.name_buf[0] = 0;
    GAME_PICKER_TEXT.picker_mode = DG4E67.state;
    DG4E67.state = 0x8000;

    for (;;) {
        if (reload != 0) {
            picker_begin(arg1, arg2, pat);

            if (((uint16_t)GAME_PICKER_TEXT.entry_max) == 0) {
                answer = 0;
                goto out;
            }

            reload  = 0;
            repaint = 1;
        }

        update_button_state();
        DG52ED.last_key = (uint8_t)bios_read_key();

        if ((DG52ED.last_key) == '\t' && DG4E67.state != 0x4000
            && DG4E67.state != 0x1000)
            picker_tab();

        if (((DG52ED.last_key) == '\r' || (DG52ED.last_key) == ' '
             || (DG52ED.last_key) == 0x1b /* Esc */)
            && DG4E67.state == 0x4000)
            DG5768.button_left = 0;

        regions_handle_pointer(DG4E67.regions_c_ptr);

        if (DG4E67.state == 0x100)
            goto dispatch;

        /*
         * **The path field.** It is entered while the field has focus *or* had
         * it on the pass before - `was` is last pass's 0x4e6b, taken at the
         * bottom of the loop - because losing focus is what commits the typed
         * path, and by then 0x4e6b no longer says the field.
         */
        if (DG4E67.state == 0x4000 || was == 0x4000) {
            DG4E67.file_op_active = 1;

            if (((DG52ED.last_key) == '\r' || DG4E67.state != 0x4000)
                && was == 0x4000) {
                /*
                 * A path of exactly `X:` skips the first `chdir` and goes
                 * straight to the second. Every other path is handed to
                 * `chdir` **twice** - once to find out whether it is reachable
                 * and once to go there - and the drive is selected only after
                 * the second succeeds.
                 */
                if ((GAME_DIRECTORIES.path_field[1] == ':' && GAME_DIRECTORIES.path_field[2] == 0)
                    || dos_chdir((const char *)GAME_DIRECTORIES.path_field) == 0) {
                    if (dos_chdir((const char *)GAME_DIRECTORIES.path_field) == 0) {
                        dos_setdisk((uint8_t)GAME_DIRECTORIES.path_field[0]);
                        reload = 2;
                    } else {
                        dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);
                        show_message_box(DG1BCC.path_error, (char *)DG1BCC.path_error_body);
                        wait_cursor();
                        paint_panel_frame();
                        restore_cursor();
                        repaint = 1;
                        rp_name = 2;
                    }

                    if (DG4E67.state == 0x4000)
                        DG4E67.state = 0x8000;
                } else {
                    dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);
                    show_message_box(DG1BCC.path_error, (char *)DG1BCC.path_error_body);
                    wait_cursor();
                    paint_panel_frame();
                    restore_cursor();
                    repaint = 1;
                    rp_name = 2;

                    if (DG4E67.state == 0x4000)
                        DG4E67.state = 0x8000;
                }
            } else {
                if (was == 0x4000)
                    picker_type((DG52ED.last_key),
                        (char *)GAME_DIRECTORIES.path_field, 0x50);

                rp_name = 2;
            }

            DG4E67.file_op_active = 0;
        }

        /*
         * The name field, the same shape and a different buffer. The original
         * tests the mode **twice** on the way in - once for each half of the
         * `||` - and the second test can never fail once the first has let it
         * through, so the branch it guards is unreachable and is not written
         * out here.
         */
        if (DG4E67.state == 0x1000 || was == 0x1000) {
            if (((DG52ED.last_key) == '\r' || DG4E67.state != 0x1000)
                && was == 0x1000) {
                force_extension((char *)DG4E4E.name_buf, GAME_FILE_STRINGS.tim);

                if (DG4E67.state == 0x1000)
                    DG4E67.state = 0x8000;
            } else if (was == 0x1000) {
                picker_type((DG52ED.last_key), (char *)DG4E4E.name_buf,
                            sizeof DG4E4E.name_buf);
            }

            rp_file = 2;
        }

    dispatch:
        switch (DG4E67.state) {
        case 0x0800:                    /* the up arrow */
            if (DG5768.button_left == 1 || DG5768.button_left == 2) {
                int16_t v = (int16_t)(GAME_PICKER_TEXT.scroll - 1);

                if (v >= 0) {
                    GAME_PICKER_TEXT.scroll = v;
                    rp_list = 2;
                }
            } else {
                DG4E67.state = 0x8000;
            }
            rp_up = 2;
            break;

        case 0x0400:                    /* the down arrow */
            if (DG5768.button_left == 1 || DG5768.button_left == 2) {
                int16_t v = (int16_t)(GAME_PICKER_TEXT.scroll + 1);

                if ((int16_t)(GAME_PICKER_TEXT.entry_count - 12) >= v) {
                    GAME_PICKER_TEXT.scroll = v;
                    rp_list = 2;
                }
            } else {
                DG4E67.state = 0x8000;
            }
            rp_down = 2;
            break;

        case 0x2000: {                  /* a click in the listing */
            const char far *rec;

            /*
             * **Which row was clicked is arithmetic, not a hit test.** The
             * pointer's y at 0x5782 less the box's top, divided by the ten
             * pixels a row takes, plus the scroll position.
             */
            idx = (int16_t)((int16_t)(DG5768.pointer_y - 0x7c) / 10
                            + GAME_PICKER_TEXT.scroll);

            if (idx >= GAME_PICKER_TEXT.entry_count) {
                DG4E67.state = 0x8000;
                break;
            }

            /* Entry `idx` of the array of far pointers at the block's
               front, followed to the text it names. */
            {
                char far * far *entries =
                    (char far * far *)GAME_PICKER_TEXT.block;

                rec = entries[idx];
            }

            if (*rec != ':' && *rec != '<') {
                string_copy((char *)DG4E4E.name_buf, listing_to_name(rec));
                rp_file = 2;
                DG4E67.state = 0x8000;
                break;
            }

            /*
             * Row zero is the `:` when there is one, and the only way to tell
             * it from a directory called nothing is that we are not at a root.
             */
            if (idx != 0 || path_is_root((const char *)GAME_DIRECTORIES.path_field) != 0)
                path_join((char *)GAME_DIRECTORIES.path_field, rec);
            else
                path_up((char *)GAME_DIRECTORIES.path_field);

            DG4E67.file_op_active = 1;

            if (dos_chdir((const char *)GAME_DIRECTORIES.path_field) == 0)
                dos_setdisk((uint8_t)GAME_DIRECTORIES.path_field[0]);

            DG4E67.file_op_active = 0;
            reload = 2;
            DG4E4E.name_buf[0] = 0;
            DG4E67.state = 0x8000;
            break;
        }

        case 0x0200:                    /* the LOAD or SAVE button */
            valid = (int16_t)validate_filename();

            if (valid == 0) {
                picker_draw_action();
                show_message_box(DG1BCC.file_error,
                                 ((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100
                                     ? (char *)DG1BCC.cant_open_for_loading
                                     : (char *)DG1BCC.cant_open_for_saving);
                wait_cursor();
                paint_panel_frame();
                restore_cursor();
                repaint = 1;
                DG4E67.state = 0x8000;
            } else if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x80) {
                if (valid == 2) {
                    picker_draw_action();

                    if (ask_yes_no(DG1BCC.overwrite_file, (char *)DG1BCC.overwrite_body)
                        == 0) {
                        wait_cursor();
                        paint_panel_frame();
                        restore_cursor();
                        repaint = 1;
                        DG4E67.state = 0x8000;
                    }
                }
            } else if (is_machine_file((char *)DG4E4E.name_buf) == 0) {
                picker_draw_action();
                show_message_box(DG1BCC.wrong_format, (char *)DG1BCC.wrong_format_body);
                wait_cursor();
                paint_panel_frame();
                restore_cursor();
                repaint = 1;
                DG4E67.state = 0x8000;
            }
            break;

        default:
            break;
        }

        was = DG4E67.state;

        /*
         * **A whole repaint is not one of the partial ones.** When it happens
         * every pending partial redraw is left pending and the pass ends - the
         * full paint has already drawn all of them, and running them again
         * would draw over what it just put down.
         */
        if (repaint != 0) {
            picker_repaint();
            repaint--;
        } else {
            if (rp_up != 0) {
                picker_draw_up();
                rp_up--;
            }
            if (rp_down != 0) {
                picker_draw_down();
                rp_down--;
            }
            if (rp_list != 0) {
                picker_draw_list();
                rp_list--;
            }
            if (rp_name != 0) {
                picker_draw_name();
                rp_name--;
            }
            if (rp_file != 0) {
                picker_draw_filename();
                rp_file--;
            }

            present_frame(1);
        }

        if (DG4E67.state == 0x200 || DG4E67.state == 0x100)
            break;
    }

    /*
     * The listing block goes back **only when it is not the borrowed one**:
     * `picker_begin` will take the pointer at 0x3576 if there is one, and
     * freeing that would hand back memory the picker never owned.
     */
    if (GAME_PICKER_TEXT.block != DG3576.scratch) {
        dos_free_far(GAME_PICKER_TEXT.block);
        GAME_PICKER_TEXT.block = NULL;
        GAME_PICKER_TEXT.text_start = NULL;
    }

    picker_draw_action();

    if (DG4E67.state == 0x200 && string_length((const char *)DG4E4E.name_buf) != 0) {
        string_copy((char *)DG52FE.name, (const char *)DG4E4E.name_buf);
        answer = 1;
    } else {
        DG4E4E.name_buf[0] = 0;
        answer = 0;
    }

out:
    return answer;
}

/*
 * 0x1319d
 *
 * **Is the typed name usable?** Three answers, not two: 0 for no, 1 for a name
 * that is free to create, and **2 for one that already exists** - which the
 * caller needs to tell apart so it can ask before overwriting.
 *
 * The rejections, in the order they are made:
 *
 *   an empty name, or one starting with `.`; a space anywhere in the stem - the
 *   scan stops at the first `.`, so spaces in an extension are not looked at;
 *   any of the fourteen characters in the table at DGROUP 0x28ec, which are
 *   `*` `/` `,` `-` `[` `]` `&` `@` `^` `%` `?` `(` `)` `:`; and any of eleven
 *   reserved DOS device names.
 *
 * A device name only counts when it is the *whole stem* - the byte after it has
 * to be the terminator or a `.` - which is why `CONFIG.TIM` survives and
 * `CON.TIM` does not.
 *
 * **The eleventh entry is wrong in the original.** "null" is compared for
 * **three** bytes - so it tests the same `nul` the tenth entry does - but checks
 * the byte at +4 rather than +3. The effect is that *any* four-letter stem
 * beginning `NUL` is rejected: `NULA` and `NULX` as much as `NULL`. Written as
 * `strnicmp(name, "null", 4)`, which is plainly what was meant, it would have
 * caught `NULL` alone. Transcribed as it is.
 *
 * Last, it *opens the file* to find out whether it is there, and closes it
 * again. A name that opens answers 2. One that does not answers 1 only when
 * 0x568f says 0x80 - the mode the picker was opened from - and 0 otherwise, so
 * asking to load something that is not there is a rejection rather than an
 * answer the caller has to interpret.
 */
uint16_t validate_filename(void)
{
    char    *si;
    uint16_t i;
    FILE *file;
    int16_t  bad = 0;

    si = (char *)DG4E4E.name_buf;

    if (*si == 0)
        bad = 1;
    if (*si == '.')
        bad = 1;

    while (*si != 0 && *si != '.') {
        if (*si == ' ')
            bad = 1;
        si++;
    }

    if (bad)
        return 0;

    for (i = 0; i < 0x0e; i++) {
        if (string_chr((char *)DG4E4E.name_buf,
                       GAME_FORBIDDEN_CHARS.forbidden[i]) != NULL)
            return 0;
    }

    for (i = 0; i < sizeof reserved_names / sizeof reserved_names[0]; i++) {
        uint16_t after;

        if (string_ncompare_i((const char *)DG4E4E.name_buf, reserved_names[i].name,
                              reserved_names[i].len) != 0)
            continue;

        after = (uint8_t)DG4E4E.name_buf[reserved_names[i].after];
        if (after == 0 || after == '.')
            return 0;
    }

    file = game_fopen((char *)DG4E4E.name_buf, GAME_FILE_STRINGS.mode_rb);

    if (file != 0) {
        game_fclose(file);
        return 2;
    }

    if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x80)
        return 1;

    return 0;
}

/*
 * 0x13402
 *
 * **Redraw the picker's one action button.** Which word it carries is not a
 * parameter: it is read back out of the mode word DGROUP 0x4e6b, and when that
 * says 0x200 - the picker's own mode - out of *0x568f*, the value 0x4e6b held
 * before the picker took it. So the button says LOAD or SAVE according to which
 * handler opened the picker, and the picker itself does not have to be told.
 *
 * Anything else says CANCEL, and it moves: 0xc0 against 0x40. The two are
 * different buttons in the same place in the code, not one button relabelled.
 */
void picker_draw_action(void)
{
    if (DG4E67.state != 0x200) {
        draw_button(DG1BCC.cancel, 0xc0, 0x130, 1);
    } else if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100) {
        draw_button(DG1BCC.load, 0x40, 0x130, 1);
    } else {
        draw_button(DG1BCC.save, 0x40, 0x130, 1);
    }

    present_back_page();
}

/*
 * 0x1345f
 *
 * **Tab inside the picker**, and the same trick as the panel's at 0x1156c: it
 * warps the pointer rather than moving any focus. Seven stops, cursor at DGROUP
 * 0x28fa, x at 0x28fc and y at 0x290a - and here the two tables are the same
 * length, because none of the picker's controls is a slider whose position has
 * to be worked out from a value.
 */
void picker_tab(void)
{
    GAME_PICKER_TABS.stop++;

    if (GAME_PICKER_TABS.stop == 7)
        GAME_PICKER_TABS.stop = 0;

    move_pointer_to(GAME_PICKER_TABS.stop_x[GAME_PICKER_TABS.stop],
                    GAME_PICKER_TABS.stop_y[GAME_PICKER_TABS.stop]);
}

/*
 * 0x13490
 *
 * **One keystroke into the picker's name field.** Backspace - 8 - takes the
 * last byte off, and does nothing on an empty field. Anything else is appended
 * *as a string*: the character is stored into a two-byte local with a NUL after
 * it and handed to `strcat`, which is why this routine has locals at all.
 *
 * Two characters never reach the field: backspace, which is handled above, and
 * **tab**, which is excluded explicitly. Tab is a key the picker wants for
 * moving the pointer, and a field that swallowed it would take it away.
 *
 * The length check is `< max`, and `max` counts the NUL's room the way the
 * caller passed it - this routine does not add one.
 */
void picker_type(uint8_t c, char *buf, int16_t max)
{
    char str[2];                  /* [bp-2], the two-byte string */
    int16_t  len;

    str[0] = (char)c;
    str[1] = 0;

    len = (int16_t)string_length(buf);

    if (c == '\b') {
        if (len != 0)
            buf[len - 1] = 0;
    } else if (len < max && c != '\t') {
        string_concat(buf, str);
    }
}

/*
 * 0x134dd
 *
 * **Is this path a drive's root?** One separator in the whole string, and it is
 * the last byte - "C:\\" and nothing else. It counts the same way `path_up`
 * does, against the same shared "\\" at DGROUP 0x1bca, which is what keeps the
 * two agreeing about where the walk up has to stop.
 */
uint16_t path_is_root(const char *path)
{
    const char *si = path;
    const char *last = 0;
    int16_t  n = 0;
    char sep = *(const char *)dg_near_ptr(GAME_PATH_SEP.path_sep_ptr);

    while (*si != 0) {
        if (*si == sep) {
            last = si;
            n++;
        }
        si++;
    }

    if (n == 1 && last[1] == 0)
        return 1;

    return 0;
}

/*
 * 0x13516
 *
 * **Drop the last component of a path**, in place. It walks to the terminator
 * counting separators - the character is not a literal here but `*GAME_PATH_SEP.path_sep_ptr`,
 * the one-character string "\\" the rest of the module shares - and remembers
 * the last one it saw.
 *
 * The two cases differ by one byte, and that byte is the whole point: with a
 * single separator the cut is *after* it, leaving "C:\\", because a drive with
 * its backslash taken off means the current directory rather than the root.
 * With more than one it cuts *at* the separator, leaving the parent. With none
 * it does nothing at all.
 */
void path_up(char *path)
{
    char *si = path;
    char *last = 0;
    int16_t  n = 0;
    char sep = *(const char *)dg_near_ptr(GAME_PATH_SEP.path_sep_ptr);

    while (*si != 0) {
        if (*si == sep) {
            last = si;
            n++;
        }
        si++;
    }

    if (n == 1)
        last[1] = 0;
    else if (n > 1)
        *last = 0;
}

/*
 * 0x1354c
 *
 * **Join a listed name onto the path.** The name comes in as a *far* pointer -
 * it is in the picker's own list block, not DGROUP - and the path is near, so
 * the name is copied through a fourteen-byte local first.
 *
 * That copy is off by one at both ends, deliberately, and `fill_file_listing` is what
 * makes it right: a directory is written into the listing as `<NAME>`. This
 * stores from the **second** byte, past the `<`, and after the join chops the
 * **last** byte, the `>`. So `<DOS>` arrives and `\\DOS` leaves.
 *
 * The separator goes in only when the path is not already a root, because a
 * root already ends in one and `path_is_root` is the routine that knows.
 *
 * The loop tests the byte *before* stepping and stores the byte *after*, so the
 * NUL is copied along with the rest and the local needs no terminating of its
 * own.
 */
void path_join(char *path, const char far * entry)
{
    char name[14];                            /* [bp-0xe] */
    uint16_t di   = 0;
    uint16_t len;

    while (*entry != 0) {
        entry++;
        name[di] = *entry;
        di++;
    }

    if (path_is_root(path) == 0)
        string_concat(path, (const char *)dg_near_ptr(GAME_PATH_SEP.path_sep_ptr));

    string_concat(path, name);

    len = string_length(path);
    path[len - 1] = 0;
}

/*
 * 0x135a6
 *
 * **Force a name into 8.3.** The eighth byte is cut off first, unconditionally
 * and before anything is looked at, so a long name loses its tail rather than
 * its extension. Then the first `.` - or the terminator, if there is none - is
 * where the new one goes, and the extension the caller passed is appended.
 *
 * An empty name is left empty: the cut at byte 8 has already happened, but
 * nothing is appended, so the picker cannot end up holding a name that is only
 * an extension.
 */
void force_extension(char *name, const char *ext)
{
    char *si;

    name[8] = 0;

    if (name[0] == 0)
        return;

    si = name;
    while (*si != 0 && *si != '.')
        si++;

    si[0] = '.';
    si[1] = 0;

    string_concat(name, ext);
}

/*
 * 0x135dc
 *
 * Hand the picker a name to start from: a straight copy into DGROUP 0x4e5a,
 * the one buffer the picker answers out of.
 *
 * **Nothing calls it**, by three searches rather than one: no near `call` to
 * it in this module, no far `call` anywhere in the image - `9a ec 55 ff 0d` -
 * and its far pointer `0dff:55ec` is not *stored* anywhere either, which is how
 * the region handlers are reached and would have been missed by the first two.
 * The same holds for `picker_name` beside it.
 *
 * The pointer search was run against `path_join` as a control: that one is
 * called, by a near `call`, and its pointer is likewise stored nowhere - so the
 * search distinguishes dispatch through a table from a direct call, rather than
 * answering "nowhere" to everything. They are the picker's public face, written and never
 * used, because `pick_file` fills 0x4e5a itself and copies the answer to
 * 0x52fe on the way out. Transcribed because they are there, and recorded as
 * dead because saying "unreached on the paths tried" would suggest a path
 * exists.
 */
void picker_set_name(const char *name)
{
    string_copy((char *)DG4E4E.name_buf, name);
}

/*
 * 0x135ef
 *
 * The picker's answer: **the buffer's address, or zero when it is empty.** A
 * caller would get a pointer it could hand straight to `load_animation`,
 * without having to know where the name lives.
 *
 * There is no caller. See `picker_set_name` above: neither is reachable from
 * anywhere in the image.
 */
char *picker_name(void)
{
    if (DG4E4E.name_buf[0] != 0)
        return (char *)DG4E4E.name_buf;

    return NULL;
}

/*
 * 0x13606
 *
 * **Get the listing a place to live, then fill it and draw it.**
 *
 * The block is allocated once and kept: the far pointer at DGROUP 0x5699 being
 * non-null is the whole test, and on the second and later openings everything
 * below is skipped. There is a second source before DOS is asked at all - the
 * pointer at 0x3576, some other part of the game's block, which is taken with a
 * flat capacity of 0x3e8 entries rather than a measured one.
 *
 * Otherwise it asks `dos_alloc_bytes` for **0xffffffff** bytes, which is the
 * "how much is there" question, and clamps the answer to 0x7530. So the picker
 * takes what is free up to 30000 bytes and no more - the listing is allowed to
 * grow into spare memory, but not to eat it.
 *
 * The entry size is 0x16, and that is where 0x5695 comes from: the block is
 * *two* arrays, `count` far pointers of four bytes each and then the records
 * themselves, so the second pointer is the first plus `4 * count`. Only the
 * offset is added - the segment is shared - which is what keeps a listing this
 * size inside one segment.
 */
void picker_begin(uint16_t arg1, uint16_t arg2, const char *pattern)
{
    uint32_t v;

    (void)arg1;
    (void)arg2;

    if (GAME_PICKER_TEXT.block == FAR_NULL_PTR) {
        if (DG3576.scratch != FAR_NULL_PTR) {
            GAME_PICKER_TEXT.entry_max = 0x3e8;
            GAME_PICKER_TEXT.block = DG3576.scratch;
        } else {
            v = dos_alloc_bytes(0xffffffffu, 0, 0).bytes;

            if ((int32_t)v > 0x7530)
                v = 0x7530;

            GAME_PICKER_TEXT.entry_max = (uint16_t)long_divide((int32_t)v, 0x16);

            GAME_PICKER_TEXT.block = (dos_alloc_bytes(v, 0, 0).ptr);
        }

        /* The table of pointers sits at the head of the block and the text
           after it, so the start is one pointer a line in, in the block's own
           segment. */
        GAME_PICKER_TEXT.text_start = (uint8_t far *)
            ((char far * far *)GAME_PICKER_TEXT.block + GAME_PICKER_TEXT.entry_max);
    }

    fill_file_listing(pattern);
    sort_file_listing();
    GAME_PICKER_TEXT.scroll = 0;
}

/*
 * 0x136c9
 *
 * **The picker's whole screen.** Everything the loop redraws piecemeal, laid
 * down once: the title bar, the four sunken wells - two for the buttons, two
 * for the scroll arrows - the heading, the buttons, and then the same four
 * routines the loop calls for its partial repaints.
 *
 * The heading and the left button say LOAD or SAVE according to 0x568f, and
 * they are drawn from **two separate branches** rather than one branch choosing
 * two strings. Both buttons are drawn unpressed here; `picker_draw_action` is
 * what draws them pressed, and it exists precisely because this routine cannot
 * be called for a button going down.
 *
 * The wells are placed round what goes in them, not derived from it: the button
 * well at (0x36, 0x129) is 0x40 by 0x20 for a button drawn at (0x40, 0x130).
 */
void picker_repaint(void)
{
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    draw_title_bar(0x30, 0x31, 0x110, 0x149, 1);

    draw_sunken_box(0x36, 0x129, 0x40, 0x20);
    draw_sunken_box(0xb6, 0x129, 0x50, 0x20);

    if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100) {
        draw_scroll_text(DG1BCC.load_machine, 0x50, 0x34, 0xa0);
        draw_button(DG1BCC.load, 0x40, 0x130, 0);
    } else {
        draw_scroll_text(DG1BCC.save_machine, 0x50, 0x34, 0xa0);
        draw_button(DG1BCC.save, 0x40, 0x130, 0);
    }

    draw_sunken_box(0xbc, 0x74, 0x20, 0x20);
    draw_sunken_box(0xbc, 0xe0, 0x20, 0x20);

    picker_draw_up();
    picker_draw_down();

    draw_button(DG1BCC.cancel, 0xc0, 0x130, 0);

    picker_draw_name();
    picker_draw_list();
    picker_draw_filename();

    present_back_page();
}

/*
 * 0x137e4
 *
 * **The list's up arrow**, redrawn. Which of the two pieces of art it uses is
 * read out of the mode word DGROUP 0x4e6b - 0x800 is "this arrow is held down"
 * - so the picker never has to tell it, the same way `picker_draw_action` reads
 * its own word back.
 *
 * The pair sits at +0x4a in the art set at DGROUP 0x52f4, and the pressed one
 * is the *second*, which is why the index is doubled before it is added.
 */
void picker_draw_up(void)
{
    int16_t pressed = (DG4E67.state == 0x800) ? 1 : 0;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[pressed + 0x25]),
                0xc4, 0x78, 0);
    restore_cursor_following();
}

/*
 * 0x1382a
 *
 * **The list's down arrow.** `picker_draw_up`'s twin, and the only differences
 * are the three numbers: the mode it answers to is 0x400, its art is at +0x4e,
 * and it sits 0x70 further down at y 0xe8.
 */
void picker_draw_down(void)
{
    int16_t pressed = (DG4E67.state == 0x400) ? 1 : 0;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[pressed + 0x27]),
                0xc4, 0xe8, 0);
    restore_cursor_following();
}

/*
 * 0x13870
 *
 * **The name field.** The buffer at DGROUP 0x53ab is copied into a local first,
 * and then the *pointer* is walked forward while the text is wider than 0xac
 * pixels - so a long name scrolls off the **left**, showing its end. That is
 * the right way round for typing: what you just typed stays in view.
 *
 * The caret is `*`, and it blinks by counting: 0x567e is bumped on every one of
 * these redraws and bit 3 decides whether the asterisk is appended, so it is on
 * for eight redraws and off for eight. It is appended *after* the width walk,
 * which means the caret can push the text past 0xac - the field is measured on
 * the name, not on the name plus caret.
 *
 * It only blinks when 0x4e6b says 0x4000, the field's own mode. Out of that
 * mode nothing is counted, so the caret is not merely hidden, it stops.
 */
void picker_draw_name(void)
{
    char buf[90];                  /* [bp-0x5a] */
    char *si  = buf;

    string_copy(buf, (const char *)GAME_DIRECTORIES.path_field);

    while ((int16_t)text_width_thunk(si) > 0xac)
        si++;

    if (DG4E67.state == 0x4000) {
        DG5677.caret_blink++;
        if ((DG5677.caret_blink & 8) != 0)
            string_concat(si, GAME_FILE_STRINGS.star_a);
    }

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    fill_panel_area(0x40, 0x56, 0xb8, 0x10, 0);

    VMDS.text_back = 0;
    VMDS.text_colour = 0x0f;

    cursor_redraw_off_thunk();
    draw_string(si, 0x44, 0x5a);
    restore_cursor_following();
}

/*
 * 0x13902
 *
 * **The "File Name:" field**, and `picker_draw_name`'s twin down to the shape
 * of the code: copy, walk the pointer forward while the text is too wide, blink
 * a caret by counting, fill, draw.
 *
 * Everything that differs is a number - a different buffer (0x4e5a against
 * 0x53ab), a narrower field (0x64 against 0xac), a different mode (0x1000), a
 * different counter (0x5680) - and a *different asterisk*: 0x2954, where the
 * other field uses 0x2952. The two one-character strings sit next to each other
 * in the image, unpooled, which is how you can tell these are two routines and
 * not one called twice.
 *
 * This one also draws its own label, because the label belongs to the field.
 */
void picker_draw_filename(void)
{
    char buf[16];                  /* [bp-0x10] */
    char *si  = buf;

    string_copy(buf, (const char *)DG4E4E.name_buf);

    while ((int16_t)text_width_thunk(si) > 0x64)
        si++;

    if (DG4E67.state == 0x1000) {
        DG5677.caret_blink_b++;
        if ((DG5677.caret_blink_b & 8) != 0)
            string_concat(si, GAME_FILE_STRINGS.star_b);
    }

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    draw_scroll_text(DG1BCC.file_name, 0x30, 0x10c, 0x54);
    fill_panel_area(0x90, 0x10c, 0x70, 0x10, 0);

    VMDS.text_back = 0;
    VMDS.text_colour = 0x0f;

    cursor_redraw_off_thunk();
    draw_string(si, 0x94, 0x110);
    restore_cursor_following();
}

/*
 * 0x139ac
 *
 * **Draw the listing.** Twelve rows of ten pixels in a 0x70 by 0x80 box at
 * (0x40, 0x78), text transparent and white, and the far pointers walked in
 * order - so what the sort did is what shows.
 *
 * **This is where the `:` record gets its words.** A row whose text begins with
 * a colon is drawn as *"&lt;PARENT DIR&gt;"* instead: the pointer is swapped for one
 * into DGROUP and the row draws normally. So the listing holds a one-byte
 * marker and the screen holds a phrase, and nothing in between has to know both.
 *
 * The scroll position at 0x5691 is **clamped on the way in, not on the way
 * out**: a listing of twelve or fewer starts at zero whatever 0x5691 says, and
 * one that has scrolled past the end is pulled back to `count - 12`. The
 * comparison is `>`, against the count rather than against `count - 12`, so a
 * position exactly at the count is *kept* and only one past it is caught.
 *
 * Two conditions end the loop, the count and the room left, and the room is
 * counted down in the same 0x0a steps the rows are drawn in.
 */
void picker_draw_list(void)
{
    int16_t  x = 0x40;                  /* [bp-0xa] */
    int16_t  y = 0x78;                  /* di */
    int16_t  w = 0x70;                  /* [bp-0xc] */
    int16_t  room = 0x80;               /* [bp-0xe] */
    /* The block is an array of far pointers, one per entry. */
    char far * far *p;                  /* [bp-4], [bp-2] */
    int16_t  top, i;

    fill_panel_area(x, y, w, room, 0);

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    VMDS.text_style = 1;                    /* transparent text */
    VMDS.text_colour = 0x0f;

    if (GAME_PICKER_TEXT.entry_count > 0x0c) {
        top = GAME_PICKER_TEXT.scroll;
        if (top > GAME_PICKER_TEXT.entry_count)
            top = (int16_t)(GAME_PICKER_TEXT.entry_count - 12);
    } else {
        top = 0;
    }

    p = (char far * far *)GAME_PICKER_TEXT.block;
    p += top;                           /* skip the rows scrolled past */

    i = 0;
    while (i < GAME_PICKER_TEXT.entry_count && room >= 0x0a) {
        const char far *t = *p;

        p++;
        if (*t == ':')
            t = DG1BCC.parent_dir;

        cursor_redraw_off_thunk();
        draw_string_body(t,
                         (int16_t)(x + 4), (int16_t)(y + 4));
        restore_cursor_following();

        y    += 0x0a;
        room -= 0x0a;
        i++;
    }
}

/*
 * 0x13a8a
 *
 * **Fill the listing.** Two pointers walk the block `picker_begin` set up: one
 * along the far-pointer array at its front, one along the text after it. Each
 * entry files a pointer and appends its text, and the array is terminated with
 * a null far pointer rather than a count - though a count is kept at 0x5693 as
 * well, because the fill also has to stop when the block is full.
 *
 * **The three record shapes are what `path_join` reads back.** A directory is
 * written `<NAME>`: `<`, then the name copied *including its NUL*, then the
 * byte before the write pointer - which is that NUL - overwritten with `>` and
 * a fresh NUL put down. That is where `path_join`'s off-by-one at both ends
 * comes from, and it is no longer a guess.
 *
 * A file is written as a fixed 8-character stem padded with spaces and then
 * everything from the `.` onwards, so the extensions line up in a column
 * without the drawing code measuring anything.
 *
 * And a lone `:` goes in first when DGROUP 0x53ae is not zero - the fourth byte
 * of the current directory, so "there is something past X:\\". It is the way
 * back up, and it is needed because `.` and `..` are both thrown away below.
 *
 * **The extension filter reads DGROUP when a name has no dot.** `strchr` for
 * `.` answers zero for a name like README, and the three comparisons that
 * follow are made through that zero, against DGROUP's own first bytes. Near
 * pointers make it harmless rather than fatal, and it is why a pattern of
 * `*.*` - whose second byte is `*` - is turned into *no filter at all* before
 * the loop starts, rather than into a filter that always matches.
 */
void fill_file_listing(const char *pattern)
{
    /* Two cursors into the one block: the array of far pointers at its
       front, and the text they point at, both stepped inside one segment. */
    char far * far *ptr;                /* [bp-4], [bp-2]: into the array */
    char far *txt;                      /* [bp-8], [bp-6]: into the text */
    const char *want_ext;                  /* [bp+6], rewritten in place */
    char *name;                      /* di */
    const char *name_ext;                  /* [bp-0xa]                        */
    int16_t  n;                         /* [bp-0xe]                        */
    uint16_t more;                      /* [bp-0xc]                        */

    GAME_PICKER_TEXT.entry_count = 0;
    dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);

    ptr = (char far * far *)GAME_PICKER_TEXT.block;
    txt = (char far *)GAME_PICKER_TEXT.text_start;

    want_ext = string_chr((char *)pattern, '.');
    if (want_ext != NULL && want_ext[1] == '*')
        want_ext = NULL;

    if (GAME_DIRECTORIES.path_field[3] != 0) {
        *ptr++ = txt;

        *txt++ = ':';
        *txt++ = 0;

        GAME_PICKER_TEXT.entry_count++;
    }

    more = dos_findfirst(GAME_FILE_STRINGS.star_dot_star_a, 0x10);

    while (more == 0 && ((uint16_t)GAME_PICKER_TEXT.entry_count) < ((uint16_t)GAME_PICKER_TEXT.entry_max)) {
        name     = dos_find_name();
        name_ext = string_chr(name, '.');

        if ((dos_find_attr() & 0x10) != 0) {
            if (string_compare(name, GAME_FILE_STRINGS.dot) != 0
                && string_compare(name,
                                  GAME_FILE_STRINGS.dot_dot_a) != 0) {
                *ptr++ = txt;
                GAME_PICKER_TEXT.entry_count++;

                *txt++ = '<';

                do {
                    *txt++ = *name;
                } while (*name++ != 0);

                txt[-1] = '>';
                *txt++  = 0;
            }
        } else if (want_ext == NULL
                   || (name_ext[1] == want_ext[1]
                       && name_ext[2] == want_ext[2]
                       && name_ext[3] == want_ext[3])) {
            *ptr++ = txt;
            GAME_PICKER_TEXT.entry_count++;

            n = 0;
            while (*name != 0 && *name != '.') {
                *txt++ = *name;
                name++;
                n++;
            }

            while (n < 8) {
                *txt++ = ' ';
                n++;
            }

            do {
                *txt++ = *name;
            } while (*name++ != 0);
        }

        more = dos_findnext(GAME_FILE_STRINGS.star_dot_star_b, 0x10);
    }

    *ptr = NULL;                        /* the list's terminator */
}

/*
 * 0x13c78
 *
 * **Sort the listing**, by exchanging the far pointers at the front of the
 * block and never the text they point at. A bubble sort: passes until one makes
 * no exchange.
 *
 * **Directories come first, and the test is the `<` the fill wrote.** There is
 * no type field to consult - the first byte of the record *is* the type - so
 * one against a directory sorts before, one after, and two of a kind fall
 * through to comparing the names. Which is why the four cases are written as
 * two nested tests rather than a comparison of two flags.
 *
 * The names are compared with `far_stricmp`, so `<Dos>` and `<DOS>` land where
 * a reader expects rather than where ASCII puts them.
 *
 * The `:` entry is **not sorted**: every pass starts one slot further in when
 * the first record begins with it, so the way back up stays at the top however
 * the rest moves.
 *
 * The walk ends on a null far pointer, and it checks *two* - the current slot
 * and the next - because a bubble pass compares a pair and there is no pair at
 * the last entry.
 */
void sort_file_listing(void)
{
    /* The block is an array of far pointers, one per entry. `p` walks it
       and `p[1]` is the next, which is what the original's `+ 4` is. */
    char far * far *p;                  /* [bp-4], [bp-2] */
    int16_t  swapped = 1;

    while (swapped) {
        swapped = 0;

        p = (char far * far *)GAME_PICKER_TEXT.block;

        /* Skip the ":" entry - the current directory - if it is first, so
           the sort below never moves it. */
        if (p[0] != NULL) {
            if (*p[0] == ':')
                p++;
        }

        while (p[0] != NULL && p[1] != NULL) {
            /* The pointers are swapped; the names are read through. */
            char far *a = p[0];
            char far *b = p[1];
            const char far *name_a = a;
            const char far *name_b = b;
            int16_t  swap = 0;

            /* "<PARENT DIR>" and the directories sort first. */
            if (*name_a != '<' && *name_b == '<')
                swap = 1;
            else if (*name_a == '<' && *name_b != '<')
                swap = 0;
            else if (far_stricmp(name_a, name_b) > 0)
                swap = 1;

            if (swap) {
                p[0] = b;
                p[1] = a;
                swapped = 1;
            }

            p++;
        }
    }
}

/*
 * 0x13d75
 *
 * **A listing record back into a plain name.** The record is far and the answer
 * is near - DGROUP 0x5682, one shared buffer - so the caller gets something it
 * can hand to `strcpy` without carrying a segment around.
 *
 * It strips exactly three things: `<`, `>` and spaces. That undoes both of the
 * shapes `fill_file_listing` writes, the angle brackets round a directory and the
 * padding that lines the extensions up, with one filter rather than two.
 *
 * A `:` record does not go through the loop at all; it answers the constant
 * `".."`, so the way back up leaves here as a path DOS understands rather than
 * as the marker the listing keeps it as.
 */
char *listing_to_name(const char far * entry)
{
    char *si;

    if (*entry == ':')
        return GAME_FILE_STRINGS.dot_dot_b;

    si = (char *)GAME_NAME_BUFFER.name;

    while (*entry != 0) {
        char c = *entry;

        if (c != '<' && c != '>' && c != ' ') {
            *si = c;
            si++;
        }
        entry++;
    }

    *si = 0;
    return (char *)GAME_NAME_BUFFER.name;
}

/*
 * 0x13dc7
 *
 * **Draw a string wrapped into a box**, centred both ways, with a shadow.
 *
 * `wrap_text_to_box` does the wrapping and leaves its results in DGROUP: a
 * list of line pointers from 0x56a6, how many at 0x56a4, and the block's
 * measured height and width at 0x56a0 and 0x56a2. This routine only places and
 * draws them.
 *
 * The centring uses the *measured* extents, not the box: `(w - 0x56a2 - 1) / 2`
 * and `(h - 0x56a0 - 1) / 2`, the minus one making an odd remainder fall left
 * and up rather than right and down. The clip box is then set to the box as
 * placed, so a line the wrapper could not fit is cut rather than drawn over
 * the panel.
 *
 * **A line's end is the next line's start, less one.** The table holds only
 * starts, so each line is bounded by looking ahead - and the trailing spaces
 * are walked back over before drawing, then a NUL is written *into the
 * caller's string* to terminate it and the displaced byte is put back
 * afterwards. The string is modified and restored, which is why this cannot be
 * handed a string in read-only memory.
 *
 * Each line is drawn twice, colour 0xf one pixel left and one down and then
 * colour 5 at the true place - the same shadow the parts bin's numbers use.
 *
 * The loop ends on a null pointer, on a line that starts with a NUL, or when
 * the count runs out, and the count is tested **before** it is decremented, so
 * a count of one draws one line.
 */
void draw_wrapped_text(char *str, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t line_height;
    uint16_t i;
    int16_t  left, top, left_at;

    VMDS.text_style = 1;                        /* transparent */
    line_height = font_line_height(0);

    wrap_text_to_box(str, w, h, line_height);

    left = (int16_t)(x + (w - GAME_PICKER_TEXT.text_width - 1) / 2);
    top  = (int16_t)(y + (h - GAME_PICKER_TEXT.text_height - 1) / 2 + 1);

    VMDS.clip_left   = left;
    VMDS.clip_right  = (int16_t)(left + w);
    VMDS.clip_top    = top;
    VMDS.clip_bottom = (int16_t)(top + h);

    i       = 0;
    left_at = GAME_PICKER_TEXT.line_count;

    while (GAME_TEXT_LINES.line_ptr[i] != 0 && *dg_near_ptr(GAME_TEXT_LINES.line_ptr[i]) != 0
           && left_at-- != 0) {
        char *start = (char *)dg_near_ptr(GAME_TEXT_LINES.line_ptr[i]);
        char *end   = (char *)dg_near_ptr(GAME_TEXT_LINES.line_ptr[i + 1]) - 1;
        char  saved;

        while (end > start && (uint8_t)*end <= ' ')
            end--;
        end++;

        saved = *end;
        *end = 0;

        cursor_redraw_off_thunk();

        VMDS.text_colour = 0x0f;
        draw_string(start, (int16_t)(left - 1), (int16_t)(top + 1));

        VMDS.text_colour = 5;
        draw_string(start, left, top);

        restore_cursor_following();

        *end = saved;
        i++;
        top = (int16_t)(top + line_height);
    }

    set_clip_full_screen();
}

/*
 * 0x13ed2
 *
 * **Break a string into lines that fit a box.** The line starts go into the
 * table from DGROUP 0x56a6, how many at 0x56a4, and the block's measured
 * height and width at 0x56a0 and 0x56a2 - which `draw_wrapped_text` then uses
 * to centre it.
 *
 * The height is capped at **seven lines** before anything else: `h` is reduced
 * to `7 * line_height` if it is larger, so a tall box does not make a tall
 * block. Seven is a constant in the code, not a table size.
 *
 * The measuring is by *word*, through `measure_word`, which answers the word's
 * width and its length. A word that does not fit starts a new line - and the
 * test is `width + word > box` **or** nothing has been placed on this line yet
 * and the block is not empty, so a single word wider than the box still gets a
 * line to itself rather than looping.
 *
 * A carriage return, 0x0d, forces a line break and the next line starts *after*
 * it. A space adds the width of a space - measured once at the top from a
 * two-byte string - and is otherwise skipped. Any other character at or below
 * a space ends the scan.
 *
 * The width recorded at 0x56a2 is the widest line, clamped to the box.
 *
 * **The last line is counted only if it has something on it**: after the loop,
 * a run width of zero with at least one line already recorded takes one back
 * off the count; otherwise the height gains one more line. Then the entry past
 * the last is set to the point the scan stopped at, which is what makes
 * `draw_wrapped_text`'s "end is the next start, less one" work for the final
 * line as well.
 */
void wrap_text_to_box(char *str, int16_t w, int16_t h, uint16_t line_height)
{
    char space[2];            /* [bp-0xc], a two-byte " " */
    int16_t o_len[3];   /* [bp-0xa] */
    int16_t o_wide[2];   /* [bp-4]   */
    char    *at     = str;
    int16_t  used   = 0;         /* height used so far */
    int16_t  run    = 0;         /* width on the current line */
    int16_t  space_w;
    int16_t  cap    = (int16_t)(line_height * 7);

    if (h > cap)
        h = cap;

    GAME_PICKER_TEXT.line_count = 0;
    GAME_PICKER_TEXT.text_height  = 0;
    GAME_PICKER_TEXT.text_width  = 0;

    if (*at != 0) {
        GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at);
        GAME_PICKER_TEXT.line_count++;
    }

    (*space)     = ' ';
    space[1] = 0;
    space_w = (int16_t)text_width_thunk(space);

    while (*at != 0 && (int16_t)(used + line_height) < h) {
        int16_t word_w, word_len;

        measure_word(at, (uint8_t *)o_wide,
                     (uint8_t *)o_len);
        word_w   = o_wide[0];
        word_len = o_len[0];

        if ((run != 0 || used == 0) && (int16_t)(run + word_w) >= w) {
            run  = 0;
            used = (int16_t)(used + line_height);
            GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at);
            GAME_PICKER_TEXT.line_count++;
            if ((int16_t)(used + line_height) >= h)
                break;
        }

        at += word_len;
        run = (int16_t)(run + word_w);
        if (run > GAME_PICKER_TEXT.text_width)
            GAME_PICKER_TEXT.text_width = run;
        if (GAME_PICKER_TEXT.text_width > w)
            GAME_PICKER_TEXT.text_width = w;

        while (*at != 0 && (uint8_t)*at <= ' '
               && (int16_t)(used + line_height) < h) {
            if (*at == 0x0d) {
                run  = 0;
                used = (int16_t)(used + line_height);
                GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at + 1);
                GAME_PICKER_TEXT.line_count++;
            } else if (*at == ' ') {
                run = (int16_t)(run + space_w);
            }
            at++;
        }
    }

    GAME_PICKER_TEXT.text_height = used;

    if (run == 0 && ((uint16_t)GAME_PICKER_TEXT.line_count) != 0)
        GAME_PICKER_TEXT.line_count--;
    else
        GAME_PICKER_TEXT.text_height = (int16_t)(GAME_PICKER_TEXT.text_height + line_height);

    GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at);
}

/*
 * 0x1401d
 *
 * Measure one word: how wide it is and how long, answered through the two near
 * pointers it is given.
 *
 * A word runs to the first character **at or below a space** - so a space, a
 * carriage return and a NUL all end it, and `wrap_text_to_box` then decides
 * which of those it was.
 *
 * The width comes from `text_width`, and to get it the routine writes a NUL
 * over the terminator, measures, and puts the displaced byte back - the same
 * trick `draw_wrapped_text` uses on the same string, for the same reason:
 * `text_width` stops at a NUL and there is nowhere else to put one.
 *
 * The length is counted separately as the walk goes rather than taken from the
 * pointer difference.
 */
void measure_word(char *str, uint8_t * out_width, uint8_t * out_length)
{
    char *at  = str;
    int16_t  len = 0;
    char     saved;

    while ((uint8_t)*at > ' ') {
        at++;
        len++;
    }

    saved   = *at;
    *at = 0;

    *(int16_t *)(out_width) = (int16_t)text_width(str);
    *(int16_t *)(out_length) = len;

    *at = saved;
}

/*
 * 0x1405b
 *
 * Build the list of parts a level may use, and reset the machine's state around
 * it: the list head at DGROUP 0x50d7, the two pairs at 0x5179 and 0x521b, the
 * play area at 0x50af..0x50b5, and the two at 0x4ead.
 *
 * Parts 0 to 0x32 are all included except in three cases. **0x14, 0x29 and 0x31
 * are never included**, and are excluded by falling into a branch that leaves
 * the flag clear rather than by being tested against a list. And **0x20, 0x21
 * and 0x22 are conditional**, each on its own word - 0x4e7d, 0x4e81 and 0x4e7b -
 * which is what makes three of the parts appear only when the game says so.
 *
 * The three conditionals are written as three independent `if`s inside the same
 * branch rather than as a switch, so a part number that is not one of the three
 * reaches the end of them with its flag still clear and is left out too - which
 * cannot happen, because only those three get in there.
 *
 * The play area is 0x43,0x110 to -8,-8 - the negative pair being the origin
 * rather than a size, which is worth saying because it reads like a mistake.
 */
void build_part_list(void)
{
    int16_t si;

    DG50D3.parts_bin.prev_ptr = 0;
    DG50D3.parts_bin.next_ptr = 0;
    DG5179.moving_parts.prev_ptr = 0;
    DG5179.moving_parts.next_ptr = 0;
    DG521B.placed_parts.prev_ptr = 0;
    DG521B.placed_parts.next_ptr = 0;

    for (si = 0; si < 0x33; si++) {
        int16_t wanted = 0;

        if (si == 0x20 || si == 0x21 || si == 0x22) {
            if (si == 0x20 && ((uint16_t)DG4E67.holiday_halloween) != 0)
                wanted = 1;
            if (si == 0x21 && ((uint16_t)DG4E67.holiday_valentine) != 0)
                wanted = 1;
            if (si == 0x22 && ((uint16_t)DG4E67.holiday_christmas) != 0)
                wanted = 1;
        } else if (si != 0x14 && si != 0x29 && si != 0x31) {
            wanted = 1;
        }

        if (wanted != 0) {
            struct part *rec = make_part((uint16_t)si);

            if (rec != PART_NONE)
                insert_sorted(rec, &DG50D3.parts_bin);
        }
    }

    DG50D3.bin_list_ptr = dg_near(dgroup, &DG50D3.parts_bin);
    DG50AF.bonus_2 = 0;
    DG50AF.bonus_1 = 0;
    DG50AF.gravity = 0x43;
    DG50AF.air = 0x110;
    DG50AF.extent_x = -8;
    DG50AF.extent_y = -8;
    DG50AF.tune = 0x3e9;
    DG4E67.counter = 0;

    recompute_kind_physics();
}

/*
 * 0x14133
 *
 * Make one part: a 0xa2-byte record off the near heap, filled from the
 * sixteen-byte-per-part table at DGROUP 0x2966 and the bitmap list
 * `load_part_bitmap` left at 0xeba.
 *
 * The fields that come across are the part's kind at +6, its size at +0xa and
 * +0x50/+0x52, its extent at +0x44/+0x46, its bitmaps at +0x80 and a word at
 * +0x94. The two at +0x8c and +0x8e start at -1 rather than 0, which is what
 * "no link" looks like everywhere else in this game.
 *
 * Each part may also have an **init function** in the table, at +12 of its
 * entry, and a part that answers 1 from it is refused - the record is freed and
 * the answer is `PART_NONE`, offset 0. The port dispatches that far pointer on
 * its value, as it does everywhere else it cannot call one.
 *
 * The heap is checked three times: before the allocation, after it, and at the
 * end.
 */
struct part *make_part(uint16_t kind)
{
    struct part *part = PART_NONE;
    int16_t failed = 0;

    heap_check_or_hang();

    /* A refusal is the offset 0 `or ax,ax` at 0x14159 tests, so `part` is
       no part on the `done` path below. */
    part = (struct part *)(void *)heap_calloc_far(1, sizeof(struct part));
    if (part == NULL) {
        part = PART_NONE;
        failed = 1;
        goto done;
    }

    heap_check_or_hang();

    part->kind = kind;
    part->flags_06 = PART_TEMPLATES[kind].flags_06;
    part->flags_0a = PART_TEMPLATES[kind].flags_0a;
    part->set_size.width = PART_TEMPLATES[kind].set_size.width;
    part->set_size.height = PART_TEMPLATES[kind].set_size.height;
    part->size[0].width = PART_TEMPLATES[kind].size.width;
    part->size[0].height = PART_TEMPLATES[kind].size.height;
    part->point_count =
        PART_KINDS[kind].point_count;
    part->start_x = 0xffff;
    part->start_y = 0xffff;
    if (PART_TEMPLATES[kind].init != NULL
        && PART_TEMPLATES[kind].init(part) == 1) {
        failed = 1;
        goto done;
    }

    part->start_flags = part->flags_08;

    set_object_extent(part);

    part->mirror_size.height = part->size[0].height;
    part->mirror_size.width = part->size[0].width;

    heap_check_or_hang();

done:
    if (failed != 0) {
        if (part != PART_NONE)
            free_part(part);
        return PART_NONE;
    }

    return part;
}

/*
 * 0x14236 .. 0x14d42 - the **part initialisers**, fifty-one routines.
 *
 * The table of part kinds at DGROUP 0x2966 carries one far pointer each, at
 * +0x0c, and `make_part` calls it through that. Fifty-eight kind
 * slots reach fifty-one distinct routines: five kinds have no initialiser at
 * all and three - 1, 46 and 48 - share 0x14267.
 *
 * Nearly all of them are the same four steps:
 *
 *   1. OR some bits into the part's flags at +6, +8 and +0x0a, if it has any;
 *   2. take four bytes per bitmap - `heap_calloc_far(count, 4)` - into +0x82;
 *   3. refuse, by answering 1, if that allocation failed;
 *   4. call the part's own setup in segment 0x172c, and answer 0.
 *
 * Three skip step 2 - 0x147a7, 0x148e0 and 0x148ff call their setup with no
 * allocation. Two more skip both: 0x14aa2 and 0x14c48 only set flags and
 * bytes. And three allocate something else instead - 0x143fb and 0x1449d a
 * 0x2c-byte belt at +0x66, 0x1443d a 0x38-byte rope at +0x54 - each writing
 * the part's own address into the new record as its back-pointer.
 *
 * **They were a table until 2026-09-11**, six columns standing in for the
 * bodies: three flag words, the setup, a list of stores and a flag for
 * whether it allocated. That is not what the binary holds. The constants live
 * as immediates inside fifty-one separate functions - searching the whole
 * image for any two of them adjacent as data finds nothing - and the form
 * cost three defects, every one recorded in a comment beside it. The worst is
 * the one it could not report: the table had **forty-eight** of the fifty-one,
 * and 0x14ca0, 0x14cd9 and 0x14d0a were missing outright.
 */

/* 0x14236 */
uint16_t part_init_bowling_ball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_0001(part);
    return 0;
}

/* 0x14267 */
uint16_t part_init_14267(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0040);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0180);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_48ab(part);
    return 0;
}

/* 0x142a1 */
uint16_t part_init_ramp(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0600);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0080);
    part->form = 0x0001;
    part->start_form = 0x0001;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_ramp(part);
    return 0;
}

/* 0x142e6 */
uint16_t part_init_seesaw(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x000c);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_seesaw(part);
    return 0;
}

/* 0x14320 */
uint16_t part_init_balloon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 16;
    part->attach[0].y = 47;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_balloon(part);
    return 0;
}

/* 0x14361 */
uint16_t part_init_conveyor(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0081);
    part->form = 0x001c;
    part->start_form = 0x001c;
    part->direction = 0x0000;
    part->start_direction = 0x0000;
    part->grab.x = 59;
    part->grab_size = 0x000e;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_conveyor(part);
    return 0;
}

/* 0x143b3 */
uint16_t part_init_mouse_cage(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0801);
    part->grab.x = 30;
    part->grab.y = 4;
    part->grab_size = 0x000c;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_mouse_cage(part);
    return 0;
}

/* 0x143fb */
uint16_t part_init_pulley(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 0;
    part->attach[0].y = 8;
    part->attach[1].x = 15;
    part->attach[1].y = 8;

    part->belt_ptr[0] = dg_near(dgroup, heap_calloc_far(1, 0x2c));
    if (part->belt_ptr[0] == 0)
        return 1;
    BELT_PTR(part->belt_ptr[0])->owner_ptr = dg_near(dgroup, part);
    return 0;
}

/* 0x1443d */
uint16_t part_init_belt(struct part *part)
{
    part->rope_ptr = dg_near(dgroup, heap_calloc_far(1, 0x38));
    if (part->rope_ptr == 0)
        return 1;
    ROPE_PTR(part->rope_ptr)->owner_ptr = dg_near(dgroup, part);
    return 0;
}

/* 0x1446c */
uint16_t part_init_basketball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_0001(part);
    return 0;
}

/* 0x1449d */
uint16_t part_init_rope(struct part *part)
{
    part->belt_ptr[0] = dg_near(dgroup, heap_calloc_far(1, 0x2c));
    if (part->belt_ptr[0] == 0)
        return 1;
    BELT_PTR(part->belt_ptr[0])->owner_ptr = dg_near(dgroup, part);
    return 0;
}

/* 0x144cb */
uint16_t part_init_bird_cage(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 21;
    part->attach[0].y = 2;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bird_cage(part);
    return 0;
}

/* 0x1450c */
uint16_t part_init_pokey(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x8000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_pokey(part);
    return 0;
}

/* 0x14547 */
uint16_t part_init_jack_in_the_box(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1001);
    part->grab.x = 8;
    part->grab.y = 9;
    part->grab_size = 0x000e;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_jack_in_the_box(part);
    return 0;
}

/* 0x1458f */
uint16_t part_init_gear(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0001);
    part->grab.y = 13;
    part->grab.x = 13;
    part->grab_size = 0x0008;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_0001(part);
    return 0;
}

/* 0x145d1 */
uint16_t part_init_bob_the_fish(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bob_the_fish(part);
    return 0;
}

/* 0x14607 */
uint16_t part_init_bellow(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bellow(part);
    return 0;
}

/* 0x1463d */
uint16_t part_init_bucket(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 18;
    part->attach[0].y = 0;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bucket(part);
    return 0;
}

/* 0x1467e */
uint16_t part_init_cannon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_cannon(part);
    return 0;
}

/* 0x146bd */
uint16_t part_init_dynamite(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0420);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_dynamite(part);
    return 0;
}

/* 0x146fc */
uint16_t part_init_146fc(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_08a1(part);
    return 0;
}

/* 0x1472d */
uint16_t part_init_electric_plug(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_electric_plug(part);
    return 0;
}

/* 0x1476c */
uint16_t part_init_dynamite_plunger(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_dynamite_plunger(part);
    return 0;
}

/* 0x147a7 */
uint16_t part_init_hook(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part_setup_hook(part);
    return 0;
}

/* 0x147c5 */
uint16_t part_init_fan(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0001);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_fan(part);
    return 0;
}

/* 0x14804 */
uint16_t part_init_flashlight(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_flashlight(part);
    return 0;
}

/* 0x1483a */
uint16_t part_init_generator(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1001);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_generator(part);
    return 0;
}

/* 0x14874 */
uint16_t part_init_gun(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_gun(part);
    return 0;
}

/* 0x148af */
uint16_t part_init_baseball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_00c9(part);
    return 0;
}

/* 0x148e0 */
uint16_t part_init_light(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part_setup_light(part);
    return 0;
}

/* 0x148ff */
uint16_t part_init_magnifying_glass(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part_setup_magnifying_glass(part);
    return 0;
}

/* 0x14919 */
uint16_t part_init_monkey(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1805);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_monkey(part);
    return 0;
}

/* 0x14954 */
uint16_t part_init_pumpkin(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_pumpkin(part);
    return 0;
}

/* 0x14985 */
uint16_t part_init_heart_balloon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 18;
    part->attach[0].y = 35;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_heart_balloon(part);
    return 0;
}

/* 0x149c6 */
uint16_t part_init_christmas_tree(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_christmas_tree(part);
    return 0;
}

/* 0x149f7 */
uint16_t part_init_boxing_glove(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_boxing_glove(part);
    return 0;
}

/* 0x14a2d */
uint16_t part_init_rocket(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_rocket(part);
    return 0;
}

/* 0x14a67 */
uint16_t part_init_scissors(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_scissors(part);
    return 0;
}

/* 0x14aa2 */
uint16_t part_init_solar_panel(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    return 0;
}

/* 0x14ab9 */
uint16_t part_init_trampoline(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_trampoline(part);
    return 0;
}

/* 0x14aef */
uint16_t part_init_windmill(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0801);
    part->grab.x = 15;
    part->grab.y = 15;
    part->grab_size = 0x0008;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_windmill(part);
    return 0;
}

/* 0x14b37 */
uint16_t part_init_mort_the_mouse(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x8000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_mort_the_mouse(part);
    return 0;
}

/* 0x14b72 */
uint16_t part_init_cannon_ball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_cannon_ball(part);
    return 0;
}

/* 0x14ba3 */
uint16_t part_init_tennis_ball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_00c9(part);
    return 0;
}

/* 0x14bd4 */
uint16_t part_init_candle(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_candle(part);
    return 0;
}

/* 0x14c12 */
uint16_t part_init_corner_pipe(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0600);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_corner_pipe(part);
    return 0;
}

/* 0x14c48 */
uint16_t part_init_14c48(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 0;
    part->attach[0].y = 0;

    return 0;
}

/* 0x14c62 */
uint16_t part_init_motor(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0001);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0001);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_motor(part);
    return 0;
}

/* 0x14ca0 */
uint16_t part_init_14ca0(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_1105(part);
    return 0;
}

/* 0x14cd9 */
uint16_t part_init_14cd9(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_10b6(part);
    return 0;
}

/* 0x14d0a */
uint16_t part_init_14d0a(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_1105(part);
    return 0;
}

/*
 * 0x14d43
 *
 * Throw the whole machine away: every part on the three lists at DGROUP
 * 0x50d7, 0x521b and 0x5179 is freed and the three heads cleared. The intro
 * calls it between one animation and the next, which is why the credits get a
 * clean machine rather than the title screen's leftovers.
 */
void free_all_lists(void)
{
    free_part_list(PART_PTR(DG50D3.parts_bin.next_ptr));
    free_part_list(PART_PTR(DG521B.placed_parts.next_ptr));
    free_part_list(PART_PTR(DG5179.moving_parts.next_ptr));

    DG50D3.parts_bin.next_ptr = 0;
    DG5179.moving_parts.next_ptr = 0;
    DG521B.placed_parts.next_ptr = 0;
}

/*
 * 0x14d71
 *
 * Free every part on one list. The next pointer is taken out of the record
 * *before* the record is freed, which is the only way to walk a list you are
 * destroying.
 */
void free_part_list(struct part *si)
{
    /* `or si,si` at 0x14d8c: the list ends on an offset of 0, which as a
       pointer is `PART_NONE` and never NULL. */
    while (si != PART_NONE) {
        struct part *next = PART_PTR(si->next_ptr);

        free_part(si);
        si = next;
    }
}

/*
 * 0x14d95
 *
 * Give a part back: its per-bitmap array, then two records it may or may not
 * own, then the part itself. Every free goes through the checked one, so a
 * corrupt heap stops here rather than later.
 *
 * The two conditions are the interesting part. The record at +0x54 is freed
 * only when bit 0 of the flags at +8 is **clear** - with it set the record
 * belongs to something else and freeing it would be a double free. And the
 * record at +0x66 is freed only for parts 7 and 0x0a, compared by number
 * rather than by a flag: two particular parts allocate it and the rest leave
 * the field as whatever it was.
 *
 * A null part is not an error; it returns.
 */
void free_part(struct part *part)
{
    if (part == PART_NONE)   /* the offset: `or si,si` at 0x14d9c */
        return;

    if (part->points_ptr != 0)
        checked_free(dg_near_ptr(part->points_ptr));

    if (part->rope_ptr != 0
        && (part->flags_08 & 1) == 0)
        checked_free(dg_near_ptr(part->rope_ptr));

    if (part->belt_ptr[0] != 0
        && (part->kind == KIND_PULLEY
            || part->kind == KIND_ROPE))
        checked_free(dg_near_ptr(part->belt_ptr[0]));

    checked_free((uint8_t *)part);
}
