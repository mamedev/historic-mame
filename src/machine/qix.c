/***************************************************************************

  machine\qix.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "timer.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6805/m6805.h"
#include "6821pia.h"

static int sdungeon_coin_r (int offset);
static void sdungeon_coinctrl_w (int offset, int data);
static void sdungeon_coin_w (int offset, int data);

static int qix_sound_r (int offset);
static void qix_dac_w (int offset, int data);
static void qix_pia_sint (void);
static void qix_pia_dint (void);
static int suspended;

static int sdungeon_coinctrl;


/***************************************************************************

	Qix has 6 PIAs on board:

	From the ROM I/O schematic:

	PIA 1 = U11:
		port A = external input (input_port_0)
		port B = external input (input_port_1) (coin)
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

static pia6821_interface pia_intf_withmcu =
{
	6,                                             	/* 6 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB,
	  PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },    	/* offsets */
	{ input_port_0_r, input_port_2_r, input_port_4_r, 0, qix_sound_r, 0 },   	/* input port A  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA2 */
	{ sdungeon_coin_r, input_port_3_r, 0, 0, 0, 0 }, /* input port B  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB2 */
	{ 0, 0, 0, pia_5_porta_w, pia_4_porta_w, 0 },   /* output port A */
	{ sdungeon_coin_w, 0, sdungeon_coinctrl_w, 0, qix_dac_w, 0 },  /* output port B */
	{ 0, 0, 0, pia_5_ca1_w, pia_4_ca1_w, 0 },       /* output CA2 */
	{ 0, 0, 0, 0, 0, 0 },                          	/* output CB2 */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ A */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ B */
};

static pia6821_interface pia_intf =
{
	6,                                             	/* 6 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB,
	  PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },    	/* offsets */
	{ input_port_0_r, input_port_2_r, input_port_4_r, 0, qix_sound_r, 0 },   	/* input port A  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA2 */
	{ input_port_1_r, input_port_3_r, 0, 0, 0, 0 }, /* input port B  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB2 */
	{ 0, 0, 0, pia_5_porta_w, pia_4_porta_w, 0 },   /* output port A */
	{ 0, 0, 0, 0, qix_dac_w, 0 },                   /* output port B */
	{ 0, 0, 0, pia_5_ca1_w, pia_4_ca1_w, 0 },       /* output CA2 */
	{ 0, 0, 0, 0, 0, 0 },                          	/* output CB2 */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ A */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ B */
};



unsigned char *qix_sharedram;


int qix_sharedram_r(int offset)
{
	return qix_sharedram[offset];
}


void qix_sharedram_w(int offset,int data)
{
	qix_sharedram[offset] = data;
}


void zoo_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	if (data & 0x04) cpu_setbank (1, &RAM[0x10000])
	else cpu_setbank (1, &RAM[0xa000]);
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
	/* The +80&0xff thing is a hack to avoid flicker in Electric Yo-Yo */
	return (cpu_scalebyfcount(256) + 80) & 0xff;
}

void withmcu_init_machine(void)
{
	suspended = 0;

	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_S;/* | M6809_FAST_U;*/

	pia_startup (&pia_intf_withmcu);

	sdungeon_coinctrl = 0x00;
}

void qix_init_machine(void)
{
	suspended = 0;

	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_S;/* | M6809_FAST_U;*/

	pia_startup (&pia_intf);

	sdungeon_coinctrl = 0x00;
}

void zoo_init_machine(void)
{
	suspended = 0;

	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_NONE;

	pia_startup (&pia_intf_withmcu);
}


/***************************************************************************

	6821 PIA handlers

***************************************************************************/

static void qix_dac_w (int offset, int data)
{
	DAC_data_w (0, data);
}

int qix_sound_r (int offset)
{
	/* if we've suspended the main CPU for this, trigger it and give up some of our timeslice */
	if (suspended)
	{
		timer_trigger (500);
		cpu_yielduntil_time (TIME_IN_USEC (100));
		suspended = 0;
	}
	return pia_5_porta_r (offset);
}

static void qix_pia_dint (void)
{
	/* not used by Qix, but others might use it; depends on a jumper on the PCB */
}

