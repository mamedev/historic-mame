/***************************************************************************

VIC Dual Game board memory map (preliminary)

0000-3fff ROM mirror image (not used in most games, apart from startup and NMI)
4000-7fff ROM
8000-87ff looks like extra work RAM in Safari
e000-e3ff Video RAM + work RAM
e400-e7ff RAM
e800-efff Character generator RAM

I/O ports:

The memory map is the same for many games, but the I/O ports change. The
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


static int protection_data;

void samurai_protection_w(int offset,int data)
{
	protection_data = data;
}

int samurai_input_r(int offset)
{
	int answer = 0;

	if (protection_data == 0xab) answer = 0x02;
	else if (protection_data == 0x1d) answer = 0x0c;

	return (readinputport(1 + offset) & 0xfd) | ((answer >> offset) & 0x02);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x7f00, 0x7f00, samurai_protection_w },	/* Samurai only */
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

static struct IOReadPort readport_samuraiports[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x03, samurai_input_r },	/* bit 1 of these ports returns a protection code */
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

INPUT_PORTS_START( frogs_input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )	/* maybe Demo Sounds */
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Free Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_DIPNAME( 0x20, 0x20, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x20, "90" )
	PORT_DIPNAME( 0x40, 0x40, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit" )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
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

INPUT_PORTS_START ( samurai_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )	/* seems to be on port 2 instead */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )	/* either vblank, or a timer. In the */
												/* Carnival schematics, it's a timer. */
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
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
MACHINEDRIVER( samurai_machine_driver, samuraiports )	/* this game has a simple protection */


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
	ROM_LOAD( "50a", 0x0000, 0x0400, 0x3ee2b370 , 0x56c5ffed )
	ROM_LOAD( "51a", 0x0400, 0x0400, 0xff162d40 , 0x695eb81f )
	ROM_LOAD( "52", 0x0800, 0x0400, 0x83c5408f , 0xaed0ba1b )
	ROM_LOAD( "53", 0x0c00, 0x0400, 0x5c669d5e , 0x2ccbd2d0 )
	ROM_LOAD( "54a", 0x1000, 0x0400, 0x6eb911cd , 0x1b7f6a43 )
	ROM_LOAD( "55a", 0x1400, 0x0400, 0xf5482cea , 0x9fc2eb41 )
ROM_END

ROM_START( safari_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "3160066.u48", 0x0000, 0x0400, 0xfbe2f628 , 0x2a26b098 )
	ROM_LOAD( "3160065.u47", 0x0400, 0x0400, 0x67f77ab3 , 0xb776f7db )
	ROM_LOAD( "3160064.u46", 0x0800, 0x0400, 0x671071a6 , 0x19d8c196 )
	ROM_LOAD( "3160063.u45", 0x0c00, 0x0400, 0x955d8de9 , 0x028bad25 )
	ROM_LOAD( "3160062.u44", 0x1000, 0x0400, 0x467c2510 , 0x504e0575 )
	ROM_LOAD( "3160061.u43", 0x1400, 0x0400, 0xa0d52b17 , 0xd4c528e0 )
	ROM_LOAD( "3160060.u42", 0x1800, 0x0400, 0xe3369cec , 0x48c7b0cc )
	ROM_LOAD( "3160059.u41", 0x1c00, 0x0400, 0x61ffd571 , 0x3f7baaff )
	ROM_LOAD( "3160058.u40", 0x2000, 0x0400, 0xf6bba995 , 0x0d5058f1 )
	ROM_LOAD( "3160057.u39", 0x2400, 0x0400, 0x36529468 , 0x298e8c41 )
ROM_END

ROM_START( frogs_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "119a.u48", 0x0000, 0x0400, 0xc97f5dbb , 0xb1d1fce4 )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "118a.u47", 0x4400, 0x0400, 0xbf0fc761 , 0x12fdcc05 )
	ROM_LOAD( "117a.u46", 0x4800, 0x0400, 0xc66ef1ae , 0x8a5be424 )
	ROM_LOAD( "116b.u45", 0x4c00, 0x0400, 0x72b7e385 , 0x09b82619 )
	ROM_LOAD( "115a.u44", 0x5000, 0x0400, 0xbed07bee , 0x3d4e4fa8 )
	ROM_LOAD( "114a.u43", 0x5400, 0x0400, 0x67ed7de3 , 0x04a21853 )
	ROM_LOAD( "113a.u42", 0x5800, 0x0400, 0x5ddd05db , 0x02786692 )
	ROM_LOAD( "112a.u41", 0x5c00, 0x0400, 0x9a8c25d4 , 0x0be2a058 )
