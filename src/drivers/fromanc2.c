/******************************************************************************

	Game Driver for Video System Mahjong series. (Preliminary driver)

	Taisen Idol-Mahjong Final Romance 2
	(c)1995 Video System Co.,Ltd.

	Taisen Mahjong FinalRomance R
	(c)1995 Video System Co.,Ltd.

	Taisen Mahjong FinalRomance 4
	(c)1998 Video System Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2001/02/28 -
	Special very thanks to Uki.

******************************************************************************/
/******************************************************************************
Memo:

******************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"


VIDEO_UPDATE( fromanc2 );
VIDEO_START( fromanc2 );
VIDEO_START( fromancr );
VIDEO_START( fromanc4 );

READ16_HANDLER( fromanc2_paletteram_0_r );
READ16_HANDLER( fromanc2_paletteram_1_r );
WRITE16_HANDLER( fromanc2_paletteram_0_w );
WRITE16_HANDLER( fromanc2_paletteram_1_w );
READ16_HANDLER( fromancr_paletteram_0_r );
READ16_HANDLER( fromancr_paletteram_1_r );
WRITE16_HANDLER( fromancr_paletteram_0_w );
WRITE16_HANDLER( fromancr_paletteram_1_w );
READ16_HANDLER( fromanc4_paletteram_0_r );
READ16_HANDLER( fromanc4_paletteram_1_r );
WRITE16_HANDLER( fromanc4_paletteram_0_w );
WRITE16_HANDLER( fromanc4_paletteram_1_w );
WRITE16_HANDLER( fromanc2_videoram_0_w );
WRITE16_HANDLER( fromanc2_videoram_1_w );
WRITE16_HANDLER( fromanc2_videoram_2_w );
WRITE16_HANDLER( fromanc2_videoram_3_w );
WRITE16_HANDLER( fromancr_videoram_0_w );
WRITE16_HANDLER( fromancr_videoram_1_w );
WRITE16_HANDLER( fromancr_videoram_2_w );
WRITE16_HANDLER( fromanc4_videoram_0_w );
WRITE16_HANDLER( fromanc4_videoram_1_w );
WRITE16_HANDLER( fromanc4_videoram_2_w );
WRITE16_HANDLER( fromanc2_gfxreg_0_w );
WRITE16_HANDLER( fromanc2_gfxreg_1_w );
WRITE16_HANDLER( fromanc2_gfxreg_2_w );
WRITE16_HANDLER( fromanc2_gfxreg_3_w );
WRITE16_HANDLER( fromancr_gfxreg_0_w );
WRITE16_HANDLER( fromancr_gfxreg_1_w );
WRITE16_HANDLER( fromanc2_gfxbank_0_w );
WRITE16_HANDLER( fromanc2_gfxbank_1_w );
void fromancr_gfxbank_w(int data);
WRITE16_HANDLER( fromanc4_gfxreg_0_w );
WRITE16_HANDLER( fromanc4_gfxreg_1_w );
WRITE16_HANDLER( fromanc4_gfxreg_2_w );

void fromanc2_set_dispvram_w(int vram);


static int fromanc2_playerside;
static int fromanc2_portselect;
static UINT16 fromanc2_datalatch1;
static UINT8 fromanc2_datalatch_2h, fromanc2_datalatch_2l;
static UINT8 fromanc2_subcpu_int_flag;
static UINT8 fromanc2_subcpu_nmi_flag;
static UINT8 fromanc2_sndcpu_nmi_flag;


// ----------------------------------------------------------------------------
// 	MACHINE INITIALYZE
// ----------------------------------------------------------------------------

static MACHINE_INIT( fromanc2 )
{
	//
}

static MACHINE_INIT( fromancr )
{
	//
}

static MACHINE_INIT( fromanc4 )
{
	//
}


static DRIVER_INIT( fromanc2 )
{
	fromanc2_subcpu_nmi_flag = 1;
	fromanc2_subcpu_int_flag = 1;
	fromanc2_sndcpu_nmi_flag = 1;

	fromanc2_playerside = 0;
}

static DRIVER_INIT( fromancr )
{
	fromanc2_subcpu_nmi_flag = 1;
	fromanc2_subcpu_int_flag = 1;
	fromanc2_sndcpu_nmi_flag = 1;

	fromanc2_playerside = 0;
}

static DRIVER_INIT( fromanc4 )
{
	fromanc2_sndcpu_nmi_flag = 1;

	fromanc2_playerside = 0;
}


// ----------------------------------------------------------------------------
// 	MAIN CPU Interrupt (fromanc2, fromancr, fromanc4)	TEST ROUTINE
// ----------------------------------------------------------------------------

static INTERRUPT_GEN( fromanc2_interrupt )
{
	static int fromanc2_playerside_old = -1;
	static int key_F1_old = 0;

	if (keyboard_pressed(KEYCODE_F1))
	{
		if (key_F1_old != 1)
		{
			key_F1_old = 1;
			fromanc2_playerside ^= 1;
		}
	} else key_F1_old = 0;

	if (fromanc2_playerside_old != fromanc2_playerside)
	{
		fromanc2_playerside_old = fromanc2_playerside;

		usrintf_showmessage("PLAYER-%01X SIDE", (fromanc2_playerside + 1));

		if (!fromanc2_playerside)
		{
			mixer_set_stereo_volume(3, 75, 75);	// 1P (LEFT)
			mixer_set_stereo_volume(4,  0,  0);	// 2P (RIGHT)
		}
		else
		{
			mixer_set_stereo_volume(3,  0,  0);	// 1P (LEFT)
			mixer_set_stereo_volume(4, 75, 75);	// 2P (RIGHT)
		}

		fromanc2_set_dispvram_w(fromanc2_playerside);
	}

	cpu_set_irq_line(0, 1, HOLD_LINE);
}


// ----------------------------------------------------------------------------
// 	Sound Command Interface (fromanc2, fromancr, fromanc4)
// ----------------------------------------------------------------------------

static WRITE16_HANDLER( fromanc2_sndcmd_w )
{
	soundlatch_w(offset, ((data >> 8) & 0xff));	// 1P (LEFT)
	soundlatch2_w(offset, (data & 0xff));		// 2P (RIGHT)

	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	fromanc2_sndcpu_nmi_flag = 0;
}

// ----------------------------------------------------------------------------
// 	Input Port Interface (COIN, TEST, KEY MATRIX, EEPROM)
// ----------------------------------------------------------------------------

static WRITE16_HANDLER( fromanc2_portselect_w )
{
	fromanc2_portselect = data;
}

static READ16_HANDLER( fromanc2_keymatrix_r )
{
	UINT16 ret;

	switch (fromanc2_portselect)
	{
		case	0x01:	ret = readinputport(1);	break;
		case	0x02:	ret = readinputport(2); break;
		case	0x04:	ret = readinputport(3); break;
		case	0x08:	ret = readinputport(4); break;
		default:	ret = 0xffff;
				logerror("PC:%08X unknown %02X\n", activecpu_get_pc(), fromanc2_portselect);
				break;
	}

	if (fromanc2_playerside) ret = (((ret & 0xff00) >> 8) | ((ret & 0x00ff) << 8));

	return ret;
}

static READ16_HANDLER( fromanc2_input_r )
{
	UINT16 cflag, coinsw, eeprom;

	cflag = (((fromanc2_subcpu_int_flag & 1) << 4) |
		 ((fromanc2_subcpu_nmi_flag & 1) << 6) |
		 ((fromanc2_sndcpu_nmi_flag & 1) << 5));
	eeprom = ((EEPROM_read_bit() & 1) << 7);	// EEPROM DATA
	coinsw = (readinputport(0) & 0x030f);		// COIN, TEST

	return (cflag | eeprom | coinsw);
}

static READ16_HANDLER( fromanc4_input_r )
{
	UINT16 cflag, coinsw, eeprom;

	cflag = ((fromanc2_sndcpu_nmi_flag & 1) << 5);
	eeprom = ((EEPROM_read_bit() & 1) << 7);	// EEPROM DATA
	coinsw = (readinputport(0) & 0x001f);		// COIN, TEST

	return (cflag | eeprom | coinsw);
}

static WRITE16_HANDLER( fromanc2_eeprom_w )
{
	if (ACCESSING_MSB)
	{
		// latch the bit
		EEPROM_write_bit(data & 0x0100);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0400) ? CLEAR_LINE : ASSERT_LINE);

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0200) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static WRITE16_HANDLER( fromancr_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		fromancr_gfxbank_w(data & 0xfff8);

		// latch the bit
		EEPROM_write_bit(data & 0x0001);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0004) ? CLEAR_LINE : ASSERT_LINE);

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0002) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static WRITE16_HANDLER( fromanc4_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		// latch the bit
		EEPROM_write_bit(data & 0x0004);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0001) ? CLEAR_LINE : ASSERT_LINE);

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0002) ? ASSERT_LINE : CLEAR_LINE);
	}
}

// ----------------------------------------------------------------------------
// 	MAIN CPU, SUB CPU Communication Interface (fromanc2, fromancr)
// ----------------------------------------------------------------------------

static WRITE16_HANDLER( fromanc2_subcpu_w )
{
	fromanc2_datalatch1 = data;

	cpu_set_irq_line(2, 0, HOLD_LINE);
	fromanc2_subcpu_int_flag = 0;
}

static READ16_HANDLER( fromanc2_subcpu_r )
{
	cpu_set_irq_line(2, IRQ_LINE_NMI, PULSE_LINE);
	fromanc2_subcpu_nmi_flag = 0;

	return ((fromanc2_datalatch_2h << 8) | fromanc2_datalatch_2l);
}

static READ_HANDLER( fromanc2_maincpu_r_l )
{
	return (fromanc2_datalatch1 & 0x00ff);
}

static READ_HANDLER( fromanc2_maincpu_r_h )
{
	fromanc2_subcpu_int_flag = 1;

	return ((fromanc2_datalatch1 & 0xff00) >> 8);
}

static WRITE_HANDLER( fromanc2_maincpu_w_l )
{
	fromanc2_datalatch_2l = data;
}

static WRITE_HANDLER( fromanc2_maincpu_w_h )
{
	fromanc2_datalatch_2h = data;
}

static WRITE_HANDLER( fromanc2_subcpu_nmi_clr )
{
	fromanc2_subcpu_nmi_flag = 1;
}

static READ_HANDLER( fromanc2_sndcpu_nmi_clr )
{
	fromanc2_sndcpu_nmi_flag = 1;

	return 0xff;
}

static WRITE_HANDLER( fromanc2_subcpu_rombank_w )
{
	UINT8 *RAM = memory_region(REGION_CPU3);
	int rombank = data & 0x03;
	int rambank = (data & 0x0c) >> 2;

	// Change ROM BANK
	cpu_setbank(1, &RAM[rombank * 0x4000]);

	// Change RAM BANK
	if (rambank != 0) cpu_setbank(2, &RAM[0x10000 + (rambank * 0x4000)]);
	else cpu_setbank(2, &RAM[0x8000]);
}


// ----------------------------------------------------------------------------
//	MAIN Program (fromanc2, fromancr, fromanc4)
// ----------------------------------------------------------------------------

static MEMORY_READ16_START( fromanc2_readmem_main )
	{ 0x000000, 0x07ffff, MRA16_ROM },		// MAIN ROM

	{ 0x802000, 0x802fff, MRA16_NOP },		// ???

	{ 0xa00000, 0xa00fff, fromanc2_paletteram_0_r },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromanc2_paletteram_1_r },// PALETTE (2P)

	{ 0xd01100, 0xd01101, fromanc2_input_r },	// INPUT COMMON, EEPROM
	{ 0xd01300, 0xd01301, fromanc2_subcpu_r },	// SUB CPU READ
	{ 0xd01800, 0xd01801, fromanc2_keymatrix_r },	// INPUT KEY MATRIX

	{ 0xd80000, 0xd8ffff, MRA16_RAM },		// WORK RAM
MEMORY_END

static MEMORY_WRITE16_START( fromanc2_writemem_main )
	{ 0x000000, 0x07ffff, MWA16_ROM },		// MAIN ROM

	{ 0x800000, 0x803fff, fromanc2_videoram_0_w },	// VRAM 0, 1 (1P)
	{ 0x880000, 0x883fff, fromanc2_videoram_1_w },	// VRAM 2, 3 (1P)
	{ 0x900000, 0x903fff, fromanc2_videoram_2_w },	// VRAM 0, 1 (2P)
	{ 0x980000, 0x983fff, fromanc2_videoram_3_w },	// VRAM 2, 3 (2P)

	{ 0xa00000, 0xa00fff, fromanc2_paletteram_0_w },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromanc2_paletteram_1_w },// PALETTE (2P)

	{ 0xd00000, 0xd00023, fromanc2_gfxreg_0_w },	// SCROLL REG (1P/2P)
	{ 0xd00100, 0xd00123, fromanc2_gfxreg_2_w },	// SCROLL REG (1P/2P)
	{ 0xd00200, 0xd00223, fromanc2_gfxreg_1_w },	// SCROLL REG (1P/2P)
	{ 0xd00300, 0xd00323, fromanc2_gfxreg_3_w },	// SCROLL REG (1P/2P)

	{ 0xd00400, 0xd00413, MWA16_NOP },		// ???
	{ 0xd00500, 0xd00513, MWA16_NOP },		// ???

	{ 0xd01000, 0xd01001, fromanc2_sndcmd_w },	// SOUND REQ (1P/2P)
	{ 0xd01200, 0xd01201, fromanc2_subcpu_w },	// SUB CPU WRITE
	{ 0xd01400, 0xd01401, fromanc2_gfxbank_0_w },	// GFXBANK (1P)
	{ 0xd01500, 0xd01501, fromanc2_gfxbank_1_w },	// GFXBANK (2P)
	{ 0xd01600, 0xd01601, fromanc2_eeprom_w },	// EEPROM DATA
	{ 0xd01a00, 0xd01a01, fromanc2_portselect_w },	// PORT SELECT (1P/2P)

	{ 0xd80000, 0xd8ffff, MWA16_RAM },		// WORK RAM
MEMORY_END

static MEMORY_READ16_START( fromancr_readmem_main )
	{ 0x000000, 0x07ffff, MRA16_ROM },		// MAIN ROM

	{ 0xa00000, 0xa00fff, fromancr_paletteram_0_r },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromancr_paletteram_1_r },// PALETTE (2P)

	{ 0xd01100, 0xd01101, fromanc2_input_r },	// INPUT COMMON, EEPROM
	{ 0xd01300, 0xd01301, fromanc2_subcpu_r },	// SUB CPU READ
	{ 0xd01800, 0xd01801, fromanc2_keymatrix_r },	// INPUT KEY MATRIX

	{ 0xd80000, 0xd8ffff, MRA16_RAM },		// WORK RAM
MEMORY_END

static MEMORY_WRITE16_START( fromancr_writemem_main )
	{ 0x000000, 0x07ffff, MWA16_ROM },		// MAIN ROM

	{ 0x800000, 0x803fff, fromancr_videoram_0_w },	// VRAM BG (1P/2P)
	{ 0x880000, 0x883fff, fromancr_videoram_1_w },	// VRAM FG (1P/2P)
	{ 0x900000, 0x903fff, fromancr_videoram_2_w },	// VRAM TEXT (1P/2P)
	{ 0x980000, 0x983fff, MWA16_NOP },		// VRAM Unused ?

	{ 0xa00000, 0xa00fff, fromancr_paletteram_0_w },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromancr_paletteram_1_w },// PALETTE (2P)

	{ 0xd00000, 0xd00023, fromancr_gfxreg_1_w },	// SCROLL REG (1P/2P)
	{ 0xd00100, 0xd00123, fromancr_gfxreg_0_w },	// SCROLL REG (1P/2P)

	{ 0xd00200, 0xd002ff, MWA16_NOP },		// ?

	{ 0xd00400, 0xd00413, MWA16_NOP },		// ???
	{ 0xd00500, 0xd00513, MWA16_NOP },		// ???

	{ 0xd01000, 0xd01001, fromanc2_sndcmd_w },	// SOUND REQ (1P/2P)
	{ 0xd01200, 0xd01201, fromanc2_subcpu_w },	// SUB CPU WRITE
	{ 0xd01400, 0xd01401, MWA16_NOP },		// COIN COUNTER ?
	{ 0xd01600, 0xd01601, fromancr_eeprom_w },	// EEPROM DATA, GFXBANK (1P/2P)
	{ 0xd01a00, 0xd01a01, fromanc2_portselect_w },	// PORT SELECT (1P/2P)

	{ 0xd80000, 0xd8ffff, MWA16_RAM },		// WORK RAM
MEMORY_END

static MEMORY_READ16_START( fromanc4_readmem_main )
	{ 0x000000, 0x07ffff, MRA16_ROM },		// MAIN ROM
	{ 0x400000, 0x7fffff, MRA16_ROM },		// DATA ROM

	{ 0x800000, 0x81ffff, MRA16_RAM },		// WORK RAM

	{ 0xdb0000, 0xdb0fff, fromanc4_paletteram_0_r },// PALETTE (1P)
	{ 0xdc0000, 0xdc0fff, fromanc4_paletteram_1_r },// PALETTE (2P)

	{ 0xd10000, 0xd10001, fromanc2_keymatrix_r },	// INPUT KEY MATRIX
	{ 0xd20000, 0xd20001, fromanc4_input_r },	// INPUT COMMON, EEPROM DATA

	{ 0xe5000c, 0xe5000d, MRA16_NOP },		// EXT-COMM PORT ?
MEMORY_END

static MEMORY_WRITE16_START( fromanc4_writemem_main )
	{ 0x000000, 0x07ffff, MWA16_ROM },		// MAIN ROM
	{ 0x400000, 0x7fffff, MWA16_ROM },		// DATA ROM

	{ 0x800000, 0x81ffff, MWA16_RAM },		// WORK RAM

	{ 0xd00000, 0xd00001, fromanc2_portselect_w },	// PORT SELECT (1P/2P)

	{ 0xd10000, 0xd10001, MWA16_NOP },		// ?
	{ 0xd30000, 0xd30001, MWA16_NOP },		// ?
	{ 0xd50000, 0xd50001, fromanc4_eeprom_w },	// EEPROM DATA

	{ 0xd70000, 0xd70001, fromanc2_sndcmd_w },	// SOUND REQ (1P/2P)

	{ 0xd80000, 0xd8ffff, fromanc4_videoram_0_w },	// VRAM FG (1P/2P)
	{ 0xd90000, 0xd9ffff, fromanc4_videoram_1_w },	// VRAM BG (1P/2P)
	{ 0xda0000, 0xdaffff, fromanc4_videoram_2_w },	// VRAM TEXT (1P/2P)

	{ 0xdb0000, 0xdb0fff, fromanc4_paletteram_0_w },// PALETTE (1P)
	{ 0xdc0000, 0xdc0fff, fromanc4_paletteram_1_w },// PALETTE (2P)

	{ 0xe00000, 0xe0001d, fromanc4_gfxreg_0_w },	// SCROLL, GFXBANK (1P/2P)
	{ 0xe10000, 0xe1001d, fromanc4_gfxreg_1_w },	// SCROLL, GFXBANK (1P/2P)
	{ 0xe20000, 0xe2001d, fromanc4_gfxreg_2_w },	// SCROLL, GFXBANK (1P/2P)

	{ 0xe30000, 0xe30013, MWA16_NOP },		// ???
	{ 0xe40000, 0xe40013, MWA16_NOP },		// ???

	{ 0xe50000, 0xe50009, MWA16_NOP },		// EXT-COMM PORT ?
MEMORY_END


// ----------------------------------------------------------------------------
// 	Z80 SUB Program (fromanc2, fromancr)
// ----------------------------------------------------------------------------

static MEMORY_READ_START( fromanc2_readmem_sub )
	{ 0x0000, 0x3fff, MRA_ROM },			// ROM
	{ 0x4000, 0x7fff, MRA_BANK1 },			// ROM(BANK)
	{ 0x8000, 0xbfff, MRA_RAM },			// RAM(WORK)
	{ 0xc000, 0xffff, MRA_BANK2 },			// RAM(BANK)
MEMORY_END

static MEMORY_WRITE_START( fromanc2_writemem_sub )
	{ 0x0000, 0x3fff, MWA_ROM },			// ROM
	{ 0x4000, 0x7fff, MWA_BANK1 },			// ROM(BANK)
	{ 0x8000, 0xbfff, MWA_RAM },			// RAM(WORK)
	{ 0xc000, 0xffff, MWA_BANK2 },			// RAM(BANK)
MEMORY_END

static PORT_READ_START( fromanc2_readport_sub )
	{ 0x02, 0x02, fromanc2_maincpu_r_l },		// to MAIN CPU
	{ 0x04, 0x04, fromanc2_maincpu_r_h },		// to MAIN CPU
PORT_END

static PORT_WRITE_START( fromanc2_writeport_sub )
	{ 0x00, 0x00, fromanc2_subcpu_rombank_w },
	{ 0x02, 0x02, fromanc2_maincpu_w_l },		// from MAIN CPU
	{ 0x04, 0x04, fromanc2_maincpu_w_h },		// from MAIN CPU
	{ 0x06, 0x06, fromanc2_subcpu_nmi_clr },
PORT_END


// ----------------------------------------------------------------------------
// 	Z80 Sound Program (fromanc2, fromancr, fromanc4)
// ----------------------------------------------------------------------------

static MEMORY_READ_START( fromanc2_readmem_sound )
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( fromanc2_writemem_sound )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( fromanc2_readport_sound )
	{ 0x00, 0x00, soundlatch_r },				// snd cmd (1P)
	{ 0x04, 0x04, soundlatch2_r },				// snd cmd (2P)
	{ 0x09, 0x09, IORP_NOP },				// ?
	{ 0x08, 0x08, YM2610_status_port_0_A_r },
	{ 0x0a, 0x0a, YM2610_status_port_0_B_r },
	{ 0x0c, 0x0c, fromanc2_sndcpu_nmi_clr },
PORT_END

static PORT_WRITE_START( fromanc2_writeport_sound )
	{ 0x00, 0x00, IOWP_NOP },				// ?
	{ 0x08, 0x08, YM2610_control_port_0_A_w },
	{ 0x09, 0x09, YM2610_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2610_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2610_data_port_0_B_w },
PORT_END


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

#define VSYSMJCTRL_PORT1 \
	PORT_START	/* (1) PORT 0 */ \
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE ) \
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE ) \
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, 0, "P2 A", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, 0, "P2 E", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, 0, "P2 I", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, 0, "P2 M", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, 0, "P2 Kan", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define VSYSMJCTRL_PORT2 \
	PORT_START	/* (2) PORT 1 */ \
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE ) \
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE ) \
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, 0, "P2 B", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, 0, "P2 F", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, 0, "P2 J", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, 0, "P2 N", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, 0, "P2 Reach", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define VSYSMJCTRL_PORT3 \
	PORT_START	/* (3) PORT 2 */ \
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE ) \
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE ) \
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, 0, "P2 C", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, 0, "P2 G", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, 0, "P2 K", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, 0, "P2 Chi", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x1000, IP_ACTIVE_LOW, 0, "P2 Ron", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define VSYSMJCTRL_PORT4 \
	PORT_START	/* (4) PORT 3 */ \
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE ) \
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE ) \
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE ) \
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE ) \
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, 0, "P2 D", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, 0, "P2 H", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0400, IP_ACTIVE_LOW, 0, "P2 L", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BITX( 0x0800, IP_ACTIVE_LOW, 0, "P2 Pon", IP_KEY_DEFAULT, IP_JOY_NONE ) \
	PORT_BIT ( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( fromanc2 )
	PORT_START	/* (0) COIN SW, TEST SW, EEPROM DATA, etc */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1 (1P)
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2 (1P)
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )		// COIN1 (2P)
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )		// COIN2 (2P)
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )		// SUBCPU INT FLAG
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )		// SNDCPU NMI FLAG
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )		// SUBCPU NMI FLAG
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )		// EEPROM READ
	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST (1P)
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST (2P)
	PORT_BIT ( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	VSYSMJCTRL_PORT1	/* (1) PORT 1-0 */
	VSYSMJCTRL_PORT2	/* (2) PORT 1-1 */
	VSYSMJCTRL_PORT3	/* (3) PORT 1-2 */
	VSYSMJCTRL_PORT4	/* (4) PORT 1-3 */
INPUT_PORTS_END

INPUT_PORTS_START( fromanc4 )
	PORT_START	/* (0) COIN SW, TEST SW, EEPROM DATA, etc */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST (1P)
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1 (1P)
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2 (1P)
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_COIN3 )		// COIN3 (2P)
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_COIN4 )		// COIN4 (2P)
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )		// SNDCPU NMI FLAG
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )		// EEPROM READ
	PORT_BIT ( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	VSYSMJCTRL_PORT1	/* (1) PORT 1-0 */
	VSYSMJCTRL_PORT2	/* (2) PORT 1-1 */
	VSYSMJCTRL_PORT3	/* (3) PORT 1-2 */
	VSYSMJCTRL_PORT4	/* (4) PORT 1-3 */
INPUT_PORTS_END


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

static struct GfxLayout fromanc2_tilelayout =
{
	8, 8,
	RGN_FRAC(1, 1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxDecodeInfo fromanc2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fromanc2_tilelayout, (  0 * 2), (256 * 2) },
	{ REGION_GFX2, 0, &fromanc2_tilelayout, (256 * 2), (256 * 2) },
	{ REGION_GFX3, 0, &fromanc2_tilelayout, (512 * 2), (256 * 2) },
	{ REGION_GFX4, 0, &fromanc2_tilelayout, (768 * 2), (256 * 2) },
	{ -1 } /* end of array */
};

static struct GfxLayout fromancr_tilelayout =
{
	8, 8,
	RGN_FRAC(1, 1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 1*8, 0*8, 3*8, 2*8, 5*8, 4*8, 7*8, 6*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static struct GfxDecodeInfo fromancr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fromancr_tilelayout, (512 * 2), 2 },
	{ REGION_GFX2, 0, &fromancr_tilelayout, (256 * 2), 2 },
	{ REGION_GFX3, 0, &fromancr_tilelayout, (  0 * 2), 2 },
	{ -1 } /* end of array */
};


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

static void irqhandler(int irq)
{
	cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,
	8000000,
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ 0 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(75, MIXER_PAN_CENTER, 75, MIXER_PAN_CENTER) }
};


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

