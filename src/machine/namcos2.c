/***************************************************************************

Namco System II

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/namcos2.h"

unsigned char namcos2_roz_ctrl[0x10];
unsigned char *namcos2_roz_ram;

unsigned char *namcos2_68k_master_ram=NULL;
unsigned char *namcos2_68k_slave_ram=NULL;
unsigned char *namcos2_sprite_ram=NULL;

unsigned char *namcos2_dpram=NULL;	/* 2Kx8 */
unsigned char  namcos2_68k_serial_comms_ctrl[0x10];
unsigned char *namcos2_68k_serial_comms_ram=NULL;
unsigned char *namcos2_68k_palette_ram=NULL;

int namcos2_gametype=0;


/*************************************************************/
/* Perform basic machine initialisation                      */
/*************************************************************/

void namcos2_init_machine(void)
{
    int loop;
//	unsigned char *RAM;

	if(namcos2_dpram==NULL) namcos2_dpram = malloc(0x800);
	memset( namcos2_dpram, 0, 0x800 );

	if(namcos2_sprite_ram==NULL) namcos2_sprite_ram = malloc(0x4000);
	memset( namcos2_sprite_ram, 0, 0x4000 );
	namcos2_sprite_bank=0;

    if(namcos2_68k_serial_comms_ram==NULL) namcos2_68k_serial_comms_ram = malloc(0x4000);
	memset( namcos2_68k_serial_comms_ram, 0, 0x4000 );

    if(namcos2_68k_palette_ram==NULL) namcos2_68k_palette_ram = malloc(0x10000);
	memset( namcos2_68k_palette_ram, 0, 0x10000 );

	/* Initialise the bank select in the sound CPU */
	namcos2_sound_bankselect_w(0,0);		/* Page in bank 0 */

	/* Place CPU2 & CPU3 into the reset condition */
	namcos2_68k_master_C148_w(0x1e2000-0x1c0000,0);
	namcos2_68k_master_C148_w(0x1e4000-0x1c0000,0);

	/* Initialise interrupt handlers */
	for(loop=0;loop<20;loop++)
	{
        namcos2_68k_master_C148[loop]=0;
        namcos2_68k_slave_C148[loop]=0;
	}
}



/*************************************************************/
/* EEPROM Load/Save and read/write handling                  */
/*************************************************************/

unsigned char *namcos2_eeprom;
int namcos2_eeprom_size;

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

