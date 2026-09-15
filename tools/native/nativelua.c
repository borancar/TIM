/*
 * The hybrid runner's half of the scripting listener.
 *
 * OURS, and not a transcription. `reconstruct/devlua.c` holds the Lua state,
 * the socket and everything a script can do to the *port*; this adds what only
 * this runner can offer - a **breakpoint in the original's own code**.
 *
 * That is the question the port cannot answer. The port has no DS, no CS and
 * no guest stack: it is C. Ask "what is DS when the part-list walk runs" and
 * only the original can say, and the original runs here, under Unicorn, with
 * the port as its hardware. `tim.bp(0x14d71)` puts a hook on that image offset
 * and every time the guest executes it the registers are recorded.
 *
 * **Why not the shared emulator in tools/?** Because a run from the entry
 * point there never presents a page - STATUS.md records it as still open, and
 * two attempts in this session hit it again: no flips, so no clicks, so
 * nothing behind the menu is ever reached. The hybrid does run the game, which
 * is the whole reason it exists, so a breakpoint here can be reached by
 * playing to it.
 *
 * Addresses are **image offsets**, as everywhere else in this project, and are
 * turned into linear addresses with the load segment the runner used.
 */
#define TIM_HOST 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicorn/unicorn.h>
#include <lua.h>
#include <lauxlib.h>

#include "native.h"
#include "../../reconstruct/dgroup.h"   /* dgroup_base, to answer about DS */

#define BPS 32

struct bp {
    uint32_t off;                 /* image offset, as the listing numbers it */
    uint32_t hits;
    uint16_t ds, cs, ss, si, sp;  /* the last hit's registers */
    uc_hook hook;
    int32_t used;
};

static struct bp bps[BPS];
static uc_engine *bp_uc;
static uint32_t bp_base;          /* load_seg * 16 */

/* What the block hook has seen, so "no hits" can be told from "never called". */
static uint64_t blocks_seen;
static uint32_t blocks_last[4];

/*
 * The hook. It only records: stopping the guest from here would need the
 * script to be resumable inside an emulation slice, and what the question
 * needs is the registers, not control.
 */
static void on_bp(uc_engine *uc, uint64_t address, uint32_t size, void *ud)
{
    struct bp *b = (struct bp *)ud;
    uint32_t v;

    (void)address;
    (void)size;
    b->hits++;
    uc_reg_read(uc, UC_X86_REG_DS, &v); b->ds = (uint16_t)v;
    uc_reg_read(uc, UC_X86_REG_CS, &v); b->cs = (uint16_t)v;
    uc_reg_read(uc, UC_X86_REG_SS, &v); b->ss = (uint16_t)v;
    uc_reg_read(uc, UC_X86_REG_SI, &v); b->si = (uint16_t)v;
    uc_reg_read(uc, UC_X86_REG_SP, &v); b->sp = (uint16_t)v;
}

/* `tim.bp(imageoffset)` - watch that instruction. */
static int32_t l_bp(lua_State *s)
{
    lua_Integer off = luaL_checkinteger(s, 1);
    int32_t i;

    if (bp_uc == NULL)
        return luaL_error(s, "no guest yet");

    for (i = 0; i < BPS; i++)
        if (!bps[i].used)
            break;
    if (i == BPS)
        return luaL_error(s, "no room for another breakpoint");

    bps[i].used = 1;
    bps[i].off = (uint32_t)off;
    bps[i].hits = 0;
    {
        uc_err e = uc_hook_add(bp_uc, &bps[i].hook, UC_HOOK_CODE,
                               (void *)on_bp, &bps[i],
                               bp_base + (uint32_t)off,
                               bp_base + (uint32_t)off);

        /* Loudly: a hook that failed to install reads exactly like a
           breakpoint nothing reached. */
        if (e != UC_ERR_OK) {
            bps[i].used = 0;
            return luaL_error(s, "uc_hook_add at %05x: %s",
                              (unsigned)off, uc_strerror(e));
        }
    }

    /*
     * **And throw away the block that was already translated.** A hook added
     * after the emulator has compiled the block containing the address does
     * not apply to that block, so a breakpoint on code the intro had already
     * run silently saw nothing - measured, ten breakpoints and not one hit.
     */
    uc_ctl_remove_cache(bp_uc, bp_base + (uint32_t)off,
                        bp_base + (uint32_t)off + 1);
    /* And the whole cache, because instrumenting for a code hook is a
       property of the translated block, not of the byte. */
    uc_ctl_flush_tb(bp_uc);
    lua_pushinteger(s, i);
    return 1;
}

