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


#include "cps1.h"       /* External CPS1 defintions */

/* Please include this credit for the CPS1 driver */
#define CPS1_CREDITS(X) X "\n\n"\
                     "Paul Leaman (Capcom System 1 driver)\n"\
                     "Aaron Giles (additional code)\n"\


#define CPS1_DEFAULT_CPU_FAST_SPEED     12000000
#define CPS1_DEFAULT_CPU_SPEED          10000000
#define CPS1_DEFAULT_CPU_SLOW_SPEED      8000000

/********************************************************************
*
*  Sound board
*
*********************************************************************/
static void cps1_irq_handler_mus (void) {  cpu_cause_interrupt (1, 0xff ); }
static struct YM2151interface ym2151_interface =
{
	1,                      /* 1 chip */
	3579580,                /* 3.579580 MHz ? */
	{ 255 },
	{ cps1_irq_handler_mus }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz ??? TODO: find out the real frequency */
	3,              /* memory region 3 */
	{ 255 }
};

void cps1_snd_bankswitch_w(int offset,int data)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


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
        { -1 }  /* end of table */
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
        { -1 }  /* end of table */
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
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress cps1_writemem[] =
{
        { 0x000000, 0x0fffff, MWA_ROM },      /* ROM */
        { 0xff0000, 0xffffff, MWA_BANK2, &cps1_ram, &cps1_ram_size },        /* RAM */
        { 0x900000, 0x92ffff, MWA_BANK5, &cps1_gfxram, &cps1_gfxram_size},
        { 0x800030, 0x800033, MWA_NOP },      /* ??? Unknown ??? */
        { 0x800180, 0x800183, cps1_sound_cmd_w},  /* Sound command */
        { 0x800100, 0x8001ff, MWA_BANK4, &cps1_output, &cps1_output_size },  /* Output ports */
        { -1 }  /* end of table */
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
                        4000000,  /* 4 Mhz ??? TODO: find real FRQ */    \
                        2,      /* memory region #2 */                   \
                        sound_readmem,sound_writemem,0,0,                \
                        ignore_interrupt,0                               \
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
        32*16+32*16+32*16+32*16,   /* lotsa colours */                   \
        32*16+32*16+32*16+32*16,   /* Colour table length */             \
        0,                                                               \
                                                                         \
        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                      \
        0,                                                               \
        cps1_vh_start,                                                   \
        cps1_vh_stop,                                                    \
        cps1_vh_screenrefresh,                                           \
                                                                         \
        /* sound hardware */                                             \
        cps1_sh_init,0,0,0,                                              \
        { { SOUND_YM2151,  &ym2151_interface },                            \
          { SOUND_OKIM6295,  &okim6295_interface }                    \
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

/* {3*PLANE_SEP,2*PLANE_SEP,PLANE_SEP, 0},    */

#define TILE32_LAYOUT2(LAYOUT, TILES, SEP, PLANE_SEP) \
static struct GfxLayout LAYOUT =                                  \
{                                                                 \
        32,32,   /* 32*32 tiles */                                \
        TILES,   /* ????  tiles */                                \
        4,       /* 4 bits per pixel */                           \
        {2*PLANE_SEP,3*PLANE_SEP,0*PLANE_SEP, 1*PLANE_SEP},       \
        {                                                         \
           0,1,2,3,4,5,6,7,\
           SEP+0, SEP+1, SEP+ 2, SEP+ 3, SEP+ 4, SEP+ 5, SEP+ 6, SEP+ 7, \
           8,9,10,11,12,13,14,15,                  \
           SEP+8, SEP+9, SEP+10, SEP+11, SEP+12, SEP+13, SEP+14, SEP+15, \
        },                                                        \
        {                                                         \
           0*16, 1*16,  2*16,    3*16,  4*16,  5*16,  6*16,  7*16,\
           8*16, 9*16,  10*16,  11*16, 12*16, 13*16, 14*16, 15*16,\
           16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,\
           24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16,\
        },                                                        \
        16*32 /* every sprite takes 32*8*4 consecutive bytes */\
};


/********************************************************************

                    Configuration table:

********************************************************************/

static struct CPS1config cps1_config_table[]=
{
  /* DRIVER    START  START  START  START      SPACE  SPACE  SPACE  CPSB CPSB
      NAME     SCRL1   OBJ   SCRL2  SCRL3  A   SCRL1  SCRL2  SCRL3  ADDR VAL */
  {"willow",       0,     0,     0,0x0600, 1, 0x7020,0x0000,0x0a00,0x00,0x0000},
  {"willowj",      0,     0,     0,0x0600, 1, 0x7020,0x0000,0x0a00,0x00,0x0000},
  {"ffight",  0x0400,     0,0x2000,0x0200, 2, 0x4420,0x3000,0x0980,0x60,0x0004},
  {"ffightj", 0x0400,     0,0x2000,0x0200, 2, 0x4420,0x3000,0x0980,0x60,0x0004},
  {"mtwins",       0,     0,0x2000,0x0e00, 3, 0x0020,0x0000,0x0000,0x5e,0x0404},
  {"chikij",       0,     0,0x2000,0x0e00, 3, 0x0020,0x0000,0x0000,0x5e,0x0404},
  {"knights", 0x0800,0x0000,0x4c00,0x1a00, 3,     -1,    -1,    -1,0x00,0x0000},
  {"strider",      0,0x0200,0x1000,     0, 4, 0x0020,0x0020,0x0020,0x00,0x0000},
  {"striderj",     0,0x0200,0x1000,     0, 4, 0x0020,0x0020,0x0020,0x00,0x0000},
  {"ghouls",  0x0000,0x0000,0x2000,0x1000, 5, 0x2420,0x2000,0x1000,0x00,0x0000, 0x0800, 1},
  {"ghoulsj", 0x0000,0x0000,0x2000,0x1000, 5, 0x2420,0x2000,0x1000,0x00,0x0000, 0x0800, 1},
  {"1941",         0,     0,0x2400,0x0400, 6, 0x0020,0x0400,0x2420,0x60,0x0005},
  {"1941j",        0,     0,0x2400,0x0400, 6, 0x0020,0x0400,0x2420,0x60,0x0005},
  {"msword",       0,     0,0x2800,0x0e00, 7,     -1,    -1,    -1,0x00,0x0000},
  {"mswordj",      0,     0,0x2800,0x0e00, 7,     -1,    -1,    -1,0x00,0x0000},
  {"nemo",         0,     0,0x2400,0x0d00, 8, 0x4020,0x2400,0x0d00,0x4e,0x0405},
  {"nemoj",        0,     0,0x2400,0x0d00, 8, 0x4020,0x2400,0x0d00,0x4e,0x0405},
  {"unsquad",      0,     0,0x2000,     0, 0, 0x0020,0x0000,0x0000,0x00,0x0000},
  {"pnickj",       0,0x0800,0x0800,0x0c00, 0,     -1,    -1,    -1,0x00,0x0000},
  {"cawingj",      0,     0,0x2c00,0x0600, 0,     -1,    -1,    -1,0x00,0x0000},
  /* End of table (default values) */
  {0,              0,     0,     0,     0, 0,     -1,    -1,    -1,0x00,0x0000},
};

static int cps1_sh_init(const char *gamename)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        struct CPS1config *pCFG=&cps1_config_table[0];
        while(pCFG->name)
        {
                if (strcmp(pCFG->name, gamename) == 0)
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

        if (strcmp(gamename, "cawingj")==0)
        {
                /* Quick hack (NOP out CPSB test) */
                if (errorlog)
                {
                   fprintf(errorlog, "Patching CPSB test\n");
                }
                WRITE_WORD(&RAM[0x04ca], 0x4e71);
                WRITE_WORD(&RAM[0x04cc], 0x4e71);
        }
        else if (strcmp(gamename, "ghouls")==0)
        {
                /* Patch out self-test... it takes forever */
                WRITE_WORD(&RAM[0x61964+0], 0x4ef9);
                WRITE_WORD(&RAM[0x61964+2], 0x0000);
                WRITE_WORD(&RAM[0x61964+4], 0x0400);
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
        { 1, 0x2f0000, &charlayout_strider,    32*16,                  32 },
        { 1, 0x004000, &spritelayout_strider,  0,                      32 },
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
	ROM_LOAD_EVEN(      "strider.30", 0x00000, 0x20000, 0x9f411127 )
	ROM_LOAD_ODD (      "strider.35", 0x00000, 0x20000, 0xf12ac928 )
	ROM_LOAD_EVEN(      "strider.31", 0x40000, 0x20000, 0x7ec385b9 )
	ROM_LOAD_ODD (      "strider.36", 0x40000, 0x20000, 0xe9a8bf28 )
	ROM_LOAD_WIDE_SWAP( "strider.32", 0x80000, 0x80000, 0x59b99ecf ) /* Tile map */

	ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "strider.02", 0x000000, 0x80000, 0x5f694f7b )
	ROM_LOAD( "strider.06", 0x080000, 0x80000, 0x19fbe6a7 )
	ROM_LOAD( "strider.04", 0x100000, 0x80000, 0x2c9017c2 )
	ROM_LOAD( "strider.08", 0x180000, 0x80000, 0x3b0e4930 )
	ROM_LOAD( "strider.01", 0x200000, 0x80000, 0x0e0c0fb8 )
	ROM_LOAD( "strider.05", 0x280000, 0x80000, 0x542a60c8 )
	ROM_LOAD( "strider.03", 0x300000, 0x80000, 0x6c8b64c9 )
	ROM_LOAD( "strider.07", 0x380000, 0x80000, 0x27932793 )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "strider.09", 0x00000, 0x10000, 0x85adabcd )
	ROM_RELOAD(             0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "strider.18", 0x00000, 0x20000, 0x6e80d9f2 )
	ROM_LOAD( "strider.19", 0x20000, 0x20000, 0xc906d49a )
ROM_END

ROM_START( striderj_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_WIDE_SWAP( "sthj23.bin", 0x00000, 0x80000, 0x651ad9fa )
	ROM_LOAD_WIDE_SWAP( "strider.32", 0x80000, 0x80000, 0x59b99ecf ) /* Tile map */

	ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "strider.02", 0x000000, 0x80000, 0x5f694f7b )
	ROM_LOAD( "strider.06", 0x080000, 0x80000, 0x19fbe6a7 )
	ROM_LOAD( "strider.04", 0x100000, 0x80000, 0x2c9017c2 )
	ROM_LOAD( "strider.08", 0x180000, 0x80000, 0x3b0e4930 )
	ROM_LOAD( "strider.01", 0x200000, 0x80000, 0x0e0c0fb8 )
	ROM_LOAD( "strider.05", 0x280000, 0x80000, 0x542a60c8 )
	ROM_LOAD( "strider.03", 0x300000, 0x80000, 0x6c8b64c9 )
	ROM_LOAD( "strider.07", 0x380000, 0x80000, 0x27932793 )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "strider.09", 0x00000, 0x10000, 0x85adabcd )
	ROM_RELOAD(             0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "strider.18", 0x00000, 0x20000, 0x6e80d9f2 )
	ROM_LOAD( "strider.19", 0x20000, 0x20000, 0xc906d49a )
ROM_END


struct GameDriver strider_driver =
{
	__FILE__,
	0,
	"strider",
	"Strider (US)",
	"1989",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
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

struct GameDriver striderj_driver =
{
	__FILE__,
	&strider_driver,
	"striderj",
	"Strider (Japan)",
	"1989",
	"Capcom",
	CPS1_CREDITS ("Marco Cassili (Game Driver)"),
	0,
	&strider_machine_driver,

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
        { 1, 0x0f0000, &charlayout_willow,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_strider, 0,                      32 },
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
	ROM_LOAD_EVEN(      "WLU_30.ROM", 0x00000, 0x20000, 0x06b81390 )
	ROM_LOAD_ODD (      "WLU_35.ROM", 0x00000, 0x20000, 0x6c67bfcf )
	ROM_LOAD_EVEN(      "WLU_31.ROM", 0x40000, 0x20000, 0xac363d78 )
	ROM_LOAD_ODD (      "WLU_36.ROM", 0x40000, 0x20000, 0xf7fc564e )
	ROM_LOAD_WIDE_SWAP( "WL_32.ROM",  0x80000, 0x80000, 0xe75769a9 ) /* Tile map */

	ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "WL_GFX1.ROM", 0x000000, 0x80000, 0x352afe30 )
	ROM_LOAD( "WL_GFX5.ROM", 0x080000, 0x80000, 0x768c375a )
	ROM_LOAD( "WL_GFX3.ROM", 0x100000, 0x80000, 0x3c9240f4 )
	ROM_LOAD( "WL_GFX7.ROM", 0x180000, 0x80000, 0x95e77d43 )
	ROM_LOAD( "WL_20.ROM",   0x200000, 0x20000, 0x67c34af5 )
	ROM_LOAD( "WL_10.ROM",   0x220000, 0x20000, 0xc8c6fe3c )
	ROM_LOAD( "WL_22.ROM",   0x240000, 0x20000, 0x6a95084b )
	ROM_LOAD( "WL_12.ROM",   0x260000, 0x20000, 0x66735153 )
	ROM_LOAD( "WL_24.ROM",   0x280000, 0x20000, 0x839200be )
	ROM_LOAD( "WL_14.ROM",   0x2a0000, 0x20000, 0x6cf8259c )
	ROM_LOAD( "WL_26.ROM",   0x2c0000, 0x20000, 0xeef53eb5 )
	ROM_LOAD( "WL_16.ROM",   0x2e0000, 0x20000, 0x6a02dfde )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "WL_09.ROM", 0x00000, 0x08000, 0x56014501 )
	ROM_CONTINUE(          0x10000, 0x08000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD ("WL_18.ROM", 0x00000, 0x20000, 0x58bb142b )
	ROM_LOAD ("WL_19.ROM", 0x20000, 0x20000, 0xc575f23d )
ROM_END

ROM_START( willowj_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN(      "WL36.bin", 0x00000, 0x20000, 0x739e4660 )
	ROM_LOAD_ODD (      "WL42.bin", 0x00000, 0x20000, 0x2eb180d7 )
	ROM_LOAD_EVEN(      "WL37.bin", 0x40000, 0x20000, 0x53d167b3 )
	ROM_LOAD_ODD (      "WL43.bin", 0x40000, 0x20000, 0x84a91c11 )
	ROM_LOAD_WIDE_SWAP( "WL_32.ROM",  0x80000, 0x80000, 0xe75769a9 ) /* Tile map */

	ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "WL_GFX1.ROM", 0x000000, 0x80000, 0x352afe30 )
	ROM_LOAD( "WL_GFX5.ROM", 0x080000, 0x80000, 0x768c375a )
	ROM_LOAD( "WL_GFX3.ROM", 0x100000, 0x80000, 0x3c9240f4 )
	ROM_LOAD( "WL_GFX7.ROM", 0x180000, 0x80000, 0x95e77d43 )
	ROM_LOAD( "WL_20.ROM",   0x200000, 0x20000, 0x67c34af5 )
	ROM_LOAD( "WL_10.ROM",   0x220000, 0x20000, 0xc8c6fe3c )
	ROM_LOAD( "WL_22.ROM",   0x240000, 0x20000, 0x6a95084b )
	ROM_LOAD( "WL_12.ROM",   0x260000, 0x20000, 0x66735153 )
	ROM_LOAD( "WL_24.ROM",   0x280000, 0x20000, 0x839200be )
	ROM_LOAD( "WL_14.ROM",   0x2a0000, 0x20000, 0x6cf8259c )
	ROM_LOAD( "WL_26.ROM",   0x2c0000, 0x20000, 0xeef53eb5 )
	ROM_LOAD( "WL_16.ROM",   0x2e0000, 0x20000, 0x6a02dfde )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "WL_09.ROM", 0x00000, 0x08000, 0x56014501 )
	ROM_CONTINUE(          0x10000, 0x08000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD ("WL_18.ROM", 0x00000, 0x20000, 0x58bb142b )
	ROM_LOAD ("WL_19.ROM", 0x20000, 0x20000, 0xc575f23d )
ROM_END


struct GameDriver willow_driver =
{
	__FILE__,
	0,
	"willow",
	"Willow (Japan, English)",
	"1989",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
	&willow_machine_driver,

	willow_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_willow,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	NULL, NULL
};

struct GameDriver willowj_driver =
{
	__FILE__,
	&willow_driver,
	"willowj",
	"Willow (Japan, Japanese)",
	"1989",
	"Capcom",
	CPS1_CREDITS ("Marco Cassili (Game Driver)"),
	0,
	&willow_machine_driver,

	willowj_rom,
	0,
	0,0,
	0,      /* sound_prom */

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
        { 1, 0x044000, &charlayout_ffight,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_ffight,  0,                      32 },
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
	ROM_LOAD( "FF09-09.BIN", 0x00000, 0x10000, 0x06b2f7f8 )
	ROM_RELOAD(              0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD ("FF18-18.BIN", 0x00000, 0x20000, 0x42467b56 )
	ROM_LOAD ("FF19-19.BIN", 0x20000, 0x20000, 0xabd11a71 )
ROM_END

ROM_START( ffightj_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN("FF30-36.BIN", 0x00000, 0x20000, 0x3ad2c910 )
	ROM_LOAD_ODD ("FF35-42.BIN", 0x00000, 0x20000, 0xe2c402c0 )
	ROM_LOAD_EVEN("FF31-37.BIN", 0x40000, 0x20000, 0x2bfa6a30 )
	ROM_LOAD_ODD ("ff43.bin", 0x40000, 0x20000, 0x7bdad98c )
	ROM_LOAD_WIDE_SWAP("FF32-32M.BIN",  0x80000, 0x80000, 0x5247370d ) /* Tile map */

	ROM_REGION(0x500000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD_EVEN( "ff17.bin", 0x000000, 0x20000, 0x13b69444 )
	ROM_LOAD_ODD ( "ff24.bin", 0x000000, 0x20000, 0x1726d636 )
	ROM_LOAD_EVEN( "ff18.bin", 0x040000, 0x20000, 0x7830bc52 )
	ROM_LOAD_ODD ( "ff25.bin", 0x040000, 0x20000, 0x9511fc3d )
	ROM_LOAD_EVEN( "ff01.bin", 0x080000, 0x20000, 0x0984eeda )
	ROM_LOAD_ODD ( "ff09.bin", 0x080000, 0x20000, 0xc1e8dc4a )
	ROM_LOAD_EVEN( "ff02.bin", 0x0c0000, 0x20000, 0x43d4f946 )
	ROM_LOAD_ODD ( "ff10.bin", 0x0c0000, 0x20000, 0xec62a61c )
	ROM_LOAD_EVEN( "ff32.bin", 0x100000, 0x20000, 0x83f02f16 )
	ROM_LOAD_ODD ( "ff38.bin", 0x100000, 0x20000, 0x70a1a353 )
	ROM_LOAD_EVEN( "ff33.bin", 0x140000, 0x20000, 0x20d1ff3d )
	ROM_LOAD_ODD ( "ff39.bin", 0x140000, 0x20000, 0x284c670a )
	ROM_LOAD_EVEN( "ff05.bin", 0x180000, 0x20000, 0xa7ba3032 )
	ROM_LOAD_ODD ( "ff13.bin", 0x180000, 0x20000, 0x79c64ef2 )
	ROM_LOAD_EVEN( "ff06.bin", 0x1c0000, 0x20000, 0x03674781 )
	ROM_LOAD_ODD ( "ff14.bin", 0x1c0000, 0x20000, 0xe6caa850 )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "FF09-09.BIN", 0x00000, 0x10000, 0x06b2f7f8 )
	ROM_RELOAD(              0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD ("FF18-18.BIN", 0x00000, 0x20000, 0x42467b56 )
	ROM_LOAD ("FF19-19.BIN", 0x20000, 0x20000, 0xabd11a71 )
ROM_END


struct GameDriver ffight_driver =
{
	__FILE__,
	0,
	"ffight",
	"Final Fight (World)",
	"1989",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
	&ffight_machine_driver,

	ffight_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_ffight,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	NULL, NULL
};

struct GameDriver ffightj_driver =
{
	__FILE__,
	&ffight_driver,
	"ffightj",
	"Final Fight (Japan)",
	"1989",
	"Capcom",
	CPS1_CREDITS("Marco Cassili (Game Driver)"),
	0,
	&ffight_machine_driver,

	ffightj_rom,
	0,
	0,0,
	0,      /* sound_prom */

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
        { 1, 0x030000, &charlayout_unsquad,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_unsquad,  0,                      32 },
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
        ROM_LOAD_EVEN("UNSQUAD.30",       0x00000, 0x20000, 0x474a8f2c )
        ROM_LOAD_ODD ("UNSQUAD.35",       0x00000, 0x20000, 0x46432039 )
        ROM_LOAD_EVEN("UNSQUAD.31",       0x40000, 0x20000, 0x4f1952f1 )
        ROM_LOAD_ODD ("UNSQUAD.36",       0x40000, 0x20000, 0xf69148b7 )
        ROM_LOAD_WIDE_SWAP( "UNSQUAD.32", 0x80000, 0x80000, 0x45a55eb7 ) /* tiles + chars */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "UNSQUAD.01", 0x000000, 0x80000, 0x87c8d9a8 )
        ROM_LOAD( "UNSQUAD.05", 0x080000, 0x80000, 0xea7f4a55 )
        ROM_LOAD( "UNSQUAD.03", 0x100000, 0x80000, 0x0ce0ac76 )
        ROM_LOAD( "UNSQUAD.07", 0x180000, 0x80000, 0x837e8800 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "UNSQUAD.09", 0x00000, 0x10000, 0xc55f46db )
        ROM_RELOAD(             0x08000, 0x10000 )

        ROM_REGION(0x20000) /* Samples */
        ROM_LOAD ("UNSQUAD.18", 0x00000, 0x20000, 0x6cc60418 )
ROM_END

struct GameDriver unsquad_driver =
{
	__FILE__,
	0,
	"unsquad",
	"UN Squadron (US)",
	"1989",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
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
        { 1, 0x030000, &charlayout_mtwins,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_mtwins,  0,                      32 },
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
	ROM_LOAD_EVEN(      "CHE_30.ROM", 0x00000, 0x20000, 0x32b80b2e )
	ROM_LOAD_ODD (      "CHE_35.ROM", 0x00000, 0x20000, 0xdb8b9017 )
	ROM_LOAD_EVEN(      "CHE_31.ROM", 0x40000, 0x20000, 0x3fa4bac6 )
	ROM_LOAD_ODD (      "CHE_36.ROM", 0x40000, 0x20000, 0x70d9f263 )
	ROM_LOAD_WIDE_SWAP( "CH_32.ROM",  0x80000, 0x80000, 0xbc81ffbb ) /* tiles + chars */

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "CH_GFX1.ROM", 0x000000, 0x80000, 0x8394505c )
	ROM_LOAD( "CH_GFX5.ROM", 0x080000, 0x80000, 0x8ce0dcfe )
	ROM_LOAD( "CH_GFX3.ROM", 0x100000, 0x80000, 0x12e50cdf )
	ROM_LOAD( "CH_GFX7.ROM", 0x180000, 0x80000, 0xb11a09e0 )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "CH_09.ROM", 0x00000, 0x10000, 0x9dfa57d4 )
	ROM_RELOAD(            0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "CH_18.ROM", 0x00000, 0x20000, 0x5c96f786 )
	ROM_LOAD( "CH_19.ROM", 0x20000, 0x20000, 0xb12d19a5 )
ROM_END

ROM_START( chikij_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN(      "chj36a.bin", 0x00000, 0x20000, 0xda26ace2 )
	ROM_LOAD_ODD (      "chj42a.bin", 0x00000, 0x20000, 0xdd5b399f )
	ROM_LOAD_EVEN(      "chj37a.bin", 0x40000, 0x20000, 0xd7301e44 )
	ROM_LOAD_ODD (      "chj43a.bin", 0x40000, 0x20000, 0xbd73ffed )
	ROM_LOAD_WIDE_SWAP( "CH_32.ROM",  0x80000, 0x80000, 0xbc81ffbb ) /* tiles + chars */

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "CH_GFX1.ROM", 0x000000, 0x80000, 0x8394505c )
	ROM_LOAD( "CH_GFX5.ROM", 0x080000, 0x80000, 0x8ce0dcfe )
	ROM_LOAD( "CH_GFX3.ROM", 0x100000, 0x80000, 0x12e50cdf )
	ROM_LOAD( "CH_GFX7.ROM", 0x180000, 0x80000, 0xb11a09e0 )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "CH_09.ROM", 0x00000, 0x10000, 0x9dfa57d4 )
	ROM_RELOAD(            0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "CH_18.ROM", 0x00000, 0x20000, 0x5c96f786 )
	ROM_LOAD( "CH_19.ROM", 0x20000, 0x20000, 0xb12d19a5 )
ROM_END


struct GameDriver mtwins_driver =
{
	__FILE__,
	0,
	"mtwins",
	"Mega Twins (World)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
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

struct GameDriver chikij_driver =
{
	__FILE__,
	&mtwins_driver,
	"chikij",
	"Chiki Chiki Boys (Japan)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Marco Cassili (Game Driver)"),
	0,
	&mtwins_machine_driver,

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

                          Nemo

********************************************************************/

INPUT_PORTS_START( input_ports_nemo )
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
        PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Very Easy" )
        PORT_DIPSETTING(    0x06, "Easy 1" )
        PORT_DIPSETTING(    0x05, "Easy 2" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Difficult 1" )
        PORT_DIPSETTING(    0x02, "Difficult 2" )
        PORT_DIPSETTING(    0x01, "Difficult 3" )
        PORT_DIPSETTING(    0x00, "Very Difficult" )
        PORT_DIPNAME( 0x18, 0x18, "Life Bar", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Minimun" )
        PORT_DIPSETTING(    0x18, "Medium" )
        PORT_DIPSETTING(    0x08, "Maximum" )
        /* 0x10 gives Medium */
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
        PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x02, "1" )
        PORT_DIPSETTING(    0x03, "2" )
        PORT_DIPSETTING(    0x01, "3" )
        PORT_DIPSETTING(    0x00, "4" )
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


CHAR_LAYOUT(charlayout_nemo,  2048, 0x100000*8)
SPRITE_LAYOUT(spritelayout_nemo, 4096*2, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_nemo, 8192-2048, 0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_nemo, 1024-256,  0x080000*8, 0x100000*8)

static struct GfxDecodeInfo gfxdecodeinfo_nemo[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x040000, &charlayout_nemo,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_nemo,  0,                      32 },
        { 1, 0x048000, &tilelayout_nemo,    32*16+32*16,            32 },
        { 1, 0x068000, &tilelayout32_nemo,  32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        nemo_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_nemo,
        CPS1_DEFAULT_CPU_SPEED)


ROM_START( nemo_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN(      "nme_30a.ROM", 0x00000, 0x20000, 0x0ea09c8e )
	ROM_LOAD_ODD (      "nme_35a.ROM", 0x00000, 0x20000, 0xa06e170e )
	ROM_LOAD_EVEN(      "nme_31a.ROM", 0x40000, 0x20000, 0xd5358aab )
	ROM_LOAD_ODD (      "nme_36a.ROM", 0x40000, 0x20000, 0xbda37457 )
	ROM_LOAD_WIDE_SWAP( "nm_32.ROM",   0x80000, 0x80000, 0xdf80bd98 ) /* Tile map */

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nm_gfx1.ROM", 0x000000, 0x80000, 0x72382998 )
	ROM_LOAD( "nm_gfx5.ROM", 0x080000, 0x80000, 0xeccabcae )
	ROM_LOAD( "nm_gfx3.ROM", 0x100000, 0x80000, 0xb87c0e86 )
	ROM_LOAD( "nm_gfx7.ROM", 0x180000, 0x80000, 0xa5fce4fc )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "nm_09.ROM", 0x000000, 0x08000, 0xe572c960 )
	ROM_CONTINUE(          0x010000, 0x08000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "nm_18.ROM", 0x00000, 0x20000, 0x2094dafa )
	ROM_LOAD( "nm_19.ROM", 0x20000, 0x20000, 0x551d35d9 )
ROM_END

ROM_START( nemoj_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN(      "nm36.bin",  0x00000, 0x20000, 0x208a14b4 )
	ROM_LOAD_ODD (      "nm42.bin",  0x00000, 0x20000, 0x9a39b6ef )
	ROM_LOAD_EVEN(      "nm37.bin",  0x40000, 0x20000, 0x2acf9fbb )
	ROM_LOAD_ODD (      "nm43.bin",  0x40000, 0x20000, 0xffd987f5 )
	ROM_LOAD_WIDE_SWAP( "nm_32.ROM", 0x80000, 0x80000, 0xdf80bd98 ) /* Tile map */

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nm_gfx1.ROM", 0x000000, 0x80000, 0x72382998 )
	ROM_LOAD( "nm_gfx5.ROM", 0x080000, 0x80000, 0xeccabcae )
	ROM_LOAD( "nm_gfx3.ROM", 0x100000, 0x80000, 0xb87c0e86 )
	ROM_LOAD( "nm_gfx7.ROM", 0x180000, 0x80000, 0xa5fce4fc )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "nm_09.ROM", 0x000000, 0x08000, 0xe572c960 )
	ROM_CONTINUE(          0x010000, 0x08000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "nm_18.ROM", 0x00000, 0x20000, 0x2094dafa )
	ROM_LOAD( "nm_19.ROM", 0x20000, 0x20000, 0x551d35d9 )
ROM_END


struct GameDriver nemo_driver =
{
	__FILE__,
	0,
	"nemo",
	"Nemo (World)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Darren Olafson (Game Driver)\nMarco Cassili (Dip Switches)\nPaul Leaman"),
	0,
	&nemo_machine_driver,

	nemo_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_nemo,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	NULL, NULL
};

struct GameDriver nemoj_driver =
{
	__FILE__,
	&nemo_driver,
	"nemoj",
	"Nemo (Japan)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Sawat Pontree (Game Driver)\nMarco Cassili (dip switches)\nPaul Leman"),
	0,
	&nemo_machine_driver,

	nemoj_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_nemo,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	NULL, NULL
};


/********************************************************************

                          1941

********************************************************************/

INPUT_PORTS_START( input_ports_1941 )
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
        PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "0 (Easier)" )
        PORT_DIPSETTING(    0x06, "1" )
        PORT_DIPSETTING(    0x05, "2" )
        PORT_DIPSETTING(    0x04, "3" )
        PORT_DIPSETTING(    0x03, "4" )
        PORT_DIPSETTING(    0x02, "5" )
        PORT_DIPSETTING(    0x01, "6" )
        PORT_DIPSETTING(    0x00, "7 (Harder)" )
        PORT_DIPNAME( 0x18, 0x18, "Life Bar", IP_KEY_NONE )
        PORT_DIPSETTING(    0x18, "More Slowly" )
        PORT_DIPSETTING(    0x10, "Slowly" )
        PORT_DIPSETTING(    0x08, "Quickly" )
        PORT_DIPSETTING(    0x00, "More Quickly" )
        PORT_DIPNAME( 0x60, 0x60, "Bullet's Speed", IP_KEY_NONE )
        PORT_DIPSETTING(    0x60, "Very Slow" )
        PORT_DIPSETTING(    0x40, "Slow" )
        PORT_DIPSETTING(    0x20, "Fast" )
        PORT_DIPSETTING(    0x00, "Very Fast" )
        PORT_DIPNAME( 0x80, 0x80, "Initial Vitality", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "3 Bars" )
        PORT_DIPSETTING(    0x00, "4 Bars" )
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


CHAR_LAYOUT(charlayout_1941,  2048, 0x100000*8)
SPRITE_LAYOUT(spritelayout_1941, 4096*2-1024, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_1941, 8192-1024,   0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_1941, 1024,  0x080000*8, 0x100000*8)

static struct GfxDecodeInfo gfxdecodeinfo_1941[] =
{
        /*   start    pointer               colour start   number of colours */
        { 1, 0x040000, &charlayout_1941,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_1941,  0    ,                  32 },
        { 1, 0x048000, &tilelayout_1941,    32*16+32*16,            32 },
        { 1, 0x020000, &tilelayout32_1941,  32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        c1941_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_1941,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( c1941_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN(      "41e_30.ROM", 0x00000, 0x20000, 0x6e4db1c3 )
	ROM_LOAD_ODD (      "41e_35.ROM", 0x00000, 0x20000, 0xc8938a93 )
	ROM_LOAD_EVEN(      "41e_31.ROM", 0x40000, 0x20000, 0x06ed4375 )
	ROM_LOAD_ODD (      "41e_36.ROM", 0x40000, 0x20000, 0x2b7b9581 )
	ROM_LOAD_WIDE_SWAP( "41_32.ROM",  0x80000, 0x80000, 0x719d7e13 )

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "41_GFX1.ROM", 0x000000, 0x80000, 0x5ef283ec )
	ROM_LOAD( "41_GFX5.ROM", 0x080000, 0x80000, 0xcded8f91 )
	ROM_LOAD( "41_GFX3.ROM", 0x100000, 0x80000, 0xd76ef474 )
	ROM_LOAD( "41_GFX7.ROM", 0x180000, 0x80000, 0xa163ecdd )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "41_09.ROM", 0x000000, 0x08000, 0xd1a4b9de )
	ROM_CONTINUE(          0x010000, 0x08000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "41_18.ROM", 0x00000, 0x20000, 0x9b0dee37 )
	ROM_LOAD( "41_19.ROM", 0x20000, 0x20000, 0x3644013c )
ROM_END

ROM_START( c1941j_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN( "4136.bin",  0x00000, 0x20000, 0x52fe93ac )
	ROM_LOAD_ODD ( "4142.bin",  0x00000, 0x20000, 0x1f6f0d31 )
	ROM_LOAD_EVEN( "4137.bin",  0x40000, 0x20000, 0xc633c493 )
	ROM_LOAD_ODD ( "4143.bin",  0x40000, 0x20000, 0x4d620534 )
	ROM_LOAD_WIDE_SWAP( "41_32.ROM",  0x80000, 0x80000, 0x719d7e13 )

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "41_GFX1.ROM", 0x000000, 0x80000, 0x5ef283ec )
	ROM_LOAD( "41_GFX5.ROM", 0x080000, 0x80000, 0xcded8f91 )
	ROM_LOAD( "41_GFX3.ROM", 0x100000, 0x80000, 0xd76ef474 )
	ROM_LOAD( "41_GFX7.ROM", 0x180000, 0x80000, 0xa163ecdd )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "41_09.ROM", 0x000000, 0x08000, 0xd1a4b9de )
	ROM_CONTINUE(          0x010000, 0x08000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "41_18.ROM", 0x00000, 0x20000, 0x9b0dee37 )
	ROM_LOAD( "41_19.ROM", 0x20000, 0x20000, 0x3644013c )
ROM_END


struct GameDriver c1941_driver =
{
	__FILE__,
	0,
	"1941",
	"1941 (World)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Darren Olafson (Game Driver)\nMarco Cassili (Dip Switches)\nPaul Leaman"),
	0,
	&c1941_machine_driver,

	c1941_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_1941,
	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	NULL, NULL
};

struct GameDriver c1941j_driver =
{
	__FILE__,
	&c1941_driver,
	"1941j",
	"1941 (Japan)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Marco Cassili (Game Driver)\nPaul Leaman"),
	0,
	&c1941_machine_driver,

	c1941j_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_1941,
	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
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

CHAR_LAYOUT(charlayout_msword,  4096, 0x100000*8)
SPRITE_LAYOUT(spritelayout_msword, 8192, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_msword, 4096, 0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_msword, 512,  0x080000*8, 0x100000*8)

static struct GfxDecodeInfo gfxdecodeinfo_msword[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x040000, &charlayout_msword,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_msword,  0,                      32 },
        { 1, 0x050000, &tilelayout_msword,    32*16+32*16,            32 },
        { 1, 0x070000, &tilelayout32_msword,  32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        msword_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_msword,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( msword_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN(      "MSE_30.ROM", 0x00000, 0x20000, 0xa99a131a )
	ROM_LOAD_ODD (      "MSE_35.ROM", 0x00000, 0x20000, 0x452c3188 )
	ROM_LOAD_EVEN(      "MSE_31.ROM", 0x40000, 0x20000, 0x1948cd8a )
	ROM_LOAD_ODD (      "MSE_36.ROM", 0x40000, 0x20000, 0x19a7b0c1 )
	ROM_LOAD_WIDE_SWAP( "MS_32.ROM",  0x80000, 0x80000, 0x78415913 )

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "MS_GFX1.ROM", 0x000000, 0x80000, 0x0ac52871 )
	ROM_LOAD( "MS_GFX5.ROM", 0x080000, 0x80000, 0xd9d46ec2 )
	ROM_LOAD( "MS_GFX3.ROM", 0x100000, 0x80000, 0x957cc634 )
	ROM_LOAD( "MS_GFX7.ROM", 0x180000, 0x80000, 0xb546d1d6 )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "MS_9.ROM", 0x00000, 0x10000, 0x16de56f0 )
	ROM_RELOAD(           0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "MS_18.ROM", 0x00000, 0x20000, 0x422df0cd )
	ROM_LOAD( "MS_19.ROM", 0x20000, 0x20000, 0xf249d475 )
ROM_END

ROM_START( mswordj_rom )
	ROM_REGION(0x100000)      /* 68000 code */
	ROM_LOAD_EVEN(      "MSJ_30.ROM", 0x00000, 0x20000, 0xd7cc6b4e )
	ROM_LOAD_ODD (      "MSJ_35.ROM", 0x00000, 0x20000, 0x0cc87b72 )
	ROM_LOAD_EVEN(      "MSJ_31.ROM", 0x40000, 0x20000, 0xd664430e )
	ROM_LOAD_ODD (      "MSJ_36.ROM", 0x40000, 0x20000, 0x098c72fc )
	ROM_LOAD_WIDE_SWAP( "MS_32.ROM",  0x80000, 0x80000, 0x78415913 )

	ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "MS_GFX1.ROM", 0x000000, 0x80000, 0x0ac52871 )
	ROM_LOAD( "MS_GFX5.ROM", 0x080000, 0x80000, 0xd9d46ec2 )
	ROM_LOAD( "MS_GFX3.ROM", 0x100000, 0x80000, 0x957cc634 )
	ROM_LOAD( "MS_GFX7.ROM", 0x180000, 0x80000, 0xb546d1d6 )

	ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "MS_9.ROM", 0x00000, 0x10000, 0x16de56f0 )
	ROM_RELOAD(           0x08000, 0x10000 )

	ROM_REGION(0x40000) /* Samples */
	ROM_LOAD( "MS_18.ROM", 0x00000, 0x20000, 0x422df0cd )
	ROM_LOAD( "MS_19.ROM", 0x20000, 0x20000, 0xf249d475 )
