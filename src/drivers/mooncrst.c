/***************************************************************************

Main clock: XTAL = 18.432 MHz
Z80 Clock: XTAL/6 = 3.072 MHz
Horizontal video frequency: HSYNC = XTAL/3/192/2 = 16 kHz
Video frequency: VSYNC = HSYNC/132/2 = 60.606060 Hz
VBlank duration: 1/VSYNC * (20/132) = 2500 us

Changes:
19 Feb 98 LBO
	* Added Checkman driver

TODO:
	* Need valid color prom for Fantazia. Current one is slightly damaged.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "Z80/Z80.h"

extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void galaxian_flipx_w(int offset,int data);
void galaxian_flipy_w(int offset,int data);
void galaxian_attributes_w(int offset,int data);
void galaxian_stars_w(int offset,int data);
void mooncrst_gfxextend_w(int offset,int data);
int galaxian_vh_start(void);
int moonqsr_vh_start(void);
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int galaxian_vh_interrupt(void);

void mooncrst_pitch_w(int offset,int data);
void mooncrst_vol_w(int offset,int data);
void mooncrst_noise_w(int offset,int data);
void mooncrst_background_w(int offset,int data);
void mooncrst_shoot_w(int offset,int data);
void mooncrst_lfo_freq_w(int offset,int data);
int mooncrst_sh_start(void);
void mooncrst_sh_stop(void);
void mooncrst_sh_update(void);



/* Send sound data to the sound cpu and cause an nmi */
void checkman_sound_command_w (int offset, int data)
{
	soundlatch_w (0,data);
	cpu_cause_interrupt (1, Z80_NMI_INT);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW (coins per play) */
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0xa000, 0xa002, mooncrst_gfxextend_w },	/* Moon Cresta only */
	{ 0xa004, 0xa007, mooncrst_lfo_freq_w },
	{ 0xa800, 0xa800, mooncrst_background_w },
	{ 0xa803, 0xa803, mooncrst_noise_w },
	{ 0xa805, 0xa805, mooncrst_shoot_w },
	{ 0xa806, 0xa807, mooncrst_vol_w },
	{ 0xb000, 0xb000, interrupt_enable_w },	/* not Checkman */
	{ 0xb001, 0xb001, interrupt_enable_w },	/* Checkman only */
	{ 0xb004, 0xb004, galaxian_stars_w },
	{ 0xb006, 0xb006, galaxian_flipx_w },
	{ 0xb007, 0xb007, galaxian_flipy_w },
	{ 0xb800, 0xb800, mooncrst_pitch_w },
	{ -1 }	/* end of table */
};

/* EXACTLY the same as above, but no gfxextend_w to avoid graphics problems */
static struct MemoryWriteAddress moonal2_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
/*	{ 0xa000, 0xa002, mooncrst_gfxextend_w },	* Moon Cresta only */
	{ 0xa004, 0xa007, mooncrst_lfo_freq_w },
	{ 0xa800, 0xa800, mooncrst_background_w },
	{ 0xa803, 0xa803, mooncrst_noise_w },
	{ 0xa805, 0xa805, mooncrst_shoot_w },
	{ 0xa806, 0xa807, mooncrst_vol_w },
	{ 0xb000, 0xb000, interrupt_enable_w },	/* not Checkman */
	{ 0xb001, 0xb001, interrupt_enable_w },	/* Checkman only */
	{ 0xb004, 0xb004, galaxian_stars_w },
	{ 0xb006, 0xb006, galaxian_flipx_w },
	{ 0xb007, 0xb007, galaxian_flipy_w },
	{ 0xb800, 0xb800, mooncrst_pitch_w },
	{ -1 }	/* end of table */
};



