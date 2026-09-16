/*
 * Borland's DOS file primitives, from the C runtime at the top of segment 0000.
 *
 * Same standing as `borland_heap.c`: **not the game**, not what the port is
 * reconstructing, and kept rather than deleted - these are the runtime's, so
 * having them transcribed and checked against a real binary is worth something
 * for any other Turbo C or Borland C++ DOS program.
 *
 * They are here because the resource loader cannot be verified without them.
 * Every route through it - even the one that reads from the packed archive -
 * ends in the runtime's stdio, and that in turn ends here.
 *
 * The DOS calls themselves go to `io_dos_read` and `io_dos_lseek`, which serve
 * the game's own files read-only. See io.c.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include <stdlib.h>

#include <string.h>

#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * The DGROUP records only this file reads or writes. Each is laid over the
 * DGROUP byte array at the address its macro names, like the shared ones in
 * dgroup.h; they are declared here because nothing else uses them.
 */

/*
 * DGROUP 0x0000..0x0074 - **the start of Borland's data segment**: four zero
 * bytes the exit code sums to report a null pointer assignment, the
 * copyright, and the three messages the runtime writes with a length rather
 * than a terminator.
 */
struct borland_data_start {
    uint8_t   null_check[4];      /* +0x00 */
    char      copyright[43];      /* +0x04  "Borland C++ - Copyright 1991 Borland Intl." */
    char      null_message[25] __attribute__((nonstring));   /* +0x2f  "Null pointer assignment\r\n" */
    char      divide_message[14] __attribute__((nonstring)); /* +0x48  "Divide error\r\n" */
    char      abort_message[30] __attribute__((nonstring));  /* +0x56  "Abnormal program termination\r\n" */
} __attribute__((packed));

struct borland_data_start BORLAND_DATA_START DGROUP_AT(0x0000) = {
    .copyright = "Borland C++ - Copyright 1991 Borland Intl.",
    .null_message = "Null pointer assignment\015\012",
    .divide_message = "Divide error\015\012",
    .abort_message = "Abnormal program termination\015\012",
};
_Static_assert(sizeof(struct borland_data_start) == 0x74, "DGROUP 0x0000..0x0074, 0x74 bytes");

/*
 * **The name the last `findfirst`/`findnext` answered**, at DGROUP 0x2d4a:
 * thirteen bytes `dos_find_to_dgroup` copies out of the DTA and
 * `dos_find_name` answers. The word before it and the 0x1f bytes after, up to
 * BORLAND_FIND_INFO, are not established.
 *
 * DGROUP 0x2d48..0x2d76, 0x2e bytes.
 */
struct borland_find_name {
    uint16_t  word_2d48;          /* +0x00 [2] */
    char      find_name[13];      /* +0x02 [0xd] */
    uint8_t   unread_2d57[0x1f];  /* +0x0f [0x1f] */
} __attribute__((packed));

struct borland_find_name BORLAND_FIND_NAME DGROUP_AT(0x2d48);
DG_ASSERT_AT(struct borland_find_name, find_name, 0x02);
_Static_assert(sizeof(struct borland_find_name) == 0x2e, "the find name's run ends at BORLAND_FIND_INFO");

/*
 * **Not established**, DGROUP 0x2d76..0x2d7d, 0x07 bytes.
 */
struct borland_find_info {
    uint8_t   word_2d76;          /* +0x00 [1] */
    uint32_t  size;               /* +0x01 [4]  the size of the entry just found, which
                                     dos_find_to_dgroup copies out of the DTA */
    int16_t   word_2d7b;          /* +0x05 [2] */
} __attribute__((packed));

struct borland_find_info BORLAND_FIND_INFO DGROUP_AT(0x2d76);
_Static_assert(sizeof(struct borland_find_info) == 0x07, "DGROUP 0x2d76..0x2d7d, 0x07 bytes");
DG_ASSERT_AT(struct borland_find_info, word_2d76, 0x00);
DG_ASSERT_AT(struct borland_find_info, size,      0x01);
DG_ASSERT_AT(struct borland_find_info, word_2d7b, 0x05);

/*
 * **The `atexit` count**, DGROUP 0x4ab4..0x4ab7, 0x03 bytes: how many far pointers the table
 * at 0x6438 holds, up to thirty-two. `borland_atexit` raises it and
 * `borland_exit_common` walks it back down. It is 0 in the image and nothing
 * in the game registers a handler, so it stays 0. The byte after it is
 * unclaimed and the `_ctype` table follows.
 */
struct borland_atexit_count {
    uint16_t  atexit_count;       /* +0x00 [2] */
    uint8_t   byte_4ab6;          /* +0x02 [1] */
} __attribute__((packed));

struct borland_atexit_count BORLAND_ATEXIT_COUNT DGROUP_AT(0x4ab4);
DG_ASSERT_AT(struct borland_atexit_count, atexit_count, 0x00);
_Static_assert(sizeof(struct borland_atexit_count) == 3, "the atexit count ends at the ctype table");

/*
 * **Borland's `_ctype` table**, DGROUP 0x4ab7..0x4bb8, 0x101 bytes: a class byte per character,
 * 0x101 of them, up to BORLAND_EXIT_VECTORS. `to_lower` tests bit 2, upper case, and is the
 * one reader in the port.
 */
struct borland_ctype {
    uint8_t   ctype[0x101];       /* +0x00 [0x101] */
} __attribute__((packed));

struct borland_ctype BORLAND_CTYPE DGROUP_AT(0x4ab7) = {
    .ctype = {
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x21, 0x21,
        0x21, 0x21, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01,
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
        0x40, 0x40, 0x40, 0x40, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x14,
        0x14, 0x14, 0x14, 0x14, 0x14, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x40, 0x40, 0x40, 0x40, 0x20,
    },
};
_Static_assert(sizeof(struct borland_ctype) == 0x101, "the ctype table ends at BORLAND_EXIT_VECTORS");

/*
 * **The three exit vectors**, DGROUP 0x4bb8..0x4bc4, 0x0c bytes: `_exitbuf`, `_exitfopen` and
 * `_exitopen`, each a far pointer that `borland_exit_common` calls through.
 * All three point at the one `retf` at 0x0bc63 in the image; `borland_setvbuf`
 * plants `exit_flush_streams` in the first and `borland_fopen` plants
 * `exit_close_streams` in the second, and the third is never replaced. The
 * stream table follows at 0x4bc4.
 */
struct borland_exit_vectors {
    struct far_ptr exit_buf;      /* +0x00 [4] */
    struct far_ptr exit_fopen;    /* +0x04 [4] */
    struct far_ptr exit_open;     /* +0x08 [4] */
} __attribute__((packed));

struct borland_exit_vectors BORLAND_EXIT_VECTORS DGROUP_AT(0x4bb8) = {
    .exit_buf = { .off = 0xbc63, .seg = LOAD_SEG + 0x0000 },
    .exit_fopen = { .off = 0xbc63, .seg = LOAD_SEG + 0x0000 },
    .exit_open = { .off = 0xbc63, .seg = LOAD_SEG + 0x0000 },
};
DG_ASSERT_AT(struct borland_exit_vectors, exit_buf,   0x00);
DG_ASSERT_AT(struct borland_exit_vectors, exit_fopen, 0x04);
DG_ASSERT_AT(struct borland_exit_vectors, exit_open,  0x08);
_Static_assert(sizeof(struct borland_exit_vectors) == 0x0c, "the exit vectors end at the stream table");

/*
 * **Borland's streams**, DGROUP 0x4bc4..0x4d04, 0x140 bytes.
 *
 * Twenty `struct file_rec`, which is what the two routines that walk the table
 * say: `flush_all_streams` counts 0x14 of them at a stride of 0x10, and
 * `find_free_stream` bounds itself with `BORLAND_NFILE.word_4d04 << 4` - the count
 * times the stride. The fields they read are already named on that struct -
 * `+2` is `flags` and `+4` is `handle`, which is tested signed because -1
 * means no handle.
 */
struct borland_streams {
    struct file_rec streams[0x14];   /* +0x00 [0x140] */
} __attribute__((packed));

struct borland_streams BORLAND_STREAMS DGROUP_AT(0x4bc4) = {
    .streams = {
        { .flags = 0x0209, .token = 0x4bc4 },
        { .flags = 0x020a, .fd = 0x01, .token = 0x4bd4 },
        { .flags = 0x0202, .fd = 0x02, .token = 0x4be4 },
        { .flags = 0x0243, .fd = 0x03, .token = 0x4bf4 },
        { .flags = 0x0242, .fd = 0x04, .token = 0x4c04 },
    },
};
_Static_assert(sizeof(struct borland_streams) == 0x140, "DGROUP 0x4bc4..0x4d04, 0x140 bytes");
DG_ASSERT_AT(struct borland_streams, streams, 0x00);

/*
 * **Not established**, DGROUP 0x4d04..0x4d06, 0x02 bytes.
 */
struct borland_nfile {
    uint16_t  word_4d04;          /* +0x00 [2] */
} __attribute__((packed));

struct borland_nfile BORLAND_NFILE DGROUP_AT(0x4d04) = { .word_4d04 = 0x0014 };
_Static_assert(sizeof(struct borland_nfile) == 0x02, "DGROUP 0x4d04..0x4d06, 0x02 bytes");
DG_ASSERT_AT(struct borland_nfile, word_4d04, 0x00);

/*
 * **Borland's handle flags**, one word per DOS handle, DGROUP 0x4d06..0x4d2e,
 * 0x28 bytes. Twenty, which is `_nfile` - `BORLAND_NFILE`, 20 in the image - and
 * exactly the run to the open-mode word at 0x4d2e.
 */
struct borland_handle_flags {
    uint16_t  flags[0x14];        /* +0x00 [0x28] */
} __attribute__((packed));

struct borland_handle_flags BORLAND_HANDLE_FLAGS DGROUP_AT(0x4d06) = { .flags = { 0x6001, 0x6002, 0x6002, 0xa004, 0xa002 } };
_Static_assert(sizeof(struct borland_handle_flags) == 0x28, "twenty handles end at BORLAND_IO_MODES");

/*
 * **Not established**, DGROUP 0x4d2e..0x4d8f, 0x61 bytes.
 */
struct borland_io_modes {
    uint16_t  word_4d2e;          /* +0x00 [2] */
    uint16_t  word_4d30;          /* +0x02 [2] */
    uint8_t   pad_4d32[2];        /* +0x04 [2] */
    int16_t   word_4d34;          /* +0x06 [2] */
    /* Borland's `_dosErrorToSV`: the errno for each DOS error code, 0x59
       entries, -1 where there is none. `io_error` clamps a code to 0x58 and
       reads through here. The string "TMP" follows at 0x4d90. */
    int8_t    errno_map[0x59];    /* +0x08 [0x59] */
} __attribute__((packed));

struct borland_io_modes BORLAND_IO_MODES DGROUP_AT(0x4d2e) = {
    .word_4d2e = 0x4000,
    .word_4d30 = 0xffff,
    .errno_map = {
        0x00, 0x13, 0x02, 0x02, 0x04, 0x05, 0x06, 0x08, 0x08, 0x08, 0x14,
        0x15, 0x05, 0x13, -0x01, 0x16, 0x05, 0x11, 0x02, -0x01, -0x01, -0x01,
        -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01,
        0x05, 0x05, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01,
        -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, 0x0f, -0x01,
        0x23, 0x02, -0x01, 0x0f, -0x01, -0x01, -0x01, -0x01, 0x13, -0x01,
        -0x01, 0x02, 0x02, 0x05, 0x0f, 0x02, -0x01, -0x01, -0x01, 0x13, -0x01,
        -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, -0x01, 0x23, -0x01, -0x01,
        -0x01, -0x01, 0x23, -0x01, 0x13, -0x01,
    },
};
DG_ASSERT_AT(struct borland_io_modes, errno_map, 0x08);
_Static_assert(sizeof(struct borland_io_modes) == 0x61, "the errno map ends before the TMP string at 0x4d90");
DG_ASSERT_AT(struct borland_io_modes, word_4d2e, 0x00);
DG_ASSERT_AT(struct borland_io_modes, word_4d30, 0x02);
DG_ASSERT_AT(struct borland_io_modes, word_4d34, 0x06);

/*
 * **The runtime's strings and the printf class table**, DGROUP 0x4d90..0x4e34, 0xa4 bytes:
 * "TMP" and ".$$$" for a temporary name, "(null)" for a null `%s`, then one
 * class byte per character from ' ' to DEL - 0x14 for "not part of a
 * conversion" - which `vprinter` indexes with the character less 0x20, and
 * then the two words and the message the float-format stub writes to stderr.
 * The heap's first-block pointer follows at 0x4e34.
 */
struct borland_runtime_strings {
    char      tmp_prefix[4];      /* +0x00 [4]  "TMP" */
    char      tmp_suffix[5];      /* +0x04 [5]  ".$$$" */
    uint8_t   pad_4d99;           /* +0x09 [1] */
    char      null_str[7];        /* +0x0a [7]  "(null)" */
    uint8_t   fmt_class[0x60];    /* +0x11 [0x60] */
    uint8_t   pad_4e01;           /* +0x71 [1] */
    char      s_print[5] __attribute__((nonstring));  /* +0x72 [5]  "print", no terminator */
    char      s_scanf[5] __attribute__((nonstring));  /* +0x77 [5]  " scan", no terminator */
    char      s_no_floats[0x28];  /* +0x7c [0x28]  " : floating point formats not linked\r\n" */
} __attribute__((packed));

struct borland_runtime_strings BORLAND_RUNTIME_STRINGS DGROUP_AT(0x4d90) = {
    .tmp_prefix = "TMP",
    .tmp_suffix = ".$$$",
    .null_str = "(null)",
    .fmt_class = {
        0x00, 0x14, 0x14, 0x01, 0x14, 0x15, 0x14, 0x14, 0x14, 0x14, 0x02,
        0x00, 0x14, 0x03, 0x04, 0x14, 0x09, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
        0x14, 0x14, 0x14, 0x14, 0x0f, 0x17, 0x0f, 0x08, 0x14, 0x14, 0x14,
        0x07, 0x14, 0x16, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
        0x14, 0x0d, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
        0x14, 0x10, 0x0a, 0x0f, 0x0f, 0x0f, 0x08, 0x0a, 0x14, 0x14, 0x06,
        0x14, 0x12, 0x0b, 0x0e, 0x14, 0x14, 0x11, 0x14, 0x0c, 0x14, 0x14,
        0x0d, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
    },
    .s_print = "print",
    .s_scanf = " scan",
    .s_no_floats = "f : floating point formats not linked\015\012",
};
DG_ASSERT_AT(struct borland_runtime_strings, tmp_prefix,  0x00);
DG_ASSERT_AT(struct borland_runtime_strings, tmp_suffix,  0x04);
DG_ASSERT_AT(struct borland_runtime_strings, null_str,    0x0a);
DG_ASSERT_AT(struct borland_runtime_strings, fmt_class,   0x11);
DG_ASSERT_AT(struct borland_runtime_strings, s_print,     0x72);
DG_ASSERT_AT(struct borland_runtime_strings, s_scanf,     0x77);
DG_ASSERT_AT(struct borland_runtime_strings, s_no_floats, 0x7c);
_Static_assert(sizeof(struct borland_runtime_strings) == 0xa4, "the runtime's strings end at the heap's first-block pointer");
/*
 * DGROUP 0x4e42..0x4e4e - **two more near vectors and the init table.** The
 * two words follow `DG4E34.realcvt_ptr` and hold the same kind of value.
 * Then the one `_INIT_` record: call type 0, priority 2, address 0000:c1d6,
 * which is `setup_streams` - the entry `main.c` runs before `game_main`.
 */
struct borland_init_table {
    uint16_t  vector_4e42;        /* +0x00 */
    uint16_t  vector_4e44;        /* +0x02 */
    uint8_t   calltype;           /* +0x04  0x4e46 */
    uint8_t   priority;           /* +0x05 */
    struct far_ptr init;          /* +0x06  0x4e48  setup_streams */
    uint8_t   pad_4e4c[2];        /* +0x0a */
} __attribute__((packed));

struct borland_init_table BORLAND_INIT_TABLE DGROUP_AT(0x4e42) = {
    .vector_4e42 = 0xc889,
    .vector_4e44 = 0xc889,
    .calltype = 0x89,
    .priority = 0xc8,
    .init = { .off = 0x0200, .seg = 0xc1d6 },
};
_Static_assert(sizeof(struct borland_init_table) == 0x0c, "DGROUP 0x4e42..0x4e4e, 0x0c bytes");


