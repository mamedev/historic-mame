/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "6821pia.h"
#include "machine/ticket.h"


/* defined in vidhrdw/williams.c */
extern unsigned char *williams_videoram;

extern unsigned char* williams2_paletteram;

/* various banking controls */
unsigned char *williams_bank_base;
unsigned char *williams_video_counter;
unsigned char *defender_bank_base;
unsigned char *blaster_bank2_base;

/* internal bank switching tracks */
int blaster_bank;
int vram_bank;
int williams2_bank;

/* switches controlled by $c900 */
int sinistar_clip;
int williams_cocktail;

/* video counter offset */
static int port_select;
static unsigned char defender_video_counter;

/* various input port maps */
int williams_input_port_0_3 (int offset);
int williams_input_port_1_4 (int offset);
int stargate_input_port_0_r (int offset);
int sinistar_input_port_0_r (int offset);
int defender_input_port_0_r (int offset);
int lottofun_input_port_0_r (int offset);

/* Defender-specific code */
int defender_io_r (int offset);
void defender_io_w (int offset,int data);
void defender_bank_select_w (int offset, int data);

/* Colony 7-specific code */
void colony7_bank_select_w (int offset, int data);

void williams2_bank_select(int offset, int data);

void joust2_sound_bank_select_w(int offset,int data);

static int tshoot_input_port_0_3(int offset);
static void tshoot_pia0_B_w(int offset, int data);
static void tshoot_maxvol(int offset, int data);

/* PIA interface functions */
static void williams_irq (int state);
static void williams_firq(int state);
static void williams_snd_irq (int state);
static void williams_snd_cmd_w (int offset, int cmd);
static void williams_port_select_w (int offset, int data);
static void joust2_snd_nmi(int state);
static void joust2_snd_firq(int state);
static void j2_pia0_B(int offset, int data);
static void j2_pia0_CB2(int offset, int data);
static void j2_pia1_B(int offset, int data);
static void j2_pia1_CA2(int offset, int data);
static void j2_pia3_CA2(int offset, int data);
static void j2_pia3_CB2(int offset, int data);

/* external code to update part of the screen */
void williams_vh_update (int counter);
void williams2_vh_update(int counter);
int williams2_palette_w(int offset, int data);
void williams_videoram_w(int offset,int data);


/***************************************************************************

	Generic old-Williams PIA interfaces

***************************************************************************/

/* Generic PIA 0, maps to input ports 0 and 1 */
static struct pia6821_interface williams_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Generic muxing PIA 0, maps to input ports 0/3 and 1; port select is CB2 */
static struct pia6821_interface williams_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic dual muxing PIA 0, maps to input ports 0/3 and 1/4; port select is CB2 */
static struct pia6821_interface williams_dual_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3, williams_input_port_1_4, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic PIA 1, maps to input port 2, sound command out, and IRQs */
static struct pia6821_interface williams_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, williams_snd_cmd_w, 0, 0,
	/*irqs   : A/B             */ williams_irq, williams_irq
};

/* Generic PIA 2, maps to DAC data in and sound IRQs */
static struct pia6821_interface williams_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_data_w, 0, 0, 0,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};



/***************************************************************************

	Game-specific old-Williams PIA interfaces

***************************************************************************/

/* Special PIA 0 for Defender, to handle the controls */
static struct pia6821_interface defender_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ defender_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 0 for Stargate, to handle the controls */
static struct pia6821_interface stargate_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ stargate_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 0 for Lotto Fun, to handle the controls and ticket dispenser */
static struct pia6821_interface lottofun_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ lottofun_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, ticket_dispenser_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 0 for Sinistar/Blaster, to handle the 49-way joystick */
static struct pia6821_interface sinistar_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ sinistar_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 2 for Sinistar, to handle the CVSD */
static struct pia6821_interface sinistar_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_data_w, 0, CVSD_digit_w, CVSD_clock_w,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};



/***************************************************************************

	Generic later-Williams PIA interfaces

***************************************************************************/

