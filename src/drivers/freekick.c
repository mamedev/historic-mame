/***************************************************************************

Free Kick  - (c) 1987 Sega / Nihon System (made by Nihon, licensed to Sega)

Driver by Tomasz Slanina  dox@space.pl
based on initial work made by David Haywood

Notes:
- Quite interestingly, Free Kick's sound ROM contains a Z80 program, but
  there isn't a sound CPU and that program isn't executed. Instead, the main
  CPU reads the sound program through an 8255 PPI and plays sounds directly.

TODO:
- Perfect Billiard dip switches
- Gigas cocktail mode / flipscreen

****************************************************************************

 currently only the freekick bootleg roms are included
 the fk bootleg roms are the same as one of the other sets + an extra
 64k ram dump from protection device.

 main program rom is unused, is it a dummy or just something to active the
 protection device?

 we don't have the game rom data / program from inside the counter run cpu,
 hopefully somebody can work out how to dump it before all the boards die
 since it appears to be battery backed

 ---

 $78d        - checksum calculations
 $d290,$d291 - level (color set , level number  - BCD)
 $d292       - lives

 To enter Test Mode - press Button1 durning RESET (code at $79d)

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"


extern data8_t *freek_videoram;

VIDEO_START(freekick);
VIDEO_UPDATE(gigas);
VIDEO_UPDATE(pbillrd);
VIDEO_UPDATE(freekick);
WRITE_HANDLER( freek_videoram_w );


static int romaddr;

static WRITE_HANDLER( snd_rom_addr_l_w )
{
	romaddr = (romaddr & 0xff00) | data;
}

static WRITE_HANDLER( snd_rom_addr_h_w )
{
	romaddr = (romaddr & 0x00ff) | (data << 8);
}

static READ_HANDLER( snd_rom_r )
{
	return memory_region(REGION_USER1)[romaddr & 0x7fff];
}

static ppi8255_interface ppi8255_intf =
{
	2, 										/* 1 chips */
	{ NULL,             input_port_0_r },	/* Port A read */
	{ NULL,             input_port_1_r },	/* Port B read */
	{ snd_rom_r,        input_port_2_r },	/* Port C read */
	{ snd_rom_addr_l_w, NULL },				/* Port A write */
	{ snd_rom_addr_h_w, NULL },				/* Port B write */
	{ NULL,             NULL },				/* Port C write */
};

MACHINE_INIT( freekckb )
{
	ppi8255_init(&ppi8255_intf);
}


static WRITE_HANDLER( flipscreen_w )
{
	/* flip Y/X could be the other way round... */
	if (offset)
		flip_screen_y_set(~data & 1);
	else
		flip_screen_x_set(~data & 1);
}

static WRITE_HANDLER( coin_w )
{
	coin_counter_w(offset,~data & 1);
}


static int spinner;

static WRITE_HANDLER( spinner_select_w )
{
	spinner = data & 1;
}

static READ_HANDLER( spinner_r )
{
	return readinputport(5 + spinner);
}

static READ_HANDLER( gigas_spinner_r )
{
	return readinputport( spinner );
}



static WRITE_HANDLER( pbillrd_bankswitch_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x10000 + 0x4000 * (data & 1));
}


static int nmi_en;

static WRITE_HANDLER( nmi_enable_w )
{
	nmi_en = data & 1;
}

static INTERRUPT_GEN( freekick_irqgen )
{
	if (nmi_en) cpu_set_irq_line(0,IRQ_LINE_NMI,PULSE_LINE);
}

static MEMORY_READ_START( gigas_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_2_r },
	{ 0xe800, 0xe800, input_port_3_r },
	{ 0xf000, 0xf000, input_port_4_r },
	{ 0xf800, 0xf800, input_port_5_r },
MEMORY_END

static MEMORY_WRITE_START( gigas_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, freek_videoram_w, &freek_videoram },
	{ 0xd800, 0xd8ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd900, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe001, MWA_NOP },// probably not flipscreen
	{ 0xe002, 0xe003, coin_w },
	{ 0xe004, 0xe004, nmi_enable_w },
	{ 0xe005, 0xe005, MWA_NOP},
	{ 0xf000, 0xf000, MWA_NOP }, //bankswitch ?
	{ 0xfc00, 0xfc00, SN76496_0_w },
	{ 0xfc01, 0xfc01, SN76496_1_w },
	{ 0xfc02, 0xfc02, SN76496_2_w },
	{ 0xfc03, 0xfc03, SN76496_3_w },
