/***************************************************************************

	sndhrdw/mcr.c

	Functions to emulate general the various MCR sound cards.

***************************************************************************/

#include <stdio.h>

#include "driver.h"
#include "machine/mcr.h"
#include "sndhrdw/mcr.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"



/*************************************
 *
 *	Global variables
 *
 *************************************/

UINT8 mcr_sound_config;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT16 dacval;

/* SSIO-specific globals */
static UINT8 ssio_sound_cpu;
static UINT8 ssio_data[4];
static UINT8 ssio_status;

/* Chip Squeak Deluxe-specific globals */
static UINT8 csdeluxe_sound_cpu;
static UINT8 csdeluxe_dac_index;
extern struct pia6821_interface csdeluxe_pia_intf;

/* Turbo Chip Squeak-specific globals */
static UINT8 turbocs_sound_cpu;
extern struct pia6821_interface turbocs_pia_intf;

/* Sounds Good-specific globals */
static UINT8 soundsgood_sound_cpu;
extern struct pia6821_interface soundsgood_pia_intf;

/* Squawk n' Talk-specific globals */
static UINT8 squawkntalk_sound_cpu;
static UINT8 squawkntalk_tms_command;
static UINT8 squawkntalk_tms_strobes;
extern struct pia6821_interface squawkntalk_pia0_intf;
extern struct pia6821_interface squawkntalk_pia1_intf;

/* Advanced Audio-specific globals */
static UINT8 advaudio_sound_cpu;
extern struct pia6821_interface advaudio_pia_intf;



/*************************************
 *
 *	Generic MCR sound initialization
 *
 *************************************/

void mcr_sound_init(void)
{
	int sound_cpu = 1;
	int dac_index = 0;
	
	/* SSIO */
	if (mcr_sound_config & MCR_SSIO)
	{
		ssio_sound_cpu = sound_cpu++;
		ssio_reset_w(1);
		ssio_reset_w(0);
	}
	
	/* Turbo Chip Squeak */
	if (mcr_sound_config & MCR_TURBO_CHIP_SQUEAK)
	{
		pia_config(0, PIA_ALTERNATE_ORDERING, &turbocs_pia_intf);
		dac_index++;
		turbocs_sound_cpu = sound_cpu++;
		turbocs_reset_w(1);
		turbocs_reset_w(0);
	}
		
	/* Chip Squeak Deluxe */
	if (mcr_sound_config & MCR_CHIP_SQUEAK_DELUXE)
	{
		/* special case: Spy Hunter 2 has both Turbo CS and CS Deluxe, so we use PIA slot 1 */
		pia_config(1, PIA_ALTERNATE_ORDERING | PIA_16BIT_AUTO, &csdeluxe_pia_intf);
		csdeluxe_dac_index = dac_index++;
		csdeluxe_sound_cpu = sound_cpu++;
		csdeluxe_reset_w(1);
		csdeluxe_reset_w(0);
	}
	
	/* Sounds Good */
	if (mcr_sound_config & MCR_SOUNDS_GOOD)
	{
		pia_config(0, PIA_ALTERNATE_ORDERING | PIA_16BIT_UPPER, &soundsgood_pia_intf);
		dac_index++;
		soundsgood_sound_cpu = sound_cpu++;
		soundsgood_reset_w(1);
		soundsgood_reset_w(0);
	}
		
	/* Squawk n Talk */
	if (mcr_sound_config & MCR_SQUAWK_N_TALK)
	{
		pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &squawkntalk_pia0_intf);
		pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &squawkntalk_pia1_intf);
		squawkntalk_sound_cpu = sound_cpu++;
		squawkntalk_reset_w(1);
		squawkntalk_reset_w(0);
	}
		
	/* Advanced Audio */
	if (mcr_sound_config & MCR_ADVANCED_AUDIO)
	{
		pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &advaudio_pia_intf);
		dac_index++;
		advaudio_sound_cpu = sound_cpu++;
		advaudio_reset_w(1);
		advaudio_reset_w(0);
	}
	
	/* reset any PIAs */
	pia_reset();
}



