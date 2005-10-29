/* Sega System 32 Protection related functions */

#include "driver.h"
#include "segas32.h"


/******************************************************************************
 ******************************************************************************
  Golden Axe 2 (Revenge of Death Adder)
 ******************************************************************************
 ******************************************************************************/

#define xxxx 0x00

const unsigned char ga2_v25_opcode_table[256] = {
     xxxx,xxxx,0xEA,xxxx,xxxx,0x8B,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xFA,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x49,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,0xE8,xxxx,xxxx,0x75,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,0x8D,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xBF,xxxx,0x88,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xBC,
     xxxx,xxxx,xxxx,0x8A,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x83,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xB8,0x26,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xEB,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xB2,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,0xC3,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xB9,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,
     xxxx,xxxx,0x8E,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xBE,xxxx,xxxx,xxxx,xxxx
};

#undef xxxx

void nec_v25_cpu_decrypt(void)
{
	int i;
	unsigned char *rom = memory_region(REGION_CPU3);
	UINT8* decrypted = auto_malloc(0x100000);
	UINT8* temp = malloc(0x100000);

	// set CPU3 opcode base
	memory_set_decrypted_region(2, 0x00000, 0xfffff, decrypted);

	// make copy of ROM so original can be overwritten
	memcpy(temp, rom, 0x10000);

	for(i = 0; i < 0x10000; i++)
	{
		int j = BITSWAP16(i, 14, 11, 15, 12, 13, 4, 3, 7, 5, 10, 2, 8, 9, 6, 1, 0);

		// normal ROM data with address swap undone
		rom[i] = temp[j];

		// decryped opcodes with address swap undone
		decrypted[i] = ga2_v25_opcode_table[ temp[j] ];
	}

	memcpy(rom+0xf0000, rom, 0x10000);
	memcpy(decrypted+0xf0000, decrypted, 0x10000);

	free(temp);
}

void decrypt_ga2_protrom(void)
{
	nec_v25_cpu_decrypt();
}


WRITE16_HANDLER( ga2_dpram_w )
{
	/* does it ever actually write.. */
}

READ16_HANDLER( ga2_dpram_r )
{
	return (ga2_dpram[offset])|(ga2_dpram[offset+1]<<8);
}


#if 0 // simulation
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
#endif

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
	system32_workram[CLEARED_LEVELS / 2] = (data & ~mem_mask) | (system32_workram[CLEARED_LEVELS / 2] & mem_mask);

//Refresh current level
		if (system32_workram[CLEARED_LEVELS / 2] == 0)
		{
			level = 0x0007;
		}
		else
		{
			level =  *((memory_region(REGION_CPU1) + LEVEL_ORDER_ARRAY) + (system32_workram[CLEARED_LEVELS / 2] * 2) - 1);
			level |= *((memory_region(REGION_CPU1) + LEVEL_ORDER_ARRAY) + (system32_workram[CLEARED_LEVELS / 2] * 2) - 2) << 8;
		}
		system32_workram[CURRENT_LEVEL / 2] = level;

//Reset level status
		system32_workram[CURRENT_LEVEL_STATUS / 2] = 0x0000;
		system32_workram[(CURRENT_LEVEL_STATUS + 2) / 2] = 0x0000;
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

	return system32_workram[0xba00/2 + offset];
}

WRITE16_HANDLER(brival_protection_w)
{
	static const int protAddress[6][2] =
	{
		{ 0x109517, 0x00/2 },
		{ 0x109597, 0x10/2 },
		{ 0x109597, 0x20/2 },
		{ 0x109597, 0x30/2 },
		{ 0x109597, 0x40/2 },
		{ 0x109617, 0x50/2 },
	};
	char ret[32];
	int curProtType;
	unsigned char *ROM = memory_region(REGION_CPU1);

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
			logerror("brival_protection_w: UNKNOWN WRITE: offset %x value %x\n", offset, data);
			return;
			break;
	}

	memcpy(ret, &ROM[protAddress[curProtType][0]], 16);
	ret[16] = '\0';

	memcpy(&system32_protram[protAddress[curProtType][1]], ret, 16);
}


/******************************************************************************
 ******************************************************************************
  Dark Edge
 ******************************************************************************
 ******************************************************************************/

void darkedge_fd1149_vblank(void)
{
	program_write_word(0x20f072, 0);
	program_write_word(0x20f082, 0);

	if( program_read_byte(0x20a12c) != 0 )
	{
		program_write_byte(0x20a12c, program_read_byte(0x20a12c)-1 );

		if( program_read_byte(0x20a12c) == 0 )
			program_write_byte(0x20a12e, 1);
	}
}


WRITE16_HANDLER( darkedge_protection_w )
{
	logerror("%06x:darkedge_prot_w(%06X) = %04X & %04X\n",
		activecpu_get_pc(), 0xa00000 + 2*offset, data, mem_mask ^ 0xffff);
}


READ16_HANDLER( darkedge_protection_r )
{
	logerror("%06x:darkedge_prot_r(%06X) & %04X\n",
		activecpu_get_pc(), 0xa00000 + 2*offset, mem_mask ^ 0xffff);
	return 0xffff;
}



/******************************************************************************
 ******************************************************************************
  DBZ VRVS
 ******************************************************************************
 ******************************************************************************/

WRITE16_HANDLER( dbzvrvs_protection_w )
{
	program_write_word( 0x2080c8, program_read_word( 0x200044 ) );
}


READ16_HANDLER( dbzvrvs_protection_r )
{
	return 0xffff;
}



/******************************************************************************
 ******************************************************************************
  Arabian Fight
 ******************************************************************************
 ******************************************************************************/


// protection ram is 8-bits wide and only occupies every other address
READ16_HANDLER(arabfgt_protection_r)
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
		ui_popup("UNKONWN ARF PROTECTION READ PC=%x\n", PC);
	}

	return 0;
}

WRITE16_HANDLER(arabfgt_protection_w)
{
}

READ16_HANDLER(arf_wakeup_protection_r)
{
	static const char *prot =
		"wake up! ARF!                                   ";
	return prot[offset];
}

/******************************************************************************
 ******************************************************************************
  The J.League 1994 (Japan)
 ******************************************************************************
 ******************************************************************************/
void jleague_fd1149_vblank( void )
{
	if( program_read_byte( 0x31ff4c ) == 0 )
	{
		// move on to team browser
		if( program_read_word( 0x31ff48 ) == 0x5050 && program_read_word( 0x31ff4a ) == 0x5050 )
		{
			program_write_byte( 0x200016, 8 );
			program_write_byte( 0x31ff4c, 1 );
		}

		// default to "Antlers" instead of "Goal"
		else if( program_read_word( 0x31ff48 ) == 0x2222 && program_read_word( 0x31ff4a ) == 0x2222 )
		{
			program_write_byte( 0x20f708, 2 );
			program_write_byte( 0x31ff4c, 1 );
		}

		// Map team browser selection to opponent browser selection
		// using same lookup table that V60 uses for sound sample mapping.
		else if( program_read_word( 0x31ff40 ) == 0x2222 && program_read_word( 0x31ff42 ) == 0x2222 )
		{
			UINT8  index = program_read_byte( 0x20f700 );
			UINT16 value = program_read_word( 0x7bbc0 + index*2 );
			program_write_byte( 0x20107c, value );
			program_write_byte( 0x31ff4c, 1 );
		}
	}
}

