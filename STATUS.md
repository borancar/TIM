# Status

*Last updated 2026-08-28.*

Reconstruction of **The Incredible Machine** (Dynamix / Sierra, 1993) from
`incredible-machine/TIM.EXE`.

**Which world this is in:** the original is **compiled C** (Borland C++ 1991,
large model), not hand-written assembly. A byte-exact *matching* decompilation
is therefore possible in principle. It is **not** what is being attempted yet;
the present standard is behavioural equivalence proved by differential
verification, and any move to matching would be a deliberate change of goal.

**Which variant:** `TIM.EXE` is the only executable, but its video driver
`VM.OVL` is a container of **eight per-adapter drivers** - `VGA`, `EGA`, `MCG`,
`CGA`, `TAN`, `HEG`, `EVG`, `EVA`. Only the **VGA** driver is being
reconstructed. The other seven are deliberate non-goals, listed here rather
than left looking unfinished.

## Done - and what was actually checked

- **The executable is recovered and the recovery is proven.** Not "it unpacked
  and looked plausible": `tools/verify_unpack.py` loads the emitted EXE and
  compares it against what the LZEXE stub itself produced in memory - all
  **214,512 bytes identical**, and CS, IP, SS and SP equal. The relocation
  table was *measured*, by running the stub at two load segments and diffing,
  and the run reports zero bytes that differ for any other reason.
- **The resource archive format is verified against the bytes**, not inherited.
  All four data files walk to exactly their own size, 159 subfiles, and the
  offsets found by walking are the offsets `RESOURCE.MAP` lists. See
  `docs/resources.md`, which marks separately the one thing taken on trust and
  not checked: the name-hash function, which is not needed to read the archive.
- **The game runs under the emulator** through the Sierra logo, the title
  screen and the credits screen, in 640x400 16-colour planar, page-flipped.
- **Captures are reproducible.** They are taken on the guest's own cue - the
  page flip that makes a composed frame visible - and the machine runs on a
  virtual clock driven by emulated instructions, so a given capture is the same
  capture on another machine and after a rebuild.

## Open

### Coverage - as last measured, 2026-08-28

| | |
| --- | --- |
| call targets found by recursive descent | **577** |
| reached by the title screen, flips 6..40 | **218** |

The transcribed and verified counts are **not** written here by hand - they
were wrong within one session when they were. They come from the sweep below,
which `tools/verify.py --all` regenerates in place. It stops the emulator at
each routine's entry, lets the **original body** run to its return, and
compares what each did to the hardware:

