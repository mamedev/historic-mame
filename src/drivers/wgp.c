/***************************************************************************

World Grand Prix	(c) Taito Corporation 1989
================

David Graves

(Thanks to Richard Bush and the Raine team for their
preliminary driver.)

				*****

World Grand Prix runs on hardware which is pretty different from the
system Taito commonly used for their pseudo-3d racing games of the
time, the Z system. Different screen and sprite hardware is used.
There's also a LAN hookup (for multiple machines).

As well as a TC0100SCN tilemap generator (two 64x64 layers of 8x8
tiles and a layer of 8x8 tiles with graphics data taken from RAM)
there's a "piv" tilemap generator, which creates three scrollable
row-scrollable zoomable 64x64 tilemaps composed of 16x16 tiles.

As well as these six tilemap layers, there is a sprite layer with
zoomable / rotatable sprites. Big sprites are created from 16x16 gfx
chunks via a sprite mapping area in RAM.

The piv and sprite layers are rotatable (but not individually, only
together).

World Grand Prix has twin 68K CPUs which communicate via $4000 bytes
of shared ram.

There is a Z80 as well, which takes over sound duties. Commands are
written to it by the one of the 68000s (the same as Taito F2 games).

Dumper's info (Raine)
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
---------

400000 area is used for sprites. The game does tile mapping
	in ram to create big sprites from the 16x16 tiles of gfx0.
	See notes in \vidhrdw\ for details.

500000 area is for the "piv" tilemaps.
	See notes in \vidhrdw\ for details.


TODO
====

Offer fake-dip selectable analogue steer

Is piv/sprite layers rotation control at 0x600000 ?

Verify y-zoom is correct on the stages that use it (including Wgp2
default course). Row zoom may be hard to verify, but Wgp2 course
selection screen is probably a good test.

Implement proper positioning/zoom/rotation for sprites.

(The current sprite coord calculations are kludgy and flawed and
ignore four control words. The sprites also seem jerky
and have [int?] timing glitches.)

DIP coinage


Wgp
---

Analogue brake pedal works but won't register in service mode.

$28336 is code that shows brake value in service mode.
$ac3e sub (called off int4) at $ac78 does three calcs to the six
	AD values: results are stored at ($d22 / $d24 / $d26,A5)
	It is the third one, ($d26,A5) which gets displayed
	in service mode as brake.


Wgp2
----

Piv y zoom may be imperfect. Check the up/down hill part of the
default course. The road looks a little odd.

Sprite colors seem ok except smoke after you crash. (And one sign on
first bend of default course doesn't go yellow for a few frames.)

[Used to die with common ram error. When CPUA enables CPUB, CPUB
writes to $140000/2 - unfortunately while CPUA is in the middle of
testing that ram. We hack prog for CPUB to disable the writes.]


				*****

[Wgp stopped with LAN error. (Looks like CPUB tells CPUA what is wrong
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


Code Documentation
------------------

CPUA
----

$1064e main game loop starting with a trap#$5

Calls $37e78 sub off which spriteram eventually gets updated

Strangely main loop does not seem to be synced to any interrupt.
This may be why the sprites are so glitchy and don't seem to
update every frame. Maybe trap#$5 should be getting us to a known
point in the frame ??

$21f4 copies sprite tilemapping data from data rom to tilemap
area then flows into:

$223c code that fills a sprite entry in spriteram

[$12770 - $133cd seems to be an irregular lookup table used to
calculate extra zoom word poked into each sprite entry.]

$23a8 picks data out of data rom and stores it to sprite tile-
mapping area by heading through to $21f4. (May also enter sprite
details into spriteram.) It uses $a0000 area of data rom.

(Alternate entry point at $23c2 uses $90000 area of data rom.)

$25ee routine stores data from rom into sprite tilemapping area
including the bad parts that produce junk sprites.

It calls interesting sub at $25be which has a table of the number
of sequential words to be pulled out of the data rom, depending
on the first word value in the data rom for that entry ("code").
Each code will pull out the following multiple of 16 words:

	Code  Words   Tiles    Actual data rom entry
	 0      1      4x4      [same]
	 1      2      8x4      [same]
	 2      4      8x8      [same]
	 3      3      12x4     [same, see $98186]
	 4      6      12x8
	 5      9      12x12    [same]
	 6      2      4x8      [WRONG! says 8x12 in data rom, see $982bf]
	 7      6      8x12     [WRONG! says 4x8 in data rom]
	 8      1      (2x2)*4  [2x2 in data rom]  (copies 12 unwanted
	                                  words - causing junk sprites)

$4083c0-47f in sprite mapping area has junk words - due to code 7
making it read 6*16 words. 0x60 words are copied from the data rom
when 0x20 would be correct. Careless programming: in the lookup
table Taito got codes 6 and 7 back to front. Enable the patch in
init_wgp to correct this... I can't see what changes.

I'm guessing sprites may be variable size, and the junk sprites
mapped +0x9b80-9d80 are 2x2 tiles rather than 4x4.

If we only use the first 4 tilemapping words, then the junk sprites
look fine. But their spacing is bad: they have gaps left between
them. They'll need to be magnified to 2x size - the pixel zoom
value must do this automatically.

This ties in with the lookup table we need to draw the sprites:
it makes sense if our standard 4x4 tile sprite is actually 4 2x2
sprites.

But what tells the sprite hardware if a sprite is 2x2 or 4x4 ??


Data rom entry
--------------

+0x00  Control word
        (determines how many words copied to tilemapping area)

+0x01  Sprite X size  [tiles]
        (2,4,8 or 12)

+0x02  Sprite Y size  [tiles]
        (2,4,8 or 12)

+0x03  sprite tile words, total (X size * Y size)
 on...

The X size and Y size values don't seem to be used by the game, and
may be a hangover from the gfx development system used.


Data Rom
--------

$80000-$8ffff   piv tilemaps, preceded by lookup table
$90000-$9ffff   sprite tilemap data, preceded by lookup table
$a0000-$affff   sprite tilemap data, preceded by lookup table
$b0000-$cffff   TC0100SCN tilemaps, preceded by lookup table

	Note that only just over half this space is used, rest is
	0xff fill.

$d0000-$dffff is the pivot port data table

	Four separate start points are available, contained in the
	first 4 long words.

	(Data words up to $da410 then 0xff fill)

$e0000-$e7ffe is a logarithmic curve table

	This is used to look up perceived height of an object at
	"distance" (offset/2).

	ffff 8000 5555 4000 3333 2aaa  etc.
	(tapering to value of 4 towards the end)

	The sprite routine shifts this one bit right and subtracts one
	before poking into spriteram. Hence 4 => 1

$e7fff-$e83fe is an unknown word table
$e83ff-$effff empty (0xff fill)
$f0000-$f03ff unknown lookup table
$f0400-$fcbff series of unknown lookup tables

	Seems to be a series of (zoom/rotate ??) lookup
	tables all $400 words long. (Total no. of tables = 25?)

	Each table starts at zero and tends upwards: the first reaches 0xfe7.
	The final one reaches 0x3ff (i.e. each successive word is 1 higher
	than the last). The values in the tables tend to go up smoothly but
	with discontinuities at regular intervals. The intervals change
	between tables.

$fcc00-$fffff empty (0xff fill)

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

int wgp_vh_start (void);
int wgp2_vh_start (void);
void wgp_vh_stop (void);
void wgp_vh_screenrefresh (struct mame_bitmap *bitmap,int full_refresh);

//static data16_t *wgp_ram;
extern data16_t *wgp_spritemap;
extern size_t    wgp_spritemap_size;

extern data16_t *wgp_pivram;
READ16_HANDLER ( wgp_pivram_word_r );
WRITE16_HANDLER( wgp_pivram_word_w );

extern data16_t *wgp_piv_ctrlram;
READ16_HANDLER ( wgp_piv_ctrl_word_r );
WRITE16_HANDLER( wgp_piv_ctrl_word_w );

static UINT16 cpua_ctrl = 0xff;
static UINT16 port_sel=0;
extern UINT16 wgp_rotate_ctrl[8];

static data16_t *sharedram;
static size_t sharedram_size;

static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static void parse_control(void)
{
	/* bit 0 enables cpu B */
	/* however this fails when recovering from a save state
	   if cpu B is disabled !! */
	cpu_set_reset_line(2,(cpua_ctrl &0x1) ? CLEAR_LINE : ASSERT_LINE);

	/* bit 1 is "vibration" acc. to test mode */
}

