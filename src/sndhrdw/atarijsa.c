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
#include "sndhrdw/atarijsa.h"
#include "cpu/m6502/m6502.h"

static unsigned char *bank_base;

static int speech_data;
static int last_ctl;

static int cpu_num;
static int input_port;
static int test_port;
static int test_mask;

static int has_pokey;
static int has_ym2151;
static int has_ym2413;
static int has_tms5220;
static int has_oki6295;

static int oki6295_bank;

static int overall_volume;
static int pokey_volume;
static int ym2151_volume;
static int ym2413_volume;
static int tms5220_volume;
static int oki6295_volume;

static void update_all_volumes(void);

static int jsa1_io_r(int offset);
static void jsa1_io_w(int offset, int data);
static int jsa2_io_r(int offset);
static void jsa2_io_w(int offset, int data);
static int jsa3_io_r(int offset);
static void jsa3_io_w(int offset, int data);


/*************************************
 *
 *		External interfaces
 *
 *************************************/

void atarijsa_init(int cpunum, int inputport, int testport, int testmask)
{
	int i;

	/* copy in the parameters */
	cpu_num = cpunum;
	input_port = inputport;
	test_port = testport;
	test_mask = testmask;

	/* predetermine the bank base */
	bank_base = &Machine->memory_region[Machine->drv->cpu[cpunum].memory_region][0x10000];

	/* determine which sound hardware is installed */
	has_tms5220 = has_oki6295 = has_pokey = has_ym2413 = has_ym2151 = 0;
	for (i = 0; i < MAX_SOUND; i++)
	{
		switch (Machine->drv->sound[i].sound_type)
		{
			case SOUND_TMS5220:
				has_tms5220 = 1;
				break;
			case SOUND_OKIM6295:
				has_oki6295 = 1;
				break;
			case SOUND_POKEY:
				has_pokey = 1;
				break;
			case SOUND_YM2413:
				has_ym2413 = 1;
				break;
			case SOUND_YM2151:
				has_ym2151 = 1;
				break;
		}
	}
	
	/* install the I/O handler based on which board we're using */
	if (has_ym2413)
	{
		install_mem_read_handler(cpunum, 0x2800, 0x2bff, jsa3_io_r);
		install_mem_write_handler(cpunum, 0x2800, 0x2bff, jsa3_io_w);
	}
	else if (has_oki6295)
	{
		install_mem_read_handler(cpunum, 0x2800, 0x2bff, jsa2_io_r);
		install_mem_write_handler(cpunum, 0x2800, 0x2bff, jsa2_io_w);
	}
	else
	{
		install_mem_read_handler(cpunum, 0x2800, 0x2bff, jsa1_io_r);
		install_mem_write_handler(cpunum, 0x2800, 0x2bff, jsa1_io_w);
	}

	/* install POKEY memory handlers */
	if (has_pokey)
	{
		install_mem_read_handler(cpunum, 0x2c00, 0x2c0f, pokey1_r);
		install_mem_write_handler(cpunum, 0x2c00, 0x2c0f, pokey1_w);
	}

	/* install YM2151 memory handlers */
	if (has_ym2151)
	{
		install_mem_read_handler(cpunum, 0x2000, 0x2001, YM2151_status_port_0_r);
		install_mem_write_handler(cpunum, 0x2000, 0x2000, YM2151_register_port_0_w);
		install_mem_write_handler(cpunum, 0x2001, 0x2001, YM2151_data_port_0_w);
	}
	
	/* install YM2413 memory handlers */
/*	if (has_ym2413)
	{
		install_mem_read_handler(cpunum, 0x2000, 0x2001, YM2413_status_port_0_r);
		install_mem_write_handler(cpunum, 0x2000, 0x2000, YM2413_register_port_0_w);
		install_mem_write_handler(cpunum, 0x2001, 0x2001, YM2413_data_port_0_w);
		install_mem_read_handler(cpunum, 0x2600, 0x2601, YM2413_status_port_0_r);
		install_mem_write_handler(cpunum, 0x2600, 0x2600, YM2413_register_port_0_w);
		install_mem_write_handler(cpunum, 0x2601, 0x2601, YM2413_data_port_0_w);
	}*/
	
	atarijsa_reset();
}


void atarijsa_reset(void)
{
	/* reset the sound I/O system */
	atarigen_sound_io_reset(1);

	/* reset the static states */
	speech_data = 0;
	last_ctl = 0;
	oki6295_bank = 0;
	overall_volume = 100;
	pokey_volume = 100;
	ym2151_volume = 100;
	ym2413_volume = 100;
	tms5220_volume = 100;
	oki6295_volume = 100;

	/* Guardians of the Hood assumes we're reset to bank 0 on startup */
	cpu_setbank(8, &bank_base[0x0000]);
}



/*************************************
 *
 *		JSA I I/O handlers
 *
 *************************************/

