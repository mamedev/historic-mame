/***************************************************************************

Taito Dual Screen Games
=======================

Darius 2      (c) 1989 Taito
Warrior Blade (c) 1991 Taito

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime,
Nicola Salmoria. Thanks to Richard Bush and the Raine team, whose open
source was very helpful in many areas particularly the sprites.)

				*****

The dual screen games operate on hardware with various similarities to
the Taito F2 system, as they share some custom ics e.g. the TC0100SCN.

For each screen the games have 3 separate layers of graphics: - one
128x64 tiled scrolling background plane of 8x8 tiles, a similar
foreground plane, and a 128x32 text plane with character definitions
held in ram. As well as this, there is a single sprite plane which
covers both screens. The sprites are 16x16 and are not zoomable.

Writing to the first TC0100SCN "writes through" to the subsidiary one
so both have identical contents. The only time the second TC0100SCN is
addressed on its own is during initial memory checks, I think. (?)

Warrior Blade has a slightly different gfx set for the 2nd screen
because the programmers ran out of scr gfx space (only 0xffff tiles
can be addressed by the TC0100SCN). In-game while tiles are
scrolling from one screen to the other it is necessary to have
identical gfx tiles for both screens. But for static screens (e.g. cut
scenes between levels) the gfx tiles needn't be the same. By
exploiting this they squeezed some extra graphics into the game.

There is a single 68000 processor which takes care of everything
except sound. That is done by a Z80 controlling a YM2610. Sound
commands are written to the Z80 by the 68000 (the same as in Taito
F2 games).


Tilemaps
========

TC0100SCN has tilemaps twice as wide as usual. The two BG tilemaps take
up twice the usual space, $8000 bytes each. The text tilemap takes up
the usual space, as its height is halved.

The double palette generator (one for each screen) is probably just a
result of the way the hardware works: they both have the same colors.

Dumper's Info
-------------

Darius II (Dual Screen Old & New JPN Ver.)
(c)1989 Taito
J1100204A
K1100483A

CPU 	:MC68000P12F(16MHz),Z80A
Sound	:YM2610
OSC 	:26.686MHz,16.000MHz
Other	:
TC0140SYT
TC0220IOC
TC0110PCR x2
TC0100SCN x2
TC0390LHC-1
TC0130LNB x8

[Function of these last two unknown].

TODO
====

Unknown sprite bits.


Darius 2
--------

The unpleasant sounds when some big enemies appear are wrong, they
are meant to create rumbling on a subwoofer in the cabinet, as a sort
of vibration device. Either strip them out or transpose them down in
pitch?


Warriorb
--------

Colscroll effects?


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

int warriorb_vh_start (void);
void warriorb_vh_stop (void);
void warriorb_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);

//static data16_t *warriorb_ram;


WRITE16_HANDLER( TC0100SCN_dual_screen_w )
{
	TC0100SCN_word_0_w(offset,data,mem_mask);
	TC0100SCN_word_1_w(offset,data,mem_mask);
}

/***********************************************************
				INTERRUPTS
***********************************************************/

static int warriorb_interrupt(void)
{
	return 4;
}


/**********************************************************
			GAME INPUTS
**********************************************************/

static READ16_HANDLER( warriorb_input_r )
{
	switch (offset)
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

logerror("CPU #0 PC %06x: warning - read unmapped input offset %06x\n",cpu_get_pc(),offset);

	return 0xff;
}


/*****************************************
			SOUND
*****************************************/

static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 7;

#ifdef MAME_DEBUG
	if (banknum>3) logerror("CPU#1 (Z80) switch to ROM bank %06x: should only happen if Z80 prg rom is 128K!\n",banknum);
#endif
	cpu_setbank (10, &RAM [0x10000 + (banknum * 0x4000)]);
}

WRITE16_HANDLER( warriorb_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);

