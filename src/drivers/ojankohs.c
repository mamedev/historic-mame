/******************************************************************************

	Game Driver for Video System Mahjong series.

	Ojanko High School
	(c)1988 Video System Co.,Ltd.

	Ojanko Yakata
	(c)1986 Video System Co.,Ltd.

	Ojanko Yakata 2bankan
	(c)1987 Video System Co.,Ltd.

	Chinese Casino [BET] (Japan)
	(c)1987 Video System Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/06/10 -

******************************************************************************/
/******************************************************************************
Memo:

- Sometimes RAM check in testmode fails (reason unknown).

- The method to get matrix key data may be incorrect.
  2player's input is not supported.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"


VIDEO_UPDATE( ojankohs );
PALETTE_INIT( ojankoy );
VIDEO_START( ojankohs );
VIDEO_START( ojankoy );
READ_HANDLER( ojankohs_palette_r );
WRITE_HANDLER( ojankohs_palette_w );
READ_HANDLER( ojankohs_videoram_r );
WRITE_HANDLER( ojankohs_videoram_w );
READ_HANDLER( ojankohs_colorram_r );
WRITE_HANDLER( ojankohs_colorram_w );
WRITE_HANDLER( ojankohs_gfxreg_w );
WRITE_HANDLER( ojankohs_flipscreen_w );


static int ojankohs_portselect;
static int ojankohs_adpcm_reset;
static int ojankohs_adpcm_data;
static int ojankohs_vclk_left;


static MACHINE_INIT( ojankohs )
{
	ojankohs_portselect = 0;

	ojankohs_adpcm_reset = 0;
	ojankohs_adpcm_data = 0;
	ojankohs_vclk_left = 0;
}

static WRITE_HANDLER( ojankohs_rombank_w )
{
	unsigned char *ROM = memory_region(REGION_CPU1);

	cpu_setbank(1, &ROM[0x10000 + (0x4000 * (data & 0x3f))]);
}

static WRITE_HANDLER( ojankoy_rombank_w )
{
	unsigned char *ROM = memory_region(REGION_CPU1);

	cpu_setbank(1, &ROM[0x10000 + (0x4000 * (data & 0x1f))]);

	ojankohs_adpcm_reset = ((data & 0x20) >> 5);
	if (!ojankohs_adpcm_reset) ojankohs_vclk_left = 0;

	MSM5205_reset_w(0, !ojankohs_adpcm_reset);
}

static WRITE_HANDLER( ojankohs_adpcm_reset_w )
{
	ojankohs_adpcm_reset = (data & 0x01);
	ojankohs_vclk_left = 0;

	MSM5205_reset_w(0, !ojankohs_adpcm_reset);
}

static WRITE_HANDLER( ojankohs_msm5205_w )
{
	ojankohs_adpcm_data = data;
	ojankohs_vclk_left = 2;
}

static void ojankohs_adpcm_int(int irq)
{
	/* skip if we're reset */
	if (!ojankohs_adpcm_reset)
		return;

	/* clock the data through */
	if (ojankohs_vclk_left)
	{
		MSM5205_data_w(0, (ojankohs_adpcm_data >> 4));
		ojankohs_adpcm_data <<= 4;
		ojankohs_vclk_left--;
	}

	/* generate an NMI if we're out of data */
	if (!ojankohs_vclk_left)
		cpu_set_nmi_line(0, PULSE_LINE);
}