/*
 * **The `atexit` table and the temporary name**, DGROUP 0x6438..0x64c8, 0x90 bytes: thirty-two
 * far pointers, counted at 0x4ab4, then the fourteen bytes `tmp_name_build`
 * writes into when given no buffer, then the one byte `borland_fgetc`'s
 * unbuffered read lands in. All zero in the image. The character being drawn
 * follows at 0x64c8.
 */
struct borland_atexit_table {
    struct far_ptr atexit[0x20];  /* +0x00 [0x80] */
    char      tmp_name[0x0e];     /* +0x80 [0xe] */
    uint8_t   getc_byte;          /* +0x8e [1] */
    uint8_t   pad_64c7;           /* +0x8f [1] */
} __attribute__((packed));

struct borland_atexit_table BORLAND_ATEXIT_TABLE DGROUP_BSS(0x6438);
DG_ASSERT_AT(struct borland_atexit_table, atexit,    0x00);
DG_ASSERT_AT(struct borland_atexit_table, tmp_name,  0x80);
DG_ASSERT_AT(struct borland_atexit_table, getc_byte, 0x8e);
_Static_assert(sizeof(struct borland_atexit_table) == 0x90, "the atexit table and the temp name end at BORLAND_FPUTC_CHAR");

/*
 * **The character being drawn**, DGROUP 0x64c8..0x64c9, 0x01 bytes.
 */
struct borland_fputc_char {
    uint8_t   character;          /* +0x00 [1]  filed here before anything else, and it stays */
} __attribute__((packed));

struct borland_fputc_char BORLAND_FPUTC_CHAR DGROUP_BSS(0x64c8);
_Static_assert(sizeof(struct borland_fputc_char) == 0x01, "DGROUP 0x64c8..0x64c9, 0x01 bytes");
DG_ASSERT_AT(struct borland_fputc_char, character, 0x00);


/*
 * 0x0bcbb
 *
 * Borland's `exit`: the common teardown at 0x0bc64 with (status, 0, 0) - the
 * `atexit` chain, the stream flush, the stream close, and INT 21h AH=4Ch.
 * The startup reaches it when `game_main` returns and when it gives up.
 */
void borland_exit(int16_t status)
{
    borland_exit_common(status, 0, 0);
}

/*
 * 0x0bcca
 *
 * `_exit`: the teardown with `quick` set - no `atexit` chain, no stream
 * flush, no stream close - and INT 21h AH=4Ch. The startup's abort at 0x027c
 * calls it with 3 after "Abnormal program termination"; nothing else does.
 */
void borland_exit_quick(int16_t status)
{
    borland_exit_common(status, 0, 1);
}

/*
 * 0x0bcdc
 *
 * `_cexit`: everything `exit` does except leave - the chain, the flush and
 * the close, and then a return to the caller. Nothing in the image calls it.
 */
void borland_cexit(void)
{
    borland_exit_common(0, 1, 0);
}

/*
 * 0x0bcea
 *
 * `_c_exit`: the two checks and the vectors put back, and nothing else, then
 * a return. Nothing in the image calls it.
 */
void borland_c_exit(void)
{
    borland_exit_common(0, 1, 1);
}

/*
 * 0x0c185
 *
 * `read`. INT 21h AH=3Fh, into a **** buffer - the count and the pointer
 * are both single words, so a read cannot cross a segment.
 *
 * Before it asks DOS it checks the handle's entry in the flag table at DGROUP
 * 0x4d06, two bytes per handle, and refuses with errno 5 if bit 1 is set. That
 * is how a handle opened write-only is stopped without DOS being troubled.
 *
 * The failure path hands the DOS error code to `__IOerror`, which is not
 * transcribed - a read that fails is not something these screens do.
 */
int16_t dos_read(int16_t handle, uint8_t * buf, uint16_t count)
{
    int16_t got;

    if ((BORLAND_HANDLE_FLAGS.flags[handle] & 2) != 0) {
        not_transcribed("__IOerror after a read refused by the handle flags");
        return -1;
    }

    got = io_dos_read(handle, buf, count);
    if (got < 0) {
        not_transcribed("__IOerror after a failed DOS read");
        return -1;
    }
    return got;
}

/*
 * 0x0c0c3
 *
 * `lseek`. INT 21h AH=42h with the direction in AL, answering the new position
 * in DX:AX.
 *
 * It clears **bit 9** of the handle's flag word first. That bit says the
 * buffered layer above has something pushed back; seeking throws that away, and
 * clearing it here rather than in the caller is what stops a pushed-back byte
 * surviving a seek and being read at the wrong offset.
 */
int32_t dos_lseek(int16_t handle, uint16_t lo, uint16_t hi, int16_t whence)
{
    int32_t pos;

    BORLAND_HANDLE_FLAGS.flags[handle] = (int16_t)(BORLAND_HANDLE_FLAGS.flags[handle] & 0xfdff);

    pos = io_dos_lseek(handle, (int32_t)(((uint32_t)hi << 16) | lo), whence);
    if (pos < 0) {
        not_transcribed("__IOerror after a failed DOS seek");
        return -1;
    }
    return pos;
}

/*
 * 0x0c1d6
 *
 * Borland's `_setupio`: make the stream table usable before `main` runs.
 *
 * The startup does not call this directly. It walks the init table between
 * DGROUP 0x4e48 and 0x4e4e - six bytes, so one entry - and calls what it finds
 * there, which is this. The port's own start-up calls it by name instead of
 * dispatching through a guest far pointer, because a table of one is not a
 * table.
 *
 * The stream table is at DGROUP 0x4bc4, sixteen bytes an entry, and `_nfile` at
 * 0x4d04 says how many there are - twenty here. Entries 0 to 4 are the standard
 * streams and are already set up; from 5 up this marks each **free** by putting
 * 0xff in the handle byte at +4 and pointing the stream's buffer field at the
 * stream itself.
 *
 * That 0xff is the whole point, and the port went without it for a while: the
 * open path tests `if ((int8_t)handle < 0)` before it opens anything, so a
 * table left as BSS looks like twenty streams that are already open on handle
 * 0, every `fopen` quietly does nothing, and the game gets as far as printing
 * "Unable to initialize vm." because it could not read its own video driver.
 *
 * The two tails give stdin and stdout a buffer, and drop the 0x200 bit from
 * either when it is not a terminal. `borland_setvbuf`'s mode is 1 for the first
 * and 2 for the second - line buffered and unbuffered - and only when the bit
 * is still set.
 */
void setup_streams(void)
{
    uint16_t dx;

    for (dx = 5; dx < BORLAND_NFILE.word_4d04; dx++) {
        BORLAND_HANDLE_FLAGS.flags[dx] = 0;
        BORLAND_STREAMS.streams[dx].fd = 0xff;
        BORLAND_STREAMS.streams[dx].token = dg_off(dgroup, &BORLAND_STREAMS.streams[dx]);
    }

    if (dos_isatty((int16_t)(int8_t)BORLAND_STREAMS.streams[0].fd) == 0)
        BORLAND_STREAMS.streams[0].flags = (uint16_t)(BORLAND_STREAMS.streams[0].flags & 0xfdff);

    borland_setvbuf(&BORLAND_STREAMS.streams[0], 0, (int16_t)((BORLAND_STREAMS.streams[0].flags & 0x200) ? 1 : 0), 0x200);

    if (dos_isatty((int16_t)(int8_t)BORLAND_STREAMS.streams[1].fd) == 0)
        BORLAND_STREAMS.streams[1].flags = (uint16_t)(BORLAND_STREAMS.streams[1].flags & 0xfdff);

    borland_setvbuf(&BORLAND_STREAMS.streams[1], 0, (int16_t)((BORLAND_STREAMS.streams[1].flags & 0x200) ? 2 : 0), 0x200);
}

/*
 * 0x0d0ed
 *
 * The buffered read under `fread`: fill `count` bytes from a `FILE` and answer
 * how many were **not** filled. A near routine with a callee-cleaned frame -
 * `ret 6` - so its three arguments are the `FILE`, the count and the buffer.
 *
 * The `FILE` fields it uses are +0 the bytes left in the buffer, +2 the flags,
 * +4 the DOS handle, +6 the buffer size and +0xa the read pointer.
 *
 * Three ways a byte arrives, and which one runs is worth knowing because they
 * differ enormously in cost. Measured over a run: 16,468 calls, of which 44
 * reach DOS and 395 reach `getc` - **97% are served entirely from the buffer
 * already in hand**.
 *
 * The DOS path is taken only when the request is at least a whole buffer and
 * the buffer is empty, and it then reads as many whole buffers as fit, straight
 * into the caller's memory without going through the buffer at all. A short
 * read sets the error flag 0x20 and gives up.
 *
 * Otherwise bytes come one at a time: from the buffer while +0 lasts, and
 * through `getc` when it runs out - which is where the refill happens, so this
 * loop never has to know a buffer exists.
 *
 * The count is incremented at the head of the loop and decremented in the body,
 * which nets to nothing on the way in and is what lets the same decrement serve
 * both the first pass and every byte after it. Written out as the original has
 * it rather than tidied, because the balance is easy to break.
 */
uint16_t buffered_read(struct file_rec *file, uint16_t count, uint8_t * buf)
{
    uint16_t di;
    /*
     * The original does not initialise DX. On the first pass the test at the
     * end of the loop reads whatever the caller left in it - and every caller
     * is `fread`, which leaves the high word of its own size-times-count, so it
     * is zero. Written as zero here rather than left to chance.
     */
    uint16_t dx = 0;

    goto test;

loop:
    count++;

    di = file->bsize;
    if (di > count)
        di = count;

    if ((file->flags & 0x40) != 0 && file->bsize != 0
        && file->bsize < count && ((uint16_t)file->level) == 0) {
        count--;
        di = 0;
        while (file->bsize <= count) {
            di = (uint16_t)(di + file->bsize);
            count = (uint16_t)(count - file->bsize);
        }

        dx = (uint16_t)dos_read((int16_t)file->fd, buf, di);
        buf += dx;
        if (dx == di)
            goto test;

        count = (uint16_t)(count + (di - dx));
        goto set_error;
    }

next_byte:
    count--;
    if (count == 0)
        goto check_eof;
    di--;
    if (di == 0)
        goto check_eof;

    file->level--;
    if (file->level < 0) {
        dx = (uint16_t)borland_getc(file);
    } else {
        uint16_t p = file->curp;

        file->curp = (int16_t)(p + 1);
        dx = *dg_ptr(dgroup, p);
    }

    if (dx != 0xffff) {
        *buf = (uint8_t)dx;
        buf++;
        goto next_byte;
    }

check_eof:
    if (dx == 0xffff)
        goto set_error;

test:
    if (count != 0)
        goto loop;
    return count;

set_error:
    file->flags = (int16_t)(file->flags | 0x20);
    return count;
}

/*
 * 0x0d1c4
 *
 * `fread`. Answers how many whole items were read, not how many bytes.
 *
 * A size of zero answers zero without touching the file. The product of size
 * and count is worked out in 32 bits and a request of more than 0xffff bytes is
 * refused outright rather than truncated - so a single `fread` can never ask
 * for more than a segment.
 *
 * The division at the end is what turns bytes into items, and it means a
 * partial item at the end of a file is **not** reported: reading three and a
 * half records answers three.
 */
uint16_t borland_fread(uint8_t * buf, uint16_t size, uint16_t count,
                     struct file_rec *file)
{
    uint32_t total;
    uint16_t left;

    if (size == 0)
        return 0;

    total = (uint32_t)size * count;
    if (total > 0xffff)
        return 0;

    left = buffered_read(file, (uint16_t)total, buf);
    return (uint16_t)(((uint16_t)total - left) / size);
}

/* The file putter, `vprinter`'s type for `stream_put_run` - defined with the engine. */
static uint16_t file_putn(void *sink, uint16_t n, const uint8_t *buf);

/*
 * 0x0d754
 *
 * Borland's `printf`: the engine at 0x0c2ed with the file putter at 0x0d8ca,
 * `stdout` - the second stream, DGROUP 0x4bd4 - and `lea ax,[bp+8]`, the
 * caller's stack past the format, as the arguments. `vprinter` pops its own
 * four words, which is why nothing follows the call but `pop bp` and `retf`.
 *
 * The port takes the arguments' address as a parameter, as `borland_sprintf`
 * does, because it has no such stack. The game's four calls - the message
 * `game_teardown` leaves with, and three fatal start-up messages - pass a
 * finished string and null for the arguments; a `%` in one would ask the
 * engine for an argument it does not have, and it says so.
 *
 * It used to abort, and that was wrong: `game_teardown` reaches it on the
 * ordinary way out, so an abort turned quitting into a crash.
 */
int16_t borland_printf(const char *fmt, const uint8_t *args)
{
    return vprinter(file_putn, &BORLAND_STREAMS.streams[1], fmt, args);
}

/*
 * 0x0da6d
 *
 * The layer between `read` and DOS: validate the handle, read, and translate
 * line endings if the handle is in text mode.
 *
 * The handle is checked against `_nfile` at DGROUP 0x4d04 and refused with
 * errno 6 above it. A count of 0 or 0xffff answers zero without reading, and so
 * does bit 9 of the handle's flags - the end-of-file mark this routine sets
 * itself when it meets a 0x1a.
 *
 * Text mode is bit 0x4000 of the flags. In it, carriage returns are dropped and
 * a 0x1a ends the file: the routine **seeks back** so the next read starts just
 * after it, and sets bit 9 so nothing reads past. It also refills when
 * translation leaves nothing, which is why a text-mode read of a file of bare
 * carriage returns loops rather than returning zero.
 *
 * **None of that runs here.** The game opens with "rb", so the flag is clear
 * and the translation is dead - transcribed as the refusal it is rather than
 * written on faith, since nothing on these screens can check it.
 */
int16_t read_translated(int16_t handle, uint16_t buf, uint16_t count)
{
    int16_t got;

    if ((uint16_t)handle >= BORLAND_NFILE.word_4d04) {
        not_transcribed("__IOerror after a read on a handle above _nfile");
        return -1;
    }

    if ((uint16_t)(count + 1) < 2
        || (BORLAND_HANDLE_FLAGS.flags[handle] & 0x200) != 0)
        return 0;

    got = dos_read(handle, dg_ptr(dgroup, buf), count);

    if ((uint16_t)(got + 1) < 2
        || (BORLAND_HANDLE_FLAGS.flags[handle] & 0x4000) == 0)
        return got;

    not_transcribed("0x0da6d's text-mode translation, which \"rb\" never uses");
    return -1;
}

/*
 * 0x0d36d
 *
 * Flush every stream that has something to flush: the twenty `FILE` structures
 * from DGROUP 0x4bc4, sixteen bytes apart, taking those whose flags have
 * **both** 0x100 and 0x200 set.
 *
 * The count is walked down rather than up, and the test is on the value before
 * the decrement, so the last structure examined is the first in the table.
 *
 * The flush itself is `flush_stream`. The game only reads, so no stream here
 * ever has both bits, and the call is read rather than measured.
 */
void flush_all_streams(void)
{
    uint16_t si = dg_off(dgroup, &BORLAND_STREAMS.streams[0]);
    int16_t n;

    for (n = 0x14; n != 0; n--) {
        if ((FILEREC_PTR(si)->flags & 0x300) == 0x300)
            flush_stream(FILEREC_PTR(si));
        si = (uint16_t)(si + 0x10);
    }
}

/*
 * 0x0d396
 *
 * Refill a `FILE`'s buffer, and answer 0 or -1. A near routine with a
 * callee-cleaned frame - `ret 2`.
 *
 * A stream marked 0x200 flushes every other stream first. That is what stops a
 * program reading stale data back out of a file something else has written and
 * not yet flushed, and it is why a read can cost a write.
 *
 * The read pointer at +0xa is reset to the buffer base at +8 **before** the
 * read, so the bytes land where the pointer already points.
 *
 * Nothing read is told apart two ways: a count of exactly zero is end of file,
 * setting 0x20 and clearing the 0x180 pair, and anything else is an error,
 * setting 0x10 and forcing the count to zero. Both answer -1, so the caller
 * cannot tell them apart from the answer alone - it has to look at the flags.
 */
int16_t refill_stream(struct file_rec *file)
{
    int16_t got;

    if ((file->flags & 0x200) != 0)
        flush_all_streams();

    file->curp = ((int16_t)file->buffer);

    got = read_translated((int16_t)file->fd, file->buffer,
                          file->bsize);
    file->level = got;

    if (got > 0) {
        file->flags = (int16_t)(file->flags & 0xffdf);
        return 0;
    }

    if (file->level == 0)
        file->flags = (int16_t)((file->flags & 0xfe7f) | 0x20);
    else {
        file->level = 0;
        file->flags = (int16_t)(file->flags | 0x10);
    }
    return -1;
}

