/***************************************************************************

	Atari GT hardware

	driver by Aaron Giles

	Games supported:
		* T-Mek (1994) [2 sets]
		* Primal Rage (1994) [2 sets]

	Known bugs:
		* protection devices unknown
		* gamma map (?)
		* CAGE audio system missing

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/atarirle.h"
#include "cpu/m68000/m68000.h"
#include "atarigt.h"


#define LOG_PROTECTION		1



/*************************************
 *
 *	Statics
 *
 *************************************/

static void (*protection_w)(offs_t offset, data16_t data);
static void (*protection_r)(offs_t offset, data16_t *data);



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 4;
	if (atarigen_sound_int_state)
		newstate = 5;
	if (atarigen_scanline_int_state)
		newstate = 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( atarigt )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(atarigt_scanline_update, 8);
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

static READ32_HANDLER( inputs_01_r )
{
	return (readinputport(0) << 16) | readinputport(1);
}


static READ32_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	temp ^= 0x0001;		/* /A2DRDY always high for now */
	temp ^= 0x0008;		/* A2D.EOC always high for now */
	return (temp << 16) | temp;
}


static READ32_HANDLER( special_port3_r )
{
	int temp = readinputport(3);
	if (atarigen_video_int_state) temp ^= 0x0001;
	if (atarigen_scanline_int_state) temp ^= 0x0002;
	return (temp << 16) | temp;
}



/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE32_HANDLER( latch_w )
{
	/*
		D13 = 68.DISA
		D12 = ERASE
		D11 = /MOGO
		D8  = VCR
		D5  = /XRESET
		D4  = /SNDRES
		D3  = CC.L
		D0  = CC.R
	*/

	/* upper byte */
	if (!(mem_mask & 0xff000000))
	{
		/* bits 13-11 are the MO control bits */
		atarirle_control_w(0, (data >> 27) & 7);
	}

	if (!(mem_mask & 0x00ff0000))
	{
		coin_counter_w(0, data & 0x00080000);
		coin_counter_w(1, data & 0x00010000);
	}

//logerror("latch=%08X\n", latch);

//	if (ACCESSING_MSW32 && !ACCESSING_MSB32)
//		cpu_set_reset_line(1, (data & 0x100000) ? CLEAR_LINE : ASSERT_LINE);
}


static WRITE32_HANDLER( led_w )
{
//	logerror("LED = %08X & %08X\n", data, ~mem_mask);
}



/*************************************
 *
 *	Sound I/O
 *
 *************************************/

static data32_t sound_data;
static READ32_HANDLER( sound_data_r )
{
	return sound_data;
}


static WRITE32_HANDLER( sound_data_w )
{
	COMBINE_DATA(&sound_data);
}



/*************************************
 *
 *	T-Mek protection
 *
 *************************************/

#define ADDRSEQ_COUNT	4

static offs_t protaddr[ADDRSEQ_COUNT];
static UINT8 protmode;
static UINT16 protresult;
static UINT8 protdata[0x800];

static void tmek_update_mode(offs_t offset)
{
	int i;

	/* pop us into the readseq */
	for (i = 0; i < ADDRSEQ_COUNT - 1; i++)
		protaddr[i] = protaddr[i + 1];
	protaddr[ADDRSEQ_COUNT - 1] = offset;

	/* check for particular sequences */
	if (!protmode)
	{
		/* this is from the code at $20f90 */
		if (protaddr[1] == 0xdcc7c4 && protaddr[2] == 0xdcc7c4 && protaddr[3] == 0xdc4010)
		{
//			logerror("prot:Entering mode 1\n");
			protmode = 1;
		}

		/* this is from the code at $27592 */
		if (protaddr[0] == 0xdcc7ca && protaddr[1] == 0xdcc7ca && protaddr[2] == 0xdcc7c6 && protaddr[3] == 0xdc4022)
		{
//			logerror("prot:Entering mode 2\n");
			protmode = 2;
		}

		/* this is from the code at $3d8dc */
		if (protaddr[0] == 0xdcc7c0 && protaddr[1] == 0xdcc7c0 && protaddr[2] == 0xdc80f2 && protaddr[3] == 0xdc7af2)
		{
//			logerror("prot:Entering mode 3\n");
			protmode = 3;
		}
	}
}


