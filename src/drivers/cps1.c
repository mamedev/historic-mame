/***************************************************************************

  Capcom System 1
  ===============

  Driver provided by:
        Paul Leaman (paull@vortexcomputing.demon.co.uk)

  If you want to add any new drivers, or want to make any major changes it
  would be worth contacting me beforehand. This code is still being
  worked on.

  Please do not send any large emails without asking first.

  M680000 for game, Z80, YM-2151 and OKIM6295 for sound.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/2151intf.h"
#include "sndhrdw/adpcm.h"

#include "cps1.h"       /* External CPS1 defintions */

/* Please include this credit for the CPS1 driver */
#define CPS1_CREDITS "Capcom System 1\n"\
                     "===============\n\n"\
                     "Paul Leaman\n"\
                     "Phil Stroffolino\n"\
                     "Aaron Giles (additional info)\n"\
                     "\n"\
                     "Game driver\n"\
                     "===========\n"

/********************************************************************
*
*  Sound board
*
*********************************************************************/
static void cps1_irq_handler_mus (void) {  cpu_cause_interrupt (0, 0); }
static struct YM2151interface interface_mus =
{
        1,                      /* 1 chip  ? */
	4000000,		/* 3.58 MHZ ? */
        { 170 },                /* 170 somethings */
        { cps1_irq_handler_mus }
};

static struct OKIM6295interface interface_okim6295 =
{
        1,              /* 1 chip?? */
	8000,           /* 8000Hz frequency */
	3,              /* memory region 3 */
	{ 255 }
};

void cps1_snd_bankswitch_w(int offset,int data)
{
        cpu_setbank (1, &RAM[0x10000+(data&0x01)*0x4000]);
}

static struct MemoryReadAddress sound_readmem[] =
{
        { 0xf001, 0xf001, YM2151_status_port_0_r },
        { 0xf002, 0xf002, OKIM6295_status_r },
        { 0xf008, 0xf008, soundlatch_r },  /* Sound board input */
        { 0xf00a, 0xf00a, MRA_NOP },       /* ???? Unknown input ???? */
        { 0xd000, 0xd7ff, MRA_RAM },
        { 0x8000, 0xbfff, MRA_BANK1 },
        { 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

void cps1_sound_cmd_w(int offset, int data)
{
        if (offset == 0) { soundlatch_w(offset, data); }
}

/* There are sound fade timers somewhere amongst this lot */
static struct MemoryWriteAddress sound_writemem[] =
{
        { 0xd000, 0xd7ff, MWA_RAM },
        { 0xf000, 0xf000, YM2151_register_port_0_w },
        { 0xf001, 0xf001, YM2151_data_port_0_w },
        { 0xf002, 0xf002, OKIM6295_data_w },
        { 0xf004, 0xf004, cps1_snd_bankswitch_w },
        { 0xf006, 0xf006, MWA_NOP }, /* ???? Unknown (??fade timer??) ???? */
        { 0x0000, 0xcfff, MWA_ROM },
	{ -1 }	/* end of table */
};

/********************************************************************
*
*  Machine Driver macro
*  ====================
*
*  Abusing the pre-processor.
*
********************************************************************/

#define MACHINE_DRIVER(CPS1_DRVNAME, CPS1_RDMEM, CPS1_WRMEM, CPS1_IRQ, CPS1_GFX) \
static struct MachineDriver CPS1_DRVNAME =                             \
{                                                                        \
        /* basic machine hardware */                                     \
        {                                                                \
                {                                                        \
                        CPU_M68000,                                      \
                        8000000,                /* 8MHz ? */             \
                        0,                                               \
                        CPS1_RDMEM,CPS1_WRMEM,0,0,                  \
                        CPS1_IRQ, 2   /* 2 interrupts per frame */       \
                },                                                       \
                {                                                        \
                        CPU_Z80 | CPU_AUDIO_CPU,                         \
                        3000000,        /* 3 Mhz ??? */                  \
                        2,      /* memory region #2 */                   \
                        sound_readmem,sound_writemem,0,0,                \
                        interrupt,4                                      \
                }                                                        \
        },                                                               \
        60, 2500,                                                       \
        1,                                                               \
        cps1_init_machine,                                              \
                                                                         \
        /* video hardware */                                             \
        0x30*8, 0x1e*8, { 0*8, 0x30*8-1, 2*8, 0x1e*8-1 },                    \
                                                                         \
        CPS1_GFX,                                                  \
        256,                       /* 256 colours */                     \
        32*16+32*16+32*16+32*16,   /* Colour table length */             \
        0,                                                               \
                                                                         \
        VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,                        \
        0,                                                               \
        cps1_vh_start,                                                   \
        cps1_vh_stop,                                                    \
        cps1_vh_screenrefresh,                                           \
                                                                         \
        /* sound hardware */                                             \
        cps1_sh_init,0,0,0,                                              \
        { { SOUND_YM2151,  &interface_mus },                            \
          { SOUND_OKIM6295,  &interface_okim6295 }                    \
        }                            \
};

#define SPRITE_SEP2 0x080000*8
#define TILE_SEP2   0x100000*8

#define SPRITE_LAYOUT(LAYOUT, SPRITES) \
static struct GfxLayout LAYOUT = \
{                                               \
        16,16,   /* 16*16 sprites */             \
        SPRITES,  /* ???? sprites */            \
        4,       /* 4 bits per pixel */            \
        { 0x100000*8+8,0x100000*8,8,0 },            \
        { SPRITE_SEP2+0,SPRITE_SEP2+1,SPRITE_SEP2+2,SPRITE_SEP2+3, \
          SPRITE_SEP2+4,SPRITE_SEP2+5,SPRITE_SEP2+6,SPRITE_SEP2+7,  \
          0,1,2,3,4,5,6,7, },\
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8, \
           16*8, 18*8, 20*8, 22*8, 24*8, 26*8, 28*8, 30*8, }, \
        32*8    /* every sprite takes 32*8*2 consecutive bytes */ \
};

#define CHAR_LAYOUT(LAYOUT, CHARS) \
static struct GfxLayout LAYOUT =        \
{                                        \
        8,8,    /* 8*8 chars */             \
        CHARS,  /* ???? chars */        \
        4,       /* 4 bits per pixel */     \
        { 0x100000*8+8,0x100000*8,8,0 },     \
        { 0,1,2,3,4,5,6,7, },                         \
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8,}, \
        16*8    /* every sprite takes 32*8*2 consecutive bytes */\
};

