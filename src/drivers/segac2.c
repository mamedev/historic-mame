/***********************************************************************************************

	Sega System C/C2 Driver
	driver by David Haywood and Aaron Giles
	---------------------------------------
	Version 0.53 - 05 Mar 2001

	Latest Changes :
	-----+-------------------------------------------------------------------------------------
	0.53 | Set the Protection Read / Write buffers to be cleared at reset in init_machine, this
		 | fixes a problem with columns when you reset it and then attempted to play.
		 |  (only drivers/segac2.c was changed)
	0.52 | Added basic save state support .. not too sure how well it will work (seems to be
		 | ok apart from sound where i guess the sound core needs updating) size of states
		 | could probably be cut down a bit with an init variables from vdp registers function
	0.51 | Added some Puyo Puyo (English Lang) Bootleg set, we don't have an *original* english
		 | set yet however.  Also added a 2nd Bootleg of Tant-R, this one still has protection
		 | inplace, however the apparently board lacks the sample playing hardware.
		 | Removed 'Button3' from Puyo Puyo 2, its unneeded, the Rannyu Button is actually the
		 | player 1 start button, its used for stopping a 2nd player interupting play.
	0.50 | Added some Columns/Puyo Puyo 2 dips from Gerardo. Fixed nasty crash bug for games
		 | not using the UPD7759. Made some minor tweaks to the VDP register accesses. Added
		 | counter/timer description. Attempted to hook up battery RAM.
	0.49 | Fixed clock speeds on sound chips and CPU. Tweaked the highlight color a bit.
	0.48 | Added a kludge to make Stack Columns high score work. Fixed several video-related
		 | differences from c2emu which caused problems in Puyo Puyo.
	0.47 | Fixed Thunder Force hanging problem. No longer calling the UPD7759 on the C-system
		 | games.
    0.46 | Cleaned up the Dip Switches. Added Ichidant and Puyopuy2 Dips.
         | Todo: Complete Columns and Bloxeedc Dips.
	0.45 | Fixed interrupt timing to match c2emu, which fixes scroll effects in several games.
		 | Swapped sample ROM loading in some bootlegs. Added support for screen disable.
	0.44 | Major cleanup. Figured out general protection mechanism. Added INT 4 and INT 2
		 | support. Added shadow effects. Hooked up sound chips. Fixed palette banking. ASG
	0.43 | Added Support for an English Language version of Ichidant-R (and I didn't even think
		 | it was released outside of Japan, this is a nice Turn-Up) .. I wonder if Tant-R was
		 | also released in English ...
	0.42 | Removed WRITE_WORD which was being used to patch the roms as it is now obsolete,
		 | replaced it with mem16[addy >> 1] = 0xXXXX which is the newer way of doing things.
	0.41 | Mapped more Dipswitches and the odd minor fix.
	0.40 | Updated the Driver so it 'works' with the new memory system introduced in 0.37b8
		 | I'm pretty sure I've not done this right in places (for example writes to VRAM
		 | should probably use data16_t or something.  <request>Help</request> :)
		 | Also Added dipswitches to a few more of the games (Borench & PotoPoto) zzzz.
	0.38 | Started Mapping Dipswitches & Controls on a Per Game basis.  Thunderforce AC now has
		 | this information correct.
	0.37 | Set it to Clear Video Memory in vh_start, this stops the corrupt tile in Puyo Puyo 2
		 | however the game isn't really playable past the first level anyhow due to protection
	0.36 | Mainly a tidy-up release, trying to make the thing more readable :)
		 | also very minor fixes & changes
	0.35 | Added Window Emualtion, Horizontal Screen Splitting only (none of the C2 Games
		 | split the screen Vertically AFAIK so that element of the Genesis Hardware doesn't
		 | need to be emulated here.
		 | Thunderforce AC (bootleg)  tfrceacb is now playable and the test screens in Potopoto
		 | will display correctly.  Don't think I broke anything but knowing me :)
	0.34 | Fixed Some Things I accidentally broke (well forgot to put back) for the
		 | previous version ...
	0.33 | Fixed the Driver to Work in Pure Dos Mode (misunderstanding of the way MAME
		 | allocated (or in this case didn't allocate) the Palette Ram.
		 | Fixed a minor bug which was causing some problems in the scrolling in
		 | potopoto's conveyer belt.
	0.32 | Added Sprite Flipping This improves the GFX in several games, Tant-R (bootleg)
		 | is probably now playable just with bad colours on the status bar due to missing
		 | IRQ 4 Emulation.  Sprite Priority Also fixed .. another Typo
	0.31 | Fixed Several Stupid Typo Type Bugs :p
	-----+-------------------------------------------------------------------------------------

	Sega's C2 was used between 1989 and 1994, the hardware being very similar to that
	used by the Sega MegaDrive/Genesis Home Console Sega produced around the same time.

	Year  Game                  Developer         Versions Dumped  Board  Status        Gen Ver Exists?
	====  ====================  ================  ===============  =====  ============  ===============
	1989  Bloxeed               Sega / Elorg      Eng              C      Playable      n
	1990  Columns               Sega              Jpn              C      Playable      y
	1990  Columns II            Sega              Jpn              C      Playable      n
	1990  Borench               Sega              Eng              C2     Playable      n
	1990  ThunderForce AC       Sega / Technosoft Eng, Jpn, EngBL  C2     Playable      y (as ThunderForce 3?)
	1992  Tant-R                Sega              Jpn, JpnBL       C2     Playable      y
	1992  Puyo Puyo             Sega / Compile    Jpn (2 Vers)     C2     Playable      y
	1994  Ichidant-R            Sega              Jpn              C2     Playable      y
	1994  PotoPoto              Sega              Jpn              C2     Playable      n
	1994  Puyo Puyo 2           Compile           Jpn              C2     Playable      y
	1994  Stack Columns         Sega              Jpn              C2     Playable      n
	1994  Zunzunkyou No Yabou   Sega              Jpn              C2     Playable      n


	Notes:	Eng indicates game is in the English Language, Most Likely a European / US Romset
			Jpn indicates the game plays in Japanese and is therefore likely a Japanes Romset

			Another way to play these games is with Charles Macdonald's C2Emu, which was the
			inspiration for much of this code. Visit http://cgfm2.emuviews.com for the
			download, also home of some _very_ good Sega Genesis VDP documentation.

			The ASM 68k Core causes a scoring problem in Columns, Starscream does this also,
			with the C 68k Core the scoring in columns is correct.

			Bloxeed doesn't Read from the Protection Chip at all; all of the other games do.
			Currently the protection chip is mostly understood, and needs a table of 256
			4-bit values for each game. In all cases except for Poto Poto and Puyo Puyo 2,
			the table is embedded in the code. Workarounds for the other 2 cases are
			provided.

			I'm assuming System-C was the Board without the uPD7759 chip and System-C2 was the
			version of the board with it, this could be completely wrong but it doesn't really
			matter anyway.

			Vertical 2 Cell Based Scrolling won't be 100% accurate on a line that uses a Line
			Scroll value which isn't divisible by 8.. I've never seen a C2 game do this tho.


	Bugs:	Puyo Puyo ends up with a black screen after doing memory tests
			Battery-backed RAM needs to be figured out


	Thanks:	(in no particular order) to any MameDev that helped me out .. (OG, Mish etc.)
			Charles MacDonald for his C2Emu .. without it working out what were bugs in my code
				and issues due to protection would have probably killed the driver long ago :p
			Razoola & Antiriad .. for helping teach me some 68k ASM needed to work out just why
				the games were crashing :)
			Sega for producing some Fantastic Games...
			and anyone else who knows they've contributed :)

***********************************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "state.h"


#define LOG_PROTECTION		1
#define LOG_PALETTE			0
#define LOG_IOCHIP			0


/******************************************************************************
	Constants
******************************************************************************/

#define MASTER_CLOCK		53693100


/******************************************************************************
	Externals
******************************************************************************/

/* in vidhrdw\segac2.c */
extern UINT8		segac2_vdp_regs[];
extern int			segac2_bg_palbase;
extern int			segac2_sp_palbase;
extern int			segac2_palbank;

VIDEO_START( segac2 );
VIDEO_EOF( segac2 );
VIDEO_UPDATE( segac2 );

