/***************************************************************************

World Grand Prix	(c) Taito Corporation 1989
================

David Graves

(this is based on the TaitoZ driver. Thanks to Richard Bush and the
Raine team, whose open source was helpful in many areas.)

				*****

World Grand Prix runs on hardware which is pretty different from the
system Taito commonly used for their pseudo-3d racing games of the
time, the Z system. Different screen and sprite hardware is used.
There's also a LAN hookup (for multiple machines).

As well as a TC0100SCN tilemap generator (two 64x64 layers of 8x8
tiles and a layer of 8x8 tiles with graphics data taken from RAM)
there is a pivoting tilemap generator. This generates three
scrollable / row-scrollable rotatable 64x64 tilemaps composed of
16x16 tiles.

As well as these six tilemap layers, there is a sprite layer with
zoomable / rotatable sprites. Big sprites are created from 16x16 gfx
chunks via a sprite mapping area in RAM.

World Grand Prix has twin 68K CPUs which communicate via $4000 bytes
of shared ram.

There is a Z80 as well, which takes over sound duties. Commands are
written to it by the one of the 68000s (the same as Taito F2 games).

Dumper's info (chips from Raine)
-------------

    TC0100SCN - known
    TC0140SYT - known
    TC0170ABT - ?
    TC0220IOC - known
    TC0240PBJ - motion objects ??
    TC0250SCR - piv tilemaps ?? [TC0280GRD was a rotatable tilemap chip in DonDokoD, also 1989]
    TC0260DAR - color related, paired with TC0360PRI in many F2 games
    TC0330CHL - ? (perhaps lan related)
This game has LAN interface board, it uses uPD72105C.

Video Map
=========

400000 area is used for sprites. The game does tile mapping
	in ram to create big sprites from the 16x16 tiles of gfx2.
	See notes in \vidhrdw\ for details.

500000 area is for the "piv" tilemaps. (The unused gaps look as though
	Taito considered making their custom chip capable of four
	rather than three piv tilemaps.)

500000 - 501fff : unknown/unused
502000 - 507fff : piv tilemaps 0-2 [tile numbers only]

508000 - 50ffff : this area relates to pixel rows in each piv tilemap.
	Includes rowscroll for the piv tilemaps, 2 (3?) of which act as a
	simple road. To curve, it has to have rowscroll applied to each
	row.

508000 - 5087ff unknown/unused

508800 piv0 row color bank (& row horizontal zoom?)
509000 piv1 row color bank (& row horizontal zoom?)
509800 piv2 row color bank (& row horizontal zoom?)

	Initially full of 0x007f (0x7f=default row horizontal zoom ?).
	In-game the high bytes are set to various values (0 - 0x2b).
	0xc00 words equates to 3*32x32 words [3 tilemaps x 1024 words].

	The high byte could be a color offset for each 4 tiles, but
	I think it is almost certainly color offset per pixel row. This
	fits in with it living next to the three rowscroll areas (below).
	And it would give a much better illusion of road movement,
	currently pretty poor.

	(I use it as offset for every 4 tiles, as I don't know how
	to make it operate on a pixel row.)

50a000  piv0 rowscroll [sky]  (not seen values here yet)
50c000  piv1 rowscroll [road] (values 0xfd00 - 0x400)
50e000  piv2 rowscroll [road] (values 0xfd00 - 0x403)

510000 - 511fff : unknown/unused
512000 - 517fff : piv tilemaps 0-2 [tile colors, flip bits?]


TODO
====

Analogue brake pedal works but won't register in service mode.

$28336 is code that shows brake value in service mode.
$ac3e sub (called off int4) at $ac78 does three calcs to the six
	AD values: results are stored at ($d22 / $d24 / $d26,A5)
	It is the third one, ($d26,A5) which gets displayed
	in service mode as brake.

Some junky looking brown sprites in-game. Perhaps mask sprites,
like the ones occasionally seen in ChaseHQ [taitoz driver]?

Color banking system for piv tilemaps is not yet right. I'm using
color bank offsets from part of the piv ram area, making 1 word
control 4 16x16 piv tiles. But I think each color bank word
*should* control 1 pixel row of that piv tilemap. So we want
row color bank control (as well as rowscroll). This should
give the impression that the road lines are moving down the
screen, when the road tiles are actually static.

There are four piv control words that we just ignore: they seem
to be fixed. Where is the piv layer rotation control?

Implement better positioning + rotation for sprites. The
current sprite coord calculations are kludgy and flawed and
ignore four control words. The sprites also seem jerky
and have timing glitches.

				*****

[Used to stop with LAN error. (Looks like CPUB tells CPUA what is wrong
with LAN in shared ram $142048. Examined at $e57c which prints out
relevant lan error message). Ended up at $e57c from $b14e-xx code
section. CPUA does PEA of $e57c which is the fallback if CPUB doesn't
respond in timely fashion to command 0x1 poked by code at $b1a6.

CPUB $830c-8332 loops waiting for command 1-8 in $140002 from CPUA.
it executes this then clears $140002 to indicate command completed.
It wasn't clearing this fast enough for CPUA, because $142048 wasn't
clear:- and that affects the amount of code command 0x1 runs.

CPUB code at $104d8 had already detected error on LAN and set $142048
to 4. We now return correct lan status read from $380000, so
no LAN problem is detected.]

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

int wgp_vh_start (void);
void wgp_vh_stop (void);
void wgp_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);

static int old_cpua_ctrl = 0xff;

//static data16_t *wgp_ram;

extern data16_t *wgp_spritemap;
extern size_t    wgp_spritemap_size;
READ16_HANDLER ( wgp_spritemap_word_r );
WRITE16_HANDLER( wgp_spritemap_word_w );

extern data16_t *wgp_pivram;
READ16_HANDLER ( wgp_pivram_word_r );
WRITE16_HANDLER( wgp_pivram_word_w );

extern data16_t *wgp_piv_ctrlram;
READ16_HANDLER ( wgp_piv_ctrl_word_r );
WRITE16_HANDLER( wgp_piv_ctrl_word_w );

static data16_t *sharedram;
size_t sharedram_size;

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
		data = data >> 8;	/* needed for Wgp */

	/* bit 0 enables cpu B */

	if ((data &0x1)!=(old_cpua_ctrl &0x1))	// perhaps unnecessary but it's often written with same value
		cpu_set_reset_line(2,(data &0x1) ? CLEAR_LINE : ASSERT_LINE);

	/* is there an irq enable ??? */

	old_cpua_ctrl = data;

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",cpu_get_pc(),data);
}


