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
    { 0x000000, 0x17ffff, MRA_ROM },     /* 68000 ROM */
        { 0xff0000, 0xffffff, MRA_BANK2 },   /* RAM */
        { 0x900000, 0x92ffff, MRA_BANK5 },
        { 0x800000, 0x800003, cps1_player_input_r}, /* Player input ports */
        { 0x800018, 0x80001f, cps1_input_r}, /* Input ports */
                { 0x860000, 0x8603ff, MRA_BANK3, &cps1_eeprom, &cps1_eeprom_size },   /* EEPROM */
        { 0x800100, 0x8001ff, MRA_BANK4 },  /* Output ports */
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress cps1_writemem[] =
{
    { 0x000000, 0x17ffff, MWA_ROM },      /* ROM */
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
    { 3*BITSEP, 2*BITSEP, BITSEP, 0,  },     \
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
  {"varth",   0x0800,     0,0x1800,0x0c00, 2, 0x5820,0x1800,0x0c00,0,0},
  {"mtwins",       0,     0,0x2000,0x0e00, 3, 0x0020,0x0000,0x0000,0x5e,0x0404},
  {"chikij",       0,     0,0x2000,0x0e00, 3, 0x0020,0x0000,0x0000,0x5e,0x0404},
  {"knights", 0x0800,0x0000,0x4c00,0x1a00, 3,     -1,    -1,    -1,0x00,0x0000},
  {"strider",      0,0x0200,0x1000,     0, 4, 0x0020,0x0020,0x0020,0x00,0x0000},
  {"striderj",     0,0x0200,0x1000,     0, 4, 0x0020,0x0020,0x0020,0x00,0x0000},
  {"ghouls",  0x0000,0x0000,0x2000,0x1000, 5, 0x2420,0x2000,0x1000,0x00,0x0000, 1},
  {"ghoulsj", 0x0000,0x0000,0x2000,0x1000, 5, 0x2420,0x2000,0x1000,0x00,0x0000, 1},
  {"1941",         0,     0,0x2400,0x0400, 6, 0x0020,0x0400,0x2420,0x60,0x0005},
  {"1941j",        0,     0,0x2400,0x0400, 6, 0x0020,0x0400,0x2420,0x60,0x0005},
  {"3wonders",0x5400,     0,0x2000,0x0400, 6,     -1,0x3bff,0x04ff,0x72,0x0800},
  {"msword",       0,     0,0x2800,0x0e00, 7,     -1,    -1,    -1,0x00,0x0000},
  {"mswordj",      0,     0,0x2800,0x0e00, 7,     -1,    -1,    -1,0x00,0x0000},
  {"nemo",         0,     0,0x2400,0x0d00, 8, 0x4020,0x2400,0x0d00,0x4e,0x0405},
  {"nemoj",        0,     0,0x2400,0x0d00, 8, 0x4020,0x2400,0x0d00,0x4e,0x0405},
  {"dynwarsj",     0,     0,0x2000,     0, 9, 0x6020,0x2000,0x0000,0,0},
  {"mercs",        0,0x2600,0x0600,0x8e6c, 9, 0x0020,0x0600,0x8e6c,0x60,0x0402, 1},
  {"unsquad",      0,     0,0x2000,     0, 0, 0x0020,0x0000,0x0000,0x00,0x0000},
  {"area88",       0,     0,0x2000,     0, 0, 0x0020,0x0000,0x0000,0x00,0x0000},
  {"captcomm",     0,     0,0x6800,0x1400,10,     -1,0x6800,0x1400,0x72,0x0800},
  {"cawingj", 0x5000,     0,0x2c00,0x0600,11, 0x5020,0x2c00,0x0600,0x00,0x0000},
  {"pnickj",       0,0x0800,0x0800,0x0c00, 0,     -1,    -1,    -1,0x00,0x0000},
  {"sf2",          0,     0,0x2800,0x0c00, 0,     -1,    -1,    -1,0x00,0x0000},
  {"sf2ce",        0,     0,0x2800,0x0c00, 0,     -1,    -1,    -1,0x00,0x0000},
  {"sf2cej",       0,     0,0x2800,0x0c00, 0,     -1,    -1,    -1,0x00,0x0000},
  {"rockmanj",     0,0x0c00,0xb000,0x3400, 0, 0x0020,0xb000,0x3400,0x00,0x0000},
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
                else if (strcmp(gamename, "mercs")==0)
        {
                /*
                                Quick hack (NOP out CPSB test)
                                Game writes to port before reading, destroying the value
                                setup.
                                */
                if (errorlog)
                {
                   fprintf(errorlog, "Patching CPSB test\n");
                }
                                WRITE_WORD(&RAM[0x0700], 0x4e71);
                                WRITE_WORD(&RAM[0x0702], 0x4e71);
        }
        else if (strcmp(gamename, "ghouls")==0)
        {
                /* Patch out self-test... it takes forever */
                WRITE_WORD(&RAM[0x61964+0], 0x4ef9);
                WRITE_WORD(&RAM[0x61964+2], 0x0000);
                WRITE_WORD(&RAM[0x61964+4], 0x0400);
        }
                else if (strcmp(gamename, "sf2")==0)
        {
                                WRITE_WORD(&RAM[0xb9370], 0x4e71);
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
        ROM_LOAD_EVEN( "strider.30",   0x00000, 0x20000, 0xda997474 )
        ROM_LOAD_ODD ( "strider.35",   0x00000, 0x20000, 0x5463aaa3 )
        ROM_LOAD_EVEN( "strider.31",   0x40000, 0x20000, 0xd20786db )
        ROM_LOAD_ODD ( "strider.36",   0x40000, 0x20000, 0x21aa2863 )
        ROM_LOAD_WIDE_SWAP( "strider.32",   0x80000, 0x80000, 0x9b3cfc08 ) /* Tile map */

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "strider.02",   0x000000, 0x80000, 0x7705aa46 )
        ROM_LOAD( "strider.06",   0x080000, 0x80000, 0x4eee9aea )
        ROM_LOAD( "strider.04",   0x100000, 0x80000, 0x5b18b722 )
        ROM_LOAD( "strider.08",   0x180000, 0x80000, 0x2d7f21e4 )
        ROM_LOAD( "strider.01",   0x200000, 0x80000, 0xb7d04e8b )
        ROM_LOAD( "strider.05",   0x280000, 0x80000, 0x005f000b )
        ROM_LOAD( "strider.03",   0x300000, 0x80000, 0x6b4713b4 )
        ROM_LOAD( "strider.07",   0x380000, 0x80000, 0xb9441519 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "strider.09",   0x00000, 0x10000, 0x2ed403bc )
        ROM_RELOAD(             0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "strider.18",   0x00000, 0x20000, 0x4386bc80 )
        ROM_LOAD( "strider.19",   0x20000, 0x20000, 0x444536d7 )
ROM_END

ROM_START( striderj_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_WIDE_SWAP( "sthj23.bin",   0x00000, 0x80000, 0x046e7b12 )
        ROM_LOAD_WIDE_SWAP( "strider.32",   0x80000, 0x80000, 0x9b3cfc08 ) /* Tile map */

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "strider.02",   0x000000, 0x80000, 0x7705aa46 )
        ROM_LOAD( "strider.06",   0x080000, 0x80000, 0x4eee9aea )
        ROM_LOAD( "strider.04",   0x100000, 0x80000, 0x5b18b722 )
        ROM_LOAD( "strider.08",   0x180000, 0x80000, 0x2d7f21e4 )
        ROM_LOAD( "strider.01",   0x200000, 0x80000, 0xb7d04e8b )
        ROM_LOAD( "strider.05",   0x280000, 0x80000, 0x005f000b )
        ROM_LOAD( "strider.03",   0x300000, 0x80000, 0x6b4713b4 )
        ROM_LOAD( "strider.07",   0x380000, 0x80000, 0xb9441519 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "strider.09",   0x00000, 0x10000, 0x2ed403bc )
        ROM_RELOAD(             0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "strider.18",   0x00000, 0x20000, 0x4386bc80 )
        ROM_LOAD( "strider.19",   0x20000, 0x20000, 0x444536d7 )
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
TILE_LAYOUT2(tilelayout_willow, 2*4096,     0x080000*8, 0x020000*8)

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
        ROM_LOAD_EVEN( "wlu_30.rom",   0x00000, 0x20000, 0xd604dbb1 )
        ROM_LOAD_ODD ( "wlu_35.rom",   0x00000, 0x20000, 0xdaee72fe )
        ROM_LOAD_EVEN( "wlu_31.rom",   0x40000, 0x20000, 0x0eb48a83 )
        ROM_LOAD_ODD ( "wlu_36.rom",   0x40000, 0x20000, 0x36100209 )
        ROM_LOAD_WIDE_SWAP( "wl_32.rom",    0x80000, 0x80000, 0xdfd9f643 ) /* Tile map */

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "wl_gfx1.rom",  0x000000, 0x80000, 0xc6f2abce )
        ROM_LOAD( "wl_gfx5.rom",  0x080000, 0x80000, 0xafa74b73 )
        ROM_LOAD( "wl_gfx3.rom",  0x100000, 0x80000, 0x4aa4c6d3 )
        ROM_LOAD( "wl_gfx7.rom",  0x180000, 0x80000, 0x12a0dc0b )
        ROM_LOAD( "wl_20.rom",    0x200000, 0x20000, 0x84992350 )
        ROM_LOAD( "wl_10.rom",    0x220000, 0x20000, 0xb87b5a36 )
        ROM_LOAD( "wl_22.rom",    0x240000, 0x20000, 0xfd3f89f0 )
        ROM_LOAD( "wl_12.rom",    0x260000, 0x20000, 0x7da49d69 )
        ROM_LOAD( "wl_24.rom",    0x280000, 0x20000, 0x6f0adee5 )
        ROM_LOAD( "wl_14.rom",    0x2a0000, 0x20000, 0x9cf3027d )
        ROM_LOAD( "wl_26.rom",    0x2c0000, 0x20000, 0xf09c8ecf )
        ROM_LOAD( "wl_16.rom",    0x2e0000, 0x20000, 0xe35407aa )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "wl_09.rom",    0x00000, 0x08000, 0xf6b3d060 )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "wl_18.rom",    0x00000, 0x20000, 0xbde23d4d )
        ROM_LOAD ( "wl_19.rom",    0x20000, 0x20000, 0x683898f5 )
ROM_END

