/***************************************************************************

  This sound driver is used by the Scramble, Super Cobra  and Amidar drivers.

***************************************************************************/


#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



/* The timer clock which feeds the upper 4 bits of    */
/* AY-3-8910 port B is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 5120, formed by a standard divide by 512, followed */
/* by a divide by 10 using a 4 bit bi-quinary count   */
/* sequence. (See LS90 data sheet for an example)     */
/* The upper three bits come directly from the        */
/* upper three bits of the bi-quinary counter.        */
/* Bit 4 comes from the output of the divide by 512.  */

static int scramble_timer[20] = {
0x00, 0x10, 0x00, 0x10, 0x20, 0x30, 0x20, 0x30, 0x40, 0x50,
0x80, 0x90, 0x80, 0x90, 0xa0, 0xb0, 0xa0, 0xb0, 0xc0, 0xd0
};

static int scramble_portB_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 5120;

	last_totalcycles = current_totalcycles;

	return scramble_timer[clock/256];
}



void scramble_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && (data & 0x08) != 0)
	{
		/* setting bit 3 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);

	/* TODO: I shouldn't do this, but if I don't, in Minefield some commands */
	/* go lost regardless of the CPU interleaving factor (even 500 won't be */
	/* enough). The command which goes lost is the "turn on helicopter sound" at */
	/* the beginning of the game. Currently, calling seticount() means that we */
	/* throw away some CPU cycles for CPU #0. Hopefully this will be fixed when */
	/* proper slice control is implemented. */
		cpu_seticount(0);
	}

	last = data & 0x08;
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 Mhz */
	{ 0x60ff, 0x60ff },
	{ soundlatch_r },
	{ scramble_portB_r },
	{ 0 },
	{ 0 }
};



int scramble_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
