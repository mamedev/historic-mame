/****************************************************************************

Sound chip is a 76477

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern void zarzon_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void zarzon_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void zarzon_characterram_w(int offset,int data);

extern unsigned char *zarzon_videoram1;
extern unsigned char *zarzon_character_ram;

extern int zarzon_input_r(int offset);
extern void zarzon_control1_w(int offset, int data);
extern void zarzon_backcolor_w(int offset, int data);
extern void zarzon_sound_w(int offset, int data);



void zarzon_control1_w(int offset, int data)
{

}

void zarzon_sound_w(int offset, int data) {

}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x4000, 0x97FF, MRA_ROM },
	{ 0xB004, 0xB004, input_port_0_r }, /* IN0 */
	{ 0xB005, 0xB005, input_port_1_r }, /* IN1 */
	{ 0xB006, 0xB006, input_port_2_r }, /* IN2 */
	{ 0xB007, 0xB007, input_port_3_r }, /* IN3 */
	{ 0xF800, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &zarzon_videoram1 },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1FFF, zarzon_characterram_w, &zarzon_character_ram },
	{ 0xB000, 0xB001, zarzon_sound_w },
	{ 0xB002, 0xB002, zarzon_control1_w },
	{ 0xB003, 0xB003, zarzon_backcolor_w },
	{ -1 }	/* end of table */
};



static int zarzon_interrupt(void)
{
	if (cpu_getiloops() != 0)
	{
		/* user asks to insert coin: generate a NMI interrupt. */
		if (readinputport(3) & 1)
			return nmi_interrupt();
		else return ignore_interrupt();
	}
	else return interrupt();	/* one IRQ per frame */
}

INPUT_PORTS_START( zarzon_input_ports )
    PORT_START  /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
    PORT_BIT( 0x7C, IP_ACTIVE_HIGH, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

    PORT_START  /* IN2 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright")
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME (0x0a, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x00, "1 Coin/1 Credits" )
	PORT_DIPSETTING (   0x02, "1 Coin/2 Credit" )
	/* 0x0a gives 2/1 again */
	PORT_DIPNAME (0x04, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "5000" )
	PORT_DIPSETTING (   0x04, "10000" )
	PORT_DIPNAME (0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "3" )
	PORT_DIPSETTING (   0x10, "4" )
	PORT_DIPSETTING (   0x20, "5" )
	/* 0x30 gives 3 again */
	PORT_DIPNAME (0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x40, "On" )
	PORT_DIPNAME (0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x80, "On" )

    PORT_START  /* IN3 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
    PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* connected to a counter - random number generator? */
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
    256,    /* 256 characters */
    2,      /* 2 bits per pixel */
    { 0, 256*8*8 },
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8   /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0x0000, &charlayout,   0, 4 },
    { 0, 0x1000, &charlayout, 4*4, 4 },
	{ -1 }
};

static unsigned char color_prom[] =
{
	0x00,0xF8,0x02,0xFF,0xF8,0x27,0xC0,0xF8,0xC0,0x80,0x07,0x07,0xFF,0xF8,0x3F,0xFF,
	0x00,0xF8,0x02,0xFF,0xC0,0xC0,0x80,0x26,0x38,0xA0,0xA0,0x04,0x07,0xC6,0xC5,0x27
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
            11289000/16,    /* 700 kHz */
			0,
			readmem,writemem,0,0,
            zarzon_interrupt,2
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
    0,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
    gfxdecodeinfo,
    32,4*4 + 4*4,
    zarzon_vh_convert_color_prom,


    VIDEO_TYPE_RASTER,
	0,
    generic_vh_start,
    generic_vh_stop,
    zarzon_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zarzon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ZARZ122.07", 0x4000, 0x0800, 0xa260b9f8 )
	ROM_LOAD( "ZARZ123.08", 0x4800, 0x0800, 0xf2b8072c )
	ROM_LOAD( "ZARZ124.09", 0x5000, 0x0800, 0xdea47b9a )
	ROM_LOAD( "ZARZ125.10", 0x5800, 0x0800, 0xa30532d5 )
	ROM_LOAD( "ZARZ126.13", 0x6000, 0x0800, 0x043c84ba )
	ROM_LOAD( "ZARZ127.14", 0x6800, 0x0800, 0xa3f1286b )
	ROM_LOAD( "ZARZ128.15", 0x7000, 0x0800, 0xfbc89252 )
	ROM_LOAD( "ZARZ129.16", 0x7800, 0x0800, 0xc7440c84 )
	ROM_RELOAD( 0xf800, 0x0800 ) /* for the reset/interrupt vectors */
	ROM_LOAD( "ZARZ130.22", 0x8000, 0x0800, 0x78362c82 )
	ROM_LOAD( "ZARZ131.23", 0x8800, 0x0800, 0x566914b5 )
	ROM_LOAD( "ZARZ132.24", 0x9000, 0x0800, 0x7c4f3143 )

	ROM_REGION(0x1000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ZARZ135.73", 0x0000, 0x0800, 0xbc67fa61 )
	ROM_LOAD( "ZARZ136.75", 0x0800, 0x0800, 0x2364fe46 )

    ROM_REGION(0x1000)  /* sound data - samples? waveforms? Vanguard-style audio section? */
	ROM_LOAD( "ZARZ133.53", 0x0000, 0x0800, 0x4b404b14 )
	ROM_LOAD( "ZARZ134.54", 0x0800, 0x0800, 0x01380400 )
ROM_END


struct GameDriver zarzon_driver =
{
    "Zarzon",
    "zarzon",
    "Dan Boris\nTheo Philips",
	&machine_driver,

    zarzon_rom,
	0, 0,
	0,
	0,	/* sound_prom */

    zarzon_input_ports,

    color_prom,0,0,
    ORIENTATION_ROTATE_90,

	0, 0
};