ROM_START( willowj_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "wl36.bin",     0x00000, 0x20000, 0x2b0d7cbc )
        ROM_LOAD_ODD ( "wl42.bin",     0x00000, 0x20000, 0x1ac39615 )
        ROM_LOAD_EVEN( "wl37.bin",     0x40000, 0x20000, 0x30a717fa )
        ROM_LOAD_ODD ( "wl43.bin",     0x40000, 0x20000, 0xd0dddc9e )
        ROM_LOAD_WIDE_SWAP( "wl_32.rom",    0x80000, 0x80000, 0xdfd9f643 ) /* Tile map */

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "wl_gfx1.rom",  0x000000, 0x80000, 0xc6f2abce )
        ROM_LOAD( "wl_gfx5.rom",  0x080000, 0x80000, 0xafa74b73 )
        ROM_LOAD( "wl_gfx3.rom",  0x100000, 0x80000, 0x4aa4c6d3 )
        ROM_LOAD( "wl_gfx7.rom",  0x180000, 0x80000, 0x12a0dc0b )
        ROM_LOAD( "wl_20.rom",    0x200000, 0x20000, 0x84992350 )
        ROM_LOAD( "wl_10.rom",    0x220000, 0x20000, 0xb87b5a36 )
        ROM_LOAD( "wl_22.rom",    0x240000, 0x20000, 0xfd3f89f0 )
        ROM_LOAD( "wl_12.rom",    0x260000, 0x20000, 0x7da49d69 )
        ROM_LOAD( "wl_24.rom",    0x280000, 0x20000, 0x6f0adee5 )
        ROM_LOAD( "wl_14.rom",    0x2a0000, 0x20000, 0x9cf3027d )
        ROM_LOAD( "wl_26.rom",    0x2c0000, 0x20000, 0xf09c8ecf )
        ROM_LOAD( "wl_16.rom",    0x2e0000, 0x20000, 0xe35407aa )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "wl_09.rom",    0x00000, 0x08000, 0xf6b3d060 )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "wl_18.rom",    0x00000, 0x20000, 0xbde23d4d )
        ROM_LOAD ( "wl_19.rom",    0x20000, 0x20000, 0x683898f5 )
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
        ROM_LOAD_EVEN( "ff30-36.bin",  0x00000, 0x20000, 0xf9a5ce83 )
        ROM_LOAD_ODD ( "ff35-42.bin",  0x00000, 0x20000, 0x65f11215 )
        ROM_LOAD_EVEN( "ff31-37.bin",  0x40000, 0x20000, 0xe1033784 )
        ROM_LOAD_ODD ( "ff36-43.bin",  0x40000, 0x20000, 0x995e968a )
        ROM_LOAD_WIDE_SWAP( "ff32-32m.bin", 0x80000, 0x80000, 0xc747696e ) /* Tile map */

        ROM_REGION(0x500000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "ff01-01m.bin", 0x000000, 0x80000, 0x0b605e44 )
        ROM_LOAD( "ff05-05m.bin", 0x080000, 0x80000, 0x9c284108 )
        ROM_LOAD( "ff03-03m.bin", 0x100000, 0x80000, 0x52291cd2 )
        ROM_LOAD( "ff07-07m.bin", 0x180000, 0x80000, 0xa7584dfb )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ff09-09.bin",  0x00000, 0x10000, 0xb8367eb5 )
        ROM_RELOAD(              0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "ff18-18.bin",  0x00000, 0x20000, 0x375c66e7 )
        ROM_LOAD ( "ff19-19.bin",  0x20000, 0x20000, 0x1ef137f9 )
ROM_END

ROM_START( ffightj_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "ff30-36.bin",  0x00000, 0x20000, 0xf9a5ce83 )
        ROM_LOAD_ODD ( "ff35-42.bin",  0x00000, 0x20000, 0x65f11215 )
        ROM_LOAD_EVEN( "ff31-37.bin",  0x40000, 0x20000, 0xe1033784 )
        ROM_LOAD_ODD ( "ff43.bin",     0x40000, 0x20000, 0xb6dee1c3 )
        ROM_LOAD_WIDE_SWAP( "ff32-32m.bin", 0x80000, 0x80000, 0xc747696e ) /* Tile map */

        ROM_REGION(0x500000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD_EVEN( "ff17.bin",     0x000000, 0x20000, 0x2dc18cf4 )
        ROM_LOAD_ODD ( "ff24.bin",     0x000000, 0x20000, 0xa1ab607a )
        ROM_LOAD_EVEN( "ff18.bin",     0x040000, 0x20000, 0xb19ede59 )
        ROM_LOAD_ODD ( "ff25.bin",     0x040000, 0x20000, 0x6e8181ea )
        ROM_LOAD_EVEN( "ff01.bin",     0x080000, 0x20000, 0x815b1797 )
        ROM_LOAD_ODD ( "ff09.bin",     0x080000, 0x20000, 0x5b116d0d )
        ROM_LOAD_EVEN( "ff02.bin",     0x0c0000, 0x20000, 0x5d91f694 )
        ROM_LOAD_ODD ( "ff10.bin",     0x0c0000, 0x20000, 0x624a924a )
        ROM_LOAD_EVEN( "ff32.bin",     0x100000, 0x20000, 0xc8bc4a57 )
        ROM_LOAD_ODD ( "ff38.bin",     0x100000, 0x20000, 0x6535a57f )
        ROM_LOAD_EVEN( "ff33.bin",     0x140000, 0x20000, 0x7369fa07 )
        ROM_LOAD_ODD ( "ff39.bin",     0x140000, 0x20000, 0x9416b477 )
        ROM_LOAD_EVEN( "ff05.bin",     0x180000, 0x20000, 0xd0fcd4b5 )
        ROM_LOAD_ODD ( "ff13.bin",     0x180000, 0x20000, 0x8721a7da )
        ROM_LOAD_EVEN( "ff06.bin",     0x1c0000, 0x20000, 0x1c18f042 )
        ROM_LOAD_ODD ( "ff14.bin",     0x1c0000, 0x20000, 0x0a2e9101 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ff09-09.bin",  0x00000, 0x10000, 0xb8367eb5 )
        ROM_RELOAD(              0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "ff18-18.bin",  0x00000, 0x20000, 0x375c66e7 )
        ROM_LOAD ( "ff19-19.bin",  0x20000, 0x20000, 0x1ef137f9 )
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
        ROM_LOAD_EVEN( "unsquad.30",   0x00000, 0x20000, 0x24d8f88d )
        ROM_LOAD_ODD ( "unsquad.35",   0x00000, 0x20000, 0x8b954b59 )
        ROM_LOAD_EVEN( "unsquad.31",   0x40000, 0x20000, 0x33e9694b )
        ROM_LOAD_ODD ( "unsquad.36",   0x40000, 0x20000, 0x7cc8fb9e )
        ROM_LOAD_WIDE_SWAP( "unsquad.32",   0x80000, 0x80000, 0xae1d7fb0 ) /* tiles + chars */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "unsquad.01",   0x000000, 0x80000, 0x5965ca8d )
        ROM_LOAD( "unsquad.05",   0x080000, 0x80000, 0xbf4575d8 )
        ROM_LOAD( "unsquad.03",   0x100000, 0x80000, 0xac6db17d )
        ROM_LOAD( "unsquad.07",   0x180000, 0x80000, 0xa02945f4 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "unsquad.09",   0x00000, 0x10000, 0xf3dd1367 )
        ROM_RELOAD(             0x08000, 0x10000 )

        ROM_REGION(0x20000) /* Samples */
        ROM_LOAD ( "unsquad.18",   0x00000, 0x20000, 0x584b43a9 )
ROM_END

ROM_START( area88_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "ar36.bin",     0x00000, 0x20000, 0x65030392 )
        ROM_LOAD_ODD ( "ar42.bin",     0x00000, 0x20000, 0xc48170de )
        ROM_LOAD_EVEN( "unsquad.31",   0x40000, 0x20000, 0x33e9694b )
        ROM_LOAD_ODD ( "unsquad.36",   0x40000, 0x20000, 0x7cc8fb9e )
        ROM_LOAD_WIDE_SWAP( "unsquad.32",   0x80000, 0x80000, 0xae1d7fb0 ) /* tiles + chars */

        ROM_REGION_DISPOSE(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "unsquad.01",   0x000000, 0x80000, 0x5965ca8d )
        ROM_LOAD( "unsquad.05",   0x080000, 0x80000, 0xbf4575d8 )
        ROM_LOAD( "unsquad.03",   0x100000, 0x80000, 0xac6db17d )
        ROM_LOAD( "unsquad.07",   0x180000, 0x80000, 0xa02945f4 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "unsquad.09",   0x00000, 0x10000, 0xf3dd1367 )
        ROM_RELOAD(             0x08000, 0x10000 )

        ROM_REGION(0x20000) /* Samples */
        ROM_LOAD ( "unsquad.18",   0x00000, 0x20000, 0x584b43a9 )
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

struct GameDriver area88_driver =
{
        __FILE__,
        &unsquad_driver,
        "area88",
        "Area 88",
        "1989",
        "Capcom",
        CPS1_CREDITS("Paul Leaman (Game Driver)\nMarco Cassili (dip switches)"),
        0,
        &unsquad_machine_driver,

        area88_rom,
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
        ROM_LOAD_EVEN( "che_30.rom",   0x00000, 0x20000, 0x9a2a2db1 )
        ROM_LOAD_ODD ( "che_35.rom",   0x00000, 0x20000, 0xa7f96b02 )
        ROM_LOAD_EVEN( "che_31.rom",   0x40000, 0x20000, 0xbbff8a99 )
        ROM_LOAD_ODD ( "che_36.rom",   0x40000, 0x20000, 0x0fa00c39 )
        ROM_LOAD_WIDE_SWAP( "ch_32.rom",    0x80000, 0x80000, 0x9b70bd41 ) /* tiles + chars */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "ch_gfx1.rom",  0x000000, 0x80000, 0xf33ca9d4 )
        ROM_LOAD( "ch_gfx5.rom",  0x080000, 0x80000, 0x4ec75f15 )
        ROM_LOAD( "ch_gfx3.rom",  0x100000, 0x80000, 0x0ba2047f )
        ROM_LOAD( "ch_gfx7.rom",  0x180000, 0x80000, 0xd85d00d6 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ch_09.rom",    0x00000, 0x10000, 0x4d4255b7 )
        ROM_RELOAD(            0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "ch_18.rom",    0x00000, 0x20000, 0xf909e8de )
        ROM_LOAD( "ch_19.rom",    0x20000, 0x20000, 0xfc158cf7 )
ROM_END

ROM_START( chikij_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "chj36a.bin",   0x00000, 0x20000, 0xec1328d8 )
        ROM_LOAD_ODD ( "chj42a.bin",   0x00000, 0x20000, 0x4ae13503 )
        ROM_LOAD_EVEN( "chj37a.bin",   0x40000, 0x20000, 0x46d2cf7b )
        ROM_LOAD_ODD ( "chj43a.bin",   0x40000, 0x20000, 0x8d387fe8 )
        ROM_LOAD_WIDE_SWAP( "ch_32.rom",    0x80000, 0x80000, 0x9b70bd41 ) /* tiles + chars */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "ch_gfx1.rom",  0x000000, 0x80000, 0xf33ca9d4 )
        ROM_LOAD( "ch_gfx5.rom",  0x080000, 0x80000, 0x4ec75f15 )
        ROM_LOAD( "ch_gfx3.rom",  0x100000, 0x80000, 0x0ba2047f )
        ROM_LOAD( "ch_gfx7.rom",  0x180000, 0x80000, 0xd85d00d6 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ch_09.rom",    0x00000, 0x10000, 0x4d4255b7 )
        ROM_RELOAD(            0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "ch_18.rom",    0x00000, 0x20000, 0xf909e8de )
        ROM_LOAD( "ch_19.rom",    0x20000, 0x20000, 0xfc158cf7 )
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
        ROM_LOAD_EVEN( "nme_30a.rom",  0x00000, 0x20000, 0xd2c03e56 )
        ROM_LOAD_ODD ( "nme_35a.rom",  0x00000, 0x20000, 0x5fd31661 )
        ROM_LOAD_EVEN( "nme_31a.rom",  0x40000, 0x20000, 0xb2bd4f6f )
        ROM_LOAD_ODD ( "nme_36a.rom",  0x40000, 0x20000, 0xee9450e3 )
        ROM_LOAD_WIDE_SWAP( "nm_32.rom",    0x80000, 0x80000, 0xd6d1add3 ) /* Tile map */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "nm_gfx1.rom",  0x000000, 0x80000, 0x9e878024 )
        ROM_LOAD( "nm_gfx5.rom",  0x080000, 0x80000, 0x487b8747 )
        ROM_LOAD( "nm_gfx3.rom",  0x100000, 0x80000, 0xbb01e6b6 )
        ROM_LOAD( "nm_gfx7.rom",  0x180000, 0x80000, 0x203dc8c6 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "nm_09.rom",    0x000000, 0x08000, 0x0f4b0581 )
        ROM_CONTINUE(          0x010000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "nm_18.rom",    0x00000, 0x20000, 0xbab333d4 )
        ROM_LOAD( "nm_19.rom",    0x20000, 0x20000, 0x2650a0a8 )
ROM_END

ROM_START( nemoj_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "nm36.bin",     0x00000, 0x20000, 0xdaeceabb )
        ROM_LOAD_ODD ( "nm42.bin",     0x00000, 0x20000, 0x55024740 )
        ROM_LOAD_EVEN( "nm37.bin",     0x40000, 0x20000, 0x619068b6 )
        ROM_LOAD_ODD ( "nm43.bin",     0x40000, 0x20000, 0xa948a53b )
        ROM_LOAD_WIDE_SWAP( "nm_32.rom",    0x80000, 0x80000, 0xd6d1add3 ) /* Tile map */

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "nm_gfx1.rom",  0x000000, 0x80000, 0x9e878024 )
        ROM_LOAD( "nm_gfx5.rom",  0x080000, 0x80000, 0x487b8747 )
        ROM_LOAD( "nm_gfx3.rom",  0x100000, 0x80000, 0xbb01e6b6 )
        ROM_LOAD( "nm_gfx7.rom",  0x180000, 0x80000, 0x203dc8c6 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "nm_09.rom",    0x000000, 0x08000, 0x0f4b0581 )
        ROM_CONTINUE(          0x010000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "nm_18.rom",    0x00000, 0x20000, 0xbab333d4 )
        ROM_LOAD( "nm_19.rom",    0x20000, 0x20000, 0x2650a0a8 )
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
        ROM_LOAD_EVEN( "41e_30.rom",   0x00000, 0x20000, 0x9deb1e75 )
        ROM_LOAD_ODD ( "41e_35.rom",   0x00000, 0x20000, 0xd63942b3 )
        ROM_LOAD_EVEN( "41e_31.rom",   0x40000, 0x20000, 0xdf201112 )
        ROM_LOAD_ODD ( "41e_36.rom",   0x40000, 0x20000, 0x816a818f )
        ROM_LOAD_WIDE_SWAP( "41_32.rom",    0x80000, 0x80000, 0x4e9648ca )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "41_gfx1.rom",  0x000000, 0x80000, 0xff77985a )
        ROM_LOAD( "41_gfx5.rom",  0x080000, 0x80000, 0x01d1cb11 )
        ROM_LOAD( "41_gfx3.rom",  0x100000, 0x80000, 0x983be58f )
        ROM_LOAD( "41_gfx7.rom",  0x180000, 0x80000, 0xaeaa3509 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "41_09.rom",    0x000000, 0x08000, 0x0f9d8527 )
        ROM_CONTINUE(          0x010000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "41_18.rom",    0x00000, 0x20000, 0xd1f15aeb )
        ROM_LOAD( "41_19.rom",    0x20000, 0x20000, 0x15aec3a6 )
