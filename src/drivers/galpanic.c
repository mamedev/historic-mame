/***************************************************************************

Gals Panic       1990 Kaneko
Fantasia         1994 Comad
New Fantasia     1995 Comad
Miss World '96   1996 Comad
Fantasia II      1997 Comad

driver by Nicola Salmoria

The Comad games run on hardware similar to Gals Panic, with a different
sprite system. They are not ROM swaps because the addresses of work RAM
and of the OKI chip change from one to the other, however everything else
is pretty much identical.


TODO:
- There is a vector for IRQ4. The function does nothing in galpanic but is
  more complicated in the Comad ones. However I'm not triggering it, and
  they seems to work anyway...
- Four unknown ROMs in fantasia. The game seems to work fine without them.
- There was a ROM in the newfant set, obj2_14.rom, which was identical to
  Terminator 2's t2.107. I can only assume this was a mistake of the dumper.
- lots of unknown reads and writes, also in galpanic but particularly in
  the Comad ones.
- fantasia and newfant have a service mode but they doesn't work well (text
  is missing or replaced by garbage). This might be just how the games are.
- Is there a background enable register? Or a background clear. fantasia and
  newfant certainly look ugly as they are.

Notes about Gals Panic:
-----------------------
The current ROM set is strange because two ROMs overlap two others replacing
the program.

It's definitely a Kaneko boardset, but it could very well be they converted
some other game to run Gals Panic, because there's some ROMs piggybacked
on top of each other and some ROMs on a daughterboard plugged into smaller
sized ROM sockets. It's not a pirate version. The piggybacked ROMs even have
Kaneko stickers. The silkscreen on the board says PAMERA-4.

There is at least another version of the Gals Panic board. It's single board,
so no daughterboard. There are only 4 IC's socketed, the rest is soldered to
the board, and no piggybacked ROMs. Board number is MDK 321 V-0    EXPRO-02


Stephh's additional notes :

  - The games might not work correctly if sound is disabled.
  - There seems to exist 3 versions of 'galpanic' (Japan, US and World),
    and we seem to have a World version according to the coinage.
    Version is stored at 0x03ffff.b :
      * 01 : Japan
      * 02 : US
      * 03 : World
    In the version we have, you can only have one type of coinage
    (there is no Dip Switch to change sort of "coin mode").
  - In Comad games, here is a possible explanation of why the "Tilt" button
    may hang the game and/or crash/exit MAME : if you look carefully at the
    code, you'll notice that you have a "rts" instruction WITHOUT restoring
    the registers saved by the "movem.l D0-D7/A0-A6, -(A7)" instruction.
    Then, a "rte" instruction is performed.
  - The "Demo Sounds" Dip Switch is told not to work and not to fit the
    manual, but it appears that 00 seems to be read from in the "trap $d"
    interruption. Is it because the addresses (0x53e830-0x53e84f) are also
    used for 'galpanic_bgvideoram' ?
    In the Comad games, the interruption is the same, but the addresses
    which are checked are in full RAM. So the Dip Switch could be checked.

  - I've added a "fake" 'galpanib' romset which is in fact the same as
    'galpanic', but with the PRG ROMS which aren't overwritten.
    Here are a few notes about what I found :
      * This version is also a World version (0x03ffff.b = 03).
      * In this version, there is a "Coin Mode" Dip Switch, but no
        "Character Test" Dip Switch.
      * Area 0xe00000-0xe00014 is a "calculator" area. I've tried to
        simulate it (see 'galpanib_calc*' handlers) by comparing the code
        with the other set. I don't know if there are some other unmapped
        reads, but the game seems to run fine with what I've done.
      * When you press the "Tilt" button, the game enters in an endless
        loop, but this isn't a bug ! Check code begining at 0x000e02 and
        ending at 0x000976 for more infos.
      * Sound hasn't been tested.
      * The Comad games are definitively based on this version, the main
        differences being that read/writes to 0xe00000 have been replaced.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern data16_t *galpanic_bgvideoram,*galpanic_fgvideoram;
extern size_t galpanic_fgvideoram_size;

PALETTE_INIT( galpanic );
WRITE16_HANDLER( galpanic_bgvideoram_w );
WRITE16_HANDLER( galpanic_paletteram_w );
VIDEO_UPDATE( galpanic );
VIDEO_UPDATE( comad );




static INTERRUPT_GEN( galpanic_interrupt )
{
	/* IRQ 3 drives the game, IRQ 5 updates the palette */
	if (cpu_getiloops() != 0)
		cpu_set_irq_line(0, 5, HOLD_LINE);
	else
		cpu_set_irq_line(0, 3, HOLD_LINE);
}

