/***************************************************************************

Zaxxon memory map (preliminary)

0000-1fff ROM 3
2000-3fff ROM 2
4000-4fff ROM 1
6000-67ff RAM 1
6800-6fff RAM 2
8000-83ff Video RAM
a000-a0ff sprites

read:
c000      IN0
c001      IN1
c002      DSW0
c003      DSW1
c100      IN2
see the input_ports definition below for details on the input bits

write:
c000-c002 ?
c006      ?
ff3c-ff3f ?
fff0      interrupt enable
fff1      ?
fff8-fff9 background playfield position
fffa      background color? (0 = standard  1 = reddish)
fffb      background enable?
fffe      ?

interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI enters the test mode.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int zaxxon_IN2_r(int offset);

extern unsigned char *zaxxon_background_position;
extern unsigned char *zaxxon_background_color;
extern unsigned char *zaxxon_background_enable;
void zaxxon_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int  zaxxon_vh_start(void);
void zaxxon_vh_stop(void);
void zaxxon_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa0ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },	/* IN0 */
	{ 0xc001, 0xc001, input_port_1_r },	/* IN1 */
	{ 0xc100, 0xc100, input_port_2_r },	/* IN2 */
	{ 0xc002, 0xc002, input_port_3_r },	/* DSW0 */
	{ 0xc003, 0xc003, input_port_4_r },	/* DSW1 */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xfff0, 0xfff0, interrupt_enable_w },
	{ 0xfff8, 0xfff9, MWA_RAM, &zaxxon_background_position },
	{ 0xfffa, 0xfffa, MWA_RAM, &zaxxon_background_color },
	{ 0xfffb, 0xfffb, MWA_RAM, &zaxxon_background_enable },
	{ -1 }  /* end of table */
};



