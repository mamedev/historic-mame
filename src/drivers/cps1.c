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

  68000 clock speeds are unknown for all games (except where commented)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/2151intf.h"
#include "sndhrdw/adpcm.h"

#include "cps1.h"       /* External CPS1 defintions */

/* Please include this credit for the CPS1 driver */
#define CPS1_CREDITS(X) X "\n\n"\
                     "Paul Leaman (Capcom System 1 driver)\n"\
                     "Aaron Giles (additional code)\n"\


#define CPS1_DEFAULT_CPU_SPEED          10000000
#define CPS1_DEFAULT_CPU_SLOW_SPEED      8000000

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
        { 0x800000, 0x800003, cps1_player_input_r}, /* Player input ports */
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

#define MACHINE_DRIVER(CPS1_DRVNAME, CPS1_IRQ, CPS1_GFX, CPS1_CPU_FRQ) \
        MACHINE_DRV(CPS1_DRVNAME, CPS1_IRQ, 2, CPS1_GFX, CPS1_CPU_FRQ)

#define MACHINE_DRV(CPS1_DRVNAME, CPS1_IRQ, CPS1_ICNT, CPS1_GFX, CPS1_CPU_FRQ) \
static struct MachineDriver CPS1_DRVNAME =                             \
{                                                                        \
        /* basic machine hardware */                                     \
        {                                                                \
                {                                                        \
                        CPU_M68000,                                      \
                        CPS1_CPU_FRQ,                                    \
                        0,                                               \
                        cps1_readmem,cps1_writemem,0,0,                  \
                        CPS1_IRQ, CPS1_ICNT  /* ??? interrupts per frame */   \
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

/********************************************************************

                    Graphics Layout macros

********************************************************************/

#define SPRITE_LAYOUT(LAYOUT, SPRITES, SPRITE_SEP2, PLANE_SEP) \
static struct GfxLayout LAYOUT = \
{                                               \
        16,16,   /* 16*16 sprites */             \
        SPRITES,  /* ???? sprites */            \
        4,       /* 4 bits per pixel */            \
        { PLANE_SEP+8,PLANE_SEP,8,0 },            \
        { SPRITE_SEP2+0,SPRITE_SEP2+1,SPRITE_SEP2+2,SPRITE_SEP2+3, \
          SPRITE_SEP2+4,SPRITE_SEP2+5,SPRITE_SEP2+6,SPRITE_SEP2+7,  \
          0,1,2,3,4,5,6,7, },\
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8, \
           16*8, 18*8, 20*8, 22*8, 24*8, 26*8, 28*8, 30*8, }, \
        32*8    /* every sprite takes 32*8*2 consecutive bytes */ \
};

#define CHAR_LAYOUT(LAYOUT, CHARS, PLANE_SEP) \
static struct GfxLayout LAYOUT =        \
{                                        \
        8,8,    /* 8*8 chars */             \
        CHARS,  /* ???? chars */        \
        4,       /* 4 bits per pixel */     \
        { PLANE_SEP+8,PLANE_SEP,8,0 },     \
        { 0,1,2,3,4,5,6,7, },                         \
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8,}, \
        16*8    /* every sprite takes 32*8*2 consecutive bytes */\
};

#define TILE32_LAYOUT(LAYOUT, TILES, SEP, PLANE_SEP) \
static struct GfxLayout LAYOUT =                                   \
{                                                                  \
        32,32,   /* 32*32 tiles */                                 \
        TILES,   /* ????  tiles */                                 \
        4,       /* 4 bits per pixel */                            \
        { PLANE_SEP+8,PLANE_SEP,8,0},                                        \
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


/********************************************************************

               Alternative Graphics Layout macros

********************************************************************/

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



#define CHAR_LAYOUT2(LAYOUT, CHARS, PLANE_SEP) \
static struct GfxLayout LAYOUT = \
{                                 \
        8,8,   /* 8*8 tiles */     \
        CHARS,  /* ???? characters */    \
        4,       /* 4 bits per pixel */   \
        {3*PLANE_SEP,2*PLANE_SEP,PLANE_SEP,0},   \
        { 0,1,2,3,4,5,6,7, },                  \
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, },    \
        8*8    /* every sprite takes 8*8 consecutive bytes */ \
};

#define SPRITE_SEP2_2 0x40000*8
#define SPRITE_LAYOUT2(LAYOUT, SPRITES) \
static struct GfxLayout LAYOUT = \
{                                               \
        16,16,   /* 16*16 sprites */             \
        SPRITES,  /* ???? sprites */            \
        4,       /* 4 bits per pixel */            \
        {0x100000*8, 0x180000*8, 0,0x80000*8 },        \
        {0,1,2,3,4,5,6,7,\
          SPRITE_SEP2_2+0,SPRITE_SEP2_2+1,SPRITE_SEP2_2+2,SPRITE_SEP2_2+3, \
          SPRITE_SEP2_2+4,SPRITE_SEP2_2+5,SPRITE_SEP2_2+6,SPRITE_SEP2_2+7,  \
           },\
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, \
           8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8, }, \
        16*8    /* every sprite takes 32*8*2 consecutive bytes */ \
};

#define TILE32_LAYOUT2(LAYOUT, TILES, SEP) \
        TILE32_LAYOUT3(LAYOUT, TILES, SEP, 0x80000*8)

#define TILE32_LAYOUT3(LAYOUT, TILES, SEP, PLANE_SEP) \
static struct GfxLayout LAYOUT =                                   \
{                                                                  \
        32,32,   /* 32*32 tiles */                                 \
        TILES,   /* ????  tiles */                                 \
        4,       /* 4 bits per pixel */                            \
        {2*PLANE_SEP, 3*PLANE_SEP, 0,PLANE_SEP  },                                  \
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


/********************************************************************

                    Configuration table:

********************************************************************/

static struct CPS1config cps1_config_table[]=
{
  /* DRIVER    START  START  START  START      SPACE  SPACE  SPACE
      NAME     SCRL1   OBJ   SCRL2  SCRL3  A   SCRL1  SCRL2  SCRL3  */
  {"strider",      0,0x0200,0x1000,     0, 4, 0x0020,0x0020,0x0020},
  {"striderj",     0,0x0200,0x1000,     0, 4, 0x0020,0x0020,0x0020},
  {"ffight",  0x0400,     0,0x2000,0x0200, 2, 0x4420,0x3000,0x0980},
  {"ffightj", 0x0400,     0,0x2000,0x0200, 2, 0x4420,0x3000,0x0980},
  {"mtwins",       0,     0,0x2000,0x0e00, 3, 0x0020,0x0000,0x0000},
  {"chikij",       0,     0,0x2000,0x0e00, 3, 0x0020,0x0000,0x0000},
  {"unsquad",      0,     0,0x2000,     0, 0, 0x0020,0x0000,0x0000},
  {"willow",       0,     0,     0,0x0600, 1, 0x7020,0x0000,0x0a00},
  {"willowj",      0,     0,     0,0x0600, 1, 0x7020,0x0000,0x0a00},
  {"msword",       0,     0,0x2800,0x0e00, 3,     -1,    -1,    -1},
  {"pnickj",       0,0x0800,0x0800,0xffff, 0,     -1,    -1,    -1},
  {"knights", 0x0800,0x0000,0x4c00,0x1a00, 3,     -1,    -1,    -1},
  {"cawingj",      0,     0,0x2c00,0x0600, 0,     -1,    -1,    -1},
  {"ghouls",  0x0000,0x0000,0x2000,0x0500, 5, 0x2420,0x2000,0x0500, 0x0800, 1},

