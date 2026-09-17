/*
 * Drive the developer build from outside, with a script.
 *
 * OURS, and not a transcription. It exists because every other way this
 * project has of reaching a screen is *blind*: `TIM_CLICK` places a click at a
 * page-flip number decided before the run starts, so reaching the editor means
 * knowing in advance which flip the picker appears on, and a click that lands
 * one screen early does nothing and reports nothing. Whole families of
 * routines - the part-list walks, the goal tests, everything behind a loaded
 * level - are marked "never called" in STATUS.md for exactly that reason.
 *
 * A script can look before it clicks. `TIM_LUA=<port>` opens a listener on
 * 127.0.0.1; whatever is sent to it is Lua, run inside the game process, with
 * the port's own input entry points bound as functions. The connection is a
 * line-at-a-time REPL, so a tool can drive the game one step at a time and
 * read state back between steps.
 *
 * **It is polled on the page flip**, from `dev_flip_dump`, which is the one
 * clock this project already trusts: `tools/capture.py`, `TIM_CLICK` and the
 * hybrid runner all count the same write to CRTC 0x0C, so "wait three flips"
 * means the same interval here as everywhere else. Nothing runs on another
 * thread, and a script therefore cannot see the game halfway through a frame.
 *
 * **`tim.wait` really waits.** A chunk runs inside a coroutine, so a script
 * that asks to wait yields back to the game and is resumed at the flip it
 * asked for. Without that a script could only ever poke at one instant, which
 * is the limitation `TIM_CLICK` already has.
 *
 * This file links into `devtim` alone. `libtim.so` - the library the verifier
 * calls - links the other dev*.c files, and a verification run has no business
 * carrying a socket, so `dev_flip_dump` reaches this through a **weak symbol**
 * and does nothing at all where it is absent.
 */
#define TIM_HOST 1        /* a host unit: the host's <stdio.h> and its FILE */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "io.h"
#include "tim.h"
#include "dgroup.h"

/*
 * The whole state. One client at a time: this drives a game, and two scripts
 * pressing the mouse at once would be a race with no reason to exist.
 */
static lua_State *L;
static int32_t listen_fd = -1;
static int32_t client_fd = -1;
static int32_t cur_flip;

/* The chunk that is running, if one is waiting for flips to pass. */
static lua_State *co;
static int32_t co_ref = LUA_NOREF;
static int32_t co_wait;           /* flips still to wait for */
static int32_t last_flip = -1;    /* the flip a wait last counted */

/* What has arrived and is not yet a whole line. */
static char inbuf[8192];
static size_t inlen;

static void reply(const char *fmt, ...)
{
    char msg[1024];
    va_list ap;
    int32_t n;

    va_start(ap, fmt);
    n = vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if ((size_t)n >= sizeof msg)
        n = (int32_t)sizeof msg - 1;
    if (client_fd >= 0) {
        ssize_t put = write(client_fd, msg, (size_t)n);

        (void)put;                /* a client that has gone is not an error */
    }
}

/*
 * The bindings. Everything a script can do to the game goes through the same
 * entry points `devdump.c` uses for `TIM_CLICK` and `TIM_KEY`, so a scripted
 * run and a flag-driven one reach the game identically.
 */
static int32_t l_flip(lua_State *s)
{
    lua_pushinteger(s, cur_flip);
    return 1;
}

static int32_t l_press(lua_State *s)
{
    io_mouse_input((int32_t)luaL_checkinteger(s, 1),
                   (int32_t)luaL_checkinteger(s, 2), 1);
    return 0;
}

static int32_t l_release(lua_State *s)
{
    io_mouse_input((int32_t)luaL_checkinteger(s, 1),
                   (int32_t)luaL_checkinteger(s, 2), 0);
    return 0;
}

static int32_t l_pointer(lua_State *s)
{
    io_mouse_input((int32_t)luaL_checkinteger(s, 1),
                   (int32_t)luaL_checkinteger(s, 2), 0);
    return 0;
}

/*
 * A scancode as the game reads it - the make on its own, and the break as a
 * second call with bit 7 set. `devdump.c` explains why both are needed: the
 * ISR sets a bit that `timer_callback` samples once a tick, so a make and a
 * break in the same instant leave nothing to sample.
 */
