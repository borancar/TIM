/*
 * `ASB:`, the digitised-sound module out of `SX.OVL`, as it is actually loaded.
 *
 * Provenance is `SX.OVL ASB:0xNNNN` - offsets within the loaded module, not
 * image addresses, because the loader chooses the segment. The offsets are
 * **1.11's** chunk: 3,063 bytes in the container, 5,884 unpacked.
 *
 * **It is Sierra's `audblast` driver** - the chunk opens with the signature
 * 0x87654321, the name `audblast` and "CMS Sound Blaster", the same header as
 * the `AUDBLAST.DRV` files Sierra shipped - and 1.11's is a later revision
 * than 1.00's. Against 1.00's 2,414 bytes it adds the **stream**: functions 9
 * to 11 open a file (a Sierra SOL file, or raw bytes), function 1 refills one
 * half of a buffer while the card plays the other, and on a DSP 2.00 or later
 * the card runs on auto-init DMA. The one-shot sample of function 3 is 1.00's
 * with its state re-laid. Detection takes the base port it is given instead
 * of probing six, probes IRQs 2, 3, 5 and 7 only, and no longer chains
 * INT 9. docs/sound-driver.md has the comparison.
 *
 * **What the game reaches of it** is install, the service tick and
 * uninstall: the game's only callers are `setup_sound_device` (0),
 * `start_sound`'s timer callback (1) and `stop_sound` (2). Nothing starts a
 * sample or a stream, so the rest is transcribed because it is the module,
 * as 1.00's functions 3 to 13 were.
 *
 * **A module is not a driver.** `setup_sound_device` loads one of these *and*
 * one of the devices; the driver plays notes and this plays sampled bytes, by
 * handing the Sound Blaster's DMA channel a block of memory. Every call comes
 * through `call_sound_module` with the function number in AX and SI at the
 * caller's arguments.
 *
 * The module's own state lives in its code segment and is reached through
 * `ASBS` (dgroup.h) up to 0x85; what the module keeps in four-byte far
 * pointer slots above that - the chained vectors, the InDOS pointers, the
 * stream buffer - the host keeps on its own side, below, because a host
 * pointer does not fit a four-byte slot.
 *
 * Reconstructed from `SX.OVL` in The Even More Incredible Machine's
 * RESOURCE.003 (1.11).
 */
#ifndef __TURBOC__
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hostlib.h"
#endif
#include "dgroup.h"
#include "hostio.h"
#include "tim.h"

/*
 * The card's ports, as the module addresses them: an offset from the base at
 * `cs:[0x72]`. The module adds the offset two ways - `add dx, n` and, in
 * most places, `add dl, n`, which carries nothing into DH - and each call
 * says which it is.
 */
#define ASB_BASE    ((uint16_t)ASBS.base)
#define ASB_DX(n)   ((uint16_t)(ASB_BASE + (n)))
#define ASB_DL(n)   ((uint16_t)((ASB_BASE & 0xff00) | (uint8_t)(ASB_BASE + (n))))

/*
 * OURS: **the blocks the card plays**, as host pointers. The module turns a
 * far pointer into the DMA controller's page and offset (ASB:0x0c07) and
 * keeps those; a host pointer has no page, so the host's controller is given
 * the block itself (`io_dma1_memory`). Half 0 and half 1 are a sample's two
 * sides of a 64K page - which on the host it never has, so the second stays
 * null - or the stream buffer's two halves.
 */
static uint8_t *asb_block[2];
/* OURS: function 9's buffer, which the module keeps at `cs:[0x5c]`. */
static uint8_t *asb_buffer;
/* OURS: the vectors the module chains, which it keeps at `cs:[0x92]`,
   `cs:[0x96]` and `cs:[0x9a]`, and the two DOS flags' addresses at
   `cs:[0x8e]` and `cs:[0x8a]`. */
static void interrupt (far *g_asb_old_int0d)();
static void interrupt (far *g_asb_old_int74)();
static void interrupt (far *g_asb_old_int10)();
static uint8_t far *g_asb_indos;
static uint8_t far *g_asb_criterr;
/* OURS: the vector `asb_hook_irq` displaces for each IRQ, which the module
   keeps in the four bytes its caller names. */
static void interrupt (far *g_asb_saved_vector[16])();


/*
 * SX.OVL ASB:0x08bf
 *
 * **The module's one entry point.** AX picks one of the sixteen near
 * offsets in the table at `cs:0x89f` and SI points at the caller's
 * arguments; the answer comes back in AX.
 *
 * Entries 7 and 15 are bare `ret`s, so they answer whatever AX held - the
 * module's own segment, which the dispatcher has just loaded into DS
 * through AX. The host has no such number and nothing calls them; they
 * answer 0. Entry 14 is `xor ax,ax; ret`.
 */
uint16_t asb_dispatch(uint16_t fn, union sound_module_args * si)
{
    switch (fn) {
    case 0:  return asb_install(si != 0 ? &si->install : 0);
    case 1:  return asb_service();
    case 2:  return asb_uninstall();
    case 3:  asb_play(&si->play); return 0;
    case 4:  return asb_status(&si->poll);
    case 5:  return asb_stop();
    case 6:  return asb_set_rate_fn(&si->rate);
    case 8:  return asb_clear_hold();
    case 9:  return asb_open_stream(&si->stream);
    case 10: return asb_prime();
    case 11: return asb_start_stream();
    case 12: return asb_shutdown();
    case 13: return asb_position(&si->position);
    case 14: return asb_fn14();

    case 7: case 15:
        return 0;
    }

    return 0;
}

/*
 * SX.OVL ASB:0x08d1  - function 1
 *
 * **The service tick**, which `start_sound` registers as a timer callback at
 * rate 2. With a stream running and `hold` clear it counts a tick and lets
 * `asb_swap_halves` start the next half if one is due; either way it then
 * tops up whichever half is free. Stopped, or playing a plain sample, it does
 * nothing - which is all it ever does in this game.
 */
uint16_t asb_service(void)
{
    if (ASBS.stopped != 1 && ASBS.streaming == 1) {
        if (ASBS.hold == 0) {
            ASBS.ticks++;
            asb_swap_halves();
        }
        asb_fill();
    }
    return 0;
}

