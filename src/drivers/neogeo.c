/***************************************************************************
	M.A.M.E. Neo Geo driver presented to you by the Shin Emu Keikaku team.

	The following people have all spent probably far too much time on this:

	AVDB
	Bryan McPhail
	Fuzz
	Ernesto Corvi
	Andrew Prime


	GRAPHICAL ISSUES :

	- Effects created using the Raster Interrupt are probably not 100% correct,
	  e.g.:
	  - full screen zoom in trally and tpgolf is broken again :-( I think this was
	    caused by the fix for kof94 japan stage.
	  - Tests on the hardware show that there are 264 raster lines; however, there
	    are one or two line alignemnt issues with some games, SCANLINE_ADJUST is
	    a kludge to get the alignment almost right in most cases.
	    Some good places to test raster effects handling and alignment:
	    - aodk 100 mega shock logo
		- viewpoin Sammy logo
	    - zedblade parallax scrolling
		- ridhero road
	    - turfmast Japan course hole 4 (the one with the waterfall)
	    - fatfury3, 7th stage (Port Town). Raster effects are used for the background.
	  - spinmast uses IRQ2 with no noticeable effect (it seems to be always near
	    the bottom of the screen).
	  - garoup enables IRQ2 on Terry's stage, but with no noticeable effect. Note
	    that it is NOT enabled in 2 players mode, only vs cpu.
	  - strhoop enables IRQ2 on every scanline during attract mode, with no
	    noticeable effect.
	  - Money Idol Exchanger runs slow during the "vs. Computer" mode. Solo mode
	    works fine.

	- Full screen zoom has some glitches in tpgolf.

	- Gururin has bad tiles all over the place (used to work ..)

	- Bad clipping during scrolling at the sides on some games.
		(tpgolf for example)

	AUDIO ISSUES :

	- Sound (Music) was cutting out in ncommand and ncombat due to a bug in the
	  original code, which should obviously have no ill effect on the real YM2610 but
	  confused the emulated one. This is fixed in the YM2610 emulator.

	- Some rather bad sounding parts in a couple of Games
		(shocktro End of Intro)

	- In mahretsu music should stop when you begin play (correct after a continue) *untested*

	GAMEPLAY ISSUES / LOCKUPS :

	- Viewpoint resets halfway through level 1. This is a bug in the asm 68k core.

	- magdrop2 behaves strangely when P2 wins a 2 Player game (reports both as losing)

	- popbounc without a patch this locks up when sound is disabled, also for this game 'paddle'
	  conroller can be selected in the setup menus, but Mame doesn't support this.

	- ssideki2 locks up sometimes during play *not tested recently, certainly used to*

	- 2020bb apparently resets when the batter gets hit by the pitcher *not tested*

	- some games apparently crash / reset when you finish them before you get the ending *untested*

	- fatfury3 locks up when you complete the game.

	NON-ISSUES / FIXED ISSUES :

	- Auto Animation Speed is not quite right in Many Games
		(mslug waterfalls, some bg's in samsho4, blazstar lev 2 etc.)

	- shocktro locking up at the bosses, this was fixed a long long time ago, it was due to bugs
	  in the 68k Core.

	- sound, graphic, the odd game crash & any other strange happenings in kof99p and garoup are
	  probably because these machines are prototypes, the games are therefore not finished.  There
	  are 'patched' versions of these romsets available in some locations, however these will not
	  be supported.

	OTHER MINOR THINGS :

	- 2020bb version display, the program roms contains two version numbers, the one which always
	  get displayed when running in Mame is that which would be displayed on a console.
	  This depends on location 0x46 of nvram, which should be set in the arcade version. There
	  doesn't seem to be a place in the BIOS where this location is initialized.

	NOTES ABOUT UNSUPPORTED GAMES :

	- Diggerman (???, 2000) - Not A Real Arcade Game .. Will Not Be Supported.

=============================================================================

Points to note, known and proven information deleted from this map:

	0x3000001		Dipswitches
				bit 0 : Selftest
				bit 1 : Unknown (Unused ?) \ something to do with
				bit 2 : Unknown (Unused ?) / auto repeating keys ?
				bit 3 : \
				bit 4 :  | communication setting ?
				bit 5 : /
				bit 6 : free play
				bit 7 : stop mode ?

	0x320001		bit 0 : COIN 1
				bit 1 : COIN 2
				bit 2 : SERVICE
				bit 3 : UNKNOWN
				bit 4 : UNKNOWN
				bit 5 : UNKNOWN
				bit 6 : 4990 test pulse bit.
				bit 7 : 4990 data bit.

	0x380051		4990 control write register
				bit 0: C0
				bit 1: C1
				bit 2: C2
				bit 3-7: unused.

				0x02 = shift.
				0x00 = register hold.
				0x04 = ????.
				0x03 = time read (reset register).

	0x3c000c		IRQ acknowledge

	0x380011		Backup bank select

	0x3a0001		Enable display.
	0x3a0011		Disable display

	0x3a0003		Swap in Bios (0x80 bytes vector table of BIOS)
	0x3a0013		Swap in Rom  (0x80 bytes vector table of ROM bank)

	0x3a000d		lock backup ram
	0x3a001d		unlock backup ram

	0x3a000b		set game vector table (?)  mirror ?
	0x3a001b		set bios vector table (?)  mirror ?

	0x3a000c		Unknown (ghost pilots)
	0x31001c		Unknown (ghost pilots)

	IO word read

	0x3c0002		return vidram word (pointed to by 0x3c0000)
	0x3c0006		?????.
	0x3c0008		shadow adress for 0x3c0000 (not confirmed).
	0x3c000a		shadow adress for 0x3c0002 (confirmed, see
							   Puzzle de Pon).
	IO word write

	0x3c0006		Unknown, set vblank counter (?)

	0x3c0008		shadow address for 0x3c0000 (not confirmed)
	0x3c000a		shadow address for 0x3c0002 (not confirmed)

	The Neo Geo contains an NEC 4990 Serial I/O calendar & clock.
	accesed through 0x320001, 0x380050, 0x280050 (shadow adress).
	A schematic for this device can be found on the NEC webpages.

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/pd4990a.h"
#include "cpu/z80/z80.h"
#include "neogeo.h"


/* values probed by Razoola from the real board */
#define RASTER_LINES 264
#define RASTER_COUNTER_START 0x1f0	/* value assumed right after vblank */
#define RASTER_COUNTER_RELOAD 0x0f8	/* value assumed after 0x1ff */
#define RASTER_LINE_RELOAD (0x200 - RASTER_COUNTER_START)

#define SCANLINE_ADJUST 3	/* in theory should be 0, give or take an off-by-one mistake */

/******************************************************************************/

unsigned int neogeo_frame_counter;
unsigned int neogeo_frame_counter_speed=4;

/******************************************************************************/

static int irq2start=1000,irq2control;
static int current_rastercounter,current_rasterline,scanline_read;
static UINT32 irq2pos_value;
static int vblank_int,scanline_int;

/*	flags for irq2control:

	0x07 unused? kof94 sets some random combination of these at the character
		 selection screen but only because it does andi.w #$ff2f, $3c0006. It
		 clears them immediately after.

	0x08 shocktro2, stops autoanim counter

	Maybe 0x07 writes to the autoanim counter, meaning that in conjunction with
	0x08 one could fine control it. However, if that was the case, writing the
	the IRQ2 control bits would interfere with autoanimation, so I'm probably
	wrong.

	0x10 irq2 enable, tile engine scanline irq that is triggered
	when a certain scanline is reached.

	0x20 when set, the next values written in the irq position register
	sets irq2 to happen N lines after the current one

	0x40 when set, irq position register is automatically loaded at vblank to
	set the irq2 line.

	0x80 when set, every time irq2 is triggered the irq position register is
	automatically loaded to set the next irq2 line.

	0x80 and 0x40 may be set at the same time (Viewpoint does this).
*/

#define IRQ2CTRL_AUTOANIM_STOP		0x08
#define IRQ2CTRL_ENABLE				0x10
#define IRQ2CTRL_LOAD_RELATIVE		0x20
#define IRQ2CTRL_AUTOLOAD_VBLANK	0x40
#define IRQ2CTRL_AUTOLOAD_REPEAT	0x80


static void update_interrupts(void)
{
	int level = 0;

	/* determine which interrupt is active */
	if (vblank_int) level = 1;
	if (scanline_int) level = 2;

	/* either set or clear the appropriate lines */
	if (level)
		cpu_set_irq_line(0, level, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}

static WRITE16_HANDLER( neo_irqack_w )
{
	if (ACCESSING_LSB)
	{
		if (data & 4) vblank_int = 0;
		if (data & 2) scanline_int = 0;
		update_interrupts();
	}
}


static INTERRUPT_GEN( neogeo_interrupt )
{
	static int fc=0;
	int line = RASTER_LINES - cpu_getiloops();

	current_rasterline = line;

	{
		int l = line;

		if (l == RASTER_LINES) l = 0;	/* vblank */
		if (l < RASTER_LINE_RELOAD)
			current_rastercounter = RASTER_COUNTER_START + l;
		else
			current_rastercounter = RASTER_COUNTER_RELOAD + l - RASTER_LINE_RELOAD;
	}

	if (line == RASTER_LINES)	/* vblank */
	{
		current_rasterline = 0;

		/* Add a timer tick to the pd4990a */
		pd4990a_addretrace();

		/* Animation counter */
		if (!(irq2control & IRQ2CTRL_AUTOANIM_STOP))
		{
			if (fc>neogeo_frame_counter_speed)
			{
				fc=0;
				neogeo_frame_counter++;
			}
			fc++;
		}

		if (irq2control & IRQ2CTRL_ENABLE)
			usrintf_showmessage("IRQ2 enabled, need raster driver");

		/* return a standard vblank interrupt */
		vblank_int = 1;	   /* vertical blank */
	}

	update_interrupts();
}


static void raster_interrupt(int busy)
{
	static int fc=0;
	int line = RASTER_LINES - cpu_getiloops();
	int do_refresh = 0;
	static int neogeo_raster_enable = 1;

	current_rasterline = line;

	{
		int l = line;

		if (l == RASTER_LINES) l = 0;	/* vblank */
		if (l < RASTER_LINE_RELOAD)
			current_rastercounter = RASTER_COUNTER_START + l;
		else
			current_rastercounter = RASTER_COUNTER_RELOAD + l - RASTER_LINE_RELOAD;
	}

	if (busy)
	{
		if (neogeo_raster_enable && scanline_read)
		{
			do_refresh = 1;
//logerror("partial refresh at raster line %d (raster counter %03x)\n",line,current_rastercounter);
			scanline_read = 0;
		}
	}

	if (irq2control & IRQ2CTRL_ENABLE)
	{
		if (line == irq2start)
		{
//logerror("trigger IRQ2 at raster line %d (raster counter %d)\n",line,current_rastercounter);
			if (!busy)
			{
				if (neogeo_raster_enable)
					do_refresh = 1;
			}

			if (irq2control & IRQ2CTRL_AUTOLOAD_REPEAT)
				irq2start += (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */

			scanline_int = 1;
		}
	}

	if (line == RASTER_LINES)	/* vblank */
	{
		current_rasterline = 0;

		if (keyboard_pressed_memory(KEYCODE_F1))
		{
			neogeo_raster_enable ^= 1;
			usrintf_showmessage("raster effects %sabled",neogeo_raster_enable ? "en" : "dis");
		}

		if (irq2control & IRQ2CTRL_AUTOLOAD_VBLANK)
			irq2start = (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */
		else
			irq2start = 1000;


		/* Add a timer tick to the pd4990a */
		pd4990a_addretrace();

		/* Animation counter */
		if (!(irq2control & IRQ2CTRL_AUTOANIM_STOP))
		{
			if (fc>neogeo_frame_counter_speed)
			{
				fc=0;
				neogeo_frame_counter++;
			}
			fc++;
		}

		/* return a standard vblank interrupt */
//logerror("trigger IRQ1\n");
		vblank_int = 1;	   /* vertical blank */
	}

	if (do_refresh)
	{
		if (line > RASTER_LINE_RELOAD)	/* avoid unnecessary updates after start of vblank */
			force_partial_update((current_rastercounter - 256) - 1 + SCANLINE_ADJUST);
	}

	update_interrupts();
}

static INTERRUPT_GEN( neogeo_raster_interrupt )
{
	raster_interrupt(0);
}

static INTERRUPT_GEN( neogeo_raster_interrupt_busy )
{
	raster_interrupt(1);
}



static int pending_command;
static int result_code;

/* Calendar, coins + Z80 communication */
static READ16_HANDLER( timer16_r )
{
	data16_t res;
	int coinflip = pd4990a_testbit_r(0);
	int databit = pd4990a_databit_r(0);

//	logerror("CPU %04x - Read timer\n",activecpu_get_pc());

	res = (readinputport(4) & ~(readinputport(5) & 0x20)) ^ (coinflip << 6) ^ (databit << 7);

	if (Machine->sample_rate)
	{
		res |= result_code << 8;
		if (pending_command) res &= 0x7fff;
	}
	else
		res |= 0x0100;

	return res;
}

static WRITE16_HANDLER( neo_z80_w )
{
	if (ACCESSING_LSB)
		return;

	soundlatch_w(0,(data>>8)&0xff);
	pending_command = 1;
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	/* spin for a while to let the Z80 read the command (fixes hanging sound in pspikes2) */
	cpu_spinuntil_time(TIME_IN_USEC(10));
}



int neogeo_has_trackball;
static int ts;

static WRITE16_HANDLER( trackball_select_16_w )
{
	ts = data & 1;
}

static READ16_HANDLER( controller1_16_r )
{
	data16_t res;

	if (neogeo_has_trackball)
		res = (readinputport(ts?7:0) << 8) + readinputport(3);
	else
		res = (readinputport(0) << 8) + readinputport(3);

	return res;
}
static READ16_HANDLER( controller2_16_r )
{
	data16_t res;

	res = (readinputport(1) << 8);

	return res;
}
static READ16_HANDLER( controller3_16_r )
{
	if (memcard_status==0)
		return (readinputport(2) << 8);
	else
		return ((readinputport(2) << 8)&0x8FFF);
}
static READ16_HANDLER( controller4_16_r )
{
	return (readinputport(6) & ~(readinputport(5) & 0x40));
}

static WRITE16_HANDLER( neo_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	if (memory_region_length(REGION_CPU1) <= 0x100000)
	{
logerror("warning: bankswitch to %02x but no banks available\n",data);
		return;
	}

	data = data&0x7;
	bankaddress = (data+1)*0x100000;
	if (bankaddress >= memory_region_length(REGION_CPU1))
	{
logerror("PC %06x: warning: bankswitch to empty bank %02x\n",activecpu_get_pc(),data);
		bankaddress = 0x100000;
	}

	cpu_setbank(4,&RAM[bankaddress]);
}



/* TODO: Figure out how this really works! */
static READ16_HANDLER( neo_control_16_r )
{
	int res;

	/*
		The format of this very important location is:	AAAA AAAA A??? BCCC

		A is the raster line counter. mosyougi relies solely on this to do the
		  raster effects on the title screen; sdodgeb loops waiting for the top
		  bit to be 1; zedblade heavily depends on it to work correctly (it
		  checks the top bit in the IRQ2 handler).
		B is definitely a PAL/NTSC flag. Evidence:
		  1) trally changes the position of the speed indicator depending on
			 it (0 = lower 1 = higher).
		  2) samsho3 sets a variable to 60 when the bit is 0 and 50 when it's 1.
			 This is obviously the video refresh rate in Hz.
		  3) samsho3 sets another variable to 256 or 307. This could be the total
			 screen height (including vblank), or close to that.
		  Some games (e.g. lstbld2, samsho3) do this (or similar):
		  bclr	  #$0, $3c000e.l
		  when the bit is set, so 3c000e (whose function is unknown) has to be
		  related
		C is a variable speed counter. In blazstar, it controls the background
		  speed in level 2.
	*/

	scanline_read = 1;	/* needed for raster_busy optimization */

	res =	((current_rastercounter << 7) & 0xff80) |	/* raster counter */
			(neogeo_frame_counter & 0x0007);			/* frame counter */

	logerror("PC %06x: neo_control_16_r (%04x)\n",activecpu_get_pc(),res);

	return res;
}


/* this does much more than this, but I'm not sure exactly what */
WRITE16_HANDLER( neo_control_16_w )
{
	logerror("%06x: neo_control_16_w %04x\n",activecpu_get_pc(),data);

	/* Auto-Anim Speed Control */
	neogeo_frame_counter_speed = (data >> 8) & 0xff;

	irq2control = data & 0xff;
}

static WRITE16_HANDLER( neo_irq2pos_16_w )
{
	logerror("%06x: neo_irq2pos_16_w offset %d %04x\n",activecpu_get_pc(),offset,data);

	if (offset)
		irq2pos_value = (irq2pos_value & 0xffff0000) | (UINT32)data;
	else
		irq2pos_value = (irq2pos_value & 0x0000ffff) | ((UINT32)data << 16);

	if (irq2control & IRQ2CTRL_LOAD_RELATIVE)
	{
//		int line = (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */
		int line = (irq2pos_value + 0x3b) / 0x180;	/* turfmast goes as low as 0x145 */

		irq2start = current_rasterline + line;

		logerror("irq2start = %d, current_rasterline = %d, current_rastercounter = %d\n",irq2start,current_rasterline,current_rastercounter);
	}
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

/******************************************************************************/

static MEMORY_READ16_START( neogeo_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },			/* Rom bank 1 */
	{ 0x100000, 0x10ffff, MRA16_BANK1 },		/* Ram bank 1 */
	{ 0x200000, 0x2fffff, MRA16_BANK4 },		/* Rom bank 2 */

	{ 0x300000, 0x300001, controller1_16_r },
	{ 0x300080, 0x300081, controller4_16_r },	/* Test switch in here */
	{ 0x320000, 0x320001, timer16_r },			/* Coins, Calendar, Z80 communication */
	{ 0x340000, 0x340001, controller2_16_r },
	{ 0x380000, 0x380001, controller3_16_r },
	{ 0x3c0000, 0x3c0001, neogeo_vidram16_data_r }, /* Baseball Stars */
	{ 0x3c0002, 0x3c0003, neogeo_vidram16_data_r },
	{ 0x3c0004, 0x3c0005, neogeo_vidram16_modulo_r },

	{ 0x3c0006, 0x3c0007, neo_control_16_r },
	{ 0x3c000a, 0x3c000b, neogeo_vidram16_data_r }, /* Puzzle de Pon */

	{ 0x400000, 0x401fff, neogeo_paletteram16_r },
	{ 0x6a0000, 0x6a1fff, MRA16_RAM },
	{ 0x800000, 0x800fff, neogeo_memcard16_r }, /* memory card */
	{ 0xc00000, 0xc1ffff, MRA16_BANK3 },		/* system bios rom */
	{ 0xd00000, 0xd0ffff, neogeo_sram16_r },	/* 64k battery backed SRAM */
MEMORY_END

static MEMORY_WRITE16_START( neogeo_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },	  /* ghost pilots writes to ROM */
	{ 0x100000, 0x10ffff, MWA16_BANK1 },	// WORK RAM
/*	{ 0x200000, 0x200fff, whp copies ROM data here. Why? Is there RAM in the banked ROM space? */
/* trally writes to 200000-200003 as well, probably looking for a serial link */
/* both games write to 0000fe before writing to 200000. The two things could be related. */
/* sidkicks reads and writes to several addresses in this range, using this for copy */
/* protection. Custom parts instead of the banked ROMs? */
	{ 0x2ffff0, 0x2fffff, neo_bankswitch_w },	/* NOTE THIS CHANGE TO END AT FF !!! */
	{ 0x300000, 0x300001, watchdog_reset16_w },
	{ 0x320000, 0x320001, neo_z80_w },				/* Sound CPU */
	{ 0x380000, 0x380001, trackball_select_16_w },	/* Used by bios, unknown */
	{ 0x380030, 0x380031, MWA16_NOP },				/* Used by bios, unknown */
	{ 0x380040, 0x380041, MWA16_NOP },				/* Output leds */
	{ 0x380050, 0x380051, pd4990a_control_16_w },
	{ 0x380060, 0x380063, MWA16_NOP },				/* Used by bios, unknown */
	{ 0x3800e0, 0x3800e3, MWA16_NOP },				/* Used by bios, unknown */

	{ 0x3a0000, 0x3a0001, MWA16_NOP },
	{ 0x3a0010, 0x3a0011, MWA16_NOP },
	{ 0x3a0002, 0x3a0003, MWA16_NOP },
	{ 0x3a0012, 0x3a0013, MWA16_NOP },
	{ 0x3a000a, 0x3a000b, neo_board_fix_16_w }, /* Select board FIX char rom */
	{ 0x3a001a, 0x3a001b, neo_game_fix_16_w },	/* Select game FIX char rom */
	{ 0x3a000c, 0x3a000d, neogeo_sram16_lock_w },
	{ 0x3a001c, 0x3a001d, neogeo_sram16_unlock_w },
	{ 0x3a000e, 0x3a000f, neogeo_setpalbank1_16_w },
	{ 0x3a001e, 0x3a001f, neogeo_setpalbank0_16_w },	/* Palette banking */

	{ 0x3c0000, 0x3c0001, neogeo_vidram16_offset_w },
	{ 0x3c0002, 0x3c0003, neogeo_vidram16_data_w },
	{ 0x3c0004, 0x3c0005, neogeo_vidram16_modulo_w },

	{ 0x3c0006, 0x3c0007, neo_control_16_w },	/* IRQ2 control */
	{ 0x3c0008, 0x3c000b, neo_irq2pos_16_w },	/* IRQ2 position */
	{ 0x3c000c, 0x3c000d, neo_irqack_w },		/* IRQ acknowledge */
//	{ 0x3c000e, 0x3c000f }, /* Unknown, see control_r */

	{ 0x400000, 0x401fff, neogeo_paletteram16_w },	// COLOR RAM BANK1
	{ 0x6a0000, 0x6a1fff, MWA16_RAM },	// COLOR RAM BANK0 (used only in startup tests?)
	{ 0x800000, 0x800fff, neogeo_memcard16_w }, 	/* mem card */
	{ 0xd00000, 0xd0ffff, neogeo_sram16_w, &neogeo_sram16 },	/* 64k battery backed SRAM */
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK5 },
	{ 0xc000, 0xdfff, MRA_BANK6 },
	{ 0xe000, 0xefff, MRA_BANK7 },
	{ 0xf000, 0xf7ff, MRA_BANK8 },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END


static READ_HANDLER( z80_port_r )
{
	static int bank[4];


#if 0
{
	char buf[80];
	sprintf(buf,"%05x %05x %05x %05x",bank[0],bank[1],bank[2],bank[3]);
	usrintf_showmessage(buf);
}
#endif

	switch (offset & 0xff)
	{
	case 0x00:
		pending_command = 0;
		return soundlatch_r(0);
		break;

	case 0x04:
		return YM2610_status_port_0_A_r(0);
		break;

	case 0x05:
		return YM2610_read_port_0_r(0);
		break;

	case 0x06:
		return YM2610_status_port_0_B_r(0);
		break;

	case 0x08:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2)+0x10000;
			bank[3] = 0x0800 * ((offset >> 8) & 0x7f);
			cpu_setbank(8,&mem08[bank[3]]);
			return 0;
			break;
		}

	case 0x09:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2)+0x10000;
			bank[2] = 0x1000 * ((offset >> 8) & 0x3f);
			cpu_setbank(7,&mem08[bank[2]]);
			return 0;
			break;
		}

	case 0x0a:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2)+0x10000;
			bank[1] = 0x2000 * ((offset >> 8) & 0x1f);
			cpu_setbank(6,&mem08[bank[1]]);
			return 0;
			break;
		}

	case 0x0b:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2)+0x10000;
			bank[0] = 0x4000 * ((offset >> 8) & 0x0f);
			cpu_setbank(5,&mem08[bank[0]]);
			return 0;
			break;
		}

	default:
logerror("CPU #1 PC %04x: read unmapped port %02x\n",activecpu_get_pc(),offset&0xff);
		return 0;
		break;
	}
}

static WRITE_HANDLER( z80_port_w )
{
	switch (offset & 0xff)
	{
	case 0x04:
		YM2610_control_port_0_A_w(0,data);
		break;

	case 0x05:
		YM2610_data_port_0_A_w(0,data);
		break;

	case 0x06:
		YM2610_control_port_0_B_w(0,data);
		break;

	case 0x07:
		YM2610_data_port_0_B_w(0,data);
		break;

	case 0x08:
		/* NMI enable / acknowledge? (the data written doesn't matter) */
		break;

	case 0x0c:
		result_code = data;
		break;

	case 0x18:
		/* NMI disable? (the data written doesn't matter) */
		break;

	default:
logerror("CPU #1 PC %04x: write %02x to unmapped port %02x\n",activecpu_get_pc(),data,offset&0xff);
		break;
	}
}

static PORT_READ_START( neo_readio )
	{ 0x0000, 0xffff, z80_port_r },
PORT_END

static PORT_WRITE_START( neo_writeio )
	{ 0x0000, 0xffff, z80_port_w },
PORT_END

/******************************************************************************/

INPUT_PORTS_START( neogeo )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Next Game",KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Previous Game",KEYCODE_8, IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */

	/* Fake  IN 5 */
	PORT_START
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
//	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )
//	PORT_DIPSETTING(	0x00,"Home" )
//	PORT_DIPSETTING(	0x04,"Arcade" )
	PORT_DIPNAME( 0x60, 0x60,"Game Slots" )		// Stored at 0x47 of NVRAM
	PORT_DIPSETTING(	0x60,"2" )
//	PORT_DIPSETTING(	0x40,"2" )
	PORT_DIPSETTING(	0x20,"4" )
	PORT_DIPSETTING(	0x00,"6" )

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( irrmaze )
	PORT_START		/* IN0 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_X | IPF_REVERSE, 10, 20, 0, 0 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )

	/* Fake  IN 5 */
	PORT_START
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
//	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )
//	PORT_DIPSETTING(	0x00,"Home" )
//	PORT_DIPSETTING(	0x04,"Arcade" )

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* This bit is used.. */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* IN0 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 20, 0, 0 )
INPUT_PORTS_END


/******************************************************************************/

