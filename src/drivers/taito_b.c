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
  0x200000 - 0x201fff : palette RAM, 4096 total colors (0x1000 words)
  0x400000 - 0x403fff : 64x64 foreground layer (offsets 0x0000-0x1fff tile codes; offsets 0x2000-0x3fff tile attributes)
  0x404000 - 0x807fff : 64x64 background layer (offsets 0x0000-0x1fff tile codes; offsets 0x2000-0x3fff tile attributes)
  0x408000 - 0x408fff : 64x64 text layer
  0x410000 - 0x41197f : ??k of sprite RAM (this is the range that Rastan Saga II tests at startup time)
  0x413800 - 0x413bff : foreground control RAM (413800.w - foreground x scroll, 413802.w - foreground y scroll)
  0x413c00 - 0x413fff : background control RAM (413c00.w - background x scroll, 413c02.w - background y scroll)

  0x600000 - 0x607fff : 32k of CPU RAM
  0x800000 - 0x800003 : communication with sound CPU
  0xa00000 - 0xa0000f : input ports and dipswitches (writes may be IRQ acknowledge)




*XXX.1988 Rastan Saga II (B81, , )

Other possible B-Sys games:

+Ashura Blaster
+Crime City
+Rambo 3
Silent Dragon
+Tetris
Violence Fight
+Hit The Ice (YM2203 sound - maybe not sysb?)
Master of Weapons (YM2203 sound - maybe not sysb?)
+Puzzle Bobble

+ means emulated

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"


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



/*TileMaps*/
int  taitob_vh_start_tm (void);
void taitob_vh_stop (void);
int taitob_vh_start_reversed_colors_tm (void); /*for crime city and puzzle bobble*/
void taitob_vh_screenrefresh_tm (struct osd_bitmap *bitmap,int full_refresh);
void ashura_vh_screenrefresh_tm (struct osd_bitmap *bitmap,int full_refresh);
void crimec_vh_screenrefresh_tm (struct osd_bitmap *bitmap,int full_refresh);
void tetrist_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
void hitice_vh_screenrefresh_tm (struct osd_bitmap *bitmap,int full_refresh);
void rambo3_vh_screenrefresh_tm (struct osd_bitmap *bitmap,int full_refresh);
void puzbobb_vh_screenrefresh_tm (struct osd_bitmap *bitmap,int full_refresh);
void qzshowby_vh_screenrefresh_tm (struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( taitob_text_w_tm );
WRITE_HANDLER( taitob_background_w_tm );
WRITE_HANDLER( taitob_foreground_w_tm );
READ_HANDLER ( taitob_text_r );
READ_HANDLER ( taitob_background_r );
READ_HANDLER ( taitob_foreground_r );
/*TileMaps end*/



READ_HANDLER ( taitob_pixelram_r );
WRITE_HANDLER( taitob_pixelram_w );

READ_HANDLER ( taitob_text_video_control_r );
WRITE_HANDLER( taitob_text_video_control_w );
READ_HANDLER ( taitob_video_control_r );
WRITE_HANDLER( taitob_video_control_w );


READ_HANDLER( hitice_pixelram_r );
WRITE_HANDLER( hitice_pixelram_w );/*this doesn't look like a pixel layer*/


READ_HANDLER ( taitob_videoram_r );
WRITE_HANDLER( taitob_videoram_w );

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
            logerror("WARNING: read input offs=%2x PC=%08x\n", offset, cpu_get_pc());
			return 0xff;
	}
}

static READ_HANDLER( hitice_input_r )
{
	return ( readinputport (5)<<8 | readinputport(6) ); /*player 3 and 4*/
}


static READ_HANDLER( eeprom_r );

static READ_HANDLER( puzbobb_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport (3)<<8; /*DSW A*/
		case 0x02:
			return ( eeprom_r(0) ) << 8; /*bit 0 - Eeprom data, other coin inputs*/
		case 0x04:
			return readinputport (0)<<8; /*player 1*/ /*tilt*/
		case 0x06:
			return readinputport (1)<<8; /*player 2*/
		case 0x08:
			return readinputport (5)<<8; /* ??? */
		case 0x0e:
			return readinputport (2)<<8; /*tilt, coins*/
		default:
            logerror("WARNING: puzbobb read input offs=%x\n",offset);
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


void hitice_interrupt6(int x)
{
	cpu_cause_interrupt(0,MC68000_IRQ_6);
}

static int hitice_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(5000,0),0,hitice_interrupt6);
	return MC68000_IRQ_4;
}


void rambo3_interrupt1(int x)
{
	cpu_cause_interrupt(0,MC68000_IRQ_1);
}

static int rambo3_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(5000,0),0,rambo3_interrupt1);
	return MC68000_IRQ_6;
}


void puzbobb_interrupt5(int x)
{
	cpu_cause_interrupt(0,MC68000_IRQ_5);
}