/* These sound structures are only used by Checkman */
static struct IOWritePort writeport[] =
{
	{ 0, 0, checkman_sound_command_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x03, 0x03, soundlatch_r },
	{ 0x06, 0x06, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x04, 0x04, AY8910_control_port_0_w },
	{ 0x05, 0x05, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( mooncrst_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "reset" on schematics */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )	/* works only in the Gremlin version */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x40, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPNAME( 0x80, 0x80, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "English" )
	PORT_DIPSETTING(    0x00, "Japanese" )

	PORT_START	/* DSW */
 	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
 	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "Free Play" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( moonqsr_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "reset" on schematics */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "Free Play" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( checkman_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 ) /* 6 credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 ) /* 1 credit */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL ) /* p2 tiles right */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 ) /* also p1 tiles left */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 ) /* also p1 tiles right */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL ) /* p2 tiles left */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x40, "A 2/1 B 1/3" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* DSW */
 	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( moonal2_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )	/* works only in the Gremlin version */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xc0, "Free Play" )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "5000" )
	PORT_DIPSETTING(    0x03, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	3,1,	/* 3*1 line */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 2, 2, 2 },	/* I "know" that this bit of the */
	{ 0 },			/* graphics ROMs is 1 */
	0	/* no use */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};



static unsigned char mooncrst_color_prom[] =
{
	/* palette */
	0x00,0x7a,0x36,0x07,0x00,0xf0,0x38,0x1f,0x00,0xc7,0xf0,0x3f,0x00,0xdb,0xc6,0x38,
	0x00,0x36,0x07,0xf0,0x00,0x33,0x3f,0xdb,0x00,0x3f,0x57,0xc6,0x00,0xc6,0x3f,0xff
};

/* this PROM was bad (bit 3 always set). I tried to "fix" it to get more reasonable */
/* colors, but it should not be considered correct. It's a bootleg anyway. */
static unsigned char fantazia_color_prom[] =
{
	/* palette */
	0x00,0x3B,0xCB,0xFE,0x00,0x1F,0xC0,0x3F,0x00,0xD8,0x07,0x3F,0x00,0xC0,0xCC,0x07,
	0x00,0xC0,0xB8,0x1F,0x00,0x1E,0x79,0x0F,0x00,0xFE,0x07,0xF8,0x00,0x7E,0x07,0xC6
};

static unsigned char moonqsr_color_prom[] =
{
	/* palette */
	0x00,0xf6,0x79,0x4f,0x00,0xc0,0x3f,0x17,0x00,0x87,0xf8,0x7f,0x00,0xc1,0x7f,0x38,
	0x00,0x7f,0xcf,0xf9,0x00,0x57,0xb7,0xc3,0x00,0xff,0x7f,0x87,0x00,0x79,0x4f,0xff
};

static unsigned char checkman_color_prom[] =
{
	/* palette */
	0x00,0x33,0xc3,0xf6,0x00,0x17,0xc0,0x3f,0x00,0xd8,0x07,0x3f,0x00,0xc0,0xc4,0x07,
	0x00,0xc0,0xb0,0x1f,0x00,0x1e,0x71,0x07,0x00,0xf6,0x07,0xf0,0x00,0x76,0x07,0xc6
};

static unsigned char moonal2_color_prom[] =
{
	0x00,0x00,0x00,0xF6,0x00,0x16,0xC0,0x3F,0x00,0xD8,0x07,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC0,0xA0,0x07,0x00,0x00,0x00,0x07,0x00,0xF6,0x07,0xF0,0x00,0x76,0x07,0xC6,
};



static struct CustomSound_interface custom_interface =
{
	mooncrst_sh_start,
	mooncrst_sh_stop,
	mooncrst_sh_update
};



static struct MachineDriver mooncrst_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};

/* identical to moooncrst, only difference is writemem */
static struct MachineDriver moonal2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,moonal2_writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};