/***************************************************************************

  Zaxxon uses NMI to trigger the self test. We use a fake input port to
  tie that event to a keypress.

***************************************************************************/
static int zaxxon_interrupt(void)
{
	if (readinputport(5) & 1)	/* get status of the F2 key */
		return nmi_interrupt();	/* trigger self test */
	else return interrupt();
}

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE,
			"Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	/* Coin Aux doesn't need IMPULSE to pass the test, but it still needs it */
	/* to avoid the freeze. */
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_COIN3 | IPF_IMPULSE,
			"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty???", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Easy?" )
	PORT_DIPSETTING(    0x04, "Medium?" )
	PORT_DIPSETTING(    0x08, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_DIPNAME( 0x40, 0x40, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* DSW1 */
 	PORT_DIPNAME( 0x0f, 0x03, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0f, "4/1" )
	PORT_DIPSETTING(    0x07, "3/1" )
	PORT_DIPSETTING(    0x0b, "2/1" )
	PORT_DIPSETTING(    0x06, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0a, "2/1+Bonus each 2" )
	PORT_DIPSETTING(    0x03, "1/1" )
	PORT_DIPSETTING(    0x02, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0c, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0x04, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0x0d, "1/2" )
	PORT_DIPSETTING(    0x08, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x00, "1/2+Bonus each 4" )
	PORT_DIPSETTING(    0x05, "1/3" )
	PORT_DIPSETTING(    0x09, "1/4" )
	PORT_DIPSETTING(    0x01, "1/5" )
	PORT_DIPSETTING(    0x0e, "1/6" )
	PORT_DIPNAME( 0xf0, 0x30, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0xf0, "4/1" )
	PORT_DIPSETTING(    0x70, "3/1" )
	PORT_DIPSETTING(    0xb0, "2/1" )
	PORT_DIPSETTING(    0x60, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0xa0, "2/1+Bonus each 2" )
	PORT_DIPSETTING(    0x30, "1/1" )
	PORT_DIPSETTING(    0x20, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0xc0, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0x40, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0xd0, "1/2" )
	PORT_DIPSETTING(    0x80, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x00, "1/2+Bonus each 4" )
	PORT_DIPSETTING(    0x50, "1/3" )
	PORT_DIPSETTING(    0x90, "1/4" )
	PORT_DIPSETTING(    0x10, "1/5" )
	PORT_DIPSETTING(    0xe0, "1/6" )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
INPUT_PORTS_END



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 103*8, 102*8, 101*8, 100*8, 99*8, 98*8, 97*8, 96*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,          0, 16 },	/* characters */
	{ 1, 0x1000, &spritelayout,      16*4, 32 },	/* sprites */
	{ 1, 0x7000, &charlayout2,  16*4+32*8, 32 },	/* background tiles */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
  0x00,0x00,0x00, /* BLACK */
  0xFF,0x00,0x00, /* RED */
  0xdb,0x92,0x49, /* BROWN */
  0xff,0xb6,0xdb, /* PINK */
  0xFF,0xFF,0xFF, /* WHITE */
  0x00,0xFF,0xFF, /* CYAN */
  0x49,0xb6,0xdb, /* DKCYAN */
  0xFF,0x60,0x00, /* DKORANGE */
  0x00,0x00,0x96, /* DKBLUE */
  0xFF,0xFF,0x00, /* YELLOW */
  0x03,0x96,0xd2, /* LTBLUE */
  0x24,0x24,0xdb, /* BLUE */
  0x00,0xdb,0x00, /* GREEN */
  0x49,0xb6,0x92, /* DKGREEN */
  0xff,0xb6,0x92, /* LTORANGE */
  0xb6,0xb6,0xb6, /* GRAY */
  0x19,0x96,0x62, /* VDKGREEN */
  0xC0,0x00,0x00, /* DKRED */

  0xFF,0x00,0xFF, /* CUSTOM1 magenta*/
  0x80,0xC0,0xFF, /* CUSTOM2 blue plane*/
  0xFF,0xE0,0x00, /* CUSTOM3 */
  0xFF,0xC0,0x40, /* CUSTOM4 */

  0xc0,0xff,0x00, /* GREEN1 */
  0x80,0xe0,0x00, /* GREEN2 */
  0x40,0xc0,0x00, /* GREEN3 */

  0x00,0x00,0x80, /* BACK1 dark blue*/
  0x00,0x00,0xC0, /* BACK2 Blue */
  0x40,0xA0,0xFF, /* BACK3 */
  0x60,0xA0,0xE0, /* BACK4 */
  0xA0,0xD0,0xF0, /* BACK5 */
  0xC0,0xE0,0xFF, /* BACK6 */
  0x00,0x60,0xC0, /* BACK7 */
  0xE0,0x80,0xE0, /* BACK8 */
  0x50,0x90,0xD0, /* BACK9 */
  0x40,0x80,0xC0, /* BACK10 */
  0x80,0xB0,0xE0, /* BACK11 */

  0x00,0x00,0xFF, /* BLUE1 */
  0x00,0x00,0xC0, /* BLUE2 */
  0x00,0x00,0x80, /* BLUE3 */

  0xFF,0xFF,0xFF, /* YELLOW1 */
  0xFF,0xFF,0x00, /* YELLOW2 */
  0xFF,0xE0,0x00, /* YELLOW3 */
  0xE0,0xC0,0x00, /* YELLOW4 */
  0xD0,0xA0,0x00, /* YELLOW5 */

  0xA0,0x00,0x00, /* DKRED2 */
  0x80,0x00,0x00, /* DKRED3 */

  0x80,0xA0,0xC0, /* GRAY1 */
  0x90,0xA0,0xC0, /* GRAY2 */
  0xC0,0xE0,0xFF, /* GRAY3 */
  0xA0,0xC0,0xE0, /* GRAY4 */
  0x80,0xA0,0xC0, /* GRAY5 */
  0x60,0x80,0xA0, /* GRAY6 */
};

enum {BLACK,RED,BROWN,PINK,WHITE,CYAN,DKCYAN,DKORANGE,
		DKBLUE,YELLOW,LTBLUE,BLUE,GREEN,DKGREEN,LTORANGE,GRAY,
		VDKGREEN,DKRED,
		CUSTOM1,CUSTOM2,CUSTOM3,CUSTOM4,
		GREEN1,GREEN2,GREEN3,
		BACK1,BACK2,BACK3,BACK4,BACK5,BACK6,BACK7,BACK8,BACK9,BACK10,BACK11,
		BLUE1,BLUE2,BLUE3,
		YELLOW1,YELLOW2,YELLOW3,YELLOW4,YELLOW5,
		DKRED2,DKRED3,GRAY1,GRAY2,GRAY3,GRAY4,GRAY5,GRAY6};