void	segac2_update_display(int scanline);
void	segac2_enable_display(int enable);

READ16_HANDLER ( segac2_vdp_r );
WRITE16_HANDLER( segac2_vdp_w );



/******************************************************************************
	Global variables
******************************************************************************/

/* interrupt states */
static UINT8		ym3438_int;			/* INT2 - from YM3438 */
static UINT8		scanline_int;		/* INT4 - programmable */
static UINT8		vblank_int;			/* INT6 - on every VBLANK */

/* internal states */
static UINT8		iochip_reg[0x10];	/* holds values written to the I/O chip */

/* protection-related tracking */
static const UINT32 *prot_table;		/* table of protection values */
static UINT16 		prot_write_buf;		/* remembers what was written */
static UINT16		prot_read_buf;		/* remembers what was returned */

/* sound-related variables */
static UINT8		sound_banks;		/* number of sound banks */
static UINT8		bloxeed_sound;		/* use kludge for bloxeed sound? */

/* RAM pointers */
static data16_t *	main_ram;			/* pointer to main RAM */



/******************************************************************************
	Interrupt handling
*******************************************************************************

	The C/C2 System uses 3 Different Interrupts, IRQ2, IRQ4 and IRQ6.

	IRQ6 = Vblank, this happens after the last visible line of the display has
			been drawn (after line 224)

	IRQ4 = H-Int, this happens based upon the value in H-Int Counter.  If the
			Horizontal Interrupt is enabled and the Counter Value = 0 there
			will be a Level 4 Interrupt Triggered

	IRQ2 = sound int, generated by the YM3438

	--------

	More H-Counter Information:

	Providing Horizontal Interrupts are active the H-Counter will be loaded
	with the value stored in register #10 (0x0A) at the following times:
		(1) At the top of the display, before drawing the first line
		(2) When the counter has expired
		(3) During the VBlank Period (lines 224-261)
	The Counter is decreased by 1 after every line.

******************************************************************************/