static int32_t l_scancode(lua_State *s)
{
    io_keyboard_scancode((uint8_t)luaL_checkinteger(s, 1));
    return 0;
}

/* Wait for `n` page flips, yielding the game back to itself meanwhile. */
static int32_t l_wait(lua_State *s)
{
    lua_Integer n = luaL_optinteger(s, 1, 1);

    co_wait = (int32_t)(n < 0 ? 0 : n);
    return lua_yield(s, 0);
}

/* DGROUP, read as bytes. This is what lets a script look before it clicks. */
static int32_t l_peek(lua_State *s)
{
    lua_Integer off = luaL_checkinteger(s, 1);
    lua_Integer len = luaL_optinteger(s, 2, 1);

    if (off < 0 || len < 0 || off + len > 0x10000)
        return luaL_error(s, "peek outside DGROUP: 0x%x+%d",
                          (unsigned)off, (int)len);
    lua_pushlstring(s, (const char *)dgroup + off, (size_t)len);
    return 1;
}

static int32_t l_peek8(lua_State *s)
{
    lua_Integer off = luaL_checkinteger(s, 1);

    if (off < 0 || off >= 0x10000)
        return luaL_error(s, "peek8 outside DGROUP: 0x%x", (unsigned)off);
    lua_pushinteger(s, dgroup[off]);
    return 1;
}

static int32_t l_peek16(lua_State *s)
{
    lua_Integer off = luaL_checkinteger(s, 1);

    if (off < 0 || off + 1 >= 0x10000)
        return luaL_error(s, "peek16 outside DGROUP: 0x%x", (unsigned)off);
    lua_pushinteger(s, (lua_Integer)*(uint16_t *)(dgroup + off));
    return 1;
}

static int32_t l_quit(lua_State *s)
{
    (void)s;
    /* `_exit` after a flush, for the reason `TIM_STOPFLIP`'s exit in
       devdump.c gives: `exit` would destroy the OPL emulator under the
       timer thread. */
    fflush(NULL);
    _exit(0);
}

static const luaL_Reg TIM_FNS[] = {
    { "flip",     l_flip },
    { "press",    l_press },
    { "release",  l_release },
    { "pointer",  l_pointer },
    { "scancode", l_scancode },
    { "wait",     l_wait },
    { "peek",     l_peek },
    { "peek8",    l_peek8 },
    { "peek16",   l_peek16 },
    { "quit",     l_quit },
    { NULL, NULL }
};

/*
 * The two compound actions are written in Lua rather than in C, because each
 * is a press, a wait and a release - and a C function cannot wait. The flip
 * counts are `devdump.c`'s: two flips for a click, four for a key, measured
 * there and not guessed here.
 */
static const char PRELUDE[] =
    "function tim.click(x, y)\n"
    "  tim.press(x, y); tim.wait(2); tim.release(x, y)\n"
    "end\n"
    "function tim.key(scan)\n"
    "  tim.scancode(scan); tim.wait(4); tim.scancode(scan + 0x80)\n"
    "end\n"
    "function tim.waitfor(fn, limit)\n"
    "  for _ = 1, limit or 600 do\n"
    "    local v = fn()\n"
    "    if v then return v end\n"
    "    tim.wait(1)\n"
    "  end\n"
    "  return nil\n"
    "end\n";

/*
 * The hybrid runner adds its own bindings - a breakpoint in the original's
 * code, which only it can offer - through this. Weak, so `devtim` links this
 * file with it absent and simply has the port's half.
 */
void native_lua_bind(lua_State *s) __attribute__((weak));

static void lua_start(void)
{
    L = luaL_newstate();
    luaL_openlibs(L);

    lua_newtable(L);
    luaL_setfuncs(L, (const luaL_Reg *)TIM_FNS, 0);
    lua_setglobal(L, "tim");

    if (native_lua_bind)
        native_lua_bind(L);

    if (luaL_dostring(L, PRELUDE) != LUA_OK) {
        fprintf(stderr, "lua: prelude failed: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void listen_start(int32_t port)
{
    struct sockaddr_in a;
    int32_t on = 1;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        fprintf(stderr, "lua: socket: %s\n", strerror(errno));
        return;
    }
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);

    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&a, sizeof a) < 0
        || listen(listen_fd, 1) < 0) {
        fprintf(stderr, "lua: listen on %d: %s\n", port, strerror(errno));
        close(listen_fd);
        listen_fd = -1;
        return;
    }
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);
    fprintf(stderr, "lua: listening on 127.0.0.1:%d\n", port);
}

