/***************************************************************************

	'Rohga' era hardware:

	Rogha Armour Attack			(c) 1991 Data East Corporation
	Wizard Fire					(c) 1992 Data East Corporation
	Nitroball					(c) 1992 Data East Corporation

	This hardware is capable of alpha-blending on sprites and playfields

	Todo:  On Wizard Fire when you insert a coin and press start, the start
	button being held seems to select the knight right away.  Check on
	real board.

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"
#include "decoprot.h"
#include "deco16ic.h"

VIDEO_START( rohga );
VIDEO_START( wizdfire );
VIDEO_UPDATE( rohga );
VIDEO_UPDATE( wizdfire );

static READ16_HANDLER( rohga_dip3_r ) { return readinputport(3); }

/**********************************************************************************/

static MEMORY_READ16_START( rohga_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x280000, 0x2807ff, deco16_104_rohga_prot_r }, /* Protection device */
	{ 0x2c0000, 0x2c0001, rohga_dip3_r },
	{ 0x321100, 0x321101, MRA16_NOP }, /* Irq ack?  Value not used */
	{ 0x3c0000, 0x3c1fff, MRA16_RAM },
	{ 0x3c2000, 0x3c2fff, MRA16_RAM },
	{ 0x3c4000, 0x3c4fff, MRA16_RAM },
	{ 0x3c6000, 0x3c6fff, MRA16_RAM },
	{ 0x3d0000, 0x3d07ff, MRA16_RAM },
	{ 0x3e0000, 0x3e1fff, MRA16_RAM },
	{ 0x3f0000, 0x3f3fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( rohga_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x20000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x240000, 0x24000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x280000, 0x2807ff, deco16_104_rohga_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0x280800, 0x280fff, deco16_104_rohga_prot_w }, /* Mirror */
//	{ 0x300000, 0x300001, MWA16_NOP },
//	{ 0x310000, 0x310003, MWA16_NOP },
	{ 0x310008, 0x31000b, MWA16_NOP }, /* Palette control?  0000 1111 always written */
	{ 0x322000, 0x322001, deco16_priority_w },
	{ 0x3c0000, 0x3c1fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x3c2000, 0x3c2fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x3c4000, 0x3c4fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x3c6000, 0x3c6fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x3c8000, 0x3c87ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x3ca000, 0x3ca7ff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x3cc000, 0x3cc7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x3ce000, 0x3ce7ff, MWA16_RAM, &deco16_pf4_rowscroll },
	{ 0x3d0000, 0x3d07ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x3e0000, 0x3e1fff, deco16_nonbuffered_palette_w, &paletteram16 },
	{ 0x3f0000, 0x3f3fff, MWA16_RAM }, /* Main ram */
MEMORY_END

static MEMORY_READ16_START( wizdfire_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x202000, 0x202fff, MRA16_RAM },
	{ 0x208000, 0x208fff, MRA16_RAM },
	{ 0x20a000, 0x20afff, MRA16_RAM },
	{ 0x20c000, 0x20cfff, MRA16_RAM },
	{ 0x20e000, 0x20efff, MRA16_RAM },
	{ 0x340000, 0x3407ff, MRA16_RAM },
	{ 0x360000, 0x3607ff, MRA16_RAM },
	{ 0x380000, 0x381fff, MRA16_RAM },
	{ 0xfdc000, 0xfe3fff, MRA16_RAM },
	{ 0xfe4000, 0xfe47ff, deco16_104_prot_r }, /* Protection device */
	{ 0xfe5000, 0xfeffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( wizdfire_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },

	{ 0x200000, 0x200fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x202000, 0x202fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x208000, 0x208fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x20a000, 0x20afff, deco16_pf4_data_w, &deco16_pf4_data },

	{ 0x20b000, 0x20b3ff, MWA16_RAM }, /* ? Always 0 written */
	{ 0x20c000, 0x20c7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x20e000, 0x20e7ff, MWA16_RAM, &deco16_pf4_rowscroll },

	{ 0x300000, 0x30000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x310000, 0x31000f, MWA16_RAM, &deco16_pf34_control },

	{ 0x320000, 0x320001, deco16_priority_w }, /* Priority */
	{ 0x320002, 0x320003, MWA16_NOP }, /* ? */
	{ 0x320004, 0x320005, MWA16_NOP }, /* VBL IRQ ack */

	{ 0x340000, 0x3407ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x350000, 0x350001, buffer_spriteram16_w }, /* Triggers DMA for spriteram */
	{ 0x360000, 0x3607ff, MWA16_RAM, &spriteram16_2, &spriteram_2_size },
	{ 0x370000, 0x370001, buffer_spriteram16_2_w }, /* Triggers DMA for spriteram */

	{ 0x380000, 0x381fff, deco16_buffered_palette_w, &paletteram16 },
	{ 0x390008, 0x390009, deco16_palette_dma_w },

	{ 0xfe4000, 0xfe47ff, deco16_104_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0xfdc000, 0xfeffff, MWA16_RAM }, /* Main ram */
MEMORY_END

static READ16_HANDLER( hr ) { return rand()%0xffff; }
static READ16_HANDLER( hr2 ) { return readinputport(0); }

static MEMORY_READ16_START( nitrobal_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x202000, 0x202fff, MRA16_RAM },
	{ 0x208000, 0x208fff, MRA16_RAM },
	{ 0x20a000, 0x20afff, MRA16_RAM },

	{ 0x300000, 0x30000f, MRA16_RAM },
	{ 0x310000, 0x31000f, MRA16_RAM },

	{ 0x320000, 0x320001, hr2 },
	{ 0x320000, 0x32000f, hr },

	{ 0x340000, 0x3407ff, MRA16_RAM },
	{ 0x360000, 0x3607ff, MRA16_RAM },
	{ 0x380000, 0x381fff, MRA16_RAM },

	{ 0xff4000, 0xff47ff, deco16_146_nitroball_prot_r }, /* Protection device */
	{ 0xfec000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( nitrobal_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },

	{ 0x200000, 0x200fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x202000, 0x202fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x208000, 0x208fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x20a000, 0x20afff, deco16_pf4_data_w, &deco16_pf4_data },

	{ 0x204000, 0x2047ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x206000, 0x2067ff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x20c000, 0x20c7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x20e000, 0x20e7ff, MWA16_RAM, &deco16_pf4_rowscroll },

	{ 0x300000, 0x30000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x310000, 0x31000f, MWA16_RAM, &deco16_pf34_control },

	{ 0x340000, 0x3407ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x350000, 0x350001, buffer_spriteram16_w }, /* Triggers DMA for spriteram */
	{ 0x360000, 0x3607ff, MWA16_RAM, &spriteram16_2, &spriteram_2_size },
	{ 0x370000, 0x370001, buffer_spriteram16_2_w }, /* Triggers DMA for spriteram */

	{ 0x380000, 0x381fff, deco16_nonbuffered_palette_w, &paletteram16 }, //check

	{ 0xff4000, 0xff47ff, deco16_146_nitroball_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0xfec000, 0xffffff, MWA16_RAM }, /* Main ram */
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
		break;
	}
}

static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP }, /* Todo:  Check Nitroball/Rohga */
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/**********************************************************************************/