static MEMORY_READ_START( readmem_ojankohs )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, ojankohs_videoram_r },
	{ 0x9000, 0x9fff, ojankohs_colorram_r },
	{ 0xa000, 0xb7ff, MRA_RAM },
	{ 0xb800, 0xbfff, ojankohs_palette_r },
	{ 0xc000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( writemem_ojankohs )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, ojankohs_videoram_w },
	{ 0x9000, 0x9fff, ojankohs_colorram_w },
	{ 0xa000, 0xb7ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0xb800, 0xbfff, ojankohs_palette_w },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( readmem_ojankoy )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, ojankohs_videoram_r },
	{ 0xa000, 0xafff, ojankohs_colorram_r },
	{ 0xb000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( writemem_ojankoy )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, ojankohs_videoram_w },
	{ 0xa000, 0xafff, ojankohs_colorram_w },
	{ 0xb000, 0xbfff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END


static WRITE_HANDLER( ojankohs_portselect_w )
{
	ojankohs_portselect = data;
}

static READ_HANDLER( ojankohs_keymatrix_r )
{
	int ret;

	switch (ojankohs_portselect)
	{
		case	0x01:	ret = readinputport(4);	break;
		case	0x02:	ret = readinputport(5); break;
		case	0x04:	ret = readinputport(6); break;
		case	0x08:	ret = readinputport(7); break;
		case	0x10:	ret = readinputport(8); break;
		case	0x20:	ret = 0xff; break;
		case	0x3f:	ret = 0xff;
				ret &= readinputport(4);
				ret &= readinputport(5);
				ret &= readinputport(6);
				ret &= readinputport(7);
				ret &= readinputport(8);
				break;
		default:	ret = 0xff;
				logerror("PC:%04X unknown %02X\n", activecpu_get_pc(), ojankohs_portselect);
				break;
	}

	return ret;
}

static READ_HANDLER( ojankohs_ay8910_0_r )
{
	// DIPSW 2
	return (((readinputport(2) & 0x01) << 7) | ((readinputport(2) & 0x02) << 5) |
	        ((readinputport(2) & 0x04) << 3) | ((readinputport(2) & 0x08) << 1) |
	        ((readinputport(2) & 0x10) >> 1) | ((readinputport(2) & 0x20) >> 3) |
	        ((readinputport(2) & 0x40) >> 5) | ((readinputport(2) & 0x80) >> 7));
}

static READ_HANDLER( ojankohs_ay8910_1_r )
{
	// DIPSW 1
	return (((readinputport(3) & 0x01) << 7) | ((readinputport(3) & 0x02) << 5) |
	        ((readinputport(3) & 0x04) << 3) | ((readinputport(3) & 0x08) << 1) |
	        ((readinputport(3) & 0x10) >> 1) | ((readinputport(3) & 0x20) >> 3) |
	        ((readinputport(3) & 0x40) >> 5) | ((readinputport(3) & 0x80) >> 7));
}

static READ_HANDLER( ojankoy_ay8910_0_r )
{
	// DIPSW 2
	return readinputport(2);
}

static READ_HANDLER( ojankoy_ay8910_1_r )
{
	// DIPSW 1
	return readinputport(3);
}

static WRITE_HANDLER( ojankoy_coinctr_w )
{
	coin_counter_w( 0, (data & 0x01));
}

static WRITE_HANDLER( ccasino_coinctr_w )
{
	coin_counter_w(0, (data & 0x02));
}


static PORT_READ_START( readport_ojankohs )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, ojankohs_keymatrix_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x06, 0x06, AY8910_read_port_0_r },
PORT_END

static PORT_WRITE_START( writeport_ojankohs )
	{ 0x00, 0x00, ojankohs_portselect_w },
	{ 0x01, 0x01, ojankohs_rombank_w },
	{ 0x02, 0x02, ojankohs_gfxreg_w },
	{ 0x03, 0x03, ojankohs_adpcm_reset_w },
	{ 0x04, 0x04, ojankohs_flipscreen_w },
	{ 0x05, 0x05, ojankohs_msm5205_w },
	{ 0x06, 0x06, AY8910_write_port_0_w },
	{ 0x07, 0x07, AY8910_control_port_0_w },
	{ 0x10, 0x10, IOWP_NOP },				// unknown
	{ 0x11, 0x11, IOWP_NOP },				// unknown
PORT_END

static PORT_WRITE_START( writeport_ojankoy )
	{ 0x00, 0x00, ojankohs_portselect_w },
	{ 0x01, 0x01, ojankoy_rombank_w },
	{ 0x02, 0x02, ojankoy_coinctr_w },
	{ 0x04, 0x04, ojankohs_flipscreen_w },
	{ 0x05, 0x05, ojankohs_msm5205_w },
	{ 0x06, 0x06, AY8910_write_port_0_w },
	{ 0x07, 0x07, AY8910_control_port_0_w },
PORT_END

