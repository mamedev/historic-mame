/*----------------------------------------------------------------
   Psikyo SH-2 Based Systems
   driver by David Haywood (+ Paul Priest)
   thanks to Farfetch'd for information about the sprite zoom table.
------------------------------------------------------------------

Moving on from the 68020 based system used for the first Strikers
1945 game Psikyo introduced a system using Hitachi's SH-2 CPU

There appear to be multiple revisions of this board

 Board PS3-V1 (Custom Chip PS6406B)
 -----------------------------------
 Sol Divide (c)1997
 Strikers 1945 II (c)1997
 Space Bomber Ver.B (c)1998
 Daraku Tenshi - The Fallen Angels (c)1998

 Board PS5 (Custom Chip PS6406B)
 -------------------------------
 Gunbird 2 (c)1998
 Strikers 1999 / Strikers 1945 III (c)1999

 The PS5 board appears to just have a different memory map to PS3
 Otherwise identical.

 Board PS5V2 (Custom Chip PS6406B)
 ---------------------------------
 Dragon Blaze (c)2000
 GunBarich (c)2001

 Board PS4 (Custom Chip PS6807)
 ------------------------------
 Taisen Hot Gimmick (c)1997
 Taisen Hot Gimmick Kairakuten (c)1998
 At least 2 of the other Hot Gimmick games (confirmed by Japump)
 Lode Runner - The Dig Fight (c)2000
 Quiz de Idol! Hot Debut (c)2000

 The PS4 board appears to be a cheaper board, with only simple sprites, no bgs,
 smaller palette etc, probably only 8bpp sprites too.
 Supports dual-screen though.

All the boards have

YMF278B-F (80 pin PQFP) & YAC513 (16 pin SOIC)
( YMF278B-F is OPL4 == OPL3 plus a sample playback engine. )

93C56 EEPROM
( 93c56 is a 93c46 with double the address space. )

To Do:

PS3/PS5:
  Backgrounds
    - see notes in vidhrdw file-
  Getting the priorities right is a pain ;)
  Sol Divid's music is not correct, related to sh-2 timers.
  Daruka really needs a default eeprom with the difficulty not on easiest.

PS4:
  Sprite List format not 100% understood.
  Some of the games suffer from sound problems (wrong pitch / samples played)



*-----------------------------------*
|         Tips and Tricks           |
*-----------------------------------*

Hold PL1 Button 1 and Test Mode button to get Maintenance mode for:

Space Bomber (Stage Select with choice of ships, BG Test)
Strikers 1945 II (Stage Select - buggy!, BG Test)
Sol Divide (Stage Select)
Daraku (Obj Test, Obj Dump etc.)
(this works for earlier Psikyo games as well)

--- Space Bomber ---
Keywords, what are these for???, you earn them when you complete the game
with different points.:

DOG-1
CAT-2
BUTA-3
KAME-4
IKA-5
RABBIT-6
FROG-7
TAKO-8

--- Lode Runner: The Dig Fight ---
Maintenance Code:
5-0-8-2-0

   Stage Select
    - You can have a proper single-screen battle using this
   Obj Dump
   Obj Test
   Map Editor
    -How does this work?
   Program Test
   Sequence Test
   Game Adjustment
    -You can switch the game to English temporarily.

Or use this cheat:
:loderndf:00000000:0600A533:00000001:FFFFFFFF:Language = English
:loderdfa:00000000:0600A473:00000001:FFFFFFFF:Language = English

--- Gunbird 2 ---
Maintenance Codes:
5-3-5-7-3 All Data Initialised

5-3-7-6-5 Sit an AINE Flag1
   Displays "AINE ?" at test, Extra character on Random Box only.
5-1-0-2-4 Sit an AINE Flag2
   Displays "AINE OK" at test, Use Aine by pressing "Down" on random box
5-3-1-5-7 AINE Flag Cancelled
   Clears Aine from being playable

5-2-0-4-8 Maintenance Mode
   Stage Select (Loads of cool stuff here)
   Obj Test
   BG Test
   Play Status
   Stage Status

--- Quiz de Idol! Hot Debut ---
Maintenance Code:
9-2-3-0-1

--- Gunbarich ---

1-2-3-4-5 Clear Hi-Scores
0-2-9-2-0 Maintainance Mode (Stage Select etc.)
0-2-9-1-0 Clear All Data

--- Strikers 1945 III / S1999 ---

8-1-6-5-0 Maintenance Mode
0-1-9-9-9 Secret Command Enabled [Originally 8-1-6-4-0?]
8-1-6-3-0 Secret Random Enabled
8-1-6-2-0 Secret All Disabled
8-1-6-1-0 All Data Erased
1-2-3-4-5 Reset Scores

--- Dragon Blaze ---

1-2-3-4-5 Best Score Erased
9-2-2-1-0 All Data Erased
9-2-2-2-0 Maintenance Mode

----------------------------------------------------------------*/

#include "driver.h"
#include "state.h"
#include "cpuintrf.h"

#include "vidhrdw/generic.h"
#include "cpu/sh2/sh2.h"
#include "machine/eeprom.h"

#include "psikyosh.h"

static data8_t factory_eeprom[16]  = { 0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00 };
static data8_t s1945iii_eeprom[16] = { 0x00,0x00,0x00,0x00,0x00,0x01,0x11,0x70,0x25,0x25,0x25,0x00,0x01,0x00,0x11,0xE0 };

int use_factory_eeprom;

data32_t *psikyosh_bgram, *psikyosh_zoomram, *psikyosh_vidregs, *psh_ram, *ps4_io_select;
data32_t *bgpen_1, *bgpen_2, *screen1_brt, *screen2_brt;

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,8*4,9*4,10*4,11*4,12*4,13*4,14*4,15*4 },
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64, 8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64},
	16*16*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{0,1,2,3,4,5,6,7},
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	{0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128, 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0x000, 0x100 }, // 4bpp tiles
	{ REGION_GFX1, 0, &layout_16x16x8, 0x000, 0x100 }, // 8bpp tiles
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfops4[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8, 0x000, 0x100 }, // 8bpp tiles
	{ -1 }
};

struct EEPROM_interface eeprom_interface_93C56 =
{
	8,		// address bits	8
	8,		// data bits	8
	"*110x",	// read			110x aaaaaaaa
	"*101x",	// write		101x aaaaaaaa dddddddd
	"*111x",	// erase		111x aaaaaaaa
	"*10000xxxxxxx",// lock			100x 00xxxx
	"*10011xxxxxxx",// unlock		100x 11xxxx
//	"*10001xxxx",	// write all	1 00 01xxxx dddddddddddddddd
//	"*10010xxxx"	// erase all	1 00 10xxxx
};

static NVRAM_HANDLER(93C56)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C56);
		if (file)
		{
			EEPROM_load(file);
		}
		else	// these games want the eeprom all zeros by default
		{
			int length;
			UINT8 *dat;

			dat = EEPROM_get_data_pointer(&length);
			memset(dat, 0, length);

 			if (use_factory_eeprom) /* Set the EEPROM to Factory Defaults for games needing them*/
 			{
				data8_t eeprom_data[0x100];
				int i;

				for(i=0; i<0x100; i++) eeprom_data[i] = 0;

				memcpy(eeprom_data, factory_eeprom, 0x10);

 				if (use_factory_eeprom==2) /* S1945iii suffers from corruption on highscore unless properly initialised at the end of the eeprom */
 					memcpy(eeprom_data+0xf0, s1945iii_eeprom, 0x10);

				EEPROM_set_data(eeprom_data,0x100);
			}
		}
	}
}

