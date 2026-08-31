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
/* Chunky 4bpp to planar, through video memory, filling a list of headers. */
void vm_load_bitmap_list(uint16_t list, uint16_t dst_off, uint16_t dst_seg,
                         uint16_t count_lo, uint16_t count_hi); /* VGA:0x1015 */
void vm_chunky_to_planar(uint16_t src_off, uint16_t src_seg, uint16_t dst_off,
                         uint16_t dst_seg, uint16_t count);     /* VGA:0x10b8 */
void vm_read_four_planes(uint16_t src_off, uint16_t src_seg, uint16_t dst_off,
                         uint16_t dst_seg, uint16_t count);     /* VGA:0x11bb */
void vm_build_mask_plane(uint16_t src_off, uint16_t src_seg, uint16_t dst_off,
                         uint16_t dst_seg, uint16_t count);     /* VGA:0x11ee */

void vm_nothing(void);                              /* VGA:0x0252 */
void vm_blit_rows(uint16_t src_off, uint16_t src_seg, int16_t x, int16_t y,
                  int16_t w, int16_t h);            /* VGA:0x15d0 */
void blit_rows_thunk(uint16_t src_off, uint16_t src_seg, int16_t x, int16_t y,
                     int16_t w, int16_t h);         /* 0x20838 */
void blit_rows_alt_thunk(void);                     /* 0x2083c */
void vm_blit_bitmap(uint16_t hdr, int16_t x, int16_t y,
                    uint16_t mode);                     /* VGA:0x1707 */
void vm_blit_scaled(uint16_t hdr, int16_t x, int16_t y); /* VGA:0x271b */
void blit_bitmap_thunk(uint16_t hdr, int16_t x, int16_t y,
                       uint16_t mode);                  /* 0x1e940 */
void blit_scaled_thunk(uint16_t hdr, int16_t x, int16_t y); /* 0x1e944 */
void draw_bitmap(uint16_t hdr, int16_t x, int16_t y, uint16_t mode); /* 0x25300 */
void draw_compressed_bitmap(uint16_t hdr, int16_t x, int16_t y,
                            uint16_t mode);             /* 0x20185 */
void draw_offset_bitmap(uint16_t hdr, int16_t x, int16_t y,
                        uint16_t mode);                 /* 0x24e9a */
uint32_t vm_bitmap_list_size(uint16_t list,
                             uint16_t out);         /* VM.OVL VGA:0x0fd4 */

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

/* The sound module's service routine - what the timer calls. */
void sound_service(void);                           /* 0x27ace */

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
uint16_t install_driver(uint16_t ax, uint16_t es);  /* 0x265f2 */
uint16_t configure_driver(void);                    /* 0x26629 */
void silence_driver(void);                          /* 0x2664e */
void set_master_level(uint8_t cl);                  /* 0x26721 */
void retire_and_tick(uint16_t es, uint16_t ax);                         /* 0x26a57 */

/* The sound module's own routines over that driver, in address order. */
uint32_t voice_playing(uint16_t off, uint16_t seg);    /* 0x287ad */
uint16_t alloc_voice_records(void);                    /* 0x28800 */
void follow_then_tick(uint16_t off, uint16_t seg,
                      int16_t count);                  /* 0x289ba */
uint16_t seek_to_sound_record(int16_t handle,
                              uint16_t want);          /* 0x28bf2 */
uint32_t read_sound_records(int16_t handle);           /* 0x28cf7 */
uint16_t read_record(uint16_t file, uint16_t mode);     /* 0x29da0 */
uint16_t start_sound(int16_t device, int16_t module_index,
                     uint16_t callback, uint16_t handle); /* 0x29c3b */
uint16_t setup_sound_device(int16_t device, int16_t module_index,
                            uint16_t callback, uint16_t handle); /* 0x28655 */
uint16_t load_sound_module(uint16_t handle, uint16_t number,
                           uint16_t index);         /* 0x28580 */
uint32_t load_named_chunk(uint16_t handle, uint16_t path,
                          uint16_t index);          /* 0x28886 */
uint32_t load_sound_bank(uint16_t file, uint16_t size_lo,
                         uint16_t size_hi, uint16_t out); /* 0x289e8 */
uint32_t load_resource_block(uint16_t file, uint16_t size_lo,
                             uint16_t size_hi, uint16_t out,
                             uint16_t kind);           /* 0x28f74 */
uint16_t build_sound_index(int16_t handle, uint16_t list_off,
                           uint16_t list_seg, uint16_t dst_off,
                           uint16_t dst_seg, uint16_t data_at,
                           uint16_t tag);              /* 0x28e87 */