static READ_HANDLER( io_ccasino_r )
{
	int ret;

	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	switch (offset & 0xff00)
	{
		case	0x0000:	ret = input_port_0_r(0); break;
		case	0x0100:	ret = ojankohs_keymatrix_r(0); break;
		case	0x0200:	ret = input_port_1_r(0); break;
		case	0x0300:	ret = (readinputport(9) ^ 0xff); break;		// DIPSW 3
		case	0x0400:	ret = (readinputport(10) ^ 0xff); break;	// DIPSW 4
		case	0x0600:	ret = AY8910_read_port_0_r(0); break;
		default:	ret = 0xff;
				logerror("PC:%04X unknown I/O Read %02X\n", activecpu_get_pc(), ((offset & 0xff00) >> 8));
				break;
	}

	return ret;
}

static PORT_READ_START( readport_ccasino )
	{ 0x0000, 0xffff, io_ccasino_r },
PORT_END

static WRITE_HANDLER( io_ccasino_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0x0800 <= offset) && (0x1000 > offset))
	{
		ojankohs_palette_w((offset & 0x07ff), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	ojankohs_portselect_w(0, data); break;
		case	0x0100:	ojankohs_rombank_w(0, data); break;
		case	0x0200:	ccasino_coinctr_w(0, data); break;
		case	0x0300:	ojankohs_adpcm_reset_w(0, data); break;
		case	0x0400:	ojankohs_flipscreen_w(0, data); break;
		case	0x0500:	ojankohs_msm5205_w(0, data); break;
		case	0x0600:	AY8910_write_port_0_w(0, data); break;
		case	0x0700:	AY8910_control_port_0_w(0, data); break;
		case	0x1000:	break;				// unknown
		case	0x1100:	break;				// unknown
		default:	logerror("PC:%04X unknown I/O Write %02X, %02X\n", activecpu_get_pc(), ((offset & 0xff00) >> 8), data);
				break;
	}
}

static PORT_WRITE_START( writeport_ccasino )
	{ 0x0000, 0xffff, io_ccasino_w },
PORT_END


