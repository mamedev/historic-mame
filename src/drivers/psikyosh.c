/*----------------------------------------------------------------
   Psikyo SH-2 Based Systems
   driver by David Haywood
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

 Board PS4 (Custom Chip PS6807)
 ------------------------------
 Taisen Hot Gimmick (c)1997
 Taisen Hot Gimmick Kairakuten (c)1998
 Rest of the Hot Gimmick games (there are several)? (guess)
 Lode Runner - The Dig Fight (c)2000
 Quiz de Idol Hot Debut (c)2001 *not confirmed*

 Board PS5 (Custom Chip PS6406B)
 -------------------------------
 Gunbird 2 (c)1998
 Strikers 1999 / Strikers 1945 III *not confirmed ps5, but is custom chip ps6406b*
 Dragon Blaze? (guess)
 GunBarich? (guess)

All the boards have

YMF278B-F (80 pin PQFP) & YAC513 (16 pin SOIC)
( YMF278-F is OPL4 == OPL3 plus a sample playback engine. )

93C56 EEPROM
( 93c56 is a 93c46 with double the address space. )

To Do:

Gunbird 2 has some bad tiles on levels 5 and 7.
Bad tiles are between 0x30000 - 0x33000.
Either 3l.u6 or 3h.u13 is bad.

Backgrounds (I think its the ram after the sprite ram .. ) *started*
  - see notes in vidhrdw file-

Sound (Sound Chip Isn't Emulated)

Investigate why other games don't work.  *different custom chip, different features?*
they might be waiting on some register at the end of sprite ram ..

Strikers 1945 II hangs on one of the bosses sometimes, core bug?
or something to do with unknown / incorrectly handled reads?

are sprite colours 100% now? black clouds in one level of sbomberb, maybe alpha, yep alpha.
See small submarines on sea level of Strikers.

Getting the priorities right is a pain ;)

We probably need some addition type blend effect which the mame core doesn't support

Fixed:

Games Hang when Saving EEProm (verify code) *done*
Sprite Zoom *done* -Paul Priest
Gunbird 2 crashes after inserting a coin if the eeprom test pass *fixed*
Find Idle Loops for Speed Up *done*
why does s1945ii boot as japan when the board was world? eeprom value? *done* -Paul Priest

----------------------------------------------------------------*/

#include "driver.h"
#include "state.h"
#include "cpuintrf.h"

#include "vidhrdw/generic.h"
#include "cpu/sh2/sh2.h"
#include "machine/eeprom.h"

#define MASTER_CLOCK 57272700	// main oscillator frequency

data32_t *psikyosh_bgram, *psikyosh_unknownram2, *psikyosh_vidregs, *psh_ram;
int psikyosh_drawbg;

int use_factory_eeprom;
static data8_t factory_eeprom[16] =	{0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00 };

VIDEO_START( psikyosh );
VIDEO_EOF( psikyosh );
VIDEO_UPDATE( psikyosh );

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
	{ REGION_GFX1, 0, &layout_16x16x4, 0x000, 0xff }, // 4bpp tiles
	{ REGION_GFX1, 0, &layout_16x16x8, 0x000, 0xff }, // 8bpp tiles
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

			if (use_factory_eeprom)	/* Set the EEPROM to Factory Defaults for games needing them*/
				EEPROM_set_data(factory_eeprom,16);
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
		return ((EEPROM_read_bit() << 28) | (readinputport(4) << 24));
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
	return ((readinputport(0) << 24) |  (readinputport(1) << 16) | (readinputport(2) << 8) | (readinputport(3) << 0));
}

static WRITE32_HANDLER( paletteram32_xRRRRRGGGGGBBBBB_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]); /* is this ok .. */

	b = ((paletteram32[offset] & 0x0000ff00) >>8);
	g = ((paletteram32[offset] & 0x00ff0000) >>16);
	r = ((paletteram32[offset] & 0xff000000) >>24);

	palette_set_color(offset,r,g,b);
}

static READ32_HANDLER( psh_ymf_fm_r )
{
	return (YM3812_status_port_0_r(0)<<24);
}

static WRITE32_HANDLER( psh_ymf_fm_w )
{
	static int fm2_adr;

	if (!(mem_mask & 0xff000000))	// FM bank 1 address (OPL2/OPL3 compatible)
	{
		YM3812_control_port_0_w(0, data>>24);
	}

	if (!(mem_mask & 0x00ff0000))	// FM bank 1 data
	{
		YM3812_write_port_0_w(0, data>>16);
	}

	if (!(mem_mask & 0x0000ff00))	// FM bank 2 address (OPL3/YMF 262 extended)
	{
		fm2_adr = (data>>8)&0xff;
	}

	if (!(mem_mask & 0x000000ff))	// FM bank 2 data
	{
		logerror("FM2: write %x to register %x\n", data&0xff, fm2_adr);
	}
}

