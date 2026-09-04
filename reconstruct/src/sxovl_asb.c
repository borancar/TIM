/*
 * `ASB:`, the digitised-sound module out of `SX.OVL`, as it is actually loaded.
 *
 * Provenance is `SX.OVL ASB:0xNNNN` - offsets within the loaded module, not
 * image addresses, because the loader chooses the segment. The module is the
 * 1906-byte `ASB:` chunk of the container, 2414 bytes once decompressed, and
 * `out/ASB_MOD.mem` is the dump every offset here was read from.
 *
 * **A module is not a driver.** `setup_sound_device` loads one of these *and*
 * one of the nine devices, and the two are independent indices in
 * `RESOURCE.CFG` - see docs/sound-driver.md. The driver plays notes; this
 * plays sampled bytes, by handing the Sound Blaster's DMA channel a block of
 * guest memory. Its far pointer lives in DGROUP at 0x4a98 and every call into
 * it comes through the `lcall [0x4a98]` at 0x0bbde, with the function number
 * in AX and a pointer to the argument block in SI.
 *
 * The module's own state lives in its code segment and is reached with `ASB8`
 * and `ASB16`; the port does not copy it anywhere.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * The card's ports, as the module addresses them: everything is an offset from
 * the base it found, which it keeps at `cs:[0x76]`.
 */
#define ASB_BASE        ASBU16(0x76)
#define ASB_RESET       (uint16_t)(ASB_BASE + 0x06)
#define ASB_READ_DATA   (uint16_t)(ASB_BASE + 0x0a)
#define ASB_WRITE       (uint16_t)(ASB_BASE + 0x0c)
#define ASB_READ_STATUS (uint16_t)(ASB_BASE + 0x0e)


/*
 * SX.OVL ASB:0x0377
 *
 * Write one byte to the DSP: spin until bit 7 of the write port clears, then
 * out. No timeout - the card is known to be there by the time anything calls
 * this.
 */
void asb_dsp_write(uint8_t value)
{
    uint16_t dx = ASB_WRITE;

    while ((io_in8(dx) & 0x80) != 0)
        ;
    io_out8(dx, value);
}

/*
 * SX.OVL ASB:0x038a
 *
 * The same write with a bounded wait: 0xffff turns of the loop and then the
 * out happens regardless. Only `asb_dma_pause` uses it, on the path where the
 * card may be mid-transfer and slow to answer.
 */
void asb_dsp_write_timed(uint8_t value)
{
    uint16_t dx = ASB_WRITE;
    uint16_t cx = 0;

    while (--cx != 0 && (io_in8(dx) & 0x80) != 0)
        ;
    io_out8(dx, value);
}

/*
 * SX.OVL ASB:0x070e
 *
 * A DSP write for the detection path, which must be able to fail: 0x800 turns
 * and then the carry comes back set and the caller gives up on this base.
 * Answers 1 for the carry rather than setting a flag.
 */