MEMORY_END

static PORT_READ_START( gigas_readport )
	{ 0x00, 0x00, gigas_spinner_r },
	{ 0x01, 0x01, MRA_NOP }, //unused dip 3
PORT_END

static PORT_WRITE_START( gigas_writeport )
	{ 0x00, 0x00 ,spinner_select_w },
PORT_END



static MEMORY_READ_START( pbillrd_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe800, 0xe800, input_port_1_r },
	{ 0xf000, 0xf000, input_port_2_r },
	{ 0xf800, 0xf800, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( pbillrd_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, freek_videoram_w, &freek_videoram },
	{ 0xd800, 0xd8ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd900, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe001, flipscreen_w },
	{ 0xe002, 0xe003, coin_w },
	{ 0xe004, 0xe004, nmi_enable_w },
	{ 0xf000, 0xf000, pbillrd_bankswitch_w },
	{ 0xfc00, 0xfc00, SN76496_0_w },
	{ 0xfc01, 0xfc01, SN76496_1_w },
	{ 0xfc02, 0xfc02, SN76496_2_w },
	{ 0xfc03, 0xfc03, SN76496_3_w },
MEMORY_END

static MEMORY_READ_START( freekckb_readmem )
	{ 0x0000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },	// tilemap
	{ 0xe800, 0xe8ff, MRA_RAM },	// sprites
	{ 0xec00, 0xec03, ppi8255_0_r },
	{ 0xf000, 0xf003, ppi8255_1_r },
	{ 0xf800, 0xf800, input_port_3_r },
	{ 0xf801, 0xf801, input_port_4_r },
	{ 0xf802, 0xf802, MRA_NOP },	//MUST return bit 0 = 0, otherwise game resets
	{ 0xf803, 0xf803, spinner_r },
MEMORY_END

static MEMORY_WRITE_START( freekckb_writemem )
	{ 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, freek_videoram_w, &freek_videoram },
	{ 0xe800, 0xe8ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xec00, 0xec03, ppi8255_0_w },
	{ 0xf000, 0xf003, ppi8255_1_w },
	{ 0xf800, 0xf801, flipscreen_w },
	{ 0xf802, 0xf803, coin_w },
	{ 0xf804, 0xf804, nmi_enable_w },
	{ 0xf806, 0xf806, spinner_select_w },
	{ 0xfc00, 0xfc00, SN76496_0_w },
	{ 0xfc01, 0xfc01, SN76496_1_w },
	{ 0xfc02, 0xfc02, SN76496_2_w },
	{ 0xfc03, 0xfc03, SN76496_3_w },
MEMORY_END


static int ff_data;

static READ_HANDLER (freekick_ff_r)
{
	return ff_data;
}

static WRITE_HANDLER (freekick_ff_w)
{
	ff_data = data;
}

static PORT_READ_START( freekckb_readport )
	{ 0xff, 0xff, freekick_ff_r },
PORT_END

static PORT_WRITE_START( freekckb_writeport )
	{ 0xff, 0xff, freekick_ff_w },
PORT_END

INPUT_PORTS_START( gigas )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 30, 15, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 30, 15, 0, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "20000, 60000, Every 60000" )
	PORT_DIPSETTING(    0x02, "20000, 60000" )
	PORT_DIPSETTING(    0x04, "30000, 90000, Every 90000" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )    /* level 1 */
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" ) /* level 4 */
  PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )



INPUT_PORTS_END

INPUT_PORTS_START( pbillrd )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
INPUT_PORTS_END

