/*************************************************************************

  Rainbow Islands C-Chip Protection

  2000-Nov-30 re-written by SJ

    - removed a hack that queried main RAM
    - removed toggling between CROM base $0000 and $4950
    - added secret room fix
    - added alternate GOAL IN fix

  2001-Mar-22

    - fixed crash in level 40 (new bug, oops)
    - removed a hack for offsets 0x000 and 0x100
    - added coin counters and lock-out

*************************************************************************/

#include "driver.h"
#include "state.h"

#ifndef RAINBOW_CROM_BASE
#define RAINBOW_CROM_BASE 0x4950
#endif

/*************************************
 *
 *		Interrupt handler
 *
 *************************************/

int rainbow_interrupt(void)
{
	return 4;  /* Interrupt vector 4 */
}

/*************************************
 *
 * Rainbow C-Chip, Protection
 *
 *************************************/

static UINT8 cval[20];
static UINT8 cc_port;

static int current_round = 0;
static int current_bank = 0;

/*************************************
 *
 * Helper for standard c-chip reading
 *
 *************************************/

static UINT8 rainbow_helper(int round, int bank, int offset)
{
	int cchip_index = RAINBOW_CROM_BASE + 2 * (8 * round + bank);

	UINT8* CROM = memory_region(REGION_USER1);

	int address1 = (CROM[cchip_index + 0] << 8) | CROM[cchip_index + 1];
	int address2 = (CROM[cchip_index + 2] << 8) | CROM[cchip_index + 3];

	/* out-of-range reads happen in round 40, returning 0 is vital */

	return (offset >= address2 - address1) ? 0 : CROM[address1 + offset];
}

/*************************************
 *
 * Writes to C-Chip - Important Bits
 *
 *************************************/

WRITE16_HANDLER( rainbow_c_chip_w )
{
	if (offset == 0x600)
	{
		current_bank = data;
	}

	if (current_bank == 0 && offset == 0x00d)
	{
		current_round = data;
	}

	if (current_bank == 0 && offset == 0x008)
	{
		cc_port = data;

		coin_lockout_w(1, data & 0x80);
		coin_lockout_w(0, data & 0x40);
		coin_counter_w(1, data & 0x20);
		coin_counter_w(0, data & 0x10);
	}

	if (current_bank == 1 && offset == 0x141)
	{
		if (data >= 40)
		{
			/* Special processing for secret rooms */

			UINT32 level_data =
				(rainbow_helper(40, 2, 0x142) << 0x18) |
				(rainbow_helper(40, 2, 0x143) << 0x10) |
				(rainbow_helper(40, 2, 0x144) << 0x08) |
				(rainbow_helper(40, 2, 0x145) << 0x00);

			level_data += (data - 40) * 0xd8;

			cval[0x00] = (UINT8) (level_data >> 0x18);
			cval[0x01] = (UINT8) (level_data >> 0x10);
			cval[0x02] = (UINT8) (level_data >> 0x08);
			cval[0x03] = (UINT8) (level_data >> 0x00);
			cval[0x04] = 0;
			cval[0x05] = 0;
			cval[0x06] = 1;	/* must not be 0 to make exit appear */
			cval[0x07] = 1;
		}
		else
		{
			int i;

			for (i = 0; i < 8; i++)
			{
				cval[i] = rainbow_helper(data, 1, 0x142 + i);
			}
		}
	}

	if (offset == 0x149)
	{
		if (data == 1)
		{

#if defined(RAINBOW_ALTERNATE_GOALIN)

			int i;

			for (i = 0; i < 10; i++)
			{
				cval[0x08 + i] = rainbow_helper(25, 1, 0x14a + i);
			}
#else
			/* this data was taken from a bootleg set */

			cval[0x08] = 0x00; /* G placement */
			cval[0x09] = 0x00;
			cval[0x0a] = 0x10; /* O placement */
			cval[0x0b] = 0x10;
			cval[0x0c] = 0x20; /* A placement */
			cval[0x0d] = 0x20;
			cval[0x0e] = 0x30; /* L placement */
			cval[0x0f] = 0x38;
			cval[0x10] = 0x40; /* I placement */
			cval[0x11] = 0x50;
			cval[0x12] = 0x50; /* N placement */
			cval[0x13] = 0x60;

#endif /* defined(RAINBOW_ALTERNATE_GOALIN) */

			return;
		}
	}
}

/*************************************
 *
 * Reads from C-Chip
 *
 *************************************/

READ16_HANDLER( rainbow_c_chip_r )
{
	/* C-chip identification */

	if (offset == 0x401)
	{
		return 0x01;
	}

	/* Check for input ports */

	if (current_bank == 0)
	{
		switch (offset)
		{
			case 0x003: return input_port_2_word_r(offset,mem_mask);
			case 0x004: return input_port_3_word_r(offset,mem_mask);
			case 0x005: return input_port_4_word_r(offset,mem_mask);
			case 0x006: return input_port_5_word_r(offset,mem_mask);
			case 0x008: return cc_port;
		}
	}

	/* Further non-standard offsets */

	if (current_bank == 1)
	{
		if (offset >= 0x142 && offset < 0x156)
		{
			return cval[offset - 0x142];
		}
	}

	/* Standard retrieval */

	return rainbow_helper(current_round, current_bank, offset);
}

/*************************************
	Save state initialization
**************************************/

void rainbow_cchip_init(void)
{
	state_save_register_int  ("cchip_bank", 0, "cchip", &current_bank);
	state_save_register_int  ("cchip_round", 0, "cchip", &current_round);
	state_save_register_UINT8("cchip_port", 0, "cchip", &cc_port, 1);
	state_save_register_UINT8("cchip_values", 0, "cchip", cval, 20);
}

