/***************************************************************************

	Driver for Atari polygon racer games

	This collection of games uses many CPUs and many boards in many
	different combinations. There are 3 different main boards:

		- the "driver" board (A045988) is the original Hard Drivin' PCB
			- Hard Drivin'
			- Race Drivin' Upgrade

		- the "multisync" board (A046901)
			- STUN Runner
			- Steel Talons
			- Hard Drivin' Compact
			- Race Drivin' Compact

		- the "multisync II" board (A049852)
			- Hard Drivin's Airborne

	To first order, all of the above boards had the same basic features:

		a 68010 @ 8MHz to drive the whole game
		a TMS34010 @ 48MHz (GSP) to render the polygons and graphics
		a TMS34012 @ 50MHz (PSP, labelled SCX6218UTP) to expand pixels
		a TMS34010 @ 50MHz (MSP, optional) to handle in-game calculations

	The original "driver" board had 1MB of VRAM. The "multisync" board
	reduced that to 512k. The "multisync II" board went back to a full
	MB again.

	Stacked on top of the main board were two or more additional boards
	that were accessible through an expansion bus. Each game had at least
	an ADSP board and a sound board. Later games had additional boards for
	extra horsepower or for communications between multiple players.

	-----------------------------------------------------------------------

	The ADSP board is usually the board stacked closest to the main board.
	It also comes in four varieties, though these do not match
	one-for-one with the main boards listed above. They are:

		- the "ADSP" board (A044420)
			- early Hard Drivin' revisions

		- the "ADSP II" board (A047046)
			- later Hard Drivin'
			- STUN Runner
			- Hard Drivin' Compact
			- Race Drivin' Upgrade
			- Race Drivin' Compact

		- the "DS III" board (A049096)
			- Steel Talons

		- the "DS IV" board (A051973)
			- Hard Drivin's Airborne

	These boards are the workhorses of the game. They contain a single
	8MHz ADSP-2100 (ADSP and ADSP II) or 12MHz ADSP-2101 (DS III and DS IV)
	chip that is responsible for all the polygon transformations, lighting,
	and slope computations. Along with the DSP, there are several high-speed
	serial-access ROMs and RAMs.

	The "ADSP II" board is nearly identical to the original "ADSP" board
	except that is has space for extra serial ROM data. The "DS III" is
	an advanced design that contains space for a bunch of complex sound
	circuitry that appears to have never been used. The "DS IV" looks to
	have the same board layout as the "DS III", but the sound circuitry
	is actually populated.

	-----------------------------------------------------------------------

	Three sound boards were used:

		- the "driver sound" board (A046491)
			- Hard Drivin'
			- Hard Drivin' Compact
			- Race Drivin' Upgrade
			- Race Drivin' Compact

		- the "JSA II" board
			- STUN Runner

		- the "JSA IIIS" board
			- Steel Talons

	The "driver sound" board runs with a 68000 master and a TMS32010 slave
	driving a DAC. The "JSA" boards are both standard Atari sound boards
	with a 6502 driving a YM2151 and an OKI6295 ADPCM chip. Hard Drivin's
	Airborne uses the "DS IV" board for its sound.

	-----------------------------------------------------------------------

	In addition, there were a number of supplemental boards that were
	included with certain games:

		- the "DSK" board (A047724)
			- Race Drivin' Upgrade
			- Race Drivin' Compact

		- the "DSPCOM" board (A049349)
			- Steel Talons

		- the "DSK II" board (A051028)
			- Hard Drivin' Airborne

	-----------------------------------------------------------------------

	There are a total of 8 known games (plus variants) on this hardware:

	Hard Drivin' Cockpit
		- "driver" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP" or "ADSP II" board (8MHz ADSP-2100)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	Hard Drivin' Compact
		- "multisync" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	S.T.U.N. Runner
		- "multisync" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "JSA II" sound board (1.7MHz 6502, YM2151, OKI6295)

	Race Drivin' Cockpit
		- "driver" board (8MHz 68010, 50MHz TMS34010, 50MHz TMS34012)
		- "ADSP" or "ADSP II" board (8MHz ADSP-2100)
		- "DSK" board (40MHz DSP32C, 20MHz TMS32015)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	Race Drivin' Compact
		- "multisync" board (8MHz 68010, 50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "DSK" board (40MHz DSP32C, 20MHz TMS32015)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	Steel Talons
		- "multisync" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "DS III" board (12MHz ADSP-2101)
		- "JSA IIIS" sound board (1.7MHz 6502, YM2151, OKI6295)
		- "DSPCOM" I/O board (10MHz ADSP-2105)

	Hard Drivin's Airborne (prototype)
		- "multisync ii" main board (8MHz 68010, 50MHz TMS34010, 50MHz TMS34012)
		- "DS IV" board (12MHz ADSP-2101, plus 2x10MHz ADSP-2105s for sound)
		- "DSK II" board (40MHz DSP32C, 20MHz TMS32015)

	BMX Heat (prototype)
		- unknown boards ???

	Police Trainer (prototype)
		- unknown boards ???

	Metal Maniax (prototype)
		- reworked hardware that is similar but not of the same layout

****************************************************************************/


#include "driver.h"
#include "sound/adpcm.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms32010/tms32010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "cpu/dsp32/dsp32.h"
#include "machine/atarigen.h"
#include "machine/asic65.h"
#include "sndhrdw/atarijsa.h"
#include "harddriv.h"

/* from slapstic.c */
void slapstic_init(int chip);



/*************************************
 *
 *	CPU configs
 *
 *************************************/

static struct tms34010_config gsp_config =
{
	1,								/* halt on reset */
	hdgsp_irq_gen,					/* generate interrupt */
	hdgsp_write_to_shiftreg,		/* write to shiftreg function */
	hdgsp_read_from_shiftreg,		/* read from shiftreg function */
	hdgsp_display_update			/* display offset update function */
};


static struct tms34010_config msp_config =
{
	1,								/* halt on reset */
	hdmsp_irq_gen,					/* generate interrupt */
	NULL,							/* write to shiftreg function */
	NULL,							/* read from shiftreg function */
	NULL							/* display offset update function */
};


static struct dsp32_config dsp32c_config =
{
	hddsk_update_pif				/* a change has occurred on an output pin */
};



/*************************************
 *
 *	Driver board memory maps
 *
 *************************************/

static MEMORY_READ16_START( driver_readmem_68k )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x600000, 0x603fff, hd68k_port0_r },
	{ 0xa80000, 0xafffff, input_port_1_word_r },
	{ 0xb00000, 0xb7ffff, hd68k_adc8_r },
	{ 0xb80000, 0xbfffff, hd68k_adc12_r },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_r },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_r },
	{ 0xff0000, 0xff001f, hd68k_duart_r },
	{ 0xff4000, 0xff4fff, hd68k_zram_r },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driver_writemem_68k )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x604000, 0x607fff, hd68k_nwr_w },
	{ 0x608000, 0x60bfff, watchdog_reset16_w },
	{ 0x60c000, 0x60ffff, hd68k_irq_ack_w },
	{ 0xa00000, 0xa7ffff, hd68k_wr0_write },
	{ 0xa80000, 0xafffff, hd68k_wr1_write },
	{ 0xb00000, 0xb7ffff, hd68k_wr2_write },
	{ 0xb80000, 0xbfffff, hd68k_adc_control_w },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_w },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_w },
	{ 0xff0000, 0xff001f, hd68k_duart_w },
	{ 0xff4000, 0xff4fff, hd68k_zram_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xff8000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( driver_readmem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA16_BANK1 },
MEMORY_END


static MEMORY_WRITE16_START( driver_writemem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MWA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_1bpp_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MWA16_BANK1, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END


static MEMORY_READ16_START( driver_readmem_msp )
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), MRA16_BANK2 },
	{ TOBYTE(0x00700000), TOBYTE(0x007fffff), MRA16_BANK3 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xfff00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driver_writemem_msp )
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), MWA16_BANK2 },
	{ TOBYTE(0x00700000), TOBYTE(0x007fffff), MWA16_BANK3 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_w },
	{ TOBYTE(0xfff00000), TOBYTE(0xffffffff), MWA16_RAM, &hdmsp_ram },
MEMORY_END



/*************************************
 *
 *	Multisync board memory maps
 *
 *************************************/

static MEMORY_READ16_START( multisync_readmem_68k )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x600000, 0x603fff, atarigen_sound_upper_r },
	{ 0x604000, 0x607fff, hd68k_sound_reset_r },
	{ 0x60c000, 0x60ffff, hd68k_port0_r },
	{ 0xa80000, 0xafffff, input_port_1_word_r },
	{ 0xb00000, 0xb7ffff, hd68k_adc8_r },
	{ 0xb80000, 0xbfffff, hd68k_adc12_r },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_r },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_r },
	{ 0xff0000, 0xff001f, hd68k_duart_r },
	{ 0xff4000, 0xff4fff, hd68k_zram_r },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( multisync_writemem_68k )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x600000, 0x603fff, atarigen_sound_upper_w },
	{ 0x604000, 0x607fff, hd68k_nwr_w },
	{ 0x608000, 0x60bfff, watchdog_reset16_w },
	{ 0x60c000, 0x60ffff, hd68k_irq_ack_w },
	{ 0xa00000, 0xa7ffff, hd68k_wr0_write },
	{ 0xa80000, 0xafffff, hd68k_wr1_write },
	{ 0xb00000, 0xb7ffff, hd68k_wr2_write },
	{ 0xb80000, 0xbfffff, hd68k_adc_control_w },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_w },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_w },
	{ 0xff0000, 0xff001f, hd68k_duart_w },
	{ 0xff4000, 0xff4fff, hd68k_zram_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xff8000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( multisync_readmem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffbfffff), MRA16_BANK1 },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( multisync_writemem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x00afffff), MWA16_NOP },	/* hit during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffbfffff), MWA16_BANK1 },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MWA16_RAM, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END


/* MSP is identical to original driver */
#define multisync_readmem_msp driver_readmem_msp
#define multisync_writemem_msp driver_writemem_msp



/*************************************
 *
 *	Multisync II board memory maps
 *
 *************************************/