static int puzbobb_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(5000,0),0,puzbobb_interrupt5);
	return MC68000_IRQ_3;
}


static WRITE_HANDLER( taitob_sound_w )
{
	if (offset == 0)
	{
		rastan_sound_port_w(0, (data>>8) & 0xff);
	}
	else if (offset == 2)
	{
		rastan_sound_comm_w(0, (data>>8) & 0xff);
	}
}

static READ_HANDLER( taitob_sound_r )
{
	if (offset == 2)
		return (rastan_sound_comm_r(0)<<8 );
	else return 0;
}


static struct MemoryReadAddress rastsag2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x600000, 0x607fff, MRA_BANK1 },			/* Main RAM */
	{ 0x200000, 0x201fff, paletteram_word_r },	/*palette*/

	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x408fff, taitob_text_r },		/*text ram*/
	{ 0x409000, 0x409fff, MRA_BANK5 },			/*ashura only (textram continue ?)*/
	{ 0x410000, 0x41197f, taitob_videoram_r },		/*sprite ram*/
	{ 0x411980, 0x411fff, MRA_BANK6 },			/*ashura only (spriteram continue ?)*/

	{ 0x413800, /*0x413803*/ 0x413bff, MRA_BANK3 }, /*1st.w foreground x, 2nd.w foreground y scroll*/
	{ 0x413c00, /*0x413c03*/ 0x413fff, MRA_BANK4 }, /*1st.w background x, 2nd.w background y scroll*/
	{ 0x41800e, 0x41800f, taitob_video_control_r },

	{ 0xa00000, 0xa0000f, rastsag2_input_r },	/* DSW A/B, player inputs*/
	{ 0x800000, 0x800003, taitob_sound_r },
	{ -1 }  /* end of table */
};

#if 0
static WRITE_HANDLER( taitob_input_w )
{
static int aa[256];

	if (aa[offset] != data)
		logerror("control_w = %4x  ofs=%2x\n",data,offset);
	aa[offset] = data;
}
#endif

