/*----------------------------------------------------------------
   Psikyo PS3/PS5/PS5v2 SH-2 Based Systems
   driver by David Haywood (+ Paul Priest)
   thanks to Farfetch'd for information about the sprite zoom table.
------------------------------------------------------------------

Moving on from the 68020 based system used for the first Strikers
1945 game Psikyo introduced a system using Hitachi's SH-2 CPU

This driver is for the single-screen PS3/PS5/PS5v2 boards

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
 Tetris The Grand Master 2 (c)2000
 Tetris The Grand Master 2 Plus (c)2000 (Confirmed by Japump to be a Dragon Blaze upgraded board)
 GunBarich (c)2001 (Appears to be a Dragon Blaze upgraded board, easily replaced chips have been swapped)

 The PS5v2 board is only different physically.

All the boards have

YMF278B-F (80 pin PQFP) & YAC513 (16 pin SOIC)
( YMF278B-F is OPL4 == OPL3 plus a sample playback engine. )

93C56 EEPROM
( 93c56 is a 93c46 with double the address space. )

To Do:

  - see notes in vidhrdw file -
  Getting the priorities right is a pain ;)

  Sol Divid's music is not correct, related to sh-2 timers.


*-----------------------------------*
|         Tips and Tricks           |
*-----------------------------------*

Hold Button during booting to test roms (Checksum 16-bit, on Words for gfx and Bytes for sound) for:

Daraku:           PL1 Button 1 (passes, doesn't test sound)
Space Bomber:     PL1 Start (passes all, only if bit 0x40 is set. But then EEPROM resets?)
Gunbird 2:        PL1 Start (passes all, only if bit 0x40 is set. But then EEPROM resets)
Strikers 1945III: PL1 Start (passes all, only if bit 0x40 is set)
Dragon Blaze:     PL1 Start (fails on undumped sample rom, bit 0x40 has to be set anyway)
Gunbarich:        PL1 Start (fails on undumped sample rom, only if bit 0x40 is set)


Hold PL1 Button 1 and Test Mode button to get Maintenance mode for:

Space Bomber, Strikers 1945 II, Sol Divide, Daraku
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

--- Gunbird 2 ---

5-2-0-4-8 Maintenance Mode
5-3-5-7-3 All Data Initialised

[Aine]
5-1-0-2-4 Secret Command Enabled ["Down" on ?]
5-3-7-6-5 Secret Random Enabled
5-3-1-5-7 Secret All Disabled

--- Strikers 1945 III / S1999 ---

8-1-6-5-0 Maintenance Mode
8-1-6-1-0 All Data Initialised
1-2-3-4-5 Best Score Erased

[X-36]
0-1-9-9-9 Secret Command Enabled ["Up" on ?]
8-1-6-3-0 Secret Random Enabled
8-1-6-2-0 Secret All Disabled

--- Dragon Blaze ---

9-2-2-2-0 Maintenance Mode
9-2-2-1-0 All Data Initialised
1-2-3-4-5 Best Score Erased

--- Gunbarich ---

0-2-9-2-0 Maintainance Mode
0-2-9-1-0 All Data Initialised
1-2-3-4-5 Best Score Erased

----------------------------------------------------------------*/

#include "driver.h"
#include "state.h"
#include "cpuintrf.h"

#include "vidhrdw/generic.h"
#include "cpu/sh2/sh2.h"
#include "machine/eeprom.h"

#include "psikyosh.h"

#define ROMTEST 0 /* Does necessary stuff to perform rom test, uses RAM as it doesn't dispose of GFX after decoding */

static data8_t factory_eeprom[16]  = { 0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00 };
static data8_t daraku_eeprom[16]   = { 0x03,0x02,0x00,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static data8_t s1945iii_eeprom[16] = { 0x00,0x00,0x00,0x00,0x00,0x01,0x11,0x70,0x25,0x25,0x25,0x00,0x01,0x00,0x11,0xe0 };
static data8_t dragnblz_eeprom[16] = { 0x00,0x01,0x11,0x70,0x25,0x25,0x25,0x00,0x01,0x00,0x11,0xe0,0x00,0x00,0x00,0x00 };
static data8_t gnbarich_eeprom[16] = { 0x00,0x0f,0x42,0x40,0x08,0x0a,0x00,0x00,0x01,0x06,0x42,0x59,0x00,0x00,0x00,0x00 };

int use_factory_eeprom, use_fake_pri;

data32_t *psikyosh_bgram, *psikyosh_zoomram, *psikyosh_vidregs, *psh_ram;

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{STEP4(0,1)},
	{STEP16(0,4)},
	{STEP16(0,16*4)},
	16*16*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{STEP8(0,1)},
	{STEP16(0,8)},
	{STEP16(0,16*8)},
	16*16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0x000, 0x100 }, // 4bpp tiles
	{ REGION_GFX1, 0, &layout_16x16x8, 0x000, 0x100 }, // 8bpp tiles
	{ -1 }
};