static WRITE32_HANDLER( psh_eeprom_w )
{
	if (ACCESSING_MSB32)
	{
		EEPROM_write_bit((data & 0x20000000) ? 1 : 0);
		EEPROM_set_cs_line((data & 0x80000000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40000000) ? ASSERT_LINE : CLEAR_LINE);

		return;
	}

	logerror("Unk EEPROM write %x mask %x\n", data, mem_mask);
}

static READ32_HANDLER( psh_eeprom_r )
{
	if (ACCESSING_MSB32)
	{
		return ((EEPROM_read_bit() << 28) | (readinputport(4) << 24)); /* EEPROM | Region */
	}

	logerror("Unk EEPROM read mask %x\n", mem_mask);

	return 0;
}

static WRITE32_HANDLER( ps4_eeprom_w )
{
	if (ACCESSING_MSW32)
	{
		EEPROM_write_bit((data & 0x00200000) ? 1 : 0);
		EEPROM_set_cs_line((data & 0x00800000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x00400000) ? ASSERT_LINE : CLEAR_LINE);

		return;
	}

	logerror("Unk EEPROM write %x mask %x\n", data, mem_mask);
}

static READ32_HANDLER( ps4_eeprom_r )
{
	if (ACCESSING_MSW32)
	{
		return ((EEPROM_read_bit() << 20)); /* EEPROM */
	}

	logerror("Unk EEPROM read mask %x\n", mem_mask);

	return 0;
}

static INTERRUPT_GEN(psikyosh_interrupt)
{
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

static READ32_HANDLER(io32_r)
{
	return ((readinputport(0) << 24) | (readinputport(1) << 16) | (readinputport(2) << 8) | (readinputport(3) << 0));
}

static READ32_HANDLER(ps4_io32_1_r) /* used by hotgmck/hgkairak for Screen1 */
{
	int ret = 0xff;
	int sel = (ps4_io_select[0] & 0x0000ff00) >> 8;

	if (sel & 1)
		ret &= readinputport(0);
	if (sel & 2)
		ret &= readinputport(1);
	if (sel & 4)
		ret &= readinputport(2);
	if (sel & 8)
		ret &= readinputport(3);

	return ret<<24 | readinputport(4);
}

static READ32_HANDLER(ps4_io32_2_r) /* used by hotgmck/hgkairak for Screen2 */
{
	int ret = 0xff;
	int sel = (ps4_io_select[0] & 0x0000ff00) >> 8;

	if (sel & 1)
		ret &= readinputport(5);
	if (sel & 2)
		ret &= readinputport(6);
	if (sel & 4)
		ret &= readinputport(7);
	if (sel & 8)
		ret &= readinputport(8);

	return ret<<24;
}

static READ32_HANDLER(loderndf_io32_1_r) /* used by loderndf for Screen1 */
{
	return ((readinputport(0) << 24) | (readinputport(1) << 16) | (readinputport(2) << 8) | (readinputport(3) << 0));
}

static READ32_HANDLER(loderndf_io32_2_r) /* used by loderndf for Screen2 */
{
	return ((readinputport(4) << 24) | (readinputport(5) << 16) | (readinputport(6) << 8) | (readinputport(7) << 0));
}

static WRITE32_HANDLER( paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]); /* is this ok .. */

	b = ((paletteram32[offset] & 0x0000ff00) >>8);
	g = ((paletteram32[offset] & 0x00ff0000) >>16);
	r = ((paletteram32[offset] & 0xff000000) >>24);

	palette_set_color(offset,r,g,b);
}

static WRITE32_HANDLER( ps4_paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	b = ((paletteram32[offset] & 0x0000ff00) >>8);
	g = ((paletteram32[offset] & 0x00ff0000) >>16);
	r = ((paletteram32[offset] & 0xff000000) >>24);

	palette_set_color(offset,r,g,b);
	palette_set_color(offset+0x800,r,g,b); // For screen 2
}

static WRITE32_HANDLER( ps4_bgpen_1_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&bgpen_1[0]);

	b = ((bgpen_1[0] & 0x0000ff00) >>8);
	g = ((bgpen_1[0] & 0x00ff0000) >>16);
	r = ((bgpen_1[0] & 0xff000000) >>24);

	palette_set_color(0x1000,r,g,b); // Clear colour for screen 1
}

static WRITE32_HANDLER( ps4_bgpen_2_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&bgpen_2[0]);

	b = ((bgpen_2[0] & 0x0000ff00) >>8);
	g = ((bgpen_2[0] & 0x00ff0000) >>16);
	r = ((bgpen_2[0] & 0xff000000) >>24);

	palette_set_color(0x1001,r,g,b); // Clear colour for screen 2
}

static WRITE32_HANDLER( ps4_screen1_brt_w )
{
	if(ACCESSING_LSB32) {
		/* Need seperate brightness for both screens if displaying together */
		double brt1 = data & 0xff;
		static double oldbrt1;

		if (brt1>0x7f) brt1 = 0x7f; /* I reckon values must be clamped to 0x7f */

		brt1 = (0x7f - brt1) / 127.0;
		if (oldbrt1 != brt1)
		{
			int i;

			for (i = 0; i < 0x800; i++)
				palette_set_brightness(i,brt1);

			oldbrt1 = brt1;
		}
	} else {
		logerror("Unk Scr 1 brt write %x mask %x\n", data, mem_mask);
	}
}

static WRITE32_HANDLER( ps4_screen2_brt_w )
{
	if(ACCESSING_LSB32) {
		/* Need seperate brightness for both screens if displaying together */
		double brt2 = data & 0xff;
		static double oldbrt2;

		if (brt2>0x7f) brt2 = 0x7f; /* I reckon values must be clamped to 0x7f */

		brt2 = (0x7f - brt2) / 127.0;

		if (oldbrt2 != brt2)
		{
			int i;

			for (i = 0x800; i < 0x1000; i++)
				palette_set_brightness(i,brt2);

			oldbrt2 = brt2;
		}
	} else {
		logerror("Unk Scr 2 brt write %x mask %x\n", data, mem_mask);
	}
}

static READ32_HANDLER( psh_ymf_fm_r )
{
	return YMF278B_status_port_0_r(0)<<24;
}

static WRITE32_HANDLER( psh_ymf_fm_w )
{
	if (!(mem_mask & 0xff000000))	// FM bank 1 address (OPL2/OPL3 compatible)
	{
		YMF278B_control_port_0_A_w(0, data>>24);
	}

	if (!(mem_mask & 0x00ff0000))	// FM bank 1 data
	{
		YMF278B_data_port_0_A_w(0, data>>16);
	}

	if (!(mem_mask & 0x0000ff00))	// FM bank 2 address (OPL3/YMF 262 extended)
	{
		YMF278B_control_port_0_B_w(0, data>>8);
	}

	if (!(mem_mask & 0x000000ff))	// FM bank 2 data
	{
		YMF278B_data_port_0_B_w(0, data);
	}
}

static WRITE32_HANDLER( psh_ymf_pcm_w )
{
	if (!(mem_mask & 0xff000000))	// PCM address (OPL4/YMF 278B extended)
	{
		YMF278B_control_port_0_C_w(0, data>>24);
	}

	if (!(mem_mask & 0x00ff0000))	// PCM data
	{
		YMF278B_data_port_0_C_w(0, data>>16);
	}
}