/* `tim.bps()` - what every breakpoint has seen, as one line. */
static int32_t l_bps(lua_State *s)
{
    luaL_Buffer b;
    int32_t i;
    char line[160];

    luaL_buffinit(s, &b);
    for (i = 0; i < BPS; i++) {
        if (!bps[i].used)
            continue;
        snprintf(line, sizeof line,
                 "%05x x%u DS=%04x CS=%04x SS=%04x SI=%04x SP=%04x; ",
                 bps[i].off, bps[i].hits, bps[i].ds, bps[i].cs, bps[i].ss,
                 bps[i].si, bps[i].sp);
        luaL_addstring(&b, line);
    }
    luaL_pushresult(&b);
    return 1;
}

/* `tim.blocks()` - what the block hook has seen, and the base offsets are
   measured against. It tells "nothing reached my breakpoint" apart from "my
   breakpoint was never consulted", which read identically until it existed. */
static int32_t l_blocks(lua_State *s)
{
    char out[160];

    /* `lua_pushfstring` takes only %d %s %f %p %c %U - not %x, which it
       answers by raising. The C library does the formatting instead. */
    snprintf(out, sizeof out, "n=%llu base=%05x last=%05x,%05x,%05x,%05x",
             (unsigned long long)blocks_seen, (unsigned)bp_base,
             (unsigned)blocks_last[0], (unsigned)blocks_last[1],
             (unsigned)blocks_last[2], (unsigned)blocks_last[3]);
    lua_pushstring(s, out);
    return 1;
}

/* `tim.dgroupseg()` - the guest's DGROUP as a segment, to compare DS against. */
static int32_t l_dgroupseg(lua_State *s)
{
    lua_pushinteger(s, (lua_Integer)(dgroup_base >> 4));
    return 1;
}

/*
 * Called by `reconstruct/devlua.c` when it builds its Lua state, through a
 * weak symbol - so the port's own `devtim` links devlua.c with this absent and
 * simply has no `tim.bp`.
 */
void native_lua_bind(lua_State *s)
{
    lua_getglobal(s, "tim");
    lua_pushcfunction(s, l_bp);        lua_setfield(s, -2, "bp");
    lua_pushcfunction(s, l_bps);       lua_setfield(s, -2, "bps");
    lua_pushcfunction(s, l_dgroupseg); lua_setfield(s, -2, "dgroupseg");
    lua_pushcfunction(s, l_blocks);    lua_setfield(s, -2, "blocks");
    lua_pop(s, 1);
}

/*
 * **The same watch, from the block hook this runner already has.** A
 * `UC_HOOK_CODE` added while the guest is running has to survive the
 * emulator's translation cache, and getting that wrong looks exactly like a
 * breakpoint nothing reached - ten of them read zero hits before this existed.
 * `on_block` in native.c is installed before the first instruction and fires
 * for every block, and a routine's entry and a loop's head are both block
 * starts, which is what a breakpoint here is ever put on.
 */
void native_lua_block(uc_engine *uc, uint32_t linear)
{
    int32_t i;
    uint32_t v;

    blocks_last[blocks_seen & 3] = linear;
    blocks_seen++;

    if (bp_uc == NULL)
        return;

    for (i = 0; i < BPS; i++) {
        if (!bps[i].used || bp_base + bps[i].off != linear)
            continue;
        bps[i].hits++;
        uc_reg_read(uc, UC_X86_REG_DS, &v); bps[i].ds = (uint16_t)v;
        uc_reg_read(uc, UC_X86_REG_CS, &v); bps[i].cs = (uint16_t)v;
        uc_reg_read(uc, UC_X86_REG_SS, &v); bps[i].ss = (uint16_t)v;
        uc_reg_read(uc, UC_X86_REG_SI, &v); bps[i].si = (uint16_t)v;
        uc_reg_read(uc, UC_X86_REG_SP, &v); bps[i].sp = (uint16_t)v;
    }
}

/* native.c hands the machine over once it exists. */
void native_lua_guest(uc_engine *uc, uint32_t load_seg)
{
    bp_uc = uc;
    bp_base = load_seg * 16;
}
