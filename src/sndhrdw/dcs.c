/***************************************************************************

	Midway DCS Audio Board

****************************************************************************

	There are several variations of this board, which was in use by
	Midway and eventually Atari for almost 10 years.
	
	DCS ROM-based mono:
		* ADSP-2105 @ 10MHz
		* single channel output
		* ROM-based, up to 8MB total
		* used in:
			Mortal Kombat 2 (1993)
			Mortal Kombat 3 (1994)
			Ultimate Mortal Kombat 3 (1994)
			Cruisin' USA (1994)
			Revolution X (1994)
			Killer Instinct (1994)
			2 On 2 Open Ice Challenge (1995)
			WWF Wrestlemania (1995)
			Killer Instinct 2 (1995)
			NBA Hangtime (1996)
			NBA Maximum Hangtime (1996)
			Cruisin' World (1996)
			Rampage World Tour (1997)
			Offroad Challenge (1997)
	
	DCS RAM-based stereo (Seattle):
		* ADSP-2115 @ 16MHz
		* dual channel output (stereo)
		* RAM-based, 2-4MB total
		* used in:
			War Gods (1995)
			Wayne Gretzky's 3D Hockey (1996)
			Mace: The Dark Age (1996)
			Biofreaks (1997)
			NFL Blitz (1997)
			California Speed (1998)
			Vapor TRX (1998)
			NFL Blitz '99 (1998)
			CarnEvil (1998)
			Hyperdrive (1998)
			NFL Blitz 2000 Gold (1999)
			
	DCS ROM-based stereo (Zeus):
		* ADSP-2104 @ 16MHz
		* dual channel output (stereo)
		* ROM-based, up to 12MB total
		* used in:
			Mortal Kombat 4 (1997)
			Invasion (1999)
			Cruisin' Exotica (1999)
			The Grid (2001)
	
	DCS RAM-based stereo (Vegas):
		* ADSP-2104 @ 16MHz
		* dual channel output (stereo)
		* RAM-based, 4MB total
		* used in:
			Gauntlet Legends (1998)
			Tenth Degree (1998)
			Gauntlet Dark Legacy (1999)
			War: The Final Assault (1999)
	
	ADAGE/ADCS RAM-based multichannel:
		* ADSP-2181 @ 16.667MHz
		* 2-6 channel output
		* RAM-based, 4MB total
		* used in:
			San Francisco Rush: 2049 (1998)
			Road Burners (1999)
			
	Unknown other DCS boards:
		* NBA Showtime
		* NBA Showtime / NFL Blitz 2000 Gold
		* Cart Fury

****************************************************************************/

#include "driver.h"
#include "cpu/adsp2100/adsp2100.h"
#include "dcs.h"
#include "sound/dmadac.h"

#include <math.h>


#define LOG_DCS_TRANSFERS			(0)
#define LOG_DCS_IO					(0)
#define LOG_BUFFER_FILLING			(0)

#define HLE_TRANSFERS				(1)



/***************************************************************************
	CONSTANTS
****************************************************************************/

#define LCTRL_OUTPUT_EMPTY			0x400
#define LCTRL_INPUT_EMPTY			0x800

#define IS_OUTPUT_EMPTY()			(dcs.latch_control & LCTRL_OUTPUT_EMPTY)
#define IS_OUTPUT_FULL()			(!(dcs.latch_control & LCTRL_OUTPUT_EMPTY))
#define SET_OUTPUT_EMPTY()			(dcs.latch_control |= LCTRL_OUTPUT_EMPTY)
#define SET_OUTPUT_FULL()			(dcs.latch_control &= ~LCTRL_OUTPUT_EMPTY)

#define IS_INPUT_EMPTY()			(dcs.latch_control & LCTRL_INPUT_EMPTY)
#define IS_INPUT_FULL()				(!(dcs.latch_control & LCTRL_INPUT_EMPTY))
#define SET_INPUT_EMPTY()			(dcs.latch_control |= LCTRL_INPUT_EMPTY)
#define SET_INPUT_FULL()			(dcs.latch_control &= ~LCTRL_INPUT_EMPTY)

/* These are the some of the control register, we dont use them all */
enum
{
	S1_AUTOBUF_REG = 15,
	S1_RFSDIV_REG,
	S1_SCLKDIV_REG,
	S1_CONTROL_REG,
	S0_AUTOBUF_REG,
	S0_RFSDIV_REG,
	S0_SCLKDIV_REG,
	S0_CONTROL_REG,
	S0_MCTXLO_REG,
	S0_MCTXHI_REG,
	S0_MCRXLO_REG,
	S0_MCRXHI_REG,
	TIMER_SCALE_REG,
	TIMER_COUNT_REG,
	TIMER_PERIOD_REG,
	WAITSTATES_REG,
	SYSCONTROL_REG
};



/***************************************************************************
	STRUCTURES
****************************************************************************/

struct dcs_state
{
	UINT8	auto_ack;
	UINT8	channels;

	UINT16 *soundboot;
	UINT16 *sounddata;
	UINT16	size;
	UINT16	incs;
	void  * reg_timer;
	void  * sport_timer;
	int		ireg;
	UINT16	ireg_base;
	UINT16	control_regs[32];

	UINT16	databank;
	UINT16	databank_count;
	UINT16	srambank;

	UINT16	latch_control;
	UINT16	input_data;
	UINT16	output_data;
	UINT16	output_control;
	UINT32	output_control_cycles;

	UINT8	last_output_full;
	UINT8	last_input_empty;

	void	(*output_full_cb)(int);
	void	(*input_empty_cb)(int);
	UINT16	(*fifo_data_r)(void);
	UINT16	(*fifo_status_r)(void);
};



/***************************************************************************
	STATIC GLOBALS
****************************************************************************/

static INT8 dcs_cpunum;

static struct dcs_state dcs;