static void tmek_protection_w(offs_t offset, UINT16 data)
{
#if LOG_PROTECTION
	logerror("%06X:Protection W@%06X = %04X\n", activecpu_get_previouspc(), offset, data);
#endif

/* mask = 0x78fff */

	/* track accesses */
	tmek_update_mode(offset);

	/* check for certain read sequences */
	if (protmode == 1 && offset >= 0xdc7800 && offset < 0xdc7800 + sizeof(protdata) * 2)
		protdata[(offset - 0xdc7800) / 2] = data;

	if (protmode == 2)
	{
		int temp = (offset - 0xdc7800) / 2;
		logerror("prot:mode 2 param = %04X\n", temp);
		protresult = temp * 0x6915 + 0x6915;
	}

	if (protmode == 3)
	{
		if (offset == 0xdc4700)
		{
			logerror("prot:Clearing mode 3\n");
			protmode = 0;
		}
	}
}

static void tmek_protection_r(offs_t offset, data16_t *data)
{
	/* track accesses */
	tmek_update_mode(offset);

#if LOG_PROTECTION
	logerror("%06X:Protection R@%06X\n", activecpu_get_previouspc(), offset);
#endif

	/* handle specific reads */
	switch (offset)
	{
		/* status register; the code spins on this waiting for the high bit to be set */
		case 0xdb8700:
//			if (protmode != 0)
			{
				*data = 0x8000;
			}
			break;

		/* some kind of result register */
		case 0xdcc7c2:
			if (protmode == 2)
			{
				*data = protresult;
				protmode = 0;
				logerror("prot:Clearing mode 2\n");
			}
			break;

		case 0xdcc7c4:
			if (protmode == 1)
			{
				protmode = 0;
				logerror("prot:Clearing mode 1\n");
			}
			break;
	}
}



/*************************************
 *
 *	Primal Rage protection
 *
 *************************************/

static void primage_update_mode(offs_t offset)
{
	int i;

	/* pop us into the readseq */
	for (i = 0; i < ADDRSEQ_COUNT - 1; i++)
		protaddr[i] = protaddr[i + 1];
	protaddr[ADDRSEQ_COUNT - 1] = offset;

	/* check for particular sequences */
	if (!protmode)
	{
		/* this is from the code at $20f90 */
		if (protaddr[1] == 0xdcc7c4 && protaddr[2] == 0xdcc7c4 && protaddr[3] == 0xdc4010)
		{
//			logerror("prot:Entering mode 1\n");
			protmode = 1;
		}

		/* this is from the code at $27592 */
		if (protaddr[0] == 0xdcc7ca && protaddr[1] == 0xdcc7ca && protaddr[2] == 0xdcc7c6 && protaddr[3] == 0xdc4022)
		{
//			logerror("prot:Entering mode 2\n");
			protmode = 2;
		}

		/* this is from the code at $3d8dc */
		if (protaddr[0] == 0xdcc7c0 && protaddr[1] == 0xdcc7c0 && protaddr[2] == 0xdc80f2 && protaddr[3] == 0xdc7af2)
		{
//			logerror("prot:Entering mode 3\n");
			protmode = 3;
		}
	}
}



static void primrage_protection_w(offs_t offset, data16_t data)
{
#if LOG_PROTECTION
{
	UINT32 pc = activecpu_get_previouspc();
	switch (pc)
	{
		/* protection code from 20f90 - 21000 */
		case 0x20fba:
			if (offset % 16 == 0) logerror("\n   ");
			logerror("W@%06X(%04X) ", offset, data);
			break;

		/* protection code from 27592 - 27664 */
		case 0x275f6:
			logerror("W@%06X(%04X) ", offset, data);
			break;

		/* protection code from 3d8dc - 3d95a */
		case 0x3d908:
		case 0x3d932:
		case 0x3d938:
		case 0x3d93e:
			logerror("W@%06X(%04X) ", offset, data);
			break;
		case 0x3d944:
			logerror("W@%06X(%04X) - done\n", offset, data);
			break;

		/* protection code from 437fa - 43860 */
		case 0x43830:
		case 0x43838:
			logerror("W@%06X(%04X) ", offset, data);
			break;

		/* catch anything else */
		default:
			logerror("%06X:Unknown protection W@%06X = %04X\n", activecpu_get_previouspc(), offset, data);
			break;
	}
}
#endif

/* mask = 0x78fff */

	/* track accesses */
	primage_update_mode(offset);

	/* check for certain read sequences */
	if (protmode == 1 && offset >= 0xdc7800 && offset < 0xdc7800 + sizeof(protdata) * 2)
		protdata[(offset - 0xdc7800) / 2] = data;

	if (protmode == 2)
	{
		int temp = (offset - 0xdc7800) / 2;
//		logerror("prot:mode 2 param = %04X\n", temp);
		protresult = temp * 0x6915 + 0x6915;
	}

	if (protmode == 3)
	{
		if (offset == 0xdc4700)
		{
//			logerror("prot:Clearing mode 3\n");
			protmode = 0;
		}
	}
}



