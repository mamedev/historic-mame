/***************************************************************************

Double Dragon     (c) 1987 Technos Japan
Double Dragon II  (c) 1988 Technos Japan

Driver by Carlos A. Lozano, Rob Rosenbrock, Phil Stroffolino, Ernesto Corvi

Toffy / Super Toffy added by David Haywood
Thanks to Bryan McPhail for spotting the Toffy program rom encryption
Toffy / Super Toffy sound hooked up by R. Belmont.

todo:

banking in Toffy / Super toffy
make Dangerous Dungeons work
DIPS etc. in Toffy / Super Toffy

-- Read Me --

Super Toffy - Unico 1994

Main cpu: 	MC6809EP
Sound cpu: 	MC6809P
Sound: 		YM2151
Clocks:		12 MHz, 3.579MHz

Graphics custom: MDE-2001

-- --

Does this make Super Toffy the sequel to a rip-off / bootleg of a
conversion kit which could be applied to a bootleg double dragon :-p?

***************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"

/* from vidhrdw */
extern unsigned char *ddragon_bgvideoram,*ddragon_fgvideoram;
extern int ddragon_scrollx_hi, ddragon_scrolly_hi;
extern unsigned char *ddragon_scrollx_lo;
extern unsigned char *ddragon_scrolly_lo;
VIDEO_START( ddragon );
VIDEO_UPDATE( ddragon );
WRITE_HANDLER( ddragon_bgvideoram_w );
WRITE_HANDLER( ddragon_fgvideoram_w );
extern unsigned char *ddragon_spriteram;
extern int technos_video_hw;
/* end of extern code & data */

/* private globals */
static int dd_sub_cpu_busy;
static int sprite_irq, sound_irq, ym_irq, snd_cpu;
static int adpcm_pos[2],adpcm_end[2],adpcm_idle[2];
/* end of private globals */

static MACHINE_INIT( ddragon )
{
	sprite_irq = IRQ_LINE_NMI;
	sound_irq = M6809_IRQ_LINE;
	ym_irq = M6809_FIRQ_LINE;
	technos_video_hw = 0;
	dd_sub_cpu_busy = 0x10;
	adpcm_idle[0] = adpcm_idle[1] = 1;
	snd_cpu = 2;
}

static MACHINE_INIT( toffy )
{
	sound_irq = M6809_IRQ_LINE;
	ym_irq = M6809_FIRQ_LINE;
	technos_video_hw = 0;
	dd_sub_cpu_busy = 0x10;
	adpcm_idle[0] = adpcm_idle[1] = 1;
	snd_cpu = 1;
}

static MACHINE_INIT( ddragonb )
{
	sprite_irq = IRQ_LINE_NMI;
	sound_irq = M6809_IRQ_LINE;
	ym_irq = M6809_FIRQ_LINE;
	technos_video_hw = 0;
	dd_sub_cpu_busy = 0x10;
	adpcm_idle[0] = adpcm_idle[1] = 1;
	snd_cpu = 2;
}

static MACHINE_INIT( ddragon2 )
{
	sprite_irq = IRQ_LINE_NMI;
	sound_irq = IRQ_LINE_NMI;
	ym_irq = 0;
	technos_video_hw = 2;
	dd_sub_cpu_busy = 0x10;
	snd_cpu = 2;
}

static WRITE_HANDLER( ddragon_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	ddragon_scrolly_hi = ( ( data & 0x02 ) << 7 );
	ddragon_scrollx_hi = ( ( data & 0x01 ) << 8 );

	flip_screen_set(~data & 0x04);

	/* bit 3 unknown */

	if (data & 0x10)
		dd_sub_cpu_busy = 0x00;
	else if (dd_sub_cpu_busy == 0x00)
		cpu_set_irq_line( 1, sprite_irq, (sprite_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );

	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( ( data & 0xe0) >> 5 ) ) ] );
}