/*************************************
 *
 *	MCR SSIO communications
 *
 *	Z80, 2 AY-3812
 *
 *************************************/

/********* internal interfaces ***********/
static void ssio_status_w(int offset,int data)
{
	ssio_status = data;
}

static int ssio_data_r(int offset)
{
	return ssio_data[offset];
}

static void ssio_delayed_data_w(int param)
{
	ssio_data[param >> 8] = param & 0xff;
}


/********* external interfaces ***********/
void ssio_data_w(int offset, int data)
{
	timer_set(TIME_NOW, (offset << 8) | (data & 0xff), ssio_delayed_data_w);
}

int ssio_status_r(int offset)
{
	return ssio_status;
}

void ssio_reset_w(int state)
{
	/* going high halts the CPU */
	if (state && cpu_getstatus(ssio_sound_cpu))
		cpu_halt(ssio_sound_cpu, 0);
	
	/* going low resets and reactivates the CPU */
	else if (!state && !cpu_getstatus(ssio_sound_cpu))
	{
		int i;

		cpu_reset(ssio_sound_cpu);
		cpu_halt(ssio_sound_cpu, 1);
		
		/* latches also get reset */
		for (i = 0; i < 4; i++)
			ssio_data[i] = 0;
		ssio_status = 0;
	}
}


/********* sound interfaces ***********/
struct AY8910interface ssio_ay8910_interface =
{
	2,			/* 2 chips */
	2000000,	/* 2 MHz ?? */
	{ MIXER(33,MIXER_PAN_LEFT), MIXER(33,MIXER_PAN_RIGHT) },	/* dotron clips with anything higher */
	AY8910_DEFAULT_GAIN,
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


/********* memory interfaces ***********/
struct MemoryReadAddress ssio_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9003, ssio_data_r },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xb001, 0xb001, AY8910_read_port_1_r },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0xf000, 0xf000, input_port_5_r },
	{ -1 }	/* end of table */
};

struct MemoryWriteAddress ssio_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_0_w },
	{ 0xb000, 0xb000, AY8910_control_port_1_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xc000, 0xc000, ssio_status_w },
	{ 0xe000, 0xe000, MWA_NOP },
	{ -1 }	/* end of table */
};



/*************************************
 *
 *	Chip Squeak Deluxe communications
 *
 *	MC68000, 1 PIA, 10-bit DAC
 *
 *************************************/

/********* internal interfaces ***********/
static void csdeluxe_porta_w(int offset, int data)
{
	dacval = (dacval & ~0x3fc) | (data << 2);
	DAC_signed_data_w(csdeluxe_dac_index, dacval >> 2);
}

static void csdeluxe_portb_w(int offset, int data)
{
	dacval = (dacval & ~0x003) | (data >> 6);
	/* only update when the MSB's are changed */
}

static void csdeluxe_irq(int state)
{
  	cpu_set_irq_line(csdeluxe_sound_cpu, 4, state ? ASSERT_LINE : CLEAR_LINE);
}

static void csdeluxe_delayed_data_w(int param)
{
	pia_1_portb_w(0, param & 0x0f);
	pia_1_ca1_w(0, ~param & 0x10);
}


/********* external interfaces ***********/
void csdeluxe_data_w(int offset, int data)
{
	timer_set(TIME_NOW, data, csdeluxe_delayed_data_w);
}

void csdeluxe_reset_w(int state)
{
	/* going high halts the CPU */
	if (state && cpu_getstatus(csdeluxe_sound_cpu))
		cpu_halt(csdeluxe_sound_cpu, 0);
	
	/* going low resets and reactivates the CPU */
	else if (!state && !cpu_getstatus(csdeluxe_sound_cpu))
	{
		cpu_reset(csdeluxe_sound_cpu);
		cpu_halt(csdeluxe_sound_cpu, 1);
	}
}


