/***************************************************************************

	Incredible Technologies/Strata system
	(32-bit blitter variant)

    driver by Aaron Giles

    Games supported:
		* Time Killers (2 sets)
		* Bloodstorm (4 sets)
		* Hard Yardage (2 sets)
		* Pairs
		* Driver's Edge (not working)
		* World Class Bowling
		* Street Fighter: The Movie (3 sets)
		* Shuffleshot

	Games not supported because IT is still selling them:
		* Golden Tee 3D Golf
		* Golden Tee Golf '97 (2 sets)
		* Golden Tee Golf '98
		* Golden Tee Golf '99
		* Golden Tee Golf 2K
		* Golden Tee Golf Classic

****************************************************************************

	Memory map TBD

***************************************************************************/


#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "machine/ticket.h"
#include "vidhrdw/generic.h"
#include "itech32.h"
#include <math.h>


#define FULL_LOGGING	0

#define CLOCK_8MHz		(8000000)
#define CLOCK_12MHz		(12000000)
#define CLOCK_25MHz		(25000000)



/*************************************
 *
 *	Static data
 *
 *************************************/

static UINT8 vint_state;
static UINT8 xint_state;
static UINT8 qint_state;

static data8_t sound_data;
static UINT8 sound_int_state;

static data8_t *via6522;
static data16_t via6522_timer_count[2];
static void *via6522_timer[2];
static data8_t via6522_int_state;

static data16_t *main_rom;
static data16_t *main_ram;
static size_t main_ram_size;
static data32_t *nvram;
static size_t nvram_size;

static offs_t itech020_prot_address;

static data8_t *sound_speedup_data;
static data16_t sound_speedup_pc;

static UINT8 is_drivedge;



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

INLINE int determine_irq_state(int vint, int xint, int qint)
{
	int level = 0;

	/* update the states */
	if (vint != -1) vint_state = vint;
	if (xint != -1) xint_state = xint;
	if (qint != -1) qint_state = qint;

	/* determine which level is active */
	if (vint_state) level = 1;
	if (xint_state) level = 2;
	if (qint_state) level = 3;

	/* Driver's Edge shifts the interrupts a bit */
	if (is_drivedge && level) level += 2;

	return level;
}


