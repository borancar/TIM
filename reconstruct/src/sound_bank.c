/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Sequences, sound banks and the record index.**
 *
 * The fourth module of the original's **code segment 2619**, image
 * 0x28935..0x293c1 - the second of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/* The five-tick wait, DGROUP 0x6430; the record is described in dgroup.h. */
struct sound_tick_wait SOUND_TICK_WAIT DGROUP_WAS(0x6430);

/*
 * 0x28935
 *
 * Build a sequence record around a block of note data, and answer it as a far
 * pointer - or a null one if there was no room.
 *
 * The record is 0x17a bytes of kind 2, so `alloc_for_kind` zeroes it; every
 * field not written below is therefore known to be zero rather than merely
 * assumed so.
 *
 * The source pointer is kept at +0x166, and +0x16a gets the same pointer
 * stepped past the first record by `advance_record` - the segment half is
 * carried across unchanged, because that routine only moves the offset.
 *
 * +8 is then made to point at **+0x16a of the record itself**, so the cursor
 * the sequencer follows lives inside the record and starts at the second entry.
 * That is why the record's own segment is stored beside it at +0xa.
 *
 * +0x15e is set to 0x7f, which `sequencer_tick` reads as "use the default"
 * where it feeds `scale_byte_pair`, and the two words at +0x172 are cleared
 * again although the allocation already did it.
 */
struct sequence far *create_sequence(const uint8_t far * src)
{
    struct sequence *seq = (struct sequence *)(void *)alloc_for_kind(sizeof(struct sequence), 2);

    if ((uint8_t *)seq == FAR_NULL_PTR)
        return seq;

    /* `cursor` and `cursor_at` are stepped inside their blocks' segments:
       `advance_record` and the `+ 0x16a` move the offset alone. */
    seq->source = src;
    seq->cursor = (uint8_t far *)advance_record(src);
    seq->cursor_at = &seq->cursor;

    seq->volume = 0x7f;
    seq->next = NULL;

    return seq;
}

/*
 * 0x289ba
 *
 * Follow a chain to its end and, if anything is there, retire and tick.
 *
 * The original writes `follow_far_chain`'s answer back over its own stack
 * arguments before testing it, which is a compiler reusing the incoming slots
 * as a local and not a second meaning for them.
 */
void follow_then_tick(struct sequence far * seq, int16_t count)
{
    struct sequence *p = follow_far_chain(seq, count);

    if (p != SEQUENCE_NONE)
        retire_and_tick_far(p);
}

/*
 * 0x289e8
 *
 * Load the sound bank for whatever device is configured, and answer it as a far
 * pointer, or null.
 *
 * The device at DGROUP 0x4aae picks the identifier to look for. Four of the
 * cases go through a jump table in this code segment at `cs:0x2a17`, which is
 * how a Borland `switch` over a small dense range is compiled, and the rest are
 * compares - 5 and 6 landing on the same answers as 1 and 2, 7 on its own, and
 * 0x7e taking the identifier from DGROUP 0x4a9e instead of a constant. Anything
 * else answers null without opening the resource at all.
 *
 * Then the resource is opened under the name at DGROUP 0x4a7e, walked to that
 * record, and its items read into a node list.
 *
 * The block to hold them is sized by walking that list: six bytes of directory
 * per node plus five, rounded up to even and to at least 0x26, and then the
 * items' own lengths on top - and one more byte, which is the `add dx,1` before
 * the allocation. That rounded directory size is also where the items start, so
 * it is handed to `build_sound_index` as the same number.
 *
 * DGROUP 0x4a9c is set to 2 on the two failures that mean the resource was
 * there but the bank was not - a missing record, or a list that came back
 * empty. An allocation that fails does not set it.
 *
 * The node list is freed on every path, and the resource closed on every path
 * that opened it.
 */
