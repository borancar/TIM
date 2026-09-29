/*
 * Borland C++ 3.0's run-time library, the parts of it TIM.EXE links, as
 * transcribed from the image - **not built**. The port calls libc; these are
 * kept to be taken out for other projects. What follows are the declarations
 * that left tim.h and dgroup.h when the port stopped linking them. The files
 * still include the port's headers and were last compiled against the tree
 * of 2026-09-28; they need those headers' DGROUP macros to build again.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#ifndef BORLAND_H
#define BORLAND_H

/*
 * ---------------------------------------------------------------------------
 * **A `FILE`**, the Borland stream structure - twenty of which the runtime
 * keeps, and which `flush_all` walks.
 *
 * The fields are the ones `buffered_read`'s own note lists: +0 the bytes left
 * in the buffer, +2 the flags, +4 the DOS handle, +6 the buffer size, +0xa the
 * read pointer. The handle is a *byte* and is read signed in three places,
 * which is what makes -1 mean "no handle".
 * ---------------------------------------------------------------------------
 */
/* **Borland's `FILE`**, sixteen bytes, with Borland's own field names:
   `level` is the bytes still buffered, negative while writing; `fd` the DOS
   handle, read signed for -1; `hold` the one-byte buffer an unbuffered stream
   reads into; `buffer` and `curp` the buffer and the cursor into it as DGROUP
   offsets; `token` the record's own offset, which `fclose` and
   `borland_setvbuf` check before believing the pointer. The stream routines
   in borland_file.c take a pointer to one and nothing outside them looks
   inside; a routine that files a stream in DGROUP keeps `dg_near` of it and
   gets it back with `FILEREC_PTR`. */
struct file_rec {
    int16_t   level;           /* +0x00  bytes still in the buffer */
    uint16_t  flags;           /* +0x02  0x40 is the one buffered_read tests */
    uint8_t   fd;              /* +0x04  the DOS handle, read signed for -1 */
    uint8_t   hold;            /* +0x05  the unbuffered stream's one byte */
    uint16_t  bsize;           /* +0x06 */
    dg_near_t buffer_ptr;      /* +0x08 */
    dg_near_t curp_ptr;        /* +0x0a  where the next byte comes from */
    uint16_t  istemp;          /* +0x0c */
    dg_near_t token_ptr;       /* +0x0e  the record's own offset, filed by setup_streams */
} PACKED;

/* A stream offset of 0 is NULL - the failed open, which every caller tests. */
#ifndef __TURBOC__
#  define FILEREC_PTR(p)       ((FILE *)(p))
#endif

/* **`FILE` is Borland's, in the game.** The game's translation units include
   no host <stdio.h> - what they need of the host's console goes through io.h -
   so the standard name is free, and the game's file routines are written
   against it: a routine that opens, reads, seeks or closes takes and answers
   `FILE *`, and another stdio could be put under the name. A *host* unit -
   io.c, sdl.c, the dev*.c files, the hybrid - needs the host's <stdio.h> and
   defines `TIM_HOST`, so this typedef is skipped there; those units read the
   game's prototypes with the host's `FILE`, which is a pointer either way, and
   none of them hands the game one. */
#ifdef TIM_HOST
#include <stdio.h>          /* the host's FILE, for the host's own units */
#else
typedef struct file_rec FILE;
#endif

/* Not volatile: the streams are Borland's and nothing on the timer thread
   touches one, so `&BORLAND_STREAMS.streams[i]` is the `FILE *` the stream
   routines take. */

/*
 * What `heapwalk` fills in, Borland's `struct heapinfo`: the block's payload,
 * its size without the in-use bit, and that bit on its own.
 */
struct heapinfo {
    dg_near_t block_ptr;       /* +0x00 */
    uint16_t  size;            /* +0x02 */
    uint16_t  in_use;          /* +0x04 */
} PACKED;

int16_t brk_set(const uint8_t *addr);                     /* 0x0c7c4 */
void    heap_ring_unlink(struct heap_block *bx);              /* 0x0c95a */
void    heap_ring_insert(struct heap_block *bx);              /* 0x0c976 */
void    heap_free_middle(struct heap_block *bx);              /* 0x0c921 */
void    heap_free_top(struct heap_block *bx);                 /* 0x0c8e7 */
void    heap_free(void *p);                         /* 0x0c8ca */
void    heap_free(uint8_t *p);                      /* 0x0c8ca */
uint8_t *heap_sbrk(uint16_t lo, uint16_t hi);       /* 0x0c7e6 */
uint8_t *heap_init(uint16_t size);                  /* 0x0c9f9 */
uint8_t *heap_grow(uint16_t size);                  /* 0x0ca39 */
uint8_t *heap_split(struct heap_block *bx, uint16_t size);    /* 0x0ca62 */
void far_move(const uint8_t far * src, uint8_t far * dst, uint16_t count);    /* 0x0bd2e */
uint32_t long_multiply(uint32_t a, uint32_t b);      /* 0x0c16e */
uint32_t ulong_divide(uint32_t a, uint32_t b);       /* 0x0bd97 */
int32_t long_divide(int32_t a, int32_t b);           /* 0x0bd93 */
uint16_t near_memset(uint8_t *dst, uint16_t count,
                     uint16_t value);               /* 0x0d543 */
