/*******************************************************************************

	Pro Soccer						(c) 1983 Data East Corporation
	Pro Sport						(c) 1983 Data East Corporation
	Boomer Rang'R / Genesis			(c) 1983 Data East Corporation
	Kamikaze Cabbie / Yellow Cab	(c) 1984 Data East Corporation
	Liberation						(c) 1984 Data East Corporation

	Liberation was available on two pcbs - a dedicated twin pcb set and
	a version on the Genesis/Yellow Cab pcb that had an extra cpu pcb attached
	for the different protection.  The program is the same on both versions.

	Emulation by Bryan McPhail, mish@tendril.co.uk

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"

PALETTE_INIT( liberate );
VIDEO_UPDATE( prosoccr );
VIDEO_UPDATE( prosport );
VIDEO_UPDATE( liberate );
VIDEO_UPDATE( boomrang );
VIDEO_START( prosoccr );
VIDEO_START( prosport );
VIDEO_START( boomrang );
VIDEO_START( liberate );

static int deco16_bank;
static data8_t *scratchram;

WRITE_HANDLER( deco16_io_w );
WRITE_HANDLER( prosport_paletteram_w );
WRITE_HANDLER( liberate_videoram_w );

/***************************************************************************/

static READ_HANDLER( deco16_bank_r )
{
	const data8_t *ROM = memory_region(REGION_USER1);

	/* The tilemap bank can be swapped into main memory */
	if (deco16_bank)
		return ROM[offset];

	/* Else the handler falls through to read the usual address */
	if (offset<0x800) return videoram[offset];
	if (offset<0x1000) return spriteram[offset-0x800];
	if (offset<0x2200) { logerror("%04x: Unmapped bank read %04x\n",activecpu_get_pc(),offset); return 0; }
	if (offset<0x2800) return scratchram[offset-0x2200];

	logerror("%04x: Unmapped bank read %04x\n",activecpu_get_pc(),offset);
	return 0;
}

static WRITE_HANDLER( deco16_bank_w )
{
	deco16_bank=data;
}

static READ_HANDLER( deco16_io_r )
{
	const data8_t *ROM = memory_region(REGION_CPU1);

	if (deco16_bank) {
		if (offset==0) return readinputport(1); /* Player 1 controls */
		if (offset==1) return readinputport(2); /* Player 2 controls */
		if (offset==2) return readinputport(3); /* Vblank, coins */
		if (offset==3) return readinputport(4); /* Dip 1 */
		if (offset==4) return readinputport(5); /* Dip 2 */

		logerror("%04x:  Read input %d\n",activecpu_get_pc(),offset);
		return 0xff;
	}
	return ROM[0x8000+offset];
}

/***************************************************************************/

