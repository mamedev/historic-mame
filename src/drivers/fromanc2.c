/******************************************************************************

	Game Driver for Video System Mahjong series. (WIP Driver)

	Taisen Idol-Mahjong Final Romance 2
	(c)1995 Video System Co.,Ltd.

	Taisen Mahjong FinalRomance R
	(c)1995 Video System Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2001/02/28 -

******************************************************************************/
/******************************************************************************
Memo:

Board No.VSAM-28-2 (Final Romance 2)

OSC	32MHz
	14.31818MHz

CPU	#1 TMP68HC000P-16 (?)
	#2 Z80 (?)
	#3 Z80 (SOUND)

RAM	#1 N341256P-25 x2	(ROM IC23)
	#2 KM62256BLS-7L x2	(ROM IC1)
	#3 MCM6264CP35		(ROM IC85, IC96, IC97)

	#1 V1 CXK5863P-30 x2
	#1 V2 CXK5863P-30 x2
	#2 V1 CXK5863P-30 x2
	#2 V2 CXK5863P-30 x2

	#1 P BR6216B-10LL x2
	#1 P BR6216B-10LL x2

SOUND	YM2610
	YM3016-D

CUSTOM	VS920E x4
	VS920A x2

ETC	93C46A		SERIAL EEPROM	1Kb(64x16 / 128x8)


Board No.VSMR-30 (Final Romance R)

OSC	32MHz
	14.31818MHz

CPU	#1 TMP68HC000P-16 (?)
	#2 Z80 (?)
	#3 Z80 (SOUND)

RAM	#1 N341256P-15 x2	(ROM IC20)
	#2 N341256P-15 x2	(ROM IC1)
	#3 CY7C185-15PC		(ROM IC73, IC81, IC82)

	V1 CY7C185-15PC x2	(ROM SUB BOARD)
	P1 CY7128A-35PC x2
	V2 CY7C185-15PC x2	(ROM SUB BOARD)
	P2 CY7128A-35PC x2
	V3 CY7C185-15PC x2	(ROM IC28, IC29)

SOUND	YM2610
	YM3016-D

CUSTOM	VS920E x3
	VS920A x2

ETC	93C46A		SERIAL EEPROM	1Kb(64x16 / 128x8)

******************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"


void fromanc2_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
int fromanc2_vh_start(void);
void fromanc2_vh_stop(void);
void fromancr_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
int fromancr_vh_start(void);
void fromancr_vh_stop(void);

READ16_HANDLER( fromanc2_paletteram_0_r );
READ16_HANDLER( fromanc2_paletteram_1_r );
WRITE16_HANDLER( fromanc2_paletteram_0_w );
WRITE16_HANDLER( fromanc2_paletteram_1_w );
READ16_HANDLER( fromancr_paletteram_0_r );
READ16_HANDLER( fromancr_paletteram_1_r );
WRITE16_HANDLER( fromancr_paletteram_0_w );
WRITE16_HANDLER( fromancr_paletteram_1_w );
WRITE16_HANDLER( fromancr_videoram_0_w );
WRITE16_HANDLER( fromancr_videoram_1_w );
WRITE16_HANDLER( fromancr_videoram_2_w );
WRITE16_HANDLER( fromancr_videoram_3_w );
WRITE16_HANDLER( fromanc2_videoram_0_w );
WRITE16_HANDLER( fromanc2_videoram_1_w );
WRITE16_HANDLER( fromanc2_videoram_2_w );
WRITE16_HANDLER( fromanc2_videoram_3_w );
WRITE16_HANDLER( fromanc2_gfxreg_0_w );
WRITE16_HANDLER( fromanc2_gfxreg_1_w );
WRITE16_HANDLER( fromanc2_gfxreg_2_w );
WRITE16_HANDLER( fromanc2_gfxreg_3_w );
WRITE16_HANDLER( fromancr_gfxreg_0_w );
WRITE16_HANDLER( fromancr_gfxreg_1_w );
WRITE16_HANDLER( fromanc2_gfxbank_0_w );
WRITE16_HANDLER( fromanc2_gfxbank_1_w );
WRITE16_HANDLER( fromancr_gfxbank_w );


int fromanc2_portselect;


void fromanc2_init_machine(void)
{
	//
}

void fromancr_init_machine(void)
{
	//
}


void init_fromanc2(void)
{
//	unsigned char *ROM = memory_region(REGION_CPU1);
}

void init_fromancr(void)
{
//	unsigned char *ROM = memory_region(REGION_CPU1);
}


WRITE16_HANDLER( fromanc2_sndcmd_w )
{
	soundlatch_w(offset, ((data >> 8) & 0xff));	// 1P (LEFT)
	soundlatch2_w(offset, (data & 0xff));		// 2P (RIGHT)
	cpu_cause_interrupt(2, Z80_NMI_INT);
}

WRITE16_HANDLER( fromanc2_portselect_w )
{
	fromanc2_portselect = data;
}

READ16_HANDLER( fromanc2_keymatrix_r )
{
	int ret;

	switch (fromanc2_portselect)
	{
		case	0x01:	ret = readinputport(1);	break;
		case	0x02:	ret = readinputport(2); break;
		case	0x04:	ret = readinputport(3); break;
		case	0x08:	ret = readinputport(4); break;
		default:	ret = 0xffff;
				logerror("PC:%08X unknown %02X\n", cpu_get_pc(), fromanc2_portselect);
				break;
	}

	return ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
READ16_HANDLER( fromanc2_input_r )
{
	unsigned short coinsw;

#if 0
	coinsw = (readinputport(0) & 0x030f);	// COIN 1-4, SERVIER 1-2
	return (coinsw | 0x0000);
#else
	coinsw = (readinputport(0) & 0x03ff);	// COIN 1-4, SERVIER 1-2, FLAGS
	return (coinsw | 0x0000);
#endif
}

// ----------------------------------------------------------------------------
//	#0 M68000 (Main Program / 4-IC23.BIN)	Used INT(vblank)
//	#0 M68000 (Main Program / 2-IC20.BIN)	Used INT(vblank)
//
//	0xd01100	R	bit  0	0x0001	COIN1
//				bit  1	0x0002	COIN2
//				bit  2	0x0004	COIN3
//				bit  3	0x0008	COIN4
//				bit  4	0x0010	0: wait loop?
//						1: 0xd01200 out ?
//				bit  5	0x0020	sound status check ? (fromancr)
//				bit  6	0x0040	0: wait loop?
//						1: 0xd01300 read ?
//				bit  7	0x0080	0xd01600 out flag?
//				bit  8	0x0100	SERVICE
//				bit  9	0x0200	SERVICE
//				bit 10	0x0400	-
//				bit 11	0x0800	-
//				bit 12	0x1000	-
//				bit 13	0x2000	-
//				bit 14	0x4000	-
//				bit 15	0x8000	-
//
//	0xd01100	R			INPUT COMMON
//	0xd01300	R			?
//	0xd01800	R			INPUT KEY MATRIX
//	0xd789a0	R			?
//	0xd789a2	R			?
//
//
//	0xd01000	W			SOUND REQ (1P/2P)
//	0xd01200	W			?
//	0xd01400	W			GFXBANK (1P)
//	0xd01500	W			GFXBANK (2P)
//	0xd01600	W			?
//	0xd01a00	W			PORT SELECT (1P/2P)
//
//
// ----------------------------------------------------------------------------
MEMORY_READ16_START( fromanc2_readmem_main )
	{ 0x000000, 0x07ffff, MRA16_ROM },		// ROM

	{ 0xa00000, 0xa00fff, fromanc2_paletteram_0_r },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromanc2_paletteram_1_r },// PALETTE (2P)

	{ 0xd01100, 0xd01101, fromanc2_input_r },	// INPUT COMMON
	{ 0xd01300, 0xd01301, MRA16_NOP },		// SUB CPU READ ?
	{ 0xd01800, 0xd01801, fromanc2_keymatrix_r },	// INPUT KEY MATRIX

	{ 0xd789a0, 0xd789a1, MRA16_NOP },		// ?????
	{ 0xd789a2, 0xd789a3, MRA16_NOP },		// ?????

	{ 0xd80000, 0xd8ffff, MRA16_RAM },		// WORK RAM
MEMORY_END

MEMORY_WRITE16_START( fromanc2_writemem_main )
	{ 0x000000, 0x07ffff, MWA16_ROM },		// ROM

	{ 0x800000, 0x803fff, fromanc2_videoram_1_w },	// VRAM FG (1P/2P)
	{ 0x880000, 0x883fff, fromanc2_videoram_0_w },	// VRAM BG (1P/2P)
	{ 0x900000, 0x903fff, fromanc2_videoram_3_w },	// VRAM ?? (1P/2P)
	{ 0x980000, 0x983fff, fromanc2_videoram_2_w },	// VRAM ?? (1P/2P)

	{ 0xa00000, 0xa00fff, fromanc2_paletteram_0_w },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromanc2_paletteram_1_w },// PALETTE (2P)

	{ 0xd00000, 0xd00023, fromanc2_gfxreg_0_w },	// SCROLL REG (1P/2P)
	{ 0xd00100, 0xd00123, fromanc2_gfxreg_2_w },	// SCROLL REG (1P/2P)
	{ 0xd00200, 0xd00223, fromanc2_gfxreg_1_w },	// SCROLL REG (1P/2P)
	{ 0xd00300, 0xd00323, fromanc2_gfxreg_3_w },	// SCROLL REG (1P/2P)

	{ 0xd00400, 0xd00413, MWA16_NOP },		// ???
	{ 0xd00500, 0xd00513, MWA16_NOP },		// ???

	{ 0xd01000, 0xd01001, fromanc2_sndcmd_w },	// SOUND REQ (1P/2P)
	{ 0xd01200, 0xd01201, MWA16_NOP },		// SUB CPU WRITE ?
	{ 0xd01400, 0xd01401, fromanc2_gfxbank_0_w },	// GFXBANK (1P)
	{ 0xd01500, 0xd01501, fromanc2_gfxbank_1_w },	// GFXBANK (2P)
	{ 0xd01600, 0xd01601, MWA16_NOP },		// EEPROM WRITE ?
	{ 0xd01a00, 0xd01a01, fromanc2_portselect_w },	// PORT SELECT (1P/2P)

	{ 0xd80000, 0xd8ffff, MWA16_RAM },		// WORK RAM
MEMORY_END

MEMORY_READ16_START( fromancr_readmem_main )
	{ 0x000000, 0x07ffff, MRA16_ROM },		// ROM

	{ 0xa00000, 0xa00fff, fromancr_paletteram_0_r },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromancr_paletteram_1_r },// PALETTE (2P)

	{ 0xd01100, 0xd01101, fromanc2_input_r },	// INPUT COMMON
	{ 0xd01300, 0xd01301, MRA16_NOP },		// SUB CPU READ ?
	{ 0xd01800, 0xd01801, fromanc2_keymatrix_r },	// INPUT KEY MATRIX

	{ 0xd789a0, 0xd789a1, MRA16_NOP },		// ?????
	{ 0xd789a2, 0xd789a3, MRA16_NOP },		// ?????

	{ 0xd80000, 0xd8ffff, MRA16_RAM },		// WORK RAM
MEMORY_END

MEMORY_WRITE16_START( fromancr_writemem_main )
	{ 0x000000, 0x07ffff, MWA16_ROM },		// ROM

	{ 0x800000, 0x803fff, fromancr_videoram_1_w },	// VRAM FG (1P/2P)
	{ 0x880000, 0x883fff, fromancr_videoram_0_w },	// VRAM BG (1P/2P)
	{ 0x900000, 0x903fff, fromancr_videoram_2_w },	// VRAM TEXT (1P/2P)
	{ 0x980000, 0x983fff, MWA16_NOP },		// VRAM Unused ?

	{ 0xa00000, 0xa00fff, fromancr_paletteram_0_w },// PALETTE (1P)
	{ 0xa80000, 0xa80fff, fromancr_paletteram_1_w },// PALETTE (2P)

	{ 0xd00000, 0xd00023, fromancr_gfxreg_1_w },	// SCROLL REG (1P/2P)
	{ 0xd00100, 0xd00123, fromancr_gfxreg_0_w },	// SCROLL REG (1P/2P)

	{ 0xd00200, 0xd002ff, MWA16_NOP },		// EEPROM WRITE ?

	{ 0xd00400, 0xd00413, MWA16_NOP },		// ???
	{ 0xd00500, 0xd00513, MWA16_NOP },		// ???

	{ 0xd01000, 0xd01001, fromanc2_sndcmd_w },	// SOUND REQ (1P/2P)
	{ 0xd01200, 0xd01201, MWA16_NOP },		// SUB CPU WRITE ?
	{ 0xd01400, 0xd01401, MWA16_NOP },		// COIN COUNTER ?
	{ 0xd01600, 0xd01601, fromancr_gfxbank_w },	// GFXBANK (1P/2P)
	{ 0xd01a00, 0xd01a01, fromanc2_portselect_w },	// PORT SELECT (1P/2P)

	{ 0xd80000, 0xd8ffff, MWA16_RAM },		// WORK RAM
MEMORY_END

// ----------------------------------------------------------------------------
//	#1 Z80-1 (Sub Program / 3-IC1.BIN)	Used INT(?)/NMI(?)
//	#1 Z80-1 (Sub Program / 4-IC1.BIN)	Used INT(?)/NMI(?)
//
//	RAM	256k x2		(0x8000 x2)
//
//
//	INT = 68000 -> data in ?
//
//	NMI = 68000 <- data out ?
//
//
//	0x02	R	INT after DATA IN 1 ?
//	0x04	R	INT after DATA IN 2 ?
//
//	0x00	W	INT after DATA OUT ?
//
//	0x02	W	NMI after DATA OUT 1 ?
//	0x04	W	NMI after DATA OUT 2 ?
//	0x06	W	NMI after DUMMY OUT ?
//
// ----------------------------------------------------------------------------
MEMORY_READ_START( fromanc2_readmem_sub )
	{ 0x0000, 0x7fff, MRA_ROM },			// ROM
	{ 0x8000, 0xbfff, MRA_RAM },			// RAM(WORK)
	{ 0xc000, 0xccff, MRA_RAM },			// RAM?
MEMORY_END

MEMORY_WRITE_START( fromanc2_writemem_sub )
	{ 0x0000, 0x7fff, MWA_ROM },			// ROM
	{ 0x8000, 0xbfff, MWA_RAM },			// RAM(WORK)
	{ 0xc000, 0xccff, MWA_RAM },			// RAM?
MEMORY_END

PORT_READ_START( fromanc2_readport_sub )
	{ 0x02, 0x02, IORP_NOP },			// to MAIN ?
	{ 0x04, 0x04, IORP_NOP },			// to MAIN ?
PORT_END

PORT_WRITE_START( fromanc2_writeport_sub )
	{ 0x00, 0x00, IOWP_NOP },			// from MAIN ?
	{ 0x02, 0x02, IOWP_NOP },			// from MAIN ?
	{ 0x04, 0x04, IOWP_NOP },			// from MAIN ?
	{ 0x06, 0x06, IOWP_NOP },			// dummy out from NMI
PORT_END

// ----------------------------------------------------------------------------
//	#2 Z80-2 (Sound Program / 5-IC85.BIN)	Used INT(vblank?)/ NMI(REQ?)
//	#2 Z80-2 (Sound Program / 5-IC73.BIN)	Used INT(vblank?)/ NMI(REQ?)
//
//	RAM	 64k
//
// ----------------------------------------------------------------------------
MEMORY_READ_START( fromanc2_readmem_sound )
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( fromanc2_writemem_sound )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

PORT_READ_START( fromanc2_readport_sound )
	{ 0x00, 0x00, soundlatch_r },				// snd cmd (1P)
	{ 0x04, 0x04, soundlatch2_r },				// snd cmd (2P)
	{ 0x09, 0x09, IORP_NOP },				// ?
	{ 0x08, 0x08, YM2610_status_port_0_A_r },
	{ 0x0a, 0x0a, YM2610_status_port_0_B_r },
	{ 0x0c, 0x0c, IORP_NOP },				// dummy read ?
PORT_END

PORT_WRITE_START( fromanc2_writeport_sound )
	// pending_command_clear_w ?
	{ 0x00, 0x00, IOWP_NOP },				// ?
	{ 0x08, 0x08, YM2610_control_port_0_A_w },
	{ 0x09, 0x09, YM2610_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2610_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2610_data_port_0_B_w },
PORT_END


INPUT_PORTS_START( fromanc2 )
	PORT_START	/* (0) COIN SW, TEST SW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1 (1P)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2 (1P)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )		// COIN1 (2P)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )		// COIN2 (2P)
#if 0
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
#else
	// MAIN CPU & SUB CPU comm flag ?
	PORT_DIPNAME( 0x0010, 0x0010, "0xD01100 bit4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "0xD01100 bit5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "0xD01100 bit6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "0xD01100 bit7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
#endif

	PORT_BITX( 0x0100, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST (1P)
	PORT_BITX( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST (2P)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) PORT 1-0 */
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0100, IP_ACTIVE_LOW, 0, "P2 A", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, 0, "P2 E", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, 0, "P2 I", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "P2 M", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "P2 Kan", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (2) PORT 1-1 */
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0100, IP_ACTIVE_LOW, 0, "P2 B", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, 0, "P2 F", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, 0, "P2 J", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "P2 N", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "P2 Reach", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (3) PORT 1-2 */
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0100, IP_ACTIVE_LOW, 0, "P2 C", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, 0, "P2 G", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, 0, "P2 K", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "P2 Chi", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "P2 Ron", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (4) PORT 1-3 */
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0100, IP_ACTIVE_LOW, 0, "P2 D", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, 0, "P2 H", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, 0, "P2 L", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "P2 Pon", IP_KEY_DEFAULT, IP_JOY_NONE )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