#define TILE32_LAYOUT(LAYOUT, TILES, SEP) \
static struct GfxLayout LAYOUT =                                   \
{                                                                  \
        32,32,   /* 32*32 tiles */                                 \
        TILES,   /* ????  tiles */                                 \
        4,       /* 4 bits per pixel */                            \
        { 0x100000*8+8,0x100000*8,8,0},                                        \
        {                                                          \
           SEP+0,SEP+1,SEP+2,SEP+3, SEP+4,SEP+5,SEP+6,SEP+7,       \
           0,1,2,3,4,5,6,7,                                        \
           16+SEP+0,16+SEP+1,16+SEP+2,                             \
           16+SEP+3,16+SEP+4,16+SEP+5,                             \
           16+SEP+6,16+SEP+7,                                      \
           16+0,16+1,16+2,16+3,16+4,16+5,16+6,16+7                 \
        },                                                         \
        {                                                          \
           0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,         \
           8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,   \
           16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32, \
           24*32, 25*32, 26*32, 27*32, 28*32, 29*32, 30*32, 31*32  \
        },                                                         \
        4*32*8    /* every sprite takes 32*8*4 consecutive bytes */\
};

#define TILE_LAYOUT2(LAYOUT, TILES, SEP, BITSEP) \
static struct GfxLayout LAYOUT =        \
{                                        \
        16,16,    /* 16*16 tiles */             \
        TILES,    /* ???? tiles */        \
        4,       /* 4 bits per pixel */     \
        { 3*BITSEP*8, 2*BITSEP*8, BITSEP*8, 0,  },     \
        { SEP+0, SEP+1,SEP+2, SEP+3,SEP+4,SEP+5,SEP+6,SEP+7,\
        0,1,2,3,4,5,6,7, },\
        {0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,\
         8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8  },\
        16*8    /* every sprite takes 16*8 consecutive bytes */\
};

static struct CPS1config cps1_config_table[]=
{
        { "strider",     0,  0, 2*4096,      0,   0x00000000, 0 },
        { "ffight",    1024,  0, 2*4096,   512,   0x00000000, 2 },
        { "mtwins",      0,   0,      2*4096, 0x0e00,   0x00000000, 3 },
        { "unsquad",      0,   0,      2*4096, 0,   0x00000000, 0 },
        { "willow",       0,  0,      0,  1536,   0x00000000, 1 },
        { "3wonders",0x0800,  0,      0,     0,   0x00000000, 0 },

        /* End of table (default values) */
        { 0,           0,   0,   0,   0,          0x00000000, 0 },
};

static int cps1_sh_init(const char *gamename)
{
        struct CPS1config *pCFG=&cps1_config_table[0];
        while(pCFG->name)
        {
                if (stricmp(pCFG->name, gamename) == 0)
                {
                        break;
                }
                pCFG++;
        }

        cps1_game_config=pCFG;

#if 0
        if (pCFG->code_start)
        {
             WRITE_WORD(&RAM[0x0004], pCFG->code_start>>16);
             WRITE_WORD(&RAM[0x0006], pCFG->code_start&0xffff);
        }
#endif
        return 0;
}

static void cps1_init_machine(void)
{
}

/********************************************************************
*
*  CPS1 Drivers
*  ============
*
*  Although the hardware is the same, the memory maps and graphics
*  layout will be different.
*
********************************************************************/


/********************************************************************

                              STRIDER

********************************************************************/

