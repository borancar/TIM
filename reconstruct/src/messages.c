/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game's message strings**, DGROUP 0x14ec..0x1d3c: the path
 * separator's and the newline's pointers, the texts, and the two literals
 * the pointers name, which are this module's pool.
 *
 * A data module of its own in 1.11: gamedata.c's tables end at 0x14eb and
 * the pad byte there puts this `_DATA` on a word boundary, as TLINK starts
 * a new object's. 1.00's gamedata.c held both; there nothing was padded to
 * show a boundary.
 *
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x14ec..0x1d3c
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* DGROUP 0x14ec and 0x14ee in 1.11: the path separator and the newline
   `game_teardown` prints, as literals - the module's pool, after the texts.
   1.00 pointed at a field of `g_messages` for the separator. */
char *g_path_separator = "\\";
char *g_newline = "\n";
struct messages g_messages = {
    "CONTINUE",   /* button_continue */
    "YES",   /* button_yes */
    "NO",   /* button_no */
    "REPLAY",   /* button_replay */
    "ADVANCE",   /* button_advance */
    "\012\012NOT ENOUGH FREE MEMORY\012",   /* not_enough_free_memory */
    "\012You need at least 560k of free memory to run 'The Incredible Machine'.\012\012",   /* you_need_at_least */
    "\012\012The data disk requires the original version of 'The Incredible Machine' in orderto run.\012\012",   /* data_disk_requires */
    "Unable to initialize vm.",   /* unable_to_initialize_vm */
    "\012\012Thanks for playing 'The Incredible Machine'.\012The last password given to you was:  ",   /* thanks_for_playing */
    "Please select, in order, the three parts listed on page ",   /* please_select_in_order */
    " of the user's manual.",   /* of_the_users_manual */
    "VERSION NUMBER",   /* version_number */
    "This is version 1.11 of 'The Incredible Machine.'",   /* this_is_version */
    "MEMORY LOW",   /* memory_low */
    "Memory is getting low.  You can only place a few more parts.",   /* memory_is_getting_low */
    "OUT OF MEMORY",   /* out_of_memory */
    "You can't place any more parts.",   /* you_cant_place_any */
    "QUIT GAME",   /* quit_game */
    "Are you sure you want to quit the game?",   /* quit_body */
    "RESTART LEVEL",   /* restart_level */
    "Are you sure you want to clear all parts and restart this level?",   /* restart_body */
    "FREEFORM MODE",   /* freeform_mode */
    "Are you sure you want to enter freeform mode?",   /* freeform_body */
    "LEAVE FREEFORM MODE",   /* leave_freeform_mode */
    "Are you sure you want to leave freeform mode?",   /* leave_freeform_body */
    "CAN'T CHANGE GRAVITY",   /* cant_change_gravity */
    "You are only allowed to change the gravitational force in freeform mode.",   /* gravity_body */
    "CAN'T CHANGE AIR PRESSURE",   /* cant_change_air_pressure */
    "You are only allowed to change the air pressure in freeform mode.",   /* air_pressure_body */
    "CLEAR PARTS BIN",   /* clear_parts_bin */
    "Are you sure you want to clear the parts bin?",   /* clear_parts_bin_body */
    "ADJUST PARTS BIN",   /* adjust_parts_bin */
    "OVERWRITE FILE",   /* overwrite_file */
    "File already exists.  Do you want to overwrite it?",   /* overwrite_body */
    "FILE ERROR",   /* file_error */
    "Unable to open that file for saving.",   /* cant_open_for_saving */
    "Unable to open that file for loading.",   /* cant_open_for_loading */
    "Disk is write protected or there is not enough memory on that disk to save this machine.",   /* disk_write_protected */
    "PATH ERROR",   /* path_error */
    "Unable to choose that path.",   /* path_error_body */
    "WRONG FORMAT",   /* wrong_format */
    "That file has not been saved in 'The Incredible Machine' format.",   /* wrong_format_body */
    "NEED PASSWORD",   /* need_password */
    "You need to enter the correct password in order to try this puzzle.",   /* need_password_body */
    "BAD PASSWORD",   /* bad_password */
    "That is not a valid password.",   /* bad_password_body */
    "SCORE CODE INVALID",   /* score_code_invalid */
    "That score code is invalid.  Your score will be set to zero.",   /* score_code_body */
    "<PARENT DIR>",   /* parent_dir */
    "LOAD MACHINE",   /* load_machine */
    "SAVE MACHINE",   /* save_machine */
    "LOAD",   /* load */
    "SAVE",   /* save */
    "CANCEL",   /* cancel */
    "File Name:",   /* file_name */
    "FREEFORM MODE",   /* freeform_mode_title */
    "PUZZLE ",   /* puzzle_prefix */
    " COMPLETED!",   /* completed */
    "Total bonus points: ",   /* total_bonus_points */
    "New Password",   /* new_password */
    "",   /* empty */
    "(click button to continue)",   /* click_button_to_continue */
    "REPLAY SOLUTION",   /* replay_solution */
    "Do you want to advance to the next puzzle or replay your solution to this puzzle?",   /* replay_body */
    "SELECT PUZZLE",   /* select_puzzle */
    "PASSWORD",   /* password */
    "SOLVED ALL PUZZLES",   /* solved_all_puzzles */
    "(Click here to enter description)",   /* enter_description */
    "Wow!!  INCREDIBLE Job!!!  You have solved all of the puzzles!!  Advance will take you to freeform mode.",   /* solved_all_body */
};
