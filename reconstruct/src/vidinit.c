/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Bringing the video up and handing the machine back**: `vm_init`, which
 * picks the adapter and loads and starts its driver, the adapter probe it
 * calls, and the routines that put the keyboard, mouse, timer and video mode
 * back on the way out.
 *
 * A module of the original's **code segment 1c25**, image 0x22483..0x22790,
 * split out of engine.c on 2026-09-27. Its `_DATA` is DGROUP 0x48f2..0x48f8,
 * `VM_START`, and it starts a byte after the divide trap's: the pad at
 * 0x48f1 is the word alignment of a new module's data.
 *
 * **So no C compiler judges this file.** Every routine is hand-written:
 * `detect_adapter` answers in AL and calls two near helpers, one answering
 * in the carry flag; `vm_init` copies the driver's table with `rep movsw`
 * across a borrowed DS; `restore_video_mode` pushes its argument and pops it
 * back. It is the host's transcription of an assembly module, and the
 * byte-exact source of these bytes is TASM's to make (not written yet). The
 * functions are in address order and each carries the image offset it was
 * read from.
 *
 * Where the module begins is not settled. 0x2241b..0x22483, the two clipped
 * pixel routines and the `vm_restore_rect` thunk, is assembly too and has no
 * data of its own, so nothing says whether it ends the divide trap's module or
 * starts this one; it stays in engine.c.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **This module's `_DATA`**, DGROUP 0x48f2..0x48f8: nothing recorded yet,
 * nothing forced, and no driver.
 */
struct vm_start VM_START DGROUP_AT(0x48f2) = {
    .mode_found = 0xff,
    .mode_forced = 0xff,
};

/*
 * 0x22483
 *
 * Bring the video up: pick the adapter, load its driver, start it, and build
 * the far vector table every drawing call goes through. Answers the adapter
 * code, or 0 if there is none or the driver would not load.
 *
 * The table at DGROUP 0x4346 is built in two passes. The driver's start-up
 * answers `DX:SI` pointing at its own table of near offsets; 0x64 words of that
 * are copied in, and then the driver's segment is written into **every second
 * word** 0x32 times. Both counts say fifty entries, which is what the table
 * actually is.
 *
 * DGROUP's own segment is planted at 0000:04f0 on the way past, where anything
 * that needs to find the program's data can read it.
 *
 * **The 8x8 font pointer is not what it looks like.** `INT 10h AX=1130 BH=3`
 * is the BIOS "get font pointer" call and it answers in **ES:BP**, so the two
 * pairs the game files here are that answer and not, as this note once said,
 * the routine's own frame pointer.
 *
 * Nothing implements it on either side. The emulator leaves the registers as
 * it found them, so it answers `0000:ffca` - which is BP, which is the frame
 * pointer, which is why the mistake was easy to make - and the port answers a
 * plain zero, because a font pointer aimed at the stack is an accident rather
 * than a behaviour to reproduce. Real fonts are a separate piece of work.
 *
 * So the two sides differ by four bytes at DGROUP 0x618a and 0x618e, on
 * purpose. `vm_init`'s spec carries `deviation` so a sweep says so, and
 * STATUS.md records it.
 */
uint16_t vm_init(uint16_t adapter, uint16_t unused, FILE *file)
{
    /*
     * **No frame.** The prologue at 0x22483 is `push bp / mov bp,sp / push si
     * / push di` with no `sub sp` at all: the four bytes are SI and DI, and
     * saving registers is not something the port has to model.
     *
     * The font pointer below is not this routine's BP either, however much it
     * looks like it - see `io_bios_font_ptr`.
     */
    struct bios_font_ptr font;
    uint16_t al;
    uint16_t r;

    (void)unused;

    VM_START.mode_forced = (uint8_t)adapter;
    VMDS.screen.mode_kind = 0;
    VMDS.vga_chunks = 0;
    VMDS.screen.screen_width = 0x140;
    VMDS.screen.screen_height = 0xc8;

    if (dg_far_ptr(VMDS.palettes.blocks[0]) != FAR_NULL_PTR) {
        dos_free_far(dg_far_ptr(VMDS.palettes.blocks[0]));
        VMDS.palettes.blocks[0] = FAR_NULL;
    }

    VM_START.mode_found = (uint8_t)bios_video_kind();

    al = detect_adapter() & 0xff;
    VMDS.pixel_shift = (uint8_t)al;

    if (al != 0) {
        uint8_t *p = load_video_driver((int16_t)al, (char *)file);

        /* Only DX is tested. */
        if (FP_SEG(p) == 0) {
            VMDS.pixel_shift = 0;
        } else {
            uint16_t seg;
            int16_t i;

            VM_START.driver = far_of(p);

            vm_driver_init(&VMDS, DG440E.driver_table, DGROUP_SEG);
            seg = VM_START.driver.seg;

            /* a hundred words of the driver's table at its 0x13e, word by
               word, and then the driver's segment over every second one */
            {
                const int16_t far *table = (const int16_t far *)(void *)
                    MK_FP(seg, 0x13e);

                for (i = 0; i < 0x64; i++)
                    ((int16_t *)DG4342.font)[i] = table[i];
            }

            for (i = 0; i < 0x32; i++)
                DG4342.font[i].seg = seg;
        }
    } else {
        VMDS.pixel_shift = 0;
    }

    *(uint16_t *)(guest_mem + 0x4f0) = DGROUP_SEG;

    VMDS.page_src_ptr = ((int16_t)VMDS.page_front_ptr);
    VMDS.page_dst_ptr = ((int16_t)VMDS.page_back_ptr);

    r = ((uint8_t)VMDS.pixel_shift);
    if (r == 0)
        goto out;

    if (DG4342.span_buffer_seg != 0)
        dos_free_far(MK_FP((uint16_t)(DG4342.span_buffer_seg - 1), 0));

    {
        uint8_t *p = dos_alloc_bytes((uint16_t)(((uint16_t)VMDS.screen.screen_height) * 4 + 0x20), 0, 0).ptr;

        /* Only the segment is kept, and tested: `or dx,dx`. */
        if (FP_SEG(p) == 0)
            goto out;

        DG4342.span_buffer_seg = (int16_t)(FP_SEG(p) + 1);
    }

    /*
     * `mov ax,0x1130 / mov bh,3 / int 0x10`, and the answer is in **ES:BP** -
     * so the four words are that pair, filed twice. The emulator does not
     * implement the call, which is why they come back zero; see
     * `io_bios_font_ptr`.
     */
    font = io_bios_font_ptr(3);

    ENGINE_FONTS.body[0] = MK_FP(font.es, font.bp);
    ENGINE_FONTS.body[1] = MK_FP(font.es, font.bp);

    *(int16_t *)(&VMDS.font_table_48[0]) = 0x808;
    *(int16_t *)(&VMDS.font_table_34[0]) = 0x808;
    *(int16_t *)(&VMDS.font_table_5c[0]) = 0;
    *(int16_t *)(&VMDS.font_table_70[0]) = (int16_t)0xffff;

out:
    return r;
}