static struct MemoryWriteAddress rastsag2_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x600000, 0x607fff, MWA_BANK1 },	/* Main RAM */ /*ashura up to 603fff only*/

	{ 0x200000, 0x201fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w_tm, &b_foregroundram, &b_foregroundram_size }, /* foreground layer */
	{ 0x404000, 0x407fff, taitob_background_w_tm, &b_backgroundram, &b_backgroundram_size }, /* background layer */
	{ 0x408000, 0x408fff, taitob_text_w_tm, &b_textram, &b_textram_size }, /* text layer */
	{ 0x409000, 0x409fff, MWA_BANK5 }, /*ashura clears this area only*/

	{ 0x410000, 0x41197f, taitob_videoram_w, &videoram, &videoram_size  },
	{ 0x411980, 0x411fff, MWA_BANK6 }, /*ashura clears this area only*/

	{ 0x413800, /*0x413803*/ 0x413bff, MWA_BANK3, &taitob_fscroll }, /*1st.w foreground x scroll, 2nd.w foreground y scroll */
	{ 0x413c00, /*0x413c03*/ 0x413fff, MWA_BANK4, &taitob_bscroll }, /*1st.w background x scroll, 2nd.w background y scroll */

	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0x440000, 0x47ffff, taitob_pixelram_w, &taitob_pixelram }, /* ashura(US) pixel layer*/

	{ 0xa00000, 0xa0000f, MWA_NOP }, // ??
	{ 0x800000, 0x800003, taitob_sound_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress crimec_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0xa00000, 0xa0ffff, MRA_BANK1 },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram_word_r },
	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x408fff, taitob_text_r },
	{ 0x410000, 0x41197f, taitob_videoram_r },

	{ 0x413800, 0x413bff, MRA_BANK3 }, /*1st.w foreground x, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MRA_BANK4 }, /*1st.w background x, 2nd.w background y scroll*/

	{ 0x200000, 0x20000f, rastsag2_input_r },	/* DSW A/B, player inputs*/
	{ 0x600000, 0x600003, taitob_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress crimec_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0xa00000, 0xa0ffff, MWA_BANK1 },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w_tm, &b_foregroundram, &b_foregroundram_size },
	{ 0x404000, 0x407fff, taitob_background_w_tm, &b_backgroundram, &b_backgroundram_size },
	{ 0x408000, 0x408fff, taitob_text_w_tm, &b_textram, &b_textram_size },
	{ 0x409000, 0x40ffff, MWA_NOP }, /* unused (just set to zero at startup), not read by the game */
	{ 0x410000, 0x41197f, taitob_videoram_w, &videoram, &videoram_size  },

	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, /*1st.w foreground x scroll, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, /*1st.w background x scroll, 2nd.w background y scroll*/

	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0x440000, 0x47ffff, taitob_pixelram_w, &taitob_pixelram }, /* pixel layer */

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
	{ 0x440000, 0x47ffff, taitob_pixelram_r },	/* Pixel Layer */
	{ 0x410000, 0x41197f, taitob_videoram_r },

	{ 0x413800, 0x413bff, MRA_BANK3 }, /*1st.w foreground x, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MRA_BANK4 }, /*1st.w background x, 2nd.w background y scroll*/

	{ 0x600000, 0x60000f, rastsag2_input_r },	/* DSW A/B, player inputs*/
	{ 0x200000, 0x200003, taitob_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress tetrist_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x800000, 0x807fff, MWA_BANK1 },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w_tm, &b_foregroundram, &b_foregroundram_size },
	{ 0x404000, 0x407fff, taitob_background_w_tm, &b_backgroundram, &b_backgroundram_size },
	{ 0x408000, 0x408fff, taitob_text_w_tm, &b_textram, &b_textram_size },
	{ 0x440000, 0x47ffff, taitob_pixelram_w, &taitob_pixelram }, /* pixel layer */
	{ 0x410000, 0x41197f, taitob_videoram_w, &videoram, &videoram_size  },

	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, /*1st.w foreground x scroll, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, /*1st.w background x scroll, 2nd.w background y scroll*/

	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0x600000, 0x60000f, MWA_NOP }, // ??
	{ 0x200000, 0x200003, taitob_sound_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress hitice_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x800000, 0x803fff, MRA_BANK1 },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram_word_r },
	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x40bfff, taitob_text_r },
	{ 0xb00000, 0xb7ffff, hitice_pixelram_r },	/* Pixel Layer ???????????? */
	{ 0x410000, 0x411fff /*97f*/, taitob_videoram_r },
	{ 0x413800, 0x413bff, MRA_BANK3 }, /*1st.w foreground x, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MRA_BANK4 }, /*1st.w background x, 2nd.w background y scroll*/

	{ 0x41800e, 0x41800f, taitob_video_control_r },

	{ 0x600000, 0x60000f, rastsag2_input_r },	/* DSW A/B, player 1,2 inputs*/
	{ 0x610000, 0x610001, hitice_input_r },		/* player 3,4 inputs*/

	{ 0x700000, 0x700003, taitob_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress hitice_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x800000, 0x803fff, MWA_BANK1 },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w_tm, &b_foregroundram, &b_foregroundram_size },
	{ 0x404000, 0x407fff, taitob_background_w_tm, &b_backgroundram, &b_backgroundram_size },
	{ 0x408000, 0x40bfff, taitob_text_w_tm, &b_textram, &b_textram_size },
	{ 0x410000, 0x411fff /*97f*/, taitob_videoram_w, &videoram, &videoram_size  },

	//{ 0x411980, 0x411fff, MWA_BANK6 }, /*ashura and hitice*/

	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, /*1st.w foreground x scroll, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, /*1st.w background x scroll, 2nd.w background y scroll*/

	{ 0x41800c, 0x41800d, taitob_text_video_control_w },
	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0xb00000, 0xb7ffff, hitice_pixelram_w, &taitob_pixelram }, /* pixel layer ????????*/

	{ 0x600000, 0x60000f, MWA_NOP }, // ??
	{ 0x700000, 0x700003, taitob_sound_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress rambo3_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x800000, 0x803fff, MRA_BANK1 },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram_word_r },
	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x40bfff, taitob_text_r },
	{ 0x410000, 0x411fff /*97f*/, taitob_videoram_r },
	{ 0x413800, 0x413bff, MRA_BANK3 }, /*1st.w foreground x, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MRA_BANK4 }, /*1st.w background x, 2nd.w background y scroll*/

	{ 0x41800c, 0x41800d, taitob_text_video_control_r },
	{ 0x41800e, 0x41800f, taitob_video_control_r },

	{ 0x600000, 0x60000f, rastsag2_input_r },	/* DSW A/B, player 1,2 inputs*/
	{ 0x200000, 0x200003, taitob_sound_r },
	{ -1 }  /* end of table */
};

#if 0
static WRITE_HANDLER( rambo3_control_w )
{
static int aa[256];

	if (aa[offset] != data)
		usrintf_showmessage("control_w = %4x  ofs=%2x\n",data,offset);
	aa[offset] = data;
}
#endif