/* call this whenever the interrupt state has changed */
static void update_interrupts(void)
{
	int level = 0;

	/* determine which interrupt is active */
	if (ym3438_int) level = 2;
	if (scanline_int) level = 4;
	if (vblank_int) level = 6;

	/* either set or clear the appropriate lines */
	if (level)
		cpu_set_irq_line(0, level, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


/* timer callback to turn off the IRQ4 signal after a short while */
static void vdp_int4_off(int param)
{
	scanline_int = 0;
	update_interrupts();
}


/* timer callback to handle reloading the H counter and generate IRQ4 */
static void vdp_reload_counter(int scanline)
{
	/* generate an int if they're enabled */
	if ((segac2_vdp_regs[0] & 0x10) && !(iochip_reg[7] & 0x10))
		if (scanline != 0 || segac2_vdp_regs[10] == 0)
		{
			scanline_int = 1;
			update_interrupts();
			timer_set(cpu_getscanlinetime(scanline + 1), 0, vdp_int4_off);
		}

	/* advance to the next scanline */
	/* behavior 2: 0 count means interrupt after one scanline */
	/* (this behavior matches the Sega C2 emulator) */
	/* (in this case the vidhrdw ichidant kludge should be -2) */
	scanline += segac2_vdp_regs[10] + 1;
	if (scanline >= 224)
		scanline = 0;

	/* set a timer */
	timer_set(cpu_getscanlinetime(scanline) + cpu_getscanlineperiod() * (320. / 342.), scanline, vdp_reload_counter);
}


/* timer callback to turn off the IRQ6 signal after a short while */
static void vdp_int6_off(int param)
{
	vblank_int = 0;
	update_interrupts();
}


/* interrupt callback to generate the VBLANK interrupt */
static INTERRUPT_GEN( vblank_interrupt )
{
	/* generate the interrupt */
	vblank_int = 1;
	update_interrupts();

	/* set a timer to turn it off */
	timer_set(cpu_getscanlineperiod() * (22. / 342.), 0, vdp_int6_off);
}


/* interrupt callback to generate the YM3438 interrupt */
static void ym3438_interrupt(int state)
{
	ym3438_int = state;
	update_interrupts();
}



/******************************************************************************
	Machine init
*******************************************************************************

	This is called at init time, when it's safe to create a timer. We use
	it to prime the scanline interrupt timer.

******************************************************************************/

MACHINE_INIT( segac2 )
{
	/* set the first scanline 0 timer to go off */
	timer_set(cpu_getscanlinetime(0) + cpu_getscanlineperiod() * (320. / 342.), 0, vdp_reload_counter);

	/* determine how many sound banks */
	sound_banks = 0;
	if (memory_region(REGION_SOUND1))
		sound_banks = memory_region_length(REGION_SOUND1) / 0x20000;
	/* reset the protection */
	prot_write_buf = 0;
	prot_read_buf = 0;
}



/******************************************************************************
	Sound handlers
*******************************************************************************

	These handlers are responsible for communicating with the (genenerally)
	8-bit sound chips. All accesses are via the low byte.

	The Sega C/C2 system uses a YM3438 (compatible with the YM2612) for FM-
	based music generation, and an SN76489 for PSG and noise effects. The
	C2 board also appears to have a UPD7759 for sample playback.

******************************************************************************/

/* handle reads from the YM3438 */
static READ16_HANDLER( ym3438_r )
{
	switch (offset)
	{
		case 0: return YM2612_status_port_0_A_r(0);
		case 1: return YM2612_read_port_0_r(0);
		case 2: return YM2612_status_port_0_B_r(0);
	}
	return 0xff;
}


/* handle writes to the YM3438 */
static WRITE16_HANDLER( ym3438_w )
{
	/* only works if we're accessing the low byte */
	if (ACCESSING_LSB)
	{
		static UINT8 last_port;

		/* kludge for Bloxeed - it seems to accidentally trip timer 2  */
		/* and has no recourse for clearing the interrupt; until we    */
		/* find more documentation on the 2612/3438, it's unknown what */
		/* to do here */
		if (bloxeed_sound && last_port == 0x27 && (offset & 1))
			data &= ~0x08;

		switch (offset)
		{
			case 0: YM2612_control_port_0_A_w(0, data & 0xff);	last_port = data;	break;
			case 1: YM2612_data_port_0_A_w(0, data & 0xff);							break;
			case 2: YM2612_control_port_0_B_w(0, data & 0xff);	last_port = data;	break;
			case 3: YM2612_data_port_0_B_w(0, data & 0xff);							break;
		}
	}
}


/* handle writes to the UPD7759 */
static WRITE16_HANDLER( upd7759_w )
{
	/* make sure we have a UPD chip */
	if (!sound_banks)
		return;

	/* only works if we're accessing the low byte */
	if (ACCESSING_LSB)
	{
		UPD7759_reset_w(0, 0);
		UPD7759_reset_w(0, 1);
		UPD7759_port_w(0, data & 0xff);
		UPD7759_start_w(0, 0);
		UPD7759_start_w(0, 1);
	}
}


/* handle writes to the SN764896 */
static WRITE16_HANDLER( sn76489_w )
{
	/* only works if we're accessing the low byte */
	if (ACCESSING_LSB)
		SN76496_0_w(0, data & 0xff);
}



/******************************************************************************
	Palette RAM Read / Write Handlers
*******************************************************************************

	The following Read / Write Handlers are used when accessing Palette RAM.
	The C2 Hardware appears to use 4 Banks of Colours 1 of which can be Mapped
	to 0x8C0000 - 0x8C03FF at any given time by writes to 0x84000E (This same
	address also looks to be used for things like Sample Banking)

	Each Colour uses 15-bits (from a 16-bit word) in the Format
		xBGRBBBB GGGGRRRR  (x = unused, B = Blue, G = Green, R = Red)

	As this works out the Palette RAM Stores 2048 from a Possible 4096 Colours
	at any given time.

******************************************************************************/

/* handle reads from the paletteram */
static READ16_HANDLER( palette_r )
{
	return paletteram16[(offset & 0x1ff) + segac2_palbank];
}


/* handle writes to the paletteram */
static WRITE16_HANDLER( palette_w )
{
	int r,g,b,newword;

	/* adjust for the palette bank */
	offset = (offset & 0x1ff) + segac2_palbank;

	/* combine data */
	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	/* up to 8 bits */
	r = ((newword << 4) & 0xf0) | ((newword >>  9) & 0x08);
	g = ((newword >> 0) & 0xf0) | ((newword >> 10) & 0x08);
	b = ((newword >> 4) & 0xf0) | ((newword >> 11) & 0x08);
	r |= r >> 5;
	g |= g >> 5;
	b |= b >> 5;

	/* set the color */
	palette_set_color(offset + 0x0000, r, g, b);
}



/******************************************************************************
	Palette I/O Read & Write Handlers
*******************************************************************************

	Controls, and Poto Poto reads 'S' 'E' 'G' and 'A' (SEGA) from this area
	as a form of protection.

	Lots of unknown writes however offset 0E certainly seems to be banking,
	both colours and sound sample banks.

******************************************************************************/

/* handle I/O chip reads */
static READ16_HANDLER( iochip_r )
{
	switch (offset)
	{
		case 0x00:	return 0xff00 | readinputport(1);
		case 0x01:	return 0xff00 | readinputport(2);
		case 0x02:	if (sound_banks)
						return 0xff00 | (UPD7759_0_busy_r(0) << 6) | 0xbf; /* must return high bit on */
					else
						return 0xffff;
		case 0x04:	return 0xff00 | readinputport(0);
		case 0x05:	return 0xff00 | readinputport(3);
		case 0x06:	return 0xff00 | readinputport(4);
		case 0x08:	return 0xff00 | 'S'; /* for Poto Poto */
		case 0x09:	return 0xff00 | 'E'; /* for Poto Poto */
		case 0x0a:	return 0xff00 | 'G'; /* for Poto Poto */
		case 0x0b:	return 0xff00 | 'A'; /* for Poto Poto */
		default:	return 0xff00 | iochip_reg[offset];
	}
	return 0xffff;
}


/* handle I/O chip writes */
static WRITE16_HANDLER( iochip_w )
{
	int newbank;

	/* only LSB matters */
	if (!ACCESSING_LSB)
		return;

	/* track what was written */
	iochip_reg[offset] = data;

	/* handle special writes */
	switch (offset)
	{
		case 0x03:
			/* Bit  4   = ??? */
			break;

		case 0x07:
			/* Bits 0-1 = master palette base - maps 1 of 4 1024-entry banks into paletteram */
			/* Bits 2-3 = UPD7759 sample bank - maps 1 of 4 128k banks into the UPD7759 space */
			/* Bit  4   = IRQ4 enable/acknowledge */
			/* Bit  5   = IRQ6 enable/acknowledge (?) */
			newbank = (data & 3) * 0x200;
			if (newbank != segac2_palbank)
			{
				segac2_update_display(cpu_getscanline() + 1);
				segac2_palbank = newbank;
			}
			if (sound_banks > 1)
			{
				newbank = (data >> 2) & (sound_banks - 1);
				UPD7759_set_bank_base(0, newbank * 0x20000);
			}
			break;

		case 0x0e:
			/* Bit  1   = YM3438 reset? no - breaks poto poto */
/*			if (!(data & 2))
				YM2612_sh_reset();*/
			break;

		case 0x0f:
			/* ???? */
			if (data != 0x88)
				if (LOG_IOCHIP) logerror("%06x:I/O write @ %02x = %02x\n", activecpu_get_previouspc(), offset, data & 0xff);
			break;

		default:
			if (LOG_IOCHIP) logerror("%06x:I/O write @ %02x = %02x\n", activecpu_get_previouspc(), offset, data & 0xff);
			break;
	}
}



/******************************************************************************
	Control Write Handler
*******************************************************************************

	Seems to control some global states. The most important bit is the low
	one, which enables/disables the display. This is used while tiles are
	being modified in Bloxeed.

******************************************************************************/

static WRITE16_HANDLER( control_w )
{
	/* skip if not LSB */
	if (!ACCESSING_LSB)
		return;
	data &= 0xff;

	/* bit 0 controls display enable */
	segac2_enable_display(~data & 1);

	/* log anything suspicious */
	if (data != 6 && data != 7)
		if (LOG_IOCHIP) logerror("%06x:control_w suspicious value = %02X (%d)\n", activecpu_get_previouspc(), data, cpu_getscanline());
}



/******************************************************************************
	Protection Read / Write Handlers
*******************************************************************************

	The protection chip is fairly simple. Writes to it control the palette
	banking for the sprites and backgrounds. The low 4 bits are also
	remembered in a 2-stage FIFO buffer. A read from this chip should
	return a value from a 256x4-bit table. The index into this table is
	computed by taking the second-to-last value written in the upper 4 bits,
	and the previously-fetched table value in the lower 4 bits.

******************************************************************************/

/* protection chip reads */
static READ16_HANDLER( prot_r )
{
	if (LOG_PROTECTION) logerror("%06X:protection r=%02X\n", activecpu_get_previouspc(), prot_table ? prot_read_buf : 0xff);
	return prot_read_buf | 0xf0;
}


/* protection chip writes */
static WRITE16_HANDLER( prot_w )
{
	int new_sp_palbase = ((data >> 2) & 3) * 0x40 + 0x100;
	int new_bg_palbase = (data & 3) * 0x40;
	int table_index;

	/* only works for the LSB */
	if (!ACCESSING_LSB)
		return;

	/* keep track of the last few writes */
	prot_write_buf = (prot_write_buf << 4) | (data & 0x0f);

	/* compute the table index */
	table_index = (prot_write_buf & 0xf0) | prot_read_buf;

	/* determine the value to return, should a read occur */
	if (prot_table)
		prot_read_buf = (prot_table[table_index >> 3] << (4 * (table_index & 7))) >> 28;
	if (LOG_PROTECTION) logerror("%06X:protection w=%02X, new result=%02X\n", activecpu_get_previouspc(), data & 0x0f, prot_read_buf);

	/* if the palette changed, force an update */
	if (new_sp_palbase != segac2_sp_palbase || new_bg_palbase != segac2_bg_palbase)
	{
		segac2_update_display(cpu_getscanline() + 1);
		segac2_sp_palbase = new_sp_palbase;
		segac2_bg_palbase = new_bg_palbase;
		if (LOG_PALETTE) logerror("Set palbank: %d/%d (scan=%d)\n", segac2_bg_palbase, segac2_sp_palbase, cpu_getscanline());
	}
}


/* special puyo puyo 2 chip reads */
static READ16_HANDLER( puyopuy2_prot_r )
{
	/* Puyo Puyo 2 uses the same protection mechanism, but the table isn't */
	/* encoded in a way that tells us the return values directly; this */
	/* function just feeds it what it wants */
	int table_index = (prot_write_buf & 0xf0) | ((prot_write_buf >> 8) & 0x0f);
	if (prot_table)
		prot_read_buf = (prot_table[table_index >> 3] << (4 * (table_index & 7))) >> 28;
	if (LOG_PROTECTION) logerror("%06X:protection r=%02X\n", activecpu_get_previouspc(), prot_table ? prot_read_buf : 0xff);
	return prot_read_buf | 0xf0;
}



/******************************************************************************
	Counter/timer I/O
*******************************************************************************

	There appears to be a chip that is used to count coins and track time
	played, or at the very least the current status of the game. All games
	except Puyo Puyo 2 and Poto Poto access this in a mostly consistent
	manner.

******************************************************************************/

static WRITE16_HANDLER( counter_timer_w )
{
	/* only LSB matters */
	if (ACCESSING_LSB)
	{
		/*int value = data & 1;*/
		switch (data & 0x1e)
		{
			case 0x00:	/* player 1 start/stop */
			case 0x02:	/* player 2 start/stop */
			case 0x04:	/* ??? */
			case 0x06:	/* ??? */
			case 0x08:	/* player 1 game timer? */
			case 0x0a:	/* player 2 game timer? */
			case 0x0c:	/* ??? */
			case 0x0e:	/* ??? */
				break;

			case 0x10:	/* coin counter */
				coin_counter_w(0,1);
				coin_counter_w(0,0);
				break;

			case 0x12:	/* set coinage info -- followed by two 4-bit values */
				break;

			case 0x14:	/* game timer? (see Tant-R) */
			case 0x16:	/* intro timer? (see Tant-R) */
			case 0x18:	/* ??? */
			case 0x1a:	/* ??? */
			case 0x1c:	/* ??? */
				break;

			case 0x1e:	/* reset */
				break;
		}
	}
}



/******************************************************************************
	NVRAM Handling
*******************************************************************************

	There is a battery on the board that keeps the high scores. However,
	simply saving the RAM doesn't seem to be good enough. This is still
	pending investigation.

******************************************************************************/

/*
static void nvram_handler(void *file, int read_or_write)
{
	int i;

	if (read_or_write)
		osd_fwrite(file, main_ram, 0x10000);
	else if (file)
		osd_fread(file, main_ram, 0x10000);
	else
		for (i = 0; i < 0x10000/2; i++)
			main_ram[i] = rand();
}
*/

/**************

Hiscores:

Bloxeed  @ f400-????			[key = ???]
Columns  @ fc00-ffff			[key = '(C) SEGA 1990.JAN BY.TAKOSUKEZOU' @ fc00,ffe0]
Columns2 @ fc00-ffff			[key = '(C) SEGA 1990.SEP.COLUMNS2 JAPAN' @ fc00,fd00,fe00,ffe0]
Borench  @ f400-f5ff			[key = 'EIJI' in last word]
TForceAC @ 8100-817f/8180-81ff	[key = '(c)Tehcno soft90' @ 8070 and 80f0]
TantR    @ fc00-fcff/fd00-fdff	[key = 0xd483 in last word]
PuyoPuyo @ fc00-fdff/fe00-ffff	[key = 0x28e1 in first word]
Ichidant @ fc00-fcff/fd00-fdff	[key = 0x85a9 in last word]
StkClmns @ fc00-fc7f/fc80-fcff	[key = ???]
PuyoPuy2
PotoPoto
ZunkYou

***************/



/******************************************************************************
	Memory Maps
*******************************************************************************

	The System C/C2 68k Memory map is fairly similar to the Genesis in terms
	of RAM, ROM, VDP access locations, although the differences between the
	arcade system and the Genesis means its not same.

	The information here has been worked out from the games, and there may
	be some uncertain things, for example Puyo Puyo 2 I believe accesses
	0x8C0400 - 0x8C0FFF which is outside of what is seeminly valid paletteram.

******************************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },					/* Main 68k Program Roms */
	{ 0x800000, 0x800001, prot_r },						/* The Protection Chip? */
	{ 0x840000, 0x84001f, iochip_r },					/* I/O Chip */
	{ 0x840100, 0x840107, ym3438_r },					/* Ym3438 Sound Chip Status Register */
	{ 0x8c0000, 0x8c0fff, palette_r },					/* Palette Ram */
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xff0000, 0xffffff, MRA16_RAM },					/* Main Ram */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },					/* Main 68k Program Roms */
	{ 0x800000, 0x800001, prot_w },						/* The Protection Chip? */
	{ 0x800200, 0x800201, control_w },					/* Seems to be global controls */
	{ 0x840000, 0x84001f, iochip_w },					/* I/O Chip */
	{ 0x840100, 0x840107, ym3438_w },					/* Ym3438 Sound Chip Writes */
	{ 0x880000, 0x880001, upd7759_w },					/* UPD7759 Sound Writes */
	{ 0x880134, 0x880135, counter_timer_w },			/* Bookkeeping */
	{ 0x880334, 0x880335, counter_timer_w },			/* Bookkeeping (mirror) */
	{ 0x8c0000, 0x8c0fff, palette_w, &paletteram16 },	/* Palette Ram */
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xff0000, 0xffffff, MWA16_RAM, &main_ram },		/* Main Ram */
MEMORY_END