ROM_END


struct GameDriver msword_driver =
{
	__FILE__,
	0,
	"msword",
	"Magic Sword (World)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
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

struct GameDriver mswordj_driver =
{
	__FILE__,
	&msword_driver,
	"mswordj",
	"Magic Sword (Japan)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
	&msword_machine_driver,

	mswordj_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_msword,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	NULL, NULL
};


/********************************************************************

                          Pnickies (Japan)

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

CHAR_LAYOUT2(charlayout_pnickj, 4096, 0x80000*8)
SPRITE_LAYOUT2(spritelayout_pnickj, 0x2800)
SPRITE_LAYOUT2(tilelayout_pnickj,   0x2800)
TILE32_LAYOUT2(tilelayout32_pnickj, 512,  0x40000*8, 0x80000*8 )

static struct GfxDecodeInfo gfxdecodeinfo_pnickj[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x000000, &charlayout_pnickj,    32*16,                  32 },
        { 1, 0x008000, &spritelayout_pnickj,  0,                      32 },
        { 1, 0x008000, &tilelayout_pnickj,    32*16+32*16,            32 },
        { 1, 0x030000, &tilelayout32_pnickj,  32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};


MACHINE_DRIVER(
        pnickj_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_pnickj,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( pnickj_rom )
        ROM_REGION(0x040000)      /* 68000 code */
        ROM_LOAD_EVEN("PNIJ36.BIN", 0x00000, 0x20000, 0xdd4c06f8 )
        ROM_LOAD_ODD("PNIJ42.BIN",  0x00000, 0x20000, 0x31a6c64a )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "PNIJ01.BIN", 0x000000, 0x20000, 0x7d70f1ae )
        ROM_LOAD( "PNIJ02.BIN", 0x020000, 0x20000, 0x0920aab2 )
        ROM_LOAD( "PNIJ18.BIN", 0x040000, 0x20000, 0x69c6ec06 )
        ROM_LOAD( "PNIJ19.BIN", 0x060000, 0x20000, 0x699426fe )

        ROM_LOAD( "PNIJ09.BIN", 0x080000, 0x20000, 0x084f08d5 )
        ROM_LOAD( "PNIJ10.BIN", 0x0a0000, 0x20000, 0x82453ef5 )
        ROM_LOAD( "PNIJ26.BIN", 0x0c0000, 0x20000, 0x1222eeb4 )
        ROM_LOAD( "PNIJ27.BIN", 0x0e0000, 0x20000, 0x58a385ed )

        ROM_LOAD( "PNIJ05.BIN", 0x100000, 0x20000, 0xa524baa6 )
        ROM_LOAD( "PNIJ06.BIN", 0x120000, 0x20000, 0xecf87970 )
        ROM_LOAD( "PNIJ32.BIN", 0x140000, 0x20000, 0xca010ba7 )
        ROM_LOAD( "PNIJ33.BIN", 0x160000, 0x20000, 0x6743e579 )

        ROM_LOAD( "PNIJ13.BIN", 0x180000, 0x20000, 0x9914a0fe )
        ROM_LOAD( "PNIJ14.BIN", 0x1a0000, 0x20000, 0x03c84eda )
        ROM_LOAD( "PNIJ38.BIN", 0x1c0000, 0x20000, 0x5cb05f98 )
        ROM_LOAD( "PNIJ39.BIN", 0x1e0000, 0x20000, 0xe561e0a9 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "PNIJ17.BIN", 0x00000, 0x10000, 0x06a99847 )
        ROM_LOAD( "PNIJ17.BIN", 0x08000, 0x10000, 0x06a99847 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("PNIJ24.BIN", 0x00000, 0x20000, 0x5aaf13c5 )
        ROM_LOAD ("PNIJ25.BIN", 0x20000, 0x20000, 0x86d371df )
ROM_END

struct GameDriver pnickj_driver =
{
	__FILE__,
	0,
	"pnickj",
	"Pnickies (Japan)",
	"1994",
	"Capcom (Compile license)",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	0,
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
        { 1, 0x088000, &charlayout_knights,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_knights,  0,                      32 },
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
        ROM_LOAD_WIDE_SWAP( "KR_23E.ROM", 0x00000, 0x80000, 0x79b18275 )
        ROM_LOAD_WIDE_SWAP( "KR_22.ROM",  0x80000, 0x80000, 0x006ee1da )

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "KR_GFX2.ROM", 0x000000, 0x80000, 0xb4dd6223 )
        ROM_LOAD( "KR_GFX6.ROM", 0x080000, 0x80000, 0x6c092895 )
        ROM_LOAD( "KR_GFX1.ROM", 0x100000, 0x80000, 0xbc0f588f )
        ROM_LOAD( "KR_GFX5.ROM", 0x180000, 0x80000, 0x5917bf8f )
        ROM_LOAD( "KR_GFX4.ROM", 0x200000, 0x80000, 0xf30df6ad )
        ROM_LOAD( "KR_GFX8.ROM", 0x280000, 0x80000, 0x21a5b3b5 )
        ROM_LOAD( "KR_GFX3.ROM", 0x300000, 0x80000, 0x07a98709 )
        ROM_LOAD( "KR_GFX7.ROM", 0x380000, 0x80000, 0x5b53bdb9 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "KR_09.ROM", 0x00000, 0x10000, 0x795a9f98 )
        ROM_RELOAD(            0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("KR_18.ROM", 0x00000, 0x20000, 0x06633ed3 )
        ROM_LOAD ("KR_19.ROM", 0x20000, 0x20000, 0x750a1ca4 )
ROM_END

struct GameDriver knights_driver =
{
	__FILE__,
	0,
	"knights",
	"Knights of the Round (World)",
	"1991",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	GAME_NOT_WORKING,
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
  32x32 tiles appear to be doubled.

********************************************************************/

INPUT_PORTS_START( input_ports_ghouls )
PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )  /* Service, but it doesn't give any credit */
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
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x02, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x30, 0x30, "Bonus", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "10K, 30K and every 30K" )
        PORT_DIPSETTING(    0x10, "20K, 50K and every 70K" )
        PORT_DIPSETTING(    0x30, "30K, 60K and every 70K")
        PORT_DIPSETTING(    0x00, "40K, 70K and every 80K" )
        PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "On" )
        PORT_DIPSETTING(    0x00, "Off")

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "3" )
        PORT_DIPSETTING(    0x02, "4" )
        PORT_DIPSETTING(    0x01, "5" )
        PORT_DIPSETTING(    0x00, "6" )
        PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x10, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x20, 0x20, "Demo Sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "On")
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x40, 0x40, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On")
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x80, "Off")

        PORT_START      /* Player 1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_START      /* Player 2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
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
SPRITE_LAYOUT(tilelayout_ghouls,   4096*2,   0x080000*8, 0x100000*8 )
TILE32_LAYOUT2(tilelayout32_ghouls, 1024,    0x040000*8, 0x10000*8 )

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x030000, &charlayout_ghouls,      32*16,                   32 },
        { 1, 0x000000, &spritelayout_ghouls,    0,                       32 },
        { 1, 0x040000, &tilelayout_ghouls,      32*16+32*16,             32 },
        { 1, 0x280000, &tilelayout32_ghouls,    32*16+32*16+32*16,       32 },
        { 1, 0x200000, &spritelayout2_ghouls,   0,                       32 },
        { -1 } /* end of array */
};