static struct MemoryWriteAddress rambo3_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x800000, 0x803fff, MWA_BANK1 },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w_tm, &b_foregroundram, &b_foregroundram_size },
	{ 0x404000, 0x407fff, taitob_background_w_tm, &b_backgroundram, &b_backgroundram_size },
	{ 0x408000, 0x40bfff, taitob_text_w_tm, &b_textram, &b_textram_size },
	{ 0x410000, 0x411fff /*97f*/, taitob_videoram_w, &videoram, &videoram_size  },
	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, /*1st.w foreground x scroll, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, /*1st.w background x scroll, 2nd.w background y scroll*/

	{ 0x41800c, 0x41800d, taitob_text_video_control_w },
	{ 0x41800e, 0x41800f, taitob_video_control_w },

	{ 0x600000, 0x60000f, MWA_NOP }, // ??
	{ 0x200000, 0x200003, taitob_sound_w },
	{ -1 }  /* end of table */
};



/***************************************************************************

  Puzzle Bobble EEPROM

***************************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/*  read command */
	"0101",			/* write command */
	0,				/* erase command */
	"0100000000",	/*  lock command */
	"0100110000" 	/* unlock command*/
};

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file)
		{
			EEPROM_load(file);
		}
	}
}

static READ_HANDLER( eeprom_r )
{
	int res;

	res = EEPROM_read_bit() & 0x01;
	res |= readinputport( 4 ) & 0xfe; /* coin inputs */

	return res;
}


static WRITE_HANDLER( eeprom_w )
{
	data >>= 8; /*M68k byte write*/

	/* bit 0 - Unused */
	/* bit 1 - Unused */
	/* bit 2 - Eeprom data  */
	/* bit 3 - Eeprom clock */
	/* bit 4 - Eeprom reset (active low) */
	/* bit 5 - Unused */
	/* bit 6 - Unused */
	/* bit 7 - set all the time (Chip Select?) */

	/* EEPROM */
	EEPROM_write_bit(data & 0x04);
	EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
}



READ_HANDLER( p_read )
{
	logerror("puzzle_read off%x\n",offset);
	return 0xffff;
}

WRITE_HANDLER( p_write )
{
	logerror("puzzle_write off%2x data=%8x   pc=%8x\n",offset,data, cpu_get_pc());
}

static struct MemoryReadAddress puzbobb_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x900000, 0x90ffff, MRA_BANK1 },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram_word_r },
	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x40bfff, taitob_text_r },
	{ 0x410000, 0x41197f, taitob_videoram_r },
	{ 0x413800, 0x413bff, MRA_BANK3 }, /*1st.w foreground x, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MRA_BANK4 }, /*1st.w background x, 2nd.w background y scroll*/

	{ 0x500010, 0x50002f, p_read }, //????

	{ 0x500000, 0x50000f, puzbobb_input_r },	/* DSW A/B, player inputs*/
	{ 0x700000, 0x700003, taitob_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress puzbobb_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x900000, 0x90ffff, MWA_BANK1 },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w_tm, &b_foregroundram, &b_foregroundram_size },
	{ 0x404000, 0x407fff, taitob_background_w_tm, &b_backgroundram, &b_backgroundram_size },
	{ 0x408000, 0x40bfff, taitob_text_w_tm, &b_textram, &b_textram_size },
	{ 0x410000, 0x41197f, taitob_videoram_w, &videoram, &videoram_size  },
	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, /*1st.w foreground x scroll, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, /*1st.w background x scroll, 2nd.w background y scroll*/

	{ 0x418000, 0x41800d, MWA_NOP }, //temporarily disabled
	{ 0x41800e, 0x41800f, taitob_video_control_w },
	{ 0x418014, 0x418017, MWA_NOP }, //temporarily disabled

	{ 0x500028, 0x50002f, p_write }, //?????

	{ 0x500026, 0x500027, eeprom_w },
	{ 0x500000, 0x500001, MWA_NOP }, /*lots of zero writes here - watchdog ?*/
	{ 0x700000, 0x700003, taitob_sound_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress qzshowby_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x900000, 0x90ffff, MRA_BANK1 },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram_word_r },
	{ 0x400000, 0x403fff, taitob_foreground_r },
	{ 0x404000, 0x407fff, taitob_background_r },
	{ 0x408000, 0x40bfff, taitob_text_r },
	{ 0x410000, 0x41197f, taitob_videoram_r },
	{ 0x413800, 0x413bff, MRA_BANK3 }, /*1st.w foreground x, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MRA_BANK4 }, /*1st.w background x, 2nd.w background y scroll*/