ROM_END

ROM_START( sspaceat_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "139.u27", 0x0000, 0x0400, 0xc3bde565 , 0x9f2112fc )
	ROM_RELOAD(          0x4000, 0x0400 )
	ROM_LOAD( "140.u26", 0x4400, 0x0400, 0xf7953eeb , 0xddbeed35 )
	ROM_LOAD( "141.u25", 0x4800, 0x0400, 0xac45484b , 0xb159924d )
	ROM_LOAD( "142.u24", 0x4c00, 0x0400, 0xc3bdcf07 , 0xf2ebfce9 )
	ROM_LOAD( "143.u23", 0x5000, 0x0400, 0x71d29b2c , 0xbff34a66 )
	ROM_LOAD( "144.u22", 0x5400, 0x0400, 0x479e9940 , 0xfa062d58 )
	ROM_LOAD( "145.u21", 0x5800, 0x0400, 0x47dbfeef , 0x7e950614 )
	ROM_LOAD( "146.u20", 0x5c00, 0x0400, 0xdd530af1 , 0x8ba94fbc )
ROM_END

ROM_START( headon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "163a", 0x0000, 0x0400, 0x811c8fa6 , 0x4bb51259 )
	ROM_RELOAD(       0x4000, 0x0400 )
	ROM_LOAD( "164a", 0x4400, 0x0400, 0x394368e9 , 0xaeac8c5f )
	ROM_LOAD( "165a", 0x4800, 0x0400, 0x50226048 , 0xf1a0cb72 )
	ROM_LOAD( "166c", 0x4c00, 0x0400, 0x22bfca11 , 0x65d12951 )
	ROM_LOAD( "167c", 0x5000, 0x0400, 0x5fe44090 , 0x2280831e )
	ROM_LOAD( "192a", 0x5400, 0x0400, 0xdbbd2f7f , 0xed4666f2 )
	ROM_LOAD( "193a", 0x5800, 0x0400, 0xa63e2ce2 , 0x37a1df4c )
ROM_END

ROM_START( invho2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "271b.u33", 0x0000, 0x0400, 0x1557248b , 0x44356a73 )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "272b.u32", 0x4400, 0x0400, 0xa290a8dc , 0xbd251265 )
	ROM_LOAD( "273b.u31", 0x4800, 0x0400, 0x0f5df4cf , 0x2fc80cd9 )
	ROM_LOAD( "274b.u30", 0x4c00, 0x0400, 0xfc5b979d , 0x4fac4210 )
	ROM_LOAD( "275b.u29", 0x5000, 0x0400, 0x6dd0c104 , 0x85af508e )
	ROM_LOAD( "276b.u28", 0x5400, 0x0400, 0x1e1eadf0 , 0xe305843a )
	ROM_LOAD( "277b.u27", 0x5800, 0x0400, 0xad1d5307 , 0xb6b4221e )
	ROM_LOAD( "278b.u26", 0x5c00, 0x0400, 0xe409f8d3 , 0x74d42250 )
	ROM_LOAD( "279b.u8", 0x6000, 0x0400, 0x436f5afb , 0x8d30a3e0 )
	ROM_LOAD( "280b.u7", 0x6400, 0x0400, 0x71d90e01 , 0xb5ee60ec )
	ROM_LOAD( "281b.u6", 0x6800, 0x0400, 0x37fa0188 , 0x21a6d4f2 )
	ROM_LOAD( "282b.u5", 0x6c00, 0x0400, 0xb83213f2 , 0x07d54f8a )
	ROM_LOAD( "283b.u4", 0x7000, 0x0400, 0xff7803c4 , 0xbdbe7ec1 )
	ROM_LOAD( "284b.u3", 0x7400, 0x0400, 0x53f1cd8b , 0xae9e9f16 )
	ROM_LOAD( "285b.u2", 0x7800, 0x0400, 0x58a0fe0e , 0x8dc3ec34 )
	ROM_LOAD( "286b.u1", 0x7C00, 0x0400, 0x8460e06c , 0x4bab9ba2 )
ROM_END

