/***************************************************************************

Atari Audio Board II
--------------------

6502 MEMORY MAP

Function                                  Address     R/W  Data
---------------------------------------------------------------
Program RAM                               0000-1FFF   R/W  D0-D7

Music (YM-2151)                           2000-2001   R/W  D0-D7

Read 68010 Port (Input Buffer)            280A        R    D0-D7

Self-test                                 280C        R    D7
Output Buffer Full (@2A02) (Active High)              R    D5
Left Coin Switch                                      R    D1
Right Coin Switch                                     R    D0

Interrupt acknowledge                     2A00        W    xx
Write 68010 Port (Outbut Buffer)          2A02        W    D0-D7
Banked ROM select (at 3000-3FFF)          2A04        W    D6-D7
???                                       2A06        W

Effects                                   2C00-2C0F   R/W  D0-D7

Banked Program ROM (4 pages)              3000-3FFF   R    D0-D7
Static Program ROM (48K bytes)            4000-FFFF   R    D0-D7


****************************************************************************/

#include "machine/atarigen.h"
#include "ataraud2.h"
#include "cpu/m6502/m6502.h"

static unsigned char *bank_base;

static int timed_int;
static int ym2151_int;

static int speech_data;
static int last_ctl;

static int cpu_num;
static int input_port;
static int test_port;
static int test_mask;

static int has_pokey;
static int has_tms5220;
static int has_okim6295;


/*************************************
 *
 *		External interfaces
 *
 *************************************/

void ataraud2_init(int cpunum, int inputport, int testport, int testmask)
{
	int i;

	/* copy in the parameters */
	cpu_num = cpunum;
	input_port = inputport;
	test_port = testport;
	test_mask = testmask;

	/* reset the interrupts */
	timed_int = 0;
	ym2151_int = 0;

	/* reset the static states */
	speech_data = 0;
	last_ctl = 0;

	/* predetermine the bank base */
	bank_base = &Machine->memory_region[Machine->drv->cpu[cpunum].memory_region][0x10000];

	/* determine which sound hardware is installed */
	has_tms5220 = has_okim6295 = 0;
	for (i = 0; i < MAX_SOUND; i++)
	{
		switch (Machine->drv->sound[i].sound_type)
		{
			case SOUND_TMS5220:
				has_tms5220 = 1;
				break;
			case SOUND_OKIM6295:
				has_okim6295 = 1;
				break;
			case SOUND_POKEY:
				has_pokey = 1;
				break;
		}
	}

	/* install special memory handlers */
	if (has_pokey)
	{
		install_mem_read_handler(cpunum, 0x2c00, 0x2c0f, pokey1_r);
		install_mem_write_handler(cpunum, 0x2c00, 0x2c0f, pokey1_w);
	}
}



/*************************************
 *
 *		Interrupt handlers
 *
 *************************************/

static void update_interrupts(void)
{
	if (timed_int || ym2151_int)
		cpu_set_irq_line(cpu_num, M6502_INT_IRQ, ASSERT_LINE);
	else
		cpu_set_irq_line(cpu_num, M6502_INT_IRQ, CLEAR_LINE);
}


int ataraud2_irq_gen(void)
{
	timed_int = 1;
	update_interrupts();
	return 0;
}


static void ym2151_sound_interrupt(int irq)
{
	ym2151_int = irq;
	update_interrupts();
}



/*************************************
 *
 *		I/O handlers
 *
 *************************************/

int ataraud2_io_r(int offset)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* RDV (Cyberball) */
			if (has_okim6295)
				result = OKIM6295_status_0_r(offset);
			break;

		case 0x002:		/* RDP */
			result = atarigen_6502_sound_r(offset);
			break;

		case 0x004:		/* RDIO */
			result = readinputport(input_port);
			if (!(readinputport(test_port) & test_mask)) result ^= 0x80;
			if (atarigen_cpu_to_sound_ready) result ^= 0x40;
			if (atarigen_sound_to_cpu_ready) result ^= 0x20;
			if (!has_tms5220 || tms5220_ready_r()) result ^= 0x10;
			break;

		case 0x006:		/* IRQACK */
			timed_int = 0;
			update_interrupts();
			break;

		case 0x200:		/* WRV */
		case 0x202:		/* WRP */
		case 0x204:		/* WRIO */
		case 0x206:		/* MIX */
			if (errorlog) fprintf(errorlog, "ATARAUD2: Unknown read at %04X\n", offset & 0x206);
			break;
	}

	return result;
}


void ataraud2_io_w(int offset, int data)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* RDV (Cyberball) */
		case 0x002:		/* RDP */
		case 0x004:		/* RDIO */
			if (errorlog) fprintf(errorlog, "ATARAUD2: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* IRQACK */
			timed_int = 0;
			update_interrupts();
			break;

		case 0x200:		/* WRV */
			if (has_okim6295)
				OKIM6295_data_0_w(offset, data);
			else if (has_tms5220)
				speech_data = data;
			break;

		case 0x202:		/* WRP */
			atarigen_6502_sound_w(offset, data);
			break;

		case 0x204:		/* WRIO */
			if (((data ^ last_ctl) & 0x02) && (data & 0x02))
				tms5220_data_w(0, speech_data);
			last_ctl = data;
			cpu_setbank(8, &bank_base[0x1000 * ((data >> 6) & 3)]);
			break;

		case 0x206:		/* MIX */
			break;
	}
}



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

struct MemoryReadAddress ataraud2_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2800, 0x2bff, ataraud2_io_r },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


struct MemoryWriteAddress ataraud2_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2bff, ataraud2_io_w },
	{ 0x3000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound definitions
 *
 *************************************/

struct TMS5220interface ataraud2_tms5220_interface =
{
    640000,     /* clock speed (80*samplerate) */
    75,         /* volume */
    0 			/* irq handler */
};


struct POKEYinterface ataraud2_pokey_interface =
{
	1,			/* 1 chip */
	1789790,
	40,
	POKEY_DEFAULT_GAIN,
	NO_CLIP
};


struct YM2151interface ataraud2_ym2151_interface =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(30,OSD_PAN_LEFT,30,OSD_PAN_RIGHT) },
	{ ym2151_sound_interrupt }
};


struct OKIM6295interface ataraud2_okim6295_interface_2 =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz ??? TODO: find out the real frequency */
	{ 2 },          /* memory region 2 */
	{ 75 }
};