INPUT_PORTS_START( input_ports_strider )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_START
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* ???? */
        PORT_DIPNAME( 0xff, 0xff, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0xff, "Off")

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0xFF, 0xFF, "DIP A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0xFF, "Off")

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0xFF, 0xFF, "DIP B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0xFF, "Off")

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x20, 0x00, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x10, 0x00, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x08, 0x00, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x04, 0x04, "Freeze Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x03, 0x03, "Players", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPSETTING(    0x02, "5" )
        PORT_DIPSETTING(    0x03, "6" )
INPUT_PORTS_END


static struct MemoryReadAddress readmem_strider[] =
{
        { 0x000000, 0x0fffff, MRA_ROM },     /* 68000 ROM */
        { 0x800000, 0x800003, cps1_input_r}, /* Not really, but it gets them working */
        { 0x800018, 0x80001f, cps1_input_r}, /* Input ports */
        { 0x800100, 0x8001ff, MRA_BANK4, &cps1_output },  /* Output ports */
        { 0xff0000, 0xffffff, MRA_BANK2 },   /* RAM */
        { 0x860000, 0x8603ff, MRA_BANK3 },   /* ??? Unknown ??? */

        /* Game dependent graphics RAM */
        { 0x900000, 0x903fff, cps1_work_ram_r }, /* Work RAM */
        { 0x904000, 0x907fff, cps1_obj_r },      /* Object RAM */
        { 0x908000, 0x90bfff, cps1_scroll1_r },  /* Video RAM 1 */
        { 0x90c000, 0x90ffff, cps1_scroll2_r },  /* Video RAM 2 */
        { 0x910000, 0x913fff, cps1_scroll3_r },  /* Video RAM 3 */
        { 0x914000, 0x9157ff, cps1_palette_r },  /* Palette */
        { 0x915800, 0x930000, MRA_BANK5 }, /* OTHER RAM (3 wonders) */

        /* Final fight 91c000 = ?? */
        { 0x91c000, 0x91c003, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_strider[] =
{
        { 0x000000, 0x0fffff, MWA_ROM },      /* ROM */
        { 0x800030, 0x800033, MWA_NOP },      /* Unknown output */
        { 0x800180, 0x800183, cps1_sound_cmd_w },  /* Sound command */
        { 0x800100, 0x8001ff, MWA_BANK4, &cps1_output, &cps1_output_size },  /* Output ports */
        { 0xff0000, 0xffffff, MWA_BANK2, &cps1_ram, &cps1_ram_size },        /* RAM */

        /* Game dependent graphics RAM */
        { 0x900000, 0x903fff, cps1_work_ram_w, &cps1_work_ram, &cps1_work_ram_size },        /* Work RAM */
        { 0x904000, 0x907fff, cps1_obj_w,      &cps1_obj, &cps1_obj_size },
        { 0x908000, 0x90bfff, cps1_scroll1_w,  &cps1_scroll1, &cps1_scroll1_size },  /* Video RAM 1 */
        { 0x90c000, 0x90ffff, cps1_scroll2_w,  &cps1_scroll2, &cps1_scroll2_size },  /* Video RAM 2 */
        { 0x910000, 0x913fff, cps1_scroll3_w,  &cps1_scroll3, &cps1_scroll3_size },  /* Video RAM 3 */
        { 0x914000, 0x9157ff, cps1_palette_w,  &cps1_palette, &cps1_palette_size },  /* Palette */
        { 0x915800, 0x930000, MWA_BANK5 }, /* OTHER RAM (3 wonders) */
        /* Final fight 91c000 = ?? */
        { 0x91c000, 0x91c003, MWA_NOP },
	{ -1 }	/* end of table */
};

CHAR_LAYOUT(charlayout_strider, 2048)
SPRITE_LAYOUT(spritelayout_strider, 8192+2048)
SPRITE_LAYOUT(tilelayout_strider, 8192-2048)
TILE32_LAYOUT(tilelayout32_strider, 4096-256,  0x080000*8)

static struct GfxDecodeInfo gfxdecodeinfo_strider[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x2f0000, &charlayout_strider,    0,                      32 },
        { 1, 0x000000, &spritelayout_strider,  32*16,                  32 },
        { 1, 0x040000, &tilelayout_strider,    32*16+32*16,            32 },
        { 1, 0x200000, &tilelayout32_strider,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        strider_machine_driver,
        readmem_strider,
        writemem_strider,
        cps1_interrupt2,
        gfxdecodeinfo_strider)

ROM_START( strider_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN("strider.30", 0x00000, 0x20000, 0x9f411127 )
        ROM_LOAD_ODD ("strider.35", 0x00000, 0x20000, 0xf12ac928 )
        ROM_LOAD_EVEN("strider.31", 0x40000, 0x20000, 0x7ec385b9 )
        ROM_LOAD_ODD ("strider.36", 0x40000, 0x20000, 0xe9a8bf28 )
        ROM_LOAD_WIDE_SWAP("strider.32", 0x80000, 0x80000, 0x59b99ecf ) /* Tile map */

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "strider.02",   0x000000, 0x80000, 0x5f694f7b ) /* sprites */
        ROM_LOAD( "strider.06",   0x080000, 0x80000, 0x19fbe6a7 ) /* sprites */
        ROM_LOAD( "strider.04",   0x100000, 0x80000, 0x2c9017c2 ) /* sprites */
        ROM_LOAD( "strider.08",   0x180000, 0x80000, 0x3b0e4930 ) /* sprites */

        ROM_LOAD( "strider.01",   0x200000, 0x80000, 0x0e0c0fb8 ) /* tiles + chars */
        ROM_LOAD( "strider.05",   0x280000, 0x80000, 0x542a60c8 ) /* tiles + chars */
        ROM_LOAD( "strider.03",   0x300000, 0x80000, 0x6c8b64c9 ) /* tiles + chars */
        ROM_LOAD( "strider.07",   0x380000, 0x80000, 0x27932793 ) /* tiles + chars */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "strider.09",    0x00000, 0x10000, 0x85adabcd )
        ROM_LOAD( "strider.09",    0x08000, 0x10000, 0x85adabcd )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("strider.18",    0x00000, 0x20000, 0x6e80d9f2 )
        ROM_LOAD ("strider.19",    0x20000, 0x20000, 0xc906d49a )
ROM_END

struct GameDriver strider_driver =
{
        "Strider",
        "strider",
        CPS1_CREDITS
        "Paul Leaman",
        &strider_machine_driver,

