/***************************************************************************

Space Invaders memory map (preliminary)

0000-1fff ROM
2000-23ff RAM
2400-3fff Video RAM
4000-57ff ROM (some clones use more, others less)


I/O ports:
read:
01        IN0
02        IN1
03        bit shift register read

write:
02        shift amount
03        ?
04        shift data
05        ?
06        ?

Space Invaders
--------------
Input:
Port 1

   bit 0 = CREDIT (0 if deposit)
   bit 1 = 2P start(1 if pressed)
   bit 2 = 1P start(1 if pressed)
   bit 3 = 0 if TILT
   bit 4 = shot(1 if pressed)
   bit 5 = left(1 if pressed)
   bit 6 = right(1 if pressed)
   bit 7 = Always 1

Port 2
   bit 0 = 00 = 3 ships  10 = 5 ships
   bit 1 = 01 = 4 ships  11 = 6 ships
   bit 2 = Always 0 (1 if TILT)
   bit 3 = 0 = extra ship at 1500, 1 = extra ship at 1000
   bit 4 = shot player 2 (1 if pressed)
   bit 5 = left player 2 (1 if pressed)
   bit 6 = right player 2 (1 if pressed)
   bit 7 = Coin info if 0, last screen


Space Invaders Deluxe
---------------------

This info was gleaned from examining the schematics of the SID
manual which Mitchell M. Rohde keeps at:

http://www.eecs.umich.edu/~bovine/space_invaders/index.html

Input:
Port 0
   bit 0 = SW 8 unused
   bit 1 = Always 0
   bit 2 = Always 1
   bit 3 = Always 0
   bit 4 = Always 1
   bit 5 = Always 1
   bit 6 = 1, Name Reset when set to zero
   bit 7 = Always 1

Port 1
   bit 0 = CREDIT (0 if deposit)
   bit 1 = 2P start(1 if pressed)
   bit 2 = 1P start(1 if pressed)
   bit 3 = 0 if TILT
   bit 4 = shot(1 if pressed)
   bit 5 = left(1 if pressed)
   bit 6 = right(1 if pressed)
   bit 7 = Always 1

Port 2
   bit 0 = Number of bases 0 - 3 , 1 - 4  (SW 4)
   bit 1 = 1 or 0 connected to SW 3 not used
   bit 2 = Always 0  (1 if TILT)
   bit 3 = Preset         (SW 2)
   bit 4 = shot player 2 (1 if pressed)  These are also used
   bit 5 = left player 2 (1 if pressed)  to enter the name in the
   bit 6 = right player 2 (1 if pressed) high score field.
   bit 7 = Coin info if 0 (SW 1) last screen

Invaders and Invaders Deluxe both write to I/O port 6 but
I don't know why.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int invaders_shift_data_r(int offset);
void invaders_shift_amount_w(int offset,int data);
void invaders_shift_data_w(int offset,int data);
int invaders_interrupt(void);

extern unsigned char *invaders_videoram;
void invaders_videoram_w(int offset,int data);
void lrescue_videoram_w(int offset,int data);    /* V.V */
void invrvnge_videoram_w(int offset,int data);   /* V.V */
void rollingc_videoram_w(int offset,int data);   /* L.T */
int invaders_vh_start(void);
void invaders_vh_stop(void);
void invaders_vh_screenrefresh(struct osd_bitmap *bitmap);
void invaders_sh_port3_w(int offset,int data);
void invaders_sh_port5_w(int offset,int data);
void invaders_sh_update(void);




static struct MemoryReadAddress readmem[] =
{
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x57ff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invaders_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress rollingc_readmem[] =
{
//	{ 0x2000, 0x2002, MRA_RAM },
//	{ 0x2003, 0x2003, hack },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0xa400, 0xbfff, MRA_RAM },
	{ 0xe400, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rollingc_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, rollingc_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0xa400, 0xbfff, MWA_RAM },
	{ 0xe400, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress lrescue_writemem[] = /* V.V */ /* Whole function */
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress invrvnge_writemem[] = /* V.V */ /* Whole function */
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invrvnge_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};


static struct IOReadPort invdelux_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

/* Catch the write to unmapped I/O port 6 */
void invaders_dummy_write(int offset,int data)
{
}

static struct IOWritePort writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ 0x05, 0x05, invaders_sh_port5_w },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};

