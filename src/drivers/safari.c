/****************************************************************************

Safari Memory Map (preliminary)


0000-2fff ROM
8000-87ff ??? Maybe a dynamically generated SYNC RAM
e000-e37f Video RAM
e380-e7ff RAM
e800-efff Character generator RAM


I/O Port Read

03	Player Control
08  VBLANK(?) and DIP Switch


I/O Port Write

01  ???
03  Probably triggers sound effects
07  ???


TODO:
----

- Sound

- This game needs the overlay bad. The white rectangle on the screen is used
  to simulate the tree visible on the flyer.
  The flyer is at www.gamearchive.com/flyers/video/segagremlin/safari_b.jpg

- The high score is stored at e445-e449, but implementing it might be more
  trouble than it's worth it. (The high score needs to be put on the screen,
  and that's messy)

- Verify the processor speed

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern void safari_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void safari_characterram_w(int offset,int data);
extern unsigned char *safari_characterram;

static int coin_inserted;  // Active low

static void safari_init_machine(void)
{
	coin_inserted = 0x80;
}

static int safari_port_1_r(int offset)
{
	return (input_port_1_r(offset) & 0x7f) | coin_inserted;
}

// This game doesn't use interrupts, but this seemed like the most convinient
// place to put the coin check
static int safari_interrupt(void)
{
	// Need to reset the CPU when a coin is inserted
	if (input_port_2_r(0) & 0x01)
	{
		coin_inserted = 0;
		cpu_reset(0);
	}

	// This game doesn't use interrupts
	return ignore_interrupt();
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x27ff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },  // ???
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xe37f, videoram_w, &videoram, &videoram_size },
	{ 0xe380, 0xe7ff, MWA_RAM },
	{ 0x8000, 0x87ff, MWA_RAM },  // ???
	{ 0xe800, 0xefff, safari_characterram_w, &safari_characterram },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x03, 0x03, input_port_0_r },
	{ 0x08, 0x08, safari_port_1_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, IOWP_NOP },  // ???
	{ 0x03, 0x03, IOWP_NOP },  // Probably sound effect select
	{ 0x07, 0x07, IOWP_NOP },  // ???
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Aim Up", OSD_KEY_A, IP_JOY_NONE, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Aim Down", OSD_KEY_Z, IP_JOY_NONE, 0)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )  // ???
    PORT_BIT( 0x0e, IP_ACTIVE_HIGH,IPT_UNUSED )
	PORT_DIPNAME (0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING (   0x00, "4 Coins/1 Credit" )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH,IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_LOW,IPT_UNUSED ) // Used to indicate coin insertion

	PORT_START		/* IN2 */
	// This is fake. When a coin is inserted, the CPU is reset, and Bit 7 of
	// input port 1 is set low. We check for this in the fake interrupt handler
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
    256,    /* 256 characters */
    1,      /* 1 bit per pixel */
    { 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8   /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 0, 0xe800, &charlayout, 0, 1 },
	{ -1 }
};

// Black and White
unsigned char palette[3 * 3] =
{
        0x00, 0x00, 0x00,
        0xc0, 0xc0, 0xc0,
        0xff, 0x00, 0x00  // For MAME's use
};

unsigned short colortable[] = { 0, 1 };

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
            4000000,    /* 4MHz ??? */
			0,
			readmem,writemem,readport,writeport,
            safari_interrupt,1  // This is not a real interrupt
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,						/* Single CPU game */
    safari_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
    gfxdecodeinfo,
    sizeof(palette)/3,sizeof(colortable),
    0,


    VIDEO_TYPE_RASTER,
	0,
    generic_vh_start,
    generic_vh_stop,
    safari_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

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

	ROM_REGION(0x1000)  /* temporary space for graphics (disposed after conversion) */
ROM_END


struct GameDriver safari_driver =
{
    "Safari",
    "safari",
    "Zsolt Vasvari",
	&machine_driver,

    safari_rom,
	0, 0,
	0,
	0,	/* sound_prom */

    input_ports,

    0,palette,colortable,
    ORIENTATION_DEFAULT,

	0, 0
};