/* character layout (same for all games) */
static struct GfxLayout charlayout =	/* All games */
{
	8,8,			/* 8 x 8 chars */
	RGN_FRAC(1,1),
	4,				/* 4 bits per pixel */
	{ 0, 1, 2, 3 },    /* planes are packed in a nibble */
	{ 33*4, 32*4, 49*4, 48*4, 1*4, 0*4, 17*4, 16*4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* 32 bytes per char */
};

/* Placeholder and also reminder of how this graphic format is put together */
static struct GfxLayout dummy_mvs_tilelayout =
{
	16,16,	 /* 16*16 sprites */
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	128*8		/* char modulo */
};

static struct GfxDecodeInfo neogeo_mvs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ REGION_GFX2, 0, &charlayout, 0, 16 },
	{ REGION_GFX3, 0, &dummy_mvs_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static void neogeo_sound_irq( int irq )
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

struct YM2610interface neogeo_ym2610_interface =
{
	1,
	8000000,
	{ MIXERG(15,MIXER_GAIN_4x,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ neogeo_sound_irq },
	{ REGION_SOUND2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

/******************************************************************************/

static MACHINE_DRIVER_START( neogeo )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 12000000)
	MDRV_CPU_MEMORY(neogeo_readmem,neogeo_writemem)
	MDRV_CPU_VBLANK_INT(neogeo_interrupt,RASTER_LINES)

	MDRV_CPU_ADD(Z80, 6000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU | CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(neo_readio,neo_writeio)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(neogeo)
	MDRV_NVRAM_HANDLER(neogeo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 39*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(neogeo_mvs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(neogeo_mvs)
	MDRV_VIDEO_UPDATE(neogeo)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, neogeo_ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( raster )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(neogeo)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(neogeo_raster_interrupt,RASTER_LINES)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( raster_busy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(raster)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(neogeo_raster_interrupt_busy,RASTER_LINES)
MACHINE_DRIVER_END


/******************************************************************************/

#define NEO_BIOS_SOUND_512K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION( 0x90000, REGION_CPU2, 0 ) \
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x80000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x80000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "ng-lo.rom", 0x00000, 0x10000, 0xe09e253c )  /* Y zoom control */

#define NEO_BIOS_SOUND_256K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x40000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x40000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "ng-lo.rom", 0x00000, 0x10000, 0xe09e253c )  /* Y zoom control */

#define NEO_BIOS_SOUND_128K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x20000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x20000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "ng-lo.rom", 0x00000, 0x10000, 0xe09e253c )  /* Y zoom control */

#define NEO_BIOS_SOUND_64K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x10000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x10000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "ng-lo.rom", 0x00000, 0x10000, 0xe09e253c )  /* Y zoom control */

#define NO_DELTAT_REGION

#define NEO_SFIX_128K(name,sum) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x20000, sum ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

#define NEO_SFIX_64K(name,sum) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x10000, sum ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

#define NEO_SFIX_32K(name,sum) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x08000, sum ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

/******************************************************************************/

ROM_START( nam1975 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "001-p1.bin", 0x000000, 0x080000, 0xcc9fc951 )

	NEO_SFIX_64K( "001-s1.bin", 0x8ded55a5 )

	NEO_BIOS_SOUND_64K( "001-m1.bin", 0xcd088502 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "001-v11.bin", 0x000000, 0x080000, 0xa7c3d5e5 )

	ROM_REGION( 0x180000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "001-v21.bin", 0x000000, 0x080000, 0x55e670b3 )
	ROM_LOAD( "001-v22.bin", 0x080000, 0x080000, 0xab0d8368 )
	ROM_LOAD( "001-v23.bin", 0x100000, 0x080000, 0xdf468e28 )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "001-c1.bin", 0x000000, 0x80000, 0x32ea98e1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "001-c2.bin", 0x000001, 0x80000, 0xcbc4064c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "001-c3.bin", 0x100000, 0x80000, 0x0151054c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "001-c4.bin", 0x100001, 0x80000, 0x0a32570d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "001-c5.bin", 0x200000, 0x80000, 0x90b74cc2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "001-c6.bin", 0x200001, 0x80000, 0xe62bed58 ) /* Plane 2,3 */
ROM_END

ROM_START( bstars )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "002-p1.bin", 0x000000, 0x080000, 0x3bc7790e )

	NEO_SFIX_128K( "002-s1.bin", 0x1a7fd0c6 )

	NEO_BIOS_SOUND_64K( "002-m1.bin", 0x79a8f4c2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "002-v11.bin", 0x000000, 0x080000, 0xb7b925bd )
	ROM_LOAD( "002-v12.bin", 0x080000, 0x080000, 0x329f26fc )
	ROM_LOAD( "002-v13.bin", 0x100000, 0x080000, 0x0c39f3c8 )
	ROM_LOAD( "002-v14.bin", 0x180000, 0x080000, 0xc7e11c38 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "002-v21.bin", 0x000000, 0x080000, 0x04a733d1 )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "002-c1.bin", 0x000000, 0x080000, 0xaaff2a45 )
	ROM_LOAD16_BYTE( "002-c2.bin", 0x000001, 0x080000, 0x3ba0f7e4 )
	ROM_LOAD16_BYTE( "002-c3.bin", 0x100000, 0x080000, 0x96f0fdfa )
	ROM_LOAD16_BYTE( "002-c4.bin", 0x100001, 0x080000, 0x5fd87f2f )
	ROM_LOAD16_BYTE( "002-c5.bin", 0x200000, 0x080000, 0x807ed83b )
	ROM_LOAD16_BYTE( "002-c6.bin", 0x200001, 0x080000, 0x5a3cad41 )
ROM_END

ROM_START( tpgolf )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "003-p1.bin", 0x000000, 0x080000, 0xf75549ba )
	ROM_LOAD16_WORD_SWAP( "003-p2.bin", 0x080000, 0x080000, 0xb7809a8f )

	NEO_SFIX_128K( "003-s1.bin", 0x7b3eb9b1 )

	NEO_BIOS_SOUND_64K( "003-m1.bin", 0x7851d0d9 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "003-v11.bin", 0x000000, 0x080000, 0xff97f1cb )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "003-v21.bin", 0x000000, 0x080000, 0xd34960c6 )
	ROM_LOAD( "003-v22.bin", 0x080000, 0x080000, 0x9a5f58d4 )
	ROM_LOAD( "003-v23.bin", 0x100000, 0x080000, 0x30f53e54 )
	ROM_LOAD( "003-v24.bin", 0x180000, 0x080000, 0x5ba0f501 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "003-c1.bin", 0x000000, 0x80000, 0x0315fbaf )
	ROM_LOAD16_BYTE( "003-c2.bin", 0x000001, 0x80000, 0xb4c15d59 )
	ROM_LOAD16_BYTE( "003-c3.bin", 0x100000, 0x80000, 0xb09f1612 )
	ROM_LOAD16_BYTE( "003-c4.bin", 0x100001, 0x80000, 0x150ea7a1 )
	ROM_LOAD16_BYTE( "003-c5.bin", 0x200000, 0x80000, 0x9a7146da )
	ROM_LOAD16_BYTE( "003-c6.bin", 0x200001, 0x80000, 0x1e63411a )
	ROM_LOAD16_BYTE( "003-c7.bin", 0x300000, 0x80000, 0x2886710c )
	ROM_LOAD16_BYTE( "003-c8.bin", 0x300001, 0x80000, 0x422af22d )
ROM_END

ROM_START( mahretsu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "004-p1.bin", 0x000000, 0x080000, 0xfc6f53db )

	NEO_SFIX_64K( "004-s1.bin", 0xb0d16529 )

	NEO_BIOS_SOUND_64K( "004-m1.bin", 0x37965a73 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "004-v1.bin", 0x000000, 0x080000, 0xb2fb2153 )
	ROM_LOAD( "004-v2.bin", 0x080000, 0x080000, 0x8503317b )

	ROM_REGION( 0x180000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "004-v3.bin", 0x000000, 0x080000, 0x4999fb27 )
	ROM_LOAD( "004-v4.bin", 0x080000, 0x080000, 0x776fa2a2 )
	ROM_LOAD( "004-v5.bin", 0x100000, 0x080000, 0xb3e7eeea )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "004-c1.bin", 0x000000, 0x80000, 0xf1ae16bc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "004-c2.bin", 0x000001, 0x80000, 0xbdc13520 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "004-c3.bin", 0x100000, 0x80000, 0x9c571a37 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "004-c4.bin", 0x100001, 0x80000, 0x7e81cb29 ) /* Plane 2,3 */
ROM_END

ROM_START( maglord )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "005-p1.bin", 0x000000, 0x080000, 0xbd0a492d )

	NEO_SFIX_128K( "005-s1.bin", 0x1c5369a2 )

	NEO_BIOS_SOUND_64K( "005-m1.bin", 0x91ee1f73 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v11.bin", 0x000000, 0x080000, 0xcc0455fd )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v21.bin", 0x000000, 0x080000, 0xf94ab5b7 )
	ROM_LOAD( "005-v22.bin", 0x080000, 0x080000, 0x232cfd04 )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "005-c1.bin", 0x000000, 0x80000, 0x806aee34 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c2.bin", 0x000001, 0x80000, 0x34aa9a86 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c3.bin", 0x100000, 0x80000, 0xc4c2b926 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c4.bin", 0x100001, 0x80000, 0x9c46dcf4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c5.bin", 0x200000, 0x80000, 0x69086dec ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c6.bin", 0x200001, 0x80000, 0xab7ac142 ) /* Plane 2,3 */
ROM_END

ROM_START( maglordh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "005-p1h.bin", 0x000000, 0x080000, 0x599043c5 )

	NEO_SFIX_128K( "005-s1.bin", 0x1c5369a2 )

	NEO_BIOS_SOUND_64K( "005-m1.bin", 0x91ee1f73 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v11.bin", 0x000000, 0x080000, 0xcc0455fd )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v21.bin", 0x000000, 0x080000, 0xf94ab5b7 )
	ROM_LOAD( "005-v22.bin", 0x080000, 0x080000, 0x232cfd04 )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "005-c1.bin", 0x000000, 0x80000, 0x806aee34 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c2.bin", 0x000001, 0x80000, 0x34aa9a86 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c3.bin", 0x100000, 0x80000, 0xc4c2b926 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c4.bin", 0x100001, 0x80000, 0x9c46dcf4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c5.bin", 0x200000, 0x80000, 0x69086dec ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c6.bin", 0x200001, 0x80000, 0xab7ac142 ) /* Plane 2,3 */
ROM_END

ROM_START( ridhero )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "006-p1.bin", 0x000000, 0x080000, 0xd4aaf597 )

	NEO_SFIX_64K( "006-s1.bin", 0x197d1a28 )

	NEO_BIOS_SOUND_128K( "006-m1.bin", 0xf0b6425d )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v11.bin", 0x000000, 0x080000, 0xcdf74a42 )
	ROM_LOAD( "006-v12.bin", 0x080000, 0x080000, 0xe2fd2371 )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v21.bin", 0x000000, 0x080000, 0x94092bce )
	ROM_LOAD( "006-v22.bin", 0x080000, 0x080000, 0x4e2cd7c3 )
	ROM_LOAD( "006-v23.bin", 0x100000, 0x080000, 0x069c71ed )
	ROM_LOAD( "006-v24.bin", 0x180000, 0x080000, 0x89fbb825 )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "006-c1.bin", 0x000000, 0x080000, 0x4a5c7f78 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c2.bin", 0x000001, 0x080000, 0xe0b70ece ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "006-c3.bin", 0x100000, 0x080000, 0x8acff765 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c4.bin", 0x100001, 0x080000, 0x205e3208 ) /* Plane 2,3 */
ROM_END

ROM_START( ridheroh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "006-p1h.bin", 0x000000, 0x080000, 0x52445646 )

	NEO_SFIX_64K( "006-s1.bin", 0x197d1a28 )

	NEO_BIOS_SOUND_128K( "006-m1.bin", 0xf0b6425d )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v11.bin", 0x000000, 0x080000, 0xcdf74a42 )
	ROM_LOAD( "006-v12.bin", 0x080000, 0x080000, 0xe2fd2371 )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v21.bin", 0x000000, 0x080000, 0x94092bce )
	ROM_LOAD( "006-v22.bin", 0x080000, 0x080000, 0x4e2cd7c3 )
	ROM_LOAD( "006-v23.bin", 0x100000, 0x080000, 0x069c71ed )
	ROM_LOAD( "006-v24.bin", 0x180000, 0x080000, 0x89fbb825 )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "006-c1.bin", 0x000000, 0x080000, 0x4a5c7f78 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c2.bin", 0x000001, 0x080000, 0xe0b70ece ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "006-c3.bin", 0x100000, 0x080000, 0x8acff765 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c4.bin", 0x100001, 0x080000, 0x205e3208 ) /* Plane 2,3 */
ROM_END

ROM_START( alpham2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "007-p1.bin", 0x000000, 0x080000, 0x5b266f47 )
	ROM_LOAD16_WORD_SWAP( "007-p2.bin", 0x080000, 0x020000, 0xeb9c1044 )

	NEO_SFIX_128K( "007-s1.bin", 0x85ec9acf )

	NEO_BIOS_SOUND_128K( "007-m1.bin", 0x28dfe2cd )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "007-v1.bin", 0x000000, 0x100000, 0xcd5db931 )
	ROM_LOAD( "007-v2.bin", 0x100000, 0x100000, 0x63e9b574 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "007-c1.bin", 0x000000, 0x100000, 0x8fba8ff3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "007-c2.bin", 0x000001, 0x100000, 0x4dad2945 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "007-c3.bin", 0x200000, 0x080000, 0x68c2994e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "007-c4.bin", 0x200001, 0x080000, 0x7d588349 ) /* Plane 2,3 */
ROM_END

ROM_START( ncombat )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "009-p1.bin", 0x000000, 0x080000, 0xb45fcfbf )

	NEO_SFIX_128K( "009-s1.bin", 0xd49afee8 )

	NEO_BIOS_SOUND_128K( "009-m1.bin", 0xb5819863 )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "009-v11.bin", 0x000000, 0x080000, 0xcf32a59c )
	ROM_LOAD( "009-v12.bin", 0x080000, 0x080000, 0x7b3588b7 )
	ROM_LOAD( "009-v13.bin", 0x100000, 0x080000, 0x505a01b5 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "009-v21.bin", 0x000000, 0x080000, 0x365f9011 )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "009-c1.bin", 0x000000, 0x80000, 0x33cc838e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "009-c2.bin", 0x000001, 0x80000, 0x26877feb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "009-c3.bin", 0x100000, 0x80000, 0x3b60a05d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "009-c4.bin", 0x100001, 0x80000, 0x39c2d039 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "009-c5.bin", 0x200000, 0x80000, 0x67a4344e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "009-c6.bin", 0x200001, 0x80000, 0x2eca8b19 ) /* Plane 2,3 */
ROM_END

ROM_START( cyberlip )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "010-p1.bin", 0x000000, 0x080000, 0x69a6b42d )

	NEO_SFIX_128K( "010-s1.bin", 0x79a35264 )

	NEO_BIOS_SOUND_64K( "010-m1.bin", 0x47980d3a )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "010-v11.bin", 0x000000, 0x080000, 0x90224d22 )
	ROM_LOAD( "010-v12.bin", 0x080000, 0x080000, 0xa0cf1834 )
	ROM_LOAD( "010-v13.bin", 0x100000, 0x080000, 0xae38bc84 )
	ROM_LOAD( "010-v14.bin", 0x180000, 0x080000, 0x70899bd2 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "010-v21.bin", 0x000000, 0x080000, 0x586f4cb2 )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "010-c1.bin", 0x000000, 0x80000, 0x8bba5113 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "010-c2.bin", 0x000001, 0x80000, 0xcbf66432 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "010-c3.bin", 0x100000, 0x80000, 0xe4f86efc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "010-c4.bin", 0x100001, 0x80000, 0xf7be4674 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "010-c5.bin", 0x200000, 0x80000, 0xe8076da0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "010-c6.bin", 0x200001, 0x80000, 0xc495c567 ) /* Plane 2,3 */
ROM_END

ROM_START( superspy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "011-p1.bin", 0x000000, 0x080000, 0xc7f944b5 )
	ROM_LOAD16_WORD_SWAP( "011-p2.bin", 0x080000, 0x020000, 0x811a4faf )

	NEO_SFIX_128K( "011-s1.bin", 0xec5fdb96 )

	NEO_BIOS_SOUND_128K( "011-m1.bin", 0xd59d5d12 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "011-v11.bin", 0x000000, 0x100000, 0x5c674d5c )
	ROM_LOAD( "011-v12.bin", 0x100000, 0x100000, 0x7df8898b )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "011-v21.bin", 0x000000, 0x100000, 0x1ebe94c7 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "011-c1.bin", 0x000000, 0x100000, 0xcae7be57 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "011-c2.bin", 0x000001, 0x100000, 0x9e29d986 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "011-c3.bin", 0x200000, 0x100000, 0x14832ff2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "011-c4.bin", 0x200001, 0x100000, 0xb7f63162 ) /* Plane 2,3 */
ROM_END

ROM_START( mutnat )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "014-p1.bin", 0x000000, 0x080000, 0x6f1699c8 )

	NEO_SFIX_128K( "014-s1.bin", 0x99419733 )

	NEO_BIOS_SOUND_128K( "014-m1.bin", 0xb6683092 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "014-v1.bin", 0x000000, 0x100000, 0x25419296 )
	ROM_LOAD( "014-v2.bin", 0x100000, 0x100000, 0x0de53d5e )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "014-c1.bin", 0x000000, 0x100000, 0x5e4381bf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "014-c2.bin", 0x000001, 0x100000, 0x69ba4e18 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "014-c3.bin", 0x200000, 0x100000, 0x890327d5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "014-c4.bin", 0x200001, 0x100000, 0xe4002651 ) /* Plane 2,3 */
ROM_END

ROM_START( kotm )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "016-p1.bin", 0x000000, 0x080000, 0x1b818731 )
	ROM_LOAD16_WORD_SWAP( "016-p2.bin", 0x080000, 0x020000, 0x12afdc2b )

	NEO_SFIX_128K( "016-s1.bin", 0x1a2eeeb3 )

	NEO_BIOS_SOUND_128K( "016-m1.bin", 0x0296abcb )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "016-v1.bin", 0x000000, 0x100000, 0x86c0a502 )
	ROM_LOAD( "016-v2.bin", 0x100000, 0x100000, 0x5bc23ec5 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "016-c1.bin", 0x000000, 0x100000, 0x71471c25 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "016-c2.bin", 0x000001, 0x100000, 0x320db048 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "016-c3.bin", 0x200000, 0x100000, 0x98de7995 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "016-c4.bin", 0x200001, 0x100000, 0x070506e2 ) /* Plane 2,3 */
ROM_END

ROM_START( sengoku )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "017-p1.bin", 0x000000, 0x080000, 0xf8a63983 )
	ROM_LOAD16_WORD_SWAP( "017-p2.bin", 0x080000, 0x020000, 0x3024bbb3 )

	NEO_SFIX_128K( "017-s1.bin", 0xb246204d )

	NEO_BIOS_SOUND_128K( "017-m1.bin", 0x9b4f34c6 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "017-v1.bin", 0x000000, 0x100000, 0x23663295 )
	ROM_LOAD( "017-v2.bin", 0x100000, 0x100000, 0xf61e6765 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "017-c1.bin", 0x000000, 0x100000, 0xb4eb82a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c2.bin", 0x000001, 0x100000, 0xd55c550d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "017-c3.bin", 0x200000, 0x100000, 0xed51ef65 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c4.bin", 0x200001, 0x100000, 0xf4f3c9cb ) /* Plane 2,3 */
ROM_END

ROM_START( sengokh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "017-p1h.bin", 0x000000, 0x080000, 0x33eccae0 )
	ROM_LOAD16_WORD_SWAP( "017-p2.bin",  0x080000, 0x020000, 0x3024bbb3 )

	NEO_SFIX_128K( "017-s1.bin", 0xb246204d )

	NEO_BIOS_SOUND_128K( "017-m1.bin", 0x9b4f34c6 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "017-v1.bin", 0x000000, 0x100000, 0x23663295 )
	ROM_LOAD( "017-v2.bin", 0x100000, 0x100000, 0xf61e6765 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "017-c1.bin", 0x000000, 0x100000, 0xb4eb82a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c2.bin", 0x000001, 0x100000, 0xd55c550d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "017-c3.bin", 0x200000, 0x100000, 0xed51ef65 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c4.bin", 0x200001, 0x100000, 0xf4f3c9cb ) /* Plane 2,3 */
ROM_END

ROM_START( burningf )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "018-p1.bin", 0x000000, 0x080000, 0x4092c8db )

	NEO_SFIX_128K( "018-s1.bin", 0x6799ea0d )

	NEO_BIOS_SOUND_128K( "018-m1.bin", 0x0c939ee2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "018-v1.bin", 0x000000, 0x100000, 0x508c9ffc )
	ROM_LOAD( "018-v2.bin", 0x100000, 0x100000, 0x854ef277 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "018-c1.bin", 0x000000, 0x100000, 0x25a25e9b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c2.bin", 0x000001, 0x100000, 0xd4378876 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "018-c3.bin", 0x200000, 0x100000, 0x862b60da ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c4.bin", 0x200001, 0x100000, 0xe2e0aff7 ) /* Plane 2,3 */
ROM_END

ROM_START( burningh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "018-p1h.bin", 0x000000, 0x080000, 0xddffcbf4 )

	NEO_SFIX_128K( "018-s1.bin", 0x6799ea0d )

	NEO_BIOS_SOUND_128K( "018-m1.bin", 0x0c939ee2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "018-v1.bin", 0x000000, 0x100000, 0x508c9ffc )
	ROM_LOAD( "018-v2.bin", 0x100000, 0x100000, 0x854ef277 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "018-c1.bin", 0x000000, 0x100000, 0x25a25e9b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c2.bin", 0x000001, 0x100000, 0xd4378876 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "018-c3.bin", 0x200000, 0x100000, 0x862b60da ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c4.bin", 0x200001, 0x100000, 0xe2e0aff7 ) /* Plane 2,3 */
ROM_END

ROM_START( lbowling )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "019-p1.bin", 0x000000, 0x080000, 0xa2de8445 )

	NEO_SFIX_128K( "019-s1.bin", 0x5fcdc0ed )

	NEO_BIOS_SOUND_128K( "019-m1.bin", 0x589d7f25 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "019-v11.bin", 0x000000, 0x080000, 0x0fb74872 )
	ROM_LOAD( "019-v12.bin", 0x080000, 0x080000, 0x029faa57 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "019-v21.bin", 0x000000, 0x080000, 0x2efd5ada )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "019-c1.bin", 0x000000, 0x080000, 0x4ccdef18 )
	ROM_LOAD16_BYTE( "019-c2.bin", 0x000001, 0x080000, 0xd4dd0802 )
ROM_END

ROM_START( gpilots )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "020-p1.bin", 0x000000, 0x080000, 0xe6f2fe64 )
	ROM_LOAD16_WORD_SWAP( "020-p2.bin", 0x080000, 0x020000, 0xedcb22ac )

	NEO_SFIX_128K( "020-s1.bin", 0xa6d83d53 )

	NEO_BIOS_SOUND_128K( "020-m1.bin", 0x48409377 )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "020-v11.bin", 0x000000, 0x100000, 0x1b526c8b )
	ROM_LOAD( "020-v12.bin", 0x100000, 0x080000, 0x4a9e6f03 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "020-v21.bin", 0x000000, 0x080000, 0x7abf113d )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "020-c1.bin", 0x000000, 0x100000, 0xbd6fe78e )
	ROM_LOAD16_BYTE( "020-c2.bin", 0x000001, 0x100000, 0x5f4a925c )
	ROM_LOAD16_BYTE( "020-c3.bin", 0x200000, 0x100000, 0xd1e42fd0 )
	ROM_LOAD16_BYTE( "020-c4.bin", 0x200001, 0x100000, 0xedde439b )
ROM_END

ROM_START( joyjoy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "021-p1.bin", 0x000000, 0x080000, 0x39c3478f )

	NEO_SFIX_128K( "021-s1.bin", 0x6956d778 )

	NEO_BIOS_SOUND_64K( "021-m1.bin", 0x058683ec )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "021-v1.bin", 0x000000, 0x080000, 0x66c1e5c4 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "021-v2.bin", 0x000000, 0x080000, 0x8ed20a86 )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "021-c1.bin", 0x000000, 0x080000, 0x509250ec )
	ROM_LOAD16_BYTE( "021-c2.bin", 0x000001, 0x080000, 0x09ed5258 )
ROM_END

ROM_START( bjourney )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "022-p1.bin", 0x000000, 0x100000, 0x6a2f6d4a )

	NEO_SFIX_128K( "022-s1.bin", 0x843c3624 )

	NEO_BIOS_SOUND_64K( "022-m1.bin",  0xa9e30496 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "022-v1.bin", 0x000000, 0x100000, 0x2cb4ad91 )
	ROM_LOAD( "022-v2.bin", 0x100000, 0x100000, 0x65a54d13 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "022-c1.bin", 0x000000, 0x100000, 0x4d47a48c )
	ROM_LOAD16_BYTE( "022-c2.bin", 0x000001, 0x100000, 0xe8c1491a )
	ROM_LOAD16_BYTE( "022-c3.bin", 0x200000, 0x080000, 0x66e69753 )
	ROM_LOAD16_BYTE( "022-c4.bin", 0x200001, 0x080000, 0x71bfd48a )
ROM_END

ROM_START( quizdais )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "023-p1.bin", 0x000000, 0x100000, 0xc488fda3 )

	NEO_SFIX_128K( "023-s1.bin", 0xac31818a )

	NEO_BIOS_SOUND_128K( "023-m1.bin", 0x2a2105e0 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "023-v1.bin", 0x000000, 0x100000, 0xa53e5bd3 )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "023-c1.bin", 0x000000, 0x100000, 0x2999535a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "023-c2.bin", 0x000001, 0x100000, 0x876a99e6 ) /* Plane 2,3 */
ROM_END

ROM_START( lresort )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "024-p1.bin", 0x000000, 0x080000, 0x89c4ab97 )

	NEO_SFIX_128K( "024-s1.bin", 0x5cef5cc6 )

	NEO_BIOS_SOUND_128K( "024-m1.bin", 0xcec19742 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "024-v1.bin", 0x000000, 0x100000, 0xefdfa063 )
	ROM_LOAD( "024-v2.bin", 0x100000, 0x100000, 0x3c7997c0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "024-c1.bin", 0x000000, 0x100000, 0x3617c2dc )
	ROM_LOAD16_BYTE( "024-c2.bin", 0x000001, 0x100000, 0x3f0a7fd8 )
	ROM_LOAD16_BYTE( "024-c3.bin", 0x200000, 0x080000, 0xe9f745f8 )
	ROM_LOAD16_BYTE( "024-c4.bin", 0x200001, 0x080000, 0x7382fefb )
ROM_END

