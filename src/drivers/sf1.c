/***************************************************************************

  Street Fighter 1

  TODO:
  - hook up the second Z80 (which plays samples through two MSM5205)
  - properly support palette shrinking in the video driver
  - clean up and optimize the video driver (it's currently redrawing the
    whole screen every frame)

***************************************************************************/

#include "driver.h"
#include "cpuintrf.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern unsigned char *sf1_video_ram;
extern unsigned char *sf1_object_ram;
extern int sf1_deltaxb;
extern int sf1_deltaxm;
extern int sf1_active;

void sf1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static int sf1_interrupt(void)
{
  return 1;
}

/* Dips */
static int control4_r(int offset)
{
  return (input_port_1_r(0)<<8)|input_port_0_r(0);
}

/* Dips again */
/* No, the 7 is not a bug */
static int control5_r(int offset)
{
  return (input_port_3_r(0)<<7)|input_port_2_r(0);
}

/* Start */
static int control6_r(int offset)
{
  return input_port_5_r(0)|0xff00;
}

static int control7_r(int offset)
{
  return 0xffff;
}

static void control10_w(int offset, int data)
{
  sf1_deltaxm = data;
}

static void control12_w(int offset, int data)
{
  sf1_deltaxb = data;
}

/* b0 = reset, or maybe "set anyway" */
/* b1 = pulsed when control6.b6==0 until it's 1 */
/* b2 = active when dip 8 (flip) on */
/* b3 = active character plane */
/* b4 = unused */
/* b5 = active background plane */
/* b6 = active middle plane */
/* b7 = active sprites */
static void control13_w(int offset, int data)
{
  if((data&0xff)!=0)
    sf1_active = data;
}


/* The protection of the japanese version */

static void control15_w(int offset, int data)
{
  static int maplist[4][10] = {
    { 1, 0, 3, 2, 4, 5, 6, 7, 8, 9 },
    { 4, 5, 6, 7, 1, 0, 3, 2, 8, 9 },
    { 3, 2, 1, 0, 6, 7, 4, 5, 8, 9 },
    { 6, 7, 4, 5, 3, 2, 1, 0, 8, 9 }
  };
  int map;

  map = maplist
    [cpu_readmem24(0xffc006)]
    [(cpu_readmem24(0xffc003)<<1) + (cpu_readmem24_word(0xffc004)>>8)];

  switch(cpu_readmem24(0xffc684)) {
  case 1:
    {
      int base;
      int j;

      base = 0x1b6e8+0x300e*map;

      cpu_writemem24_dword(0xffc01c, 0x16bfc+0x270*map);
      cpu_writemem24_dword(0xffc020, base+0x80);
      cpu_writemem24_dword(0xffc024, base);
      cpu_writemem24_dword(0xffc028, base+0x86);
      cpu_writemem24_dword(0xffc02c, base+0x8e);
      cpu_writemem24_dword(0xffc030, base+0x20e);
      cpu_writemem24_dword(0xffc034, base+0x30e);
      cpu_writemem24_dword(0xffc038, base+0x38e);
      cpu_writemem24_dword(0xffc03c, base+0x40e);
      cpu_writemem24_dword(0xffc040, base+0x80e);
      cpu_writemem24_dword(0xffc044, base+0xc0e);
      cpu_writemem24_dword(0xffc048, base+0x180e);
      cpu_writemem24_dword(0xffc04c, base+0x240e);
      cpu_writemem24_dword(0xffc050, 0x19548+0x60*map);
      cpu_writemem24_dword(0xffc054, 0x19578+0x60*map);
      break;
    }
  case 2:
    {
      static int delta1[10] = {
	0x1f80, 0x1c80, 0x2700, 0x2400, 0x2b80, 0x2e80, 0x3300, 0x3600, 0x3a80, 0x3d80
      };
      static int delta2[10] = {
	0x2180, 0x1800, 0x3480, 0x2b00, 0x3e00, 0x4780, 0x5100, 0x5a80, 0x6400, 0x6d80
      };

      int d1 = delta1[map] + 0xc0;
      int d2 = delta2[map];

      cpu_writemem24_word(0xffc680, d1);
      cpu_writemem24_word(0xffc682, d2);
      cpu_writemem24_word(0xffc00c, 0xc0);

      control10_w(0, d1);
      control12_w(0, d2);
      break;
    }
  case 4:
    {
      int pos = cpu_readmem24(0xffc010);
      pos = (pos+1) & 3;
      cpu_writemem24(0xffc010, pos);
      if(!pos) {
	int d1 = cpu_readmem24_word(0xffc682);
	int off = cpu_readmem24_word(0xffc00e);
	if(off!=512) {
	  off++;
	  d1++;
	} else {
	  off = 0;
	  d1 -= 512;
	}
	cpu_writemem24_word(0xffc682, d1);
	cpu_writemem24_word(0xffc00e, off);
	control12_w(0, d1);
      }
      break;
    }
  default:
    {
      if(errorlog) {
	fprintf(errorlog, "Write control15 at %06x (%04x)\n", cpu_get_pc(), data&0xffff);
	fprintf(errorlog, "*** Unknown protection %d\n", cpu_readmem24(0xffc684));
      }
      break;
    }
  }
}


