/***************************************************************************

  Sony ZN1/ZN2 - Arcade PSX Hardware
  ==================================
  Driver by smf

  QSound emulation based on information from the cps1/cps2 driver, Amuse & Miguel Angel Horna
  Taito FX1a sound emulation based on information from the Taito F2 driver

  The ZN1/ZN2 boards are standard boards from Sony. Extra sound h/w is
  included on the rom board,

  The BIOS is protected in all the games that run on this hardware.
  During boot up the BIOS decrypts parts of itself into ram using
  a device connected to the PSX controller port. As this is not
  emulated yet you only get the boot up colours.


  Capcom ZN1 ( coh-1000 ) / ZN2 ( coh-3002c )
  -------------------------------------------

  The QSound hardware is different to cps2 as it uses an i/o port and
  nmi's instead of shared memory. The driver uses 8bit i/o addresses
  but the real ZN1 hardware may use 16bit i/o addresses as the code
  always accesses port 0xf100. The ZN2 code however seems to vary the
  top eight bits of the address, this may or may not be important but
  it is currently ignored.

  The Gallop Racer dump came from a Capcom ZN1 QSound board but there
  were no QSound program/sample roms & the game doesn't appear to use
  it. Tecmo went on to produce games on their own boards.

    Key
    ---
    CP09 - kikaioh rom board
    CP10 - kikaioh/shiryu2 motherboard
    CP13 - shiryu2 rom board


  Taito FX1a
  ----------

  The extra sound h/w is the same as taito f2 ( z80-a + ym2610b ).

    Key
    ---
    TT02 - Super Football Champ
    TT06 - Magical Date


  Taito FX1b
  ----------

  The extra sound h/w isn't emulated and consists of an mn1020012a +
  zoom zsg-2 + tms57002dpha.

  mn1020012a is a 16bit panasonic mpu & the zoom zsg-2 is a sound generator
  lsi produced by zoom corp in 1995.

    Key
    ---
    TT04 - Ray Storm
    TT05 - Fighter's Impact Ace
    TT07 - G-Darius

  Beastorizer bootleg runs on a ray storm motherboard, but doesn't have fx1b sound h/w.

  PS Arcade 95 ( coh-1002e )
  --------------------------
Main Board: Sony ZN-1  (1-659-709-12  COH-1002E)
 Sub Board: Raizing RA9701 SUB

    Key
    ---
    ET01 - Main board
    ET02 - Beastorizer

TMP68HC00N-16 (Toshiba 68K 12MHz)
OSC: 12.0000MHz (Kyocera KX-01-1)
Mach211 (x2) Labled "Main_IF2" & "SUB_IF2"
ST M628032-15E1 (x2) Ram??
Yamaha YMF271-F
GAL16V8B
CAT702 103090-ET02 (the "ET02" is a printed sticker/label) ??


  Acclaim PSX ( coh-1000 )
  ------------------------

    Key
    ---
    AC01 - Motherboard
    AC02 - NBA Jam Extreme

Sound: Analog Devices ADSP-2181

  Video System
  ------------

    Key
    ---
    KN01 - Motherboard
    KN02 - Sonic Wings Limited

  Tecmo ( coh-1002m )
  -------------------

    Key
    ---
    MG01 - Motherboard
    MG02 - Gallop Racer 2
    MG05 - Dead or Alive ++
    MG09 - Tondemo Crisis

  ***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/mips/psx.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/taitosnd.h"
#include "includes/psx.h"
#include "machine/znsec.h"
#include "machine/idectrl.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		if( cpu_getactivecpu() != -1 )
		{
			logerror( "%08x: %s", activecpu_get_pc(), buf );
		}
		else
		{
			logerror( "(timer) : %s", buf );
		}
	}
}

#define NODUMP 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80

static const unsigned char ac01[ 8 ] = { 0x80, 0x1c, 0xe2, 0xfa, 0xf9, 0xf1, 0x30, 0xc0 };
static const unsigned char ac02[ 8 ] = { 0xfc, 0x60, 0xe2, 0xfa, 0xf9, 0xf1, 0x30, 0xc0 };
static const unsigned char tw01[ 8 ] = { 0xc0, 0x18, 0xf9, 0x81, 0x82, 0xfe, 0x0c, 0xf0 };
static const unsigned char tw02[ 8 ] = { 0xf0, 0x81, 0x03, 0xfa, 0x18, 0x1c, 0x3c, 0xc0 };
static const unsigned char at01[ 8 ] = { NODUMP };
static const unsigned char at02[ 8 ] = { NODUMP };
static const unsigned char cp01[ 8 ] = { 0xf0, 0x81, 0xc1, 0x20, 0xe2, 0xfe, 0x04, 0xf8 };
static const unsigned char cp02[ 8 ] = { NODUMP };
static const unsigned char cp03[ 8 ] = { 0xc0, 0x10, 0x60, 0x7c, 0x04, 0xfa, 0x03, 0x01 };
static const unsigned char cp04[ 8 ] = { 0xf8, 0xe2, 0xe1, 0x81, 0x7c, 0x0c, 0x30, 0xc0 };
static const unsigned char cp05[ 8 ] = { 0x80, 0x08, 0x30, 0xc2, 0xfe, 0xfd, 0xe1, 0xe0 };
static const unsigned char cp06[ 8 ] = { 0xf0, 0x20, 0x3c, 0xfd, 0x81, 0x78, 0xfa, 0x02 };
static const unsigned char cp07[ 8 ] = { NODUMP };
static const unsigned char cp08[ 8 ] = { 0xe0, 0xf2, 0x70, 0x81, 0xc1, 0x3c, 0x04, 0xf8 };
static const unsigned char cp09[ 8 ] = { NODUMP };
static const unsigned char cp10[ 8 ] = { 0xe0, 0x40, 0x38, 0x08, 0xf1, 0x03, 0xfe, 0xfc };
static const unsigned char cp11[ 8 ] = { NODUMP };
static const unsigned char cp12[ 8 ] = { NODUMP };
static const unsigned char cp13[ 8 ] = { 0x02, 0x70, 0x08, 0x04, 0x3c, 0x20, 0xe1, 0x01 };
static const unsigned char cp14[ 8 ] = { NODUMP };
static const unsigned char et01[ 8 ] = { 0x02, 0x08, 0x18, 0x1c, 0xfd, 0xc1, 0x40, 0x80 };
static const unsigned char et02[ 8 ] = { 0xc0, 0xe1, 0xe2, 0xfe, 0x7c, 0x70, 0x08, 0xf8 };
static const unsigned char et03[ 8 ] = { 0xc0, 0x08, 0xfa, 0xe2, 0xe1, 0xfd, 0x7c, 0x80 };
static const unsigned char mg01[ 8 ] = { 0x80, 0xf2, 0x30, 0x38, 0xf9, 0xfd, 0x1c, 0xe0 };
static const unsigned char mg02[ 8 ] = { 0xe0, 0x7c, 0x40, 0xc1, 0xf9, 0xfa, 0xf2, 0xf0 };
static const unsigned char mg03[ 8 ] = { 0xc0, 0x04, 0x78, 0x82, 0x03, 0xf1, 0x10, 0xe0 };
static const unsigned char mg04[ 8 ] = { 0xf0, 0xe1, 0x81, 0x82, 0xfa, 0x04, 0x3c, 0xc0 };
static const unsigned char mg05[ 8 ] = { NODUMP };
static const unsigned char mg09[ 8 ] = { NODUMP };
static const unsigned char mg11[ 8 ] = { 0x80, 0xc2, 0x38, 0xf9, 0xfd, 0x1c, 0x10, 0xf0 };
static const unsigned char tt01[ 8 ] = { 0xe0, 0xf9, 0xfd, 0x7c, 0x70, 0x30, 0xc2, 0x02 };
static const unsigned char tt02[ 8 ] = { NODUMP };
static const unsigned char tt03[ 8 ] = { NODUMP };
static const unsigned char tt04[ 8 ] = { 0xc0, 0xe1, 0xe2, 0xfa, 0x78, 0x7c, 0x0c, 0xf0 };
static const unsigned char tt05[ 8 ] = { NODUMP };
static const unsigned char tt06[ 8 ] = { NODUMP };
static const unsigned char tt07[ 8 ] = { NODUMP };
static const unsigned char tt10[ 8 ] = { 0x80, 0x20, 0x38, 0x08, 0xf1, 0x03, 0xfe, 0xfc };
static const unsigned char tt16[ 8 ] = { 0xc0, 0x04, 0xf9, 0xe1, 0x60, 0x70, 0xf2, 0x02 };
static const unsigned char kn01[ 8 ] = { NODUMP };
static const unsigned char kn02[ 8 ] = { NODUMP };

static struct
{
	const char *s_name;
	const unsigned char *p_n_mainsec;
	const unsigned char *p_n_gamesec;
} zn_config_table[] =
{
	{ "nbajamex", ac01, ac02 }, /* black screen */
	{ "jdredd",   ac01, ac02 }, /* OK ( missing guns ) */
	{ "jdreddb",  ac01, ac02 }, /* OK ( missing guns ) */
	{ "primrag2", tw01, tw02 }, /* boots */
/*	{ "hvnsgate", at01, at02 }, */
	{ "ts2",      cp01, cp02 }, /* system error C930 */
	{ "ts2j",     cp01, cp02 }, /* system error C930 */
	{ "starglad", cp01, cp03 }, /* OK */
	{ "sfex",     cp01, cp04 }, /* OK */
	{ "sfexa",    cp01, cp04 }, /* OK */
	{ "sfexj",    cp01, cp04 }, /* OK */
	{ "glpracr",  cp01, cp05 }, /* OK */
	{ "rvschool", cp10, cp06 }, /* OK */
	{ "jgakuen",  cp10, cp06 }, /* OK */
	{ "plsmaswd", cp10, cp07 }, /* system error D094 */
	{ "stargld2", cp10, cp07 }, /* system error D094 */
	{ "sfex2",    cp10, cp08 }, /* OK */
	{ "techromn", cp10, cp09 }, /* system error D094 */
	{ "kikaioh",  cp10, cp09 }, /* system error D094 */
	{ "tgmj",     cp10, cp11 }, /* system error D094 */ /* ? */
	{ "sfexp",    cp10, cp12 }, /* system error C094 */
	{ "sfexpj",   cp10, cp12 }, /* system error C094 */
	{ "strider2", cp10, cp13 }, /* OK ( crashes on bosses ) */
	{ "shiryu2",  cp10, cp13 }, /* OK ( crashes on bosses ) */
	{ "sfex2p",   cp10, cp14 }, /* system error D094 */ /* ? */
	{ "sfex2pj",  cp10, cp14 }, /* system error D094 */ /* ? */
	{ "beastrzr", et01, et02 }, /* OK */
	{ "beastrza", et01, et02 }, /* OK */
	{ "beastrzb", et01, et02 }, /* OK */
/*	{ "bldyror2", et01, et03 }, */
	{ "glpracr2", mg01, mg02 }, /* locks up when starting a game/entering test mode */
	{ "glprac2j", mg01, mg02 }, /* locks up when starting a game/entering test mode */
	{ "glprac2l", mg01, mg02 }, /* locks up when starting a game/entering test mode */
	{ "cbaj",     mg01, mg03 }, /* OK */
	{ "shngmtkb", mg01, mg04 }, /* OK */
	{ "doapp",    mg01, mg05 }, /* system error D094 */
	{ "tondemo",  mg01, mg09 }, /* system error D094 */
	{ "brvblade", mg01, mg11 }, /* OK */
	{ "sfchamp",  tt01, tt02 }, /* red screen */
	{ "psyfrcex", tt01, tt03 }, /* red screen */
	{ "psyforce", tt01, tt03 }, /* red screen */
	{ "raystorm", tt01, tt04 }, /* OK */
	{ "ftimpcta", tt01, tt05 }, /* red screen */
	{ "mgcldate", tt01, tt06 }, /* red screen */
	{ "mgcldtea", tt01, tt06 }, /* red screen */
	{ "gdarius",  tt01, tt07 }, /* red screen */
	{ "gdarius2", tt01, tt07 }, /* red screen */
	{ "taitogn",  tt10, tt16 }, /* error B930 */
	{ "sncwgltd", kn01, kn02 }, /* red screen */
	{ NULL, NULL, NULL }
};

static UINT32 m_n_znsecsel;
static UINT32 m_b_znsecport;
static int m_n_dip_bit;

static READ32_HANDLER( znsecsel_r )
{
	verboselog( 2, "znsecsel_r( %08x, %08x )\n", offset, mem_mask );
	return m_n_znsecsel;
}

static void sio_znsec0_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_CLOCK ) != 0 )
	{
		psx_sio_input( 0, PSX_SIO_IN_DATA, ( znsec_step( 0, ( n_data & PSX_SIO_OUT_DATA ) / PSX_SIO_OUT_DATA ) & 1 ) * PSX_SIO_IN_DATA );
	}
}

static void sio_znsec1_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_CLOCK ) != 0 )
	{
		psx_sio_input( 0, PSX_SIO_IN_DATA, ( znsec_step( 1, ( n_data & PSX_SIO_OUT_DATA ) / PSX_SIO_OUT_DATA ) & 1 ) * PSX_SIO_IN_DATA );
	}
}

