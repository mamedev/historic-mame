/*************************************************************************

	Sega Z80-3D system

	driver by Alex Pasadyn, Howie Cohen, Frank Palazzolo, Ernesto Corvi,
	and Aaron Giles

	Games supported:
		* Turbo
		* Subroc 3D
		* Buck Rogers: Planet of Zoom

	Known bugs:
		* no sound support in Subroc

**************************************************************************
	TURBO
**************************************************************************

	Memory Map:  ( * not complete * )

	Address Range:	R/W:	 Function:
	--------------------------------------------------------------------------
	0000 - 5fff		R		 Program ROM
	a000 - a0ff		W		 Sprite RAM
	a800 - a803		W		 Lamps / Coin Meters
	b000 - b1ff		R/W		 Collision RAM
	e000 - e7ff		R/W		 character RAM
	f000 - f7ff		R/W		 RAM
	f202					 coinage 2
	f205					 coinage 1
	f800 - f803		R/W		 road drawing
	f900 - f903		R/W		 road drawing
	fa00 - fa03		R/W		 sound
	fb00 - fb03		R/W		 x,DS2,x,x
	fc00 - fc01		R		 DS1,x
	fc00 - fc01		W		 score
	fd00			R		 Coin Inputs, etc.
	fe00			R		 DS3,x

	Switch settings:
	Notes:
		1) Facing the CPU board, with the two large IDC connectors at
		   the top of the board, and the large and small IDC
		   connectors at the bottom, DIP switch #1 is upper right DIP
		   switch, DIP switch #2 is the DIP switch to the right of it.

		2) Facing the Sound board, with the IDC connector at the
		   bottom of the board, DIP switch #3 (4 bank) can be seen.
	----------------------------------------------------------------------------

	Option	   (DIP Swtich #1) | SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	1 Car On Extended Play	   | ON	 | ON  |	 |	   |	 |	   |	 |	   |
	2 Car On Extended Play	   | OFF | ON  |	 |	   |	 |	   |	 |	   |
	3 Car On Extended Play	   | ON	 | OFF |	 |	   |	 |	   |	 |	   |
	4 Car On Extended Play	   | OFF | OFF |	 |	   |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Game Time Adjustable	   |	 |	   | ON	 |	   |	 |	   |	 |	   |
	Game Time Fixed (55 Sec.)  |	 |	   | OFF |	   |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Hard Game Difficulty	   |	 |	   |	 | ON  |	 |	   |	 |	   |
	Easy Game Difficulty	   |	 |	   |	 | OFF |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Normal Game Mode		   |	 |	   |	 |	   | ON	 |	   |	 |	   |
	No Collisions (cheat)	   |	 |	   |	 |	   | OFF |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Initial Entry Off (?)	   |	 |	   |	 |	   |	 | ON  |	 |	   |
	Initial Entry On  (?)	   |	 |	   |	 |	   |	 | OFF |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Not Used				   |	 |	   |	 |	   |	 |	   |  X	 |	X  |
	---------------------------------------------------------------------------

	Option	   (DIP Swtich #2) | SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 |
	--------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	60 Seconds Game Time	   | ON	 | ON  |	 |	   |	 |	   |	 |	   |
	70 Seconds Game Time	   | OFF | ON  |	 |	   |	 |	   |	 |	   |
	80 Seconds Game Time	   | ON	 | OFF |	 |	   |	 |	   |	 |	   |
	90 Seconds Game Time	   | OFF | OFF |	 |	   |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Slot 1	 1 Coin	 1 Credit  |	 |	   | ON	 | ON  | ON	 |	   |	 |	   |
	Slot 1	 1 Coin	 2 Credits |	 |	   | OFF | ON  | ON	 |	   |	 |	   |
	Slot 1	 1 Coin	 3 Credits |	 |	   | ON	 | OFF | ON	 |	   |	 |	   |
	Slot 1	 1 Coin	 6 Credits |	 |	   | OFF | OFF | ON	 |	   |	 |	   |
	Slot 1	 2 Coins 1 Credit  |	 |	   | ON	 | ON  | OFF |	   |	 |	   |
	Slot 1	 3 Coins 1 Credit  |	 |	   | OFF | ON  | OFF |	   |	 |	   |
	Slot 1	 4 Coins 1 Credit  |	 |	   | ON	 | OFF | OFF |	   |	 |	   |
	Slot 1	 1 Coin	 1 Credit  |	 |	   | OFF | OFF | OFF |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Slot 2	 1 Coin	 1 Credit  |	 |	   |	 |	   |	 | ON  | ON	 | ON  |
	Slot 2	 1 Coin	 2 Credits |	 |	   |	 |	   |	 | OFF | ON	 | ON  |
	Slot 2	 1 Coin	 3 Credits |	 |	   |	 |	   |	 | ON  | OFF | ON  |
	Slot 2	 1 Coin	 6 Credits |	 |	   |	 |	   |	 | OFF | OFF | ON  |
	Slot 2	 2 Coins 1 Credit  |	 |	   |	 |	   |	 | ON  | ON	 | OFF |
	Slot 2	 3 Coins 1 Credit  |	 |	   |	 |	   |	 | OFF | ON	 | OFF |
	Slot 2	 4 Coins 1 Credit  |	 |	   |	 |	   |	 | ON  | OFF | OFF |
	Slot 2	 1 Coins 1 Credit  |	 |	   |	 |	   |	 | OFF | OFF | OFF |
	---------------------------------------------------------------------------

	Option	   (DIP Swtich #3) | SW1 | SW2 | SW3 | SW4 |
	 --------------------------|-----|-----|-----|-----|
	Not Used				   |  X	 |	X  |	 |	   |
	 --------------------------|-----|-----|-----|-----|
	Digital (LED) Tachometer   |	 |	   | ON	 |	   |
	Analog (Meter) Tachometer  |	 |	   | OFF |	   |
	 --------------------------|-----|-----|-----|-----|
	Cockpit Sound System	   |	 |	   |	 | ON  |
	Upright Sound System	   |	 |	   |	 | OFF |
	---------------------------------------------------

	Here is a complete list of the ROMs:

	Turbo ROMLIST - Frank Palazzolo
	Name	 Loc			 Function
	-----------------------------------------------------------------------------
	Images Acquired:
	EPR1262,3,4	 IC76, IC89, IC103
	EPR1363,4,5
	EPR15xx				Program ROMS
	EPR1244				Character Data 1
	EPR1245				Character Data 2
	EPR-1125			Road ROMS
	EPR-1126
	EPR-1127
	EPR-1238
	EPR-1239
	EPR-1240
	EPR-1241
	EPR-1242
	EPR-1243
	EPR1246-1258		Sprite ROMS
	EPR1288-1300

	PR-1114		IC13	Color 1 (road, etc.)
	PR-1115		IC18	Road gfx
	PR-1116		IC20	Crash (collision detection?)
	PR-1117		IC21	Color 2 (road, etc.)
	PR-1118		IC99	256x4 Character Color PROM
	PR-1119		IC50	512x8 Vertical Timing PROM
	PR-1120		IC62	Horizontal Timing PROM
	PR-1121		IC29	Color PROM
	PR-1122		IC11	Pattern 1
	PR-1123		IC21	Pattern 2

	PA-06R		IC22	Mathbox Timing PAL
	PA-06L		IC90	Address Decode PAL

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "turbo.h"
#include "machine/segacrpt.h"


/*************************************
 *
 *	Turbo CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( turbo_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xb000, 0xb1ff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf803, ppi8255_0_r },
	{ 0xf900, 0xf903, ppi8255_1_r },
	{ 0xfa00, 0xfa03, ppi8255_2_r },
	{ 0xfb00, 0xfb03, ppi8255_3_r },
	{ 0xfc00, 0xfcff, turbo_8279_r },
	{ 0xfd00, 0xfdff, input_port_0_r },
	{ 0xfe00, 0xfeff, turbo_collision_r },
MEMORY_END


static MEMORY_WRITE_START( turbo_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xa800, 0xa807, turbo_coin_and_lamp_w },
	{ 0xb000, 0xb1ff, MWA_RAM, &sega_sprite_position },
	{ 0xb800, 0xb800, MWA_NOP },	/* resets the analog wheel value */
	{ 0xe000, 0xe7ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xe800, 0xe800, turbo_collision_clear_w },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf803, ppi8255_0_w },
	{ 0xf900, 0xf903, ppi8255_1_w },
	{ 0xfa00, 0xfa03, ppi8255_2_w },
	{ 0xfb00, 0xfb03, ppi8255_3_w },
	{ 0xfc00, 0xfcff, turbo_8279_w },
