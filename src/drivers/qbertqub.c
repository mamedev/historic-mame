/***************************************************************************

Q*bert Qubes: same as Q*bert with two banks of sprites
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int qbert_vh_start(void);
void gottlieb_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void qbert_sh_w(int offset, int data);
void gottlieb_video_outputs(int offset,int data);
extern unsigned char *gottlieb_paletteram;
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);

extern struct MemoryReadAddress gottlieb_sound_readmem[];
extern struct MemoryWriteAddress gottlieb_sound_writemem[];

int gottlieb_nvram_load(void);
void gottlieb_nvram_save(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, input_port_1_r },     /* IN0 buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* IN1 trackball: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* IN2 trackball: not used */
	{ 0x5804, 0x5804, input_port_4_r },     /* IN3 joystick */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3bff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, MWA_RAM }, /* bg object ram... ? not used ? */
	{ 0x5000, 0x501f, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball: not used */
	{ 0x5802, 0x5802, qbert_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_video_outputs },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x02, 0x00, "1st Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "15000" )
	PORT_DIPNAME( 0x08, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x35, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x24, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x14, "A 1/1 B 4/1" )
	PORT_DIPSETTING(    0x30, "A 1/1 B 3/1" )
	PORT_DIPSETTING(    0x10, "A 1/1 B 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x11, "A 2/3 B 2/1" )
	PORT_DIPSETTING(    0x15, "A 1/2 B 3/1" )
	PORT_DIPSETTING(    0x20, "A 1/2 B 2/1" )
	PORT_DIPSETTING(    0x21, "A 1/2 B 1/1" )
	PORT_DIPSETTING(    0x31, "A 1/2 B 1/5" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 2/1" )
	PORT_DIPSETTING(    0x05, "A 1/3 B 1/1" )
	PORT_DIPSETTING(    0x35, "Free Play" )
/* 0x25 "2 Coins/1 Credit"
   0x01 "1 Coin/1 Credit"
   0x34 "Free Play" */
	PORT_DIPNAME( 0x40, 0x00, "Additional Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x40, "25000" )
	PORT_DIPNAME( 0x80, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x80, "Hard" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x40, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_SERVICE, "Select in Service Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )

	PORT_START      /* IN2 trackball not used */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN3 trackball not used */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN4 joystick */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	512,    /* 512 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x4000*8, 0x8000*8, 0xC000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x2000, &spritelayout, 0, 1 },
	{ -1 } /* end of array */
};



static struct DACinterface dac_interface =
{
	1,
	441000,
	{ 255 },
	{ 0 },
};



static const struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			5000000,        /* 5 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU ,
			3579545/4,
			2,             /* memory region #2 */
			gottlieb_sound_readmem,gottlieb_sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	16, 16,
	gottlieb_vh_init_color_palette,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,      /* init vh */
	qbert_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



ROM_START( qbertqub_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "qq-rom3.bin", 0x8000, 0x2000, 0xac3cb8e2 )
	ROM_LOAD( "qq-rom2.bin", 0xa000, 0x2000, 0x64167070 )
	ROM_LOAD( "qq-rom1.bin", 0xc000, 0x2000, 0xdc7d6dc1 )
	ROM_LOAD( "qq-rom0.bin", 0xe000, 0x2000, 0xf2bad75a )

	ROM_REGION(0x12000)      /* temporary space for graphics */
	ROM_LOAD( "qq-bg0.bin", 0x0000, 0x1000, 0x13c600e6 )
	ROM_LOAD( "qq-bg1.bin", 0x1000, 0x1000, 0x542c9488 )
	ROM_LOAD( "qq-fg3.bin", 0x2000, 0x4000, 0xacd201f8 )       /* sprites */
	ROM_LOAD( "qq-fg2.bin", 0x6000, 0x4000, 0xa6a4660c )       /* sprites */
	ROM_LOAD( "qq-fg1.bin", 0xA000, 0x4000, 0x038fc633 )       /* sprites */
	ROM_LOAD( "qq-fg0.bin", 0xE000, 0x4000, 0x65b1f0f1 )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "qb-snd1.bin", 0xf000, 0x800, 0x469952eb )
	ROM_RELOAD(0x7000, 0x800) /* A15 is not decoded */
	ROM_LOAD( "qb-snd2.bin", 0xf800, 0x800, 0x200e1d22 )
	ROM_RELOAD(0x7800, 0x800) /* A15 is not decoded */
ROM_END



struct GameDriver qbertqub_driver =
{
	"Q*Bert Qubes",
	"qbertqub",
	"Fabrice Frances (MAME driver)\nMarco Cassili",
	&machine_driver,

	qbertqub_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	gottlieb_nvram_load, gottlieb_nvram_save
};

