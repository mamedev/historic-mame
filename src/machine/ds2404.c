/* Dallas DS2404 RTC/NVRAM */

#include "driver.h"

typedef enum {
	DS2404_STATE_IDLE = 1,				/* waiting for ROM command, in 1-wire mode */
	DS2404_STATE_COMMAND,				/* waiting for memory command */
	DS2404_STATE_ADDRESS1,				/* waiting for address bits 0-7 */
	DS2404_STATE_ADDRESS2,				/* waiting for address bits 8-15 */
	DS2404_STATE_OFFSET,				/* waiting for ending offset */
	DS2404_STATE_INIT_COMMAND,
	DS2404_STATE_READ_MEMORY,			/* Read Memory command active */
	DS2404_STATE_WRITE_SCRATCHPAD,		/* Write Scratchpad command active */
	DS2404_STATE_READ_SCRATCHPAD,		/* Read Scratchpad command active */
	DS2404_STATE_COPY_SCRATCHPAD,		/* Copy Scratchpad command active */
} DS2404_STATE;

typedef struct {
	UINT16 address;
	UINT16 offset;
	UINT16 end_offset;
	UINT8 a1, a2;
	UINT8 sram[512];	/* 4096 bits */
	UINT8 ram[32];		/* scratchpad ram, 256 bits */
	UINT64 rtc;			/* 40-bit RTC counter */
	UINT64 timer;		/* 40-bit interval timer */
	DS2404_STATE state[8];
	int state_ptr;
} DS2404;

static DS2404 ds2404;

static void ds2404_rom_cmd(UINT8 cmd)
{
	switch(cmd)
	{
		case 0xcc:		/* Skip ROM */
			ds2404.state[0] = DS2404_STATE_COMMAND;
			ds2404.state_ptr = 0;
			break;

		default:
			osd_die("DS2404: Unknown ROM command %02X\n", cmd);
			break;
	}
}

static void ds2404_cmd(UINT8 cmd)
{
	switch(cmd)
	{
		case 0x0f:		/* Write scratchpad */
			ds2404.state[0] = DS2404_STATE_ADDRESS1;
			ds2404.state[1] = DS2404_STATE_ADDRESS2;
			ds2404.state[2] = DS2404_STATE_INIT_COMMAND;
			ds2404.state[3] = DS2404_STATE_WRITE_SCRATCHPAD;
			ds2404.state_ptr = 0;
			break;

		case 0x55:		/* Copy scratchpad */
			ds2404.state[0] = DS2404_STATE_ADDRESS1;
			ds2404.state[1] = DS2404_STATE_ADDRESS2;
			ds2404.state[2] = DS2404_STATE_OFFSET;
			ds2404.state[3] = DS2404_STATE_INIT_COMMAND;
			ds2404.state[4] = DS2404_STATE_COPY_SCRATCHPAD;
			ds2404.state_ptr = 0;
			break;

		case 0xf0:		/* Read memory */
			ds2404.state[0] = DS2404_STATE_ADDRESS1;
			ds2404.state[1] = DS2404_STATE_ADDRESS2;
			ds2404.state[2] = DS2404_STATE_INIT_COMMAND;
			ds2404.state[3] = DS2404_STATE_READ_MEMORY;
			ds2404.state_ptr = 0;
			break;

		default:
			osd_die("DS2404: Unknown command %02X\n", cmd);
			break;
	}
}

static UINT8 ds2404_readmem(void)
{
	UINT8 value;

	if( ds2404.address < 0x200 ) {
		value = ds2404.sram[ds2404.address];
		return value;
	} else {

		switch( ds2404.address )
		{
			case 0x200:
				return 0;
			case 0x202:
				return (ds2404.rtc >> 8) & 0xff;
			case 0x203:
				return (ds2404.rtc >> 16) & 0xff;
			case 0x204:
				return (ds2404.rtc >> 24) & 0xff;
			case 0x205:
				return (ds2404.rtc >> 32) & 0xff;
			case 0x206:
				return ds2404.rtc & 0xff;
			default:
				return 0;
		}
	}
}

static void ds2404_writemem(UINT8 value)
{
	if( ds2404.address < 0x200 ) {
		ds2404.sram[ds2404.address] = value;
	} else {

		switch( ds2404.address )
		{
			case 0x202:
				ds2404.rtc &= ~(UINT64)0xff;
				ds2404.rtc |= (UINT64)value;
				break;
			case 0x203:
				ds2404.rtc &= ~(UINT64)0xff00;
				ds2404.rtc |= ((UINT64)value << 8);
				break;
			case 0x204:
				ds2404.rtc &= ~(UINT64)0xff0000;
				ds2404.rtc |= ((UINT64)value << 16);
				break;
			case 0x205:
				ds2404.rtc &= ~(UINT64)0xff000000;
				ds2404.rtc |= ((UINT64)value << 24);
				break;
			case 0x206:
				ds2404.rtc &= ~(UINT64)U64(0xff00000000);
				ds2404.rtc |= ((UINT64)value << 32);
				break;
		}
	}
}