static MEMORY_READ32_START( ps3v1_readmem )
	{ 0x00000000, 0x000fffff, MRA32_ROM },	// program ROM (1 meg)
	{ 0x02000000, 0x021fffff, MRA32_BANK1 }, // data ROM */
	{ 0x03000000, 0x03003fff, MRA32_RAM },	// sprites
	{ 0x03004000, 0x0300ffff, MRA32_RAM },
	{ 0x03040000, 0x03044fff, MRA32_RAM },
	{ 0x03050000, 0x030501ff, MRA32_RAM },
//	{ 0x0305ffdc, 0x0305ffdf, MRA32_RAM }, // also writes to this address - might be vblank reads?
	{ 0x0305ffe0, 0x0305ffff, MRA32_RAM }, //  video registers .. or so it seems, needed by s1945ii
	{ 0x05000000, 0x05000003, psh_ymf_fm_r }, // read YMF status
	{ 0x05800000, 0x05800003, io32_r },
	{ 0x05800004, 0x05800007, psh_eeprom_r },
	{ 0x06000000, 0x060fffff, MRA32_RAM },	// main RAM (1 meg)
MEMORY_END

static MEMORY_WRITE32_START( ps3v1_writemem )
	{ 0x00000000, 0x000fffff, MWA32_ROM },	// program ROM (1 meg)
	{ 0x02000000, 0x021fffff, MWA32_ROM }, // data ROM */
	{ 0x03000000, 0x03003fff, MWA32_RAM, &spriteram32, &spriteram_size },	// sprites (might be a bit longer)
	{ 0x03004000, 0x0300ffff, MWA32_RAM, &psikyosh_bgram }, // backgrounds I think
	{ 0x03040000, 0x03044fff, paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w, &paletteram32 }, // palette..
	{ 0x03050000, 0x030501ff, MWA32_RAM, &psikyosh_zoomram }, // a gradient sometimes ...
//	{ 0x0305ffdc, 0x0305ffdf, MWA32_RAM }, // also reads from this address
	{ 0x0305ffe0, 0x0305ffff, MWA32_RAM, &psikyosh_vidregs }, //  video registers
	{ 0x05000000, 0x05000003, psh_ymf_fm_w }, // first 2 OPL4 register banks
	{ 0x05000004, 0x05000007, psh_ymf_pcm_w }, // third OPL4 register bank
	{ 0x05800004, 0x05800007, psh_eeprom_w },
	{ 0x06000000, 0x060fffff, MWA32_RAM, &psh_ram },	// work RAM
MEMORY_END

static MEMORY_READ32_START( ps4_readmem )
	{ 0x00000000, 0x000fffff, MRA32_ROM },	// program ROM (1 meg)
	{ 0x02000000, 0x021fffff, MRA32_BANK1 }, // data ROM */
	{ 0x03000000, 0x030037ff, MRA32_RAM },
	{ 0x03003fe0, 0x03003fe3, ps4_eeprom_r },
	{ 0x03003fe4, 0x03003fe7, MRA32_NOP }, // also writes to this address - might be vblank?
//	{ 0x03003fe8, 0x03003fef, MRA32_RAM }, // vid regs?
	{ 0x03004000, 0x03005fff, MRA32_RAM },
	{ 0x05000000, 0x05000003, psh_ymf_fm_r }, // read YMF status
	{ 0x05800000, 0x05800003, ps4_io32_1_r },
	{ 0x05800004, 0x05800007, ps4_io32_2_r }, // Screen 2's Controls

	{ 0x06000000, 0x060fffff, MRA32_RAM },	// main RAM (1 meg)
MEMORY_END

static MEMORY_READ32_START( loderndf_readmem )
	{ 0x00000000, 0x000fffff, MRA32_ROM },	// program ROM (1 meg)
	{ 0x02000000, 0x021fffff, MRA32_BANK1 }, // data ROM */
	{ 0x03000000, 0x030037ff, MRA32_RAM },
	{ 0x03003fe0, 0x03003fe3, ps4_eeprom_r },
	{ 0x03003fe4, 0x03003fe7, MRA32_NOP }, // also writes to this address - might be vblank?
//	{ 0x03003fe8, 0x03003fef, MRA32_RAM }, // vid regs?
	{ 0x03004000, 0x03005fff, MRA32_RAM },
	{ 0x05000000, 0x05000003, psh_ymf_fm_r }, // read YMF status
	{ 0x05800000, 0x05800003, loderndf_io32_1_r },
	{ 0x05800004, 0x05800007, loderndf_io32_2_r }, // Screen 2's Controls
	{ 0x06000000, 0x060fffff, MRA32_RAM },	// main RAM (1 meg)
MEMORY_END

static MEMORY_WRITE32_START( ps4_writemem )
	{ 0x00000000, 0x000fffff, MWA32_ROM },	// program ROM (1 meg)
	{ 0x03000000, 0x030037ff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x03003fe0, 0x03003fe3, ps4_eeprom_w },
//	{ 0x03003fe4, 0x03003fe7, MWA32_NOP }, // might be vblank?
	{ 0x03003fe4, 0x03003fef, MWA32_RAM, &psikyosh_vidregs }, // vid regs?
	{ 0x03003ff0, 0x03003ff3, ps4_screen1_brt_w }, // screen 1 brightness
	{ 0x03003ff4, 0x03003ff7, ps4_bgpen_1_dword_w, &bgpen_1 }, // screen 1 clear colour
	{ 0x03003ff8, 0x03003ffb, ps4_screen2_brt_w }, // screen 2 brightness
	{ 0x03003ffc, 0x03003fff, ps4_bgpen_2_dword_w, &bgpen_2 }, // screen 2 clear colour
	{ 0x03004000, 0x03005fff, ps4_paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w, &paletteram32 }, // palette..
	{ 0x05000000, 0x05000003, psh_ymf_fm_w }, // first 2 OPL4 register banks
	{ 0x05000004, 0x05000007, psh_ymf_pcm_w }, // third OPL4 register bank
	{ 0x05800008, 0x0580000b, MWA32_RAM, &ps4_io_select }, // Used by Mahjong games to choose input
	{ 0x06000000, 0x060fffff, MWA32_RAM, &psh_ram },	// work RAM
MEMORY_END

static MEMORY_READ32_START( ps5_readmem )
	{ 0x00000000, 0x000fffff, MRA32_ROM },	// program ROM (1 meg)
	{ 0x03000000, 0x03000003, io32_r },
	{ 0x03000004, 0x03000007, psh_eeprom_r },
	{ 0x03100000, 0x03100003, psh_ymf_fm_r },
	{ 0x04000000, 0x04003fff, MRA32_RAM },
	{ 0x04004000, 0x0400ffff, MRA32_RAM },
	{ 0x04040000, 0x04044fff, MRA32_RAM },
	{ 0x04050000, 0x040501ff, MRA32_RAM },
	{ 0x05000000, 0x0507ffff, MRA32_BANK1 },
	{ 0x0405ffe0, 0x0405ffff, MRA32_RAM }, //  video registers
	{ 0x06000000, 0x060fffff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( ps5_writemem )
	{ 0x03000004, 0x03000007, psh_eeprom_w },
	{ 0x03100000, 0x03100003, psh_ymf_fm_w }, // first 2 OPL4 register banks
	{ 0x03100004, 0x03100007, psh_ymf_pcm_w }, // third OPL4 register bank
	{ 0x04000000, 0x04003fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x04004000, 0x0400ffff, MWA32_RAM, &psikyosh_bgram },
	{ 0x04040000, 0x04044fff, paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w, &paletteram32 },
	{ 0x04050000, 0x040501ff, MWA32_RAM, &psikyosh_zoomram },
	{ 0x0405ffe0, 0x0405ffff, MWA32_RAM, &psikyosh_vidregs }, //  video registers .. or so it seems
	{ 0x06000000, 0x060fffff, MWA32_RAM, &psh_ram },
MEMORY_END

static void irqhandler(int linestate)
{
	if (linestate)
		cpu_set_irq_line(0, 12, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 12, CLEAR_LINE);
}

static struct YMF278B_interface ymf278b_interface =
{
	1,
	{ REGION_SOUND1 },
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) },
	{ irqhandler }
};