static MACHINE_DRIVER_START( fromanc2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)		/* 16.00 MHz */
	MDRV_CPU_MEMORY(fromanc2_readmem_main,fromanc2_writemem_main)
	MDRV_CPU_VBLANK_INT(fromanc2_interrupt,1)

	MDRV_CPU_ADD(Z80,32000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 8.00 Mhz */
	MDRV_CPU_MEMORY(fromanc2_readmem_sound,fromanc2_writemem_sound)
	MDRV_CPU_PORTS(fromanc2_readport_sound,fromanc2_writeport_sound)

	MDRV_CPU_ADD(Z80,32000000/4)		/* 8.00 Mhz */
	MDRV_CPU_MEMORY(fromanc2_readmem_sub,fromanc2_writemem_sub)
	MDRV_CPU_PORTS(fromanc2_readport_sub,fromanc2_writeport_sub)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(fromanc2)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 352-1, 0, 240-1)
	MDRV_GFXDECODE(fromanc2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(fromanc2)
	MDRV_VIDEO_UPDATE(fromanc2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fromancr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)		/* 16.00 MHz */
	MDRV_CPU_MEMORY(fromancr_readmem_main,fromancr_writemem_main)
	MDRV_CPU_VBLANK_INT(fromanc2_interrupt,1)

	MDRV_CPU_ADD(Z80,32000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 8.00 Mhz */
	MDRV_CPU_MEMORY(fromanc2_readmem_sound,fromanc2_writemem_sound)
	MDRV_CPU_PORTS(fromanc2_readport_sound,fromanc2_writeport_sound)

	MDRV_CPU_ADD(Z80,32000000/4)		/* 8.00 Mhz */
	MDRV_CPU_MEMORY(fromanc2_readmem_sub,fromanc2_writemem_sub)
	MDRV_CPU_PORTS(fromanc2_readport_sub,fromanc2_writeport_sub)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(fromancr)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 352-1, 0, 240-1)
	MDRV_GFXDECODE(fromancr_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(fromancr)
	MDRV_VIDEO_UPDATE(fromanc2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fromanc4 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)		/* 16.00 MHz */
	MDRV_CPU_MEMORY(fromanc4_readmem_main,fromanc4_writemem_main)
	MDRV_CPU_VBLANK_INT(fromanc2_interrupt,1)

	MDRV_CPU_ADD(Z80,32000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 8.00 Mhz */
	MDRV_CPU_MEMORY(fromanc2_readmem_sound,fromanc2_writemem_sound)
	MDRV_CPU_PORTS(fromanc2_readport_sound,fromanc2_writeport_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(fromanc4)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(2048, 256)
	MDRV_VISIBLE_AREA(0, 352-1, 0, 240-1)
	MDRV_GFXDECODE(fromancr_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(fromanc4)
	MDRV_VIDEO_UPDATE(fromanc2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

ROM_START( fromanc2 )
	ROM_REGION( 0x0080000, REGION_CPU1, 0 )			// MAIN CPU
	ROM_LOAD16_WORD_SWAP( "4-ic23.bin", 0x000000, 0x080000, 0x96c90f9e )

	ROM_REGION( 0x0010000, REGION_CPU2, 0 )			// SOUND CPU
	ROM_LOAD( "5-ic85.bin",  0x00000, 0x10000, 0xd8f19aa3 )

	ROM_REGION( 0x0020000, REGION_CPU3, 0 )			// SUB CPU + BANK RAM
	ROM_LOAD( "3-ic1.bin",   0x00000, 0x10000, 0x6d02090e )

	ROM_REGION( 0x0480000, REGION_GFX1, ROMREGION_DISPOSE )	// LAYER4 DATA
	ROM_LOAD( "124-121.bin", 0x000000, 0x200000, 0x0b62c9c5 )
	ROM_LOAD( "125-122.bin", 0x200000, 0x200000, 0x1d6dc86e )
	ROM_LOAD( "126-123.bin", 0x400000, 0x080000, 0x9c0f7abc )

	ROM_REGION( 0x0480000, REGION_GFX2, ROMREGION_DISPOSE )	// LAYER3 DATA
	ROM_LOAD( "35-47.bin",   0x000000, 0x200000, 0x97ff0ad6 )
	ROM_LOAD( "161-164.bin", 0x200000, 0x200000, 0xeedbc4d1 )
	ROM_LOAD( "162-165.bin", 0x400000, 0x080000, 0x9b546e59 )

	ROM_REGION( 0x0200000, REGION_GFX3, ROMREGION_DISPOSE )	// LAYER2 DATA
	ROM_LOAD( "36-48.bin",   0x000000, 0x200000, 0xc8ee7f40 )

	ROM_REGION( 0x0100000, REGION_GFX4, ROMREGION_DISPOSE )	// LAYER1 DATA
	ROM_LOAD( "40-52.bin",   0x000000, 0x100000, 0xdbb5062d )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 )		// SOUND DATA
	ROM_LOAD( "ic96.bin",    0x000000, 0x200000, 0x2f1b394c )
	ROM_LOAD( "ic97.bin",    0x200000, 0x200000, 0x1d1377fc )
ROM_END

ROM_START( fromancr )
	ROM_REGION( 0x0080000, REGION_CPU1, 0 )			// MAIN CPU
	ROM_LOAD16_WORD_SWAP( "2-ic20.bin", 0x000000, 0x080000, 0x378eeb9c )

	ROM_REGION( 0x0010000, REGION_CPU2, 0 )			// SOUND CPU
	ROM_LOAD( "5-ic73.bin",  0x0000000, 0x010000, 0x3e4727fe )

	ROM_REGION( 0x0020000, REGION_CPU3, 0 )			// SUB CPU + BANK RAM
	ROM_LOAD( "4-ic1.bin",   0x0000000, 0x010000, 0x6d02090e )

	ROM_REGION( 0x0800000, REGION_GFX1, ROMREGION_DISPOSE )	// BG DATA
	ROM_LOAD( "ic1-3.bin",   0x0000000, 0x400000, 0x70ad9094 )
	ROM_LOAD( "ic2-4.bin",   0x0400000, 0x400000, 0xc6c6e8f7 )

	ROM_REGION( 0x2400000, REGION_GFX2, ROMREGION_DISPOSE )	// FG DATA
	ROM_LOAD( "ic28-13.bin", 0x0000000, 0x400000, 0x7d7f9f63 )
	ROM_LOAD( "ic29-14.bin", 0x0400000, 0x400000, 0x8ec65f31 )
	ROM_LOAD( "ic31-16.bin", 0x0800000, 0x400000, 0xe4859534 )
	ROM_LOAD( "ic32-17.bin", 0x0c00000, 0x400000, 0x20d767da )
	ROM_LOAD( "ic34-19.bin", 0x1000000, 0x400000, 0xd62a383f )
	ROM_LOAD( "ic35-20.bin", 0x1400000, 0x400000, 0x4e697f38 )
	ROM_LOAD( "ic37-22.bin", 0x1800000, 0x400000, 0x6302bf5f )
	ROM_LOAD( "ic38-23.bin", 0x1c00000, 0x400000, 0xc6cffa53 )
	ROM_LOAD( "ic40-25.bin", 0x2000000, 0x400000, 0xaf60bd0e )

	ROM_REGION( 0x0200000, REGION_GFX3, ROMREGION_DISPOSE )	// TEXT DATA
	ROM_LOAD( "ic28-29.bin", 0x0000000, 0x200000, 0xf5e262aa )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 )		// SOUND DATA
	ROM_LOAD( "ic81.bin",    0x0000000, 0x200000, 0x8ab6e343 )
	ROM_LOAD( "ic82.bin",    0x0200000, 0x200000, 0xf57daaf8 )
ROM_END

ROM_START( fromanc4 )
	ROM_REGION( 0x0800000, REGION_CPU1, 0 )			// MAIN CPU + DATA
	ROM_LOAD16_WORD_SWAP( "ic18.bin",    0x0000000, 0x080000, 0x46a47839 )
	ROM_LOAD16_WORD_SWAP( "em33-m00.19", 0x0400000, 0x400000, 0x6442534b )

	ROM_REGION( 0x0020000, REGION_CPU2, 0 )			// SOUND CPU
	ROM_LOAD( "ic79.bin", 0x0000000, 0x020000, 0xc9587c09 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	// BG DATA
	ROM_LOAD16_WORD_SWAP( "em33-c00.59", 0x0000000, 0x400000, 0x7192bbad )
	ROM_LOAD16_WORD_SWAP( "em33-c01.60", 0x0400000, 0x400000, 0xd75af19a )
	ROM_LOAD16_WORD_SWAP( "em33-c02.61", 0x0800000, 0x400000, 0x4f4d2735 )
	ROM_LOAD16_WORD_SWAP( "em33-c03.62", 0x0c00000, 0x400000, 0x7ece6ad5 )

	ROM_REGION( 0x3000000, REGION_GFX2, ROMREGION_DISPOSE )	// FG DATA
	ROM_LOAD16_WORD_SWAP( "em33-b00.38", 0x0000000, 0x400000, 0x10b8f90d )
	ROM_LOAD16_WORD_SWAP( "em33-b01.39", 0x0400000, 0x400000, 0x3b3ea291 )
	ROM_LOAD16_WORD_SWAP( "em33-b02.40", 0x0800000, 0x400000, 0xde88f95b )
	ROM_LOAD16_WORD_SWAP( "em33-b03.41", 0x0c00000, 0x400000, 0x35c1b398 )
	ROM_LOAD16_WORD_SWAP( "em33-b04.42", 0x1000000, 0x400000, 0x84b8d5db )
	ROM_LOAD16_WORD_SWAP( "em33-b05.43", 0x1400000, 0x400000, 0xb822b57c )
	ROM_LOAD16_WORD_SWAP( "em33-b06.44", 0x1800000, 0x400000, 0x8f1b2b19 )
	ROM_LOAD16_WORD_SWAP( "em33-b07.45", 0x1c00000, 0x400000, 0xdd4ddcb7 )
	ROM_LOAD16_WORD_SWAP( "em33-b08.46", 0x2000000, 0x400000, 0x3d8ce018 )
	ROM_LOAD16_WORD_SWAP( "em33-b09.47", 0x2400000, 0x400000, 0x4ad79143 )
	ROM_LOAD16_WORD_SWAP( "em33-b10.48", 0x2800000, 0x400000, 0xd6ab74b2 )
	ROM_LOAD16_WORD_SWAP( "em33-b11.49", 0x2c00000, 0x400000, 0x4aa206b1 )

	ROM_REGION( 0x0400000, REGION_GFX3, ROMREGION_DISPOSE )	// TEXT DATA
	ROM_LOAD16_WORD_SWAP( "em33-a00.37", 0x0000000, 0x400000, 0xa3bd4a34 )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 )		// SOUND DATA
	ROM_LOAD16_WORD_SWAP( "em33-p00.88", 0x0000000, 0x400000, 0x1c6418d2 )
	ROM_LOAD16_WORD_SWAP( "em33-p01.89", 0x0400000, 0x400000, 0x615b4e6e )
ROM_END


GAME( 1995, fromanc2, 0, fromanc2, fromanc2, fromanc2, ROT0, "Video System", "Taisen Idol-Mahjong Final Romance 2 (Japan)" )
GAME( 1995, fromancr, 0, fromancr, fromanc2, fromancr, ROT0, "Video System", "Taisen Mahjong FinalRomance R (Japan)" )
GAME( 1998, fromanc4, 0, fromanc4, fromanc4, fromanc4, ROT0, "Video System", "Taisen Mahjong FinalRomance 4 (Japan)" )