void itech32_update_interrupts(int vint, int xint, int qint)
{
	int level = determine_irq_state(vint, xint, qint);

	/* update it */
	if (level)
		cpu_set_irq_line(0, level, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static INTERRUPT_GEN( generate_int1 )
{
	/* signal the NMI */
	itech32_update_interrupts(1, -1, -1);
	if (FULL_LOGGING) logerror("------------ VBLANK (%d) --------------\n", cpu_getscanline());
}


static WRITE16_HANDLER( int1_ack_w )
{
	itech32_update_interrupts(0, -1, -1);
}



/*************************************
 *
 *	Machine initialization
 *
 *************************************/

static void via6522_timer_callback(int which);

static MACHINE_INIT( itech32 )
{
	vint_state = xint_state = qint_state = 0;
	sound_data = 0;
	sound_int_state = 0;

	/* reset the VIA chip (if used) */
	via6522_timer_count[0] = via6522_timer_count[1] = 0;
	via6522_timer[0] = timer_alloc(via6522_timer_callback);
	via6522_timer[1] = 0;
	via6522_int_state = 0;

	/* reset the ticket dispenser */
	ticket_dispenser_init(200, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);

	/* map the mirrored RAM in Driver's Edge */
	if (is_drivedge)
		cpu_setbank(2, main_ram);
}



/*************************************
 *
 *	Input handlers
 *
 *************************************/

static READ16_HANDLER( special_port3_r )
{
	int result = readinputport(3);
	if (sound_int_state) result ^= 0x08;
	return result;
}


static READ16_HANDLER( special_port4_r )
{
	int result = readinputport(4);
	if (sound_int_state) result ^= 0x08;
	return result;
}


static READ16_HANDLER( trackball_r )
{
	int lower = readinputport(6);
	int upper = readinputport(7);

	return (lower & 15) | ((upper & 15) << 4);
}


static READ32_HANDLER( trackball32_8bit_r )
{
	int lower = readinputport(6);
	int upper = readinputport(7);

	return (lower & 255) | ((upper & 255) << 8);
}


static READ32_HANDLER( trackball32_4bit_r )
{
	static int effx, effy;
	static int lastresult;
	static double lasttime;
	double curtime = timer_get_time();

	if (curtime - lasttime > cpu_getscanlineperiod())
	{
		int upper, lower;
		int dx, dy;

		int curx = readinputport(6);
		int cury = readinputport(7);

		dx = curx - effx;
		if (dx < -0x80) dx += 0x100;
		else if (dx > 0x80) dx -= 0x100;
		if (dx > 7) dx = 7;
		else if (dx < -7) dx = -7;
		effx = (effx + dx) & 0xff;
		lower = effx & 15;

		dy = cury - effy;
		if (dy < -0x80) dy += 0x100;
		else if (dy > 0x80) dy -= 0x100;
		if (dy > 7) dy = 7;
		else if (dy < -7) dy = -7;
		effy = (effy + dy) & 0xff;
		upper = effy & 15;

		lastresult = lower | (upper << 4);
	}

	lasttime = curtime;
	return lastresult | (lastresult << 16);
}


static READ32_HANDLER( trackball32_4bit_p2_r )
{
	static int effx, effy;
	static int lastresult;
	static double lasttime;
	double curtime = timer_get_time();

	if (curtime - lasttime > cpu_getscanlineperiod())
	{
		int upper, lower;
		int dx, dy;

		int curx = readinputport(8);
		int cury = readinputport(9);

		dx = curx - effx;
		if (dx < -0x80) dx += 0x100;
		else if (dx > 0x80) dx -= 0x100;
		if (dx > 7) dx = 7;
		else if (dx < -7) dx = -7;
		effx = (effx + dx) & 0xff;
		lower = effx & 15;

		dy = cury - effy;
		if (dy < -0x80) dy += 0x100;
		else if (dy > 0x80) dy -= 0x100;
		if (dy > 7) dy = 7;
		else if (dy < -7) dy = -7;
		effy = (effy + dy) & 0xff;
		upper = effy & 15;

		lastresult = lower | (upper << 4);
	}

	lasttime = curtime;
	return lastresult | (lastresult << 16);
}



/*************************************
 *
 *	Protection?
 *
 *************************************/

static READ16_HANDLER( wcbowl_prot_result_r )
{
	return main_ram[0x111d/2];
}


static READ32_HANDLER( itech020_prot_result_r )
{
	data32_t result = ((data32_t *)main_ram)[itech020_prot_address >> 2];
	result >>= (~itech020_prot_address & 3) * 8;
	return (result & 0xff) << 8;
}



/*************************************
 *
 *	Sound banking
 *
 *************************************/

static WRITE_HANDLER( sound_bank_w )
{
	logerror("sound bank = %02x\n", data);
	cpu_setbank(1, &memory_region(REGION_CPU2)[0x10000 + data * 0x4000]);
}



/*************************************
 *
 *	Sound communication
 *
 *************************************/

static void delayed_sound_data_w(int data)
{
	sound_data = data;
	sound_int_state = 1;
	cpu_set_irq_line(1, M6809_IRQ_LINE, ASSERT_LINE);
	logerror("sound_data_w() = %02x\n", sound_data);
}


static WRITE16_HANDLER( sound_data_w )
{
	if (ACCESSING_LSB)
		timer_set(TIME_NOW, data & 0xff, delayed_sound_data_w);
}


static WRITE32_HANDLER( sound_data32_w )
{
	if (!(mem_mask & 0x00ff0000))
		timer_set(TIME_NOW, (data >> 16) & 0xff, delayed_sound_data_w);
}


static READ_HANDLER( sound_data_r )
{
	logerror("sound_data_r() = %02x\n", sound_data);
	cpu_set_irq_line(1, M6809_IRQ_LINE, CLEAR_LINE);
	sound_int_state = 0;
	return sound_data;
}


static READ_HANDLER( sound_data_buffer_r )
{
	return 0;
}



/*************************************
 *
 *	Sound I/O port handling
 *
 *************************************/

static WRITE_HANDLER( pia_portb_out )
{
	logerror("PIA port B write = %02x\n", data);

	/* bit 4 controls the ticket dispenser */
	/* bit 5 controls the coin counter */
	/* bit 6 controls the diagnostic sound LED */
	ticket_dispenser_w(0, (data & 0x10) << 3);
	coin_counter_w(0, (data & 0x20) >> 5);
}


static WRITE_HANDLER( sound_output_w )
{
	logerror("sound output write = %02x\n", data);

	coin_counter_w(0, (~data & 0x20) >> 5);
}



/*************************************
 *
 *	Sound 6522 VIA handling
 *
 *************************************/

INLINE void update_via_int(void)
{
	/* if interrupts are enabled and one is pending, set the line */
	if ((via6522[14] & 0x80) && (via6522_int_state & via6522[14]))
		cpu_set_irq_line(1, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(1, M6809_FIRQ_LINE, CLEAR_LINE);
}


static void via6522_timer_callback(int which)
{
	via6522_int_state |= 0x40 >> which;
	update_via_int();
}


static WRITE_HANDLER( via6522_w )
{
	double period;

	/* update the data */
	via6522[offset] = data;

	/* switch off the offset */
	switch (offset)
	{
		case 0:		/* write to port B */
			pia_portb_out(0, data);
			break;

		case 5:		/* write into high order timer 1 */
			via6522_timer_count[0] = (via6522[5] << 8) | via6522[4];
			period = TIME_IN_HZ(CLOCK_8MHz/4) * (double)via6522_timer_count[0];
			timer_adjust(via6522_timer[0], period, 0, period);

			via6522_int_state &= ~0x40;
			update_via_int();
			break;

		case 13:	/* write interrupt flag register */
			via6522_int_state &= ~data;
			update_via_int();
			break;

		default:	/* log everything else */
			if (FULL_LOGGING) logerror("VIA write(%02x) = %02x\n", offset, data);
			break;
	}

}


static READ_HANDLER( via6522_r )
{
	int result = 0;

	/* switch off the offset */
	switch (offset)
	{
		case 4:		/* read low order timer 1 */
			via6522_int_state &= ~0x40;
			update_via_int();
			break;

		case 13:	/* interrupt flag register */
			result = via6522_int_state & 0x7f;
			if (via6522_int_state & via6522[14]) result |= 0x80;
			break;
	}

	if (FULL_LOGGING) logerror("VIA read(%02x) = %02x\n", offset, result);
	return result;
}



/*************************************
 *
 *	Additional sound code
 *
 *************************************/

static WRITE_HANDLER( firq_clear_w )
{
	cpu_set_irq_line(1, M6809_FIRQ_LINE, CLEAR_LINE);
}



/*************************************
 *
 *	Speedups
 *
 *************************************/

static READ_HANDLER( sound_speedup_r )
{
	if (sound_speedup_data[0] == sound_speedup_data[1] && activecpu_get_previouspc() == sound_speedup_pc)
		cpu_spinuntil_int();
	return sound_speedup_data[0];
}


static WRITE32_HANDLER( itech020_watchdog_w )
{
	watchdog_reset_w(0,0);
}



/*************************************
 *
 *	32-bit shunts
 *
 *************************************/

static READ32_HANDLER( input_port_0_msw_r )
{
	return input_port_0_word_r(offset,0) << 16;
}

static READ32_HANDLER( input_port_1_msw_r )
{
	return input_port_1_word_r(offset,0) << 16;
}

static READ32_HANDLER( input_port_2_msw_r )
{
	return input_port_2_word_r(offset,0) << 16;
}

static READ32_HANDLER( input_port_3_msw_r )
{
	return input_port_3_word_r(offset,0) << 16;
}

static READ32_HANDLER( input_port_4_msw_r )
{
	return special_port4_r(offset,0) << 16;
}

static READ32_HANDLER( input_port_5_msw_r )
{
	return input_port_5_word_r(offset,0) << 16;
}

static WRITE32_HANDLER( int1_ack32_w )
{
	int1_ack_w(offset, data, mem_mask);
}



/*************************************
 *
 *	NVRAM read/write
 *
 *************************************/

static NVRAM_HANDLER( itech32 )
{
	int i;

	if (read_or_write)
		mame_fwrite(file, main_ram, main_ram_size);
	else if (file)
		mame_fread(file, main_ram, main_ram_size);
	else
		for (i = 0x80; i < main_ram_size; i++)
			((UINT8 *)main_ram)[i] = rand();
}


static NVRAM_HANDLER( itech020 )
{
	int i;

	if (read_or_write)
		mame_fwrite(file, nvram, nvram_size);
	else if (file)
		mame_fread(file, nvram, nvram_size);
	else
		for (i = 0; i < nvram_size; i++)
			((UINT8 *)nvram)[i] = rand();
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

/*------ Time Killers memory layout ------*/
static MEMORY_READ16_START( timekill_readmem )
	{ 0x000000, 0x003fff, MRA16_RAM },
	{ 0x040000, 0x040001, input_port_0_word_r },
	{ 0x048000, 0x048001, input_port_1_word_r },
	{ 0x050000, 0x050001, input_port_2_word_r },
	{ 0x058000, 0x058001, special_port3_r },
	{ 0x080000, 0x08007f, itech32_video_r },
	{ 0x0c0000, 0x0c7fff, MRA16_RAM },
	{ 0x100000, 0x17ffff, MRA16_ROM },
MEMORY_END


static MEMORY_WRITE16_START( timekill_writemem )
	{ 0x000000, 0x003fff, MWA16_RAM, &main_ram, &main_ram_size },
	{ 0x050000, 0x050001, timekill_intensity_w },
	{ 0x058000, 0x058001, watchdog_reset16_w },
	{ 0x060000, 0x060001, timekill_colora_w },
	{ 0x068000, 0x068001, timekill_colorbc_w },
	{ 0x070000, 0x070001, MWA16_NOP },	/* noisy */
	{ 0x078000, 0x078001, sound_data_w },
	{ 0x080000, 0x08007f, itech32_video_w, &itech32_video },
	{ 0x0a0000, 0x0a0001, int1_ack_w },
	{ 0x0c0000, 0x0c7fff, timekill_paletteram_w, &paletteram16 },
	{ 0x100000, 0x17ffff, MWA16_ROM, &main_rom },
MEMORY_END


/*------ BloodStorm and later games memory layout ------*/
static MEMORY_READ16_START( bloodstm_readmem )
	{ 0x000000, 0x00ffff, MRA16_RAM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x100000, 0x100001, input_port_1_word_r },
	{ 0x180000, 0x180001, input_port_2_word_r },
	{ 0x200000, 0x200001, input_port_3_word_r },
	{ 0x280000, 0x280001, special_port4_r },
	{ 0x500000, 0x5000ff, bloodstm_video_r },
	{ 0x580000, 0x59ffff, MRA16_RAM },
	{ 0x780000, 0x780001, input_port_5_word_r },
	{ 0x800000, 0x87ffff, MRA16_ROM },
MEMORY_END


static MEMORY_WRITE16_START( bloodstm_writemem )
	{ 0x000000, 0x00ffff, MWA16_RAM, &main_ram, &main_ram_size },
	{ 0x080000, 0x080001, int1_ack_w },
	{ 0x200000, 0x200001, watchdog_reset16_w },
	{ 0x300000, 0x300001, bloodstm_color1_w },
	{ 0x380000, 0x380001, bloodstm_color2_w },
	{ 0x400000, 0x400001, watchdog_reset16_w },
	{ 0x480000, 0x480001, sound_data_w },
	{ 0x500000, 0x5000ff, bloodstm_video_w, &itech32_video },
	{ 0x580000, 0x59ffff, bloodstm_paletteram_w, &paletteram16 },
	{ 0x700000, 0x700001, bloodstm_plane_w },
	{ 0x800000, 0x87ffff, MWA16_ROM, &main_rom },
MEMORY_END


/*------ Pairs memory layout ------*/
static MEMORY_READ16_START( pairs_readmem )
	{ 0x000000, 0x00ffff, MRA16_RAM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x100000, 0x100001, input_port_1_word_r },
	{ 0x180000, 0x180001, input_port_2_word_r },
	{ 0x200000, 0x200001, input_port_3_word_r },
	{ 0x280000, 0x280001, special_port4_r },
	{ 0x500000, 0x5000ff, bloodstm_video_r },
	{ 0x580000, 0x59ffff, MRA16_RAM },
	{ 0x780000, 0x780001, input_port_5_word_r },
	{ 0xd00000, 0xd7ffff, MRA16_ROM },
MEMORY_END


static MEMORY_WRITE16_START( pairs_writemem )
	{ 0x000000, 0x00ffff, MWA16_RAM, &main_ram, &main_ram_size },
	{ 0x080000, 0x080001, int1_ack_w },
	{ 0x200000, 0x200001, watchdog_reset16_w },
	{ 0x300000, 0x300001, bloodstm_color1_w },
	{ 0x380000, 0x380001, bloodstm_color2_w },
	{ 0x400000, 0x400001, watchdog_reset16_w },
	{ 0x480000, 0x480001, sound_data_w },
	{ 0x500000, 0x5000ff, bloodstm_video_w, &itech32_video },
	{ 0x580000, 0x59ffff, bloodstm_paletteram_w, &paletteram16 },
	{ 0x700000, 0x700001, bloodstm_plane_w },
	{ 0xd00000, 0xd7ffff, MWA16_ROM, &main_rom },
MEMORY_END


/*------ Driver's Edge memory layout ------*/
static MEMORY_READ32_START( drivedge_readmem )
	{ 0x000000, 0x03ffff, MRA32_RAM },
	{ 0x040000, 0x07ffff, MRA32_BANK2 },
	{ 0x08c000, 0x08c003, input_port_0_msw_r },
	{ 0x08e000, 0x08e003, input_port_1_msw_r },
	{ 0x1a0000, 0x1bffff, MRA32_RAM },
	{ 0x1e0000, 0x1e00ff, itech020_video_r },
	{ 0x200000, 0x200003, input_port_2_msw_r },
	{ 0x280000, 0x280fff, MRA32_RAM },
	{ 0x300000, 0x300fff, MRA32_RAM },
	{ 0x600000, 0x607fff, MRA32_ROM },
MEMORY_END


static MEMORY_WRITE32_START( drivedge_writemem )
	{ 0x000000, 0x03ffff, MWA32_RAM, (data32_t **)&main_ram, &main_ram_size },
	{ 0x040000, 0x07ffff, MWA32_BANK2 },
	{ 0x084000, 0x084003, sound_data32_w },
//	{ 0x100000, 0x10000f, ???_w },	= 4 longwords (TMS control?)
	{ 0x180000, 0x180003, drivedge_color0_w },
	{ 0x1a0000, 0x1bffff, itech020_paletteram_w, &paletteram32 },
//	{ 0x1c0000, 0x1c0001, ???_w },	= 0x64
	{ 0x1e0000, 0x1e00ff, itech020_video_w, (data32_t **)&itech32_video },
//	{ 0x1e4000, 0x1e4003, ???_w },	= 0x1ffff
	{ 0x280000, 0x280fff, MWA32_RAM },	// initialized to zero
	{ 0x300000, 0x300fff, MWA32_RAM },	// initialized to zero
	{ 0x380000, 0x380003, MWA32_NOP },	// watchdog
	{ 0x600000, 0x607fff, MWA32_ROM, (data32_t **)&main_rom },
MEMORY_END

// 0x10000c/0/4/8 = $8000/$0/$0/$ffff1e
// 0x100008/c     = $ffffff/$8000


/*------ 68EC020-based memory layout ------*/
static MEMORY_READ32_START( itech020_readmem )
	{ 0x000000, 0x007fff, MRA32_RAM },
	{ 0x080000, 0x080003, input_port_0_msw_r },
	{ 0x100000, 0x100003, input_port_1_msw_r },
	{ 0x180000, 0x180003, input_port_2_msw_r },
	{ 0x200000, 0x200003, input_port_3_msw_r },
	{ 0x280000, 0x280003, input_port_4_msw_r },
	{ 0x500000, 0x5000ff, itech020_video_r },
	{ 0x578000, 0x57ffff, MRA32_NOP },				/* touched by protection */
	{ 0x580000, 0x59ffff, MRA32_RAM },
	{ 0x600000, 0x603fff, MRA32_RAM },
	{ 0x680000, 0x680003, itech020_prot_result_r },
	{ 0x800000, 0x9fffff, MRA32_ROM },
MEMORY_END


static MEMORY_WRITE32_START( itech020_writemem )
	{ 0x000000, 0x007fff, MWA32_RAM, (data32_t **)&main_ram, &main_ram_size },
	{ 0x080000, 0x080003, int1_ack32_w },
	{ 0x300000, 0x300003, itech020_color1_w },
	{ 0x380000, 0x380003, itech020_color2_w },
	{ 0x400000, 0x400003, itech020_watchdog_w },
	{ 0x480000, 0x480003, sound_data32_w },
	{ 0x500000, 0x5000ff, itech020_video_w, (data32_t **)&itech32_video },
	{ 0x580000, 0x59ffff, itech020_paletteram_w, &paletteram32 },
	{ 0x600000, 0x603fff, MWA32_RAM, &nvram, &nvram_size },
	{ 0x680000, 0x680003, MWA32_NOP },				/* written by protection */
	{ 0x700000, 0x700003, itech020_plane_w },
	{ 0x800000, 0x9fffff, MWA32_ROM, (data32_t **)&main_rom },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

/*------ Rev 1 sound board memory layout ------*/
static MEMORY_READ_START( sound_readmem )
	{ 0x0400, 0x0400, sound_data_r },
	{ 0x0800, 0x083f, ES5506_data_0_r },
	{ 0x0880, 0x08bf, ES5506_data_0_r },
	{ 0x1400, 0x140f, via6522_r },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0800, 0x083f, ES5506_data_0_w },
	{ 0x0880, 0x08bf, ES5506_data_0_w },
	{ 0x0c00, 0x0c00, sound_bank_w },
	{ 0x1000, 0x1000, MWA_NOP },	/* noisy */
	{ 0x1400, 0x140f, via6522_w, &via6522 },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END


/*------ Rev 2 sound board memory layout ------*/
static MEMORY_READ_START( sound_020_readmem )
	{ 0x0000, 0x0000, sound_data_r },
	{ 0x0400, 0x0400, sound_data_r },
	{ 0x0800, 0x083f, ES5506_data_0_r },
	{ 0x0880, 0x08bf, ES5506_data_0_r },
	{ 0x1800, 0x1800, sound_data_buffer_r },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_020_writemem )
	{ 0x0800, 0x083f, ES5506_data_0_w },
	{ 0x0880, 0x08bf, ES5506_data_0_w },
	{ 0x0c00, 0x0c00, sound_bank_w },
	{ 0x1400, 0x1400, firq_clear_w },
	{ 0x1800, 0x1800, MWA_NOP },
	{ 0x1c00, 0x1c00, sound_output_w },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( timekill )
	PORT_START	/* 40000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* 48000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* 50000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 58000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_DIPNAME( 0x0010, 0x0000, "Video Sync" )
	PORT_DIPSETTING(      0x0000, "-" )
	PORT_DIPSETTING(      0x0010, "+" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0020, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0000, "Violence" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0080, DEF_STR( On ))
INPUT_PORTS_END


INPUT_PORTS_START( bloodstm )
	PORT_START	/* 080000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* 100000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* 180000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 200000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 280000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_DIPNAME( 0x0010, 0x0000, "Video Sync" )
	PORT_DIPSETTING(      0x0000, "-" )
	PORT_DIPSETTING(      0x0010, "+" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0020, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0000, "Violence" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0080, DEF_STR( On ))

	PORT_START	/* 780000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER2 )
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( hardyard )
	PORT_START	/* 080000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* 100000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* 180000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )

	PORT_START	/* 200000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )

	PORT_START	/* 280000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_DIPNAME( 0x0010, 0x0000, "Video Sync" )
	PORT_DIPSETTING(      0x0000, "-" )
	PORT_DIPSETTING(      0x0010, "+" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Cabinet ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ))
	PORT_DIPNAME( 0x0040, 0x0000, "Players" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0080, DEF_STR( On ))

	PORT_START	/* 780000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( pairs )
	PORT_START	/* 080000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* 100000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* 180000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 200000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 280000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_DIPNAME( 0x0010, 0x0000, "Video Sync" )
	PORT_DIPSETTING(      0x0000, "-" )
	PORT_DIPSETTING(      0x0010, "+" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0020, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0000, "Modesty" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0040, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0080, DEF_STR( On ))

	PORT_START	/* 780000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( wcbowl )
	PORT_START	/* 080000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* 100000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* 180000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 200000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 280000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0040, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0080, DEF_STR( On ))

	PORT_START	/* 780000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* analog */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1 | IPF_REVERSE, 25, 32, 0, 255 )

	PORT_START	/* analog */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 25, 32, 0, 255 )
