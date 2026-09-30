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
/* `FILE`, which the prototypes below take. */
#include <stdio.h>

#ifdef __TURBOC__
/* **The names Borland sees**, where the port's differ: the game's entry point
   is C's `main`, and the video driver's three calls are the thunks the image
   has. tools/judge.py reads this block. */
#define game_main               main
#define vm_buffer_size          buffer_size_thunk
#define vm_restore_rect         restore_rect_thunk
#define vm_show_page            show_page_thunk
#define vm_set_border_colour    border_colour_thunk
#endif

#include "dgroup.h"

/*
 * **Borland's pointer tags**, `near`, `far`, `huge` and `interrupt`, are kept
 * in the source for the compiler it came from and defined as nothing on the
 * host (dgroup.h). An untagged data pointer is near, as the medium model has
 * it. They are a spelling, not a semantics: on the host a `far` pointer is an
 * ordinary pointer and does not wrap at 64K, so where the wrap or the
 * `seg:off` pair matters the site says so. `MK_FP`, `FP_SEG` and `FP_OFF`
 * are Borland's own names for making and taking one apart; on the host a
 * segment is a pointer to its paragraph (dgroup.h).
 */

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

uint16_t part_hook_yes(struct part *part);              /* 0x00297 */
void     part_hook_none_2a1(struct part *part);         /* 0x0029f */
void     part_hook_none_2a6(struct part *part);         /* 0x002a4 */
void     part_hook_none_2ab(struct part *part);         /* 0x002a9 */
void     part_hook_none_2b0(struct part *part);         /* 0x002ae */
uint16_t part_hook_no(struct part *part);               /* 0x002b3 */

/* Subtract two fields of the structure DGROUP 0x5400 points at. */
void compute_moved(void);                               /* 0x002ba */

/* Step the counter at DGROUP 0x4e87. */
void step_loop_frames(void);                          /* 0x01497 */

/* Advance the button state for one frame. */
void update_button_state(void);                     /* 0x08c28 */

/* Set the clip box from the mode word: a saved rectangle or a fixed one. */
void set_clip_for_mode(void);                       /* 0x08dc0 */

/* Set the clipping box to the whole visible screen. */
void set_clip_play_area(void);                      /* 0x08e2d */
void set_clip_full_screen(void);                    /* 0x08e46 */

/* Apply the kind's gravity, clamp, and compute a Manhattan speed. */
void apply_gravity_and_speed(struct part *rec);         /* 0x03828 */
void touch_vel_y(struct part *rec);                    /* 0x0387f */

/* Build the swept bounding box of the object at DGROUP 0x5400. */
void compute_swept_bounds(void);               /* 0x002d5 */

/* Derive a rectangle and its centre from the structure at DGROUP 0x53fe. */
void compute_other_bounds(void);                     /* 0x0035c */

/* Are two angles on the same side of a reference direction? */
int16_t angles_same_side(int16_t angle);            /* 0x003a1 */

/* Reduce a 16-bit angle to one of four directions. */
int16_t angle_to_quadrant(int16_t angle);           /* 0x00486 */

/* Recompute gravity and the velocity limit for every kind. */
void recompute_kind_physics(void);                  /* 0x0369c */

/* Clamp two signed fields of a record to plus or minus a per-kind limit. */
void clamp_record_pair(struct part *rec);               /* 0x037bc */

/* Rotate a point about the origin, in place. */
/* px and py are read and written in place; the guest passes each as one
   DGROUP word, which `volatile uint8_t *` is what tells the shim generator. */
void rotate_point(int16_t *px, int16_t *py, uint16_t angle); /* 0x046f1 */

/* Is a node on the chain hanging off a record? */
int16_t chain_contains(struct part *rec, struct part *node);      /* 0x0463d */

/* Find which record owns the far pointer in the globals at 0x5482. */
int16_t find_entry_for_pointer(struct game_file *out);       /* 0x0a535 */

/* Bytes a w by h planar image needs. */
uint32_t vm_buffer_size(uint16_t w, uint16_t h);    /* VM.OVL VGA:0x138e */
/* Chunky 4bpp to planar, through video memory, filling a list of headers. */
void vm_load_bitmap_list(struct bitmap ** list, uint8_t far * dst,
                         uint32_t count);                       /* VGA:0x1015 */
void vm_chunky_to_planar(const uint8_t far * src, uint8_t far * dst,
                         uint16_t count);                       /* VGA:0x10b8 */
void vm_read_four_planes(const uint8_t far * src, uint8_t far * dst,
                         uint16_t count);                       /* VGA:0x11bb */
void vm_build_mask_plane(const uint8_t far * src, uint8_t far * dst,
                         uint16_t count);                       /* VGA:0x11ee */

void vm_nothing(void);                              /* VGA:0x0252 */
void vm_blit_rows(const uint8_t far * src, int16_t x, int16_t y,
                  int16_t w, int16_t h);            /* VGA:0x15d0 */
void blit_rows_thunk(const uint8_t far * src, int16_t x, int16_t y,
                     int16_t w, int16_t h);         /* 0x224c2 */
void blit_rows_alt_thunk(const uint8_t far * src, int16_t x, int16_t y,
                         int16_t w, int16_t h);  /* 0x224c6 */
void vm_blit_bitmap(struct bitmap * bmp, int16_t x, int16_t y,
                    uint16_t mode);                     /* VGA:0x1707 */
void vm_blit_scaled(struct bitmap * bmp, int16_t x, int16_t y); /* VGA:0x271b */
void blit_bitmap_thunk(struct bitmap * bmp, int16_t x, int16_t y,
                       uint16_t mode);                  /* 0x205ca */
void blit_scaled_thunk(struct bitmap * bmp, int16_t x, int16_t y); /* 0x205ce */
void draw_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode); /* 0x26f8a */
void draw_compressed_body(struct bitmap *bmp, int16_t x, int16_t y,
                          uint16_t mode);          /* 0x21e13 */
void draw_compressed_bitmap(struct bitmap * bmp, int16_t x, int16_t y,
                            uint16_t mode);             /* 0x21e0f */
void draw_offset_bitmap(struct bitmap * bmp, int16_t x, int16_t y,
                        uint16_t mode);                 /* 0x26b24 */
uint32_t vm_bitmap_list_size(struct bitmap **list,
                             uint8_t * out);         /* VM.OVL VGA:0x0fd4 */

/* Save a rectangle of the source page into a buffer, all four planes. */
void vm_save_rect(uint8_t far * buf, int16_t x, int16_t y,
                  int16_t w, int16_t h);            /* VM.OVL VGA:0x12fb */

/* Restore a rectangle from a buffer into the destination page. */
void vm_restore_rect(const uint8_t far * buf, int16_t x, int16_t y,
                     int16_t w, int16_t h);         /* VM.OVL VGA:0x13b9 */

/* atan2 of two longs, in the whole-turn-is-0x10000 space. */
int16_t atan2_long(int32_t a, int32_t b);          /* 0x2ede0 */

/* Chain every object whose box comes within the given margins. */
void link_nearby_objects(struct part *obj, uint16_t flags,
                         int16_t margin_x0, int16_t margin_x1,
                         int16_t margin_y0, int16_t margin_y1); /* 0x0417e */

/* The same sweep with the two objects exchanged. */
int16_t find_edge_contact_reversed(int16_t test_only);  /* 0x00ac8 */

/* Step one sequence forward by one tick. */
void step_sequence(struct sequence far * seq, uint16_t di);  /* 0x2a59a */

/* Handle an explicit note-off event; answers the advanced cursor. */
const uint8_t far * midi_note_off_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2a7de */