/*
 * 0x0d404
 *
 * `fgetc`. Answers the byte, or -1.
 *
 * The fast path is three instructions: if +0 says the buffer still holds
 * something, take a byte and step the pointer at +0xa. Everything else is the
 * slow half.
 *
 * A negative +0, or either of the flags 0x10 and 0x100, or a stream not marked
 * readable at all, is an error at once. Otherwise 0x80 is set - the mark that
 * says this stream has been read from - and the buffer is refilled.
 *
 * After a refill the byte is taken by **jumping back into the fast path**,
 * without re-testing +0. That is safe only because `refill_stream` answers zero
 * exactly when it put something there.
 *
 * A stream with no buffer at all - +6 zero - reads a single byte into a static
 * at DGROUP 0x64c6 instead, and has its own end-of-file dance with `eof`. Not
 * transcribed: every stream the game reads is buffered.
 */
int16_t borland_fgetc(struct file_rec *file)
{
    if (file == 0)
        return -1;

    if (file->level <= 0) {
        if (file->level < 0
            || (file->flags & 0x110) != 0
            || (file->flags & 1) == 0) {
            file->flags = (int16_t)(file->flags | 0x10);
            return -1;
        }

        file->flags = (int16_t)(file->flags | 0x80);

        if (file->bsize == 0) {
            /*
             * 0x0d451: unbuffered. A stream marked 0x200 flushes every other
             * stream first; then one byte is read into DGROUP 0x64c6 through
             * `read_translated`. Nothing read is either the end - `eof`
             * answers 1, and the flags take 0x20 and lose 0x80 and 0x100 -
             * or an error, which sets 0x10; either answers -1. In text mode
             * a carriage return is read past. No stream in this program is
             * unbuffered, so this is read rather than measured.
             */
            for (;;) {
                if ((file->flags & 0x200) != 0)
                    flush_all_streams();

                if (read_translated((int16_t)((int8_t)file->fd),
                                    dg_off(dgroup, &BORLAND_ATEXIT_TABLE.getc_byte), 1) == 0) {
                    if (borland_eof((int16_t)((int8_t)file->fd)) == 1) {
                        file->flags = (uint16_t)((file->flags & 0xfe7f) | 0x20);
                        return -1;
                    }
                    file->flags = (int16_t)(file->flags | 0x10);
                    return -1;
                }

                if (BORLAND_ATEXIT_TABLE.getc_byte == 0x0d && (file->flags & 0x40) == 0)
                    continue;

                file->flags &= (uint16_t)~0x20u;
                return BORLAND_ATEXIT_TABLE.getc_byte;
            }
        }

        if (refill_stream(file) != 0)
            return -1;
    }

    {
        uint16_t p = file->curp;

        file->level--;
        file->curp = (int16_t)(p + 1);
        return *dg_ptr(dgroup, p);
    }
}

/*
 * 0x0d3ef
 *
 * `getc`. Two instructions and a call: **step +0 up** and hand over to `fgetc`.
 *
 * That increment looks pointless until you see who calls it. `buffered_read`
 * decrements +0 to test whether the buffer still holds anything, and only calls
 * here once it has gone negative; this puts it back before `fgetc` looks. The
 * two routines share one counter and each expects the other's convention.
 */
int16_t borland_getc(struct file_rec *file)
{
    file->level++;
    return borland_fgetc(file);
}

/*
 * 0x0ce92
 *
 * `fflush` on one stream. Answers 0, or -1 for a stream that is not open.
 *
 * A null argument means "every open stream", which is 0x0cf13 - not
 * transcribed, and not reached: nothing here flushes them all.
 *
 * The open test is `+0xe == the stream's own address`, which is Borland's way
 * of marking a `FILE` in the table as live without spending a flag.
 *
 * A negative +0 is a stream with buffered *writes*, and its half of this
 * routine is the one that actually writes anything: the bytes in the buffer are
 * `+6 + +0 + 1` - the size, plus the negative free count, plus one - and they
 * go out in a single `write_text`. The pointer is put back to the start of the
 * buffer *before* the write, not after, so a failed write leaves the stream
 * empty rather than holding bytes it could not place.
 *
 * A short write sets **0x10** in the flags, the error bit, unless 0x200 is
 * already set - and answers -1. A full write answers 0 without clearing
 * anything.
 *
 * What remains is bookkeeping. The count at +0 is zeroed, and a stream whose
 * pointer sits at `+5` - the one-byte hold field, so an unbuffered stream - has
 * its pointer reset from +8 as well. The `test +2,8` and the two identical
 * comparisons that follow it are the compiler making one condition out of two.
 */
int16_t flush_stream(struct file_rec *file)
{
    if (file == 0)
        return borland_flushall();

    if (file->token != dg_off(dgroup, file))
        return -1;

    if (file->level < 0) {
        int16_t n = (int16_t)(((int16_t)file->bsize) + file->level + 1);

        file->level = (int16_t)(file->level - n);
        file->curp = file->buffer;

        if (write_text((int16_t)((int8_t)file->fd),
                       dg_ptr(dgroup, file->buffer),
                       (uint16_t)n) == n)
            return 0;

        if ((file->flags & 0x200) != 0)
            return 0;

        file->flags |= 0x10;
        return -1;
    }

    if ((file->flags & 8) == 0) {
        if (file->curp != dg_off(dgroup, &file->hold))
            return 0;
    }

    file->level = 0;

    if (file->curp != dg_off(dgroup, &file->hold))
        return 0;

    file->curp = ((int16_t)file->buffer);
    return 0;
}

/*
 * 0x0d26c
 *
 * `fseek`. Answers 0, or -1 if the flush or the seek failed.
 *
 * Seeking forward from the current position has to account for what is already
 * in the buffer, and that is 0x0d20f - the unread count, with the newline
 * translation taken off. It is only wanted when the buffer holds something and
 * the whence is 1, which does not happen on these screens, so it is a stub.
 *
 * Then the stream is put back to a clean state: flags 0x10, 0x20 and 0x180
 * cleared - `and +2,0xfe5f` - the count zeroed and the pointer reset to the
 * buffer's start. The seek itself is the DOS one, and only a -1 from it is a
 * failure.
 */
int16_t borland_fseek(struct file_rec *file, int32_t off, int16_t whence)
{
    if (flush_stream(file) != 0)
        return -1;

    if (whence == 1 && file->level > 0) {
        not_transcribed("0x0d20f, the unread count");
        return -1;
    }

    file->flags = (int16_t)(file->flags & 0xfe5f);
    file->level = 0;
    file->curp = ((int16_t)file->buffer);

    if (dos_lseek((int8_t)file->fd, (uint16_t)off,
                  (uint16_t)((uint32_t)off >> 16), whence) == -1)
        return -1;

    return 0;
}

/*
 * 0x0c27b
 *
 * `tell` on a DOS handle: `lseek` by zero from the current position, which is
 * the only way to ask DOS where a file is. Four pushes and a call.
 */
int32_t dos_tell(int16_t handle)
{
    return dos_lseek(handle, 0, 0, 1);
}

/*
 * 0x0d20f
 *
 * How many bytes of a stream's buffer have not been handed out yet, so that
 * `ftell` can take them off what DOS reports.
 *
 * The count at +0 is negative while the buffer is being filled and positive
 * while it is being drained, and both are turned into the same magnitude - the
 * negative branch by adding the buffer size at +6 and one, the positive one by
 * `cwd`/`xor`/`sub`, which is how a compiler writes `abs`.
 *
 * A stream in binary mode - flag 0x40 - is done there. A text stream then walks
 * the buffer counting newlines, because each one was two bytes in the file; that
 * is not reached here, every stream the game opens being binary, and it is left
 * as a stub.
 *
 * The original cleans its own argument off the stack - `ret 2` - which is
 * Borland's convention for this helper and not a mistake in the caller.
 */
int16_t unread_count(struct file_rec *file)
{
    int16_t di;

    if (file->level < 0)
        di = (int16_t)(file->bsize + ((uint16_t)file->level) + 1);
    else
        di = (int16_t)(file->level < 0 ? -file->level : file->level);

    if ((file->flags & 0x40) == 0) {
        not_transcribed("0x0d20f's newline scan, for a text stream");
        return 0;
    }

    return di;
}

/*
 * 0x0d2d4
 *
 * `ftell`. DOS is asked where the handle is, and then the buffer is accounted
 * for: bytes read ahead and not yet handed out come **off** the answer, bytes
 * written and not yet flushed go **on** to it, and the sign of the count at +0
 * is what says which.
 *
 * A failed `tell` is passed straight through as -1 without the adjustment.
 */
int32_t borland_ftell(struct file_rec *file)
{
    int32_t p = dos_tell((int8_t)file->fd);

    if (p == -1)
        return p;

    if (file->level < 0)
        return p + unread_count(file);

    return p - unread_count(file);
}

/*
 * 0x0cd80
 *
 * `_close`: INT 21h AH=3Eh, and clear the handle's entry in the flag table at
 * DGROUP 0x4d06. Answers 0, or -1 through `__IOerror` - which is not
 * transcribed, a close that fails not being something these screens do.
 *
 * The flags are cleared **after** DOS agrees, so a handle DOS refused to close
 * keeps its entry.
 */
int16_t dos_close(int16_t handle)
{
    io_dos_close(handle);
    BORLAND_HANDLE_FLAGS.flags[handle] = 0;
    return 0;
}

/*
 * 0x0cd58
 *
 * `close`: the same, with the handle checked against `_nfile` at DGROUP 0x4d04
 * first, and the flag entry cleared **before** the DOS call rather than after.
 * So the two routines disagree about that ordering, and this is the one the
 * runtime uses.
 *
 * An out-of-range handle is errno 6 through `__IOerror`; not transcribed, and
 * measured as never reached.
 */
int16_t close_handle(int16_t handle)
{
    if ((uint16_t)handle >= BORLAND_NFILE.word_4d04) {
        not_transcribed("__IOerror for a handle above _nfile");
        return -1;
    }

    BORLAND_HANDLE_FLAGS.flags[handle] = 0;
    return dos_close(handle);
}

/*
 * 0x0ce15
 *
 * `fclose`. Answers what the close answered, or -1 for a stream that is not
 * open - the same `+0xe == the stream's own address` test `flush_stream` uses.
 *
 * A stream with a buffer is flushed first if it was being written to, and its
 * buffer freed if flag 4 says the runtime allocated it - the free happens
 * whether or not there was a flush. The temporary-file cleanup at the end is
 * still unreached: none of the game's streams has a name to unlink.
 *
 * The `FILE` is then wiped - flags, buffer size and count zeroed, the handle
 * set to 0xff - whether or not the close worked.
 */
int16_t borland_fclose(struct file_rec *file)
{
    int16_t si = -1;

    if (file->token != dg_off(dgroup, file))
        return -1;

    if (file->bsize != 0) {
        /*
         * A stream with bytes still in it is flushed, and a flush that fails
         * abandons the close with -1 - the `FILE` is *not* wiped, so a caller
         * that retries has something to retry.
         *
         * The buffer is freed either way, which is why the `heap_free` sits
         * after the flush rather than inside its else.
         */
        if (file->level < 0 && flush_stream(file) != 0)
            return -1;

        if ((file->flags & 4) != 0)
            heap_free(file->buffer);
    }

    if ((int8_t)file->fd >= 0)
        si = close_handle((int8_t)file->fd);

    file->flags = 0;
    file->bsize = 0;
    file->level = 0;
    file->fd = 0xff;

    /* A temporary stream's file goes with it: the name is rebuilt from the
       number `istemp` holds - no prefix, the static buffer - and unlinked,
       and the number is cleared. The close's own answer is unaffected. */
    if (file->istemp != 0) {
        borland_unlink(tmp_name_build(file->istemp, NULL, NULL));
        file->istemp = 0;
    }

    return si;
}

/*
 * 0x0c018
 *
 * `isatty`: INT 21h AH=44h AL=0, answering bit 7 of the device word - 0x80 for
 * a character device, 0 for a file. Six instructions, and it does not look at
 * the carry flag at all, so a bad handle answers whatever DX happened to hold.
 */
int16_t dos_isatty(int16_t handle)
{
    return (int16_t)(io_dos_devinfo(handle) & 0x80);
}

/*
 * 0x0c8a3
 *
 * The IOCTL call, INT 21h AH=44h, with the sub-function in AL. It answers DX -
 * the device word - when AL is 0 and AX otherwise, and a failure goes to
 * `__IOerror`, which is not transcribed.
 *
 * Only AL=0 is reached here, so only the device word is modelled; the port's
 * `io_dos_devinfo` answers what the emulator does.
 */
int16_t dos_ioctl(int16_t handle, uint16_t al, uint16_t dx, uint16_t cx)
{
    (void)dx;
    (void)cx;

    if (al != 0) {
        not_transcribed("an IOCTL sub-function other than 0");
        return -1;
    }

    return io_dos_devinfo(handle);
}

/*
 * 0x0cd3d
 *
 * `_chmod`: INT 21h AH=43h, with AL choosing between reading the attributes and
 * writing them. Answers CX on success - the attributes - and -1 through
 * `__IOerror` on failure.
 *
 * The runtime uses it as a **file-exists test**: `open_file` asks for the
 * attributes and only looks at bit 0, the read-only flag, and at whether the
 * call worked at all.
 */
int16_t dos_getattr(const char *name, uint16_t al, uint16_t cx)
{
    (void)cx;

    if (al != 0) {
        not_transcribed("0x0cd3d writing a file's attributes");
        return -1;
    }

    return io_dos_getattr(name);
}

/*
 * 0x0d707
 *
 * `_open`: INT 21h AH=3Dh. The access mode in AL is worked out from the flags -
 * 2 for read-write, 1 for write-only, 0 for read - and the sharing bits at 0xf0
 * are passed through beside it.
 *
 * On success the handle's entry in the flag table at DGROUP 0x4d06 is set from
 * the flags with 0x0700 cleared and 0x8000 - "open" - added. A failure goes to
 * `io_error` with the DOS code.
 *
 * **The access mode changes nothing on this side.** A handle in the port is a
 * buffer either way; whether writing it survives is decided by where the file
 * came from - the write overlay or the host - and not by what was asked for at
 * the open. That is the emulator's model too, and the mode is computed here
 * because the original computes it, not because anything downstream reads it.
 */
int16_t dos_open_named(const char *name, uint16_t flags)
{
    char path[256];
    uint16_t i;
    int16_t h;
    uint8_t access;

    if ((flags & 2) != 0)
        access = 1;
    else if ((flags & 4) != 0)
        access = 2;
    else
        access = 0;

    access = (uint8_t)(access | (flags & 0xf0));
    (void)access;

    for (i = 0; i < sizeof path - 1 && name[i] != 0; i++)
        path[i] = name[i];
    path[i] = 0;

    h = io_dos_open(path);
    if (h < 0)
        return io_error(2);               /* DOS 2: file not found */

    BORLAND_HANDLE_FLAGS.flags[h] = (int16_t)((flags & 0xb8ff) | 0x8000);
    return h;
}

/*
 * 0x0cf4d
 *
 * Parse a mode string into the two words `fopen` needs: the open flags, and a
 * permission word for a file that has to be created. Answers a third value -
 * 1, 2 or 3 for read, write and update, with 0x40 added for binary - or 0 for a
 * mode string that starts with none of `r`, `w` or `a`.
 *
 * The `+` may come before or after the `t`/`b`, which is why the second
 * character is looked at twice. With neither `t` nor `b` the default comes from
 * DGROUP 0x4d2e, the global text/binary setting.
 *
 * It also plants a far pointer at DGROUP 0x4bbc on the way out, which is
 * nothing to do with the mode; it is transcribed because it happens.
 *
 * **The segment half of that pointer is a relocation.** In the recovered image
 * the immediate reads 0x0000, because the image is unrelocated; the loader
 * patches it to the program's own base. Transcribing the 0 as written left
 * DGROUP 0x4bbe zero where the original had 0x0110, which is that base. Any
 * immediate that is a segment has to be worked out from where the program
 * actually is, never read off the bytes.
 *
 * The original cleans its own arguments - `ret 6`.
 */