/* identical to mooncrst, only difference is vh_start */
static struct MachineDriver moonqsr_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			0,
			readmem,writemem,0,0,
			galaxian_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	moonqsr_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
	}
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1620000,	/* 1.62 MHz */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver checkman_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			0,
			readmem,writemem,0,writeport,
			galaxian_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1620000,	/* 1.62 MHz */
			2,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			interrupt,1	/* NMIs are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	galaxian_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		},
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mooncrst_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mc1", 0x0000, 0x0800, 0x09517d4d )
	ROM_LOAD( "mc2", 0x0800, 0x0800, 0xc38036ea )
	ROM_LOAD( "mc3", 0x1000, 0x0800, 0x55850d07 )
	ROM_LOAD( "mc4", 0x1800, 0x0800, 0xce9fa607 )
	ROM_LOAD( "mc5", 0x2000, 0x0800, 0xbe597a3b )
	ROM_LOAD( "mc6", 0x2800, 0x0800, 0xccf35bef )
	ROM_LOAD( "mc7", 0x3000, 0x0800, 0x589bfa8f )
	ROM_LOAD( "mc8", 0x3800, 0x0800, 0xc2ca7a86 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mcs_b", 0x0000, 0x0800, 0xec117295 )
	ROM_LOAD( "mcs_d", 0x0800, 0x0800, 0xdfbc68ba )
	ROM_LOAD( "mcs_a", 0x1000, 0x0800, 0xc5975785 )
	ROM_LOAD( "mcs_c", 0x1800, 0x0800, 0xc1dc1cde )
ROM_END

ROM_START( mooncrsg_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "EPR194", 0x0000, 0x0800, 0x719fde2d )
	ROM_LOAD( "EPR195", 0x0800, 0x0800, 0xb592b35a )
	ROM_LOAD( "EPR196", 0x1000, 0x0800, 0xa68c9f7e )
	ROM_LOAD( "EPR197", 0x1800, 0x0800, 0xdd96a2c2 )
	ROM_LOAD( "EPR198", 0x2000, 0x0800, 0xb3df4fd5 )
	ROM_LOAD( "EPR199", 0x2800, 0x0800, 0x4b7654e0 )
	ROM_LOAD( "EPR200", 0x3000, 0x0800, 0x765799c9 )
	ROM_LOAD( "EPR201", 0x3800, 0x0800, 0xb1cd92a3 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "EPR203", 0x0000, 0x0800, 0xa27f5447 )
	ROM_LOAD( "EPR172", 0x0800, 0x0800, 0xdfbc68ba )
	ROM_LOAD( "EPR202", 0x1000, 0x0800, 0xec79cbdb )
	ROM_LOAD( "EPR171", 0x1800, 0x0800, 0xc1dc1cde )
ROM_END

ROM_START( mooncrsb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "BEPR194", 0x0000, 0x0800, 0x1c6e3b4a )
	ROM_LOAD( "BEPR195", 0x0800, 0x0800, 0xa8901964 )
	ROM_LOAD( "BEPR196", 0x1000, 0x0800, 0x3247a543 )
	ROM_LOAD( "BEPR197", 0x1800, 0x0800, 0x8e22a4b2 )
	ROM_LOAD( "BEPR198", 0x2000, 0x0800, 0x981d7a7f )
	ROM_LOAD( "BEPR199", 0x2800, 0x0800, 0x3def1bab )
	ROM_LOAD( "BEPR200", 0x3000, 0x0800, 0xb31bfacf )
	ROM_LOAD( "BEPR201", 0x3800, 0x0800, 0xe0a15117 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "BEPR203", 0x0000, 0x0800, 0xa27f5447 )
	ROM_LOAD( "BEPR172", 0x0800, 0x0800, 0xdfbc68ba )
	ROM_LOAD( "BEPR202", 0x1000, 0x0800, 0xec79cbdb )
	ROM_LOAD( "BEPR171", 0x1800, 0x0800, 0xc1dc1cde )
ROM_END

ROM_START( fantazia_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "F01.bin", 0x0000, 0x0800, 0x22693d4d )
	ROM_LOAD( "F02.bin", 0x0800, 0x0800, 0xe65a30ae )
	ROM_LOAD( "F03.bin", 0x1000, 0x0800, 0x3247a543 )
	ROM_LOAD( "F04.bin", 0x1800, 0x0800, 0x8e22a4b2 )
	ROM_LOAD( "F09.bin", 0x2000, 0x0800, 0x730c6f5a )
	ROM_LOAD( "F10.bin", 0x2800, 0x0800, 0x50694b53 )
	ROM_LOAD( "F11.bin", 0x3000, 0x0800, 0x932f1ab9 )
	ROM_LOAD( "F12.bin", 0x3800, 0x0800, 0xdc786302 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1h_1_10.bin", 0x0000, 0x0800, 0x22bd9067 )
	ROM_LOAD( "1k_2_12.bin", 0x0800, 0x0800, 0xdfbc68ba )
	ROM_LOAD( "1k_1_11.bin", 0x1000, 0x0800, 0xf93f9153 )
	ROM_LOAD( "1h_2_09.bin", 0x1800, 0x0800, 0xc1dc1cde )
ROM_END

ROM_START( eagle_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "E1", 0x0000, 0x0800, 0xdf0c83ca )
	ROM_LOAD( "E2", 0x0800, 0x0800, 0xfb3119cb )
	ROM_LOAD( "E3", 0x1000, 0x0800, 0x3247a543 )
	ROM_LOAD( "E4", 0x1800, 0x0800, 0x8e22a4b2 )
	ROM_LOAD( "E5", 0x2000, 0x0800, 0x981d7a7f )
	ROM_LOAD( "E6", 0x2800, 0x0800, 0x3de71ba3 )
	ROM_LOAD( "E7", 0x3000, 0x0800, 0xb31bfacf )
	ROM_LOAD( "E8", 0x3800, 0x0800, 0xb722917e )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "E10", 0x0000, 0x0800, 0xe31bdc41 )
	ROM_LOAD( "E12", 0x0800, 0x0200, 0x48cd3351 )
	ROM_CONTINUE(    0x0c00, 0x0200 )	/* this version of the gfx ROMs has two */
	ROM_CONTINUE(    0x0a00, 0x0200 )	/* groups of 16 sprites swapped */
	ROM_CONTINUE(    0x0e00, 0x0200 )
	ROM_LOAD( "E9",  0x1000, 0x0800, 0x6f97f19d )
	ROM_LOAD( "E11", 0x1800, 0x0200, 0x9c42def2 )
	ROM_CONTINUE(    0x1c00, 0x0200 )
	ROM_CONTINUE(    0x1a00, 0x0200 )
	ROM_CONTINUE(    0x1e00, 0x0200 )