  /* End of table (default values) */
  {0,              0,     0,     0,     0, 0,     -1,    -1,    -1},
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

        if (stricmp(gamename, "cawingj")==0)
        {
                /* Quick hack (NOP out CPSB test) */
                if (errorlog)
                {
                   fprintf(errorlog, "Patching CPSB test\n");
                }
                WRITE_WORD(&RAM[0x04ca], 0x4e71);
                WRITE_WORD(&RAM[0x04cc], 0x4e71);
        }

        cps1_game_config=pCFG;

        if (pCFG->space_scroll1 != -1)
        {
                pCFG->space_scroll1 &= 0xfff;
        }

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

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0xc0, 0xc0, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(    0xc0, "Upright 1 Player" )
        PORT_DIPSETTING(    0x80, "Upright 1,2 Players" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
        /* 0x40 Cocktail */
        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "Easiest" )
        PORT_DIPSETTING(    0x05, "Easier" )
        PORT_DIPSETTING(    0x06, "Easy" )
        PORT_DIPSETTING(    0x07, "Normal" )
        PORT_DIPSETTING(    0x03, "Medium" )
        PORT_DIPSETTING(    0x02, "Hard" )
        PORT_DIPSETTING(    0x01, "Harder" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x08, 0x00, "Continue Coinage ?", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1 Coin" )
        PORT_DIPSETTING(    0x08, "2 Coins")
        /* Continue coinage needs working coin input to be tested */
        PORT_DIPNAME( 0x30, 0x30, "Bonus", IP_KEY_NONE )
        PORT_DIPSETTING(    0x10, "20000 60000")
        PORT_DIPSETTING(    0x00, "30000 60000" )
        PORT_DIPSETTING(    0x30, "20000 40000 and every 60000" )
        PORT_DIPSETTING(    0x20, "30000 50000 and every 70000" )
        PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE ) /* Unused ? */
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE ) /* Unused ? */
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "3" )
        PORT_DIPSETTING(    0x02, "4" )
        PORT_DIPSETTING(    0x01, "5" )
        PORT_DIPSETTING(    0x00, "6" )
        PORT_DIPNAME( 0x04, 0x04, "Freeze Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x08, 0x00, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x10, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 3 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 4 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

CHAR_LAYOUT(charlayout_strider, 2048, 0x100000*8)
SPRITE_LAYOUT(spritelayout_strider, 0x2600, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_strider,   8192-2048, 0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_strider, 4096-512,  0x080000*8, 0x100000*8)

static struct GfxDecodeInfo gfxdecodeinfo_strider[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x2f0000, &charlayout_strider,    0,                      32 },
        { 1, 0x004000, &spritelayout_strider,  32*16,                  32 },
        { 1, 0x050000, &tilelayout_strider,    32*16+32*16,            32 },
        { 1, 0x200000, &tilelayout32_strider,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        strider_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_strider,
        CPS1_DEFAULT_CPU_SPEED)

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
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
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

MACHINE_DRIVER(
        striderj_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_strider,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( striderj_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_WIDE_SWAP( "sthj23.bin", 0x00000, 0x80000, 0x651ad9fa ) /* Tile map */
        ROM_LOAD_WIDE_SWAP( "sthj22.bin", 0x80000, 0x80000, 0x59b99ecf ) /* Tile map */

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "sth03.bin",   0x000000, 0x80000, 0x5f694f7b ) /* sprites */
        ROM_LOAD( "sth01.bin",   0x080000, 0x80000, 0x19fbe6a7 ) /* sprites */
        ROM_LOAD( "sth04.bin",   0x100000, 0x80000, 0x2c9017c2 ) /* sprites */
        ROM_LOAD( "sth02.bin",   0x180000, 0x80000, 0x3b0e4930 ) /* sprites */

        ROM_LOAD( "sth07.bin",   0x200000, 0x80000, 0x0e0c0fb8 ) /* tiles + chars */
        ROM_LOAD( "sth05.bin",   0x280000, 0x80000, 0x542a60c8 ) /* tiles + chars */
        ROM_LOAD( "sth08.bin",   0x300000, 0x80000, 0x6c8b64c9 ) /* tiles + chars */
        ROM_LOAD( "sth06.bin",   0x380000, 0x80000, 0x27932793 ) /* tiles + chars */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "sth09.bin",    0x00000, 0x10000, 0x85adabcd )
        ROM_LOAD( "sth09.bin",    0x08000, 0x10000, 0x85adabcd )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("sth18.bin",    0x00000, 0x20000, 0x6e80d9f2 )
        ROM_LOAD ("sth19.bin",    0x20000, 0x20000, 0xc906d49a )
ROM_END

struct GameDriver striderj_driver =
{
        "Strider (Japanese)",
        "striderj",
        CPS1_CREDITS ("Marco Cassili (Game Driver)"),
        &striderj_machine_driver,

        striderj_rom,
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

INPUT_PORTS_START( input_ports_willow )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x18, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
        PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
        PORT_DIPNAME( 0xc0, 0xc0, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(    0xc0, "Upright 1 Player" )
        PORT_DIPSETTING(    0x80, "Upright 1,2 Players" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
        /* 0x40 Cocktail */
        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "Very easy" )
        PORT_DIPSETTING(    0x05, "Easier" )
        PORT_DIPSETTING(    0x06, "Easy" )
        PORT_DIPSETTING(    0x07, "Normal" )
        PORT_DIPSETTING(    0x03, "Medium" )
        PORT_DIPSETTING(    0x02, "Hard" )
        PORT_DIPSETTING(    0x01, "Harder" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x18, 0x18, "Nando Speed", IP_KEY_NONE )
        PORT_DIPSETTING(    0x10, "Slow")
        PORT_DIPSETTING(    0x18, "Normal")
        PORT_DIPSETTING(    0x08, "Fast")
        PORT_DIPSETTING(    0x00, "Very Fast" )
        PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE ) /* Unused ? */
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPSETTING(    0x20, "Off" )
        PORT_DIPNAME( 0x40, 0x20, "Unknown", IP_KEY_NONE ) /* Unused ? */
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Stage Magic Continue (power up?)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )
        /* The test mode reports Stage Magic, a file with dip says if
         power up are on the player gets sword and magic item without having
         to buy them. To test */
        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x02, "1" )
        PORT_DIPSETTING(    0x03, "2" )
        PORT_DIPSETTING(    0x01, "3" )
        PORT_DIPSETTING(    0x00, "4" )
        PORT_DIPNAME( 0x0c, 0x0c, "Vitality", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "2" )
        PORT_DIPSETTING(    0x0c, "3")
        PORT_DIPSETTING(    0x08, "4" )
        PORT_DIPSETTING(    0x04, "5")
        PORT_DIPNAME( 0x10, 0x10, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


CHAR_LAYOUT(charlayout_willow, 4096, 0x100000*8)
TILE32_LAYOUT(tile32layout_willow, 1024,    0x080000*8, 0x100000*8)
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
        gfxdecodeinfo_willow,
        CPS1_DEFAULT_CPU_SLOW_SPEED)

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
        ROM_LOAD( "WL_09.ROM",    0x00000, 0x08000, 0x56014501 )
                    ROM_CONTINUE( 0x10000, 0x08000 )


        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("WL_18.ROM",    0x00000, 0x20000, 0x58bb142b )
        ROM_LOAD ("WL_19.ROM",    0x20000, 0x20000, 0xc575f23d )
