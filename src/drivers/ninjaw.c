/***************************************************************************

Taito Triple Screen Games
=========================

Ninja Warriors (c) 1987 Taito
Darius 2       (c) 1989 Taito

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime,
Nicola Salmoria. Thanks to Richard Bush and the Raine team, whose open
source was very helpful in many areas particularly the sprites.)

				*****

The triple screen games operate on hardware with various similarities to
the Taito F2 system, as they share some custom ics e.g. the TC0100SCN.
In the arcades they have three horizontal screens side by side. (???)

For each screen the games have 3 separate layers of graphics:- one
128x64 tiled scrolling background plane of 8x8 tiles, a similar
foreground plane, and a 128x32 text plane with character definitions
held in ram. It appears that the tilemap generating chips for each
screen actually all contain *identical* tilemaps, but the scroll
controls are used to get the correct part visible on that particular
screen.

Hence the tilemap area for the first screen appears to contain a
complete tilemap *including* what would be displayed on the other two
screens.

There is a single sprite plane which covers all 3 screens.
The sprites are 16x16 and are not zoomable.

Twin 68000 processors are used; both have access to sprite ram and
the tilemap areas, and they communicate via 64K of shared ram.

Sound is dealt with by a Z80 controlling a YM2610. Sound commands
are written to the Z80 by the 68000 (the same as in Taito F2 games).


Tilemaps
========

TC0100SCN has tilemaps twice as wide as usual. The two BG tilemaps take
up twice the usual space, $8000 bytes each. The text tilemap takes up
the usual space, because its height is halved [like Cameltru].

The triple palette generator (one for each screen) is probably just a
result of the way the 3-screen hardware works. They all seem to offer an
identical set of colors. We emulate all three, but currently we are only
drawing the first TC0100SCN tilemap.

TODO
====

Sprite / tile priority

Sprite / tile alignment

If we drew tilemaps from *all three* TC0100SCNs, with their colors
individually set by their particular palette generator, then the
emulation would be more accurate. However, probably the result of
only drawing one is identical.

DIPs


Ninjaw
------

Some bad/rough sounds.


Darius 2
--------

Some bad/rough sounds.


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

int ninjaw_vh_start (void);
void ninjaw_vh_stop (void);

void ninjaw_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);

//static data16_t *ninjaw_ram;

static int ioc220_port=0;
static int old_cpua_ctrl = 0xff;

extern size_t ninjaw_spriteram_size;
extern data16_t *ninjaw_spriteram16;
READ16_HANDLER( ninjaw_spriteram_r );
WRITE16_HANDLER( ninjaw_spriteram_w );

static size_t sharedram_size;
static data16_t *sharedram;

static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static WRITE16_HANDLER( cpua_ctrl_w )	// assumes Z80 sandwiched between 68Ks
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;

	/* bit 0 enables cpu B */

	if ((data &0x1)!=(old_cpua_ctrl &0x1))	// perhaps unnecessary but may be written with same value
		cpu_set_reset_line(2,(data &0x1) ? CLEAR_LINE : ASSERT_LINE);

	/* is there an irq enable ??? */

	old_cpua_ctrl = data;

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",cpu_get_pc(),data);
}

/***********************************************************
				INTERRUPTS
***********************************************************/

static int ninjaw_interrupt(void)
{
	return 4;
}


/**********************************************************
			GAME INPUTS
**********************************************************/

static READ16_HANDLER( ninjaw_ioc_r )
{
	switch (offset)
	{
		case 0x00:
			{
				switch (ioc220_port & 0xf)
				{
					case 0x00:
						return input_port_3_word_r(0);	/* DSW A */

					case 0x01:
						return input_port_4_word_r(0);	/* DSW B */

					case 0x02:
						return input_port_0_word_r(0);	/* IN0 */

					case 0x03:
						return input_port_1_word_r(0);	/* IN1 */

					case 0x07:
						return input_port_2_word_r(0);	/* IN2 */
				}

logerror("CPU #0 PC %06x: warning - read unmapped ioc220 port %02x\n",cpu_get_pc(),ioc220_port);
					return 0;
			}

		case 0x01:
			return ioc220_port;
	}

	return 0x00;	// keep compiler happy
}

static WRITE16_HANDLER( ninjaw_ioc_w )
{
	switch (offset)
	{
		case 0x00:
			// write to ioc [coin lockout etc.?]
			break;

		case 0x01:
			ioc220_port = data &0xff;	// mask seems ok
	}
}


/*****************************************
			SOUND
*****************************************/

static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 7;