ROM_END

ROM_START( moonqsr_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mq1", 0x0000, 0x0800, 0x158eb218 )
	ROM_LOAD( "mq2", 0x0800, 0x0800, 0x59362b9c )
	ROM_LOAD( "mq3", 0x1000, 0x0800, 0xa9e7a5e7 )
	ROM_LOAD( "mq4", 0x1800, 0x0800, 0x8cac8d0e )
	ROM_LOAD( "mq5", 0x2000, 0x0800, 0xd436f3fa )
	ROM_LOAD( "mq6", 0x2800, 0x0800, 0xd9f90a93 )
	ROM_LOAD( "mq7", 0x3000, 0x0800, 0x8ebe83a0 )
	ROM_LOAD( "mq8", 0x3800, 0x0800, 0x5faa5ffe )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mqb", 0x0000, 0x0800, 0x7603cc0b )
	ROM_LOAD( "mqd", 0x0800, 0x0800, 0x6552d98e )
	ROM_LOAD( "mqa", 0x1000, 0x0800, 0x9a9e81d6 )
	ROM_LOAD( "mqc", 0x1800, 0x0800, 0x3cf1ef43 )
ROM_END

ROM_START( checkman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cm1", 0x0000, 0x0800, 0x778f0ed3 )
	ROM_LOAD( "cm2", 0x0800, 0x0800, 0x43b09b92 )
	ROM_LOAD( "cm3", 0x1000, 0x0800, 0x7fcab522 )
	ROM_LOAD( "cm4", 0x1800, 0x0800, 0x07b3b9cd )
	ROM_LOAD( "cm5", 0x2000, 0x0800, 0x21b7b633 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cm11", 0x0000, 0x0800, 0x7473dcf9 )
	/* 0800-0fff empty */
	ROM_LOAD( "cm9",  0x1000, 0x0800, 0x6ea84040 )
	/* 1800-1fff empty */

	ROM_REGION(0x10000)	/* 64k for sound code */
	ROM_LOAD( "cm13", 0x0000, 0x0800, 0x489360c5 )
	ROM_LOAD( "cm14", 0x0800, 0x0800, 0x8c673289 )
ROM_END

ROM_START( moonal2_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "ali1",  0x0000, 0x0400, 0x1a3f23c1 )
	ROM_LOAD( "ali2",  0x0400, 0x0400, 0x9d0a00ba )
	ROM_LOAD( "ali3",  0x0800, 0x0400, 0xabd2cf90 )
	ROM_LOAD( "ali4",  0x0c00, 0x0400, 0x1a807a06 )
	ROM_LOAD( "ali5",  0x1000, 0x0400, 0xe14af8fe )
	ROM_LOAD( "ali6",  0x1400, 0x0400, 0x2b77be15 )
	ROM_LOAD( "ali7",  0x1800, 0x0400, 0x9bb5fe05 )
	ROM_LOAD( "ali8",  0x1c00, 0x0400, 0xf55e7144 )
	ROM_LOAD( "ali9",  0x2000, 0x0400, 0xe7545590 )
	ROM_LOAD( "ali10", 0x2400, 0x0400, 0x0ce64696 )
	ROM_LOAD( "ali11", 0x2800, 0x0400, 0x7abb0a83 )

	ROM_REGION(0x2000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ali13.1h", 0x0000, 0x0800, 0x879421ae )
	/* 0800-0fff empty */
	ROM_LOAD( "ali12.1k", 0x1000, 0x0800, 0xcfb52239 )
	/* 1800-1fff empty */
ROM_END

ROM_START( moonal2b_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "ali1",  0x0000, 0x0400, 0x1a3f23c1 )
	ROM_LOAD( "ali2",  0x0400, 0x0400, 0x9d0a00ba )
	ROM_LOAD( "MD-2",  0x0800, 0x0800, 0xd643a5a7 )
	ROM_LOAD( "ali5",  0x1000, 0x0400, 0xe14af8fe )
	ROM_LOAD( "ali6",  0x1400, 0x0400, 0x2b77be15 )
	ROM_LOAD( "ali7",  0x1800, 0x0400, 0x9bb5fe05 )
	ROM_LOAD( "ali8",  0x1c00, 0x0400, 0xf55e7144 )
	ROM_LOAD( "ali9",  0x2000, 0x0400, 0xe7545590 )
	ROM_LOAD( "ali10", 0x2400, 0x0400, 0x0ce64696 )
	ROM_LOAD( "MD-6",  0x2800, 0x0800, 0x225ff205 )

	ROM_REGION(0x2000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ali13.1h", 0x0000, 0x0800, 0x879421ae )
	/* 0800-0fff empty */
	ROM_LOAD( "ali12.1k", 0x1000, 0x0800, 0xcfb52239 )
	/* 1800-1fff empty */
ROM_END



static const char *mooncrst_sample_names[] =
{
	"*galaxian",
	"shot.sam",
	"death.sam",
	0	/* end of array */
};



static unsigned char evetab[] =
{
	0x00,0x01,0x06,0x07,0x40,0x41,0x46,0x47,0x08,0x09,0x0e,0x0f,0x48,0x49,0x4e,0x4f,
	0x10,0x11,0x16,0x17,0x50,0x51,0x56,0x57,0x18,0x19,0x1e,0x1f,0x58,0x59,0x5e,0x5f,
	0x60,0x61,0x66,0x67,0x20,0x21,0x26,0x27,0x68,0x69,0x6e,0x6f,0x28,0x29,0x2e,0x2f,
	0x70,0x71,0x76,0x77,0x30,0x31,0x36,0x37,0x78,0x79,0x7e,0x7f,0x38,0x39,0x3e,0x3f,
	0x04,0x05,0x02,0x03,0x44,0x45,0x42,0x43,0x0c,0x0d,0x0a,0x0b,0x4c,0x4d,0x4a,0x4b,
	0x14,0x15,0x12,0x13,0x54,0x55,0x52,0x53,0x1c,0x1d,0x1a,0x1b,0x5c,0x5d,0x5a,0x5b,
	0x64,0x65,0x62,0x63,0x24,0x25,0x22,0x23,0x6c,0x6d,0x6a,0x6b,0x2c,0x2d,0x2a,0x2b,
	0x74,0x75,0x72,0x73,0x34,0x35,0x32,0x33,0x7c,0x7d,0x7a,0x7b,0x3c,0x3d,0x3a,0x3b,
	0x80,0x81,0x86,0x87,0xc0,0xc1,0xc6,0xc7,0x88,0x89,0x8e,0x8f,0xc8,0xc9,0xce,0xcf,
	0x90,0x91,0x96,0x97,0xd0,0xd1,0xd6,0xd7,0x98,0x99,0x9e,0x9f,0xd8,0xd9,0xde,0xdf,
	0xe0,0xe1,0xe6,0xe7,0xa0,0xa1,0xa6,0xa7,0xe8,0xe9,0xee,0xef,0xa8,0xa9,0xae,0xaf,
	0xf0,0xf1,0xf6,0xf7,0xb0,0xb1,0xb6,0xb7,0xf8,0xf9,0xfe,0xff,0xb8,0xb9,0xbe,0xbf,
	0x84,0x85,0x82,0x83,0xc4,0xc5,0xc2,0xc3,0x8c,0x8d,0x8a,0x8b,0xcc,0xcd,0xca,0xcb,
	0x94,0x95,0x92,0x93,0xd4,0xd5,0xd2,0xd3,0x9c,0x9d,0x9a,0x9b,0xdc,0xdd,0xda,0xdb,
	0xe4,0xe5,0xe2,0xe3,0xa4,0xa5,0xa2,0xa3,0xec,0xed,0xea,0xeb,0xac,0xad,0xaa,0xab,
	0xf4,0xf5,0xf2,0xf3,0xb4,0xb5,0xb2,0xb3,0xfc,0xfd,0xfa,0xfb,0xbc,0xbd,0xba,0xbb
};
static unsigned char oddtab[] =
{
	0x00,0x01,0x42,0x43,0x04,0x05,0x46,0x47,0x08,0x09,0x4a,0x4b,0x0c,0x0d,0x4e,0x4f,
	0x10,0x11,0x52,0x53,0x14,0x15,0x56,0x57,0x18,0x19,0x5a,0x5b,0x1c,0x1d,0x5e,0x5f,
	0x24,0x25,0x66,0x67,0x20,0x21,0x62,0x63,0x2c,0x2d,0x6e,0x6f,0x28,0x29,0x6a,0x6b,
	0x34,0x35,0x76,0x77,0x30,0x31,0x72,0x73,0x3c,0x3d,0x7e,0x7f,0x38,0x39,0x7a,0x7b,
	0x40,0x41,0x02,0x03,0x44,0x45,0x06,0x07,0x48,0x49,0x0a,0x0b,0x4c,0x4d,0x0e,0x0f,
	0x50,0x51,0x12,0x13,0x54,0x55,0x16,0x17,0x58,0x59,0x1a,0x1b,0x5c,0x5d,0x1e,0x1f,
	0x64,0x65,0x26,0x27,0x60,0x61,0x22,0x23,0x6c,0x6d,0x2e,0x2f,0x68,0x69,0x2a,0x2b,
	0x74,0x75,0x36,0x37,0x70,0x71,0x32,0x33,0x7c,0x7d,0x3e,0x3f,0x78,0x79,0x3a,0x3b,
	0x80,0x81,0xc2,0xc3,0x84,0x85,0xc6,0xc7,0x88,0x89,0xca,0xcb,0x8c,0x8d,0xce,0xcf,
	0x90,0x91,0xd2,0xd3,0x94,0x95,0xd6,0xd7,0x98,0x99,0xda,0xdb,0x9c,0x9d,0xde,0xdf,
	0xa4,0xa5,0xe6,0xe7,0xa0,0xa1,0xe2,0xe3,0xac,0xad,0xee,0xef,0xa8,0xa9,0xea,0xeb,
	0xb4,0xb5,0xf6,0xf7,0xb0,0xb1,0xf2,0xf3,0xbc,0xbd,0xfe,0xff,0xb8,0xb9,0xfa,0xfb,
	0xc0,0xc1,0x82,0x83,0xc4,0xc5,0x86,0x87,0xc8,0xc9,0x8a,0x8b,0xcc,0xcd,0x8e,0x8f,
	0xd0,0xd1,0x92,0x93,0xd4,0xd5,0x96,0x97,0xd8,0xd9,0x9a,0x9b,0xdc,0xdd,0x9e,0x9f,
	0xe4,0xe5,0xa6,0xa7,0xe0,0xe1,0xa2,0xa3,0xec,0xed,0xae,0xaf,0xe8,0xe9,0xaa,0xab,
	0xf4,0xf5,0xb6,0xb7,0xf0,0xf1,0xb2,0xb3,0xfc,0xfd,0xbe,0xbf,0xf8,0xf9,0xba,0xbb
};

static void mooncrst_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0;A < 0x10000;A++)
	{
		if (A & 1) RAM[A] = oddtab[RAM[A]];
		else RAM[A] = evetab[RAM[A]];
	}
}

static void moonqsr_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0;A < 0x10000;A++)
	{
		if (A & 1) ROM[A] = oddtab[RAM[A]];
		else ROM[A] = evetab[RAM[A]];
	}
}

static void checkman_decode(void)
{
/*
                     Encryption Table
                     ----------------
+---+---+---+------+------+------+------+------+------+------+------+
|A2 |A1 |A0 |D7    |D6    |D5    |D4    |D3    |D2    |D1    |D0    |
+---+---+---+------+------+------+------+------+------+------+------+
| 0 | 0 | 0 |D7    |D6    |D5    |D4    |D3    |D2    |D1    |D0^^D6|
| 0 | 0 | 1 |D7    |D6    |D5    |D4    |D3    |D2    |D1^^D5|D0    |
| 0 | 1 | 0 |D7    |D6    |D5    |D4    |D3    |D2^^D4|D1^^D6|D0    |
| 0 | 1 | 1 |D7    |D6    |D5    |D4^^D2|D3    |D2    |D1    |D0^^D5|
| 1 | 0 | 0 |D7    |D6^^D4|D5^^D1|D4    |D3    |D2    |D1    |D0    |
| 1 | 0 | 1 |D7    |D6^^D0|D5^^D2|D4    |D3    |D2    |D1    |D0    |
| 1 | 1 | 0 |D7    |D6    |D5    |D4    |D3    |D2^^D0|D1    |D0    |
| 1 | 1 | 1 |D7    |D6    |D5    |D4^^D1|D3    |D2    |D1    |D0    |
+---+---+---+------+------+------+------+------+------+------+------+

For example if A2=1, A1=1 and A0=0 then D2 to the CPU would be an XOR of
D2 and D0 from the ROM's. Note that D7 and D3 are not encrypted.

Encryption PAL 16L8 on cardridge
         +--- ---+
    OE --|   U   |-- VCC
 ROMD0 --|       |-- D0
 ROMD1 --|       |-- D1
 ROMD2 --|VER 5.2|-- D2
    A0 --|       |-- NOT USED
    A1 --|       |-- A2
 ROMD4 --|       |-- D4
 ROMD5 --|       |-- D5
 ROMD6 --|       |-- D6
   GND --|       |-- M1 (NOT USED)
         +-------+
Pin layout is such that links can replace the PAL if encryption is not used.

*/
	int A;
	int data_xor=0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0;A < 0x2800;A++)
	{
		switch (A & 0x07)
		{
			case 0: data_xor = (RAM[A] & 0x40) >> 6; break;
			case 1: data_xor = (RAM[A] & 0x20) >> 4; break;
			case 2: data_xor = ((RAM[A] & 0x10) >> 2) | ((RAM[A] & 0x40) >> 5); break;
			case 3: data_xor = ((RAM[A] & 0x04) << 2) | ((RAM[A] & 0x20) >> 5); break;
			case 4: data_xor = ((RAM[A] & 0x10) << 2) | ((RAM[A] & 0x02) << 4); break;
			case 5: data_xor = ((RAM[A] & 0x01) << 6) | ((RAM[A] & 0x04) << 3); break;
			case 6: data_xor = (RAM[A] & 0x01) << 2; break;
			case 7: data_xor = (RAM[A] & 0x02) << 3; break;
		}
		RAM[A] ^= data_xor;
	}
}