uint8_t far *load_sound_bank(FILE *file, uint32_t size,
                         uint8_t * out)
{
    uint16_t want;
    int16_t handle;
    struct sound_node *list = SOUND_NODE_NONE;
    uint8_t *blk = FAR_NULL_PTR;
    uint8_t *r = FAR_NULL_PTR;

    /*
     * None of this routine's locals has its address taken, but the ones it
     * calls do, and their frames have to land **below** this one. So the frame
     * is reserved anyway: 0x12 bytes of locals and the two saved registers.
     */

    switch (DG4A82.device) {
    case 0:    want = 0x12; break;
    case 1:    want = 0x13; break;
    case 2:    want = 0;    break;
    case 3:    want = 0xc;  break;
    case 5:    want = 0x13; break;
    case 6:    want = 0;    break;
    case 0x7e: want = ((uint8_t)DG4A82.identifier); break;

    /*
     * The original **falls through here**, and so does this. At 0x28a4c it
     * stores 7 and the next instruction is 0x28a50, `xor dx,dx / xor ax,ax`,
     * which is where `default` goes; every other case ends `jmp 0x28a5a` and
     * goes on to open the resource. So the store is dead and `GMD:` can never
     * load a sound bank - a bug in Dynamix's code, transcribed as it behaves.
     *
     * The port carried a deliberate fix here between 2026-09-04 and the
     * removal of the General Midi driver later the same day. With `GMD:` gone
     * the fix had nothing left to fix, so the deviation went with it and this
     * file is a transcription again.
     */
    case 7:    want = 7;    /* falls through: the store is dead */

    default:   goto out;
    }

    handle = open_resource(0, file, SOUND_TAGS.mode_r_a, size);
    if (handle < 0)
        goto out;

    if (seek_to_sound_record(handle, want) == 0) {
        DG4A82.load_error = 2;
        close_resource(handle);
        goto out;
    }

    {
        list = read_sound_records(handle);

        if (list == SOUND_NODE_NONE) {
            DG4A82.load_error = 2;
            close_resource(handle);
            goto out;
        }
    }

    {
        const struct sound_node *walk = list;
        /* One 32-bit total. The port had it as two words with the carry
           tested by hand, which is how the original's `add`/`adc` reads on
           the way in; every use of it below is of the whole. */
        uint32_t len = 0;
        uint16_t si = 5;

        while (walk != SOUND_NODE_NONE) {
            uint16_t n = walk->length;

            len += n;

            si = (uint16_t)(si + 6);
            walk = walk->next;
        }

        if ((si & 1) != 0)
            si++;
        if (si < 0x26)
            si = 0x26;

        len += si;

        {
            blk = alloc_for_kind(len + 1, 4);
            if (blk == FAR_NULL_PTR) {
                close_resource(handle);
                free_node_list(list);
                goto out;
            }
        }

        if (build_sound_index(handle, list,
                              blk,
                              si, want) == 0) {
            close_resource(handle);
            free_node_list(list);
            goto out;
        }

        free_node_list(list);

        if (out != NULL) {
            *(int16_t *)(out + 2) = (int16_t)(len >> 16);
            *(int16_t *)(out) = (int16_t)len;
        }
    }

    close_resource(handle);
    r = blk;

out:
    return r;
}

/*
 * 0x28baf
 *
 * Free a whole chain of nodes, each linked to the next by the far pointer at
 * its +4, and all of them kind 9.
 *
 * The next pointer is read out **before** the node is freed, into the routine's
 * own locals; reading it afterwards would be following a pointer into a block
 * that has just been given back. The original does this by overwriting its own
 * two argument words with the next pointer and freeing the copy it kept, so the
 * argument is also the loop variable.
 *
 * A null chain is not a special case - the test is at the top.
 */
void free_node_list(struct sound_node far * list)
{
    while (list != SOUND_NODE_NONE) {
        struct sound_node *cur = list;

        list = list->next;
        free_for_kind((uint8_t *)cur, 9);
    }
}