/*
 * 0x225a5
 *
 * The four things that have to be handed back before the program can leave:
 * the keyboard, the mouse, the timer and vector 0. Four calls and nothing else.
 */
void shutdown_input(void)
{
    remove_keyboard();
    remove_mouse();
    timer_remove();
    restore_int0_vector();
}

/*
 * 0x225ba
 *
 * Put the adapter back in the mode the program found it in. DGROUP 0x48f2 is
 * that mode, and 0xff means it was never recorded - which is why the store of
 * 0xff afterwards is inside the test and not after it: having restored the
 * mode, the record is spent.
 */
void restore_video_mode(void)
{
    uint16_t mode = VM_START.mode_found;

    if (mode != 0xff) {
        set_bios_video_mode(mode);
        VM_START.mode_found = 0xff;
    }
}

/*
 * 0x225d2
 *
 * Decide which video adapter is there, and answer its code. Hand-written
 * assembly: no frame, the answer in `AL`.
 *
 * DGROUP 0x48f3 is a forced setting, and it is the answer on most paths - the
 * BIOS is asked only to confirm it, not to override it. A zero at DGROUP 0x4344
 * refuses to look at all and answers 0.
 *
 * Of the eight paths only one runs on these screens: 0x48f3 is 0xd, which
 * sends it to the `INT 10h AH=1Ah` display-combination call, and a BL of 7 or
 * 8 there - a monochrome or colour VGA - accepts the forced setting unchanged.
 *
 * The rest are transcribed as stubs, each measured as unreached: the EGA
 * information call at AH=12h, and two probes that write 0x66 to a CRTC register
 * and read it back to tell a real card from an absent one. Those two are the
 * only places in this routine that touch hardware directly, and reproducing
 * them would mean modelling a card that is not there.
 */
uint16_t detect_adapter(void)
{
    uint8_t al = VM_START.mode_forced;

    if (DG4342.detect_allowed == 0)
        return 0;

    if (al == 0)
        goto ask_dcc;

    switch (al) {
    case 9:
        not_transcribed("0x22612, the adapter path for a forced 9");
        return 0;
    case 0xa: case 8: case 0xd: case 0xc: case 0xe: case 0xf:
        goto ask_dcc;
    case 5:
        not_transcribed("0x2264b, the second display-combination path");
        return 0;
    case 2: case 7: case 0xb:
        not_transcribed("0x22682, the EGA information path");
        return 0;
    default:
        not_transcribed("0x226ab, the CRTC probes");
        return 0;
    }

ask_dcc:
    {
        uint16_t bx = io_bios_display_combination();

        if ((bx & 0xff) == 7 || (bx & 0xff) == 8) {
            /* accepted; fall through */
        } else if ((bx >> 8) == 7 || (bx >> 8) == 8) {
            not_transcribed("0x2263a, a second display of 7 or 8");
            return 0;
        } else {
            not_transcribed("0x2264b, reached from the first DCC call");
            return 0;
        }
    }

    al = VM_START.mode_forced;
    if (al == 0)
        al = 8;

    return al;
}

/*
 * 0x22741
 *
 * **Back to text.** The two bits at 4 and 5 of the BIOS equipment word at
 * 0040:0010 are set from the argument - that is the "initial video mode" the
 * BIOS boots with - and INT 10h AX=0x0003 puts the adapter in mode 3.
 *
 * BX is loaded with 3 as well, which mode 3 has no use for. Transcribed as the
 * mode set it is.
 *
 * Near and cdecl: `ret` with the argument at [bp+4].
 */
void set_bios_video_mode(uint16_t equipment_bits)
{
    uint8_t eq = FAR8(0x40, 0x10);

    FAR8(0x40, 0x10) = (uint8_t)((eq & 0xcf)
                                 | (uint8_t)((equipment_bits << 4) & 0x30));

    io_bios_set_mode(3);
}

/*
 * 0x22764
 *
 * The BIOS video mode the machine booted in, as bits 4 and 5 of the equipment
 * word at 0040:0010 shifted down - so 0 to 3, of which 3 is monochrome.
 *
 * The port reads that byte out of guest memory. The BIOS data area is at
 * absolute 0x400 and is part of what the verifier seeds and compares, so this
 * needs nothing invented.
 */
uint16_t bios_video_kind(void)
{
    return (uint16_t)((*MK_FP(0x40, 0x10) & 0x30) >> 4);
}