static int mooncrst_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8042],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0x804e],"\x00\x50\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8042],17*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void mooncrst_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8042],17*5);
		osd_fclose(f);
	}
}

static int mooncrsg_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8045],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0x8051],"\x00\x50\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8045],17*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void mooncrsg_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8045],17*5);
		osd_fclose(f);
	}
}

static int moonqsr_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x804e],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0x805a],"\x00\x50\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x804e],10*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void moonqsr_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x804e],10*5);
		osd_fclose(f);
	}
}

static int checkman_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	/* Peek into videoram to see if the opening screen is drawn */
	if (memcmp(&RAM[0x9382],"\x10\xA0\xA1",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8020],8*9);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void checkman_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8020],8*9);
		osd_fclose(f);
	}
}



struct GameDriver mooncrst_driver =
{
	__FILE__,
	0,
	"mooncrst",
	"Moon Cresta (Nichibutsu)",
	"1980",
	"Nihon Bussan",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&mooncrst_machine_driver,

	mooncrst_rom,
	mooncrst_decode, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	mooncrst_input_ports,

	mooncrst_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	mooncrst_hiload, mooncrst_hisave
};

struct GameDriver mooncrsg_driver =
{
	__FILE__,
	&mooncrst_driver,
	"mooncrsg",
	"Moon Cresta (Gremlin)",
	"1980",
	"Gremlin",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&mooncrst_machine_driver,

