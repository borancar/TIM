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

/* The DGROUP layout, for the record types a prototype below takes by value -
   `struct far_ptr` is the one. dgroup.h includes nothing but <stdint.h>, so
   this is not a cycle. */
#include "dgroup.h"
/*
 * **A near pointer, as a parameter type.**
 *
 * A routine handed the address of a local or of a DGROUP record used to take a
 * `uint16_t` offset; it takes the pointer itself now, because the port's stack
 * is the port's own and an offset into it means nothing. The guest still
 * pushes **one word** for such an argument, and a plain `uint8_t *` parameter
 * would be a *far* pointer costing two - so the hybrid's shim generator has to
 * be able to tell them apart, and these are what it reads. See
 * `tools/native/genshims.py`.
 *
 * `volatile` because the bytes are the guest's memory and something else may
 * be writing them; byte-wide because a packed record's field can sit at an odd
 * address, which is what `dg_rd16` below is for.
 */
/*
 * **The tags themselves.** Borland's `near`, `far` and `huge` say which of the
 * two pointer kinds a declaration still needs to distinguish; a modern
 * compiler has one kind and they erase to nothing. Keeping them in the source costs the host build
 * nothing and is what would let this be compiled by the compiler it came from.
 *
 * They are defined *after* every system header this program includes - each
 * `.c` puts its `<...>` first - so erasing two identifiers cannot reach
 * anything but this port. Nothing here is named `far` or `huge`.
 *
 * **There is no `near` tag: an untagged pointer is the near one.** Marking
 * only what is far is the smaller thing to write and the smaller thing to get
 * wrong, and `tools/native/genshims.py` reads it that way - `far` is two guest
 * words, anything else with a `*` is one.
 *
 * Measured before the tag was dropped, and the first measurement was wrong in
 * the direction that would have hidden a bug: a check that scanned only the
 * `FAR_*` dispatch entries said **none** of the routines had an untagged
 * pointer parameter. Over all 181 entries of every kind there is exactly one,
 * `vm_set_palette`'s `rgb` - and it was being marshalled as *far*, two words,
 * where its spec says one at `[bp+4]` with `first` and `count` behind it. So
 * the old default was not merely unrelied upon, it was shifting that routine's
 * arguments, and dropping the tag fixes it.
 *
 * The cost is Borland's, and it is worth stating rather than discovering. In
 * the large model a pointer with no tag is `far`, so a rebuild by the original
 * compiler would need `near` put back on the near ones. It buys nothing on the
 * host, where both erase, and this port is not built by that compiler today.
 *
 * **They are a spelling, not a semantics.** On the host a `far` pointer is an
 * ordinary pointer: it does not wrap at 64K the way the real one does. Where
 * the wrap or the stored `seg:off` pair actually matters, the port uses
 * `struct far_ptr` in dgroup.h instead, and says so at the site.
 */
#ifndef __BORLANDC__
#  define far    /* one kind of pointer here */
#  define huge   /* likewise */
#endif

/* The typedefs `dg_near`/`dg_cnear` stood here. They are spelled out now -
   `volatile uint8_t *` - so the tag says which pointer kind it is. */

/*
 * And the **far** pair, which the guest pushes as two words - the offset then
 * the segment, in that order, because the last argument pushed is the first.
 *
 * A plain `uint8_t *` already costs two words in the shim generator, so these
 * are not needed to make that work; they are here so that a far pointer says
 * so at the call site the way `volatile uint8_t *` does, and so that a parameter which
 * used to be spelled `(uint16_t off, uint16_t seg)` reads as the one value it
 * always was. `MK_FP(seg, off)` makes one and `FP_SEG`/`FP_OFF` in dgroup.h
 * take one apart, which are Borland's own names for both halves of the job. Those answer the
 * **normalised** pair and can only answer that one, because a host pointer
 * does not remember which of the many `seg:off` pairs addressing it the guest
 * was holding.
 */
/* And `dg_far`/`dg_cfar`, likewise spelled out. */


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

uint16_t part_hook_yes(uint16_t part);              /* 0x00297 */
void     part_hook_none_2a1(uint16_t part);         /* 0x002a1 */
void     part_hook_none_2a6(uint16_t part);         /* 0x002a6 */
void     part_hook_none_2ab(uint16_t part);         /* 0x002ab */
void     part_hook_none_2b0(uint16_t part);         /* 0x002b0 */
uint16_t part_hook_no(uint16_t part);               /* 0x002b5 */

/* Subtract two fields of the structure DGROUP 0x5400 points at. */
void sub_002be(void);                               /* 0x002be */

/* Step the counter at DGROUP 0x4e87. */
void step_word_4e87(void);                          /* 0x0144e */

/* Advance the button state for one frame. */
void update_button_state(void);                     /* 0x08136 */

/* Set the clip box from the mode word: a saved rectangle or a fixed one. */
void set_clip_for_mode(void);                       /* 0x082c3 */

/* Set the clipping box to the whole visible screen. */
void set_clip_play_area(void);                      /* 0x08332 */
void set_clip_full_screen(void);                    /* 0x0834b */

/* Apply the kind's gravity, clamp, and compute a Manhattan speed. */
void apply_gravity_and_speed(struct part *rec);         /* 0x02c39 */

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
void clamp_record_pair(struct part *rec);               /* 0x02bcc */

/* Rotate a point about the origin, in place. */
/* px and py are read and written in place; the guest passes each as one
   DGROUP word, which `volatile uint8_t *` is what tells the shim generator. */
void rotate_point(volatile uint8_t * px, volatile uint8_t * py, uint16_t angle); /* 0x03b17 */

/* Is a node on the chain hanging off a record? */
int16_t chain_contains(struct part *rec, uint16_t node);      /* 0x03a61 */

/* Find which record owns the far pointer in the globals at 0x5482. */
int16_t find_entry_for_pointer(uint16_t out);       /* 0x098e0 */

/* Bytes a w by h planar image needs. */
uint32_t vm_buffer_size(uint16_t w, uint16_t h);    /* VM.OVL VGA:0x138e */
/* Chunky 4bpp to planar, through video memory, filling a list of headers. */
void vm_load_bitmap_list(bmp_ptr_t * list, struct far_ptr dst,
                         uint32_t count);                       /* VGA:0x1015 */
void vm_chunky_to_planar(struct far_ptr src, struct far_ptr dst,
                         uint16_t count);                       /* VGA:0x10b8 */
void vm_read_four_planes(struct far_ptr src, struct far_ptr dst,
                         uint16_t count);                       /* VGA:0x11bb */
void vm_build_mask_plane(struct far_ptr src, struct far_ptr dst,
                         uint16_t count);                       /* VGA:0x11ee */

void vm_nothing(void);                              /* VGA:0x0252 */
void vm_blit_rows(struct far_ptr src, int16_t x, int16_t y,
                  int16_t w, int16_t h);            /* VGA:0x15d0 */
void blit_rows_thunk(struct far_ptr src, int16_t x, int16_t y,
                     int16_t w, int16_t h);         /* 0x20838 */
void blit_rows_alt_thunk(void);                     /* 0x2083c */
void vm_blit_bitmap(struct bitmap * bmp, int16_t x, int16_t y,
                    uint16_t mode);                     /* VGA:0x1707 */
void vm_blit_scaled(struct bitmap * bmp, int16_t x, int16_t y); /* VGA:0x271b */
void blit_bitmap_thunk(struct bitmap * bmp, int16_t x, int16_t y,
                       uint16_t mode);                  /* 0x1e940 */
void blit_scaled_thunk(struct bitmap * bmp, int16_t x, int16_t y); /* 0x1e944 */
void draw_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode); /* 0x25300 */
void draw_compressed_bitmap(struct bitmap * bmp, int16_t x, int16_t y,
                            uint16_t mode);             /* 0x20185 */
void draw_offset_bitmap(struct bitmap * bmp, int16_t x, int16_t y,
                        uint16_t mode);                 /* 0x24e9a */
uint32_t vm_bitmap_list_size(uint16_t list,
                             volatile uint8_t * out);         /* VM.OVL VGA:0x0fd4 */

/* Save a rectangle of the source page into a buffer, all four planes. */
void vm_save_rect(struct far_ptr buf, int16_t x, int16_t y,
                  int16_t w, int16_t h);            /* VM.OVL VGA:0x12fb */

/* Restore a rectangle from a buffer into the destination page. */
void vm_restore_rect(struct far_ptr buf, int16_t x, int16_t y,
                     int16_t w, int16_t h);         /* VM.OVL VGA:0x13b9 */

/* atan2 of two longs, in the whole-turn-is-0x10000 space. */
int16_t atan2_long(int32_t a, int32_t b);          /* 0x2d296 */

/* Chain every object whose box comes within the given margins. */
void link_nearby_objects(struct part *obj, uint16_t flags,
                         int16_t margin_x0, int16_t margin_x1,
                         int16_t margin_y0, int16_t margin_y1); /* 0x03566 */

/* The same sweep with the two objects exchanged. */
int16_t find_edge_contact_reversed(int16_t test_only);  /* 0x00b6c */

/* Step one sequence forward by one tick. */
void step_sequence(uint16_t es, uint16_t bx, uint16_t di);  /* 0x27c4e */

/* Handle an explicit note-off event; answers the advanced cursor. */
uint16_t midi_note_off_event(uint16_t ds, uint16_t bp, uint16_t es,
                             uint16_t bx, uint16_t si,
                             uint16_t ax);          /* 0x27e92 */

/* Two-byte event, driver function 6 (a stub). */
uint16_t midi_event_6(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                      uint16_t si, uint16_t ax);    /* 0x27f54 */

/* Sequencer meta events: checkpoints, loop counters, rewinds. */
uint16_t midi_meta_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax);  /* 0x2817e */

/* Step past an event this module does not handle. */
uint16_t skip_unknown_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                            uint16_t si, uint16_t ax);  /* 0x2828e */

/* A forwarder to skip_unknown_event. */
uint16_t midi_skip_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax);     /* 0x2817a */

/* Controller change: keeps most of a channel's state. */
uint16_t midi_controller_event(uint16_t ds, uint16_t bp, uint16_t es,
                               uint16_t bx, uint16_t si,
                               uint16_t ax);        /* 0x27f85 */

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
struct far_ptr alloc_for_kind(uint32_t size,
                              uint16_t kind);             /* 0x29f89 */

/* Release a block by the same kind it was allocated with. */
void free_for_kind(struct far_ptr blk,
                   uint16_t kind);                  /* 0x2a017 */

/* Free a chain of kind-9 nodes linked at +4. */
void free_node_list(struct far_ptr list);            /* 0x28baf */

/* Build a sequence record around note data; null far pointer on failure. */
struct far_ptr create_sequence(struct far_ptr src);       /* 0x28935 */

/* The ordinary-call face of start_sequence. */
void start_sequence_far(struct far_ptr rec,
                        uint16_t flag);             /* 0x28480 */

/* Locate a sequence, set its volume, and start it. */
uint32_t load_and_start_sequence(struct far_ptr rec, int16_t count,
                                 uint16_t volume);  /* 0x29034 */

/* Start a sequence: reset it, read its header, place it in the table. */
void start_sequence(uint16_t es, uint16_t ax, uint16_t cx);  /* 0x26783 */

/* Advance a sequence's volume fade by one tick. */
void advance_volume_ramp(uint16_t es, uint16_t bx,
                         uint16_t seq_slot);        /* 0x278e9 */

/* Set a sequence's volume and push it to every voice it owns. */
void set_sequence_volume(uint16_t es, uint16_t bx, uint8_t volume,
                         uint8_t defer, uint16_t seq_slot);  /* 0x279a9 */

/* The sound module's service routine - what the timer calls. */
void sound_service(void);                           /* 0x27ace */

/* Remove a sequence unless it is on the poll table. */
void drop_unless_polled(uint16_t es, uint16_t bx);  /* 0x27b52 */

/* Poll sequences on the cs:0x48 table through the host callback. */
void poll_sequences(void);                          /* 0x27b7e */

/* Take a sequence out of the playing table and stop it. */
void remove_sequence(uint16_t es, uint16_t ax);     /* 0x26e7b */

/* Call the host's sound callback if one is installed. */
uint16_t sound_callback(uint16_t ax, volatile uint8_t * si);  /* 0x292a1 */

/* The sequencer tick: place voices and tell the driver. */
void sequencer_tick(void);                          /* 0x26f2a */

/* Flush up to two pending volume changes to the driver. */
void flush_pending_volumes(void);                   /* 0x27a86 */

/* The PC-speaker sound driver, SX.OVL - see docs/sound-driver.md. */
uint16_t install_driver(struct far_ptr drv);  /* 0x265f2 */
uint16_t configure_driver(struct far_ptr drv); /* 0x26629 */
void silence_driver(void);                          /* 0x2664e */
void set_master_level(uint8_t cl);                  /* 0x26721 */
void retire_and_tick(struct far_ptr rec);                         /* 0x26a57 */

/* The sound module's own routines over that driver, in address order. */
uint32_t voice_playing(struct far_ptr rec);    /* 0x287ad */
uint16_t alloc_voice_records(void);                    /* 0x28800 */
void follow_then_tick(struct far_ptr rec,
                      int16_t count);                  /* 0x289ba */
uint16_t seek_to_sound_record(int16_t handle,
                              uint16_t want);          /* 0x28bf2 */
struct far_ptr read_sound_records(int16_t handle);           /* 0x28cf7 */
uint16_t read_record(uint16_t file, uint16_t mode);     /* 0x29da0 */
uint16_t start_sound(int16_t device, int16_t module_index,
                     uint16_t callback, uint16_t handle); /* 0x29c3b */
uint16_t setup_sound_device(int16_t device, int16_t module_index,
                            uint16_t callback, uint16_t handle); /* 0x28655 */
uint16_t load_sound_module(uint16_t handle, uint16_t number,
                           uint16_t index);         /* 0x28580 */
struct far_ptr load_named_chunk(uint16_t handle, const char * path,
                          uint16_t index);          /* 0x28886 */
struct far_ptr load_sound_bank(uint16_t file, uint32_t size,
                               volatile uint8_t * out);    /* 0x289e8 */
