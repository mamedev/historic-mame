#include "driver.h"
#include "vidhrdw/generic.h"

int gameplan_vh_start(void);
void gameplan_vh_stop(void);
void gameplan_video_w(int offset, int data);
void gameplan_vh_screenrefresh(struct osd_bitmap *bitmap);
void gameplan_select_port(int offset, int data);
int gameplan_read_port(int offset);

static int gameplan_current_port;

void gameplan_select_port(int offset, int data)
{
	switch(data)
	{
		case 0x01: gameplan_current_port = 0; break;
		case 0x02: gameplan_current_port = 1; break;
		case 0x04: gameplan_current_port = 2; break;
		case 0x08: gameplan_current_port = 3; break;
		case 0x80: gameplan_current_port = 4; break;
		case 0x40: gameplan_current_port = 5; break;

		default:
			if (errorlog)
				fprintf(errorlog, "strange port request byte: %02x\n", data);
			break;
	}
}

int gameplan_read_port(int offset)
{
	return readinputport(gameplan_current_port);
}

static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x01ff, MRA_RAM },
    { 0x032d, 0x03d8, MRA_RAM }, /* note: 300-32c and 3d9-3ff is
								  * written but never read?
								  * (write by code at e1df and e1e9,
								  * 32d is read by e258)*/
    { 0x2801, 0x2801, gameplan_read_port },
    { 0xc000, 0xffff, MRA_ROM },

    { 0x200d, 0x200d, MRA_RAM }, /* suspect from here down*/
	{ 0x3000, 0x3000, MRA_RAM },
    { -1 }						/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
    { 0x0300, 0x03ff, MWA_RAM },
    { 0x2000, 0x2003, gameplan_video_w },
    { 0x2800, 0x2800, gameplan_select_port },
	{ 0xc000, 0xffff, MWA_ROM },

    { 0x200c, 0x200e, MWA_RAM }, /* suspect from here down */
    { 0x2802, 0x2803, MWA_RAM },
    { 0x280c, 0x280c, MWA_RAM },
    { 0x280e, 0x280e, MWA_RAM },
    { 0x3001, 0x3001, MWA_RAM },
    { 0x3002, 0x3002, MWA_RAM },
    { 0x3003, 0x3003, MWA_RAM },
    { 0x300c, 0x300c, MWA_RAM },
    { 0x300e, 0x300e, MWA_RAM },

	{ -1 }						/* end of table */
};