INPUT_PORTS_END


INPUT_PORTS_START( drivedge )
	PORT_START	/* 40000 */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON7 | IPF_PLAYER1, "Gear 1", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON8 | IPF_PLAYER1, "Gear 2", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON9 | IPF_PLAYER1, "Gear 3", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON10 | IPF_PLAYER1, "Gear 4", KEYCODE_V, IP_JOY_DEFAULT )
	PORT_SERVICE_NO_TOGGLE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 48000 */
	PORT_BITX( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "Fan",         KEYCODE_F, IP_JOY_DEFAULT )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1, "Tow Truck",   KEYCODE_T, IP_JOY_DEFAULT )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1, "Horn",        KEYCODE_SPACE, IP_JOY_DEFAULT )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "Turbo Boost", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1, "Network On",  KEYCODE_N, IP_JOY_DEFAULT )
	PORT_BITX( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Key",         KEYCODE_K, IP_JOY_DEFAULT )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 48000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )

	PORT_START	/* 50000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 58000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_DIPNAME( 0x0070, 0x0000, "Network Number" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPSETTING(      0x0030, "4" )
	PORT_DIPSETTING(      0x0040, "5" )
	PORT_DIPSETTING(      0x0050, "6" )
	PORT_DIPSETTING(      0x0060, "7" )
	PORT_DIPSETTING(      0x0070, "8" )
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( sftm )
	PORT_START	/* 080000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* 100000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* 180000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5        | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON6        | IPF_PLAYER2 )

	PORT_START	/* 200000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 280000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0010, 0x0000, "Video Sync" )
	PORT_DIPSETTING(      0x0000, "-" )
	PORT_DIPSETTING(      0x0010, "+" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, "Freeze" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	/* 780000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( shufshot )
	PORT_START	/* 080000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 100000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 | IPF_COCKTAIL )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 | IPF_COCKTAIL )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 180000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE4 ) //volume up
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE3 ) //volume down
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 200000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 280000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	/* 780000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* analog */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1 | IPF_REVERSE, 25, 32, 0, 255 )

	PORT_START	/* analog */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 25, 32, 0, 255 )

	PORT_START	/* analog */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2 | IPF_COCKTAIL | IPF_REVERSE, 25, 32, 0, 255 )

	PORT_START	/* analog */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2 | IPF_COCKTAIL, 25, 32, 0, 255 )