/********* sound interfaces ***********/
struct DACinterface csdeluxe_dac_interface =
{
	1,
	{ 80 }
};

struct DACinterface turbocs_plus_csdeluxe_dac_interface =
{
	2,
	{ 80, 80 }
};


/********* memory interfaces ***********/
struct MemoryReadAddress csdeluxe_readmem[] =
{
	{ 0x000000, 0x007fff, MRA_ROM },
	{ 0x018000, 0x018007, pia_1_r },
	{ 0x01c000, 0x01cfff, MRA_BANK1 },
	{ -1 }	/* end of table */
};

struct MemoryWriteAddress csdeluxe_writemem[] =
{
	{ 0x000000, 0x007fff, MWA_ROM },
	{ 0x018000, 0x018007, pia_1_w },
	{ 0x01c000, 0x01cfff, MWA_BANK1 },
	{ -1 }	/* end of table */
};


/********* PIA interfaces ***********/

/* Note: we map this board to PIA #1. It is only used in Spy Hunter and Spy Hunter 2 */
/* For Spy Hunter 2, we also have a Turbo Chip Squeak in PIA slot 0, so we don't want */
/* to interfere */
struct pia6821_interface csdeluxe_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ csdeluxe_porta_w, csdeluxe_portb_w, 0, 0,
	/*irqs   : A/B             */ csdeluxe_irq, csdeluxe_irq
};



/*************************************
 *
 *	MCR Sounds Good communications
 *
 *	MC68000, 1 PIA, 10-bit DAC
 *
 *************************************/

/********* internal interfaces ***********/
static void soundsgood_porta_w(int offset, int data)
{
	dacval = (dacval & ~0x3fc) | (data << 2);
	DAC_signed_data_w(0, dacval >> 2);
}

static void soundsgood_portb_w(int offset, int data)
{
	dacval = (dacval & ~0x003) | (data >> 6);
	/* only update when the MSB's are changed */
}

static void soundsgood_irq(int state)
{
  	cpu_set_irq_line(soundsgood_sound_cpu, 4, state ? ASSERT_LINE : CLEAR_LINE);
}

static void soundsgood_delayed_data_w(int param)
{
	pia_0_portb_w(0, (param >> 1) & 0x0f);
	pia_0_ca1_w(0, ~param & 0x01);
}


/********* external interfaces ***********/
void soundsgood_data_w(int offset, int data)
{
	timer_set(TIME_NOW, data, soundsgood_delayed_data_w);
}

void soundsgood_reset_w(int state)
{
	/* going high halts the CPU */
	if (state && cpu_getstatus(soundsgood_sound_cpu))
		cpu_halt(soundsgood_sound_cpu, 0);
	
	/* going low resets and reactivates the CPU */
	else if (!state && !cpu_getstatus(soundsgood_sound_cpu))
	{
		cpu_reset(soundsgood_sound_cpu);
		cpu_halt(soundsgood_sound_cpu, 1);
	}
}


/********* memory interfaces ***********/
struct MemoryReadAddress soundsgood_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x060000, 0x060007, pia_0_r },
	{ 0x070000, 0x070fff, MRA_BANK1 },
	{ -1 }	/* end of table */
};

struct MemoryWriteAddress soundsgood_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x060000, 0x060007, pia_0_w },
	{ 0x070000, 0x070fff, MWA_BANK1 },
	{ -1 }	/* end of table */
};


/********* PIA interfaces ***********/
struct pia6821_interface soundsgood_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ soundsgood_porta_w, soundsgood_portb_w, 0, 0,
	/*irqs   : A/B             */ soundsgood_irq, soundsgood_irq
};



/*************************************
 *
 *	MCR Turbo Chip Squeak communications
 *
 *	MC6809, 1 PIA, 8-bit DAC
 *
 *************************************/