/***********************************************************
				INTERRUPTS
***********************************************************/

/* 68000 A */

void wgp_interrupt6(int x)
{
	cpu_cause_interrupt(0,6);
}

/* 68000 B */

void wgp_cpub_interrupt4(int x)
{
	cpu_cause_interrupt(2,4);	// assumes Z80 sandwiched between the 68Ks
}

void wgp_cpub_interrupt6(int x)
{
	cpu_cause_interrupt(2,6);	// assumes Z80 sandwiched between the 68Ks
}



/***** Routines for particular games *****/

static int wgp_interrupt(void)
{
	return 4;
}

/* FWIW offset of 10000 on ints gets CPUB obeying the
   first CPUA command the same frame, maybe not necessary */

static int wgp_cpub_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(10000,0),0, wgp_cpub_interrupt4);
	timer_set(TIME_IN_CYCLES(10500,0),0, wgp_cpub_interrupt6);
	return 0;
}


/**********************************************************
				GAME INPUTS
**********************************************************/

static READ16_HANDLER( lan_status_r )
{
	logerror("CPU #2 PC %06x: warning - read lan status\n",cpu_get_pc());

	return  (0x4 << 8);	// CPUB expects this value in code at $104d0
}

static WRITE16_HANDLER( unknown_port_w )
{
	// standard pattern: port number (0-7) in 2nd word
	// and data in 1st word. Perhaps this relates to
	// sprite/tile and layer priorities ?

	switch (offset)
	{
		case 0x00:
		{
//logerror("CPU #0 PC %06x: warning - port write %04x\n",cpu_get_pc(),data);
			return;
		}

		case 0x01:
		{
//logerror("CPU #0 PC %06x: warning - select port %04x\n",cpu_get_pc(),data);
		}
	}
}