uint32_t insert_by_key(uint16_t head_off, uint16_t head_seg,
                       uint16_t node_off, uint16_t node_seg);  /* 0x28ddb */
void stop_voice_playing(uint16_t off, uint16_t seg);   /* 0x290ab */
uint16_t free_voice_records(void);                     /* 0x29106 */
uint32_t start_on_free_voice(uint16_t off, uint16_t seg, uint16_t index,
                             uint16_t byte_arg);       /* 0x29152 */
void stop_all_voices(void);                            /* 0x2923d */
void set_sound_callback(uint16_t off, uint16_t seg);   /* 0x2928c */
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
uint16_t install_driver_far(uint16_t off, uint16_t seg);    /* 0x28458 */
uint16_t configure_driver_far(uint16_t off, uint16_t seg);  /* 0x2846a */
void retire_and_tick_far(uint16_t off, uint16_t seg);  /* 0x284ef */
void silence_driver_far(uint16_t off, uint16_t seg);   /* 0x28559 */

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
uint16_t clone_part(uint16_t part);                 /* 0x059e4 */
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
int16_t claim_buffer_slot(uint16_t a_lo, uint16_t a_hi,
                          uint16_t b_lo, uint16_t b_hi);  /* 0x0b5ed */

/* Clear one byte of the one-based four-entry array at 0x5734. */
void clear_slot_5734(int16_t n);                    /* 0x0b69c */

/* Make a resource file the open one, closing whatever was open before. */
void make_file_current(uint16_t index);             /* 0x09a62 */

/* Put a file at a position, without asking DOS if it is already there. */
void seek_file_to(uint16_t lo, uint16_t hi);        /* 0x09b38 */

/* The archive entry standing in for an open file, or null for a real one. */
uint16_t archive_entry_for(uint16_t file);          /* 0x09b7c */
int16_t game_fseek(uint16_t file, uint16_t lo, uint16_t hi,
                   int16_t whence);                 /* 0x092dc */
uint32_t fread_huge(uint16_t dst_off, uint16_t dst_seg, uint16_t size_lo,
                    uint16_t size_hi, uint16_t count_lo, uint16_t count_hi,
                    uint16_t file);                 /* 0x0b93d */
int32_t game_ftell(uint16_t file);                  /* 0x093a2 */
int16_t game_fgetc(uint16_t file);                  /* 0x093f6 */
int16_t game_fclose(uint16_t file);                 /* 0x0917f */
void game_rewind(uint16_t file);                    /* 0x093e0 */
int16_t  answer_carry_on(uint16_t what);            /* 0x08fc3 */
uint16_t game_fopen(uint16_t name, uint16_t mode);   /* 0x08fcd */
void load_archive_map(void);                        /* 0x0960f */
int32_t hash_filename(uint16_t name);               /* 0x0980d */
uint16_t game_fread(uint16_t buf, uint16_t size, uint16_t count,
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
void far_move(uint16_t src_off, uint16_t src_seg, uint16_t dst_off,
              uint16_t dst_seg, uint16_t count);    /* 0x0bd2e */
uint32_t long_multiply(uint32_t a, uint32_t b);      /* 0x0c16e */
uint32_t ulong_divide(uint32_t a, uint32_t b);       /* 0x0bd97 */
int32_t long_divide(int32_t a, int32_t b);           /* 0x0bd93 */
void read_far(uint16_t dst_off, uint16_t dst_seg, uint16_t count_lo,
              uint16_t count_hi, uint16_t file);     /* 0x2551a */
void decode_vqt_list(uint16_t file, uint16_t list); /* 0x25639 */
void vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h);   /* 0x25db8 */
void fill_quadrant(uint16_t x, uint16_t y,
                   uint16_t w, uint16_t h);         /* 0x25eb5 */
uint16_t near_memset(uint16_t dst, uint16_t count,
                     uint16_t value);               /* 0x0d543 */
uint16_t heap_calloc(uint16_t count, uint16_t size); /* 0x0c833 */
uint16_t heap_calloc_far(uint16_t count, uint16_t size); /* 0x0bb75 */
uint16_t heap_malloc_far(uint16_t bytes);            /* 0x0bb1e */
uint16_t int_to_string(int16_t value, uint16_t buf,
                       uint16_t radix);             /* 0x0d4bd */
uint16_t long_int_to_string(uint16_t lo, uint16_t hi, uint16_t buf,
                            uint16_t radix);        /* 0x0d4ff */