ROM_END

struct GameDriver willow_driver =
{
        "Willow",
        "willow",
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        &willow_machine_driver,
        willow_rom,
        0,0,0,0,
        input_ports_willow,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};


ROM_START( willowj_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN("WL36.bin", 0x00000, 0x20000, 0x739e4660 )
        ROM_LOAD_ODD ("WL42.bin", 0x00000, 0x20000, 0x2eb180d7 )
        ROM_LOAD_EVEN("WL37.bin", 0x40000, 0x20000, 0x53d167b3 )
        ROM_LOAD_ODD ("WL43.bin", 0x40000, 0x20000, 0x84a91c11 )
        ROM_LOAD_EVEN("WL34.bin", 0x80000, 0x20000, 0xd09be8a9 )
        ROM_LOAD_ODD ("WL40.bin", 0x80000, 0x20000, 0x3806c3ac )
        ROM_LOAD_EVEN("WL35.bin", 0xc0000, 0x20000, 0x8027957d )
        ROM_LOAD_ODD ("WL41.bin", 0xc0000, 0x20000, 0x8457999f )

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */

        ROM_LOAD_EVEN("WL17.bin", 0x000000, 0x20000, 0x3f832a2b )
        ROM_LOAD_ODD ("WL24.bin", 0x000000, 0x20000, 0x95a17755 )
        ROM_LOAD_EVEN("WL18.bin", 0x040000, 0x20000, 0x0cc65b6a )
        ROM_LOAD_ODD ("WL25.bin", 0x040000, 0x20000, 0xe525fb27 )

        ROM_LOAD_EVEN("WL32.bin", 0x100000, 0x20000, 0xc0bcbf46 )
        ROM_LOAD_ODD ("WL38.bin", 0x100000, 0x20000, 0xcdc6ce78 )
        ROM_LOAD_EVEN("WL33.bin", 0x140000, 0x20000, 0xcba5e8e5 )
        ROM_LOAD_ODD ("WL39.bin", 0x140000, 0x20000, 0x49252bdd )

        ROM_LOAD_EVEN("WL01.bin", 0x080000, 0x20000, 0x127618c2 )
        ROM_LOAD_ODD ("WL09.bin", 0x080000, 0x20000, 0x5d1f617b )
        ROM_LOAD_EVEN("WL02.bin", 0x0c0000, 0x20000, 0xff7474f4 )
        ROM_LOAD_ODD ("WL10.bin", 0x0c0000, 0x20000, 0xc3ca8ba6 )

        ROM_LOAD_EVEN("WL05.bin", 0x180000, 0x20000, 0xdbe12d2b )
        ROM_LOAD_ODD ("WL13.bin", 0x180000, 0x20000, 0x364281d8 )
        ROM_LOAD_EVEN("WL06.bin", 0x1c0000, 0x20000, 0x72874a0f )
        ROM_LOAD_ODD ("WL14.bin", 0x1c0000, 0x20000, 0x88d92307 )

        ROM_LOAD( "WL26.bin",     0x200000, 0x20000, 0x67c34af5 )
        ROM_LOAD( "WL19.bin",     0x220000, 0x20000, 0xc8c6fe3c )
        ROM_LOAD( "WL28.bin",     0x240000, 0x20000, 0x6a95084b )
        ROM_LOAD( "WL21.bin",     0x260000, 0x20000, 0x66735153 )

        ROM_LOAD( "WL11.bin",     0x280000, 0x20000, 0x839200be )
        ROM_LOAD( "WL03.bin",     0x2a0000, 0x20000, 0x6cf8259c )
        ROM_LOAD( "WL15.bin",     0x2c0000, 0x20000, 0xeef53eb5 )
        ROM_LOAD( "WL07.bin",     0x2e0000, 0x20000, 0x6a02dfde )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "WL23.bin",    0x00000, 0x10000, 0x56014501 )
        ROM_LOAD( "WL23.bin",    0x08000, 0x10000, 0x56014501 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("WL30.bin",    0x00000, 0x20000, 0x58bb142b )
        ROM_LOAD ("WL31.bin",    0x20000, 0x20000, 0xc575f23d )
ROM_END

struct GameDriver willowj_driver =
{
        "Willow (Japanese)",
        "willowj",
        CPS1_CREDITS ("Marco Cassili (Game Driver)"),
        &willow_machine_driver,
        willowj_rom,
        0,0,0,0,
        input_ports_willow,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};


/********************************************************************

                              FINAL FIGHT
                              ===========

********************************************************************/

INPUT_PORTS_START( input_ports_ffight )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x40, 0x40, "Force 2 Coins/1 Credit (1 to continue if On)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x04, "Difficulty Level 1", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Very easy" )
        PORT_DIPSETTING(    0x06, "Easier" )
        PORT_DIPSETTING(    0x05, "Easy" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Medium" )
        PORT_DIPSETTING(    0x02, "Hard" )
        PORT_DIPSETTING(    0x01, "Harder" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x18, 0x10, "Difficulty Level 2", IP_KEY_NONE )
        PORT_DIPSETTING(    0x18, "Easy")
        PORT_DIPSETTING(    0x10, "Normal")
        PORT_DIPSETTING(    0x08, "Hard")
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x60, 0x60, "Bonus", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "None" )
        PORT_DIPSETTING(    0x60, "100000" )
        PORT_DIPSETTING(    0x40, "200000" )
        PORT_DIPSETTING(    0x20, "100000 and every 200000" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1" )
        PORT_DIPSETTING(    0x03, "2" )
        PORT_DIPSETTING(    0x02, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Freeze Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x00, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

CHAR_LAYOUT(charlayout_ffight,     2048, 0x100000*8)
SPRITE_LAYOUT(spritelayout_ffight, 4096*2+512, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_ffight,   4096       , 0x80000*8, 0x100000*8)
TILE32_LAYOUT(tile32layout_ffight, 512+128,     0x080000*8, 0x100000*8)

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
        gfxdecodeinfo_ffight,
        10000000)                /* 10 MHz */

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
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        &ffight_machine_driver,
        ffight_rom,
        0,0,0,0,
        input_ports_ffight,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};

