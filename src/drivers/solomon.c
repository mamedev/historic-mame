#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *solomon_bgvideoram;
extern unsigned char *solomon_bgcolorram;

void solomon_flipscreen_w(int offset,int data);
void solomon_bgvideoram_w(int offset,int data);
void solomon_bgcolorram_w(int offset,int data);
int  solomon_vh_start(void);
void solomon_vh_stop(void);
void solomon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static void solomon_sh_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },	/* RAM */
	{ 0xd000, 0xdfff, MRA_RAM },	/* video + color + bg */
	{ 0xe000, 0xe07f, MRA_RAM },	/* spriteram  */
	{ 0xe400, 0xe5ff, MRA_RAM },	/* paletteram */
	{ 0xe600, 0xe600, input_port_0_r },
	{ 0xe601, 0xe601, input_port_1_r },
	{ 0xe602, 0xe602, input_port_2_r },
	{ 0xe604, 0xe604, input_port_3_r },	/* DSW1 */
	{ 0xe605, 0xe605, input_port_4_r },	/* DSW2 */
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd3ff, colorram_w, &colorram },
	{ 0xd400, 0xd7ff, videoram_w, &videoram, &videoram_size },
	{ 0xd800, 0xdbff, solomon_bgcolorram_w, &solomon_bgcolorram },
	{ 0xdc00, 0xdfff, solomon_bgvideoram_w, &solomon_bgvideoram },
	{ 0xe000, 0xe07f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe400, 0xe5ff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0xe600, 0xe600, interrupt_enable_w },
	{ 0xe604, 0xe604, solomon_flipscreen_w },
	{ 0xe800, 0xe800, solomon_sh_command_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress solomon_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x8000, 0x8000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress solomon_sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0xffff, 0xffff, MWA_NOP },	/* watchdog? */
	{ -1 }	/* end of table */
};

static struct IOWritePort solomon_sound_writeport[] =
{
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x11, 0x11, AY8910_write_port_0_w },
	{ 0x20, 0x20, AY8910_control_port_1_w },
	{ 0x21, 0x21, AY8910_write_port_1_w },
	{ 0x30, 0x30, AY8910_control_port_2_w },
	{ 0x31, 0x31, AY8910_write_port_2_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( solomon_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x0c, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0xc0, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/3 Credits" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown DSW2 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown DSW2 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown DSW2 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown DSW2 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown DSW2 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0xe0, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "30k 200k 500k" )
	PORT_DIPSETTING(    0x40, "30k 200k" )
	PORT_DIPSETTING(    0x20, "30k" )
	PORT_DIPSETTING(    0xa0, "100k" )
	PORT_DIPSETTING(    0xc0, "100k 300k" )
	PORT_DIPSETTING(    0x80, "100k 300k 800k" )
	PORT_DIPSETTING(    0x60, "200k" )
	PORT_DIPSETTING(    0xe0, "None" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 8*8 sprites */
	512,	/* 512 sprites */
	4,		/* 4 bits per pixel */
	{ 0, 512*32*8, 2*512*32*8, 3*512*32*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,     0, 8 },	/* colors   0-127 */
	{ 1, 0x10000, &charlayout,   128, 8 },	/* colors 128-255 */
	{ 1, 0x20000, &spritelayout,   0, 8 },	/* colors   0-127 */
	{ -1 } /* end of array */
};

static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 255, 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver solomon_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?????) */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz (?????) */
			2,
			solomon_sound_readmem,solomon_sound_writemem,0,solomon_sound_writeport,
			interrupt,2	/* ??? */
						/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_DIRTY,
	0,
	solomon_vh_start,
	solomon_vh_stop,
	solomon_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( solomon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "slmn_06.bin", 0x00000, 0x4000, 0x9211c53d , 0xe4d421ff )
	ROM_LOAD( "slmn_07.bin", 0x08000, 0x4000, 0x35d3a0dd , 0xd52d7e38 )
	ROM_CONTINUE(            0x04000, 0x4000 )
	ROM_LOAD( "slmn_08.bin", 0x0f000, 0x1000, 0xe026fd64 , 0x0a6cdefc )

	ROM_REGION_DISPOSE(0x30000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "slmn_12.bin", 0x00000, 0x08000, 0x92abe6f5 , 0xaa26dfcb )	/* characters */
	ROM_LOAD( "slmn_11.bin", 0x08000, 0x08000, 0x5139f6c1 , 0x6f94d2af )
	ROM_LOAD( "slmn_10.bin", 0x10000, 0x08000, 0x515e3a4a , 0x8310c2a1 )
	ROM_LOAD( "slmn_09.bin", 0x18000, 0x08000, 0xfb92016e , 0xab7e6c42 )
	ROM_LOAD( "slmn_02.bin", 0x20000, 0x04000, 0x15ad4397 , 0x80fa2be3 )	/* sprites */
	ROM_LOAD( "slmn_03.bin", 0x24000, 0x04000, 0xb9936b79 , 0x236106b4 )
	ROM_LOAD( "slmn_04.bin", 0x28000, 0x04000, 0xb916d26c , 0x088fe5d9 )
	ROM_LOAD( "slmn_05.bin", 0x2c000, 0x04000, 0x86ac094c , 0x8366232a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "slmn_01.bin", 0x0000, 0x4000, 0xd893aff5 , 0xfa6e562e )
ROM_END


static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xcaa2],"\x2e\x2e\x03\x01",4) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xca4c],90);
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
		osd_fwrite(f,&RAM[0xca4c],90);
		osd_fclose(f);
	}
}


struct GameDriver solomon_driver =
{
	__FILE__,
	0,
	"solomon",
	"Solomon's Key (Japan)",
	"1986",
	"Tecmo",
	"Mirko Buffoni",
	0,
	&solomon_machine_driver,

	solomon_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	solomon_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
