/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "m6809/m6809.h"
#include "6821pia.h"
#include "sndhrdw/dac.h"

static void zoo_dac_w (int offset, int data);
static void zoo_pia_dint (void);
static void zoo_pia_sint (void);

/***************************************************************************

	Zookeeper has 6 PIAs on board:

	From the ROM I/O schematic:

	PIA 1 = U11:
		port A = external input (input_port_0)
		port B = external input (input_port_1)
	PIA 2 = U20:
		port A = external input (???)
		port B = external input (???)
	PIA 3 = U30:
		port A = external input (???)
		port B = external input (???)


	From the data/sound processor schematic:

	PIA 4 = U20:
		port A = data CPU to sound CPU communication
		port B = some kind of sound control
		CA1 = interrupt signal from sound CPU
		CA2 = interrupt signal to sound CPU
	PIA 5 = U8:
		port A = sound CPU to data CPU communication
		port B = DAC value (port B)
		CA1 = interrupt signal from data CPU
		CA2 = interrupt signal to data CPU
	PIA 6 = U7: (never actually used)
		port A = unused
		port B = sound CPU to TMS5220 communication
		CA1 = interrupt signal from TMS5220
		CA2 = write signal to TMS5220
		CB1 = ready signal from TMS5220
		CB2 = read signal to TMS5220

***************************************************************************/

static pia6821_interface pia_intf =
{
	6,                                             /* 6 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },    /* offsets */
	{ input_port_0_r, 0, 0, 0, 0, 0 },             /* input port A */
	{ input_port_1_r, 0, 0, 0, 0, 0 },             /* input port B */
	{ 0, 0, 0, pia_5_porta_w, pia_4_porta_w, 0 },  /* output port A */
	{ 0, 0, 0, 0, zoo_dac_w, 0 },                  /* output port B */
	{ 0, 0, 0, pia_5_ca1_w, pia_4_ca1_w, 0 },      /* output CA2 */
	{ 0, 0, 0, 0, 0, 0 },                          /* output CB2 */
	{ 0, 0, 0, zoo_pia_dint, zoo_pia_sint, 0 },    /* IRQ A */
	{ 0, 0, 0, zoo_pia_dint, zoo_pia_sint, 0 },    /* IRQ B */
};


unsigned char *zoo_sharedram;

/* Because all the scanlines are drawn at once, this has no real
   meaning. But the game sometimes waits for the scanline to be
   a particular value, so we fake it.
*/
int zookeeper_scanline_r(int offset)
{
	return 255 - (cpu_getfcount() * 256 / cpu_getfperiod());
}


void zookeeper_bankswitch_w(int offset,int data)
{
	if (data & 0x04)
		cpu_setbank (1, &RAM[0x10000])
	else
		cpu_setbank (1, &RAM[0xa000]);
}


void zookeeper_init_machine(void)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_NONE;

	pia_startup (&pia_intf);
}


int zoo_sharedram_r(int offset)
{
	return zoo_sharedram[offset];
}


void zoo_sharedram_w(int offset,int data)
{
	zoo_sharedram[offset] = data;
}


/***************************************************************************

	6821 PIA handlers

***************************************************************************/


static void zoo_dac_w (int offset, int data)
{
	DAC_data_w (0, data);
}


static void zoo_pia_dint (void)
{
	/* not used by Qix, but Zookeeper might use it; depends on a jumper on the PCB */
}

static void zoo_pia_sint (void)
{
	cpu_cause_interrupt (2, M6809_INT_IRQ);

	/* Sometimes the data CPU sends commands in quick succession; to prevent this from doing
	   bad things, we kill the rest of this CPU slice. Since commands are not sent that often,
	   and we're only losing 1/6000th of a second, I don't think it's a big deal */
	cpu_seticount (0);
}