/***************************************************************************

    NeoGeo Protection Devices

    unknown devices
        ssideki, fatfury2, fatfury3, kof98, mslugx

    SMA chip
        kof99, garou, garouo, mslug3, kof2002

        custom banking, random number generator
        encryption (see machine/neocrypt.c)
        internal rom data

    PVC chip
        mslug5, kof2003, svcchaos

***************************************************************************/

#include "driver.h"
#include "neogeo.h"

/************************ SRAM protection***********************
  very basic check which is used on many games
***************************************************************/

int neogeo_sram_locked;
offs_t neogeo_sram_protection_hack;

void install_sram_protection(void)
{
	/* hacks to make the games which do protection checks run in arcade mode */
	/* we write protect a SRAM location so it cannot be set to 1 */
	neogeo_sram_protection_hack = ~0;
	if (	!strcmp(Machine->gamedrv->name,"fatfury3") ||
			!strcmp(Machine->gamedrv->name,"samsho3") ||
			!strcmp(Machine->gamedrv->name,"samsho3a") ||
			!strcmp(Machine->gamedrv->name,"samsho4") ||
			!strcmp(Machine->gamedrv->name,"aof3") ||
			!strcmp(Machine->gamedrv->name,"rbff1") ||
			!strcmp(Machine->gamedrv->name,"rbffspec") ||
			!strcmp(Machine->gamedrv->name,"kof95") ||
			!strcmp(Machine->gamedrv->name,"kof96") ||
			!strcmp(Machine->gamedrv->name,"kof96h") ||
			!strcmp(Machine->gamedrv->name,"kof97") ||
			!strcmp(Machine->gamedrv->name,"kof97a") ||
			!strcmp(Machine->gamedrv->name,"kof97pls") ||
			!strcmp(Machine->gamedrv->name,"kog") ||
			!strcmp(Machine->gamedrv->name,"kof98") ||
			!strcmp(Machine->gamedrv->name,"kof98k") ||
			!strcmp(Machine->gamedrv->name,"kof98n") ||
			!strcmp(Machine->gamedrv->name,"kof99") ||
			!strcmp(Machine->gamedrv->name,"kof99a") ||
			!strcmp(Machine->gamedrv->name,"kof99e") ||
			!strcmp(Machine->gamedrv->name,"kof99n") ||
			!strcmp(Machine->gamedrv->name,"kof99p") ||
			!strcmp(Machine->gamedrv->name,"kof2000") ||
			!strcmp(Machine->gamedrv->name,"kof2000n") ||
			!strcmp(Machine->gamedrv->name,"kizuna") ||
			!strcmp(Machine->gamedrv->name,"lastblad") ||
			!strcmp(Machine->gamedrv->name,"lastblda") ||
			!strcmp(Machine->gamedrv->name,"lastbld2") ||
			!strcmp(Machine->gamedrv->name,"rbff2") ||
			!strcmp(Machine->gamedrv->name,"rbff2a") ||
			!strcmp(Machine->gamedrv->name,"mslug2") ||
			!strcmp(Machine->gamedrv->name,"mslug3") ||
			!strcmp(Machine->gamedrv->name,"garou") ||
			!strcmp(Machine->gamedrv->name,"garouo") ||
			!strcmp(Machine->gamedrv->name,"garoup"))
			neogeo_sram_protection_hack = 0x100/2;

	if (!strcmp(Machine->gamedrv->name,"pulstar"))
		neogeo_sram_protection_hack = 0x35a/2;
}

WRITE16_HANDLER( neogeo_sram16_lock_w )
{
	neogeo_sram_locked = 1;
}

WRITE16_HANDLER( neogeo_sram16_unlock_w )
{
	neogeo_sram_locked = 0;
}

READ16_HANDLER( neogeo_sram16_r )
{
	return neogeo_sram16[offset];
}

WRITE16_HANDLER( neogeo_sram16_w )
{
	if (neogeo_sram_locked)
	{
logerror("PC %06x: warning: write %02x to SRAM %04x while it was protected\n",activecpu_get_pc(),data,offset<<1);
	}
	else
	{
		if (offset == neogeo_sram_protection_hack)
		{
			if (ACCESSING_LSB && (data & 0xff) == 0x01)
				return; /* fake protection pass */
		}

		COMBINE_DATA(&neogeo_sram16[offset]);
	}
}


/************************ Super Sidekicks***********************
  todo: emulate, not patch!
***************************************************************/

void ssideki_install_protection(void)
{
		/* patch out protection check */
		/* the protection routines are at 0x25dcc and involve reading and writing */
		/* addresses in the 0x2xxxxx range */
		UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
		mem16[0x2240/2] = 0x4e71;
}

