/*************************************************************************

  Rainbow Islands C-Chip Protection


  2000-Nov-25 changes by SJ

    - removed all code that toggled between ROM addresses $0000
	  and $4950 (see rainbow_chip_c)
    - added a fix for the secret rooms (hopefully correct)
    - removed a hack that queried a location in main RAM for the
      current round number
    - simplified the code a bit

*************************************************************************/

#include "driver.h"

data16_t *rainbow_mainram;

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

static int bank = 0;
static int round_A = 0;
static int round_B = 0;

/*************************************
 *
 * Writes to C-Chip - Important Bits
 *
 *************************************/

WRITE16_HANDLER( rainbow_c_chip_w )
{
	switch (offset)
	{
		case 0x00d:
			round_A = data;
			break;

		case 0x141:
			round_B = data;
			break;

		case 0x600:
			bank = data;
			break;
	}
}

/*************************************
 *
 * Reads from C-Chip
 *
 *************************************/

READ16_HANDLER( rainbow_c_chip_r )
{
	int address;

	unsigned char *CROM;

	/* Check for input ports */

	if (bank == 0)
	{
		switch (offset)
		{
			case 0x003: return input_port_2_word_r(offset);
			case 0x004: return input_port_3_word_r(offset);
			case 0x005: return input_port_4_word_r(offset);
			case 0x006: return input_port_5_word_r(offset);
		}
	}

	/* Further non-standard offsets. Movement of GOAL IN letters
	   is controlled by offsets $14a to $155; this data was taken
	   from a look-up table in a bootleg set. Note that there is
	   similar data hidden in the c-chip ROM at round 25, but the
	   movement produced by the bootleg data looks more correct. */

	switch (offset)
	{
		case 0x000: return 0xff;	/* data ready? */
		case 0x100: return 0xff;	/* data ready? */
		case 0x14a: return 0x00;	/* 'G' right */
		case 0x14b: return 0x00;	/* 'G' below */
		case 0x14c: return 0x10;	/* 'O' right */
		case 0x14d: return 0x10;	/* 'O' below */
		case 0x14e: return 0x20;	/* 'A' right */
		case 0x14f: return 0x20;	/* 'A' below */
		case 0x150: return 0x30;	/* 'L' right */
		case 0x151: return 0x38;	/* 'L' below */
		case 0x152: return 0x40;	/* 'I' right */
		case 0x153: return 0x50;	/* 'I' below */
		case 0x154: return 0x50;	/* 'N' right */
		case 0x155: return 0x60;	/* 'N' below */
		case 0x401: return 0x01;	/* c-chip check */
	}

	/* Data retrieval from c-chip ROM. The ROM is divided into
	   two parts, starting at base addresses $0000 and $4950
	   respectively. Only the second part is used. If you chose
	   the first part instead, the game would play w/o enemies. */

	address = 0x4950;

	if (round_B >= 40 && offset < 0x148)
	{
		/* This should fix the secret rooms. Rainbow Islands has
		   7 regular and 3 hidden islands with 4 rounds each
		   which are numbered from 0 to 39 internally. Numbers
		   from 40 onward indicate a secret room and require
		   special processing. For some odd reason we need to
		   exclude offset $148, because otherwise there would be
		   no way out of the secret room. */

		address += 16 * 40 + 4 * bank;
	}
	else
	{
		address += 16 * round_A + 2 * bank;
	}

	CROM = memory_region(REGION_USER1);

	address = (CROM[address] << 8) + CROM[address + 1];

	return CROM[address + offset];
}