static void qix_pia_sint (void)
{
	/* generate a sound interrupt */
	cpu_cause_interrupt (2, M6809_INT_IRQ);

	/* wait for the sound CPU to read the command */
	cpu_yielduntil_trigger (500);
	suspended = 1;

	/* but add a watchdog so that we're not hosed if interrupts are disabled */
	cpu_triggertime (TIME_IN_USEC (100), 500);
}

/***************************************************************************

        68705 Communication

***************************************************************************/

static unsigned char portA_in,portA_out,ddrA;
static unsigned char portB_in,portB_out,ddrB;
static unsigned char portC_in,portC_out,ddrC;

int sdungeon_68705_portA_r(int offset)
{
//if (errorlog) fprintf(errorlog,"PC: %x MCU PORTA R = %x\n",cpu_getpc(),portA_in);
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

void sdungeon_68705_portA_w(int offest,int data)
{
//if (errorlog) fprintf(errorlog,"PC: %x SD COINTOMAIN W: %x\n",cpu_getpc(),data);
	portA_out = data;
}

void sdungeon_68705_ddrA_w(int offset,int data)
{
	ddrA = data;
}


int sdungeon_68705_portB_r(int offset)
{
	portB_in = input_port_1_r(0) & 0x0F;
	portB_in = portB_in | ((input_port_1_r(0) & 0x80) >> 3);
//if (errorlog) fprintf(errorlog,"PC: %x MCU PORTB R = %x\n",cpu_getpc(),portB_in);

	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

void sdungeon_68705_portB_w(int offest,int data)
{
//if (errorlog) fprintf(errorlog,"PC: %x port B write %x\n",cpu_getpc(),data);
	portB_out = data;
}

void sdungeon_68705_ddrB_w(int offset,int data)
{
	ddrB = data;
}


int sdungeon_68705_portC_r(int offset)
{
	portC_in = (~sdungeon_coinctrl & 0x08) | ((input_port_1_r(0) & 0x70) >> 4);
//if (errorlog) fprintf(errorlog,"PC: %x MCU PORTC R = %x\n",cpu_getpc(),portC_in);

	return (portC_out & ddrC) | (portC_in & ~ddrC);
}

void sdungeon_68705_portC_w(int offest,int data)
{
//if (errorlog) fprintf(errorlog,"PC: %x port C write %x\n",cpu_getpc(),data);
	portC_out = data;
}

void sdungeon_68705_ddrC_w(int offset,int data)
{
	ddrC = data;
}



int sdungeon_coin_r (int offset)
{
	return portA_out;
}

static void sdungeon_coin_w (int offset, int data)
{
//if (errorlog) fprintf(errorlog,"PC: %x COIN COMMAND W: %x\n",cpu_getpc(),data);
	/* this is a callback called by pia_1_w(), so I don't need to synchronize */
	/* the CPUs - they have already been synchronized by sdungeon_pia_1_w() */
	portA_in = data;
}

static void sdungeon_coinctrl_w (int offset, int data)
{
//if (errorlog) fprintf(errorlog,"PC: %x COIN CTRL W: %x\n",cpu_getpc(),data);
	if (data & 0x04)
	{
		cpu_set_irq_line(3,0,ASSERT_LINE);
		/* spin for a while to let the 68705 write the result */
		cpu_spinuntil_time(TIME_IN_USEC(50));
	}
	else
		cpu_set_irq_line(3,0,CLEAR_LINE);

	/* this is a callback called by pia_1_w(), so I don't need to synchronize */
	/* the CPUs - they have already been synchronized by sdungeon_pia_1_w() */
	sdungeon_coinctrl = data;
}


static void pia_1_w_callback(int param)
{
	pia_1_w(param >> 8,param & 0xff);
}

void sdungeon_pia_1_w(int offset,int data)
{
//if (errorlog) fprintf(errorlog,"%04x: PIA 1 write offset %02x data %02x\n",cpu_getpc(),offset,data);

	/* Hack: Kram and Zoo Keeper for some reason (protection?) leave the port A */
	/* DDR set to 0xff, so they cannot read the player 1 controls. Here I force */
	/* the DDR to 0, so the controls work correctly. */
	if (offset == 0) data = 0;

	/* make all the CPUs synchronize, and only AFTER that write the command to the PIA */
	/* otherwise the 68705 will miss commands */
	timer_set(TIME_NOW,data | (offset << 8),pia_1_w_callback);
}
