/*************************************************************************

	Driver for Williams/Midway Wolf-unit games.

	Hints for finding speedups:

		search disassembly for ": CAF9"

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/m6809/m6809.h"
#include "sndhrdw/williams.h"
#include "wmswolfu.h"


/* speedup installation macros */
#define INSTALL_SPEEDUP_1_16BIT(addr, pc, spin1, offs1, offs2) \
	wms_speedup_pc = (pc); \
	wms_speedup_offset = ((addr) & 0x10) >> 4; \
	wms_speedup_spin[0] = spin1; \
	wms_speedup_spin[1] = offs1; \
	wms_speedup_spin[2] = offs2; \
	wms_speedup_base = install_mem_read16_handler(0, TOBYTE((addr) & ~0x1f), TOBYTE((addr) | 0x1f), wms_generic_speedup_1_16bit);

#define INSTALL_SPEEDUP_3(addr, pc, spin1, spin2, spin3) \
	wms_speedup_pc = (pc); \
	wms_speedup_offset = ((addr) & 0x10) >> 4; \
	wms_speedup_spin[0] = spin1; \
	wms_speedup_spin[1] = spin2; \
	wms_speedup_spin[2] = spin3; \
	wms_speedup_base = install_mem_read16_handler(0, TOBYTE((addr) & ~0x1f), TOBYTE((addr) | 0x1f), wms_generic_speedup_3);

#define INSTALL_SPEEDUP_1_ADDRESS( addr, pc ) \
	wms_speedup_pc = (pc); \
	wms_speedup_offset = ((addr) & 0x10) >> 4; \
	wms_speedup_base = install_mem_read16_handler(0, TOBYTE((addr) & ~0x1f), TOBYTE((addr) | 0x1f), wms_generic_speedup_1_address);



/* code-related variables */
       UINT8 *	wms_wolfu_decode_memory;

/* CMOS-related variables */
static UINT8	cmos_write_enable;

/* I/O-related variables */
static data16_t	iodata[8];
static UINT8	ioshuffle[16];
static UINT8	revx_analog_port;

/* protection-related variables */
static UINT8	security_data[16];
static UINT8	security_buffer;
static UINT8	security_index;
static UINT8	security_status;
static UINT8	security_bits;

/* UART-related variables */
static UINT8	uart[8];


/* prototype */
static READ16_HANDLER( wms_wolfu_sound_state_r );
static void revx_dcs_notify(int state);



/*************************************
 *
 *	CMOS reads/writes
 *
 *************************************/

WRITE16_HANDLER( wms_wolfu_cmos_enable_w )
{
	cmos_write_enable = 1;
}


WRITE16_HANDLER( wms_wolfu_cmos_w )
{
	if (cmos_write_enable)
	{
		COMBINE_DATA(&((data16_t *)generic_nvram)[offset]);
		cmos_write_enable = 0;
	}
	else
	{
		logerror("%08X:Unexpected CMOS W @ %05X\n", activecpu_get_pc(), offset);
		usrintf_showmessage("Bad CMOS write");
	}
}


WRITE16_HANDLER( revx_cmos_w )
{
	COMBINE_DATA(&((data16_t *)generic_nvram)[offset]);
}


READ16_HANDLER( wms_wolfu_cmos_r )
{
	return ((data16_t *)generic_nvram)[offset];
}



/*************************************
 *
 *	General I/O writes
 *
 *************************************/

WRITE16_HANDLER( wms_wolfu_io_w )
{
	int oldword, newword;

	offset %= 8;
	oldword = iodata[offset];
	newword = oldword;
	COMBINE_DATA(&newword);

	switch (offset)
	{
		case 1:
			logerror("%08X:Control W @ %05X = %04X\n", activecpu_get_pc(), offset, data);

			/* bit 4 reset sound CPU */
			williams_dcs_reset_w(newword & 0x10);

			/* bit 5 (active low) reset security chip */
			if (!(oldword & 0x20) && (newword & 0x20))
			{
				security_index = 0;
				security_status = 0;
				security_buffer = 0;
			}
			break;

		case 3:
			/* watchdog reset */
			/* MK3 resets with this enabled */
/*			watchdog_reset_w(0,0);*/
			break;

		default:
			logerror("%08X:Unknown I/O write to %d = %04X\n", activecpu_get_pc(), offset, data);
			break;
	}
	iodata[offset] = newword;
}