//{ 0x200010, 0x20002f, p_read }, //????

	{ 0x200000, 0x20000f, puzbobb_input_r },	/* DSW A/B, player inputs*/
	{ 0x600000, 0x600003, taitob_sound_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress qzshowby_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x900000, 0x90ffff, MWA_BANK1 },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram, &b_paletteram_size },
	{ 0x400000, 0x403fff, taitob_foreground_w_tm, &b_foregroundram, &b_foregroundram_size },
	{ 0x404000, 0x407fff, taitob_background_w_tm, &b_backgroundram, &b_backgroundram_size },
	{ 0x408000, 0x40bfff, taitob_text_w_tm, &b_textram, &b_textram_size },
	{ 0x410000, 0x41197f, taitob_videoram_w, &videoram, &videoram_size  },
	{ 0x413800, 0x413bff, MWA_BANK3, &taitob_fscroll }, /*1st.w foreground x scroll, 2nd.w foreground y scroll*/
	{ 0x413c00, 0x413fff, MWA_BANK4, &taitob_bscroll }, /*1st.w background x scroll, 2nd.w background y scroll*/

	{ 0x418000, 0x41800d, MWA_NOP }, //temporarily disabled
	{ 0x41800e, 0x41800f, taitob_video_control_w },
	{ 0x418014, 0x418017, MWA_NOP }, //temporarily disabled

//{ 0x200028, 0x20002f, p_write }, //?????

	{ 0x200026, 0x200027, eeprom_w },
	{ 0x200000, 0x200001, MWA_NOP }, /*lots of zero writes here - watchdog ?*/
	{ 0x600000, 0x600003, taitob_sound_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
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

static struct MemoryReadAddress hitice_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa001, 0xa001, rastan_a001_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress hitice_sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xb000, 0xb000, OKIM6295_data_0_w },
	{ 0xb001, 0xb001, OKIM6295_data_1_w },
	{ 0xa000, 0xa000, rastan_a000_w },
	{ 0xa001, 0xa001, rastan_a001_w },
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
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x0c, "100000 Only" )
    PORT_DIPSETTING(    0x08, "150000 Only" )
    PORT_DIPSETTING(    0x04, "200000 Only" )
    PORT_DIPSETTING(    0x00, "250000 Only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
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
    PORT_DIPNAME( 0x01, 0x01, "Hi Score" )
    PORT_DIPSETTING(    0x01, "Scribble" )
    PORT_DIPSETTING(    0x00, "3 Characters" )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x08, "Every 80000" )
    PORT_DIPSETTING(    0x0c, "80000 Only" )
    PORT_DIPSETTING(    0x04, "160000 Only" )
    PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x10, "1" )
    PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPNAME( 0xc0, 0x40, "Allow Continue" )
    PORT_DIPSETTING(    0x80, DEF_STR( No ) )
    PORT_DIPSETTING(    0xc0, "5 Times" )
    PORT_DIPSETTING(    0x00, "8 Times" )
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )

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
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ))
    PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

INPUT_PORTS_END

INPUT_PORTS_START( ashura )
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
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
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
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x08, "Every 100000" )
    PORT_DIPSETTING(    0x0c, "Every 150000" )
    PORT_DIPSETTING(    0x04, "Every 200000" )
    PORT_DIPSETTING(    0x00, "Every 250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x00, "1" )
    PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

INPUT_PORTS_END


INPUT_PORTS_START( hitice )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT(         0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1, 2 )
	PORT_BIT(         0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(         0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT(         0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(         0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

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

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START      /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

INPUT_PORTS_END



/* Helps document the input ports. */

#define PORT_SERVICE_NO_TOGGLE(mask,default)	\
	PORT_BITX(    mask, mask & default, IPT_SERVICE1, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )


INPUT_PORTS_START( puzbobb )
	PORT_START      /* IN2 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1, 2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3, 2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START /* IN X */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/

	PORT_START      /* IN0 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START /* DSW B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW ) /*ok*/

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN1, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN2, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN3, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN4, 2 ) /*ok*/

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


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

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &tilelayout,  0, 256 },  /* sprites & playfield */
	{ -1 } /* end of array */
};


static struct GfxLayout rambo3_charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 512*1*1024*8 , 512*2*1024*8 , 512*3*1024*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};
static struct GfxLayout rambo3_tilelayout =
{
	16,16,	/* 16*16 tiles */
	16384,	/* 16384 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 512*1*1024*8 , 512*2*1024*8 , 512*3*1024*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 64+0, 64+1, 64+2, 64+3, 64+4, 64+5, 64+6, 64+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo rambo3_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0, &rambo3_charlayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &rambo3_tilelayout,  0, 256 },  /* sprites & playfield */
	{ -1 } /* end of array */
};


static struct GfxLayout qzshowby_charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 8 , 2048*1024*8 , 2048*1024*8+8},
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};
static struct GfxLayout qzshowby_tilelayout =
{
	16,16,	/* 16*16 tiles */
	32768,	/* 8192 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 8 , 2048*1024*8 , 2048*1024*8+8},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo qzshowby_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x1000*64, &qzshowby_charlayout,  0, 256 },  /* sprites & playfield */
	{ REGION_GFX1, 0x0, &qzshowby_tilelayout,  0, 256 },  /* sprites & playfield */
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

WRITE_HANDLER( portAwrite )
{
	static int a;

	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 1;

	if (a!=data)
		cpu_setbank (2, &RAM [0x10000 + (banknum * 0x4000)]);

	a=data;

	if ((a!=1) && (a!=2) )
		logerror("hitice write to port A on YM2203 val=%x\n",data);

}

static struct YM2203interface ym2203_interface =
{
	1,
	3500000,				/* 3.5 MHz complete guess*/
	{ YM2203_VOL(80,35) },	/* complete guess */
	{ 0 },
	{ 0 },
	{ portAwrite },
	{ 0 },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 8000,8000 },			/* complete guess */
	{ REGION_SOUND1,REGION_SOUND1 }, /* memory regions */
	{ 50,65 }				/*complete guess */
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
	10,
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_tm,
	taitob_vh_stop,
	taitob_vh_screenrefresh_tm,

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
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_tm,
	taitob_vh_stop,
	ashura_vh_screenrefresh_tm,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface_crimec
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
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_reversed_colors_tm,
	taitob_vh_stop,
	crimec_vh_screenrefresh_tm,

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
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	0, /*either no graphics rom dump, or the game does not use them. It uses pixel layer for sure*/
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_tm,
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


static struct MachineDriver machine_driver_hitice =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz */
			hitice_readmem,hitice_writemem,0,0,
			hitice_interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz */
			hitice_sound_readmem, hitice_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_tm,
	taitob_vh_stop,
	hitice_vh_screenrefresh_tm,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_rambo3 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz */
			rambo3_readmem,rambo3_writemem,0,0,
			rambo3_interrupt,1
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
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	rambo3_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_tm,
	taitob_vh_stop,
	rambo3_vh_screenrefresh_tm,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface_crimec
		}
	}
};