INPUT_PORTS_START( rohga )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1/2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 3 */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( wizdfire )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1/2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) ) /* Coin mode for continue */
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0300, "4" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x3000, 0x3000, "Magic Guage Speed" )
	PORT_DIPSETTING(      0x0000, "Very Slow" )
	PORT_DIPSETTING(      0x1000, "Slow" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Fast" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/**********************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 16, 0, 24, 8 },
	{ 64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+4, 64*8+5, 64*8+6, 64*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout spritelayout_6bpp =
{
	16,16,
	4096*8,
	6,
	{ 0x400000*8+8, 0x400000*8, 0x200000*8+8, 0x200000*8, 8, 0 },
	{ 7,6,5,4,3,2,1,0,
	32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,  },
	{ 15*16, 14*16, 13*16, 12*16, 11*16, 10*16, 9*16, 8*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	64*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0,  },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 32 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,          0, 32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,        512, 32 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout_6bpp,1024, 32 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_wizdfire[] =
{
	{ REGION_GFX1, 0, &charlayout,        0, 32 },	/* Gfx chip 1 as 8x8 */
	{ REGION_GFX2, 0, &tilelayout,        0, 32 },	/* Gfx chip 1 as 16x16 */
	{ REGION_GFX3, 0, &tilelayout,      512, 32 },  /* Gfx chip 2 as 16x16 */
	{ REGION_GFX4, 0, &spritelayout,   1024, 32 }, /* Sprites 16x16 */
	{ REGION_GFX5, 0, &spritelayout,   1536, 32 },
	{ -1 } /* end of array */
};

/**********************************************************************************/

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	OKIM6295_set_bank_base(0, ((data & 1)>>0) * 0x40000);
	OKIM6295_set_bank_base(1, ((data & 2)>>1) * 0x40000);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 75, 60 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

/**********************************************************************************/

static MACHINE_DRIVER_START( rohga )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(rohga_readmem,rohga_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(rohga)
	MDRV_VIDEO_UPDATE(rohga)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wizdfire )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(wizdfire_readmem,wizdfire_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_wizdfire)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(wizdfire)
	MDRV_VIDEO_UPDATE(wizdfire)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nitrobal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(nitrobal_readmem,nitrobal_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_wizdfire)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(rohga)
	MDRV_VIDEO_UPDATE(rohga)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/**********************************************************************************/

ROM_START( rohgau )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ha00.2a",  0x000000, 0x40000, 0xd8d13052 )
	ROM_LOAD16_BYTE( "ha03.2d",  0x000001, 0x40000, 0x5f683bbf )
	ROM_LOAD16_BYTE( "mam00.8a",  0x100000, 0x80000, 0x0fa440a6 )
	ROM_LOAD16_BYTE( "mam07.8d",  0x100001, 0x80000, 0xf8bc7f20 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "ha04.18p",  0x00000,  0x10000,  0xeb6608eb )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ha01.13a",  0x00000,  0x10000,  0xfb8f8519 ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "ha02.14a",  0x00001,  0x10000,  0xaa47c17f )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mam01.10a", 0x000000, 0x080000,  0xdbf4fbcc ) /* Encrypted tiles */
	ROM_LOAD( "mam02.11a", 0x080000, 0x080000,  0xb1fac481 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mam08.17d",  0x000000, 0x100000,  0xca97a83f ) /* tiles 1 & 2 */
	ROM_LOAD( "mam09.18d",  0x100000, 0x100000,  0x3f57d56f )

	ROM_REGION( 0x600000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mam05.19a", 0x000000, 0x100000,  0x307a2cd1 ) /* 6bpp sprites */
	ROM_LOAD( "mam06.20a", 0x100000, 0x100000,  0xa1119a2d )
	ROM_LOAD( "mam10.19d", 0x200000, 0x100000,  0x99f48f9f )
	ROM_LOAD( "mam11.20d", 0x300000, 0x100000,  0xc3f12859 )
	ROM_LOAD( "mam03.17a", 0x400000, 0x100000,  0xfc4dfd48 )
	ROM_LOAD( "mam04.18a", 0x500000, 0x100000,  0x7d3b38bf )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mam12.14p", 0x00000,  0x80000,  0x6f00b791 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mam13.15p", 0x00000,  0x80000,  0x525b9461 )

	ROM_REGION( 512, REGION_PROMS, 0 )
	ROM_LOAD( "hb-00.11p", 0x00000,  0x200,  0xb7a7baad )	/* ? */
ROM_END

ROM_START( rohgah )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jd00-2.2a", 0x000000, 0x40000, 0xec70646a )
	ROM_LOAD16_BYTE( "jd03-2.2d", 0x000001, 0x40000, 0x11d4c9a2 )
	ROM_LOAD16_BYTE( "mam00.8a",  0x100000, 0x80000, 0x0fa440a6 )
	ROM_LOAD16_BYTE( "mam07.8d",  0x100001, 0x80000, 0xf8bc7f20 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "ha04.18p",  0x00000,  0x10000,  0xeb6608eb )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ha01.13a",  0x00000,  0x10000,  0xfb8f8519 ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "ha02.14a",  0x00001,  0x10000,  0xaa47c17f )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mam01.10a", 0x000000, 0x080000,  0xdbf4fbcc ) /* Encrypted tiles */
	ROM_LOAD( "mam02.11a", 0x080000, 0x080000,  0xb1fac481 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mam08.17d",  0x000000, 0x100000,  0xca97a83f ) /* tiles 1 & 2 */
	ROM_LOAD( "mam09.18d",  0x100000, 0x100000,  0x3f57d56f )

	ROM_REGION( 0x600000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mam05.19a", 0x000000, 0x100000,  0x307a2cd1 ) /* 6bpp sprites */
	ROM_LOAD( "mam06.20a", 0x100000, 0x100000,  0xa1119a2d )
	ROM_LOAD( "mam10.19d", 0x200000, 0x100000,  0x99f48f9f )
	ROM_LOAD( "mam11.20d", 0x300000, 0x100000,  0xc3f12859 )
	ROM_LOAD( "mam03.17a", 0x400000, 0x100000,  0xfc4dfd48 )
	ROM_LOAD( "mam04.18a", 0x500000, 0x100000,  0x7d3b38bf )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mam12.14p", 0x00000,  0x80000,  0x6f00b791 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mam13.15p", 0x00000,  0x80000,  0x525b9461 )

	ROM_REGION( 512, REGION_PROMS, 0 )
	ROM_LOAD( "hb-00.11p", 0x00000,  0x200,  0xb7a7baad )	/* ? */
ROM_END

ROM_START( rohga )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jd00.bin",  0x000000, 0x40000, 0xe046c77a )
	ROM_LOAD16_BYTE( "jd03.bin",  0x000001, 0x40000, 0x2c5120b8 )
	ROM_LOAD16_BYTE( "mam00.8a",  0x100000, 0x80000, 0x0fa440a6 )
	ROM_LOAD16_BYTE( "mam07.8d",  0x100001, 0x80000, 0xf8bc7f20 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "ha04.18p",  0x00000,  0x10000,  0xeb6608eb )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ha01.13a",  0x00000,  0x10000,  0xfb8f8519 ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "ha02.14a",  0x00001,  0x10000,  0xaa47c17f )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mam01.10a", 0x000000, 0x080000,  0xdbf4fbcc ) /* Encrypted tiles */
	ROM_LOAD( "mam02.11a", 0x080000, 0x080000,  0xb1fac481 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mam08.17d",  0x000000, 0x100000,  0xca97a83f ) /* tiles 1 & 2 */
	ROM_LOAD( "mam09.18d",  0x100000, 0x100000,  0x3f57d56f )

	ROM_REGION( 0x600000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mam05.19a", 0x000000, 0x100000,  0x307a2cd1 ) /* 6bpp sprites */
	ROM_LOAD( "mam06.20a", 0x100000, 0x100000,  0xa1119a2d )
	ROM_LOAD( "mam10.19d", 0x200000, 0x100000,  0x99f48f9f )
	ROM_LOAD( "mam11.20d", 0x300000, 0x100000,  0xc3f12859 )
	ROM_LOAD( "mam03.17a", 0x400000, 0x100000,  0xfc4dfd48 )
	ROM_LOAD( "mam04.18a", 0x500000, 0x100000,  0x7d3b38bf )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mam12.14p", 0x00000,  0x80000,  0x6f00b791 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mam13.15p", 0x00000,  0x80000,  0x525b9461 )

	ROM_REGION( 512, REGION_PROMS, 0 )
	ROM_LOAD( "hb-00.11p", 0x00000,  0x200,  0xb7a7baad )	/* ? */
ROM_END

ROM_START( wizdfire )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jf-01.3d",   0x000000, 0x20000, 0xbde42a41 )
	ROM_LOAD16_BYTE( "jf-00.3a",   0x000001, 0x20000, 0xbca3c995 )
	ROM_LOAD16_BYTE( "jf-03.5d",   0x040000, 0x20000, 0x5217d404 )
	ROM_LOAD16_BYTE( "jf-02.5a",   0x040001, 0x20000, 0x36a1ce28 )
	ROM_LOAD16_BYTE( "mas13",   0x080000, 0x80000, 0x7e5256ce )
	ROM_LOAD16_BYTE( "mas12",   0x080001, 0x80000, 0x005bd499 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "jf-06.20r",  0x00000,  0x10000,  0x79042546 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "jf-04.10d",  0x00000,  0x10000,  0x73cba800 ) /* Chars */
	ROM_LOAD16_BYTE( "jf-05.12d",  0x00001,  0x10000,  0x22e2c49d )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mas00", 0x000000, 0x100000,  0x3d011034 ) /* Tiles */
	ROM_LOAD( "mas01", 0x100000, 0x100000,  0x6d0c9d0b )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mas02", 0x000000, 0x080000,  0xaf00e620 )
	ROM_LOAD( "mas03", 0x080000, 0x080000,  0x2fe61ea2 )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas04", 0x000001, 0x100000,  0x1e56953b ) /* Sprites #1 */
	ROM_LOAD16_BYTE( "mas05", 0x000000, 0x100000,  0x3826b8f8 )
	ROM_LOAD16_BYTE( "mas06", 0x200001, 0x100000,  0x3b8bbd45 )
	ROM_LOAD16_BYTE( "mas07", 0x200000, 0x100000,  0x31303769 )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas08", 0x000001, 0x080000,  0xe224fb7a ) /* Sprites #2 */
	ROM_LOAD16_BYTE( "mas09", 0x000000, 0x080000,  0x5f6deb41 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mas10",  0x00000,  0x80000,  0x6edc06a7 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mas11",  0x00000,  0x80000,  0xc2f0a4f2 )

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.16l", 0x00000,  0x400,  0x2bee57cc )	/* Priority (unused) */
ROM_END

ROM_START( darksel2 )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jb01-3",  0x000000, 0x20000, 0x82308c01 )
	ROM_LOAD16_BYTE( "jb00-3",  0x000001, 0x20000, 0x1d38113a )
	ROM_LOAD16_BYTE( "jf-03.5d",0x040000, 0x20000, 0x5217d404 )
	ROM_LOAD16_BYTE( "jf-02.5a",0x040001, 0x20000, 0x36a1ce28 )
	ROM_LOAD16_BYTE( "mas13",   0x080000, 0x80000, 0x7e5256ce )
	ROM_LOAD16_BYTE( "mas12",   0x080001, 0x80000, 0x005bd499 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "jb06",  0x00000,  0x10000,  0x2066a1dd )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "jf-04.10d",  0x00000,  0x10000,  0x73cba800 ) /* Chars */
	ROM_LOAD16_BYTE( "jf-05.12d",  0x00001,  0x10000,  0x22e2c49d )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mas00", 0x000000, 0x100000,  0x3d011034 ) /* Tiles */
	ROM_LOAD( "mas01", 0x100000, 0x100000,  0x6d0c9d0b )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mas02", 0x000000, 0x080000,  0xaf00e620 )
	ROM_LOAD( "mas03", 0x080000, 0x080000,  0x2fe61ea2 )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas04", 0x000001, 0x100000,  0x1e56953b ) /* Sprites #1 */
	ROM_LOAD16_BYTE( "mas05", 0x000000, 0x100000,  0x3826b8f8 )
	ROM_LOAD16_BYTE( "mas06", 0x200001, 0x100000,  0x3b8bbd45 )
	ROM_LOAD16_BYTE( "mas07", 0x200000, 0x100000,  0x31303769 )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas08", 0x000001, 0x080000,  0xe224fb7a ) /* Sprites #2 */
	ROM_LOAD16_BYTE( "mas09", 0x000000, 0x080000,  0x5f6deb41 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mas10",  0x00000,  0x80000,  0x6edc06a7 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mas11",  0x00000,  0x80000,  0xc2f0a4f2 )

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.16l", 0x00000,  0x400,  0x2bee57cc )	/* Priority (unused) */
ROM_END

ROM_START( nitrobal ) /* Todo - rename these roms to correct DE codes */
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "nitrobal.d3",   0x000000, 0x20000, 0x0414e409 )
	ROM_LOAD16_BYTE( "nitrobal.b3",   0x000001, 0x20000, 0xdd9e2bcc )
	ROM_LOAD16_BYTE( "nitrobal.d5",   0x040000, 0x20000, 0xea264ac5 )
	ROM_LOAD16_BYTE( "nitrobal.b5",   0x040001, 0x20000, 0x74047997 )
	ROM_LOAD16_BYTE( "nitrobal.d6",   0x080000, 0x40000, 0xb820fa20 )
	ROM_LOAD16_BYTE( "nitrobal.b6",   0x080001, 0x40000, 0x1fd8995b )
	/* Two empty rom slots at d7, b7 */

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "nitrobal.r20",  0x00000,  0x10000,  0x93d93fe1 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nitrobal.d10",  0x00000,  0x10000,  0x91cf668e ) /* Chars */
	ROM_LOAD16_BYTE( "nitrobal.d12",  0x00001,  0x10000,  0xe61d0e42 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "nitrobal.b10", 0x00000, 0x80000,  0x34785d97 ) /* Tiles */
	ROM_LOAD( "nitrobal.b12", 0x80000, 0x80000,  0x8b531b16 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "nitrobal.16e", 0x000000, 0x100000,  0xef6195f0 )
	ROM_LOAD( "nitrobal.b16", 0x100000, 0x100000,  0x20723bf7 )

	ROM_REGION( 0x300000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nitrobal.e19", 0x000000, 0x100000,  0xd92d769c ) /* Sprites #1 */
	ROM_LOAD16_BYTE( "nitrobal.b19", 0x000001, 0x100000,  0x8ba48385 )
	ROM_LOAD16_BYTE( "nitrobal.e20", 0x200000, 0x080000,  0x5fc10ccd )
	ROM_LOAD16_BYTE( "nitrobal.b20", 0x200001, 0x080000,  0xae6201a5 )

	ROM_REGION( 0x80000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "nitrobal.e23", 0x000000, 0x040000,  0x1ce7b51a ) /* Sprites #2 */
	ROM_LOAD16_BYTE( "nitrobal.b23", 0x000001, 0x040000,  0x64966576 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "nitrobal.r17",  0x00000,  0x80000,  0x8ad734b0 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "nitrobal.r19",  0x00000,  0x80000,  0xef513908 )
ROM_END

/**********************************************************************************/

static DRIVER_INIT( rohga )
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
}

static DRIVER_INIT( wizdfire )
{
	deco74_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);
}

