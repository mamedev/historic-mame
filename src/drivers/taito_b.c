/***************************************************************************

Taito B System

heavily based on Taito F2 System driver by Brad Oliver, Andrew Prime


The Taito F2 system is a fairly flexible hardware platform. It supports 4
separate layers of graphics - one 64x64 tiled scrolling background plane
of 8x8 tiles, a similar foreground plane, a sprite plane capable of handling
all the video chores by itself (used in e.g. Super Space Invaders) and a text
plane which may or may not scroll.

Sound is handled by a Z80 with a YM2610 connected to it.

The memory map for each of the games is similar but not identical.

Memory map for Rastan Saga 2 / Nastar / Nastar Warrior

CPU 1 : 68000, uses irqs 2 & 4. One of the IRQs just sets a flag which is
checked in the other IRQ routine. Could be timed to vblank...

  0x000000 - 0x07ffff : ROM
  0x200000 - 0x201fff : palette RAM, 4096 total colors
  0x400000 - 0x403fff : 64x64 foreground layer (offsets 0x0000-0x1fff tile codes; offsets 0x2000-0x3fff tile attributes)
  0x404000 - 0x807fff : 64x64 background layer (offsets 0x0000-0x1fff tile codes; offsets 0x2000-0x3fff tile attributes)
  0x408000 - 0x408fff : 64x64 text layer
  0x410000 - 0x41197f : ??k of sprite RAM (this is the range that Rastan Saga II tests at startup time)
  0x413800 - 0x413fff : foreground control RAM (413800.w - foreground x scroll, 413802.w - foreground y scroll)
  0x413c00 - 0x413fff : background control RAM (413c00.w - background x scroll, 413c02.w - background y scroll)

  0x600000 - 0x607fff : 32k of CPU RAM
  0x800000 - 0x800003 : communication with sound CPU
  0xa00000 - 0xa0000f : input ports and dipswitches (writes may be IRQ acknowledge)




*XXX.1988 Rastan Saga II (B81, , )

Other possible B-Sys games:

Ashura Blaster
Rambo 3
Silent Dragon
Tetris
Violence Fight
Hit The Ice (YM2203 sound - maybe not sysb?)
Master of Weapons (YM2203 sound - maybe not sysb?)


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"

extern unsigned char *taitob_fscroll;
extern unsigned char *taitob_bscroll;
extern unsigned char *b_backgroundram;
extern size_t b_backgroundram_size;
extern unsigned char *b_foregroundram;
extern size_t b_foregroundram_size;
extern unsigned char *b_textram;
extern size_t b_textram_size;
extern size_t b_paletteram_size;
extern unsigned char *taitob_pixelram;


#if 0
extern unsigned char *b_rom;
#endif

int taitob_vh_start (void);
void taitob_vh_stop (void);

READ_HANDLER ( taitob_text_r );
WRITE_HANDLER( taitob_text_w );
READ_HANDLER ( taitob_background_r );
WRITE_HANDLER( taitob_background_w );
READ_HANDLER ( taitob_foreground_r );
WRITE_HANDLER( taitob_foreground_w );
void taitob_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
void crimec_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
void tetrist_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);


WRITE_HANDLER( taitob_video_control_w );


READ_HANDLER( tetrist_pixelram_r );
WRITE_HANDLER( tetrist_pixelram_w );

WRITE_HANDLER( crimec_pixelram_w );


READ_HANDLER ( ssi_videoram_r );
WRITE_HANDLER( ssi_videoram_w );

WRITE_HANDLER( rastan_sound_port_w );
WRITE_HANDLER( rastan_sound_comm_w );
READ_HANDLER ( rastan_sound_comm_r );

WRITE_HANDLER( rastan_a000_w );
WRITE_HANDLER( rastan_a001_w );
READ_HANDLER ( rastan_a001_r );


static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 3;

	cpu_setbank (2, &RAM [0x10000 + (banknum * 0x4000)]);
}



static READ_HANDLER( rastsag2_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport (3)<<8; /*DSW A*/
		case 0x02:
			return readinputport (4)<<8; /*DSW B*/
		case 0x04:
			return readinputport (0)<<8; /*player 1*/
		case 0x06:
			return readinputport (1)<<8; /*player 2*/
		case 0x0e:
			return readinputport (2)<<8; /*tilt, coins*/
		default:
			return 0xff;
	}
}

void rsaga2_interrupt2(int x)
{
	cpu_cause_interrupt(0,MC68000_IRQ_2);
}

static int rastansaga2_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(5000,0),0,rsaga2_interrupt2);
	return MC68000_IRQ_4;
}

