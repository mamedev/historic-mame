/******************************************************************************

	Game Driver for Video System Mahjong series.

	Idol-Mahjong Final Romance
	(c)1991 Video System Co.,Ltd.

	Nekketsu Mahjong Sengen! AFTER 5
	(c)1991 Video System Co.,Ltd.

	Mahjong Daiyogen
	(c)1990 Video System Co.,Ltd.

	Mahjong Fun Club - Idol Saizensen
	(c)1989 Video System Co.,Ltd.

	Mahjong Natsu Monogatari (Mahjong Summer Story)
	(c)1989 Video System Co.,Ltd.

	Idol-Mahjong Housoukyoku
	(c)1988 System Service Co.,Ltd.

	Rettou Juudan Nekkyoku Janshi - Higashi Nippon Hen
	(c)1988 Video System Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2001/02/04 -
	and Nicola Salmoria, Aaron Giles

******************************************************************************/
/******************************************************************************
Memo:

- 2player's input is not supported.

- Communication between MAIN CPU and SUB CPU can be wrong.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "fromance.h"


/* Local variables */
static UINT8 fromance_directionflag;
static UINT8 fromance_commanddata;
static UINT8 fromance_portselect;

static UINT8 fromance_adpcm_reset;
static UINT8 fromance_adpcm_data;
static UINT8 fromance_vclk_left;



/*************************************
 *
 *	Machine init
 *
 *************************************/

static MACHINE_INIT( fromance )
{
	fromance_directionflag = 0;
	fromance_commanddata = 0;
	fromance_portselect = 0;

	fromance_adpcm_reset = 0;
	fromance_adpcm_data = 0;
	fromance_vclk_left = 0;
}



/*************************************
 *
 *	Master/slave communication
 *
 *************************************/

static READ_HANDLER( fromance_commanddata_r )
{
	return fromance_commanddata;
}


static void deferred_commanddata_w(int data)
{
	fromance_commanddata = data;
	fromance_directionflag = 1;
}


static WRITE_HANDLER( fromance_commanddata_w )
{
	/* do this on a timer to let the slave CPU synchronize */
	timer_set(TIME_NOW, data, deferred_commanddata_w);
}


static READ_HANDLER( fromance_busycheck_main_r )
{
	/* set a timer to force synchronization after the read */
	timer_set(TIME_NOW, 0, NULL);

	if (!fromance_directionflag) return 0x00;		// standby
	else return 0xff;								// busy
}


static READ_HANDLER( fromance_busycheck_sub_r )
{
	if (fromance_directionflag) return 0xff;		// standby
	else return 0x00;								// busy
}


static WRITE_HANDLER( fromance_busycheck_sub_w )
{
	fromance_directionflag = 0;
}



/*************************************
 *
 *	Slave CPU ROM banking
 *
 *************************************/

static WRITE_HANDLER( fromance_rombank_w )
{
	unsigned char *ROM = memory_region(REGION_CPU2);

	cpu_setbank(1, &ROM[0x010000 + (0x4000 * data)]);
}



/*************************************
 *
 *	ADPCM interface
 *
 *************************************/

static WRITE_HANDLER( fromance_adpcm_reset_w )
{
	fromance_adpcm_reset = (data & 0x01);
	fromance_vclk_left = 0;

	MSM5205_reset_w(0, !(data & 0x01));
}


static WRITE_HANDLER( fromance_adpcm_w )
{
	fromance_adpcm_data = data;
	fromance_vclk_left = 2;
}


static void fromance_adpcm_int(int irq)
{
	/* skip if we're reset */
	if (!fromance_adpcm_reset)
		return;

	/* clock the data through */
	if (fromance_vclk_left)
	{
		MSM5205_data_w(0, (fromance_adpcm_data >> 4));
		fromance_adpcm_data <<= 4;
		fromance_vclk_left--;
	}

	/* generate an NMI if we're out of data */
	if (!fromance_vclk_left)
		cpu_set_nmi_line(1, PULSE_LINE);
}



