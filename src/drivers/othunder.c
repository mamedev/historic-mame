/***************************************************************************

Operation Thunderbolt  (Taito)
---------------------

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime,
Nicola Salmoria. Thanks to Richard Bush and the Raine team, whose open
source was very helpful in many areas particularly the sprites.)

				*****

Operation Thunderbolt operates on hardware very similar to the Taito Z
system, in particular to the game Spacegun. The lightgun hardware in these
two is identical. The eerom and the calibration process also look very much
alike.

Like Spacegun, this game has 4 separate layers of graphics - one 64x64
tiled scrolling background plane of 8x8 tiles, a similar foreground plane,
a sprite plane [with varying properties], and a text plane with character
definitions held in ram.

The sprites are 16x8 tiles aggregated through a spritemap rom into 64x64
sprites - identical to Spacegun.

The big difference is that Operation Thunderbolt only uses a *single* 68000
CPU, whereas Spacegun has twin 68K CPUs. Operation Thunderbolt has a Z80
taking over sound duties, which Spacegun doesn't.

Commands are written to the Z80 by the 68000 (the same as Taito F2 games).


TODO
====

We need to have artificial P1/2 targets [I put in a fake extra dip
to enable/disable]. Problem is that the game does not update screen
coords for your aim except (i) when you shoot (ii) when you have
the power up which shows you where you are aiming. So they aren't
any good to us: we will have to work with the raw analogue inputs?

Light gun interrupt timing is arbitrary.

TC0100SCN problem: text vs. bg0/1 offsets seem wrong: first level
wants bg layers 4 further right than usual. Some cut screens want
them 4 further left. This may be a flaw in assumptions made in
vidhrdw\taitoic.c, or related to game being ORIENTATION_FLIP_X ?

DIPs


Aim coordinate documentation
----------------------------

RAM:	$82732/4 P1x/y aim sprite coords
	$82736/8 P2x/y aim sprite coords

$1c58a: calls $1c626 (if p2 exists?) and $1c6f4 (if p1 exists?).

PLAYER 1
$1c6f4 code at $1c74e calls $1436
		$1c754 subtracts 32 from P1x and P1y aim sprite coords

$1436 calls $a932 which does P1 raw lightgun coord conversion,
	based on calibration info from eerom [eerom data $8a000 on].
	Raw x/y coords in $82848/a.

PLAYER 2
$1c626 code at $1c680 calls $15de
		$1c686 subtracts 32 from P2x and P2y aim sprite coords

$15de calls $aa48 which does P2 raw lightgun coord conversion,
	based on calibration info from eerom [eerom data $8a000 on].
	Raw x/y coords in $8284c/e.


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/eeprom.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

int othunder_vh_start (void);
void othunder_vh_stop (void);

void othunder_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);

static data16_t eep_latch = 0;

//static data16_t *othunder_ram;


/***********************************************************
				INTERRUPTS
***********************************************************/

static int othunder_interrupt(void)
{
	return 5;
}

void othunder_gun_interrupt(int x)
{
	cpu_cause_interrupt(0,6);
}


/******************************************************************
					EEPROM

This is an earlier version of the eeprom used in some TaitoB games.
The eeprom unlock command is different, and the write/clock/reset
bits are different.
******************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* lock command */
	"0100111111" 	/* unlock command */
};

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)  EEPROM_load(file);
		else       usrintf_showmessage("You MUST calibrate guns in service mode");
	}
}

static int eeprom_r(void)
{
	return (EEPROM_read_bit() & 0x01)<<7;
}

static READ16_HANDLER( eep_latch_r )
{
	return eep_latch;
}

static WRITE16_HANDLER( eeprom_w )
{
/*
	0000xxxx	(unused)
	000x0000	eeprom reset (active low)
	00x00000	eeprom clock
	0x000000	eeprom data
	x0000000	(unused)
*/

	COMBINE_DATA(&eep_latch);

	EEPROM_write_bit(data & 0x40);
	EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);

#if 0
	logerror("CPU #1 (2nd 68000) PC %06x: warning - write eeprom, data %04x\n",cpu_get_pc(),data);
#endif
}


/**********************************************************
			GAME INPUTS
**********************************************************/

