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

***************************************************************************

Mat Mania
Memetron, 1985
(copyright Taito, licensed by Technos, distributed by Memetron)

MAIN BOARD:

0000-0fff RAM
1000-13ff Video RAM
1400-17ff Attribute RAM
1800-1fff ?? Only used in self-test ??
2000-21ff Background video RAM #1
2200-23ff Background attribute RAM #1
2400-25ff Background video RAM #2
2600-27ff Background attribute RAM #2
4000-ffff ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "m6502/m6502.h"



int mystston_DSW2_r(int offset);
int mystston_interrupt(void);

extern unsigned char *mystston_videoram2,*mystston_colorram2;
extern int mystston_videoram2_size;
extern unsigned char *mystston_videoram3,*mystston_colorram3;
extern int mystston_videoram3_size;
extern unsigned char *mystston_scroll;
extern unsigned char *mystston_paletteram;

void mystston_paletteram_w(int offset,int data);
void mystston_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void mystston_videoram3_w(int offset,int data);
void mystston_colorram3_w(int offset,int data);
int mystston_vh_start(void);
void mystston_vh_stop(void);
void mystston_vh_screenrefresh(struct osd_bitmap *bitmap);

extern unsigned char *matmania_pageselect;
void matmania_paletteram_w(int offset,int data);
void matmania_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void matmania_vh_screenrefresh(struct osd_bitmap *bitmap);