void namcos2_68k_eeprom_w(int offset, int data)
{
	int oldword = READ_WORD (&namcos2_eeprom[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&namcos2_eeprom[offset], newword);
}

int namcos2_68k_eeprom_r( int offset )
{
	return READ_WORD(&namcos2_eeprom[offset]);
}

/*************************************************************/
/* 68000 Shared memory area - Data ROM area                  */
/*************************************************************/

int namcos2_68k_data_rom_r(int offset)
{
	int data;
	data= READ_WORD(&Machine->memory_region[MEM_DATA][offset]);
	return data;
}


/*************************************************************/
/* 68000 Shared memory area - Video RAM control              */
/*************************************************************/

unsigned char namcos2_68k_vram_ctrl[0x40];

void namcos2_68k_vram_ctrl_w( int offset, int data )
{
	COMBINE_WORD_MEM(&namcos2_68k_vram_ctrl[offset&0x3f],data);
}

int namcos2_68k_vram_ctrl_r( int offset )
{
	return READ_WORD(&namcos2_68k_vram_ctrl[offset&0x3f]);
}


/*************************************************************/
/* 68000 Shared memory area - Video palette control          */
/*************************************************************/

unsigned char *namcos2_68k_video_palette;

int  namcos2_68k_video_palette_r( int offset )
{
	if (errorlog) fprintf(errorlog,"Video Palette read  Addr=%08x\n",offset);
	return READ_WORD(&namcos2_68k_palette_ram[offset&0xffff]);
}

void namcos2_68k_video_palette_w( int offset, int data )
{
	if (errorlog) fprintf(errorlog,"Video Palette write Addr=%08x, Data=%04x\n",offset,data);
	COMBINE_WORD_MEM(&namcos2_68k_palette_ram[offset&0xffff],data);
}

/*************************************************************/
/* 68000/6809/63705 Shared memory area - DUAL PORT Memory    */
/*************************************************************/

int	namcos2_68k_dpram_word_r(int offset)
{
	offset = offset/2;
	return READ_WORD(&namcos2_dpram[offset]);
}

void namcos2_68k_dpram_word_w(int offset, int data)
{
	offset = offset/2;
	namcos2_dpram[offset] = COMBINE_WORD( namcos2_dpram[offset], data )&0xff;
}

int	namcos2_dpram_byte_r(int offset)
{
	return namcos2_dpram[offset];
}

void namcos2_dpram_byte_w(int offset, int data)
{
	namcos2_dpram[offset] = data;
}


/**************************************************************/
/* 68000 Shared serial communications processor (CPU5?)       */
/**************************************************************/

int namcos2_68k_serial_comms_ram_r(int offset)
{
	if (errorlog) fprintf(errorlog,"Serial Comms read  Addr=%08x\n",offset);
	return READ_WORD(&namcos2_68k_serial_comms_ram[offset&0x3fff]);
}

void namcos2_68k_serial_comms_ram_w(int offset, int data)
{
	COMBINE_WORD_MEM(&namcos2_68k_serial_comms_ram[offset&0x3fff],data&0x1ff);
	if (errorlog) fprintf(errorlog,"Serial Comms write Addr=%08x, Data=%04x\n",offset,data);
}


int namcos2_68k_serial_comms_ctrl_r(int offset)
{
	if (errorlog) fprintf(errorlog,"Serial Comms read  Addr=%08x\n",offset);
	return READ_WORD(&namcos2_68k_serial_comms_ctrl[offset&0x0f]);
}

void namcos2_68k_serial_comms_ctrl_w(int offset,int data)
{
	COMBINE_WORD_MEM(&namcos2_68k_serial_comms_ctrl[offset&0x0f],data);
	if (errorlog) fprintf(errorlog,"Serial Comms write Addr=%08x, Data=%04x\n",offset,data);
}



/*************************************************************/
/* 68000 Shared SPRITE/OBJECT Memory access/control          */
/*************************************************************/

int namcos2_sprite_bank=0;

void namcos2_68k_sprite_ram_w(int offset, int data)
{
	WRITE_WORD(&namcos2_sprite_ram[offset],data);
}

void namcos2_68k_sprite_bank_w( int offset, int data )
{
    namcos2_sprite_bank=data&0x000f;
}

int	namcos2_68k_sprite_ram_r(int offset)
{
	int data=READ_WORD(&namcos2_sprite_ram[offset&0x3fff]);
	return data;
}

int namcos2_68k_sprite_bank_r( int offset )
{
    return namcos2_sprite_bank;
}



/*************************************************************/
/* 68000 Shared protection/random key generator              */
/*************************************************************/
//
//$d00000
//$d00002
//$d00004     Write 13 x $0000, read back $00bd from $d00002 (burnf)
//$d00006     Write $b929, read $014a (cosmog, does a jmp $0 if it doesnt get this)
//$d00008     Write $13ec, read $013f (rthun2)
//$d0000a     Write $f00f, read $f00f (phelios)
//$d0000c     Write $8fc8, read $00b2 (rthun2)
//$d0000e     Write $31ad, read $00bd (burnf)
//

unsigned char namcos2_68k_key[0x10];

int namcos2_68k_key_r( int offset )
{
	switch(offset)
	{
	    case 0x02:
	        return 0x00bd;
		case 0x06:
			return 0x014a;
	    case 0x08:
	        return 0x013f;
	    case 0x0a:
	        return 0xf00f;
	    case 0x0c:
	        return 0x00b2;
	    case 0x0e:
	        return 0x00bd;
	    default:
	        break;
	}
	return READ_WORD(&namcos2_68k_key[offset&0x0f]);
}

void namcos2_68k_key_w( int offset, int data )
{
	COMBINE_WORD_MEM(&namcos2_68k_key[offset&0x0f],data);
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

void namcos2_68k_roz_ctrl_w( int offset, int data )
{
	COMBINE_WORD_MEM(&namcos2_roz_ctrl[offset&0x0f],data);
}

int namcos2_68k_roz_ctrl_r( int offset )
{
	return READ_WORD(&namcos2_roz_ctrl[offset&0x0f]);
}


/*************************************************************/
/* 68000 Interrupt/IO Handlers - CUSTOM 148 - NOT SHARED     */
/*************************************************************/

//#define NAMCOS2_C148_0          0       /* 0x1c0000 */
//#define NAMCOS2_C148_1          1
//#define NAMCOS2_C148_2          2
//#define NAMCOS2_C148_CPUIRQ     3
//#define NAMCOS2_C148_EXIRQ      4       /* 0x1c8000 */
//#define NAMCOS2_C148_POSIRQ     5
//#define NAMCOS2_C148_SERIRQ     6
//#define NAMCOS2_C148_VBLANKIRQ  7

#define NO_OF_LINES     256
#define FRAME_TIME      (1.0/60.0)
#define LINE_LENGTH     (FRAME_TIME/NO_OF_LINES)

int  namcos2_68k_master_C148[0x20];
int  namcos2_68k_slave_C148[0x20];

void namcos2_68k_master_C148_w( int offset, int data )
{
//	int idx=offset>>12;
	offset+=0x1c0000;
	offset&=0x1fe000;

    data&=0x0007;
    namcos2_68k_master_C148[(offset>>13)&0x1f]=data;

	switch(offset)
	{
	    case 0x1de000:
	        cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
	        break;
	    case 0x1da000:
	        cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
	        break;

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
}


int namcos2_68k_master_C148_r( int offset )
{
//	int idx=offset>>12;
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
    return namcos2_68k_master_C148[(offset>>13)&0x1f];
}

int namcos2_68k_master_vblank(void)
{
    // If the POS interrupt is running then set it at half way thru the frame
    if(namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ])
    {
        timer_set(TIME_IN_NSEC(LINE_LENGTH*100), 0, namcos2_68k_master_posirq);
    }

    // Assert the VBLANK interrupt
    return namcos2_68k_master_C148[NAMCOS2_C148_VBLANKIRQ];
}

void namcos2_68k_master_posirq( int moog )
{
    cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ], ASSERT_LINE);
    cpu_set_irq_line(CPU_SLAVE , namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ] , ASSERT_LINE);
}