/*
 * SX.OVL ASB:0x08f8  - function 6
 */
uint16_t asb_set_rate_fn(const struct sound_rate_args *si)
{
    ASBS.rate = si->rate;
    asb_set_rate(si->rate);
    return 0;
}

/*
 * SX.OVL ASB:0x0906  - function 8
 */
uint16_t asb_clear_hold(void)
{
    ASBS.hold = 0;
    return 0;
}

/*
 * SX.OVL ASB:0x090f  - function 12
 *
 * Stop the card and give the interrupt back. `stopped` is the guard: once it
 * is 1 this does nothing, which is what lets the interrupt handler stop the
 * last block and the game stop it again afterwards.
 */
uint16_t asb_shutdown(void)
{
    if (ASBS.stopped == 1)
        return 0;

    ASBS.stopped = 1;
    asb_dma_pause();
    asb_dma_stop();
    asb_unhook_irq(ASBS.irq, 0x86);
    return 0;
}

/*
 * SX.OVL ASB:0x0933  - function 3
 *
 * Play a sample. The argument block is the loop flag in the high byte of its
 * first word, the rate, the sample's far pointer and its length.
 *
 * The length is added to the sample's offset in its 64K DMA page, and a carry
 * means it crosses the page: it is cut in two, the first half running to the
 * end of the page and the second from offset 0 of the next, which
 * `asb_isr` starts when the first finishes. A host pointer has no page, so on
 * the host the carry never happens and there is one half (see `asb_block`).
 *
 * Then the IRQ is hooked, the flags cleared, and the first block goes.
 */
void asb_play(const struct sound_play_args *si)
{
    asb_shutdown();

    if (si->loop != 0)
        ASBS.looping = 1;
    else
        ASBS.looping = 0;

    asb_set_rate(si->rate);

    asb_block[0] = (uint8_t *)si->sample;
    asb_block[1] = 0;
    ASBS.length_a = si->length;
    ASBS.length_b = 0;

    asb_hook_irq(ASBS.irq, 0x86, 0x0b22);
    ASBS.play_half = 0;
    ASBS.streaming = 0;
    ASBS.first_block = 1;

    asb_arm_block();

    ASBS.ticks = 0;
    ASBS.stopped = 0;
    ASBS.looped = 0;
}

/*
 * SX.OVL ASB:0x09cf  - function 4
 *
 * The caller's loop flag again - 1.11 takes it here as well as at the start -
 * and then how the sample is getting on: AL is `stopped` and AH is `looped`,
 * which reading clears.
 */
uint16_t asb_status(const struct sound_poll_args *si)
{
    uint16_t r;

    if (si->loop != 0)
        ASBS.looping = 1;
    else
        ASBS.looping = 0;

    r = (uint16_t)(ASBS.stopped | (ASBS.looped << 8));
    ASBS.looped = 0;
    return r;
}

/*
 * SX.OVL ASB:0x09f4  - function 5
 */
uint16_t asb_stop(void)
{
    return asb_shutdown();
}

/*
 * SX.OVL ASB:0x09f8  - function 2
 *
 * Take the module out: stop, put the three chained vectors back - 1.11
 * chains INT 9 no more - and close the stream's file if one is open.
 */
uint16_t asb_uninstall(void)
{
    asb_shutdown();

    setvect(0x10, g_asb_old_int10);
    setvect(0x0d, g_asb_old_int0d);
    setvect(0x74, g_asb_old_int74);

    if (ASBS.file_handle != 0xffff) {
        close((int16_t)ASBS.file_handle);
        ASBS.file_handle = 0xffff;
    }

    return 0;
}

/*
 * SX.OVL ASB:0x0a3d
 *
 * Choose the block to play next and start it: `play_half` 0 takes page_a,
 * offset_a and length_a, 1 the other three. Under auto-init the channel is
 * programmed once, for the whole buffer, so after the first block this only
 * takes note of which half is current.
 */
void asb_arm_block(void)
{
    if (ASBS.play_half == 0) {
        ASBS.length = ASBS.length_a;
        ASBS.page = ASBS.page_a;
        ASBS.offset = ASBS.offset_a;
    } else {
        ASBS.length = ASBS.length_b;
        ASBS.page = ASBS.page_b;
        ASBS.offset = ASBS.offset_b;
    }

    if (ASBS.streaming == 1 && ASBS.autoinit == 1) {
        if (ASBS.first_block == 0)
            return;
        ASBS.first_block = 0;
    }

    asb_dma_start();
}

/*
 * SX.OVL ASB:0x0a95
 *
 * Hand the card the current block: channel 1 masked, the flip-flop cleared,
 * the address, mode 0x49 - single transfer, read, channel 1 - and the count
 * less one, unmasked; then DSP 0x14 with the length less one.
 *
 * **Under auto-init** the mode is 0x59 and the count covers both halves, and
 * the DSP is told 0x1C alone - the block size went to it as DSP 0x48 when
 * the stream was opened - so it raises its line at every half and runs on.
 *
 * The address and page go to the host's controller as the block itself
 * (`asb_block`); the order of the writes is the module's.
 */
void asb_dma_start(void)
{
    uint16_t cx, dx;
    uint8_t  al;

    io_out8(0x0a, 5);
    io_out8(0x0c, 0);
    io_dma1_memory(asb_block[ASBS.play_half == 0 ? 0 : 1]);

    al = 0x49;
    cx = ASBS.length;
    if (ASBS.streaming == 1 && ASBS.autoinit == 1) {
        al = 0x59;
        cx = (uint16_t)(cx << 1);
    }
    ASBS.dma_count = cx;
    io_out8(0x0b, al);
    io_out8(0x03, (uint8_t)(cx - 1));
    io_out8(0x03, (uint8_t)((uint16_t)(cx - 1) >> 8));
    io_out8(0x0a, 1);

    dx = ASB_DL(0x0c);
    if (ASBS.streaming == 1 && ASBS.autoinit == 1) {
        asb_dsp_write(dx, 0x1c);
        return;
    }

    asb_dsp_write(dx, 0x14);
    cx = (uint16_t)(ASBS.length - 1);
    asb_dsp_write(dx, (uint8_t)cx);
    asb_dsp_write(dx, (uint8_t)(cx >> 8));
}

