/******************************************************************************

	nbmj8688 - Nichibutsu Mahjong games for years 1986-1988

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/01/28 -

******************************************************************************/
/******************************************************************************

TODO:

- Inputs slightly wrong for the LCD games. In those games, start 1 begins a
  2 players game. To start a 1 player game, press flip (X).

- Animation in bijokkoy and bijokkog (while DAC playback) is not correct.
  Interrupt problem?

- Sampling rate of some DAC playback in bijokkoy and bijokkog is too high.
  Interrupt problem?

- Input handling is wrong in crystalg and crystal2.

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


#define	SIGNED_DAC	0		// 0:unsigned DAC, 1:signed DAC


PALETTE_INIT( mbmj8688_8bit );
PALETTE_INIT( mbmj8688_12bit );
PALETTE_INIT( mbmj8688_16bit );
VIDEO_UPDATE( mbmj8688 );
VIDEO_UPDATE( mbmj8688_LCD );
VIDEO_START( mbmj8688_8bit );
VIDEO_START( mbmj8688_hybrid_12bit );
VIDEO_START( mbmj8688_pure_12bit );
VIDEO_START( mbmj8688_hybrid_16bit );
VIDEO_START( mbmj8688_pure_16bit );
VIDEO_START( mbmj8688_pure_16bit_LCD );

WRITE_HANDLER( nbmj8688_color_lookup_w );
WRITE_HANDLER( nbmj8688_blitter_w );
WRITE_HANDLER( mjsikaku_gfxflag1_w );
WRITE_HANDLER( mjsikaku_gfxflag2_w );
WRITE_HANDLER( mjsikaku_gfxflag3_w );
WRITE_HANDLER( mjsikaku_scrolly_w );
WRITE_HANDLER( mjsikaku_romsel_w );
WRITE_HANDLER( secolove_romsel_w );
WRITE_HANDLER( seiha_romsel_w );
WRITE_HANDLER( crystal2_romsel_w );

WRITE_HANDLER( nbmj8688_HD61830B_0_instr_w );
WRITE_HANDLER( nbmj8688_HD61830B_0_data_w );
WRITE_HANDLER( nbmj8688_HD61830B_1_instr_w );
WRITE_HANDLER( nbmj8688_HD61830B_1_data_w );
WRITE_HANDLER( nbmj8688_HD61830B_both_instr_w );
WRITE_HANDLER( nbmj8688_HD61830B_both_data_w );


static DRIVER_INIT( mjsikaku )
{
	nb1413m3_type = NB1413M3_MJSIKAKU;
}

static DRIVER_INIT( otonano )
{
	nb1413m3_type = NB1413M3_OTONANO;
}

static DRIVER_INIT( mjcamera )
{
	UINT8 *rom = memory_region(REGION_SOUND1) + 0x20000;
	UINT8 *prot = memory_region(REGION_USER1);
	int i;

	/* this is one possible way to rearrange the protection ROM data to get the
	   expected 0x5894 checksum. It's probably completely wrong! But since the
	   game doesn't do anything else with that ROM, this is more than enough. I
	   could just fill this are with fake data, the only thing that matters is
	   the checksum. */
	for (i = 0;i < 0x10000;i++)
	{
		rom[i] = BITSWAP8(prot[i],1,6,0,4,2,3,5,7);
	}

	nb1413m3_type = NB1413M3_MJCAMERA;
}

static DRIVER_INIT( kanatuen )
{
	/* uses the same protection data as mjcamer, but a different check */
	UINT8 *rom = memory_region(REGION_SOUND1) + 0x30000;

	rom[0x0004] = 0x09;
	rom[0x0103] = 0x0e;
	rom[0x0202] = 0x08;
	rom[0x0301] = 0xdc;

	nb1413m3_type = NB1413M3_KANATUEN;
}

static DRIVER_INIT( idhimitu )
{
	UINT8 *rom = memory_region(REGION_SOUND1) + 0x20000;
	UINT8 *prot = memory_region(REGION_USER1);
	int i;

	/* this is one possible way to rearrange the protection ROM data to get the
	   expected 0x9944 checksum. It's probably completely wrong! But since the
	   game doesn't do anything else with that ROM, this is more than enough. I
	   could just fill this are with fake data, the only thing that matters is
	   the checksum. */
	for (i = 0;i < 0x10000;i++)
	{
		rom[i] = BITSWAP8(prot[i + 0x10000],4,6,2,1,7,0,3,5);
	}

	nb1413m3_type = NB1413M3_IDHIMITU;
}

static DRIVER_INIT( kaguya )
{
	nb1413m3_type = NB1413M3_KAGUYA;
}

static DRIVER_INIT( secolove )
{
	nb1413m3_type = NB1413M3_SECOLOVE;
}

static DRIVER_INIT( citylove )
{
	nb1413m3_type = NB1413M3_CITYLOVE;
}

static DRIVER_INIT( seiha )
{
	nb1413m3_type = NB1413M3_SEIHA;
}

static DRIVER_INIT( seiham )
{
	nb1413m3_type = NB1413M3_SEIHAM;
}

static DRIVER_INIT( korinai )
{
	nb1413m3_type = NB1413M3_KORINAI;
}

static DRIVER_INIT( iemoto )
{
	nb1413m3_type = NB1413M3_IEMOTO;
}

static DRIVER_INIT( ojousan )
{
	nb1413m3_type = NB1413M3_OJOUSAN;
}

static DRIVER_INIT( bijokkoy )
{
	nb1413m3_type = NB1413M3_BIJOKKOY;
}

static DRIVER_INIT( bijokkog )
{
	nb1413m3_type = NB1413M3_BIJOKKOG;
}

static DRIVER_INIT( housemnq )
{
	nb1413m3_type = NB1413M3_HOUSEMNQ;
}

static DRIVER_INIT( housemn2 )
{
	nb1413m3_type = NB1413M3_HOUSEMN2;
}

static DRIVER_INIT( orangec )
{
	nb1413m3_type = NB1413M3_ORANGEC;
}

static DRIVER_INIT( crystalg )
{
	nb1413m3_type = NB1413M3_CRYSTALG;
}

static DRIVER_INIT( crystal2 )
{
	nb1413m3_type = NB1413M3_CRYSTAL2;
}

static DRIVER_INIT( apparel )
{
	nb1413m3_type = NB1413M3_APPAREL;
}



static MEMORY_READ_START( readmem_mjsikaku )
	{ 0x0000, 0xf7ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_mjsikaku )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
MEMORY_END

static MEMORY_READ_START( readmem_secolove )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_secolove )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
MEMORY_END

static MEMORY_READ_START( readmem_ojousan )
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_ojousan )
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



static READ_HANDLER( sndrom_r )
{
	/* get top 8 bits of the I/O port address */
	offset = (offset << 8) | (activecpu_get_reg(Z80_BC) >> 8);
	return nb1413m3_sndrom_r(offset);
}

static READ_HANDLER( ff_r )
{
	/* possibly because of a bug, reads from port 0xd0 must return 0xff
	   otherwise apparel doesn't clear the background when you insert a coin */
	return 0xff;
}



static PORT_READ_START( readport_secolove )
	{ 0x00, 0x7f, sndrom_r },
	{ 0x81, 0x81, AY8910_read_port_0_r },
	{ 0x90, 0x90, nb1413m3_inputport0_r },
	{ 0xa0, 0xa0, nb1413m3_inputport1_r },
	{ 0xb0, 0xb0, nb1413m3_inputport2_r },
	{ 0xd0, 0xd0, ff_r },	// irq ack? watchdog?
	{ 0xf0, 0xf0, nb1413m3_dipsw1_r },
	{ 0xf1, 0xf1, nb1413m3_dipsw2_r },
PORT_END