static MEMORY_READ16_START( multisync2_readmem_68k )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x60c000, 0x60ffff, hd68k_port0_r },
	{ 0xa80000, 0xafffff, input_port_1_word_r },
	{ 0xb00000, 0xb7ffff, hd68k_adc8_r },
	{ 0xb80000, 0xbfffff, hd68k_adc12_r },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_r },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_r },
	{ 0xfc0000, 0xfc001f, hd68k_duart_r },
	{ 0xfd0000, 0xfd0fff, hd68k_zram_r },
	{ 0xfd4000, 0xfd4fff, hd68k_zram_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( multisync2_writemem_68k )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x604000, 0x607fff, hd68k_nwr_w },
	{ 0x608000, 0x60bfff, watchdog_reset16_w },
	{ 0x60c000, 0x60ffff, hd68k_irq_ack_w },
	{ 0xa00000, 0xa7ffff, hd68k_wr0_write },
	{ 0xa80000, 0xafffff, hd68k_wr1_write },
	{ 0xb00000, 0xb7ffff, hd68k_wr2_write },
	{ 0xb80000, 0xbfffff, hd68k_adc_control_w },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_w },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_w },
	{ 0xfc0000, 0xfc001f, hd68k_duart_w },
	{ 0xfd0000, 0xfd0fff, hd68k_zram_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END


/* GSP is identical to original multisync */
static MEMORY_READ16_START( multisync2_readmem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA16_BANK1 },
MEMORY_END


static MEMORY_WRITE16_START( multisync2_writemem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x00afffff), MWA16_NOP },	/* hit during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MWA16_BANK1, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END



/*************************************
 *
 *	ADSP/ADSP II board memory maps
 *
 *************************************/

static MEMORY_READ16_START( adsp_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), hdadsp_special_r },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( adsp_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), hdadsp_special_w },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	DS III/IV board memory maps
 *
 *************************************/

static MEMORY_READ16_START( ds3_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MRA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_r },	/* adsp control regs */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x3fff), hdds3_special_r },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( ds3_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MWA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_w },	/* adsp control regs */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x3fff), hdds3_special_w },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( ds3snd_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MRA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_r },	/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MRA16_RAM },
//
//	/SIRQ2 = IRQ2
//	/SRES -> RESET
//
//	2xx0 W = SWR0 (POUT)
//	2xx1 W = SWR1 (SINT)
//	2xx2 W = SWR2 (TFLAG)
//	2xx3 W = SWR3 (INTSRC)
//	2xx4 W = DACL
//	2xx5 W = DACR
//	2xx6 W = SRMADL
//	2xx7 W = SRMADH
//
//	2xx0 R = SRD0 (PIN)
//	2xx1 R = SRD1 (RSAT)
//	2xx4 R = SROM
//	2xx7 R = SFWCLR
//
//
//	/XRES -> RESET
//	communicate over serial I/O

MEMORY_END


static MEMORY_WRITE16_START( ds3snd_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MWA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_w },	/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	DSK board memory maps
 *
 *************************************/

static MEMORY_READ32_START( dsk_readmem_dsp32 )
	{ 0x000000, 0x001fff, MRA32_RAM },
	{ 0x600000, 0x63ffff, MRA32_RAM },
	{ 0xfff800, 0xffffff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( dsk_writemem_dsp32 )
	{ 0x000000, 0x001fff, MWA32_RAM },
	{ 0x600000, 0x63ffff, MWA32_RAM },
	{ 0xfff800, 0xffffff, MWA32_RAM },
MEMORY_END



/*************************************
 *
 *	DSK II board memory maps
 *
 *************************************/

static MEMORY_READ32_START( dsk2_readmem_dsp32 )
	{ 0x000000, 0x001fff, MRA32_RAM },
	{ 0x200000, 0x23ffff, MRA32_RAM },
	{ 0x400000, 0x5fffff, MRA32_BANK4 },
	{ 0xfff800, 0xffffff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( dsk2_writemem_dsp32 )
	{ 0x000000, 0x001fff, MWA32_RAM },
	{ 0x200000, 0x23ffff, MWA32_RAM },
	{ 0x400000, 0x5fffff, MWA32_ROM },
	{ 0xfff800, 0xffffff, MWA32_RAM },
MEMORY_END



/*************************************
 *
 *	Driver sound board memory maps
 *
 *************************************/

static MEMORY_READ16_START( driversnd_readmem_68k )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0xff0000, 0xff0fff, hdsnd68k_data_r },
	{ 0xff1000, 0xff1fff, hdsnd68k_switches_r },
	{ 0xff2000, 0xff2fff, hdsnd68k_320port_r },
	{ 0xff3000, 0xff3fff, hdsnd68k_status_r },
	{ 0xff4000, 0xff5fff, hdsnd68k_320ram_r },
	{ 0xff6000, 0xff7fff, hdsnd68k_320ports_r },
	{ 0xff8000, 0xffbfff, hdsnd68k_320com_r },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driversnd_writemem_68k )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0xff0000, 0xff0fff, hdsnd68k_data_w },
	{ 0xff1000, 0xff1fff, hdsnd68k_latches_w },
	{ 0xff2000, 0xff2fff, hdsnd68k_speech_w },
	{ 0xff3000, 0xff3fff, hdsnd68k_irqclr_w },
	{ 0xff4000, 0xff5fff, hdsnd68k_320ram_w },
	{ 0xff6000, 0xff7fff, hdsnd68k_320ports_w },
	{ 0xff8000, 0xffbfff, hdsnd68k_320com_w },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( driversnd_readmem_dsp )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x0ff), MRA16_RAM },
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0xfff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driversnd_writemem_dsp )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x0ff), MWA16_RAM },
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0xfff), MWA16_RAM, &hdsnddsp_ram },
MEMORY_END


static PORT_READ16_START( driversnd_readport_dsp )
	{ TMS32010_PORT_RANGE(0, 0), hdsnddsp_rom_r },
	{ TMS32010_PORT_RANGE(1, 1), hdsnddsp_comram_r },
	{ TMS32010_PORT_RANGE(2, 2), hdsnddsp_compare_r },
	{ TMS32010_BIO, TMS32010_BIO, hdsnddsp_get_bio },
PORT_END


static PORT_WRITE16_START( driversnd_writeport_dsp )
	{ TMS32010_PORT_RANGE(0, 0), hdsnddsp_dac_w },
	{ TMS32010_PORT_RANGE(1, 2), MWA16_NOP },
	{ TMS32010_PORT_RANGE(3, 3), hdsnddsp_comport_w },
	{ TMS32010_PORT_RANGE(4, 4), hdsnddsp_mute_w },
	{ TMS32010_PORT_RANGE(5, 5), hdsnddsp_gen68kirq_w },
	{ TMS32010_PORT_RANGE(6, 7), hdsnddsp_soundaddr_w },
PORT_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( harddriv )
	PORT_START		/* 600000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* diagnostic switch */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* key */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0xfff8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 - clutch pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 25, 100, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 2 - seat */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 3 - shifter lever Y */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 4 - shifter lever X*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 5 - wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b00000 - 8 bit ADC 6 - line volts */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 7 - shift force */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b80000 - 12 bit ADC 0 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b80000 - 12 bit ADC 1 - force brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2 | IPF_REVERSE, 25, 40, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( racedriv )
	PORT_START		/* 600000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* diagnostic switch */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* key */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0xfff8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 - clutch pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 25, 100, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 2 - seat */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 3 - shifter lever Y */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 4 - shifter lever X*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 5 - wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b00000 - 8 bit ADC 6 - line volts */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 0 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b80000 - 12 bit ADC 1 - force brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2 | IPF_REVERSE, 25, 40, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( racedrvc )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* diagnostic switch */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* key */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* 1st gear */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* 2nd gear */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* 3rd gear */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* 4th gear */
	PORT_BIT( 0x3000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* center edge on steering wheel */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 - clutch pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 25, 100, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 - force brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2 | IPF_REVERSE, 25, 40, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 400000 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	/* dummy ADC ports to end up with the same number as the full version */
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( stunrun )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xfff8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 25, 10, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 25, 10, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_II_PORT		/* audio port */
INPUT_PORTS_END


INPUT_PORTS_START( steeltal )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* trigger */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* thumb */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* zoom */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* real helicopter flight */
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* volume control */

	PORT_START		/* b00000 - 8 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 0 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 25, 10, 0x00, 0xff )	/* left/right */

	PORT_START		/* b80000 - 12 bit ADC 1 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 25, 10, 0x00, 0xff )	/* up/down */

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_REVERSE, 25, 10, 0x00, 0xff )	/* collective */

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 25, 10, 0x00, 0xff )	/* rudder */

	JSA_III_PORT	/* audio port */
INPUT_PORTS_END


INPUT_PORTS_START( hdrivair )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* start */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* ??? */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_TOGGLE )	/* reverse */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 )	/* ??? */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* wings */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* wings */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* center edge on steering wheel */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 2 - voice mic */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 3 - volume */
	PORT_BIT( 0xff, 0X80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 - elevator */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_REVERSE, 25, 10, 0x00, 0xff )	/* up/down */

	PORT_START		/* b00000 - 8 bit ADC 5 - canopy */
	PORT_BIT( 0xff, 0X80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 - brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2 | IPF_REVERSE, 25, 40, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 7 - seat adjust */
	PORT_BIT( 0xff, 0X80, IPT_UNUSED )

	PORT_START		/* 400000 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_REVERSE, 25, 5, 0x10, 0xf0 )

	/* dummy ADC ports to end up with the same number as the full version */
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct DACinterface dac_interface =
{
	1,
	{ MIXER(100, MIXER_PAN_CENTER) }
};


static struct DACinterface dac2_interface =
{
	2,
	{ MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) }
};



/*************************************
 *
 *	Main board pieces
 *
 *************************************/

/*
	Video timing:

				VERTICAL					HORIZONTAL
	Harddriv:	001D-019D / 01A0 (384)		001A-0099 / 009F (508)
	Harddrvc:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
	Racedriv:	001D-019D / 01A0 (384)		001A-0099 / 009F (508)
	Racedrvc:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
	Stunrun:	0013-00F8 / 0105 (229)		0037-0137 / 013C (512)
	Steeltal:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
	Hdrivair:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
*/