static void sio_pad_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_DTR ) != 0 )
	{
		m_b_znsecport = 1;
	}
	else
	{
		m_b_znsecport = 0;
	}

	verboselog( 2, "read pad %04x %04x %02x\n", m_n_znsecsel, m_b_znsecport, n_data );
	psx_sio_input( 0, PSX_SIO_IN_DATA, PSX_SIO_IN_DATA );
}

static void sio_dip_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_CLOCK ) != 0 )
	{
		verboselog( 2, "read dip %02x -> %02x\n", n_data, ( ( readinputport( 7 ) >> m_n_dip_bit ) & 1 ) * PSX_SIO_IN_DATA );
		psx_sio_input( 0, PSX_SIO_IN_DATA, ( ( readinputport( 7 ) >> m_n_dip_bit ) & 1 ) * PSX_SIO_IN_DATA );
		m_n_dip_bit++;
		m_n_dip_bit &= 7;
	}
}

static WRITE32_HANDLER( znsecsel_w )
{
	COMBINE_DATA( &m_n_znsecsel );

	if( ( m_n_znsecsel & 0x80 ) == 0 )
	{
		psx_sio_install_handler( 0, sio_pad_handler );
	}
	else if( ( m_n_znsecsel & 0x08 ) == 0 )
	{
		znsec_start( 1 );
		psx_sio_install_handler( 0, sio_znsec1_handler );
	}
	else if( ( m_n_znsecsel & 0x04 ) == 0 )
	{
		znsec_start( 0 );
		psx_sio_install_handler( 0, sio_znsec0_handler );
	}
	else
	{
		psx_sio_install_handler( 0, sio_dip_handler );
	}

	verboselog( 2, "znsecsel_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static WRITE_HANDLER( qsound_bankswitch_w )
{
	cpu_setbank( 10, memory_region( REGION_CPU2 ) + 0x10000 + ( ( data & 0x0f ) * 0x4000 ) );
}

static WRITE_HANDLER( fx1a_sound_bankswitch_w )
{
	cpu_setbank( 10, memory_region( REGION_CPU2 ) + 0x10000 + ( ( ( data - 1 ) & 0x07 ) * 0x4000 ) );
}

static READ32_HANDLER( jamma_0_r )
{
	return readinputport( 0 );
}

static READ32_HANDLER( jamma_1_r )
{
	return readinputport( 1 );
}

static READ32_HANDLER( jamma_2_r )
{
	return readinputport( 2 );
}

static READ32_HANDLER( jamma_3_r )
{
	return readinputport( 3 );
}

static READ32_HANDLER( jamma_4_r )
{
	return readinputport( 4 );
}

static READ32_HANDLER( jamma_5_r )
{
	return readinputport( 5 );
}

static READ32_HANDLER( jamma_6_r )
{
	return readinputport( 6 );
}

static READ32_HANDLER( unknown_r )
{
	verboselog( 0, "unknown_r( %08x, %08x )\n", offset, mem_mask );
	return 0xffffffff;
}

static WRITE32_HANDLER( zn_qsound_w )
{
	soundlatch_w(0, data);
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

static ADDRESS_MAP_START( zn_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x00400000, 0x007fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80100f) AM_NOP
	AM_RANGE(0x1f801010, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(psx_spu_delay_r)
	AM_RANGE(0x1f801020, 0x1f801023) AM_READ(psx_com_delay_r)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_RAM
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80113f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fa00000, 0x1fa00003) AM_READ(jamma_0_r)
	AM_RANGE(0x1fa00100, 0x1fa00103) AM_READ(jamma_1_r)
	AM_RANGE(0x1fa00200, 0x1fa00203) AM_READ(jamma_2_r)
	AM_RANGE(0x1fa00300, 0x1fa00303) AM_READ(jamma_3_r)
	AM_RANGE(0x1fa10000, 0x1fa10003) AM_READ(jamma_4_r)
	AM_RANGE(0x1fa10100, 0x1fa10103) AM_READ(jamma_5_r)
	AM_RANGE(0x1fa10200, 0x1fa10203) AM_READ(jamma_6_r)
	AM_RANGE(0x1fa10300, 0x1fa10303) AM_READWRITE(znsecsel_r, znsecsel_w)
	AM_RANGE(0x1faf0000, 0x1faf07ff) AM_RAM AM_BASE((data32_t **)&generic_nvram) AM_SIZE(&generic_nvram_size) /* eeprom */
	AM_RANGE(0x1fb20000, 0x1fb20007) AM_READ(unknown_r)
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x80400000, 0x807fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( qsound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK10)	/* banked (contains music data) */
	AM_RANGE(0xd007, 0xd007) AM_READ(qsound_status_r)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qsound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(qsound_data_h_w)
	AM_RANGE(0xd001, 0xd001) AM_WRITE(qsound_data_l_w)
	AM_RANGE(0xd002, 0xd002) AM_WRITE(qsound_cmd_w)
	AM_RANGE(0xd003, 0xd003) AM_WRITE(qsound_bankswitch_w)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qsound_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( link_readmem, ADDRESS_SPACE_PROGRAM, 8 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( link_writemem, ADDRESS_SPACE_PROGRAM, 8 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( fx1a_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK10)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0xe001, 0xe001) AM_READ(YM2610_read_port_0_r)
	AM_RANGE(0xe002, 0xe002) AM_READ(YM2610_status_port_0_B_r)
	AM_RANGE(0xe200, 0xe200) AM_READ(MRA8_NOP)
	AM_RANGE(0xe201, 0xe201) AM_READ(taitosound_slave_comm_r)
	AM_RANGE(0xea00, 0xea00) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( fx1a_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0xe002, 0xe002) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0xe003, 0xe003) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0xe200, 0xe200) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xe201, 0xe201) AM_WRITE(taitosound_slave_comm_w)
	AM_RANGE(0xe400, 0xe403) AM_WRITE(MWA8_NOP) /* pan */
	AM_RANGE(0xee00, 0xee00) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf000, 0xf000) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf200, 0xf200) AM_WRITE(fx1a_sound_bankswitch_w)
ADDRESS_MAP_END