ROM_START( eightman )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "025-p1.bin", 0x000000, 0x080000, 0x43344cb0 )

	NEO_SFIX_128K( "025-s1.bin", 0xa402202b )

	NEO_BIOS_SOUND_128K( "025-m1.bin", 0x9927034c )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "025-v1.bin", 0x000000, 0x100000, 0x4558558a )
	ROM_LOAD( "025-v2.bin", 0x100000, 0x100000, 0xc5e052e9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "025-c1.bin", 0x000000, 0x100000, 0x555e16a4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "025-c2.bin", 0x000001, 0x100000, 0xe1ee51c3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "025-c3.bin", 0x200000, 0x080000, 0x0923d5b0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "025-c4.bin", 0x200001, 0x080000, 0xe3eca67b ) /* Plane 2,3 */
ROM_END

ROM_START( minasan )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "027-p1.bin", 0x000000, 0x080000, 0xc8381327 )

	NEO_SFIX_128K( "027-s1.bin", 0xe5824baa )

	NEO_BIOS_SOUND_128K( "027-m1.bin", 0xadd5a226 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "027-v11.bin", 0x000000, 0x100000, 0x59ad4459 )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "027-v21.bin", 0x000000, 0x100000, 0xdf5b4eeb )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "027-c1.bin", 0x000000, 0x100000, 0xd0086f94 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "027-c2.bin", 0x000001, 0x100000, 0xda61f5a6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "027-c3.bin", 0x200000, 0x100000, 0x08df1228 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "027-c4.bin", 0x200001, 0x100000, 0x54e87696 ) /* Plane 2,3 */
ROM_END

ROM_START( legendos )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "029-p1.bin", 0x000000, 0x080000, 0x9d563f19 )

	NEO_SFIX_128K( "029-s1.bin",  0xbcd502f0 )

	NEO_BIOS_SOUND_64K( "029-m1.bin", 0x909d4ed9 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "029-v1.bin", 0x000000, 0x100000, 0x85065452 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "029-c1.bin", 0x000000, 0x100000, 0x2f5ab875 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "029-c2.bin", 0x000001, 0x100000, 0x318b2711 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "029-c3.bin", 0x200000, 0x100000, 0x6bc52cb2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "029-c4.bin", 0x200001, 0x100000, 0x37ef298c ) /* Plane 2,3 */
ROM_END

ROM_START( 2020bb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "030-p1.bin", 0x000000, 0x080000, 0xd396c9cb )

	NEO_SFIX_128K( "030-s1.bin", 0x7015b8fc )

	NEO_BIOS_SOUND_128K( "030-m1.bin", 0x4cf466ec )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "030-v1.bin", 0x000000, 0x100000, 0xd4ca364e )
	ROM_LOAD( "030-v2.bin", 0x100000, 0x100000, 0x54994455 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "030-c1.bin", 0x000000, 0x100000, 0x4f5e19bd ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c2.bin", 0x000001, 0x100000, 0xd6314bf0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "030-c3.bin", 0x200000, 0x080000, 0x6a87ae30 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c4.bin", 0x200001, 0x080000, 0xbef75dd0 ) /* Plane 2,3 */
ROM_END

ROM_START( 2020bbh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "030-p1h.bin", 0x000000, 0x080000, 0x12d048d7 )

	NEO_SFIX_128K( "030-s1.bin", 0x7015b8fc )

	NEO_BIOS_SOUND_128K( "030-m1.bin", 0x4cf466ec )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "030-v1.bin", 0x000000, 0x100000, 0xd4ca364e )
	ROM_LOAD( "030-v2.bin", 0x100000, 0x100000, 0x54994455 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "030-c1.bin", 0x000000, 0x100000, 0x4f5e19bd ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c2.bin", 0x000001, 0x100000, 0xd6314bf0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "030-c3.bin", 0x200000, 0x080000, 0x6a87ae30 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c4.bin", 0x200001, 0x080000, 0xbef75dd0 ) /* Plane 2,3 */
ROM_END

ROM_START( socbrawl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "031-p1.bin", 0x000000, 0x080000, 0xa2801c24 )

	NEO_SFIX_64K( "031-s1.bin", 0x2db38c3b )

	NEO_BIOS_SOUND_64K( "031-m1.bin", 0x2f38d5d3 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "031-v1.bin", 0x000000, 0x100000, 0xcc78497e )
	ROM_LOAD( "031-v2.bin", 0x100000, 0x100000, 0xdda043c6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "031-c1.bin", 0x000000, 0x100000, 0xbd0a4eb8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "031-c2.bin", 0x000001, 0x100000, 0xefde5382 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "031-c3.bin", 0x200000, 0x080000, 0x580f7f33 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "031-c4.bin", 0x200001, 0x080000, 0xed297de8 ) /* Plane 2,3 */
ROM_END

ROM_START( roboarmy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "032-p1.bin", 0x000000, 0x080000, 0xcd11cbd4 )

	NEO_SFIX_128K( "032-s1.bin", 0xac0daa1b )

	NEO_BIOS_SOUND_128K( "032-m1.bin", 0x98edc671 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "032-v1.bin", 0x000000, 0x080000, 0xdaff9896 )
	ROM_LOAD( "032-v2.bin", 0x080000, 0x080000, 0x8781b1bc )
	ROM_LOAD( "032-v3.bin", 0x100000, 0x080000, 0xb69c1da5 )
	ROM_LOAD( "032-v4.bin", 0x180000, 0x080000, 0x2c929c17 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "032-c1.bin", 0x000000, 0x080000, 0xe17fa618 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "032-c2.bin", 0x000001, 0x080000, 0xd5ebdb4d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "032-c3.bin", 0x100000, 0x080000, 0xaa4d7695 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "032-c4.bin", 0x100001, 0x080000, 0x8d4ebbe3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "032-c5.bin", 0x200000, 0x080000, 0x40adfccd ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "032-c6.bin", 0x200001, 0x080000, 0x462571de ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "033-p1.bin", 0x000000, 0x080000, 0x47ebdc2f )
	ROM_LOAD16_WORD_SWAP( "033-p2.bin", 0x080000, 0x020000, 0xc473af1c )

	NEO_SFIX_128K( "033-s1.bin", 0x3c3bdf8c )

	NEO_BIOS_SOUND_128K( "033-m1.bin", 0xa8603979 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "033-v1.bin", 0x000000, 0x100000, 0x212fd20d )
	ROM_LOAD( "033-v2.bin", 0x100000, 0x100000, 0xfa2ae47f )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "033-c1.bin", 0x000000, 0x100000, 0x74317e54 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "033-c2.bin", 0x000001, 0x100000, 0x5bb952f3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "033-c3.bin", 0x200000, 0x100000, 0x9b714a7c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "033-c4.bin", 0x200001, 0x100000, 0x9397476a ) /* Plane 2,3 */
ROM_END

ROM_START( fbfrenzy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "034-p1.bin", 0x000000, 0x080000, 0xcdef6b19 )

	NEO_SFIX_128K( "034-s1.bin", 0x8472ed44 )

	NEO_BIOS_SOUND_128K( "034-m1.bin", 0xf41b16b8 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "034-v1.bin", 0x000000, 0x100000, 0x50c9d0dd )
	ROM_LOAD( "034-v2.bin", 0x100000, 0x100000, 0x5aa15686 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "034-c1.bin", 0x000000, 0x100000, 0x91c56e78 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "034-c2.bin", 0x000001, 0x100000, 0x9743ea2f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "034-c3.bin", 0x200000, 0x080000, 0xe5aa65f5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "034-c4.bin", 0x200001, 0x080000, 0x0eb138cc ) /* Plane 2,3 */
ROM_END

ROM_START( bakatono )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "036-p1.bin", 0x000000, 0x080000, 0x1c66b6fa )

	NEO_SFIX_128K( "036-s1.bin", 0xf3ef4485 )

	NEO_BIOS_SOUND_128K( "036-m1.bin", 0xf1385b96 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "036-v1.bin", 0x000000, 0x100000, 0x1c335dce )
	ROM_LOAD( "036-v2.bin", 0x100000, 0x100000, 0xbbf79342 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "036-c1.bin", 0x000000, 0x100000, 0xfe7f1010 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "036-c2.bin", 0x000001, 0x100000, 0xbbf003f5 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "036-c3.bin", 0x200000, 0x100000, 0x9ac0708e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "036-c4.bin", 0x200001, 0x100000, 0xf2577d22 ) /* Plane 2,3 */
ROM_END

ROM_START( crsword )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "037-p1.bin", 0x000000, 0x080000, 0xe7f2553c )

	NEO_SFIX_128K( "037-s1.bin", 0x74651f27 )

	NEO_BIOS_SOUND_128K( "037-m1.bin", 0x9c384263 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "037-v1.bin",  0x000000, 0x100000, 0x61fedf65 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "037-c1.bin", 0x000000, 0x100000, 0x09df6892 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "037-c2.bin", 0x000001, 0x100000, 0xac122a78 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "037-c3.bin", 0x200000, 0x100000, 0x9d7ed1ca ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "037-c4.bin", 0x200001, 0x100000, 0x4a24395d ) /* Plane 2,3 */
ROM_END

ROM_START( trally )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "038-p1.bin", 0x000000, 0x080000, 0x1e52a576 )
	ROM_LOAD16_WORD_SWAP( "038-p2.bin", 0x080000, 0x080000, 0xa5193e2f )

	NEO_SFIX_128K( "038-s1.bin", 0xfff62ae3 )

	NEO_BIOS_SOUND_128K( "038-m1.bin", 0x0908707e )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "038-v1.bin", 0x000000, 0x100000, 0x5ccd9fd5 )
	ROM_LOAD( "038-v2.bin", 0x100000, 0x080000, 0xddd8d1e6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "038-c1.bin", 0x000000, 0x100000, 0xc58323d4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "038-c2.bin", 0x000001, 0x100000, 0xbba9c29e ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "038-c3.bin", 0x200000, 0x080000, 0x3bb7b9d6 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "038-c4.bin", 0x200001, 0x080000, 0xa4513ecf ) /* Plane 2,3 */
ROM_END

ROM_START( kotm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "039-p1.bin", 0x000000, 0x080000, 0xb372d54c )
	ROM_LOAD16_WORD_SWAP( "039-p2.bin", 0x080000, 0x080000, 0x28661afe )

	NEO_SFIX_128K( "039-s1.bin", 0x63ee053a )

	NEO_BIOS_SOUND_128K( "039-m1.bin", 0x0c5b2ad5 )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "039-v1.bin", 0x000000, 0x200000, 0x86d34b25 )
	ROM_LOAD( "039-v2.bin", 0x200000, 0x100000, 0x8fa62a0b )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "039-c1.bin", 0x000000, 0x100000, 0x6d1c4aa9 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "039-c2.bin", 0x000001, 0x100000, 0xf7b75337 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "039-c3.bin", 0x200000, 0x100000, 0x40156dca ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "039-c4.bin", 0x200001, 0x100000, 0xb0d44111 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x600001, 0x100000 )
ROM_END

ROM_START( sengoku2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "040-p1.bin", 0x000000, 0x080000, 0xcc245299 )
	ROM_LOAD16_WORD_SWAP( "040-p2.bin", 0x080000, 0x080000, 0x2e466360 )

	NEO_SFIX_128K( "040-s1.bin", 0xcd9802a3 )

	NEO_BIOS_SOUND_128K( "040-m1.bin", 0x9902dfa2 )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "040-v1.bin", 0x000000, 0x100000, 0xb3725ced )
	ROM_LOAD( "040-v2.bin", 0x100000, 0x100000, 0xb5e70a0e )
	ROM_LOAD( "040-v3.bin", 0x200000, 0x100000, 0xc5cece01 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "040-c1.bin", 0x000000, 0x200000, 0x3cacd552 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "040-c2.bin", 0x000001, 0x200000, 0xe2aadef3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "040-c3.bin", 0x400000, 0x200000, 0x037614d5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "040-c4.bin", 0x400001, 0x200000, 0xe9947e5b ) /* Plane 2,3 */
ROM_END

ROM_START( bstars2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "041-p1.bin", 0x000000, 0x080000, 0x523567fd )

	NEO_SFIX_128K( "041-s1.bin", 0x015c5c94 )

	NEO_BIOS_SOUND_64K( "041-m1.bin", 0xb2611c03 )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "041-v1.bin", 0x000000, 0x100000, 0xcb1da093 )
	ROM_LOAD( "041-v2.bin", 0x100000, 0x100000, 0x1c954a9d )
	ROM_LOAD( "041-v3.bin", 0x200000, 0x080000, 0xafaa0180 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "041-c1.bin", 0x000000, 0x100000, 0xb39a12e1 )
	ROM_LOAD16_BYTE( "041-c2.bin", 0x000001, 0x100000, 0x766cfc2f )
	ROM_LOAD16_BYTE( "041-c3.bin", 0x200000, 0x100000, 0xfb31339d )
	ROM_LOAD16_BYTE( "041-c4.bin", 0x200001, 0x100000, 0x70457a0c )
ROM_END

ROM_START( quizdai2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "042-p1.bin", 0x000000, 0x100000, 0xed719dcf)

	NEO_SFIX_128K( "042-s1.bin", 0x164fd6e6 )

	NEO_BIOS_SOUND_128K( "042-m1.bin", 0xbb19995d )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "042-v1.bin", 0x000000, 0x100000, 0xaf7f8247 )
	ROM_LOAD( "042-v2.bin", 0x100000, 0x100000, 0xc6474b59 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "042-c1.bin", 0x000000, 0x100000, 0xcb5809a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "042-c2.bin", 0x000001, 0x100000, 0x1436dfeb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "042-c3.bin", 0x200000, 0x080000, 0xbcd4a518 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "042-c4.bin", 0x200001, 0x080000, 0xd602219b ) /* Plane 2,3 */
ROM_END

ROM_START( 3countb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "043-p1.bin", 0x000000, 0x080000, 0xeb2714c4 )
	ROM_LOAD16_WORD_SWAP( "043-p2.bin", 0x080000, 0x080000, 0x5e764567 )

	NEO_SFIX_128K( "043-s1.bin", 0xc362d484 )

	NEO_BIOS_SOUND_128K( "043-m1.bin", 0x3377cda3 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "043-v1.bin", 0x000000, 0x200000, 0x63688ce8 )
	ROM_LOAD( "043-v2.bin", 0x200000, 0x200000, 0xc69a827b )

	NO_DELTAT_REGION

	ROM_REGION( 0x0800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "043-c1.bin", 0x0000000, 0x200000, 0xd290cc33 )
	ROM_LOAD16_BYTE( "043-c2.bin", 0x0000001, 0x200000, 0x0b28095d )
	ROM_LOAD16_BYTE( "043-c3.bin", 0x0400000, 0x200000, 0xbcc0cb35 )
	ROM_LOAD16_BYTE( "043-c4.bin", 0x0400001, 0x200000, 0x4d1ff7b9 )
ROM_END

ROM_START( aof )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "044-p1.bin", 0x000000, 0x080000, 0xca9f7a6d )

	NEO_SFIX_128K( "044-s1.bin", 0x89903f39 )

	NEO_BIOS_SOUND_128K( "044-m1.bin", 0x981345f8 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "044-v2.bin", 0x000000, 0x200000, 0x3ec632ea )
	ROM_LOAD( "044-v4.bin", 0x200000, 0x200000, 0x4b0f8e23 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "044-c1.bin", 0x000000, 0x100000, 0xddab98a7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			 0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "044-c2.bin", 0x000001, 0x100000, 0xd8ccd575 ) /* Plane 2,3 */
	ROM_CONTINUE(      			 0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "044-c3.bin", 0x200000, 0x100000, 0x403e898a ) /* Plane 0,1 */
	ROM_CONTINUE(      			 0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "044-c4.bin", 0x200001, 0x100000, 0x6235fbaa ) /* Plane 2,3 */
	ROM_CONTINUE(      			 0x600001, 0x100000 )
ROM_END

ROM_START( samsho )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "045-p1.bin", 0x000000, 0x080000, 0x80aa6c97 )
	ROM_LOAD16_WORD_SWAP( "045-p2.bin", 0x080000, 0x080000, 0x71768728 )
	ROM_LOAD16_WORD_SWAP( "045-p3.bin", 0x100000, 0x080000, 0x38ee9ba9 )

	NEO_SFIX_128K( "045-s1.bin", 0x9142a4d3 )

	NEO_BIOS_SOUND_128K( "045-m1.bin", 0x95170640 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "045-v1.bin", 0x000000, 0x200000, 0x37f78a9b )
	ROM_LOAD( "045-v2.bin", 0x200000, 0x200000, 0x568b20cf )

	NO_DELTAT_REGION

	ROM_REGION( 0x900000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "045-c1.bin", 0x000000, 0x200000, 0x2e5873a4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "045-c2.bin", 0x000001, 0x200000, 0x04febb10 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "045-c3.bin", 0x400000, 0x200000, 0xf3dabd1e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "045-c4.bin", 0x400001, 0x200000, 0x935c62f0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "045-c5.bin", 0x800000, 0x080000, 0xa2bb8284 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "045-c6.bin", 0x800001, 0x080000, 0x4fa71252 ) /* Plane 2,3 */
ROM_END

ROM_START( tophuntr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "046-p1.bin", 0x000000, 0x100000, 0x69fa9e29 )
	ROM_LOAD16_WORD_SWAP( "046-p2.sp2", 0x100000, 0x100000, 0xf182cb3e )

	NEO_SFIX_128K( "046-s1.bin", 0x14b01d7b )

	NEO_BIOS_SOUND_128K( "046-m1.bin", 0x3f84bb9f )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "046-v1.bin", 0x000000, 0x100000, 0xc1f9c2db )
	ROM_LOAD( "046-v2.bin", 0x100000, 0x100000, 0x56254a64 )
	ROM_LOAD( "046-v3.bin", 0x200000, 0x100000, 0x58113fb1 )
	ROM_LOAD( "046-v4.bin", 0x300000, 0x100000, 0x4f54c187 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "046-c1.bin", 0x000000, 0x100000, 0xfa720a4a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c2.bin", 0x000001, 0x100000, 0xc900c205 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c3.bin", 0x200000, 0x100000, 0x880e3c25 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c4.bin", 0x200001, 0x100000, 0x7a2248aa ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c5.bin", 0x400000, 0x100000, 0x4b735e45 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c6.bin", 0x400001, 0x100000, 0x273171df ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c7.bin", 0x600000, 0x100000, 0x12829c4c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c8.bin", 0x600001, 0x100000, 0xc944e03d ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "047-p1.bin", 0x000000, 0x080000, 0xbe40ea92 )
	ROM_LOAD16_WORD_SWAP( "047-p2.bin", 0x080000, 0x080000, 0x2a9beac5 )

	NEO_SFIX_128K( "047-s1.bin", 0xd7dbbf39 )

	NEO_BIOS_SOUND_128K( "047-m1.bin", 0x820b0ba7 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "047-v1.bin", 0x000000, 0x200000, 0xd9d00784 )
	ROM_LOAD( "047-v2.bin", 0x200000, 0x200000, 0x2c9a4b33 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "047-c1.bin", 0x000000, 0x100000, 0xf72a939e ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "047-c2.bin", 0x000001, 0x100000, 0x05119a0d ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "047-c3.bin", 0x200000, 0x100000, 0x01e00738 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "047-c4.bin", 0x200001, 0x100000, 0x9fe27432 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x600001, 0x100000 )
ROM_END

ROM_START( janshin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "048-p1.bin", 0x000000, 0x100000, 0x7514cb7a )

	NEO_SFIX_128K( "048-s1.bin", 0x8285b25a )

	NEO_BIOS_SOUND_64K( "048-m1.bin", 0xe191f955 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "048-v1.bin", 0x000000, 0x200000, 0xf1947d2b )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "048-c1.bin", 0x000000, 0x200000, 0x3fa890e9 )
	ROM_LOAD16_BYTE( "048-c2.bin", 0x000001, 0x200000, 0x59c48ad8 )
ROM_END

ROM_START( androdun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "049-p1.bin", 0x000000, 0x080000, 0x3b857da2 )
	ROM_LOAD16_WORD_SWAP( "049-p2.bin", 0x080000, 0x080000, 0x2f062209 )

	NEO_SFIX_128K( "049-s1.bin", 0x6349de5d )

	NEO_BIOS_SOUND_128K( "049-m1.bin", 0x1a009f8c )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "049-v1.bin", 0x000000, 0x080000, 0x577c85b3 )
	ROM_LOAD( "049-v2.bin", 0x080000, 0x080000, 0xe14551c4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "049-c1.bin", 0x000000, 0x100000, 0x7ace6db3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "049-c2.bin", 0x000001, 0x100000, 0xb17024f7 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "049-c3.bin", 0x200000, 0x100000, 0x2e0f3f9a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "049-c4.bin", 0x200001, 0x100000, 0x4a19fb92 ) /* Plane 2,3 */
ROM_END

ROM_START( ncommand )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "050-p1.bin", 0x000000, 0x100000, 0x4e097c40 )

	NEO_SFIX_128K( "050-s1.bin", 0xdb8f9c8e )

	NEO_BIOS_SOUND_128K( "050-m1.bin", 0x6fcf07d3 )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "050-v1.bin", 0x000000, 0x100000, 0x23c3ab42 )
	ROM_LOAD( "050-v2.bin", 0x100000, 0x080000, 0x80b8a984 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "050-c1.bin", 0x000000, 0x100000, 0x87421a0a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "050-c2.bin", 0x000001, 0x100000, 0xc4cf5548 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "050-c3.bin", 0x200000, 0x100000, 0x03422c1e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "050-c4.bin", 0x200001, 0x100000, 0x0845eadb ) /* Plane 2,3 */
ROM_END

ROM_START( viewpoin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "051-p1.bin", 0x000000, 0x100000, 0x17aa899d )

	NEO_SFIX_64K( "051-s1.bin", 0x6d0f146a )

	NEO_BIOS_SOUND_64K( "051-m1.bin", 0xd57bd7af )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "051-v1.bin", 0x000000, 0x200000, 0x019978b6 )
	ROM_LOAD( "051-v2.bin", 0x200000, 0x200000, 0x5758f38c )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "051-c1.bin", 0x000000, 0x100000, 0xd624c132 )
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "051-c2.bin", 0x000001, 0x100000, 0x40d69f1e )
	ROM_CONTINUE(      			   0x400001, 0x100000 )
ROM_END

ROM_START( ssideki )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "052-p1.bin", 0x000000, 0x080000, 0x9cd97256 )

	NEO_SFIX_128K( "052-s1.bin", 0x97689804 )

	NEO_BIOS_SOUND_128K( "052-m1.bin", 0x49f17d2d )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "052-v1.bin", 0x000000, 0x200000, 0x22c097a5 )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "052-c1.bin", 0x000000, 0x100000, 0x53e1c002 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "052-c2.bin", 0x000001, 0x100000, 0x776a2d1f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
ROM_END

ROM_START( wh1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "053-p1.bin", 0x000000, 0x080000, 0x95b574cb )
	ROM_LOAD16_WORD_SWAP( "053-p2.bin", 0x080000, 0x080000, 0xf198ed45 )

	NEO_SFIX_128K( "053-s1.bin", 0x8c2c2d6b )

	NEO_BIOS_SOUND_128K( "053-m1.bin", 0x1bd9d04b )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "053-v2.bin", 0x000000, 0x200000, 0xa68df485 )
	ROM_LOAD( "053-v4.bin", 0x200000, 0x100000, 0x7bea8f66 )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "053-c1.bin", 0x000000, 0x100000, 0x85eb5bce ) /* Plane 0,1 */
	ROM_CONTINUE(      			0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "053-c2.bin", 0x000001, 0x100000, 0xec93b048 ) /* Plane 2,3 */
	ROM_CONTINUE(      			0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "053-c3.bin", 0x200000, 0x100000, 0x0dd64965 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "053-c4.bin", 0x200001, 0x100000, 0x9270d954 ) /* Plane 2,3 */
ROM_END

ROM_START( kof94 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "055-p1.bin", 0x100000, 0x100000, 0xf10a2042 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "055-s1.bin", 0x825976c1 )

	NEO_BIOS_SOUND_128K( "055-m1.bin", 0xf6e77cf5 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "055-v1.bin", 0x000000, 0x200000, 0x8889596d )
	ROM_LOAD( "055-v2.bin", 0x200000, 0x200000, 0x25022b27 )
	ROM_LOAD( "055-v3.bin", 0x400000, 0x200000, 0x83cf32c0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "055-c1.bin", 0x000000, 0x200000, 0xb96ef460 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c2.bin", 0x000001, 0x200000, 0x15e096a7 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "055-c3.bin", 0x400000, 0x200000, 0x54f66254 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c4.bin", 0x400001, 0x200000, 0x0b01765f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "055-c5.bin", 0x800000, 0x200000, 0xee759363 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c6.bin", 0x800001, 0x200000, 0x498da52c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "055-c7.bin", 0xc00000, 0x200000, 0x62f66888 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c8.bin", 0xc00001, 0x200000, 0xfe0a235d ) /* Plane 2,3 */
ROM_END

ROM_START( aof2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "056-p1.bin", 0x000000, 0x100000, 0xa3b1d021 )

	NEO_SFIX_128K( "056-s1.bin", 0x8b02638e )

	NEO_BIOS_SOUND_128K( "056-m1.bin", 0xf27e9d52 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "056-v1.bin", 0x000000, 0x200000, 0x4628fde0 )
	ROM_LOAD( "056-v2.bin", 0x200000, 0x200000, 0xb710e2f2 )
	ROM_LOAD( "056-v3.bin", 0x400000, 0x100000, 0xd168c301 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "056-c1.bin", 0x000000, 0x200000, 0x17b9cbd2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c2.bin", 0x000001, 0x200000, 0x5fd76b67 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "056-c3.bin", 0x400000, 0x200000, 0xd2c88768 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c4.bin", 0x400001, 0x200000, 0xdb39b883 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "056-c5.bin", 0x800000, 0x200000, 0xc3074137 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c6.bin", 0x800001, 0x200000, 0x31de68d3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "056-c7.bin", 0xc00000, 0x200000, 0x3f36df57 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c8.bin", 0xc00001, 0x200000, 0xe546d7a8 ) /* Plane 2,3 */
ROM_END

ROM_START( wh2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "057-p1.bin", 0x100000, 0x100000, 0x65a891d9 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "057-s1.bin", 0xfcaeb3a4 )

	NEO_BIOS_SOUND_128K( "057-m1.bin", 0x8fa3bc77 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "057-v1.bin", 0x000000, 0x200000, 0x8877e301 )
	ROM_LOAD( "057-v2.bin", 0x200000, 0x200000, 0xc1317ff4 )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "057-c1.bin", 0x000000, 0x200000, 0x21c6bb91 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "057-c2.bin", 0x000001, 0x200000, 0xa3999925 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "057-c3.bin", 0x400000, 0x200000, 0xb725a219 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "057-c4.bin", 0x400001, 0x200000, 0x8d96425e ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "057-c5.bin", 0x800000, 0x200000, 0xb20354af ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "057-c6.bin", 0x800001, 0x200000, 0xb13d1de3 ) /* Plane 2,3 */