/*
 * SX.OVL ASB:0x0b14
 *
 * Mask DMA channel 1 and acknowledge the card by reading its status port.
 */
void asb_dma_stop(void)
{
    io_out8(0x0a, 5);
    (void)io_in8(ASB_DL(0x0e));
}

/*
 * SX.OVL ASB:0x0b22
 *
 * **The card's interrupt handler**: a block has finished.
 *
 * A stream adds what was played to the position - the whole buffer on every
 * second interrupt under auto-init, the block on every one otherwise, after
 * which the channel is stopped - and asks for the next half, unless the
 * stream has stopped.
 *
 * A sample starts its second half if it has one (`play_half` flips, and the
 * `xor` leaves the zero flag set when it flipped back), starts again if it
 * loops, raising `looped`, and otherwise stops.
 *
 * The EOI goes to the master PIC only: 1.11 probes no IRQ above 7.
 */
void asb_isr(void)
{
    if (ASBS.streaming == 1) {
        if (ASBS.autoinit == 1) {
            if ((ASBS.autoinit_phase ^= 1) == 0) {
                uint32_t pos = ((uint32_t)ASBS.pos_hi << 16) | ASBS.pos_lo;

                pos += (uint16_t)(ASBS.length << 1);
                ASBS.pos_lo = (uint16_t)pos;
                ASBS.pos_hi = (uint16_t)(pos >> 16);
            }
        } else {
            uint32_t pos = ((uint32_t)ASBS.pos_hi << 16) | ASBS.pos_lo;

            pos += ASBS.length;
            ASBS.pos_lo = (uint16_t)pos;
            ASBS.pos_hi = (uint16_t)(pos >> 16);
            asb_dma_stop();
        }

        if (ASBS.stopped != 1)
            asb_refill_now();
    } else if (ASBS.length_b != 0 && (ASBS.play_half ^= 1) != 0) {
        asb_arm_block();
    } else if (ASBS.looping == 1) {
        asb_arm_block();
        ASBS.looped = 1;
    } else {
        asb_stop();
    }

    (void)io_in8(ASB_DL(0x0e));
    io_out8(0x20, 0x20);
}

/*
 * SX.OVL ASB:0x0bbd
 *
 * Set the sampling rate: the time constant 256 - 1,000,000/rate, as a 32-bit
 * divide of 0x000f4240 by the rate and a `neg` of the quotient's low byte,
 * sent as DSP 0x40 and the byte. The rate written is kept in `dsp_rate`.
 */
void asb_set_rate(uint16_t rate)
{
    uint8_t  cl;
    uint16_t dx;

    ASBS.dsp_rate = rate;
    cl = (uint8_t)(uint16_t)(0x000f4240UL / rate);
    cl = (uint8_t)(0 - cl);

    dx = ASB_DL(0x0c);
    asb_dsp_write(dx, 0x40);
    asb_dsp_write(dx, cl);
}

/*
 * SX.OVL ASB:0x0be4
 *
 * DSP 0x48, the block size auto-init interrupts at, low byte first.
 */
void asb_set_block_size(uint16_t n)
{
    uint16_t dx = ASB_DL(0x0c);

    asb_dsp_write(dx, 0x48);
    asb_dsp_write(dx, (uint8_t)n);
    asb_dsp_write(dx, (uint8_t)(n >> 8));
}

/*
 * SX.OVL ASB:0x0c1b
 *
 * Pause the DMA transfer, DSP 0xD0, and wait for the DSP to be ready again.
 * (ASB:0x0c07, before it, turns a segment and offset into the DMA page and
 * offset, which a host pointer has no use for - see `asb_block`.)
 */
void asb_dma_pause(void)
{
    uint16_t dx = ASB_DL(0x0c);

    asb_dsp_write(dx, 0xd0);
    while ((io_in8(dx) & 0x80) != 0)
        ;
}

/*
 * SX.OVL ASB:0x0c2f
 *
 * Continue it, DSP 0xD4, the same way.
 */
void asb_dma_continue(void)
{
    uint16_t dx = ASB_DL(0x0c);

    asb_dsp_write(dx, 0xd4);
    while ((io_in8(dx) & 0x80) != 0)
        ;
}

/*
 * SX.OVL ASB:0x0c43
 *
 * Write one byte to the DSP at DX: spin until bit 7 clears, then out. No
 * timeout - the card is known to be there by the time anything calls this.
 */
void asb_dsp_write(uint16_t dx, uint8_t value)
{
    while ((io_in8(dx) & 0x80) != 0)
        ;
    io_out8(dx, value);
}

/*
 * OURS: which routine a hooked vector names. `asb_hook_irq` is handed the
 * near offset of a handler inside the module, and on the original that offset
 * *is* the handler; here the offsets are mapped back to the C functions. All
 * five are read off the module - the four probe handlers and the real one.
 */
static void (*asb_handler_for(uint16_t off))(void)
{
    switch (off) {
    case 0x0b22: return asb_isr;
    case 0x16bc: return asb_probe_isr_2;
    case 0x16c5: return asb_probe_isr_3;
    case 0x16ce: return asb_probe_isr_5;
    case 0x16d7: return asb_probe_isr_7;
    }
    return 0;
}

/*
 * SX.OVL ASB:0x0c50
 *
 * Hook an IRQ - 1.11 takes only the master PIC's, so the vector is IRQ+8 and
 * the mask port 0x21 - with the old vector kept where the caller says and the
 * new one at DX. The line is unmasked and the mask as it was goes to
 * `pic_mask_was`, one byte for every hook.
 *
 * **The port does not run guest interrupt vectors**: the vector write is
 * kept for the state it leaves, and the handler is registered with the
 * hardware by `io_on_sb_irq`. The old vector goes into
 * `g_asb_saved_vector`, not the module's four bytes at `save_at`, which a
 * host's handler is wider than.
 */