MEMORY_END



/*************************************
 *
 *	Subroc3D CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( subroc3d_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xa800, 0xa800, input_port_0_r },
	{ 0xa801, 0xa801, input_port_1_r },
	{ 0xa802, 0xa802, input_port_2_r },
	{ 0xa803, 0xa803, input_port_3_r },
	{ 0xb000, 0xb7ff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xe803, ppi8255_0_r },
	{ 0xf000, 0xf003, ppi8255_1_r },
MEMORY_END


static MEMORY_WRITE_START( subroc3d_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xa000, 0xa3ff, MWA_RAM, &sega_sprite_position },
	{ 0xa400, 0xa7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xb000, 0xb7ff, MWA_RAM },
	{ 0xe000, 0xe7ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xe800, 0xe803, ppi8255_0_w },
	{ 0xf000, 0xf003, ppi8255_1_w },
	{ 0xf800, 0xf801, turbo_8279_w },
MEMORY_END



/*************************************
 *
 *	Buck Rogers CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( buckrog_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc803, ppi8255_0_r },
	{ 0xd000, 0xd003, ppi8255_1_r },
	{ 0xe000, 0xe1ff, MRA_RAM },
	{ 0xe800, 0xe800, input_port_0_r },
	{ 0xe801, 0xe801, input_port_1_r },
	{ 0xe802, 0xe802, buckrog_port_2_r },
	{ 0xe803, 0xe803, buckrog_port_3_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( buckrog_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xc800, 0xc803, ppi8255_0_w },
	{ 0xd000, 0xd003, ppi8255_1_w },
	{ 0xd800, 0xd801, turbo_8279_w },
	{ 0xe000, 0xe1ff, MWA_RAM, &sega_sprite_position },
	{ 0xe400, 0xe4ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END


static MEMORY_READ_START( buckrog_readmem2 )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
MEMORY_END


static PORT_READ_START( buckrog_readport2 )
	{ 0x00, 0x00, buckrog_cpu2_command_r },
PORT_END


static MEMORY_WRITE_START( buckrog_writemem2 )
	{ 0x0000, 0xdfff, buckrog_bitmap_w },
	{ 0xf000, 0xf7ff, MWA_RAM },
MEMORY_END




/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( turbo )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )				/* ACCEL B */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* ACCEL A */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE )	/* SHIFT */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_SERVICE_NO_TOGGLE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x03, 0x03, "Car On Extended Play" )
	PORT_DIPSETTING( 0x03, "1" )
	PORT_DIPSETTING( 0x02, "2" )
	PORT_DIPSETTING( 0x01, "3" )
	PORT_DIPSETTING( 0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, "Game Time" )
	PORT_DIPSETTING( 0x00, "Fixed (55 sec)" )
	PORT_DIPSETTING( 0x04, "Adjustable" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING( 0x00, "Easy")
	PORT_DIPSETTING( 0x08, "Hard")
	PORT_DIPNAME( 0x10, 0x00, "Game Mode" )
	PORT_DIPSETTING( 0x10, "No Collisions (cheat)" )
	PORT_DIPSETTING( 0x00, "Normal" )
	PORT_DIPNAME( 0x20, 0x00, "Initial Entry" )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ))
	PORT_DIPSETTING( 0x00, DEF_STR( On ))
	PORT_BIT( 0xc0, 0xc0, IPT_UNUSED )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, "Game Time" )
	PORT_DIPSETTING( 0x00, "60 seconds" )
	PORT_DIPSETTING( 0x01, "70 seconds" )
	PORT_DIPSETTING( 0x02, "80 seconds" )
	PORT_DIPSETTING( 0x03, "90 seconds" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ))
	PORT_DIPSETTING(	0x18, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(	0x14, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ))
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ))*/
	PORT_DIPSETTING(	0x1c, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ))
	PORT_DIPSETTING(	0xc0, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(	0xa0, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ))
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ))*/
	PORT_DIPSETTING(	0xe0, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(	0x60, DEF_STR( 1C_6C ))

	PORT_START	/* DSW 3 */
	PORT_BIT( 0x0f, 0x00, IPT_UNUSED )		/* Merged with collision bits */
	PORT_BIT( 0x30, 0x00, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x40, "Tachometer" )
	PORT_DIPSETTING(	0x40, "Analog (Meter)")
	PORT_DIPSETTING(	0x00, "Digital (led)")
	PORT_DIPNAME( 0x80, 0x80, "Sound System" )
	PORT_DIPSETTING(	0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, "Cockpit")

	PORT_START		/* IN0 */
	PORT_ANALOG( 0xff, 0, IPT_DIAL | IPF_CENTER, 10, 30, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( subroc3d )
	PORT_START
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_SERVICE_NO_TOGGLE( 0x10, 0x10 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x07, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xc0, 0x40, "Ships" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START  /* DSW 3 */
	PORT_DIPNAME( 0x03, 0x03, "Extra Ship" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x01, "40000" )
	PORT_DIPSETTING(    0x02, "60000" )
	PORT_DIPSETTING(    0x03, "80000" )
	PORT_DIPNAME( 0x04, 0x04, "Initial Input" )
	PORT_DIPSETTING(    0x00, "Disable" )
	PORT_DIPSETTING(    0x04, "Enable" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPNAME( 0x10, 0x10, "Play" )
	PORT_DIPSETTING(    0x00, "Free" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPNAME( 0x20, 0x20, "Motion" )
	PORT_DIPSETTING(    0x00, "Stop" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPNAME( 0x40, 0x00, "Screen" )
	PORT_DIPSETTING(    0x00, "Mono" )
	PORT_DIPSETTING(    0x40, "Stereo" )
	PORT_DIPNAME( 0x80, 0x80, "Game" )
	PORT_DIPSETTING(    0x00, "Endless" )
	PORT_DIPSETTING(    0x80, "Normal" )

	PORT_START  /* DSW 1 */					/* Unused */
INPUT_PORTS_END


INPUT_PORTS_START( buckrog )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) // Accel Hi
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) // Accel Lo
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )

	PORT_START /* Inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_SERVICE_NO_TOGGLE( 0x10, 0x10 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START  /* DSW 1 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x07, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x01, 0x00, "Collisions" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Accel by" )
	PORT_DIPSETTING(    0x00, "Pedal" )
	PORT_DIPSETTING(    0x02, "Button" )
	PORT_DIPNAME( 0x04, 0x00, "Best 5 Scores" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Score Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPNAME( 0x60, 0x00, "Extra Ships" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x60, "6" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, "Cockpit" )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout numlayout =
{
	8,10,
	16,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8
};


static struct GfxLayout rotated_numlayout =
{
	10,8,
	16,
	1,
	{ 0 },
	{ 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	10*8
};


static struct GfxLayout rotated_tachlayout =
{
	16,1,
	2,
	1,
	{ 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0 },
	1*8
};


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo turbo_gfxdecode[] =
{
	{ REGION_GFX4, 0x0000, &rotated_numlayout,	512,   1 },
	{ REGION_GFX4, 0x0100, &rotated_tachlayout,	512,   3 },
	{ REGION_GFX2, 0x0000, &charlayout,	512,   3 },
	{ -1 }
};


static struct GfxDecodeInfo subroc3d_gfxdecode[] =
{
	{ REGION_GFX4, 0x0000, &numlayout,	512,   1 },
	{ -1 }
};


static struct GfxDecodeInfo buckrog_gfxdecode[] =
{
	{ REGION_GFX4, 0x0000, &numlayout,	1024+512+256, 1 },
	{ -1 }
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static const char *turbo_sample_names[]=
{
	"*turbo",
	"01.wav",		/* Trig1 */
	"02.wav",		/* Trig2 */
	"03.wav",		/* Trig3 */
	"04.wav",		/* Trig4 */
	"05.wav",		/* Screech */
	"06.wav",		/* Crash */
	"skidding.wav",	/* Spin */
	"idle.wav",		/* Idle */
	"ambulanc.wav",	/* Ambulance */
	0
};

static struct Samplesinterface turbo_samples_interface =
{
	8,			/* eight channels */
	25,			/* volume */
	turbo_sample_names
};


static const char *buckrog_sample_names[]=
{
	"*buckrog",
	"alarm0.wav",
	"alarm1.wav",
	"alarm2.wav",
	"alarm3.wav",
	"exp.wav",
	"fire.wav",
	"rebound.wav",
	"hit.wav",
	"shipsnd1.wav",
	"shipsnd2.wav",
	"shipsnd3.wav",
	0
};

static struct Samplesinterface buckrog_samples_interface =
{
	6,                      /* 6 channels */
	25,			/* volume */
	buckrog_sample_names
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( turbo )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(turbo_readmem,turbo_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(turbo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_ASPECT_RATIO(104,105)
	MDRV_SCREEN_SIZE(32*8, 35*8)
	MDRV_VISIBLE_AREA(1*8, 32*8-1, 0*8, 35*8-1)
	MDRV_GFXDECODE(turbo_gfxdecode)
	MDRV_PALETTE_LENGTH(512+6)

	MDRV_PALETTE_INIT(turbo)
	MDRV_VIDEO_START(turbo)
	MDRV_VIDEO_EOF(turbo)
	MDRV_VIDEO_UPDATE(turbo)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("samples", SAMPLES, turbo_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( subroc3d )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(subroc3d_readmem,subroc3d_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(subroc3d)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_ASPECT_RATIO(56,45)
	MDRV_SCREEN_SIZE(32*8, 34*8)
	MDRV_VISIBLE_AREA(0*8, 30*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(subroc3d_gfxdecode)
	MDRV_PALETTE_LENGTH(512+2)

	MDRV_PALETTE_INIT(subroc3d)
	MDRV_VIDEO_START(subroc3d)
	MDRV_VIDEO_UPDATE(subroc3d)

	/* sound hardware */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( buckrog )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(buckrog_readmem,buckrog_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(buckrog_readmem2,buckrog_writemem2)
	MDRV_CPU_PORTS(buckrog_readport2,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)
	MDRV_MACHINE_INIT(buckrog)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_ASPECT_RATIO(13,8)
	MDRV_SCREEN_SIZE(39*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 39*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(buckrog_gfxdecode)
	MDRV_PALETTE_LENGTH(1024+512+256+2)

	MDRV_PALETTE_INIT(buckrog)
	MDRV_VIDEO_START(buckrog)
	MDRV_VIDEO_UPDATE(buckrog)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("samples", SAMPLES, buckrog_samples_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( turbo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1513.bin",  0x0000, 0x2000, 0x0326adfc )
	ROM_LOAD( "epr1514.bin",  0x2000, 0x2000, 0x25af63b0 )
	ROM_LOAD( "epr1515.bin",  0x4000, 0x2000, 0x059c1c36 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* sprite data */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, 0x555bfe9a )	/* level 0 */
	ROM_RELOAD(				 0x02000, 0x2000 )
	ROM_LOAD( "epr1247.rom", 0x04000, 0x2000, 0xc8c5e4d5 )	/* level 1 */
	ROM_RELOAD(				 0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, 0x82fe5b94 )	/* level 2 */
	ROM_RELOAD(				 0x0a000, 0x2000 )
	ROM_LOAD( "epr1249.rom", 0x0c000, 0x2000, 0xe258e009 )	/* level 3 */
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, 0x292573de )	/* level 4 */
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, 0x92783626 )	/* level 5 */
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, 0x485dcef9 )	/* level 6 */
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, 0x4ca984ce )	/* level 7 */
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, 0xaee6e05e )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, 0x17f67424 )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, 0x2ba0b46b )

	ROM_REGION( 0x4800, REGION_GFX3, 0 )	/* road data */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, 0x65b5d44b )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, 0x685ace1b )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, 0x9233c9ca )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, 0xd94fd83f )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, 0x4c41124f )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, 0x371d6282 )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, 0x1109358a )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, 0x04866769 )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, 0x29854c48 )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1121.bin",	 0x0000, 0x0200, 0x7692f497 )	/* palette */
	ROM_LOAD( "pr1122.bin",	 0x0200, 0x0400, 0x1a86ce70 )	/* sprite priorities */
	ROM_LOAD( "pr1123.bin",	 0x0600, 0x0400, 0x02d2cb52 )	/* sprite/road/background priorities */
	ROM_LOAD( "pr-1118.bin", 0x0a00, 0x0100, 0x07324cfd )	/* background color table */
	ROM_LOAD( "pr1114.bin",	 0x0b00, 0x0020, 0x78aded46 )	/* road red/green color table */
	ROM_LOAD( "pr1117.bin",	 0x0b20, 0x0020, 0xf06d9907 )	/* road green/blue color table */
	ROM_LOAD( "pr1115.bin",	 0x0b40, 0x0020, 0x5394092c )	/* road collision/enable */
	ROM_LOAD( "pr1116.bin",	 0x0b60, 0x0020, 0x3956767d )	/* collision detection */
	ROM_LOAD( "sndprom.bin", 0x0b80, 0x0020, 0xb369a6ae )
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, 0x628d3f1d )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, 0x591b6a68 )	/* timing - not used */
ROM_END