static WRITE_HANDLER( toffy_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	ddragon_scrolly_hi = ( ( data & 0x02 ) << 7 );
	ddragon_scrollx_hi = ( ( data & 0x01 ) << 8 );

//	flip_screen_set(~data & 0x04);

	/* bit 3 unknown */

//	if (data & 0x10)
//		dd_sub_cpu_busy = 0x00;
//	else if (dd_sub_cpu_busy == 0x00)
//		cpu_set_irq_line( 1, sprite_irq, (sprite_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );

	/* I don't know ... */
	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( ( data & 0x20) >> 5 ) ) ] );
}

static WRITE_HANDLER( ddragon_forcedIRQ_w )
{
	cpu_set_irq_line( 0, M6809_IRQ_LINE, HOLD_LINE );
}

static READ_HANDLER( port4_r )
{
	int port = readinputport( 4 );

	return port | dd_sub_cpu_busy;
}

static READ_HANDLER( ddragon_spriteram_r )
{
	return ddragon_spriteram[offset];
}

static WRITE_HANDLER( ddragon_spriteram_w )
{
	if ( cpu_getactivecpu() == 1 && offset == 0 )
		dd_sub_cpu_busy = 0x10;

	ddragon_spriteram[offset] = data;
}

static WRITE_HANDLER( cpu_sound_command_w )
{
	soundlatch_w( offset, data );
	cpu_set_irq_line( snd_cpu, sound_irq, (sound_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );
}

static WRITE_HANDLER( dd_adpcm_w )
{
	int chip = offset & 1;

	switch (offset/2)
	{
		case 3:
			adpcm_idle[chip] = 1;
			MSM5205_reset_w(chip,1);
			break;

		case 2:
			adpcm_pos[chip] = (data & 0x7f) * 0x200;
			break;

		case 1:
			adpcm_end[chip] = (data & 0x7f) * 0x200;
			break;

		case 0:
			adpcm_idle[chip] = 0;
			MSM5205_reset_w(chip,0);
			break;
	}
}

static void dd_adpcm_int(int chip)
{
	static int adpcm_data[2] = { -1, -1 };

	if (adpcm_pos[chip] >= adpcm_end[chip] || adpcm_pos[chip] >= 0x10000)
	{
		adpcm_idle[chip] = 1;
		MSM5205_reset_w(chip,1);
	}
	else if (adpcm_data[chip] != -1)
	{
		MSM5205_data_w(chip,adpcm_data[chip] & 0x0f);
		adpcm_data[chip] = -1;
	}
	else
	{
		unsigned char *ROM = memory_region(REGION_SOUND1) + 0x10000 * chip;

		adpcm_data[chip] = ROM[adpcm_pos[chip]++];
		MSM5205_data_w(chip,adpcm_data[chip] >> 4);
	}
}

static READ_HANDLER( dd_adpcm_status_r )
{
	return adpcm_idle[0] + (adpcm_idle[1] << 1);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, ddragon_spriteram_r },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r },
	{ 0x3801, 0x3801, input_port_1_r },
	{ 0x3802, 0x3802, port4_r },
	{ 0x3803, 0x3803, input_port_2_r },
	{ 0x3804, 0x3804, input_port_3_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x11ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x1200, 0x13ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x1400, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2000, 0x2fff, ddragon_spriteram_w, &ddragon_spriteram },
	{ 0x3000, 0x37ff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3808, 0x3808, ddragon_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x380b, 0x380d, MWA_RAM },	/* ??? */
	{ 0x380e, 0x380e, cpu_sound_command_w },
	{ 0x380f, 0x380f, ddragon_forcedIRQ_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( dd2_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, ddragon_spriteram_r },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r },
	{ 0x3801, 0x3801, input_port_1_r },
	{ 0x3802, 0x3802, port4_r },
	{ 0x3803, 0x3803, input_port_2_r },
	{ 0x3804, 0x3804, input_port_3_r },
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( dd2_writemem )
	{ 0x0000, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2000, 0x2fff, ddragon_spriteram_w, &ddragon_spriteram },
	{ 0x3000, 0x37ff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3808, 0x3808, ddragon_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x380b, 0x380d, MWA_RAM },	/* ??? */
	{ 0x380e, 0x380e, cpu_sound_command_w },
	{ 0x380f, 0x380f, ddragon_forcedIRQ_w },
	{ 0x3c00, 0x3dff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3e00, 0x3fff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_WRITE_START( toffy_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x11ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x1200, 0x13ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x1400, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2000, 0x2fff, ddragon_spriteram_w, &ddragon_spriteram },
	{ 0x3000, 0x37ff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3808, 0x3808, toffy_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x380b, 0x380d, MWA_RAM },	/* ??? */
	{ 0x380e, 0x380e, cpu_sound_command_w },
	{ 0x380f, 0x380f, ddragon_forcedIRQ_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sub_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x8000, 0x8fff, ddragon_spriteram_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x8000, 0x8fff, ddragon_spriteram_w },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x1800, 0x1800, dd_adpcm_status_r },
	{ 0x2800, 0x2801, YM2151_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x2800, 0x2800, YM2151_register_port_0_w },
	{ 0x2801, 0x2801, YM2151_data_port_0_w },
	{ 0x3800, 0x3807, dd_adpcm_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( dd2_sub_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, ddragon_spriteram_r },
	{ 0xd000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( dd2_sub_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, ddragon_spriteram_w },
	{ 0xd000, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( dd2_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xA000, 0xA000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( dd2_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

#define COMMON_PORT4	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* verified to be active high (palette fades in ddragon2) */ \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* sub cpu busy */ \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define COMMON_INPUT_PORTS PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_START \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) ) \
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) ) \
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) ) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )


INPUT_PORTS_START( ddragon )
	COMMON_INPUT_PORTS

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20k" )
	PORT_DIPSETTING(    0x00, "40k" )
	PORT_DIPSETTING(    0x30, "30k and every 60k" )
	PORT_DIPSETTING(    0x20, "40k and every 80k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )

	COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( ddragon2 )
	COMMON_INPUT_PORTS

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Hurricane Kick" )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPNAME( 0x30, 0x30, "Timer" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "65" )
	PORT_DIPSETTING(    0x30, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "4" )

	COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( toffy )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0f, "4 Coin/6 Credits" )
	PORT_DIPSETTING(    0x0a, "3 Coin/5 Credits" )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0e, "3 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0xf0, "4 Coin/6 Credits" )
	PORT_DIPSETTING(    0xa0, "3 Coin/5 Credits" )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xe0, "3 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "30k, 50k and 100k" )
	PORT_DIPSETTING(    0x08, "50k and 100k" )
	PORT_DIPSETTING(    0x18, "100k and 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	COMMON_PORT4
INPUT_PORTS_END

#undef COMMON_INPUT_PORTS
#undef COMMON_PORT4



static struct GfxLayout char_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 2, 4, 6 },
	{ 1, 0, 8*8+1, 8*8+0, 16*8+1, 16*8+0, 24*8+1, 24*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		  32*8+3, 32*8+2, 32*8+1, 32*8+0, 48*8+3, 48*8+2, 48*8+1, 48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout,   0, 8 },	/* colors   0-127 */
	{ REGION_GFX2, 0, &tile_layout, 128, 8 },	/* colors 128-255 */
	{ REGION_GFX3, 0, &tile_layout, 256, 8 },	/* colors 256-383 */
	{ -1 }
};



static void irq_handler(int irq)
{
	cpu_set_irq_line( snd_cpu, ym_irq , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* ??? */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ irq_handler }
};

static struct MSM5205interface msm5205_interface =
{
	2,					/* 2 chips             */
	384000,				/* 384KHz             */
	{ dd_adpcm_int, dd_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B, MSM5205_S48_4B },	/* 8kHz */
	{ 40, 40 }				/* volume */
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 8000 },           /* frequency (Hz) */
	{ REGION_SOUND1 },  /* memory region */
	{ 15 }
};