/*
 * 0x28bf2
 *
 * Walk a resource's record list looking for one with a given identifier.
 * Answers 1 if it stopped on it, 0 for anything else.
 *
 * The resource opens with 0x84 and one more byte; anything else and this gives
 * up at once. After that it is a list of records, each an identifier byte
 * followed by items terminated by 0xff, and each item five bytes long - which
 * are stepped over with `resource_seek` rather than read, since only the
 * identifiers matter here.
 *
 * An identifier of 0xff ends the list and is the failure. Every read that does
 * not answer 1 is also a failure, so a truncated resource stops rather than
 * running on.
 *
 * The three bytes it reads into are locals, and their addresses are handed to
 * `read_resource` as `SS:offset` - which in this program is a DGROUP address,
 * so the port puts them on the guest stack. See `dg_alloca` in dgroup.h.
 *
 * The name is a guess from the shape; what the records are is not established
 * here.
 */
uint16_t seek_to_sound_record(int16_t handle, uint16_t want)
{
    uint16_t fp = dg_alloca(6);            /* four bytes of locals, and SI */
    uint16_t bp = (uint16_t)(fp + 6);
    /* The three bytes at [bp-3], [bp-2] and [bp-1], as pointers: the frame
       has to be the guest's, because `read_resource` takes its destination
       as a DGROUP address, but nothing here needs their offsets again. */
    uint8_t *b3 = dg_near_ptr((uint16_t)(bp - 3));
    uint8_t *b2 = dg_near_ptr((uint16_t)(bp - 2));
    uint8_t *b1 = dg_near_ptr((uint16_t)(bp - 1));
    uint16_t r = 0;

    if (read_resource(handle, b3, 1) != 1)
        goto out;
    if (*b3 != 0x84)
        goto out;
    if (read_resource(handle, b3, 1) != 1)
        goto out;
    if (read_resource(handle, b1, 1) != 1)
        goto out;

    for (;;) {
        if (*b1 == (uint8_t)want) {
            r = 1;
            goto out;
        }

        if (*b1 == 0xff)
            goto out;
        if (read_resource(handle, b2, 1) != 1)
            goto out;

        while (*b2 != 0xff) {
            resource_seek(handle, 5, 1);
            if (read_resource(handle, b2, 1) != 1)
                goto out;
        }

        if (read_resource(handle, b1, 1) != 1)
            goto out;
    }

out:
    dg_free(6);
    return r;
}

/*
 * 0x28cf7
 *
 * Read a run of four-byte items out of a resource into an ordered list of
 * eight-byte nodes, and answer the head.
 *
 * Each item is preceded by a byte that is read *ahead* - once before the loop
 * and once at the end of each turn - so the terminating 0xff is seen before a
 * node is allocated for it. One byte is skipped before each item, which is what
 * `resource_seek` with a whence of 1 is doing.
 *
 * A node is 8 bytes of kind 9: four read from the resource and a link at +4
 * that is cleared first, which is the layout `insert_by_key` expects - it
 * orders on the word at +0 and links at +4.
 *
 * The first node becomes the head outright; every later one goes through
 * `insert_by_key`, which can move the head.
 *
 * If an allocation fails part-way the whole list is freed and null answered.
 * That is the only path on which the byte read ahead is not 0xff, which is what
 * the second test distinguishes.
 */
struct sound_node far *read_sound_records(int16_t handle)
{
    uint16_t fp = dg_alloca(0xc);          /* ten bytes of locals, and SI */
    /* The byte at [bp-1], as a pointer - the frame is the guest's because
       `read_resource` takes a DGROUP address; see `seek_to_sound_record`. */
    uint8_t *b = dg_near_ptr((uint16_t)(fp + 0xc - 1));
    struct sound_node *head = SOUND_NODE_NONE;
    struct sound_node *node;

    read_resource(handle, b, 1);