INPUT_PORTS_END



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct ES5506interface es5506_interface =
{
	1,
	{ 16000000 },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ REGION_SOUND3 },
	{ REGION_SOUND4 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( timekill )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, CLOCK_12MHz)
	MDRV_CPU_MEMORY(timekill_readmem,timekill_writemem)
	MDRV_CPU_VBLANK_INT(generate_int1,1)

	MDRV_CPU_ADD_TAG("sound", M6809, CLOCK_8MHz/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((263. - 240.) / 263.) * 1000000. / 60.))

	MDRV_MACHINE_INIT(itech32)
	MDRV_NVRAM_HANDLER(itech32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(384,256)
	MDRV_VISIBLE_AREA(0, 383, 0, 239)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(itech32)
	MDRV_VIDEO_UPDATE(itech32)

	/* sound hardware */
	MDRV_SOUND_ADD(ES5506, es5506_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bloodstm )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(timekill)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(bloodstm_readmem,bloodstm_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32768)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pairs )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bloodstm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(pairs_readmem,pairs_writemem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wcbowl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bloodstm)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 383, 0, 254)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( drivedge )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bloodstm)

	MDRV_CPU_REPLACE("main", M68EC020, CLOCK_25MHz)
	MDRV_CPU_MEMORY(drivedge_readmem,drivedge_writemem)
	MDRV_CPU_VBLANK_INT(NULL,0)

	MDRV_NVRAM_HANDLER(itech020)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sftm )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bloodstm)

	MDRV_CPU_REPLACE("main", M68EC020, CLOCK_25MHz)
	MDRV_CPU_MEMORY(itech020_readmem,itech020_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(sound_020_readmem,sound_020_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_assert,4)

	MDRV_NVRAM_HANDLER(itech020)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 383, 0, 254)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( timekill )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "tk00v132.u54", 0x00000, 0x40000, 0x68c74b81 )
	ROM_LOAD16_BYTE( "tk01v132.u53", 0x00001, 0x40000, 0x2158d8ef )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "tksndv41.u17", 0x10000, 0x18000, 0xc699af7b )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "tk0_rom.1", 0x000000, 0x200000, 0x94cbf6f8 )
	ROM_LOAD32_BYTE( "tk1_rom.2", 0x000001, 0x200000, 0xc07dea98 )
	ROM_LOAD32_BYTE( "tk2_rom.3", 0x000002, 0x200000, 0x183eed2a )
	ROM_LOAD32_BYTE( "tk3_rom.4", 0x000003, 0x200000, 0xb1da1058 )
	ROM_LOAD32_BYTE( "tkgrom.01", 0x800000, 0x020000, 0xb030c3d9 )
	ROM_LOAD32_BYTE( "tkgrom.02", 0x800001, 0x020000, 0xe98492a4 )
	ROM_LOAD32_BYTE( "tkgrom.03", 0x800002, 0x020000, 0x6088fa64 )
	ROM_LOAD32_BYTE( "tkgrom.04", 0x800003, 0x020000, 0x95be2318 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "tksrom00.u18", 0x000000, 0x80000, 0x79d8b83a )
	ROM_LOAD16_BYTE( "tksrom01.u20", 0x100000, 0x80000, 0xec01648c )
	ROM_LOAD16_BYTE( "tksrom02.u26", 0x200000, 0x80000, 0x051ced3e )
ROM_END


ROM_START( timek131 )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "tk00v131.u54", 0x00000, 0x40000, 0xe09ae32b )
	ROM_LOAD16_BYTE( "tk01v131.u53", 0x00001, 0x40000, 0xc29137ec )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "tksnd.u17", 0x10000, 0x18000, 0xab1684c3 )
	ROM_CONTINUE(          0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "tk0_rom.1", 0x000000, 0x200000, 0x94cbf6f8 )
	ROM_LOAD32_BYTE( "tk1_rom.2", 0x000001, 0x200000, 0xc07dea98 )
	ROM_LOAD32_BYTE( "tk2_rom.3", 0x000002, 0x200000, 0x183eed2a )
	ROM_LOAD32_BYTE( "tk3_rom.4", 0x000003, 0x200000, 0xb1da1058 )
	ROM_LOAD32_BYTE( "tkgrom.01", 0x800000, 0x020000, 0xb030c3d9 )
	ROM_LOAD32_BYTE( "tkgrom.02", 0x800001, 0x020000, 0xe98492a4 )
	ROM_LOAD32_BYTE( "tkgrom.03", 0x800002, 0x020000, 0x6088fa64 )
	ROM_LOAD32_BYTE( "tkgrom.04", 0x800003, 0x020000, 0x95be2318 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "tksrom00.u18", 0x000000, 0x80000, 0x79d8b83a )
	ROM_LOAD16_BYTE( "tksrom01.u20", 0x100000, 0x80000, 0xec01648c )
	ROM_LOAD16_BYTE( "tksrom02.u26", 0x200000, 0x80000, 0x051ced3e )
ROM_END


ROM_START( bloodstm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "bld02.bin", 0x00000, 0x40000, 0x95f36db6 )
	ROM_LOAD16_BYTE( "bld01.bin", 0x00001, 0x40000, 0xfcc04b93 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "bldsnd.u17", 0x10000, 0x18000, 0xdddeedbb )
	ROM_CONTINUE(           0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "bsgrom0.bin",  0x000000, 0x080000, 0x4e10b8c1 )
	ROM_LOAD32_BYTE( "bsgrom5.bin",  0x000001, 0x080000, 0x6333b6ce )
	ROM_LOAD32_BYTE( "bsgrom10.bin", 0x000002, 0x080000, 0xa972a65c )
	ROM_LOAD32_BYTE( "bsgrom15.bin", 0x000003, 0x080000, 0x9a8f54aa )
	ROM_LOAD32_BYTE( "bsgrom1.bin",  0x200000, 0x080000, 0x10abf660 )
	ROM_LOAD32_BYTE( "bsgrom6.bin",  0x200001, 0x080000, 0x06a260d5 )
	ROM_LOAD32_BYTE( "bsgrom11.bin", 0x200002, 0x080000, 0xf2cab3c7 )
	ROM_LOAD32_BYTE( "bsgrom16.bin", 0x200003, 0x080000, 0x403aef7b )
	ROM_LOAD32_BYTE( "bsgrom2.bin",  0x400000, 0x080000, 0x488200b1 )
	ROM_LOAD32_BYTE( "bsgrom7.bin",  0x400001, 0x080000, 0x5bb19727 )
	ROM_LOAD32_BYTE( "bsgrom12.bin", 0x400002, 0x080000, 0xb10d674f )
	ROM_LOAD32_BYTE( "bsgrom17.bin", 0x400003, 0x080000, 0x7119df7e )
	ROM_LOAD32_BYTE( "bsgrom3.bin",  0x600000, 0x080000, 0x2378792e )
	ROM_LOAD32_BYTE( "bsgrom8.bin",  0x600001, 0x080000, 0x3640ca2e )
	ROM_LOAD32_BYTE( "bsgrom13.bin", 0x600002, 0x080000, 0xbd4a071d )
	ROM_LOAD32_BYTE( "bsgrom18.bin", 0x600003, 0x080000, 0x12959bb8 )
	ROM_FILL(                        0x800000, 0x080000, 0xff )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "enssnd2m.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bssrom0.bin",  0x000000, 0x80000, 0xee4570c8 )
	ROM_LOAD16_BYTE( "bssrom1.bin",  0x100000, 0x80000, 0xb0f32ec5 )
	ROM_LOAD16_BYTE( "bssrom2.bin",  0x300000, 0x40000, 0x8aee1e77 )