void crimec_interrupt3(int x)
{
	cpu_cause_interrupt(0,MC68000_IRQ_3);
}

static int crimec_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(5000,0),0,crimec_interrupt3);
	return MC68000_IRQ_5;
}


static WRITE_HANDLER( taitob_sound_w )
{
	if (offset == 0)
	{
		/*logerror("sound_w off0 data=%04x\n",data);*/
		rastan_sound_port_w (0, (data>>8) & 0xff);
	}
	else if (offset == 2)
	{
		/*logerror("sound_w off2 data=%04x\n",data);*/
		rastan_sound_comm_w (0, (data>>8) & 0xff);
	}
}

static READ_HANDLER( taitob_sound_r )
{
	if (offset == 2)
		return (rastan_sound_comm_r (0)<<8);
	else return 0;
}


static READ_HANDLER( sound_hack_r )
{
	return YM2610_status_port_0_A_r (0) | 1;
}

static struct MemoryReadAddress rastsag2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x600000, 0x607fff, MRA_BANK1 },	/* Main RAM */
	{ 0x200000, 0x201fff, paletteram_word_r }, /*palette*/

	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x408fff, taitob_text_r }, /*text ram*/

	{ 0x410000, 0x41197f, ssi_videoram_r },

	{ 0x413800, 0x413bff, MRA_BANK3 }, //1st.w foreground x, 2nd.w foreground y scroll
	{ 0x413c00, 0x413fff, MRA_BANK4 }, //1st.w background x, 2nd.w background y scroll

	{ 0xa00000, 0xa0000f, rastsag2_input_r },	/* DSW A/B, player inputs*/
	{ 0x800000, 0x800003, taitob_sound_r },
	{ -1 }  /* end of table */
};

#if 0
//patched
static struct MemoryReadAddress rastsag2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x600000, 0x607fff, MRA_BANK1 },	/* Main RAM */
	{ 0x200000, 0x201fff, MRA_NOP }, /*palette*/
	{ 0x400000, 0x403fff, MRA_NOP },
	{ 0x404000, 0x407fff, MRA_NOP },
	{ 0x408000, 0x408fff, MRA_NOP }, /*text ram*/
	{ 0x410000, 0x41197f, MRA_NOP },
	{ 0x413800, 0x413bff, MRA_NOP }, //1st.w foreground x, 2nd.w foreground y scroll
	{ 0x413c00, 0x413fff, MRA_NOP }, //1st.w background x, 2nd.w background y scroll
	{ 0xa00000, 0xa0000f, rastsag2_input_r },	/* DSW A/B, player inputs*/
	{ 0x800000, 0x800003, taitob_sound_r },
	{ -1 }  /* end of table */
};
#endif