    for (;;) {
        if (*b == 0xff)
            break;

        node = (struct sound_node *)(void *)alloc_for_kind(sizeof(struct sound_node), 9);
        if (node == SOUND_NODE_NONE)
            break;

        node->next = NULL;

        resource_seek(handle, 1, 1);
        read_resource(handle, (uint8_t *)node, 4);
        read_resource(handle, b, 1);

        if (head == SOUND_NODE_NONE)
            head = node;
        else
            head = insert_by_key(head, node);
    }

    if (*b != 0xff)
        free_node_list(head);

    dg_free(0xc);
    return head;
}

/*
 * 0x28ddb
 *
 * Insert a node into a list kept in ascending order of the word at its +0. The
 * link is at +4, as a far pointer, and the answer is the head - which changes
 * only when the new node goes in front of it.
 *
 * An empty list is answered unchanged: the routine has nowhere to put the node
 * and does not make it the head. Whether that is deliberate or an oversight is
 * not established; it is transcribed as it stands.
 *
 * The walk keeps `prev` and `cur` a step apart and stops at the first node
 * whose key is not below the new one, so equal keys go **after** the ones
 * already there.
 */
struct sound_node far *insert_by_key(struct sound_node far * head,
                                     struct sound_node far * node)
{
    struct sound_node *cur, *prev;
    uint16_t key = node->key;

    if (head == SOUND_NODE_NONE)
        return head;

    /* The links are filed as each node's own pair - a DOS block starting a
       segment - so `far_of` files what the original does. */
    if (head->key >= key) {
        node->next = head;
        return node;
    }

    cur = prev = head;

    for (;;) {
        prev = cur;
        cur = cur->next;

        if (cur == SOUND_NODE_NONE)
            break;
        if (cur->key >= key)
            break;
    }

    node->next = cur;
    prev->next = node;

    return head;
}

/*
 * 0x28e87
 *
 * Gather the items a node list names into one block: a small directory at the
 * front and the items themselves behind it.
 *
 * The block opens the way `seek_to_sound_record` expects to find it - 0x84, a
 * zero, and a tag byte from the caller - and then carries six bytes per node:
 * two zeros, the item's offset within the block **less two**, and its length.
 * A 0xffff ends the directory.
 *
 * The items go to a second cursor that starts the caller's given distance into
 * the block, so the directory and the data grow towards each other from known
 * ends rather than being sized first.
 *
 * Each item is read by seeking the resource to the node's own offset plus two -
 * from the start, not from where the last read left off - and reading its
 * length. A short read abandons the whole thing and answers 0.
 */
uint16_t build_sound_index(int16_t handle, const struct sound_node far * list,
                           uint8_t far * dst, uint16_t data_at, uint16_t tag)
{
    /* The original steps two offsets inside `dst`'s segment; the block is
       sized in a word, so neither can leave it, and they are pointers. */
    uint8_t *dir = dst;
    uint8_t *data = dst + data_at;

    *dir++ = 0x84;
    *dir++ = 0;
    *dir++ = (uint8_t)tag;

    while (list != SOUND_NODE_NONE) {
        uint16_t len = list->length;

        dir[0] = 0;
        dir[1] = 0;
        *(uint16_t *)(dir + 2) = (uint16_t)(data - dst - 2);
        *(uint16_t *)(dir + 4) = len;

        resource_seek(handle, (uint16_t)(list->key + 2), 0);

        if ((uint16_t)read_resource(handle, data, len) != len)
            return 0;

        data += len;
        list = list->next;
        dir += 6;
    }

    *(uint16_t *)dir = 0xffff;
    return 1;
}

/*
 * 0x28f74
 *
 * Load a whole resource into a fresh block and answer it as a far pointer, or
 * null.
 *
 * The resource is opened under the name at DGROUP 0x4a80, its size asked for,
 * a block of exactly that size allocated of the caller's kind, and the whole
 * thing read in. A short read - or any size at all in the high half - frees the
 * block and answers null, so a partial resource is never handed back.
 *
 * The resource is closed on every path that opened it, including the failures.
 *
 * The optional pointer in the fourth argument is filled with the size, but only
 * when there is a block to go with it.
 */