static PORT_WRITE_START( writeport_secolove )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x82, 0x82, AY8910_write_port_0_w },
	{ 0x83, 0x83, AY8910_control_port_0_w },
	{ 0x90, 0x95, nbmj8688_blitter_w },
	{ 0x96, 0x96, mjsikaku_gfxflag1_w },
//	{ 0x97, 0x97,
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, nb1413m3_sndrombank1_w },
	{ 0xc0, 0xcf, nbmj8688_color_lookup_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, secolove_romsel_w },
	{ 0xf0, 0xf0, mjsikaku_scrolly_w },
PORT_END

static PORT_WRITE_START( writeport_crystal2 )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x82, 0x82, AY8910_write_port_0_w },
	{ 0x83, 0x83, AY8910_control_port_0_w },
	{ 0x90, 0x95, nbmj8688_blitter_w },
	{ 0x96, 0x96, mjsikaku_gfxflag1_w },
//	{ 0x97, 0x97,
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, nb1413m3_sndrombank1_w },
	{ 0xc0, 0xcf, nbmj8688_color_lookup_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, crystal2_romsel_w },
//	{ 0xf0, 0xf0,
PORT_END


static PORT_READ_START( readport_otonano )
	{ 0x00, 0x7f, sndrom_r },
	{ 0x80, 0x80, YM3812_status_port_0_r },
	{ 0x90, 0x90, nb1413m3_inputport0_r },
	{ 0xa0, 0xa0, nb1413m3_inputport1_r },
	{ 0xb0, 0xb0, nb1413m3_inputport2_r },
	{ 0xd0, 0xd0, ff_r },	// irq ack? watchdog?
	{ 0xf0, 0xf0, nb1413m3_dipsw1_r },
	{ 0xf1, 0xf1, nb1413m3_dipsw2_r },
PORT_END

static PORT_WRITE_START( writeport_otonano )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x20, 0x3f, nbmj8688_color_lookup_w },
	{ 0x50, 0x50, mjsikaku_romsel_w },
	{ 0x70, 0x75, nbmj8688_blitter_w },
	{ 0x76, 0x76, mjsikaku_gfxflag1_w },
//	{ 0x77, 0x77,
	{ 0x80, 0x80, YM3812_control_port_0_w },
	{ 0x81, 0x81, YM3812_write_port_0_w },
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, nb1413m3_sndrombank1_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, mjsikaku_gfxflag2_w },
	{ 0xf0, 0xf0, mjsikaku_scrolly_w },
PORT_END


static PORT_READ_START( readport_kaguya )
	{ 0x00, 0x7f, sndrom_r },
	{ 0x81, 0x81, AY8910_read_port_0_r },
	{ 0x90, 0x90, nb1413m3_inputport0_r },
	{ 0xa0, 0xa0, nb1413m3_inputport1_r },
	{ 0xb0, 0xb0, nb1413m3_inputport2_r },
	{ 0xd0, 0xd0, ff_r },	// irq ack? watchdog?
	{ 0xf0, 0xf0, nb1413m3_dipsw1_r },
	{ 0xf1, 0xf1, nb1413m3_dipsw2_r },
PORT_END

static PORT_WRITE_START( writeport_kaguya )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x20, 0x3f, nbmj8688_color_lookup_w },
	{ 0x50, 0x50, mjsikaku_romsel_w },
	{ 0x70, 0x75, nbmj8688_blitter_w },
	{ 0x76, 0x76, mjsikaku_gfxflag1_w },
//	{ 0x77, 0x77,
	{ 0x82, 0x82, AY8910_write_port_0_w },
	{ 0x83, 0x83, AY8910_control_port_0_w },
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, nb1413m3_sndrombank1_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, mjsikaku_gfxflag2_w },
	{ 0xf0, 0xf0, mjsikaku_scrolly_w },
PORT_END


static PORT_WRITE_START( writeport_iemoto )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x20, 0x3f, nbmj8688_color_lookup_w },
	{ 0x10, 0x10, nb1413m3_sndrombank2_w },
	{ 0x40, 0x45, nbmj8688_blitter_w },
	{ 0x46, 0x46, mjsikaku_gfxflag1_w },
//	{ 0x47, 0x47,
	{ 0x50, 0x50, seiha_romsel_w },
	{ 0x82, 0x82, AY8910_write_port_0_w },
	{ 0x83, 0x83, AY8910_control_port_0_w },
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, nb1413m3_sndrombank1_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, mjsikaku_gfxflag2_w },
	{ 0xf0, 0xf0, mjsikaku_scrolly_w },
PORT_END


static WRITE_HANDLER( seiha_b0_w )
{
	nb1413m3_outcoin_w(0,data);
	nb1413m3_sndrombank1_w(0,data);
}

static PORT_WRITE_START( writeport_seiha )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x10, 0x10, nb1413m3_sndrombank2_w },
	{ 0x20, 0x3f, nbmj8688_color_lookup_w },
	{ 0x50, 0x50, seiha_romsel_w },
	{ 0x82, 0x82, AY8910_write_port_0_w },
	{ 0x83, 0x83, AY8910_control_port_0_w },
	{ 0x90, 0x95, nbmj8688_blitter_w },
	{ 0x96, 0x96, mjsikaku_gfxflag1_w },
//	{ 0x97, 0x97,
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, seiha_b0_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, mjsikaku_gfxflag2_w },
	{ 0xf0, 0xf0, mjsikaku_scrolly_w },
PORT_END


static PORT_WRITE_START( writeport_p16bit_LCD )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x42, 0x42, nbmj8688_HD61830B_0_data_w },
	{ 0x43, 0x43, nbmj8688_HD61830B_0_instr_w },
	{ 0x44, 0x44, nbmj8688_HD61830B_1_data_w },
	{ 0x45, 0x45, nbmj8688_HD61830B_1_instr_w },
	{ 0x46, 0x46, nbmj8688_HD61830B_both_data_w },
	{ 0x47, 0x47, nbmj8688_HD61830B_both_instr_w },
	{ 0x82, 0x82, AY8910_write_port_0_w },
	{ 0x83, 0x83, AY8910_control_port_0_w },
	{ 0x90, 0x95, nbmj8688_blitter_w },
	{ 0x96, 0x96, mjsikaku_gfxflag1_w },
//	{ 0x97, 0x97,
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, nb1413m3_sndrombank1_w },
	{ 0xc0, 0xcf, nbmj8688_color_lookup_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, secolove_romsel_w },
	{ 0xf0, 0xf0, mjsikaku_scrolly_w },
PORT_END


static PORT_READ_START( readport_mjsikaku )
	{ 0x00, 0x7f, sndrom_r },
	{ 0x90, 0x90, nb1413m3_inputport0_r },
	{ 0xa0, 0xa0, nb1413m3_inputport1_r },
	{ 0xb0, 0xb0, nb1413m3_inputport2_r },
	{ 0xd0, 0xd0, ff_r },	// irq ack? watchdog?
	{ 0xf0, 0xf0, nb1413m3_dipsw1_r },
	{ 0xf1, 0xf1, nb1413m3_dipsw2_r },
PORT_END

static PORT_WRITE_START( writeport_mjsikaku )
	{ 0x00, 0x00, nb1413m3_nmi_clock_w },
	{ 0x10, 0x10, nb1413m3_sndrombank2_w },
	{ 0x20, 0x3f, nbmj8688_color_lookup_w },
	{ 0x50, 0x50, mjsikaku_romsel_w },
	{ 0x60, 0x65, nbmj8688_blitter_w },
	{ 0x66, 0x66, mjsikaku_gfxflag1_w },
//	{ 0x67, 0x67,
	{ 0x80, 0x80, YM3812_control_port_0_w },
	{ 0x81, 0x81, YM3812_write_port_0_w },
	{ 0xa0, 0xa0, nb1413m3_inputportsel_w },
	{ 0xb0, 0xb0, nb1413m3_sndrombank1_w },