/*************************************
 *
 *	Input handlers
 *
 *************************************/

static WRITE_HANDLER( fromance_portselect_w )
{
	fromance_portselect = data;
}


static READ_HANDLER( fromance_keymatrix_r )
{
	int ret = 0xff;

	if (fromance_portselect & 0x01)
		ret &= readinputport(4);
	if (fromance_portselect & 0x02)
		ret &= readinputport(5);
	if (fromance_portselect & 0x04)
		ret &= readinputport(6);
	if (fromance_portselect & 0x08)
		ret &= readinputport(7);
	if (fromance_portselect & 0x10)
		ret &= readinputport(8);

	return ret;
}



/*************************************
 *
 *	Coin counters
 *
 *************************************/

static WRITE_HANDLER( fromance_coinctr_w )
{
	//
}



/*************************************
 *
 *	Master CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( nekkyoku_readmem_main )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xf000, 0xf000, input_port_0_r },
	{ 0xf001, 0xf001, fromance_keymatrix_r },
	{ 0xf002, 0xf002, input_port_1_r },
	{ 0xf003, 0xf003, fromance_busycheck_main_r },
	{ 0xf004, 0xf004, input_port_3_r },
	{ 0xf005, 0xf005, input_port_2_r },
MEMORY_END

static MEMORY_WRITE_START( nekkyoku_writemem_main )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xf000, 0xf000, fromance_portselect_w },
	{ 0xf001, 0xf001, MWA_NOP },
	{ 0xf002, 0xf002, fromance_coinctr_w },
	{ 0xf003, 0xf003, fromance_commanddata_w },
MEMORY_END


static MEMORY_READ_START( fromance_readmem_main )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0x9e89, 0x9e89, MRA_NOP },			// unknown (idolmj)
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe001, 0xe001, fromance_keymatrix_r },
	{ 0xe002, 0xe002, input_port_1_r },
	{ 0xe003, 0xe003, fromance_busycheck_main_r },
	{ 0xe004, 0xe004, input_port_3_r },
	{ 0xe005, 0xe005, input_port_2_r },
MEMORY_END

static MEMORY_WRITE_START( fromance_writemem_main )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, fromance_portselect_w },
	{ 0xe002, 0xe002, fromance_coinctr_w },
	{ 0xe003, 0xe003, fromance_commanddata_w },
MEMORY_END



/*************************************
 *
 *	Slave CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( nekkyoku_readmem_sub )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xefff, fromance_videoram_r },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, fromance_paletteram_r },
MEMORY_END

static MEMORY_WRITE_START( nekkyoku_writemem_sub )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xefff, fromance_videoram_w },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, fromance_paletteram_w },
MEMORY_END


static MEMORY_READ_START( fromance_readmem_sub )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, fromance_paletteram_r },
	{ 0xd000, 0xffff, fromance_videoram_r },
MEMORY_END

static MEMORY_WRITE_START( fromance_writemem_sub )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, fromance_paletteram_w },
	{ 0xd000, 0xffff, fromance_videoram_w },
MEMORY_END



/*************************************
 *
 *	Slave CPU port handlers
 *
 *************************************/

static PORT_READ_START( nekkyoku_readport_sub )
	{ 0x12, 0x12, IORP_NOP },				// unknown
	{ 0xe1, 0xe1, fromance_busycheck_sub_r },
	{ 0xe6, 0xe6, fromance_commanddata_r },
PORT_END

static PORT_WRITE_START( nekkyoku_writeport_sub )
	{ 0x10, 0x10, fromance_crtc_data_w },
	{ 0x11, 0x11, fromance_crtc_register_w },
	{ 0xe0, 0xe0, fromance_rombank_w },
	{ 0xe1, 0xe1, fromance_gfxreg_w },
	{ 0xe2, 0xe5, fromance_scroll_w },
	{ 0xe6, 0xe6, fromance_busycheck_sub_w },
	{ 0xe7, 0xe7, fromance_adpcm_reset_w },
	{ 0xe8, 0xe8, fromance_adpcm_w },
	{ 0xe9, 0xe9, AY8910_write_port_0_w },
	{ 0xea, 0xea, AY8910_control_port_0_w },