static int jsa1_io_r(int offset)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* n/c */
			if (errorlog) fprintf(errorlog, "atarijsa: Unknown read at %04X\n", offset & 0x206);
			break;

		case 0x002:		/* /RDP */
			result = atarigen_6502_sound_r(offset);
			break;

		case 0x004:		/* /RDIO */
			/*
				0x80 = self test
				0x40 = NMI line state (active low)
				0x20 = sound output full
				0x10 = TMS5220 ready (active low)
				0x08 = +5V
				0x04 = +5V
				0x02 = coin 2
				0x01 = coin 1
			*/
			result = readinputport(input_port);
			if (!(readinputport(test_port) & test_mask)) result ^= 0x80;
			if (atarigen_cpu_to_sound_ready) result ^= 0x40;
			if (atarigen_sound_to_cpu_ready) result ^= 0x20;
			if (!has_tms5220 || tms5220_ready_r()) result ^= 0x10;
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /VOICE */
		case 0x202:		/* /WRP */
		case 0x204:		/* /WRIO */
		case 0x206:		/* /MIX */
			if (errorlog) fprintf(errorlog, "atarijsa: Unknown read at %04X\n", offset & 0x206);
			break;
	}

	return result;
}


static void jsa1_io_w(int offset, int data)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* n/c */
		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
			if (errorlog) fprintf(errorlog, "atarijsa: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /VOICE */
			speech_data = data;
			break;

		case 0x202:		/* /WRP */
			atarigen_6502_sound_w(offset, data);
			break;

		case 0x204:		/* WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = squeak (tweaks the 5220 frequency)
				0x04 = TMS5220 reset (active low)
				0x02 = TMS5220 write strobe (active low)
				0x01 = YM2151 reset (active low)
			*/
			
			/* handle TMS5220 I/O */
			if (has_tms5220)
			{
				if (((data ^ last_ctl) & 0x02) && (data & 0x02))
					tms5220_data_w(0, speech_data);
			}
			
			/* update the bank */
			cpu_setbank(8, &bank_base[0x1000 * ((data >> 6) & 3)]);
			last_ctl = data;
			break;

		case 0x206:		/* MIX */
			/*
				0xc0 = TMS5220 volume (0-3)
				0x30 = POKEY volume (0-3)
				0x0e = YM2151 volume (0-7)
				0x01 = low-pass filter enable
			*/
			tms5220_volume = ((data >> 6) & 3) * 100 / 3;
			pokey_volume = ((data >> 4) & 3) * 100 / 3;
			ym2151_volume = ((data >> 1) & 7) * 100 / 7;
			update_all_volumes();
			break;
	}
}



/*************************************
 *
 *		JSA II I/O handlers
 *
 *************************************/

static int jsa2_io_r(int offset)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
			if (has_oki6295)
				result = OKIM6295_status_0_r(offset);
			else
				if (errorlog) fprintf(errorlog, "atarijsa: Unknown read at %04X\n", offset & 0x206);
			break;

		case 0x002:		/* /RDP */
			result = atarigen_6502_sound_r(offset);
			break;

		case 0x004:		/* /RDIO */
			/*
				0x80 = self test
				0x40 = NMI line state (active low)
				0x20 = sound output full
				0x10 = +5V
				0x08 = +5V
				0x04 = +5V
				0x02 = coin 2
				0x01 = coin 1
			*/
			result = readinputport(input_port);
			if (!(readinputport(test_port) & test_mask)) result ^= 0x80;
			if (atarigen_cpu_to_sound_ready) result ^= 0x40;
			if (atarigen_sound_to_cpu_ready) result ^= 0x20;
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /WRV */
		case 0x202:		/* /WRP */
		case 0x204:		/* /WRIO */
		case 0x206:		/* /MIX */
			if (errorlog) fprintf(errorlog, "atarijsa: Unknown read at %04X\n", offset & 0x206);
			break;
	}

	return result;
}


static void jsa2_io_w(int offset, int data)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
			if (errorlog) fprintf(errorlog, "atarijsa: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /WRV */
			if (has_oki6295)
				OKIM6295_data_0_w(offset, data);
			else
				if (errorlog) fprintf(errorlog, "atarijsa: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x202:		/* /WRP */
			atarigen_6502_sound_w(offset, data);
			break;

		case 0x204:		/* /WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = voice frequency (tweaks the OKI6295 frequency)
				0x04 = OKI6295 reset (active low)
				0x02 = n/c
				0x01 = YM2151 reset (active low)
			*/
			
			/* update the bank */
			cpu_setbank(8, &bank_base[0x1000 * ((data >> 6) & 3)]);
			last_ctl = data;
			break;

		case 0x206:		/* /MIX */
			/*
				0xc0 = n/c
				0x20 = low-pass filter enable
				0x10 = n/c
				0x0e = YM2151 volume (0-7)
				0x01 = OKI6295 volume (0-1)
			*/
			ym2151_volume = ((data >> 1) & 7) * 100 / 7;
			oki6295_volume = (data & 1) * 100;
			update_all_volumes();
			break;
	}
}