/* Players actions, world version */
/* That one has analog buttons */
/* We simulate them with 3 buttons the same way the other versions
   internally do */

/* Coins */
static int control0_r(int offset)
{
  return input_port_4_r(0)|0xff00;
}

/* Moves */
static int control1_r(int offset)
{
  return (input_port_7_r(0)<<8)|input_port_6_r(0);
}

static int scale[8] = { 0x00, 0x40, 0xe0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe };

/* Buttons punch */
static int control2_r(int offset)
{
  return (scale[input_port_10_r(0)]<<8)|scale[input_port_8_r(0)];
}

static int control3_r(int offset)
{
  return  (scale[input_port_11_r(0)]<<8)|scale[input_port_9_r(0)];
}


/* Players actions, japanese version */
/* That one has three buttons per attack */

/* Coins */
static int control0jp_r(int offset)
{
  return input_port_4_r(0)|0xff00;
}

/* player 1 buttons & moves */
static int control1jp_r(int offset)
{
  return (input_port_7_r(0)<<8)|input_port_6_r(0);
}

/* Player 2 buttons & moves */
static int control2jp_r(int offset)
{
  return (input_port_9_r(0)<<8)|input_port_8_r(0);
}

static int control3jp_r(int offset)
{
  return 0xffff;
}


/* Players actions, US version */
/* That one has three buttons per attack */

/* Coins and buttons */
static int control0us_r(int offset)
{
  return (input_port_8_r(0)<<8)|input_port_4_r(0);
}

/* Moves & Buttons */
static int control1us_r(int offset)
{
  return (input_port_7_r(0)<<8)|input_port_6_r(0);
}

static int control2us_r(int offset)
{
  return 0xffff;
}

static int control3us_r(int offset)
{
  return 0xffff;
}


static void sf1_soundcmd_w(int offset,int data)
{
	if (data != 0xffff)
	{
		soundlatch_w(offset,data);
		cpu_cause_interrupt(1,Z80_NMI_INT);
	}
}



static struct MemoryReadAddress readmem[] =
{
  { 0x000000, 0x04ffff, MRA_ROM },
  { 0x800000, 0x800fff, MRA_BANK3 },
  { 0xc00000, 0xc00001, control0_r },
  { 0xc00002, 0xc00003, control1_r },
  { 0xc00004, 0xc00005, control2_r },
  { 0xc00006, 0xc00007, control3_r },
  { 0xc00008, 0xc00009, control4_r },
  { 0xc0000a, 0xc0000b, control5_r },
  { 0xc0000c, 0xc0000d, control6_r },
  { 0xc0000e, 0xc0000f, control7_r },
  { 0xff8000, 0xffdfff, MRA_BANK1 },
  { 0xffe000, 0xffffff, MRA_BANK2 },
  { -1 }
};