        strider_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_strider,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};



/********************************************************************

                              WILLOW

********************************************************************/


static struct MemoryReadAddress readmem_willow[] =
{
        { 0x000000, 0x0fffff, MRA_ROM },     /* 68000 ROM */
        { 0x800000, 0x800003, cps1_input_r}, /* Not really, but it gets them working */
        { 0x800018, 0x80001f, cps1_input_r}, /* Input ports */
        { 0xff0000, 0xffffff, MRA_BANK2 },   /* RAM */
        { 0x860000, 0x8603ff, MRA_BANK3 },   /* ??? Unknown ??? */

        { 0x900000, 0x903fff, cps1_obj_r },      /* Object RAM */
        { 0x904000, 0x907fff, cps1_work_ram_r }, /* Work RAM */
        { 0x910000, 0x913fff, cps1_scroll1_r },  /* Video RAM 3 */
        { 0x914000, 0x917fff, cps1_scroll3_r },  /* Video RAM 1 */
        { 0x918000, 0x91bfff, cps1_scroll2_r },  /* Video RAM 2 */

        { 0x91c000, 0x91d7ff, cps1_palette_r },  /* Palette */
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem_willow[] =
{
        { 0x000000, 0x0fffff, MWA_ROM },      /* ROM */
        { 0x800030, 0x800033, MWA_NOP },      /* Unknown output (0x800000)*/
        { 0x800180, 0x800183, cps1_sound_cmd_w},  /* Sound command */
        { 0x800100, 0x8001ff, MWA_BANK4, &cps1_output, &cps1_output_size },  /* Output ports */
        { 0xff0000, 0xffffff, MWA_BANK2, &cps1_ram, &cps1_ram_size },        /* RAM */

        { 0x900000, 0x903fff, cps1_obj_w,      &cps1_obj, &cps1_obj_size },
        { 0x904000, 0x907fff, cps1_work_ram_w, &cps1_work_ram, &cps1_work_ram_size },        /* Work RAM */

        { 0x910000, 0x913fff, cps1_scroll1_w,  &cps1_scroll3, &cps1_scroll3_size },  /* Video RAM 3 */
        { 0x914000, 0x917fff, cps1_scroll3_w,  &cps1_scroll1, &cps1_scroll1_size },  /* Video RAM 1 */
        { 0x918000, 0x91bfff, cps1_scroll2_w,  &cps1_scroll2, &cps1_scroll2_size },  /* Video RAM 2 */

        { 0x91c000, 0x91d7ff, cps1_palette_w,  &cps1_palette, &cps1_palette_size },  /* Palette */
	{ -1 }	/* end of table */
};

CHAR_LAYOUT(charlayout_willow, 4096)
TILE32_LAYOUT(tile32layout_willow, 1024,    0x080000*8)
TILE_LAYOUT2(tilelayout_willow, 2*4096,     0x080000*8, 0x020000 )

static struct GfxDecodeInfo gfxdecodeinfo_willow[] =
{
        /*   start    pointer                 start   number of colours */
        { 1, 0x0f0000, &charlayout_willow,    0,                      32 },
        { 1, 0x000000, &spritelayout_strider, 32*16,                  32 },
        { 1, 0x200000, &tilelayout_willow,    32*16+32*16,            32 },
        { 1, 0x050000, &tile32layout_willow,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        willow_machine_driver,
        readmem_willow,
        writemem_willow,
        cps1_interrupt2,
        gfxdecodeinfo_willow)

ROM_START( willow_rom )
        ROM_REGION(0x100000)      /* 68000 code */