static struct MemoryWriteAddress rastsag2_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x600000, 0x607fff, MWA_BANK1 },	/* Main RAM */
	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },

	{ 0x400000, 0x403fff, taitob_foreground_w, &b_foregroundram, &b_foregroundram_size }, /* foreground layer */
	{ 0x404000, 0x407fff, taitob_background_w, &b_backgroundram, &b_backgroundram_size }, /* background layer */
	{ 0x408000, 0x408fff, taitob_text_w, &b_textram, &b_textram_size }, /* text layer */

	{ 0x410000, 0x41197f, ssi_videoram_w, &videoram, &videoram_size  },

	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, //1st.w foreground x scroll, 2nd.w foreground y scroll
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, //1st.w background x scroll, 2nd.w background y scroll

	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0xa00000, 0xa0000f, MWA_NOP }, // ??
	{ 0x800000, 0x800003, taitob_sound_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress crimec_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0xa00000, 0xa0ffff, MRA_BANK1 },	/* Main RAM */
	{ 0x800000, 0x801fff, paletteram_word_r }, /*palette*/

	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x408fff, taitob_text_r }, /*text ram*/

	{ 0x410000, 0x41197f, ssi_videoram_r },

	{ 0x413800, 0x413bff, MRA_BANK3 }, //1st.w foreground x, 2nd.w foreground y scroll
	{ 0x413c00, 0x413fff, MRA_BANK4 }, //1st.w background x, 2nd.w background y scroll

	{ 0x200000, 0x20000f, rastsag2_input_r },	/* DSW A/B, player inputs*/
	{ 0x600000, 0x600003, taitob_sound_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress crimec_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0xa00000, 0xa0ffff, MWA_BANK1 },	/* Main RAM */
	{ 0x800000, 0x801fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w, &b_foregroundram, &b_foregroundram_size }, /* foreground layer */
	{ 0x404000, 0x407fff, taitob_background_w, &b_backgroundram, &b_backgroundram_size }, /* background layer */
	{ 0x408000, 0x408fff, taitob_text_w, &b_textram, &b_textram_size }, /* text layer */

	{ 0x409000, 0x40ffff, MWA_NOP }, /* unused (just set to zero at startup), not read by the game */
	{ 0x410000, 0x41197f, ssi_videoram_w, &videoram, &videoram_size  },

	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, //1st.w foreground x scroll, 2nd.w foreground y scroll
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, //1st.w background x scroll, 2nd.w background y scroll

	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0x440000, 0x47ffff, crimec_pixelram_w, &taitob_pixelram }, /* pixel layer */

	{ 0x200000, 0x20000f, MWA_NOP }, /**/
	{ 0x600000, 0x600003, taitob_sound_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress tetrist_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x800000, 0x807fff, MRA_BANK1 },	/* Main RAM */
	{ 0xa00000, 0xa01fff, paletteram_word_r }, /*palette*/

	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x408fff, taitob_text_r }, /*text ram*/

	{ 0x440000, 0x47ffff, tetrist_pixelram_r },	/* Pixel Layer */

	{ 0x410000, 0x41197f, ssi_videoram_r },

	{ 0x413800, 0x413bff, MRA_BANK3 }, //1st.w foreground x, 2nd.w foreground y scroll
	{ 0x413c00, 0x413fff, MRA_BANK4 }, //1st.w background x, 2nd.w background y scroll

	{ 0x600000, 0x60000f, rastsag2_input_r },	/* DSW A/B, player inputs*/
	{ 0x200000, 0x200003, taitob_sound_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress tetrist_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x800000, 0x807fff, MWA_BANK1 },	/* Main RAM */
	{ 0xa00000, 0xa01fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },

	{ 0x400000, 0x403fff, taitob_foreground_w, &b_foregroundram, &b_foregroundram_size }, /* foreground layer */
	{ 0x404000, 0x407fff, taitob_background_w, &b_backgroundram, &b_backgroundram_size }, /* background layer */
	{ 0x408000, 0x408fff, taitob_text_w, &b_textram, &b_textram_size }, /* text layer */

	{ 0x440000, 0x47ffff, tetrist_pixelram_w, &taitob_pixelram }, /* pixel layer */

	{ 0x410000, 0x41197f, ssi_videoram_w, &videoram, &videoram_size  },

	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, //1st.w foreground x scroll, 2nd.w foreground y scroll
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, //1st.w background x scroll, 2nd.w background y scroll

	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0x600000, 0x60000f, MWA_NOP }, // ??
	{ 0x200000, 0x200003, taitob_sound_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, sound_hack_r },
//	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, rastan_a001_r },
	{ 0xea00, 0xea00, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, rastan_a000_w },
	{ 0xe201, 0xe201, rastan_a001_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xe600, 0xe600, MWA_NOP }, /* ? */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, bankswitch_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( rastsag2 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1, 2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

INPUT_PORTS_END

INPUT_PORTS_START( crimec )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1, 2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

INPUT_PORTS_END

INPUT_PORTS_START( tetrist )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1, 2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

INPUT_PORTS_END

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
	8192,	/* 8192 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 8 , 512*1024*8 , 512*1024*8+8},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 8 , 512*1024*8 , 512*1024*8+8},
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &tilelayout,  0, 256 },  /* sprites & playfield */
	{ -1 } /* end of array */
};

#define ROM_S 128*1024
static struct GfxLayout charlayout_tetrist =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	8,	/* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /*bitplanes are _not_ separated*/
	{ 0, 8 , 128*1024*8 , 128*1024*8+8},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxDecodeInfo tetrist_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0, &charlayout_tetrist,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &charlayout_tetrist,  0, 256 },  /* sprites & playfield */
	{ -1 } /* end of array */
};


/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2610interface ym2610_interface_rsaga2 =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct YM2610interface ym2610_interface_crimec =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct MachineDriver machine_driver_rastsag2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz */
			rastsag2_readmem,rastsag2_writemem,0,0,
			rastansaga2_interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz */
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start,
	taitob_vh_stop,
	taitob_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface_rsaga2
		}
	}
};

static struct MachineDriver machine_driver_crimec =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz */
			crimec_readmem,crimec_writemem,0,0,
			crimec_interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz */
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start,
	taitob_vh_stop,
	crimec_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface_crimec
		}
	}
};