static struct IOWritePort invdelux_writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ 0x05, 0x05, invaders_sh_port5_w },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( invaders_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

INPUT_PORTS_START( earthinv_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown DSW 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown DSW 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/1 Credit" )
INPUT_PORTS_END

INPUT_PORTS_START( spaceatt_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

INPUT_PORTS_START( galxwars_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

INPUT_PORTS_START( lrescue_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

INPUT_PORTS_START( invdelux_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* N ? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

INPUT_PORTS_START( invadpt2_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

INPUT_PORTS_START( desterth_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

INPUT_PORTS_START( cosmicmo_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
INPUT_PORTS_END

INPUT_PORTS_START( spaceph_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Fuel", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "35000" )
	PORT_DIPSETTING(    0x02, "25000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt */
	PORT_DIPNAME( 0x08, 0x00, "Bonus Fuel", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "10000" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Fire */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Left */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Right */
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
INPUT_PORTS_END

INPUT_PORTS_START( rollingc_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) /* Game Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) /* Game Select */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Player 1 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY ) /* Player 1 Controls */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY ) /* Player 1 Controls */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt ? */
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Player 2 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) /* Player 2 Controls */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) /* Player 2 Controls */
	PORT_DIPNAME( 0x80, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

static unsigned char palette[] = /* V.V */ /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0x20,0x20, /* RED */
	0x20,0xff,0x20, /* GREEN */
	0xff,0xff,0x20, /* YELLOW */
	0xff,0xff,0xff, /* WHITE */
	0x20,0xff,0xff, /* CYAN */
	0xff,0x20,0xff  /* PURPLE */
};

enum { BLACK,RED,GREEN,YELLOW,WHITE,CYAN,PURPLE }; /* V.V */


static unsigned char rollingc_palette[] =
{
	0xff,0xff,0xff, /* BLACK */
	0xa0,0xa0,0xa0, /* ?????? */
	0x00,0xff,0x00, /* GREEN */
	0x00,0xa0,0xa0, /* GREEN */
	0xff,0x00,0xff, /* PURPLE */
	0x00,0xff,0xff, /* CYAN */
	0xa0,0x00,0xa0, /* WHITE */
	0xff,0x00,0x00, /* RED */
	0xff,0xff,0x00,  /* YELLOW */
	0x80,0x40,0x20, /* ?????? */
	0x80,0x80,0x00, /* GREEN */
	0x00,0x80,0x80, /* GREEN */
	0xa0,0xa0,0xff, /* PURPLE */
	0xa0,0xff,0x80, /* CYAN */
	0xff,0xff,0x00, /* WHITE */
	0xff,0x00,0xa0, /* RED */
	0x00,0x00,0x00 /* RED */

};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 1 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	invaders_sh_update
};


static struct MachineDriver lrescue_machine_driver = /* V.V */ /* Whole function */
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			readmem,lrescue_writemem,readport,writeport, /* V.V */
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	invaders_sh_update
};


static struct MachineDriver invrvnge_machine_driver = /* V.V */ /* Whole function */
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			readmem,invrvnge_writemem,readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	invaders_sh_update
};



static struct MachineDriver invdelux_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			readmem,writemem,invdelux_readport, invdelux_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	invaders_sh_update
};



static struct MachineDriver rollingc_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 1 Mhz? */
			0,
			rollingc_readmem,rollingc_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(rollingc_palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	invaders_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( invaders_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invaders.h", 0x0000, 0x0800, 0x50d4f038 )
	ROM_LOAD( "invaders.g", 0x0800, 0x0800, 0x0d20437a )
	ROM_LOAD( "invaders.f", 0x1000, 0x0800, 0x5175f639 )
	ROM_LOAD( "invaders.e", 0x1800, 0x0800, 0x1206db68 )
ROM_END

ROM_START( earthinv_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "invaders.h", 0x0000, 0x0800, 0xec409c20 )
	ROM_LOAD( "invaders.g", 0x0800, 0x0800, 0x46a1b083 )
	ROM_LOAD( "invaders.f", 0x1000, 0x0800, 0xfcb8d54c )
	ROM_LOAD( "invaders.e", 0x1800, 0x0800, 0x38dff747 )
ROM_END

ROM_START( spaceatt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "spaceatt.h", 0x0000, 0x0800, 0x607fceab )
	ROM_LOAD( "spaceatt.g", 0x0800, 0x0800, 0x47146b7c )
	ROM_LOAD( "spaceatt.f", 0x1000, 0x0800, 0xf7306b0a )
	ROM_LOAD( "spaceatt.e", 0x1800, 0x0800, 0xb362b53c )
ROM_END

ROM_START( invrvnge_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invrvnge.h", 0x0000, 0x0800, 0xe6a7b0ab )
	ROM_LOAD( "invrvnge.g", 0x0800, 0x0800, 0xc3355da1 )
	ROM_LOAD( "invrvnge.f", 0x1000, 0x0800, 0x81b0bac0 )
	ROM_LOAD( "invrvnge.e", 0x1800, 0x0800, 0xe90b9c6f )
ROM_END

ROM_START( invdelux_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invdelux.h", 0x0000, 0x0800, 0x5b009eba )
	ROM_LOAD( "invdelux.g", 0x0800, 0x0800, 0x5f0db2f5 )
	ROM_LOAD( "invdelux.f", 0x1000, 0x0800, 0x0204561c )
	ROM_LOAD( "invdelux.e", 0x1800, 0x0800, 0x8a364d1a )
	ROM_LOAD( "invdelux.d", 0x4000, 0x0800, 0x5239e87f )
ROM_END

ROM_START( galxwars_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "galxwars.0", 0x0000, 0x0400, 0x3ad5521b )
	ROM_LOAD( "galxwars.1", 0x0400, 0x0400, 0xce40024c )
	ROM_LOAD( "galxwars.2", 0x0800, 0x0400, 0xe3dff58d )
	ROM_LOAD( "galxwars.3", 0x0c00, 0x0400, 0x93490efd )
	ROM_LOAD( "galxwars.4", 0x4000, 0x0400, 0x83edc2fd )
	ROM_LOAD( "galxwars.5", 0x4400, 0x0400, 0xc1234a3d )
ROM_END

ROM_START( lrescue_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lrescue.1", 0x0000, 0x0800, 0x9fa38d01 )
	ROM_LOAD( "lrescue.2", 0x0800, 0x0800, 0x3d00e2b8 )
	ROM_LOAD( "lrescue.3", 0x1000, 0x0800, 0x2fc56073 )
	ROM_LOAD( "lrescue.4", 0x1800, 0x0800, 0xc2170fa3 )
	ROM_LOAD( "lrescue.5", 0x4000, 0x0800, 0x238ce80c )
	ROM_LOAD( "lrescue.6", 0x4800, 0x0800, 0x464afa4a )
ROM_END

ROM_START( desterth_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "36_h.bin", 0x0000, 0x0800, 0x386b443d )
	ROM_LOAD( "35_g.bin", 0x0800, 0x0800, 0x838c80be )
	ROM_LOAD( "34_f.bin", 0x1000, 0x0800, 0x1fc49074 )
	ROM_LOAD( "33_e.bin", 0x1800, 0x0800, 0x7bb1a60f )
	ROM_LOAD( "32_d.bin", 0x4000, 0x0800, 0x5a2d5195 )
	ROM_LOAD( "31_c.bin", 0x4800, 0x0800, 0x8c9da1e1 )
	ROM_LOAD( "42_b.bin", 0x5000, 0x0800, 0xcea0bb2c )
ROM_END

ROM_START( invaders2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "pv.01", 0x0000, 0x0800,0x8b00eeba )
	ROM_LOAD( "pv.02", 0x0800, 0x0800,0x0c7adc30 )
	ROM_LOAD( "pv.03", 0x1000, 0x0800,0xb2384c12 )
	ROM_LOAD( "pv.04", 0x1800, 0x0800,0x14411f47 )
	ROM_LOAD( "pv.05", 0x4000, 0x0800,0x2990da7a )
ROM_END

ROM_START( cosmicmo_rom )  /* L.T */
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "cosmicmo.1", 0x0000, 0x0400,0x3ccca2e4 )
	ROM_LOAD( "cosmicmo.2", 0x0400, 0x0400,0xc8ecbfbc )
	ROM_LOAD( "cosmicmo.3", 0x0800, 0x0400,0xd9e97659 )
	ROM_LOAD( "cosmicmo.4", 0x0c00, 0x0400,0xa1c2ead0 )
	ROM_LOAD( "cosmicmo.5", 0x4000, 0x0400,0xff83f719 )
	ROM_LOAD( "cosmicmo.6", 0x4400, 0x0400,0x79df7b39 )
	ROM_LOAD( "cosmicmo.7", 0x4800, 0x0400,0x95dd75a7 )
ROM_END

ROM_START( spaceph_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sv01.bin", 0x0000, 0x0400, 0x5e769e98 )
	ROM_LOAD( "sv02.bin", 0x0400, 0x0400, 0xaaacc506 )
	ROM_LOAD( "sv03.bin", 0x0800, 0x0400, 0x734dec43 )
	ROM_LOAD( "sv04.bin", 0x0c00, 0x0400, 0xf16661e4 )
	ROM_LOAD( "sv05.bin", 0x1000, 0x0400, 0x02ba1408 )
	ROM_LOAD( "sv06.bin", 0x1400, 0x0400, 0xcab9ea59 )
	ROM_LOAD( "sv07.bin", 0x1800, 0x0400, 0xc8f1c563 )
	ROM_LOAD( "sv08.bin", 0x1c00, 0x0400, 0x923d6d79 )
	ROM_LOAD( "sv09.bin", 0x4000, 0x0400, 0xae6f21e9 )
	ROM_LOAD( "sv10.bin", 0x4400, 0x0400, 0x1176e23a )
	ROM_LOAD( "sv11.bin", 0x4800, 0x0400, 0xbbb49a42 )
	ROM_LOAD( "sv12.bin", 0x4c00, 0x0400, 0xca632a7b )
ROM_END

ROM_START( rollingc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "rc01.bin", 0x0000, 0x0400, 0x6fcfc861 )
	ROM_LOAD( "rc02.bin", 0x0400, 0x0400, 0xaa7205d8 )
	ROM_LOAD( "rc03.bin", 0x0800, 0x0400, 0xb1d8f7c8 )
	ROM_LOAD( "rc04.bin", 0x0c00, 0x0400, 0x2637f611 )
	ROM_LOAD( "rc05.bin", 0x1000, 0x0400, 0x82c0fb70 )
	ROM_LOAD( "rc06.bin", 0x1400, 0x0400, 0x52da00c8 )
	ROM_LOAD( "rc07.bin", 0x1800, 0x0400, 0xf0c569f7 )
	ROM_LOAD( "rc08.bin", 0x1c00, 0x0400, 0x033ac1a8 )
	ROM_LOAD( "rc09.bin", 0x4000, 0x0800, 0x4c3b8fe3 )
	ROM_LOAD( "rc10.bin", 0x4800, 0x0800, 0xceebaefd )
	ROM_LOAD( "rc11.bin", 0x5000, 0x0800, 0x68228d74 )
	ROM_LOAD( "rc12.bin", 0x5800, 0x0800, 0xdd2c4d68 )
ROM_END



static const char *invaders_sample_names[] =
{
	"*invaders",
	"0.SAM",
	"1.SAM",
	"2.SAM",
	"3.SAM",
	"4.SAM",
	"5.SAM",
	"6.SAM",
	"7.SAM",
	"8.SAM",
	0       /* end of array */
};



static int invaders_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20f4],"\x00\x00",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x20f4],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void invaders_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x20f4],2);
		osd_fclose(f);
	}
}


static int invdelux_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2340],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
	 /* Load the actual score */
		   osd_fread(f,&RAM[0x20f4], 0x2);
	 /* Load the name */
		   osd_fread(f,&RAM[0x2340], 0xa);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invdelux_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
      /* Save the actual score */
		osd_fwrite(f,&RAM[0x20f4], 0x2);
      /* Save the name */
		osd_fwrite(f,&RAM[0x2340], 0xa);
		osd_fclose(f);
	}
}