static INTERRUPT_GEN( ddragon_interrupt )
{
    cpu_set_irq_line(0, 1, HOLD_LINE); /* hold the FIRQ line */
    cpu_set_nmi_line(0, PULSE_LINE); /* pulse the NMI line */
}



static MACHINE_DRIVER_START( ddragon )

	/* basic machine hardware */
 	MDRV_CPU_ADD(HD6309, 3579545)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,1)

 	MDRV_CPU_ADD(HD63701, 2000000) /* 2 MHz ???*/
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

 	MDRV_CPU_ADD(HD6309, 3579545)
 	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(ddragon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ddragonb )

	/* basic machine hardware */
 	MDRV_CPU_ADD(HD6309, 3579545)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,1)

 	MDRV_CPU_ADD(HD6309, 12000000 / 3) /* 4 MHz */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

 	MDRV_CPU_ADD(HD6309, 3579545)
 	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(ddragonb)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ddragon2 )

	/* basic machine hardware */
 	MDRV_CPU_ADD(HD6309, 3579545)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(dd2_readmem,dd2_writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,1)

	MDRV_CPU_ADD(Z80,12000000 / 3) /* 4 MHz */
	MDRV_CPU_MEMORY(dd2_sub_readmem,dd2_sub_writemem)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(dd2_sound_readmem,dd2_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(ddragon2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( toffy )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,3579545) // 12 MHz / 2 or 3.579545 ?
	MDRV_CPU_MEMORY(readmem,toffy_writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,1)

	MDRV_CPU_ADD(M6809, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	MDRV_MACHINE_INIT(toffy)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ddragon )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "21j-1-5",      0x08000, 0x08000, 0x42045dfd )
	ROM_LOAD( "21j-2-3",      0x10000, 0x08000, 0x5779705e ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21j-3",        0x18000, 0x08000, 0x3bdea613 ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21j-4-1",      0x20000, 0x08000, 0x728f87b9 ) /* banked at 0x4000-0x8000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, 0xf5232d03 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, 0x9efa95bb )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-5",        0x00000, 0x08000, 0x7a8b8db4 )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-a",        0x00000, 0x10000, 0x574face3 )	/* sprites */
	ROM_LOAD( "21j-b",        0x10000, 0x10000, 0x40507a76 )
	ROM_LOAD( "21j-c",        0x20000, 0x10000, 0xbb0bc76f )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, 0xcb4f231b )
	ROM_LOAD( "21j-e",        0x40000, 0x10000, 0xa0a0c261 )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, 0x6ba152f6 )
	ROM_LOAD( "21j-g",        0x60000, 0x10000, 0x3220a0b6 )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, 0x65c7517d )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-8",        0x00000, 0x10000, 0x7c435887 )	/* tiles */
	ROM_LOAD( "21j-9",        0x10000, 0x10000, 0xc6640aed )
	ROM_LOAD( "21j-i",        0x20000, 0x10000, 0x5effb0a0 )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, 0x5fb42e7c )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, 0x34755de3 )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, 0x904de6f8 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, 0xfdb130a9 )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, 0x46339529 )	/* unknown */
ROM_END

ROM_START( ddragonu )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "21a-1-5",      0x08000, 0x08000, 0xe24a6e11 )
	ROM_LOAD( "21j-2-3",      0x10000, 0x08000, 0x5779705e ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21a-3",        0x18000, 0x08000, 0xdbf24897 ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21a-4",        0x20000, 0x08000, 0x6ea16072 ) /* banked at 0x4000-0x8000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, 0xf5232d03 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, 0x9efa95bb )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-5",        0x00000, 0x08000, 0x7a8b8db4 )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-a",        0x00000, 0x10000, 0x574face3 )	/* sprites */
	ROM_LOAD( "21j-b",        0x10000, 0x10000, 0x40507a76 )
	ROM_LOAD( "21j-c",        0x20000, 0x10000, 0xbb0bc76f )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, 0xcb4f231b )
	ROM_LOAD( "21j-e",        0x40000, 0x10000, 0xa0a0c261 )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, 0x6ba152f6 )
	ROM_LOAD( "21j-g",        0x60000, 0x10000, 0x3220a0b6 )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, 0x65c7517d )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-8",        0x00000, 0x10000, 0x7c435887 )	/* tiles */
	ROM_LOAD( "21j-9",        0x10000, 0x10000, 0xc6640aed )
	ROM_LOAD( "21j-i",        0x20000, 0x10000, 0x5effb0a0 )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, 0x5fb42e7c )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, 0x34755de3 )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, 0x904de6f8 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, 0xfdb130a9 )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, 0x46339529 )	/* unknown */