static void init_znsec( void )
{
	int n_game;

	psx_driver_init();

	n_game = 0;
	while( zn_config_table[ n_game ].s_name != NULL )
	{
		if( strcmp( Machine->gamedrv->name, zn_config_table[ n_game ].s_name ) == 0 )
		{
			znsec_init( 0, zn_config_table[ n_game ].p_n_mainsec );
			znsec_init( 1, zn_config_table[ n_game ].p_n_gamesec );
			psx_sio_install_handler( 0, sio_pad_handler );
			break;
		}
		n_game++;
	}

	if( strcmp( Machine->gamedrv->name, "glpracr" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glprac2l" ) == 0 )
	{
		/* disable:
			the QSound CPU for glpracr as it doesn't have any roms &
			the link cpu for glprac2l as the h/w is not emulated yet. */
		timer_suspendcpu( 1, 1, SUSPEND_REASON_DISABLE );
	}
}

/* sound player */

static int scode;
static int scode_last;
static int queue_data;
static int queue_len;
static int n_playermode;

static WRITE32_HANDLER( player_queue_w )
{
	if( cpu_getstatus( 1 ) != 0 )
	{
		queue_data = data;
		queue_len = 4;
	}
}

static void player_reset( void )
{
	queue_len = 0;
	scode_last = -1;

	if( strcmp( Machine->gamedrv->name, "sfex2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfex2p" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfex2pj" ) == 0 ||
		strcmp( Machine->gamedrv->name, "tgmj" ) == 0 )
	{
		scode = 0x0400;
	}
	else if( strcmp( Machine->gamedrv->name, "techromn" ) == 0 ||
		strcmp( Machine->gamedrv->name, "kikaioh" ) == 0 )
	{
		scode = 0x8000;
	}
	else
	{
		scode = 0x0000;
	}
	cpu_set_reset_line( 0, PULSE_LINE );
	cpu_set_reset_line( 1, PULSE_LINE );
	if( n_playermode == 0 )
	{
		timer_suspendcpu( 0, 0, SUSPEND_ANY_REASON );
	}
	else
	{
		timer_suspendcpu( 0, 1, SUSPEND_REASON_DISABLE );
	}
}

static VIDEO_UPDATE( player )
{
	if( keyboard_pressed_memory( KEYCODE_F1 ) )
	{
		n_playermode = !n_playermode;
		player_reset();
	}

	if( n_playermode == 0 )
	{
		video_update_psx( bitmap, cliprect );
	}
	else
	{
		struct DisplayText dt[ 4 ];
		char text1[ 256 ];
		char text2[ 256 ];
		char text3[ 256 ];

		if( queue_len == 0 )
		{
			int stick;
			static int old_stick = 0x0f;

			stick = ~readinputport( 0 );
			if( ( stick & old_stick & 0x01 ) != 0 )
			{
				scode=( scode & 0xff00 ) | ( ( scode + 0x0001 ) & 0xff );
			}
			if( ( stick & old_stick & 0x02 ) != 0 )
			{
				scode=( scode & 0xff00 ) | ( ( scode - 0x0001 ) & 0xff );
			}
			if( ( stick & old_stick & 0x08 ) != 0 )
			{
				scode=( ( scode + 0x0100 ) & 0xff00 ) | ( scode & 0xff );
			}
			if( ( stick & old_stick & 0x04 ) != 0 )
			{
				scode=( ( scode - 0x0100 ) & 0xff00 ) | ( scode & 0xff );
			}
			old_stick = ~stick;
		}

		if( scode != scode_last )
		{
			player_queue_w( 0, scode, 0x0000ffff );
			scode_last = scode;
		}

		fillbitmap( bitmap, 0, &Machine->visible_area );

		sprintf( text1, "%s", Machine->gamedrv->description );
		if( strlen( text1 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text1[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		sprintf( text2, "SOUND CODE=%02x/%02x", scode >> 8, scode & 0xff );
		if( strlen( text2 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text2[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		sprintf( text3, "SELECT WITH RIGHT&LEFT/UP&DN" );
		if( strlen( text3 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text3[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		dt[ 0 ].text = text1;
		dt[ 0 ].color = UI_COLOR_NORMAL;
		dt[ 0 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 0 ].text ) ) / 2;
		dt[ 0 ].y = Machine->uiheight - Machine->uifontheight * 5;
		dt[ 1 ].text = text2;
		dt[ 1 ].color = UI_COLOR_NORMAL;
		dt[ 1 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 1 ].text ) ) / 2;
		dt[ 1 ].y = Machine->uiheight - Machine->uifontheight * 3;
		dt[ 2 ].text = text3;
		dt[ 2 ].color = UI_COLOR_NORMAL;
		dt[ 2 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 2 ].text ) ) / 2;
		dt[ 2 ].y = Machine->uiheight - Machine->uifontheight * 1;
		dt[ 3 ].text = 0; /* terminate array */
		displaytext( Machine->scrbitmap, dt );
	}
}

static void player_init( void )
{
	n_playermode = 0;
	player_reset();
}

static INTERRUPT_GEN( qsound_interrupt )
{
	if( queue_len == 4 )
	{
		soundlatch_w( 0, queue_data >> 8 );
		queue_len -= 2;
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	}
	else if( queue_len == 2 )
	{
		soundlatch_w( 0, queue_data & 0xff );
		queue_len -= 2;
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	}
	else
	{
		cpu_set_irq_line(1, 0, HOLD_LINE);
	}
}

static INTERRUPT_GEN( fx1a_sound_interrupt )
{
	if( queue_len == 4 )
	{
		taitosound_port_w( 0, 0 );
		taitosound_comm_w( 0, ( queue_data >> 0 ) & 0x0f );
		queue_len--;
	}
	else if( queue_len == 3 )
	{
		taitosound_port_w( 0, 1 );
		taitosound_comm_w( 0, ( queue_data >> 4 ) & 0x0f );
		queue_len--;
	}
	if( queue_len == 2 )
	{
		taitosound_port_w( 0, 2 );
		taitosound_comm_w( 0, ( queue_data >> 8 ) & 0x0f );
		queue_len--;
	}
	else if( queue_len == 1 )
	{
		taitosound_port_w( 0, 3 );
		taitosound_comm_w( 0, ( queue_data >> 12 ) & 0x0f );
		queue_len--;
	}
}

static struct PSXSPUinterface psxspu_interface =
{
	35,
};

static struct QSound_interface qsound_interface =
{
	QSOUND_CLOCK,
	REGION_SOUND1,
	{ 100,100 }
};

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irq_handler },
	{ REGION_SOUND1 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static void zn_machine_init( void )
{
	m_n_dip_bit = 0;
	psx_machine_init();
}

static READ32_HANDLER( capcom_kickharness_r )
{
	/* required for buttons 4,5&6 */
	verboselog( 2, "capcom_kickharness_r( %08x, %08x )\n", offset, mem_mask );
	return 0xffffffff;
}

static WRITE32_HANDLER( bank_coh1000c_w )
{
	cpu_setbank( 2, memory_region( REGION_USER2 ) + 0x400000 + ( data * 0x400000 ) );
}

DRIVER_INIT( coh1000c )
{
	install_mem_read32_handler ( 0, 0x1f000000, 0x1f3fffff, MRA32_BANK1 );     /* fixed game rom */
	install_mem_read32_handler ( 0, 0x1f400000, 0x1f7fffff, MRA32_BANK2 );     /* banked game rom */
	install_mem_write32_handler( 0, 0x1fb00000, 0x1fb00003, bank_coh1000c_w ); /* bankswitch */
	install_mem_read32_handler ( 0, 0x1fb40010, 0x1fb40013, capcom_kickharness_r );
	install_mem_read32_handler ( 0, 0x1fb40020, 0x1fb40023, capcom_kickharness_r );
	install_mem_read32_handler ( 0, 0x1fb80000, 0x1fbfffff, MRA32_BANK3 );     /* country rom */
	install_mem_write32_handler( 0, 0x1fb60000, 0x1fb60003, zn_qsound_w );

	init_znsec();
}

MACHINE_INIT( coh1000c )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	cpu_setbank( 2, memory_region( REGION_USER2 ) + 0x400000 ); /* banked game rom */
	cpu_setbank( 3, memory_region( REGION_USER3 ) ); /* country rom */
	zn_machine_init();
	player_init();
}

static MACHINE_DRIVER_START( coh1000c )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	MDRV_CPU_FLAGS( CPU_AUDIO_CPU )  /* 8MHz ?? */
	MDRV_CPU_PROGRAM_MAP( qsound_readmem, qsound_writemem )
	MDRV_CPU_IO_MAP( qsound_readport, 0 )
	MDRV_CPU_VBLANK_INT( qsound_interrupt, 4 ) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1000c )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x512 )
	MDRV_VIDEO_UPDATE( player )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
	MDRV_SOUND_ADD( QSOUND, qsound_interface )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh1002c )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	MDRV_CPU_FLAGS( CPU_AUDIO_CPU )  /* 8MHz ?? */
	MDRV_CPU_PROGRAM_MAP( qsound_readmem, qsound_writemem )
	MDRV_CPU_IO_MAP( qsound_readport, 0 )
	MDRV_CPU_VBLANK_INT( qsound_interrupt, 4 ) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1000c )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( player )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
	MDRV_SOUND_ADD( QSOUND, qsound_interface )
MACHINE_DRIVER_END

static size_t taitofx1_eeprom_size;
static data8_t *taitofx1_eeprom;

static WRITE32_HANDLER( bank_coh1000t_w )
{
	verboselog( 1, "bank_coh1000t_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
	cpu_setbank( 1, memory_region( REGION_USER2 ) + ( ( data & 3 ) * 0x800000 ) );
}

static WRITE32_HANDLER( taitofx1_volume_w )
{
	verboselog( 1, "taitofx1_volume_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static WRITE32_HANDLER( taitofx1_sound_w )
{
	verboselog( 1, "taitofx1_sound_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static READ32_HANDLER( taitofx1_sound_r )
{
	data32_t data = 0; // bit 0 = busy?
	verboselog( 1, "taitofx1_sound_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

DRIVER_INIT( coh1000t )
{
	taitofx1_eeprom_size = 0x200; taitofx1_eeprom = auto_malloc( taitofx1_eeprom_size );

	install_mem_read32_handler ( 0, 0x1f000000, 0x1f7fffff, MRA32_BANK1 );     /* banked game rom */
	install_mem_read32_handler ( 0, 0x1fa40000, 0x1fa40003, MRA32_NOP );
	install_mem_write32_handler( 0, 0x1fa20000, 0x1fa20003, MWA32_NOP );
	install_mem_write32_handler( 0, 0x1fa30000, 0x1fa30003, MWA32_NOP );
	install_mem_write32_handler( 0, 0x1fb40000, 0x1fb40003, bank_coh1000t_w ); /* bankswitch */
	install_mem_write32_handler( 0, 0x1fb80000, 0x1fb80003, taitofx1_volume_w );
	install_mem_write32_handler( 0, 0x1fba0000, 0x1fba0003, taitofx1_sound_w );
	install_mem_read32_handler ( 0, 0x1fbc0000, 0x1fbc0003, taitofx1_sound_r );
	install_mem_read32_handler ( 0, 0x1fbe0000, 0x1fbe0000 + ( taitofx1_eeprom_size - 1 ), MRA32_BANK2 );
	install_mem_write32_handler( 0, 0x1fbe0000, 0x1fbe0000 + ( taitofx1_eeprom_size - 1 ), MWA32_BANK2 );

	init_znsec();
}

MACHINE_INIT( coh1000t )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* banked game rom */
	cpu_setbank( 2, taitofx1_eeprom );
	zn_machine_init();
	player_init();
}

static NVRAM_HANDLER( coh1000t )
{
	if (read_or_write)
	{
		mame_fwrite(file, generic_nvram, generic_nvram_size);
		mame_fwrite(file, taitofx1_eeprom, taitofx1_eeprom_size);
	}
	else if (file)
	{
		mame_fread(file, generic_nvram, generic_nvram_size);
		mame_fread(file, taitofx1_eeprom, taitofx1_eeprom_size);
	}
	else
		memset(generic_nvram, 0, generic_nvram_size);
}

static MACHINE_DRIVER_START( coh1000ta )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 16000000 / 4 )
	MDRV_CPU_FLAGS( CPU_AUDIO_CPU )	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP( fx1a_sound_readmem, fx1a_sound_writemem )
	MDRV_CPU_VBLANK_INT( fx1a_sound_interrupt, 1 ) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1000t )
	MDRV_NVRAM_HANDLER( coh1000t )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x512 )
	MDRV_VIDEO_UPDATE( player )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
	MDRV_SOUND_ADD( YM2610, ym2610_interface )
MACHINE_DRIVER_END

INTERRUPT_GEN( coh1000tb_vblank )
{
	/* kludge: stop dropping into test mode on bootup */
	if( strcmp( Machine->gamedrv->name, "raystorm" ) == 0 )
	{
		if( g_p_n_psxram[ 0x1b358 / 4 ] == 0x34020001 )
		{
			g_p_n_psxram[ 0x1b358 / 4 ] = 0x34020000;
		}
	}
	psx_vblank();
}

static MACHINE_DRIVER_START( coh1000tb )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( coh1000tb_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1000t )
	MDRV_NVRAM_HANDLER( coh1000t )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x512 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END


static WRITE32_HANDLER( bank_coh3002c_w )
{
	cpu_setbank( 2, memory_region( REGION_USER2 ) + 0x400000 + ( data * 0x400000 ) );
}

DRIVER_INIT( coh3002c )
{
	install_mem_read32_handler ( 0, 0x1f000000, 0x1f3fffff, MRA32_BANK1 );     /* fixed game rom */
	install_mem_read32_handler ( 0, 0x1f400000, 0x1f7fffff, MRA32_BANK2 );     /* banked game rom */
	install_mem_read32_handler ( 0, 0x1fa60000, 0x1fa60003, MRA32_NOP );
	install_mem_read32_handler ( 0, 0x1fb40010, 0x1fb40013, capcom_kickharness_r );
	install_mem_read32_handler ( 0, 0x1fb40020, 0x1fb40023, capcom_kickharness_r );
	install_mem_write32_handler( 0, 0x1fb00000, 0x1fb00003, bank_coh3002c_w ); /* bankswitch */
	install_mem_read32_handler ( 0, 0x1fb80000, 0x1fbfffff, MRA32_BANK3 );     /* country rom */
	install_mem_write32_handler( 0, 0x1fb60000, 0x1fb60003, zn_qsound_w );

	init_znsec();
}

MACHINE_INIT( coh3002c )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	cpu_setbank( 2, memory_region( REGION_USER2 ) + 0x400000 ); /* banked game rom */
	cpu_setbank( 3, memory_region( REGION_USER3 ) ); /* country rom */
	zn_machine_init();
	player_init();
}

static MACHINE_DRIVER_START( coh3002c )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	MDRV_CPU_FLAGS( CPU_AUDIO_CPU )  /* 8MHz ?? */
	MDRV_CPU_PROGRAM_MAP( qsound_readmem, qsound_writemem )
	MDRV_CPU_IO_MAP( qsound_readport, 0 )
	MDRV_CPU_VBLANK_INT( qsound_interrupt, 4 ) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh3002c )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( player )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
	MDRV_SOUND_ADD( QSOUND, qsound_interface )
MACHINE_DRIVER_END

static void atpsx_interrupt(int state)
{
	if (state)
	{
	 	psx_irq_set(0x400);
	}
}

static struct ide_interface atpsx_intf =
{
	atpsx_interrupt
};

INLINE void psxwritebyte( UINT32 n_address, UINT8 n_data )
{
	*( (UINT8 *)g_p_n_psxram + BYTE_XOR_LE( n_address ) ) = n_data;
}

static void atpsx_dma_read( UINT32 n_address, INT32 n_size )
{
	logerror("DMA read: %d bytes (%d words) to %08x\n", n_size<<2, n_size, n_address);

	if (n_address < 0xe0000)
	{
		// protect kernel+program space (what should we really do here?)
		logerror( "skip read to low memory\n" );
		return;
	}

	/* dma size is in 32-bit words, convert to bytes */
	n_size <<= 2;
	while( n_size > 0 )
	{
		psxwritebyte( n_address, ide_controller32_0_r( 0x1f0 / 4, 0xffffff00 ) );
		n_address++;
		n_size--;
	}
}

static void atpsx_dma_write( UINT32 n_address, INT32 n_size )
{
	logerror("DMA write from %08x for %d bytes\n", n_address, n_size<<2);
}

DRIVER_INIT( coh1000w )
{
	install_mem_read32_handler ( 0, 0x1f000000, 0x1f1fffff, MRA32_BANK1 );
	install_mem_write32_handler( 0, 0x1f000000, 0x1f000003, MWA32_NOP );
	install_mem_write32_handler( 0, 0x1fa20000, 0x1fa20003, MWA32_NOP );
	install_mem_read32_handler ( 0, 0x1fa30000, 0x1fa30003, MRA32_NOP );
	install_mem_write32_handler( 0, 0x1fa30000, 0x1fa30003, MWA32_NOP );
	install_mem_read32_handler ( 0, 0x1fa40000, 0x1fa40003, MRA32_NOP );
	install_mem_read32_handler ( 0, 0x1f7e4000, 0x1f7e4fff, ide_controller32_0_r );
	install_mem_write32_handler( 0, 0x1f7e4000, 0x1f7e4fff, ide_controller32_0_w );
	install_mem_write32_handler( 0, 0x1f7e8000, 0x1f7e8003, MWA32_NOP );
	install_mem_read32_handler ( 0, 0x1f7e8000, 0x1f7e8003, MRA32_NOP );
	install_mem_read32_handler ( 0, 0x1f7f4000, 0x1f7f4fff, ide_controller32_0_r );
	install_mem_write32_handler( 0, 0x1f7f4000, 0x1f7f4fff, ide_controller32_0_w );

	// init hard disk
	ide_controller_init(0, &atpsx_intf);

	init_znsec();
}

MACHINE_INIT( coh1000w )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	zn_machine_init();

	ide_controller_reset(0);
	psx_dma_install_read_handler(5, atpsx_dma_read);
	psx_dma_install_write_handler(5, atpsx_dma_write);
}

static MACHINE_DRIVER_START( coh1000w )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1000w )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END

static WRITE32_HANDLER( coh1002e_bank_w )
{
	znsecsel_w( offset, data, mem_mask );

	cpu_setbank( 1, memory_region( REGION_USER2 ) + ( ( data & 1 ) * 0x400000 ) );
}

static WRITE32_HANDLER( coh1002e_latch_w )
{
	if (offset)
	{
		cpu_set_irq_line(1, 2, HOLD_LINE);	// irq 2 on the 68k
	}
	else
	{
		soundlatch_w(0, data);
	}
}

DRIVER_INIT( coh1002e )
{
	install_mem_read32_handler( 0, 0x1f000000, 0x1f3fffff, MRA32_BANK1 );
	install_mem_read32_handler( 0, 0x1f400000, 0x1f7fffff, MRA32_BANK2 );
	install_mem_write32_handler( 0, 0x1fa10300, 0x1fa10303, coh1002e_bank_w );
	install_mem_write32_handler( 0, 0x1fb00000, 0x1fb00007, coh1002e_latch_w );

	init_znsec();
}

MACHINE_INIT( coh1002e )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* banked game rom */
	cpu_setbank( 2, memory_region( REGION_USER2 ) + 0x800000 ); /* fixed game rom */
	zn_machine_init();
}

static READ16_HANDLER( psarc_ymf_r )
{
	return YMF271_0_r(0);
}

static WRITE16_HANDLER( psarc_ymf_w )
{
	YMF271_0_w(offset, data);
}

static READ16_HANDLER( psarc_latch_r )
{
	return soundlatch_r(0);
}

static ADDRESS_MAP_START( psarc_snd_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x0fffff) AM_RAM
	AM_RANGE(0x100000, 0x10001f) AM_READWRITE( psarc_ymf_r, psarc_ymf_w )
	AM_RANGE(0x180008, 0x180009) AM_READ( psarc_latch_r )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITENOP
	AM_RANGE(0x100020, 0xffffff) AM_WRITENOP
ADDRESS_MAP_END

static struct YMF271interface ymf271_interface =
{
	1,
	{ REGION_SOUND1, },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT), },
};

static MACHINE_DRIVER_START( coh1002e )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( M68000, 8000000 )
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP( psarc_snd_map, 0 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1002e )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
	MDRV_SOUND_ADD( YMF271, ymf271_interface )
MACHINE_DRIVER_END