static data16_t *dcs_sram_bank0;
static data16_t *dcs_sram_bank1;

static data16_t *dcs_polling_base;

static data16_t *dcs_data_ram;
static data32_t *dcs_program_ram;

static size_t bank20_size;
static size_t bootbank_stride;

static int dcs_state;
static int transfer_state;
static int transfer_start;
static int transfer_stop;
static int transfer_type;
static int transfer_temp;
static int transfer_writes_left;
static UINT16 transfer_sum;
static int transfer_fifo_entries;
static mame_timer *transfer_watchdog;



/***************************************************************************
	PROTOTYPES
****************************************************************************/

static READ16_HANDLER( dcs_sdrc_asic_ver_r );

static WRITE16_HANDLER( dcs_data_bank_select_w );
static READ16_HANDLER( dcs_data_bank_select_r );
static WRITE16_HANDLER( dcs2_sram_bank_w );
static READ16_HANDLER( dcs2_sram_bank_r );

static WRITE16_HANDLER( dcs_control_w );

static READ16_HANDLER( latch_status_r );
static READ16_HANDLER( fifo_input_r );
static READ16_HANDLER( input_latch_r );
static WRITE16_HANDLER( input_latch_ack_w );
static WRITE16_HANDLER( output_latch_w );
static READ16_HANDLER( output_control_r );
static WRITE16_HANDLER( output_control_w );

static void dcs_irq(int state);
static void sport0_irq(int state);
static void sound_tx_callback(int port, INT32 data);

static READ16_HANDLER( dcs_polling_r );

static void transfer_watchdog_callback(int param);
static int preprocess_write(data16_t data);



/***************************************************************************
	PROCESSOR STRUCTURES
****************************************************************************/

