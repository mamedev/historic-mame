/***************************************************************************

Atari Football machine

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int CTRLD;
static int sign_x_1, sign_y_1;
static int sign_x_2, sign_y_2;
static int sign_x_3, sign_y_3;
static int sign_x_4, sign_y_4;

void atarifb_out1_w(int offset, int data)
{
	CTRLD = data;
	/* we also need to handle the whistle, hit, and kicker sound lines */
//	if (errorlog) fprintf (errorlog, "out1_w: %02x\n", data);
}

void atarifb4_out1_w(int offset, int data)
{
	CTRLD = data;
	coin_counter_w (0, data & 0x80);
	/* we also need to handle the whistle, hit, and kicker sound lines */
//	if (errorlog) fprintf (errorlog, "out1_w: %02x\n", data);
}


int atarifb_in0_r(int offset)
{
	if ((CTRLD & 0x20)==0x00)
	{
		int val;

		val = (sign_y_2 >> 7) |
			  (sign_x_2 >> 6) |
			  (sign_y_1 >> 5) |
			  (sign_x_1 >> 4) |
			  input_port_0_r(offset);
		return val;
	}
	else
	{
		static int counter_x;
		static int counter_y;
		int delta_x, delta_y;

		/* Read player 1 trackball */
		delta_x = readinputport(3);
		if (delta_x != 0)
		{
			counter_x = (counter_x + delta_x) & 0x0f;
			sign_x_1 = delta_x & 0x80;
		}

		delta_y = readinputport(2);
		if (delta_y != 0)
		{
			counter_y = (counter_y + delta_y) & 0x0f;
			sign_y_1 = delta_y & 0x80;
		}

		return ((counter_y << 4) | counter_x);
	}
}


int atarifb_in2_r(int offset)
{
	if ((CTRLD & 0x20)==0x00)
	{
		return input_port_1_r(offset);
	}
	else
	{
		static int counter_x;
		static int counter_y;
		int delta_x, delta_y;

		/* Read player 2 trackball */
		delta_x = readinputport(5);
		if (delta_x != 0)
		{
			counter_x = (counter_x + delta_x) & 0x0f;
			sign_x_2 = delta_x & 0x80;
		}

		delta_y = readinputport(4);
		if (delta_y != 0)
		{
			counter_y = (counter_y + delta_y) & 0x0f;
			sign_y_2 = delta_y & 0x80;
		}

		return ((counter_y << 4) | counter_x);
	}
}

int atarifb4_in0_r(int offset)
{
	/* LD1 and LD2 low, return sign bits */
	if ((CTRLD & 0x60)==0x00)
	{
		int val;

		/* TODO: double-check these, probably wrong */
		val = (sign_y_2 >> 7) |
			  (sign_x_2 >> 6) |
			  (sign_y_1 >> 5) |
			  (sign_x_1 >> 4) |
			  (sign_y_4 >> 3) |
			  (sign_x_4 >> 2) |
			  (sign_y_3 >> 1) |
			  (sign_x_3 >> 0);
		return val;
	}
	else if ((CTRLD & 0x60) == 0x60)
	/* LD1 and LD2 both high, return Team 1 left player */
	{
		static int counter_x;
		static int counter_y;
		int delta_x, delta_y;

		/* Read player 1 trackball */
		delta_x = readinputport(4);
		if (delta_x != 0)
		{
			counter_x = (counter_x + delta_x) & 0x0f;
			sign_x_1 = delta_x & 0x80;
		}

		delta_y = readinputport(3);
		if (delta_y != 0)
		{
			counter_y = (counter_y + delta_y) & 0x0f;
			sign_y_1 = delta_y & 0x80;
		}

		return ((counter_y << 4) | counter_x);
	}
	else if ((CTRLD & 0x60) == 0x20)
	/* LD1 high, LD2 low, return Team 1 right player */
	{
		static int counter_x;
		static int counter_y;
		int delta_x, delta_y;

		/* Read player 3 trackball */
		delta_x = readinputport(8);
		if (delta_x != 0)
		{
			counter_x = (counter_x + delta_x) & 0x0f;
			sign_x_3 = delta_x & 0x80;
		}

		delta_y = readinputport(7);
		if (delta_y != 0)
		{
			counter_y = (counter_y + delta_y) & 0x0f;
			sign_y_3 = delta_y & 0x80;
		}

		return ((counter_y << 4) | counter_x);
	}

	else return 0;
}


int atarifb4_in2_r(int offset)
{
	if ((CTRLD & 0x20)==0x00)
	{
		return input_port_2_r(offset);
	}
	else if ((CTRLD & 0x60) == 0x60)
	/* LD1 and LD2 both high, return Team 2 left player */
	{
		static int counter_x;
		static int counter_y;
		int delta_x, delta_y;

		/* Read player 2 trackball */
		delta_x = readinputport(6);
		if (delta_x != 0)
		{
			counter_x = (counter_x + delta_x) & 0x0f;
			sign_x_2 = delta_x & 0x80;
		}

		delta_y = readinputport(5);
		if (delta_y != 0)
		{
			counter_y = (counter_y + delta_y) & 0x0f;
			sign_y_2 = delta_y & 0x80;
		}

		return ((counter_y << 4) | counter_x);
	}
	else if ((CTRLD & 0x60) == 0x20)
	/* LD1 high, LD2 low, return Team 2 right player */
	{
		static int counter_x;
		static int counter_y;
		int delta_x, delta_y;

		/* Read player 4 trackball */
		delta_x = readinputport(10);
		if (delta_x != 0)
		{
			counter_x = (counter_x + delta_x) & 0x0f;
			sign_x_4 = delta_x & 0x80;
		}

		delta_y = readinputport(9);
		if (delta_y != 0)
		{
			counter_y = (counter_y + delta_y) & 0x0f;
			sign_y_4 = delta_y & 0x80;
		}

		return ((counter_y << 4) | counter_x);
	}

	else return 0;
}