PORT_END

static PORT_READ_START( fromance_readport_sub )
	{ 0x12, 0x12, IORP_NOP },				// unknown
	{ 0x21, 0x21, fromance_busycheck_sub_r },
	{ 0x26, 0x26, fromance_commanddata_r },
PORT_END

static PORT_WRITE_START( idolmj_writeport_sub )
	{ 0x10, 0x10, fromance_crtc_data_w },
	{ 0x11, 0x11, fromance_crtc_register_w },
	{ 0x20, 0x20, fromance_rombank_w },
	{ 0x21, 0x21, fromance_gfxreg_w },
	{ 0x22, 0x25, fromance_scroll_w },
	{ 0x26, 0x26, fromance_busycheck_sub_w },
	{ 0x27, 0x27, fromance_adpcm_reset_w },
	{ 0x28, 0x28, fromance_adpcm_w },
	{ 0x29, 0x29, AY8910_write_port_0_w },
	{ 0x2a, 0x2a, AY8910_control_port_0_w },
PORT_END

static PORT_WRITE_START( fromance_writeport_sub )
	{ 0x10, 0x10, fromance_crtc_data_w },
	{ 0x11, 0x11, fromance_crtc_register_w },
	{ 0x20, 0x20, fromance_rombank_w },
	{ 0x21, 0x21, fromance_gfxreg_w },
	{ 0x22, 0x25, fromance_scroll_w },
	{ 0x26, 0x26, fromance_busycheck_sub_w },
	{ 0x27, 0x27, fromance_adpcm_reset_w },
	{ 0x28, 0x28, fromance_adpcm_w },
	{ 0x2a, 0x2a, YM2413_register_port_0_w },
	{ 0x2b, 0x2b, YM2413_data_port_0_w },
PORT_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

#define FROMANCE_KEYMATRIX1 \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) \

#define FROMANCE_KEYMATRIX2 \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet", KEYCODE_2, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) \

#define FROMANCE_KEYMATRIX3 \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) \

#define FROMANCE_KEYMATRIX4 \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define FROMANCE_KEYMATRIX5 \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance", KEYCODE_RALT, IP_JOY_NONE ) \
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score", KEYCODE_RCONTROL, IP_JOY_NONE ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up", KEYCODE_RSHIFT, IP_JOY_NONE ) \
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_X, IP_JOY_NONE ) \
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big", KEYCODE_ENTER, IP_JOY_NONE ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small", KEYCODE_BACKSPACE, IP_JOY_NONE ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )


INPUT_PORTS_START( nekkyoku )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	FROMANCE_KEYMATRIX1	/* (4) PORT 1-0 */
	FROMANCE_KEYMATRIX2	/* (5) PORT 1-1 */
	FROMANCE_KEYMATRIX3	/* (6) PORT 1-2 */
	FROMANCE_KEYMATRIX4	/* (7) PORT 1-3 */
	FROMANCE_KEYMATRIX5	/* (8) PORT 1-4 */
INPUT_PORTS_END


INPUT_PORTS_START( idolmj )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Voices" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	FROMANCE_KEYMATRIX1	/* (4) PORT 1-0 */
	FROMANCE_KEYMATRIX2	/* (5) PORT 1-1 */
	FROMANCE_KEYMATRIX3	/* (6) PORT 1-2 */
	FROMANCE_KEYMATRIX4	/* (7) PORT 1-3 */
	FROMANCE_KEYMATRIX5	/* (8) PORT 1-4 */
INPUT_PORTS_END


INPUT_PORTS_START( fromance )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	FROMANCE_KEYMATRIX1	/* (4) PORT 1-0 */
	FROMANCE_KEYMATRIX2	/* (5) PORT 1-1 */
	FROMANCE_KEYMATRIX3	/* (6) PORT 1-2 */
	FROMANCE_KEYMATRIX4	/* (7) PORT 1-3 */
	FROMANCE_KEYMATRIX5	/* (8) PORT 1-4 */