        ROM_LOAD_EVEN("WLU_30.ROM", 0x00000, 0x20000, 0x06b81390 )
        ROM_LOAD_ODD ("WLU_35.ROM", 0x00000, 0x20000, 0x6c67bfcf )
        ROM_LOAD_EVEN("WLU_31.ROM", 0x40000, 0x20000, 0xac363d78 )
        ROM_LOAD_ODD ("WLU_36.ROM", 0x40000, 0x20000, 0xf7fc564e )
        ROM_LOAD_WIDE_SWAP("WL_32.ROM",  0x80000, 0x80000, 0xe75769a9 ) /* Tile map */

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "WL_GFX1.ROM",   0x000000, 0x80000, 0x352afe30 ) /* sprites */
        ROM_LOAD( "WL_GFX5.ROM",   0x080000, 0x80000, 0x768c375a ) /* sprites */
        ROM_LOAD( "WL_GFX3.ROM",   0x100000, 0x80000, 0x3c9240f4 ) /* sprites */
        ROM_LOAD( "WL_GFX7.ROM",   0x180000, 0x80000, 0x95e77d43 ) /* sprites */

        ROM_LOAD( "WL_20.ROM",     0x200000, 0x20000, 0x67c34af5 ) /* tiles */
        ROM_LOAD( "WL_10.ROM",     0x220000, 0x20000, 0xc8c6fe3c ) /* tiles */
        ROM_LOAD( "WL_22.ROM",     0x240000, 0x20000, 0x6a95084b ) /* tiles */
        ROM_LOAD( "WL_12.ROM",     0x260000, 0x20000, 0x66735153 ) /* tiles */

        ROM_LOAD( "WL_24.ROM",     0x280000, 0x20000, 0x839200be ) /* tiles */
        ROM_LOAD( "WL_14.ROM",     0x2a0000, 0x20000, 0x6cf8259c ) /* tiles */
        ROM_LOAD( "WL_26.ROM",     0x2c0000, 0x20000, 0xeef53eb5 ) /* tiles */
        ROM_LOAD( "WL_16.ROM",     0x2e0000, 0x20000, 0x6a02dfde ) /* tiles */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "WL_09.ROM",    0x00000, 0x10000, 0x56014501 )
        ROM_LOAD( "WL_09.ROM",    0x08000, 0x10000, 0x56014501 )


        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("WL_18.ROM",    0x00000, 0x20000, 0x58bb142b )
        ROM_LOAD ("WL_19.ROM",    0x20000, 0x20000, 0xc575f23d )
ROM_END

struct GameDriver willow_driver =
{
        "Willow",
        "willow",
        CPS1_CREDITS
        "Paul Leaman",
        &willow_machine_driver,
        willow_rom,
        0,0,0,0,
        input_ports_strider,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};

/********************************************************************

                              FINAL FIGHT
                              ===========

   Strider memory map.


********************************************************************/

INPUT_PORTS_START( input_ports_ffight )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_START
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* ???? */
        PORT_DIPNAME( 0xff, 0xff, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0xff, "Off")

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0xFF, 0xFF, "DIP A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0xFF, "Off")

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0xFF, 0xFF, "DIP B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0xFF, "Off")

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x20, 0x00, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x10, 0x00, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Freeze Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x04, 0x00, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x03, 0x03, "Players", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPSETTING(    0x02, "5" )
        PORT_DIPSETTING(    0x03, "6" )
INPUT_PORTS_END

CHAR_LAYOUT(charlayout_ffight,     2048)
SPRITE_LAYOUT(spritelayout_ffight, 4096*2+512)
SPRITE_LAYOUT(tilelayout_ffight,   4096)
TILE32_LAYOUT(tile32layout_ffight, 512+128,     0x080000*8)

static struct GfxDecodeInfo gfxdecodeinfo_ffight[] =
{
        /*   start    pointer                 start   number of colours */
        { 1, 0x044000, &charlayout_ffight,    0,                      32 },
        { 1, 0x000000, &spritelayout_ffight,  32*16,                  32 },
        { 1, 0x060000, &tilelayout_ffight,    32*16+32*16,            32 },
        { 1, 0x04c000, &tile32layout_ffight,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        ffight_machine_driver,
        readmem_strider,
        writemem_strider,
        cps1_interrupt2,
        gfxdecodeinfo_ffight)

ROM_START( ffight_rom )
        ROM_REGION(0x100000)      /* 68000 code */

        ROM_LOAD_EVEN("FF30-36.BIN", 0x00000, 0x20000, 0x3ad2c910 )
        ROM_LOAD_ODD ("FF35-42.BIN", 0x00000, 0x20000, 0xe2c402c0 )
        ROM_LOAD_EVEN("FF31-37.BIN", 0x40000, 0x20000, 0x2bfa6a30 )
        ROM_LOAD_ODD ("FF36-43.BIN", 0x40000, 0x20000, 0x7fdadd8c )
        ROM_LOAD_WIDE_SWAP("FF32-32M.BIN",  0x80000, 0x80000, 0x5247370d ) /* Tile map */