static MACHINE_DRIVER_START( psikyo3v1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, MASTER_CLOCK/2)
	MDRV_CPU_MEMORY(ps3v1_readmem,ps3v1_writemem)
	MDRV_CPU_VBLANK_INT(psikyosh_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C56)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT) /* If using alpha */
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x5000/4)

	MDRV_VIDEO_START(psikyosh)
	MDRV_VIDEO_EOF(psikyosh)
	MDRV_VIDEO_UPDATE(psikyosh)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF278B, ymf278b_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psikyo4 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, MASTER_CLOCK/2)
	MDRV_CPU_MEMORY(ps4_readmem,ps4_writemem)
	MDRV_CPU_VBLANK_INT(psikyosh_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C56)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_DUAL_MONITOR)
#if DUAL_SCREEN
	MDRV_ASPECT_RATIO(8,3)
	MDRV_SCREEN_SIZE(80*8, 32*8)
	MDRV_VISIBLE_AREA(0, 80*8-1, 0, 28*8-1)
#else
	MDRV_ASPECT_RATIO(4,3)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 28*8-1)
#endif

	MDRV_GFXDECODE(gfxdecodeinfops4)
	MDRV_PALETTE_LENGTH((0x2000/4)*2 + 2) /* 0x2000/4 for each screen. 1 for each screen clear colour */

	MDRV_VIDEO_START(psikyo4)
	MDRV_VIDEO_UPDATE(psikyo4)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF278B, ymf278b_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( loderndf )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(psikyo4)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(loderndf_readmem,ps4_writemem)

#if DUAL_SCREEN
	MDRV_VISIBLE_AREA(0, 80*8-1, 0, 30*8-1)
#else
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 30*8-1)
#endif
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psikyo5 )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(psikyo3v1)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(ps5_readmem,ps5_writemem)
MACHINE_DRIVER_END

#define UNUSED_PORT \
	PORT_START	/* not read? */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( psikyosh )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START	/* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN     )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* IN4 fake region */
	PORT_DIPNAME( 0x01, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" )
INPUT_PORTS_END

INPUT_PORTS_START( gunbird2 )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START	/* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* IN4 fake region */
	PORT_DIPNAME( 0x03, 0x02, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "International Ver A." )
	PORT_DIPSETTING(    0x02, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( s1945iii )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START	/* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* IN4 fake region */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( dragnblz )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START	/* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* IN4 fake region */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( gnbarich )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START	/* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* IN4 fake region */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( daraku )
	PORT_START /* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START /* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START  /* IN2 more controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

	PORT_START /* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* IN4 fake region */
	PORT_DIPNAME( 0x01, 0x00, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" ) /* Title screen is different, but English region text is missing */
INPUT_PORTS_END

INPUT_PORTS_START( loderndf )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 ) // Can be used as Retry button
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 ) // Can be used as Retry button
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START /* IN3 system inputs */
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) // Screen 1
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    ) // Screen 1 - 2nd slot
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_COIN3    ) // Screen 2
	PORT_BIT(  0x08, IP_ACTIVE_LOW,  IPT_COIN4    ) // Screen 2 - 2nd slot
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) // Screen 1
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) // Screen 2

	PORT_START	/* IN4 player 1 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 ) // Can be used as Retry button
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* IN5 player 2 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER4 ) // Can be used as Retry button
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START4 )

	UNUSED_PORT /* IN6 unused? */

	UNUSED_PORT /* IN7 unused? */

#if !DUAL_SCREEN
	UNUSED_PORT /* IN8 dummy, to pad below to IN9 */
	PORT_START /* IN9 fake port for screen switching */
	PORT_BITX(  0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL1+PL2 Screen", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(  0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL3+PL4 Screen", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT(   0xfc, IP_ACTIVE_LOW, IPT_UNUSED )
#endif
INPUT_PORTS_END

INPUT_PORTS_START( hotgmck )
	PORT_START	/* IN0 fake player 1 controls 1st bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 A",     KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 M",     KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 fake player 1 controls 2nd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 fake player 1 controls 3rd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 C",     KEYCODE_C,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 G",     KEYCODE_G,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 K",     KEYCODE_K,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 Chi",   KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P1 Ron",   KEYCODE_Z,        IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 fake player 1 controls 4th bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P1 D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P1 H",     KEYCODE_H,        IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P1 L",     KEYCODE_L,        IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P1 Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN4 system inputs */
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) // Screen 1
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_COIN2    ) // Screen 2
	PORT_BIT(  0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) // Screen 1
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) // Screen 2

	PORT_START	/* IN5 fake player 2 controls 1st bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 A",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 E",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 I",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 M",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P2 Kan",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 fake player 2 controls 2nd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 B",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 F",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 J",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 N",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P2 Reach", IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 fake player 2 controls 3rd bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 C",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 G",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 K",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 Chi",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0, "P2 Ron",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN8 fake player 2 controls 4th bank */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "P2 D",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "P2 H",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, 0, "P2 L",     IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "P2 Pon",   IP_KEY_DEFAULT,   IP_JOY_NONE )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#if !DUAL_SCREEN
	PORT_START /* IN9 fake port for screen switching */
	PORT_BITX(  0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL1 Screen", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(  0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL2 Screen", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT(   0xfc, IP_ACTIVE_LOW, IPT_UNUSED )
#endif
INPUT_PORTS_END

/* unused inputs also act as duplicate buttons */
INPUT_PORTS_START( hotdebut )
	PORT_START	/* IN0 player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	UNUSED_PORT /* IN2 unused? */

	PORT_START /* IN3 system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) // Screen 1
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    ) // Screen 1 - 2nd slot
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN3    ) // Screen 2
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN4    ) // Screen 2 - 2nd slot
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) // Screen 1
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) // Screen 2

	PORT_START	/* IN4 player 1 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* IN5 player 2 controls on second screen */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START4 )

	UNUSED_PORT /* IN6 unused? */

	UNUSED_PORT /* IN7 unused? */