/************************ Fatal Fury 2 *************************
  todo: emulate, not patch!
***************************************************************/
int neogeo_prot_data;

static READ16_HANDLER( fatfury2_protection_16_r )
{
	UINT16 res = (neogeo_prot_data >> 24) & 0xff;

	switch (offset)
	{
		case 0x55550/2:
		case 0xffff0/2:
		case 0x00000/2:
		case 0xff000/2:
		case 0x36000/2:
		case 0x36008/2:
			return res;

		case 0x36004/2:
		case 0x3600c/2:
			return ((res & 0xf0) >> 4) | ((res & 0x0f) << 4);

		default:
logerror("unknown protection read at pc %06x, offset %08x\n",activecpu_get_pc(),offset<<1);
			return 0;
	}
}

static WRITE16_HANDLER( fatfury2_protection_16_w )
{
	switch (offset)
	{
		case 0x55552/2:	 /* data == 0x5555; read back from 55550, ffff0, 00000, ff000 */
			neogeo_prot_data = 0xff00ff00;
			break;

		case 0x56782/2:	 /* data == 0x1234; read back from 36000 *or* 36004 */
			neogeo_prot_data = 0xf05a3601;
			break;

		case 0x42812/2:	 /* data == 0x1824; read back from 36008 *or* 3600c */
			neogeo_prot_data = 0x81422418;
			break;

		case 0x55550/2:
		case 0xffff0/2:
		case 0xff000/2:
		case 0x36000/2:
		case 0x36004/2:
		case 0x36008/2:
		case 0x3600c/2:
			neogeo_prot_data <<= 8;
			break;

		default:
logerror("unknown protection write at pc %06x, offset %08x, data %02x\n",activecpu_get_pc(),offset,data);
			break;
	}
}

void fatfury2_install_protection(void)
{
	/* Hacks the program rom of Fatal Fury 2, needed either in arcade or console mode */
	/* otherwise at level 2 you cannot hit the opponent and other problems */

	/* there seems to also be another protection check like the countless ones */
	/* patched above by protecting a SRAM location, but that trick doesn't work */
	/* here (or maybe the SRAM location to protect is different), so I patch out */
	/* the routine which trashes memory. Without this, the game goes nuts after */
	/* the first bonus stage. */
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
	mem16[0xb820/2] = 0x4e71;
	mem16[0xb822/2] = 0x4e71;

	/* again, the protection involves reading and writing addresses in the */
	/* 0x2xxxxx range. There are several checks all around the code. */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x2fffff, 0, 0, fatfury2_protection_16_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x2fffff, 0, 0, fatfury2_protection_16_w);
}

/************************ Fatal Fury 3 *************************
  is this still needed? -- some people report other lockups
  in this.
***************************************************************/

void fatfury3_install_protection(void)
{
	/* patch the first word, it must be 0x0010 not 0x0000 (initial stack pointer) */
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
	mem16[0x0000/2] = 0x0010;
}

/************************ King of Fighters 98*******************
  The encrypted set has a rom overlay feature, checked at
  various points in the game
***************************************************************/

WRITE16_HANDLER ( kof98_prot_w )
{
	/* info from razoola */
	UINT16* mem16 = (UINT16*)memory_region(REGION_CPU1);

	switch (data)
	{
		case 0x0090:
		logerror ("%06x kof98 - protection 0x0090 old %04x %04x\n",activecpu_get_pc(), mem16[0x100/2], mem16[0x102/2]);
		mem16[0x100/2] = 0x00c2; mem16[0x102/2] = 0x00fd;
		break;

		case 0x00f0:
		logerror ("%06x kof98 - protection 0x00f0 old %04x %04x\n",activecpu_get_pc(), mem16[0x100/2], mem16[0x102/2]);
		mem16[0x100/2] = 0x4e45; mem16[0x102/2] = 0x4f2d;
		break;

		default: // 00aa is written, but not needed?
		logerror ("%06x kof98 - unknown protection write %04x\n",activecpu_get_pc(), data);
		break;
	}
}

void install_kof98_protection(void)
{
	/* when 0x20aaaa contains 0x0090 (word) then 0x100 (normally the neogeo header) should return 0x00c200fd
   worked out using real hw */

	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x20aaaa, 0x20aaab, 0, 0, kof98_prot_w);
}


/************************ Metal Slug X *************************
  todo: emulate, not patch!
***************************************************************/