static void jdredd_ide_interrupt(int state)
{
	if (state)
	{
	 	psx_irq_set(0x400);
	}
}

static struct ide_interface jdredd_ide_intf =
{
	jdredd_ide_interrupt
};

static READ32_HANDLER( jdredd_idestat_r )
{
	return ide_controller_0_r(0x1f7);
}

static READ32_HANDLER( jdredd_unknown_r )
{
	return 0xffffffff;
}

static READ32_HANDLER( jdredd_ideunknown_r )
{
	return 0xffffffff-2;
}

static READ32_HANDLER( jdredd_ide_r)
{
	int reg = offset*2;
	int shift = 0;
	int ret;

	if (mem_mask == 0xff00ffff)
	{
		shift = 16;
		reg++;
	}
	else if (mem_mask == 0xffff0000)	// code sometimes reads shorts from the DATA register
	{
		if (reg != 0)
		{
			logerror("JDREDD IDE: read 16-bit from non-DATA register %d!\n", reg);
		}

		return (ide_controller_0_r(0x1f0) | (ide_controller_0_r(0x1f0)<<8));
	}

	ret = ide_controller_0_r(0x1f0 + reg) << shift;

	return ret;
}

static WRITE32_HANDLER( jdredd_ide_w )
{
	int reg = offset*2;

	if (mem_mask == 0xff00ffff)
	{
		data >>= 16;
		reg++;
	}

	ide_controller_0_w(0x1f0 + reg, data);
}

static size_t nbajamex_eeprom_size;
static data8_t *nbajamex_eeprom;

static WRITE32_HANDLER( acpsx_00_w )
{
	verboselog( 0, "acpsx_00_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static WRITE32_HANDLER( acpsx_10_w )
{
	verboselog( 0, "acpsx_10_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static WRITE32_HANDLER( nbajamex_80_w )
{
	verboselog( 0, "nbajamex_80_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
	psx_irq_set(0x400);
}

static READ32_HANDLER( nbajamex_08_r )
{
	data32_t data = 0xffffffff;
	verboselog( 0, "nbajamex_08_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

static READ32_HANDLER( nbajamex_80_r )
{
	data32_t data = 0xffffffff;
	verboselog( 0, "nbajamex_80_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

DRIVER_INIT( coh1000a )
{
	install_mem_read32_handler( 0, 0x1f000000, 0x1f1fffff, MRA32_BANK1 );
	install_mem_write32_handler( 0, 0x1fbfff00, 0x1fbfff03, acpsx_00_w );
	install_mem_write32_handler( 0, 0x1fbfff10, 0x1fbfff13, acpsx_10_w );

	if( strcmp( Machine->gamedrv->name, "nbajamex" ) == 0 )
	{
		nbajamex_eeprom_size = 0x8000; nbajamex_eeprom = auto_malloc( nbajamex_eeprom_size );

		install_mem_read32_handler ( 0, 0x1f200000, 0x1f200000 + ( nbajamex_eeprom_size - 1 ), MRA32_BANK2 );
		install_mem_write32_handler( 0, 0x1f200000, 0x1f200000 + ( nbajamex_eeprom_size - 1 ), MWA32_BANK2 );
		install_mem_write32_handler( 0, 0x1fbfff80, 0x1fbfff83, nbajamex_80_w );
		install_mem_read32_handler ( 0, 0x1fbfff08, 0x1fbfff0b, nbajamex_08_r );
		install_mem_read32_handler ( 0, 0x1fbfff80, 0x1fbfff83, nbajamex_80_r );

		cpu_setbank( 2, nbajamex_eeprom ); /* ram/eeprom/?? */
	}

	if( ( !strcmp( Machine->gamedrv->name, "jdredd" ) ) ||
		( !strcmp( Machine->gamedrv->name, "jdreddb" ) ) )
	{
		install_mem_read32_handler ( 0, 0x1fa30000, 0x1fa30003, jdredd_unknown_r );
		install_mem_read32_handler ( 0, 0x1fa40000, 0x1fa40003, jdredd_ideunknown_r );
		install_mem_read32_handler ( 0, 0x1fbfff8c, 0x1fbfff8f, jdredd_idestat_r );
		install_mem_write32_handler( 0, 0x1fbfff8c, 0x1fbfff8f, MWA32_NOP );
		install_mem_read32_handler ( 0, 0x1fbfff90, 0x1fbfff9f, jdredd_ide_r );
		install_mem_write32_handler( 0, 0x1fbfff90, 0x1fbfff9f, jdredd_ide_w );

		ide_controller_init( 0, &jdredd_ide_intf );
	}

	init_znsec();
}

MACHINE_INIT( coh1000a )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	zn_machine_init();
	if( ( !strcmp( Machine->gamedrv->name, "jdredd" ) ) ||
		( !strcmp( Machine->gamedrv->name, "jdreddb" ) ) )
	{
		ide_controller_reset( 0 );
	}
}

static MACHINE_DRIVER_START( coh1000a )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1000a )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END

DRIVER_INIT( coh1002v )
{
	install_mem_read32_handler( 0, 0x1f000000, 0x1f7fffff, MRA32_BANK1 );

	init_znsec();
}

MACHINE_INIT( coh1002v )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	zn_machine_init();
}

static MACHINE_DRIVER_START( coh1002v )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1002v )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END

DRIVER_INIT( coh3002t )
{
	install_mem_read32_handler( 0, 0x1f000000, 0x1f7fffff, MRA32_BANK1 );

	init_znsec();
}

MACHINE_INIT( coh3002t )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	zn_machine_init();
}

static MACHINE_DRIVER_START( coh3002t )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh3002t )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END

static WRITE32_HANDLER( coh1002m_bank_w )
{
	verboselog( 1, "coh1002m_bank_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
	cpu_setbank( 1, memory_region( REGION_USER2 ) + ((data>>16) * 0x800000));
}

static WRITE32_HANDLER( coh1002m_watchdog_w )
{
}

static int cbaj_to_z80 = 0, cbaj_to_r3k = 0;
static int latch_to_z80;

static READ32_HANDLER( cbaj_z80_r )
{
	int ready = cbaj_to_r3k;

	cbaj_to_r3k &= ~2;

	return soundlatch2_r(0) | ready<<24;
}

static WRITE32_HANDLER( cbaj_z80_w )
{
	cbaj_to_z80 |= 2;
	latch_to_z80 = data;
}

DRIVER_INIT( coh1002m )
{
	install_mem_read32_handler( 0, 0x1f000000, 0x1f7fffff, MRA32_BANK1 );
	install_mem_write32_handler( 0, 0x1fa20000, 0x1fa20003, coh1002m_watchdog_w );
	install_mem_read32_handler( 0, 0x1fb00000, 0x1fb00003, cbaj_z80_r );
	install_mem_write32_handler( 0, 0x1fb00000, 0x1fb00003, cbaj_z80_w );
	install_mem_write32_handler( 0, 0x1fb00004, 0x1fb00007, coh1002m_bank_w );

	init_znsec();
}

MACHINE_INIT( coh1002m )
{
	cpu_setbank( 1, memory_region( REGION_USER2 ) );
	zn_machine_init();
}

static READ_HANDLER( cbaj_z80_latch_r )
{
	cbaj_to_z80 &= ~2;
	return latch_to_z80;
}

static WRITE_HANDLER( cbaj_z80_latch_w )
{
	cbaj_to_r3k |= 2;
	soundlatch2_w(0, data);
}

static READ_HANDLER( cbaj_z80_ready_r )
{
	int ret = cbaj_to_z80;

	cbaj_to_z80 &= ~2;

	return ret;
}

ADDRESS_MAP_START( cbaj_z80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0xffff ) AM_RAM
ADDRESS_MAP_END

ADDRESS_MAP_START( cbaj_z80_port_map, ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x84, 0x84 ) AM_READWRITE( YMZ280B_status_0_r, YMZ280B_register_0_w )
	AM_RANGE( 0x85, 0x85 ) AM_READWRITE( YMZ280B_status_0_r, YMZ280B_data_0_w )
	AM_RANGE( 0x90, 0x90 ) AM_READWRITE( cbaj_z80_latch_r, cbaj_z80_latch_w )
	AM_RANGE( 0x91, 0x91 ) AM_READ( cbaj_z80_ready_r )
ADDRESS_MAP_END

static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 16934400 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }
};

static MACHINE_DRIVER_START( coh1002m )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1002m )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh1002msnd )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	MDRV_CPU_FLAGS( CPU_AUDIO_CPU )
	MDRV_CPU_PROGRAM_MAP( cbaj_z80_map, 0 )
	MDRV_CPU_IO_MAP( cbaj_z80_port_map, 0 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1002m )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
	MDRV_SOUND_ADD( YMZ280B, ymz280b_intf )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh1002ml )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	MDRV_CPU_PROGRAM_MAP( link_readmem, link_writemem )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( coh1002m )
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END

INPUT_PORTS_START( zn )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN2 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xcc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN4 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN5 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN7 */
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/* Capcom ZN1 */

#define CPZN1_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1000c.353", 0x0000000, 0x080000, CRC(50033af6) SHA1(486d92ff6c7f1e54f8e0ef41cd9116eca0e10e1a) )

ROM_START( cpzn1 )
	CPZN1_BIOS
ROM_END