/* Driver board without MSP (used by Race Drivin' cockpit) */
static MACHINE_DRIVER_START( driver_nomsp )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68010, 32000000/4)
	MDRV_CPU_MEMORY(driver_readmem_68k,driver_writemem_68k)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	MDRV_CPU_PERIODIC_INT(hd68k_irq_gen,244)

	MDRV_CPU_ADD_TAG("gsp", TMS34010, 48000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_MEMORY(driver_readmem_gsp,driver_writemem_gsp)
	MDRV_CPU_CONFIG(gsp_config)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (416 - 384)) / (60 * 416))
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(harddriv)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(640, 384)
	MDRV_VISIBLE_AREA(97, 596, 0, 383)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(harddriv)
	MDRV_VIDEO_EOF(harddriv)
	MDRV_VIDEO_UPDATE(harddriv)
MACHINE_DRIVER_END


/* Driver board with MSP (used by Hard Drivin' cockpit) */
static MACHINE_DRIVER_START( driver_msp )
	MDRV_IMPORT_FROM(driver_nomsp)

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("msp", TMS34010, 50000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_MEMORY(driver_readmem_msp,driver_writemem_msp)
	MDRV_CPU_CONFIG(msp_config)

	/* video hardware */
	MDRV_VISIBLE_AREA(89, 596, 0, 383)
MACHINE_DRIVER_END


/* Multisync board without MSP (used by STUN Runner, Steel Talons, Race Drivin' compact) */
static MACHINE_DRIVER_START( multisync_nomsp )
	MDRV_IMPORT_FROM(driver_nomsp)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(multisync_readmem_68k,multisync_writemem_68k)

	MDRV_CPU_MODIFY("gsp")
	MDRV_CPU_MEMORY(multisync_readmem_gsp,multisync_writemem_gsp)

	MDRV_VBLANK_DURATION((1000000 * (307 - 288)) / (60 * 307))

	/* video hardware */
	MDRV_SCREEN_SIZE(640, 288)
	MDRV_VISIBLE_AREA(109, 620, 0, 287)
MACHINE_DRIVER_END


/* Multisync board with MSP (used by Hard Drivin' compact) */
static MACHINE_DRIVER_START( multisync_msp )
	MDRV_IMPORT_FROM(multisync_nomsp)

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("msp", TMS34010, 50000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_MEMORY(multisync_readmem_msp,multisync_writemem_msp)
	MDRV_CPU_CONFIG(msp_config)
MACHINE_DRIVER_END


/* Multisync II board (used by Hard Drivin's Airborne) */
static MACHINE_DRIVER_START( multisync2 )
	MDRV_IMPORT_FROM(multisync_nomsp)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(multisync2_readmem_68k,multisync2_writemem_68k)

	MDRV_CPU_MODIFY("gsp")
	MDRV_CPU_MEMORY(multisync2_readmem_gsp,multisync2_writemem_gsp)
MACHINE_DRIVER_END



/*************************************
 *
 *	ADSP board pieces
 *
 *************************************/

/* ADSP/ADSP II boards (used by Hard/Race Drivin', STUN Runner) */
static MACHINE_DRIVER_START( adsp )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("adsp", ADSP2100, 8000000)
	MDRV_CPU_MEMORY(adsp_readmem,adsp_writemem)
MACHINE_DRIVER_END


/* DS III board (used by Steel Talons) */
static MACHINE_DRIVER_START( ds3 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("adsp", ADSP2101, 12000000)
	MDRV_CPU_MEMORY(ds3_readmem,ds3_writemem)
MACHINE_DRIVER_END


/* DS IV board (used by Hard Drivin's Airborne) */
static MACHINE_DRIVER_START( ds4 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("adsp", ADSP2101, 12000000)
	MDRV_CPU_MEMORY(ds3_readmem,ds3_writemem)

//	MDRV_CPU_ADD_TAG("sound", ADSP2105, 10000000)
//	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
//	MDRV_CPU_MEMORY(ds3snd_readmem,ds3snd_writemem)

//	MDRV_CPU_ADD_TAG("sounddsp", ADSP2105, 10000000)
//	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
//	MDRV_CPU_MEMORY(ds3snd_readmem,ds3snd_writemem)

	MDRV_SOUND_ADD(DAC, dac2_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	DSK board pieces
 *
 *************************************/

/* DSK board (used by Race Drivin') */
static MACHINE_DRIVER_START( dsk )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("dsp32", DSP32C, 40000000)
	MDRV_CPU_CONFIG(dsp32c_config)
	MDRV_CPU_MEMORY(dsk_readmem_dsp32,dsk_writemem_dsp32)
MACHINE_DRIVER_END


/* DSK II board (used by Hard Drivin's Airborne) */
static MACHINE_DRIVER_START( dsk2 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("dsp32", DSP32C, 40000000)
	MDRV_CPU_CONFIG(dsp32c_config)
	MDRV_CPU_MEMORY(dsk2_readmem_dsp32,dsk2_writemem_dsp32)
MACHINE_DRIVER_END



/*************************************
 *
 *	Sound board pieces
 *
 *************************************/

static MACHINE_DRIVER_START( driversnd )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("sound", M68000, 16000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(driversnd_readmem_68k,driversnd_writemem_68k)

	MDRV_CPU_ADD_TAG("sounddsp", TMS32010, 20000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(driversnd_readmem_dsp,driversnd_writemem_dsp)
	MDRV_CPU_PORTS(driversnd_readport_dsp,driversnd_writeport_dsp)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( harddriv )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( driver_msp )		/* original driver board with MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( harddrvc )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_msp )	/* multisync board with MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( racedriv )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( driver_nomsp )	/* original driver board without MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( dsk )				/* DSK board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( racedrvc )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_nomsp )	/* multisync board without MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( dsk )				/* DSK board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( stunrun )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_nomsp )	/* multisync board without MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( jsa_ii_mono )		/* JSA II sound board */

	MDRV_VBLANK_DURATION((1000000 * (261 - 240)) / (60 * 261))

	/* video hardware */
	MDRV_SCREEN_SIZE(640, 240)
	MDRV_VISIBLE_AREA(103, 614, 0, 239)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( steeltal )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_msp )	/* multisync board with MSP */
	MDRV_IMPORT_FROM( ds3 )				/* DS III board */
	MDRV_IMPORT_FROM( jsa_iii_mono )	/* JSA III sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hdrivair )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync2 )		/* multisync II board */
	MDRV_IMPORT_FROM( ds4 )				/* DS IV board */
	MDRV_IMPORT_FROM( dsk2 )			/* DSK II board */
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( harddriv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "hd_200.r", 0x00000, 0x10000, 0xa42a2c69 )
	ROM_LOAD16_BYTE( "hd_210.r", 0x00001, 0x10000, 0x358995b5 )
	ROM_LOAD16_BYTE( "hd_200.s", 0x20000, 0x10000, 0xa668db0e )
	ROM_LOAD16_BYTE( "hd_210.s", 0x20001, 0x10000, 0xab689a94 )
	ROM_LOAD16_BYTE( "hd_200.w", 0xa0000, 0x10000, 0x908ccbbe )
	ROM_LOAD16_BYTE( "hd_210.w", 0xa0001, 0x10000, 0x5b25023c )
	ROM_LOAD16_BYTE( "hd_200.x", 0xc0000, 0x10000, 0xe1f455a3 )
	ROM_LOAD16_BYTE( "hd_210.x", 0xc0001, 0x10000, 0xa7fc3aaa )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "hd_s.70n", 0x00000, 0x08000, 0x0c77fab6 )
	ROM_LOAD16_BYTE( "hd_s.45n", 0x00001, 0x08000, 0x54d6dd5f )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "hd_dsp.10h", 0x00000, 0x10000, 0x1b77f171 )
	ROM_LOAD16_BYTE( "hd_dsp.10k", 0x00001, 0x10000, 0xe50bec32 )
	ROM_LOAD16_BYTE( "hd_dsp.10j", 0x20000, 0x10000, 0x998d3da2 )
	ROM_LOAD16_BYTE( "hd_dsp.10l", 0x20001, 0x10000, 0xbc59a2b7 )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "hd_s.65a", 0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD( "hd_s.55a", 0x10000, 0x10000, 0x071a4309 )
	ROM_LOAD( "hd_s.45a", 0x20000, 0x10000, 0xebf391af )
	ROM_LOAD( "hd_s.30a", 0x30000, 0x10000, 0xf46ef09c )
ROM_END


ROM_START( harddrvc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "068-2102.00r", 0x00000, 0x10000, 0x6252048b )
	ROM_LOAD16_BYTE( "068-2101.10r", 0x00001, 0x10000, 0x4805ba06 )
	ROM_LOAD16_BYTE( "068-2104.00s", 0x20000, 0x10000, 0x8246f945 )
	ROM_LOAD16_BYTE( "068-2103.10s", 0x20001, 0x10000, 0x729941e8 )
	ROM_LOAD16_BYTE( "068-1112.00w", 0xa0000, 0x10000, 0xe5ea74e4 )
	ROM_LOAD16_BYTE( "068-1111.10w", 0xa0001, 0x10000, 0x4d759891 )
	ROM_LOAD16_BYTE( "068-1114.00x", 0xc0000, 0x10000, 0x293c153b )
	ROM_LOAD16_BYTE( "068-1113.10x", 0xc0001, 0x10000, 0x5630390d )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "052-3122.70n", 0x00000, 0x08000, 0x3f20a396 )
	ROM_LOAD16_BYTE( "052-3121.45n", 0x00001, 0x08000, 0x6346bca3 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "hd_dsp.10h", 0x00000, 0x10000, 0x1b77f171 )
	ROM_LOAD16_BYTE( "hd_dsp.10k", 0x00001, 0x10000, 0xe50bec32 )
	ROM_LOAD16_BYTE( "hd_dsp.10j", 0x20000, 0x10000, 0x998d3da2 )
	ROM_LOAD16_BYTE( "hd_dsp.10l", 0x20001, 0x10000, 0xbc59a2b7 )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "hd_s.65a",     0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD( "hd_s.55a",     0x10000, 0x10000, 0x071a4309 )
	ROM_LOAD( "052-3125.30a", 0x20000, 0x10000, 0x856548ff )
	ROM_LOAD( "hd_s.30a",     0x30000, 0x10000, 0xf46ef09c )
ROM_END


ROM_START( stunrun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "sr_200.r", 0x00000, 0x10000, 0xe0ed54d8 )
	ROM_LOAD16_BYTE( "sr_210.r", 0x00001, 0x10000, 0x3008bcf8 )
	ROM_LOAD16_BYTE( "sr_200.s", 0x20000, 0x10000, 0x6acdeeaa )
	ROM_LOAD16_BYTE( "sr_210.s", 0x20001, 0x10000, 0xe8b1262a )
	ROM_LOAD16_BYTE( "sr_200.t", 0x40000, 0x10000, 0x41c4778c )
	ROM_LOAD16_BYTE( "sr_210.t", 0x40001, 0x10000, 0x0d6c9b8f )
	ROM_LOAD16_BYTE( "sr_200.u", 0x60000, 0x10000, 0x0ce849aa )
	ROM_LOAD16_BYTE( "sr_210.u", 0x60001, 0x10000, 0x19bc7495 )
	ROM_LOAD16_BYTE( "sr_200.v", 0x80000, 0x10000, 0x4f6d22c5 )
	ROM_LOAD16_BYTE( "sr_210.v", 0x80001, 0x10000, 0xac6d4d4a )
	ROM_LOAD16_BYTE( "sr_200.w", 0xa0000, 0x10000, 0x3f896aaf )
	ROM_LOAD16_BYTE( "sr_210.w", 0xa0001, 0x10000, 0x47f010ad )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "sr_snd.10c", 0x10000, 0x4000, 0x121ab09a )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "sr_dsp_9.10h", 0x00000, 0x10000, 0x0ebf8e58 )
	ROM_LOAD16_BYTE( "sr_dsp_9.10k", 0x00001, 0x10000, 0xfb98abaf )
	ROM_LOAD16_BYTE( "sr_dsp.10h",   0x20000, 0x10000, 0xbd5380bd )
	ROM_LOAD16_BYTE( "sr_dsp.10k",   0x20001, 0x10000, 0xbde8bd31 )
	ROM_LOAD16_BYTE( "sr_dsp.9h",    0x40000, 0x10000, 0x55a30976 )
	ROM_LOAD16_BYTE( "sr_dsp.9k",    0x40001, 0x10000, 0xd4a9696d )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "sr_snd.1fh",  0x00000, 0x10000, 0x4dc14fe8 )
	ROM_LOAD( "sr_snd.1ef",  0x10000, 0x10000, 0xcbdabbcc )
	ROM_LOAD( "sr_snd.1de",  0x20000, 0x10000, 0xb973d9d1 )
	ROM_LOAD( "sr_snd.1cd",  0x30000, 0x10000, 0x3e419f4e )
ROM_END


ROM_START( stunrnp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "sr_200.r",     0x00000, 0x10000, 0xe0ed54d8 )
	ROM_LOAD16_BYTE( "sr_210.r",     0x00001, 0x10000, 0x3008bcf8 )
	ROM_LOAD16_BYTE( "prog-hi0.s20", 0x20000, 0x10000, 0x0be15a99 )
	ROM_LOAD16_BYTE( "prog-lo0.s21", 0x20001, 0x10000, 0x757c0840 )
	ROM_LOAD16_BYTE( "prog-hi.t20",  0x40000, 0x10000, 0x49bcde9d )
	ROM_LOAD16_BYTE( "prog-lo1.t21", 0x40001, 0x10000, 0x3bdafd89 )
	ROM_LOAD16_BYTE( "sr_200.u",     0x60000, 0x10000, 0x0ce849aa )
	ROM_LOAD16_BYTE( "sr_210.u",     0x60001, 0x10000, 0x19bc7495 )
	ROM_LOAD16_BYTE( "sr_200.v",     0x80000, 0x10000, 0x4f6d22c5 )
	ROM_LOAD16_BYTE( "sr_210.v",     0x80001, 0x10000, 0xac6d4d4a )
	ROM_LOAD16_BYTE( "sr_200.w",     0xa0000, 0x10000, 0x3f896aaf )
	ROM_LOAD16_BYTE( "sr_210.w",     0xa0001, 0x10000, 0x47f010ad )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "sr_snd.10c", 0x10000, 0x4000, 0x121ab09a )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "sr_dsp_9.10h", 0x00000, 0x10000, 0x0ebf8e58 )
	ROM_LOAD16_BYTE( "sr_dsp_9.10k", 0x00001, 0x10000, 0xfb98abaf )
	ROM_LOAD16_BYTE( "sr_dsp.10h",   0x20000, 0x10000, 0xbd5380bd )
	ROM_LOAD16_BYTE( "sr_dsp.10k",   0x20001, 0x10000, 0xbde8bd31 )
	ROM_LOAD16_BYTE( "sr_dsp.9h",    0x40000, 0x10000, 0x55a30976 )
	ROM_LOAD16_BYTE( "sr_dsp.9k",    0x40001, 0x10000, 0xd4a9696d )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "sr_snd.1fh",  0x00000, 0x10000, 0x4dc14fe8 )
	ROM_LOAD( "sr_snd.1ef",  0x10000, 0x10000, 0xcbdabbcc )
	ROM_LOAD( "sr_snd.1de",  0x20000, 0x10000, 0xb973d9d1 )
	ROM_LOAD( "sr_snd.1cd",  0x30000, 0x10000, 0x3e419f4e )
