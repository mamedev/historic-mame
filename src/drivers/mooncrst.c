/***************************************************************************

Main clock: XTAL = 18.432 MHz
Z80 Clock: XTAL/6 = 3.072 MHz
Horizontal video frequency: HSYNC = XTAL/3/192/2 = 16 kHz
Video frequency: VSYNC = HSYNC/132/2 = 60.606060 Hz
VBlank duration: 1/VSYNC * (20/132) = 2500 us

Changes:
19 Feb 98 LBO
	* Added Checkman driver
19 Jul 98 LBO
	* Added King & Balloon driver

TODO:
	* Need valid color prom for Fantazia. Current one is slightly damaged.


Moon Cresta versions supported:
------------------------------

mooncrst    Nichibutsu - later revision with better demo mode and
						 text for docking. Encrypted. No ROM/RAM check
mooncrs2    Nichibutsu - probably first revision (no patches) and ROM/RAM check code.
                         This came from a bootleg board, with the logos erased
						 from the graphics
mooncrsg    Gremlin    - same docking text as mooncrst
mooncrsb    bootleg of mooncrs2. ROM/RAM check erased.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"



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
int mooncrst_vh_start(void);
int moonqsr_vh_start(void);
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int galaxian_vh_interrupt(void);

void mooncrst_pitch_w(int offset,int data);
void mooncrst_vol_w(int offset,int data);
void mooncrst_noise_w(int offset,int data);
void mooncrst_background_w(int offset,int data);
void mooncrst_shoot_w(int offset,int data);
void mooncrst_lfo_freq_w(int offset,int data);
int mooncrst_sh_start(const struct MachineSound *msound);
void mooncrst_sh_stop(void);
void mooncrst_sh_update(void);



/* Send sound data to the sound cpu and cause an nmi */
void checkman_sound_command_w (int offset, int data)
{
	soundlatch_w (0,data);
	cpu_cause_interrupt (1, Z80_NMI_INT);
}

static int kingball_speech_dip;

/* Hack? If $b003 is high, we'll check our "fake" speech dipswitch */
static int kingball_IN0_r (int offset)
{
	if (kingball_speech_dip)
		return (readinputport (0) & 0x80) >> 1;
	else
		return readinputport (0);
}

static void kingball_speech_dip_w (int offset, int data)
{
	kingball_speech_dip = data;
}

static int kingball_sound;

static void kingball_sound1_w (int offset, int data)
{
	kingball_sound = (kingball_sound & ~0x01) | data;
	if (errorlog) fprintf (errorlog, "kingball_sample latch: %02x (%02x)\n", kingball_sound, data);
}

static void kingball_sound2_w (int offset, int data)
{
	kingball_sound = (kingball_sound & ~0x02) | (data << 1);
	soundlatch_w (0, kingball_sound | 0xf0);
	if (errorlog) fprintf (errorlog, "kingball_sample play: %02x (%02x)\n", kingball_sound, data);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9400, 0x97ff, videoram_r },	/* Checkman - video RAM mirror */
	{ 0x9800, 0x9fff, MRA_RAM },	/* screen attributes, sprites, bullets */
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
	{ 0xa800, 0xa802, mooncrst_background_w },
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
	{ 0xa800, 0xa802, mooncrst_background_w },
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