ROM_END

ROM_START( fatfursp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "058-p1.bin", 0x000000, 0x100000, 0x2f585ba2 )
	ROM_LOAD16_WORD_SWAP( "058-p2.bin", 0x100000, 0x080000, 0xd7c71a6b )
	ROM_LOAD16_WORD_SWAP( "058-p3.bin", 0x180000, 0x080000, 0x9f0c1e1a )

	NEO_SFIX_128K( "058-s1.bin", 0x2df03197 )

	NEO_BIOS_SOUND_128K( "058-m1.bin", 0xccc5186e )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "058-v1.bin", 0x000000, 0x200000, 0x55d7ce84 )
	ROM_LOAD( "058-v2.bin", 0x200000, 0x200000, 0xee080b10 )
	ROM_LOAD( "058-v3.bin", 0x400000, 0x100000, 0xf9eb3d4a )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "058-c1.bin", 0x000000, 0x200000, 0x044ab13c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c2.bin", 0x000001, 0x200000, 0x11e6bf96 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "058-c3.bin", 0x400000, 0x200000, 0x6f7938d5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c4.bin", 0x400001, 0x200000, 0x4ad066ff ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "058-c5.bin", 0x800000, 0x200000, 0x49c5e0bf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c6.bin", 0x800001, 0x200000, 0x8ff1f43d ) /* Plane 2,3 */
ROM_END

ROM_START( savagere )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "059-p1.bin", 0x100000, 0x100000, 0x01d4e9c0 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "059-s1.bin", 0xe08978ca )

	NEO_BIOS_SOUND_128K( "059-m1.bin", 0x29992eba )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "059-v1.bin", 0x000000, 0x200000, 0x530c50fd )
	ROM_LOAD( "059-v2.bin", 0x200000, 0x200000, 0xeb6f1cdb )
	ROM_LOAD( "059-v3.bin", 0x400000, 0x200000, 0x7038c2f9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "059-c1.bin", 0x000000, 0x200000, 0x763ba611 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c2.bin", 0x000001, 0x200000, 0xe05e8ca6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "059-c3.bin", 0x400000, 0x200000, 0x3e4eba4b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c4.bin", 0x400001, 0x200000, 0x3c2a3808 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "059-c5.bin", 0x800000, 0x200000, 0x59013f9e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c6.bin", 0x800001, 0x200000, 0x1c8d5def ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "059-c7.bin", 0xc00000, 0x200000, 0xc88f7035 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c8.bin", 0xc00001, 0x200000, 0x484ce3ba ) /* Plane 2,3 */
ROM_END

ROM_START( fightfev )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "060-p1.bin", 0x000000, 0x080000, 0x3032041b )
	ROM_LOAD16_WORD_SWAP( "060-p2.bin", 0x080000, 0x080000, 0xb0801d5f )

	NEO_SFIX_128K( "060-s1.bin", 0x70727a1e )

	NEO_BIOS_SOUND_128K( "060-m1.bin", 0x0b7c4e65 )

	ROM_REGION(  0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "060-v1.bin", 0x000000, 0x200000, 0xf417c215 )
	ROM_LOAD( "060-v2.bin", 0x200000, 0x100000, 0x64470036 )

	NO_DELTAT_REGION

	ROM_REGION( 0x0800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "060-c1.bin", 0x0000000, 0x200000, 0x8908fff9 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "060-c2.bin", 0x0000001, 0x200000, 0xc6649492 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "060-c3.bin", 0x0400000, 0x200000, 0x0956b437 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "060-c4.bin", 0x0400001, 0x200000, 0x026f3b62 ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "061-p1.bin", 0x000000, 0x100000, 0x5969e0dc )

	NEO_SFIX_128K( "061-s1.bin", 0x226d1b68 )

	NEO_BIOS_SOUND_128K( "061-m1.bin", 0x156f6951 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "061-v1.bin", 0x000000, 0x200000, 0xf081c8d3 )
	ROM_LOAD( "061-v2.bin", 0x200000, 0x200000, 0x7cd63302 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "061-c1.bin", 0x000000, 0x200000, 0xa626474f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "061-c2.bin", 0x000001, 0x200000, 0xc3be42ae ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "061-c3.bin", 0x400000, 0x200000, 0x2a7b98b9 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "061-c4.bin", 0x400001, 0x200000, 0xc0be9a1f ) /* Plane 2,3 */
ROM_END

ROM_START( spinmast )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "062-p1.bin", 0x000000, 0x100000, 0x37aba1aa )
	ROM_LOAD16_WORD_SWAP( "062-p2.bin", 0x100000, 0x080000, 0x43763ad2 )

	NEO_SFIX_128K( "062-s1.bin", 0x289e2bbe )

	NEO_BIOS_SOUND_128K( "062-m1.bin", 0x76108b2f )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "062-v1.bin", 0x000000, 0x100000, 0xcc281aef )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "062-c1.bin", 0x000000, 0x100000, 0xa9375aa2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c2.bin", 0x000001, 0x100000, 0x0e73b758 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "062-c3.bin", 0x200000, 0x100000, 0xdf51e465 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c4.bin", 0x200001, 0x100000, 0x38517e90 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "062-c5.bin", 0x400000, 0x100000, 0x7babd692 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c6.bin", 0x400001, 0x100000, 0xcde5ade5 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "062-c7.bin", 0x600000, 0x100000, 0xbb2fd7c0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c8.bin", 0x600001, 0x100000, 0x8d7be933 ) /* Plane 2,3 */
ROM_END

ROM_START( samsho2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "063-p1.bin", 0x100000, 0x100000, 0x22368892 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "063-s1.bin", 0x64a5cd66 )

	NEO_BIOS_SOUND_128K( "063-m1.bin", 0x56675098 )

	ROM_REGION( 0x700000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "063-v1.bin", 0x000000, 0x200000, 0x37703f91 )
	ROM_LOAD( "063-v2.bin", 0x200000, 0x200000, 0x0142bde8 )
	ROM_LOAD( "063-v3.bin", 0x400000, 0x200000, 0xd07fa5ca )
	ROM_LOAD( "063-v4.bin", 0x600000, 0x100000, 0x24aab4bb )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "063-c1.bin", 0x000000, 0x200000, 0x86cd307c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c2.bin", 0x000001, 0x200000, 0xcdfcc4ca ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "063-c3.bin", 0x400000, 0x200000, 0x7a63ccc7 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c4.bin", 0x400001, 0x200000, 0x751025ce ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "063-c5.bin", 0x800000, 0x200000, 0x20d3a475 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c6.bin", 0x800001, 0x200000, 0xae4c0a88 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "063-c7.bin", 0xc00000, 0x200000, 0x2df3cbcf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c8.bin", 0xc00001, 0x200000, 0x1ffc6dfa ) /* Plane 2,3 */
ROM_END

ROM_START( wh2j )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "064-p1.bin", 0x100000, 0x100000, 0x385a2e86 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "064-s1.bin", 0x2a03998a )

	NEO_BIOS_SOUND_128K( "064-m1.bin", 0xd2eec9d3 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "064-v1.bin", 0x000000, 0x200000, 0xaa277109 )
	ROM_LOAD( "064-v2.bin", 0x200000, 0x200000, 0xb6527edd )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "064-c1.bin", 0x000000, 0x200000, 0x2ec87cea ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c2.bin", 0x000001, 0x200000, 0x526b81ab ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "064-c3.bin", 0x400000, 0x200000, 0x436d1b31 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c4.bin", 0x400001, 0x200000, 0xf9c8dd26 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "064-c5.bin", 0x800000, 0x200000, 0x8e34a9f4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c6.bin", 0x800001, 0x200000, 0xa43e4766 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "064-c7.bin", 0xc00000, 0x200000, 0x59d97215 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c8.bin", 0xc00001, 0x200000, 0xfc092367 ) /* Plane 0,1 */
ROM_END

ROM_START( wjammers )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "065-p1.bin", 0x000000, 0x080000, 0xe81e7a31 )

	NEO_SFIX_128K( "065-s1.bin", 0x074b5723 )

	NEO_BIOS_SOUND_128K( "065-m1.bin", 0x52c23cfc )

	ROM_REGION( 0x380000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "065-v1.bin", 0x000000, 0x100000, 0xce8b3698 )
	ROM_LOAD( "065-v2.bin", 0x100000, 0x100000, 0x659f9b96 )
	ROM_LOAD( "065-v3.bin", 0x200000, 0x100000, 0x39f73061 )
	ROM_LOAD( "065-v4.bin", 0x300000, 0x080000, 0x3740edde )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "065-c1.bin", 0x000000, 0x100000, 0xc7650204 )
	ROM_LOAD16_BYTE( "065-c2.bin", 0x000001, 0x100000, 0xd9f3e71d )
	ROM_LOAD16_BYTE( "065-c3.bin", 0x200000, 0x100000, 0x40986386 )
	ROM_LOAD16_BYTE( "065-c4.bin", 0x200001, 0x100000, 0x715e15ff )
ROM_END

ROM_START( karnovr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "066-p1.bin", 0x000000, 0x100000, 0x8c86fd22 )

	NEO_SFIX_128K( "066-s1.bin", 0xbae5d5e5 )

	NEO_BIOS_SOUND_128K( "066-m1.bin", 0x030beae4 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "066-v1.bin", 0x000000, 0x200000, 0x0b7ea37a )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "066-c1.bin", 0x000000, 0x200000, 0x09dfe061 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "066-c2.bin", 0x000001, 0x200000, 0xe0f6682a ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "066-c3.bin", 0x400000, 0x200000, 0xa673b4f7 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "066-c4.bin", 0x400001, 0x200000, 0xcb3dc5f4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "066-c5.bin", 0x800000, 0x200000, 0x9a28785d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "066-c6.bin", 0x800001, 0x200000, 0xc15c01ed ) /* Plane 2,3 */
ROM_END

ROM_START( gururin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "067-p1.bin", 0x000000, 0x80000, 0x4cea8a49 )

	NEO_SFIX_128K( "067-s1.bin", 0x4f0cbd58 )

	NEO_BIOS_SOUND_64K( "067-m1.bin", 0x833cdf1b )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "067-v1.bin", 0x000000, 0x80000, 0xcf23afd0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "067-c1.bin", 0x000000, 0x200000, 0x35866126 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "067-c2.bin", 0x000001, 0x200000, 0x9db64084 ) /* Plane 2,3 */
ROM_END

ROM_START( pspikes2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "068-p1.bin", 0x000000, 0x100000, 0x105a408f )

	NEO_SFIX_128K( "068-s1.bin", 0x18082299 )

	NEO_BIOS_SOUND_128K( "068-m1.bin", 0xb1c7911e )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "068-v1.bin", 0x000000, 0x100000, 0x2ced86df )
	ROM_LOAD( "068-v2.bin", 0x100000, 0x100000, 0x970851ab )
	ROM_LOAD( "068-v3.bin", 0x200000, 0x100000, 0x81ff05aa )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "068-c1.bin", 0x000000, 0x100000, 0x7f250f76 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "068-c2.bin", 0x000001, 0x100000, 0x20912873 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "068-c3.bin", 0x200000, 0x100000, 0x4b641ba1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "068-c4.bin", 0x200001, 0x100000, 0x35072596 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "068-c5.bin", 0x400000, 0x100000, 0x151dd624 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "068-c6.bin", 0x400001, 0x100000, 0xa6722604 ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "069-p1.bin", 0x000000, 0x100000, 0xa8bcfbbc )
	ROM_LOAD16_WORD_SWAP( "069-p2.bin", 0x100000, 0x200000, 0xdbe963ed )

	NEO_SFIX_128K( "069-s1.bin", 0x0b33a800 )

	NEO_BIOS_SOUND_128K( "069-m1.bin", 0xfce72926 )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "069-v1.bin", 0x000000, 0x400000, 0x2bdbd4db )
	ROM_LOAD( "069-v2.bin", 0x400000, 0x400000, 0xa698a487 )
	ROM_LOAD( "069-v3.bin", 0x800000, 0x200000, 0x581c5304 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "069-c1.bin", 0x0000000, 0x400000, 0xe302f93c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c2.bin", 0x0000001, 0x400000, 0x1053a455 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c3.bin", 0x0800000, 0x400000, 0x1c0fde2f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c4.bin", 0x0800001, 0x400000, 0xa25fc3d0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c5.bin", 0x1000000, 0x200000, 0xb3ec6fa6 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c6.bin", 0x1000001, 0x200000, 0x69210441 ) /* Plane 2,3 */
ROM_END

ROM_START( panicbom )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "073-p1.bin", 0x000000, 0x040000, 0x0b21130d )

	NEO_SFIX_128K( "073-s1.bin", 0xb876de7e )

	NEO_BIOS_SOUND_128K( "073-m1.bin", 0x3cdf5d88 )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "073-v1.bin", 0x000000, 0x200000, 0x7fc86d2f )
	ROM_LOAD( "073-v2.bin", 0x200000, 0x100000, 0x082adfc7 )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "073-c1.bin", 0x000000, 0x100000, 0x8582e1b5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "073-c2.bin", 0x000001, 0x100000, 0xe15a093b ) /* Plane 2,3 */
ROM_END

ROM_START( aodk )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "074-p1.bin", 0x100000, 0x100000, 0x62369553 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "074-s1.bin", 0x96148d2b )

	NEO_BIOS_SOUND_128K( "074-m1.bin", 0x5a52a9d1 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "074-v1.bin", 0x000000, 0x200000, 0x7675b8fa )
	ROM_LOAD( "074-v2.bin", 0x200000, 0x200000, 0xa9da86e9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "074-c1.bin", 0x000000, 0x200000, 0xa0b39344 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c2.bin", 0x000001, 0x200000, 0x203f6074 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "074-c3.bin", 0x400000, 0x200000, 0x7fff4d41 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c4.bin", 0x400001, 0x200000, 0x48db3e0a ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "074-c5.bin", 0x800000, 0x200000, 0xc74c5e51 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c6.bin", 0x800001, 0x200000, 0x73e8e7e0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "074-c7.bin", 0xc00000, 0x200000, 0xac7daa01 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c8.bin", 0xc00001, 0x200000, 0x14e7ad71 ) /* Plane 2,3 */
ROM_END

ROM_START( sonicwi2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "075-p1.bin", 0x100000, 0x100000, 0x92871738 )
	ROM_CONTINUE(                         0x000000, 0x100000 )

	NEO_SFIX_128K( "075-s1.bin", 0xc9eec367 )

	NEO_BIOS_SOUND_128K( "075-m1.bin", 0xbb828df1 )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "075-v1.bin", 0x000000, 0x200000, 0x7577e949 )
	ROM_LOAD( "075-v2.bin", 0x200000, 0x080000, 0x6d0a728e )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "075-c1.bin", 0x000000, 0x200000, 0x3278e73e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "075-c2.bin", 0x000001, 0x200000, 0xfe6355d6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "075-c3.bin", 0x400000, 0x200000, 0xc1b438f1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "075-c4.bin", 0x400001, 0x200000, 0x1f777206 ) /* Plane 2,3 */
ROM_END

ROM_START( zedblade )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "076-p1.bin", 0x000000, 0x080000, 0xd7c1effd )

	NEO_SFIX_128K( "076-s1.bin", 0xf4c25dd5 )

	NEO_BIOS_SOUND_128K( "076-m1.bin", 0x7b5f3d0a )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "076-v1.bin", 0x000000, 0x200000, 0x1a21d90c )
	ROM_LOAD( "076-v2.bin", 0x200000, 0x200000, 0xb61686c3 )
	ROM_LOAD( "076-v3.bin", 0x400000, 0x100000, 0xb90658fa )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "076-c1.bin", 0x000000, 0x200000, 0x4d9cb038 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "076-c2.bin", 0x000001, 0x200000, 0x09233884 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "076-c3.bin", 0x400000, 0x200000, 0xd06431e3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "076-c4.bin", 0x400001, 0x200000, 0x4b1c089b ) /* Plane 2,3 */
ROM_END

ROM_START( galaxyfg )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "078-p1.bin", 0x100000, 0x100000, 0x45906309 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "078-s1.bin", 0x72f8923e )

	NEO_BIOS_SOUND_128K( "078-m1.bin", 0x8e9e3b10 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "078-v1.bin", 0x000000, 0x200000, 0xe3b735ac )
	ROM_LOAD( "078-v2.bin", 0x200000, 0x200000, 0x6a8e78c2 )
	ROM_LOAD( "078-v3.bin", 0x400000, 0x100000, 0x70bca656 )

	NO_DELTAT_REGION

	ROM_REGION( 0xe00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "078-c1.bin", 0x000000, 0x200000, 0xc890c7c0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c2.bin", 0x000001, 0x200000, 0xb6d25419 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "078-c3.bin", 0x400000, 0x200000, 0x9d87e761 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c4.bin", 0x400001, 0x200000, 0x765d7cb8 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "078-c5.bin", 0x800000, 0x200000, 0xe6b77e6a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c6.bin", 0x800001, 0x200000, 0xd779a181 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "078-c7.bin", 0xc00000, 0x100000, 0x4f27d580 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c8.bin", 0xc00001, 0x100000, 0x0a7cc0d8 ) /* Plane 2,3 */
ROM_END

ROM_START( strhoop )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "079-p1.bin", 0x000000, 0x100000, 0x5e78328e )

	NEO_SFIX_128K( "079-s1.bin", 0x3ac06665 )

	NEO_BIOS_SOUND_64K( "079-m1.bin", 0x1a5f08db )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "079-v1.bin", 0x000000, 0x200000, 0x718a2400 )
	ROM_LOAD( "079-v2.bin", 0x200000, 0x080000, 0xb19884f8 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "079-c1.bin", 0x000000, 0x200000, 0x0581c72a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "079-c2.bin", 0x000001, 0x200000, 0x5b9b8fb6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "079-c3.bin", 0x400000, 0x200000, 0xcd65bb62 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "079-c4.bin", 0x400001, 0x200000, 0xa4c90213 ) /* Plane 2,3 */
ROM_END

ROM_START( quizkof )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "080-p1.bin", 0x000000, 0x100000, 0x4440315e )

	NEO_SFIX_128K( "080-s1.bin", 0xd7b86102 )

	NEO_BIOS_SOUND_128K( "080-m1.bin", 0xf5f44172 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "080-v1.bin", 0x000000, 0x200000, 0x0be18f60 )
	ROM_LOAD( "080-v2.bin", 0x200000, 0x200000, 0x4abde3ff )
	ROM_LOAD( "080-v3.bin", 0x400000, 0x200000, 0xf02844e2 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "080-c1.bin",  0x000000, 0x200000, 0xea1d764a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "080-c2.bin",  0x000001, 0x200000, 0xc78c49da ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "080-c3.bin",  0x400000, 0x200000, 0xb4851bfe ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "080-c4.bin",  0x400001, 0x200000, 0xca6f5460 ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "081-p1.bin", 0x100000, 0x100000, 0x6bc27a3d )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "081-s1.bin", 0x7626da34 )

	NEO_BIOS_SOUND_128K( "081-m1.bin", 0x82fcd863 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "081-v1.bin", 0x000000, 0x200000, 0x201fa1e1 )
	ROM_LOAD( "081-v2.bin", 0x200000, 0x200000, 0xacf29d96 )
	ROM_LOAD( "081-v3.bin", 0x400000, 0x200000, 0xe524e415 )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "081-c1.bin", 0x000000, 0x200000, 0x1fb68ebe ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "081-c2.bin", 0x000001, 0x200000, 0xb28d928f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "081-c3.bin", 0x400000, 0x200000, 0x3b2572e8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "081-c4.bin", 0x400001, 0x200000, 0x47d26a7c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "081-c5.bin", 0x800000, 0x200000, 0x17d42f0d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "081-c6.bin", 0x800001, 0x200000, 0x6b53fb75 ) /* Plane 2,3 */
ROM_END

ROM_START( doubledr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "082-p1.bin", 0x100000, 0x100000, 0x34ab832a )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "082-s1.bin", 0xbef995c5 )

	NEO_BIOS_SOUND_128K( "082-m1.bin", 0x10b144de )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "082-v1.bin", 0x000000, 0x200000, 0xcc1128e4 )
	ROM_LOAD( "082-v2.bin", 0x200000, 0x200000, 0xc3ff5554 )

	NO_DELTAT_REGION

	ROM_REGION( 0xe00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "082-c1.bin", 0x000000, 0x200000, 0xb478c725 )
	ROM_LOAD16_BYTE( "082-c2.bin", 0x000001, 0x200000, 0x2857da32 )
	ROM_LOAD16_BYTE( "082-c3.bin", 0x400000, 0x200000, 0x8b0d378e )
	ROM_LOAD16_BYTE( "082-c4.bin", 0x400001, 0x200000, 0xc7d2f596 )
	ROM_LOAD16_BYTE( "082-c5.bin", 0x800000, 0x200000, 0xec87bff6 )
	ROM_LOAD16_BYTE( "082-c6.bin", 0x800001, 0x200000, 0x844a8a11 )
	ROM_LOAD16_BYTE( "082-c7.bin", 0xc00000, 0x100000, 0x727c4d02 )
	ROM_LOAD16_BYTE( "082-c8.bin", 0xc00001, 0x100000, 0x69a5fa37 )
ROM_END

ROM_START( pbobblen )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "083-p1.bin", 0x000000, 0x040000, 0x7c3c34e1 )

	NEO_SFIX_128K( "083-s1.bin", 0x9caae538 )

	NEO_BIOS_SOUND_64K( "083-m1.bin", 0x129e6054 )

	ROM_REGION( 0x380000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* 0x000000-0x1fffff empty */
	ROM_LOAD( "083-v3.bin", 0x200000, 0x100000, 0x0840cbc4 )
	ROM_LOAD( "083-v4.bin", 0x300000, 0x080000, 0x0a548948 )

	NO_DELTAT_REGION

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "083-c5.bin", 0x000000, 0x080000, 0xe89ad494 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "083-c6.bin", 0x000001, 0x080000, 0x4b42d7eb ) /* Plane 2,3 */
ROM_END

ROM_START( kof95 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "084-p1.bin", 0x100000, 0x100000, 0x5e54cf95 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "084-s1.bin", 0xde716f8a )

	NEO_BIOS_SOUND_128K( "084-m1.bin", 0x6f2d7429 )

	ROM_REGION( 0x900000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "084-v1.bin", 0x000000, 0x400000, 0x21469561 )
	ROM_LOAD( "084-v2.bin", 0x400000, 0x200000, 0xb38a2803 )
	/* 600000-7fffff empty */
	ROM_LOAD( "084-v3.bin", 0x800000, 0x100000, 0xd683a338 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "084-c1.bin", 0x0400000, 0x200000, 0x33bf8657 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "084-c2.bin", 0x0400001, 0x200000, 0xf21908a4 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "084-c3.bin", 0x0c00000, 0x200000, 0x0cee1ddb ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "084-c4.bin", 0x0c00001, 0x200000, 0x729db15d ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "084-c5.bin", 0x1000000, 0x200000, 0x8a2c1edc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "084-c6.bin", 0x1000001, 0x200000, 0xf593ac35 ) /* Plane 2,3 */
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "084-c7.bin", 0x1800000, 0x100000, 0x9904025f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "084-c8.bin", 0x1800001, 0x100000, 0x78eb0f9b ) /* Plane 2,3 */
ROM_END

ROM_START( tws96 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "086-p1.bin", 0x000000, 0x100000, 0x03e20ab6 )

	NEO_SFIX_128K( "086-s1.bin", 0x6f5e2b3a )

	NEO_BIOS_SOUND_64K( "086-m1.bin", 0x860ba8c7 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "086-v1.bin", 0x000000, 0x200000, 0x97bf1986 )
	ROM_LOAD( "086-v2.bin", 0x200000, 0x200000, 0xb7eb05df )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "086-c1.bin", 0x400000, 0x200000, 0xd301a867 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "086-c2.bin", 0x400001, 0x200000, 0x305fc74f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "086-c3.bin", 0x800000, 0x100000, 0x750ddc0c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "086-c4.bin", 0x800001, 0x100000, 0x7a6e7d82 ) /* Plane 2,3 */
ROM_END

ROM_START( samsho3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "087-p1.bin", 0x000000, 0x100000, 0x282a336e )
	ROM_LOAD16_WORD_SWAP( "087-p2.bin", 0x100000, 0x200000, 0x9bbe27e0 )

	NEO_SFIX_128K( "087-s1.bin", 0x74ec7d9f )

	NEO_BIOS_SOUND_128K( "087-m1.bin", 0x8e6440eb )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "087-v1.bin", 0x000000, 0x400000, 0x84bdd9a0 )
	ROM_LOAD( "087-v2.bin", 0x400000, 0x200000, 0xac0f261a )

	NO_DELTAT_REGION

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 )	/* lowering this to 0x1900000 corrupts the graphics */
	ROM_LOAD16_BYTE( "087-c1.bin", 0x0400000, 0x200000, 0xe079f767 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "087-c2.bin", 0x0400001, 0x200000, 0xfc045909 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "087-c3.bin", 0x0c00000, 0x200000, 0xc61218d7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "087-c4.bin", 0x0c00001, 0x200000, 0x054ec754 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "087-c5.bin", 0x1400000, 0x200000, 0x05feee47 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "087-c6.bin", 0x1400001, 0x200000, 0xef7d9e29 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "087-c7.bin", 0x1800000, 0x080000, 0x7a01f666 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "087-c8.bin", 0x1800001, 0x080000, 0xffd009c2 ) /* Plane 2,3 */
ROM_END

ROM_START( stakwin )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "088-p1.bin",  0x100000, 0x100000, 0xbd5814f6 )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "088-s1.bin", 0x073cb208 )

	NEO_BIOS_SOUND_128K( "088-m1.bin", 0x2fe1f499 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "088-v1.bin", 0x000000, 0x200000, 0xb7785023 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "088-c1.bin", 0x000000, 0x200000, 0x6e733421 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "088-c2.bin", 0x000001, 0x200000, 0x4d865347 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "088-c3.bin", 0x400000, 0x200000, 0x8fa5a9eb ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "088-c4.bin", 0x400001, 0x200000, 0x4604f0dc ) /* Plane 2,3 */
ROM_END