MACHINE_DRV(
        ghouls_machine_driver,
        cps1_interrupt, 1,
        gfxdecodeinfo,
        CPS1_DEFAULT_CPU_SLOW_SPEED)

ROM_START( ghouls_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_EVEN("GHL29.BIN", 0x00000, 0x20000, 0x8821a2b3 ) /* 68000 code */
	ROM_LOAD_ODD ("GHL30.BIN", 0x00000, 0x20000, 0xaff1cd13 ) /* 68000 code */
	ROM_LOAD_EVEN("GHL27.BIN", 0x40000, 0x20000, 0x82a7d49d ) /* 68000 code */
	ROM_LOAD_ODD ("GHL28.BIN", 0x40000, 0x20000, 0xe32cdaa6 ) /* 68000 code */
	ROM_LOAD_WIDE("GHL17.BIN", 0x80000, 0x80000, 0x12eee9a4 ) /* Tile map */

	ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "GHL6.BIN",  0x000000, 0x80000, 0xeaf5b7e3 ) /* Sprites / tiles */
	ROM_LOAD( "GHL5.BIN",  0x080000, 0x80000, 0xa4adc913 )
	ROM_LOAD( "GHL8.BIN",  0x100000, 0x80000, 0xe7fa3f94 )
	ROM_LOAD( "GHL7.BIN",  0x180000, 0x80000, 0x8587f2cb )
	ROM_LOAD( "GHL10.BIN", 0x200000, 0x10000, 0x1ad5edb3 ) /* Sprite set 2 */
	ROM_LOAD( "GHL23.BIN", 0x210000, 0x10000, 0xa1a14a87 )
	ROM_LOAD( "GHL14.BIN", 0x220000, 0x10000, 0xa1e55fd9 )
	ROM_LOAD( "GHL19.BIN", 0x230000, 0x10000, 0x142db95f )
	ROM_LOAD( "GHL12.BIN", 0x240000, 0x10000, 0xb0882fde )
	ROM_LOAD( "GHL25.BIN", 0x250000, 0x10000, 0x0f231d0f )
	ROM_LOAD( "GHL16.BIN", 0x260000, 0x10000, 0xc47e04ba )
	ROM_LOAD( "GHL21.BIN", 0x270000, 0x10000, 0x30cecdb6 )
	/* 32 * 32 tiles - left half */
	ROM_LOAD( "GHL18.BIN", 0x280000, 0x10000, 0x237eb922 ) /* Plane x */
	ROM_LOAD( "GHL09.BIN", 0x290000, 0x10000, 0x3d57242d ) /* Plane x */
	ROM_LOAD( "GHL22.BIN", 0x2a0000, 0x10000, 0x08a71d8f ) /* Plane x */
	ROM_LOAD( "GHL13.BIN", 0x2b0000, 0x10000, 0x18a99b29 ) /* Plane x */
	/* 32 * 32 tiles - right half */
	ROM_LOAD( "GHL20.BIN", 0x2c0000, 0x10000, 0xf2757633 ) /* Plane x */
	ROM_LOAD( "GHL11.BIN", 0x2d0000, 0x10000, 0x11957991 ) /* Plane x */
	ROM_LOAD( "GHL24.BIN", 0x2e0000, 0x10000, 0x3cf07f7a ) /* Plane x */
	ROM_LOAD( "GHL15.BIN", 0x2f0000, 0x10000, 0xdfcd0bfb ) /* Plane x */

	ROM_REGION(0x18000) /* 64k for the audio CPU */
	ROM_LOAD( "GHL26.BIN", 0x000000, 0x08000, 0x8df5e803 )
	ROM_CONTINUE(          0x010000, 0x08000 )