static int invrvnge_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2019],"\x00\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2019],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void invrvnge_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2019],3);
		osd_fclose(f);
	}
}

static int galxwars_hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2004],"\x00\x00",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2004],7);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void galxwars_hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2004],7);
		osd_fclose(f);
	}
}

static int lrescue_hiload(void)     /* V.V */ /* Whole function */
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	       {
	 /* Load the actual score */
		   osd_fread(f,&RAM[0x20F4], 0x2);
	 /* Load the name */
		   osd_fread(f,&RAM[0x20CF], 0xa);
	 /* Load the high score length */
		   osd_fread(f,&RAM[0x20DB], 0x1);
		   osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */

}

static void lrescue_hisave(void)    /* V.V */ /* Whole function */
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
      /* Save the actual score */
		osd_fwrite(f,&RAM[0x20F4],0x02);
      /* Save the name */
		osd_fwrite(f,&RAM[0x20CF],0xa);
      /* Save the high score length */
		osd_fwrite(f,&RAM[0x20DB],0x1);
		osd_fclose(f);
	}
}

static int desterth_hiload(void)     /* V.V */ /* Whole function */
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x07",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	       {
	 /* Load the actual score */
		   osd_fread(f,&RAM[0x20F4], 0x2);
	 /* Load the name */
		   osd_fread(f,&RAM[0x20CF], 0xa);
	 /* Load the high score length */
		   osd_fread(f,&RAM[0x20DB], 0x1);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}