#if !DUAL_SCREEN
	UNUSED_PORT /* IN8 dummy, to pad below to IN9 */
	PORT_START /* IN9 fake port for screen switching */
	PORT_BITX(  0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL1+PL2 Screen", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(  0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select PL3+PL4 Screen", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT(   0xfc, IP_ACTIVE_LOW, IPT_UNUSED )
#endif
INPUT_PORTS_END


/* PS3 */

ROM_START( s1945ii )
	ROM_REGION( 0x100000, REGION_CPU1, 0) /* Code */
	ROM_LOAD32_WORD_SWAP( "2_prog_l.u18", 0x000002, 0x080000, 0x20a911b8 )
	ROM_LOAD32_WORD_SWAP( "1_prog_h.u17", 0x000000, 0x080000, 0x4c0fe85e )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Tiles */
	ROM_LOAD32_WORD( "0l.u4",    0x0000000, 0x400000, 0xbfacf98d )
	ROM_LOAD32_WORD( "0h.u13",   0x0000002, 0x400000, 0x1266f67c )
	ROM_LOAD32_WORD( "1l.u3",    0x0800000, 0x400000, 0x2d3332c9 )
	ROM_LOAD32_WORD( "1h.u12",   0x0800002, 0x400000, 0x27b32c3e )
	ROM_LOAD32_WORD( "2l.u2",    0x1000000, 0x400000, 0x91ba6d23 )
	ROM_LOAD32_WORD( "2h.u20",   0x1000002, 0x400000, 0xfabf4334 )
	ROM_LOAD32_WORD( "3l.u1",    0x1800000, 0x400000, 0xa6c3704e )
	ROM_LOAD32_WORD( "3h.u19",   0x1800002, 0x400000, 0x4cd3ca70 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u32", 0x000000, 0x400000, 0xba680ca7 )
	/* 0x400000 - 0x7fffff allocated but left blank, it randomly reads from here on the
	    Iron Casket level causing a crash otherwise, not sure why, bug in the sound emulation? */
ROM_END

ROM_START( soldivid )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-prog_l.u18", 0x000002, 0x080000, 0xcf179b04 )
	ROM_LOAD32_WORD_SWAP( "1-prog_h.u17", 0x000000, 0x080000, 0xf467d1c4 )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMREGION_DISPOSE )
	/* This Space Empty! */
	ROM_LOAD32_WORD_SWAP( "4l.u10", 0x2000000, 0x400000, 0x9eb9f269 )
	ROM_LOAD32_WORD_SWAP( "4h.u31", 0x2000002, 0x400000, 0x7c76cfe7 )
	ROM_LOAD32_WORD_SWAP( "5l.u9",  0x2800000, 0x400000, 0xc59c6858 )
	ROM_LOAD32_WORD_SWAP( "5h.u30", 0x2800002, 0x400000, 0x73bc66d0 )
	ROM_LOAD32_WORD_SWAP( "6l.u8",  0x3000000, 0x400000, 0xf01b816e )
	ROM_LOAD32_WORD_SWAP( "6h.u37", 0x3000002, 0x400000, 0xfdd57361 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "sound.bin", 0x000000, 0x400000, 0xe98f8d45 )
ROM_END

ROM_START( sbomberb )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1-b_pr_l.u18", 0x000002, 0x080000, 0x52d12225 )
	ROM_LOAD32_WORD_SWAP( "1-b_pr_h.u17", 0x000000, 0x080000, 0x1bbd0345 )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.u4",  0x0000000, 0x400000, 0xb7e4ac51 )
	ROM_LOAD32_WORD( "0h.u13", 0x0000002, 0x400000, 0x235e6c27 )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, 0x3c88c48c )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, 0x15626a6e )
	ROM_LOAD32_WORD( "2l.u2",  0x1000000, 0x400000, 0x41e92f64 )
	ROM_LOAD32_WORD( "2h.u20", 0x1000002, 0x400000, 0x4ae62e84 )
	ROM_LOAD32_WORD( "3l.u1",  0x1800000, 0x400000, 0x43ba5f0f )
	ROM_LOAD32_WORD( "3h.u19", 0x1800002, 0x400000, 0xff01bb12 )
	ROM_LOAD32_WORD( "4l.u10", 0x2000000, 0x400000, 0xe491d593 )
	ROM_LOAD32_WORD( "4h.u31", 0x2000002, 0x400000, 0x7bdd377a )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u32", 0x000000, 0x400000, 0x85cbff69 )
ROM_END

ROM_START( daraku )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "4_prog_l.u18", 0x000002, 0x080000, 0x660b4609 )
	ROM_LOAD32_WORD_SWAP( "3_prog_h.u17", 0x000000, 0x080000, 0x7a9cf601 )
	ROM_LOAD16_WORD_SWAP( "prog.u16",     0x100000, 0x100000, 0x3742e990 )

	ROM_REGION( 0x3400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.u4",  0x0000000, 0x400000, 0x565d8427 )
	ROM_LOAD32_WORD( "0h.u13", 0x0000002, 0x400000, 0x9a602630 )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, 0xac5ce8e1 )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, 0xb0a59f7b )
	ROM_LOAD32_WORD( "2l.u2",  0x1000000, 0x400000, 0x2daa03b2 )
	ROM_LOAD32_WORD( "2h.u20", 0x1000002, 0x400000, 0xe98e185a )
	ROM_LOAD32_WORD( "3l.u1",  0x1800000, 0x400000, 0x1d372aa1 )
	ROM_LOAD32_WORD( "3h.u19", 0x1800002, 0x400000, 0x597f3f15 )
	ROM_LOAD32_WORD( "4l.u10", 0x2000000, 0x400000, 0xe3d58cd8 )
	ROM_LOAD32_WORD( "4h.u31", 0x2000002, 0x400000, 0xaebc9cd0 )
	ROM_LOAD32_WORD( "5l.u9",  0x2800000, 0x400000, 0xeab5a50b )
	ROM_LOAD32_WORD( "5h.u30", 0x2800002, 0x400000, 0xf157474f )
	ROM_LOAD32_WORD( "6l.u8",  0x3000000, 0x200000, 0x9f008d1b )
	ROM_LOAD32_WORD( "6h.u37", 0x3000002, 0x200000, 0xacd2d0e3 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u32", 0x000000, 0x400000, 0xef2c781d )
ROM_END

/* PS5 */

ROM_START( gunbird2 )
	ROM_REGION( 0x180000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2_prog_l.u16", 0x000002, 0x080000, 0x76f934f0 )
	ROM_LOAD32_WORD_SWAP( "1_prog_h.u17", 0x000000, 0x080000, 0x7328d8bf )
	ROM_LOAD16_WORD_SWAP( "3_pdata.u1",   0x100000, 0x080000, 0xa5b697e6 )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.u3",  0x0000000, 0x800000, 0x5c826bc8 )
	ROM_LOAD32_WORD( "0h.u10", 0x0000002, 0x800000, 0x3df0cb6c )
	ROM_LOAD32_WORD( "1l.u4",  0x1000000, 0x800000, 0x8df0c310 )
	ROM_LOAD32_WORD( "1h.u11", 0x1000002, 0x800000, 0x4ee0103b )
	ROM_LOAD32_WORD( "2l.u5",  0x2000000, 0x800000, 0xe1c7a7b8 )
	ROM_LOAD32_WORD( "2h.u12", 0x2000002, 0x800000, 0xbc8a41df )
	ROM_LOAD32_WORD( "3l.u6",  0x3000000, 0x400000, 0x0229d37f )
	ROM_LOAD32_WORD( "3h.u13", 0x3000002, 0x400000, 0xf41bbf2b )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u9", 0x000000, 0x400000, 0xf19796ab )
ROM_END

ROM_START( s1945iii )
	ROM_REGION( 0x180000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2_progl.u16", 0x000002, 0x080000, 0x5d5d385f )
	ROM_LOAD32_WORD_SWAP( "1_progh.u17", 0x000000, 0x080000, 0x1b8a5a18 )
	ROM_LOAD16_WORD_SWAP( "3_data.u1",   0x100000, 0x080000, 0x8ff5f7d3 )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.u3",  0x0000000, 0x800000, 0x70a0d52c )
	ROM_LOAD32_WORD( "0h.u10", 0x0000002, 0x800000, 0x4dcd22b4 )
	ROM_LOAD32_WORD( "1l.u4",  0x1000000, 0x800000, 0xde1042ff )
	ROM_LOAD32_WORD( "1h.u11", 0x1000002, 0x800000, 0xb51a4430 )
	ROM_LOAD32_WORD( "2l.u5",  0x2000000, 0x800000, 0x23b02dca )
	ROM_LOAD32_WORD( "2h.u12", 0x2000002, 0x800000, 0x9933ab04 )
	ROM_LOAD32_WORD( "3l.u6",  0x3000000, 0x400000, 0xf693438c )
	ROM_LOAD32_WORD( "3h.u13", 0x3000002, 0x400000, 0x2d0c334f )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u9", 0x000000, 0x400000, 0xc5374beb )
	ROM_RELOAD ( 0x400000, 0x400000 )
ROM_END

/* PS4 */

ROM_START( hotgmck )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-u23.bin", 0x000002, 0x080000, 0x23ed4aa5 )
	ROM_LOAD32_WORD_SWAP( "1-u22.bin", 0x000000, 0x080000, 0x5db3649f )
	ROM_LOAD16_WORD_SWAP( "prog.bin",  0x100000, 0x200000, 0x500f6b1b )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.bin", 0x0000000, 0x400000, 0x91f9ba60 )
	ROM_LOAD32_WORD( "0h.bin", 0x0000002, 0x400000, 0xbfa800b7 )
	ROM_LOAD32_WORD( "1l.bin", 0x0800000, 0x400000, 0x4b670809 )
	ROM_LOAD32_WORD( "1h.bin", 0x0800002, 0x400000, 0xab513a4d )
	ROM_LOAD32_WORD( "2l.bin", 0x1000000, 0x400000, 0x1a7d51e9 )
	ROM_LOAD32_WORD( "2h.bin", 0x1000002, 0x400000, 0xbf866222 )
	ROM_LOAD32_WORD( "3l.bin", 0x1800000, 0x400000, 0xa8a646f7 )
	ROM_LOAD32_WORD( "3h.bin", 0x1800002, 0x400000, 0x8c32becd )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.bin", 0x000000, 0x400000, 0xc090d51a )
	ROM_LOAD( "snd1.bin", 0x400000, 0x400000, 0xc24243b5 )
ROM_END

ROM_START( hgkairak )
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u23",   0x000002, 0x080000, 0x1c1a034d )
	ROM_LOAD32_WORD_SWAP( "1.u22",   0x000000, 0x080000, 0x24b04aa2 )
	ROM_LOAD16_WORD_SWAP( "prog.u1", 0x100000, 0x100000, 0x83cff542 )

	ROM_REGION( 0x3000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, 0xf7472212 )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, 0x30019d0f )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, 0xf46d5002 )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, 0x210592b6 )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, 0xb98adf21 )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, 0x8e3da1e1 )
	ROM_LOAD32_WORD( "3l.u5",  0x1800000, 0x400000, 0xfa7ba4ed )
	ROM_LOAD32_WORD( "3h.u14", 0x1800002, 0x400000, 0xa5d400ea )
	ROM_LOAD32_WORD( "4l.u6",  0x2000000, 0x400000, 0x76c10026 )
	ROM_LOAD32_WORD( "4h.u15", 0x2000002, 0x400000, 0x799f0889 )
	ROM_LOAD32_WORD( "5l.u7",  0x2800000, 0x400000, 0x4639ef36 )
	ROM_LOAD32_WORD( "5h.u16", 0x2800002, 0x400000, 0x549e9e9e )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, 0x0e8e5fdf )
	ROM_LOAD( "snd1.u19", 0x400000, 0x400000, 0xd8057d2f )