static struct MachineDriver machine_driver_tetrist =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ???*/
			tetrist_readmem,tetrist_writemem,0,0,
			rastansaga2_interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz */
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },
	/*64*8, 64*8, { 0*8, 64*8-1, 0*8, 64*8-1 },*/

	tetrist_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start,
	taitob_vh_stop,
	tetrist_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface_rsaga2
		}
	}
};

static struct MachineDriver machine_driver_ashura =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz */
			rastsag2_readmem,rastsag2_writemem,0,0,
			rastansaga2_interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz */
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start,
	taitob_vh_stop,
	taitob_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface_crimec
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rastsag2 )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "b81-08.50" , 0x00000, 0x20000, 0xd6da9169 )
	ROM_LOAD_ODD ( "b81-07.bin", 0x00000, 0x20000, 0x8edf17d7 )
	ROM_LOAD_EVEN( "b81-10.49" , 0x40000, 0x20000, 0x53f34344 )
	ROM_LOAD_ODD ( "b81-09.30" , 0x40000, 0x20000, 0x630d34af )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "b81-11.bin", 0x00000, 0x4000, 0x3704bf09 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b81-03.bin", 0x000000, 0x080000, 0x551b75e6 )
	ROM_LOAD( "b81-04.bin", 0x080000, 0x080000, 0xcf734e12 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "b81-01.bin", 0x00000, 0x80000, 0xb33f796b )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* DELTA-T samples */
	ROM_LOAD( "b81-02.bin", 0x00000, 0x80000, 0x20ec3b86 )
ROM_END

ROM_START( nastarw )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "b81-08.50" , 0x00000, 0x20000, 0xd6da9169 )
	ROM_LOAD_ODD ( "b81-12.31",  0x00000, 0x20000, 0xf9d82741 )
	ROM_LOAD_EVEN( "b81-10.49" , 0x40000, 0x20000, 0x53f34344 )
	ROM_LOAD_ODD ( "b81-09.30" , 0x40000, 0x20000, 0x630d34af )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "b81-11.bin", 0x00000, 0x4000, 0x3704bf09 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b81-03.bin", 0x000000, 0x080000, 0x551b75e6 )
	ROM_LOAD( "b81-04.bin", 0x080000, 0x080000, 0xcf734e12 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "b81-01.bin", 0x00000, 0x80000, 0xb33f796b )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* DELTA-T samples */
	ROM_LOAD( "b81-02.bin", 0x00000, 0x80000, 0x20ec3b86 )
ROM_END

ROM_START( nastar )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "b81-08.50" , 0x00000, 0x20000, 0xd6da9169 )
	ROM_LOAD_ODD ( "b81-13.bin", 0x00000, 0x20000, 0x60d176fb )
	ROM_LOAD_EVEN( "b81-10.49" , 0x40000, 0x20000, 0x53f34344 )
	ROM_LOAD_ODD ( "b81-09.30" , 0x40000, 0x20000, 0x630d34af )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "b81-11.bin", 0x00000, 0x4000, 0x3704bf09 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b81-03.bin", 0x000000, 0x080000, 0x551b75e6 )
	ROM_LOAD( "b81-04.bin", 0x080000, 0x080000, 0xcf734e12 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "b81-01.bin", 0x00000, 0x80000, 0xb33f796b )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* DELTA-T samples */
	ROM_LOAD( "b81-02.bin", 0x00000, 0x80000, 0x20ec3b86 )
ROM_END

ROM_START( crimecu )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "b99-07"   , 0x00000, 0x20000, 0x26e886e6 )
	ROM_LOAD_ODD ( "b99-05"   , 0x00000, 0x20000, 0xff7f9a9d )
	ROM_LOAD_EVEN( "b99-06"   , 0x40000, 0x20000, 0x1f26aa92 )
	ROM_LOAD_ODD ( "b99_14-2" , 0x40000, 0x20000, 0x06cf8441 )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "b99-08", 0x00000, 0x4000, 0x26135451 )
	ROM_CONTINUE(       0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b99-02.ch1", 0x000000, 0x080000, 0x2a5d4a26 )
	ROM_LOAD( "b99-01.ch0", 0x080000, 0x080000, 0xa19e373a )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "b99-03.roa", 0x00000, 0x80000, 0xdda10df7 )
ROM_END

