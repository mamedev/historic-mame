/***************************************************************************

GDI Red Alert Driver

Everything in this driver is guesswork and speculation.  If something
seems wrong, it probably is.

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* vidhrdw/redalert.c */
extern unsigned char *redalert_backram;
extern unsigned char *redalert_spriteram1;
extern unsigned char *redalert_spriteram2;
extern unsigned char *redalert_characterram;
extern void redalert_backram_w(int offset, int data);
extern void redalert_spriteram1_w(int offset, int data);
extern void redalert_spriteram2_w(int offset, int data);
extern void redalert_characterram_w(int offset, int data);
extern void redalert_vh_screenrefresh(struct osd_bitmap *bitmap);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM }, /* Zero page / stack */
	{ 0x0300, 0x03ff, MRA_RAM }, /* ? */
	{ 0x0c00, 0x0cff, MRA_RAM }, /* ? */
	{ 0x0d00, 0x0dff, MRA_RAM }, /* ? */
	{ 0x0e00, 0x0eff, MRA_RAM }, /* ? */
	{ 0x0f00, 0x0fff, MRA_RAM }, /* ? */
	{ 0x1000, 0x1fff, MRA_RAM }, /* Video scratchpad RAM */
	{ 0x2000, 0x4fff, MRA_RAM }, /* Video RAM */
	{ 0x5000, 0xbfff, MRA_ROM },
	{ 0xc100, 0xc100, input_port_0_r },
	{ 0xc110, 0xc110, input_port_1_r },
	{ 0xc120, 0xc120, input_port_2_r },
	{ 0xc170, 0xc170, input_port_3_r },
	{ 0xf000, 0xffff, MRA_ROM }, /* remapped ROM for 6502 vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x0300, 0x03ff, MWA_RAM }, /* ? */
	{ 0x0c00, 0x0cff, MWA_RAM }, /* ? */
	{ 0x0d00, 0x0dff, MWA_RAM }, /* ? */
	{ 0x0e00, 0x0eff, MWA_RAM }, /* ? */
	{ 0x0f00, 0x0fff, MWA_RAM }, /* ? */
	{ 0x1000, 0x1fff, MWA_RAM }, /* Scratchpad video RAM */
	{ 0x2000, 0x3fff, redalert_backram_w, &redalert_backram },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, redalert_spriteram1_w, &redalert_spriteram1 },
	{ 0x4800, 0x4bff, redalert_characterram_w, &redalert_characterram },
	{ 0x4c00, 0x4fff, redalert_spriteram2_w, &redalert_spriteram2 },
	{ 0x5000, 0xbfff, MWA_ROM },
	{ 0xc130, 0xc130, MWA_RAM }, /* Output port? */
	{ 0xc140, 0xc140, MWA_RAM }, /* Output port? */
	{ 0xc150, 0xc150, MWA_RAM }, /* Output port? */
	{ 0xc160, 0xc160, MWA_RAM }, /* Output port? */
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( redalert_input_ports )
	PORT_START			   /* DIP Switches */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x08, "7000" )
	PORT_DIPNAME( 0x30, 0x10, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

   PORT_START			   /* IN1 */
   PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
   PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
   PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
   PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
   PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
   PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

   PORT_START			   /* IN2 - probably player 2 cocktail */
   PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

   PORT_START			   /* IN3 - probably coins */
   PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout backlayout =
{
	8,8,	/* 8*8 characters */
	0x400,	  /* 1024 characters */
	1,	/* 1 bits per pixel */
	{ 0 }, /* No info needed for bit offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	128,	/* 128 characters */
	1,	/* 1 bits per pixel */
	{ 0 }, /* No info needed for bit offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	8,8,	/* 8*8 characters */
	128,	/* 128 characters */
	2,		/* 1 bits per pixel */
	{ 0, 0x800*8 }, /* No info needed for bit offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x3000, &backlayout,	0, 8 }, 	/* the game dynamically modifies this */
	{ 0, 0x4800, &charlayout,	0, 8 }, 	/* the game dynamically modifies this */
	{ 0, 0x4400, &spritelayout,16, 1 }, 	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};

/* Arbitrary colors */
static unsigned char palette[] =
{
	0x00,0x00,0x00,
	0x00,0x00,0xff,
	0x00,0xff,0x00,
	0x00,0xff,0xff,
	0xff,0x00,0x00,
	0xff,0x00,0xff,
	0xff,0xff,0x00,
	0xff,0xff,0xff
};

static unsigned short colortable[] =
{
	0, 0,
	0, 1,
	0, 2,
	0, 3,
	0, 4,
	0, 5,
	0, 6,
	0, 7,
	0, 4, 5, 6,
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	   /* ???? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	redalert_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0

};


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( redalert_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "RAG5",	 0x5000, 0x1000, 0x73e366e1 )
	ROM_LOAD( "RAG6",	 0x6000, 0x1000, 0x14c2fb04 )
	ROM_LOAD( "RAG7N",	 0x7000, 0x1000, 0x4e8ca3b2 )
	ROM_LOAD( "RAG8N",	 0x8000, 0x1000, 0xbb20e5a2 )
	ROM_RELOAD( 		 0xF000, 0x1000 )
	ROM_LOAD( "RAG9",	 0x9000, 0x1000, 0x76d8c402 )
	ROM_LOAD( "RAGAB",	 0xa000, 0x1000, 0x104383a9 )
	ROM_LOAD( "RAGB",	 0xb000, 0x1000, 0xb8f2925c )

	ROM_REGION(0x100) /* temporary for graphics - unused */

	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "RAS1B",	 0x1000, 0x1000, 0x21cf7edb )
	ROM_LOAD( "RAS2",	 0x2000, 0x1000, 0x2e81d96b )
	ROM_LOAD( "RAS3",	 0x3000, 0x1000, 0x495b2183 )
	ROM_LOAD( "RAS4",	 0x4000, 0x1000, 0x0418b74e )
	ROM_LOAD( "W3S1",	 0x7800, 0x0800, 0x283b3023 )
	ROM_RELOAD( 		 0xF800, 0x0800 )

ROM_END

/***************************************************************************
  Hi Score Routines
***************************************************************************/


/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver redalert_driver =
{
	"Red Alert",
	"redalert",
	"Mike Balfour",
	&machine_driver,

	redalert_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	redalert_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,
	0,0
};