struct far_ptr load_resource_block(uint16_t file, uint32_t size,
                                   volatile uint8_t * out,
                                   uint16_t kind);      /* 0x28f74 */
uint16_t build_sound_index(int16_t handle, struct far_ptr list,
                           struct far_ptr dst, uint16_t data_at,
                           uint16_t tag);              /* 0x28e87 */
struct far_ptr insert_by_key(struct far_ptr head, struct far_ptr node);
void stop_voice_playing(struct far_ptr rec);   /* 0x290ab */
uint16_t free_voice_records(void);                     /* 0x29106 */
uint32_t start_on_free_voice(struct far_ptr rec, uint16_t index,
                             uint16_t byte_arg);       /* 0x29152 */
void stop_all_voices(void);                            /* 0x2923d */
void set_sound_callback(struct far_ptr cb);   /* 0x2928c */
void stop_sound(void);                                 /* 0x292f4 */
void shutdown_sound(void);                             /* 0x29cf6 */
void delay_five_ticks(void);                           /* 0x2937f */
void tick_delay(void);                                 /* 0x293b8 */
uint16_t remove_and_free_records(int16_t selector);    /* 0x293c1 */
uint16_t stop_sequences(int16_t selector);             /* 0x294ff */
uint16_t open_sound_file(uint16_t handle, int16_t id);  /* 0x296b4 */
uint16_t set_master_level_ok(uint16_t level);          /* 0x296a1 */
uint16_t start_sequence_by_id(int16_t id);             /* 0x29a49 */

/* The ordinary-call faces of the hand-written routines above. */
void set_master_level_far(uint16_t level);             /* 0x28431 */
uint16_t install_driver_far(struct far_ptr drv);    /* 0x28458 */
uint16_t configure_driver_far(struct far_ptr drv);  /* 0x2846a */
void retire_and_tick_far(struct far_ptr rec);  /* 0x284ef */
void silence_driver_far(struct far_ptr drv);   /* 0x28559 */

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

/*
 * `SBP:`, the Sound Blaster Pro driver, in reconstruct/src/sxovl_sbp.c. Two
 * OPL2 chips and a mixer; see that file's header for the ports.
 */
void     sbp_write(uint16_t reg, uint16_t val);     /* SX.OVL SBP:0x2185 */
void     sbp_mixer_write(uint16_t reg, uint16_t val); /* SX.OVL SBP:0x21ac */
void     sbp_program_change(uint16_t ax, uint16_t cx); /* SX.OVL SBP:0x1a20 */
uint16_t sbp_param_349(uint16_t cl);                /* SX.OVL SBP:0x1abc */
uint16_t sbp_param_345(uint16_t cl);                /* SX.OVL SBP:0x1a92 */
uint16_t sbp_set_enable(uint16_t cl);               /* SX.OVL SBP:0x1a6d */
void     sbp_write_left(uint16_t reg, uint16_t val);  /* SX.OVL SBP:0x21d3 */
void     sbp_write_right(uint16_t reg, uint16_t val); /* SX.OVL SBP:0x220f */
void     sbp_touch_voice(uint16_t voice);           /* SX.OVL SBP:0x1dfe */
uint16_t sbp_apply_bend(uint16_t voice, uint16_t cx); /* SX.OVL SBP:0x1ed7 */
void     sbp_write_level(uint16_t voice, uint16_t level); /* SX.OVL SBP:0x1f2e */
void     sbp_note(uint16_t voice, uint16_t note, uint16_t keyon); /* SBP:0x1e2d */
void     sbp_key_off(uint16_t voice);               /* SX.OVL SBP:0x1dce */
uint16_t sbp_alloc_voice(uint16_t channel);         /* SX.OVL SBP:0x1ade */
void     sbp_pitch_bend(uint16_t ax, uint16_t cx);  /* SX.OVL SBP:0x1a2e */
void     sbp_write_op_level(uint16_t op);           /* SX.OVL SBP:0x232a */
void     sbp_write_op_feedback(uint16_t op);        /* SX.OVL SBP:0x238a */
void     sbp_write_op_attack_decay(uint16_t op);    /* SX.OVL SBP:0x23da */
void     sbp_write_op_sustain_release(uint16_t op); /* SX.OVL SBP:0x2420 */
void     sbp_write_op_mult(uint16_t op);            /* SX.OVL SBP:0x2466 */
void     sbp_write_op_wave(uint16_t op);            /* SX.OVL SBP:0x24d4 */
void     sbp_write_rhythm(void);                    /* SX.OVL SBP:0x2278 */
void     sbp_write_nts(void);                       /* SX.OVL SBP:0x2372 */
void     sbp_write_operator(uint16_t op);           /* SX.OVL SBP:0x2311 */
void     sbp_load_operator(uint16_t op, uint16_t src, uint16_t dl); /* SBP:0x22c8 */
void     sbp_load_operator_scratch(uint16_t op, uint16_t src, uint16_t dl);
                                                    /* SX.OVL SBP:0x22a1 */
void     sbp_reset_operators(void);                 /* SX.OVL SBP:0x224b */
void     sbp_silence(void);                         /* SX.OVL SBP:0x2513 */
void     sbp_load_patch(uint16_t voice, uint16_t patch); /* SX.OVL SBP:0x20d8 */
void     sbp_start_voice(uint16_t voice, uint16_t cx); /* SX.OVL SBP:0x1d4f */
void     sbp_ctrl_volume(uint16_t ax, uint16_t cx); /* SX.OVL SBP:0x1cb3 */
void     sbp_ctrl_pan(uint16_t ax, uint16_t cx);    /* SX.OVL SBP:0x1cec */
void     sbp_ctrl_sustain(uint16_t ax, uint16_t cx); /* SX.OVL SBP:0x1d1f */
void     sbp_reserve_voices(uint16_t ax, uint16_t cx); /* SX.OVL SBP:0x1bf6 */
void     sbp_release_voices(uint16_t ax, uint16_t cx); /* SX.OVL SBP:0x1c3a */
void     sbp_redistribute_voices(void);             /* SX.OVL SBP:0x1b9c */
void     sbp_ctrl_reserve(uint16_t ax, uint16_t cx); /* SX.OVL SBP:0x1b5c */
void     sbp_controller(uint16_t ax, uint16_t cx);  /* SX.OVL SBP:0x19d4 */
void     sbp_stop_note(uint16_t ax, uint16_t cx);   /* SX.OVL SBP:0x1957 */
void     sbp_start_note(uint16_t ax, uint16_t cx);  /* SX.OVL SBP:0x198d */
void     sbp_stop_all(uint16_t cx);                 /* SX.OVL SBP:0x1acd */
void     sbp_nop(void);                             /* SX.OVL SBP:0x1956 */
uint16_t sbp_query(uint16_t ax, uint16_t cx);       /* SX.OVL SBP:0x253d */
void     sbp_init(uint16_t off, uint16_t seg, uint16_t *ax, uint16_t *cx);
                                                    /* SX.OVL SBP:0x25aa */
void     sbp_describe_0(uint16_t *ax, uint16_t *cx); /* SX.OVL SBP:0x25dc */

/*
 * `ADL:`, the AdLib driver, in reconstruct/src/sxovl_adl.c. The OPL2 it
 * programs is hardware and is behind src/opl.h; this is only the driver.
 */
void     adl_write(uint16_t reg, uint16_t val);     /* SX.OVL ADL:0x208e */
void     adl_write_bd(void);                        /* SX.OVL ADL:0x216b */
void     adl_write_nts(void);                       /* SX.OVL ADL:0x2194 */
void     adl_write_level(uint16_t slot);            /* SX.OVL ADL:0x21ac */
void     adl_write_attack_decay(uint16_t slot);     /* SX.OVL ADL:0x2244 */
void     adl_write_sustain_release(uint16_t slot);  /* SX.OVL ADL:0x228a */
void     adl_write_feedback(uint16_t slot);         /* SX.OVL ADL:0x21f4 */
void     adl_write_mult(uint16_t slot);             /* SX.OVL ADL:0x22d0 */
void     adl_write_wave(uint16_t slot);             /* SX.OVL ADL:0x233e */
uint16_t adl_bend(uint16_t voice, uint16_t cx);     /* SX.OVL ADL:0x1eee */
void     adl_write_voice_level(uint16_t voice, uint16_t level);
                                                    /* SX.OVL ADL:0x1f45 */
void     adl_note(uint16_t voice, uint16_t cx, uint16_t dx);
                                                    /* SX.OVL ADL:0x1e23 */
void     adl_touch_voice(uint16_t voice);           /* SX.OVL ADL:0x1df4 */
void     adl_key_off(uint16_t voice);               /* SX.OVL ADL:0x1dc4 */
void     adl_load_patch(uint16_t voice, uint16_t at);   /* SX.OVL ADL:0x1fe1 */
void     adl_write_operator(uint16_t slot, uint16_t src, uint8_t connect);
                                                    /* SX.OVL ADL:0x2109 */
void     adl_reset(void);                           /* SX.OVL ADL:0x237d */
void     adl_default_operators(void);               /* SX.OVL ADL:0x20b5 */
void     adl_default_operator(uint16_t slot, uint16_t src);
                                                    /* SX.OVL ADL:0x20e2 */
uint16_t adl_alloc_voice(uint16_t ax);              /* SX.OVL ADL:0x1ad4 */
void     adl_key_on(uint16_t voice, uint16_t cx);   /* SX.OVL ADL:0x1d45 */
void     adl_nop(void);                             /* SX.OVL ADL:0x1951 */
void     adl_stop_note(uint16_t ax, uint16_t cx);   /* SX.OVL ADL:0x1952 */
void     adl_start_note(uint16_t ax, uint16_t cx);  /* SX.OVL ADL:0x1988 */
void     adl_ctl_volume(uint16_t ax, uint16_t cx);  /* SX.OVL ADL:0x1ca9 */
void     adl_ctl_pan(uint16_t ax, uint16_t cx);     /* SX.OVL ADL:0x1ce2 */
void     adl_ctl_sustain(uint16_t ax, uint16_t cx); /* SX.OVL ADL:0x1d15 */
void     adl_grant_voices(uint16_t ax, uint8_t cl);    /* SX.OVL ADL:0x1bec */
void     adl_release_voices(uint16_t ax, uint8_t cl);  /* SX.OVL ADL:0x1c30 */
void     adl_rebalance(void);                       /* SX.OVL ADL:0x1b92 */
void     adl_ctl_reserve(uint16_t ax, uint16_t cx); /* SX.OVL ADL:0x1b52 */
void     adl_controller(uint16_t ax, uint16_t cx);  /* SX.OVL ADL:0x19cf */
void     adl_program(uint16_t ax, uint16_t cx);     /* SX.OVL ADL:0x1a1b */
void     adl_pitch_bend(uint16_t ax, uint16_t cx);  /* SX.OVL ADL:0x1a29 */
uint16_t adl_param_345(uint16_t cl);                /* SX.OVL ADL:0x1a8d */
uint16_t adl_param_346(uint16_t cl);                /* SX.OVL ADL:0x1a68 */
uint16_t adl_param_349(uint16_t cl);                /* SX.OVL ADL:0x1abf */
void     adl_stop_all(void);                        /* SX.OVL ADL:0x1ad0 */
uint16_t adl_query(uint16_t ax, uint16_t cx);       /* SX.OVL ADL:0x23a7 */
void     adl_init(uint16_t off, uint16_t seg, uint16_t *ax, uint16_t *cx);
                                                    /* SX.OVL ADL:0x2414 */
void     adl_describe_0(uint16_t *ax, uint16_t *cx);   /* SX.OVL ADL:0x2446 */

/* The driver call: which loaded driver a function number goes to. Ours. */
void     driver_describe_0(uint16_t *ax, uint16_t *cx);
void     driver_describe_1(struct far_ptr drv,
                           uint16_t *ax, uint16_t *cx);
void     driver_stop_all(uint16_t cx);
void     driver_stop_note(uint16_t ax, uint16_t cx);
void     driver_start_note(uint16_t ax, uint16_t cx);
void     driver_nop(void);
void     driver_controller(uint16_t ax, uint16_t cx);
void     driver_program_change(uint16_t ax, uint16_t cx);
void     driver_pitch_bend(uint16_t ax, uint16_t cx);
uint16_t driver_param_349(uint16_t cl);
uint16_t driver_param_345(uint16_t cl);
uint16_t driver_param_346(uint16_t cl);
/* OURS: the driver's single entry by function number - the shape the hybrid
 * needs, because a guest call arrives as a number in BP. See sxovl.c. */
void     sx_driver_call(uint16_t fn, uint16_t *ax, uint16_t *cx, uint16_t es);

/*
 * `ASB:`, the digitised-sound module, in reconstruct/src/sxovl_asb.c. A module
 * is not a driver: the game loads one of each and this one plays sampled bytes
 * over the Sound Blaster's DMA channel while the driver plays notes.
 */
void     asb_dsp_write(uint8_t value);          /* SX.OVL ASB:0x0377 */
void     asb_dsp_write_timed(uint8_t value);    /* SX.OVL ASB:0x038a */
uint16_t asb_dsp_write_try(uint8_t value);      /* SX.OVL ASB:0x070e */
uint8_t  asb_dsp_read_try(uint16_t *failed);    /* SX.OVL ASB:0x072a */
uint8_t  asb_dsp_read(void);                    /* SX.OVL ASB:0x0749 */
void     asb_dma_pause(void);                   /* SX.OVL ASB:0x0369 */
void     asb_dma_continue(void);                /* SX.OVL ASB:0x0370 */
void     asb_speaker_on(void);                  /* SX.OVL ASB:0x079e */
void     asb_set_rate(uint16_t rate);           /* SX.OVL ASB:0x031b */
void     asb_set_block_size(uint16_t n);        /* SX.OVL ASB:0x033a */
uint32_t asb_linear(struct far_ptr h);   /* SX.OVL ASB:0x0355 */
void     asb_dma_program(uint16_t off, uint16_t count,
                         uint8_t mode, uint8_t page);  /* SX.OVL ASB:0x08ec */
