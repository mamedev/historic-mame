/****************************************************************************
Supported games:
	Twin Cobra (World)
	Twin Cobra (USA license)
	Kyukyoku Tiger (Japan license)

Difference between Twin Cobra and Kyukyoko Tiger:
	T.C. supports two simultaneous players.
	K.T. supports two players, but only one at a time.
		 for this reason, it also supports Table Top cabinets.
	T.C. stores 3 characters for high scores.
	K.T. stores 6 characters for high scores.
	T.C. heros are Red and Blue for player 1 and 2 respectively.
	K.T. heros are grey for both players.
	T.C. dead remains of ground tanks are circular.
	K.T. dead remains of ground tanks always vary in shape.
	T.C. does not use DSW1-1 and DSW2-8.
	K.T. uses DSW1-1 for cabinet type, and DSW2-8 for allow game continue.
	T.C. continues new hero and continued game at current position.
	K.T. continues new hero and continued game at predefined positions.
		 After dying, and your new hero appears, if you do not travel more
		 than your helicopter length forward, you are penalised and moved
		 back further when your next hero appears.
	K.T. Due to this difference in continue sequence, Kyukyoko Tiger is MUCH
		 harder, challenging, and nearly impossible to complete !

68000:

00000-2ffff ROM
30000-33fff RAM shared with TMS320C10NL-14 protection microcontroller
40000-40fff RAM sprite character display properties (co-ordinates, character, color - etc)
50000-50dff Palette RAM
7a000-7abff RAM shared with Z80; 16-bit on this side, 8-bit on Z80 side

read:
78000		Player 1 Joystick and Buttons input port  (Flying shark)
78002		Player 2 Joystick and Buttons input port  (Flying shark)

78004		Player 1 Joystick and Buttons input port  (Twin Cobra)
78006		Player 2 Joystick and Buttons input port  (Twin Cobra)
78009		bit 7 vblank
7e000-7e005 read data from video RAM (see below)

write:
60000-60003 CRT 6845 controller. 0 = register offset , 2 = register data
70000-70001 scroll   y   for character page (centre normally 0x01c9)
70002-70003 scroll < x > for character page (centre normally 0x00e2)
70004-70005 offset in character page to write character (7e000)

72000-72001 scroll   y   for foreground page (starts from     0x03c9)
72002-72003 scroll < x > for foreground page (centre normally 0x002a)
72004-72005 offset in character page to write character (7e002)

74000-74001 scroll   y   for background page (starts from     0x03c9)
74002-74003 scroll < x > for background page (centre normally 0x002a)
74004-74005 offset in character page to write character (7e004)

76000-76003 as above but for another layer maybe ??? (Not used here)
7800a		see 7800c, except this activates INT line for Flying shark.
7800c		Control register (Byte write access).
			bits 7-4 always 0
			bits 3-1 select the control signal to drive.
			bit   0  is the value passed to the control signal.

			Value (hex):
			00-03	????
			04		Clear IPL2 line to 68000 inactive hi (Interrupt priority 4)
			05		Set   IPL2 line to 68000 active  low (Interrupt priority 4)
			06		Dont flip display
			07		Flip display
			08		Switch to background layer ram bank 0
			09		Switch to background layer ram bank 1
			0A		Switch to foreground layer rom bank 0
			0B		Switch to foreground layer rom bank 1
			0C		Activate INTerrupt line to the TMS320C10 DSP.
			0D		Inhibit  INTerrupt line to the TMS320C10 DSP.
			0E		Turn screen off
			0F		Turn screen on

7e000-7e001 data to write in text video RAM (70000)
7e002-7e003 data to write in bg video RAM (72004)
7e004-7e005 data to write in fg video RAM (74004)

Z80:
0000-7fff ROM
8000-87ff shared with 68000; 8-bit on this side, 16-bit on 68000 side

in:
00		  YM3812 status
10		  Coin inputs and control/service inputs
40		  DSW1
50		  DSW2

out:
00		  YM3812 control
01		  YM3812 data
20		  ????


TMS320C10 DSP: Harvard type architecture. RAM and ROM on seperate data buses.
0000-07ff ROM (words)
0000-0090 Internal RAM (words).	Moved to 8000-8120 for MAME compatibility.
								View this memory in the debugger at 4000h

68K writes the following to $30000 to tell DSP to do the following:
Twin  Kyukyoku
Cobra Tiger
00		00	 do nothing
01		0C	 run self test, and report DSP ROM checksum		from 68K PC:23CA6
02		07	 control all enemy shots						from 68K PC:23BFA
04		0b	 start the enemy helicopters					from 68K PC:23C66
05		08	 check for colision with enemy fire ???			from 68K PC:23C20
06		09	 check for colision with enemy ???				from 68K PC:23C44
07		01	 control enemy helicopter shots					from 68K PC:23AB2
08		02	 control all ground enemy shots
0A		04	 read hero position and send enemy to it ?		from 68K PC:23B58

03		0A	\
09		03	 \ These functions within the DSPs never seem to be called ????
0B		05	 /
0C		06	/
*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/M68000/M68000.h"

/**************** Video stuff ******************/
int twincobr_60000_r(int offset);
void twincobr_60000_w(int offset,int data);
void twincobr_70004_w(int offset,int data);
int twincobr_7e000_r(int offset);
void twincobr_7e000_w(int offset,int data);
void twincobr_72004_w(int offset,int data);
int twincobr_7e002_r(int offset);
void twincobr_7e002_w(int offset,int data);
void twincobr_74004_w(int offset,int data);
int twincobr_7e004_r(int offset);
void twincobr_7e004_w(int offset,int data);
void twincobr_76004_w(int offset,int data);
void twincobr_txscroll_w(int offset,int data);
void twincobr_bgscroll_w(int offset,int data);
void twincobr_fgscroll_w(int offset,int data);
int twincobr_vh_start(void);
void twincobr_vh_stop(void);
void twincobr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/**************** Machine stuff ******************/
int twincobr_dsp_in(int offset);
void twincobr_dsp_out(int fnction,int data);
int twincobr_68k_dsp_r(int offset);
void twincobr_68k_dsp_w(int offset,int data);
int twincobr_7800c_r(int offset);
void twincobr_7800c_w(int offset,int data);
int twincobr_sharedram_r(int offset);
void twincobr_sharedram_w(int offset,int data);