INPUT_PORTS_START( freekckb )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "2-3-4-5-60000 Points" )
	PORT_DIPSETTING(    0x02, "3-4-5-6-7-80000 Points" )
	PORT_DIPSETTING(    0x04, "20000 & 60000 Points" )
	PORT_DIPSETTING(    0x00, "ONLY 20000 Points" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )    /* level 1 */
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" ) /* level 4 */
    PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xc0, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/25 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/50 Credits" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Manufacturer" )
	PORT_DIPSETTING(    0x00, "Nihon System" )
	PORT_DIPSETTING(    0x01, "Sega/Nihon System" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 30, 15, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 30, 15, 0, 0 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0,1,2,3, 4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3),RGN_FRAC(2,3),RGN_FRAC(1,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	128+0,128+1,128+2,128+3,128+4,128+5,128+6,128+7
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8,12*8,13*8,14*8,15*8
	},
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000, 32 },
	{ REGION_GFX2, 0, &spritelayout, 0x100, 32 },
	{ -1 }
};



static struct SN76496interface sn76496_interface =
{
	4,	/* 4 chips */
	{ 12000000/4, 12000000/4, 12000000/4, 12000000/4 },	/* 3 MHz??? */
	{ 50, 50, 50, 50 }
};



static MACHINE_DRIVER_START( pbillrd )
	MDRV_CPU_ADD(Z80, 12000000/2)	/* 6 MHz? */
	MDRV_CPU_MEMORY(pbillrd_readmem,pbillrd_writemem)
	MDRV_CPU_PERIODIC_INT(irq0_line_pulse,60*3) //??
	MDRV_CPU_VBLANK_INT(freekick_irqgen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(freekick)
	MDRV_VIDEO_UPDATE(pbillrd)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( freekckb )
	MDRV_CPU_ADD(Z80, 12000000/2)	/* 6 MHz? */
	MDRV_CPU_MEMORY(freekckb_readmem,freekckb_writemem)
	MDRV_CPU_PORTS(freekckb_readport,freekckb_writeport)
	MDRV_CPU_PERIODIC_INT(irq0_line_pulse,60*3) //??
	MDRV_CPU_VBLANK_INT(freekick_irqgen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(freekckb)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(freekick)
	MDRV_VIDEO_UPDATE(freekick)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gigas )
	MDRV_CPU_ADD(Z80, 18432000/6)	//confirmed
	MDRV_CPU_MEMORY(gigas_readmem,gigas_writemem)
	MDRV_CPU_PORTS(gigas_readport,gigas_writeport)
	MDRV_CPU_PERIODIC_INT(irq0_line_pulse,60*3)
	MDRV_CPU_VBLANK_INT(freekick_irqgen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(freekick)
	MDRV_VIDEO_UPDATE(gigas)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END



/* roms */

ROM_START( pbillrd )
	ROM_REGION( 0x18000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "pb.18",       0x00000, 0x4000, 0x9e6275ac )
	ROM_LOAD( "pb.7",        0x04000, 0x4000, 0xdd438431 )
	ROM_CONTINUE(            0x10000, 0x4000 )
	ROM_LOAD( "pb.9",        0x14000, 0x4000, 0x089ce80a )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "pb.4",        0x000000, 0x04000, 0x2f4d4dd3 )
	ROM_LOAD( "pb.5",        0x004000, 0x04000, 0x9dfccbd3 )
	ROM_LOAD( "pb.6",        0x008000, 0x04000, 0xb5c3f6f6 )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "10619.3r",    0x000000, 0x02000, 0x3296b9d9 )
	ROM_LOAD( "10620.3n",    0x002000, 0x02000, 0xee76b079 )
	ROM_LOAD( "10621.3m",    0x004000, 0x02000, 0x3dca8e4b )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
    ROM_LOAD( "82s129.3a", 0x0000, 0x0100, 0x44802169 )
	ROM_LOAD( "82s129.4d", 0x0100, 0x0100, 0x69ca07cc )
	ROM_LOAD( "82s129.4a", 0x0200, 0x0100, 0x145f950a )
	ROM_LOAD( "82s129.3d", 0x0300, 0x0100, 0x43d24e17 )
	ROM_LOAD( "82s129.3b", 0x0400, 0x0100, 0x7fdc872c )
	ROM_LOAD( "82s129.3c", 0x0500, 0x0100, 0xcc1657e5 )
ROM_END