ROM_START( turboa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1262.rom",  0x0000, 0x2000, 0x1951b83a )
	ROM_LOAD( "epr1263.rom",  0x2000, 0x2000, 0x45e01608 )
	ROM_LOAD( "epr1264.rom",  0x4000, 0x2000, 0x1802f6c7 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* sprite data */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, 0x555bfe9a )	/* level 0 */
	ROM_RELOAD(				 0x02000, 0x2000 )
	ROM_LOAD( "epr1247.rom", 0x04000, 0x2000, 0xc8c5e4d5 )	/* level 1 */
	ROM_RELOAD(				 0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, 0x82fe5b94 )	/* level 2 */
	ROM_RELOAD(				 0x0a000, 0x2000 )
	ROM_LOAD( "epr1249.rom", 0x0c000, 0x2000, 0xe258e009 )	/* level 3 */
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, 0x292573de )	/* level 4 */
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, 0x92783626 )	/* level 5 */
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, 0x485dcef9 )	/* level 6 */
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, 0x4ca984ce )	/* level 7 */
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, 0xaee6e05e )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, 0x17f67424 )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, 0x2ba0b46b )

	ROM_REGION( 0x4800, REGION_GFX3, 0 )	/* road data */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, 0x65b5d44b )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, 0x685ace1b )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, 0x9233c9ca )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, 0xd94fd83f )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, 0x4c41124f )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, 0x371d6282 )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, 0x1109358a )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, 0x04866769 )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, 0x29854c48 )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1121.bin",	 0x0000, 0x0200, 0x7692f497 )	/* palette */
	ROM_LOAD( "pr1122.bin",	 0x0200, 0x0400, 0x1a86ce70 )	/* sprite priorities */
	ROM_LOAD( "pr1123.bin",	 0x0600, 0x0400, 0x02d2cb52 )	/* sprite/road/background priorities */
	ROM_LOAD( "pr-1118.bin", 0x0a00, 0x0100, 0x07324cfd )	/* background color table */
	ROM_LOAD( "pr1114.bin",	 0x0b00, 0x0020, 0x78aded46 )	/* road red/green color table */
	ROM_LOAD( "pr1117.bin",	 0x0b20, 0x0020, 0xf06d9907 )	/* road green/blue color table */
	ROM_LOAD( "pr1115.bin",	 0x0b40, 0x0020, 0x5394092c )	/* road collision/enable */
	ROM_LOAD( "pr1116.bin",	 0x0b60, 0x0020, 0x3956767d )	/* collision detection */
	ROM_LOAD( "sndprom.bin", 0x0b80, 0x0020, 0xb369a6ae )
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, 0x628d3f1d )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, 0x591b6a68 )	/* timing - not used */
ROM_END


