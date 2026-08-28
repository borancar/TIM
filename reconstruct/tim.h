/*
 * The Incredible Machine - reconstruction
 *
 * Reconstructed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993), recovered from its LZEXE packing. This
 * file carries no licence: it is derived from someone else's executable and
 * that is not ours to license. See the repository README for the position on
 * both works.
 *
 * Addresses in comments are **image offsets** into the recovered image
 * (out/TIM.img), which is what tools/disasm.py lists.
 */
#ifndef TIM_H
#define TIM_H

#include <stdint.h>

/*
 * Widths are transcribed, not chosen. The original is 16-bit code where every
 * value has a width it depended on, so `int16_t` says "this truncation is the
 * instruction's" in a way `short` cannot. See CLAUDE.md.
 */

/* ---------------------------------------------------------------- segment 0000
 * Image 0x00000..0x0dff0. Contains the Borland startup at the entry point
 * 0000:0000 and game code; which parts are the C runtime is not yet
 * established - see STATUS.md.
 */

/* Subtract two fields of the structure DGROUP 0x5400 points at. */
void sub_002be(void);                               /* 0x002be */

/* Step the counter at DGROUP 0x4e87. */
void step_word_4e87(void);                          /* 0x0144e */

/* Advance the button state for one frame. */
void update_button_state(void);                     /* 0x08136 */

/* Set the clip box from the mode word: a saved rectangle or a fixed one. */
void set_clip_for_mode(void);                       /* 0x082c3 */

/* Set the clipping box to the whole visible screen. */
void set_clip_full_screen(void);                    /* 0x0834b */

/* Apply the kind's gravity, clamp, and compute a Manhattan speed. */
void apply_gravity_and_speed(uint16_t rec);         /* 0x02c39 */

/* Build the swept bounding box of the object at DGROUP 0x5400. */
void compute_swept_bounds_5400(void);               /* 0x002dd */

/* Derive a rectangle and its centre from the structure at DGROUP 0x53fe. */
void compute_bounds_53fe(void);                     /* 0x00386 */

/* Are two angles on the same side of a reference direction? */
int16_t angles_same_side(int16_t angle);            /* 0x003df */

/* Reduce a 16-bit angle to one of four directions. */
int16_t angle_to_quadrant(int16_t angle);           /* 0x004d1 */

/* Clamp two signed fields of a record to plus or minus a per-kind limit. */
void clamp_record_pair(uint16_t rec);               /* 0x02bcc */

/* Is a node on the chain hanging off a record? */
int16_t chain_contains(uint16_t rec, uint16_t node);      /* 0x03a61 */

/* Step the second word of each pair one further from the first. */
void step_pair_apart(uint16_t rec);                 /* 0x03d2e */

/* Are two points within 140 in both axes? */
int16_t points_within_140(uint16_t a, uint16_t b);  /* 0x04b53 */

/* Recompute a record's velocity from its movement, then clamp it. */
void update_velocity(uint16_t rec, uint8_t shift_x, uint8_t shift_y,
                     uint16_t which);               /* 0x07283 */

/* Splice one list onto the front of another and empty the first. */
void splice_list_4e58_onto_4e56(void);              /* 0x07b3e */

/* Is a value between two bounds, whichever way round they are? */
int16_t value_between(uint16_t v, uint16_t a, uint16_t b);   /* 0x03d67 */

/* Which side of a range a value falls on, as two flag bytes. */
void set_side_flags(uint16_t range, int16_t v, uint16_t out);   /* 0x004fd */

/* Insert a record into a sorted doubly-linked list. */
void insert_sorted(uint16_t rec, uint16_t head);    /* 0x05646 */

/* First of three words that is non-zero and enabled by its flag bit. */
int16_t pick_by_flag(uint16_t flags);               /* 0x05b65 */

/* Choose a value for a record: its own, or a shared slot. */
int16_t pick_for_record(uint16_t rec, uint16_t flags);    /* 0x05ba7 */

/* Which of two structure fields matches a value. */
int16_t match_field_5a_5c(int16_t value, uint16_t obj);   /* 0x06f43 */

/* Pick one of two record fields by matching the other. */
int16_t select_field_2_or_4(int16_t key, uint16_t rec);   /* 0x06f68 */

/* Present the frame: the game's wrapper around the driver's page flip. */
void present_frame(uint16_t wait_retrace);          /* 0x081cc */

/* Claim a slot in the two-entry page table at DGROUP 0x56e6. */
uint16_t claim_page_slot(uint16_t want);            /* 0x0b429 */

/* Save the driver's drawing state, or put it back. */
void save_or_restore_draw_state(int16_t save);      /* 0x0b47f */

/* Wait for the frame, then latch input state and clear the accumulators. */
void wait_and_latch_frame(void);                    /* 0x0aaca */

/* Not transcribed yet; see the source. */
void sub_0b078(void);                               /* 0x0b078 */
void sub_0e34a(uint16_t arg);                       /* 0x0e34a */

/* Look a word up through the far pointer at DGROUP 0x546c. */
int16_t lookup_table_546c(int16_t index);           /* 0x11d44 */

/* Set the number of scan lines the CRTC displays before blanking. */
void vm_set_display_lines(uint16_t lines);          /* 0x08f77 */

/* Non-zero while the frame flag has not yet been set by the timer handler. */
int16_t frame_pending(void);                        /* 0x0b4e2 */