static void primrage_protection_r(offs_t offset, data16_t *data)
{
	/* track accesses */
	primage_update_mode(offset);

#if LOG_PROTECTION
{
	UINT32 pc = activecpu_get_previouspc();
	UINT32 p1, p2, a6;
	switch (pc)
	{
		/* protection code from 20f90 - 21000 */
		case 0x20f90:
			logerror("Known Protection @ 20F90: R@%06X ", offset);
			break;
		case 0x20f98:
		case 0x20fa0:
			logerror("R@%06X ", offset);
			break;
		case 0x20fcc:
			logerror("R@%06X - done\n", offset);
			break;

		/* protection code from 27592 - 27664 */
		case 0x275bc:
			break;
		case 0x275cc:
			a6 = activecpu_get_reg(M68K_A6);
			p1 = (cpu_readmem24bedw_word(a6+8) << 16) | cpu_readmem24bedw_word(a6+10);
			p2 = (cpu_readmem24bedw_word(a6+12) << 16) | cpu_readmem24bedw_word(a6+14);
			logerror("Known Protection @ 275BC(%08X, %08X): R@%06X ", p1, p2, offset);
			break;
		case 0x275d2:
		case 0x275d8:
		case 0x275de:
		case 0x2761e:
		case 0x2762e:
			logerror("R@%06X ", offset);
			break;
		case 0x2763e:
			logerror("R@%06X - done\n", offset);
			break;

		/* protection code from 3d8dc - 3d95a */
		case 0x3d8f4:
			a6 = activecpu_get_reg(M68K_A6);
			p1 = (cpu_readmem24bedw_word(a6+12) << 16) | cpu_readmem24bedw_word(a6+14);
			logerror("Known Protection @ 3D8F4(%08X): R@%06X ", p1, offset);
			break;
		case 0x3d8fa:
		case 0x3d90e:
			logerror("R@%06X ", offset);
			break;

		/* protection code from 437fa - 43860 */
		case 0x43814:
			a6 = activecpu_get_reg(M68K_A6);
			p1 = cpu_readmem24bedw(a6+15);
			logerror("Known Protection @ 43814(%08X): R@%06X ", p1, offset);
			break;
		case 0x4381c:
		case 0x43840:
			logerror("R@%06X ", offset);
			break;
		case 0x43848:
			logerror("R@%06X - done\n", offset);
			break;

		/* catch anything else */
		default:
			logerror("%06X:Unknown protection R@%06X\n", activecpu_get_previouspc(), offset);
			break;
	}
}
#endif

	/* handle specific reads */
	switch (offset)
	{
		/* status register; the code spins on this waiting for the high bit to be set */
		case 0xdc4700:
//			if (protmode != 0)
			{
				*data = 0x8000;
			}
			break;

		/* some kind of result register */
		case 0xdcc7c2:
			if (protmode == 2)
			{
				*data = protresult;
				protmode = 0;
//				logerror("prot:Clearing mode 2\n");
			}
			break;

		case 0xdcc7c4:
			if (protmode == 1)
			{
				protmode = 0;
//				logerror("prot:Clearing mode 1\n");
			}
			break;
	}
}



/*************************************
 *
 *	Protection/color RAM
 *
 *************************************/

static READ32_HANDLER( colorram_protection_r )
{
	offs_t address = 0xd80000 + offset * 4;
	data32_t result32 = 0;
	data16_t result;

	if ((mem_mask & 0xffff0000) != 0xffff0000)
	{
		result = atarigt_colorram_r(address);
		(*protection_r)(address, &result);
		result32 |= result << 16;
	}
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
	{
		result = atarigt_colorram_r(address + 2);
		(*protection_r)(address + 2, &result);
		result32 |= result;
	}

	return result32;
}