ROM_START( pulstar )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "089-p1.bin", 0x000000, 0x100000, 0x5e5847a2 )
	ROM_LOAD16_WORD_SWAP( "089-p2.bin", 0x100000, 0x200000, 0x028b774c )

	NEO_SFIX_128K( "089-s1.bin", 0xc79fc2c8 )

	NEO_BIOS_SOUND_128K( "089-m1.bin", 0xff3df7c7 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "089-v1.bin", 0x000000, 0x400000, 0xb458ded2 )
	ROM_LOAD( "089-v2.bin", 0x400000, 0x400000, 0x9d2db551 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "089-c1.bin", 0x0400000, 0x200000, 0x63020fc6 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "089-c2.bin", 0x0400001, 0x200000, 0x260e9d4d ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "089-c3.bin", 0x0c00000, 0x200000, 0x21ef41d7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "089-c4.bin", 0x0c00001, 0x200000, 0x3b9e288f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "089-c5.bin", 0x1400000, 0x200000, 0x6fe9259c ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "089-c6.bin", 0x1400001, 0x200000, 0xdc32f2b4 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "089-c7.bin", 0x1800000, 0x200000, 0x6a5618ca ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "089-c8.bin", 0x1800001, 0x200000, 0xa223572d ) /* Plane 2,3 */
ROM_END

ROM_START( whp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "090-p1.bin", 0x100000, 0x100000, 0xafaa4702 )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "090-s1.bin",  0x174a880f )

	NEO_BIOS_SOUND_128K( "090-m1.bin", 0x28065668 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "090-v1.bin", 0x000000, 0x200000, 0x30cf2709 )
	ROM_LOAD( "090-v2.bin", 0x200000, 0x200000, 0xb6527edd )
	ROM_LOAD( "090-v3.bin", 0x400000, 0x200000, 0x1908a7ce )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "090-c1.bin", 0x0400000, 0x200000, 0xaecd5bb1 )
	ROM_CONTINUE(      			 0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "090-c2.bin", 0x0400001, 0x200000, 0x7566ffc0 )
	ROM_CONTINUE(      			 0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "090-c3.bin", 0x0800000, 0x200000, 0x436d1b31 )
	ROM_LOAD16_BYTE( "090-c4.bin", 0x0800001, 0x200000, 0xf9c8dd26 )
	/* 0c00000-0ffffff empty */
	ROM_LOAD16_BYTE( "090-c5.bin", 0x1000000, 0x200000, 0x8e34a9f4 )
	ROM_LOAD16_BYTE( "090-c6.bin", 0x1000001, 0x200000, 0xa43e4766 )
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "090-c7.bin", 0x1800000, 0x200000, 0x59d97215 )
	ROM_LOAD16_BYTE( "090-c8.bin", 0x1800001, 0x200000, 0xfc092367 )
ROM_END

ROM_START( kabukikl )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "092-p1.bin", 0x100000, 0x100000, 0x28ec9b77 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "092-s1.bin", 0xa3d68ee2 )

	NEO_BIOS_SOUND_128K( "092-m1.bin", 0x91957ef6 )

	ROM_REGION( 0x700000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "092-v1.bin", 0x000000, 0x200000, 0x69e90596 )
	ROM_LOAD( "092-v2.bin", 0x200000, 0x200000, 0x7abdb75d )
	ROM_LOAD( "092-v3.bin", 0x400000, 0x200000, 0xeccc98d3 )
	ROM_LOAD( "092-v4.bin", 0x600000, 0x100000, 0xa7c9c949 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "092-c1.bin", 0x400000, 0x200000, 0x4d896a58 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "092-c2.bin", 0x400001, 0x200000, 0x3cf78a18 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "092-c3.bin", 0xc00000, 0x200000, 0x58c454e7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "092-c4.bin", 0xc00001, 0x200000, 0xe1a8aa6a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( neobombe )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "093-p1.bin", 0x000000, 0x100000, 0xa1a71d0d )

	NEO_SFIX_128K( "093-s1.bin", 0x4b3fa119 )

	NEO_BIOS_SOUND_128K( "093-m1.bin", 0xe81e780b )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "093-v1.bin", 0x200000, 0x200000, 0x43057e99 )
	ROM_CONTINUE(			  0x000000, 0x200000 )
	ROM_LOAD( "093-v2.bin", 0x400000, 0x200000, 0xa92b8b3d )

	NO_DELTAT_REGION

	ROM_REGION( 0x900000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "093-c1.bin", 0x400000, 0x200000, 0xb90ebed4 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "093-c2.bin", 0x400001, 0x200000, 0x41e62b4f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "093-c3.bin", 0x800000, 0x080000, 0xe37578c5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "093-c4.bin", 0x800001, 0x080000, 0x59826783 ) /* Plane 2,3 */
ROM_END

ROM_START( gowcaizr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "094-p1.bin", 0x100000, 0x100000, 0x33019545 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "094-s1.bin", 0x2f8748a2 )

	NEO_BIOS_SOUND_128K( "094-m1.bin", 0x78c851cb )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "094-v1.bin", 0x000000, 0x200000, 0x6c31223c )
	ROM_LOAD( "094-v2.bin", 0x200000, 0x200000, 0x8edb776c )
	ROM_LOAD( "094-v3.bin", 0x400000, 0x100000, 0xc63b9285 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "094-c1.bin", 0x000000, 0x200000, 0x042f6af5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c2.bin", 0x000001, 0x200000, 0x0fbcd046 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "094-c3.bin", 0x400000, 0x200000, 0x58bfbaa1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c4.bin", 0x400001, 0x200000, 0x9451ee73 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "094-c5.bin", 0x800000, 0x200000, 0xff9cf48c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c6.bin", 0x800001, 0x200000, 0x31bbd918 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "094-c7.bin", 0xc00000, 0x200000, 0x2091ec04 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c8.bin", 0xc00001, 0x200000, 0x0d31dee6 ) /* Plane 2,3 */
ROM_END

ROM_START( rbff1 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "095-p1.bin", 0x000000, 0x100000, 0x63b4d8ae )
	ROM_LOAD16_WORD_SWAP( "095-p2.bin", 0x100000, 0x200000, 0xcc15826e )

	NEO_SFIX_128K( "095-s1.bin", 0xb6bf5e08 )

	NEO_BIOS_SOUND_128K( "095-m1.bin", 0x653492a7 )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "069-v1.bin", 0x000000, 0x400000, 0x2bdbd4db )
	ROM_LOAD( "069-v2.bin", 0x400000, 0x400000, 0xa698a487 )
	ROM_LOAD( "095-v3.bin", 0x800000, 0x400000, 0x189d1c6c )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "069-c1.bin", 0x0000000, 0x400000, 0xe302f93c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c2.bin", 0x0000001, 0x400000, 0x1053a455 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c3.bin", 0x0800000, 0x400000, 0x1c0fde2f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c4.bin", 0x0800001, 0x400000, 0xa25fc3d0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "095-c5.bin", 0x1000000, 0x400000, 0x8b9b65df ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "095-c6.bin", 0x1000001, 0x400000, 0x3e164718 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "095-c7.bin", 0x1800000, 0x200000, 0xca605e12 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "095-c8.bin", 0x1800001, 0x200000, 0x4e6beb6c ) /* Plane 2,3 */
ROM_END

ROM_START( aof3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "096-p1.bin", 0x000000, 0x100000, 0x9edb420d )
	ROM_LOAD16_WORD_SWAP( "096-p2.bin", 0x100000, 0x200000, 0x4d5a2602 )

	NEO_SFIX_128K( "096-s1.bin", 0xcc7fd344 )

	NEO_BIOS_SOUND_128K( "096-m1.bin", 0xcb07b659 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "096-v1.bin", 0x000000, 0x200000, 0xe2c32074 )
	ROM_LOAD( "096-v2.bin", 0x200000, 0x200000, 0xa290eee7 )
	ROM_LOAD( "096-v3.bin", 0x400000, 0x200000, 0x199d12ea )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "096-c1.bin", 0x0400000, 0x200000, 0xf6c74731 ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "096-c2.bin", 0x0400001, 0x200000, 0xf587f149 ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "096-c3.bin", 0x0c00000, 0x200000, 0x7749f5e6 ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "096-c4.bin", 0x0c00001, 0x200000, 0xcbd58369 ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "096-c5.bin", 0x1400000, 0x200000, 0x1718bdcd ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "096-c6.bin", 0x1400001, 0x200000, 0x4fca967f ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "096-c7.bin", 0x1800000, 0x200000, 0x51bd8ab2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "096-c8.bin", 0x1800001, 0x200000, 0x9a34f99c ) /* Plane 2,3 */
ROM_END

ROM_START( sonicwi3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "097-p1.bin", 0x100000, 0x100000, 0x0547121d )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "097-s1.bin", 0x8dd66743 )

	NEO_BIOS_SOUND_128K( "097-m1.bin", 0xb20e4291 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "097-v1.bin", 0x000000, 0x400000, 0x6f885152 )
	ROM_LOAD( "097-v2.bin", 0x400000, 0x100000, 0x32187ccd )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "097-c1.bin", 0x400000, 0x200000, 0x3ca97864 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "097-c2.bin", 0x400001, 0x200000, 0x1da4b3a9 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "097-c3.bin", 0x800000, 0x200000, 0xc339fff5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "097-c4.bin", 0x800001, 0x200000, 0x84a40c6e ) /* Plane 2,3 */
ROM_END

ROM_START( turfmast )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "200-p1.bin",  0x100000, 0x100000, 0x28c83048 )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "200-s1.bin", 0x9a5402b2 )

	NEO_BIOS_SOUND_128K( "200-m1.bin", 0x9994ac00 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "200-v1.bin", 0x000000, 0x200000, 0x00fd48d2 )
	ROM_LOAD( "200-v2.bin", 0x200000, 0x200000, 0x082acb31 )
	ROM_LOAD( "200-v3.bin", 0x400000, 0x200000, 0x7abca053 )
	ROM_LOAD( "200-v4.bin", 0x600000, 0x200000, 0x6c7b4902 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "200-c1.bin", 0x400000, 0x200000, 0x8c6733f2 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "200-c2.bin", 0x400001, 0x200000, 0x596cc256 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
ROM_END

ROM_START( mslug )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "201-p1.bin", 0x100000, 0x100000, 0x08d8daa5 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "201-s1.bin", 0x2f55958d )

	NEO_BIOS_SOUND_128K( "201-m1.bin", 0xc28b3253 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "201-v1.bin", 0x000000, 0x400000, 0x23d22ed1 )
	ROM_LOAD( "201-v2.bin", 0x400000, 0x400000, 0x472cf9db )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "201-c1.bin", 0x400000, 0x200000, 0xd00bd152 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "201-c2.bin", 0x400001, 0x200000, 0xddff1dea ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "201-c3.bin", 0xc00000, 0x200000, 0xd3d5f9e5 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "201-c4.bin", 0xc00001, 0x200000, 0x5ac1d497 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( puzzledp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "202-p1.bin", 0x000000, 0x080000, 0x2b61415b )

	NEO_SFIX_64K( "202-s1.bin", 0x4a421612 )

	NEO_BIOS_SOUND_128K( "202-m1.bin", 0x9c0291ea )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "202-v1.bin", 0x000000, 0x080000, 0xdebeb8fb )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "202-c1.bin", 0x000000, 0x100000, 0xcc0095ef ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "202-c2.bin", 0x000001, 0x100000, 0x42371307 ) /* Plane 2,3 */
ROM_END

ROM_START( mosyougi )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "203-p1.bin", 0x000000, 0x100000, 0x7ba70e2d )

	NEO_SFIX_128K( "203-s1.bin", 0x4e132fac )

	NEO_BIOS_SOUND_128K( "203-m1.bin", 0xa602c2c2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "203-v1.bin", 0x000000, 0x200000, 0xbaa2b9a5 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "203-c1.bin",  0x000000, 0x200000, 0xbba9e8c0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "203-c2.bin",  0x000001, 0x200000, 0x2574be03 ) /* Plane 2,3 */
ROM_END

ROM_START( marukodq )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "206-p1.bin", 0x000000, 0x100000, 0xc33ed21e )

	NEO_SFIX_32K( "206-s1.bin", 0x3b52a219 )

	NEO_BIOS_SOUND_128K( "206-m1.bin", 0x0e22902e )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "206-v1.bin", 0x000000, 0x200000, 0x5385eca8 )
	ROM_LOAD( "206-v2.bin", 0x200000, 0x200000, 0xf8c55404 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "206-c1.bin", 0x000000, 0x400000, 0x4bd5e70f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "206-c2.bin", 0x000001, 0x400000, 0x67dbe24d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "206-c3.bin", 0x800000, 0x100000, 0x79aa2b48 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "206-c4.bin", 0x800001, 0x100000, 0x55e1314d ) /* Plane 2,3 */
ROM_END

ROM_START( neomrdo )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "207-p1.bin", 0x000000, 0x80000, 0x39efdb82 )

	NEO_SFIX_64K( "207-s1.bin", 0x6c4b09c4 )

	NEO_BIOS_SOUND_128K( "207-m1.bin", 0x81eade02 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "207-v1.bin", 0x000000, 0x200000, 0x4143c052 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "207-c1.bin", 0x000000, 0x200000, 0xc7541b9d )
	ROM_LOAD16_BYTE( "207-c2.bin", 0x000001, 0x200000, 0xf57166d2 )
ROM_END

ROM_START( sdodgeb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "208-p1.bin", 0x100000, 0x100000, 0x127f3d32 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "208-s1.bin", 0x64abd6b3 )

	NEO_BIOS_SOUND_128K( "208-m1.bin", 0x0a5f3325 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "208-v1.bin", 0x000000, 0x200000, 0x8b53e945 )
	ROM_LOAD( "208-v2.bin", 0x200000, 0x200000, 0xaf37ebf8 )

	NO_DELTAT_REGION

	ROM_REGION( 0x0c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "208-c1.bin", 0x0000000, 0x400000, 0x93d8619b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "208-c2.bin", 0x0000001, 0x400000, 0x1c737bb6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "208-c3.bin", 0x0800000, 0x200000, 0x14cb1703 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "208-c4.bin", 0x0800001, 0x200000, 0xc7165f19 ) /* Plane 2,3 */
ROM_END

ROM_START( goalx3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "209-p1.bin", 0x100000, 0x100000, 0x2a019a79 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "209-s1.bin", 0xc0eaad86 )

	NEO_BIOS_SOUND_64K( "209-m1.bin", 0xdd945773 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "209-v1.bin", 0x000000, 0x200000, 0xef214212 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "209-c1.bin", 0x400000, 0x200000, 0xd061f1f5 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "209-c2.bin", 0x400001, 0x200000, 0x3f63c1a2 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "209-c3.bin", 0x800000, 0x100000, 0x5f91bace ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "209-c4.bin", 0x800001, 0x100000, 0x1e9f76f2 ) /* Plane 2,3 */
ROM_END

ROM_START( overtop )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "212-p1.bin", 0x100000, 0x100000, 0x16c063a9 )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "212-s1.bin",  0x481d3ddc )

	NEO_BIOS_SOUND_128K( "212-m1.bin", 0xfcab6191 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "212-v1.bin", 0x000000, 0x400000, 0x013d4ef9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "212-c1.bin", 0x0000000, 0x400000, 0x50f43087 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "212-c2.bin", 0x0000001, 0x400000, 0xa5b39807 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "212-c3.bin", 0x0800000, 0x400000, 0x9252ea02 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "212-c4.bin", 0x0800001, 0x400000, 0x5f41a699 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "212-c5.bin", 0x1000000, 0x200000, 0xfc858bef ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "212-c6.bin", 0x1000001, 0x200000, 0x0589c15e ) /* Plane 2,3 */
ROM_END

ROM_START( neodrift )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "213-p1.bin",  0x100000, 0x100000, 0xe397d798 )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "213-s1.bin", 0xb76b61bc )

	NEO_BIOS_SOUND_128K( "213-m1.bin", 0x200045f1 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "213-v1.bin", 0x000000, 0x200000, 0xa421c076 )
	ROM_LOAD( "213-v2.bin", 0x200000, 0x200000, 0x233c7dd9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "213-c1.bin", 0x400000, 0x200000, 0x62c5edc9 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "213-c2.bin", 0x400001, 0x200000, 0x9dc9c72a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
ROM_END

ROM_START( kof96 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "214-p1.bin", 0x000000, 0x100000, 0x52755d74 )
	ROM_LOAD16_WORD_SWAP( "214-p2.bin", 0x100000, 0x200000, 0x002ccb73 )

	NEO_SFIX_128K( "214-s1.bin", 0x1254cbdb )

	NEO_BIOS_SOUND_128K( "214-m1.bin", 0xdabc427c )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "214-v1.bin", 0x000000, 0x400000, 0x63f7b045 )
	ROM_LOAD( "214-v2.bin", 0x400000, 0x400000, 0x25929059 )
	ROM_LOAD( "214-v3.bin", 0x800000, 0x200000, 0x92a2257d )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "214-c1.bin", 0x0000000, 0x400000, 0x7ecf4aa2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c2.bin", 0x0000001, 0x400000, 0x05b54f37 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c3.bin", 0x0800000, 0x400000, 0x64989a65 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c4.bin", 0x0800001, 0x400000, 0xafbea515 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c5.bin", 0x1000000, 0x400000, 0x2a3bbd26 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c6.bin", 0x1000001, 0x400000, 0x44d30dc7 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c7.bin", 0x1800000, 0x400000, 0x3687331b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c8.bin", 0x1800001, 0x400000, 0xfa1461ad ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki4 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "215-p1.bin", 0x100000, 0x100000, 0x519b4ba3 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "215-s1.bin", 0xf0fe5c36 )

	NEO_BIOS_SOUND_128K( "215-m1.bin", 0xa932081d )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "215-v1.bin", 0x200000, 0x200000, 0xc4bfed62 )
	ROM_CONTINUE(			  0x000000, 0x200000 )
	ROM_LOAD( "215-v2.bin", 0x400000, 0x200000, 0x1bfa218b )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "215-c1.bin", 0x0400000, 0x200000, 0x288a9225 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "215-c2.bin", 0x0400001, 0x200000, 0x3fc9d1c4 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "215-c3.bin", 0x0c00000, 0x200000, 0xfedfaebe ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "215-c4.bin", 0x0c00001, 0x200000, 0x877a5bb2 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "215-c5.bin", 0x1000000, 0x200000, 0x0c6f97ec ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "215-c6.bin", 0x1000001, 0x200000, 0x329c5e1b ) /* Plane 2,3 */
ROM_END

ROM_START( kizuna )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "216-p1.bin", 0x100000, 0x100000, 0x75d2b3de )
	ROM_CONTINUE(					 0x000000, 0x100000 )

	NEO_SFIX_128K( "216-s1.bin",   0xefdc72d7 )

	NEO_BIOS_SOUND_128K( "216-m1.bin", 0x1b096820 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "216-v1.bin", 0x000000, 0x200000, 0x530c50fd )
	ROM_LOAD( "216-v2.bin", 0x200000, 0x200000, 0x03667a8d )
	ROM_LOAD( "216-v3.bin", 0x400000, 0x200000, 0x7038c2f9 )
	ROM_LOAD( "216-v4.bin", 0x600000, 0x200000, 0x31b99bd6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "216-c1.bin", 0x0000000, 0x200000, 0x763ba611 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c2.bin", 0x0000001, 0x200000, 0xe05e8ca6 ) /* Plane 2,3 */
	/* 400000-7fffff empty */
	ROM_LOAD16_BYTE( "216-c3.bin", 0x0800000, 0x400000, 0x665c9f16 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c4.bin", 0x0800001, 0x400000, 0x7f5d03db ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "216-c5.bin", 0x1000000, 0x200000, 0x59013f9e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c6.bin", 0x1000001, 0x200000, 0x1c8d5def ) /* Plane 2,3 */
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "216-c7.bin", 0x1800000, 0x200000, 0xc88f7035 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c8.bin", 0x1800001, 0x200000, 0x484ce3ba ) /* Plane 2,3 */
ROM_END

ROM_START( ninjamas )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "217-p1.bin", 0x000000, 0x100000, 0x3e97ed69 )
	ROM_LOAD16_WORD_SWAP( "217-p2.bin", 0x100000, 0x200000, 0x191fca88 )

	NEO_SFIX_128K( "217-s1.bin", 0x8ff782f0 )

	NEO_BIOS_SOUND_128K( "217-m1.bin", 0xd00fb2af )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "217-v1.bin", 0x000000, 0x400000, 0x1c34e013 )
	ROM_LOAD( "217-v2.bin", 0x400000, 0x200000, 0x22f1c681 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "217-c1.bin", 0x0400000, 0x200000, 0x58f91ae0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c2.bin", 0x0400001, 0x200000, 0x4258147f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "217-c3.bin", 0x0c00000, 0x200000, 0x36c29ce3 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c4.bin", 0x0c00001, 0x200000, 0x17e97a6e ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "217-c5.bin", 0x1400000, 0x200000, 0x4683ffc0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c6.bin", 0x1400001, 0x200000, 0xde004f4a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "217-c7.bin", 0x1c00000, 0x200000, 0x3e1885c0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c8.bin", 0x1c00001, 0x200000, 0x5a5df034 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( ragnagrd )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "218-p1.bin", 0x100000, 0x100000, 0xca372303 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "218-s1.bin", 0x7d402f9a )

	NEO_BIOS_SOUND_128K( "218-m1.bin", 0x17028bcf )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "218-v1.bin", 0x000000, 0x400000, 0x61eee7f4 )
	ROM_LOAD( "218-v2.bin", 0x400000, 0x400000, 0x6104e20b )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "218-c1.bin", 0x0400000, 0x200000, 0x18f61d79 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c2.bin", 0x0400001, 0x200000, 0xdbf4ff4b ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "218-c3.bin", 0x0c00000, 0x200000, 0x108d5589 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c4.bin", 0x0c00001, 0x200000, 0x7962d5ac ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "218-c5.bin", 0x1400000, 0x200000, 0x4b74021a ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c6.bin", 0x1400001, 0x200000, 0xf5cf90bc ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "218-c7.bin", 0x1c00000, 0x200000, 0x32189762 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c8.bin", 0x1c00001, 0x200000, 0xd5915828 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( pgoal )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "219-p1.bin", 0x100000, 0x100000, 0x6af0e574 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "219-s1.bin", 0x002f3c88 )

	NEO_BIOS_SOUND_128K( "219-m1.bin", 0x958efdc8 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "219-v1.bin", 0x000000, 0x200000, 0x2cc1bd05 )
	ROM_LOAD( "219-v2.bin", 0x200000, 0x200000, 0x06ac1d3f )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "219-c1.bin", 0x0000000, 0x200000, 0x2dc69faf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "219-c2.bin", 0x0000001, 0x200000, 0x5db81811 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "219-c3.bin", 0x0400000, 0x200000, 0x9dbfece5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "219-c4.bin", 0x0400001, 0x200000, 0xc9f4324c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "219-c5.bin", 0x0800000, 0x200000, 0x5fdad0a5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "219-c6.bin", 0x0800001, 0x200000, 0xf57b4a1c ) /* Plane 2,3 */
ROM_END

ROM_START( magdrop2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "221-p1.bin", 0x000000, 0x80000, 0x7be82353 )

	NEO_SFIX_128K( "221-s1.bin", 0x2a4063a3 )

	NEO_BIOS_SOUND_128K( "221-m1.bin", 0xbddae628 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "221-v1.bin", 0x000000, 0x200000, 0x7e5e53e4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "221-c1.bin", 0x000000, 0x400000, 0x1f862a14 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "221-c2.bin", 0x000001, 0x400000, 0x14b90536 ) /* Plane 2,3 */
ROM_END

ROM_START( samsho4 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "222-p1.bin", 0x000000, 0x100000, 0x1a5cb56d )
	ROM_LOAD16_WORD_SWAP( "222-p2.bin", 0x300000, 0x200000, 0x7587f09b )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "222-s1.bin", 0x8d3d3bf9 )

	NEO_BIOS_SOUND_128K( "222-m1.bin", 0x7615bc1b )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "222-v1.bin", 0x000000, 0x400000, 0x7d6ba95f )
	ROM_LOAD( "222-v2.bin", 0x400000, 0x400000, 0x6c33bb5d )
	ROM_LOAD( "222-v3.bin", 0x800000, 0x200000, 0x831ea8c0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "222-c1.bin", 0x0400000, 0x200000, 0x289100fa ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c2.bin", 0x0400001, 0x200000, 0xc2716ea0 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "222-c3.bin", 0x0c00000, 0x200000, 0x6659734f ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c4.bin", 0x0c00001, 0x200000, 0x91ebea00 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "222-c5.bin", 0x1400000, 0x200000, 0xe22254ed ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c6.bin", 0x1400001, 0x200000, 0x00947b2e ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "222-c7.bin", 0x1c00000, 0x200000, 0xe3e3b0cd ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c8.bin", 0x1c00001, 0x200000, 0xf33967f1 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( rbffspec )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "223-p1.bin", 0x000000, 0x100000, 0xf84a2d1d )
	ROM_LOAD16_WORD_SWAP( "223-p2.bin", 0x300000, 0x200000, 0x27e3e54b )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "223-s1.bin", 0x7ecd6e8c )

	NEO_BIOS_SOUND_128K( "223-m1.bin", 0x3fee46bf )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "223-v1.bin", 0x000000, 0x400000, 0x76673869 )
	ROM_LOAD( "223-v2.bin", 0x400000, 0x400000, 0x7a275acd )
	ROM_LOAD( "223-v3.bin", 0x800000, 0x400000, 0x5a797fd2 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "223-c1.bin", 0x0400000, 0x200000, 0x436edad4 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c2.bin", 0x0400001, 0x200000, 0xcc7dc384 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "223-c3.bin", 0x0c00000, 0x200000, 0x375954ea ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c4.bin", 0x0c00001, 0x200000, 0xc1a98dd7 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "223-c5.bin", 0x1400000, 0x200000, 0x12c5418e ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c6.bin", 0x1400001, 0x200000, 0xc8ad71d5 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "223-c7.bin", 0x1c00000, 0x200000, 0x5c33d1d8 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c8.bin", 0x1c00001, 0x200000, 0xefdeb140 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( twinspri )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "224-p1.bin", 0x100000, 0x100000, 0x7697e445 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "224-s1.bin", 0xeeed5758 )

	NEO_BIOS_SOUND_128K( "224-m1.bin", 0x364d6f96 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "224-v1.bin", 0x000000, 0x400000, 0xff57f088 )
	ROM_LOAD( "224-v2.bin", 0x400000, 0x200000, 0x7ad26599 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "224-c1.bin", 0x400000, 0x200000, 0x73b2a70b ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "224-c2.bin", 0x400001, 0x200000, 0x3a3e506c ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "224-c3.bin", 0x800000, 0x100000, 0xc59e4129 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "224-c4.bin", 0x800001, 0x100000, 0xb5532e53 ) /* Plane 2,3 */
ROM_END

