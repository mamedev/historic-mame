/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6808/m6808.h"
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
static void tshoot_pia2_B_w(int offset, int data);
static void tshoot_maxvol(int offset, int data);

/* PIA interface functions */
static void williams_irq (void);
static void williams_firq(void);
static void williams_snd_irq (void);
static void williams_snd_cmd_w (int offset, int cmd);
static void williams_port_select_w (int offset, int data);
static void sinistar_snd_cmd_w (int offset, int cmd);
static void joust2_snd_nmi(void);
static void joust2_snd_firq(void);
static void j2_pia1_B(int offset, int data);
static void j2_pia1_CA2(int offset, int data);
static void j2_pia2_B(int offset, int data);
static void j2_pia2_CB2(int offset, int data);
static void j2_pia4_CA2(int offset, int data);
static void j2_pia4_CB2(int offset, int data);

/* external code to update part of the screen */
void williams_vh_update (int counter);
void williams2_vh_update(int counter);
int williams2_palette_w(int offset, int data);
void williams_videoram_w(int offset,int data);


/***************************************************************************

	PIA Interfaces for each game

***************************************************************************/

static pia6821_interface robotron_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ input_port_0_r, input_port_2_r, 0 },          /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface joust_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ williams_input_port_0_3, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ williams_port_select_w, 0, 0 },               /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface stargate_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ stargate_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface bubbles_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ input_port_0_r, input_port_2_r, 0 },          /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface splat_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ williams_input_port_0_3, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ williams_input_port_1_4, 0, 0 },              /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ williams_port_select_w, 0, 0 },               /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface sinistar_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ sinistar_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, sinistar_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, CVSD_digit_w },                         /* output CA2 */
	{ 0, 0, CVSD_clock_w },                         /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface blaster_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ sinistar_input_port_0_r, input_port_2_r, 0 },  /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface defender_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ defender_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface colony7_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ input_port_0_r, input_port_2_r, 0 },          /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface lottofun_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ lottofun_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ ticket_dispenser_w, williams_snd_cmd_w, 0 },  /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface mysticm_pia_intf =
{
	3,														/* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB }, 			/* offsets */
	{ input_port_0_r, input_port_1_r,				 0 },	/* input port A */
	{			   0,			   0,				 0 },	/* input bit CA1 */
	{			   0,			   0,				 0 },	/* input bit CA2 */
	{			   0,			   0,				 0 },	/* input port B */
	{			   0,			   0,				 0 },	/* input bit CB1 */
	{			   0,			   0,				 0 },	/* input bit CB2 */
	{			   0,			   0,	 pia_1_portb_w },	/* output port A */
	{  pia_3_porta_w,			   0,		DAC_data_w },	/* output port B */
	{			   0,			   0,	   pia_1_cb1_w },	/* output CA2 */
	{	 pia_3_ca1_w,			   0,				 0 },	/* output CB2 */
	{	williams_irq,  williams_firq, williams_snd_irq },	/* IRQ A */
	{	williams_irq,	williams_irq, williams_snd_irq }	/* IRQ B */
};

static pia6821_interface tshoot_pia_intf =
{
	3,																/* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB }, 					/* offsets */
	{ input_port_2_r,   tshoot_input_port_0_3,				  0 },	/* input port A */
	{			   0,						0,				  0 },	/* input bit CA1 */
	{			   0,						0,				  0 },	/* input bit CA2 */
	{			   0,		   input_port_1_r,				  0 },	/* input port B */
	{			   0,						0,				  0 },	/* input bit CB1 */
	{			   0,						0,				  0 },	/* input bit CB2 */
	{			   0,						0,	  pia_1_portb_w },	/* output port A */
	{  pia_3_porta_w,		  tshoot_pia2_B_w,		 DAC_data_w },	/* output port B */
	{			   0,  williams_port_select_w,		pia_1_cb1_w },	/* output CA2 */
	{	 pia_3_ca1_w,						0,	  tshoot_maxvol },	/* output CB2 */
	{	williams_irq,			 williams_irq, williams_snd_irq },	/* IRQ A */
	{	williams_irq,			 williams_irq, williams_snd_irq }	/* IRQ B */
};