WRITE16_HANDLER( revx_io_w )
{
	int oldword, newword;

	offset = (offset / 2) % 8;
	oldword = iodata[offset];
	newword = oldword;
	COMBINE_DATA(&newword);

	switch (offset)
	{
		case 2:
			/* watchdog reset */
//			watchdog_reset_w(0,0);
			break;

		default:
			logerror("%08X:I/O write to %d = %04X\n", activecpu_get_pc(), offset, data);
//			logerror("%08X:Unknown I/O write to %d = %04X\n", activecpu_get_pc(), offset, data);
			break;
	}
	iodata[offset] = newword;
}


WRITE16_HANDLER( revx_unknown_w )
{
	int offs = offset / 0x40000;
	if (ACCESSING_LSB && offset % 0x40000 == 0)
		logerror("%08X:revx_unknown_w @ %d = %02X\n", activecpu_get_pc(), offs, data & 0xff);
}


/*************************************
 *
 *	General I/O reads
 *
 *************************************/

READ16_HANDLER( wms_wolfu_io_r )
{
	/* apply I/O shuffling */
	offset = ioshuffle[offset % 16];

	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			return readinputport(offset);

		case 4:
			return (security_status << 12) | wms_wolfu_sound_state_r(0,0);

		default:
			logerror("%08X:Unknown I/O read from %d\n", activecpu_get_pc(), offset);
			break;
	}
	return ~0;
}


READ16_HANDLER( revx_io_r )
{
	offset = (offset / 2) % 8;

	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			return readinputport(offset);

		default:
			logerror("%08X:Unknown I/O read from %d\n", activecpu_get_pc(), offset);
			break;
	}
	return ~0;
}


READ16_HANDLER( revx_analog_r )
{
	return readinputport(revx_analog_port);
}


WRITE16_HANDLER( revx_analog_select_w )
{
	if (offset == 0 && ACCESSING_LSB)
		revx_analog_port = data - 8 + 4;
}


READ16_HANDLER( revx_status_r )
{
	/* low bit indicates whether the ADC is done reading the current input */
	return (security_status << 1) | 1;
}



/*************************************
 *
 *	Revolution X UART
 *
 *************************************/

void revx_dcs_notify(int state)
{
	/* only signal if not in loopback state */
	if (uart[1] != 0x66)
		cpu_set_irq_line(1, 1, state ? ASSERT_LINE : CLEAR_LINE);
}


READ16_HANDLER( revx_uart_r )
{
	int result = 0;

	/* convert to a byte offset */
	if (offset & 1)
		return 0;
	offset /= 2;

	/* switch off the offset */
	switch (offset)
	{
		case 0:	/* register 0 must return 0x13 in order to pass the self test */
			result = 0x13;
			break;

		case 1:	/* register 1 contains the status */

			/* loopback case: data always ready, and always ok to send */
			if (uart[1] == 0x66)
				result |= 5;

			/* non-loopback case: bit 0 means data ready, bit 2 means ok to send */
			else
			{
				int temp = wms_wolfu_sound_state_r(0, 0);
				result |= (temp & 0x800) >> 9;
				result |= (~temp & 0x400) >> 10;
				timer_set(TIME_NOW, 0, 0);
			}
			break;

		case 3:	/* register 3 contains the data read */

			/* loopback case: feed back last data wrtten */
			if (uart[1] == 0x66)
				result = uart[3];

			/* non-loopback case: read from the DCS system */
			else
				result = wms_wolfu_sound_r(0, 0);
			break;

		case 5:	/* register 5 seems to be like 3, but with in/out swapped */

			/* loopback case: data always ready, and always ok to send */
			if (uart[1] == 0x66)
				result |= 5;

			/* non-loopback case: bit 0 means data ready, bit 2 means ok to send */
			else
			{
				int temp = wms_wolfu_sound_state_r(0, 0);
				result |= (temp & 0x800) >> 11;
				result |= (~temp & 0x400) >> 8;
				timer_set(TIME_NOW, 0, 0);
			}
			break;

		default: /* everyone else reads themselves */
			result = uart[offset];
			break;
	}

/*	logerror("%08X:UART R @ %X = %02X\n", activecpu_get_pc(), offset, result);*/
	return result;
}