/* Generic muxing PIA 0, maps to input ports 0/3 and 1; port select is CA2 */
static struct pia6821_interface williams2_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, williams_port_select_w, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Generic PIA 1, maps to input port 2, sound command out, and IRQs */
static struct pia6821_interface williams2_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, pia_2_porta_w, 0, pia_2_ca1_w,
	/*irqs   : A/B             */ williams_irq, williams_irq
};

/* Generic PIA 2, maps to DAC data in and sound IRQs */
static struct pia6821_interface williams2_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_1_portb_w, DAC_data_w, pia_1_cb1_w, 0,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};



/***************************************************************************

	Game-specific later-Williams PIA interfaces

***************************************************************************/

/* Mystic Marathon PIA 0 */
static struct pia6821_interface mysticm_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ williams_firq, williams_irq
};

/* Turkey Shoot PIA 0 */
static struct pia6821_interface tshoot_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ tshoot_input_port_0_3, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, tshoot_pia0_B_w, williams_port_select_w, 0,
	/*irqs   : A/B             */ williams_irq, williams_irq
};

/* Turkey Shoot PIA 2 */
static struct pia6821_interface tshoot_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_1_portb_w, DAC_data_w, pia_1_cb1_w, tshoot_maxvol,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};

/* Joust 2 PIA 1 */
static struct pia6821_interface joust2_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, j2_pia1_B, j2_pia1_CA2, pia_2_ca1_w,
	/*irqs   : A/B             */ williams_irq, williams_irq
};

/* Joust 2 PIA 3 */
static struct pia6821_interface joust2_extsnd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_data_w, 0, j2_pia3_CA2, j2_pia3_CB2,
	/*irqs   : A/B             */ joust2_snd_firq, joust2_snd_nmi
};



/***************************************************************************

	Common Williams routines

***************************************************************************/

/*
 *  Initialize the machine
 */

void robotron_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
}

void joust_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_muxed_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
}

void stargate_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &stargate_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
}

void bubbles_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
}

void splat_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_dual_muxed_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
}

void sinistar_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &sinistar_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &sinistar_snd_pia_intf);
	pia_reset();
}

void blaster_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &sinistar_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
}

void defender_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &defender_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
	williams_video_counter = &defender_video_counter;

	/* initialize the banks */
	defender_bank_select_w (0, 0);
}

void colony7_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();
	williams_video_counter = &defender_video_counter;

	/* initialize the banks */
	colony7_bank_select_w (0, 0);
}

void lottofun_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &lottofun_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams_snd_pia_intf);
	pia_reset();

	/* Initialize the ticket dispenser to 70 milliseconds */
	/* (I'm not sure what the correct value really is) */
	ticket_dispenser_init(70, TICKET_MOTOR_ACTIVE_LOW, TICKET_STATUS_ACTIVE_HIGH);
}

void mysticm_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &mysticm_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_snd_pia_intf);
	pia_reset();
	
	williams2_bank_select(0, 0);
}

void tshoot_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &tshoot_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &tshoot_snd_pia_intf);
	pia_reset();

	williams2_bank_select(0, 0);
}

void inferno_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_muxed_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_snd_pia_intf);
	pia_reset();
	
	williams2_bank_select(0, 0);
}

void joust2_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_muxed_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &joust2_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &williams2_snd_pia_intf);
	pia_config(3, PIA_STANDARD_ORDERING | PIA_8BIT, &joust2_extsnd_pia_intf);
	pia_reset();
	
	williams2_bank_select(0, 0);

	/*
		make sure sound board starts out
		in the reset state
    */
	joust2_sound_bank_select_w(0, 0);
	j2_pia0_CB2(0, 0);
}

/*
 *  Generic interrupt routine; interrupts are generated via the PIA, so we merely pulse the
 *  external inputs here; the PIA emulator will generate the interrupt if the proper conditions
 *  are met
 */

int williams_interrupt (void)
{
	/* the video counter value is taken from the six bits of VA8-VA13 */
	*williams_video_counter = ((64 - cpu_getiloops ()) << 2);

	/* the IRQ signal comes into CB1, and is set to VA11 */
	pia_1_cb1_w (0, *williams_video_counter & 0x20);

	/* the COUNT240 signal comes into CA1, and is set to the logical AND of VA10-VA13 */
	pia_1_ca1_w (0, (*williams_video_counter & 0xf0) == 0xf0);

	/* update the screen partially */
	williams_vh_update (*williams_video_counter);

	/* PIA generates interrupts, not us */
	return ignore_interrupt ();
}


