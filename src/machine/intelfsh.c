/*
	Intel 28F016S5 Flash ROM emulation

	(could also handle 28F004S5 and 28F008S5 with minor changes)
*/

#include "driver.h"
#include "intelfsh.h"

enum
{
	FM_NORMAL,	// normal read/write
	FM_READID,	// read ID
	FM_READSTATUS,	// read status
	FM_WRITEPART1,	// first half of programming, awaiting second
	FM_CLEARPART1,	// first half of clear, awaiting second
	FM_SETMASTER	// first half of set master lock, awaiting on/off
};

struct flash_chip 
{
	int flash_mode;
	int flash_master_lock;
	data8_t *flash_memory;
};

static struct flash_chip chips[FLASH_CHIPS_MAX];

static void intelflash_init(int chip)
{
	chips[chip].flash_mode = FM_NORMAL;
	chips[chip].flash_master_lock = 0;
	chips[chip].flash_memory = auto_malloc(2*1024*1024);	// 16Mbit
	memset(chips[chip].flash_memory, 0xff, 2*1024*1024);
}

static void nvram_handler_intelflash(int chip,mame_file *file,int read_or_write)
{
	if( chip >= FLASH_CHIPS_MAX )
	{
		logerror( "intelfsh: invalid chip %d\n", chip );
		return;
	}
	if (read_or_write)
	{
		mame_fwrite(file,chips[chip].flash_memory,2*1024*1024);
	}
	else
	{
		intelflash_init(chip);
		if (file)
		{
			mame_fread(file,chips[chip].flash_memory,2*1024*1024);
		}
	}
}

void nvram_handler_intelflash_0(mame_file *file,int read_or_write) { nvram_handler_intelflash( 0, file, read_or_write ); }
void nvram_handler_intelflash_1(mame_file *file,int read_or_write) { nvram_handler_intelflash( 1, file, read_or_write ); }
void nvram_handler_intelflash_2(mame_file *file,int read_or_write) { nvram_handler_intelflash( 2, file, read_or_write ); }
void nvram_handler_intelflash_3(mame_file *file,int read_or_write) { nvram_handler_intelflash( 3, file, read_or_write ); }

data8_t intelflash_read_byte(int chip, data32_t address)
{
	if( chip >= FLASH_CHIPS_MAX )
	{
		logerror( "intelfsh: invalid chip %d\n", chip );
		return 0;
	}
	switch (chips[chip].flash_mode)
	{
		default:
		case FM_NORMAL:
//			printf("F%d: R %06x = %02x\n", chip, address, chips[chip].flash_memory[address]);
			return chips[chip].flash_memory[address];
			break;
		case FM_READSTATUS:
//			chips[chip].flash_mode = FM_NORMAL;
			return 0x80;
			break;
		case FM_READID:
			switch (address)
			{
				case 0:	// maker ID
					return 0x89;	// Intel
					break;
				case 1:	// chip ID
					return 0xaa;	// 16 Mbit
					break;
				case 2:	// block lock config
					return 0;	// we don't support this yet
					break;
				case 3: // master lock config
					if (chips[chip].flash_master_lock)
					{
						return 1;
					}
					else
					{
						return 0;
					}
					break;
			}
			break;
	}

	return 0;
}

void intelflash_write_byte(int chip, data32_t address, data8_t data)
{
	if( chip >= FLASH_CHIPS_MAX )
	{
		logerror( "intelfsh: invalid chip %d\n", chip );
		return;
	}
	switch (chips[chip].flash_mode)
	{
		case FM_NORMAL:
		case FM_READSTATUS:
		case FM_READID:
			switch (data & 0xff)
			{
				case 0xff:	// reset chip mode
					chips[chip].flash_mode = FM_NORMAL;
					break;
				case 0x90:	// read ID
					chips[chip].flash_mode = FM_READID;
					break;
				case 0x40:
				case 0x10:	// program
					chips[chip].flash_mode = FM_WRITEPART1;
					break;
				case 0x50:	// clear status reg
					chips[chip].flash_mode = FM_READSTATUS;
					break;
				case 0x20:	// block erase
					chips[chip].flash_mode = FM_CLEARPART1;
					break;
				case 0x60:	// set master lock
					chips[chip].flash_mode = FM_SETMASTER;
					break;
				case 0x70:	// read status	
					chips[chip].flash_mode = FM_READSTATUS;
					break;
				default:
					printf("Unknown flash mode byte %x\n", data & 0xff);
					break;
			}
			break;
		case FM_WRITEPART1:
//			printf("F%d: W %x @ %x\n", chip, data, address);
			chips[chip].flash_memory[address] = data;
			chips[chip].flash_mode = FM_READSTATUS;
			break;
		case FM_CLEARPART1:
			if ((data & 0xff) == 0xd0)
			{
				// clear the 64k block containing the current address
				// to all 0xffs
				address &= 0xff0000;
				memset(&chips[chip].flash_memory[address], 0xff, 64*1024);

				chips[chip].flash_mode = FM_READSTATUS;
			}
			break;
		case FM_SETMASTER:
			if ((data & 0xff) == 0xf1)
			{
				chips[chip].flash_master_lock = 1;
			}
			else if ((data & 0xff) == 0xd0) 
			{
				chips[chip].flash_master_lock = 0;
			}
			chips[chip].flash_mode = FM_NORMAL;
			break;
	}
}