<!-- VERIFY:BEGIN -->
| routine | address | occurrences checked | result |
| --- | --- | --- | --- |
| `vm_set_display_lines` | 0x08f77 | 0, 1 | agreed |
| `vm_save_rect` | VM.OVL VGA:0x12fb | 0 | agreed |
| `vm_restore_rect` | VM.OVL VGA:0x13b9 | 0 | agreed |
| `atan2_long` | 0x2d296 | 0 | agreed |
| `link_nearby_objects` | 0x03566 | 0 | agreed |
| `find_edge_contact_reversed` | 0x00b6c | 0 | agreed |
| `resolve_collisions` | 0x00556 | 0 | agreed |
| `find_edge_contact` | 0x007af | 0 | agreed |
| `integrate_object` | 0x02c93 | 0 | agreed |
| `place_object_for_draw` | 0x05be4 | 0 | agreed |
| `add_sub_object_shapes` | 0x05ef6 | - | **transcribed, never called** on these screens |
| `set_object_extent` | 0x05c77 | 0 | agreed |
| `object_delta_angle` | 0x004ab | 0 | agreed |
| `arctan_lookup` | 0x2a941 | 0 | agreed |
| `apply_contact_friction` | 0x02da0 | 0 | agreed |
| `vm_read_pixel` | VM.OVL VGA:0x1453 | - | **transcribed, never called** on these screens |
| `read_pixel_clipped` | 0x2241b | - | **transcribed, never called** on these screens |
| `vm_plot_pixel` | VM.OVL VGA:0x14c9 | - | **transcribed, never called** on these screens |
| `plot_pixel_clipped` | 0x2244d | - | **transcribed, never called** on these screens |
| `init_sequence_params` | 0x28305 | 0, 1 | agreed |
| `next_matching_record` | 0x29966 | 0, 1, 4 | agreed |
| `midi_bend_event` | 0x280fe | 0, 1 | agreed |
| `step_sequence` | 0x27c4e | 0, 1 | agreed |
| `midi_note_off_event` | 0x27e92 | - | **transcribed, never called** on these screens |
| `midi_event_6` | 0x27f54 | - | **transcribed, never called** on these screens |
| `midi_meta_event` | 0x2817e | 0, 1 | agreed |
| `midi_skip_event` | 0x2817a | - | **transcribed, never called** on these screens |
| `skip_unknown_event` | 0x2828e | - | **transcribed, never called** on these screens |
| `midi_controller_event` | 0x27f85 | 0, 1 | agreed |
| `midi_program_event` | 0x28086 | 0 | agreed |
| `midi_event_9` | 0x280da | - | **transcribed, never called** on these screens |
| `midi_note_event` | 0x27ee1 | 0, 1 | agreed |
| `free_node_list` | 0x28baf | 0, 1 | agreed |
| `create_sequence` | 0x28935 | 0 | agreed |
| `free_for_kind` | 0x2a017 | 0, 1 | agreed |
| `alloc_for_kind` | 0x29f89 | 0, 1 | agreed |
| `start_sequence_far` | 0x28480 | 0 | agreed |
| `load_and_start_sequence` | 0x29034 | 0 | agreed |
| `start_sequence` | 0x26783 | 0 | agreed |
| `advance_volume_ramp` | 0x278e9 | - | **transcribed, never called** on these screens |
| `set_sequence_volume` | 0x279a9 | - | **transcribed, never called** on these screens |
| `sound_service` | 0x27ace | 0, 1 | agreed |
| `drop_unless_polled` | 0x27b52 | - | **transcribed, never called** on these screens |
| `poll_sequences` | 0x27b7e | 0, 1 | agreed |
| `remove_sequence` | 0x26e7b | 0, 1 | agreed |
| `sound_callback` | 0x292a1 | - | **transcribed, never called** on these screens |
| `sequencer_tick` | 0x26f2a | 0, 1 | agreed |
| `install_driver` | 0x265f2 | 0 | agreed |
| `configure_driver` | 0x26629 | 0 | agreed |
| `silence_driver` | 0x2664e | - | **transcribed, never called** on these screens |
| `set_master_level` | 0x26721 | 0 | agreed |
| `retire_and_tick` | 0x26a57 | 0, 1 | agreed |
| `set_master_level_far` | 0x28431 | 0 | agreed |
| `install_driver_far` | 0x28458 | 0 | agreed |
| `configure_driver_far` | 0x2846a | 0 | agreed |
| `retire_and_tick_far` | 0x284ef | 0, 1 | agreed |
| `silence_driver_far` | 0x28559 | - | **transcribed, never called** on these screens |
| `voice_playing` | 0x287ad | 0, 1, 4 | agreed |
| `follow_then_tick` | 0x289ba | 0 | agreed |
| `seek_to_sound_record` | 0x28bf2 | 0, 1, 4 | agreed |
| `read_sound_records` | 0x28cf7 | 0, 1 | agreed |
| `open_sound_file` | 0x296b4 | 0, 1 | agreed |
| `read_record` | 0x29da0 | 0, 1, 4 | agreed |
| `load_sound_bank` | 0x289e8 | 0, 1 | agreed |
| `load_resource_block` | 0x28f74 | 0 | agreed |
| `build_sound_index` | 0x28e87 | 0, 1 | agreed |
| `insert_by_key` | 0x28ddb | - | **transcribed, never called** on these screens |
| `stop_voice_playing` | 0x290ab | 0, 1 | agreed |
| `free_voice_records` | 0x29106 | - | **transcribed, never called** on these screens |
| `start_on_free_voice` | 0x29152 | 0, 1 | agreed |
| `stop_all_voices` | 0x2923d | 0 | agreed |
| `set_sound_callback` | 0x2928c | - | **transcribed, never called** on these screens |
| `set_master_level_ok` | 0x296a1 | 0 | agreed |
| `alloc_voice_records` | 0x28800 | 0 | agreed |
| `stop_sequences` | 0x294ff | 0, 1 | agreed |
| `delay_five_ticks` | 0x2937f | - | **transcribed, never called** on these screens |
| `tick_delay` | 0x293b8 | - | **transcribed, never called** on these screens |
| `remove_and_free_records` | 0x293c1 | 0, 1 | agreed |
| `start_sequence_by_id` | 0x29a49 | 0, 1, 4 | agreed |
| `timer_add_callback` | 0x20654 | 0, 1 | agreed |
| `timer_drop_callback` | 0x2069e | - | **transcribed, never called** on these screens |
| `huge_equal` | 0x0bd0d | 0, 1, 4 | agreed |
| `near_memset` | 0x0d543 | 0, 1, 4 | agreed |
| `heap_calloc` | 0x0c833 | 0, 1, 4 | agreed |
| `heap_calloc_far` | 0x0bb75 | 0, 1, 4 | agreed |
| `huge_add_to` | 0x0be82 | 0, 1, 4 | agreed |
| `huge_add` | 0x0bf0a | 0, 1, 4 | agreed |
| `huge_post_add` | 0x0bf6a | - | **transcribed, never called** on these screens |
| `decompress_lzss` | 0x1e7f2 | 0, 1, 4 | agreed |
| `huff_get_bit` | 0x1dfd6 | 0, 1, 4 | agreed |
| `huff_get_byte` | 0x1e00b | 0, 1, 4 | agreed |
| `huffman_reconst` | 0x1e1af | - | **transcribed, never called** on these screens |
| `huffman_update` | 0x1e338 | 0, 1, 4 | agreed |
| `huffman_start` | 0x1e0b3 | 0, 1 | agreed |
| `decompress_lzw` | 0x1ca62 | 0, 1, 4 | agreed |
| `read_input_block` | 0x1c3e6 | 0, 1, 4 | agreed |
| `next_lzw_code` | 0x1cc65 | 0, 1, 4 | agreed |
| `resource_seek` | 0x1d983 | 0, 1, 4 | agreed |
| `lzw_reset` | 0x1c970 | 0, 1 | agreed |
| `lzss_reset` | 0x1dc15 | 0, 1, 4 | agreed |
| `open_resource` | 0x1d54e | 0, 1, 4 | agreed |
| `close_resource` | 0x1d798 | 0, 1, 4 | agreed |
| `resource_size` | 0x1d95f | 0, 1, 4 | agreed |
| `read_resource` | 0x1d868 | 0, 1, 4 | agreed |
| `resource_read` | 0x1c92b | 0, 1, 4 | agreed |
| `decompress_rle` | 0x1c278 | 0 | agreed |
| `emit_literal_run` | 0x1c493 | 0, 1, 4 | agreed |
| `emit_fill_run` | 0x1c51e | 0, 1, 4 | agreed |
| `emit_byte` | 0x1c5a3 | 0, 1, 4 | agreed |
| `open_file_record` | 0x23f2c | 0, 1, 4 | agreed |
| `make_file_current` | 0x09a62 | 0, 1, 4 | agreed |
| `find_file_record` | 0x23df2 | 0, 1, 4 | agreed |
| `file_record_size` | 0x242af | 0, 1, 4 | agreed |
| `file_record_valid` | 0x24308 | 0, 1, 4 | agreed |
| `close_file_record` | 0x242d9 | 0, 1, 4 | agreed |
| `close_resource_slot` | 0x1c71a | 0, 1 | agreed |
| `open_resource_slot` | 0x1c783 | 0, 1, 4 | agreed |
| `prepare_resource_slot` | 0x1c7d5 | 0, 1, 4 | agreed |
| `free_if_set` | 0x1c705 | 0, 1 | agreed |
| `read_into_huge` | 0x1c319 | 0, 1, 4 | agreed |
| `next_input_byte` | 0x1c389 | 0, 1, 4 | agreed |
| `stdio_setvbuf` | 0x0db5e | 0, 1, 4 | agreed |
| `stdio_fopen_into` | 0x0d007 | 0, 1, 4 | agreed |
| `io_error` | 0x0bfcd | 0, 1, 4 | agreed |
| `dos_getvect` | 0x0bd70 | 0 | agreed |
| `dos_setvect` | 0x0bd7f | 0 | agreed |
| `long_shift_left` | 0x0be3e | 0, 1, 4 | agreed |
| `string_compare_nocase` | 0x0dd55 | 0, 1, 4 | agreed |
| `string_copy_padded` | 0x0ddaf | 0, 1, 4 | agreed |
| `stdio_fopen` | 0x0d0ce | 0, 1, 4 | agreed |
| `find_free_stream` | 0x0d0a3 | 0, 1, 4 | agreed |
| `parse_open_mode` | 0x0cf4d | 0, 1, 4 | agreed |
| `open_file` | 0x0d5af | 0, 1, 4 | agreed |
| `dos_isatty` | 0x0c018 | 0, 1, 4 | agreed |
| `dos_ioctl` | 0x0c8a3 | 0, 1, 4 | agreed |
| `dos_getattr` | 0x0cd3d | 0, 1, 4 | agreed |
| `dos_open_named` | 0x0d707 | 0, 1, 4 | agreed |
| `dos_close` | 0x0cd80 | 0, 1, 4 | agreed |
| `close_handle` | 0x0cd58 | 0, 1, 4 | agreed |
| `stdio_fclose` | 0x0ce15 | 0, 1, 4 | agreed |
| `game_fopen` | 0x08fcd | 0, 1, 4 | agreed |
| `load_archive_map` | 0x0960f | 0, 1 | agreed |
| `hash_filename` | 0x0980d | 0, 1, 4 | agreed |
| `game_rewind` | 0x093e0 | 0, 1, 4 | agreed |
| `reset_file_record` | 0x23e23 | 0, 1, 4 | agreed |
| `game_fclose` | 0x0917f | 0, 1, 4 | agreed |
| `dos_tell` | 0x0c27b | 0, 1, 4 | agreed |
| `unread_count` | 0x0d20f | 0, 1, 4 | agreed |
| `stdio_ftell` | 0x0d2d4 | 0, 1, 4 | agreed |
| `ulong_divide` | 0x0bd97 | 0 | agreed |
| `fread_huge` | 0x0b93d | 0 | agreed |
| `game_ftell` | 0x093a2 | 0, 1, 4 | agreed |
| `flush_stream` | 0x0ce92 | 0, 1, 4 | agreed |
| `stdio_fseek` | 0x0d26c | 0, 1, 4 | agreed |
| `game_fseek` | 0x092dc | 0, 1, 4 | agreed |
| `game_fgetc` | 0x093f6 | 0, 1, 4 | agreed |
| `game_fread` | 0x091ef | 0, 1, 4 | agreed |
| `flush_pending_volumes` | 0x27a86 | 0, 1 | agreed |
| `sx_controller` | SX.OVL SPKR:0x03a1 | 0, 1 | agreed |
| `sx_pitch_bend` | SX.OVL SPKR:0x0410 | 0 | agreed |
| `sx_stop_note` | SX.OVL SPKR:0x037b | 0, 1 | agreed |
| `sx_start_note` | SX.OVL SPKR:0x0386 | 0, 1 | agreed |
| `sx_speaker_off` | SX.OVL SPKR:0x0480 | 0 | agreed |
| `sx_apply_bend` | SX.OVL SPKR:0x04fd | - | **transcribed, never called** on these screens |
| `sx_note_on` | SX.OVL SPKR:0x0497 | 0 | agreed |
| `vm_buffer_size` | VM.OVL VGA:0x138e | 0, 1 | agreed |
| `vm_show_page` | VM.OVL VGA:0x150f | 0, 3, 9 | agreed |
| `vm_copy_rect` | VM.OVL VGA:0x1561 | 0, 2, 5 | agreed |
| `vm_span` | VM.OVL VGA:0x034f | 0, 4, 9, 17, 40, 73 | agreed |
| `vm_blit_run` | VM.OVL VGA:0x0938 | 0, 2, 19, 3359, 3360 | agreed |
| `vm_fill_spans` | VM.OVL VGA:0x0be6 | 0, 1, 40, 300 | agreed |
| `vm_set_palette` | VM.OVL VGA:0x0ec1 | 0, 1, 3 | agreed |
| `present_frame` | 0x081cc | 0, 5, 20 | agreed |
| `fill_rect` | 0x20079 | 0, 3, 60, 900 | agreed |
| `step_word_4e87` | 0x0144e | 0, 5, 60 | agreed |
| `set_clip_full_screen` | 0x0834b | 0 | agreed |
| `sub_002be` | 0x002be | 0, 3, 12 | agreed |
| `clear_word_array_50bf` | 0x166d6 | 0, 1 | agreed |
| `bit0_of_468c` | 0x2147d | 0, 4, 25 | agreed |
| `advance_record` | 0x2891a | 0, 2 | agreed |
| `match_field_5a_5c` | 0x06f43 | 0, 3, 20 | agreed |
| `lookup_table_546c` | 0x11d44 | 0, 5, 30 | agreed |
| `string_contains_r` | 0x1c6e3 | 0, 2 | agreed |
| `flag_bit_48ea` | 0x2213e | 0, 4, 30 | agreed |
| `select_field_2_or_4` | 0x06f68 | 0, 3, 20 | agreed |
| `read_pair_4740` | 0x220e9 | 0, 2, 15 | agreed |
| `angle_sin` | 0x2a456 | 0, 4, 25 | agreed |
| `angle_cos` | 0x2a47b | 0, 4, 25 | agreed |
| `angle_to_quadrant` | 0x004d1 | 0, 5, 40 | agreed |
| `chain_contains` | 0x03a61 | 0, 3, 25 | agreed |
| `normalise_far_ptr` | 0x22161 | 0, 4, 30 | agreed |
| `follow_far_chain` | 0x2907b | 0, 1 | agreed |
| `step_pair_apart` | 0x03d2e | 0, 3, 20 | agreed |
| `points_within_140` | 0x04b53 | 0, 3, 20 | agreed |
| `splice_list_4e58_onto_4e56` | 0x07b3e | 0, 2, 10 | agreed |
| `scale_byte_pair` | 0x282cb | 0, 1 | agreed |
| `value_between` | 0x03d67 | 0, 3, 20 | agreed |
| `pick_by_flag` | 0x05b65 | 0, 3, 20 | agreed |
| `normalise_far_ptr_far` | 0x22386 | 0, 3, 20 | agreed |
| `compute_bounds_53fe` | 0x00386 | 0, 3, 20 | agreed |
| `pick_for_record` | 0x05ba7 | 0, 3, 20 | agreed |
| `set_side_flags` | 0x004fd | 0, 3, 20 | agreed |
| `far_memcpy` | 0x222c6 | 0, 2 | agreed |
| `claim_page_slot` | 0x0b429 | 0, 3, 9 | agreed |
| `save_or_restore_draw_state` | 0x0b47f | 0, 1, 8 | agreed |
| `clamp_record_pair` | 0x02bcc | 0, 3, 20 | agreed |
| `set_clip_for_mode` | 0x082c3 | 0, 2, 8 | agreed |
| `link_record_into_buckets` | 0x166ef | 0, 3, 20 | agreed |
| `update_velocity` | 0x07283 | 0, 3, 20 | agreed |
| `clip_and_draw_line` | 0x21e34 | 0, 3, 20 | agreed |
| `vm_draw_line` | VM.OVL VGA:0x0998 | 0, 2, 9, 30 | agreed |
| `far_memset` | 0x22300 | 0, 2, 9 | agreed |
| `compute_swept_bounds_5400` | 0x002dd | 0, 3, 20 | agreed |
| `angles_same_side` | 0x003df | 0, 3, 20 | agreed |
| `insert_sorted` | 0x05646 | 0, 3, 20 | agreed |
| `dos_alloc_bytes` | 0x21abd | 0, 2, 9 | agreed |
| `mul16x16` | 0x2a269 | 0, 5, 40 | agreed |
| `apply_gravity_and_speed` | 0x02c39 | 0, 3, 20 | agreed |
| `vm_load_palette` | VM.OVL VGA:0x0f15 | 0, 1, 2 | agreed |
| `set_palette_pointer` | 0x1eb6a | 0, 1, 2 | agreed |
| `rotate_point` | 0x03b17 | 0, 3, 20 | agreed |
| `alloc_shape` | 0x064b4 | 0, 3, 20 | agreed |
| `add_record_shapes` | 0x0642a | 0, 3, 20 | agreed |
| `recompute_kind_physics` | 0x02ac0 | 0, 1 | agreed |
| `reset_input_state` | 0x0b4f1 | - | **transcribed, never called** on these screens |
| `compute_link_endpoints` | 0x04e65 | 0, 3, 6 | agreed |
| `find_entry_for_pointer` | 0x098e0 | 0, 1, 4 | agreed |
| `erase_both_pages` | 0x080e7 | 0 | agreed |
| `erase_object` | 0x0ad51 | 0 | agreed |
| `restage_object_rect` | 0x0aef6 | 0 | agreed |
| `claim_buffer_slot` | 0x0b5ed | 0 | agreed |
| `clear_slot_5734` | 0x0b69c | - | **transcribed, never called** on these screens |
| `seek_file_to` | 0x09b38 | 0, 2 | agreed |
| `archive_entry_for` | 0x09b7c | 0, 1, 4 | agreed |
| `clear_flag_2d44` | 0x0a7a3 | 0, 1, 4 | agreed |
| `clear_flag_2d44_thunk` | 0x0811b | 0, 1, 4 | agreed |
| `resource_advance` | 0x1c8a7 | 0, 1, 4 | agreed |
| `select_resource` | 0x1c649 | 0, 1, 4 | agreed |
| `stdio_fgetc` | 0x0d404 | 0, 1, 4 | agreed |
| `buffered_read` | 0x0d0ed | 0, 1, 4 | agreed |
| `stdio_getc` | 0x0d3ef | 0, 1, 4 | agreed |
| `stdio_fread` | 0x0d1c4 | 0, 1, 4 | agreed |
| `refill_stream` | 0x0d396 | 0, 1, 4 | agreed |
| `read_translated` | 0x0da6d | 0, 1, 4 | agreed |
| `dos_read` | 0x0c185 | 0, 1, 4 | agreed |
| `dos_lseek` | 0x0c0c3 | 0, 1, 4 | agreed |
| `heap_malloc` | 0x0c999 | 0, 1 | agreed |
| `heap_free` | 0x0c8ca | 0, 1 | agreed |
| `dos_free_far` | 0x21b34 | 0, 1, 4 | agreed |
| `refresh_link_geometry` | 0x04f7f | 0, 1, 4 | agreed |
| `set_vector_from_angle` | 0x07223 | - | **transcribed, never called** on these screens |
| `link_slack` | 0x0713d | 0, 1, 4 | agreed |
| `link_endpoint_gap` | 0x07947 | 0, 1, 4 | agreed |
| `link_end_distance` | 0x06f8e | 0, 1, 4 | agreed |
| `shift_all_histories` | 0x07ca2 | 0, 1, 4 | agreed |
| `shift_state_history` | 0x07ce3 | 0, 1, 4 | agreed |
| `compare_link_ends` | 0x06de9 | 0, 1, 4 | agreed |
| `intersect_segments` | 0x03ba9 | 0, 3, 20 | agreed |
| `frame_pending` | 0x0b4e2 | 0, 1 | agreed |
| `decode_position` | 0x1e561 | - | **transcribed, not verifiable**: it has no return to detect - the compiler replaced its `ret` with `jmp 0x1e89c`, so 0x1e7f2 jumps in and it jumps back. Covered by decompress_lzss, which runs it on every one of its 226 verified calls. |
| `wait_and_latch_frame` | 0x0aaca | - | **transcribed, not verifiable**: waits for an interrupt the harness must suppress |
| `update_button_state` | 0x08136 | - | **transcribed, not verifiable**: calls wait_and_latch_frame, which waits for an interrupt |

