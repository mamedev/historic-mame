/***************************************************************************

Burger Time hardware description:

Actually Lock'n'Chase is (C)1981 while Burger Time is (C)1982, so it might
be more accurate to say 'Lock'n'Chase hardware'.

This hardware is pretty straightforward, but has a couple of interesting
twists. There are two ports to the video and color RAMs, one normal access,
and one with X and Y coordinates swapped. The sprite RAM occupies the
first row of the swapped area, so it appears in the regular video RAM as
the first column of on the left side.

These games don't have VBLANK interrupts, but instead an IRQ or NMI
(depending on the particular board) is generated when a coin is inserted.

Burger Time and Bump 'n' Jump also have a background playfield which, in the
case of Bump 'n' Jump, can be scrolled vertically.

These boards use two 8910's for sound, controlled by a dedicated 6502 (except
in Eggs, where the main processor handles the sound). The main processor
triggers an IRQ request when writing a command to the sound CPU.

Main clock: XTAL = 12 MHz
Horizontal video frequency: HSYNC = XTAL/768?? = 15.625 kHz ??
Video frequency: VSYNC = HSYNC/272 = 57.44 Hz ?
VBlank duration: 1/VSYNC * (24/272) = 1536 us ?


Note on Lock'n'Chase:

The watchdog test prints "WATCHDOG TEST ER". Just by looking at the code,
I can't see how it could print anything else, there is only one path it
can take. Should the game reset????

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/M6502.h"

extern unsigned char *lnc_charbank;
extern unsigned char *lnc_control;
extern unsigned char *bnj_backgroundram;
extern int bnj_backgroundram_size;
extern unsigned char *bnj_scroll2;

void btime_paletteram_w(int offset,int data);
void btime_background_w(int offset,int data);
void bnj_background_w(int offset, int data);
void bnj_scroll1_w(int offset, int data);
int  btime_mirrorvideoram_r(int offset);
int  btime_mirrorcolorram_r(int offset);
void btime_mirrorvideoram_w(int offset,int data);
void lnc_videoram_w(int offset,int data);
void lnc_mirrorvideoram_w(int offset,int data);
void btime_mirrorcolorram_w(int offset,int data);
void btime_vh_screenrefresh(struct osd_bitmap *bitmap);
static void sound_command_w(int offset,int data);

static struct MemoryReadAddress btime_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_r },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_r },
	{ 0x4000, 0x4000, input_port_0_r },     /* IN0 */
	{ 0x4001, 0x4001, input_port_1_r },     /* IN1 */
	{ 0x4002, 0x4002, input_port_2_r },     /* coin */
	{ 0x4003, 0x4003, input_port_3_r },     /* DSW1 */
	{ 0x4004, 0x4004, input_port_4_r },     /* DSW2 */
	{ 0xb000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress btime_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0c00, 0x0c0f, btime_paletteram_w },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_w },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_w },
	{ 0x4000, 0x4000, MWA_NOP },
	{ 0x4003, 0x4003, sound_command_w },
	{ 0x4004, 0x4004, btime_background_w },
	{ 0xb000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress eggs_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_r },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_r },
	{ 0x2000, 0x2000, input_port_2_r },     /* DSW1 */
	{ 0x2001, 0x2001, input_port_3_r },     /* DSW2 */
	{ 0x2002, 0x2002, input_port_0_r },     /* IN0 */
	{ 0x2003, 0x2003, input_port_1_r },     /* IN1 */
	{ 0x3000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },    /* reset/interrupt vectors */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress eggs_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_w },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_w },
	{ 0x2001, 0x2001, MWA_NOP },
	{ 0x2004, 0x2004, AY8910_control_port_0_w },
	{ 0x2005, 0x2005, AY8910_write_port_0_w },
	{ 0x2006, 0x2006, AY8910_control_port_1_w },
	{ 0x2007, 0x2007, AY8910_write_port_1_w },
	{ 0x3000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress lnc_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_RAM },
	{ 0x7c00, 0x7fff, btime_mirrorvideoram_r },
	{ 0x8000, 0x8000, input_port_3_r },     /* DSW1 */
	{ 0x8001, 0x8001, input_port_4_r },     /* DSW2 */
	{ 0x9000, 0x9000, input_port_0_r },     /* IN0 */
	{ 0x9001, 0x9001, input_port_1_r },     /* IN1 */
	{ 0x9002, 0x9002, input_port_2_r },     /* coin */
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress lnc_writemem[] =
{
	{ 0x0000, 0x3bff, MWA_RAM },
	{ 0x3c00, 0x3fff, lnc_videoram_w, &videoram, &videoram_size },
	{ 0x7800, 0x7bff, MWA_RAM, &colorram },  // Dummy
	{ 0x7c00, 0x7fff, lnc_mirrorvideoram_w },
	{ 0x8000, 0x8000, MWA_NOP },            /* ??? */
	{ 0x8001, 0x8001, MWA_RAM, &lnc_control },
	{ 0x8003, 0x8003, MWA_RAM, &lnc_charbank },
	{ 0x9000, 0x9000, MWA_NOP },            /* ??? */
	{ 0x9002, 0x9002, sound_command_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress bnj_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_3_r },     /* DSW1 */
	{ 0x1001, 0x1001, input_port_4_r },     /* DSW2 */
	{ 0x1002, 0x1002, input_port_0_r },     /* IN0 */
	{ 0x1003, 0x1003, input_port_1_r },     /* IN1 */
	{ 0x1004, 0x1004, input_port_2_r },     /* coin */
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4bff, btime_mirrorvideoram_r },
	{ 0x4c00, 0x4fff, btime_mirrorcolorram_r },
	{ 0xa000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress bnj_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1002, 0x1002, sound_command_w },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4800, 0x4bff, btime_mirrorvideoram_w },
	{ 0x4c00, 0x4fff, btime_mirrorcolorram_w },
	{ 0x5000, 0x51ff, bnj_background_w, &bnj_backgroundram, &bnj_backgroundram_size },
	{ 0x5400, 0x5400, bnj_scroll1_w },
	{ 0x5800, 0x5800, MWA_RAM, &bnj_scroll2 },
	{ 0x5c00, 0x5c0f, btime_paletteram_w },
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x4000, 0x4000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x8000, 0x8000, AY8910_control_port_1_w },
	{ 0xc000, 0xc000, interrupt_enable_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/***************************************************************************

These games don't have VBlank interrupts.
Interrupts are still used by the game, coin insertion generates an IRQ.

***************************************************************************/
static int btime_interrupt(int(*generated_interrupt)(void), int active_high)
{
	static int coin;
	int port;

	port = readinputport(2) & 0xc0;
	if (active_high) port ^= 0xc0;

	if (port != 0xc0)    /* Coin */
	{
		if (coin == 0)
		{
			coin = 1;
			return generated_interrupt();
		}
	}
	else coin = 0;

	return ignore_interrupt();
}

static int btime_irq_interrupt(void)
{
	return btime_interrupt(interrupt, 1);
}

static int btime_nmi_interrupt(void)
{
	return btime_interrupt(nmi_interrupt, 0);
}

int lnc_sound_interrupt(void)
{
	// I have a feeling that this only works by coincidence. I couldn't
	// figure out how NMI's are disabled by the sound processor
	if (*lnc_control & 0x08)
		return nmi_interrupt();
	else
		return ignore_interrupt();
}

static void sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,INT_IRQ);
}


