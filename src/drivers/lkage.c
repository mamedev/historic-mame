/***************************************************************************

Legend of Kage
(C)1985 Taito
CPU: Z80 (x2), MC68705
Sound: YM2203 (x2)

Phil Stroffolino
phil@maya.com

status: playable

Known issues:

BACKGROUND LAYERS:
	The main CPU's Z80 ports seem to serve as a window to a 256 byte banked area.
	This data (from ROM a54-03.51) is used when drawing the background layers.

	The background layers can render tiles from any of the three character
	banks:
	gfx[0]: alphanumerics (used in ending)
	gfx[1]: trees (stage1)
	gfx[2]: palace (stage4)

SOUND:
	Lots of unknown writes to the YM2203 I/O ports

MCU:
	MCU isn't hooked up, yet.
	It's a nasty little beast, hard to trace, since it does self-modifying code.

COLORS:
	There's a 512 byte prom.  Every odd nibble is 0.
	Is there a missing 256 byte prom for the third color component?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"
#include "machine/6821pia.h"

static int bank_select = 0;

int lkage_video_reg_r( int offset );
void lkage_video_reg_w( int offset, int data );

/***************************************************************************/

static unsigned char video_reg[6];
/*
	video_reg[0x00]: unknown (always 0xf7)
	video_reg[0x01]: unknown (always 0x00)
	video_reg[0x02]: foreground layer horizontal scroll
	video_reg[0x03]: foreground layer vertical scroll
	video_reg[0x04]: background layer horizontal scroll
	video_reg[0x05]: background layer vertical scroll
*/

int lkage_video_reg_r( int offset ){
	return video_reg[offset];
}

void lkage_video_reg_w( int offset, int data ){
	video_reg[offset] = data;
}

int lkage_vh_start(void){
	return 0;
}

void lkage_vh_stop(void){
}

static void draw_background( struct osd_bitmap *bitmap ){
	unsigned char *source = videoram + 0x800;
	int xscroll = -video_reg[4]-8;
	int yscroll = -video_reg[5]-8;
	int sx,sy;

	static int bank = 2;
	if( osd_key_pressed( OSD_KEY_F ) ) bank = 0;
	if( osd_key_pressed( OSD_KEY_G ) ) bank = 1;
	if( osd_key_pressed( OSD_KEY_H ) ) bank = 2;

	for( sy=0; sy<256; sy+=8 ){
		for( sx=0; sx<256; sx+=8 ){
			drawgfx( bitmap,Machine->gfx[bank],
				source[0],
				0, /* color */
				0,0, /* flip */
				(sx+xscroll)&0xff,(sy+yscroll)&0xff,
				&Machine->drv->visible_area,
				TRANSPARENCY_NONE,0 );
			source++;
		}
	}
}

static void draw_foreground( struct osd_bitmap *bitmap ){
	unsigned char *source = videoram + 0x400;

	int xscroll = -video_reg[2]-8;
	int yscroll = -video_reg[3]-8;
	int sx,sy;

	static int bank = 1;
	if( osd_key_pressed( OSD_KEY_J ) ) bank = 0;
	if( osd_key_pressed( OSD_KEY_K ) ) bank = 1;
	if( osd_key_pressed( OSD_KEY_L ) ) bank = 2;

	for( sy=0; sy<256; sy+=8 ){
		for( sx=0; sx<256; sx+=8 ){
			drawgfx( bitmap,Machine->gfx[bank],
				source[0],
				0, /* color */
				0,0, /* flip */
				(sx+xscroll)&0xff,(sy+yscroll)&0xff,
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN,0 );
			source++;
		}
	}
}