static READ16_HANDLER( wgp_input_r )
{
	switch (offset)
	{
		case 0x00:
			return input_port_3_word_r(0) << 8;	/* DSW A */

		case 0x01:
			return input_port_4_word_r(0) << 8;	/* DSW B */

		case 0x02:
			return input_port_0_word_r(0) << 8;	/* IN0 */

		case 0x03:
			return input_port_1_word_r(0) << 8;	/* IN1 */

		case 0x07:
			return input_port_2_word_r(0) << 8;	/* IN2 */
	}

logerror("CPU #0 PC %06x: warning - read unmapped input offset %06x\n",cpu_get_pc(),offset);

	return 0x00;
}

static READ16_HANDLER( wgp_adinput_r )
{
	UINT16 steer = 0x40;

	if (input_port_5_word_r(0) & 0x8)	/* pressing down */
		steer = 0x0d;

	if (input_port_5_word_r(0) & 0x4)	/* pressing up */
		steer = 0x73;

	if (input_port_5_word_r(0) & 0x2)	/* pressing right */
		steer = 0x00;

	if (input_port_5_word_r(0) & 0x1)	/* pressing left */
		steer = 0x80;

	switch (offset)
	{
		case 0x00:
		{
			if (input_port_5_word_r(0) &0x40)	/* pressing accel */
				return 0xff;
			else
				return 0x00;
		}

		case 0x01:
			return steer;

		case 0x02:
			return 0xc0; 	/* steer offset, correct acc. to service mode */

		case 0x03:
			return 0xbf;	/* accel offset, correct acc. to service mode */

		case 0x04:
		{
			if (input_port_5_word_r(0) &0x80)	/* pressing brake */
				return 0xcf;
			else
				return 0xff;
		}

		case 0x05:
			return input_port_6_word_r(0);	/* unknown */
	}

logerror("CPU #0 PC %06x: warning - read unmapped a/d input offset %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

static WRITE16_HANDLER( wgp_adinput_w )
{
	/* Each write invites a new interrupt as soon as the
	   hardware has got the next a/d conversion ready. We set a token
	   delay of 10000 cycles although our inputs are always ready. */

	timer_set(TIME_IN_CYCLES(10000,0),0, wgp_interrupt6);
}


/**********************************************************
				SOUND
**********************************************************/

static WRITE_HANDLER( bankswitch_w )	// assumes Z80 sandwiched between 68Ks
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 7;

#ifdef MAME_DEBUG
	if (banknum>3) logerror("CPU#3 (Z80) switch to ROM bank %06x: should only happen if Z80 prg rom is 128K!\n",banknum);
#endif
	cpu_setbank (10, &RAM [0x10000 + (banknum * 0x4000)]);
}

WRITE16_HANDLER( wgp_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);
#ifdef MAME_DEBUG
	if (data & 0xff00)
	{
		char buf[80];

		sprintf(buf,"wgp_sound_w to high byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ16_HANDLER( wgp_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff));
	else return 0;
}


/*****************************************************************
				 MEMORY STRUCTURES
*****************************************************************/

static MEMORY_READ16_START( wgp_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },	/* main CPUA ram */
	{ 0x140000, 0x143fff, sharedram_r },
	{ 0x180000, 0x18000f, wgp_input_r },
	{ 0x200000, 0x20000f, wgp_adinput_r },
	{ 0x300000, 0x30ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_r },
	{ 0x400000, 0x40bfff, wgp_spritemap_word_r },	/* sprite tilemaps */
	{ 0x40c000, 0x40dfff, MRA16_RAM },	/* sprite ram */
	{ 0x500000, 0x501fff, MRA16_RAM },	/* unknown/unused */
	{ 0x502000, 0x517fff, wgp_pivram_word_r },	/* piv tilemaps */
	{ 0x520000, 0x52001f, wgp_piv_ctrl_word_r },
	{ 0x700000, 0x701fff, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( wgp_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x140000, 0x143fff, sharedram_w, &sharedram, &sharedram_size },
	{ 0x180000, 0x180001, MWA16_NOP },	/* watchdog ?? */
	{ 0x1c0000, 0x1c0001, cpua_ctrl_w },
	{ 0x200000, 0x20000f, wgp_adinput_w },
	{ 0x300000, 0x30ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_w },
	{ 0x400000, 0x40bfff, wgp_spritemap_word_w, &wgp_spritemap, &wgp_spritemap_size },
	{ 0x40c000, 0x40dfff, MWA16_RAM, &spriteram16, &spriteram_size  },	/* sprite ram */
	{ 0x40fff0, 0x40fff1, MWA16_NOP },	// ?? (writes 0x8000 and 0 alternately)
	{ 0x500000, 0x501fff, MWA16_RAM },	/* unknown/unused */
	{ 0x502000, 0x517fff, wgp_pivram_word_w, &wgp_pivram },	/* piv tilemaps */
	{ 0x520000, 0x52001f, wgp_piv_ctrl_word_w, &wgp_piv_ctrlram },
	{ 0x600000, 0x600003, unknown_port_w },
	{ 0x700000, 0x701fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( wgp_cpub_readmem )	// LAN areas not mapped
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x140000, 0x143fff, sharedram_r },
	{ 0x200000, 0x200003, wgp_sound_r },
	{ 0x380000, 0x380001, lan_status_r },	// ??
	// a lan input area is read somewhere above the status
	// (make the status return 0 and log)...
MEMORY_END

static MEMORY_WRITE16_START( wgp_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x140000, 0x143fff, sharedram_w, &sharedram },
	{ 0x200000, 0x200003, wgp_sound_w },
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

INPUT_PORTS_START( wgp )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON7 | IPF_PLAYER1 )	// freeze
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON4 | IPF_PLAYER1 )	// shift up
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	// shift down
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )	// "start lump" (lamp?)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	// "brake lump" (lamp?)
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

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

	PORT_START /* DSW B, copied from ChaseHQ, probably wrong */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Setting" )
	PORT_DIPSETTING(    0x0c, "60 Seconds" )
	PORT_DIPSETTING(    0x08, "70 Seconds" )
	PORT_DIPSETTING(    0x04, "85 Seconds" )
	PORT_DIPSETTING(    0x00, "55 Seconds" )
	PORT_DIPNAME( 0x10, 0x10, "Turbos Stocked" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Damage Cleared at Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* fake inputs, for steering etc. */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	// accel
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	// brake

/* It's not clear for accel and brake which is the input and
   which the offset, but that doesn't matter. These continuous
   inputs are replaced by discrete values derived from the fake
   input port above, so keyboard control is feasible. */

//	PORT_START	/* accel, 0-255 */
//	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER1, 20, 10, 0, 0xff)

//	PORT_START	/* steer -64 to +64 */
//	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 20, 10, 0, 0x80)

//	PORT_START	/* steer offset */

//	PORT_START	/* accel offset */

//	PORT_START	/* brake, 0-0x30: needs to start at 0xff; then 0xcf is max brake */
//	PORT_ANALOG( 0xff, 0xff, IPT_AD_STICK_X | IPF_PLAYER2, 10, 5, 0xcf, 0xff)

	PORT_START	/* unknown */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER2, 20, 10, 0, 0xff)