ROM_START( glpracr )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "graj-04.2j", 0x0000000, 0x080000, CRC(53bf551c) SHA1(320632b5010630cee4c5ccb1578d5ee6d2754632) )

	ROM_REGION32_LE( 0x0c00000, REGION_USER2, 0 )
	ROM_LOAD( "gra-05m.3j", 0x0000000, 0x400000, CRC(78053700) SHA1(38727c8cc34bb57b7b7e73041e382fb0361f184e) )
	ROM_LOAD( "gra-06m.4j", 0x0400000, 0x400000, CRC(d73b392b) SHA1(241ddf474cea035e81a2abc580d3c0395ee925bb) )
	ROM_LOAD( "gra-07m.5j", 0x0800000, 0x400000, CRC(acaefe3a) SHA1(32d596b0f975e1558fa7929c3166d8dad40a1c80) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_FILL( 0, 0x50000, 0x76 )
/* there are no QSound program roms
	ROM_LOAD( "gra-02",  0x00000, 0x08000, NO_DUMP )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "gra-03",  0x28000, 0x20000, NO_DUMP )
*/
	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
/* or QSound sample roms either
	ROM_LOAD( "gra-01m", 0x0000000, 0x400000, NO_DUMP )
*/
ROM_END

ROM_START( sfex )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfeu-04b", 0x0000000, 0x080000, CRC(de02bd29) SHA1(62a88a30f73db661f5b98fc7e2d34d51acb965cc) )

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-05m", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexa )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfea-04", 0x0000000, 0x080000, CRC(08247bd4) SHA1(07f356ef2827b3fbd0bfaf2010915315d9d60ef1) )

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-05m", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfej-04", 0x0000000, 0x080000, CRC(ea100607) SHA1(27ef8c619804999d32d14fcc5ec783c057b4dc73) )

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-05m", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( starglad )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ps1u-04.2h",   0x000000, 0x080000, CRC(121fb234) SHA1(697d18d37afd95f302b40a5a6a78d8c92a41ea73) )

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "ps1-05m.3h", 0x0000000, 0x400000, CRC(8ad72c4f) SHA1(c848c37eb5365000b4d4720b5c08d89ddd8e2c33) )
	ROM_LOAD( "ps1-06m.4h", 0x0400000, 0x400000, CRC(95d8ed61) SHA1(e9f259d589dc38a8321a6fea1f5dac741cadc0ff) )
	ROM_LOAD( "ps1-07m.5h", 0x0800000, 0x400000, CRC(c06752db) SHA1(0884b308e9cd9dde8660b422bc8fec9a362bcb52) )
	ROM_LOAD( "ps1-08m.2k", 0x0c00000, 0x400000, CRC(381f9ded) SHA1(b7878a90740f5b3c5881ac7d46e2b84b18727337) )
	ROM_LOAD( "ps1-09m.3k", 0x1000000, 0x400000, CRC(bd894812) SHA1(9f0c3365e685a53ae793f4a256a6c177a843a424) )
	ROM_LOAD( "ps1-10m.4k", 0x1400000, 0x400000, CRC(ff80c18a) SHA1(8d01717eed6ec1f508fe7c445da941fb84ef7d22) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ps1-02a.2e",   0x000000, 0x008000, CRC(b854df92) SHA1(ea71a613b5b19ec7e9c6e342e7743d320582a6bb) )
	ROM_CONTINUE(             0x010000, 0x018000 )
	ROM_LOAD( "ps1-03a.3e",   0x028000, 0x020000, CRC(a2562fbb) SHA1(3de02a4aa7ea620961ca2a5c331f38134033db79) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ps1-01m.3b",   0x000000, 0x400000, CRC(0bfb17aa) SHA1(cf4482785a2a33ad814c8b1461c5bc8e8e027895) )
ROM_END

ROM_START( ts2 )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ts2u-04", 0x0000000, 0x080000, CRC(ddb52e7c) SHA1(e77891abae7681d911ef6eba2e0920d81433ebe6) )

	ROM_REGION32_LE( 0x0e00000, REGION_USER2, 0 )
	ROM_LOAD( "ts2-05",  0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m", 0x0400000, 0x400000, CRC(cd7e0a27) SHA1(325b5f2e653cdea07cddc9d20d12b5ab50dca949) )
	ROM_LOAD( "ts2-08m", 0x0800000, 0x400000, CRC(b1f7f115) SHA1(3f416d2aac07aa73a99593b5a21b047da60cea6a) )
	ROM_LOAD( "ts2-10",  0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2-02",  0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ts2-01",  0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )
ROM_END

ROM_START( ts2j )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ts2j-04", 0x0000000, 0x080000, CRC(4aba8c5e) SHA1(a56001bf50bfc1b03036e88ae1febd1aac8c63c0) )

	ROM_REGION32_LE( 0x0e00000, REGION_USER2, 0 )
	ROM_LOAD( "ts2-05",  0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m", 0x0400000, 0x400000, CRC(cd7e0a27) SHA1(325b5f2e653cdea07cddc9d20d12b5ab50dca949) )
	ROM_LOAD( "ts2-08m", 0x0800000, 0x400000, CRC(b1f7f115) SHA1(3f416d2aac07aa73a99593b5a21b047da60cea6a) )
	ROM_LOAD( "ts2-10",  0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2-02",  0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ts2-01",  0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )
ROM_END

/* Capcom ZN2 */

#define CPZN2_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-3002c.353", 0x0000000, 0x080000, CRC(e860ea8b) SHA1(66e7e1d4e426466b8f48a2ba055a91b475569504) )

ROM_START( cpzn2 )
	CPZN2_BIOS
ROM_END

ROM_START( rvschool )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "jsta-04", 0x0000000, 0x080000, CRC(034b1011) SHA1(6773246be242ee336503d21d7d44a3884832eb1e) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "jst-05m", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )
ROM_END

ROM_START( jgakuen )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "jstj-04", 0x0000000, 0x080000, CRC(28b8000a) SHA1(9ebf74b453d775cadca9c2d7d8e2c7eb57bb9a38) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "jst-05m", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )
ROM_END

ROM_START( tgmj )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "atej-04", 0x0000000, 0x080000, CRC(bb4bbb96) SHA1(808f4b29493e74efd661d561d11cbec2f4afd1c8) )

	ROM_REGION32_LE( 0x0800000, REGION_USER2, 0 )
	ROM_LOAD( "ate-05",  0x0000000, 0x400000, CRC(50977f5a) SHA1(78c2b1965957ff1756c25b76e549f11fc0001153) )
	ROM_LOAD( "ate-06",  0x0400000, 0x400000, CRC(05973f16) SHA1(c9262e8de14c4a9489f7050316012913c1caf0ff) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ate-02",  0x00000, 0x08000, CRC(f4f6e82f) SHA1(ad6c49197a60f456367c9f78353741fb847819a1) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ate-01",  0x0000000, 0x400000, CRC(a21c6521) SHA1(560e4855f6e00def5277bdd12064b49e55c3b46b) )
ROM_END

ROM_START( techromn )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "kiou-04",     0x0000000, 0x080000, CRC(08aca34a) SHA1(768a37f719af5d96993db5592b6505b013e0d6f4) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "kio-05m.bin", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.bin", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.bin", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.bin", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.bin", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.bin", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio-02.bin",  0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(            0x10000, 0x18000 )
	ROM_LOAD( "kio-03.bin",  0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "kio-01m.bin", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )
ROM_END

ROM_START( kikaioh )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "kioj-04",     0x0000000, 0x080000, CRC(3a2a3bc8) SHA1(3c4ae3cfe00a7f60ab2196ae042dab4a8eb6f597) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "kio-05m.bin", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.bin", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.bin", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.bin", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.bin", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.bin", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio-02.bin",  0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(            0x10000, 0x18000 )
	ROM_LOAD( "kio-03.bin",  0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "kio-01m.bin", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )
ROM_END

ROM_START( sfexp )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfpe-04", 0x0000000, 0x080000, CRC(305e4ec0) SHA1(0df9572d7fc1bbc7131483960771d016fa5487a5) )

	ROM_REGION32_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-05",  0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06",  0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07",  0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08",  0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09",  0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10",  0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexpj )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfpj-04", 0x0000000, 0x080000, CRC(18d043f5) SHA1(9e6e24a722d13888fbfd391ddb1a5045b162488c) )

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-05",  0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06",  0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07",  0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08",  0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09",  0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10",  0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfex2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ex2j-04", 0x0000000, 0x080000, CRC(5d603586) SHA1(ff546d3bd011d6441e9672b88bab763d3cd89be2) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ex2-05m", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2-02",  0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )
ROM_END

ROM_START( sfex2p )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sf2pu-04", 0x0000000, 0x080000, CRC(6179f937) SHA1(5e3664b5ebb25ba1c1730020e9dbffccf083fb03) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "sf2p-05", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "sf2p-06", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "sf2p-07", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "sf2p-08", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "sf2p-09", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "sf2p-10", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2p-02", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sf2p-03", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )
ROM_END

ROM_START( sfex2pj )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sf2pj-04", 0x0000000, 0x080000, CRC(c6d0aea3) SHA1(f48ee889dd743109f830063da3eb0f687db2d86c) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "sf2p-05", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "sf2p-06", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "sf2p-07", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "sf2p-08", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "sf2p-09", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "sf2p-10", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2p-02", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sf2p-03", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )
ROM_END

