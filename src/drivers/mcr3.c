/***************************************************************************

MCR/III memory map (preliminary)

0000-3fff ROM 0
4000-7fff ROM 1
8000-cfff ROM 2
c000-dfff ROM 3
e000-e7ff RAM
e800-e9ff spriteram
f000-f7ff tiles
f800-f8ff palette ram

IN0

bit 0 : left coin
bit 1 : right coin
bit 2 : 1 player
bit 3 : 2 player
bit 4 : ?
bit 5 : tilt
bit 6 : ?
bit 7 : service

IN1,IN2

joystick, sometimes spinner

IN3

Usually dipswitches. Most game configuration is really done by holding down
the service key (F2) during the startup self-test.

IN4

extra inputs

Known issues:

* Destruction Derby has no sound
* Destruction Derby player 3 and 4 steering wheels are not properly muxed
* Rampage has no sound (uses a 68000-based sound board -- we're SOL for now)
* Spy Hunter has no sound (music uses a 68000-based board; SSIO ROMs are corrupt)
* Discs of Tron halts between levels 6? and 7? Could be speech-related...

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "Z80.h"

void mcr3_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void mcr3_vh_screenrefresh(struct osd_bitmap *bitmap);
void mcr3_videoram_w(int offset,int data);
void mcr3_palette_w(int offset,int data);
extern unsigned char *mcr3_paletteram;

void rampage_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void rampage_vh_screenrefresh(struct osd_bitmap *bitmap);

void spyhunt_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int spyhunt_vh_start(void);
void spyhunt_vh_stop(void);
void spyhunt_vh_screenrefresh(struct osd_bitmap *bitmap);
extern unsigned char *spyhunt_alpharam;
extern int spyhunt_alpharam_size;

void mcr_init_machine(void);
void mcr_init_machine_no_watchdog(void);
int mcr_interrupt(void);
extern int mcr_loadnvram;

void mcr_writeport(int port,int value);
int mcr_readport(int port);
void mcr_soundstatus_w (int offset,int data);
int mcr_soundlatch_r (int offset);

int destderb_port_r(int offset);

int spyhunt_port_r(int offset);
void spyhunt_writeport(int port,int value);

int mcr_sh_interrupt(void);
int mcr_sh_start(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xe9ff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe800, 0xe9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf7ff, mcr3_videoram_w, &videoram, &videoram_size },
	{ 0xf800, 0xf8ff, mcr3_palette_w, &mcr3_paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress rampage_readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xebff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rampage_writemem[] =
{
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe800, 0xebff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf7ff, mcr3_videoram_w, &videoram, &videoram_size },
	{ 0xec00, 0xecff, mcr3_palette_w, &mcr3_paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress spyhunt_readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xebff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress spyhunt_writemem[] =
{
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xe800, 0xebff, MWA_RAM, &spyhunt_alpharam, &spyhunt_alpharam_size },
	{ 0xe000, 0xe7ff, videoram_w, &videoram, &videoram_size },
	{ 0xf800, 0xf9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xfa00, 0xfaff, mcr3_palette_w, &mcr3_paletteram },
	{ 0x0000, 0xdfff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9003, mcr_soundlatch_r },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xb001, 0xb001, AY8910_read_port_1_r },
	{ 0xf000, 0xf000, input_port_5_r },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_0_w },
	{ 0xb000, 0xb000, AY8910_control_port_1_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xc000, 0xc000, mcr_soundstatus_w },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress timber_sound_readmem[] =
{
	{ 0x3000, 0x3fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9003, mcr_soundlatch_r },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xb001, 0xb001, AY8910_read_port_1_r },
	{ 0xf000, 0xf000, input_port_5_r },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0x0000, 0x2fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress timber_sound_writemem[] =
{
	{ 0x3000, 0x3fff, MWA_RAM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_0_w },
	{ 0xb000, 0xb000, AY8910_control_port_1_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xc000, 0xc000, mcr_soundstatus_w },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0x0000, 0x2fff, MWA_ROM },
	{ -1 }	/* end of table */
};