struct GfxLayout fromanc2_tilelayout =
{
	8, 8,
	RGN_FRAC(1, 1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

struct GfxDecodeInfo fromanc2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fromanc2_tilelayout, (  0 * 2), (256 * 2) },
	{ REGION_GFX2, 0, &fromanc2_tilelayout, (256 * 2), (256 * 2) },
	{ REGION_GFX3, 0, &fromanc2_tilelayout, (512 * 2), (256 * 2) },
	{ REGION_GFX4, 0, &fromanc2_tilelayout, (768 * 2), (256 * 2) },
	{ -1 } /* end of array */
};

struct GfxLayout fromancr_tilelayout =
{
	8, 8,
	RGN_FRAC(1, 1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 1*8, 0*8, 3*8, 2*8, 5*8, 4*8, 7*8, 6*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

struct GfxDecodeInfo fromancr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fromancr_tilelayout, (512 * 2), 2 },	// BG
	{ REGION_GFX2, 0, &fromancr_tilelayout, (256 * 2), 2 },	// FG
	{ REGION_GFX3, 0, &fromancr_tilelayout, (  0 * 2), 2 },	// TEXT
	{ -1 } /* end of array */
};


void irqhandler(int irq)
{
	cpu_set_irq_line(2, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

struct YM2610interface ym2610_interface =
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
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) }
};