ROM_END


ROM_START( racedriv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "077-5002.bin", 0x00000, 0x10000, 0x0a78adca )
	ROM_LOAD16_BYTE( "077-5001.bin", 0x00001, 0x10000, 0x74b4cd49 )
	ROM_LOAD16_BYTE( "077-5004.bin", 0x20000, 0x10000, 0xc0cbdf4e )
	ROM_LOAD16_BYTE( "077-5003.bin", 0x20001, 0x10000, 0x28eeff77 )
	ROM_LOAD16_BYTE( "077-5006.bin", 0x40000, 0x10000, 0x11cd9323 )
	ROM_LOAD16_BYTE( "077-5005.bin", 0x40001, 0x10000, 0x49c33786 )
	ROM_LOAD16_BYTE( "077-4008.bin", 0x60000, 0x10000, 0xaef71435 )
	ROM_LOAD16_BYTE( "077-4007.bin", 0x60001, 0x10000, 0x446e62fb )
	ROM_LOAD16_BYTE( "077-4010.bin", 0x80000, 0x10000, 0xe7e03770 )
	ROM_LOAD16_BYTE( "077-4009.bin", 0x80001, 0x10000, 0x5dd8ebe4 )
	ROM_LOAD16_BYTE( "rd1012.bin",   0xa0000, 0x10000, 0x9a78b952 )
	ROM_LOAD16_BYTE( "rd1011.bin",   0xa0001, 0x10000, 0xc5cd5491 )
	ROM_LOAD16_BYTE( "rd1014.bin",   0xc0000, 0x10000, 0xa872792a )
	ROM_LOAD16_BYTE( "rd1013.bin",   0xc0001, 0x10000, 0xca7b3e53 )
	ROM_LOAD16_BYTE( "077-1016.bin", 0xe0000, 0x10000, 0xe83a9c99 )
	ROM_LOAD16_BYTE( "077-4015.bin", 0xe0001, 0x10000, 0x725806f3 )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "rd1032.bin", 0x00000, 0x10000, 0x33005f2a )
	ROM_LOAD16_BYTE( "rd1033.bin", 0x00001, 0x10000, 0x4fc800ac )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "rd2021.bin", 0x00000, 0x10000, 0x8b2a98da )
	ROM_LOAD16_BYTE( "rd2023.bin", 0x00001, 0x10000, 0xc6d83d38 )
	ROM_LOAD16_BYTE( "rd2022.bin", 0x20000, 0x10000, 0xc0393c31 )
	ROM_LOAD16_BYTE( "rd2024.bin", 0x20001, 0x10000, 0x1e2fb25f )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x31000, REGION_USER3, 0 )	/* 128k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "077-4030.bin", 0x00000, 0x10000, 0x4207c784 )
	ROM_LOAD16_BYTE( "077-4031.bin", 0x00001, 0x10000, 0x796486b3 )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "rd1123.bin", 0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD( "rd1124.bin", 0x10000, 0x10000, 0x071a4309 )
	ROM_LOAD( "rd3125.bin", 0x20000, 0x10000, 0x856548ff )
	ROM_LOAD( "rd1126.bin", 0x30000, 0x10000, 0xf46ef09c )
	ROM_LOAD( "rd1017.bin", 0x40000, 0x10000, 0xe93129a3 )
ROM_END


ROM_START( racedrv3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "077-3002",   0x00000, 0x10000, 0x78771253 )
	ROM_LOAD16_BYTE( "077-3001",   0x00001, 0x10000, 0xc75373a4 )
	ROM_LOAD16_BYTE( "077-2004",   0x20000, 0x10000, 0x4eb19582 )
	ROM_LOAD16_BYTE( "077-2003",   0x20001, 0x10000, 0x8c36b745 )
	ROM_LOAD16_BYTE( "077-2006",   0x40000, 0x10000, 0x07fd762e )
	ROM_LOAD16_BYTE( "077-2005",   0x40001, 0x10000, 0x71c0a770 )
	ROM_LOAD16_BYTE( "077-2008",   0x60000, 0x10000, 0x5144d31b )
	ROM_LOAD16_BYTE( "077-2007",   0x60001, 0x10000, 0x17903148 )
	ROM_LOAD16_BYTE( "077-2010",   0x80000, 0x10000, 0x8674e44e )
	ROM_LOAD16_BYTE( "077-2009",   0x80001, 0x10000, 0x1e9e4c31 )
	ROM_LOAD16_BYTE( "rd1012.bin", 0xa0000, 0x10000, 0x9a78b952 )
	ROM_LOAD16_BYTE( "rd1011.bin", 0xa0001, 0x10000, 0xc5cd5491 )
	ROM_LOAD16_BYTE( "rd1014.bin", 0xc0000, 0x10000, 0xa872792a )
	ROM_LOAD16_BYTE( "rd1013.bin", 0xc0001, 0x10000, 0xca7b3e53 )
	ROM_LOAD16_BYTE( "077-1016",   0xe0000, 0x10000, 0xe83a9c99 )
	ROM_LOAD16_BYTE( "077-1015",   0xe0001, 0x10000, 0xc51f2702 )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "rd1032.bin", 0x00000, 0x10000, 0x33005f2a )
	ROM_LOAD16_BYTE( "rd1033.bin", 0x00001, 0x10000, 0x4fc800ac )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "rd2021.bin", 0x00000, 0x10000, 0x8b2a98da )
	ROM_LOAD16_BYTE( "rd2023.bin", 0x00001, 0x10000, 0xc6d83d38 )
	ROM_LOAD16_BYTE( "rd2022.bin", 0x20000, 0x10000, 0xc0393c31 )
	ROM_LOAD16_BYTE( "rd2024.bin", 0x20001, 0x10000, 0x1e2fb25f )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x31000, REGION_USER3, 0 )	/* 128k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "077-4030.bin", 0x00000, 0x10000, 0x4207c784 )
	ROM_LOAD16_BYTE( "077-4031.bin", 0x00001, 0x10000, 0x796486b3 )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "rd1123.bin", 0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD( "rd1124.bin", 0x10000, 0x10000, 0x071a4309 )
	ROM_LOAD( "rd3125.bin", 0x20000, 0x10000, 0x856548ff )
	ROM_LOAD( "rd1126.bin", 0x30000, 0x10000, 0xf46ef09c )
	ROM_LOAD( "rd1017.bin", 0x40000, 0x10000, 0xe93129a3 )
ROM_END


ROM_START( racedrvc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "rd2002.bin", 0x00000, 0x10000, 0x669fe6fe )
	ROM_LOAD16_BYTE( "rd2001.bin", 0x00001, 0x10000, 0x9312fd5f )
	ROM_LOAD16_BYTE( "rd1004.bin", 0x20000, 0x10000, 0xf937ce55 )
	ROM_LOAD16_BYTE( "rd1003.bin", 0x20001, 0x10000, 0x5131ee87 )
	ROM_LOAD16_BYTE( "rd1006.bin", 0x40000, 0x10000, 0xfe09241e )
	ROM_LOAD16_BYTE( "rd1005.bin", 0x40001, 0x10000, 0x627c4294 )
	ROM_LOAD16_BYTE( "rd1008.bin", 0x60000, 0x10000, 0x0540d53d )
	ROM_LOAD16_BYTE( "rd1007.bin", 0x60001, 0x10000, 0x2517035a )
	ROM_LOAD16_BYTE( "rd1010.bin", 0x80000, 0x10000, 0x84329826 )
	ROM_LOAD16_BYTE( "rd1009.bin", 0x80001, 0x10000, 0x33556cb5 )
	ROM_LOAD16_BYTE( "rd1012.bin", 0xa0000, 0x10000, 0x9a78b952 )
	ROM_LOAD16_BYTE( "rd1011.bin", 0xa0001, 0x10000, 0xc5cd5491 )
	ROM_LOAD16_BYTE( "rd1014.bin", 0xc0000, 0x10000, 0xa872792a )
	ROM_LOAD16_BYTE( "rd1013.bin", 0xc0001, 0x10000, 0xca7b3e53 )
	ROM_LOAD16_BYTE( "rd1016.bin", 0xe0000, 0x10000, 0xa2a0ed28 )
	ROM_LOAD16_BYTE( "rd1015.bin", 0xe0001, 0x10000, 0x64dd6040 )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "rd1032.bin", 0x00000, 0x10000, 0x33005f2a )
	ROM_LOAD16_BYTE( "rd1033.bin", 0x00001, 0x10000, 0x4fc800ac )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "rd2021.bin", 0x00000, 0x10000, 0x8b2a98da )
	ROM_LOAD16_BYTE( "rd2023.bin", 0x00001, 0x10000, 0xc6d83d38 )
	ROM_LOAD16_BYTE( "rd2022.bin", 0x20000, 0x10000, 0xc0393c31 )
	ROM_LOAD16_BYTE( "rd2024.bin", 0x20001, 0x10000, 0x1e2fb25f )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x31000, REGION_USER3, 0 )	/* 128k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "rd1030.bin", 0x00000, 0x10000, 0x31a600db )
	ROM_LOAD16_BYTE( "rd1031.bin", 0x00001, 0x10000, 0x059c410b )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "rd1123.bin", 0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD( "rd1124.bin", 0x10000, 0x10000, 0x071a4309 )
	ROM_LOAD( "rd3125.bin", 0x20000, 0x10000, 0x856548ff )
	ROM_LOAD( "rd1126.bin", 0x30000, 0x10000, 0xf46ef09c )
	ROM_LOAD( "rd1017.bin", 0x40000, 0x10000, 0xe93129a3 )
ROM_END


ROM_START( steeltal )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "st_200.r", 0x00000, 0x10000, 0x31bf01a9 )
	ROM_LOAD16_BYTE( "st_210.r", 0x00001, 0x10000, 0xb4fa2900 )
	ROM_LOAD16_BYTE( "st_200.s", 0x20000, 0x10000, 0xc31ca924 )
	ROM_LOAD16_BYTE( "st_210.s", 0x20001, 0x10000, 0x7802e86d )
	ROM_LOAD16_BYTE( "st_200.t", 0x40000, 0x10000, 0x01ebc0c3 )
	ROM_LOAD16_BYTE( "st_210.t", 0x40001, 0x10000, 0x1107499c )
	ROM_LOAD16_BYTE( "st_200.u", 0x60000, 0x10000, 0x78e72af9 )
	ROM_LOAD16_BYTE( "st_210.u", 0x60001, 0x10000, 0x420be93b )
	ROM_LOAD16_BYTE( "st_200.v", 0x80000, 0x10000, 0x7eff9f8b )
	ROM_LOAD16_BYTE( "st_210.v", 0x80001, 0x10000, 0x53e9fe94 )
	ROM_LOAD16_BYTE( "st_200.w", 0xa0000, 0x10000, 0xd39e8cef )
	ROM_LOAD16_BYTE( "st_210.w", 0xa0001, 0x10000, 0xb388bf91 )
	ROM_LOAD16_BYTE( "st_200.x", 0xc0000, 0x10000, 0x9f047de7 )
	ROM_LOAD16_BYTE( "st_210.x", 0xc0001, 0x10000, 0xf6b99901 )
	ROM_LOAD16_BYTE( "st_200.y", 0xe0000, 0x10000, 0xdb62362e )
	ROM_LOAD16_BYTE( "st_210.y", 0xe0001, 0x10000, 0xef517db7 )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2101 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "stsnd.1f", 0x10000, 0x4000, 0xc52d8218 )
	ROM_CONTINUE(         0x04000, 0xc000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "stdspcom.5f",  0x00000, 0x10000, 0x4c645933 )

	ROM_REGION16_BE( 0xc0000, REGION_USER1, 0 )	/* 768k for object ROM */
	ROM_LOAD16_BYTE( "stds3.2t",  0x00000, 0x20000, 0xa5882384 )
	ROM_LOAD16_BYTE( "stds3.2lm", 0x00001, 0x20000, 0x0a29db30 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "stsnd.1m",  0x80000, 0x20000, 0xc904db9c )
	ROM_LOAD( "stsnd.1n",  0xa0000, 0x20000, 0x164580b3 )
	ROM_LOAD( "stsnd.1p",  0xc0000, 0x20000, 0x296290a0 )
	ROM_LOAD( "stsnd.1r",  0xe0000, 0x20000, 0xc029d037 )
