/***************************************************************************

Loco-Motion memory map (preliminary)

CPU #1
0000-4fff ROM (empty space at 5000-5fff for diagnostic ROM)
8040-83ff video RAM (top 8 rows of screen)
8440-87ff video RAM
8840-8bff color RAM (top 8 rows of screen)
8c40-8fff color RAM
8000-803f sprite RAM #1
8800-883f sprite RAM #2
9800-9fff RAM

read:
a000      IN0
a080      IN1
a100      IN2
a180      DSW

write:
a080      watchdog reset
a100      command for the audio CPU
a180      interrupt trigger on audio CPU
a181      interrupt enable


CPU #2:
2000-23ff RAM

read:
4000      8910 #0 read
6000      8910 #1 read

write:
5000      8910 #0 control
4000      8910 #0 write
7000      8910 #1 control
6000      8910 #1 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *rallyx_videoram2,*rallyx_colorram2;
extern unsigned char *rallyx_radarx,*rallyx_radary,*rallyx_radarattr;
extern int rallyx_radarram_size;
extern unsigned char *rallyx_scrollx,*rallyx_scrolly;
void rallyx_videoram2_w(int offset,int data);
void rallyx_colorram2_w(int offset,int data);
void rallyx_flipscreen_w(int offset,int data);
void rallyx_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int rallyx_vh_start(void);
void rallyx_vh_stop(void);
void locomotn_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void jungler_init(void);
void rallyx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void commsega_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* defined in sndhrdw/timeplt.c */
extern struct MemoryReadAddress timeplt_sound_readmem[];
extern struct MemoryWriteAddress timeplt_sound_writemem[];
extern struct AY8910interface timeplt_ay8910_interface;
void timeplt_sh_irqtrigger_w(int offset,int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa080, 0xa080, input_port_1_r },	/* IN1 */
	{ 0xa100, 0xa100, input_port_2_r },	/* IN2 */
	{ 0xa180, 0xa180, input_port_3_r },	/* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress jungler_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa034, 0xa03f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa185, osd_led_w },
	{ 0xa186, 0xa186, MWA_NOP },
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radary },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa00f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa185, osd_led_w },
	{ 0xa186, 0xa186, MWA_NOP },
	{ 0x8000, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8800, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8020, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8820, 0x883f, MWA_RAM, &rallyx_radary },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( locomotn_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_BITX(0,  0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Disabled" )
INPUT_PORTS_END


INPUT_PORTS_START( jungler_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( commsega_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters (256 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8+0,8*8+1,8*8+2,8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites (64 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout dotlayout =
{
	4,4,	/* 4*4 characters */
	8,	/* 8 characters */
	2,	/* 2 bits per pixel */
	{ 6, 7 },
	{ 3*8, 2*8, 1*8, 0*8 },
	{ 3*32, 2*32, 1*32, 0*32 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,        0, 64 },
	{ 1, 0x0000, &spritelayout,      0, 64 },
	{ 1, 0x2000, &dotlayout,      64*4,  1 },
	{ -1 } /* end of array */
};



#define MACHINE_DRIVER(GAMENAME)   \
																	\
static struct MachineDriver GAMENAME##_machine_driver =             \
{                                                                   \
	/* basic machine hardware */                                    \
	{                                                               \
		{                                                           \
			CPU_Z80,                                                \
			3072000,	/* 3.072 Mhz ? */                           \
			0,                                                      \
			readmem,GAMENAME##_writemem,0,0,                        \
			nmi_interrupt,1                                         \
		},                                                          \
		{                                                           \
			CPU_Z80 | CPU_AUDIO_CPU,                                \
			14318180/8,	/* 1.789772727 MHz */						\
			3,	/* memory region #3 */                              \
			timeplt_sound_readmem,timeplt_sound_writemem,0,0,       \
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */ \
		}                                                           \
	},                                                              \
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */ \
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	0,                                                              \
                                                                    \
	/* video hardware */                                            \
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },                       \
	gfxdecodeinfo,                                                  \
	32,64*4+4,                                                      \
	rallyx_vh_convert_color_prom,                                   \
                                                                    \
	VIDEO_TYPE_RASTER,                                              \
	0,                                                              \
	rallyx_vh_start,                                                \
	rallyx_vh_stop,                                                 \
	GAMENAME##_vh_screenrefresh,                                    \
                                                                    \
	/* sound hardware */                                            \
	0,0,0,0,                                                        \
	{                                                               \
		{                                                           \
			SOUND_AY8910,                                           \
			&timeplt_ay8910_interface                               \
		}                                                           \
	}                                                               \
};