void   *heap_calloc(uint16_t count, uint16_t size); /* 0x0c833 */
uint8_t *heap_calloc(uint16_t count, uint16_t size); /* 0x0c833 */
char *int_to_string(int16_t value, char *buf,
                    int16_t radix);                 /* 0x0d4bd */
char *int_to_string(int16_t value, char *buf,
                       uint16_t radix);             /* 0x0d4bd */
char *long_int_to_string(int32_t value, char *buf,
                         int16_t radix);            /* 0x0d4ff */
char *long_int_to_string(int32_t value, char *buf,
                            uint16_t radix);        /* 0x0d4ff */
char *long_to_string(uint16_t letters, uint16_t is_signed,
                        uint16_t radix, char *buf,
                        int32_t value);               /* 0x0c029 */
void   *heap_malloc(uint16_t want);                 /* 0x0c999 */
uint8_t *heap_malloc(uint16_t want);                /* 0x0c999 */
int16_t dos_read(int16_t handle, uint8_t * buf, uint16_t count);   /* 0x0c185 */
int32_t dos_lseek(int16_t handle, int32_t off,
                  int16_t whence);                  /* 0x0c0c3 */
int16_t read_translated(int16_t handle, uint8_t *buf,
                        uint16_t count);            /* 0x0da6d */
void    flush_all_streams(void);                    /* 0x0d36d */
int16_t refill_stream(struct file_rec *file);               /* 0x0d396 */
int16_t borland_fgetc(struct file_rec *file);                 /* 0x0d404 */
int16_t borland_read(int16_t handle, uint8_t *buf, uint16_t count); /* 0x0db3b */
void    borland_rewind(struct file_rec *file);              /* 0x0db3e */
int16_t borland_getchar(void);                              /* 0x0d4b3 */
int16_t flush_stream(struct file_rec *file);                /* 0x0ce92 */
int16_t borland_flushall(void);                             /* 0x0cf13 */
int16_t borland_eof(int16_t handle);                        /* 0x0cd9e */
void    exit_close_streams(void);                           /* 0x0dfb4 */
void    exit_flush_streams(void);                           /* 0x0dfdc */
void    exit_hook_none(void);                               /* 0x0bc63 */
int16_t borland_atexit(struct far_ptr fn);                  /* 0x0bbfe */
void    borland_exit_common(int16_t status, int16_t dontexit,
                            int16_t quick);                 /* 0x0bc64 */
void    borland_exit_quick(int16_t status);                 /* 0x0bcca */
void    borland_cexit(void);                                /* 0x0bcdc */
void    borland_c_exit(void);                               /* 0x0bcea */
int16_t io_error_code(int16_t code);                        /* 0x0c006 */
int16_t dos_get_file_attr(const char *name, uint16_t *attr);  /* 0x0bc2b */
int16_t dos_set_file_attr(const char *name, uint16_t attr);   /* 0x0bc48 */
char   *tmp_number(char *buf, uint16_t number);             /* 0x0c0a6 */
char   *string_copy_end(char *dst, const char *src);        /* 0x0c79b */
char   *tmp_name_build(uint16_t number, const char *prefix,
                       char *buf);                          /* 0x0c0ec */
char   *tmp_name_unused(int16_t *counter, char *buf);       /* 0x0c12b */
int16_t borland_unlink(const char *name);                   /* 0x0c2bf */
void    float_formats_missing(int16_t from_scanf);          /* 0x0c884 */
void     printer_flush(struct printer *p);                  /* 0x0c31d */
void     printer_put(struct printer *p, char c);            /* 0x0c314 */
uint16_t printer_len(const char *s);                        /* 0x0c307 */
char    *hex_word(char *dst, uint16_t v);                   /* 0x0c2d5 */
int16_t vprinter(putn_fn put, void *sink, const char *fmt,
                 const uint8_t *args);                      /* 0x0c2ed */
uint16_t string_putn(void *sink, uint16_t n, const uint8_t *buf); /* 0x0dc34 */
int16_t borland_sprintf(char *buf, const char *fmt,
                        const uint8_t *args);               /* 0x0dc5c */
int16_t borland_vsprintf(char *buf, const char *fmt,
                         const uint8_t *args);              /* 0x0dc79 */
int32_t dos_tell(int16_t handle);                   /* 0x0c27b */
int16_t dos_isatty(int16_t handle);                 /* 0x0c018 */
int16_t dos_ioctl(int16_t handle, uint16_t al, uint16_t dx,
                  uint16_t cx);                     /* 0x0c8a3 */
