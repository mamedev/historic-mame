/***************************************************************************

Taito B System

driver by Jarek Burczynski, with help from:
Nicola Salmoria, Brian A. Troha, Stephane Humbert, Gerardo Oporto Jorrin, David Graves
heavily based on Taito F2 System driver by Brad Oliver, Andrew Prime

The board uses TC0220IOC, TC0260DAR, TC0180VCU, and TC0140SYT.
Sonic Blast Man uses TC0510NIO instead of TC0220IOC.

The palette resolution is 12 bits in some games and 15 bits in others.

TODO:
- hitice: ice trails might not be 100% correct (I'm doubling them horizontally)
- hitice: the pixel bitmap is not cleared on startup nor during attract mode. There's a
  kludge to fix it in the first case.
- rambo3a: has a lot of unmapped writes in the VCU area (log up to end of
  round 2) [viofight also does a few]
- The eprom games could have a single io handler if it's confirmed all
  3 use a special 4 player I/O chip. Puzzle Bobble and qzshowby use TC0640FIO
- can the text layer scroll? (hitice: glass walls at beginning of match, also check when the
  screen wiggles after the puck hits the wall shortly into the first round of attract mode)
- sprites are not in perfect sync with the background. Check ashura, they are almost
  tight during gameplay but completely off in attract mode.

The Taito B system is a fairly flexible hardware platform. It supports 4
separate layers of graphics - one 64x64 tiled scrolling background plane
of 16x16 tiles, a similar foreground plane, a sprite plane capable of sprite
zooming and 'pageable' text plane of 8x8 tiles.

Sound is handled by a Z80 with a YM2610 or YM2610B or YM2203's connected
to it. Different sound chips - depending on game.

The memory map for each of the games is similar but not identical.


Memory map for Rastan Saga 2 / Nastar / Nastar Warrior :

CPU 1 : 68000, uses irqs 2 & 4. One of the IRQs just sets a flag which is
checked in the other IRQ routine. Could be timed to vblank...

  0x000000 - 0x07ffff : ROM
  0x200000 - 0x201fff : palette RAM, 4096 total colors (0x1000 words)
  0x400000 - 0x403fff : 64x64 foreground layer (offsets 0x0000-0x1fff tile codes; offsets 0x2000-0x3fff tile attributes)
  0x404000 - 0x407fff : 64x64 background layer (offsets 0x0000-0x1fff tile codes; offsets 0x2000-0x3fff tile attributes)
  0x408000 - 0x408fff : 64x64 text layer
  0x410000 - 0x41197f : ??k of sprite RAM (this is the range that Rastan Saga II tests at startup time)
  0x413800 - 0x413bff : foreground (line/screen) scroll RAM
  0x413c00 - 0x413fff : background (line/screen) scroll RAM

  0x600000 - 0x607fff : 32k of CPU RAM
  0x800000 - 0x800003 : communication with sound CPU via TC0140SYT
  0xa00000 - 0xa0000f : input ports and dipswitches


Notes:
 Master of Weapon has secret command to select level:
 (sequence is the same as in Metal Black):
 - boot machine with service switch pressed
 - message appears: "SERVICE SWITCH ERROR"
 - press 1p start, 1p start, 1p start, service switch, 1p start
 - message appears: "SELECT BY DOWN SW"
 - select level with joy down/up
 - press 1p start button

Other games that have this feature:
 Rastan Saga 2
 Crime City
 Violence Fight
 Rambo 3



List of known B-System games:

	Rastan Saga II					(YM2610 sound)
	Ashura Blaster					(YM2610 sound)
	Crime City						(YM2610 sound)
	Rambo 3 (two different versions)(YM2610 sound)
	Tetris							(YM2610 sound)
	Space Invaders DX				(YM2610 sound, MB87078 - electronic volume control)
	Silent Dragon					(YM2610 sound)
	Sel Feena						(YM2610 sound)
	Ryujin							(YM2610 sound)

	Violence Fight					(YM2203 sound, 2xMSM6295 )
	Hit The Ice						(YM2203 sound, 2xMSM6295 )
	Master of Weapons				(YM2203 sound)

	Quiz Sekai wa SHOW by shobai	(YM2610-B sound, MB87078 - electronic volume control)
	Puzzle Bobble					(YM2610-B sound, MB87078 - electronic volume control)
	Sonic Blast Man					(YM2610-B sound)

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "machine/eeprom.h"
#include "machine/mb87078.h"
#include "sndhrdw/taitosnd.h"

extern data16_t *taitob_scroll;
extern data16_t *TC0180VCU_ram;
extern data16_t *taitob_spriteram;
extern data16_t *taitob_pixelram;


VIDEO_START( taitob_color_order0 );
VIDEO_START( taitob_color_order1 );
VIDEO_START( taitob_color_order2 );
VIDEO_START( hitice );
VIDEO_EOF( taitob );

VIDEO_UPDATE( taitob );

WRITE16_HANDLER( TC0180VCU_word_w );
READ16_HANDLER ( TC0180VCU_word_r );

READ16_HANDLER( TC0180VCU_framebuffer_word_r );
WRITE16_HANDLER( TC0180VCU_framebuffer_word_w );

WRITE16_HANDLER( taitob_v_control_w );
READ16_HANDLER ( taitob_v_control_r );

WRITE16_HANDLER( hitice_pixelram_w );
WRITE16_HANDLER( hitice_pixel_scroll_w );

static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 3;

	cpu_setbank (1, &RAM [0x10000 + (banknum * 0x4000)]);
}



void rsaga2_interrupt2(int x)
{
	cpu_set_irq_line(0,2,HOLD_LINE);
}

static INTERRUPT_GEN( rastansaga2_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,rsaga2_interrupt2);
	cpu_set_irq_line(0, 4, HOLD_LINE);
}


void crimec_interrupt3(int x)
{
	cpu_set_irq_line(0,3,HOLD_LINE);
}

static INTERRUPT_GEN( crimec_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,crimec_interrupt3);
	cpu_set_irq_line(0, 5, HOLD_LINE);
}


void hitice_interrupt6(int x)
{
	cpu_set_irq_line(0,6,HOLD_LINE);
}

static INTERRUPT_GEN( hitice_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,hitice_interrupt6);
	cpu_set_irq_line(0, 4, HOLD_LINE);
}


void rambo3_interrupt1(int x)
{
	cpu_set_irq_line(0,1,HOLD_LINE);
}

static INTERRUPT_GEN( rambo3_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,rambo3_interrupt1);
	cpu_set_irq_line(0, 6, HOLD_LINE);
}


void pbobble_interrupt5(int x)
{
	cpu_set_irq_line(0,5,HOLD_LINE);
}

static INTERRUPT_GEN( pbobble_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,pbobble_interrupt5);
	cpu_set_irq_line(0, 3, HOLD_LINE);
}

void viofight_interrupt1(int x)
{
	cpu_set_irq_line(0,1,HOLD_LINE);
}

static INTERRUPT_GEN( viofight_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,viofight_interrupt1);
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

void masterw_interrupt4(int x)
{
	cpu_set_irq_line(0,4,HOLD_LINE);
}

static INTERRUPT_GEN( masterw_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,masterw_interrupt4);
	cpu_set_irq_line(0, 5, HOLD_LINE);
}

void silentd_interrupt6(int x)
{
	cpu_set_irq_line(0,6,HOLD_LINE);
}

static INTERRUPT_GEN( silentd_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,silentd_interrupt6);
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

void selfeena_interrupt4(int x)
{
	cpu_set_irq_line(0,4,HOLD_LINE);
}

static INTERRUPT_GEN( selfeena_interrupt )
{
	timer_set(TIME_IN_CYCLES(5000,0),0,selfeena_interrupt4);
	cpu_set_irq_line(0, 6, HOLD_LINE);
}

void sbm_interrupt5(int x)//4
{
	cpu_set_irq_line(0,5,HOLD_LINE);
}

static INTERRUPT_GEN( sbm_interrupt )//5
{
	timer_set(TIME_IN_CYCLES(10000,0),0,sbm_interrupt5);
	cpu_set_irq_line(0, 4, HOLD_LINE);
}



static READ16_HANDLER( tracky1_hi_r )
{
	return input_port_5_word_r(0,0);
}
static READ16_HANDLER( tracky1_lo_r )
{
	return (input_port_5_word_r(0,0) & 0xff) <<8;
}
static READ16_HANDLER( trackx1_hi_r )
{
	return input_port_6_word_r(0,0);
}
static READ16_HANDLER( trackx1_lo_r )
{
	return (input_port_6_word_r(0,0) & 0xff) <<8;
}
static READ16_HANDLER( tracky2_hi_r )
{
	return input_port_7_word_r(0,0);
}
static READ16_HANDLER( tracky2_lo_r )
{
	return (input_port_7_word_r(0,0) & 0xff) <<8;
}
static READ16_HANDLER( trackx2_hi_r )
{
	return input_port_8_word_r(0,0);
}
static READ16_HANDLER( trackx2_lo_r )
{
	return (input_port_8_word_r(0,0) & 0xff) <<8;
}


static WRITE16_HANDLER( gain_control_w )
{
	if (ACCESSING_MSB)
	{
		if (offset==0)
		{
			MB87078_data_w(0, data>>8, 0);
            //logerror("MB87078 dsel=0 data=%4x\n",data);
		}
		else
		{
			MB87078_data_w(0, data>>8, 1);
            //logerror("MB87078 dsel=1 data=%4x\n",data);
		}
	}
}

/***************************************************************************

  Puzzle Bobble, Qzshowby, Space DX   EEPROM

***************************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/*  read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/*  lock command */
	"0100110000" 	/* unlock command*/
};