/* Hand one chunk to Lua, in a coroutine so that `tim.wait` can yield. */
static void run_chunk(const char *src)
{
    int32_t nres;
    int32_t rc;

    co = lua_newthread(L);
    co_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    if (luaL_loadstring(co, src) != LUA_OK) {
        reply("error %s\n", lua_tostring(co, -1));
        goto done;
    }

    rc = lua_resume(co, NULL, 0, &nres);
    if (rc == LUA_YIELD)
        return;                   /* waiting; the flip hook resumes it */

    if (rc != LUA_OK)
        reply("error %s\n", lua_tostring(co, -1));
    else if (nres > 0)
        reply("ok %s\n", luaL_tolstring(co, -1, NULL));
    else
        reply("ok\n");

done:
    luaL_unref(L, LUA_REGISTRYINDEX, co_ref);
    co_ref = LUA_NOREF;
    co = NULL;
}

static void resume_chunk(void)
{
    int32_t nres;
    int32_t rc = lua_resume(co, NULL, 0, &nres);

    if (rc == LUA_YIELD)
        return;

    if (rc != LUA_OK)
        reply("error %s\n", lua_tostring(co, -1));
    else if (nres > 0)
        reply("ok %s\n", luaL_tolstring(co, -1, NULL));
    else
        reply("ok\n");

    luaL_unref(L, LUA_REGISTRYINDEX, co_ref);
    co_ref = LUA_NOREF;
    co = NULL;
}

/* Take whatever has arrived, and run one line if a whole one is there. */
static void poll_client(void)
{
    ssize_t n;
    char *nl;

    if (client_fd < 0) {
        client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0)
            return;
        fcntl(client_fd, F_SETFL, O_NONBLOCK);
        inlen = 0;
        reply("ready flip %d\n", cur_flip);
        return;
    }

    n = read(client_fd, inbuf + inlen, sizeof inbuf - inlen - 1);
    if (n == 0) {                 /* the client went away */
        close(client_fd);
        client_fd = -1;
        return;
    }
    if (n > 0)
        inlen += (size_t)n;
    inbuf[inlen] = '\0';

    nl = memchr(inbuf, '\n', inlen);
    if (nl == NULL)
        return;

    *nl = '\0';
    {
        char line[sizeof inbuf];
        size_t rest = inlen - (size_t)(nl + 1 - inbuf);

        snprintf(line, sizeof line, "%s", inbuf);
        memmove(inbuf, nl + 1, rest);
        inlen = rest;
        run_chunk(line);
    }
}

/*
 * The page-flip hook, called from `dev_flip_dump`. `TIM_LUA=<port>` turns it
 * on and nothing else does, so a run that does not ask for it pays one
 * comparison per flip.
 */
void dev_lua_flip(int32_t flip)
{
    static int32_t port = -2;

    cur_flip = flip;

    if (port == -2) {
        const char *spec = getenv("TIM_LUA");

        port = spec ? (int32_t)strtol(spec, NULL, 0) : -1;
        if (port > 0) {
            lua_start();
            listen_start(port);
        }
    }
    if (port <= 0 || listen_fd < 0)
        return;

    /*
     * A chunk that is waiting owns the connection until it is done.
     *
     * **The wait counts flips, not calls.** `devtim` calls this once per page
     * flip, but the hybrid runner calls it between emulation slices - hundreds
     * of times a frame - so counting calls made `tim.wait(120)` elapse in an
     * instant there and a script could never wait for anything. Counting the
     * flip number changing means the same interval on both sides.
     */
    if (co != NULL) {
        if (co_wait > 0) {
            if (flip != last_flip)
                co_wait--;
            last_flip = flip;
            return;
        }
        resume_chunk();
        return;
    }

    poll_client();
}