struct GameDriver invaders_driver =
{
	"Space Invaders",
	"invaders",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	&machine_driver,

	invaders_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, invaders_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	invaders_hiload, invaders_hisave
};

struct GameDriver earthinv_driver =
{
	"Super Earth Invasion",
	"earthinv",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nMarco Cassili",
	&machine_driver,

	earthinv_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, earthinv_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver spaceatt_driver =
{
	"Space Attack II",
	"spaceatt",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	&machine_driver,

	spaceatt_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, spaceatt_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	invaders_hiload, invaders_hisave
};

struct GameDriver invrvnge_driver =
{
	"Invaders Revenge",
	"invrvnge",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nMarco Cassili (high score save)",
	&invrvnge_machine_driver,

	invrvnge_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, desterth_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	invrvnge_hiload, invrvnge_hisave
};

struct GameDriver invdelux_driver =
{
	"Space Invaders Part II (Midway)",
	"invdelux",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	&invdelux_machine_driver,

	invdelux_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, invdelux_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	invdelux_hiload, invdelux_hisave
};

/* LT 24-11-1997 */
struct GameDriver invadpt2_driver =
{
	"Space Invaders Part II (Taito)",
	"invadpt2",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nMarco Cassili",
	&lrescue_machine_driver,

	invaders2_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, invadpt2_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

