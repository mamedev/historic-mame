/***************************************************************************

Commando memory map (preliminary)

MAIN CPU
0000-bfff ROM
d000-d3ff Video RAM
d400-d7ff Color RAM
d800-dbff background video RAM
dc00-dfff background color RAM
e000-ffff RAM
fe00-ff7f Sprites

read:
c000      IN0
c001      IN1
c002      IN2
c003      DSW1
c004      DSW2

write:
c808-c809 background scroll y position
c80a-c80b background scroll x position

SOUND CPU
0000-3fff ROM
4000-47ff RAM

write:
8000      YM2203 #1 control
8001      YM2203 #1 write
8002      YM2203 #2 control
8003      YM2203 #2 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *commando_bgvideoram,*commando_bgcolorram;
extern int commando_bgvideoram_size;
extern unsigned char *commando_scrollx,*commando_scrolly;
void commando_bgvideoram_w(int offset,int data);
void commando_bgcolorram_w(int offset,int data);
void commando_spriteram_w(int offset,int data);
void commando_c804_w(int offset,int data);
int commando_vh_start(void);
void commando_vh_stop(void);
void commando_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void commando_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static int commando_interrupt(void)
{
	return 0x00d7;	/* RST 10h - VBLANK */
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc804, 0xc804, commando_c804_w },
	{ 0xc808, 0xc809, MWA_RAM, &commando_scrolly },
	{ 0xc80a, 0xc80b, MWA_RAM, &commando_scrollx },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xdbff, commando_bgvideoram_w, &commando_bgvideoram, &commando_bgvideoram_size },
	{ 0xdc00, 0xdfff, commando_bgcolorram_w, &commando_bgcolorram },
	{ 0xe000, 0xfdff, MWA_RAM },
	{ 0xfe00, 0xff7f, commando_spriteram_w, &spriteram, &spriteram_size },
	{ 0xff80, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0x8002, 0x8002, YM2203_control_port_1_w },
	{ 0x8003, 0x8003, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Starting Stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "40000 50000" )
	PORT_DIPSETTING(    0x07, "10000 500000" )
	PORT_DIPSETTING(    0x03, "10000 600000" )
	PORT_DIPSETTING(    0x05, "20000 600000" )
	PORT_DIPSETTING(    0x01, "20000 700000" )
	PORT_DIPSETTING(    0x06, "30000 700000" )
	PORT_DIPSETTING(    0x02, "30000 800000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x20, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0xc0, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright One Player" )
	PORT_DIPSETTING(    0x40, "Upright Two Players" )
