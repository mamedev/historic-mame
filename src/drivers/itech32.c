/***************************************************************************

	Incredible Technologies/Strata system
	(32-bit blitter variant)

    driver by Aaron Giles

    Games supported:
		* Time Killers (2 sets)
		* Bloodstorm (3 sets)
		* Hard Yardage (2 sets)

	Not working:
		* Street Fighter: The Movie

	Games not supported because IT is still selling them:
		* World Class Bowling
		* Golden Tee 3D Golf
		* Golden Tee Golf '97 (2 sets)
		* Golden Tee Golf '98
		* Golden Tee Golf '99
		* Golden Tee Golf 2K

	Games that might use this hardware, but have no known (good) dumps:
		* Dyno-Bop
		* Poker Dice
		* Neck & Neck
		* Driver's Edge
		* Pairs

	Known issues:
		* Street Fighter: The Movie has 8 surface-mount ROMs that aren't dumped

****************************************************************************

	Memory map TBD

***************************************************************************/


#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "machine/ticket.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/tms34061.h"
#include <math.h>


#define FULL_LOGGING	0

#define CLOCK_8MHz		(8000000)
#define CLOCK_12MHz		(12000000)
#define CLOCK_25MHz		(25000000)



/*************************************
 *
 *	External definitions
 *
 *************************************/

/* video driver data & functions */
extern data16_t *itech32_video;
extern UINT8 itech32_planes;
extern UINT16 itech32_vram_height;

int itech32_vh_start(void);
void itech32_vh_stop(void);
void itech32_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

WRITE16_HANDLER( timekill_paletteram_w );
WRITE16_HANDLER( bloodstm_paletteram_w );
WRITE32_HANDLER( itech020_paletteram_w );

WRITE16_HANDLER( itech32_video_w );
WRITE16_HANDLER( bloodstm_video_w );
WRITE32_HANDLER( itech020_video_w );
READ16_HANDLER( itech32_video_r );
READ16_HANDLER( bloodstm_video_r );
READ32_HANDLER( itech020_video_r );

WRITE16_HANDLER( timekill_colora_w );
WRITE16_HANDLER( timekill_colorbc_w );
WRITE16_HANDLER( timekill_intensity_w );
WRITE16_HANDLER( bloodstm_color1_w );
WRITE16_HANDLER( bloodstm_color2_w );
WRITE16_HANDLER( bloodstm_plane_w );
WRITE32_HANDLER( itech020_color1_w );
WRITE32_HANDLER( itech020_color2_w );
WRITE32_HANDLER( itech020_plane_w );



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


void itech32_update_interrupts_fast(int vint, int xint, int qint)
{
	int level = determine_irq_state(vint, xint, qint);

	/* update it */
	if (level)
		(*cpuintf[Machine->drv->cpu[0].cpu_type & ~CPU_FLAGS_MASK].set_irq_line)(level, ASSERT_LINE);
	else
		(*cpuintf[Machine->drv->cpu[0].cpu_type & ~CPU_FLAGS_MASK].set_irq_line)(7, CLEAR_LINE);
}


static int generate_int1(void)
{
	/* signal the NMI */
	itech32_update_interrupts(1, -1, -1);

	if (FULL_LOGGING) logerror("------------ VBLANK (%d) --------------\n", cpu_getscanline());
	return ignore_interrupt();
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

static void init_machine(void)
{
	vint_state = xint_state = qint_state = 0;
	sound_data = 0;
	sound_int_state = 0;

	/* reset the VIA chip (if used) */
	via6522_timer_count[0] = via6522_timer_count[1] = 0;
	via6522_timer[0] = via6522_timer[1] = 0;
	via6522_int_state = 0;

	/* reset the ticket dispenser */
	ticket_dispenser_init(200, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);
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
	int lower = readinputport(6);
	int upper = readinputport(7);
	int result;

	result = (lower & 15) | ((upper & 15) << 4);
	return result | (result << 16);
}



/*************************************
 *
 *	Protection?
 *
 *************************************/

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
	if (ACCESSING_MSW32)
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
			if (via6522_timer[0])
				timer_remove(via6522_timer[0]);
			via6522_timer[0] = timer_pulse(TIME_IN_HZ(CLOCK_8MHz/4) * (double)via6522_timer_count[0], 0, via6522_timer_callback);

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

static int generate_firq(void)
{
	cpu_set_irq_line(1, M6809_FIRQ_LINE, ASSERT_LINE);
	return ignore_interrupt();
}


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
	if (sound_speedup_data[0] == sound_speedup_data[1] && cpu_getpreviouspc() == sound_speedup_pc)
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
	return input_port_0_word_r(offset) << 16;
}