static WRITE32_HANDLER( colorram_protection_w )
{
	offs_t address = 0xd80000 + offset * 4;

	if ((mem_mask & 0xffff0000) != 0xffff0000)
	{
		atarigt_colorram_w(address, data >> 16, mem_mask >> 16);
		(*protection_w)(address, data >> 16);
	}
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
	{
		atarigt_colorram_w(address + 2, data, mem_mask);
		(*protection_w)(address + 2, data);
	}
}



/*************************************
 *
 *	Primal Rage speedups
 *
 *************************************/

static data32_t *speedup;

static READ32_HANDLER( primrage_speedup_r )
{
	if (activecpu_get_previouspc() == 0x10b0e && ((*speedup >> 8) & 0xff) == 0)
		cpu_spinuntil_int();
	return *speedup;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ32_START( readmem )
	{ 0x000000, 0x1fffff, MRA32_ROM },
	{ 0xc00000, 0xc00003, sound_data_r },
	{ 0xd20000, 0xd20fff, atarigen_eeprom_upper32_r },
	{ 0xd70000, 0xd7ffff, MRA32_RAM },
	{ 0xd80000, 0xdfffff, colorram_protection_r },
	{ 0xe80000, 0xe80003, inputs_01_r },
	{ 0xe82000, 0xe82003, special_port2_r },
	{ 0xe82004, 0xe82007, special_port3_r },
	{ 0xf80000, 0xffffff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( writemem )
	{ 0x000000, 0x1fffff, MWA32_ROM },
	{ 0xc00000, 0xc00003, sound_data_w },
	{ 0xd20000, 0xd20fff, atarigen_eeprom32_w, (data32_t **)&atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xd40000, 0xd4ffff, atarigen_eeprom_enable32_w },
	{ 0xd70000, 0xd71fff, MWA32_RAM },
	{ 0xd72000, 0xd75fff, atarigen_playfield32_w, &atarigen_playfield32 },
	{ 0xd76000, 0xd76fff, atarigen_alpha32_w, &atarigen_alpha32 },
	{ 0xd77000, 0xd77fff, MWA32_RAM },
	{ 0xd78000, 0xd78fff, atarirle_0_spriteram32_w, &atarirle_0_spriteram32 },
	{ 0xd79000, 0xd7ffff, MWA32_RAM },
	{ 0xd80000, 0xdfffff, colorram_protection_w, (data32_t **)&atarigt_colorram },
	{ 0xe04000, 0xe04003, led_w },
	{ 0xe08000, 0xe08003, latch_w },
	{ 0xe0a000, 0xe0a003, atarigen_scanline_int_ack32_w },
	{ 0xe0c000, 0xe0c003, atarigen_video_int_ack32_w },
	{ 0xe0e000, 0xe0e003, MWA32_NOP },//watchdog_reset_w },
	{ 0xf80000, 0xffffff, MWA32_RAM },
MEMORY_END


#if 0
static struct MemoryReadAddress readmem_cage[] =
{
	{ 0x000000, 0x680000, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem_cage[] =
{
	{ 0x000000, 0x680000, MWA_ROM },
	{ -1 }  /* end of table */
};
#endif



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( tmek )
	PORT_START		/* 68.SW (A1=0) */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 68.SW (A1=1) */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START      /* 68.STATUS (A2=0) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /A2DRDY */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_TILT )		/* TILT */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ23 */
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )	/* A2D.EOC */
	PORT_BIT( 0x0030, IP_ACTIVE_LOW, IPT_UNUSED )	/* NC */
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )			/* SELFTEST */
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 68.STATUS (A2=1) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /VBIRQ */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )	/* /4MSIRQ */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ0 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ1 */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SERVICER */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SER.L */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )	/* COINR */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )	/* COINL */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( primrage )
	PORT_START		/* 68.SW (A1=0) */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 68.SW (A1=1) */
// bit 0x0008 does something
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START      /* 68.STATUS (A2=0) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /A2DRDY */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_TILT )		/* TILT */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ23 */
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )	/* A2D.EOC */
	PORT_BIT( 0x0030, IP_ACTIVE_LOW, IPT_UNUSED )	/* NC */
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )			/* SELFTEST */
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 68.STATUS (A2=1) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /VBIRQ */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )	/* /4MSIRQ */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ0 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ1 */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SERVICER */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SER.L */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )	/* COINR */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )	/* COINL */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,3),
	5,
	{ 0, 0, 1, 2, 3 },
	{ RGN_FRAC(1,3)+0, RGN_FRAC(1,3)+4, 0, 4, RGN_FRAC(1,3)+8, RGN_FRAC(1,3)+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};