struct MachineDriver machine_driver_fromanc2 =
{
	{
		{
			CPU_M68000,
			32000000/2,		/* 16.00 MHz ? */
			fromanc2_readmem_main, fromanc2_writemem_main, 0, 0,
			m68_level1_irq, 1
		},
		{
			CPU_Z80,
			32000000/4,		/* 8.00 MHz ? */
			fromanc2_readmem_sub, fromanc2_writemem_sub, fromanc2_readport_sub, fromanc2_writeport_sub,
			ignore_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			32000000/4,		/* 8.00 MHz ? */
			fromanc2_readmem_sound, fromanc2_writemem_sound, fromanc2_readport_sound, fromanc2_writeport_sound,
			ignore_interrupt, 0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	fromanc2_init_machine,

	/* video hardware */
//	512, 512, { 0, 352-1, 0, 240-1 },
	704, 512, { 0, 704-1, 0, 240-1 },
//	704, 512, { 0, 352-1, 0, 240-1 },
	fromanc2_gfxdecodeinfo,
	4096, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR | VIDEO_ASPECT_RATIO(8,3),
	0,
	fromanc2_vh_start,
	fromanc2_vh_stop,
	fromanc2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO, 0, 0, 0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	},
	0
};

struct MachineDriver machine_driver_fromancr =
{
	{
		{
			CPU_M68000,
			32000000/2,		/* 16.00 MHz ? */
			fromancr_readmem_main, fromancr_writemem_main, 0, 0,
			m68_level1_irq, 1
		},
		{
			CPU_Z80,
			32000000/4,		/* 8.00 MHz ? */
			fromanc2_readmem_sub, fromanc2_writemem_sub, fromanc2_readport_sub, fromanc2_writeport_sub,
			ignore_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			32000000/4,		/* 8.00 MHz ? */
			fromanc2_readmem_sound, fromanc2_writemem_sound, fromanc2_readport_sound, fromanc2_writeport_sound,
			ignore_interrupt, 0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	fromancr_init_machine,

	/* video hardware */
//	512, 512, { 0, 352-1, 0, 240-1 },
	704, 512, { 0, 704-1, 0, 240-1 },
//	704, 512, { 0, 352-1, 0, 240-1 },
	fromancr_gfxdecodeinfo,
	4096, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR | VIDEO_ASPECT_RATIO(8,3),
	0,
	fromancr_vh_start,
	fromancr_vh_stop,
	fromancr_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO, 0, 0, 0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	},
	0
};


ROM_START( fromanc2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "4-ic23.bin", 0x000000, 0x080000, 0x96c90f9e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "3-ic1.bin",   0x00000, 0x10000, 0x6d02090e )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "5-ic85.bin",  0x00000, 0x10000, 0xd8f19aa3 )