static struct MemoryReadAddress kingball_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9400, 0x97ff, videoram_r },	/* Checkman - video RAM mirror */
	{ 0x9800, 0x9fff, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0xa000, 0xa000, kingball_IN0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW (coins per play) */
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress kingball_writemem[] =
{
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x9880, 0x98ff, MWA_RAM },
	{ 0xa000, 0xa003, MWA_NOP }, /* lamps */
	{ 0xa004, 0xa007, mooncrst_lfo_freq_w },
	{ 0xa800, 0xa802, mooncrst_background_w },
	{ 0xa803, 0xa803, mooncrst_noise_w }, //
	{ 0xa805, 0xa805, mooncrst_shoot_w }, //
	{ 0xa806, 0xa807, mooncrst_vol_w }, //
	{ 0xb001, 0xb001, interrupt_enable_w },
	{ 0xb000, 0xb000, kingball_sound1_w },
	{ 0xb002, 0xb002, kingball_sound2_w },
	{ 0xb003, 0xb003, kingball_speech_dip_w },
//	{ 0xb004, 0xb004, galaxian_stars_w },
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

/* These 4 structures are used only by King & Balloon */
static struct MemoryReadAddress kingball_sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress kingball_sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort kingball_sound_readport[] =
{
	{ 0x00, 0x00, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort kingball_sound_writeport[] =
{
	{ 0x00, 0x00, DAC_data_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( mooncrst )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "reset" on schematics */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )	/* works only in the Gremlin version */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPNAME( 0x80, 0x80, "Language" )
	PORT_DIPSETTING(    0x80, "English" )
	PORT_DIPSETTING(    0x00, "Japanese" )

	PORT_START	/* DSW */
 	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
 	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( eagle )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "reset" on schematics */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPNAME( 0x80, 0x80, "Language" )
	PORT_DIPSETTING(    0x80, "English" )
	PORT_DIPSETTING(    0x00, "Japanese" )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( eagle2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "reset" on schematics */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPNAME( 0x80, 0x80, "Language" )
	PORT_DIPSETTING(    0x80, "English" )
	PORT_DIPSETTING(    0x00, "Japanese" )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0c, 0x00, "Game Type" )
	PORT_DIPSETTING(    0x00, "Normal 1?" )
	PORT_DIPSETTING(    0x04, "Normal 2?" )
	PORT_DIPSETTING(    0x08, "Normal 3?" )
	PORT_DIPSETTING(    0x0c, DEF_STR ( Free_Play ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( moonqsr )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "reset" on schematics */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( checkman )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL ) /* p2 tiles right */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 ) /* also p1 tiles left */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 ) /* also p1 tiles right */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL ) /* p2 tiles left */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x40, "A 2/1 B 1/3" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW */
 	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPSETTING(    0x04, "200000" )
	PORT_DIPNAME( 0x08, 0x00, "Difficulty Increases At Level" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( moonal2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )	/* works only in the Gremlin version */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "5000" )
	PORT_DIPSETTING(    0x03, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( kingball )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	/* Hack? - possibly multiplexed via writes to $b003 */
	PORT_DIPNAME( 0x80, 0x80, "Speech" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x01, "12000" )
	PORT_DIPSETTING(    0x02, "15000" )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPNAME( 0xf8, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xf8, DEF_STR( On ) )
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
static struct GfxLayout backgroundlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	8,8,
	32,	/* one for each column */
	7,	/* 128 colors max */
	{ 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	8*8*8	/* each character takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 2 },
	{ 0, 0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};



static struct CustomSound_interface custom_interface =
{
	mooncrst_sh_start,
	mooncrst_sh_stop,
	mooncrst_sh_update
};



static struct MachineDriver machine_driver_mooncrst =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
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
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	mooncrst_vh_start,
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
static struct MachineDriver machine_driver_moonal2 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
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
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
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
static struct MachineDriver machine_driver_moonqsr =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
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
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
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
	{ 50 },
	AY8910_DEFAULT_GAIN,
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver machine_driver_checkman =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			readmem,writemem,0,writeport,
			galaxian_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1620000,	/* 1.62 MHz */
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
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	mooncrst_vh_start,
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

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

static struct MachineDriver machine_driver_kingball =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz? */
			kingball_readmem,kingball_writemem,0,0,
			galaxian_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2500000,	/* 2.5 MHz */
			kingball_sound_readmem,kingball_sound_writemem,
			kingball_sound_readport,kingball_sound_writeport,
			interrupt,1	/* NMIs are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
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
			SOUND_DAC,
			&dac_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mooncrst )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "mc1",          0x0000, 0x0800, 0x7d954a7a )
	ROM_LOAD( "mc2",          0x0800, 0x0800, 0x44bb7cfa )
	ROM_LOAD( "mc3",          0x1000, 0x0800, 0x9c412104 )
	ROM_LOAD( "mc4",          0x1800, 0x0800, 0x7e9b1ab5 )
	ROM_LOAD( "mc5",          0x2000, 0x0800, 0x16c759af )
	ROM_LOAD( "mc6",          0x2800, 0x0800, 0x69bcafdb )
	ROM_LOAD( "mc7",          0x3000, 0x0800, 0xb50dbc46 )
	ROM_LOAD( "mc8",          0x3800, 0x0800, 0x18ca312b )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mcs_b",        0x0000, 0x0800, 0xfb0f1f81 )
	ROM_LOAD( "mcs_d",        0x0800, 0x0800, 0x13932a15 )
	ROM_LOAD( "mcs_a",        0x1000, 0x0800, 0x631ebb5a )
	ROM_LOAD( "mcs_c",        0x1800, 0x0800, 0x24cfd145 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( mooncrsg )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "epr194",       0x0000, 0x0800, 0x0e5582b1 )
	ROM_LOAD( "epr195",       0x0800, 0x0800, 0x12cb201b )
	ROM_LOAD( "epr196",       0x1000, 0x0800, 0x18255614 )
	ROM_LOAD( "epr197",       0x1800, 0x0800, 0x05ac1466 )
	ROM_LOAD( "epr198",       0x2000, 0x0800, 0xc28a2e8f )
	ROM_LOAD( "epr199",       0x2800, 0x0800, 0x5a4571de )
	ROM_LOAD( "epr200",       0x3000, 0x0800, 0xb7c85bf1 )
	ROM_LOAD( "epr201",       0x3800, 0x0800, 0x2caba07f )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr203",       0x0000, 0x0800, 0xbe26b561 )
	ROM_LOAD( "mcs_d",        0x0800, 0x0800, 0x13932a15 )
	ROM_LOAD( "epr202",       0x1000, 0x0800, 0x26c7e800 )
	ROM_LOAD( "mcs_c",        0x1800, 0x0800, 0x24cfd145 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( smooncrs )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "927",          0x0000, 0x0800, 0x55c5b994 )
	ROM_LOAD( "928a",         0x0800, 0x0800, 0x77ae26d3 )
	ROM_LOAD( "929",          0x1000, 0x0800, 0x716eaa10 )
	ROM_LOAD( "930",          0x1800, 0x0800, 0xcea864f2 )
	ROM_LOAD( "931",          0x2000, 0x0800, 0x702c5f51 )
	ROM_LOAD( "932a",         0x2800, 0x0800, 0xe6a2039f )
	ROM_LOAD( "933",          0x3000, 0x0800, 0x73783cee )
	ROM_LOAD( "934",          0x3800, 0x0800, 0xc1a14aa2 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr203",       0x0000, 0x0800, 0xbe26b561 )
	ROM_LOAD( "mcs_d",        0x0800, 0x0800, 0x13932a15 )
	ROM_LOAD( "epr202",       0x1000, 0x0800, 0x26c7e800 )
	ROM_LOAD( "mcs_c",        0x1800, 0x0800, 0x24cfd145 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( mooncrsb )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "bepr194",      0x0000, 0x0800, 0x6a23ec6d )
	ROM_LOAD( "bepr195",      0x0800, 0x0800, 0xee262ff2 )
	ROM_LOAD( "f03.bin",      0x1000, 0x0800, 0x29a2b0ab )
	ROM_LOAD( "f04.bin",      0x1800, 0x0800, 0x4c6a5a6d )
	ROM_LOAD( "e5",           0x2000, 0x0800, 0x06d378a6 )
	ROM_LOAD( "bepr199",      0x2800, 0x0800, 0x6e84a927 )
	ROM_LOAD( "e7",           0x3000, 0x0800, 0xb45af1e8 )
	ROM_LOAD( "bepr201",      0x3800, 0x0800, 0x66da55d5 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr203",       0x0000, 0x0800, 0xbe26b561 )
	ROM_LOAD( "mcs_d",        0x0800, 0x0800, 0x13932a15 )
	ROM_LOAD( "epr202",       0x1000, 0x0800, 0x26c7e800 )
	ROM_LOAD( "mcs_c",        0x1800, 0x0800, 0x24cfd145 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( mooncrs2 )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "f8.bin",       0x0000, 0x0800, 0xd36003e5 )
	ROM_LOAD( "bepr195",      0x0800, 0x0800, 0xee262ff2 )
	ROM_LOAD( "f03.bin",      0x1000, 0x0800, 0x29a2b0ab )
	ROM_LOAD( "f04.bin",      0x1800, 0x0800, 0x4c6a5a6d )
	ROM_LOAD( "e5",           0x2000, 0x0800, 0x06d378a6 )
	ROM_LOAD( "bepr199",      0x2800, 0x0800, 0x6e84a927 )
	ROM_LOAD( "e7",           0x3000, 0x0800, 0xb45af1e8 )
	ROM_LOAD( "m7.bin",       0x3800, 0x0800, 0x957ee078 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1h_1_10.bin",  0x0000, 0x0800, 0x528da705 )
	ROM_LOAD( "12.chr",       0x0800, 0x0200, 0x5a4b17ea )
	ROM_CONTINUE(             0x0c00, 0x0200 )	/* this version of the gfx ROMs has two */
	ROM_CONTINUE(             0x0a00, 0x0200 )	/* groups of 16 sprites swapped */
	ROM_CONTINUE(             0x0e00, 0x0200 )
	ROM_LOAD( "1k_1_11.bin",  0x1000, 0x0800, 0x4e79ff6b )
	ROM_LOAD( "11.chr",       0x1800, 0x0200, 0xe0edccbd )
	ROM_CONTINUE(             0x1c00, 0x0200 )
	ROM_CONTINUE(             0x1a00, 0x0200 )
	ROM_CONTINUE(             0x1e00, 0x0200 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( fantazia )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "f01.bin",      0x0000, 0x0800, 0xd3e23863 )
	ROM_LOAD( "f02.bin",      0x0800, 0x0800, 0x63fa4149 )
	ROM_LOAD( "f03.bin",      0x1000, 0x0800, 0x29a2b0ab )
	ROM_LOAD( "f04.bin",      0x1800, 0x0800, 0x4c6a5a6d )
	ROM_LOAD( "f09.bin",      0x2000, 0x0800, 0x75fd5ca1 )
	ROM_LOAD( "f10.bin",      0x2800, 0x0800, 0xe4da2dd4 )
	ROM_LOAD( "f11.bin",      0x3000, 0x0800, 0x42869646 )
	ROM_LOAD( "f12.bin",      0x3800, 0x0800, 0xa48d7fb0 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1h_1_10.bin",  0x0000, 0x0800, 0x528da705 )
	ROM_LOAD( "mcs_d",        0x0800, 0x0800, 0x13932a15 )
	ROM_LOAD( "1k_1_11.bin",  0x1000, 0x0800, 0x4e79ff6b )
	ROM_LOAD( "mcs_c",        0x1800, 0x0800, 0x24cfd145 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	/* this PROM was bad (bit 3 always set). I tried to "fix" it to get more reasonable */
	/* colors, but it should not be considered correct. It's a bootleg anyway. */
	ROM_LOAD( "6l_prom.bin",  0x0000, 0x0020, BADCRC( 0xf5381d3e ) )
ROM_END

ROM_START( eagle )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "e1",           0x0000, 0x0800, 0x224c9526 )
	ROM_LOAD( "e2",           0x0800, 0x0800, 0xcc538ebd )
	ROM_LOAD( "f03.bin",      0x1000, 0x0800, 0x29a2b0ab )
	ROM_LOAD( "f04.bin",      0x1800, 0x0800, 0x4c6a5a6d )
	ROM_LOAD( "e5",           0x2000, 0x0800, 0x06d378a6 )
	ROM_LOAD( "e6",           0x2800, 0x0800, 0x0dea20d5 )
	ROM_LOAD( "e7",           0x3000, 0x0800, 0xb45af1e8 )
	ROM_LOAD( "e8",           0x3800, 0x0800, 0xc437a876 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "e10",          0x0000, 0x0800, 0x40ce58bf )
	ROM_LOAD( "e12",          0x0800, 0x0200, 0x628fdeed )
	ROM_CONTINUE(             0x0c00, 0x0200 )	/* this version of the gfx ROMs has two */
	ROM_CONTINUE(             0x0a00, 0x0200 )	/* groups of 16 sprites swapped */
	ROM_CONTINUE(             0x0e00, 0x0200 )
	ROM_LOAD( "e9",           0x1000, 0x0800, 0xba664099 )
	ROM_LOAD( "e11",          0x1800, 0x0200, 0xee4ec5fd )
	ROM_CONTINUE(             0x1c00, 0x0200 )
	ROM_CONTINUE(             0x1a00, 0x0200 )
	ROM_CONTINUE(             0x1e00, 0x0200 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( eagle2 )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "e1.7f",        0x0000, 0x0800, 0x45aab7a3 )
	ROM_LOAD( "e2",           0x0800, 0x0800, 0xcc538ebd )
	ROM_LOAD( "f03.bin",      0x1000, 0x0800, 0x29a2b0ab )
	ROM_LOAD( "f04.bin",      0x1800, 0x0800, 0x4c6a5a6d )
	ROM_LOAD( "e5",           0x2000, 0x0800, 0x06d378a6 )
	ROM_LOAD( "e6.6",         0x2800, 0x0800, 0x9f09f8c6 )
	ROM_LOAD( "e7",           0x3000, 0x0800, 0xb45af1e8 )
	ROM_LOAD( "e8",           0x3800, 0x0800, 0xc437a876 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "e10.2",        0x0000, 0x0800, 0x25b38ebd )
	ROM_LOAD( "e12",          0x0800, 0x0200, 0x628fdeed )
	ROM_CONTINUE(             0x0c00, 0x0200 )	/* this version of the gfx ROMs has two */
	ROM_CONTINUE(             0x0a00, 0x0200 )	/* groups of 16 sprites swapped */
	ROM_CONTINUE(             0x0e00, 0x0200 )
	ROM_LOAD( "e9",           0x1000, 0x0800, 0xba664099 )
	ROM_LOAD( "e11",          0x1800, 0x0200, 0xee4ec5fd )
	ROM_CONTINUE(             0x1c00, 0x0200 )
	ROM_CONTINUE(             0x1a00, 0x0200 )
	ROM_CONTINUE(             0x1e00, 0x0200 )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "l06_prom.bin", 0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( moonqsr )
	ROM_REGIONX( 0x20000, REGION_CPU1 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "mq1",          0x0000, 0x0800, 0x132c13ec )
	ROM_LOAD( "mq2",          0x0800, 0x0800, 0xc8eb74f1 )
	ROM_LOAD( "mq3",          0x1000, 0x0800, 0x33965a89 )
	ROM_LOAD( "mq4",          0x1800, 0x0800, 0xa3861d17 )
	ROM_LOAD( "mq5",          0x2000, 0x0800, 0x8bcf9c67 )
	ROM_LOAD( "mq6",          0x2800, 0x0800, 0x5750cda9 )
	ROM_LOAD( "mq7",          0x3000, 0x0800, 0x78d7fe5b )
	ROM_LOAD( "mq8",          0x3800, 0x0800, 0x4919eed5 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mqb",          0x0000, 0x0800, 0xb55ec806 )
	ROM_LOAD( "mqd",          0x0800, 0x0800, 0x9e7d0e13 )
	ROM_LOAD( "mqa",          0x1000, 0x0800, 0x66eee0db )
	ROM_LOAD( "mqc",          0x1800, 0x0800, 0xa6db5b0d )

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "vid_e6.bin",   0x0000, 0x0020, 0x0b878b54 )
ROM_END

ROM_START( checkman )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "cm1",          0x0000, 0x0800, 0xe8cbdd28 )
	ROM_LOAD( "cm2",          0x0800, 0x0800, 0xb8432d4d )
	ROM_LOAD( "cm3",          0x1000, 0x0800, 0x15a97f61 )
	ROM_LOAD( "cm4",          0x1800, 0x0800, 0x8c12ecc0 )
	ROM_LOAD( "cm5",          0x2000, 0x0800, 0x2352cfd6 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cm11",         0x0000, 0x0800, 0x8d1bcca0 )
	/* 0800-0fff empty */
	ROM_LOAD( "cm9",          0x1000, 0x0800, 0x3cd5c751 )
	/* 1800-1fff empty */

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "checkman.clr", 0x0000, 0x0020, 0x57a45057 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for sound code */
	ROM_LOAD( "cm13",         0x0000, 0x0800, 0x0b09a3e8 )
	ROM_LOAD( "cm14",         0x0800, 0x0800, 0x47f043be )
ROM_END

ROM_START( moonal2 )
	ROM_REGIONX( 0x10000, REGION_CPU1 ) /* 64k for code */
	ROM_LOAD( "ali1",         0x0000, 0x0400, 0x0dcecab4 )
	ROM_LOAD( "ali2",         0x0400, 0x0400, 0xc6ee75a7 )
	ROM_LOAD( "ali3",         0x0800, 0x0400, 0xcd1be7e9 )
	ROM_LOAD( "ali4",         0x0c00, 0x0400, 0x83b03f08 )
	ROM_LOAD( "ali5",         0x1000, 0x0400, 0x6f3cf61d )
	ROM_LOAD( "ali6",         0x1400, 0x0400, 0xe169d432 )
	ROM_LOAD( "ali7",         0x1800, 0x0400, 0x41f64b73 )
	ROM_LOAD( "ali8",         0x1c00, 0x0400, 0xf72ee876 )
	ROM_LOAD( "ali9",         0x2000, 0x0400, 0xb7fb763c )
	ROM_LOAD( "ali10",        0x2400, 0x0400, 0xb1059179 )
	ROM_LOAD( "ali11",        0x2800, 0x0400, 0x9e79a1c6 )

	ROM_REGION_DISPOSE(0x2000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ali13.1h",     0x0000, 0x0800, 0xa1287bf6 )
	/* 0800-0fff empty */
	ROM_LOAD( "ali12.1k",     0x1000, 0x0800, 0x528f1481 )
	/* 1800-1fff empty */

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( moonal2b )
	ROM_REGIONX( 0x10000, REGION_CPU1 ) /* 64k for code */
	ROM_LOAD( "ali1",         0x0000, 0x0400, 0x0dcecab4 )
	ROM_LOAD( "ali2",         0x0400, 0x0400, 0xc6ee75a7 )
	ROM_LOAD( "md-2",         0x0800, 0x0800, 0x8318b187 )
	ROM_LOAD( "ali5",         0x1000, 0x0400, 0x6f3cf61d )
	ROM_LOAD( "ali6",         0x1400, 0x0400, 0xe169d432 )
	ROM_LOAD( "ali7",         0x1800, 0x0400, 0x41f64b73 )
	ROM_LOAD( "ali8",         0x1c00, 0x0400, 0xf72ee876 )
	ROM_LOAD( "ali9",         0x2000, 0x0400, 0xb7fb763c )
	ROM_LOAD( "ali10",        0x2400, 0x0400, 0xb1059179 )
	ROM_LOAD( "md-6",         0x2800, 0x0800, 0x9cc973e0 )

	ROM_REGION_DISPOSE(0x2000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ali13.1h",     0x0000, 0x0800, 0xa1287bf6 )
	/* 0800-0fff empty */
	ROM_LOAD( "ali12.1k",     0x1000, 0x0800, 0x528f1481 )
	/* 1800-1fff empty */

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "galaxian.clr", 0x0000, 0x0020, 0xc3ac9467 )
ROM_END

ROM_START( kingball )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "prg1.7f",      0x0000, 0x1000, 0x6cb49046 )
	ROM_LOAD( "prg2.7j",      0x1000, 0x1000, 0xc223b416 )
	ROM_LOAD( "prg3.7l",      0x2000, 0x0800, 0x453634c0 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "chg1.1h",      0x0000, 0x0800, 0x9cd550e7 )
	/* 0800-0fff empty */
	ROM_LOAD( "chg2.1k",      0x1000, 0x0800, 0xa206757d )
	/* 1800-1fff empty */

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "kb2-1",        0x0000, 0x0020, 0x15dd5b16 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for sound code */
	ROM_LOAD( "kbe1.ic4",     0x0000, 0x0800, 0x5be2c80a )
	ROM_LOAD( "kbe2.ic5",     0x0800, 0x0800, 0xbb59e965 )
	ROM_LOAD( "kbe3.ic6",     0x1000, 0x0800, 0x1c94dd31 )
	ROM_LOAD( "kbe2.ic7",     0x1800, 0x0800, 0xbb59e965 )
ROM_END

ROM_START( kingbalj )
	ROM_REGIONX( 0x10000, REGION_CPU1 )	/* 64k for code */
	ROM_LOAD( "prg1.7f",      0x0000, 0x1000, 0x6cb49046 )
	ROM_LOAD( "prg2.7j",      0x1000, 0x1000, 0xc223b416 )
	ROM_LOAD( "prg3.7l",      0x2000, 0x0800, 0x453634c0 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "chg1.1h",      0x0000, 0x0800, 0x9cd550e7 )
	/* 0800-0fff empty */
	ROM_LOAD( "chg2.1k",      0x1000, 0x0800, 0xa206757d )
	/* 1800-1fff empty */

	ROM_REGIONX( 0x0020, REGION_PROMS )
	ROM_LOAD( "kb2-1",        0x0000, 0x0020, 0x15dd5b16 )

	ROM_REGIONX( 0x10000, REGION_CPU2 )	/* 64k for sound code */
	ROM_LOAD( "kbj1.ic4",     0x0000, 0x0800, 0xba16beb7 )
	ROM_LOAD( "kbj2.ic5",     0x0800, 0x0800, 0x56686a63 )
	ROM_LOAD( "kbj3.ic6",     0x1000, 0x0800, 0xfbc570a5 )
	ROM_LOAD( "kbj2.ic7",     0x1800, 0x0800, 0x56686a63 )
ROM_END



static unsigned char decode(int data,int addr)
{
	int res;

	res = data;
	if (data & 0x02) res ^= 0x40;
	if (data & 0x20) res ^= 0x04;
	if ((addr & 1) == 0)
		res = (res & 0xbb) | ((res & 0x40) >> 4) | ((res & 0x04) << 4);
	return res;
}

static void mooncrst_decode(void)
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);


	for (A = 0;A < 0x10000;A++)
		rom[A] = decode(rom[A],A);
}

static void moonqsr_decode(void)
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	memory_set_opcode_base(0,rom+diff);

	for (A = 0;A < 0x10000;A++)
		rom[A + diff] = decode(rom[A],A);
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
	unsigned char *rom = memory_region(REGION_CPU1);


	for (A = 0;A < 0x2800;A++)
	{
		switch (A & 0x07)
		{
			case 0: data_xor =  (rom[A] & 0x40) >> 6; break;
			case 1: data_xor =  (rom[A] & 0x20) >> 4; break;
			case 2: data_xor = ((rom[A] & 0x10) >> 2) | ((rom[A] & 0x40) >> 5); break;
			case 3: data_xor = ((rom[A] & 0x04) << 2) | ((rom[A] & 0x20) >> 5); break;
			case 4: data_xor = ((rom[A] & 0x10) << 2) | ((rom[A] & 0x02) << 4); break;
			case 5: data_xor = ((rom[A] & 0x01) << 6) | ((rom[A] & 0x04) << 3); break;
			case 6: data_xor =  (rom[A] & 0x01) << 2; break;
			case 7: data_xor =  (rom[A] & 0x02) << 3; break;
		}
		rom[A] ^= data_xor;
	}
}



struct GameDriver driver_mooncrst =
{
	__FILE__,
	0,
	"mooncrst",
	"Moon Cresta (Nichibutsu)",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	mooncrst_decode,

	rom_mooncrst,
	0, 0,
	0,
	0,

	input_ports_mooncrst,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_mooncrsg =
{
	__FILE__,
	&driver_mooncrst,
	"mooncrsg",
	"Moon Cresta (Gremlin)",
	"1980",
	"Gremlin",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	0,

	rom_mooncrsg,
	0, 0,
	0,
	0,

	input_ports_mooncrst,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_smooncrs =
{
	__FILE__,
	&driver_mooncrst,
	"smooncrs",
	"Super Moon Cresta",
	"1980?",
	"Gremlin",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	0,

	rom_smooncrs,
	0, 0,
	0,
	0,

	input_ports_mooncrst,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_mooncrsb =
{
	__FILE__,
	&driver_mooncrst,
	"mooncrsb",
	"Moon Cresta (bootleg set 1)",
	"1980",
	"bootleg",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	0,

	rom_mooncrsb,
	0, 0,
	0,
	0,

	input_ports_mooncrst,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_mooncrs2 =
{
	__FILE__,
	&driver_mooncrst,
	"mooncrs2",
	"Moon Cresta (bootleg set 2)",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	0,

	rom_mooncrs2,
	0, 0,
	0,
	0,

	input_ports_mooncrst,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_fantazia =
{
	__FILE__,
	&driver_mooncrst,
	"fantazia",
	"Fantazia",
	"1980",
	"bootleg",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	0,

	rom_fantazia,
	0, 0,
	0,
	0,

	input_ports_mooncrst,

	0, 0, 0,
	ROT90 | GAME_IMPERFECT_COLORS,
	0,0
};

struct GameDriver driver_eagle =
{
	__FILE__,
	&driver_mooncrst,
	"eagle",
	"Eagle (set 1)",
	"1980",
	"Centuri",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	0,

	rom_eagle,
	0, 0,
	0,
	0,

	input_ports_eagle,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_eagle2 =
{
	__FILE__,
	&driver_mooncrst,
	"eagle2",
	"Eagle (set 2)",
	"1980",
	"Centuri",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott",
	0,
	&machine_driver_mooncrst,
	0,

	rom_eagle2,
	0, 0,
	0,
	0,

	input_ports_eagle2,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_moonqsr =
{
	__FILE__,
	0,
	"moonqsr",
	"Moon Quasar",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nMike Coates (decryption info)\nNicola Salmoria (MAME driver)\nGary Walton (color info)\nSimon Walls (color info)\nAndrew Scott\nMarco Cassili",
	0,
	&machine_driver_moonqsr,
	moonqsr_decode,

	rom_moonqsr,
	0, 0,
	0,
	0,

	input_ports_moonqsr,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_checkman =
{
	__FILE__,
	0,
	"checkman",
	"Checkman",
	"1982",
	"Zilec-Zenitone",
	"Brad Oliver (MAME driver)\nMalcolm Lear (hardware & encryption info)",
	0,
	&machine_driver_checkman,
	checkman_decode,

	rom_checkman,
	0, 0,
	0,
	0,

	input_ports_checkman,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_moonal2 =
{
	__FILE__,
	0,
	"moonal2",
	"Moon Alien Part 2",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAndrew Scott",
	0,
	&machine_driver_moonal2,
	0,

	rom_moonal2,
	0, 0,
	0,
	0,

	input_ports_moonal2,

	0, 0, 0,
	ROT90,

	0, 0
};

struct GameDriver driver_moonal2b =
{
	__FILE__,
	&driver_moonal2,
	"moonal2b",
	"Moon Alien Part 2 (older version)",
	"1980",
	"Nichibutsu",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nAndrew Scott",
	0,
	&machine_driver_moonal2,
	0,

	rom_moonal2b,
	0, 0,
	0,
	0,

	input_ports_moonal2,

	0, 0, 0,
	ROT90,

	0, 0
};

struct GameDriver driver_kingball =
{
	__FILE__,
	0,
	"kingball",
	"King & Balloon (US)",
	"1980",
	"Namco",
	"Brad Oliver",
	0,
	&machine_driver_kingball,
	0,

	rom_kingball,
	0, 0,
	0,
	0,

	input_ports_kingball,

	0, 0, 0,
	ROT90,
	0,0
};

struct GameDriver driver_kingbalj =
{
	__FILE__,
	&driver_kingball,
	"kingbalj",
	"King & Balloon (Japan)",
	"1980",
	"Namco",
	"Brad Oliver",
	0,
	&machine_driver_kingball,
	0,

	rom_kingbalj,
	0, 0,
	0,
	0,

	input_ports_kingball,

	0, 0, 0,
	ROT90,
	0,0
};
