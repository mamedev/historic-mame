/*************************************************************************

  atarisys2' machine hardware

*************************************************************************/

#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/t11/t11.h"

extern void atarisys2_scanline_update(int scanline);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *atarisys2_interrupt_enable;
unsigned char *atarisys2_bankselect;

int atarisys2_pedal_count;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int has_tms5220;
static int tms5220_data;
static int tms5220_data_strobe;

static int last_sound_reset;
static int which_adc;

static int v32_state;
static int vblank_state;
static int p2portwr_state;
static int p2portrd_state;

static int pedal_value[3];



/*************************************
 *
 *		Interrupt updating
 *
 *************************************/

static void update_interrupts(int vblank, int sound)
{
	if (vblank_state)
		cpu_set_irq_line(0, 3, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 3, CLEAR_LINE);

	if (v32_state)
		cpu_set_irq_line(0, 2, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 2, CLEAR_LINE);

	if (p2portwr_state)
		cpu_set_irq_line(0, 1, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 1, CLEAR_LINE);

	if (p2portrd_state)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}



/*************************************
 *
 *		Every 8-scanline update
 *
 *************************************/

static void scanline_update(int scanline)
{
	if (scanline < Machine->drv->screen_height)
	{
		/* update the display list */
		atarisys2_scanline_update(scanline);

		/* generate the 32V interrupt (IRQ 2) */
		if ((scanline % 64) == 0)
		{
			v32_state = (READ_WORD(&atarisys2_interrupt_enable[0]) & 4) != 0;
			atarigen_update_interrupts();
		}
	}
}



/*************************************
 *
 *		Initialization
 *
 *************************************/

void atarisys2_init_machine(void)
{
	int ch, i;

	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_init(update_interrupts, scanline_update);

	last_sound_reset = 0;
	tms5220_data_strobe = 1;

	v32_state = 0;
	vblank_state = 0;
	p2portwr_state = 0;
	p2portrd_state = 0;

	/* determine which sound hardware is installed */
	has_tms5220 = 0;
	for (i = 0; i < MAX_SOUND; i++)
		if (Machine->drv->sound[i].sound_type == SOUND_TMS5220)
			has_tms5220 = 1;

	which_adc = 0;
	pedal_value[0] = pedal_value[1] = pedal_value[2] = 0;
}



/*************************************
 *
 *		Interrupt handlers
 *
 *************************************/

int atarisys2_interrupt(void)
{
    int i;

	/* update the pedals once per frame */
    for (i = 0; i < atarisys2_pedal_count; i++)
	{
		if (readinputport(3 + i) & 0x80)
		{
			pedal_value[i] += 64;
			if (pedal_value[i] > 0xff) pedal_value[i] = 0xff;
		}
		else
		{
			pedal_value[i] -= 64;
			if (pedal_value[i] < 0) pedal_value[i] = 0;
		}
	}

	/* clock the VBLANK through */
	vblank_state = (READ_WORD(&atarisys2_interrupt_enable[0]) & 8) != 0;
	atarigen_update_interrupts();

	/* let the atarigen system think it's generating VBLANK so we get our scanlines */
	return atarigen_vblank_gen();
}


void atarisys2_interrupt_ack_w(int offset, int data)
{
	/* reset sound IRQ */
	if (offset == 0x00)
	{
		p2portrd_state = 0;
		atarigen_update_interrupts();
	}

	/* reset sound CPU */
	else if (offset == 0x20)
	{
		if (last_sound_reset == 0 && (data & 1))
			atarigen_sound_reset_w(0, 0);
		last_sound_reset = data & 1;
	}

	/* reset 32V IRQ */
	else if (offset == 0x40)
	{
		v32_state = 0;
		atarigen_update_interrupts();
	}

	/* reset VBLANK IRQ */
	else if (offset == 0x60)
	{
		vblank_state = 0;
		atarigen_update_interrupts();
	}
}



/*************************************
 *
 *		Bank selection.
 *
 *************************************/