ROM_START( crimec )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "b99-07"   , 0x00000, 0x20000, 0x26e886e6 )
	ROM_LOAD_ODD ( "b99-05"   , 0x00000, 0x20000, 0xff7f9a9d )
	ROM_LOAD_EVEN( "b99-06"   , 0x40000, 0x20000, 0x1f26aa92 )
	ROM_LOAD_ODD ( "b99-14"   , 0x40000, 0x20000, 0x71c8b4d7 )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "b99-08", 0x00000, 0x4000, 0x26135451 )
	ROM_CONTINUE(       0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b99-02.ch1", 0x000000, 0x080000, 0x2a5d4a26 )
	ROM_LOAD( "b99-01.ch0", 0x080000, 0x080000, 0xa19e373a )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "b99-03.roa", 0x00000, 0x80000, 0xdda10df7 )
ROM_END

ROM_START( crimecj )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "b99-07"   , 0x00000, 0x20000, 0x26e886e6 )
	ROM_LOAD_ODD ( "b99-05"   , 0x00000, 0x20000, 0xff7f9a9d )
	ROM_LOAD_EVEN( "b99-06"   , 0x40000, 0x20000, 0x1f26aa92 )
	ROM_LOAD_ODD ( "15"       , 0x40000, 0x20000, 0xe8c1e56d )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "b99-08", 0x00000, 0x4000, 0x26135451 )
	ROM_CONTINUE(       0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b99-02.ch1", 0x000000, 0x080000, 0x2a5d4a26 )
	ROM_LOAD( "b99-01.ch0", 0x080000, 0x080000, 0xa19e373a )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "b99-03.roa", 0x00000, 0x80000, 0xdda10df7 )
ROM_END

ROM_START( ashura )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "c4307-1.50"   , 0x00000, 0x20000, 0xd5ceb20f )
	ROM_LOAD_ODD ( "c4305-1.31"   , 0x00000, 0x20000, 0xa6f3bb37 )
	ROM_LOAD_EVEN( "c4306-1.49"   , 0x40000, 0x20000, 0x0f331802 )
	ROM_LOAD_ODD ( "c4304-1.30"   , 0x40000, 0x20000, 0xe06a2414 )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "ashura37.bin", 0x00000, 0x4000, 0xcb26fce1 )
	ROM_CONTINUE(             0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ashura1.bin", 0x000000, 0x080000, 0x105722ae )
	ROM_LOAD( "ashura0.bin", 0x080000, 0x080000, 0x426606ba )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "ashura2.bin", 0x00000, 0x80000, 0xdb953f37 )

//other sound region?

ROM_END

ROM_START( tetrist )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "c12-03.bin", 0x000000, 0x020000, 0x38f1ed41 )
	ROM_LOAD_ODD ( "c12-02.bin", 0x000000, 0x020000, 0xed9530bc )
	ROM_LOAD_EVEN( "c12-05.bin", 0x040000, 0x020000, 0x128e9927 )
	ROM_LOAD_ODD ( "c12-04.bin", 0x040000, 0x020000, 0x5da7a319 )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "c12-06.bin", 0x00000, 0x4000, 0xf2814b38 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x40000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c12-04.bin", 0x000000, 0x020000, 0x5da7a319 )
	ROM_LOAD( "c12-05.bin", 0x020000, 0x020000, 0x128e9927 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	/* empty */

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* DELTA-T samples */
	/* empty */
ROM_END



GAMEX( 1988, nastar,   0,      rastsag2, rastsag2, 0, ROT0,       "Taito Corporation Japan", "Nastar (World)", GAME_NO_COCKTAIL)
GAMEX( 1988, nastarw,  nastar, rastsag2, rastsag2, 0, ROT0,       "Taito America Corporation", "Nastar Warrior (US)", GAME_NO_COCKTAIL)
GAMEX( 1988, rastsag2, nastar, rastsag2, rastsag2, 0, ROT0,       "Taito Corporation", "Rastan Saga 2 (Japan)", GAME_NO_COCKTAIL)
GAMEX( 1989, crimec,   0,      crimec,   crimec,   0, ROT0_16BIT, "Taito Corporation Japan", "Crime City (World)", GAME_NO_COCKTAIL)
GAMEX( 1989, crimecu,  crimec, crimec,   crimec,   0, ROT0_16BIT, "Taito America Corporation", "Crime City (US)", GAME_NO_COCKTAIL)
GAMEX( 1989, crimecj,  crimec, crimec,   crimec,   0, ROT0_16BIT, "Taito Corporation", "Crime City (Japan)", GAME_NO_COCKTAIL)
GAMEX( 1989, tetrist,  tetris, tetrist,  tetrist,  0, ROT0_16BIT, "Sega", "Tetris (Japan, B-System)", GAME_NO_COCKTAIL)

GAMEX( 1990, ashura,   0,      ashura,   rastsag2, 0, ROT270,     "Taito Corporation", "Ashura Blaster (Japan)", GAME_NO_COCKTAIL)