#if SIGNED_DAC
	{ 0xd0, 0xd0, DAC_0_signed_data_w },
#else
	{ 0xd0, 0xd0, DAC_0_data_w },
#endif
	{ 0xe0, 0xe0, mjsikaku_gfxflag2_w },
	{ 0xf0, 0xf0, mjsikaku_scrolly_w },
PORT_END



INPUT_PORTS_START( mjsikaku )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Game Sounds" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( otonano )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "TSUMIPAI ENCHOU" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Last chance needs 1,000points" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x40, 0x40, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Play fee display" )
	PORT_DIPSETTING(    0x80, "100 Yen" )
	PORT_DIPSETTING(    0x00, "50 Yen" )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, "Character Display Test" )
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( mjcamera )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Character Display Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( kaguya )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	// NOTE:Coins counted by pressing service switch
	PORT_DIPNAME( 0x04, 0x04, "NOTE" )
	PORT_DIPSETTING(    0x04, "Coin x5" )
	PORT_DIPSETTING(    0x00, "Coin x10" )
	PORT_DIPNAME( 0x18, 0x18, "Game Out" )
	PORT_DIPSETTING(    0x18, "90% (Easy)" )
	PORT_DIPSETTING(    0x10, "80%" )
	PORT_DIPSETTING(    0x08, "70%" )
	PORT_DIPSETTING(    0x00, "60% (Hard)" )
	PORT_DIPNAME( 0x20, 0x20, "Bonus awarded on" )
	PORT_DIPSETTING(    0x20, "[over BAIMAN]" )
	PORT_DIPSETTING(    0x00, "[BAIMAN]" )
	PORT_DIPNAME( 0x40, 0x40, "Variability of payout rate" )
	PORT_DIPSETTING(    0x40, "[big]" )
	PORT_DIPSETTING(    0x00, "[small]" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x00, "Nudity graphic on bet" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, "Bet Min" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x18, 0x00, "Number of extend TSUMO" )
	PORT_DIPSETTING(    0x18, "0" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x20, 0x20, "Extend TSUMO needs credit" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( kanatuen )
	PORT_START	/* (0) DIPSW-A */
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

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Character Display Test" )
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( idhimitu )
	PORT_START	/* (0) DIPSW-A */
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

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Character Display Test" )
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( secolove )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x00, "Hanahai" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Open Reach of CPU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Cancel Hand" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Wareme" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( citylove )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0f, "1 (Easy)" )
	PORT_DIPSETTING(    0x0e, "2" )
	PORT_DIPSETTING(    0x0d, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0b, "5" )
	PORT_DIPSETTING(    0x0a, "6" )
	PORT_DIPSETTING(    0x09, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x07, "9" )
	PORT_DIPSETTING(    0x06, "10" )
	PORT_DIPSETTING(    0x05, "11" )
	PORT_DIPSETTING(    0x04, "12" )
	PORT_DIPSETTING(    0x03, "13" )
	PORT_DIPSETTING(    0x02, "14" )
	PORT_DIPSETTING(    0x01, "15" )
	PORT_DIPSETTING(    0x00, "16 (Hard)" )
	PORT_DIPNAME( 0x30, 0x30, "YAKUMAN cut" )
	PORT_DIPSETTING(    0x30, "10%" )
	PORT_DIPSETTING(    0x20, "30%" )
	PORT_DIPSETTING(    0x10, "50%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x40, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x04, "Hanahai" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Chonbo" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Open Reach of CPU" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Open Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, "Cansel Type" )
	PORT_DIPSETTING(    0xc0, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, "TSUMO 3" )
	PORT_DIPSETTING(    0x40, "TSUMO 7" )
	PORT_DIPSETTING(    0x00, "HAIPAI" )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( seiha )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Hard)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Easy)" )
	PORT_DIPNAME( 0x08, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Change Pai and Mat Color" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Character Display Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( seiham )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x00, "Game Out" )
	PORT_DIPSETTING(    0x07, "60% (Hard)" )
	PORT_DIPSETTING(    0x06, "65%" )
	PORT_DIPSETTING(    0x05, "70%" )
	PORT_DIPSETTING(    0x04, "75%" )
	PORT_DIPSETTING(    0x03, "80%" )
	PORT_DIPSETTING(    0x02, "85%" )
	PORT_DIPSETTING(    0x01, "90%" )
	PORT_DIPSETTING(    0x00, "95% (Easy)" )
	PORT_DIPNAME( 0x18, 0x18, "Rate Min" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x60, 0x00, "Rate Max" )
	PORT_DIPSETTING(    0x60, "8" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x20, "12" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x80, 0x00, "Score Pool" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x00, "Rate Up" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Last Chance" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Character Display Test" )
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( iemoto )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Hard)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Easy)" )
	PORT_DIPNAME( 0x08, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
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

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( bijokkoy )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, "2P Simultaneous Play (LCD req'd)" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "See non-Reacher's hand" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Character Display Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Double Tsumo" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Chonbo" )
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( bijokkog )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, "2P Simultaneous Play (LCD req'd)" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "See non-Reacher's hand" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bonus Score for Extra Credit" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Double Tsumo" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Chonbo" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Cancel Hand" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( housemnq )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Chonbo" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Character Display Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "See CPU's hand" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x03, "Time" )
	PORT_DIPSETTING(    0x03, "120" )
	PORT_DIPSETTING(    0x02, "100" )
	PORT_DIPSETTING(    0x01, "80" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Speed" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Coin Selector" )
	PORT_DIPSETTING(    0x20, "common" )
	PORT_DIPSETTING(    0x00, "separate" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( housemn2 )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "Kan-Ura" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Chonbo" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Character Display Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "See CPU's hand" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x03, "Time" )
	PORT_DIPSETTING(    0x03, "120" )
	PORT_DIPSETTING(    0x02, "100" )
	PORT_DIPSETTING(    0x01, "80" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Speed" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Coin Selector" )
	PORT_DIPSETTING(    0x20, "common" )
	PORT_DIPSETTING(    0x00, "separate" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( orangec )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Select Girl" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	// COIN1 in other games, not working?

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( vipclub )
	PORT_START	/* (0) DIPSW-A */
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
	PORT_DIPNAME( 0x40, 0x40, "Character Display Test" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	// COIN1 in other games, not working?

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( ojousan )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x00, "RENCHAN after TENPAIed RYUKYOKU" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
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

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( korinai )
	PORT_START	/* (0) DIPSW-A */
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
	PORT_DIPNAME( 0x10, 0x10, "Character Display Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Select" )
	PORT_DIPSETTING(    0x20, "Region" )
	PORT_DIPSETTING(    0x00, "Girl" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
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

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( crystalg )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )		// OPTION (?)

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( crystal2 )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x0f, 0x0d, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0d, "1 (Easy)" )
	PORT_DIPSETTING(    0x0a, "2" )
	PORT_DIPSETTING(    0x09, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x07, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x05, "7" )
	PORT_DIPSETTING(    0x04, "8" )
	PORT_DIPSETTING(    0x00, "9 (Hard)" )
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

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x0c, 0x00, "SANGEN Rush" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "infinite" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )		// OPTION (?)

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END

INPUT_PORTS_START( apparel )

	// I don't have manual for this game.

	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE

	NBMJCTRL_PORT1	/* (3) PORT 1-1 */
	NBMJCTRL_PORT2	/* (4) PORT 1-2 */
	NBMJCTRL_PORT3	/* (5) PORT 1-3 */
	NBMJCTRL_PORT4	/* (6) PORT 1-4 */
	NBMJCTRL_PORT5	/* (7) PORT 1-5 */
INPUT_PORTS_END


static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip */
	20000000/8,		/* 2.50 MHz */
	{ 70 }
};

