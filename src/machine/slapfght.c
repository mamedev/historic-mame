/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


unsigned char *slapfight_dpram;
int slapfight_dpram_size;

int slapfight_status;

static int slapfight_status_state;

/* Perform basic machine initialisation */

void slapfight_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* MAIN CPU */

	slapfight_status_state=0;
	slapfight_status = 0xc7;

	/* SOUND CPU */
	cpu_halt(1,0);
}

/* Interrupt handlers cpu & sound */

void slapfight_dpram_w(int offset, int data)
{
    slapfight_dpram[offset]=data;

//	if (errorlog) fprintf(errorlog,"SLAPFIGHT MAIN  CPU : Write to   $c8%02x = %02x\n",offset,slapfight_dpram[offset]);

	// Synchronise CPUs
/*	timer_set(TIME_NOW,0,0);        P'tit Seb 980926 Commented out because it doesn't seem to be necessary

	// Now cause the interrupt
    cpu_cause_interrupt (1, Z80_NMI_INT);
*/
    return;
}

int slapfight_dpram_r(int offset)
{
    return slapfight_dpram[offset];
}



/* Slapfight CPU input/output ports

  These ports seem to control memory access

*/

/* Reset and hold sound CPU */
void slapfight_port_00_w(int offset, int data)
{
//	cpu_reset(1);
	cpu_halt(1,0);
}

/* Release reset on sound CPU */
void slapfight_port_01_w(int offset, int data)
{
	cpu_halt(1,1);
	cpu_reset(1);
	cpu_yield();
}

/* Disable and clear hardware interrupt */
void slapfight_port_06_w(int offset, int data)
{
	interrupt_enable_w(0,0);
}

/* Enable hardware interrupt */
void slapfight_port_07_w(int offset, int data)
{
	interrupt_enable_w(0,1);
}

void slapfight_port_08_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	cpu_setbank(1,&RAM[0x10000]);
}

void slapfight_port_09_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	cpu_setbank(1,&RAM[0x14000]);
}


/* Status register */

int  slapfight_port_00_r(int offset)
{
	int states[3]={ 0xc7, 0x55, 0x00 };

	slapfight_status = states[slapfight_status_state];

	slapfight_status_state++;
	if (slapfight_status_state > 2) slapfight_status_state = 0;

	return slapfight_status;
}
