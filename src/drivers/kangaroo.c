/* Kangaroo driver
Kangaroo (c) Atari Games / Sun Electronics Corp 1982

   Changes:
   97/06/19 - mod to ensure it's really safe to load the scores.

   97/06/17 - added the coin counter output so the error log will
              not get cluttered with so much garbage about unknown locations.
            - added music on/off to dip switch settings.
              Thanks to S. Joe Dunkle for mentioning the game should
              have music. btw. this is not really a dip switch on the PCB,
              it's a pin on the edge connector and the damn manual doesn't
              really tell what it does, so I hadn't ever tried it out...
            - added high score saving/loading. Since I try to avoid disassembling
              the code at all costs I'm not sure it's correct - seems to work tho' ;-) -V-

   97/05/07 - changed to conform the new AUDIO_CPU code
            - changed the audio command read to use the latched version.

   97/04/xx - created, based on the Arabian driver,
            The two games (arabian & kangaroo) are both by Sun Electronics
            and run on very similar hardware.
            Kangaroo PCB is constructed from two boards:
            TVG-1-CPU-B , which houses program ROMS, two Z80 CPUs,
             a GI-AY-8910, a custom microcontroller and the logic chips connecting these.
            TVG-1-VIDEO-B is obviously the video board. On this one
             you can find the graphics ROMS (tvg83 -tvg86), some logic
             chips and 4 banks of 8 4116 RAM chips.

            */
/* TODO */
/* This is still a bit messy after my cut/paste from the arabian driver */
/* will have to clean up && correct the sound problems */

/***************************************************************************

Kangaroo memory map (preliminary)
(these are for CPU#0)

0000-0fff tvg75
1000-1fff tvg76
2000-2fff tvg77
3000-3fff tvg78
4000-4fff tvg79
5000-5fff tvg80

8000-bfff VIDEO RAM

e000-e3ff RAM

e800-e80a CRT controller  ? blitter ?


memory mapped ports:
read:
ec00 - IN 0
ed00 - IN 1
ee00 - IN 2

efxx - security chip in/out
       it seems to work like a clock.

e400 - DSW 0

DSW 0
bit 7 = ?
bit 6 = ?
bit 5 = ?
bit 4 = ?
bit 3 = ?
bit 2 = 1 - test mode
bit 1 = COIN 2
bit 0 = COIN 1
---------------------------------------------------------------------------
(these are for CPU#1)
0x0000-0x0fff tvg81

0x4000-0x43ff RAM
0x6000        sound commands from CPU#0

8000 - AY-3-8910 control
7000 - AY-3-8910 write
---------------------------------------------------------------------------

interrupts:
(CPU#0) standard IM 1 interrupt mode (rst #38 every vblank)
(CPU#1) same here
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"
#include "sndhrdw/generic.h"



/* machine */
int  kangaroo_sec_chip_r(int offset);
void kangaroo_sec_chip_w(int offset,int val);

/* vidhrdw */
int  kangaroo_vh_start(void);
void kangaroo_vh_stop(void);
void kangaroo_vh_screenrefresh(struct osd_bitmap *bitmap);
void kangaroo_spriteramw(int offset, int val);
void kangaroo_videoramw(int offset, int val);
void kangaroo_color_shadew(int offset, int val);

/*sndhrdw*/
int kangaroo_sh_start(void);

static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x5fff, MRA_ROM },
        { 0x8000, 0xbfff, MRA_RAM },
        { 0xee00, 0xee00, input_port_2_r },
        { 0xe400, 0xe400, input_port_3_r },
        { 0xe000, 0xe3ff, MRA_RAM },
        { 0xec00, 0xec00, input_port_0_r },
        { 0xed00, 0xed00, input_port_1_r },
        { 0xef00, 0xef00, kangaroo_sec_chip_r },
        { 0xe800, 0xe80a, MRA_RAM },
	{ -1 }  /* end of table */
};
static struct MemoryReadAddress sh_readmem[] =
{
        { 0x0000, 0x0fff, MRA_ROM },
        { 0x4000, 0x43ff, MRA_RAM },
        { 0x6000, 0x6000, sound_command_latch_r },
        { -1 }
};
static struct MemoryWriteAddress writemem[] =
{
        { 0x8000, 0xbfff, kangaroo_videoramw, &videoram },
        { 0xe000, 0xe3ff, MWA_RAM },
        { 0xe800, 0xe80a, kangaroo_spriteramw, &spriteram },
        { 0xef00, 0xefff, kangaroo_sec_chip_w },
        { 0xec00, 0xec00, sound_command_w },
        { 0xed00, 0xed00, MWA_NOP },
        { 0x0000, 0x5fff, MWA_ROM },
        { -1 }  /* end of table */
};
static struct MemoryWriteAddress sh_writemem[] =
{
        { 0x4000, 0x43ff, MWA_RAM },
        { 0x0000, 0x0fff, MWA_ROM },
        { 0x6000, 0x6000, MWA_ROM },
        { -1 }
};