/*
 *  Read either port 0 or 3, depending on the value in CB2
 */

int williams_input_port_0_3 (int offset)
{
	if (port_select)
		return input_port_3_r (0);
	else
		return input_port_0_r (0);
}


/*
 *  Read either port 2 or 3, depending on the value in CB2
 */

int williams_input_port_1_4 (int offset)
{
	if (port_select)
	   return input_port_4_r (0);
	else
	   return input_port_1_r (0);
}


/*
 *  Switch between VRAM and ROM
 */

void williams_vram_select_w (int offset, int data)
{
	vram_bank = data & 0x01;
	williams_cocktail = data & 0x02;
	sinistar_clip = data & 0x04;

	if (vram_bank)
	{
		cpu_setbank (1, williams_bank_base);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
	}
}

void williams2_memory_w(int offset, int data)
{
	if ((williams2_bank & 0x03) == 0x03)
	{
		if (0x8000 <= offset && offset < 0x8800)
		{
			williams2_palette_w(offset - 0x8000, data);
		}
		return;
	}

	williams_videoram_w(offset, data);
}

void williams2_bank_select(int offset, int data)
{
	static int bank[8] = { 0, 0x10000, 0x20000, 0x10000,
						   0, 0x30000, 0x40000, 0x30000 };

	williams2_bank = data & 0x07; /* only lower 3 bits used by IC56 */
	if (williams2_bank == 0)
	{
		cpu_setbank(1, williams_videoram);
		cpu_setbank(2, williams_videoram + 0x8000);
	}
	else
	{
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

		cpu_setbank(1, &RAM[bank[williams2_bank]]);

		if ((williams2_bank & 0x03) == 0x03)
		{
			cpu_setbank(2, williams2_paletteram);
		}
		else
		{
			cpu_setbank(2, williams_videoram + 0x8000);
		}
	}

	cpu_setbank(3, williams_videoram + 0x8800);
}

void williams2_watchdog(int offset, int data)
{
	if (data != 0x14)
	{
		if (errorlog)
			fprintf(errorlog, "watchdog: data != 0x14 : data = %d\n", data);
	}
}

void williams2_7segment(int offset, int data)
{
	int n;
	char dot;
	char buffer[5];

	switch (data & 0x7F)
	{
		case 0x40:	n = 0; break;
		case 0x79:	n = 1; break;
		case 0x24:	n = 2; break;
		case 0x30:	n = 3; break;
		case 0x19:	n = 4; break;
		case 0x12:	n = 5; break;
		case 0x02:	n = 6; break;
		case 0x03:	n = 6; break;
		case 0x78:	n = 7; break;
		case 0x00:	n = 8; break;
		case 0x18:	n = 9; break;
		case 0x10:	n = 9; break;
		default:	n =-1; break;
	}

	if ((data & 0x80) == 0x00)
		dot = '.';
	else
		dot = ' ';

	if (n == -1)
		sprintf(buffer, "[ %c]\n", dot);
	else
		sprintf(buffer, "[%d%c]\n", n, dot);

	if (errorlog)
		fputs(buffer, errorlog);
}

/*
 *  PIA callback to generate the interrupt to the main CPU
 */