void asb_hook_irq(uint8_t irq, uint16_t save_at, uint16_t handler)
{
    /* `add al,8` and the vector's address worked out in AL: IRQ+8 for
       the four IRQs it is given. */
    uint16_t vec = (uint16_t)(irq + 8);
    uint8_t  was;

    (void)save_at;
    g_asb_saved_vector[irq & 0xf] = getvect(vec);
    setvect(vec, (void interrupt (far *)())asb_handler_for(handler));

    was = io_in8(0x21);
    ASBS.pic_mask_was = was;
    io_out8(0x21, (uint8_t)(was & (uint8_t)~(1u << irq)));

    io_on_sb_irq(irq, asb_handler_for(handler));
}

/*
 * SX.OVL ASB:0x0c8e
 *
 * Put a hooked vector back and the mask byte the last hook found.
 */
void asb_unhook_irq(uint8_t irq, uint16_t save_at)
{
    uint16_t vec = (uint16_t)(irq + 8);

    (void)save_at;
    setvect(vec, g_asb_saved_vector[irq & 0xf]);
    io_out8(0x21, ASBS.pic_mask_was);

    io_on_sb_irq(irq, 0);
}

/*
 * SX.OVL ASB:0x0cb2  - function 14
 */
uint16_t asb_fn14(void)
{
    return 0;
}

/*
 * SX.OVL ASB:0x0e31
 *
 * Take a stream's format byte apart into its six flags, and start the DPCM
 * value at 0x80, the middle.
 */
void asb_set_format(uint8_t al)
{
    ASBS.dpcm_value = 0x80;
    ASBS.fmt_dpcm  = (uint8_t)((al & 0x01) ? 1 : 0);
    ASBS.fmt_bit1  = (uint8_t)((al & 0x02) ? 1 : 0);
    ASBS.fmt_16bit = (uint8_t)((al & 0x04) ? 1 : 0);
    ASBS.fmt_bit3  = (uint8_t)((al & 0x08) ? 1 : 0);
    ASBS.fmt_bit4  = (uint8_t)((al & 0x10) ? 1 : 0);
    ASBS.fmt_bit7  = (uint8_t)((al & 0x80) ? 1 : 0);
}

/*
 * SX.OVL ASB:0x0ea4
 *
 * **A Sierra SOL header**, if the file has one. Two bytes are read into the
 * buffer: 0x8D and the header's length. That many more must begin
 * `SOL\0`, and then carry the rate (a word), the format byte and the data's
 * length (a doubleword) - which replace the caller's - and the data's start
 * moves past the header. Anything else is raw data, and the file goes back to
 * where it started.
 *
 * Neither read's answer is looked at.
 */
void asb_read_header(void)
{
    int16_t bx = (int16_t)ASBS.file_handle;
    uint16_t cx = 2;
    const uint8_t *si = asb_buffer;
    uint16_t rate, lo, hi;
    uint8_t  flags;
    uint32_t start;

    (void)read(bx, asb_buffer, cx);
    if (*si++ == 0x8d) {
        cx = *si;
        si = asb_buffer;
        (void)read(bx, asb_buffer, cx);
        if (si[0] == 'S' && si[1] == 'O' && si[2] == 'L' && si[3] == 0) {
            start = ((uint32_t)ASBS.start_hi << 16) | ASBS.start_lo;
            start += (uint16_t)(cx + 2);
            ASBS.start_lo = (uint16_t)start;
            ASBS.start_hi = (uint16_t)(start >> 16);

            rate  = (uint16_t)(si[4] | (si[5] << 8));
            flags = si[6];
            lo    = (uint16_t)(si[7] | (si[8] << 8));
            hi    = (uint16_t)(si[9] | (si[10] << 8));

            ASBS.left_lo = lo;
            ASBS.left_hi = hi;
            asb_set_format(flags);
            ASBS.rate = rate;
            asb_set_rate(rate);
            return;
        }
    }

    (void)lseek(bx, (off_t)(((uint32_t)ASBS.start_hi << 16) | ASBS.start_lo),
                SEEK_SET);
}

/*
 * SX.OVL ASB:0x0cb5  - function 9
 *
 * **Open a stream.** The argument block: the file (see `sound_stream_args`),
 * where in it the data begins, its length, the buffer to play it through and
 * that buffer's size, the loop flag and the format byte.
 *
 * The buffer is two halves. On a DSP 2.00 or later the stream will run on
 * auto-init DMA and the buffer starts as silence (0x80). The file is opened -
 * or the handle given, or the one already open, is used - and positioned,
 * and `asb_read_header` takes a SOL header if there is one.
 *
 * Answers **how long it will play, in sixtieths of a second** - the length
 * times 60 over the rate, the length counted twice for DPCM - or 0xfffe if
 * that overflows, or 0 if the file would not open or seek. Under auto-init
 * the half size, or the whole length if it is shorter, goes to the DSP as
 * the block size.
 */
uint16_t asb_open_stream(const struct sound_stream_args *si)
{
    uint16_t ax, bx;
    uint32_t len;
    int16_t  fd;

    asb_shutdown();

    ASBS.loop_file = (uint8_t)(si->loop != 0 ? 1 : 0);
    asb_set_format(si->format);

    ASBS.left_lo = (uint16_t)si->length;
    ASBS.left_hi = (uint16_t)(si->length >> 16);

    asb_buffer = (uint8_t *)si->buffer;
    /* `buffer_off`, `buffer_seg`, the pages and offset_a are the far
       pointer and its DMA address, which the host has as `asb_buffer`. */

    ax = (uint16_t)(si->buffer_size >> 1);
    ASBS.half_size = ax;

    if (ASBS.dsp_v2 == 1) {
        ASBS.autoinit = 1;
        ASBS.autoinit_phase = 0;
        memset(asb_buffer, 0x80, (uint16_t)(ASBS.half_size << 1));
    } else {
        ASBS.autoinit = 0;
    }

    asb_block[0] = asb_buffer;
    asb_block[1] = asb_buffer + ax;

    if (si->name == 0 && si->by_handle == 0) {
        fd = (int16_t)ASBS.file_handle;
    } else if (si->by_handle != 0) {
        fd = si->handle;
        ASBS.file_handle = (uint16_t)fd;
    } else {
        if (ASBS.file_handle != 0xffff) {
            close((int16_t)ASBS.file_handle);
            ASBS.file_handle = 0xffff;
        }
        fd = (int16_t)open(si->name, O_RDONLY);
        if (fd < 0)
            return 0;
        ASBS.file_handle = (uint16_t)fd;
    }

    ASBS.start_lo = (uint16_t)si->start;
    ASBS.start_hi = (uint16_t)(si->start >> 16);
    if (lseek(fd, (off_t)si->start, SEEK_SET) < 0)
        return 0;

    asb_read_header();
    ASBS.primed = 0;

    ASBS.total_lo = ASBS.left_lo;
    ASBS.total_hi = ASBS.left_hi;

    len = ((uint32_t)ASBS.left_hi << 16) | ASBS.left_lo;
    if (ASBS.fmt_dpcm == 1)
        len <<= 1;

    /* `mul cx` twice, the high word's product added to DX: the length times
       60, kept to 32 bits. */
    len *= 0x3c;
    if ((uint16_t)(len >> 16) < ASBS.dsp_rate)
        ax = (uint16_t)(len / ASBS.dsp_rate);
    else
        ax = 0xfffe;

    if (ASBS.autoinit == 1) {
        bx = ASBS.half_size;
        if (ASBS.left_hi == 0 && bx > ASBS.left_lo)
            bx = ASBS.left_lo;
        asb_set_block_size(bx);
    }

    return ax;
}