INPUT_PORTS_END


INPUT_PORTS_START( nmsengen )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	FROMANCE_KEYMATRIX1	/* (4) PORT 1-0 */
	FROMANCE_KEYMATRIX2	/* (5) PORT 1-1 */
	FROMANCE_KEYMATRIX3	/* (6) PORT 1-2 */
	FROMANCE_KEYMATRIX4	/* (7) PORT 1-3 */
	FROMANCE_KEYMATRIX5	/* (8) PORT 1-4 */
INPUT_PORTS_END


INPUT_PORTS_START( daiyogen )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	FROMANCE_KEYMATRIX1	/* (4) PORT 1-0 */
	FROMANCE_KEYMATRIX2	/* (5) PORT 1-1 */
	FROMANCE_KEYMATRIX3	/* (6) PORT 1-2 */
	FROMANCE_KEYMATRIX4	/* (7) PORT 1-3 */
	FROMANCE_KEYMATRIX5	/* (8) PORT 1-4 */
INPUT_PORTS_END


INPUT_PORTS_START( mfunclub )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Voices" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	FROMANCE_KEYMATRIX1	/* (4) PORT 1-0 */
	FROMANCE_KEYMATRIX2	/* (5) PORT 1-1 */
	FROMANCE_KEYMATRIX3	/* (6) PORT 1-2 */
	FROMANCE_KEYMATRIX4	/* (7) PORT 1-3 */
	FROMANCE_KEYMATRIX5	/* (8) PORT 1-4 */
INPUT_PORTS_END


INPUT_PORTS_START( mjnatsu )
	PORT_START	/* (0) TEST SW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Voices" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* (3) DIPSW-2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	FROMANCE_KEYMATRIX1	/* (4) PORT 1-0 */
	FROMANCE_KEYMATRIX2	/* (5) PORT 1-1 */
	FROMANCE_KEYMATRIX3	/* (6) PORT 1-2 */
	FROMANCE_KEYMATRIX4	/* (7) PORT 1-3 */
	FROMANCE_KEYMATRIX5	/* (8) PORT 1-4 */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout bglayout =
{
	8,4,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32 },
	16*8
};


static struct GfxDecodeInfo fromance_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bglayout,   0, 128 },
	{ REGION_GFX2, 0, &bglayout,   0, 128 },
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct YM2413interface ym2413_interface=
{
	1,						/* 1 chip */
	3579545*2,				/* 3.579545 MHz ? */
	{ 100 },				/* Volume */
};


static struct AY8910interface ay8910_interface =
{
	1,						/* 1 chip */
	12000000/6,				/* 1.5 MHz ? */
	{ 15 },					/* volume */
	{ 0 },					/* read port #0 */
	{ 0 },					/* read port #1 */
	{ 0 },					/* write port #0 */
	{ 0 }					/* write port #1 */
};


static struct MSM5205interface msm5205_interface =
{
	1,						/* 1 chip */
	384000,					/* 384 KHz */
	{ fromance_adpcm_int },	/* IRQ handler */
	{ MSM5205_S48_4B },		/* 8 KHz */
	{ 80 }					/* volume */
};

static struct MSM5205interface fromance_msm5205_interface =
{
	1,						/* 1 chip */
	384000,					/* 384 KHz */
	{ fromance_adpcm_int },	/* IRQ handler */
	{ MSM5205_S48_4B },		/* 8 KHz */
	{ 20 }					/* volume */
};


