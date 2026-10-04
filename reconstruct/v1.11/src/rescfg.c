/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **RESOURCE.CFG, read as text** - new in 1.11. The installer writes lines
 * of `key = value` in the format of Sierra's driver loader (`soundDrv =
 * SBPRO.DRV`, `audioDrv = AUDBLAST.DRV`, ...), and this turns the ones the
 * game understands into what it uses: the sound device and the digitised
 * module `start_sound` is given, and three flags nothing reads. 1.00 read
 * three bytes of the same file, in `game_startup`.
 *
 * In 1.11, image 0x0f0ef..0x0f322, between `gamemain.c`'s last routine and
 * `intro.c`'s first, with its two switch tables in the code after it to
 * 0x0f340. **A module of its own**: its data, DGROUP 0x00f8..0x0222, comes
 * after `gamemain.c`'s literal pool and before `gamedata.c`'s first object,
 * and a module's pool is the last of its data - so this is not
 * `gamemain.c`'s. `game_startup` calls it through TLINK's `nop / push cs /
 * call`. The file's name is ours.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x00f8..0x0222
 */
#include <string.h>
#ifndef __TURBOC__
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* DGROUP 0x00f8: 8, and no instruction names it. */
int16_t g_pad_00f8 = 8;
/* DGROUP 0x00fa: the digitised module `start_sound` is given - -2 for none. */
int16_t g_sound_module = -2;
/* DGROUP 0x00fc: the music device `start_sound` is given - 0, the speaker. */
int16_t g_sound_device = 0;
/* DGROUP 0x00fe..0x0100: whether RESOURCE.CFG named a memory driver, a
   mouse driver and a joystick driver the game accepts. Written here and
   read nowhere. */
uint8_t g_cfg_memory_drv = 0;
uint8_t g_cfg_mouse_drv = 0;
uint8_t g_cfg_joystick_drv = 0;
/* DGROUP 0x0101: two bytes nothing names. */
uint8_t g_pad_0101[2] = { 0, 0 };

/* DGROUP 0x0103: the keys, in the order `read_resource_cfg`'s switch takes
   them. */
char *g_cfg_keys[7] = {
    "VideoDrv", "SoundDrv", "AudioDrv", "JoyDrv", "KeyDrv", "MouseDrv",
    "MemoryDrv",
};

/* DGROUP 0x0111: the music drivers `SoundDrv` may name. */
char *g_cfg_sound_drivers[8] = {
    "STD.DRV", "TANDY.DRV", "ADL.DRV", "MT32.DRV", "SBPRO.DRV",
    "IBMPS1.DRV", "PROAUDIO.DRV", "GENMIDI.DRV",
};

/* DGROUP 0x0121: the digitised drivers `AudioDrv` may name, whose index is
   the module. */
char *g_cfg_audio_drivers[4] = {
    "AUDBLAST.DRV", "AUDPS1.DRV", "AUDTANDY.DRV", "AUDPRO.DRV",
};

/*
 * 0x0f0ef
 *
 * **Read RESOURCE.CFG.** Each line is cut at the first blank run into a key
 * and, after the `=` and any blanks, a value that ends at the next blank;
 * the key is looked up without regard to case, and the answer is how many
 * lines had a key the game knows.
 *
 * `SoundDrv` picks the music device and, for the drivers that have no
 * digitised half, sets the module to none (-2): the speaker (0), Tandy (1,
 * module 2), AdLib (2), MT-32 (3), Sound Blaster Pro (2, module 0), PS/1
 * (5, module 1), Pro Audio (6, module 3), General MIDI (7). `AudioDrv` sets
 * the module to the driver's index, 0..3. `JoyDrv`, `MouseDrv` and
 * `MemoryDrv` set the three flags; `VideoDrv` and `KeyDrv` are known and
 * change nothing.
 *
 * "Blank" is anything up to and including a space, compared unsigned.
 */
int16_t read_resource_cfg(void)
{
    FILE *file;                 /* [bp-2] */
    uint8_t *key;               /* [bp-4] */
    uint8_t *end;               /* [bp-6] */
    int16_t known;              /* [bp-8] */
    char line[0x50];            /* [bp-0x58] */
    register uint8_t *value;    /* si */
    register int16_t i;         /* di */

    known = 0;
    file = fopen("RESOURCE.CFG", "rt");
    if (file == NULL)
        return known;
    while (fgets(line, 0x50, file) != NULL) {
        for (key = (uint8_t *)line; *key != 0 && *key <= ' '; key++)
            ;
        for (value = key; *value != 0 && *value > ' '; value++)
            ;
        *value = 0;
        value++;
        while (*value != 0 && *value++ != '=')
            ;
        for (; *value != 0 && *value <= ' '; value++)
            ;
        for (end = value; *end != 0 && *end > ' '; end++)
            ;
        *end = 0;

        for (i = 0; i < 7 && stricmp(g_cfg_keys[i], (char *)key) != 0; i++)
            ;

        known++;
        switch (i) {
        case 0:                                     /* VideoDrv */
            break;
        case 1:                                     /* SoundDrv */
            for (i = 0; i < 8
                        && stricmp(g_cfg_sound_drivers[i], (char *)value) != 0; i++)
                ;
            switch (i) {
            case 0:
                g_sound_module = -2;
                g_sound_device = 0;
                break;
            case 1:
                g_sound_module = 2;
                g_sound_device = 1;
                break;
            case 2:
                g_sound_module = -2;
                g_sound_device = 2;
                break;
            case 3:
                g_sound_module = -2;
                g_sound_device = 3;
                break;
            case 4:
                g_sound_module = 0;
                g_sound_device = 2;
                break;
            case 5:
                g_sound_module = 1;
                g_sound_device = 5;
                break;
            case 6:
                g_sound_module = 3;
                g_sound_device = 6;
                break;
            case 7:
                g_sound_module = -2;
                g_sound_device = 7;
                break;
            }
            break;
        case 2:                                     /* AudioDrv */
            for (i = 0; i < 4
                        && stricmp(g_cfg_audio_drivers[i], (char *)value) != 0; i++)
                ;
            if (i < 4) {
                g_sound_module = i;
                break;
            }
            break;
        case 3:                                     /* JoyDrv */
            g_cfg_joystick_drv = !stricmp((char *)value, "JOYSTICK.DRV");
            break;
        case 4:                                     /* KeyDrv */
            break;
        case 5:                                     /* MouseDrv */
            g_cfg_mouse_drv = (stricmp((char *)value, "STDMOUSE.DRV") == 0
                               || stricmp((char *)value, "NONE") == 0);
            break;
        case 6:                                     /* MemoryDrv */
            g_cfg_memory_drv = (stricmp((char *)value, "ARM.DRV") == 0
                                || stricmp((char *)value, "NONE") == 0);
            break;
        default:
            known--;
            break;
        }
    }
    fclose(file);
    return known;
}
