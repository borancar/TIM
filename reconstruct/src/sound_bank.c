/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Sequences, sound banks and the record index.**
 *
 * The fourth module of the original's **code segment 2619**, image
 * 0x28935..0x2928c - the third of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 * JUDGE: data 0x4a7e..0x4a82
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x281d1
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
    struct sequence far *seq;

    if ((seq = (struct sequence far *)alloc_for_kind(sizeof(struct sequence), 2))
        != NULL) {
        /* `cursor` and `cursor_at` are stepped inside their blocks' segments:
           `advance_record` and the `+ 0x16a` move the offset alone. */
        seq->source = src;
        seq->cursor = (uint8_t far *)advance_record(src);
        seq->cursor_at = &seq->cursor;

        seq->volume = 0x7f;
        seq->next = 0;

        return seq;
    }

    return NULL;
}

/*
 * 0x289ba (1.00's; not yet placed in 1.11)
 *
 * Follow a chain to its end and, if anything is there, retire and tick.
 *
 * The original writes `follow_far_chain`'s answer back over its own stack
 * arguments before testing it, which is a compiler reusing the incoming slots
 * as a local and not a second meaning for them.
 */
void follow_then_tick(struct sequence far * seq, int16_t count)
{
    if ((seq = follow_far_chain(seq, count)) != NULL)
        retire_and_tick_far(seq);
}

/*
 * 0x28256
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
                         uint8_t * out, uint16_t kind)
{
    struct sound_node far *walk;
    uint8_t want;
    uint32_t len;
    struct sound_node far *list = NULL;
    uint8_t far *blk = NULL;
    int16_t handle;
    uint16_t dir;

    switch (g_sound_bank.device) {
    case 0:
        want = 0x12;
        break;
    case 1:
    case 5:
        want = 0x13;
        break;
    case 2:
    case 6:
        want = 0;
        break;
    case 3:
        want = 0xc;
        break;
    case 0x7e:
        want = (uint8_t)g_sound_bank.identifier;
        break;

    /*
     * The original **falls through here**, and so does this: the store is
     * dead, `default` answers null, and `GMD:` can never load a sound bank -
     * a bug in Dynamix's code, transcribed as it behaves.
     */
    case 7:
        want = 7;
        /* falls through */
    default:
        return NULL;
    }

    if ((handle = open_resource(0, file, "r", size)) < 0)
        goto done;

    if (seek_to_sound_record(handle, want) == 0)
        goto missing;

    if ((list = read_sound_records(handle)) == NULL)
        goto missing;

    /* Six bytes of directory per node plus five, rounded up to even and to
       at least 0x26; the items' own lengths on top. */
    walk = list;
    len = 0;
    dir = 5;
    while (walk != NULL) {
        len += walk->length;
        dir += 6;
        walk = walk->next;
    }

    if ((dir & 1) != 0)
        dir++;
    dir = dir >= 0x26 ? dir : 0x26;

    len += dir;

    /* `kind` is its caller's, `read_record` passing the same four words it
       passes `load_resource_block`; nothing here reads it. */
    (void)kind;

    if ((blk = alloc_for_kind(len + 1, 4)) != NULL
        && build_sound_index(handle, list, blk, dir, want) != 0) {
        free_node_list(list);

        if (out != NULL)
            *(uint32_t *)out = len;

        close_resource(handle);
        return blk;
    }
    goto close;

missing:
    g_sound_bank.load_error = 2;
close:
    close_resource(handle);
done:
    free_node_list(list);
    return NULL;
}

/*
 * 0x2841b
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
    struct sound_node far *cur;

    while (list != NULL) {
        cur = list;
        list = list->next;
        free_for_kind((uint8_t far *)cur, 9);
    }
}

/*
 * 0x2845e
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
 * `read_resource` as `SS:offset` - which in this program is a DGROUP address.
 *
 * The name is a guess from the shape; what the records are is not established
 * here.
 */
uint16_t seek_to_sound_record(int16_t handle, uint8_t want)
{
    uint8_t id;                         /* [bp-1] */
    uint8_t skip;                       /* [bp-2] */
    uint8_t tag;                        /* [bp-3] */

    if (read_resource(handle, &tag, 1) != 1)
        return 0;
    if (tag != 0x84)
        return 0;
    if (read_resource(handle, &tag, 1) != 1)
        return 0;
    if (read_resource(handle, &id, 1) != 1)
        return 0;

    while (id != want) {
        if (id == 0xff || read_resource(handle, &skip, 1) != 1)
            return 0;

        while (skip != 0xff) {
            resource_seek(handle, 5L, 1);
            if (read_resource(handle, &skip, 1) != 1)
                return 0;
        }

        if (read_resource(handle, &id, 1) != 1)
            return 0;
    }

    return 1;
}

/*
 * 0x2855f
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
    uint8_t id;                         /* [bp-1] */
    struct sound_node far *head = NULL;
    struct sound_node far *node;

    read_resource(handle, &id, 1);

    while (id != 0xff
           && (node = (struct sound_node far *)
                   alloc_for_kind(sizeof(struct sound_node), 9))
              != NULL) {
        node->next = 0;

        resource_seek(handle, 1L, 1);
        read_resource(handle, (uint8_t far *)node, 4);
        read_resource(handle, &id, 1);

        if (head == NULL)
            head = node;
        else
            head = insert_by_key(head, node);
    }

    /* A list cut short by a failed allocation is freed, and then answered
       all the same. */
    if (id != 0xff)
        free_node_list(head);

    return head;
}