#ifdef MAME_DEBUG
	if (banknum>3) logerror("Z80 switch to ROM bank %06x: should only happen if Z80 prg rom is 128K!\n",banknum);
#endif
	cpu_setbank (10, &RAM [0x10000 + (banknum * 0x4000)]);
}

WRITE16_HANDLER( ninjaw_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);

#ifdef MAME_DEBUG
	if (data & 0xff00)
	{
		char buf[80];

		sprintf(buf,"ninjaw_sound_w to high byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ16_HANDLER( ninjaw_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff));
	else return 0;
}


/***********************************************************
			 MEMORY STRUCTURES

	TC0100SCN memory table from Raine
	---------------------------------

	280000 - 287fff | BG0 (1024x512 - 4 bytes/tile)
	288000 - 28ffff | BG1 (1024x512 - 4 bytes/tile)
	290000 - 2903ff | BG0 rowscroll
	290400 - 2907ff | BG1 rowscroll
	291000 - 291fff | FG0 GFX (8x8x4 - 16 bytes/tile)
	292000 - 293fff | FG0 (1024x256 - 2 bytes/tile)

***********************************************************/

static MEMORY_READ16_START( ninjaw_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x20000f, ninjaw_ioc_r },
	{ 0x220000, 0x220003, ninjaw_sound_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, ninjaw_spriteram_r },	/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_r },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_r },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_r },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_r },
	{ 0x340000, 0x340007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_word_2_r },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_WRITE16_START( ninjaw_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, MWA16_RAM },
	{ 0x200000, 0x20000f, ninjaw_ioc_w },
	{ 0x210000, 0x210001, cpua_ctrl_w },
	{ 0x220000, 0x220003, ninjaw_sound_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram, &sharedram_size },
	{ 0x260000, 0x263fff, ninjaw_spriteram_w, &ninjaw_spriteram16, &ninjaw_spriteram_size },
	{ 0x280000, 0x293fff, TC0100SCN_word_0_w },	/* tilemaps (1st screen) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_w },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_w },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_w },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_w },
	{ 0x340000, 0x340007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_step1_word_2_w },	/* palette (3rd screen) */
MEMORY_END

// NB there could be conflicts between which cpu writes what to the
// palette, as our interleaving won't match the original board.

static MEMORY_READ16_START( ninjaw_cpub_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x080000, 0x08ffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x20000f, ninjaw_ioc_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, ninjaw_spriteram_r },	/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x340000, 0x340007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_word_2_r },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_WRITE16_START( ninjaw_cpub_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x080000, 0x08ffff, MWA16_RAM },
	{ 0x200000, 0x20000f, ninjaw_ioc_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram },
	{ 0x260000, 0x263fff, ninjaw_spriteram_w },
	{ 0x280000, 0x293fff, TC0100SCN_word_0_w },	/* tilemaps (1st screen) */
	{ 0x340000, 0x340007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_step1_word_2_w },	/* palette (3rd screen) */
MEMORY_END


static MEMORY_READ16_START( darius2_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x20000f, ninjaw_ioc_r },
	{ 0x220000, 0x220003, ninjaw_sound_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, ninjaw_spriteram_r },	/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_r },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_r },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_r },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_r },
	{ 0x340000, 0x340007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_word_2_r },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_WRITE16_START( darius2_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, MWA16_RAM },
	{ 0x200000, 0x20000f, ninjaw_ioc_w },
	{ 0x210000, 0x210001, cpua_ctrl_w },
	{ 0x220000, 0x220003, ninjaw_sound_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram, &sharedram_size },
	{ 0x260000, 0x263fff, ninjaw_spriteram_w, &ninjaw_spriteram16, &ninjaw_spriteram_size },
	{ 0x280000, 0x293fff, TC0100SCN_word_0_w },	/* tilemaps (1st screen) */
	{ 0x2a0000, 0x2a000f, TC0100SCN_ctrl_word_0_w },
	{ 0x2c0000, 0x2d3fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x2e0000, 0x2e000f, TC0100SCN_ctrl_word_1_w },
	{ 0x300000, 0x313fff, TC0100SCN_word_2_w },	/* tilemaps (3rd screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_2_w },
	{ 0x340000, 0x340007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x350000, 0x350007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x360000, 0x360007, TC0110PCR_step1_word_2_w },	/* palette (3rd screen) */
MEMORY_END