void atarisys2_bankselect_w(int offset, int data)
{
	static int bankoffset[64] =
	{
		0x28000, 0x20000, 0x18000, 0x10000,
		0x2a000, 0x22000, 0x1a000, 0x12000,
		0x2c000, 0x24000, 0x1c000, 0x14000,
		0x2e000, 0x26000, 0x1e000, 0x16000,
		0x48000, 0x40000, 0x38000, 0x30000,
		0x4a000, 0x42000, 0x3a000, 0x32000,
		0x4c000, 0x44000, 0x3c000, 0x34000,
		0x4e000, 0x46000, 0x3e000, 0x36000,
		0x68000, 0x60000, 0x58000, 0x50000,
		0x6a000, 0x62000, 0x5a000, 0x52000,
		0x6c000, 0x64000, 0x5c000, 0x54000,
		0x6e000, 0x66000, 0x5e000, 0x56000,
		0x88000, 0x80000, 0x78000, 0x70000,
		0x8a000, 0x82000, 0x7a000, 0x72000,
		0x8c000, 0x84000, 0x7c000, 0x74000,
		0x8e000, 0x86000, 0x7e000, 0x76000
	};

	int oldword = READ_WORD(&atarisys2_bankselect[offset]);
	int newword = COMBINE_WORD(oldword, data);
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char *base = &RAM[bankoffset[(newword >> 10) & 0x3f]];

	WRITE_WORD(&atarisys2_bankselect[offset], newword);
	if (offset == 0)
	{
		cpu_setbank(1, base);
		t11_SetBank(0x4000, base);
	}
	else if (offset == 2)
	{
		cpu_setbank(2, base);
		t11_SetBank(0x6000, base);
	}
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int atarisys2_switch_r(int offset)
{
	int result = input_port_1_r(offset) | (input_port_2_r(offset) << 8);

	if (atarigen_cpu_to_sound_ready) result ^= 0x20;
	if (atarigen_sound_to_cpu_ready) result ^= 0x10;

	return result;
}


int atarisys2_6502_switch_r(int offset)
{
	int result = input_port_0_r(offset);

	if (atarigen_cpu_to_sound_ready) result ^= 0x01;
	if (atarigen_sound_to_cpu_ready) result ^= 0x02;
	if (!has_tms5220 || tms5220_ready_r()) result ^= 0x04;
	if (!(input_port_2_r(offset) & 0x80)) result ^= 0x10;

	return result;
}


void atarisys2_6502_switch_w(int offset, int data)
{
}



/*************************************
 *
 *		Controls read
 *
 *************************************/

void atarisys2_adc_strobe_w(int offset, int data)
{
	which_adc = (offset / 2) & 3;
}


int atarisys2_adc_r(int offset)
{
    if (which_adc == 1 && atarisys2_pedal_count == 1)   /* APB */
        return ~pedal_value[0];

	if (which_adc < atarisys2_pedal_count)
		return (~pedal_value[which_adc]);
	return readinputport(3 + which_adc) | 0xff00;
}


int atarisys2_leta_r(int offset)
{
	return readinputport(7 + (offset & 3));
}



/*************************************
 *
 *		Global sound control
 *
 *************************************/

void atarisys2_mixer_w(int offset, int data)
{
	atarigen_set_ym2151_vol((data & 7) * 100 / 7);
	atarigen_set_pokey_vol(((data >> 3) & 3) * 100 / 3);
	atarigen_set_tms5220_vol(((data >> 5) & 7) * 100 / 7);
}


void atarisys2_sound_enable_w(int offset, int data)
{
}


int atarisys2_sound_r(int offset)
{
	/* clear the p2portwr state on a p1portrd */
	p2portwr_state = 0;
	atarigen_update_interrupts();

	/* handle it normally otherwise */
	return atarigen_sound_r(offset);
}


void atarisys2_6502_sound_w(int offset, int data)
{
	/* clock the state through */
	p2portwr_state = (READ_WORD(&atarisys2_interrupt_enable[0]) & 2) != 0;
	atarigen_update_interrupts();

	/* handle it normally otherwise */
	atarigen_6502_sound_w(offset, data);
}


int atarisys2_6502_sound_r(int offset)
{
	/* clock the state through */
	p2portrd_state = (READ_WORD(&atarisys2_interrupt_enable[0]) & 1) != 0;
	atarigen_update_interrupts();

	/* handle it normally otherwise */
	return atarigen_6502_sound_r(offset);
}


/*************************************
 *
 *		Speech chip
 *
 *************************************/

void atarisys2_tms5220_w(int offset, int data)
{
	tms5220_data = data;
}


void atarisys2_tms5220_strobe_w(int offset, int data)
{
	if (!(offset & 1) && tms5220_data_strobe)
		if (has_tms5220)
			tms5220_data_w(0, tms5220_data);
	tms5220_data_strobe = offset & 1;
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/



