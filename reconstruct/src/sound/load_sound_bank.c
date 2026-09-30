/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Load a sound bank, and the index of records it is read through.**
 *
 * A module of the sound library, in 1.11 **code segment 2825** on its own,
 * image 0x28256..0x287d6. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

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
