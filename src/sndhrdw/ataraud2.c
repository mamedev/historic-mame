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


static int speech_val;
static int last_ctl;

static int cpu_num;
static int input_port;
static int test_port;
static int test_mask;


/*************************************
 *
 *		External interfaces
 *
 *************************************/

void ataraud2_reset(int cpunum, int inputport, int testport, int testmask)
{
	cpu_num = cpunum;
	input_port = inputport;
	test_port = testport;
	test_mask = testmask;

	speech_val = 0;
	last_ctl = 0;
}



/*************************************
 *
 *		I/O handlers
 *
 *************************************/

int ataraud2_6502_switch_r(int offset)
{
	int temp = readinputport(input_port);

	if (!(readinputport(test_port) & test_mask)) temp ^= 0x80;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x20;
	if (tms5220_ready_r()) temp ^= 0x10;

	return temp;
}


void ataraud2_tms5220_w(int offset, int data)
{
	speech_val = data;
}


void ataraud2_6502_ctl_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[cpu_num].memory_region];

	if (((data ^ last_ctl) & 0x02) && (data & 0x02))
		tms5220_data_w(0, speech_val);
	last_ctl = data;
	cpu_setbank(8, &RAM[0x10000 + 0x1000 * ((data >> 6) & 3)]);
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
	{ 0x280a, 0x280a, atarigen_6502_sound_r },
	{ 0x280c, 0x280c, ataraud2_6502_switch_r },
	{ 0x280e, 0x280e, atarigen_6502_irq_ack_r },
	{ 0x2c00, 0x2c0f, pokey1_r },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


struct MemoryWriteAddress ataraud2_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x280e, 0x280e, atarigen_6502_irq_ack_w },
	{ 0x2a00, 0x2a00, ataraud2_tms5220_w },
	{ 0x2a02, 0x2a02, atarigen_6502_sound_w },
	{ 0x2a04, 0x2a04, ataraud2_6502_ctl_w },
	{ 0x2a06, 0x2a06, MWA_NOP },	/* mixer */
	{ 0x2c00, 0x2c0f, pokey1_w },
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
    100,        /* volume */
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
	{ 0 }
};