extern unsigned char *twincobr_68k_dsp_ram;
extern unsigned char *twincobr_7800c;
extern unsigned char *twincobr_sharedram;
extern int intenable;



int twincobr_input_r(int offset)
{
	return readinputport(1 + offset / 2);
}

int twincobr_interrupt(void)
{
	if (intenable) {
		intenable = 0;
		return MC68000_IRQ_4;
	}
	else return MC68000_INT_NONE;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x02ffff, MRA_ROM },
	{ 0x030000, 0x033fff, twincobr_68k_dsp_r, &twincobr_68k_dsp_ram },	/* 68K and DSP shared RAM */
	{ 0x040000, 0x040fff, MRA_BANK1 },				/* sprite ram data */
	{ 0x050000, 0x050dff, paletteram_word_r },
	{ 0x060000, 0x060003, twincobr_60000_r },
/*	{ 0x078000, 0x078003, twincobr_input_r }, */	/* Flying Shark - fshark */
	{ 0x078004, 0x078007, twincobr_input_r },
	{ 0x078008, 0x07800b, input_port_0_r }, 		/* vblank??? */
	{ 0x07800c, 0x07800f, twincobr_7800c_r, &twincobr_7800c },
	{ 0x07a000, 0x07abff, twincobr_sharedram_r },	/* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x07e000, 0x07e001, twincobr_7e000_r },		/* data from text video RAM */
	{ 0x07e002, 0x07e003, twincobr_7e002_r },		/* data from bg video RAM */
	{ 0x07e004, 0x07e005, twincobr_7e004_r },		/* data from fg video RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0x030000, 0x033fff, twincobr_68k_dsp_w, &twincobr_68k_dsp_ram },	/* 68K and DSP shared RAM */
	{ 0x040000, 0x040fff, MWA_BANK1, &spriteram, &spriteram_size },		/* sprite ram data */
	{ 0x050000, 0x050dff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x060000, 0x060003, twincobr_60000_w },		/* 6845 CRT controller */
	{ 0x070000, 0x070003, twincobr_txscroll_w },	/* scroll */
	{ 0x070004, 0x070005, twincobr_70004_w },		/* offset in text video RAM */
	{ 0x072000, 0x072003, twincobr_bgscroll_w },	/* scroll */
	{ 0x072004, 0x072005, twincobr_72004_w },		/* offset in bg video RAM */
	{ 0x074000, 0x074003, twincobr_fgscroll_w },	/* scroll */
	{ 0x074004, 0x074005, twincobr_74004_w },		/* offset in fg video RAM */
	{ 0x076000, 0x076004, twincobr_76004_w },       /* ???? */