ROM_END

ROM_START( loderndf )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1b.u23", 0x000002, 0x080000, 0xfae92286 )
	ROM_LOAD32_WORD_SWAP( "2b.u22", 0x000000, 0x080000, 0xfe2424c0 )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x800000, 0xccae855d )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x800000, 0x7a146c59 )
	ROM_LOAD32_WORD( "1l.u3",  0x1000000, 0x800000, 0x7a9cd21e )
	ROM_LOAD32_WORD( "1h.u12", 0x1000002, 0x800000, 0x78f40d0d )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x800000, 0x2da3788f )
ROM_END

ROM_START( loderdfa )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "12.u23", 0x000002, 0x080000, 0x661d372e )
	ROM_LOAD32_WORD_SWAP( "3.u22", 0x000000, 0x080000, 0x0a63529f )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x800000, 0xccae855d )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x800000, 0x7a146c59 )
	ROM_LOAD32_WORD( "1l.u3",  0x1000000, 0x800000, 0x7a9cd21e )
	ROM_LOAD32_WORD( "1h.u12", 0x1000002, 0x800000, 0x78f40d0d )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x800000, 0x2da3788f )
ROM_END

ROM_START( hotdebut )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1.u23",   0x000002, 0x080000, 0x0b0d0027 )
	ROM_LOAD32_WORD_SWAP( "2.u22",   0x000000, 0x080000, 0xc3b5180b )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, 0x15da9983  )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, 0x76d7b73f  )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, 0x76ea3498  )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, 0xa056859f  )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, 0x9d2d1bb1  )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, 0xa7753c4d  )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, 0xeef28aa7 )
ROM_END