ROM_END

ROM_START( ghoulsj_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_EVEN("GHLJ29.BIN", 0x00000, 0x20000, 0x260306c9 ) /* 68000 code */
	ROM_LOAD_ODD ("GHLJ30.BIN", 0x00000, 0x20000, 0x324ef16e ) /* 68000 code */
	ROM_LOAD_EVEN("GHLJ27.BIN", 0x40000, 0x20000, 0xbf89b891 ) /* 68000 code */
	ROM_LOAD_ODD ("GHLJ28.BIN", 0x40000, 0x20000, 0xa6f27342 ) /* 68000 code */
	ROM_LOAD_WIDE("GHL17.BIN",  0x80000, 0x80000, 0x12eee9a4 ) /* Tile map */

	ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "GHL6.BIN",  0x000000, 0x80000, 0xeaf5b7e3 ) /* Sprites / tiles */
	ROM_LOAD( "GHL5.BIN",  0x080000, 0x80000, 0xa4adc913 )
	ROM_LOAD( "GHL8.BIN",  0x100000, 0x80000, 0xe7fa3f94 )
	ROM_LOAD( "GHL7.BIN",  0x180000, 0x80000, 0x8587f2cb )
	ROM_LOAD( "GHL10.BIN", 0x200000, 0x10000, 0x1ad5edb3 ) /* Sprite set 2 */
	ROM_LOAD( "GHL23.BIN", 0x210000, 0x10000, 0xa1a14a87 )
	ROM_LOAD( "GHL14.BIN", 0x220000, 0x10000, 0xa1e55fd9 )
	ROM_LOAD( "GHL19.BIN", 0x230000, 0x10000, 0x142db95f )
	ROM_LOAD( "GHL12.BIN", 0x240000, 0x10000, 0xb0882fde )
	ROM_LOAD( "GHL25.BIN", 0x250000, 0x10000, 0x0f231d0f )
	ROM_LOAD( "GHL16.BIN", 0x260000, 0x10000, 0xc47e04ba )
	ROM_LOAD( "GHL21.BIN", 0x270000, 0x10000, 0x30cecdb6 )
	/* 32 * 32 tiles - left half */
	ROM_LOAD( "GHL18.BIN", 0x280000, 0x10000, 0x237eb922 ) /* Plane x */
	ROM_LOAD( "GHL09.BIN", 0x290000, 0x10000, 0x3d57242d ) /* Plane x */
	ROM_LOAD( "GHL22.BIN", 0x2a0000, 0x10000, 0x08a71d8f ) /* Plane x */
	ROM_LOAD( "GHL13.BIN", 0x2b0000, 0x10000, 0x18a99b29 ) /* Plane x */
	/* 32 * 32 tiles - right half */
	ROM_LOAD( "GHL20.BIN", 0x2c0000, 0x10000, 0xf2757633 ) /* Plane x */
	ROM_LOAD( "GHL11.BIN", 0x2d0000, 0x10000, 0x11957991 ) /* Plane x */
	ROM_LOAD( "GHL24.BIN", 0x2e0000, 0x10000, 0x3cf07f7a ) /* Plane x */
	ROM_LOAD( "GHL15.BIN", 0x2f0000, 0x10000, 0xdfcd0bfb ) /* Plane x */

	ROM_REGION(0x18000) /* 64k for the audio CPU */
	ROM_LOAD( "GHL26.BIN", 0x000000, 0x08000, 0x8df5e803 )
	ROM_CONTINUE(          0x010000, 0x08000 )