static unsigned char colortable[] =
{
	/* chars */
	0,BLACK,BLACK,WHITE,          /* 0-9 */
	0,BLACK,BLACK,WHITE,          /* A-O */
	0,BLACK,BLACK,WHITE,          /* P-Z */
	0,BLACK,BLACK,WHITE,          /* = */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,BLACK,YELLOW,BLACK,         /* Round Flag - Stick */
	0,BLACK,YELLOW,RED,           /* Round Flag - Flag */
	0,BLACK,BACK5,BLACK,
	0,0,0,0,
	0,0,0,0,
	0,BLACK,BACK5,RED,            /* Altitude Bar - Foreground */
	0,BLACK,BACK5,BLACK,          /* Altitude Bar - Background */
	0,BLACK,YELLOW,RED,           /* Fuel Bar & Remaining Ships */

 /*       0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,    */

	/* sprites */
	0,YELLOW3,YELLOW1,YELLOW2,YELLOW2,YELLOW1,YELLOW5,DKORANGE,   /* Explosion Space */
	0,RED,WHITE,CYAN,YELLOW,BLUE,GREEN,DKORANGE,                  /* Explosion plane 1 */
	0,DKRED,YELLOW,CYAN,RED,BLUE,GREEN,DKORANGE,                  /* Explosion plane 2 */
	0,RED,DKRED,RED,DKORANGE,RED,DKRED,DKRED2,                    /* Explosion Rocket 1 and NMI bullet in space (low level)*/
	0,BLACK,BLACK,RED,DKORANGE,RED,DKRED,DKRED2,                /* Player Bullet and NMI in space */
	0,0,0,0,0,0,0,0,
	0,RED,RED,RED,RED,RED,RED,RED,                            /* NMI Bullet */
	0,BLUE2,RED,BACK5,BACK4,BLUE1,BLUE3,BLACK,                /* Satellite 1/2 */
	0,DKRED,BLACK,DKCYAN,RED,RED,RED,RED,                     /* Plane When Hit and Cannon bullet */
	0,BLACK,GRAY5,GRAY1,GRAY2,GRAY3,GRAY4,GRAY6,              /* Cannon 2 */
	0,WHITE,BACK3,BACK5,BACK6,BACK9,DKORANGE,BLACK,           /* NMI Plane 1 (and Standing planes) */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BLACK,WHITE,BLACK,BLACK,BLACK,BLACK,BLACK,              /* Cross-Target */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,CUSTOM1,RED,BACK5,CUSTOM3,CUSTOM4,BLACK,WHITE,          /* Plane and Force field */
	0,YELLOW1,YELLOW2,YELLOW3,YELLOW4,YELLOW5,BLACK,BLACK,    /* NMI Plane 2 */
	0,BLACK,GREEN3,RED,CUSTOM3,GREEN1,GREEN2,GREEN2,          /* Cannon 1 */
	0,WHITE,BLACK,BACK6,BACK5,BACK4,LTBLUE,BLACK,             /* Wall and Junk pile from shooting radar,fuel...(with score) */
	0,GRAY5,BLACK,RED,DKRED,DKORANGE,CUSTOM3,WHITE,           /* Radar & Fuel */
	0,RED,DKRED,RED,DKORANGE,RED,CUSTOM3,LTORANGE,            /* Rocket 1 */
	0,WHITE,BACK6,BACK5,BACK4,BACK11,BACK10,BLACK,            /* Zaxxon 1 */
	0,BLUE2,CYAN,BACK5,BACK4,BLUE1,BLUE3,BLACK,               /* Satellite 2/2 */
	0,WHITE,PINK,DKORANGE,CYAN,RED,DKRED,BLACK,               /* Zaxxon 2 (RED) */
	0,RED,DKRED,RED,DKORANGE,YELLOW,LTORANGE,LTORANGE,        /* Rocket 2 */
	0,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,              /* Shadow of Plane */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BROWN,DKORANGE,DKRED2,RED,DKRED,DKRED2,BLACK,           /* Zaxxon Dead (more RED) */
	0,0,0,0,0,0,0,0,

	/* background tiles (normal) */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BLACK,BACK7,BACK8,BACK2,BACK4,BLACK,BLACK,              /* Zaxxon Arena */
	0,DKBLUE,LTBLUE,WHITE,DKCYAN,BLACK,BLUE,CYAN,             /* Space BG */
	0,BACK1,WHITE,BACK5,BLACK,BACK6,BACK2,BACK4,              /* City BG 2 */
	0,BACK2,BACK5,BACK4,BACK6,BACK1,BACK3,BLACK,              /* City BG 1 */
	0,WHITE,BACK6,BLACK,BACK5,BACK4,YELLOW,BACK2,             /* Buildings */
	/* background tiles (during death sequence) */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BLACK,BLACK,RED,DKRED2,DKRED3,BLACK,BLACK,              /* Zaxxon Arena */
	0,DKBLUE,LTBLUE,WHITE,DKCYAN,DKRED2,BLUE,CYAN,            /* Space BG */
	0,DKRED3,WHITE,BACK4,BLACK,WHITE,DKRED2,BACK10,           /* City BG 2 */
	0,DKRED2,BACK4,BACK10,WHITE,DKRED3,BACK3,BLACK,           /* City BG 1 */
	0,WHITE,WHITE,BLACK,BACK4,BACK10,YELLOW,DKRED2            /* Buildings */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			zaxxon_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER,
	0,
	zaxxon_vh_start,
	zaxxon_vh_stop,
	zaxxon_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zaxxon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "zaxxon.3", 0x0000, 0x2000, 0x5ab9126b )
	ROM_LOAD( "zaxxon.2", 0x2000, 0x2000, 0x37df69f1 )
	ROM_LOAD( "zaxxon.1", 0x4000, 0x1000, 0xba9f739d )

	ROM_REGION(0xd000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "zaxxon.15", 0x0000, 0x0800, 0x77cd19bf )
	ROM_LOAD( "zaxxon.14", 0x0800, 0x0800, 0xec944b06 )
	ROM_LOAD( "zaxxon.13", 0x1000, 0x2000, 0xe2f87f5c )
	ROM_LOAD( "zaxxon.12", 0x3000, 0x2000, 0x4b4f1449 )
	ROM_LOAD( "zaxxon.11", 0x5000, 0x2000, 0x666b4c71 )
	ROM_LOAD( "zaxxon.6",  0x7000, 0x2000, 0x27b35545 )
	ROM_LOAD( "zaxxon.5",  0x9000, 0x2000, 0x1b73959d )
	ROM_LOAD( "zaxxon.4",  0xb000, 0x2000, 0x3deedf22 )

	ROM_REGION(0x8000)	/* background graphics */
	ROM_LOAD( "zaxxon.8",  0x0000, 0x2000, 0x974ee47c )
	ROM_LOAD( "zaxxon.7",  0x2000, 0x2000, 0xa9dc2e00 )
	ROM_LOAD( "zaxxon.10", 0x4000, 0x2000, 0x46743110 )
	ROM_LOAD( "zaxxon.9",  0x6000, 0x2000, 0x6be85050 )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6110],"\x00\x89\x00",3) == 0 &&
			memcmp(&RAM[0x6179],"\x00\x37\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x6100],1,21*6,f);
			RAM[0x6038] = RAM[0x6110];
			RAM[0x6039] = RAM[0x6111];
			RAM[0x603a] = RAM[0x6112];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	/* make sure that the high score table is still valid (entering the */
	/* test mode corrupts it) */
	if (memcmp(&RAM[0x6110],"\x00\x00\x00",3) != 0)
	{
		FILE *f;


		if ((f = fopen(name,"wb")) != 0)
		{
			fwrite(&RAM[0x6100],1,21*6,f);
			fclose(f);
		}
	}
}



struct GameDriver zaxxon_driver =
{
	"Zaxxon",
	"zaxxon",
	"Mirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)\nMarc Vergoossen (colors)\nMarc LaFontaine (colors)",
	&machine_driver,

	zaxxon_rom,
	0, 0,
	0,

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