/* Two-byte event, driver function 6 (a stub). */
const uint8_t far * midi_event_6(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2a8a0 */

/* Sequencer meta events: checkpoints, loop counters, rewinds. */
const uint8_t far * midi_meta_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2aaca */

/* Step past an event this module does not handle. */
const uint8_t far * skip_unknown_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2abda */

/* A forwarder to skip_unknown_event. */
const uint8_t far * midi_skip_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2817a */

/* Controller change: keeps most of a channel's state. */
const uint8_t far * midi_controller_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2a8d1 */

/* Program change: stores the instrument at +0x116. */
const uint8_t far * midi_program_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2a9d2 */

/* One-byte event, driver function 9 (a stub). */
const uint8_t far * midi_event_9(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2aa26 */

/* Handle one MIDI note event; answers the advanced stream cursor. */
const uint8_t far * midi_note_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2a82d */

/* Parse a sequence's device parameter table once, cached in place. */
void init_sequence_params(struct sequence far * seq);  /* 0x28305 */

/* Next record matching a selector, as a far pointer in DX:AX. */
struct sound_record far * next_matching_record(int16_t selector);    /* 0x280b5 */

/* Handle one pitch bend event; answers the advanced stream cursor. */
const uint8_t far * midi_bend_event(const uint8_t far * data,
        struct sequence far * seq, uint16_t si, uint16_t ax);  /* 0x2aa4a */

/* Allocate a block for the sound module by kind; zero some kinds. */
uint8_t far * alloc_for_kind(uint32_t size,
                              uint16_t kind);             /* 0x16beb */

/* Release a block by the same kind it was allocated with. */
void free_for_kind(uint8_t far * blk,
                   uint16_t kind);                  /* 0x2a017 */

/* Free a chain of kind-9 nodes linked at +4. */
void free_node_list(struct sound_node far * list);            /* 0x2841b */

/* Build a sequence record around note data; null far pointer on failure. */
struct sequence far * create_sequence(const uint8_t far * src);     /* 0x281d1 */

/* The ordinary-call face of start_sequence. */
void start_sequence_far(struct sequence far * seq,
                        uint16_t flag);             /* 0x28480 */

/* Locate a sequence, set its volume, and start it. */
struct sequence far * load_and_start_sequence(struct sequence far * seq, int16_t count,
                                       uint16_t volume);  /* 0x29034 */

/* Start a sequence: reset it, read its header, place it in the table. */
void start_sequence(struct sequence far * seq, uint16_t cx);  /* 0x290d5 */

/* Advance a sequence's volume fade by one tick. */
void advance_volume_ramp(struct sequence far * seq,
                         uint16_t seq_slot);        /* 0x2a23b */

/* Set a sequence's volume and push it to every voice it owns. */
void set_sequence_volume(struct sequence far * seq, uint8_t volume,
                         uint8_t defer, uint16_t seq_slot);  /* 0x2a2fb */

/*
 * **What the loaded sound module is handed through SI**, one record per
 * function number. The original builds each on the stack and points SI at it;
 * the module reads it by offset. The layouts are the module's reads -
 * `asb_play`, `asb_set_rate_fn`, `asb_position` - and what `poll_sequences`
 * pushes for functions 3 and 4.
 */
struct sound_play_args {                /* function 3 */
    uint8_t        volume;              /* +0 */
    uint8_t        loop;                /* +1  non-zero plays it again */
    uint16_t       rate;                /* +2 */
    const uint8_t far *sample;          /* +4  a segment and an offset inside it */
    uint16_t       length;              /* +8 */
} PACKED;

struct sound_poll_args {                /* function 4 */
    uint8_t        volume;              /* +0 */
    uint8_t        loop;                /* +1 */
} PACKED;

struct sound_rate_args {                /* function 6 */
    uint16_t       rate;                /* +0 */
} PACKED;

struct sound_position_args {            /* function 13, written back */
    uint16_t       id;                  /* +0  the sample's */
    uint32_t       position;            /* +2 */
} PACKED;

union sound_module_args {
    struct sound_play_args     play;
    struct sound_poll_args     poll;
    struct sound_rate_args     rate;
    struct sound_position_args position;
};


/* The sound module's service routine - what the timer calls. */
void sound_service(void);                           /* 0x2a420 */

/* Remove a sequence unless it is on the poll table. */
void drop_unless_polled(struct sequence far * seq);  /* 0x2a4a4 */

/* Poll sequences on the cs:0x48 table through the host callback. */
void poll_sequences(void);                          /* 0x2a4d0 */

/* Take a sequence out of the playing table and stop it. */
void remove_sequence(struct sequence far * seq);    /* 0x297cd */

/* Call the host's sound callback if one is installed. */
uint16_t sound_callback(uint16_t ax, union sound_module_args * si); /* 0x2bb41 */
void sound_callback_quiet(uint16_t ax, union sound_module_args * si); /* 0x2bb79 */

/* The sequencer tick: place voices and tell the driver. */
void sequencer_tick(void);                          /* 0x2987c */

/* Flush up to two pending volume changes to the driver. */
void flush_pending_volumes(void);                   /* 0x2a3d8 */

/* The PC-speaker sound driver, SX.OVL - see docs/sound-driver.md. */
uint16_t install_driver(const uint8_t far * drv);   /* 0x28f44 */
uint16_t configure_driver(const uint8_t far * drv); /* 0x28f7b */
void silence_driver(void);                          /* 0x28fa0 */
void set_master_level(uint8_t cl);                  /* 0x29073 */
/* sound.c's routines nothing in the game calls, named 2026-09-27. */
uint16_t sound_api(uint16_t fn);                       /* 0x28cef */
void     sound_api_dispatch(void);                     /* 0x28d51 */
void     sound_hold(uint16_t cx);                      /* 0x28fbf */
void     driver_fn13(void);                            /* 0x28fd8 */
void     seek_sequence(struct sequence far *seq);      /* 0x28fe3 */
uint16_t set_sequence_level(uint8_t cl);               /* 0x2908a */
void retire_and_tick(struct sequence far * seq);                         /* 0x293a9 */

/* The sound module's own routines over that driver, in address order. */
struct sequence far * voice_playing(const uint8_t far * source);    /* 0x287ad */
uint16_t alloc_voice_records(void);                    /* 0x28800 */
void follow_then_tick(struct sequence far * seq,
                      int16_t count);                  /* 0x289ba */
uint16_t seek_to_sound_record(int16_t handle,
                              uint8_t want);          /* 0x2845e */
struct sound_node far * read_sound_records(int16_t handle);           /* 0x2855f */
uint16_t read_record(FILE *file, uint8_t mode);      /* 0x27e22 */
uint16_t start_sound(int16_t device, int16_t module_index,
                     uint16_t callback, FILE *handle); /* 0x27ffe */
uint16_t setup_sound_device(int16_t device, int16_t module_index,
                            uint16_t callback, FILE *handle); /* 0x2b29e */
uint16_t load_sound_module(FILE *handle, const int16_t *number,
                           uint16_t index);         /* 0x2ad3f */
uint8_t far * load_named_chunk(char *name, const char * path,
                          uint16_t index);          /* 0x288aa */
uint8_t far * load_sound_bank(FILE *file, uint32_t size,
                               uint8_t * out, uint16_t kind); /* 0x28256 */
uint8_t far * load_resource_block(FILE *file, uint32_t size,
                                   uint8_t * out,
                                   uint16_t kind);      /* 0x2ba6a */
uint16_t build_sound_index(int16_t handle, const struct sound_node far * list,
                           uint8_t far * dst, uint16_t data_at,
                           uint16_t tag);              /* 0x286ef */
struct sound_node far * insert_by_key(struct sound_node far * head,
                                      struct sound_node far * node);
void stop_voice_playing(const uint8_t far * source);   /* 0x2b929 */
uint16_t free_voice_records(void);                     /* 0x29106 */
struct sequence far * start_on_free_voice(const uint8_t far * source, uint16_t index,
                                   uint8_t byte_arg);        /* 0x2b982 */
void stop_all_voices(void);                            /* 0x2923d */
void set_sound_callback(const uint8_t far * cb);   /* 0x2bb2c */
void stop_sound(void);                                 /* 0x292f4 */
void shutdown_sound(void);                             /* 0x2b1f8 */
void delay_five_ticks(void);                           /* 0x2b68f */
void tick_delay(void);                                 /* 0x293b8 */
uint16_t remove_and_free_records(int16_t selector);    /* 0x2b0bf */
uint16_t stop_sequences(int16_t selector);             /* 0x2b6cd */
FILE *open_sound_file(char *name, int16_t id);     /* 0x2ae14 */
uint16_t set_master_level_ok(uint16_t level);          /* 0x2ad2c */
uint16_t start_sequence_by_id(int16_t id);             /* 0x2b418 */

/* The ordinary-call faces of the hand-written routines above. */
void set_master_level_far(uint16_t level);             /* 0x28431 */
uint16_t install_driver_far(const uint8_t far * drv);    /* 0x28458 */
uint16_t configure_driver_far(const uint8_t far * drv);  /* 0x2846a */
void retire_and_tick_far(struct sequence far * seq);  /* 0x284ef */
void silence_driver_far(void);                      /* 0x28559 */
void seek_sequence_far(struct sequence far * seq);  /* 0x2841f */
void driver_fn13_far(void);                         /* 0x284b0 */
void set_sequence_level_far(uint16_t level);        /* 0x2852c */

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
void     sbp_init(const uint8_t far * src, uint16_t *ax, uint16_t *cx);
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
void     adl_load_patch(uint16_t voice, const struct adl_patch *p);   /* SX.OVL ADL:0x1fe1 */
void     adl_write_operator(uint16_t slot, const uint8_t *run,
                            uint8_t connect);
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
void     adl_init(const uint8_t far * src, uint16_t *ax, uint16_t *cx);
                                                    /* SX.OVL ADL:0x2414 */
void     adl_describe_0(uint16_t *ax, uint16_t *cx);   /* SX.OVL ADL:0x2446 */

/* The driver call: which loaded driver a function number goes to. Ours. */
void     driver_describe_0(uint16_t *ax, uint16_t *cx);
void     driver_describe_1(const uint8_t far * drv,
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
void     asb_dma_program(const uint8_t far *block, uint16_t count,
                         uint8_t mode);         /* SX.OVL ASB:0x08ec */
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
void     asb_play(const struct sound_play_args *si); /* SX.OVL ASB:0x011e */
uint16_t asb_status(void);                      /* SX.OVL ASB:0x01be */
void     asb_stop(void);                        /* SX.OVL ASB:0x01ce */
uint16_t asb_uninstall(void);                   /* SX.OVL ASB:0x01d2 */
uint16_t asb_set_rate_fn(const struct sound_rate_args *si); /* SX.OVL ASB:0x00de */
uint16_t asb_clear_49(void);                    /* SX.OVL ASB:0x00ec */
uint16_t asb_position(struct sound_position_args *si); /* SX.OVL ASB:0x0435 */
uint16_t asb_install(void);                     /* SX.OVL ASB:0x0577 */
uint16_t asb_dispatch(uint16_t fn, union sound_module_args * si); /* SX.OVL ASB:0x00c8 */

/* Resolve one object against everything it could be touching. */
int16_t resolve_collisions(struct part *obj);           /* 0x004fd */

/* Sweep one object's edges against another's; record the contact. */
int16_t find_edge_contact(int16_t test_only);       /* 0x00749 */

/* Advance an object one step: velocity, gravity, clamp, place. */
void integrate_object(struct part *obj);                /* 0x0388f */

/* Work out where an object is drawn, at +0x2a/+0x2c. */
struct part *clone_part(struct part *part);             /* 0x066aa */
void place_object_for_draw(struct part *obj);           /* 0x068aa */

/* Add shape records for a sub-object's point pairs. */
void add_sub_object_shapes(struct part *obj, int16_t mask);  /* 0x06bac */

/* Set an object's extent at +0x44/+0x46 from its kind. */
void set_object_extent(struct part *obj);               /* 0x06940 */

/* Angle from two differences across an object's +0x1e/+0x22 fields. */
int16_t object_delta_angle(struct part *obj);           /* 0x00462 */

/* Arctangent table lookup; index is a ratio in 0..511. */
int16_t arctan_ratio(int16_t x, int16_t y);            /* 0x2c3e8 */
int16_t arctan_lookup(uint16_t index);              /* 0x2c48b */

/* Apply contact friction to an object. */
void apply_contact_friction(struct part *obj);          /* 0x03980 */

/* Read one pixel's colour from the source page; no clipping. */
uint16_t vm_driver_init(const struct vmds *data, void (far * const *params)(void),
                        uint16_t ds);           /* VM.OVL VGA:0x0000 */
void vm_reset_attributes(void);                     /* VM.OVL VGA:0x011d */
uint16_t vm_read_pixel(int16_t x, int16_t y);       /* VM.OVL VGA:0x1453 */

/* Read a pixel if inside the driver's clip window, else -1. */
int16_t read_pixel_clipped(int16_t x, int16_t y);   /* 0x240a5 */

/* Plot one pixel; no clipping. */
uint16_t vm_plot_pixel(int16_t x, int16_t y,
                       uint8_t colour);             /* VM.OVL VGA:0x14c9 */

/* Plot a pixel if inside the driver's clip window, else -1. */
int16_t plot_pixel_clipped(int16_t x, int16_t y,
                           int16_t colour);         /* 0x240d7 */

/* Take the object off both the drawn-into and on-screen pages. */
void erase_both_pages(void);                        /* 0x08bd9 */

/* Put back what an object was covering; mark it not drawn. */
void erase_object(uint16_t handle);                 /* 0x0b9a8 */

/* Age an object's on-screen rectangle by one frame. */
void restage_object_rect(uint16_t handle);          /* 0x0bb4b */

/* Claim one of four scratch buffers; one-based index, or -1. */
int16_t claim_buffer_slot(int32_t a, int32_t b);        /* 0x0c234 */

/* Clear one byte of the one-based four-entry array at 0x5734. */
void release_buffer(int16_t n);                    /* 0x0c2de */

/* Make a resource file the open one, closing whatever was open before. */
void make_file_current(uint16_t index);             /* 0x0a6a7 */

/* Put a file at a position, without asking DOS if it is already there. */
void seek_file_to(uint32_t at);                     /* 0x0a77d */

/* The archive entry standing in for an open file, or null for a real one. */
struct game_file *archive_entry_for(FILE *file); /* 0x0a7c1 */
int16_t game_fseek(FILE *file, int32_t off,
                   int16_t whence);                 /* 0x09f7b */
uint32_t fread_huge(uint8_t far * dst, uint32_t size, uint32_t count,
                    FILE *file);                 /* 0x0c57f */
int32_t game_ftell(FILE *file);                  /* 0x0a038 */
int16_t game_fgetc(FILE *file);                  /* 0x0a086 */
int16_t game_fputc(int16_t c, FILE *file);       /* 0x0a1e6 */
int16_t game_fclose(FILE *file);                 /* 0x09e2e */
void game_rewind(FILE *file);                    /* 0x0a072 */
int16_t  answer_carry_on(uint16_t what);            /* 0x09c79 */
FILE *game_fopen(char *name, const char *mode);   /* 0x09c81 */
void load_archive_map(void);                        /* 0x0a26f */
int32_t hash_filename(char *name);               /* 0x0a465 */
uint16_t game_fread(uint8_t * buf, uint16_t size, uint16_t count,
                    FILE *file);                 /* 0x09e9c */

/* Zero the word at DGROUP 0x2d44; meaning not established. */
void cursor_redraw_off(void);                         /* 0x0b3e6 */
void cursor_redraw_off_thunk(void);                   /* 0x08c0d */

/* **A `long` multiply as the compiler writes it**: Borland compiles `a * b`
   on longs to a call to N_LXMUL@ in CM.LIB, and on the host the multiply is
   done unsigned - which is also what makes it wrap, as the 16-bit code does,
   where a host `int32_t` multiply may overflow. Ours. */
#ifdef __TURBOC__
#  define LONG_MUL(a, b)  ((a) * (b))
#else
#  define LONG_MUL(a, b)  ((int32_t)((uint32_t)(a) * (uint32_t)(b)))
#endif
/* **A local the original writes past the end of**, into the one beside it,
   which by then it no longer needs. The host lays its frame out otherwise,
   so it is given the bytes; Borland sees the original's size. Ours. */
#ifdef __TURBOC__
#  define OVERRUN(n, past)  (n)
#else
#  define OVERRUN(n, past)  ((n) + (past))
#endif
/* **A buffer handed to `setbuf`**, which takes it to be `BUFSIZ` bytes:
   Borland's is 512, so the game's 0x210 covers it, and glibc's is 8192.
   The host's is made big enough; Borland sees the game's size. Ours. */
#ifdef __TURBOC__
#  define SETBUF_ROOM(n)  (n)
#else
#  define SETBUF_ROOM(n)  ((n) > BUFSIZ ? (n) : BUFSIZ)
#endif
/* **A signed word shifted left in place**, `shl word ptr [..],cl`: Borland's
   `x <<= n`, and on the host the same bits through `uint16_t`, because a
   negative `int` shifted left is undefined there. Ours. */
#ifdef __TURBOC__
#  define SHL16_ASSIGN(lv, n)  ((lv) <<= (n))
#else
#  define SHL16_ASSIGN(lv, n)  ((lv) = (int16_t)((uint16_t)(lv) << (n)))
#endif
void near read_far(uint8_t huge *dst, int32_t count,
              FILE *file);                        /* 0x271a4 */
void near decode_vqt_list(FILE *file, struct bitmap **list); /* 0x272c3 */
void near vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h);   /* 0x27a42 */
void near fill_quadrant(uint16_t x, uint16_t y,
                   uint16_t w, uint16_t h);         /* 0x27b3f */
void    *calloc_far(uint16_t count, uint16_t size); /* 0x0c7b7 */
/* thunks.c: far faces of the runtime's routines, the first twelve uncalled. */
int16_t open_far(const char *name, uint16_t flags);                  /* 0x0c674 */
int16_t open_file_perm_far(const char *name, uint16_t flags, uint16_t perm); /* 0x0c687 */
int16_t close_far(int16_t handle);                                 /* 0x0c69d */
int16_t readfd_far(int16_t handle, uint8_t *buf, uint16_t count);   /* 0x0c6ac */
FILE *fopen_far(const char *name, const char *mode);   /* 0x0c6c2 */
int16_t fseek_far(FILE *file, int32_t off, int16_t whence); /* 0x0c6d5 */
int32_t ftell_far(FILE *file);                         /* 0x0c6ee */
uint16_t fread_far(uint8_t *buf, uint16_t size, uint16_t count,
                           FILE *file);                        /* 0x0c6fd */
uint16_t fwrite_far(const uint8_t *buf, uint16_t size, uint16_t count,
                            FILE *file);                       /* 0x0c716 */
int16_t fputc_far(int16_t c, FILE *file);              /* 0x0c72f */
void    rewind_far(FILE *file);                        /* 0x0c742 */
int16_t fclose_far(FILE *file);                        /* 0x0c751 */
void    *malloc_far(uint16_t bytes);             /* 0x0c760 */
/* `buf` is written through and handed back; the guest passes and expects a
   DGROUP offset, which the shim converts in both directions. */
void draw_odometer_digit(uint8_t c, int16_t x, int16_t y); /* 0x17954 */
void set_clip_counter_strip(void);                  /* 0x0329e */
void draw_counter_word(int16_t value, int16_t x, int16_t y,
                       int16_t all);                /* 0x031e1 */
void draw_counter_long(int32_t value, int16_t x, int16_t y,
                       int16_t all);                /* 0x0323c */
void redraw_counters(void);                         /* 0x0318e */
void start_counters(void);                          /* 0x030b0 */
int32_t parse_base(char *text, int16_t base);    /* 0x03612 */
void score_to_code(int32_t score, char *text); /* 0x033ed */
int32_t score_code_to_score(char *text);         /* 0x034e2 */
void step_counters(void);                           /* 0x030c6 */

/* Borland's DOS file primitives - NOT part of the reconstruction. */
/* The `printf` engine's putter: `(sink, count, bytes)`, answering the count
   or 0. `stream_put_run` for a stream, `string_putn` for a buffer. */
typedef uint16_t (*putn_fn)(void *sink, uint16_t n, const uint8_t *buf);
/* The engine's state, kept in `vprinter`'s frame in the original and reached
   by its helpers through BP; here a struct the helpers take a pointer to. */
struct printer {
    putn_fn  put;                /* [bp+0xa] */
    void    *sink;               /* [bp+8] */
    char     out[0x50];          /* [bp-0x96] */
    char    *cur;                /* di */
    int16_t  room;               /* [bp-0x14] */
    uint16_t total;              /* [bp-0x12] */
    int16_t  failed;             /* [bp-0x16] */
};
uint16_t call_sound_module(uint16_t fn, union sound_module_args * si); /* 0x0c816 */
uint16_t sound_module_install(uint16_t callback, uint16_t flag); /* 0x0c808 */
uint16_t sound_module_set_rate(union sound_module_args * si); /* 0x0c7ec */
uint16_t sound_module_service(union sound_module_args * si); /* 0x0bba6 */
/* **`sound_module_service` as a timer callback.** The timer calls it with no
   arguments and it reads whatever SI holds, which C can only say with a cast;
   the host cannot call it that way, so there it is `sound_module_tick`, which
   hands it the guest's stack as the arguments the way the port's other calls
   into the module do. Ours. */
#ifdef __TURBOC__
#  define SOUND_MODULE_TICK ((void (far *)(void))sound_module_service)
#else
void sound_module_tick(void);
#  define SOUND_MODULE_TICK sound_module_tick
#endif
uint16_t sound_module_9(union sound_module_args * si);  /* 0x0bbb1 */
uint16_t sound_module_10(union sound_module_args * si); /* 0x0bbb8 */
uint16_t sound_module_11(union sound_module_args * si); /* 0x0bbbf */
uint16_t stop_loaded_module(void);                  /* 0x0bbc6 */
uint16_t sound_module_shutdown(void);               /* 0x0bbcd */
uint16_t sound_module_position(uint16_t *a, uint16_t *b, uint16_t *c);
                                                    /* 0x0bbe6 */
char *strcpy_far(char *dst, const char *src); /* 0x0c791 */
char *strcat_far(char *dst, const char *src); /* 0x0c77e */
char *strchr_far(char *s, int16_t c);        /* 0x0c7a4 */
int16_t  fgetc_far(FILE *file);              /* 0x0c7ca */

/* Hand over the next run of bytes from the selected resource. */

/* Select a resource by handle; unpack its entry into the loader globals. */
struct open_file *near find_file_record(FILE *handle);         /* 0x25a7c */
int32_t file_record_size(FILE *handle);         /* 0x25f39 */
int16_t file_record_valid(FILE *handle);         /* 0x25f92 */
int16_t close_file_record(FILE *handle);         /* 0x25f63 */
void near reset_file_record(struct open_file *rec);               /* 0x25aad */
int16_t near string_equal_upto(const char * a, const char * b,
                          uint16_t n);              /* 0x25afa */
struct open_file *copy_file_record(struct open_file *dst, FILE *handle); /* 0x25b32 */
FILE *open_file_record(char *name);           /* 0x25bb6 */
int32_t near restore_file_record(struct open_file *rec);         /* 0x25c1a */
int32_t seek_named_chunk(FILE *handle, const char * path,
                          int16_t index);           /* 0x25c4c */

/* Free a pointer unless it is null. */

/* Hand a block back to DOS; only the pointer's segment is used. */
void dos_free_far(void far *block);            /* 0x237be */

/* Recompute a link's endpoints, then the rest lengths they imply. */
void refresh_link_geometry(struct rope *link);          /* 0x05bf3 */

/* Set an object's vector at +0x36/+0x38 from angle and magnitude. */
void set_vector_from_angle(struct part *obj, uint16_t angle,
                           int16_t mag);            /* 0x07d97 */

/* Rest length less actual separation, at one end of a link. */
int16_t link_slack(struct part *obj, struct rope *link,
                   int16_t gen);                    /* 0x07cd2 */

/* The vector a link has to close, and its approximate length. */
int16_t link_endpoint_gap(struct rope *link, struct part *obj, uint8_t * out_dx,
                          uint8_t * out_dy);         /* 0x08473 */

/* Distance from a link's endpoint to the endpoint it joins. */
int16_t link_end_distance(struct rope *link, int16_t gen,
                          int16_t end);             /* 0x07b33 */

/* Age the state histories of everything about to be stepped. */
void shift_all_histories(void);                     /* 0x087ac */

/* Age every tracked quantity on an object by one step. */
void shift_state_history(struct part *obj);             /* 0x087ed */


/* Intersect two segments; answers whether the point lies on both. */
int16_t intersect_segments(const int16_t *seg1, const int16_t *seg2,
                           uint8_t * out);            /* 0x04783 */

/* Step the second word of each pair one further from the first. */
void step_pair_apart(int16_t *rec);                  /* 0x048fc */

/* Are two points within 140 in both axes? */
int16_t points_within_140(const struct point16 *a,
                          const struct point16 *b);      /* 0x057f0 */

/* Recompute a record's velocity from its movement, then clamp it. */
void update_velocity(struct part *rec, int16_t shift_x, int16_t shift_y,
                     uint16_t which);               /* 0x07df7 */

/* Splice one list onto the front of another and empty the first. */
void release_part_queue(void);              /* 0x0864b */

/* Is a value between two bounds, whichever way round they are? */
int16_t value_between(uint16_t v, uint16_t a, uint16_t b);   /* 0x04935 */

/* Work out the endpoints of the link between a pair of objects. */
void compute_link_endpoints(struct belt *link);         /* 0x05aed */

/* Which side of a range a value falls on, as two flag bytes. */
void set_side_flags(const int16_t *range, int16_t v, struct part_contact *out);   /* 0x004b0 */

/* Insert a record into a sorted doubly-linked list. */
void insert_sorted(struct part *rec, struct part *head);    /* 0x0629d */

/* First of three words that is non-zero and enabled by its flag bit. */
struct part *pick_by_flag(uint16_t flags);          /* 0x0682f */

/* Choose a value for a record: its own, or a shared slot. */
struct part *pick_for_record(struct part *rec,
                             uint16_t flags);    /* 0x0686f */

/* Add one or both of a record's two shapes. */
void add_record_shapes(struct part *rec, uint16_t which);   /* 0x070a8 */

/* Take a node off the free list and fill it in as a shape. */
/* pt1 and pt2 are each an (x, y) pair the routine only reads, so they are
   pointers: a caller's stack locals in some places and a DGROUP record's
   fields in others, and a pointer is the one type that is both. */
void alloc_shape(const uint8_t *pt1, const uint8_t *pt2,
                 uint8_t flags, uint8_t which,
                 int16_t width);                    /* 0x0712e */

/* Which of two structure fields matches a value. */
int16_t link_slot_of(struct part *value, struct part *obj);   /* 0x07aee */

/* Pick one of two record fields by matching the other. */
struct part *rope_other_end(struct part *key, struct rope *rec);   /* 0x07b11 */

/* Present the frame: the game's wrapper around the driver's page flip. */
void present_frame(uint16_t wait_retrace);          /* 0x08cb8 */

/* Clear the input accumulators and latched state. */
void reset_input_state(void);                       /* 0x0c134 */

/* Claim a slot in the two-entry page table at DGROUP 0x56e6. */
struct page_slot *claim_page_slot(uint16_t want);            /* 0x0c070 */
void swap_page_objects(uint16_t page_a, uint16_t page_b); /* 0x0bae5 */

/* Save the driver's drawing state, or put it back. */
void save_or_restore_draw_state(int16_t save);      /* 0x0c0c4 */

/* Wait for the frame, then latch input state and clear the accumulators. */
void wait_and_latch_frame(void);                    /* 0x0b719 */

/* Not transcribed yet; see the source. */

uint16_t load_screen(char *name);                /* 0x27071 */
void     keyboard_isr(void);                        /* 0x22e20 */
void     keyboard_tick_isr(void);                   /* ours: the hook at 0x21386 */
uint16_t bios_read_key(void);                       /* 0x230be */
uint16_t translate_key(uint16_t key);               /* 0x0905a */
int16_t  read_resource_cfg(void);                   /* 0x0f0ef */
void copy_rect_thunk(uint16_t x, uint16_t y, uint16_t width,
                     uint16_t height);              /* 0x22d12 */
void step_and_draw_machine(int16_t redraw_all);     /* 0x17f2d */
void refile_overlapping_parts(void);                /* 0x0771a */
void draw_machine(int16_t a, int16_t b);            /* 0x1854a */
void draw_belt(struct part *part, int16_t a);           /* 0x185e1 */
void draw_curve(uint16_t colour, int16_t shift,
                int32_t x0, int32_t x1, int32_t x2,
                int32_t y0, int32_t y1, int32_t y2); /* 0x18764 */
void draw_rope_segment(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t slack);              /* 0x18920 */
void draw_rope(struct part *part, int16_t a);           /* 0x18996 */
void draw_part(struct part *part, uint8_t level,
               int16_t a, int16_t b);               /* 0x18b94 */
void draw_part_extra(struct part *part);                /* 0x18fe9 */
void draw_polygon(int16_t n, const int16_t *xs,
                  const int16_t *ys);                  /* 0x20a77 */
void draw_bitmap_scaled(struct bitmap *hdr, int16_t x, int16_t y,
                        int16_t w, int16_t h,
                        uint16_t mode);             /* 0x0c60b */
#ifndef TIM_BITMAPS_C
/* bitmaps.c's own declaration of it is near, and wrong - see
   `draw_bitmap_scaled_248f`. */
void blit_scaled_a(struct bitmap *bmp, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h); /* 0x24436 */
#endif
void blit_scaled_b(struct bitmap *bmp, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h); /* 0x2257d */
struct part *find_part_from(struct part *rec);              /* 0x0509d */
int16_t  belt_ends_close(struct belt *belt);            /* 0x0582a */
int16_t  point_in_play_area(void);                  /* 0x08baf */
void draw_bitmap_centred(struct bitmap *bmp, int16_t x, int16_t y,
                         int16_t w, int16_t h); /* 0x17e27 */
void draw_panel(int16_t x, int16_t y, int16_t w, int16_t h); /* 0x17080 */
void draw_scroll_text(const char *str, int16_t x, int16_t y, int16_t w); /* 0x16ebf */
void show_level_complete(void);                      /* 0x1779e */
void free_all_lists(void);                          /* 0x16b42 */
void clear_parts_bin(void);                                 /* 0x16b2d */
void free_part_list(struct part *si);                       /* 0x16b70 */
void load_animation(char *name);             /* 0x1419d */
uint16_t game_fread_byte(FILE *file, uint8_t * buf); /* 0x135cd */
void game_fread_line(FILE *file, char *buf);  /* 0x1361f */
void read_password_line(int16_t count, char *buf); /* 0x1442a */
void game_setbuf(FILE *file, uint8_t *buf);  /* 0x0a236 */
void game_fread_string(FILE *file, char *buf);/* 0x13600 */
void alloc_part_table(int16_t n);                   /* 0x1357f */
void read_list(FILE *file, struct part *head, int16_t n);   /* 0x13a36 */
void read_record_fields(FILE *file, struct part *rec);      /* 0x13653 */
void build_part_list(void);                         /* 0x15c97 */
void free_two_bitmap_lists(void);                   /* 0x0ff45 */
void free_all_part_bitmaps(void);                   /* 0x107f0 */
void free_part_bitmap(uint16_t n);                  /* 0x10808 */
void load_part_bitmap(uint16_t n);                  /* 0x10770 */
uint16_t part_init_bowling_ball(struct part *part);           /* 0ecc:71d4, 0x15e94 */
uint16_t part_init_platform(struct part *part);           /* 0ecc:7203, 0x15ec3 */
uint16_t part_init_ramp(struct part *part);           /* 0ecc:723b, 0x15efb */
uint16_t part_init_seesaw(struct part *part);           /* 0ecc:727e, 0x15f3e */
uint16_t part_init_balloon(struct part *part);           /* 0ecc:72b6, 0x15f76 */
uint16_t part_init_conveyor(struct part *part);           /* 0ecc:72f5, 0x15fb5 */
uint16_t part_init_mouse_cage(struct part *part);           /* 0ecc:7345, 0x16005 */
uint16_t part_init_pulley(struct part *part);           /* 0ecc:738b, 0x1604b */
uint16_t part_init_belt(struct part *part);           /* 0ecc:73cb, 0x1608b */
uint16_t part_init_basketball(struct part *part);           /* 0ecc:73f8, 0x160b8 */
uint16_t part_init_rope(struct part *part);           /* 0ecc:7427, 0x160e7 */
uint16_t part_init_bird_cage(struct part *part);           /* 0ecc:7453, 0x16113 */
uint16_t part_init_pokey(struct part *part);           /* 0ecc:7492, 0x16152 */
uint16_t part_init_jack_in_the_box(struct part *part);           /* 0ecc:74cb, 0x1618b */
uint16_t part_init_gear(struct part *part);           /* 0ecc:7511, 0x161d1 */
uint16_t part_init_bob_the_fish(struct part *part);           /* 0ecc:7551, 0x16211 */
uint16_t part_init_bellow(struct part *part);           /* 0ecc:7585, 0x16245 */
uint16_t part_init_bucket(struct part *part);           /* 0ecc:75b9, 0x16279 */
uint16_t part_init_cannon(struct part *part);           /* 0ecc:75f8, 0x162b8 */
uint16_t part_init_dynamite(struct part *part);           /* 0ecc:7635, 0x162f5 */
uint16_t part_init_bullet(struct part *part);           /* 0ecc:7672, 0x16332 */
uint16_t part_init_electric_plug(struct part *part);           /* 0ecc:76a1, 0x16361 */
uint16_t part_init_dynamite_plunger(struct part *part);           /* 0ecc:76de, 0x1639e */
uint16_t part_init_hook(struct part *part);           /* 0ecc:7717, 0x163d7 */
uint16_t part_init_fan(struct part *part);           /* 0ecc:7733, 0x163f3 */
uint16_t part_init_flashlight(struct part *part);           /* 0ecc:7770, 0x16430 */
uint16_t part_init_generator(struct part *part);           /* 0ecc:77a4, 0x16464 */
uint16_t part_init_gun(struct part *part);           /* 0ecc:77dc, 0x1649c */
uint16_t part_init_baseball(struct part *part);           /* 0ecc:7815, 0x164d5 */
uint16_t part_init_light(struct part *part);           /* 0ecc:7844, 0x16504 */
uint16_t part_init_magnifying_glass(struct part *part);           /* 0ecc:7861, 0x16521 */
uint16_t part_init_monkey(struct part *part);           /* 0ecc:7879, 0x16539 */
uint16_t part_init_pumpkin(struct part *part);           /* 0ecc:78b2, 0x16572 */
uint16_t part_init_heart_balloon(struct part *part);           /* 0ecc:78e1, 0x165a1 */
uint16_t part_init_christmas_tree(struct part *part);           /* 0ecc:7920, 0x165e0 */
uint16_t part_init_boxing_glove(struct part *part);           /* 0ecc:794f, 0x1660f */
uint16_t part_init_rocket(struct part *part);           /* 0ecc:7983, 0x16643 */
uint16_t part_init_scissors(struct part *part);           /* 0ecc:79bb, 0x1667b */
uint16_t part_init_solar_panel(struct part *part);           /* 0ecc:79f4, 0x166b4 */
uint16_t part_init_trampoline(struct part *part);           /* 0ecc:7a09, 0x166c9 */
uint16_t part_init_windmill(struct part *part);           /* 0ecc:7a3d, 0x166fd */
uint16_t part_init_mort_the_mouse(struct part *part);           /* 0ecc:7a83, 0x16743 */
uint16_t part_init_cannon_ball(struct part *part);           /* 0ecc:7abc, 0x1677c */
uint16_t part_init_tennis_ball(struct part *part);           /* 0ecc:7aeb, 0x167ab */
uint16_t part_init_candle(struct part *part);           /* 0ecc:7b1a, 0x167da */
uint16_t part_init_corner_pipe(struct part *part);           /* 0ecc:7b56, 0x16816 */
uint16_t part_init_anchor(struct part *part);           /* 0ecc:7b8a, 0x1684a */
uint16_t part_init_motor(struct part *part);           /* 0ecc:7ba2, 0x16862 */
uint16_t part_init_kind_55(struct part *part);           /* 0ecc:7cad, 0x1696d */
uint16_t part_init_kind_56(struct part *part);           /* 0ecc:7cf7, 0x169b7 */
uint16_t part_init_kind_57(struct part *part);           /* 0ecc:7d26, 0x169e6 */
void part_finish_angles(struct part *part);             /* 0x069e8 */
void part_setup_boxing_glove(struct part *part);                /* 190f:064c, 0x1973c */
void part_setup_kind_56(struct part *part);                /* 190f:10bd, 0x1a1ad */
void part_setup_kinds_55_57(struct part *part);                /* 190f:110c, 0x1a1fc */
void part_setup_motor(struct part *part);                /* 190f:144d, 0x1a53d */
void part_setup_electric_plug(struct part *part);                /* 190f:155a, 0x1a64a */
void part_setup_gear(struct part *part);                /* 190f:202a, 0x1b11a */
void part_setup_light(struct part *part);                /* 190f:2aff, 0x1bbef */
void part_setup_solar_panel(struct part *part);                /* 190f:3d55, 0x1ce45 */
void part_setup_big_ball(struct part *part);                /* 190f:000c, 0x190fc */
void part_setup_cannon_ball(struct part *part);                /* 190f:0070, 0x19160 */
void part_setup_small_ball(struct part *part);                /* 190f:00d4, 0x191c4 */
void part_setup_bucket(struct part *part);                /* 190f:07a2, 0x19892 */
void part_setup_candle(struct part *part);                /* 190f:093d, 0x19a2d */
void part_setup_bird_cage(struct part *part);                /* 190f:0f79, 0x1a069 */
void part_setup_conveyor(struct part *part);                /* 190f:248a, 0x1b57a */
void part_setup_jack_in_the_box(struct part *part);                /* 190f:290c, 0x1b9fc */
void part_setup_mouse_cage(struct part *part);                /* 190f:2e70, 0x1bf60 */
void part_setup_mort_the_mouse(struct part *part);                /* 190f:33ea, 0x1c4da */
void part_setup_rocket(struct part *part);                /* 190f:370a, 0x1c7fa */
void part_setup_trampoline(struct part *part);                /* 190f:3f08, 0x1cff8 */
void part_setup_platform(struct part *part);                /* 190f:47cc, 0x1d8bc */
void part_setup_windmill(struct part *part);                /* 190f:488a, 0x1d97a */
void part_setup_balloon(struct part *part);                /* 190f:0138, 0x19228 */
void part_setup_bellow(struct part *part);                /* 190f:035f, 0x1944f */
void part_setup_bullet(struct part *part);                /* 190f:088d, 0x1997d */
void part_setup_cannon(struct part *part);                /* 190f:0b7a, 0x19c6a */
void part_setup_gun(struct part *part);                /* 190f:2373, 0x1b463 */
void part_setup_dynamite_plunger(struct part *part);                /* 190f:3216, 0x1c306 */
void part_setup_dynamite(struct part *part);                /* 190f:127e, 0x1a36e */
void part_setup_hook(struct part *part);                /* 190f:19ba, 0x1aaaa */
void part_setup_monkey(struct part *part);                /* 190f:2c5e, 0x1bd4e */
void part_setup_pokey(struct part *part);                /* 190f:0c0b, 0x19cfb */
void part_setup_fan(struct part *part);                /* 190f:1a11, 0x1ab01 */
void part_setup_bob_the_fish(struct part *part);                /* 190f:1bce, 0x1acbe */
void part_setup_flashlight(struct part *part);                /* 190f:1d03, 0x1adf3 */
void part_setup_generator(struct part *part);                /* 190f:1dc3, 0x1aeb3 */
void part_setup_heart_balloon(struct part *part);                /* 190f:263d, 0x1b72d */
void part_setup_corner_pipe(struct part *part);                   /* 190f:374e, 0x1c83e */
void part_setup_ramp(struct part *part);                          /* 190f:26e1, 0x1b7d1 */
void part_setup_pumpkin(struct part *part);                /* 190f:35bf, 0x1c6af */
void part_setup_scissors(struct part *part);                /* 190f:3861, 0x1c951 */
void part_setup_christmas_tree(struct part *part);                /* 190f:107c, 0x1a16c */
void part_setup_seesaw(struct part *part);                /* 190f:406d, 0x1d15d */
struct part *make_part(uint16_t kind);                     /* 0x15d9b */
void free_part(struct part *part);                      /* 0x16b94 */
void load_all_parts(void);                          /* 0x10722 */
void draw_frame_corners(struct bitmap **rec);              /* 0x0fddf */
void draw_answer_slot(struct bitmap *bmp, uint16_t slot);  /* 0x0fd64 */
void redraw_cursor_all(void);                       /* 0x0bcc5 */
void copy_protect_screen(struct bitmap **bitmaps);                         /* 0x0f9e2 */
void restore_object_backdrop(uint16_t from_page,
                             uint16_t to_page);      /* 0x0ba48 */
void restore_saved_rect_lists(int16_t which);       /* 0x0b06d */
void restore_saved_rects(vga_page_t page_src, vga_page_t page_dst, uint16_t refcount); /* 0x0b26f */
void free_saved_rects(vga_page_t page_src, vga_page_t page_dst, uint16_t refcount); /* 0x0b31a */
struct rect_list_entry **find_saved_rect_slot(vga_page_t page_src, vga_page_t page_dst,
                              uint16_t refcount);        /* 0x0b225 */
char far *far_strchr(const char far *s, char c);                  /* 0x0ac03 */
char far *far_strcat(char far *dst, const char far *src);         /* 0x0ac48 */
uint16_t build_rect_pool(uint16_t n);                             /* 0x0aca2 */
void     file_saved_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t mode, vga_page_t page_src, vga_page_t page_dst,
                         uint16_t refcount, uint8_t far * buf);  /* 0x0ad1a */
void     discard_saved_rects(void);                               /* 0x0b102 */
uint16_t saved_rect_covers(int16_t x, int16_t y, int16_t w, int16_t h,
                           vga_page_t page_dst, uint16_t refcount); /* 0x0b13c */
void     free_rect_pool(void);                                    /* 0x0b1e4 */
uint16_t rect_pool_count(void);                                   /* 0x0b21b */
void     copy_saved_rects(vga_page_t from_src, vga_page_t from_dst, uint16_t from_ref,
                          vga_page_t to_src, vga_page_t to_dst, uint16_t to_ref); /* 0x0b35a */
void clear_object_covered(uint16_t page);           /* 0x0bb31 */
void copy_rect_around_cursor(int16_t x, int16_t y,
                             int16_t w, int16_t h); /* 0x0bed6 */
void move_pointer_to(int16_t x, int16_t y);         /* 0x0b6c5 */
void vm_set_line_compare(uint16_t line);                         /* 0x09bdd */
void regions_handle_pointer(struct region *first);        /* 0x09084 */

/*
 * OURS: the port cannot call through a far pointer held in guest memory, so a
 * region's two handlers are dispatched on their value. See machine.c.
 */
void stop_music_or_effect(int16_t id);              /* 0x08eed */
void play_sound(int16_t id);                        /* 0x08ea6 */
void select_music(int16_t id);                      /* 0x08e5f */
void restore_cursor_following(void);                /* 0x08c17 */
void show_cursor_again(void);                               /* 0x08bfd */
void reset_machine(void);                           /* 0x0892b */
void replay_shapes(void);                           /* 0x0729d */
void clear_machine(void);                           /* 0x013cd */
void restart_machine(void);                         /* 0x0141b */
void pause_machine(void);                           /* 0x01438 */
void unlink_part(struct part *part);                /* 0x0627f */
void step_machine(void);                            /* 0x00e89 */
void step_moving_object(struct part *obj);              /* 0x0118c */
void collect_carried(struct part *obj);                 /* 0x0453a */
void carry_riders_along(struct part *obj);              /* 0x04667 */
void bounce_off_contact(struct part *obj);              /* 0x03c1c */
void bounce_pair(struct part *obj);                       /* 0x03df7 */
void part_moved(struct part *part);                     /* 0x0793f */
void free_all_shapes(void);                             /* 0x06abc */
int16_t other_end_direction(struct part *part);         /* 0x07d7b */
int16_t game_feof(FILE *file);                          /* 0x0a136 */
void request_archive_reopen(void);                      /* 0x0a45b */
void interrupt crit_error_handler(uint16_t bp, uint16_t di, uint16_t si,
                                  uint16_t ds, uint16_t es, uint16_t dx,
                                  uint16_t cx, uint16_t bx, uint16_t ax); /* 0x0a822 */
void draw_xor_rect(int16_t x, int16_t y, int16_t w, int16_t h); /* 0x0a866 */
int16_t far_strlen(const char far *s);                  /* 0x0aa8f */
char far *far_strcpy(char far *dst, const char far *src); /* 0x0aab3 */
char far *far_strncpy(char far *dst, const char far *src, int16_t n); /* 0x0aae8 */
int16_t far_strnicmp(const char far *a, const char far *b, uint16_t n); /* 0x0ab3b */
void rope_in_dirty_rect(struct part *part);             /* 0x07570 */
void mark_parts_in_dirty_rects(void);               /* 0x073f5 */
void add_carried_weight(struct part *obj);              /* 0x08744 */
void add_mass_capped(struct part *obj, struct part *other); /* 0x08765 */
/* **A kind's step and hit hooks, called through its record** - inline in
   the original, `lcall [bx+0x0ecc]` and `lcall [bx+0x0ec8]` with `bx` the
   kind times 0x3a, which is what these expand to, and the flip hook at
   `[bx+0x0ed4]` the same way. Ours in name. */
#define part_step(part)        (g_part_kinds[(part)->kind].step(part))
#define part_hit(kind, part)   (g_part_kinds[kind].hit(part))
#define part_flip(part, how)   (g_part_kinds[(part)->kind].flip((part), (how)))
uint16_t part_hit_bellow(struct part *part);              /* 0x19415 */
void     nudge_x_add(struct part *obj, int16_t d);      /* 0x1afc2 */
void     nudge_x_sub(struct part *obj, int16_t d);      /* 0x1afdc */
void     nudge_y_add(struct part *obj, int16_t d);      /* 0x1affa */
void     nudge_y_sub(struct part *obj, int16_t d);      /* 0x1b014 */
uint16_t part_hit_gear(struct part *part);              /* 0x1b032 */
void part_step_monkey(struct part *part);             /* 0x1bdbd */
uint16_t part_hit_monkey(struct part *part);              /* 0x1bd05 */
uint16_t part_hit_bucket(struct part *part);              /* 0x19847 */
uint16_t part_hit_bullet(struct part *part);              /* 0x19945 */
uint16_t part_hit_dynamite(struct part *part);              /* 0x1a348 */
void         part_settle_conveyor(struct part *part);           /* 0x1b6cd */
void         part_settle_ramp(struct part *part);           /* 0x1b82f */
void         part_settle_platform(struct part *part);           /* 0x1d908 */
uint16_t part_drive_bird_cage(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, int32_t p6);              /* 0x1a0f5 */
uint16_t part_drive_heart_balloon(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, int32_t p6);              /* 0x1b76e */
uint16_t part_drive_dynamite_plunger(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, int32_t p6);              /* 0x1c48a */
uint16_t part_drive_seesaw(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, int32_t p6);              /* 0x1d52d */
uint16_t part_hit_dynamite_plunger(struct part *part);              /* 0x1c2b8 */
uint16_t part_drive_monkey(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, int32_t p6);              /* 0x1becc */
void part_step_dynamite_plunger(struct part *part);             /* 0x1c399 */
void part_step_solar_panel(struct part *part);             /* 0x1ce68 */
uint16_t part_drive_balloon(struct part *p1, struct part *p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, int32_t p6);              /* 0x193b2 */
void     part_flip_bellow(struct part *part);             /* 0x194ad */
void     part_flip_boxing_glove(struct part *part);             /* 0x197a6 */
void     part_flip_cannon(struct part *part);             /* 0x19cc8 */
void     part_flip_pokey(struct part *part);             /* 0x1a036 */
void     part_flip_dynamite(struct part *part);             /* 0x1a406 */
void     part_flip_motor(struct part *part);             /* 0x1a5a0 */
void     part_flip_electric_plug(struct part *part);             /* 0x1a6ed */
void     part_flip_hook(struct part *part);             /* 0x1aac9 */
void     part_flip_fan(struct part *part);             /* 0x1ac92 */
void     part_flip_flashlight(struct part *part);             /* 0x1ae73 */
void     part_flip_gun(struct part *part);             /* 0x1b4c1 */
void     part_flip_jack_in_the_box(struct part *part);             /* 0x1ba38 */
void     part_flip_light(struct part *part);             /* 0x1bc49 */
void     part_flip_monkey(struct part *part);             /* 0x1be8d */
void     part_flip_magnifying_glass(struct part *part);             /* 0x1c228 */
void     part_flip_dynamite_plunger(struct part *part);             /* 0x1c452 */
void     part_flip_mort_the_mouse(struct part *part);             /* 0x1c682 */
void     part_flip_corner_pipe(struct part *part, uint16_t which); /* 0x1c8a5 */
void     part_flip_scissors(struct part *part);             /* 0x1c9f7 */
void     part_flip_seesaw(struct part *part);             /* 0x1d226 */
void     part_flip_windmill(struct part *part);             /* 0x1da2f */
void part_step_bellow(struct part *part);             /* 0x194e0 */
void goal_test_puzzle_2(void);                          /* 0x014bf */
void goal_test_puzzle_1(void);                          /* 0x01564 */
void goal_test_pop_balloons(void);                          /* 0x01645 */
void goal_test_puzzle_4(void);                          /* 0x01cfe */
void goal_test_puzzle_5(void);                          /* 0x01d24 */
void goal_test_puzzles_6_58(void);                          /* 0x01d57 */
void goal_test_puzzles_7_51_65(void);                          /* 0x01d98 */
void goal_test_puzzle_20(void);                            /* 0x014f6 */
void goal_test_puzzle_21(void);                            /* 0x01515 */
void goal_test_puzzle_22(void);                            /* 0x01537 */
void goal_test_puzzle_23(void);                            /* 0x016eb */
void goal_test_puzzle_26(void);                            /* 0x01740 */
void goal_test_puzzle_43(void);                            /* 0x01772 */
void goal_test_puzzle_39(void);                            /* 0x01798 */
void goal_test_puzzle_25(void);                            /* 0x01820 */
void goal_test_puzzle_41(void);                            /* 0x0185e */
void goal_test_puzzles_10_32(void);                            /* 0x0188b */
void goal_test_puzzle_46(void);                            /* 0x018cd */
void goal_test_puzzle_34(void);                            /* 0x0191e */
void goal_test_puzzles_14_15_64_73(void);                            /* 0x0194c */
void goal_test_puzzle_24(void);                            /* 0x0197a */
void goal_test_puzzle_38(void);                            /* 0x019f1 */
void goal_test_puzzle_44(void);                            /* 0x01a2c */
void goal_test_puzzles_16_56_83(void);                            /* 0x01a95 */
void goal_test_puzzle_47(void);                            /* 0x01afc */
void goal_test_puzzles_19_48(void);                            /* 0x01bd5 */
void goal_test_puzzle_9(void);                            /* 0x01dc6 */
void goal_test_puzzle_11(void);                            /* 0x01df5 */
void goal_test_puzzle_12(void);                            /* 0x01e2b */
void goal_test_puzzle_13(void);                            /* 0x01e58 */
void goal_test_puzzle_17(void);                            /* 0x01e93 */
void goal_test_puzzle_18(void);                            /* 0x01ef3 */
void goal_test_puzzles_42_75(void);                            /* 0x01f5f */
void goal_test_puzzle_31(void);                            /* 0x01fe0 */
void goal_test_puzzle_29(void);                            /* 0x0204a */
void goal_test_puzzle_28(void);                            /* 0x0209f */
void goal_test_puzzle_36(void);                            /* 0x02134 */
void goal_test_puzzle_37(void);                            /* 0x0229a */
void goal_test_puzzle_40(void);                            /* 0x02429 */
void goal_test_puzzle_35(void);                            /* 0x02466 */
void goal_test_puzzle_88(void);                          /* 0x024a1 */
void goal_test_puzzle_89(void);                          /* 0x024e3 */
void goal_test_puzzle_90(void);                          /* 0x02512 */
void goal_test_puzzle_91(void);                          /* 0x0251b */
void goal_test_puzzle_92(void);                          /* 0x02552 */
void goal_test_puzzle_93(void);                          /* 0x0255b */
void goal_test_puzzle_94(void);                          /* 0x0258a */
void goal_test_puzzle_95(void);                          /* 0x025b9 */
void goal_test_puzzle_96(void);                          /* 0x025fb */
void goal_test_puzzle_97(void);                          /* 0x02621 */
void goal_test_puzzle_98(void);                          /* 0x0266b */
void goal_test_puzzle_99(void);                          /* 0x02674 */
void goal_test_puzzle_100(void);                         /* 0x026ab */
void goal_test_puzzle_101(void);                         /* 0x026b4 */
void goal_test_puzzle_102(void);                         /* 0x02716 */
void goal_test_puzzle_103(void);                         /* 0x0271f */
void goal_test_puzzle_104(void);                         /* 0x02760 */
void goal_test_puzzle_105(void);                         /* 0x02798 */
void goal_test_puzzle_106(void);                         /* 0x027e7 */
void goal_test_puzzle_107(void);                         /* 0x02812 */
void goal_test_puzzle_108(void);                         /* 0x02846 */
void goal_test_puzzle_109(void);                         /* 0x0284f */
void goal_test_puzzle_110(void);                         /* 0x0287d */
void goal_test_puzzle_111(void);                         /* 0x02886 */
void goal_test_puzzle_112(void);                         /* 0x028d7 */
void goal_test_puzzle_113(void);                         /* 0x0292a */
void goal_test_puzzle_114(void);                         /* 0x02990 */
void goal_test_puzzle_115(void);                         /* 0x029bf */
void goal_test_puzzle_116(void);                         /* 0x029f2 */
void goal_test_puzzle_117(void);                         /* 0x02a25 */
void goal_test_puzzle_118(void);                         /* 0x02a6e */
void goal_test_puzzle_119(void);                         /* 0x02a9d */
void goal_test_puzzle_120(void);                         /* 0x02ad3 */
void goal_test_puzzle_121(void);                         /* 0x02adc */
void goal_test_puzzle_122(void);                         /* 0x02ae5 */
void goal_test_puzzle_123(void);                         /* 0x02aee */
void goal_test_puzzle_124(void);                         /* 0x02af7 */
void goal_test_puzzle_125(void);                         /* 0x02b00 */
void goal_test_puzzle_126(void);                         /* 0x02b34 */
void goal_test_puzzle_127(void);                         /* 0x02b3d */
void goal_test_puzzle_128(void);                         /* 0x02b6c */
void goal_test_puzzle_129(void);                         /* 0x02b75 */
void goal_test_puzzle_130(void);                         /* 0x02bab */
void goal_test_puzzle_131(void);                         /* 0x02bb4 */
void goal_test_puzzle_132(void);                         /* 0x02bbd */
void goal_test_puzzle_133(void);                         /* 0x02bc6 */
void goal_test_puzzle_134(void);                         /* 0x02c01 */
void goal_test_puzzle_135(void);                         /* 0x02c35 */
void goal_test_puzzle_136(void);                         /* 0x02c3e */
void goal_test_puzzle_137(void);                         /* 0x02c98 */
void goal_test_puzzle_138(void);                         /* 0x02ced */
void goal_test_puzzle_139(void);                         /* 0x02d7c */
void goal_test_puzzle_140(void);                         /* 0x02d85 */
void goal_test_puzzle_141(void);                         /* 0x02e25 */
void goal_test_puzzle_142(void);                         /* 0x02e2e */
void goal_test_puzzle_143(void);                         /* 0x02e84 */
void goal_test_puzzle_144(void);                         /* 0x02e8d */
void goal_test_puzzle_145(void);                         /* 0x02e96 */
void goal_test_puzzle_146(void);                         /* 0x02ed9 */
void goal_test_puzzle_147(void);                         /* 0x02ee2 */
void goal_test_puzzle_148(void);                         /* 0x02f24 */
void goal_test_puzzle_149(void);                         /* 0x02f2d */
void goal_test_puzzle_150(void);                         /* 0x02f36 */
void goal_test_puzzle_151(void);                         /* 0x02f64 */
void goal_test_puzzle_152(void);                         /* 0x02f9e */
void goal_test_puzzle_153(void);                         /* 0x02fd9 */
void goal_test_puzzle_154(void);                         /* 0x03012 */
void goal_test_puzzle_155(void);                         /* 0x0301b */
void goal_test_puzzle_156(void);                         /* 0x0305e */
void goal_test_puzzle_157(void);                         /* 0x0308c */
void goal_test_puzzle_158(void);                         /* 0x03095 */
void goal_test_puzzle_159(void);                         /* 0x0309e */
void goal_test_puzzle_160(void);                         /* 0x030a7 */
void goal_test_puzzle_78(void);                            /* 0x0159b */
void goal_test_puzzle_79(void);                            /* 0x0167b */
void goal_test_puzzles_53_54_63_67_87(void);                            /* 0x017f2 */
void goal_test_puzzle_55(void);                            /* 0x019c3 */
void goal_test_puzzle_71(void);                            /* 0x01a58 */
void goal_test_puzzle_80(void);                            /* 0x01ac3 */
void goal_test_puzzle_70(void);                            /* 0x01b43 */
void goal_test_puzzle_69(void);                            /* 0x01b7b */
void goal_test_puzzle_52(void);                            /* 0x01baf */
void goal_test_puzzle_82(void);                            /* 0x01c25 */
void goal_test_puzzle_81(void);                            /* 0x01c56 */
void goal_test_puzzle_76(void);                            /* 0x01f20 */
void goal_test_puzzles_57_74(void);                            /* 0x01fb1 */
void goal_test_puzzle_66(void);                            /* 0x0201d */
void goal_test_puzzle_61(void);                            /* 0x02079 */
void goal_test_puzzle_86(void);                            /* 0x021ac */
void goal_test_puzzle_77(void);                            /* 0x021e0 */
void goal_test_puzzle_85(void);                            /* 0x02237 */
void goal_test_puzzle_60(void);                            /* 0x0226b */
void goal_test_puzzle_84(void);                            /* 0x022cc */
void goal_test_puzzle_68(void);                            /* 0x02312 */
void goal_test_puzzle_72(void);                            /* 0x0235c */
void goal_test_puzzle_59(void);                            /* 0x0238b */
void goal_test_puzzle_49(void);                            /* 0x023de */
void check_goal(void);                              /* 0x014ae */
struct part *find_rope_anchor(int16_t *out_end, struct part *rec); /* 0x0515d */
void retension_pulleys(struct part *part);              /* 0x05953 */
void rehome_carried_part(void);                     /* 0x05d14 */
uint16_t part_flip_options(struct part *part);          /* 0x05314 */
uint16_t part_handle_at_pointer(struct part *part);     /* 0x05423 */
void pointer_frame(void);                           /* 0x10bd6 */
void move_carried(void);                            /* 0x10e2e */
void move_carried_belt(void);                       /* 0x10e63 */
void move_carried_rope(void);                       /* 0x10f64 */
void scroll_play_area(void);                        /* 0x10d4f */
void draw_carried_icon(void);                       /* 0x17eab */
void draw_part_selection(struct part *part, int16_t which, int16_t flags); /* 0x17fb0 */
void part_flip_ramp(struct part *part);                 /* 0x1b85c */
void part_flip_mouse_cage(struct part *part);                 /* 0x1c031 */
uint16_t part_drive_bucket(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5,
                         int32_t momentum);              /* 172c:0802 */
uint16_t part_drive_kind_57(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5,
                         int32_t momentum);              /* 172c:11d2 */
uint16_t part_drive_gun(struct part *p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t p6);                   /* 172c:2451 */
uint16_t part_drive_light(struct part *p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t p6);              /* 172c:2c19 */
/* **A part's drive hook**, the far pointer at +0x36 of its kind's record,
   called through the record inline the way `part_step` is. It takes six
   arguments where the other hooks take one - **six and not seven, because
   the last is a `long`**: the original pushes seven words and the last two
   are the driving part's momentum, low half first, which three of the hooks
   put straight back together to compare against their own. `tools/verify.py`
   is where the guest's two words become the value. Ours in name. */
#define part_drive(by, p1, p2, p3, p4, p5, p6) \
    (g_part_kinds[(by)->kind].drive((p1), (p2), (p3), (p4), (p5), (p6)))
uint16_t drive_ropes(struct part *from, struct part *part, uint16_t flags,
                     uint16_t a, int32_t momentum);      /* 172c:461a */
uint16_t part_hit_trampoline(struct part *part);              /* 172c:3ebf */
void part_step_trampoline(struct part *part);             /* 172c:3fae */
uint16_t part_hit_seesaw(struct part *part);              /* 172c:3fe8 */
void part_step_seesaw(struct part *part);             /* 172c:420f */
struct part *belt_other_end(struct part *part);             /* 0x07970 */
void link_objects_in_range(struct part *obj, uint16_t flags,
                           int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1);  /* 0x042da */
void link_objects_crossing(struct part *obj, uint16_t flags,
                           const int16_t *line);     /* 0x0437d */
void link_objects_at_point(struct part *obj, int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1);  /* 0x0448e */
void     seg172c_nothing(void);                     /* 172c:0000 */
void     sound_on_hard_impact(struct part *obj);        /* 0x03bdf */
void     mark_needs_refile(struct part *part, int16_t n); /* 0x065c1 */
void     mark_rope_shapes(struct part *part, uint16_t mode); /* 0x06c3d */
void     mark_joined_shapes(struct part *part, uint16_t mode); /* 0x06b30 */
void     mark_part_shapes(struct part *part, uint16_t mode); /* 0x070fb */
int16_t  outlines_cross(struct part *a, struct part *b);    /* 0x04b53 */
int16_t kinds_may_overlap(int16_t a, int16_t b);      /* 0x049e5 */
int16_t  object_overlaps_any(struct part *obj);         /* 0x04a36 */
int16_t  queue_part(struct part *src, struct part *part);   /* 0x0867c */
int16_t  tension_rope(struct part *part);               /* 0x07e3b */
int16_t  rope_orientation(struct rope *rope, int16_t which,
                          int16_t dir);             /* 0x07996 */
uint16_t part_hit_balloon(struct part *part);              /* 172c:016e */
uint16_t part_hit_generator(struct part *part);              /* 172c:1de0 */
uint16_t part_hit_light(struct part *part);              /* 172c:2b7e */
uint16_t part_hit_mouse_cage(struct part *part);              /* 172c:2f25 */
void part_step_balloon(struct part *part);             /* 172c:018e */
uint16_t part_hit_boxing_glove(struct part *part);              /* 172c:0552 */
void part_step_boxing_glove(struct part *part);             /* 172c:057e */
void part_step_bullet(struct part *part);             /* 172c:08f1 */
void part_step_candle(struct part *part);             /* 172c:098a */
void part_step_cannon(struct part *part);             /* 172c:0a5d */
void part_step_pokey(struct part *part);             /* 172c:0ca3 */
void part_step_kind_57(struct part *part);             /* 172c:11a6 */
int16_t  bounce_speed_for_mass(struct part *obj);       /* 172c:06f9 */
void     break_bob_the_fish(struct part *part);              /* 172c:1c9e */
void     trigger_mouse_cage(struct part *part);             /* 172c:2ffd */
int16_t  push_speed_for_mass(struct part *obj);         /* 172c:471f */
void     trigger_things_at(struct part *part, int16_t mode,
                           int16_t dx);             /* 172c:477d */
void     iff_write_be(uint8_t *p, int16_t count, int16_t size,
                      FILE *f);          /* 172c:4a54 */
void     iff_write_cmap(FILE *f);        /* 172c:4aea */
void     iff_write_body(FILE *f);        /* 172c:4b6c */
void     iff_save(char *name);                      /* 172c:4c21 */
void     save_screenshot(char *name);               /* 172c:4d90 */
void     vga_set_dac(const uint8_t *rgb, int16_t first,
                     int16_t count);                /* 172c:4dc7 */
void     vga_get_dac(uint8_t *rgb, int16_t first,
                     int16_t count);                /* 172c:4e02 */
void     chunky_to_planar(const uint8_t *src, uint8_t *dst); /* 172c:4e3f */
uint16_t part_hit_pokey(struct part *part);              /* 172c:0c6c */
void part_step_dynamite(struct part *part);             /* 172c:12c2 */
void     burst_dynamite(struct part *part);              /* 172c:1328 */
void part_step_motor(struct part *part);             /* 172c:13c9 */
uint16_t part_hit_electric_plug(struct part *part);              /* 172c:14d3 */
void part_step_electric_plug(struct part *part);             /* 172c:15ce */
void part_step_gear(struct part *part);             /* 172c:20fc */
void part_step_gun(struct part *part);             /* 172c:22ae */
uint16_t part_hit_conveyor(struct part *part);              /* 172c:2514 */
void part_step_conveyor(struct part *part);             /* 172c:2592 */
void part_step_mouse_cage(struct part *part);             /* 172c:2f3e */
void     part_setup_magnifying_glass(struct part *part);            /* 172c:3030 */
void part_step_magnifying_glass(struct part *part);             /* 172c:3035 */
void part_step_mort_the_mouse(struct part *part);             /* 172c:34d0 */
uint16_t part_hit_scissors(struct part *part);              /* 172c:3824 */
void part_step_rocket(struct part *part);             /* 172c:3635 */
void part_step_scissors(struct part *part);             /* 172c:38fc */
void     cut_ropes(struct part *part, const int16_t *line);   /* 172c:3970 */
void grab_distance(struct part *a, struct part *b,
                   int16_t *out_x, int16_t *out_y); /* 172c:31dc */
uint16_t spread_gear_signal(struct part *from, struct part *to, int16_t how,
                            uint16_t flag);         /* 172c:105d */
void settle_gear_signal(struct part *part, int16_t clear); /* 172c:1225 */
void part_step_blast(struct part *part);             /* 172c:1649 */
int16_t  blast_speed_for_mass(struct part *part);       /* 172c:1748 */
void     split_part_at(struct part *part, struct part *blast); /* 172c:17bc */
int16_t  angle_between_centres(struct part *a, struct part *b); /* 0x0496e */
void part_step_fan(struct part *part);             /* 172c:1a82 */
uint16_t part_hit_bob_the_fish(struct part *part);              /* 172c:1c39 */
uint16_t part_hit_flashlight(struct part *part);              /* 172c:1d07 */
void part_step_flashlight(struct part *part);             /* 172c:1d78 */
void part_step_generator(struct part *part);             /* 172c:1e5c */
uint16_t part_hit_mort_the_mouse(struct part *part);              /* 172c:34b5 */
void part_step_bob_the_fish(struct part *part);             /* 172c:1c5f */
void part_step_jack_in_the_box(struct part *part);             /* 172c:27e2 */
int16_t  conveyor_speed_for_mass(struct part *obj);     /* 172c:29c6 */
void     conveyor_nudge_3(struct part *obj, int16_t mid);  /* 172c:2a3a */
void     conveyor_nudge_10(struct part *obj, int16_t mid); /* 172c:2a91 */
void     conveyor_nudge_15(struct part *obj, int16_t mid); /* 172c:2acb */
void     conveyor_nudge_25(struct part *obj, int16_t mid); /* 172c:2b1e */
void part_step_light(struct part *part);             /* 172c:2b99 */
void part_step_windmill(struct part *part);             /* 172c:49a1 */
void game_teardown(int16_t really);                 /* 0x0ef9c */
void game_intro(void);                          /* 0x0f340 */
void game_play(void);                               /* 0x0fe40 */
void game_setup(void);                              /* 0x0fe81 */
void game_round(void);                              /* 0x0ff5e */
void round_setup(void);                             /* 0x0ffb2 */
void round_teardown(void);                          /* 0x10021 */
void load_level(uint16_t number);                   /* 0x140de */
void save_level(uint16_t number);                   /* 0x14144 */
void read_level(char *name);                 /* 0x13a82 */
void paint_game_screen(uint16_t present);           /* 0x128b3 */
void draw_machine_thunk(void);                      /* 0x179c8 */
void draw_machine_layer_a(void);                    /* 0x17caf */
void draw_machine_layer_b(void);                    /* 0x179e6 */
void draw_machine_layer_c(void);                    /* 0x17a65 */
void draw_machine_layer_d(void);                    /* 0x17ad9 */
void draw_machine_layer_e(void);                    /* 0x17b43 */
void draw_machine_layer_f(void);                    /* 0x17e5b */
void paint_panel_frame(void);                       /* 0x12a65 */
void draw_description(void);                        /* 0x12b4f */
void adjust_bin_screen(void);                       /* 0x1304b */
void draw_adjust_icons(int16_t top);                /* 0x132d0 */
void draw_adjust_screen(void);                      /* 0x1336e */
void draw_adjust_buttons(void);                     /* 0x133b4 */
void draw_adjust_done(uint16_t frame);              /* 0x133f7 */
void draw_adjust_more(uint16_t frame);              /* 0x1342d */
void draw_adjust_clear(uint16_t frame);             /* 0x13463 */
int16_t check_room_in_bin(void);                    /* 0x13499 */
void draw_title_bar(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint16_t filled);               /* 0x16ca7 */
void draw_sunken_box(int16_t x, int16_t y, int16_t w, int16_t h); /* 0x17270 */
/* **Two declarations, as the original had.** The routine takes a byte, and
   `machine_draw.c`, which defines it, says so; the modules that call it from
   segment 0dff push the colour as a word - `xor ax,ax / push ax` for a 0 - so
   the declaration they were compiled against took an `int`. */
#ifndef TIM_MACHINE_DRAW_C
void fill_panel_area(int16_t x, int16_t y, int16_t w, int16_t h,
                     int16_t colour);              /* 0x173db */
#endif
void draw_wrapped_text(char *str, int16_t x, int16_t y,
                       int16_t w, int16_t h, int16_t shadow);       /* 0x15774 */
void wrap_text_to_box(char *str, int16_t w, int16_t h,
                      int16_t line_height);        /* 0x1587a */
void measure_word(char *str, int16_t *out_width,
                  int16_t *out_length);             /* 0x159c5 */
char *edit_text_key(uint16_t key, char *text, char *caret, int16_t max); /* 0x14d5e */
void text_caret_position(char *text, char *caret, int16_t x, int16_t y,
                         int16_t w, int16_t h, int16_t *out_x, int16_t *out_y); /* 0x15a03 */
char *text_at_point(char *text, int16_t x, int16_t y, int16_t w, int16_t h,
                    int16_t px, int16_t py);                /* 0x15b7d */
uint16_t font_line_height(int16_t slot);            /* 0x2322f */
void paint_panel_frame_rest(void);                  /* 0x129da */
void paint_panel_a(uint16_t frame);                  /* 0x12c58 */
void paint_panel_b(uint16_t frame);                  /* 0x12c8e */
void paint_panel_c(uint16_t frame);                  /* 0x12cc4 */
void paint_panel_d(uint16_t frame);                  /* 0x12cfa */
void paint_panel_free_a(uint16_t frame);           /* 0x12d30 */
void paint_panel_free_b(uint16_t frame);           /* 0x12d8a */
void paint_panel_level(uint16_t frame);            /* 0x12de4 */
void paint_panel_e(void);                           /* 0x12e1a */
void paint_panel_f(void);                           /* 0x12f21 */
void paint_panel_g(void);                           /* 0x12fb6 */
void present_back_page(void);                       /* 0x08ce5 */

void game_screen(void);                             /* 0x11f9e */
void region_cursor_freeform_24(struct region *region); /* 0x127a1 */
void region_cursor_freeform_25(struct region *region); /* 0x127c8 */
void description_key(void);                                /* 0x126ab */
void tab_move_pointer(void);                               /* 0x127e5 */
void show_message_box(const char *title, char *body);
uint16_t ask_yes_no(const char *title, char *body); /* 0x17533 */
uint16_t message_box(const char *title, char *body,
                     const char *button1, const char *button2); /* 0x1754e */
void message_box_tab(const char *button2);             /* 0x17769 */
void draw_button(const char *str, int16_t x, int16_t y,
                 int16_t pressed);                 /* 0x16f96 */
void remove_all_parts(void);                        /* 0x064c5 */
void untie_belt(struct part *part);                     /* 0x05efb */
void detach_rope(struct part *part, uint16_t how);      /* 0x05f69 */
void detach_part_to_bin(struct part *part);                      /* 0x063e1 */
void finish_part_removal(void);                               /* 0x060f2 */
void break_second_attachment(struct part *part);                      /* 0x05e2f */
void aim_link_at_bisector(struct part *part);                      /* 0x059d5 */
uint16_t angle_between_parts(struct part *part, struct part *other);  /* 0x058a6 */
void discard_part(struct part *part);                   /* 0x060c0 */
uint16_t select_puzzle_screen(void);                           /* 0x1002b */
uint16_t dos_chdir(const char *path);                  /* 0x0c397 */
uint16_t diskette_motor_bit(uint16_t bit);            /* 0x0c382 */
uint16_t dos_mkdir(const char *path);                 /* 0x0c3ac */
uint16_t dos_rmdir(const char *path);                 /* 0x0c3c1 */
uint16_t dos_drive_letter(void);                      /* 0x0c3eb */
uint16_t dos_drive_fixed(uint16_t drive);             /* 0x0c41d */
void     dos_disk_reset(void);                        /* 0x0c42d */
uint16_t dos_set_attributes(const char *name, uint16_t attr); /* 0x0c433 */
uint16_t dos_get_attributes(const char *name);        /* 0x0c447 */
void     dos_setdisk(uint8_t letter);               /* 0x0c45b */
void reverse_link_ends(struct rope *rec);               /* 0x04d3e */
struct part *part_under_pointer(struct part *exclude, struct part *part); /* 0x04e50 */
void repaint_whole_screen(void);                    /* 0x08d13 */
int16_t heap_largest_free(void);                    /* 0x08fc6 */
int16_t check_room_for_part(void);                  /* 0x08f4c */
void redraw_machine_area(void);                     /* 0x17908 */
struct part *bin_part_at_index(int16_t index);           /* 0x06527 */
void refile_part_list(struct part *part);               /* 0x06469 */
struct part *bin_scroll_end(void);                      /* 0x0658b */
void bin_scroll_back(uint16_t back_to);             /* 0x11d21 */
void bin_scroll_forward(uint16_t back_to);          /* 0x11d86 */
void select_music_by_key(void);                      /* 0x10ac6 */
void reset_level_state(void);                       /* 0x10ba2 */
void edge_scroll_flags(void);                       /* 0x10cec */
void discard_carried_part(void);                    /* 0x115ed */
void move_carried_part(void);                       /* 0x111b5 */
void part_key_shortcut(void);                 /* 0x11395 */
void pick_up_part(void);                            /* 0x11525 */
void run_drag_frame(void);                          /* 0x116c4 */
int16_t drag_carried_part_first(void);              /* 0x117b3 */
int16_t settle_carried_part_first(void);            /* 0x11925 */
int16_t drag_carried_part_pair(void);               /* 0x11a6a */
void flip_carried_horizontal(void);                      /* 0x11660 */
void flip_carried_vertical(void);                      /* 0x11692 */
int16_t settle_carried_part(void);                  /* 0x11bdc */
void region_click_bin(struct region *region);             /* 0x11e70 */
void region_cursor_bin_above(struct region *region);      /* 0x11de5 */
void region_cursor_bin(struct region *region);            /* 0x11e0e */
void region_cursor_playfield(struct region *region);      /* 0x11f8c */
void region_cursor_freeform(struct region *region);       /* 0x12710 */
void region_cursor_load(struct region *region);           /* 0x1272d */
void region_cursor_save(struct region *region);           /* 0x1274a */
void region_cursor_gravity(struct region *region);        /* 0x12767 */
void region_cursor_air(struct region *region);            /* 0x12784 */
void puzzle_tab(void);                              /* 0x103e2 */
uint16_t puzzle_page_of_score(void);                /* 0x10413 */
void puzzle_repaint(void);                          /* 0x1042d */
void puzzle_draw_password(const char *text);           /* 0x105ac */
void puzzle_draw_list(int16_t first, int16_t selected); /* 0x10638 */
void puzzle_draw_up(void);                          /* 0x104ea */
void puzzle_draw_down(void);                        /* 0x10530 */
void puzzle_draw_ok(uint16_t pressed);              /* 0x10576 */
uint16_t pick_file(uint16_t a, uint16_t b, char *pattern);       /* 0x144ee */
uint16_t get_puzzle_title(int16_t n, char *buf);  /* 0x142f6 */
uint16_t password_to_level(char *text);          /* 0x1439c */
uint16_t is_machine_file(char *name);             /* 0x14223 */
uint16_t validate_filename(void);                    /* 0x14a00 */
void picker_draw_action(void);                       /* 0x14c8a */
void picker_begin(uint16_t a, uint16_t b, char *pattern);       /* 0x14fc3 */
char *listing_to_name(const char far * entry);     /* 0x1572d */
void picker_draw_list(void);                        /* 0x15363 */
void fill_file_listing(char *pattern);                         /* 0x15441 */
void sort_file_listing(void);                               /* 0x15631 */
void picker_repaint(void);                           /* 0x15086 */
void picker_draw_name(void);                         /* 0x15227 */
void picker_draw_filename(void);                     /* 0x152b9 */
void picker_draw_up(void);                           /* 0x1519b */
void picker_draw_down(void);                        /* 0x151e1 */
void picker_tab(void);                              /* 0x14cde */
void picker_type(uint8_t c, char *buf, int16_t max); /* 0x14d0f */
uint16_t path_is_root(const char *path);               /* 0x14ea2 */
void path_up(char *path);                        /* 0x14ed7 */
void path_join(char *path, const char far * entry);     /* 0x14f0d */
void force_extension(char *name, const char *ext);  /* 0x14f67 */
void picker_set_name(const char *name);                /* 0x14f9d */
char *picker_name(void);                         /* 0x14fb0 */
uint16_t save_machine(char *name);                  /* 0x141d9 */
uint16_t write_level(char *name);                   /* 0x13f83 */
void write_byte(FILE *file, const uint8_t * addr);      /* 0x13c21 */
void write_word(FILE *file, const uint8_t * addr);      /* 0x13c4b */
void write_string(FILE *file, char *str);        /* 0x13c78 */
uint16_t game_fwrite(const uint8_t * ptr, uint16_t size, uint16_t count,
                     FILE *file);                /* 0x0a185 */
void write_part_count(FILE *file, struct part *head);       /* 0x13f53 */
void write_record_fields(FILE *file, struct part *part);       /* 0x13c97 */
uint16_t part_index(struct part *part);                 /* 0x1351f */
void write_part_list(FILE *file, struct part *head, uint16_t which); /* 0x13f1a */
uint16_t dos_unlink(const char *path);                 /* 0x0c3d6 */
void game_screen_loop(void);                               /* 0x10856 */
void run_machine_loop(void);                               /* 0x01230 */
void finish_level(void);                             /* 0x032c6 */
void write_config(void);                               /* 0x144b5 */
void count_level_files(void);                       /* 0x14271 */
void wait_cursor(void);                             /* 0x051f3 */
void pause_cursor(void);                            /* 0x0520f */
void restore_cursor(void);                          /* 0x0522b */
int16_t cursor_for_tool(void);                      /* 0x0529f */
void select_cursor(int16_t which);                  /* 0x0523a */
void set_cursor(struct bitmap *bitmap, int16_t hot_x,
                int16_t hot_y);                     /* 0x0b665 */
void redraw_cursor(uint16_t page);                  /* 0x0b91c */
void cursor_redraw_on(void);                           /* 0x0b3d1 */
int16_t button_state(uint16_t index, int16_t down); /* 0x0c18b */
void isr_stack_switch(int16_t to_private);          /* 0x0c46e */
void timer_callback(void);                          /* 0x0b3f1 */
struct vqt_reader *open_bit_reader(uint8_t far *data); /* 0x26588 */
void close_bit_reader(void);                        /* 0x265ba */
void near vqt_screen_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h); /* 0x2762b */
/* **It answers AL alone** - the byte it looked up, with nothing put in AH -
   so under Turbo C++ it returns a `char`. The host calls it through the same
   16-bit pointer as `vqt_read_bits` and gives it that type. */