/*	{ 0x078008, 0x07800b, twincobr_7800c_w }, */	/* Flying Shark - fshark */
	{ 0x07800c, 0x07800f, twincobr_7800c_w, &twincobr_7800c },
	{ 0x07a000, 0x07abff, twincobr_sharedram_w },	/* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x07e000, 0x07e001, twincobr_7e000_w },		/* data for text video RAM */
	{ 0x07e002, 0x07e003, twincobr_7e002_w },		/* data for bg video RAM */
	{ 0x07e004, 0x07e005, twincobr_7e004_w },		/* data for fg video RAM */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &twincobr_sharedram },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ 0x10, 0x10, input_port_3_r },
	{ 0x40, 0x40, input_port_4_r },
	{ 0x50, 0x50, input_port_5_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress DSP_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x8000, 0x811F, MRA_RAM },	/* The real DSP has this at address 0 */
									/* View this at 4000h in the debugger */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress DSP_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x8000, 0x811F, MWA_RAM },	/* The real DSP has this at address 0 */
									/* View this at 4000h in the debugger */
	{ -1 }	/* end of table */
};

static struct IOReadPort DSP_readport[] =
{
	{ 0x0000, 0x0001, twincobr_dsp_in },
	{ -1 }	/* end of table */
};
static struct IOWritePort DSP_writeport[] =
{
	{ 0x0000,  0x0003, twincobr_dsp_out },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )     /* ? could be wrong */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "50K, then every 150K" )
	PORT_DIPSETTING(    0x04, "70K, then every 200K" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( ktiger_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )     /* ? could be wrong */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet Type", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Up-right" )
	PORT_DIPSETTING(    0x00, "Table Top" )
	PORT_DIPNAME( 0x02, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0xc0, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Credits" )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "70K, then every 200K" )
	PORT_DIPSETTING(    0x04, "50K, then every 150K" )
	PORT_DIPSETTING(    0x08, "100000" )
	PORT_DIPSETTING(    0x0c, "No Extend" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,		/* 3 bits per pixel */
	{ 0*2048*8*8, 1*2048*8*8, 2*2048*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every char takes 8 consecutive bytes */
};

static struct GfxLayout fgtilelayout =
{
	8,8,	/* 8*8 tiles */
	8192,	/* 8192 tiles */
	4,		/* 4 bits per pixel */
	{ 0*8192*8*8, 1*8192*8*8, 2*8192*8*8, 3*8192*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every tile takes 8 consecutive bytes */
};

static struct GfxLayout bgtilelayout =
{
	8,8,	/* 8*8 tiles */
	4096,	/* 4096 tiles */
	4,		/* 4 bits per pixel */
	{ 0*4096*8*8, 1*4096*8*8, 2*4096*8*8, 3*4096*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every tile takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,		/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   1536, 32 },		/* colors 1536-1791 */
	{ 1, 0x0c000, &fgtilelayout, 1280, 16 },		/* colors 1280-1535 */
	{ 1, 0x4c000, &bgtilelayout, 1024, 16 },		/* colors 1024-1279 */
	{ 1, 0x6c000, &spritelayout,    0, 64 },		/* colors    0-1023 */
	{ -1 } /* end of array */
};



/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip  */
	3500000,		/* 3.5 MHz */
	{ 255 },		/* (not supported) */
	irqhandler,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7000000,			/* 7 MHz */
			0,
			readmem,writemem,0,0,
			twincobr_interrupt,1
		},
		{
			CPU_Z80,
			3500000,			/* 3.5 MHz  */
			2,					/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the YM3812 */
		},
		{
			CPU_TMS320C10,
			3500000,			/* 3.5 MHz  */
			3,					/* memory region #3 */
			DSP_readmem,DSP_writemem,DSP_readport,DSP_writeport,
			ignore_interrupt,0	/* IRQs are caused by 68000 */
		},
	},
	56, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	100,    /* 100 CPU slices per frame */
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1792, 1792,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	twincobr_vh_start,
	twincobr_vh_stop,
	twincobr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	},
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( twincobr_rom )
	ROM_REGION(0x30000)		/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",         0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",         0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "tc15",         0x20000, 0x08000, 0x3a646618 )
	ROM_LOAD_ODD ( "tc13",         0x20000, 0x08000, 0xd7d1e317 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc11",         0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",         0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",         0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",         0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",         0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc06",         0x2c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc05",         0x3c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc07",         0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",         0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",         0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",         0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",         0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",         0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",         0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",         0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)		/* 32k for second CPU */
	ROM_LOAD( "tc12",         0x00000, 0x08000, 0xe37b3c44 )	/* slightly different from the other two sets */

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "tc1b",    0x0000, 0x0800, 0x1757cc33 )
	ROM_LOAD_ODD ( "tc2a",    0x0000, 0x0800, 0xd6d878c9 )
