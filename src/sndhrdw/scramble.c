/***************************************************************************

  This sound driver is used by the Scramble, Super Cobra  and Amidar drivers.

***************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/7474.h"



/* The timer clock which feeds the upper 4 bits of    					*/
/* AY-3-8910 port A is based on the same clock        					*/
/* feeding the sound CPU Z80.  It is a divide by      					*/
/* 5120, formed by a standard divide by 512,        					*/
/* followed by a divide by 10 using a 4 bit           					*/
/* bi-quinary count sequence. (See LS90 data sheet    					*/
/* for an example).                                   					*/
/*																		*/
/* Bit 4 comes from the output of the divide by 1024  					*/
/*       0, 1, 0, 1, 0, 1, 0, 1, 0, 1									*/
/* Bit 5 comes from the QC output of the LS90 producing a sequence of	*/
/* 		 0, 0, 1, 1, 0, 0, 1, 1, 1, 0									*/
/* Bit 6 comes from the QD output of the LS90 producing a sequence of	*/
/*		 0, 0, 0, 0, 1, 0, 0, 0, 0, 1									*/
/* Bit 7 comes from the QA output of the LS90 producing a sequence of	*/
/*		 0, 0, 0, 0, 0, 1, 1, 1, 1, 1			 						*/

static int scramble_timer[10] =
{
	0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0
};

READ_HANDLER( scramble_portB_r )
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 5120;

	last_totalcycles = current_totalcycles;

	return scramble_timer[clock/512];
}



WRITE_HANDLER( scramble_sh_irqtrigger_w )
{
	/* the complement of bit 3 is connected to the flip-flop's clock */
	TTL7474_set_inputs(0, -1, -1, ~data & 0x08, -1);
}

static int scramble_sh_irq_callback(int irqline)
{
	/* interrupt acknowledge clears the flip-flop --
	   we need to pulse the CLR line because MAME's core never clears this
	   line, only asserts it */
	TTL7474_set_inputs(0, 1, -1, -1, -1);
	TTL7474_set_inputs(0, 0, -1, -1, -1);

	return 0;
}

static void scramble_sh_7474_callback(void)
{
	/* the Q bar is connected to the Z80's INT line.  But since INT is complemented, */
	/* we need to complement Q bar */
	cpu_set_irq_line(1, 0, !TTL7474_output_comp_r(0) ? ASSERT_LINE : CLEAR_LINE);
}

WRITE_HANDLER( hotshock_sh_irqtrigger_w )
{
	cpu_set_irq_line(1, 0, PULSE_LINE);
}


static void filter_w(int chip, int channel, int data)
{
	int C;


	C = 0;
	if (data & 1) C += 220000;	/* 220000pF = 0.220uF */
	if (data & 2) C +=  47000;	/*  47000pF = 0.047uF */
	set_RC_filter(3*chip + channel,1000,5100,0,C);
}

WRITE_HANDLER( scramble_filter_w )
{
	filter_w(1, 0, (offset >>  0) & 3);
	filter_w(1, 1, (offset >>  2) & 3);
	filter_w(1, 2, (offset >>  4) & 3);
	filter_w(0, 0, (offset >>  6) & 3);
	filter_w(0, 1, (offset >>  8) & 3);
	filter_w(0, 2, (offset >> 10) & 3);
}


static const struct TTL7474_interface scramble_sh_7474_intf =
{
	scramble_sh_7474_callback
};

void scramble_sh_init(void)
{
	cpu_set_irq_callback(1, scramble_sh_irq_callback);

	TTL7474_config(0, &scramble_sh_7474_intf);

	/* PR is always 0, D is always 1 */
	TTL7474_set_inputs(0, 0, 0, -1, 1);
}