ROM_START( samurai_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr289.u33", 0x0000, 0x0400, 0xcca8599a , 0xa1a9cb03 )
	ROM_RELOAD(             0x4000, 0x0400 )
	ROM_LOAD( "epr290.u32", 0x4400, 0x0400, 0x127e7fba , 0x49fede51 )
	ROM_LOAD( "epr291.u31", 0x4800, 0x0400, 0xf50158f1 , 0x6503dd72 )
	ROM_LOAD( "epr292.u30", 0x4c00, 0x0400, 0xf877166f , 0x179c224f )
	ROM_LOAD( "epr366.u29", 0x5000, 0x0400, 0x3ec67476 , 0x3df2abec )
	ROM_LOAD( "epr355.u28", 0x5400, 0x0400, 0x49ab7dab , 0xb24517a4 )
	ROM_LOAD( "epr367.u27", 0x5800, 0x0400, 0x6df6c5b2 , 0x992a6e5a )
	ROM_LOAD( "epr368.u26", 0x5c00, 0x0400, 0x923de237 , 0x403c72ce )
	ROM_LOAD( "epr369.u8", 0x6000, 0x0400, 0x6dfd6e8b , 0x3cfd115b )
	ROM_LOAD( "epr370.u7", 0x6400, 0x0400, 0xa5788dba , 0x2c30db12 )
	ROM_LOAD( "epr299.u6", 0x6800, 0x0400, 0xb2fed46a , 0x87c71139 )
	ROM_LOAD( "epr371.u5", 0x6c00, 0x0400, 0xce8e7b6c , 0x761f56cf )
	ROM_LOAD( "epr301.u4", 0x7000, 0x0400, 0x938ef5c6 , 0x23de1ff7 )
	ROM_LOAD( "epr372.u3", 0x7400, 0x0400, 0xbac246f2 , 0x292cfd89 )
ROM_END

ROM_START( invinco_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "310a.u27", 0x0000, 0x0400, 0x0982135c , 0xe3931365 )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "311a.u26", 0x4400, 0x0400, 0x17d6221c , 0xde1a6c4a )
	ROM_LOAD( "312a.u25", 0x4800, 0x0400, 0x0280d0e8 , 0xe3c08f39 )
	ROM_LOAD( "313a.u24", 0x4c00, 0x0400, 0xe9d6530e , 0xb680b306 )
	ROM_LOAD( "314a.u23", 0x5000, 0x0400, 0x31f350ab , 0x790f07d9 )
	ROM_LOAD( "315a.u22", 0x5400, 0x0400, 0x0c6fffb1 , 0x0d13bed2 )
	ROM_LOAD( "316a.u21", 0x5800, 0x0400, 0x9b5dfb21 , 0x88d7eab8 )
	ROM_LOAD( "317a.u20", 0x5c00, 0x0400, 0x6b868418 , 0x75389463 )
	ROM_LOAD( "318a.uxx", 0x6000, 0x0400, 0x8c7227ea , 0x0780721d )
ROM_END

ROM_START( invds_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "367.u33", 0x0000, 0x0400, 0x8ea45a92 , 0xe6a33eae )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "368.u32", 0x4400, 0x0400, 0x55431ea5 , 0x421554a8 )
	ROM_LOAD( "369.u31", 0x4800, 0x0400, 0x291a5996 , 0x531e917a )
	ROM_LOAD( "370.u30", 0x4c00, 0x0400, 0xb99bcbc1 , 0x2ad68f8c )
	ROM_LOAD( "371.u29", 0x5000, 0x0400, 0x2fcbde79 , 0x1b98dc5c )
	ROM_LOAD( "372.u28", 0x5400, 0x0400, 0xd06afa0c , 0x3a72190a )
	ROM_LOAD( "373.u27", 0x5800, 0x0400, 0xb997bb8d , 0x3d361520 )
	ROM_LOAD( "374.u26", 0x5c00, 0x0400, 0x051b17b5 , 0xe606e7d9 )
	ROM_LOAD( "375.u8", 0x6000, 0x0400, 0x321ab590 , 0xadbe8d32 )
	ROM_LOAD( "376.u7", 0x6400, 0x0400, 0xe8dc8206 , 0x79409a46 )
	ROM_LOAD( "377.u6", 0x6800, 0x0400, 0xcbab8b9b , 0x3f021a71 )
	ROM_LOAD( "378.u5", 0x6c00, 0x0400, 0xd727a56b , 0x49a542b0 )
	ROM_LOAD( "379.u4", 0x7000, 0x0400, 0x554bafd5 , 0xee140e49 )
	ROM_LOAD( "380.u3", 0x7400, 0x0400, 0x79e672c0 , 0x688ba831 )
	ROM_LOAD( "381.u2", 0x7800, 0x0400, 0x8d194e23 , 0x798ba0c7 )
	ROM_LOAD( "382.u1", 0x7C00, 0x0400, 0xa162274c , 0x8d195c24 )
ROM_END