static WRITE32_HANDLER( psh_ymf_pcm_w )
{
	static int pcm_adr;

	if (!(mem_mask & 0xff000000))	// PCM address (OPL4/YMF 278B extended)
	{
		pcm_adr = data>>24;
	}

	if (!(mem_mask & 0x00ff0000))	// PCM data
	{
		logerror("PCM: write %x to register %x\n", data>>16, pcm_adr);
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
	{ 0x03040000, 0x03044fff, paletteram32_xRRRRRGGGGGBBBBB_dword_w, &paletteram32 }, // palette..
	{ 0x03050000, 0x030501ff, MWA32_RAM, &psikyosh_unknownram2 }, // a gradient sometimes ...
//	{ 0x0305ffdc, 0x0305ffdf, MWA32_RAM }, // also reads from this address
	{ 0x0305ffe0, 0x0305ffff, MWA32_RAM, &psikyosh_vidregs }, //  video registers .. or so it seems
	{ 0x05000000, 0x05000003, psh_ymf_fm_w }, // first 2 OPL4 register banks
	{ 0x05000004, 0x05000007, psh_ymf_pcm_w }, // third OPL4 register bank
	{ 0x05800004, 0x05800007, psh_eeprom_w },
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
	{ 0x0405ffe0, 0x0405ffff, MRA32_RAM }, //  video registers .. or so it seems
	{ 0x06000000, 0x060fffff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( ps5_writemem )
	{ 0x03000004, 0x03000007, psh_eeprom_w },
	{ 0x03100000, 0x03100003, psh_ymf_fm_w }, // first 2 OPL4 register banks
	{ 0x03100004, 0x03100007, psh_ymf_pcm_w }, // third OPL4 register bank
	{ 0x04000000, 0x04003fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x04004000, 0x0400ffff, MWA32_RAM, &psikyosh_bgram },
	{ 0x04040000, 0x04044fff, paletteram32_xRRRRRGGGGGBBBBB_dword_w, &paletteram32 },
	{ 0x04050000, 0x040501ff, MWA32_RAM, &psikyosh_unknownram2 },
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

static struct YM3812interface ym3812_interface =
{
	1,
	3579545,	// almost certainly wrong
	{ 35 },
	{ irqhandler },
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
//	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT) /* If using alpha */
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	// more colours breaks mame? ...
	MDRV_PALETTE_LENGTH((0x4000/4)*2)

	MDRV_VIDEO_START(psikyosh)
	MDRV_VIDEO_EOF(psikyosh)
	MDRV_VIDEO_UPDATE(psikyosh)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psikyo5 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(psikyo3v1)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(ps5_readmem,ps5_writemem)
MACHINE_DRIVER_END

INPUT_PORTS_START( psikyosh )
	PORT_START	/* player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )


	PORT_START	/* not read? */
	PORT_DIPNAME( 0x01, 0x01, "3" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN     )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* fake region */
	PORT_DIPNAME( 0x01, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" )
INPUT_PORTS_END

INPUT_PORTS_START( gunbird2 )
	PORT_START	/* player 1 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* player 2 controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )


	PORT_START	/* not read? */
	PORT_DIPNAME( 0x01, 0x01, "3" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* fake region */
	PORT_DIPNAME( 0x03, 0x01, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "International Ver A." )
	PORT_DIPSETTING(    0x02, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( daraku )
    PORT_START /* player 1 controls */
    PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1                       )
    PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
    PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
    PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
    PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
    PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
    PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
    PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

    PORT_START /* player 2 controls */
    PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2                       )
    PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
    PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
    PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
    PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
    PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
    PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
    PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

    PORT_START  /* more controls */
    PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
    PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
    PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )
    PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
    PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
    PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
    PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )
    PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

    PORT_START /* system inputs */
    PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
    PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
    PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
    PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
    PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
    PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
    PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START /* fake region */
	PORT_DIPNAME( 0x01, 0x00, "Region" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x01, "World" ) /* Title screen is different, but English region text is missing */
INPUT_PORTS_END


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

ROM_START( gunbird2 )
	ROM_REGION( 0x180000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2_prog_l.u16", 0x000002, 0x080000, 0x76f934f0 )
	ROM_LOAD32_WORD_SWAP( "1_prog_h.u17", 0x000000, 0x080000, 0x7328d8bf )
	ROM_LOAD16_WORD_SWAP( "3_pdata.u1",   0x100000, 0x080000, 0xa5b697e6 )

	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "0l.u3",  0x0000000, 0x800000, 0x5c826bc8 )
	ROM_LOAD32_WORD( "0h.u10", 0x0000002, 0x800000, 0x3df0cb6c )
	ROM_LOAD32_WORD( "1l.u4",  0x1000000, 0x800000, 0x8df0c310 )
	ROM_LOAD32_WORD( "1h.u11", 0x1000002, 0x800000, 0x4ee0103b )
	ROM_LOAD32_WORD( "2l.u5",  0x2000000, 0x800000, 0xe1c7a7b8 )
	ROM_LOAD32_WORD( "2h.u12", 0x2000002, 0x800000, 0xbc8a41df )
	ROM_LOAD32_WORD( "3l.u6",  0x3000000, 0x800000, BADCRC(0x5102f479) ) // FIRST AND SECOND HALF IDENTICAL
	ROM_LOAD32_WORD( "3h.u13", 0x3000002, 0x800000, 0xaf9bd58c )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u9", 0x000000, 0x400000, 0xf19796ab )
ROM_END

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

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u32", 0x000000, 0x400000, 0xba680ca7 )
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
	ROM_LOAD32_WORD_SWAP( "2.u23", 0x000002, 0x080000, 0x1c1a034d )
	ROM_LOAD32_WORD_SWAP( "1.u22", 0x000000, 0x080000, 0x24b04aa2 )
	ROM_LOAD16_WORD_SWAP( "prog.u1",  0x100000, 0x100000, 0x83cff542 )

	ROM_REGION( 0x3400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
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


/*	are these right? should i fake the counter return? */

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
	if (activecpu_get_pc()==0x609fc6a) cpu_spinuntil_int(); // Title Screens
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

static DRIVER_INIT( psh )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	psikyosh_drawbg=0;
	use_factory_eeprom=0;
}

static DRIVER_INIT( s1945ii )
{
	install_mem_read32_handler(0, 0x600000C, 0x600000f, s1945ii_speedup_r );
	psikyosh_drawbg=1;
	use_factory_eeprom=1;
}

static DRIVER_INIT( soldivid )
{
	install_mem_read32_handler(0, 0x600000C, 0x600000f, soldivid_speedup_r );
	psikyosh_drawbg=1;
	use_factory_eeprom=0;
}

static DRIVER_INIT( sbomberb )
{
	install_mem_read32_handler(0, 0x600000C, 0x600000f, sbomberb_speedup_r );
	psikyosh_drawbg=1;
	use_factory_eeprom=1;
}

static DRIVER_INIT( daraku )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x600000C, 0x600000f, daraku_speedup_r );
	psikyosh_drawbg=1;
	use_factory_eeprom=0;
}

static DRIVER_INIT( gunbird2 )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank(1,&RAM[0x100000]);
	install_mem_read32_handler(0, 0x604000C, 0x604000f, gunbird2_speedup_r );
	psikyosh_drawbg=1;
	use_factory_eeprom=1;
}