#if 0
static void patch_puzzb(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	WRITE_WORD(&RAM[0x7fffe],0x0004);
/*
0x0004 - Puzzle Bobble US Version "by Taito Japan",
0x0003 - Puzzle Buster US Version "by Taito Japan",
0x0002 - Puzzle Buster US Version,
0x0001 - Puzzle Bobble Japan Version,
0x0000 - test version (prototype)
*/
}
#endif

static struct MachineDriver machine_driver_puzbobb =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz */
			puzbobb_readmem,puzbobb_writemem,0,0,
			puzbobb_interrupt,1
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
	0, /*patch_puzzb,*/

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_reversed_colors_tm,
	taitob_vh_stop,
	puzbobb_vh_screenrefresh_tm,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610B,
			&ym2610_interface_crimec
		}
	},

	nvram_handler

};

static struct MachineDriver machine_driver_qzshowby =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,	/* 16 MHz according to the readme*/
			qzshowby_readmem,qzshowby_writemem,0,0,
			puzbobb_interrupt,1
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
	64*8, 32*8, { 0*8, 40*8-1, 2*8, 30*8-1 },

	qzshowby_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitob_vh_start_reversed_colors_tm,
	taitob_vh_stop,
	qzshowby_vh_screenrefresh_tm,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610B,
			&ym2610_interface_crimec
		}
	},

	nvram_handler

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
	ROM_LOAD_EVEN( "b99-07", 0x00000, 0x20000, 0x26e886e6 )
	ROM_LOAD_ODD ( "b99-05", 0x00000, 0x20000, 0xff7f9a9d )
	ROM_LOAD_EVEN( "b99-06", 0x40000, 0x20000, 0x1f26aa92 )
	ROM_LOAD_ODD ( "15"    , 0x40000, 0x20000, 0xe8c1e56d )

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
	ROM_LOAD_EVEN( "c4307-1.50", 0x00000, 0x20000, 0xd5ceb20f )
	ROM_LOAD_ODD ( "c4305-1.31", 0x00000, 0x20000, 0xa6f3bb37 )
	ROM_LOAD_EVEN( "c4306-1.49", 0x40000, 0x20000, 0x0f331802 )
	ROM_LOAD_ODD ( "c4304-1.30", 0x40000, 0x20000, 0xe06a2414 )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "c43-16",  0x00000, 0x4000, 0xcb26fce1 )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c43-02",  0x00000, 0x80000, 0x105722ae )
	ROM_LOAD( "c43-03",  0x80000, 0x80000, 0x426606ba )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "c43-01",  0x00000, 0x80000, 0xdb953f37 )

ROM_END

ROM_START( ashurau )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "c43-11", 0x00000, 0x20000, 0xd5aefc9b )
	ROM_LOAD_ODD ( "c43-09", 0x00000, 0x20000, 0xe91d0ab1 )
	ROM_LOAD_EVEN( "c43-10", 0x40000, 0x20000, 0xc218e7ea )
	ROM_LOAD_ODD ( "c43-08", 0x40000, 0x20000, 0x5ef4f19f )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "c43-16",  0x00000, 0x4000, 0xcb26fce1 )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c43-02",  0x00000, 0x80000, 0x105722ae )
	ROM_LOAD( "c43-03",  0x80000, 0x80000, 0x426606ba )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "c43-01",  0x00000, 0x80000, 0xdb953f37 )

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

	/*NOTE: no graphics roms*/

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	/* empty */

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* DELTA-T samples */
	/* empty */