/******************************************************************************
	Input Ports
*******************************************************************************

	The input ports on the C2 games always consist of 1 Coin Port, 2 Player
	Input ports and 2 Dipswitch Ports, 1 of those Dipswitch Ports being used
	for coinage, the other for Game Options.

	Most of the Games List the Dipswitchs and Inputs in the Test Menus, adding
	them is just a tedious task.  I think Columnns & Bloxeed are Exceptions
	and will need their Dipswitches working out by observation.  The Coin Part
	of the DSW's seems fairly common to all games.

******************************************************************************/

#define COINS \
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) \
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )

#define JOYSTICK_1 \
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) \
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) \
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )

#define JOYSTICK_2 \
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 ) \
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) \
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )

#define COIN_A \
    PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) ) \
    PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) ) \
    PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) ) \
    PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) ) \
    PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3 6/4" ) \
    PORT_DIPSETTING(    0x04, "2 Coins/1 Credit, 4/3" ) \
    PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) ) \
    PORT_DIPSETTING(    0x03, "1 Coin/1 Credit, 5/6" ) \
    PORT_DIPSETTING(    0x02, "1 Coin/1 Credit, 4/5" ) \
    PORT_DIPSETTING(    0x01, "1 Coin/1 Credit, 2/3" ) \
    PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) ) \
    PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) ) \
    PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) ) \
    PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) ) \
    PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) ) \
    PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) ) \
    PORT_DIPSETTING(    0x00, "1 Coin/1 Credit (Freeplay if Coin B also)" )

#define COIN_B \
    PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) ) \
    PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) ) \
    PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) ) \
    PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) ) \
    PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3 6/4" ) \
    PORT_DIPSETTING(    0x40, "2 Coins/1 Credit, 4/3" ) \
    PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) ) \
    PORT_DIPSETTING(    0x30, "1 Coin/1 Credit, 5/6" ) \
    PORT_DIPSETTING(    0x20, "1 Coin/1 Credit, 4/5" ) \
    PORT_DIPSETTING(    0x10, "1 Coin/1 Credit, 2/3" ) \
    PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) ) \
    PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) ) \
    PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) ) \
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) ) \
    PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) ) \
    PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit (Freeplay if Coin A also)" )


