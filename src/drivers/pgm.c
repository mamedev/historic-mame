
/* PGM System (c)1997 IGS

Based on Information from ElSemi

A flexible cartridge based platform some would say was designed to compete with
SNK's NeoGeo and Capcom's CPS Hardware systems, despite its age it only uses a
68000 for the main processor and a Z80 to drive the sound, just like the two
previously mentioned systems in that respect..

Resolution is 448x224, 15 bit colour

Sound system is ICS WaveFront 2115 Wavetable midi syntesizer, used in some
actual sound cards (Turtle Beach)

The later games would appear to be Encrypted

Roms Contain the Following Data

Pxxxx - 68k Program
Txxxx - TX & BG Graphics (2 formats within the same rom)
Mxxxx - Music samples (8 bit mono 11025Hz)
Axxxx - Colour Data (for sprites)
Bxxxx - Masks & A Rom Colour Indexes (for sprites)

There is no rom for the Z80, the program is uploaded by the 68k

Known Games on this Platform
----------------------------

Oriental Legends
Sengoku Senki / Knights of Valour Series
-
Sangoku Senki (c)1999 IGS
Sangoku Senki Super Heroes (c)1999 IGS
Sangoku Senki 2 Knights of Valour (c)2000 IGS
Sangoku Senki Busyou Souha (c)2001 IGS
-
DoDonPachi II (Bee Storm)
Photo Y2K? (and its sequel?)
Material Artist
Dragon World 2+3
.. lots more ..


To Do / Notes:

get the other game working
sprite zooming (zoom table is contained in vidram)
sprite masking (lower priority sprites under bg layers can mask higher ones)
calender
better protection emulation in orlegend
hook up the z80 + emulate sound chip
optimize?
layer enables?
sprites use dma?
verify some things
other 2 interrupts
the 'encryption' info came from unknown 3rd party, could be wrong

*/

#include "driver.h"

data16_t *pgm_mainram, *pgm_bg_videoram, *pgm_tx_videoram, *pgm_videoregs;
WRITE16_HANDLER( pgm_tx_videoram_w );
WRITE16_HANDLER( pgm_bg_videoram_w );
VIDEO_START( pgm );
VIDEO_UPDATE( pgm );
//void pgm_p_decrypt(void);

/*** Memory Maps *************************************************************/

static READ16_HANDLER( orlegend_prot_r );

static MEMORY_READ16_START( pgm_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },   /* BIOS ROM */
	{ 0x100000, 0x5fffff, MRA16_BANK1 }, /* Game ROM */

	{ 0x800000, 0x81ffff, MRA16_RAM }, /* Main Ram, Sprites, SRAM? */
	{ 0x900000, 0x903fff, MRA16_RAM }, /* Backgrounds */
	{ 0x904000, 0x90ffff, MRA16_RAM }, /* Text Layer */
	{ 0xa00000, 0xa011ff, MRA16_RAM }, /* Palette */
	{ 0xb00000, 0xb0ffff, MRA16_RAM }, /* Video Regs inc. Zoom Table */
	{ 0xc10000, 0xC1ffff, MRA16_RAM }, /* Z80 Program? */
//
//	{ 0xc00004, 0xc00005, input_port_4_word_r }, // calender stuff?
//	{ 0xc00006, 0xc00007, input_port_5_word_r }, // calender stuff?

//	{ 0xC0400e, 0xC0400f, asic3_r1 }, // ASIC 3

	{ 0xC08000, 0xC08001, input_port_0_word_r }, // p1+p2 controls
	{ 0xC08002, 0xC08003, input_port_1_word_r }, // p3+p4 controls
	{ 0xC08004, 0xC08005, input_port_2_word_r }, // extra controls
	{ 0xC08006, 0xC08007, input_port_3_word_r }, // dipswitches
MEMORY_END

static MEMORY_WRITE16_START( pgm_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },   /* BIOS ROM */
	{ 0x100000, 0x5fffff, MWA16_BANK1 }, /* Game ROM */

	{ 0x800000, 0x81ffff, MWA16_RAM, &pgm_mainram },
	{ 0x900000, 0x903fff, pgm_bg_videoram_w, &pgm_bg_videoram },
	{ 0x904000, 0x90ffff, pgm_tx_videoram_w, &pgm_tx_videoram },
	{ 0xa00000, 0xa011ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xb00000, 0xb0ffff, MWA16_RAM, &pgm_videoregs },
	{ 0xc10000, 0xC1ffff, MWA16_RAM },

//	{ 0xC04000, 0xC04001, asic3_w1 }, //ASIC3
//	{ 0xC0400e, 0xC0400f, asic3_w2 }, //ASIC3
MEMORY_END

/*** ASIC 3 - Protection? ****************************************************/

/* this isn't right, we patch the branch instructions for now */