static struct GfxLayout pftoplayout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(2,3)+0, RGN_FRAC(2,3)+4, 0, 0, 0, 0 },
	{ 3, 2, 1, 0, 11, 10, 9, 8 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};


static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout, 0x000, 64 },
	{ REGION_GFX2, 0, &anlayout, 0x000, 16 },
	{ REGION_GFX1, 0, &pftoplayout, 0x000, 64 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( atarigt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, ATARI_CLOCK_50MHz/2)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	MDRV_CPU_PERIODIC_INT(atarigen_scanline_int_gen,250)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(atarigt)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(atarigt)
	MDRV_VIDEO_EOF(atarirle)
	MDRV_VIDEO_UPDATE(atarigt)

	/* sound hardware */
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( tmek )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "0044d", 0x00000, 0x20000, 0x1cd62725 )
	ROM_LOAD32_BYTE( "0043d", 0x00001, 0x20000, 0x82185051 )
	ROM_LOAD32_BYTE( "0042d", 0x00002, 0x20000, 0xef9feda4 )
	ROM_LOAD32_BYTE( "0041d", 0x00003, 0x20000, 0x179da056 )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "0078c", 0x000000, 0x080000, 0xff5b979a )
	ROM_LOAD( "0074",  0x080000, 0x200000, 0x8dfc6ce0 )
	ROM_LOAD( "0076",  0x280000, 0x200000, 0x74dffe2d )
	ROM_LOAD( "0077",  0x480000, 0x200000, 0x8f650f8b )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0250",  0x000000, 0x80000, 0x56bd9f25 ) /* playfield, planes 0-1 */
	ROM_LOAD( "0253a", 0x080000, 0x80000, 0x23e2f83d )
	ROM_LOAD( "0251",  0x100000, 0x80000, 0x0d3b08f7 ) /* playfield, planes 2-3 */
	ROM_LOAD( "0254a", 0x180000, 0x80000, 0x448aea87 )
	ROM_LOAD( "0252",  0x200000, 0x80000, 0x95a1c23b ) /* playfield, planes 4-5 */
	ROM_LOAD( "0255a", 0x280000, 0x80000, 0xf0fbb700 )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "0045a", 0x000000, 0x20000, 0x057a5304 ) /* alphanumerics */

	ROM_REGION16_BE( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0300", 0x000001, 0x100000, 0x8367ddac )
	ROM_LOAD16_BYTE( "0301", 0x000000, 0x100000, 0x94524b5b )
	ROM_LOAD16_BYTE( "0302", 0x200001, 0x100000, 0xc03f1aa7 )
	ROM_LOAD16_BYTE( "0303", 0x200000, 0x100000, 0x3ac5b24f )
	ROM_LOAD16_BYTE( "0304", 0x400001, 0x100000, 0xb053ef78 )
	ROM_LOAD16_BYTE( "0305", 0x400000, 0x100000, 0xb012b8e9 )
	ROM_LOAD16_BYTE( "0306", 0x600001, 0x100000, 0xd086f149 )
	ROM_LOAD16_BYTE( "0307", 0x600000, 0x100000, 0x49c1a541 )
	ROM_LOAD16_BYTE( "0308", 0x800001, 0x100000, 0x97033c8a )
	ROM_LOAD16_BYTE( "0309", 0x800000, 0x100000, 0xe095ecb3 )
	ROM_LOAD16_BYTE( "0310", 0xa00001, 0x100000, 0xe056a0c3 )
	ROM_LOAD16_BYTE( "0311", 0xa00000, 0x100000, 0x05afb2dc )
	ROM_LOAD16_BYTE( "0312", 0xc00001, 0x100000, 0xcc224dae )
	ROM_LOAD16_BYTE( "0313", 0xc00000, 0x100000, 0xa8cf049d )
	ROM_LOAD16_BYTE( "0314", 0xe00001, 0x100000, 0x4f01db8d )
	ROM_LOAD16_BYTE( "0315", 0xe00000, 0x100000, 0x28e97d06 )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, 0xa70ade3f )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, 0xf4768b4d )
	ROM_LOAD( "0001c",  0x0400, 0x0200, 0x22a76ad4 )