void namcos2_68k_slave_C148_w( int offset, int data )
{
//	int idx=offset>>12;
	offset+=0x1c0000;
	offset&=0x1fe000;

    data&=0x0007;
    namcos2_68k_slave_C148[(offset>>13)&0x1f]=data;

	switch(offset)
	{
	    case 0x1de000:
	        cpu_set_irq_line(CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
	        break;
	    case 0x1da000:
	        cpu_set_irq_line(CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
	        break;
		case 0x1e6000:					/* Watchdog reset */
			/* watchdog_reset_w(0,0); */
			break;
		default:
			break;
	}
}


int namcos2_68k_slave_C148_r( int offset )
{
//	int idx=offset>>12;
	offset+=0x1c0000;
	offset&=0x1fe000;

	switch(offset)
	{
		case 0x1e6000:					/* Watchdog reset */
			/* watchdog_reset_w(0,0); */
			break;
		default:
			break;
	}
    return namcos2_68k_slave_C148[(offset>>13)&0x1f];
}

int namcos2_68k_slave_vblank(void)
{
    return namcos2_68k_slave_C148[NAMCOS2_C148_VBLANKIRQ];
}

/**************************************************************/
/*  Sound sub-system                                          */
/**************************************************************/

//int namcos2_sound_interrupt(int irq)
int namcos2_sound_interrupt(void)
{
//    cpu_set_irq_line( CPU_SOUND, M6809_INT_FIRQ , irq ? ASSERT_LINE : CLEAR_LINE);
	cpu_set_irq_line( CPU_SOUND, M6809_INT_FIRQ , PULSE_LINE);
//	if (errorlog) fprintf(errorlog,"NAMCOS2 Flyback Sound Interrupt - M6809_INT_FIRQ\n");
	return M6809_INT_FIRQ;
}


void namcos2_sound_bankselect_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[CPU_SOUND];
	int bank = ( data >> 4 ) & 0x07;
	cpu_setbank( CPU3_ROM1, &RAM[ 0x10000 + ( 0x4000 * bank ) ] );
}