#ifdef __TURBOC__
typedef uint8_t pixel_byte_t;
#else
typedef uint16_t pixel_byte_t;
#endif
pixel_byte_t near read_palette_pixel(uint16_t bits);                /* 0x265c5 */
void near draw_vqt_flipped(int16_t x, int16_t y, int16_t w, int16_t h);   /* 0x265de */
void near vqt_flip_node(int16_t x, int16_t y, int16_t w, int16_t h);      /* 0x26677 */
void near fill_rows_flip_horizontal(int16_t x0, int16_t y0, int16_t x1, int16_t y1);  /* 0x267ef */
void near fill_rows_flip_vertical(int16_t x0, int16_t y0, int16_t x1, int16_t y1);  /* 0x2683e */
void near fill_rows_flip_both(int16_t x0, int16_t y0, int16_t x1, int16_t y1); /* 0x2688d */
void near vqt_flip_leaf(int16_t x, int16_t y, int16_t w, int16_t h);      /* 0x268df */
uint16_t near vqt_read_bits(uint16_t bits);                     /* 0x275dd */
void near fill_screen_quadrant(uint16_t x, uint16_t y,
                          uint16_t w, uint16_t h);  /* 0x27734 */
uint16_t load_screen_plain(char *name);        /* 0x257b3 */
void draw_cursor(uint16_t page);                    /* 0x0b775 */
void build_screen_regions(void);                    /* 0x09107 */
void mouse_set_speed(uint16_t mickeys);             /* 0x0c49b */
void mono_clear(void);                              /* 0x0c4aa */
void mono_puts(const char *s, int16_t x, int16_t y); /* 0x0c4df */
void mono_printf(int16_t x, int16_t y, const char *fmt, ...); /* 0x0c549 */
uint16_t install_keyboard(int16_t hook_timer);      /* 0x22d1e */
uint16_t mouse_init(void);                          /* 0x23ba7 */
void mouse_set_ranges(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h);      /* 0x23c17 */
struct bitmap **load_bitmap_list(char *name);           /* 0x25306 */
struct bitmap **load_bitmaps(char *name);               /* 0x26bfc */