ROM_START( wakuwak7 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "225-p1.bin", 0x000000, 0x100000, 0xb14da766 )
	ROM_LOAD16_WORD_SWAP( "225-p2.bin", 0x100000, 0x200000, 0xfe190665 )

	NEO_SFIX_128K( "225-s1.bin", 0x71c4b4b5 )

	NEO_BIOS_SOUND_128K( "225-m1.bin", 0x0634bba6 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "225-v1.bin", 0x000000, 0x400000, 0x6195c6b4 )
	ROM_LOAD( "225-v2.bin", 0x400000, 0x400000, 0x6159c5fe )

	NO_DELTAT_REGION

	ROM_REGION( 0x1800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "225-c1.bin", 0x0400000, 0x200000, 0xd91d386f ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "225-c2.bin", 0x0400001, 0x200000, 0x36b5cf41 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "225-c3.bin", 0x0c00000, 0x200000, 0x02fcff2f ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "225-c4.bin", 0x0c00001, 0x200000, 0xcd7f1241 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "225-c5.bin", 0x1400000, 0x200000, 0x03d32f25 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "225-c6.bin", 0x1400001, 0x200000, 0xd996a90a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
ROM_END

ROM_START( stakwin2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "227-p1.bin", 0x100000, 0x100000, 0xdaf101d2 )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "227-s1.bin", 0x2a8c4462 )

	NEO_BIOS_SOUND_128K( "227-m1.bin", 0xc8e5e0f9 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "227-v1.bin", 0x000000, 0x400000, 0xb8f24181 )
	ROM_LOAD( "227-v2.bin", 0x400000, 0x400000, 0xee39e260 )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "227-c1.bin", 0x0000000, 0x400000, 0x7d6c2af4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "227-c2.bin", 0x0000001, 0x400000, 0x7e402d39 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "227-c3.bin", 0x0800000, 0x200000, 0x93dfd660 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "227-c4.bin", 0x0800001, 0x200000, 0x7efea43a ) /* Plane 2,3 */
ROM_END

ROM_START( breakers )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "230-p1.bin", 0x100000, 0x100000, 0xed24a6e6 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "230-s1.bin", 0x076fb64c )

	NEO_BIOS_SOUND_128K( "230-m1.bin", 0x3951a1c1 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "230-v1.bin", 0x000000, 0x400000, 0x7f9ed279 )
	ROM_LOAD( "230-v2.bin", 0x400000, 0x400000, 0x1d43e420 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "230-c1.bin", 0x000000, 0x400000, 0x68d4ae76 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c2.bin", 0x000001, 0x400000, 0xfdee05cd ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "230-c3.bin", 0x800000, 0x400000, 0x645077f3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c4.bin", 0x800001, 0x400000, 0x63aeb74c ) /* Plane 2,3 */
ROM_END

ROM_START( miexchng )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "231-p1.bin", 0x000000, 0x80000, 0x61be1810 )

	NEO_SFIX_128K( "231-s1.bin", 0xfe0c0c53 )

	NEO_BIOS_SOUND_128K( "231-m1.bin", 0xde41301b )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "231-v1.bin", 0x000000, 0x400000, 0x113fb898 )

	NO_DELTAT_REGION

	ROM_REGION( 0x500000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "231-c1.bin", 0x000000, 0x200000, 0x6c403ba3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "231-c2.bin", 0x000001, 0x200000, 0x554bcd9b ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "231-c3.bin", 0x400000, 0x080000, 0x14524eb5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "231-c4.bin", 0x400001, 0x080000, 0x1694f171 ) /* Plane 2,3 */
ROM_END

ROM_START( kof97 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "232-p1.bin", 0x000000, 0x100000, 0x7db81ad9 )
	ROM_LOAD16_WORD_SWAP( "232-p2.bin", 0x100000, 0x400000, 0x158b23f6 )

	NEO_SFIX_128K( "232-s1.bin", 0x8514ecf5 )

	NEO_BIOS_SOUND_128K( "232-m1.bin", 0x45348747 )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "232-v1.bin", 0x000000, 0x400000, 0x22a2b5b5 )
	ROM_LOAD( "232-v2.bin", 0x400000, 0x400000, 0x2304e744 )
	ROM_LOAD( "232-v3.bin", 0x800000, 0x400000, 0x759eb954 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "232-c1.bin", 0x0000000, 0x800000, 0x5f8bf0a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c2.bin", 0x0000001, 0x800000, 0xe4d45c81 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c3.bin", 0x1000000, 0x800000, 0x581d6618 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c4.bin", 0x1000001, 0x800000, 0x49bb1e68 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c5.bin", 0x2000000, 0x400000, 0x34fc4e51 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c6.bin", 0x2000001, 0x400000, 0x4ff4d47b ) /* Plane 2,3 */
ROM_END

ROM_START( magdrop3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "233-p1.bin", 0x000000, 0x100000, 0x931e17fa )

	NEO_SFIX_128K( "233-s1.bin", 0x7399e68a )

	NEO_BIOS_SOUND_128K( "233-m1.bin", 0x5beaf34e )

	ROM_REGION( 0x480000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "233-v1.bin", 0x000000, 0x400000, 0x58839298 )
	ROM_LOAD( "233-v2.bin", 0x400000, 0x080000, 0xd5e30df4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "233-c1.bin", 0x400000, 0x200000, 0x734db3d6 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "233-c2.bin", 0x400001, 0x200000, 0xd78f50e5 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "233-c3.bin", 0xc00000, 0x200000, 0xec65f472 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "233-c4.bin", 0xc00001, 0x200000, 0xf55dddf3 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( lastblad )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "234-p1.bin", 0x000000, 0x100000, 0xcd01c06d )
	ROM_LOAD16_WORD_SWAP( "234-p2.bin", 0x100000, 0x400000, 0x0fdc289e )

	NEO_SFIX_128K( "234-s1.bin", 0x95561412 )

	NEO_BIOS_SOUND_128K( "234-m1.bin", 0x087628ea )

	ROM_REGION( 0xe00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "234-v1.bin", 0x000000, 0x400000, 0xed66b76f )
	ROM_LOAD( "234-v2.bin", 0x400000, 0x400000, 0xa0e7f6e2 )
	ROM_LOAD( "234-v3.bin", 0x800000, 0x400000, 0xa506e1e2 )
	ROM_LOAD( "234-v4.bin", 0xc00000, 0x200000, 0x13583c4b )

	NO_DELTAT_REGION

	ROM_REGION( 0x2400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "234-c1.bin", 0x0000000, 0x800000, 0x9f7e2bd3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c2.bin", 0x0000001, 0x800000, 0x80623d3c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "234-c3.bin", 0x1000000, 0x800000, 0x91ab1a30 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c4.bin", 0x1000001, 0x800000, 0x3d60b037 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "234-c5.bin", 0x2000000, 0x200000, 0x17bbd7ca ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c6.bin", 0x2000001, 0x200000, 0x5c35d541 ) /* Plane 2,3 */
ROM_END

ROM_START( puzzldpr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "235-p1.bin", 0x000000, 0x080000, 0xafed5de2 )

	NEO_SFIX_64K( "235-s1.bin", 0x5a68d91e )

	NEO_BIOS_SOUND_128K( "202-m1.bin", 0x9c0291ea )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "202-v1.bin", 0x000000, 0x080000, 0xdebeb8fb )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "202-c1.bin", 0x000000, 0x100000, 0xcc0095ef ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "202-c2.bin", 0x000001, 0x100000, 0x42371307 ) /* Plane 2,3 */
ROM_END

ROM_START( irrmaze )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "236-p1.bin", 0x000000, 0x200000, 0x6d536c6e )

	NEO_SFIX_128K( "236-s1.bin", 0x5d1ca640 )

	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 )
	/* special BIOS with trackball support */
	ROM_LOAD16_WORD_SWAP( "236-bios.bin", 0x00000, 0x020000, 0x853e6b96 )
	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */
	ROM_LOAD( "236-m1.bin", 0x00000, 0x20000, 0x880a1abd )  /* so overwrite it with the real thing */
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "ng-lo.rom", 0x00000, 0x10000, 0xe09e253c )  /* Y zoom control */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "236-v1.bin", 0x000000, 0x200000, 0x5f89c3b4 )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "236-v2.bin", 0x000000, 0x100000, 0x1e843567 )

	ROM_REGION( 0x0800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "236-c1.bin", 0x000000, 0x400000, 0xc1d47902 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "236-c2.bin", 0x000001, 0x400000, 0xe15f972e ) /* Plane 2,3 */
ROM_END

ROM_START( popbounc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "237-p1.bin", 0x000000, 0x100000, 0xbe96e44f )

	NEO_SFIX_128K( "237-s1.bin", 0xb61cf595 )

	NEO_BIOS_SOUND_128K( "237-m1.bin", 0xd4c946dd )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "237-v1.bin", 0x000000, 0x200000, 0xedcb1beb )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "237-c1.bin", 0x000000, 0x200000, 0xeda42d66 )
	ROM_LOAD16_BYTE( "237-c2.bin", 0x000001, 0x200000, 0x5e633c65 )
ROM_END

ROM_START( shocktro )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "238-p1.bin", 0x000000, 0x100000, 0x5677456f )
	ROM_LOAD16_WORD_SWAP( "238-p2.bin", 0x300000, 0x200000, 0x646f6c76 )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "238-s1.bin", 0x1f95cedb )

	NEO_BIOS_SOUND_128K( "238-m1.bin", 0x075b9518 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "238-v1.bin", 0x000000, 0x400000, 0x260c0bef )
	ROM_LOAD( "238-v2.bin", 0x400000, 0x200000, 0x4ad7d59e )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "238-c1.bin", 0x0400000, 0x200000, 0xaad087fc ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c2.bin", 0x0400001, 0x200000, 0x7e39df1f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c3.bin", 0x0c00000, 0x200000, 0x6682a458 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c4.bin", 0x0c00001, 0x200000, 0xcbef1f17 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c5.bin", 0x1400000, 0x200000, 0xe17762b1 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c6.bin", 0x1400001, 0x200000, 0x28beab71 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c7.bin", 0x1c00000, 0x200000, 0xa47e62d2 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c8.bin", 0x1c00001, 0x200000, 0xe8e890fb ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( shocktrj )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "238-pg1.p1", 0x000000, 0x100000, 0xefedf8dc )
	ROM_LOAD16_WORD_SWAP( "238-p2.bin", 0x300000, 0x200000, 0x646f6c76 )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "238-s1.bin", 0x1f95cedb )

	NEO_BIOS_SOUND_128K( "238-m1.bin", 0x075b9518 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "238-v1.bin", 0x000000, 0x400000, 0x260c0bef )
	ROM_LOAD( "238-v2.bin", 0x400000, 0x200000, 0x4ad7d59e )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "238-c1.bin", 0x0400000, 0x200000, 0xaad087fc ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c2.bin", 0x0400001, 0x200000, 0x7e39df1f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c3.bin", 0x0c00000, 0x200000, 0x6682a458 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c4.bin", 0x0c00001, 0x200000, 0xcbef1f17 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c5.bin", 0x1400000, 0x200000, 0xe17762b1 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c6.bin", 0x1400001, 0x200000, 0x28beab71 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c7.bin", 0x1c00000, 0x200000, 0xa47e62d2 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c8.bin", 0x1c00001, 0x200000, 0xe8e890fb ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( blazstar )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "239-p1.bin", 0x000000, 0x100000, 0x183682f8 )
	ROM_LOAD16_WORD_SWAP( "239-p2.bin", 0x100000, 0x200000, 0x9a9f4154 )

	NEO_SFIX_128K( "239-s1.bin", 0xd56cb498 )

	NEO_BIOS_SOUND_128K( "239-m1.bin", 0xd31a3aea )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "239-v1.bin", 0x000000, 0x400000, 0x1b8d5bf7 )
	ROM_LOAD( "239-v2.bin", 0x400000, 0x400000, 0x74cf0a70 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "239-c1.bin", 0x0400000, 0x200000, 0x754744e0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c2.bin", 0x0400001, 0x200000, 0xaf98c037 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "239-c3.bin", 0x0c00000, 0x200000, 0x7b39b590 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c4.bin", 0x0c00001, 0x200000, 0x6e731b30 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "239-c5.bin", 0x1400000, 0x200000, 0x9ceb113b ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c6.bin", 0x1400001, 0x200000, 0x6a78e810 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "239-c7.bin", 0x1c00000, 0x200000, 0x50d28eca ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c8.bin", 0x1c00001, 0x200000, 0xcdbbb7d7 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( rbff2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "240-p1.bin", 0x000000, 0x100000, 0xb6969780 )
	ROM_LOAD16_WORD_SWAP( "240-p2.bin", 0x100000, 0x400000, 0x960aa88d )

	NEO_SFIX_128K( "240-s1.bin",  0xda3b40de )

	NEO_BIOS_SOUND_256K( "240-m1.bin", 0xed482791 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "240-v1.bin", 0x000000, 0x400000, 0xf796265a )
	ROM_LOAD( "240-v2.bin", 0x400000, 0x400000, 0x2cb3f3bb )
	ROM_LOAD( "240-v3.bin", 0x800000, 0x400000, 0xdf77b7fa )
	ROM_LOAD( "240-v4.bin", 0xc00000, 0x400000, 0x33a356ee )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "240-c1.bin", 0x0000000, 0x800000, 0xeffac504 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c2.bin", 0x0000001, 0x800000, 0xed182d44 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "240-c3.bin", 0x1000000, 0x800000, 0x22e0330a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c4.bin", 0x1000001, 0x800000, 0xc19a07eb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "240-c5.bin", 0x2000000, 0x800000, 0x244dff5a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c6.bin", 0x2000001, 0x800000, 0x4609e507 ) /* Plane 2,3 */
ROM_END

ROM_START( mslug2 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "241-p1.bin", 0x000000, 0x100000, 0x2a53c5da )
	ROM_LOAD16_WORD_SWAP( "241-p2.bin", 0x100000, 0x200000, 0x38883f44 )

	NEO_SFIX_128K( "241-s1.bin",  0xf3d32f0f )

	NEO_BIOS_SOUND_128K( "241-m1.bin", 0x94520ebd )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "241-v1.bin", 0x000000, 0x400000, 0x99ec20e8 )
	ROM_LOAD( "241-v2.bin", 0x400000, 0x400000, 0xecb16799 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "241-c1.bin", 0x0000000, 0x800000, 0x394b5e0d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "241-c2.bin", 0x0000001, 0x800000, 0xe5806221 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "241-c3.bin", 0x1000000, 0x800000, 0x9f6bfa6f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "241-c4.bin", 0x1000001, 0x800000, 0x7d3e306f ) /* Plane 2,3 */
ROM_END

ROM_START( kof98 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "242-p1.bin", 0x000000, 0x100000, 0x61ac868a )
	ROM_LOAD16_WORD_SWAP( "242-p2.bin", 0x100000, 0x400000, 0x980aba4c )

	NEO_SFIX_128K( "242-s1.bin", 0x7f7b4805 )

	NEO_BIOS_SOUND_256K( "242-m1.bin", 0x4e7a6b1b )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "242-v1.bin", 0x000000, 0x400000, 0xb9ea8051 )
	ROM_LOAD( "242-v2.bin", 0x400000, 0x400000, 0xcc11106e )
	ROM_LOAD( "242-v3.bin", 0x800000, 0x400000, 0x044ea4e1 )
	ROM_LOAD( "242-v4.bin", 0xc00000, 0x400000, 0x7985ea30 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "242-c1.bin", 0x0000000, 0x800000, 0xe564ecd6 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c2.bin", 0x0000001, 0x800000, 0xbd959b60 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c3.bin", 0x1000000, 0x800000, 0x22127b4f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c4.bin", 0x1000001, 0x800000, 0x0b4fa044 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c5.bin", 0x2000000, 0x800000, 0x9d10bed3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c6.bin", 0x2000001, 0x800000, 0xda07b6a2 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c7.bin", 0x3000000, 0x800000, 0xf6d7a38a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c8.bin", 0x3000001, 0x800000, 0xc823e045 ) /* Plane 2,3 */
ROM_END

ROM_START( lastbld2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "243-p1.bin", 0x000000, 0x100000, 0xaf1e6554 )
	ROM_LOAD16_WORD_SWAP( "243-p2.bin", 0x100000, 0x400000, 0xadd4a30b )

	NEO_SFIX_128K( "243-s1.bin", 0xc9cd2298 )

	NEO_BIOS_SOUND_128K( "243-m1.bin", 0xacf12d10 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "243-v1.bin", 0x000000, 0x400000, 0xf7ee6fbb )
	ROM_LOAD( "243-v2.bin", 0x400000, 0x400000, 0xaa9e4df6 )
	ROM_LOAD( "243-v3.bin", 0x800000, 0x400000, 0x4ac750b2 )
	ROM_LOAD( "243-v4.bin", 0xc00000, 0x400000, 0xf5c64ba6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "243-c1.bin",  0x0000000, 0x800000, 0x5839444d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "243-c2.bin",  0x0000001, 0x800000, 0xdd087428 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "243-c3.bin",  0x1000000, 0x800000, 0x6054cbe0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "243-c4.bin",  0x1000001, 0x800000, 0x8bd2a9d2 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "243-c5.bin",  0x2000000, 0x800000, 0x6a503dcf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "243-c6.bin",  0x2000001, 0x800000, 0xec9c36d0 ) /* Plane 2,3 */
ROM_END

ROM_START( neocup98 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "244-p1.bin", 0x100000, 0x100000, 0xf8fdb7a5 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "244-s1.bin", 0x9bddb697 )

	NEO_BIOS_SOUND_128K( "244-m1.bin", 0xa701b276 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "244-v1.bin", 0x000000, 0x400000, 0x79def46d )
	ROM_LOAD( "244-v2.bin", 0x400000, 0x200000, 0xb231902f )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "244-c1.bin", 0x000000, 0x800000, 0xd2c40ec7 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "244-c2.bin", 0x000001, 0x800000, 0x33aa0f35 ) /* Plane 2,3 */
ROM_END

ROM_START( breakrev )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "245-p1.bin", 0x100000, 0x100000, 0xc828876d )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "245-s1.bin", 0xe7660a5d )

	NEO_BIOS_SOUND_128K( "245-m1.bin", 0x00f31c66 )

	ROM_REGION(  0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "245-v1.bin", 0x000000, 0x400000, 0xe255446c )
	ROM_LOAD( "245-v2.bin", 0x400000, 0x400000, 0x9068198a )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "230-c1.bin", 0x0000000, 0x400000, 0x68d4ae76 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c2.bin", 0x0000001, 0x400000, 0xfdee05cd ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "230-c3.bin", 0x0800000, 0x400000, 0x645077f3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c4.bin", 0x0800001, 0x400000, 0x63aeb74c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "245-c5.bin",  0x1000000, 0x200000, 0x28ff1792 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "245-c6.bin",  0x1000001, 0x200000, 0x23c65644 ) /* Plane 2,3 */
ROM_END

ROM_START( shocktr2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "246-p1.bin", 0x000000, 0x100000, 0x6d4b7781 )
	ROM_LOAD16_WORD_SWAP( "246-p2.bin", 0x100000, 0x400000, 0x72ea04c3 )

	NEO_SFIX_128K( "246-s1.bin", 0x2a360637 )

	NEO_BIOS_SOUND_128K( "246-m1.bin", 0xd0604ad1 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "246-v1.bin", 0x000000, 0x400000, 0x16986fc6 )
	ROM_LOAD( "246-v2.bin", 0x400000, 0x400000, 0xada41e83 )
	ROM_LOAD( "246-v3.bin", 0x800000, 0x200000, 0xa05ba5db )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "246-c1.bin", 0x0000000, 0x800000, 0x47ac9ec5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "246-c2.bin", 0x0000001, 0x800000, 0x7bcab64f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "246-c3.bin", 0x1000000, 0x800000, 0xdb2f73e8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "246-c4.bin", 0x1000001, 0x800000, 0x5503854e ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "246-c5.bin", 0x2000000, 0x800000, 0x055b3701 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "246-c6.bin", 0x2000001, 0x800000, 0x7e2caae1 ) /* Plane 2,3 */
ROM_END

ROM_START( flipshot )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "247-p1.bin", 0x000000, 0x080000, 0xd2e7a7e3 )

	NEO_SFIX_128K( "247-s1.bin", 0x6300185c )

	NEO_BIOS_SOUND_128K( "247-m1.bin", 0xa9fe0144 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "247-v1.bin", 0x000000, 0x200000, 0x42ec743d )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "247-c1.bin",  0x000000, 0x200000, 0xc9eedcb2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "247-c2.bin",  0x000001, 0x200000, 0x7d6d6e87 ) /* Plane 2,3 */
ROM_END

ROM_START( pbobbl2n )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "248-p1.bin", 0x000000, 0x100000, 0x9d6c0754 )

	NEO_SFIX_128K( "248-s1.bin", 0x0a3fee41 )

	NEO_BIOS_SOUND_128K( "248-m1.bin", 0x883097a9 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "248-v1.bin", 0x000000, 0x400000, 0x57fde1fa )
	ROM_LOAD( "248-v2.bin", 0x400000, 0x400000, 0x4b966ef3 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "248-c1.bin", 0x000000, 0x400000, 0xd9115327 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "248-c2.bin", 0x000001, 0x400000, 0x77f9fdac ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "248-c3.bin", 0x800000, 0x100000, 0x8890bf7c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "248-c4.bin", 0x800001, 0x100000, 0x8efead3f ) /* Plane 2,3 */
ROM_END

ROM_START( ctomaday )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "249-p1.bin", 0x100000, 0x100000, 0xc9386118 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "249-s1.bin", 0xdc9eb372 )

	NEO_BIOS_SOUND_128K( "249-m1.bin", 0x80328a47 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "249-v1.bin", 0x000000, 0x400000, 0xde7c8f27 )
	ROM_LOAD( "249-v2.bin", 0x400000, 0x100000, 0xc8e40119 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "249-c1.bin",  0x000000, 0x400000, 0x041fb8ee ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "249-c2.bin",  0x000001, 0x400000, 0x74f3cdf4 ) /* Plane 2,3 */
ROM_END

ROM_START( mslugx )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "250-p1.bin", 0x000000, 0x100000, 0x81f1f60b )
	ROM_LOAD16_WORD_SWAP( "250-p2.bin", 0x100000, 0x400000, 0x1fda2e12 )

	NEO_SFIX_128K( "250-s1.bin",  0xfb6f441d )

	NEO_BIOS_SOUND_128K( "250-m1.bin", 0xfd42a842 )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "250-v1.bin", 0x000000, 0x400000, 0xc79ede73 )
	ROM_LOAD( "250-v2.bin", 0x400000, 0x400000, 0xea9aabe1 )
	ROM_LOAD( "250-v3.bin", 0x800000, 0x200000, 0x2ca65102 )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "250-c1.bin", 0x0000000, 0x800000, 0x09a52c6f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "250-c2.bin", 0x0000001, 0x800000, 0x31679821 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "250-c3.bin", 0x1000000, 0x800000, 0xfd602019 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "250-c4.bin", 0x1000001, 0x800000, 0x31354513 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "250-c5.bin", 0x2000000, 0x800000, 0xa4b56124 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "250-c6.bin", 0x2000001, 0x800000, 0x83e3e69d ) /* Plane 0,1 */
ROM_END

ROM_START( kof99 ) /* Original Version - Encrypted Code & GFX */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "251-sma.bin", 0x0c0000, 0x040000, 0x7766d09e )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "251-p1.bin",  0x100000, 0x400000, 0x006e4532 )
	ROM_LOAD16_WORD_SWAP( "251-p2.bin",  0x500000, 0x400000, 0x90175f15 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "251-m1.bin", 0x5e74539c )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, 0xef2eecc8 )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, 0x73e211ca )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, 0x821901da )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, 0xb49e6178 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "251-c1.bin",   0x0000000, 0x800000, 0x0f9e93fe ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2.bin",   0x0000001, 0x800000, 0xe71e2ea3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3.bin",   0x1000000, 0x800000, 0x238755d2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4.bin",   0x1000001, 0x800000, 0x438c8b22 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5.bin",   0x2000000, 0x800000, 0x0b0abd0a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6.bin",   0x2000001, 0x800000, 0x65bbf281 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7.bin",   0x3000000, 0x800000, 0xff65f62e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8.bin",   0x3000001, 0x800000, 0x8d921c68 ) /* Plane 2,3 */
ROM_END

ROM_START( kof99e ) /* Original Version - Encrypted Code & GFX */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "251-sma.bin", 0x0c0000, 0x040000, 0x7766d09e )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "251-ep1.p1",  0x100000, 0x200000, 0x1e8d692d )
	ROM_LOAD16_WORD_SWAP( "251-ep2.p2",  0x300000, 0x200000, 0xd6206e5a )
	ROM_LOAD16_WORD_SWAP( "251-ep3.p3",  0x500000, 0x200000, 0xd58c3ef8 )
	ROM_LOAD16_WORD_SWAP( "251-ep4.p4",  0x700000, 0x200000, 0x52de02ae )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "251-m1.bin", 0x5e74539c )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, 0xef2eecc8 )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, 0x73e211ca )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, 0x821901da )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, 0xb49e6178 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "251-c1.bin",   0x0000000, 0x800000, 0x0f9e93fe ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2.bin",   0x0000001, 0x800000, 0xe71e2ea3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3.bin",   0x1000000, 0x800000, 0x238755d2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4.bin",   0x1000001, 0x800000, 0x438c8b22 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5.bin",   0x2000000, 0x800000, 0x0b0abd0a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6.bin",   0x2000001, 0x800000, 0x65bbf281 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7.bin",   0x3000000, 0x800000, 0xff65f62e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8.bin",   0x3000001, 0x800000, 0x8d921c68 ) /* Plane 2,3 */
ROM_END

ROM_START( kof99n ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "152-p1.bin", 0x000000, 0x100000, 0xf2c7ddfa )
	ROM_LOAD16_WORD_SWAP( "152-p2.bin", 0x100000, 0x400000, 0x274ef47a )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "251-m1.bin", 0x5e74539c )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, 0xef2eecc8 )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, 0x73e211ca )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, 0x821901da )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, 0xb49e6178 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "251-c1.bin",   0x0000000, 0x800000, 0x0f9e93fe ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2.bin",   0x0000001, 0x800000, 0xe71e2ea3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3.bin",   0x1000000, 0x800000, 0x238755d2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4.bin",   0x1000001, 0x800000, 0x438c8b22 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5.bin",   0x2000000, 0x800000, 0x0b0abd0a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6.bin",   0x2000001, 0x800000, 0x65bbf281 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7.bin",   0x3000000, 0x800000, 0xff65f62e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8.bin",   0x3000001, 0x800000, 0x8d921c68 ) /* Plane 2,3 */
ROM_END