ROM_END

ROM_START( ddragonb )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "ic26",         0x08000, 0x08000, 0xae714964 )
	ROM_LOAD( "21j-2-3",      0x10000, 0x08000, 0x5779705e ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21a-3",        0x18000, 0x08000, 0xdbf24897 ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "ic23",         0x20000, 0x08000, 0x6c9f46fa ) /* banked at 0x4000-0x8000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "ic38",         0x0c000, 0x04000, 0x6a6a0325 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, 0x9efa95bb )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-5",        0x00000, 0x08000, 0x7a8b8db4 )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-a",        0x00000, 0x10000, 0x574face3 )	/* sprites */
	ROM_LOAD( "21j-b",        0x10000, 0x10000, 0x40507a76 )
	ROM_LOAD( "21j-c",        0x20000, 0x10000, 0xbb0bc76f )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, 0xcb4f231b )
	ROM_LOAD( "21j-e",        0x40000, 0x10000, 0xa0a0c261 )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, 0x6ba152f6 )
	ROM_LOAD( "21j-g",        0x60000, 0x10000, 0x3220a0b6 )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, 0x65c7517d )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-8",        0x00000, 0x10000, 0x7c435887 )	/* tiles */
	ROM_LOAD( "21j-9",        0x10000, 0x10000, 0xc6640aed )
	ROM_LOAD( "21j-i",        0x20000, 0x10000, 0x5effb0a0 )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, 0x5fb42e7c )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, 0x34755de3 )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, 0x904de6f8 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, 0xfdb130a9 )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, 0x46339529 )	/* unknown */
ROM_END