/********* internal interfaces ***********/
static void turbocs_irq(int state)
{
	cpu_set_irq_line(turbocs_sound_cpu, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void turbocs_delayed_data_w(int param)
{
	pia_0_portb_w(0, (param >> 1) & 0x0f);
	pia_0_ca1_w(0, ~param & 0x01);
}


/********* external interfaces ***********/
void turbocs_data_w(int offset, int data)
{
	timer_set(TIME_NOW, data, turbocs_delayed_data_w);
}

void turbocs_reset_w(int state)
{
	/* going high halts the CPU */
	if (state && cpu_getstatus(turbocs_sound_cpu))
		cpu_halt(turbocs_sound_cpu, 0);
	
	/* going low resets and reactivates the CPU */
	else if (!state && !cpu_getstatus(turbocs_sound_cpu))
	{
		cpu_reset(turbocs_sound_cpu);
		cpu_halt(turbocs_sound_cpu, 1);
	}
}


/********* memory interfaces ***********/
struct MemoryReadAddress turbocs_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x6000, 0x6003, pia_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

struct MemoryWriteAddress turbocs_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x6000, 0x6003, pia_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


/********* PIA interfaces ***********/
struct pia6821_interface turbocs_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_data_w, 0, 0, 0,
	/*irqs   : A/B             */ turbocs_irq, turbocs_irq
};



/*************************************
 *
 *	MCR Squawk n Talk communications
 *
 *	MC6802, 2 PIAs, TMS5220, AY8912 (not used), 8-bit DAC (not used)
 *
 *************************************/

/********* internal interfaces ***********/
static void squawkntalk_porta1_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "Write to AY-8912\n");
}

static void squawkntalk_porta2_w(int offset, int data)
{
	squawkntalk_tms_command = data;
}

static void squawkntalk_portb2_w(int offset, int data)
{
	/* bits 0-1 select read/write strobes on the TMS5220 */
	data &= 0x03;
	
	/* write strobe -- pass the current command to the TMS5220 */
	if (((data ^ squawkntalk_tms_strobes) & 0x02) && !(data & 0x02))
	{
		tms5220_data_w(offset, squawkntalk_tms_command);

		/* DoT expects the ready line to transition on a command/write here, so we oblige */
		pia_1_ca2_w(0, 1);
		pia_1_ca2_w(0, 0);
	}

	/* read strobe -- read the current status from the TMS5220 */
	else if (((data ^ squawkntalk_tms_strobes) & 0x01) && !(data & 0x01))
	{
		pia_1_porta_w(0, tms5220_status_r(offset));

		/* DoT expects the ready line to transition on a command/write here, so we oblige */
		pia_1_ca2_w(0, 1);
		pia_1_ca2_w(0, 0);
	}
	
	/* remember the state */
	squawkntalk_tms_strobes = data;
}

static void squawkntalk_irq(int state)
{
	cpu_set_irq_line(squawkntalk_sound_cpu, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void squawkntalk_delayed_data_w(int param)
{
	pia_0_porta_w(0, ~param & 0x0f);
	pia_0_cb1_w(0, ~param & 0x10);
}


/********* external interfaces ***********/
void squawkntalk_data_w(int offset, int data)
{
	timer_set(TIME_NOW, data, squawkntalk_delayed_data_w);
}

void squawkntalk_reset_w(int state)
{
	/* going high halts the CPU */
	if (state && cpu_getstatus(squawkntalk_sound_cpu))
		cpu_halt(squawkntalk_sound_cpu, 0);
	
	/* going low resets and reactivates the CPU */
	else if (!state && !cpu_getstatus(squawkntalk_sound_cpu))
	{
		cpu_reset(squawkntalk_sound_cpu);
		cpu_halt(squawkntalk_sound_cpu, 1);
	}
}


/********* sound interfaces ***********/
struct TMS5220interface squawkntalk_tms5220_interface =
{
	640000,
	MIXER(60,MIXER_PAN_LEFT),
	0
};


/********* memory interfaces ***********/
struct MemoryReadAddress squawkntalk_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0080, 0x0083, pia_0_r },
	{ 0x0090, 0x0093, pia_1_r },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

struct MemoryWriteAddress squawkntalk_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x0080, 0x0083, pia_0_w },
	{ 0x0090, 0x0093, pia_1_w },
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