static NVRAM_HANDLER( taito_b )
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

static READ16_HANDLER( eeprom_r )
{
	int res;

	res = (EEPROM_read_bit() & 0x01);
	res |= input_port_1_word_r(0,0) & 0xfe; /* coin inputs */

	return res;
}

static data16_t eep_latch = 0;

static READ16_HANDLER( eep_latch_r )
{
	return eep_latch;
}

static WRITE16_HANDLER( eeprom_w )
{
	COMBINE_DATA(&eep_latch);

    if (ACCESSING_MSB)
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
}

/*************************************************************************
   The input area for the three eprom games ($500000-2f) may well be
   addressing a single i/o chip with 4 player and coin inputs as
   standard.

   Does anyone have custom chip numbers from the Space Invaders DX ?
   (qzshowby and pbobble do use TC0640FIO).

*************************************************************************/

static UINT16 coin_word=0;

static READ16_HANDLER( player_34_coin_ctrl_r )
{
	return coin_word;
}

static WRITE16_HANDLER( player_34_coin_ctrl_w )
{
	COMBINE_DATA(&coin_word);

	/* coin counters and lockout */
	coin_lockout_w(2,~data & 0x0100);
	coin_lockout_w(3,~data & 0x0200);
	coin_counter_w(2, data & 0x0400);
	coin_counter_w(3, data & 0x0800);
}

static READ16_HANDLER( pbobble_input_bypass_r )
{
	switch (offset)
	{
		case 0x01:
			return eeprom_r(0,mem_mask) << 8;

		default:
			return TC0640FIO_r( offset ) << 8;
	}
}



#define TC0180VCU_MEMR( ADDR )											\
	{ ADDR+0x00000, ADDR+0x0ffff, TC0180VCU_word_r },					\
	{ ADDR+0x10000, ADDR+0x1197f, MRA16_RAM },							\
	{ ADDR+0x11980, ADDR+0x11fff, MRA16_RAM },							\
	{ ADDR+0x13800, ADDR+0x13fff, MRA16_RAM },							\
	{ ADDR+0x18000, ADDR+0x1801f, taitob_v_control_r },					\
	{ ADDR+0x40000, ADDR+0x7ffff, TC0180VCU_framebuffer_word_r },

#define TC0180VCU_MEMW( ADDR )											\
	{ ADDR+0x00000, ADDR+0x0ffff, TC0180VCU_word_w, &TC0180VCU_ram },	\
	{ ADDR+0x10000, ADDR+0x1197f, MWA16_RAM, &taitob_spriteram },		\
	{ ADDR+0x11980, ADDR+0x137ff, MWA16_RAM },							\
	{ ADDR+0x13800, ADDR+0x13fff, MWA16_RAM, &taitob_scroll },			\
	{ ADDR+0x18000, ADDR+0x1801f, taitob_v_control_w },					\
	{ ADDR+0x40000, ADDR+0x7ffff, TC0180VCU_framebuffer_word_w },