ROM_START( tranqgun_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u33.bin", 0x0000, 0x0400, 0x3eeddfbf , 0x6d50e902 )
	ROM_LOAD( "u32.bin", 0x0400, 0x0400, 0x7e5df21d , 0xf0ba0e60 )
	ROM_LOAD( "u31.bin", 0x0800, 0x0400, 0xcee84712 , 0x9fe440d3 )
	ROM_LOAD( "u30.bin", 0x0c00, 0x0400, 0x2172937a , 0x1041608e )
	ROM_LOAD( "u29.bin", 0x1000, 0x0400, 0x2bd38cad , 0xfb5de95f )
	ROM_LOAD( "u28.bin", 0x1400, 0x0400, 0x4f138de5 , 0x03fd8727 )
	ROM_LOAD( "u27.bin", 0x1800, 0x0400, 0x03f896b4 , 0x3d93239b )
	ROM_LOAD( "u26.bin", 0x1c00, 0x0400, 0x95781332 , 0x20f64a7f )
	ROM_LOAD( "u8.bin", 0x2000, 0x0400, 0xea469a9c , 0x5121c695 )
	ROM_LOAD( "u7.bin", 0x2400, 0x0400, 0xdb155d07 , 0xb13d21f7 )
	ROM_LOAD( "u6.bin", 0x2800, 0x0400, 0xdf58678c , 0x603cee59 )
	ROM_LOAD( "u5.bin", 0x2c00, 0x0400, 0xe428b710 , 0x7f25475f )
	ROM_LOAD( "u4.bin", 0x3000, 0x0400, 0xc4a49bf0 , 0x57dc3123 )
	ROM_LOAD( "u3.bin", 0x3400, 0x0400, 0x8965d69f , 0x7aa7829b )
	ROM_LOAD( "u2.bin", 0x3800, 0x0400, 0xa3a7a675 , 0xa9b10df5 )
	ROM_LOAD( "u1.bin", 0x3C00, 0x0400, 0xcd3f6e6f , 0x431a7449 )
ROM_END

ROM_START( spacetrk_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u33.bin", 0x0000, 0x0400, 0x588d1157 , 0x9033fe50 )
	ROM_RELOAD(           0x4000, 0x0400 )
	ROM_LOAD( "u32.bin", 0x4400, 0x0400, 0x41ef7817 , 0x08f61f0d )
	ROM_LOAD( "u31.bin", 0x4800, 0x0400, 0x71de526c , 0x1088a8c4 )
	ROM_LOAD( "u30.bin", 0x4c00, 0x0400, 0x2587979b , 0x55560cc8 )
	ROM_LOAD( "u29.bin", 0x5000, 0x0400, 0x3251fdcd , 0x71713958 )
	ROM_LOAD( "u28.bin", 0x5400, 0x0400, 0x6af6d58a , 0x7bcf5ca3 )
	ROM_LOAD( "u27.bin", 0x5800, 0x0400, 0x215b3f45 , 0xad7a2065 )
	ROM_LOAD( "u26.bin", 0x5c00, 0x0400, 0xbc8dc10f , 0x6060fe77 )
	ROM_LOAD( "u8.bin", 0x6000, 0x0400, 0xfbac89da , 0x75a90624 )
	ROM_LOAD( "u7.bin", 0x6400, 0x0400, 0x103ba0f1 , 0x7b31a2ab )
	ROM_LOAD( "u6.bin", 0x6800, 0x0400, 0x17bea0f4 , 0x94135b33 )
	ROM_LOAD( "u5.bin", 0x6c00, 0x0400, 0x5477723d , 0xcfbf2538 )
	ROM_LOAD( "u4.bin", 0x7000, 0x0400, 0x11df358f , 0xb4b95129 )
	ROM_LOAD( "u3.bin", 0x7400, 0x0400, 0xa930320a , 0x03ca1d70 )
	ROM_LOAD( "u2.bin", 0x7800, 0x0400, 0x8f57b039 , 0xa968584b )
	ROM_LOAD( "u1.bin", 0x7C00, 0x0400, 0xbc3e52b8 , 0xe6e300e8 )
ROM_END

ROM_START( carnival_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "651u33.cpu", 0x0000, 0x0400, 0x661fa2ef , 0xa98d5f3d )
	ROM_RELOAD(             0x4000, 0x0400 )
	ROM_LOAD( "652u32.cpu", 0x4400, 0x0400, 0x5172c958 , 0xacb73519 )
	ROM_LOAD( "653u31.cpu", 0x4800, 0x0400, 0x3771ff13 , 0xbd179c15 )
	ROM_LOAD( "654u30.cpu", 0x4c00, 0x0400, 0x6b0caeb6 , 0x59f75cc0 )
	ROM_LOAD( "655u29.cpu", 0x5000, 0x0400, 0x15283224 , 0x92f6c07d )
	ROM_LOAD( "656u28.cpu", 0x5400, 0x0400, 0xec2fd83b , 0x410914ac )
	ROM_LOAD( "657u27.cpu", 0x5800, 0x0400, 0x9a30851e , 0xc99a9bbd )
	ROM_LOAD( "658u26.cpu", 0x5c00, 0x0400, 0x914e4bf2 , 0xd2e4925a )
	ROM_LOAD( "659u8.cpu", 0x6000, 0x0400, 0x22c6547c , 0x3a894cfe )
	ROM_LOAD( "660u7.cpu", 0x6400, 0x0400, 0xba76241e , 0x8b792ab7 )
	ROM_LOAD( "661u6.cpu", 0x6800, 0x0400, 0x3bab3df7 , 0x261c3ca0 )
	ROM_LOAD( "662u5.cpu", 0x6c00, 0x0400, 0xc315eb83 , 0x59247fa2 )
	ROM_LOAD( "663u4.cpu", 0x7000, 0x0400, 0x434d30f7 , 0x42787079 )
	ROM_LOAD( "664u3.cpu", 0x7400, 0x0400, 0x75f1b261 , 0xe0ff6e25 )
	ROM_LOAD( "665u2.cpu", 0x7800, 0x0400, 0xecdd5165 , 0xc5868119 )
	ROM_LOAD( "666u1.cpu", 0x7c00, 0x0400, 0x24809182 , 0x4846ae8b )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x0800)
	ROM_LOAD( "crvl.snd", 0x0000, 0x0800, 0xd8240000 , 0xaded4170 )
ROM_END

ROM_START( pulsar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "790.u33", 0x0000, 0x0400, 0xbc154259 , 0x5e3816da )
	ROM_RELOAD(          0x4000, 0x0400 )
	ROM_LOAD( "791.u32", 0x4400, 0x0400, 0x8bc8bf1a , 0xce0aee83 )
	ROM_LOAD( "792.u31", 0x4800, 0x0400, 0x5b6fc8a9 , 0x72d78cf1 )
	ROM_LOAD( "793.u30", 0x4c00, 0x0400, 0xb3b44342 , 0x42155dd4 )
	ROM_LOAD( "794.u29", 0x5000, 0x0400, 0xbe5d6fd1 , 0x11c7213a )
	ROM_LOAD( "795.u28", 0x5400, 0x0400, 0x0b53ae73 , 0xd2f02e29 )
	ROM_LOAD( "796.u27", 0x5800, 0x0400, 0xc3cfc4c5 , 0x67737a2e )
	ROM_LOAD( "797.u26", 0x5c00, 0x0400, 0x9efb41b3 , 0xec250b24 )
	ROM_LOAD( "798.u8", 0x6000, 0x0400, 0xcdae0cc0 , 0x1d34912d )
	ROM_LOAD( "799.u7", 0x6400, 0x0400, 0x9617dc67 , 0xf5695e4c )
	ROM_LOAD( "800.u6", 0x6800, 0x0400, 0xb1ed255f , 0xbf91ad92 )
	ROM_LOAD( "801.u5", 0x6C00, 0x0400, 0x140f4fff , 0x1e9721dc )
	ROM_LOAD( "802.u4", 0x7000, 0x0400, 0x181f535d , 0xd32d2192 )
	ROM_LOAD( "803.u3", 0x7400, 0x0400, 0x1ca50c4d , 0x3ede44d5 )
	ROM_LOAD( "804.u2", 0x7800, 0x0400, 0x75482596 , 0x62847b01 )
	ROM_LOAD( "805.u1", 0x7C00, 0x0400, 0x4215bd79 , 0xab418e86 )
ROM_END



static unsigned char bw_color_prom[] =
{
	/* for b/w games, let's use the Head On PROM */
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

static unsigned char samurai_color_prom[] =
{
	/* PR55.CLR: palette */
	0xE0,0xE0,0x84,0x80,0xE0,0xA0,0x60,0x40,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x40,0xA0,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
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

	bw_color_prom, 0, 0,

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

	bw_color_prom, 0, 0,

	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver frogs_driver =
{
	__FILE__,
	0,
	"frogs",
	"Frogs",
	"1978",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Aports_machine_driver,

	frogs_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	frogs_input_ports,

	bw_color_prom, 0, 0,

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

struct GameDriver samurai_driver =
{
	__FILE__,
	0,
	"samurai",
	"Samurai",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&samurai_machine_driver,

	samurai_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	samurai_input_ports,

	samurai_color_prom, 0, 0,

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