/***************************************************************************

  Input port definitions

***************************************************************************/

INPUT_PORTS_START( tapper_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( dotron_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 50, 0, 0, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Aim Down", OSD_KEY_Z, IP_KEY_NONE, 0 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Aim Up", OSD_KEY_A, IP_KEY_NONE, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* this needs to be low to allow 1 player games? */

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( destderb_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 -- the high 6 bits contain the sterring wheel value */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_ANALOG ( 0xfc, 0x00, IPT_DIAL | IPF_REVERSE, 50, 0, 0, 0 )

	PORT_START	/* IN2 -- the high 6 bits contain the sterring wheel value */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_ANALOG ( 0xfc, 0x00, IPT_DIAL | IPF_REVERSE, 50, 0, 0, 0 )

	PORT_START	/* IN3 -- dipswitches */
 	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2P Upright" )
	PORT_DIPSETTING(    0x00, "4P Upright" )
 	PORT_DIPNAME( 0x02, 0x02, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Harder" )
 	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
 	PORT_DIPNAME( 0x08, 0x08, "Reward Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Expanded" )
	PORT_DIPSETTING(    0x00, "Limited" )
 	PORT_DIPNAME( 0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( timber_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( rampage_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
 	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Normal" )
	PORT_DIPSETTING(    0x02, "0 Easy" )
	PORT_DIPSETTING(    0x01, "2 Hard" )
	PORT_DIPSETTING(    0x00, "1 Free" )
 	PORT_DIPNAME( 0x04, 0x04, "Score Option", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_BIT( 0x78, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Advance", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( spyhunt_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, 0 | IPF_TOGGLE, "Gear Shift", OSD_KEY_ENTER, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_KEY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 -- various buttons, low 5 bits */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4, "Oil Slick", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5, "Missiles", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3, "Weapon Truck", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2, "Smoke Screen", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* machine guns */
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 -- actually not used at all, but read as a trakport */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches -- low 4 bits only */
 	PORT_DIPNAME( 0x01, 0x01, "Game Timer", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1:00" )
	PORT_DIPSETTING(    0x01, "1:30" )
 	PORT_DIPNAME( 0x02, 0x02, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* fake port for additional controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )	/* accelerator */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	/* accelerator */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )	/* steering */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )	/* steering */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static struct IOReadPort readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, input_port_1_r },
   { 0x02, 0x02, input_port_2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort dotron_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, input_port_1_r },
   { 0x02, 0x02, input_port_2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort destderb_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x02, destderb_port_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort spyhunt_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, input_port_1_r },
   { 0x02, 0x02, spyhunt_port_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOWritePort writeport[] =
{
   { 0, 0xFF, mcr_writeport },
   { -1 }	/* end of table */
};

static struct IOWritePort sh_writeport[] =
{
   { 0, 0xFF, spyhunt_writeport },
   { -1 }	/* end of table */
};


/***************************************************************************

  Graphics layouts

***************************************************************************/

/* generic character layouts */

/* note that characters are half the resolution of sprites in each direction, so we generate
   them at double size */

/* 1024 characters; used by tapper, timber, rampage */
static struct GfxLayout mcr3_charlayout_1024 =
{
	16, 16,
	1024,
	4,
	{ 1024*16*8, 1024*16*8+1, 0, 1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};

/* 512 characters; used by dotron, destderb */
static struct GfxLayout mcr3_charlayout_512 =
{
	16, 16,
	512,
	4,
	{ 512*16*8, 512*16*8+1, 0, 1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};

/* generic sprite layouts */

/* 512 sprites; used by rampage */
#define X (512*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr3_spritelayout_512 =
{
   32,32,
   512,
   4,
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
   {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8
};
#undef X
#undef Y
#undef Z

/* 256 sprites; used by tapper, timber, destderb, spyhunt */
#define X (256*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr3_spritelayout_256 =
{
   32,32,
   256,
   4,
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
   {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8
};
#undef X
#undef Y
#undef Z

/* 128 sprites; used by dotron */
#define X (128*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr3_spritelayout_128 =
{
   32,32,
   128,
   4,
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
   {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8
};
#undef X
#undef Y
#undef Z

/***************************** spyhunt layouts **********************************/

/* 128 32x16 characters; used by spyhunt */
static struct GfxLayout spyhunt_charlayout_128 =
{
	64, 32,
	128,
	4,
	{ 0, 1, 128*128*8, 128*128*8+1  },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 16, 16, 18, 18, 20, 20, 22, 22, 24, 24, 26, 26, 28, 28, 30, 30,
	  32, 32, 34, 34, 36, 36, 38, 38, 40, 40, 42, 42, 44, 44, 46, 46, 48, 48, 50, 50, 52, 52, 54, 54, 56, 56, 58, 58, 60, 60, 62, 62 },
	{ 0, 0, 8*8, 8*8, 16*8, 16*8, 24*8, 24*8, 32*8, 32*8, 40*8, 40*8, 48*8, 48*8, 56*8, 56*8,
	  64*8, 64*8, 72*8, 72*8, 80*8, 80*8, 88*8, 88*8, 96*8, 96*8, 104*8, 104*8, 112*8, 112*8, 120*8, 120*8 },
	128*8
};

/* of course, Spy Hunter just *had* to be different than everyone else... */
static struct GfxLayout spyhunt_alphalayout =
{
	16, 16,
	256,
	2,
	{ 0, 1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};



static struct GfxDecodeInfo tapper_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_1024,   0, 4 },
	{ 1, 0x8000, &mcr3_spritelayout_256,  0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo dotron_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_512,   0, 4 },
	{ 1, 0x4000, &mcr3_spritelayout_128, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo destderb_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_512,   0, 4 },
	{ 1, 0x4000, &mcr3_spritelayout_256, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo timber_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_1024,   0, 4 },
	{ 1, 0x8000, &mcr3_spritelayout_256,  0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rampage_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_1024,  0, 4 },
	{ 1, 0x8000, &mcr3_spritelayout_512, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo spyhunt_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &spyhunt_charlayout_128,    0, 4 },
	{ 1, 0x8000, &mcr3_spritelayout_256,     0, 4 },
	{ 1, 0x28000, &spyhunt_alphalayout,   8*16, 4 },
	{ -1 } /* end of array */
};



/***************************************************************************

  Machine drivers

***************************************************************************/

static struct MachineDriver tapper_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,readport,writeport,
			mcr_interrupt,32
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			mcr_sh_interrupt,26
		}
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	tapper_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr3_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};

static struct MachineDriver dotron_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,dotron_readport,writeport,
			mcr_interrupt,32
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			mcr_sh_interrupt,26
		}
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	dotron_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr3_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};

static struct MachineDriver destderb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,destderb_readport,writeport,
			mcr_interrupt,32
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			mcr_sh_interrupt,26
		}
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	destderb_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr3_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};

static struct MachineDriver timber_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,readport,writeport,
			mcr_interrupt,32
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			timber_sound_readmem,timber_sound_writemem,0,0,
			mcr_sh_interrupt,26
		}
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	timber_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr3_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};

static struct MachineDriver rampage_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			rampage_readmem,rampage_writemem,readport,writeport,
			mcr_interrupt,32
		},
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine_no_watchdog,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	rampage_gfxdecodeinfo,
	8*16, 8*16,
	rampage_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	rampage_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};

static struct MachineDriver spyhunt_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			spyhunt_readmem,spyhunt_writemem,spyhunt_readport,sh_writeport,
			mcr_interrupt,32
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			mcr_sh_interrupt,26
		}
	},
	30,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mcr_init_machine,

	/* video hardware */
	30*16, 30*16, { 0, 30*16-1, 0, 30*16-1 },
	spyhunt_gfxdecodeinfo,
	8*16+4, 8*16+4,
	spyhunt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	spyhunt_vh_start,
	spyhunt_vh_stop,
	spyhunt_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	mcr_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tapper_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "TAPPG0.BIN", 0x0000, 0x4000, 0xcb048516 )
	ROM_LOAD( "TAPPG1.BIN", 0x4000, 0x4000, 0x4f5f9141 )
	ROM_LOAD( "TAPPG2.BIN", 0x8000, 0x4000, 0x88f856dc )
	ROM_LOAD( "TAPPG3.BIN", 0xc000, 0x2000, 0x2bb09d80 )

	ROM_REGION(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "TAPBG1.BIN", 0x00000, 0x4000, 0xea6a7c78 )
	ROM_LOAD( "TAPBG0.BIN", 0x04000, 0x4000, 0x5dde8902 )
	ROM_LOAD( "TAPFG7.BIN", 0x08000, 0x4000, 0x9cfc4174 )
	ROM_LOAD( "TAPFG6.BIN", 0x0c000, 0x4000, 0x54a41abe )
	ROM_LOAD( "TAPFG5.BIN", 0x10000, 0x4000, 0xbdbb4c45 )
	ROM_LOAD( "TAPFG4.BIN", 0x14000, 0x4000, 0xed4ff871 )
	ROM_LOAD( "TAPFG3.BIN", 0x18000, 0x4000, 0xcd14ce26 )
	ROM_LOAD( "TAPFG2.BIN", 0x1c000, 0x4000, 0xfaa1aaa1 )
	ROM_LOAD( "TAPFG1.BIN", 0x20000, 0x4000, 0xf2cde3f3 )
	ROM_LOAD( "TAPFG0.BIN", 0x24000, 0x4000, 0xbe24e6b8 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tapsnda7.bin", 0x0000, 0x1000, 0x2a3cef68 )
	ROM_LOAD( "tapsnda8.bin", 0x1000, 0x1000, 0x1b700dfa )
	ROM_LOAD( "tapsnda9.bin", 0x2000, 0x1000, 0xb4de31ba )
	ROM_LOAD( "tapsda10.bin", 0x3000, 0x1000, 0x5700e3bc )
ROM_END

ROM_START( dotron_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "loc-cpu1", 0x0000, 0x4000, 0x2b48866e )
	ROM_LOAD( "loc-cpu2", 0x4000, 0x4000, 0xfa50f58a )
	ROM_LOAD( "loc-cpu3", 0x8000, 0x4000, 0x9c8ca44a )
	ROM_LOAD( "loc-cpu4", 0xc000, 0x2000, 0x12811315 )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "loc-bg2",  0x00000, 0x2000, 0x42dc6d00 )
	ROM_LOAD( "loc-bg1",  0x02000, 0x2000, 0x02fce9a8 )
	ROM_LOAD( "vgaloc-a", 0x04000, 0x2000, 0xd1d760a7 )
	ROM_LOAD( "vgaloc-b", 0x06000, 0x2000, 0x33fc94b0 )
	ROM_LOAD( "vgaloc-c", 0x08000, 0x2000, 0x36de3db6 )
	ROM_LOAD( "vgaloc-d", 0x0a000, 0x2000, 0x61ee7c48 )
	ROM_LOAD( "fga-5",    0x0c000, 0x2000, 0x54eeaaf6 )
	ROM_LOAD( "fga-6",    0x0e000, 0x2000, 0x66e6b362 )
	ROM_LOAD( "fga-7",    0x10000, 0x2000, 0xf0b6e286 )
	ROM_LOAD( "fga-8",    0x12000, 0x2000, 0xd6847bd4 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "loc-a", 0x0000, 0x1000, 0xf24a3842 )
	ROM_LOAD( "loc-b", 0x1000, 0x1000, 0xe06bfdad )
	ROM_LOAD( "loc-c", 0x2000, 0x1000, 0x4eb9e1c9 )
	ROM_LOAD( "loc-d", 0x3000, 0x1000, 0x33985af4 )