INPUT_PORTS_START( columns ) /* Columns Input Ports */
    PORT_START
    COINS
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )     // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )     // Button 2 Unused
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )     // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )    // Button 2 Unused
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )    // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )							// Game Options..
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    /* The first level increase (from 0 to 1) is allways after destroying
       35 jewels. Then, the leve gets 1 level more every : */
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x00, "Easy" )     // 50 jewels
    PORT_DIPSETTING(    0x10, "Medium" )   // 40 jewels
    PORT_DIPSETTING(    0x30, "Hard" )     // 35 jewels
    PORT_DIPSETTING(    0x20, "Hardest" )  // 25 jewels
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( columns2 ) /* Columns 2 Input Ports */
    PORT_START
    COINS
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )     // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )     // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )     // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )     // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )     // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )     // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "VS. Mode Credits/Match" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Flash Mode Difficulty" )
	PORT_DIPSETTING(    0x20, "Easy" )
    PORT_DIPSETTING(    0x30, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( borench ) /* Borench Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      // Button 'Set'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )      // Button 'Turbo'
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )       // Button 'Set'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )       // Button 'Turbo'
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )       // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Credits to Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x0c, 0x0c, "Lives 1P Mode" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
    PORT_DIPNAME( 0x30, 0x30, "Lives 2P Mode" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
    PORT_DIPSETTING(    0xc0, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END


INPUT_PORTS_START( tfrceac ) /* ThunderForce AC Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )      // Button Speed Change
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )      // Button Shot
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )      // Button Weapon Select
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )      // Button Speed Change
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      // Button Shot
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )      // Button Weapon Select
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Credits to Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30,  DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x10, "10k, 70k, 150k" )
    PORT_DIPSETTING(    0x30, "20k, 100k, 200k" )
    PORT_DIPSETTING(    0x20, "40k, 150k, 300k" )
    PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
    PORT_DIPSETTING(    0xc0, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END


INPUT_PORTS_START( puyopuyo ) /* PuyoPuyo Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )        // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )        // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )      // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )      // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "VS. Mode Credits/Match" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x18, 0x18, "1P Mode Difficulty" )
	PORT_DIPSETTING(    0x10, "Easy" )
    PORT_DIPSETTING(    0x18, "Medium" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Moving Seat" )
	PORT_DIPSETTING(    0x80, "No Use" )
	PORT_DIPSETTING(    0x00, "In Use" )
INPUT_PORTS_END


INPUT_PORTS_START( stkclmns ) /* Stack Columns Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )      // Button 'Attack'
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )      // Button 'Attack'
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )      // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Match Mode Price" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( potopoto ) /* PotoPoto Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )     // Button 'Bomb'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )     // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )     // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    // Button 'Bomb'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )    // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )    // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Credits to Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Chute Type" )
	PORT_DIPSETTING(    0x04, "Common" )
	PORT_DIPSETTING(    0x00, "Individual" )
	PORT_DIPNAME( 0x08, 0x08, "Credits to Continue" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, "Buy-In" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
    PORT_DIPSETTING(    0x60, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, "Moving Seat" )
	PORT_DIPSETTING(    0x80, "No Use" )
	PORT_DIPSETTING(    0x00, "In Use" )
INPUT_PORTS_END


INPUT_PORTS_START( zunkyou ) /* ZunkYou Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )       // Button 'Shot'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )       // Button 'Bomb'
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )       // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      // Button 'Shot'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )      // Button 'Bomb'
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )      // Button 3 Unused
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Game Difficulty 1" )
    PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x02, 0x02, "Game Difficulty 2" )
    PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( ichidant ) /*  Ichidant-R and Tant-R Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )      // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )    // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )    // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x04, "Easy" )
    PORT_DIPSETTING(    0x06, "Medium" )
    PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( bloxeedc ) /*  Bloxeed Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )      // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )     // Button 'Rotate'
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )     // Button 2 Unused == Button 1
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )     // Button 3 Unused == Button 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, "VS Mode Price" )
    PORT_DIPSETTING(    0x00, "Same as Ordinary" )
    PORT_DIPSETTING(    0x01, "Double as Ordinary" )
    PORT_DIPNAME( 0x02, 0x02, "Credits to Start" )
    PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( puyopuy2 ) /*  Puyo Puyo 2 Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )   // Rotate clockwise
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )   // Rotate anti-clockwise. Can be inverted using the dips
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )   // Button 3 Unused  _NOT_ Rannyu which is Start 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, "Rannyu Off Button" )
    PORT_DIPSETTING(    0x01, "Use" )
    PORT_DIPSETTING(    0x00, "No Use" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "Turn Direction" )
    PORT_DIPSETTING(    0x04, "1:Right  2:Left" )
    PORT_DIPSETTING(    0x00, "1:Left  2:Right")
    PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x10, "Easy" )
    PORT_DIPSETTING(    0x18, "Medium" )
    PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x60, 0x60, "VS Mode Match/1 Play" )
    PORT_DIPSETTING(    0x60, "1" )
    PORT_DIPSETTING(    0x40, "2" )
    PORT_DIPSETTING(    0x20, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPNAME( 0x80, 0x80, "Battle Start credit" )
    PORT_DIPSETTING(    0x00, "1" )
    PORT_DIPSETTING(    0x80, "2" )
INPUT_PORTS_END



/******************************************************************************
	Sound interfaces
******************************************************************************/

static struct UPD7759_interface upd7759_intf =
{
	1,								/* One chip */
	{ 50 },							/* Volume */
	{ REGION_SOUND1 },				/* Memory pointer (gen.h) */
	UPD7759_STANDALONE_MODE			/* Chip mode */
};

static struct YM2612interface ym3438_intf =
{
	1,								/* One chip */
	MASTER_CLOCK/7,					/* Clock: 7.67 MHz */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },	/* Volume */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ ym3438_interrupt }			/* IRQ handler */
};

static struct SN76496interface sn76489_intf =
{
	1,								/* One chip */
	{ MASTER_CLOCK/15 },			/* Clock: 3.58 MHz */
	{ 50 }							/* Volume */
};



/******************************************************************************
	Machine Drivers
*******************************************************************************

	General Overview
		M68000 @ 10MHz (Main Processor)
		YM3438 (Fm Sound)
		SN76489 (PSG, Noise, Part of the VDP)
		UPD7759 (Sample Playback, C-2 Only)

******************************************************************************/

static MACHINE_DRIVER_START( segac )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, MASTER_CLOCK/7) 		/* yes, there is a divide-by-7 circuit */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vblank_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((262. - 224.) / 262.) * 1000000. / 60.))

	MDRV_MACHINE_INIT(segac2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(336,224)
	MDRV_VISIBLE_AREA(8, 327, 0, 223)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(segac2)
	MDRV_VIDEO_EOF(segac2)
	MDRV_VIDEO_UPDATE(segac2)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2612, ym3438_intf)
	MDRV_SOUND_ADD(SN76496, sn76489_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( segac2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, MASTER_CLOCK/7) 		/* yes, there is a divide-by-7 circuit */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vblank_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((262. - 224.) / 262.) * 1000000. / 60.))

	MDRV_MACHINE_INIT(segac2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(336,224)
	MDRV_VISIBLE_AREA(8, 327, 0, 223)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(segac2)
	MDRV_VIDEO_EOF(segac2)
	MDRV_VIDEO_UPDATE(segac2)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2612, ym3438_intf)
	MDRV_SOUND_ADD(SN76496, sn76489_intf)
	MDRV_SOUND_ADD(UPD7759, upd7759_intf)
MACHINE_DRIVER_END



/******************************************************************************
	Rom Definitions
*******************************************************************************

	All the known System C/C2 Dumps are listed here with the exception of
	the version of Puzzle & Action (I believe its actually Ichidant-R) which
	was credited to SpainDumps in the included text file.  This appears to be
	a bad dump (half sized roms) however the roms do not match up exactly with
	the good dump of the game.  English language sets are assumed to be the
	parent where they exist.  Hopefully some more alternate version dumps will
	turn up sometime soon for example English Language version of Tant-R or
	Japanese Language versions of Borench (if of course these games were
	released in other locations.

	Games are in Order of Date (Year) with System-C titles coming first.

******************************************************************************/

/* ----- System C Games ----- */

ROM_START( bloxeedc ) /* Bloxeed (C System Version)  (c)1989 Sega / Elorg */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr12908.32", 0x000000, 0x020000, 0xfc77cb91 )
	ROM_LOAD16_BYTE( "epr12907.31", 0x000001, 0x020000, 0xe5fcbac6 )
	ROM_LOAD16_BYTE( "epr12993.34", 0x040000, 0x020000, 0x487bc8fc )
	ROM_LOAD16_BYTE( "epr12992.33", 0x040001, 0x020000, 0x19b0084c )
ROM_END


ROM_START( columns ) /* Columns (US) (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13114.32", 0x000000, 0x020000, 0xff78f740 )
	ROM_LOAD16_BYTE( "epr13113.31", 0x000001, 0x020000, 0x9a426d9b )
ROM_END


ROM_START( columnsj ) /* Columns (Jpn) (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13112.32", 0x000000, 0x020000, 0xbae6e53e )
	ROM_LOAD16_BYTE( "epr13111.31", 0x000001, 0x020000, 0xaa5ccd6d )
ROM_END


ROM_START( columns2 ) /* Columns II - The Voyage Through Time (Jpn)  (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13361.rom", 0x000000, 0x020000, 0xb54b5f12 )
	ROM_LOAD16_BYTE( "epr13360.rom", 0x000001, 0x020000, 0xa59b1d4f )
ROM_END

ROM_START( tantrbl2 ) /* Tant-R (Puzzle & Action) (Alt Bootleg Running on C Board?, No Samples) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "trb2_2.32",    0x000000, 0x080000, 0x8fc99c48 )
	ROM_LOAD16_BYTE( "trb2_1.31",    0x000001, 0x080000, 0xc318d00d )
	ROM_LOAD16_BYTE( "mpr15616.34",  0x100000, 0x080000, 0x17b80202 )
	ROM_LOAD16_BYTE( "mpr15615.33",  0x100001, 0x080000, 0x36a88bd4 )
ROM_END

/* ----- System C-2 Games ----- */