        ROM_REGION(0x500000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "FF01-01M.BIN",   0x000000, 0x80000, 0x6dd3c4d5 ) /* sprites */
        ROM_LOAD( "FF05-05M.BIN",   0x080000, 0x80000, 0x16c0c960 ) /* sprites */
        ROM_LOAD( "FF03-03M.BIN",   0x100000, 0x80000, 0x98c30301 ) /* sprites */
        ROM_LOAD( "FF07-07M.BIN",   0x180000, 0x80000, 0x306ada3e ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "FF09-09.BIN",    0x00000, 0x10000, 0x06b2f7f8 )
        ROM_LOAD( "FF09-09.BIN",    0x08000, 0x10000, 0x06b2f7f8 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("FF18-18.BIN",    0x00000, 0x20000, 0x42467b56 )
        ROM_LOAD ("FF19-19.BIN",    0x20000, 0x20000, 0xabd11a71 )
ROM_END


struct GameDriver ffight_driver =
{
        "Final Fight",
        "ffight",
        CPS1_CREDITS
        "Paul Leaman",
        &ffight_machine_driver,
        ffight_rom,
        0,0,0,0,
        input_ports_ffight,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};

/********************************************************************

                          UN Squadron

********************************************************************/

CHAR_LAYOUT(charlayout_unsquad, 4096)
SPRITE_LAYOUT(spritelayout_unsquad, 4096*2-2048)
SPRITE_LAYOUT(tilelayout_unsquad, 8192-2048)
TILE32_LAYOUT(tilelayout32_unsquad, 1024,  0x080000*8)

static struct GfxDecodeInfo gfxdecodeinfo_unsquad[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x030000, &charlayout_unsquad,    0,                      32 },
        { 1, 0x000000, &spritelayout_unsquad,  32*16,                  32 },
        { 1, 0x040000, &tilelayout_unsquad,    32*16+32*16,            32 },
        { 1, 0x060000, &tilelayout32_unsquad,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        unsquad_machine_driver,
        readmem_strider,
        writemem_strider,
        cps1_interrupt2,
        gfxdecodeinfo_unsquad)

ROM_START( unsquad_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN("UNSQUAD.30",  0x00000, 0x20000, 0x474a8f2c )
        ROM_LOAD_ODD ("UNSQUAD.35",  0x00000, 0x20000, 0x46432039 )
        ROM_LOAD_EVEN("UNSQUAD.31",  0x40000, 0x20000, 0x4f1952f1 )
        ROM_LOAD_ODD ("UNSQUAD.36",  0x40000, 0x20000, 0xf69148b7 )
        ROM_LOAD_WIDE_SWAP( "UNSQUAD.32", 0x80000, 0x80000, 0x45a55eb7 ) /* tiles + chars */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "UNSQUAD.01",   0x000000, 0x80000, 0x87c8d9a8 ) /* tiles + chars */
        ROM_LOAD( "UNSQUAD.05",   0x080000, 0x80000, 0xea7f4a55 ) /* tiles + chars */
        ROM_LOAD( "UNSQUAD.03",   0x100000, 0x80000, 0x0ce0ac76 ) /* tiles + chars */
        ROM_LOAD( "UNSQUAD.07",   0x180000, 0x80000, 0x837e8800 ) /* tiles + chars */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "UNSQUAD.09",    0x00000, 0x10000, 0xc55f46db )
        ROM_LOAD( "UNSQUAD.09",    0x08000, 0x10000, 0xc55f46db )

        ROM_REGION(0x20000) /* Samples */
        ROM_LOAD ("UNSQUAD.18",    0x00000, 0x20000, 0x6cc60418 )
ROM_END

struct GameDriver unsquad_driver =
{
        "UN Squadron",
        "unsquad",
        CPS1_CREDITS
        "Paul Leaman",
        &unsquad_machine_driver,