static READ16_HANDLER( othunder_input_r )
{
	switch (offset)
	{
		case 0x00:
			return input_port_2_word_r(0);	/* DSW A */

		case 0x01:
			return input_port_3_word_r(0);	/* DSW B */

		case 0x02:
			return input_port_0_word_r(0);	/* IN0 */

		case 0x03:
			return eeprom_r();

		case 0x07:
			return input_port_1_word_r(0);	/* IN1 */
	}

logerror("CPU #0 PC %06x: warning - read unmapped input offset %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

static READ16_HANDLER( othunder_lightgun_r )
{
	switch (offset)
	{
		case 0x00:
			return input_port_5_word_r(0);	/* P1X */

		case 0x01:
			return input_port_6_word_r(0);	/* P1Y */

		case 0x02:
			return input_port_7_word_r(0);	/* P2X */

		case 0x03:
			return input_port_8_word_r(0);	/* P2Y */
	}

logerror("CPU #0 lightgun_r offset %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),offset);

	return 0x0;
}

static WRITE16_HANDLER( othunder_lightgun_w )
{
	/* Each write invites a new lightgun interrupt as soon as the
	   hardware has got the next coordinate ready. We set a token
	   delay of 10000 cycles; our "lightgun" coords are always ready
	   but we don't want CPUB to have an int5 before int4 is over (?).

	   Four lightgun interrupts happen before the collected coords
	   are moved to shared ram where CPUA can use them. */

	timer_set(TIME_IN_CYCLES(10000,0),0, othunder_gun_interrupt);
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

WRITE16_HANDLER( othunder_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);

#ifdef MAME_DEBUG
	if (data & 0xff00)
	{
		char buf[80];

		sprintf(buf,"othunder_sound_w to high byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ16_HANDLER( othunder_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff));
	else return 0;
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( othunder_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x08ffff, MRA16_RAM },	/* main ram */
	{ 0x090000, 0x09000f, othunder_input_r },
	{ 0x100000, 0x100007, TC0110PCR_word_r },	/* palette */
	{ 0x200000, 0x20ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_r },
	{ 0x300000, 0x300003, othunder_sound_r },
	{ 0x400000, 0x4005ff, MRA16_RAM },	/* sprite ram */
	{ 0x500000, 0x500007, othunder_lightgun_r },
MEMORY_END

static MEMORY_WRITE16_START( othunder_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x08ffff, MWA16_RAM },
	{ 0x090000, 0x090001, MWA16_NOP },   /* watchdog ?? (alternates 1 and 0xffff) */
	{ 0x090006, 0x090007, eeprom_w },
	{ 0x090008, 0x090009, MWA16_NOP },   /* coin ctr, lockout ? */
	{ 0x09000c, 0x09000d, MWA16_NOP },   /* ?? (keeps writing 0x77) */
	{ 0x100000, 0x100007, TC0110PCR_step1_rbswap_word_w },	/* palette */
	{ 0x200000, 0x20ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_w },
	{ 0x300000, 0x300003, othunder_sound_w },
	{ 0x400000, 0x4005ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x500000, 0x500007, othunder_lightgun_w },
	{ 0x600000, 0x600003, MWA16_NOP },   /* zeros written: see code at $854 */
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

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

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

INPUT_PORTS_START( othunder )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Magazines/Rockets" )
	PORT_DIPSETTING(    0x0c, "5/3" )
	PORT_DIPSETTING(    0x08, "6/4" )
	PORT_DIPSETTING(    0x04, "7/5" )
	PORT_DIPSETTING(    0x00, "8/6" )
	PORT_DIPNAME( 0x30, 0x30, "Bullets per Magazine" )
	PORT_DIPSETTING(    0x00, "30" )
	PORT_DIPSETTING(    0x10, "35" )
	PORT_DIPSETTING(    0x30, "40" )
	PORT_DIPSETTING(    0x20, "50" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

	PORT_START	/* Fake DSW */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME, "Show gun target", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	/* speed of 13 is compromise between moving aim around screen fast
	   enough and being accurate enough not to miss targets. 20 is too
	   inaccurate, and 10 is too slow. */

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 25, 13, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 25, 13, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 25, 13, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER2, 25, 13, 0, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( othundu )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Magazines/Rockets" )
	PORT_DIPSETTING(    0x0c, "5/3" )
	PORT_DIPSETTING(    0x08, "6/4" )
	PORT_DIPSETTING(    0x04, "7/5" )
	PORT_DIPSETTING(    0x00, "8/6" )
	PORT_DIPNAME( 0x30, 0x30, "Bullets per Magazine" )
	PORT_DIPSETTING(    0x00, "30" )
	PORT_DIPSETTING(    0x10, "35" )
	PORT_DIPSETTING(    0x30, "40" )
	PORT_DIPSETTING(    0x20, "50" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

	PORT_START	/* Fake DSW */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME, "Show gun target", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	/* speed of 13 is compromise between moving aim around screen fast
	   enough and being accurate enough not to miss targets. 20 is too
	   inaccurate, and 10 is too slow. */

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 25, 13, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 25, 13, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 25, 13, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER2, 25, 13, 0, 0xff)
INPUT_PORTS_END


/***********************************************************
				GFX DECODING
***********************************************************/

static struct GfxLayout tile16x8_layout =
{
	16,8,	/* 16*8 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8	/* every sprite takes 64 consecutive bytes */
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

static struct GfxDecodeInfo othunder_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* sprites & playfield */
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

static struct MachineDriver machine_driver_othunder =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			othunder_readmem,othunder_writemem,0,0,
			othunder_interrupt, 1
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
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	othunder_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	othunder_vh_start,
	othunder_vh_stop,
	othunder_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	},

	nvram_handler	/* for the eerom */

};


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( othunder )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b67-20",    0x00000, 0x20000, 0x21439ea2 )
	ROM_LOAD16_BYTE( "b67-23",    0x00001, 0x20000, 0x789e9daa )
	ROM_LOAD16_BYTE( "b67-14.61", 0x40000, 0x20000, 0x7f3dd724 )
	ROM_LOAD16_BYTE( "b67-15.62", 0x40001, 0x20000, 0xe84f62d0 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "b67-13.40",   0x00000, 0x04000, 0x2936b4b1 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b67-06", 0x00000, 0x80000, 0xb9a38d64 )		/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b67-01", 0x00000, 0x80000, 0x81ad9acb )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD32_BYTE( "b67-02", 0x00001, 0x80000, 0xc20cd2fb )
	ROM_LOAD32_BYTE( "b67-03", 0x00002, 0x80000, 0xbc9019ed )
	ROM_LOAD32_BYTE( "b67-04", 0x00003, 0x80000, 0x2af4c8af )

	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b67-05", 0x00000, 0x80000, 0x9593e42b )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b67-08", 0x00000, 0x80000, 0x458f41fb )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "b67-07", 0x00000, 0x80000, 0x4f834357 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
//	ROM_LOAD( "b67-09", 0x00000, 0xd56, 0x130fd2ab )	/* pals ? */
//	ROM_LOAD( "b67-10", 0x00000, 0xcd5, 0x312f9e2a )
//	ROM_LOAD( "b67-11", 0x00000, 0xada, 0xf863b864 )
//	ROM_LOAD( "b67-12", 0x00000, 0xcd5, 0x653d86bb )
ROM_END

ROM_START( othundu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b67-20-1.63", 0x00000, 0x20000, 0x851a453b )
	ROM_LOAD16_BYTE( "b67-22-1.64", 0x00001, 0x20000, 0x19480dc0 )
	ROM_LOAD16_BYTE( "b67-14.61",   0x40000, 0x20000, 0x7f3dd724 )
	ROM_LOAD16_BYTE( "b67-15.62",   0x40001, 0x20000, 0xe84f62d0 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "b67-13.40",   0x00000, 0x04000, 0x2936b4b1 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b67-06", 0x00000, 0x80000, 0xb9a38d64 )		/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b67-01", 0x00000, 0x80000, 0x81ad9acb )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD32_BYTE( "b67-02", 0x00001, 0x80000, 0xc20cd2fb )
	ROM_LOAD32_BYTE( "b67-03", 0x00002, 0x80000, 0xbc9019ed )
	ROM_LOAD32_BYTE( "b67-04", 0x00003, 0x80000, 0x2af4c8af )

	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b67-05", 0x00000, 0x80000, 0x9593e42b )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b67-08", 0x00000, 0x80000, 0x458f41fb )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "b67-07", 0x00000, 0x80000, 0x4f834357 )
ROM_END




GAME( 1988, othunder, 0,        othunder, othunder, 0,        ORIENTATION_FLIP_X, "Taito Corporation Japan", "Operation Thunderbolt (World)" )
GAME( 1988, othundu,  othunder, othunder, othundu,  0,        ORIENTATION_FLIP_X, "Taito America Corporation", "Operation Thunderbolt (US)" )