static struct EEPROM_interface eeprom_interface_93C56 =
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

 			if (use_factory_eeprom!=EEPROM_0) /* Set the EEPROM to Factory Defaults for games needing them*/
 			{
				data8_t eeprom_data[0x100];
				int i;

				for(i=0; i<0x100; i++) eeprom_data[i] = 0;

				memcpy(eeprom_data, factory_eeprom, 0x10);

  				if (use_factory_eeprom==EEPROM_DARAKU) /* Daraku, replace top 10 bytes with defaults (different to other games) */
 					memcpy(eeprom_data, daraku_eeprom, 0x10);

				if (use_factory_eeprom==EEPROM_S1945III) /* S1945iii suffers from corruption on highscore unless properly initialised at the end of the eeprom */
 					memcpy(eeprom_data+0xf0, s1945iii_eeprom, 0x10);

 				if (use_factory_eeprom==EEPROM_DRAGNBLZ) /* Dragnblz too */
 					memcpy(eeprom_data+0xf0, dragnblz_eeprom, 0x10);

 				if (use_factory_eeprom==EEPROM_GNBARICH) /* Might as well do Gnbarich as well, otherwise the highscore is incorrect */
 					memcpy(eeprom_data+0xf0, gnbarich_eeprom, 0x10);

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

static INTERRUPT_GEN(psikyosh_interrupt)
{
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

static READ32_HANDLER(io32_r)
{
	return ((readinputport(0) << 24) | (readinputport(1) << 16) | (readinputport(2) << 8) | (readinputport(3) << 0));
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

static WRITE32_HANDLER( psikyosh_vidregs_w )
{
	COMBINE_DATA(&psikyosh_vidregs[offset]);

#if ROMTEST
	if(offset==4) /* Configure bank for gfx test */
	{
		if (!(mem_mask & 0x000000ff) || !(mem_mask & 0x0000ff00))	// Bank
		{
			unsigned char *ROM = memory_region(REGION_GFX1);
			cpu_setbank(2,&ROM[0x20000 * (psikyosh_vidregs[offset]&0xfff)]); /* Bank comes from vidregs */
		}
	}
#endif
}

#if ROMTEST
static UINT32 sample_offs = 0;

static READ32_HANDLER( psh_sample_r ) /* Send sample data for test */
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

	return ROM[sample_offs++]<<16;
}
#endif

static READ32_HANDLER( psh_ymf_fm_r )
{
	return YMF278B_status_port_0_r(0)<<24; /* Also, bit 0 being high indicates not ready to send sample data for test */
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

#if ROMTEST
		if (data>>24 == 0x06)	// Reset Sample reading (They always write this code immediately before reading data)
		{
			sample_offs = 0;
		}
#endif
	}

	if (!(mem_mask & 0x00ff0000))	// PCM data
	{
		YMF278B_data_port_0_C_w(0, data>>16);
	}
}

static MEMORY_READ32_START( ps3v1_readmem )
	{ 0x00000000, 0x000fffff, MRA32_ROM },	// program ROM (1 meg)
	{ 0x02000000, 0x021fffff, MRA32_BANK1 }, // data ROM
	{ 0x03000000, 0x03003fff, MRA32_RAM },	// sprites
	{ 0x03004000, 0x0300ffff, MRA32_RAM },
	{ 0x03040000, 0x03044fff, MRA32_RAM },
	{ 0x03050000, 0x030501ff, MRA32_RAM },
	{ 0x0305ffdc, 0x0305ffdf, MRA32_NOP }, // also writes to this address - might be vblank reads?
	{ 0x0305ffe0, 0x0305ffff, MRA32_RAM }, //  video registers
	{ 0x05000000, 0x05000003, psh_ymf_fm_r }, // read YMF status
	{ 0x05800000, 0x05800003, io32_r },
	{ 0x05800004, 0x05800007, psh_eeprom_r },
	{ 0x06000000, 0x060fffff, MRA32_RAM }, // main RAM (1 meg)

#if ROMTEST
	{ 0x05000004, 0x05000007, psh_sample_r }, // data for rom tests (Used to verify Sample rom)
	{ 0x03060000, 0x0307ffff, MRA32_BANK2 }, // data for rom tests (gfx), data is controlled by vidreg
	{ 0x04060000, 0x0407ffff, MRA32_BANK2 }, // data for rom tests (gfx) (Mirrored?)
#endif
MEMORY_END

static MEMORY_WRITE32_START( ps3v1_writemem )
	{ 0x00000000, 0x000fffff, MWA32_ROM },	// program ROM (1 meg)
	{ 0x02000000, 0x021fffff, MWA32_ROM }, // data ROM
	{ 0x03000000, 0x03003fff, MWA32_RAM, &spriteram32, &spriteram_size },	// sprites (might be a bit longer)
	{ 0x03004000, 0x0300ffff, MWA32_RAM, &psikyosh_bgram }, // backgrounds
	{ 0x03040000, 0x03044fff, paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w, &paletteram32 }, // palette..
	{ 0x03050000, 0x030501ff, MWA32_RAM, &psikyosh_zoomram }, // a gradient sometimes ...
	{ 0x0305ffdc, 0x0305ffdf, MWA32_RAM }, // also reads from this address
	{ 0x0305ffe0, 0x0305ffff, psikyosh_vidregs_w, &psikyosh_vidregs }, //  video registers
	{ 0x05000000, 0x05000003, psh_ymf_fm_w }, // first 2 OPL4 register banks
	{ 0x05000004, 0x05000007, psh_ymf_pcm_w }, // third OPL4 register bank
	{ 0x05800004, 0x05800007, psh_eeprom_w },
	{ 0x06000000, 0x060fffff, MWA32_RAM, &psh_ram }, // work RAM
MEMORY_END

static MEMORY_READ32_START( ps5_readmem )
	{ 0x00000000, 0x000fffff, MRA32_ROM }, // program ROM (1 meg)
	{ 0x03000000, 0x03000003, io32_r },
	{ 0x03000004, 0x03000007, psh_eeprom_r },
	{ 0x03100000, 0x03100003, psh_ymf_fm_r },
	{ 0x04000000, 0x04003fff, MRA32_RAM },	// sprites
	{ 0x04004000, 0x0400ffff, MRA32_RAM },
	{ 0x04040000, 0x04044fff, MRA32_RAM },
	{ 0x04050000, 0x040501ff, MRA32_RAM },
	{ 0x0405ffdc, 0x0405ffdf, MRA32_NOP }, // also writes to this address - might be vblank reads?
	{ 0x0405ffe0, 0x0405ffff, MRA32_RAM }, // video registers
	{ 0x05000000, 0x0507ffff, MRA32_BANK1 }, // data ROM
	{ 0x06000000, 0x060fffff, MRA32_RAM },

#if ROMTEST
	{ 0x03100004, 0x03100007, psh_sample_r }, // data for rom tests (Used to verify Sample rom)
	{ 0x04060000, 0x0407ffff, MRA32_BANK2 }, // data for rom tests (gfx), data is controlled by vidreg
#endif
MEMORY_END

static MEMORY_WRITE32_START( ps5_writemem )
	{ 0x00000000, 0x000fffff, MWA32_ROM },	// program ROM (1 meg)
	{ 0x03000004, 0x03000007, psh_eeprom_w },
	{ 0x03100000, 0x03100003, psh_ymf_fm_w }, // first 2 OPL4 register banks
	{ 0x03100004, 0x03100007, psh_ymf_pcm_w }, // third OPL4 register bank
	{ 0x04000000, 0x04003fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x04004000, 0x0400ffff, MWA32_RAM, &psikyosh_bgram }, // backgrounds
	{ 0x04040000, 0x04044fff, paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w, &paletteram32 },
	{ 0x04050000, 0x040501ff, MWA32_RAM, &psikyosh_zoomram },
	{ 0x0405ffdc, 0x0405ffdf, MWA32_RAM }, // also reads from this address
	{ 0x0405ffe0, 0x0405ffff, psikyosh_vidregs_w, &psikyosh_vidregs }, // video registers
	{ 0x05000000, 0x0507ffff, MWA32_ROM }, // data ROM
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
	{ MASTER_CLOCK/2 },
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

#define PORT_COIN( debug ) \
	PORT_START /* IN3 system inputs */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1    ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2    ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN  ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) \
	PORT_DIPNAME( 0x40, debug ? 0x00 : 0x40, "Debug" ) /* Must be high for dragnblz, low for others (Resets EEPROM?). Debug stuff */ \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

