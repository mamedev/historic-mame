/***************************************************************************

Mr Do! memory map (preliminary)

0000-7fff ROM
8000-83ff color RAM 1
8400-87ff video RAM 1
8800-8bff color RAM 2
8c00-8fff video RAM 2
e000-efff RAM

memory mapped ports:

read:
9803      SECRE 1/6-J2-11
a000      IN0
a001      IN1
a002      DSW1
a003      DSW2

write:
9000-90ff sprites, 64 groups of 4 bytes.
9800      flip (bit 0) playfield priority selector? (bits 1-3)
9801      sound port 1
9802      sound port 2
f000      playfield 0 Y scroll position (not used by Mr. Do!)
f800      playfield 0 X scroll position

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



extern unsigned char *mrdo_videoram2;
extern unsigned char *mrdo_colorram2;
extern unsigned char *mrdo_scroll_y;
void mrdo_videoram2_w(int offset,int data);
void mrdo_colorram2_w(int offset,int data);
void mrdo_flipscreen_w(int offset,int data);
void mrdo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int mrdo_vh_start(void);
void mrdo_vh_stop(void);
void mrdo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



/* this looks like some kind of protection. The game doesn't clear the screen */
/* if a read from this address doesn't return the value it expects. */
int mrdo_SECRE_r(int offset)
{
	Z80_Regs regs;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	Z80_GetRegs(&regs);
	return RAM[regs.HL.D];
}