static struct AY8910interface ay8910_interface =
{
	1,				/* 1 chip */
	1250000,		/* 1.25 MHz ?? */
	{ 35 },
	{ input_port_0_r },		// DIPSW-A read
	{ input_port_1_r },		// DIPSW-B read
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,				/* 1 channels */
	{ 50 }
};



static MACHINE_DRIVER_START( NBMJDRV_4096 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 5000000)	/* 5.00 MHz */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nb1413m3)
	MDRV_NVRAM_HANDLER(nb1413m3)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 240-1)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_PALETTE_INIT(mbmj8688_12bit)
	MDRV_VIDEO_START(mbmj8688_pure_12bit)
	MDRV_VIDEO_UPDATE(mbmj8688)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("8910", AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( NBMJDRV_256 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_4096)

	/* video hardware */
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(mbmj8688_8bit)
	MDRV_VIDEO_START(mbmj8688_8bit)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( NBMJDRV_65536 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_4096)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2 | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(mbmj8688_16bit)
	MDRV_VIDEO_START(mbmj8688_hybrid_16bit)
MACHINE_DRIVER_END


// --------------------------------------------------------------------------------

static MACHINE_DRIVER_START( crystalg )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_256)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_secolove,writemem_secolove)
	MDRV_CPU_PORTS(readport_secolove,writeport_crystal2)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,96)	// nmiclock = 2f
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( apparel )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_256)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_secolove,writemem_secolove)
	MDRV_CPU_PORTS(readport_secolove,writeport_secolove)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)	// nmiclock = 60
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbmj_h12bit )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_4096)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_secolove,writemem_secolove)
	MDRV_CPU_PORTS(readport_secolove,writeport_secolove)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)	// nmiclock = 60

	/* video hardware */
	MDRV_VIDEO_START(mbmj8688_hybrid_12bit)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbmj_p16bit )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_65536)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_secolove,writemem_secolove)
	MDRV_CPU_PORTS(readport_secolove,writeport_secolove)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)	// nmiclock = 60/40

	/* video hardware */
	MDRV_VIDEO_START(mbmj8688_pure_16bit)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbmj_p16bit_LCD )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mbmj_p16bit)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(readport_secolove,writeport_p16bit_LCD)

	/* video hardware */
	MDRV_SCREEN_SIZE(512, 256 + 64*2)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 240-1 + 64*2)
	MDRV_ASPECT_RATIO(4*224,3*(224+64*2))

	MDRV_VIDEO_START(mbmj8688_pure_16bit_LCD)
	MDRV_VIDEO_UPDATE(mbmj8688_LCD)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( seiha )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_65536)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_secolove,writemem_secolove)
	MDRV_CPU_PORTS(readport_secolove,writeport_seiha)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)	// nmiclock = 60
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( iemoto )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_65536)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_secolove,writemem_secolove)
	MDRV_CPU_PORTS(readport_secolove,writeport_iemoto)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)	// nmiclock = 60
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ojousan )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_65536)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_ojousan,writemem_ojousan)
	MDRV_CPU_PORTS(readport_secolove,writeport_iemoto)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)	// nmiclock = 60
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbmj_p12bit )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_4096)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_mjsikaku,writemem_mjsikaku)
	MDRV_CPU_PORTS(readport_kaguya,writeport_kaguya)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mjsikaku )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(NBMJDRV_4096)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem_mjsikaku,writemem_mjsikaku)
	MDRV_CPU_PORTS(readport_mjsikaku,writeport_mjsikaku)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,144)	// nmiclock = 70

	/* sound hardware */
	MDRV_SOUND_REPLACE("8910", YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( otonano )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mjsikaku)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(readport_otonano,writeport_otonano)
MACHINE_DRIVER_END




ROM_START( crystalg )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "mbs1.3c",  0x00000, 0x04000, 0x1cacdbbd )
	ROM_LOAD( "mbs2.4c",  0x04000, 0x04000, 0xbf833674 )
	ROM_LOAD( "mbs3.5c",  0x08000, 0x04000, 0xfaacafd0 )
	ROM_LOAD( "mbs4.6c",  0x0c000, 0x04000, 0xb3bedcf1 )

	ROM_REGION( 0x080000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "ft1.1h",   0x000000, 0x08000, 0x99b982ea )
	ROM_LOAD( "ft2.2h",   0x008000, 0x08000, 0x026301da )
	ROM_LOAD( "ft3.3h",   0x010000, 0x08000, 0xbff22ef7 )
	ROM_LOAD( "ft4.4h",   0x018000, 0x08000, 0x4601e3a7 )
	ROM_LOAD( "ft5.5h",   0x020000, 0x08000, 0xe1388239 )
	ROM_LOAD( "ft6.6h",   0x028000, 0x08000, 0xda635046 )
	ROM_LOAD( "ft7.7h",   0x030000, 0x08000, 0xb4d2121b )
	ROM_LOAD( "ft8.8h",   0x038000, 0x08000, 0xb3fab376 )
	ROM_LOAD( "ft9.9h",   0x040000, 0x08000, 0x3d4102ca )
	ROM_LOAD( "ft10.10h", 0x048000, 0x08000, 0x264b6f7d )
ROM_END

ROM_START( crystal2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "cgl2_01.bin",  0x00000, 0x04000, 0x67673350 )
	ROM_LOAD( "cgl2_02.bin",  0x04000, 0x04000, 0x79c599d8 )
	ROM_LOAD( "cgl2_03.bin",  0x08000, 0x04000, 0xc11987ed )
	ROM_LOAD( "cgl2_04.bin",  0x0c000, 0x04000, 0xae0b7df8 )

	ROM_REGION( 0x080000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "cgl2_01s.bin", 0x000000, 0x08000, 0x99b982ea )
	ROM_LOAD( "cgl2_01m.bin", 0x008000, 0x08000, 0x7c7a0416 )
	ROM_LOAD( "cgl2_02m.bin", 0x010000, 0x08000, 0x8511ddcd )
	ROM_LOAD( "cgl2_03m.bin", 0x018000, 0x08000, 0xf594e3bc )
	ROM_LOAD( "cgl2_04m.bin", 0x020000, 0x08000, 0x01a6bf99 )
	ROM_LOAD( "cgl2_05m.bin", 0x028000, 0x08000, 0xee941bf6 )
	ROM_LOAD( "cgl2_06m.bin", 0x030000, 0x08000, 0x93a8bf3b )
	ROM_LOAD( "cgl2_07m.bin", 0x038000, 0x08000, 0xb9626199 )
	ROM_LOAD( "cgl2_08m.bin", 0x040000, 0x08000, 0x8a4d02c9 )
	ROM_LOAD( "cgl2_09m.bin", 0x048000, 0x08000, 0xe0d58e86 )
	ROM_LOAD( "cgl2_02s.bin", 0x050000, 0x08000, 0x7e0ca2a5 )
	ROM_LOAD( "cgl2_03s.bin", 0x058000, 0x08000, 0x78fc9502 )
	ROM_LOAD( "cgl2_04s.bin", 0x060000, 0x08000, 0xc2140826 )
	ROM_LOAD( "cgl2_05s.bin", 0x068000, 0x08000, 0x257df5f3 )
	ROM_LOAD( "cgl2_06s.bin", 0x070000, 0x08000, 0x27da3e4d )
	ROM_LOAD( "cgl2_07s.bin", 0x078000, 0x08000, 0xbd202788 )
ROM_END