static READ16_HANDLER( orlegend_prot_r )
{
	int res;
	res = -1;

	/* Start of Boot Up */
	if (activecpu_get_pc() == 0x145bda) res = 0x80;
	if (activecpu_get_pc() == 0x145c00) res = 0x00; // NOT 0x80 set
	if (activecpu_get_pc() == 0x145c28) res = 0x84;
	if (activecpu_get_pc() == 0x145c54) res = 0x80;
	if (activecpu_get_pc() == 0x145c80) res = 0x04;
	if (activecpu_get_pc() == 0x145caa) res = 0x00; // NOT 0x84 set
	if (activecpu_get_pc() == 0x145cd2) res = 0x20;
	if (activecpu_get_pc() == 0x145cfc) res = 0x00; // NOT 0x20 set

	if (res != -1) logerror("%06x: prot_r %04x\n",activecpu_get_pc(),res);
	else logerror("%06x: prot_r unknown\n",activecpu_get_pc());

	if (res == -1) res = 0x00;

	return res;
}

void remove_orlegend_prot(void)
{
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);

		mem16[0x146AF4/2]=0x4e71; mem16[0x146AF6/2]=0x4e71;
		mem16[0x145D02/2]=0x4e71; mem16[0x145D04/2]=0x4e71;
		mem16[0x145CDC/2]=0x4e71; mem16[0x145CDE/2]=0x4e71;
		mem16[0x145CB0/2]=0x4e71; mem16[0x145CB2/2]=0x4e71;
		mem16[0x145C8A/2]=0x4e71; mem16[0x145C8C/2]=0x4e71;
		mem16[0x145C5E/2]=0x4e71; mem16[0x145C60/2]=0x4e71;
		mem16[0x145C32/2]=0x4e71; mem16[0x145C34/2]=0x4e71;
		mem16[0x145C06/2]=0x4e71; mem16[0x145C08/2]=0x4e71;
		mem16[0x145BE4/2]=0x4e71; mem16[0x145BE6/2]=0x4e71;

		mem16[0x145D48/2]=0x4e71; mem16[0x145D4a/2]=0x4e71;
}

/*** Input Ports *************************************************************/

/* enough for 4 players, the basic dips mapped are listed in the test mode */

INPUT_PORTS_START( pgm )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER4 )

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, "Test Mode" )  // start 1?
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" ) // ip?
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" ) // down?
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )


	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, "4" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ) )  // Freezes if off?
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, "5" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/*** GFX Decodes *************************************************************/

/* we can't decode the sprite data like this because it isn't tile based.  the
   data decoded by pgm32_charlayout was rearranged at start-up */

static struct GfxLayout pgm8_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20,16,  28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static struct GfxLayout pgm32_charlayout =
{
	32,32,
	RGN_FRAC(1,1),
	5,
	{ 3,4,5,6,7 },
	{ 0  , 8 ,16 ,24 ,32 ,40 ,48 ,56 ,
	  64 ,72 ,80 ,88 ,96 ,104,112,120,
	  128,136,144,152,160,168,176,184,
	  192,200,208,216,224,232,240,248 },
	{ 0*256, 1*256, 2*256, 3*256, 4*256, 5*256, 6*256, 7*256,
	  8*256, 9*256,10*256,11*256,12*256,13*256,14*256,15*256,
	 16*256,17*256,18*256,19*256,20*256,21*256,22*256,23*256,
	 24*256,25*256,26*256,27*256,28*256,29*256,30*256,31*256 },
	 32*256
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pgm8_charlayout,    0x800, 32  }, /* 8x8x4 Tiles */
	{ REGION_GFX2, 0, &pgm32_charlayout,   0x400, 32  }, /* 32x32x5 Tiles */
	{ -1 } /* end of array */
};

/*** Machine Driver **********************************************************/

static MACHINE_DRIVER_START( pgm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(pgm_readmem,pgm_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	/* theres also a z80, program is uploaded by the 68k */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 56*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1200/2)

	MDRV_VIDEO_START(pgm)
	MDRV_VIDEO_UPDATE(pgm)
MACHINE_DRIVER_END

/*** Init Stuff **************************************************************/

/* This function expands the 32x32 5-bit data into a format which is easier to
   decode in MAME */

static void expand_32x32x5bpp(void)
{
	data8_t *src    = memory_region       ( REGION_GFX1 );
	data8_t *dst    = memory_region       ( REGION_GFX2 );
	size_t  srcsize = memory_region_length( REGION_GFX1 );
	int cnt, pix;

	for (cnt = 0; cnt < srcsize/5 ; cnt ++)
	{
		pix =  ((src[0+5*cnt] >> 0)& 0x1f );							  dst[0+8*cnt]=pix;
		pix =  ((src[0+5*cnt] >> 5)& 0x07) | ((src[1+5*cnt] << 3) & 0x18);dst[1+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 2)& 0x1f );		 					  dst[2+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 7)& 0x01) | ((src[2+5*cnt] << 1) & 0x1e);dst[3+8*cnt]=pix;
		pix =  ((src[2+5*cnt] >> 4)& 0x0f) | ((src[3+5*cnt] << 4) & 0x10);dst[4+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 1)& 0x1f );							  dst[5+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 6)& 0x03) | ((src[4+5*cnt] << 2) & 0x1c);dst[6+8*cnt]=pix;
		pix =  ((src[4+5*cnt] >> 3)& 0x1f );							  dst[7+8*cnt]=pix;
	}
}