static void williams_irq (int state)
{
	cpu_set_irq_line (0, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void williams_firq(int state)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
 *  PIA callback to generate the interrupt to the sound CPU
 */

static void williams_snd_irq (int state)
{
	cpu_set_irq_line (1, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


/*
 *  Handle a write to the PIA which communicates to the sound CPU
 */

static void williams_snd_cmd_real_w (int param)
{
	pia_2_portb_w (0, param);
	pia_2_cb1_w (0, (param == 0xff) ? 0 : 1);
}

static void williams_snd_cmd_w (int offset, int cmd)
{
	/* the high two bits are set externally, and should be 1 */
	timer_set (TIME_NOW, cmd | 0xc0, williams_snd_cmd_real_w);
}


/*
 *  Detect a switch between input port sets
 */

static void williams_port_select_w (int offset, int data)
{
	port_select = data;
}


/***************************************************************************

	Stargate-specific routines

***************************************************************************/

int stargate_input_port_0_r(int offset)
{
	int keys, altkeys;

	keys = input_port_0_r (0);
	altkeys = input_port_3_r (0);

	if (altkeys)
	{
		keys |= altkeys;
		if (Machine->memory_region[0][0x9c92] == 0xfd)
		{
			if (keys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (keys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}



/***************************************************************************

	Defender-specific routines

***************************************************************************/

/*
 *  Defender Select a bank
 *  There is just data in bank 0
 */

void defender_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x0c000, 0x0c000, 0x0c000, 0x13000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* set bank address */
	cpu_setbank (1, &RAM[bank[data & 7]]);

	if (bank[data] < 0x10000)
	{
		/* i/o area map */
		cpu_setbankhandler_r (1, defender_io_r);
		cpu_setbankhandler_w (1, defender_io_w);
	}
	else
	{
		/* bank rom map */
		cpu_setbankhandler_r (1, MRA_BANK1);
		cpu_setbankhandler_w (1, MWA_ROM);
	}
}


/*
 *   Defender Read at C000-CFFF
 */

int defender_input_port_0_r(int offset)
{
	int keys, altkeys;

	keys = readinputport(0);
	altkeys = readinputport(3);

	if (altkeys)
	{
		keys |= altkeys;
		if (Machine->memory_region[0][0xa0bb] == 0xfd)
		{
			if (keys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (keys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}

int defender_io_r (int offset)
{
	/* PIAs */
	if (offset >= 0x0c00 && offset < 0x0c04)
		return pia_1_r (offset - 0x0c00);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		return pia_0_r (offset - 0x0c04);

	/* video counter */
	else if (offset == 0x800)
		return *williams_video_counter;

	/* If not bank 0 then return banked RAM */
	return defender_bank_base[offset];
}


/*
 *  Defender Write at C000-CFFF
 */

void defender_io_w (int offset,int data)
{
	defender_bank_base[offset] = data;

	/* WatchDog */
	if (offset == 0x03fc)
		watchdog_reset_w (offset, data);

	/* Palette */
	else if (offset < 0x10)
		paletteram_BBGGGRRR_w(offset,data);

	/* PIAs */
	else if (offset >= 0x0c00 && offset < 0x0c04)
		pia_1_w (offset - 0x0c00, data);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		pia_0_w (offset - 0x0c04, data);
}



/***************************************************************************

	Colony 7-specific routines

***************************************************************************/

/*
 *  Colony7 Select a bank
 *  There is just data in bank 0
 */
void colony7_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x0c000, 0x0c000, 0x0c000, 0xc000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* set bank address */
	cpu_setbank (1, &RAM[bank[data & 7]]);
	if (bank[data] < 0x10000)
	{
		/* i/o area map */
		cpu_setbankhandler_r (1, defender_io_r);
		cpu_setbankhandler_w (1, defender_io_w);
	}
	else
	{
		/* bank rom map */
		cpu_setbankhandler_r (1, MRA_BANK1);
		cpu_setbankhandler_w (1, MWA_ROM);
	}
}


/***************************************************************************

	Defense Command-specific routines

***************************************************************************/

/*
 *  Defense Command Select a bank
 *  There is just data in bank 0
 */
void defcomnd_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x13000, 0x0c000, 0x0c000, 0x14000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	static int bankhit[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if ((errorlog) && (bankhit[data & 7] == 0))
		fprintf(errorlog,"bank = %02X\n",data);

	bankhit[data & 7] = 1;

	/* set bank address */
	cpu_setbank (1, &RAM[bank[data & 7]]);
	if (bank[data] < 0x10000)
	{
		/* i/o area map */
		cpu_setbankhandler_r (1, defender_io_r);
		cpu_setbankhandler_w (1, defender_io_w);
	}
	else
	{
		/* bank rom map */
		cpu_setbankhandler_r (1, MRA_BANK1);
		cpu_setbankhandler_w (1, MWA_ROM);
	}
}



/***************************************************************************

	Sinistar-specific routines

***************************************************************************/

/*
 *  Sinistar and Blaster Joystick
 *
 * The joystick has 48 positions + center.
 *
 * I'm not 100% sure but it looks like it's mapped this way:
 *
 *	xxxx1000 = up full
 *	xxxx1100 = up 2/3
 *	xxxx1110 = up 1/3
 *	xxxx0111 = center
 *	xxxx0011 = down 1/3
 *	xxxx0001 = down 1/3
 *	xxxx0000 = down 1/3
 *
 *	1000xxxx = right full
 *	1100xxxx = right 2/3
 *	1110xxxx = right 1/3
 *	0111xxxx = center
 *	0011xxxx = left 1/3
 *	0001xxxx = left 1/3
 *	0000xxxx = left 1/3
 *
 */
int sinistar_input_port_0_r(int offset)
{
	int joy_x,joy_y;
	int bits_x,bits_y;

	joy_x = readinputport(3) >> 4;	/* 0 = left 3 = center 6 = right */
	joy_y = readinputport(4) >> 4;	/* 0 = down 3 = center 6 = up */

	bits_x = (0x70 >> (7 - joy_x)) & 0x0f;
	bits_y = (0x70 >> (7 - joy_y)) & 0x0f;

	return (bits_x << 4) | bits_y;
}


/***************************************************************************

	Blaster-specific routines

***************************************************************************/


/*
 *  Blaster bank select
 */

static int bl_bank[16] = { 0x00000, 0x10000, 0x14000, 0x18000, 0x1c000, 0x20000, 0x24000, 0x28000,
								0x2c000, 0x30000, 0x34000, 0x38000, 0x2c000, 0x30000, 0x34000, 0x38000 };

void blaster_vram_select_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	vram_bank = data;
	if (vram_bank)
	{
		cpu_setbank (1, &RAM[blaster_bank[bl_bank]]);
		cpu_setbank (2, blaster_bank2_base);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
		cpu_setbank (2, williams_videoram + 0x4000);
	}
}


void blaster_bank_select_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	blaster_bank = data & 15;
	if (vram_bank)
	{
		cpu_setbank (1, &RAM[blaster_bank[bl_bank]]);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
	}
}



/***************************************************************************

	Lotto Fun-specific routines

***************************************************************************/

int lottofun_input_port_0_r(int offset)
{
	return input_port_0_r(offset) | ticket_dispenser_r(offset);
}

/***************************************************************************

	Mystic Marathon specific routines

***************************************************************************/

int mysticm_interrupt(void)
{
	/* the video counter value is taken from the eight bits of V6-V13 (IC24) */
	*williams_video_counter = 256 - cpu_getiloops();

	/* the IRQ signal comes into CB1, and is set to VA11 */
	pia_0_cb1_w(0, *williams_video_counter & 0x20);

	/* *END_SCREEN */
	if (*williams_video_counter == 0xFE)
		pia_0_ca1_w(0, 0);
	if (*williams_video_counter == 0x08)
		pia_0_ca1_w(0, 1);

	/* update the screen partially */
	williams2_vh_update(*williams_video_counter);

	/* PIA generates interrupts, not us */
	return ignore_interrupt();
}

/***************************************************************************

	Turkey Shoot specific routines

***************************************************************************/

int tshoot_interrupt(void)
{
	/* the video counter value is taken from the eight bits of V6-V13 (IC24) */
	*williams_video_counter = 256 - cpu_getiloops();

	/*
		The IRQ signal comes into CB1 of PIA0, AND CA1 of PIA1.
	*/

#if 0 /* HACK: the following 2 lines is a hack to get past crashes in tshoot: */
	pia_0_cb1_w(0, *williams_video_counter == 0x80);
	pia_1_ca1_w(0, *williams_video_counter == 0x80);
#else
	pia_0_cb1_w(0, *williams_video_counter & 0x20);
	pia_1_ca1_w(0, *williams_video_counter & 0x20);
#endif

	/* *END_SCREEN */
	if (*williams_video_counter == 0xFE)
		pia_0_ca1_w(0, 0);
	if (*williams_video_counter == 0x08)
		pia_0_ca1_w(0, 1);

	/* update the screen partially */
	williams2_vh_update(*williams_video_counter);

	/* PIA generates interrupts, not us */
	return ignore_interrupt();
}

static void tshoot_pia0_B_w(int offset, int data)
{
	/* grenade lamp */
	if (data & 0x04)
		osd_led_w(0, 1);
	else
		osd_led_w(0, 0);

	/* gun lamp */
	if (data & 0x08)
		osd_led_w(1, 1);
	else
		osd_led_w(1, 0);

#if 0
	/* gun coil */
	if (data & 0x10)
		printf("[gun coil] ");
	else
		printf("           ");

	/* feather coil */
	if (data & 0x20)
		printf("[feather coil] ");
	else
		printf("               ");

	printf("\n");
#endif
}

static int tshoot_input_port_0_3(int offset)
{
	int data;
	int gun;

	data = williams_input_port_0_3(offset);

	gun = (data & 0x3F) ^ ((data & 0x3F) >> 1);

	return (data & 0xC0) | gun;
}

static void tshoot_maxvol(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "tshoot maxvol = %d (pc:%x)\n", data, cpu_get_pc());
}

/***************************************************************************

	Joust2 specific routines

***************************************************************************/

int joust2_interrupt(void)
{
	/* the video counter value is taken from the eight bits of V6-V13 (IC24) */
	*williams_video_counter = 256 - cpu_getiloops();

	pia_1_ca1_w(0, *williams_video_counter & 0x20);

	/* update the screen partially */
	williams2_vh_update(*williams_video_counter);

	/* PIA generates interrupts, not us */
	return ignore_interrupt();
}

/* write data to sound PIA and to sound board PIA */
static void j2_pia1_B(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "pia1 B = %d (pc:%x)\n", data, cpu_get_pc());
	pia_2_porta_w(offset, data);
	pia_3_portb_w(offset, data);
}

/* CA2 is hooked up to the sound board PIA */
static void j2_pia1_CA2(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "pia0_CA2 = %d (pc:%x)\n", data, cpu_get_pc());
	pia_3_cb1_w(0, data);
}

/* pia2 has the controller input and empty portB */

static void j2_pia0_B(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "pia1 B = %d (pc:%x)\n", data, cpu_get_pc());
}

/* Connected to the 'reset' of the sound board cpu and pia */
static void j2_pia0_CB2(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "pia1_CB2 = %d (pc:%x)\n", data, cpu_get_pc());

	if (data == 0)
	{
		/* stop cpu */
		cpu_halt(2, 0);
	}
	else
	{
		/* start cpu */
		cpu_halt(2, 1);
		cpu_reset(2);
	}
}

static void j2_pia3_CA2(int offset, int data)
{
	/* ym2151 ~IC 'initial clear' */
	if (errorlog)
		fprintf(errorlog, "pia3 CA2 = %d (pc:%x)\n", data, cpu_get_pc());
}

static void j2_pia3_CB2(int offset, int data)
{
	/* to CPU board */
	if (errorlog)
		fprintf(errorlog, "pia3 CB2 = %d (pc:%x)\n", data, cpu_get_pc());
}

void joust2_ym2151_int(int irq)
{
	pia_3_ca1_w (0, irq==0);
	/* pia_3_ca1_w(0, 0); */
	/* pia_3_ca1_w(0, 1); */
}

void joust2_sound_bank_select_w (int offset,int data)
{
	static int bank[4] = { 0x08000, 0x10000, 0x18000, 0x08000 };

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
	/* set bank address */
	cpu_setbank(4, &RAM[bank[data & 0x03]]);
}

static void joust2_snd_nmi(int state)
{
	cpu_set_nmi_line(2, state ? ASSERT_LINE : CLEAR_LINE);
}

static void joust2_snd_firq(int state)
{
	cpu_set_irq_line(2, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/***************************************************************************

	Inferno specific routines

***************************************************************************/

int inferno_interrupt(void)
{
	/* same as Joust2? */
	return joust2_interrupt();
}