/********* PIA interfaces ***********/
struct pia6821_interface squawkntalk_pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ squawkntalk_porta1_w, 0, 0, 0,
	/*irqs   : A/B             */ squawkntalk_irq, squawkntalk_irq
};

struct pia6821_interface squawkntalk_pia1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ squawkntalk_porta2_w, squawkntalk_portb2_w, 0, 0,
	/*irqs   : A/B             */ squawkntalk_irq, squawkntalk_irq
};



/*************************************
 *
 *	MCR Advanced Audio communications
 *
 *	MC6809, 1 PIA, YM2151, HC55536 CVSD
 *
 *************************************/

/********* internal interfaces ***********/
static void advaudio_ym2151_irq(int state)
{
	pia_0_ca1_w(0, !state);
}

static void advaudio_irqa(int state)
{
	cpu_set_irq_line(advaudio_sound_cpu, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void advaudio_irqb(int state)
{
	cpu_set_nmi_line(advaudio_sound_cpu, state ? ASSERT_LINE : CLEAR_LINE);
}

static void advaudio_bank_select_w(int offset, int data)
{
	UINT8 *RAM = Machine->memory_region[Machine->drv->cpu[advaudio_sound_cpu].memory_region];
	int bank = data & 3;
	int half = (data >> 2) & 1;
	cpu_setbank(1, &RAM[0x10000 + (bank * 0x10000) + (half * 0x8000)]);
}

static void advaudio_delayed_data_w(int param)
{
	pia_0_portb_w(0, param & 0xff);
	pia_0_cb1_w(0, param & 0x100);
	pia_0_cb2_w(0, param & 0x200);
}


/********* external interfaces ***********/
void advaudio_data_w(int offset, int data)
{
	timer_set(TIME_NOW, data, advaudio_delayed_data_w);
}

void advaudio_reset_w(int state)
{
	/* going high halts the CPU */
	if (state && cpu_getstatus(advaudio_sound_cpu))
		cpu_halt(advaudio_sound_cpu, 0);
	
	/* going low resets and reactivates the CPU */
	else if (!state && !cpu_getstatus(advaudio_sound_cpu))
	{
		cpu_reset(advaudio_sound_cpu);
		cpu_halt(advaudio_sound_cpu, 1);

		/* IMPORTANT: the bank must be reset here! */
		advaudio_bank_select_w(0, 0);
	}
}


/********* sound interfaces ***********/
struct YM2151interface advaudio_ym2151_interface =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
	{ advaudio_ym2151_irq }
};

struct DACinterface advaudio_dac_interface =
{
	1,
	{ 50 }
};

struct CVSDinterface advaudio_cvsd_interface =
{
	1,			/* 1 chip */
	{ 40 }
};


/********* memory interfaces ***********/
struct MemoryReadAddress advaudio_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x4000, 0x4003, pia_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
	{ -1 }	/* end of table */
};

struct MemoryWriteAddress advaudio_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x4000, 0x4003, pia_0_w },
	{ 0x6000, 0x6000, MWA_NOP /*CVSD_dig_and_clk_w*/},
	{ 0x6800, 0x6800, MWA_NOP /*CVSD_clock_w*/},
	{ 0x7800, 0x7800, advaudio_bank_select_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


/********* PIA interfaces ***********/
struct pia6821_interface advaudio_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_data_w, 0, 0, 0,
	/*irqs   : A/B             */ advaudio_irqa, advaudio_irqb
};
