/***************************************************************************

Mysterious Stones memory map (preliminary)

MAIN BOARD:

0000-0fff RAM
1000-13ff Video RAM
1400-17ff Attribute RAM
1800-19ff Background video RAM #1
1a00-1bff Background attribute RAM #1
1c00-1fff probably scratchpad RAM, contains a copy of the background without objects
4000-ffff ROM

read
2000      IN0
          bit 7 = coin 2 (must also cause a NMI)
          bit 6 = coin 1 (must also cause a NMI)
          bit 0-5 = player 1 controls
2010      IN1
          bit 7 = start 2
          bit 6 = start 1
		  bit 0-5 = player 2 controls (cocktail only)
2020      DSW1
          bit 3-7 = probably unused
          bit 2 = ?
		  bit 1 = ?
          bit 0 = lives
2030      DSW2
          bit 7 = vblank
          bit 6 = coctail/upright (0 = upright)
		  bit 4-5 = probably unused
		  bit 0-3 = coins per play

write
0780-07df sprites
2000      Tile RAM bank select?
2010      Commands for the sound CPU?
2020      background scroll
2030      ?
2040      ?
2060-2077 palette

Known problems:

* In "intermission" screens, the characters come out black. Is this normal?
* There appears to be a sound CPU, similar to Mat Mania. The ROMs are missing.
* Some dipswitches may not be mapped correctly.
* Not sure if the "cocktail" mode flips the screen or not.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m6502/m6502.h"



int mystston_DSW2_r(int offset);
int mystston_interrupt(void);

extern unsigned char *mystston_videoram2,*mystston_colorram2;
extern int mystston_videoram2_size;
extern unsigned char *mystston_videoram3,*mystston_colorram3;
extern int mystston_videoram3_size;
extern unsigned char *mystston_scroll;

void mystston_videoram3_w(int offset,int data);
void mystston_colorram3_w(int offset,int data);
int mystston_vh_start(void);
void mystston_vh_stop(void);
void mystston_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* extern unsigned char *mystston_pageselect; */


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x077f, MRA_RAM },
	{ 0x1000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2000, input_port_0_r },
	{ 0x2010, 0x2010, input_port_1_r },
	{ 0x2020, 0x2020, input_port_2_r },
	{ 0x2030, 0x2030, input_port_3_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x077f, MWA_RAM },
	{ 0x0780, 0x07df, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1000, 0x13ff, MWA_RAM, &mystston_videoram2, &mystston_videoram2_size },
	{ 0x1400, 0x17ff, MWA_RAM, &mystston_colorram2 },
	{ 0x1800, 0x19ff, videoram_w, &videoram, &videoram_size },
	{ 0x1a00, 0x1bff, colorram_w, &colorram },
	{ 0x1c00, 0x1dff, mystston_videoram3_w, &mystston_videoram3, &mystston_videoram3_size },
	{ 0x1e00, 0x1fff, mystston_colorram3_w, &mystston_colorram3 },
/*	{ 0x2000, 0x2000, MWA_RAM, &mystston_pageselect }, */
/*	{ 0x2010, 0x2010, soundlatch_w }, */
	{ 0x2020, 0x2020, MWA_RAM, &mystston_scroll },
	{ 0x2060, 0x2077, paletteram_BBGGGRRR_w, &paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};




INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME (0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "5" )
	PORT_DIPSETTING (   0x01, "3" )
	PORT_DIPNAME (0x02, 0x02, "Switch 1", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPSETTING (   0x02, "Off" )
	PORT_DIPNAME (0x04, 0x04, "Switch 2", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPSETTING (   0x04, "Off" )
	PORT_BIT ( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW2 */
	PORT_DIPNAME (0x03, 0x03, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME (0x0c, 0x0c, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x04, "1 Coin/3 Credits" )
	PORT_BIT ( 0x30, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME (0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Upright" )
	PORT_DIPSETTING (   0x40, "Cocktail" )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	512,    /* 512 tiles */
	3,	/* 3 bits per pixel */
	{ 2*512*16*16, 512*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every tile takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   1*8, 1 },
	{ 1, 0x06000, &charlayout,   1*8, 1 },
	{ 1, 0x0c000, &tilelayout,   2*8, 1 },
	{ 1, 0x00000, &spritelayout,   0, 1 },
	{ 1, 0x06000, &spritelayout,   0, 1 },
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 MHz ???? */
			0,
			readmem,writemem,0,0,
			mystston_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	24, 24,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	mystston_vh_start,
	mystston_vh_stop,
	mystston_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};





/***************************************************************************

  Mysterious Stones driver

***************************************************************************/

ROM_START( mystston_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ms0", 0x4000, 0x2000, 0xdc153dbd )
	ROM_LOAD( "ms1", 0x6000, 0x2000, 0xfee91da9 )
	ROM_LOAD( "ms2", 0x8000, 0x2000, 0x6a8e9056 )
	ROM_LOAD( "ms3", 0xa000, 0x2000, 0x01236275 )
	ROM_LOAD( "ms4", 0xc000, 0x2000, 0x4b454e47 )
	ROM_LOAD( "ms5", 0xe000, 0x2000, 0x3d2ed112 )

	ROM_REGION(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ms6",  0x00000, 0x2000, 0x36d91451 )
	ROM_LOAD( "ms7",  0x02000, 0x2000, 0x1c50b31a )
	ROM_LOAD( "ms8",  0x04000, 0x2000, 0x8fa87372 )
	ROM_LOAD( "ms9",  0x06000, 0x2000, 0xdb1e1106 )
	ROM_LOAD( "ms10", 0x08000, 0x2000, 0xf58ef682 )
	ROM_LOAD( "ms11", 0x0a000, 0x2000, 0xb435a9c1 )
	ROM_LOAD( "ms12", 0x0c000, 0x2000, 0x86001ec2 )
	ROM_LOAD( "ms13", 0x0e000, 0x2000, 0x9db56e87 )
	ROM_LOAD( "ms14", 0x10000, 0x2000, 0x1406c3f8 )
	ROM_LOAD( "ms15", 0x12000, 0x2000, 0x511f13b1 )
	ROM_LOAD( "ms16", 0x14000, 0x2000, 0x382355d5 )
	ROM_LOAD( "ms17", 0x16000, 0x2000, 0xf2d7eb01 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0308],"\x00\x00\x21",3) == 0) &&
		(memcmp(&RAM[0x033C],"\x0C\x1D\x0C",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0308],11*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0308],11*5);
		osd_fclose(f);
	}

}



struct GameDriver mystston_driver =
{
	__FILE__,
	0,
	"mystston",
	"Mysterious Stones",
	"1984",
	"Technos",
	"Nicola Salmoria\nMike Balfour\nBrad Oliver",
	0,
	&machine_driver,

	mystston_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};