/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( nekkyoku )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 Mhz ? */
	MDRV_CPU_MEMORY(nekkyoku_readmem_main,nekkyoku_writemem_main)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 Mhz ? */
	MDRV_CPU_MEMORY(nekkyoku_readmem_sub,nekkyoku_writemem_sub)
	MDRV_CPU_PORTS(nekkyoku_readport_sub,nekkyoku_writeport_sub)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(fromance)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 352-1, 0, 240-1)
	MDRV_GFXDECODE(fromance_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(nekkyoku)
	MDRV_VIDEO_UPDATE(fromance)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( idolmj )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 Mhz ? */
	MDRV_CPU_MEMORY(fromance_readmem_main,fromance_writemem_main)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 Mhz ? */
	MDRV_CPU_MEMORY(fromance_readmem_sub,fromance_writemem_sub)
	MDRV_CPU_PORTS(fromance_readport_sub,idolmj_writeport_sub)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(fromance)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 352-1, 0, 240-1)
	MDRV_GFXDECODE(fromance_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(fromance)
	MDRV_VIDEO_UPDATE(fromance)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fromance )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 Mhz ? */
	MDRV_CPU_MEMORY(fromance_readmem_main,fromance_writemem_main)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.00 Mhz ? */
	MDRV_CPU_MEMORY(fromance_readmem_sub,fromance_writemem_sub)
	MDRV_CPU_PORTS(fromance_readport_sub,fromance_writeport_sub)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(fromance)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 352-1, 0, 240-1)
	MDRV_GFXDECODE(fromance_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(fromance)
	MDRV_VIDEO_UPDATE(fromance)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
	MDRV_SOUND_ADD(MSM5205, fromance_msm5205_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( nekkyoku )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "1-ic1a.bin",  0x000000, 0x008000, 0xbb52d959 )
	ROM_LOAD( "2-ic2a.bin",  0x008000, 0x008000, 0x61848d8b )

	ROM_REGION( 0x210000, REGION_CPU2, 0 )
	ROM_LOAD( "3-ic3a.bin",  0x000000, 0x008000, 0xa13da011 )
	ROM_LOAD( "ic4a.bin",    0x010000, 0x080000, 0x1cc4d31b )
	ROM_LOAD( "ic5a.bin",    0x090000, 0x080000, 0x8b0945a1 )
	ROM_LOAD( "ic6a.bin",    0x110000, 0x080000, 0xd5615e1d )
	ROM_LOAD( "4-ic7a.bin",  0x190000, 0x008000, 0xe259cfbb )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic11a.bin",   0x000000, 0x080000, 0x2bc2b1d0 )
	ROM_LOAD( "ic12a.bin",   0x080000, 0x040000, 0xcac93dc0 )
	ROM_LOAD( "6-ic13a.bin", 0x0c0000, 0x008000, 0x84830e34 )
	ROM_FILL(                0x0c8000, 0x038000, 0xff )
	ROM_FILL(                0x100000, 0x100000, 0xff )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic8a.bin",    0x000000, 0x080000, 0x599790d8 )
	ROM_LOAD( "ic9a.bin",    0x080000, 0x040000, 0x78c1906f )
	ROM_LOAD( "5-ic10a.bin", 0x0c0000, 0x008000, 0x2e78515f )
	ROM_FILL(                0x0c8000, 0x038000, 0xff )
	ROM_FILL(                0x100000, 0x100000, 0xff )
ROM_END


ROM_START( idolmj )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "3-13g.bin", 0x000000, 0x008000, 0x910e9e7a )

	ROM_REGION( 0x410000, REGION_CPU2, 0 )
	ROM_LOAD( "5-13e.bin", 0x000000, 0x008000, 0xcda33264 )
	ROM_LOAD( "18e.bin",   0x010000, 0x080000, 0x7ee5aaf3 )
	ROM_LOAD( "17e.bin",   0x090000, 0x080000, 0x38055f94 )
	ROM_LOAD( "4-14e.bin", 0x190000, 0x010000, 0x84d80b43 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6e.bin",    0x000000, 0x080000, 0x51dadedd )
	ROM_LOAD( "2-8e.bin",  0x080000, 0x008000, 0xa1a62c4c )
	ROM_FILL(              0x088000, 0x008000, 0xff )
	ROM_FILL(              0x090000, 0x170000, 0xff )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "3e.bin",    0x000000, 0x080000, 0xeff9b562 )
	ROM_LOAD( "1-1e.bin",  0x080000, 0x008000, 0xabf03c62 )
	ROM_FILL(              0x088000, 0x008000, 0xff )
	ROM_FILL(              0x090000, 0x170000, 0xff )