WRITE16_HANDLER( revx_uart_w )
{
	/* convert to a byte offset, ignoring MSB writes */
	if ((offset & 1) || !ACCESSING_LSB)
		return;
	offset /= 2;
	data &= 0xff;

	/* switch off the offset */
	switch (offset)
	{
		case 3:	/* register 3 contains the data to be sent */

			/* loopback case: don't feed through */
			if (uart[1] == 0x66)
				uart[3] = data;

			/* non-loopback case: send to the DCS system */
			else
				wms_wolfu_sound_w(0, data, mem_mask);
			break;

		case 5:	/* register 5 write seems to reset things */
			williams_dcs_data_r();
			break;

		default: /* everyone else just stores themselves */
			uart[offset] = data;
			break;
	}

/*	logerror("%08X:UART W @ %X = %02X\n", activecpu_get_pc(), offset, data);*/
}



/*************************************
 *
 *	Serial number encoding
 *
 *************************************/

static void generate_serial(int upper)
{
	int year = atoi(Machine->gamedrv->year), month = 12, day = 11;
	UINT32 serial_number, temp;
	UINT8 serial_digit[9];

	serial_number = 123456;
	serial_number += upper * 1000000;

	serial_digit[0] = (serial_number / 100000000) % 10;
	serial_digit[1] = (serial_number / 10000000) % 10;
	serial_digit[2] = (serial_number / 1000000) % 10;
	serial_digit[3] = (serial_number / 100000) % 10;
	serial_digit[4] = (serial_number / 10000) % 10;
	serial_digit[5] = (serial_number / 1000) % 10;
	serial_digit[6] = (serial_number / 100) % 10;
	serial_digit[7] = (serial_number / 10) % 10;
	serial_digit[8] = (serial_number / 1) % 10;

	security_data[12] = rand() & 0xff;
	security_data[13] = rand() & 0xff;

	security_data[14] = 0; /* ??? */
	security_data[15] = 0; /* ??? */

	temp = 0x174 * (year - 1980) + 0x1f * (month - 1) + day;
	security_data[10] = (temp >> 8) & 0xff;
	security_data[11] = temp & 0xff;

	temp = serial_digit[4] + serial_digit[7] * 10 + serial_digit[1] * 100;
	temp = (temp + 5 * security_data[13]) * 0x1bcd + 0x1f3f0;
	security_data[7] = temp & 0xff;
	security_data[8] = (temp >> 8) & 0xff;
	security_data[9] = (temp >> 16) & 0xff;

	temp = serial_digit[6] + serial_digit[8] * 10 + serial_digit[0] * 100 + serial_digit[2] * 10000;
	temp = (temp + 2 * security_data[13] + security_data[12]) * 0x107f + 0x71e259;
	security_data[3] = temp & 0xff;
	security_data[4] = (temp >> 8) & 0xff;
	security_data[5] = (temp >> 16) & 0xff;
	security_data[6] = (temp >> 24) & 0xff;

	temp = serial_digit[5] * 10 + serial_digit[3] * 100;
	temp = (temp + security_data[12]) * 0x245 + 0x3d74;
	security_data[0] = temp & 0xff;
	security_data[1] = (temp >> 8) & 0xff;
	security_data[2] = (temp >> 16) & 0xff;
}



/*************************************
 *
 *	Generic driver init
 *
 *************************************/

static void init_wolfu_generic(void)
{
	UINT8 *base;
	int i, j;

	/* set up code ROMs */
	memcpy(wms_code_rom, memory_region(REGION_USER1), memory_region_length(REGION_USER1));

	/* load the graphics ROMs -- quadruples */
	wms_gfx_rom = base = memory_region(REGION_GFX1);
	for (i = 0; i < memory_region_length(REGION_GFX1) / 0x400000; i++)
	{
		memcpy(wms_wolfu_decode_memory, base, 0x400000);
		for (j = 0; j < 0x100000; j++)
		{
			*base++ = wms_wolfu_decode_memory[0x000000 + j];
			*base++ = wms_wolfu_decode_memory[0x100000 + j];
			*base++ = wms_wolfu_decode_memory[0x200000 + j];
			*base++ = wms_wolfu_decode_memory[0x300000 + j];
		}
	}

	/* init sound */
	williams_dcs_init();
}