ROM_END


ROM_START( steeltap )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "rom-200r.bin", 0x00000, 0x10000, 0x72a9ce3b )
	ROM_LOAD16_BYTE( "rom-210r.bin", 0x00001, 0x10000, 0x46d83b42 )
	ROM_LOAD16_BYTE( "rom-200s.bin", 0x20000, 0x10000, 0xbf1b31ae )
	ROM_LOAD16_BYTE( "rom-210s.bin", 0x20001, 0x10000, 0xeaf46672 )
	ROM_LOAD16_BYTE( "rom-200t.bin", 0x40000, 0x10000, 0x3dfe9a3e )
	ROM_LOAD16_BYTE( "rom-210t.bin", 0x40001, 0x10000, 0x3c4e8521 )
	ROM_LOAD16_BYTE( "rom-200u.bin", 0x60000, 0x10000, 0x7a52a980 )
	ROM_LOAD16_BYTE( "rom-210u.bin", 0x60001, 0x10000, 0x6c20e861 )
	ROM_LOAD16_BYTE( "rom-200v.bin", 0x80000, 0x10000, 0x137df911 )
	ROM_LOAD16_BYTE( "rom-210v.bin", 0x80001, 0x10000, 0x2dd87840 )
	ROM_LOAD16_BYTE( "rom-200w.bin", 0xa0000, 0x10000, 0x0bbe5f80 )
	ROM_LOAD16_BYTE( "rom-210w.bin", 0xa0001, 0x10000, 0x31dc9321 )
	ROM_LOAD16_BYTE( "rom-200x.bin", 0xc0000, 0x10000, 0xb494ba85 )
	ROM_LOAD16_BYTE( "rom-210x.bin", 0xc0001, 0x10000, 0x63765dc6 )
	ROM_LOAD16_BYTE( "rom-200y.bin", 0xe0000, 0x10000, 0xb568e1be )
	ROM_LOAD16_BYTE( "rom-210y.bin", 0xe0001, 0x10000, 0x3f5cdd3e )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2101 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "stsnd.1f", 0x10000, 0x4000, 0xc52d8218 )
	ROM_CONTINUE(         0x04000, 0xc000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "dspcom.5f",  0x00000, 0x10000, 0x5bd3acea )

	ROM_REGION16_BE( 0xc0000, REGION_USER1, 0 )	/* 768k for object ROM */
	ROM_LOAD16_BYTE( "rom.2t",  0x00000, 0x20000, 0x05284504 )
	ROM_LOAD16_BYTE( "rom.2lm", 0x00001, 0x20000, 0xd6e65b87 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "stsnd.1m",  0x80000, 0x20000, 0xc904db9c )
	ROM_LOAD( "stsnd.1n",  0xa0000, 0x20000, 0x164580b3 )
	ROM_LOAD( "stsnd.1p",  0xc0000, 0x20000, 0x296290a0 )
	ROM_LOAD( "stsnd.1r",  0xe0000, 0x20000, 0xc029d037 )
ROM_END


