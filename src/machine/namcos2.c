/***************************************************************************

Namco System II

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6805/m6805.h"
#include "machine/namcos2.h"
#include "vidhrdw/generic.h"

data16_t *namcos2_68k_master_ram=NULL;
data16_t *namcos2_68k_slave_ram=NULL;
data16_t *namcos2_68k_mystery_ram=NULL;

int namcos2_gametype=0;

/*************************************************************/
/* Perform basic machine initialisation 					 */
/*************************************************************/

void namcos2_init_machine(void){
	int loop;

	/* Initialise the bank select in the sound CPU */
	namcos2_sound_bankselect_w(0,0); /* Page in bank 0 */

	/* Place CPU2 & CPU3 into the reset condition */
//	namcos2_68k_master_C148_w(0x1e2000-0x1c0000,0);
//	namcos2_68k_master_C148_w(0x1e4000-0x1c0000,0);
	cpu_set_reset_line(NAMCOS2_CPU3, ASSERT_LINE);
	cpu_set_reset_line(NAMCOS2_CPU2, ASSERT_LINE);
	cpu_set_reset_line(NAMCOS2_CPU4, ASSERT_LINE);

	/* Initialise interrupt handlers */
	for(loop=0;loop<20;loop++){
		namcos2_68k_master_C148[loop]=0;
		namcos2_68k_slave_C148[loop]=0;
	}

	/* Initialise the video control registers */
//	for(loop=0;loop<0x20;loop++) namcos2_68k_vram_ctrl_w(loop,0);

	/* Initialise ROZ */
//	for(loop=0;loop<0x8;loop++) namcos2_68k_roz_ctrl_w(loop,0);

	/* Initialise the Roadway generator */
//	for(loop=0;loop<0x10;loop+=2) namcos2_68k_road_ctrl_w(loop,0);
}

/*************************************************************/
/* EEPROM Load/Save and read/write handling 				 */
/*************************************************************/

data16_t *namcos2_eeprom;
size_t namcos2_eeprom_size;

void namcos2_nvram_handler(void *file,int read_or_write){
	if( read_or_write ){
		osd_fwrite (file, namcos2_eeprom, namcos2_eeprom_size);
	}
	else {
		if (file)
			osd_fread (file, namcos2_eeprom, namcos2_eeprom_size);
		else
			memset (namcos2_eeprom, 0xff, namcos2_eeprom_size);
	}
}

WRITE16_HANDLER( namcos2_68k_eeprom_w ){
	COMBINE_DATA( &namcos2_eeprom[offset] );
}

READ16_HANDLER( namcos2_68k_eeprom_r ){
	return namcos2_eeprom[offset];
}

/*************************************************************/
/* 68000 Shared memory area - Data ROM area 				 */
/*************************************************************/

READ16_HANDLER( namcos2_68k_data_rom_r ){
	data16_t *ROM = (data16_t *)memory_region(REGION_USER1);
	return ROM[offset];
}



/**************************************************************/
/* 68000 Shared serial communications processor (CPU5?) 	  */
/**************************************************************/

data16_t  namcos2_68k_serial_comms_ctrl[0x8];
data16_t *namcos2_68k_serial_comms_ram;

READ16_HANDLER( namcos2_68k_serial_comms_ram_r ){
//	logerror("Serial Comms read  Addr=%08x\n",offset);
	return namcos2_68k_serial_comms_ram[offset];
}

WRITE16_HANDLER( namcos2_68k_serial_comms_ram_w ){
	COMBINE_DATA( &namcos2_68k_serial_comms_ram[offset] );
//	logerror( "Serial Comms write Addr=%08x, Data=%04x\n",offset,data );
}


READ16_HANDLER( namcos2_68k_serial_comms_ctrl_r )
{
	data16_t retval = namcos2_68k_serial_comms_ctrl[offset];

	switch(offset){
		case 0x00:
			retval |= 0x0004; 	/* Set READY? status bit */
			break;
		default:
			break;
	}
//	logerror("Serial Comms read  Addr=%08x\n",offset);

	return retval;
}