MACHINE_DRIVER(
        ffightj_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_ffight,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( ffightj_rom )
        ROM_REGION(0x100000)      /* 68000 code */

        ROM_LOAD_EVEN("ff36.bin", 0x00000, 0x20000, 0x3ad2c910 )
        ROM_LOAD_ODD ("ff42.bin", 0x00000, 0x20000, 0xe2c402c0 )
        ROM_LOAD_EVEN("ff37.bin", 0x40000, 0x20000, 0x2bfa6a30 )
        ROM_LOAD_ODD ("ff43.bin", 0x40000, 0x20000, 0x7bdad98c )

        /* tile map */
        ROM_LOAD_EVEN("ff34.bin", 0x80000, 0x20000, 0xcb2b1811 )
        ROM_LOAD_ODD ("ff40.bin", 0x80000, 0x20000, 0xbd47e80b )
        ROM_LOAD_EVEN("ff35.bin", 0xc0000, 0x20000, 0x05033f3b )
        ROM_LOAD_ODD ("ff41.bin", 0xc0000, 0x20000, 0xf13d5581 )

        ROM_REGION(0x500000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD_EVEN("ff17.bin",    0x000000, 0x20000, 0x13b69444 )
        ROM_LOAD_ODD ("ff24.bin",    0x000000, 0x20000, 0x1726d636 )
        ROM_LOAD_EVEN("ff18.bin",    0x040000, 0x20000, 0x7830bc52 )
        ROM_LOAD_ODD ("ff25.bin",    0x040000, 0x20000, 0x9511fc3d )

        ROM_LOAD_EVEN("ff01.bin",    0x080000, 0x20000, 0x0984eeda )
        ROM_LOAD_ODD ("ff09.bin",    0x080000, 0x20000, 0xc1e8dc4a )
        ROM_LOAD_EVEN("ff02.bin",    0x0c0000, 0x20000, 0x43d4f946 )
        ROM_LOAD_ODD ("ff10.bin",    0x0c0000, 0x20000, 0xec62a61c )

        ROM_LOAD_EVEN("ff32.bin",    0x100000, 0x20000, 0x83f02f16 )
        ROM_LOAD_ODD ("ff38.bin",    0x100000, 0x20000, 0x70a1a353 )
        ROM_LOAD_EVEN("ff33.bin",    0x140000, 0x20000, 0x20d1ff3d )
        ROM_LOAD_ODD ("ff39.bin",    0x140000, 0x20000, 0x284c670a )

        ROM_LOAD_EVEN("ff05.bin",    0x180000, 0x20000, 0xa7ba3032 )
        ROM_LOAD_ODD ("ff13.bin",    0x180000, 0x20000, 0x79c64ef2 )
        ROM_LOAD_EVEN("ff06.bin",    0x1c0000, 0x20000, 0x03674781 )
        ROM_LOAD_ODD ("ff14.bin",    0x1c0000, 0x20000, 0xe6caa850 )


        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ff23.bin",    0x00000, 0x10000, 0x06b2f7f8 )
        ROM_LOAD( "ff23.bin",    0x08000, 0x10000, 0x06b2f7f8 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("ff30.bin",    0x00000, 0x20000, 0x42467b56 )
        ROM_LOAD ("ff31.bin",    0x20000, 0x20000, 0xabd11a71 )
ROM_END

struct GameDriver ffightj_driver =
{
        "Final Fight (Japanese)",
        "ffightj",
        CPS1_CREDITS("Marco Cassili (Game Driver)"),
        &ffightj_machine_driver,
        ffightj_rom,
        0,0,0,0,
        input_ports_ffight,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};


/********************************************************************

                          UN Squadron

********************************************************************/

INPUT_PORTS_START( input_ports_unsquad )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x18, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
        PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
//        PORT_DIPNAME( 0xc0, 0xc0, "Unknown", IP_KEY_NONE )
//        PORT_DIPSETTING(    0x00, "On" )
//        PORT_DIPSETTING(    0xc0, "Off" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Super Easy" )
        PORT_DIPSETTING(    0x06, "Very Easy" )
        PORT_DIPSETTING(    0x05, "Easy" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Difficult" )
        PORT_DIPSETTING(    0x02, "Very Difficult" )
        PORT_DIPSETTING(    0x01, "Super Difficult" )
        PORT_DIPSETTING(    0x00, "Ultra Super Difficult" )
//        PORT_DIPNAME( 0xf8, 0xf8, "Unknown", IP_KEY_NONE )
//        PORT_DIPSETTING(    0x00, "On")
//        PORT_DIPSETTING(    0xf8, "Off")

        PORT_START      /* DSWC */
//        PORT_DIPNAME( 0x03, 0x03, "Unknown", IP_KEY_NONE )
//        PORT_DIPSETTING(    0x00, "On" )
//        PORT_DIPSETTING(    0x03, "Off" )
        PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Freeze Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x10, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


CHAR_LAYOUT(charlayout_unsquad, 4096, 0x100000*8)
SPRITE_LAYOUT(spritelayout_unsquad, 4096*2-2048, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_unsquad, 8192-2048,  0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_unsquad, 1024,  0x080000*8, 0x100000*8)

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
        gfxdecodeinfo_unsquad,
        CPS1_DEFAULT_CPU_SPEED)

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
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        &unsquad_machine_driver,

        unsquad_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_unsquad,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};


/********************************************************************

                          Mega Twins

********************************************************************/

INPUT_PORTS_START( input_ports_mtwins )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Super Easy" )
        PORT_DIPSETTING(    0x06, "Very Easy" )
        PORT_DIPSETTING(    0x05, "Easy" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Difficult" )
        PORT_DIPSETTING(    0x02, "Very Difficult" )
        PORT_DIPSETTING(    0x01, "Super Difficult" )
        PORT_DIPSETTING(    0x00, "Ultra Super Difficult" )
        PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off" )
        PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "1" )
        PORT_DIPSETTING(    0x10, "2" )
        PORT_DIPSETTING(    0x00, "3" )
        /*  0x30 gives 1 life */
        PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x01, "Off" )
        PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x02, "Off" )
        PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Freeze Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x10, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


CHAR_LAYOUT(charlayout_mtwins,  4096, 0x100000*8)
SPRITE_LAYOUT(spritelayout_mtwins, 4096*2-2048, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_mtwins, 8192-2048, 0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_mtwins, 512,  0x080000*8, 0x100000*8)

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
        gfxdecodeinfo_mtwins,
        CPS1_DEFAULT_CPU_SPEED)

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
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        &mtwins_machine_driver,

        mtwins_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_mtwins,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};



MACHINE_DRIVER(
        chikij_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_mtwins,
        CPS1_DEFAULT_CPU_SPEED)