ROM_START( plsmaswd )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sg2u-04", 0x0000000, 0x080000, CRC(154187c0) SHA1(58cc0e9d32786b1c1d64ecee4667190456b36ef6) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sg2-05m", 0x0000000, 0x800000, CRC(f1759236) SHA1(fbe3a820a8c571dfb186eae68346e6461168ed48) )
	ROM_LOAD( "sg2-06m", 0x0800000, 0x800000, CRC(33de4f72) SHA1(ab32af76b5682e3d9f67dadbaed35abc043912b4) )
	ROM_LOAD( "sg2-07m", 0x1000000, 0x800000, CRC(72f724ba) SHA1(e6658b495d308d1de6710f87b5b9d346008b0c5a) )
	ROM_LOAD( "sg2-08m", 0x1800000, 0x800000, CRC(9e169eee) SHA1(6141b1a7863fdfb200ca35d2893979a34dcc3f6c) )
	ROM_LOAD( "sg2-09m", 0x2000000, 0x400000, CRC(33f73d4c) SHA1(954695a43e77b58585409678bd87c76adac1d855) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sg2-02",  0x00000, 0x08000, CRC(415ee138) SHA1(626083c8705f012552691c450f95401ddc88065b) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sg2-03",  0x28000, 0x20000, CRC(43806735) SHA1(88d389bcc79cbd4fa1f4b62008e171a897e77652) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sg2-01m", 0x0000000, 0x400000, CRC(643ea27b) SHA1(40747432d5cfebac54d3824b6a6f26b5e7742fc1) )
ROM_END

ROM_START( stargld2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sg2j-04", 0x0000000, 0x080000, CRC(cf4ce6ac) SHA1(52b6f61d79671c9c108b3dfbd3c2ac333285412c) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sg2-05m", 0x0000000, 0x800000, CRC(f1759236) SHA1(fbe3a820a8c571dfb186eae68346e6461168ed48) )
	ROM_LOAD( "sg2-06m", 0x0800000, 0x800000, CRC(33de4f72) SHA1(ab32af76b5682e3d9f67dadbaed35abc043912b4) )
	ROM_LOAD( "sg2-07m", 0x1000000, 0x800000, CRC(72f724ba) SHA1(e6658b495d308d1de6710f87b5b9d346008b0c5a) )
	ROM_LOAD( "sg2-08m", 0x1800000, 0x800000, CRC(9e169eee) SHA1(6141b1a7863fdfb200ca35d2893979a34dcc3f6c) )
	ROM_LOAD( "sg2-09m", 0x2000000, 0x400000, CRC(33f73d4c) SHA1(954695a43e77b58585409678bd87c76adac1d855) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sg2-02",  0x00000, 0x08000, CRC(415ee138) SHA1(626083c8705f012552691c450f95401ddc88065b) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sg2-03",  0x28000, 0x20000, CRC(43806735) SHA1(88d389bcc79cbd4fa1f4b62008e171a897e77652) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sg2-01m", 0x0000000, 0x400000, CRC(643ea27b) SHA1(40747432d5cfebac54d3824b6a6f26b5e7742fc1) )
ROM_END

ROM_START( strider2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "hr2a-04",   0x000000, 0x080000, CRC(56ff9394) SHA1(fe8417965d945210ac098c6678c02f1c678bd13b) )

	ROM_REGION32_LE( 0x2c00000, REGION_USER2, 0 )
	ROM_LOAD( "hr2-05m.bin", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.bin", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.bin", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.bin", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.bin", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.bin", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2-02.bin",  0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(            0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "hr2-01m.bin", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )
	ROM_RELOAD( 0x0200000, 0x200000 )
ROM_END

ROM_START( shiryu2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "hr2j-04", 0x0000000, 0x080000, CRC(0824ee5f) SHA1(a296ffe03f0d947deb9803d05de3c240a26b52bb) )

	ROM_REGION32_LE( 0x2c00000, REGION_USER2, 0 )
	ROM_LOAD( "hr2-05m.bin", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.bin", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.bin", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.bin", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.bin", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.bin", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2-02.bin",  0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(            0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "hr2-01m.bin", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )
	ROM_RELOAD( 0x0200000, 0x200000 )
ROM_END


/* Tecmo */

#define TPS_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1002m.353", 0x0000000, 0x080000, CRC(69ffbcb4) SHA1(03eb2febfab3fcde716defff291babd9392de965) )

ROM_START( tps )
	TPS_BIOS
ROM_END

/*

 TPS ROM addressing note: .216 *if present* goes at 0x400000, else nothing there.
 .217 goes at 0x800000, .218 at 0xc00000, .219 at 0x1000000, and so on.

*/

ROM_START( glpracr2 )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "gallop2u.119", 0x0000001, 0x100000, CRC(9899911c) SHA1(f043fb97760c53422ad6aeb214474c0be00017ce) )
	ROM_LOAD16_BYTE( "gallop2u.120", 0x0000000, 0x100000, CRC(fd69bd4b) SHA1(26a183bdc3b2fb3d93bd7694e429a676106f4e58) )
	ROM_LOAD( "gra2-0.217",          0x0800000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0c00000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x1000000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x1400000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1800000, 0x400000, CRC(2b070307) SHA1(43c028aaca297358f87c6633c2020d71e34317b8) )
	ROM_LOAD( "gra2-5.222",          0x1c00000, 0x400000, CRC(94a363c1) SHA1(4c53822a672ac99b001c9fe82f9d0f8496989e67) )
	ROM_LOAD( "gra2-6.223",          0x2000000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x2400000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )
ROM_END

ROM_START( glprac2j )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "1.119",        0x0000001, 0x100000, CRC(0fe2d2df) SHA1(031369f4e1138e2ee293c321e5ee418e560b3f06) )
	ROM_LOAD16_BYTE( "2.120",        0x0000000, 0x100000, CRC(8e3fb1c0) SHA1(2126c1e43bee7cd938e0f2a3ea841da8811223cd) )
	ROM_LOAD( "gra2-0.217",          0x0800000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0c00000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x1000000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x1400000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1800000, 0x400000, CRC(2b070307) SHA1(43c028aaca297358f87c6633c2020d71e34317b8) )
	ROM_LOAD( "gra2-5.222",          0x1c00000, 0x400000, CRC(94a363c1) SHA1(4c53822a672ac99b001c9fe82f9d0f8496989e67) )
	ROM_LOAD( "gra2-6.223",          0x2000000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x2400000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )
ROM_END

ROM_START( glprac2l )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "gra2b.119",    0x0000001, 0x100000, CRC(43abee7c) SHA1(ea0afc820d8480c12c9af54057877ff11a8012fb) )
	ROM_LOAD16_BYTE( "gra2a.120",    0x0000000, 0x100000, CRC(f60096d4) SHA1(5349d780d41a5711b483cd7eb66cd4e496b4fbe4) )
	ROM_LOAD( "gra2-0.217",          0x0800000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0c00000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x1000000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x1400000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1800000, 0x400000, CRC(2b070307) SHA1(43c028aaca297358f87c6633c2020d71e34317b8) )
	ROM_LOAD( "gra2-5.222",          0x1c00000, 0x400000, CRC(94a363c1) SHA1(4c53822a672ac99b001c9fe82f9d0f8496989e67) )
	ROM_LOAD( "gra2-6.223",          0x2000000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x2400000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )

	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD( "link3118.bin", 0x0000000, 0x040000, CRC(a4d4761e) SHA1(3fb25dfa5220d25093588d9501e0666214491100) )
ROM_END

ROM_START( cbaj )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "cbaj_1.119",   0x0000001, 0x080000, CRC(814f8b4b) SHA1(17966038a692d0701139660f25725d7c10a2a928) )
	ROM_LOAD16_BYTE( "cbaj_2.120",   0x0000000, 0x080000, CRC(89286229) SHA1(18a84ef648ec3b79707eb42b55563adf38dffd0d) )
	ROM_LOAD( "cb-00.216",           0x0400000, 0x400000, CRC(3db68bea) SHA1(77ab334e0c02e608b11d8fdb9505b2301f6f9afb) )
	ROM_LOAD( "cb-01.217",           0x0800000, 0x400000, CRC(481040bc) SHA1(c6fe575b77d1eb5f613691dec5ed08929b72b955) )
	ROM_LOAD( "cb-02.218",           0x0c00000, 0x400000, CRC(858f116c) SHA1(e3546862d367d2fe88913fea3185b23bc6a9777d) )
	ROM_LOAD( "cb-03.219",           0x1000000, 0x400000, CRC(3576ea2a) SHA1(a5ee7bb9f4650e99ee067eb1cc28c62d9099a6cf) )
	ROM_LOAD( "cb-04.220",           0x1400000, 0x400000, CRC(551c4b29) SHA1(c3f8508a006b475491c9ea20eb64c3bea6b35afb) )
	ROM_LOAD( "cb-05.221",           0x1800000, 0x400000, CRC(7da453da) SHA1(85b2c93b9453e8c7791b530b7e036e4ef6abc077) )
	ROM_LOAD( "cb-06.222",           0x1c00000, 0x400000, CRC(833cb18b) SHA1(dbc390e1dbf3e7815eb3d170c0890d3785d8002c) )
	ROM_LOAD( "cb-07.223",           0x2000000, 0x400000, CRC(3b64ce9e) SHA1(a137da126295736bb7643655d52bd570004e87fd) )
	ROM_LOAD( "cb-08.323",           0x2400000, 0x400000, CRC(57cc482e) SHA1(603c3d13a6cd796c209a97aa7e63b77bdbf71580) )

	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD( "cbaj_z80.3118",       0x0000000, 0x040000, CRC(92b02ad2) SHA1(f72317679ecbd8a0c3b081baaf9ff20a8c9ec00f) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* YMZ280B Sound Samples */
	ROM_LOAD( "cb-se.5121",   0x000000, 0x400000, CRC(f12b3db9) SHA1(d5231ad664603050bdca2081b114b07fc905ddc2) )
	ROM_LOAD( "cb-vo.5120",   0x400000, 0x400000, CRC(afb05d6d) SHA1(0c08010579813814fbf8a978cf4376bab18697a4) )
ROM_END

ROM_START( shngmtkb )
	TPS_BIOS

	ROM_REGION32_LE( 0x01800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "shmj-b.119",   0x0000001, 0x080000, CRC(65522c67) SHA1(b5981e5859aab742a87d6742feb9c55a3e6ba13f) )
	ROM_LOAD16_BYTE( "shmj-a.120",   0x0000000, 0x080000, CRC(a789defa) SHA1(f8f0d1c9e3492cda652a9561ef1d549b92f73efd) )
	ROM_LOAD( "sh-00.217",           0x0800000, 0x400000, CRC(081fed1c) SHA1(fb18add9521b8b104329871b4c1b8ae5e0254f8b) )
	ROM_LOAD( "sh-01.218",           0x0c00000, 0x400000, CRC(5a84ea96) SHA1(af4972cc10706999361d7505b975f5f1e1fc6761) )
	ROM_LOAD( "sh-02.219",           0x1000000, 0x400000, CRC(c8f80d76) SHA1(51e4eac6cec8e37e5b8c0e7d341feea574add7da) )
	ROM_LOAD( "sh-03.220",           0x1400000, 0x400000, CRC(daaa4c73) SHA1(eb31d4cadd9eba3d3431f3f6ef880bb2effa0b9f) )
ROM_END

ROM_START( doapp )
	TPS_BIOS

	ROM_REGION32_LE( 0x01c00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "doapp119.bin", 0x0000001, 0x100000, CRC(bbe04cef) SHA1(f2dae4810ca78075fc3007a6001531a455235a2e) )
	ROM_LOAD16_BYTE( "doapp120.bin", 0x0000000, 0x100000, CRC(b614d7e6) SHA1(373756d9b88b45c677e987ee1e5cb2d5446ecfe8) )
	ROM_LOAD( "doapp-0.216",         0x0400000, 0x400000, CRC(acc6c539) SHA1(a744567a3d75634098b1749103307981be9acbdd) )
	ROM_LOAD( "doapp-1.217",         0x0800000, 0x400000, CRC(14b961c4) SHA1(3fae1fcb4665ba8bad391881b26c2d087718d42f) )
	ROM_LOAD( "doapp-2.218",         0x0c00000, 0x400000, CRC(134f698f) SHA1(6422972cf5d30a0f09f0c20f042691d5969207b4) )
	ROM_LOAD( "doapp-3.219",         0x1000000, 0x400000, CRC(1c6540f3) SHA1(8631fde93a1da6325d7b31c7edf12c964f0ac4fc) )
	ROM_LOAD( "doapp-4.220",         0x1400000, 0x400000, CRC(f83bacf7) SHA1(5bd66da993f0db966581dde80dd7e5b377754412) )
	ROM_LOAD( "doapp-5.221",         0x1800000, 0x400000, CRC(e11e8b71) SHA1(b1d1b9532b5f074ce216a603436d5674d136865d) )
ROM_END

ROM_START( tondemo )
	TPS_BIOS

	ROM_REGION32_LE( 0x01c00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "u0119.bin",    0x0000001, 0x100000, CRC(5711e301) SHA1(005375d32c1eda9bd39e46326880a62506d06389) )
	ROM_LOAD16_BYTE( "u0120.bin",    0x0000000, 0x100000, CRC(0b8312c6) SHA1(93e0e4b796cc953daf7ed2ff2f327aed07cf833a) )
	ROM_LOAD( "tca-0.217",           0x0800000, 0x400000, CRC(ef175910) SHA1(b77aa9016804172d433d97d5fdc242a1361e941c) )
	ROM_LOAD( "tca-1.218",           0x0c00000, 0x400000, CRC(c3474e8a) SHA1(46dd0ae7cd2e54c639fe39d6965ef71ce6a1b921) )
	ROM_LOAD( "tca-2.219",           0x1000000, 0x400000, CRC(89b8e1a8) SHA1(70c5f0f2d0a7869e29b62b32fa485f941b683678) )
	ROM_LOAD( "tca-3.220",           0x1400000, 0x400000, CRC(4fcf8032) SHA1(3ea815548c3bda32b1d4e88454c29e5025431b1c) )
	ROM_LOAD( "tca-4.221",           0x1800000, 0x400000, CRC(c9e23f25) SHA1(145d4e7f0cb67d2552559ce90305a56802a253f9) )
ROM_END

/* video system */

#define KN_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1002v.353", 0x0000000, 0x080000, CRC(5ff165f3) SHA1(8f59314c1093446b9bcb06d232244da6df78e206) )

ROM_START( sncwgltd )
	KN_BIOS

	ROM_REGION32_LE( 0x01a80000, REGION_USER2, 0 )
	ROM_LOAD( "ic5.bin",      0x0000000, 0x080000, CRC(458f14aa) SHA1(b4e50be60ffb9b7911561dd35b6a7e0df3432a3a) )
	ROM_LOAD( "ic6.bin",      0x0080000, 0x080000, CRC(8233dd1e) SHA1(1422b4530d671e3b8b471ec16c20ef7c819ab762) )
	ROM_LOAD( "ic7.bin",      0x0100000, 0x080000, CRC(df5ba2f7) SHA1(19153084e7cff632380b67a2fff800644a2fbf7d) )
	ROM_LOAD( "ic8.bin",      0x0180000, 0x080000, CRC(e8145f2b) SHA1(3a1cb189426998856dfeda47267fde64be34c6ec) )
	ROM_LOAD( "ic9.bin",      0x0200000, 0x080000, CRC(605c9370) SHA1(9734549cae3028c089f4c9f2336ee374b3f950f8) )
	ROM_LOAD( "ic11.bin",     0x0280000, 0x400000, CRC(a93f6fee) SHA1(6f079643b50833f8fb497c49945ad23326cc9170) )
	ROM_LOAD( "ic12.bin",     0x0680000, 0x400000, CRC(9f584ef7) SHA1(12c04e198f17d1915f58e83aff45ca2e76773df8) )
	ROM_LOAD( "ic13.bin",     0x0a80000, 0x400000, CRC(652e9c78) SHA1(a929b2944de72606338acb822c1031463e2b1cc5) )
	ROM_LOAD( "ic14.bin",     0x0e80000, 0x400000, CRC(c4ef1424) SHA1(1734a6ee6d0be94d24afefcf2a125b74747f53d0) )
	ROM_LOAD( "ic15.bin",     0x1280000, 0x400000, CRC(2551d816) SHA1(e1500d4bfa8cc55220c366a5852263ac2070da82) )
	ROM_LOAD( "ic16.bin",     0x1680000, 0x400000, CRC(21b401bc) SHA1(89374b80453c474aa1dd3a219422f557f95a262c) )
ROM_END


/* Taito FX1a/FX1b */

#define TAITOFX1_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1000t.353", 0x0000000, 0x080000, CRC(e3f23b6e) SHA1(e18907cf8c6ba54d96edba0a9a00487a90219e0d) )

ROM_START( taitofx1 )
	TAITOFX1_BIOS
ROM_END

ROM_START( ftimpcta )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00e00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e25-13.4",     0x0000001, 0x100000, CRC(7f078d7b) SHA1(df9800dd6885dbc33736c5143d877b0847221061) )
	ROM_LOAD16_BYTE( "e25-14.3",     0x0000000, 0x100000, CRC(0c5f474f) SHA1(ce7031ba860297b99cddd6d0177f07e03520faeb) )
	ROM_LOAD( "e25-01.1",            0x0200000, 0x400000, CRC(8cc4be0c) SHA1(9ca15558a83b7e332e50accf1f7852444a7ce730) )
	ROM_LOAD( "e25-02.2",            0x0600000, 0x400000, CRC(8e8b4c82) SHA1(55c9d4d3a08fc3226a75ab3a674be433af83e289) )
	ROM_LOAD( "e25-03.12",           0x0a00000, 0x400000, CRC(43b1c085) SHA1(6e53550e9be0d2f415fc6b4f3b8a71185c5370b2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e25-10.14",    0x0000000, 0x080000, CRC(2b2ad1b1) SHA1(6d064d0b6805d43ce42929ac8f5645b56384f53c) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e25-04.27",    0x0000000, 0x400000, CRC(09a66d35) SHA1(f0df24bc9bfc9eb0f5150dc035c19fc5b8a39bf9) )
	ROM_LOAD( "e25-05.28",    0x0040000, 0x200000, CRC(3fb57636) SHA1(aa38bfac11ecf10fd55143cf4525a2a529be8bb6) )
ROM_END

ROM_START( gdarius )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00e00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e39-06.4",     0x0000001, 0x100000, CRC(2980c30d) SHA1(597321642125c3ae37581c2d9abc2723c7909996) )
	ROM_LOAD16_BYTE( "e39-05.3",     0x0000000, 0x100000, CRC(750e5b13) SHA1(68fe9cbd7d506cfd587dccc40b6ae0b0b6ee7c29) )
	ROM_LOAD( "e39-01.1",            0x0200000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0600000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0a00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )
ROM_END

ROM_START( gdarius2 )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00e00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e39-12.4",     0x0000001, 0x100000, CRC(b23266c3) SHA1(80aaddaaf10e40280ade4c7d11f45ddab47ee9a6) )
	ROM_LOAD16_BYTE( "e39-11.3",     0x0000000, 0x100000, CRC(766f73df) SHA1(9ce24c153920d259bc7fdef0778083eb6d639be3) )
	ROM_LOAD( "e39-01.1",            0x0200000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0600000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0a00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )
ROM_END

ROM_START( mgcldate )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00c00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e32-08.2",     0x0000001, 0x100000, CRC(3d42cd28) SHA1(9017922e835a359ba5126c8a9e8c27380a5ce081) )
	ROM_LOAD16_BYTE( "e32-09.7",     0x0000000, 0x100000, CRC(db7ec115) SHA1(fa6f18de71ba997389d887d7ffe745aa25e24c20) )
	ROM_LOAD( "e32-01.1",            0x0200000, 0x400000, CRC(cf5f1d01) SHA1(5417f8aef5c8d0e9e63ba8c68efb5b3ef37b4693) )
	ROM_LOAD( "e32-02.6",            0x0600000, 0x400000, CRC(61c8438c) SHA1(bdbe6079cc634c0cd6580f76619eb2944c9a31d9) )
	ROM_LOAD( "e32-03.12",           0x0a00000, 0x200000, CRC(190d1618) SHA1(838a651d32752015baa7e8caea62fd739631b8be) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e32-10.22",           0x0000000, 0x004000, CRC(adf3feb5) SHA1(bae5bc3fad99a92a3492be1b775dab861007eb3b) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e32-04.15",           0x0000000, 0x400000, CRC(c72f9eea) SHA1(7ab8b412a8ed00a42016acb7d13d3b074155780a) )
ROM_END

ROM_START( mgcldtea )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00c00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e32-05.2",     0x0000001, 0x080000, CRC(72fc7f7b) SHA1(50d9e84bc74fb63ec1900ab149051888bc3d03a5) )
	ROM_LOAD16_BYTE( "e32-06.7",     0x0000000, 0x080000, CRC(d11c3881) SHA1(f7046c5bed4818152edcf697a49664b0bcf12a1b) )
	ROM_LOAD( "e32-01.1",            0x0200000, 0x400000, CRC(cf5f1d01) SHA1(5417f8aef5c8d0e9e63ba8c68efb5b3ef37b4693) )
	ROM_LOAD( "e32-02.6",            0x0600000, 0x400000, CRC(61c8438c) SHA1(bdbe6079cc634c0cd6580f76619eb2944c9a31d9) )
	ROM_LOAD( "e32-03.12",           0x0a00000, 0x200000, CRC(190d1618) SHA1(838a651d32752015baa7e8caea62fd739631b8be) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e32-10.22",           0x0000000, 0x004000, CRC(adf3feb5) SHA1(bae5bc3fad99a92a3492be1b775dab861007eb3b) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e32-04.15",           0x0000000, 0x400000, CRC(c72f9eea) SHA1(7ab8b412a8ed00a42016acb7d13d3b074155780a) )
ROM_END

ROM_START( psyfrcex )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00900000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "pfexic2.bin",  0x0000001, 0x080000, CRC(997e4500) SHA1(4a90b452c9a877ccec55a11f36c4cbc6df1f1f41) )
	ROM_LOAD16_BYTE( "e22-10.7",     0x0000000, 0x080000, CRC(f6341d63) SHA1(99dc27aa694ae5951148054291912a486726e8c9) )
	ROM_LOAD( "e22-02.16",           0x0100000, 0x200000, CRC(03b50064) SHA1(0259537e86b266b3f34308c4fc0bcc04c037da71) )
	ROM_LOAD( "e22-03.1",            0x0300000, 0x200000, CRC(8372f839) SHA1(646b3919b6be63412c11850ec1524685abececc0) )
	ROM_LOAD( "e22-04.21",           0x0500000, 0x200000, CRC(397b71aa) SHA1(48743c362503c1d2dbeb3c8be4cb2aaaae015b88) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e22-07.22",           0x0000000, 0x004000, CRC(739af589) SHA1(dbb4d1c6d824a99ccf27168e2c21644e19811523) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e22-01.15",           0x000000,  0x200000, CRC(808b8340) SHA1(d8bde850dd9b5b71e94ea707d2d728754f907977) )
ROM_END

ROM_START( psyforce )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00900000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e22-05.2",     0x0000001, 0x080000, CRC(7770242c) SHA1(dd37575d3d9ffdef60fe0e4cab6c9e42d087f714) )
	ROM_LOAD16_BYTE( "e22-10.7",     0x0000000, 0x080000, CRC(f6341d63) SHA1(99dc27aa694ae5951148054291912a486726e8c9) )
	ROM_LOAD( "e22-02.16",           0x0100000, 0x200000, CRC(03b50064) SHA1(0259537e86b266b3f34308c4fc0bcc04c037da71) )
	ROM_LOAD( "e22-03.1",            0x0300000, 0x200000, CRC(8372f839) SHA1(646b3919b6be63412c11850ec1524685abececc0) )
	ROM_LOAD( "e22-04.21",           0x0500000, 0x200000, CRC(397b71aa) SHA1(48743c362503c1d2dbeb3c8be4cb2aaaae015b88) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e22-07.22",           0x0000000, 0x004000, CRC(739af589) SHA1(dbb4d1c6d824a99ccf27168e2c21644e19811523) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e22-01.15",           0x000000,  0x200000, CRC(808b8340) SHA1(d8bde850dd9b5b71e94ea707d2d728754f907977) )
ROM_END

ROM_START( raystorm )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e24-05.4",     0x0000001, 0x080000, CRC(40097ab9) SHA1(67e73568b35515c2c5a9119e97ac4709baff8c5a) )
	ROM_LOAD16_BYTE( "e24-06.3",     0x0000000, 0x080000, CRC(d70cdf46) SHA1(da6163d69d3ea9c1e3f4b7961a548f1f9d8d9909) )
	ROM_LOAD( "e24-02.1",            0x0400000, 0x400000, CRC(9f70950d) SHA1(b3e4f925a61ae2e5dd4cc5d7ec3030a0d5c2c04d) )
	ROM_LOAD( "e24-03.2",            0x0800000, 0x400000, CRC(6c1f0a5d) SHA1(1aac37a7ff23e54021a4cec18c9bb93242337180) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e24-09.14",    0x0000000, 0x080000, CRC(808589e1) SHA1(46ada4c6d68c2462186a0b962abb435ee740c0ba) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e24-04.27",    0x0000000, 0x400000, CRC(f403493a) SHA1(3e49fd2a060a3893e26f14cc3cf47c4ba91e17d4) )
ROM_END

ROM_START( sfchamp )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00900000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e18-07.2",     0x0000001, 0x080000, CRC(1b484e1c) SHA1(f29f40a9988475d8abbb126095b0716133c087a0) )
	ROM_LOAD16_BYTE( "e18-08.7",     0x0000000, 0x080000, CRC(6a5558cd) SHA1(75b26bcaaa213283e7e0dace69ee58f305b4572d) )
	ROM_LOAD( "e18-02.12",           0x0100000, 0x200000, CRC(c7b4fe29) SHA1(7f823bd61abf2b15d3ba62bca829a5b1acacfd09) )
	ROM_LOAD( "e18-03.16",           0x0300000, 0x200000, CRC(76392346) SHA1(2c5b70c4708208f866feea0472fcc72333061124) )
	ROM_LOAD( "e18-04.19",           0x0500000, 0x200000, CRC(fc3731da) SHA1(58948aad8d7bb7a8449d2bf12e9d5e6d7b4426b5) )
	ROM_LOAD( "e18-05.21",           0x0700000, 0x200000, CRC(2e984c50) SHA1(6d8255e38c67d68bf489c9885663ed2edf148188) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e18-09.22",           0x0000000, 0x004000, CRC(bb5a5319) SHA1(0bb700cafc157d3af663cc9bebb8167487ff2852) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e18-01.15",           0x0000000, 0x200000, CRC(dbd1408c) SHA1(ef81064f2f95e5ae25eb1f10d1e78f27f9e294f5) )
ROM_END

#define TAITOGNET_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-3002t.353", 0x0000000, 0x080000, CRC(03967fa7) SHA1(0e17fec2286e4e25deb23d40e41ce0986f373d49) )

ROM_START( taitogn )
	TAITOGNET_BIOS
	ROM_REGION32_LE( 0x0200000, REGION_USER2, 0 )
ROM_END

/* Eighteen Raizing */

#define PSARC95_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1002e.353", 0x000000, 0x080000, CRC(910f3a8b) SHA1(cd68532967a25f476a6d73473ec6b6f4df2e1689) )

ROM_START( psarc95 )
	PSARC95_BIOS
ROM_END

ROM_START( beastrzr )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x0c00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "broar.213",    0x000001, 0x080000, CRC(2c586534) SHA1(a38dfc3a45446d24a1caac89b0f560989d46ded5) )
	ROM_LOAD16_BYTE( "broar.212",    0x000000, 0x080000, CRC(1c85d7fb) SHA1(aa406a42c424cc16a9e5330c68dda9acf8760088) )
	ROM_LOAD16_BYTE( "broar.215",    0x100001, 0x080000, CRC(31c8e055) SHA1(2811789ab6221b972d1e3ffe98916587990f7564) )