void kanga_moja(int port, int val)
{
/* does this really work, or is there a problem with the PSG code,
   it seems like one channel is completely missing from the output
*/

  Z80_Regs regs;
  Z80_GetRegs(&regs);

  if (regs.BC.D==0x8000)
    AY8910_control_port_0_w(port,val);
  else /* it must be 0x7000 ;-) -V- */
    AY8910_write_port_0_w(port,val);

}
int kanga_rdport(int port)
{
/* this is actually not used*/
        return AY8910_read_port_0_r(port);
}
static struct IOWritePort sh_writeport[] =
{
        { 0x00, 0x00, kanga_moja },
	{ -1 }	/* end of table */
};
static struct IOReadPort sh_readport[] =
{
        { 0x00, 0x00, kanga_rdport },
        { -1 }
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x20, 0x00, "Music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "10000 30000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x0c, "20000 40000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xf0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x20, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x30, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x40, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x50, "A 1/1 B 1/4" )
	PORT_DIPSETTING(    0x60, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x70, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x80, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x90, "A 1/2 B 1/4" )
	PORT_DIPSETTING(    0xa0, "A 1/2 B 1/5" )
	PORT_DIPSETTING(    0xe0, "A 1/2 B 1/6" )
	PORT_DIPSETTING(    0xb0, "A 1/2 B 1/10" )
	PORT_DIPSETTING(    0xc0, "A 1/2 B 1/11" )
	PORT_DIPSETTING(    0xd0, "A 1/2 B 1/12" )
	PORT_DIPSETTING(    0xf0, "Free Play" )
INPUT_PORTS_END



static struct GfxLayout sprlayout =
{
	1,4,    /* 4 * 1 lines */
	8192,   /* 8192 "characters" */
	4,      /* 4 bpp        */
	{ 0x2000*8, 0x2000*8+4, 0, 4  },       /* 4 and 8192 bytes between planes */
	{ 0,1,2,3 },
	{ 0,1,2,3 },
	1*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 2, 0 , &sprlayout, 0, 1 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,
	0x00,0x00,42*4,
	0x00,42*4,0x00,
	0x00,42*4,42*4,
	42*4,00,00,
	42*4,00,42*4,
	42*4,21*4,00,
	42*4,42*4,42*4,
	21*4,21*4,21*4,
	21*4,21*4,63*4,
	21*4,63*4,21*4,
	21*4,63*4,63*4,
	63*4,21*4,21*4,
	63*4,21*4,63*4,
	63*4,63*4,21*4,
	63*4,63*4,63*4
};


static unsigned char colortable[] =
{
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};

/* 971706 -V- */
static int kangaroo_hiload(void)
{
        /* Ok, we need to explicitly tell what RAM to read...*/
        /* realizing this was necessary took me quite a long time :( -V- */

        unsigned char *RAM = Machine->memory_region[0];

        /* just a guess really... */
        if ( RAM[0xe1da] == 0x50 )
        {
                void *f;
                if (( f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0xe1a0],0x40);
                        /* is this enough ??? */
                        osd_fclose(f);
                }
                return 1;
         }
         else return 0; /* didn't load them yet, do stop by later ;-) -V- */
}
/* 970617 -V- */
static void kangaroo_hisave(void)
{
        void *f;

        unsigned char *RAM = Machine->memory_region[0];

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xe1a0],0x40);
                osd_fclose(f);
        }
}



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3000000,        /* 3 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2500000,
			3,
			sh_readmem,sh_writemem,sh_readport,sh_writeport,
			interrupt,1
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
	32*8, 32*8, { 0x0b, 0xfa, 0, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,
	kangaroo_vh_start,
	kangaroo_vh_stop,
	kangaroo_vh_screenrefresh,

	/* sound hardware */
	0,
	kangaroo_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kangaroo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tvg75.bin", 0x0000, 0x1000, 0xc79eeb0a )  /* fonts are messed up with */
	ROM_LOAD( "tvg76.bin", 0x1000, 0x1000, 0x1f254c59 )  /* program code */
	ROM_LOAD( "tvg77.bin", 0x2000, 0x1000, 0x84cf6cd7 )
	ROM_LOAD( "tvg78.bin", 0x3000, 0x1000, 0xfec4c312 )
	ROM_LOAD( "tvg79.bin", 0x4000, 0x1000, 0x55d4a740 )
	ROM_LOAD( "tvg80.bin", 0x5000, 0x1000, 0xf47576eb )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) /* space for graphics roms */
	ROM_LOAD( "tvg84.bin", 0x0000, 0x1000, 0x6d872ead )  /* because of very rare way */
	ROM_LOAD( "tvg86.bin", 0x1000, 0x1000, 0x9048077c )  /* CRT controller uses these roms */
	ROM_LOAD( "tvg83.bin", 0x2000, 0x1000, 0x5da33f11 )  /* there's no way, but to decode */
	ROM_LOAD( "tvg85.bin", 0x3000, 0x1000, 0x6450024c )  /* it at runtime - which is SLOW */

	ROM_REGION(0x10000) /* sound */
	ROM_LOAD( "tvg81.bin", 0x0000, 0x1000, 0x03a6e98a )
ROM_END



struct GameDriver kangaroo_driver =
{
	"Kangaroo",
	"kangaroo",
	"Ville Laitinen (MAME driver)\nMarco Cassili",
	&machine_driver,

	kangaroo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	kangaroo_hiload, kangaroo_hisave
};
