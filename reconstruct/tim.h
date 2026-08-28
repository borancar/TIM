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

/* Recompute gravity and the velocity limit for every kind. */
void recompute_kind_physics(void);                  /* 0x02ac0 */

/* Clamp two signed fields of a record to plus or minus a per-kind limit. */
void clamp_record_pair(uint16_t rec);               /* 0x02bcc */

/* Rotate a point about the origin, in place. */
void rotate_point(uint16_t px, uint16_t py, uint16_t angle);   /* 0x03b17 */

/* Is a node on the chain hanging off a record? */
int16_t chain_contains(uint16_t rec, uint16_t node);      /* 0x03a61 */

/* Find which record owns the far pointer in the globals at 0x5482. */
int16_t find_entry_for_pointer(uint16_t out);       /* 0x098e0 */

/* Bytes a w by h planar image needs. */
uint32_t vm_buffer_size(uint16_t w, uint16_t h);    /* VM.OVL VGA:0x138e */

/* Save a rectangle of the source page into a buffer, all four planes. */
void vm_save_rect(uint16_t buf_off, uint16_t buf_seg, int16_t x, int16_t y,
                  int16_t w, int16_t h);            /* VM.OVL VGA:0x12fb */

/* Restore a rectangle from a buffer into the destination page. */
void vm_restore_rect(uint16_t buf_off, uint16_t buf_seg, int16_t x, int16_t y,
                     int16_t w, int16_t h);         /* VM.OVL VGA:0x13b9 */

/* atan2 of two longs, in the whole-turn-is-0x10000 space. */
int16_t atan2_long(uint16_t a_lo, uint16_t a_hi,
                   uint16_t b_lo, uint16_t b_hi);   /* 0x2d296 */

/* Chain every object whose box comes within the given margins. */
void link_nearby_objects(uint16_t obj, uint16_t flags,
                         int16_t margin_x0, int16_t margin_x1,
                         int16_t margin_y0, int16_t margin_y1); /* 0x03566 */

/* The same sweep with the two objects exchanged. */
int16_t find_edge_contact_reversed(int16_t test_only);  /* 0x00b6c */

/* Handle an explicit note-off event; answers the advanced cursor. */
uint16_t midi_note_off_event(uint16_t ds, uint16_t bp, uint16_t es,
                             uint16_t bx, uint16_t si,
                             uint16_t ax);          /* 0x27e92 */

/* Two-byte event, driver function 6 (a stub). */
uint16_t midi_event_6(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                      uint16_t si, uint16_t ax);    /* 0x27f54 */

/* Program change: stores the instrument at +0x116. */
uint16_t midi_program_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                            uint16_t si, uint16_t ax);  /* 0x28086 */

/* One-byte event, driver function 9 (a stub). */
uint16_t midi_event_9(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                      uint16_t si, uint16_t ax);    /* 0x280da */

/* Handle one MIDI note event; answers the advanced stream cursor. */
uint16_t midi_note_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax);  /* 0x27ee1 */

/* Parse a sequence's device parameter table once, cached in place. */
void init_sequence_params(uint16_t es, uint16_t ax);  /* 0x28305 */

/* Next record matching a selector, as a far pointer in DX:AX. */
uint32_t next_matching_record(int16_t selector);    /* 0x29966 */

/* Handle one pitch bend event; answers the advanced stream cursor. */
uint16_t midi_bend_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax);  /* 0x280fe */

/* Allocate a block for the sound module by kind; zero some kinds. */
uint32_t alloc_for_kind(uint16_t size_lo, uint16_t size_hi,
                        uint16_t kind);             /* 0x29f89 */

/* Release a block by the same kind it was allocated with. */
void free_for_kind(uint16_t off, uint16_t seg,
                   uint16_t kind);                  /* 0x2a017 */

/* Free a chain of kind-9 nodes linked at +4. */
void free_node_list(uint16_t off, uint16_t seg);    /* 0x28baf */

/* Build a sequence record around note data; null far pointer on failure. */
uint32_t create_sequence(uint16_t src_off, uint16_t src_seg);  /* 0x28935 */

/* The ordinary-call face of start_sequence. */
void start_sequence_far(uint16_t off, uint16_t seg,
                        uint16_t flag);             /* 0x28480 */

/* Locate a sequence, set its volume, and start it. */
uint32_t load_and_start_sequence(uint16_t off, uint16_t seg, int16_t count,
                                 uint16_t volume);  /* 0x29034 */

/* Start a sequence: reset it, read its header, place it in the table. */
void start_sequence(uint16_t es, uint16_t ax, uint16_t cx);  /* 0x26783 */