ROM_START( pbillrds )
	ROM_REGION( 0x18000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "10627.10n",   0x00000, 0x4000, 0x2335e6dd )	/* encrypted */
	ROM_LOAD( "10625.8r",    0x04000, 0x4000, 0x8977c724 )	/* encrypted */
	ROM_CONTINUE(            0x10000, 0x4000 )
	ROM_LOAD( "10626.8n",    0x14000, 0x4000, 0x51d725e6 )	/* encrypted */

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "10622.3h",    0x000000, 0x04000, 0x23b864ac )
	ROM_LOAD( "10623.3h",    0x004000, 0x04000, 0x3dbfb790 )
	ROM_LOAD( "10624.3g",    0x008000, 0x04000, 0xb80032a9 )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "10619.3r",    0x000000, 0x02000, 0x3296b9d9 )
	ROM_LOAD( "10620.3n",    0x002000, 0x02000, 0xee76b079 )
	ROM_LOAD( "10621.3m",    0x004000, 0x02000, 0x3dca8e4b )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
    ROM_LOAD( "82s129.3a", 0x0000, 0x0100, 0x44802169 )
	ROM_LOAD( "82s129.4d", 0x0100, 0x0100, 0x69ca07cc )
	ROM_LOAD( "82s129.4a", 0x0200, 0x0100, 0x145f950a )
	ROM_LOAD( "82s129.3d", 0x0300, 0x0100, 0x43d24e17 )
	ROM_LOAD( "82s129.3b", 0x0400, 0x0100, 0x7fdc872c )
	ROM_LOAD( "82s129.3c", 0x0500, 0x0100, 0xcc1657e5 )
ROM_END

/*

original sets don't work, they're missing the main cpu code which is probably inside
the custom cpu, battery backed too so hopefully somebody can work out how to get it
from an original counter run board before they all die :-(

*/

ROM_START( freekick )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 Code */
	// Custom (NS6201-A 1987.9)
	ROM_LOAD( "freekck.cpu", 0x00000, 0x10000, 0x00000000 ) // missing, might be the same as the bootleg but not confirmed

	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "11.1e",       0x00000, 0x08000, 0xa6030ba9 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "12.1h",       0x000000, 0x04000, 0xfb82e486 )
	ROM_LOAD( "13.1j",       0x004000, 0x04000, 0x3ad78ee2 )
	ROM_LOAD( "14.1l",       0x008000, 0x04000, 0x0185695f )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "15.1m",       0x000000, 0x04000, 0x0fa7c13c )
	ROM_LOAD( "16.1p",       0x004000, 0x04000, 0x2b996e89 )
	ROM_LOAD( "17.1r",       0x008000, 0x04000, 0xe7894def )

	ROM_REGION( 0x0600, REGION_PROMS, 0 ) /* use the Japanese set proms, others seemed bad */
    ROM_LOAD( "8j.bpr",    0x0000, 0x0100, 0x53a6bc21 )
	ROM_LOAD( "7j.bpr",    0x0100, 0x0100, 0x38dd97d8 )
	ROM_LOAD( "8k.bpr",    0x0200, 0x0100, 0x18e66087 )
	ROM_LOAD( "7k.bpr",    0x0300, 0x0100, 0xbc21797a )
	ROM_LOAD( "8h.bpr",    0x0400, 0x0100, 0x8aac5fd0 )
	ROM_LOAD( "7h.bpr",    0x0500, 0x0100, 0xa507f941 )
ROM_END

ROM_START( freekckb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "freekbl8.q7", 0x00000, 0x10000, 0x4208cfe5 ) // this was on the bootleg, would normally be battery backed inside cpu?

	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "11.1e",       0x00000, 0x08000, 0xa6030ba9 )		// freekbl1.e2

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "12.1h",       0x000000, 0x04000, 0xfb82e486 )	// freekbl2.f2
	ROM_LOAD( "13.1j",       0x004000, 0x04000, 0x3ad78ee2 )	// freekbl3.j2
	ROM_LOAD( "14.1l",       0x008000, 0x04000, 0x0185695f )	// freekbl4.k2

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "15.1m",       0x000000, 0x04000, 0x0fa7c13c )	// freekbl5.m2
	ROM_LOAD( "16.1p",       0x004000, 0x04000, 0x2b996e89 )	// freekbl6.n2
	ROM_LOAD( "17.1r",       0x008000, 0x04000, 0xe7894def )	// freekbl7.r2

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
    ROM_LOAD( "8j.bpr",    0x0000, 0x0100, 0x53a6bc21 )
	ROM_LOAD( "7j.bpr",    0x0100, 0x0100, 0x38dd97d8 )
	ROM_LOAD( "8k.bpr",    0x0200, 0x0100, 0x18e66087 )
	ROM_LOAD( "7k.bpr",    0x0300, 0x0100, 0xbc21797a )
	ROM_LOAD( "8h.bpr",    0x0400, 0x0100, 0x8aac5fd0 )
	ROM_LOAD( "7h.bpr",    0x0500, 0x0100, 0xa507f941 )