/*
 * SX.OVL ASB:0x0f2b  - function 10
 *
 * Fill both halves ahead of function 11, so it can start at once.
 */
uint16_t asb_prime(void)
{
    ASBS.primed = 1;
    ASBS.play_half = 0xff;
    ASBS.fill_half = 0;
    asb_fill();
    asb_fill();
    return 0;
}

/*
 * SX.OVL ASB:0x0f46  - function 11
 *
 * **Start the stream**: hook the IRQ, set the rate if it has changed, fill
 * both halves unless function 10 already did, and start the first.
 */
uint16_t asb_start_stream(void)
{
    uint8_t al;

    asb_hook_irq(ASBS.irq, 0x86, 0x0b22);

    if (ASBS.rate != ASBS.dsp_rate)
        asb_set_rate(ASBS.rate);

    ASBS.streaming = 1;
    ASBS.refill_due = 1;

    if (ASBS.primed != 1) {
        ASBS.play_half = 0xff;
        ASBS.fill_half = 0;
        asb_fill();
        asb_fill();
    } else {
        ASBS.primed = 0;
    }

    ASBS.play_half = 1;
    ASBS.pos_lo = 0;
    ASBS.pos_hi = 0;
    ASBS.first_block = 1;
    ASBS.dsp_paused = 0;

    al = ASBS.fill_half;
    ASBS.fill_half = 0xff;
    asb_swap_halves();
    ASBS.fill_half = al;

    ASBS.hold = 0;
    ASBS.ticks = 0;
    ASBS.stopped = 0;
    return 0;
}

/*
 * SX.OVL ASB:0x0fd9
 *
 * **Fill the free half** from the file, converting as the format says.
 *
 * Nothing happens if a fill is already running, the data is used up, the
 * half wanted is the one playing, or DOS or the BIOS is busy
 * (`asb_safe_to_call`). Otherwise, with interrupts on:
 *
 * - Sixteen-bit data is read through the module's own 0x800-byte buffer at
 *   0x9f and only each high byte kept - 0x7f minus it with format bit 3 -
 *   a buffer's worth at a time.
 * - Format bit 1 rotates each byte left through AX, as the loop carries it.
 * - DPCM is read into the top of the half and expanded downward into it,
 *   two samples a byte, each nibble a step up or down the table.
 * - A looping stream that runs out seeks back to its start and goes on
 *   filling the same half.
 *
 * What was put in becomes that half's length, and the next half is the
 * other.
 */