/*************************************
 *
 *		JSA III I/O handlers
 *
 *************************************/

static int jsa3_io_r(int offset)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV */
			if (has_oki6295)
				result = OKIM6295_status_0_r(offset);
			break;

		case 0x002:		/* /RDP */
			result = atarigen_6502_sound_r(offset);
			break;

		case 0x004:		/* /RDIO */
			/*
				0x80 = self test (active low)
				0x40 = NMI line state (active low)
				0x20 = sound output full (active low)
				0x10 = self test (active low)
				0x08 = service (active low)
				0x04 = tilt (active low)
				0x02 = coin L (active low)
				0x01 = coin R (active low)
			*/
			result = readinputport(input_port);
			if (!(readinputport(test_port) & test_mask)) result ^= 0x90;
			if (atarigen_cpu_to_sound_ready) result ^= 0x40;
			if (atarigen_sound_to_cpu_ready) result ^= 0x20;
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /WRV */
		case 0x202:		/* /WRP */
		case 0x204:		/* /WRIO */
		case 0x206:		/* /MIX */
			if (errorlog) fprintf(errorlog, "atarijsa: Unknown read at %04X\n", offset & 0x206);
			break;
	}

	return result;
}


static void jsa3_io_w(int offset, int data)
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* /RDV -- used for volume by Off the Wall */
			overall_volume = data * 100 / 127;
			update_all_volumes();
			break;
			
		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
			if (errorlog) fprintf(errorlog, "atarijsa: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /WRV */
			if (has_oki6295)
				OKIM6295_data_0_w(offset, data);
			break;

		case 0x202:		/* /WRP */
			atarigen_6502_sound_w(offset, data);
			break;

		case 0x204:		/* /WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = voice frequency (tweaks the OKI6295 frequency)
				0x04 = OKI6295 reset (active low)
				0x02 = OKI6295 bank bit 0
				0x01 = YM2151 reset (active low)
			*/
			
			/* update the OKI bank */
			OKIM6295_set_bank_base(0, oki6295_bank * 0x40000);
			
			/* update the bank */
			cpu_setbank(8, &bank_base[0x1000 * ((data >> 6) & 3)]);
			last_ctl = data;
			break;

		case 0x206:		/* /MIX */
			/*
				0xc0 = n/c
				0x20 = low-pass filter enable
				0x10 = OKI6295 bank bit 1
				0x0e = YM2151/YM2413 volume (0-7)
				0x01 = OKI6295 volume (0-1)
			*/

			/* update the OKI bank */
			OKIM6295_set_bank_base(0, oki6295_bank * 0x40000);

			/* update the volumes */
			ym2413_volume = ym2151_volume = ((data >> 1) & 7) * 100 / 7;
			oki6295_volume = (data & 1) * 100;
			update_all_volumes();
			break;
	}
}



/*************************************
 *
 *		Volume helpers
 *
 *************************************/

static void update_all_volumes(void)
{
	if (has_pokey) atarigen_set_pokey_vol(overall_volume * pokey_volume / 100);
	if (has_ym2151) atarigen_set_ym2151_vol(overall_volume * ym2151_volume / 100);
	if (has_ym2413) atarigen_set_ym2413_vol(overall_volume * ym2413_volume / 100);
	if (has_tms5220) atarigen_set_tms5220_vol(overall_volume * tms5220_volume / 100);
	if (has_oki6295) atarigen_set_oki6295_vol(overall_volume * oki6295_volume / 100);
}



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

struct MemoryReadAddress atarijsa_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


struct MemoryWriteAddress atarijsa_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x3000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound definitions
 *
 *************************************/

struct TMS5220interface atarijsa_tms5220_interface =
{
    640000,     /* clock speed (80*samplerate) */
    100,        /* volume */
    0 			/* irq handler */
};


struct POKEYinterface atarijsa_pokey_interface =
{
	1,			/* 1 chip */
	1789790,
	{ 40 },
	POKEY_DEFAULT_GAIN,
	NO_CLIP
};


struct YM2413interface atarijsa_ym2413_interface_mono =
{
    1,					/* 1 chip */
    3579580,			/* ~7MHz */
    { 100 },			/* Volume */
    NULL				/* IRQ handler */
};


struct YM2151interface atarijsa_ym2151_interface_mono =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
	{ atarigen_ym2151_irq_gen }
};


struct YM2151interface atarijsa_ym2151_interface_stereo =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ atarigen_ym2151_irq_gen }
};


struct OKIM6295interface atarijsa_okim6295_interface_2 =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz ??? TODO: find out the real frequency */
	{ 2 },          /* memory region 2 */
	{ 75 }
};


struct OKIM6295interface atarijsa_okim6295_interface_3 =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz ??? TODO: find out the real frequency */
	{ 3 },          /* memory region 3 */
	{ 75 }
};