static void draw_sprites( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned char *finish = spriteram;
	const unsigned char *source = spriteram+0x60-4;
	const struct GfxElement *gfx = Machine->gfx[3];

	while( source>=finish ){
		int attributes = source[2];
		/* bit 0: horizontal flip */
		/* bit 1: vertical flip */
		/* bit 2: bank select */
		/* bit 3: sprite size */

		int sy = 240-source[1];
		int sx = source[0] - 16;

		int sprite_number = source[3];
		if( attributes&0x04 )
			sprite_number += 128;
		else
			sprite_number += 256;

		if( sprite_number!=256 ){ /* enable */
			if( attributes&0x02 ){ /* vertical flip */
				if( attributes&0x08 ){ /* tall sprite */
					sy -= 16;
					drawgfx( bitmap,gfx,
						sprite_number^1,
						0,
						attributes&1,1, /* flip */
						sx,sy+16,
						clip,
						TRANSPARENCY_PEN,0 );
				}
				drawgfx( bitmap,gfx,
					sprite_number,
					0,
					attributes&1,1, /* flip */
					sx,sy,
					clip,
					TRANSPARENCY_PEN,0 );
			}
			else {
				if( attributes&0x08 ){ /* tall sprite */
					drawgfx( bitmap,gfx,
						sprite_number^1,
						0,
						attributes&1,0, /* flip */
						sx,sy-16,
						clip,
						TRANSPARENCY_PEN,0 );
				}
				drawgfx( bitmap,gfx,
					sprite_number,
					0,
					attributes&1,0, /* flip */
					sx,sy,
					clip,
					TRANSPARENCY_PEN,0 );
			}
		}
		source-=4;
	}
}

static void draw_text( struct osd_bitmap *bitmap ){
	const struct GfxElement *gfx = Machine->gfx[0];
	const unsigned char *source = videoram;
	int sx,sy;

	for( sy=0; sy<256; sy+=8 ){
		for( sx=0; sx<256; sx+=8 ){
			drawgfx( bitmap,gfx,
				source[0],
				0, /* color */
				0,0, /* flip */
				sx,sy,
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN,0 );
			source++;
		}
	}
}

void lkage_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	if( bank_select>0 && osd_key_pressed( OSD_KEY_Z ) ){
		while( osd_key_pressed( OSD_KEY_Z ) );
		bank_select--;
	}
	if( osd_key_pressed( OSD_KEY_X ) ){
		while( osd_key_pressed( OSD_KEY_X ) );
		bank_select++;
	}

	draw_background( bitmap );
	draw_foreground( bitmap );
	draw_sprites( bitmap );
	draw_text( bitmap );

	{
		unsigned char value = bank_select;
		static unsigned char digit[16] = "0123456789abcdef";

		drawgfx( bitmap,Machine->uifont,
			digit[value>>4],
			0, /* color */
			0,0, /* flip */
			32,8+32,
			&Machine->drv->visible_area,
			TRANSPARENCY_NONE,0 );
		drawgfx( bitmap,Machine->uifont,
			digit[value&0xf],
			0, /* color */
			0,0,
			32+8,8+32,
			&Machine->drv->visible_area,
			TRANSPARENCY_NONE,0 );
	}
}

/***************************************************************************/

static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpu_cause_interrupt(1,Z80_NMI_INT);
	else pending_nmi = 1;
}

void lkage_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	timer_set(TIME_NOW,data,nmi_callback);
}

void lkage_sh_nmi_disable_w(int offset,int data)
{
	sound_nmi_enable = 0;
}

void lkage_sh_nmi_enable_w(int offset,int data)
{
	sound_nmi_enable = 1;
	if (pending_nmi)	/* probably wrong but commands may go lost otherwise */
	{
		cpu_cause_interrupt(1,Z80_NMI_INT);
		pending_nmi = 0;
	}
}

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,          /* 2 chips */
	4000000,    /* 4 MHz ? (hand tuned) */
	{ YM2203_VOL(25,25), YM2203_VOL(25,25) },
	AY8910_DEFAULT_GAIN,
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

/***************************************************************************/

static int status_r(int offset){
	return 0x3;
}
static int unknown1_r( int offset ){
	return 0xff;
}
static int unknown0_r( int offset ){
	return 0x00;
}

static struct MemoryReadAddress readmem[] = {
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_RAM },

	{ 0xf000, 0xf003, unknown0_r },		/* PIA#1 */
	{ 0xf062, 0xf062, unknown0_r },

	/* PIA#2? */
	{ 0xf080, 0xf080, input_port_0_r }, /* DSW1 */
	{ 0xf081, 0xf081, input_port_1_r }, /* DSW2 (coinage) */
	{ 0xf082, 0xf082, input_port_2_r }, /* DSW3 */
	{ 0xf083, 0xf083, input_port_3_r },	/* start buttons, insert coin, tilt */

	/* PIA#3? */
	{ 0xf084, 0xf084, input_port_4_r },	/* P1 controls */
	{ 0xf084, 0xf084, input_port_5_r },	/* P2 controls */
	{ 0xf087, 0xf087, status_r },

	{ 0xf0a3, 0xf0a3, unknown0_r }, /* unknown */
	{ 0xf0c0, 0xf0c4, lkage_video_reg_r },

	{ 0xf100, 0xf15f, MRA_RAM, &spriteram },
	{ 0xf400, 0xffff, MRA_RAM, &videoram },
	{ -1 }
};