int16_t parse_open_mode(uint8_t * out_perm, uint8_t * out_flags, const char *mode)
{
    uint16_t perm = 0;
    uint16_t flags;
    int16_t r;
    char c = *mode;

    mode++;

    if (c == 'r') {
        flags = 1;
        r = 1;
    } else if (c == 'w') {
        flags = 0x302;
        perm = 0x80;
        r = 2;
    } else if (c == 'a') {
        flags = 0x902;
        perm = 0x80;
        r = 2;
    } else {
        return 0;
    }

    c = *mode;
    mode++;

    if (c == '+' || (*mode == '+' && (c == 't' || c == 'b'))) {
        if (c != '+')
            c = *mode;
        flags = (uint16_t)((flags & 0xfffc) | 4);
        perm = 0x180;
        r = 3;
    }

    if (c == 't') {
        flags |= 0x4000;
    } else if (c == 'b') {
        flags |= 0x8000;
        r |= 0x40;
    } else {
        flags |= (uint16_t)(BORLAND_IO_MODES.word_4d2e & 0xc000);
        if ((flags & 0x8000) != 0)
            r |= 0x40;
    }

    BORLAND_EXIT_VECTORS.exit_fopen.seg = (uint16_t)(IMAGE_BASE >> 4);
    BORLAND_EXIT_VECTORS.exit_fopen.off = 0xdfb4;       /* exit_close_streams */

    *(int16_t *)(out_flags) = (int16_t)flags;
    *(int16_t *)(out_perm) = (int16_t)perm;

    return r;
}

/*
 * 0x0d784
 *
 * `fputc`.
 *
 * **The character is filed at DGROUP 0x64c8 before anything else**, and stays
 * there: the unbuffered path hands `dos_write` that *address* rather than a
 * copy, and the return value is read back out of it at the end. So a global is
 * doing the work a local would, because a one-byte write needs somewhere with
 * an address.
 *
 * The fast path is `file[0] < -1` - room in the buffer, the counter being the
 * negative space left - and it stores the byte, bumps both, and returns. A
 * **line-buffered** stream, flag 8, flushes on a `\n` *or* a `\r`.
 *
 * When there is no room the stream has to be able to take more: flags 0x90
 * mean it cannot - end of file or already in error - and flag 2 means it is a
 * write stream. Anything else sets the error bit 0x10 and answers -1. Then
 * 0x100 goes on, which is "has been written to".
 *
 * A stream with **no buffer at all** writes the byte straight to the handle,
 * and for a text stream - `\n` with 0x40 clear - it writes a `\r` first, from
 * the one-byte constant at DGROUP 0x4e3a. That is two DOS calls per newline,
 * which is why nothing writes a text stream a byte at a time by choice.
 */
int16_t borland_fputc(int16_t c, struct file_rec *file)
{
    int16_t handle;

    BORLAND_FPUTC_CHAR.character = (uint8_t)c;

    if (file->level < -1) {
        file->level++;
        *dg_ptr(dgroup, file->curp) = BORLAND_FPUTC_CHAR.character;
        file->curp++;

        if ((file->flags & 8) == 0)
            return (int16_t)BORLAND_FPUTC_CHAR.character;
        if (BORLAND_FPUTC_CHAR.character != '\n' && BORLAND_FPUTC_CHAR.character != '\r')
            return (int16_t)BORLAND_FPUTC_CHAR.character;
        if (flush_stream(file) == 0)
            return (int16_t)BORLAND_FPUTC_CHAR.character;

        return -1;
    }

    for (;;) {
        if ((file->flags & 0x90) != 0 || (file->flags & 2) == 0) {
            file->flags |= 0x10;
            return -1;
        }

        file->flags |= 0x100;

        if (file->bsize != 0) {
            if (file->level != 0 && flush_stream(file) != 0)
                return -1;

            file->level = (int16_t)(-((int16_t)file->bsize));
            *dg_ptr(dgroup, file->curp) = BORLAND_FPUTC_CHAR.character;
            file->curp++;

            if ((file->flags & 8) == 0)
                return (int16_t)BORLAND_FPUTC_CHAR.character;
            if (BORLAND_FPUTC_CHAR.character != '\n' && BORLAND_FPUTC_CHAR.character != '\r')
                return (int16_t)BORLAND_FPUTC_CHAR.character;
            if (flush_stream(file) == 0)
                return (int16_t)BORLAND_FPUTC_CHAR.character;

            return -1;
        }

        handle = (int16_t)((int8_t)file->fd);

        if ((BORLAND_HANDLE_FLAGS.flags[handle] & 0x800) != 0)
            dos_lseek(handle, 0, 0, 2);

        if (BORLAND_FPUTC_CHAR.character == '\n' && (file->flags & 0x40) == 0) {
            if (dos_write(handle, dg_ptr(dgroup, 0x4e3a /* "\r" */), 1) != 1)
                goto failed;
        }

        if (dos_write(handle, dg_ptr(dgroup, 0x64c8), 1) == 1)
            return (int16_t)BORLAND_FPUTC_CHAR.character;

    failed:
        /*
         * 0x200 says the caller does not want the error bit set, and the byte
         * is reported as written anyway. Otherwise round the loop again, which
         * lands on the flag test above and turns into the -1 return.
         */
        if ((file->flags & 0x200) != 0)
            return (int16_t)BORLAND_FPUTC_CHAR.character;

        file->flags |= 0x10;
        return -1;
    }
}

/*
 * 0x0d76b
 *
 * `putc`: decrement the stream's free-space counter and hand the character to
 * `fputc`. Two instructions of its own.
 *
 * The decrement is **not** an optimisation that `fputc` then undoes - `fputc`
 * increments it back on the fast path, so the pair leaves it where it was and
 * the byte is stored. What the decrement buys is that a caller which has
 * already tested the counter and found room does not have to say so.
 */
int16_t borland_putc(int16_t c, struct file_rec *file)
{
    file->level--;
    return borland_fputc(c, file);
}

/*
 * 0x0de6e
 *
 * The runtime's **`write`**: everything `dos_write` is, plus the checks and the
 * text expansion. It is what a buffered stream's flush goes through, so the
 * name is misleading - the text part is the branch it does *not* usually take.
 *
 * A handle at or above DGROUP 0x4d04, the size of the flag table, is `io_error`
 * 6 - a bad handle - before anything else touches it.
 *
 * **The length test is `count + 1 < 2`**, unsigned, which rejects both 0 and
 * 0xffff in one comparison. A zero-length write answering 0 here is why the
 * *truncating* write has its own door at 0x0d59d: this one would swallow it.
 *
 * Append - 0x800 in the handle's flags - seeks to the end first, because DOS
 * does not do it for you.
 *
 * Then the fork: a handle **without** 0x4000 is binary, and the bytes go
 * straight to `dos_write`. A text handle takes the expansion. The game opens
 * every file "rb" or "wb", so none of those does - but **`stdout` is text**,
 * and `game_teardown` prints the message it leaves with through it, so
 * quitting the game comes this way. It was a stub that aborted until that was
 * found by quitting.
 *
 * **The expansion** clears 0x200 in the handle's flags and copies the caller's
 * bytes into 0x80 bytes at the bottom of its own frame, putting a CR in front
 * of every LF. The fill is tested *after* up to two bytes are stored - `jl`
 * against 0x80 - so a CR LF landing at 0x7f reaches index 0x81, which is still
 * inside the 0x88-byte frame; the port's array is that 0x82. A full buffer is
 * handed to `dos_write` and refilled from the start.
 *
 * What it answers is the count it was asked for, unless a `dos_write` came
 * back short: -1 if that answered -1, and otherwise the caller's count, minus
 * what was still to copy, plus what was written, minus what was asked of that
 * write. So a short write is reported in the caller's bytes, CRs aside.
 */
int16_t write_text(int16_t handle, const uint8_t * buf, uint16_t count)
{
    if ((uint16_t)handle >= BORLAND_NFILE.word_4d04)
        return io_error(6);             /* DOS 6: invalid handle */

    if ((uint16_t)(count + 1) < 2)
        return 0;

    if ((BORLAND_HANDLE_FLAGS.flags[handle] & 0x800) != 0)
        dos_lseek(handle, 0, 0, 2);

    if ((BORLAND_HANDLE_FLAGS.flags[handle] & 0x4000) == 0)
        return dos_write(handle, buf, count);

    BORLAND_HANDLE_FLAGS.flags[handle] &= (int16_t)0xfdff;

    {
        uint8_t out[0x82];                  /* [bp-0x88]: 0x80, and the CR LF past it */
        const uint8_t *src = buf;           /* [bp-6] */
        uint16_t remaining = count;         /* [bp-2] */
        uint16_t at = 0;                    /* SI, as an index into `out` */
        uint16_t n;
        int16_t written;                    /* DX */

        while (remaining != 0) {
            uint8_t ch;                     /* [bp-3] */

            remaining--;
            ch = *src++;
            if (ch == 0x0a)
                out[at++] = 0x0d;
            out[at++] = ch;

            if ((int16_t)at < 0x80)
                continue;

            n = at;
            written = dos_write(handle, out, n);
            if ((uint16_t)written == n) {
                at = 0;
                continue;
            }
            if (written == -1)
                return -1;
            return (int16_t)(count - remaining + (uint16_t)written - n);
        }

        n = at;
        if (n == 0)
            return (int16_t)count;

        written = dos_write(handle, out, n);
        if ((uint16_t)written == n)
            return (int16_t)count;
        if (written == -1)
            return -1;
        return (int16_t)(count + (uint16_t)written - n);
    }
}

/*
 * 0x0d524
 *
 * `memcpy`, near. A `rep movsw` for the pairs and one `movsb` for an odd byte,
 * the carry out of `shr cx,1` deciding whether there is one. Answers the
 * destination.
 */
uint8_t * mem_copy(uint8_t * dst, const uint8_t * src, uint16_t n)
{
    uint16_t i;

    for (i = 0; i < n; i++)
        dst[i] = src[i];

    return dst;
}

/*
 * 0x0df7a
 *
 * DOS's **write**, INT 21h AH=40h, with a handle rather than a stream.
 *
 * It refuses first: **bit 0 of the handle's flag word** at DGROUP 0x4d06 is
 * "this handle is read-only", and a write to one answers `io_error(5)` -
 * access denied - without asking DOS. That check is the runtime's, not DOS's.
 *
 * On success it sets **0x1000** in the same word, which is the "has been
 * written" bit the close path looks at.
 */
int16_t dos_write(int16_t handle, const uint8_t * buf, uint16_t count)
{
    int16_t n;

    if ((BORLAND_HANDLE_FLAGS.flags[handle] & 1) != 0)
        return io_error(5);             /* DOS 5: access denied */

    n = io_dos_write(handle, buf, count);

    if (n < 0)
        return io_error(5);

    BORLAND_HANDLE_FLAGS.flags[handle] |= 0x1000;
    return n;
}

/*
 * 0x0d584
 *
 * DOS's **create**, INT 21h AH=3Ch: the attribute in `CX`, the name in `DX`,
 * the handle back in `AX`. A carry files the code through `io_error`, which
 * answers -1, and success falls past that with the handle already in `AX`.
 *
 * `ret 4` - it takes its arguments in the *caller's* frame and pops them
 * itself, which is Borland's `__pascal` convention and is why the two words
 * are at `[bp+4]` and `[bp+6]` rather than at `[bp+6]` and `[bp+8]`.
 *
 * The create itself is `io_dos_creat`, which is the port's own: it makes the
 * file in the write overlay and never on the host.
 */
int16_t dos_creat(const char *name, uint16_t attr)
{
    char path[256];
    uint16_t i;
    int16_t h;

    (void)attr;

    for (i = 0; i < sizeof path - 1 && name[i] != 0; i++)
        path[i] = name[i];
    path[i] = 0;

    h = io_dos_creat(path);
    if (h < 0)
        return io_error(3);             /* DOS 3: path not found */

    return h;
}

/*
 * 0x0d59d
 *
 * DOS's **write**, INT 21h AH=40h - and this one is the *truncating* door:
 * `CX` and `DX` are both zeroed before the call, so it always writes **zero
 * bytes**, which DOS reads as "cut the file here". The runtime calls it to
 * empty a file it is about to rewrite.
 *
 * `ret 2`, one word: the handle. Nothing else is passed because nothing else
 * is needed to say "end the file at the current position".
 */
void dos_truncate(int16_t handle)
{
    io_dos_write(handle, NULL, 0);
}

/*
 * 0x0d5af
 *
 * `open`, over `dos_open_named`. Answers the handle, or a negative.
 *
 * With neither text nor binary asked for, the default at DGROUP 0x4d2e is
 * added. The attributes are then read - the file-exists test - and the file
 * opened.
 *
 * Afterwards the handle's flag entry at DGROUP 0x4d06 is rewritten: the flags
 * with 0x0700 cleared, 0x1000 added when the file was opened for writing, and
 * **0x100 added when the file is not read-only** - which is the one thing the
 * attribute read is for.
 *
 * Creating and truncating are both here now, reached by Save Machine. The
 * character-device branch is still a stub and still unreached: the game opens
 * files and never `CON`.
 */
int16_t open_file(const char *name, uint16_t flags, uint16_t perm)
{
    int16_t attr;
    int16_t h;
    int16_t info;

    if ((flags & 0xc000) == 0)
        flags |= (uint16_t)(BORLAND_IO_MODES.word_4d2e & 0xc000);

    attr = dos_getattr(name, 0, 0);

    /*
     * **The create branch.** `flags & 0x100` is `O_CREAT`, and what happens
     * next depends on whether the file was already there:
     *
     *   it was, and `O_EXCL` (0x400) is asked for - that is an error, DOS 0x50;
     *   it was, and `O_EXCL` is not - fall through and just *open* it, without
     *   truncating anything;
     *   it was not - create it, with an attribute worked out below.
     *
     * So `O_CREAT` on an existing file does **not** empty it here. Emptying is
     * the zero-length write the runtime does afterwards, which is why that
     * write has to work.
     *
     * The permission argument is masked with DGROUP 0x4d30 and, if nothing is
     * left of the read and write bits (0x180), `io_error(1)` is filed - and the
     * routine carries on anyway. The errno is set for a caller that looks; the
     * open is not abandoned.
     */
    if ((flags & 0x100) != 0) {
        uint16_t perms = (uint16_t)(perm & BORLAND_IO_MODES.word_4d30);

        if ((perms & 0x180) == 0)
            io_error(1);

        if (attr == -1) {
            /*
             * Not there. `io_error` has already filed the DOS code in 0x4d34;
             * anything but 2 - "file not found" - is a real failure, because a
             * create is only justified by the file's absence.
             */
            if (((uint16_t)BORLAND_IO_MODES.word_4d34) != 2)
                return io_error((int16_t)((uint16_t)BORLAND_IO_MODES.word_4d34));

            attr = (int16_t)((perms & 0x80) ? 0 : 1);

            if ((flags & 0xf0) != 0) {
                h = dos_creat(name, 0);
                if (h < 0)
                    return h;
                dos_close(h);
            } else {
                h = dos_creat(name, (uint16_t)attr);
                if (h < 0)
                    return h;
                goto have_handle;
            }
        } else if ((flags & 0x400) != 0) {
            return io_error(0x50);      /* DOS 0x50: file already exists */
        }
    }

    h = dos_open_named(name, flags);
    if (h >= 0) {
        info = dos_ioctl(h, 0, 0, 0);
        if ((info & 0x80) != 0) {
            not_transcribed("0x0d67f, opening a character device");
            return -1;
        }
        /*
         * **0x200 is truncate-on-open**, and it is done with a *zero-length
         * write* rather than with anything named like a truncate. An earlier
         * reading of this branch had it as an append seek, which is the same
         * shape and the opposite meaning.
         */
        if ((flags & 0x200) != 0)
            dos_truncate(h);

        /*
         * A read-only file that was just created with sharing bits has its
         * attribute put back afterwards - the create had to leave it writable
         * to write it.
         */
        if ((attr & 1) != 0 && (flags & 0x100) != 0 && (flags & 0xf0) != 0)
            dos_getattr(name, 1, 1);
    }

have_handle:
    if (h < 0)
        return h;

    {
        uint16_t v = (uint16_t)((flags & 0xf8ff)
                                | ((flags & 0x300) ? 0x1000 : 0));

        v |= (uint16_t)((attr & 1) ? 0 : 0x100);
        BORLAND_HANDLE_FLAGS.flags[h] = (int16_t)v;
    }

    return h;
}

/*
 * 0x0db5e
 *
 * `setvbuf`. Answers 0, or -1 for a stream that is not open, a mode above 2 or
 * a size above 0x7fff.
 *
 * Two flags at DGROUP 0x4e3c and 0x4e3e remember that `stdin` and `stdout` -
 * the streams at 0x4bc4 and 0x4bd4 - have had a buffer set, so the runtime does
 * not do it again behind the program's back.
 *
 * Whatever the stream had is undone first: seeked back to nought if anything
 * was buffered, the old buffer freed if flag 4 says the runtime owned it, and
 * the pointer set to the one-byte hold field at +5 so an unbuffered stream is
 * consistent before anything else happens.
 *
 * A caller that asks for buffering without supplying a buffer gets one from the
 * heap, and flag 4 records that. Mode 1 adds flag 8, which is line buffering.
 *
 * The far pointer planted at DGROUP 0x4bb8 has the same relocated segment as
 * the one in `parse_open_mode`.
 */