ROM_START( hdrivair )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "stesthi.bin", 0x000000, 0x20000, 0xb4bfa451 )
	ROM_LOAD16_BYTE( "stestlo.bin", 0x000001, 0x20000, 0x58758419 )
	ROM_LOAD16_BYTE( "drivehi.bin", 0x040000, 0x20000, 0xd15f5119 )
	ROM_LOAD16_BYTE( "drivelo.bin", 0x040001, 0x20000, 0x34adf4af )
	ROM_LOAD16_BYTE( "wavehi.bin",  0x0c0000, 0x20000, 0xb21a34b6 )
	ROM_LOAD16_BYTE( "wavelo.bin",  0x0c0001, 0x20000, 0x15ed4394 )
	ROM_LOAD16_BYTE( "ms2pics.hi",  0x140000, 0x20000, 0xbca0af7e )
	ROM_LOAD16_BYTE( "ms2pics.lo",  0x140001, 0x20000, 0xc3c6be8c )
	ROM_LOAD16_BYTE( "univhi.bin",  0x180000, 0x20000, 0x86351673 )
	ROM_LOAD16_BYTE( "univlo.bin",  0x180001, 0x20000, 0x22d3b699 )
	ROM_LOAD16_BYTE( "coprochi.bin",0x1c0000, 0x20000, 0x5d2ca109 )
	ROM_LOAD16_BYTE( "coproclo.bin",0x1c0001, 0x20000, 0x5f98b04d )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* dummy region for ADSP 2101 */

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU4, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "sboot.bin", ADSP2100_SIZE + 0x00000, 0x10000, 0xcde4d010 )

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU5, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "xboot.bin", ADSP2100_SIZE + 0x00000, 0x10000, 0x054b46a0 )

	ROM_REGION( 0xc0000, REGION_USER1, 0 )		/* 768k for object ROM */
	ROM_LOAD16_BYTE( "obj0l.bin",   0x00000, 0x20000, 0x1f835f2e )
	ROM_LOAD16_BYTE( "obj0h.bin",   0x00001, 0x20000, 0xc321ab55 )
	ROM_LOAD16_BYTE( "obj1l.bin",   0x40000, 0x20000, 0x3d65f264 )
	ROM_LOAD16_BYTE( "obj1h.bin",   0x40001, 0x20000, 0x2c06b708 )
	ROM_LOAD16_BYTE( "obj2l.bin",   0x80000, 0x20000, 0xb206cc7e )
	ROM_LOAD16_BYTE( "obj2h.bin",   0x80001, 0x20000, 0xa666e98c )

	ROM_REGION16_BE( 0x140000, REGION_USER3, 0 )/* 1MB for DSK ROMs + 256k for RAM */
	ROM_LOAD16_BYTE( "dsk2phi.bin", 0x00000, 0x80000, 0x71c268e0 )
	ROM_LOAD16_BYTE( "dsk2plo.bin", 0x00001, 0x80000, 0xedf96363 )

	ROM_REGION32_LE( 0x200000, REGION_USER4, 0 )/* 2MB for ASIC61 ROMs */
	ROM_LOAD32_BYTE( "roads0.bin",  0x000000, 0x80000, 0x5028eb41 )
	ROM_LOAD32_BYTE( "roads1.bin",  0x000001, 0x80000, 0xc3f2c201 )
	ROM_LOAD32_BYTE( "roads2.bin",  0x000002, 0x80000, 0x527923fe )
	ROM_LOAD32_BYTE( "roads3.bin",  0x000003, 0x80000, 0x2f2023b2 )

	ROM_REGION16_BE( 0x100000, REGION_USER5, 0 )
	/* DS IV sound section (2 x ADSP2105)*/
	ROM_LOAD16_BYTE( "ds3rom0.bin", 0x00001, 0x80000, 0x90b8dbb6 )
	ROM_LOAD16_BYTE( "ds3rom1.bin", 0x00000, 0x80000, 0x58173812 )
	ROM_LOAD16_BYTE( "ds3rom2.bin", 0x00001, 0x80000, 0x5a4b18fa )
	ROM_LOAD16_BYTE( "ds3rom3.bin", 0x00000, 0x80000, 0x63965868 )
	ROM_LOAD16_BYTE( "ds3rom4.bin", 0x00001, 0x80000, 0x15ffb19a )
	ROM_LOAD16_BYTE( "ds3rom5.bin", 0x00000, 0x80000, 0x8d0e9b27 )
	ROM_LOAD16_BYTE( "ds3rom6.bin", 0x00001, 0x80000, 0xce7edbae )
	ROM_LOAD16_BYTE( "ds3rom7.bin", 0x00000, 0x80000, 0x323eff0b )
ROM_END


ROM_START( hdrivaip )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "stest.0h",    0x000000, 0x20000, 0xbf4bb6a0 )
	ROM_LOAD16_BYTE( "stest.0l",    0x000001, 0x20000, 0xf462b511 )
	ROM_LOAD16_BYTE( "drive.hi",    0x040000, 0x20000, 0x56571590 )
	ROM_LOAD16_BYTE( "drive.lo",    0x040001, 0x20000, 0x799e3138 )
	ROM_LOAD16_BYTE( "wave1.hi",    0x0c0000, 0x20000, 0x63872d12 )
	ROM_LOAD16_BYTE( "wave1.lo",    0x0c0001, 0x20000, 0x1a472475 )
	ROM_LOAD16_BYTE( "ms2pics.hi",  0x140000, 0x20000, 0xbca0af7e )
	ROM_LOAD16_BYTE( "ms2pics.lo",  0x140001, 0x20000, 0xc3c6be8c )
	ROM_LOAD16_BYTE( "ms2univ.hi",  0x180000, 0x20000, 0x59c91b15 )
	ROM_LOAD16_BYTE( "ms2univ.lo",  0x180001, 0x20000, 0x7493bf60 )
	ROM_LOAD16_BYTE( "ms2cproc.0h", 0x1c0000, 0x20000, 0x19024f2d )
	ROM_LOAD16_BYTE( "ms2cproc.0l", 0x1c0001, 0x20000, 0x1e48bd46 )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* dummy region for ADSP 2101 */

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU4, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "sboota.bin", ADSP2100_SIZE + 0x00000, 0x10000, 0x3ef819cd )

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU5, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "xboota.bin", ADSP2100_SIZE + 0x00000, 0x10000, 0xd9c49901 )

	ROM_REGION( 0xc0000, REGION_USER1, 0 )		/* 768k for object ROM */
	ROM_LOAD16_BYTE( "objects.0l",  0x00000, 0x20000, 0x3c9e9078 )
	ROM_LOAD16_BYTE( "objects.0h",  0x00001, 0x20000, 0x4480dbae )
	ROM_LOAD16_BYTE( "objects.1l",  0x40000, 0x20000, 0x700bd978 )
	ROM_LOAD16_BYTE( "objects.1h",  0x40001, 0x20000, 0xf613adaf )
	ROM_LOAD16_BYTE( "objects.2l",  0x80000, 0x20000, 0xe3b512f0 )
	ROM_LOAD16_BYTE( "objects.2h",  0x80001, 0x20000, 0x3f83742b )

	ROM_REGION16_BE( 0x140000, REGION_USER3, 0 )/* 1MB for DSK ROMs + 256k for RAM */
	ROM_LOAD16_BYTE( "dskpics.hi",  0x00000, 0x80000, 0xeaa88101 )
	ROM_LOAD16_BYTE( "dskpics.lo",  0x00001, 0x80000, 0x8c6f0750 )

	ROM_REGION32_LE( 0x200000, REGION_USER4, 0 )/* 2MB for ASIC61 ROMs */
	ROM_LOAD16_BYTE( "roads.0",     0x000000, 0x80000, 0xcab2e335 )
	ROM_LOAD16_BYTE( "roads.1",     0x000001, 0x80000, 0x62c244ba )
	ROM_LOAD16_BYTE( "roads.2",     0x000002, 0x80000, 0xba57f415 )
	ROM_LOAD16_BYTE( "roads.3",     0x000003, 0x80000, 0x1e6a4ca0 )

	ROM_REGION16_BE( 0x100000, REGION_USER5, 0 )
	/* DS IV sound section (2 x ADSP2105)*/
	ROM_LOAD16_BYTE( "ds3rom.0",    0x00001, 0x80000, 0x90b8dbb6 )
	ROM_LOAD16_BYTE( "ds3rom.1",    0x00000, 0x80000, 0x03673d8d )
	ROM_LOAD16_BYTE( "ds3rom.2",    0x00001, 0x80000, 0xf67754e9 )
	ROM_LOAD16_BYTE( "ds3rom.3",    0x00000, 0x80000, 0x008d3578 )
	ROM_LOAD16_BYTE( "ds3rom.4",    0x00001, 0x80000, 0x6281efee )
	ROM_LOAD16_BYTE( "ds3rom.5",    0x00000, 0x80000, 0x6ef9ed90 )
	ROM_LOAD16_BYTE( "ds3rom.6",    0x00001, 0x80000, 0xcd4cd6bc )
	ROM_LOAD16_BYTE( "ds3rom.7",    0x00000, 0x80000, 0x3d695e1f )
ROM_END



/*************************************
 *
 *	Common initialization
 *
 *************************************/

/* COMMON INIT: find all the CPUs */
static void find_cpus(void)
{
	hdcpu_main = mame_find_cpu_index("main");
	hdcpu_gsp = mame_find_cpu_index("gsp");
	hdcpu_msp = mame_find_cpu_index("msp");
	hdcpu_adsp = mame_find_cpu_index("adsp");
	hdcpu_sound = mame_find_cpu_index("sound");
	hdcpu_sounddsp = mame_find_cpu_index("sounddsp");
	hdcpu_jsa = mame_find_cpu_index("jsa");
	hdcpu_dsp32 = mame_find_cpu_index("dsp32");
}


static const UINT16 default_eeprom[] =
{
	1,
	0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,
	0x0800,0
};


/* COMMON INIT: initialize the original "driver" main board */
static void init_driver(void)
{
	/* assume we're first to be called */
	find_cpus();

	/* note that we're not multisync and set the default EEPROM data */
	hdgsp_multisync = 0;
	atarigen_eeprom_default = default_eeprom;
}


