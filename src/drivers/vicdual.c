/***************************************************************************

VIC Dual Game board memory map (preliminary)

0000-3fff ROM mirror image (not used in most games, apart from startup and NMI)
4000-7fff ROM
8000-87ff looks like extra work RAM in Safari
e000-e3ff Video RAM + work RAM
e400-e7ff RAM
e800-efff Character generator RAM

I/O ports:

The memory map is the same for alla games, but the I/O ports change. The
following ones are for Carnival, and apply to many other games as well.

read:
00        IN0
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = dsw
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

01        IN1
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = vblank
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

02        IN2
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = timer? is this used?
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

03        IN3
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = COIN (must reset the CPU to make the game acknowledge it)
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

write:
	(ports 1 and 2: see definitions in sound driver)

08        ?

40        palette bank

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "I8039/I8039.h"



#define	PSG_CLOCK_CARNIVAL	( 3579545 / 3 )	/* Hz */



extern unsigned char *vicdual_characterram;
void vicdual_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void vicdual_characterram_w(int offset,int data);
void vicdual_palette_bank_w(int offset, int data);
void vicdual_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void carnival_sh_port1_w(int offset, int data);
void carnival_sh_port2_w(int offset, int data);

int carnival_music_port_t1_r( int offset );

void carnival_music_port_1_w( int offset, int data );
void carnival_music_port_2_w( int offset, int data );



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, vicdual_characterram_w, &vicdual_characterram },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_2Aports[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x08, 0x08, input_port_1_r },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_2Bports[] =
{
	{ 0x03, 0x03, input_port_0_r },
	{ 0x08, 0x08, input_port_1_r },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_3ports[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x04, 0x04, input_port_1_r },
	{ 0x08, 0x08, input_port_2_r },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_4ports[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, carnival_sh_port1_w },
	{ 0x02, 0x02, carnival_sh_port2_w },
	{ 0x40, 0x40, vicdual_palette_bank_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress i8039_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress i8039_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort i8039_readport[] =
{
	{ I8039_t1, I8039_t1, carnival_music_port_t1_r },
	{ -1 }
};

static struct IOWritePort i8039_writeport[] =
{
	{ I8039_p1, I8039_p1, carnival_music_port_1_w },
	{ I8039_p2, I8039_p2, carnival_music_port_2_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( depthch_input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_DIPNAME (0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING (   0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING (   0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x30, "1 Coin/1 Credit" )
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( safari_input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Aim Up", OSD_KEY_A, 0, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3, "Aim Down", OSD_KEY_Z, 0, 0)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME (0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING (   0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING (   0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x30, "1 Coin/1 Credit" )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( sspaceat_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0e, 0x0e, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0e, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0a, "5" )
	PORT_DIPSETTING(    0x06, "6" )
/* the following are duplicates
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x02, "5" ) */
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x80, 0x00, "Show Credits", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( headon_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but used (sound related?) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START ( invho2_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Head On Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Head On Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x04, "6" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )	/* probably unused */

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	/* There's probably a bug in the code: this would likely be the second */
	/* bit of the Invinco Lives setting, but the game reads bit 3 instead */
	/* of bit 2. */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( invinco_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x80, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START ( invds_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+2" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Deep Scan Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	/* +1 and +2 gives 2 lives instead of 6 */
	PORT_DIPNAME( 0x04, 0x00, "Deep Scan Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+2" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( tranqgun_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( spacetrk_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but could be used */
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but could be used */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( carnival_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* unknown, but used */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( pulsar_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+2" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xe800, &charlayout, 0, 32 },	/* the game dynamically modifies this */
	{ -1 }	/* end of array */
};



static struct Samplesinterface samples_interface =
{
	10	/* 10 channels */
};



/* There are three version of the machine driver, identical apart from the readport */
/* array and interrupt handler */

#define MACHINEDRIVER(NAME,PORT)					\
static struct MachineDriver NAME =					\
{													\
	/* basic machine hardware */					\
	{												\
		{											\
			CPU_Z80,								\
			3867120/2,								\
			0,										\
			readmem,writemem,readport_##PORT,writeport,	\
			ignore_interrupt,1						\
		}											\
	},												\
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */	\
	1,	/* single CPU, no need for interleaving */	\
	0,												\
													\
	/* video hardware */							\
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },		\
	gfxdecodeinfo,									\
	64, 64,											\
	vicdual_vh_convert_color_prom,					\
													\
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,			\
	0,												\
	generic_vh_start,								\
	generic_vh_stop,								\
	vicdual_vh_screenrefresh,						\
													\
	/* sound hardware */							\
	0,0,0,0,										\
	{												\
		{											\
			SOUND_SAMPLES,							\
			&samples_interface						\
		}											\
	}												\
};

MACHINEDRIVER( vicdual_2Aports_machine_driver, 2Aports )
MACHINEDRIVER( vicdual_2Bports_machine_driver, 2Bports )
MACHINEDRIVER( vicdual_3ports_machine_driver, 3ports )
MACHINEDRIVER( vicdual_4ports_machine_driver, 4ports )


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chips */
	PSG_CLOCK_CARNIVAL,
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

/* don't know if any of the other games use the 8048 music board */
/* so, we won't burden those drivers with the extra music handling */
static struct MachineDriver carnival_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120/2,
			0,
			readmem,writemem,readport_4ports,writeport,
			ignore_interrupt,1
		},
		{
			CPU_I8039 | CPU_AUDIO_CPU,
			( ( 3579545 / 5 ) / 3 ),
			2,
			i8039_readmem,i8039_writemem,i8039_readport,i8039_writeport,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	64, 64,
	vicdual_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	vicdual_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( depthch_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "50A", 0x0000, 0x0400, 0x3ee2b370 )
	ROM_LOAD( "51A", 0x0400, 0x0400, 0xff162d40 )
	ROM_LOAD( "52",  0x0800, 0x0400, 0x83c5408f )
	ROM_LOAD( "53",  0x0c00, 0x0400, 0x5c669d5e )
	ROM_LOAD( "54A", 0x1000, 0x0400, 0x6eb911cd )
	ROM_LOAD( "55A", 0x1400, 0x0400, 0xf5482cea )
ROM_END

ROM_START( safari_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "3160066.u48", 0x0000, 0x0400, 0xfbe2f628 )
	ROM_LOAD( "3160065.u47", 0x0400, 0x0400, 0x67f77ab3 )
	ROM_LOAD( "3160064.u46", 0x0800, 0x0400, 0x671071a6 )
	ROM_LOAD( "3160063.u45", 0x0c00, 0x0400, 0x955d8de9 )
	ROM_LOAD( "3160062.u44", 0x1000, 0x0400, 0x467c2510 )
	ROM_LOAD( "3160061.u43", 0x1400, 0x0400, 0xa0d52b17 )
	ROM_LOAD( "3160060.u42", 0x1800, 0x0400, 0xe3369cec )
	ROM_LOAD( "3160059.u41", 0x1c00, 0x0400, 0x61ffd571 )
	ROM_LOAD( "3160058.u40", 0x2000, 0x0400, 0xf6bba995 )
	ROM_LOAD( "3160057.u39", 0x2400, 0x0400, 0x36529468 )
ROM_END

ROM_START( sspaceat_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "139.u27", 0x0000, 0x0400, 0xc3bde565 )
	ROM_RELOAD(          0x4000, 0x0400 )
	ROM_LOAD( "140.u26", 0x4400, 0x0400, 0xf7953eeb )
	ROM_LOAD( "141.u25", 0x4800, 0x0400, 0xac45484b )
	ROM_LOAD( "142.u24", 0x4c00, 0x0400, 0xc3bdcf07 )
	ROM_LOAD( "143.u23", 0x5000, 0x0400, 0x71d29b2c )
	ROM_LOAD( "144.u22", 0x5400, 0x0400, 0x479e9940 )
	ROM_LOAD( "145.u21", 0x5800, 0x0400, 0x47dbfeef )
	ROM_LOAD( "146.u20", 0x5c00, 0x0400, 0xdd530af1 )
ROM_END

ROM_START( headon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "163A", 0x0000, 0x0400, 0x811c8fa6 )
	ROM_RELOAD(       0x4000, 0x0400 )
	ROM_LOAD( "164A", 0x4400, 0x0400, 0x394368e9 )
	ROM_LOAD( "165A", 0x4800, 0x0400, 0x50226048 )
	ROM_LOAD( "166C", 0x4c00, 0x0400, 0x22bfca11 )
	ROM_LOAD( "167C", 0x5000, 0x0400, 0x5fe44090 )
	ROM_LOAD( "192A", 0x5400, 0x0400, 0xdbbd2f7f )
	ROM_LOAD( "193A", 0x5800, 0x0400, 0xa63e2ce2 )
ROM_END

ROM_START( invho2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "271b.u33", 0x0000, 0x0400, 0x1557248b )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "272b.u32", 0x4400, 0x0400, 0xa290a8dc )
	ROM_LOAD( "273b.u31", 0x4800, 0x0400, 0x0f5df4cf )
	ROM_LOAD( "274b.u30", 0x4c00, 0x0400, 0xfc5b979d )
	ROM_LOAD( "275b.u29", 0x5000, 0x0400, 0x6dd0c104 )
	ROM_LOAD( "276b.u28", 0x5400, 0x0400, 0x1e1eadf0 )
	ROM_LOAD( "277b.u27", 0x5800, 0x0400, 0xad1d5307 )
	ROM_LOAD( "278b.u26", 0x5c00, 0x0400, 0xe409f8d3 )
	ROM_LOAD( "279b.u8",  0x6000, 0x0400, 0x436f5afb )
	ROM_LOAD( "280b.u7",  0x6400, 0x0400, 0x71d90e01 )
	ROM_LOAD( "281b.u6",  0x6800, 0x0400, 0x37fa0188 )
	ROM_LOAD( "282b.u5",  0x6c00, 0x0400, 0xb83213f2 )
	ROM_LOAD( "283b.u4",  0x7000, 0x0400, 0xff7803c4 )
	ROM_LOAD( "284b.u3",  0x7400, 0x0400, 0x53f1cd8b )
	ROM_LOAD( "285b.u2",  0x7800, 0x0400, 0x58a0fe0e )
	ROM_LOAD( "286b.u1",  0x7C00, 0x0400, 0x8460e06c )
ROM_END

ROM_START( invinco_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "310a.u27", 0x0000, 0x0400, 0x0982135c )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "311a.u26", 0x4400, 0x0400, 0x17d6221c )
	ROM_LOAD( "312a.u25", 0x4800, 0x0400, 0x0280d0e8 )
	ROM_LOAD( "313a.u24", 0x4c00, 0x0400, 0xe9d6530e )
	ROM_LOAD( "314a.u23", 0x5000, 0x0400, 0x31f350ab )
	ROM_LOAD( "315a.u22", 0x5400, 0x0400, 0x0c6fffb1 )
	ROM_LOAD( "316a.u21", 0x5800, 0x0400, 0x9b5dfb21 )
	ROM_LOAD( "317a.u20", 0x5c00, 0x0400, 0x6b868418 )
	ROM_LOAD( "318a.uxx", 0x6000, 0x0400, 0x8c7227ea )
ROM_END

ROM_START( invds_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "367.u33", 0x0000, 0x0400, 0x8ea45a92 )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "368.u32", 0x4400, 0x0400, 0x55431ea5 )
	ROM_LOAD( "369.u31", 0x4800, 0x0400, 0x291a5996 )
	ROM_LOAD( "370.u30", 0x4c00, 0x0400, 0xb99bcbc1 )
	ROM_LOAD( "371.u29", 0x5000, 0x0400, 0x2fcbde79 )
	ROM_LOAD( "372.u28", 0x5400, 0x0400, 0xd06afa0c )
	ROM_LOAD( "373.u27", 0x5800, 0x0400, 0xb997bb8d )
	ROM_LOAD( "374.u26", 0x5c00, 0x0400, 0x051b17b5 )
	ROM_LOAD( "375.u8",  0x6000, 0x0400, 0x321ab590 )
	ROM_LOAD( "376.u7",  0x6400, 0x0400, 0xe8dc8206 )
	ROM_LOAD( "377.u6",  0x6800, 0x0400, 0xcbab8b9b )
	ROM_LOAD( "378.u5",  0x6c00, 0x0400, 0xd727a56b )
	ROM_LOAD( "379.u4",  0x7000, 0x0400, 0x554bafd5 )
	ROM_LOAD( "380.u3",  0x7400, 0x0400, 0x79e672c0 )
	ROM_LOAD( "381.u2",  0x7800, 0x0400, 0x8d194e23 )
	ROM_LOAD( "382.u1",  0x7C00, 0x0400, 0xa162274c )
ROM_END

ROM_START( tranqgun_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u33.bin", 0x0000, 0x0400, 0x3eeddfbf )
	ROM_LOAD( "u32.bin", 0x0400, 0x0400, 0x7e5df21d )
	ROM_LOAD( "u31.bin", 0x0800, 0x0400, 0xcee84712 )
	ROM_LOAD( "u30.bin", 0x0c00, 0x0400, 0x2172937a )
	ROM_LOAD( "u29.bin", 0x1000, 0x0400, 0x2bd38cad )
	ROM_LOAD( "u28.bin", 0x1400, 0x0400, 0x4f138de5 )
	ROM_LOAD( "u27.bin", 0x1800, 0x0400, 0x03f896b4 )
	ROM_LOAD( "u26.bin", 0x1c00, 0x0400, 0x95781332 )
	ROM_LOAD( "u8.bin",  0x2000, 0x0400, 0xea469a9c )
	ROM_LOAD( "u7.bin",  0x2400, 0x0400, 0xdb155d07 )
	ROM_LOAD( "u6.bin",  0x2800, 0x0400, 0xdf58678c )
	ROM_LOAD( "u5.bin",  0x2c00, 0x0400, 0xe428b710 )
	ROM_LOAD( "u4.bin",  0x3000, 0x0400, 0xc4a49bf0 )
	ROM_LOAD( "u3.bin",  0x3400, 0x0400, 0x8965d69f )
	ROM_LOAD( "u2.bin",  0x3800, 0x0400, 0xa3a7a675 )
	ROM_LOAD( "u1.bin",  0x3C00, 0x0400, 0xcd3f6e6f )
ROM_END

ROM_START( spacetrk_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u33.bin", 0x0000, 0x0400, 0x588d1157 )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "u32.bin", 0x4400, 0x0400, 0x41ef7817 )
	ROM_LOAD( "u31.bin", 0x4800, 0x0400, 0x71de526c )
	ROM_LOAD( "u30.bin", 0x4c00, 0x0400, 0x2587979b )
	ROM_LOAD( "u29.bin", 0x5000, 0x0400, 0x3251fdcd )
	ROM_LOAD( "u28.bin", 0x5400, 0x0400, 0x6af6d58a )
	ROM_LOAD( "u27.bin", 0x5800, 0x0400, 0x215b3f45 )
	ROM_LOAD( "u26.bin", 0x5c00, 0x0400, 0xbc8dc10f )
	ROM_LOAD( "u8.bin",  0x6000, 0x0400, 0xfbac89da )
	ROM_LOAD( "u7.bin",  0x6400, 0x0400, 0x103ba0f1 )
	ROM_LOAD( "u6.bin",  0x6800, 0x0400, 0x17bea0f4 )
	ROM_LOAD( "u5.bin",  0x6c00, 0x0400, 0x5477723d )
	ROM_LOAD( "u4.bin",  0x7000, 0x0400, 0x11df358f )
	ROM_LOAD( "u3.bin",  0x7400, 0x0400, 0xa930320a )
	ROM_LOAD( "u2.bin",  0x7800, 0x0400, 0x8f57b039 )
	ROM_LOAD( "u1.bin",  0x7C00, 0x0400, 0xbc3e52b8 )
ROM_END

ROM_START( carnival_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "651u33.cpu", 0x0000, 0x0400, 0x661fa2ef )
	ROM_RELOAD(             0x4000, 0x0400 )
	ROM_LOAD( "652u32.cpu", 0x4400, 0x0400, 0x5172c958 )
	ROM_LOAD( "653u31.cpu", 0x4800, 0x0400, 0x3771ff13 )
	ROM_LOAD( "654u30.cpu", 0x4c00, 0x0400, 0x6b0caeb6 )
	ROM_LOAD( "655u29.cpu", 0x5000, 0x0400, 0x15283224 )
	ROM_LOAD( "656u28.cpu", 0x5400, 0x0400, 0xec2fd83b )
	ROM_LOAD( "657u27.cpu", 0x5800, 0x0400, 0x9a30851e )
	ROM_LOAD( "658u26.cpu", 0x5c00, 0x0400, 0x914e4bf2 )
	ROM_LOAD( "659u8.cpu",  0x6000, 0x0400, 0x22c6547c )
	ROM_LOAD( "660u7.cpu",  0x6400, 0x0400, 0xba76241e )
	ROM_LOAD( "661u6.cpu",  0x6800, 0x0400, 0x3bab3df7 )
	ROM_LOAD( "662u5.cpu",  0x6c00, 0x0400, 0xc315eb83 )
	ROM_LOAD( "663u4.cpu",  0x7000, 0x0400, 0x434d30f7 )
	ROM_LOAD( "664u3.cpu",  0x7400, 0x0400, 0x75f1b261 )
	ROM_LOAD( "665u2.cpu",  0x7800, 0x0400, 0xecdd5165 )
	ROM_LOAD( "666u1.cpu",  0x7c00, 0x0400, 0x24809182 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x0800)
	ROM_LOAD( "crvl.snd",  0x0000, 0x0800, 0xd8240000 )
ROM_END

ROM_START( pulsar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "790.u33", 0x0000, 0x0400, 0xbc154259 )
	ROM_RELOAD(          0x4000, 0x0400 )
	ROM_LOAD( "791.u32", 0x4400, 0x0400, 0x8bc8bf1a )
	ROM_LOAD( "792.u31", 0x4800, 0x0400, 0x5b6fc8a9 )
	ROM_LOAD( "793.u30", 0x4c00, 0x0400, 0xb3b44342 )
	ROM_LOAD( "794.u29", 0x5000, 0x0400, 0xbe5d6fd1 )
	ROM_LOAD( "795.u28", 0x5400, 0x0400, 0x0b53ae73 )
	ROM_LOAD( "796.u27", 0x5800, 0x0400, 0xc3cfc4c5 )
	ROM_LOAD( "797.u26", 0x5c00, 0x0400, 0x9efb41b3 )
	ROM_LOAD( "798.u8",  0x6000, 0x0400, 0xcdae0cc0 )
	ROM_LOAD( "799.u7",  0x6400, 0x0400, 0x9617dc67 )
	ROM_LOAD( "800.u6",  0x6800, 0x0400, 0xb1ed255f )
	ROM_LOAD( "801.u5",  0x6C00, 0x0400, 0x140f4fff )
	ROM_LOAD( "802.u4",  0x7000, 0x0400, 0x181f535d )
	ROM_LOAD( "803.u3",  0x7400, 0x0400, 0x1ca50c4d )
	ROM_LOAD( "804.u2",  0x7800, 0x0400, 0x75482596 )
	ROM_LOAD( "805.u1",  0x7C00, 0x0400, 0x4215bd79 )
ROM_END



static unsigned char depthch_color_prom[] =
{
	/* Depth Charge is a b/w game, here is the PROM for Head On */
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
};

static unsigned char safari_color_prom[] =
{
	/* Safari is a b/w game, here is the PROM for Head On */
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
};

static unsigned char sspaceat_color_prom[] =
{
	/* Guessed palette! */
	0x60,0x40,0x40,0x20,0xC0,0x80,0xA0,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char headon_color_prom[] =
{
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
};

#if 0
/* ROMs for this game are not available yet */
static unsigned char ho2ds_color_prom[] =
{
	/* 306-283: palette */
	0x31,0xB1,0x71,0x31,0x31,0x31,0x31,0x31,0x91,0xF1,0x31,0xF1,0x51,0xB1,0x91,0xB1,
	0xF5,0x79,0x31,0xB9,0xF5,0xF5,0xB5,0x95,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31
};
#endif

static unsigned char invho2_color_prom[] =
{
	/* 306-287: palette */
	/* note: the image contained in the original archive was bad */
	/* (bit 4 always 0). This seems to be a better read. */
	0xF1,0xB1,0x71,0xF1,0xF1,0xF1,0xF1,0xF1,0x91,0xF1,0x31,0xF1,0x51,0xB1,0x91,0xB1,
	0xD1,0xB1,0x31,0x51,0xF1,0xB1,0x71,0x91,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1
};

static unsigned char invinco_color_prom[] =
{
	/* selected palette from the invho2 and invds PROMs */
	0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91,0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91,
	0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91,0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91
};

static unsigned char invds_color_prom[] =
{
	/* 306-246: palette */
	0x7f,0xb1,0x71,0x7f,0x7f,0x7f,0x7f,0x7f,0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91,
	0xf5,0x79,0x31,0xb9,0xf5,0xf5,0xb5,0x95,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

static unsigned char tranqgun_color_prom[] =
{
	/* PR-57: palette */
	/* was a bad read, this is just a guess! */
	0xc0,0xe0,0x60,0x60,0x80,0xa0,0x60,0x20,0xc0,0xe0,0x60,0x60,0x80,0xa0,0x60,0x20,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char spacetrk_color_prom[] =
{
	/* PR-80: palette */
	0xa0,0xe0,0xa0,0x80,0xa0,0xc0,0x20,0x60,0xa0,0xe0,0xa0,0x80,0xa0,0x80,0x20,0x60,
	0xa0,0xe0,0xa0,0x80,0xa0,0xc0,0x20,0x60,0xa4,0xe4,0xa4,0x84,0xa4,0xc4,0x24,0x64
};

static unsigned char carnival_color_prom[] =
{
	/* 316-633: palette */
	0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,
	0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60,0xE0,0xA0,0x80,0xA0,0x60,0xA0,0x20,0x60
};

static unsigned char pulsar_color_prom[] =
{
	/* 316-789: palette */
	0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91,0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91,
	0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91,0xD1,0x51,0x71,0xb1,0xf1,0xb1,0x31,0x91
};



static const char *carnival_sample_names[] =
{
	"bear.sam",
	"bonus1.sam",
	"bonus2.sam",
	"clang.sam",
	"duck1.sam",
	"duck2.sam",
	"duck3.sam",
	"pipehit.sam",
	"ranking.sam",
	"rifle.sam",
	NULL
};



static int carnival_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xE397],"\x00\x00\x00",3) == 0 &&
			memcmp(&RAM[0xE5A2],"   ",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Read the scores */
			osd_fread(f,&RAM[0xE397],2*30);
			/* Read the initials */
			osd_fread(f,&RAM[0xE5A2],9);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void carnival_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the scores */
		osd_fwrite(f,&RAM[0xE397],2*30);
		/* Save the initials */
		osd_fwrite(f,&RAM[0xE5A2],9);
		osd_fclose(f);
	}

}



struct GameDriver depthch_driver =
{
	__FILE__,
	0,
	"depthch",
	"Depth Charge",
	"1977",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Aports_machine_driver,

	depthch_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	depthch_input_ports,

	depthch_color_prom, 0, 0,

	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver safari_driver =
{
	__FILE__,
	0,
	"safari",
	"Safari",
	"1977",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Bports_machine_driver,

	safari_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	safari_input_ports,

	safari_color_prom, 0, 0,

	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver sspaceat_driver =
{
	__FILE__,
	0,
	"sspaceat",
	"Sega Space Attack",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_3ports_machine_driver,

	sspaceat_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sspaceat_input_ports,

	sspaceat_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver headon_driver =
{
	__FILE__,
	0,
	"headon",
	"Head On",
	"1979",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Aports_machine_driver,

	headon_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	headon_input_ports,

	headon_color_prom, 0, 0,

	ORIENTATION_DEFAULT,

	0, 0
};

/* No ROMs yet for the following one, but we have the color PROM. */
//GAMEDRIVER( ho2ds,    "Head On 2 / Deep Scan", vicdual_4ports_machine_driver,  0, ORIENTATION_ROTATE_270, 0, 0 )

struct GameDriver invho2_driver =
{
	__FILE__,
	0,
	"invho2",
	"Invinco / Head On 2",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,

	invho2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	invho2_input_ports,

	invho2_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver invinco_driver =
{
	__FILE__,
	0,
	"invinco",
	"Invinco",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_3ports_machine_driver,

	invinco_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	invinco_input_ports,

	invinco_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver invds_driver =
{
	__FILE__,
	0,
	"invds",
	"Invinco / Deep Scan",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,

	invds_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	invds_input_ports,

	invds_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver tranqgun_driver =
{
	__FILE__,
	0,
	"tranqgun",
	"Tranquilizer Gun",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,

	tranqgun_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tranqgun_input_ports,

	tranqgun_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver spacetrk_driver =
{
	__FILE__,
	0,
	"spacetrk",
	"Space Trek",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,

	spacetrk_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	spacetrk_input_ports,

	spacetrk_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver carnival_driver =
{
	__FILE__,
	0,
	"carnival",
	"Carnival",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari\nPeter Clare (sound)\nAlan J McCormick (sound)",
	0,
	&carnival_machine_driver,

	carnival_rom,
	0, 0,
	carnival_sample_names,
	0,	/* sound_prom */

	carnival_input_ports,

	carnival_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	carnival_hiload, carnival_hisave
};

struct GameDriver pulsar_driver =
{
	__FILE__,
	0,
	"pulsar",
	"Pulsar",
	"1981",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,

	pulsar_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pulsar_input_ports,

	pulsar_color_prom, 0, 0,

	ORIENTATION_ROTATE_270,

	0, 0
};