uint8_t far *load_resource_block(FILE *file, uint32_t size,
                                 uint8_t * out, uint16_t kind)
{
    uint8_t *buf = FAR_NULL_PTR;
    uint32_t len = 0;
    int16_t handle;

    handle = open_resource(0, file, SOUND_TAGS.mode_r_b, size);

    if (handle >= 0) {
        int32_t sz = resource_size(handle);

        len = sz;

        buf = alloc_for_kind(sz, kind);

        if (buf != FAR_NULL_PTR) {
            uint16_t got = (uint16_t)read_resource(handle, buf, (uint16_t)len);

            /* `len_hi != 0` was "the size does not fit in a word". */
            if (len > 0xffff || got != (uint16_t)len) {
                free_for_kind(buf, kind);
                buf = FAR_NULL_PTR;
            }
        }

        close_resource(handle);
    }

    if (out != NULL && buf != FAR_NULL_PTR) {
        *(int16_t *)(out + 2) = (int16_t)(len >> 16);
        *(int16_t *)(out) = (int16_t)len;
    }

    return buf;
}

/*
 * 0x29034
 *
 * Load a sequence and start it: follow the chain of far pointers to the record,
 * set its default volume, and hand it to `start_sequence`.
 *
 * The far pointer that comes back is written **into the caller's own first two
 * argument words** before anything else uses it, so those arguments are both
 * input and output - the caller sees the located record even though the value
 * is also returned in DX:AX.
 *
 * A null result is answered as a null far pointer without touching anything
 * else. Otherwise +0x15e takes the fourth argument, which is the byte
 * `sequencer_tick` feeds to `scale_byte_pair` as the sequence's own volume, and
 * the sequence is started with the flag set - so `start_sequence` will write 2
 * to +0x159 and mark every channel as needing its own voice.
 */
struct sequence far *load_and_start_sequence(struct sequence far * seq, int16_t count,
                                             uint16_t volume)
{
    struct sequence *r = follow_far_chain(seq, count);

    if (r == SEQUENCE_NONE)
        return SEQUENCE_NONE;

    r->volume = (uint8_t)volume;

    start_sequence_far(r, 1);

    return r;
}

/*
 * 0x2907b
 *
 * Follow a chain of **far** pointers - offset at +0x172, segment at +0x174 -
 * for at most `count` links, stopping early on a null pointer.
 *
 * The original walks by overwriting its own stack arguments, and tests the
 * pointer for null by OR-ing the two halves together, which is how a far
 * pointer is compared with zero without two compares. It answers the pointer
 * it stopped on, in DX:AX.
 */
struct sequence far *follow_far_chain(struct sequence far * seq, int16_t count)
{
    for (;;) {
        if (seq == SEQUENCE_NONE)
            break;
        if (count == 0)
            break;
        seq = seq->next;
        count--;
    }
    return seq;
}

/*
 * 0x290ab
 *
 * Stop whichever voice is playing a given sequence. The same seven-entry table
 * at DGROUP 0x6414 that `voice_playing` searches, and the same match on the far
 * pointer at +0x166 - but this one does not test +0x158 first, so a voice
 * already marked stopped is retired and marked again.
 *
 * It returns after the first match: nothing here handles a second voice on the
 * same sequence, which is the assumption that a sequence has one.
 */
void stop_voice_playing(const uint8_t far * source)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        struct sequence *v = SOUND_VOICES.voice[i];

        /* Which note data this voice is playing - see `voice_playing`. */
        if ((const uint8_t *)v->source != source)
            continue;

        retire_and_tick_far(v);
        v->state = 0xff;
        return;
    }
}