ROM_START( turbob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr-1363.cpu",  0x0000, 0x2000, 0x5c110fb6 )
	ROM_LOAD( "epr-1364.cpu",  0x2000, 0x2000, 0x6a341693 )
	ROM_LOAD( "epr-1365.cpu",  0x4000, 0x2000, 0x3b6b0dc8 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* sprite data */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, 0x555bfe9a )	/* level 0 */
	ROM_RELOAD(				 0x02000, 0x2000 )
	ROM_LOAD( "mpr1290.rom", 0x04000, 0x2000, 0x95182020 )	/* is this good? */
	ROM_RELOAD(				 0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, 0x82fe5b94 )	/* level 2 */
	ROM_RELOAD(				 0x0a000, 0x2000 )
	ROM_LOAD( "mpr1291.rom", 0x0c000, 0x2000, 0x0e857f82 )	/* is this good? */
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, 0x292573de )	/* level 4 */
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, 0x92783626 )	/* level 5 */
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, 0x485dcef9 )	/* level 6 */
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, 0x4ca984ce )	/* level 7 */
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, 0xaee6e05e )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, 0x17f67424 )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, 0x2ba0b46b )

	ROM_REGION( 0x4800, REGION_GFX3, 0 )	/* road data */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, 0x65b5d44b )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, 0x685ace1b )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, 0x9233c9ca )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, 0xd94fd83f )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, 0x4c41124f )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, 0x371d6282 )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, 0x1109358a )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, 0x04866769 )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, 0x29854c48 )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1121.bin",	 0x0000, 0x0200, 0x7692f497 )	/* palette */
	ROM_LOAD( "pr1122.bin",	 0x0200, 0x0400, 0x1a86ce70 )	/* sprite priorities */
	ROM_LOAD( "pr1123.bin",	 0x0600, 0x0400, 0x02d2cb52 )	/* sprite/road/background priorities */
	ROM_LOAD( "pr-1118.bin", 0x0a00, 0x0100, 0x07324cfd )	/* background color table */
	ROM_LOAD( "pr1114.bin",	 0x0b00, 0x0020, 0x78aded46 )	/* road red/green color table */
	ROM_LOAD( "pr1117.bin",	 0x0b20, 0x0020, 0xf06d9907 )	/* road green/blue color table */
	ROM_LOAD( "pr1115.bin",	 0x0b40, 0x0020, 0x5394092c )	/* road collision/enable */
	ROM_LOAD( "pr1116.bin",	 0x0b60, 0x0020, 0x3956767d )	/* collision detection */
	ROM_LOAD( "sndprom.bin", 0x0b80, 0x0020, 0xb369a6ae )
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, 0x628d3f1d )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, 0x591b6a68 )	/* timing - not used */
ROM_END