static DRIVER_INIT( nitrobal )
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);
}

GAMEX(1991, rohga,    0,       rohga,    rohga,    rohga,    ROT0,   "Data East Corporation", "Rohga Armour Force (Asia/Europe v3.0)", GAME_UNEMULATED_PROTECTION )
GAMEX(1991, rohgah,   rohga,   rohga,    rohga,    rohga,    ROT0,   "Data East Corporation", "Rohga Armour Force (Hong Kong v3.0)", GAME_UNEMULATED_PROTECTION )
GAMEX(1991, rohgau,   rohga,   rohga,    rohga,    rohga,    ROT0,   "Data East Corporation", "Rohga Armour Force (US v1.0)", GAME_UNEMULATED_PROTECTION )
GAME( 1992, wizdfire, 0,       wizdfire, wizdfire, wizdfire, ROT0,   "Data East Corporation", "Wizard Fire (US v1.1)" )
GAME( 1992, darksel2, wizdfire,wizdfire, wizdfire, wizdfire, ROT0,   "Data East Corporation", "Dark Seal 2 (Japan v2.1)" )
GAMEX(1992, nitrobal, 0,       nitrobal, rohga,    nitrobal, ROT270, "Data East Corporation", "Nitroball (US)", GAME_UNEMULATED_PROTECTION )