INPUT_PORTS_START( ojankohs )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) COIN SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) DIPSW-1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x06, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x04, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING (   0x20, "1" )
	PORT_DIPSETTING (   0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (3) DIPSW-2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x02, "Opponent's initial score" )
	PORT_DIPSETTING (   0x02, "2000" )
	PORT_DIPSETTING (   0x00, "3000" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (4) PORT 1-0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (5) PORT 1-1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet", KEYCODE_2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (6) PORT 1-2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (7) PORT 1-3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (8) PORT 1-4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance", KEYCODE_RALT, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ojankoy )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) COIN SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) DIPSW-1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x06, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x04, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x18, 0x18, "Player's initial score" )
	PORT_DIPSETTING(    0x18, "1000" )
	PORT_DIPSETTING(    0x08, "2000" )
	PORT_DIPSETTING(    0x10, "3000" )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPNAME( 0x60, 0x60, "Noten penalty after ryukyoku" )
	PORT_DIPSETTING(    0x60, "1000" )
	PORT_DIPSETTING(    0x20, "2000" )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x03, 0x02, "Number of ending chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x04, "Ending chance requires fee" )
	PORT_DIPSETTING (   0x04, DEF_STR( No ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x60, 0x60, "Opponent's initial score" )
	PORT_DIPSETTING (   0x60, "3000 - 8000" )
	PORT_DIPSETTING (   0x20, "5000 - 10000" )
	PORT_DIPSETTING (   0x40, "8000" )
	PORT_DIPSETTING (   0x00, "10000" )
	PORT_DIPNAME( 0x80, 0x00, "Gal select / Continue" )
	PORT_DIPSETTING(    0x80, "Yes / No" )
	PORT_DIPSETTING(    0x00, "No / Yes" )

	PORT_START	/* (4) PORT 1-0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (5) PORT 1-1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet", KEYCODE_2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (6) PORT 1-2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (7) PORT 1-3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (8) PORT 1-4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance", KEYCODE_RALT, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ccasino )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2 )		// ANALYZER
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) COIN SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) DIPSW-1 */
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

	PORT_START	/* (3) DIPSW-2 */
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

	PORT_START	/* (4) PORT 1-0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (5) PORT 1-1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet", KEYCODE_2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (6) PORT 1-2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (7) PORT 1-3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (8) PORT 1-4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance", KEYCODE_RALT, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (9) DIPSW-3 */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 3-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 3-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 3-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 3-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 3-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 3-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 3-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 3-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (10) DIPSW-4 */
	PORT_DIPNAME( 0x01, 0x01, "DIPSW 4-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIPSW 4-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 4-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 4-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 4-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 4-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 4-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 4-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout ojankohs_bglayout =
{
	8, 4,
	RGN_FRAC(1, 1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	16*8
};

static struct GfxDecodeInfo ojankohs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &ojankohs_bglayout,   0, 64 },
	{ -1 } /* end of array */
};


static struct AY8910interface ojankohs_ay8910_interface =
{
	1,				/* 1 chip */
	12000000/6,			/* 2 MHz ? */
	{ 15 },				/* volume */
	{ ojankohs_ay8910_0_r },	/* read port #0 */
	{ ojankohs_ay8910_1_r },	/* read port #1 */
	{ 0 },				/* write port #0 */
	{ 0 }				/* write port #1 */
};

static struct AY8910interface ojankoy_ay8910_interface =
{
	1,				/* 1 chip */
	12000000/8,			/* 1.5 MHz ? */
	{ 15 },				/* volume */
	{ ojankoy_ay8910_0_r },		/* read port #0 */
	{ ojankoy_ay8910_1_r },		/* read port #1 */
	{ 0 },				/* write port #0 */
	{ 0 }				/* write port #1 */
};

static struct MSM5205interface msm5205_interface =
{
	1,				/* 1 chip */
	384000,				/* 384 KHz */
	{ ojankohs_adpcm_int },		/* IRQ handler */
	{ MSM5205_S48_4B },		/* 8 KHz */
	{ 65 }				/* volume */
};


static MACHINE_DRIVER_START( ojankohs )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 MHz ? */
	MDRV_CPU_MEMORY(readmem_ojankohs,writemem_ojankohs)
	MDRV_CPU_PORTS(readport_ojankohs,writeport_ojankohs)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(ojankohs)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 288-1, 0, 224-1)
	MDRV_GFXDECODE(ojankohs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(ojankohs)
	MDRV_VIDEO_UPDATE(ojankohs)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ojankohs_ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ojankoy )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 MHz ? */
	MDRV_CPU_MEMORY(readmem_ojankoy,writemem_ojankoy)
	MDRV_CPU_PORTS(readport_ojankohs,writeport_ojankoy)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(ojankohs)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 288-1, 0, 224-1)
	MDRV_GFXDECODE(ojankohs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_PALETTE_INIT(ojankoy)

	MDRV_VIDEO_START(ojankoy)
	MDRV_VIDEO_UPDATE(ojankohs)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ojankoy_ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ccasino )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)		/* 6.00 MHz ? */
	MDRV_CPU_MEMORY(readmem_ojankoy,writemem_ojankoy)
	MDRV_CPU_PORTS(readport_ccasino,writeport_ccasino)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(ojankohs)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 288-1, 0, 224-1)
	MDRV_GFXDECODE(ojankohs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(ojankoy)
	MDRV_VIDEO_UPDATE(ojankohs)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ojankoy_ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


ROM_START( ojankohs )
	ROM_REGION( 0x98000, REGION_CPU1, 0 )
	ROM_LOAD( "3.3c", 0x00000, 0x08000, 0xf652db23 )
	ROM_LOAD( "5b",   0x10000, 0x80000, 0xbd4fd0b6 )
	ROM_LOAD( "6.6c", 0x90000, 0x08000, 0x30772679 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "13b",  0x00000, 0x80000, 0xbda30bfa )
ROM_END

ROM_START( ojankoy )
	ROM_REGION( 0x70000, REGION_CPU1, 0 )
	ROM_LOAD( "p-ic17.bin", 0x00000, 0x08000, 0x9f149c30 )
	ROM_LOAD( "ic30.bin",   0x10000, 0x20000, 0x37be3f7c )
	ROM_LOAD( "ic29.bin",   0x30000, 0x20000, 0xdab7c4d8 )
	ROM_LOAD( "a-ic34.bin", 0x50000, 0x08000, 0x93c20ea3 )
	ROM_LOAD( "b-ic33.bin", 0x58000, 0x08000, 0xef86d711 )
	ROM_LOAD( "c-ic32.bin", 0x60000, 0x08000, 0xd20de9b0 )
	ROM_LOAD( "d-ic31.bin", 0x68000, 0x08000, 0xb78e6913 )

	ROM_REGION( 0x70000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic55.bin",   0x00000, 0x20000, 0x586fb385 )
	ROM_LOAD( "0-ic53.bin", 0x40000, 0x08000, 0xdb38c288 )
	ROM_LOAD( "1-ic52.bin", 0x48000, 0x08000, 0xa8b4a10b )
	ROM_LOAD( "2-ic51.bin", 0x50000, 0x08000, 0x5e2bb752 )
	ROM_LOAD( "3-ic50.bin", 0x58000, 0x08000, 0x10c73a44 )
	ROM_LOAD( "4-ic49.bin", 0x60000, 0x08000, 0x31807d24 )
	ROM_LOAD( "5-ic48.bin", 0x68000, 0x08000, 0xe116721d )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "0-ic65.bin", 0x0000, 0x0400, 0x28fde5ef )
	ROM_LOAD( "1-ic64.bin", 0x0400, 0x0400, 0x36c305c5 )
ROM_END

ROM_START( ojanko2 )
	ROM_REGION( 0x70000, REGION_CPU1, 0 )
	ROM_LOAD( "p-ic17.bin", 0x00000, 0x08000, 0x4b33bd54 )
	ROM_LOAD( "ic30.bin",   0x10000, 0x20000, 0x37be3f7c )
	ROM_LOAD( "ic29.bin",   0x30000, 0x20000, 0xdab7c4d8 )
	ROM_LOAD( "a-ic34.bin", 0x50000, 0x08000, 0x93c20ea3 )
	ROM_LOAD( "b-ic33.bin", 0x58000, 0x08000, 0xef86d711 )
	ROM_LOAD( "c-ic32.bin", 0x60000, 0x08000, 0x5453b9de )
	ROM_LOAD( "d-ic31.bin", 0x68000, 0x08000, 0x44cd5348 )

	ROM_REGION( 0x70000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic55.bin",   0x00000, 0x20000, 0xb058fb3d )
	ROM_LOAD( "0-ic53.bin", 0x40000, 0x08000, 0xdb38c288 )
	ROM_LOAD( "1-ic52.bin", 0x48000, 0x08000, 0x49f2ca73 )
	ROM_LOAD( "2-ic51.bin", 0x50000, 0x08000, 0x199a9bfb )
	ROM_LOAD( "3-ic50.bin", 0x58000, 0x08000, 0xf175510e )
	ROM_LOAD( "4-ic49.bin", 0x60000, 0x08000, 0x3a6a9685 )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "0-ic65.bin", 0x0000, 0x0400, 0x86e19b01 )
	ROM_LOAD( "1-ic64.bin", 0x0400, 0x0400, 0xe2f7093d )
ROM_END

ROM_START( ccasino )
	ROM_REGION( 0x68000, REGION_CPU1, 0 )
	ROM_LOAD( "p5.bin", 0x00000, 0x08000, 0xd6cf3387 )
	ROM_LOAD( "l5.bin", 0x10000, 0x20000, 0x49c9ecfb )
	ROM_LOAD( "f5.bin", 0x50000, 0x08000, 0xfa71c91c )
	ROM_LOAD( "g5.bin", 0x58000, 0x08000, 0x8cfd60aa )
	ROM_LOAD( "h5.bin", 0x60000, 0x08000, 0xd20dfcf9 )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "r1.bin", 0x00000, 0x20000, 0x407f77ca )
	ROM_LOAD( "s1.bin", 0x20000, 0x20000, 0x8572d156 )
	ROM_LOAD( "e1.bin", 0x40000, 0x08000, 0xd78c3428 )
	ROM_LOAD( "f1.bin", 0x48000, 0x08000, 0x799cc0e7 )
	ROM_LOAD( "g1.bin", 0x50000, 0x08000, 0x3ac8ae04 )
	ROM_LOAD( "h1.bin", 0x58000, 0x08000, 0xf0af2d38 )
ROM_END


GAME( 1986, ojankoy,  0, ojankoy,  ojankoy,  0, ROT0, "V-System Co.", "Ojanko Yakata (Japan)" )
GAME( 1987, ojanko2,  0, ojankoy,  ojankoy,  0, ROT0, "V-System Co.", "Ojanko Yakata 2bankan (Japan)" )
GAME( 1987, ccasino,  0, ccasino,  ccasino,  0, ROT0, "V-System Co.", "Chinese Casino [BET] (Japan)" )
GAME( 1988, ojankohs, 0, ojankohs, ojankohs, 0, ROT0, "V-System Co.", "Ojanko High School (Japan)" )