INPUT_PORTS_START( btime_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "10000" )
	PORT_DIPSETTING(    0x04, "15000" )
	PORT_DIPSETTING(    0x02, "20000"  )
	PORT_DIPSETTING(    0x00, "30000"  )
	PORT_DIPNAME( 0x08, 0x08, "Enemies", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x10, 0x10, "End of Level Pepper", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


INPUT_PORTS_START( eggs_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_BIT( 0x30, 0x30, IPT_UNKNOWN )     /* almost certainly unused */
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x04, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x02, "50000" )
	PORT_DIPSETTING(    0x00, "70000"  )
	PORT_DIPSETTING(    0x06, "None"  )
	PORT_BIT( 0x78, 0x78, IPT_UNKNOWN )     /* almost certainly unused */
	PORT_DIPNAME( 0x80, 0x80, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
INPUT_PORTS_END


INPUT_PORTS_START( lnc_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x30, 0x30, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Off" )
	PORT_DIPSETTING(    0x00, "RAM Test Only" )
	PORT_DIPSETTING(    0x20, "Watchdog Test Only" )
	PORT_DIPSETTING(    0x10, "All Tests" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Controller" )
	PORT_DIPSETTING(    0x40, "2 Controllers" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "15,000" )
	PORT_DIPSETTING(    0x04, "20,000" )
	PORT_DIPSETTING(    0x02, "30,000" )
	PORT_DIPSETTING(    0x00, "Never" )
	PORT_DIPNAME( 0x08, 0x08, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 1", IP_KEY_NONE )  // Doesn't seem to be accessed
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 2", IP_KEY_NONE )  // Doesn't seem to be accessed
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 3", IP_KEY_NONE )  // Doesn't seem to be accessed
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 4", IP_KEY_NONE )  // Doesn't seem to be accessed
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x50, "On" )
INPUT_PORTS_END


INPUT_PORTS_START( bnj_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(        0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(        0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(        0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(        0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(        0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(        0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(        0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(        0x04, "1 Coin/3 Credits" )
	PORT_BITX(      0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(        0x10, "Off" )
	PORT_DIPSETTING(        0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(        0x20, "Off" )
	PORT_DIPSETTING(        0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(        0x00, "Upright" )
	PORT_DIPSETTING(        0x40, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(        0x01, "3" )
	PORT_DIPSETTING(        0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(        0x06, "every 30k" )
	PORT_DIPSETTING(        0x04, "every 70k" )
	PORT_DIPSETTING(        0x02, "20k only"  )
	PORT_DIPSETTING(        0x00, "30k only"  )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(        0x08, "No" )
	PORT_DIPSETTING(        0x00, "Yes" )
	PORT_DIPNAME( 0x10, 0x10, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(        0x10, "Easy" )
	PORT_DIPSETTING(        0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(        0x20, "Off" )
	PORT_DIPSETTING(        0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(        0x40, "Off" )
	PORT_DIPSETTING(        0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(        0x80, "Off" )
	PORT_DIPSETTING(        0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },    /* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,      /* 3 bits per pixel */
	{ 0, 256*16*16, 2*256*16*16 },  /* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
	  16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout btime_tilelayout =
{
	16,16,  /* 16*16 characters */
	64,    /* 64 characters */
	3,      /* 3 bits per pixel */
	{ 0, 64*16*16, 64*2*16*16 },    /* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
	  16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8    /* every tile takes 32 consecutive bytes */
};

static struct GfxLayout bnj_tilelayout =
{
	16,16,  /* 16*16 characters */
	64, /* 64 characters */
	3,  /* 3 bits per pixel */
	{ 2*64*16*16+4, 0, 4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  2*16*8+3, 2*16*8+2, 2*16*8+1, 2*16*8+0, 3*16*8+3, 3*16*8+2, 3*16*8+1, 3*16*8+0 },
	64*8    /* every tile takes 64 consecutive bytes */
};

static struct GfxDecodeInfo btime_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 1 }, /* char set #1 */
	{ 1, 0x0000, &spritelayout,     0, 1 }, /* sprites */
	{ 1, 0x6000, &btime_tilelayout, 8, 1 }, /* background tiles */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },     /* char set #1 */
	{ 1, 0x0000, &spritelayout, 0, 1 },     /* sprites */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo bnj_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 1 }, /* char set #1 */
	{ 1, 0x0000, &spritelayout,     0, 1 }, /* sprites */
	{ 1, 0x6000, &bnj_tilelayout,   8, 1 }, /* background tiles */
	{ -1 } /* end of array */
};


/* The original Burger Time has color RAM, but the bootleg uses a PROM. */
static unsigned char hamburge_color_prom[] =
{
	0x00,0xFF,0x2F,0x3F,0x07,0x38,0x1E,0x2B,0x00,0xAD,0xF8,0xC0,0xFF,0x07,0x3F,0xC7,
	0x00,0xFF,0x2F,0x3F,0x07,0x38,0x1E,0x2B,0x00,0xAD,0xF8,0xC0,0xFF,0x07,0x3F,0xC7
};

unsigned char eggs_palette[] =
{
	0x00,0x00,0x00,   /* black      */
	0xd8,0xd8,0x74,   /* darkyellow */
	0xd8,0x74,0x40,   /* darkwhite  */
	0xff,0x44,0x00,   /* orange     */
	0x00,0x00,0xd8,   /* blue   */
	0xf8,0x00,0x00,   /* red    */
	0xf8,0xf8,0x20,   /* yellow */
	0xff,0xff,0xff    /* white  */
};

enum
{
	black, darkyellow, darkwhite, orange, blue, red, yellow, white
};

unsigned char eggs_colortable[] =
{
	black, darkyellow, blue, darkwhite, red, orange, yellow, white
};

static unsigned char lnc_color_prom[] =
{
	// I think the color PROM is corrupted in the nibble indicated by the arrow.
	// Changing it to 0x07 makes the color scheme match the flyer's drawing to
	// a tee. The flyer is at
	// www.gamearchive.com/flyers/video/taito/locknchase_f.jpg

	/* palette SC-5M */
  //0x00,0xdf,0x51,0x1c,0xa7,0xe0,0xfc,0xff,
    0x00,0xdf,0x51,0x1c,0x07,0xe0,0xfc,0xff,
  //                      ^

	/* ROM SB-4C, don't know what it is, unused for now */
	0xf7,0xf7,0xf5,0xfd,0xf9,0xf9,0xf0,0xf6,0xf6,0xf6,0xf6,0xf4,0xfc,0xf8,0xf8,0xf6,
	0xf6,0xf6,0xf4,0xfc,0xf8,0xf8,0xf6,0xf6,0xf6,0xf6,0xf6,0xf4,0xfc,0xf8,0xf8,0xf6
};


static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ? (hand tuned) */
	{ 0x20ff, 0x20ff },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

/********************************************************************
*
*  Machine Driver macro
*  ====================
*
*  Abusing the pre-processor.
*
********************************************************************/

#define MACHINE_DRIVER(GAMENAME, MAIN_IRQ, SOUND_IRQ, GFX, COLOR)   \
																	\
void GAMENAME##_init_machine(void);                                 \
void GAMENAME##_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom); \
int  GAMENAME##_vh_start (void);                                    \
void GAMENAME##_vh_stop (void);                                     \
																	\
static struct MachineDriver GAMENAME##_machine_driver =             \
{                                                                   \
	/* basic machine hardware */                                	\
	{		                                                        \
		{	  	                                                    \
			CPU_M6502,                                  			\
			750000,        /* 750 Khz ???? */          				\
			0,                                          			\
			GAMENAME##_readmem,GAMENAME##_writemem,0,0, 			\
			MAIN_IRQ,1                                  			\
		},		                                                    \
		{		                                                    \
			CPU_M6502 | CPU_AUDIO_CPU,                  			\
			500000, /* 500 kHz */                       			\
			2,      /* memory region #2 */              			\
			sound_readmem,sound_writemem,0,0,           			\
			SOUND_IRQ,16   /* IRQs are triggered by the main CPU */ \
		}                                                   		\
	},                                                          	\
	57, 1536,        /* frames per second, vblank duration */   	\
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	GAMENAME##_init_machine,                                    	\
																	\
	/* video hardware */                                        	\
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },                   	\
	GFX,                                                        	\
	COLOR,COLOR,                                                	\
	GAMENAME##_vh_convert_color_prom,                           	\
																	\
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,                   	\
	0,                                                          	\
	GAMENAME##_vh_start,                                        	\
	GAMENAME##_vh_stop,                                         	\
	btime_vh_screenrefresh,                                     	\
																	\
	/* sound hardware */                                        	\
	0,0,0,0,                                                    	\
	{                                                           	\
		{                                                   		\
			SOUND_AY8910,                               			\
			&ay8910_interface                           			\
		}                                                   		\
	}                                                           	\
}

MACHINE_DRIVER(btime, btime_irq_interrupt, nmi_interrupt, btime_gfxdecodeinfo, 16);

MACHINE_DRIVER(eggs, interrupt, nmi_interrupt, gfxdecodeinfo, sizeof(eggs_palette)/3);

MACHINE_DRIVER(lnc, btime_nmi_interrupt, lnc_sound_interrupt, gfxdecodeinfo, 8);

MACHINE_DRIVER(bnj, btime_nmi_interrupt, nmi_interrupt, bnj_gfxdecodeinfo, 16);

/***************************************************************************

	Game driver(s)

***************************************************************************/

ROM_START( btime_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ab05a1.12b", 0xb000, 0x1000, 0x1d9da777 )
	ROM_LOAD( "ab04.9b",    0xc000, 0x1000, 0x9dcc9634 )
	ROM_LOAD( "ab06.13b",   0xd000, 0x1000, 0x8fe2de1c )
	ROM_LOAD( "ab05.10b",   0xe000, 0x1000, 0x24560b1c )
	ROM_LOAD( "ab07.15b",   0xf000, 0x1000, 0x7e750c89 )

	ROM_REGION(0x7800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ab8.13k",    0x0000, 0x1000, 0xccc1f7cf )    /* charset #1 */
	ROM_LOAD( "ab9.15k",    0x1000, 0x1000, 0xfcee0000 )
	ROM_LOAD( "ab10.10k",   0x2000, 0x1000, 0x9f7eaf02 )
	ROM_LOAD( "ab11.12k",   0x3000, 0x1000, 0xf491ffff )
	ROM_LOAD( "ab12.7k",    0x4000, 0x1000, 0x85b738a9 )
	ROM_LOAD( "ab13.9k" ,   0x5000, 0x1000, 0xf5faff00 )
	ROM_LOAD( "ab02.4b",    0x6000, 0x0800, 0x644f1331 )    /* charset #2 */
	ROM_LOAD( "ab01.3b",    0x6800, 0x0800, 0x18000000 )
	ROM_LOAD( "ab00.1b",    0x7000, 0x0800, 0xea53eb39 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "ab14.12h",   0xf000, 0x1000, 0x06b5888d )

	ROM_REGION(0x0800)      /* background graphics */
	ROM_LOAD( "ab03.6b",    0x0000, 0x0800, 0x8d200806 )
ROM_END

ROM_START( btimea_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "aa04.9b",    0xc000, 0x1000, 0x3d080936 )
	ROM_LOAD( "aa06.13b",   0xd000, 0x1000, 0xe2db880f )
	ROM_LOAD( "aa05.10b",   0xe000, 0x1000, 0x34bec090 )
	ROM_LOAD( "aa07.15b",   0xf000, 0x1000, 0xd993a235 )

	ROM_REGION(0x7800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "aa8.13k",    0x0000, 0x1000, 0xf994fcc4 )    /* charset #1 */
	ROM_LOAD( "aa9.15k",    0x1000, 0x1000, 0xfcee0000 )
	ROM_LOAD( "aa10.10k",   0x2000, 0x1000, 0x9f7eaf02 )
	ROM_LOAD( "aa11.12k",   0x3000, 0x1000, 0xf491ffff )
	ROM_LOAD( "aa12.7k",    0x4000, 0x1000, 0x7f8438a2 )
	ROM_LOAD( "aa13.9k" ,   0x5000, 0x1000, 0xf5faff00 )
	ROM_LOAD( "aa02.4b",    0x6000, 0x0800, 0x644f1331 )    /* charset #2 */
	ROM_LOAD( "aa01.3b",    0x6800, 0x0800, 0x18000000 )
	ROM_LOAD( "aa00.1b",    0x7000, 0x0800, 0xea53eb39 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "aa14.12h",   0xf000, 0x1000, 0x06b5888d )
	ROM_LOAD( "aa00.1b",    0x7000, 0x0800, 0xea53eb39 )

	ROM_REGION(0x0800)      /* background graphics */
	ROM_LOAD( "aa03.6b",    0x0000, 0x0800, 0x8d200806 )
ROM_END

ROM_START( hamburge_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	/* the following might be wrong - ROMs seem encrypted */
	ROM_LOAD( "inc.1", 0xc000, 0x2000, 0x5e7aae64 )
	ROM_LOAD( "inc.2", 0xe000, 0x2000, 0x57e824c8 )

	ROM_REGION(0x7800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "inc.9", 0x0000, 0x2000, 0x2cd80018 ) /* charset #1 */
	ROM_LOAD( "inc.8", 0x2000, 0x2000, 0x940f50fd )
	ROM_LOAD( "inc.7", 0x4000, 0x2000, 0xdd7cc6dc )
	ROM_LOAD( "inc.5", 0x6000, 0x0800, 0x241199e7 ) /* garbage?? */
	ROM_CONTINUE(      0x6000, 0x0800 )             /* charset #2 */
	ROM_LOAD( "inc.4", 0x6800, 0x0800, 0x02c16917 ) /* garbage?? */
	ROM_CONTINUE(      0x6800, 0x0800 )
	ROM_LOAD( "inc.3", 0x7000, 0x0800, 0xc8b931bf ) /* garbage?? */
	ROM_CONTINUE(      0x7000, 0x0800 )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "inc.6", 0x0000, 0x1000, 0x3c760fb8 ) /* starts at 0000, not f000; 0000-01ff is RAM */
	ROM_RELOAD(        0xf000, 0x1000 )     /* for the reset/interrupt vectors */

	ROM_REGION(0x0800)      /* background graphics */
	/* this ROM is missing? maybe the hardware works differently */
ROM_END

ROM_START( eggs_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "e14.bin", 0x3000, 0x1000, 0x996dbebf )
	ROM_LOAD( "d14.bin", 0x4000, 0x1000, 0xbbbce334 )
	ROM_LOAD( "c14.bin", 0x5000, 0x1000, 0xc89bff1d )
	ROM_LOAD( "b14.bin", 0x6000, 0x1000, 0x86e4f94e )
	ROM_LOAD( "a14.bin", 0x7000, 0x1000, 0x9beb93e9 )
	ROM_RELOAD(          0xf000, 0x1000 )   /* for reset/interrupt vectors */

	ROM_REGION(0x6000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "j12.bin",  0x0000, 0x1000, 0x27ab70f5 )
	ROM_LOAD( "j10.bin",  0x1000, 0x1000, 0x8786e8c4 )
	ROM_LOAD( "h12.bin",  0x2000, 0x1000, 0x9c23f42b )
	ROM_LOAD( "h10.bin",  0x3000, 0x1000, 0x77b53ac7 )
	ROM_LOAD( "g12.bin",  0x4000, 0x1000, 0x4beb2eab )
	ROM_LOAD( "g10.bin",  0x5000, 0x1000, 0x61460352 )

	ROM_REGION(0x10000)     /* Dummy region for nonexistent audio CPU */
ROM_END

ROM_START( scregg_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "scregg.e14", 0x3000, 0x1000, 0x84a288b2 )
	ROM_LOAD( "scregg.d14", 0x4000, 0x1000, 0x8ab8d190 )
	ROM_LOAD( "scregg.c14", 0x5000, 0x1000, 0xf212ac78 )
	ROM_LOAD( "scregg.b14", 0x6000, 0x1000, 0xdc095969 )
	ROM_LOAD( "scregg.a14", 0x7000, 0x1000, 0x1afd9ddb )
	ROM_RELOAD(             0xf000, 0x1000 )        /* for reset/interrupt vectors */

	ROM_REGION(0x6000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scregg.j12",  0x0000, 0x1000, 0xe5077119 )
	ROM_LOAD( "scregg.j10",  0x1000, 0x1000, 0xc41a7b5e )
	ROM_LOAD( "scregg.h12",  0x2000, 0x1000, 0xc48207f2 )
	ROM_LOAD( "scregg.h10",  0x3000, 0x1000, 0x007800b8 )
	ROM_LOAD( "scregg.g12",  0x4000, 0x1000, 0xa3b0e2d6 )
	ROM_LOAD( "scregg.g10",  0x5000, 0x1000, 0xeb7f0865 )

	ROM_REGION(0x10000)     /* Dummy region for nonexistent audio CPU */
ROM_END

ROM_START( lnc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "s3-3d", 0xc000, 0x1000, 0xa8106830 )
	ROM_LOAD( "s2-3c", 0xd000, 0x1000, 0x1c274145 )
	ROM_LOAD( "s1-3b", 0xe000, 0x1000, 0xa41f02d5 )
	ROM_LOAD( "s0-3a", 0xf000, 0x1000, 0xee021c06 )

	ROM_REGION(0x6000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s8-15l",  0x0000, 0x1000, 0x04ac8f32 )
	ROM_LOAD( "s9-15m",  0x1000, 0x1000, 0x0845b92f )
	ROM_LOAD( "s6-13l",  0x2000, 0x1000, 0x7550c9d6 )
	ROM_LOAD( "s7-13m",  0x3000, 0x1000, 0x23457281 )
	ROM_LOAD( "s4-11l",  0x4000, 0x1000, 0x4b6f09bf )
	ROM_LOAD( "s5-11m",  0x5000, 0x1000, 0x7aa5ddab )
// 0 1 2   2 1 0   1 0 2   0 2 1   1 2 0    2 0 1
	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "sa-1h",  0xf000, 0x1000, 0x2c680040 )
ROM_END

ROM_START( bnj_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "bnj12b.bin", 0xa000, 0x2000, 0x9ce25062 )
	ROM_LOAD( "bnj12c.bin", 0xc000, 0x2000, 0x9e763206 )
	ROM_LOAD( "bnj12d.bin", 0xe000, 0x2000, 0xc5a135b9 )

	ROM_REGION(0x8000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bnj4h.bin",  0x0000, 0x2000, 0xc141332f )
	ROM_LOAD( "bnj4f.bin",  0x2000, 0x2000, 0x5035d3c9 )
	ROM_LOAD( "bnj4e.bin",  0x4000, 0x2000, 0x97b719fb )
	ROM_LOAD( "bnj10e.bin", 0x6000, 0x1000, 0x43556767 )
	ROM_LOAD( "bnj10f.bin", 0x7000, 0x1000, 0xc1bfbfbf )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "bnj6c.bin",  0xf000, 0x1000, 0x80f9e12b )
ROM_END

ROM_START( brubber_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	/* a000-bfff space for the service ROM */
	ROM_LOAD( "brubber.12c", 0xc000, 0x2000, 0x2e85db11 )
	ROM_LOAD( "brubber.12d", 0xe000, 0x2000, 0x1d905348 )

	ROM_REGION(0x8000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "brubber.4h",  0x0000, 0x2000, 0xc141332f )
	ROM_LOAD( "brubber.4f",  0x2000, 0x2000, 0x5035d3c9 )
	ROM_LOAD( "brubber.4e",  0x4000, 0x2000, 0x97b719fb )
	ROM_LOAD( "brubber.10e", 0x6000, 0x1000, 0x43556767 )
	ROM_LOAD( "brubber.10f", 0x7000, 0x1000, 0xc1bfbfbf )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "brubber.6c",  0xf000, 0x1000, 0x80f9e12b )
ROM_END

ROM_START( caractn_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	/* a000-bfff space for the service ROM */
	ROM_LOAD( "caractn.a7", 0xc000, 0x2000, 0x2e85db11 )
	ROM_LOAD( "caractn.a6", 0xe000, 0x2000, 0x105bf9d5 )

	ROM_REGION(0x8000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "caractn.a2", 0x0000, 0x2000, 0xa72dd54b )
	ROM_LOAD( "caractn.a1", 0x2000, 0x2000, 0x3582b118 )
	ROM_LOAD( "caractn.a0", 0x4000, 0x2000, 0x6da52fab )
	ROM_LOAD( "caractn.a3", 0x6000, 0x1000, 0x43556767 )
	ROM_LOAD( "caractn.a4", 0x7000, 0x1000, 0xc1bfbfbf )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "caractn.a5", 0xf000, 0x1000, 0x80f9e12b )
ROM_END


static void btime_decode(void)
{
	/* the encryption is a simple bit rotation: 76543210 -> 65342710      */
	/* it is not known, however, when it must be applied. Only opcodes at */
	/* addresses with this bit pattern:                                   */
	/* xxxx xxx1 xxxx x1xx                                                */
	/* _can_ (but not necessarily are) be encrypted.                      */

	static int patchlist[] =
	{
		0xb105,0xb116,0xb11e,0xb12c,0xb13d,0xb146,0xb14d,0xb154,
		0xb15e,0xb175,0xb18f,0xb195,0xb19d,0xb1a4,0xb1bd,0xb1c4,
		0xb1cf,0xb1d4,0xb1dd,0xb1f4,0xb31c,0xb326,0xb336,0xb347,
		0xb34f,0xb35d,0xb396,0xb39e,0xb3ad,0xb3af,0xb3bc,0xb3cc,
		0xb3dd,0xb3ed,0xb3f7,0xb507,0xb50c,0xb516,0xb525,0xb52d,
		0xb537,0xb53c,0xb544,0xb55d,0xb567,0xb56c,0xb56f,0xb575,
		0xb57d,0xb587,0xb58c,0xb594,0xb59e,0xb5ac,0xb5b6,0xb5be,
		0xb5d4,0xb5df,0xb5e4,0xb5e6,0xb5fd,0xb5ff,0xb707,0xb70d,
		0xb715,0xb71f,0xb73d,0xb744,0xb746,0xb74c,0xb755,0xb757,
		0xb76d,0xb786,0xb7d6,0xb7dc,0xb7fe,0xb90e,0xb916,0xb91e,
		0xb927,0xb92d,0xb92f,0xb937,0xb93f,0xb947,0xb965,0xb96f,
		0xb974,0xb97e,0xb986,0xb98c,0xb99f,0xb9a5,0xb9bc,0xb9ce,
		0xb9f6,0xbb0d,0xc105,0xc10e,0xc115,0xc15e,0xc16c,0xc17f,
		0xc187,0xc18d,0xc1b5,0xc1bd,0xc1bf,0xc1de,0xc1ed,0xc1f5,
		0xc347,0xc34d,0xc355,0xc35c,0xc35f,0xc365,0xc36e,0xc377,
		0xc37c,0xc386,0xc396,0xc3a4,0xc3ac,0xc3b7,0xc3bc,0xc3d6,
		0xc504,0xc50c,0xc51e,0xc525,0xc52e,0xc534,0xc537,0xc53d,
		0xc546,0xc54c,0xc54f,0xc557,0xc55e,0xc56e,0xc57d,0xc587,
		0xc58c,0xc594,0xc596,0xc59c,0xc5a6,0xc5bd,0xc7dd,0xc7ee,
		0xc7fd,0xc7ff,0xc906,0xc92f,0xc937,0xc93c,0xc946,0xc955,
		0xc95d,0xc966,0xc96f,0xc976,0xc97f,0xc986,0xc9b5,0xc9dd,
		0xcb17,0xcb1e,0xcb34,0xcb44,0xcb56,0xcb5e,0xcb6e,0xcb76,
		0xcb7e,0xcb8c,0xcb9e,0xcbad,0xcbb5,0xcbe4,0xcbe7,0xcd0e,
		0xcd14,0xcd1e,0xcd2d,0xcd3e,0xcd47,0xcd4d,0xcd56,0xcd6f,
		0xcd7c,0xcd8e,0xcda4,0xcdaf,0xcdb5,0xcdbf,0xcdc5,0xcdc7,
		0xcdce,0xcdd4,0xd105,0xd117,0xd11d,0xd124,0xd137,0xd13c,
		0xd15d,0xd197,0xd19f,0xd1a7,0xd1d6,0xd1dc,0xd1e4,0xd1ff,
		0xd306,0xd30d,0xd334,0xd345,0xd347,0xd36c,0xd394,0xd3ac,
		0xd3ae,0xd3b4,0xd3bf,0xd3c4,0xd3de,0xd3e6,0xd3ec,0xd3ee,
		0xd3ff,0xd50f,0xd526,0xd52c,0xd53c,0xd557,0xd55d,0xd56d,
		0xd57e,0xd58f,0xd597,0xd5a6,0xd5af,0xd5bf,0xd5ce,0xd5df,
		0xd5e7,0xd5f6,0xd72d,0xd734,0xd744,0xd757,0xd764,0xd78d,
		0xd90f,0xd92e,0xd937,0xd94d,0xd94f,0xd956,0xd95e,0xd965,
		0xd9b7,0xd9c5,0xd9cc,0xd9d7,0xd9f7,0xdb1d,0xdb45,0xdb4d,
		0xdb57,0xdb6d,0xdb77,0xdb8c,0xdba5,0xdbaf,0xdbb5,0xdbb7,
		0xdbbd,0xdbc5,0xdbd7,0xdbe5,0xdbed,0xdd1c,0xdd2e,0xdd47,
		0xdd6d,0xdd8d,0xdda4,0xddae,0xddb6,0xddbd,0xddd7,0xdddc,
		0xddde,0xddee,0xddfd,0xdf0f,0xdf15,0xdf1c,0xdf36,0xdf3d,
		0xdf67,0xdf75,0xdf87,0xdf8d,0xdf9f,0xdfa5,0xdfb7,0xdfbd,
		0xdfcf,0xdfd6,0xe10d,0xe127,0xe12e,0xe134,0xe13f,0xe157,
		0xe15c,0xe167,0xe16d,0xe17e,0xe18f,0xe1b5,0xe1be,0xe1e5,
		0xe1fe,0xe305,0xe315,0xe32d,0xe347,0xe355,0xe357,0xe35d,
		0xe36c,0xe376,0xe38d,0xe38f,0xe395,0xe3a4,0xe3ae,0xe3c5,
		0xe3c7,0xe3cd,0xe3dc,0xe3e6,0xe3fd,0xe3ff,0xe51c,0xe524,
		0xe53e,0xe546,0xe567,0xe56c,0xe586,0xe5ac,0xe5b5,0xe5be,
		0xe5c7,0xe5cc,0xe5ce,0xe704,0xe70f,0xe72e,0xe737,0xe746,
		0xe75e,0xe764,0xe775,0xe784,0xe794,0xe7bf,0xe7e7,0xe90f,
		0xe916,0xe947,0xe95e,0xe966,0xe96f,0xe97f,0xe986,0xe994,
		0xe99d,0xe9a4,0xe9ad,0xe9f5,0xeb26,0xeb35,0xeb75,0xeb77,
		0xeb7d,0xebad,0xebbd,0xebc7,0xebff,0xed2d,0xed3e,0xed56,
		0xf1af,0xf1b4,0xf1b6,0xf1bf,0xf1c5,0xf1dc,0xf1e4,0xf1ec,
		0xf306,0xf317,0xf31e,0xf34d,0xf35c,0xf36c,0xf37d,0xf387,
		0xf38c,0xf38e,0xf395,0xf39d,0xf39f,0xf3ae,0xf3b4,0xf3c6,
		0xf507,0xf50f,0xf51d,0xf51f,0xf524,0xf52c,0xf52e,0xf535,
		0xf537,0xf54c,0xf564,0xf576,0xf57f,0xf5c7,0xf5de,0xf5ee,
		0xf707,0xf70f,0xf714,0xf736,0xf73d,0xf73f,0xf745,0xf747,
		0xf74d,0xf74f,0xf756,0xf767,0xf76c,0xf775,0xf77d,0xf796,
		0xf79e,0xf7ad,0xf7b4,0xf7c7,0xf7dc,0xf7f7,0xf7fd,0xff1c,
		0xff1e,0xff3f
	};
	int i;


	memcpy(ROM,RAM,0x10000);

	for (i = 0;i < sizeof(patchlist) / sizeof(int);i++)
	{
		int A;

		A = patchlist[i];
		ROM[A] = (RAM[A] & 0x13) | ((RAM[A] & 0x80) >> 5) | ((RAM[A] & 0x64) << 1)
			   | ((RAM[A] & 0x08) << 2);
	}
}

static void btimea_decode(void)
{
	/* the encryption is a simple bit rotation: 76543210 -> 65342710      */
	/* it is not known, however, when it must be applied. Only opcodes at */
	/* addresses with this bit pattern:                                   */
	/* xxxx xxx1 xxxx x1xx                                                */
	/* _can_ (but not necessarily are) be encrypted.                      */

	static int patchlist[] =
	{
		0xc12e,0xc13c,0xc14f,0xc157,0xc15d,0xc185,0xc18d,0xc18f,0xc1ae,
		0xc1bd,0xc1c5,0xc1de,0xc1fc,0xc317,0xc31d,0xc325,0xc32c,0xc32f,
		0xc335,0xc33e,0xc347,0xc34c,0xc356,0xc366,0xc374,0xc37c,0xc387,
		0xc38c,0xc50c,0xc56c,0xc56e,0xc576,0xc57d,0xc586,0xc58c,0xc58f,
		0xc595,0xc597,0xc59d,0xc5a7,0xc5ac,0xc5b6,0xc5c4,0xc5c6,0xc5d7,
		0xc70d,0xc71e,0xc72d,0xc72f,0xc736,0xc74d,0xc74f,0xc75c,0xc764,
		0xc767,0xc78c,0xc795,0xc79f,0xc7c6,0xc7d7,0xc7dd,0xc7ec,0xc7f5,
		0xc91d,0xc927,0xc94e,0xc965,0xc976,0xc97f,0xc985,0xc98c,0xc997,
		0xc99e,0xc9bc,0xc9bf,0xc9cc,0xc9e5,0xcb06,0xcb0d,0xcb1e,0xcb34,
		0xcb3e,0xcb47,0xcb55,0xcb57,0xcb5d,0xcb6c,0xcb74,0xcb7d,0xcb7f,
		0xcb8d,0xcba4,0xcbb4,0xcbb6,0xcbce,0xcbee,0xcbf7,0xcbfc,0xcd36,
		0xcd3e,0xcd44,0xcd54,0xcd5d,0xcd76,0xcd87,0xcd8d,0xcd8f,0xcd95,
		0xcd9d,0xcda7,0xcdad,0xcdb6,0xcf3d,0xcf46,0xcf57,0xcf8d,0xcf9c,
		0xcfd7,0xcfef,0xcff7,0xd106,0xd11e,0xd125,0xd12c,0xd13f,0xd16e,
		0xd177,0xd1ad,0xd1b6,0xd1cd,0xd1d5,0xd1e5,0xd1e7,0xd1fe,0xd30c,
		0xd31f,0xd326,0xd32c,0xd32e,0xd336,0xd347,0xd3d5,0xd3de,0xd3e5,
		0xd3f4,0xd3fc,0xd51f,0xd535,0xd537,0xd53c,0xd53e,0xd56f,0xd584,
		0xd594,0xd5b7,0xd5be,0xd796,0xd7a5,0xd7be,0xd7c4,0xd7cf,0xd7d5,
		0xd7f5,0xd906,0xd917,0xd93f,0xd94e,0xd954,0xd95e,0xd97f,0xd987,
		0xd994,0xd996,0xd99e,0xd9be,0xd9e4,0xd9ed,0xd9ef,0xdb35,0xdb3f,
		0xdb5e,0xdb65,0xdb67,0xdb6c,0xdb6e,0xdb74,0xdb7d,0xdb87,0xdb95,
		0xdbc4,0xdbdf,0xdbe6,0xdbef,0xdbf4,0xdd04,0xdd14,0xdd1c,0xdd1e,
		0xdd65,0xdd6e,0xdd86,0xdd95,0xdd9d,0xddd7,0xdddd,0xddec,0xdf34,
		0xdf4e,0xdf54,0xdf6e,0xdfbc,0xdfd6,0xe107,0xe116,0xe11e,0xe12e,
		0xe13d,0xe15c,0xe166,0xe16d,0xe174,0xe196,0xe1a4,0xe1ae,0xe1c6,
		0xe1ee,0xe1fd,0xe1ff,0xe31f,0xe334,0xe357,0xe36c,0xe38f,0xe3a4,
		0xe3c7,0xe3dc,0xe3ff,0xe516,0xe55d,0xe56e,0xe57f,0xe586,0xe5b4,
		0xe5be,0xe5ce,0xe5d7,0xe5dc,0xe5ff,0xe717,0xe726,0xe737,0xe73e,
		0xe746,0xe76f,0xe786,0xe7be,0xe7cd,0xe93c,0xe93f,0xe94c,0xe965,
		0xe98c,0xe98f,0xe99c,0xe9af,0xe9e7,0xe9ed,0xe9f7,0xe9fe,0xeb07,
		0xeb0c,0xeb2f,0xeb34,0xeb37,0xeb3e,0xeb44,0xeb4c,0xeb77,0xeb7e,
		0xeb87,0xeb8c,0xebb5,0xebd5,0xefff,0xf107,0xf10f,0xf115,0xf117,
		0xf146,0xf14c,0xf15d,0xf164,0xf194,0xf196,0xf1ae,0xf1b6,0xf1bc,
		0xf1c6,0xf1d4,0xf1dc,0xf1ed,0xf1fd,0xf305,0xf30f,0xf317,0xf324,
		0xf32f,0xf35e,0xf365,0xf36d,0xf36f,0xf376,0xf37c,0xf38d,0xf3a5,
		0xf3b7,0xf3cc,0xf504,0xf515,0xf51e,0xf52f,0xf535,0xf53f,0xf555,
		0xf564,0xf577,0xf57e,0xf584,0xf586,0xf58c,0xf58e,0xf597,0xf59c,
		0xf5ad,0xf5b6,0xf5be,0xf5d7,0xf5df,0xf5ec,0xf5ee,0xf5f5
	};
	int i;


	memcpy(ROM,RAM,0x10000);

	for (i = 0;i < sizeof(patchlist) / sizeof(int);i++)
	{
		int A;

		A = patchlist[i];
		ROM[A] = (RAM[A] & 0x13) | ((RAM[A] & 0x80) >> 5) | ((RAM[A] & 0x64) << 1)
			   | ((RAM[A] & 0x08) << 2);
	}
}

static void lnc_decode (void)
{
	static int encrypttable[] = { 0x00, 0x10, 0x40, 0x50, 0x20, 0x30, 0x60, 0x70,
								  0x80, 0x90, 0xc0, 0xd0, 0xa0, 0xb0, 0xe0, 0xf0};

	int A;

	for (A = 0;A < 0x10000;A++)
	{
		/*
		 *  The encryption is dead simple.  Just swap bits 5 & 6 for opcodes
		 *  only.
		 */
		ROM[A] = (RAM[A] & 0x0f) | encrypttable[RAM[A] >> 4];
	}
}



static int btime_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0036],"\x00\x80\x02",3) == 0 &&
		memcmp(&RAM[0x0042],"\x50\x48\00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0036],6*6);
			RAM[0x0033] = RAM[0x0036];
			RAM[0x0034] = RAM[0x0037];
			RAM[0x0035] = RAM[0x0038];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void btime_hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0036],6*6);
		osd_fclose(f);
	}
}


static int eggs_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0400],"\x17\x25\x19",3)==0) &&
		(memcmp(&RAM[0x041B],"\x00\x47\x00",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0400],0x1E);
			/* Fix hi score at top */
			memcpy(&RAM[0x0015],&RAM[0x0403],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void eggs_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0400],0x1E);
		osd_fclose(f);
	}
}

static int lnc_hiload(void)
{
	/*
	 *   Get the RAM pointer (this game is multiCPU so we can't assume the
	 *   global RAM pointer is pointing to the right place)
	 */
	unsigned char *RAM = Machine->memory_region[0];

	/*   Check if the hi score table has already been initialized.
	 */
	if (RAM[0x02b7] == 0xff)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int banksave, i;

			osd_fread(f,&RAM[0x0008],0x03);
			osd_fread(f,&RAM[0x0294],0x0f);
			osd_fread(f,&RAM[0x02a6],0x0f);

			// Put high score on screen as we missed when it was done
			// by the program
			banksave = *lnc_charbank;
			*lnc_charbank = 0;

			lnc_videoram_w(0x004c, 0x0b);
			lnc_videoram_w(0x0052, (RAM[0x0008] & 0x0f) + 1);
			lnc_videoram_w(0x0051, (RAM[0x0008] >> 4) + 1);
			lnc_videoram_w(0x0050, (RAM[0x0009] & 0x0f) + 1);
			lnc_videoram_w(0x004f, (RAM[0x0009] >> 4) + 1);
			lnc_videoram_w(0x004e, (RAM[0x000a] & 0x0f) + 1);
			lnc_videoram_w(0x004d, (RAM[0x000a] >> 4) + 1);

			// Remove leading zeros
			for (i = 0; i < 5; i++)
			{
				if (videoram_r(0x004d + i) != 0x01) break;

				lnc_videoram_w(0x004c + i, 0x00);
				lnc_videoram_w(0x004d + i, 0x0b);
			}

			*lnc_charbank = banksave;

			osd_fclose(f);
		}

		return 1;
	}

	/*
	 *  The hi scores can't be loaded yet.
	 */
	return 0;
}

static void lnc_hisave(void)
{
	void *f;

	/*
	 *   Get the RAM pointer (this game is multiCPU so we can't assume the
	 *   global RAM pointer is pointing to the right place)
	 */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0008],0x03);
		osd_fwrite(f,&RAM[0x0294],0x0f);
		osd_fwrite(f,&RAM[0x02a6],0x0f);
		osd_fclose(f);
	}
}

static int bnj_hiload(void)
{
	/*
	 *   Get the RAM pointer (this game is multiCPU so we can't assume the
	 *   global RAM pointer is pointing to the right place)
	 */
	unsigned char *RAM = Machine->memory_region[0];

	/*   Check if the hi score table has already been initialized.
	 */
	if (RAM[0x0640] == 0x4d)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0500],640);
			osd_fclose(f);
		}

		return 1;
	}

	/*
	 *  The hi scores can't be loaded yet.
	 */
	return 0;
}