INPUT_PORTS_END


/***********************************************************
				GFX DECODING
***********************************************************/

static struct GfxLayout wgp_tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 5*4, 4*4, 3*4, 2*4, 7*4, 6*4, 9*4, 8*4, 13*4, 12*4, 11*4, 10*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout wgp_tile2layout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 7*4, 6*4, 15*4, 14*4, 5*4, 4*4, 13*4, 12*4, 3*4, 2*4, 11*4, 10*4, 1*4, 0*4, 9*4, 8*4 },
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

/* taitoic.c TC0100SCN routines expect scr stuff to be in second gfx
   slot */

static struct GfxDecodeInfo wgp_gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0x0, &wgp_tilelayout,  0, 256 },		/* sprites */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },		/* sprites & playfield */
	{ REGION_GFX2, 0x0, &wgp_tile2layout,  0, 256 },	/* piv */
	{ -1 } /* end of array */
};


/**************************************************************
			     YM2610 (SOUND)
**************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)	// assumes Z80 sandwiched between 68Ks
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

Wgp has high interleaving to prevent "common ram error"
on startup.
***********************************************************/

static struct MachineDriver machine_driver_wgp =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			wgp_readmem,wgp_writemem,0,0,
			wgp_interrupt, 1
		},
		{																			\
			CPU_Z80 | CPU_AUDIO_CPU,												\
			16000000/4,	/* 4 MHz ??? */													\
			z80_sound_readmem, z80_sound_writemem,0,0,										\
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */				\
		},																			\
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			wgp_cpub_readmem,wgp_cpub_writemem,0,0,
			wgp_cpub_interrupt, 1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	200,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	wgp_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	wgp_vh_start,
	wgp_vh_stop,
	wgp_vh_screenrefresh,

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