static WRITE16_HANDLER( galpanic_6295_bankswitch_w )
{
	if (ACCESSING_MSB)
	{
		UINT8 *rom = memory_region(REGION_SOUND1);

		memcpy(&rom[0x30000],&rom[0x40000 + ((data >> 8) & 0x0f) * 0x10000],0x10000);
	}
}


static data16_t *galpanib_calc_data;

static READ16_HANDLER(galpanib_calc_r)
{
	UINT16 data = 0;

	switch (offset)
	{
		case 0x00 >> 1:	// watchdog ?
			return 0;

		case 0x04 >> 1:
			if (galpanib_calc_data[0x00 >> 1] >  galpanib_calc_data[0x08 >> 1])
				data |= 0x00000002;	// bit 1	(cmp [0xe00000], [0xe00008])
			if (galpanib_calc_data[0x00 >> 1] == galpanib_calc_data[0x08 >> 1])
				data |= 0x00000004;	// bit 2	(cmp [0xe00000], [0xe00008])
			if (galpanib_calc_data[0x00 >> 1] <  galpanib_calc_data[0x08 >> 1])
				data |= 0x00000008;	// bit 3	(cmp [0xe00000], [0xe00008])

			if (galpanib_calc_data[0x04 >> 1] >  galpanib_calc_data[0x0c >> 1])
				data |= 0x00000020;	// bit 5	(cmp [0xe00004], [0xe0000c])
			if (galpanib_calc_data[0x04 >> 1] == galpanib_calc_data[0x0c >> 1])
				data |= 0x00000040;	// bit 6	(cmp [0xe00004], [0xe0000c])
			if (galpanib_calc_data[0x04 >> 1] <  galpanib_calc_data[0x0c >> 1])
				data |= 0x00000080;	// bit 7	(cmp [0xe00004], [0xe0000c])

			return ((data & 0xff) << 8);

		case 0x10 >> 1:
			return ((((UINT32)(galpanib_calc_data[0x10 >> 1] * galpanib_calc_data[0x12 >> 1])) & 0xffff0000) >> 16);
		case 0x12 >> 1:
			return ((((UINT32)(galpanib_calc_data[0x10 >> 1] * galpanib_calc_data[0x12 >> 1])) & 0x0000ffff) >> 0);

		case 0x14 >> 1:
			return (rand() & 0xffff);

		default:
			logerror("CPU #0 PC %06x: warning - read unmapped calc address %06x\n",activecpu_get_pc(),offset<<1);
	}

	return galpanib_calc_data[offset];
}

static WRITE16_HANDLER(galpanib_calc_w)
{
	COMBINE_DATA(&galpanib_calc_data[offset]);
}


static MEMORY_READ16_START( galpanic_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_status_0_lsb_r },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x7047ff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
MEMORY_END

static MEMORY_WRITE16_START( galpanic_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_data_0_lsb_w },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x7047ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* ??? */
	{ 0xb00000, 0xb00001, MWA16_NOP },	/* ??? */
	{ 0xc00000, 0xc00001, MWA16_NOP },	/* ??? */
MEMORY_END

static MEMORY_READ16_START( galpanib_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_status_0_lsb_r },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x7047ff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
	{ 0xe00000, 0xe00015, galpanib_calc_r },