/* DCS readmem/writemem structures */
ADDRESS_MAP_START( dcs_program_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM	AM_BASE(&dcs_program_ram)		/* internal/external program ram */
ADDRESS_MAP_END


ADDRESS_MAP_START( dcs_data_map, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&dcs_data_ram)		/* ??? */
	AM_RANGE(0x2000, 0x2fff) AM_ROMBANK(20) AM_SIZE(&bank20_size)/* banked roms read */
	AM_RANGE(0x3000, 0x3000) AM_WRITE(dcs_data_bank_select_w)	/* bank selector */
	AM_RANGE(0x3400, 0x3403) AM_READWRITE(input_latch_r, output_latch_w) /* soundlatch read/write */
	AM_RANGE(0x3800, 0x39ff) AM_RAM								/* internal data ram */
	AM_RANGE(0x3fe0, 0x3fff) AM_WRITE(dcs_control_w)			/* adsp control regs */
ADDRESS_MAP_END


/* DCS with UART readmem/writemem structures */
ADDRESS_MAP_START( dcs_uart_data_map, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&dcs_data_ram)		/* ??? */
	AM_RANGE(0x2000, 0x2fff) AM_ROMBANK(20) AM_SIZE(&bank20_size)/* banked roms read */
	AM_RANGE(0x3000, 0x3000) AM_WRITE(dcs_data_bank_select_w)	/* bank selector */
	AM_RANGE(0x3400, 0x3402) AM_READNOP							/* UART (ignored) */
	AM_RANGE(0x3400, 0x3402) AM_WRITENOP						/* UART (ignored) */
	AM_RANGE(0x3403, 0x3403) AM_READWRITE(input_latch_r, output_latch_w) /* soundlatch read/write */
	AM_RANGE(0x3404, 0x3405) AM_NOP								/* UART (ignored) */
	AM_RANGE(0x3800, 0x39ff) AM_RAM								/* internal data ram */
	AM_RANGE(0x3fe0, 0x3fff) AM_WRITE(dcs_control_w)			/* adsp control regs */
ADDRESS_MAP_END



/* DCS2-based readmem/writemem structures */
ADDRESS_MAP_START( dcs2_program_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x3fff) AM_RAM	AM_BASE(&dcs_program_ram) /* internal/external program ram */
ADDRESS_MAP_END


ADDRESS_MAP_START( dcs2_data_map, ADDRESS_SPACE_DATA, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x03ff) AM_RAMBANK(20) AM_SIZE(&bank20_size)	AM_BASE(&dcs_data_ram) /* D/RAM */
	AM_RANGE(0x0400, 0x0400) AM_READWRITE(input_latch_r, input_latch_ack_w) /* input latch read */
	AM_RANGE(0x0401, 0x0401) AM_WRITE(output_latch_w)			/* soundlatch write */
	AM_RANGE(0x0402, 0x0402) AM_READWRITE(output_control_r, output_control_w) /* secondary soundlatch read */
	AM_RANGE(0x0403, 0x0403) AM_READ(latch_status_r)			/* latch status read */
	AM_RANGE(0x0404, 0x0407) AM_READ(fifo_input_r)				/* FIFO input read */
	AM_RANGE(0x0480, 0x0480) AM_READWRITE(dcs2_sram_bank_r, dcs2_sram_bank_w) /* S/RAM bank */
	AM_RANGE(0x0481, 0x0481) AM_NOP								/* LED in bit $2000 */
	AM_RANGE(0x0482, 0x0482) AM_READWRITE(dcs_data_bank_select_r, dcs_data_bank_select_w) /* D/RAM bank */
	AM_RANGE(0x0483, 0x0483) AM_READ(dcs_sdrc_asic_ver_r)		/* SDRC version number */
	AM_RANGE(0x0800, 0x17ff) AM_RAM								/* S/RAM */
	AM_RANGE(0x1800, 0x27ff) AM_RAMBANK(21) AM_BASE(&dcs_sram_bank0) /* banked S/RAM */
	AM_RANGE(0x2800, 0x37ff) AM_RAM								/* S/RAM */
	AM_RANGE(0x3800, 0x39ff) AM_RAM								/* internal data ram */
	AM_RANGE(0x3a00, 0x3a00) AM_READNOP							/* controls final sample rate */
	AM_RANGE(0x3fe0, 0x3fff) AM_WRITE(dcs_control_w)			/* adsp control regs */
ADDRESS_MAP_END


ADDRESS_MAP_START( dcs2_rom_data_map, ADDRESS_SPACE_DATA, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x03ff) AM_ROMBANK(20) AM_SIZE(&bank20_size) AM_BASE(&dcs_data_ram) /* banked ROM */
	AM_RANGE(0x0400, 0x0400) AM_READWRITE(input_latch_r, input_latch_ack_w) /* input latch read */
	AM_RANGE(0x0401, 0x0401) AM_WRITE(output_latch_w)			/* soundlatch write */
	AM_RANGE(0x0402, 0x0402) AM_READWRITE(output_control_r, output_control_w) /* secondary soundlatch read */
	AM_RANGE(0x0403, 0x0403) AM_READ(latch_status_r)			/* latch status read */
	AM_RANGE(0x0404, 0x0407) AM_READ(fifo_input_r)				/* FIFO input read */
	AM_RANGE(0x0480, 0x0480) AM_READWRITE(dcs2_sram_bank_r, dcs2_sram_bank_w) /* S/RAM bank */
	AM_RANGE(0x0481, 0x0481) AM_NOP								/* LED in bit $2000 */
	AM_RANGE(0x0482, 0x0482) AM_READWRITE(dcs_data_bank_select_r, dcs_data_bank_select_w) /* ROM bank */
	AM_RANGE(0x0483, 0x0483) AM_READ(dcs_sdrc_asic_ver_r)		/* SDRC version number */
	AM_RANGE(0x0800, 0x17ff) AM_RAM								/* S/RAM */
	AM_RANGE(0x1800, 0x27ff) AM_RAMBANK(21) AM_BASE(&dcs_sram_bank0) /* banked S/RAM */
	AM_RANGE(0x2800, 0x37ff) AM_RAM								/* S/RAM */
	AM_RANGE(0x3800, 0x39ff) AM_RAM								/* internal data ram */
	AM_RANGE(0x3a00, 0x3a00) AM_READNOP							/* controls final sample rate */
	AM_RANGE(0x3fe0, 0x3fff) AM_WRITE(dcs_control_w)			/* adsp control regs */
ADDRESS_MAP_END



/* DCS3-based readmem/writemem structures */
ADDRESS_MAP_START( dcs3_program_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x3fff) AM_RAM	AM_BASE(&dcs_program_ram) /* internal/external program ram */
ADDRESS_MAP_END


ADDRESS_MAP_START( dcs3_data_map, ADDRESS_SPACE_DATA, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x03ff) AM_RAMBANK(20) AM_SIZE(&bank20_size) AM_BASE(&dcs_data_ram) /* D/RAM */
	AM_RANGE(0x0400, 0x0400) AM_READWRITE(input_latch_r, input_latch_ack_w) /* input latch read */
	AM_RANGE(0x0401, 0x0401) AM_WRITE(output_latch_w)			/* soundlatch write */
	AM_RANGE(0x0402, 0x0402) AM_READWRITE(output_control_r, output_control_w) /* secondary soundlatch read */
	AM_RANGE(0x0403, 0x0403) AM_READ(latch_status_r)			/* latch status read */
	AM_RANGE(0x0404, 0x0407) AM_READ(fifo_input_r)				/* FIFO input read */
	AM_RANGE(0x0480, 0x0480) AM_READWRITE(dcs2_sram_bank_r, dcs2_sram_bank_w) /* S/RAM bank */
	AM_RANGE(0x0481, 0x0481) AM_NOP								/* LED in bit $2000 */
	AM_RANGE(0x0482, 0x0482) AM_READWRITE(dcs_data_bank_select_r, dcs_data_bank_select_w) /* D/RAM bank */
	AM_RANGE(0x0483, 0x0483) AM_READ(dcs_sdrc_asic_ver_r)		/* SDRC version number */
	AM_RANGE(0x0800, 0x17ff) AM_RAM								/* S/RAM */
	AM_RANGE(0x1800, 0x27ff) AM_RAMBANK(21) AM_BASE(&dcs_sram_bank0) /* banked S/RAM */
	AM_RANGE(0x2800, 0x37ff) AM_RAM								/* S/RAM */
	AM_RANGE(0x3800, 0x3fdf) AM_RAM								/* internal data ram */
	AM_RANGE(0x3fe0, 0x3fff) AM_WRITE(dcs_control_w)			/* adsp control regs */
ADDRESS_MAP_END



/***************************************************************************
	MACHINE DRIVERS
****************************************************************************/

MACHINE_DRIVER_START( dcs_audio )
	MDRV_CPU_ADD_TAG("dcs", ADSP2105, 10000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(dcs_program_map,0)
	MDRV_CPU_DATA_MAP(dcs_data_map,0)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs_audio_uart )
	MDRV_IMPORT_FROM(dcs_audio)

	MDRV_CPU_MODIFY("dcs")
	MDRV_CPU_DATA_MAP(dcs_uart_data_map,0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs2_audio )
	MDRV_CPU_ADD_TAG("dcs2", ADSP2115, 16000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(dcs2_program_map,0)
	MDRV_CPU_DATA_MAP(dcs2_data_map,0)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs2_audio_2104 )
	MDRV_IMPORT_FROM(dcs2_audio)
	MDRV_CPU_REPLACE("dcs2", ADSP2104, 16000000)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs2_audio_2104_rom )
	MDRV_IMPORT_FROM(dcs2_audio_2104)
	MDRV_CPU_MODIFY("dcs2")
	MDRV_CPU_DATA_MAP(dcs2_rom_data_map,0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs3_audio )
	MDRV_CPU_ADD_TAG("dcs3", ADSP2181, 33000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(dcs3_program_map,0)
	MDRV_CPU_DATA_MAP(dcs3_data_map,0)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
MACHINE_DRIVER_END



/***************************************************************************
	INITIALIZATION
****************************************************************************/

static void dcs_boot(void)
{
	UINT32 max_banks = dcs.databank_count * bank20_size / 0x1000;
	UINT8 buffer[0x1000];
	int i;
	
	for (i = 0; i < 0x1000; i++)
		buffer[i] = dcs.soundboot[(dcs.databank % max_banks) * 0x1000 + i];
	adsp2105_load_boot_data(buffer, dcs_program_ram);
}


static void dcs_reset(void)
{
	int i;

	/* initialize our state structure and install the transmit callback */
	dcs.size = 0;
	dcs.incs = 0;
	dcs.ireg = 0;

	/* initialize the ADSP control regs */
	for (i = 0; i < sizeof(dcs.control_regs) / sizeof(dcs.control_regs[0]); i++)
		dcs.control_regs[i] = 0;

	/* initialize banking */
	dcs.databank = 0;
	dcs.srambank = 0;
	cpu_setbank(20, dcs.sounddata);
	if (dcs_sram_bank0)
		cpu_setbank(21, dcs_sram_bank0);

	/* initialize the ADSP Tx callback */
	cpunum_set_info_fct(dcs_cpunum, CPUINFO_PTR_ADSP2100_TX_HANDLER, (genf *)sound_tx_callback);

	/* clear all interrupts */
	cpunum_set_input_line(dcs_cpunum, ADSP2105_IRQ0, CLEAR_LINE);
	cpunum_set_input_line(dcs_cpunum, ADSP2105_IRQ1, CLEAR_LINE);
	cpunum_set_input_line(dcs_cpunum, ADSP2105_IRQ2, CLEAR_LINE);

	/* initialize the comm bits */
	SET_INPUT_EMPTY();
	SET_OUTPUT_EMPTY();
	if (!dcs.last_input_empty && dcs.input_empty_cb)
		(*dcs.input_empty_cb)(dcs.last_input_empty = 1);
	if (dcs.last_output_full && dcs.output_full_cb)
		(*dcs.output_full_cb)(dcs.last_output_full = 0);

	/* boot */
	dcs.control_regs[SYSCONTROL_REG] = 0;
	dcs_boot();

	/* start the SPORT0 timer */
	if (dcs.sport_timer)
		timer_adjust(dcs.sport_timer, TIME_IN_HZ(1000), 0, TIME_IN_HZ(1000));

#if (LOG_DCS_TRANSFERS || HLE_TRANSFERS)
	dcs_state = transfer_state = 0;
#endif
}


void dcs_init(void)
{
	/* find the DCS CPU and the sound ROMs */
	dcs_cpunum = mame_find_cpu_index("dcs");
	dcs.channels = 1;
	dcs.soundboot = (UINT16 *)memory_region(REGION_SOUND1);
	dcs.sounddata = dcs.soundboot;
	dcs.databank_count = memory_region_length(REGION_SOUND1) / bank20_size;

	/* reset RAM-based variables */
	dcs_sram_bank0 = dcs_sram_bank1 = NULL;

	/* create the timer */
	dcs.reg_timer = timer_alloc(dcs_irq);
	dcs.sport_timer = NULL;

	/* disable notification by default */
	dcs.output_full_cb = NULL;
	dcs.input_empty_cb = NULL;

	/* non-RAM based automatically acks */
	dcs.auto_ack = 1;

	/* reset the system */
	dcs_reset();
}


void dcs2_init(offs_t polling_offset)
{
	/* find the DCS CPU and the sound ROMs */
	dcs_cpunum = mame_find_cpu_index("dcs2");
	if (dcs_cpunum == -1)
		dcs_cpunum = mame_find_cpu_index("dcs3");
	dcs.channels = 2;
	dcs.soundboot = (UINT16 *)memory_region(REGION_SOUND1);
	dcs.sounddata = dcs.soundboot + 0x10000/2;
	dcs.databank_count = (memory_region_length(REGION_SOUND1) - 0x10000) / bank20_size;

	/* borrow memory for the extra 8k */
	dcs_sram_bank1 = auto_malloc(0x1000*2);
	bootbank_stride = bank20_size*4;

	/* create the timer */
	dcs.reg_timer = timer_alloc(dcs_irq);
	dcs.sport_timer = timer_alloc(sport0_irq);

	/* RAM based doesn't do auto-ack, but it has a FIFO */
	dcs.auto_ack = 0;
	dcs.output_full_cb = NULL;
	dcs.input_empty_cb = NULL;
	dcs.fifo_data_r = NULL;
	dcs.fifo_status_r = NULL;

	/* install the speedup handler */
	if (polling_offset)
		dcs_polling_base = memory_install_read16_handler(dcs_cpunum, ADDRESS_SPACE_DATA, polling_offset, polling_offset, 0, 0, dcs_polling_r);

	/* allocate a watchdog timer for HLE transfers */
	if (HLE_TRANSFERS)
		transfer_watchdog = timer_alloc(transfer_watchdog_callback);

	/* reset the system */
	dcs_reset();
}


void dcs2_rom_init(offs_t polling_offset)
{
	/* find the DCS CPU and the sound ROMs */
	dcs_cpunum = mame_find_cpu_index("dcs2");
	dcs.channels = 2;
	dcs.soundboot = (UINT16 *)memory_region(REGION_SOUND1);
	dcs.sounddata = dcs.soundboot;
	dcs.databank_count = memory_region_length(REGION_SOUND1) / bank20_size;

	/* borrow memory for the extra 8k */
	dcs_sram_bank1 = auto_malloc(0x1000*2);
	bootbank_stride = bank20_size;

	/* create the timer */
	dcs.reg_timer = timer_alloc(dcs_irq);
	dcs.sport_timer = timer_alloc(sport0_irq);

	/* RAM based doesn't do auto-ack, but it has a FIFO */
	dcs.auto_ack = 0;
	dcs.output_full_cb = NULL;
	dcs.input_empty_cb = NULL;

	/* install the speedup handler */
	if (polling_offset)
		dcs_polling_base = memory_install_read16_handler(dcs_cpunum, ADDRESS_SPACE_DATA, polling_offset, polling_offset, 0, 0, dcs_polling_r);

	/* reset the system */
	dcs_reset();
}


void dcs3_init(offs_t polling_offset)
{
	dcs2_init(polling_offset);
}


void dcs_set_auto_ack(int state)
{
	dcs.auto_ack = state;
}



/***************************************************************************
	DCS ASIC VERSION
****************************************************************************/

static READ16_HANDLER( dcs_sdrc_asic_ver_r )
{
	return 0x5a03;
}



/***************************************************************************
	DCS ROM BANK SELECT
****************************************************************************/

static WRITE16_HANDLER( dcs_data_bank_select_w )
{
	dcs.databank = data;
	cpu_setbank(20, &dcs.sounddata[(dcs.databank % dcs.databank_count) * bank20_size/2]);

	/* bit 11 = sound board led */
#if 0
	set_led_status(2, data & 0x800);
#endif
}


static READ16_HANDLER( dcs_data_bank_select_r )
{
	return dcs.databank;
}



/***************************************************************************
	DCS STATIC RAM BANK SELECT
****************************************************************************/

static WRITE16_HANDLER( dcs2_sram_bank_w )
{
	COMBINE_DATA(&dcs.srambank);
	cpu_setbank(21, (dcs.srambank & 0x1000) ? dcs_sram_bank1 : dcs_sram_bank0);

	/* it appears that the Vegas games also access the boot ROM via this location */
	if (((dcs.srambank >> 7) & 7) == dcs.databank)
		cpu_setbank(20, &dcs.soundboot[dcs.databank * bootbank_stride/2]);
}


static READ16_HANDLER( dcs2_sram_bank_r )
{
	return dcs.srambank;
}



/***************************************************************************
	DCS COMMUNICATIONS
****************************************************************************/

void dcs_set_io_callbacks(void (*output_full_cb)(int), void (*input_empty_cb)(int))
{
	dcs.input_empty_cb = input_empty_cb;
	dcs.output_full_cb = output_full_cb;
}


void dcs_set_fifo_callbacks(UINT16 (*fifo_data_r)(void), UINT16 (*fifo_status_r)(void))
{
	dcs.fifo_data_r = fifo_data_r;
	dcs.fifo_status_r = fifo_status_r;
}


int dcs_control_r(void)
{
	/* only boost for DCS2 boards */
	if (!dcs.auto_ack && !HLE_TRANSFERS)
		cpu_boost_interleave(TIME_IN_USEC(0.5), TIME_IN_USEC(5));
	return dcs.latch_control;
}


void dcs_reset_w(int state)
{
	/* going high halts the CPU */
	if (state)
	{
		logerror("%08x: DCS reset = %d\n", activecpu_get_pc(), state);

		/* just run through the init code again */
		dcs_reset();
		cpunum_set_input_line(dcs_cpunum, INPUT_LINE_RESET, ASSERT_LINE);
	}

	/* going low resets and reactivates the CPU */
	else
		cpunum_set_input_line(dcs_cpunum, INPUT_LINE_RESET, CLEAR_LINE);
}


static READ16_HANDLER( latch_status_r )
{
	int result = 0;
	if (IS_INPUT_FULL())
		result |= 0x80;
	if (IS_OUTPUT_EMPTY())
		result |= 0x40;
	if (dcs.fifo_status_r && (!HLE_TRANSFERS || transfer_state == 0))
		result |= (*dcs.fifo_status_r)() & 0x38;
	if (HLE_TRANSFERS && transfer_state != 0)
		result |= 0x08;
	return result;
}


static READ16_HANDLER( fifo_input_r )
{
	if (dcs.fifo_data_r)
		return (*dcs.fifo_data_r)();
	else
		return 0xffff;
}



/***************************************************************************
	INPUT LATCH (data from host to DCS)
****************************************************************************/

static void dcs_delayed_data_w(int data)
{
	if (LOG_DCS_IO)
		logerror("%08X:dcs_data_w(%04X)\n", activecpu_get_pc(), data);

	/* boost the interleave temporarily */
	cpu_boost_interleave(TIME_IN_USEC(0.5), TIME_IN_USEC(5));

	/* set the IRQ line on the ADSP */
	cpunum_set_input_line(dcs_cpunum, ADSP2105_IRQ2, ASSERT_LINE);

	/* indicate we are no longer empty */
	if (dcs.last_input_empty && dcs.input_empty_cb)
		(*dcs.input_empty_cb)(dcs.last_input_empty = 0);
	SET_INPUT_FULL();

	/* set the data */
	dcs.input_data = data;
}


void dcs_data_w(int data)
{
	/* preprocess the write */
	if (preprocess_write(data))
		return;

	/* if we are DCS1, set a timer to latch the data */
	if (!dcs.sport_timer)
		timer_set(TIME_NOW, data, dcs_delayed_data_w);
	else
	 	dcs_delayed_data_w(data);
}


static WRITE16_HANDLER( input_latch_ack_w )
{
	if (!dcs.last_input_empty && dcs.input_empty_cb)
		(*dcs.input_empty_cb)(dcs.last_input_empty = 1);
	SET_INPUT_EMPTY();
	cpunum_set_input_line(dcs_cpunum, ADSP2105_IRQ2, CLEAR_LINE);
}


static READ16_HANDLER( input_latch_r )
{
	if (dcs.auto_ack)
		input_latch_ack_w(0,0,0);
	if (LOG_DCS_IO)
		logerror("%08X:input_latch_r(%04X)\n", activecpu_get_pc(), dcs.input_data);
	return dcs.input_data;
}



/***************************************************************************
	OUTPUT LATCH (data from DCS to host)
****************************************************************************/

static void latch_delayed_w(int data)
{
	if (!dcs.last_output_full && dcs.output_full_cb)
		(*dcs.output_full_cb)(dcs.last_output_full = 1);
	SET_OUTPUT_FULL();
	dcs.output_data = data;
}


static WRITE16_HANDLER( output_latch_w )
{
	if (LOG_DCS_IO)
		logerror("%08X:output_latch_w(%04X) (empty=%d)\n", activecpu_get_pc(), data, IS_OUTPUT_EMPTY());
	timer_set(TIME_NOW, data, latch_delayed_w);
}


static void delayed_ack_w(int param)
{
	SET_OUTPUT_EMPTY();
}


void dcs_ack_w(void)
{
	timer_set(TIME_NOW, 0, delayed_ack_w);
}


int dcs_data_r(void)
{
	/* data is actually only 8 bit (read from d8-d15) */
	if (dcs.last_output_full && dcs.output_full_cb)
		(*dcs.output_full_cb)(dcs.last_output_full = 0);
	if (dcs.auto_ack)
		delayed_ack_w(0);

	if (LOG_DCS_IO)
		logerror("%08X:dcs_data_r(%04X)\n", activecpu_get_pc(), dcs.output_data);
	return dcs.output_data;
}



/***************************************************************************
	OUTPUT CONTROL BITS (has 3 additional lines to the host)
****************************************************************************/

static void output_control_delayed_w(int data)
{
	if (LOG_DCS_IO)
		logerror("output_control = %04X\n", data);
	dcs.output_control = data;
	dcs.output_control_cycles = 0;
}


static WRITE16_HANDLER( output_control_w )
{
	if (LOG_DCS_IO)
		logerror("%04X:output_control = %04X\n", activecpu_get_pc(), data);
	timer_set(TIME_NOW, data, output_control_delayed_w);
}


static READ16_HANDLER( output_control_r )
{
	dcs.output_control_cycles = activecpu_gettotalcycles();
	return dcs.output_control;
}


int dcs_data2_r(void)
{
	return dcs.output_control;
}



/***************************************************************************
	ADSP CONTROL & TRANSMIT CALLBACK
****************************************************************************/

/*
	The ADSP2105 memory map when in boot rom mode is as follows:

	Program Memory:
	0x0000-0x03ff = Internal Program Ram (contents of boot rom gets copied here)
	0x0400-0x07ff = Reserved
	0x0800-0x3fff = External Program Ram

	Data Memory:
	0x0000-0x03ff = External Data - 0 Waitstates
	0x0400-0x07ff = External Data - 1 Waitstates
	0x0800-0x2fff = External Data - 2 Waitstates
	0x3000-0x33ff = External Data - 3 Waitstates
	0x3400-0x37ff = External Data - 4 Waitstates
	0x3800-0x39ff = Internal Data Ram
	0x3a00-0x3bff = Reserved (extra internal ram space on ADSP2101, etc)
	0x3c00-0x3fff = Memory Mapped control registers & reserved.
*/

static WRITE16_HANDLER( dcs_control_w )
{
	dcs.control_regs[offset] = data;
	switch (offset)
	{
		case SYSCONTROL_REG:
			if (data & 0x0200)
			{
				/* boot force */
				cpunum_set_input_line(dcs_cpunum, INPUT_LINE_RESET, PULSE_LINE);
				dcs_boot();
				dcs.control_regs[SYSCONTROL_REG] = 0;
			}

			/* see if SPORT1 got disabled */
			if ((data & 0x0800) == 0)
			{
				dmadac_enable(0, dcs.channels, 0);
				timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
			}
			break;

		case S1_AUTOBUF_REG:
			/* autobuffer off: nuke the timer, and disable the DAC */
			if ((data & 0x0002) == 0)
			{
				dmadac_enable(0, dcs.channels, 0);
				timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
			}
			break;

		case S1_CONTROL_REG:
			if (((data >> 4) & 3) == 2)
				logerror("Oh no!, the data is compresed with u-law encoding\n");
			if (((data >> 4) & 3) == 3)
				logerror("Oh no!, the data is compresed with A-law encoding\n");
			break;
	}
}



/***************************************************************************
	DCS IRQ GENERATION CALLBACKS
****************************************************************************/

static void dcs_irq(int state)
{
	/* get the index register */
	int reg = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);

	/* copy the current data into the buffer */
	if (dcs.incs)
		dmadac_transfer(0, dcs.channels, dcs.incs, dcs.channels * dcs.incs, (dcs.size / 2) / (dcs.channels * dcs.incs), (INT16 *)&dcs_data_ram[reg]);

	/* increment it */
	reg += dcs.size / 2;

	/* check for wrapping */
	if (reg >= dcs.ireg_base + dcs.size)
	{
		/* reset the base pointer */
		reg = dcs.ireg_base;

		/* generate the (internal, thats why the pulse) irq */
		cpunum_set_input_line(dcs_cpunum, ADSP2105_IRQ1, PULSE_LINE);
	}

	/* store it */
	cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, reg);
}