ROM_END

ROM_START( countrun )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 Code */
	//  Custom CPU (pack) No. NS6201-A 1988.3 COUNTER RUN
	ROM_LOAD( "countrun.cpu", 0x00000, 0x10000, 0x00000000 ) // missing

	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "c-run.e1", 0x00000, 0x08000, 0x2c3b6f8f )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.h1", 0x000000, 0x04000, 0x3385b7b5 ) // rom 2
	ROM_LOAD( "c-run.j1", 0x004000, 0x04000, 0x58dc148d ) // rom 3
	ROM_LOAD( "c-run.l1", 0x008000, 0x04000, 0x3201f1e9 ) // rom 4

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.m1", 0x000000, 0x04000, 0x1efab3b4 ) // rom 5
	ROM_LOAD( "c-run.p1", 0x004000, 0x04000, 0xd0bf8d42 ) // rom 6
	ROM_LOAD( "c-run.r1", 0x008000, 0x04000, 0x4bb4a3e3 ) // rom 7

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "prom5.bpr",    0x0000, 0x0100, 0x63c114ad )
	ROM_LOAD( "prom2.bpr",    0x0100, 0x0100, 0xd16f95cc )
	ROM_LOAD( "prom4.bpr",    0x0200, 0x0100, 0x217db2c1 )
	ROM_LOAD( "prom1.bpr",    0x0300, 0x0100, 0x8d983949 )
	ROM_LOAD( "prom6.bpr",    0x0400, 0x0100, 0x33e87550 )
	ROM_LOAD( "prom3.bpr",    0x0500, 0x0100, 0xc77d0077 )
ROM_END

ROM_START( gigasm2b )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
	ROM_LOAD( "8.rom", 0x10000, 0x4000, 0xc00a4a6c )
	ROM_CONTINUE(      0x00000, 0x4000 )
	ROM_LOAD( "7.rom", 0x14000, 0x4000, 0x92bd9045 )
	ROM_CONTINUE(      0x04000, 0x4000 )
	ROM_LOAD( "9.rom", 0x18000, 0x4000, 0xa3ef809c )
	ROM_CONTINUE(      0x08000, 0x4000 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "4.rom",       0x00000, 0x04000, 0x20b3405f )
	ROM_LOAD( "5.rom",       0x04000, 0x04000, 0xd04ecfa8 )
	ROM_LOAD( "6.rom",       0x08000, 0x04000, 0x33776801 )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "1.rom",       0x00000, 0x04000, 0xf64cbd1e )
	ROM_LOAD( "3.rom",       0x04000, 0x04000, 0xc228df19 )
	ROM_LOAD( "2.rom",       0x08000, 0x04000, 0xa6ad9ce2 )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "1.pr",    0x0000, 0x0100, 0xa784e71f )
	ROM_LOAD( "6.pr",    0x0100, 0x0100, 0x376df30c )
	ROM_LOAD( "5.pr",    0x0200, 0x0100, 0x4edff5bd )
	ROM_LOAD( "4.pr",    0x0300, 0x0100, 0xfe201a4e )
	ROM_LOAD( "2.pr",    0x0400, 0x0100, 0x5796cc4a )
	ROM_LOAD( "3.pr",    0x0500, 0x0100, 0x28b5ee4c )
ROM_END