MEMORY_END

static MEMORY_WRITE16_START( galpanib_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_data_0_lsb_w },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x7047ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* ??? */
	{ 0xb00000, 0xb00001, MWA16_NOP },	/* ??? */
	{ 0xc00000, 0xc00001, MWA16_NOP },	/* ??? */
	{ 0xe00000, 0xe00015, galpanib_calc_w, &galpanib_calc_data },
MEMORY_END

static READ16_HANDLER( kludge )
{
	return rand() & 0x0700;
}

static MEMORY_READ16_START( comad_readmem )
	{ 0x000000, 0x4fffff, MRA16_ROM },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x700fff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
//	{ 0x800006, 0x800007,  },	??
	{ 0x80000a, 0x80000b, kludge },	/* bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0x80000c, 0x80000d, kludge },	/* missw96 bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0xc00000, 0xc0ffff, MRA16_RAM },	/* missw96 */
	{ 0xc80000, 0xc8ffff, MRA16_RAM },	/* fantasia, newfant */
	{ 0xf00000, 0xf00001, OKIM6295_status_0_msb_r },	/* fantasia, missw96 */
	{ 0xf80000, 0xf80001, OKIM6295_status_0_msb_r },	/* newfant */
MEMORY_END

static MEMORY_WRITE16_START( comad_writemem )
	{ 0x000000, 0x4fffff, MWA16_ROM },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },	/* not sure */
	{ 0xc00000, 0xc0ffff, MWA16_RAM },	/* missw96 */
	{ 0xc80000, 0xc8ffff, MWA16_RAM },	/* fantasia, newfant */
	{ 0xf00000, 0xf00001, OKIM6295_data_0_msb_w },	/* fantasia, missw96 */
	{ 0xf80000, 0xf80001, OKIM6295_data_0_msb_w },	/* newfant */
MEMORY_END

static MEMORY_READ16_START( fantsia2_readmem )
	{ 0x000000, 0x4fffff, MRA16_ROM },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x700fff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
//	{ 0x800006, 0x800007,  },	??
	{ 0x800008, 0x800009, kludge },	/* bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0xf80000, 0xf8ffff, MRA16_RAM },
	{ 0xc80000, 0xc80001, OKIM6295_status_0_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( fantsia2_writemem )
	{ 0x000000, 0x4fffff, MWA16_ROM },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },	/* not sure */
	{ 0xf80000, 0xf8ffff, MWA16_RAM },
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* coin counters, + ? */
	{ 0xc80000, 0xc80001, OKIM6295_data_0_msb_w },
MEMORY_END



INPUT_PORTS_START( galpanic )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x000522 */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	/* Coinage - World (0x03ffff.b = 03) */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coinage - Japan (0x03ffff.b = 01) and US (0x03ffff.b = 02)
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - see notes */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Character Test" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( galpanib )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x00060a */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, "Coin Mode" )
	PORT_DIPSETTING(      0x0008, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coinage - Japan (0x03ffff.b = 01)
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	*/
	/* Coin Mode 1 - US (0x03ffff.b = 02) and World (0x03ffff.b = 03) */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coin Mode 2 - US (0x03ffff.b = 02) and World (0x03ffff.b = 03)
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - see notes */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( fantasia )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )	/* freeze/vblank? - code at 0x000734 ('fantasia') */
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )		/* or 0x00075a ('newfant') - not called ? */
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - same "trap    #$d" as in 'galpanic' */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x00021c */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, "Coin Mode" )
	PORT_DIPSETTING(      0x0008, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )		/* MAME may crash when pressed (see notes) */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Service" in "test mode" */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* Same as 'fantasia', but no "Service Mode" Dip Switch (and thus no "hidden" buttons) */