#ifdef MAME_DEBUG
	if (data & 0xff00)
	{
		char buf[80];

		sprintf(buf,"warriorb_sound_w to high byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ16_HANDLER( warriorb_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff));
	else return 0;
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( darius2d_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },	/* main ram */
	{ 0x200000, 0x213fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_r },
	{ 0x240000, 0x253fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x260000, 0x26000f, TC0100SCN_ctrl_word_1_r },
	{ 0x400000, 0x400007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MRA16_RAM },	/* sprite ram */
	{ 0x800000, 0x80000f, warriorb_input_r },
	{ 0x830000, 0x830003, warriorb_sound_r },
MEMORY_END

static MEMORY_WRITE16_START( darius2d_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x213fff, TC0100SCN_dual_screen_w },	/* tilemaps (all screens) */
	{ 0x214000, 0x2141ff, MWA16_NOP },	/* error in screen clearing code ? */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_w },
	{ 0x240000, 0x253fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x260000, 0x26000f, TC0100SCN_ctrl_word_1_w },
	{ 0x400000, 0x400007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MWA16_RAM, &spriteram16, &spriteram_size },
//	{ 0x800000, 0x800001, MWA16_NOP },	/* watchdog ?? */
//	{ 0x800008, 0x800009, MWA16_NOP },	/* coin lockout/ctr ? */
//	{ 0x820000, 0x820001, MWA16_NOP },	// ???
	{ 0x830000, 0x830003, warriorb_sound_w },
MEMORY_END


static MEMORY_READ16_START( warriorb_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x213fff, MRA16_RAM },	/* main ram */
	{ 0x300000, 0x313fff, TC0100SCN_word_0_r },	/* tilemaps (1st screen) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_r },
	{ 0x340000, 0x353fff, TC0100SCN_word_1_r },	/* tilemaps (2nd screen) */
	{ 0x360000, 0x36000f, TC0100SCN_ctrl_word_1_r },
	{ 0x400000, 0x400007, TC0110PCR_word_r },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_word_1_r },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MRA16_RAM },	/* sprite ram */
	{ 0x800000, 0x80000f, warriorb_input_r },
	{ 0x830000, 0x830003, warriorb_sound_r },
MEMORY_END

static MEMORY_WRITE16_START( warriorb_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x213fff, MWA16_RAM },
	{ 0x300000, 0x313fff, TC0100SCN_dual_screen_w },	/* tilemaps (all screens) */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_w },
	{ 0x340000, 0x353fff, TC0100SCN_word_1_w },	/* tilemaps (2nd screen) */
	{ 0x360000, 0x36000f, TC0100SCN_ctrl_word_1_w },
	{ 0x400000, 0x400007, TC0110PCR_step1_word_w },		/* palette (1st screen) */
	{ 0x420000, 0x420007, TC0110PCR_step1_word_1_w },	/* palette (2nd screen) */
	{ 0x600000, 0x6013ff, MWA16_RAM, &spriteram16, &spriteram_size },