/* `main`, and the bring-up it calls first. */
void game_main(void);                               /* 0x0eccf */
void game_startup(void);                            /* 0x0eced */

/* Load a palette, a font, and make a font current. Names from the call sites. */
uint8_t far * load_palette(char *name);         /* 0x205f1 */
uint16_t load_font(char *name);                  /* 0x24d07 */
uint16_t set_font(int16_t slot);                    /* 0x23128 */

/* Borland's `printf` and `exit`; the start-up uses them only to give up. */

/* Look a word up through the far pointer at DGROUP 0x546c. */
struct part *part_by_index(int16_t index);           /* 0x13561 */

/* Set the number of scan lines the CRTC displays before blanking. */
void vm_set_display_lines(uint16_t lines);          /* 0x09c2d */

/* Non-zero while the frame flag has not yet been set by the timer handler. */
int16_t timer_may_draw_cursor(void);                /* 0x0c056 */
int16_t frame_pending(void);                        /* 0x0c127 */

/* ------------------------------------------------------- VM.OVL, VGA driver
 * A separate module: the game's video driver, loaded from the resource
 * archive. Addresses are offsets within the loaded driver, not image offsets.
 */

/* Show the page just drawn and swap the buffers. */
void vm_show_page(uint16_t wait_retrace);           /* VM.OVL VGA:0x150f */