static WRITE16_HANDLER( cpua_ctrl_w )	/* assumes Z80 sandwiched between 68Ks */
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;	/* for Wgp */
	cpua_ctrl = data;

	parse_control();

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",cpu_get_pc(),data);
}


/***********************************************************
				INTERRUPTS
***********************************************************/

/* 68000 A */

/*
void wgp_interrupt4(int x)
{
	cpu_cause_interrupt(0,4);
}
*/

void wgp_interrupt6(int x)
{
	cpu_cause_interrupt(0,6);
}

/* 68000 B */

void wgp_cpub_interrupt6(int x)
{
	cpu_cause_interrupt(2,6);	/* assumes Z80 sandwiched between the 68Ks */
}



/***** Routines for particular games *****/

static int wgp_interrupt(void)
{
	return 4;
}

/* FWIW offset of 10000,10500 on ints can get CPUB obeying the
   first CPUA command the same frame; probably not necessary */

static int wgp_cpub_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(200000-500,0),0, wgp_cpub_interrupt6);
	return 4;
}


/**********************************************************
				GAME INPUTS
**********************************************************/

static READ16_HANDLER( lan_status_r )
{
	logerror("CPU #2 PC %06x: warning - read lan status\n",cpu_get_pc());

	return  (0x4 << 8);	/* CPUB expects this in code at $104d0 (Wgp) */
}