        invdelux_hiload, invdelux_hisave
};


struct GameDriver galxwars_driver =
{
	"Galaxy Wars",
	"galxwars",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	&machine_driver,

	galxwars_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, galxwars_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	galxwars_hiload, galxwars_hisave
};

struct GameDriver lrescue_driver =
{
	"Lunar Rescue",
	"lrescue",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	&lrescue_machine_driver,

	lrescue_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, lrescue_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	lrescue_hiload, lrescue_hisave  /* V.V */
};


struct GameDriver desterth_driver =
{
	"Destination Earth",
	"desterth",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
        &machine_driver,

	desterth_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, desterth_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	desterth_hiload, lrescue_hisave
};


struct GameDriver cosmicmo_driver =
{
	"Cosmic Monsters",
	"cosmicmo",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nMarco Cassili",
	&machine_driver,

        cosmicmo_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, cosmicmo_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	invaders_hiload, invaders_hisave
};



/* LT 3-12-1997 */
struct GameDriver spaceph_driver =
{
	"Space Phantoms",
	"spaceph",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nPaul Swan\nMarco Cassili",
	&lrescue_machine_driver,

	spaceph_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, spaceph_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, 0,
	ORIENTATION_DEFAULT,

        0,0
};

/* LT 3-12-1997 */
struct GameDriver rollingc_driver =
{
	"Rolling Crash - Moon Base",
	"rollingc",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nPaul Swan",    /*L.T */
	&rollingc_machine_driver,

	rollingc_rom,
	0, 0,
	invaders_sample_names,
	0,	/* sound_prom */

	0/*TBR*/, rollingc_input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0,rollingc_palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