static void bnj_hisave(void)
{
	void *f;

	/*
	 *   Get the RAM pointer (this game is multiCPU so we can't assume the
	 *   global RAM pointer is pointing to the right place)
	 */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0500],640);
		osd_fclose(f);
	}
}


#define GAMEDRIVER(GAMENAME, BASENAME, DECODE, PROM, DESC, CREDITS) \
struct GameDriver GAMENAME##_driver =          \
{                                              \
	DESC,	                                   \
	#GAMENAME,                             	   \
	CREDITS,                               	   \
	&BASENAME##_machine_driver,            	   \
											   \
	GAMENAME##_rom,                        	   \
	0, DECODE,                          	   \
	0,                                     	   \
	0,      /* sound_prom */             	   \
											   \
	BASENAME##_input_ports,                	   \
											   \
	PROM, 0, 0,                            	   \
	ORIENTATION_DEFAULT,                   	   \
											   \
	BASENAME##_hiload, BASENAME##_hisave   	   \
}

GAMEDRIVER(btime, btime, btime_decode, 0, "Burger Time (Midway)",
		   "Kevin Brisley (Replay emulator)\nMirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)");

GAMEDRIVER(btimea, btime, btimea_decode, 0, "Burger Time (Data East)",
		   "Kevin Brisley (Replay emulator)\nMirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)");