static MEMORY_READ16_START( rastsag2_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x600000, 0x607fff, MRA16_RAM },			/* Main RAM */

	{ 0x200000, 0x201fff, MRA16_RAM },			/* palette */

	TC0180VCU_MEMR( 0x400000 )

	{ 0xa00000, 0xa0000f, TC0220IOC_halfword_byteswap_r },

	{ 0x800000, 0x800001, MRA16_NOP },
	{ 0x800002, 0x800003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( rastsag2_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x600000, 0x607fff, MWA16_RAM },	/* Main RAM */ /*ashura up to 603fff only*/

	{ 0x200000, 0x201fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0xa00000, 0xa0000f, TC0220IOC_halfword_byteswap_w },

	{ 0x800000, 0x800001, taitosound_port16_msb_w },
	{ 0x800002, 0x800003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( crimec_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xa00000, 0xa0ffff, MRA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x400000 )

	{ 0x200000, 0x20000f, TC0220IOC_halfword_byteswap_r },

	{ 0x600000, 0x600001, MRA16_NOP },
	{ 0x600002, 0x600003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( crimec_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xa00000, 0xa0ffff, MWA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x200000, 0x20000f, TC0220IOC_halfword_byteswap_w },

	{ 0x600000, 0x600001, taitosound_port16_msb_w },
	{ 0x600002, 0x600003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( tetrist_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x800000, 0x807fff, MRA16_RAM },	/* Main RAM */

	{ 0xa00000, 0xa01fff, MRA16_RAM }, /*palette*/

	TC0180VCU_MEMR( 0x400000 )

	{ 0x600000, 0x60000f, TC0220IOC_halfword_byteswap_r },

	{ 0x200000, 0x200001, MRA16_NOP },
	{ 0x200002, 0x200003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( tetrist_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x800000, 0x807fff, MWA16_RAM },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x600000, 0x60000f, TC0220IOC_halfword_byteswap_w },

	{ 0x200000, 0x200001, taitosound_port16_msb_w },
	{ 0x200002, 0x200003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( hitice_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x800000, 0x803fff, MRA16_RAM },	/* Main RAM */

	{ 0xa00000, 0xa01fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x400000 )

	{ 0x600000, 0x60000f, TC0220IOC_halfword_byteswap_r },
	{ 0x610000, 0x610001, input_port_5_word_r },		/* player 3,4 inputs*/

	{ 0x700000, 0x700001, MRA16_NOP },
	{ 0x700002, 0x700003, taitosound_comm16_msb_r },

	{ 0xb00000, 0xb7ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( hitice_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x800000, 0x803fff, MWA16_RAM },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x600000, 0x60000f, TC0220IOC_halfword_byteswap_w },

	{ 0x700000, 0x700001, taitosound_port16_msb_w },
	{ 0x700002, 0x700003, taitosound_comm16_msb_w },

	{ 0xb00000, 0xb7ffff, hitice_pixelram_w, &taitob_pixelram },
//	{ 0xbffff0, 0xbffff1, ???
	{ 0xbffff2, 0xbffff5, hitice_pixel_scroll_w },
//	{ 0xbffffa, 0xbffffb, ???
MEMORY_END


static MEMORY_READ16_START( rambo3_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x800000, 0x803fff, MRA16_RAM },	/* Main RAM */

	{ 0xa00000, 0xa01fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x400000 )

	{ 0x600000, 0x60000f, TC0220IOC_halfword_byteswap_r },
	{ 0x600010, 0x600011, tracky1_lo_r }, /*player 1*/
	{ 0x600012, 0x600013, tracky1_hi_r },
	{ 0x600014, 0x600015, trackx1_lo_r },
	{ 0x600016, 0x600017, trackx1_hi_r },
	{ 0x600018, 0x600019, tracky2_lo_r }, /*player 2*/
	{ 0x60001a, 0x60001b, tracky2_hi_r },
	{ 0x60001c, 0x60001d, trackx2_lo_r },
	{ 0x60001e, 0x60001f, trackx2_hi_r },

	{ 0x200000, 0x200001, MRA16_NOP },
	{ 0x200002, 0x200003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( rambo3_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x800000, 0x803fff, MWA16_RAM },	/* Main RAM */

	{ 0xa00000, 0xa01fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x600000, 0x60000f, TC0220IOC_halfword_byteswap_w },

	{ 0x200000, 0x200001, taitosound_port16_msb_w },
	{ 0x200002, 0x200003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( pbobble_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x900000, 0x90ffff, MRA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x400000 )

	{ 0x500000, 0x50000f, pbobble_input_bypass_r },
	{ 0x500024, 0x500025, input_port_5_word_r },	/* shown in service mode, game omits to read it */
	{ 0x500026, 0x500027, eep_latch_r },	/* not read by this game */
	{ 0x50002e, 0x50002f, input_port_6_word_r },	/* shown in service mode, game omits to read it */

	{ 0x700000, 0x700001, MRA16_NOP },
	{ 0x700002, 0x700003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( pbobble_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x900000, 0x90ffff, MWA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x500000, 0x50000f, TC0640FIO_halfword_byteswap_w },
	{ 0x500026, 0x500027, eeprom_w },
	{ 0x500028, 0x500029, player_34_coin_ctrl_w },	/* simply locks coins 3&4 out */

	{ 0x700000, 0x700001, taitosound_port16_msb_w },
	{ 0x700002, 0x700003, taitosound_comm16_msb_w },
	{ 0x600000, 0x600003, gain_control_w },
MEMORY_END

static MEMORY_READ16_START( spacedx_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x900000, 0x90ffff, MRA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x400000 )

	{ 0x500000, 0x50000f, pbobble_input_bypass_r },
	{ 0x500024, 0x500025, input_port_5_word_r },
	{ 0x500026, 0x500027, eep_latch_r },
	{ 0x50002e, 0x50002f, input_port_6_word_r },

	{ 0x700000, 0x700001, MRA16_NOP },
	{ 0x700002, 0x700003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( spacedx_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x900000, 0x90ffff, MWA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x500000, 0x50000f, TC0640FIO_halfword_byteswap_w },
	{ 0x500026, 0x500027, eeprom_w },
	{ 0x500028, 0x500029, player_34_coin_ctrl_w },	/* simply locks coins 3&4 out */

	{ 0x700000, 0x700001, taitosound_port16_msb_w },
	{ 0x700002, 0x700003, taitosound_comm16_msb_w },
	{ 0x600000, 0x600003, gain_control_w },
MEMORY_END

static MEMORY_READ16_START( spacedxo_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, MRA16_RAM },	/* Main RAM */

	{ 0x300000, 0x303fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x500000 )

	{ 0x200000, 0x20000f, TC0220IOC_halfword_r },
	{ 0x210000, 0x210001, input_port_5_word_r },
	{ 0x220000, 0x220001, input_port_6_word_r },
	{ 0x230000, 0x230001, input_port_7_word_r },

	{ 0x100000, 0x100001, MRA16_NOP },
	{ 0x100002, 0x100003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( spacedxo_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, MWA16_RAM },	/* Main RAM */

	{ 0x300000, 0x301fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x500000 )

	{ 0x200000, 0x20000f, TC0220IOC_halfword_w },

	{ 0x100000, 0x100001, taitosound_port16_msb_w },
	{ 0x100002, 0x100003, taitosound_comm16_msb_w },
MEMORY_END

static MEMORY_READ16_START( qzshowby_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x900000, 0x90ffff, MRA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x400000 )

	{ 0x200000, 0x20000f, pbobble_input_bypass_r },
	{ 0x200024, 0x200025, input_port_5_word_r },	/* player 3,4 start */
	{ 0x200028, 0x200029, player_34_coin_ctrl_r },
	{ 0x20002e, 0x20002f, input_port_6_word_r },	/* player 3,4 buttons */

	{ 0x600000, 0x600001, MRA16_NOP },
	{ 0x600002, 0x600003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( qzshowby_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x900000, 0x90ffff, MWA16_RAM },	/* Main RAM */

	{ 0x800000, 0x801fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x200000, 0x20000f, TC0640FIO_halfword_byteswap_w },
	{ 0x200026, 0x200027, eeprom_w },
	{ 0x200028, 0x200029, player_34_coin_ctrl_w },

	{ 0x600000, 0x600001, taitosound_port16_msb_w },
	{ 0x600002, 0x600003, taitosound_comm16_msb_w },
	{ 0x700000, 0x700003, gain_control_w },
MEMORY_END


static MEMORY_READ16_START( viofight_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xa00000, 0xa03fff, MRA16_RAM },	/* Main RAM */

	{ 0x600000, 0x601fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x400000 )

	{ 0x800000, 0x80000f, TC0220IOC_halfword_byteswap_r },

	{ 0x200000, 0x200001, MRA16_NOP },
	{ 0x200002, 0x200003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( viofight_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xa00000, 0xa03fff, MWA16_RAM },	/* Main RAM */

	{ 0x600000, 0x601fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x800000, 0x80000f, TC0220IOC_halfword_byteswap_w },

	{ 0x200000, 0x200001, taitosound_port16_msb_w },
	{ 0x200002, 0x200003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( masterw_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x200000, 0x203fff, MRA16_RAM },			/* Main RAM */

	{ 0x600000, 0x601fff, MRA16_RAM },	/*palette*/

	TC0180VCU_MEMR( 0x400000 )

	{ 0x800000, 0x800001, TC0220IOC_halfword_byteswap_portreg_r },	/* DSW A/B, player inputs*/
	{ 0x800002, 0x800003, TC0220IOC_halfword_byteswap_port_r /*watchdog_reset16_r*/ },

	{ 0xa00000, 0xa00001, MRA16_NOP },
	{ 0xa00002, 0xa00003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( masterw_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x200000, 0x203fff, MWA16_RAM },	/* Main RAM */

	{ 0x600000, 0x601fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x400000 )

	{ 0x800000, 0x800001, TC0220IOC_halfword_byteswap_portreg_w },
	{ 0x800002, 0x800003, TC0220IOC_halfword_byteswap_port_w },

	{ 0xa00000, 0xa00001, taitosound_port16_msb_w },
	{ 0xa00002, 0xa00003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( silentd_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x400000, 0x403fff, MRA16_RAM },	/* Main RAM */

	{ 0x300000, 0x301fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x500000 )

	{ 0x200000, 0x20000f, TC0220IOC_halfword_r },
	{ 0x210000, 0x210001, input_port_5_word_r },
	{ 0x220000, 0x220001, input_port_6_word_r },
	{ 0x230000, 0x230001, input_port_7_word_r },
//	{ 0x240000, 0x240001, MRA16_NOP },	/* read 4 times at init */

	{ 0x100000, 0x100001, MRA16_NOP },
	{ 0x100002, 0x100003, taitosound_comm16_msb_r },
//	{ 0x10001a, 0x10001b, MRA16_NOP },	// ??? read at $1e344
//	{ 0x10001c, 0x10001d, MRA16_NOP },	// ??? read at $1e356
MEMORY_END

static MEMORY_WRITE16_START( silentd_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x400000, 0x403fff, MWA16_RAM },	/* Main RAM */

	{ 0x300000, 0x301fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x500000 )

	{ 0x200000, 0x20000f, TC0220IOC_halfword_w },
	{ 0x240000, 0x240001, MWA16_NOP }, // ???

	{ 0x100000, 0x100001, taitosound_port16_msb_w },
	{ 0x100002, 0x100003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( selfeena_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },	/* Main RAM */

	{ 0x300000, 0x301fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x200000 )

	{ 0x400000, 0x40000f, TC0220IOC_halfword_byteswap_r },
	{ 0x410000, 0x41000f, TC0220IOC_halfword_byteswap_r }, /* mirror address - seems to be only used for coin control */

	{ 0x500000, 0x500001, MRA16_NOP },
	{ 0x500002, 0x500003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( selfeena_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },	/* Main RAM */

	{ 0x300000, 0x301fff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x200000 )

	{ 0x400000, 0x40000f, TC0220IOC_halfword_byteswap_w },
	{ 0x410000, 0x41000f, TC0220IOC_halfword_byteswap_w }, /* mirror address - seems to be only used for coin control */

	{ 0x500000, 0x500001, taitosound_port16_msb_w },
	{ 0x500002, 0x500003, taitosound_comm16_msb_w },
MEMORY_END


static MEMORY_READ16_START( sbm_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },	/* Main RAM */

	{ 0x200000, 0x201fff, MRA16_RAM },

	TC0180VCU_MEMR( 0x900000 )

	{ 0x300000, 0x30000f, TC0510NIO_halfword_wordswap_r },

	{ 0x320000, 0x320001, MRA16_NOP },
	{ 0x320002, 0x320003, taitosound_comm16_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( sbm_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },	/* Main RAM */

	{ 0x200000, 0x201fff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },

	TC0180VCU_MEMW( 0x900000 )

	{ 0x300000, 0x30000f, TC0510NIO_halfword_wordswap_w },

	{ 0x320000, 0x320001, taitosound_port16_msb_w },
	{ 0x320002, 0x320003, taitosound_comm16_msb_w },
MEMORY_END



static MEMORY_READ_START( masterw_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( masterw_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xe600, 0xe600, MWA_NOP }, /* ? */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, bankswitch_w },
MEMORY_END

static MEMORY_READ_START( hitice_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xb000, 0xb000, OKIM6295_status_0_r },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( hitice_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xb000, 0xb000, OKIM6295_data_0_w },
	{ 0xb001, 0xb001, OKIM6295_data_1_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

#define TAITO_COINAGE_JAPAN_NEW_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )


/* Included only the bits that are common to all sets (viofight has 3 buttons) */
#define TAITO_B_PLAYERS_INPUT( player ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | player ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | player ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | player )

#define TAITO_B_SYSTEM_INPUT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 2 ) \
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

#define TAITO_B_DSWA_2_4 \
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off )) \
	PORT_DIPSETTING(    0x00, DEF_STR( On )) \
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW ) \
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off )) \
	PORT_DIPSETTING(    0x08, DEF_STR( On ))


INPUT_PORTS_START( rastsag2 )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )  // all 2 "unused" in manual
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100k only" )
	PORT_DIPSETTING(    0x08, "150k only" )
	PORT_DIPSETTING(    0x04, "200k only" )
	PORT_DIPSETTING(    0x00, "250k only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( nastar )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100k only" )
	PORT_DIPSETTING(    0x08, "150k only" )
	PORT_DIPSETTING(    0x04, "200k only" )
	PORT_DIPSETTING(    0x00, "250k only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( nastarw )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_US_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100k only" )
	PORT_DIPSETTING(    0x08, "150k only" )
	PORT_DIPSETTING(    0x04, "200k only" )
	PORT_DIPSETTING(    0x00, "250k only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( masterw )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "500k, 1000k and 1500k" )
	PORT_DIPSETTING(    0x0c, "500k and 1000k" )
	PORT_DIPSETTING(    0x04, "500k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Ship" )
	PORT_DIPSETTING(    0x80, "Default" )
	PORT_DIPSETTING(    0x00, "Alternate" )

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT

INPUT_PORTS_END

INPUT_PORTS_START( crimec )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Hi Score" )
	PORT_DIPSETTING(    0x01, "Scribble" )
	PORT_DIPSETTING(    0x00, "3 Characters" )
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "every 80k" )
	PORT_DIPSETTING(    0x0c, "80k only" )
	PORT_DIPSETTING(    0x04, "160k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xc0, "5 Times" )
	PORT_DIPSETTING(    0x00, "8 Times" )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( crimecj )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Hi Score" )
	PORT_DIPSETTING(    0x01, "Scribble" )
	PORT_DIPSETTING(    0x00, "3 Characters" )
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "every 80k" )
	PORT_DIPSETTING(    0x0c, "80k only" )
	PORT_DIPSETTING(    0x04, "160k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xc0, "5 Times" )
	PORT_DIPSETTING(    0x00, "8 Times" )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( crimecu )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Hi Score" )
	PORT_DIPSETTING(    0x01, "Scribble" )
	PORT_DIPSETTING(    0x00, "3 Characters" )
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_US_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "every 80k" )
	PORT_DIPSETTING(    0x0c, "80k only" )
	PORT_DIPSETTING(    0x04, "160k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xc0, "5 Times" )
	PORT_DIPSETTING(    0x00, "8 Times" )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( tetrist )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
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

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( ashura )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "every 100k" )
	PORT_DIPSETTING(    0x0c, "every 150k" )
	PORT_DIPSETTING(    0x04, "every 200k" )
	PORT_DIPSETTING(    0x00, "every 250k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( ashurau )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_US_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "every 100k" )
	PORT_DIPSETTING(    0x0c, "every 150k" )
	PORT_DIPSETTING(    0x04, "every 200k" )
	PORT_DIPSETTING(    0x00, "every 250k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

/*
Hit the Ice dipswitches
(info from Kevin Watson)

[1 is switch on and 0 is switch off]

Dip switch A
------------

Setting             Options          1 2 3 4 5 6 7 8
cabinet style       4 player         0
                    2 player         1
Test mode           normal               0
                    test mode            1
Attract mode        on                     0
                    off                    1
Game price          1 coin 1 play            0 0 0 0
                    2 coin 1 play            1 0 0 0
                    3 coin 1 play            0 1 0 0
           coin1    1 coin 2 play            0 0 1 0
           coin2    1 coin 3 play            1 1 0 0
                    1 coin 4 play            0 1 0 0
                    1 coin 5 play            1 0 1 0
                    1 coin 6 play            1 1 1 0

switch 2 and 8 are always set to off

Dip switch table B
------------------

Setting             Options          1 2 3 4 5 6 7 8
Difficulty          normal           0 0
                    easy             1 0
                    hard             0 1
                    hardest          1 1
Timer count         1 sec = 58/60        0 0
                    1 sec = 56/60        1 0
                    1 sec = 62/60        0 1
                    1 sec = 45/60        1 1
maximum credit      9                             0
                    99                            1

5,6,7 are set to off
*/

INPUT_PORTS_START( hitice )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet Style" )
	PORT_DIPSETTING(    0x01, "4 Players")
	PORT_DIPSETTING(    0x00, "2 Players")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Timer count" )
	PORT_DIPSETTING(    0x0c, "1 sec = 58/60" )
	PORT_DIPSETTING(    0x04, "1 sec = 56/60" )
	PORT_DIPSETTING(    0x08, "1 sec = 62/60" )
	PORT_DIPSETTING(    0x00, "1 sec = 45/60" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x20, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, "Maximum credits" )
	PORT_DIPSETTING(    0x00, "99" )
	PORT_DIPSETTING(    0x80, "9"  )

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
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
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

	PORT_START      /* IN5 IN6 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( rambo3 )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )  // all 5 "unused" in manual
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( rambo3a )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	/* Coinage similar to US, but there are some differences */
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, "Same as Start" )
	PORT_DIPSETTING(    0x00, "Same as Start or 1C/1C (if Coinage 4C/3C)" )

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, "Control" )
	PORT_DIPSETTING(    0x08, "8 way Joystick" )
	PORT_DIPSETTING(    0x00, "Trackball" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT(         0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(         0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(         0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_HIGH, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN2, 2 )
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

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_Y | IPF_PLAYER1 | IPF_REVERSE, 70, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_X | IPF_PLAYER1, 70, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_Y | IPF_PLAYER2 | IPF_REVERSE, 70, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_X | IPF_PLAYER2, 70, 30, 0, 0 )
INPUT_PORTS_END


/* Helps document the input ports. */

INPUT_PORTS_START( pbobble )	/* Missing P3&4 controls ! */
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW ) /*ok*/

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN1, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN2, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN3, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN4, 2 ) /*ok*/

	PORT_START /* IN2 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START /* IN3 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/

	PORT_START /* IN4 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/

	PORT_START /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
INPUT_PORTS_END

INPUT_PORTS_START( spacedxo )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_JAPAN_NEW_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Match Point" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "1500 Points" )
	PORT_DIPSETTING(    0x00, "1000 Points" )
	PORT_DIPNAME( 0x80, 0x80, "Game Type" )
	PORT_DIPSETTING(    0x80, "Double Company" )
	PORT_DIPSETTING(    0x00, "Single Company" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT(         0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(         0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(         0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT(         0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(         0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )

	PORT_START      /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )

	PORT_START      /* IN7 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN3, 2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN4, 2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

INPUT_PORTS_START( qzshowby )
	PORT_START /* DSW B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*unused in test mode*/
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW ) /*ok*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN1, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN2, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN3, 2 ) /*ok*/
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN4, 2 ) /*ok*/

	PORT_START      /* IN0 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START		/* IN 1 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  ) /* IPT_START1 in test mode */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  ) /* IPT_START2 in test mode */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */ /*all OK*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START      /* IN5 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  ) /* IPT_START3 in test mode */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  ) /* IPT_START4 in test mode */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN6 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
INPUT_PORTS_END

INPUT_PORTS_START( viofight )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )  // all 7 "unused" in manual
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	TAITO_B_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	TAITO_B_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( silentd )
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
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

/* These bits are tied together... Maybe "Credits" should be "Coin Slots"???  You can play
	the game with 2, 3, or 4 players and the last option maybe a linked 4 players.
	Using bit6 and bit7&8 you end up with 1, 2 or 4 seperate "Credits" on the demo screens.
	Using bits7&8 you can have 2-4 players as shown at the top of the game screens.
	I have no clue about the rest of them.  I cannot seem to find a way to disable the
	continue option.  I've set all other bits to off, then all to on and it makes no
	difference.  Also Coin B doesn't seem to be affected by the dip settings.
*/

	PORT_DIPNAME( 0x20, 0x20, "Credits" )	/* Only shows 4 seperate credits with 4p/1m below */
	PORT_DIPSETTING(    0x20, "Combined" )
	PORT_DIPSETTING(    0x00, "Seperate" )	/* When multiple credits show, Coin B will affect p2 credits */
	PORT_DIPNAME( 0xc0, 0x80, "Cabinet Style" )
	PORT_DIPSETTING(    0xc0, "3 Players")
	PORT_DIPSETTING(    0x80, "2 Players")
	PORT_DIPSETTING(    0x40, "4 Players/1 Machine??") /* with bit6, shows 4 seperate credits */
	PORT_DIPSETTING(    0x00, "4 Players/2 Machines??")	/* with bit6 shows 2 seperate credits */

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
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
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

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )

	PORT_START      /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )

	PORT_START      /* IN7 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN3, 2 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN3, 2 ) /*not sure if this is legal under MAME*/
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN4, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN4, 2 ) /*not sure if this is legal under MAME*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( silentdj )
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
	TAITO_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, "Credits" )	/* Only shows 4 seperate credits with 4p/1m below */
	PORT_DIPSETTING(    0x20, "Combined" )
	PORT_DIPSETTING(    0x00, "Seperate" )	/* When multiple credits show, Coin B will affect p2 credits */
	PORT_DIPNAME( 0xc0, 0x80, "Cabinet Style" )
	PORT_DIPSETTING(    0xc0, "3 Players")
	PORT_DIPSETTING(    0x80, "2 Players")
	PORT_DIPSETTING(    0x40, "4 Players/1 Machine??") /* with bit6, shows 4 seperate credits */
	PORT_DIPSETTING(    0x00, "4 Players/2 Machines??")	/* with bit6 shows 2 seperate credits */

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
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
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

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )

	PORT_START      /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )

	PORT_START      /* IN7 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN3, 2 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN3, 2 ) /*not sure if this is legal under MAME*/
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN4, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN4, 2 ) /*not sure if this is legal under MAME*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( selfeena )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_JAPAN_NEW_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100k only" )
	PORT_DIPSETTING(    0x08, "200k only" )
	PORT_DIPSETTING(    0x04, "300k only" )
	PORT_DIPSETTING(    0x00, "400k only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

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
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
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
INPUT_PORTS_END

INPUT_PORTS_START( ryujin )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	TAITO_B_DSWA_2_4
	TAITO_COINAGE_JAPAN_NEW_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

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
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
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
INPUT_PORTS_END

INPUT_PORTS_START( sbm )
	PORT_START /* DSW A *///+-ok
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
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* DSW B */ //+-ok
	TAITO_DIFFICULTY_8
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
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )//sound select UP
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )//sound select DOWN
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )//ok (object test)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )//ok (object test)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )//-- unused in test modes
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )//-- unused in test modes
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )//BEN IN (ticket dispenser)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )//LADY ????

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )//select; ok (1P in object test)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )//start ; ok (2P in object test)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT(         0x01, IP_ACTIVE_LOW, IPT_TILT )       //ok
	PORT_BIT(         0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )	//ok
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 2 )   //ok
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 2 )   //ok
	/* BUTTON1 ACTIVE LOW, - game thinks that punching pad has already been raised */
	PORT_BIT(         0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )//PHOTO 1 (punching pad photosensor 1)
	PORT_BIT(         0x20, IP_ACTIVE_HIGH,IPT_BUTTON2 | IPF_PLAYER1 )//PHOTO 2 (punching pad photosensor 2)
	PORT_BIT(         0x40, IP_ACTIVE_HIGH,IPT_BUTTON3 | IPF_PLAYER1 )//PHOTO 3	(punching pad photosensor 3)
 	/*To simulate a punch:
		- wait for "READY GO!" message,
		- press button1 + button 2 (LCTRL + ALT) (you'll hear a "punching" sound),
  		- THEN  press button 3 (SPACE)
 		The time passed between the presses will be used to calculate the power of your punch.
		The longer the time - the less power.
	*/
	PORT_BIT(         0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )//PHOTO 4  ??? ACTIVE_LOW  ??? (punching pad photosensor 4)
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 8, RGN_FRAC(1,2), RGN_FRAC(1,2)+8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};
static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 8, RGN_FRAC(1,2), RGN_FRAC(1,2)+8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 256 },  /* text */
	{ REGION_GFX1, 0, &tilelayout,  0, 256 },  /* sprites & playfield */
	{ -1 } /* end of array */
};