ROM_START( wgp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c32-25",      0x00000, 0x20000, 0x0cc81e77 )
	ROM_LOAD16_BYTE( "c32-29",      0x00001, 0x20000, 0xfab47cf0 )
	ROM_LOAD16_WORD_SWAP( "c32-10", 0x80000, 0x80000, 0xa44c66e9 )	/* data rom */

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c32-28", 0x00000, 0x20000, 0x38f3c7bf )
	ROM_LOAD16_BYTE( "c32-27", 0x00001, 0x20000, 0xbe2397fb )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c32-24",   0x00000, 0x04000, 0xe9adb447 )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c32-09", 0x00000, 0x80000, 0x96495f35 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c32-04", 0x000000, 0x80000, 0x473a19c9 )	/* PIV */
	ROM_LOAD32_BYTE( "c32-03", 0x000001, 0x80000, 0x9ec3e134 )
	ROM_LOAD32_BYTE( "c32-02", 0x000002, 0x80000, 0xc5721f3a )
	ROM_LOAD32_BYTE( "c32-01", 0x000003, 0x80000, 0xd27d7d93 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c32-05", 0x000000, 0x80000, 0x3698d47a )	/* OBJ */
	ROM_LOAD16_BYTE( "c32-06", 0x000001, 0x80000, 0xf0267203 )
	ROM_LOAD16_BYTE( "c32-07", 0x100000, 0x80000, 0x743d46bd )
	ROM_LOAD16_BYTE( "c32-08", 0x100001, 0x80000, 0xfaab63b0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c32-11", 0x00000, 0x80000, 0x2b326ff0 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "c32-12", 0x00000, 0x80000, 0xdf48a37b )

//	(no unused roms in my set)
ROM_END

ROM_START( wgpj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c32-48",      0x00000, 0x20000, 0x819cc134 )
	ROM_LOAD16_BYTE( "c32-49",      0x00001, 0x20000, 0x4a515f02 )
	ROM_LOAD16_WORD_SWAP( "c32-10", 0x80000, 0x80000, 0xa44c66e9 )	/* data rom */

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c32-28", 0x00000, 0x20000, 0x38f3c7bf )
	ROM_LOAD16_BYTE( "c32-27", 0x00001, 0x20000, 0xbe2397fb )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c32-24",   0x00000, 0x04000, 0xe9adb447 )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c32-09", 0x00000, 0x80000, 0x96495f35 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c32-04", 0x000000, 0x80000, 0x473a19c9 )	/* PIV */
	ROM_LOAD32_BYTE( "c32-03", 0x000001, 0x80000, 0x9ec3e134 )
	ROM_LOAD32_BYTE( "c32-02", 0x000002, 0x80000, 0xc5721f3a )
	ROM_LOAD32_BYTE( "c32-01", 0x000003, 0x80000, 0xd27d7d93 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c32-05", 0x000000, 0x80000, 0x3698d47a )	/* OBJ */
	ROM_LOAD16_BYTE( "c32-06", 0x000001, 0x80000, 0xf0267203 )
	ROM_LOAD16_BYTE( "c32-07", 0x100000, 0x80000, 0x743d46bd )
	ROM_LOAD16_BYTE( "c32-08", 0x100001, 0x80000, 0xfaab63b0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c32-11", 0x00000, 0x80000, 0x2b326ff0 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "c32-12", 0x00000, 0x80000, 0xdf48a37b )

//	(no unused roms in my set)
ROM_END


void init_wgp(void)
{
//	taitosnd_setz80_soundcpu( 2 );
	old_cpua_ctrl = 0xff;
}


/* Working Games with graphics problems */

GAMEX( 1989, wgp,  0,   wgp, wgp, wgp, ROT0, "Taito America Corporation", "World Grand Prix (US)", GAME_IMPERFECT_COLORS )
GAMEX( 1989, wgpj, wgp, wgp, wgp, wgp, ROT0, "Taito Corporation", "World Grand Prix (Japan)", GAME_IMPERFECT_COLORS )