WRITE16_HANDLER( namcos2_68k_serial_comms_ctrl_w )
{
	COMBINE_DATA( &namcos2_68k_serial_comms_ctrl[offset] );
//	logerror("Serial Comms write Addr=%08x, Data=%04x\n",offset,data);
}



/*************************************************************/
/* 68000 Shared protection/random key generator 			 */
/*************************************************************

Custom chip ID numbers:

Game		Year	ID (dec)	ID (hex)
--------	----	---			-----
finallap	1987
assault		1988	unused
metlhawk	1988
ordyne		1988	176			$00b0
mirninja	1988	177			$00b1
phelios		1988	178			$00b2	readme says 179
dirtfoxj	1989	180			$00b4
fourtrax	1989
valkyrie	1989
finehour	1989	188			$00bc
burnforc	1989	189			$00bd
marvland	1989	190			$00be
kyukaidk	1990	191			$00bf
dsaber		1990	192			$00c0
finalap2	1990	318			$013e
rthun2		1990	319			$013f
sgunner2	1991	346			$015a	ID out of order; gfx board is not standard
cosmogng	1991	330			$014a
finalap3	1992	318			$013e	same as finalap2
suzuka8h	1992
sws92		1992	331			$014b
sws92g		1992	332			$014c
suzuk8h2	1993
sws93		1993	334			$014e

$d00000	Write $7a25, read back $00b4 from $d00002 (dirtfoxj)
$d00002	Write 13 x $0000, read back $00be from $d00008 (marvland)
$d00004	Write 13 x $0000, read back $00bd from $d00002 (burnf)
		Write $a713, read $00c0 (dsaber)
$d00006	Write $b929, read $014a (cosmogng)
		Write $ac1d, read $014b (sws92)
		Write $f14a, read $014c (sws92g)
		Write $1fd0, read $014e (sws93)
		Write $8fc8, read $00b2 also from $d00004 (phelios)
$d00008	Write $13ec, read $013f (rthun2)
		Write 13 x $0000, read back $00c0 from $d00004 (dsaber)
$d0000a	Write $f00f, read $f00f (phelios)
$d0000c	Write $a2c7, read $00b0 (ordyne)
$d0000e	Write $31ad, read $00bd (burnforc)
		Write $b607, read $00bc (finehour)
		Write $615e, read $00be (marvland)
		Write $31ae, read $00b1 (mirninja)

$a00008	Write $6987, read $015a (sgunner2)
 *************************************************************/

data16_t namcos2_68k_key[0x08];

READ16_HANDLER( namcos2_68k_key_r )
{
/*	return namcos2_68k_key[offset]); */
//logerror("%06x: key_r 0xd0000%x\n",cpu_get_pc(),2*offset);
	return rand()&0xffff;
}

WRITE16_HANDLER( namcos2_68k_key_w )
{
//logerror("%06x: key_w 0xd0000%x = %04x\n",cpu_get_pc(),2*offset,data);
	COMBINE_DATA(&namcos2_68k_key[offset]);
}



/**************************************************************/
/*															  */
/*	Final Lap 1/2/3 Roadway generator function handlers 	  */
/*															  */
/**************************************************************/

data16_t *namcos2_68k_roadtile_ram=NULL;
data16_t *namcos2_68k_roadgfx_ram=NULL;
size_t namcos2_68k_roadtile_ram_size;
size_t namcos2_68k_roadgfx_ram_size;

WRITE16_HANDLER( namcos2_68k_roadtile_ram_w ){
	COMBINE_DATA( &namcos2_68k_roadtile_ram[offset] );
}

READ16_HANDLER( namcos2_68k_roadtile_ram_r ){
	return namcos2_68k_roadtile_ram[offset];
}

WRITE16_HANDLER( namcos2_68k_roadgfx_ram_w ){
	COMBINE_DATA( &namcos2_68k_roadgfx_ram[offset] );
}

READ16_HANDLER( namcos2_68k_roadgfx_ram_r ){
	return namcos2_68k_roadgfx_ram[offset];
}