static struct GfxLayout rambo3_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};
static struct GfxLayout rambo3_tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxDecodeInfo rambo3_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &rambo3_charlayout,  0, 256 },  /* text */
	{ REGION_GFX1, 0, &rambo3_tilelayout,  0, 256 },  /* sprites & playfield */
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
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static struct YM2610interface ym2610_interface_crimec =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static struct YM2203interface ym2203_interface =
{
	1,
	3000000,				/* ?? */
	{ YM2203_VOL(80,25) },	/* ?? */
	{ 0 },
	{ 0 },
	{ bankswitch_w },
	{ 0 },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 8000,8000 },			/* ?? */
	{ REGION_SOUND1,REGION_SOUND1 }, /* memory regions */
	{ 50,65 }				/* ?? */
};

/*
	Games that use the mb87078 are: pbobble, spacedx and qzshowby
	schems are not available, but from the writes I guess that
	they only use channel 1
	The sound chips' volume altered with the mb87078 are:
	ym2610 in spacedx,
	ym2610b in pbobble,qzshowby,

	Both ym2610 and ym2610b generate 3 (PSG like) + 2 (fm left,right) channels.
	I use mixer_set_volume() to emulate the effect.
*/
static void mb87078_gain_changed(int channel, int percent)
{
	if (channel==1)
	{
		mixer_set_volume(0,percent);
		mixer_set_volume(1,percent);
		mixer_set_volume(2,percent);
		mixer_set_volume(3,percent);
		mixer_set_volume(4,percent);
		//usrintf_showmessage("MB87078 gain ch#%i percent=%i",channel,percent);
	}
}