ROM_END

ROM_START( destderb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "DD_PRO", 0x0000, 0x4000, 0x92df12bf )
	ROM_LOAD( "DD_PRO1", 0x4000, 0x4000, 0x87f5f32b )
	ROM_LOAD( "DD_PRO2", 0x8000, 0x4000, 0xf7d3cba3 )

	ROM_REGION(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "DD_BG0.6F", 0x00000, 0x2000, 0xaca450be )
	ROM_LOAD( "DD_BG1.5F", 0x02000, 0x2000, 0xc6416501 )
	ROM_LOAD( "DD_FG-3.A10", 0x04000, 0x4000, 0xef11eb4f )
	ROM_LOAD( "DD_FG-7.A9", 0x08000, 0x4000, 0x34d19e2f )
	ROM_LOAD( "DD_FG-2.A8", 0x0c000, 0x4000, 0x39702db0 )
	ROM_LOAD( "DD_FG-6.A7", 0x10000, 0x4000, 0x1f76977e )
	ROM_LOAD( "DD_FG-1.A6", 0x14000, 0x4000, 0x154a3f9c )
	ROM_LOAD( "DD_FG-5.A5", 0x18000, 0x4000, 0xa895e3f7 )
	ROM_LOAD( "DD_FG-0.A4", 0x1c000, 0x4000, 0x113e4f22 )
	ROM_LOAD( "DD_FG-4.A3", 0x20000, 0x4000, 0x2cbae0ce )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "DD_SSIO.A7", 0x0000, 0x1000, 0xd920c104 )
	ROM_LOAD( "DD_SSIO.A8", 0x1000, 0x1000, 0x241e9f44 )