static void sport0_irq(int state)
{
	/* this latches internally, so we just pulse */
	/* note that there is non-interrupt code that reads/modifies/writes the output_control */
	/* register; if we don't interlock it, we will eventually lose sound (see CarnEvil) */
	/* so we skip the SPORT interrupt if we read with output_control within the last 5 cycles */
	if ((cpunum_gettotalcycles(dcs_cpunum) - dcs.output_control_cycles) > 5)
		cpunum_set_input_line(dcs_cpunum, ADSP2115_SPORT0_RX, PULSE_LINE);
}


static void sound_tx_callback(int port, INT32 data)
{
	/* check if it's for SPORT1 */
	if (port != 1)
		return;

	/* check if SPORT1 is enabled */
	if (dcs.control_regs[SYSCONTROL_REG] & 0x0800) /* bit 11 */
	{
		/* we only support autobuffer here (wich is what this thing uses), bail if not enabled */
		if (dcs.control_regs[S1_AUTOBUF_REG] & 0x0002) /* bit 1 */
		{
			/* get the autobuffer registers */
			int		mreg, lreg;
			UINT16	source;
			double	sample_rate;

			dcs.ireg = (dcs.control_regs[S1_AUTOBUF_REG] >> 9) & 7;
			mreg = (dcs.control_regs[S1_AUTOBUF_REG] >> 7) & 3;
			mreg |= dcs.ireg & 0x04; /* msb comes from ireg */
			lreg = dcs.ireg;

			/* now get the register contents in a more legible format */
			/* we depend on register indexes to be continuous (wich is the case in our core) */
			source = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);
			dcs.incs = cpunum_get_reg(dcs_cpunum, ADSP2100_M0 + mreg);
			dcs.size = cpunum_get_reg(dcs_cpunum, ADSP2100_L0 + lreg);

			/* get the base value, since we need to keep it around for wrapping */
			source -= dcs.incs;

			/* make it go back one so we dont lose the first sample */
			cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, source);

			/* save it as it is now */
			dcs.ireg_base = source;

			/* calculate how long until we generate an interrupt */

			/* frequency in Hz per each bit sent */
			sample_rate = (double)Machine->drv->cpu[dcs_cpunum].cpu_clock / (double)(2 * (dcs.control_regs[S1_SCLKDIV_REG] + 1));

			/* now put it down to samples, so we know what the channel frequency has to be */
			sample_rate /= 16 * dcs.channels;
			dmadac_set_frequency(0, dcs.channels, sample_rate);
			dmadac_enable(0, dcs.channels, 1);

			/* fire off a timer wich will hit every half-buffer */
			if (dcs.channels == 1)
				timer_adjust(dcs.reg_timer, TIME_IN_HZ(sample_rate) * (dcs.size / (2 * dcs.incs)), 0, TIME_IN_HZ(sample_rate) * (dcs.size / (2 * dcs.incs)));
			else
				timer_adjust(dcs.reg_timer, TIME_IN_HZ(sample_rate) * (dcs.size / (4 * dcs.incs)), 0, TIME_IN_HZ(sample_rate) * (dcs.size / (4 * dcs.incs)));

			return;
		}
		else
			logerror( "ADSP SPORT1: trying to transmit and autobuffer not enabled!\n" );
	}

	/* if we get there, something went wrong. Disable playing */
	dmadac_enable(0, dcs.channels, 0);

	/* remove timer */
	timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
}



