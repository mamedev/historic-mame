/**************************************************************************

	Interrupt System Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"
#include "includes/astrocde.h"

READ8_HANDLER( gorf_timer_r )
{
	static int Skip=0;
	unsigned char *RAM = memory_region(REGION_CPU1);


	if ((RAM[0x5A93]==160) || (RAM[0x5A93]==4)) 	/* INVADERS AND    */
	{												/* GALAXIAN SCREEN */
		if (activecpu_get_pc()==0x3086)
		{
		    if(--Skip==-1)
			{
				Skip=2;
			}
		}

	   	return Skip;
	}
	else
	{
		return RAM[0xD0A5];
	}

}


/****************************************************************************
 * Seawolf Controllers
 ****************************************************************************/

/*
 * Seawolf2 uses rotary controllers on input ports 10 + 11
 * each controller responds 0-63 for reading, with bit 7 as
 * fire button.
 *
 * The controllers look like they returns Grays binary,
 * so I use a table to translate my simple counter into it!
 */

static const int ControllerTable[64] = {
	0  , 1  , 3  , 2  , 6  , 7  , 5  , 4  ,
	12 , 13 , 15 , 14 , 10 , 11 , 9  , 8  ,
	24 , 25 , 27 , 26 , 30 , 31 , 29 , 28 ,
	20 , 21 , 23 , 22 , 18 , 19 , 17 , 16 ,
	48 , 49 , 51 , 50 , 54 , 55 , 53 , 52 ,
	60 , 61 , 63 , 62 , 58 , 59 , 57 , 56 ,
	40 , 41 , 43 , 42 , 46 , 47 , 45 , 44 ,
	36 , 37 , 39 , 38 , 34 , 35 , 33 , 32
};

READ8_HANDLER( seawolf2_controller1_r )
{
	return (input_port_0_r(0) & 0xc0) + ControllerTable[input_port_0_r(0) & 0x3f];
}

READ8_HANDLER( seawolf2_controller2_r )
{
	return (input_port_1_r(0) & 0xc0) + ControllerTable[input_port_1_r(0) & 0x3f];
}


static int ebases_trackball_select = 0;

WRITE8_HANDLER( ebases_trackball_select_w )
{
	ebases_trackball_select = data;
}

READ8_HANDLER( ebases_trackball_r )
{
	int ret = readinputport(3 + ebases_trackball_select);
#ifdef VERBOSE
	logerror("Port %d = %d\n", ebases_trackball_select, ret);
#endif
	return ret;
}

static UINT8 ram_write_enable = 0;
UINT8 *wow_protected_ram;
size_t wow_protected_ram_size;

WRITE8_HANDLER( wow_ramwrite_enable_w )
{
	ram_write_enable = 1;
}

READ8_HANDLER( wow_protected_ram_r )
{
	ram_write_enable = 0;
	return wow_protected_ram[offset];
}

/* protection is disabled by strapping of X33 */
WRITE8_HANDLER( wow_protected_ram_w )
{
#if 0
	if (offset < 0x400)
	{
		if (ram_write_enable)
#endif
			wow_protected_ram[offset] = data;
#if 0
	}
	else
	{
		wow_protected_ram[offset] = data;
	}
#endif
	ram_write_enable = 0;
}

#define NVRAM_SIZE 0x800
static UINT8 nvram[NVRAM_SIZE];

READ8_HANDLER( robby_nvram_r )
{
	ram_write_enable = 0;
	return nvram[offset];
}

WRITE8_HANDLER( robby_nvram_w )
{
	if (offset < 0x400)
	{
		if (ram_write_enable)
		{
			nvram[offset] = data;
		}
	}
	else
	{
		nvram[offset] = data;
	}

	ram_write_enable = 0;
}

READ8_HANDLER( profpac_nvram_r )
{
	ram_write_enable = 0;
	return nvram[offset];
}

WRITE8_HANDLER( profpac_nvram_w )
{
	if (offset < 0x200)
	{
		if (ram_write_enable)
		{
			nvram[offset] = data;
		}
	}
	else
	{
		nvram[offset] = data;
	}
	ram_write_enable = 0;
}

NVRAM_HANDLER( robby_nvram )
{
	if (read_or_write)
		mame_fwrite(file,nvram,NVRAM_SIZE);
	else if (file)
		mame_fread(file,nvram,NVRAM_SIZE);
	else
		memset(nvram,0,NVRAM_SIZE);
}

READ8_HANDLER( profpac_blank_r )
{
	return 0xff;
}

WRITE8_HANDLER( profpac_banksw_w )
{
	/* Note: There is a jumper which could map them from 0xa8-0xb6 instead */
	if (data >= 0x80) /* 0x80-0xa7 maps x1-x40 */
	{
		if (data < 0x8e)
		{
			/* 640K eprom board bank handling */
			int bankoffset = (data-0x80) * 0x4000;
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_BANK1);
			cpu_setbank(1, memory_region(REGION_USER1) + bankoffset);
		}
		else
		{
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, profpac_blank_r);
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK2);
		cpu_setbank(2, memory_region(REGION_CPU1) + 0x8000);
	}
	else
	{
		data &= 0xe0;
		if (data == 0x00)
		{
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, astrocde_videoram_r);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK2);
			cpu_setbank(2, memory_region(REGION_CPU1) + 0x8000);
		}
		else if (data == 0x20)
		{
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_BANK1);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK2);
			cpu_setbank(1, memory_region(REGION_CPU1) + 0x14000);
			cpu_setbank(2, memory_region(REGION_CPU1) + 0x18000);
		}
		else
		{
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, profpac_blank_r);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, profpac_blank_r);
		}
	}
}

READ8_HANDLER( spacezap_io_r )
{
	UINT8 cc1 = (offset>>8)&1;
	UINT8 cc2 = (offset>>9)&1;

	coin_counter_w(0,cc1);
	coin_counter_w(1,cc2);
	return input_port_3_r(offset & 0xff);
}

WRITE8_HANDLER( ebases_io_w )
{
	coin_counter_w(0,data&1);
}