ROM_END


ROM_START( tmekprot )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "pgm0", 0x00000, 0x20000, 0xf5f7f7be )
	ROM_LOAD32_BYTE( "pgm1", 0x00001, 0x20000, 0x284f7971 )
	ROM_LOAD32_BYTE( "pgm2", 0x00002, 0x20000, 0xce9a77d4 )
	ROM_LOAD32_BYTE( "pgm3", 0x00003, 0x20000, 0x28b0e210 )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "0078c", 0x000000, 0x080000, 0xff5b979a )
	ROM_LOAD( "0074",  0x080000, 0x200000, 0x8dfc6ce0 )
	ROM_LOAD( "0076",  0x280000, 0x200000, 0x74dffe2d )
	ROM_LOAD( "0077",  0x480000, 0x200000, 0x8f650f8b )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0250",  0x000000, 0x80000, 0x56bd9f25 ) /* playfield, planes 0-1 */
	ROM_LOAD( "0253a", 0x080000, 0x80000, 0x23e2f83d )
	ROM_LOAD( "0251",  0x100000, 0x80000, 0x0d3b08f7 ) /* playfield, planes 2-3 */
	ROM_LOAD( "0254a", 0x180000, 0x80000, 0x448aea87 )
	ROM_LOAD( "0252",  0x200000, 0x80000, 0x95a1c23b ) /* playfield, planes 4-5 */
	ROM_LOAD( "0255a", 0x280000, 0x80000, 0xf0fbb700 )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "alpha", 0x000000, 0x20000, 0x8f57a604 ) /* alphanumerics */

	ROM_REGION16_BE( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0300", 0x000001, 0x100000, 0x8367ddac )
	ROM_LOAD16_BYTE( "0301", 0x000000, 0x100000, 0x94524b5b )
	ROM_LOAD16_BYTE( "0302", 0x200001, 0x100000, 0xc03f1aa7 )
	ROM_LOAD16_BYTE( "0303", 0x200000, 0x100000, 0x3ac5b24f )
	ROM_LOAD16_BYTE( "0304", 0x400001, 0x100000, 0xb053ef78 )
	ROM_LOAD16_BYTE( "0305", 0x400000, 0x100000, 0xb012b8e9 )
	ROM_LOAD16_BYTE( "0306", 0x600001, 0x100000, 0xd086f149 )
	ROM_LOAD16_BYTE( "0307", 0x600000, 0x100000, 0x49c1a541 )
	ROM_LOAD16_BYTE( "0308", 0x800001, 0x100000, 0x97033c8a )
	ROM_LOAD16_BYTE( "0309", 0x800000, 0x100000, 0xe095ecb3 )
	ROM_LOAD16_BYTE( "0310", 0xa00001, 0x100000, 0xe056a0c3 )
	ROM_LOAD16_BYTE( "0311", 0xa00000, 0x100000, 0x05afb2dc )
	ROM_LOAD16_BYTE( "0312", 0xc00001, 0x100000, 0xcc224dae )
	ROM_LOAD16_BYTE( "0313", 0xc00000, 0x100000, 0xa8cf049d )
	ROM_LOAD16_BYTE( "0314", 0xe00001, 0x100000, 0x4f01db8d )
	ROM_LOAD16_BYTE( "0315", 0xe00000, 0x100000, 0x28e97d06 )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, 0xa70ade3f )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, 0xf4768b4d )
	ROM_LOAD( "0001c",  0x0400, 0x0200, 0x22a76ad4 )
ROM_END