INPUT_PORTS_START( missw96 )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )	/* freeze/vblank? - code at 0x00074e - not called ? */
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - same "trap    #$d" as in 'galpanic' */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x00021c */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Coin Mode" )
	PORT_DIPSETTING(      0x0008, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )		/* MAME may crash when pressed (see notes) */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			64*4, 65*4, 66*4, 67*4, 68*4, 69*4, 70*4, 71*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,  256, 16 },
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 12000 },          /* 12000Hz frequency */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }
};


static MACHINE_DRIVER_START( galpanic )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 8000000)
	MDRV_CPU_MEMORY(galpanic_readmem,galpanic_writemem)
	MDRV_CPU_VBLANK_INT(galpanic_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024 + 32768)
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_PALETTE_INIT(galpanic)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(galpanic)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( galpanib )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(galpanic)
	MDRV_CPU_REPLACE("main", M68000, 10000000)
	MDRV_CPU_MEMORY(galpanib_readmem,galpanib_writemem)

	/* video hardware */
//	MDRV_VIDEO_UPDATE(comad)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( comad )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(galpanic)
	MDRV_CPU_REPLACE("main", M68000, 10000000)
	MDRV_CPU_MEMORY(comad_readmem,comad_writemem)

	/* video hardware */
	MDRV_VIDEO_UPDATE(comad)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fantsia2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(comad)
	MDRV_CPU_REPLACE("main", M68000, 12000000)	/* ? */
	MDRV_CPU_MEMORY(fantsia2_readmem,fantsia2_writemem)

	/* video hardware */
	MDRV_VIDEO_UPDATE(comad)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galpanic )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110.4m2",    0x000000, 0x80000, 0xae6b17a8 )
	ROM_LOAD16_BYTE( "pm109.4m1",    0x000001, 0x80000, 0xb85d792d )
	/* The above two ROMs contain valid 68000 code, but the game doesn't */
	/* work. I think there might be a protection (addressed at e00000). */
	/* The two following ROMs replace the code with a working version. */
	ROM_LOAD16_BYTE( "pm112.6",      0x000000, 0x20000, 0x7b972b58 )
	ROM_LOAD16_BYTE( "pm111.5",      0x000001, 0x20000, 0x4eb7298d )
	ROM_LOAD16_BYTE( "pm004e.8",     0x100001, 0x80000, 0xd3af52bc )
	ROM_LOAD16_BYTE( "pm005e.7",     0x100000, 0x80000, 0xd7ec650c )
	ROM_LOAD16_BYTE( "pm000e.15",    0x200001, 0x80000, 0x5d220f3f )
	ROM_LOAD16_BYTE( "pm001e.14",    0x200000, 0x80000, 0x90433eb1 )
	ROM_LOAD16_BYTE( "pm002e.17",    0x300001, 0x80000, 0x713ee898 )
	ROM_LOAD16_BYTE( "pm003e.16",    0x300000, 0x80000, 0x6bb060fd )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "pm006e.67",    0x000000, 0x100000, 0x57aec037 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008e.l",     0x00000, 0x80000, 0xd9379ba8 )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "pm007e.u",     0xc0000, 0x80000, 0xc7ed7950 )
ROM_END

ROM_START( galpanib )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110.4m2",    0x000000, 0x80000, 0xae6b17a8 )
	ROM_LOAD16_BYTE( "pm109.4m1",    0x000001, 0x80000, 0xb85d792d )
	ROM_LOAD16_BYTE( "pm004e.8",     0x100001, 0x80000, 0xd3af52bc )
	ROM_LOAD16_BYTE( "pm005e.7",     0x100000, 0x80000, 0xd7ec650c )
	ROM_LOAD16_BYTE( "pm000e.15",    0x200001, 0x80000, 0x5d220f3f )
	ROM_LOAD16_BYTE( "pm001e.14",    0x200000, 0x80000, 0x90433eb1 )
	ROM_LOAD16_BYTE( "pm002e.17",    0x300001, 0x80000, 0x713ee898 )
	ROM_LOAD16_BYTE( "pm003e.16",    0x300000, 0x80000, 0x6bb060fd )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "pm006e.67",    0x000000, 0x100000, 0x57aec037 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008e.l",     0x00000, 0x80000, 0xd9379ba8 )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "pm007e.u",     0xc0000, 0x80000, 0xc7ed7950 )
