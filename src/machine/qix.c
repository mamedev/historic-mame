/***************************************************************************

  machine\qix.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"
#include "6821pia.h"
#include "sndhrdw/dac.h"


static void qix_dac_w (int offset, int data);
static void qix_pia_dint (void);
static void qix_pia_sint (void);


/***************************************************************************

	Qix has 6 PIAs on board:

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
	{ 0, 0, 0, 0, qix_dac_w, 0 },                  /* output port B */
	{ 0, 0, 0, pia_5_ca1_w, pia_4_ca1_w, 0 },      /* output CA2 */
	{ 0, 0, 0, 0, 0, 0 },                          /* output CB2 */
	{ 0, 0, 0, qix_pia_dint, qix_pia_sint, 0 },    /* IRQ A */
	{ 0, 0, 0, qix_pia_dint, qix_pia_sint, 0 },    /* IRQ B */
};




unsigned char *qix_sharedram;


int qix_sharedram_r_1(int offset)
{
	return qix_sharedram[offset];
}


int qix_sharedram_r_2(int offset)
{
	return qix_sharedram[offset];
}


void qix_sharedram_w(int offset,int data)
{
	qix_sharedram[offset] = data;
}



void qix_video_firq_w(int offset, int data)
{
	/* generate firq for video cpu */
	cpu_cause_interrupt(1,M6809_INT_FIRQ);
}



void qix_data_firq_w(int offset, int data)
{
	/* generate firq for data cpu */
	cpu_cause_interrupt(0,M6809_INT_FIRQ);
}



/* Return the current video scan line */
int qix_scanline_r(int offset)
{
	return 255 - (cpu_getfcount() * 256 / cpu_getfperiod());
}



void qix_init_machine(void)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_S;/* | M6809_FAST_U;*/

	pia_startup (&pia_intf);
}


/***************************************************************************

	6821 PIA handlers

***************************************************************************/

static struct DACinterface dac_intf =
{
	1,
	441000,
	{ 255 },
	{ 0 },
};


static void qix_dac_w (int offset, int data)
{
	DAC_data_w (0, data);
}


static void qix_pia_dint (void)
{
	/* not used by Qix, but Zookeeper might use it; depends on a jumper on the PCB */
}

static void qix_pia_sint (void)
{
	cpu_cause_interrupt (2, M6809_INT_IRQ);

	/* Sometimes the data CPU sends commands in quick succession; to prevent this from doing
	   bad things, we kill the rest of this CPU slice. Since commands are not sent that often,
	   and we're only losing 1/6000th of a second, I don't think it's a big deal */
	cpu_seticount (0);
}

int qix_sh_start (void)
{
	return DAC_sh_start (&dac_intf);
}