#define PORT_PLAYER( player, start, buttons ) \
	PORT_START \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, start ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, (buttons>=3)?(IPT_BUTTON3 | player):IPT_UNKNOWN ) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, (buttons>=2)?(IPT_BUTTON2 | player):IPT_UNKNOWN ) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, (buttons>=1)?(IPT_BUTTON1 | player):IPT_UNKNOWN ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | player ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | player ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | player ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | player )

INPUT_PORTS_START( s1945ii )
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 2 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 2 )
	UNUSED_PORT /* IN2 unused? */
	PORT_COIN( 0 )

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" )
INPUT_PORTS_END

INPUT_PORTS_START( soldivid )
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 3 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 3 )
	UNUSED_PORT /* IN2 unused? */
	PORT_COIN( 0 )

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" )
INPUT_PORTS_END

INPUT_PORTS_START( daraku )
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 2 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 2 )

	PORT_START  /* IN2 more controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

	PORT_COIN( 0 )

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" ) /* Title screen is different, English is default now */
INPUT_PORTS_END

INPUT_PORTS_START( sbomberb )
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 2 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 2 )
	UNUSED_PORT /* IN2 unused? */
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, but EEPROM resets? */

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" )
INPUT_PORTS_END

INPUT_PORTS_START( gunbird2 ) /* Different Region */
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 3 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 3 )
	UNUSED_PORT /* IN2 unused? */
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, but EEPROM resets */

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x02, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "International Ver A." )
	PORT_DIPSETTING(    0x02, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( s1945iii ) /* Different Region again */
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 3 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 3 )
	UNUSED_PORT /* IN2 unused? */
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, EEPROM doesn't reset */

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( dragnblz ) /* Security requires bit high */
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 3 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 3 )
	UNUSED_PORT /* IN2 unused? */
	PORT_COIN( 1 ) /* Must be HIGH (Or Security Error), so can perform test */

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( gnbarich ) /* Same as S1945iii except only one button */
	PORT_PLAYER( IPF_PLAYER1, IPT_START1, 1 )
	PORT_PLAYER( IPF_PLAYER2, IPT_START2, 1 )
	UNUSED_PORT /* IN2 unused? */
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, but EEPROM resets? */

	PORT_START /* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

#if ROMTEST
#define ROMTEST_GFX 0
#else
#define ROMTEST_GFX ROMREGION_DISPOSE
#endif

/* PS3 */