WRITE8_HANDLER( DS2404_1W_reset_w )
{
	ds2404.state[0] = DS2404_STATE_IDLE;
	ds2404.state_ptr = 0;
}

WRITE8_HANDLER( DS2404_3W_reset_w )
{
	ds2404.state[0] = DS2404_STATE_COMMAND;
	ds2404.state_ptr = 0;
}

READ8_HANDLER( DS2404_data_r )
{
	UINT8 value;
	switch( ds2404.state[ds2404.state_ptr] )
	{
		case DS2404_STATE_IDLE:
		case DS2404_STATE_COMMAND:
		case DS2404_STATE_ADDRESS1:
		case DS2404_STATE_ADDRESS2:
		case DS2404_STATE_OFFSET:
		case DS2404_STATE_INIT_COMMAND:
			break;

		case DS2404_STATE_READ_MEMORY:
			value = ds2404_readmem();
			return value;

		case DS2404_STATE_READ_SCRATCHPAD:
			if( ds2404.offset < 0x20 ) {
				value = ds2404.ram[ds2404.offset];
				ds2404.offset++;
				return value;
			}
			break;

		case DS2404_STATE_WRITE_SCRATCHPAD:
			break;

		case DS2404_STATE_COPY_SCRATCHPAD:
			break;
	}
	return 0;
}

WRITE8_HANDLER( DS2404_data_w )
{
	int i;
		
	switch( ds2404.state[ds2404.state_ptr] )
	{
		case DS2404_STATE_IDLE:
			ds2404_rom_cmd(data & 0xff);
			break;

		case DS2404_STATE_COMMAND:
			ds2404_cmd(data & 0xff);
			break;

		case DS2404_STATE_ADDRESS1:
			ds2404.a1 = data & 0xff;
			ds2404.state_ptr++;
			break;

		case DS2404_STATE_ADDRESS2:
			ds2404.a2 = data & 0xff;
			ds2404.state_ptr++;
			break;

		case DS2404_STATE_OFFSET:
			ds2404.end_offset = data & 0xff;
			ds2404.state_ptr++;
			break;

		case DS2404_STATE_INIT_COMMAND:
			break;

		case DS2404_STATE_READ_MEMORY:
			break;

		case DS2404_STATE_READ_SCRATCHPAD:
			break;

		case DS2404_STATE_WRITE_SCRATCHPAD:
			if( ds2404.offset < 0x20 ) {
				ds2404.ram[ds2404.offset] = data & 0xff;
				ds2404.offset++;
			} else {
				/* Set OF flag */
			}
			break;

		case DS2404_STATE_COPY_SCRATCHPAD:
			break;
	}

	if( ds2404.state[ds2404.state_ptr] == DS2404_STATE_INIT_COMMAND ) {
		switch( ds2404.state[ds2404.state_ptr+1] )
		{
			case DS2404_STATE_IDLE:
			case DS2404_STATE_COMMAND:
			case DS2404_STATE_ADDRESS1:
			case DS2404_STATE_ADDRESS2:
			case DS2404_STATE_OFFSET:
			case DS2404_STATE_INIT_COMMAND:
				break;

			case DS2404_STATE_READ_MEMORY:
				ds2404.address = (ds2404.a2 << 8) | ds2404.a1;
				ds2404.address -= 2;
				break;

			case DS2404_STATE_WRITE_SCRATCHPAD:
				ds2404.address = (ds2404.a2 << 8) | ds2404.a1;
				ds2404.offset = ds2404.address & 0x1f;
				break;

			case DS2404_STATE_READ_SCRATCHPAD:
				ds2404.address = (ds2404.a2 << 8) | ds2404.a1;
				ds2404.offset = ds2404.address & 0x1f;
				break;

			case DS2404_STATE_COPY_SCRATCHPAD:
				ds2404.address = (ds2404.a2 << 8) | ds2404.a1;
				for( i=0; i < ds2404.end_offset; i++ ) {
					ds2404_writemem( ds2404.ram[i] );
					ds2404.address++;
				}
				break;
		}
		ds2404.state_ptr++;
	}
}

WRITE8_HANDLER( DS2404_clk_w )
{
	switch( ds2404.state[ds2404.state_ptr] )
	{
		case DS2404_STATE_IDLE:
		case DS2404_STATE_COMMAND:
		case DS2404_STATE_ADDRESS1:
		case DS2404_STATE_ADDRESS2:
		case DS2404_STATE_OFFSET:
		case DS2404_STATE_INIT_COMMAND:
			break;

		case DS2404_STATE_READ_MEMORY:
			ds2404.address++;
			break;

		case DS2404_STATE_READ_SCRATCHPAD:
			break;

		case DS2404_STATE_WRITE_SCRATCHPAD:
			break;

		case DS2404_STATE_COPY_SCRATCHPAD:
			break;
	}
}

void DS2404_init(void)
{

}

void DS2404_load(mame_file *file)
{
	mame_fread(file, ds2404.sram, 512);
	mame_fread(file, &ds2404.rtc, 8);
}

void DS2404_save(mame_file *file)
{
	mame_fwrite(file, ds2404.sram, 512);
	mame_fwrite(file, &ds2404.rtc, 8);
}