ROM_END

ROM_START( fantasia )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2_16.rom", 0x000000, 0x80000, 0xe27c6c57 )
	ROM_LOAD16_BYTE( "prog1_13.rom", 0x000001, 0x80000, 0x68d27413 )
	ROM_LOAD16_BYTE( "iscr6_09.rom", 0x100000, 0x80000, 0x2a588393 )
	ROM_LOAD16_BYTE( "iscr5_05.rom", 0x100001, 0x80000, 0x6160e0f0 )
	ROM_LOAD16_BYTE( "iscr4_08.rom", 0x200000, 0x80000, 0xf776b743 )
	ROM_LOAD16_BYTE( "iscr3_04.rom", 0x200001, 0x80000, 0x5df0dff2 )
	ROM_LOAD16_BYTE( "iscr2_07.rom", 0x300000, 0x80000, 0x5707d861 )
	ROM_LOAD16_BYTE( "iscr1_03.rom", 0x300001, 0x80000, 0x36cb811a )
	ROM_LOAD16_BYTE( "imag2_10.rom", 0x400000, 0x80000, 0x1f14a395 )
	ROM_LOAD16_BYTE( "imag1_06.rom", 0x400001, 0x80000, 0xfaf870e4 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1_17.rom",  0x00000, 0x80000, 0xaadb6eb7 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "mus-1_01.rom", 0x00000, 0x80000, 0x22955efb )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "mus-2_02.rom", 0xc0000, 0x80000, 0x4cd4d6c3 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD16_BYTE( "gscr2_15.rom", 0x000000, 0x80000, 0x46666768 )
	ROM_LOAD16_BYTE( "gscr1_12.rom", 0x000001, 0x80000, 0x4bd25be6 )
	ROM_LOAD16_BYTE( "gscr4_14.rom", 0x100000, 0x80000, 0x4e7e6ed4 )
	ROM_LOAD16_BYTE( "gscr3_11.rom", 0x100001, 0x80000, 0x6d00a4c5 )
ROM_END

ROM_START( newfant )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2_12.rom", 0x000000, 0x80000, 0xde43a457 )
	ROM_LOAD16_BYTE( "prog1_07.rom", 0x000001, 0x80000, 0x370b45be )
	ROM_LOAD16_BYTE( "iscr2_10.rom", 0x100000, 0x80000, 0x4f2da2eb )
	ROM_LOAD16_BYTE( "iscr1_05.rom", 0x100001, 0x80000, 0x63c6894f )
	ROM_LOAD16_BYTE( "iscr4_09.rom", 0x200000, 0x80000, 0x725741ec )
	ROM_LOAD16_BYTE( "iscr3_04.rom", 0x200001, 0x80000, 0x51d6b362 )
	ROM_LOAD16_BYTE( "iscr6_08.rom", 0x300000, 0x80000, 0x178b2ef3 )
	ROM_LOAD16_BYTE( "iscr5_03.rom", 0x300001, 0x80000, 0xd2b5c5fa )
	ROM_LOAD16_BYTE( "iscr8_11.rom", 0x400000, 0x80000, 0xf4148528 )
	ROM_LOAD16_BYTE( "iscr7_06.rom", 0x400001, 0x80000, 0x2dee0c31 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1_13.rom",  0x00000, 0x80000, 0xe6d1bc71 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "musc1_01.rom", 0x00000, 0x80000, 0x10347fce )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "musc2_02.rom", 0xc0000, 0x80000, 0xb9646a8c )
ROM_END

ROM_START( missw96 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "mw96_10.bin",  0x000000, 0x80000, 0xb1309bb1 )
	ROM_LOAD16_BYTE( "mw96_06.bin",  0x000001, 0x80000, 0xa5892bb3 )
	ROM_LOAD16_BYTE( "mw96_09.bin",  0x100000, 0x80000, 0x7032dfdf )
	ROM_LOAD16_BYTE( "mw96_05.bin",  0x100001, 0x80000, 0x91de5ab5 )
	ROM_LOAD16_BYTE( "mw96_08.bin",  0x200000, 0x80000, 0xb8e66fb5 )
	ROM_LOAD16_BYTE( "mw96_04.bin",  0x200001, 0x80000, 0xe77a04f8 )
	ROM_LOAD16_BYTE( "mw96_07.bin",  0x300000, 0x80000, 0x26112ed3 )
	ROM_LOAD16_BYTE( "mw96_03.bin",  0x300001, 0x80000, 0xe9374a46 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "mw96_11.bin",  0x00000, 0x80000, 0x3983152f )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "mw96_01.bin",  0x00000, 0x80000, 0xe78a659e )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "mw96_02.bin",  0xc0000, 0x80000, 0x60fa0c00 )