ROM_END

ROM_START( c1941j_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "4136.bin",     0x00000, 0x20000, 0x7fbd42ab )
        ROM_LOAD_ODD ( "4142.bin",     0x00000, 0x20000, 0xc7781f89 )
        ROM_LOAD_EVEN( "4137.bin",     0x40000, 0x20000, 0xc6464b0b )
        ROM_LOAD_ODD ( "4143.bin",     0x40000, 0x20000, 0x440fc0b5 )
        ROM_LOAD_WIDE_SWAP( "41_32.rom",    0x80000, 0x80000, 0x4e9648ca )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "41_gfx1.rom",  0x000000, 0x80000, 0xff77985a )
        ROM_LOAD( "41_gfx5.rom",  0x080000, 0x80000, 0x01d1cb11 )
        ROM_LOAD( "41_gfx3.rom",  0x100000, 0x80000, 0x983be58f )
        ROM_LOAD( "41_gfx7.rom",  0x180000, 0x80000, 0xaeaa3509 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "41_09.rom",    0x000000, 0x08000, 0x0f9d8527 )
        ROM_CONTINUE(          0x010000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "41_18.rom",    0x00000, 0x20000, 0xd1f15aeb )
        ROM_LOAD( "41_19.rom",    0x20000, 0x20000, 0x15aec3a6 )
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

                                                  Dynasty Wars

********************************************************************/