//	ROM_LOAD16_BYTE( "broar_u0.214", 0x100000, 0x080000, CRC(911e6c90) SHA1(724e4cae49bb124200e188a0288516b3a7d5ab53) ) /* bad dump? */
	ROM_LOAD16_BYTE( "broar.214",    0x100000, 0x080000, CRC(1cdc450a) SHA1(9215e5fec52f7c5c0070feb621eb9c77f98e2362) )
	ROM_LOAD( "rabroar2.216",        0x400000, 0x400000, CRC(d46d46b7) SHA1(1c42cb5dcda4b26c08c4ecf95efeadaf3a1d1dd2) )
	ROM_LOAD( "rabroar1.217",        0x800000, 0x400000, CRC(11f1ba36) SHA1(d41ae686c2c607640cbadf906215c89134758050) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
//	ROM_LOAD16_BYTE( "broar.046",    0x000001, 0x040000, CRC(98e47d50) SHA1(2750c14ee650a0de41360105665ea9feaaacd770) ) /* bad dump? */
	ROM_LOAD16_BYTE( "broar.046",    0x000001, 0x040000, CRC(d4bb261a) SHA1(9a295b1354ef15f37ea09bb209cf0cb98437c462) )
//	ROM_LOAD16_BYTE( "broar.042",    0x000000, 0x040000, CRC(47dac191) SHA1(076253f97574255f01a001a4264260bc09a92d08) ) /* bad dump? */
	ROM_LOAD16_BYTE( "broar.042",    0x000000, 0x040000, CRC(4d537f88) SHA1(1760367d70a81606e29885ea315185d2c2a9409b) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "rabroar3.snd",        0x000000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )
ROM_END

ROM_START( beastrzb )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x0c00000, REGION_USER2, 0 )
	ROM_LOAD( "27c160.1",     0x000000, 0x200000, CRC(820855e2) SHA1(18bdd4d0b4a92ae4fde457e1f37c813be6eece71) )
	ROM_LOAD( "27c160.2",     0x400000, 0x200000, CRC(1712af34) SHA1(3a78997a2ad0fec1b09828b47150a4be611cd9ad) )
	ROM_LOAD( "27c800.3",     0x700000, 0x100000, CRC(7192eb4e) SHA1(bb276a38261099d91080d8613dc7500322f6fcab) )
//	ROM_LOAD( "4",            0x800000, 0x200000, CRC(bff21f44) SHA1(2dffc518c069f0692a3b75e10091658d9c10ecb5) ) /* bad dump? */
	ROM_LOAD( "27c160.4",     0x800000, 0x200000, CRC(2d2b25f4) SHA1(77d8ad94602e71f16b47de47bc2e0a97957c530b) )
	ROM_LOAD( "27c160.5",     0xa00000, 0x200000, CRC(10fe6f4d) SHA1(9faee2faa6d741e1caf25edd093644be5723aa5c) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
/*	http://www.atmel.com/dyn/products/product_card.asp?family_id=604&family_name=8051+Architecture&part_id=1939 */
	ROM_LOAD( "at89c4051",    0x000000, 0x001000, NO_DUMP ) /* cat 702 replacement or sound cpu? */

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "27c4096.1",    0x000000, 0x080000, CRC(217734a1) SHA1(de4f519215123c09b3b5f27509b4d74604b5e03d) )
	ROM_LOAD( "27c4096.2",    0x080000, 0x080000, CRC(d1f2a9b2) SHA1(d1475a453ce4e3b9f2ff59abedf0f57ba3c408fe) )
	ROM_LOAD( "27c240.3",     0x100000, 0x080000, CRC(509cdc8b) SHA1(8b92b79be09de56e7d40c2d02fcbeca92bb60226) ) /* bad dump? (only contains 8k of data) */
ROM_END

