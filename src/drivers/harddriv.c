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
			- Hard Drivin' Airborne

	To first order, all of the above boards had the same basic features:

		a 68010 @ 7MHz to drive the whole game
		a TMS34010 @ 48MHz (GSP) to render the polygons and graphics
		a TMS34012 @ 50MHz (PSP, labelled SCX6218UTP) to expand pixels
		a TMS34010 @ 50MHz (MSP, optional) to handle in-game calculations

	The original "driver" board had 1MB of VRAM. The "multisync" board
	reduced that to 512k.

	Stacked on top of the main board were two or more additional boards
	that were accessible through an expansion bus. Each game had at least
	an ADSP board and a sound board. Later games had additional boards for
	extra horsepower or for communications between multiple players.

	-----------------------------------------------------------------------

	The ADSP board is usually the board stacked closest to the main board.
	It also comes in three varieties, though these do not match
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
			- Hard Drivin' Airborne

	These boards are the workhorses of the game. They contain a single
	8MHz ADSP-2100 (ADSP and ADSP II) or 12MHz ADSP-2101 (DS III) chip
	that is responsible for all the polygon transformations, lighting, and
	slope computations. Along with the DSP, there are several high-speed
	serial-access ROMs and RAMs.

	The "ADSP II" board is nearly identical to the original "ADSP" board
	except that is has space for extra serial ROM data. The "DS III" is
	an advanced design that contains space for a bunch of complex sound
	circuitry that appears to have never been used.

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
	with a 6502 driving a YM2151 and an OKI6295 ADPCM chip.

	-----------------------------------------------------------------------

	In addition, there were a number of supplemental boards that were
	included with certain games:

		- the "DSK" board (A047724)
			- Race Drivin' Upgrade
			- Race Drivin' Compact

		- the "DSPCOM" board (A049349)
			- Steel Talons

		- the "DSK II" board (A051028)
			- Hard Drivin' Airborne ???

	-----------------------------------------------------------------------

	There are a total of 7 known games (plus variants) on this hardware:

	Hard Drivin'
		- "driver" board (7MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP" or "ADSP II" board (8MHz ADSP-2100)
		- "driver sound" board (7MHz 68000, 20MHz TMS32010)

	Hard Drivin' Compact
		- "multisync" board (7MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "driver sound" board (7MHz 68000, 20MHz TMS32010)

	S.T.U.N. Runner
		- "multisync" board (7MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "JSA II" sound board (1.7MHz 6502, YM2151, OKI6295)

	Race Drivin' Upgrade
		- "driver" board (7MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP" or "ADSP II" board (8MHz ADSP-2100)
		- "DSK" board (40MHz DSP32C, 20MHz TMS32015)
		- "driver sound" board (7MHz 68000, 20MHz TMS32010)

	Race Drivin' Compact
		- "multisync" board (7MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "DSK" board (40MHz DSP32C, 20MHz TMS32015)
		- "driver sound" board (7MHz 68000, 20MHz TMS32010)

	Steel Talons
		- "multisync" board (7MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "DS III" board (12MHz ADSP-2101)
		- "JSA IIIS" sound board (1.7MHz 6502, YM2151, OKI6295)
		- "DSPCOM" I/O board (10MHz ADSP-2105)

	Hard Drivin's Airborne (prototype)
		- main board with ???
		- other boards ???

	BMX Heat (prototype)
		- unknown boards ???

	Metal Maniax (prototype)
		- unknown boards ???

****************************************************************************/


#include "driver.h"
#include "sound/adpcm.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"


/* from slapstic.c */
void slapstic_init(int chip);


/* from machine */
extern data16_t *hd68k_slapstic_base;
extern data16_t *hdgsp_speedup_addr[2];
extern offs_t hdgsp_speedup_pc;
extern data16_t *hdmsp_speedup_addr;
extern offs_t hdmsp_speedup_pc;
extern offs_t hdadsp_speedup_pc;
extern data16_t *hdmsp_ram;
extern data16_t *hddsk_ram;
extern data16_t *hddsk_rom;
extern data16_t *hddsk_zram;

void harddriv_init_machine(void);
int hd68k_vblank_gen(void);
int hd68k_irq_gen(void);
void hdgsp_irq_gen(int state);
void hdmsp_irq_gen(int state);

READ16_HANDLER( hd68k_adsp_program_r );
READ16_HANDLER( hd68k_adsp_data_r );
READ16_HANDLER( hd68k_adsp_buffer_r );
READ16_HANDLER( hd68k_adsp_irq_state_r );
READ16_HANDLER( hd68k_ds3_irq_state_r );
READ16_HANDLER( hd68k_ds3_program_r );
READ16_HANDLER( hd68k_sound_reset_r );
READ16_HANDLER( hd68k_adc8_r );
READ16_HANDLER( hd68k_adc12_r );
READ16_HANDLER( hd68k_gsp_io_r );
READ16_HANDLER( hd68k_msp_io_r );
READ16_HANDLER( hd68k_duart_r );
READ16_HANDLER( hd68k_zram_r );
READ16_HANDLER( hd68k_port0_r );
READ16_HANDLER( hdadsp_special_r );
READ16_HANDLER( hdds3_special_r );
READ16_HANDLER( hdadsp_speedup_r );
READ16_HANDLER( hdadsp_speedup2_r );
READ16_HANDLER( hdgsp_speedup_r );
READ16_HANDLER( hdmsp_speedup_r );
READ16_HANDLER( hddsk_ram_r );
READ16_HANDLER( hddsk_zram_r );
READ16_HANDLER( hddsk_rom_r );
READ16_HANDLER( racedriv_68k_slapstic_r );
READ16_HANDLER( racedriv_asic65_r );
READ16_HANDLER( racedriv_asic65_io_r );
READ16_HANDLER( racedriv_asic61_r );
READ16_HANDLER( steeltal_68k_slapstic_r );

WRITE16_HANDLER( hd68k_nwr_w );
WRITE16_HANDLER( hd68k_irq_ack_w );
WRITE16_HANDLER( hd68k_adsp_program_w );
WRITE16_HANDLER( hd68k_adsp_data_w );
WRITE16_HANDLER( hd68k_adsp_buffer_w );
WRITE16_HANDLER( hd68k_adsp_control_w );
WRITE16_HANDLER( hd68k_adsp_irq_clear_w );
WRITE16_HANDLER( hd68k_ds3_program_w );
WRITE16_HANDLER( hd68k_ds3_control_w );
WRITE16_HANDLER( hd68k_wr0_write );
WRITE16_HANDLER( hd68k_wr1_write );
WRITE16_HANDLER( hd68k_wr2_write );
WRITE16_HANDLER( hd68k_adc_control_w );
WRITE16_HANDLER( hd68k_gsp_io_w );
WRITE16_HANDLER( hd68k_msp_io_w );
WRITE16_HANDLER( hd68k_duart_w );
WRITE16_HANDLER( hd68k_zram_w );
WRITE16_HANDLER( hdgsp_io_w );
WRITE16_HANDLER( hdadsp_special_w );
WRITE16_HANDLER( hdds3_special_w );
WRITE16_HANDLER( hdadsp_speedup2_w );
WRITE16_HANDLER( hdgsp_speedup1_w );
WRITE16_HANDLER( hdgsp_speedup2_w );
WRITE16_HANDLER( hdmsp_speedup_w );
WRITE16_HANDLER( hddsk_ram_w );
WRITE16_HANDLER( hddsk_zram_w );
WRITE16_HANDLER( racedriv_68k_slapstic_w );
WRITE16_HANDLER( racedriv_asic65_w );
WRITE16_HANDLER( racedriv_asic61_w );
WRITE16_HANDLER( steeltal_68k_slapstic_w );


/* from sndhrdw */
READ16_HANDLER( hdsnd_data_r );
READ16_HANDLER( hdsnd_320port_r );
READ16_HANDLER( hdsnd_switches_r );
READ16_HANDLER( hdsnd_status_r );
READ16_HANDLER( hdsnd_320ram_r );
READ16_HANDLER( hdsnd_320com_r );
READ16_HANDLER( hdsnd_68k_r );

WRITE16_HANDLER( hdsnd_data_w );
WRITE16_HANDLER( hdsnd_latches_w );
WRITE16_HANDLER( hdsnd_speech_w );
WRITE16_HANDLER( hdsnd_irqclr_w );
WRITE16_HANDLER( hdsnd_320ram_w );
WRITE16_HANDLER( hdsnd_320com_w );
WRITE16_HANDLER( hdsnd_68k_w );


/* from vidhrdw */
extern UINT8 hdgsp_multisync;
extern UINT8 *hdgsp_vram;
extern data16_t *hdgsp_vram_2bpp;
extern data16_t *hdgsp_control_lo;
extern data16_t *hdgsp_control_hi;
extern data16_t *hdgsp_paletteram_lo;
extern data16_t *hdgsp_paletteram_hi;
extern size_t hdgsp_vram_size;

int harddriv_vh_start(void);
void harddriv_vh_stop(void);
void harddriv_vh_eof(void);
void harddriv_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void hdgsp_write_to_shiftreg(UINT32 address, UINT16 *shiftreg);
void hdgsp_read_from_shiftreg(UINT32 address, UINT16 *shiftreg);
void hdgsp_display_update(UINT32 offs, int rowbytes, int scanline);

READ16_HANDLER( hdgsp_vram_2bpp_r );
READ16_HANDLER( hdgsp_paletteram_lo_r );
READ16_HANDLER( hdgsp_paletteram_hi_r );
READ16_HANDLER( hdgsp_control_lo_r );
READ16_HANDLER( hdgsp_control_hi_r );

WRITE16_HANDLER( hdgsp_vram_1bpp_w );
WRITE16_HANDLER( hdgsp_vram_2bpp_w );
WRITE16_HANDLER( hdgsp_paletteram_lo_w );
WRITE16_HANDLER( hdgsp_paletteram_hi_w );
WRITE16_HANDLER( hdgsp_control_lo_w );
WRITE16_HANDLER( hdgsp_control_hi_w );



/*************************************
 *
 *	Main Memory Map
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0xa80000, 0xafffff, input_port_1_word_r },
	{ 0xb00000, 0xb7ffff, hd68k_adc8_r },
	{ 0xb80000, 0xbfffff, hd68k_adc12_r },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_r },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_r },
	{ 0xff0000, 0xff001f, hd68k_duart_r },
	{ 0xff4000, 0xff4fff, hd68k_zram_r },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x604000, 0x607fff, hd68k_nwr_w },
	{ 0x608000, 0x60bfff, MWA16_NOP },	/* watchdog_reset16_w */
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



/*************************************
 *
 *	GSP Memory Map
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


static MEMORY_READ16_START( gsp_readmem_512k )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffbfffff), MRA16_BANK1 },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( gsp_writemem_512k )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MWA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_2bpp_w, &hdgsp_vram_2bpp },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffbfffff), MWA16_BANK1 },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MWA16_RAM, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END


static MEMORY_READ16_START( gsp_readmem_1mb )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA16_BANK1 },
MEMORY_END


static MEMORY_WRITE16_START( gsp_writemem_1mb )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MWA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_2bpp_w, &hdgsp_vram_2bpp },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MWA16_BANK1, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END



/*************************************
 *
 *	MSP Memory Map
 *
 *************************************/

static struct tms34010_config msp_config =
{
	1,							/* halt on reset */
	hdmsp_irq_gen,				/* generate interrupt */
	NULL,						/* write to shiftreg function */
	NULL						/* read from shiftreg function */
};


static MEMORY_READ16_START( msp_readmem )
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), MRA16_BANK2 },
	{ TOBYTE(0x00700000), TOBYTE(0x007fffff), MRA16_BANK3 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xfff00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( msp_writemem )
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), MWA16_BANK2 },
	{ TOBYTE(0x00700000), TOBYTE(0x007fffff), MWA16_BANK3 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_w },
	{ TOBYTE(0xfff00000), TOBYTE(0xffffffff), MWA16_RAM, &hdmsp_ram },
MEMORY_END



/*************************************
 *
 *	ADSP Memory Map
 *
 *************************************/

static MEMORY_READ16_START( adsp_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x200f), hdadsp_special_r },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( adsp_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x200f), hdadsp_special_w },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	DS III Memory Map
 *
 *************************************/

static MEMORY_READ16_START( ds3_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x200f), hdds3_special_r },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( ds3_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x200f), hdds3_special_w },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Driver Sound Memory Map
 *
 *************************************/