ROM_START( borench ) /* Borench  (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ic32.bin", 0x000000, 0x040000, 0x2c54457d )
	ROM_LOAD16_BYTE( "ic31.bin", 0x000001, 0x040000, 0xb46445fc )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic4.bin", 0x000000, 0x020000, 0x62b85e56 )
ROM_END


ROM_START( tfrceac ) /* ThunderForce AC  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ic32.bin", 0x000000, 0x040000, 0x95ecf202 )
	ROM_LOAD16_BYTE( "ic31.bin", 0x000001, 0x040000, 0xe63d7f1a )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "ic34.bin", 0x100000, 0x040000, 0x29f23461 )
	ROM_LOAD16_BYTE( "ic33.bin", 0x100001, 0x040000, 0x9e23734f )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic4.bin", 0x000000, 0x040000, 0xe09961f6 )
ROM_END


ROM_START( tfrceacj ) /* ThunderForce AC (Jpn)  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13657.32", 0x000000, 0x040000, 0xa0f38ffd )
	ROM_LOAD16_BYTE( "epr13656.31", 0x000001, 0x040000, 0xb9438d1e )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "ic34.bin",    0x100000, 0x040000, 0x29f23461 )
	ROM_LOAD16_BYTE( "ic33.bin",    0x100001, 0x040000, 0x9e23734f )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic4.bin", 0x000000, 0x040000, 0xe09961f6 )
ROM_END


ROM_START( tfrceacb ) /* ThunderForce AC (Bootleg)  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "4.bin",    0x000000, 0x040000, 0xeba059d3 )
	ROM_LOAD16_BYTE( "3.bin",    0x000001, 0x040000, 0x3e5dc542 )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "ic34.bin", 0x100000, 0x040000, 0x29f23461 )
	ROM_LOAD16_BYTE( "ic33.bin", 0x100001, 0x040000, 0x9e23734f )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "2.bin", 0x000000, 0x020000, 0x00000000 )
	ROM_LOAD( "1.bin", 0x020000, 0x020000, 0x4e2ca65a )
ROM_END


ROM_START( tantr ) /* Tant-R (Puzzle & Action)  (c)1992 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr15614.32", 0x000000, 0x080000, 0x557782bc )
	ROM_LOAD16_BYTE( "epr15613.31", 0x000001, 0x080000, 0x14bbb235 )
	ROM_LOAD16_BYTE( "mpr15616.34", 0x100000, 0x080000, 0x17b80202 )
	ROM_LOAD16_BYTE( "mpr15615.33", 0x100001, 0x080000, 0x36a88bd4 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr15617.4", 0x000000, 0x040000, 0x338324a1 )
ROM_END


ROM_START( tantrbl ) /* Tant-R (Puzzle & Action) (Bootleg)  (c)1992 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pa_e10.bin",  0x000000, 0x080000, 0x6c3f711f )
	ROM_LOAD16_BYTE( "pa_f10.bin",  0x000001, 0x080000, 0x75526786 )
	ROM_LOAD16_BYTE( "mpr15616.34", 0x100000, 0x080000, 0x17b80202 )
	ROM_LOAD16_BYTE( "mpr15615.33", 0x100001, 0x080000, 0x36a88bd4 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "pa_e03.bin", 0x000000, 0x020000, 0x72918c58 )
	ROM_LOAD( "pa_e02.bin", 0x020000, 0x020000, 0x4e85b2a3 )
ROM_END


ROM_START( puyopuyo	) /* Puyo Puyo  (c)1992 Sega / Compile */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr15036", 0x000000, 0x020000, 0x5310ca1b )
	ROM_LOAD16_BYTE( "epr15035", 0x000001, 0x020000, 0xbc62e400 )
	/* 0x040000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "epr15038", 0x100000, 0x020000, 0x3b9eea0c )
	ROM_LOAD16_BYTE( "epr15037", 0x100001, 0x020000, 0xbe2f7974 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr15034", 0x000000, 0x020000, 0x5688213b )
ROM_END


ROM_START( puyopuya	) /* Puyo Puyo (Rev A)  (c)1992 Sega / Compile */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep15036a.32", 0x000000, 0x020000, 0x61b35257 )
	ROM_LOAD16_BYTE( "ep15035a.31", 0x000001, 0x020000, 0xdfebb6d9 )
	/* 0x040000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "epr15038",    0x100000, 0x020000, 0x3b9eea0c )
	ROM_LOAD16_BYTE( "epr15037",    0x100001, 0x020000, 0xbe2f7974 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr15034", 0x000000, 0x020000, 0x5688213b )
ROM_END

ROM_START( puyopuyb ) /* Puyo Puyo  (c)1992 Sega / Compile  Bootleg */
 ROM_REGION( 0x200000, REGION_CPU1, 0 )
 ROM_LOAD16_BYTE( "puyopuyb.4bo", 0x000000, 0x020000, 0x89ea4d33 )
 ROM_LOAD16_BYTE( "puyopuyb.3bo", 0x000001, 0x020000, 0xc002e545 )
 /* 0x040000 - 0x100000 Empty */
 ROM_LOAD16_BYTE( "puyopuyb.6bo", 0x100000, 0x020000, 0xa0692e5 )
 ROM_LOAD16_BYTE( "puyopuyb.5bo", 0x100001, 0x020000, 0x353109b8 )

 ROM_REGION( 0x020000, REGION_SOUND1, 0 )
 ROM_LOAD( "puyopuyb.abo", 0x000000, 0x020000, 0x79112b3b )
ROM_END

ROM_START( ichidant ) /* Ichident-R (Puzzle & Action 2)  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16886", 0x000000, 0x080000, 0x38208e28 )
	ROM_LOAD16_BYTE( "epr16885", 0x000001, 0x080000, 0x1ce4e837 )
	ROM_LOAD16_BYTE( "epr16888", 0x100000, 0x080000, 0x85d73722 )
	ROM_LOAD16_BYTE( "epr16887", 0x100001, 0x080000, 0xbc3bbf25 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16884", 0x000000, 0x080000, 0xfd9dcdd6)
ROM_END


ROM_START( ichidnte ) /* Ichident-R (Puzzle & Action 2)  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pa2_32.bin", 0x000000, 0x080000, 0x7ba0c025 )
	ROM_LOAD16_BYTE( "pa2_31.bin", 0x000001, 0x080000, 0x5f86e5cc )
	ROM_LOAD16_BYTE( "epr16888",   0x100000, 0x080000, 0x85d73722 )
	ROM_LOAD16_BYTE( "epr16887",   0x100001, 0x080000, 0xbc3bbf25 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "pa2_02.bin", 0x000000, 0x080000, 0xfc7b0da5 )
ROM_END


ROM_START( stkclmns ) /* Stack Columns  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16795.32", 0x000000, 0x080000, 0xb478fd02 )
	ROM_LOAD16_BYTE( "epr16794.31", 0x000001, 0x080000, 0x6d0e8c56 )
	ROM_LOAD16_BYTE( "mpr16797.34", 0x100000, 0x080000, 0xb28e9bd5 )
	ROM_LOAD16_BYTE( "mpr16796.33", 0x100001, 0x080000, 0xec7de52d )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16793.4", 0x000000, 0x020000, 0xebb2d057 )
ROM_END


ROM_START( puyopuy2 ) /* Puyo Puyo 2  (c)1994 Compile */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pp2.eve", 0x000000, 0x080000, 0x1cad1149 )
	ROM_LOAD16_BYTE( "pp2.odd", 0x000001, 0x080000, 0xbeecf96d )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "pp2.snd", 0x000000, 0x080000, 0x020ff6ef )
ROM_END


ROM_START( potopoto ) /* Poto Poto  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16662", 0x000000, 0x040000, 0xbbd305d6 )
	ROM_LOAD16_BYTE( "epr16661", 0x000001, 0x040000, 0x5a7d14f4 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16660", 0x000000, 0x040000, 0x8251c61c )
ROM_END


ROM_START( zunkyou ) /* Zunzunkyou No Yabou  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16812.32", 0x000000, 0x080000, 0xeb088fb0 )
	ROM_LOAD16_BYTE( "epr16811.31", 0x000001, 0x080000, 0x9ac7035b )
	ROM_LOAD16_BYTE( "epr16814.34", 0x100000, 0x080000, 0x821b3b77 )
	ROM_LOAD16_BYTE( "epr16813.33", 0x100001, 0x080000, 0x3cba9e8f )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16810.4", 0x000000, 0x080000, 0xd542f0fe )
ROM_END



/******************************************************************************
	Machine Init Functions
*******************************************************************************

	All of the Sega C/C2 games apart from Bloxeed used a protection chip.
	The games contain various checks which make sure this protection chip is
	present and returning the expected values.  The chip uses a table of
	256x4-bit values to produce its results.  It appears that different
	tables are used for Japanese vs. English variants of some games
	(Puzzle & Action 2) but not others (Columns).

******************************************************************************/