/* Set the border colour, through INT 10h. */
void vm_set_border_colour(uint16_t colour);         /* VM.OVL VGA:0x2ae7 */

/* Copy a rectangle between the two pages, in latch mode. */
void vm_copy_rect(uint16_t x, uint16_t y,
                  uint16_t width, uint16_t height);  /* VM.OVL VGA:0x1561 */

/* Fill a run of pixels on one scan line. Register arguments; see the source. */
void vm_span(uint16_t ax, uint16_t bx, int16_t cx,
             uint8_t far * dst);                    /* VM.OVL VGA:0x034f */

/* One row of a scaled bitmap, from the column table. Register arguments. */
void vm_blit_scaled_row(uint16_t plane_size, const int16_t *coltab,
                        uint8_t far * row,
                        int16_t x, int16_t width,
                        const uint8_t far * src);        /* VGA:0x03db */

/* The main blitter: a run of pixels from a byte-per-pixel source. */
void vm_blit_run(uint16_t bx, uint16_t cx, const uint8_t far * src,
                 uint8_t far * dst,
                 int32_t backwards);                 /* VM.OVL VGA:0x0938 */

/* Fill a list of horizontal spans with one colour. */
void vm_fill_spans(const uint8_t far * spans);      /* VM.OVL VGA:0x0be6 */