int16_t dos_getattr(const char *name, uint16_t al, uint16_t cx); /* 0x0cd3d */
int16_t dos_open_named(const char *name, uint16_t flags); /* 0x0d707 */
int16_t parse_open_mode(uint8_t * out_perm, uint8_t * out_flags,
                        const char *mode);             /* 0x0cf4d */
int16_t borland_setvbuf(struct file_rec *file, uint8_t *buf, int16_t mode,
                      uint16_t size);               /* 0x0db5e */
struct file_rec *find_free_stream(void);                    /* 0x0d0a3 */
struct file_rec *borland_fopen_into(uint16_t extra_flags, const char *mode, const char *name,
                          struct file_rec *file);           /* 0x0d007 */
struct file_rec *borland_fopen(const char *name, const char *mode); /* 0x0d0ce */
uint32_t long_shift_left(uint32_t v, uint8_t count);  /* 0x0be3e */
int16_t io_error(int16_t code);                     /* 0x0bfcd */
struct far_ptr dos_getvect(uint16_t n);                   /* 0x0bd70 */
void dos_setvect(uint16_t n, struct far_ptr handler);     /* 0x0bd7f */
char *string_copy(char *dst, const char *src);    /* 0x0dd33 */
uint16_t string_length(const char *s);                 /* 0x0dd95 */
char *string_reverse(char *s);                /* 0x0de1e */
char *string_upper(char *s);                  /* 0x0de4e */
int16_t  string_ncompare_i(const char *a, const char *b,
                           uint16_t n);             /* 0x0dddb */
char *string_chr(const char *s, int16_t c); /* 0x0dcce */
char *string_chr(char *s, int16_t c);       /* 0x0dcce */
int16_t  string_compare(const char *a, const char *b);    /* 0x0dd04 */
int16_t string_compare_nocase(const char *a, const char *b); /* 0x0dd55 */
char *string_copy_padded(char *dst, const char *src,
                            uint16_t n);            /* 0x0ddaf */
int16_t open_file(const char *name, uint16_t flags, ...); /* 0x0d5af */
int16_t open_file(const char *name, uint16_t flags,
                  uint16_t perm);                   /* 0x0d5af */
int16_t dos_close(int16_t handle);                  /* 0x0cd80 */
uint8_t *  mem_copy(uint8_t * dst, const uint8_t * src, uint16_t n); /* 0x0d524 */
int16_t dos_write(int16_t handle, const uint8_t * buf, uint16_t count); /* 0x0df7a */
int16_t dos_creat(const char *name, uint16_t attr);    /* 0x0d584 */
void    dos_truncate(int16_t handle);               /* 0x0d59d */
int16_t close_handle(int16_t handle);               /* 0x0cd58 */
int16_t borland_fclose(struct file_rec *file);                /* 0x0ce15 */
int16_t unread_count(struct file_rec *file);                /* 0x0d20f */
int32_t borland_ftell(struct file_rec *file);                 /* 0x0d2d4 */
int16_t borland_fseek(struct file_rec *file, int32_t off,
                    int16_t whence);                /* 0x0d26c */
int16_t borland_getc(struct file_rec *file);                  /* 0x0d3ef */
uint16_t buffered_read(struct file_rec *file, uint16_t count,
                       uint8_t * buf);               /* 0x0d0ed */
uint16_t borland_fread(uint8_t * buf, uint16_t size, uint16_t count,
                     struct file_rec *file);                /* 0x0d1c4 */
int16_t heapwalk(struct heapinfo *info);                    /* 0x0ccef */
uint16_t borland_fwrite(const uint8_t * ptr, uint16_t size, uint16_t count,
                   struct file_rec *file);                  /* 0x0d321 */
uint16_t stream_put_run(struct file_rec *file, uint16_t count, const uint8_t * buf); /* 0x0d8ca */
int16_t borland_fputc(int16_t c, struct file_rec *file);      /* 0x0d784 */
int16_t borland_putc(int16_t c, struct file_rec *file);       /* 0x0d76b */
int16_t write_text(int16_t handle, const uint8_t * buf, uint16_t count); /* 0x0de6e */
int16_t borland_printf(const char *fmt, ...);        /* 0x0d754 */
void borland_exit(int16_t status);                    /* 0x0bcbb */
int32_t long_shift_right(int32_t v, uint8_t count);  /* 0x0be62 */
uint32_t long_multiply_2(uint32_t a, uint32_t b);    /* 0x0bcf6 */
void dos_getdate(uint8_t * out);                        /* 0x0bd4a */
uint16_t to_lower(uint16_t c);                         /* 0x0c293 */
char *string_concat(char *dst, const char *src);     /* 0x0dc95 */
int16_t borland_setbuf(struct file_rec *file, uint8_t *buf);     /* 0x0c1b2 */
int16_t heap_check(void);                              /* 0x0cb45 */
void setup_streams(void);                              /* 0x0c1d6 */

#endif