ROM_END


ROM_START( bloods22 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "bld00v22.u83", 0x00000, 0x40000, 0x904e9208 )
	ROM_LOAD16_BYTE( "bld01v22.u88", 0x00001, 0x40000, 0x78336a7b )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "bldsnd.u17", 0x10000, 0x18000, 0xdddeedbb )
	ROM_CONTINUE(           0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "bsgrom0.bin",  0x000000, 0x080000, 0x4e10b8c1 )
	ROM_LOAD32_BYTE( "bsgrom5.bin",  0x000001, 0x080000, 0x6333b6ce )
	ROM_LOAD32_BYTE( "bsgrom10.bin", 0x000002, 0x080000, 0xa972a65c )
	ROM_LOAD32_BYTE( "bsgrom15.bin", 0x000003, 0x080000, 0x9a8f54aa )
	ROM_LOAD32_BYTE( "bsgrom1.bin",  0x200000, 0x080000, 0x10abf660 )
	ROM_LOAD32_BYTE( "bsgrom6.bin",  0x200001, 0x080000, 0x06a260d5 )
	ROM_LOAD32_BYTE( "bsgrom11.bin", 0x200002, 0x080000, 0xf2cab3c7 )
	ROM_LOAD32_BYTE( "bsgrom16.bin", 0x200003, 0x080000, 0x403aef7b )
	ROM_LOAD32_BYTE( "bsgrom2.bin",  0x400000, 0x080000, 0x488200b1 )
	ROM_LOAD32_BYTE( "bsgrom7.bin",  0x400001, 0x080000, 0x5bb19727 )
	ROM_LOAD32_BYTE( "bsgrom12.bin", 0x400002, 0x080000, 0xb10d674f )
	ROM_LOAD32_BYTE( "bsgrom17.bin", 0x400003, 0x080000, 0x7119df7e )
	ROM_LOAD32_BYTE( "bsgrom3.bin",  0x600000, 0x080000, 0x2378792e )
	ROM_LOAD32_BYTE( "bsgrom8.bin",  0x600001, 0x080000, 0x3640ca2e )
	ROM_LOAD32_BYTE( "bsgrom13.bin", 0x600002, 0x080000, 0xbd4a071d )
	ROM_LOAD32_BYTE( "bsgrom18.bin", 0x600003, 0x080000, 0x12959bb8 )
	ROM_FILL(                        0x800000, 0x080000, 0xff )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "enssnd2m.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bssrom0.bin",  0x000000, 0x80000, 0xee4570c8 )
	ROM_LOAD16_BYTE( "bssrom1.bin",  0x100000, 0x80000, 0xb0f32ec5 )
	ROM_LOAD16_BYTE( "bssrom2.bin",  0x300000, 0x40000, 0x8aee1e77 )
ROM_END


ROM_START( bloods21 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "bld00v21.u83", 0x00000, 0x40000, 0x71215c8e )
	ROM_LOAD16_BYTE( "bld01v21.u88", 0x00001, 0x40000, 0xda403da6 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "bldsnd.u17", 0x10000, 0x18000, 0xdddeedbb )
	ROM_CONTINUE(           0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "bsgrom0.bin",  0x000000, 0x080000, 0x4e10b8c1 )
	ROM_LOAD32_BYTE( "bsgrom5.bin",  0x000001, 0x080000, 0x6333b6ce )
	ROM_LOAD32_BYTE( "bsgrom10.bin", 0x000002, 0x080000, 0xa972a65c )
	ROM_LOAD32_BYTE( "bsgrom15.bin", 0x000003, 0x080000, 0x9a8f54aa )
	ROM_LOAD32_BYTE( "bsgrom1.bin",  0x200000, 0x080000, 0x10abf660 )
	ROM_LOAD32_BYTE( "bsgrom6.bin",  0x200001, 0x080000, 0x06a260d5 )
	ROM_LOAD32_BYTE( "bsgrom11.bin", 0x200002, 0x080000, 0xf2cab3c7 )
	ROM_LOAD32_BYTE( "bsgrom16.bin", 0x200003, 0x080000, 0x403aef7b )
	ROM_LOAD32_BYTE( "bsgrom2.bin",  0x400000, 0x080000, 0x488200b1 )
	ROM_LOAD32_BYTE( "bsgrom7.bin",  0x400001, 0x080000, 0x5bb19727 )
	ROM_LOAD32_BYTE( "bsgrom12.bin", 0x400002, 0x080000, 0xb10d674f )
	ROM_LOAD32_BYTE( "bsgrom17.bin", 0x400003, 0x080000, 0x7119df7e )
	ROM_LOAD32_BYTE( "bsgrom3.bin",  0x600000, 0x080000, 0x2378792e )
	ROM_LOAD32_BYTE( "bsgrom8.bin",  0x600001, 0x080000, 0x3640ca2e )
	ROM_LOAD32_BYTE( "bsgrom13.bin", 0x600002, 0x080000, 0xbd4a071d )
	ROM_LOAD32_BYTE( "bsgrom18.bin", 0x600003, 0x080000, 0x12959bb8 )
	ROM_FILL(                        0x800000, 0x080000, 0xff )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "enssnd2m.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bssrom0.bin",  0x000000, 0x80000, 0xee4570c8 )
	ROM_LOAD16_BYTE( "bssrom1.bin",  0x100000, 0x80000, 0xb0f32ec5 )
	ROM_LOAD16_BYTE( "bssrom2.bin",  0x300000, 0x40000, 0x8aee1e77 )
ROM_END


ROM_START( bloods11 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "bld00-11.u83", 0x00000, 0x40000, 0x4fff8f9b )
	ROM_LOAD16_BYTE( "bld01-11.u88", 0x00001, 0x40000, 0x59ce23ea )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "bldsnd.u17", 0x10000, 0x18000, 0xdddeedbb )
	ROM_CONTINUE(           0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "bsgrom0.bin",  0x000000, 0x080000, 0x4e10b8c1 )
	ROM_LOAD32_BYTE( "bsgrom5.bin",  0x000001, 0x080000, 0x6333b6ce )
	ROM_LOAD32_BYTE( "bsgrom10.bin", 0x000002, 0x080000, 0xa972a65c )
	ROM_LOAD32_BYTE( "bsgrom15.bin", 0x000003, 0x080000, 0x9a8f54aa )
	ROM_LOAD32_BYTE( "bsgrom1.bin",  0x200000, 0x080000, 0x10abf660 )
	ROM_LOAD32_BYTE( "bsgrom6.bin",  0x200001, 0x080000, 0x06a260d5 )
	ROM_LOAD32_BYTE( "bsgrom11.bin", 0x200002, 0x080000, 0xf2cab3c7 )
	ROM_LOAD32_BYTE( "bsgrom16.bin", 0x200003, 0x080000, 0x403aef7b )
	ROM_LOAD32_BYTE( "bsgrom2.bin",  0x400000, 0x080000, 0x488200b1 )
	ROM_LOAD32_BYTE( "bsgrom7.bin",  0x400001, 0x080000, 0x5bb19727 )
	ROM_LOAD32_BYTE( "bsgrom12.bin", 0x400002, 0x080000, 0xb10d674f )
	ROM_LOAD32_BYTE( "bsgrom17.bin", 0x400003, 0x080000, 0x7119df7e )
	ROM_LOAD32_BYTE( "bsgrom3.bin",  0x600000, 0x080000, 0x2378792e )
	ROM_LOAD32_BYTE( "bsgrom8.bin",  0x600001, 0x080000, 0x3640ca2e )
	ROM_LOAD32_BYTE( "bsgrom13.bin", 0x600002, 0x080000, 0xbd4a071d )
	ROM_LOAD32_BYTE( "bsgrom18.bin", 0x600003, 0x080000, 0x12959bb8 )
	ROM_FILL(                        0x800000, 0x080000, 0xff )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "enssnd2m.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bssrom0.bin",  0x000000, 0x80000, 0xee4570c8 )
	ROM_LOAD16_BYTE( "bssrom1.bin",  0x100000, 0x80000, 0xb0f32ec5 )
	ROM_LOAD16_BYTE( "bssrom2.bin",  0x300000, 0x40000, 0x8aee1e77 )
