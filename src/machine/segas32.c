/* Sega System 32 Protection related functions */

#include "driver.h"

extern data16_t* segas32_workram;
extern data16_t* segas32_protram;

/******************************************************************************
 ******************************************************************************
  Golden Axe 2 (Revenge of Death Adder)
 ******************************************************************************
 ******************************************************************************/

READ16_HANDLER(ga2_sprite_protection_r)
{
	static unsigned int prot[16] =
	{
		0x0a, 0,
		0xc5, 0,
		0x11, 0,
		0x11, 0,
		0x18, 0,
		0x18, 0,
		0x1f, 0,
		0xc6, 0,
	};

	return prot[offset];
}

READ16_HANDLER(ga2_wakeup_protection_r)
{
	static const char *prot =
		"wake up! GOLDEN AXE The Revenge of Death-Adder! ";
	return prot[offset];
}


/******************************************************************************
 ******************************************************************************
  Sonic Arcade protection
 ******************************************************************************
 ******************************************************************************/


// This code duplicates the actions of the protection device used in SegaSonic
// arcade revision C, allowing the game to run correctly.
#define CLEARED_LEVELS			0xE5C4
#define CURRENT_LEVEL			0xF06E
#define CURRENT_LEVEL_STATUS		0xF0BC
#define LEVEL_ORDER_ARRAY		0x263A

WRITE16_HANDLER(sonic_level_load_protection)
{
	unsigned short level;
//Perform write
	segas32_workram[CLEARED_LEVELS / 2] = (data & ~mem_mask) | (segas32_workram[CLEARED_LEVELS / 2] & mem_mask);

//Refresh current level
		if (segas32_workram[CLEARED_LEVELS / 2] == 0)
		{
			level = 0x0007;
		}
		else
		{
			level =  *((memory_region(REGION_CPU1) + LEVEL_ORDER_ARRAY) + (segas32_workram[CLEARED_LEVELS / 2] * 2) - 1);
			level |= *((memory_region(REGION_CPU1) + LEVEL_ORDER_ARRAY) + (segas32_workram[CLEARED_LEVELS / 2] * 2) - 2) << 8;
		}
		segas32_workram[CURRENT_LEVEL / 2] = level;

//Reset level status
		segas32_workram[CURRENT_LEVEL_STATUS / 2] = 0x0000;
		segas32_workram[(CURRENT_LEVEL_STATUS + 2) / 2] = 0x0000;
}


/******************************************************************************
 ******************************************************************************
  Burning Rival
 ******************************************************************************
 ******************************************************************************/


// the protection board on many system32 games has full dma/bus access
// and can write things into work RAM.  we simulate that here for burning rival.
READ16_HANDLER(brival_protection_r)
{
	if (!mem_mask)	// only trap on word-wide reads
	{
		switch (offset)
		{
			case 0:
			case 2:
			case 3:
				return 0;
				break;
		}
	}

	return segas32_workram[0xba00/2 + offset];
}

WRITE16_HANDLER(brival_protboard_w)
{
	static const int protAddress[6][2] =
	{
		{ 0x9517, 0x00/2 },
		{ 0x9597, 0x10/2 },
		{ 0x9597, 0x20/2 },
		{ 0x9597, 0x30/2 },
		{ 0x9597, 0x40/2 },
		{ 0x9617, 0x50/2 },
	};
	char ret[32];
	int curProtType;
	unsigned char *ROM = memory_region(REGION_USER1);

	switch (offset)
	{
		case 0x800/2:
			curProtType = 0;
			break;
		case 0x802/2:
			curProtType = 1;
			break;
		case 0x804/2:
			curProtType = 2;
			break;
		case 0x806/2:
			curProtType = 3;
			break;
		case 0x808/2:
			curProtType = 4;
			break;
		case 0x80a/2:
			curProtType = 5;
			break;
		default:
			if (offset >= 0xa00/2 && offset < 0xc00/2)
				return;
			logerror("brival_protboard_w: UNKNOWN WRITE: offset %x value %x\n", offset, data);
			return;
			break;
	}

	memcpy(ret, &ROM[protAddress[curProtType][0]], 16);
	ret[16] = '\0';

	memcpy(&segas32_protram[protAddress[curProtType][1]], ret, 16);
}


/******************************************************************************
 ******************************************************************************
  Arabian Fight
 ******************************************************************************
 ******************************************************************************/


// protection ram is 8-bits wide and only occupies every other address
READ16_HANDLER(arabfgt_protboard_r)
{
	int PC = activecpu_get_pc();
	int cmpVal;

	if (PC == 0xfe0325 || PC == 0xfe01e5 || PC == 0xfe035e || PC == 0xfe03cc)
	{
		cmpVal = activecpu_get_reg(1);

		// R0 always contains the value the protection is supposed to return (!)
		return cmpVal;
	}
	else
	{
		usrintf_showmessage("UNKONWN ARF PROTECTION READ PC=%x\n", PC);
	}

	return 0;
}

WRITE16_HANDLER(arabfgt_protboard_w)
{
}

READ16_HANDLER(arf_wakeup_protection_r)
{
	static const char *prot =
		"wake up! ARF!                                   ";
	return prot[offset];
}