*262 transcribed, 231 verified. Written by `tools/verify.py --all`, not by hand - one run of the original captures every call.*
<!-- VERIFY:END -->

Each routine is checked at **more than one occurrence**, because a check at one
value of a routine's inputs says nothing about the others: `vm_show_page`'s
first call has both page segments equal, so the swap is a no-op and the start
address is zero - it agrees there whatever it does with the pages. Occurrences
3 and 9 have them differing.

`frame_pending` has no hardware effect at all, so a trace comparison would have
found "0 writes on both sides" and called it agreement. It is checked on its
return value instead, at both values its one input takes, with the DGROUP word
seeded from the original's own memory at the moment of the call.

Two contaminations had to be removed before any of this meant anything: an
interrupt firing *inside* a routine wrote its own end-of-interrupt to port 0x20
and appeared as three events of the routine's, and the original's single 16-bit
`out dx, ax` had to be recorded as the two 8-bit writes the port performs.

The 577 come from direct calls only. Indirect calls through handler tables are
**not** followed yet, so the true figure is higher - finding those tables is the
next high-leverage task, not an afterthought.

**The renderer is the driver, not the game.** Every pixel the title screen
draws is written by `VM.OVL` - see `docs/video-driver.md`. The game reaches it
through a vector table in DGROUP filled in by the loader, so those entry points
are invisible to a static map and are resolved from the running machine by
`tools/driverapi.py`.