static struct MB87078interface mb87078_interface =
{
	mb87078_gain_changed	/*callback function for gain change*/
};


static MACHINE_INIT( mb87078 )
{
	if (Machine->sample_rate != 0)
		MB87078_start(0, &mb87078_interface); /*chip #0*/
/*
	{
		int i;
		for (i=0; i<6; i++)
			logerror("SOUND Chan#%i name=%s\n", i, mixer_get_name(i) );
	}
*/
}


static MACHINE_DRIVER_START( rastsag2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(rastsag2_readmem,rastsag2_writemem)
	MDRV_CPU_VBLANK_INT(rastansaga2_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order0)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_rsaga2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ashura )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(rastsag2_readmem,rastsag2_writemem)
	MDRV_CPU_VBLANK_INT(rastansaga2_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order0)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( crimec )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(crimec_readmem,crimec_writemem)
	MDRV_CPU_VBLANK_INT(crimec_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order1)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tetrist )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ???*/
	MDRV_CPU_MEMORY(tetrist_readmem,tetrist_writemem)
	MDRV_CPU_VBLANK_INT(rastansaga2_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order0)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_rsaga2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hitice )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(hitice_readmem,hitice_writemem)
	MDRV_CPU_VBLANK_INT(hitice_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(hitice_sound_readmem,hitice_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(hitice)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rambo3 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(rambo3_readmem,rambo3_writemem)
	MDRV_CPU_VBLANK_INT(rambo3_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(rambo3_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order0)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rambo3a )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(rambo3_readmem,rambo3_writemem)
	MDRV_CPU_VBLANK_INT(rambo3_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order2)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pbobble )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(pbobble_readmem,pbobble_writemem)
	MDRV_CPU_VBLANK_INT(pbobble_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(mb87078)
	MDRV_NVRAM_HANDLER(taito_b)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order1)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610B, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spacedx )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(spacedx_readmem,spacedx_writemem)
	MDRV_CPU_VBLANK_INT(pbobble_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(mb87078)
	MDRV_NVRAM_HANDLER(taito_b)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order1)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spacedxo )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(spacedxo_readmem,spacedxo_writemem)
	MDRV_CPU_VBLANK_INT(selfeena_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order2)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( qzshowby )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz according to the readme*/
	MDRV_CPU_MEMORY(qzshowby_readmem,qzshowby_writemem)
	MDRV_CPU_VBLANK_INT(pbobble_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(mb87078)
	MDRV_NVRAM_HANDLER(taito_b)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order1)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610B, ym2610_interface_crimec)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( viofight )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(viofight_readmem,viofight_writemem)
	MDRV_CPU_VBLANK_INT(viofight_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(hitice_sound_readmem,hitice_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order2)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

#if 0
static void masterw_patch(void)
{
	data16_t *rom = (data16_t*)memory_region(REGION_CPU1);
	rom[ 0x3fffe/2 ] = 2; //US version
}
#endif

static MACHINE_DRIVER_START( masterw )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(masterw_readmem,masterw_writemem)
	MDRV_CPU_VBLANK_INT(masterw_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(masterw_sound_readmem,masterw_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order2)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( silentd )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz ??? */
	MDRV_CPU_MEMORY(silentd_readmem,silentd_writemem)
	MDRV_CPU_VBLANK_INT(silentd_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order2)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_rsaga2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( selfeena )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(selfeena_readmem,selfeena_writemem)
	MDRV_CPU_VBLANK_INT(selfeena_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order2)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END

#if 0
static void ryujin_patch(void)
{
	data16_t *rom = (data16_t*)memory_region(REGION_CPU1);
	rom[ 0x62/2 ] = 1;
	//0 (already in rom) - Taito Corporation 1993
	//1 - Taito America corp with blue FBI logo
}
#endif

static MACHINE_DRIVER_START( ryujin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(selfeena_readmem,selfeena_writemem)
	MDRV_CPU_VBLANK_INT(selfeena_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order2)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface_crimec)
MACHINE_DRIVER_END

#if 0
static void sbm_patch(void)
{
	data16_t *rom = (data16_t*)memory_region(REGION_CPU1);
	rom[ 0x7ffff/2 ] = 2; //US version
}
#endif

static MACHINE_DRIVER_START( sbm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(sbm_readmem,sbm_writemem)
	MDRV_CPU_VBLANK_INT(sbm_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitob_color_order0)
	MDRV_VIDEO_EOF(taitob)
	MDRV_VIDEO_UPDATE(taitob)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610B, ym2610_interface_crimec)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rastsag2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "b81-08.50" , 0x00000, 0x20000, 0xd6da9169 )
	ROM_LOAD16_BYTE( "b81-07.bin", 0x00001, 0x20000, 0x8edf17d7 )
	ROM_LOAD16_BYTE( "b81-10.49" , 0x40000, 0x20000, 0x53f34344 )
	ROM_LOAD16_BYTE( "b81-09.30" , 0x40001, 0x20000, 0x630d34af )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "b81-11.bin", 0x00000, 0x4000, 0x3704bf09 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b81-03.bin", 0x000000, 0x080000, 0x551b75e6 )
	ROM_LOAD( "b81-04.bin", 0x080000, 0x080000, 0xcf734e12 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b81-01.bin", 0x00000, 0x80000, 0xb33f796b )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "b81-02.bin", 0x00000, 0x80000, 0x20ec3b86 )
ROM_END

ROM_START( nastarw )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "b81-08.50" , 0x00000, 0x20000, 0xd6da9169 )
	ROM_LOAD16_BYTE( "b81-12.31",  0x00001, 0x20000, 0xf9d82741 )
	ROM_LOAD16_BYTE( "b81-10.49" , 0x40000, 0x20000, 0x53f34344 )
	ROM_LOAD16_BYTE( "b81-09.30" , 0x40001, 0x20000, 0x630d34af )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "b81-11.bin", 0x00000, 0x4000, 0x3704bf09 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b81-03.bin", 0x000000, 0x080000, 0x551b75e6 )
	ROM_LOAD( "b81-04.bin", 0x080000, 0x080000, 0xcf734e12 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b81-01.bin", 0x00000, 0x80000, 0xb33f796b )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "b81-02.bin", 0x00000, 0x80000, 0x20ec3b86 )
ROM_END

ROM_START( nastar )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "b81-08.50" , 0x00000, 0x20000, 0xd6da9169 )
	ROM_LOAD16_BYTE( "b81-13.bin", 0x00001, 0x20000, 0x60d176fb )
	ROM_LOAD16_BYTE( "b81-10.49" , 0x40000, 0x20000, 0x53f34344 )
	ROM_LOAD16_BYTE( "b81-09.30" , 0x40001, 0x20000, 0x630d34af )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "b81-11.bin", 0x00000, 0x4000, 0x3704bf09 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b81-03.bin", 0x000000, 0x080000, 0x551b75e6 )
	ROM_LOAD( "b81-04.bin", 0x080000, 0x080000, 0xcf734e12 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b81-01.bin", 0x00000, 0x80000, 0xb33f796b )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "b81-02.bin", 0x00000, 0x80000, 0x20ec3b86 )
ROM_END

ROM_START( crimec )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "b99_07.40",    0x00000, 0x20000, 0x26e886e6 )
	ROM_LOAD16_BYTE( "b99_05.29",    0x00001, 0x20000, 0xff7f9a9d )
	ROM_LOAD16_BYTE( "b99_06.39",    0x40000, 0x20000, 0x1f26aa92 )
	ROM_LOAD16_BYTE( "b99_14.28",    0x40001, 0x20000, 0x71c8b4d7 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "b99_08.45", 0x00000, 0x4000, 0x26135451 )
	ROM_CONTINUE(          0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b99_02.18", 0x000000, 0x080000, 0x2a5d4a26 )
	ROM_LOAD( "b99_01.19", 0x080000, 0x080000, 0xa19e373a )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b99_03.37", 0x000000, 0x080000, 0xdda10df7 )
ROM_END

ROM_START( crimecu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "b99_07.40",    0x00000, 0x20000, 0x26e886e6 )
	ROM_LOAD16_BYTE( "b99_05.29",    0x00001, 0x20000, 0xff7f9a9d )
	ROM_LOAD16_BYTE( "b99_06.39",    0x40000, 0x20000, 0x1f26aa92 )
	ROM_LOAD16_BYTE( "b99_13.28",    0x40001, 0x20000, 0x06cf8441 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "b99_08.45", 0x00000, 0x4000, 0x26135451 )
	ROM_CONTINUE(          0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b99_02.18", 0x000000, 0x080000, 0x2a5d4a26 )
	ROM_LOAD( "b99_01.19", 0x080000, 0x080000, 0xa19e373a )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b99_03.37", 0x000000, 0x080000, 0xdda10df7 )
ROM_END

ROM_START( crimecj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "b99_07.40", 0x00000, 0x20000, 0x26e886e6 )
	ROM_LOAD16_BYTE( "b99_05.29", 0x00001, 0x20000, 0xff7f9a9d )
	ROM_LOAD16_BYTE( "b99_06.39", 0x40000, 0x20000, 0x1f26aa92 )
	ROM_LOAD16_BYTE( "b99_15.28", 0x40001, 0x20000, 0xe8c1e56d )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "b99_08.45", 0x00000, 0x4000, 0x26135451 )
	ROM_CONTINUE(          0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b99_02.18", 0x000000, 0x080000, 0x2a5d4a26 )
	ROM_LOAD( "b99_01.19", 0x080000, 0x080000, 0xa19e373a )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b99_03.37", 0x000000, 0x080000, 0xdda10df7 )
ROM_END

ROM_START( ashura )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c4307-1.50", 0x00000, 0x20000, 0xd5ceb20f )
	ROM_LOAD16_BYTE( "c4305-1.31", 0x00001, 0x20000, 0xa6f3bb37 )
	ROM_LOAD16_BYTE( "c4306-1.49", 0x40000, 0x20000, 0x0f331802 )
	ROM_LOAD16_BYTE( "c4304-1.30", 0x40001, 0x20000, 0xe06a2414 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "c43-16",  0x00000, 0x4000, 0xcb26fce1 )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c43-02",  0x00000, 0x80000, 0x105722ae )
	ROM_LOAD( "c43-03",  0x80000, 0x80000, 0x426606ba )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "c43-01",  0x00000, 0x80000, 0xdb953f37 )
ROM_END

ROM_START( ashurau )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c43-11", 0x00000, 0x20000, 0xd5aefc9b )
	ROM_LOAD16_BYTE( "c43-09", 0x00001, 0x20000, 0xe91d0ab1 )
	ROM_LOAD16_BYTE( "c43-10", 0x40000, 0x20000, 0xc218e7ea )
	ROM_LOAD16_BYTE( "c43-08", 0x40001, 0x20000, 0x5ef4f19f )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "c43-16",  0x00000, 0x4000, 0xcb26fce1 )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c43-02",  0x00000, 0x80000, 0x105722ae )
	ROM_LOAD( "c43-03",  0x80000, 0x80000, 0x426606ba )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "c43-01",  0x00000, 0x80000, 0xdb953f37 )