static struct MemoryReadAddress readmemus[] =
{
  { 0x000000, 0x04ffff, MRA_ROM },
  { 0x800000, 0x800fff, MRA_BANK3 },
  { 0xc00000, 0xc00001, control0us_r },
  { 0xc00002, 0xc00003, control1us_r },
  { 0xc00004, 0xc00005, control2us_r },
  { 0xc00006, 0xc00007, control3us_r },
  { 0xc00008, 0xc00009, control4_r },
  { 0xc0000a, 0xc0000b, control5_r },
  { 0xc0000c, 0xc0000d, control6_r },
  { 0xc0000e, 0xc0000f, control7_r },
  { 0xff8000, 0xffdfff, MRA_BANK1 },
  { 0xffe000, 0xffffff, MRA_BANK2 },
  { -1 }
};

static struct MemoryReadAddress readmemjp[] =
{
  { 0x000000, 0x04ffff, MRA_ROM },
  { 0x800000, 0x800fff, MRA_BANK3 },
  { 0xc00000, 0xc00001, control0jp_r },
  { 0xc00002, 0xc00003, control1jp_r },
  { 0xc00004, 0xc00005, control2jp_r },
  { 0xc00006, 0xc00007, control3jp_r },
  { 0xc00008, 0xc00009, control4_r },
  { 0xc0000a, 0xc0000b, control5_r },
  { 0xc0000c, 0xc0000d, control6_r },
  { 0xc0000e, 0xc0000f, control7_r },
  { 0xff8000, 0xffdfff, MRA_BANK1 },
  { 0xffe000, 0xffffff, MRA_BANK2 },
  { -1 }
};

static struct MemoryWriteAddress writemem[] =
{
  { 0x800000, 0x800fff, MWA_BANK3, &sf1_video_ram },
  { 0xb00000, 0xb007ff, paletteram_xxxxRRRRGGGGBBBB_word_w, &paletteram },
  { 0xc00014, 0xc00015, control10_w },
  { 0xc00017, 0xc00018, control12_w },
  { 0xc0001a, 0xc0001b, control13_w },
  { 0xc0001c, 0xc0001d, sf1_soundcmd_w },
  { 0xc0001e, 0xc0001f, control15_w },
  { 0xff8000, 0xffdfff, MWA_BANK1 },
  { 0xffe000, 0xffffff, MWA_BANK2, &sf1_object_ram },
  { -1 }
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ 0xe001, 0xe001, YM2151_status_port_0_r },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
	{ -1 }
};