static MEMORY_READ16_START( darius2_cpub_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x080000, 0x08ffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x20000f, ninjaw_ioc_r },
	{ 0x240000, 0x24ffff, sharedram_r },
	{ 0x260000, 0x263fff, ninjaw_spriteram_r },	/* sprite ram */
	{ 0x280000, 0x293fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
MEMORY_END

static MEMORY_WRITE16_START( darius2_cpub_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x080000, 0x08ffff, MWA16_RAM },
	{ 0x200000, 0x20000f, ninjaw_ioc_w },
	{ 0x240000, 0x24ffff, sharedram_w, &sharedram },
	{ 0x260000, 0x263fff, ninjaw_spriteram_w },
	{ 0x280000, 0x293fff, TC0100SCN_word_0_w },	/* tilemaps (1st screen) */
MEMORY_END


/***************************************************************************/

static MEMORY_READ_START( z80_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( z80_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, bankswitch_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

INPUT_PORTS_START( ninjaw )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	// Stops working if this is high
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )	// Freezes game
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Continuous fire" )	// Darius only
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/***********************************************************
				GFX DECODING

	(Thanks to Raine for the obj decoding)
***********************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },	/* pixel bits separated, jump 4 to get to next one */
	{ 3, 2, 1, 0, 19, 18, 17, 16,
	  3+ 32*8, 2+ 32*8, 1+ 32*8, 0+ 32*8, 19+ 32*8, 18+ 32*8, 17+ 32*8, 16+ 32*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  64*8 + 0*32, 64*8 + 1*32, 64*8 + 2*32, 64*8 + 3*32,
	  64*8 + 4*32, 64*8 + 5*32, 64*8 + 6*32, 64*8 + 7*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo ninjaw_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* scr tiles (screen 1) */
	{ REGION_GFX3, 0, &charlayout,  0, 256 },	/* scr tiles (screens 2+) */
	{ -1 } /* end of array */
};


/**************************************************************
			     YM2610 (SOUND)
**************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz ?? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};


/***********************************************************
			     MACHINE DRIVERS

Arbitrary interleaving of 10 set to help keep cpus in
sync. They both write to spriteram, for example.
***********************************************************/

static struct MachineDriver machine_driver_ninjaw =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			ninjaw_readmem,ninjaw_writemem,0,0,
			ninjaw_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/4,	/* 4 MHz ??? */
			z80_sound_readmem, z80_sound_writemem,0,0,
			ignore_interrupt, 0	/* IRQs are triggered by the YM2610 */
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			ninjaw_cpub_readmem,ninjaw_cpub_writemem,0,0,
			ninjaw_interrupt, 1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* CPU slices */
	0,

	/* video hardware */
	113*8, 32*8, { 2*8+6, 112*8+6-1, 3*8, 31*8-1 },

	ninjaw_gfxdecodeinfo,
	4096*3, 4096*3,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_DUAL_MONITOR,
	0,
	ninjaw_vh_start,
	ninjaw_vh_stop,
	ninjaw_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_darius2 =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			darius2_readmem,darius2_writemem,0,0,
			ninjaw_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/4,	/* 4 MHz ??? */
			z80_sound_readmem, z80_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			darius2_cpub_readmem,darius2_cpub_writemem,0,0,
			ninjaw_interrupt, 1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* CPU slices */
	0,

	/* video hardware */
	113*8, 32*8, { 2*8+6, 112*8+6-1, 3*8, 31*8-1 },

	ninjaw_gfxdecodeinfo,
	4096*3, 4096*3,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_DUAL_MONITOR,
	0,
	ninjaw_vh_start,
	ninjaw_vh_stop,
	ninjaw_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( ninjaw )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 256K for 68000 CPUA code */
	ROM_LOAD16_BYTE( "b31-45", 0x00000, 0x10000, 0x107902c3 )
	ROM_LOAD16_BYTE( "b31-47", 0x00001, 0x10000, 0xbd536b1e )
	ROM_LOAD16_BYTE( "b31-29", 0x20000, 0x10000, 0xf2941a37 )
	ROM_LOAD16_BYTE( "b31-27", 0x20001, 0x10000, 0x2f3ff642 )

	ROM_LOAD16_BYTE( "b31-41", 0x40000, 0x20000, 0x0daef28a )	/* last 4 data roms ? */
	ROM_LOAD16_BYTE( "b31-39", 0x40001, 0x20000, 0xe9197c3c )
	ROM_LOAD16_BYTE( "b31-40", 0x80000, 0x20000, 0x2ce0f24e )
	ROM_LOAD16_BYTE( "b31-38", 0x80001, 0x20000, 0xbc68cd99 )

	ROM_REGION( 0x60000, REGION_CPU3, 0 )	/* 384K for 68000 CPUB code */
	ROM_LOAD16_BYTE( "b31-33", 0x00000, 0x10000, 0x6ce9af44 )
	ROM_LOAD16_BYTE( "b31-36", 0x00001, 0x10000, 0xba20b0d4 )
	ROM_LOAD16_BYTE( "b31-32", 0x20000, 0x10000, 0xe6025fec )
	ROM_LOAD16_BYTE( "b31-35", 0x20001, 0x10000, 0x70d9a89f )
	ROM_LOAD16_BYTE( "b31-31", 0x40000, 0x10000, 0x837f47e2 )
	ROM_LOAD16_BYTE( "b31-34", 0x40001, 0x10000, 0xd6b5fb2a )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "b31-37",  0x00000, 0x04000, 0x0ca5799d )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b31-01", 0x00000, 0x80000, 0x8e8237a7 )	/* SCR (screen 1) */
	ROM_LOAD( "b31-02", 0x80000, 0x80000, 0x4c3b4e33 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b31-07", 0x000000, 0x80000, 0x33568cdb )	/* OBJ */
	ROM_LOAD( "b31-06", 0x080000, 0x80000, 0x0d59439e )
	ROM_LOAD( "b31-05", 0x100000, 0x80000, 0x0a1fc9fb )
	ROM_LOAD( "b31-04", 0x180000, 0x80000, 0x2e1e4cb5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screens 2+) */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b31-09", 0x000000, 0x80000, 0x60a73382 )
	ROM_LOAD( "b31-10", 0x080000, 0x80000, 0xc6434aef )
	ROM_LOAD( "b31-11", 0x100000, 0x80000, 0x8da531d4 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "b31-08", 0x000000, 0x80000, 0xa0a1f87d )

	// no unused roms in my set
ROM_END

ROM_START( darius2 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 256K for 68000 CPUA code */
	ROM_LOAD16_BYTE( "c07-32-1", 0x00000, 0x10000, 0x216c8f6a )
	ROM_LOAD16_BYTE( "c07-29-1", 0x00001, 0x10000, 0x48de567f )
	ROM_LOAD16_BYTE( "c07-31-1", 0x20000, 0x10000, 0x8279d2f8 )
	ROM_LOAD16_BYTE( "c07-30-1", 0x20001, 0x10000, 0x6122e400 )

	ROM_LOAD16_BYTE( "c07-27",   0x40000, 0x20000, 0x0a6f7b6c )	/* last 4 data roms ? */
	ROM_LOAD16_BYTE( "c07-25",   0x40001, 0x20000, 0x059f40ce )
	ROM_LOAD16_BYTE( "c07-26",   0x80000, 0x20000, 0x1f411242 )
	ROM_LOAD16_BYTE( "c07-24",   0x80001, 0x20000, 0x486c9c20 )

	ROM_REGION( 0x60000, REGION_CPU3, 0 )	/* 384K for 68000 CPUB code */
	ROM_LOAD16_BYTE( "c07-35-1", 0x00000, 0x10000, 0xdd8c4723 )
	ROM_LOAD16_BYTE( "c07-38-1", 0x00001, 0x10000, 0x46afb85c )
	ROM_LOAD16_BYTE( "c07-34-1", 0x20000, 0x10000, 0x296984b8 )
	ROM_LOAD16_BYTE( "c07-37-1", 0x20001, 0x10000, 0x8b7d461f )
	ROM_LOAD16_BYTE( "c07-33-1", 0x40000, 0x10000, 0x2da03a3f )
	ROM_LOAD16_BYTE( "c07-36-1", 0x40001, 0x10000, 0x02cf2b1c )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c07-28",  0x00000, 0x04000, 0xda304bc5 )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-03", 0x00000, 0x80000, 0x189bafce )	/* SCR (screen 1) */
	ROM_LOAD( "c07-04", 0x80000, 0x80000, 0x50421e81 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-01", 0x00000, 0x80000, 0x3cf0f050 )	/* OBJ */
	ROM_LOAD( "c07-02", 0x80000, 0x80000, 0x75d16d4b )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screens 2+) */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c07-10", 0x00000, 0x80000, 0x4bbe0ed9 )
	ROM_LOAD( "c07-11", 0x80000, 0x80000, 0x3c815699 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "c07-12", 0x00000, 0x80000, 0xe0b71258 )

	// no unused roms in my set
ROM_END




/* Working Games */

GAME( 1987, ninjaw,  0, ninjaw,  ninjaw, 0, ROT0, "Taito Corporation Japan", "The Ninja Warriors (Japan)" )
GAME( 1989, darius2, 0, darius2, ninjaw, 0, ROT0, "Taito Corporation", "Darius II (Japan)" )

