/***************************************************************************

Pinball Action memory map (preliminary)
0000-9fff ROM
d000-d3ff Video RAM
d400-d7ff Color RAM
d800-dbff Background Video RAM
dc00-dfff Background Color RAM
e000-e07f Sprites
e400-e5ff Palette RAM

read:
e600      IN0
e601      IN1
e602      IN2
e604      DSW1
e605      DSW2
e606      watchdog reset????

write:
e600      interrupt enable

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


extern unsigned char *pbaction_videoram2,*pbaction_colorram2;
extern unsigned char *pbaction_paletteram;
void pbaction_paletteram_w(int offset,int data);
void pbaction_videoram2_w(int offset,int data);
void pbaction_colorram2_w(int offset,int data);
int pbaction_vh_start(void);
void pbaction_vh_stop(void);

void pbaction_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void pbaction_vh_screenrefresh(struct osd_bitmap *bitmap);

int pbaction_sh_interrupt(void);
int pbaction_sh_start(void);
void pbaction_sh_stop(void);
void pbaction_sh_update(void);

void pbaction_sh_0_w( int offset , int data );
void pbaction_sh_1_w( int offset , int data );
void pbaction_sh_2_w( int offset , int data );

void pbaction_pio_w( int offset , int data );
int  pbaction_pio_r( int offset );
void pbaction_ctc_w( int offset , int data );
int  pbaction_ctc_r( int offset );

extern unsigned char *pbaction_sound_command;
void pbaction_sound_latch( int offset , int data );


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe07f, MRA_RAM },
	{ 0xe400, 0xe5ff, MRA_RAM },
	{ 0xe600, 0xe600, input_port_0_r },	/* IN0 */
	{ 0xe601, 0xe601, input_port_1_r },	/* IN1 */
	{ 0xe602, 0xe602, input_port_2_r },	/* IN2 */
	{ 0xe604, 0xe604, input_port_3_r },	/* DSW1 */
	{ 0xe605, 0xe605, input_port_4_r },	/* DSW2 */
	{ 0xe606, 0xe606, MRA_NOP },	/* ??? */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xdbff, pbaction_videoram2_w, &pbaction_videoram2 },
	{ 0xdc00, 0xdfff, pbaction_colorram2_w, &pbaction_colorram2 },
	{ 0xe000, 0xe07f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe400, 0xe5ff, pbaction_paletteram_w, &pbaction_paletteram },
	{ 0xe600, 0xe600, interrupt_enable_w },
	{ 0xe800, 0xe800, sound_command_w },	/* ??? */
	{ -1 }  /* end of table */
};

#ifdef TRY_SOUND
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
//	{ 0x00, 0x03, pbaction_pio_r },
//	{ 0x08, 0x0b, pbaction_ctc_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
//	{ 0x00, 0x03, pbaction_pio_w },
//	{ 0x08, 0x0b, pbaction_ctc_w },
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x11, 0x11, AY8910_write_port_0_w },
	{ 0x20, 0x20, AY8910_control_port_1_w },
	{ 0x21, 0x21, AY8910_write_port_1_w },
	{ 0x30, 0x30, AY8910_control_port_2_w },
	{ 0x31, 0x31, AY8910_write_port_2_w },
	{ -1 }	/* end of table */
};
#endif


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "70K 200K 1000K" )
	PORT_DIPSETTING(    0x00, "70K 200K" )
	PORT_DIPSETTING(    0x04, "100K 300K 1000K" )
	PORT_DIPSETTING(    0x03, "100K 300K" )
	PORT_DIPSETTING(    0x02, "100K" )
	PORT_DIPSETTING(    0x06, "200K 1000K" )
	PORT_DIPSETTING(    0x05, "200K" )
	PORT_DIPSETTING(    0x07, "None" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 2048*8*8, 2*2048*8*8, 3*2048*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 256*16*16, 2*256*16*16 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout2 =
{
	32,32,	/* 32*32 sprites */
	32,	/* 32 sprites */
	3,	/* 3 bits per pixel */
	{ 2*64*32*32, 64*32*32, 0 },	/* the bitplanes are separated */
	{ 87*8, 86*8, 85*8, 84*8, 83*8, 82*8, 81*8, 80*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			40*8+0, 40*8+1, 40*8+2, 40*8+3, 40*8+4, 40*8+5, 40*8+6, 40*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout1,      0, 16 },	/* characters */
	{ 1, 0x06000, &charlayout2,   16*8, 8 },	/* background */
	{ 1, 0x16000, &spritelayout1,    0, 16 },	/* normal sprites */
	{ 1, 0x17000, &spritelayout2,    0, 16 },	/* large sprites */
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz? */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
#ifdef TRY_SOUND
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,        /* 2 Mhz? */
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			pbaction_sh_interrupt,10
#endif
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0, 32*8-1 },
	gfxdecodeinfo,
	256,16*8+8*16,
	pbaction_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	pbaction_vh_start,
	pbaction_vh_stop,
	pbaction_vh_screenrefresh,

	/* sound hardware */
#ifdef TRY_SOUND
	0,
	pbaction_sh_start,
	pbaction_sh_stop,
	pbaction_sh_update,
#else
	0,0,0,0
#endif
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pbaction_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "b-p7.bin", 0x0000, 0x4000, 0x2458fd9a )
	ROM_LOAD( "b-n7.bin", 0x4000, 0x4000, 0x431e9dea )
	ROM_LOAD( "b-l7.bin", 0x8000, 0x2000, 0x8203095b )

	ROM_REGION(0x1c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a-s6.bin", 0x00000, 0x2000, 0x85fb19bf )
	ROM_LOAD( "a-s7.bin", 0x02000, 0x2000, 0x94ea0412 )
	ROM_LOAD( "a-s8.bin", 0x04000, 0x2000, 0xbdb591db )

	ROM_LOAD( "a-j5.bin", 0x06000, 0x4000, 0x0160bf76 )
	ROM_LOAD( "a-j6.bin", 0x0a000, 0x4000, 0x0758a878 )
	ROM_LOAD( "a-j7.bin", 0x0e000, 0x4000, 0x2b2f8ed3 )
	ROM_LOAD( "a-j8.bin", 0x12000, 0x4000, 0xe5da76b0 )

	ROM_LOAD( "b-c7.bin", 0x16000, 0x2000, 0x9266049c )
	ROM_LOAD( "b-d7.bin", 0x18000, 0x2000, 0x7f9d2741 )
	ROM_LOAD( "b-f7.bin", 0x1a000, 0x2000, 0x62735677 )

	ROM_REGION(0x10000)	/* 64k for sound board */
	ROM_LOAD( "a-e3.bin", 0x0000, 0x2000, 0x1b9c25e0 )
ROM_END



struct GameDriver pbaction_driver =
{
	"Pinball Action",
	"pbaction",
	"Nicola Salmoria",
	&machine_driver,

	pbaction_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