/* COMMON INIT: initialize the later "multisync" main board */
static void init_multisync(int compact_inputs)
{
	/* assume we're first to be called */
	find_cpus();

	/* note that we're multisync and set the default EEPROM data */
	hdgsp_multisync = 1;
	atarigen_eeprom_default = default_eeprom;

	/* install handlers for the compact driving games' inputs */
	if (compact_inputs)
	{
		install_mem_read16_handler(hdcpu_main, 0x400000, 0x400001, hdc68k_wheel_r);
		install_mem_write16_handler(hdcpu_main, 0x408000, 0x408001, hdc68k_wheel_edge_reset_w);
		install_mem_read16_handler(hdcpu_main, 0xa80000, 0xafffff, hdc68k_port1_r);
	}
}


/* COMMON INIT: initialize the ADSP/ADSP2 board */
static void init_adsp(void)
{
	/* install ADSP program RAM */
	install_mem_read16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_adsp_program_r);
	install_mem_write16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_adsp_program_w);

	/* install ADSP data RAM */
	install_mem_read16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_r);
	install_mem_write16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_w);

	/* install ADSP serial buffer RAM */
	install_mem_read16_handler(hdcpu_main, 0x810000, 0x813fff, hd68k_adsp_buffer_r);
	install_mem_write16_handler(hdcpu_main, 0x810000, 0x813fff, hd68k_adsp_buffer_w);

	/* install ADSP control locations */
	install_mem_write16_handler(hdcpu_main, 0x818000, 0x81801f, hd68k_adsp_control_w);
	install_mem_write16_handler(hdcpu_main, 0x818060, 0x81807f, hd68k_adsp_irq_clear_w);
	install_mem_read16_handler(hdcpu_main, 0x838000, 0x83ffff, hd68k_adsp_irq_state_r);
}


/* COMMON INIT: initialize the DS3 board */
static void init_ds3(void)
{
	/* install ADSP program RAM */
	install_mem_read16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_ds3_program_r);
	install_mem_write16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_ds3_program_w);

	/* install ADSP data RAM */
	install_mem_read16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_r);
	install_mem_write16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_w);
	install_mem_read16_handler(hdcpu_main, 0x80c000, 0x80dfff, hdds3_special_r);
	install_mem_write16_handler(hdcpu_main, 0x80c000, 0x80dfff, hdds3_special_w);

	/* install ADSP control locations */
	install_mem_read16_handler(hdcpu_main, 0x820000, 0x8207ff, hd68k_ds3_gdata_r);
	install_mem_read16_handler(hdcpu_main, 0x820800, 0x820fff, hd68k_ds3_girq_state_r);
	install_mem_write16_handler(hdcpu_main, 0x820000, 0x8207ff, hd68k_ds3_gdata_w);
	install_mem_write16_handler(hdcpu_main, 0x821000, 0x8217ff, hd68k_adsp_irq_clear_w);
	install_mem_read16_handler(hdcpu_main, 0x822000, 0x8227ff, hd68k_ds3_sdata_r);
	install_mem_read16_handler(hdcpu_main, 0x822800, 0x822fff, hd68k_ds3_sirq_state_r);
	install_mem_write16_handler(hdcpu_main, 0x822000, 0x8227ff, hd68k_ds3_sdata_w);
	install_mem_write16_handler(hdcpu_main, 0x823800, 0x823fff, hd68k_ds3_control_w);

	/* if we have a sound DSP, boot it */
	if (hdcpu_sound != -1 && Machine->drv->cpu[hdcpu_sound].cpu_type == CPU_ADSP2105)
		adsp2105_load_boot_data((data8_t *)(memory_region(REGION_CPU1 + hdcpu_sound) + ADSP2100_SIZE),
								(data32_t *)(memory_region(REGION_CPU1 + hdcpu_sound) + ADSP2100_PGM_OFFSET));
	if (hdcpu_sounddsp != -1 && Machine->drv->cpu[hdcpu_sounddsp].cpu_type == CPU_ADSP2105)
		adsp2105_load_boot_data((data8_t *)(memory_region(REGION_CPU1 + hdcpu_sounddsp) + ADSP2100_SIZE),
								(data32_t *)(memory_region(REGION_CPU1 + hdcpu_sounddsp) + ADSP2100_PGM_OFFSET));

/*


/PMEM   = RVASB & EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16 & /AB15
        = 0 0000 0xxx xxxx xxxx xxxx (read/write)
        = 0x000000-0x007fff

/DMEM   = RVASB & EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16 &  AB15
        = 0 0000 1xxx xxxx xxxx xxxx (read/write)
		= 0x008000-0x00ffff

/G68WR  = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 & /AB12 & /AB11
        = 0 0010 0000 0xxx xxxx xxxx (write)
        = 0x020000-0x0207ff

/G68RD0 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 & /AB12 & /AB11
        = 0 0010 0000 0xxx xxxx xxxx (read)
        = 0x020000-0x0207ff

/G68RD1 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 & /AB12 &  AB11
        = 0 0010 0000 1xxx xxxx xxxx (read)
        = 0x020800-0x020fff

/GCGINT = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 &  AB12 & /AB11
        = 0 0010 0001 0xxx xxxx xxxx (write)
        = 0x021000-0x0217ff

/S68WR  = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 & /AB11
        = 0 0010 0010 0xxx xxxx xxxx (write)
        = 0x022000-0x0227ff

/S68RD0 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 & /AB11
        = 0 0010 0010 0xxx xxxx xxxx (read)
        = 0x022000-0x0227ff

/S68RD1 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 &  AB11
        = 0 0010 0010 1xxx xxxx xxxx (read)
        = 0x022800-0x022fff

/SCGINT = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 &  AB12 & /AB11
        = 0 0010 0011 0xxx xxxx xxxx (write)
        = 0x023000-0x0237ff

/LATCH  = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 &  AB12 &  AB11
        = 0 0010 0011 1xxx xxxx xxxx (write)
        = 0x023800-0x023fff




/SBUFF  =         EXTB & /EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12
        |         EXTB &         /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 & /AB11
		= 0 0010 0010 xxxx xxxx xxxx (read)
		| 0 0010 0010 0xxx xxxx xxxx (read/write)

/GBUFF  =         EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16
        |         EXTB & /EWRB & /AB20 & /AB19 & /AB18 &         /AB16 & /AB15 & /AB14 & /AB13 &         /AB11
        |         EXTB &         /AB20 & /AB19 & /AB18 &         /AB16 & /AB15 & /AB14 & /AB13 & /AB12 & /AB11
        = 0 0000 xxxx xxxx xxxx xxxx (read/write)
        | 0 00x0 000x 0xxx xxxx xxxx (read)
        | 0 00x0 0000 0xxx xxxx xxxx (read/write)

/GBUFF2 =         EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16
        = 0 0000 xxxx xxxx xxxx xxxx (read/write)

*/
}


/* COMMON INIT: initialize the DSK add-on board */
static void init_dsk(void)
{
	/* install ASIC61 */
	install_mem_write16_handler(hdcpu_main, 0x85c000, 0x85c7ff, hd68k_dsk_dsp32_w);
	install_mem_read16_handler(hdcpu_main, 0x85c000, 0x85c7ff, hd68k_dsk_dsp32_r);

	/* install control registers */
	install_mem_write16_handler(hdcpu_main, 0x85c800, 0x85c81f, hd68k_dsk_control_w);

	/* install extra RAM */
	install_mem_read16_handler(hdcpu_main, 0x900000, 0x90ffff, hd68k_dsk_ram_r);
	install_mem_write16_handler(hdcpu_main, 0x900000, 0x90ffff, hd68k_dsk_ram_w);
	hddsk_ram = (data16_t *)(memory_region(REGION_USER3) + 0x20000);

	/* install extra ZRAM */
	install_mem_read16_handler(hdcpu_main, 0x910000, 0x910fff, hd68k_dsk_zram_r);
	install_mem_write16_handler(hdcpu_main, 0x910000, 0x910fff, hd68k_dsk_zram_w);
	hddsk_zram = (data16_t *)(memory_region(REGION_USER3) + 0x30000);

	/* install ASIC65 */
	install_mem_write16_handler(hdcpu_main, 0x914000, 0x917fff, asic65_data_w);
	install_mem_read16_handler(hdcpu_main, 0x914000, 0x917fff, asic65_r);
	install_mem_read16_handler(hdcpu_main, 0x918000, 0x91bfff, asic65_io_r);

	/* install extra ROM */
	install_mem_read16_handler(hdcpu_main, 0x940000, 0x95ffff, hd68k_dsk_rom_r);
	hddsk_rom = (data16_t *)(memory_region(REGION_USER3) + 0x00000);

	/* set up the ASIC65 */
	asic65_config(ASIC65_STANDARD);
}


/* COMMON INIT: initialize the DSK II add-on board */
static void init_dsk2(void)
{
	/* install ASIC65 */
	install_mem_write16_handler(hdcpu_main, 0x824000, 0x824003, asic65_data_w);
	install_mem_read16_handler(hdcpu_main, 0x824000, 0x824003, asic65_r);
	install_mem_read16_handler(hdcpu_main, 0x825000, 0x825001, asic65_io_r);

	/* install ASIC61 */
	install_mem_write16_handler(hdcpu_main, 0x827000, 0x8277ff, hd68k_dsk_dsp32_w);
	install_mem_read16_handler(hdcpu_main, 0x827000, 0x8277ff, hd68k_dsk_dsp32_r);

	/* install control registers */
	install_mem_write16_handler(hdcpu_main, 0x827800, 0x82781f, hd68k_dsk_control_w);

	/* install extra RAM */
	install_mem_read16_handler(hdcpu_main, 0x880000, 0x8bffff, hd68k_dsk_ram_r);
	install_mem_write16_handler(hdcpu_main, 0x880000, 0x8bffff, hd68k_dsk_ram_w);
	hddsk_ram = (data16_t *)(memory_region(REGION_USER3) + 0x100000);

	/* install extra ROM */
	install_mem_read16_handler(hdcpu_main, 0x900000, 0x9fffff, hd68k_dsk_rom_r);
	hddsk_rom = (data16_t *)(memory_region(REGION_USER3) + 0x000000);

	/* set up the ASIC65 */
	asic65_config(ASIC65_STANDARD);
}