static WRITE16_HANDLER( rotate_port_w )
{
	/* This port may be for piv/sprite layer rotation.

	Wgp2 pokes a single set of values (see 2 routines from
	$4e4a), so if this is rotation then Wgp2 *doesn't* use
	it.

	Wgp pokes a wide variety of values here, which appear
	to move up and down as rotation control words might.
	See $ae06-d8 which pokes piv ctrl words, then pokes
	values to this port.

	There is a lookup area in the data rom from $d0000-$da400
	which contains sets of 4 words (used for ports 0-3).
	NB: port 6 is not written.
	*/

	switch (offset)
	{
		case 0x00:
		{
//logerror("CPU #0 PC %06x: warning - port %04x write %04x\n",cpu_get_pc(),port_sel,data);

			wgp_rotate_ctrl[port_sel] = data;
			return;
		}

		case 0x01:
		{
			port_sel = data &0x7;
		}
	}
}


static READ16_HANDLER( wgp_adinput_r )
{
	UINT16 steer = 0x40;

	if (input_port_5_word_r(0,0) & 0x8)	/* pressing down */
		steer = 0x0d;

	if (input_port_5_word_r(0,0) & 0x4)	/* pressing up */
		steer = 0x73;

	if (input_port_5_word_r(0,0) & 0x2)	/* pressing right */
		steer = 0x00;

	if (input_port_5_word_r(0,0) & 0x1)	/* pressing left */
		steer = 0x80;

	switch (offset)
	{
		case 0x00:
		{
			if (input_port_5_word_r(0,0) &0x40)	/* pressing accel */
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
			if (input_port_5_word_r(0,0) &0x80)	/* pressing brake */
				return 0xcf;
			else
				return 0xff;
		}

		case 0x05:
			return input_port_6_word_r(0,0);	/* unknown */
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

static int banknum = -1;

static void reset_sound_region(void)	/* assumes Z80 sandwiched between the 68Ks */
{
	cpu_setbank( 10, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000 );
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 7;
	reset_sound_region();
}

WRITE16_HANDLER( wgp_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);
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
	{ 0x140000, 0x143fff, MRA16_RAM },
	{ 0x180000, 0x18000f, TC0220IOC_halfword_byteswap_r },
	{ 0x200000, 0x20000f, wgp_adinput_r },
	{ 0x300000, 0x30ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_r },
	{ 0x400000, 0x40bfff, MRA16_RAM },	/* sprite tilemaps */
	{ 0x40c000, 0x40dfff, MRA16_RAM },	/* sprite ram */
	{ 0x500000, 0x501fff, MRA16_RAM },	/* unknown/unused */
	{ 0x502000, 0x517fff, wgp_pivram_word_r },	/* piv tilemaps */
	{ 0x520000, 0x52001f, wgp_piv_ctrl_word_r },
	{ 0x700000, 0x701fff, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( wgp_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x140000, 0x143fff, MWA16_RAM, &sharedram, &sharedram_size },
	{ 0x180000, 0x18000f, TC0220IOC_halfword_byteswap_w },
	{ 0x1c0000, 0x1c0001, cpua_ctrl_w },
	{ 0x200000, 0x20000f, wgp_adinput_w },
	{ 0x300000, 0x30ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x320000, 0x32000f, TC0100SCN_ctrl_word_0_w },
	{ 0x400000, 0x40bfff, MWA16_RAM, &wgp_spritemap, &wgp_spritemap_size },
	{ 0x40c000, 0x40dfff, MWA16_RAM, &spriteram16, &spriteram_size  },	/* sprite ram */
	{ 0x40fff0, 0x40fff1, MWA16_NOP },	// ?? (writes 0x8000 and 0 alternately - Wgp2 just 0)
	{ 0x500000, 0x501fff, MWA16_RAM },	/* unknown/unused */
	{ 0x502000, 0x517fff, wgp_pivram_word_w, &wgp_pivram },	/* piv tilemaps */
	{ 0x520000, 0x52001f, wgp_piv_ctrl_word_w, &wgp_piv_ctrlram },
	{ 0x600000, 0x600003, rotate_port_w },	/* rotation control ? */
	{ 0x700000, 0x701fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( wgp_cpub_readmem )	/* LAN areas not mapped... */
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
	{ 0x140000, 0x143fff, sharedram_w },
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
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

INPUT_PORTS_START( wgp )	/* Wgp2 has no "lumps" ? */
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

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Shift Pattern Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON7 | IPF_PLAYER1 )	/* freeze */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON4 | IPF_PLAYER1 )	/* shift up */
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* shift down */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )	/* "start lump" (lamp?) */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* "brake lump" (lamp?) */
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

	PORT_START      /* fake inputs, for steering etc. */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* accel */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* brake */

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

INPUT_PORTS_START( wgpjoy )
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

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Shift Pattern Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )
//	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	// freeze

	PORT_START      /* IN1, is it read? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* doesn't exist */

	PORT_START	/* doesn't exist */
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
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};