ROM_END


ROM_START( hitice )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "c59-10", 0x00000, 0x20000, 0xe4ffad15 )
	ROM_LOAD_ODD ( "c59-12", 0x00000, 0x20000, 0xa080d7af )
	ROM_LOAD_EVEN( "c59-09", 0x40000, 0x10000, 0xe243e3b0 )
	ROM_LOAD_ODD ( "c59-11", 0x40000, 0x10000, 0x4d4dfa52 )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "c59-08",  0x00000, 0x4000, 0xd3cbc10b )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c59-03",  0x00000, 0x80000, 0x9e513048 )
	ROM_LOAD( "c59-02",  0x80000, 0x80000, 0xaffb5e07 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "c59-01",  0x00000, 0x20000, 0x46ae291d )

ROM_END

ROM_START( rambo3 )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "r3-0e.rom",  0x00000, 0x10000, 0x3efa4177 )
	ROM_LOAD_ODD ( "r3-0o.rom",  0x00000, 0x10000, 0x55c38d92 )

/*NOTE: there is a hole in address space here */

	ROM_LOAD_EVEN( "r3-1e.rom" , 0x40000, 0x20000, 0x40e363c7 )
	ROM_LOAD_ODD ( "r3-1o.rom" , 0x40000, 0x20000, 0x7f1fe6ab )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "r3-00.rom", 0x00000, 0x4000, 0xdf7a6ed6 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "r3-ch1ll.rom", 0x000000, 0x020000, 0xc86ea5fc )
	ROM_LOAD( "r3-ch1hl.rom", 0x020000, 0x020000, 0x7525eb92 )
	ROM_LOAD( "r3-ch3ll.rom", 0x040000, 0x020000, 0xabe54b1e )
	ROM_LOAD( "r3-ch3hl.rom", 0x060000, 0x020000, 0x80e5647e )
	ROM_LOAD( "r3-ch1lh.rom", 0x080000, 0x020000, 0x75568cf0 )
	ROM_LOAD( "r3-ch1hh.rom", 0x0a0000, 0x020000, 0xe39cff37 )
	ROM_LOAD( "r3-ch3lh.rom", 0x0c0000, 0x020000, 0x5a155c04 )
	ROM_LOAD( "r3-ch3hh.rom", 0x0e0000, 0x020000, 0xabe58fdb )
	ROM_LOAD( "r3-ch0ll.rom", 0x100000, 0x020000, 0xb416f1bf )
	ROM_LOAD( "r3-ch0hl.rom", 0x120000, 0x020000, 0xa4cad36d )
	ROM_LOAD( "r3-ch2ll.rom", 0x140000, 0x020000, 0xd0ce3051 )
	ROM_LOAD( "r3-ch2hl.rom", 0x160000, 0x020000, 0x837d8677 )
	ROM_LOAD( "r3-ch0lh.rom", 0x180000, 0x020000, 0x76a330a2 )
	ROM_LOAD( "r3-ch0hh.rom", 0x1a0000, 0x020000, 0x4dc69751 )
	ROM_LOAD( "r3-ch2lh.rom", 0x1c0000, 0x020000, 0xdf3bc48f )
	ROM_LOAD( "r3-ch2hh.rom", 0x1e0000, 0x020000, 0xbf37dfac )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "r3-a1.rom", 0x00000, 0x20000, 0x4396fa19 )
	ROM_LOAD( "r3-a2.rom", 0x20000, 0x20000, 0x41fe53a8 )
	ROM_LOAD( "r3-a3.rom", 0x40000, 0x20000, 0xe89249ba )
	ROM_LOAD( "r3-a4.rom", 0x60000, 0x20000, 0x9cf4c21b )

ROM_END

ROM_START( puzbobb )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "pb-1c18.bin", 0x00000, 0x40000, 0x5de14f49 )
	ROM_LOAD_ODD ( "pb-ic2.bin",  0x00000, 0x40000, 0x2abe07d1 )

	ROM_REGION( 0x2c000, REGION_CPU2 )     /* 128k for Z80 code */
	ROM_LOAD( "pb-ic27.bin", 0x00000, 0x04000, 0x26efa4c4 )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pb-ic14.bin", 0x00000, 0x80000, 0x55f90ea4 )
	ROM_LOAD( "pb-ic9.bin",  0x80000, 0x80000, 0x3253aac9 )

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "pb-ic15.bin", 0x000000, 0x100000, 0x0840cbc4 )

ROM_END