ROM_START( apparel )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "11.bin", 0x00000, 0x04000, 0x31bd49d5 )
	ROM_LOAD( "12.bin", 0x04000, 0x04000, 0x56acd87d )
	ROM_LOAD( "13.bin", 0x08000, 0x04000, 0x3e2a9c66 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	// not used

	ROM_REGION( 0x0a0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1.bin",  0x000000, 0x10000, 0x6c7713ea )
	ROM_LOAD( "2.bin",  0x010000, 0x10000, 0x206f4d2c )
	ROM_LOAD( "3.bin",  0x020000, 0x10000, 0x5d8a732b )
	ROM_LOAD( "4.bin",  0x030000, 0x10000, 0xc40e4435 )
	ROM_LOAD( "5.bin",  0x040000, 0x10000, 0xe5bde704 )
	ROM_LOAD( "6.bin",  0x050000, 0x10000, 0x263673bc )
	ROM_LOAD( "7.bin",  0x060000, 0x10000, 0xc502dc5a )
	ROM_LOAD( "8.bin",  0x070000, 0x10000, 0xc0af5f0f )
	ROM_LOAD( "9.bin",  0x080000, 0x10000, 0x477b6cdd )
	ROM_LOAD( "10.bin", 0x090000, 0x10000, 0xd06d8972 )
ROM_END

ROM_START( citylove )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "14.12c", 0x00000, 0x08000, 0x2db5186c )
	ROM_LOAD( "13.11c", 0x08000, 0x08000, 0x52c7632b )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "11.8c",  0x00000, 0x08000, 0xeabb3f32 )
	ROM_LOAD( "12.10c", 0x08000, 0x08000, 0xc280f573 )

	ROM_REGION( 0xa0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1.1h",   0x00000, 0x10000, 0x55b911a3 )
	ROM_LOAD( "2.2h",   0x10000, 0x10000, 0x35298484 )
	ROM_LOAD( "3.4h",   0x20000, 0x10000, 0x6860c6d3 )
	ROM_LOAD( "4.5h",   0x30000, 0x10000, 0x21085a9a )
	ROM_LOAD( "5.7h",   0x40000, 0x10000, 0xfcf53e1a )
	ROM_LOAD( "6.1f",   0x50000, 0x10000, 0xdb11300c )
	ROM_LOAD( "7.2f",   0x60000, 0x10000, 0x57a90aac )
	ROM_LOAD( "8.4f",   0x70000, 0x10000, 0x58e1ad6f )
	ROM_LOAD( "9.5f",   0x80000, 0x10000, 0x242f07e9 )
	ROM_LOAD( "10.7f",  0x90000, 0x10000, 0xc032d8c3 )
ROM_END

ROM_START( secolove )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "slov_08.bin",  0x00000, 0x08000, 0x5aad556e )
	ROM_LOAD( "slov_07.bin",  0x08000, 0x08000, 0x94175129 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "slov_05.bin",  0x00000, 0x08000, 0xfa1debd9 )
	ROM_LOAD( "slov_06.bin",  0x08000, 0x08000, 0xa83be399 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "slov_01.bin",  0x000000, 0x10000, 0x9d792c34 )
	ROM_LOAD( "slov_02.bin",  0x010000, 0x10000, 0xb9671c88 )
	ROM_LOAD( "slov_03.bin",  0x020000, 0x10000, 0x5f57e4f2 )
	ROM_LOAD( "slov_04.bin",  0x030000, 0x10000, 0x4b0c700c )
	ROM_LOAD( "slov_c1.bin",  0x100000, 0x80000, 0x200170ba )
	ROM_LOAD( "slov_c2.bin",  0x180000, 0x80000, 0xdd5c23a1 )
ROM_END

ROM_START( housemnq )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4c",   0x00000, 0x08000, 0x465f61bb )
	ROM_LOAD( "2.3c",   0x08000, 0x08000, 0xe4499d02 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "3.5a",   0x00000, 0x10000, 0x141ce8b9 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1i.bin", 0x000000, 0x40000, 0x2199e3e9 )
	ROM_LOAD( "2i.bin", 0x040000, 0x40000, 0xf730ea47 )
	ROM_LOAD( "3i.bin", 0x080000, 0x40000, 0xf85c5b07 )
	ROM_LOAD( "4i.bin", 0x0c0000, 0x40000, 0x88f33049 )
	ROM_LOAD( "5i.bin", 0x100000, 0x40000, 0x77ba1eaf )
ROM_END

ROM_START( housemn2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "hmq2_01.bin",  0x00000, 0x08000, 0xa5aaf6c8 )
	ROM_LOAD( "hmq2_02.bin",  0x08000, 0x08000, 0x6bdcc867 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "hmq2_03.bin",  0x00000, 0x10000, 0xc08081d8 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "hmq2_c5.bin",  0x000000, 0x40000, 0x0263ff75 )
	ROM_LOAD( "hmq2_c1.bin",  0x040000, 0x40000, 0x788cd3ca )
	ROM_LOAD( "hmq2_c2.bin",  0x080000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "hmq2_c3.bin",  0x0c0000, 0x40000, 0xda46163e )
	ROM_LOAD( "hmq2_c4.bin",  0x100000, 0x40000, 0xea2b78b3 )
ROM_END

ROM_START( seiha )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "seiha1.4g",  0x00000, 0x08000, 0xad5ba5b5 )
	ROM_LOAD( "seiha2.3g",  0x08000, 0x08000, 0x0fe7a4b8 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "seiha03.3i",  0x00000, 0x10000, 0x2bcf3d87 )
	ROM_LOAD( "seiha04.2i",  0x10000, 0x10000, 0x2fc905d0 )
	ROM_LOAD( "seiha05.1i",  0x20000, 0x10000, 0x8eace19c )

	ROM_REGION( 0x280000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "seiha19.1a",  0x000000, 0x40000, 0x788cd3ca )
	ROM_LOAD( "seiha20.2a",  0x040000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "seiha21.3a",  0x080000, 0x40000, 0xda46163e )
	ROM_LOAD( "seiha22.4a",  0x0c0000, 0x40000, 0xea2b78b3 )
	ROM_LOAD( "seiha23.5a",  0x100000, 0x40000, 0x0263ff75 )
	ROM_LOAD( "seiha06.8a",  0x180000, 0x10000, 0x9fefe2ca )
	ROM_LOAD( "seiha07.9a",  0x190000, 0x10000, 0xa7d438ec )
	ROM_LOAD( "se1507.6a",   0x200000, 0x80000, 0xf1e9555e )
ROM_END

ROM_START( seiham )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "seih_01m.bin", 0x00000, 0x08000, 0x0c9a081b )
	ROM_LOAD( "seih_02m.bin", 0x08000, 0x08000, 0xa32cdb9a )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "seiha03.3i",   0x00000, 0x10000, 0x2bcf3d87 )
	ROM_LOAD( "seiha04.2i",   0x10000, 0x10000, 0x2fc905d0 )
	ROM_LOAD( "seiha05.1i",   0x20000, 0x10000, 0x8eace19c )

	ROM_REGION( 0x280000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "seiha19.1a",   0x000000, 0x40000, 0x788cd3ca )
	ROM_LOAD( "seiha20.2a",   0x040000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "seiha21.3a",   0x080000, 0x40000, 0xda46163e )
	ROM_LOAD( "seiha22.4a",   0x0c0000, 0x40000, 0xea2b78b3 )
	ROM_LOAD( "seiha23.5a",   0x100000, 0x40000, 0x0263ff75 )
	ROM_LOAD( "seiha06.8a",   0x180000, 0x10000, 0x9fefe2ca )
	ROM_LOAD( "seiha07.9a",   0x190000, 0x10000, 0xa7d438ec )
	ROM_LOAD( "seih_08m.bin", 0x1a0000, 0x10000, 0xe8e61e48 )
	ROM_LOAD( "se1507.6a",    0x200000, 0x80000, 0xf1e9555e )
ROM_END