void draw_odometer_digit(char c, int16_t x, int16_t y); /* 0x15a7e */
void set_clip_counter_strip(void);                  /* 0x026e8 */
void draw_counter_word(int16_t value, int16_t x, int16_t y,
                       int16_t all);                /* 0x0262b */
void draw_counter_long(uint16_t lo, uint16_t hi, int16_t x, int16_t y,
                       int16_t all);                /* 0x02686 */
void redraw_counters(void);                         /* 0x025d8 */
void start_counters(void);                          /* 0x024fa */
void step_counters(void);                           /* 0x02510 */
uint16_t long_to_string(uint16_t letters, uint16_t is_signed, uint16_t radix,
                        uint16_t buf, uint16_t lo,
                        uint16_t hi);               /* 0x0c029 */
uint16_t heap_malloc(uint16_t want);                /* 0x0c999 */

/* Borland's DOS file primitives - NOT part of the reconstruction. */
int16_t dos_read(int16_t handle, uint16_t buf, uint16_t count);   /* 0x0c185 */
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
int16_t dos_getattr(uint16_t name, uint16_t al, uint16_t cx); /* 0x0cd3d */
int16_t dos_open_named(uint16_t name, uint16_t flags); /* 0x0d707 */
int16_t parse_open_mode(uint16_t out_perm, uint16_t out_flags,
                        uint16_t mode);             /* 0x0cf4d */
int16_t stdio_setvbuf(uint16_t file, uint16_t buf, int16_t mode,
                      uint16_t size);               /* 0x0db5e */
uint16_t find_free_stream(void);                    /* 0x0d0a3 */
uint16_t stdio_fopen_into(uint16_t extra_flags, uint16_t mode, uint16_t name,
                          uint16_t file);           /* 0x0d007 */
uint16_t stdio_fopen(uint16_t name, uint16_t mode); /* 0x0d0ce */
uint32_t long_shift_left(uint32_t v, uint8_t count);  /* 0x0be3e */
int16_t io_error(int16_t code);                     /* 0x0bfcd */
uint32_t dos_getvect(uint16_t n);                   /* 0x0bd70 */
void dos_setvect(uint16_t n, uint16_t off, uint16_t seg); /* 0x0bd7f */
uint16_t string_copy(uint16_t dst, uint16_t src);   /* 0x0dd33 */
uint16_t string_copy_far(uint16_t dst, uint16_t src); /* 0x0bb4f */
int16_t string_compare_nocase(uint16_t a, uint16_t b); /* 0x0dd55 */
uint16_t string_copy_padded(uint16_t dst, uint16_t src,
                            uint16_t n);            /* 0x0ddaf */
int16_t open_file(uint16_t name, uint16_t flags,
                  uint16_t perm);                   /* 0x0d5af */
int16_t dos_close(int16_t handle);                  /* 0x0cd80 */
int16_t close_handle(int16_t handle);               /* 0x0cd58 */
int16_t stdio_fclose(uint16_t file);                /* 0x0ce15 */
int16_t unread_count(uint16_t file);                /* 0x0d20f */
int32_t stdio_ftell(uint16_t file);                 /* 0x0d2d4 */
int16_t stdio_fseek(uint16_t file, uint16_t lo, uint16_t hi,
                    int16_t whence);                /* 0x0d26c */
int16_t stdio_getc(uint16_t file);                  /* 0x0d3ef */
uint16_t buffered_read(uint16_t file, uint16_t count,
                       uint16_t buf);               /* 0x0d0ed */