ROM_START( chikij_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN("chj36a.bin",  0x00000, 0x20000, 0xda26ace2 )
        ROM_LOAD_ODD ("chj42a.bin",  0x00000, 0x20000, 0xdd5b399f )
        ROM_LOAD_EVEN("chj37a.bin",  0x40000, 0x20000, 0xd7301e44 )
        ROM_LOAD_ODD ("chj43a.bin",  0x40000, 0x20000, 0xbd73ffed )
        /* Tile map */
        ROM_LOAD_EVEN("ch34.bin",    0x80000, 0x20000, 0x13130ed1 )
        ROM_LOAD_ODD ("ch40.bin",    0x80000, 0x20000, 0x29becc62 )
        ROM_LOAD_EVEN("ch35.bin",    0xc0000, 0x20000, 0x5dcbb5d1 )
        ROM_LOAD_ODD ("ch41.bin",    0xc0000, 0x20000, 0xf6e3f2a3 )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD_EVEN("ch17.bin",    0x000000, 0x20000, 0x372677a2 )
        ROM_LOAD_ODD ("ch24.bin",    0x000000, 0x20000, 0x60a77d7f )
        ROM_LOAD_EVEN("ch18.bin",    0x040000, 0x20000, 0xd8f01198 )
        ROM_LOAD_ODD ("ch25.bin",    0x040000, 0x20000, 0x2db187d5 )

        ROM_LOAD_EVEN("ch01.bin",    0x080000, 0x20000, 0xca3bc09b )
        ROM_LOAD_ODD ("ch09.bin",    0x080000, 0x20000, 0xc94d9917 )
        ROM_LOAD_EVEN("ch02.bin",    0x0c0000, 0x20000, 0x50176cc9 )
        ROM_LOAD_ODD ("ch10.bin",    0x0c0000, 0x20000, 0x490c4012 )

        ROM_LOAD_EVEN("ch32.bin",    0x100000, 0x20000, 0xf7231989 )
        ROM_LOAD_ODD ("ch38.bin",    0x100000, 0x20000, 0x01f7df53 )
        ROM_LOAD_EVEN("ch33.bin",    0x140000, 0x20000, 0xa6a2bdf2 )
        ROM_LOAD_ODD ("ch39.bin",    0x140000, 0x20000, 0xdfdeb232 )

        ROM_LOAD_EVEN("ch05.bin",    0x180000, 0x20000, 0xcffc1812 )
        ROM_LOAD_ODD ("ch13.bin",    0x180000, 0x20000, 0x7e0863ec )
        ROM_LOAD_EVEN("ch06.bin",    0x1c0000, 0x20000, 0x65fdb55f )
        ROM_LOAD_ODD ("ch14.bin",    0x1c0000, 0x20000, 0xec2aa82e )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ch23.bin",    0x00000, 0x10000, 0x9dfa57d4 )
        ROM_LOAD( "ch23.bin",    0x08000, 0x10000, 0x9dfa57d4 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("ch30.bin",    0x00000, 0x20000, 0x5c96f786 )
        ROM_LOAD ("ch31.bin",    0x20000, 0x20000, 0xb12d19a5 )
ROM_END


struct GameDriver chikij_driver =
{
        "Chiki Chiki Boys",
        "chikij",
        CPS1_CREDITS("Marco Cassili (Game Driver)"),
        &chikij_machine_driver,

        chikij_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_mtwins,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};

/********************************************************************

                          Magic Sword

********************************************************************/

INPUT_PORTS_START( input_ports_msword )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x40, 0x40, "Force 2 Coins/1 Credit (1 to continue if On)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x04, "Level 1", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Easiest" )
        PORT_DIPSETTING(    0x06, "Very Easy" )
        PORT_DIPSETTING(    0x05, "Easy" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Difficult" )
        PORT_DIPSETTING(    0x02, "Hard" )
        PORT_DIPSETTING(    0x01, "Very Hard" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x38, 0x38, "Level 2", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "Easiest" )
        PORT_DIPSETTING(    0x28, "Very Easy" )
        PORT_DIPSETTING(    0x30, "Easy" )
        PORT_DIPSETTING(    0x38, "Normal" )
        PORT_DIPSETTING(    0x18, "Difficult" )
        PORT_DIPSETTING(    0x10, "Hard" )
        PORT_DIPSETTING(    0x08, "Very Hard" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x40, 0x40, "Stage Select", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Vitality Bar", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1" )
        PORT_DIPSETTING(    0x03, "2" )
        PORT_DIPSETTING(    0x02, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Stop Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x10, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off")
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, "Test Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

CHAR_LAYOUT(charlayout_msword,  4096, 0x100000*8);
SPRITE_LAYOUT(spritelayout_msword, 8192, 0x80000*8, 0x100000*8);
SPRITE_LAYOUT(tilelayout_msword, 4096, 0x80000*8, 0x100000*8);
TILE32_LAYOUT(tilelayout32_msword, 512,  0x080000*8, 0x100000*8);

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
        gfxdecodeinfo_msword,
        CPS1_DEFAULT_CPU_SPEED);

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
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        &msword_machine_driver,

        msword_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_msword,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};


/********************************************************************

                          Pnickies (Japanese)

 Scroll 3 graphics not decoded yet.

********************************************************************/

INPUT_PORTS_START( input_ports_pnickj )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_START1 , "Left Player Start", IP_KEY_DEFAULT, IP_JOY_NONE, 0 )
        PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_START2 , "Right Player Start", IP_KEY_DEFAULT, IP_JOY_NONE, 0 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coinage", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x08, 0x08, "Chuter Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "1 Chuter" )
        PORT_DIPSETTING(    0x00, "2 Chuters" )
        PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off" )
        PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off" )
        PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x04, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Easiest" )
        PORT_DIPSETTING(    0x06, "Very Easy" )
        PORT_DIPSETTING(    0x05, "Easy" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Hard" )
        PORT_DIPSETTING(    0x02, "Very Hard" )
        PORT_DIPSETTING(    0x01, "Hardest" )
        PORT_DIPSETTING(    0x00, "Master Level" )
        PORT_DIPNAME( 0x08, 0x00, "Unknkown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x30, 0x30, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1")
        PORT_DIPSETTING(    0x10, "2" )
        PORT_DIPSETTING(    0x20, "3" )
        PORT_DIPSETTING(    0x30, "4" )
        PORT_DIPNAME( 0xc0, 0xc0, "Vs Play Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0xc0, "1 Game Match" )
        PORT_DIPSETTING(    0x80, "3 Games Match" )
        PORT_DIPSETTING(    0x40, "5 Games Match" )
        PORT_DIPSETTING(    0x00, "7 Games Match" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "1" )
        PORT_DIPSETTING(    0x02, "2" )
        PORT_DIPSETTING(    0x01, "3" )
        PORT_DIPSETTING(    0x00, "4" )
        PORT_DIPNAME( 0x04, 0x04, "Freeze Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Stop Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x10, "Video Flip", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/*

Massive memory wastage. Scroll 2 and the objects use the same
character set. There are two copies resident in memory.

*/

CHAR_LAYOUT2(charlayout_pnickj, 4096, 0x80000*8);
SPRITE_LAYOUT2(spritelayout_pnickj, 0x2800);
SPRITE_LAYOUT2(tilelayout_pnickj,   0x2800);
TILE32_LAYOUT2(tilelayout32_pnickj, 1,  0x40000*8 );

static struct GfxDecodeInfo gfxdecodeinfo_pnickj[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x000000, &charlayout_pnickj,    0,                      32 },
        { 1, 0x008000, &spritelayout_pnickj,  32*16,                  32 },
        { 1, 0x008000, &tilelayout_pnickj,    32*16+32*16,            32 },
        { 1, 0x030000, &tilelayout32_pnickj,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};


MACHINE_DRIVER(
        pnickj_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_pnickj,
        CPS1_DEFAULT_CPU_SPEED);

ROM_START( pnickj_rom )
        ROM_REGION(0x040000)      /* 68000 code */
        ROM_LOAD_EVEN("PNIJ36.BIN", 0x00000, 0x20000, 0xdd4c06f8 )
        ROM_LOAD_ODD("PNIJ42.BIN",  0x00000, 0x20000, 0x31a6c64a )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */

        ROM_LOAD( "PNIJ01.BIN",   0x000000, 0x20000, 0x7d70f1ae )
        ROM_LOAD( "PNIJ02.BIN",   0x020000, 0x20000, 0x0920aab2 )
        ROM_LOAD( "PNIJ18.BIN",   0x040000, 0x20000, 0x69c6ec06 )
        ROM_LOAD( "PNIJ19.BIN",   0x060000, 0x20000, 0x699426fe )

        ROM_LOAD( "PNIJ09.BIN",   0x080000, 0x20000, 0x084f08d5 )
        ROM_LOAD( "PNIJ10.BIN",   0x0a0000, 0x20000, 0x82453ef5 )
        ROM_LOAD( "PNIJ26.BIN",   0x0c0000, 0x20000, 0x1222eeb4 )
        ROM_LOAD( "PNIJ27.BIN",   0x0e0000, 0x20000, 0x58a385ed )

        ROM_LOAD( "PNIJ05.BIN",   0x100000, 0x20000, 0xa524baa6 )
        ROM_LOAD( "PNIJ06.BIN",   0x120000, 0x20000, 0xecf87970 )
        ROM_LOAD( "PNIJ32.BIN",   0x140000, 0x20000, 0xca010ba7 )
        ROM_LOAD( "PNIJ33.BIN",   0x160000, 0x20000, 0x6743e579 )

        ROM_LOAD( "PNIJ13.BIN",   0x180000, 0x20000, 0x9914a0fe )
        ROM_LOAD( "PNIJ14.BIN",   0x1a0000, 0x20000, 0x03c84eda )
        ROM_LOAD( "PNIJ38.BIN",   0x1c0000, 0x20000, 0x5cb05f98 )
        ROM_LOAD( "PNIJ39.BIN",   0x1e0000, 0x20000, 0xe561e0a9 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "PNIJ17.BIN",    0x00000, 0x10000, 0x06a99847 )
        ROM_LOAD( "PNIJ17.BIN",    0x08000, 0x10000, 0x06a99847 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("PNIJ24.BIN",    0x00000, 0x20000, 0x5aaf13c5 )
        ROM_LOAD ("PNIJ25.BIN",    0x20000, 0x20000, 0x86d371df )
ROM_END

struct GameDriver pnickj_driver =
{
        "Pnickies (Japanese)",
        "pnickj",
        CPS1_CREDITS("Paul Leaman (Game Driver)"),
        &pnickj_machine_driver,

        pnickj_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_pnickj,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};



/********************************************************************

                          Knights of the Round

 Input ports don't work at all. Sprites a little messed up.

********************************************************************/


INPUT_PORTS_START( input_ports_knights )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) /* service */
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B?", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x38, "Off" )
        PORT_DIPNAME( 0x40, 0x40, "Force 2 Coins/1 Credit (1 to continue if On)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x04, "Player speed and vitality consumption", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Very easy" )
        PORT_DIPSETTING(    0x06, "Easier" )
        PORT_DIPSETTING(    0x05, "Easy" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Medium" )
        PORT_DIPSETTING(    0x02, "Hard" )
        PORT_DIPSETTING(    0x01, "Harder" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x38, 0x38, "Enemy's vitality and attack power", IP_KEY_NONE )
        PORT_DIPSETTING(    0x10, "Very Easy" )
        PORT_DIPSETTING(    0x08, "Easier" )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPSETTING(    0x38, "Normal" )
        PORT_DIPSETTING(    0x30, "Medium" )
        PORT_DIPSETTING(    0x28, "Hard" )
        PORT_DIPSETTING(    0x20, "Harder" )
        PORT_DIPSETTING(    0x18, "Hardest" )
        PORT_DIPNAME( 0x40, 0x40, "Coin Shooter", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1" )
        PORT_DIPSETTING(    0x40, "3" )
        PORT_DIPNAME( 0x80, 0x80, "Play type", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "2 Players" )
        PORT_DIPSETTING(    0x80, "3 Players" )
        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1" )
        PORT_DIPSETTING(    0x03, "2" )
        PORT_DIPSETTING(    0x02, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x00, "Flip Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x00, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off")
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, "Test Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 3 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


CHAR_LAYOUT(charlayout_knights,     0x1000, 0x200000*8)
SPRITE_LAYOUT(spritelayout_knights, 0x4800, 0x100000*8, 0x200000*8)
SPRITE_LAYOUT(tilelayout_knights,   0x1d00, 0x100000*8, 0x200000*8)
TILE32_LAYOUT(tilelayout32_knights, 2048-512,  0x100000*8, 0x200000*8)

static struct GfxDecodeInfo gfxdecodeinfo_knights[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x088000, &charlayout_knights,    0,                      32 },
        { 1, 0x000000, &spritelayout_knights,  32*16,                  32 },
        { 1, 0x098000, &tilelayout_knights,    32*16+32*16,            32 },
        { 1, 0x0d0000, &tilelayout32_knights,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(
        knights_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_knights,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( knights_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_WIDE_SWAP("KR_23E.ROM", 0x00000, 0x80000, 0x79b18275 )
        ROM_LOAD_WIDE_SWAP("KR_22.ROM", 0x80000, 0x80000, 0x006ee1da )

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "KR_GFX2.ROM",   0x000000, 0x80000, 0xb4dd6223 )
        ROM_LOAD( "KR_GFX6.ROM",   0x080000, 0x80000, 0x6c092895 )
        ROM_LOAD( "KR_GFX1.ROM",   0x100000, 0x80000, 0xbc0f588f )
        ROM_LOAD( "KR_GFX5.ROM",   0x180000, 0x80000, 0x5917bf8f )
        ROM_LOAD( "KR_GFX4.ROM",   0x200000, 0x80000, 0xf30df6ad )
        ROM_LOAD( "KR_GFX8.ROM",   0x280000, 0x80000, 0x21a5b3b5 )
        ROM_LOAD( "KR_GFX3.ROM",   0x300000, 0x80000, 0x07a98709 )
        ROM_LOAD( "KR_GFX7.ROM",   0x380000, 0x80000, 0x5b53bdb9 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "KR_09.ROM",    0x00000, 0x10000, 0x795a9f98 )
        ROM_LOAD( "KR_09.ROM",    0x08000, 0x10000, 0x795a9f98 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("KR_18.ROM",    0x00000, 0x20000, 0x06633ed3 )
        ROM_LOAD ("KR_19.ROM",    0x20000, 0x20000, 0x750a1ca4 )
ROM_END

struct GameDriver knights_driver =
{
        "Knights of the Round",
        "knights",
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        &knights_machine_driver,

        knights_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_knights,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};

/********************************************************************

                          GHOULS AND GHOSTS

  Sprites are split across 2 different graphics layouts.


********************************************************************/

INPUT_PORTS_START( input_ports_ghouls )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME (0xff, 0xff, "DIP A", IP_KEY_NONE)
        PORT_DIPSETTING(    0xff, "OFF" )
        PORT_DIPSETTING(    0x00, "ON" )

        PORT_START      /* DSWB */
        PORT_DIPNAME (0xff, 0xff, "DIP B", IP_KEY_NONE)
        PORT_DIPSETTING(    0xff, "OFF" )
        PORT_DIPSETTING(    0x00, "ON" )

        PORT_START      /* DSWC */
        PORT_DIPNAME (0xff, 0xff, "DIP C", IP_KEY_NONE)
        PORT_DIPSETTING(    0xff, "OFF" )
        PORT_DIPSETTING(    0x00, "ON" )

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2)
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

#define SPRITE_SEP (0x040000*8)
static struct GfxLayout spritelayout2_ghouls =
{
        16,16,  /* 16*16 sprites */
        4096,   /* 4096  sprites ???? */
        4,      /* 4 bits per pixel */
        { 1*0x010000*8, 2*0x010000*8, 3*0x010000*8, 0*0x010000*8 },
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

CHAR_LAYOUT(charlayout_ghouls,     4096*2,   0x100000*8)
SPRITE_LAYOUT(spritelayout_ghouls, 4096,     0x080000*8, 0x100000*8 )
SPRITE_LAYOUT(tilelayout_ghouls,   4096*3,   0x080000*8, 0x100000*8 )


/* Can't work out the bottom half of this */
#define SEP2 16*8
#define TILE32_LAYOUT4(LAYOUT, TILES, SEP, PLANE_SEP) \
static struct GfxLayout LAYOUT =                                   \
{                                                                  \
        32,32,   /* 32*32 tiles */                                 \
        TILES,   /* ????  tiles */                                 \
        4,       /* 4 bits per pixel */                            \
        {0x30000*8,0x20000*8, 0x10000*8, 0},                       \
        {                                                          \
           0,1,2,3,4,5,6,7,8,\
           SEP+0, SEP+1, SEP+ 2, SEP+ 3, SEP+ 4, SEP+ 5, SEP+ 6, SEP+ 7, \
           9,10,11,12,13,14,15,                  \
           SEP+8, SEP+9, SEP+10, SEP+11, SEP+12, SEP+13, SEP+14, SEP+15, \
        },                                                         \
        {                                                          \
           0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,         \
           8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,   \
           SEP2+0*32, SEP2+1*32, SEP2+2*32, SEP2+3*32,\
           SEP2+4*32, SEP2+5*32, SEP2+6*32, SEP2+7*32,\
           SEP2+8*32, SEP2+9*32, SEP2+10*32, SEP2+11*32,\
           SEP2+12*32, SEP2+13*32, SEP2+14*32, SEP2+15*32,   \
        },                                                         \
        16*32 /* every sprite takes 32*8*4 consecutive bytes */\
};

TILE32_LAYOUT4(tilelayout32_ghouls, 1 /*4096*4-1*/,    0x040000*8, 0x020000*8 )

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x030000, &charlayout_ghouls,      0,                       32 },
        { 1, 0x000000, &spritelayout_ghouls,    32*16,                   32 },
        { 1, 0x040000, &tilelayout_ghouls,      32*16+32*16,             32 },
        { 1, 0x280000, &tilelayout32_ghouls,    32*16+32*16+32*16,       32 },
        { 1, 0x200000, &spritelayout2_ghouls,   32*16,                   32 },
	{ -1 } /* end of array */
};

MACHINE_DRV(
        ghouls_machine_driver,
        cps1_interrupt, 1,
        gfxdecodeinfo,
        CPS1_DEFAULT_CPU_SLOW_SPEED)

ROM_START( ghouls_rom )
        ROM_REGION(0x100000)
#if 1
        ROM_LOAD_EVEN("GHL29.BIN",   0x00000, 0x20000, 0x8821a2b3 ) /* 68000 code */
        ROM_LOAD_ODD ("GHL30.BIN",   0x00000, 0x20000, 0xaff1cd13 ) /* 68000 code */
        ROM_LOAD_EVEN("GHL27.BIN",   0x40000, 0x20000, 0x82a7d49d ) /* 68000 code */
        ROM_LOAD_ODD ("GHL28.BIN",   0x40000, 0x20000, 0xe32cdaa6 ) /* 68000 code */
#else
        /*
         Alternative set - boots up faster, game will reboot due to
         incorrect tile map data.
        */

        ROM_LOAD_EVEN("DMU.29",   0x00000, 0x20000, 0x90efb087 ) /* 68000 code */
        ROM_LOAD_ODD ("DMU.30",   0x00000, 0x20000, 0xac09ca89 ) /* 68000 code */
        ROM_LOAD_EVEN("DMU.27",   0x40000, 0x20000, 0x7bc0c8d8 ) /* 68000 code */
        ROM_LOAD_ODD ("DMU.28",   0x40000, 0x20000, 0xc07d69e3 ) /* 68000 code */
#endif

        ROM_LOAD_WIDE("GHL17.BIN",   0x80000, 0x80000, 0x12eee9a4 ) /* Tile map */

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */

        ROM_LOAD( "GHL6.BIN",       0x000000, 0x80000, 0xeaf5b7e3 ) /* Sprites / tiles */
        ROM_LOAD( "GHL5.BIN",       0x080000, 0x80000, 0xa4adc913 )
        ROM_LOAD( "GHL8.BIN",       0x100000, 0x80000, 0xe7fa3f94 )
        ROM_LOAD( "GHL7.BIN",       0x180000, 0x80000, 0x8587f2cb )

        ROM_LOAD( "GHL10.BIN",      0x200000, 0x10000, 0x1ad5edb3 ) /* Sprite set 2 */
        ROM_LOAD( "GHL23.BIN",      0x210000, 0x10000, 0xa1a14a87 )
        ROM_LOAD( "GHL14.BIN",      0x220000, 0x10000, 0xa1e55fd9 )
        ROM_LOAD( "GHL19.BIN",      0x230000, 0x10000, 0x142db95f )

        ROM_LOAD( "GHL12.BIN",      0x240000, 0x10000, 0xb0882fde )
        ROM_LOAD( "GHL25.BIN",      0x250000, 0x10000, 0x0f231d0f )
        ROM_LOAD( "GHL16.BIN",      0x260000, 0x10000, 0xc47e04ba )
        ROM_LOAD( "GHL21.BIN",      0x270000, 0x10000, 0x30cecdb6 )


        /* 32 * 32 tiles - left half */
        ROM_LOAD( "GHL09.BIN",      0x280000, 0x10000, 0x3d57242d ) /* Plane x */
        ROM_LOAD( "GHL18.BIN",      0x290000, 0x10000, 0x237eb922 ) /* Plane x */
        ROM_LOAD( "GHL13.BIN",      0x2a0000, 0x10000, 0x18a99b29 ) /* Plane x */
        ROM_LOAD( "GHL22.BIN",      0x2b0000, 0x10000, 0x08a71d8f ) /* Plane x */

        /* 32 * 32 tiles - right half */
        ROM_LOAD( "GHL11.BIN",      0x2c0000, 0x10000, 0x11957991 ) /* Plane x */
        ROM_LOAD( "GHL20.BIN",      0x2d0000, 0x10000, 0xf2757633 ) /* Plane x */
        ROM_LOAD( "GHL15.BIN",      0x2e0000, 0x10000, 0xdfcd0bfb ) /* Plane x */
        ROM_LOAD( "GHL24.BIN",      0x2f0000, 0x10000, 0x3cf07f7a ) /* Plane x */

        ROM_REGION(0x18000) /* 64k for the audio CPU */
        ROM_LOAD( "GHL26.BIN",      0x000000, 0x08000, 0x8df5e803 )
                ROM_CONTINUE(    0x010000, 0x08000 )
ROM_END

struct GameDriver ghouls_driver =
{
        "Ghouls and Ghosts",
        "ghouls",
        CPS1_CREDITS("Paul Leaman"),
        &ghouls_machine_driver,

        ghouls_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_ghouls,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};

/********************************************************************

                      Carrier Air Wing

  Not finished yet.  Graphics not completely decoded. Tile maps
  in the wrong order.

********************************************************************/


INPUT_PORTS_START( input_ports_cawing )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* DSWA */
        PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
        PORT_DIPNAME( 0x40, 0x40, "Force 2 Coins/1 Credit (1 to continue if On)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x07, 0x07, "Difficulty Level 1", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Very Easy" )
        PORT_DIPSETTING(    0x06, "Easy 2" )
        PORT_DIPSETTING(    0x05, "Easy 1" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Difficult 1" )
        PORT_DIPSETTING(    0x02, "Difficult 2" )
        PORT_DIPSETTING(    0x01, "Difficult 3" )
        PORT_DIPSETTING(    0x00, "Very Difficult" )
        PORT_DIPNAME( 0x18, 0x10, "Difficulty Level 2", IP_KEY_NONE )
        PORT_DIPSETTING(    0x10, "Easy")
        PORT_DIPSETTING(    0x18, "Normal")
        PORT_DIPSETTING(    0x08, "Difficult")
        PORT_DIPSETTING(    0x00, "Very Difficult" )
        PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off" )
        PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Lives?", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "1" )
        PORT_DIPSETTING(    0x03, "2" )
        PORT_DIPSETTING(    0x02, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "Off")
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x08, "Off")
        PORT_DIPNAME( 0x10, 0x00, "Flip Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x10, "Off")
        PORT_DIPNAME( 0x20, 0x00, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x40, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

CHAR_LAYOUT2  (charlayout_cawingj,   2048,   0x80000*8);
SPRITE_LAYOUT2(spritelayout_cawingj, 0x2800);
SPRITE_LAYOUT2(tilelayout_cawingj,   0x2800);
TILE32_LAYOUT2(tilelayout32_cawingj, 0x200,  0x40000*8);

static struct GfxDecodeInfo gfxdecodeinfo_cawingj[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x028000, &charlayout_cawingj,    0,                      32 },
        { 1, 0x000000, &spritelayout_cawingj,  32*16,                  32 },
        { 1, 0x030000, &tilelayout_cawingj,    32*16+32*16,            32 },
        { 1, 0x018000, &tilelayout32_cawingj,  32*16+32*16+32*16,      32 },
	{ -1 } /* end of array */
};



MACHINE_DRIVER(
        cawingj_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_cawingj,
        CPS1_DEFAULT_CPU_SPEED);

ROM_START( cawingj_rom )
        ROM_REGION(0x100000)      /* 68000 code */

        ROM_LOAD_EVEN("CAJ36A.BIN", 0x00000, 0x20000, 0x7713f4b3 )
        ROM_LOAD_ODD ("CAJ42A.BIN", 0x00000, 0x20000, 0xa66f132d )
        ROM_LOAD_EVEN("CAJ37A.BIN", 0x40000, 0x20000, 0x00435ecb )
        ROM_LOAD_ODD ("CAJ43A.BIN", 0x40000, 0x20000, 0xb7dcf07a )


        /* what about these 4 ? They could be correct */
        ROM_LOAD_EVEN("CAJ34.BIN",  0x80000, 0x20000, 0xf148094e )
        ROM_LOAD_ODD ("CAJ40.BIN",  0x80000, 0x20000, 0xdd000e7c )
        ROM_LOAD_EVEN("CAJ35.BIN",  0xc0000, 0x20000, 0x6875566d )
        ROM_LOAD_ODD ("CAJ41.BIN",  0xc0000, 0x20000, 0xe7579e57 )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */

        ROM_LOAD( "CAJ01.BIN",   0x000000, 0x20000, 0x16e3d26f )
        ROM_LOAD( "CAJ02.BIN",   0x020000, 0x20000, 0x8b15590b )
        ROM_LOAD( "CAJ17.BIN",   0x040000, 0x20000, 0xa74852ba )
        ROM_LOAD( "CAJ18.BIN",   0x060000, 0x20000, 0x2200b432 )

        ROM_LOAD( "CAJ09.BIN",   0x080000, 0x20000, 0x9352d168 )
        ROM_LOAD( "CAJ10.BIN",   0x0a0000, 0x20000, 0xd3ca8a60 )
        ROM_LOAD( "CAJ24.BIN",   0x0c0000, 0x20000, 0xd88e2df8 )
        ROM_LOAD( "CAJ25.BIN",   0x0e0000, 0x20000, 0x4370b1da )

        ROM_LOAD( "CAJ05.BIN",   0x100000, 0x20000, 0xa08d5d65 )
        ROM_LOAD( "CAJ06.BIN",   0x120000, 0x20000, 0x7e86a5d6 )
        ROM_LOAD( "CAJ32.BIN",   0x140000, 0x20000, 0x1934f4f2 )
        ROM_LOAD( "CAJ33.BIN",   0x160000, 0x20000, 0xdd91dbd9 )

        ROM_LOAD( "CAJ13.BIN",   0x180000, 0x20000, 0x72b8d260 )
        ROM_LOAD( "CAJ14.BIN",   0x1a0000, 0x20000, 0xab063786 )
        ROM_LOAD( "CAJ38.BIN",   0x1c0000, 0x20000, 0xb843f3f9 )
        ROM_LOAD( "CAJ39.BIN",   0x1e0000, 0x20000, 0xf7c731c1 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "CAJ-S.BIN",    0x00000, 0x10000, 0xa7fe4540 )
        ROM_LOAD( "CAJ-S.BIN",    0x08000, 0x10000, 0xa7fe4540 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("CAJ30.BIN",    0x00000, 0x20000, 0x71b13751 )
        ROM_LOAD ("CAJ31.BIN",    0x20000, 0x20000, 0x25b14e0b )
ROM_END

struct GameDriver cawingj_driver =
{
        "Carrier Air Wing (Japanese)",
        "cawingj",
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        &cawingj_machine_driver,

        cawingj_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_cawing,
        NULL, 0, 0,

        ORIENTATION_DEFAULT,
        NULL, NULL
};