/*************************************
 *
 *	Wolf-unit init (DCS)
 *
 * 	music: ADSP2101
 *
 *************************************/

/********************** Mortal Kombat 3 **********************/

static void init_mk3_common(void)
{
	/* common init */
	init_wolfu_generic();

	/* serial prefixes 439, 528 */
	generate_serial(528);
}

DRIVER_INIT( mk3 )
{
	init_mk3_common();
	INSTALL_SPEEDUP_3(0x1069bd0, 0xff926810, 0x105dc10, 0x105dc30, 0x105dc50);
}

DRIVER_INIT( mk3r20 )
{
	init_mk3_common();
	INSTALL_SPEEDUP_3(0x1069bd0, 0xff926790, 0x105dc10, 0x105dc30, 0x105dc50);
}

DRIVER_INIT( mk3r10 )
{
	init_mk3_common();
	INSTALL_SPEEDUP_3(0x1078e50, 0xff923e30, 0x105d490, 0x105d4b0, 0x105d4d0);
}

DRIVER_INIT( umk3 )
{
	init_mk3_common();
	INSTALL_SPEEDUP_3(0x106a0e0, 0xff9696a0, 0x105dc10, 0x105dc30, 0x105dc50);
}

DRIVER_INIT( umk3r11 )
{
	init_mk3_common();
	INSTALL_SPEEDUP_3(0x106a0e0, 0xff969680, 0x105dc10, 0x105dc30, 0x105dc50);
}


/********************** 2 On 2 Open Ice Challenge **********************/

DRIVER_INIT( openice )
{
	/* common init */
	init_wolfu_generic();

	/* serial prefixes 438, 528 */
	generate_serial(528);
}


/********************** NBA Hangtime & NBA Maximum Hangtime **********************/

DRIVER_INIT( nbahangt )
{
	/* common init */
	init_wolfu_generic();

	/* serial prefixes 459, 470, 528 */
	generate_serial(528);

	INSTALL_SPEEDUP_1_16BIT(0x10731f0, 0xff8a5510, 0x1002040, 0xd0, 0xb0);
}


/********************** WWF Wrestlemania **********************/

static READ16_HANDLER( wms_generic_speedup_1_address )
{
	data16_t value = wms_speedup_base[offset];

	/* just return if this isn't the offset we're looking for */
	if (offset != wms_speedup_offset)
		return value;

	/* suspend cpu if it's waiting for an interrupt */
	if (activecpu_get_pc() == wms_speedup_pc && !value)
		cpu_spinuntil_int();

	return value;
}

static WRITE16_HANDLER( wwfmania_io_0_w )
{
	int i;

	/* start with the originals */
	for (i = 0; i < 16; i++)
		ioshuffle[i] = i % 8;

	/* based on the data written, shuffle */
	switch (data)
	{
		case 0:
			break;

		case 1:
			ioshuffle[4] = 0;
			ioshuffle[8] = 1;
			ioshuffle[1] = 2;
			ioshuffle[9] = 3;
			ioshuffle[2] = 4;
			break;

		case 2:
			ioshuffle[8] = 0;
			ioshuffle[2] = 1;
			ioshuffle[4] = 2;
			ioshuffle[6] = 3;
			ioshuffle[1] = 4;
			break;

		case 3:
			ioshuffle[1] = 0;
			ioshuffle[8] = 1;
			ioshuffle[2] = 2;
			ioshuffle[10] = 3;
			ioshuffle[5] = 4;
			break;

		case 4:
			ioshuffle[2] = 0;
			ioshuffle[4] = 1;
			ioshuffle[1] = 2;
			ioshuffle[7] = 3;
			ioshuffle[8] = 4;
			break;
	}
	logerror("Changed I/O swiching to %d\n", data);
}