ROM_END

ROM_START( timber_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "timpg0.bin", 0x0000, 0x4000, 0x2a48e890 )
	ROM_LOAD( "timpg1.bin", 0x4000, 0x4000, 0xb4fa87d0 )
	ROM_LOAD( "timpg2.bin", 0x8000, 0x4000, 0x4df6b19a )
	ROM_LOAD( "timpg3.bin", 0xc000, 0x2000, 0xfb590c8f )

	ROM_REGION(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "timbg1.bin", 0x00000, 0x4000, 0x5b0ff893 )
	ROM_LOAD( "timbg0.bin", 0x04000, 0x4000, 0xcbece7a8 )
	ROM_LOAD( "timfg7.bin", 0x08000, 0x4000, 0x5ed46eae )
	ROM_LOAD( "timfg6.bin", 0x0c000, 0x4000, 0x31c7f47f )
	ROM_LOAD( "timfg5.bin", 0x10000, 0x4000, 0x01ac936c )
	ROM_LOAD( "timfg4.bin", 0x14000, 0x4000, 0x8c437a91 )
	ROM_LOAD( "timfg3.bin", 0x18000, 0x4000, 0xf4aaa2fa )
	ROM_LOAD( "timfg2.bin", 0x1c000, 0x4000, 0x4cfe5f16 )
	ROM_LOAD( "timfg1.bin", 0x20000, 0x4000, 0x08963712 )
	ROM_LOAD( "timfg0.bin", 0x24000, 0x4000, 0x567d8457 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tima7.bin", 0x0000, 0x1000, 0x607ed3b8 )
	ROM_LOAD( "tima8.bin", 0x1000, 0x1000, 0x85853a95 )
	ROM_LOAD( "tima9.bin", 0x2000, 0x1000, 0x49e515b1 )
ROM_END

ROM_START( rampage_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pro-0.rv3", 0x0000, 0x8000, 0x471a3c00 )
	ROM_LOAD( "pro-1.rv3", 0x8000, 0x6000, 0x54429658 )

	ROM_REGION(0x48000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bg-0", 0x00000, 0x4000, 0xefa953c5 )
	ROM_LOAD( "bg-1", 0x04000, 0x4000, 0x88fe2998 )
	ROM_LOAD( "fg-3", 0x08000, 0x10000, 0x06033763 )
	ROM_LOAD( "fg-2", 0x18000, 0x10000, 0xdf6c8714 )
	ROM_LOAD( "fg-1", 0x28000, 0x10000, 0xa0449c5e )
	ROM_LOAD( "fg-0", 0x38000, 0x10000, 0xf9f7bf39 )
ROM_END

ROM_START( spyhunt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu_pg0.6d", 0x0000, 0x2000, 0x69818221 )
	ROM_LOAD( "cpu_pg1.7d", 0x2000, 0x2000, 0xb2695673 )
	ROM_LOAD( "cpu_pg2.8d", 0x4000, 0x2000, 0xbbf9e30f )
	ROM_LOAD( "cpu_pg3.9d", 0x6000, 0x2000, 0x256011f6 )
	ROM_LOAD( "cpu_pg4.10d",0x8000, 0x2000, 0xf5a5e14b )
	ROM_LOAD( "cpu_pg5.11d",0xA000, 0x4000, 0x8d0af17c )

	ROM_REGION(0x29000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cpu_bg2.5a", 0x0000, 0x2000, 0x6d2296e2 )
	ROM_LOAD( "cpu_bg3.6a", 0x2000, 0x2000, 0x113ff55b )
	ROM_LOAD( "cpu_bg0.3a", 0x4000, 0x2000, 0x0d0f68b7 )
	ROM_LOAD( "cpu_bg1.4a", 0x6000, 0x2000, 0x6d113309 )
	ROM_LOAD( "vid_6fg.a2", 0x8000, 0x4000, 0x80d21978 )
	ROM_LOAD( "vid_7fg.a1", 0xc000, 0x4000, 0x1a41cab7 )
	ROM_LOAD( "vid_4fg.a4", 0x10000, 0x4000, 0x0fdc474c )
	ROM_LOAD( "vid_5fg.a3", 0x14000, 0x4000, 0x638c1f46 )
	ROM_LOAD( "vid_2fg.a6", 0x18000, 0x4000, 0xa9e6820c )
	ROM_LOAD( "vid_3fg.a5", 0x1c000, 0x4000, 0xd87b5e19 )
	ROM_LOAD( "vid_0fg.a8", 0x20000, 0x4000, 0x3a09c10f )
	ROM_LOAD( "vid_1fg.a7", 0x24000, 0x4000, 0xd6383ff6 )
	ROM_LOAD( "cpu_alph.10g",0x28000, 0x1000, 0xf22c49b2 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_0sd.a8", 0x0000, 0x1000, 0xd920c104 )
	ROM_LOAD( "snd_1sd.a7", 0x1000, 0x1000, 0x241e9f44 )
ROM_END


static int mcr3_hiload(int addr, int len)
{
   unsigned char *RAM = Machine->memory_region[0];

   /* see if it's okay to load */
   if (mcr_loadnvram)
   {
      void *f;

		f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
      if (f)
      {
			osd_fread(f,&RAM[addr],len);
			osd_fclose (f);
      }
      return 1;
   }
   else return 0;	/* we can't load the hi scores yet */
}

static void mcr3_hisave(int addr, int len)
{
   unsigned char *RAM = Machine->memory_region[0];
   void *f;

	f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);
   if (f)
   {
      osd_fwrite(f,&RAM[addr],len);
      osd_fclose (f);
   }
}

static int  tapper_hiload(void)   { return mcr3_hiload(0xe000, 0x9d); }
static void tapper_hisave(void)   {        mcr3_hisave(0xe000, 0x9d); }

static int  dotron_hiload(void)   { return mcr3_hiload(0xe543, 0xac); }
static void dotron_hisave(void)   {        mcr3_hisave(0xe543, 0xac); }

static int  destderb_hiload(void) { return mcr3_hiload(0xe4e6, 0x153); }
static void destderb_hisave(void) {        mcr3_hisave(0xe4e6, 0x153); }

static int  timber_hiload(void)   { return mcr3_hiload(0xe000, 0x9f); }
static void timber_hisave(void)   {        mcr3_hisave(0xe000, 0x9f); }

static int  rampage_hiload(void)  { return mcr3_hiload(0xe631, 0x3f); }
static void rampage_hisave(void)  {        mcr3_hisave(0xe631, 0x3f); }

static int  spyhunt_hiload(void)  { return mcr3_hiload(0xf42b, 0xfb); }
static void spyhunt_hisave(void)  {        mcr3_hisave(0xf42b, 0xfb); }

static void spyhunt_decode (void)
{
   unsigned char *RAM = Machine->memory_region[0];


	/* some versions of rom 11d have the top and bottom 8k swapped; to enable us to work with either
	   a correct set or a swapped set (both of which pass the checksum!), we swap them here */
	if (RAM[0xa000] != 0x0c)
	{
		int i;
		unsigned char temp;

		for (i = 0;i < 0x2000;i++)
		{
			temp = RAM[0xa000 + i];
			RAM[0xa000 + i] = RAM[0xc000 + i];
			RAM[0xc000 + i] = temp;
		}
	}
}

struct GameDriver tapper_driver =
{
	"Tapper",
	"tapper",
	"CHRISTOPHER KIRMSE\nAARON GILES\nNICOLA SALMORIA",
	&tapper_machine_driver,

	tapper_rom,
	0, 0,
	0,

	0, tapper_input_ports, 0, 0, 0,

	0, 0,0,
	ORIENTATION_DEFAULT,

	tapper_hiload, tapper_hisave
};

struct GameDriver dotron_driver =
{
	"Discs of Tron",
	"dotron",
	"CHRISTOPHER KIRMSE\nAARON GILES\nNICOLA SALMORIA",
	&dotron_machine_driver,

	dotron_rom,
	0, 0,
	0,

	0, dotron_input_ports, 0, 0, 0,

	0, 0,0,
	ORIENTATION_FLIP_X,

	dotron_hiload, dotron_hisave
};

struct GameDriver destderb_driver =
{
	"Demolition Derby",
	"destderb",
	"CHRISTOPHER KIRMSE\nAARON GILES\nNICOLA SALMORIA\nBRAD OLIVER",
	&destderb_machine_driver,

	destderb_rom,
	0, 0,
	0,

	0, destderb_input_ports, 0, 0, 0,

	0, 0,0,
	ORIENTATION_DEFAULT,

	destderb_hiload, destderb_hisave
};

struct GameDriver timber_driver =
{
	"Timber",
	"timber",
	"CHRISTOPHER KIRMSE\nAARON GILES\nNICOLA SALMORIA\nBRAD OLIVER",
	&timber_machine_driver,

	timber_rom,
	0, 0,
	0,

	0, timber_input_ports, 0, 0, 0,

	0, 0,0,
	ORIENTATION_DEFAULT,

	timber_hiload, timber_hisave
};

struct GameDriver rampage_driver =
{
	"Rampage",
	"rampage",
	"CHRISTOPHER KIRMSE\nAARON GILES\nNICOLA SALMORIA\nBRAD OLIVER",
	&rampage_machine_driver,

	rampage_rom,
	0, 0,
	0,

	0, rampage_input_ports, 0, 0, 0,

	0, 0,0,
	ORIENTATION_DEFAULT,

	rampage_hiload, rampage_hisave
};

struct GameDriver spyhunt_driver =
{
	"Spy Hunter",
	"spyhunt",
	"AARON GILES\nCHRISTOPHER KIRMSE\nNICOLA SALMORIA\nBRAD OLIVER\nLAWNMOWER MAN",
	&spyhunt_machine_driver,

	spyhunt_rom,
	spyhunt_decode, 0,
	0,

	0, spyhunt_input_ports, 0, 0, 0,

	0, 0,0,
	ORIENTATION_ROTATE_90,

	spyhunt_hiload, spyhunt_hisave
};