uint16_t asb_dsp_write_try(uint8_t value)
{
    uint16_t dx = ASB_WRITE;
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
 * SX.OVL ASB:0x072a
 *
 * Read one byte back from the DSP, bounded: poll the status port for bit 7,
 * then read the data port four below it. Answers the byte, or sets the carry -
 * reported here as a non-zero answer in `*failed`.
 */
uint8_t asb_dsp_read_try(uint16_t *failed)
{
    uint16_t dx = ASB_READ_STATUS;
    uint16_t cx = 0x800;

    do {
        if ((io_in8(dx) & 0x80) != 0) {
            *failed = 0;
            return io_in8((uint16_t)(dx - 4));
        }
    } while (--cx != 0);

    *failed = 1;
    return 0;
}

/*
 * SX.OVL ASB:0x0749
 *
 * The same read with no timeout, for once the card has answered already.
 */
uint8_t asb_dsp_read(void)
{
    uint16_t dx = ASB_READ_STATUS;

    while ((io_in8(dx) & 0x80) == 0)
        ;
    return io_in8((uint16_t)(dx - 4));
}

/*
 * SX.OVL ASB:0x0369
 *
 * Pause an eight-bit DMA transfer - DSP 0xD0 - through the bounded write,
 * because this is called from the interrupt handler.
 */
void asb_dma_pause(void)
{
    asb_dsp_write_timed(0xd0);
}

/*
 * SX.OVL ASB:0x0370
 *
 * Continue a paused transfer: DSP 0xD4.
 */
void asb_dma_continue(void)
{
    asb_dsp_write(0xd4);
}

/*
 * SX.OVL ASB:0x079e
 *
 * Connect the DAC to the output - DSP 0xD1, "speaker on". The last thing
 * detection does once a base has answered every probe.
 */
void asb_speaker_on(void)
{
    asb_dsp_write(0xd1);
}

/*
 * SX.OVL ASB:0x031b
 *
 * Set the sampling rate. The time constant is 256 - 1,000,000/rate, which the
 * original computes as a 32-bit divide of 0x000f4240 by the rate and then
 * `neg`, and sends as DSP 0x40 followed by the byte. The rate is kept at
 * `cs:[0x84]` as well.
 */
void asb_set_rate(uint16_t rate)
{
    uint8_t tc;

    ASB16(0x84) = (int16_t)rate;
    tc = (uint8_t)(1000000UL / rate);
    tc = (uint8_t)(-(int8_t)tc);

    asb_dsp_write(0x40);
    asb_dsp_write(tc);
}

/*
 * SX.OVL ASB:0x033a
 *
 * DSP 0x48, the block transfer size, low byte first. Nothing on the paths the
 * game reaches calls it - the module programs the length into DSP 0x14 instead
 * - but it is part of the module and is transcribed with it.
 */
void asb_set_block_size(uint16_t n)
{
    asb_dsp_write(0x48);
    asb_dsp_write((uint8_t)n);
    asb_dsp_write((uint8_t)(n >> 8));
}

/*
 * SX.OVL ASB:0x0355
 *
 * A far pointer to the 20-bit address the DMA controller wants: the segment
 * rotated left four, its top nibble becoming the page and the rest adding into
 * the offset. Returns the page in the high half and the offset in the low.
 */
uint32_t asb_linear(uint16_t off, uint16_t seg)
{
    uint16_t dx = (uint16_t)((seg << 4) | (seg >> 12));
    uint16_t cx = (uint16_t)(dx & 0xfff0);
    uint16_t page = (uint16_t)(dx & 0x000f);
    uint32_t sum = (uint32_t)off + cx;

    if (sum > 0xffff)
        page++;

    return ((uint32_t)page << 16) | (uint16_t)sum;
}

/*
 * SX.OVL ASB:0x08ec
 *
 * Program DMA channel 1 and unmask it. AX is the offset within the page, CX
 * the count the hardware wants - one less than the length - DH the mode byte
 * and DL the page.
 */
void asb_dma_program(uint16_t off, uint16_t count, uint8_t mode, uint8_t page)
{
    io_out8(0x0a, 5);                       /* mask channel 1            */
    io_out8(0x0c, 0);                       /* clear the flip-flop       */
    io_out8(0x0b, mode);
    io_out8(0x02, (uint8_t)off);
    io_out8(0x02, (uint8_t)(off >> 8));
    io_out8(0x03, (uint8_t)count);
    io_out8(0x03, (uint8_t)(count >> 8));
    io_out8(0x83, page);
    io_out8(0x0a, 1);                       /* unmask                    */
}

/*
 * SX.OVL ASB:0x025d
 *
 * Hand the card the block that `asb_arm_block` selected: channel 1 masked,
 * flip-flop cleared, address, mode 0x49 - single transfer, read from memory,
 * channel 1 - page, count less one, unmask, and then DSP 0x14 with the length
 * less one.
 *
 * The order is the original's and is not the same as `asb_dma_program`'s: the
 * mode goes out *after* the address here and before it there. Transcribed as
 * written; the two are separate routines in the module for that reason.
 */
void asb_dma_start(void)
{
    uint16_t cx;

    io_out8(0x0a, 5);
    io_out8(0x0c, 0);
    io_out8(0x02, (uint8_t)ASBU16(0x70));
    io_out8(0x02, (uint8_t)(ASBU16(0x70) >> 8));

    cx = ASBU16(0x6e);
    ASB16(0x6c) = (int16_t)cx;
    io_out8(0x0b, 0x49);
    io_out8(0x83, ASB8(0x39));
    io_out8(0x03, (uint8_t)(cx - 1));
    io_out8(0x03, (uint8_t)((cx - 1) >> 8));
    io_out8(0x0a, 1);

    asb_dsp_write(0x14);
    cx = (uint16_t)(ASBU16(0x6e) - 1);
    asb_dsp_write((uint8_t)cx);
    asb_dsp_write((uint8_t)(cx >> 8));
}

/*
 * SX.OVL ASB:0x0224
 *
 * Choose which of the two halves to play next and start it. A sample that
 * crosses a 64K DMA page is split in two when it is handed over, and
 * `cs:[0x4c]` says which half is current: zero takes the page, offset and
 * length at 0x34/0x58/0x56, one takes 0x35/0x5c/0x5a.
 */
void asb_arm_block(void)
{
    if (ASB8(0x4c) == 0) {
        ASB8(0x39) = ASB8(0x34);
        ASB16(0x70) = (int16_t)ASBU16(0x58);
        ASB16(0x6e) = (int16_t)ASBU16(0x56);
    } else {
        ASB8(0x39) = ASB8(0x35);
        ASB16(0x70) = (int16_t)ASBU16(0x5c);
        ASB16(0x6e) = (int16_t)ASBU16(0x5a);
    }

    asb_dma_start();
}

/*
 * SX.OVL ASB:0x02a9
 *
 * Mask DMA channel 1 and acknowledge the card's interrupt by reading the
 * status port, which is what tells it to drop the line.
 */
void asb_dma_stop(void)
{
    io_out8(0x0a, 5);
    (void)io_in8(ASB_READ_STATUS);
}

/*
 * SX.OVL ASB:0x02b7
 *
 * The card's interrupt handler, and the whole reason the module has one: a
 * block has finished, so acknowledge it, stop the channel, and either start
 * the sample's second half, start it again, or stop.
 *
 * `cs:[0x5a]` non-zero means the sample was split across a page and
 * `cs:[0x4c]` flips between the halves - the `xor ... , 1` sets the zero flag
 * when the result is zero, so the *second* half is the one that starts here
 * and the flip back to zero falls through to the end. `cs:[0x47]` is the loop
 * flag: set, the sample starts over and `cs:[0x46]` is raised to say a pass
 * finished; clear, `asb_shutdown` takes the card down.
 *
 * The EOI goes to the slave as well when the IRQ is above 7.
 */
void asb_isr(void)
{
    (void)io_in8(ASB_READ_STATUS);
    asb_dma_pause();
    asb_dma_stop();

    if (ASBU16(0x5a) != 0 && (ASB8(0x4c) ^= 1) != 0) {
        asb_arm_block();
    } else if (ASB8(0x47) == 1) {
        asb_arm_block();
        ASB8(0x46) = 1;
    } else {
        asb_stop();
    }

    if (ASB8(0x45) > 7)
        io_out8(0xa0, 0x20);
    io_out8(0x20, 0x20);
}

/*
 * OURS: which routine a hooked vector names.
 *
 * `asb_hook_irq` is handed the near offset of a handler inside the module, and
 * on the original that offset *is* the handler. The port has C functions
 * instead, so the offsets are mapped back to them here. Every one is read off
 * the module's own code - they are the five interrupt-probe handlers and the
 * real one - so this says which routine is meant, it does not decide it.
 */
static void (*asb_handler_for(uint16_t off))(void)
{
    switch (off) {
    case 0x02b7: return asb_isr;
    case 0x0915: return asb_probe_isr_2;
    case 0x091e: return asb_probe_isr_3;
    case 0x0927: return asb_probe_isr_5;
    case 0x0930: return asb_probe_isr_7;
    case 0x0939: return asb_probe_isr_10;
    }
    return 0;
}

/*
 * SX.OVL ASB:0x03a5
 *
 * Hook a hardware interrupt: AL is the IRQ, which becomes vector IRQ+8 below 8
 * and IRQ+0x68 above it, BX is where in the module's own segment the old
 * far pointer is kept, and DX the near offset of the new handler. The old
 * vector is saved, the new one written, and the IRQ unmasked at the PIC whose
 * mask port is at `cs:[0x74]`. Answers the mask byte as it was.
 *
 * **The port does not run guest interrupt vectors**, so the vector write is
 * kept for the state it leaves behind and the handler is registered with the
 * hardware by `io_on_sb_irq` instead - the same shape as the timer. The near
 * offset is the module's own, so which routine is meant is read off the
 * module, not decided here.
 */
uint8_t asb_hook_irq(uint8_t irq, uint16_t save_at, uint16_t handler)
{
    uint16_t vec = (uint16_t)(irq < 8 ? irq + 8 : irq + 0x68);
    uint16_t mask;
    uint8_t  bit, was;

    {
        uint32_t old = dos_getvect(vec);
        ASB16(save_at)     = (int16_t)(uint16_t)old;
        ASB16(save_at + 2) = (int16_t)(uint16_t)(old >> 16);
    }
    dos_setvect(vec, handler, ASB_SEG);

    bit = (uint8_t)(irq < 8 ? (1u << irq) : (1u << (irq - 8)));

    mask = ASBU16(0x74);
    was = io_in8(mask);
    io_out8(mask, (uint8_t)(was & ~bit));

    io_on_sb_irq(irq, asb_handler_for(handler));
    return was;
}

/*
 * SX.OVL ASB:0x03f6
 *
 * Put a hooked vector back and restore the mask byte AH was carrying.
 */
void asb_unhook_irq(uint8_t irq, uint16_t save_at, uint8_t mask_was)
{
    uint16_t vec = (uint16_t)(irq < 8 ? irq + 8 : irq + 0x68);

    dos_setvect(vec, ASBU16(save_at), ASBU16(save_at + 2));
    io_out8(ASBU16(0x74), mask_was);

    io_on_sb_irq(irq, 0);
}

/*
 * SX.OVL ASB:0x06c0
 *
 * Reset the DSP: 1 to the reset port, four reads for the delay the card needs,
 * 0 back, and then up to 0x20 attempts to read 0xAA. Answers 0 if the card is
 * there and 2 if it is not.
 */
uint16_t asb_probe_reset(void)
{
    uint16_t dx = ASB_RESET;
    uint16_t cx = 0x20;
    uint8_t  al;
    uint16_t failed;

    io_out8(dx, 1);
    (void)io_in8(dx);
    (void)io_in8(dx);
    (void)io_in8(dx);
    (void)io_in8(dx);
    io_out8(dx, 0);

    do {
        al = asb_dsp_read_try(&failed);
        if (al == 0xaa)
            return 0;
    } while (--cx != 0);

    return 2;
}

/*
 * SX.OVL ASB:0x06eb
 *
 * The identify handshake: DSP 0xE0 with 0xAA as its argument answers with the
 * complement, 0x55. Answers 0 or 3.
 */
uint16_t asb_probe_identify(void)
{
    uint16_t failed;
    uint8_t  al;

    if (asb_dsp_write_try(0xe0))
        return 3;
    if (asb_dsp_write_try(0xaa))
        return 3;

    al = asb_dsp_read_try(&failed);
    if (failed || al != 0x55)
        return 3;

    return 0;
}

/*
 * SX.OVL ASB:0x075d
 *
 * DSP 0xE1, the version, major byte first. Below 1.01 the card is too old and
 * the answer is 4; 2.00 and above sets `cs:[0x38]`, and 3.00 and above sets
 * `cs:[0x4f]` as well, which is what later decides whether IRQ 10 is worth
 * probing.
 */
uint16_t asb_probe_version(void)
{
    uint16_t ver;

    asb_dsp_write(0xe1);
    ver = (uint16_t)(asb_dsp_read() << 8);
    ver |= asb_dsp_read();

    ASB8(0x38) = 0;
    ASB8(0x4f) = 0;

    if ((int16_t)ver < 0x0101)
        return 4;

    if ((int16_t)ver >= 0x0200) {
        ASB8(0x38) = 1;
        if ((int16_t)ver >= 0x0300)
            ASB8(0x4f) = 1;
    }

    return 0;
}

/*
 * OURS: the body the five interrupt-probe handlers share.
 *
 * On the module each of them is its own routine and they differ in one byte -
 * the `mov dl,<irq>` - because they all tail into the same 0x0942. That common
 * tail is this, and the five below are the routines; splitting it this way
 * keeps one copy of the code the module has one copy of.
 */
static void asb_probe_isr(uint8_t irq)
{
    ASB8(0x45) = irq;
    io_out8(0x0a, 5);
    (void)io_in8((uint16_t)(ASB_BASE + 0x0e));

    if (ASB8(0x45) > 7)
        io_out8(0xa0, 0x20);
    io_out8(0x20, 0x20);
}

/*
 * SX.OVL ASB:0x0915
 *
 * One of five interrupt-probe handlers, one per candidate IRQ, each of which
 * does nothing but write its own number into `cs:[0x45]` and acknowledge.
 * Which one fires is how the module learns which IRQ the card is on.
 */
void asb_probe_isr_2(void)  { asb_probe_isr(2);  }

/* SX.OVL ASB:0x091e */
void asb_probe_isr_3(void)  { asb_probe_isr(3);  }

/* SX.OVL ASB:0x0927 */
void asb_probe_isr_5(void)  { asb_probe_isr(5);  }

/* SX.OVL ASB:0x0930 */
void asb_probe_isr_7(void)  { asb_probe_isr(7);  }

/* SX.OVL ASB:0x0939 */
void asb_probe_isr_10(void) { asb_probe_isr(10); }

/*
 * SX.OVL ASB:0x07c5
 *
 * Find the IRQ, by provoking one. Hook 2, 3, 5 and 7 - and 10 as well if the
 * DSP said 3.00 or better, which needs the mask port switched to the slave's
 * 0xa1 for that one - then hand the card a single byte of the module's own
 * memory at 0xa6 with DSP 0x14 and a length of zero, which finishes at once
 * and raises the line. Whichever probe handler runs writes its number into
 * `cs:[0x45]`, and 0x800 turns of a spin is long enough to see it.
 *
 * Every vector goes back afterwards, and `cs:[0x74]` is left holding the mask
 * port the found IRQ belongs to. Answers 0, or 5 if nothing fired.
 */
uint16_t asb_probe_irq(void)
{
    uint32_t lin;
    uint16_t cx, answer;

    ASB8(0x7b9) = asb_hook_irq(2, 0x7a5, 0x0915);
    ASB8(0x7ba) = asb_hook_irq(3, 0x7a9, 0x091e);
    ASB8(0x7bb) = asb_hook_irq(5, 0x7ad, 0x0927);
    ASB8(0x7bc) = asb_hook_irq(7, 0x7b1, 0x0930);

    if (ASB8(0x4f) == 1) {
        ASB16(0x74) = (int16_t)0xa1;
        ASB8(0x7bd) = asb_hook_irq(10, 0x7b5, 0x0939);
    }

    lin = asb_linear(0xa6, ASB_SEG);
    asb_dma_program((uint16_t)lin, 0, 0x49, (uint8_t)(lin >> 16));

    asb_dsp_write(0x40);
    asb_dsp_write(0x64);
    asb_dsp_write(0x14);
    asb_dsp_write(0);
    asb_dsp_write(0);

    ASB8(0x45) = 0;
    cx = 0x800;
    while (ASB8(0x45) == 0 && --cx != 0)
        io_sb_wait();   /* OURS: the original's spin waits to be preempted */

    answer = (uint16_t)(ASB8(0x45) == 0 ? 5 : 0);

    if (ASB8(0x4f) == 1) {
        ASB16(0x74) = (int16_t)0xa1;
        asb_unhook_irq(10, 0x7b5, ASB8(0x7bd));
    }
    asb_unhook_irq(7, 0x7b1, ASB8(0x7bc));
    asb_unhook_irq(5, 0x7ad, ASB8(0x7bb));
    ASB16(0x74) = 0x21;
    asb_unhook_irq(3, 0x7a9, ASB8(0x7ba));
    asb_unhook_irq(2, 0x7a5, ASB8(0x7b9));

    ASB16(0x74) = (int16_t)(ASB8(0x45) > 7 ? 0xa1 : 0x21);
    return answer;
}

/*
 * SX.OVL ASB:0x069c
 *
 * Try one base port: reset, identify, version, IRQ, and then the speaker on.
 * Answers 0 when every step passed.
 */
uint16_t asb_try_base(uint16_t base)
{
    ASB16(0x76) = (int16_t)base;

    if (asb_probe_reset())
        return 2;
    if (asb_probe_identify())
        return 3;
    if (asb_probe_version())
        return 4;
    if (asb_probe_irq())
        return 5;

    asb_speaker_on();
    return 0;
}

/*
 * SX.OVL ASB:0x0665
 *
 * The six bases a Sound Blaster could be jumpered to, in the order the module
 * tries them. Answers 0 as soon as one works.
 */
uint16_t asb_detect(void)
{
    static const uint16_t BASES[6] = {
        0x220, 0x240, 0x210, 0x230, 0x250, 0x260
    };
    uint16_t i, r = 0;

    for (i = 0; i < 6; i++) {
        r = asb_try_base(BASES[i]);
        if (r == 0)
            return 0;
    }

    return r;
}

/*
 * SX.OVL ASB:0x052b, 0x053e, 0x0551, 0x0564
 *
 * Four vector hooks that do nothing but say "busy": each raises a byte, chains
 * to the handler it replaced, and lowers it again. With the DOS InDOS flag at
 * `cs:[0x92]` and the one byte below it at `cs:[0x8e]`, they are what
 * `asb_safe_to_call` adds up.
 *
 * **The port has no vectors to chain to**, so these are the flags and nothing
 * else. They exist because the module's state includes them and because
 * `asb_safe_to_call` reads them.
 */
void asb_int10_hook(void) { ASB8(0x43) = 1; ASB8(0x43) = 0; }
void asb_int0d_hook(void) { ASB8(0x3f) = 1; ASB8(0x3f) = 0; }
void asb_int74_hook(void) { ASB8(0x40) = 1; ASB8(0x40) = 0; }
void asb_int09_hook(void) { ASB8(0x3e) = 1; ASB8(0x3e) = 0; }

/*
 * SX.OVL ASB:0x0506
 *
 * Is it safe to go near DOS and the BIOS? The `or` of the two InDOS bytes and
 * the four busy flags, so zero means nothing is in progress. Nothing on the
 * paths the game reaches calls it; it is here because the four hooks above
 * only make sense with it.
 */
uint8_t asb_safe_to_call(void)
{
    uint8_t al;

    al  = *(uint8_t *)FAR_PTR(ASBU16(0x90), ASBU16(0x8e));
    al |= *(uint8_t *)FAR_PTR(ASBU16(0x94), ASBU16(0x92));
    al |= ASB8(0x43);
    al |= ASB8(0x3f);
    al |= ASB8(0x40);
    al |= ASB8(0x3e);

    return al;
}

/*
 * SX.OVL ASB:0x00f5  - function 12
 *
 * Stop the card and give the interrupt back. `cs:[0x54]` is the guard: once it
 * is 1 this does nothing, which is why the interrupt handler can call it on
 * the last block and the game can call it again afterwards.
 */
uint16_t asb_shutdown(void)
{
    if (ASB8(0x54) == 1)
        return 0;

    ASB8(0x54) = 1;
    asb_dma_pause();
    asb_dma_stop();
    asb_unhook_irq(ASB8(0x45), 0x8a, ASB8(0x4e));

    return 0;
}

/*
 * SX.OVL ASB:0x011e  - function 3
 *
 * Play a sample. The argument block is five words: a flag whose high byte asks
 * for looping, the rate, the sample's far pointer, and its length.
 *
 * The length is added to the offset within the 64K DMA page, and a carry means
 * the block crosses the page - so it is cut in two, the first running to the
 * end of the page and the second starting at offset 0 of the next one. That
 * second half is what `asb_isr` starts when the first finishes; when there is
 * no carry `cs:[0x5a]` is zero and there is no second half.
 *
 * Then the IRQ is hooked, the flags are cleared, and the first block goes.
 */
void asb_play(uint16_t si)
{
    uint32_t lin;
    uint16_t ax;

    asb_shutdown();

    if ((DGU16(si) >> 8) != 0)
        ASB8(0x47) = 1;
    else
        ASB8(0x47) = 0;

    asb_set_rate(DGU16((uint16_t)(si + 2)));

    lin = asb_linear(DGU16((uint16_t)(si + 4)), DGU16((uint16_t)(si + 6)));
    ASB8(0x34)  = (uint8_t)(lin >> 16);
    ASB16(0x58) = (int16_t)lin;

    ax = DGU16((uint16_t)(si + 8));
    ASB16(0x56) = (int16_t)ax;

    if ((uint32_t)ax + ASBU16(0x58) > 0xffff) {
        ax = (uint16_t)(ax + ASBU16(0x58));
        ASB16(0x5a) = (int16_t)ax;
        ASB16(0x56) = (int16_t)(ASBU16(0x56) - ax);
        ASB16(0x5c) = 0;
        ASB8(0x35)  = (uint8_t)((lin >> 16) + 1);
    } else {
        ASB16(0x5a) = 0;
    }

    ASB8(0x4e) = asb_hook_irq(ASB8(0x45), 0x8a, 0x02b7);
    ASB8(0x4c) = 0;
    ASB8(0x3b) = 0;
    ASB8(0x3d) = 1;

    asb_arm_block();

    ASB16(0x72) = 0;
    ASB8(0x54) = 0;
    ASB8(0x46) = 0;
}

/*
 * SX.OVL ASB:0x01be  - function 4
 *
 * How the sample is getting on: AL is `cs:[0x54]`, set once it has stopped,
 * and AH is `cs:[0x46]`, raised by the interrupt handler when a looping pass
 * came round. Reading it clears the second.
 */
uint16_t asb_status(void)
{
    uint16_t r = (uint16_t)(ASB8(0x54) | (ASB8(0x46) << 8));

    ASB8(0x46) = 0;
    return r;
}

/*
 * SX.OVL ASB:0x01ce  - function 5
 */
void asb_stop(void)
{
    asb_shutdown();
}

/*
 * SX.OVL ASB:0x01d2  - function 2
 *
 * Take the module out: stop, put the four chained vectors back, and close the
 * sample file if one is open.
 */
uint16_t asb_uninstall(void)
{
    asb_shutdown();

    dos_setvect(0x10, ASBU16(0x9e), ASBU16(0xa0));
    dos_setvect(0x0d, ASBU16(0x96), ASBU16(0x98));
    dos_setvect(0x74, ASBU16(0x9a), ASBU16(0x9c));
    dos_setvect(0x09, ASBU16(0xa2), ASBU16(0xa4));

    if (ASBU16(0x7a) != 0xffff) {
        io_dos_close((int16_t)ASBU16(0x7a));
        ASB16(0x7a) = (int16_t)0xffff;
    }

    return 0;
}

/*
 * SX.OVL ASB:0x00de  - function 6
 */
uint16_t asb_set_rate_fn(uint16_t si)
{
    ASB16(0x78) = (int16_t)DGU16(si);
    asb_set_rate(DGU16(si));
    return 0;
}

/*
 * SX.OVL ASB:0x00ec  - function 8
 */
uint16_t asb_clear_49(void)
{
    ASB8(0x49) = 0;
    return 0;
}

/*
 * SX.OVL ASB:0x0435  - function 13
 *
 * Where the sample has got to, as three words in the caller's block: an id
 * from `cs:[0x72]` and a 32-bit position. The position is the count still to
 * go, read straight out of the DMA controller's current-count register at port
 * 3 - low byte then high, the flip-flop having been cleared when the channel
 * was programmed - taken away from what was programmed and added to the base
 * at `cs:[0x64]`.
 *
 * All ones means the sample is past its end or has stopped; all zeroes means
 * `cs:[0x4d]` says there is nothing to report.
 */
uint16_t asb_position(uint16_t si)
{
    uint16_t cx, dx, ax, bx;

    if (ASB8(0x4d) == 1) {
        DGU16(si) = 0;
        DGU16((uint16_t)(si + 2)) = 0;
        DGU16((uint16_t)(si + 4)) = 0;
        return 0;
    }

    if (ASB8(0x54) == 1) {
        DGU16(si) = 0xffff;
        DGU16((uint16_t)(si + 2)) = 0xffff;
        DGU16((uint16_t)(si + 4)) = 0xffff;
        return 0;
    }

    cx  = io_in8(0x03);
    cx |= (uint16_t)(io_in8(0x03) << 8);

    dx = ASBU16(0x6c);
    ax = ASBU16(0x66);
    bx = ASBU16(0x64);

    dx = (uint16_t)(dx - cx);
    {
        uint32_t sum = (uint32_t)ax + dx;
        ax = (uint16_t)sum;
        bx = (uint16_t)(bx + (sum >> 16));
    }

    if (ASB8(0x52) == 1) {
        uint32_t v = ((uint32_t)bx << 16 | ax) << 1;
        ax = (uint16_t)v;
        bx = (uint16_t)(v >> 16);
    }

    if (ASB8(0x44) != 1) {
        if (ASB8(0x36) == 1) {
            uint32_t v = ((uint32_t)bx << 16 | ax) >> 1;
            ax = (uint16_t)v;
            bx = (uint16_t)(v >> 16);
        }

        if (bx != ASBU16(0x80) ? bx > ASBU16(0x80) : ax > ASBU16(0x82)) {
            DGU16(si) = 0xffff;
            DGU16((uint16_t)(si + 2)) = 0xffff;
            DGU16((uint16_t)(si + 4)) = 0xffff;
            return 0;
        }

        if (ASB8(0x36) == 1) {
            uint32_t v = ((uint32_t)bx << 16 | ax) << 1;
            ax = (uint16_t)v;
            bx = (uint16_t)(v >> 16);
        }
    }

    DGU16((uint16_t)(si + 2)) = ax;
    DGU16((uint16_t)(si + 4)) = bx;
    DGU16(si) = ASBU16(0x72);

    return 0;
}

/*
 * SX.OVL ASB:0x0577  - function 0
 *
 * Install: find the card, chain the four vectors, take the InDOS flag's
 * address, and set the default rate of 11025.
 *
 * The `int 2fh` check the original does first is guarded by `cs:[0x96d]` being
 * 0xcd, and in this build that byte is 0xdc, so the branch is dead and is not
 * transcribed. Answers 0x577 - its own address, so non-zero - when the card is
 * there and 0 when it is not.
 */
uint16_t asb_install(void)
{
    uint32_t indos;

    if (asb_detect() != 0)
        return 0;

    ASB8(0x54) = 1;

    ASB16(0x9e) = (int16_t)(uint16_t)dos_getvect(0x10);
    ASB16(0xa0) = (int16_t)(uint16_t)(dos_getvect(0x10) >> 16);
    dos_setvect(0x10, 0x052b, ASB_SEG);

    ASB16(0x96) = (int16_t)(uint16_t)dos_getvect(0x0d);
    ASB16(0x98) = (int16_t)(uint16_t)(dos_getvect(0x0d) >> 16);
    dos_setvect(0x0d, 0x053e, ASB_SEG);

    ASB16(0x9a) = (int16_t)(uint16_t)dos_getvect(0x74);
    ASB16(0x9c) = (int16_t)(uint16_t)(dos_getvect(0x74) >> 16);
    dos_setvect(0x74, 0x0551, ASB_SEG);

    ASB16(0xa2) = (int16_t)(uint16_t)dos_getvect(0x09);
    ASB16(0xa4) = (int16_t)(uint16_t)(dos_getvect(0x09) >> 16);
    dos_setvect(0x09, 0x0564, ASB_SEG);

    /*
     * INT 21h AH=34h, the address of the InDOS flag, and the byte below it.
     * **The port has no DOS to be inside**, so both point at a byte of low
     * memory that stays zero and `asb_safe_to_call` therefore always says it
     * is safe - which is the truth here rather than a shortcut.
     */
    indos = 0;
    ASB16(0x92) = 1;
    ASB16(0x94) = 0;
    ASB16(0x8e) = 0;
    ASB16(0x90) = 0;
    (void)indos;

    ASB16(0x78) = (int16_t)0x2b11;          /* 11025 Hz */
    asb_set_rate(0x2b11);

    ASB8(0x42) = 0;
    ASB8(0x41) = 0;
    ASB16(0x7a) = (int16_t)0xffff;

    return 0x577;
}

/*
 * SX.OVL ASB:0x00c8
 *
 * The module's one entry point. AX picks one of the sixteen far offsets in the
 * table at `cs:0xa8` and SI points at the caller's arguments; the answer comes
 * back in AX with the zero flag set from it.
 *
 * The table's other entries - 1, 7, 9, 10, 11, 14 and 15 - are the bare `ret`s
 * and `xor ax,ax; ret`s at 0xda, 0xdd, 0xeb, 0x429, 0x42c, 0x42f and 0x430.
 * This module implements none of them, which is why the game's wrappers for
 * 9, 10 and 11 at 0x0bbb1, 0x0bbb8 and 0x0bbbf do nothing when it is loaded.
 */
uint16_t asb_dispatch(uint16_t fn, uint16_t si)
{
    switch (fn) {
    case 0:  return asb_install();
    case 2:  return asb_uninstall();
    case 3:  asb_play(si); return 0;
    case 4:  return asb_status();
    case 5:  asb_stop(); return 0;
    case 6:  return asb_set_rate_fn(si);
    case 8:  return asb_clear_49();
    case 12: return asb_shutdown();
    case 13: return asb_position(si);

    case 1: case 7: case 9: case 10: case 11: case 14: case 15:
        return 0;
    }

    return 0;
}