The 218 is measured by `tools/reached.py`, which records the basic blocks
executed between two page flips and intersects them with the code map. The
title screen also reaches **263 distinct blocks inside `VM.OVL`**, which are not
in the 577 at all because the overlay is loaded at run time and the static map
does not see it.

### The original's translation units

Large model, so each module is its own code segment and the boundaries are read
off the binary rather than guessed. Eight of them:

| segment | image range | routines |
| --- | --- | --- |
| `0000` | 00000..0dff0 | 270 |
| `0dff` | 0dff0..14de0 | 69 |
| `14de` | 14de0..1c250 | 32 |
| `1c25` | 1c250..248f0 | 112 |
| `248f` | 248f0..26190 | 20 |
| `2619` | 26190..2a040 | 69 |
| `2a04` | 2a040..2d290 | 4 |
| `2d29` | 2d290..2d3c0 | 1 |

Which of these are the C runtime rather than the game is **not yet
established**. Segment `0000` contains the Borland startup (the entry point is
`0000:0000`) and also game code - the CRTC routine at 0x8f77 - so it is not a
clean split, and the question is open.

### Emulator gaps closed, and where they belong

All four were found by this game and all four are generic; they live in
`TimMachine` in `tools/tim.py` and **have not been pushed upstream yet**.

1. **INT 10h AH=1Ah and AH=12h** were unimplemented.
2. **Program memory ownership.** An EXE with `maxalloc = 0xFFFF` owns all of
   conventional memory until its runtime shrinks it with AH=4Ah.
