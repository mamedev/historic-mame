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
#define CPS1_CREDITS(X) X " (Game driver)\n"\
                     "Paul Leaman (Capcom System 1 driver)\n"\
                     "Aaron Giles (additional info)\n"\


/********************************************************************
*
*  Sound board
*
*********************************************************************/
static void cps1_irq_handler_mus (void) {  cpu_cause_interrupt (0, 0); }
static struct YM2151interface interface_mus =
{
        1,                      /* 1 chip */
        4000000,                /* 4 MHZ TODO: find out the real frq */
        { 170 },                /* 170 somethings */
        { cps1_irq_handler_mus }
};

static struct OKIM6295interface interface_okim6295 =
{
        1,              /* 1 chip */
        8000,           /* 8000Hz ??? TODO: find out the real frequency */
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
*  Memory Read-Write handlers
*  ==========================
*
********************************************************************/

static struct MemoryReadAddress cps1_readmem[] =
{
        { 0x000000, 0x0fffff, MRA_ROM },     /* 68000 ROM */
        { 0xff0000, 0xffffff, MRA_BANK2 },   /* RAM */
        { 0x900000, 0x92ffff, MRA_BANK5 },
        { 0x800000, 0x800003, cps1_input_r}, /* Not really, but it gets them working */
        { 0x800018, 0x80001f, cps1_input_r}, /* Input ports */
        { 0x860000, 0x8603ff, MRA_BANK3 },   /* ??? Unknown ??? */
        { 0x800100, 0x8001ff, MRA_BANK4 },  /* Output ports */
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress cps1_writemem[] =
{
        { 0x000000, 0x0fffff, MWA_ROM },      /* ROM */
        { 0xff0000, 0xffffff, MWA_BANK2, &cps1_ram, &cps1_ram_size },        /* RAM */
        { 0x900000, 0x92ffff, MWA_BANK5, &cps1_gfxram, &cps1_gfxram_size},
        { 0x800030, 0x800033, MWA_NOP },      /* ??? Unknown ??? */
        { 0x800180, 0x800183, cps1_sound_cmd_w},  /* Sound command */
        { 0x800100, 0x8001ff, MWA_BANK4, &cps1_output, &cps1_output_size },  /* Output ports */
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

#define MACHINE_DRIVER(CPS1_DRVNAME, CPS1_IRQ, CPS1_GFX) \
static struct MachineDriver CPS1_DRVNAME =                             \
{                                                                        \
        /* basic machine hardware */                                     \
        {                                                                \
                {                                                        \
                        CPU_M68000,                                      \
                        8000000,   /* 8MHz ? TODO: Find real FRQ */      \
                        0,                                               \
                        cps1_readmem,cps1_writemem,0,0,                  \
                        CPS1_IRQ, 2   /* 2 interrupts per frame */       \
                },                                                       \
                {                                                        \
                        CPU_Z80 | CPU_AUDIO_CPU,                         \
                        3000000,  /* 3 Mhz ??? TODO: find real FRQ */    \
                        2,      /* memory region #2 */                   \
                        sound_readmem,sound_writemem,0,0,                \
                        interrupt,4                                      \
                }                                                        \
        },                                                               \
        60, DEFAULT_60HZ_VBLANK_DURATION,                                \
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
  {"strider",      0,   0, 0x2000,      0, 0x000000, 0, 0x0020,0x0020,0x0020},
  {"ffight",  0x0400,   0, 0x2000, 0x0200, 0x000000, 2, 0x4420,0x3000,0x0980},
  {"mtwins",       0,   0, 0x2000, 0x0e00, 0x000000, 3, 0x0020,0x0000,0x0000},
  {"unsquad",      0,   0, 0x2000,      0, 0x000000, 0, 0x0020,0x0000,0x0000},
  {"willow",       0,   0,      0,   1536, 0x000000, 1, 0x7020,0x0000,0x0a00},
  {"msword",       0,   0, 0x2800, 0x0e00, 0x000000, 3,   -1,   -1,   -1 },
//  {"nemo",         0,   0,      0,   1536, 0x000000, 1,   -1,   -1,   -1 },
//  {"3wonders",0x0800,   0,      0,      0, 0x000000, 0,   -1,   -1,   -1 },

  /* End of table (default values) */
  {0,           0,   0,   0,   0,          0x000000, 0,   -1,   -1,   -1 },
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

        if (pCFG->space_scroll1 != -1)
        {
                pCFG->space_scroll1 &= 0xfff;
        }


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
        ROM_LOAD( "strider.02",   0x000000, 0x80000, 0x5f694f7b )
        ROM_LOAD( "strider.06",   0x080000, 0x80000, 0x19fbe6a7 )
        ROM_LOAD( "strider.04",   0x100000, 0x80000, 0x2c9017c2 )
        ROM_LOAD( "strider.08",   0x180000, 0x80000, 0x3b0e4930 )

        ROM_LOAD( "strider.01",   0x200000, 0x80000, 0x0e0c0fb8 )
        ROM_LOAD( "strider.05",   0x280000, 0x80000, 0x542a60c8 )
        ROM_LOAD( "strider.03",   0x300000, 0x80000, 0x6c8b64c9 )
        ROM_LOAD( "strider.07",   0x380000, 0x80000, 0x27932793 )

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
        CPS1_CREDITS("Paul Leaman"),
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
        ROM_LOAD( "WL_GFX1.ROM",   0x000000, 0x80000, 0x352afe30 )
        ROM_LOAD( "WL_GFX5.ROM",   0x080000, 0x80000, 0x768c375a )
        ROM_LOAD( "WL_GFX3.ROM",   0x100000, 0x80000, 0x3c9240f4 )
        ROM_LOAD( "WL_GFX7.ROM",   0x180000, 0x80000, 0x95e77d43 )

        ROM_LOAD( "WL_20.ROM",     0x200000, 0x20000, 0x67c34af5 )
        ROM_LOAD( "WL_10.ROM",     0x220000, 0x20000, 0xc8c6fe3c )
        ROM_LOAD( "WL_22.ROM",     0x240000, 0x20000, 0x6a95084b )
        ROM_LOAD( "WL_12.ROM",     0x260000, 0x20000, 0x66735153 )

        ROM_LOAD( "WL_24.ROM",     0x280000, 0x20000, 0x839200be )
        ROM_LOAD( "WL_14.ROM",     0x2a0000, 0x20000, 0x6cf8259c )
        ROM_LOAD( "WL_26.ROM",     0x2c0000, 0x20000, 0xeef53eb5 )
        ROM_LOAD( "WL_16.ROM",     0x2e0000, 0x20000, 0x6a02dfde )

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
        CPS1_CREDITS("Paul Leaman"),
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
        ROM_LOAD( "FF01-01M.BIN",   0x000000, 0x80000, 0x6dd3c4d5 )
        ROM_LOAD( "FF05-05M.BIN",   0x080000, 0x80000, 0x16c0c960 )
        ROM_LOAD( "FF03-03M.BIN",   0x100000, 0x80000, 0x98c30301 )
        ROM_LOAD( "FF07-07M.BIN",   0x180000, 0x80000, 0x306ada3e )

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
        CPS1_CREDITS("Paul Leaman"),
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
        ROM_LOAD( "UNSQUAD.01",   0x000000, 0x80000, 0x87c8d9a8 )
        ROM_LOAD( "UNSQUAD.05",   0x080000, 0x80000, 0xea7f4a55 )
        ROM_LOAD( "UNSQUAD.03",   0x100000, 0x80000, 0x0ce0ac76 )
        ROM_LOAD( "UNSQUAD.07",   0x180000, 0x80000, 0x837e8800 )

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
        CPS1_CREDITS("Paul Leaman"),
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
        ROM_LOAD( "CH_GFX1.ROM",   0x000000, 0x80000, 0x8394505c )
        ROM_LOAD( "CH_GFX5.ROM",   0x080000, 0x80000, 0x8ce0dcfe )
        ROM_LOAD( "CH_GFX3.ROM",   0x100000, 0x80000, 0x12e50cdf )
        ROM_LOAD( "CH_GFX7.ROM",   0x180000, 0x80000, 0xb11a09e0 )

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
        CPS1_CREDITS("Paul Leaman"),
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

                          Magic Sword

********************************************************************/


CHAR_LAYOUT(charlayout_msword,  4096);
SPRITE_LAYOUT(spritelayout_msword, 8192);
SPRITE_LAYOUT(tilelayout_msword, 4096);
TILE32_LAYOUT(tilelayout32_msword, 512,  0x080000*8);

static struct GfxDecodeInfo gfxdecodeinfo_msword[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x040000, &charlayout_msword,    0,                      32 },
        { 1, 0x000000, &spritelayout_msword,  32*16,                  32 },
        { 1, 0x050000, &tilelayout_msword,    32*16+32*16,            32 },
        { 1, 0x070000, &tilelayout32_msword,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        msword_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_msword);

ROM_START( msword_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN("MSE_30.ROM",  0x00000, 0x20000, 0xa99a131a )
        ROM_LOAD_ODD ("MSE_35.ROM",  0x00000, 0x20000, 0x452c3188 )
        ROM_LOAD_EVEN("MSE_31.ROM",  0x40000, 0x20000, 0x1948cd8a )
        ROM_LOAD_ODD ("MSE_36.ROM",  0x40000, 0x20000, 0x19a7b0c1 )
        ROM_LOAD_WIDE_SWAP( "MS_32.ROM", 0x80000, 0x80000, 0x78415913 )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "MS_GFX1.ROM",   0x000000, 0x80000, 0x0ac52871 )
        ROM_LOAD( "MS_GFX5.ROM",   0x080000, 0x80000, 0xd9d46ec2 )
        ROM_LOAD( "MS_GFX3.ROM",   0x100000, 0x80000, 0x957cc634 )
        ROM_LOAD( "MS_GFX7.ROM",   0x180000, 0x80000, 0xb546d1d6 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "MS_9.ROM",    0x00000, 0x10000, 0x16de56f0 )
        ROM_LOAD( "MS_9.ROM",    0x08000, 0x10000, 0x16de56f0 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("MS_18.ROM",    0x00000, 0x20000, 0x422df0cd )
        ROM_LOAD ("MS_19.ROM",    0x20000, 0x20000, 0xf249d475 )
ROM_END

struct GameDriver msword_driver =
{
        "Magic Sword",
        "msword",
        CPS1_CREDITS("Paul Leaman"),
        &msword_machine_driver,

        msword_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_ffight,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};