static pia6821_interface inferno_pia_intf =
{
	3,																/* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB }, 					/* offsets */
	{ input_port_2_r, williams_input_port_0_3,				  0 },	/* input port A */
	{			   0,						0,				  0 },	/* input bit CA1 */
	{			   0,						0,				  0 },	/* input bit CA2 */
	{			   0,		   input_port_1_r,				  0 },	/* input port B */
	{			   0,						0,				  0 },	/* input bit CB1 */
	{			   0,						0,				  0 },	/* input bit CB2 */
	{			   0,						0,	  pia_1_portb_w },	/* output port A */
	{  pia_3_porta_w,						0,		 DAC_data_w },	/* output port B */
	{			   0,  williams_port_select_w,		pia_1_cb1_w },	/* output CA2 */
	{	 pia_3_ca1_w,						0,				  0 },	/* output CB2 */
	{	williams_irq,						0, williams_snd_irq },	/* IRQ A */
	{	williams_irq,						0, williams_snd_irq }	/* IRQ B */
};

static pia6821_interface joust2_pia_intf =
{
	4,																				/* 4 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB }, 									/* offsets */
	{ input_port_2_r, williams_input_port_0_3,				   0, 0},				/* input port A */
	{			   0,						0,				   0, 0},				/* input bit CA1 */
	{			   0,						0,				   0, 0},				/* input bit CA2 */
	{			   0,						0,				   0, 0},				/* input port B */
	{			   0,						0,				   0, 0},				/* input bit CB1 */
	{			   0,						0,				   0, 0},				/* input bit CB2 */
	{			   0,						0,	   pia_1_portb_w, DAC_data_w},		/* output port A */
	{	   j2_pia1_B,						0,		  DAC_data_w, 0},				/* output port B */
	{	 j2_pia1_CA2,  williams_port_select_w,		 pia_1_cb1_w, j2_pia4_CA2}, 	/* output CA2 */
	{	 pia_3_ca1_w,			  j2_pia2_CB2,				   0, j2_pia4_CB2}, 	/* output CB2 */
	{	williams_irq,						0,	williams_snd_irq, joust2_snd_firq}, /* IRQ A */
	{	williams_irq,						0,	williams_snd_irq, joust2_snd_nmi}	/* IRQ B */
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
	pia_startup (&robotron_pia_intf);
}

void joust_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&joust_pia_intf);
}

void stargate_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&stargate_pia_intf);
}

void bubbles_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&bubbles_pia_intf);
}

void splat_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&splat_pia_intf);
}

void sinistar_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&sinistar_pia_intf);
}

void blaster_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&blaster_pia_intf);
}

void defender_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&defender_pia_intf);
	williams_video_counter = &defender_video_counter;

	/* initialize the banks */
	defender_bank_select_w (0, 0);
}

void colony7_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&colony7_pia_intf);
	williams_video_counter = &defender_video_counter;

	/* initialize the banks */
	colony7_bank_select_w (0, 0);
}

void lottofun_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&lottofun_pia_intf);

	/* Initialize the ticket dispenser to 70 milliseconds */
	/* (I'm not sure what the correct value really is) */
	ticket_dispenser_init(70, TICKET_MOTOR_ACTIVE_LOW, TICKET_STATUS_ACTIVE_HIGH);
}

void mysticm_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup(&mysticm_pia_intf);
	williams2_bank_select(0, 0);
}

void tshoot_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup(&tshoot_pia_intf);
	williams2_bank_select(0, 0);
}

void inferno_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup(&inferno_pia_intf);
	williams2_bank_select(0, 0);
}

