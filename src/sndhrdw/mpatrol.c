#include "driver.h"
#include "m6808/m6808.h"


static int port1;
static int port2;
static int ddr1;
static int ddr2;


/* sound commands from the main CPU come through here; we generate an IRQ and/or sample */
void mpatrol_sound_cmd_w (int offset, int data)
{
	/* generate IRQ on offset 0 only; should we also do it on offset 1? */
	if (offset == 0)
	{
		soundlatch_w (0, data);
		cpu_cause_interrupt (1, M6808_INT_IRQ);
	}
	else
		soundlatch2_w (0, data);
}


void mpatrol_io_w(int offset, int data)
{
	switch (offset)
	{
		/* port 1 DDR */
		case 0:
			ddr1 = data;
			break;

		/* port 2 DDR */
		case 1:
			ddr2 = data;
			break;

		/* port 1 */
		case 2:
			data = (port1 & ~ddr1) | (data & ddr1);
			port1 = data;
			break;

		/* port 2 */
		case 3:
			data = (port2 & ~ddr2) | (data & ddr2);

			/* write latch */
			if ((port2 & 0x01) && !(data & 0x01))
			{
				/* control or data port? */
				if (port2 & 0x04)
				{
					/* PSG 0 or 1? */
					if (port2 & 0x10)
						AY8910_control_port_0_w (0, port1);
					else if (port2 & 0x08)
						AY8910_control_port_1_w (0, port1);
				}
				else
				{
					/* PSG 0 or 1? */
					if (port2 & 0x10)
						AY8910_write_port_0_w (0, port1);
					else if (port2 & 0x08)
						AY8910_write_port_1_w (0, port1);
				}
			}
			port2 = data;
			break;
	}
}


int mpatrol_io_r(int offset)
{
	switch (offset)
	{
		/* port 1 DDR */
		case 0:
			return ddr1;

		/* port 2 DDR */
		case 1:
			return ddr2;

		/* port 1 */
		case 2:

			/* input 0 or 1? */
			if (port2 & 0x10)
				return (soundlatch2_r(0) & ~ddr1) | (port1 & ddr1);
			else if (port2 & 0x08)
				return (soundlatch_r(0) & ~ddr1) | (port1 & ddr1);
			return (port1 & ddr1);

		/* port 2 */
		case 3:
			return (port2 & ddr2);
	}

	return 0;
}


void mpatrol_adpcm_reset_w (int offset, int data)
{
	MSM5205_reset_w (0, data & 1);
	MSM5205_reset_w (1, data & 2);
}


void mpatrol_adpcm_int (int data)
{
	cpu_cause_interrupt (1, M6808_INT_NMI);
}