int16_t borland_setvbuf(struct file_rec *file, uint16_t buf, int16_t mode, uint16_t size)
{
    if (file->token != dg_off(dgroup, file) || mode > 2 || size > 0x7fff)
        return -1;

    if (DG4E34.stdout_is_tty == 0 && file == &BORLAND_STREAMS.streams[1])
        DG4E34.stdout_is_tty = 1;
    else if (DG4E34.stdin_is_tty == 0 && file == &BORLAND_STREAMS.streams[0])
        DG4E34.stdin_is_tty = 1;

    if (file->level != 0)
        borland_fseek(file, 0, 1);

    if ((file->flags & 4) != 0)
        heap_free(file->buffer);

    file->flags = (int16_t)(file->flags & 0xfff3);
    file->bsize = 0;
    file->buffer = dg_off(dgroup, &file->hold);
    file->curp = dg_off(dgroup, &file->hold);

    if (mode == 2 || size == 0)
        return 0;

    BORLAND_EXIT_VECTORS.exit_buf.seg = (uint16_t)(IMAGE_BASE >> 4);
    BORLAND_EXIT_VECTORS.exit_buf.off = 0xdfdc;         /* exit_flush_streams */

    if (buf == 0) {
        buf = heap_malloc(size);
        if (buf == 0)
            return -1;
        file->flags = (int16_t)(file->flags | 4);
    }

    file->curp = (int16_t)buf;
    file->buffer = (int16_t)buf;
    file->bsize = (int16_t)size;

    if (mode == 1)
        file->flags = (int16_t)(file->flags | 8);

    return 0;
}

/*
 * 0x0d0a3
 *
 * Find a free entry in the `FILE` table at DGROUP 0x4bc4, 0x10 bytes apiece and
 * `_nfile` of them, or 0.
 *
 * Free means a **negative** handle byte at +4, which is the 0xff `fclose`
 * leaves. The loop's exit and its found-test are the same comparison written
 * twice, so a table that is entirely full falls out of the bottom and is tested
 * once more before answering 0.
 */
struct file_rec *find_free_stream(void)
{
    struct file_rec *si  = &BORLAND_STREAMS.streams[0];
    struct file_rec *end = &BORLAND_STREAMS.streams[BORLAND_NFILE.word_4d04];

    while ((int8_t)si->fd >= 0) {
        struct file_rec *prev = si;

        si++;
        if (end <= prev)
            break;
    }

    if ((int8_t)si->fd >= 0)
        return NULL;

    return si;
}

/*
 * 0x0d007
 *
 * The body of `fopen`, over a `FILE` the caller has already found. Answers the
 * `FILE`, or 0.
 *
 * The mode string is parsed, the flags stored at +2, and the file opened -
 * unless the caller had already put a handle in +4, in which case the open is
 * skipped and only the flags are taken. Either way a negative handle wipes the
 * `FILE` and answers 0.
 *
 * `isatty` on the handle adds flag 0x200, and that flag then chooses the
 * buffering: a character device gets mode 1, line buffering, and a file gets
 * mode 0 with a 0x200-byte buffer from the heap. A `setvbuf` that fails closes
 * the stream again.
 *
 * The arguments are **mode before name**, which is the opposite of `fopen`'s
 * own order: 0x0d0ce pushes them the other way round. Reading them the way the
 * caller declares them gave a mode string that started with none of `r`, `w`
 * or `a`, so the parse refused and the open never happened.
 *
 * The original cleans its own arguments - `ret 8`.
 */
struct file_rec *borland_fopen_into(uint16_t extra_flags, const char *mode, const char *name,
                          struct file_rec *file)
{
    int16_t perm;                    /* [bp-4] */
    int16_t flags;   /* [bp-2] */
    struct file_rec *r = NULL;

    file->flags = parse_open_mode((uint8_t *)&perm,
                                          (uint8_t *)&flags,
                                          mode);

    if (file->flags == 0)
        goto fail;

    if ((int8_t)file->fd < 0) {
        file->fd = (uint8_t)open_file(name,
                                           (uint16_t)((uint16_t)flags
                                                      | extra_flags),
                                           (uint16_t)perm);
        if ((int8_t)file->fd < 0)
            goto fail;
    }

    if (dos_isatty((int8_t)file->fd) != 0)
        file->flags = (int16_t)(file->flags | 0x200);

    if (borland_setvbuf(file, 0,
                      (int16_t)((file->flags & 0x200) ? 1 : 0),
                      0x200) != 0) {
        borland_fclose(file);
        goto fail;
    }

    file->istemp = 0;
    r = file;
    goto out;

fail:
    file->fd = 0xff;
    file->flags = 0;

out:
    return r;
}

/*
 * 0x0d0ce
 *
 * `fopen`. Finds a free `FILE` and hands it to the body above with no extra
 * flags. Answers the `FILE`, or 0 when the table is full.
 */
struct file_rec *borland_fopen(const char *name, const char *mode)
{
    struct file_rec *file = find_free_stream();

    if (file == NULL)
        return NULL;

    return borland_fopen_into(0, mode, name, file);
}

/*
 * 0x0be3e
 *
 * A 32-bit shift left, `DX:AX` by `CL`. Two paths - under sixteen bits it
 * shifts both halves and carries the bits that fall off the low one into the
 * high one, sixteen or more it moves the low half up and zeroes it - and the
 * port writes the shift they both compute.
 *
 * The entry is the near door of the pair: `pop bx / push cs / push bx` turns
 * the caller's return address into the far one the `retf` wants.
 *
 * **0x0be41 is the same routine.** These three bytes are the near door - `pop
 * es / push cs / push es`, turning a near caller's return address into the far
 * one the `retf` wants - and a far caller jumps straight past them. The pair at
 * 0x0be7f and 0x0be82 is arranged the same way.
 */
uint32_t long_shift_left(uint32_t v, uint8_t count)
{
    return (uint32_t)(v << count);
}

/*
 * 0x0dd55
 *
 * `stricmp`. Answers the difference of the first pair of characters that
 * differ, with both folded to upper case only **after** they have failed to
 * match exactly - so an exact match never pays for the folding.
 *
 * The letter range is kept in one register pair, `CH` holding `a` and `CL`
 * holding `z`, which is why the two range tests are against a register rather
 * than an immediate.
 *
 * A NUL in the first string ends it before the comparison, so the answer there
 * is `0 - *b`.
 */
int16_t string_compare_nocase(const char *a, const char *b)
{
    for (;;) {
        uint8_t al = (uint8_t)*a;
        uint8_t bl = (uint8_t)*b;

        a++;
        if (al == 0)
            return (int16_t)((uint16_t)al - (uint16_t)bl);

        b++;
        if (al == bl)
            continue;

        if (al >= 'a' && al <= 'z')
            al = (uint8_t)(al - 0x20);
        if (bl >= 'a' && bl <= 'z')
            bl = (uint8_t)(bl - 0x20);

        if (al != bl)
            return (int16_t)((uint16_t)al - (uint16_t)bl);
    }
}

/*
 * 0x0ddaf
 *
 * `strncpy`. Copies up to `n` bytes and pads the rest with zeros, which is what
 * the second `rep` is for. Answers the destination.
 *
 * The length is found first with a bounded `repne scasb`, so a source with no
 * NUL inside `n` copies exactly `n` bytes and pads nothing.
 */
char *string_copy_padded(char *dst, const char *src, uint16_t n)
{
    uint16_t i = 0;

    while (i < n && src[i] != 0) {
        dst[i] = src[i];
        i++;
    }

    if (i < n) {
        dst[i] = 0;
        i++;
    }

    while (i < n) {
        dst[i] = 0;
        i++;
    }

    return dst;
}

/*
 * 0x0bd70
 *
 * `getvect`: INT 21h AH=35h, answering the interrupt vector as `DX:BX` - which
 * the caller reads as `DX:AX` after the `xchg`: one far pointer.
 *
 * The port reads the vector table itself. It is at absolute 0 and is part of
 * the memory the verifier seeds and compares, so this needs nothing invented.
 */
struct far_ptr dos_getvect(uint16_t n)
{
    return *(const struct far_ptr *)(guest_mem + 4 * (n & 0xff));
}

/*
 * 0x0bd7f
 *
 * `setvect`: INT 21h AH=25h. Ten instructions, of which two are saving and
 * restoring DS around the `lds` that loads the handler.
 *
 * The port writes the vector table directly, for the same reason `getvect`
 * reads it.
 */
void dos_setvect(uint16_t n, uint16_t off, uint16_t seg)
{
    uint8_t *v = guest_mem + 4 * (n & 0xff);

    *(uint16_t *)v = off;
    *(uint16_t *)(v + 2) = seg;
}

/*
 * 0x0bfcd
 *
 * `__IOerror`: turn a DOS error code into `errno`, and answer -1 so the caller
 * can `return __IOerror(ax)`.
 *
 * A **positive** code is a DOS one: it is remembered at DGROUP 0x4d34 as
 * `_doserrno` and mapped through the table at 0x4d36 to a C errno. Anything
 * above 0x58 is clamped to 0x57 first, so an unknown code lands on the last
 * entry rather than off the end of the table.
 *
 * A **negative** code is already a C errno, negated: `_doserrno` is set to -1
 * and the value used directly. That path also clamps, by jumping into the
 * positive path's clamp - so a negated value beyond 0x23 is turned into DOS
 * code 0x57 and mapped, which is not what the negation meant. Transcribed as it
 * stands.
 *
 * `errno` is DGROUP 0x94. The original cleans its own argument - `ret 2`.
 */
int16_t io_error(int16_t code)
{
    int16_t si = code;

    if (si >= 0) {
        if (si > 0x58)
            si = 0x57;
        BORLAND_IO_MODES.word_4d34 = si;
        si = BORLAND_IO_MODES.errno_map[si];
    } else {
        si = (int16_t)(-si);
        if (si > 0x23) {
            si = 0x57;
            BORLAND_IO_MODES.word_4d34 = si;
            si = BORLAND_IO_MODES.errno_map[si];
        } else {
            BORLAND_IO_MODES.word_4d34 = -1;
        }
    }

    DG0094.err_no = si;
    return -1;
}

/*
 * 0x0dd33
 *
 * `strcpy`. The length is found first with a `repne scasb` over 0xffff bytes
 * and the copy is one `rep movsb`, so the NUL is copied with the rest. Answers
 * the destination.
 */
char *string_copy(char *dst, const char *src)
{
    uint16_t i = 0;

    for (;;) {
        dst[i] = src[i];
        if (src[i] == 0)
            break;
        i++;
    }

    return dst;
}

/*
 * 0x0dcce
 *
 * `strchr`. The original reads a **word** at a time once the pointer is even,
 * testing both halves, so an odd start gets one `lodsb` first to align. Answers
 * a pointer to the match or zero - and the two exits differ by the `inc si`
 * that makes `[si-2]` name the high half instead of the low one.
 */
char *string_chr(char *s, char c)
{
    for (;;) {
        if (*s == c)
            return s;
        if (*s == 0)
            return NULL;
        s++;
    }
}

/*
 * 0x0dd04
 *
 * `strcmp`. The length of the **second** string is measured first with a
 * `repne scasb`, and that length is what the `repe cmpsb` runs for - so the
 * comparison stops at the second string's NUL, whichever string is shorter.
 * The answer is the difference of the last two bytes compared, which for equal
 * strings is the two NULs and therefore zero.
 */
int16_t string_compare(const char *a, const char *b)
{
    uint16_t n = string_length(b) + 1;

    while (n != 0) {
        if (*a != *b)
            return (int16_t)((uint16_t)(uint8_t)*a - (uint16_t)(uint8_t)*b);
        if (*a == 0)
            break;
        a++;
        b++;
        n--;
    }

    return 0;
}

/*
 * 0x0dddb
 *
 * `strnicmp`. The fold is upper-case, not lower: `dx` is seeded with 0x617a -
 * `a` in `dh` and `z` in `dl` - and a byte inside that range has 0x20 taken
 * off. Both bytes are folded, and only after the raw comparison has already
 * failed, so a matching pair never goes through it.
 *
 * The answer is the difference of the two bytes as they stood at the exit, and
 * the exits do not agree about folding: running out of count leaves a folded
 * pair, and reaching the **first** string's NUL leaves the second string's byte
 * *unfolded*. It never matters to a caller that only asks whether the answer is
 * zero.
 */
int16_t string_ncompare_i(const char *a, const char *b, uint16_t n)
{
    uint16_t al = 0, bl = 0;

    for (;;) {
        if (n == 0)
            break;

        al = (uint8_t)*a;
        a++;
        bl = (uint8_t)*b;

        if (al == 0)
            break;

        b++;
        n--;

        if (al != bl) {
            if (al >= 'a' && al <= 'z')
                al -= 0x20;
            if (bl >= 'a' && bl <= 'z')
                bl -= 0x20;
            if (al != bl)
                break;
        }
    }

    return (int16_t)(al - bl);
}

/*
 * 0x0de1e
 *
 * `strrev`, in place. The length comes from a `repne scasb`, and the guard is
 * `cx == -2` - the value the counter has after scanning exactly one byte, the
 * terminator - so an **empty string is left alone** rather than having its
 * pointers cross.
 *
 * It answers the buffer it was given, and it does **not** put it back: a caller
 * that still wants the original order has to have kept a copy.
 */
char *string_reverse(char *s)
{
    char *i = s;
    char *j;
    uint16_t n = string_length(s);

    if (n == 0)
        return s;

    j = s + n - 1;

    while (i < j) {
        char t = *i;

        *i = *j;
        *j = t;
        i++;
        j--;
    }

    return s;
}

/*
 * 0x0de4e
 *
 * `strupr`, in place. The test is one unsigned comparison rather than two:
 * `b - 'a'` is taken first and compared against 0x19, so anything below `a`
 * wraps past it and is left alone. It answers the pointer it was given, kept in
 * `dx` across the loop because `lodsb` is walking `si`.
 */
char *string_upper(char *s)
{
    char *si = s;

    while (*si != 0) {
        if ((uint8_t)(*si - 'a') <= 0x19)
            *si = (char)(*si - 'a' + 'A');
        si++;
    }

    return s;
}

/*
 * 0x0dd95
 *
 * `strlen`. One `repne scasb` over 0xffff bytes, then `not` and `dec` on what
 * is left of the counter - the count of bytes *not* scanned, complemented, less
 * the NUL the scan stopped on.
 */
uint16_t string_length(const char *s)
{
    uint16_t n = 0;

    while (s[n] != 0)
        n++;

    return n;
}


/*
 * 0x0bd4a
 *
 * `getdate`: INT 21h AH=2Ah, with the year written to the caller's +0 and the
 * packed month and day to +2. The weekday DOS puts in AL is dropped.
 */
void dos_getdate(uint8_t * out)
{
    uint16_t year, monthday, weekday;

    io_dos_getdate(&year, &monthday, &weekday);

    *(int16_t *)(out) = (int16_t)year;
    *(int16_t *)(out + 2) = (int16_t)monthday;
}

/*
 * 0x0dc95
 *
 * `strcat`. Both strings are measured with a `repne scasb` first, and the copy
 * is words with a trailing byte.
 *
 * The alignment step - `test si,1`, then `movsb` and **`dec cx`** - takes its
 * byte off the count, so the total is right either way. That `dec` is easy to
 * miss: it is the single byte between the `movsb` and the `shr`, and a
 * disassembly window that stops at the `movsb` does not show it, and reading it
 * as absent turns a correct routine into an apparent off-by-one.
 *
 * `far_memcpy` at 0x222c6 has the same `movsb`/`dec cx` pair guarded by `jae`
 * instead of `je` - and `test` always clears carry, so there the alignment step
 * never runs at all. The count stays right either way; only the alignment is
 * lost. The two routines are wrong and right in different places.
 */
char *string_concat(char *dst, const char *src)
{
    char *d = dst;
    uint16_t n = 0;

    while (*d != 0)
        d++;

    while (src[n] != 0)
        n++;
    n++;                                  /* the NUL counts */

    /*
     * **The parity is the DGROUP offset's, not the host pointer's.** The
     * original aligns with one `movsb` when the source is odd, and "odd" there
     * means odd in the segment. A host address has its own parity and it is a
     * different number - the bytes copied come out the same either way, but
     * the branch would no longer be the one the original takes. `dg_off` is
     * what makes it the same test.
     *
     * **And a source that is not the guest's has no such parity at all.**
     * Three callers hand this a C array - `count_level_files`, `load_level`
     * and `load_part_bitmap`, whose buffers stopped being DGROUP frames when
     * the string routines took pointers - and `dg_off` on one of those is the
     * distance between two unrelated objects, so the branch was being decided
     * by a number that means nothing. It cost nothing, because the two arms
     * copy the same bytes: the step copies one and takes one off `n`. So the
     * port asks the question only where there is an offset to ask it about,
     * and takes the even arm otherwise - the one an aligned buffer gets.
     */
    if (dg_is_guest(src) && (dg_off(dgroup, src) & 1) != 0) {
        *d = *src;
        d++;
        src++;
        n--;
    }

    while (n-- != 0) {
        *d = *src;
        d++;
        src++;
    }

    return dst;
}

