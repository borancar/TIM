/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Levels and settings on disk**: reading and writing a level or a machine,
 * counting the levels, a puzzle's title, the passwords, and `tim.cfg`.
 *
 * The sixth module of the original's **code segment 0dff**, image
 * 0x11d00..0x12c26. Its data is its literal pool alone, DGROUP
 * 0x2870..0x28d2, and its uninitialised data is DGROUP 0x546c..0x547a.
 * Functions are in address order and each carries the image offset it was
 * read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -d
 * JUDGE: data 0x2870..0x28d2
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

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

/* The level reader's and writer's own words - see `struct level_io`. */
struct level_io LEVEL_IO DGROUP_WAS(0x546c);

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
    register struct part *si;
    register uint16_t n;

    if (part == PART_NONE)
        return 0xffff;

    n = 0;
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
    else
        return PART_TABLE->part_ptr[index];
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
void alloc_part_table(register int16_t n)
{
    register int16_t si;

    LEVEL_IO.table = DOS_ALLOC_PTR(DOS_ALLOC((uint16_t)(n * 4), 0));

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
    /* A reference the compiler counts and emits nothing for. Without a
       second one Borland's weighting leaves `file` on the stack; the image
       has it in DI, so the original's source named it twice. */
    (void)file;
    while (game_fread_byte(file, (uint8_t *)buf), *buf)
        buf++;
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

    if (LEVEL_IO.version >= 0x101)
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

    if (LEVEL_IO.version >= 0x101) {
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

    if (LEVEL_IO.version <= 0x101) {
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
void read_list(FILE *file, register struct part *head, int16_t n)
{
    struct part *list = head;           /* [bp-2] */
    struct part *rec;                   /* [bp-4] */
    register int16_t di;

    head->next_ptr = head->prev_ptr = 0;

    for (di = 0; di < n; di++) {
        rec = PART_PTR(part_by_index(LEVEL_IO.record_count));
        read_record_fields(file, rec);
        insert_sorted(rec, list);
        LEVEL_IO.record_count++;
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
void read_level(char *name)
{
    int16_t n_machine;                  /* [bp-2] */
    int16_t n_moving;                   /* [bp-4] */
    int16_t n_given;                    /* [bp-6] */
#ifdef __TURBOC__
    char buf[0x210];                    /* [bp-0x216] */
#else
    /*
     * **The buffer has to be the guest's.** `game_setbuf` files its address
     * into the file record's `read_ptr` at +0x0a - a guest word the C
     * library then steps as a cursor, compares against the record's own
     * buffer and frees as a heap handle - and the library is the machine's,
     * laid out as the guest's. So on the host it is carved from the guest's
     * stack, where a sixteen-bit address can reach it. Ours.
     */
    uint16_t at = dg_alloca(0x210);
    char *buf = (char *)dg_near_ptr(at);
#endif
    register FILE *file;

    if ((file = game_fopen(name, GAME_FILE_NAMES.rb_read_level)) != 0) {
        game_setbuf(file, (uint8_t *)buf);
        game_fread_far(file, (uint8_t *)&LEVEL_IO.version_out);

        if (LEVEL_IO.version_out == 0xaced) {
            game_fread_far(file, (uint8_t *)&LEVEL_IO.version);

            if (LEVEL_IO.is_level != 0) {
                game_fread_string(file, (char *)DG4E67.title);
                game_fread_string(file, (char *)DG4E67.hint);
                game_fread_far(file, (uint8_t *)&DG50AF.bonus_1);
                game_fread_far(file, (uint8_t *)&DG50AF.bonus_2);
            }

            game_fread_far(file, (uint8_t *)&DG50AF.gravity);
            game_fread_far(file, (uint8_t *)&DG50AF.air);
            recompute_kind_physics();

            if (LEVEL_IO.is_level != 0) {
                game_fread_far(file, (uint8_t *)&DG50AF.extent_y);
                game_fread_far(file, (uint8_t *)&DG50AF.extent_x);
            }

            game_fread_far(file, (uint8_t *)&DG50AF.tune);

            game_fread_far(file, (uint8_t *)&n_machine);
            game_fread_far(file, (uint8_t *)&n_moving);
            game_fread_far(file, (uint8_t *)&n_given);

            LEVEL_IO.record_count = 0;
            alloc_part_table(n_machine + n_moving + n_given);

            read_list(file, &DG521B.placed_parts, n_machine);
            read_list(file, &DG5179.moving_parts, n_moving);
            if (LEVEL_IO.is_level != 0)
                read_list(file, &DG50D3.parts_bin, n_given);

            dos_free_far(LEVEL_IO.table);
        }

        game_fclose(file);
    }

    DG50D3.bin_list_ptr = dg_near(dgroup, &DG50D3.parts_bin);
#ifndef __TURBOC__
    dg_free(0x210);
#endif
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
    if (LEVEL_IO.error == 0 && game_fwrite(addr, 1, 1, file) != 1)
        LEVEL_IO.error = 1;
}

/*
 * 0x123e4
 *
 * **Write one word.** The same routine as `write_byte` with a size of 2, and
 * the original writes it out twice rather than sharing one - so this does too.
 */
void write_word(FILE *file, const uint8_t * addr)
{
    if (LEVEL_IO.error == 0 && game_fwrite(addr, 2, 1, file) != 1)
        LEVEL_IO.error = 1;
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
    (void)file;                         /* as in game_fread_string */
    while (write_byte(file, (const uint8_t *)str), *str)
        str++;
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
        else if (LEVEL_IO.is_level != 0)
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

    LEVEL_IO.error = 0;
    LEVEL_IO.version_out = 0xaced;
    LEVEL_IO.version = 0x0102;
    DG4E67.file_op_active = 1;

    f = game_fopen(name, GAME_FILE_NAMES.wb_write_level);
    if (f == 0) {
        DG4E67.file_op_active = 0;
        return 1;
    }

    write_word(f, (const uint8_t *)&LEVEL_IO.version_out);
    write_word(f, (const uint8_t *)&LEVEL_IO.version);

    if (LEVEL_IO.is_level != 0) {
        write_string(f, (char *)DG4E67.title);
        write_string(f, (char *)DG4E67.hint);
        write_word(f, (const uint8_t *)&DG50AF.bonus_1);
        write_word(f, (const uint8_t *)&DG50AF.bonus_2);
    }

    write_word(f, (const uint8_t *)&DG50AF.gravity);
    write_word(f, (const uint8_t *)&DG50AF.air);

    if (LEVEL_IO.is_level != 0) {
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
        LEVEL_IO.error = 1;

    if (LEVEL_IO.error != 0)
        dos_unlink(name);

    DG4E67.file_op_active = 0;
    return LEVEL_IO.error;
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

    LEVEL_IO.is_level = 1;
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
void load_animation(char *name)
{
    build_part_list();
    LEVEL_IO.is_level = 0;
    read_level(name);
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
    LEVEL_IO.is_level = 0;

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
    register FILE *file;
    register uint16_t found;

    if ((file = game_fopen(GAME_FILE_NAMES.tim_cfg_read, GAME_FILE_NAMES.rb_tim_cfg)) != 0) {
        game_fread_far(file, (uint8_t *)&DG4E67.furthest_level);
        game_fread_far(file, (uint8_t *)&DG4E67.master_level);
        game_fclose(file);
        found = 1;
    } else {
        found = 0;
    }
    return found;
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