ROM_START( brvblade )
	TPS_BIOS

	ROM_REGION32_LE( 0x0c00000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(97e12c63) SHA1(382970617a363f6c98ee741f26be6a75c9752bdb) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(d9d40a34) SHA1(c91dbc6f85404e9397fa79a4bac28e8c3c1a5228) )
	ROM_LOAD( "ra-bbl_rom1.028", 0x0400000, 0x400000, CRC(418535e0) SHA1(7c443e651704f2cd552565c35f4a93f2dc250558) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "spu0u049.bin", 0x0000000, 0x080000, CRC(c9df8ed9) SHA1(00a58522189091c48d781b6703e4378e04343c33) )
	ROM_LOAD16_BYTE( "spu1u412.bin", 0x0000001, 0x080000, CRC(6408b5b2) SHA1(ba60aa1074df87e98fa260211e9ec99cea25023f) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ra-bbl_rom2.336", 0x000000, 0x400000, CRC(cd052c02) SHA1(d955a70a89b3b1a0b505a05c0887c399fe7a2c68) )
ROM_END

/* Atari PSX */

#define TW_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1000w.353", 0x000000, 0x080000, CRC(45e8a4b4) SHA1(815488d8563c85f97fbc3384ff21f08e4c88b7b7) )

ROM_START( atpsx )
	TW_BIOS
ROM_END

ROM_START( primrag2 )
	TW_BIOS

	ROM_REGION32_LE( 0x200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "pr2_036.u16",  0x000001, 0x080000, CRC(3ee39e4f) SHA1(dbd859b54fb9be33effc14eb847dcd829024eea3) )
	ROM_LOAD16_BYTE( "pr2_036.u14",  0x000000, 0x080000, CRC(c86450cd) SHA1(19c3c50d839a9efb6ffa9ada8a072f56697c1abb) )
	ROM_LOAD16_BYTE( "pr2_036.u17",  0x100001, 0x080000, CRC(3681516c) SHA1(714f73ea4ac190c36a6eb2308616a4aecabc4e69) )
	ROM_LOAD16_BYTE( "pr2_036.u15",  0x100000, 0x080000, CRC(4b24bd54) SHA1(7f27cd524d10e5869aab6d4dc6a4217d049c475d) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "primrag2", 0, MD5(618a56ce62a5f7a94a7781b994a201e4) SHA1(d79b2dad16fbdb174bcd34eb70025a53ebdb0e51) )
ROM_END

/* Acclaim PSX */

#define AC_BIOS \
	ROM_REGION( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1000a.353", 0x0000000, 0x080000, CRC(8d8d0764) SHA1(7ee83d317190bb1cef2f8f01c81eaaae47150ebb) )

ROM_START( acpsx )
	AC_BIOS
ROM_END

ROM_START( nbajamex )
	AC_BIOS

	ROM_REGION32_LE( 0x2000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "360mpa1o.u36", 0x0000001, 0x100000, CRC(c433e827) SHA1(1d2a5a6990a1b1864e63ce3ba7306d48ebbd4775) )
	ROM_LOAD16_BYTE( "360mpa1e.u35", 0x0000000, 0x100000, CRC(d8f5b2f7) SHA1(e38609d314721b8b612e047406e2888395917b0d) )
	ROM_LOAD16_BYTE( "nbax0o.u28",   0x0200001, 0x200000, CRC(be13c5af) SHA1(eee5c9d985384ecfe4f00fae27d66fbefc15b28e) )
	ROM_LOAD16_BYTE( "nbax0e.u41",   0x0200000, 0x200000, CRC(077f4355) SHA1(63c52bb82943b52bb0906d114acd5ea8643068b6) )
	ROM_LOAD16_BYTE( "nbax1o.u29",   0x0600001, 0x200000, CRC(3650e85b) SHA1(a36bfa235c8e3bb516e178f54d3c5e3955c7e918) )
	ROM_LOAD16_BYTE( "nbax1e.u42",   0x0600000, 0x200000, CRC(f1212cf9) SHA1(b2f80af3ec4d559056e86f695d89d1d32b500f50) )
	ROM_LOAD16_BYTE( "nbax2o.u30",   0x0a00001, 0x200000, CRC(ccbb6125) SHA1(998eda99182b984f88f5fc58095cb35bf232a26b) )
	ROM_LOAD16_BYTE( "nbax2e.u43",   0x0a00000, 0x200000, CRC(c20ab628) SHA1(7ffe5005e1913da56770452ae2f907a4a270ab24) )
	ROM_LOAD16_BYTE( "nbax3o.u31",   0x0e00001, 0x200000, CRC(d5238edf) SHA1(d1eb30ec65dd6cfa8cbb2b36af3a83820d1de99a) )
	ROM_LOAD16_BYTE( "nbax3e.u44",   0x0e00000, 0x200000, CRC(07ba00a3) SHA1(c14bffd35ee715b07d6065b454b0443438ab6536) )
	ROM_LOAD16_BYTE( "nbax4o.u3",    0x1200001, 0x200000, CRC(1cf16a34) SHA1(a7e984a2db846854f1c4a9a2fdefd0d17ce3108c) )
	ROM_LOAD16_BYTE( "nbax4e.u17",   0x1200000, 0x200000, CRC(b5977765) SHA1(08acdfe413a5a9182ca117f44b7acac9dac9ecea) )
	ROM_LOAD16_BYTE( "nbax5o.u4",    0x1600001, 0x200000, CRC(5272754b) SHA1(c35ba5377eb812991e4bf0d954a34af90b986341) )
	ROM_LOAD16_BYTE( "nbax5e.u18",   0x1600000, 0x200000, CRC(0eb917da) SHA1(d6c8991ba7cd492668757658ee64078d0e82b596) )
	ROM_LOAD16_BYTE( "nbax6o.u5",    0x1a00001, 0x200000, CRC(b1dfb42e) SHA1(fb9627e228bf2a744842eb44afbca4a6232cadb2) )
	ROM_LOAD16_BYTE( "nbax6e.u19",   0x1a00000, 0x200000, CRC(6f17d8c1) SHA1(22cf263efb64cf62030e02b641c485debe75944d) )

	ROM_REGION( 0x0a0000, REGION_CPU2, 0 ) /* 512k for the audio CPU (+banks) */
	ROM_LOAD( "360snda1.u52", 0x000000, 0x08000, CRC(36d8a628) SHA1(944a01c9128f5e90c7dba3557a3ecb2c5ca90831) )
	ROM_CONTINUE(             0x010000, 0x78000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "sound0.u48",   0x000000, 0x200000, CRC(38873b67) SHA1(b2f8d32270ae604c099a1b9b71d2e06468c7d4a9) )
	ROM_LOAD( "sound1.u49",   0x200000, 0x200000, CRC(57014589) SHA1(d360ff1c52424bd91a5a8d1a2a9c10bf7abb0602) )
ROM_END

ROM_START( jdredd )
	AC_BIOS

	ROM_REGION32_LE( 0x040000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "j-dread.u36",  0x000001, 0x020000, CRC(37addbf9) SHA1(a4061a1ba9e230f080f0bfea69bf77efe9264a92) )
	ROM_LOAD16_BYTE( "j-dread.u35",  0x000000, 0x020000, CRC(c1e17191) SHA1(82901439b1a51b9aadb4df4b9d944f26697a1460) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "jdreddc", 0, MD5(b66074f3df1e90ec4e3ff09dcdf33033) SHA1(83ed8df25d100b1c060f4dde2f162ba31803db7d) )
ROM_END

ROM_START( jdreddb )
	AC_BIOS

	ROM_REGION32_LE( 0x040000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "j-dread.u36",  0x000001, 0x020000, CRC(37addbf9) SHA1(a4061a1ba9e230f080f0bfea69bf77efe9264a92) )
	ROM_LOAD16_BYTE( "j-dread.u35",  0x000000, 0x020000, CRC(c1e17191) SHA1(82901439b1a51b9aadb4df4b9d944f26697a1460) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "jdreddb", 0, MD5(0da1d048d7223df74fca4f349473cefa) SHA1(9b810e3a16de62cabfc8271b6606574c7034cf41) )
ROM_END

/* Capcom ZN1 */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-1000c.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1995, cpzn1,    0,        coh1000c, zn, coh1000c, ROT0, "Sony/Capcom", "ZN1", NOT_A_DRIVER )

GAMEX( 1995, ts2,      cpzn1,    coh1000c, zn, coh1000c, ROT0, "Capcom/Takara", "Battle Arena Toshinden 2 (USA 951124)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1995, ts2j,     ts2,      coh1000c, zn, coh1000c, ROT0, "Capcom/Takara", "Battle Arena Toshinden 2 (JAPAN 951124)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, starglad, cpzn1,    coh1000c, zn, coh1000c, ROT0, "Capcom", "Star Gladiator (USA 960627)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1996, sfex,     cpzn1,    coh1002c, zn, coh1000c, ROT0, "Capcom/Arika", "Street Fighter EX (USA 961219)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1996, sfexa,    sfex,     coh1002c, zn, coh1000c, ROT0, "Capcom/Arika", "Street Fighter EX (ASIA 961219)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1996, sfexj,    sfex,     coh1002c, zn, coh1000c, ROT0, "Capcom/Arika", "Street Fighter EX (JAPAN 961130)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1996, glpracr,  cpzn1,    coh1000c, zn, coh1000c, ROT0, "Tecmo", "Gallop Racer (JAPAN Ver 9.01.12)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Capcom ZN2 */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-3002c.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1997, cpzn2,    0,        coh3002c, zn, coh3002c, ROT0, "Sony/Capcom", "ZN2", NOT_A_DRIVER )

GAMEX( 1997, sfexp,    cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom/Arika", "Street Fighter EX Plus (USA 970311)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1997, sfexpj,   sfexp,    coh3002c, zn, coh3002c, ROT0, "Capcom/Arika", "Street Fighter EX Plus (JAPAN 970311)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1997, rvschool, cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom", "Rival Schools (ASIA 971117)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1997, jgakuen,  rvschool, coh3002c, zn, coh3002c, ROT0, "Capcom", "Justice Gakuen (JAPAN 971117)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, sfex2,    cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom/Arika", "Street Fighter EX 2 (JAPAN 980312)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1998, plsmaswd, cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom", "Plasma Sword (USA 980316)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, stargld2, plsmaswd, coh3002c, zn, coh3002c, ROT0, "Capcom", "Star Gladiator 2 (JAPAN 980316)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, tgmj,     cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom/Akira", "Tetris The Grand Master (JAPAN 980710)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, techromn, cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom", "Tech Romancer (USA 980914)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, kikaioh,  techromn, coh3002c, zn, coh3002c, ROT0, "Capcom", "Kikaioh (JAPAN 980914)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, sfex2p,   cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom/Arika", "Street Fighter EX 2 Plus (USA 990611)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, sfex2pj,  sfex2p,   coh3002c, zn, coh3002c, ROT0, "Capcom/Arika", "Street Fighter EX 2 Plus (JAPAN 990611)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, strider2, cpzn2,    coh3002c, zn, coh3002c, ROT0, "Capcom", "Strider 2 (ASIA 991213)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, shiryu2,  strider2, coh3002c, zn, coh3002c, ROT0, "Capcom", "Strider Hiryu 2 (JAPAN 991213)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )

/* Atari */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-1000w.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1996, atpsx,    0,        coh1000w, zn, coh1000w, ROT0, "Atari", "Atari PSX", NOT_A_DRIVER )

GAMEX( 1996, primrag2, atpsx,    coh1000w, zn, coh1000w, ROT0, "Atari", "Primal Rage 2 (Ver 0.36a)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )

/* Acclaim */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-1000a.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1995, acpsx,    0,        coh1000a, zn, coh1000a, ROT0, "Acclaim", "Acclaim PSX", NOT_A_DRIVER )

GAMEX( 1996, nbajamex, acpsx,    coh1000a, zn, coh1000a, ROT0, "Acclaim", "NBA Jam Extreme", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, jdredd,   acpsx,    coh1000a, zn, coh1000a, ROT0, "Acclaim", "Judge Dredd (Rev C Dec. 17 1997)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1996, jdreddb,  jdredd,   coh1000a, zn, coh1000a, ROT0, "Acclaim", "Judge Dredd (Rev B Nov. 26 1997)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Tecmo */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-1002m.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1997, tps,      0,        coh1002m, zn, coh1002m, ROT0, "Sony/Tecmo", "TPS", NOT_A_DRIVER )

GAMEX( 1997, glpracr2, tps,      coh1002m, zn, coh1002m, ROT0, "Tecmo", "Gallop Racer 2 (USA)", GAME_NOT_WORKING )
GAMEX( 1997, glprac2j, glpracr2, coh1002m, zn, coh1002m, ROT0, "Tecmo", "Gallop Racer 2 (JAPAN)", GAME_NOT_WORKING )
GAMEX( 1997, glprac2l, glpracr2, coh1002ml,zn, coh1002m, ROT0, "Tecmo", "Gallop Racer 2 Link HW (JAPAN)", GAME_NOT_WORKING )
GAMEX( 1998, doapp,    tps,      coh1002m, zn, coh1002m, ROT0, "Tecmo", "Dead Or Alive ++ (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, cbaj,     tps,      coh1002msnd, zn, coh1002m, ROT0, "Tecmo", "Cool Boarders Arcade Jam", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1998, shngmtkb, tps,      coh1002m, zn, coh1002m, ROT0, "Sunsoft / Activision", "Shanghai Matekibuyuu", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1999, tondemo,  tps,      coh1002m, zn, coh1002m, ROT0, "Tecmo", "Tondemo Crisis (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* Video System */

/* only one game dumped on this system, so coh-1002v.353 is included in the game zip file */
GAMEX( 1996, sncwgltd, 0,        coh1002v, zn, coh1002v, ROT0, "Video System", "Sonic Wings Limited (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* Taito */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-1000t.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1995, taitofx1, 0,        coh1000ta,zn, coh1000t, ROT0, "Sony/Taito", "Taito FX1", NOT_A_DRIVER )

GAMEX( 1995, sfchamp,  taitofx1, coh1000ta,zn, coh1000t, ROT0, "Taito", "Super Football Champ (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1995, psyfrcex, taitofx1, coh1000ta,zn, coh1000t, ROT0, "Taito", "Psychic Force EX (?)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1995, psyforce, psyfrcex, coh1000ta,zn, coh1000t, ROT0, "Taito", "Psychic Force (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, mgcldate, taitofx1, coh1000ta,zn, coh1000t, ROT0, "Taito", "Magical Date (JAPAN) set 1", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, mgcldtea, mgcldate, coh1000ta,zn, coh1000t, ROT0, "Taito", "Magical Date (JAPAN) set 2", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, raystorm, taitofx1, coh1000tb,zn, coh1000t, ROT0, "Taito", "Ray Storm (JAPAN)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1996, ftimpcta, taitofx1, coh1000tb,zn, coh1000t, ROT0, "Taito", "Fighter's Impact Ace (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, gdarius,  taitofx1, coh1000tb,zn, coh1000t, ROT0, "Taito", "G-Darius (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, gdarius2, gdarius,  coh1000tb,zn, coh1000t, ROT0, "Taito", "G-Darius Ver.2 (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-3002t.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1997, taitogn,  0,        coh3002t, zn, coh3002t, ROT0, "Sony/Taito", "Taito GNET", NOT_A_DRIVER )

/* Eighting/Raizing */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-1002e.353 file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1997, psarc95,  0,        coh1002e, zn, coh1002e, ROT0, "Sony/Eighting/Raizing", "PS Arcade 95", NOT_A_DRIVER )

GAMEX( 1997, beastrzr, psarc95,  coh1002e, zn, coh1002e, ROT0, "Eighting/Raizing", "Beastorizer", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1997, beastrzb, psarc95,  coh1002e, zn, coh1002e, ROT0, "Eighting/Raizing", "Beastorizer (bootleg)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 2000, brvblade, tps,      coh1002e, zn, coh1002e, ROT270, "Eighting/Raizing", "Brave Blade (JAPAN)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