/*
 * 0x29106
 *
 * Give the seven voice records back to the allocator, as kind 2.
 *
 * The whole table is skipped when its **first** entry is null, and the answer
 * is 0 rather than 1 - so an uninitialised table is reported as a failure
 * rather than as nothing to do. Each entry is then tested again inside the
 * loop, which is what makes a hole in the middle harmless.
 *
 * The table itself is not cleared. What clears it is not this routine.
 */
uint16_t free_voice_records(void)
{
    int16_t i;

    if (SOUND_VOICES.voice[0] == NULL)
        return 0;

    for (i = 0; i < 7; i++) {
        struct sequence *v = SOUND_VOICES.voice[i];

        if (v == SEQUENCE_NONE)
            continue;
        free_for_kind((uint8_t far *)v, 2);
    }

    return 1;
}

/*
 * 0x29152
 *
 * Give a sequence to the first free voice and start it.
 *
 * Free means 0xff at +0x158 - the same mark `stop_all_voices` writes. The voice
 * then remembers the sequence twice: the pointer it was given, at +0x166, and
 * the record **after** it, at +0x16a, which is where playing begins.
 *
 * Three bytes of per-voice state are set from one of two places. When the table
 * at DGROUP 0x4a92 exists, +0x15d and +0x15c come out of it as a pair - two
 * bytes per index - and +0x15e is 0x7f. When it does not, the three come from
 * the caller instead: +0x15d from the third argument, +0x15c is 1, and +0x15e
 * is the index itself. So the table, when present, overrides what the caller
 * asked for.
 *
 * Answers the voice as a far pointer, or 0 if the sequence was null or every
 * voice was busy.
 */
struct sequence far *start_on_free_voice(const uint8_t far * source, uint16_t index,
                                         uint16_t byte_arg)
{
    int16_t i;

    if (source == FAR_NULL_PTR)
        return SEQUENCE_NONE;

    for (i = 0; i < 7; i++) {
        struct sequence *voice = SOUND_VOICES.voice[i];

        if (voice->state != 0xff)
            continue;

        /* Which note data this voice is playing, and how far into it - the
           second a segment beside the offset `advance_record` stepped. */
        voice->source = source;
        voice->cursor = (uint8_t far *)advance_record(source);

        if (DG4A82.bank_ptr != 0) {
            const struct sound_bank_entry *bank =
                (const struct sound_bank_entry *)dg_near_ptr(DG4A82.bank_ptr);

            voice->loop = bank[index].loop;
            voice->priority = bank[index].priority;
            voice->volume = 0x7f;
        } else {
            voice->loop = (uint8_t)byte_arg;
            voice->priority = 1;
            voice->volume = (uint8_t)index;
        }

        start_sequence_far(voice, 0);
        return voice;
    }

    return SEQUENCE_NONE;
}

/*
 * 0x2923d
 *
 * Retire every voice that is still marked as playing. The seven-entry table at
 * DGROUP 0x6414 again, the 0xff at +0x158 as the mark, and
 * `retire_and_tick_far` as the retirement - the same three pieces as
 * `stop_voice_playing`, over all of them rather than one.
 */
void stop_all_voices(void)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        struct sequence *v = SOUND_VOICES.voice[i];

        if (v->state == 0xff)
            continue;

        retire_and_tick_far(v);
        v->state = 0xff;
    }
}

/*
 * 0x2928c
 *
 * Install the host callback: a far pointer written into this module's own code
 * segment at `cs:0x30f6`, which is the cell `sound_callback` calls through.
 *
 * `AX` is pushed and popped around the two stores, so the caller's `AX`
 * survives - the routine has no return value of its own.
 */
void set_sound_callback(const uint8_t far * cb)
{
    SNDCALL.callback = far_of(cb);
}