ROM_END


ROM_START( mjnatsu )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "3-ic70.bin", 0x000000, 0x008000, 0x543eb9e1 )

	ROM_REGION( 0x410000, REGION_CPU2, 0 )
	ROM_LOAD( "4-ic47.bin", 0x000000, 0x008000, 0x27a61dc7 )
	ROM_LOAD( "ic87.bin",   0x010000, 0x080000, 0xcaec9310 )
	ROM_LOAD( "ic78.bin",   0x090000, 0x080000, 0x2b291006 )
	ROM_LOAD( "ic72.bin",   0x110000, 0x020000, 0x42464fba )
	ROM_LOAD( "5-ic48.bin", 0x210000, 0x010000, 0xd3c06cd9 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic58.bin",   0x000000, 0x080000, 0x257a8075 )
	ROM_LOAD( "ic63.bin",   0x080000, 0x020000, 0xb54c7d3a )
	ROM_LOAD( "1-ic74.bin", 0x0a0000, 0x008000, 0xfbafa46b )
	ROM_FILL(               0x0a8000, 0x008000, 0xff )
	ROM_FILL(               0x0b0000, 0x150000, 0xff )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic59.bin",   0x000000, 0x080000, 0x03983ac7 )
	ROM_LOAD( "ic64.bin",   0x080000, 0x040000, 0x9bd8e855 )
	ROM_FILL(               0x0c0000, 0x140000, 0xff )
ROM_END


ROM_START( mfunclub )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "3.70",        0x000000, 0x008000, 0xe6f76ca3 )

	ROM_REGION( 0x410000, REGION_CPU2, 0 )
	ROM_LOAD( "4.47",        0x000000, 0x008000, 0xd71ee0e3 )
	ROM_LOAD( "586.87",      0x010000, 0x080000, 0xe197af4a )
	ROM_LOAD( "587.78",      0x090000, 0x080000, 0x08ff39c3 )
	ROM_LOAD( "5.57",        0x290000, 0x010000, 0xbf988bde )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "584.58",      0x000000, 0x080000, 0xd65af431 )
	ROM_LOAD( "lh634a14.63", 0x080000, 0x080000, 0xcdda9b9e )
	ROM_FILL(                0x100000, 0x080000, 0xff )
	ROM_LOAD( "1.74",        0x180000, 0x008000, 0x5b0b2efc )
	ROM_FILL(                0x188000, 0x078000, 0xff )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "585.59",      0x000000, 0x080000, 0x58ce0937 )
	ROM_FILL(                0x080000, 0x100000, 0xff )
	ROM_LOAD( "2.75",        0x180000, 0x010000, 0x4dd4f786 )
	ROM_FILL(                0x190000, 0x070000, 0xff )
ROM_END