DRIVER_INIT( wwfmania )
{
	/* common init */
	init_wolfu_generic();

	/* enable I/O shuffling */
	install_mem_write16_handler(0, TOBYTE(0x01800000), TOBYTE(0x0180000f), wwfmania_io_0_w);

	/* serial prefixes 430, 528 */
	generate_serial(528);

	INSTALL_SPEEDUP_1_ADDRESS(0x105c250, 0xff8189d0);
}


/********************** Rampage World Tour **********************/

DRIVER_INIT( rmpgwt )
{
	/* common init */
	init_wolfu_generic();

	/* serial prefixes 465, 528 */
	generate_serial(528);
}


/********************** Revolution X **********************/

DRIVER_INIT( revx )
{
	UINT8 *base;
	int i, j;

	/* common init */
	/* set up code ROMs */
	memcpy(wms_code_rom, memory_region(REGION_USER1), memory_region_length(REGION_USER1));

	/* load the graphics ROMs -- quadruples */
	wms_gfx_rom = base = memory_region(REGION_GFX1);
	for (i = 0; i < memory_region_length(REGION_GFX1) / 0x200000; i++)
	{
		memcpy(wms_wolfu_decode_memory, base, 0x200000);
		for (j = 0; j < 0x80000; j++)
		{
			*base++ = wms_wolfu_decode_memory[0x000000 + j];
			*base++ = wms_wolfu_decode_memory[0x080000 + j];
			*base++ = wms_wolfu_decode_memory[0x100000 + j];
			*base++ = wms_wolfu_decode_memory[0x180000 + j];
		}
	}

	/* init sound */
	williams_dcs_init();

	/* serial prefixes 419, 420 */
	generate_serial(419);
}



/*************************************
 *
 *	Machine init
 *
 *************************************/

MACHINE_INIT( wms_wolfu )
{
	int i;

	/* reset sound */
	williams_dcs_reset_w(1);
	williams_dcs_reset_w(0);

	/* reset I/O shuffling */
	for (i = 0; i < 16; i++)
		ioshuffle[i] = i % 8;
}


MACHINE_INIT( revx )
{
	machine_init_wms_wolfu();
	williams_dcs_set_notify(revx_dcs_notify);
}



/*************************************
 *
 *	Security chip I/O
 *
 *************************************/

READ16_HANDLER( wms_wolfu_security_r )
{
	logerror("%08X:security R = %04X\n", activecpu_get_pc(), security_buffer);
	security_status = 1;
	return security_buffer;
}


WRITE16_HANDLER( wms_wolfu_security_w )
{
	if (offset == 0 && ACCESSING_LSB)
	{
		logerror("%08X:security W = %04X\n", activecpu_get_pc(), data);

		/* status seems to reflect the clock bit */
		security_status = (data >> 4) & 1;

		/* on the falling edge, clock the next data byte through */
		if (!(data & 0x10))
		{
			/* the self-test writes 1F, 0F, and expects to read an F in the low 4 bits */
			if (data & 0x0f)
				security_buffer = data;
			else
				security_buffer = security_data[security_index++ % sizeof(security_data)];
		}
	}
}


WRITE16_HANDLER( revx_security_w )
{
	if (ACCESSING_LSB)
		security_bits = data & 0x0f;
}


WRITE16_HANDLER( revx_security_clock_w )
{
	if (ACCESSING_LSB)
		wms_wolfu_security_w(offset, ((~data & 2) << 3) | security_bits, 0);
}



/*************************************
 *
 *	Sound write handlers
 *
 *************************************/

READ16_HANDLER( wms_wolfu_sound_r )
{
	logerror("%08X:Sound read\n", activecpu_get_pc());

	if (Machine->sample_rate)
		return williams_dcs_data_r();
	return 0x0000;
}


READ16_HANDLER( wms_wolfu_sound_state_r )
{
	if (Machine->sample_rate)
		return williams_dcs_control_r();
	return 0x0800;
}


WRITE16_HANDLER( wms_wolfu_sound_w )
{
	/* check for out-of-bounds accesses */
	if (offset)
	{
		logerror("%08X:Unexpected write to sound (hi) = %04X\n", activecpu_get_pc(), data);
		return;
	}

	/* call through based on the sound type */
	if (ACCESSING_LSB)
	{
		logerror("%08X:Sound write = %04X\n", activecpu_get_pc(), data);
		williams_dcs_data_w(data & 0xff);
	}
}