/* Advance a sequence's volume fade by one tick. */
void advance_volume_ramp(uint16_t es, uint16_t bx,
                         uint16_t seq_slot);        /* 0x278e9 */

/* Set a sequence's volume and push it to every voice it owns. */
void set_sequence_volume(uint16_t es, uint16_t bx, uint8_t volume,
                         uint8_t defer, uint16_t seq_slot);  /* 0x279a9 */

/* Remove a sequence unless it is on the poll table. */
void drop_unless_polled(uint16_t es, uint16_t bx);  /* 0x27b52 */

/* Poll sequences on the cs:0x48 table through the host callback. */
void poll_sequences(void);                          /* 0x27b7e */

/* Take a sequence out of the playing table and stop it. */
void remove_sequence(uint16_t es, uint16_t ax);     /* 0x26e7b */

/* Call the host's sound callback if one is installed. */
uint16_t sound_callback(uint16_t ax);               /* 0x292a1 */

/* The sequencer tick: place voices and tell the driver. */
void sequencer_tick(void);                          /* 0x26f2a */

/* Flush up to two pending volume changes to the driver. */
void flush_pending_volumes(void);                   /* 0x27a86 */

/* The PC-speaker sound driver, SX.OVL - see docs/sound-driver.md. */
void     sx_speaker_off(void);                  /* SX.OVL SPKR:0x0480 */
uint16_t sx_apply_bend(uint16_t index);         /* SX.OVL SPKR:0x04fd */
void     sx_note_on(uint16_t note);             /* SX.OVL SPKR:0x0497 */
void     sx_stop_note(uint16_t cx);             /* SX.OVL SPKR:0x037b */
void     sx_start_note(uint16_t ax, uint16_t cx);  /* SX.OVL SPKR:0x0386 */
void     sx_nop(void);                          /* SX.OVL SPKR:0x037a */
void     sx_controller(uint16_t ax, uint16_t cx);  /* SX.OVL SPKR:0x03a1 */
void     sx_pitch_bend(uint16_t ax, uint16_t cx);  /* SX.OVL SPKR:0x0410 */
void     sx_stop_all(void);                     /* SX.OVL SPKR:0x0525 */
uint16_t sx_param_345(uint16_t cx);             /* SX.OVL SPKR:0x0529 */
uint16_t sx_param_349(uint16_t cx);             /* SX.OVL SPKR:0x0549 */
uint16_t sx_param_346(uint16_t cx);             /* SX.OVL SPKR:0x055b */
uint16_t sx_query(uint16_t ax, uint16_t cx);    /* SX.OVL SPKR:0x057d */
void     sx_describe_1(uint16_t *ax, uint16_t *cx);  /* SX.OVL SPKR:0x05a8 */
void     sx_describe_0(uint16_t *ax, uint16_t *cx);  /* SX.OVL SPKR:0x05b0 */

/* Resolve one object against everything it could be touching. */
int16_t resolve_collisions(uint16_t obj);           /* 0x00556 */

/* Sweep one object's edges against another's; record the contact. */
int16_t find_edge_contact(int16_t test_only);       /* 0x007af */

/* Advance an object one step: velocity, gravity, clamp, place. */
void integrate_object(uint16_t obj);                /* 0x02c93 */

/* Work out where an object is drawn, at +0x2a/+0x2c. */
void place_object_for_draw(uint16_t obj);           /* 0x05be4 */

/* Add shape records for a sub-object's point pairs. */
void add_sub_object_shapes(uint16_t obj, int16_t mask);  /* 0x05ef6 */

/* Set an object's extent at +0x44/+0x46 from its kind. */
void set_object_extent(uint16_t obj);               /* 0x05c77 */

/* Angle from two differences across an object's +0x1e/+0x22 fields. */
int16_t object_delta_angle(uint16_t obj);           /* 0x004ab */

/* Arctangent table lookup; index is a ratio in 0..511. */
int16_t arctan_lookup(uint16_t index);              /* 0x2a941 */

/* Apply contact friction to an object. */
void apply_contact_friction(uint16_t obj);          /* 0x02da0 */

/* Read one pixel's colour from the source page; no clipping. */
uint16_t vm_read_pixel(int16_t x, int16_t y);       /* VM.OVL VGA:0x1453 */

/* Read a pixel if inside the driver's clip window, else -1. */
int16_t read_pixel_clipped(int16_t x, int16_t y);   /* 0x2241b */