GAMEDRIVER(hamburge, btime, 0, hamburge_color_prom, "Hamburger",
		   "Kevin Brisley (Replay emulator)\nMirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)");

GAMEDRIVER(eggs, eggs, 0, 0, "Eggs",
		   "Nicola Salmoria (MAME driver)\nMike Balfour (high score save)");

GAMEDRIVER(scregg, eggs, 0, 0, "Scrambled Egg",
		   "Nicola Salmoria (MAME driver)\nMike Balfour (high score save)");

GAMEDRIVER(lnc, lnc, lnc_decode, lnc_color_prom, "Lock'n'Chase",
		   "Zsolt Vasvari\nKevin Brisley (Bunp 'n' Jump driver)\nMirko Buffoni (Audio/Add. code)");

GAMEDRIVER(bnj, bnj, lnc_decode, 0, "Bump 'n' Jump",
		   "Kevin Brisley (MAME driver)\nMirko Buffoni (Audio/Add. code)");

GAMEDRIVER(brubber, bnj, lnc_decode, 0, "Burnin' Rubber",
		   "Kevin Brisley (MAME driver)\nMirko Buffoni (Audio/Add. code)");

GAMEDRIVER(caractn, bnj, lnc_decode, 0, "Car Action",
		   "Ivan Mackintosh\nKevin Brisley (Bump 'n' Jump driver)\nMirko Buffoni (Audio/Add. code)");