static READ32_HANDLER( input_port_1_msw_r )
{
	return input_port_1_word_r(offset) << 16;
}

static READ32_HANDLER( input_port_2_msw_r )
{
	return input_port_2_word_r(offset) << 16;
}

static READ32_HANDLER( input_port_3_msw_r )
{
	return input_port_3_word_r(offset) << 16;
}

static READ32_HANDLER( input_port_4_msw_r )
{
	return special_port4_r(offset) << 16;
}

static READ32_HANDLER( input_port_5_msw_r )
{
	return input_port_5_word_r(offset) << 16;
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

static void nvram_handler(void *file, int read_or_write)
{
	int i;

	if (read_or_write)
		osd_fwrite(file, main_ram, main_ram_size);
	else if (file)
		osd_fread(file, main_ram, main_ram_size);
	else
		for (i = 0x80; i < main_ram_size; i++)
			((UINT8 *)main_ram)[i] = rand();
}


static void nvram_handler_itech020(void *file, int read_or_write)
{
	int i;

	if (read_or_write)
		osd_fwrite(file, nvram, nvram_size);
	else if (file)
		osd_fread(file, nvram, nvram_size);
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
	{ 0x600000, 0x601fff, MRA32_RAM },
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
	{ 0x600000, 0x601fff, MWA32_RAM, &nvram, &nvram_size },
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

#define PORT_SERVICE_NO_TOGGLE(mask,default)	\
	PORT_BITX(    mask, mask & default, IPT_SERVICE1, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

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
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
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
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
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
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0020, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0040, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0080, DEF_STR( On ))

	PORT_START	/* 780000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( sftm )
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
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 200000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 280000 */
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0000, "Rotate Trackball" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Cabinet ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x0080, 0x0000, "Force Test Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0080, DEF_STR( On ))

	PORT_START	/* 780000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
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

#define ITECH_DRIVER(NAME, CPUTYPE, CPUCLOCK, MAINMEM, SOUNDMEM, SOUNDINT, COLORS, YMAX, NVRAM)	\
static struct MachineDriver machine_driver_##NAME =								\
{																				\
	/* basic machine hardware */												\
	{																			\
		{																		\
			CPUTYPE,															\
			CPUCLOCK,															\
			MAINMEM##_readmem,MAINMEM##_writemem,0,0,							\
			generate_int1,1														\
		},																		\
		{																		\
			CPU_M6809,															\
			CLOCK_8MHz/4,														\
			SOUNDMEM##_readmem,SOUNDMEM##_writemem,0,0,							\
			SOUNDINT,4															\
		}																		\
	},																			\
	60,(int)(((263. - 240.) / 263.) * 1000000. / 60.),							\
	1,																			\
	init_machine,																\
																				\
	/* video hardware */														\
	384, 240, { 0, 383, 0, YMAX },												\
	0,																			\
	COLORS,COLORS,																\
	0,																			\
																				\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,	\
	0,																			\
	itech32_vh_start,															\
	itech32_vh_stop,															\
	itech32_vh_screenrefresh,													\
																				\
	/* sound hardware */														\
	0,0,0,0,																	\
	{																			\
		{ SOUND_ES5506, &es5506_interface }										\
	},																			\
	NVRAM																		\
};


/*           NAME,     CPU,          CPUCLOCK,    MAINMEM,  SOUNDMEM,  SOUNDINT,         COLORS,YMAX,NVRAM) */
ITECH_DRIVER(timekill, CPU_M68000,   CLOCK_12MHz, timekill, sound,     ignore_interrupt,  8192, 239, nvram_handler)
ITECH_DRIVER(bloodstm, CPU_M68000,   CLOCK_12MHz, bloodstm, sound,     ignore_interrupt, 32768, 239, nvram_handler)
ITECH_DRIVER(sftm,     CPU_M68EC020, CLOCK_25MHz, itech020, sound_020, generate_firq,    32768, 254, nvram_handler_itech020)



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( timekill )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )

	ROM_REGION16_BE( 0x80000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "tk00v132.u54", 0x00000, 0x40000, 0x68c74b81 )
	ROM_LOAD16_BYTE( "tk00v132.u53", 0x00001, 0x40000, 0x2158d8ef )

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
	ROM_LOAD16_BYTE( "tk00v131.u53", 0x00001, 0x40000, 0xc29137ec )

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