/*
 * 0x0c1b2
 *
 * `setbuf`: `setvbuf` with a fixed size of 0x200 and the mode chosen by whether
 * a buffer was given - 0 for full buffering with one, 2 for none without.
 */
int16_t borland_setbuf(struct file_rec *file, uint16_t buf)
{
    return borland_setvbuf(file, buf, (int16_t)(buf != 0 ? 0 : 2), 0x200);
}

/*
 * 0x0d321
 *
 * **`fwrite`'s body**: turn elements into bytes, write them, and turn the bytes
 * back into elements.
 *
 * A size of zero answers the *count* rather than zero, which is the C library's
 * answer to "write `count` things of no length" - it wrote all of them, and
 * none of them took any room. Answering zero there would look like failure to
 * every caller.
 *
 * The product is worked out as a **long**, and anything that does not fit a word
 * answers zero without writing: this cannot write 64 KB or more in one call. The
 * test is `dx > 1` then `dx < 1`, and the third branch - `or ax, ax` followed by
 * `jae` - can only go one way, because `or` clears the carry. So a high word of
 * exactly 1 also answers zero, and the instruction that looks like it is
 * deciding something is a comparison the compiler left behind.
 *
 * The answer is the bytes written divided by the size, so a partial write of the
 * last element is not counted - the caller learns that fewer elements went, not
 * that some fraction did.
 */
uint16_t borland_fwrite(const uint8_t * ptr, uint16_t size, uint16_t count,
                   struct file_rec *file)
{
    uint32_t total;

    if (size == 0)
        return count;

    total = (uint32_t)size * (uint32_t)count;
    if (total > 0xFFFF)
        return 0;

    return (uint16_t)(stream_put_run(file, (uint16_t)total, ptr) / size);
}

/*
 * 0x0d8ca
 *
 * **Put a run of bytes on a stream.** What `fwrite` narrows to, and the sink
 * Borland's `printf` hands the formatter, so the two reach the file the same
 * way. `__pascal`: the arguments are the caller's and it pops them itself.
 *
 * **Four paths, chosen by two bits of the stream's flag word at +2.**
 *
 *   0x08 - a stream that has to go a byte at a time, through `fputc`. Nothing
 *   is batched, and a `-1` from any byte abandons the rest.
 *
 *   0x40 with an empty buffer (+6 zero) - unbuffered: one `dos_write` straight
 *   to the handle. If the handle is in append mode - 0x800 in the flag table at
 *   DGROUP 0x4d06 - it seeks to the end first, because DOS does not.
 *
 *   0x40 with a buffer - the interesting one, below.
 *   
 *   neither bit - the text path, a byte at a time through `putc` when there is
 *   a buffer, and `write_text` when there is not.
 *
 * **The buffered path's counter runs negative.** `+0` is the space *left*,
 * counted up towards zero, which is why the tests are `jl` and `jge` and why a
 * first use sets it to `0xffff - size` rather than to the size. Adding the
 * count to it and finding the sum still negative means the bytes fit, and they
 * are copied to `+0xa` and the counter and pointer both advanced.
 *
 * A write **larger than the buffer** does not fill it first: the buffer is
 * flushed and the whole run handed to `dos_write` in one call, so a big write
 * costs one DOS call and not one per bufferful.
 *
 * Every failure answers **zero**, not a partial count. A short `dos_write` -
 * fewer bytes than asked for, which is a full disk - is one of them.
 *
 * One detail in the text path that a future argument comparison would trip
 * over: the original reaches `putc` with `mov al, [bx]` and then `push ax`,
 * and **`ah` is never cleared** - it still holds the high byte of the count,
 * left there by the loop test at 0x0da3b. So the word passed is
 * `(count >> 8) << 8 | c` rather than `c`. `putc` reads only `al`, so nothing
 * behaves differently and the port passes the byte alone; but a check that
 * compared the *argument* rather than the effect would see a difference that
 * is not one. Recorded because this path is unreached and therefore unverified,
 * so the next person to reach it has only this note to go on.
 */
uint16_t stream_put_run(struct file_rec *file, uint16_t count, const uint8_t * buf)
{
    uint16_t asked = count;
    int16_t  handle;

    if ((file->flags & 8) != 0) {
        while (count-- != 0) {
            uint8_t c = *buf;

            buf++;
            if (borland_fputc((int16_t)(int8_t)c, file) == -1)
                return 0;
        }
        return asked;
    }

    handle = (int16_t)((int8_t)file->fd);

    if ((file->flags & 0x40) != 0) {
        if (file->bsize == 0) {
            /* Unbuffered. */
            if ((BORLAND_HANDLE_FLAGS.flags[handle] & 0x800) != 0)
                dos_lseek(handle, 0, 0, 2);

            if ((uint16_t)dos_write(handle, buf, count) < count)
                return 0;

            return asked;
        }

        if (file->bsize < count) {
            /* Bigger than the buffer: flush, then one write for the lot. */
            if (((uint16_t)file->level) != 0 && flush_stream(file) != 0)
                return 0;

            if ((BORLAND_HANDLE_FLAGS.flags[handle] & 0x800) != 0)
                dos_lseek(handle, 0, 0, 2);

            if ((uint16_t)dos_write(handle, buf, count) < count)
                return 0;

            return asked;
        }

        if ((int16_t)(file->level + (int16_t)count) >= 0) {
            if (((uint16_t)file->level) == 0)
                file->level = (uint16_t)(0xffff - file->bsize);
            else if (flush_stream(file) != 0)
                return 0;
        }

        mem_copy(dg_ptr(dgroup, file->curp), buf, count);
        file->level = (uint16_t)(((uint16_t)file->level) + count);
        file->curp =
            (uint16_t)(file->curp + count);

        return asked;
    }

    /* The text path. */
    if (file->bsize == 0) {
        if ((uint16_t)write_text(handle, buf, count) < count)
            return 0;

        return asked;
    }

    while (count-- != 0) {
        int16_t r;

        file->level++;

        if (file->level >= 0) {
            uint8_t c = *buf;

            buf++;
            r = borland_putc((int16_t)c, file);
        } else {
            uint8_t c = *buf;

            buf++;
            *dg_ptr(dgroup, file->curp) = c;
            file->curp++;
            r = (int16_t)c;
        }

        if ((uint16_t)r == 0xffff)
            return 0;
    }

    return asked;
}

/*
 * 0x0b794
 *
 * Borland's `unlink`: INT 21h AH=41h with the path in DX, answering 0 on
 * success and the DOS error otherwise, filed at DGROUP 0x2d7b like the rest.
 * The machine writer uses it to delete a file it failed to finish, so a
 * half-written machine cannot be loaded.
 *
 * The same `xor ax,ax` before the call and after it as `chdir`, with only the
 * carry flag choosing between them.
 *
 * **It deletes from the overlay and nothing else.** A name the port never wrote
 * is not there to delete, and answering 0 for it would be claiming to have
 * removed a file that is still on the disk - so a name that is not in the
 * overlay is DOS 2, "file not found". The host copy is never touched, which is
 * also why a failed save cannot destroy the machine it was overwriting.
 */
uint16_t dos_unlink(const char *path)
{
    char name[256];
    uint16_t i;
    int16_t r;

    for (i = 0; i < sizeof name - 1 && path[i] != 0; i++)
        name[i] = path[i];
    name[i] = 0;

    r = io_dos_forget(name) ? 0 : 2;    /* DOS 2: file not found */

    BORLAND_FIND_INFO.word_2d7b = r;
    return (uint16_t)r;
}

/*
 * 0x0c293
 *
 * `tolower`. `EOF` - the argument compared against -1 as a *word* - passes
 * straight through; anything else indexes the ctype table at DGROUP 0x4ab7 and
 * adds 0x20 when bit 2, the "this is upper case" bit, is set.
 *
 * The table is the runtime's own and comes in with the image, so the port reads
 * it rather than deciding for itself what an upper-case byte is. That matters
 * above 0x7f, where the table and C's `isupper` need not agree.
 */
uint16_t to_lower(uint16_t c)
{
    if ((int16_t)c == -1)
        return 0xffff;

    if ((BORLAND_CTYPE.ctype[(uint8_t)c] & 4) != 0)
        return (uint16_t)((uint8_t)c + 0x20);

    return (uint8_t)c;
}

/*
 * NOT a transcription: where the port keeps the find result between the DOS
 * call and `dos_find_to_dgroup` reading it back.
 *
 * The three fields are also written into the **DTA itself**, at the guest
 * address DOS would use, because that block is *in guest memory* and a
 * comparison against the original sees it. It is not decoration: `verify.py`
 * reported `fill_file_listing` differing in 33 places, all of them between PSP+0x80 and
 * PSP+0xaa, because the original's `findfirst` filled the block and the port
 * filled nothing. A program that read the DTA directly would have seen the
 * same nothing.
 *
 * The game never moves the DTA - there is no INT 21h AH=1Ah anywhere in a run -
 * so the default, PSP+0x80, is where it is. The PSP is the usual 0x10
 * paragraphs below the image, so the address is `IMAGE_BASE - 0x100 + 0x80`,
 * and it is worked out from `IMAGE_BASE` rather than written down: `verify.py`
 * moves DGROUP to wherever the original had it, and a constant here would then
 * write the block 0x1080 bytes from the wrong place.
 */
#define DTA_ADDR ((uint32_t)IMAGE_BASE - 0x80u)

static uint8_t  dta_attr;
static uint32_t dta_size;
static uint8_t  dta_name[13];

/*
 * NOT a transcription: the port's own. DOS lays this block out; the original
 * program never does, so there is no address to point at.
 *
 * Lay the find result out in guest memory the way DOS lays it out: 21 bytes of
 * DOS's own search state, then the attribute at +0x15, the time and date at
 * +0x16 and +0x18, the size at +0x1a, and the name at +0x1e.
 *
 * The first 21 bytes are DOS's private business and nothing reads them; they
 * are left as they were rather than zeroed, because zeroing them would be
 * inventing a value the original does not write either. The time and date are
 * left alone for the same reason - the emulator writes a fixed pair there and
 * the game never looks.
 */
static void dta_publish(void)
{
    uint16_t i;

    guest_mem[DTA_ADDR + 0x15] = dta_attr;
    guest_mem[DTA_ADDR + 0x1a] = (uint8_t)dta_size;
    guest_mem[DTA_ADDR + 0x1b] = (uint8_t)(dta_size >> 8);
    guest_mem[DTA_ADDR + 0x1c] = (uint8_t)(dta_size >> 16);
    guest_mem[DTA_ADDR + 0x1d] = (uint8_t)(dta_size >> 24);

    for (i = 0; i < 13; i++)
        guest_mem[DTA_ADDR + 0x1e + i] = dta_name[i];
}

/*
 * 0x0b6ef
 *
 * **Copy the find result out of the DTA and into DGROUP.** It asks DOS where
 * the DTA is - AH=2Fh, answered in ES:BX - and lifts three things out of it:
 * the attribute byte at +0x15 to 0x2d76, the four size bytes at +0x1a to
 * 0x2d77, and the thirteen name bytes at +0x1e to 0x2d4a.
 *
 * The name is copied with a `loop` of exactly 0x0d, so the NUL comes with it
 * only because DOS wrote one - nothing here terminates the string.
 *
 * Both `findfirst` and `findnext` call this on the way out, *after* their own
 * epilogue and with AX pushed across it, which is why the answer survives.
 */
void dos_find_to_dgroup(void)
{
    uint16_t i;

    BORLAND_FIND_INFO.word_2d76  = dta_attr;
    BORLAND_FIND_INFO.size = dta_size;

    for (i = 0; i < 0x0d; i++)
        BORLAND_FIND_NAME.find_name[i] = (char)dta_name[i];
}

/*
 * 0x0b6b7
 *
 * Borland's `findfirst`: INT 21h AH=4Eh with the pattern in DS:DX and the
 * attribute in CX, then the DTA copied out. The answer is AL zero-extended, so
 * 0 is a match and 18 is "no more files" - the carry flag is never looked at.
 */
uint16_t dos_findfirst(const char *pattern, uint16_t attr)
{
    char name[256];
    uint16_t i;
    int16_t r;

    for (i = 0; i < sizeof name - 1 && pattern[i] != 0; i++)
        name[i] = pattern[i];
    name[i] = 0;

    /*
     * **Nothing is cleared first.** A find that fails leaves the DTA exactly as
     * it was - DOS does not touch it - and 0x0b6ef copies it out either way, so
     * the *previous* name is still there afterwards. Zeroing the buffer here
     * would publish a blank where the original publishes the last name it
     * found, which `verify.py` caught as `fill_file_listing` differing on the twelve
     * bytes of "TONSOFUN.TIM" after the listing loop ran off the end.
     */
    r = io_dos_findfirst(name, attr, dta_name, &dta_attr, &dta_size);

    dta_publish();
    dos_find_to_dgroup();
    return (uint16_t)r;
}

/*
 * 0x0b6d3
 *
 * Borland's `findnext`: INT 21h AH=4Fh, and `findfirst`'s code to the byte
 * apart from the function number. It loads DS:DX and CX from its arguments the
 * same way even though AH=4Fh reads neither - the search state is DOS's, in the
 * DTA - so the pattern it is passed is decoration.
 */
uint16_t dos_findnext(const char *pattern, uint16_t attr)
{
    int16_t r;

    (void)pattern;
    (void)attr;

    /* Nothing cleared, for the reason given in `dos_findfirst`. */
    r = io_dos_findnext(dta_name, &dta_attr, &dta_size);

    dta_publish();
    dos_find_to_dgroup();
    return (uint16_t)r;
}

/*
 * 0x0b72e
 *
 * The attribute of the entry just found, zero-extended out of the byte at
 * DGROUP 0x2d76. Three instructions and no frame - it is a field accessor that
 * happens to be far-callable.
 */
uint16_t dos_find_attr(void)
{
    return BORLAND_FIND_INFO.word_2d76;
}

/*
 * 0x0b734
 *
 * The name of the entry just found: the *address* 0x2d4a, not a copy. Two
 * instructions. Every caller reads through it before the next `findnext`
 * overwrites it.
 */
char *dos_find_name(void)
{
    return (char *)BORLAND_FIND_NAME.find_name;
}

/*
 * 0x0b738
 *
 * The size of the entry just found, as a long in DX:AX out of the long at
 * 0x2d77.
 */
uint32_t dos_find_size(void)
{
    return BORLAND_FIND_INFO.size;
}

/*
 * 0x0b755
 *
 * Borland's `chdir`: INT 21h AH=3Bh with the path in DX, answering 0 on success
 * and the DOS error code otherwise, and filing that same value at DGROUP
 * 0x2d7b - which is `errno`.
 *
 * **`ax` is zeroed before the call and again after it**, and only the carry
 * flag decides which zero survives. So a DOS that leaves rubbish in `ax` on
 * success cannot make this look like a failure, and `errno` is cleared by a
 * successful call rather than merely left alone.
 *
 * The change itself is `io_dos_chdir`, which is the port's own and models what
 * the emulator does: a directory inside the game's, with the game's directory
 * as a **floor** rather than a starting point, so a guest that walks up with
 * `..` cannot walk out.
 */
uint16_t dos_chdir(const char *path)
{
    char name[256];
    uint16_t i;
    int16_t r;

    for (i = 0; i < sizeof name - 1 && path[i] != 0; i++)
        name[i] = path[i];
    name[i] = 0;

    r = io_dos_chdir(name);

    BORLAND_FIND_INFO.word_2d7b = r;
    return (uint16_t)r;
}

/*
 * 0x0b819
 *
 * Borland's `setdisk`: INT 21h AH=0Eh, with the drive taken from a *letter* -
 * `and al, 0x5f` uppercases it and `sub al, 0x41` makes it the number DOS
 * wants, so 'a' and 'A' are both drive zero.
 *
 * The mask is 0x5f and not 0xdf, so it also clears bit 5 **and bit 7**: a
 * letter with the high bit set still lands on a drive rather than on a number
 * over 0x80. It answers nothing - DOS returns the drive count in `al` and this
 * throws it away.
 *
 * The port serves one directory and therefore one drive, so `io_dos_setdisk`
 * changes nothing. That is not a stub: selecting the only drive there is *is*
 * a no-op, and the game is never told otherwise because this answers nothing.
 *
 * **The reference agrees by not implementing it at all** - the emulator logs
 * `UNHANDLED INT 21h AH=0eh` every time the game gets here and carries on. So
 * a no-op is not the port settling for less than the reference does; it is the
 * same behaviour reached from the other direction.
 */