/* COMMON INIT: initialize the DSPCOM add-on board */
static void init_dspcom(void)
{
	/* install ASIC65 */
	install_mem_write16_handler(hdcpu_main, 0x900000, 0x900003, asic65_data_w);
	install_mem_read16_handler(hdcpu_main, 0x900000, 0x900003, asic65_r);
	install_mem_read16_handler(hdcpu_main, 0x901000, 0x910001, asic65_io_r);

	/* set up the ASIC65 */
	asic65_config(ASIC65_STEELTAL);

	/* install DSPCOM control */
	install_mem_write16_handler(hdcpu_main, 0x904000, 0x90401f, hddspcom_control_w);
}


/* COMMON INIT: initialize the original "driver" sound board */
static void init_driver_sound(void)
{
	hdsnd_init();

	/* install sound handlers */
	install_mem_write16_handler(hdcpu_main, 0x840000, 0x840001, hd68k_snd_data_w);
	install_mem_read16_handler(hdcpu_main, 0x840000, 0x840001, hd68k_snd_data_r);
	install_mem_read16_handler(hdcpu_main, 0x844000, 0x844001, hd68k_snd_status_r);
	install_mem_write16_handler(hdcpu_main, 0x84c000, 0x84c001, hd68k_snd_reset_w);
}




/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( harddriv )
{
	/* initialize the boards */
	init_driver();
	init_adsp();
	init_driver_sound();

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc0f), hdgsp_speedup2_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xffc00f10;

	/* set up msp speedup handler */
	hdmsp_speedup_addr = install_mem_write16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_w);
	install_mem_read16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_r);
	hdmsp_speedup_pc = 0x00723b00;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
}


static DRIVER_INIT( harddrvc )
{
	/* initialize the boards */
	init_multisync(1);
	init_adsp();
	init_driver_sound();

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc0f), hdgsp_speedup2_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xfff40ff0;

	/* set up msp speedup handler */
	hdmsp_speedup_addr = install_mem_write16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_w);
	install_mem_read16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_r);
	hdmsp_speedup_pc = 0x00723b00;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
}


static DRIVER_INIT( stunrun )
{
	/* initialize the boards */
	init_multisync(0);
	init_adsp();
	atarijsa_init(hdcpu_jsa, 14, 0, 0x0020);

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc0f), hdgsp_speedup2_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xfff41070;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);

	/* speed up the 6502 */
	atarigen_init_6502_speedup(hdcpu_jsa, 0x4159, 0x4171);
}


data32_t *rddsp32_speedup;
offs_t rddsp32_speedup_pc;
READ32_HANDLER( rddsp32_speedup_r )
{
	if (activecpu_get_pc() == rddsp32_speedup_pc && (*rddsp32_speedup >> 16) == 0)
	{
		UINT32 r14 = activecpu_get_reg(DSP32_R14);
		UINT32 r1 = cpu_readmem24ledw_word(r14 - 0x14);
		int cycles_to_burn = 17 * 4 * (0x2bc - r1 - 2);
		if (cycles_to_burn > 20 * 4)
		{
			activecpu_adjust_icount(-cycles_to_burn);
			cpu_writemem24ledw_word(r14 - 0x14, r1 + cycles_to_burn / 17);
		}
		msp_speedup_count[0]++;
	}
	return *rddsp32_speedup;
}


static DRIVER_INIT( racedriv )
{
	/* initialize the boards */
	init_driver();
	init_adsp();
	init_dsk();
	init_driver_sound();

	/* set up the slapstic */
	slapstic_init(117);
	hd68k_slapstic_base = install_mem_read16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_r);
	hd68k_slapstic_base = install_mem_write16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_w);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x613c00, 0x613c03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x613e00, 0x613e03, rddsp32_sync1_w);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);

	/* set up dsp32 speedup handlers */
	rddsp32_speedup = install_mem_read32_handler(hdcpu_dsp32, 0x613e04, 0x613e07, rddsp32_speedup_r);
	rddsp32_speedup_pc = 0x6054b0;
}


static DRIVER_INIT( racedrvc )
{
	/* initialize the boards */
	init_multisync(1);
	init_adsp();
	init_dsk();
	init_driver_sound();

	/* set up the slapstic */
	slapstic_init(117);
	hd68k_slapstic_base = install_mem_read16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_r);
	hd68k_slapstic_base = install_mem_write16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_w);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x613c00, 0x613c03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x613e00, 0x613e03, rddsp32_sync1_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff7ecd0), TOBYTE(0xfff7ecdf), hdgsp_protection_w);

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff76f60), TOBYTE(0xfff76f6f), rdgsp_speedup1_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff76f60), TOBYTE(0xfff76f6f), rdgsp_speedup1_r);
	hdgsp_speedup_pc = 0xfff43a00;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);

	/* set up dsp32 speedup handlers */
	rddsp32_speedup = install_mem_read32_handler(hdcpu_dsp32, 0x613e04, 0x613e07, rddsp32_speedup_r);
	rddsp32_speedup_pc = 0x6054b0;
}


static READ16_HANDLER( steeltal_dummy_r )
{
	/* this is required so that INT 4 is recongized as a sound INT */
	return ~0;
}
static DRIVER_INIT( steeltal )
{
	/* initialize the boards */
	init_multisync(0);
	init_ds3();
	init_dspcom();
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarijsa_init(hdcpu_jsa, 14, 0, 0x0020);

	install_mem_read16_handler(hdcpu_main, 0x908000, 0x908001, steeltal_dummy_r);

	/* set up the SLOOP */
	hd68k_slapstic_base = install_mem_read16_handler(hdcpu_main, 0xe0000, 0xfffff, st68k_sloop_r);
	hd68k_slapstic_base = install_mem_write16_handler(hdcpu_main, 0xe0000, 0xfffff, st68k_sloop_w);
	st68k_sloop_alt_base = install_mem_read16_handler(hdcpu_main, 0x4e000, 0x4ffff, st68k_sloop_alt_r);

	/* synchronization */
	stmsp_sync[0] = &hdmsp_ram[TOWORD(0x80010)];
	install_mem_write16_handler(hdcpu_msp, TOBYTE(0x80010), TOBYTE(0x8007f), stmsp_sync0_w);
	stmsp_sync[1] = &hdmsp_ram[TOWORD(0x99680)];
	install_mem_write16_handler(hdcpu_msp, TOBYTE(0x99680), TOBYTE(0x9968f), stmsp_sync1_w);
	stmsp_sync[2] = &hdmsp_ram[TOWORD(0x99d30)];
	install_mem_write16_handler(hdcpu_msp, TOBYTE(0x99d30), TOBYTE(0x99d50), stmsp_sync2_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff965d0), TOBYTE(0xfff965df), hdgsp_protection_w);

	/* set up msp speedup handlers */
	install_mem_read16_handler(hdcpu_msp, TOBYTE(0x80020), TOBYTE(0x8002f), stmsp_speedup_r);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1f99, 0x1f99), hdds3_speedup_r);
	hdds3_speedup_addr = (data16_t *)(memory_region(REGION_CPU1 + hdcpu_adsp) + ADSP2100_DATA_OFFSET) + 0x1f99;
	hdds3_speedup_pc = 0xff;
	hdds3_transfer_pc = 0x4fc18;

	/* speed up the 6502 */
	atarigen_init_6502_speedup(hdcpu_jsa, 0x4168, 0x4180);
}


static DRIVER_INIT( hdrivair )
{
	/* initialize the boards */
	init_multisync(1);
	init_ds3();
	init_dsk2();

	install_mem_read16_handler(hdcpu_main, 0xa80000, 0xafffff, hda68k_port1_r);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x21fe00, 0x21fe03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x21ff00, 0x21ff03, rddsp32_sync1_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff943f0), TOBYTE(0xfff943ff), hdgsp_protection_w);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1f99, 0x1f99), hdds3_speedup_r);
	hdds3_speedup_addr = (data16_t *)(memory_region(REGION_CPU1 + hdcpu_adsp) + ADSP2100_DATA_OFFSET) + 0x1f99;
	hdds3_speedup_pc = 0x2da;
	hdds3_transfer_pc = 0x407b8;
}


static DRIVER_INIT( hdrivaip )
{
	/* initialize the boards */
	init_multisync(1);
	init_ds3();
	init_dsk2();

	install_mem_read16_handler(hdcpu_main, 0xa80000, 0xafffff, hda68k_port1_r);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x21fe00, 0x21fe03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x21ff00, 0x21ff03, rddsp32_sync1_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff916c0), TOBYTE(0xfff916cf), hdgsp_protection_w);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1f9a, 0x1f9a), hdds3_speedup_r);
	hdds3_speedup_addr = (data16_t *)(memory_region(REGION_CPU1 + hdcpu_adsp) + ADSP2100_DATA_OFFSET) + 0x1f9a;
	hdds3_speedup_pc = 0x2d9;
	hdds3_transfer_pc = 0X407da;
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME ( 1988, harddriv, 0,        harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit)" )
GAME ( 1990, harddrvc, harddriv, harddrvc, racedrvc, harddrvc, ROT0, "Atari Games", "Hard Drivin' (compact)" )
GAME ( 1989, stunrun,  0,        stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner" )
GAME ( 1989, stunrnp,  stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (upright prototype)" )
GAME ( 1990, racedriv, 0,        racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit)" )
GAME ( 1990, racedrv3, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, rev 3)" )
GAME ( 1990, racedrvc, racedriv, racedrvc, racedrvc, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact)" )
GAME ( 1991, steeltal, 0,        steeltal, steeltal, steeltal, ROT0, "Atari Games", "Steel Talons" )
GAMEX( 1991, steeltap, steeltal, steeltal, steeltal, steeltal, ROT0, "Atari Games", "Steel Talons (prototype)", GAME_NOT_WORKING )
GAMEX( 1993, hdrivair, 0,        hdrivair, hdrivair, hdrivair, ROT0, "Atari Games", "Hard Drivin's Airborne (prototype)", GAME_NO_SOUND )
GAMEX( 1993, hdrivaip, hdrivair, hdrivair, hdrivair, hdrivaip, ROT0, "Atari Games", "Hard Drivin's Airborne (prototype, early rev)", GAME_NOT_WORKING | GAME_NO_SOUND )