/*
 * 0x28643
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
    struct sound_node far *cur;
    struct sound_node far *prev;

    if (head != NULL) {
        if (head->key >= node->key) {
            node->next = head;
            head = node;
        } else {
            prev = cur = head;
            do {
                prev = cur;
                cur = cur->next;
            } while (cur != NULL && cur->key < node->key);

            node->next = cur;
            prev->next = node;
        }
    }

    return head;
}

/*
 * 0x286ef
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
    int32_t pos;                        /* kept, and never read */
    uint8_t far *data;
    uint8_t far *dir;

    /* Two cursors stepped inside `dst`'s segment; the block is sized in a
       word, so neither can leave it. */
    data = dir = dst;
    data += data_at;

    *dir++ = 0x84;
    *dir++ = 0;
    *dir++ = (uint8_t)tag;

    while (list != NULL) {
        dir[0] = 0;
        dir[1] = 0;
        *(uint16_t far *)(dir + 2) = (uint16_t)(data - dst - 2);
        *(uint16_t far *)(dir + 4) = list->length;

        pos = resource_seek(handle, (uint16_t)(list->key + 2), 0);

        if (read_resource(handle, data, list->length) != list->length)
            return 0;

        data += list->length;
        list = list->next;
        dir += 6;
    }

    *(uint16_t far *)dir = 0xffff;
    (void)pos;
    return 1;
}

/*
 * 0x2ba6a
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
    uint32_t len;
    uint8_t far *buf = NULL;
    int16_t handle;

    if ((handle = open_resource(0, file, "r", size)) >= 0) {
        len = resource_size(handle);

        if ((buf = alloc_for_kind(len, kind)) != NULL) {
            /* A size that does not fit in a word never compares equal. */
            if ((uint16_t)read_resource(handle, buf, (uint16_t)len) != len) {
                free_for_kind(buf, kind);
                buf = NULL;
            }
        }

        close_resource(handle);
    }

    if (out != NULL && buf != NULL)
        *(uint32_t *)out = len;

    return buf;
}

/*
 * 0x29034 (1.00's; not yet placed in 1.11)
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
    if ((seq = follow_far_chain(seq, count)) != NULL) {
        seq->volume = (uint8_t)volume;
        start_sequence_far(seq, 1);
        return seq;
    }

    return NULL;
}

/*
 * 0x2b8b4
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
    while (seq != NULL && count != 0) {
        seq = seq->next;
        count--;
    }
    return seq;
}

/*
 * 0x2b929
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
    struct sequence far *v;

    for (i = 0; i < 7; i++) {
        v = g_sound_voice[i];

        /* Which note data this voice is playing - see `voice_playing`. */
        if (v->source == source) {
            retire_and_tick_far(v);
            v->state = 0xff;
            return;
        }
    }
}

/*
 * 0x29106 (1.00's; not yet placed in 1.11)
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

    if (g_sound_voice[0] != NULL) {
        for (i = 0; i < 7; i++) {
            if (g_sound_voice[i] != NULL)
                free_for_kind((uint8_t far *)g_sound_voice[i], 2);
        }
        return 1;
    }

    return 0;
}

/*
 * 0x2b982
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
                                         uint8_t byte_arg)
{
    struct sequence far *voice = NULL;
    int16_t i;

    if (source != NULL) {
        for (i = 0; i < 7; i++) {
            voice = g_sound_voice[i];

            if (voice->state == 0xff) {
                /* Which note data this voice is playing, and how far into
                   it - the second a segment beside the offset
                   `advance_record` stepped. */
                voice->source = source;
                voice->cursor = (uint8_t far *)advance_record(source);

                if (g_sound_bank.bank != 0) {
                    voice->loop = g_sound_bank.bank[index].loop;
                    voice->priority = g_sound_bank.bank[index].priority;
                    voice->volume = 0x7f;
                } else {
                    voice->loop = (uint8_t)byte_arg;
                    voice->priority = 1;
                    voice->volume = (uint8_t)index;
                }

                start_sequence_far(voice, 0);
                return voice;
            }
        }
    }

    return NULL;
}

/*
 * 0x2923d (1.00's; not yet placed in 1.11)
 *
 * Retire every voice that is still marked as playing. The seven-entry table at
 * DGROUP 0x6414 again, the 0xff at +0x158 as the mark, and
 * `retire_and_tick_far` as the retirement - the same three pieces as
 * `stop_voice_playing`, over all of them rather than one.
 *
 * It is called before the voices are allocated, when every entry is a far
 * null, and then it reads and writes the interrupt table's byte at 0000:0158
 * - the first pass marks it 0xff and the other six find it so. `ZERO_PAGE`
 * gives the host the same bytes.
 */
void stop_all_voices(void)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        if (ZERO_PAGE(g_sound_voice[i])->state != 0xff) {
            retire_and_tick_far(ZERO_PAGE(g_sound_voice[i]));
            ZERO_PAGE(g_sound_voice[i])->state = 0xff;
        }
    }
}