void dos_setdisk(uint16_t letter)
{
    io_dos_setdisk((uint8_t)(((letter & 0x5f) - 0x41) & 0xff));
}

/*
 * 0x0b7b3
 *
 * `getcurdir`-style: write the current drive and directory into the caller's
 * buffer as `X:\\` followed by the path.
 *
 * The drive letter comes from INT 21h AH=19h plus 0x41, so drive 0 is `A`. The
 * path is then asked for with AH=47h **for drive 0** - `dl` is zeroed, which
 * DOS reads as "the current drive" - and written straight after the backslash,
 * without its own leading one, which is why the backslash is put there first.
 *
 * Nothing checks whether either call failed.
 */
void dos_get_cur_dir(char *buf)
{
    buf[0] = (char)(io_dos_curdrive() + 0x41);
    buf[1] = ':';
    buf[2] = '\\';

    /*
         * The cast goes through `uintptr_t` because DGROUP is volatile - the
         * timer handler runs on a thread and shares it - and this one place
         * hands a pointer *into* it to the IO layer, which fills the buffer
         * itself. Nothing else does that, and the volatility is the port's
         * memory model rather than anything the original had.
         */
        io_dos_getcwd((uint8_t *)(buf + 3));
}

/*
 * ===========================================================================
 * **The rest of the runtime in segment 0000**: the exit path, `atexit`, the
 * temporary-file name, `eof`, `flushall`, and the `printf` family. Every one
 * is a libc function, so its behaviour is pinned twice - by Borland's code
 * and by the standard - and the host's libc is a second oracle for the
 * transcriptions where one is wanted. Almost none of them is reached by the
 * game as shipped: the sweep says which, and each comment says who calls it.
 * ===========================================================================
 */

/*
 * 0x0bbfe
 *
 * `atexit`: file a far pointer in the table at DGROUP 0x6438, thirty-two deep,
 * counted at 0x4ab4 - answering 1 when the table is full and 0 otherwise. The
 * segment lands at +2 and the offset at +0, which is `struct far_ptr`'s own
 * layout. Nothing in the image calls it: the count is 0 in the image and
 * nothing ever raises it, so `borland_exit_common`'s chain never runs a
 * handler. Not that the walk is skipped - it is walked, and finds nothing.
 */
int16_t borland_atexit(struct far_ptr fn)
{
    if (BORLAND_ATEXIT_COUNT.atexit_count == 0x20)
        return 1;

    BORLAND_ATEXIT_TABLE.atexit[BORLAND_ATEXIT_COUNT.atexit_count] = fn;
    BORLAND_ATEXIT_COUNT.atexit_count++;
    return 0;
}

/*
 * 0x0c006
 *
 * `__IOerror`'s other face: file the DOS error through `io_error`, then
 * answer the **code** rather than -1. The two attribute wrappers below use it,
 * and `tmp_name_unused` reads the non-zero answer as "the name is free".
 */
int16_t io_error_code(int16_t code)
{
    io_error(code);
    return code;
}

/*
 * 0x0bc2b
 *
 * `_dos_getfileattr`: INT 21h AX=4300h, the attribute word into `*attr` and
 * 0 answered; on failure the DOS code goes through `io_error_code` and comes
 * back as the answer. The port's filesystem answers through `io_dos_getattr`,
 * which is the attribute or -1, and -1 is DOS 2, "file not found" - the only
 * failure a read-only tree can produce for a name.
 *
 * Its one caller is `tmp_name_unused`, which nothing calls.
 */
int16_t dos_get_file_attr(const char *name, uint16_t *attr)
{
    int16_t r = io_dos_getattr(name);

    if (r < 0)
        return io_error_code(2);

    *attr = (uint16_t)r;
    return 0;
}

/*
 * 0x0bc48
 *
 * `_dos_setfileattr`: INT 21h AX=4301h with the new attribute in CX, 0 on
 * success and the DOS code otherwise. Nothing calls it. The port's tree is
 * read-only and `io_dos_setattr` says so with DOS 5, "access denied".
 */
int16_t dos_set_file_attr(const char *name, uint16_t attr)
{
    int16_t r = io_dos_setattr(name, attr);

    if (r != 0)
        return io_error_code(r);

    return 0;
}

/*
 * 0x0bc63
 *
 * One `retf`: what the three exit vectors at DGROUP 0x4bb8, 0x4bbc and 0x4bc0
 * point at until a stdio module plants its own. `borland_setvbuf` puts
 * `exit_flush_streams` in the first and `borland_fopen` puts
 * `exit_close_streams` in the second; the third is never replaced.
 */
void exit_hook_none(void)
{
}

/*
 * Call one of the three exit vectors, or an `atexit` entry.
 *
 * NOT a transcription: the original does `lcall [0x4bb8]` and the port cannot
 * call through a guest far pointer. The offsets are the three the runtime
 * ever files - see `exit_hook_none` - and an `atexit` handler would be a
 * fourth, which nothing registers.
 */
static void call_exit_hook(struct far_ptr h)
{
    switch (h.off) {
    case 0xbc63:
        exit_hook_none();
        return;
    case 0xdfdc:
        exit_flush_streams();
        return;
    case 0xdfb4:
        exit_close_streams();
        return;
    default:
        break;
    }

    {
        static char what[64];

        io_format(what, sizeof what, "an exit hook at %04x:%04x", h.seg, h.off);
        not_transcribed(what);
    }
}

/*
 * 0x0bc64
 *
 * Borland's `__exit(status, dontexit, quick)`, which `exit`, `_exit` and
 * `_cexit` all reach with different flags. With `quick` clear it runs the
 * `atexit` chain backwards - decrementing the count and calling each entry -
 * and then the first exit vector, the stream flush. Then the startup's two
 * checks: 0x0160 compares the null-pointer canary and 0x0173 sums the first
 * 0x2f bytes of DGROUP against what it was at load, to print "Null pointer
 * assignment"; and 0x01f0 puts back the four interrupt vectors the startup
 * took. With `dontexit` clear it then runs the second and third vectors -
 * closing the streams - unless `quick`, and leaves through 0x019b, which is
 * INT 21h AH=4Ch with the status.
 *
 * The four startup pieces are `main.c`'s here, the loader's half of the
 * program: no canary is planted, so none is checked, and the vectors are the
 * machine's. The terminate is the host's `exit`, which is the one place the
 * port is allowed to be a different program. `ret 6` - near, and it cleans
 * its own three words.
 */
void borland_exit_common(int16_t status, int16_t dontexit, int16_t quick)
{
    if (quick == 0) {
        while (BORLAND_ATEXIT_COUNT.atexit_count != 0) {
            BORLAND_ATEXIT_COUNT.atexit_count--;
            call_exit_hook(BORLAND_ATEXIT_TABLE.atexit[BORLAND_ATEXIT_COUNT.atexit_count]);
        }
        /* 0x0160: the null-pointer canary check. Nothing to check here. */
        call_exit_hook(BORLAND_EXIT_VECTORS.exit_buf);
    }

    /* 0x01f0: restore the vectors the startup took; 0x0173: the null-pointer
       assignment check. Both are the loader's, and the loader is main.c. */

    if (dontexit != 0)
        return;

    if (quick == 0) {
        call_exit_hook(BORLAND_EXIT_VECTORS.exit_fopen);
        call_exit_hook(BORLAND_EXIT_VECTORS.exit_open);
    }

    /* 0x019b: INT 21h AH=4Ch. */
    exit(status);
}

/*
 * 0x0c0a6
 *
 * The number for a temporary-file name: `long_to_string` in decimal, unsigned,
 * lower case, of a 16-bit value with a zero high word. `ret 4`, near, and the
 * two words are the buffer and the number. Called only from `tmp_name_build`.
 */
char *tmp_number(char *buf, uint16_t number)
{
    return long_to_string('a', 0, 10, buf, number, 0);
}

/*
 * 0x0c79b
 *
 * `stpcpy`: copy `src` and its terminator with `mem_copy`, and answer the
 * address of the terminator rather than the start. `tmp_name_build` uses the
 * answer as the place to write the number.
 */
char *string_copy_end(char *dst, const char *src)
{
    uint16_t n = string_length(src);

    mem_copy((uint8_t *)dst, (const uint8_t *)src, (uint16_t)(n + 1));
    return dst + n;
}

/*
 * 0x0c0ec
 *
 * Build a temporary-file name: the prefix - "TMP" at DGROUP 0x4d90 when none
 * is given - then the number in decimal, then ".$$$". A null buffer means the
 * static one at 0x64b8. `ret 6`, near.
 *
 * Two callers: `tmp_name_unused`, and `borland_fclose` rebuilding the name of
 * a temporary stream so it can unlink it.
 */
char *tmp_name_build(uint16_t number, const char *prefix, char *buf)
{
    if (buf == NULL)
        buf = BORLAND_ATEXIT_TABLE.tmp_name;
    if (prefix == NULL)
        prefix = BORLAND_RUNTIME_STRINGS.tmp_prefix;

    tmp_number(string_copy_end(buf, prefix), number);
    string_concat(buf, BORLAND_RUNTIME_STRINGS.tmp_suffix);
    return buf;
}

/*
 * 0x0c12b
 *
 * The heart of `tmpnam`: step the counter - by two past -1, so the name never
 * carries 0xffff - build the name, and ask DOS for its attributes; a name
 * that answers an error is one no file has, and is the answer. `ret 4`, near,
 * over the counter's address and the buffer. Nothing in the image calls it:
 * `tmpnam` and `tmpfile` were linked in and never used.
 */
char *tmp_name_unused(int16_t *counter, char *buf)
{
    uint16_t attr;
    char *name;

    do {
        *counter = (int16_t)(*counter + (*counter == -1 ? 2 : 1));
        name = tmp_name_build((uint16_t)*counter, NULL, buf);
    } while (dos_get_file_attr(name, &attr) == 0);

    return name;
}

/*
 * 0x0c2bf
 *
 * The library's `unlink`: INT 21h AH=41h, 0 on success and `io_error` on the
 * carry. The game has its own at 0x0b794, `dos_unlink`, with the same shape
 * and a flag of its own to set; this one is only reached from
 * `borland_fclose` unlinking a temporary. The port deletes from its write
 * overlay and nothing else, as `dos_unlink` explains, and a name that is not
 * there is DOS 2.
 */
int16_t borland_unlink(const char *name)
{
    if (io_dos_forget(name))
        return 0;

    return io_error(2);
}

/*
 * 0x0c884
 *
 * What the float-format vector at DGROUP 0x4e40 points at when no floating
 * point formats are linked, which is this program: "print" or "scanf" and
 * then " : floating point formats not linked", five and 0x27 bytes to
 * handle 2, and a jump into the startup's abort at 0x027c - which writes
 * "Abnormal program termination" and leaves with status 3. `scanf` is the
 * entry at 0x0c889; the game has no `scanf`, so only the first is reachable,
 * and only from a `%e`, `%f` or `%g` that nothing in the image writes.
 *
 * The abort is the loader's here, and the loader is main.c: its message is
 * not reproduced, its status is.
 */
void float_formats_missing(int16_t from_scanf)
{
    io_dos_write(2, (const uint8_t *)(from_scanf ? BORLAND_RUNTIME_STRINGS.s_scanf
                                                 : BORLAND_RUNTIME_STRINGS.s_print), 5);
    io_dos_write(2, (const uint8_t *)BORLAND_RUNTIME_STRINGS.s_no_floats, 0x27);
    exit(3);
}

/*
 * 0x0cd9e
 *
 * `eof(handle)`: 1 at the end, 0 before it, -1 for a bad handle.
 *
 * A handle at or past `_nfile` is EBADF through `io_error`. One whose flag
 * word carries 0x200 - Borland's "at end" mark - answers 1 without asking. A
 * device, by INT 21h AX=4400h bit 7, answers 0: a console has no end. A file
 * answers by seeking: the current position, then the end, then back to where
 * it was, and 1 when the position is at or past the end. Every seek that
 * fails goes through `io_error` with the DOS code.
 *
 * Its one caller is `borland_fgetc`'s unbuffered path.
 */
int16_t borland_eof(int16_t handle)
{
    int32_t cur, end;

    if ((uint16_t)handle >= BORLAND_NFILE.word_4d04)
        return io_error(6);

    if ((BORLAND_HANDLE_FLAGS.flags[handle] & 0x200) != 0)
        return 1;

    if ((io_dos_devinfo(handle) & 0x80) != 0)
        return 0;

    cur = io_dos_lseek(handle, 0, 1);
    if (cur < 0)
        return io_error((int16_t)cur);
    end = io_dos_lseek(handle, 0, 2);
    if (end < 0)
        return io_error((int16_t)end);
    if (io_dos_lseek(handle, cur, 0) < 0)
        return io_error(6);

    return (uint32_t)cur >= (uint32_t)end ? 1 : 0;
}

/*
 * 0x0cf13
 *
 * `flushall`: every one of the `_nfile` streams from DGROUP 0x4bc4 whose
 * flags carry either open bit is flushed through `flush_stream`, and the
 * answer is how many were. `flush_stream` itself reaches it for a null
 * stream, which is `fflush(NULL)`.
 */
int16_t borland_flushall(void)
{
    int16_t  count = 0;
    uint16_t si = dg_off(dgroup, &BORLAND_STREAMS.streams[0]);
    uint16_t n;

    for (n = BORLAND_NFILE.word_4d04; n != 0; n--) {
        if ((FILEREC_PTR(si)->flags & 3) != 0) {
            flush_stream(FILEREC_PTR(si));
            count++;
        }
        si = (uint16_t)(si + 0x10);
    }

    return count;
}

/*
 * 0x0d4b3
 *
 * `getchar`: `fgetc` on the first stream. Nothing calls it.
 */
int16_t borland_getchar(void)
{
    return borland_fgetc(&BORLAND_STREAMS.streams[0]);
}

/*
 * The `printf` engine's state: the 80-byte staging buffer at `[bp-0x96]`,
 * the cursor `di` walks through it, the room left in it at `[bp-0x14]`, the
 * running total at `[bp-0x12]` and the failure mark at `[bp-0x16]`. The
 * original keeps these in `vprinter`'s frame and its two helpers reach them
 * through BP; C hands the helpers a pointer to the same thing.
 */
/* `struct printer` is declared in tim.h beside `putn_fn`. */

/*
 * 0x0c31d
 *
 * Hand the staging buffer to the putter and start it again. A putter that
 * answers 0 marks the whole call failed; the total is what the buffer held.
 */
void printer_flush(struct printer *p)
{
    uint16_t n = (uint16_t)(p->cur - p->out);

    if (p->put(p->sink, n, (const uint8_t *)p->out) == 0)
        p->failed = 1;

    p->room = 0x50;
    p->total = (uint16_t)(p->total + n);
    p->cur = p->out;
}

/*
 * 0x0c314
 *
 * One character into the buffer, and a flush when it fills.
 */
void printer_put(struct printer *p, char c)
{
    *p->cur++ = c;
    if (--p->room == 0)
        printer_flush(p);
}

/*
 * 0x0c307
 *
 * The engine's own `strlen` - `repne scasb` on ES:DI - over whichever string
 * it is about to copy out.
 */
uint16_t printer_len(const char *s)
{
    uint16_t n = 0;

    while (s[n] != 0)
        n++;
    return n;
}

/*
 * 0x0c2d5
 *
 * A word as four upper-case hex digits, high byte first: `aam 0x10` splits
 * each byte and `add 0x90 / daa / adc 0x40 / daa` is the classic nibble to
 * ASCII without a table. `%p` is its only user.
 */
char *hex_word(char *dst, uint16_t v)
{
    static const char digits[] = "0123456789ABCDEF";

    *dst++ = digits[(v >> 12) & 0xf];
    *dst++ = digits[(v >> 8) & 0xf];
    *dst++ = digits[(v >> 4) & 0xf];
    *dst++ = digits[v & 0xf];
    return dst;
}