	ROM_REGION( 0x480000, REGION_GFX1, ROMREGION_DISPOSE )		// 4th
	ROM_LOAD( "124-121.bin", 0x000000, 0x200000, 0x0b62c9c5 )
	ROM_LOAD( "125-122.bin", 0x200000, 0x200000, 0x1d6dc86e )
	ROM_LOAD( "126-123.bin", 0x400000, 0x080000, 0x9c0f7abc )

	ROM_REGION( 0x480000, REGION_GFX2, ROMREGION_DISPOSE )		// 3rd
	ROM_LOAD( "35-47.bin",   0x000000, 0x200000, 0x97ff0ad6 )
	// 0x200000 - unknown
	ROM_LOAD( "162-165.bin", 0x400000, 0x080000, 0x9b546e59 )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )		// 2nd
	ROM_LOAD( "36-48.bin",   0x000000, 0x200000, 0xc8ee7f40 )
	ROM_LOAD( "161-164.bin", 0x200000, 0x200000, 0xeedbc4d1 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )		// 1st
	ROM_LOAD( "40-52.bin",   0x000000, 0x100000, 0xdbb5062d )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic96.bin",    0x000000, 0x200000, 0x2f1b394c )
	ROM_LOAD( "ic97.bin",    0x200000, 0x200000, 0x1d1377fc )