ROM_END

ROM_START( tetrist )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c12-03.bin", 0x000000, 0x020000, 0x38f1ed41 )
	ROM_LOAD16_BYTE( "c12-02.bin", 0x000001, 0x020000, 0xed9530bc )
	ROM_LOAD16_BYTE( "c12-05.bin", 0x040000, 0x020000, 0x128e9927 )
	ROM_LOAD16_BYTE( "c12-04.bin", 0x040001, 0x020000, 0x5da7a319 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "c12-06.bin", 0x00000, 0x4000, 0xf2814b38 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	/* empty (uses only pixel layer) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* adpcm samples */
	/* empty */

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* DELTA-T samples */
	/* empty */
ROM_END

ROM_START( hitice )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c59-10", 0x00000, 0x20000, 0xe4ffad15 )
	ROM_LOAD16_BYTE( "c59-12", 0x00001, 0x20000, 0xa080d7af )
	ROM_LOAD16_BYTE( "c59-09", 0x40000, 0x10000, 0xe243e3b0 )
	ROM_LOAD16_BYTE( "c59-11", 0x40001, 0x10000, 0x4d4dfa52 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "c59-08",  0x00000, 0x4000, 0xd3cbc10b )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c59-03",  0x00000, 0x80000, 0x9e513048 )
	ROM_LOAD( "c59-02",  0x80000, 0x80000, 0xaffb5e07 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "c59-01",  0x00000, 0x20000, 0x46ae291d )
ROM_END

ROM_START( rambo3 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "r3-0e.rom",  0x00000, 0x10000, 0x3efa4177 )
	ROM_LOAD16_BYTE( "r3-0o.rom",  0x00001, 0x10000, 0x55c38d92 )

/*NOTE: there is a hole in address space here */

	ROM_LOAD16_BYTE( "r3-1e.rom" , 0x40000, 0x20000, 0x40e363c7 )
	ROM_LOAD16_BYTE( "r3-1o.rom" , 0x40001, 0x20000, 0x7f1fe6ab )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "r3-00.rom", 0x00000, 0x4000, 0xdf7a6ed6 )
	ROM_CONTINUE(          0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
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

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "r3-a1.rom", 0x00000, 0x20000, 0x4396fa19 )
	ROM_LOAD( "r3-a2.rom", 0x20000, 0x20000, 0x41fe53a8 )
	ROM_LOAD( "r3-a3.rom", 0x40000, 0x20000, 0xe89249ba )
	ROM_LOAD( "r3-a4.rom", 0x60000, 0x20000, 0x9cf4c21b )