static MEMORY_READ_START( prosport_readmem )
	{ 0x0200, 0x021f, paletteram_r },
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x2fff, MRA_RAM },
	{ 0x8000, 0x800f, deco16_io_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( prosport_writemem )
	{ 0x0200, 0x021f, prosport_paletteram_w, &paletteram },
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1200, 0x1fff, MWA_RAM },
	{ 0x3000, 0x37ff, liberate_videoram_w, &videoram },
	{ 0x3800, 0x3fff, MWA_RAM, &spriteram },
	{ 0x8000, 0x800f, deco16_io_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( liberate_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x3fff, MRA_ROM }, /* Mirror of main rom */
	{ 0x4000, 0x7fff, deco16_bank_r },
	{ 0x8000, 0x800f, deco16_io_r },
	{ 0x6200, 0x67ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( liberate_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x3fff, MWA_ROM }, /* Mirror of main rom */
	{ 0x4000, 0x47ff, liberate_videoram_w, &videoram },
	{ 0x4800, 0x4fff, MWA_RAM, &spriteram },
	{ 0x6200, 0x67ff, MWA_RAM, &scratchram },
	{ 0x8000, 0x800f, deco16_io_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( deco16_readport )
	{ 0x00, 0x00, input_port_0_r },
PORT_END

static PORT_WRITE_START( deco16_writeport )
	{ 0x00, 0x00, deco16_bank_w },
PORT_END

static MEMORY_READ_START( liberatb_readmem )
	{ 0x00fe, 0x00fe, input_port_0_r },
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x3fff, MRA_ROM }, /* Mirror of main rom */
	{ 0x4000, 0x7fff, deco16_bank_r },
	{ 0xf000, 0xf00f, deco16_io_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( liberatb_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x3fff, MWA_ROM }, /* Mirror of main rom */
	{ 0x4000, 0x47ff, liberate_videoram_w, &videoram },
	{ 0x4800, 0x4fff, MWA_RAM, &spriteram },
	{ 0x6200, 0x67ff, MWA_RAM, &scratchram },
//	{ 0xf000, 0xf00f, deco16_io_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/***************************************************************************/

static MEMORY_READ_START( prosoccr_sound_readmem )
    { 0x0000, 0x01ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
    { 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( prosoccr_sound_writemem )
    { 0x0000, 0x01ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x4000, 0x4000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x8000, 0x8000, AY8910_control_port_1_w },
    { 0xe000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
    { 0x0000, 0x01ff, MRA_RAM },
	{ 0xb000, 0xb000, soundlatch_r },
    { 0xc000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x0000, 0x01ff, MWA_RAM },
	{ 0x3000, 0x3000, AY8910_write_port_0_w },
	{ 0x4000, 0x4000, AY8910_control_port_0_w },
	{ 0x7000, 0x7000, AY8910_write_port_1_w },
	{ 0x8000, 0x8000, AY8910_control_port_1_w },
    { 0xc000, 0xffff, MWA_ROM },
MEMORY_END

/***************************************************************************/

INPUT_PORTS_START( boomrang )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Manufacturer" )
	PORT_DIPSETTING(    0x00, "Data East USA" )
	PORT_DIPSETTING(    0x80, "Data East Corporation" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* Difficulty? */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) /* Difficulty? */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Invincibility" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( kamikcab )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Manufacturer" )
	PORT_DIPSETTING(    0x00, "Data East USA" )
	PORT_DIPSETTING(    0x80, "Data East Corporation" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x0c, "20000 30000" )
	PORT_DIPSETTING(    0x08, "30000 40000" )
	PORT_DIPSETTING(    0x04, "40000 50000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* Difficulty? */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) /* Difficulty? */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( liberate )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Manufacturer" )
	PORT_DIPSETTING(    0x00, "Data East USA" )
	PORT_DIPSETTING(    0x80, "Data East Corporation" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x0c, "20000 30000" )
	PORT_DIPSETTING(    0x08, "30000 50000" )
	PORT_DIPSETTING(    0x04, "50000 70000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) /* Difficulty? */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) /* Difficulty? */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
 	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout sprites =
{
	16,16,
	RGN_FRAC(1,3),
	3,
 	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
		0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	16*16
};

static struct GfxLayout pro_tiles =
{
	16,16,
	16,
	2,
	{ 0, 4, 1024*8, 1024*8+4 },
	{
 		24,25,26,27, 16,17,18,19, 8,9,10,11, 0,1,2,3
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32
	},
	64*8
};

static struct GfxLayout tiles1 =
{
	16,16,
	128,
	3,
	{ 4, 0, 0x4000*8+4 },
	{
		24,25,26,27, 16,17,18,19, 8,9,10,11, 0,1,2,3
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32
	},
	64*8
};

static struct GfxLayout tiles2 =
{
	16,16,
	128,
	3,
	{ 0x2000*8+4, 0x2000*8+0, 0x4000*8 },
	{
		24,25,26,27, 16,17,18,19, 8,9,10,11, 0,1,2,3
	},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,  0, 4 },
	{ REGION_GFX1, 0x00000, &sprites,     0, 4 },
	{ REGION_GFX2, 0x00000, &tiles1,      0, 4 },
	{ REGION_GFX2, 0x00000, &tiles2,      0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo prosport_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,  0, 4 },
	{ REGION_GFX1, 0x00000, &sprites,     0, 4 },
	{ REGION_GFX2, 0x00000, &pro_tiles,   0, 4 },
	{ REGION_GFX2, 0x00800, &pro_tiles,   0, 4 },
	{ -1 } /* end of array */
};

/***************************************************************************/

static INTERRUPT_GEN( deco16_interrupt )
{
	static int latch=0;
	int p=~readinputport(3);
	if (p&0x43 && !latch) {
		cpu_set_irq_line(0,DECO16_IRQ_LINE,ASSERT_LINE);
		latch=1;
	} else {
		if (!(p&0x43))
			latch=0;
	}
}

static struct AY8910interface ay8910_interface =
{
	2, /* 2 chips */
	1500000,     /* 12 Mhz / 8 = 1.5 Mhz */
	{ 30, 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( prosoccr )

	/* basic machine hardware */
	MDRV_CPU_ADD(DECO16, 3000000)
	MDRV_CPU_MEMORY(liberate_readmem,liberate_writemem)
	MDRV_CPU_PORTS(deco16_readport,deco16_writeport)

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(prosoccr_sound_readmem,prosoccr_sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529) /* 529ms Vblank duration?? */
	MDRV_INTERLEAVE(200)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(33)
	MDRV_PALETTE_INIT(liberate)

	MDRV_VIDEO_START(prosoccr)
	MDRV_VIDEO_UPDATE(prosoccr)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( prosport )

	/* basic machine hardware */
	MDRV_CPU_ADD(DECO16, 2000000)
	MDRV_CPU_MEMORY(prosport_readmem,prosport_writemem)
	MDRV_CPU_PORTS(deco16_readport,deco16_writeport)

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529) /* 529ms Vblank duration?? */
	MDRV_INTERLEAVE(200)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(prosport_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(boomrang)
	MDRV_VIDEO_UPDATE(prosport)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( boomrang )

	/* basic machine hardware */
	MDRV_CPU_ADD(DECO16, 2000000)
	MDRV_CPU_MEMORY(liberate_readmem,liberate_writemem)
	MDRV_CPU_PORTS(deco16_readport,deco16_writeport)
	MDRV_CPU_VBLANK_INT(deco16_interrupt,1)

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529) /* 529ms Vblank duration?? */
	MDRV_INTERLEAVE(200)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(33)
	MDRV_PALETTE_INIT(liberate)

	MDRV_VIDEO_START(boomrang)
	MDRV_VIDEO_UPDATE(boomrang)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( liberate )

	/* basic machine hardware */
	MDRV_CPU_ADD(DECO16, 2000000)
	MDRV_CPU_MEMORY(liberate_readmem,liberate_writemem)
	MDRV_CPU_PORTS(deco16_readport,deco16_writeport)
	MDRV_CPU_VBLANK_INT(deco16_interrupt,1)

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529) /* 529ms Vblank duration?? */
	MDRV_INTERLEAVE(200)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(33)
	MDRV_PALETTE_INIT(liberate)

	MDRV_VIDEO_START(liberate)
	MDRV_VIDEO_UPDATE(liberate)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( liberatb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 2000000)
	MDRV_CPU_MEMORY(liberatb_readmem,liberatb_writemem)
	MDRV_CPU_PORTS(deco16_readport,deco16_writeport)
	MDRV_CPU_VBLANK_INT(deco16_interrupt,1) //todo

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529) /* 529ms Vblank duration?? */
	MDRV_INTERLEAVE(200)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(33)
	MDRV_PALETTE_INIT(liberate)

	MDRV_VIDEO_START(liberate)
	MDRV_VIDEO_UPDATE(liberate)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( prosoccr )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD( "am07.7e",  0x8000, 0x2000, 0x55415fb5 )
	ROM_LOAD( "am08.9e",  0xa000, 0x2000, 0x73d45d0d )
	ROM_LOAD( "am09.10e", 0xc000, 0x2000, 0xa7ee0b3a )
	ROM_LOAD( "am10.11e", 0xe000, 0x2000, 0x5571bdb8 )
//low reload??
	ROM_REGION( 0x10000 * 2, REGION_CPU2, 0 )
	ROM_LOAD( "am06.10a", 0xe000, 0x2000, 0x37a0c74f )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "am00.2b",  0x0000, 0x2000, 0xf3c8b649 )
	ROM_LOAD( "am01.5b",  0x2000, 0x2000, 0x24785bda )
	ROM_LOAD( "am02.7b",  0x4000, 0x2000, 0xc5af58ea )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "am03.10b", 0x0000, 0x2000, 0x47dc31dc )
	ROM_LOAD( "am04.c10", 0x2000, 0x2000, 0xe057d827 )

	ROM_REGION(0x04000, REGION_USER1, 0 )
	ROM_LOAD( "am05.d12", 0x0000, 0x2000,  0xf63e5a73 )

	ROM_REGION( 64, REGION_PROMS, 0 )
	ROM_LOAD( "k1",    0, 32,  0xebdc8343 ) /* Colour */
	ROM_LOAD( "e13",  32, 32,  0x6909a061 ) /* Timing? */
ROM_END

ROM_START( prosport )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD( "ic21ar09.bin", 0x4000, 0x2000,  0x4faa8d12 )
	ROM_LOAD( "ic22ar10.bin", 0x6000, 0x2000,  0x389e405b )
	ROM_LOAD( "ic23ar11.bin", 0x8000, 0x2000,  0xc0bc7f2a )
	ROM_LOAD( "ic24ar12.bin", 0xa000, 0x2000,  0x4acd3f0d )
	ROM_LOAD( "ic25ar13.bin", 0xc000, 0x2000,  0x2bdabdf3 )
	ROM_LOAD( "ic26ar14.bin", 0xe000, 0x2000,  0x10ccfddb )

	ROM_REGION( 0x10000 * 2, REGION_CPU2, 0 )
	ROM_LOAD( "ic43ar16.bin", 0xc000, 0x2000,  0x113a4f89 )
	ROM_LOAD( "ic42ar15.bin", 0xe000, 0x2000,  0x635425a6 )

	ROM_REGION( 0x12000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic52ar00.bin",   0x00000, 0x2000, 0x1e16adde )
	ROM_LOAD( "ic53ar01.bin",   0x02000, 0x2000, 0x4b7a6431 )
	ROM_LOAD( "ic54ar02.bin",   0x04000, 0x2000, 0x039eba80 )

	ROM_LOAD( "ic55ar03.bin",   0x06000, 0x2000, 0xcaecafcb )
	ROM_LOAD( "ic56ar04.bin",   0x08000, 0x2000, 0xd555835e )
	ROM_LOAD( "ic57ar05.bin",   0x0a000, 0x2000, 0x9d05c4cc )

	ROM_LOAD( "ic58ar06.bin",   0x0c000, 0x2000, 0x903ea834 )
	ROM_LOAD( "ic59ar07.bin",   0x0e000, 0x2000, 0xe6527838 )
	ROM_LOAD( "ic60ar08.bin",   0x10000, 0x2000, 0xff1e6b01 )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic46ar18.bin",   0x00000, 0x1000, 0xd23998d3 )
	ROM_LOAD( "ic45ar17.bin",   0x01000, 0x1000, 0x5f1c621e )
ROM_END

ROM_START( boomrang )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD( "bp13.9k",  0x8000, 0x4000,  0xb70439b1 )
	ROM_RELOAD(           0x0000, 0x4000 )
	ROM_LOAD( "bp14.11k", 0xc000, 0x4000,  0x98050e13 )

	ROM_REGION(0x10000*2, REGION_CPU2, 0)
	ROM_LOAD( "bp11.11f", 0xc000, 0x4000,  0xd6106f00 )

	ROM_REGION(0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bp04.7b",  0x00000, 0x4000, 0x5d4b12eb )
	ROM_LOAD( "bp06.10b", 0x04000, 0x4000, 0x5a18296e )
	ROM_LOAD( "bp08.13b", 0x08000, 0x4000, 0x4cdb30d9 )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bp02.4b",  0x00000, 0x4000, 0xf3c2b84f )
	ROM_LOAD( "bp00.1b",  0x04000, 0x4000, 0x3370cf6e )

	ROM_REGION(0x04000, REGION_USER1, 0 )
	ROM_LOAD( "bp10.10a", 0x0000, 0x4000,  0xdd18a96f )

	ROM_REGION( 32, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.5l",  0, 32,  0xa71e19ff )
ROM_END

ROM_START( kamikcab )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD( "bp11", 0x0c000, 0x4000, 0xa69e5580 )
	ROM_RELOAD(       0x00000, 0x4000 )

	ROM_REGION(0x10000*2, REGION_CPU2, 0)	/* 64K for CPU 2 */
	ROM_LOAD( "bp09", 0x0e000, 0x2000, 0x16b13676 )

	ROM_REGION(0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bp04", 0x00000, 0x4000, 0xb471542d )
	ROM_LOAD( "bp06", 0x04000, 0x4000, 0x4bf96d0d )
	ROM_LOAD( "bp08", 0x08000, 0x4000, 0xb4756bed )

	ROM_REGION(0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bp02", 0x00000, 0x4000, 0x77299e6e )
	ROM_LOAD( "bp00", 0x04000, 0x2000, 0xc20ca7ca )

	ROM_REGION(0x4000, REGION_USER1, 0 )
	ROM_LOAD( "bp12", 0x00000, 0x4000, 0x8c8f5d35 )

	ROM_REGION(32, REGION_PROMS, 0 )
	ROM_LOAD( "bp15", 0, 32,  0x30d3acce)
ROM_END

ROM_START( liberate )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
 	ROM_LOAD( "bt12-2.bin", 0x8000, 0x4000, 0xa0079ffd )
	ROM_RELOAD(             0x0000, 0x4000 )
 	ROM_LOAD( "bt13-2.bin", 0xc000, 0x4000, 0x19f8485c )

	ROM_REGION( 0x10000 * 2, REGION_CPU2, 0 )
	ROM_LOAD( "bt11.bin",  0xe000, 0x2000,  0xb549ccaa )

	ROM_REGION( 0x12000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bt04.bin", 0x00000, 0x4000, 0x96e48d72 )	/* Chars/Sprites */
	ROM_LOAD( "bt03.bin", 0x04000, 0x2000, 0x29ad1b59 )
	ROM_LOAD( "bt06.bin", 0x06000, 0x4000, 0x7bed1497 )
	ROM_LOAD( "bt05.bin", 0x0a000, 0x2000, 0xa8896c20 )
	ROM_LOAD( "bt08.bin", 0x0c000, 0x4000, 0x828ef78d )
	ROM_LOAD( "bt07.bin", 0x10000, 0x2000, 0xf919e8e2 )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
 	ROM_LOAD( "bt02.bin", 0x0000, 0x4000, 0x7169f7bb )
 	ROM_LOAD( "bt00.bin", 0x4000, 0x2000, 0xb744454d )
	/* On early revision bt02 is split as BT01-A (0x2000) BT02-A (0x2000) */

	ROM_REGION(0x4000, REGION_USER1, 0 )
	ROM_LOAD( "bt10.bin",  0x0000, 0x4000,  0xee335397 )

	ROM_REGION( 32, REGION_PROMS, 0 )
	ROM_LOAD( "bt14.bin", 0x0000, 32,  0x20281d61 )
ROM_END

ROM_START( liberatb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "liber6.c17", 0x8000, 0x2000, 0xc1811fe0 )
	ROM_RELOAD(             0x0000, 0x2000)
	ROM_LOAD( "liber4.c18", 0xa000, 0x2000, 0x0e8db1ce )
	ROM_RELOAD(             0x2000, 0x2000)
	ROM_LOAD( "liber3.c20", 0xc000, 0x2000, 0x16c423f3 )
 	ROM_LOAD( "liber5.c19", 0xe000, 0x2000, 0x7738c194 )

	ROM_REGION( 0x10000 * 2, REGION_CPU2, 0 )
	ROM_LOAD( "bt11.bin",  0xe000, 0x2000,  0xb549ccaa )

	ROM_REGION( 0x12000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bt04.bin", 0x00000, 0x4000, 0x96e48d72 )	/* Chars/Sprites */
	ROM_LOAD( "bt03.bin", 0x04000, 0x2000, 0x29ad1b59 )
	ROM_LOAD( "bt06.bin", 0x06000, 0x4000, 0x7bed1497 )
	ROM_LOAD( "bt05.bin", 0x0a000, 0x2000, 0xa8896c20 )
	ROM_LOAD( "bt08.bin", 0x0c000, 0x4000, 0x828ef78d )
	ROM_LOAD( "bt07.bin", 0x10000, 0x2000, 0xf919e8e2 )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
 	ROM_LOAD( "bt02.bin", 0x0000, 0x4000, 0x7169f7bb )
 	ROM_LOAD( "bt00.bin", 0x4000, 0x2000, 0xb744454d )

	ROM_REGION(0x4000, REGION_USER1, 0 )
	ROM_LOAD( "bt10.bin",  0x0000, 0x4000,  0xee335397 )

	ROM_REGION( 32, REGION_PROMS, 0 )
	ROM_LOAD( "bt14.bin", 0x0000, 32,  0x20281d61 )
ROM_END

/***************************************************************************/

static void sound_cpu_decrypt(void)
{
	data8_t *RAM = memory_region(REGION_CPU2);
	int i;

	/* Bit swapping on sound cpu - Opcodes only */
	RAM = memory_region(REGION_CPU2);
	for (i=0xc000; i<0x10000; i++)
		RAM[i+0x10000]=((RAM[i] & 0x20) << 1) | ((RAM[i] & 0x40) >> 1) | (RAM[i] & 0x9f);

	memory_set_opcode_base(1,RAM+0x10000);
}

static DRIVER_INIT( prosport )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int i;

	/* Main cpu has the nibbles swapped */
	for (i=0; i<0x10000; i++)
		RAM[i]=((RAM[i] & 0x0f) << 4) | ((RAM[i] & 0xf0) >> 4);

	sound_cpu_decrypt();
}

static DRIVER_INIT( liberate )
{
	int A;
	unsigned char *ROM = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;

	memory_set_opcode_base(0,ROM+diff);

	/* Swap bits for opcodes only, not data */
	for (A = 0;A < diff;A++) {
		ROM[A+diff] = (ROM[A] & 0xd7) | ((ROM[A] & 0x08) << 2) | ((ROM[A] & 0x20) >> 2);
		ROM[A+diff] = (ROM[A+diff] & 0xbb) | ((ROM[A+diff] & 0x04) << 4) | ((ROM[A+diff] & 0x40) >> 4);
		ROM[A+diff] = (ROM[A+diff] & 0x7d) | ((ROM[A+diff] & 0x02) << 6) | ((ROM[A+diff] & 0x80) >> 6);
	}

	sound_cpu_decrypt();
}

/***************************************************************************/

GAMEX(1983, prosoccr, 0,        prosoccr,  liberate, prosport, ROT270, "Data East Corporation", "Pro Soccer", GAME_NOT_WORKING )
GAMEX(1983, prosport, 0,        prosport,  liberate, prosport, ROT270, "Data East Corporation", "Prosport", GAME_NOT_WORKING )
GAME( 1983, boomrang, 0,        boomrang,  boomrang, prosport, ROT270, "Data East Corporation", "Boomer Rang'r / Genesis" )
GAME( 1984, kamikcab, 0,        boomrang,  kamikcab, prosport, ROT270, "Data East Corporation", "Kamikaze Cabbie" )
GAME( 1984, liberate, 0,        liberate,  liberate, liberate, ROT270, "Data East Corporation", "Liberation" )
GAMEX(1984, liberatb, liberate, liberatb,  liberate, prosport, ROT270, "bootleg",               "Liberation (bootleg)", GAME_NOT_WORKING )