ROM_END


ROM_START( hardyard )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "fb00v12.u83", 0x00000, 0x40000, 0xc7497692 )
	ROM_LOAD16_BYTE( "fb00v12.u88", 0x00001, 0x40000, 0x3320c79a )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "fbsndv11.u17", 0x10000, 0x18000, 0xd221b121 )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "itfb0.bin",  0x000000, 0x100000, 0x0b7781af )
	ROM_LOAD32_BYTE( "itfb1.bin",  0x000001, 0x100000, 0x178d0f9b )
	ROM_LOAD32_BYTE( "itfb2.bin",  0x000002, 0x100000, 0x0a17231e )
	ROM_LOAD32_BYTE( "itfb3.bin",  0x000003, 0x100000, 0x104456af )
	ROM_LOAD32_BYTE( "itfb4.bin",  0x400000, 0x100000, 0x2cb6f454 )
	ROM_LOAD32_BYTE( "itfb5.bin",  0x400001, 0x100000, 0x9b19b873 )
	ROM_LOAD32_BYTE( "itfb6.bin",  0x400002, 0x100000, 0x58694394 )
	ROM_LOAD32_BYTE( "itfb7.bin",  0x400003, 0x100000, 0x9b7a2d1a )
	ROM_LOAD32_BYTE( "itfb8.bin",  0x800000, 0x020000, 0xa1656bf8 )
	ROM_LOAD32_BYTE( "itfb9.bin",  0x800001, 0x020000, 0x2afa9e10 )
	ROM_LOAD32_BYTE( "itfb10.bin", 0x800002, 0x020000, 0xd5d15b38 )
	ROM_LOAD32_BYTE( "itfb11.bin", 0x800003, 0x020000, 0xcd4f0df0 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "fbrom0.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "fbsrom00.bin", 0x000000, 0x080000, 0xb0a76ad2 )
	ROM_LOAD16_BYTE( "fbsrom01.bin", 0x100000, 0x080000, 0x9fbf6a02 )
ROM_END


ROM_START( hardyd10 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "hrdyrd.u83", 0x00000, 0x40000, 0xf839393c )
	ROM_LOAD16_BYTE( "hrdyrd.u88", 0x00001, 0x40000, 0xca444702 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "hy_fbsnd.u17", 0x10000, 0x18000, 0x6c6db5b8 )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "itfb0.bin",  0x000000, 0x100000, 0x0b7781af )
	ROM_LOAD32_BYTE( "itfb1.bin",  0x000001, 0x100000, 0x178d0f9b )
	ROM_LOAD32_BYTE( "itfb2.bin",  0x000002, 0x100000, 0x0a17231e )
	ROM_LOAD32_BYTE( "itfb3.bin",  0x000003, 0x100000, 0x104456af )
	ROM_LOAD32_BYTE( "itfb4.bin",  0x400000, 0x100000, 0x2cb6f454 )
	ROM_LOAD32_BYTE( "itfb5.bin",  0x400001, 0x100000, 0x9b19b873 )
	ROM_LOAD32_BYTE( "itfb6.bin",  0x400002, 0x100000, 0x58694394 )
	ROM_LOAD32_BYTE( "itfb7.bin",  0x400003, 0x100000, 0x9b7a2d1a )
	ROM_LOAD32_BYTE( "itfb8.bin",  0x800000, 0x020000, 0xa1656bf8 )
	ROM_LOAD32_BYTE( "itfb9.bin",  0x800001, 0x020000, 0x2afa9e10 )
	ROM_LOAD32_BYTE( "itfb10.bin", 0x800002, 0x020000, 0xd5d15b38 )
	ROM_LOAD32_BYTE( "itfb11.bin", 0x800003, 0x020000, 0xcd4f0df0 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "fbrom0.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "fbsrom00.bin", 0x000000, 0x080000, 0xb0a76ad2 )
	ROM_LOAD16_BYTE( "fbsrom01.bin", 0x100000, 0x080000, 0x9fbf6a02 )
ROM_END


ROM_START( pairs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x40000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "pair0", 0x00000, 0x20000, 0x774995a3 )
	ROM_LOAD16_BYTE( "pair1", 0x00001, 0x20000, 0x85d0b73a )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "pairsnd", 0x10000, 0x18000, 0x7a514cfd )
	ROM_CONTINUE(        0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "grom0",  0x000000, 0x80000, 0xbaf1c2dd )
	ROM_LOAD32_BYTE( "grom5",  0x000001, 0x80000, 0x30e993f3 )
	ROM_LOAD32_BYTE( "grom10", 0x000002, 0x80000, 0x3f52f50d )
	ROM_LOAD32_BYTE( "grom15", 0x000003, 0x80000, 0xfd38aa36 )
	ROM_LOAD32_BYTE( "grom1",  0x200000, 0x80000, 0xe4a11687 )
	ROM_LOAD32_BYTE( "grom6",  0x200001, 0x80000, 0x1fbda5f2 )
	ROM_LOAD32_BYTE( "grom11", 0x200002, 0x80000, 0xaed4a84d )
	ROM_LOAD32_BYTE( "grom16", 0x200003, 0x80000, 0x3be7031b )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "fbrom0.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom0", 0x000000, 0x80000, 0x1d96c581 )
ROM_END


ROM_START( wcbowl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x40000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "wcb.83", 0x00000, 0x20000, 0x0602c5ce )
	ROM_LOAD16_BYTE( "wcb.88", 0x00001, 0x20000, 0x49573493 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "wcb_snd.17", 0x10000, 0x18000, 0xc14907ba )
	ROM_CONTINUE(           0x08000, 0x08000 )

	ROM_REGION( 0x880000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "wcb_grm0.0", 0x000000, 0x080000, 0x5d79aaae )
	ROM_LOAD32_BYTE( "wcb_grm0.1", 0x000001, 0x080000, 0xe26dcedb )
	ROM_LOAD32_BYTE( "wcb_grm0.2", 0x000002, 0x080000, 0x32735875 )
	ROM_LOAD32_BYTE( "wcb_grm0.3", 0x000003, 0x080000, 0x019d0ab8 )
	ROM_LOAD32_BYTE( "wcb_grm1.0", 0x200000, 0x080000, 0x8bd31762 )
	ROM_LOAD32_BYTE( "wcb_grm1.1", 0x200001, 0x080000, 0xb3f761fc )
	ROM_LOAD32_BYTE( "wcb_grm1.2", 0x200002, 0x080000, 0xc22f44ad )
	ROM_LOAD32_BYTE( "wcb_grm1.3", 0x200003, 0x080000, 0x036084c4 )
	ROM_FILL(                      0x400000, 0x480000, 0xff )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "wcb_rom.0",   0x000000, 0x200000, 0x0814ab80 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "wcb_srom.0",  0x000000, 0x080000, 0x115bcd1f )
	ROM_LOAD16_BYTE( "wcb_srom.1",  0x100000, 0x080000, 0x87a4a4d8 )
ROM_END