void asb_fill(void)
{
    uint16_t cx, dx, di, ax, si, n;
    int16_t  bx;
    uint8_t *dst, *src;
    uint8_t  ah, dl, b;
    uint16_t ax16;

    if (ASBS.filling == 1)
        return;
    if ((ASBS.left_lo | ASBS.left_hi) == 0)
        return;
    if (ASBS.fill_half == ASBS.play_half)
        return;
    if (asb_safe_to_call() != 0)
        return;

    ASBS.filling = 1;

    ASBS.filled = 0;
    cx = ASBS.half_size;
    dx = (uint16_t)(ASBS.fill_half == 0 ? 0 : cx);

    for (;;) {
        if (ASBS.fmt_16bit == 1)
            cx = (uint16_t)(cx << 1);
        if (ASBS.fmt_dpcm == 1)
            cx = (uint16_t)(cx >> 1);

        if (ASBS.left_hi == 0 && cx > ASBS.left_lo)
            cx = ASBS.left_lo;
        {
            uint32_t left = ((uint32_t)ASBS.left_hi << 16) | ASBS.left_lo;

            left -= cx;
            ASBS.left_lo = (uint16_t)left;
            ASBS.left_hi = (uint16_t)(left >> 16);
        }

        di = dx;
        if (ASBS.fmt_dpcm == 1)
            dx = (uint16_t)(dx + cx);

        for (;;) {
            bx = (int16_t)ASBS.file_handle;

            if (ASBS.fmt_16bit == 1) {
                /* Into the module's own buffer at `cs:0x9f`, 0x800 at a
                   time; DX keeps 0x9f, and so does SI below. */
                src = g_sound_bank.module;
                dx = 0x9f;
                if (cx > 0x800) {
                    ASBS.left_16bit = (uint16_t)(cx - 0x800);
                    cx = 0x800;
                } else {
                    ASBS.left_16bit = 0;
                }
            } else {
                src = asb_buffer;
            }
            si = dx;

            {
                ssize_t got = read(bx, src + si, cx);

                ax = (uint16_t)(got < 0 ? cx : (uint16_t)got);
            }

            if (ASBS.fmt_16bit == 1) {
                ax = (uint16_t)(ax >> 1);
                dst = asb_buffer + di;
                for (n = 0; n < ax; n++) {
                    ah = src[si + n * 2 + 1];
                    *dst++ = (uint8_t)(ASBS.fmt_bit3 == 1 ? 0x7f - ah : ah);
                }
            }

            if (ASBS.fmt_bit1 == 1) {
                /* `lodsb / rol ax,1 / stosb`, AX starting as the count and
                   carrying the rotated bit from byte to byte. */
                ax16 = ax;
                for (n = 0; n < ax; n++) {
                    ax16 = (uint16_t)((ax16 & 0xff00) | asb_buffer[(uint16_t)(si + n)]);
                    ax16 = (uint16_t)((ax16 << 1) | (ax16 >> 15));
                    asb_buffer[(uint16_t)(di + n)] = (uint8_t)ax16;
                }
            }

            if (ASBS.fmt_dpcm == 1) {
                ah = ASBS.dpcm_value;
                src = asb_buffer + si;
                dst = asb_buffer + di;
                for (n = 0; n < ax; n++) {
                    dl = *src++;
                    b = (uint8_t)(dl >> 4);
                    if ((int8_t)b >= 8)
                        ah = (uint8_t)(ah - ASBS.dpcm_step[0x0f - b]);
                    else
                        ah = (uint8_t)(ah + ASBS.dpcm_step[b]);
                    *dst++ = ah;
                    b = (uint8_t)(dl & 0x0f);
                    if ((int8_t)b >= 8)
                        ah = (uint8_t)(ah - ASBS.dpcm_step[0x0f - b]);
                    else
                        ah = (uint8_t)(ah + ASBS.dpcm_step[b]);
                    *dst++ = ah;
                }
                ASBS.dpcm_value = ah;
                ax = (uint16_t)(ax << 1);
            }

            ASBS.filled = (uint16_t)(ASBS.filled + ax);

            if (ASBS.fmt_16bit == 1 && ASBS.left_16bit != 0) {
                di = (uint16_t)(di + ax);
                cx = ASBS.left_16bit;
                continue;
            }
            break;
        }

        if (ASBS.loop_file == 1 && (ASBS.left_lo | ASBS.left_hi) == 0) {
            ASBS.pos_lo = 0;
            ASBS.pos_hi = 0;
            ASBS.ticks = 0;

            (void)lseek(bx, (off_t)(((uint32_t)ASBS.start_hi << 16)
                                    | ASBS.start_lo), SEEK_SET);

            ASBS.left_lo = ASBS.total_lo;
            ASBS.left_hi = ASBS.total_hi;

            if (ASBS.fmt_dpcm == 1) {
                ASBS.dpcm_value = 0x80;
                ax = (uint16_t)(ax >> 1);
            }

            cx = (uint16_t)(ASBS.half_size - ASBS.filled);
            if (cx != 0) {
                dx = (uint16_t)(dx + ax);
                continue;
            }
        }
        break;
    }

    if (ASBS.fill_half == 0)
        ASBS.length_a = ASBS.filled;
    else
        ASBS.length_b = ASBS.filled;

    ASBS.fill_half ^= 1;
    ASBS.filling = 0;
}

/*
 * SX.OVL ASB:0x120b
 *
 * Make the half that is to be filled next silence, 0x80 throughout.
 */
void asb_clear_half(void)
{
    uint16_t di;

    if (ASBS.fill_half == 0)
        di = 0;
    else if (ASBS.fill_half == 1)
        di = ASBS.half_size;
    else
        return;

    memset(asb_buffer + di, 0x80, ASBS.half_size);
}

/*
 * SX.OVL ASB:0x1238
 *
 * The interrupt handler's way in: a half is done, so one is due.
 */
void asb_refill_now(void)
{
    ASBS.refill_due = 1;
    asb_swap_halves();
}

/*
 * SX.OVL ASB:0x1242
 *
 * **Start the next half**, if one is due and nothing else is doing this.
 *
 * If the half that would play next is the one still waiting to be filled,
 * the stream has caught up with the file: at the end of the data it stops,
 * and under auto-init the DSP is paused (0xD0) until the fill arrives.
 * Otherwise a paused DSP is continued (0xD4), the halves change over, the
 * new one starts, and the one just played becomes silence to fill.
 */
void asb_swap_halves(void)
{
    if (ASBS.swapping == 1)
        return;
    if (ASBS.refill_due != 1)
        return;

    ASBS.swapping = 1;

    if ((uint8_t)(ASBS.play_half ^ 1) == ASBS.fill_half) {
        if ((ASBS.left_lo | ASBS.left_hi) == 0) {
            asb_shutdown();
        } else if (ASBS.autoinit == 1 && ASBS.dsp_paused == 0) {
            ASBS.dsp_paused = 1;
            asb_dma_pause();
        }
    } else {
        if (ASBS.dsp_paused == 1) {
            asb_dma_continue();
            ASBS.dsp_paused = 0;
        }
        ASBS.refill_due = 0;
        ASBS.play_half ^= 1;
        asb_arm_block();
        asb_clear_half();
    }

    ASBS.swapping = 0;
}

/*
 * SX.OVL ASB:0x12bc  - function 13
 *
 * Where it has got to, as three words in the caller's block: the tick count
 * and a 32-bit position. The position is what the DMA controller still has
 * to do - its current count at port 3, low byte then high - taken from the
 * count it was given and added to the position at the last interrupt;
 * doubled for sixteen-bit data.
 *
 * Past the data's end, or stopped, the answer is all ones; primed and not
 * yet started, all zeroes. A looping stream is never past its end. For DPCM
 * the end test is made at half the position, the bytes the file holds.
 */
uint16_t asb_position(struct sound_position_args *si)
{
    uint16_t cx, dx;
    uint32_t pos, total;

    if (ASBS.primed == 1) {
        si->id = 0;
        si->position = 0;
        return 0;
    }

    if (ASBS.stopped == 1) {
        si->id = 0xffff;
        si->position = 0xffffffffu;
        return 0;
    }

    cx  = io_in8(0x03);
    cx |= (uint16_t)(io_in8(0x03) << 8);
    dx = ASBS.dma_count;
    pos = ((uint32_t)ASBS.pos_hi << 16) | ASBS.pos_lo;

    dx = (uint16_t)(dx - cx);
    pos += dx;

    if (ASBS.fmt_16bit == 1)
        pos <<= 1;

    if (ASBS.loop_file != 1) {
        if (ASBS.fmt_dpcm == 1)
            pos >>= 1;

        total = ((uint32_t)ASBS.total_hi << 16) | ASBS.total_lo;
        if (pos > total) {
            si->id = 0xffff;
            si->position = 0xffffffffu;
            return 0;
        }

        if (ASBS.fmt_dpcm == 1)
            pos <<= 1;
    }

    si->position = pos;
    si->id = ASBS.ticks;
    return 0;
}