/*	PORT_DIPSETTING(    0x80, "Cocktail" ) */
	PORT_DIPSETTING(    0xc0, "Cocktail" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
	1024,	/* 1024 tiles */
	3,	/* 3 bits per pixel */
	{ 0, 1024*32*8, 2*1024*32*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	768,	/* 768 sprites */
	4,	/* 4 bits per pixel */
	{ 768*64*8+4, 768*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   192, 16 },	/* colors 192-255 */
	{ 1, 0x04000, &tilelayout,     0, 16 },	/* colors   0-127 */
	{ 1, 0x1c000, &spritelayout, 128,  4 },	/* colors 128-191 */
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz */
	{ YM2203_VOL(128,255), YM2203_VOL(128,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			commando_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, 500,	/* frames per second, vblank duration */
				/* vblank duration is crucial to get proper sprite/background alignment */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256,16*4+4*16+16*8,
	commando_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	commando_vh_start,
	commando_vh_stop,
	commando_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( commando_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "m09_cm04.bin", 0x0000, 0x8000, 0xf44b9f43 )
	ROM_LOAD( "m08_cm03.bin", 0x8000, 0x4000, 0x6e158a17 )

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d05_vt01.bin", 0x00000, 0x4000, 0x9c3344b3 )	/* characters */
	ROM_LOAD( "a05_vt11.bin", 0x04000, 0x4000, 0x0babe1d9 )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x08000, 0x4000, 0x0ef15ee7 )
	ROM_LOAD( "a07_vt13.bin", 0x0c000, 0x4000, 0x8244ea38 )
	ROM_LOAD( "a08_vt14.bin", 0x10000, 0x4000, 0x91390ad1 )
	ROM_LOAD( "a09_vt15.bin", 0x14000, 0x4000, 0x755876be )
	ROM_LOAD( "a10_vt16.bin", 0x18000, 0x4000, 0x8c6d8225 )
	ROM_LOAD( "e07_vt05.bin", 0x1c000, 0x4000, 0x4eda8b78 )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x20000, 0x4000, 0x280b34f9 )
	ROM_LOAD( "e09_vt07.bin", 0x24000, 0x4000, 0x2ab5880f )
	ROM_LOAD( "h07_vt08.bin", 0x28000, 0x4000, 0x48696aa7 )
	ROM_LOAD( "h08_vt09.bin", 0x2c000, 0x4000, 0xab521082 )
	ROM_LOAD( "h09_vt10.bin", 0x30000, 0x4000, 0x998c53a6 )

	ROM_REGION(0x0600)	/* color PROMs */
	ROM_LOAD( "01d_vtb1.bin", 0x0000, 0x0200, 0x524d0207 )
	ROM_LOAD( "02d_vtb2.bin", 0x0200, 0x0200, 0xfb20050e )
	ROM_LOAD( "03d_vtb3.bin", 0x0400, 0x0200, 0x1e8b0b07 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "f09_cm02.bin", 0x0000, 0x4000, 0x07fced60 )
ROM_END

ROM_START( commandj_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "09m_so04.bin", 0x0000, 0x8000, 0xf5dffe09 )
	ROM_LOAD( "08m_so03.bin", 0x8000, 0x4000, 0xf8463efe )

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d05_vt01.bin", 0x00000, 0x4000, 0x9c3344b3 )	/* characters */
	ROM_LOAD( "a05_vt11.bin", 0x04000, 0x4000, 0x0babe1d9 )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x08000, 0x4000, 0x0ef15ee7 )
	ROM_LOAD( "a07_vt13.bin", 0x0c000, 0x4000, 0x8244ea38 )
	ROM_LOAD( "a08_vt14.bin", 0x10000, 0x4000, 0x91390ad1 )
	ROM_LOAD( "a09_vt15.bin", 0x14000, 0x4000, 0x755876be )
	ROM_LOAD( "a10_vt16.bin", 0x18000, 0x4000, 0x8c6d8225 )
	ROM_LOAD( "e07_vt05.bin", 0x1c000, 0x4000, 0x4eda8b78 )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x20000, 0x4000, 0x280b34f9 )
	ROM_LOAD( "e09_vt07.bin", 0x24000, 0x4000, 0x2ab5880f )
	ROM_LOAD( "h07_vt08.bin", 0x28000, 0x4000, 0x48696aa7 )
	ROM_LOAD( "h08_vt09.bin", 0x2c000, 0x4000, 0xab521082 )
	ROM_LOAD( "h09_vt10.bin", 0x30000, 0x4000, 0x998c53a6 )

	ROM_REGION(0x0600)	/* color PROMs */
	ROM_LOAD( "01d_vtb1.bin", 0x0000, 0x0200, 0x524d0207 )
	ROM_LOAD( "02d_vtb2.bin", 0x0200, 0x0200, 0xfb20050e )
	ROM_LOAD( "03d_vtb3.bin", 0x0400, 0x0200, 0x1e8b0b07 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "09f_so02.bin", 0x0000, 0x4000, 0xfda056a2 )
ROM_END



static void commando_decode(void)
{
	static const unsigned char xortable[] =
	{
		0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,
		0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,
		0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,
		0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,
		0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,
		0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,
		0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00,0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,
		0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00,0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,
		0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,
		0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,
		0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,
		0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,
		0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,
		0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,
		0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00,
		0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00
	};
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0;A < 0xc000;A++)
	{
		if (A > 0) ROM[A] = RAM[A] ^ xortable[RAM[A]];
	}
}



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xee00],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0xee4e],"\x00\x08\x00",3) == 0 &&
			memcmp(&RAM[0xee97],"\x00\x50\x00",3) == 0)	/* high score */
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xee00],13*7);
			RAM[0xee97] = RAM[0xee00];
			RAM[0xee98] = RAM[0xee01];
			RAM[0xee99] = RAM[0xee02];
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
		osd_fwrite(f,&RAM[0xee00],13*7);
		osd_fclose(f);
	}
}



struct GameDriver commando_driver =
{
	__FILE__,
	0,
	"commando",
	"Commando (US)",
	"1985",
	"Capcom",
	"Paul Johnson (hardware info)\nNicola Salmoria (MAME driver)",
	0,
	&machine_driver,

	commando_rom,
	0, commando_decode,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver commandj_driver =
{
	__FILE__,
	&commando_driver,
	"commandj",
	"Commando (Japan)",
	"1985",
	"Capcom",
	"Paul Johnson (hardware info)\nNicola Salmoria (MAME driver)",
	0,
	&machine_driver,

	commandj_rom,
	0, commando_decode,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