WRITE16_HANDLER( namcos2_68k_road_ctrl_w ){
}

READ16_HANDLER( namcos2_68k_road_ctrl_r ){
	return 0;
}


/*************************************************************/
/* 68000 Interrupt/IO Handlers - CUSTOM 148 - NOT SHARED	 */
/*************************************************************/

#define NO_OF_LINES 	256
#define FRAME_TIME		(1.0/60.0)
#define LINE_LENGTH 	(FRAME_TIME/NO_OF_LINES)

data16_t  namcos2_68k_master_C148[0x20];
data16_t  namcos2_68k_slave_C148[0x20];

WRITE16_HANDLER( namcos2_68k_master_C148_w ){
	offset*=2;
	offset+=0x1c0000;
	offset&=0x1fe000;

	data&=0x0007;
	namcos2_68k_master_C148[(offset>>13)&0x1f]=data;

	switch(offset){
		case 0x1d4000:
			/* Trigger Master to Slave interrupt */
//			cpu_set_irq_line(CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_CPUIRQ], ASSERT_LINE);
			break;
		case 0x1d6000:
			/* Clear Slave to Master*/
//			cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_CPUIRQ], CLEAR_LINE);
			break;
		case 0x1de000:
			cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_VBLANKIRQ], CLEAR_LINE);
			break;
		case 0x1da000:
			cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
			break;

		case 0x1e2000:				/* Reset register CPU3 */
			{
				data&=0x01;
				if(data&0x01)
				{
					/* Resume execution */
					cpu_set_reset_line (NAMCOS2_CPU3, CLEAR_LINE);
					cpu_yield();
				}
				else
				{
					/* Suspend execution */
					cpu_set_reset_line(NAMCOS2_CPU3, ASSERT_LINE);
				}
			}
			break;
		case 0x1e4000:				/* Reset register CPU2 & CPU4 */
			{
				data&=0x01;
				if(data&0x01)
				{
					/* Resume execution */
					cpu_set_reset_line(NAMCOS2_CPU2, CLEAR_LINE);
					cpu_set_reset_line(NAMCOS2_CPU4, CLEAR_LINE);
					/* Give the new CPU an immediate slice of the action */
					cpu_yield();
				}
				else
				{
					/* Suspend execution */
					cpu_set_reset_line(NAMCOS2_CPU2, ASSERT_LINE);
					cpu_set_reset_line(NAMCOS2_CPU4, ASSERT_LINE);
				}
			}
			break;
		case 0x1e6000:					/* Watchdog reset */
			/* watchdog_reset_w(0,0); */
			break;
		default:
			break;
	}
}

READ16_HANDLER( namcos2_68k_master_C148_r ){
	offset*=2;
	offset+=0x1c0000;
	offset&=0x1fe000;

	switch(offset){
		case 0x1d6000:
			/* Clear Slave to Master*/
//			cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_CPUIRQ], CLEAR_LINE);
			break;
		case 0x1de000:
			cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_VBLANKIRQ], CLEAR_LINE);
			break;
		case 0x1da000:
			cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
			break;
		case 0x1e0000:					/* EEPROM Status register*/
			return ~0;				/* Only BIT0 used: 1=EEPROM READY 0=EEPROM BUSY */
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
	/* If the POS interrupt is running then set it at half way thru the frame */
	if(namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ]){
		timer_set(TIME_IN_NSEC(LINE_LENGTH*100), 0, namcos2_68k_master_posirq);
	}

	/* Assert the VBLANK interrupt */
	return namcos2_68k_master_C148[NAMCOS2_C148_VBLANKIRQ];
}

void namcos2_68k_master_posirq( int moog ){
	cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ], ASSERT_LINE);
	cpu_set_irq_line(CPU_SLAVE , namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ] , ASSERT_LINE);
}