/* Load a sixteen-colour palette into the DAC and keep a copy. */
void vm_load_palette(const uint8_t far * pal);           /* VM.OVL VGA:0x0f15 */

/* Load colours into the DAC. */
void vm_span_dithered(uint16_t ax, uint16_t bx, int16_t cx,
                      uint8_t far * dst);            /* VGA:0x27a */
void vm_blit_glyph(const uint8_t far * glyph,
                   uint16_t w, uint16_t h, int16_t x, int16_t y); /* VGA:0x124b */
void vm_blend_palette(uint16_t first, uint16_t count, uint16_t colour,
                      uint8_t weight); /* VM.OVL VGA:0x0f57 */
void restore_write_mode(void);           /* 0x205d6 */
void vm_null_hook(void);                 /* 0x205f0 */
void fade_palette_run(uint16_t first, uint16_t count, uint16_t colour,
                      uint16_t weight);  /* 0x208c0 */
int16_t add_palette_cycle(int16_t first, int16_t count, int16_t step);  /* 0x208e6 */
void cycle_palettes(void);                                /* 0x20961 */
void near copy_far_bytes(uint8_t far *src, uint8_t far *dst, int16_t n); /* 0x20a2c */
void fill_span_list(uint8_t far *spans);                  /* 0x20a51 */
void span_list_nothing(uint8_t far *spans);               /* 0x20a64 */
void vm_set_palette(const uint8_t *rgb, uint16_t first,
                    uint16_t count);                 /* VM.OVL VGA:0x0ec1 */