/*
 * 0x292a1
 *
 * Call the host's sound callback, if one is installed, and answer what it
 * returned.
 *
 * The vector is the far pointer at the module's `cs:0x30f6` and it is only
 * called when the word at DGROUP 0x4aaa says a callback exists. With none
 * installed the routine still answers - AX is untouched from entry, so the
 * caller gets back whatever it passed in.
 *
 * The answer is parked at `cs:0x30fa` before the registers are popped and read
 * back afterwards, because the pops would otherwise destroy it. That is why a
 * routine that appears to return AX has a global in the middle of it.
 *
 * Everything is saved, flags included, because a callback is arbitrary code.
 * The port takes only the register input: the two stack arguments are read
 * solely on the path that calls the callback, and calling an arbitrary guest
 * function pointer is not something the port can do.
 */
uint16_t sound_callback(uint16_t ax, union sound_module_args * si)
{
    /*
     * `mov ax, 0x2d3c` loads DS two instructions before the test, and the
     * branch that skips the call lands *after* it - so with no module the
     * answer is that constant, which is a **relocation**: the program's DGROUP
     * segment, not the 0x2d3c the bytes read.
     */
    uint16_t answer = DGROUP_SEG;

    if (((int16_t)DG4A82.module_live) != 0)
        answer = call_sound_module(ax, si);

    SNDCALL.answer = (int16_t)answer;
    return (uint16_t)SNDCALL.answer;
}

/*
 * 0x292f4
 *
 * Shut the sound down: silence the driver, let whatever is playing finish, and
 * give both blocks back.
 *
 * How it waits depends on whether the sequencer's timer callback is
 * registered - DGROUP 0x4a8e. With it registered the tick is running and
 * `delay_five_ticks` is enough; without it nothing is driving the sequencer, so
 * `sound_service` is called twice by hand instead.
 *
 * `silence_driver_far` is called with **no arguments at all**, which is safe
 * only because it reads none - the same dead argument 0x2846a has.
 *
 * The loaded module is told to stop through its own dispatcher at 0x0bbc6, a
 * call into a block that is not part of this binary. Not reached here, and left
 * as a stub.
 */
void stop_sound(void)
{
    if (dg_far_ptr(DG4A82.driver) != FAR_NULL_PTR) {
        silence_driver_far(FAR_NULL_PTR);

        if (((int16_t)DG4A82.tick_handle) == 0) {
            sound_service();
            sound_service();
        } else {
            delay_five_ticks();
        }
    }

    if (dg_far_ptr(DG4A82.module) != FAR_NULL_PTR) {
        stop_loaded_module();
    }

    if (dg_far_ptr(DG4A82.driver) != FAR_NULL_PTR) {
        free_for_kind(dg_far_ptr(DG4A82.driver), 1);
        DG4A82.driver = FAR_NULL;
    }

    if (dg_far_ptr(DG4A82.module) != FAR_NULL_PTR) {
        free_for_kind(dg_far_ptr(DG4A82.module), 1);
        DG4A82.module = FAR_NULL;
    }
}

/*
 * 0x2937f
 *
 * Wait five timer ticks. A counter at DGROUP 0x6430 is set to five, a callback
 * registered at four ticks a time, and the routine **spins** until the callback
 * has counted it down; then the slot is given back.
 *
 * The far pointer it registers is this module's own `cs:0x3228`, which is
 * `tick_delay` below.
 *
 * The spin only ends because the timer interrupt runs the callback, so in the
 * port it ends only when something drives the timer - the same standing as
 * `wait_and_latch_frame`. Nothing reaches it on these screens.
 */
void delay_five_ticks(void)
{
    uint16_t handle;

    SOUND_TICK_WAIT.ticks_left = 5;

    handle = timer_add_callback(tick_delay, 4);

    while (SOUND_TICK_WAIT.ticks_left > 0)
        ;

    timer_drop_callback(handle);
}

/*
 * 0x293b8
 *
 * The callback `delay_five_ticks` registers: one instruction of work, counting
 * DGROUP 0x6430 down by one each tick.
 */
void tick_delay(void)
{
    SOUND_TICK_WAIT.ticks_left = (int16_t)(((uint16_t)SOUND_TICK_WAIT.ticks_left) - 1);
}