INPUT_PORTS_START( input_ports_dynwars )
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
        PORT_DIPNAME( 0xff, 0xff, "DIP A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0xff, 0xff, "DIP B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0xff, 0xff, "DIP C", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

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


CHAR_LAYOUT(charlayout_dynwars,     4096*2, 0x100000*8)
SPRITE_LAYOUT(spritelayout_dynwars, 4096*4-2048, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_dynwars, 4096*2,   0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_dynwars, 2048,   0x080000*8, 0x100000*8)

static struct GfxDecodeInfo gfxdecodeinfo_dynwars[] =
{
        /*   start    pointer               colour start   number of colours */
                { 1, 0x060000, &charlayout_dynwars,    32*16,                  32 },
                { 1, 0x000000, &spritelayout_dynwars,  0    ,                  32 },
                { 1, 0x240000, &tilelayout_dynwars,    32*16+32*16,            32 },
                { 1, 0x200000, &tilelayout32_dynwars,  32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
                dynwars_machine_driver,
        cps1_interrupt2,
                gfxdecodeinfo_dynwars,
        CPS1_DEFAULT_CPU_SPEED)


ROM_START( dynwarsj_rom )
        ROM_REGION(0x100000)      /* 68000 code */

        ROM_LOAD_EVEN( "36.bin",       0x00000, 0x20000, 0x1a516657 )
        ROM_LOAD_ODD ( "42.bin",       0x00000, 0x20000, 0x12a290a0 )
        ROM_LOAD_EVEN( "37.bin",       0x40000, 0x20000, 0x932fc943 )
        ROM_LOAD_ODD ( "43.bin",       0x40000, 0x20000, 0x872ad76d )

        ROM_LOAD_EVEN( "34.bin",       0x80000, 0x20000, 0x8f663d00 )
        ROM_LOAD_ODD ( "40.bin",       0x80000, 0x20000, 0x1586dbf3 )
        ROM_LOAD_EVEN( "35.bin",       0xc0000, 0x20000, 0x9db93d7a )
        ROM_LOAD_ODD ( "41.bin",       0xc0000, 0x20000, 0x1aae69a4 )

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD_EVEN( "17.bin",       0x000000, 0x20000, 0x2e2f8320 )
        ROM_LOAD_ODD ( "24.bin",       0x000000, 0x20000, 0xc6909b6f )
        ROM_LOAD_EVEN( "18.bin",       0x040000, 0x20000, 0x1833f932 )
        ROM_LOAD_ODD ( "25.bin",       0x040000, 0x20000, 0x152ea74a )

        ROM_LOAD_EVEN( "01.bin",       0x080000, 0x20000, 0x187b2886 )
        ROM_LOAD_ODD ( "09.bin",       0x080000, 0x20000, 0xc3e83c69 )
        ROM_LOAD_EVEN( "02.bin",       0x0c0000, 0x20000, 0xcc83c02f )
        ROM_LOAD_ODD ( "10.bin",       0x0c0000, 0x20000, 0xff28f8d0 )

        ROM_LOAD_EVEN( "32.bin",       0x100000, 0x20000, 0x21a0a453 )
        ROM_LOAD_ODD ( "38.bin",       0x100000, 0x20000, 0xcd7923ed )
        ROM_LOAD_EVEN( "33.bin",       0x140000, 0x20000, 0x89de1533 )
        ROM_LOAD_ODD ( "39.bin",       0x140000, 0x20000, 0xbc09b360 )

        ROM_LOAD_EVEN( "05.bin",       0x180000, 0x20000, 0x339378b8 )
        ROM_LOAD_ODD ( "13.bin",       0x180000, 0x20000, 0x0273d87d )
        ROM_LOAD_EVEN( "06.bin",       0x1c0000, 0x20000, 0x6f9edd75 )
        ROM_LOAD_ODD ( "14.bin",       0x1c0000, 0x20000, 0x18fb232c )

        /* TILES */
        ROM_LOAD_EVEN( "19.bin",       0x200000, 0x20000, 0x7114e5c6 )
        ROM_LOAD_ODD ( "26.bin",       0x200000, 0x20000, 0x07fc714b )
        ROM_LOAD_EVEN( "20.bin",       0x240000, 0x20000, 0x002796dc )
        ROM_LOAD_ODD ( "27.bin",       0x240000, 0x20000, 0xa27e81fa )

        ROM_LOAD_EVEN( "03.bin",       0x280000, 0x20000, 0x7bf51337 )
        ROM_LOAD_ODD ( "11.bin",       0x280000, 0x20000, 0x29eaf490 )
        ROM_LOAD_EVEN( "04.bin",       0x2c0000, 0x20000, 0x4951bc0f )
        ROM_LOAD_ODD ( "12.bin",       0x2c0000, 0x20000, 0x38652339 )

        ROM_LOAD_EVEN( "21.bin",       0x300000, 0x20000, 0x523f462a )
        ROM_LOAD_ODD ( "28.bin",       0x300000, 0x20000, 0xaf62bf07 )
        ROM_LOAD_EVEN( "22.bin",       0x340000, 0x20000, 0x52145369 )
        ROM_LOAD_ODD ( "29.bin",       0x340000, 0x20000, 0x6b41f82d )

        ROM_LOAD_EVEN( "07.bin",       0x380000, 0x20000, 0xe04af054 )
        ROM_LOAD_ODD ( "15.bin",       0x380000, 0x20000, 0xd36cdb91 )
        ROM_LOAD_EVEN( "08.bin",       0x3c0000, 0x20000, 0xb475d4e9 )
        ROM_LOAD_ODD ( "16.bin",       0x3c0000, 0x20000, 0x381608ae )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "23.bin",       0x000000, 0x08000, 0xb3b79d4f )
        ROM_CONTINUE(            0x010000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "30.bin",       0x00000, 0x20000, 0x7e5f6cb4 )
        ROM_LOAD ( "31.bin",       0x20000, 0x20000, 0x4a30c737 )
ROM_END


struct GameDriver dynwarsj_driver =
{
        __FILE__,
        0,
        "dynwarsj",
        "Dynsaty Wars (Japanese)",
        "1989",
        "Capcom",
        CPS1_CREDITS("Paul Leaman"),
        0,
        &dynwars_machine_driver,

        dynwarsj_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_dynwars,
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
        ROM_LOAD_EVEN( "mse_30.rom",   0x00000, 0x20000, 0x03fc8dbc )
        ROM_LOAD_ODD ( "mse_35.rom",   0x00000, 0x20000, 0xd5bf66cd )
        ROM_LOAD_EVEN( "mse_31.rom",   0x40000, 0x20000, 0x30332bcf )
        ROM_LOAD_ODD ( "mse_36.rom",   0x40000, 0x20000, 0x8f7d6ce9 )
        ROM_LOAD_WIDE_SWAP( "ms_32.rom",    0x80000, 0x80000, 0x2475ddfc )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "ms_gfx1.rom",  0x000000, 0x80000, 0x0d2bbe00 )
        ROM_LOAD( "ms_gfx5.rom",  0x080000, 0x80000, 0xc00fe7e2 )
        ROM_LOAD( "ms_gfx3.rom",  0x100000, 0x80000, 0x3a1a5bf4 )
        ROM_LOAD( "ms_gfx7.rom",  0x180000, 0x80000, 0x4ccacac5 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ms_9.rom",     0x00000, 0x10000, 0x57b29519 )
        ROM_RELOAD(           0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "ms_18.rom",    0x00000, 0x20000, 0xfb64e90d )
        ROM_LOAD( "ms_19.rom",    0x20000, 0x20000, 0x74f892b9 )
ROM_END

ROM_START( mswordj_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "msj_30.rom",   0x00000, 0x20000, 0x04f0ef50 )
        ROM_LOAD_ODD ( "msj_35.rom",   0x00000, 0x20000, 0x9fcbb9cd )
        ROM_LOAD_EVEN( "msj_31.rom",   0x40000, 0x20000, 0x6c060d70 )
        ROM_LOAD_ODD ( "msj_36.rom",   0x40000, 0x20000, 0xaec77787 )
        ROM_LOAD_WIDE_SWAP( "ms_32.rom",    0x80000, 0x80000, 0x2475ddfc )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "ms_gfx1.rom",  0x000000, 0x80000, 0x0d2bbe00 )
        ROM_LOAD( "ms_gfx5.rom",  0x080000, 0x80000, 0xc00fe7e2 )
        ROM_LOAD( "ms_gfx3.rom",  0x100000, 0x80000, 0x3a1a5bf4 )
        ROM_LOAD( "ms_gfx7.rom",  0x180000, 0x80000, 0x4ccacac5 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "ms_9.rom",     0x00000, 0x10000, 0x57b29519 )
        ROM_RELOAD(           0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "ms_18.rom",    0x00000, 0x20000, 0xfb64e90d )
        ROM_LOAD( "ms_19.rom",    0x20000, 0x20000, 0x74f892b9 )
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

                                                  MERCS

    There has to be one doesn't there. The graphics are all over the
    place.

********************************************************************/

INPUT_PORTS_START( input_ports_mercs )
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
        PORT_DIPNAME( 0xff, 0x00, "DIP A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0xff, 0x00, "DIP B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0xf7, 0xf7, "DIP C", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xf7, "On" )

        PORT_DIPNAME( 0x08, 0x08, "Screen Stop", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "Off" )
        PORT_DIPSETTING(    0x00, "On" )

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


CHAR_LAYOUT(charlayout_mercs,     4096-1024, 0x100000*8)
SPRITE_LAYOUT(spritelayout_mercs, 4*4096,   0x080000*8, 0x100000*8)
TILE_LAYOUT2(spritelayout2_mercs, 4096,     0x080000*8, 0x020000*8)

SPRITE_LAYOUT(tilelayout_mercs,   4096*2,    0x080000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_mercs, 1024,      0x080000*8, 0x100000*8)

static struct GfxDecodeInfo gfxdecodeinfo_mercs[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x000000, &charlayout_mercs,    32*16,                  32 },
        { 1, 0x060000, &spritelayout_mercs,  0,                      32 },
        { 1, 0x00c000, &tilelayout_mercs,    32*16+32*16,            32 },
        { 1, 0x03c000, &tilelayout32_mercs,  32*16+32*16+32*16,      32 },
        { 1, 0x200000, &spritelayout2_mercs, 0,                      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        mercs_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_mercs,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( mercs_rom )


        ROM_REGION(0x100000)      /* 68000 code */

        ROM_LOAD_EVEN( "so2_30e.rom",  0x00000, 0x20000, 0xea03645d )
        ROM_LOAD_ODD ( "so2_35e.rom",  0x00000, 0x20000, 0x09345484 )
        ROM_LOAD_EVEN( "so2_31e.rom",  0x40000, 0x20000, 0xb7d2fc06 )
        ROM_LOAD_ODD ( "so2_36e.rom",  0x40000, 0x20000, 0xd504b112 )
        ROM_LOAD_WIDE_SWAP( "so2_32.rom",   0x80000, 0x80000, 0xec9b7bc3 )

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "so2_gfx2.rom", 0x000000, 0x80000, 0x744239f8 )
        ROM_LOAD( "so2_gfx6.rom", 0x080000, 0x80000, 0xf8aa4dd2 )
        ROM_LOAD( "so2_gfx4.rom", 0x100000, 0x80000, 0x5a44f5e4 )
        ROM_LOAD( "so2_gfx8.rom", 0x180000, 0x80000, 0xcf205c1e )

        ROM_LOAD( "so2_20.rom",   0x200000, 0x20000, 0x27a13e35 )
        ROM_LOAD( "so2_10.rom",   0x220000, 0x20000, 0x9e7e0d9a )
        ROM_LOAD( "so2_22.rom",   0x240000, 0x20000, 0xd0afa7ab )
        ROM_LOAD( "so2_12.rom",   0x260000, 0x20000, 0x7a0d5f5d )
        ROM_LOAD( "so2_24.rom",   0x280000, 0x20000, 0x9b6d03ab )
        ROM_LOAD( "so2_14.rom",   0x2a0000, 0x20000, 0x0fedfdc3 )
        ROM_LOAD( "so2_26.rom",   0x2c0000, 0x20000, 0xa3cd9f0d )
        ROM_LOAD( "so2_16.rom",   0x2e0000, 0x20000, 0xadc6ff22 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "so2_09.rom",   0x00000, 0x10000, 0xa7999e1d )
        ROM_RELOAD(             0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "so2_18.rom",   0x00000, 0x20000, 0xbab28eee )
        ROM_LOAD( "so2_19.rom",   0x20000, 0x20000, 0x40e3af89 )
ROM_END

struct GameDriver mercs_driver =
{
        __FILE__,
        0,
        "mercs",
        "Mercs",
        "1990",
        "Capcom",
        CPS1_CREDITS("Paul Leaman\n"),
        GAME_NOT_WORKING,
        &mercs_machine_driver,

        mercs_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_mercs,
        NULL, 0, 0,

        ORIENTATION_ROTATE_270,
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
        ROM_LOAD_EVEN( "pnij36.bin",   0x00000, 0x20000, 0x2d4ffb2b )
        ROM_LOAD_ODD( "pnij42.bin",   0x00000, 0x20000, 0xc085dfaf )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "pnij01.bin",   0x000000, 0x20000, 0x01a0f311 )
        ROM_LOAD( "pnij02.bin",   0x020000, 0x20000, 0x0e21fc33 )
        ROM_LOAD( "pnij18.bin",   0x040000, 0x20000, 0xf17a0e56 )
        ROM_LOAD( "pnij19.bin",   0x060000, 0x20000, 0xaf08b230 )

        ROM_LOAD( "pnij09.bin",   0x080000, 0x20000, 0x48177b0a )
        ROM_LOAD( "pnij10.bin",   0x0a0000, 0x20000, 0xc2acc171 )
        ROM_LOAD( "pnij26.bin",   0x0c0000, 0x20000, 0xe2af981e )
        ROM_LOAD( "pnij27.bin",   0x0e0000, 0x20000, 0x83d5cb0e )

        ROM_LOAD( "pnij05.bin",   0x100000, 0x20000, 0x8c515dc0 )
        ROM_LOAD( "pnij06.bin",   0x120000, 0x20000, 0x79f4bfe3 )
        ROM_LOAD( "pnij32.bin",   0x140000, 0x20000, 0x84560bef )
        ROM_LOAD( "pnij33.bin",   0x160000, 0x20000, 0x3ed2c680 )

        ROM_LOAD( "pnij13.bin",   0x180000, 0x20000, 0x406451b0 )
        ROM_LOAD( "pnij14.bin",   0x1a0000, 0x20000, 0x7fe59b19 )
        ROM_LOAD( "pnij38.bin",   0x1c0000, 0x20000, 0xeb75bd8c )
        ROM_LOAD( "pnij39.bin",   0x1e0000, 0x20000, 0x70fbe579 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "pnij17.bin",   0x00000, 0x10000, 0xe86f787a )
        ROM_LOAD( "pnij17.bin",   0x08000, 0x10000, 0xe86f787a )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "pnij24.bin",   0x00000, 0x20000, 0x5092257d )
        ROM_LOAD ( "pnij25.bin",   0x20000, 0x20000, 0x22109aaa )
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
        ROM_LOAD_WIDE_SWAP( "kr_23e.rom",   0x00000, 0x80000, 0x1b3997eb )
        ROM_LOAD_WIDE_SWAP( "kr_22.rom",    0x80000, 0x80000, 0xd0b671a9 )

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "kr_gfx2.rom",  0x000000, 0x80000, 0xf095be2d )
        ROM_LOAD( "kr_gfx6.rom",  0x080000, 0x80000, 0x0200bc3d )
        ROM_LOAD( "kr_gfx1.rom",  0x100000, 0x80000, 0x9e36c1a4 )
        ROM_LOAD( "kr_gfx5.rom",  0x180000, 0x80000, 0x1f4298d2 )
        ROM_LOAD( "kr_gfx4.rom",  0x200000, 0x80000, 0x179dfd96 )
        ROM_LOAD( "kr_gfx8.rom",  0x280000, 0x80000, 0x0bb2b4e7 )
        ROM_LOAD( "kr_gfx3.rom",  0x300000, 0x80000, 0xc5832cae )
        ROM_LOAD( "kr_gfx7.rom",  0x380000, 0x80000, 0x37fa8751 )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "kr_09.rom",    0x00000, 0x10000, 0x5e44d9ee )
        ROM_RELOAD(            0x08000, 0x10000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "kr_18.rom",    0x00000, 0x20000, 0xda69d15f )
        ROM_LOAD ( "kr_19.rom",    0x20000, 0x20000, 0xbfc654e9 )
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
        ROM_LOAD_EVEN( "ghl29.bin",    0x00000, 0x20000, 0x166a58a2 ) /* 68000 code */
        ROM_LOAD_ODD ( "ghl30.bin",    0x00000, 0x20000, 0x7ac8407a ) /* 68000 code */
        ROM_LOAD_EVEN( "ghl27.bin",    0x40000, 0x20000, 0xf734b2be ) /* 68000 code */
        ROM_LOAD_ODD ( "ghl28.bin",    0x40000, 0x20000, 0x03d3e714 ) /* 68000 code */
        ROM_LOAD_WIDE( "ghl17.bin",    0x80000, 0x80000, 0x3ea1b0f2 ) /* Tile map */

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "ghl6.bin",     0x000000, 0x80000, 0x4ba90b59 ) /* Sprites / tiles */
        ROM_LOAD( "ghl5.bin",     0x080000, 0x80000, 0x0ba9c0b0 )
        ROM_LOAD( "ghl8.bin",     0x100000, 0x80000, 0x4bdee9de )
        ROM_LOAD( "ghl7.bin",     0x180000, 0x80000, 0x5d760ab9 )

        ROM_LOAD( "ghl10.bin",    0x200000, 0x10000, 0xbcc0f28c ) /* Sprite set 2 */
        ROM_LOAD( "ghl23.bin",    0x210000, 0x10000, 0x8426144b )
        ROM_LOAD( "ghl14.bin",    0x220000, 0x10000, 0x20f85c03 )
        ROM_LOAD( "ghl19.bin",    0x230000, 0x10000, 0x2a40166a )
        ROM_LOAD( "ghl12.bin",    0x240000, 0x10000, 0xda088d61 )
        ROM_LOAD( "ghl25.bin",    0x250000, 0x10000, 0x29f79c78 )
        ROM_LOAD( "ghl16.bin",    0x260000, 0x10000, 0xf187ba1c )
        ROM_LOAD( "ghl21.bin",    0x270000, 0x10000, 0x17e11df0 )

        /* 32 * 32 tiles - left half */
        ROM_LOAD( "ghl18.bin",    0x280000, 0x10000, 0xd34e271a ) /* Plane x */
        ROM_LOAD( "ghl09.bin",    0x290000, 0x10000, 0xae24bb19 ) /* Plane x */
        ROM_LOAD( "ghl22.bin",    0x2a0000, 0x10000, 0x7e69e2e6 ) /* Plane x */
        ROM_LOAD( "ghl13.bin",    0x2b0000, 0x10000, 0x3f70dd37 ) /* Plane x */
        /* 32 * 32 tiles - right half */
        ROM_LOAD( "ghl20.bin",    0x2c0000, 0x10000, 0x2f1345b4 ) /* Plane x */
        ROM_LOAD( "ghl11.bin",    0x2d0000, 0x10000, 0x37c9b6c6 ) /* Plane x */
        ROM_LOAD( "ghl24.bin",    0x2e0000, 0x10000, 0x889aac05 ) /* Plane x */
        ROM_LOAD( "ghl15.bin",    0x2f0000, 0x10000, 0x3c2a212a ) /* Plane x */

        ROM_REGION(0x18000) /* 64k for the audio CPU */
        ROM_LOAD( "ghl26.bin",    0x000000, 0x08000, 0x3692f6e5 )
        ROM_CONTINUE(          0x010000, 0x08000 )