/* Plot one pixel; no clipping. */
uint16_t vm_plot_pixel(int16_t x, int16_t y,
                       uint8_t colour);             /* VM.OVL VGA:0x14c9 */

/* Plot a pixel if inside the driver's clip window, else -1. */
int16_t plot_pixel_clipped(int16_t x, int16_t y,
                           int16_t colour);         /* 0x2244d */

/* Take the object off both the drawn-into and on-screen pages. */
void erase_both_pages(void);                        /* 0x080e7 */

/* Put back what an object was covering; mark it not drawn. */
void erase_object(uint16_t handle);                 /* 0x0ad51 */

/* Age an object's on-screen rectangle by one frame. */
void restage_object_rect(uint16_t handle);          /* 0x0aef6 */

/* Claim one of four scratch buffers; one-based index, or -1. */
int16_t claim_buffer_slot(uint16_t a_lo, uint16_t a_hi,
                          uint16_t b_lo, uint16_t b_hi);  /* 0x0b5ed */

/* Clear one byte of the one-based four-entry array at 0x5734. */
void clear_slot_5734(int16_t n);                    /* 0x0b69c */

/* Zero the word at DGROUP 0x2d44; meaning not established. */
void clear_flag_2d44(void);                         /* 0x0a7a3 */
void clear_flag_2d44_thunk(void);                   /* 0x0811b */

/* Hand a block back to DOS; only the pointer's segment is used. */
void dos_free_far(uint16_t off, uint16_t seg);      /* 0x21b34 */

/* Recompute a link's endpoints, then the rest lengths they imply. */
void refresh_link_geometry(uint16_t link);          /* 0x04f7f */

/* Set an object's vector at +0x36/+0x38 from angle and magnitude. */
void set_vector_from_angle(uint16_t obj, uint16_t angle,
                           int16_t mag);            /* 0x07223 */

/* Rest length less actual separation, at one end of a link. */
int16_t link_slack(uint16_t obj, uint16_t link,
                   int16_t gen);                    /* 0x0713d */

/* The vector a link has to close, and its approximate length. */
int16_t link_endpoint_gap(uint16_t link, uint16_t obj, uint16_t out_dx,
                          uint16_t out_dy);         /* 0x07947 */

/* Distance from a link's endpoint to the endpoint it joins. */
int16_t link_end_distance(uint16_t link, int16_t gen,
                          int16_t end);             /* 0x06f8e */

/* Age the state histories of everything about to be stepped. */
void shift_all_histories(void);                     /* 0x07ca2 */

/* Age every tracked quantity on an object by one step. */
void shift_state_history(uint16_t obj);             /* 0x07ce3 */

/* Classify a link's endpoints against the ones they connect to. */
int16_t compare_link_ends(uint16_t link, int16_t end,
                          int16_t reversed);        /* 0x06de9 */

/* Intersect two segments; answers whether the point lies on both. */
int16_t intersect_segments(uint16_t seg1, uint16_t seg2,
                           uint16_t out);           /* 0x03ba9 */

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

/* Work out the endpoints of the link between a pair of objects. */
void compute_link_endpoints(uint16_t link);         /* 0x04e65 */

/* Which side of a range a value falls on, as two flag bytes. */
void set_side_flags(uint16_t range, int16_t v, uint16_t out);   /* 0x004fd */

/* Insert a record into a sorted doubly-linked list. */
void insert_sorted(uint16_t rec, uint16_t head);    /* 0x05646 */

/* First of three words that is non-zero and enabled by its flag bit. */
int16_t pick_by_flag(uint16_t flags);               /* 0x05b65 */

/* Choose a value for a record: its own, or a shared slot. */
int16_t pick_for_record(uint16_t rec, uint16_t flags);    /* 0x05ba7 */

/* Add one or both of a record's two shapes. */
void add_record_shapes(uint16_t rec, uint16_t which);   /* 0x0642a */

/* Take a node off the free list and fill it in as a shape. */
void alloc_shape(uint16_t pt1, uint16_t pt2, uint8_t flags, uint8_t which,
                 int16_t width);                    /* 0x064b4 */

/* Which of two structure fields matches a value. */
int16_t match_field_5a_5c(int16_t value, uint16_t obj);   /* 0x06f43 */

/* Pick one of two record fields by matching the other. */
int16_t select_field_2_or_4(int16_t key, uint16_t rec);   /* 0x06f68 */

/* Present the frame: the game's wrapper around the driver's page flip. */
void present_frame(uint16_t wait_retrace);          /* 0x081cc */

/* Clear the input accumulators and latched state. */
void reset_input_state(void);                       /* 0x0b4f1 */

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