ROM_START( bijokkoy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4c",   0x00000, 0x08000, 0x7dec7ae1 )
	ROM_LOAD( "2.3c",   0x08000, 0x08000, 0x3ae9650f )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "3.ic1",  0x00000, 0x10000, 0x221743b1 )
	ROM_LOAD( "4.ic2",  0x10000, 0x10000, 0x9f1f4461 )
	ROM_LOAD( "5.ic3",  0x20000, 0x10000, 0x6e7b3024 )
	ROM_LOAD( "6.ic4",  0x30000, 0x10000, 0x5e912211 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1h.bin", 0x000000, 0x40000, 0xda56ccac )
	ROM_LOAD( "2h.bin", 0x040000, 0x40000, 0x21c0227a )
	ROM_LOAD( "3h.bin", 0x080000, 0x40000, 0xaa66d9f3 )
	ROM_LOAD( "4h.bin", 0x0c0000, 0x40000, 0x5d10fb0a )
	ROM_LOAD( "5h.bin", 0x100000, 0x40000, 0xe22d6ca8 )
ROM_END

ROM_START( iemoto )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "iemoto1.4g",  0x00000, 0x08000, 0xab51f5c3 )
	ROM_LOAD( "iemoto2.3g",  0x08000, 0x08000, 0x873cd265 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "iemoto3.3i",  0x00000, 0x10000, 0x32d71ff9 )
	ROM_LOAD( "iemoto4.2i",  0x10000, 0x10000, 0x06f8e505 )
	ROM_LOAD( "iemoto5.1i",  0x20000, 0x10000, 0x261eb61a )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "iemoto31.1a", 0x000000, 0x40000, 0xba005a3a )
	ROM_LOAD( "iemoto32.2a", 0x040000, 0x40000, 0xfa9a74ae )
	ROM_LOAD( "iemoto33.3a", 0x080000, 0x40000, 0xefb13b61 )
	ROM_LOAD( "iemoto44.4a", 0x0c0000, 0x40000, 0x9acc54fa )
ROM_END

ROM_START( ojousan )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4g",    0x00000, 0x08000, 0xc0166351 )
	ROM_LOAD( "2.3g",    0x08000, 0x08000, 0x2c264eb2 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mask.3i", 0x00000, 0x10000, 0x59f355eb )
	ROM_LOAD( "mask.2i", 0x10000, 0x10000, 0x6f750500 )
	ROM_LOAD( "mask.1i", 0x20000, 0x10000, 0x4babcb40 )

	ROM_REGION( 0x1c0000, REGION_GFX1, 0 ) /* gfx */
	/* 000000-0fffff empty */
	ROM_LOAD( "3.5a",    0x100000, 0x20000, 0x3bdb9d2a )
	ROM_LOAD( "4.6a",    0x120000, 0x20000, 0x72b689b9 )
	ROM_LOAD( "5.7a",    0x140000, 0x20000, 0xe32e5e8a )
	ROM_LOAD( "6.8a",    0x160000, 0x20000, 0xf313337a )
	ROM_LOAD( "7.9a",    0x180000, 0x20000, 0xc2428e95 )
	ROM_LOAD( "8.10a",   0x1a0000, 0x20000, 0xf04c6003 )
ROM_END

ROM_START( bijokkog )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4c",    0x00000, 0x08000, 0x3c28b45c )
	ROM_LOAD( "2.3c",    0x08000, 0x08000, 0x396f6a05 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "3.ic1",   0x00000, 0x10000, 0xa92b1445 )
	ROM_LOAD( "4.ic2",   0x10000, 0x10000, 0x5127e958 )
	ROM_LOAD( "5.ic3",   0x20000, 0x10000, 0x6c717330 )
	ROM_LOAD( "6.ic4",   0x30000, 0x10000, 0xa3cf8d12 )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1s.bin",  0x000000, 0x10000, 0x9eadc3ea )
	ROM_LOAD( "2s.bin",  0x010000, 0x10000, 0x1161484c )
	ROM_LOAD( "3s.bin",  0x020000, 0x10000, 0x41f5dc43 )
	ROM_LOAD( "4s.bin",  0x030000, 0x10000, 0x3d9b79db )
	ROM_LOAD( "5s.bin",  0x040000, 0x10000, 0xeb54c3e3 )
	ROM_LOAD( "6s.bin",  0x050000, 0x10000, 0xd8deeea2 )
	ROM_LOAD( "7s.bin",  0x060000, 0x10000, 0xe42c67f1 )
	ROM_LOAD( "8s.bin",  0x070000, 0x10000, 0xcd11c78a )
	ROM_LOAD( "9s.bin",  0x080000, 0x10000, 0x2f3453a1 )
	ROM_LOAD( "10s.bin", 0x090000, 0x10000, 0xd80dd0b4 )
	ROM_LOAD( "11s.bin", 0x0a0000, 0x10000, 0xade64867 )
	ROM_LOAD( "12s.bin", 0x0b0000, 0x10000, 0x918a8f36 )
ROM_END

ROM_START( orangec )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "ft2.3c",     0x00000, 0x08000, 0x4ed413aa )
	ROM_LOAD( "ft1.2c",     0x08000, 0x08000, 0xf26bfd1b )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "ft3.5c",     0x00000, 0x10000, 0x2390a28b )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "ic5.bin",     0x000000, 0x10000, 0xe6fe4540 )
	ROM_LOAD( "ic6.bin",     0x010000, 0x10000, 0x343664f4 )
	ROM_LOAD( "ic7.bin",     0x020000, 0x10000, 0x5d5bcba8 )
	ROM_LOAD( "ic8.bin",     0x030000, 0x10000, 0x80ec6473 )
	ROM_LOAD( "ic9.bin",     0x040000, 0x10000, 0x30648437 )
	ROM_LOAD( "ic10.bin",    0x050000, 0x10000, 0x30e74967 )
	ROM_LOAD( "ic1.bin",     0x100000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "ic2.bin",     0x140000, 0x40000, 0xda46163e )
	ROM_LOAD( "ic3.bin",     0x180000, 0x40000, 0xefb13b61 )
	ROM_LOAD( "ic4.bin",     0x1c0000, 0x40000, 0x9acc54fa )
	ROM_LOAD( "ic6i.bin",    0x0f0000, 0x10000, 0x94bf4847 )
	ROM_LOAD( "ic7i.bin",    0x110000, 0x10000, 0x284f5648 )	// overlaps ic1!
ROM_END

ROM_START( vipclub )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "2.3c",       0x00000, 0x08000, 0x49acc59a )
	ROM_LOAD( "1.2c",       0x08000, 0x08000, 0x42101925 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "ft3.5c",     0x00000, 0x10000, 0x2390a28b )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "ic5.bin",     0x000000, 0x10000, 0xe6fe4540 )
	ROM_LOAD( "ic6.bin",     0x010000, 0x10000, 0x343664f4 )
	ROM_LOAD( "ic7_bin",     0x020000, 0x10000, 0x4811e122 )
	ROM_LOAD( "ic8.bin",     0x030000, 0x10000, 0x80ec6473 )
	ROM_LOAD( "ic9.bin",     0x040000, 0x10000, 0x30648437 )
	ROM_LOAD( "ic10.bin",    0x050000, 0x10000, 0x30e74967 )
	ROM_LOAD( "ic1.bin",     0x100000, 0x40000, 0xa3175a8f )
	ROM_LOAD( "ic2.bin",     0x140000, 0x40000, 0xda46163e )
	ROM_LOAD( "ic3.bin",     0x180000, 0x40000, 0xefb13b61 )
	ROM_LOAD( "ic4.bin",     0x1c0000, 0x40000, 0x9acc54fa )
	ROM_LOAD( "ic6i.bin",    0x0f0000, 0x10000, 0x94bf4847 )
	ROM_LOAD( "ic7i.bin",    0x110000, 0x10000, 0x284f5648 )	// overlaps ic1!
ROM_END