ROM_END

ROM_START( ghoulsj_rom )
        ROM_REGION(0x100000)
        ROM_LOAD_EVEN( "ghlj29.bin",   0x00000, 0x20000, 0x82fd1798 ) /* 68000 code */
        ROM_LOAD_ODD ( "ghlj30.bin",   0x00000, 0x20000, 0x35366ccc ) /* 68000 code */
        ROM_LOAD_EVEN( "ghlj27.bin",   0x40000, 0x20000, 0xa17c170a ) /* 68000 code */
        ROM_LOAD_ODD ( "ghlj28.bin",   0x40000, 0x20000, 0x6af0b391 ) /* 68000 code */
        ROM_LOAD_WIDE( "ghl17.bin",    0x80000, 0x80000, 0x3ea1b0f2 ) /* Tile map */

        ROM_REGION(0x300000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "ghl6.bin",     0x000000, 0x80000, 0x4ba90b59 ) /* Sprites / tiles */
        ROM_LOAD( "ghl5.bin",     0x080000, 0x80000, 0x0ba9c0b0 )
        ROM_LOAD( "ghl8.bin",     0x100000, 0x80000, 0x4bdee9de )
        ROM_LOAD( "ghl7.bin",     0x180000, 0x80000, 0x5d760ab9 )
        ROM_LOAD( "ghl10.bin",    0x200000, 0x10000, 0xbcc0f28c ) /* Sprite set 2 */
        ROM_LOAD( "ghl23.bin",    0x210000, 0x10000, 0x8426144b )
        ROM_LOAD( "ghl14.bin",    0x220000, 0x10000, 0x20f85c03 )
        ROM_LOAD( "ghl19.bin",    0x230000, 0x10000, 0x2a40166a )
        ROM_LOAD( "ghl12.bin",    0x240000, 0x10000, 0xda088d61 )
        ROM_LOAD( "ghl25.bin",    0x250000, 0x10000, 0x29f79c78 )
        ROM_LOAD( "ghl16.bin",    0x260000, 0x10000, 0xf187ba1c )
        ROM_LOAD( "ghl21.bin",    0x270000, 0x10000, 0x17e11df0 )
        /* 32 * 32 tiles - left half */
        ROM_LOAD( "ghl18.bin",    0x280000, 0x10000, 0xd34e271a ) /* Plane x */
        ROM_LOAD( "ghl09.bin",    0x290000, 0x10000, 0xae24bb19 ) /* Plane x */
        ROM_LOAD( "ghl22.bin",    0x2a0000, 0x10000, 0x7e69e2e6 ) /* Plane x */
        ROM_LOAD( "ghl13.bin",    0x2b0000, 0x10000, 0x3f70dd37 ) /* Plane x */
        /* 32 * 32 tiles - right half */
        ROM_LOAD( "ghl20.bin",    0x2c0000, 0x10000, 0x2f1345b4 ) /* Plane x */
        ROM_LOAD( "ghl11.bin",    0x2d0000, 0x10000, 0x37c9b6c6 ) /* Plane x */
        ROM_LOAD( "ghl24.bin",    0x2e0000, 0x10000, 0x889aac05 ) /* Plane x */
        ROM_LOAD( "ghl15.bin",    0x2f0000, 0x10000, 0x3c2a212a ) /* Plane x */

        ROM_REGION(0x18000) /* 64k for the audio CPU */
        ROM_LOAD( "ghl26.bin",    0x000000, 0x08000, 0x3692f6e5 )
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
SPRITE_LAYOUT2(spritelayout_cawingj, 0x4000)
SPRITE_LAYOUT2(tilelayout_cawingj,   0x2400)
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

        ROM_LOAD_EVEN( "caj36a.bin",   0x00000, 0x20000, 0x91fceacd )
        ROM_LOAD_ODD ( "caj42a.bin",   0x00000, 0x20000, 0x039f8362 )
        ROM_LOAD_EVEN( "caj37a.bin",   0x40000, 0x20000, 0xe5b75caf )
        ROM_LOAD_ODD ( "caj43a.bin",   0x40000, 0x20000, 0xc73fd713 )

        ROM_LOAD_EVEN( "caj34.bin",    0x80000, 0x20000, 0x51ea57f4 )
        ROM_LOAD_ODD ( "caj40.bin",    0x80000, 0x20000, 0x2ab71ae1 )
        ROM_LOAD_EVEN( "caj35.bin",    0xc0000, 0x20000, 0x01d71973 )
        ROM_LOAD_ODD ( "caj41.bin",    0xc0000, 0x20000, 0x3a43b538 )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "caj09.bin",    0x000000, 0x20000, 0x41b0f9a6 ) /* Plane 2 */
        ROM_LOAD( "caj10.bin",    0x020000, 0x20000, 0xbf8a5f52 )
        ROM_LOAD( "caj24.bin",    0x040000, 0x20000, 0xe356aad7 )
        ROM_LOAD( "caj25.bin",    0x060000, 0x20000, 0xcdd0204d )

        ROM_LOAD( "caj01.bin",    0x080000, 0x20000, 0x1002d0b8 )  /* Plane 1 */
        ROM_LOAD( "caj02.bin",    0x0a0000, 0x20000, 0x125b018d )
        ROM_LOAD( "caj17.bin",    0x0c0000, 0x20000, 0x540f2fd8 )
        ROM_LOAD( "caj18.bin",    0x0e0000, 0x20000, 0x29c1d4b1 )

        ROM_LOAD( "caj13.bin",    0x100000, 0x20000, 0x6f3948b2 ) /* Plane 4 */
        ROM_LOAD( "caj14.bin",    0x120000, 0x20000, 0x8458e7d7 )
        ROM_LOAD( "caj38.bin",    0x140000, 0x20000, 0x2464d4ab )
        ROM_LOAD( "caj39.bin",    0x160000, 0x20000, 0xeea23b67 )

        ROM_LOAD( "caj05.bin",    0x180000, 0x20000, 0x207373d7 ) /* Plane 3 */
        ROM_LOAD( "caj06.bin",    0x1a0000, 0x20000, 0xcf80e164 )
        ROM_LOAD( "caj32.bin",    0x1c0000, 0x20000, 0x9b5836b3 )
        ROM_LOAD( "caj33.bin",    0x1e0000, 0x20000, 0xdde3891f )


        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "caj-s.bin",    0x00000, 0x08000, 0x96fe7485 )
        ROM_CONTINUE(             0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "caj30.bin",    0x00000, 0x20000, 0x4a613a2c )
        ROM_LOAD ( "caj31.bin",    0x20000, 0x20000, 0x74584493 )
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

                                                Street Fighter 2

********************************************************************/