#define locomotn_writemem writemem
#define commsega_writemem writemem
#define jungler_vh_screenrefresh rallyx_vh_screenrefresh
MACHINE_DRIVER(locomotn)
MACHINE_DRIVER(jungler)
MACHINE_DRIVER(commsega)


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( locomotn_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1a.cpu",       0x0000, 0x1000, 0xb43e689a )
	ROM_LOAD( "2a.cpu",       0x1000, 0x1000, 0x529c823d )
	ROM_LOAD( "3.cpu",        0x2000, 0x1000, 0xc9dbfbd1 )
	ROM_LOAD( "4.cpu",        0x3000, 0x1000, 0xcaf6431c )
	ROM_LOAD( "5.cpu",        0x4000, 0x1000, 0x64cf8dd6 )

	ROM_REGION_DISPOSE(0x2100)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c1.cpu",       0x0000, 0x1000, 0x5732eda9 )
	ROM_LOAD( "c2.cpu",       0x1000, 0x1000, 0xc3035300 )
	ROM_LOAD( "5.bpr",        0x2000, 0x0100, BADCRC( 0x21fb583f ) ) /* dots */

	ROM_REGION(0x0160)	/* PROMs */
	ROM_LOAD( "8b.cpu",       0x0000, 0x0020, 0x75b05da0 ) /* palette */
	ROM_LOAD( "9d.cpu",       0x0020, 0x0100, 0xaa6cf063 ) /* loookup table */
	ROM_LOAD( "1.bpr",        0x0120, 0x0020, 0x48c8f094 ) /* ?? */
	ROM_LOAD( "4.bpr",        0x0140, 0x0020, 0xb8861096 ) /* ?? */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "s1.snd",       0x0000, 0x1000, 0xa1105714 )
ROM_END

ROM_START( cottong_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "c1",           0x0000, 0x1000, 0x2c256fe6 )
	ROM_LOAD( "c2",           0x1000, 0x1000, 0x1de5e6a0 )
	ROM_LOAD( "c3",           0x2000, 0x1000, 0x01f909fe )
	ROM_LOAD( "c4",           0x3000, 0x1000, 0xa89eb3e3 )

	ROM_REGION_DISPOSE(0x2100) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c5",           0x0000, 0x1000, 0x992d079c )
	ROM_LOAD( "c6",           0x1000, 0x1000, 0x0149ef46 )
	ROM_LOAD( "5.bpr",        0x2000, 0x0100, 0x21fb583f ) /* dots */

	ROM_REGION(0x0160)	/* PROMs */
	ROM_LOAD( "2.bpr",        0x0000, 0x0020, 0x26f42e6f ) /* palette */
	ROM_LOAD( "3.bpr",        0x0020, 0x0100, 0x4aecc0c8 ) /* loookup table */
	ROM_LOAD( "1.bpr",        0x0120, 0x0020, 0x48c8f094 ) /* ?? */
	ROM_LOAD( "4.bpr",        0x0140, 0x0020, 0xb8861096 ) /* ?? */

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "c7",           0x0000, 0x1000, 0x3d83f6d3 )
	ROM_LOAD( "c8",           0x1000, 0x1000, 0x323e1937 )
ROM_END

ROM_START( jungler_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jungr1",       0x0000, 0x1000, 0x5bd6ad15 )
	ROM_LOAD( "jungr2",       0x1000, 0x1000, 0xdc99f1e3 )
	ROM_LOAD( "jungr3",       0x2000, 0x1000, 0x3dcc03da )
	ROM_LOAD( "jungr4",       0x3000, 0x1000, 0xf92e9940 )

	ROM_REGION_DISPOSE(0x2100)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5k",           0x0000, 0x0800, 0x924262bf )
	ROM_LOAD( "5m",           0x0800, 0x0800, 0x131a08ac )
	/* 1000-1fff empty for my convenience */
	ROM_LOAD( "82s129.10g",   0x2000, 0x0100, 0x2ef89356 ) /* dots */

	ROM_REGION(0x0160)	/* PROMs */
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, 0x55a7e6d1 ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, 0xd223f7b8 ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, 0x8f574815 ) /* ?? */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, 0xb8861096 ) /* ?? */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, 0xf86999c3 )
ROM_END

ROM_START( junglers_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "5c",           0x0000, 0x1000, 0xedd71b28 )
	ROM_LOAD( "5a",           0x1000, 0x1000, 0x61ea4d46 )
	ROM_LOAD( "4d",           0x2000, 0x1000, 0x557c7925 )
	ROM_LOAD( "4c",           0x3000, 0x1000, 0x51aac9a5 )

	ROM_REGION_DISPOSE(0x2100)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5k",           0x0000, 0x0800, 0x924262bf )
	ROM_LOAD( "5m",           0x0800, 0x0800, 0x131a08ac )
	/* 1000-1fff empty for my convenience */
	ROM_LOAD( "82s129.10g",   0x2000, 0x0100, 0x2ef89356 ) /* dots */

	ROM_REGION(0x0160)	/* PROMs */
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, 0x55a7e6d1 ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, 0xd223f7b8 ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, 0x8f574815 ) /* ?? */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, 0xb8861096 ) /* ?? */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, 0xf86999c3 )
ROM_END