ROM_START( soldivid )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-prog_l.u18", 0x000002, 0x080000, 0xcf179b04 )
	ROM_LOAD32_WORD_SWAP( "1-prog_h.u17", 0x000000, 0x080000, 0xf467d1c4 )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMTEST_GFX )
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

ROM_START( s1945ii )
	ROM_REGION( 0x100000, REGION_CPU1, 0) /* Code */
	ROM_LOAD32_WORD_SWAP( "2_prog_l.u18", 0x000002, 0x080000, 0x20a911b8 )
	ROM_LOAD32_WORD_SWAP( "1_prog_h.u17", 0x000000, 0x080000, 0x4c0fe85e )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )	/* Tiles */
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
	ROM_RELOAD ( 0x400000, 0x400000 )
	/* 0x400000 - 0x7fffff allocated but left blank, it randomly reads from here on the
	    Iron Casket level causing a crash otherwise, not sure why, bug in the sound emulation? */
ROM_END

ROM_START( daraku )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "4_prog_l.u18", 0x000002, 0x080000, 0x660b4609 )
	ROM_LOAD32_WORD_SWAP( "3_prog_h.u17", 0x000000, 0x080000, 0x7a9cf601 )
	ROM_LOAD16_WORD_SWAP( "prog.u16",     0x100000, 0x100000, 0x3742e990 )

	ROM_REGION( 0x3400000, REGION_GFX1, ROMTEST_GFX )
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

ROM_START( sbomberb )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1-b_pr_l.u18", 0x000002, 0x080000, 0x52d12225 )
	ROM_LOAD32_WORD_SWAP( "1-b_pr_h.u17", 0x000000, 0x080000, 0x1bbd0345 )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMTEST_GFX )
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