ROM_START( daiyogen )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "n1-ic70.bin", 0x000000, 0x008000, 0x29af632b )

	ROM_REGION( 0x130000, REGION_CPU2, 0 )
	ROM_LOAD( "n2-ic47.bin", 0x000000, 0x008000, 0x8896604c )
	ROM_LOAD( "ic87.bin",    0x010000, 0x080000, 0x4f86ffe2 )
	ROM_LOAD( "ic78.bin",    0x090000, 0x080000, 0xae52bccd )
	ROM_LOAD( "7-ic72.bin",  0x110000, 0x020000, 0x30279296 )

	ROM_REGION( 0x0c0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic58.bin",    0x000000, 0x080000, 0x8cf3d5f5 )
	ROM_LOAD( "ic63.bin",    0x080000, 0x040000, 0x64611070 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic59.bin",    0x000000, 0x080000, 0x715f2f8c )
	ROM_LOAD( "ic64.bin",    0x080000, 0x080000, 0xe5a41864 )
ROM_END


ROM_START( nmsengen )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "3-ic70.bin",   0x000000, 0x008000, 0x4e6edbbb )

	ROM_REGION( 0x410000, REGION_CPU2, 0 )
	ROM_LOAD( "4-ic47.bin",   0x000000, 0x008000, 0xd31c596e )
	ROM_LOAD( "vsj-ic87.bin", 0x010000, 0x100000, 0xd3e8bd73 )
	ROM_LOAD( "j5-ic72.bin",  0x210000, 0x020000, 0xdb937253 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vsk-ic63.bin", 0x000000, 0x100000, 0xf95f9c67 )
	ROM_LOAD( "ic58.bin",     0x100000, 0x040000, 0xc66dcf18 )
	ROM_FILL(                 0x140000, 0x080000, 0xff )
	ROM_LOAD( "1-ic68.bin",   0x1c0000, 0x020000, 0xa944a8d6 )
	ROM_FILL(                 0x1e0000, 0x020000, 0xff )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vsh-ic64.bin", 0x000000, 0x100000, 0xf546ffaf )
	ROM_LOAD( "vsg-ic59.bin", 0x100000, 0x080000, 0x25bae018 )
	ROM_LOAD( "ic69.bin",     0x180000, 0x040000, 0xdc867ccd )
	ROM_LOAD( "2-ic75.bin",   0x1c0000, 0x020000, 0xe2fad82e )
	ROM_FILL(                 0x1e0000, 0x020000, 0xff )
ROM_END


ROM_START( fromance )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "2-ic70.bin", 0x000000, 0x008000, 0xa0866e26 )

	ROM_REGION( 0x410000, REGION_CPU2, 0 )
	ROM_LOAD( "1-ic47.bin", 0x000000, 0x008000, 0xac859917 )
	ROM_LOAD( "ic87.bin",   0x010000, 0x100000, 0xbb0d224e )
	ROM_LOAD( "ic78.bin",   0x110000, 0x040000, 0xba2dba83 )
	ROM_LOAD( "5-ic72.bin", 0x210000, 0x020000, 0x377cd57c )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic63.bin",   0x000000, 0x100000, 0xfaa9cdf3 )
	ROM_FILL(               0x100000, 0x0c0000, 0xff )
	ROM_LOAD( "4-ic68.bin", 0x1c0000, 0x020000, 0x9b35cea3 )
	ROM_FILL(               0x1e0000, 0x020000, 0xff )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic64.bin",   0x000000, 0x100000, 0x23b9a484 )
	ROM_FILL(               0x100000, 0x080000, 0xff )
	ROM_LOAD( "ic69.bin",   0x180000, 0x040000, 0xd06a0fc0 )
	ROM_LOAD( "3-ic75.bin", 0x1c0000, 0x020000, 0xbb314e78 )
	ROM_FILL(               0x1e0000, 0x020000, 0xff )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX(1988, nekkyoku, 0, nekkyoku, nekkyoku, 0, ROT0, "Video System", "Rettou Juudan Nekkyoku Janshi - Higashi Nippon Hen (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1988, idolmj,   0, idolmj,   idolmj,   0, ROT0, "System Service", "Idol-Mahjong Housoukyoku (Japan)" )
GAME( 1989, mjnatsu,  0, fromance, mjnatsu,  0, ROT0, "Video System", "Mahjong Natsu Monogatari (Japan)" )
GAME( 1989, mfunclub, 0, fromance, mfunclub, 0, ROT0, "Video System", "Mahjong Fun Club - Idol Saizensen (Japan)" )
GAME( 1990, daiyogen, 0, fromance, daiyogen, 0, ROT0, "Video System", "Mahjong Daiyogen (Japan)" )
GAME( 1991, nmsengen, 0, fromance, nmsengen, 0, ROT0, "Video System", "Nekketsu Mahjong Sengen! AFTER 5 (Japan)" )
GAME( 1991, fromance, 0, fromance, fromance, 0, ROT0, "Video System", "Idol-Mahjong Final Romance (Japan)" )