3. **Vertical blanking was ignored** by the planar renderer.
4. **CRTC registers read back as 0.** The register file now starts from the
   BIOS's own mode-12h table. This one is the cautionary example: the game
   reads Overflow and Maximum Scan Line back before setting one bit in each, so
   with reads returning 0 it silently cleared every other timing bit - and it
   *happened to reach the same blanking line anyway*, so nothing looked wrong.

### Calling conventions found so far

Not everything is cdecl, and getting this wrong reads a return address as an
argument:

- **far cdecl** - the common case; arguments from `[bp+6]`.
- **near cdecl** - `ret`, not `retf`; arguments from `[bp+4]`.
- **register** - the driver's blitters, and some helpers, take arguments in
  registers and one takes the *carry flag* as a direction.
- **pascal** - `ret 2`: the callee clears its own argument. So far only in
  Borland's runtime, never in the game's own code, which is itself a signal
  when classifying a routine.
- **no frame at all** - `mov bx, sp` and index off that, as sine and cosine do.

### Transcribed, stubbed, and the difference

`reconstruct/tests/provenance.py` now counts three things, not two, because
"we know where this routine is" and "we have read it" are different claims:

- **transcribed** - the body was read from the disassembly;
- **stub** - the address is known and the body is not written yet. A stub
  **aborts** when reached rather than returning quietly, because a silent
  no-op in a drawing path is a missing frame that looks like a blitter fault;
- **ours** - the port's own, said so explicitly.

Two stubs exist, both reached from `present_frame` at 0x081cc: `0x0b078` and
`0x0e34a`. Neither is reachable on the intro screens - **all 436 calls to
`present_frame` while they run have both DGROUP flags at zero**, which is
measured rather than argued.

### What is checked, and what a check covers

Each routine is verified at several occurrences chosen to reach **different
paths**, not merely several times. `vm_span`'s six are one byte-aligned
multi-byte run, two unaligned ones, one ending exactly on a byte boundary, and
two that fit inside a single byte - the last found by scanning the arguments of
all 67,970 calls, because the first four never reached that branch and four
checks of one path are one check.

Where a routine can still go somewhere untranscribed, the port **aborts**
rather than guessing, and the fact that the branch is unreachable in the states
being compared is measured rather than assumed.

### A second model corrected

The port had an array of its own for the span lists the rectangle routine
builds. It passed every check until the comparison widened from DGROUP to all
of conventional memory - and then `fill_rect` and `vm_fill_spans` both failed
at once, because the original writes that list into a **block DOS gave it**,
named by a segment in DGROUP 0x4342, which the port never touched.

The port now models the guest's whole address space as a flat megabyte, with
DGROUP as a window into it and far pointers formed the way the hardware forms
them. That is also what let `lookup_table_546c` be transcribed at all: it
follows `les bx, [0x546c]` into an allocation outside DGROUP.

### A model corrected