ROM_START( korinai )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.4g",       0x00000, 0x08000, 0xddcf787c )
	ROM_LOAD( "2.3g",       0x08000, 0x08000, 0x9bb992f5 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "3.3i",        0x00000, 0x10000, 0xd6fb023f )
	ROM_LOAD( "4.2i",        0x10000, 0x10000, 0x460917cf )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "5.2a",        0x000000, 0x20000, 0xf0732f3e )
	ROM_LOAD( "6.3a",        0x020000, 0x20000, 0x2b1da51e )
	ROM_LOAD( "7.4a",        0x040000, 0x20000, 0x85c260b9 )
	ROM_LOAD( "8.5a",        0x060000, 0x20000, 0x6a2763e1 )
	ROM_LOAD( "9.6a",        0x080000, 0x20000, 0x81287588 )
	ROM_LOAD( "10.7a",       0x0a0000, 0x20000, 0x9506d9cc )
	ROM_LOAD( "11.8a",       0x0c0000, 0x20000, 0x680d882e )
	ROM_LOAD( "12.9a",       0x0e0000, 0x20000, 0x41a25dfe )
	ROM_LOAD( "13.10a",      0x100000, 0x10000, 0x7dc27aa9 )
	ROM_LOAD( "se-1507.1a",  0x180000, 0x80000, 0xf1e9555e )
ROM_END

ROM_START( kaguya )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "kaguya01.bin", 0x00000, 0x10000, 0x6ac18c32 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "kaguya02.bin", 0x00000, 0x10000, 0x561dc656 )
	ROM_LOAD( "kaguya03.bin", 0x10000, 0x10000, 0xa09e9387 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "kaguya04.bin", 0x000000, 0x20000, 0xccd08d8d )
	ROM_LOAD( "kaguya05.bin", 0x020000, 0x20000, 0xa3abc686 )
	ROM_LOAD( "kaguya06.bin", 0x040000, 0x20000, 0x6accd6d3 )
	ROM_LOAD( "kaguya07.bin", 0x060000, 0x20000, 0x93c64846 )
	ROM_LOAD( "kaguya08.bin", 0x080000, 0x20000, 0xf0ad7c6c )
	ROM_LOAD( "kaguya09.bin", 0x0a0000, 0x20000, 0xf33fefdf )
	ROM_LOAD( "kaguya10.bin", 0x0c0000, 0x20000, 0x741d13f6 )
	ROM_LOAD( "kaguya11.bin", 0x0e0000, 0x20000, 0xfcbede4f )
ROM_END

ROM_START( kanatuen )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "11.3f", 0x00000, 0x10000, 0x3345d977 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "12.3k", 0x00000, 0x10000, 0xa4424adc )
	/* protection data is mapped at 30000-3ffff */

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "1.6a",  0x000000, 0x20000, 0xa62b5982 )
	ROM_LOAD( "2.6bc", 0x020000, 0x20000, 0xfd36dcae )
	ROM_LOAD( "3.6d",  0x040000, 0x10000, 0x7636cbde )
	ROM_LOAD( "4.6e",  0x050000, 0x10000, 0xed9c7744 )
	ROM_LOAD( "5.6f",  0x060000, 0x10000, 0xd54cd45d )
	ROM_LOAD( "6.6h",  0x070000, 0x10000, 0x1a0fbf52 )
	ROM_LOAD( "7.6k",  0x080000, 0x10000, 0xea0c45f5 )
	ROM_LOAD( "8.6l",  0x090000, 0x10000, 0x8754fc38 )
	ROM_LOAD( "9.6m",  0x0a0000, 0x10000, 0x51437563 )
	ROM_LOAD( "10.6p", 0x0b0000, 0x10000, 0x1447ed65 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* protection data */
	ROM_LOAD( "mask.bin", 0x00000, 0x40000, 0xf85c5b07 )	// same as housemnq/3i.bin gfx data
ROM_END

ROM_START( mjsikaku )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "mjsk_01.bin", 0x00000, 0x10000, 0x6b64c96a )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mjsk_02.bin", 0x00000, 0x10000, 0xcc0262bb )
	ROM_LOAD( "mjsk_03.bin", 0x10000, 0x10000, 0x7dedcd75 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "mjsk_04.bin", 0x000000, 0x20000, 0x34d13d1e )
	ROM_LOAD( "mjsk_05.bin", 0x020000, 0x20000, 0x8c70aed5 )
	ROM_LOAD( "mjsk_06.bin", 0x040000, 0x20000, 0x1dad8355 )
	ROM_LOAD( "mjsk_07.bin", 0x060000, 0x20000, 0x8174a28a )
	ROM_LOAD( "mjsk_08.bin", 0x080000, 0x20000, 0x3e182aaa )
	ROM_LOAD( "mjsk_09.bin", 0x0a0000, 0x20000, 0xa17a3328 )
	ROM_LOAD( "mjsk_10.bin", 0x0c0000, 0x10000, 0xcab4909f )
	ROM_LOAD( "mjsk_11.bin", 0x0d0000, 0x10000, 0xdd7a95c8 )
	ROM_LOAD( "mjsk_12.bin", 0x0e0000, 0x10000, 0x20c25377 )
	ROM_LOAD( "mjsk_13.bin", 0x0f0000, 0x10000, 0x967e9a91 )
ROM_END

ROM_START( mjsikakb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "sikaku.1",    0x00000, 0x10000, 0x66349663 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "mjsk_02.bin", 0x00000, 0x10000, 0xcc0262bb )
	ROM_LOAD( "mjsk_03.bin", 0x10000, 0x10000, 0x7dedcd75 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "mjsk_04.bin", 0x000000, 0x20000, 0x34d13d1e )
	ROM_LOAD( "mjsk_05.bin", 0x020000, 0x20000, 0x8c70aed5 )
	ROM_LOAD( "mjsk_06.bin", 0x040000, 0x20000, 0x1dad8355 )
	ROM_LOAD( "mjsk_07.bin", 0x060000, 0x20000, 0x8174a28a )
	ROM_LOAD( "mjsk_08.bin", 0x080000, 0x20000, 0x3e182aaa )
	ROM_LOAD( "mjsk_09.bin", 0x0a0000, 0x20000, 0xa17a3328 )
	ROM_LOAD( "sikaku.10",   0x0c0000, 0x20000, 0xf91757bc )
	ROM_LOAD( "sikaku.11",   0x0e0000, 0x20000, 0xabd280b6 )
ROM_END

ROM_START( otonano )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "otona_01.bin", 0x00000, 0x10000, 0xee629b72 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "otona_02.bin", 0x00000, 0x10000, 0x2864b8ef )
	ROM_LOAD( "otona_03.bin", 0x10000, 0x10000, 0xece880e0 )
	ROM_LOAD( "otona_04.bin", 0x20000, 0x10000, 0x5a25b251 )
	ROM_LOAD( "otona_05.bin", 0x30000, 0x10000, 0x469d580d )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "otona_06.bin", 0x000000, 0x20000, 0x2d41f854 )
	ROM_LOAD( "otona_07.bin", 0x020000, 0x20000, 0x58d6717d )
	ROM_LOAD( "otona_08.bin", 0x040000, 0x20000, 0x40f8d432 )
	ROM_LOAD( "otona_09.bin", 0x060000, 0x20000, 0xfd80fdc2 )
	ROM_LOAD( "otona_10.bin", 0x080000, 0x20000, 0x50ff867a )
	ROM_LOAD( "otona_11.bin", 0x0a0000, 0x20000, 0xc467e822 )
	ROM_LOAD( "otona_12.bin", 0x0c0000, 0x20000, 0x1a0f9250 )
	ROM_LOAD( "otona_13.bin", 0x0e0000, 0x20000, 0x208dee43 )
ROM_END