static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },	/* video and color RAM */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa001, 0xa001, input_port_1_r },	/* IN1 */
	{ 0xa002, 0xa002, input_port_2_r },	/* DSW1 */
	{ 0xa003, 0xa003, input_port_3_r },	/* DSW2 */
	{ 0x9803, 0x9803, mrdo_SECRE_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0x8000, 0x83ff, colorram_w, &colorram },
	{ 0x8400, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x8bff, mrdo_colorram2_w, &mrdo_colorram2 },
	{ 0x8c00, 0x8fff, mrdo_videoram2_w, &mrdo_videoram2 },
	{ 0x9000, 0x90ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9800, 0x9800, mrdo_flipscreen_w },	/* screen flip + playfield priority */
	{ 0x9801, 0x9801, SN76496_0_w },
	{ 0x9802, 0x9802, SN76496_1_w },
	{ 0xf800, 0xffff, MWA_RAM, &mrdo_scroll_y },
	{ 0xf000, 0xf7ff, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Special", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, "Extra", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free play" )
	/* settings 0x01 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free play" )
	/* settings 0x10 thru 0x50 all give 1 Coin/1 Credit */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 4, 0 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			16+3, 16+2, 16+1, 16+0, 24+3, 24+2, 24+1, 24+0 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 128 },
	{ 1, 0x2000, &charlayout,       0, 128 },
	{ 1, 0x4000, &spritelayout, 4*128,  16 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	4000000,	/* 4 MHz */
	{ 255, 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	257,4*144,
	mrdo_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	mrdo_vh_start,
	mrdo_vh_stop,
	mrdo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mrdo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a4-01.bin", 0x0000, 0x2000, 0x30cf97ed )
	ROM_LOAD( "c4-02.bin", 0x2000, 0x2000, 0x06b5fea1 )
	ROM_LOAD( "e4-03.bin", 0x4000, 0x2000, 0xa56b5f71 )
	ROM_LOAD( "f4-04.bin", 0x6000, 0x2000, 0x4b09fc6d )

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s8-09.bin", 0x0000, 0x1000, 0xdf8ac406 )
	ROM_LOAD( "u8-10.bin", 0x1000, 0x1000, 0x78c4a89a )
	ROM_LOAD( "r8-08.bin", 0x2000, 0x1000, 0x005b757b )
	ROM_LOAD( "n8-07.bin", 0x3000, 0x1000, 0x5d25fe31 )
	ROM_LOAD( "h5-05.bin", 0x4000, 0x1000, 0x7f8e8642 )
	ROM_LOAD( "k5-06.bin", 0x5000, 0x1000, 0xb456cce4 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin", 0x0000, 0x0020, 0xe2c70b13 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin", 0x0020, 0x0020, 0x00392931 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin", 0x0040, 0x0020, 0x91e24fe2 )	/* sprite color lookup table */
ROM_END

ROM_START( mrdot_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "D1", 0x0000, 0x2000, 0xb738fe88 )
	ROM_LOAD( "D2", 0x2000, 0x2000, 0xa21679bc )
	ROM_LOAD( "D3", 0x4000, 0x2000, 0xa36ea250 )
	ROM_LOAD( "D4", 0x6000, 0x2000, 0xe25d6e3d )

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "D9",        0x0000, 0x1000, 0x0360f41e )
	ROM_LOAD( "D10",       0x1000, 0x1000, 0x9c9a9882 )
	ROM_LOAD( "r8-08.bin", 0x2000, 0x1000, 0x005b757b )
	ROM_LOAD( "n8-07.bin", 0x3000, 0x1000, 0x5d25fe31 )
	ROM_LOAD( "h5-05.bin", 0x4000, 0x1000, 0x7f8e8642 )
	ROM_LOAD( "k5-06.bin", 0x5000, 0x1000, 0xb456cce4 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin", 0x0000, 0x0020, 0xe2c70b13 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin", 0x0020, 0x0020, 0x00392931 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin", 0x0040, 0x0020, 0x91e24fe2 )	/* sprite color lookup table */
ROM_END

ROM_START( mrlo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "MRLO01.bin", 0x0000, 0x2000, 0xaca06df6 )
	ROM_LOAD( "D2",         0x2000, 0x2000, 0xa21679bc )
	ROM_LOAD( "MRLO03.bin", 0x4000, 0x2000, 0xf21e4688 )
	ROM_LOAD( "MRLO04.bin", 0x6000, 0x2000, 0xc031b3e9 )

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "MRLO09.bin", 0x0000, 0x1000, 0xe96e0ef6 )
	ROM_LOAD( "MRLO10.bin", 0x1000, 0x1000, 0xaa29cb17 )
	ROM_LOAD( "r8-08.bin",  0x2000, 0x1000, 0x005b757b )
	ROM_LOAD( "n8-07.bin",  0x3000, 0x1000, 0x5d25fe31 )
	ROM_LOAD( "h5-05.bin",  0x4000, 0x1000, 0x7f8e8642 )
	ROM_LOAD( "k5-06.bin",  0x5000, 0x1000, 0xb456cce4 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin", 0x0000, 0x0020, 0xe2c70b13 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin", 0x0020, 0x0020, 0x00392931 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin", 0x0040, 0x0020, 0x91e24fe2 )	/* sprite color lookup table */
ROM_END

ROM_START( mrdu_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "D1",       0x0000, 0x2000, 0xb738fe88 )
	ROM_LOAD( "D2",       0x2000, 0x2000, 0xa21679bc )
	ROM_LOAD( "D3",       0x4000, 0x2000, 0xa36ea250 )
	ROM_LOAD( "DU4.BIN",  0x6000, 0x2000, 0xdf60933e)

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "DU9.BIN",   0x0000, 0x1000, 0xe717f699 )
	ROM_LOAD( "DU10.BIN",  0x1000, 0x1000, 0xd0b2db78 )
	ROM_LOAD( "r8-08.bin", 0x2000, 0x1000, 0x005b757b )
	ROM_LOAD( "n8-07.bin", 0x3000, 0x1000, 0x5d25fe31 )
	ROM_LOAD( "h5-05.bin", 0x4000, 0x1000, 0x7f8e8642 )
	ROM_LOAD( "k5-06.bin", 0x5000, 0x1000, 0xb456cce4 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin", 0x0000, 0x0020, 0xe2c70b13 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin", 0x0020, 0x0020, 0x00392931 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin", 0x0040, 0x0020, 0x91e24fe2 )	/* sprite color lookup table */
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe017],"\x01\x00\x00",3) == 0 &&
			memcmp(&RAM[0xe071],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xe017],10*10);
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
		osd_fwrite(f,&RAM[0xe017],10*10);
		osd_fclose(f);
	}
}



struct GameDriver mrdo_driver =
{
	__FILE__,
	0,
	"mrdo",
	"Mr. Do! (Universal)",
	"1982",
	"Universal",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,

	mrdo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver mrdot_driver =
{
	__FILE__,
	&mrdo_driver,
	"mrdot",
	"Mr. Do! (Taito)",
	"1982",
	"Universal (Taito license)",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,

	mrdot_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver mrlo_driver =
{
	__FILE__,
	&mrdo_driver,
	"mrlo",
	"Mr. Lo!",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,

	mrlo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver mrdu_driver =
{
	__FILE__,
	&mrdo_driver,
	"mrdu",
	"Mr. Du!",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili\nLee Taylor",
	0,
	&machine_driver,

	mrdu_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

