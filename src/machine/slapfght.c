/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "z80\z80.h"


unsigned char *slapfight_bg_ram1;
unsigned char *slapfight_bg_ram2;
int slapfight_bg_ram_size;

unsigned char *slapfight_dpram;
int slapfight_dpram_size;


int slapfight_scroll_char_x;
int slapfight_scroll_char_y;
int slapfight_scroll_pixel_x;
int slapfight_scroll_pixel_y;

static int bankaddress;
static int cpu_int_enable;
static int sound_int_enable;

/* Perform basic machine initialisation */

void slapfight_init_machine(void)
{
	/* MAIN CPU */

	cpu_int_enable=0;
	bankaddress=0x10000;
	slapfight_scroll_char_x=0;
	slapfight_scroll_char_y=0;
	slapfight_scroll_pixel_x=0;
	slapfight_scroll_pixel_y=0;

	/* SOUND CPU */

	sound_int_enable=0;
        cpu_halt(1,0);
}

/* Interrupt handlers cpu & sound */

int slapfight_cpu_interrupt(void)
{
	return ((cpu_int_enable)?0xff:Z80_IGNORE_INT);
}

int slapfight_sound_interrupt(void)
{
	return Z80_IGNORE_INT;
}

void slapfight_dpram_w(int offset, int data)
{
    slapfight_dpram[offset]=data;
#ifdef MAME_DEBUG
    if (errorlog) fprintf(errorlog,"SLAPFIGHT MAIN  CPU : Write to   $c8%02x = %02x\n",offset,slapfight_dpram[offset]);
#endif
    cpu_cause_interrupt (1, Z80_NMI_INT);
    return;
}

int slapfight_dpram_r(int offset)
{
    return slapfight_dpram[offset];
}


int slapfight_bankrom_r(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    return RAM[bankaddress+offset];
}


/* Slapfight CPU input/output ports

  These ports seem to control memory access

*/

/* Reset and hold sound CPU */
void slapfight_port_00_w(int offset, int data)
{
//        cpu_reset(1);
        cpu_halt(1,0);
}

/* Release reset on sound CPU */
void slapfight_port_01_w(int offset, int data)
{
        cpu_halt(1,1);
	cpu_reset(1);
        cpu_yield();
}

void slapfight_port_02_w(int offset, int data) {}
void slapfight_port_03_w(int offset, int data) {}
void slapfight_port_04_w(int offset, int data) {}
void slapfight_port_05_w(int offset, int data) {}

/* Disable and clear hardware interrupt */
void slapfight_port_06_w(int offset, int data)
{
  cpu_int_enable=0;
}

/* Enable hardware interrupt */
void slapfight_port_07_w(int offset, int data)
{
  cpu_int_enable=1;
}

/* Scrolling registers

  Slapfight seems to be a little strange in that
  the scroll rountine at $1ee1 outputs the same
  value for pixel and char UNLESS pixel scroll
  is zero then it calulates a special value.
  Hence we only pickup the char_x if pixel_x
  is equal to zero.

  These two registers also control the bank switching
*/

void slapfight_port_08_w(int offset, int data)
{
  if(!slapfight_scroll_pixel_x)
    slapfight_scroll_char_x=data;

  bankaddress=0x10000;
}

void slapfight_port_09_w(int offset, int data)
{
  slapfight_scroll_pixel_x=data;

  bankaddress=0x14000;
}

void slapfight_port_0a_w(int offset, int data) {}
void slapfight_port_0b_w(int offset, int data) {}
void slapfight_port_0c_w(int offset, int data) {}
void slapfight_port_0d_w(int offset, int data) {}
void slapfight_port_0e_w(int offset, int data) {}
void slapfight_port_0f_w(int offset, int data) {}

int  slapfight_port_00_r(int offset) { return 0; }
int  slapfight_port_01_r(int offset) { return 0; }
int  slapfight_port_02_r(int offset) { return 0; }
int  slapfight_port_03_r(int offset) { return 0; }
int  slapfight_port_04_r(int offset) { return 0; }
int  slapfight_port_05_r(int offset) { return 0; }
int  slapfight_port_06_r(int offset) { return 0; }
int  slapfight_port_07_r(int offset) { return 0; }
int  slapfight_port_08_r(int offset) { return 0; }
int  slapfight_port_09_r(int offset) { return 0; }
int  slapfight_port_0a_r(int offset) { return 0; }
int  slapfight_port_0b_r(int offset) { return 0; }
int  slapfight_port_0c_r(int offset) { return 0; }
int  slapfight_port_0d_r(int offset) { return 0; }
int  slapfight_port_0e_r(int offset) { return 0; }
int  slapfight_port_0f_r(int offset) { return 0; }