static struct AY8910interface interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHZ?????? */
	{ 255, 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


int matmania_sh_start(void)
{
	return AY8910_sh_start(&interface);
}

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
	{ 0x2000, 0x2000, MWA_RAM, &matmania_pageselect },
//	{ 0x2010, 0x2010, sound_command_w },
	{ 0x2020, 0x2020, MWA_RAM, &mystston_scroll },
	{ 0x2060, 0x2077, mystston_paletteram_w, &mystston_paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



void matmania_sh_command_w (int offset, int data)
{
	sound_command_w (offset, data);
	cpu_cause_interrupt (1,INT_IRQ);
}

static struct MemoryReadAddress matmania_readmem[] =
{
	{ 0x0000, 0x077f, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3010, 0x3010, input_port_1_r },
	{ 0x3020, 0x3020, input_port_2_r },
	{ 0x3030, 0x3030, input_port_3_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress matmania_writemem[] =
{
	{ 0x0000, 0x077f, MWA_RAM },
	{ 0x0780, 0x07df, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1000, 0x13ff, MWA_RAM, &mystston_videoram2, &mystston_videoram2_size },
	{ 0x1400, 0x17ff, MWA_RAM, &mystston_colorram2 },
	{ 0x2000, 0x21ff, videoram_w, &videoram, &videoram_size },
	{ 0x2200, 0x23ff, colorram_w, &colorram },
	{ 0x2400, 0x25ff, mystston_videoram3_w, &mystston_videoram3, &mystston_videoram3_size },
	{ 0x2600, 0x27ff, mystston_colorram3_w, &mystston_colorram3 },
	{ 0x3000, 0x3000, MWA_RAM, &matmania_pageselect },
	{ 0x3010, 0x3010, matmania_sh_command_w },
	{ 0x3020, 0x3020, MWA_RAM, &mystston_scroll },
	{ 0x3030, 0x3030, MWA_NOP },	/* ??? */
	{ 0x3050, 0x307f, matmania_paletteram_w, &mystston_paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x2007, 0x2007, sound_command_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x2001, 0x2001, AY8910_control_port_0_w },
	{ 0x2002, 0x2002, AY8910_write_port_1_w },
	{ 0x2003, 0x2003, AY8910_control_port_1_w },
	{ 0x2004, 0x2004, AY8910_write_port_2_w },
	{ 0x2005, 0x2005, AY8910_control_port_2_w },
	{ 0x8000, 0xffff, MWA_ROM },
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

INPUT_PORTS_START( matmania_input_ports )
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
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME (0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING (   0x03, "Easy" )
	PORT_DIPSETTING (   0x02, "Medium" )
	PORT_DIPSETTING (   0x01, "Hard" )
	PORT_DIPSETTING (   0x00, "Hardest" )
	PORT_DIPNAME (0x04, 0x04, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING (   0x04, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x08, 0x08, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING (   0x08, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x10, 0x10, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING (   0x10, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x20, 0x20, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING (   0x20, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x40, 0x40, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING (   0x40, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x80, 0x80, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING (   0x80, "Off" )
	PORT_DIPSETTING (   0x00, "On" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME (0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x03, "1 Coin/1 Credits" )
	PORT_DIPSETTING (   0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME (0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x0c, "1 Coin/1 Credits" )
	PORT_DIPSETTING (   0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME (0x10, 0x10, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x10, "On" )
	PORT_DIPNAME (0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Upright" )
	PORT_DIPSETTING (   0x20, "Cocktail" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
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

static struct GfxLayout matmania_spritelayout =
{
	16,16,  /* 16*16 sprites */
	3584,    /* 3584 sprites */
	3,	/* 3 bits per pixel */
	{ 2*3584*16*16, 3584*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	512,    /* 256 tiles */
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

static struct GfxDecodeInfo matmania_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,              0, 4 },
	{ 1, 0x06000, &tilelayout,            4*8, 4 },
	{ 1, 0x12000, &matmania_spritelayout, 8*8, 2 },
	{ -1 } /* end of array */
};


static unsigned char matmania_color_prom[] =
{
	/* prom 1 - char palette red and green components */
	0x00,0x0F,0xAF,0xFF,0xA0,0xBA,0xF0,0xFF,0x00,0x4F,0xB4,0xF8,0xFF,0xD0,0xC4,0xFF,
	0x00,0xFF,0xB4,0x0F,0x00,0xF8,0xD8,0xFF,0x00,0xDF,0x00,0xFF,0x0F,0x0B,0x00,0xFF,
	/* prom 5 - tile palette red and green components */
	0x00,0x0F,0xAF,0xFF,0xA0,0xBA,0xF0,0xFF,0x00,0xAF,0x39,0x7D,0x37,0x0F,0xFF,0xFF,
	0x00,0xAF,0x39,0x7D,0x37,0xBA,0xA0,0xFF,0xDF,0xFF,0xAF,0x0B,0xA0,0xBA,0x00,0xFF,
	/* prom 2 - char palette blue component */
	0x0A,0x00,0x00,0x00,0x0E,0x0A,0x0F,0x0F,0x00,0x04,0x0F,0x08,0x00,0x00,0x0F,0x0F,
	0x00,0x00,0x0E,0x00,0x00,0x0F,0x0F,0x0F,0x00,0x0A,0x00,0x00,0x00,0x00,0x0F,0x0F,
	/* prom 16 - tile palette blue component */
	0x0A,0x00,0x00,0x00,0x0E,0x0A,0x0F,0x0F,0x0A,0x08,0x0F,0x00,0x00,0x00,0x00,0x0F,
	0x0A,0x08,0x0F,0x00,0x00,0x0A,0x0E,0x0F,0x0A,0x00,0x00,0x00,0x0E,0x0A,0x00,0x0F,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ???? */
			0,
			readmem,writemem,0,0,
			mystston_interrupt,1
		},
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	24, 3*8,
	mystston_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	mystston_vh_start,
	mystston_vh_stop,
	mystston_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};

static struct MachineDriver matmania_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ???? */
			0,
			matmania_readmem,matmania_writemem,0,0,
			mystston_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,	/* 1.5 Mhz ???? */
			2,
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,15	/* ???? */
		},
	},
	60,
	10,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	matmania_gfxdecodeinfo,
	64+16,8*8+2*8,
	matmania_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	mystston_vh_start,
	mystston_vh_stop,
	matmania_vh_screenrefresh,

	/* sound hardware */
	0,
	matmania_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
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
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

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

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0308],11*5);
		osd_fclose(f);
	}

}



struct GameDriver mystston_driver =
{
	"Mysterious Stones",
	"mystston",
	"Nicola Salmoria\nMike Balfour\nBrad Oliver",
	&machine_driver,

	mystston_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};



/***************************************************************************

  Mat Mania driver

***************************************************************************/

ROM_START( matmania_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "k0-03", 0x4000, 0x4000, 0x24f61794 )
	ROM_LOAD( "k1-03", 0x8000, 0x4000, 0x9c46fb0e )
	ROM_LOAD( "k2-03", 0xc000, 0x4000, 0x48f096d0 )

	ROM_REGION(0x66000)	/* temporary space for graphics (disposed after conversion) */
	/* Character ROMs - 1024 chars, 3 bpp */
	ROM_LOAD( "ku-02",  0x00000, 0x2000, 0xd8c27456 )
	ROM_LOAD( "kv-02",  0x02000, 0x2000, 0xea2bd99f )
	ROM_LOAD( "kw-02",  0x04000, 0x2000, 0x1bfe7c88 )
	/* tile set */
	ROM_LOAD( "kt-02",  0x06000, 0x4000, 0x37bbe291 )
	ROM_LOAD( "ks-02",  0x0a000, 0x4000, 0x716bd393 )
	ROM_LOAD( "kr-02",  0x0e000, 0x4000, 0x388a1238 )
	/* sprites */
	ROM_LOAD( "k6-00",  0x12000, 0x4000, 0xe63b9ca9 )
	ROM_LOAD( "k7-00",  0x16000, 0x4000, 0xa8d5e8d7 )
	ROM_LOAD( "k8-00",  0x1a000, 0x4000, 0x975ae136 )
	ROM_LOAD( "k9-00",  0x1e000, 0x4000, 0x65aaaea8 )
	ROM_LOAD( "ka-00",  0x22000, 0x4000, 0x37fb0e09 )
	ROM_LOAD( "kb-00",  0x26000, 0x4000, 0xc474d040 )
	ROM_LOAD( "kc-00",  0x2a000, 0x4000, 0x55a8420a )
	ROM_LOAD( "kd-00",  0x2e000, 0x4000, 0x23b32cbd )
	ROM_LOAD( "ke-00",  0x32000, 0x4000, 0xdba47f90 )
	ROM_LOAD( "kf-00",  0x36000, 0x4000, 0x1634b750 )
	ROM_LOAD( "kg-00",  0x3a000, 0x4000, 0xd21c3f64 )
	ROM_LOAD( "kh-00",  0x3e000, 0x4000, 0xe88f63a9 )
	ROM_LOAD( "ki-00",  0x42000, 0x4000, 0x3278b6ac )
	ROM_LOAD( "kj-00",  0x46000, 0x4000, 0xc3ff1da1 )
	ROM_LOAD( "kk-00",  0x4a000, 0x4000, 0x65219221 )
	ROM_LOAD( "kl-00",  0x4e000, 0x4000, 0x5ff211bc )
	ROM_LOAD( "km-00",  0x52000, 0x4000, 0x388a2acc )
	ROM_LOAD( "kn-00",  0x56000, 0x4000, 0x9c9eaba6 )
	ROM_LOAD( "ko-00",  0x5a000, 0x4000, 0x8b4989ad )
	ROM_LOAD( "kp-00",  0x5e000, 0x4000, 0x04ae3d8a )
	ROM_LOAD( "kq-00",  0x62000, 0x4000, 0x824b4449 )

	ROM_REGION(0x10000)	/* 64k for audio code */
	ROM_LOAD( "k4-0", 0x8000, 0x4000, 0x3ad35dff )
	ROM_LOAD( "k5-0", 0xc000, 0x4000, 0x1ebde7cb )
ROM_END


ROM_START( excthour_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "E29",  0x04000, 0x4000, 0xe9f18bdf )
	ROM_LOAD( "E28",  0x08000, 0x4000, 0x1884e41c )
	ROM_LOAD( "E27",  0x0c000, 0x4000, 0x0456e8ea )

	ROM_REGION(0x66000)	/* temporary space for graphics (disposed after conversion) */
	/* Character ROMs - 1024 chars, 3 bpp */
	ROM_LOAD( "E30",  0x00000, 0x2000, 0x19224262 )
	ROM_LOAD( "E31",  0x02000, 0x2000, 0x9a1121fd )
	ROM_LOAD( "E32",  0x04000, 0x2000, 0xee0cfdfa )
	/* tile set */
	ROM_LOAD( "E5",   0x06000, 0x4000, 0x269c2d5e )
	ROM_LOAD( "E4",   0x0a000, 0x4000, 0x716bd393 )
	ROM_LOAD( "E3",   0x0e000, 0x4000, 0x276bddf7 )
	/* sprites */
	ROM_LOAD( "E6",   0x12000, 0x4000, 0xe63b9ca9 )
	ROM_LOAD( "E7",   0x16000, 0x4000, 0xa8d5e8d7 )
	ROM_LOAD( "E8",   0x1a000, 0x4000, 0x975ae136 )
	ROM_LOAD( "E9",   0x1e000, 0x4000, 0x63aaaca8 )
	ROM_LOAD( "E10",  0x22000, 0x4000, 0x37fb0e09 )
	ROM_LOAD( "E11",  0x26000, 0x4000, 0xc474d040 )
	ROM_LOAD( "E12",  0x2a000, 0x4000, 0x55a8420a )
	ROM_LOAD( "E13",  0x2e000, 0x4000, 0x23b32cbd )
	ROM_LOAD( "E14",  0x32000, 0x4000, 0xdba47f90 )
	ROM_LOAD( "E15",  0x36000, 0x4000, 0x1634b750 )
	ROM_LOAD( "E16",  0x3a000, 0x4000, 0xd21c3f64 )
	ROM_LOAD( "E17",  0x3e000, 0x4000, 0xe88f63a9 )
	ROM_LOAD( "E18",  0x42000, 0x4000, 0x3278b6ac )
	ROM_LOAD( "E19",  0x46000, 0x4000, 0xc3ff1da1 )
	ROM_LOAD( "E20",  0x4a000, 0x4000, 0x65219221 )
	ROM_LOAD( "E21",  0x4e000, 0x4000, 0x5ff211bc )
	ROM_LOAD( "E22",  0x52000, 0x4000, 0x388a2acc )
	ROM_LOAD( "E23",  0x56000, 0x4000, 0x9c9eaba6 )
	ROM_LOAD( "E24",  0x5a000, 0x4000, 0x8b4989ad )
	ROM_LOAD( "E25",  0x5e000, 0x4000, 0x04ae3d8a )
	ROM_LOAD( "E26",  0x62000, 0x4000, 0x824b4449 )

	ROM_REGION(0x10000)	/* 64k for audio code */
	ROM_LOAD( "E1",   0x8000, 0x4000, 0x3ad35dff )
	ROM_LOAD( "E2",   0xc000, 0x4000, 0x1ebde7cb )
ROM_END


static int matmania_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0700],"\x00\x30\x00",3) == 0) &&
		(memcmp(&RAM[0x074d],"\xb0\xb0\xb0",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0700],16*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void matmania_hisave(void)
{
	void *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0700],16*5);
		osd_fclose(f);
	}

}


struct GameDriver matmania_driver =
{
	"Mat Mania",
	"matmania",
	"Brad Oliver (MAME driver)\nTim Lindquist (color info)",
	&matmania_machine_driver,

	matmania_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/, matmania_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	matmania_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	matmania_hiload, matmania_hisave
};


struct GameDriver excthour_driver =
{
	"Exciting Hour",
	"excthour",
	"Brad Oliver (MAME driver)\nTim Lindquist (color info)",
	&matmania_machine_driver,

	excthour_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/, matmania_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	matmania_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	matmania_hiload, matmania_hisave
};