static void init_saves(void)
{
	/* Do we need the int states ? */
	state_save_register_UINT8 ("C2_main", 0, "Int 2 Status", &ym3438_int, 1);
	state_save_register_UINT8 ("C2_main", 0, "Int 4 Status", &scanline_int, 1);
	state_save_register_UINT8 ("C2_main", 0, "Int 6 Status", &vblank_int, 1);

	state_save_register_UINT8 ("C2_IO", 0, "I/O Writes", iochip_reg, 0x10);

	state_save_register_UINT16 ("C2 Protection", 0, "Write Buffer", &prot_write_buf, 1);
	state_save_register_UINT16 ("C2 Protection", 0, "Read Buffer", &prot_read_buf, 1);
}

static DRIVER_INIT( segac2 )
{
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( bloxeedc )
{
	init_saves();
	bloxeed_sound = 1;
}

static DRIVER_INIT( columns )
{
	static const UINT32 columns_table[256/8] =
	{
		0x20a41397, 0x64e057d3, 0x20a41397, 0x64e057d3,
		0x20a41397, 0x64e057d3, 0xa8249b17, 0xec60df53,
		0x20a41397, 0x64e057d3, 0x75f546c6, 0x31b10282,
		0x20a41397, 0x64e057d3, 0xfd75ce46, 0xb9318a02,
		0xb8348b07, 0xfc70cf43, 0xb8348b07, 0xfc70cf43,
		0x9a168b07, 0xde52cf43, 0x9a168b07, 0xde52cf43,
		0x30b40387, 0x74f047c3, 0x75f546c6, 0x31b10282,
		0x30b40387, 0x74f047c3, 0xfd75ce46, 0xb9318a02
	};
	prot_table = columns_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( columns2 )
{
	static const UINT32 columns2_table[256/8] =
	{
		0x0015110c, 0x0015110c, 0x889d9984, 0xcedb9b86,
		0x4455554c, 0x4455554c, 0xddddccc4, 0x9b9bcec6,
		0x2237332e, 0x2237332e, 0x6677776e, 0x2031756c,
		0x6677776e, 0x6677776e, 0x7777666e, 0x3131646c,
		0x0015110c, 0x0015110c, 0x889d9984, 0xcedb9b86,
		0x6677776e, 0x6677776e, 0xffffeee6, 0xb9b9ece4,
		0xaabfbba6, 0xaabfbba6, 0xeeffffe6, 0xa8b9fde4,
		0xeeffffe6, 0xeeffffe6, 0xffffeee6, 0xb9b9ece4
	};
	prot_table = columns2_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( borench )
{
	static const UINT32 borench_table[256/8] =
	{
		0x12fe56ba, 0x56ba56ba, 0x00aa44ee, 0xcceeccee,
		0x13ff57bb, 0x759957bb, 0x11bb55ff, 0xffddddff,
		0x12ba56fe, 0x56fe56fe, 0x00aa44ee, 0xcceeccee,
		0x933bd77f, 0xf55dd77f, 0x913bd57f, 0x7f5d5d7f,
		0x12fe56ba, 0x56ba56ab, 0x00aa44ee, 0xcceeccff,
		0xd73bd73b, 0xf519d72a, 0xd57fd57f, 0x7f5d5d6e,
		0x12ba56fe, 0x56fe56ef, 0x00aa44ee, 0xcceeccff,
		0xd77fd77f, 0xf55dd76e, 0xd57fd57f, 0x7f5d5d6e
	};
	prot_table = borench_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( tfrceac )
{
	static const UINT32 tfrceac_table[256/8] =
	{
		0x3a3a6f6f, 0x38386d6d, 0x3a3a6f6f, 0x28287d7d,
		0x3a3a6f6f, 0x38386d6d, 0x3a3a6f6f, 0x28287d7d,
		0x7e3a2b6f, 0x7c38296d, 0x7eb22be7, 0x6ca039f5,
		0x7e3a2b6f, 0x7c38296d, 0x7eb22be7, 0x6ca039f5,
		0x3b3b6e6e, 0x39396c6c, 0x5dd50880, 0x4ec61b93,
		0x3b3b6e6e, 0x39396c6c, 0x3bb36ee6, 0x28a07df5,
		0x5d19084c, 0x5d19084c, 0x7ff72aa2, 0x6ee63bb3,
		0x5d19084c, 0x5d19084c, 0x5d9108c4, 0x4c8019d5
	};
	prot_table = tfrceac_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( tfrceacb )
{
	/* disable the palette bank switching from the protecton chip */
	install_mem_write16_handler(0, 0x800000, 0x800001, MWA16_NOP);
}

static DRIVER_INIT( tantr )
{
	static const UINT32 tantr_table[256/8] =
	{
		0x91ddd19d, 0x91ddd19d, 0xd4dc949c, 0xf6feb6be,
		0x91bbd1fb, 0x91bbd1fb, 0xd4fe94be, 0xf6feb6be,
		0x80cce2ae, 0x88cceaae, 0xc5cda7af, 0xefef8d8d,
		0x91bbf3d9, 0x99bbfbd9, 0xd4feb69c, 0xfefe9c9c,
		0x5d55959d, 0x5d55959d, 0x5c54949c, 0x7e76b6be,
		0x5d7795bf, 0x5d7795bf, 0x5c7694be, 0x7e76b6be,
		0x5d55b7bf, 0x4444aeae, 0x5c54b6be, 0x67678d8d,
		0x5d77b79d, 0x5577bf9d, 0x5c76b69c, 0x76769c9c
	};
	prot_table = tantr_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( ichidant )
{
	static const UINT32 ichidant_table[256/8] =
	{
		0x55116622, 0x55116622, 0x55117733, 0x55117733,
		0x8800aa22, 0x8800aa22, 0x8800bb33, 0x8800bb33,
		0x11550044, 0x55114400, 0x11551155, 0x55115511,
		0xcc44cc44, 0x88008800, 0xcc44dd55, 0x88009911,
		0xdd99eeaa, 0xdd99eeaa, 0xdd99ffbb, 0xdd99ffbb,
		0xaa228800, 0xaa228800, 0xaa229911, 0xaa229911,
		0x99dd88cc, 0xdd99cc88, 0x99dd99dd, 0xdd99dd99,
		0xee66ee66, 0xaa22aa22, 0xee66ff77, 0xaa22bb33
	};
	prot_table = ichidant_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( ichidnte )
{
	static const UINT32 ichidnte_table[256/8] =
	{
		0x4c4c4c4c, 0x08080808, 0x5d5d4c4c, 0x19190808,
		0x33332222, 0x33332222, 0x22222222, 0x22222222,
		0x082a082a, 0x082a082a, 0x193b082a, 0x193b082a,
		0x77556644, 0x33112200, 0x66446644, 0x22002200,
		0x6e6e6e6e, 0x2a2a2a2a, 0x7f7f6e6e, 0x3b3b2a2a,
		0xbbbbaaaa, 0xbbbbaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
		0x2a082a08, 0x2a082a08, 0x3b192a08, 0x3b192a08,
		0xffddeecc, 0xbb99aa88, 0xeecceecc, 0xaa88aa88
	};
	prot_table = ichidnte_table;
	bloxeed_sound = 0;
	init_saves();
}


static DRIVER_INIT( potopoto )
{
	/* note: this is not the real table; Poto Poto only tests one  */
	/* very specific case, so we don't have enough data to provide */
	/* the correct table in its entirety */
	static const UINT32 potopoto_table[256/8] =
	{
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x22222222, 0x22222222,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	prot_table = potopoto_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( puyopuyo )
{
	static const UINT32 puyopuyo_table[256/8] =
	{
		0x33aa55cc, 0x33aa55cc, 0xba22fe66, 0xba22fe66,
		0x77ee55cc, 0x55cc77ee, 0xfe66fe66, 0xdc44dc44,
		0x33aa77ee, 0x77aa33ee, 0xba22fe66, 0xfe22ba66,
		0x77ee77ee, 0x11cc11cc, 0xfe66fe66, 0x98449844,
		0x22bb44dd, 0x3ba25dc4, 0xab33ef77, 0xba22fe66,
		0x66ff44dd, 0x5dc47fe6, 0xef77ef77, 0xdc44dc44,
		0x22bb66ff, 0x7fa23be6, 0xab33ef77, 0xfe22ba66,
		0x66ff66ff, 0x19c419c4, 0xef77ef77, 0x98449844
	};
	prot_table = puyopuyo_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( puyopuy2 )
{
	/* note: this is not the real table; Puyo Puyo 2 doesn't  */
	/* store the original table; instead it loops through all */
	/* combinations 0-255 and expects the following results;  */
	/* to work around this, we install a custom read handler  */
	static const UINT32 puyopuy2_table[256/8] =
	{
		0x00008383, 0xb3b33030, 0xcccc4f4f, 0x7f7ffcfc,
		0x02028181, 0xb1b13232, 0xcece4d4d, 0x7d7dfefe,
		0x4444c1c1, 0x91911414, 0x99991c1c, 0x4c4cc9c9,
		0x4646c3c3, 0x93931616, 0x9b9b1e1e, 0x4e4ecbcb,
		0x5555d7d7, 0xf7f77575, 0xdddd5f5f, 0x7f7ffdfd,
		0x5757d5d5, 0xf5f57777, 0xdfdf5d5d, 0x7d7dffff,
		0x11119595, 0xd5d55151, 0x88880c0c, 0x4c4cc8c8,
		0x13139797, 0xd7d75353, 0x8a8a0e0e, 0x4e4ecaca
	};
	prot_table = puyopuy2_table;
	bloxeed_sound = 0;

	install_mem_read16_handler(0, 0x800000, 0x800001, puyopuy2_prot_r);
	init_saves();
}

static DRIVER_INIT( stkclmns )
{
	static const UINT32 stkclmns_table[256/8] =
	{
		0xcc88cc88, 0xcc88cc88, 0xcc99cc99, 0xcc99cc99,
		0x00001111, 0x88889999, 0x00111100, 0x88999988,
		0xaaee88cc, 0xeeaacc88, 0xaaff88dd, 0xeebbcc99,
		0x66665555, 0xaaaa9999, 0x66775544, 0xaabb9988,
		0xeeaaeeaa, 0xeeaaeeaa, 0xeebbeebb, 0xeebbeebb,
		0x00001111, 0x88889999, 0x00111100, 0x88999988,
		0x00442266, 0x44006622, 0x00552277, 0x44116633,
		0xeeeedddd, 0x22221111, 0xeeffddcc, 0x22331100
	};
	prot_table = stkclmns_table;
	bloxeed_sound = 0;

	/* until the battery RAM is understood, we must fill RAM with */
	/* random values so that the high scores are properly reset at */
	/* startup */
	{
		int i;
		for (i = 0; i < 0x10000/2; i++)
			main_ram[i] = rand();
	}
	init_saves();
}

static DRIVER_INIT( zunkyou )
{
	static const UINT32 zunkyou_table[256/8] =
	{
		0xa0a06c6c, 0x82820a0a, 0xecec2020, 0xecec6464,
		0xa2a26e6e, 0x80800808, 0xaaaa6666, 0xaaaa2222,
		0x39287d6c, 0x1b0a1b0a, 0x75643120, 0x75647564,
		0x3b2a7f6e, 0x19081908, 0x33227766, 0x33223322,
		0xb1b17d7d, 0x93931b1b, 0xfdfd3131, 0xfdfd7575,
		0xa2a26e6e, 0x80800808, 0xaaaa6666, 0xaaaa2222,
		0x28396c7d, 0x0a1b0a1b, 0x64752031, 0x64756475,
		0x3b2a7f6e, 0x19081908, 0x33227766, 0x33223322
	};
	prot_table = zunkyou_table;
	bloxeed_sound = 0;
	init_saves();
}



/******************************************************************************
	Game Drivers
*******************************************************************************

	These cover all the above games.

	Dates are all verified correct from Ingame display, some of the Titles
	such as Ichidant-R, Tant-R might be slightly incorrect as I've seen the
	games refered to by other names such as Ichident-R, Tanto-R, Tanto Arle
	etc.

	bloxeedc is set as as clone of bloxeed as it is the same game but running
	on a different piece of hardware.  The parent 'bloxeed' is a system18 game
	and does not currently work due to it being encrypted.

******************************************************************************/

/* System C Games */
GAME ( 1989, bloxeedc, bloxeed,  segac,  bloxeedc, bloxeedc, ROT0, "Sega / Elorg",           "Bloxeed (C System)" )
GAME ( 1990, columns,  0,        segac,  columns,  columns,  ROT0, "Sega",                   "Columns (US)" )
GAME ( 1990, columnsj, columns,  segac,  columns,  columns,  ROT0, "Sega",                   "Columns (Japan)" )
GAME ( 1990, columns2, 0,        segac,  columns2, columns2, ROT0, "Sega",                   "Columns II - The Voyage Through Time (Japan)" )

/* System C-2 Games */
GAME ( 1990, borench,  0,        segac2, borench,  borench,  ROT0, "Sega",                   "Borench" )
GAME ( 1990, tfrceac,  0,        segac2, tfrceac,  tfrceac,  ROT0, "Sega / Technosoft",      "ThunderForce AC" )
GAME ( 1990, tfrceacj, tfrceac,  segac2, tfrceac,  tfrceac,  ROT0, "Sega / Technosoft",      "ThunderForce AC (Japan)" )
GAME ( 1990, tfrceacb, tfrceac,  segac2, tfrceac,  tfrceacb, ROT0, "bootleg",                "ThunderForce AC (bootleg)" )
GAME ( 1992, tantr,    0,        segac2, ichidant, tantr,    ROT0, "Sega",                   "Tant-R (Puzzle & Action) (Japan)" )
GAME ( 1992, tantrbl,  tantr,    segac2, ichidant, segac2,   ROT0, "bootleg",                "Tant-R (Puzzle & Action) (Japan) (bootleg set 1)" )
GAME ( 1994, tantrbl2, tantr,    segac,  ichidant, tantr,    ROT0, "bootleg",                "Tant-R (Puzzle & Action) (Japan) (bootleg set 2)" )
GAME ( 1992, puyopuyo, 0,        segac2, puyopuyo, puyopuyo, ROT0, "Sega / Compile",         "Puyo Puyo (Japan)" )
GAME ( 1992, puyopuya, puyopuyo, segac2, puyopuyo, puyopuyo, ROT0, "Sega / Compile",         "Puyo Puyo (Japan) (Rev A)" )
GAME ( 1992, puyopuyb, puyopuyo, segac2, puyopuyo, puyopuyo, ROT0, "bootleg",                "Puyo Puyo (English) (bootleg)" )
GAME ( 1994, ichidant, 0,        segac2, ichidant, ichidant, ROT0, "Sega",                   "Ichidant-R (Puzzle & Action 2) (Japan)" )
GAME ( 1994, ichidnte, ichidant, segac2, ichidant, ichidnte, ROT0, "Sega",                   "Ichidant-R (Puzzle & Action 2) (English)" )
GAME ( 1994, stkclmns, 0,        segac2, stkclmns, stkclmns, ROT0, "Sega",                   "Stack Columns (Japan)" )
GAME ( 1994, puyopuy2, 0,        segac2, puyopuy2, puyopuy2, ROT0, "Compile (Sega license)", "Puyo Puyo 2 (Japan)" )
GAME ( 1994, potopoto, 0,        segac2, potopoto, potopoto, ROT0, "Sega",                   "Poto Poto (Japan)" )
GAME ( 1994, zunkyou,  0,        segac2, zunkyou,  zunkyou,  ROT0, "Sega",                   "Zunzunkyou No Yabou (Japan)" )