INPUT_PORTS_START( input_ports_sf2 )
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
        PORT_DIPNAME( 0xff, 0x00, "DIP A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0xff, 0x00, "DIP B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0xff, 0x00, "DIP C", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

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

CHAR_LAYOUT(charlayout_sf2, 4096, 0x180000*8)
SPRITE_LAYOUT(spritelayout_sf2, 4096*10-2048, 0x180000*8, 0x300000*8)
TILE32_LAYOUT(tile32layout_sf2, 1024,    0x180000*8, 0x300000*8)
SPRITE_LAYOUT(tilelayout_sf2, 4096*2-2048,     0x180000*8, 0x300000*8 )


static struct GfxDecodeInfo gfxdecodeinfo_sf2[] =
{
        /*   start    pointer                 start   number of colours */
                { 1, 0x140000, &charlayout_sf2,       32*16,                  32 },
                { 1, 0x000000, &spritelayout_sf2,     0,                      32 },
                { 1, 0x150000, &tilelayout_sf2,       32*16+32*16,            32 },
                { 1, 0x120000, &tile32layout_sf2,     32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        sf2_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_sf2,
        12000000)              /* 12 MHz */

ROM_START( sf2_rom )
        ROM_REGION(0x180000)      /* 68000 code */
                ROM_LOAD_WIDE_SWAP( "sf2.23",       0x000000, 0x80000, 0x7358cad0 )
                ROM_LOAD_WIDE_SWAP( "sf2.22",       0x080000, 0x80000, 0x4bd83faa )
                ROM_LOAD_WIDE_SWAP( "sf2.21",       0x100000, 0x80000, 0x30994e2b )

                ROM_REGION(0x600000)     /* temporary space for graphics (disposed after conversion) */
                /* Plane 1+2 */
                ROM_LOAD( "sf2.02",       0x000000, 0x80000, 0x7164324c ) /* sprites (left half)*/
                ROM_LOAD( "sf2.06",       0x080000, 0x80000, 0x4f235543 ) /* sprites */
                ROM_LOAD( "sf2.11",       0x100000, 0x80000, 0x9b5e7e52 ) /* sprites */

                ROM_LOAD( "sf2.01",       0x180000, 0x80000, 0x8af37b19 ) /* sprites (right half) */
                ROM_LOAD( "sf2.05",       0x200000, 0x80000, 0x7b0473b2 ) /* sprites */
                ROM_LOAD( "sf2.10",       0x280000, 0x80000, 0xfa8bb429 ) /* sprites */

                /* Plane 3+4 */
                ROM_LOAD( "sf2.04",       0x300000, 0x80000, 0xd68e120c ) /* sprites */
                ROM_LOAD( "sf2.08",       0x380000, 0x80000, 0x3f23bdbb ) /* sprites */
                ROM_LOAD( "sf2.13",       0x400000, 0x80000, 0x68140ca8 ) /* sprites */

                ROM_LOAD( "sf2.03",       0x480000, 0x80000, 0x7d490a7f ) /* sprites */
                ROM_LOAD( "sf2.07",       0x500000, 0x80000, 0xdb013e65 ) /* sprites */
                ROM_LOAD( "sf2.12",       0x580000, 0x80000, 0x56e705ef ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
                ROM_LOAD( "sf2.09",       0x00000, 0x08000, 0x5fe33a55 )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
                ROM_LOAD ( "sf2.18",       0x00000, 0x20000, 0xe615bdb5 )
                ROM_LOAD ( "sf2.19",       0x20000, 0x20000, 0x006f92e1 )
ROM_END


struct GameDriver sf2_driver =
{
        __FILE__,
        0,
        "sf2",
        "Street Fighter 2",
        "????",
        "Capcom",
        CPS1_CREDITS("Paul Leaman"),
        GAME_NOT_WORKING,
        &sf2_machine_driver,

        sf2_rom,
        0,0,0,0,
        input_ports_sf2,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};


ROM_START( sf2ce_rom )
        ROM_REGION(0x180000)      /* 68000 code */
                ROM_LOAD_WIDE_SWAP( "sf2ce.23",     0x000000, 0x80000, 0xa591242b )
                ROM_LOAD_WIDE_SWAP( "sf2ce.22",     0x080000, 0x80000, 0xc4273edd )
                ROM_LOAD_WIDE_SWAP( "sf2ce.21",     0x100000, 0x80000, 0x6ae07592 )

                ROM_REGION(0x600000)     /* temporary space for graphics (disposed after conversion) */
                /* Plane 1+2 */
                ROM_LOAD( "sf2.02",       0x000000, 0x80000, 0x7164324c ) /* sprites (left half)*/
                ROM_LOAD( "sf2.06",       0x080000, 0x80000, 0x4f235543 ) /* sprites */
                ROM_LOAD( "sf2.11",       0x100000, 0x80000, 0x9b5e7e52 ) /* sprites */

                ROM_LOAD( "sf2.01",       0x180000, 0x80000, 0x8af37b19 ) /* sprites (right half) */
                ROM_LOAD( "sf2.05",       0x200000, 0x80000, 0x7b0473b2 ) /* sprites */
                ROM_LOAD( "sf2.10",       0x280000, 0x80000, 0xfa8bb429 ) /* sprites */

                /* Plane 3+4 */
                ROM_LOAD( "sf2.04",       0x300000, 0x80000, 0xd68e120c ) /* sprites */
                ROM_LOAD( "sf2.08",       0x380000, 0x80000, 0x3f23bdbb ) /* sprites */
                ROM_LOAD( "sf2.13",       0x400000, 0x80000, 0x68140ca8 ) /* sprites */

                ROM_LOAD( "sf2.03",       0x480000, 0x80000, 0x7d490a7f ) /* sprites */
                ROM_LOAD( "sf2.07",       0x500000, 0x80000, 0xdb013e65 ) /* sprites */
                ROM_LOAD( "sf2.12",       0x580000, 0x80000, 0x56e705ef ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
                ROM_LOAD( "sf2.09",       0x00000, 0x08000, 0x5fe33a55 )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
                ROM_LOAD ( "sf2.18",       0x00000, 0x20000, 0xe615bdb5 )
                ROM_LOAD ( "sf2.19",       0x20000, 0x20000, 0x006f92e1 )
ROM_END

struct GameDriver sf2ce_driver =
{
        __FILE__,
                &sf2_driver,
                "sf2ce",
                "Street Fighter 2 (Championship Edition)",
        "????",
                "Capcom",
        CPS1_CREDITS("Paul Leaman"),
                GAME_NOT_WORKING,
        &sf2_machine_driver,

                sf2ce_rom,
        0,0,0,0,
                input_ports_sf2,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};

ROM_START( sf2cej_rom )
        ROM_REGION(0x180000)      /* 68000 code */
                ROM_LOAD_WIDE_SWAP( "sf2cej.23",    0x000000, 0x80000, 0x2ed68002 )
                ROM_LOAD_WIDE_SWAP( "sf2cej.22",    0x080000, 0x80000, 0x349ceb0a )
                ROM_LOAD_WIDE_SWAP( "sf2cej.21",    0x100000, 0x80000, 0xe3411225 )

                ROM_REGION(0x600000)     /* temporary space for graphics (disposed after conversion) */
                /* Plane 1+2 */
                ROM_LOAD( "sf2.02",       0x000000, 0x80000, 0x7164324c ) /* sprites (left half)*/
                ROM_LOAD( "sf2.06",       0x080000, 0x80000, 0x4f235543 ) /* sprites */
                ROM_LOAD( "sf2.11",       0x100000, 0x80000, 0x9b5e7e52 ) /* sprites */

                ROM_LOAD( "sf2.01",       0x180000, 0x80000, 0x8af37b19 ) /* sprites (right half) */
                ROM_LOAD( "sf2.05",       0x200000, 0x80000, 0x7b0473b2 ) /* sprites */
                ROM_LOAD( "sf2.10",       0x280000, 0x80000, 0xfa8bb429 ) /* sprites */

                /* Plane 3+4 */
                ROM_LOAD( "sf2.04",       0x300000, 0x80000, 0xd68e120c ) /* sprites */
                ROM_LOAD( "sf2.08",       0x380000, 0x80000, 0x3f23bdbb ) /* sprites */
                ROM_LOAD( "sf2.13",       0x400000, 0x80000, 0x68140ca8 ) /* sprites */

                ROM_LOAD( "sf2.03",       0x480000, 0x80000, 0x7d490a7f ) /* sprites */
                ROM_LOAD( "sf2.07",       0x500000, 0x80000, 0xdb013e65 ) /* sprites */
                ROM_LOAD( "sf2.12",       0x580000, 0x80000, 0x56e705ef ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
                ROM_LOAD( "sf2.09",       0x00000, 0x08000, 0x5fe33a55 )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
                ROM_LOAD ( "sf2.18",       0x00000, 0x20000, 0xe615bdb5 )
                ROM_LOAD ( "sf2.19",       0x20000, 0x20000, 0x006f92e1 )
ROM_END

struct GameDriver sf2cej_driver =
{
        __FILE__,
                &sf2_driver,
                "sf2cej",
                "Street Fighter 2 (Japanese Championship Edition)",
        "????",
                "Capcom",
        CPS1_CREDITS("Paul Leaman"),
                GAME_NOT_WORKING,
        &sf2_machine_driver,

                sf2cej_rom,
        0,0,0,0,
                input_ports_sf2,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};

/********************************************************************

                          Rockman

********************************************************************/

INPUT_PORTS_START( input_ports_rockman )
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
        PORT_DIPNAME( 0x1f, 0x1f, "Coinage", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0f, "9 Coins/1 Credit" )
        PORT_DIPSETTING(    0x10, "8 Coins/1 Credit" )
        PORT_DIPSETTING(    0x11, "7 Coins/1 Credit" )
        PORT_DIPSETTING(    0x12, "6 Coins/1 Credit" )
        PORT_DIPSETTING(    0x13, "5 Coins/1 Credit" )
        PORT_DIPSETTING(    0x14, "4 Coins/1 Credit" )
        PORT_DIPSETTING(    0x15, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x16, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x0e, "2 Coins/1 Credit - 1 to continue (if on)" )
        PORT_DIPSETTING(    0x1f, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x1e, "1 Coin/2 Credits" )
        PORT_DIPSETTING(    0x1d, "1 Coin/3 Credits" )
        PORT_DIPSETTING(    0x1c, "1 Coin/4 Credits" )
        PORT_DIPSETTING(    0x1b, "1 Coin/5 Credits" )
        PORT_DIPSETTING(    0x1a, "1 Coin/6 Credits" )
        PORT_DIPSETTING(    0x19, "1 Coin/7 Credits" )
        PORT_DIPSETTING(    0x18, "1 Coin/8 Credits" )
        PORT_DIPSETTING(    0x17, "1 Coin/9 Credits" )
        PORT_DIPSETTING(    0x0d, "Free Play" )
        /* 0x00 to 0x0c 1 Coin/1 Credit */
        PORT_DIPNAME( 0x60, 0x20, "2 Player Game", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "1 Credit" )
        PORT_DIPSETTING(    0x40, "2 Credits" )
        PORT_DIPSETTING(    0x60, "2 Credits - pl1 may play on right" )
       /* Unused 0x00 */
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "On" )
        PORT_DIPSETTING(    0x00, "Off" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "Easy" )
        PORT_DIPSETTING(    0x02, "Normal" )
        PORT_DIPSETTING(    0x01, "Difficult" )
        PORT_DIPSETTING(    0x00, "Hard" )
        PORT_DIPNAME( 0x0c, 0x0c, "Time", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "100" )
        PORT_DIPSETTING(    0x08, "90" )
        PORT_DIPSETTING(    0x04, "70" )
        PORT_DIPSETTING(    0x00, "60" )
        PORT_DIPNAME( 0xf0, 0xf0, "Unknown DSW B", IP_KEY_NONE )
        PORT_DIPSETTING(    0xf0, "On" )
        PORT_DIPSETTING(    0x00, "Off" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x02, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x04, 0x04, "Continue", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x10, 0x10, "Flip Screen", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On")
        PORT_DIPSETTING(    0x10, "Off" )
        PORT_DIPNAME( 0x20, 0x20, "Demo sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off")
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, "Test Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

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

CHAR_LAYOUT(charlayout_rockman, 4096+2048, 0x400000*8)
SPRITE_LAYOUT(spritelayout_rockman, 4096*10, 0x200000*8, 0x400000*8)
SPRITE_LAYOUT(tilelayout_rockman, 4096*2,   0x200000*8, 0x400000*8 )
TILE32_LAYOUT(tile32layout_rockman, 2048+1024,    0x200000*8, 0x400000*8)

static struct GfxDecodeInfo gfxdecodeinfo_rockman[] =
{
        /*   start    pointer                 start   number of colours */
        { 1, 0x000000, &charlayout_rockman,       32*16,                  32 },
        { 1, 0x018000, &spritelayout_rockman,     0,                      32 },
        { 1, 0x160000, &tilelayout_rockman,       32*16+32*16,            32 },
        { 1, 0x1a0000, &tile32layout_rockman,     32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};


MACHINE_DRIVER(
        rockman_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_rockman,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( rockmanj_rom )
        ROM_REGION(0x180000)      /* 68000 code */

        ROM_LOAD_WIDE_SWAP( "rcm23a.bin",   0x00000, 0x80000, 0xefd96cb2 )
        ROM_LOAD_WIDE_SWAP( "rcm22a.bin",   0x80000, 0x80000, 0x8729a689 )
        ROM_LOAD_WIDE_SWAP( "rcm21a.bin",   0x100000, 0x80000, 0x517ccde2 )

        ROM_REGION(0x800000)     /* temporary space for graphics (disposed after conversion) */

        /* Plane 1+2 */
        ROM_LOAD( "rcm03.bin",    0x000000, 0x80000, 0x36f3073c ) /* sprites (left half)*/
        ROM_LOAD( "rcm07.bin",    0x080000, 0x80000, 0x826de013 ) /* sprites */
        ROM_LOAD( "rcm12.bin",    0x100000, 0x80000, 0xfed5f203 ) /* sprites */
        ROM_LOAD( "rcm16.bin",    0x180000, 0x80000, 0x93d97fde ) /* sprites */

        ROM_LOAD( "rcm01.bin",    0x200000, 0x80000, 0x6ecdf13f ) /* sprites (right half) */
        ROM_LOAD( "rcm05.bin",    0x280000, 0x80000, 0x5dd131fd ) /* sprites */
        ROM_LOAD( "rcm10.bin",    0x300000, 0x80000, 0x4dc8ada9 ) /* sprites */
        ROM_LOAD( "rcm14.bin",    0x380000, 0x80000, 0x303be3bd ) /* sprites */

        /* Plane 3+4 */
        ROM_LOAD( "rcm04.bin",    0x400000, 0x80000, 0x54e622ff ) /* sprites */
        ROM_LOAD( "rcm08.bin",    0x480000, 0x80000, 0xfbff64cf ) /* sprites */
        ROM_LOAD( "rcm13.bin",    0x500000, 0x80000, 0x5069d4a9 ) /* sprites */
        ROM_LOAD( "rcm17.bin",    0x580000, 0x80000, 0x92371042 ) /* sprites */

        ROM_LOAD( "rcm02.bin",    0x600000, 0x80000, 0x944d4f0f ) /* sprites */
        ROM_LOAD( "rcm06.bin",    0x680000, 0x80000, 0xf0faf813 ) /* sprites */
        ROM_LOAD( "rcm11.bin",    0x700000, 0x80000, 0xf2b9ee06 ) /* sprites */
        ROM_LOAD( "rcm15.bin",    0x780000, 0x80000, 0x4f2d372f ) /* sprites */

        ROM_REGION(0x28000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "rcm09.bin",    0x000000, 0x08000, 0x9632d6ef )
        ROM_CONTINUE(          0x010000, 0x18000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "rcm18.bin",    0x00000, 0x20000, 0x80f1f8aa )
        ROM_LOAD( "rcm19.bin",    0x20000, 0x20000, 0xf257dbe1 )
ROM_END

struct GameDriver rockmanj_driver =
{
	__FILE__,
	0,
	"rockmanj",
	"Rockman - The Power Battle (Japan)",
	"1995",
	"Capcom",
	CPS1_CREDITS("Paul Leaman\nT.J. Grant\nMarco Cassili (dip switches)"),
	0,
	&rockman_machine_driver,

	rockmanj_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports_rockman,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	NULL, NULL
};



/********************************************************************

                          Captain Commando

********************************************************************/

INPUT_PORTS_START( input_ports_captcomm )
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
        PORT_DIPNAME( 0x07, 0x07, "Difficulty 1", IP_KEY_NONE )
        PORT_DIPSETTING(    0x07, "Very Easy" )
        PORT_DIPSETTING(    0x06, "Easy 1" )
        PORT_DIPSETTING(    0x05, "Easy 2" )
        PORT_DIPSETTING(    0x04, "Normal" )
        PORT_DIPSETTING(    0x03, "Difficult" )
        PORT_DIPSETTING(    0x02, "Very Difficult" )
        PORT_DIPSETTING(    0x01, "Hard" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x18, 0x18, "Difficulty 2", IP_KEY_NONE )
        PORT_DIPSETTING(    0x18, "1" )
        PORT_DIPSETTING(    0x10, "2" )
        PORT_DIPSETTING(    0x08, "3" )
        PORT_DIPSETTING(    0x00, "4" )
        PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPSETTING(    0x20, "Off" )
        PORT_DIPNAME( 0xc0, 0xc0, "Players allowed", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "1" )
        PORT_DIPSETTING(    0xc0, "2" )
        PORT_DIPSETTING(    0x80, "3" )
        PORT_DIPSETTING(    0x00, "4" )

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

CHAR_LAYOUT(charlayout_captcomm, 2*4096, 0x200000*8)
SPRITE_LAYOUT(spritelayout_captcomm, 4096*5,  0x100000*8, 0x200000*8)
TILE32_LAYOUT(tile32layout_captcomm, 1024,    0x100000*8, 0x200000*8)
SPRITE_LAYOUT(tilelayout_captcomm, 4096*3,    0x100000*8, 0x200000*8 )

static struct GfxDecodeInfo gfxdecodeinfo_captcomm[] =
{
        /*   start    pointer                 start   number of colours */
        { 1, 0x0c0000, &charlayout_captcomm,       32*16,                  32 },
        { 1, 0x000000, &spritelayout_captcomm,     0,                      32 },
        { 1, 0x0d0000, &tilelayout_captcomm,       32*16+32*16,            32 },
        { 1, 0x0a0000, &tile32layout_captcomm,     32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        captcomm_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_captcomm,
        CPS1_DEFAULT_CPU_SPEED)              /* 12 MHz */

ROM_START( captcomm_rom )
        ROM_REGION(0x140000)      /* 68000 code */
        ROM_LOAD_WIDE_SWAP( "cce_23d.rom",  0x000000, 0x80000, 0x19c58ece )
        ROM_LOAD_WIDE_SWAP( "cc_22d.rom",   0x080000, 0x80000, 0xa91949b7 )
        ROM_LOAD_EVEN( "cc_24d.rom",   0x100000, 0x20000, 0x680e543f )
        ROM_LOAD_ODD ( "cc_28d.rom",   0x100000, 0x20000, 0x8820039f )

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
                /* Plane 1+2 */
        ROM_LOAD( "gfx_02.rom",   0x000000, 0x80000, 0x00637302 ) /* sprites (left half)*/
        ROM_LOAD( "gfx_06.rom",   0x080000, 0x80000, 0x0c69f151 ) /* sprites */

        ROM_LOAD( "gfx_01.rom",   0x100000, 0x80000, 0x7261d8ba ) /* sprites (right half) */
        ROM_LOAD( "gfx_05.rom",   0x180000, 0x80000, 0x28718bed ) /* sprites */

                /* Plane 3+4 */
        ROM_LOAD( "gfx_04.rom",   0x200000, 0x80000, 0xcc87cf61 ) /* sprites */
        ROM_LOAD( "gfx_08.rom",   0x280000, 0x80000, 0x1f9ebb97 ) /* sprites */

        ROM_LOAD( "gfx_03.rom",   0x300000, 0x80000, 0x6a60f949 ) /* sprites */
        ROM_LOAD( "gfx_07.rom",   0x380000, 0x80000, 0xd4acc53a ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "cc_09.rom",    0x00000, 0x08000, 0x698e8b58 )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "cc_18.rom",    0x00000, 0x20000, 0x6de2c2db )
        ROM_LOAD ( "cc_19.rom",    0x20000, 0x20000, 0xb99091ae )
ROM_END

struct GameDriver captcomm_driver =
{
	__FILE__,
	0,
	"captcomm",
	"Captain Commando (World)",
	"1991",
	"Capcom",
	CPS1_CREDITS("Paul Leaman\nMarco Cassili (dip switches)"),
	0,
	&captcomm_machine_driver,

	captcomm_rom,
	0,0,0,0,
	input_ports_captcomm,
	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	NULL, NULL
};


/********************************************************************

                          3 Wonders

********************************************************************/

INPUT_PORTS_START( input_ports_3wonders )
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
        PORT_DIPNAME( 0xff, 0x00, "DIP A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0xff, 0x00, "DIP B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0xff, 0x00, "DIP C", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

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

CHAR_LAYOUT(charlayout_3wonders, 4096*2-2048, 0x200000*8)
SPRITE_LAYOUT(spritelayout_3wonders, 4096*4, 0x100000*8, 0x200000*8)
TILE32_LAYOUT(tile32layout_3wonders, 0x0700,  0x100000*8, 0x200000*8)
SPRITE_LAYOUT(tilelayout_3wonders, 4096*3-2048,     0x100000*8, 0x200000*8 )


static struct GfxDecodeInfo gfxdecodeinfo_3wonders[] =
{
        /*   start    pointer                 start   number of colours */
                { 1, 0x054000, &charlayout_3wonders,       32*16,                  32 },
                { 1, 0x000000, &spritelayout_3wonders,     0,                      32 },
                { 1, 0x0C0000, &tilelayout_3wonders,       32*16+32*16,            32 },
        { 1, 0x070000, &tile32layout_3wonders,     32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        c3wonders_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_3wonders,
        CPS1_DEFAULT_CPU_SPEED)              /* 12 MHz */

ROM_START( c3wonders_rom )
        ROM_REGION(0x140000)      /* 68000 code */
        ROM_LOAD_EVEN( "3wonders.30",  0x00000, 0x20000, 0x324962b3 )
        ROM_LOAD_ODD ( "3wonders.35",  0x00000, 0x20000, 0x02d7fc83 )
        ROM_LOAD_EVEN( "3wonders.31",  0x40000, 0x20000, 0xc8eca128 )
        ROM_LOAD_ODD ( "3wonders.36",  0x40000, 0x20000, 0x614d343d )

        ROM_LOAD_EVEN( "3wonders.28",  0x80000, 0x20000, 0x153e0892 )
        ROM_LOAD_ODD ( "3wonders.33",  0x80000, 0x20000, 0xcf61bb0d )
        ROM_LOAD_EVEN( "3wonders.29",  0xc0000, 0x20000, 0xc3d9f797 )
        ROM_LOAD_ODD ( "3wonders.34",  0xc0000, 0x20000, 0xcec3c19b )

        ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
                /* Plane 1+2 */
        ROM_LOAD( "3wonders.01",  0x000000, 0x80000, 0xbe911819 ) /* sprites (right half) */
        ROM_LOAD( "3wonders.02",  0x080000, 0x80000, 0x74fe5718 ) /* sprites (left half)*/

        ROM_LOAD( "3wonders.05",  0x100000, 0x80000, 0x4d7ec81a ) /* sprites */
        ROM_LOAD( "3wonders.06",  0x180000, 0x80000, 0xb021039b ) /* sprites */

                /* Plane 3+4 */
        ROM_LOAD( "3wonders.03",  0x200000, 0x80000, 0x31a668e6 ) /* sprites */
        ROM_LOAD( "3wonders.04",  0x280000, 0x80000, 0x6a4ae75a ) /* sprites */

        ROM_LOAD( "3wonders.07",  0x300000, 0x80000, 0x2c6e4b52 ) /* sprites */
        ROM_LOAD( "3wonders.08",  0x380000, 0x80000, 0xd0b297c2 ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "3wonders.09",  0x00000, 0x08000, 0xad902b9c )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "3wonders.18",  0x00000, 0x20000, 0x4b87390b )
        ROM_LOAD ( "3wonders.19",  0x20000, 0x20000, 0x55902200 )
ROM_END

struct GameDriver c3wonders_driver =
{
        __FILE__,
        0,
        "3wonders",
        "3 Wonders",
        "????",
        "Capcom",
        CPS1_CREDITS("Paul Leaman"),
        GAME_NOT_WORKING,
        &c3wonders_machine_driver,

        c3wonders_rom,
        0,0,0,0,
        input_ports_3wonders,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};

/********************************************************************

                          King Of Dragons

********************************************************************/

INPUT_PORTS_START( input_ports_kod )
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
        PORT_DIPNAME( 0xff, 0x00, "DIP A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWB */
        PORT_DIPNAME( 0xff, 0x00, "DIP B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

        PORT_START      /* DSWC */
        PORT_DIPNAME( 0xff, 0x00, "DIP C", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0xff, "On" )

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

CHAR_LAYOUT(charlayout_kod, 4096*2-2048, 0x100000*8)
SPRITE_LAYOUT(spritelayout_kod, 4096*6, 0x100000*8, 0x200000*8)
TILE32_LAYOUT(tile32layout_kod, 4096,  0x100000*8, 0x200000*8)
SPRITE_LAYOUT(tilelayout_kod, 4096*2-2048,     0x100000*8, 0x200000*8 )


static struct GfxDecodeInfo gfxdecodeinfo_kod[] =
{
        /*   start    pointer                 start   number of colours */
        { 1, 0x0c0000, &charlayout_3wonders,       32*16,                  32 },
                { 1, 0x000000, &spritelayout_3wonders,     0,                      32 },
                { 1, 0x0C0000, &tilelayout_3wonders,       32*16+32*16,            32 },
                { 1, 0x070000, &tile32layout_3wonders,     32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        kod_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_kod,
        CPS1_DEFAULT_CPU_SPEED)              /* 12 MHz */

ROM_START( kod_rom )
        ROM_REGION(0x140000)      /* 68000 code */
        ROM_LOAD_EVEN( "kod30.rom",    0x00000, 0x20000, 0x324962b3 )
        ROM_LOAD_ODD ( "kod35.rom",    0x00000, 0x20000, 0x02d7fc83 )
        ROM_LOAD_EVEN( "kod31.rom",    0x40000, 0x20000, 0xc8eca128 )
        ROM_LOAD_ODD ( "kod36.rom",    0x40000, 0x20000, 0x614d343d )
        ROM_LOAD_EVEN( "kod28.rom",    0x80000, 0x20000, 0x153e0892 )
        ROM_LOAD_ODD ( "kod37.rom",    0x80000, 0x20000, 0xcf61bb0d )
        ROM_LOAD_EVEN( "kod29.rom",    0xc0000, 0x20000, 0xc3d9f797 )
        ROM_LOAD_ODD ( "kod38.rom",    0xc0000, 0x20000, 0xcec3c19b )

                ROM_REGION(0x400000)     /* temporary space for graphics (disposed after conversion) */
                /* Plane 1+2 */
        ROM_LOAD( "kod01.rom",    0x000000, 0x80000, 0xbe911819 ) /* sprites (right half) */
        ROM_LOAD( "kod02.rom",    0x080000, 0x80000, 0x74fe5718 ) /* sprites (left half)*/

        ROM_LOAD( "kod10.rom",    0x100000, 0x80000, 0x4d7ec81a ) /* sprites */
        ROM_LOAD( "kod11.rom",    0x180000, 0x80000, 0xb021039b ) /* sprites */

                /* Plane 3+4 */
        ROM_LOAD( "kod03.rom",    0x200000, 0x80000, 0x31a668e6 ) /* sprites */
        ROM_LOAD( "kod04.rom",    0x280000, 0x80000, 0x6a4ae75a ) /* sprites */

        ROM_LOAD( "kod12.rom",    0x300000, 0x80000, 0x2c6e4b52 ) /* sprites */
        ROM_LOAD( "kod13.rom",    0x380000, 0x80000, 0xd0b297c2 ) /* sprites */

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "kod09.rom",    0x00000, 0x08000, 0xad902b9c )
        ROM_CONTINUE(          0x10000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD ( "kod18.rom",    0x00000, 0x20000, 0x4b87390b )
        ROM_LOAD ( "kod19.rom",    0x20000, 0x20000, 0x55902200 )
ROM_END

struct GameDriver kod_driver =
{
        __FILE__,
        0,
        "kod",
        "King Of Dragons",
        "????",
        "Capcom",
        CPS1_CREDITS("Paul Leaman"),
        GAME_NOT_WORKING,
        &kod_machine_driver,

        kod_rom,
        0,0,0,0,
        input_ports_kod,
        NULL, 0, 0,
        ORIENTATION_DEFAULT,
        NULL, NULL
};


/********************************************************************

                          Varth

********************************************************************/

INPUT_PORTS_START( input_ports_varth )
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
        PORT_DIPSETTING(    0x03, "Difficult" )
        PORT_DIPSETTING(    0x02, "Very Difficult" )
        PORT_DIPSETTING(    0x01, "Hard" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x18, 0x18, "Bonus", IP_KEY_NONE )
        PORT_DIPSETTING(    0x18, "500K & every 1.400K ?" )
        PORT_DIPSETTING(    0x10, "500K, 2.000K, +500K ?" )
        PORT_DIPSETTING(    0x08, "1.200K & 3.500K ?" )
        PORT_DIPSETTING(    0x00, "2000K ?" )
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
        PORT_DIPSETTING(    0x01, "2" )
        PORT_DIPSETTING(    0x03, "3" )
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

CHAR_LAYOUT(charlayout_varth,     2048, 0x100000*8)
SPRITE_LAYOUT(spritelayout_varth, 4096*3-1024, 0x80000*8, 0x100000*8)
SPRITE_LAYOUT(tilelayout_varth,   4096+1024,  0x80000*8, 0x100000*8)
TILE32_LAYOUT(tilelayout32_varth, 1024,  0x080000*8, 0x100000*8)

static struct GfxDecodeInfo gfxdecodeinfo_varth[] =
{
        /*   start    pointer          colour start   number of colours */
        { 1, 0x058000, &charlayout_varth,    32*16,                  32 },
        { 1, 0x000000, &spritelayout_varth,  0,                      32 },
        { 1, 0x030000, &tilelayout_varth,    32*16+32*16,            32 },
        { 1, 0x060000, &tilelayout32_varth,  32*16+32*16+32*16,      32 },
        { -1 } /* end of array */
};

MACHINE_DRIVER(
        varth_machine_driver,
        cps1_interrupt2,
        gfxdecodeinfo_varth,
        CPS1_DEFAULT_CPU_SPEED)

ROM_START( varth_rom )
        ROM_REGION(0x100000)      /* 68000 code */
        ROM_LOAD_EVEN( "vae_30a.rom",  0x00000, 0x20000, 0x7fcd0091 )
        ROM_LOAD_ODD ( "vae_35a.rom",  0x00000, 0x20000, 0x35cf9509 )
        ROM_LOAD_EVEN( "vae_31a.rom",  0x40000, 0x20000, 0x15e5ee81 )
        ROM_LOAD_ODD ( "vae_36a.rom",  0x40000, 0x20000, 0x153a201e )
        ROM_LOAD_EVEN( "vae_28a.rom",  0x80000, 0x20000, 0x7a0e0d25 )
        ROM_LOAD_ODD ( "vae_33a.rom",  0x80000, 0x20000, 0xf2365922 )
        ROM_LOAD_EVEN( "vae_29a.rom",  0xc0000, 0x20000, 0x5e2cd2c3 )
        ROM_LOAD_ODD ( "vae_34a.rom",  0xc0000, 0x20000, 0x3d9bdf83 )

        ROM_REGION(0x200000)     /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "va_gfx1.rom",  0x000000, 0x80000, 0x0b1ace37 )
        ROM_LOAD( "va_gfx5.rom",  0x080000, 0x80000, 0xb1fb726e )
        ROM_LOAD( "va_gfx3.rom",  0x100000, 0x80000, 0x44dfe706 )
        ROM_LOAD( "va_gfx7.rom",  0x180000, 0x80000, 0x4c6588cd )

        ROM_REGION(0x18000) /* 64k for the audio CPU (+banks) */
        ROM_LOAD( "va_09.rom",    0x000000, 0x08000, 0x7a99446e )
        ROM_CONTINUE(          0x010000, 0x08000 )

        ROM_REGION(0x40000) /* Samples */
        ROM_LOAD( "va_18.rom",    0x00000, 0x20000, 0xde30510e )
        ROM_LOAD( "va_19.rom",    0x20000, 0x20000, 0x0610a4ac )
ROM_END

struct GameDriver varth_driver =
{
        __FILE__,
        &varth_driver,
        "varth",
        "Varth (World)",
        "1992",
        "Capcom",
        CPS1_CREDITS("Paul Leaman\nMarco Cassili (dip switches)"),
        0,
        &varth_machine_driver,

        varth_rom,
        0,
        0,0,
        0,      /* sound_prom */

        input_ports_varth,
        NULL, 0, 0,

        ORIENTATION_ROTATE_270,
        NULL, NULL
};