ROM_END

ROM_START( rambo3a )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "ramb3-11.bin",  0x00000, 0x20000, 0x1cc42247 )
	ROM_LOAD16_BYTE( "ramb3-13.bin",  0x00001, 0x20000, 0x0a964cb7 )
	ROM_LOAD16_BYTE( "ramb3-07.bin",  0x40000, 0x20000, 0xc973ff6f )
	ROM_LOAD16_BYTE( "ramb3-06.bin",  0x40001, 0x20000, 0xa83d3fd5 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "ramb3-10.bin", 0x00000, 0x4000, 0xb18bc020 )
	ROM_CONTINUE(             0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ramb3-03.bin",  0x000000, 0x80000, 0xf5808c41 )
	ROM_LOAD( "ramb3-04.bin",  0x080000, 0x80000, 0xc57831ce )
	ROM_LOAD( "ramb3-01.bin",  0x100000, 0x80000, 0xc55fcf54 )
	ROM_LOAD( "ramb3-02.bin",  0x180000, 0x80000, 0x9dd014c6 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "ramb3-05.bin", 0x00000, 0x80000, 0x0179dc40 )
ROM_END

ROM_START( rambo3ae )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "ramb3-11.bin",  0x00000, 0x20000, 0x1cc42247 )
	ROM_LOAD16_BYTE( "ramb3-14.bin",  0x00001, 0x20000, 0x7d917c21 )
	ROM_LOAD16_BYTE( "ramb3-07.bin",  0x40000, 0x20000, 0xc973ff6f )
	ROM_LOAD16_BYTE( "ramb3-06.bin",  0x40001, 0x20000, 0xa83d3fd5 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "ramb3-10.bin", 0x00000, 0x4000, 0xb18bc020 )
	ROM_CONTINUE(             0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ramb3-03.bin",  0x000000, 0x80000, 0xf5808c41 )
	ROM_LOAD( "ramb3-04.bin",  0x080000, 0x80000, 0xc57831ce )
	ROM_LOAD( "ramb3-01.bin",  0x100000, 0x80000, 0xc55fcf54 )
	ROM_LOAD( "ramb3-02.bin",  0x180000, 0x80000, 0x9dd014c6 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "ramb3-05.bin", 0x00000, 0x80000, 0x0179dc40 )
ROM_END

ROM_START( pbobble )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "pb-1c18.bin", 0x00000, 0x40000, 0x5de14f49 )
	ROM_LOAD16_BYTE( "pb-ic2.bin",  0x00001, 0x40000, 0x2abe07d1 )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 128k for Z80 code */
	ROM_LOAD( "pb-ic27.bin", 0x00000, 0x04000, 0x26efa4c4 )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pb-ic14.bin", 0x00000, 0x80000, 0x55f90ea4 )
	ROM_LOAD( "pb-ic9.bin",  0x80000, 0x80000, 0x3253aac9 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "pb-ic15.bin", 0x000000, 0x100000, 0x0840cbc4 )
ROM_END