/* ---------------------------------------------------------- segment 1c25
 * Image 0x1c250..0x248f0, the largest of the game's own modules.
 */

/* Fill a rectangle, clipped, via the driver's span filler. */
void fill_rect(int16_t x, int16_t y,
               int16_t w, int16_t h);               /* 0x21d03 */

/* Does a NUL-terminated string contain 'r'? */

/* Copy between two far pointers, normalising both first. */
uint8_t far * huge_move(uint8_t far * dst, const uint8_t far * src, uint32_t count);  /* 0x23e77 */
void far_memcpy(uint8_t far * dst, const uint8_t far * src, uint16_t count);                    /* 0x23f50 */

/* Set the current palette, or answer the one already set. */
uint8_t far * set_palette_pointer(uint8_t far * h);   /* 0x207f4 */

/*
 * **Allocate from DOS by byte count**, answering seg:0000 in DX:AX - or, asked
 * for 0xffffffff bytes, how many are free, in the same DX:AX. A caller takes
 * the answer as the one it wants.
 *
 * The flags are the high word of one `long` - `bitmaps.c` pushes `0L` with one
 * `xor ax,ax` and two `push ax` - and the routine reads only that word,
 * [bp+0xc]; bit 0 of it asks for the block zeroed, `DOS_ZERO_FILL`.
 *
 * `DOS_ALLOC_BYTES` reads the count: the four bytes as they stand under
 * Borland C++, and on the host the pointer-sized value the routine put it in,
 * which a cast straight to `uint32_t` would truncate from the wrong side of a
 * warning. Ours.
 */
uint8_t far *dos_alloc_bytes(uint32_t size, uint32_t flags); /* 0x23747 */
#define DOS_ZERO_FILL 0x10000UL
#ifdef __TURBOC__
#  define DOS_ALLOC_BYTES(r)   ((uint32_t)(r))
#else
#  define DOS_ALLOC_BYTES(r)   ((uint32_t)(uintptr_t)(r))
#endif

/* Fill memory through a far pointer, with a 32-bit count. */
void far_memset(uint8_t far * dst, uint16_t value, uint32_t count);   /* 0x23f8a */

void expand_1bpp_to_4bpp(const uint8_t huge * src, uint8_t huge * dst,
                         uint16_t count);                     /* 0x25714 */
/* Borland's long arithmetic - see borland_huge.c. */

int16_t near lzss_reset(void);                      /* 0x2012a */
/* resource.c: the resource streams, near routines of segment 1c25. */
int16_t near decompress_store(void);                   /* 0x1ee50 */
int16_t near decompress_rle(void);                     /* 0x1ee77 */
int16_t near read_into_huge(uint8_t huge *dst, uint16_t count); /* 0x1eec9 */
int16_t near next_input_byte(void);                    /* 0x1ef3a */
int16_t near read_input_block(uint8_t *dst, uint16_t count); /* 0x1ef97 */
int16_t near emit_literal_run(uint16_t n);             /* 0x1f044 */
int16_t near emit_fill_run(uint16_t value, int16_t n); /* 0x1f0cf */
int16_t near emit_byte(uint16_t value);                /* 0x1f154 */
int16_t near put_output_byte(int16_t c);               /* 0x1f1a2 */
int16_t near select_resource(int16_t handle);          /* 0x1f1f6 */
int16_t near string_contains_r(const char *s);         /* 0x1f290 */
void    near free_if_set(void *p);                     /* 0x1f2b2 */
int16_t near close_resource_slot(int16_t slot);        /* 0x1f2c7 */
int16_t near open_resource_slot(char *mode);           /* 0x1f330 */
int16_t near prepare_resource_slot(int16_t type, char *mode); /* 0x1f37f */
void    near resource_advance(void);                   /* 0x1f44c */
int16_t near resource_read(int16_t handle, uint16_t count); /* 0x1f4cf */
void    near lzw_reset(void);                          /* 0x1f514 */
void    near resource_nothing_1(void);                 /* 0x1f5e1 */
void    near resource_nothing_2(void);                 /* 0x1f5e6 */
/* The handler table's routines in the modules after it. */
int16_t near rle_from_memory(void);                    /* 0x1f8d1 */
/* resfile.c: the resource API and the coders behind it. */
int16_t open_resource(int16_t type, FILE *file, char *mode, int32_t size); /* 0x1f9c4 */
int16_t open_resource_mem(int16_t type, char huge *data, char *mode,
                          int32_t size);               /* 0x1fb08 */
int16_t close_resource(int16_t handle);                /* 0x1fc05 */
int16_t read_resource(int16_t handle, uint8_t far *dst, uint16_t count); /* 0x1fcd4 */
int16_t write_resource(int16_t handle, uint8_t huge *src, uint16_t count); /* 0x1fd10 */
int32_t resource_size(int16_t handle);                 /* 0x1fdd1 */
int32_t resource_seek(int16_t handle, int32_t by, int16_t whence); /* 0x1fdf5 */
int16_t restart_resource_stream(int16_t handle);       /* 0x1ff52 */
void    vm_call_4_thunk(void);                         /* 0x205c6 */
void    vm_call_38_thunk(void);                        /* 0x205d2 */
int16_t near decompress_lzw(void);                     /* 0x1f607 */
int16_t huff_get_bit(void);                            /* 0x20015 */
int16_t decode_char(void);                             /* 0x20043 */
int16_t huff_get_byte(void);                           /* 0x20155 */
void huffman_start(void);                              /* 0x20198 */
void huffman_reconst(void);                            /* 0x20285 */
int16_t decode_position(void);                         /* 0x2040d */
int16_t near decompress_lzss(void);                    /* 0x2045a */
int16_t next_lzw_code(void);                           /* 0x1f80a */
uint16_t font_slot_in_use(int16_t index);             /* 0x2325f */
uint16_t near detect_adapter(void);                    /* 0x2425c */
uint8_t far * load_video_driver(int16_t adapter, char *name); /* 0x24b87 */
uint16_t vm_init(uint16_t adapter, uint16_t unused,
                 FILE *file);                    /* 0x2410d */