ROM_START( drivedge )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x8000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD( "de-u130.bin", 0x00000, 0x8000, 0x873579b0 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "desndu17.bin", 0x10000, 0x18000, 0x6e8ca8bc )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "de-u7net.bin", 0x08000, 0x08000, 0xdea8b9de )

	ROM_REGION( 0xa00000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "de-grom0.bin", 0x000000, 0x80000, 0x7fe5b01b )
	ROM_LOAD32_BYTE( "de-grom5.bin", 0x000001, 0x80000, 0x5ea6dbc2 )
	ROM_LOAD32_BYTE( "de-grm10.bin", 0x000002, 0x80000, 0x76be06cd )
	ROM_LOAD32_BYTE( "de-grm15.bin", 0x000003, 0x80000, 0x119bf46b )
	ROM_LOAD32_BYTE( "de-grom1.bin", 0x200000, 0x80000, 0x5b88e8b7 )
	ROM_LOAD32_BYTE( "de-grom6.bin", 0x200001, 0x80000, 0x2cb76b9a )
	ROM_LOAD32_BYTE( "de-grm11.bin", 0x200002, 0x80000, 0x5d29018c )
	ROM_LOAD32_BYTE( "de-grm16.bin", 0x200003, 0x80000, 0x476940fb )
	ROM_LOAD32_BYTE( "de-grom2.bin", 0x400000, 0x80000, 0x5ccc4c62 )
	ROM_LOAD32_BYTE( "de-grom7.bin", 0x400001, 0x80000, 0x45367070 )
	ROM_LOAD32_BYTE( "de-grm12.bin", 0x400002, 0x80000, 0xb978ef5a )
	ROM_LOAD32_BYTE( "de-grm17.bin", 0x400003, 0x80000, 0xeff8abac )
	ROM_LOAD32_BYTE( "de-grom3.bin", 0x600000, 0x20000, 0x9cd252c9 )
	ROM_LOAD32_BYTE( "de-grom8.bin", 0x600001, 0x20000, 0x43f10ca4 )
	ROM_LOAD32_BYTE( "de-grm13.bin", 0x600002, 0x20000, 0x431d131e )
	ROM_LOAD32_BYTE( "de-grm18.bin", 0x600003, 0x20000, 0xb09e0d9c )
	ROM_LOAD32_BYTE( "de-grom4.bin", 0x800000, 0x20000, 0xc499cdfa )
	ROM_LOAD32_BYTE( "de-grom9.bin", 0x800001, 0x20000, 0xe5f21566 )
	ROM_LOAD32_BYTE( "de-grm14.bin", 0x800002, 0x20000, 0x09dbe382 )
	ROM_LOAD32_BYTE( "de-grm19.bin", 0x800003, 0x20000, 0x4ced78e1 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "fbrom0.bin", 0x000000, 0x200000, 0x9fdc4825 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "de-srom0.bin", 0x000000, 0x80000, 0x649c685f )
	ROM_LOAD16_BYTE( "de-srom1.bin", 0x100000, 0x80000, 0xdf4fff97 )
ROM_END


ROM_START( sftm )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x100000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "sftmrom0.112", 0x00000, 0x40000, 0x9d09355c )
	ROM_LOAD32_BYTE( "sftmrom1.112", 0x00001, 0x40000, 0xa58ac6a9 )
	ROM_LOAD32_BYTE( "sftmrom2.112", 0x00002, 0x40000, 0x2f21a4f6 )
	ROM_LOAD32_BYTE( "sftmrom3.112", 0x00003, 0x40000, 0xd26648d9 )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )
	ROM_LOAD( "snd-v10.bin", 0x10000, 0x38000, 0x10d85366 )
	ROM_CONTINUE(            0x08000, 0x08000 )

	ROM_REGION( 0x2080000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "rm0-0.bin",  0x0000000, 0x400000, 0x09ef29cb )
	ROM_LOAD32_BYTE( "rm0-1.bin",  0x0000001, 0x400000, 0x6f5910fa )
	ROM_LOAD32_BYTE( "rm0-2.bin",  0x0000002, 0x400000, 0xb8a2add5 )
	ROM_LOAD32_BYTE( "rm0-3.bin",  0x0000003, 0x400000, 0x6b6ff867 )
	ROM_LOAD32_BYTE( "rm1-0.bin",  0x1000000, 0x400000, 0xd5d65f77 )
	ROM_LOAD32_BYTE( "rm1-1.bin",  0x1000001, 0x400000, 0x90467e27 )
	ROM_LOAD32_BYTE( "rm1-2.bin",  0x1000002, 0x400000, 0x903e56c2 )
	ROM_LOAD32_BYTE( "rm1-3.bin",  0x1000003, 0x400000, 0xfac35686 )
	ROM_LOAD32_BYTE( "grm3_0.bin", 0x2000000, 0x020000, 0x3e1f76f7 )
	ROM_LOAD32_BYTE( "grm3_1.bin", 0x2000001, 0x020000, 0x578054b6 )
	ROM_LOAD32_BYTE( "grm3_2.bin", 0x2000002, 0x020000, 0x9af2f698 )
	ROM_LOAD32_BYTE( "grm3_3.bin", 0x2000003, 0x020000, 0xcd38d1d6 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom0.bin", 0x000000, 0x200000, 0x6ca1d3fc )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom3.bin",  0x000000, 0x080000, 0x4f181534 )
ROM_END


ROM_START( sftm110 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x100000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "prom0_v1.1", 0x00000, 0x40000, 0x00c0c63c )
	ROM_LOAD32_BYTE( "prom1_v1.1", 0x00001, 0x40000, 0xd4d2a67e )
	ROM_LOAD32_BYTE( "prom2_v1.1", 0x00002, 0x40000, 0xd7b36c92 )
	ROM_LOAD32_BYTE( "prom3_v1.1", 0x00003, 0x40000, 0xbe3efdbd )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )
	ROM_LOAD( "snd-v10.bin", 0x10000, 0x38000, 0x10d85366 )
	ROM_CONTINUE(            0x08000, 0x08000 )

	ROM_REGION( 0x2080000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "rm0-0.bin",  0x0000000, 0x400000, 0x09ef29cb )
	ROM_LOAD32_BYTE( "rm0-1.bin",  0x0000001, 0x400000, 0x6f5910fa )
	ROM_LOAD32_BYTE( "rm0-2.bin",  0x0000002, 0x400000, 0xb8a2add5 )
	ROM_LOAD32_BYTE( "rm0-3.bin",  0x0000003, 0x400000, 0x6b6ff867 )
	ROM_LOAD32_BYTE( "rm1-0.bin",  0x1000000, 0x400000, 0xd5d65f77 )
	ROM_LOAD32_BYTE( "rm1-1.bin",  0x1000001, 0x400000, 0x90467e27 )
	ROM_LOAD32_BYTE( "rm1-2.bin",  0x1000002, 0x400000, 0x903e56c2 )
	ROM_LOAD32_BYTE( "rm1-3.bin",  0x1000003, 0x400000, 0xfac35686 )
	ROM_LOAD32_BYTE( "grm3_0.bin", 0x2000000, 0x020000, 0x3e1f76f7 )
	ROM_LOAD32_BYTE( "grm3_1.bin", 0x2000001, 0x020000, 0x578054b6 )
	ROM_LOAD32_BYTE( "grm3_2.bin", 0x2000002, 0x020000, 0x9af2f698 )
	ROM_LOAD32_BYTE( "grm3_3.bin", 0x2000003, 0x020000, 0xcd38d1d6 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom0.bin", 0x000000, 0x200000, 0x6ca1d3fc )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom3.bin",  0x000000, 0x080000, 0x4f181534 )
ROM_END


ROM_START( sftmj )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x100000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "prom0112.bin", 0x00000, 0x40000, 0x640a04a8 )
	ROM_LOAD32_BYTE( "prom1112.bin", 0x00001, 0x40000, 0x2a27b690 )
	ROM_LOAD32_BYTE( "prom2112.bin", 0x00002, 0x40000, 0xcec1dd7b )
	ROM_LOAD32_BYTE( "prom3112.bin", 0x00003, 0x40000, 0x48fa60f4 )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )
	ROM_LOAD( "snd_v111.u23", 0x10000, 0x38000, 0x004854ed )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x2080000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "rm0-0.bin",  0x0000000, 0x400000, 0x09ef29cb )
	ROM_LOAD32_BYTE( "rm0-1.bin",  0x0000001, 0x400000, 0x6f5910fa )
	ROM_LOAD32_BYTE( "rm0-2.bin",  0x0000002, 0x400000, 0xb8a2add5 )
	ROM_LOAD32_BYTE( "rm0-3.bin",  0x0000003, 0x400000, 0x6b6ff867 )
	ROM_LOAD32_BYTE( "rm1-0.bin",  0x1000000, 0x400000, 0xd5d65f77 )
	ROM_LOAD32_BYTE( "rm1-1.bin",  0x1000001, 0x400000, 0x90467e27 )
	ROM_LOAD32_BYTE( "rm1-2.bin",  0x1000002, 0x400000, 0x903e56c2 )
	ROM_LOAD32_BYTE( "rm1-3.bin",  0x1000003, 0x400000, 0xfac35686 )
	ROM_LOAD32_BYTE( "grm3_0.bin", 0x2000000, 0x020000, 0x3e1f76f7 )
	ROM_LOAD32_BYTE( "grm3_1.bin", 0x2000001, 0x020000, 0x578054b6 )
	ROM_LOAD32_BYTE( "grm3_2.bin", 0x2000002, 0x020000, 0x9af2f698 )
	ROM_LOAD32_BYTE( "grm3_3.bin", 0x2000003, 0x020000, 0xcd38d1d6 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom0.bin", 0x000000, 0x200000, 0x6ca1d3fc )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom3.bin",  0x000000, 0x080000, 0x4f181534 )