void mslugx_install_protection(void)
{
	int i;
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);

	for (i = 0;i < (0x100000/2) - 4;i++)
	{
		if (mem16[i+0] == 0x0243 &&
			mem16[i+1] == 0x0001 && 	/* andi.w  #$1, D3 */
			mem16[i+2] == 0x6600)		/* bne xxxx */
		{
			mem16[i+2] = 0x4e71;
			mem16[i+3] = 0x4e71;
		}
	}
	mem16[0x3bdc/2] = 0x4e71;
	mem16[0x3bde/2] = 0x4e71;
	mem16[0x3be0/2] = 0x4e71;
	mem16[0x3c0c/2] = 0x4e71;
	mem16[0x3c0e/2] = 0x4e71;
	mem16[0x3c10/2] = 0x4e71;

	mem16[0x3c36/2] = 0x4e71;
	mem16[0x3c38/2] = 0x4e71;
}

/************************ SMA Protection************************
  thanks to Razoola
***************************************************************/

static WRITE16_HANDLER( kof99_bankswitch_w )
{
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000,
		0x3cc000, 0x4cc000, 0x3f2000, 0x4f2000,
		0x407800, 0x507800, 0x40d000, 0x50d000,
		0x417800, 0x517800, 0x420800, 0x520800,
		0x424800, 0x524800, 0x429000, 0x529000,
		0x42e800, 0x52e800, 0x431800, 0x531800,
		0x54d000, 0x551000, 0x567000, 0x592800,
		0x588800, 0x581800, 0x599800, 0x594800,
		0x598000,	/* rest not used? */
	};

	/* unscramble bank number */
	data =
		(((data>>14)&1)<<0)+
		(((data>> 6)&1)<<1)+
		(((data>> 8)&1)<<2)+
		(((data>>10)&1)<<3)+
		(((data>>12)&1)<<4)+
		(((data>> 5)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( garou_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000, // 00
		0x280000, 0x380000, 0x2d0000, 0x3d0000, // 04
		0x2f0000, 0x3f0000, 0x400000, 0x500000, // 08
		0x420000, 0x520000, 0x440000, 0x540000, // 12
		0x498000, 0x598000, 0x4a0000, 0x5a0000, // 16
		0x4a8000, 0x5a8000, 0x4b0000, 0x5b0000, // 20
		0x4b8000, 0x5b8000, 0x4c0000, 0x5c0000, // 24
		0x4c8000, 0x5c8000, 0x4d0000, 0x5d0000, // 28
		0x458000, 0x558000, 0x460000, 0x560000, // 32
		0x468000, 0x568000, 0x470000, 0x570000, // 36
		0x478000, 0x578000, 0x480000, 0x580000, // 40
		0x488000, 0x588000, 0x490000, 0x590000, // 44
		0x5d0000, 0x5d8000, 0x5e0000, 0x5e8000, // 48
		0x5f0000, 0x5f8000, 0x600000, /* rest not used? */
	};

	/* unscramble bank number */
	data =
		(((data>> 5)&1)<<0)+
		(((data>> 9)&1)<<1)+
		(((data>> 7)&1)<<2)+
		(((data>> 6)&1)<<3)+
		(((data>>14)&1)<<4)+
		(((data>>12)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( garouo_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000, // 00
		0x280000, 0x380000, 0x2d0000, 0x3d0000, // 04
		0x2c8000, 0x3c8000, 0x400000, 0x500000, // 08
		0x420000, 0x520000, 0x440000, 0x540000, // 12
		0x598000, 0x698000, 0x5a0000, 0x6a0000, // 16
		0x5a8000, 0x6a8000, 0x5b0000, 0x6b0000, // 20
		0x5b8000, 0x6b8000, 0x5c0000, 0x6c0000, // 24
		0x5c8000, 0x6c8000, 0x5d0000, 0x6d0000, // 28
		0x458000, 0x558000, 0x460000, 0x560000, // 32
		0x468000, 0x568000, 0x470000, 0x570000, // 36
		0x478000, 0x578000, 0x480000, 0x580000, // 40
		0x488000, 0x588000, 0x490000, 0x590000, // 44
		0x5d8000, 0x6d8000, 0x5e0000, 0x6e0000, // 48
		0x5e8000, 0x6e8000, 0x6e8000, 0x000000, // 52
		0x000000, 0x000000, 0x000000, 0x000000, // 56
		0x000000, 0x000000, 0x000000, 0x000000, // 60
	};

	/* unscramble bank number */
	data =
		(((data>> 4)&1)<<0)+
		(((data>> 8)&1)<<1)+
		(((data>>14)&1)<<2)+
		(((data>> 2)&1)<<3)+
		(((data>>11)&1)<<4)+
		(((data>>13)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( mslug3_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
	  0x000000, 0x020000, 0x040000, 0x060000, // 00
	  0x070000, 0x090000, 0x0b0000, 0x0d0000, // 04
	  0x0e0000, 0x0f0000, 0x120000, 0x130000, // 08
	  0x140000, 0x150000, 0x180000, 0x190000, // 12
	  0x1a0000, 0x1b0000, 0x1e0000, 0x1f0000, // 16
	  0x200000, 0x210000, 0x240000, 0x250000, // 20
	  0x260000, 0x270000, 0x2a0000, 0x2b0000, // 24
	  0x2c0000, 0x2d0000, 0x300000, 0x310000, // 28
	  0x320000, 0x330000, 0x360000, 0x370000, // 32
	  0x380000, 0x390000, 0x3c0000, 0x3d0000, // 36
	  0x400000, 0x410000, 0x440000, 0x450000, // 40
	  0x460000, 0x470000, 0x4a0000, 0x4b0000, // 44
	  0x4c0000, /* rest not used? */
	};

	/* unscramble bank number */
	data =
		(((data>>14)&1)<<0)+
		(((data>>12)&1)<<1)+
		(((data>>15)&1)<<2)+
		(((data>> 6)&1)<<3)+
		(((data>> 3)&1)<<4)+
		(((data>> 9)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( kof2000_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000, // 00
		0x3f7800, 0x4f7800, 0x3ff800, 0x4ff800, // 04
		0x407800, 0x507800, 0x40f800, 0x50f800, // 08
		0x416800, 0x516800, 0x41d800, 0x51d800, // 12
		0x424000, 0x524000, 0x523800, 0x623800, // 16
		0x526000, 0x626000, 0x528000, 0x628000, // 20
		0x52a000, 0x62a000, 0x52b800, 0x62b800, // 24
		0x52d000, 0x62d000, 0x52e800, 0x62e800, // 28
		0x618000, 0x619000, 0x61a000, 0x61a800, // 32
	};

	/* unscramble bank number */
	data =
		(((data>>15)&1)<<0)+
		(((data>>14)&1)<<1)+
		(((data>> 7)&1)<<2)+
		(((data>> 3)&1)<<3)+
		(((data>>10)&1)<<4)+
		(((data>> 5)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}

static READ16_HANDLER( prot_9a37_r )
{
	return 0x9a37;
}

/* information about the sma random number generator provided by razoola */
/* this RNG is correct for KOF99, other games might be different */

int neogeo_rng = 0x2345;	/* this is reset in MACHINE_INIT() */

static READ16_HANDLER( sma_random_r )
{
	int old = neogeo_rng;

	int newbit = (
			(neogeo_rng >> 2) ^
			(neogeo_rng >> 3) ^
			(neogeo_rng >> 5) ^
			(neogeo_rng >> 6) ^
			(neogeo_rng >> 7) ^
			(neogeo_rng >>11) ^
			(neogeo_rng >>12) ^
			(neogeo_rng >>15)) & 1;

	neogeo_rng = ((neogeo_rng << 1) | newbit) & 0xffff;

	return old;
}

void kof99_install_protection(void)
{
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff0, 0x2ffff1, 0, 0, kof99_bankswitch_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fe446, 0x2fe447, 0, 0, prot_9a37_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff8, 0x2ffff9, 0, 0, sma_random_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffffa, 0x2ffffb, 0, 0, sma_random_r);
}

void garou_install_protection(void)
{
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fffc0, 0x2fffc1, 0, 0, garou_bankswitch_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fe446, 0x2fe447, 0, 0, prot_9a37_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff8, 0x2ffff9, 0, 0, sma_random_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffffa, 0x2ffffb, 0, 0, sma_random_r);
}

void garouo_install_protection(void)
{
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fffc0, 0x2fffc1, 0, 0, garouo_bankswitch_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fe446, 0x2fe447, 0, 0, prot_9a37_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff8, 0x2ffff9, 0, 0, sma_random_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffffa, 0x2ffffb, 0, 0, sma_random_r);
}

void mslug3_install_protection(void)
{
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fffe4, 0x2fffe5, 0, 0, mslug3_bankswitch_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fe446, 0x2fe447, 0, 0, prot_9a37_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff8, 0x2ffff9, 0, 0, sma_random_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffffa, 0x2ffffb, 0, 0, sma_random_r);
}

void kof2000_install_protection(void)
{
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fffec, 0x2fffed, 0, 0, kof2000_bankswitch_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2fe446, 0x2fe447, 0, 0, prot_9a37_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff8, 0x2ffff9, 0, 0, sma_random_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffffa, 0x2ffffb, 0, 0, sma_random_r);
}