/* PS5 */

ROM_START( gunbird2 )
	ROM_REGION( 0x180000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2_prog_l.u16", 0x000002, 0x080000, 0x76f934f0 )
	ROM_LOAD32_WORD_SWAP( "1_prog_h.u17", 0x000000, 0x080000, 0x7328d8bf )
	ROM_LOAD16_WORD_SWAP( "3_pdata.u1",   0x100000, 0x080000, 0xa5b697e6 )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u3",  0x0000000, 0x800000, 0x5c826bc8 )
	ROM_LOAD32_WORD( "0h.u10", 0x0000002, 0x800000, 0x3df0cb6c )
	ROM_LOAD32_WORD( "1l.u4",  0x1000000, 0x800000, 0x1558358d )
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

	ROM_REGION( 0x3800000, REGION_GFX1, ROMTEST_GFX )
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

/* PS5v2 */

ROM_START( dragnblz )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2prog_h.u21",   0x000000, 0x080000, 0xfc5eade8 )
	ROM_LOAD32_WORD_SWAP( "1prog_l.u22",   0x000002, 0x080000, 0x95d6fd02 )

	ROM_REGION( 0x2c00000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "1l.u4",  0x0400000, 0x200000, 0xc2eb565c )
	ROM_LOAD32_WORD( "1h.u12", 0x0400002, 0x200000, 0x23cb46b7 )
	ROM_LOAD32_WORD( "2l.u5",  0x0800000, 0x200000, 0xbc256aea )
	ROM_LOAD32_WORD( "2h.u13", 0x0800002, 0x200000, 0xb75f59ec )
	ROM_LOAD32_WORD( "3l.u6",  0x0c00000, 0x200000, 0x4284f008 )
	ROM_LOAD32_WORD( "3h.u14", 0x0c00002, 0x200000, 0xabe5cbbf )
	ROM_LOAD32_WORD( "4l.u7",  0x1000000, 0x200000, 0xc9fcf2e5 )
	ROM_LOAD32_WORD( "4h.u15", 0x1000002, 0x200000, 0x0ab0a12a )
	ROM_LOAD32_WORD( "5l.u8",  0x1400000, 0x200000, 0x68d03ccf )
	ROM_LOAD32_WORD( "5h.u16", 0x1400002, 0x200000, 0x5450fbca )
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

	ROM_REGION( 0x2c00000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	/* Gunbarich doesn't actually use 1-5 and 10, they're on the board, but all the gfx are in 6-9
	   The game was an upgrade to Dragon Blaze, only some of the roms were replaced however it
	   appears the board needs to be fully populated to work correctly so the Dragon Blaze roms
	   were left on it.  After hooking up hidden rom test we can see only the 8 roms we load are
	   tested */
//	ROM_LOAD32_WORD( "1l.u4",  0x0400000, 0x200000, 0xc2eb565c ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "1h.u12", 0x0400002, 0x200000, 0x23cb46b7 ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "2l.u5",  0x0800000, 0x200000, 0xbc256aea ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "2h.u13", 0x0800002, 0x200000, 0xb75f59ec ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "3l.u6",  0x0c00000, 0x200000, 0x4284f008 ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "3h.u14", 0x0c00002, 0x200000, 0xabe5cbbf ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "4l.u7",  0x1000000, 0x200000, 0xc9fcf2e5 ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "4h.u15", 0x1000002, 0x200000, 0x0ab0a12a ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "5l.u8",  0x1400000, 0x200000, 0x68d03ccf ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "5h.u16", 0x1400002, 0x200000, 0x5450fbca ) /* From Dragon Blaze */
	ROM_LOAD32_WORD( "6l.u1",  0x1800000, 0x200000, 0x0432e1a8 )
	ROM_LOAD32_WORD( "6h.u2",  0x1800002, 0x200000, 0xf90fa3ea )
	ROM_LOAD32_WORD( "7l.u19", 0x1c00000, 0x200000, 0x36bf9a58 )
	ROM_LOAD32_WORD( "7h.u20", 0x1c00002, 0x200000, 0x4b3eafd8 )
	ROM_LOAD32_WORD( "8l.u28", 0x2000000, 0x200000, 0x026754da )
	ROM_LOAD32_WORD( "8h.u29", 0x2000002, 0x200000, 0x8cd7aaa0 )
	ROM_LOAD32_WORD( "9l.u41", 0x2400000, 0x200000, 0x02c066fe )
	ROM_LOAD32_WORD( "9h.u42", 0x2400002, 0x200000, 0x5433385a )
//	ROM_LOAD32_WORD( "10l.u58",0x2800000, 0x200000, 0xa3f5c7f8 ) /* From Dragon Blaze */
//	ROM_LOAD32_WORD( "10h.u59",0x2800002, 0x200000, 0x30e304c4 ) /* From Dragon Blaze */

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "snd0.u52", 0x000000, 0x200000, 0x7b10436b )
ROM_END

/* are these right? should i fake the counter return?
   'speedups / idle skipping isn't needed for 'hotgmck, hgkairak'
   as the core catches and skips the idle loops automatically'
*/

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

static DRIVER_INIT( soldivid )
{
	install_mem_read32_handler(0, 0x600000c, 0x600000f, soldivid_speedup_r );
	use_factory_eeprom=EEPROM_0;
	use_fake_pri=0; /* Fixes fades, breaks pics in intro */
}

static DRIVER_INIT( s1945ii )
{
	install_mem_read32_handler(0, 0x600000c, 0x600000f, s1945ii_speedup_r );
	use_factory_eeprom=EEPROM_DEFAULT;
	use_fake_pri=1; /* Breaks small subs, fixes everything else */
}

static DRIVER_INIT( daraku )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x600000c, 0x600000f, daraku_speedup_r );
	use_factory_eeprom=EEPROM_DARAKU;
	use_fake_pri=0; /* Breaks Character bg to use it, ok without */
}