INPUT_PORTS_START(sf1jp_input_ports)
	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Screen orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Flip" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPNAME( 0x02, 0x02, "Attract music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, "Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPNAME( 0x20, 0x20, "Attract sound?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPNAME( 0x40, 0x40, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Continuation max stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5th" )
	PORT_DIPSETTING(    0x06, "4th" )
	PORT_DIPSETTING(    0x05, "3rd" )
	PORT_DIPSETTING(    0x04, "2nd" )
	PORT_DIPSETTING(    0x03, "1st" )
	PORT_DIPSETTING(    0x02, "No continuation" )
	PORT_DIPNAME( 0x18, 0x18, "Round time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "100" )
	PORT_DIPSETTING(    0x10, "150" )
	PORT_DIPSETTING(    0x08, "200" )
	PORT_DIPSETTING(    0x00, "250" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very difficult" )
	/* Current port system can't handle the situation of a dipswitch group
	* half on two 8-bit ports
	*/
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Buy-in max stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5th" )
	PORT_DIPSETTING(    0x06, "4th" )
	PORT_DIPSETTING(    0x05, "3rd" )
	PORT_DIPSETTING(    0x04, "2nd" )
	PORT_DIPSETTING(    0x03, "1st" )
	PORT_DIPSETTING(    0x02, "No buy-in" )
	PORT_DIPNAME( 0x08, 0x08, "Number of start countries", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, 0x00, 0x80, "Freeze", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START(sf1us_input_ports)
	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Screen orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Flip" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPNAME( 0x02, 0x02, "Attract music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, "Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPNAME( 0x20, 0x20, "Attract sound?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPNAME( 0x40, 0x40, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Continuation max stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5th" )
	PORT_DIPSETTING(    0x06, "4th" )
	PORT_DIPSETTING(    0x05, "3rd" )
	PORT_DIPSETTING(    0x04, "2nd" )
	PORT_DIPSETTING(    0x03, "1st" )
	PORT_DIPSETTING(    0x02, "No continuation" )
	PORT_DIPNAME( 0x18, 0x18, "Round time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "100" )
	PORT_DIPSETTING(    0x10, "150" )
	PORT_DIPSETTING(    0x08, "200" )
	PORT_DIPSETTING(    0x00, "250" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very difficult" )
	/* Current port system can't handle the situation of a dipswitch group
	* half on two 8-bit ports
	*/
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Buy-in max stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5th" )
	PORT_DIPSETTING(    0x06, "4th" )
	PORT_DIPSETTING(    0x05, "3rd" )
	PORT_DIPSETTING(    0x04, "2nd" )
	PORT_DIPSETTING(    0x03, "1st" )
	PORT_DIPSETTING(    0x02, "No buy-in" )
	PORT_DIPNAME( 0x08, 0x08, "Number of start countries", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, 0x00, 0x80, "Freeze", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START(sf1_input_ports)
	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Screen orientation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Flip" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPNAME( 0x02, 0x02, "Attract music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, "Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPNAME( 0x20, 0x20, "Attract sound?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPNAME( 0x40, 0x40, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Continuation max stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5th" )
	PORT_DIPSETTING(    0x06, "4th" )
	PORT_DIPSETTING(    0x05, "3rd" )
	PORT_DIPSETTING(    0x04, "2nd" )
	PORT_DIPSETTING(    0x03, "1st" )
	PORT_DIPSETTING(    0x02, "No continuation" )
	PORT_DIPNAME( 0x18, 0x18, "Round time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "100" )
	PORT_DIPSETTING(    0x10, "150" )
	PORT_DIPSETTING(    0x08, "200" )
	PORT_DIPSETTING(    0x00, "250" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very difficult" )
	/* Current port system can't handle the situation of a dipswitch group
	* half on two 8-bit ports
	*/
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Buy-in max stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5th" )
	PORT_DIPSETTING(    0x06, "4th" )
	PORT_DIPSETTING(    0x05, "3rd" )
	PORT_DIPSETTING(    0x04, "2nd" )
	PORT_DIPSETTING(    0x03, "1st" )
	PORT_DIPSETTING(    0x02, "No buy-in" )
	PORT_DIPNAME( 0x08, 0x08, "Number of start countries", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, 0x00, 0x80, "Freeze", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout char_layout =
{
	8,8,
	1024,
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout sprite_layout1 =
{
	16,16,
	8192,
	4,
	{ 4, 0, 8192*64*8+4, 8192*64*8 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+8+0, 16*16+8+1, 16*16+8+2, 16*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout sprite_layout2 =
{
	16,16,
	4096,
	4,
	{ 4, 0, 4096*64*8+4, 4096*64*8 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+8+0, 16*16+8+1, 16*16+8+2, 16*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout sprite_layouts =
{
	16,16,
	14336,
	4,
	{ 4, 0, 14336*64*8+4, 14336*64*8 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+8+0, 16*16+8+1, 16*16+8+2, 16*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
  { 1, 0x000000, &sprite_layout1, 256, 16 },
  { 1, 0x100000, &sprite_layout2,   0, 16 },
  { 1, 0x180000, &sprite_layouts, 512, 16 },
  { 1, 0x340000, &char_layout,    768, 64 },
  { -1 }
};



static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,	/* 1 chip */
	3579545,	/* ? xtal is 3.579545MHz */
	{ 60 },
	{ irq_handler }
};

static struct MachineDriver machine_driver =
{
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz ? (xtal is 16MHz) */
			0,
			readmem,writemem,0,0,
			sf1_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ? xtal is 3.579545MHz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2151 */
								/* NMIs are caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	48*8, 32*8, { 0*8, 48*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	sf1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver machineus_driver =
{
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz ? (xtal is 16MHz) */
			0,
			readmemus,writemem,0,0,
			sf1_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ? xtal is 3.579545MHz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2151 */
								/* NMIs are caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	48*8, 32*8, { 0*8, 48*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	sf1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver machinejp_driver =
{
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz ? (xtal is 16MHz) */
			0,
			readmemjp,writemem,0,0,
			sf1_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ? xtal is 3.579545MHz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2151 */
								/* NMIs are caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	48*8, 32*8, { 0*8, 48*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	sf1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};


ROM_START( sf1_rom )
	ROM_REGION(0x60000)
	ROM_LOAD_EVEN("sfe-19", 0x00000, 0x10000, 0x8346c3ca )
	ROM_LOAD_ODD ("sfe-22", 0x00000, 0x10000, 0x3a4bfaa8 )
	ROM_LOAD_EVEN("sfe-20", 0x20000, 0x10000, 0xb40e67ee )
	ROM_LOAD_ODD ("sfe-23", 0x20000, 0x10000, 0x477c3d5b )
	ROM_LOAD_EVEN("sfe-21", 0x40000, 0x10000, 0x2547192b )
	ROM_LOAD_ODD ("sfe-24", 0x40000, 0x10000, 0x79680f4e )

	ROM_REGION_DISPOSE(0x344000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sf-25.bin", 0x000000, 0x020000, 0x7f23042e )	/* Background 1 planes 0-1 */
	ROM_LOAD( "sf-28.bin", 0x020000, 0x020000, 0x92f8b91c )
	ROM_LOAD( "sf-30.bin", 0x040000, 0x020000, 0xb1399856 )
	ROM_LOAD( "sf-34.bin", 0x060000, 0x020000, 0x96b6ae2e )
	ROM_LOAD( "sf-26.bin", 0x080000, 0x020000, 0x54ede9f5 ) /* planes 2-3 */
	ROM_LOAD( "sf-29.bin", 0x0a0000, 0x020000, 0xf0649a67 )
	ROM_LOAD( "sf-31.bin", 0x0c0000, 0x020000, 0x8f4dd71a )
	ROM_LOAD( "sf-35.bin", 0x0e0000, 0x020000, 0x70c00fb4 )
	ROM_LOAD( "sf-39.bin", 0x100000, 0x020000, 0xcee3d292 ) /* Background 2 planes 0-1*/
	ROM_LOAD( "sf-38.bin", 0x120000, 0x020000, 0x2ea99676 )
	ROM_LOAD( "sf-41.bin", 0x140000, 0x020000, 0xe0280495 ) /* planes 2-3 */
	ROM_LOAD( "sf-40.bin", 0x160000, 0x020000, 0xc70b30de )
	ROM_LOAD( "sf-15.bin", 0x180000, 0x020000, 0xfc0113db ) /* Sprites planes 1-2 */
	ROM_LOAD( "sf-16.bin", 0x1a0000, 0x020000, 0x82e4a6d3 )
	ROM_LOAD( "sf-11.bin", 0x1c0000, 0x020000, 0xe112df1b )
	ROM_LOAD( "sf-12.bin", 0x1e0000, 0x020000, 0x42d52299 )
	ROM_LOAD( "sf-07.bin", 0x200000, 0x020000, 0x49f340d9 )
	ROM_LOAD( "sf-08.bin", 0x220000, 0x020000, 0x95ece9b1 )
	ROM_LOAD( "sf-03.bin", 0x240000, 0x020000, 0x5ca05781 )
	ROM_LOAD( "sf-17.bin", 0x260000, 0x020000, 0x69fac48e ) /* planes 2-3 */
	ROM_LOAD( "sf-18.bin", 0x280000, 0x020000, 0x71cfd18d )
	ROM_LOAD( "sf-13.bin", 0x2a0000, 0x020000, 0xfa2eb24b )
	ROM_LOAD( "sf-14.bin", 0x2c0000, 0x020000, 0xad955c95 )
	ROM_LOAD( "sf-09.bin", 0x2e0000, 0x020000, 0x41b73a31 )
	ROM_LOAD( "sf-10.bin", 0x300000, 0x020000, 0x91c41c50 )
	ROM_LOAD( "sf-05.bin", 0x320000, 0x020000, 0x538c7cbe )
	ROM_LOAD( "sf-27.bin", 0x340000, 0x004000, 0x2b09b36d ) /* Characters planes 1-2 */

	ROM_REGION(0x40000) /* Backgrounds */
	ROM_LOAD( "sf-37.bin", 0x000000, 0x010000, 0x23d09d3d )
	ROM_LOAD( "sf-36.bin", 0x010000, 0x010000, 0xea16df6c )
	ROM_LOAD( "sf-32.bin", 0x020000, 0x010000, 0x72df2bd9 )
	ROM_LOAD( "sf-33.bin", 0x030000, 0x010000, 0x3e99d3d5 )

	ROM_REGION(0x10000)	/* 64k for the music CPU */
	ROM_LOAD( "sf-02.bin", 0x0000, 0x8000, 0x4a9ac534 )

	ROM_REGION(0x40000)	/* 256k for the samples CPU */
	ROM_LOAD( "sfu-00",    0x00000, 0x20000, 0xa7cce903 )
	ROM_LOAD( "sf-01.bin", 0x20000, 0x20000, 0x86e0f0d5 )
ROM_END

ROM_START( sf1us_rom )
	ROM_REGION(0x60000)
	ROM_LOAD_EVEN("sfd-19", 0x00000, 0x10000, 0xfaaf6255 )
	ROM_LOAD_ODD ("sfd-22", 0x00000, 0x10000, 0xe1fe3519 )
	ROM_LOAD_EVEN("sfd-20", 0x20000, 0x10000, 0x44b915bd )
	ROM_LOAD_ODD ("sfd-23", 0x20000, 0x10000, 0x79c43ff8 )
	ROM_LOAD_EVEN("sfd-21", 0x40000, 0x10000, 0xe8db799b )
	ROM_LOAD_ODD ("sfd-24", 0x40000, 0x10000, 0x466a3440 )

	ROM_REGION_DISPOSE(0x344000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sf-25.bin", 0x000000, 0x020000, 0x7f23042e )	/* Background 1 planes 0-1 */
	ROM_LOAD( "sf-28.bin", 0x020000, 0x020000, 0x92f8b91c )
	ROM_LOAD( "sf-30.bin", 0x040000, 0x020000, 0xb1399856 )
	ROM_LOAD( "sf-34.bin", 0x060000, 0x020000, 0x96b6ae2e )
	ROM_LOAD( "sf-26.bin", 0x080000, 0x020000, 0x54ede9f5 ) /* planes 2-3 */
	ROM_LOAD( "sf-29.bin", 0x0a0000, 0x020000, 0xf0649a67 )
	ROM_LOAD( "sf-31.bin", 0x0c0000, 0x020000, 0x8f4dd71a )
	ROM_LOAD( "sf-35.bin", 0x0e0000, 0x020000, 0x70c00fb4 )
	ROM_LOAD( "sf-39.bin", 0x100000, 0x020000, 0xcee3d292 ) /* Background 2 planes 0-1*/
	ROM_LOAD( "sf-38.bin", 0x120000, 0x020000, 0x2ea99676 )
	ROM_LOAD( "sf-41.bin", 0x140000, 0x020000, 0xe0280495 ) /* planes 2-3 */
	ROM_LOAD( "sf-40.bin", 0x160000, 0x020000, 0xc70b30de )
	ROM_LOAD( "sf-15.bin", 0x180000, 0x020000, 0xfc0113db ) /* Sprites planes 1-2 */
	ROM_LOAD( "sf-16.bin", 0x1a0000, 0x020000, 0x82e4a6d3 )
	ROM_LOAD( "sf-11.bin", 0x1c0000, 0x020000, 0xe112df1b )
	ROM_LOAD( "sf-12.bin", 0x1e0000, 0x020000, 0x42d52299 )
	ROM_LOAD( "sf-07.bin", 0x200000, 0x020000, 0x49f340d9 )
	ROM_LOAD( "sf-08.bin", 0x220000, 0x020000, 0x95ece9b1 )
	ROM_LOAD( "sf-03.bin", 0x240000, 0x020000, 0x5ca05781 )
	ROM_LOAD( "sf-17.bin", 0x260000, 0x020000, 0x69fac48e ) /* planes 2-3 */
	ROM_LOAD( "sf-18.bin", 0x280000, 0x020000, 0x71cfd18d )
	ROM_LOAD( "sf-13.bin", 0x2a0000, 0x020000, 0xfa2eb24b )
	ROM_LOAD( "sf-14.bin", 0x2c0000, 0x020000, 0xad955c95 )
	ROM_LOAD( "sf-09.bin", 0x2e0000, 0x020000, 0x41b73a31 )
	ROM_LOAD( "sf-10.bin", 0x300000, 0x020000, 0x91c41c50 )
	ROM_LOAD( "sf-05.bin", 0x320000, 0x020000, 0x538c7cbe )
	ROM_LOAD( "sf-27.bin", 0x340000, 0x004000, 0x2b09b36d ) /* Characters planes 1-2 */

	ROM_REGION(0x40000) /* Backgrounds */
	ROM_LOAD( "sf-37.bin", 0x000000, 0x010000, 0x23d09d3d )
	ROM_LOAD( "sf-36.bin", 0x010000, 0x010000, 0xea16df6c )
	ROM_LOAD( "sf-32.bin", 0x020000, 0x010000, 0x72df2bd9 )
	ROM_LOAD( "sf-33.bin", 0x030000, 0x010000, 0x3e99d3d5 )

	ROM_REGION(0x10000)	/* 64k for the music CPU */
	ROM_LOAD( "sf-02.bin", 0x0000, 0x8000, 0x4a9ac534 )

	ROM_REGION(0x40000)	/* 256k for the samples CPU */
	ROM_LOAD( "sfu-00",    0x00000, 0x20000, 0xa7cce903 )
	ROM_LOAD( "sf-01.bin", 0x20000, 0x20000, 0x86e0f0d5 )
ROM_END

ROM_START( sf1jp_rom )
	ROM_REGION(0x60000)
	ROM_LOAD_EVEN("sf-19.bin", 0x00000, 0x10000, 0x116027d7 )
	ROM_LOAD_ODD ("sf-22.bin", 0x00000, 0x10000, 0xd3cbd09e )
	ROM_LOAD_EVEN("sf-20.bin", 0x20000, 0x10000, 0xfe07e83f )
	ROM_LOAD_ODD ("sf-23.bin", 0x20000, 0x10000, 0x1e435d33 )
	ROM_LOAD_EVEN("sf-21.bin", 0x40000, 0x10000, 0xe086bc4c )
	ROM_LOAD_ODD ("sf-24.bin", 0x40000, 0x10000, 0x13a6696b )

	ROM_REGION_DISPOSE(0x344000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sf-25.bin", 0x000000, 0x020000, 0x7f23042e )	/* Background 1 planes 0-1 */
	ROM_LOAD( "sf-28.bin", 0x020000, 0x020000, 0x92f8b91c )
	ROM_LOAD( "sf-30.bin", 0x040000, 0x020000, 0xb1399856 )
	ROM_LOAD( "sf-34.bin", 0x060000, 0x020000, 0x96b6ae2e )
	ROM_LOAD( "sf-26.bin", 0x080000, 0x020000, 0x54ede9f5 ) /* planes 2-3 */
	ROM_LOAD( "sf-29.bin", 0x0a0000, 0x020000, 0xf0649a67 )
	ROM_LOAD( "sf-31.bin", 0x0c0000, 0x020000, 0x8f4dd71a )
	ROM_LOAD( "sf-35.bin", 0x0e0000, 0x020000, 0x70c00fb4 )
	ROM_LOAD( "sf-39.bin", 0x100000, 0x020000, 0xcee3d292 ) /* Background 2 planes 0-1*/
	ROM_LOAD( "sf-38.bin", 0x120000, 0x020000, 0x2ea99676 )
	ROM_LOAD( "sf-41.bin", 0x140000, 0x020000, 0xe0280495 ) /* planes 2-3 */
	ROM_LOAD( "sf-40.bin", 0x160000, 0x020000, 0xc70b30de )
	ROM_LOAD( "sf-15.bin", 0x180000, 0x020000, 0xfc0113db ) /* Sprites planes 1-2 */
	ROM_LOAD( "sf-16.bin", 0x1a0000, 0x020000, 0x82e4a6d3 )
	ROM_LOAD( "sf-11.bin", 0x1c0000, 0x020000, 0xe112df1b )
	ROM_LOAD( "sf-12.bin", 0x1e0000, 0x020000, 0x42d52299 )
	ROM_LOAD( "sf-07.bin", 0x200000, 0x020000, 0x49f340d9 )
	ROM_LOAD( "sf-08.bin", 0x220000, 0x020000, 0x95ece9b1 )
	ROM_LOAD( "sf-03.bin", 0x240000, 0x020000, 0x5ca05781 )
	ROM_LOAD( "sf-17.bin", 0x260000, 0x020000, 0x69fac48e ) /* planes 2-3 */
	ROM_LOAD( "sf-18.bin", 0x280000, 0x020000, 0x71cfd18d )
	ROM_LOAD( "sf-13.bin", 0x2a0000, 0x020000, 0xfa2eb24b )
	ROM_LOAD( "sf-14.bin", 0x2c0000, 0x020000, 0xad955c95 )
	ROM_LOAD( "sf-09.bin", 0x2e0000, 0x020000, 0x41b73a31 )
	ROM_LOAD( "sf-10.bin", 0x300000, 0x020000, 0x91c41c50 )
	ROM_LOAD( "sf-05.bin", 0x320000, 0x020000, 0x538c7cbe )
	ROM_LOAD( "sf-27.bin", 0x340000, 0x004000, 0x2b09b36d ) /* Characters planes 1-2 */

	ROM_REGION(0x40000) /* Backgrounds */
	ROM_LOAD( "sf-37.bin", 0x000000, 0x010000, 0x23d09d3d )
	ROM_LOAD( "sf-36.bin", 0x010000, 0x010000, 0xea16df6c )
	ROM_LOAD( "sf-32.bin", 0x020000, 0x010000, 0x72df2bd9 )
	ROM_LOAD( "sf-33.bin", 0x030000, 0x010000, 0x3e99d3d5 )

	ROM_REGION(0x10000)	/* 64k for the music CPU */
	ROM_LOAD( "sf-02.bin", 0x0000, 0x8000, 0x4a9ac534 )

	ROM_REGION(0x40000)	/* 256k for the samples CPU */
	ROM_LOAD( "sf-00.bin", 0x00000, 0x20000, 0x4b733845 )
	ROM_LOAD( "sf-01.bin", 0x20000, 0x20000, 0x86e0f0d5 )
ROM_END



struct GameDriver sf1_driver =
{
	__FILE__,
	0,
	"sf1",
	"Street Fighter (World)",
	"1987",
	"Capcom",
	"Olivier Galibert",
	0,
	&machine_driver,
	0,

	sf1_rom,
	0,0,0,0,

	sf1_input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver sf1us_driver =
{
	__FILE__,
	&sf1_driver,
	"sf1us",
	"Street Fighter (US)",
	"1987",
	"Capcom",
	"Olivier Galibert",
	0,
	&machineus_driver,
	0,

	sf1us_rom,
	0,0,0,0,

	sf1us_input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,
	0,0
};

struct GameDriver sf1jp_driver =
{
	__FILE__,
	&sf1_driver,
	"sf1jp",
	"Street Fighter (Japan)",
	"1987",
	"Capcom",
	"Olivier Galibert",
	0,
	&machinejp_driver,
	0,

	sf1jp_rom,
	0,0,0,0,

	sf1jp_input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,
	0,0
};