ROM_START( sftm )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x100000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "prog0.bin", 0x00000, 0x40000, 0x00c0c63c )
	ROM_LOAD32_BYTE( "prog1.bin", 0x00001, 0x40000, 0xd4d2a67e )
	ROM_LOAD32_BYTE( "prog2.bin", 0x00002, 0x40000, 0xd7b36c92 )
	ROM_LOAD32_BYTE( "prog3.bin", 0x00003, 0x40000, 0xbe3efdbd )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )
	ROM_LOAD( "snd-v10.bin", 0x10000, 0x38000, 0x10d85366 )
	ROM_CONTINUE(            0x08000, 0x08000 )

	ROM_REGION( 0x3080000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "grom0_0.bin", 0x0000000, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom0_1.bin", 0x0000001, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom0_2.bin", 0x0000002, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom0_3.bin", 0x0000003, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom1_0.bin", 0x1000000, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom1_1.bin", 0x1000001, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom1_2.bin", 0x1000002, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom1_3.bin", 0x1000003, 0x400000, 0x00000000 )
	ROM_LOAD32_BYTE( "grom3_0.bin", 0x3000000, 0x020000, 0x3e1f76f7 )
	ROM_LOAD32_BYTE( "grom3_1.bin", 0x3000001, 0x020000, 0x578054b6 )
	ROM_LOAD32_BYTE( "grom3_2.bin", 0x3000002, 0x020000, 0x9af2f698 )
	ROM_LOAD32_BYTE( "grom3_3.bin", 0x3000003, 0x020000, 0xcd38d1d6 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom0.bin",  0x000000, 0x200000, 0x00000000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "srom3.bin",  0x000000, 0x080000, 0x4f181534 )
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


static void init_timekill(void)
{
	init_program_rom();
	init_sound_speedup(0x2010, 0x8c54);
	itech32_vram_height = 512;
	itech32_planes = 2;
}

static void init_hardyard(void)
{
	init_program_rom();
	init_sound_speedup(0x2010, 0x8e16);
	itech32_vram_height = 1024;
	itech32_planes = 1;
}

static void init_bloodstm(void)
{
	init_program_rom();
	init_sound_speedup(0x2011, 0x8ebf);
	itech32_vram_height = 1024;
	itech32_planes = 1;
}

static void init_sftm(void)
{
	init_program_rom();
	init_sound_speedup(0x2011, 0x9059);
	itech32_vram_height = 1024;
	itech32_planes = 2;

	itech020_prot_address = 0x7a66;
	memset(memory_region(REGION_GFX1), 0x81, 0x3000000);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME ( 1992, timekill, 0,        timekill, timekill, timekill, ROT0_16BIT,  "Strata/Incredible Technologies", "Time Killers (v1.32)" )
GAME ( 1992, timek131, timekill, timekill, timekill, timekill, ROT0_16BIT,  "Strata/Incredible Technologies", "Time Killers (v1.31)" )
GAME ( 1993, hardyard, 0,        bloodstm, hardyard, hardyard, ROT0_16BIT,  "Strata/Incredible Technologies", "Hard Yardage (v1.20)" )
GAME ( 1993, hardyd10, hardyard, bloodstm, hardyard, hardyard, ROT0_16BIT,  "Strata/Incredible Technologies", "Hard Yardage (v1.00)" )
GAME ( 1994, bloodstm, 0,        bloodstm, bloodstm, bloodstm, ROT0_16BIT,  "Strata/Incredible Technologies", "Blood Storm (v2.22)" )
GAME ( 1994, bloods22, bloodstm, bloodstm, bloodstm, bloodstm, ROT0_16BIT,  "Strata/Incredible Technologies", "Blood Storm (v2.20)" )
GAME ( 1994, bloods21, bloodstm, bloodstm, bloodstm, bloodstm, ROT0_16BIT,  "Strata/Incredible Technologies", "Blood Storm (v2.10)" )

GAMEX( 1995, sftm,     0,        sftm,     sftm,     sftm,     ROT0_16BIT,  "Capcom/Incredible Technologies", "Street Fighter: The Movie", GAME_NOT_WORKING )