#if 0
/*
Quiz Sekai wa SHOW by shobai RoMset.(C)1994 Taito Corp.

D72-01.bin	:16M-MaskRoM	:Voice-RoM
D72-02.bin	:16M-MaskRoM	:GFX-RoM
D72-03.bin	:16M-MaskRoM	:GFX-RoM
D72-11.bin	:1M-EpRoM	:Sound-RoM
D72-12.bin	:4M-EpRoM	:PRG-RoM
D72-13.bin	:4M-EpRoM	:PRG-RoM

D72-05.pal	:PAL16L8
D72-06.pal	:PAL16L8
D72-07.pal	:PAL20L8B
D72-08.pal	:PAL20L8B
D72-09.pal	:PAL20L8B
D72-10.pal	:PAL16V8H

Total 12 files.

----------------------------
Quiz Sekai wa SHOWby shobai Board Infomation.
----------------------------

CPU	:MC68000P12F (Motorora 16MHz)
Sound	:Z80A (Zilog Z0840004PSC)
	:YM2610B+YM3016F
OSC	:32.0000MHz. 27.164MHz

Control: 4-buttons (Answer1,2,3 and 4)

* Taito Custom Chips.

 "TC0180VCU","TC0140SYT","TC0340BEX","TC0640FIO","TC0260DAR"

-----------------------------

# This game is based on Japanese TV Quiz show.

19/Nov/1999 Tatsuhiko.

*/

#endif

ROM_START( qzshowby )
	ROM_REGION( 0x100000, REGION_CPU1 )     /* 1M for 68000 code */
	ROM_LOAD_EVEN( "d72-13.bin", 0x00000, 0x80000, 0xa867759f )
	ROM_LOAD_ODD ( "d72-12.bin", 0x00000, 0x80000, 0x522c09a7 )

	ROM_REGION( 0x2c000, REGION_CPU2 )     /* 128k for Z80 code */
	ROM_LOAD(  "d72-11.bin", 0x00000, 0x04000, 0x2ca046e2 )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_GFX1 | REGIONFLAG_DISPOSE)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d72-03.bin", 0x000000, 0x200000, 0x1de257d0 )
	ROM_LOAD( "d72-02.bin", 0x200000, 0x200000, 0xbf0da640 )

	ROM_REGION( 0x200000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "d72-01.bin", 0x00000, 0x200000, 0xb82b8830 )

ROM_END


/*     year  rom       parent   machine   inp       init */
GAMEX( 1988, nastar,   0,       rastsag2, rastsag2, 0, ROT0,        "Taito Corporation Japan", "Nastar (World)", GAME_NO_COCKTAIL )
GAMEX( 1988, nastarw,  nastar,  rastsag2, rastsag2, 0, ROT0,        "Taito America Corporation", "Nastar Warrior (US)", GAME_NO_COCKTAIL )
GAMEX( 1988, rastsag2, nastar,  rastsag2, rastsag2, 0, ROT0,        "Taito Corporation", "Rastan Saga 2 (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1989, crimec,   0,       crimec,   crimec,   0, ROT0_16BIT,  "Taito Corporation Japan", "Crime City (World)", GAME_NO_COCKTAIL )
GAMEX( 1989, crimecu,  crimec,  crimec,   crimec,   0, ROT0_16BIT,  "Taito America Corporation", "Crime City (US)", GAME_NO_COCKTAIL )
GAMEX( 1989, crimecj,  crimec,  crimec,   crimec,   0, ROT0_16BIT,  "Taito Corporation", "Crime City (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1989, tetrist,  tetris,  tetrist,  tetrist,  0, ROT0_16BIT,  "Sega", "Tetris (Japan, B-System)", GAME_NO_COCKTAIL )
GAMEX( 1990, ashura,   0,       ashura,   ashura,   0, ROT270,      "Taito Corporation", "Ashura Blaster (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, ashurau,  ashura,  ashura,   ashura,   0, ROT270_16BIT,"Taito America Corporation", "Ashura Blaster (US)", GAME_NO_COCKTAIL )
GAMEX( 1990, hitice,   0,       hitice,   hitice,   0, ROT180,      "Williams", "Hit the Ice (US)", GAME_NO_COCKTAIL )
GAMEX( 1989, rambo3,   0,       rambo3,   rastsag2, 0, ROT180,      "Taito Europe Corporation", "Rambo III (Europe)", GAME_NO_COCKTAIL )
GAMEX( 1994, puzbobb,  pbobble, puzbobb,  puzbobb,  0, ROT0,        "Taito Corporation", "Puzzle Bobble (Japan, B-System)", GAME_NO_COCKTAIL )
GAMEX( 1994, qzshowby, 0,       qzshowby, puzbobb,  0, ROT0,        "Taito Corporation", "Quiz Sekai wa SHOW by shobai (Japan)", GAME_NO_COCKTAIL )