ROM_START( gigasb )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
	ROM_LOAD( "g-7",   0x10000, 0x4000, 0xdaf4e88d )
	ROM_CONTINUE(      0x00000, 0x4000 )
	ROM_LOAD( "g-8",   0x14000, 0x8000, 0x4ab4c1f1 )
	ROM_CONTINUE(      0x04000, 0x8000 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-4",     0x00000, 0x04000, 0x8ed78981 )
	ROM_LOAD( "g-5",     0x04000, 0x04000, 0x0645ec2d )
	ROM_LOAD( "g-6",     0x08000, 0x04000, 0x99e9cb27 )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-1",     0x00000, 0x04000, 0xd78fae6e )
	ROM_LOAD( "g-3",     0x04000, 0x04000, 0x37df4a4c )
	ROM_LOAD( "g-2",     0x08000, 0x04000, 0x3a46e354 )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "1.pr",    0x0000, 0x0100, 0xa784e71f )
	ROM_LOAD( "6.pr",    0x0100, 0x0100, 0x376df30c )
	ROM_LOAD( "5.pr",    0x0200, 0x0100, 0x4edff5bd )
	ROM_LOAD( "4.pr",    0x0300, 0x0100, 0xfe201a4e )
	ROM_LOAD( "2.pr",    0x0400, 0x0100, 0x5796cc4a )
	ROM_LOAD( "3.pr",    0x0500, 0x0100, 0x28b5ee4c )
ROM_END

ROM_START( oigas )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
	ROM_LOAD( "rom.7",   0x10000, 0x4000, 0xe5bc04cc )
	ROM_CONTINUE(        0x00000, 0x4000)
	ROM_LOAD( "rom.8",   0x04000, 0x8000, 0xc199060d )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )
	ROM_LOAD( "8748.bin",		0x0000, 0x0800, 0x00000000 )	/* missing */

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-4",     0x00000, 0x04000, 0x8ed78981 )
	ROM_LOAD( "g-5",     0x04000, 0x04000, 0x0645ec2d )
	ROM_LOAD( "g-6",     0x08000, 0x04000, 0x99e9cb27 )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-1",     0x00000, 0x04000, 0xd78fae6e )
	ROM_LOAD( "g-3",     0x04000, 0x04000, 0x37df4a4c )
	ROM_LOAD( "g-2",     0x08000, 0x04000, 0x3a46e354 )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "1.pr",    0x0000, 0x0100, 0xa784e71f )
	ROM_LOAD( "6.pr",    0x0100, 0x0100, 0x376df30c )
	ROM_LOAD( "5.pr",    0x0200, 0x0100, 0x4edff5bd )
	ROM_LOAD( "4.pr",    0x0300, 0x0100, 0xfe201a4e )
	ROM_LOAD( "2.pr",    0x0400, 0x0100, 0x5796cc4a )
	ROM_LOAD( "3.pr",    0x0500, 0x0100, 0x28b5ee4c )
ROM_END



static DRIVER_INIT(gigas)
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;
	memory_set_opcode_base(0,rom+diff);
}



GAMEX(1986, gigasb,   0,        gigas,    gigas,    gigas, ROT270, "bootleg", "Gigas (bootleg)", GAME_NO_COCKTAIL )
GAMEX(1986, oigas,    gigasb,   gigas,    gigas,    gigas, ROT270, "bootleg", "Oigas (bootleg)", GAME_NOT_WORKING )
GAMEX(1986, gigasm2b, 0,        gigas,    gigas,    gigas, ROT270, "bootleg", "Gigas Mark II (bootleg)", GAME_NO_COCKTAIL )
GAME( 1987, pbillrd,  0,        pbillrd,  pbillrd,  0,     ROT0,   "Nihon System", "Perfect Billiard" )
GAMEX(1987, pbillrds, pbillrd,  pbillrd,  pbillrd,  0,     ROT0,   "Nihon System", "Perfect Billiard (Sega)", GAME_UNEMULATED_PROTECTION )	// encrypted
GAMEX(1987, freekick, 0,        freekckb, freekckb, 0,     ROT270, "Nihon System (Sega license)", "Free Kick", GAME_NOT_WORKING )
GAME( 1987, freekckb, freekick, freekckb, freekckb, 0,     ROT270, "bootleg", "Free Kick (bootleg)" )
GAMEX(198?, countrun, 0,        freekckb, freekckb, 0,     ROT0,   "Nihon System (Sega license)", "Counter Run", GAME_NOT_WORKING )