static DRIVER_INIT( sbomberb )
{
	install_mem_read32_handler(0, 0x600000c, 0x600000f, sbomberb_speedup_r );
	use_factory_eeprom=EEPROM_DEFAULT;
	use_fake_pri=1;
}

static DRIVER_INIT( gunbird2 )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x604000c, 0x604000f, gunbird2_speedup_r );
	use_factory_eeprom=EEPROM_DEFAULT;
	use_fake_pri=1;
}

static DRIVER_INIT( s1945iii )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x606000c, 0x606000f, s1945iii_speedup_r );
	use_factory_eeprom=EEPROM_S1945III;
	use_fake_pri=1;
}

static DRIVER_INIT( dragnblz )
{
	install_mem_read32_handler(0, 0x606000c, 0x606000f, dragnblz_speedup_r );
	use_factory_eeprom=EEPROM_DRAGNBLZ;
	use_fake_pri=1;
}

static DRIVER_INIT( gnbarich )
{
	install_mem_read32_handler(0, 0x606000c, 0x606000f, gnbarich_speedup_r );
	use_factory_eeprom=EEPROM_GNBARICH;
	use_fake_pri=1; /* Fixes transitions and endings, time and lives are hidden though :( */
}

/*     YEAR  NAME      PARENT    MACHINE    INPUT     INIT      MONITOR COMPANY   FULLNAME FLAGS */

/* ps3-v1 */
GAMEX( 1997, soldivid, 0,        psikyo3v1, soldivid, soldivid, ROT0,   "Psikyo", "Sol Divide - The Sword Of Darkness", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) // Music Tempo
GAMEX( 1997, s1945ii,  0,        psikyo3v1, s1945ii,  s1945ii,  ROT270, "Psikyo", "Strikers 1945 II", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, daraku,   0,        psikyo3v1, daraku,   daraku,   ROT0,   "Psikyo", "Daraku Tenshi - The Fallen Angels", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, sbomberb, 0,        psikyo3v1, sbomberb, sbomberb, ROT270, "Psikyo", "Space Bomber (ver. B)", GAME_IMPERFECT_GRAPHICS )

/* ps5 */
GAMEX( 1998, gunbird2, 0,        psikyo5,   gunbird2, gunbird2, ROT270, "Psikyo", "Gunbird 2", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, s1945iii, 0,        psikyo5,   s1945iii, s1945iii, ROT270, "Psikyo", "Strikers 1945 III (World) / Strikers 1999 (Japan)", GAME_IMPERFECT_GRAPHICS )

/* ps5v2 */
GAMEX( 2000, dragnblz, 0,        psikyo5,   dragnblz, dragnblz, ROT270, "Psikyo", "Dragon Blaze", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) // incomplete dump
GAMEX( 2001, gnbarich, 0,        psikyo5,   gnbarich, gnbarich, ROT270, "Psikyo", "Gunbarich", GAME_IMPERFECT_GRAPHICS )