ROM_START( ddragon2 )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "26a9-04.bin",  0x08000, 0x8000, 0xf2cfc649 )
	ROM_LOAD( "26aa-03.bin",  0x10000, 0x8000, 0x44dd5d4b )
	ROM_LOAD( "26ab-0.bin",   0x18000, 0x8000, 0x49ddddcd )
	ROM_LOAD( "26ac-0e.63",   0x20000, 0x8000, 0x57acad2c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite CPU 64kb (Upper 16kb = 0) */
	ROM_LOAD( "26ae-0.bin",   0x00000, 0x10000, 0xea437867 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* music CPU, 64kb */
	ROM_LOAD( "26ad-0.bin",   0x00000, 0x8000, 0x75e36cd6 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "26a8-0e.19",   0x00000, 0x10000, 0x4e80cd36 )	/* chars */

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "26j0-0.bin",   0x00000, 0x20000, 0xdb309c84 )	/* sprites */
	ROM_LOAD( "26j1-0.bin",   0x20000, 0x20000, 0xc3081e0c )
	ROM_LOAD( "26af-0.bin",   0x40000, 0x20000, 0x3a615aad )
	ROM_LOAD( "26j2-0.bin",   0x60000, 0x20000, 0x589564ae )
	ROM_LOAD( "26j3-0.bin",   0x80000, 0x20000, 0xdaf040d6 )
	ROM_LOAD( "26a10-0.bin",  0xa0000, 0x20000, 0x6d16d889 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "26j4-0.bin",   0x00000, 0x20000, 0xa8c93e76 )	/* tiles */
	ROM_LOAD( "26j5-0.bin",   0x20000, 0x20000, 0xee555237 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "26j6-0.bin",   0x00000, 0x20000, 0xa84b2a29 )
	ROM_LOAD( "26j7-0.bin",   0x20000, 0x20000, 0xbc6a48d5 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "prom.16",      0x0000, 0x0200, 0x46339529 )	/* unknown (same as ddragon) */
ROM_END

ROM_START( ddragn2u )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "26a9-04.bin",  0x08000, 0x8000, 0xf2cfc649 )
	ROM_LOAD( "26aa-03.bin",  0x10000, 0x8000, 0x44dd5d4b )
	ROM_LOAD( "26ab-0.bin",   0x18000, 0x8000, 0x49ddddcd )
	ROM_LOAD( "26ac-02.bin",  0x20000, 0x8000, 0x097eaf26 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite CPU 64kb (Upper 16kb = 0) */
	ROM_LOAD( "26ae-0.bin",   0x00000, 0x10000, 0xea437867 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* music CPU, 64kb */
	ROM_LOAD( "26ad-0.bin",   0x00000, 0x8000, 0x75e36cd6 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "26a8-0.bin",   0x00000, 0x10000, 0x3ad1049c )	/* chars */

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "26j0-0.bin",   0x00000, 0x20000, 0xdb309c84 )	/* sprites */
	ROM_LOAD( "26j1-0.bin",   0x20000, 0x20000, 0xc3081e0c )
	ROM_LOAD( "26af-0.bin",   0x40000, 0x20000, 0x3a615aad )
	ROM_LOAD( "26j2-0.bin",   0x60000, 0x20000, 0x589564ae )
	ROM_LOAD( "26j3-0.bin",   0x80000, 0x20000, 0xdaf040d6 )
	ROM_LOAD( "26a10-0.bin",  0xa0000, 0x20000, 0x6d16d889 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "26j4-0.bin",   0x00000, 0x20000, 0xa8c93e76 )	/* tiles */
	ROM_LOAD( "26j5-0.bin",   0x20000, 0x20000, 0xee555237 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "26j6-0.bin",   0x00000, 0x20000, 0xa84b2a29 )
	ROM_LOAD( "26j7-0.bin",   0x20000, 0x20000, 0xbc6a48d5 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "prom.16",      0x0000, 0x0200, 0x46339529 )	/* unknown (same as ddragon) */
ROM_END

ROM_START( toffy )
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) /* Main CPU? */
	ROM_LOAD( "2-27512.rom", 0x00000, 0x10000, 0x244709dd )
	ROM_RELOAD( 0x10000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU? */
	ROM_LOAD( "u142.1", 0x00000, 0x10000, 0x541bd7f0 )

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "7-27512.rom", 0x000, 0x10000, 0xf9e8ec64 )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* GFX */
	/* the same as 'Dangerous Dungeons' once decrypted */
	ROM_LOAD( "4-27512.rom", 0x00000, 0x10000, 0x94b5ef6f )
	ROM_LOAD( "3-27512.rom", 0x10000, 0x10000, 0xa7a053a3 )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* GFX */
	ROM_LOAD( "6-27512.rom", 0x00000, 0x10000, 0x2ba7ca47 )
	ROM_LOAD( "5-27512.rom", 0x10000, 0x10000, 0x4f91eec6 )
ROM_END

ROM_START( stoffy )
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) /* Main CPU? */
	ROM_LOAD( "u70.2", 0x00000, 0x10000, 0x3c156610 )
	ROM_RELOAD( 0x10000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU? */
	ROM_LOAD( "u142.1", 0x00000, 0x10000, 0x541bd7f0 ) // same as 'toffy'

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "u35.7", 0x00000, 0x10000, 0x83735d25 )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* GFX */
	ROM_LOAD( "u78.4", 0x00000, 0x10000, 0x9743a74d ) // 0
	ROM_LOAD( "u77.3", 0x10000, 0x10000, 0xf267109a ) // 0

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* GFX */
	ROM_LOAD( "u80.5", 0x00000, 0x10000, 0xff190865 ) // 1 | should be u80.6 ?
	ROM_LOAD( "u79.5", 0x10000, 0x10000, 0x333d5b8a ) // 1