ROM_START( spacedx )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "d89-06",       0x00000, 0x40000, 0x7122751e )
	ROM_LOAD16_BYTE( "si-dxusa.ic2", 0x00001, 0x40000, 0x50a4b8d1 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "d89-07.27", 0x00000, 0x4000, 0xbd743401 )
	ROM_CONTINUE(          0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d89-02.14", 0x00000, 0x80000, 0xc36544b9 )
	ROM_LOAD( "d89-01.9",  0x80000, 0x80000, 0xfffa0660 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* adpcm samples */
	ROM_LOAD( "d89-03.15", 0x00000, 0x80000, 0x218f31a4 )
ROM_END

ROM_START( spacedxj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "d89-06", 0x00000, 0x40000, 0x7122751e )
	ROM_LOAD16_BYTE( "d89-05", 0x00001, 0x40000, 0xbe1638af )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "d89-07.27", 0x00000, 0x4000, 0xbd743401 )
	ROM_CONTINUE(          0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d89-02.14", 0x00000, 0x80000, 0xc36544b9 )
	ROM_LOAD( "d89-01.9" , 0x80000, 0x80000, 0xfffa0660 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* adpcm samples */
	ROM_LOAD( "d89-03.15", 0x00000, 0x80000, 0x218f31a4 )
ROM_END

ROM_START( spacedxo )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "d89-08.bin",  0x00000, 0x20000, 0x0c2fe7f9 )
	ROM_LOAD16_BYTE( "d89-09j.bin", 0x00001, 0x20000, 0x7f0a0ba4 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "d89-07.27", 0x00000, 0x4000, 0xbd743401 )
	ROM_CONTINUE(          0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d89-12.bin",0x00000, 0x80000, 0x53df86f1 )
	ROM_LOAD( "d89-13.bin",0x80000, 0x80000, 0xc44c1352 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* adpcm samples */
	ROM_LOAD( "d89-03.15", 0x00000, 0x80000, 0x218f31a4 )
ROM_END

ROM_START( qzshowby )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 1M for 68000 code */
	ROM_LOAD16_BYTE( "d72-13.bin", 0x00000, 0x80000, 0xa867759f )
	ROM_LOAD16_BYTE( "d72-12.bin", 0x00001, 0x80000, 0x522c09a7 )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 128k for Z80 code */
	ROM_LOAD(  "d72-11.bin", 0x00000, 0x04000, 0x2ca046e2 )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d72-03.bin", 0x000000, 0x200000, 0x1de257d0 )
	ROM_LOAD( "d72-02.bin", 0x200000, 0x200000, 0xbf0da640 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "d72-01.bin", 0x00000, 0x200000, 0xb82b8830 )
ROM_END

ROM_START( viofight )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )     /* 1M for 68000 code */
	ROM_LOAD16_BYTE( "c16-11.rom", 0x00000, 0x10000, 0x23dbd388 )
	ROM_LOAD16_BYTE( "c16-14.rom", 0x00001, 0x10000, 0xdc934f6a )
	ROM_LOAD16_BYTE( "c16-07.rom", 0x40000, 0x20000, 0x64d1d059 )
	ROM_LOAD16_BYTE( "c16-06.rom", 0x40001, 0x20000, 0x043761d8 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 128k for Z80 code */
	ROM_LOAD(  "c16-12.rom", 0x00000, 0x04000, 0x6fb028c7 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c16-01.rom", 0x000000, 0x080000, 0x7059ce83 )
	ROM_LOAD( "c16-02.rom", 0x080000, 0x080000, 0xb458e905 )
	ROM_LOAD( "c16-03.rom", 0x100000, 0x080000, 0x515a9431 )
	ROM_LOAD( "c16-04.rom", 0x180000, 0x080000, 0xebf285e2 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "c16-05.rom", 0x000000, 0x80000, 0xa49d064a )
ROM_END

ROM_START( masterw )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "b72-06.rom"   , 0x00000, 0x20000, 0xae848eff )
	ROM_LOAD16_BYTE( "b72-12.rom"   , 0x00001, 0x20000, 0x7176ce70 )
	ROM_LOAD16_BYTE( "b72-04.rom"   , 0x40000, 0x20000, 0x141e964c )
	ROM_LOAD16_BYTE( "b72-03.rom"   , 0x40001, 0x20000, 0xf4523496 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "b72-07.rom", 0x00000, 0x4000, 0x2b1a946f )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mow-m02.rom", 0x000000, 0x080000, 0xc519f65a )
	ROM_LOAD( "mow-m01.rom", 0x080000, 0x080000, 0xa24ac26e )
ROM_END

ROM_START( silentd )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "sr_12-1.rom", 0x00000, 0x20000, 0x5883d362 )
	ROM_LOAD16_BYTE( "sr_15-1.rom", 0x00001, 0x20000, 0x8c0a72ae )
	ROM_LOAD16_BYTE( "sr_11.rom",   0x40000, 0x20000, 0x35da4428 )
	ROM_LOAD16_BYTE( "sr_09.rom",   0x40001, 0x20000, 0x2f05b14a )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD(  "sr_13.rom", 0x00000, 0x04000, 0x651861ab )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sd_m04.rom", 0x000000, 0x100000, 0x53237217 )
	ROM_LOAD( "sd_m06.rom", 0x100000, 0x100000, 0xe6e6dfa7 )
	ROM_LOAD( "sd_m03.rom", 0x200000, 0x100000, 0x1b9b2846 )
	ROM_LOAD( "sd_m05.rom", 0x300000, 0x100000, 0xe02472c5 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "sd_m02.rom", 0x00000, 0x80000, 0xe0de5c39 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "sd_m01.rom", 0x00000, 0x80000, 0xb41fff1a )
ROM_END

ROM_START( silentdj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "sr_12-1.rom", 0x00000, 0x20000, 0x5883d362 )
	ROM_LOAD16_BYTE( "10-1.10",     0x00001, 0x20000, 0x584306fc )
	ROM_LOAD16_BYTE( "sr_11.rom",   0x40000, 0x20000, 0x35da4428 )
	ROM_LOAD16_BYTE( "sr_09.rom",   0x40001, 0x20000, 0x2f05b14a )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD(  "sr_13.rom", 0x00000, 0x04000, 0x651861ab )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sd_m04.rom", 0x000000, 0x100000, 0x53237217 )
	ROM_LOAD( "sd_m06.rom", 0x100000, 0x100000, 0xe6e6dfa7 )
	ROM_LOAD( "sd_m03.rom", 0x200000, 0x100000, 0x1b9b2846 )
	ROM_LOAD( "sd_m05.rom", 0x300000, 0x100000, 0xe02472c5 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "sd_m02.rom", 0x00000, 0x80000, 0xe0de5c39 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "sd_m01.rom", 0x00000, 0x80000, 0xb41fff1a )
ROM_END

ROM_START( selfeena )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "se-02.27", 0x00000, 0x20000, 0x08f0c8e3 )
	ROM_LOAD16_BYTE( "se-01.26", 0x00001, 0x20000, 0xa06ca64b )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "se-03.39",0x00000, 0x4000, 0x675998be )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "se-04.2",  0x000000, 0x80000, 0x920ad100 )
	ROM_LOAD( "se-05.1",  0x080000, 0x80000, 0xd297c995 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* adpcm samples */
	ROM_LOAD( "se-06.11", 0x00000, 0x80000, 0x80d5e772 )
ROM_END

ROM_START( ryujin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "ruj02.27", 0x00000, 0x20000, 0x0d223aee )
	ROM_LOAD16_BYTE( "ruj01.26", 0x00001, 0x20000, 0xc6bcdd1e )
	ROM_LOAD16_BYTE( "ruj04.29", 0x40000, 0x20000, 0x0c153cab )
	ROM_LOAD16_BYTE( "ruj03.28", 0x40001, 0x20000, 0x7695f89c )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "ruj05.39",0x00000, 0x4000, 0x95270b16 )
	ROM_CONTINUE(        0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ryujin07.2", 0x000000, 0x100000, 0x34f50980 )
	ROM_LOAD( "ryujin06.1", 0x100000, 0x100000, 0x1b85ff34 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* adpcm samples */
	ROM_LOAD( "ryujin08.11", 0x00000, 0x80000, 0x480d040d )
ROM_END

ROM_START( sbm )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "c69-20-1.10", 0x00000, 0x20000, 0xb40e4910 )
	ROM_LOAD16_BYTE( "c69-22-1.12", 0x00001, 0x20000, 0xecbcf830 )
	ROM_LOAD16_BYTE( "c69-19-1.9" , 0x40000, 0x20000, 0x5719c158 )
	ROM_LOAD16_BYTE( "c69-21-1.11", 0x40001, 0x20000, 0x73562394 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "c69-23.28",0x00000, 0x4000, 0xb2fce13e )
	ROM_CONTINUE(         0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD       ( "c69-01.ic5", 0x000000, 0x100000, 0x521fabe3 )
	ROM_LOAD16_BYTE( "c69-13.ic2", 0x100000, 0x020000, 0xd1550884 )
	ROM_LOAD16_BYTE( "c69-12.ic1", 0x100001, 0x020000, 0xeb56582c )
	/* 140000-1fffff empty */
	ROM_LOAD       ( "c69-02.ic6", 0x200000, 0x100000, 0xf0e20d35 )
	ROM_LOAD16_BYTE( "c69-15.ic4", 0x300000, 0x020000, 0x9761d316 )
	ROM_LOAD16_BYTE( "c69-14.ic3", 0x300001, 0x020000, 0x0ed0272a )
	/* 340000-3fffff empty */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* adpcm samples */
	ROM_LOAD( "c69-03.36", 0x00000, 0x80000, 0x63e6b6e7 )
ROM_END


GAME( 1989, masterw,  0,       masterw,  masterw,  0, ROT270, "Taito Corporation Japan", "Master of Weapon (World)" )
GAME( 1988, nastar,   0,       rastsag2, nastar,   0, ROT0,   "Taito Corporation Japan", "Nastar (World)" )
GAME( 1988, nastarw,  nastar,  rastsag2, nastarw,  0, ROT0,   "Taito America Corporation", "Nastar Warrior (US)" )
GAME( 1988, rastsag2, nastar,  rastsag2, rastsag2, 0, ROT0,   "Taito Corporation", "Rastan Saga 2 (Japan)" )
GAME( 1989, rambo3,   0,       rambo3,   rambo3,   0, ROT0,   "Taito Europe Corporation", "Rambo III (Europe set 1)" )
GAME( 1989, rambo3ae, rambo3,  rambo3a,  rambo3a,  0, ROT0,   "Taito Europe Corporation", "Rambo III (Europe set 2)" )
GAME( 1989, rambo3a,  rambo3,  rambo3a,  rambo3a,  0, ROT0,   "Taito America Corporation", "Rambo III (US)" )
GAME( 1989, crimec,   0,       crimec,   crimec,   0, ROT0,   "Taito Corporation Japan", "Crime City (World)" )
GAME( 1989, crimecu,  crimec,  crimec,   crimecu,  0, ROT0,   "Taito America Corporation", "Crime City (US)" )
GAME( 1989, crimecj,  crimec,  crimec,   crimecj,  0, ROT0,   "Taito Corporation", "Crime City (Japan)" )
GAME( 1989, tetrist,  tetris,  tetrist,  tetrist,  0, ROT0,   "Sega", "Tetris (Japan, B-System)" )
GAME( 1989, viofight, 0,       viofight, viofight, 0, ROT0,   "Taito Corporation Japan", "Violence Fight (World)" )
GAME( 1990, ashura,   0,       ashura,   ashura,   0, ROT270, "Taito Corporation", "Ashura Blaster (Japan)" )
GAME( 1990, ashurau,  ashura,  ashura,   ashurau,  0, ROT270, "Taito America Corporation", "Ashura Blaster (US)" )
GAME( 1990, hitice,   0,       hitice,   hitice,   0, ROT0,   "Williams", "Hit the Ice (US)" )
GAME( 1991, selfeena, 0,       selfeena, selfeena, 0, ROT0,   "East Technology", "Sel Feena" )
GAME( 1992, silentd,  0,       silentd,  silentd,  0, ROT0,   "Taito Corporation Japan", "Silent Dragon (World)" )
GAME( 1992, silentdj, silentd, silentd,  silentdj, 0, ROT0,   "Taito Corporation", "Silent Dragon (Japan)" )
GAME( 1993, ryujin,   0,       ryujin,   ryujin,   0, ROT270, "Taito Corporation", "Ryu Jin (Japan)" )
GAME( 1993, qzshowby, 0,       qzshowby, qzshowby, 0, ROT0,   "Taito Corporation", "Quiz Sekai wa SHOW by shobai (Japan)" )
GAME( 1994, pbobble,  0,       pbobble,  pbobble,  0, ROT0,   "Taito Corporation", "Puzzle Bobble (Japan, B-System)" )
GAME( 1994, spacedx,  0,       spacedx,  pbobble,  0, ROT0,   "Taito Corporation", "Space Invaders DX (US) v2.1" )
GAME( 1994, spacedxj, spacedx, spacedx,  pbobble,  0, ROT0,   "Taito Corporation", "Space Invaders DX (Japan) v2.1" )
GAME( 1994, spacedxo, spacedx, spacedxo, spacedxo, 0, ROT0,   "Taito Corporation", "Space Invaders DX (Japan) v2.0" )
/*
	Sonic Blast Man is a ticket dispensing game.
	(Japanese version however does not dispense them, only US does - try the "sbm_patch" in the machine_driver).
	It is a bit different from other games running on this system,
	in that it has a punching pad that player needs to punch to hit
 	the enemy.
*/
GAME(  1990, sbm,      0,       sbm,      sbm,      0, ROT0,   "Taito Corporation", "Sonic Blast Man (Japan)" )