uint16_t stdio_fread(uint16_t buf, uint16_t size, uint16_t count,
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
int16_t string_equal_upto(uint16_t a, uint16_t b,
                          uint16_t n);              /* 0x23e70 */
uint16_t copy_file_record(uint16_t dst, uint16_t handle); /* 0x23ea8 */
uint16_t open_file_record(uint16_t name);           /* 0x23f2c */
uint32_t restore_file_record(uint16_t rec);         /* 0x23f90 */
uint32_t seek_named_chunk(uint16_t handle, uint16_t path,
                          int16_t index);           /* 0x23fc2 */
int16_t open_resource_slot(void);                   /* 0x1c783 */
int16_t prepare_resource_slot(int16_t type,
                              uint16_t name);       /* 0x1c7d5 */

/* Free a pointer unless it is null. */
void free_if_set(uint16_t p);                       /* 0x1c705 */

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
uint16_t load_screen(uint16_t name);                /* 0x253e7 */
uint16_t bios_read_key(void);                       /* 0x21434 */
void copy_rect_thunk(uint16_t x, uint16_t y, uint16_t width,
                     uint16_t height);              /* 0x21088 */
void step_and_draw_machine(int16_t redraw_all);     /* 0x16181 */
void refile_overlapping_parts(void);                /* 0x06b5b */
void draw_machine(int16_t a, int16_t b);            /* 0x1675e */
void draw_rope(uint16_t part, int16_t a);           /* 0x167fa */
void draw_curve(uint8_t colour, int16_t shift,
                int32_t x0, int32_t x1, int32_t x2,
                int32_t y0, int32_t y1, int32_t y2); /* 0x1697d */
void draw_belt_segment(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t slack);              /* 0x16b39 */
void draw_belt(uint16_t part, int16_t a);           /* 0x16baf */
void draw_part(uint16_t part, int16_t level,
               int16_t a, int16_t b);               /* 0x16db1 */
void draw_part_extra(uint16_t part);                /* 0x171b5 */
void draw_polygon(int16_t n, uint16_t xs, uint16_t ys); /* 0x1eded */
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
void sub_15f76(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e); /* 0x15f76 */
void draw_panel(int16_t x, int16_t y, int16_t w, int16_t h); /* 0x151c8 */
void sub_15004(uint16_t a, uint16_t b, uint16_t c, uint16_t d); /* 0x15004 */
void free_all_lists(void);                          /* 0x14d43 */
void free_part_list(uint16_t p);                    /* 0x14d71 */
uint16_t load_animation(uint16_t name);             /* 0x12915 */
uint16_t load_animation_into(uint16_t name);        /* 0x12269 */
void game_fread_byte(uint16_t file, uint16_t buf);  /* 0x11db4 */
void stdio_setbuf_for(uint16_t file, uint16_t buf);  /* 0x095cf */
void game_fread_string(uint16_t file, uint16_t buf);/* 0x11dec */
void alloc_part_table(int16_t n);                   /* 0x11d66 */
void read_list(uint16_t file, uint16_t head, int16_t n);   /* 0x1221b */
void read_record_fields(uint16_t file, uint16_t rec);      /* 0x11e3f */
void build_part_list(void);                         /* 0x1405b */
void free_two_bitmap_lists(void);                   /* 0x0efdc */
void free_all_part_bitmaps(void);                   /* 0x0f86e */
void free_part_bitmap(uint16_t n);                  /* 0x0f886 */
void load_part_bitmap(uint16_t n);                  /* 0x0f7f4 */
uint16_t part_init(uint32_t at, uint16_t part);     /* 0x14236.. */
uint16_t part_init_special(uint32_t at, uint16_t part);
void part_setup(uint16_t off, uint16_t part);       /* segment 172c */
void part_finish(uint16_t off, uint16_t part);
void part_finish_angles(uint16_t part);             /* 0x05d1e */
void part_setup_40f0(uint16_t part);                /* 172c:40f0 */
uint16_t make_part(uint16_t n);                     /* 0x14133 */
void free_part(uint16_t part);                      /* 0x14d95 */
void load_all_parts(void);                          /* 0x0f7b6 */
void draw_frame_corners(uint16_t rec);              /* 0x0ee6e */
void sub_0edf1(uint16_t a, uint16_t b);             /* 0x0edf1 */
uint16_t copy_protect_screen(uint16_t bitmaps);                         /* 0x0ea39 */
void restore_object_backdrop(uint16_t from_page,
                             uint16_t to_page);      /* 0x0adf1 */
void clear_object_covered(uint16_t page);           /* 0x0aedc */
void copy_rect_around_cursor(int16_t x, int16_t y,
                             int16_t w, int16_t h); /* 0x0b28e */
void sub_0aa76(uint16_t a, uint16_t b);             /* 0x0aa76 */
void sub_08f27(uint16_t a);                         /* 0x08f27 */
void regions_handle_pointer(uint16_t list);         /* 0x08546 */

/*
 * OURS: the port cannot call through a far pointer held in guest memory, so a
 * region's two handlers are dispatched on their value. See seg0000.c.
 */
void call_region_handler(uint16_t off, uint16_t seg, uint16_t region);
void stop_music_or_effect(int16_t id);              /* 0x083ea */
void play_sound(int16_t id);                        /* 0x083ab */
void select_music(int16_t id);                      /* 0x08364 */
void restore_cursor_following(void);                /* 0x08125 */
void sub_0810b(void);                               /* 0x0810b */
void reset_machine(void);                           /* 0x07e45 */
void replay_shapes(void);                           /* 0x06699 */
void clear_machine(void);                           /* 0x013e9 */
void restart_machine(void);                         /* 0x01431 */
void unlink_node(uint16_t node);                    /* 0x05628 */
void step_machine(void);                            /* 0x00f86 */
void step_moving_object(uint16_t obj);              /* 0x01216 */
void collect_carried(uint16_t obj);                 /* 0x03972 */
void carry_riders_along(uint16_t obj);              /* 0x03a8d */
void bounce_off_contact(uint16_t obj);              /* 0x03046 */
void bounce_pair(uint16_t obj);                       /* 0x03201 */
void part_moved(uint16_t part);                     /* 0x06d8e */
void belt_in_dirty_rect(uint16_t part);             /* 0x06994 */
void mark_parts_in_dirty_rects(void);               /* 0x06806 */
void add_carried_weight(uint16_t obj);              /* 0x07c3a */
void add_mass_capped(uint16_t obj, uint16_t other); /* 0x07c5b */
void part_step(uint16_t part);                      /* dispatch, ours */
uint16_t part_hit(uint16_t kind, uint16_t part);    /* dispatch, ours */
uint16_t part_hook_172c(uint16_t off, uint16_t part); /* segment 172c */
uint16_t part_drive_172c(uint16_t off, uint16_t p1, uint16_t p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6, uint16_t p7);
uint16_t part_drive_0802(uint16_t from, uint16_t part, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t lo,
                         uint16_t hi);                   /* 172c:0802 */
uint16_t part_drive_11d2(uint16_t from, uint16_t part, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t lo,
                         uint16_t hi);                   /* 172c:11d2 */
uint16_t part_drive_2451(uint16_t p1, uint16_t si, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t p6,
                         uint16_t p7);                   /* 172c:2451 */
uint16_t part_drive_2c19(uint16_t p1, uint16_t si, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t p6,
                         uint16_t p7);              /* 172c:2c19 */
uint16_t part_drive(uint16_t by, uint16_t p1, uint16_t p2, uint16_t p3,
                    uint16_t p4, uint16_t p5, uint16_t p6, uint16_t p7);
uint16_t drive_belts(uint16_t from, uint16_t part, uint16_t flags,
                     uint16_t a, uint16_t b, uint16_t c); /* 172c:461a */
uint16_t part_hit_3ebf(uint16_t part);              /* 172c:3ebf */
uint16_t part_step_3fae(uint16_t part);             /* 172c:3fae */
uint16_t part_hit_3fe8(uint16_t part);              /* 172c:3fe8 */
uint16_t part_step_420f(uint16_t part);             /* 172c:420f */
uint16_t rope_other_end(uint16_t part);             /* 0x06dbf */
void link_objects_in_range(uint16_t obj, uint16_t flags,
                           int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1);  /* 0x036de */
void link_objects_crossing(uint16_t obj, uint16_t flags,
                           uint16_t line);           /* 0x03782 */
void link_objects_at_point(uint16_t obj, int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1);  /* 0x038b9 */
void     seg172c_nothing(void);                     /* 172c:0000 */
void     sound_on_hard_impact(uint16_t obj);        /* 0x03009 */
void     mark_needs_refile(uint16_t part, uint8_t n); /* 0x058f3 */
void     mark_belt_shapes(uint16_t part, uint16_t mode); /* 0x05f87 */
void     mark_joined_shapes(uint16_t part, uint16_t mode); /* 0x05e70 */
void     mark_part_shapes(uint16_t part, uint16_t mode); /* 0x0647f */
int16_t  outlines_cross(uint16_t a, uint16_t b);    /* 0x03f4d */
int16_t  object_overlaps_any(uint16_t obj);         /* 0x03e23 */
int16_t  queue_part(uint16_t src, uint16_t part);   /* 0x07b6f */
int16_t  tension_belt(uint16_t part);               /* 0x072c7 */
int16_t  belt_orientation(uint16_t belt, int16_t which,
                          int16_t dir);             /* 0x06de9 */
uint16_t part_hit_016e(uint16_t part);              /* 172c:016e */
uint16_t part_hit_1de0(uint16_t part);              /* 172c:1de0 */
uint16_t part_hit_2b7e(uint16_t part);              /* 172c:2b7e */
uint16_t part_hit_2f25(uint16_t part);              /* 172c:2f25 */
uint16_t part_step_018e(uint16_t part);             /* 172c:018e */
uint16_t part_hit_0552(uint16_t part);              /* 172c:0552 */
uint16_t part_step_057e(uint16_t part);             /* 172c:057e */
uint16_t part_step_08f1(uint16_t part);             /* 172c:08f1 */
uint16_t part_step_098a(uint16_t part);             /* 172c:098a */
uint16_t part_step_0a5d(uint16_t part);             /* 172c:0a5d */
uint16_t part_step_0ca3(uint16_t part);             /* 172c:0ca3 */
uint16_t part_step_11a6(uint16_t part);             /* 172c:11a6 */
int16_t  bounce_speed_for_mass(uint16_t obj);       /* 172c:06f9 */
void     break_kind_15(uint16_t part);              /* 172c:1c9e */
void     trigger_kind_6(uint16_t part);             /* 172c:2ffd */
int16_t  push_speed_for_mass(uint16_t obj);         /* 172c:271f */
void     trigger_things_at(uint16_t part, int16_t mode,
                           int16_t dx);             /* 172c:277d */
uint16_t part_hit_0c6c(uint16_t part);              /* 172c:0c6c */
uint16_t part_step_12c2(uint16_t part);             /* 172c:12c2 */
void     burst_kind_19(uint16_t part);              /* 172c:1328 */
uint16_t part_step_13c9(uint16_t part);             /* 172c:13c9 */
uint16_t part_hit_14d3(uint16_t part);              /* 172c:14d3 */
uint16_t part_step_15ce(uint16_t part);             /* 172c:15ce */
uint16_t part_step_20fc(uint16_t part);             /* 172c:20fc */
uint16_t part_step_22ae(uint16_t part);             /* 172c:22ae */
uint16_t part_hit_2514(uint16_t part);              /* 172c:2514 */
uint16_t part_step_2592(uint16_t part);             /* 172c:2592 */
uint16_t part_step_2f3e(uint16_t part);             /* 172c:2f3e */
void     part_setup_3030(uint16_t part);            /* 172c:3030 */
uint16_t part_step_3035(uint16_t part);             /* 172c:3035 */
uint16_t part_step_34d0(uint16_t part);             /* 172c:34d0 */
uint16_t part_hit_3824(uint16_t part);              /* 172c:3824 */
uint16_t part_step_3635(uint16_t part);             /* 172c:3635 */
uint16_t part_step_38fc(uint16_t part);             /* 172c:38fc */
void     cut_belts(uint16_t part, uint16_t line);   /* 172c:3970 */
void grab_distance(uint16_t a, uint16_t b,
                   uint16_t out_x, uint16_t out_y); /* 172c:31dc */
uint16_t spread_gear_signal(uint16_t from, uint16_t to, int16_t how,
                            uint16_t flag);         /* 172c:105d */
void settle_gear_signal(uint16_t part, int16_t clear); /* 172c:1225 */
uint16_t part_step_1649(uint16_t part);             /* 172c:1649 */
int16_t  blast_speed_for_mass(uint16_t part);       /* 172c:1748 */
void     split_part_at(uint16_t part, uint16_t blast); /* 172c:17bc */
int16_t  angle_between_centres(uint16_t a, uint16_t b); /* 0x03da5 */
uint16_t part_step_1a82(uint16_t part);             /* 172c:1a82 */
uint16_t part_hit_1c39(uint16_t part);              /* 172c:1c39 */
uint16_t part_hit_1d07(uint16_t part);              /* 172c:1d07 */
uint16_t part_step_1d78(uint16_t part);             /* 172c:1d78 */
uint16_t part_step_1e5c(uint16_t part);             /* 172c:1e5c */
uint16_t part_hit_34b5(uint16_t part);              /* 172c:34b5 */
uint16_t part_step_1c5f(uint16_t part);             /* 172c:1c5f */
uint16_t part_step_27e2(uint16_t part);             /* 172c:27e2 */
int16_t  conveyor_speed_for_mass(uint16_t obj);     /* 172c:29c6 */
void     conveyor_nudge_3(uint16_t obj, int16_t mid);  /* 172c:2a3a */
void     conveyor_nudge_10(uint16_t obj, int16_t mid); /* 172c:2a91 */
void     conveyor_nudge_15(uint16_t obj, int16_t mid); /* 172c:2acb */
void     conveyor_nudge_25(uint16_t obj, int16_t mid); /* 172c:2b1e */
uint16_t part_step_2b99(uint16_t part);             /* 172c:2b99 */
uint16_t part_step_49a1(uint16_t part);             /* 172c:49a1 */
uint16_t sub_0e34a(uint16_t arg);                   /* 0x0e34a */
uint16_t game_intro(void);                          /* 0x0e4be */
void sub_0eed5(void);                               /* 0x0eed5 */
void count_level_files(void);                       /* 0x129a8 */
void wait_cursor(void);                             /* 0x04652 */
void restore_cursor(void);                          /* 0x0466e */
void select_cursor(int16_t which);                  /* 0x0467d */
void set_cursor(uint16_t bitmap, int16_t hot_y,
                int16_t hot_x);                     /* 0x0aa14 */
void redraw_cursor(uint16_t page);                  /* 0x0acc3 */
void set_flag_2d44(void);                           /* 0x0a78e */
int16_t button_state(uint16_t index, int16_t down); /* 0x0b542 */
void isr_stack_switch(int16_t to_private);          /* 0x0b82c */
void timer_callback(void);                          /* 0x0a7ae */
uint16_t open_bit_reader(uint16_t off, uint16_t seg); /* 0x248fe */
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
uint16_t load_bitmaps(uint16_t name);               /* 0x24f72 */

/* `main`, and the bring-up it calls first. */
uint16_t game_main(void);                           /* 0x0dfff */
void game_startup(void);                            /* 0x0e01d */

/* Load a palette, a font, and make a font current. Names from the call sites. */
uint32_t load_palette(uint16_t name);               /* 0x1e967 */
uint16_t load_font(uint16_t name);                  /* 0x2307d */
uint16_t set_font(int16_t slot);                    /* 0x2149e */

/* Borland's `printf` and `exit`; the start-up uses them only to give up. */
int16_t stdio_printf(uint16_t fmt);                 /* 0x0d754 */
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
uint32_t huge_move(uint16_t dst_off, uint16_t dst_seg,
                   uint16_t src_off, uint16_t src_seg,
                   uint16_t count_lo, uint16_t count_hi);  /* 0x221ed */
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
/* Borland's huge-pointer arithmetic - see borland_huge.c. */
int16_t huge_equal(uint16_t off_a, uint16_t seg_a,
                   uint16_t off_b, uint16_t seg_b);    /* 0x0bd0d */
uint32_t huge_sub_from(uint16_t var_off, uint16_t var_seg,
                       int32_t delta);              /* 0x0bec6 */
void expand_1bpp_to_4bpp(uint16_t src_off, uint16_t src_seg, uint16_t dst_off,
                         uint16_t dst_seg, uint16_t count);   /* 0x23a8a */
int32_t long_shift_right(int32_t v, uint8_t count);  /* 0x0be62 */
uint32_t long_multiply_2(uint32_t a, uint32_t b);    /* 0x0bcf6 */
uint32_t huge_add_to(uint16_t var_off, uint16_t var_seg,
                     int32_t delta);                   /* 0x0be82 */
uint32_t huge_add(uint16_t off, uint16_t seg, int32_t delta);  /* 0x0bf0a */
uint32_t huge_post_add(uint16_t var_off, uint16_t var_seg,
                       uint16_t inc);                  /* 0x0bf6a */

int16_t decompress_rle(void);                          /* 0x1c278 */
int16_t resource_read(uint16_t handle, uint16_t count); /* 0x1c92b */
void lzw_reset(void);                               /* 0x1c970 */
int16_t restart_resource_stream(int16_t handle);     /* 0x1dae6 */
int16_t lzss_reset(void);                           /* 0x1dc15 */
int16_t open_resource(uint16_t unused, uint16_t file, uint16_t name,
                      uint16_t size_lo, uint16_t size_hi);  /* 0x1d54e */
int16_t close_resource(int16_t handle);             /* 0x1d798 */
uint32_t resource_size(int16_t handle);             /* 0x1d95f */
uint32_t resource_seek(int16_t handle, uint16_t lo, uint16_t hi,
                       int16_t whence);                /* 0x1d983 */
int16_t read_resource(int16_t handle, uint16_t dst_off,
                      uint16_t dst_seg, uint16_t count); /* 0x1d868 */
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
int16_t read_into_huge(uint16_t dst_off, uint16_t dst_seg,
                       uint16_t count);                /* 0x1c319 */
int16_t next_input_byte(void);                         /* 0x1c389 */
uint16_t table_618a_in_use(int16_t index);             /* 0x215d5 */
uint16_t detect_adapter(void);                         /* 0x225d2 */
uint32_t load_video_driver(int16_t adapter, uint16_t file); /* 0x22efd */
uint16_t vm_init(uint16_t adapter, uint16_t unused,
                 uint16_t file);                    /* 0x22483 */
void free_bitmap_list(uint16_t list);                /* 0x23a18 */
void free_bitmaps(uint16_t list);                   /* 0x23a3c */
void planes_to_chunky(uint16_t dst_off, uint16_t dst_seg, uint16_t src_off,
                      uint16_t src_seg, uint16_t count);  /* 0x24320 */
void emit_packed_value(int16_t value);              /* 0x2451f */
void write_literal_run(uint8_t count, uint16_t buf); /* 0x245b9 */
void compress_row(uint16_t src, int16_t remaining); /* 0x24639 */
void compress_bitmap(uint16_t header);              /* 0x24757 */
int32_t compress_bitmap_list(uint16_t list,
                             uint16_t colours);     /* 0x243bf */
void free_bitmaps_thunk(uint16_t list);             /* 0x252d0 */
uint16_t count_list_entries(uint16_t list);         /* 0x23a6a */
uint16_t read_bmp_info(uint16_t handle, uint16_t count_at,
                       uint16_t out);                  /* 0x234d2 */
uint16_t mouse_move_to(uint16_t x, uint16_t y);        /* 0x22113 */
uint32_t huge_add_positive(uint16_t off, uint16_t seg, uint16_t lo,
                           uint16_t hi);               /* 0x22190 */
void install_divide_trap(void);                        /* 0x22394 */
int16_t restore_file_record_from(uint16_t src);        /* 0x23ee4 */
void set_field_4_of_each(uint16_t value, uint16_t list); /* 0x252b4 */
uint16_t count_list(uint16_t list);                    /* 0x252e0 */
void far_copy(uint16_t dst_off, uint16_t dst_seg, uint16_t src_off,
              uint16_t src_seg, uint16_t count);       /* 0x25d96 */
void dos_getdate(uint16_t out);                        /* 0x0bd4a */
void dos_get_cur_dir(uint16_t buf);                    /* 0x0b7b3 */
uint16_t string_concat(uint16_t dst, uint16_t src);    /* 0x0dc95 */
int16_t stdio_setbuf(uint16_t file, uint16_t buf);     /* 0x0c1b2 */
int16_t heap_check(void);                              /* 0x0cb45 */
void heap_check_or_hang(void);                         /* 0x08528 */
void checked_free(uint16_t p);                         /* 0x08510 */
void setup_streams(void);                              /* 0x0c1d6 */
void set_holiday_flags(void);                          /* 0x08259 */
void heap_free_far(uint16_t p);                        /* 0x0bb2d */
void game_fread_far(uint16_t file, uint16_t buf);      /* 0x11dd1 */
uint16_t read_tim_cfg(void);                           /* 0x12ba7 */
void show_page_thunk(uint16_t wait_retrace);           /* 0x2149a */
void save_rect_thunk(uint16_t buf_off, uint16_t buf_seg, int16_t x,
                     int16_t y, int16_t w, int16_t h); /* 0x21ab5 */
uint32_t buffer_size_thunk(uint16_t w, uint16_t h);    /* 0x21ab9 */
void restore_rect_thunk(uint16_t buf_off, uint16_t buf_seg, int16_t x,
                        int16_t y, int16_t w, int16_t h); /* 0x2247f */
uint16_t bios_video_kind(void);                        /* 0x22764 */
int16_t detect_pcjr(void);                             /* 0x20be0 */
void timer_tick(void);                              /* 0x20767 */
int16_t timer_install(uint16_t rate);                  /* 0x206c1 */
int16_t timer_remove(void);                            /* 0x2072e */
uint16_t timer_add_callback(uint16_t off, uint16_t seg,
                            uint16_t period);          /* 0x20654 */
uint16_t timer_drop_callback(uint16_t handle);         /* 0x2069e */
uint32_t normalise_far_ptr_far(uint16_t off, uint16_t seg);  /* 0x22386 */

/* Carry paragraphs out of a far pointer's offset into its segment. */
void normalise_far_ptr(uint16_t *off, uint16_t *seg);       /* 0x22161 */

/* Store a quarter of each of two words through near pointers. */
void read_pair_4740(uint16_t out_a, uint16_t out_b); /* 0x220e9 */

/* Bit 0 of one of two flag bytes at DGROUP 0x48ea. */
int16_t flag_bit_48ea(uint16_t which);              /* 0x2213e */
void mouse_save_vga(void);                          /* 0x2200f */
void mouse_restore_vga(void);                       /* 0x22074 */
void mouse_set_user_handler(uint16_t off, uint16_t seg); /* 0x21fbe */
void mouse_event(uint16_t buttons, uint16_t x, uint16_t y); /* 0x21fcf */

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
uint16_t text_width(uint16_t str);                  /* 0x21610 */
uint16_t text_width_thunk(uint16_t str);            /* 0x215ff */
void clip_and_draw_line(int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2);    /* 0x21e34 */

#endif /* TIM_H */