/***************************************************************************
	VERY BASIC & SAFE OPTIMIZATIONS
****************************************************************************/

static READ16_HANDLER( dcs_polling_r )
{
	activecpu_eat_cycles(100);
	return *dcs_polling_base;
}



/***************************************************************************
	DATA TRANSFER HLE MECHANISM
****************************************************************************/

void dcs_fifo_notify(int count, int max)
{
	/* skip if not in mid-transfer */
	if (!HLE_TRANSFERS || transfer_state == 0 || !dcs.fifo_data_r)
	{
		transfer_fifo_entries = 0;
		return;
	}
	
	/* preprocess a word */
	transfer_fifo_entries = count;
	if (transfer_state != 5 || transfer_fifo_entries == transfer_writes_left || transfer_fifo_entries >= 256)
	{
		for ( ; transfer_fifo_entries; transfer_fifo_entries--)
			preprocess_write((*dcs.fifo_data_r)());
	}
}


static void transfer_watchdog_callback(int starting_writes_left)
{
	if (transfer_fifo_entries && starting_writes_left == transfer_writes_left)
	{
		for ( ; transfer_fifo_entries; transfer_fifo_entries--)
			preprocess_write((*dcs.fifo_data_r)());
	}
	timer_adjust(transfer_watchdog, TIME_IN_MSEC(1), transfer_writes_left, 0);
}