The video driver's data is **not** a separate segment: it lives inside DGROUP
at offset 0x3890, and the game writes the driver's page variables directly
through DGROUP. The port had them as separate C globals, which was wrong, and
the whole-segment comparison is what caught it - `vm_show_page` and
`present_frame` failed the moment the check became strong enough to notice.
`docs/video-driver.md` has the evidence.

### The C runtime occupies the top of segment 0000

Routines from **0x0bd00 to the end of the segment at 0x0dff0** are Borland's,
and are skipped as a block rather than read one at a time. This is a claim
about the module layout and it is kept separate from the routines that were
actually read, because they are different kinds of knowledge:

- 23 in that range have been read individually and every one is Borland's -
  stdio, `malloc`, long arithmetic, `errno`, file I/O;
- 19 below the line have been read and every one is the game's; the only
  runtime routine below it is the stderr write at 0x00274, in the start-up
  module at the very bottom;
- the linker lays each module down whole and in link order, and the runtime
  links last, so a contiguous run at one end is what a runtime cluster is.

Below that block sit a few **far wrappers** - push the arguments back, call the
near runtime routine, return - which are recognised structurally, by their
whole body being argument forwarding around exactly one call into the block,
rather than by address.

Anything in there that later proves to be the game's own is a retraction to
record, not a surprise to absorb quietly.

### DOS allocation is primed, not simulated

`dos_alloc_bytes` (0x21abd) calls DOS for memory, and the port has no DOS and
no arena, so it cannot decide where a block goes. Rather than mark the routine
unverifiable - it has twenty-two callers - the harness records what DOS
answered during the original's own call and **primes** the port with it. Every
part of the routine except the address itself is then genuinely compared: the
32-bit size arithmetic, the round-up, the "how much is free" path, and the
zero fill.

Asked for an allocation nothing has primed, the port aborts rather than
inventing an address.

### Not yet settled: routines that call Borland's allocator

`0x1c705` is "free this if it is not null". The port models the guest's memory
and the verifier compares all of it, so a routine that calls `malloc` or `free`
cannot agree unless the port moves the same heap bytes - which would mean
transcribing Borland's allocator after all, or excluding the heap from the
comparison. Neither has been decided, so such routines are left alone rather
than transcribed into a check that cannot pass.

### Bugs in the original, transcribed as they behave

Two so far, both in the same family and both left as they are:

- `far_memcpy` (0x222c6) aligns its destination with `test di,1 / jae`, and
  `test` always clears carry, so the branch is **always** taken and the
  aligning byte is never copied.
- `far_memset` (0x22300) does the same job with `or di,di / jp`, and `jp` is
  jump-if-**parity**: it stores the aligning byte according to how many bits
  are set in the low byte of the address, which has nothing to do with whether
  the address is even.

Both were presumably meant to be a test of bit 0. Neither is corrected: the
port reproduces what the original does, and the reasoning is in the source next
to the code.

### Retractions and near-misses

- **2026-08-28. The long divide was recorded as a comparison.** 0x0bd90 and its
  three siblings were classified as long comparison helpers on the strength of
  their shape - a family of entries loading a small constant into CX and
  jumping to one body. The body turned out to take two 32-bit arguments, keep
  the selector in DI, test its bit 0 for signedness and negate the operands by
  sign: it divides. Found when 0x02ac0 was seen calling 0x0bd90 to divide.
  Nothing downstream changed - it is runtime either way - but the description
  in `docs/runtime.md` was wrong and is corrected there.

- **2026-08-28. `find_free_slot_4bc4` was not the game's.** It was transcribed
  from 0x0d0a3 as a free-slot scan over 16-byte records and verified. It is
  Borland's: those records are `FILE` structures, the signed byte at +4 is the
  file descriptor, and the count beside it at DGROUP 0x4d04 is the stream
  count - the table's neighbour at 0x4d06 is the handle-flags table already
  identified. Removed from the port. It was the only apparent game routine in
  the runtime block, and it turned out not to be one, which is part of why the
  block claim above stands.

- **2026-08-28. A routine was named wrongly and is corrected.** 0x22161 was
  transcribed as `fixed_normalise`, on the reading that AX held a fixed-point
  fraction and DX its whole part. The six instructions are the same either way,
  so it verified, and the wrong name stood. 0x222c6 settled it: that routine
  calls 0x22161 on the offset and segment halves of *two far pointers* and then
  copies between them, which only makes sense for **far pointer
  normalisation**. Renamed to `normalise_far_ptr`, and the correction is
  recorded in the source rather than quietly applied - a wrong name outlives a
  wrong line.

- **2026-08-28. Occurrence numbers are not stable across runs.** The batched
  sweep suppresses timer and keyboard interrupts while any tracked routine is
  open, so that an interrupt's own hardware writes are not counted as the
  routine's. That gating perturbs how far the guest gets in a given number of
  instructions, so the *N*th call to a routine in the sweep is not necessarily
  the *N*th call in an ungated run: a probe saw eight calls to `advance_record`
  where the sweep saw fewer. An earlier commit message claimed the numbering
  "means the same thing" across runs. It does not. Low occurrence numbers are
  reliable; the last call is not, and the sweep now prints how many calls it
  actually saw so the choice is grounded rather than guessed.

- **2026-08-28.** Renaming two driver variables in the C left the old names in
  the verifier's spec, and `vm_copy_rect` and `vm_fill_spans` went from
  *verified* to *not verified* until the sweep was re-run. Nothing about the
  transcription was wrong; the tooling was. This is why the sweep regenerates
  the table rather than the table being edited: a claim that is only ever added
  to is a marketing document.
- The same sweep reported `vm_fill_spans` occurrence 300 as **NOT ENTERED**
  rather than passing it. The routine is called 1,078 times in all but not that
  often within the default instruction budget. Distinguishing "never called"
  from "called and agreed" is the whole point of that message.

### Branches transcribed but never run

Verified means the paths that were reached agreed. These were not reached:

- `step_word_4e87` (0x0144e) wraps its counter at 0x2a00. That needs 10,752
  calls; over both intro screens it is called 428 times and the counter never
  exceeds 0x1ab.
- `fill_rect` (0x20079) has an outline path, and `present_frame` (0x081cc) two
  hooks - all stubs, all measured unreachable here.