void     asb_dma_start(void);                   /* SX.OVL ASB:0x025d */
void     asb_arm_block(void);                   /* SX.OVL ASB:0x0224 */
void     asb_dma_stop(void);                    /* SX.OVL ASB:0x02a9 */
void     asb_isr(void);                         /* SX.OVL ASB:0x02b7 */
uint8_t  asb_hook_irq(uint8_t irq, uint16_t save_at,
                      uint16_t handler);        /* SX.OVL ASB:0x03a5 */
void     asb_unhook_irq(uint8_t irq, uint16_t save_at,
                        uint8_t mask_was);      /* SX.OVL ASB:0x03f6 */
uint16_t asb_probe_reset(void);                 /* SX.OVL ASB:0x06c0 */
uint16_t asb_probe_identify(void);              /* SX.OVL ASB:0x06eb */
uint16_t asb_probe_version(void);               /* SX.OVL ASB:0x075d */
void     asb_probe_isr_2(void);                 /* SX.OVL ASB:0x0915 */
void     asb_probe_isr_3(void);                 /* SX.OVL ASB:0x091e */
void     asb_probe_isr_5(void);                 /* SX.OVL ASB:0x0927 */
void     asb_probe_isr_7(void);                 /* SX.OVL ASB:0x0930 */
void     asb_probe_isr_10(void);                /* SX.OVL ASB:0x0939 */
uint16_t asb_probe_irq(void);                   /* SX.OVL ASB:0x07c5 */
uint16_t asb_try_base(uint16_t base);           /* SX.OVL ASB:0x069c */
uint16_t asb_detect(void);                      /* SX.OVL ASB:0x0665 */
void     asb_int10_hook(void);                  /* SX.OVL ASB:0x052b */
void     asb_int0d_hook(void);                  /* SX.OVL ASB:0x053e */
void     asb_int74_hook(void);                  /* SX.OVL ASB:0x0551 */
void     asb_int09_hook(void);                  /* SX.OVL ASB:0x0564 */
uint8_t  asb_safe_to_call(void);                /* SX.OVL ASB:0x0506 */
uint16_t asb_shutdown(void);                    /* SX.OVL ASB:0x00f5 */
void     asb_play(volatile uint8_t * si);                 /* SX.OVL ASB:0x011e */
uint16_t asb_status(void);                      /* SX.OVL ASB:0x01be */
void     asb_stop(void);                        /* SX.OVL ASB:0x01ce */
uint16_t asb_uninstall(void);                   /* SX.OVL ASB:0x01d2 */
uint16_t asb_set_rate_fn(volatile uint8_t * si);          /* SX.OVL ASB:0x00de */
uint16_t asb_clear_49(void);                    /* SX.OVL ASB:0x00ec */
uint16_t asb_position(volatile uint8_t * si);             /* SX.OVL ASB:0x0435 */
uint16_t asb_install(void);                     /* SX.OVL ASB:0x0577 */
uint16_t asb_dispatch(uint16_t fn, volatile uint8_t * si);   /* SX.OVL ASB:0x00c8 */

/* Resolve one object against everything it could be touching. */
int16_t resolve_collisions(uint16_t obj);           /* 0x00556 */

/* Sweep one object's edges against another's; record the contact. */
int16_t find_edge_contact(int16_t test_only);       /* 0x007af */

/* Advance an object one step: velocity, gravity, clamp, place. */
void integrate_object(struct part *obj);                /* 0x02c93 */

/* Work out where an object is drawn, at +0x2a/+0x2c. */
uint16_t clone_part(struct part *part);                 /* 0x059e4 */
void place_object_for_draw(struct part *obj);           /* 0x05be4 */

/* Add shape records for a sub-object's point pairs. */
void add_sub_object_shapes(struct part *obj, int16_t mask);  /* 0x05ef6 */

/* Set an object's extent at +0x44/+0x46 from its kind. */
void set_object_extent(struct part *obj);               /* 0x05c77 */

/* Angle from two differences across an object's +0x1e/+0x22 fields. */
int16_t object_delta_angle(struct part *obj);           /* 0x004ab */

/* Arctangent table lookup; index is a ratio in 0..511. */
int16_t arctan_lookup(uint16_t index);              /* 0x2a941 */

/* Apply contact friction to an object. */
void apply_contact_friction(struct part *obj);          /* 0x02da0 */

/* Read one pixel's colour from the source page; no clipping. */
uint16_t vm_driver_init(uint16_t data_delta, uint16_t params,
                        uint16_t ds);           /* VM.OVL VGA:0x0000 */
void vm_reset_attributes(void);                     /* VM.OVL VGA:0x011d */
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
int16_t claim_buffer_slot(int32_t a, int32_t b);        /* 0x0b5ed */

/* Clear one byte of the one-based four-entry array at 0x5734. */
void clear_slot_5734(int16_t n);                    /* 0x0b69c */

/* Make a resource file the open one, closing whatever was open before. */
void make_file_current(uint16_t index);             /* 0x09a62 */

/* Put a file at a position, without asking DOS if it is already there. */
void seek_file_to(uint32_t at);                     /* 0x09b38 */

/* The archive entry standing in for an open file, or null for a real one. */
uint16_t archive_entry_for(uint16_t file);          /* 0x09b7c */
int16_t game_fseek(uint16_t file, int32_t off,
                   int16_t whence);                 /* 0x092dc */
uint32_t fread_huge(struct far_ptr dst, uint32_t size, uint32_t count,
                    uint16_t file);                 /* 0x0b93d */
int32_t game_ftell(uint16_t file);                  /* 0x093a2 */
int16_t game_fgetc(uint16_t file);                  /* 0x093f6 */
int16_t game_fclose(uint16_t file);                 /* 0x0917f */
void game_rewind(uint16_t file);                    /* 0x093e0 */
int16_t  answer_carry_on(uint16_t what);            /* 0x08fc3 */
uint16_t game_fopen(volatile uint8_t * name, const volatile uint8_t * mode);   /* 0x08fcd */
void load_archive_map(void);                        /* 0x0960f */
int32_t hash_filename(volatile uint8_t * name);               /* 0x0980d */
uint16_t game_fread(volatile uint8_t * buf, uint16_t size, uint16_t count,
                    uint16_t file);                 /* 0x091ef */

/* Zero the word at DGROUP 0x2d44; meaning not established. */
void clear_flag_2d44(void);                         /* 0x0a7a3 */
void clear_flag_2d44_thunk(void);                   /* 0x0811b */

/* Borland's near heap - NOT part of the reconstruction, see borland_heap.c. */
int16_t brk_set(uint16_t addr);                     /* 0x0c7c4 */
void    heap_ring_unlink(uint16_t bx);              /* 0x0c95a */
void    heap_ring_insert(uint16_t bx);              /* 0x0c976 */
void    heap_free_middle(uint16_t bx);              /* 0x0c921 */
void    heap_free_top(uint16_t bx);                 /* 0x0c8e7 */
void    heap_free(uint16_t p);                      /* 0x0c8ca */
uint16_t heap_sbrk(uint16_t lo, uint16_t hi);       /* 0x0c7e6 */
uint16_t heap_init(uint16_t size);                  /* 0x0c9f9 */
uint16_t heap_grow(uint16_t size);                  /* 0x0ca39 */
uint16_t heap_split(uint16_t bx, uint16_t size);    /* 0x0ca62 */
void far_move(const volatile uint8_t far * src, volatile uint8_t far * dst, uint16_t count);    /* 0x0bd2e */
uint32_t long_multiply(uint32_t a, uint32_t b);      /* 0x0c16e */
uint32_t ulong_divide(uint32_t a, uint32_t b);       /* 0x0bd97 */
int32_t long_divide(int32_t a, int32_t b);           /* 0x0bd93 */
void read_far(uint8_t far *dst, int32_t count,
              uint16_t file);                        /* 0x2551a */
void decode_vqt_list(uint16_t file, uint16_t list); /* 0x25639 */
void vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h);   /* 0x25db8 */
void fill_quadrant(uint16_t x, uint16_t y,
                   uint16_t w, uint16_t h);         /* 0x25eb5 */
uint16_t near_memset(uint16_t dst, uint16_t count,
                     uint16_t value);               /* 0x0d543 */
uint16_t heap_calloc(uint16_t count, uint16_t size); /* 0x0c833 */
uint16_t heap_calloc_far(uint16_t count, uint16_t size); /* 0x0bb75 */
volatile uint8_t *  heap_malloc_far(uint16_t bytes);            /* 0x0bb1e */
/* `buf` is written through and handed back; the guest passes and expects a
   DGROUP offset, which the shim converts in both directions. */
volatile uint8_t *  int_to_string(int16_t value, volatile uint8_t * buf,
                       uint16_t radix);             /* 0x0d4bd */
volatile uint8_t *  long_int_to_string(uint16_t lo, uint16_t hi, volatile uint8_t * buf,
                            uint16_t radix);        /* 0x0d4ff */
void draw_odometer_digit(char c, int16_t x, int16_t y); /* 0x15a7e */
void set_clip_counter_strip(void);                  /* 0x026e8 */
void draw_counter_word(int16_t value, int16_t x, int16_t y,
                       int16_t all);                /* 0x0262b */
void draw_counter_long(int32_t value, int16_t x, int16_t y,
                       int16_t all);                /* 0x02686 */
void redraw_counters(void);                         /* 0x025d8 */
void start_counters(void);                          /* 0x024fa */
int32_t parse_base(volatile uint8_t * text, int16_t base);    /* 0x02a34 */
void score_to_code(int32_t score, volatile uint8_t * text); /* 0x02809 */
int32_t score_code_to_score(uint16_t text);         /* 0x02900 */
void step_counters(void);                           /* 0x02510 */
volatile uint8_t *  long_to_string(uint16_t letters, uint16_t is_signed,
                        uint16_t radix, volatile uint8_t * buf, uint16_t lo,
                        uint16_t hi);               /* 0x0c029 */
uint16_t heap_malloc(uint16_t want);                /* 0x0c999 */

/* Borland's DOS file primitives - NOT part of the reconstruction. */
int16_t dos_read(int16_t handle, volatile uint8_t * buf, uint16_t count);   /* 0x0c185 */
int32_t dos_lseek(int16_t handle, uint16_t lo, uint16_t hi,
                  int16_t whence);                  /* 0x0c0c3 */
int16_t read_translated(int16_t handle, uint16_t buf,
                        uint16_t count);            /* 0x0da6d */
void    flush_all_streams(void);                    /* 0x0d36d */
int16_t refill_stream(uint16_t file);               /* 0x0d396 */
int16_t stdio_fgetc(uint16_t file);                 /* 0x0d404 */
int16_t flush_stream(uint16_t file);                /* 0x0ce92 */
int32_t dos_tell(int16_t handle);                   /* 0x0c27b */
int16_t dos_isatty(int16_t handle);                 /* 0x0c018 */
int16_t dos_ioctl(int16_t handle, uint16_t al, uint16_t dx,
                  uint16_t cx);                     /* 0x0c8a3 */
int16_t dos_getattr(const volatile uint8_t * name, uint16_t al, uint16_t cx); /* 0x0cd3d */
int16_t dos_open_named(const volatile uint8_t * name, uint16_t flags); /* 0x0d707 */
int16_t parse_open_mode(volatile uint8_t * out_perm, volatile uint8_t * out_flags,
                        const volatile uint8_t * mode);             /* 0x0cf4d */
int16_t stdio_setvbuf(uint16_t file, uint16_t buf, int16_t mode,
                      uint16_t size);               /* 0x0db5e */
uint16_t find_free_stream(void);                    /* 0x0d0a3 */
uint16_t stdio_fopen_into(uint16_t extra_flags, const volatile uint8_t * mode, const volatile uint8_t * name,
                          uint16_t file);           /* 0x0d007 */
uint16_t stdio_fopen(const volatile uint8_t * name, const volatile uint8_t * mode); /* 0x0d0ce */
uint32_t long_shift_left(uint32_t v, uint8_t count);  /* 0x0be3e */
int16_t io_error(int16_t code);                     /* 0x0bfcd */
uint16_t call_sound_module(uint16_t fn, volatile uint8_t * si);   /* 0x0bbd4 */
uint16_t sound_module_install(uint16_t callback, uint16_t flag); /* 0x0bb98 */
uint16_t sound_module_set_rate(volatile uint8_t * si);        /* 0x0bb9f */
uint16_t sound_module_service(volatile uint8_t * si);         /* 0x0bba6 */
uint16_t sound_module_9(volatile uint8_t * si);               /* 0x0bbb1 */
uint16_t sound_module_10(volatile uint8_t * si);              /* 0x0bbb8 */
uint16_t sound_module_11(volatile uint8_t * si);              /* 0x0bbbf */
uint16_t stop_loaded_module(void);                  /* 0x0bbc6 */
uint16_t sound_module_shutdown(void);               /* 0x0bbcd */
uint16_t sound_module_position(uint16_t *a, uint16_t *b, uint16_t *c);
                                                    /* 0x0bbe6 */
uint32_t dos_getvect(uint16_t n);                   /* 0x0bd70 */
void dos_setvect(uint16_t n, uint16_t off, uint16_t seg); /* 0x0bd7f */
volatile uint8_t *  string_copy(volatile uint8_t * dst, const volatile uint8_t * src);    /* 0x0dd33 */
uint16_t string_length(const volatile uint8_t * s);                 /* 0x0dd95 */
volatile uint8_t * string_reverse(volatile uint8_t * s);                /* 0x0de1e */
volatile uint8_t * string_upper(volatile uint8_t * s);                  /* 0x0de4e */
int16_t  string_ncompare_i(uint16_t a, uint16_t b,
                           uint16_t n);             /* 0x0dddb */
volatile uint8_t *  string_chr(volatile uint8_t * s, uint8_t c);          /* 0x0dcce */
int16_t  string_compare(const volatile uint8_t * a, const volatile uint8_t * b);    /* 0x0dd04 */
uint16_t string_copy_far(uint16_t dst, uint16_t src); /* 0x0bb4f */
int16_t string_compare_nocase(const volatile uint8_t * a, const volatile uint8_t * b); /* 0x0dd55 */
volatile uint8_t * string_copy_padded(volatile uint8_t * dst, const volatile uint8_t * src,
                            uint16_t n);            /* 0x0ddaf */