static void s1_ack_callback2(int data)
{
	/* if the output is full, stall for a usec */
	if (IS_OUTPUT_FULL())
	{
		timer_set(TIME_IN_USEC(1), data, s1_ack_callback2);
		return;
	}
	output_latch_w(0, 0x000a, 0);
}


static void s1_ack_callback1(int data)
{
	/* if the output is full, stall for a usec */
	if (IS_OUTPUT_FULL())
	{
		timer_set(TIME_IN_USEC(1), data, s1_ack_callback1);
		return;
	}
	output_latch_w(0, data, 0);

	/* chain to the next word we need to write back */
	timer_set(TIME_IN_USEC(1), 0, s1_ack_callback2);
}


static int preprocess_stage_1(data16_t data)
{
	switch (transfer_state)
	{
		case 0:
			/* look for command 0x001a to transfer chunks of data */
			if (data == 0x001a)
			{
				if (LOG_DCS_TRANSFERS) logerror("%08X:DCS Transfer command %04X\n", activecpu_get_pc(), data);
				transfer_state++;
				if (HLE_TRANSFERS) return 1;
			}

			/* look for command 0x002a to start booting the uploaded program */
			else if (data == 0x002a)
			{
				if (LOG_DCS_TRANSFERS) logerror("%08X:DCS State change %04X\n", activecpu_get_pc(), data);
				dcs_state = 1;
			}

			/* anything else is ignored */
			else
			{
				if (LOG_DCS_TRANSFERS) logerror("Command: %04X\n", data);
			}
			break;

		case 1:
			/* first word is the start address */
			transfer_start = data;
			transfer_state++;
			if (LOG_DCS_TRANSFERS) logerror("Start address = %04X\n", transfer_start);
			if (HLE_TRANSFERS) return 1;
			break;

		case 2:
			/* second word is the stop address */
			transfer_stop = data;
			transfer_state++;
			if (LOG_DCS_TRANSFERS) logerror("Stop address = %04X\n", transfer_stop);
			if (HLE_TRANSFERS) return 1;
			break;

		case 3:
			/* third word is the transfer type */
			/* transfer type 0 = program memory */
			/* transfer type 1 = SRAM bank 0 */
			/* transfer type 2 = SRAM bank 1 */
			transfer_type = data;
			transfer_state++;
			if (LOG_DCS_TRANSFERS) logerror("Transfer type = %04X\n", transfer_type);

			/* at this point, we can compute how many words to expect for the transfer */
			transfer_writes_left = transfer_stop - transfer_start + 1;
			if (transfer_type == 0)
				transfer_writes_left *= 2;

			/* reset the checksum */
			transfer_sum = 0;

			/* handle the HLE case */
			if (HLE_TRANSFERS)
			{
				if (transfer_type == 2)
					dcs2_sram_bank_w(0, 0x1000, 0);
				return 1;
			}
			break;

		case 4:
			/* accumulate the sum over all data */
			transfer_sum += data;

			/* if we're out, stop the transfer */
			if (--transfer_writes_left == 0)
			{
				if (LOG_DCS_TRANSFERS) logerror("Transfer done, sum = %04X\n", transfer_sum);
				transfer_state = 0;
			}

			/* handle the HLE case */
			if (HLE_TRANSFERS)
			{
				/* write the new data to memory */
				cpuintrf_push_context(dcs_cpunum);
				if (transfer_type == 0)
				{
					if (transfer_writes_left & 1)
						transfer_temp = data;
					else
						program_write_dword(transfer_start++ * 4, (transfer_temp << 8) | (data & 0xff));
				}
				else
					data_write_word(transfer_start++ * 2, data);
				cpuintrf_pop_context();

				/* if we're done, start a timer to send the response words */
				if (transfer_state == 0)
					timer_set(TIME_IN_USEC(1), transfer_sum, s1_ack_callback1);
				return 1;
			}
			break;
	}
	return 0;
}