ROM_START( subroc3d )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1614a.88", 0x0000, 0x2000, 0x0ed856b4 )
	ROM_LOAD( "epr1615.87",  0x2000, 0x2000, 0x6281eb2e )
	ROM_LOAD( "epr1616.86",  0x4000, 0x2000, 0xcc7b0c9b )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* sprite data */
	ROM_LOAD( "epr1417.29",  0x00000, 0x2000, 0x2aaff4e0 )	/* level 0 */
	ROM_LOAD( "epr1418.30",  0x02000, 0x2000, 0x41ff0f15 )
	ROM_LOAD( "epr1419.55",  0x08000, 0x2000, 0x37ac818c )	/* level 1 */
	ROM_LOAD( "epr1420.56",  0x0a000, 0x2000, 0x41ff0f15 )
	ROM_LOAD( "epr1422.81",  0x10000, 0x2000, 0x0221db58 )	/* level 2 */
	ROM_LOAD( "epr1423.82",  0x12000, 0x2000, 0x08b1a4b8 )
	ROM_LOAD( "epr1421.80",  0x16000, 0x2000, 0x1db33c09 )
	ROM_LOAD( "epr1425.107", 0x18000, 0x2000, 0x0221db58 )	/* level 3 */
	ROM_LOAD( "epr1426.108", 0x1a000, 0x2000, 0x08b1a4b8 )
	ROM_LOAD( "epr1424.106", 0x1e000, 0x2000, 0x1db33c09 )
	ROM_LOAD( "epr1664.116", 0x20000, 0x2000, 0x6c93ece7 )	/* level 4 */
	ROM_LOAD( "epr1427.115", 0x22000, 0x2000, 0x2f8cfc2d )
	ROM_LOAD( "epr1429.117", 0x26000, 0x2000, 0x80e649c7 )
	ROM_LOAD( "epr1665.90",  0x28000, 0x2000, 0x6c93ece7 )	/* level 5 */
	ROM_LOAD( "epr1430.89",  0x2a000, 0x2000, 0x2f8cfc2d )
	ROM_LOAD( "epr1432.91",  0x2e000, 0x2000, 0xd9cd98d0 )
	ROM_LOAD( "epr1666.64",  0x30000, 0x2000, 0x6c93ece7 )	/* level 6 */
	ROM_LOAD( "epr1433.63",  0x32000, 0x2000, 0x2f8cfc2d )
	ROM_LOAD( "epr1436.66",  0x34000, 0x2000, 0xfc4ad926 )
	ROM_LOAD( "epr1435.65",  0x36000, 0x2000, 0x40662eef )
	ROM_LOAD( "epr1438.38",  0x38000, 0x2000, 0xd563d4c1 )	/* level 7 */
	ROM_LOAD( "epr1437.37",  0x3a000, 0x2000, 0x18ba6aad )
	ROM_LOAD( "epr1440.40",  0x3c000, 0x2000, 0x3a0e659c )
	ROM_LOAD( "epr1439.39",  0x3e000, 0x2000, 0x3d051668 )

	ROM_REGION( 0x01000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1618.82",  0x0000, 0x0800, 0xa25fea71 )
	ROM_LOAD( "epr1617.83",  0x0800, 0x0800, 0xf70c678e )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x01000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1419.108", 0x00000, 0x0200, 0x2cfa2a3f )  /* color prom */
	ROM_LOAD( "pr1620.62",  0x00200, 0x0100, 0x0ab7ef09 )  /* char color palette */
	ROM_LOAD( "pr1449.14",  0x00300, 0x0200, 0x5eb9ff47 )  /* ??? */
	ROM_LOAD( "pr1450.21",  0x00500, 0x0200, 0x66bdb00c )  /* sprite priority */
	ROM_LOAD( "pr1451.58",  0x00700, 0x0200, 0x6a575261 )  /* ??? */
	ROM_LOAD( "pr1453.39",  0x00900, 0x0020, 0x181c6d23 )  /* ??? */
	ROM_LOAD( "pr1454.67",  0x00920, 0x0020, 0xdc683440 )  /* ??? */
ROM_END


ROM_START( buckrog )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "br-3.bin", 0x0000, 0x4000, 0xf0055e97 )	/* encrypted */
	ROM_LOAD( "br-4.bin", 0x4000, 0x4000, 0x7d084c39 )	/* encrypted */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "5200.66", 0x0000, 0x1000, 0x0d58b154 )

	ROM_REGION( 0x40000, REGION_GFX1, 0 ) /* sprite data */
	ROM_LOAD( "5216.100",  0x00000, 0x2000, 0x8155bd73 )	/* level 0 */
	ROM_LOAD( "5213.84",   0x08000, 0x2000, 0xfd78dda4 )	/* level 1 */
	ROM_LOAD( "ic68",      0x10000, 0x4000, 0x2a194270 )	/* level 2 */
	ROM_LOAD( "ic52",      0x18000, 0x4000, 0xb31a120f )	/* level 3 */
	ROM_LOAD( "ic43",      0x20000, 0x4000, 0xd3584926 )	/* level 4 */
	ROM_LOAD( "ic59",      0x28000, 0x4000, 0xd83c7fcf )	/* level 5 */
	ROM_LOAD( "5208.58",   0x2c000, 0x2000, 0xd181fed2 )
	ROM_LOAD( "ic75",      0x30000, 0x4000, 0x1bd6e453 )	/* level 6 */
	ROM_LOAD( "5239.74",   0x34000, 0x2000, 0xc34e9b82 )
	ROM_LOAD( "ic91",      0x38000, 0x4000, 0x221f4ced )	/* level 7 */
	ROM_LOAD( "5238.90",   0x3c000, 0x2000, 0x7aff0886 )

	ROM_REGION( 0x01000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "5201.102",  0x0000, 0x0800, 0x7f21b0a4 )
	ROM_LOAD( "5202.103",  0x0800, 0x0800, 0x43f3e5a7 )

	ROM_REGION( 0x2000, REGION_GFX3, 0 )	/* background color data */
	ROM_LOAD( "5203.91", 0x0000, 0x2000, 0x631f5b65 )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x0600, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "ic95",    0x0000, 0x0400, 0x45e997a8 ) /* sprite colortable */
	ROM_LOAD( "5198.93", 0x0400, 0x0200, 0x32e74bc8 ) /* char colortable */