int16_t open_file(const volatile uint8_t * name, uint16_t flags,
                  uint16_t perm);                   /* 0x0d5af */
int16_t dos_close(int16_t handle);                  /* 0x0cd80 */
volatile uint8_t *  mem_copy(volatile uint8_t * dst, const volatile uint8_t * src, uint16_t n); /* 0x0d524 */
int16_t dos_write(int16_t handle, const volatile uint8_t * buf, uint16_t count); /* 0x0df7a */
int16_t dos_creat(const volatile uint8_t * name, uint16_t attr);    /* 0x0d584 */
void    dos_truncate(int16_t handle);               /* 0x0d59d */
int16_t close_handle(int16_t handle);               /* 0x0cd58 */
int16_t stdio_fclose(uint16_t file);                /* 0x0ce15 */
int16_t unread_count(uint16_t file);                /* 0x0d20f */
int32_t stdio_ftell(uint16_t file);                 /* 0x0d2d4 */
int16_t stdio_fseek(uint16_t file, int32_t off,
                    int16_t whence);                /* 0x0d26c */
int16_t stdio_getc(uint16_t file);                  /* 0x0d3ef */
uint16_t buffered_read(uint16_t file, uint16_t count,
                       volatile uint8_t * buf);               /* 0x0d0ed */
uint16_t stdio_fread(volatile uint8_t * buf, uint16_t size, uint16_t count,
                     uint16_t file);                /* 0x0d1c4 */

/* Hand over the next run of bytes from the selected resource. */
void resource_advance(void);                        /* 0x1c8a7 */

/* Select a resource by handle; unpack its entry into the loader globals. */
int16_t select_resource(int16_t handle);            /* 0x1c649 */
int16_t close_resource_slot(uint16_t slot);         /* 0x1c71a */
uint16_t find_file_record(uint16_t handle);         /* 0x23df2 */
uint32_t file_record_size(uint16_t handle);         /* 0x242af */
int16_t file_record_valid(uint16_t handle);         /* 0x24308 */
int16_t close_file_record(uint16_t handle);         /* 0x242d9 */
void reset_file_record(uint16_t rec);               /* 0x23e23 */
int16_t string_equal_upto(const char * a, const char * b,
                          uint16_t n);              /* 0x23e70 */
volatile uint8_t *  copy_file_record(volatile uint8_t * dst, uint16_t handle); /* 0x23ea8 */
uint16_t open_file_record(volatile uint8_t * name);           /* 0x23f2c */
uint32_t restore_file_record(uint16_t rec);         /* 0x23f90 */
uint32_t seek_named_chunk(uint16_t handle, const char * path,
                          int16_t index);           /* 0x23fc2 */
int16_t open_resource_slot(void);                   /* 0x1c783 */
int16_t prepare_resource_slot(int16_t type,
                              uint16_t name);       /* 0x1c7d5 */

/* Free a pointer unless it is null. */
void free_if_set(uint16_t p);                       /* 0x1c705 */

/* Hand a block back to DOS; only the pointer's segment is used. */
void dos_free_far(struct far_ptr block);            /* 0x21b34 */

/* Recompute a link's endpoints, then the rest lengths they imply. */
void refresh_link_geometry(uint16_t link);          /* 0x04f7f */

/* Set an object's vector at +0x36/+0x38 from angle and magnitude. */
void set_vector_from_angle(struct part *obj, uint16_t angle,
                           int16_t mag);            /* 0x07223 */

/* Rest length less actual separation, at one end of a link. */
int16_t link_slack(struct part *obj, uint16_t link,
                   int16_t gen);                    /* 0x0713d */

/* The vector a link has to close, and its approximate length. */
int16_t link_endpoint_gap(uint16_t link, struct part *obj, volatile uint8_t * out_dx,
                          volatile uint8_t * out_dy);         /* 0x07947 */

/* Distance from a link's endpoint to the endpoint it joins. */
int16_t link_end_distance(uint16_t link, int16_t gen,
                          int16_t end);             /* 0x06f8e */

/* Age the state histories of everything about to be stepped. */
void shift_all_histories(void);                     /* 0x07ca2 */

/* Age every tracked quantity on an object by one step. */
void shift_state_history(struct part *obj);             /* 0x07ce3 */

/* Classify a link's endpoints against the ones they connect to. */
int16_t compare_link_ends(uint16_t link, int16_t end,
                          int16_t reversed);        /* 0x06de9 */

/* Intersect two segments; answers whether the point lies on both. */
int16_t intersect_segments(const volatile uint8_t * seg1, const volatile uint8_t * seg2,
                           volatile uint8_t * out);            /* 0x03ba9 */

/* Step the second word of each pair one further from the first. */
void step_pair_apart(volatile uint8_t * rec);                  /* 0x03d2e */

/* Are two points within 140 in both axes? */
int16_t points_within_140(const struct point16 *a,
                          const struct point16 *b);      /* 0x04b53 */

/* Recompute a record's velocity from its movement, then clamp it. */
void update_velocity(struct part *rec, uint8_t shift_x, uint8_t shift_y,
                     uint16_t which);               /* 0x07283 */

/* Splice one list onto the front of another and empty the first. */
void splice_list_4e58_onto_4e56(void);              /* 0x07b3e */

/* Is a value between two bounds, whichever way round they are? */
int16_t value_between(uint16_t v, uint16_t a, uint16_t b);   /* 0x03d67 */

/* Work out the endpoints of the link between a pair of objects. */
void compute_link_endpoints(uint16_t link);         /* 0x04e65 */

/* Which side of a range a value falls on, as two flag bytes. */
void set_side_flags(const volatile uint8_t * range, int16_t v, volatile uint8_t * out);   /* 0x004fd */

/* Insert a record into a sorted doubly-linked list. */
void insert_sorted(struct part *rec, uint16_t head);    /* 0x05646 */

/* First of three words that is non-zero and enabled by its flag bit. */
int16_t pick_by_flag(uint16_t flags);               /* 0x05b65 */

/* Choose a value for a record: its own, or a shared slot. */
int16_t pick_for_record(uint16_t rec, uint16_t flags);    /* 0x05ba7 */

/* Add one or both of a record's two shapes. */
void add_record_shapes(struct part *rec, uint16_t which);   /* 0x0642a */

/* Take a node off the free list and fill it in as a shape. */
/* pt1 and pt2 are each an (x, y) pair the routine only reads, so they are
   pointers: a caller's stack locals in some places and a DGROUP record's
   fields in others, and a pointer is the one type that is both. */
void alloc_shape(const volatile uint8_t *pt1, const volatile uint8_t *pt2,
                 uint8_t flags, uint8_t which,
                 int16_t width);                    /* 0x064b4 */

/* Which of two structure fields matches a value. */
int16_t match_field_5a_5c(int16_t value, struct part *obj);   /* 0x06f43 */

/* Pick one of two record fields by matching the other. */
int16_t select_field_2_or_4(int16_t key, uint16_t rec);   /* 0x06f68 */

/* Present the frame: the game's wrapper around the driver's page flip. */
void present_frame(uint16_t wait_retrace);          /* 0x081cc */

/* Clear the input accumulators and latched state. */
void reset_input_state(void);                       /* 0x0b4f1 */

/* Claim a slot in the two-entry page table at DGROUP 0x56e6. */
uint16_t claim_page_slot(uint16_t want);            /* 0x0b429 */
void swap_page_objects(uint16_t page_a, uint16_t page_b); /* 0x0ae8e */

/* Save the driver's drawing state, or put it back. */
void save_or_restore_draw_state(int16_t save);      /* 0x0b47f */

/* Wait for the frame, then latch input state and clear the accumulators. */
void wait_and_latch_frame(void);                    /* 0x0aaca */

/* Not transcribed yet; see the source. */

uint16_t load_screen(uint16_t name);                /* 0x253e7 */
void     keyboard_isr(void);                        /* 0x21196 */
uint16_t bios_read_key(void);                       /* 0x21434 */
void copy_rect_thunk(uint16_t x, uint16_t y, uint16_t width,
                     uint16_t height);              /* 0x21088 */
void step_and_draw_machine(int16_t redraw_all);     /* 0x16181 */
void refile_overlapping_parts(void);                /* 0x06b5b */
void draw_machine(int16_t a, int16_t b);            /* 0x1675e */
void draw_rope(struct part *part, int16_t a);           /* 0x167fa */
void draw_curve(uint8_t colour, int16_t shift,
                int32_t x0, int32_t x1, int32_t x2,
                int32_t y0, int32_t y1, int32_t y2); /* 0x1697d */
void draw_belt_segment(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t slack);              /* 0x16b39 */
void draw_belt(struct part *part, int16_t a);           /* 0x16baf */
void draw_part(struct part *part, int16_t level,
               int16_t a, int16_t b);               /* 0x16db1 */
void draw_part_extra(struct part *part);                /* 0x171b5 */
void draw_polygon(int16_t n, const int16_t *xs,
                  const int16_t *ys);                  /* 0x1eded */
void draw_bitmap_scaled(uint16_t hdr, int16_t x, int16_t y,
                        int16_t w, int16_t h,
                        uint16_t mode);             /* 0x0b9c9 */
void blit_scaled_a(uint16_t hdr, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h); /* 0x227ac */
void blit_scaled_b(uint16_t hdr, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h); /* 0x208f3 */
uint16_t find_part_from(uint16_t rec);              /* 0x04500 */
int16_t  rope_ends_close(uint16_t rope);            /* 0x04b8f */
int16_t  point_in_play_area(void);                  /* 0x080b9 */
void draw_bitmap_centred(uint16_t bmp, int16_t x, int16_t y,
                         int16_t w, int16_t h); /* 0x15f76 */