WRITE16_HANDLER( namcos2_68k_slave_C148_w ){
	offset*=2;
	offset+=0x1c0000;
	offset&=0x1fe000;

	data&=0x0007;
	namcos2_68k_slave_C148[(offset>>13)&0x1f]=data;

	switch(offset)
	{
		case 0x1d4000:
			/* Trigger Slave to Master interrupt */
//			cpu_set_irq_line(CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_CPUIRQ], ASSERT_LINE);
			break;
		case 0x1d6000:
			/* Clear Master to Slave */
//			cpu_set_irq_line(CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_CPUIRQ], CLEAR_LINE);
			break;
		case 0x1de000:
			cpu_set_irq_line(CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_VBLANKIRQ], CLEAR_LINE);
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


READ16_HANDLER( namcos2_68k_slave_C148_r )
{
	offset*=2;
	offset+=0x1c0000;
	offset&=0x1fe000;

	switch(offset)
	{
		case 0x1d6000:
			/* Clear Master to Slave */
//			cpu_set_irq_line(CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_CPUIRQ], CLEAR_LINE);
			break;
		case 0x1de000:
			cpu_set_irq_line(CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_VBLANKIRQ], CLEAR_LINE);
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
	return namcos2_68k_slave_C148[(offset>>13)&0x1f];
}

int namcos2_68k_slave_vblank(void){
	return namcos2_68k_slave_C148[NAMCOS2_C148_VBLANKIRQ];
}

/**************************************************************/
/*	Sound sub-system										  */
/**************************************************************/

int namcos2_sound_interrupt(void)
{
	return M6809_INT_FIRQ;
}


WRITE_HANDLER( namcos2_sound_bankselect_w )
{
	unsigned char *RAM=memory_region(REGION_CPU3);
	int bank = ( data >> 4 ) & 0x0f;	/* 991104.CAB */
	cpu_setbank( CPU3_ROM1, &RAM[ 0x10000 + ( 0x4000 * bank ) ] );
}



/**************************************************************/
/*															  */
/*	68705 IO CPU Support functions							  */
/*															  */
/**************************************************************/

int namcos2_mcu_interrupt(void)
{
	return HD63705_INT_IRQ;
}

static int namcos2_mcu_analog_ctrl=0;
static int namcos2_mcu_analog_data=0xaa;
static int namcos2_mcu_analog_complete=0;

WRITE_HANDLER( namcos2_mcu_analog_ctrl_w )
{
	namcos2_mcu_analog_ctrl=data&0xff;

	/* Check if this is a start of conversion */
	/* Input ports 2 thru 9 are the analog channels */
	if(data&0x40)
	{
		/* Set the conversion complete flag */
		namcos2_mcu_analog_complete=2;
		/* We convert instantly, good eh! */
		switch((data>>2)&0x07)
		{
			case 0:
				namcos2_mcu_analog_data=input_port_2_r(0);
				break;
			case 1:
				namcos2_mcu_analog_data=input_port_3_r(0);
				break;
			case 2:
				namcos2_mcu_analog_data=input_port_4_r(0);
				break;
			case 3:
				namcos2_mcu_analog_data=input_port_5_r(0);
				break;
			case 4:
				namcos2_mcu_analog_data=input_port_6_r(0);
				break;
			case 5:
				namcos2_mcu_analog_data=input_port_7_r(0);
				break;
			case 6:
				namcos2_mcu_analog_data=input_port_8_r(0);
				break;
			case 7:
				namcos2_mcu_analog_data=input_port_9_r(0);
				break;
		}
#if 0
		/* Perform the offset handling on the input port */
		/* this converts it to a twos complement number */
		if ((namcos2_gametype==NAMCOS2_DIRT_FOX) ||
			(namcos2_gametype==NAMCOS2_DIRT_FOX_JP))
		namcos2_mcu_analog_data^=0x80;
#endif

		/* If the interrupt enable bit is set trigger an A/D IRQ */
		if(data&0x20)
		{
			cpu_set_irq_line( CPU_MCU, HD63705_INT_ADCONV , PULSE_LINE);
		}
	}
}

READ_HANDLER( namcos2_mcu_analog_ctrl_r )
{
	int data=0;

	/* ADEF flag is only cleared AFTER a read from control THEN a read from DATA */
	if(namcos2_mcu_analog_complete==2) namcos2_mcu_analog_complete=1;
	if(namcos2_mcu_analog_complete) data|=0x80;

	/* Mask on the lower 6 register bits, Irq EN/Channel/Clock */
	data|=namcos2_mcu_analog_ctrl&0x3f;
	/* Return the value */
	return data;
}

WRITE_HANDLER( namcos2_mcu_analog_port_w )
{
}

READ_HANDLER( namcos2_mcu_analog_port_r )
{
	if(namcos2_mcu_analog_complete==1) namcos2_mcu_analog_complete=0;
	return namcos2_mcu_analog_data;
}

WRITE_HANDLER( namcos2_mcu_port_d_w )
{
	/* Undefined operation on write */
}

READ_HANDLER( namcos2_mcu_port_d_r )
{
	/* Provides a digital version of the analog ports */
	int threshold=0x7f;
	int data=0;

	/* Read/convert the bits one at a time */
	if(input_port_2_r(0)>threshold) data|=0x01;
	if(input_port_3_r(0)>threshold) data|=0x02;
	if(input_port_4_r(0)>threshold) data|=0x04;
	if(input_port_5_r(0)>threshold) data|=0x08;
	if(input_port_6_r(0)>threshold) data|=0x10;
	if(input_port_7_r(0)>threshold) data|=0x20;
	if(input_port_8_r(0)>threshold) data|=0x40;
	if(input_port_9_r(0)>threshold) data|=0x80;

	/* Return the result */
	return data;
}

READ_HANDLER( namcos2_input_port_0_r )
{
	int data=readinputport(0);

	int one_joy_trans0[2][10]={
        {0x05,0x01,0x09,0x08,0x0a,0x02,0x06,0x04,0x12,0x14},
        {0x00,0x20,0x20,0x20,0x08,0x08,0x00,0x08,0x02,0x02}};

	int datafake, i;

	switch(namcos2_gametype)
	{
		case NAMCOS2_ASSAULT:
		case NAMCOS2_ASSAULT_JP:
		case NAMCOS2_ASSAULT_PLUS:
			datafake=~readinputport(15) & 0xff;
			logerror("xxx=%08x\n",datafake);
			for (i=0;i<10;i++)
				if (datafake==one_joy_trans0[0][i])
				{
					data&=~one_joy_trans0[1][i];
					break;
				}
	}
	return data;
}

READ_HANDLER( namcos2_input_port_10_r )
{
	int data=readinputport(10);

	int one_joy_trans10[2][10]={
        {0x05,0x01,0x09,0x08,0x0a,0x02,0x06,0x04,0x1a,0x18},
        {0x08,0x08,0x00,0x02,0x00,0x02,0x02,0x08,0x80,0x80}};

	int datafake, i;

	switch(namcos2_gametype)
	{
		case NAMCOS2_ASSAULT:
		case NAMCOS2_ASSAULT_JP:
		case NAMCOS2_ASSAULT_PLUS:
			datafake=~readinputport(15) & 0xff;
			for (i=0;i<10;i++)
				if (datafake==one_joy_trans10[0][i])
				{
					data&=~one_joy_trans10[1][i];
					break;
				}
	}
	return data;
}

READ_HANDLER( namcos2_input_port_12_r )
{
	int data=readinputport(12);

	int one_joy_trans12[2][4]={
        {0x12,0x14,0x11,0x18},
        {0x02,0x08,0x08,0x02}};

	int datafake, i;

	switch(namcos2_gametype)
	{
		case NAMCOS2_ASSAULT:
		case NAMCOS2_ASSAULT_JP:
		case NAMCOS2_ASSAULT_PLUS:
			datafake=~readinputport(15) & 0xff;
			for (i=0;i<4;i++)
				if (datafake==one_joy_trans12[0][i])
				{
					data&=~one_joy_trans12[1][i];
					break;
				}
	}
	return data;
}