ROM_START( kof99p ) /* Prototype Version - Possibly Hacked */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "251-p1p.bin", 0x000000, 0x100000, 0xf37929c4 )
	ROM_LOAD16_WORD_SWAP( "251-p2p.bin", 0x100000, 0x400000, 0x739742ad )

	/* This is the S1 from the prototype, the final is certainly be different */
	NEO_SFIX_128K( "251-s1p.bin", 0xfb1498ed )

	/* Did the Prototype really use the same sound program / voice roms, sound isn't great .. */
	NEO_BIOS_SOUND_128K( "251-m1.bin", 0x5e74539c )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, 0xef2eecc8 )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, 0x73e211ca )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, 0x821901da )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, 0xb49e6178 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* these are probably decrypted versions of the roms found in the final */
	ROM_LOAD16_BYTE( "251-c1p.bin", 0x0000000, 0x800000, 0xe5d8ffa4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2p.bin", 0x0000001, 0x800000, 0xd822778f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3p.bin", 0x1000000, 0x800000, 0xf20959e8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4p.bin", 0x1000001, 0x800000, 0x54ffbe9f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5p.bin", 0x2000000, 0x800000, 0xd87a3bbc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6p.bin", 0x2000001, 0x800000, 0x4d40a691 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7p.bin", 0x3000000, 0x800000, 0xa4479a58 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8p.bin", 0x3000001, 0x800000, 0xead513ce ) /* Plane 2,3 */
ROM_END

ROM_START( ganryu ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "252-p1.bin", 0x100000, 0x100000, 0x4b8ac4fb )
	ROM_CONTINUE(						0x000000, 0x100000 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "252-m1.bin", 0x30cc4099 )

	ROM_REGION( 0x0400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "252-v1.bin", 0x000000, 0x400000, 0xe5946733 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "252-c1.bin", 0x0000000, 0x800000, 0x50ee7882 )
	ROM_LOAD16_BYTE( "252-c2.bin", 0x0000001, 0x800000, 0x62585474 )
ROM_END

ROM_START( garou )
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "253-sma.bin", 0x0c0000, 0x040000, 0x98bc93dc )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "253-ep1.p1",  0x100000, 0x200000, 0xea3171a4 )
	ROM_LOAD16_WORD_SWAP( "253-ep2.p2",  0x300000, 0x200000, 0x382f704b )
	ROM_LOAD16_WORD_SWAP( "253-ep3.p3",  0x500000, 0x200000, 0xe395bfdd )
	ROM_LOAD16_WORD_SWAP( "253-ep4.p4",  0x700000, 0x200000, 0xda92c08e )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_256K( "253-m1.bin", 0x36a806be )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "253-v1.bin", 0x000000, 0x400000, 0x263e388c )
	ROM_LOAD( "253-v2.bin", 0x400000, 0x400000, 0x2c6bc7be )
	ROM_LOAD( "253-v3.bin", 0x800000, 0x400000, 0x0425b27d )
	ROM_LOAD( "253-v4.bin", 0xc00000, 0x400000, 0xa54be8a9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "253-c1.bin", 0x0000000, 0x800000, 0x0603e046 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c2.bin", 0x0000001, 0x800000, 0x0917d2a4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c3.bin", 0x1000000, 0x800000, 0x6737c92d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c4.bin", 0x1000001, 0x800000, 0x5ba92ec6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c5.bin", 0x2000000, 0x800000, 0x3eab5557 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c6.bin", 0x2000001, 0x800000, 0x308d098b ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c7.bin", 0x3000000, 0x800000, 0xc0e995ae ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c8.bin", 0x3000001, 0x800000, 0x21a11303 ) /* Plane 2,3 */
ROM_END

ROM_START( garouo )
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "253-smao.bin", 0x0c0000, 0x040000, 0x96c72233 )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "253-p1.bin",   0x100000, 0x400000, 0x18ae5d7e )
	ROM_LOAD16_WORD_SWAP( "253-p2.bin",   0x500000, 0x400000, 0xafffa779 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_256K( "253-m1.bin", 0x36a806be )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "253-v1.bin", 0x000000, 0x400000, 0x263e388c )
	ROM_LOAD( "253-v2.bin", 0x400000, 0x400000, 0x2c6bc7be )
	ROM_LOAD( "253-v3.bin", 0x800000, 0x400000, 0x0425b27d )
	ROM_LOAD( "253-v4.bin", 0xc00000, 0x400000, 0xa54be8a9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "253-c1.bin", 0x0000000, 0x800000, 0x0603e046 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c2.bin", 0x0000001, 0x800000, 0x0917d2a4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c3.bin", 0x1000000, 0x800000, 0x6737c92d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c4.bin", 0x1000001, 0x800000, 0x5ba92ec6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c5.bin", 0x2000000, 0x800000, 0x3eab5557 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c6.bin", 0x2000001, 0x800000, 0x308d098b ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c7.bin", 0x3000000, 0x800000, 0xc0e995ae ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c8.bin", 0x3000001, 0x800000, 0x21a11303 ) /* Plane 2,3 */
ROM_END

ROM_START( garoup ) /* Prototype Version, seems genuine */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "253-p1p.bin", 0x000000, 0x100000, 0xc72f0c16 )
	ROM_LOAD16_WORD_SWAP( "253-p2p.bin", 0x100000, 0x400000, 0xbf8de565 )

	NEO_SFIX_128K( "253-s1p.bin", 0x779989de )

	NEO_BIOS_SOUND_256K( "253-m1p.bin", 0xbbe464f7 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "253-v1p.bin", 0x000000, 0x400000, 0x274f3923 )
	ROM_LOAD( "253-v2p.bin", 0x400000, 0x400000, 0x8f86dabe )
	ROM_LOAD( "253-v3p.bin", 0x800000, 0x400000, 0x05fd06cd )
	ROM_LOAD( "253-v4p.bin", 0xc00000, 0x400000, 0x14984063 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "253-c1p.bin", 0x0000000, 0x800000, 0x5bb5d137 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c2p.bin", 0x0000001, 0x800000, 0x5c8d2960 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c3p.bin", 0x1000000, 0x800000, 0x234d16fc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c4p.bin", 0x1000001, 0x800000, 0xb9b5b993 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c5p.bin", 0x2000000, 0x800000, 0x722615d2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c6p.bin", 0x2000001, 0x800000, 0x0a6fab38 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c7p.bin", 0x3000000, 0x800000, 0xd68e806f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c8p.bin", 0x3000001, 0x800000, 0xf778fe99 ) /* Plane 2,3 */
ROM_END

ROM_START( s1945p ) /* Original Version, Encrypted GFX Roms */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "254-p1.bin", 0x000000, 0x100000, 0xff8efcff )
	ROM_LOAD16_WORD_SWAP( "254-p2.bin", 0x100000, 0x400000, 0xefdfd4dd )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "254-m1.bin", 0x994b4487 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "254-v1.bin", 0x000000, 0x400000, 0x844f58fb )
	ROM_LOAD( "254-v2.bin", 0x400000, 0x400000, 0xd9a248f0 )
	ROM_LOAD( "254-v3.bin", 0x800000, 0x400000, 0x0b0d2d33 )
	ROM_LOAD( "254-v4.bin", 0xc00000, 0x400000, 0x6d13dc91 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "254-c1.bin", 0x0000000, 0x800000, 0xae6fc8ef ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c2.bin", 0x0000001, 0x800000, 0x436fa176 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "254-c3.bin", 0x1000000, 0x800000, 0xe53ff2dc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c4.bin", 0x1000001, 0x800000, 0x818672f0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "254-c5.bin", 0x2000000, 0x800000, 0x4580eacd ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c6.bin", 0x2000001, 0x800000, 0xe34970fc ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "254-c7.bin", 0x3000000, 0x800000, 0xf2323239 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c8.bin", 0x3000001, 0x800000, 0x66848c7d ) /* Plane 2,3 */
ROM_END

ROM_START( preisle2 ) /* Original Version, Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "255-p1.bin", 0x000000, 0x100000, 0xdfa3c0f3 )
	ROM_LOAD16_WORD_SWAP( "255-p2.bin", 0x100000, 0x400000, 0x42050b80 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "255-m1.bin", 0x8efd4014 )

	ROM_REGION( 0x0600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "255-v1.bin", 0x000000, 0x400000, 0x5a14543d )
	ROM_LOAD( "255-v2.bin", 0x400000, 0x200000, 0x6610d91a )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "255-c1.bin",   0x0000000, 0x800000, 0xea06000b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "255-c2.bin",   0x0000001, 0x800000, 0x04e67d79 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "255-c3.bin",   0x1000000, 0x800000, 0x60e31e08 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "255-c4.bin",   0x1000001, 0x800000, 0x40371d69 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "255-c5.bin",   0x2000000, 0x800000, 0x0b2e6adf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "255-c6.bin",   0x2000001, 0x800000, 0xb001bdd3 ) /* Plane 2,3 */
ROM_END

ROM_START( mslug3 ) /* Original Version - Encrypted Code & GFX */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "256-sma.bin", 0x0c0000, 0x040000, 0x9cd55736 )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "256-p1.bin",  0x100000, 0x400000, 0xb07edfd5 )
	ROM_LOAD16_WORD_SWAP( "256-p2.bin",  0x500000, 0x400000, 0x6097c26b )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_512K( "256-m1.bin", 0xeaeec116 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "256-v1.bin", 0x000000, 0x400000, 0xf2690241 )
	ROM_LOAD( "256-v2.bin", 0x400000, 0x400000, 0x7e2a10bd )
	ROM_LOAD( "256-v3.bin", 0x800000, 0x400000, 0x0eaec17c )
	ROM_LOAD( "256-v4.bin", 0xc00000, 0x400000, 0x9b4b22d4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "256-c1.bin",   0x0000000, 0x800000, 0x5a79c34e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c2.bin",   0x0000001, 0x800000, 0x944c362c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c3.bin",   0x1000000, 0x800000, 0x6e69d36f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c4.bin",   0x1000001, 0x800000, 0xb755b4eb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c5.bin",   0x2000000, 0x800000, 0x7aacab47 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c6.bin",   0x2000001, 0x800000, 0xc698fd5d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c7.bin",   0x3000000, 0x800000, 0xcfceddd2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c8.bin",   0x3000001, 0x800000, 0x4d9be34c ) /* Plane 2,3 */
ROM_END

ROM_START( mslug3n ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "256-ph1.rom",  0x000000, 0x100000, 0x9c42ca85 )
	ROM_LOAD16_WORD_SWAP( "256-ph2.rom",  0x100000, 0x400000, 0x1f3d8ce8 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_512K( "256-m1.bin", 0xeaeec116 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "256-v1.bin", 0x000000, 0x400000, 0xf2690241 )
	ROM_LOAD( "256-v2.bin", 0x400000, 0x400000, 0x7e2a10bd )
	ROM_LOAD( "256-v3.bin", 0x800000, 0x400000, 0x0eaec17c )
	ROM_LOAD( "256-v4.bin", 0xc00000, 0x400000, 0x9b4b22d4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "256-c1.bin",   0x0000000, 0x800000, 0x5a79c34e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c2.bin",   0x0000001, 0x800000, 0x944c362c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c3.bin",   0x1000000, 0x800000, 0x6e69d36f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c4.bin",   0x1000001, 0x800000, 0xb755b4eb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c5.bin",   0x2000000, 0x800000, 0x7aacab47 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c6.bin",   0x2000001, 0x800000, 0xc698fd5d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c7.bin",   0x3000000, 0x800000, 0xcfceddd2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c8.bin",   0x3000001, 0x800000, 0x4d9be34c ) /* Plane 2,3 */
ROM_END

ROM_START( kof2000 ) /* Original Version, Encrypted Code + Sound + GFX Roms */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "257-sma.bin", 0x0c0000, 0x040000, 0x71c6e6bb )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "257-p1.bin",  0x100000, 0x400000, 0x60947b4c )
	ROM_LOAD16_WORD_SWAP( "257-p2.bin",  0x500000, 0x400000, 0x1b7ec415 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	/* The M1 ROM is encrypted, we load it here for reference and replace it with a decrypted version */
	ROM_REGION( 0x40000, REGION_USER4, 0 )
	ROM_LOAD( "257-m1.bin", 0x00000, 0x40000, 0x4b749113 )
	NEO_BIOS_SOUND_256K( "257-m1d.bin", 0xd404db70 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "257-v1.bin", 0x000000, 0x400000, 0x17cde847 )
	ROM_LOAD( "257-v2.bin", 0x400000, 0x400000, 0x1afb20ff )
	ROM_LOAD( "257-v3.bin", 0x800000, 0x400000, 0x4605036a )
	ROM_LOAD( "257-v4.bin", 0xc00000, 0x400000, 0x764bbd6b )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "257-c1.bin",   0x0000000, 0x800000, 0xcef1cdfa ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c2.bin",   0x0000001, 0x800000, 0xf7bf0003 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c3.bin",   0x1000000, 0x800000, 0x101e6560 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c4.bin",   0x1000001, 0x800000, 0xbd2fc1b1 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c5.bin",   0x2000000, 0x800000, 0x89775412 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c6.bin",   0x2000001, 0x800000, 0xfa7200d5 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c7.bin",   0x3000000, 0x800000, 0x7da11fe4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c8.bin",   0x3000001, 0x800000, 0xb1afa60b ) /* Plane 2,3 */
ROM_END

ROM_START( kof2000n ) /* Original Version, Encrypted Sound + GFX Roms */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "257-p1n.bin",  0x000000, 0x100000, 0x5f809dbe )
	ROM_LOAD16_WORD_SWAP( "257-p2n.bin",  0x100000, 0x400000, 0x693c2c5e )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	/* The M1 ROM is encrypted, we load it here for reference and replace it with a decrypted version */
	ROM_REGION( 0x40000, REGION_USER4, 0 )
	ROM_LOAD( "257-m1.bin", 0x00000, 0x40000, 0x4b749113 )
	NEO_BIOS_SOUND_256K( "257-m1d.bin", 0xd404db70 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "257-v1.bin", 0x000000, 0x400000, 0x17cde847 )
	ROM_LOAD( "257-v2.bin", 0x400000, 0x400000, 0x1afb20ff )
	ROM_LOAD( "257-v3.bin", 0x800000, 0x400000, 0x4605036a )
	ROM_LOAD( "257-v4.bin", 0xc00000, 0x400000, 0x764bbd6b )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "257-c1.bin",   0x0000000, 0x800000, 0xcef1cdfa ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c2.bin",   0x0000001, 0x800000, 0xf7bf0003 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c3.bin",   0x1000000, 0x800000, 0x101e6560 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c4.bin",   0x1000001, 0x800000, 0xbd2fc1b1 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c5.bin",   0x2000000, 0x800000, 0x89775412 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c6.bin",   0x2000001, 0x800000, 0xfa7200d5 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c7.bin",   0x3000000, 0x800000, 0x7da11fe4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c8.bin",   0x3000001, 0x800000, 0xb1afa60b ) /* Plane 2,3 */
ROM_END

ROM_START( bangbead )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "259-p1.bin", 0x100000, 0x100000, 0x88a37f8b )
	ROM_CONTINUE(                       0x000000, 0x100000 )

	NEO_SFIX_128K( "259-s1.bin", 0xbb50fb2d )

	NEO_BIOS_SOUND_128K( "259-m1.bin", 0x85668ee9 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "259-v1.bin", 0x000000, 0x200000, 0xe97b9385 )
	ROM_LOAD( "259-v2.bin", 0x200000, 0x200000, 0xb0cbd70a )
	ROM_LOAD( "259-v3.bin", 0x400000, 0x100000, 0x97528fe9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "259-c1.bin", 0x000000, 0x200000, 0xe3919e44 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "259-c2.bin", 0x000001, 0x200000, 0xbaf5a320 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "259-c3.bin", 0x400000, 0x100000, 0xc8e52157 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "259-c4.bin", 0x400001, 0x100000, 0x69fa8e60 ) /* Plane 2,3 */
ROM_END

ROM_START( nitd ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "260-p1.bin", 0x000000, 0x080000, 0x61361082 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_512K( "260-m1.bin", 0x6407c5e5 )

	ROM_REGION( 0x0400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "260-v1.bin", 0x000000, 0x400000, 0x24b0480c )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "260-c1.bin", 0x0000000, 0x800000, 0x147b0c7f )
	ROM_LOAD16_BYTE( "260-c2.bin", 0x0000001, 0x800000, 0xd2b04b0d )
ROM_END

ROM_START( zupapa ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "070-p1.bin", 0x000000, 0x100000, 0x5a96203e )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "070-m1.bin", 0x5a3b3191 )

	ROM_REGION( 0x0200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "070-v1.bin", 0x000000, 0x200000, 0xd3a7e1ff )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "070-c1.bin", 0x0000000, 0x800000, 0xf8ad02d8 )
	ROM_LOAD16_BYTE( "070-c2.bin", 0x0000001, 0x800000, 0x70156dde )
ROM_END

ROM_START( sengoku3 ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "261-p1.bin", 0x100000, 0x100000, 0x5b557201 )
	ROM_CONTINUE(                       0x000000, 0x100000 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "261-m1.bin", 0x36ed9cdd )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "261-v1.bin", 0x000000, 0x400000, 0x64c30081 )
	ROM_LOAD( "261-v2.bin", 0x400000, 0x400000, 0x392a9c47 )
	ROM_LOAD( "261-v3.bin", 0x800000, 0x400000, 0xc1a7ebe3 )
	ROM_LOAD( "261-v4.bin", 0xc00000, 0x200000, 0x9000d085 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "261-c1.bin", 0x0000000, 0x800000, 0xded84d9c )
	ROM_LOAD16_BYTE( "261-c2.bin", 0x0000001, 0x800000, 0xb8eb4348 )
	ROM_LOAD16_BYTE( "261-c3.bin", 0x1000000, 0x800000, 0x84e2034a )
	ROM_LOAD16_BYTE( "261-c4.bin", 0x1000001, 0x800000, 0x0b45ae53 )
ROM_END

ROM_START( kof2001 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "262-p1.bin", 0x000000, 0x100000, 0x9381750d )
	ROM_LOAD16_WORD_SWAP( "262-p2.bin", 0x100000, 0x400000, 0x8e0d8329 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	/* The M1 ROM is encrypted, we load it here for reference and replace it with a decrypted version */
	ROM_REGION( 0x40000, REGION_USER4, 0 )
	ROM_LOAD( "262-m1.bin", 0x00000, 0x20000, 0x00000000 )
	NEO_BIOS_SOUND_256K( "262-m1d.bin", 0xdfb908ca )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "262-v1.bin", 0x000000, 0x400000, 0x83d49ecf )
	ROM_LOAD( "262-v2.bin", 0x400000, 0x400000, 0x003f1843 )
	ROM_LOAD( "262-v3.bin", 0x800000, 0x400000, 0x2ae38dbe )
	ROM_LOAD( "262-v4.bin", 0xc00000, 0x400000, 0x26ec4dd9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "262-c1.bin", 0x0000000, 0x800000, 0x99cc785a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c2.bin", 0x0000001, 0x800000, 0x50368cbf ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c3.bin", 0x1000000, 0x800000, 0xfb14ff87 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c4.bin", 0x1000001, 0x800000, 0x4397faf8 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c5.bin", 0x2000000, 0x800000, 0x91f24be4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c6.bin", 0x2000001, 0x800000, 0xa31e4403 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c7.bin", 0x3000000, 0x800000, 0x54d9d1ec ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c8.bin", 0x3000001, 0x800000, 0x59289a6b ) /* Plane 2,3 */
ROM_END

ROM_START( mslug4 ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "263-p1.bin",  0x000000, 0x100000, 0x27e4def3 )
	ROM_LOAD16_WORD_SWAP( "263-p2.bin",  0x100000, 0x400000, 0xfdb7aed8 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	/* The M1 ROM is encrypted, we load it here for reference and replace it with a decrypted version */
	ROM_REGION( 0x40000, REGION_USER4, 0 )
	ROM_LOAD( "263-m1.bin", 0x00000, 0x10000, 0x38ffad14 )
	NEO_BIOS_SOUND_64K( "263-m1d.bin", 0x69fedba1 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "263-v1.bin", 0x000000, 0x800000, 0xb1a08c67 )
	ROM_LOAD( "263-v2.bin", 0x800000, 0x800000, 0x0040015b )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "263-c1.bin",   0x0000000, 0x800000, 0x6c2b0856 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "263-c2.bin",   0x0000001, 0x800000, 0xc6035792 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "263-c3.bin",   0x1000000, 0x800000, 0x0721d112 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "263-c4.bin",   0x1000001, 0x800000, 0x6aa688dd ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "263-c5.bin",   0x2000000, 0x800000, 0x794bc2d6 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "263-c6.bin",   0x2000001, 0x800000, 0xf85eae54 ) /* Plane 2,3 */
ROM_END

ROM_START( rotd )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "264-p1.bin", 0x000000, 0x800000, 0xb8cc969d )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(				  0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	ROM_REGION( 0x40000, REGION_USER4, 0 )
	ROM_LOAD( "264-m1.bin", 0x00000, 0x10000, 0x9abd048c ) /* encrypted, we load it here for reference and replace with decrypted ROM */
	NEO_BIOS_SOUND_64K( "264-m1d.bin", 0xe5f42e7d ) /* decrypted */

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* encrypted */
	ROM_LOAD( "264-v1.bin", 0x000000, 0x800000, 0xfa005812 )
	ROM_LOAD( "264-v2.bin", 0x800000, 0x800000, 0xc3dc8bf0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "264-c1.bin", 0x0000000, 0x800000, 0x4f148fee )
	ROM_LOAD16_BYTE( "264-c2.bin", 0x0000001, 0x800000, 0x7cf5ff72 )
	ROM_LOAD16_BYTE( "264-c3.bin", 0x1000000, 0x800000, 0x64d84c98 )
	ROM_LOAD16_BYTE( "264-c4.bin", 0x1000001, 0x800000, 0x2f394a95 )
	ROM_LOAD16_BYTE( "264-c5.bin", 0x2000000, 0x800000, 0x6b99b978 )
	ROM_LOAD16_BYTE( "264-c6.bin", 0x2000001, 0x800000, 0x847d5c7d )
	ROM_LOAD16_BYTE( "264-c7.bin", 0x3000000, 0x800000, 0x231d681e )
	ROM_LOAD16_BYTE( "264-c8.bin", 0x3000001, 0x800000, 0xc5edb5c4 )
ROM_END

ROM_START( kof2002 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "265-p1.bin", 0x000000, 0x100000, 0x9ede7323 )
	ROM_LOAD16_WORD_SWAP( "265-p2.bin", 0x100000, 0x400000, 0x327266b8 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(				  0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )

	ROM_REGION( 0x40000, REGION_USER4, 0 )
	ROM_LOAD( "265-m1.bin", 0x00000, 0x40000, 0x7dbab719 ) /* encrypted, we load it here for reference and replace with decrypted ROM */
	NEO_BIOS_SOUND_128K( "265-m1d.bin", 0xab9d360e ) /* decrypted */

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* encrypted, we load these here for reference and replace with decrypted ROMs */
	ROM_LOAD( "265-v1.bin", 0x000000, 0x800000, 0x15e8f3f5 )
	ROM_LOAD( "265-v2.bin", 0x800000, 0x800000, 0xda41d6f9 )
	/* decrypted */
	ROM_LOAD( "265-v1d.bin", 0x000000, 0x400000, 0x13d98607 )
	ROM_LOAD( "265-v2d.bin", 0x400000, 0x400000, 0x9cf74677 )
	ROM_LOAD( "265-v3d.bin", 0x800000, 0x400000, 0x8e9448b5 )
	ROM_LOAD( "265-v4d.bin", 0xc00000, 0x400000, 0x067271b5 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "265-c1.bin", 0x0000000, 0x800000, 0x2b65a656 )
	ROM_LOAD16_BYTE( "265-c2.bin", 0x0000001, 0x800000, 0xadf18983 )
	ROM_LOAD16_BYTE( "265-c3.bin", 0x1000000, 0x800000, 0x875e9fd7 )
	ROM_LOAD16_BYTE( "265-c4.bin", 0x1000001, 0x800000, 0x2da13947 )
	ROM_LOAD16_BYTE( "265-c5.bin", 0x2000000, 0x800000, 0x61bd165d )
	ROM_LOAD16_BYTE( "265-c6.bin", 0x2000001, 0x800000, 0x03fdd1eb )
	ROM_LOAD16_BYTE( "265-c7.bin", 0x3000000, 0x800000, 0x1a2749d8 )
	ROM_LOAD16_BYTE( "265-c8.bin", 0x3000001, 0x800000, 0xab0bb549 )
ROM_END


/******************************************************************************/

/* dummy entry for the dummy bios driver */
ROM_START( neogeo )
	ROM_REGION16_BE( 0x020000, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "ng-lo.rom", 0x00000, 0x10000, 0xe09e253c )  /* Y zoom control */

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x000000, 0x20000, 0x354029fc )
ROM_END



DRIVER_INIT( kof99 )
{
	data16_t *rom;
	int i,j;

	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	/* swap data lines on the whole ROMs */
	for (i = 0;i < 0x800000/2;i++)
	{
		rom[i] = BITSWAP16(rom[i],13,7,3,0,9,4,5,6,1,12,8,14,10,11,2,15);
	}

	/* swap address lines for the banked part */
	for (i = 0;i < 0x600000/2;i+=0x800/2)
	{
		data16_t buffer[0x800/2];
		memcpy(buffer,&rom[i],0x800);
		for (j = 0;j < 0x800/2;j++)
		{
			rom[i+j] = buffer[BITSWAP24(j,23,22,21,20,19,18,17,16,15,14,13,12,11,10,6,2,4,9,8,3,1,7,0,5)];
		}
	}

	/* swap address lines & relocate fixed part */
	rom = (data16_t *)memory_region(REGION_CPU1);
	for (i = 0;i < 0x0c0000/2;i++)
	{
		rom[i] = rom[0x700000/2 + BITSWAP24(i,23,22,21,20,19,18,11,6,14,17,16,5,8,10,12,0,4,3,2,7,9,15,13,1)];
	}

	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x00);
	init_neogeo();
	install_mem_read16_handler(0, 0x2ffff8, 0x2ffff9, sma_random_r);
	install_mem_read16_handler(0, 0x2ffffa, 0x2ffffb, sma_random_r);
}