ROM_END

ROM_START( twincobu_rom )
	ROM_REGION(0x30000)		/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",         0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",         0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "tcbra26.bin",  0x20000, 0x08000, 0xbdd00ba4 )
	ROM_LOAD_ODD ( "tcbra27.bin",  0x20000, 0x08000, 0xed600907 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc11",         0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",         0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",         0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",         0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",         0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc06",         0x2c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc05",         0x3c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc07",         0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",         0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",         0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",         0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",         0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",         0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",         0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",         0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)		/* 32k for second CPU */
	ROM_LOAD( "b30-05",       0x00000, 0x08000, 0x1a8f1e10 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp_22.bin",    0x0000, 0x0800, 0x79389a71 )
	ROM_LOAD_ODD ( "dsp_21.bin",    0x0000, 0x0800, 0x2d135376 )
ROM_END

ROM_START( ktiger_rom )
	ROM_REGION(0x30000)		/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",    0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",    0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "b30-02",  0x20000, 0x08000, 0x1d63e9c4 )
	ROM_LOAD_ODD ( "b30-04",  0x20000, 0x08000, 0x03957a30 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc11",         0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",         0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",         0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",         0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",         0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc06",         0x2c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc05",         0x3c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc07",         0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",         0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",         0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",         0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",         0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",         0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",         0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",         0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)		/* 32k for second CPU */
	ROM_LOAD( "b30-05",       0x00000, 0x08000, 0x1a8f1e10 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp-22",    0x0000, 0x0800, 0x00000000 )
	ROM_LOAD_ODD ( "dsp-21",    0x0000, 0x0800, 0x00000000 )
ROM_END


static void twincobr_decode(void)
{
  /* TMS320C10 roms have their address lines A0 and A1 swapped. Decode it. */
	int A;
	unsigned char D1;
	unsigned char D2;
	unsigned char *DSP_ROMS;

	DSP_ROMS = Machine->memory_region[Machine->drv->cpu[2].memory_region];

	for (A = 2;A <= 0x0ffa;A+=8) {
		D1 = DSP_ROMS[A];
		D2 = DSP_ROMS[A+1];
		DSP_ROMS[A] = DSP_ROMS[A+2];
		DSP_ROMS[A+1] = DSP_ROMS[A+3];
		DSP_ROMS[A+2] = D1;
		DSP_ROMS[A+3] = D2;
	}
}

struct GameDriver twincobr_driver =
{
	__FILE__,
	0,
	"twincobr",
	"Twin Cobra (Taito)",
	"1987",
	"Taito",
	"Quench\nNicola Salmoria",
	0,
	&machine_driver,
	0,
	twincobr_rom,
	twincobr_decode, 0,
	0,
	0,
	input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver twincobu_driver =
{
	__FILE__,
	&twincobr_driver,
	"twincobu",
	"Twin Cobra (Romstar)",
	"1987",
	"Taito of America (Romstar license)",
	"Quench\nNicola Salmoria",
	0,
	&machine_driver,
	0,
	twincobu_rom,
	0, 0,
	0,
	0,
	input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver ktiger_driver =
{
	__FILE__,
	&twincobr_driver,
	"ktiger",
	"Kyukyoku Tiger",
	"1987",
	"Taito",
	"Quench\nNicola Salmoria\nCarl-Henrik Skarstedt (Hardware Advice)",
	0,
	&machine_driver,
	0,
	ktiger_rom,
	0, 0,
	0,
	0,
	ktiger_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};
