#include "driver.h"

#define MASTER_CLOCK_BASE 3600000


struct YM3812interface *intf;

static int pending_register;
static void *timer1;
static void *timer2;
static unsigned char status_register;
static unsigned char timer_register;
static unsigned int timer1_val;
static unsigned int timer2_val;
static double timer_scale;


void timer1_callback (int param)
{
	if (!(timer_register & 0x40))
	{
		if (intf->handler) (*intf->handler)();

		/* set the IRQ and timer 1 signal bits */
		status_register |= 0x80;
		status_register |= 0x40;
	}

	/* next! */
	timer1 = timer_set (TIME_IN_USEC (timer1_val * 80) * timer_scale, 0, timer1_callback);
}

void timer2_callback (int param)
{
	if (!(timer_register & 0x20))
	{
		if (intf->handler) (*intf->handler)();

		/* set the IRQ and timer 2 signal bits */
		status_register |= 0x80;
		status_register |= 0x20;
	}

	/* next! */
	timer2 = timer_set (TIME_IN_USEC (timer2_val * 320) * timer_scale, 0, timer2_callback);
}



int YM3812_status_port_0_r(int offset)
{
	/* mask out the timer 1 and 2 signal bits as requested by the timer register */
	return status_register & ~(timer_register & 0x60);
}

void YM3812_control_port_0_w(int offset,int data)
{
	pending_register = data;

	/* pass through all non-timer registers */
	if (data < 2 || data > 4)
		osd_ym3812_control(data);
}

void YM3812_write_port_0_w(int offset,int data)
{
	if (pending_register < 2 || pending_register > 4)
		osd_ym3812_write(data);
	else
	{
		switch (pending_register)
		{
			case 2:
				timer1_val = 256 - data;
				break;

			case 3:
				timer2_val = 256 - data;
				break;

			case 4:
				/* bit 7 means reset the IRQ signal and status bits, and ignore all the other bits */
				if (data & 0x80)
				{
					status_register = 0;
				}
				else
				{
					/* set the new timer register */
					timer_register = data;

					/*  bit 0 starts/stops timer 1 */
					if (data & 0x01)
					{
						if (!timer1)
							timer1 = timer_set (TIME_IN_USEC (timer1_val * 80) * timer_scale, 0, timer1_callback);
					}
					else if (timer1)
					{
						timer_remove (timer1);
						timer1 = 0;
					}

					/*  bit 1 starts/stops timer 2 */
					if (data & 0x02)
					{
						if (!timer2)
							timer2 = timer_set (TIME_IN_USEC (timer2_val * 320) * timer_scale, 0, timer2_callback);
					}
					else if (timer2)
					{
						timer_remove (timer2);
						timer2 = 0;
					}

					/* bits 5 & 6 clear and mask the appropriate bit in the status register */
					status_register &= ~(data & 0x60);
				}
				break;
		}
	}
	pending_register = -1;
}


int YM3812_sh_start(struct YM3812interface *interface)
{
	pending_register = -1;
	timer1 = timer2 = 0;
	status_register = 0x80;
	timer_register = 0;
	timer1_val = timer2_val = 256;
	intf = interface;

	timer_scale = (double)MASTER_CLOCK_BASE / (double)interface->clock;

	return 0;
}

void YM3812_sh_stop(void)
{
}

void YM3812_sh_update(void)
{
}