#ifdef USE_SOUND_ROMS
static struct MemoryReadAddress readmem_snd[] =
{
#define MAP_READS
#ifdef MAP_READS
#endif /* MAP_READS */
    { 0xf800, 0xffff, MRA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_snd[] =
{
/* #define MAP_WRITES */
#ifdef MAP_WRITES
	{ 0x0000, 0x01ff, MWA_RAM },
#endif /* MAP_WRITES */
	{ 0xf800, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};
#endif /* USE_SOUND_ROMS */

int gameplan_interrupt(void)
{
	return 1;
}

INPUT_PORTS_START( killcom_input_ports )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_TILT,
			  "Slam", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0,
			  "Do Tests", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE,
			  "Select Test", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )	 /* coin sw. no.3 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )	 /* coin sw. no.2 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )	 /* coin sw. no.1 */

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )  /* 2 player start */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )  /* 1 player start */

	PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1,
			  "P1 Hyperspace", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1,
			  "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1,
			  "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1,
			  "P1 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1,
			  "P1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1,
			  "P1 Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1,
			  "P1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1,
			  "P1 Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2,
			  "P2 Hyperspace", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2,
			  "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2,
			  "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2,
			  "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2,
			  "P2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2,
			  "P2 Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2,
			  "P2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2,
			  "P2 Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */
	PORT_DIPNAME(0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(   0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(   0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(   0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(   0x00, "Free Play" )
	PORT_DIPNAME(0x04, 0x04, "Dip A 3", IP_KEY_NONE )
	PORT_DIPSETTING(   0x04, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x08, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "4" )
	PORT_DIPSETTING(   0x08, "5" )
	PORT_DIPNAME(0x10, 0x10, "Dip A 5", IP_KEY_NONE )
	PORT_DIPSETTING(   0x10, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x20, 0x20, "Dip A 6", IP_KEY_NONE )
	PORT_DIPSETTING(   0x20, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0xc0, 0xc0, "Reaction", IP_KEY_NONE )
	PORT_DIPSETTING(   0xc0, "Slowest" )
	PORT_DIPSETTING(   0x80, "Slow" )
	PORT_DIPSETTING(   0x40, "Fast" )
	PORT_DIPSETTING(   0x00, "Fastest" )

	PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */
	PORT_DIPNAME(0x01, 0x01, "Dip B 1", IP_KEY_NONE )
	PORT_DIPSETTING(   0x01, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x02, 0x02, "Dip B 2", IP_KEY_NONE )
	PORT_DIPSETTING(   0x02, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x04, 0x04, "Dip B 3", IP_KEY_NONE )
	PORT_DIPSETTING(   0x04, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x08, 0x08, "Dip B 4", IP_KEY_NONE )
	PORT_DIPSETTING(   0x08, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x10, 0x10, "Dip B 5", IP_KEY_NONE )
	PORT_DIPSETTING(   0x10, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x20, 0x20, "Dip B 6", IP_KEY_NONE )
	PORT_DIPSETTING(   0x20, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(   0x40, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(   0x80, "Upright" )
	PORT_DIPSETTING(   0x00, "Cocktail" )
INPUT_PORTS_END


INPUT_PORTS_START( megatack_input_ports )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_TILT,
			  "Slam", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0,
			  "Do Tests", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE,
			  "Select Test", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )	 /* coin sw. no.3 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )	 /* coin sw. no.2 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )	 /* coin sw. no.1 */

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )  /* 2 player start */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )  /* 1 player start */

	PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1,
			  "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1,
			  "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1,
			  "P1 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1,
			  "P1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1,
			  "P1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2,
			  "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2,
			  "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2,
			  "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2,
			  "P2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2,
			  "P2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */
	PORT_DIPNAME(0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(   0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(   0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(   0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(   0x00, "Free Play" )
	PORT_DIPNAME(0x04, 0x04, "Dip A 3", IP_KEY_NONE )
	PORT_DIPSETTING(   0x04, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x08, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "3" )
	PORT_DIPSETTING(   0x08, "4" )
	PORT_DIPNAME(0x10, 0x10, "Dip A 5", IP_KEY_NONE )
	PORT_DIPSETTING(   0x10, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x20, 0x20, "Dip A 6", IP_KEY_NONE )
	PORT_DIPSETTING(   0x20, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x40, 0x40, "Dip A 7", IP_KEY_NONE )
	PORT_DIPSETTING(   0x40, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x80, 0x80, "Dip A 8", IP_KEY_NONE )
	PORT_DIPSETTING(   0x80, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */
	PORT_DIPNAME(0x07, 0x07, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(   0x07, "20000" )
	PORT_DIPSETTING(   0x06, "30000" )
	PORT_DIPSETTING(   0x05, "40000" )
	PORT_DIPSETTING(   0x04, "50000" )
	PORT_DIPSETTING(   0x03, "60000" )
	PORT_DIPSETTING(   0x02, "70000" )
	PORT_DIPSETTING(   0x01, "80000" )
	PORT_DIPSETTING(   0x00, "90000" )
	PORT_DIPNAME(0x08, 0x08, "Dip B 4", IP_KEY_NONE )
	PORT_DIPSETTING(   0x08, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x10, 0x10, "Monitor View", IP_KEY_NONE )
	PORT_DIPSETTING(   0x10, "Direct" )
	PORT_DIPSETTING(   0x00, "Mirror" )
	PORT_DIPNAME(0x20, 0x20, "Monitor Orientation", IP_KEY_NONE )
	PORT_DIPSETTING(   0x20, "Horizontal" )
	PORT_DIPSETTING(   0x00, "Vertical" )
	PORT_DIPNAME(0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(   0x40, "Off" )
	PORT_DIPSETTING(   0x00, "On" )
	PORT_DIPNAME(0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(   0x80, "Upright" )
	PORT_DIPSETTING(   0x00, "Cocktail" )
INPUT_PORTS_END


static unsigned char palette[] =
{
	0xff,0xff,0xff, /* WHITE   */
	0xff,0xff,0x20, /* YELLOW  */
	0xff,0x20,0xff, /* MAGENTA */
	0xff,0x20,0x20, /* RED     */
	0x20,0xff,0xff, /* CYAN    */
	0x20,0xff,0x20, /* GREEN   */
	0x20,0x20,0xFF, /* BLUE    */
	0x00,0x00,0x00, /* BLACK   */
};


static unsigned char colortable[] =
{
	0, 0,	0, 1,	0, 2,	0, 3,	0, 4,	0, 5,	0, 6,	0, 7,
};


static struct MachineDriver machine_driver =
{
    /* basic machine hardware */
    {							/* MachineCPU */
		{
			CPU_M6502,
			1000000,			/* 1 Mhz??? */
			0,					/* memory_region */
			readmem, writemem, 0, 0,
			gameplan_interrupt,1 /* 1 interrupt per frame */
		},
#if USE_SOUND_ROMS
		{
			CPU_M6502,
			1000000,			/* 1 Mhz??? */
			1,					/* memory_region */
			readmem_snd,writemem_snd,0,0,
			gameplan_interrupt,1
		},
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,							/* CPU slices per frame */
    0,							/* init_machine() */

    /* video hardware */
    32*8, 32*8,					/* screen_width, height */
    { 0, 32*8-1, 0, 32*8-1 },		/* visible_area */
    0,
    sizeof(palette)/3, sizeof(colortable), 0,

	VIDEO_TYPE_RASTER, 0,
	gameplan_vh_start,
	gameplan_vh_stop,
	gameplan_vh_screenrefresh,

	0, 0, 0, 0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kaos_rom )
    ROM_REGION(0x10000)
    ROM_LOAD( "kaosab.g1", 0xa000, 0x1000, 0x8ab39f1d )
    ROM_LOAD( "kaosab.f1", 0xb000, 0x1000, 0x26d3e973 )
    ROM_LOAD( "kaosab.j1", 0xc000, 0x1000, 0x5768e2da )
    ROM_LOAD( "kaosab.j2", 0xd000, 0x1000, 0xa40d1c5d )
	/* can't find anything to map to 0xa000 - A911, A929, (ac5f for
	   nmi?), AF40 should all be 'sensible' code */
    ROM_LOAD( "kaosab.g2", 0x9000, 0x1000, 0x1bf0f6b2 )	/* ok */
    ROM_LOAD( "kaosab.e1", 0xf000, 0x1000, 0x42b95983 )	/* ok */

    ROM_REGION(0x10000)
	ROM_LOAD( "kaossnd.e1", 0xf800, 0x800, 0xd406d7a6 )
ROM_END


ROM_START( killcom_rom )
    ROM_REGION(0x10000)
    ROM_LOAD( "killcom.e2", 0xc000, 0x800, 0x5cf2ceb6 )
    ROM_LOAD( "killcom.f2", 0xc800, 0x800, 0x28b53265 )
    ROM_LOAD( "killcom.g2", 0xd000, 0x800, 0xc85a6fe6 )
    ROM_LOAD( "killcom.j2", 0xd800, 0x800, 0xdcfaa92a )
    ROM_LOAD( "killcom.j1", 0xe000, 0x800, 0x8772c5e8 )
    ROM_LOAD( "killcom.g1", 0xe800, 0x800, 0xf2738623 )
    ROM_LOAD( "killcom.f1", 0xf000, 0x800, 0xca4212d4 )
    ROM_LOAD( "killcom.e1", 0xf800, 0x800, 0x6ed6e10a )

    ROM_REGION(0x10000)
	ROM_LOAD( "killsnd.e1", 0xf800, 0x800, 0x5ac95bbb )
ROM_END

ROM_START( megatack_rom )
    ROM_REGION(0x10000)
    ROM_LOAD( "megattac.e2", 0xc000, 0x800, 0xe1dfd187 )
    ROM_LOAD( "megattac.f2", 0xc800, 0x800, 0x64875115 )
    ROM_LOAD( "megattac.g2", 0xd000, 0x800, 0xcf270d9d )
    ROM_LOAD( "megattac.j2", 0xd800, 0x800, 0xf8d4bdea )
    ROM_LOAD( "megattac.j1", 0xe000, 0x800, 0x2ab96ecb )
    ROM_LOAD( "megattac.g1", 0xe800, 0x800, 0x57427484 )
    ROM_LOAD( "megattac.f1", 0xf000, 0x800, 0xd0fdd29f )
    ROM_LOAD( "megattac.e1", 0xf800, 0x800, 0x5a5799db )

    ROM_REGION(0x10000)
	ROM_LOAD( "megatsnd.e1", 0xf800, 0x800, 0xf47bab9f )
ROM_END


struct GameDriver kaos_driver =
{
	"Kaos",
	"kaos",
	"Chris Moore (MAME driver)\n"
	"Santeri Saarimaa (hardware info)",
	&machine_driver,

	kaos_rom,
	0, 0, 0, 0,					/* sound_prom */

	megatack_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};


struct GameDriver killcom_driver =
{
	"Killer Comet",
	"killcom",
	"Chris Moore (MAME driver)\n"
	"Santeri Saarimaa (hardware info)",
	&machine_driver,

	killcom_rom,
	0, 0, 0, 0,					/* sound_prom */

	killcom_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};


struct GameDriver megatack_driver =
{
	"MegaTack",
	"megatack",
	"Chris Moore (MAME driver)\n"
	"Santeri Saarimaa (hardware info)",
	&machine_driver,

	megatack_rom,
	0, 0, 0, 0,					/* sound_prom */

	megatack_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};
