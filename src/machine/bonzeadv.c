/*************************************************************************

Bonze Adventure C-Chip Protection
=================================

Based on RAINE. Data improved by Frotz with help from Clockwork.

*************************************************************************/

#include "driver.h"

static int current_round = 0;
static int current_bank = 0;

static UINT8 cval[26];
static UINT8 cc_port;

static UINT16 CROM[9][13] =
{
/*	  map start       player start    player y-range  player x-range  map y-range     map x-range     time   */
	{ 0x0000, 0x0018, 0x0020, 0x0080, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0000, 0x0108, 0x0000, 0x0CA0, 0x5000 },
	{ 0x0000, 0x0218, 0x0030, 0x0030, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0108, 0x0218, 0x0000, 0x0CC0, 0x5000 },
	{ 0x0000, 0x0518, 0x0020, 0x00D0, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x02F8, 0x0518, 0x0000, 0x0EF0, 0x5000 },
	{ 0x09E0, 0x0610, 0x007C, 0x00C8, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0610, 0x06E8, 0x0000, 0x0A30, 0x5000 },
	{ 0x03E0, 0x0708, 0x00A0, 0x0050, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0708, 0x0708, 0x03E0, 0x1080, 0x5000 },
	{ 0x1280, 0x0808, 0x00A0, 0x00D0, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0000, 0x0818, 0x1280, 0x1780, 0x5000 },
	{ 0x11C0, 0x0908, 0x0100, 0x0098, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0908, 0x0908, 0x0000, 0x11C0, 0x5000 },
	{ 0x0000, 0x0808, 0x0030, 0x00D0, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0808, 0x0808, 0x0000, 0x1FFF, 0x5000 },
	{ 0x06FC, 0x0808, 0x0040, 0x00D0, 0x0030, 0x00D0, 0x0060, 0x00C0, 0x0808, 0x0808, 0x06FC, 0x06FC, 0x5000 }
};

/*************************************
 *
 * Writes to C-Chip - Important Bits
 *
 *************************************/

WRITE16_HANDLER( bonzeadv_c_chip_w )
{
	if (offset == 0x600)
	{
		current_bank = data;
	}

	if (current_bank == 0)
	{
		if (offset == 0x008)
		{
			cc_port = data;

			coin_lockout_w(1, data & 0x80);
			coin_lockout_w(0, data & 0x40);
			coin_counter_w(1, data & 0x20);
			coin_counter_w(0, data & 0x10);
		}

		if (offset == 0x00F && data != 0x00)
		{
			int i;

			for (i = 0; i < 13; i++)
			{
				UINT16 v = CROM[current_round][i];

				cval[2 * i + 0] = v & 0xff;
				cval[2 * i + 1] = v >> 8;
			}
		}

		if (offset == 0x010)
		{
			current_round = data;
		}

		if (offset >= 0x011 && offset <= 0x02A)
		{
			cval[offset - 0x11] = data;
		}
	}
}

/*************************************
 *
 * Reads from C-Chip
 *
 *************************************/

READ16_HANDLER( bonzeadv_c_chip_r )
{
	/* C-chip ID */

	if (offset == 0x401)
	{
		return 0x01;
	}

	if (current_bank == 0)
	{
		switch (offset)
		{
		case 0x003: return input_port_2_word_r(offset, mem_mask);
		case 0x004: return input_port_3_word_r(offset, mem_mask);
		case 0x005: return input_port_4_word_r(offset, mem_mask);
		case 0x006: return input_port_5_word_r(offset, mem_mask);
		case 0x008: return cc_port;
		}

		if (offset == 0x00E)
		{
			return 0x00;	// non-zero signals error
		}

		if (offset >= 0x011 && offset <= 0x2A)
		{
			return cval[offset - 0x11];
		}
	}

	return 0;
}