	mooncrsg_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	mooncrst_input_ports,

	mooncrst_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	mooncrsg_hiload, mooncrsg_hisave
};

struct GameDriver mooncrsb_driver =
{
	__FILE__,
	&mooncrst_driver,
	"mooncrsb",
	"Moon Cresta (bootleg)",
	"1980",
	"bootleg",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&mooncrst_machine_driver,

	mooncrsb_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	mooncrst_input_ports,

	mooncrst_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	mooncrst_hiload, mooncrst_hisave
};

struct GameDriver fantazia_driver =
{
	__FILE__,
	&mooncrst_driver,
	"fantazia",
	"Fantazia",
	"1980",
	"bootleg",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&mooncrst_machine_driver,

	fantazia_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	mooncrst_input_ports,

	fantazia_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	mooncrst_hiload, mooncrst_hisave
};

struct GameDriver eagle_driver =
{
	__FILE__,
	&mooncrst_driver,
	"eagle",
	"Eagle",
	"1980",
	"Centuri",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&mooncrst_machine_driver,

	eagle_rom,
	0, 0,
	mooncrst_sample_names,
	0,	/* sound_prom */

	mooncrst_input_ports,

	mooncrst_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	mooncrst_hiload, mooncrst_hisave
};

struct GameDriver moonqsr_driver =
{
	__FILE__,
	0,
	"moonqsr",
	"Moon Quasar",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nMike Coates (decryption info)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott\nMarco Cassili",
	0,
	&moonqsr_machine_driver,