/* This function expands the sprite colour data (in the A Roms) from 3 pixels
   in each word to a byte per pixel making it easier to use */

static void expand_colourdata(void)
{
	data8_t *src    = memory_region       ( REGION_GFX3 );
	data8_t *dst    = memory_region       ( REGION_GFX4 );
	size_t  srcsize = memory_region_length( REGION_GFX3 );
	int cnt;

		for (cnt = 0 ; cnt < srcsize/2 ; cnt++)
		{
			data16_t colpack;

			colpack = ((src[cnt*2]) | (src[cnt*2+1] << 8));

			dst[cnt*3+0] = (colpack >> 0 ) & 0x1f;
			dst[cnt*3+1] = (colpack >> 5 ) & 0x1f;
			dst[cnt*3+2] = (colpack >> 10) & 0x1f;
		}
}

/* Oriental Legend INIT */

static DRIVER_INIT( orlegend )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();

	install_mem_read16_handler(0, 0xC0400e, 0xC0400f, orlegend_prot_r);
	remove_orlegend_prot(); /* removes  bsr 145b66 */
}

static DRIVER_INIT( sango )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();

// 	pgm_p_decrypt();

}


/*** Rom Loading *************************************************************/

/* take note of REGION_GFX2 needed for expanding the 32x32x5bpp data and
   REGION_GFX4 needed for expanding the Sprite Colour Data */

/* The Bios - NOT A GAME */
ROM_START( pgm )
	ROM_REGION( 0x520000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_bios.rom", 0x00000, 0x20000, 0xe42b166e )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* 8x8 Text Layer Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, 0x1a7123a0 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, 0x45ae7159 )
ROM_END

ROM_START( orlegend )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_bios.rom", 0x000000, 0x020000, 0xe42b166e )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0102.rom",    0x100000, 0x200000, 0x4d0f6cc5 )

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, 0x1a7123a0 ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, 0x61425e1e )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, 0x8b3bd88a )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, 0x3b9e9644 )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, 0x069e2c38 )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, 0x4460a3fd )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, 0x5f8abb56 )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, 0xa17a7147 )

	/* 0x1800000/2*3 = 0x2400000
		round this up to 0x4000000 so we can mask .. waste of ram?  */
	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, 0x69d2e48c )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, 0x0d587bf3 )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, 0x43823c1e )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, 0x45ae7159 ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x200000, 0x200000, 0xe5c36c83 )
ROM_END

ROM_START( sango119 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_bios.rom", 0x000000, 0x020000, 0xe42b166e ) // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.119",    0x100000, 0x400000, 0xe4b0875d )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, 0x1a7123a0 ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, 0x4acc1ad6 )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, 0xd8167834 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, 0xff7a4373 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, 0xe7a32959 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0200000, 0x5ce4b5fa )

	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, 0x7d3cd059 )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, 0x0d256ba7 )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, 0x45ae7159 ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, 0x3ada4fd6 )
ROM_END

ROM_START( sango117 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_bios.rom", 0x000000, 0x020000, 0xe42b166e )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.117",    0x100000, 0x400000, 0xc4d19fe6 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, 0x1a7123a0 ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, 0x4acc1ad6 )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, 0xd8167834 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, 0xff7a4373 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, 0xe7a32959 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0200000, 0x5ce4b5fa )

	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, 0x7d3cd059 )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, 0x0d256ba7 )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, 0x45ae7159 ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, 0x3ada4fd6 )
ROM_END

ROM_START( sango115 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_bios.rom", 0x000000, 0x020000, 0xe42b166e )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.115",    0x100000, 0x400000, 0x527a2924 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, 0x1a7123a0 ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, 0x4acc1ad6 )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, 0xd8167834 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, 0xff7a4373 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, 0xe7a32959 ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0200000, 0x5ce4b5fa )

	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* Sprite Colour Data */
	/* Sprite Colour Data is Unpacked Here */

	ROM_REGION( 0x1000000, REGION_GFX5, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, 0x7d3cd059 )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, 0x0d256ba7 )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, 0x45ae7159 ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x200000, 0x400000, 0x3ada4fd6 )
ROM_END

/*** GAME ********************************************************************/

GAMEX( 1997, pgm,      0,          pgm, pgm, 0,          ROT0, "IGS", "PGM (Polygame Master) System BIOS", NOT_A_DRIVER )

GAMEX( 1997, orlegend, pgm,        pgm, pgm, orlegend,   ROT0, "IGS", "Oriental Legend", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND  )
GAMEX( 1999, sango119, pgm,        pgm, pgm, sango,		 ROT0, "IGS", "Sangoku Senki (ver. 119)", GAME_NOT_WORKING )
GAMEX( 1999, sango117, sango119,   pgm, pgm, sango,		 ROT0, "IGS", "Sangoku Senki (ver. 117)", GAME_NOT_WORKING ) // displays 007?
GAMEX( 1999, sango115, sango119,   pgm, pgm, sango,		 ROT0, "IGS", "Sangoku Senki (ver. 115)", GAME_NOT_WORKING ) // displays 006?