#if 0
static MEMORY_READ16_START( snd_main_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0xff0000, 0xff0fff, hdsnd_data_r },
	{ 0xff1000, 0xff1fff, hdsnd_320port_r },
	{ 0xff2000, 0xff2fff, hdsnd_switches_r },
	{ 0xff3000, 0xff3fff, hdsnd_status_r },
	{ 0xff4000, 0xff7fff, hdsnd_320ram_r },
	{ 0xff8000, 0xffbfff, hdsnd_320com_r },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( snd_main_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0xff0000, 0xff0fff, hdsnd_data_w },
	{ 0xff1000, 0xff1fff, hdsnd_latches_w },
	{ 0xff2000, 0xff2fff, hdsnd_speech_w },
	{ 0xff3000, 0xff3fff, hdsnd_irqclr_w },
	{ 0xff4000, 0xff7fff, hdsnd_320ram_w },
	{ 0xff8000, 0xffbfff, hdsnd_320com_w },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END
#endif



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( harddriv )
	PORT_START		/* 600000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 (gas) */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 10, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 (clutch) */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 25, 10, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 2 (seat) */
	PORT_BIT( 0xff, 0x80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 3 (shift F/B) */
	PORT_BIT( 0xff, 0x80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 (shift L/R) */
	PORT_BIT( 0xff, 0x80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 5 */
	PORT_BIT( 0xff, 0x80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 */
	PORT_BIT( 0xff, 0x80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, 0x80, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 0 (wheel) */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 10, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 1 (brake) */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2 | IPF_REVERSE, 25, 10, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( racedriv )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 25, 10, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 25, 10, 0x00, 0xff )

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
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 10, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 1 */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2 | IPF_REVERSE, 25, 10, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( stunrun )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

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
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 )
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

	JSA_III_PORT	/* audio port */
INPUT_PORTS_END


INPUT_PORTS_START( hdrivair )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

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
INPUT_PORTS_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_harddriv =
{
	/* basic machine hardware */
	{
		{
			CPU_M68010,
			32000000/4,
			main_readmem,main_writemem,0,0,
			hd68k_vblank_gen,1,
			hd68k_irq_gen,244
		},
		{
			CPU_TMS34010,
			48000000/TMS34010_CLOCK_DIVIDER,
			gsp_readmem_1mb,gsp_writemem_1mb,0,0,
			ignore_interrupt,1,
			0,0,&gsp_config
		},
		{
			CPU_TMS34010,
			50000000/TMS34010_CLOCK_DIVIDER,
			msp_readmem,msp_writemem,0,0,
			ignore_interrupt,1,
			0,0,&msp_config
		},
		{
			CPU_ADSP2100,
			8000000,
			adsp_readmem,adsp_writemem,0,0,
			ignore_interrupt,1
		},
/*		{
			CPU_M68000 | CPU_AUDIO_CPU,
			16000000/2,
			snd_main_readmem,snd_main_writemem,0,0,
			ignore_interrupt,1
		},
		{
			CPU_TMS320C10 | CPU_AUDIO_CPU,
			20000000/4,
			main_readmem,main_writemem,0,0,
			hd68k_vblank_gen,1,
			hd68k_irq_gen,244
		}*/
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,
	harddriv_init_machine,

	/* video hardware */
	512, 384, { 0, 511, 0, 383 },
	0,
	1024, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	harddriv_vh_eof,
	harddriv_vh_start,
	harddriv_vh_stop,
	harddriv_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ 0 }
	},
	atarigen_nvram_handler
};


static struct MachineDriver machine_driver_racedriv =
{
	/* basic machine hardware */
	{
		{
			CPU_M68010,
			32000000/4,
			main_readmem,main_writemem,0,0,
			hd68k_vblank_gen,1,
			hd68k_irq_gen,244
		},
		{
			CPU_TMS34010,
			48000000/TMS34010_CLOCK_DIVIDER,
			gsp_readmem_512k,gsp_writemem_512k,0,0,
			ignore_interrupt,1,
			0,0,&gsp_config
		},
		{
			CPU_TMS34010,
			50000000/TMS34010_CLOCK_DIVIDER,
			msp_readmem,msp_writemem,0,0,
			ignore_interrupt,1,
			0,0,&msp_config
		},
		{
			CPU_ADSP2100,
			8000000,
			adsp_readmem,adsp_writemem,0,0,
			ignore_interrupt,1
		},
/*		{
			CPU_DSP32,
			40000000/4,
			dsp32_readmem,dsp32_readmem,0,0,
			ignore_interrupt,1
		},
	{
			CPU_M68000 | CPU_AUDIO_CPU,
			ATARI_CLOCK_14MHz/2,
			main_readmem,main_writemem,0,0,
			hd68k_vblank_gen,1,
			hd68k_irq_gen,244
		},
		{
			CPU_TMS320C10 | CPU_AUDIO_CPU,
			20000000/4,
			main_readmem,main_writemem,0,0,
			hd68k_vblank_gen,1,
			hd68k_irq_gen,244
		}*/
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,
	harddriv_init_machine,

	/* video hardware */
	512, 288, { 0, 511, 0, 287 },
	0,
	1024, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	harddriv_vh_eof,
	harddriv_vh_start,
	harddriv_vh_stop,
	harddriv_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ 0 }
	},
	atarigen_nvram_handler
};


static struct MachineDriver machine_driver_stunrun =
{
	/* basic machine hardware */
	{
		{
			CPU_M68010,
			32000000/4,
			main_readmem,main_writemem,0,0,
			hd68k_vblank_gen,1,
			hd68k_irq_gen,244
		},
		{
			CPU_TMS34010,
			48000000/TMS34010_CLOCK_DIVIDER,
			gsp_readmem_512k,gsp_writemem_512k,0,0,
			ignore_interrupt,1,
			0,0,&gsp_config
		},
		{
			CPU_ADSP2100,
			8000000,
			adsp_readmem,adsp_writemem,0,0,
			ignore_interrupt,1
		},
		JSA_II_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,
	harddriv_init_machine,

	/* video hardware */
	512, 240, { 0, 511, 0, 239 },
	0,
	1024, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	harddriv_vh_eof,
	harddriv_vh_start,
	harddriv_vh_stop,
	harddriv_vh_screenrefresh,

	/* sound hardware */
	JSA_II_MONO(REGION_SOUND1),

	atarigen_nvram_handler
};


static struct MachineDriver machine_driver_steeltal =
{
	/* basic machine hardware */
	{
		{
			CPU_M68010,
			32000000/4,
			main_readmem,main_writemem,0,0,
			hd68k_vblank_gen,1,
			hd68k_irq_gen,244
		},
		{
			CPU_TMS34010,
			48000000/TMS34010_CLOCK_DIVIDER,
			gsp_readmem_1mb,gsp_writemem_1mb,0,0,
			ignore_interrupt,1,
			0,0,&gsp_config
		},
		{
			CPU_TMS34010,
			50000000/TMS34010_CLOCK_DIVIDER,
			msp_readmem,msp_writemem,0,0,
			ignore_interrupt,1,
			0,0,&msp_config
		},
		{
			CPU_ADSP2105,
			12000000,
			ds3_readmem,ds3_writemem,0,0,
			ignore_interrupt,1
		},
		JSA_III_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	harddriv_init_machine,

	/* video hardware */
	512, 288, { 0, 511, 0, 287 },
	0,
	1024, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	harddriv_vh_eof,
	harddriv_vh_start,
	harddriv_vh_stop,
	harddriv_vh_screenrefresh,

	/* sound hardware */
	JSA_III_MONO(REGION_SOUND1),

	atarigen_nvram_handler
};



/*************************************
 *
 *	Common initialization
 *
 *************************************/

/* COMMON INIT: initialize the original "driver" main board */
static void init_driver(void)
{
	static const UINT16 default_eeprom[] =
	{
		1,
		0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,
		0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,
		0x1000,0
	};

	hdgsp_multisync = 0;
	atarigen_eeprom_default = default_eeprom;

	/* install the non-multisync memory handlers */
	install_mem_write16_handler(1, TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_1bpp_w);

	/* set up the port handlers */
	install_mem_read16_handler(0, 0x600000, 0x600001, hd68k_port0_r);
//	install_mem_read16_handler(0, 0x840000, 0x84ffff, hdsnd_68k_r);
//	install_mem_write16_handler(0, 0x840000, 0x84ffff, hdsnd_68k_w);
}


/* COMMON INIT: initialize the later "multisync" main board */
static void init_multisync(void)
{
	hdgsp_multisync = 1;
	atarigen_eeprom_default = NULL;

	/* set up the port handlers */
	install_mem_read16_handler(0, 0x600000, 0x600001, atarigen_sound_upper_r);
	install_mem_write16_handler(0, 0x600000, 0x600001, atarigen_sound_upper_w);
	install_mem_read16_handler(0, 0x60c000, 0x60c001, hd68k_port0_r);
	install_mem_read16_handler(0, 0x604000, 0x607fff, hd68k_sound_reset_r);
}


/* COMMON INIT: initialize the ADSP/ADSP2 board */
static void init_adsp(void)
{
	/* install ADSP program RAM */
	install_mem_read16_handler(0, 0x800000, 0x807fff, hd68k_adsp_program_r);
	install_mem_write16_handler(0, 0x800000, 0x807fff, hd68k_adsp_program_w);

	/* install ADSP data RAM */
	install_mem_read16_handler(0, 0x808000, 0x80bfff, hd68k_adsp_data_r);
	install_mem_write16_handler(0, 0x808000, 0x80bfff, hd68k_adsp_data_w);

	/* install ADSP serial buffer RAM */
	install_mem_read16_handler(0, 0x810000, 0x813fff, hd68k_adsp_buffer_r);
	install_mem_write16_handler(0, 0x810000, 0x813fff, hd68k_adsp_buffer_w);

	/* install ADSP control locations */
	install_mem_read16_handler(0, 0x838000, 0x83ffff, hd68k_adsp_irq_state_r);
	install_mem_write16_handler(0, 0x818000, 0x81801f, hd68k_adsp_control_w);
	install_mem_write16_handler(0, 0x818060, 0x81807f, hd68k_adsp_irq_clear_w);
}


/* COMMON INIT: initialize the DS3 board */
static void init_ds3(void)
{
	/* install ADSP program RAM */
	install_mem_read16_handler(0, 0x800000, 0x807fff, hd68k_ds3_program_r);
	install_mem_write16_handler(0, 0x800000, 0x807fff, hd68k_ds3_program_w);

	/* install ADSP data RAM */
	install_mem_read16_handler(0, 0x808000, 0x80bfff, hd68k_adsp_data_r);
	install_mem_write16_handler(0, 0x808000, 0x80bfff, hd68k_adsp_data_w);

	/* install ADSP control locations */
	install_mem_read16_handler(0, 0x820800, 0x820801, hd68k_ds3_irq_state_r);
	install_mem_write16_handler(0, 0x821000, 0x821001, hd68k_adsp_irq_clear_w);
	install_mem_write16_handler(0, 0x823800, 0x82381f, hd68k_ds3_control_w);

/*
	other DS3 addresses:
		$821000 = write after IRQ
		$820000 = serial incrementing read
		$820800

	-----------------------------------------------------------------------

	/G68RD0=820000:	reads 16-bit latch from ADSP, clears GFLAG

	/G68RD1=820800:	D15 = G68FLAG
					D14 = GFLAG
					D13 = G68IRQS
					D12 = /GRIRQ

	/G68WR=820000:	clocks a 1 into G68FLAG (cleared by /GRD0)
					clocks A1 into GCMD
					latches 16 bits of data, sets G68FLAG

	/GCGINT=821000:	clears ADSPIRQ and /GRIRQ

	-----------------------------------------------------------------------

	/GRD0=2000:		reads 16-bit latch from 68k, clears G68FLAG

	/GRD1=2001:		D15 = GCMD
					D14 = G68FLAG
					D13 = GLFAG
					D12 = /GWB

	/GROM=2006:		reads 16-bits of ROM data

	/GFWCLR=2007:

	/GWR0=2000:		latches 16 bits of data, sets GFLAG

	/GWR1=2001:		clocks D9 into ADSPIRQ, and /D9 into /GRIRQ (cleared by /GCGINT)

	/GWR2=2002:		clocks D8 into SEND, and /D8 into /RECEIVE (cleared by /GRES)

	/GWR3=2003:		clocks D9 into GFIRQS, and /D9 into G68IRQS (cleared by /GRES)

	/GRML=2004:		clocks D0-D15 into A0-A15 for ROMs

	/GRMADH=2005: 	clocks D0-D2 into A16-A18 and ROM select for ROMs

	/IRQ2 = /GIRQ2 = (!/G68FLAG || !G68IRQS) && (!GFLAG || !GFIRQS)
*/
}


/* COMMON INIT: initialize the DSK add-on board */
static void init_dsk(void)
{
	/* install extra ROM */
	install_mem_read16_handler(0, 0x940000, 0x95ffff, hddsk_rom_r);
	hddsk_rom = (data16_t *)(memory_region(REGION_USER3) + 0x00000);

	/* install extra RAM */
	install_mem_read16_handler(0, 0x900000, 0x90ffff, hddsk_ram_r);
	install_mem_write16_handler(0, 0x900000, 0x90ffff, hddsk_ram_w);
	hddsk_ram = (data16_t *)(memory_region(REGION_USER3) + 0x20000);

	/* install extra ZRAM */
	install_mem_read16_handler(0, 0x910000, 0x910fff, hddsk_zram_r);
	install_mem_write16_handler(0, 0x910000, 0x910fff, hddsk_zram_w);
	hddsk_zram = (data16_t *)(memory_region(REGION_USER3) + 0x30000);

	/* install ASIC65 */
	install_mem_write16_handler(0, 0x914000, 0x917fff, racedriv_asic65_w);
	install_mem_read16_handler(0, 0x914000, 0x917fff, racedriv_asic65_r);
	install_mem_read16_handler(0, 0x918000, 0x91bfff, racedriv_asic65_io_r);

	/* install ASIC61 */
	install_mem_write16_handler(0, 0x85c000, 0x85c7ff, racedriv_asic61_w);
	install_mem_read16_handler(0, 0x85c000, 0x85c7ff, racedriv_asic61_r);
}


/* COMMON INIT: initialize the original "driver" sound board */
static void init_driver_sound(void)
{
	data16_t *base = (data16_t *)memory_region(REGION_SOUND1);
	int length = memory_region_length(REGION_SOUND1) / 2;
	int i;

	/* adjust the sound ROMs */
	for (i = 0; i < length; i++)
		base[i] >>= 1;
}




/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_harddriv(void)
{
	/* initialize the boards */
	init_driver();
	init_adsp();
	init_driver_sound();

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(1, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc1f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(1, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc1f), hdgsp_speedup2_w);
	install_mem_read16_handler(1, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc1f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xffc00f10;

	/* set up msp speedup handler */
	hdmsp_speedup_addr = install_mem_write16_handler(2, TOBYTE(0x00751b00), TOBYTE(0x00751b1f), hdmsp_speedup_w);
	install_mem_read16_handler(2, TOBYTE(0x00751b00), TOBYTE(0x00751b1f), hdmsp_speedup_r);
	hdmsp_speedup_pc = 0x00723b00;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(3, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(3, ADSP_DATA_ADDR_RANGE(0x0958, 0x0958), hdadsp_speedup2_r);
	install_mem_write16_handler(3, ADSP_DATA_ADDR_RANGE(0x0033, 0x0033), hdadsp_speedup2_w);
	hdadsp_speedup_pc = 0x139;
}


static void init_harddrvc(void)
{
	/* initialize the boards */
	init_multisync();
	init_adsp();
	init_driver_sound();
}


static void init_stunrun(void)
{
	/* initialize the boards */
	init_multisync();
	init_adsp();

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(1, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc1f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(1, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc1f), hdgsp_speedup2_w);
	install_mem_read16_handler(1, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc1f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xfff41070;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(2, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(2, ADSP_DATA_ADDR_RANGE(0x0958, 0x0958), hdadsp_speedup2_r);
	install_mem_write16_handler(2, ADSP_DATA_ADDR_RANGE(0x0033, 0x0033), hdadsp_speedup2_w);
	hdadsp_speedup_pc = 0x147;

	atarijsa_init(3, 14, 0, 0x0020);

	/* speed up the 6502 */
	atarigen_init_6502_speedup(3, 0x4159, 0x4171);
}


static READ16_HANDLER( steeltal_dummy_r )
{
	/* this is required so that INT 4 is recongized as a sound INT */
	return ~0;
}
static void init_steeltal(void)
{
	/* initialize the boards */
	init_multisync();
	init_ds3();

	install_mem_read16_handler(0, 0x908000, 0x908001, steeltal_dummy_r);

	/* set up the "slapstic" */
	hd68k_slapstic_base = install_mem_read16_handler(0, 0xe0000, 0xfffff, steeltal_68k_slapstic_r);
	hd68k_slapstic_base = install_mem_write16_handler(0, 0xe0000, 0xfffff, steeltal_68k_slapstic_w);

	atarijsa3_init_adpcm(REGION_SOUND1);
	atarijsa_init(4, 14, 0, 0x0020);

	/* speed up the 6502 */
/*	atarigen_init_6502_speedup(4, 0x4159, 0x4171);*/
}


static void init_racedriv(void)
{
	/* initialize the boards */
	init_driver();
	init_adsp();
	init_dsk();
	init_driver_sound();

	/* set up the slapstic */
	slapstic_init(117);
	hd68k_slapstic_base = install_mem_read16_handler(0, 0xe0000, 0xfffff, racedriv_68k_slapstic_r);
	hd68k_slapstic_base = install_mem_write16_handler(0, 0xe0000, 0xfffff, racedriv_68k_slapstic_w);
}


static void init_racedrvc(void)
{
	/* initialize the boards */
	init_multisync();
	init_adsp();
	init_dsk();
	init_driver_sound();

	/* set up the slapstic */
	slapstic_init(117);
	hd68k_slapstic_base = install_mem_read16_handler(0, 0xe0000, 0xfffff, racedriv_68k_slapstic_r);
	hd68k_slapstic_base = install_mem_write16_handler(0, 0xe0000, 0xfffff, racedriv_68k_slapstic_w);
}


static void init_hdrivair(void)
{
	init_multisync();
	init_adsp();
}



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( harddriv )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "hd_200.r", 0x00000, 0x10000, 0xa42a2c69 )
	ROM_LOAD16_BYTE( "hd_210.r", 0x00001, 0x10000, 0x358995b5 )
	ROM_LOAD16_BYTE( "hd_200.s", 0x20000, 0x10000, 0xa668db0e )
	ROM_LOAD16_BYTE( "hd_210.s", 0x20001, 0x10000, 0xab689a94 )
	ROM_LOAD16_BYTE( "hd_200.w", 0xa0000, 0x10000, 0x908ccbbe )
	ROM_LOAD16_BYTE( "hd_210.w", 0xa0001, 0x10000, 0x5b25023c )
	ROM_LOAD16_BYTE( "hd_200.x", 0xc0000, 0x10000, 0xe1f455a3 )
	ROM_LOAD16_BYTE( "hd_210.x", 0xc0001, 0x10000, 0xa7fc3aaa )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "hd_s.45n", 0x00001, 0x08000, 0x54d6dd5f )
	ROM_LOAD16_BYTE( "hd_s.70n", 0x00000, 0x08000, 0x0c77fab6 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* dummy region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "hd_dsp.10h", 0x00000, 0x10000, 0x1b77f171 )
	ROM_LOAD16_BYTE( "hd_dsp.10k", 0x00001, 0x10000, 0xe50bec32 )
	ROM_LOAD16_BYTE( "hd_dsp.10j", 0x20000, 0x10000, 0x998d3da2 )
	ROM_LOAD16_BYTE( "hd_dsp.10l", 0x20001, 0x10000, 0xbc59a2b7 )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION16_BE( 0x80000, REGION_SOUND1, 0 )	/* 4*128k for audio serial ROMs */
	ROM_LOAD16_BYTE( "hd_s.65a", 0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD16_BYTE( "hd_s.55a", 0x20000, 0x10000, 0x071a4309 )
	ROM_LOAD16_BYTE( "hd_s.45a", 0x40000, 0x10000, 0xebf391af )
	ROM_LOAD16_BYTE( "hd_s.30a", 0x60000, 0x10000, 0xf46ef09c )
ROM_END


ROM_START( harddrvc )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "068-2101.10r", 0x00001, 0x10000, 0x4805ba06 )
	ROM_LOAD16_BYTE( "068-2102.00r", 0x00000, 0x10000, 0x6252048b )
	ROM_LOAD16_BYTE( "068-2103.10s", 0x20001, 0x10000, 0x729941e8 )
	ROM_LOAD16_BYTE( "068-2104.00s", 0x20000, 0x10000, 0x8246f945 )
	ROM_LOAD16_BYTE( "068-1111.10w", 0xa0001, 0x10000, 0x4d759891 )
	ROM_LOAD16_BYTE( "068-1112.00w", 0xa0000, 0x10000, 0xe5ea74e4 )
	ROM_LOAD16_BYTE( "068-1113.10x", 0xc0001, 0x10000, 0x5630390d )
	ROM_LOAD16_BYTE( "068-1114.00x", 0xc0000, 0x10000, 0x293c153b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x10000, REGION_CPU5, 0 )		/* dummy region for DSP32 */

	ROM_REGION( 0x50000, REGION_CPU6, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "052-3121.45n", 0x00001, 0x08000, 0x6346bca3 )
	ROM_LOAD16_BYTE( "052-3122.70n", 0x00000, 0x08000, 0x3f20a396 )

	ROM_REGION( 0x10000, REGION_CPU7, 0 )		/* dummy region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "hd_dsp.10h", 0x00000, 0x10000, 0x1b77f171 )
	ROM_LOAD16_BYTE( "hd_dsp.10k", 0x00001, 0x10000, 0xe50bec32 )
	ROM_LOAD16_BYTE( "hd_dsp.10j", 0x20000, 0x10000, 0x998d3da2 )
	ROM_LOAD16_BYTE( "hd_dsp.10l", 0x20001, 0x10000, 0xbc59a2b7 )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION16_BE( 0x20000, REGION_USER3, 0 )		/* 128k for DSK ROMs */

	ROM_REGION16_BE( 0x80000, REGION_SOUND1, 0 )	/* 10*128k for audio serial ROMs */
	ROM_LOAD16_BYTE( "hd_s.65a",     0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD16_BYTE( "hd_s.55a",     0x20000, 0x10000, 0x071a4309 )
	ROM_LOAD16_BYTE( "052-3125.30a", 0x40000, 0x10000, 0x856548ff )
	ROM_LOAD16_BYTE( "hd_s.30a",     0x60000, 0x10000, 0xf46ef09c )
ROM_END


ROM_START( stunrun )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
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

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "sr_snd.10c", 0x10000, 0x4000, 0x121ab09a )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "sr_dsp_9.10h", 0x00000, 0x10000, 0x0ebf8e58 )
	ROM_LOAD16_BYTE( "sr_dsp_9.10k", 0x00001, 0x10000, 0xfb98abaf )
	ROM_LOAD16_BYTE( "sr_dsp.10h",   0x20000, 0x10000, 0xbd5380bd )
	ROM_LOAD16_BYTE( "sr_dsp.10k",   0x20001, 0x10000, 0xbde8bd31 )
	ROM_LOAD16_BYTE( "sr_dsp.9h",    0x40000, 0x10000, 0x55a30976 )
	ROM_LOAD16_BYTE( "sr_dsp.9k",    0x40001, 0x10000, 0xd4a9696d )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "sr_snd.1fh",  0x00000, 0x10000, 0x4dc14fe8 )
	ROM_LOAD( "sr_snd.1ef",  0x10000, 0x10000, 0xcbdabbcc )
	ROM_LOAD( "sr_snd.1de",  0x20000, 0x10000, 0xb973d9d1 )
	ROM_LOAD( "sr_snd.1cd",  0x30000, 0x10000, 0x3e419f4e )
ROM_END


ROM_START( stunrnp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "sr_200.r", 0x00000, 0x10000, 0xe0ed54d8 )
	ROM_LOAD16_BYTE( "sr_210.r", 0x00001, 0x10000, 0x3008bcf8 )
	ROM_LOAD16_BYTE( "prog-hi0.s20", 0x20000, 0x10000, 0x0be15a99 )
	ROM_LOAD16_BYTE( "prog-lo0.s21", 0x20001, 0x10000, 0x757c0840 )
	ROM_LOAD16_BYTE( "prog-hi.t20",  0x40000, 0x10000, 0x49bcde9d )
	ROM_LOAD16_BYTE( "prog-lo1.t21", 0x40001, 0x10000, 0x3bdafd89 )
	ROM_LOAD16_BYTE( "sr_200.u", 0x60000, 0x10000, 0x0ce849aa )
	ROM_LOAD16_BYTE( "sr_210.u", 0x60001, 0x10000, 0x19bc7495 )
	ROM_LOAD16_BYTE( "sr_200.v", 0x80000, 0x10000, 0x4f6d22c5 )
	ROM_LOAD16_BYTE( "sr_210.v", 0x80001, 0x10000, 0xac6d4d4a )
	ROM_LOAD16_BYTE( "sr_200.w", 0xa0000, 0x10000, 0x3f896aaf )
	ROM_LOAD16_BYTE( "sr_210.w", 0xa0001, 0x10000, 0x47f010ad )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "sr_snd.10c", 0x10000, 0x4000, 0x121ab09a )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "sr_dsp_9.10h", 0x00000, 0x10000, 0x0ebf8e58 )
	ROM_LOAD16_BYTE( "sr_dsp_9.10k", 0x00001, 0x10000, 0xfb98abaf )
	ROM_LOAD16_BYTE( "sr_dsp.10h",   0x20000, 0x10000, 0xbd5380bd )
	ROM_LOAD16_BYTE( "sr_dsp.10k",   0x20001, 0x10000, 0xbde8bd31 )
	ROM_LOAD16_BYTE( "sr_dsp.9h",    0x40000, 0x10000, 0x55a30976 )
	ROM_LOAD16_BYTE( "sr_dsp.9k",    0x40001, 0x10000, 0xd4a9696d )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "sr_snd.1fh",  0x00000, 0x10000, 0x4dc14fe8 )
	ROM_LOAD( "sr_snd.1ef",  0x10000, 0x10000, 0xcbdabbcc )
	ROM_LOAD( "sr_snd.1de",  0x20000, 0x10000, 0xb973d9d1 )
	ROM_LOAD( "sr_snd.1cd",  0x30000, 0x10000, 0x3e419f4e )