void draw_panel(int16_t x, int16_t y, int16_t w, int16_t h); /* 0x151c8 */
void draw_scroll_text(const volatile uint8_t * str, int16_t x, int16_t y, int16_t w); /* 0x15004 */
void show_level_complete(void);                      /* 0x158c5 */
void free_all_lists(void);                          /* 0x14d43 */
void free_part_list(struct part *p);                    /* 0x14d71 */
uint16_t load_animation(uint16_t name);             /* 0x12915 */
uint16_t game_fread_byte(uint16_t file, volatile uint8_t * buf); /* 0x11db4 */
void game_fread_line(uint16_t file, volatile uint8_t * buf);  /* 0x11e0b */
void read_password_line(int16_t count, volatile uint8_t * buf); /* 0x12b60 */
void stdio_setbuf_for(uint16_t file, uint16_t buf);  /* 0x095cf */
void game_fread_string(uint16_t file, volatile uint8_t * buf);/* 0x11dec */
void alloc_part_table(int16_t n);                   /* 0x11d66 */
void read_list(uint16_t file, uint16_t head, int16_t n);   /* 0x1221b */
void read_record_fields(uint16_t file, struct part *rec);      /* 0x11e3f */
void build_part_list(void);                         /* 0x1405b */
void free_two_bitmap_lists(void);                   /* 0x0efdc */
void free_all_part_bitmaps(void);                   /* 0x0f86e */
void free_part_bitmap(uint16_t n);                  /* 0x0f886 */
void load_part_bitmap(uint16_t n);                  /* 0x0f7f4 */
uint16_t part_init(uint32_t at, struct part *part);     /* OURS: by address */
uint16_t part_init_bowling_ball(struct part *part);           /* 0dff:6246, 0x14236 */
uint16_t part_init_14267(struct part *part);           /* 0dff:6277, 0x14267 */
uint16_t part_init_ramp(struct part *part);           /* 0dff:62b1, 0x142a1 */
uint16_t part_init_seesaw(struct part *part);           /* 0dff:62f6, 0x142e6 */
uint16_t part_init_balloon(struct part *part);           /* 0dff:6330, 0x14320 */
uint16_t part_init_conveyor(struct part *part);           /* 0dff:6371, 0x14361 */
uint16_t part_init_mouse_cage(struct part *part);           /* 0dff:63c3, 0x143b3 */
uint16_t part_init_pulley(struct part *part);           /* 0dff:640b, 0x143fb */
uint16_t part_init_belt(struct part *part);           /* 0dff:644d, 0x1443d */
uint16_t part_init_basketball(struct part *part);           /* 0dff:647c, 0x1446c */
uint16_t part_init_rope(struct part *part);           /* 0dff:64ad, 0x1449d */
uint16_t part_init_bird_cage(struct part *part);           /* 0dff:64db, 0x144cb */
uint16_t part_init_pokey(struct part *part);           /* 0dff:651c, 0x1450c */
uint16_t part_init_jack_in_the_box(struct part *part);           /* 0dff:6557, 0x14547 */
uint16_t part_init_gear(struct part *part);           /* 0dff:659f, 0x1458f */
uint16_t part_init_bob_the_fish(struct part *part);           /* 0dff:65e1, 0x145d1 */
uint16_t part_init_bellow(struct part *part);           /* 0dff:6617, 0x14607 */
uint16_t part_init_bucket(struct part *part);           /* 0dff:664d, 0x1463d */
uint16_t part_init_cannon(struct part *part);           /* 0dff:668e, 0x1467e */
uint16_t part_init_dynamite(struct part *part);           /* 0dff:66cd, 0x146bd */
uint16_t part_init_146fc(struct part *part);           /* 0dff:670c, 0x146fc */
uint16_t part_init_electric_plug(struct part *part);           /* 0dff:673d, 0x1472d */
uint16_t part_init_dynamite_plunger(struct part *part);           /* 0dff:677c, 0x1476c */
uint16_t part_init_hook(struct part *part);           /* 0dff:67b7, 0x147a7 */
uint16_t part_init_fan(struct part *part);           /* 0dff:67d5, 0x147c5 */
uint16_t part_init_flashlight(struct part *part);           /* 0dff:6814, 0x14804 */
uint16_t part_init_generator(struct part *part);           /* 0dff:684a, 0x1483a */
uint16_t part_init_gun(struct part *part);           /* 0dff:6884, 0x14874 */
uint16_t part_init_baseball(struct part *part);           /* 0dff:68bf, 0x148af */
uint16_t part_init_light(struct part *part);           /* 0dff:68f0, 0x148e0 */
uint16_t part_init_magnifying_glass(struct part *part);           /* 0dff:690f, 0x148ff */
uint16_t part_init_monkey(struct part *part);           /* 0dff:6929, 0x14919 */
uint16_t part_init_pumpkin(struct part *part);           /* 0dff:6964, 0x14954 */
uint16_t part_init_heart_balloon(struct part *part);           /* 0dff:6995, 0x14985 */
uint16_t part_init_christmas_tree(struct part *part);           /* 0dff:69d6, 0x149c6 */
uint16_t part_init_boxing_glove(struct part *part);           /* 0dff:6a07, 0x149f7 */
uint16_t part_init_rocket(struct part *part);           /* 0dff:6a3d, 0x14a2d */
uint16_t part_init_scissors(struct part *part);           /* 0dff:6a77, 0x14a67 */
uint16_t part_init_solar_panel(struct part *part);           /* 0dff:6ab2, 0x14aa2 */
uint16_t part_init_trampoline(struct part *part);           /* 0dff:6ac9, 0x14ab9 */
uint16_t part_init_windmill(struct part *part);           /* 0dff:6aff, 0x14aef */
uint16_t part_init_mort_the_mouse(struct part *part);           /* 0dff:6b47, 0x14b37 */
uint16_t part_init_cannon_ball(struct part *part);           /* 0dff:6b82, 0x14b72 */
uint16_t part_init_tennis_ball(struct part *part);           /* 0dff:6bb3, 0x14ba3 */
uint16_t part_init_candle(struct part *part);           /* 0dff:6be4, 0x14bd4 */
uint16_t part_init_corner_pipe(struct part *part);           /* 0dff:6c22, 0x14c12 */
uint16_t part_init_14c48(struct part *part);           /* 0dff:6c58, 0x14c48 */
uint16_t part_init_motor(struct part *part);           /* 0dff:6c72, 0x14c62 */
uint16_t part_init_14ca0(struct part *part);           /* 0dff:6cb0, 0x14ca0 */
uint16_t part_init_14cd9(struct part *part);           /* 0dff:6ce9, 0x14cd9 */
uint16_t part_init_14d0a(struct part *part);           /* 0dff:6d1a, 0x14d0a */
void part_setup(uint16_t off, struct part *part);       /* segment 172c */
void part_finish(uint16_t off, struct part *part);
void part_finish_angles(struct part *part);             /* 0x05d1e */
void part_setup_boxing_glove(struct part *part);                /* 172c:065b, 0x1791b */
void part_setup_10b6(struct part *part);                /* 172c:10b6, 0x18376 */
void part_setup_1105(struct part *part);                /* 172c:1105, 0x183c5 */
void part_setup_motor(struct part *part);                /* 172c:1435, 0x186f5 */
void part_setup_electric_plug(struct part *part);                /* 172c:1556, 0x18816 */
void part_setup_gear(struct part *part);                /* 172c:2068, 0x19328 */
void part_setup_light(struct part *part);                /* 172c:2b58, 0x19e18 */
void part_setup_solar_panel(struct part *part);                /* 172c:3de5, 0x1b0a5 */
void part_setup_0001(struct part *part);                /* 172c:0001, 0x172c1 */
void part_setup_cannon_ball(struct part *part);                /* 172c:0065, 0x17325 */
void part_setup_00c9(struct part *part);                /* 172c:00c9, 0x17389 */
void part_setup_bucket(struct part *part);                /* 172c:07b2, 0x17a72 */
void part_setup_candle(struct part *part);                /* 172c:0950, 0x17c10 */
void part_setup_bird_cage(struct part *part);                /* 172c:0f70, 0x18230 */
void part_setup_conveyor(struct part *part);                /* 172c:24d0, 0x19790 */
void part_setup_jack_in_the_box(struct part *part);                /* 172c:295d, 0x19c1d */
void part_setup_mouse_cage(struct part *part);                /* 172c:2ee1, 0x1a1a1 */
void part_setup_mort_the_mouse(struct part *part);                /* 172c:346f, 0x1a72f */
void part_setup_rocket(struct part *part);                /* 172c:3737, 0x1a9f7 */
void part_setup_trampoline(struct part *part);                /* 172c:3f72, 0x1b232 */
void part_setup_48ab(struct part *part);                /* 172c:48ab, 0x1bb6b */
void part_setup_windmill(struct part *part);                /* 172c:496f, 0x1bc2f */
void part_setup_balloon(struct part *part);                /* 172c:012d, 0x173ed */
void part_setup_bellow(struct part *part);                /* 172c:0371, 0x17631 */
void part_setup_08a1(struct part *part);                /* 172c:08a1, 0x17b61 */
void part_setup_cannon(struct part *part);                /* 172c:0b88, 0x17e48 */
void part_setup_gun(struct part *part);                /* 172c:23b1, 0x19671 */
void part_setup_dynamite_plunger(struct part *part);                /* 172c:3294, 0x1a554 */
void part_setup_dynamite(struct part *part);                /* 172c:1261, 0x18521 */
void part_setup_hook(struct part *part);                /* 172c:19db, 0x18c9b */
void part_setup_monkey(struct part *part);                /* 172c:2cce, 0x19f8e */
void part_setup_pokey(struct part *part);                /* 172c:0c1c, 0x17edc */
void part_setup_fan(struct part *part);                /* 172c:1a32, 0x18cf2 */
void part_setup_bob_the_fish(struct part *part);                /* 172c:1be9, 0x18ea9 */
void part_setup_flashlight(struct part *part);                /* 172c:1d28, 0x18fe8 */
void part_setup_generator(struct part *part);                /* 172c:1dfb, 0x190bb */
void part_setup_heart_balloon(struct part *part);                /* 172c:2682, 0x19942 */
void part_setup_corner_pipe(struct part *part);                   /* 172c:377b, 0x1aa3b */
void part_setup_ramp(struct part *part);                          /* 172c:2728, 0x199e8 */
void part_setup_pumpkin(struct part *part);                /* 172c:35f4, 0x1a8b4 */
void part_setup_scissors(struct part *part);                /* 172c:389b, 0x1ab5b */
void part_setup_christmas_tree(struct part *part);                /* 172c:1075, 0x18335 */
void part_setup_seesaw(struct part *part);                /* 172c:40f0, 0x1b3b0 */
struct part *make_part(uint16_t kind);                     /* 0x14133 */
void free_part(struct part *part);                      /* 0x14d95 */
void load_all_parts(void);                          /* 0x0f7b6 */
void draw_frame_corners(uint16_t rec);              /* 0x0ee6e */
void draw_answer_slot(uint16_t bmp, uint16_t slot);  /* 0x0edf1 */
void redraw_cursor_all(void);                       /* 0x0b078 */
uint16_t copy_protect_screen(uint16_t bitmaps);                         /* 0x0ea39 */
void restore_object_backdrop(uint16_t from_page,
                             uint16_t to_page);      /* 0x0adf1 */
void restore_saved_rect_lists(int16_t which);       /* 0x0a42a */
void restore_saved_rects(uint16_t page_src, uint16_t page_dst, uint16_t refcount); /* 0x0a62c */
void free_saved_rects(uint16_t page_src, uint16_t page_dst, uint16_t refcount); /* 0x0a6d7 */
uint16_t find_saved_rect_slot(uint16_t page_src, uint16_t page_dst,
                              uint16_t refcount);        /* 0x0a5e2 */
char far *far_strchr(const char far *s, char c);                  /* 0x09fc0 */
char far *far_strcat(char far *dst, const char far *src);         /* 0x0a005 */
uint16_t build_rect_pool(uint16_t n);                             /* 0x0a05f */
void     file_saved_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t mode, uint16_t page_src, uint16_t page_dst,
                         uint16_t refcount, struct far_ptr buf);  /* 0x0a0d7 */
void     discard_saved_rects(void);                               /* 0x0a4bf */
uint16_t saved_rect_covers(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint16_t page_dst, uint16_t refcount); /* 0x0a4f9 */
void     free_rect_pool(void);                                    /* 0x0a5a1 */
uint16_t rect_pool_count(void);                                   /* 0x0a5d8 */
void     copy_saved_rects(uint16_t from_src, uint16_t from_dst, uint16_t from_ref,
                          uint16_t to_src, uint16_t to_dst, uint16_t to_ref); /* 0x0a717 */
void clear_object_covered(uint16_t page);           /* 0x0aedc */
void copy_rect_around_cursor(int16_t x, int16_t y,
                             int16_t w, int16_t h); /* 0x0b28e */
void move_pointer_to(int16_t x, int16_t y);         /* 0x0aa76 */
void vm_set_line_compare(uint16_t line);                         /* 0x08f27 */
void regions_handle_pointer(uint16_t first);        /* 0x08546 */

/*
 * OURS: the port cannot call through a far pointer held in guest memory, so a
 * region's two handlers are dispatched on their value. See machine.c.
 */