ROM_END

ROM_START( fantsia2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2.g17",    0x000000, 0x80000, 0x57c59972 )
	ROM_LOAD16_BYTE( "prog1.f17",    0x000001, 0x80000, 0xbf2d9a26 )
	ROM_LOAD16_BYTE( "scr2.g16",     0x100000, 0x80000, 0x887b1bc5 )
	ROM_LOAD16_BYTE( "scr1.f16",     0x100001, 0x80000, 0xcbba3182 )
	ROM_LOAD16_BYTE( "scr4.g15",     0x200000, 0x80000, 0xce97e411 )
	ROM_LOAD16_BYTE( "scr3.f15",     0x200001, 0x80000, 0x480cc2e8 )
	ROM_LOAD16_BYTE( "scr6.g14",     0x300000, 0x80000, 0xb29d49de )
	ROM_LOAD16_BYTE( "scr5.f14",     0x300001, 0x80000, 0xd5f88b83 )
	ROM_LOAD16_BYTE( "scr8.g20",     0x400000, 0x80000, 0x694ae2b3 )
	ROM_LOAD16_BYTE( "scr7.f20",     0x400001, 0x80000, 0x6068712c )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1.1i",      0x00000, 0x80000, 0x52e6872a )
	ROM_LOAD( "obj2.2i",      0x80000, 0x80000, 0xea6e3861 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "music2.1b",    0x00000, 0x80000, 0x23cc4f9c )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "music1.1a",    0xc0000, 0x80000, 0x864167c2 )
ROM_END


static DRIVER_INIT( galpanib )
{
	/* Hack to avoid the game to loop at the begining (check code at 0x000570) */
	/* I have no hell of an idea of the reason of the read in the code ! */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	RAM[0x000588/2] = 0x4e71;
}


GAMEX( 1990, galpanic, 0,        galpanic, galpanic, 0,        ROT90, "Kaneko", "Gals Panic (set 1)", GAME_NO_COCKTAIL )
GAMEX( 1990, galpanib, galpanic, galpanib, galpanib, galpanib, ROT90, "Kaneko", "Gals Panic (set 2)", GAME_NO_COCKTAIL )
GAMEX( 1994, fantasia, 0,        comad,    fantasia, 0,        ROT90, "Comad & New Japan System", "Fantasia", GAME_NO_COCKTAIL )
GAMEX( 1995, newfant,  0,        comad,    fantasia, 0,        ROT90, "Comad & New Japan System", "New Fantasia", GAME_NO_COCKTAIL )
GAMEX( 1996, missw96,  0,        comad,    missw96,  0,        ROT0,  "Comad", "Miss World '96 Nude", GAME_NO_COCKTAIL )
GAMEX( 1997, fantsia2, 0,        fantsia2, missw96,  0,        ROT0,  "Comad", "Fantasia II", GAME_NO_COCKTAIL )