        unsquad_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_ffight,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};


/********************************************************************

                          Mega Twins

********************************************************************/


CHAR_LAYOUT(charlayout_mtwins,  4096)
SPRITE_LAYOUT(spritelayout_mtwins, 4096*2-2048)
SPRITE_LAYOUT(tilelayout_mtwins, 8192-2048)
TILE32_LAYOUT(tilelayout32_mtwins, 512,  0x080000*8)

static struct GfxDecodeInfo gfxdecodeinfo_mtwins[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x030000, &charlayout_mtwins,    0,                      32 },
        { 1, 0x000000, &spritelayout_mtwins,  32*16,                  32 },
        { 1, 0x040000, &tilelayout_mtwins,    32*16+32*16,            32 },
        { 1, 0x070000, &tilelayout32_mtwins,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        mtwins_machine_driver,
        readmem_strider,
        writemem_strider,
        cps1_interrupt2,
        gfxdecodeinfo_mtwins)

ROM_START( mtwins_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN("CHE_30.ROM",  0x00000, 0x20000, 0x32b80b2e )
        ROM_LOAD_ODD ("CHE_35.ROM",  0x00000, 0x20000, 0xdb8b9017 )
        ROM_LOAD_EVEN("CHE_31.ROM",  0x40000, 0x20000, 0x3fa4bac6 )
        ROM_LOAD_ODD ("CHE_36.ROM",  0x40000, 0x20000, 0x70d9f263 )
        ROM_LOAD_WIDE_SWAP( "CH_32.ROM", 0x80000, 0x80000, 0xbc81ffbb ) /* tiles + chars */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "CH_GFX1.ROM",   0x000000, 0x80000, 0x8394505c ) /* tiles + chars */
        ROM_LOAD( "CH_GFX5.ROM",   0x080000, 0x80000, 0x8ce0dcfe ) /* tiles + chars */
        ROM_LOAD( "CH_GFX3.ROM",   0x100000, 0x80000, 0x12e50cdf ) /* tiles + chars */
        ROM_LOAD( "CH_GFX7.ROM",   0x180000, 0x80000, 0xb11a09e0 ) /* tiles + chars */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "CH_09.ROM",    0x00000, 0x10000, 0x9dfa57d4 )
        ROM_LOAD( "CH_09.ROM",    0x08000, 0x10000, 0x9dfa57d4 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("CH_18.ROM",    0x00000, 0x20000, 0x5c96f786 )
        ROM_LOAD ("CH_19.ROM",    0x20000, 0x20000, 0xb12d19a5 )
ROM_END


struct GameDriver mtwins_driver =
{
        "Mega Twins",
        "mtwins",
        CPS1_CREDITS
        "Paul Leaman",
        &mtwins_machine_driver,

        mtwins_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_ffight,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};



/********************************************************************

                          GHOULS AND GHOSTS

        Possibly incomplete ROM set.

********************************************************************/

static struct MemoryReadAddress readmem_ghouls[] =
{
        { 0x000000, 0x0fffff, MRA_ROM },     /* 68000 ROM */
        { 0x800000, 0x800003, cps1_input_r}, /* Not really, but it gets them working */
        { 0x800018, 0x80001f, cps1_input_r}, /* Input ports */
        { 0xff0000, 0xffffff, MRA_BANK2 },   /* RAM */
        { 0x860000, 0x8603ff, MRA_BANK3 },   /* ??? Unknown ??? */

        { 0x900000, 0x903fff, cps1_scroll1_r },
        { 0x904000, 0x907fff, cps1_scroll2_r },
        { 0x908000, 0x90bfff, cps1_scroll3_r },
        { 0x90c000, 0x90ffff, cps1_work_ram_r },
        { 0x920000, 0x9287ff, cps1_obj_r },
        { 0x910000, 0x911fff, cps1_palette_r },

	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_ghouls[] =
{
        { 0x000000, 0x0fffff, MWA_ROM },      /* ROM */
        { 0x800030, 0x800033, MWA_NOP },      /* Unknown output (0x800000)*/
        { 0x800180, 0x800183, cps1_sound_cmd_w},  /* Sound command */
        { 0x800100, 0x8001ff, MWA_BANK4, &cps1_output, &cps1_output_size },  /* Output ports */
        { 0xff0000, 0xffffff, MWA_BANK2, &cps1_ram, &cps1_ram_size },        /* RAM */

        { 0x900000, 0x903fff, cps1_scroll1_w,  &cps1_scroll1, &cps1_scroll1_size },
        { 0x904000, 0x907fff, cps1_scroll2_w,  &cps1_scroll2, &cps1_scroll2_size },
        { 0x908000, 0x90bfff, cps1_scroll3_w,  &cps1_scroll3, &cps1_scroll3_size },
        { 0x90c000, 0x90ffff, cps1_work_ram_w, &cps1_work_ram, &cps1_work_ram_size },
        { 0x910000, 0x911fff, cps1_palette_w,  &cps1_palette, &cps1_palette_size },
        { 0x920000, 0x9287ff, cps1_obj_w,      &cps1_obj, &cps1_obj_size },
	{ -1 }	/* end of table */
};
#define SPRITE_SEP (0x80000*8)
static struct GfxLayout spritelayout_ghouls =
{
        16,16,  /* 16*16 sprites */
        8192+1024,   /* 8192 sprites */
        4,      /* 4 bits per pixel */
        { 0x60000*8,0x40000*8,0x20000*8,0 },
        {
           0,1,2,3,4,5,6,7,
           SPRITE_SEP+0, SPRITE_SEP+1, SPRITE_SEP+2, SPRITE_SEP+3,
           SPRITE_SEP+4, SPRITE_SEP+5, SPRITE_SEP+6, SPRITE_SEP+7
        },
        {
           0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
           8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
        },
        16*8    /* every sprite takes 16*8 consecutive bytes */
};

#define TILE_SEP (32*8)

static struct GfxLayout charlayout =
{
        8,8,   /* 16*16 tiles */
        2*4096,  /* 8192  tiles */
        4,       /* 4 bits per pixel */
        {0x8000*8+8,0x8000*8,8,0},
        {
           0,1,2,3,4,5,6,7,
        },
        {
           0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8,
        },
        16*8    /* every sprite takes 16*8 consecutive bytes */
};

#if 0
/* WRONG !!! */
static struct GfxLayout tilelayout=
{
  16,16,  /* 16*16 tiles */
  3096,   /* 2048 tiles */
  4,      /* 4 bits per pixel */
  { 0,4, 0, 4 }, //0x08000*8,(0x08000*8)+4 },
  {
    0,1,2,3,8,9,10,11,
    (8*4*8)+0,(8*4*8)+1,(8*4*8)+2,(8*4*8)+3,
    (8*4*8)+8,(8*4*8)+9,(8*4*8)+10,(8*4*8)+11
  },
  {
    0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
    8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
  },
  512   /* each tile takes 512 consecutive bytes */
};
#endif

TILE_LAYOUT2(tilelayout_ghouls, 4096,     0x040000*8, 0x010000 )

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        /*   start    pointer       colour start   number of colours */
        { 1, 0x10F000, &charlayout,             0,  32 },
        { 1, 0x000000, &spritelayout_ghouls,    0,  32 },
        { 1, 0x080000, &tilelayout_ghouls,      0,  32 },
        { 1, 0x100000, &tilelayout_ghouls,      0,  32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        ghouls_machine_driver,
        readmem_ghouls,
        writemem_ghouls,
        cps1_interrupt,
        gfxdecodeinfo)

ROM_START( ghouls_rom )
        ROM_REGION(0x100000)      /*  */
        ROM_LOAD_EVEN("dmu.29",   0x00000, 0x20000, 0x90efb087 ) /* 68000 code */
        ROM_LOAD_ODD ("dmu.30",   0x00000, 0x20000, 0xac09ca89 ) /* 68000 code */
        ROM_LOAD_EVEN("dmu.27",   0x40000, 0x20000, 0x7bc0c8d8 ) /* 68000 code */
        ROM_LOAD_ODD ("dmu.28",   0x40000, 0x20000, 0xc07d69e3 ) /* 68000 code */
        ROM_LOAD_WIDE("dmu.29",   0x80000, 0x20000, 0x90efb087 ) /* 68000 code */
        ROM_LOAD_WIDE("dmu.29",   0xa0000, 0x20000, 0x90efb087 ) /* 68000 code */

        ROM_REGION(0x800000)     /* temporary space for graphics (disposed after conversion) */

        ROM_LOAD( "dmu.23",      0x000000, 0x10000, 0xa1a14a87 ) /* sprites Plane 4 */
        ROM_LOAD( "dmu.23",      0x010000, 0x10000, 0xa1a14a87 ) /* sprites Plane 4 */
        ROM_LOAD( "dmu.10",      0x020000, 0x10000, 0x1ad5edb3 ) /* sprites Plane 1 */
        ROM_LOAD( "dmu.10",      0x030000, 0x10000, 0x1ad5edb3 ) /* sprites Plane 1 */
        ROM_LOAD( "dmu.14",      0x040000, 0x10000, 0xa1e55fd9 ) /* sprites Plane 2 */
        ROM_LOAD( "dmu.14",      0x050000, 0x10000, 0xa1e55fd9 ) /* sprites Plane 2 */
        ROM_LOAD( "dmu.19",      0x060000, 0x10000, 0x142db95f ) /* sprites Plane 3 */
        ROM_LOAD( "dmu.19",      0x070000, 0x10000, 0x142db95f ) /* sprites Plane 3 */

        ROM_LOAD( "dmu.10",      0x080000, 0x10000, 0x0f231d0f ) /* 16x16 */
        ROM_LOAD( "dmu.23",      0x090000, 0x10000, 0x0f231d0f ) /* 16x16 */
        ROM_LOAD( "dmu.14",      0x0a0000, 0x10000, 0xb0882fde ) /* 16x16 */

        ROM_LOAD( "dmu.21",      0x0c0000, 0x10000, 0xc47e04ba ) /* 16x16 */
        ROM_LOAD( "dmu.12",      0x0d0000, 0x10000, 0xc47e04ba ) /* 16x16 */
        ROM_LOAD( "dmu.25",      0x0e0000, 0x10000, 0x30cecdb6 ) /* 16x16 */
        ROM_LOAD( "dmu.16",      0x0f0000, 0x10000, 0x30cecdb6 ) /* 16x16 */

        ROM_LOAD( "dmu.11",      0x100000, 0x10000, 0x3d57242d ) /* 32x32 */
        ROM_LOAD( "dmu.13",      0x110000, 0x10000, 0x18a99b29 ) /* 32x32 */
        ROM_LOAD( "dmu.15",      0x120000, 0x10000, 0x237eb922 ) /* 32x32 */
        ROM_LOAD( "dmu.20",      0x130000, 0x10000, 0x08a71d8f ) /* 32x32 */
        ROM_LOAD( "dmu.22",      0x130000, 0x10000, 0x08a71d8f ) /* 32x32 */
        ROM_LOAD( "dmu.24",      0x130000, 0x10000, 0x08a71d8f ) /* 32x32 */

        ROM_REGION(0x18000) /* 64k for the audio CPU */
        ROM_LOAD( "dmu.26",      0x0000, 0x10000, 0x8df5e803 )
ROM_END

struct GameDriver ghouls_driver =
{
        "Ghouls and Ghosts",
        "ghouls",
        CPS1_CREDITS
        "Paul Leaman",
        &ghouls_machine_driver,

        ghouls_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_strider,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};