/*
 * SX.OVL ASB:0x138d
 *
 * Is it safe to go near DOS and the BIOS? The `or` of the critical-error and
 * InDOS bytes and the three busy flags; zero means nothing is in progress.
 * `asb_fill` asks before it reads the file.
 */
uint8_t asb_safe_to_call(void)
{
    uint8_t al;

    al  = *ZERO_PAGE(g_asb_criterr);
    al |= *ZERO_PAGE(g_asb_indos);
    al |= ASBS.busy_int10;
    al |= ASBS.busy_int0d;
    al |= ASBS.busy_int74;

    return al;
}

/*
 * SX.OVL ASB:0x13ad
 *
 * The three vector hooks: each raises a byte, chains to the handler it
 * replaced, and lowers it again, which is what `asb_safe_to_call` reads.
 * **The port has no vectors to chain to**, so these are the flags only.
 */
void asb_int10_hook(void) { ASBS.busy_int10 = 1; ASBS.busy_int10 = 0; }
/* SX.OVL ASB:0x13c0 - the same, for the DOS critical-error vector. */
void asb_int0d_hook(void) { ASBS.busy_int0d = 1; ASBS.busy_int0d = 0; }
/* SX.OVL ASB:0x13d3 - the same, for the mouse's. */
void asb_int74_hook(void) { ASBS.busy_int74 = 1; ASBS.busy_int74 = 0; }

/*
 * SX.OVL ASB:0x13e6  - function 0
 *
 * **Install**: the base port is the first word of the arguments, 0x220 if it
 * is zero - the game's is its callback, which is zero - and the card must
 * answer there. Then chain INT 10h, 0Dh and 74h, take the InDOS flag's
 * address and the byte below it, and set the default rate of 11025.
 *
 * The CD-ROM check before the hooks - MSCDEX's drive check, INT 2Fh
 * AX=150Bh, on the current drive - runs only if the module's last byte, at
 * 0x16fb, is 0xCD; in this build it is 0xDC, so the branch is not taken.
 *
 * Answers 0x13e6 - its own address, so non-zero - when the card is there and
 * 0 when it is not.
 */
uint16_t asb_install(const struct sound_install_args *si)
{
    ASBS.base = (si != 0 && si->base != 0) ? si->base : 0x220;

    if (asb_detect() != 0)
        return 0;

    ASBS.stopped = 1;

    if (ASB8(0x16fb) == 0xcd) {
        /* NOT TRANSCRIBED YET: INT 2Fh AX=150Bh, which the port cannot
           answer - unreachable while the byte is 0xDC, as shipped. */
        fprintf(stderr, "asb_install: the MSCDEX check (SX.OVL ASB:0x1415)"
                        " is not transcribed\n");
        abort();
    }

    g_asb_old_int10 = getvect(0x10);
    setvect(0x10, (void interrupt (far *)())asb_int10_hook);

    g_asb_old_int0d = getvect(0x0d);
    setvect(0x0d, (void interrupt (far *)())asb_int0d_hook);

    g_asb_old_int74 = getvect(0x74);
    setvect(0x74, (void interrupt (far *)())asb_int74_hook);

    /*
     * INT 21h AH=34h, the address of the InDOS flag, and the byte below it.
     * **The port has no DOS to be inside**, so both point at a byte of low
     * memory that stays zero and `asb_safe_to_call` always says it is safe -
     * which is the truth here rather than a shortcut.
     */
    g_asb_indos = ZERO_PAGE((uint8_t far *)NULL) + 1;
    g_asb_criterr = g_asb_indos - 1;

    ASBS.rate = 0x2b11;          /* 11025 Hz */
    asb_set_rate(0x2b11);

    ASBS.filling = 0;
    ASBS.swapping = 0;
    ASBS.file_handle = 0xffff;

    return 0x13e6;
}

/*
 * SX.OVL ASB:0x14cc
 *
 * Is a card at the base: reset, identify, version and IRQ, then the speaker
 * on. Answers the first step's failure, or 0.
 */
uint16_t asb_detect(void)
{
    uint16_t r;

    if ((r = asb_probe_reset()) != 0)
        return r;
    if ((r = asb_probe_identify()) != 0)
        return r;
    if ((r = asb_probe_version()) != 0)
        return r;
    if ((r = asb_probe_irq()) != 0)
        return r;

    asb_speaker_on();
    return 0;
}

/*
 * SX.OVL ASB:0x14eb
 *
 * Reset the DSP: 1 to the reset port, four reads for the delay the card
 * needs, 0 back, and up to 0x20 reads for 0xAA - whether or not each read
 * timed out. Answers 0 if the card is there and 2 if it is not.
 */
uint16_t asb_probe_reset(void)
{
    uint16_t dx = ASB_DL(0x06);
    uint16_t cx = 0x20;
    uint16_t failed;
    uint8_t  al;

    io_out8(dx, 1);
    (void)io_in8(dx);
    (void)io_in8(dx);
    (void)io_in8(dx);
    (void)io_in8(dx);
    io_out8(dx, 0);

    do {
        al = asb_dsp_read_try(&failed);
    } while (--cx != 0 && al != 0xaa);

    return (uint16_t)(al == 0xaa ? 0 : 2);
}

/*
 * SX.OVL ASB:0x1516
 *
 * The identify handshake: DSP 0xE0 with 0xAA, answered with the complement,
 * 0x55. Answers 0, or 2 - 1.11's number for every failure here, where 1.00's
 * was 3.
 */
uint16_t asb_probe_identify(void)
{
    uint16_t dx = ASB_DX(0x0c);
    uint16_t failed;
    uint8_t  al;

    if (asb_dsp_write_try(dx, 0xe0))
        return 2;
    if (asb_dsp_write_try(dx, 0xaa))
        return 2;

    al = asb_dsp_read_try(&failed);
    if (failed || al != 0x55)
        return 2;

    return 0;
}