ROM_START( mjcamera )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "mcam_01.bin", 0x00000, 0x10000, 0x73d4b9ff )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "1.2k",        0x00000, 0x10000, 0xfe8e975e )
	/* protection data is mapped at 20000-2ffff */

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "3.8c",        0x000000, 0x10000, 0x273fb8bc )
	ROM_LOAD( "4.8d",        0x010000, 0x10000, 0x82995399 )
	ROM_LOAD( "5.8e",        0x020000, 0x10000, 0xa7c51d54 )
	ROM_LOAD( "6.8f",        0x030000, 0x10000, 0xf221700c )
	ROM_LOAD( "7.8h",        0x040000, 0x10000, 0x6baa4d45 )
	ROM_LOAD( "8.8k",        0x050000, 0x10000, 0x91d9c868 )
	ROM_LOAD( "9.8l",        0x060000, 0x10000, 0x56a35d4b )
	ROM_LOAD( "10.8m",       0x070000, 0x10000, 0x480e23c4 )
	ROM_LOAD( "11.8n",       0x080000, 0x10000, 0x2c29accc )
	ROM_LOAD( "12.8p",       0x090000, 0x10000, 0x902d73f8 )
	ROM_LOAD( "13.10c",      0x0a0000, 0x10000, 0xfcba0179 )
	ROM_LOAD( "14.10d",      0x0b0000, 0x10000, 0xee2c37a9 )
	ROM_LOAD( "15.10e",      0x0c0000, 0x10000, 0x90fd36f8 )
	ROM_LOAD( "16.10f",      0x0d0000, 0x10000, 0x41265f7f )
	ROM_LOAD( "17.10h",      0x0e0000, 0x10000, 0x78cef468 )
	ROM_LOAD( "mcam_18.bin", 0x0f0000, 0x10000, 0x3a3da341 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* protection data */
	ROM_LOAD( "mcam_m1.bin", 0x00000, 0x40000, 0xf85c5b07 )	// same as housemnq/3i.bin gfx data
ROM_END

ROM_START( idhimitu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "1.3f",   0x00000, 0x10000, 0x619f9465 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "2.3k",   0x00000, 0x10000, 0x9a5f7907 )
	/* protection data is mapped at 20000-2ffff */

	ROM_REGION( 0x0e0000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "3.6a",   0x000000, 0x10000, 0x4677f0d0 )
	ROM_LOAD( "4.6b",   0x010000, 0x10000, 0xf935a681 )
	ROM_LOAD( "8.6h",   0x050000, 0x10000, 0xf03768b0 )
	ROM_LOAD( "9.6k",   0x060000, 0x10000, 0x04918709 )
	ROM_LOAD( "10.6l",  0x070000, 0x10000, 0xae95e5e2 )
	ROM_LOAD( "11.6m",  0x080000, 0x10000, 0xf9865cf3 )
	ROM_LOAD( "12.6p",  0x090000, 0x10000, 0x99545a6b )
	ROM_LOAD( "13.7a",  0x0a0000, 0x10000, 0x60472080 )
	ROM_LOAD( "14.7b",  0x0b0000, 0x10000, 0x3e26e374 )
	ROM_LOAD( "15.7d",  0x0c0000, 0x10000, 0x9e303eda )
	ROM_LOAD( "16.7e",  0x0d0000, 0x10000, 0x0429ae8f )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* protection data */
	ROM_LOAD( "ic3m.bin",   0x00000, 0x40000, 0xba005a3a )	// same as iemoto/iemoto31.1a gfx data
ROM_END


/* 8-bit palette */
GAMEX(1986, crystalg, 0,        crystalg,        crystalg, crystalg, ROT0, "Nichibutsu", "Crystal Gal (Japan 860512)", GAME_NOT_WORKING )
GAMEX(1986, crystal2, 0,        crystalg,        crystal2, crystal2, ROT0, "Nichibutsu", "Crystal Gal 2 (Japan 860620)", GAME_NOT_WORKING )
GAME( 1986, apparel,  0,        apparel,         apparel,  apparel,  ROT0, "Central Denshi", "Apparel Night (Japan 860929)" )

/* hybrid 12-bit palette */
GAME( 1986, citylove, 0,        mbmj_h12bit,     citylove, citylove, ROT0, "Nichibutsu", "City Love (Japan 860908)" )
GAME( 1986, secolove, 0,        mbmj_h12bit,     secolove, secolove, ROT0, "Nichibutsu", "Second Love (Japan 861201)" )

/* hybrid 16-bit palette */
GAME( 1987, seiha,    0,        seiha,           seiha,    seiha,    ROT0, "Nichibutsu", "Seiha (Japan 870725)" )
GAME( 1987, seiham,   seiha,    seiha,           seiham,   seiham,   ROT0, "Nichibutsu", "Seiha [BET] (Japan 870723)" )
GAME( 1987, iemoto,   0,        iemoto,          iemoto,   iemoto,   ROT0, "Nichibutsu", "Iemoto (Japan 871020)" )
GAME( 1987, ojousan,  0,        ojousan,         ojousan,  ojousan,  ROT0, "Nichibutsu", "Ojousan (Japan 871204)" )
GAME( 1988, korinai,  0,        ojousan,         korinai,  korinai,  ROT0, "Nichibutsu", "Mahjong-zukino Korinai Menmen (Japan 880425)" )

/* pure 16-bit palette (+ LCD in some) */
GAME( 1987, housemnq, 0,        mbmj_p16bit_LCD, housemnq, housemnq, ROT0, "Nichibutsu", "House Mannequin (Japan 870217)" )
GAME( 1987, housemn2, 0,        mbmj_p16bit_LCD, housemn2, housemn2, ROT0, "Nichibutsu", "House Mannequin Roppongi Live hen (Japan 870418)" )
GAME( 1987, bijokkoy, 0,        mbmj_p16bit_LCD, bijokkoy, bijokkoy, ROT0, "Nichibutsu", "Bijokko Yume Monogatari (Japan 870925)" )
GAME( 1988, bijokkog, 0,        mbmj_p16bit_LCD, bijokkog, bijokkog, ROT0, "Nichibutsu", "Bijokko Gakuen (Japan 880116)" )
GAME( 1988, orangec,  0,        mbmj_p16bit,     orangec,  orangec,  ROT0, "Daiichi Denshi", "Orange Club - Maruhi Kagai Jugyou (Japan 880213)" )
GAME( 1988, vipclub,  orangec,  mbmj_p16bit,     vipclub,  orangec,  ROT0, "Daiichi Denshi", "Vip Club [BET] (Japan 880310)" )

/* pure 12-bit palette */
GAME( 1988, kaguya,   0,        mbmj_p12bit,     kaguya,   kaguya,   ROT0, "MIKI SYOUJI", "Mahjong Kaguyahime [BET] (Japan 880521)" )
GAME( 1988, kanatuen, 0,        mbmj_p12bit,     kanatuen, kanatuen, ROT0, "Panac", "Kanatsuen no Onna [BET] (Japan 880905)" )
GAME( 1989, idhimitu, 0,        mbmj_p12bit,     idhimitu, idhimitu, ROT0, "Digital Soft", "Idol no Himitsu [BET] (Japan 890304)" )

/* pure 12-bit palette + YM3812 instead of AY-3-8910 */
GAME( 1988, mjsikaku, 0,        mjsikaku,        mjsikaku, mjsikaku, ROT0, "Nichibutsu", "Mahjong Sikaku (Japan 880908)" )
GAME( 1988, mjsikakb, mjsikaku, mjsikaku,        mjsikaku, mjsikaku, ROT0, "Nichibutsu", "Mahjong Sikaku (Japan 880722)" )
GAME( 1988, otonano,  0,        otonano,         otonano,  otonano,  ROT0, "Apple", "Otona no Mahjong (Japan 880628)" )
GAME( 1988, mjcamera, 0,        otonano,         mjcamera, mjcamera, ROT0, "MIKI SYOUJI", "Mahjong Camera Kozou (Japan 881109)" )