ROM_END

ROM_START( ddungeon )
	/* there were various double dragon roms in the zip but
	I'm not sure they're needed, there is also an undumped MCU */
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) /* Main CPU? */
	ROM_LOAD( "dd3.bin", 0x10000, 0x8000, 0x922e719c )
	ROM_LOAD( "dd2.bin", 0x08000, 0x8000, 0xa6e7f608 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU? */
	// I guess it uses ddragon's Sound Rom?

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "dd6.bin", 0x00000, 0x08000, 0x057588ca )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* GFX */
	ROM_LOAD( "dd-6b.bin", 0x00000, 0x08000, 0x3deacae9 ) // 0
	ROM_LOAD( "dd-7c.bin", 0x10000, 0x08000, 0x5a2f31eb ) // 0

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* GFX */
	ROM_LOAD( "dd-7r.bin", 0x00000, 0x08000, 0x50d6ab5d ) // 1
	ROM_LOAD( "dd-7k.bin", 0x10000, 0x08000, 0x43264ad8 ) // 1
ROM_END

/** INITS **
toffy / stoffy are 'encrytped

*/

static DRIVER_INIT( toffy )
{
	/* the program rom has a simple bitswap encryption */
	data8_t *rom=memory_region(REGION_CPU1);
	int i;

	for (i = 0;i < 0x20000;i++)
		rom[i] = BITSWAP8(rom[i] , 6,7,5,4,3,2,1,0);

	/* and the fg gfx ... */
	rom=memory_region(REGION_GFX1);

	for (i = 0;i < 0x10000;i++)
		rom[i] = BITSWAP8(rom[i] , 7,6,5,3,4,2,1,0);

	/* and the bg gfx */
	rom=memory_region(REGION_GFX3);

	for (i = 0;i < 0x10000;i++)
	{
		rom[i] = BITSWAP8(rom[i] , 7,6,1,4,3,2,5,0);
		rom[i+0x10000] = BITSWAP8(rom[i+0x10000] , 7,6,2,4,3,5,1,0);
	}

	/* and the sprites gfx */
	rom=memory_region(REGION_GFX2);

	for (i = 0;i < 0x20000;i++)
		rom[i] = BITSWAP8(rom[i] , 7,6,5,4,3,2,0,1);

	/* should the sound rom be bitswapped too? */

}

GAME( 1987, ddragon,  0,        ddragon,  ddragon,  0, ROT0, "Technos", "Double Dragon (Japan)" )
GAME( 1987, ddragonu, ddragon,  ddragon,  ddragon,  0, ROT0, "[Technos] (Taito America license)", "Double Dragon (US)" )
GAME( 1987, ddragonb, ddragon,  ddragonb, ddragon,  0, ROT0, "bootleg", "Double Dragon (bootleg)" )
GAME( 1988, ddragon2, 0,        ddragon2, ddragon2, 0, ROT0, "Technos", "Double Dragon II - The Revenge (World)" )
GAME( 1988, ddragn2u, ddragon2, ddragon2, ddragon2, 0, ROT0, "Technos", "Double Dragon II - The Revenge (US)" )

/* this was a conversion of a double dragon board */
GAMEX( 199?, ddungeon, 0, toffy, toffy, 0, ROT0, "East Coast Coin Company (Melbourne)", "Dangerous Dungeons", GAME_NOT_WORKING )

/* these run on their own board but are the basically the same game, Toffy even has 'dangerous dungeons' text in it */
GAME( 1993, toffy,  0, toffy, toffy, toffy, ROT0, "Midas",                 "Toffy" )
GAME( 1994, stoffy, 0, toffy, toffy, toffy, ROT0, "Midas (Unico license)", "Super Toffy" )