void joust2_init_machine(void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup(&joust2_pia_intf);
	williams2_bank_select(0, 0);

	/*
		make sure sound board starts out
		in the reset state
    */
	joust2_sound_bank_select_w(0, 0);
	j2_pia2_CB2(0, 0);
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
	pia_2_cb1_w (0, *williams_video_counter & 0x20);

	/* the COUNT240 signal comes into CA1, and is set to the logical AND of VA10-VA13 */
	pia_2_ca1_w (0, (*williams_video_counter & 0xf0) == 0xf0);

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

static void williams_irq (void)
{
	cpu_cause_interrupt (0, M6809_INT_IRQ);
}

static void williams_firq(void)
{
	cpu_cause_interrupt(0, M6809_INT_FIRQ);
}

/*
 *  PIA callback to generate the interrupt to the sound CPU
 */

static void williams_snd_irq (void)
{
	cpu_cause_interrupt (1, M6808_INT_IRQ);
}


/*
 *  Handle a write to the PIA which communicates to the sound CPU
 */

static void williams_snd_cmd_real_w (int param)
{
	pia_3_portb_w (0, param);
	pia_3_cb1_w (0, (param == 0xff) ? 0 : 1);
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
		return pia_2_r (offset - 0x0c00);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		return pia_1_r (offset - 0x0c04);

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
		pia_2_w (offset - 0x0c00, data);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		pia_1_w (offset - 0x0c04, data);
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


static void sinistar_snd_cmd_w (int offset, int cmd)
{
/*	if (errorlog) fprintf (errorlog, "snd command: %02x %d\n", cmd, cmd); */

	timer_set (TIME_NOW, cmd | 0xc0, williams_snd_cmd_real_w);
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
	pia_2_cb1_w(0, *williams_video_counter & 0x20);

	/* *END_SCREEN */
	if (*williams_video_counter == 0xFE)
		pia_2_ca1_w(0, 0);
	if (*williams_video_counter == 0x08)
		pia_2_ca1_w(0, 1);

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
		The IRQ signal comes into CB1 of PIA2, AND CA1 of PIA1.
	*/

#if 1 /* HACK: the following 2 lines is a hack to get past crashes in tshoot: */
	pia_2_cb1_w(0, *williams_video_counter == 0x80);
	pia_1_ca1_w(0, *williams_video_counter == 0x80);
#else
	pia_2_cb1_w(0, *williams_video_counter & 0x20);
	pia_1_ca1_w(0, *williams_video_counter & 0x20);
#endif

	/* *END_SCREEN */
	if (*williams_video_counter == 0xFE)
		pia_2_ca1_w(0, 0);
	if (*williams_video_counter == 0x08)
		pia_2_ca1_w(0, 1);

	/* update the screen partially */
	williams2_vh_update(*williams_video_counter);

	/* PIA generates interrupts, not us */
	return ignore_interrupt();
}

static void tshoot_pia2_B_w(int offset, int data)
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
		fprintf(errorlog, "tshoot maxvol = %d (pc:%x)\n", data, cpu_getpc());
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
		fprintf(errorlog, "pia1 B = %d (pc:%x)\n", data, cpu_getpc());
	pia_3_porta_w(offset, data);
	pia_4_portb_w(offset, data);
}

/* CA2 is hooked up to the sound board PIA */
static void j2_pia1_CA2(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "pia1_CA2 = %d (pc:%x)\n", data, cpu_getpc());
	pia_4_cb1_w(0, data);
}

/* pia2 has the controller input and empty portB */

static void j2_pia2_B(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "pia2 B = %d (pc:%x)\n", data, cpu_getpc());
}

/* Connected to the 'reset' of the sound board cpu and pia */
static void j2_pia2_CB2(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "pia2_CB2 = %d (pc:%x)\n", data, cpu_getpc());

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

static void j2_pia4_CA2(int offset, int data)
{
	/* ym2151 ~IC 'initial clear' */
	if (errorlog)
		fprintf(errorlog, "pia4 CA2 = %d (pc:%x)\n", data, cpu_getpc());
}

static void j2_pia4_CB2(int offset, int data)
{
	/* to CPU board */
	if (errorlog)
		fprintf(errorlog, "pia4 CB2 = %d (pc:%x)\n", data, cpu_getpc());
}

void joust2_ym2151_int(void)
{
	pia_4_ca1_w(0, 0);
	pia_4_ca1_w(0, 1);
}

void joust2_sound_bank_select_w (int offset,int data)
{
	static int bank[4] = { 0x08000, 0x10000, 0x18000, 0x08000 };

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
	/* set bank address */
	cpu_setbank(4, &RAM[bank[data & 0x03]]);
}

static void joust2_snd_nmi(void)
{
	cpu_cause_interrupt(2, M6809_INT_NMI);
}

static void joust2_snd_firq(void)
{
	cpu_cause_interrupt(2, M6809_INT_FIRQ);
}

/***************************************************************************

	Inferno specific routines

***************************************************************************/

int inferno_interrupt(void)
{
	/* same as Joust2? */
	return joust2_interrupt();
}


