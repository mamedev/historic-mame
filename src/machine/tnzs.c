#define MALLOC_CPU1_ROM_AREAS
/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

unsigned char *tnzs_objram,*tnzs_workram,*tnzs_vdcram,*tnzs_scrollram;
unsigned char *tnzs_cpu2ram;
int current_inputport;
int number_of_credits = 0;

void init_tnzs(void)
{
    current_inputport = -3;
}

int tnzs_objram_r(int offset)
{
	return tnzs_objram[offset];
}

int tnzs_workram_r(int offset)
{
	return tnzs_workram[offset];
}

int tnzs_vdcram_r(int offset)
{
#if 0
	if (errorlog) fprintf(errorlog, "read %02x from VDC %02x\n",
						  tnzs_vdcram[offset], offset);
#endif
	return tnzs_vdcram[offset];
}

int tnzs_cpu2ram_r(int offset)
{
	return tnzs_cpu2ram[offset];
}


int tnzs_inputport_r(int offset)
{
	int ret;

	if (offset == 0)
	{
        switch(current_inputport)
		{
			case -3: ret = 0x5a; break;
			case -2: ret = 0xa5; break;
			case -1: ret = 0x55; break;
            case 6: current_inputport = 0; /* fall through */
			case 0: ret = number_of_credits;
				break;
			default:
                ret = readinputport(current_inputport); break;
		}
#if 0
		if (errorlog) fprintf(errorlog, "$c000 is %02x (count %d)\n",
                              ret, current_inputport);
#endif
        current_inputport++;
		return ret;
	}
	else
		return (0x01);			/* 0xE* for TILT */
}

void tnzs_objram_w(int offset, int data)
{
	tnzs_objram[offset] = data;
}

void tnzs_workram_w(int offset, int data)
{
	tnzs_workram[offset] = data;
}

void tnzs_vdcram_w(int offset, int data)
{
#if 0
	if (errorlog) fprintf(errorlog,
						  "write %02x to VDC %02x\n", data, offset);
#endif
	tnzs_vdcram[offset] = data;
}

void tnzs_scrollram_w(int offset, int data)
{
	tnzs_scrollram[offset] = data;
}

void tnzs_cpu2ram_w(int offset, int data)
{
	tnzs_cpu2ram[offset] = data;
}

void tnzs_inputport_w(int offset, int data)
{
	if (offset == 0)
	{
        current_inputport = (current_inputport + 5) % 6;
		if (errorlog) fprintf(errorlog,
							  "writing %02x to 0xc000: count set back to %d\n",
                              data, current_inputport);
	}
}

int bank = 0;

extern unsigned char *banked_ram_0, *banked_ram_1;
#ifdef MALLOC_CPU1_ROM_AREAS
extern unsigned char *banked_rom_0, *banked_rom_1, *banked_rom_2, *banked_rom_3;
#endif

void tnzs_bankswitch_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (errorlog && (data < 0x10 || data > 0x17))
	{
		fprintf(errorlog, "WARNING: writing %02x to bankswitch\n", data);
		return;
	}

	bank = data & 7;

	if (bank == 0)
	{
		cpu_setbank(1, banked_ram_0);
	}
	else if (bank == 1)
	{
		cpu_setbank(1, banked_ram_1);
	}
	else
	{
		cpu_setbank(1, &RAM[0x8000 + 0x4000 * bank]);
	}
}

void tnzs_bankswitch1_w(int offset,int data)
{
#ifdef MALLOC_CPU1_ROM_AREAS
	switch (data & 3)
	{
		case 0:
			cpu_setbank(2, banked_rom_0);
			break;
		case 1:
			cpu_setbank(2, banked_rom_1);
			break;
		case 2:
			cpu_setbank(2, banked_rom_2);
			break;
		case 3:
			cpu_setbank(2, banked_rom_3);
			break;
	}
#else
	{
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
		cpu_setbank(2,&RAM[0x8000 + 0x2000 * (data & 3)]);
	}
#endif
}