/* ps3-v1 */
GAMEX( 1997, s1945ii,  0,        psikyo3v1, psikyosh, s1945ii,  ROT270, "Psikyo", "Strikers 1945 II", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1997, soldivid, 0,        psikyo3v1, psikyosh, soldivid, ROT0,   "Psikyo", "Sol Divide - The Sword Of Darkness", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, sbomberb, 0,        psikyo3v1, psikyosh, sbomberb, ROT270, "Psikyo", "Space Bomber (ver. B)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, daraku,   0,        psikyo3v1, daraku,   daraku,   ROT0,   "Psikyo", "Daraku Tenshi - The Fallen Angels (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* ps4 */
GAMEX( 1997, hotgmck,  0,        psikyo3v1, psikyosh, psh,      ROT0,   "Psikyo", "Taisen Hot Gimmick (Japan)", GAME_NOT_WORKING )
GAMEX( 1998, hgkairak, 0,        psikyo3v1, psikyosh, psh,      ROT0,   "Psikyo", "Taisen Hot Gimmick Kairakuten (Japan)", GAME_NOT_WORKING )
GAMEX( 2000, loderndf, 0,        psikyo3v1, psikyosh, psh,      ROT0,   "Psikyo", "Lode Runner - The Dig Fight (ver. B)  (Japan)", GAME_NOT_WORKING )

/* ps5 */
GAMEX( 1998, gunbird2, 0,        psikyo5,   gunbird2, gunbird2, ROT270, "Psikyo", "Gunbird 2", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