/* ------------------------------------------------------- VM.OVL, VGA driver
 * A separate module: the game's video driver, loaded from the resource
 * archive. Addresses are offsets within the loaded driver, not image offsets.
 */

/* Show the page just drawn and swap the buffers. */
void vm_show_page(uint16_t wait_retrace);           /* VM.OVL VGA:0x150f */

/* Copy a rectangle between the two pages, in latch mode. */
void vm_copy_rect(uint16_t x, uint16_t y,
                  uint16_t width, uint16_t height);  /* VM.OVL VGA:0x1561 */

/* Fill a run of pixels on one scan line. Register arguments; see the source. */
void vm_span(uint16_t ax, uint16_t bx, int16_t cx,
             uint16_t dst_seg, uint16_t di);         /* VM.OVL VGA:0x034f */

/* The main blitter: a run of pixels from a byte-per-pixel source. */
void vm_blit_run(uint16_t bx, uint16_t cx, const uint8_t *src,
                 uint16_t dst_seg, uint16_t di,
                 int32_t backwards);                 /* VM.OVL VGA:0x0938 */

/* Fill a list of horizontal spans with one colour. */
void vm_fill_spans(uint16_t spans_seg,
                   uint16_t spans_off);              /* VM.OVL VGA:0x0be6 */

/* Load a sixteen-colour palette into the DAC and keep a copy. */
void vm_load_palette(uint16_t off, uint16_t seg);   /* VM.OVL VGA:0x0f15 */

/* Load colours into the DAC. */
void vm_set_palette(const uint8_t *rgb, uint16_t first,
                    uint16_t count);                 /* VM.OVL VGA:0x0ec1 */

/* ---------------------------------------------------------- segment 1c25
 * Image 0x1c250..0x248f0, the largest of the game's own modules.
 */

/* Fill a rectangle, clipped, via the driver's span filler. */
void fill_rect(int16_t x, int16_t y,
               int16_t w, int16_t h);               /* 0x20079 */

/* Does a NUL-terminated string contain 'r'? */
int16_t string_contains_r(uint16_t str);            /* 0x1c6e3 */

/* Copy between two far pointers, normalising both first. */
void far_memcpy(uint16_t dst_off, uint16_t dst_seg,
                uint16_t src_off, uint16_t src_seg,
                uint16_t count);                    /* 0x222c6 */

/* Set the current palette, or answer the one already set. */
uint32_t set_palette_pointer(uint16_t off, uint16_t seg);   /* 0x1eb6a */

/* Allocate from DOS by byte count; answers seg:0000 in DX:AX. */
uint32_t dos_alloc_bytes(uint16_t size_lo, uint16_t size_hi,
                         uint16_t unused,
                         uint16_t flags);           /* 0x21abd */

/* Fill memory through a far pointer, with a 32-bit count. */
void far_memset(uint16_t off, uint16_t seg, uint16_t value,
                uint16_t count_lo, uint16_t count_hi);   /* 0x22300 */

/* The far-callable face of normalise_far_ptr; answers seg:off in DX:AX. */
uint32_t normalise_far_ptr_far(uint16_t off, uint16_t seg);  /* 0x22386 */

/* Carry paragraphs out of a far pointer's offset into its segment. */
void normalise_far_ptr(uint16_t *off, uint16_t *seg);       /* 0x22161 */

/* Store a quarter of each of two words through near pointers. */
void read_pair_4740(uint16_t out_a, uint16_t out_b); /* 0x220e9 */

/* Bit 0 of one of two flag bytes at DGROUP 0x48ea. */
int16_t flag_bit_48ea(uint16_t which);              /* 0x2213e */

/* Bit 0 of the byte array at DGROUP 0x468c. */
int16_t bit0_of_468c(uint16_t index);               /* 0x2147d */

/* ---------------------------------------------------------- segment 14de */
void clear_word_array_50bf(void);                   /* 0x166d6 */

/* Link a record into up to two buckets headed by that array. */
void link_record_into_buckets(uint16_t rec);        /* 0x166ef */

/* ---------------------------------------------------------- segment 2619 */
uint16_t advance_record(const uint8_t *rec, uint16_t off);  /* 0x2891a */

/* Follow a chain of far pointers; answers seg:off packed into 32 bits. */
uint32_t follow_far_chain(uint16_t off, uint16_t seg,
                          int16_t count);           /* 0x2907b */

/* Scale one byte by another and halve the range. */
uint8_t scale_byte_pair(uint8_t cl, uint8_t dl);    /* 0x282cb */

/* ---------------------------------------------------------- segment 2a04 */

/* Sine and cosine of a 16-bit angle, 16384 standing for 1. */
int16_t angle_sin(uint16_t angle);                  /* 0x2a456 */
int16_t angle_cos(uint16_t angle);                  /* 0x2a47b */

/* Signed 16x16 multiply; answers the 32-bit product in DX:AX. */
uint32_t mul16x16(int16_t a, int16_t b);            /* 0x2a269 */

/* Not transcribed yet; the driver's line drawer. */
void vm_draw_line(int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2);          /* VM.OVL VGA:0x0998 */

/* Clip a line to the clip box and draw what is left. */
void clip_and_draw_line(int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2);    /* 0x21e34 */

#endif /* TIM_H */