ROM_END


ROM_START( steeltal )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
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

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "stsnd.1f", 0x10000, 0x4000, 0xc52d8218 )
	ROM_CONTINUE(         0x04000, 0xc000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "stdspcom.5f",  0x00000, 0x10000, 0x4c645933 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "stds3.2lm", 0x00000, 0x20000, 0x0a29db30 )
	ROM_LOAD16_BYTE( "stds3.2t",  0x00001, 0x20000, 0xa5882384 )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "stsnd.1m",  0x80000, 0x20000, 0xc904db9c )
	ROM_LOAD( "stsnd.1n",  0xa0000, 0x20000, 0x164580b3 )
	ROM_LOAD( "stsnd.1p",  0xc0000, 0x20000, 0x296290a0 )
	ROM_LOAD( "stsnd.1r",  0xe0000, 0x20000, 0xc029d037 )
ROM_END


ROM_START( steeltdb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "sta200r",    0x00000, 0x10000, 0x1e69690a )
	ROM_LOAD16_BYTE( "sta210r",    0x00001, 0x10000, 0x3f6af83e )
	ROM_LOAD16_BYTE( "xprog200.s", 0x20000, 0x10000, 0x0bdcd267 )
	ROM_LOAD16_BYTE( "xprog210.s", 0x20001, 0x10000, 0x565786e9 )
	ROM_LOAD16_BYTE( "st_200.t",   0x40000, 0x10000, 0x01ebc0c3 )
	ROM_LOAD16_BYTE( "st_210.t",   0x40001, 0x10000, 0x1107499c )
	ROM_LOAD16_BYTE( "st_200.u",   0x60000, 0x10000, 0x78e72af9 )
	ROM_LOAD16_BYTE( "st_210.u",   0x60001, 0x10000, 0x420be93b )
	ROM_LOAD16_BYTE( "st_200.v",   0x80000, 0x10000, 0x7eff9f8b )
	ROM_LOAD16_BYTE( "st_210.v",   0x80001, 0x10000, 0x53e9fe94 )
	ROM_LOAD16_BYTE( "st_200.w",   0xa0000, 0x10000, 0xd39e8cef )
	ROM_LOAD16_BYTE( "st_210.w",   0xa0001, 0x10000, 0xb388bf91 )
	ROM_LOAD16_BYTE( "st_200.x",   0xc0000, 0x10000, 0x9f047de7 )
	ROM_LOAD16_BYTE( "st_210.x",   0xc0001, 0x10000, 0xf6b99901 )
	ROM_LOAD16_BYTE( "st_200.y",   0xe0000, 0x10000, 0xdb62362e )
	ROM_LOAD16_BYTE( "st_210.y",   0xe0001, 0x10000, 0xef517db7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "stsnd.1f", 0x10000, 0x4000, 0xc52d8218 )
	ROM_CONTINUE(         0x04000, 0xc000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "stdspcom.5f",  0x00000, 0x10000, 0x4c645933 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "stds3.2lm", 0x00000, 0x20000, 0x0a29db30 )
	ROM_LOAD16_BYTE( "stds3.2t",  0x00001, 0x20000, 0xa5882384 )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "stsnd.1m",  0x80000, 0x20000, 0xc904db9c )
	ROM_LOAD( "stsnd.1n",  0xa0000, 0x20000, 0x164580b3 )
	ROM_LOAD( "stsnd.1p",  0xc0000, 0x20000, 0x296290a0 )
	ROM_LOAD( "stsnd.1r",  0xe0000, 0x20000, 0xc029d037 )