ROM_START( commsega_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "csega1",       0x0000, 0x1000, 0x92de3405 )
	ROM_LOAD( "csega2",       0x1000, 0x1000, 0xf14e2f9a )
	ROM_LOAD( "csega3",       0x2000, 0x1000, 0x941dbf48 )
	ROM_LOAD( "csega4",       0x3000, 0x1000, 0xe0ac69b4 )
	ROM_LOAD( "csega5",       0x4000, 0x1000, 0xbc56ebd0 )

	ROM_REGION_DISPOSE(0x2100) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "csega7",       0x0000, 0x1000, 0xe8e374f9 )
	ROM_LOAD( "csega6",       0x1000, 0x1000, 0xcf07fd5e )
	ROM_LOAD( "gg3.bpr",      0x2000, 0x0100, 0xae7fd962 ) /* dots */

	ROM_REGION(0x0160)	/* PROMs */
	ROM_LOAD( "gg1.bpr",      0x0000, 0x0020, 0xf69e585a ) /* palette */
	ROM_LOAD( "gg2.bpr",      0x0020, 0x0100, 0x0b756e30 ) /* loookup table */
	ROM_LOAD( "gg0.bpr",      0x0120, 0x0020, 0x48c8f094 ) /* ?? */
	ROM_LOAD( "tt3.bpr",      0x0140, 0x0020, 0xb8861096 ) /* ?? */

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "csega8",       0x0000, 0x1000, 0x588b4210 )
ROM_END



static int locomotn_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x9f00],"\x00\x00\x01",3) == 0 &&
	    memcmp(&RAM[0x99c6],"\x00\x00\x01",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x9f00],12*10);
			RAM[0x99c6] = RAM[0x9f00];
			RAM[0x99c7] = RAM[0x9f01];
			RAM[0x99c8] = RAM[0x9f02];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void locomotn_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x9f00],12*10);
		osd_fclose(f);
	}
}

static int jungler_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x991c],"\x00\x00\x02",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x9940],16*10);
			RAM[0x991c] = RAM[0x9940];
			RAM[0x991d] = RAM[0x9941];
			RAM[0x991e] = RAM[0x9942];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void jungler_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x9940],16*10);
		osd_fclose(f);
	}
}

static int commsega_hiload(void)
{
	static int firsttime = 0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (firsttime == 0)
	{

		memset(&RAM[0x9c6d],0xff,6);	/* high score */
		firsttime = 1;
	}


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x9c6d],"\x00\x00\x00\x00\x00\x00",6) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x9c6d],6);
			osd_fclose(f);
			firsttime =0;
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void commsega_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x9c6d],6);
		osd_fclose(f);
	}
}



struct GameDriver locomotn_driver =
{
	__FILE__,
	0,
	"locomotn",
	"Loco-Motion",
	"1982",
	"Konami (Centuri license)",
	"Nicola Salmoria\nKevin Klopp (color info)",
	0,
	&locomotn_machine_driver,
	0,

	locomotn_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	locomotn_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	locomotn_hiload, locomotn_hisave
};

struct GameDriver cottong_driver =
{
	__FILE__,
	&locomotn_driver,
	"cottong",
	"Cotocoto Cottong",
	"1982",
	"bootleg",
	"Nicola Salmoria\nKevin Klopp (color info)",
	0,
	&locomotn_machine_driver,
	0,

	cottong_rom,
	0, 0,
	0,
	0, /* sound_prom */

	locomotn_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	locomotn_hiload, locomotn_hisave
};

struct GameDriver jungler_driver =
{
	__FILE__,
	0,
	"jungler",
	"Jungler",
	"1981",
	"Konami",
	"Nicola Salmoria",
	0,
	&jungler_machine_driver,
	jungler_init,

	jungler_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	jungler_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	jungler_hiload, jungler_hisave
};

struct GameDriver junglers_driver =
{
	__FILE__,
	&jungler_driver,
	"junglers",
	"Jungler (Stern)",
	"1981",
	"[Konami] (Stern license)",
	"Nicola Salmoria",
	0,
	&jungler_machine_driver,
	jungler_init,

	junglers_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	jungler_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	jungler_hiload, jungler_hisave
};

struct GameDriver commsega_driver =
{
	__FILE__,
	0,
	"commsega",
	"Commando (Sega)",
	"1983",
	"Sega",
	"Nicola Salmoria\nBrad Oliver",
	0,
	&commsega_machine_driver,
	0,

	commsega_rom,
	0, 0,
	0,
	0, /* sound_prom */

	commsega_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	commsega_hiload, commsega_hisave
};