static void s2_ack_callback(int data)
{
	/* if the output is full, stall for a usec */
	if (IS_OUTPUT_FULL())
	{
		timer_set(TIME_IN_USEC(1), data, s2_ack_callback);
		return;
	}
	output_latch_w(0, data, 0);
	output_control_w(0, (dcs.output_control & ~0xff00) | 0x0300, 0);
}


static int preprocess_stage_2(data16_t data)
{
	switch (transfer_state)
	{
		case 0:
			/* look for command 0x55d0 or 0x55d1 to transfer chunks of data */
			if (data == 0x55d0 || data == 0x55d1)
			{
				if (LOG_DCS_TRANSFERS) logerror("%08X:DCS Transfer command %04X\n", activecpu_get_pc(), data);
				transfer_state++;
				if (HLE_TRANSFERS) return 1;
			}

			/* anything else is ignored */
			else
			{
				if (LOG_DCS_TRANSFERS) logerror("%08X:Command: %04X\n", activecpu_get_pc(), data);
			}
			break;

		case 1:
			/* first word is the upper bits of the start address */
			transfer_start = data << 16;
			transfer_state++;
			if (HLE_TRANSFERS) return 1;
			break;

		case 2:
			/* second word is the lower bits of the start address */
			transfer_start |= data;
			transfer_state++;
			if (LOG_DCS_TRANSFERS) logerror("Start address = %08X\n", transfer_start);
			if (HLE_TRANSFERS) return 1;
			break;

		case 3:
			/* third word is the upper bits of the stop address */
			transfer_stop = data << 16;
			transfer_state++;
			if (HLE_TRANSFERS) return 1;
			break;

		case 4:
			/* fourth word is the lower bits of the stop address */
			transfer_stop |= data;
			transfer_state++;
			if (LOG_DCS_TRANSFERS) logerror("Stop address = %08X\n", transfer_stop);

			/* at this point, we can compute how many words to expect for the transfer */
			transfer_writes_left = transfer_stop - transfer_start + 1;

			/* reset the checksum */
			transfer_sum = 0;
			if (HLE_TRANSFERS)
			{
				timer_adjust(transfer_watchdog, TIME_IN_MSEC(1), transfer_writes_left, 0);
				return 1;
			}
			break;

		case 5:
			/* accumulate the sum over all data */
			transfer_sum += data;

			/* if we're out, stop the transfer */
			if (--transfer_writes_left == 0)
			{
				if (LOG_DCS_TRANSFERS) logerror("Transfer done, sum = %04X\n", transfer_sum);
				transfer_state = 0;
			}

			/* handle the HLE case */
			if (HLE_TRANSFERS)
			{
				/* write the new data to memory */
				dcs.sounddata[transfer_start++] = data;

				/* if we're done, start a timer to send the response words */
				if (transfer_state == 0)
				{
					timer_set(TIME_IN_USEC(1), transfer_sum, s2_ack_callback);
					timer_adjust(transfer_watchdog, TIME_NEVER, 0, 0);
				}
				return 1;
			}
			break;
	}
	return 0;
}


static int preprocess_write(data16_t data)
{
	int result;
	
	/* if we're not DCS2, skip */
	if (!dcs.sport_timer)
		return 0;

	/* state 0 - initialization phase */
	if (dcs_state == 0)
		result = preprocess_stage_1(data);
	else
		result = preprocess_stage_2(data);
	
	/* if we did the write, toggle the full/not full state so interrupts are generated */
	if (result && dcs.input_empty_cb)
	{
		if (dcs.last_input_empty)
			(*dcs.input_empty_cb)(dcs.last_input_empty = 0);
		if (!dcs.last_input_empty)
			(*dcs.input_empty_cb)(dcs.last_input_empty = 1);
	}
	return result;
}