/*
 * SX.OVL ASB:0x1541
 *
 * Write to the DSP at DX, giving up after 0x800 polls. Answers non-zero - the
 * carry flag - if it gave up.
 */
uint16_t asb_dsp_write_try(uint16_t dx, uint8_t value)
{
    uint16_t cx = 0x800;

    do {
        if ((io_in8(dx) & 0x80) == 0) {
            io_out8(dx, value);
            return 0;
        }
    } while (--cx != 0);

    return 1;
}

/*
 * SX.OVL ASB:0x1555
 *
 * Read from the DSP, giving up after 0x800 polls of the status port; on
 * giving up `failed` is set and the answer is the last status read.
 */
uint8_t asb_dsp_read_try(uint16_t *failed)
{
    uint16_t dx = ASB_DL(0x0e);
    uint16_t cx = 0x800;
    uint8_t  al;

    do {
        al = io_in8(dx);
        if ((al & 0x80) != 0) {
            *failed = 0;
            return io_in8((uint16_t)((dx & 0xff00) | (uint8_t)(dx - 4)));
        }
    } while (--cx != 0);

    *failed = 1;
    return al;
}

/*
 * SX.OVL ASB:0x1574
 *
 * DSP 0xE1, the version, major byte first. Below 1.01 the card is too old
 * and the answer is 1; 2.00 and above sets `dsp_v2`, which a stream will run
 * on auto-init for. (1.00's answered 4, and noted 3.00 for an IRQ 10 probe
 * 1.11 does not make.)
 */
uint16_t asb_probe_version(void)
{
    uint16_t ver;

    asb_dsp_write(ASB_DL(0x0c), 0xe1);
    ver = (uint16_t)(asb_dsp_read() << 8);
    ver |= asb_dsp_read();

    if ((int16_t)ver < 0x0101)
        return 1;

    ASBS.dsp_v2 = (uint8_t)((int16_t)ver >= 0x0200 ? 1 : 0);
    return 0;
}

/*
 * SX.OVL ASB:0x15ae
 *
 * Read from the DSP, waiting as long as it takes.
 */
uint8_t asb_dsp_read(void)
{
    uint16_t dx = ASB_DL(0x0e);

    while ((io_in8(dx) & 0x80) == 0)
        ;
    return io_in8((uint16_t)((dx & 0xff00) | (uint8_t)(dx - 4)));
}

/*
 * SX.OVL ASB:0x15c4
 *
 * Connect the DAC to the output - DSP 0xD1, "speaker on".
 */
void asb_speaker_on(void)
{
    asb_dsp_write(ASB_DX(0x0c), 0xd1);
}

/*
 * SX.OVL ASB:0x15e3
 *
 * Find the IRQ by provoking one. Hook 2, 3, 5 and 7 - the four saved vectors
 * go to 0x15d3..0x15e2, the sixteen zero bytes before this routine - then
 * hand the card one byte of the module's own memory at 0x9e with DSP 0x14
 * and a length of zero, which finishes at once and raises the line, at a
 * time constant of 0x64. Whichever probe handler runs writes its number to
 * `irq`, and 0x800 turns of a spin is long enough to see it.
 *
 * The vectors go back afterwards. Answers 0, or 3 if nothing fired.
 */
uint16_t asb_probe_irq(void)
{
    uint16_t cx, answer, dx;

    asb_hook_irq(2, 0x15d3, 0x16bc);
    asb_hook_irq(3, 0x15d7, 0x16c5);
    asb_hook_irq(5, 0x15db, 0x16ce);
    asb_hook_irq(7, 0x15df, 0x16d7);

    asb_dma_program(g_sound_bank.module + 0x9e, 0, 0x49);

    dx = ASB_DX(0x0c);
    asb_dsp_write(dx, 0x40);
    asb_dsp_write(dx, 0x64);
    asb_dsp_write(dx, 0x14);
    asb_dsp_write(dx, 0);
    asb_dsp_write(dx, 0);

    cx = 0x800;
    ASBS.irq = 0;
    while (ASBS.irq == 0 && --cx != 0)
        io_sb_wait();   /* OURS: the original's spin waits to be preempted */

    answer = (uint16_t)(ASBS.irq == 0 ? 3 : 0);

    asb_unhook_irq(2, 0x15d3);
    asb_unhook_irq(3, 0x15d7);
    asb_unhook_irq(5, 0x15db);
    asb_unhook_irq(7, 0x15df);

    return answer;
}

/*
 * SX.OVL ASB:0x1693
 *
 * Program DMA channel 1 and unmask it: the mode, the address, CX the count
 * the hardware wants - one less than the length - and the page.
 */
void asb_dma_program(const uint8_t far *block, uint16_t count, uint8_t mode)
{
    io_out8(0x0a, 5);                       /* mask channel 1 */
    io_out8(0x0c, 0);                       /* clear the flip-flop */
    io_out8(0x0b, mode);
    io_dma1_memory(block);
    io_out8(0x03, (uint8_t)count);
    io_out8(0x03, (uint8_t)(count >> 8));
    io_out8(0x0a, 1);                       /* unmask */
}

/*
 * SX.OVL ASB:0x16bc
 *
 * One of four interrupt-probe handlers, one per candidate IRQ, each of which
 * does nothing but hand its number to `asb_probe_isr_tail`.
 */
void asb_probe_isr_2(void) { asb_probe_isr_tail(2); }
/* SX.OVL ASB:0x16c5 */
void asb_probe_isr_3(void) { asb_probe_isr_tail(3); }
/* SX.OVL ASB:0x16ce */
void asb_probe_isr_5(void) { asb_probe_isr_tail(5); }
/* SX.OVL ASB:0x16d7 */
void asb_probe_isr_7(void) { asb_probe_isr_tail(7); }

/*
 * SX.OVL ASB:0x16e0
 *
 * What the four share, a routine of its own in 1.11: note the IRQ, read the
 * card's status to drop its line, and EOI the master PIC.
 */
void asb_probe_isr_tail(uint8_t irq)
{
    ASBS.irq = irq;
    (void)io_in8(ASB_DX(0x0e));
    io_out8(0x20, 0x20);
}
