/***************************************************************************

Namco System II

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/namcos2.h"

unsigned char *namcos2_eeprom;
int namcos2_eeprom_size;

unsigned char namcos2_roz_ctrl[0x10];
unsigned char *namcos2_roz_ram;

unsigned char *namcos2_cpu1_ram2;
unsigned char *namcos2_sharedram1;
unsigned char *namcos2_cpu1_workingram;
unsigned char *namcos2_sprite_ram;
static unsigned char *namcos2_dpram=NULL;	/* 2Kx8 */

unsigned char namcos2_1c0000[0x40];
unsigned char namcos2_420000[0x40];
unsigned char namcos2_4a0000[0x10];
unsigned char namcos2_c40000[0x01];
unsigned char namcos2_d00000[0x10];

int namcos2_hiload(void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, namcos2_eeprom, namcos2_eeprom_size);
		osd_fclose (f);
	}
	else
		memset (namcos2_eeprom, 0xff, namcos2_eeprom_size);

	return 1;
}

void namcos2_hisave(void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite (f, namcos2_eeprom, namcos2_eeprom_size);
		osd_fclose (f);
	}
}

void namcos2_eeprom_w(int offset, int data)
{
	int oldword = READ_WORD (&namcos2_eeprom[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&namcos2_eeprom[offset], newword);
}

int namcos2_eeprom_r( int offset )
{
	return READ_WORD(&namcos2_eeprom[offset]);
}


void namcos2_1c0000_w( int offset, int data )
{
	int idx=offset>>12;
	offset+=0x1c0000;
	offset&=0x1fe000;

	switch(offset)
	{
		case 0x1e2000:				/* Reset register CPU3 */
			if(data&0x01)
			{
				/* Resume execution */
				cpu_reset(NAMCOS2_CPU3);
				cpu_halt(NAMCOS2_CPU3,1);
				/* Give the new CPU an immediate slice of the action */
				cpu_yield();
			}
			else
			{
				/* Suspend execution */
				cpu_reset(NAMCOS2_CPU3);
				cpu_halt(NAMCOS2_CPU3,0);
			}
			break;
		case 0x1e4000:				/* Reset register CPU2 & CPU4 */
			if(data&0x01)
			{
				/* Resume execution */
				cpu_reset(NAMCOS2_CPU2);
				cpu_halt(NAMCOS2_CPU2,1);
				cpu_reset(NAMCOS2_CPU4);
				cpu_halt(NAMCOS2_CPU4,1);
				/* Give the new CPU an immediate slice of the action */
				cpu_yield();
			}
			else
			{
				/* Suspend execution */
				cpu_reset(NAMCOS2_CPU2);
				cpu_halt(NAMCOS2_CPU2,0);
				cpu_reset(NAMCOS2_CPU4);
				cpu_halt(NAMCOS2_CPU4,0);
			}
			break;
		case 0x1e6000:					/* Watchdog reset */
			/* watchdog_reset_w(0,0); */
			break;
		default:
			break;
	}
	COMBINE_WORD_MEM(&namcos2_1c0000[idx],data);
}

void namcos2_420000_w( int offset, int data )
{
	COMBINE_WORD_MEM(&namcos2_420000[offset&0x3f],data);
}

void namcos2_4a0000_w( int offset, int data )
{
	COMBINE_WORD_MEM(&namcos2_4a0000[offset&0x0f],data);
}

void namcos2_c40000_w( int offset, int data )
{
	COMBINE_WORD_MEM(&namcos2_c40000[offset&0x01],data);
}

void namcos2_d00000_w( int offset, int data )
{
	switch(offset)
	{
		case 0x04:					/* D00004 - Usage unkown */
			break;
		case 0x06:					/* D00006 - Protection register (See CPU1 PC=0034C0) */
			break;					/* #$B929 is written and #$014A MUST be returned     */
	}
	COMBINE_WORD_MEM(&namcos2_d00000[offset&0x0f],data);
}


int namcos2_1c0000_r( int offset )
{
	int idx=offset>>12;
	offset+=0x1c0000;
	offset&=0x1fe000;

	switch(offset)
	{
		case 0x1e0000:					/* EEPROM Status register*/
			return 0xffff;				/* Only BIT0 used: 1=EEPROM READY 0=EEPROM BUSY */
			break;
		case 0x1e6000:					/* Watchdog reset */
			/* watchdog_reset_w(0,0); */
			break;
		default:
			break;
	}
	return READ_WORD(&namcos2_1c0000[idx]);
}

int namcos2_420000_r( int offset )
{
	return READ_WORD(&namcos2_420000[offset&0x3f]);
}

int namcos2_4a0000_r( int offset )
{
	return READ_WORD(&namcos2_4a0000[offset&0x0f]);
}

int namcos2_c40000_r( int offset )
{
	return READ_WORD(&namcos2_c40000[offset&0x01]);
}

int namcos2_d00000_r( int offset )
{
	switch(offset)
	{
		case 0x04:					/* D00004 - Usage unkown */
			break;
		case 0x06:					/* D00006 - Protection register */
			return 0x014a;			/* This looks like some kind of protection, if this value  */
			break;					/* isnt read back then the cosmogang code does a jmp $0 !! */
	}
	return READ_WORD(&namcos2_d00000[offset&0x0f]);
}

/* Perform basic machine initialisation */

void namcos2_init_machine(void)
{
	unsigned char *RAM;

	namcos2_dpram = malloc(0x800);
	memset( namcos2_dpram, 0, 0x800 );

	namcos2_sprite_ram = malloc(0x4000);
	memset( namcos2_sprite_ram, 0, 0x4000 );

	/* Initialise the bank select in the sound CPU */
	namcos2_sound_bankselect_w(0,0);		/* Page in bank 0 */

	/* Manually copy the lower bank into the correct position in rom */
	/* if we had any spare banks we could do this the simple way     */

	RAM = Machine->memory_region[Machine->drv->cpu[CPU_SOUND].memory_region];
	memcpy(&RAM[0xC000],&RAM[0x10000],0x4000);

	/* Place CPU2 & CPU3 into the reset condition */
	namcos2_1c0000_w(0x1e2000-0x1c0000,0);
	namcos2_1c0000_w(0x1e4000-0x1c0000,0);
}



/**************************************************************/
/*  ROZ - Rotate & Zoom memory function handlers              */
/*                                                            */
/*  ROZ control made up of 8 registers, looks to be split     */
/*  into 2 groups of 3 registers one for each plane and two   */
/*  other registers ??                                        */
/*                                                            */
/*  0 - Plane 2 Offset 0x20000                                */
/*  2 - Plane 2                                               */
/*  4 - Plane 2                                               */
/*  6 - Plane 1 Offset 0x00000                                */
/*  8 - Plane 1                                               */
/*  A   Plane 1                                               */
/*  C - Unused                                                */
/*  E - Control reg ??                                        */
/*                                                            */
/**************************************************************/

void namcos2_roz_ctrl_w( int offset, int data )
{
	COMBINE_WORD_MEM(&namcos2_roz_ctrl[offset&0x0f],data);
}

int namcos2_roz_ctrl_r( int offset )
{
	return READ_WORD(&namcos2_roz_ctrl[offset&0x0f]);
}


/**************************************************************/
/*                                                            */
/**************************************************************/

void namcos2_sound_interrupt( int irq )
{
	cpu_set_irq_line( CPU_SOUND, ASSERT_LINE, M6809_INT_FIRQ);
}

int	namcos2_dpram_word_r(int offset)
{
	offset = offset/2;

	/* hold down space to pass initial RAM tests */
	return READ_WORD(&namcos2_dpram[offset]);

}

void namcos2_dpram_word_w(int offset, int data)
{
	offset = offset/2;
	namcos2_dpram[offset] = COMBINE_WORD( namcos2_dpram[offset], data )&0xff;
	namcos2_sound_interrupt( 0 );
}

int	namcos2_dpram_byte_r(int offset)
{
	return namcos2_dpram[offset];
}

void namcos2_dpram_byte_w(int offset, int data)
{
	namcos2_dpram[offset] = data;
}


/* Sprite RAM Emulation */

int	namcos2_sprite_word_r(int offset)
{
	int data=READ_WORD(&namcos2_sprite_ram[offset]);
	return data;
}

void namcos2_sprite_word_w(int offset, int data)
{
	WRITE_WORD(&namcos2_sprite_ram[offset],data);
}

void namcos2_sound_bankselect_w(int offset, int data)
{
	int bankaddress = 0x10000 + ((data & 0x07) * 0x4000);
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[CPU_SOUND].memory_region];
	cpu_setbank(CPU3_ROM1, &RAM[bankaddress]);
}