/***********************************************************
			     MACHINE DRIVERS

Wgp has high interleaving to prevent "common ram error".
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
	250,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	wgp_gfxdecodeinfo,
	4096, 0,
	0,

	VIDEO_TYPE_RASTER,
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

static struct MachineDriver machine_driver_wgp2 =
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
	4096, 0,
	0,

	VIDEO_TYPE_RASTER,
	0,
	wgp2_vh_start,
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
	ROM_LOAD16_BYTE( "c32-25.12",      0x00000, 0x20000, 0x0cc81e77 )
	ROM_LOAD16_BYTE( "c32-29.13",      0x00001, 0x20000, 0xfab47cf0 )
	ROM_LOAD16_WORD_SWAP( "c32-10.9",  0x80000, 0x80000, 0xa44c66e9 )	/* data rom */

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c32-28.64", 0x00000, 0x20000, 0x38f3c7bf )
	ROM_LOAD16_BYTE( "c32-27.63", 0x00001, 0x20000, 0xbe2397fb )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c32-24.34",   0x00000, 0x04000, 0xe9adb447 )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c32-09.16", 0x00000, 0x80000, 0x96495f35 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c32-04.9",  0x000000, 0x80000, 0x473a19c9 )	/* PIV */
	ROM_LOAD32_BYTE( "c32-03.10", 0x000001, 0x80000, 0x9ec3e134 )
	ROM_LOAD32_BYTE( "c32-02.11", 0x000002, 0x80000, 0xc5721f3a )
	ROM_LOAD32_BYTE( "c32-01.12", 0x000003, 0x80000, 0xd27d7d93 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c32-05.71", 0x000000, 0x80000, 0x3698d47a )	/* OBJ */
	ROM_LOAD16_BYTE( "c32-06.70", 0x000001, 0x80000, 0xf0267203 )
	ROM_LOAD16_BYTE( "c32-07.69", 0x100000, 0x80000, 0x743d46bd )
	ROM_LOAD16_BYTE( "c32-08.68", 0x100001, 0x80000, 0xfaab63b0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c32-11.8",  0x00000, 0x80000, 0x2b326ff0 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c32-12.7",  0x00000, 0x80000, 0xdf48a37b )

//	Pals (not dumped)
//	ROM_LOAD( "c32-18.64", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c32-19.27", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c32-20.67", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c32-21.85", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c32-22.24", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c32-23.13", 0x00000, 0x00???, 0x00000000 )

//	Pals on lan interface board
//	ROM_LOAD( "c32-34", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c32-35", 0x00000, 0x00???, 0x00000000 )
ROM_END

ROM_START( wgpj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c32-48.12",      0x00000, 0x20000, 0x819cc134 )
	ROM_LOAD16_BYTE( "c32-49.13",      0x00001, 0x20000, 0x4a515f02 )
	ROM_LOAD16_WORD_SWAP( "c32-10.9",  0x80000, 0x80000, 0xa44c66e9 )	/* data rom */

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c32-28.64", 0x00000, 0x20000, 0x38f3c7bf )
	ROM_LOAD16_BYTE( "c32-27.63", 0x00001, 0x20000, 0xbe2397fb )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c32-24.34",   0x00000, 0x04000, 0xe9adb447 )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c32-09.16", 0x00000, 0x80000, 0x96495f35 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c32-04.9",  0x000000, 0x80000, 0x473a19c9 )	/* PIV */
	ROM_LOAD32_BYTE( "c32-03.10", 0x000001, 0x80000, 0x9ec3e134 )
	ROM_LOAD32_BYTE( "c32-02.11", 0x000002, 0x80000, 0xc5721f3a )
	ROM_LOAD32_BYTE( "c32-01.12", 0x000003, 0x80000, 0xd27d7d93 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c32-05.71", 0x000000, 0x80000, 0x3698d47a )	/* OBJ */
	ROM_LOAD16_BYTE( "c32-06.70", 0x000001, 0x80000, 0xf0267203 )
	ROM_LOAD16_BYTE( "c32-07.69", 0x100000, 0x80000, 0x743d46bd )
	ROM_LOAD16_BYTE( "c32-08.68", 0x100001, 0x80000, 0xfaab63b0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c32-11.8", 0x00000, 0x80000, 0x2b326ff0 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c32-12.7", 0x00000, 0x80000, 0xdf48a37b )
ROM_END

ROM_START( wgpjoy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c32-57.12",      0x00000, 0x20000, 0x13a78911 )
	ROM_LOAD16_BYTE( "c32-58.13",      0x00001, 0x20000, 0x326d367b )
	ROM_LOAD16_WORD_SWAP( "c32-10.9",  0x80000, 0x80000, 0xa44c66e9 )	/* data rom */

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c32-60.64", 0x00000, 0x20000, 0x7a980312 )
	ROM_LOAD16_BYTE( "c32-59.63", 0x00001, 0x20000, 0xed75b333 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c32-61.34",   0x00000, 0x04000, 0x2fcad5a3 )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c32-09.16", 0x00000, 0x80000, 0x96495f35 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c32-04.9",  0x000000, 0x80000, 0x473a19c9 )	/* PIV */
	ROM_LOAD32_BYTE( "c32-03.10", 0x000001, 0x80000, 0x9ec3e134 )
	ROM_LOAD32_BYTE( "c32-02.11", 0x000002, 0x80000, 0xc5721f3a )
	ROM_LOAD32_BYTE( "c32-01.12", 0x000003, 0x80000, 0xd27d7d93 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c32-05.71", 0x000000, 0x80000, 0x3698d47a )	/* OBJ */
	ROM_LOAD16_BYTE( "c32-06.70", 0x000001, 0x80000, 0xf0267203 )
	ROM_LOAD16_BYTE( "c32-07.69", 0x100000, 0x80000, 0x743d46bd )
	ROM_LOAD16_BYTE( "c32-08.68", 0x100001, 0x80000, 0xfaab63b0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c32-11.8", 0x00000, 0x80000, 0x2b326ff0 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "c32-12.7", 0x00000, 0x80000, 0xdf48a37b )
ROM_END

ROM_START( wgp2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c73-01.12",      0x00000, 0x20000, 0Xc6434834 )
	ROM_LOAD16_BYTE( "c73-02.13",      0x00001, 0x20000, 0xc67f1ed1 )
	ROM_LOAD16_WORD_SWAP( "c32-10.9",  0x80000, 0x80000, 0xa44c66e9 )	/* data rom */

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c73-04.64", 0x00000, 0x20000, 0x383aa776 )
	ROM_LOAD16_BYTE( "c73-03.63", 0x00001, 0x20000, 0xeb5067ef )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c73-05.34",   0x00000, 0x04000, 0x7e00a299 )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c32-09.16", 0x00000, 0x80000, 0x96495f35 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c32-04.9",  0x000000, 0x80000, 0x473a19c9 )	/* PIV */
	ROM_LOAD32_BYTE( "c32-03.10", 0x000001, 0x80000, 0x9ec3e134 )
	ROM_LOAD32_BYTE( "c32-02.11", 0x000002, 0x80000, 0xc5721f3a )
	ROM_LOAD32_BYTE( "c32-01.12", 0x000003, 0x80000, 0xd27d7d93 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c32-05.71", 0x000000, 0x80000, 0x3698d47a )	/* OBJ */
	ROM_LOAD16_BYTE( "c32-06.70", 0x000001, 0x80000, 0xf0267203 )
	ROM_LOAD16_BYTE( "c32-07.69", 0x100000, 0x80000, 0x743d46bd )
	ROM_LOAD16_BYTE( "c32-08.68", 0x100001, 0x80000, 0xfaab63b0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c32-11.8", 0x00000, 0x80000, 0x2b326ff0 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* delta-t samples */
	ROM_LOAD( "c32-12.7", 0x00000, 0x80000, 0xdf48a37b )

//	WGP2 security board (has TC0190FMC)
//	ROM_LOAD( "c73-06", 0x00000, 0x00???, 0x00000000 )
ROM_END


void init_wgp(void)
{
#if 0
	/* Patch for coding error that causes corrupt data in
	   sprite tilemapping area from $4083c0-847f */
	data16_t *ROM = (data16_t *)memory_region(REGION_CPU1);
	ROM[0x25dc / 2] = 0x0602;	// faulty value is 0x0206
#endif

//	taitosnd_setz80_soundcpu( 2 );
	cpua_ctrl = 0xff;

	state_save_register_UINT16("main1", 0, "control", &cpua_ctrl, 1);
	state_save_register_func_postload(parse_control);

	state_save_register_int("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}

void init_wgp2(void)
{
	/* Code patches to prevent failure in memory checks */
	data16_t *ROM = (data16_t *)memory_region(REGION_CPU3);
	ROM[0x8008 / 2] = 0x0;
	ROM[0x8010 / 2] = 0x0;

	init_wgp();
}

/* Working Games with a few graphics problems - missing rotation */

GAMEX( 1989, wgp,    0,      wgp,    wgp,    wgp,    ROT0, "Taito America Corporation", "World Grand Prix (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, wgpj,   wgp,    wgp,    wgp,    wgp,    ROT0, "Taito Corporation", "World Grand Prix (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, wgpjoy, wgp,    wgp,    wgpjoy, wgp,    ROT0, "Taito Corporation", "World Grand Prix (joystick version) (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1990, wgp2,   wgp,    wgp2,   wgp,    wgp2,   ROT0, "Taito Corporation", "World Grand Prix 2 (Japan)", GAME_IMPERFECT_GRAPHICS )