static struct MemoryWriteAddress writemem[] = {
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_RAM },

	{ 0xf000, 0xf003, MWA_RAM },
	{ 0xf060, 0xf060, lkage_sound_command_w },
	{ 0xf061, 0xf062, MWA_NOP },

	{ 0xf0a3, 0xf0a3, MWA_NOP }, /* unknown */

	{ 0xf0c0, 0xf0c5, lkage_video_reg_w },

//	{ 0xf0e1, 0xf0e1, MWA_NOP }, /* unknown */

	{ 0xf100, 0xf15f, MWA_RAM }, /* spriteram */
	{ 0xf400, 0xffff, MWA_RAM }, /* videoram */
	{ -1 }
};

static int port_fetch_r( int offset ){
	return Machine->memory_region[0][0x10000+(bank_select&0x3f)*0x100+offset];
}

static struct IOReadPort readport[] = {
	{ 0x00, 0xff, port_fetch_r },
	{ -1 }
};

static struct IOWritePort writeport[] = {
	{ -1 }
};

static struct MemoryReadAddress m68705_readmem[] = {
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress m68705_writemem[] = {
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
	{ -1 }
};

/***************************************************************************/

/* sound section is almost identical to Bubble Bobble, YM2203 instead of YM3526 */

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa000, 0xa000, YM2203_status_port_1_r },
	{ 0xb000, 0xb000, soundlatch_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* ??? */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
	{ -1 }
};

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0xb000, 0xb000, MWA_NOP },	/* ??? */
	{ 0xb001, 0xb001, lkage_sh_nmi_enable_w },
	{ 0xb002, 0xb002, lkage_sh_nmi_disable_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
	{ -1 }
};

