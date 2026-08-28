/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the `VGA:` chunk of `VM.OVL`, the video driver of The
 * Incredible Machine (Dynamix / Sierra On-Line, 1993). No licence is asserted:
 * this is derived from someone else's binary.
 *
 * `VM.OVL` is a container of eight per-adapter drivers - VGA, EGA, MCG, CGA,
 * TAN, HEG, EVG, EVA - each compressed. Only the **VGA** one is reconstructed;
 * the other seven are deliberate non-goals. The chunk expands to about 10 KB
 * and the game loads it into a block of 0x2b1 paragraphs, so it is a
 * translation unit of its own and gets a file of its own.
 *
 * **Addresses in this file are offsets within the loaded VGA driver**, written
 * `VM.OVL VGA:0xNNNN`, not image offsets - the loader chooses the segment, so
 * there is no fixed image address to quote. Dump it with
 * tools/dump_overlay.py and disassemble with
 * `tools/disasm.py --file out/res/VM_VGA.mem`.
 *
 * Every pixel the game draws is written by this driver: attributing the A000
 * writes of nine frames to the instructions that made them found 19
 * instructions, all of them here.
 */
#include "tim.h"
#include "io.h"

/*
 * The driver's own data segment, which it loads from `cs:[0x13a]`. These are
 * NOT DGROUP - the driver is a separate module with its own data - so they are
 * named by their offset within it.
 */
/*
 * Not `static`: tools/verify.py seeds these from the original's own driver
 * data segment before calling a routine, so that both sides are asked the
 * same question. A concession to testability, noted rather than left to be
 * discovered.
 */
uint16_t vga_page_back  = 0xA000;      /* VGA:DS 0x12, being drawn into */
uint16_t vga_page_front = 0xA820;      /* VGA:DS 0x14, on screen */
uint16_t vga_screen_height = 480;      /* VGA:DS 0x6ec, the mode's height */

/*
 * VM.OVL VGA:0x150f
 *
 * Make the page just drawn visible and swap the two pages over, then
 * optionally wait out a whole vertical retrace.
 *
 * The two page variables hold **segments**, 0xA000 and 0xA820; the start
 * address the CRTC wants is the segment shifted right by four, and only its
 * low byte is written, to index 0x0C. That is why the page offset is always a
 * multiple of 256 and why the game never writes index 0x0D.
 *
 * `vga_screen_height == 400` takes fifteen paragraphs off the start address. It is
 * never taken in the mode this game runs - the height here is 480, with
 * blanking moved up to 399 - and is transcribed rather than dropped because it
 * is in the original.
 */
void vm_show_page(uint16_t wait_retrace)
{
    uint16_t shown = vga_page_back;
    uint16_t other = vga_page_front;
    vga_page_front = vga_page_back;
    vga_page_back = other;

    uint16_t start = (uint16_t)(shown >> 4);
    if (vga_screen_height == 400)
        start = (uint16_t)(start - 0x0F);

    io_out16(bios_crtc_base(), (uint16_t)(0x0C | ((start & 0xFF) << 8)));

    if (wait_retrace) {
        while (io_in8(PORT_INPUT_ST1) & 0x08)
            ;
        while (!(io_in8(PORT_INPUT_ST1) & 0x08))
            ;
    }
}