ROM_END

ROM_START( fromancr )
	ROM_REGION( 0x0080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "2-ic20.bin", 0x000000, 0x080000, 0x378eeb9c )

	ROM_REGION( 0x0010000, REGION_CPU2, 0 )
	ROM_LOAD( "4-ic1.bin",   0x0000000, 0x010000, 0x6d02090e )

	ROM_REGION( 0x0010000, REGION_CPU3, 0 )
	ROM_LOAD( "5-ic73.bin",  0x0000000, 0x010000, 0x3e4727fe )

	ROM_REGION( 0x0800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic1-3.bin",   0x0000000, 0x400000, 0x70ad9094 )
	ROM_LOAD( "ic2-4.bin",   0x0400000, 0x400000, 0xc6c6e8f7 )

	ROM_REGION( 0x2400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic28-13.bin", 0x0000000, 0x400000, 0x7d7f9f63 )
	ROM_LOAD( "ic29-14.bin", 0x0400000, 0x400000, 0x8ec65f31 )
	ROM_LOAD( "ic31-16.bin", 0x0800000, 0x400000, 0xe4859534 )
	ROM_LOAD( "ic32-17.bin", 0x0c00000, 0x400000, 0x20d767da )
	ROM_LOAD( "ic34-19.bin", 0x1000000, 0x400000, 0xd62a383f )
	ROM_LOAD( "ic35-20.bin", 0x1400000, 0x400000, 0x4e697f38 )
	ROM_LOAD( "ic37-22.bin", 0x1800000, 0x400000, 0x6302bf5f )
	ROM_LOAD( "ic38-23.bin", 0x1c00000, 0x400000, 0xc6cffa53 )
	ROM_LOAD( "ic40-25.bin", 0x2000000, 0x400000, 0xaf60bd0e )

	ROM_REGION( 0x0200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ic28-29.bin", 0x0000000, 0x200000, 0xf5e262aa )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic81.bin",    0x0000000, 0x200000, 0x8ab6e343 )
	ROM_LOAD( "ic82.bin",    0x0200000, 0x200000, 0xf57daaf8 )
ROM_END


GAMEX( 1995, fromanc2, 0, fromanc2, fromanc2, fromanc2, ROT0, "Video System", "Taisen Idol-Mahjong Final Romance 2 (Japan)", GAME_NOT_WORKING )
GAMEX( 1995, fromancr, 0, fromancr, fromanc2, fromancr, ROT0, "Video System", "Taisen Mahjong FinalRomance R (Japan)", GAME_NOT_WORKING )