void call_region_handler(struct far_ptr h, uint16_t region);
void stop_music_or_effect(int16_t id);              /* 0x083ea */
void play_sound(int16_t id);                        /* 0x083ab */
void select_music(int16_t id);                      /* 0x08364 */
void restore_cursor_following(void);                /* 0x08125 */
void show_cursor_again(void);                               /* 0x0810b */
void reset_machine(void);                           /* 0x07e45 */
void replay_shapes(void);                           /* 0x06699 */
void clear_machine(void);                           /* 0x013e9 */
void restart_machine(void);                         /* 0x01431 */
void unlink_part(struct part *part);                /* 0x05628 */
void step_machine(void);                            /* 0x00f86 */
void step_moving_object(struct part *obj);              /* 0x01216 */
void collect_carried(struct part *obj);                 /* 0x03972 */
void carry_riders_along(struct part *obj);              /* 0x03a8d */
void bounce_off_contact(struct part *obj);              /* 0x03046 */
void bounce_pair(struct part *obj);                       /* 0x03201 */
void part_moved(struct part *part);                     /* 0x06d8e */
void belt_in_dirty_rect(struct part *part);             /* 0x06994 */
void mark_parts_in_dirty_rects(void);               /* 0x06806 */
void add_carried_weight(struct part *obj);              /* 0x07c3a */
void add_mass_capped(struct part *obj, struct part *other); /* 0x07c5b */
void part_step(struct part *part);                      /* dispatch, ours */
uint16_t part_hit(uint16_t kind, uint16_t part);    /* dispatch, ours */
uint16_t part_hit_bellow(struct part *part);              /* 0x175f2 */
void     nudge_x_add(struct part *obj, int16_t d);      /* 0x191c8 */
void     nudge_x_sub(struct part *obj, int16_t d);      /* 0x191e2 */
void     nudge_y_add(struct part *obj, int16_t d);      /* 0x19200 */
void     nudge_y_sub(struct part *obj, int16_t d);      /* 0x1921a */
uint16_t part_hit_gear(struct part *part);              /* 0x19238 */
uint16_t part_step_monkey(struct part *part);             /* 0x1a000 */
uint16_t part_hit_monkey(struct part *part);              /* 0x19f43 */
uint16_t part_hit_bucket(struct part *part);              /* 0x17a23 */
uint16_t part_hit_0867(struct part *part);              /* 0x17b27 */
uint16_t part_hit_dynamite(struct part *part);              /* 0x184f7 */
uint16_t     part_settle_conveyor(struct part *part);           /* 0x198dd */
uint16_t     part_settle_ramp(struct part *part);           /* 0x19a49 */
uint16_t     part_settle_48f7(struct part *part);           /* 0x1bbb7 */
uint16_t part_drive_0ffc(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 0x182bc */
uint16_t part_drive_26c3(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 0x19983 */
uint16_t part_drive_341d(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 0x1a6dd */
uint16_t part_drive_44fe(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 0x1b7be */
uint16_t part_hit_dynamite_plunger(struct part *part);              /* 0x1a4ff */
uint16_t part_drive_2e4b(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 0x1a10b */
uint16_t part_step_dynamite_plunger(struct part *part);             /* 0x1a5ea */
uint16_t part_step_solar_panel(struct part *part);             /* 0x1b0c8 */
uint16_t part_drive_02cd(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 0x1758d */
void     part_flip_bellow(struct part *part);             /* 0x17692 */
void     part_flip_boxing_glove(struct part *part);             /* 0x17986 */
void     part_flip_cannon(struct part *part);             /* 0x17ea9 */
void     part_flip_pokey(struct part *part);             /* 0x181fd */
void     part_flip_dynamite(struct part *part);             /* 0x185bc */
void     part_flip_motor(struct part *part);             /* 0x1875b */
void     part_flip_electric_plug(struct part *part);             /* 0x188bc */
void     part_flip_hook(struct part *part);             /* 0x18cba */
void     part_flip_fan(struct part *part);             /* 0x18e7d */
void     part_flip_flashlight(struct part *part);             /* 0x19068 */
void     part_flip_gun(struct part *part);             /* 0x196d2 */
void     part_flip_jack_in_the_box(struct part *part);             /* 0x19c59 */
void     part_flip_light(struct part *part);             /* 0x19e85 */
void     part_flip_monkey(struct part *part);             /* 0x1a0cc */
void     part_flip_magnifying_glass(struct part *part);             /* 0x1a46f */
void     part_flip_dynamite_plunger(struct part *part);             /* 0x1a6a5 */
void     part_flip_mort_the_mouse(struct part *part);             /* 0x1a887 */
void     part_flip_corner_pipe(struct part *part, uint16_t which); /* 0x1aaa5 */
void     part_flip_scissors(struct part *part);             /* 0x1ac04 */
void     part_flip_seesaw(struct part *part);             /* 0x1b47b */
void     part_flip_windmill(struct part *part);             /* 0x1bce2 */
uint16_t part_step_bellow(struct part *part);             /* 0x176c5 */
uint16_t part_hook_172c(uint16_t off, struct part *part); /* segment 172c */
void call_goal_test(struct far_ptr h);
void goal_test_1476(void);                          /* 0x01476 */
void goal_test_151b(void);                          /* 0x0151b */
void goal_test_15fa(void);                          /* 0x015fa */
void goal_test_1cc4(void);                          /* 0x01cc4 */
void goal_test_1cea(void);                          /* 0x01cea */
void goal_test_1d1d(void);                          /* 0x01d1d */
void goal_test_1d5e(void);                          /* 0x01d5e */
void goal_test_14ad(void);                            /* 0x014ad */
void goal_test_14cc(void);                            /* 0x014cc */
void goal_test_14ee(void);                            /* 0x014ee */
void goal_test_16a6(void);                            /* 0x016a6 */
void goal_test_16fb(void);                            /* 0x016fb */
void goal_test_172d(void);                            /* 0x0172d */
void goal_test_1753(void);                            /* 0x01753 */
void goal_test_17db(void);                            /* 0x017db */
void goal_test_1819(void);                            /* 0x01819 */
void goal_test_1846(void);                            /* 0x01846 */
void goal_test_1888(void);                            /* 0x01888 */
void goal_test_18d9(void);                            /* 0x018d9 */
void goal_test_1907(void);                            /* 0x01907 */
void goal_test_1935(void);                            /* 0x01935 */
void goal_test_19ac(void);                            /* 0x019ac */
void goal_test_19e0(void);                            /* 0x019e0 */
void goal_test_1a49(void);                            /* 0x01a49 */
void goal_test_1ab0(void);                            /* 0x01ab0 */
void goal_test_1b89(void);                            /* 0x01b89 */
void goal_test_1d8c(void);                            /* 0x01d8c */
void goal_test_1dbb(void);                            /* 0x01dbb */
void goal_test_1df1(void);                            /* 0x01df1 */
void goal_test_1e1e(void);                            /* 0x01e1e */
void goal_test_1e59(void);                            /* 0x01e59 */
void goal_test_1eb9(void);                            /* 0x01eb9 */
void goal_test_1f25(void);                            /* 0x01f25 */
void goal_test_1fa6(void);                            /* 0x01fa6 */
void goal_test_2010(void);                            /* 0x02010 */
void goal_test_2065(void);                            /* 0x02065 */
void goal_test_20fa(void);                            /* 0x020fa */
void goal_test_2260(void);                            /* 0x02260 */
void goal_test_23ef(void);                            /* 0x023ef */
void goal_test_242c(void);                            /* 0x0242c */
void goal_test_1552(void);                            /* 0x01552 */
void goal_test_1630(void);                            /* 0x01630 */
void goal_test_17ad(void);                            /* 0x017ad */
void goal_test_197e(void);                            /* 0x0197e */
void goal_test_1a0c(void);                            /* 0x01a0c */
void goal_test_1a77(void);                            /* 0x01a77 */
void goal_test_1af7(void);                            /* 0x01af7 */
void goal_test_1b2f(void);                            /* 0x01b2f */
void goal_test_1b63(void);                            /* 0x01b63 */
void goal_test_1bd9(void);                            /* 0x01bd9 */
void goal_test_1c0a(void);                            /* 0x01c0a */
void goal_test_1ee6(void);                            /* 0x01ee6 */
void goal_test_1f77(void);                            /* 0x01f77 */
void goal_test_1fe3(void);                            /* 0x01fe3 */
void goal_test_203f(void);                            /* 0x0203f */
void goal_test_2172(void);                            /* 0x02172 */
void goal_test_21a6(void);                            /* 0x021a6 */
void goal_test_21fd(void);                            /* 0x021fd */
void goal_test_2231(void);                            /* 0x02231 */
void goal_test_2292(void);                            /* 0x02292 */
void goal_test_22d8(void);                            /* 0x022d8 */
void goal_test_2322(void);                            /* 0x02322 */
void goal_test_2351(void);                            /* 0x02351 */
void goal_test_23a4(void);                            /* 0x023a4 */
void check_goal(void);                              /* 0x01465 */
void call_part_flip(struct far_ptr h, uint16_t part,
                    uint16_t which);
uint16_t find_belt_anchor(volatile uint8_t * out_end, uint16_t rec); /* 0x045b8 */
void retension_pulleys(struct part *part);              /* 0x04cc8 */
void rehome_carried_part(void);                     /* 0x050a6 */
uint16_t part_flip_options(struct part *part);          /* 0x04748 */
uint16_t part_handle_at_pointer(struct part *part);     /* 0x04830 */
void pointer_frame(void);                           /* 0x0fc0e */
void move_carried(void);                            /* 0x0fe47 */
void move_carried_rope(void);                       /* 0x0fe84 */
void move_carried_belt(void);                       /* 0x0ff80 */
void scroll_play_area(void);                        /* 0x0fd65 */
void draw_carried_icon(void);                       /* 0x160fc */
void draw_part_selection(struct part *part, uint16_t which, uint8_t flags); /* 0x16209 */
void part_shape_2728(struct part *part);                /* 0x199e8 */
void part_flip_ramp(struct part *part);                 /* 0x19a76 */
void part_flip_mouse_cage(struct part *part);                 /* 0x1a27a */
uint16_t part_drive_172c(uint16_t off, struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6, uint16_t p7);
uint16_t part_drive_0802(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t lo,
                         uint16_t hi);                   /* 172c:0802 */
uint16_t part_drive_11d2(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t lo,
                         uint16_t hi);                   /* 172c:11d2 */
uint16_t part_drive_2451(uint16_t p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t p6,
                         uint16_t p7);                   /* 172c:2451 */
uint16_t part_drive_2c19(uint16_t p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 172c:2c19 */
uint16_t part_drive(struct part *by, struct part *p1, struct part *p2, uint16_t p3,
                    uint16_t p4, uint16_t p5, uint16_t p6, uint16_t p7);
uint16_t drive_belts(uint16_t from, struct part *part, uint16_t flags,
                     uint16_t a, uint16_t b, uint16_t c); /* 172c:461a */
uint16_t part_hit_trampoline(struct part *part);              /* 172c:3ebf */
uint16_t part_step_trampoline(struct part *part);             /* 172c:3fae */
uint16_t part_hit_seesaw(struct part *part);              /* 172c:3fe8 */
uint16_t part_step_seesaw(struct part *part);             /* 172c:420f */
uint16_t rope_other_end(struct part *part);             /* 0x06dbf */
void link_objects_in_range(struct part *obj, uint16_t flags,
                           int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1);  /* 0x036de */
void link_objects_crossing(struct part *obj, uint16_t flags,
                           uint16_t line);           /* 0x03782 */
void link_objects_at_point(struct part *obj, int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1);  /* 0x038b9 */
void     seg172c_nothing(void);                     /* 172c:0000 */
void     sound_on_hard_impact(struct part *obj);        /* 0x03009 */
void     mark_needs_refile(struct part *part, uint8_t n); /* 0x058f3 */
void     mark_belt_shapes(struct part *part, uint16_t mode); /* 0x05f87 */
void     mark_joined_shapes(struct part *part, uint16_t mode); /* 0x05e70 */
void     mark_part_shapes(struct part *part, uint16_t mode); /* 0x0647f */
int16_t  outlines_cross(struct part *a, struct part *b);    /* 0x03f4d */
int16_t  object_overlaps_any(struct part *obj);         /* 0x03e23 */
int16_t  queue_part(struct part *src, uint16_t part);   /* 0x07b6f */
int16_t  tension_belt(struct part *part);               /* 0x072c7 */
int16_t  belt_orientation(uint16_t belt, int16_t which,
                          int16_t dir);             /* 0x06de9 */
uint16_t part_hit_balloon(struct part *part);              /* 172c:016e */
uint16_t part_hit_generator(struct part *part);              /* 172c:1de0 */
uint16_t part_hit_light(struct part *part);              /* 172c:2b7e */
uint16_t part_hit_mouse_cage(struct part *part);              /* 172c:2f25 */
uint16_t part_step_balloon(struct part *part);             /* 172c:018e */
uint16_t part_hit_boxing_glove(struct part *part);              /* 172c:0552 */
uint16_t part_step_boxing_glove(struct part *part);             /* 172c:057e */
uint16_t part_step_08f1(struct part *part);             /* 172c:08f1 */
uint16_t part_step_candle(struct part *part);             /* 172c:098a */
uint16_t part_step_cannon(struct part *part);             /* 172c:0a5d */
uint16_t part_step_pokey(struct part *part);             /* 172c:0ca3 */
uint16_t part_step_11a6(struct part *part);             /* 172c:11a6 */
int16_t  bounce_speed_for_mass(struct part *obj);       /* 172c:06f9 */
void     break_bob_the_fish(struct part *part);              /* 172c:1c9e */
void     trigger_mouse_cage(struct part *part);             /* 172c:2ffd */
int16_t  push_speed_for_mass(struct part *obj);         /* 172c:271f */
void     trigger_things_at(struct part *part, int16_t mode,
                           int16_t dx);             /* 172c:277d */
uint16_t part_hit_pokey(struct part *part);              /* 172c:0c6c */
uint16_t part_step_dynamite(struct part *part);             /* 172c:12c2 */
void     burst_dynamite(struct part *part);              /* 172c:1328 */
uint16_t part_step_motor(struct part *part);             /* 172c:13c9 */
uint16_t part_hit_electric_plug(struct part *part);              /* 172c:14d3 */
uint16_t part_step_electric_plug(struct part *part);             /* 172c:15ce */
uint16_t part_step_gear(struct part *part);             /* 172c:20fc */
uint16_t part_step_gun(struct part *part);             /* 172c:22ae */
uint16_t part_hit_conveyor(struct part *part);              /* 172c:2514 */
uint16_t part_step_conveyor(struct part *part);             /* 172c:2592 */
uint16_t part_step_mouse_cage(struct part *part);             /* 172c:2f3e */
void     part_setup_magnifying_glass(struct part *part);            /* 172c:3030 */
uint16_t part_step_magnifying_glass(struct part *part);             /* 172c:3035 */
uint16_t part_step_mort_the_mouse(struct part *part);             /* 172c:34d0 */
uint16_t part_hit_scissors(struct part *part);              /* 172c:3824 */
uint16_t part_step_rocket(struct part *part);             /* 172c:3635 */
uint16_t part_step_scissors(struct part *part);             /* 172c:38fc */
void     cut_belts(struct part *part, uint16_t line);   /* 172c:3970 */
void grab_distance(struct part *a, struct part *b,
                   volatile uint8_t * out_x, volatile uint8_t * out_y); /* 172c:31dc */
uint16_t spread_gear_signal(struct part *from, struct part *to, int16_t how,
                            uint16_t flag);         /* 172c:105d */
void settle_gear_signal(struct part *part, int16_t clear); /* 172c:1225 */
uint16_t part_step_1649(struct part *part);             /* 172c:1649 */
int16_t  blast_speed_for_mass(struct part *part);       /* 172c:1748 */
void     split_part_at(struct part *part, struct part *blast); /* 172c:17bc */
int16_t  angle_between_centres(struct part *a, struct part *b); /* 0x03da5 */
uint16_t part_step_fan(struct part *part);             /* 172c:1a82 */
uint16_t part_hit_bob_the_fish(struct part *part);              /* 172c:1c39 */
uint16_t part_hit_flashlight(struct part *part);              /* 172c:1d07 */
uint16_t part_step_flashlight(struct part *part);             /* 172c:1d78 */
uint16_t part_step_generator(struct part *part);             /* 172c:1e5c */
uint16_t part_hit_mort_the_mouse(struct part *part);              /* 172c:34b5 */
uint16_t part_step_bob_the_fish(struct part *part);             /* 172c:1c5f */
uint16_t part_step_jack_in_the_box(struct part *part);             /* 172c:27e2 */
int16_t  conveyor_speed_for_mass(struct part *obj);     /* 172c:29c6 */
void     conveyor_nudge_3(struct part *obj, int16_t mid);  /* 172c:2a3a */
void     conveyor_nudge_10(struct part *obj, int16_t mid); /* 172c:2a91 */
void     conveyor_nudge_15(struct part *obj, int16_t mid); /* 172c:2acb */
void     conveyor_nudge_25(struct part *obj, int16_t mid); /* 172c:2b1e */
uint16_t part_step_light(struct part *part);             /* 172c:2b99 */
uint16_t part_step_windmill(struct part *part);             /* 172c:49a1 */
uint16_t game_teardown(int16_t really);             /* 0x0e34a */
uint16_t game_intro(void);                          /* 0x0e4be */
void game_play(void);                               /* 0x0eed5 */
void game_setup(void);                              /* 0x0ef19 */
void game_round(void);                              /* 0x0eff5 */
void round_setup(void);                             /* 0x0f04b */
void round_teardown(void);                          /* 0x0f0a6 */
void load_level(uint16_t number);                   /* 0x12863 */
uint16_t read_level(volatile uint8_t * name);                 /* 0x12269 */
void paint_game_screen(uint16_t present);           /* 0x11632 */
void draw_machine_thunk(void);                      /* 0x15af8 */
void draw_machine_layer_a(void);                    /* 0x15dfd */
void draw_machine_layer_b(void);                    /* 0x15b16 */
void draw_machine_layer_c(void);                    /* 0x15b9f */
void draw_machine_layer_d(void);                    /* 0x15c13 */
void draw_machine_layer_e(void);                    /* 0x15c83 */
void draw_machine_layer_f(void);                    /* 0x15faa */
void paint_panel_frame(void);                       /* 0x117ed */
void draw_title_bar(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint16_t filled);               /* 0x14dec */
void draw_sunken_box(int16_t x, int16_t y, int16_t w, int16_t h); /* 0x153b8 */
void fill_panel_area(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t colour);              /* 0x15523 */
void draw_wrapped_text(uint16_t str, int16_t x, int16_t y,
                       int16_t w, int16_t h);       /* 0x13dc7 */
void wrap_text_to_box(uint16_t str, int16_t w, int16_t h,
                      uint16_t line_height);        /* 0x13ed2 */
void measure_word(volatile uint8_t * str, volatile uint8_t * out_width,
                  volatile uint8_t * out_length);             /* 0x1401d */
uint16_t font_line_height(int16_t slot);            /* 0x215a5 */
void paint_panel_frame_rest(void);                  /* 0x1175c */
void paint_panel_a(uint16_t frame);                  /* 0x1190d */
void paint_panel_b(uint16_t frame);                  /* 0x11943 */
void paint_panel_c(uint16_t frame);                  /* 0x11979 */
void paint_panel_d(uint16_t frame);                  /* 0x119af */
void paint_panel_free_a(uint16_t frame);           /* 0x119e5 */
void paint_panel_free_b(uint16_t frame);           /* 0x11a3f */
void paint_panel_level(uint16_t frame);            /* 0x11a99 */
void paint_panel_e(void);                           /* 0x11acf */
void paint_panel_f(void);                           /* 0x11bd6 */
void paint_panel_g(void);                           /* 0x11c6b */
void present_back_page(void);                       /* 0x081f9 */

void game_screen(void);                             /* 0x10f03 */
void sub_1156c(void);                               /* 0x1156c */
void show_message_box(uint16_t title, uint16_t body);
uint16_t ask_yes_no(uint16_t title, uint16_t body); /* 0x1567b */
uint16_t message_box(uint16_t title, uint16_t body,
                     uint16_t button1, uint16_t button2); /* 0x15698 */
void message_box_tab(uint16_t button2);             /* 0x1588c */
void draw_button(uint16_t str, uint16_t x, uint16_t y,
                 uint16_t pressed);                 /* 0x150db */
void remove_all_parts(void);                        /* 0x057e6 */
void untie_rope(struct part *part);                     /* 0x0527f */
void detach_belt(struct part *part, uint16_t how);      /* 0x052f5 */
void sub_05704(struct part *part);                      /* 0x05704 */
void sub_05482(void);                               /* 0x05482 */
void sub_051cb(struct part *part);                      /* 0x051cb */
void sub_04d4c(struct part *part);                      /* 0x04d4c */
uint16_t sub_04c0d(struct part *part, struct part *other);  /* 0x04c0d */
void discard_part(struct part *part);                   /* 0x05457 */
uint16_t sub_0f0b0(void);                           /* 0x0f0b0 */
uint16_t dos_chdir(uint16_t path);                  /* 0x0b755 */
void     dos_setdisk(uint16_t letter);              /* 0x0b819 */
void reverse_link_ends(uint16_t rec);               /* 0x04169 */
uint16_t part_under_pointer(uint16_t exclude, struct part *part); /* 0x042a2 */
int16_t heapwalk(volatile uint8_t * info);                    /* 0x0ccef */
void repaint_whole_screen(void);                    /* 0x08229 */
int16_t heap_largest_free(void);                    /* 0x084b0 */
int16_t check_room_for_part(void);                  /* 0x08432 */
void redraw_machine_area(void);                     /* 0x15a2f */
int16_t bin_part_at_index(int16_t index);           /* 0x05855 */
void refile_part_list(struct part *part);               /* 0x0578c */
uint16_t bin_scroll_end(void);                      /* 0x058bb */
void bin_scroll_back(void);                         /* 0x10cc8 */
void bin_scroll_forward(void);                      /* 0x10d37 */
void select_music_by_key(void);                      /* 0x0faf9 */
void reset_level_state(void);                       /* 0x0fbda */
void edge_scroll_flags(void);                       /* 0x0fd02 */
void discard_carried_part(void);                    /* 0x10733 */
void carried_part_grow(void);                       /* 0x10466 */
void carried_part_shrink(void);                     /* 0x10551 */
void move_carried_part(void);                       /* 0x101dc */
void part_key_shortcut(void);                 /* 0x10410 */
void pick_up_part(void);                            /* 0x10658 */
void run_drag_frame(void);                          /* 0x10816 */
int16_t drag_carried_part_first(void);              /* 0x108ec */
int16_t settle_carried_part_first(void);            /* 0x10a00 */
int16_t drag_carried_part_pair(void);               /* 0x10ada */
void flip_carried_end_1(void);                      /* 0x107b6 */
void flip_carried_end_2(void);                      /* 0x107e6 */
int16_t settle_carried_part(void);                  /* 0x10bee */
void region_click_bin(uint16_t region);             /* 0x10e14 */
void region_cursor_bin_above(uint16_t region);      /* 0x10da9 */
void region_cursor_bin(uint16_t region);            /* 0x10dc2 */
void region_cursor_playfield(uint16_t region);      /* 0x10ef1 */
void region_cursor_freeform(uint16_t region);       /* 0x114db */
void region_cursor_load(uint16_t region);           /* 0x114f8 */
void region_cursor_save(uint16_t region);           /* 0x11515 */
void region_cursor_gravity(uint16_t region);        /* 0x11532 */
void region_cursor_air(uint16_t region);            /* 0x1154f */
void puzzle_tab(void);                              /* 0x0f468 */
uint16_t puzzle_page_of_score(void);                /* 0x0f499 */
void puzzle_repaint(void);                          /* 0x0f4b5 */
void puzzle_draw_password(uint16_t text);           /* 0x0f640 */
void puzzle_draw_list(int16_t first, int16_t selected); /* 0x0f6cc */
void puzzle_draw_up(void);                          /* 0x0f57e */
void puzzle_draw_down(void);                        /* 0x0f5c4 */
void puzzle_draw_ok(uint16_t pressed);              /* 0x0f60a */
uint16_t pick_file(uint16_t a, uint16_t b, uint16_t pattern); /* 0x12c26 */
uint16_t get_puzzle_title(int16_t n, volatile uint8_t * buf);  /* 0x12a2f */
uint16_t password_to_level(uint16_t text);          /* 0x12ad0 */
uint16_t is_machine_file(uint16_t name);             /* 0x1295f */
uint16_t validate_filename(void);                    /* 0x1319d */
void picker_draw_action(void);                       /* 0x13402 */
void picker_begin(uint16_t a, uint16_t b, const volatile uint8_t * pattern); /* 0x13606 */
uint16_t listing_to_name(const char far * entry);     /* 0x13d75 */
void picker_draw_list(void);                        /* 0x139ac */
void sub_13a8a(const volatile uint8_t * pattern);                   /* 0x13a8a */
void sub_13c78(void);                               /* 0x13c78 */
void picker_repaint(void);                           /* 0x136c9 */
void picker_draw_name(void);                         /* 0x13870 */
void picker_draw_filename(void);                     /* 0x13902 */
void picker_draw_up(void);                           /* 0x137e4 */
void picker_draw_down(void);                        /* 0x1382a */
void picker_tab(void);                              /* 0x1345f */
void picker_type(uint8_t c, uint16_t buf, int16_t max); /* 0x13490 */
uint16_t path_is_root(uint16_t path);               /* 0x134dd */
void path_up(uint16_t path);                        /* 0x13516 */
void path_join(uint16_t path, const char far * entry);     /* 0x1354c */
void force_extension(uint16_t name, uint16_t ext);  /* 0x135a6 */
void picker_set_name(uint16_t name);                /* 0x135dc */
uint16_t picker_name(void);                         /* 0x135ef */
uint16_t save_machine(uint16_t name);               /* 0x1292d */
uint16_t sub_1271c(uint16_t name);                  /* 0x1271c */
void write_byte(uint16_t file, const volatile uint8_t * addr);      /* 0x123b7 */
void write_word(uint16_t file, const volatile uint8_t * addr);      /* 0x123e4 */
void write_string(uint16_t file, uint16_t str);     /* 0x12411 */
uint16_t game_fwrite(const volatile uint8_t * ptr, uint16_t size, uint16_t count,
                     uint16_t file);                /* 0x094fb */
uint16_t sub_0d321(const volatile uint8_t * ptr, uint16_t size, uint16_t count,
                   uint16_t file);                  /* 0x0d321 */
uint16_t sub_0d8ca(uint16_t file, uint16_t count, const volatile uint8_t * buf); /* 0x0d8ca */
int16_t stdio_fputc(int16_t c, uint16_t file);      /* 0x0d784 */
int16_t stdio_putc(int16_t c, uint16_t file);       /* 0x0d76b */
int16_t write_text(int16_t handle, const volatile uint8_t * buf, uint16_t count); /* 0x0de6e */
void sub_126ec(uint16_t file, uint16_t head);       /* 0x126ec */
void sub_12430(uint16_t file, struct part *part);       /* 0x12430 */
uint16_t part_index(uint16_t part);                 /* 0x11d00 */
void sub_126b3(uint16_t file, uint16_t head, uint16_t which); /* 0x126b3 */
uint16_t dos_unlink(uint16_t path);                 /* 0x0b794 */
/*
 * The frame `game_screen` shares with the handlers its jump table reaches.
 *
 * They are not functions in the original: the table at CS:0x34bf *jumps* to
 * them and each ends by jumping back into the loop's tail at 0x1145b, so they
 * run in `game_screen`'s own stack frame and read and write its locals. A port
 * that made them `void f(void)` could not express that, and the counters it
 * would have to duplicate are exactly the ones that decide what gets repainted.
 * So the frame is passed instead.
 */
struct screen_loop {
    int16_t  held;          /* di          - passes since the button went down */
    uint16_t repaint_all;   /* si          - the whole screen */
    uint16_t repaint_e;     /* [bp-0x0a]   - one panel piece each */
    uint16_t repaint_f;     /* [bp-0x0c] */
    uint16_t repaint_g;     /* [bp-0x0e] */
    uint16_t done;          /* [bp-0x08]   - leave the loop */
    uint16_t reload;        /* [bp-0x06]   - the level wants loading again */
    uint16_t file_err;      /* [bp-0x10]   - what the last save answered */
};

void screen_state_4000(struct screen_loop *s);
void screen_state_2000(struct screen_loop *s);
void screen_state_1000(struct screen_loop *s);
void screen_state_0800(struct screen_loop *s);
void screen_state_0400(struct screen_loop *s);
void screen_state_0200(struct screen_loop *s);
void screen_state_0100(struct screen_loop *s);
void screen_state_0080(struct screen_loop *s);
void screen_state_0040(struct screen_loop *s);
void screen_state_0020(struct screen_loop *s);

void game_screen_loop(void);                               /* 0x0f8c2 */
void run_machine_loop(void);                               /* 0x012ab */
void finish_level(void);                             /* 0x02710 */
void sub_12bed(void);                               /* 0x12bed */
void count_level_files(void);                       /* 0x129a8 */
void wait_cursor(void);                             /* 0x04652 */
void restore_cursor(void);                          /* 0x0466e */
int16_t cursor_for_tool(void);                      /* 0x046d8 */
void select_cursor(int16_t which);                  /* 0x0467d */
void set_cursor(uint16_t bitmap, int16_t hot_y,
                int16_t hot_x);                     /* 0x0aa14 */
void redraw_cursor(uint16_t page);                  /* 0x0acc3 */
void set_flag_2d44(void);                           /* 0x0a78e */
int16_t button_state(uint16_t index, int16_t down); /* 0x0b542 */
void isr_stack_switch(int16_t to_private);          /* 0x0b82c */
void timer_callback(void);                          /* 0x0a7ae */
dg_off_t open_bit_reader(struct far_ptr data); /* 0x248fe */
void close_bit_reader(void);                        /* 0x24930 */
void vqt_screen_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h); /* 0x259a1 */
void fill_screen_quadrant(uint16_t x, uint16_t y,
                          uint16_t w, uint16_t h);  /* 0x25aaa */
uint16_t load_screen_plain(uint16_t handle);        /* 0x23b29 */
void draw_cursor(uint16_t page);                    /* 0x0ab1f */
void build_screen_regions(void);                    /* 0x085c9 */
void mouse_set_speed(uint16_t mickeys);             /* 0x0b859 */
uint16_t install_keyboard(int16_t hook_timer);      /* 0x21094 */
uint16_t mouse_init(void);                          /* 0x21f1d */
void mouse_set_ranges(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h);      /* 0x21f8d */
uint16_t load_bitmap_list(uint16_t name);           /* 0x2367c */
uint16_t load_bitmaps(uint8_t * name);               /* 0x24f72 */

/* `main`, and the bring-up it calls first. */
uint16_t game_main(void);                           /* 0x0dfff */
void game_startup(void);                            /* 0x0e01d */

/* Load a palette, a font, and make a font current. Names from the call sites. */
uint32_t load_palette(uint16_t name);               /* 0x1e967 */
uint16_t load_font(uint16_t name);                  /* 0x2307d */
uint16_t set_font(int16_t slot);                    /* 0x2149e */

/* Borland's `printf` and `exit`; the start-up uses them only to give up. */
int16_t stdio_printf(const volatile uint8_t * fmt);                 /* 0x0d754 */
void stdio_exit(int16_t status);                    /* 0x0bcbb */

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
             struct far_ptr dst);                    /* VM.OVL VGA:0x034f */

/* One row of a scaled bitmap, from the column table. Register arguments. */
void vm_blit_scaled_row(uint16_t plane_size, uint16_t coltab,
                        uint16_t dest_row, uint16_t page_seg,
                        int16_t x, int16_t width,
                        struct far_ptr src);        /* VGA:0x03db */

/* The main blitter: a run of pixels from a byte-per-pixel source. */
void vm_blit_run(uint16_t bx, uint16_t cx, const volatile uint8_t far * src,
                 struct far_ptr dst,
                 int32_t backwards);                 /* VM.OVL VGA:0x0938 */

/* Fill a list of horizontal spans with one colour. */
void vm_fill_spans(const uint8_t far * spans);      /* VM.OVL VGA:0x0be6 */

/* Load a sixteen-colour palette into the DAC and keep a copy. */
void vm_load_palette(struct far_ptr pal);           /* VM.OVL VGA:0x0f15 */

/* Load colours into the DAC. */
void vm_span_dithered(uint16_t ax, uint16_t bx, int16_t cx,
                      struct far_ptr dst);            /* VGA:0x27a */
void vm_blit_glyph(const uint8_t far * glyph,
                   uint16_t w, uint16_t h, int16_t x, int16_t y); /* VGA:0x124b */
void vm_blend_palette(uint16_t first, uint16_t count, uint16_t colour,
                      uint8_t weight); /* VM.OVL VGA:0x0f57 */
void restore_write_mode(void);           /* 0x1e94c */
void fade_palette_run(uint16_t first, uint16_t count, uint16_t colour,
                      uint16_t weight);  /* 0x1ec36 */
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
volatile uint8_t far * huge_move(volatile uint8_t far * dst, const volatile uint8_t far * src, uint32_t count);  /* 0x221ed */
void far_memcpy(volatile uint8_t far * dst, const volatile uint8_t far * src, uint16_t count);                    /* 0x222c6 */

/* Set the current palette, or answer the one already set. */
uint32_t set_palette_pointer(struct far_ptr h);   /* 0x1eb6a */

/* Allocate from DOS by byte count; answers seg:0000 in DX:AX. */
union far_or_size dos_alloc_bytes(uint32_t size,
                         uint16_t unused,
                         uint16_t flags);           /* 0x21abd */

/* Fill memory through a far pointer, with a 32-bit count. */
void far_memset(volatile uint8_t far * dst, uint16_t value, uint32_t count);   /* 0x22300 */

/* The far-callable face of normalise_far_ptr; answers seg:off in DX:AX. */
/* Borland's huge-pointer arithmetic - see borland_huge.c. */
int16_t huge_equal(uint16_t off_a, uint16_t seg_a,
                   uint16_t off_b, uint16_t seg_b);    /* 0x0bd0d */
struct far_ptr huge_sub_from(volatile struct far_ptr *var,
                             int32_t delta);   /* 0x0bec6 */
void expand_1bpp_to_4bpp(struct far_ptr src, struct far_ptr dst,
                         uint16_t count);                     /* 0x23a8a */
int32_t long_shift_right(int32_t v, uint8_t count);  /* 0x0be62 */
uint32_t long_multiply_2(uint32_t a, uint32_t b);    /* 0x0bcf6 */
/* Every caller's segment is DGROUP - see the note in borland_huge.c - so the
   variable is a near pointer rather than a far one, and the shim reads one
   word for it. */
struct far_ptr huge_add_to(volatile struct far_ptr *var,
                           int32_t delta);      /* 0x0be82 */
struct far_ptr huge_add(struct far_ptr p, int32_t delta);  /* 0x0bf0a */
uint32_t huge_post_add(volatile struct far_ptr * var,
                       uint16_t inc);                  /* 0x0bf6a */

int16_t decompress_rle(void);                          /* 0x1c278 */
int16_t resource_read(uint16_t handle, uint16_t count); /* 0x1c92b */
void lzw_reset(void);                               /* 0x1c970 */
int16_t restart_resource_stream(int16_t handle);     /* 0x1dae6 */
int16_t lzss_reset(void);                           /* 0x1dc15 */
int16_t open_resource(uint16_t unused, uint16_t file, uint16_t name,
                      uint32_t size);                       /* 0x1d54e */
int16_t close_resource(int16_t handle);             /* 0x1d798 */
uint32_t resource_size(int16_t handle);             /* 0x1d95f */
uint32_t resource_seek(int16_t handle, uint32_t by,
                       int16_t whence);                /* 0x1d983 */
int16_t read_resource(int16_t handle, volatile uint8_t far * dst, uint16_t count); /* 0x1d868 */
int16_t read_input_block(uint16_t dst, uint16_t count); /* 0x1c3e6 */
int16_t decompress_lzw(void);                          /* 0x1ca62 */
int16_t huff_get_bit(void);                            /* 0x1dfd6 */
int16_t huff_get_byte(void);                           /* 0x1e00b */
void huffman_start(void);                              /* 0x1e0b3 */
void huffman_reconst(void);                            /* 0x1e1af */
void huffman_update(uint16_t c);                       /* 0x1e338 */
int16_t decode_position(void);                         /* 0x1e561 */
int16_t decompress_lzss(void);                         /* 0x1e7f2 */
int16_t next_lzw_code(void);                           /* 0x1cc65 */
int16_t emit_literal_run(uint16_t n);                  /* 0x1c493 */
int16_t emit_fill_run(uint16_t value, uint16_t n);     /* 0x1c51e */
int16_t emit_byte(uint16_t value);                     /* 0x1c5a3 */
int16_t read_into_huge(volatile uint8_t far * dst, uint16_t count);                /* 0x1c319 */
int16_t next_input_byte(void);                         /* 0x1c389 */
uint16_t table_618a_in_use(int16_t index);             /* 0x215d5 */
uint16_t detect_adapter(void);                         /* 0x225d2 */
uint32_t load_video_driver(int16_t adapter, uint16_t file); /* 0x22efd */
uint16_t vm_init(uint16_t adapter, uint16_t unused,
                 uint16_t file);                    /* 0x22483 */
/*
 * The polygon filler. These were `static` until the day the rule that a
 * transcribed routine may not be - it keeps them out of libtim.so, so nothing
 * can verify them - and they are declared here now that they are not.
 * Their arguments arrive in **registers**, so the addresses matter to
 * tools/native/dispatch.c rather than to any caller here.
 */
void poly_walk(uint16_t seg, int16_t x, int16_t frac, int16_t step,
               int16_t acc, int16_t count, uint16_t di);      /* 0x1f562 */
void poly_edge_vertical(uint16_t seg, int16_t x,
                        int16_t y1, int16_t y2);              /* 0x1f265 */
void poly_edge_diagonal(uint16_t seg, int16_t x1, int16_t x2,
                        int16_t y1, int16_t y2);              /* 0x1f3bf */
void poly_edge_steep(uint16_t seg, int16_t x1, int16_t x2,
                     int16_t y1, int16_t y2);                 /* 0x1f281 */
void poly_edge_shallow_right(uint16_t seg, int16_t x1, int16_t x2,
                             int16_t y1, int16_t y2);         /* 0x1f3e6 */
void poly_edge_shallow_left(uint16_t seg, int16_t x1, int16_t x2,
                            int16_t y1, int16_t y2);          /* 0x1f4a1 */
void poly_outline(volatile int16_t *xs, volatile int16_t *ys,
                  int16_t n);                             /* 0x1f219 */
void clip_polygon(void);                                      /* 0x20c07 */

void free_bitmap_list(bmp_ptr_t * list);         /* 0x23a18 */
void free_bitmaps(bmp_ptr_t * list);            /* 0x23a3c */
void planes_to_chunky(uint8_t far * dst, const uint8_t far * src,
                      uint16_t count);                    /* 0x24320 */
void emit_packed_value(int16_t value);              /* 0x2451f */
void write_literal_run(uint8_t count, const volatile uint8_t * buf); /* 0x245b9 */
void compress_row(uint16_t src, int16_t remaining); /* 0x24639 */
void compress_bitmap(uint16_t header);              /* 0x24757 */
int32_t compress_bitmap_list(uint16_t list,
                             uint16_t colours);     /* 0x243bf */
void free_bitmaps_thunk(bmp_ptr_t * list);      /* 0x252d0 */
uint16_t count_list_entries(bmp_ptr_t * list);  /* 0x23a6a */
uint16_t read_bmp_info(uint16_t handle, uint16_t * count_at,
                       bmp_ptr_t ** out);                        /* 0x234d2 */
uint16_t mouse_move_to(uint16_t x, uint16_t y);        /* 0x22113 */
uint32_t huge_add_positive(struct far_ptr p, uint16_t lo,
                           uint16_t hi);               /* 0x22190 */
void install_divide_trap(void);                        /* 0x22394 */
int16_t restore_file_record_from(const volatile uint8_t * src);        /* 0x23ee4 */
void set_field_4_of_each(uint16_t value, bmp_ptr_t * list); /* 0x252b4 */
uint16_t count_list(bmp_ptr_t * list);             /* 0x252e0 */
void far_copy(uint8_t far *dst, const uint8_t far *src,
              uint16_t count);       /* 0x25d96 */
void dos_getdate(volatile uint8_t * out);                        /* 0x0bd4a */
uint16_t to_lower(uint16_t c);                         /* 0x0c293 */
int16_t  far_stricmp(const char far * a,
                     const char far * b);              /* 0x09f68 */
void dos_find_to_dgroup(void);                         /* 0x0b6ef */
uint16_t dos_findfirst(uint16_t pattern, uint16_t attr); /* 0x0b6b7 */
uint16_t dos_findnext(uint16_t pattern, uint16_t attr);  /* 0x0b6d3 */
uint16_t dos_find_attr(void);                          /* 0x0b72e */
volatile uint8_t *  dos_find_name(void);                          /* 0x0b734 */
uint32_t dos_find_size(void);                          /* 0x0b738 */
void dos_get_cur_dir(uint16_t buf);                    /* 0x0b7b3 */
volatile uint8_t *  string_concat(volatile uint8_t * dst, const volatile uint8_t * src);     /* 0x0dc95 */
int16_t stdio_setbuf(uint16_t file, uint16_t buf);     /* 0x0c1b2 */
int16_t heap_check(void);                              /* 0x0cb45 */
void heap_check_or_hang(void);                         /* 0x08528 */
void checked_free(uint16_t p);                         /* 0x08510 */
void free_region_lists(void);                          /* 0x08eb5 */
void free_archive_lists(void);                         /* 0x09784 */
int16_t remove_keyboard(void);                         /* 0x21158 */
int16_t remove_mouse(void);                            /* 0x220cd */
void restore_int0_vector(void);                        /* 0x223f7 */
void set_bios_video_mode(uint16_t bits);               /* 0x22741 */
void shutdown_input(void);                             /* 0x225a5 */
void restore_video_mode(void);                         /* 0x225ba */
void free_far_block(struct far_ptr h);       /* 0x1ebdc */
void close_table_618a_slot(int16_t index);             /* 0x233ef */
void setup_streams(void);                              /* 0x0c1d6 */
void set_holiday_flags(void);                          /* 0x08259 */
void heap_free_far(volatile uint8_t * p);                        /* 0x0bb2d */
void game_fread_far(uint16_t file, volatile uint8_t * buf);      /* 0x11dd1 */
uint16_t read_tim_cfg(void);                           /* 0x12ba7 */
void show_page_thunk(uint16_t wait_retrace);           /* 0x2149a */
void save_rect_thunk(struct far_ptr buf, int16_t x,
                     int16_t y, int16_t w, int16_t h); /* 0x21ab5 */
uint32_t buffer_size_thunk(uint16_t w, uint16_t h);    /* 0x21ab9 */
void restore_rect_thunk(struct far_ptr buf, int16_t x,
                        int16_t y, int16_t w, int16_t h); /* 0x2247f */
uint16_t bios_video_kind(void);                        /* 0x22764 */
int16_t detect_pcjr(void);                             /* 0x20be0 */
void timer_tick(void);                              /* 0x20767 */
int16_t timer_install(uint16_t rate);                  /* 0x206c1 */
int16_t timer_remove(void);                            /* 0x2072e */
uint16_t timer_add_callback(struct far_ptr cb,
                            uint16_t period);          /* 0x20654 */
uint16_t timer_drop_callback(uint16_t handle);         /* 0x2069e */
struct far_ptr normalise_far_ptr_far(struct far_ptr p);      /* 0x22386 */

/* Carry paragraphs out of a far pointer's offset into its segment. */
void normalise_far_ptr(uint16_t *off, uint16_t *seg);       /* 0x22161 */

/* Store a quarter of each of two words through near pointers. */
void read_pair_4740(volatile int16_t *out_a,
                    volatile int16_t *out_b);         /* 0x220e9 */

/* Bit 0 of one of two flag bytes at DGROUP 0x48ea. */
int16_t compute_step(volatile uint8_t * rec, int16_t count);   /* 0x20840 */
int16_t scale_table_delta(int16_t n);               /* 0x22790 */
int16_t flag_bit_48ea(uint16_t which);              /* 0x2213e */
void mouse_save_vga(void);                          /* 0x2200f */
void mouse_restore_vga(void);                       /* 0x22074 */
void mouse_set_user_handler(struct far_ptr h); /* 0x21fbe */
void mouse_event(uint16_t buttons, uint16_t x, uint16_t y); /* 0x21fcf */

/* Bit 0 of the byte array at DGROUP 0x468c. */
int16_t bit0_of_468c(uint16_t index);               /* 0x2147d */

/* ---------------------------------------------------------- segment 14de */
void clear_layer_heads(void);                   /* 0x166d6 */

/* Link a record into up to two buckets headed by that array. */
void link_record_into_buckets(struct part *rec);        /* 0x166ef */

/* ---------------------------------------------------------- segment 2619 */
uint16_t advance_record(const uint8_t *rec, uint16_t off);  /* 0x2891a */

/* Follow a chain of far pointers; answers seg:off packed into 32 bits. */
uint32_t follow_far_chain(struct far_ptr rec,
                          int16_t count);           /* 0x2907b */

/* Scale one byte by another and halve the range. */
uint8_t scale_byte_pair(uint8_t cl, uint8_t dl);    /* 0x282cb */

/* ---------------------------------------------------------- segment 2a04 */

/* Sine and cosine of a 16-bit angle, 16384 standing for 1. */
int16_t angle_sin(uint16_t angle);                  /* 0x2a456 */
int16_t angle_cos(uint16_t angle);                  /* 0x2a47b */

/* Signed 16x16 multiply; answers the 32-bit product in DX:AX. */
int32_t  mul16x16(int16_t a, int16_t b);            /* 0x2a269 */

/* Not transcribed yet; the driver's line drawer. */
void vm_draw_line(int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2);          /* VM.OVL VGA:0x0998 */

/* Clip a line to the clip box and draw what is left. */
uint16_t draw_char(uint8_t c, int16_t x, int16_t y); /* 0x21670 */
void draw_string_body(const volatile uint8_t far * str,
                      int16_t x, int16_t y);        /* 0x218eb */
void draw_string(const volatile uint8_t * str, int16_t x, int16_t y); /* 0x218d4 */
uint16_t text_width(const volatile uint8_t * str);                  /* 0x21610 */
uint16_t text_width_thunk(const volatile uint8_t * str);            /* 0x215ff */
void clip_and_draw_line(int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2);    /* 0x21e34 */

#endif /* TIM_H */