ROM_END


ROM_START( shufshot )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "pgm0v1.39", 0x00000, 0x20000, 0xe811fc4a )
	ROM_LOAD32_BYTE( "pgm1v1.39", 0x00001, 0x20000, 0xf9d120c5 )
	ROM_LOAD32_BYTE( "pgm2v1.39", 0x00002, 0x20000, 0x9f12414d )
	ROM_LOAD32_BYTE( "pgm3v1.39", 0x00003, 0x20000, 0x108a69be )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )
	ROM_LOAD( "sndv1.1", 0x10000, 0x18000, 0xe37d599d )
	ROM_CONTINUE(        0x08000, 0x08000 )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "grom0_0", 0x000000, 0x80000, 0x832a3d6a )
	ROM_LOAD32_BYTE( "grom0_1", 0x000001, 0x80000, 0x155e48a2 )
	ROM_LOAD32_BYTE( "grom0_2", 0x000002, 0x80000, 0x9f2b470d )
	ROM_LOAD32_BYTE( "grom0_3", 0x000003, 0x80000, 0x3855a16a )
	ROM_LOAD32_BYTE( "grom1_0", 0x200000, 0x80000, 0xed140389 )
	ROM_LOAD32_BYTE( "grom1_1", 0x200001, 0x80000, 0xbd2ffbca )
	ROM_LOAD32_BYTE( "grom1_2", 0x200002, 0x80000, 0xc6de4187 )
	ROM_LOAD32_BYTE( "grom1_3", 0x200003, 0x80000, 0x0c707aa2 )
	ROM_LOAD32_BYTE( "grom2_0", 0x400000, 0x80000, 0x529b4259 )
	ROM_LOAD32_BYTE( "grom2_1", 0x400001, 0x80000, 0x4b52ab1a )
	ROM_LOAD32_BYTE( "grom2_2", 0x400002, 0x80000, 0xf45fad03 )
	ROM_LOAD32_BYTE( "grom2_3", 0x400003, 0x80000, 0x1bcb26c8 )
	ROM_LOAD32_BYTE( "grom3_0", 0x600000, 0x80000, 0xa29763db )
	ROM_LOAD32_BYTE( "grom3_1", 0x600001, 0x80000, 0xc757084c )
	ROM_LOAD32_BYTE( "grom3_2", 0x600002, 0x80000, 0x2971cb25 )
	ROM_LOAD32_BYTE( "grom3_3", 0x600003, 0x80000, 0x4fcbee51 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom0", 0x000000, 0x80000, 0x9a3cb6c9 )
	ROM_LOAD16_BYTE( "srom1", 0x200000, 0x80000, 0x8c89948a )
ROM_END



/*************************************
 *
 *	Driver-specific init
 *
 *************************************/

static void init_program_rom(void)
{
	memcpy(main_rom, memory_region(REGION_USER1), memory_region_length(REGION_USER1));
	memcpy(memory_region(REGION_CPU1), memory_region(REGION_USER1), 0x80);
}


static void init_sound_speedup(offs_t offset, offs_t pc)
{
	sound_speedup_data = install_mem_read_handler(1, offset, offset, sound_speedup_r);
	sound_speedup_pc = pc;
}


static DRIVER_INIT( timekill )
{
	init_program_rom();
	init_sound_speedup(0x2010, 0x8c54);
	itech32_vram_height = 512;
	itech32_planes = 2;
	is_drivedge = 0;
}


static DRIVER_INIT( hardyard )
{
	init_program_rom();
	init_sound_speedup(0x2010, 0x8e16);
	itech32_vram_height = 1024;
	itech32_planes = 1;
	is_drivedge = 0;
}


static DRIVER_INIT( bloodstm )
{
	init_program_rom();
	init_sound_speedup(0x2011, 0x8ebf);
	itech32_vram_height = 1024;
	itech32_planes = 1;
	is_drivedge = 0;
}


static DRIVER_INIT( wcbowl )
{
	init_program_rom();
	init_sound_speedup(0x2011, 0x8f93);
	itech32_vram_height = 1024;
	itech32_planes = 1;

	install_mem_read16_handler(0, 0x680000, 0x680001, trackball_r);

	install_mem_read16_handler(0, 0x578000, 0x57ffff, MRA16_NOP);
	install_mem_read16_handler(0, 0x680080, 0x680081, wcbowl_prot_result_r);
	install_mem_write16_handler(0, 0x680080, 0x680081, MWA16_NOP);
}


static DRIVER_INIT( drivedge )
{
	init_program_rom();
//	init_sound_speedup(0x2011, 0x8ebf);
	itech32_vram_height = 1024;
	itech32_planes = 1;
	is_drivedge = 1;
}


static void init_sftm_common(int prot_addr, int sound_pc)
{
	init_program_rom();
	init_sound_speedup(0x2011, sound_pc);
	itech32_vram_height = 1024;
	itech32_planes = 1;
	is_drivedge = 0;

	itech020_prot_address = prot_addr;

	install_mem_write32_handler(0, 0x300000, 0x300003, itech020_color2_w);
	install_mem_write32_handler(0, 0x380000, 0x380003, itech020_color1_w);
}


static DRIVER_INIT( sftm )
{
	init_sftm_common(0x7a6a, 0x905f);
}


static DRIVER_INIT( sftm110 )
{
	init_sftm_common(0x7a66, 0x9059);
}


static DRIVER_INIT( shufshot )
{
	init_program_rom();
	init_sound_speedup(0x2011, 0x906c);
	itech32_vram_height = 1024;
	itech32_planes = 1;
	is_drivedge = 0;

	itech020_prot_address = 0x111a;

	install_mem_write32_handler(0, 0x300000, 0x300003, itech020_color2_w);
	install_mem_write32_handler(0, 0x380000, 0x380003, itech020_color1_w);
	install_mem_read32_handler(0, 0x180800, 0x180803, trackball32_4bit_r);
	install_mem_read32_handler(0, 0x181000, 0x181003, trackball32_4bit_p2_r);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1992, timekill, 0,        timekill, timekill, timekill, ROT0, "Strata/Incredible Technologies", "Time Killers (v1.32)" )
GAME( 1992, timek131, timekill, timekill, timekill, timekill, ROT0, "Strata/Incredible Technologies", "Time Killers (v1.31)" )
GAME( 1993, hardyard, 0,        bloodstm, hardyard, hardyard, ROT0, "Strata/Incredible Technologies", "Hard Yardage (v1.20)" )
GAME( 1993, hardyd10, hardyard, bloodstm, hardyard, hardyard, ROT0, "Strata/Incredible Technologies", "Hard Yardage (v1.00)" )
GAME( 1994, bloodstm, 0,        bloodstm, bloodstm, bloodstm, ROT0, "Strata/Incredible Technologies", "Blood Storm (v2.22)" )
GAME( 1994, bloods22, bloodstm, bloodstm, bloodstm, bloodstm, ROT0, "Strata/Incredible Technologies", "Blood Storm (v2.20)" )
GAME( 1994, bloods21, bloodstm, bloodstm, bloodstm, bloodstm, ROT0, "Strata/Incredible Technologies", "Blood Storm (v2.10)" )
GAME( 1994, bloods11, bloodstm, bloodstm, bloodstm, bloodstm, ROT0, "Strata/Incredible Technologies", "Blood Storm (v1.10)" )
GAME( 1994, pairs,    0,        pairs,    pairs,    bloodstm, ROT0, "Strata/Incredible Technologies", "Pairs (09/07/94)" )
GAMEX(1994, drivedge, 0,        drivedge, drivedge, drivedge, ROT0, "Strata/Incredible Technologies", "Driver's Edge", GAME_NOT_WORKING )
GAME( 1995, wcbowl,   0,        wcbowl,   wcbowl,   wcbowl,   ROT0, "Incredible Technologies", "World Class Bowling (v1.2)" )
GAME( 1995, sftm,     0,        sftm,     sftm,     sftm,     ROT0, "Capcom/Incredible Technologies", "Street Fighter: The Movie (v1.12)" )
GAME( 1995, sftm110,  sftm,     sftm,     sftm,     sftm110,  ROT0, "Capcom/Incredible Technologies", "Street Fighter: The Movie (v1.10)" )
GAME( 1995, sftmj,    sftm,     sftm,     sftm,     sftm,     ROT0, "Capcom/Incredible Technologies", "Street Fighter: The Movie (v1.12N, Japan)" )
GAME( 1997, shufshot, 0,        sftm,     shufshot, shufshot, ROT0, "Strata/Incredible Technologies", "Shuffleshot (v1.39)" )