ROM_END


ROM_START( racedriv )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "077-3001", 0x00001, 0x10000, 0xc75373a4 )
	ROM_LOAD16_BYTE( "077-3002", 0x00000, 0x10000, 0x78771253 )
	ROM_LOAD16_BYTE( "077-2003", 0x20001, 0x10000, 0x8c36b745 )
	ROM_LOAD16_BYTE( "077-2004", 0x20000, 0x10000, 0x4eb19582 )
	ROM_LOAD16_BYTE( "077-2005", 0x40001, 0x10000, 0x71c0a770 )
	ROM_LOAD16_BYTE( "077-2006", 0x40000, 0x10000, 0x07fd762e )
	ROM_LOAD16_BYTE( "077-2007", 0x60001, 0x10000, 0x17903148 )
	ROM_LOAD16_BYTE( "077-2008", 0x60000, 0x10000, 0x5144d31b )
	ROM_LOAD16_BYTE( "077-2009", 0x80001, 0x10000, 0x1e9e4c31 )
	ROM_LOAD16_BYTE( "077-2010", 0x80000, 0x10000, 0x8674e44e )
	ROM_LOAD16_BYTE( "rd1011.bin", 0xa0001, 0x10000, 0xc5cd5491 )
	ROM_LOAD16_BYTE( "rd1012.bin", 0xa0000, 0x10000, 0x9a78b952 )
	ROM_LOAD16_BYTE( "rd1013.bin", 0xc0001, 0x10000, 0xca7b3e53 )
	ROM_LOAD16_BYTE( "rd1014.bin", 0xc0000, 0x10000, 0xa872792a )
	ROM_LOAD16_BYTE( "077-1015", 0xe0001, 0x10000, 0xc51f2702 )
	ROM_LOAD16_BYTE( "077-1016", 0xe0000, 0x10000, 0xe83a9c99 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x10000, REGION_CPU5, 0 )		/* dummy region for DSP32 */

	ROM_REGION( 0x50000, REGION_CPU6, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "rd1032.bin", 0x00001, 0x10000, 0x33005f2a )
	ROM_LOAD16_BYTE( "rd1033.bin", 0x00000, 0x10000, 0x4fc800ac )

	ROM_REGION( 0x10000, REGION_CPU7, 0 )		/* dummy region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "rd2021.bin", 0x00000, 0x10000, 0x8b2a98da )
	ROM_LOAD16_BYTE( "rd2023.bin", 0x00001, 0x10000, 0xc6d83d38 )
	ROM_LOAD16_BYTE( "rd2022.bin", 0x20000, 0x10000, 0xc0393c31 )
	ROM_LOAD16_BYTE( "rd2024.bin", 0x20001, 0x10000, 0x1e2fb25f )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION16_BE( 0x31000, REGION_USER3, 0 )		/* 128k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "rd1030.bin", 0x00000, 0x10000, 0x31a600db )
	ROM_LOAD16_BYTE( "rd1031.bin", 0x00001, 0x10000, 0x059c410b )

	ROM_REGION16_BE( 0x120000, REGION_SOUND1, 0 )	/* 10*128k for audio serial ROMs */
	ROM_LOAD16_BYTE( "rd1123.bin", 0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD16_BYTE( "rd1124.bin", 0x20000, 0x10000, 0x071a4309 )
	ROM_LOAD16_BYTE( "rd3125.bin", 0x40000, 0x10000, 0x856548ff )
	ROM_LOAD16_BYTE( "rd1126.bin", 0x60000, 0x10000, 0xf46ef09c )
	ROM_LOAD16_BYTE( "rd1017.bin",0x100000, 0x10000, 0xe93129a3 )
ROM_END


ROM_START( racedrvc )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "rd2001.bin", 0x00001, 0x10000, 0x9312fd5f )
	ROM_LOAD16_BYTE( "rd2002.bin", 0x00000, 0x10000, 0x669fe6fe )
	ROM_LOAD16_BYTE( "rd1003.bin", 0x20001, 0x10000, 0x5131ee87 )
	ROM_LOAD16_BYTE( "rd1004.bin", 0x20000, 0x10000, 0xf937ce55 )
	ROM_LOAD16_BYTE( "rd1005.bin", 0x40001, 0x10000, 0x627c4294 )
	ROM_LOAD16_BYTE( "rd1006.bin", 0x40000, 0x10000, 0xfe09241e )
	ROM_LOAD16_BYTE( "rd1007.bin", 0x60001, 0x10000, 0x2517035a )
	ROM_LOAD16_BYTE( "rd1008.bin", 0x60000, 0x10000, 0x0540d53d )
	ROM_LOAD16_BYTE( "rd1009.bin", 0x80001, 0x10000, 0x33556cb5 )
	ROM_LOAD16_BYTE( "rd1010.bin", 0x80000, 0x10000, 0x84329826 )
	ROM_LOAD16_BYTE( "rd1011.bin", 0xa0001, 0x10000, 0xc5cd5491 )
	ROM_LOAD16_BYTE( "rd1012.bin", 0xa0000, 0x10000, 0x9a78b952 )
	ROM_LOAD16_BYTE( "rd1013.bin", 0xc0001, 0x10000, 0xca7b3e53 )
	ROM_LOAD16_BYTE( "rd1014.bin", 0xc0000, 0x10000, 0xa872792a )
	ROM_LOAD16_BYTE( "rd1015.bin", 0xe0001, 0x10000, 0x64dd6040 )
	ROM_LOAD16_BYTE( "rd1016.bin", 0xe0000, 0x10000, 0xa2a0ed28 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x10000, REGION_CPU5, 0 )		/* dummy region for DSP32 */

	ROM_REGION( 0x50000, REGION_CPU6, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "rd1032.bin", 0x00001, 0x10000, 0x33005f2a )
	ROM_LOAD16_BYTE( "rd1033.bin", 0x00000, 0x10000, 0x4fc800ac )

	ROM_REGION( 0x10000, REGION_CPU7, 0 )		/* dummy region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */
	ROM_LOAD16_BYTE( "rd2021.bin", 0x00000, 0x10000, 0x8b2a98da )
	ROM_LOAD16_BYTE( "rd2023.bin", 0x00001, 0x10000, 0xc6d83d38 )
	ROM_LOAD16_BYTE( "rd2022.bin", 0x20000, 0x10000, 0xc0393c31 )
	ROM_LOAD16_BYTE( "rd2024.bin", 0x20001, 0x10000, 0x1e2fb25f )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION16_BE( 0x20000, REGION_USER3, 0 )		/* 128k for DSK ROMs */
	ROM_LOAD16_BYTE( "rd1030.bin", 0x00000, 0x10000, 0x31a600db )
	ROM_LOAD16_BYTE( "rd1031.bin", 0x00001, 0x10000, 0x059c410b )

	ROM_REGION16_BE( 0x120000, REGION_SOUND1, 0 )	/* 10*128k for audio serial ROMs */
	ROM_LOAD16_BYTE( "rd1123.bin", 0x00000, 0x10000, 0xa88411dc )
	ROM_LOAD16_BYTE( "rd1124.bin", 0x20000, 0x10000, 0x071a4309 )
	ROM_LOAD16_BYTE( "rd3125.bin", 0x40000, 0x10000, 0x856548ff )
	ROM_LOAD16_BYTE( "rd1126.bin", 0x60000, 0x10000, 0xf46ef09c )
	ROM_LOAD16_BYTE( "rd1017.bin",0x100000, 0x10000, 0xe93129a3 )