- `vm_span` (VGA:0x034f) and `vm_fill_spans` (VGA:0x0be6) each branch to a
  high-colour variant that no call on these screens takes.

### Three outcomes that are not "verified"

The sweep distinguishes them, because collapsing any of them into a pass would
be the whole failure this project exists to avoid:

- **transcribed, never called** - the routine exists in C and nothing on these
  two screens reaches it, so nothing has been checked. `reset_input_state`
  (0x0b4f1) is the first.
- **transcribed, not verifiable** - the harness cannot run it, for a reason it
  names.
- **not verified** - it was checked and it differed, or an occurrence that was
  asked for was never reached.

### Routines the harness cannot verify

`wait_and_latch_frame` (0x0aaca) is **transcribed and not verified**, and the
sweep reports it that way rather than counting it as agreeing.

Its whole purpose is to wait for the INT 08h handler to set a flag. The harness
suppresses interrupts while a routine is open, so that an interrupt's own
hardware writes are not attributed to the routine - and with them suppressed
the original's spin can never be released, so the emulator sits in it forever.
Verifying it would need the harness to *distinguish* an interrupt's effects
from the routine's rather than excluding them, which it cannot do today.

The port's own side of that wait is IO: the loop body calls
`io_await_frame_tick`, the port's stand-in for the handler, because an empty
spin in C could never exit. That is marked as ours in `io.c`.

`update_button_state` (0x08136) inherits the same limit, because it calls that
routine. Anything that waits for an interrupt, directly or through a callee,
falls in this class.

**The harness now has a watchdog** so this can never hang a whole sweep again:
an instance still open after 30M instructions is abandoned, reported by name
and occurrence, and counted as not verified. Finding the first such routine
cost a run that never finished.

### Limits of the verifier as it stands

- It compares **writes**, not reads. A read has no external effect of its own -
  it can only change behaviour through a write that follows - so the writes are
  the complete observable. But the original's reads are not recorded at all,
  because a second Unicorn IN hook would override the emulator's own and change
  what the guest sees.
- ~~It knows how to seed only the DGROUP words a routine is declared to use.~~
  **Fixed.** The port models DGROUP as a 64 KB byte array, so the verifier
  seeds the **whole segment** before a call and compares the whole of it
  afterwards. A routine touching state nobody declared is now caught rather
  than missed, and near pointers into DGROUP work at all.
- The comparison now covers **all 640 KB below the VGA aperture**, not just
  DGROUP, so a routine writing into an allocation is checked too.
- **The driver's own code is excluded, deliberately.** `VM.OVL` is
  self-modifying - VGA:0x0be6 patches the row-table pointer into `cs:[0xbe4]`
  and VGA:0x15d0 patches an immediate at `cs:[0x15ce]` - and a C transcription
  has no code to patch. This is the one class of difference the port cannot
  reproduce and should not; it is excluded by name and reason, not because it
  was awkward.
- **A routine's own arguments are excluded too.** Several here modify them in
  place - `far_memset` walks its 32-bit count down with `sub`/`sbb` - and in
  cdecl the caller pops them, so those writes cannot be observed by anyone.
  The port's arguments live in its own frame.
- The stack is **inside** the compared segment - SS is DGROUP in this program -
  so the bytes a call used as stack are excluded, bounded by the lowest SP the
  call reached. The port has its own C stack and cannot reproduce them.
- Registering memory hooks across all of memory **derails the guest** - it
  opened a file with a garbage name and then executed an invalid instruction.
  They are range-limited to the VGA aperture.

### Known gaps, not argued away

- **The emulated instruction rate is a guess.** `drive.DEFAULT_IPS` is
  2,000,000, chosen and not measured. It sets the frame rate the guest believes
  it is achieving, so no timing claim can be made until it is measured against
  the original in cycles.
- **Sound is not modelled.** `SX.OVL` is loaded but the sound path is unchecked.
- **`VM.OVL`'s other seven drivers** are never executed and never will be.
- The **name-hash** in `RESOURCE.MAP` is not derived.

## Next

1. Find the **handler tables** and re-seed the code map through them; the 577 is
   a floor, not a count.
2. Establish which segments are the C runtime and which are the game.
3. Decide which of the 577 are the **Borland C runtime**. Those are not the
   game's logic and reconstructing them from Borland's binary is both pointless
   and worse legally; the port uses the host's C library and marks them as
   ours, kept out of the verifier's dispatch. Which routines those are is not
   yet established.
4. Continue transcription, targeting the **two intro screens** - the title screen
   (page flips 6..279) and the credits screen (from flip 280) - both of which
   animate and so exercise real game logic, and prove each routine against the
   original rather than against the screen.

## Borland's own allocator

`reconstruct/borland_heap.c`. Not the game, and not what this port is
reconstructing - but game routines call `free` and `malloc`, and the
whole-memory comparison cannot pass them unless the port moves the same heap
bytes. So it is transcribed, in a file of its own.

**It is kept, not deleted.** This is Borland's allocator rather than this game's,
so having it transcribed and checked against a real binary is worth something to
anyone taking apart another Turbo C or Borland C++ DOS program. Whether this
port links it is a separate question from whether it exists.

Read from the disassembly and confirmed against the published description of the
near heap. A block header is four bytes below the caller's pointer - size at +0
with **bit 0 as the in-use flag**, previous-block-by-address at +2 - and a free
block reuses its own first four payload bytes as a doubly linked ring at +4 and
+6, which is why the smallest block is eight. `0x4e34` is the first block,
`0x4e36` the topmost, `0x4e38` the ring cursor, `0x9c` is `__brklvl` and `0x94`
`errno`.

Done and verified: `heap_free` and its four helpers, at 379 calls. `malloc`
(0x0c999) and its own helpers are not transcribed yet.

## The file layer

The port has none, and two transcribed routines are limited by that.

`seek_file_to` (0x09b38) **is** verified, but only at occurrences that take its
cached path. `io_file_seek` is a stand-in whose limit was measured rather than
assumed: a no-op was tried, and an occurrence that seeks a long way then showed
four bytes differing at DGROUP 0x4c14 - the runtime's own `FILE` buffer, which
its `fseek` resets.