ROM_START( primrage )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "1044b", 0x000000, 0x80000, 0x35c9c34b )
	ROM_LOAD32_BYTE( "1043b", 0x000001, 0x80000, 0x86322829 )
	ROM_LOAD32_BYTE( "1042b", 0x000002, 0x80000, 0x750e8095 )
	ROM_LOAD32_BYTE( "1041b", 0x000003, 0x80000, 0x6a90d283 )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "1078a", 0x000000, 0x080000, 0x0656435f )
	ROM_LOAD( "0075",  0x080000, 0x200000, 0xb685a88e )
	ROM_LOAD( "0077",  0x480000, 0x200000, 0x3283cea8 )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0050a",  0x000000, 0x80000, 0x66896e8f ) /* playfield, planes 0-1 */
	ROM_LOAD( "0051a",  0x100000, 0x80000, 0xfb5b3e7b ) /* playfield, planes 2-3 */
	ROM_LOAD( "0052a",  0x200000, 0x80000, 0xcbe38670 ) /* playfield, planes 4-5 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1045b", 0x000000, 0x20000, 0x1d3260bf ) /* alphanumerics */

	ROM_REGION16_BE( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "1100a", 0x0000001, 0x080000, 0x6e9c80b5 )
	ROM_LOAD16_BYTE( "1101a", 0x0000000, 0x080000, 0xbb7ee624 )
	ROM_LOAD16_BYTE( "0332",  0x0800001, 0x100000, 0x610cfcb4 )
	ROM_LOAD16_BYTE( "0333",  0x0800000, 0x100000, 0x3320448e )
	ROM_LOAD16_BYTE( "0334",  0x0a00001, 0x100000, 0xbe3acb6f )
	ROM_LOAD16_BYTE( "0335",  0x0a00000, 0x100000, 0xe4f6e87a )
	ROM_LOAD16_BYTE( "0336",  0x0c00001, 0x100000, 0xa78a8525 )
	ROM_LOAD16_BYTE( "0337",  0x0c00000, 0x100000, 0x73fdd050 )
	ROM_LOAD16_BYTE( "0338",  0x0e00001, 0x100000, 0xfa19cae6 )
	ROM_LOAD16_BYTE( "0339",  0x0e00000, 0x100000, 0xe0cd1393 )
	ROM_LOAD16_BYTE( "0316",  0x1000001, 0x100000, 0x9301c672 )
	ROM_LOAD16_BYTE( "0317",  0x1000000, 0x100000, 0x9e3b831a )
	ROM_LOAD16_BYTE( "0318",  0x1200001, 0x100000, 0x8523db5d )
	ROM_LOAD16_BYTE( "0319",  0x1200000, 0x100000, 0x42f22e4b )
	ROM_LOAD16_BYTE( "0320",  0x1400001, 0x100000, 0x21369d13 )
	ROM_LOAD16_BYTE( "0321",  0x1400000, 0x100000, 0x3b7d498a )
	ROM_LOAD16_BYTE( "0322",  0x1600001, 0x100000, 0x05e9f407 )
	ROM_LOAD16_BYTE( "0323",  0x1600000, 0x100000, 0x603f3bb6 )
	ROM_LOAD16_BYTE( "0324",  0x1800001, 0x100000, 0x3c37769f )
	ROM_LOAD16_BYTE( "0325",  0x1800000, 0x100000, 0xf43321e3 )
	ROM_LOAD16_BYTE( "0326",  0x1a00001, 0x100000, 0x63d4ccea )
	ROM_LOAD16_BYTE( "0327",  0x1a00000, 0x100000, 0x9f4806d5 )
	ROM_LOAD16_BYTE( "0328",  0x1c00001, 0x100000, 0xa08d73e1 )
	ROM_LOAD16_BYTE( "0329",  0x1c00000, 0x100000, 0xeff3d2cd )
	ROM_LOAD16_BYTE( "0330",  0x1e00001, 0x100000, 0x7bf6bb8f )
	ROM_LOAD16_BYTE( "0331",  0x1e00000, 0x100000, 0xc6a64dad )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, 0xa70ade3f )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, 0xf4768b4d )
	ROM_LOAD( "0001c",  0x0400, 0x0200, 0x22a76ad4 )
ROM_END