	moonqsr_rom,
	0, moonqsr_decode,
	mooncrst_sample_names,
	0,	/* sound_prom */

	moonqsr_input_ports,

	moonqsr_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	moonqsr_hiload, moonqsr_hisave
};

struct GameDriver checkman_driver =
{
	__FILE__,
	0,
	"checkman",
	"Checkman",
	"1982",
	"Zilec-Zenitone",
	"Brad Oliver (MAME driver)\nMalcolm Lear (hardware & encryption info)",
	0,
	&checkman_machine_driver,

	checkman_rom,
	checkman_decode, 0,
	0,
	0,	/* sound_prom */

	checkman_input_ports,

	checkman_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	checkman_hiload, checkman_hisave
};

struct GameDriver moonal2_driver =
{
	__FILE__,
	0,
	"moonal2",
	"Moon Alien Part 2",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAndrew Scott",
	0,
	&moonal2_machine_driver,

	moonal2_rom,
	0, 0,
	mooncrst_sample_names,
	0, /* sound_prom */

	moonal2_input_ports,

	moonal2_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver moonal2b_driver =
{
	__FILE__,
	&moonal2_driver,
	"moonal2b",
	"Moon Alien Part 2 (older version)",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAndrew Scott",
	0,
	&moonal2_machine_driver,

	moonal2b_rom,
	0, 0,
	mooncrst_sample_names,
	0, /* sound_prom */

	moonal2_input_ports,

	moonal2_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