`make_file_current` (0x09a62) is transcribed and **not** verified. Every
occurrence sampled reaches `fopen`, which refuses rather than inventing a
`FILE` for everything above it to read through.

`io.c` now serves DOS file reads and seeks **read-only from the game
directory**, and `borland_file.c` has the runtime's `read` (0x0c185) and `lseek`
(0x0c0c3) over them - same standing as `borland_heap.c`, kept for reference.
Handles are numbered from 5, as DOS does once the five standard ones are taken,
because the guest stores the number it is given and the comparison sees it.

Both **are** verified. A handle and a file position are not in guest memory, so
seeding memory was never enough on its own - but the emulator knows both.
`TimMachine._dos` tracks INT 21h AH=3Dh, 3Eh, 3Fh and 42h into a handle-to-name
map, `tools/verify.py` captures it at each instance, and `io_prime_file` reopens
the same file at the same offset. The same remedy as `io_prime_dos_alloc`, and
in the same place.

Until then the loading path above them stays unverifiable, and with it the seven
sound-module routines that load and decompress.

### The chain, as measured

Everything below is read from the disassembly, not guessed, and each step was
confirmed by hooking the running game:

```
sound routines 0x28bf2 0x28cf7 0x28e87 0x28f74 0x289e8 0x29da0 0x296b4
  -> 0x1d868, 0x1d983
    -> 0x1c92b            dispatch through the table at DGROUP 0x3580
      -> 0x1c278  type 1  (1 call)     [verified]
      -> 0x1ca62  type 2  (12 calls)   helper  0x1cc65
      -> 0x1e7f2  type 3  (226 calls)  helpers 0x1e0b3 0x1c5a3
         input   0x1c389  next byte      [verified, 1,471 calls]
                 0x1c319  run into huge  [verified, 119 calls]
         output  0x1c493  literal run    [verified, 119 calls]
                 0x1c51e  fill run       [verified, 129 calls]
                 0x1c5a3  one byte       [verified, 2,500 calls]
        -> 0x091ef  fread wrapper   [verified, 7,597 calls]
          -> 0x09a62  open      18,930 calls, 26 reach DOS   [transcribed]
          -> 0x09b38  seek      18,930 calls, 319 reach DOS  [verified]
          -> 0x09b7c  archive?  10,454 calls                 [verified]
          -> 0x0d1c4  runtime fread
             -> 0x0d0ed  buffered read
                -> 0x0d3ef  getc   [verified]
                   -> 0x0d404  fgetc  [verified]
                      -> 0x0d396  refill [verified]
                         -> 0x0d36d  flush all streams [verified]
                            0x0da6d  translating read  [verified]
                            -> 0x0c185  read   [verified, 441 calls]
                               0x0c0c3  lseek  [verified, 472 calls]
```

**The handler table was measured, not read off.** Hooking the indirect call at
0x1c94c gives exactly three live entries - index 1 to 0x1c278, 2 to 0x1ca62 and
3 to 0x1e7f2 - which is how their call counts above are known.

The two decompressors left, 0x1ca62 and 0x1e7f2, are hand-written assembly that
switches DS and keeps its state in registers across jumps; they are the largest
single piece of work remaining on this chain.

That whole stdio column is now transcribed and verified: `0x0d1c4` (`fread`),
`0x0d0ed` (its buffered inner loop), `0x0d3ef` (`getc`), `0x0d404` (`fgetc`),
`0x0d396` (the refill), `0x0d36d` (the flush it calls first) and `0x0da6d` (the
translating read), bottoming out in the already-verified `0x0c185`/`0x0c0c3`.

Two routines named on that chain are *not* transcribed, and neither is reached:
`0x0cd9e` - a DOS IOCTL call, so `isatty` or `eof` - and `0x0ce92`, a stream
flush. Both hang off `fgetc`'s unbuffered branch, and every stream the game
reads has a 512-byte buffer, so that branch aborts rather than pretending.

After those: the `fopen`/`fclose` pair at `0x0d0ce`/`0x0ce15`, then `0x091ef`
and the three decompressors.

The handler table for 0x1c92b is at DGROUP 0x3580, fourteen bytes per entry with
the handler offset first; which entries are live was measured, not read off the
table.

**Even the archive path calls the runtime's `fread`.** It substitutes the
archive's own `FILE` and reads through the same buffered layer, so there is no
route through the loader that avoids stdio - which is why the file layer is not
optional. Priming file state in the harness is what made any of it checkable;
the per-routine path does not prime, so these routines are only meaningful
under `--all`.

## Deferred

- ~~**The sound module, segment 2619.**~~ **No longer deferred** - the user
  asked for it directly. The reasoning below is kept because it records why it
  was set aside and what that was costing; it is history, not current policy.
  `tools/worklist.py --sound` includes the module in the work list.

- **The sound module, segment 2619.** Its routines call through a vector in
  their own code segment at `cs:[0x1e7]` and keep their tables beside it, and
  they are on the intro screens' execution path - but **not on the drawing
  path**: attributing every A000 write of nine frames to the instruction that
  made it found all of them in `VM.OVL`, reached from segments 0000 and 1c25.
  They cannot change a pixel, so they are deferred against the goal of matching
  the two screens.

  Measured, so that the cost of the deferral is on the record: it keeps three
  game routines permanently blocked. `0x083ab` (5 callers) is a thin dispatcher
  whose whole body calls `0x2619:0x38b9` = `0x29a49`; `0x03009` then waits on
  `0x083ab`. `0x29a49` itself walks a record list and would transcribe easily,
  but it calls `0x294ff`, `0x28935`, `0x29034` and `0x287ad`, all inside the
  module, so taking it means taking a large part of the module with it. That is
  the right trade against a pixel goal and the wrong one against a complete
  port; it is a scope decision, not an oversight.

  If a sound routine turns out to share state with the drawing
  code, that is a retraction to record.

- Matching (byte-exact) decompilation.
- The seven non-VGA drivers in `VM.OVL`.
- Sound.
- Anything past the intro screens: the menu, the puzzles, the level editor.