/*
 * The polygon filler. These were `static` until the day the rule that a
 * transcribed routine may not be - it keeps them out of libtim.so, so nothing
 * can verify them - and they are declared here now that they are not.
 * Their arguments arrive in **registers**, so the addresses matter to
 * tools/native/dispatch.c rather than to any caller here.
 */
void poly_walk(uint8_t far * span, int16_t x, int16_t frac, int16_t step,
               int16_t acc, int16_t count, uint16_t di);      /* 0x211ec */
void poly_edge_vertical(uint8_t far * span, int16_t x,
                        int16_t y1, int16_t y2);              /* 0x20eef */
void poly_edge_diagonal(uint8_t far * span, int16_t x1, int16_t x2,
                        int16_t y1, int16_t y2);              /* 0x21049 */
void poly_edge_steep(uint8_t far * span, int16_t x1, int16_t x2,
                     int16_t y1, int16_t y2);                 /* 0x20f0b */
void poly_edge_shallow_right(uint8_t far * span, int16_t x1, int16_t x2,
                             int16_t y1, int16_t y2);         /* 0x21070 */
void poly_edge_shallow_left(uint8_t far * span, int16_t x1, int16_t x2,
                            int16_t y1, int16_t y2);          /* 0x2112b */
void poly_outline(int16_t *xs, int16_t *ys,
                  int16_t n);                             /* 0x20ea3 */
void clip_polygon(void);                                      /* 0x22891 */

void free_bitmap_list(struct bitmap ** list);         /* 0x256a2 */
void free_bitmaps(struct bitmap ** list);            /* 0x256c6 */
void near planes_to_chunky(uint8_t far * dst, const uint8_t far * src,
                      uint16_t count);                    /* 0x25faa */
void near emit_packed_value(int16_t value);              /* 0x261a9 */
void near write_literal_run(uint8_t count, uint8_t * buf); /* 0x26243 */
void near compress_row(uint8_t *src, int16_t remaining); /* 0x262c3 */
void near compress_bitmap(struct bitmap *bmp);              /* 0x263e1 */
int32_t compress_bitmap_list(struct bitmap **list,
                             uint8_t colours);     /* 0x26049 */
void free_bitmaps_thunk(struct bitmap ** list);      /* 0x26f5a */
uint16_t count_list_entries(struct bitmap ** list);  /* 0x256f4 */
uint16_t read_bmp_info(FILE *handle, int16_t * count_at,
                       struct bitmap *** out);                        /* 0x2515c */
uint16_t mouse_move_to(uint16_t x, uint16_t y);        /* 0x23d9d */
uint8_t far *huge_add_positive(uint8_t far *p, uint32_t delta); /* 0x23e1a */
int16_t far_ptr_compare(const uint8_t far *a, const uint8_t far *b); /* 0x23fe4 */
void install_divide_trap(void);                        /* 0x2401e */
void divide_error_handler(void);                       /* 0x24048 */
/* The joystick driver, which nothing calls. */
void     joy_time_axes(void);                          /* 0x237ce */
int16_t  joy_scale_axis(void);                         /* 0x23835 */
int16_t  joy_init(void);                               /* 0x2386c */
void     joy_read(int16_t stick, int16_t *x, int16_t *y); /* 0x239a3 */
uint16_t joy_direction(int16_t stick);                 /* 0x23a1b */
uint16_t joy_button(uint16_t n);                       /* 0x23a74 */
int16_t  joy_axis(uint16_t n);                         /* 0x23a8e */
int16_t restore_file_record_from(const struct open_file *src);        /* 0x25b6e */
void near set_mask_of_each(uint16_t value, struct bitmap ** list); /* 0x26f3e */
void draw_bitmap_scaled_248f(struct bitmap *bmp, int16_t x, int16_t y,
                             int16_t a, int16_t b, int16_t c);  /* 0x27007 */
uint16_t count_list(struct bitmap ** list);             /* 0x26f6a */
void near far_copy(uint8_t far *dst, const uint8_t far *src,
              uint16_t count);       /* 0x27a20 */
int16_t  far_stricmp(const char far * a,
                     const char far * b);              /* 0x0abab */
void dos_find_to_dgroup(void);                         /* 0x0c331 */
uint16_t dos_findfirst(const char *pattern, uint16_t attr); /* 0x0c2f9 */
uint16_t dos_findnext(const char *pattern, uint16_t attr);  /* 0x0c315 */
uint16_t dos_find_attr(void);                          /* 0x0c370 */
char *dos_find_name(void);                          /* 0x0c376 */
uint32_t dos_find_size(void);                          /* 0x0c37a */
void dos_get_cur_dir(char *buf);                    /* 0x0c3f5 */
void heap_check_or_hang(void);                         /* 0x0903c */
void checked_free(void *p);                            /* 0x09024 */
void free_region_lists(void);                          /* 0x09b6b */
void free_archive_lists(void);                         /* 0x0a3dc */
int16_t remove_keyboard(void);                         /* 0x22de2 */
int16_t remove_mouse(void);                            /* 0x23d57 */
void restore_int0_vector(void);                        /* 0x24081 */
void near crtc_present(void);                          /* 0x243b1 */
void near set_bios_video_mode(uint16_t bits);          /* 0x243cb */
void shutdown_input(void);                             /* 0x2422f */
void restore_video_mode(void);                         /* 0x24244 */
void free_far_block(uint8_t far * h);        /* 0x20866 */
void close_font_slot(int16_t index);             /* 0x25079 */
void set_holiday_flags(void);                          /* 0x08d41 */
void free_far(void *p);                              /* 0x0c76f */
void game_fread_far(FILE *file, uint8_t * buf);      /* 0x135e5 */
uint16_t read_tim_cfg(void);                           /* 0x14471 */
void save_rect_thunk(uint8_t far * buf, int16_t x,
                     int16_t y, int16_t w, int16_t h); /* 0x2373f */
#ifndef __TURBOC__
/* Under Borland these are the `vm_*` declarations above, renamed. */
void border_colour_thunk(uint16_t colour);             /* 0x2311c */
void show_page_thunk(uint16_t wait_retrace);           /* 0x23124 */
/* The driver answers a `long` in DX:AX; the one caller, `decode_vqt_list`,
   declares the thunk `unsigned` and reads AX alone. */
uint16_t buffer_size_thunk(uint16_t w, uint16_t h);    /* 0x23743 */
void restore_rect_thunk(const uint8_t far * buf, int16_t x,
                        int16_t y, int16_t w, int16_t h); /* 0x24109 */
#endif
uint16_t near bios_video_kind(void);                   /* 0x243ee */
void near set_colour_text_mode(void);                  /* 0x24406 */
int16_t detect_pcjr(void);                             /* 0x2286a */
void timer_tick(void);                              /* 0x223f1 */
int16_t timer_install(uint16_t rate);                  /* 0x2234b */
int16_t timer_remove(void);                            /* 0x223b8 */
uint16_t timer_add_callback(void (far *cb)(void),
                            uint16_t period);          /* 0x222de */
uint16_t timer_drop_callback(uint16_t handle);         /* 0x22328 */
/* The far-callable face of normalise_pointer; answers seg:off in DX:AX. */
uint8_t far *normalise_pointer_far(uint8_t far *p);  /* 0x24010 */

/* Carry paragraphs out of a far pointer's offset into its segment. */
void normalise_pointer(uint8_t far **p);       /* 0x23deb */

/* Store a quarter of each of two words through near pointers. */
void read_mouse_pointer(int16_t *x,
                    int16_t *y);         /* 0x23d73 */

/* Bit 0 of one of two flag bytes at DGROUP 0x48ea. */
/* **`compute_step`'s record** - an accumulator and a step, both 16.16 - as
   the words the callers write and the longs they add. Ours: the original's
   declaration is not known, only that it is read and written both ways. */
union scale_step {
    int32_t l[2];
    int16_t w[4];
};
int16_t compute_step(union scale_step *v, int16_t count);   /* 0x224ca */
void blit_scaled_centred(struct bitmap *bmp, int16_t x, int16_t y,
                         uint16_t mode, int16_t scale);  /* 0x227fe */
int16_t near scale_table_delta(int16_t n);               /* 0x2441a */
int16_t read_mouse_button(uint16_t which);              /* 0x23dc8 */
void mouse_save_vga(void);                          /* 0x23c99 */
void mouse_restore_vga(void);                       /* 0x23cfe */
void mouse_set_user_handler(void (far *h)(void)); /* 0x23c48 */
void mouse_event(uint16_t buttons, uint16_t x, uint16_t y); /* 0x23c59 */

/* Bit 0 of the byte array at DGROUP 0x468c. */
int16_t key_is_down(uint16_t index);               /* 0x23107 */

/* ---------------------------------------------------------- segment 14de */
void clear_layer_heads(void);                   /* 0x184bc */

/* Link a record into up to two buckets headed by that array. */
void link_record_into_buckets(struct part *rec);        /* 0x184d5 */

/* ---------------------------------------------------------- segment 2619 */
const uint8_t far * advance_record(const uint8_t far * rec);  /* 0x2b86d */

/* Follow a chain of far pointers; answers the one it stopped on. */
struct sequence far * follow_far_chain(struct sequence far * seq,
                                int16_t count);           /* 0x2b8b4 */

/* Scale one byte by another and halve the range. */
uint8_t scale_byte_pair(uint8_t cl, uint8_t dl);    /* 0x2ac17 */

/* ---------------------------------------------------------- segment 2a04 */

/* Sine and cosine of a 16-bit angle, 16384 standing for 1. */
int16_t angle_sin(uint16_t angle);                  /* 0x2bfa0 */
int16_t angle_cos(uint16_t angle);                  /* 0x2bfc5 */

/* Signed 16x16 multiply; answers the 32-bit product in DX:AX. */
int32_t long_mul_div(void);                            /* 0x2bb94 */
int32_t scale_record_a(void);                          /* 0x2bd52 */
int32_t scale_record_b(void);                          /* 0x2bd8c */
int32_t  mul16x16(int16_t a, int16_t b);            /* 0x2bdb3 */
int32_t mul_48(void);                                  /* 0x2bdbc */

/* Not transcribed yet; the driver's line drawer. */
void vm_draw_line(int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2);          /* VM.OVL VGA:0x0998 */

/* Clip a line to the clip box and draw what is left. */
uint16_t near draw_char(uint8_t c, int16_t x, int16_t y); /* 0x232fa */
void draw_string_body(const char far *str,
                      int16_t x, int16_t y);        /* 0x23575 */
void draw_string(const char *str, int16_t x, int16_t y); /* 0x2355e */
uint16_t text_width(const char far *str);                /* 0x2329a */
uint16_t font_char_width(int16_t slot);                  /* 0x231ff */
uint16_t glyph_size(int16_t c, uint16_t *w, uint16_t *h); /* 0x236da */
uint16_t text_width_thunk(const char *str);            /* 0x23289 */
void clip_and_draw_line(int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2);    /* 0x23abe */


/* ------------------------------------------------ 1.11's new kinds, stubs
 * Named for the kind and the slot that holds them, until transcribed. */
uint16_t part_init_kind_51(struct part *part);     /* 0x1689e */
uint16_t part_init_kind_52(struct part *part);     /* 0x168db */
uint16_t part_init_kind_53(struct part *part);     /* 0x1690a */
uint16_t part_init_kind_54(struct part *part);     /* 0x16939 */
uint16_t part_init_kind_58(struct part *part);     /* 0x16a1d */
uint16_t part_init_kind_59(struct part *part);     /* 0x16a2e */
uint16_t part_init_kind_61(struct part *part);     /* 0x16a5d */
uint16_t part_init_kind_62(struct part *part);     /* 0x16a96 */
uint16_t part_init_kind_64(struct part *part);     /* 0x16acf */
uint16_t part_init_kind_65(struct part *part);     /* 0x16afe */
uint16_t part_hit_kind_61(struct part *part);      /* 0x1da61 */
void part_setup_kind_61(struct part *part);        /* 0x1dc13 */
void part_step_kind_61(struct part *part);         /* 0x1dc63 */
void part_flip_kind_61(struct part *part);         /* 0x1dd85 */
void kind_61_tip_seesaw(struct part *seesaw, int16_t mouth);       /* 0x1ddb1 */
void kind_61_press_bellow(struct part *bellow, int16_t mouth);     /* 0x1ddec */
void kind_61_pull_plug(struct part *plug, int16_t mouth);          /* 0x1de23 */
void kind_61_close_scissors(struct part *scissors, int16_t mouth); /* 0x1de76 */
uint16_t part_hit_kind_64(struct part *part);      /* 0x1dead */
void part_setup_kind_64(struct part *part);        /* 0x1df70 */
void part_step_kind_64(struct part *part);         /* 0x1dfb1 */
void part_setup_kind_52(struct part *part);        /* 0x1dfe9 */
uint16_t part_hit_kind_54(struct part *part);      /* 0x1e02a */
void part_setup_kind_54(struct part *part);        /* 0x1e0e4 */
void part_step_kind_54(struct part *part);         /* 0x1e133 */
void part_flip_kind_54(struct part *part);         /* 0x1e21b */
void part_step_kind_58(struct part *part);         /* 0x1e247 */
uint16_t part_hit_kind_53(struct part *part);      /* 0x1e2ce */
void part_setup_kind_53(struct part *part);        /* 0x1e2fc */
void part_setup_kind_65(struct part *part);        /* 0x1e33d */
void part_setup_kind_62(struct part *part);        /* 0x1e37e */
void part_flip_kind_62(struct part *part);         /* 0x1e3bf */
void part_step_kind_62(struct part *part);         /* 0x1e3f2 */
int16_t kind_62_push(struct part *what);            /* 0x1e578 */
uint16_t part_hit_kind_51(struct part *part);      /* 0x1e5af */
void part_setup_kind_51(struct part *part);        /* 0x1e5f3 */
void part_step_kind_51(struct part *part);         /* 0x1e643 */
void part_flip_kind_51(struct part *part);         /* 0x1e822 */
int16_t kind_51_pull(struct part *what);            /* 0x1e84e */
void kind_51_swallow(struct part *what);            /* 0x1e8bb */

#endif /* TIM_H */