ROM_START( dragnblz )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2prog_h.u21",   0x000000, 0x080000, 0xfc5eade8 )
	ROM_LOAD32_WORD_SWAP( "1prog_l.u22",   0x000002, 0x080000, 0x95d6fd02 )

	ROM_REGION( 0x2c00000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites - Not All Dumped */
	ROM_LOAD32_WORD( "1l.u4",  0x0400000, 0x200000, 0 )
	ROM_LOAD32_WORD( "1h.u12", 0x0400002, 0x200000, 0 )
	ROM_LOAD32_WORD( "2l.u5",  0x0800000, 0x200000, 0 )
	ROM_LOAD32_WORD( "2h.u13", 0x0800002, 0x200000, 0 )
	ROM_LOAD32_WORD( "3l.u6",  0x0c00000, 0x200000, 0 )
	ROM_LOAD32_WORD( "3h.u14", 0x0c00002, 0x200000, 0 )
	ROM_LOAD32_WORD( "4l.u7",  0x1000000, 0x200000, 0 )
	ROM_LOAD32_WORD( "4h.u15", 0x1000002, 0x200000, 0 )
	ROM_LOAD32_WORD( "5l.u8",  0x1400000, 0x200000, 0 )
	ROM_LOAD32_WORD( "5h.u16", 0x1400002, 0x200000, 0 )
	ROM_LOAD32_WORD( "6l.u1",  0x1800000, 0x200000, 0x8b52c90b )
	ROM_LOAD32_WORD( "6h.u2",  0x1800002, 0x200000, 0x7362f929 )
	ROM_LOAD32_WORD( "7l.u19", 0x1c00000, 0x200000, 0xb4f4d86e )
	ROM_LOAD32_WORD( "7h.u20", 0x1c00002, 0x200000, 0x44b7b9cc )
	ROM_LOAD32_WORD( "8l.u28", 0x2000000, 0x200000, 0xcd079f89 )
	ROM_LOAD32_WORD( "8h.u29", 0x2000002, 0x200000, 0x3edb508a )
	ROM_LOAD32_WORD( "9l.u41", 0x2400000, 0x200000, 0x0b53cd78 )
	ROM_LOAD32_WORD( "9h.u42", 0x2400002, 0x200000, 0xbc61998a )
	ROM_LOAD32_WORD( "10l.u58",0x2800000, 0x200000, 0xa3f5c7f8 )
	ROM_LOAD32_WORD( "10h.u59",0x2800002, 0x200000, 0x30e304c4 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - Not Dumped */
	ROM_LOAD( "snd0.u52", 0x000000, 0x200000, 0 )
ROM_END

ROM_START( gnbarich )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-prog_l.u21",   0x000000, 0x080000, 0xc136cd9c )
	ROM_LOAD32_WORD_SWAP( "1-prog_h.u22",   0x000002, 0x080000, 0x6588fc96 )

	ROM_REGION( 0x2c00000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites - Not All Dumped */
	/* does gunbarich actually use 1-5, they're on the board, but all the gfx seem to be in 6-10 */
	ROM_LOAD32_WORD( "1l.u4",  0x0400000, 0x200000, 0 )
	ROM_LOAD32_WORD( "1h.u12", 0x0400002, 0x200000, 0 )
	ROM_LOAD32_WORD( "2l.u5",  0x0800000, 0x200000, 0 )
	ROM_LOAD32_WORD( "2h.u13", 0x0800002, 0x200000, 0 )
	ROM_LOAD32_WORD( "3l.u6",  0x0c00000, 0x200000, 0 )
	ROM_LOAD32_WORD( "3h.u14", 0x0c00002, 0x200000, 0 )
	ROM_LOAD32_WORD( "4l.u7",  0x1000000, 0x200000, 0 )
	ROM_LOAD32_WORD( "4h.u15", 0x1000002, 0x200000, 0 )
	ROM_LOAD32_WORD( "5l.u8",  0x1400000, 0x200000, 0 )
	ROM_LOAD32_WORD( "5h.u16", 0x1400002, 0x200000, 0 )
	ROM_LOAD32_WORD( "6l.u1",  0x1800000, 0x200000, 0x0432e1a8 )
	ROM_LOAD32_WORD( "6h.u2",  0x1800002, 0x200000, 0xf90fa3ea )
	ROM_LOAD32_WORD( "7l.u19", 0x1c00000, 0x200000, 0x36bf9a58 )
	ROM_LOAD32_WORD( "7h.u20", 0x1c00002, 0x200000, 0x4b3eafd8 )
	ROM_LOAD32_WORD( "8l.u28", 0x2000000, 0x200000, 0x026754da )
	ROM_LOAD32_WORD( "8h.u29", 0x2000002, 0x200000, 0x8cd7aaa0 )
	ROM_LOAD32_WORD( "9l.u41", 0x2400000, 0x200000, 0x02c066fe )
	ROM_LOAD32_WORD( "9h.u42", 0x2400002, 0x200000, 0x5433385a )
	ROM_LOAD32_WORD( "10l.u58",0x2800000, 0x200000, 0xa3f5c7f8 )
	ROM_LOAD32_WORD( "10h.u59",0x2800002, 0x200000, 0x30e304c4 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - Not Dumped */
	ROM_LOAD( "snd0.u52", 0x000000, 0x200000, 0 )
ROM_END


/* are these right? should i fake the counter return?
   'speedups / idle skipping isn't needed for 'hotgmck, hgkairak'
   as the core catches and skips the idle loops automatically'
*/

static READ32_HANDLER( s1945ii_speedup_r )
{
/*
PC  : 0609FC68: MOV.L   @R13,R1  // R13 is 600000C  R1 is counter  (read from r13)
PC  : 0609FC6A: ADD     #$01,R1  // add 1 to counter
PC  : 0609FC6C: MOV.L   R1,@R13  // write it back
PC  : 0609FC6E: MOV.L   @($3C,PC),R3 // 609fdac into r3
PC  : 0609FC70: MOV.L   @R3,R0  // whats there into r0
PC  : 0609FC72: TST     R0,R0 // test
PC  : 0609FC74: BT      $0609FC68
*/
	if (activecpu_get_pc()==0x609FC6A) cpu_spinuntil_int(); // Title Screens
	if (activecpu_get_pc()==0x609FED4) cpu_spinuntil_int(); // In Game
	if (activecpu_get_pc()==0x60A0172) cpu_spinuntil_int(); // Attract Demo
	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( soldivid_speedup_r )
{
 /*
PC  : 0001AE74: MOV.L   @R14,R1
PC  : 0001AE76: ADD     #$01,R1
PC  : 0001AE78: MOV.L   R1,@R14
PC  : 0001AE7A: MOV.L   @($7C,PC),R3
PC  : 0001AE7C: MOV.L   @R3,R0
PC  : 0001AE7E: TST     R0,R0
PC  : 0001AE80: BT      $0001AE74
*/
	if (activecpu_get_pc()==0x0001AFAC) cpu_spinuntil_int(); // Character Select + InGame
	if (activecpu_get_pc()==0x0001AE76) cpu_spinuntil_int(); // Everything Else?
	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( sbomberb_speedup_r )
{
/*
PC  : 060A10EC: MOV.L   @R13,R3
PC  : 060A10EE: ADD     #$01,R3
PC  : 060A10F0: MOV.L   R3,@R13
PC  : 060A10F2: MOV.L   @($34,PC),R1
PC  : 060A10F4: MOV.L   @R1,R2
PC  : 060A10F6: TST     R2,R2
PC  : 060A10F8: BT      $060A10EC
*/
	if (activecpu_get_pc()==0x060A10EE) cpu_spinuntil_int(); // title
	if (activecpu_get_pc()==0x060A165A) cpu_spinuntil_int(); // attract
	if (activecpu_get_pc()==0x060A1382) cpu_spinuntil_int(); // game
	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( daraku_speedup_r )
{
/*
PC  : 00047618: MOV.L   @($BC,PC),R0
PC  : 0004761A: MOV.L   @R0,R1
PC  : 0004761C: ADD     #$01,R1
PC  : 0004761E: MOV.L   R1,@R0
PC  : 00047620: MOV.L   @($BC,PC),R3
PC  : 00047622: MOV.L   @R3,R0
PC  : 00047624: TST     R0,R0
PC  : 00047626: BT      $00047618
*/
	if (activecpu_get_pc()==0x0004761C) cpu_spinuntil_int(); // title
	if (activecpu_get_pc()==0x00047978) cpu_spinuntil_int(); // ingame
	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( gunbird2_speedup_r )
{
/*
PC  : 06028972: MOV.L   @R14,R3   // r14 is 604000c on this one
PC  : 06028974: MOV.L   @($D4,PC),R1
PC  : 06028976: ADD     #$01,R3
PC  : 06028978: MOV.L   R3,@R14
PC  : 0602897A: MOV.L   @R1,R2
PC  : 0602897C: TST     R2,R2
PC  : 0602897E: BT      $06028972
*/
	if (activecpu_get_pc()==0x06028974) cpu_spinuntil_int();
	if (activecpu_get_pc()==0x06028E64) cpu_spinuntil_int();
	if (activecpu_get_pc()==0x06028BE6) cpu_spinuntil_int();
	return psh_ram[0x04000C/4];
}

static READ32_HANDLER( s1945iii_speedup_r )
{
	if (activecpu_get_pc()==0x0602B464) cpu_spinuntil_int(); // start up text
	if (activecpu_get_pc()==0x0602B6E2) cpu_spinuntil_int(); // intro attract
	if (activecpu_get_pc()==0x0602BC1E) cpu_spinuntil_int(); // game attract
	if (activecpu_get_pc()==0x0602B97C) cpu_spinuntil_int(); // game

	return psh_ram[0x06000C/4];
}

static READ32_HANDLER( loderndf_speedup_r )
{
/*
PC  :00001B3C: MOV.L   @R14,R3  R14 = 0x6000020
PC  :00001B3E: ADD     #$01,R3
PC  :00001B40: MOV.L   R3,@R14
PC  :00001B42: MOV.L   @($54,PC),R1
PC  :00001B44: MOV.L   @R1,R2
PC  :00001B46: TST     R2,R2
PC  :00001B48: BT      $00001B3C
*/

	if (activecpu_get_pc()==0x00001B3E) cpu_spinuntil_int();
	return psh_ram[0x000020/4];
}

static READ32_HANDLER( loderdfa_speedup_r )
{
/*
PC  :00001B48: MOV.L   @R14,R3  R14 = 0x6000020
PC  :00001B4A: ADD     #$01,R3
PC  :00001B4C: MOV.L   R3,@R14
PC  :00001B4E: MOV.L   @($54,PC),R1
PC  :00001B50: MOV.L   @R1,R2
PC  :00001B52: TST     R2,R2
PC  :00001B54: BT      $00001B48
*/

	if (activecpu_get_pc()==0x00001B4A) cpu_spinuntil_int();
	return psh_ram[0x000020/4];
}

static READ32_HANDLER( hotdebut_speedup_r )
{
/*
PC  :000029EC: MOV.L   @R14,R2
PC  :000029EE: ADD     #$01,R2
PC  :000029F0: MOV.L   R2,@R14
PC  :000029F2: MOV.L   @($64,PC),R1
PC  :000029F4: MOV.L   @R1,R3
PC  :000029F6: TST     R3,R3
PC  :000029F8: BT      $000029EC
*/

	if (activecpu_get_pc()==0x000029EE) cpu_spinuntil_int();
	return psh_ram[0x00001c/4];
}

static READ32_HANDLER( dragnblz_speedup_r )
{
	if (activecpu_get_pc()==0x06027440) cpu_spinuntil_int(); // startup texts
	if (activecpu_get_pc()==0x060276E6) cpu_spinuntil_int(); // attract intro
	if (activecpu_get_pc()==0x06027C74) cpu_spinuntil_int(); // attract game
	if (activecpu_get_pc()==0x060279A8) cpu_spinuntil_int(); // game

	return psh_ram[0x006000C/4];
}

static READ32_HANDLER( gnbarich_speedup_r )
{
/*
PC  :0602CAE6: MOV.L   @R14,R3 // R14 = 0x606000C
PC  :0602CAE8: MOV.L   @($F4,PC),R1
PC  :0602CAEA: ADD     #$01,R3
PC  :0602CAEC: MOV.L   R3,@R14 // R14 = 0x606000C
PC  :0602CAEE: MOV.L   @R1,R2
PC  :0602CAF0: TST     R2,R2
PC  :0602CAF2: BT      $0602CAE6
*/

	if (activecpu_get_pc()==0x0602CAE8) cpu_spinuntil_int(); // title logos
	if (activecpu_get_pc()==0x0602CD88) cpu_spinuntil_int(); // attract intro
	if (activecpu_get_pc()==0x0602D2F0) cpu_spinuntil_int(); // game attract
	if (activecpu_get_pc()==0x0602D042) cpu_spinuntil_int(); // game play

	return psh_ram[0x006000C/4];
}


static DRIVER_INIT( psh )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	use_factory_eeprom=0;
}

static DRIVER_INIT( s1945ii )
{
	install_mem_read32_handler(0, 0x600000c, 0x600000f, s1945ii_speedup_r );
	use_factory_eeprom=1;
}

static DRIVER_INIT( soldivid )
{
	install_mem_read32_handler(0, 0x600000c, 0x600000f, soldivid_speedup_r );
	use_factory_eeprom=0;
}

static DRIVER_INIT( sbomberb )
{
	install_mem_read32_handler(0, 0x600000c, 0x600000f, sbomberb_speedup_r );
	use_factory_eeprom=1;
}

static DRIVER_INIT( daraku )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x600000c, 0x600000f, daraku_speedup_r );
	use_factory_eeprom=0;
}

static DRIVER_INIT( gunbird2 )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x604000c, 0x604000f, gunbird2_speedup_r );
	use_factory_eeprom=1;
}

static DRIVER_INIT( s1945iii )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x606000c, 0x606000f, s1945iii_speedup_r );
	use_factory_eeprom=2;
}

static DRIVER_INIT( loderndf )
{
	install_mem_read32_handler(0, 0x6000020, 0x6000023, loderndf_speedup_r );
}

static DRIVER_INIT( loderdfa )
{
	install_mem_read32_handler(0, 0x6000020, 0x6000023, loderdfa_speedup_r );
}

static DRIVER_INIT( hotdebut )
{
	install_mem_read32_handler(0, 0x600001c, 0x600001f, hotdebut_speedup_r );
}

static DRIVER_INIT( dragnblz )
{
	install_mem_read32_handler(0, 0x606000c, 0x606000f, dragnblz_speedup_r );
	use_factory_eeprom=1;
}

static DRIVER_INIT( gnbarich )
{
	install_mem_read32_handler(0, 0x606000c, 0x606000f, gnbarich_speedup_r );
	use_factory_eeprom=1;
}

/*     YEAR  NAME      PARENT    MACHINE    INPUT     INIT      MONITOR COMPANY   FULLNAME FLAGS */

/* ps3-v1 */
GAMEX( 1997, s1945ii,  0,        psikyo3v1, psikyosh, s1945ii,  ROT270, "Psikyo", "Strikers 1945 II", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, soldivid, 0,        psikyo3v1, psikyosh, soldivid, ROT0,   "Psikyo", "Sol Divide - The Sword Of Darkness", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) // Music Tempo
GAMEX( 1998, sbomberb, 0,        psikyo3v1, psikyosh, sbomberb, ROT270, "Psikyo", "Space Bomber (ver. B)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, daraku,   0,        psikyo3v1, daraku,   daraku,   ROT0,   "Psikyo", "Daraku Tenshi - The Fallen Angels (Japan)", GAME_IMPERFECT_GRAPHICS )

/* ps5 */
GAMEX( 1998, gunbird2, 0,        psikyo5,   gunbird2, gunbird2, ROT270, "Psikyo", "Gunbird 2", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, s1945iii, 0,        psikyo5,   s1945iii, s1945iii, ROT270, "Psikyo", "Strikers 1945 III (World) / Strikers 1999 (Japan)", GAME_IMPERFECT_GRAPHICS )

/* ps5v2 */
GAMEX( 2000, dragnblz, 0,        psikyo5,   dragnblz, dragnblz, ROT270, "Psikyo", "Dragon Blaze", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND ) // incomplete dump
GAMEX( 2001, gnbarich, 0,        psikyo5,   gnbarich, gnbarich, ROT270, "Psikyo", "Gunbarich", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND ) // incomplete dump

/* ps4 */
GAME ( 1997, hotgmck,  0,        psikyo4,   hotgmck,  psh,      ROT0,   "Psikyo", "Taisen Hot Gimmick (Japan)" )
GAME ( 1998, hgkairak, 0,        psikyo4,   hotgmck,  psh,      ROT0,   "Psikyo", "Taisen Hot Gimmick Kairakuten (Japan)" )
GAME ( 2000, loderndf, 0,        loderndf,  loderndf, loderndf, ROT0,   "Psikyo", "Lode Runner - The Dig Fight (ver. B) (Japan)" )
GAME ( 2000, loderdfa, loderndf, loderndf,  loderndf, loderdfa, ROT0,   "Psikyo", "Lode Runner - The Dig Fight (ver. A) (Japan)" )
GAME ( 2000, hotdebut, 0,        loderndf,  hotdebut, hotdebut, ROT0,   "Psikyo / Moss", "Quiz de Idol! Hot Debut (Japan)" )