//	{ 0x800000, 0x800001, MWA16_NOP },	/* watchdog ?? */
//	{ 0x800008, 0x800009, MWA16_NOP },	/* coin lockout/ctr ? */
//	{ 0x820000, 0x820001, MWA16_NOP },	// ? uses bits 0,2,3
	{ 0x830000, 0x830003, warriorb_sound_w },
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

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( darius2d )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )	// Freezes game
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )  //used, but manual in japanese
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Continuous fire" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "every 500k" )
	PORT_DIPSETTING(    0x0c, "every 700k" )
	PORT_DIPSETTING(    0x08, "every 800k" )
	PORT_DIPSETTING(    0x04, "every 900k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )  // in manual
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( warriorb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	// Freezes game
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Power Ups" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Magician" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/***********************************************************
				GFX DECODING
***********************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 3*4, 2*4, 11*4, 10*4, 1*4, 0*4, 9*4, 8*4, 7*4, 6*4, 15*4, 14*4, 5*4, 4*4, 13*4, 12*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
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

static struct GfxDecodeInfo warriorb_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* scr tiles (screen 1) */
	{ REGION_GFX3, 0, &charlayout,  0, 256 },	/* scr tiles (screen 2) */
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
***********************************************************/

static struct MachineDriver machine_driver_darius2d =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			darius2d_readmem,darius2d_writemem,0,0,
			warriorb_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/4,	/* 4 MHz ??? */
			z80_sound_readmem, z80_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices */
	0,

	/* video hardware */
	81*8, 32*8, { 0*8+4, 80*8+4-1, 3*8, 32*8-1 },

	warriorb_gfxdecodeinfo,
	4096*2, 4096*2,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_DUAL_MONITOR,
	0,
	warriorb_vh_start,
	warriorb_vh_stop,
	warriorb_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_warriorb =
{
	{
		{
			CPU_M68000,
			16000000,	/* 16 MHz ??? */
			warriorb_readmem,warriorb_writemem,0,0,
			warriorb_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/4,	/* 4 MHz ??? */
			z80_sound_readmem, z80_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices */
	0,

	/* video hardware */
	81*8, 32*8, { 0*8+4, 80*8+4-1, 2*8, 32*8-1 },

	warriorb_gfxdecodeinfo,
	4096*2, 4096*2,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_DUAL_MONITOR,
	0,
	warriorb_vh_start,
	warriorb_vh_stop,
	warriorb_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610B,
			&ym2610_interface
		}
	}
};


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( darius2d )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 512K for 68000 code */
	ROM_LOAD16_BYTE( "c07_20-2.74", 0x00000, 0x20000, 0xa0f345b8 )
	ROM_LOAD16_BYTE( "c07_19-2.73", 0x00001, 0x20000, 0x925412c6 )
	ROM_LOAD16_BYTE( "c07_21-2.76", 0x40000, 0x20000, 0xbdd60e37 )
	ROM_LOAD16_BYTE( "c07_18-2.71", 0x40001, 0x20000, 0x23fcd89b )

	ROM_LOAD16_WORD_SWAP( "c07-09.75",   0x80000, 0x80000, 0xcc69c2ce )	/* data rom */

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c07-17.69", 0x00000, 0x04000, 0xae16c905 )
	ROM_CONTINUE(          0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-03.12", 0x00000, 0x80000, 0x189bafce )	/* SCR (screen 1) */
	ROM_LOAD( "c07-04.11", 0x80000, 0x80000, 0x50421e81 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c07-06.27", 0x00000, 0x80000, 0x5eebbcd6 )	/* OBJ */
	ROM_LOAD32_BYTE( "c07-05.24", 0x00001, 0x80000, 0xfb6d0550 )
	ROM_LOAD32_BYTE( "c07-08.25", 0x00002, 0x80000, 0xa07dc846 )
	ROM_LOAD32_BYTE( "c07-07.26", 0x00003, 0x80000, 0xfd9f9e74 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screen 2) */

/* The actual board duplicates the SCR gfx roms for the 2nd TC0100SCN */
//	ROM_LOAD( "c07-03.47", 0x00000, 0x80000, 0x189bafce )
//	ROM_LOAD( "c07-04.48", 0x80000, 0x80000, 0x50421e81 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c07-10.95", 0x00000, 0x80000, 0x4bbe0ed9 )
	ROM_LOAD( "c07-11.96", 0x80000, 0x80000, 0x3c815699 )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "c07-12.107", 0x00000, 0x80000, 0xe0b71258 )

	ROM_REGION( 0x001000, REGION_USER1, 0 )	/* unknown roms */
	ROM_LOAD( "c07-13.37", 0x00000, 0x00400, 0x3ca18eb3 )
	ROM_LOAD( "c07-14.38", 0x00000, 0x00400, 0xbaf2a193 )

// Pals, not dumped
//	ROM_LOAD( "C07-15.78", 0x00000, 0x00?00, 0x00000000 )
//	ROM_LOAD( "C07-16.79", 0x00000, 0x00?00, 0x00000000 )
ROM_END

ROM_START( drius2do )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 512K for 68000 code */
	ROM_LOAD16_BYTE( "c07_20-1.74", 0x00000, 0x20000, 0x48b0804a )
	ROM_LOAD16_BYTE( "c07_19-1.73", 0x00001, 0x20000, 0x1f9a4f83 )
	ROM_LOAD16_BYTE( "c07_21-1.76", 0x40000, 0x20000, 0xb491b0ca )
	ROM_LOAD16_BYTE( "c07_18-1.71", 0x40001, 0x20000, 0xc552e42f )

	ROM_LOAD16_WORD_SWAP( "c07-09.75",   0x80000, 0x80000, 0xcc69c2ce )	/* data rom */

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c07-17.69", 0x00000, 0x04000, 0xae16c905 )
	ROM_CONTINUE(          0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c07-03.12", 0x00000, 0x80000, 0x189bafce )	/* SCR (screen 1) */
	ROM_LOAD( "c07-04.11", 0x80000, 0x80000, 0x50421e81 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c07-06.27", 0x00000, 0x80000, 0x5eebbcd6 )	/* OBJ */
	ROM_LOAD32_BYTE( "c07-05.24", 0x00001, 0x80000, 0xfb6d0550 )
	ROM_LOAD32_BYTE( "c07-08.25", 0x00002, 0x80000, 0xa07dc846 )
	ROM_LOAD32_BYTE( "c07-07.26", 0x00003, 0x80000, 0xfd9f9e74 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x100000 )	/* SCR (screen 2) */

/* The actual board duplicates the SCR gfx roms for the 2nd TC0100SCN */
//	ROM_LOAD( "c07-03.47", 0x00000, 0x80000, 0x189bafce )
//	ROM_LOAD( "c07-04.48", 0x80000, 0x80000, 0x50421e81 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c07-10.95", 0x00000, 0x80000, 0x4bbe0ed9 )
	ROM_LOAD( "c07-11.96", 0x80000, 0x80000, 0x3c815699 )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "c07-12.107", 0x00000, 0x80000, 0xe0b71258 )

	ROM_REGION( 0x001000, REGION_USER1, 0 )	/* unknown roms */
	ROM_LOAD( "c07-13.37", 0x00000, 0x00400, 0x3ca18eb3 )
	ROM_LOAD( "c07-14.38", 0x00000, 0x00400, 0xbaf2a193 )
ROM_END

ROM_START( warriorb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 1024K for 68000 code */
	ROM_LOAD16_BYTE( "d24-20", 0x000000, 0x40000, 0x4452dc25 )
	ROM_LOAD16_BYTE( "d24-19", 0x000001, 0x40000, 0x15c16016 )
	ROM_LOAD16_BYTE( "d24-21", 0x080000, 0x40000, 0x783ef8e1 )
	ROM_LOAD16_BYTE( "d24-18", 0x080001, 0x40000, 0x4502db60 )

	ROM_LOAD16_WORD_SWAP( "d24-09",   0x100000, 0x100000, 0xece5cc59 )	/* data rom */
	/* Note: Raine wrongly doubles up d24-09 as delta-t samples */

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "d24-17",  0x00000, 0x04000, 0xe41e4aae )
	ROM_CONTINUE(        0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d24-02", 0x000000, 0x100000, 0x9f50c271 )	/* SCR A, screen 1 */
	ROM_LOAD( "d24-01", 0x100000, 0x100000, 0x326dcca9 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "d24-06", 0x000000, 0x100000, 0x918486fe )	/* OBJ */
	ROM_LOAD32_BYTE( "d24-03", 0x000001, 0x100000, 0x46db9fd7 )
	ROM_LOAD32_BYTE( "d24-04", 0x000002, 0x100000, 0x148e0493 )
	ROM_LOAD32_BYTE( "d24-05", 0x000003, 0x100000, 0x9f414317 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "d24-07", 0x000000, 0x100000, 0x9f50c271 )	/* SCR B, screen 2 */
	ROM_LOAD( "d24-08", 0x100000, 0x100000, 0x1e6d1528 )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "d24-12", 0x000000, 0x100000, 0x279203a1 )
	ROM_LOAD( "d24-10", 0x100000, 0x100000, 0x0e0c716d )
	ROM_LOAD( "d24-11", 0x200000, 0x100000, 0x15362573 )

	/* No Delta-T samples */
ROM_END



/* Working Games */

GAME( 1989, darius2d, darius2,  darius2d, darius2d, 0, ROT0, "Taito Corporation", "Darius II (dual screen) (Japan)" )
GAME( 1989, drius2do, darius2,  darius2d, darius2d, 0, ROT0, "Taito Corporation", "Darius II (dual screen) (Japan old version)" )
GAME( 1991, warriorb, 0,        warriorb, warriorb, 0, ROT0, "Taito Corporation", "Warrior Blade (Japan)" )