ROM_END


struct GameDriver ghouls_driver =
{
	__FILE__,
	0,
	"ghouls",
	"Ghouls'n Ghosts (US?)",
	"1988",
	"Capcom",
	CPS1_CREDITS("Paul Leaman\nMarco Cassili (dip switches)"),
	0,
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

struct GameDriver ghoulsj_driver =
{
	__FILE__,
	&ghouls_driver,
	"ghoulsj",
	"Ghouls'n Ghosts (Japan)",
	"1988",
	"Capcom",
	CPS1_CREDITS("Paul Leaman\nMarco Cassili (dip switches)"),
	0,
	&ghouls_machine_driver,

	ghoulsj_rom,
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

CHAR_LAYOUT2  (charlayout_cawingj,   2048,   0x80000*8)
SPRITE_LAYOUT2(spritelayout_cawingj, 0x1800)
SPRITE_LAYOUT2(tilelayout_cawingj,   0x2400) //0x2800)
TILE32_LAYOUT2(tilelayout32_cawingj, 0x400,  0x40000*8, 0x80000*8)

static struct GfxDecodeInfo gfxdecodeinfo_cawingj[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x028000, &charlayout_cawingj,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_cawingj,  0,                      32 },
        { 1, 0x02c000, &tilelayout_cawingj,    32*16+32*16,            32 },
        { 1, 0x018000, &tilelayout32_cawingj,  32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};


MACHINE_DRIVER(
        cawingj_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_cawingj,
        CPS1_DEFAULT_CPU_SPEED)

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
        ROM_LOAD( "CAJ-S.BIN",    0x00000, 0x08000, 0xa7fe4540 )
                   ROM_CONTINUE(  0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("CAJ30.BIN",    0x00000, 0x20000, 0x71b13751 )
        ROM_LOAD ("CAJ31.BIN",    0x20000, 0x20000, 0x25b14e0b )
ROM_END

struct GameDriver cawingj_driver =
{
	__FILE__,
	0,
	"cawingj",
	"Carrier Air Wing (Japan)",
	"1990",
	"Capcom",
	CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
	GAME_NOT_WORKING,
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


/********************************************************************

                       Super Street Fighter 2

  Not finished yet.

********************************************************************/

CHAR_LAYOUT(charlayout_sf2, 1*4096, 0x100000*8)
SPRITE_LAYOUT(spritelayout_sf2, 6*4096, 0x80000*8, 0x100000*8)
TILE32_LAYOUT(tile32layout_sf2, 1,    0x080000*8, 0x100000*8)
TILE_LAYOUT2(tilelayout_sf2, 1,     0x080000*8, 0x020000*8 )

static struct GfxDecodeInfo gfxdecodeinfo_sf2[] =
{
        /*   start    pointer                 start   number of colours */
        { 1, 0x400000, &charlayout_sf2,       32*16,                  32 },
        { 1, 0x000000, &spritelayout_sf2,     0,                      32 },
        { 1, 0x200000, &tilelayout_sf2,       32*16+32*16,            32 },
        { 1, 0x050000, &tile32layout_sf2,     32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        sf2_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_sf2,
        12000000)              /* 12 MHz */

ROM_START( sf2_rom )
        ROM_REGION(0x180000)      /* 68000 code */

        ROM_LOAD_WIDE_SWAP("SF2.23", 0x000000, 0x80000, 0x6c67bfcf )
        ROM_LOAD_WIDE_SWAP("SF2.22", 0x080000, 0x80000, 0x06b81390 )
        ROM_LOAD_WIDE_SWAP("SF2.21", 0x100000, 0x80000, 0xac363d78 )

        ROM_REGION(0x600000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "SF2.01",   0x000000, 0x80000, 0x352afe30 ) /* sprites */
        ROM_LOAD( "SF2.05",   0x080000, 0x80000, 0x768c375a ) /* sprites */
        ROM_LOAD( "SF2.03",   0x100000, 0x80000, 0x3c9240f4 ) /* sprites */
        ROM_LOAD( "SF2.07",   0x180000, 0x80000, 0x95e77d43 ) /* sprites */

        ROM_LOAD( "SF2.02",   0x200000, 0x80000, 0x352afe30 ) /* sprites */
        ROM_LOAD( "SF2.06",   0x280000, 0x80000, 0x768c375a ) /* sprites */
        ROM_LOAD( "SF2.04",   0x300000, 0x80000, 0x3c9240f4 ) /* sprites */
        ROM_LOAD( "SF2.08",   0x380000, 0x80000, 0x95e77d43 ) /* sprites */
        ROM_LOAD( "SF2.10",   0x400000, 0x80000, 0x352afe30 ) /* sprites */
        ROM_LOAD( "SF2.12",   0x480000, 0x80000, 0x768c375a ) /* sprites */
        ROM_LOAD( "SF2.11",   0x500000, 0x80000, 0x3c9240f4 ) /* sprites */
        ROM_LOAD( "SF2.13",   0x580000, 0x80000, 0x95e77d43 ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "SF2.09",    0x00000, 0x10000, 0x56014501 )
        ROM_LOAD( "SF2.18",    0x08000, 0x10000, 0x56014501 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ("SF2.18",    0x00000, 0x20000, 0x58bb142b )
        ROM_LOAD ("SF2.19",    0x20000, 0x20000, 0xc575f23d )
ROM_END

struct GameDriver sf2_driver =
{
	__FILE__,
	0,
        "sf2",
        "Street Fighter 2",
	"????",
	"?????",
        CPS1_CREDITS("Paul Leaman"),
	0,
        &sf2_machine_driver,

        sf2_rom,
        0,0,0,0,
        input_ports_strider,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};