DRIVER_INIT( garou )
{
	data16_t *rom;
	int i,j;

	/* thanks to Razoola and Mr K for the info */
	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	/* swap data lines on the whole ROMs */
	for (i = 0;i < 0x800000/2;i++)
	{
		rom[i] = BITSWAP16(rom[i],13,12,14,10,8,2,3,1,5,9,11,4,15,0,6,7);
	}

	/* swap address lines & relocate fixed part */
	rom = (data16_t *)memory_region(REGION_CPU1);
	for (i = 0;i < 0x0c0000/2;i++)
	{
		rom[i] = rom[0x710000/2 + BITSWAP24(i,23,22,21,20,19,18,4,5,16,14,7,9,6,13,17,15,3,1,2,12,11,8,10,0)];
	}

	/* swap address lines for the banked part */
	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	for (i = 0;i < 0x800000/2;i+=0x8000/2)
	{
		data16_t buffer[0x8000/2];
		memcpy(buffer,&rom[i],0x8000);
		for (j = 0;j < 0x8000/2;j++)
		{
			rom[i+j] = buffer[BITSWAP24(j,23,22,21,20,19,18,17,16,15,14,9,4,8,3,13,6,2,7,0,12,1,11,10,5)];
		}
	}

	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x06);
	init_neogeo();
	install_mem_read16_handler(0, 0x2fffcc, 0x2fffcd, sma_random_r);
	install_mem_read16_handler(0, 0x2ffff0, 0x2ffff1, sma_random_r);
}

DRIVER_INIT( garouo )
{
	data16_t *rom;
	int i,j;

	/* thanks to Razoola and Mr K for the info */
	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	/* swap data lines on the whole ROMs */
	for (i = 0;i < 0x800000/2;i++)
	{
		rom[i] = BITSWAP16(rom[i],14,5,1,11,7,4,10,15,3,12,8,13,0,2,9,6);
	}

	/* swap address lines & relocate fixed part */
	rom = (data16_t *)memory_region(REGION_CPU1);
	for (i = 0;i < 0x0c0000/2;i++)
	{
		rom[i] = rom[0x7f8000/2 + BITSWAP24(i,23,22,21,20,19,18,5,16,11,2,6,7,17,3,12,8,14,4,0,9,1,10,15,13)];
	}

	/* swap address lines for the banked part */
	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	for (i = 0;i < 0x800000/2;i+=0x8000/2)
	{
		data16_t buffer[0x8000/2];
		memcpy(buffer,&rom[i],0x8000);
		for (j = 0;j < 0x8000/2;j++)
		{
			rom[i+j] = buffer[BITSWAP24(j,23,22,21,20,19,18,17,16,15,14,12,8,1,7,11,3,13,10,6,9,5,4,0,2)];
		}
	}

	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x06);
	init_neogeo();
	install_mem_read16_handler(0, 0x2fffcc, 0x2fffcd, sma_random_r);
	install_mem_read16_handler(0, 0x2ffff0, 0x2ffff1, sma_random_r);
}

DRIVER_INIT( mslug3 )
{
	data16_t *rom;
	int i,j;

	/* thanks to Razoola and Mr K for the info */
	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	/* swap data lines on the whole ROMs */
	for (i = 0;i < 0x800000/2;i++)
	{
		rom[i] = BITSWAP16(rom[i],4,11,14,3,1,13,0,7,2,8,12,15,10,9,5,6);
	}

	/* swap address lines & relocate fixed part */
	rom = (data16_t *)memory_region(REGION_CPU1);
	for (i = 0;i < 0x0c0000/2;i++)
	{
		rom[i] = rom[0x5d0000/2 + BITSWAP24(i,23,22,21,20,19,18,15,2,1,13,3,0,9,6,16,4,11,5,7,12,17,14,10,8)];
	}

	/* swap address lines for the banked part */
	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	for (i = 0;i < 0x800000/2;i+=0x10000/2)
	{
		data16_t buffer[0x10000/2];
		memcpy(buffer,&rom[i],0x10000);
		for (j = 0;j < 0x10000/2;j++)
		{
			rom[i+j] = buffer[BITSWAP24(j,23,22,21,20,19,18,17,16,15,2,11,0,14,6,4,13,8,9,3,10,7,5,12,1)];
		}
	}

	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xad);
	init_neogeo();
}

DRIVER_INIT( kof2000 )
{
	data16_t *rom;
	int i,j;

	/* thanks to Razoola and Mr K for the info */
	rom = (data16_t *)(memory_region(REGION_CPU1) + 0x100000);
	/* swap data lines on the whole ROMs */
	for (i = 0;i < 0x800000/2;i++)
	{
		rom[i] = BITSWAP16(rom[i],12,8,11,3,15,14,7,0,10,13,6,5,9,2,1,4);
	}

	/* swap address lines for the banked part */
	for (i = 0;i < 0x63a000/2;i+=0x800/2)
	{
		data16_t buffer[0x800/2];
		memcpy(buffer,&rom[i],0x800);
		for (j = 0;j < 0x800/2;j++)
		{
			rom[i+j] = buffer[BITSWAP24(j,23,22,21,20,19,18,17,16,15,14,13,12,11,10,4,1,3,8,6,2,7,0,9,5)];
		}
	}

	/* swap address lines & relocate fixed part */
	rom = (data16_t *)memory_region(REGION_CPU1);
	for (i = 0;i < 0x0c0000/2;i++)
	{
		rom[i] = rom[0x73a000/2 + BITSWAP24(i,23,22,21,20,19,18,8,4,15,13,3,14,16,2,6,17,7,12,10,0,5,11,1,9)];
	}

	neogeo_fix_bank_type = 2;
	kof2000_neogeo_gfx_decrypt(0x00);
	init_neogeo();
	install_mem_read16_handler(0, 0x2fffd8, 0x2fffd9, sma_random_r);
	install_mem_read16_handler(0, 0x2fffda, 0x2fffdb, sma_random_r);
}


DRIVER_INIT( kof99n )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x00);
	init_neogeo();
}

DRIVER_INIT( ganryu )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x07);
	init_neogeo();
}

DRIVER_INIT( s1945p )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x05);
	init_neogeo();
}

DRIVER_INIT( preisle2 )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x9f);
	init_neogeo();
}

DRIVER_INIT( mslug3n )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xad);
	init_neogeo();
}

DRIVER_INIT( kof2000n )
{
	neogeo_fix_bank_type = 2;
	kof2000_neogeo_gfx_decrypt(0x00);
	init_neogeo();
}

DRIVER_INIT( nitd )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xff);
	init_neogeo();
}

DRIVER_INIT( zupapa )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xbd);
	init_neogeo();
}

DRIVER_INIT( sengoku3 )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xfe);
	init_neogeo();
}

DRIVER_INIT( kof2001 )
{
	neogeo_fix_bank_type = 0;
	kof2000_neogeo_gfx_decrypt(0x1e);
	init_neogeo();
}

DRIVER_INIT( mslug4 )
{
	data16_t *rom;
	int i,j;

	neogeo_fix_bank_type = 1; /* maybe slightly different, USA violent content screen is wrong */
	kof2000_neogeo_gfx_decrypt(0x31);
	init_neogeo();

	/* thanks to Elsemi for the NEO-PCM2 info */
	rom = (data16_t *)(memory_region(REGION_SOUND1));
	if( rom != NULL )
	{
		/* swap address lines on the whole ROMs */
		for( i = 0; i < 0x1000000 / 2; i += 8 / 2 )
		{
			data16_t buffer[ 8 / 2 ];
			memcpy( buffer, &rom[ i ], 8 );
			for( j = 0; j < 8 / 2; j++ )
			{
				rom[ i + j ] = buffer[ j ^ 2 ];
			}
		}
	}
}

DRIVER_INIT( rotd )
{
	data16_t *rom;
	int i,j;

	neogeo_fix_bank_type = 1;
	kof2000_neogeo_gfx_decrypt(0x3f);
	init_neogeo();

	/* thanks to Elsemi for the NEO-PCM2 info */
	rom = (data16_t *)(memory_region(REGION_SOUND1));
	if( rom != NULL )
	{
		/* swap address lines on the whole ROMs */
		for( i = 0; i < 0x1000000 / 2; i += 16 / 2 )
		{
			data16_t buffer[ 16 / 2 ];
			memcpy( buffer, &rom[ i ], 16 );
			for( j = 0; j < 16 / 2; j++ )
			{
				rom[ i + j ] = buffer[ j ^ 4 ];
			}
		}
	}
}

DRIVER_INIT( kof2002 )
{
	UINT8 *src = memory_region(REGION_CPU1)+0x100000;
	UINT8 *dst = malloc(0x400000);
	int i;
	unsigned int sec[]={0x100000,0x280000,0x300000,0x180000,0x000000,0x380000,0x200000,0x080000};

	if (dst)
	{
		memcpy(dst,src,0x400000);

		for(i=0;i<8;++i)
		{
			memcpy(src+i*0x80000,dst+sec[i],0x80000);
		}
		free(dst);
	}

	neogeo_fix_bank_type = 0;
	kof2000_neogeo_gfx_decrypt(0xec);

	init_neogeo();
}


/******************************************************************************/

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the neo-geo.rom file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1990, neogeo, 0, neogeo, neogeo, neogeo, ROT0, "SNK", "Neo-Geo", NOT_A_DRIVER )

/******************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE INPUT	 INIT	 MONITOR  */

/* SNK */
GAME( 1990, nam1975,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "NAM-1975" )
GAME( 1990, bstars,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Baseball Stars Professional" )
GAME( 1990, tpgolf,   neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Top Player's Golf" )
GAME( 1990, mahretsu, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Mahjong Kyoretsuden" )
GAME( 1990, ridhero,  neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Riding Hero (set 1)" )
GAME( 1990, ridheroh, ridhero,  raster, neogeo,  neogeo,   ROT0, "SNK", "Riding Hero (set 2)" )
GAME( 1991, alpham2,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Alpha Mission II / ASO II - Last Guardian" )
GAME( 1990, cyberlip, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Cyber-Lip" )
GAME( 1990, superspy, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "The Super Spy" )
GAME( 1992, mutnat,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Mutation Nation" )
GAME( 1991, kotm,     neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "King of the Monsters" )
GAME( 1991, sengoku,  neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Sengoku / Sengoku Denshou (set 1)" )
GAME( 1991, sengokh,  sengoku,  raster, neogeo,  neogeo,   ROT0, "SNK", "Sengoku / Sengoku Denshou (set 2)" )
GAME( 1991, burningf, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Burning Fight (set 1)" )
GAME( 1991, burningh, burningf, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Burning Fight (set 2)" )
GAME( 1990, lbowling, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "League Bowling" )
GAME( 1991, gpilots,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Ghost Pilots" )
GAME( 1990, joyjoy,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Puzzled / Joy Joy Kid" )
GAME( 1991, quizdais, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Quiz Daisousa Sen - The Last Count Down" )
GAME( 1992, lresort,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Last Resort" )
GAME( 1991, eightman, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK / Pallas", "Eight Man" )
GAME( 1991, legendos, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Legend of Success Joe / Ashitano Joe Densetsu" )
GAME( 1991, 2020bb,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK / Pallas", "2020 Super Baseball (set 1)" )
GAME( 1991, 2020bbh,  2020bb,   neogeo, neogeo,  neogeo,   ROT0, "SNK / Pallas", "2020 Super Baseball (set 2)" )
GAME( 1991, socbrawl, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Soccer Brawl" )
GAME( 1991, fatfury1, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury - King of Fighters / Garou Densetsu - shukumei no tatakai" )
GAME( 1991, roboarmy, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Robo Army" )
GAME( 1992, fbfrenzy, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Football Frenzy" )
GAME( 1992, kotm2,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "King of the Monsters 2 - The Next Thing" )
GAME( 1993, sengoku2, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Sengoku 2 / Sengoku Denshou 2")
GAME( 1992, bstars2,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Baseball Stars 2" )
GAME( 1992, quizdai2, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Quiz Meintantei Neo Geo - Quiz Daisousa Sen Part 2" )
GAME( 1993, 3countb,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "3 Count Bout / Fire Suplex" )
GAME( 1992, aof,      neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Art of Fighting / Ryuuko no Ken" )
GAME( 1993, samsho,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown / Samurai Spirits" )
GAME( 1994, tophuntr, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Top Hunter - Roddy & Cathy" )
GAME( 1992, fatfury2, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury 2 / Garou Densetsu 2 - arata-naru tatakai" )
GAME( 1992, ssideki,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Super Sidekicks / Tokuten Ou" )
GAME( 1994, kof94,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '94" )
GAME( 1994, aof2,     neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Art of Fighting 2 / Ryuuko no Ken 2" )
GAME( 1993, fatfursp, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury Special / Garou Densetsu Special" )
GAME( 1995, savagere, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Savage Reign / Fu'un Mokushiroku - kakutou sousei" )
GAME( 1994, ssideki2, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Super Sidekicks 2 - The World Championship / Tokuten Ou 2 - real fight football" )
GAME( 1994, samsho2,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown II / Shin Samurai Spirits - Haohmaru jigokuhen" )
GAME( 1995, fatfury3, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury 3 - Road to the Final Victory / Garou Densetsu 3 - haruka-naru tatakai" )
GAME( 1995, ssideki3, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Super Sidekicks 3 - The Next Glory / Tokuten Ou 3 - eikoue no michi" )
GAME( 1995, kof95,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '95" )
GAME( 1995, samsho3,  neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown III / Samurai Spirits - Zankurou Musouken" )
GAME( 1995, rbff1,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Real Bout Fatal Fury / Real Bout Garou Densetsu" )
GAME( 1996, aof3,     neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Art of Fighting 3 - The Path of the Warrior / Art of Fighting - Ryuuko no Ken Gaiden" )
GAME( 1996, kof96,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '96" )
GAME( 1996, ssideki4, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "The Ultimate 11 / Tokuten Ou - Honoo no Libero" )
GAME( 1996, kizuna,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Kizuna Encounter - Super Tag Battle / Fu'un Super Tag Battle" )
GAME( 1996, samsho4,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown IV - Amakusa's Revenge / Samurai Spirits - Amakusa Kourin" )
GAME( 1996, rbffspec, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Real Bout Fatal Fury Special / Real Bout Garou Densetsu Special" )
GAME( 1997, kof97,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '97" )
GAME( 1997, lastblad, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "The Last Blade / Bakumatsu Roman - Gekkano Kenshi" )
GAME( 1997, irrmaze,  neogeo,   neogeo, irrmaze, neogeo,   ROT0, "SNK / Saurus", "The Irritating Maze / Ultra Denryu Iraira Bou" )
GAME( 1998, rbff2,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Real Bout Fatal Fury 2 - The Newcomers / Real Bout Garou Densetsu 2 - the newcomers" )
GAME( 1998, mslug2,   neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Metal Slug 2 - Super Vehicle-001/II" )
GAME( 1998, kof98,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '98 - The Slugfest / King of Fighters '98 - dream match never ends" )
GAME( 1998, lastbld2, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "The Last Blade 2 / Bakumatsu Roman - Dai Ni Maku Gekkano Kenshi" )
GAME( 1998, neocup98, neogeo,   raster, neogeo,  neogeo,   ROT0, "SNK", "Neo-Geo Cup '98 - The Road to the Victory" )
GAME( 1999, mslugx,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "SNK", "Metal Slug X - Super Vehicle-001" )
GAME( 1999, kof99,    neogeo,   raster, neogeo,  kof99,    ROT0, "SNK", "The King of Fighters '99 - Millennium Battle" ) /* Encrypted Code & GFX */
GAME( 1999, kof99e,   kof99,    raster, neogeo,  kof99,    ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (earlier)" ) /* Encrypted Code & GFX */
GAME( 1999, kof99n,   kof99,    raster, neogeo,  kof99n,   ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (not encrypted)" )	/* Encrypted GFX */
GAME( 1999, kof99p,   kof99,    raster, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (prototype)" )
GAME( 1999, garou,    neogeo,   raster, neogeo,  garou,    ROT0, "SNK", "Garou - Mark of the Wolves (set 1)" ) /* Encrypted Code & GFX */
GAME( 1999, garouo,   garou,    raster, neogeo,  garouo,   ROT0, "SNK", "Garou - Mark of the Wolves (set 2))" ) /* Encrypted Code & GFX */
GAME( 1999, garoup,   garou,    raster, neogeo,  neogeo,   ROT0, "SNK", "Garou - Mark of the Wolves (prototype)" )
GAME( 2000, mslug3,   neogeo,   raster, neogeo,  mslug3,   ROT0, "SNK", "Metal Slug 3" ) /* Encrypted Code & GFX */
GAME( 2000, mslug3n,  mslug3,   raster, neogeo,  mslug3n,  ROT0, "SNK", "Metal Slug 3 (not encrypted)" ) /* Encrypted GFX */
GAME( 2000, kof2000,  neogeo,   neogeo, neogeo,  kof2000,  ROT0, "SNK", "The King of Fighters 2000" ) /* Encrypted Code & GFX */
GAME( 2000, kof2000n, kof2000,  neogeo, neogeo,  kof2000n, ROT0, "SNK", "The King of Fighters 2000 (not encrypted)" ) /* Encrypted GFX */
GAME( 2001, zupapa,   neogeo,   neogeo, neogeo,  zupapa,   ROT0, "SNK", "Zupapa!" )	/* Encrypted GFX */
GAME( 2001, sengoku3, neogeo,   neogeo, neogeo,  sengoku3, ROT0, "SNK", "Sengoku 3" )	/* Encrypted GFX */

/* Alpha Denshi Co. / ADK (changed name in 1993) */
GAME( 1990, maglord,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Magician Lord (set 1)" )
GAME( 1990, maglordh, maglord,  neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Magician Lord (set 2)" )
GAME( 1990, ncombat,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Ninja Combat" )
GAME( 1990, bjourney, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Blue's Journey / Raguy" )
GAME( 1991, crsword,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Crossed Swords" )
GAME( 1991, trally,   neogeo,   raster, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Thrash Rally" )
GAME( 1992, ncommand, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Ninja Commando" )
GAME( 1992, wh1,      neogeo,   raster, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "World Heroes" )
GAME( 1993, wh2,      neogeo,   raster, neogeo,  neogeo,   ROT0, "ADK",              "World Heroes 2" )
GAME( 1994, wh2j,     neogeo,   raster, neogeo,  neogeo,   ROT0, "ADK / SNK",        "World Heroes 2 Jet" )
GAME( 1994, aodk,     neogeo,   raster, neogeo,  neogeo,   ROT0, "ADK / SNK",        "Aggressors of Dark Kombat / Tsuukai GANGAN Koushinkyoku" )
GAME( 1995, whp,      neogeo,   neogeo, neogeo,  neogeo,   ROT0, "ADK / SNK",        "World Heroes Perfect" )
GAME( 1995, mosyougi, neogeo,   raster_busy, neogeo,  neogeo,   ROT0, "ADK / SNK",        "Syougi No Tatsujin - Master of Syougi" )
GAME( 1996, overtop,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "ADK",              "Over Top" )
GAME( 1996, ninjamas, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "ADK / SNK",        "Ninja Master's - haoh-ninpo-cho" )
GAME( 1996, twinspri, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "ADK",              "Twinkle Star Sprites" )

/* Aicom */
GAME( 1994, janshin,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Aicom", "Jyanshin Densetsu - Quest of Jongmaster" )
GAME( 1995, pulstar,  neogeo,   raster, neogeo,  neogeo,   ROT0, "Aicom", "Pulstar" )

/* Data East Corporation */
GAME( 1993, spinmast, neogeo,   raster, neogeo,  neogeo,   ROT0, "Data East Corporation", "Spin Master / Miracle Adventure" )
GAME( 1994, wjammers, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Data East Corporation", "Windjammers / Flying Power Disc" )
GAME( 1994, karnovr,  neogeo,   raster, neogeo,  neogeo,   ROT0, "Data East Corporation", "Karnov's Revenge / Fighter's History Dynamite" )
GAME( 1994, strhoop,  neogeo,   raster, neogeo,  neogeo,   ROT0, "Data East Corporation", "Street Hoop / Street Slam / Dunk Dream" )
GAME( 1996, magdrop2, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Data East Corporation", "Magical Drop II" )
GAME( 1997, magdrop3, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Data East Corporation", "Magical Drop III" )

/* Eleven */
GAME( 2000, nitd,     neogeo,   neogeo, neogeo,  nitd,     ROT0, "Eleven / Gavaking", "Nightmare in the Dark" ) /* Encrypted GFX */

/* Face */
GAME( 1994, gururin,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Face", "Gururin" )
GAME( 1997, miexchng, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Face", "Money Puzzle Exchanger / Money Idol Exchanger" )

/* Hudson Soft */
GAME( 1994, panicbom, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Eighting / Hudson", "Panic Bomber" )
GAME( 1995, kabukikl, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Hudson", "Kabuki Klash - Far East of Eden / Tengai Makyou Shinden - Far East of Eden" )
GAME( 1997, neobombe, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Hudson", "Neo Bomberman" )

/* Monolith Corp. */
GAME( 1990, minasan,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Monolith Corp.", "Minnasanno Okagesamadesu" )
GAME( 1991, bakatono, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Monolith Corp.", "Bakatonosama Mahjong Manyuki" )

/* Nazca */
GAME( 1996, turfmast, neogeo,   raster, neogeo,  neogeo,   ROT0, "Nazca", "Neo Turf Masters / Big Tournament Golf" )
GAME( 1996, mslug,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Nazca", "Metal Slug - Super Vehicle-001" )

/* NMK */
GAME( 1994, zedblade, neogeo,   raster, neogeo,  neogeo,   ROT0, "NMK", "Zed Blade / Operation Ragnarok" )

/* Psikyo */
GAME( 1999, s1945p,   neogeo,   neogeo, neogeo,  s1945p,   ROT0, "Psikyo", "Strikers 1945 Plus" )	/* Encrypted GFX */

/* Sammy */
GAME( 1992, viewpoin, neogeo,   raster, neogeo,  neogeo,   ROT0, "Sammy", "Viewpoint" )

/* Saurus */
GAME( 1995, quizkof,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Quiz King of Fighters" )
GAME( 1995, stakwin,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Stakes Winner / Stakes Winner - GI kinzen seihae no michi" )
GAME( 1996, ragnagrd, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Operation Ragnagard / Shin-Oh-Ken" )
GAME( 1996, pgoal,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Pleasure Goal / Futsal - 5 on 5 Mini Soccer" )
GAME( 1996, stakwin2, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Stakes Winner 2" )
GAME( 1997, shocktro, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Shock Troopers" )
GAME( 1997, shocktrj, shocktro, neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Shock Troopers (Japan)" )
GAME( 1998, shocktr2, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Shock Troopers - 2nd Squad" )

/* Sunsoft */
GAME( 1995, galaxyfg, neogeo,   raster, neogeo,  neogeo,   ROT0, "Sunsoft", "Galaxy Fight - Universal Warriors" )
GAME( 1996, wakuwak7, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Sunsoft", "Waku Waku 7" )

/* Taito */
GAME( 1994, pbobblen, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Taito", "Puzzle Bobble / Bust-A-Move (Neo-Geo)" )
GAME( 1999, pbobbl2n, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Taito (SNK license)", "Puzzle Bobble 2 / Bust-A-Move Again (Neo-Geo)" )

/* Takara */
GAME( 1995, marukodq, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Takara", "Chibi Marukochan Deluxe Quiz" )

/* Technos */
GAME( 1995, doubledr, neogeo,   raster, neogeo,  neogeo,   ROT0, "Technos", "Double Dragon (Neo-Geo)" )
GAME( 1995, gowcaizr, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Technos", "Voltage Fighter - Gowcaizer / Choujin Gakuen Gowcaizer")
GAME( 1996, sdodgeb,  neogeo,   raster, neogeo,  neogeo,   ROT0, "Technos", "Super Dodge Ball / Kunio no Nekketsu Toukyuu Densetsu" )

/* Tecmo */
GAME( 1996, tws96,    neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Tecmo", "Tecmo World Soccer '96" )

/* Yumekobo */
GAME( 1998, blazstar, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Yumekobo", "Blazing Star" )
GAME( 1999, preisle2, neogeo,   neogeo, neogeo,  preisle2, ROT0, "Yumekobo", "Prehistoric Isle 2" ) /* Encrypted GFX */

/* Viccom */
GAME( 1994, fightfev, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Viccom", "Fight Fever / Crystal Legacy" )

/* Video System Co. */
GAME( 1994, pspikes2, neogeo,   raster, neogeo,  neogeo,   ROT0, "Video System Co.", "Power Spikes II" )
GAME( 1994, sonicwi2, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Video System Co.", "Aero Fighters 2 / Sonic Wings 2" )
GAME( 1995, sonicwi3, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Video System Co.", "Aero Fighters 3 / Sonic Wings 3" )
GAME( 1997, popbounc, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Video System Co.", "Pop 'n Bounce / Gapporin" )

/* Visco */
GAME( 1992, androdun, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Visco", "Andro Dunos" )
GAME( 1995, puzzledp, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Taito (Visco license)", "Puzzle De Pon" )
GAME( 1996, neomrdo,  neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Visco", "Neo Mr. Do!" )
GAME( 1995, goalx3,   neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Visco", "Goal! Goal! Goal!" )
GAME( 1996, neodrift, neogeo,   raster, neogeo,  neogeo,   ROT0, "Visco", "Neo Drift Out - New Technology" )
GAME( 1996, breakers, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Visco", "Breakers" )
GAME( 1997, puzzldpr, puzzledp, neogeo, neogeo,  neogeo,   ROT0, "Taito (Visco license)", "Puzzle De Pon R" )
GAME( 1998, breakrev, breakers, neogeo, neogeo,  neogeo,   ROT0, "Visco", "Breakers Revenge")
GAME( 1998, flipshot, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Visco", "Battle Flip Shot" )
GAME( 1999, ctomaday, neogeo,   neogeo, neogeo,  neogeo,   ROT0, "Visco", "Captain Tomaday" )
GAME( 1999, ganryu,   neogeo,   neogeo, neogeo,  ganryu,   ROT0, "Visco", "Musashi Ganryuuki" )	/* Encrypted GFX */
GAME( 2000, bangbead, neogeo,   raster, neogeo,  neogeo,   ROT0, "Visco", "Bang Bead (prototype)" )

/* Eolith */
GAME( 2001, kof2001,  neogeo,   neogeo, neogeo,  kof2001,  ROT0, "Eolith / SNK", "The King of Fighters 2001" )
GAME( 2002, kof2002,  neogeo,	neogeo, neogeo,  kof2002,  ROT0, "Eolith / Playmore Corporation", "The King of Fighters 2002" )

/* Mega Enterprise */
GAME( 2002, mslug4,   neogeo,   neogeo, neogeo,  mslug4,   ROT0, "Mega Enterprise / Playmore Corporation", "Metal Slug 4" )

/* Evoga */
GAME( 2002, rotd,	  neogeo,	neogeo, neogeo,  rotd,	   ROT0, "Evoga / Playmore Corporation", "Rage of the Dragons" )