/***************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" ) /* unconfirmed */
	PORT_DIPSETTING(    0x02, "15000" ) /* unconfirmed */
	PORT_DIPSETTING(    0x01, "20000" ) /* unconfirmed */
	PORT_DIPSETTING(    0x00, "24000" ) /* unconfirmed */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown DSW A 6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) ) /* unconfirmed */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) ) /* unconfirmed */
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easiest" )	/* unconfirmed */
	PORT_DIPSETTING(    0x02, "Easy" ) 		/* unconfirmed */
	PORT_DIPSETTING(    0x01, "Normal" )	/* unconfirmed */
	PORT_DIPSETTING(    0x00, "Hard" )		/* unconfirmed */
	PORT_DIPNAME( 0x04, 0x04, "Unknown DSW C 3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown DSW C 4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x20, "Roman Numerals" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME /*| IPF_CHEAT*/, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,	IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,	IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

static struct GfxLayout char_layout = {
	8,8,	/* 8x8 characters */
	256,	/* number of characters */
	4,		/* 4 bits per pixel */
	{ 0*0x20000,1*0x20000,2*0x20000,3*0x20000 },
	{ 7,6,5,4,3,2,1,0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	64 /* offset to next character */
};

static struct GfxLayout sprite_layout = {
	16,16,	/* sprite size */
	384,	/* number of sprites */
	4,		/* bits per pixel */
	{ 0*0x20000,1*0x20000,2*0x20000,3*0x20000 }, /* plane offsets */
	{ /* x offsets */
		7,6,5,4,3,2,1,0,
		64+7,64+6,64+5,64+4,64+3,64+2,64+1,64
	},
	{ /* y offsets */
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		128+0*8, 128+1*8, 128+2*8, 128+3*8, 128+4*8, 128+5*8, 128+6*8, 128+7*8 },
	256 /* offset to next sprite */
};

static struct GfxLayout sprite_layout2 = {
	16,16,	/* sprite size */
	192,	/* number of sprites */
	4,		/* bits per pixel */
	{ 0*0x20000,1*0x20000,2*0x20000,3*0x20000 }, /* plane offsets */
	{ /* x offsets */
		7,6,5,4,3,2,1,0,
		64+7,64+6,64+5,64+4,64+3,64+2,64+1,64
	},
	{ /* y offsets */
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		128+0*8, 128+1*8, 128+2*8, 128+3*8, 128+4*8, 128+5*8, 128+6*8, 128+7*8 },
	256 /* offset to next sprite */
};
static struct GfxLayout sprite_layout3 = {
	16,16,	/* sprite size */
	128,	/* number of sprites */
	4,		/* bits per pixel */
	{ 0*0x20000,1*0x20000,2*0x20000,3*0x20000 }, /* plane offsets */
	{ /* x offsets */
		7,6,5,4,3,2,1,0,
		64+7,64+6,64+5,64+4,64+3,64+2,64+1,64
	},
	{ /* y offsets */
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		128+0*8, 128+1*8, 128+2*8, 128+3*8, 128+4*8, 128+5*8, 128+6*8, 128+7*8 },
	256 /* offset to next sprite */
};

static struct GfxDecodeInfo gfxdecodeinfo[] = {
	{ 3, 0x0000, 	&char_layout,		0, 256 },
	{ 3, 0x0800, 	&char_layout,		0, 256 },
	{ 3, 0x0800*5,	&char_layout,		0, 256 },
	{ 3, 0x1000, 	&sprite_layout,		0, 256 },
	{ 3, 0x1000, 	&sprite_layout2,		0, 256 },
	{ 3, 0x0800*6, 	&sprite_layout3,		0, 256 },
	{ -1 }
};

static struct MachineDriver machine_driver = {
	{
		{
			CPU_Z80,
			6000000,	/* ??? */
			0,
			readmem,writemem,readport,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			6000000,	/* ??? */
			1,
			readmem_sound,writemem_sound,0,0,
			ignore_interrupt,0	/* NMIs are triggered by the main CPU */
								/* IRQs are triggered by the YM2203 */
		},
#if 0
		{
			CPU_M68705,
			4000000/2,	/* ??? */
			2,
			m68705_readmem,m68705_writemem,0,0,
			ignore_interrupt,1
		},
#endif
	},
	60,DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100, /* heavy synching between the MCU and main CPU */
	0, /* init machine */

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	1024,1024,
	0,
	VIDEO_TYPE_RASTER,
	0,
	lkage_vh_start,
	lkage_vh_stop,
	lkage_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

ROM_START( lkage_rom )
	ROM_REGION( 0x14000 ) /* Z80 code (main CPU) */
	ROM_LOAD( "a54-01-1.37",	0x00000, 0x8000, 0x973da9c5 )
	ROM_LOAD( "a54-02-1.38",	0x08000, 0x8000, 0x27b509da )

	ROM_LOAD( "a54-03.51",		0x10000, 0x4000, 0x493e76d8 ) /* data */

	ROM_REGION( 0x10000 ) /* Z80 code (sound CPU) */
	ROM_LOAD( "a54-04.54",		0x0000, 0x8000, 0x541faf9a )

	ROM_REGION( 0x10000 )	/* 68705 MCU code */
	ROM_LOAD( "a54-09.53",      0x0000, 0x0800, 0x0e8b8846 )

	ROM_REGION_DISPOSE( 0x10000 ) /* GFX: tiles & sprites */
	ROM_LOAD( "a54-05-1.84",	0x00000, 0x4000, 0x0033c06a )
	ROM_LOAD( "a54-06-1.85",	0x04000, 0x4000, 0x9f04d9ad )
	ROM_LOAD( "a54-07-1.86",	0x08000, 0x4000, 0xb20561a4 )
	ROM_LOAD( "a54-08-1.87",	0x0c000, 0x4000, 0x3ff3b230 )

	ROM_REGION( 512 ) /* color prom? */
	ROM_LOAD( "a54-10.prm", 0, 512, 0x17dfbd14 )
ROM_END

struct GameDriver lkage_driver = {
	__FILE__,
	0,
	"lkage",
	"The Legend of Kage",
	"1984",
	"Taito",
	"Phil Stroffolino",
	0,
	&machine_driver,
		0,

	lkage_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