ROM_END


ROM_START( hdrivair )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "stestlo.bin", 0x000001, 0x20000, 0x58758419 )
	ROM_LOAD16_BYTE( "stesthi.bin", 0x000000, 0x20000, 0xb4bfa451 )
	ROM_LOAD16_BYTE( "drivelo.bin", 0x040001, 0x20000, 0x34adf4af )
	ROM_LOAD16_BYTE( "drivehi.bin", 0x040000, 0x20000, 0xd15f5119 )
	ROM_LOAD16_BYTE( "wavelo.bin",  0x0c0001, 0x20000, 0x15ed4394 )
	ROM_LOAD16_BYTE( "wavehi.bin",  0x0c0000, 0x20000, 0xb21a34b6 )
	ROM_LOAD16_BYTE( "ms2pics.lo",  0x140001, 0x20000, 0xc3c6be8c )
	ROM_LOAD16_BYTE( "ms2pics.hi",  0x140000, 0x20000, 0xbca0af7e )
	ROM_LOAD16_BYTE( "univlo.bin",  0x180001, 0x20000, 0x22d3b699 )
	ROM_LOAD16_BYTE( "univhi.bin",  0x180000, 0x20000, 0x86351673 )
	ROM_LOAD16_BYTE( "coproclo.bin",0x1c0001, 0x20000, 0x5f98b04d )
	ROM_LOAD16_BYTE( "coprochi.bin",0x1c0000, 0x20000, 0x5d2ca109 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x50000, REGION_CPU5, 0 )		/* 5*64k for 32C010 code */

	ROM_REGION( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION16_BE( 0x100000, REGION_USER3, 0 )	/* unknown ROMs */
	ROM_LOAD16_BYTE( "ds3rom0.bin", 0x00001, 0x80000, 0x90b8dbb6 )
	ROM_LOAD16_BYTE( "ds3rom1.bin", 0x00000, 0x80000, 0x58173812 )
	ROM_LOAD16_BYTE( "ds3rom2.bin", 0x00001, 0x80000, 0x5a4b18fa )
	ROM_LOAD16_BYTE( "ds3rom3.bin", 0x00000, 0x80000, 0x63965868 )
	ROM_LOAD16_BYTE( "ds3rom4.bin", 0x00001, 0x80000, 0x15ffb19a )
	ROM_LOAD16_BYTE( "ds3rom5.bin", 0x00000, 0x80000, 0x8d0e9b27 )
	ROM_LOAD16_BYTE( "ds3rom6.bin", 0x00001, 0x80000, 0xce7edbae )
	ROM_LOAD16_BYTE( "ds3rom7.bin", 0x00000, 0x80000, 0x323eff0b )
	ROM_LOAD16_BYTE( "dsk2plo.bin", 0x00001, 0x80000, 0xedf96363 )
	ROM_LOAD16_BYTE( "dsk2phi.bin", 0x00000, 0x80000, 0x71c268e0 )
	ROM_LOAD16_BYTE( "obj0l.bin",   0x00001, 0x20000, 0x1f835f2e )
	ROM_LOAD16_BYTE( "obj0h.bin",   0x00000, 0x20000, 0xc321ab55 )
	ROM_LOAD16_BYTE( "obj1l.bin",   0x00001, 0x20000, 0x3d65f264 )
	ROM_LOAD16_BYTE( "obj1h.bin",   0x00000, 0x20000, 0x2c06b708 )
	ROM_LOAD16_BYTE( "obj2l.bin",   0x00001, 0x20000, 0xb206cc7e )
	ROM_LOAD16_BYTE( "obj2h.bin",   0x00000, 0x20000, 0xa666e98c )
	ROM_LOAD16_BYTE( "roads0.bin",  0x00001, 0x80000, 0x5028eb41 )
	ROM_LOAD16_BYTE( "roads1.bin",  0x00000, 0x80000, 0xc3f2c201 )
	ROM_LOAD16_BYTE( "roads2.bin",  0x00001, 0x80000, 0x527923fe )
	ROM_LOAD16_BYTE( "roads3.bin",  0x00000, 0x80000, 0x2f2023b2 )
	ROM_LOAD     ( "sboot.bin",   0x00000, 0x10000, 0xcde4d010 )
	ROM_LOAD     ( "xboot.bin",   0x00000, 0x10000, 0x054b46a0 )
ROM_END


ROM_START( hdrivaip )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "stest.0l",    0x000001, 0x20000, 0xf462b511 )
	ROM_LOAD16_BYTE( "stest.0h",    0x000000, 0x20000, 0xbf4bb6a0 )
	ROM_LOAD16_BYTE( "drive.lo",    0x040001, 0x20000, 0x799e3138 )
	ROM_LOAD16_BYTE( "drive.hi",    0x040000, 0x20000, 0x56571590 )
	ROM_LOAD16_BYTE( "wave1.lo",    0x0c0001, 0x20000, 0x1a472475 )
	ROM_LOAD16_BYTE( "wave1.hi",    0x0c0000, 0x20000, 0x63872d12 )
	ROM_LOAD16_BYTE( "ms2pics.lo",  0x140001, 0x20000, 0xc3c6be8c )
	ROM_LOAD16_BYTE( "ms2pics.hi",  0x140000, 0x20000, 0xbca0af7e )
	ROM_LOAD16_BYTE( "ms2univ.lo",  0x180001, 0x20000, 0x7493bf60 )
	ROM_LOAD16_BYTE( "ms2univ.hi",  0x180000, 0x20000, 0x59c91b15 )
	ROM_LOAD16_BYTE( "ms2cproc.0l", 0x1c0001, 0x20000, 0x1e48bd46 )
	ROM_LOAD16_BYTE( "ms2cproc.0h", 0x1c0000, 0x20000, 0x19024f2d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* dummy region for GSP 34010 */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )		/* dummy region for MSP 34010 */

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )/* dummy region for ADSP 2100 */

	ROM_REGION( 0x50000, REGION_CPU5, 0 )		/* 5*64k for 32C010 code */

	ROM_REGION( 0x60000, REGION_USER1, 0 )		/* 384k for object ROM */

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for I/O buffers */

	ROM_REGION16_BE( 0x100000, REGION_USER3, 0 )	/* unknown ROMs */
	ROM_LOAD16_BYTE( "ds3rom.0",    0x00001, 0x80000, 0x90b8dbb6 )
	ROM_LOAD16_BYTE( "ds3rom.1",    0x00000, 0x80000, 0x03673d8d )
	ROM_LOAD16_BYTE( "ds3rom.2",    0x00001, 0x80000, 0xf67754e9 )
	ROM_LOAD16_BYTE( "ds3rom.3",    0x00000, 0x80000, 0x008d3578 )
	ROM_LOAD16_BYTE( "ds3rom.4",    0x00001, 0x80000, 0x6281efee )
	ROM_LOAD16_BYTE( "ds3rom.5",    0x00000, 0x80000, 0x6ef9ed90 )
	ROM_LOAD16_BYTE( "ds3rom.6",    0x00001, 0x80000, 0xcd4cd6bc )
	ROM_LOAD16_BYTE( "ds3rom.7",    0x00000, 0x80000, 0x3d695e1f )
	ROM_LOAD16_BYTE( "dskpics.lo",  0x00001, 0x80000, 0x8c6f0750 )
	ROM_LOAD16_BYTE( "dskpics.hi",  0x00000, 0x80000, 0xeaa88101 )
	ROM_LOAD16_BYTE( "objects.0l",  0x00001, 0x20000, 0x3c9e9078 )
	ROM_LOAD16_BYTE( "objects.0h",  0x00000, 0x20000, 0x4480dbae )
	ROM_LOAD16_BYTE( "objects.1l",  0x00001, 0x20000, 0x700bd978 )
	ROM_LOAD16_BYTE( "objects.1h",  0x00000, 0x20000, 0xf613adaf )
	ROM_LOAD16_BYTE( "objects.2l",  0x00001, 0x20000, 0xe3b512f0 )
	ROM_LOAD16_BYTE( "objects.2h",  0x00000, 0x20000, 0x3f83742b )
	ROM_LOAD16_BYTE( "roads.0",     0x00001, 0x80000, 0xcab2e335 )
	ROM_LOAD16_BYTE( "roads.1",     0x00000, 0x80000, 0x62c244ba )
	ROM_LOAD16_BYTE( "roads.2",     0x00001, 0x80000, 0xba57f415 )
	ROM_LOAD16_BYTE( "roads.3",     0x00000, 0x80000, 0x1e6a4ca0 )
	ROM_LOAD     ( "sboota.bin",  0x00000, 0x10000, 0x3ef819cd )
	ROM_LOAD     ( "xboota.bin",  0x00000, 0x10000, 0xd9c49901 )
ROM_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1988, harddriv, 0,        harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin'", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1990, harddrvc, harddriv, racedriv, racedriv, harddrvc, ROT0, "Atari Games", "Hard Drivin' (compact)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAME ( 1989, stunrun,  0,        stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner" )
GAME ( 1989, stunrnp,  stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (prototype)" )
GAMEX( 1990, racedriv, 0,        harddriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (upgrade)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1990, racedrvc, racedriv, racedriv, racedriv, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1990, steeltal, 0,        steeltal, steeltal, steeltal, ROT0, "Atari Games", "Steel Talons", GAME_NOT_WORKING )
GAMEX( 1990, steeltdb, steeltal, steeltal, steeltal, steeltal, ROT0, "Atari Games", "Steel Talons (debug)", GAME_NOT_WORKING )
GAMEX( 1993, hdrivair, 0,        steeltal, hdrivair, hdrivair, ROT0, "Atari Games", "Hard Drivin's Airborne (prototype)", GAME_NOT_WORKING )
GAMEX( 1993, hdrivaip, hdrivair, steeltal, hdrivair, hdrivair, ROT0, "Atari Games", "Hard Drivin's Airborne (prototype, early rev)", GAME_NOT_WORKING )