/*
 * 0x0c2ed
 *
 * **Borland's `__vprinter`**, the engine under `printf`, `sprintf` and
 * `vsprintf`: 1150 bytes of hand-written assembly, transcribed as the state
 * machine it is. `ret 8`, near, over four words: the putter, its sink, the
 * format and the address of the arguments.
 *
 * Text outside a conversion is copied into the staging buffer; the buffer
 * goes to the putter when it fills and once more at the end. A `%` starts a
 * conversion, parsed one character at a time through the class table at
 * DGROUP 0x4da1 - one byte per character from ' ' to DEL, 0x14 meaning
 * "not part of a conversion" - and a jump table of twenty-four cases at
 * 0x0c76b. The parser's state is `ch`: 0 before anything, 1 after a leading
 * `0`, 2 in the width, 3 after a `*` width, 4 in the precision, 5 after a
 * size letter. The flags word at `[bp-2]` collects `#` (1), `-` (2), "the
 * value was not zero" (4), `0` (8), `l` (0x10), `F` (0x20), "put 0x in
 * front" (0x40) and `L` (0x100). Width and precision start at -1.
 *
 * A number is built by `long_to_string` into the 65-byte `num` at
 * `[bp-0x46]`, with a byte spare in front for the sign; a `%p` is built there
 * by hand as SSSS:OOOO or OOOO in upper case; `%c` and `%s` point at their
 * one byte or their string, `(null)` at DGROUP 0x4d9a standing in for a null
 * `%s`. All of them then go out through one path at 0x0c642: leading spaces
 * to the width unless `-`, the `0x` prefix, the sign ahead of any zeros,
 * the zeros, the digits, and trailing spaces for `-`.
 *
 * The quirks are the original's and are kept. `%u` reaches the number path
 * without clearing the sign character, so `%+u` prints the plus that `%+o`
 * and `%+x` drop. A precision on `%s` is compared unsigned, so -1 never
 * clamps. A conversion character the table does not know - class 0x14, or
 * an index past the table - puts `%` and then **every remaining character of
 * the format** verbatim, and stops. And a `%e`, `%f` or `%g` goes through
 * the vector at DGROUP 0x4e40, which in this program is
 * `float_formats_missing`.
 *
 * The arguments are read as guest words at `args`, which is where the
 * original's `[bp+4]` points - into the caller's stack. The port's own
 * `printf` has no such stack, and passes null; a format that then asks for
 * an argument is the one thing the port cannot answer, and says so.
 */
int16_t vprinter(putn_fn put, void *sink, const char *fmt, const uint8_t *args)
{
    struct printer p;
    char     num[0x41];          /* [bp-0x46]: one spare byte, then 64 */
    const char *s = fmt;         /* si */

    p.put = put;
    p.sink = sink;
    p.cur = p.out;
    p.room = 0x50;
    p.total = 0;
    p.failed = 0;

    for (;;) {
        uint8_t  c;
        const char *spec;        /* [bp-0x10]: just past the '%' */
        uint16_t flags;          /* [bp-2] */
        int16_t  width, prec;    /* [bp-8], [bp-0xa] */
        int16_t  zeros;          /* [bp-0xe] */
        char     sign;           /* [bp-0xb] */
        char     conv;           /* [bp-5] */
        uint8_t  is_signed;      /* [bp-6] */
        uint8_t  ch;             /* the parser's state */
        char    *text;           /* es:si at 0x0c642: what goes out */
        uint16_t len;            /* cx: how much of it */
        uint16_t radix;          /* bh */
        uint16_t letters;        /* bl, for `long_to_string` */
        int16_t  done;

        /* 0x0c35c: plain text, straight into the buffer. */
        c = (uint8_t)*s++;
        if (c == 0)
            break;
        if (c != '%') {
            printer_put(&p, (char)c);
            continue;
        }

        spec = s;
        c = (uint8_t)*s++;
        if (c == '%') {
            printer_put(&p, '%');
            continue;
        }

        flags = 0;
        zeros = 0;
        sign = 0;
        width = -1;
        prec = -1;
        ch = 0;
        radix = 10;
        letters = 'a';
        is_signed = 0;
        conv = 0;
        text = NULL;
        len = 0;
        done = 0;

        /* 0x0c398: one character of the conversion at a time. */
        for (;;) {
            uint8_t cls = (uint8_t)(c - 0x20) < 0x60
                          ? BORLAND_RUNTIME_STRINGS.fmt_class[(uint8_t)(c - 0x20)] : 0x14;
            int16_t bad = 0;

            if (cls > 0x17)
                bad = 1;
            else switch (cls) {
            case 0x00:                          /* ' ' and '+' */
                if (ch != 0)
                    bad = 1;
                else if (sign != '+')
                    sign = (char)c;
                break;
            case 0x01:                          /* '#' */
                if (ch != 0)
                    bad = 1;
                else
                    flags |= 1;
                break;
            case 0x02: {                        /* '*' */
                int16_t v = (int16_t)*(const uint16_t *)args;

                args += 2;
                if (ch < 2) {
                    if (v < 0) {
                        v = (int16_t)-v;
                        flags |= 2;
                    }
                    width = v;
                    ch = 3;
                } else if (ch == 4) {
                    prec = v;
                    ch = 5;
                } else {
                    bad = 1;
                }
                break;
            }
            case 0x03:                          /* '-' */
                if (ch != 0)
                    bad = 1;
                else
                    flags |= 2;
                break;
            case 0x04:                          /* '.' */
                if (ch >= 4)
                    bad = 1;
                else {
                    ch = 4;
                    prec++;
                }
                break;
            case 0x09:                          /* '0' */
                if (ch == 0) {
                    if ((flags & 2) == 0) {
                        flags |= 8;
                        ch = 1;
                    }
                    break;
                }
                /* fall through - a zero after the first is a digit */
            case 0x05: {                        /* '1'..'9' */
                int16_t d = (int16_t)(c - '0');

                if (ch <= 2) {
                    int16_t old = width;

                    ch = 2;
                    width = d;
                    if (old >= 0)
                        width = (int16_t)(width + old * 10);
                } else if (ch == 4) {
                    int16_t old = prec;

                    prec = d;
                    if (old != 0)
                        prec = (int16_t)(prec + old * 10);
                } else {
                    bad = 1;
                }
                break;
            }
            case 0x06:                          /* 'l' */
                flags |= 0x10;
                ch = 5;
                break;
            case 0x07:                          /* 'L' */
                flags = (uint16_t)((flags | 0x100) & ~0x10u);
                ch = 5;
                break;
            case 0x08:                          /* 'h' */
                flags &= (uint16_t)~0x10u;
                ch = 5;
                break;
            case 0x16:                          /* 'N' */
                flags &= (uint16_t)~0x20u;
                ch = 5;
                break;
            case 0x17:                          /* 'F' */
                flags |= 0x20;
                ch = 5;
                break;

            case 0x0a:                          /* 'd', 'i' */
                is_signed = 1;
                radix = 10;
                done = 1;
                break;
            case 0x0b:                          /* 'o' */
                radix = 8;
                sign = 0;
                done = 1;
                break;
            case 0x0c:                          /* 'u': the sign survives */
                radix = 10;
                done = 1;
                break;
            case 0x0d:                          /* 'x', 'X' */
                radix = 16;
                letters = (uint16_t)(uint8_t)(0xe9 + c);
                sign = 0;
                done = 1;
                break;

            case 0x0e: {                        /* 'p' */
                uint16_t off, seg = 0;
                char *d = num + 1;

                /* The original builds this at `[bp-0x46]`, the spare byte
                   itself, so a `%+p` would put its sign one byte below the
                   number - over the end of the staging buffer. Built one
                   byte along here, which only a `%+p` or `% p` could tell,
                   and no format in the image has one. */
                conv = (char)c;
                off = *(const uint16_t *)args;
                args += 2;
                if ((flags & 0x20) != 0) {
                    seg = *(const uint16_t *)args;
                    args += 2;
                    d = hex_word(d, seg);
                    *d++ = ':';
                }
                d = hex_word(d, off);
                *d = 0;
                is_signed = 0;
                flags &= (uint16_t)~4u;
                text = num + 1;
                len = (uint16_t)(d - (num + 1));
                /* 0x0c554: `dx = max(prec, len)` - and then the path it
                   jumps to reloads dx from the width. No effect; kept out. */
                done = 2;
                break;
            }
            case 0x0f: {                        /* 'e', 'f', 'g', 'E', 'G' */
                /* The vector at DGROUP 0x4e40: what is linked answers, and
                   in this program that is the stub. It never returns. */
                (void)prec;
                float_formats_missing(0);
                break;
            }
            case 0x10:                          /* 'c' */
                conv = (char)c;
                num[1] = (char)*(const uint16_t *)args;
                num[2] = 0;
                args += 2;
                text = num + 1;
                len = 1;
                done = 3;
                break;
            case 0x11: {                        /* 's' */
                const char *str;

                conv = (char)c;
                if ((flags & 0x20) == 0) {
                    uint16_t off = *(const uint16_t *)args;

                    args += 2;
                    str = off != 0 ? (const char *)dg_ptr(dgroup, off)
                                   : NULL;
                } else {
                    uint16_t off = *(const uint16_t *)args;
                    uint16_t seg = *(const uint16_t *)(args + 2);

                    args += 4;
                    str = (seg | off) != 0 ? (const char *)MK_FP(seg, off)
                                           : NULL;
                }
                if (str == NULL)
                    str = BORLAND_RUNTIME_STRINGS.null_str;
                text = (char *)str;
                len = printer_len(str);
                if (len > (uint16_t)prec)      /* unsigned: -1 never clamps */
                    len = (uint16_t)prec;
                done = 3;
                break;
            }
            case 0x12: {                        /* 'n' */
                uint16_t *at;
                uint16_t off = *(const uint16_t *)args;

                if ((flags & 0x20) == 0) {
                    args += 2;
                    at = (uint16_t *)dg_ptr(dgroup, off);
                } else {
                    uint16_t seg = *(const uint16_t *)(args + 2);

                    args += 4;
                    at = (uint16_t *)MK_FP(seg, off);
                }
                *at = (uint16_t)((0x50 - p.room) + p.total);
                if ((flags & 0x10) != 0)
                    at[1] = 0;
                done = 4;                       /* nothing to print */
                break;
            }
            default:                            /* 0x13, 0x14, 0x15 */
                bad = 1;
                break;
            }

            if (bad) {
                /* 0x0c73b: `%`, then the rest of the format, verbatim. */
                const char *r = spec;

                printer_put(&p, '%');
                while ((c = (uint8_t)*r++) != 0)
                    printer_put(&p, (char)c);
                s = r - 1;                      /* at the NUL: the loop ends */
                done = 5;
                break;
            }
            if (done)
                break;
            c = (uint8_t)*s++;
        }

        if (done == 4)
            continue;
        if (done == 5)
            continue;

        if (done == 1) {
            /* 0x0c4c6: the value, one word or two, then `long_to_string`. */
            uint16_t lo = *(const uint16_t *)args;
            uint16_t hi;

            conv = (char)c;
            args += 2;
            hi = is_signed ? (uint16_t)(((int16_t)lo < 0) ? 0xffff : 0) : 0;
            if ((flags & 0x10) != 0) {
                hi = *(const uint16_t *)args;
                args += 2;
            }
            if ((lo | hi) == 0 && prec == 0)
                continue;                       /* 0x0c4eb: nothing at all */
            if ((lo | hi) != 0)
                flags |= 4;
            long_to_string(letters, is_signed, radix, num + 1, lo, hi);
            text = num + 1;
        }

        if (done == 1 || done == 2) {
            /* 0x0c5ff / 0x0c60d: how many zeros go in front. */
            if (prec >= 0) {
                int16_t n;

                len = printer_len(text);
                n = (int16_t)len;
                if (text[0] == '-')
                    n--;
                if ((int16_t)(prec - n) > 0)
                    zeros = (int16_t)(prec - n);
            } else if ((flags & 8) != 0 && width > 0) {
                int16_t n;

                len = printer_len(text);
                n = (int16_t)len;
                if (text[0] == '-')
                    n--;
                if ((int16_t)(width - n) > 0)
                    zeros = (int16_t)(width - n);
            }
            /* 0x0c61e: the sign goes into the spare byte in front, and a
               sign - this one or a minus the number brought - takes one of
               the zeros back when the zeros came from the width. */
            if (text[0] == '-' || sign != 0) {
                if (text[0] != '-')
                    *--text = sign;
                if (zeros > 0 && prec < 0)
                    zeros--;
            }
            len = printer_len(text);
        }

        /* 0x0c642: everything goes out this way. */
        {
            int16_t  bx = width;
            uint16_t cx = len;

            if ((flags & 5) == 5) {
                if (conv == 'o') {
                    if (zeros <= 0)
                        zeros = 1;
                } else if (conv == 'x' || conv == 'X') {
                    flags |= 0x40;
                    bx = (int16_t)(bx - 2);
                    zeros = (int16_t)(zeros - 2);
                    if (zeros < 0)
                        zeros = 0;
                }
            }
            cx = (uint16_t)(cx + zeros);

            if ((flags & 2) == 0)
                for (; bx > (int16_t)cx; bx--)
                    printer_put(&p, ' ');

            if ((flags & 0x40) != 0) {
                printer_put(&p, '0');
                printer_put(&p, conv);
            }

            if (zeros > 0) {
                cx = (uint16_t)(cx - zeros);
                bx = (int16_t)(bx - zeros);
                if (text[0] == '-' || text[0] == ' ' || text[0] == '+') {
                    printer_put(&p, *text++);
                    cx--;
                    bx--;
                }
                for (; zeros != 0; zeros--)
                    printer_put(&p, '0');
            }

            if (cx != 0) {
                bx = (int16_t)(bx - cx);
                for (; cx != 0; cx--)
                    printer_put(&p, *text++);
            }

            for (; bx > 0; bx--)
                printer_put(&p, ' ');
        }
    }

    /* 0x0c74b */
    if (p.room < 0x50)
        printer_flush(&p);

    return p.failed ? -1 : (int16_t)p.total;
}

/*
 * The file putter, as `vprinter` wants it typed.
 *
 * NOT a transcription: the original passes 0x0d8ca itself, and C wants one
 * parameter type for every putter it can be handed.
 */
static uint16_t file_putn(void *sink, uint16_t n, const uint8_t *buf)
{
    return stream_put_run((struct file_rec *)sink, n, buf);
}

/*
 * 0x0dc34
 *
 * The string putter: copy the run to where the cursor points, advance the
 * cursor by that much and put a terminator after it. `ret 6`, near. The
 * cursor is the caller's own `buf` slot, which is how `sprintf` answers into
 * the buffer it was given.
 */
uint16_t string_putn(void *sink, uint16_t n, const uint8_t *buf)
{
    char **cursor = (char **)sink;

    mem_copy((uint8_t *)*cursor, buf, n);
    *cursor += n;
    **cursor = 0;
    return n;
}

/*
 * 0x0dc5c
 *
 * `sprintf`: an empty string into the buffer first, then the engine with the
 * string putter, the address of the buffer argument as its sink, and the
 * arguments' address - `lea ax,[bp+0xa]`, the caller's stack past the format.
 * The port takes that address as a parameter, because it has no such stack.
 * Nothing in the image calls it.
 */
int16_t borland_sprintf(char *buf, const char *fmt, const uint8_t *args)
{
    char *cursor = buf;

    *buf = 0;
    return vprinter(string_putn, &cursor, fmt, args);
}

/*
 * 0x0dc79
 *
 * `vsprintf`: the same, with the arguments' address handed in as the third
 * word rather than taken from the stack. Its one caller is the debugging
 * printer at 0x0b907, which writes to the monochrome adapter and which
 * nothing calls.
 */
int16_t borland_vsprintf(char *buf, const char *fmt, const uint8_t *args)
{
    char *cursor = buf;

    *buf = 0;
    return vprinter(string_putn, &cursor, fmt, args);
}

/*
 * 0x0dfb4
 *
 * `_exitfopen`, the second exit vector once `borland_fopen` has planted it:
 * `fclose` on every stream whose flags carry either open bit, all `_nfile`
 * of them.
 */
void exit_close_streams(void)
{
    uint16_t i;

    for (i = 0; i < BORLAND_NFILE.word_4d04; i++)
        if ((BORLAND_STREAMS.streams[i].flags & 3) != 0)
            borland_fclose(&BORLAND_STREAMS.streams[i]);
}

/*
 * 0x0dfdc
 *
 * `_exitbuf`, the first exit vector once `borland_setvbuf` has planted it:
 * `flush_stream` on the first **four** streams that are open - the ones the
 * startup opened, `stdin` to `stdaux` - and no more, which is the constant 4
 * in the original rather than `_nfile`.
 */
void exit_flush_streams(void)
{
    uint16_t i;

    for (i = 0; i < 4; i++)
        if ((BORLAND_STREAMS.streams[i].flags & 3) != 0)
            flush_stream(&BORLAND_STREAMS.streams[i]);
}