ROM_START( primrag2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "0044b", 0x000000, 0x80000, 0x26139575 )
	ROM_LOAD32_BYTE( "0043b", 0x000001, 0x80000, 0x928d2447 )
	ROM_LOAD32_BYTE( "0042b", 0x000002, 0x80000, 0xcd6062b9 )
	ROM_LOAD32_BYTE( "0041b", 0x000003, 0x80000, 0x3008f6f0 )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "0078a", 0x000000, 0x080000, 0x91df8d8f )
	ROM_LOAD( "0075",  0x080000, 0x200000, 0xb685a88e )
	ROM_LOAD( "0077",  0x480000, 0x200000, 0x3283cea8 )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0050a",  0x000000, 0x80000, 0x66896e8f ) /* playfield, planes 0-1 */
	ROM_LOAD( "0051a",  0x100000, 0x80000, 0xfb5b3e7b ) /* playfield, planes 2-3 */
	ROM_LOAD( "0052a",  0x200000, 0x80000, 0xcbe38670 ) /* playfield, planes 4-5 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "0045a", 0x000000, 0x20000, 0xc8b39b1c ) /* alphanumerics */

	ROM_REGION16_BE( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0100a", 0x0000001, 0x080000, 0x5299fb2a )
	ROM_LOAD16_BYTE( "0101a", 0x0000000, 0x080000, 0x3e234711 )
	ROM_LOAD16_BYTE( "0332",  0x0800001, 0x100000, 0x610cfcb4 )
	ROM_LOAD16_BYTE( "0333",  0x0800000, 0x100000, 0x3320448e )
	ROM_LOAD16_BYTE( "0334",  0x0a00001, 0x100000, 0xbe3acb6f )
	ROM_LOAD16_BYTE( "0335",  0x0a00000, 0x100000, 0xe4f6e87a )
	ROM_LOAD16_BYTE( "0336",  0x0c00001, 0x100000, 0xa78a8525 )
	ROM_LOAD16_BYTE( "0337",  0x0c00000, 0x100000, 0x73fdd050 )
	ROM_LOAD16_BYTE( "0338",  0x0e00001, 0x100000, 0xfa19cae6 )
	ROM_LOAD16_BYTE( "0339",  0x0e00000, 0x100000, 0xe0cd1393 )
	ROM_LOAD16_BYTE( "0316",  0x1000001, 0x100000, 0x9301c672 )
	ROM_LOAD16_BYTE( "0317",  0x1000000, 0x100000, 0x9e3b831a )
	ROM_LOAD16_BYTE( "0318",  0x1200001, 0x100000, 0x8523db5d )
	ROM_LOAD16_BYTE( "0319",  0x1200000, 0x100000, 0x42f22e4b )
	ROM_LOAD16_BYTE( "0320",  0x1400001, 0x100000, 0x21369d13 )
	ROM_LOAD16_BYTE( "0321",  0x1400000, 0x100000, 0x3b7d498a )
	ROM_LOAD16_BYTE( "0322",  0x1600001, 0x100000, 0x05e9f407 )
	ROM_LOAD16_BYTE( "0323",  0x1600000, 0x100000, 0x603f3bb6 )
	ROM_LOAD16_BYTE( "0324",  0x1800001, 0x100000, 0x3c37769f )
	ROM_LOAD16_BYTE( "0325",  0x1800000, 0x100000, 0xf43321e3 )
	ROM_LOAD16_BYTE( "0326",  0x1a00001, 0x100000, 0x63d4ccea )
	ROM_LOAD16_BYTE( "0327",  0x1a00000, 0x100000, 0x9f4806d5 )
	ROM_LOAD16_BYTE( "0328",  0x1c00001, 0x100000, 0xa08d73e1 )
	ROM_LOAD16_BYTE( "0329",  0x1c00000, 0x100000, 0xeff3d2cd )
	ROM_LOAD16_BYTE( "0330",  0x1e00001, 0x100000, 0x7bf6bb8f )
	ROM_LOAD16_BYTE( "0331",  0x1e00000, 0x100000, 0xc6a64dad )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, 0xa70ade3f )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, 0xf4768b4d )
	ROM_LOAD( "0001c",  0x0400, 0x0200, 0x22a76ad4 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( tmek )
{
	atarigen_eeprom_default = NULL;
	atarigt_motion_object_mask = 0xff0;

	/* setup protection */
	protection_r = tmek_protection_r;
	protection_w = tmek_protection_w;
}


static DRIVER_INIT( primrage )
{
	atarigen_eeprom_default = NULL;
	atarigt_motion_object_mask = 0x7f0;

	/* install protection */
	protection_r = primrage_protection_r;
	protection_w = primrage_protection_w;

	/* intall speedups */
	speedup = install_mem_read32_handler(0, 0xffcde8, 0xffcdeb, primrage_speedup_r);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1994, tmek,     0,        atarigt,  tmek,     tmek,     ROT0, "Atari Games", "T-MEK", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1994, tmekprot, tmek,     atarigt,  tmek,     tmek,     ROT0, "Atari Games", "T-MEK (prototype)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1994, primrage, 0,        atarigt,  primrage, primrage, ROT0, "Atari Games", "Primal Rage (version 2.3)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1994, primrag2, primrage, atarigt,  primrage, primrage, ROT0, "Atari Games", "Primal Rage (version 1.7)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