ROM_END


ROM_START( buckrogn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic3", 0x0000, 0x4000, 0x7f1910af )
	ROM_LOAD( "ic4", 0x4000, 0x4000, 0x5ecd393b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "5200.66", 0x0000, 0x1000, 0x0d58b154 )

	ROM_REGION( 0x40000, REGION_GFX1, 0 ) /* sprite data */
	ROM_LOAD( "5216.100",  0x00000, 0x2000, 0x8155bd73 )	/* level 0 */
	ROM_LOAD( "5213.84",   0x08000, 0x2000, 0xfd78dda4 )	/* level 1 */
	ROM_LOAD( "ic68",      0x10000, 0x4000, 0x2a194270 )	/* level 2 */
	ROM_LOAD( "ic52",      0x18000, 0x4000, 0xb31a120f )	/* level 3 */
	ROM_LOAD( "ic43",      0x20000, 0x4000, 0xd3584926 )	/* level 4 */
	ROM_LOAD( "ic59",      0x28000, 0x4000, 0xd83c7fcf )	/* level 5 */
	ROM_LOAD( "5208.58",   0x2c000, 0x2000, 0xd181fed2 )
	ROM_LOAD( "ic75",      0x30000, 0x4000, 0x1bd6e453 )	/* level 6 */
	ROM_LOAD( "5239.74",   0x34000, 0x2000, 0xc34e9b82 )
	ROM_LOAD( "ic91",      0x38000, 0x4000, 0x221f4ced )	/* level 7 */
	ROM_LOAD( "5238.90",   0x3c000, 0x2000, 0x7aff0886 )

	ROM_REGION( 0x01000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "5201.102",  0x0000, 0x0800, 0x7f21b0a4 )
	ROM_LOAD( "5202.103",  0x0800, 0x0800, 0x43f3e5a7 )

	ROM_REGION( 0x2000, REGION_GFX3, 0 )	/* background color data */
	ROM_LOAD( "5203.91", 0x0000, 0x2000, 0x631f5b65 )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x0600, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "ic95",    0x0000, 0x0400, 0x45e997a8 ) /* sprite colortable */
	ROM_LOAD( "5198.93", 0x0400, 0x0200, 0x32e74bc8 ) /* char colortable */
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static const UINT8 led_number_data[] =
{
	0x3e,0x41,0x41,0x41,0x00,0x41,0x41,0x41,0x3e,0x00,
	0x00,0x01,0x01,0x01,0x00,0x01,0x01,0x01,0x00,0x00,
	0x3e,0x01,0x01,0x01,0x3e,0x40,0x40,0x40,0x3e,0x00,
	0x3e,0x01,0x01,0x01,0x3e,0x01,0x01,0x01,0x3e,0x00,
	0x00,0x41,0x41,0x41,0x3e,0x01,0x01,0x01,0x00,0x00,
	0x3e,0x40,0x40,0x40,0x3e,0x01,0x01,0x01,0x3e,0x00,
	0x3e,0x40,0x40,0x40,0x3e,0x41,0x41,0x41,0x3e,0x00,
	0x3e,0x01,0x01,0x01,0x00,0x01,0x01,0x01,0x00,0x00,
	0x3e,0x41,0x41,0x41,0x3e,0x41,0x41,0x41,0x3e,0x00,
	0x3e,0x41,0x41,0x41,0x3e,0x01,0x01,0x01,0x3e,0x00
};

static const UINT8 led_tach_data[] =
{
	0xff,0x00
};


static DRIVER_INIT( common )
{
	memset(memory_region(REGION_GFX4), 0, memory_region_length(REGION_GFX4));
	memcpy(memory_region(REGION_GFX4), led_number_data, sizeof(led_number_data));
	memcpy(memory_region(REGION_GFX4)+0x100, led_tach_data, sizeof(led_tach_data));
}


static DRIVER_INIT( decode_turbo )
{
	init_common();
	turbo_rom_decode();
}


static DRIVER_INIT( decode_buckrog )
{
	init_common();
	buckrog_decode();
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1981, turbo,    0,       turbo,    turbo,    common,         ROT270,             "Sega", "Turbo", GAME_NO_COCKTAIL )
GAMEX( 1981, turboa,   turbo,   turbo,    turbo,    decode_turbo,   ROT270,             "Sega", "Turbo (encrypted set 1)", GAME_NO_COCKTAIL )
GAMEX( 1981, turbob,   turbo,   turbo,    turbo,    decode_turbo,   ROT270,             "Sega", "Turbo (encrypted set 2)", GAME_NO_COCKTAIL )
GAMEX( 1982, subroc3d, 0,       subroc3d, subroc3d, common,         ORIENTATION_FLIP_X, "Sega", "Subroc3D", GAME_NO_SOUND )
GAME ( 1982, buckrog,  0,       buckrog,  buckrog,  decode_buckrog, ROT0,               "Sega", "Buck Rogers: Planet of Zoom" )
GAME ( 1982, buckrogn, buckrog, buckrog,  buckrog,  common,         ROT0,               "Sega", "Buck Rogers: Planet of Zoom (not encrypted)" )
