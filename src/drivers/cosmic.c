/***************************************************************************

Space Panic memory map

0000-3FFF ROM
4000-5BFF Video RAM (Bitmap)
5C00-5FFF RAM
6000-601F Sprite Controller
          4 bytes per sprite

          byte 1 - 80 = ?
                   40 = Rotate sprite left/right
                   3F = Sprite Number (Conversion to my layout via table)

          byte 2 - X co-ordinate
          byte 3 - Y co-ordinate

          byte 4 - 08 = Switch sprite bank
                   07 = Colour

6800      IN1 - Player controls. See input port setup for mappings
6801      IN2 - Player 2 controls for cocktail mode. See input port setup for mappings.
6802      DSW - Dip switches
6803      IN0 - Coinage and player start
700C-700E Colour Map Selector

(Not Implemented)

7000-700B Various triggers, Sound etc
700F      Ditto
7800      80 = Flash Screen?

I/O ports:
read:

write:

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"



/**************************************************/
/* Cosmic routines                                */
/**************************************************/

int PixelClock = 0;

extern unsigned char *cosmic_videoram;

void cosmic_videoram_w(int offset,int data);
void cosmic_flipscreen_w(int offset, int data);
int  cosmic_vh_start(void);
void cosmic_vh_stop(void);
void cosmic_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void cosmic_vh_screenrefresh_sprites(struct osd_bitmap *bitmap,int full_refresh);

static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x5FFF, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x6800, 0x6800, input_port_0_r }, /* IN1 */
	{ 0x6801, 0x6801, input_port_1_r }, /* IN2 */
	{ 0x6802, 0x6802, input_port_2_r }, /* DSW */
	{ 0x6803, 0x6803, input_port_3_r }, /* IN0 */
	{ -1 }	/* end of table */
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

static struct Samplesinterface samples_interface =
{
	9       /* 9 channels */
};

/**************************************************/
/* Space Panic specific routines                  */
/**************************************************/

void panic_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void panic_colourmap_select(int offset,int data);
int  panic_interrupt(void);

static struct MemoryWriteAddress panic_writemem[] =
{
    { 0x4000, 0x43FF, MWA_RAM },
	{ 0x4400, 0x5BFF, cosmic_videoram_w, &cosmic_videoram },
	{ 0x5C00, 0x5FFF, MWA_RAM },
	{ 0x6000, 0x601F, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x0000, 0x3fff, MWA_ROM },
    { 0x700C, 0x700E, panic_colourmap_select },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x07, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x05, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/5 Credits" )
/* 0x06 and 0x07 disabled */
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPNAME( 0x20, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0xc0, 0x40, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/3 Credits" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

INPUT_PORTS_END


/* Main Sprites */

static struct GfxLayout panic_spritelayout0 =
{
	16,16,	/* 16*16 sprites */
	48 ,	/* 64 sprites */
	2,	    /* 2 bits per pixel */
	{ 4096*8, 0 },	/* the two bitplanes are separated */
	{ 0,1,2,3,4,5,6,7,16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
   	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

/* Man Top */

static struct GfxLayout panic_spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	16 ,	/* 16 sprites */
	2,	    /* 2 bits per pixel */
	{ 4096*8, 0 },	/* the two bitplanes are separated */
	{ 0,1,2,3,4,5,6,7,16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
  	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo panic_gfxdecodeinfo[] =
{
	{ 1, 0x0A00, &panic_spritelayout0, 0, 8 },	/* Monsters             */
	{ 1, 0x0200, &panic_spritelayout0, 0, 8 },	/* Monsters eating Man  */
	{ 1, 0x0800, &panic_spritelayout1, 0, 8 },	/* Man                  */
	{ -1 } /* end of array */
};

static struct MachineDriver panic_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,	/* 2 Mhz? */
			0,
			readmem,panic_writemem,0,0,
			panic_interrupt,2
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	panic_gfxdecodeinfo,
	16, 8*4,
	panic_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	cosmic_vh_start,
	cosmic_vh_stop,
	cosmic_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};


static int panic_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* wait for default to be copied */
	if (RAM[0x40c1] == 0x00 && RAM[0x40c2] == 0x03 && RAM[0x40c3] == 0x04)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
        	RAM[0x4004] = 0x01;	/* Prevent program resetting high score */

			osd_fread(f,&RAM[0x40C1],5);
                osd_fread(f,&RAM[0x5C00],12);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void panic_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x40C1],5);
        osd_fwrite(f,&RAM[0x5C00],12);
		osd_fclose(f);
	}
}

int panic_interrupt(void)
{
	static int count=0;

	count++;

	if (count == 1)
	{
		return 0x00cf;		/* RST 08h */
    }
    else
    {
        count=0;
        return 0x00d7;		/* RST 10h */
    }
}

ROM_START( panic_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "spcpanic.1",   0x0000, 0x0800, 0x405ae6f9 )         /* Code */
	ROM_LOAD( "spcpanic.2",   0x0800, 0x0800, 0xb6a286c5 )
	ROM_LOAD( "spcpanic.3",   0x1000, 0x0800, 0x85ae8b2e )
	ROM_LOAD( "spcpanic.4",   0x1800, 0x0800, 0xb6d4f52f )
	ROM_LOAD( "spcpanic.5",   0x2000, 0x0800, 0x5b80f277 )
	ROM_LOAD( "spcpanic.6",   0x2800, 0x0800, 0xb73babf0 )
	ROM_LOAD( "spcpanic.7",   0x3000, 0x0800, 0xfc27f4e5 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spcpanic.9",   0x0000, 0x0800, 0xeec78b4c )
	ROM_LOAD( "spcpanic.10",  0x0800, 0x0800, 0xc9631c2d )
	ROM_LOAD( "spcpanic.12",  0x1000, 0x0800, 0xe83423d0 )
	ROM_LOAD( "spcpanic.11",  0x1800, 0x0800, 0xacea9df4 )

	ROM_REGION(0x0820)	/* color PROMs */
	ROM_LOAD( "82S123.SP",    0x0000, 0x0020, 0x35d43d2f )
	ROM_LOAD( "spcpanic.8",   0x0020, 0x0800, 0x7da0b321 )
ROM_END

ROM_START( panica_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "panica.1",     0x0000, 0x0800, 0x289720ce )         /* Code */
	ROM_LOAD( "spcpanic.2",   0x0800, 0x0800, 0xb6a286c5 )
	ROM_LOAD( "spcpanic.3",   0x1000, 0x0800, 0x85ae8b2e )
	ROM_LOAD( "spcpanic.4",   0x1800, 0x0800, 0xb6d4f52f )
	ROM_LOAD( "spcpanic.5",   0x2000, 0x0800, 0x5b80f277 )
	ROM_LOAD( "spcpanic.6",   0x2800, 0x0800, 0xb73babf0 )
	ROM_LOAD( "panica.7",     0x3000, 0x0800, 0x3641cb7f )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spcpanic.9",   0x0000, 0x0800, 0xeec78b4c )
	ROM_LOAD( "spcpanic.10",  0x0800, 0x0800, 0xc9631c2d )
	ROM_LOAD( "spcpanic.12",  0x1000, 0x0800, 0xe83423d0 )
	ROM_LOAD( "spcpanic.11",  0x1800, 0x0800, 0xacea9df4 )

	ROM_REGION(0x0820)	/* color PROMs */
	ROM_LOAD( "82S123.SP",    0x0000, 0x0020, 0x35d43d2f )
	ROM_LOAD( "spcpanic.8",   0x0020, 0x0800, 0x7da0b321 )
ROM_END

ROM_START( panicger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "spacepan.001", 0x0000, 0x0800, 0xa6d9515a )         /* Code */
	ROM_LOAD( "spacepan.002", 0x0800, 0x0800, 0xcfc22663 )
	ROM_LOAD( "spacepan.003", 0x1000, 0x0800, 0xe1f36893 )
	ROM_LOAD( "spacepan.004", 0x1800, 0x0800, 0x01be297c )
	ROM_LOAD( "spacepan.005", 0x2000, 0x0800, 0xe0d54805 )
	ROM_LOAD( "spacepan.006", 0x2800, 0x0800, 0xaae1458e )
	ROM_LOAD( "spacepan.007", 0x3000, 0x0800, 0x14e46e70 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spcpanic.9",   0x0000, 0x0800, 0xeec78b4c )
	ROM_LOAD( "spcpanic.10",  0x0800, 0x0800, 0xc9631c2d )
	ROM_LOAD( "spcpanic.12",  0x1000, 0x0800, 0xe83423d0 )
	ROM_LOAD( "spcpanic.11",  0x1800, 0x0800, 0xacea9df4 )

	ROM_REGION(0x0820)	/* color PROMs */
	ROM_LOAD( "82S123.SP",    0x0000, 0x0020, 0x35d43d2f )
	ROM_LOAD( "spcpanic.8",   0x0020, 0x0800, 0x7da0b321 )
ROM_END


struct GameDriver panic_driver =
{
	__FILE__,
	0,
	"panic",
	"Space Panic (set 1)",
	"1980",
	"Universal",
	"Mike Coates (MAME driver)\nMarco Cassili",
	0,
	&panic_machine_driver,
	0,

	panic_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	panic_hiload, panic_hisave
};

struct GameDriver panica_driver =
{
	__FILE__,
	&panic_driver,
	"panica",
	"Space Panic (set 2)",
	"1980",
	"Universal",
	"Mike Coates (MAME driver)\nMarco Cassili",
	0,
	&panic_machine_driver,
	0,

	panica_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	panic_hiload, panic_hisave
};

struct GameDriver panicger_driver =
{
	__FILE__,
	&panic_driver,
	"panicger",
	"Space Panic (German)",
	"1980",
	"Universal (ADP Automaten license)",
	"Mike Coates (MAME driver)\nMarco Cassili",
	0,
	&panic_machine_driver,
	0,

	panicger_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	panic_hiload, panic_hisave
};


/**************************************************/
/* Cosmic Alien specific routines                 */
/**************************************************/

void cosmicalien_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void cosmicalien_colourmap_select(int offset,int data);

int cosmicalien_interrupt(void)
{
    PixelClock = (PixelClock + 2) & 63;

    if (PixelClock == 0)
    {
		if (readinputport(3) & 1)	/* Left Coin */
			return nmi_interrupt();
        else
        	return ignore_interrupt();
    }
	else
       	return ignore_interrupt();
}

int cosmicalien_video_address_r(int offset)
{
	return PixelClock;
}

static struct MemoryReadAddress cosmicalien_readmem[] =
{
	{ 0x4000, 0x5FFF, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x6800, 0x6800, input_port_0_r }, /* IN1 */
	{ 0x6801, 0x6801, input_port_1_r }, /* IN2 */
	{ 0x6802, 0x6802, input_port_2_r }, /* DSW */
	{ 0x6803, 0x6803, cosmicalien_video_address_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress cosmicalien_writemem[] =
{
    { 0x4000, 0x43ff, MWA_RAM },
	{ 0x4400, 0x5bff, cosmic_videoram_w, &cosmic_videoram},
    { 0x5c00, 0x5fff, MWA_RAM },
	{ 0x6000, 0x601f, MWA_RAM ,&spriteram, &spriteram_size },
    { 0x7000, 0x700B, MWA_RAM },   			/* Sound Triggers */
    { 0x700C, 0x700C, cosmicalien_colourmap_select },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( cosmicalien_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin 1 Play" )
	PORT_DIPSETTING(    0x08, "1 Coin 2 Plays" )
	PORT_DIPSETTING(    0x04, "2 Coins 1 Play" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "5000 pts" )
	PORT_DIPSETTING(    0x20, "10000 pts" )
	PORT_DIPSETTING(    0x10, "15000 pts" )
	PORT_DIPSETTING(    0x00, "No Bonus" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	/* The coin slots are not memory mapped. Coin  causes a NMI, */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */

	PORT_START	/* FAKE */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

INPUT_PORTS_END

static struct GfxLayout cosmicalien_spritelayout16 =
{
	16,16,				/* 16*16 sprites */
	64,					/* 64 sprites */
	2,					/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
    { 0,1,2,3,4,5,6,7,16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7},
  	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8				/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout cosmicalien_spritelayout32 =
{
	32,32,				/* 32*32 sprites */
	16,					/* 16 sprites */
	2,					/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0,1,2,3,4,5,6,7,
	  32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
	  64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+4, 64*8+5, 64*8+6, 64*8+7,
      96*8+0, 96*8+1, 96*8+2, 96*8+3, 96*8+4, 96*8+5, 96*8+6, 96*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8, 16*8, 17*8,18*8,19*8,20*8,21*8,22*8,23*8,24*8,25*8,26*8,27*8,28*8,29*8,30*8,31*8 },
	128*8				/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo cosmicalien_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &cosmicalien_spritelayout16,  0,  8 },
	{ 1, 0x0000, &cosmicalien_spritelayout32,  0,  8 },
	{ -1 } /* end of array */
};

/* HSC 12/02/98 */
static int cosmicalienhiload(void)
{
	static int firsttime = 0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (firsttime == 0)
	{
		memset(&RAM[0x400e],0xff,4);	/* high score */
		firsttime = 1;
	}

    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x400e],"\x00\x00\x00",3) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			osd_fread(f,&RAM[0x400e],3);
			osd_fclose(f);
        }

 		firsttime = 0;
 		return 1;
    }
    else return 0;  /* we can't load the hi scores yet */
}

static void cosmicalienhisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
       osd_fwrite(f,&RAM[0x400e],3);
	   osd_fclose(f);
    }
}

static struct MachineDriver cosmicalien_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1081600,
			0,
			cosmicalien_readmem,cosmicalien_writemem,0,0,
			cosmicalien_interrupt,32
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	cosmicalien_gfxdecodeinfo,
	8, 16*4,
	cosmicalien_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	cosmic_vh_start,
	cosmic_vh_stop,
	cosmic_vh_screenrefresh_sprites,

	/* sound hardware */
	0,0,0,0
};

ROM_START( cosmicalien_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "r1",           0x0000, 0x0800, 0x535ee0c5 )
	ROM_LOAD( "r2",           0x0800, 0x0800, 0xed3cf8f7 )
	ROM_LOAD( "r3",           0x1000, 0x0800, 0x6a111e5e )
	ROM_LOAD( "r4",           0x1800, 0x0800, 0xc9b5ca2a )
	ROM_LOAD( "r5",           0x2000, 0x0800, 0x43666d68 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "r6",           0x0000, 0x0800, 0x431e866c )
  	ROM_LOAD( "r7",           0x0800, 0x0800, 0xaa6c6079 )

	ROM_REGION(0x0420)	/* color PROMs */
	ROM_LOAD( "BPR1",         0x0000, 0x0020, 0xdfb60f19 )
	ROM_LOAD( "R9",           0x0020, 0x0400, 0xea4ee931 )
ROM_END

struct GameDriver cosmica_driver =
{
	__FILE__,
	0,
	"cosmica",
	"Cosmic Alien",
	"1980",
	"Universal",
	"Lee Taylor",
	0,
	&cosmicalien_machine_driver,
	0,

	cosmicalien_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	cosmicalien_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	cosmicalienhiload, cosmicalienhisave 	/* hsc 12/02/98 */
};

/*************************************************************************/
/* Cosmic Guerilla specific routines                                     */
/*************************************************************************/
/* 0000-03FF   ROM                          COSMICG.1                    */
/* 0400-07FF   ROM                          COSMICG.2                    */
/* 0800-0BFF   ROM                          COSMICG.3                    */
/* 0C00-0FFF   ROM                          COSMICG.4                    */
/* 1000-13FF   ROM                          COSMICG.5                    */
/* 1400-17FF   ROM                          COSMICG.6                    */
/* 1800-1BFF   ROM                          COSMICG.7                    */
/* 1C00-1FFF   ROM                          COSMICG.8 - Graphics         */
/*                                                                       */
/* 2000-23FF   RAM                                                       */
/* 2400-3FEF   Screen RAM                                                */
/* 3FF0-3FFF   CRT Controller registers (3FF0, register, 3FF4 Data)      */
/*                                                                       */
/* CRTC data                                                             */
/*             ROM                          COSMICG.9 - Color Prom       */
/*                                                                       */
/* CR Bits (Inputs)                                                      */
/* 0000-0003   A9-A13 from CRTC Pixel Clock                              */
/* 0004-000B   Controls                                                  */
/* 000C-000F   Dip Switches                                              */
/*                                                                       */
/* CR Bits (Outputs)                                                     */
/* 0016-0017   Colourmap Selector                                        */
/*************************************************************************/

void cosmicguerilla_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void cosmicguerilla_output_w(int offset, int data);
void cosmicguerilla_colourmap_select(int offset,int data);
int  cosmicguerilla_read_pixel_clock(int offset);

static struct MemoryReadAddress cosmicguerilla_readmem[] =
{
	{ 0x2000, 0x3fff, MRA_RAM},
	{ 0x0000, 0x1fff, MRA_ROM},
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress cosmicguerilla_writemem[] =
{
  	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3bff, cosmic_videoram_w, &cosmic_videoram },
  	{ 0x3C00, 0x3fff, MWA_RAM },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static const char *cosmicguerilla_sample_names[] =
{
	"*cosmicg",
	"CG_M0.SAM",	/* 8 Different pitches of March Sound */
	"CG_M1.SAM",
	"CG_M2.SAM",
	"CG_M3.SAM",
	"CG_M4.SAM",
	"CG_M5.SAM",
	"CG_M6.SAM",
	"CG_M7.SAM",
	"CG_ATT.SAM",	/* Killer Attack */
	"CG_CHNC.SAM",	/* Bonus Chance  */
	"CG_EXT.SAM",	/* Got Bonus - have not got sound for, so use extend */
	"CG_DEST.SAM",	/* Gun Destroy */
	"CG_GUN.SAM",	/* Gun Shot */
	"CG_GOTM.SAM",	/* Got Monster */
	"CG_EXT.SAM",	/* Coin Extend */
	0       /* end of array */
};

static struct IOReadPort cosmicguerilla_readport[] =
{
	{ 0x00, 0x00, cosmicguerilla_read_pixel_clock },
	{ 0x01, 0x01, input_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort cosmicguerilla_writeport[] =
{
	{ 0x00, 0x15, cosmicguerilla_output_w },
    { 0x16, 0x17, cosmicguerilla_colourmap_select },
	{ -1 }	/* end of table */
};

void cosmicguerilla_output_w(int offset, int data)
{
	static int MarchSelect;
    static int GunDieSelect;
    static int SoundEnable;

    int Count;

    /* Sound Enable / Disable */

    if (offset == 12)
    {
    	SoundEnable = data;
    	if (data == 0)
        	for(Count=0;Count<9;Count++) sample_stop(Count);
    }

    if (SoundEnable)
    {
        switch (offset)
        {
        	/* The schematics show a direct link to the sound amp  */
            /* as other cosmic series games, but it never seems to */
            /* be used for anything. It is implemented for sake of */
            /* completness. Maybe it plays a tune if you win ?     */

            case 1 : DAC_data_w(0, -data);
        		     break;


            /* March Sound */

    	    case 2 : if (errorlog) fprintf(errorlog,"March = %d\n",MarchSelect);
				     if (data) sample_start (0, MarchSelect, 0);
        		     break;

            case 3 : MarchSelect = (MarchSelect & 0xFE) | data;
                     break;

            case 4 : MarchSelect = (MarchSelect & 0xFD) | (data << 1);
                     break;

            case 5 : MarchSelect = (MarchSelect & 0xFB) | (data << 2);
                     break;


            /* Killer Attack (crawly thing at bottom of screen) */

            case 6 : if (data)
        			     sample_start(1, 8, 1);
                     else
                 	     sample_stop(1);
                     break;


            /* Bonus Chance & Got Bonus */

            case 7 : if (data)
        		     {
        			     sample_stop(4);
        			     sample_start(4, 10, 0);
                     }
                     break;

            case 8 : if (data)
		   		     {
					    if (!sample_playing(4)) sample_start(4, 9, 1);
                     }
        		     else
                 	    sample_stop(4);
                     break;


            /* Got Ship */

            case 9 : if (data) sample_start(3, 11, 0);
                     break;



            case 11: /* Watchdog */
        		     break;


            /* Got Monster / Gunshot */

            case 13: if (data) sample_start(8, 13-GunDieSelect, 0);
                     break;

            case 14: GunDieSelect = data;


            /* Coin Extend (extra base) */

            case 15: if (data) sample_start(5, 14, 0);
                     break;

        }
    }

 	if((errorlog) && (offset != 11))
		fprintf(errorlog,"Output %x=%x\n",offset,data);
}

int cosmicguerilla_read_pixel_clock(int offset)
{
	/* The top four address lines from the CRTC are bits 0-3 */

	return (input_port_0_r(0) & 0xf0) | PixelClock;
}

int cosmicguerilla_interrupt(void)
{
	/* Increment Pixel Clock */

    PixelClock = (PixelClock + 1) & 15;

    /* Insert Coin */

	if ((readinputport(2) & 1) & (PixelClock == 0))	/* Coin */
    {
		return 4;
    }
	else
    {
      	return ignore_interrupt();
    }
}

static void cosmicguerilla_decode(void)
{
	/* Roms have data pins connected different from normal */

	int Count;
	unsigned char Scrambled,Normal;

    for(Count=0x1fff;Count>=0;Count--)
	{
        Scrambled = Machine->memory_region[0][Count];

        Normal = (Scrambled >> 3 & 0x11)
               | (Scrambled >> 1 & 0x22)
               | (Scrambled << 1 & 0x44)
               | (Scrambled << 3 & 0x88);

        Machine->memory_region[0][Count] = Normal;
    }

    /* Patch to avoid crash - Seems like duff romcheck routine */
    /* I would expect it to be bitrot, but have two romsets    */
    /* from different sources with the same problem!           */

    Machine->memory_region[0][0x1e9e] = 0x04;
    Machine->memory_region[0][0x1e9f] = 0xc0;
}

/* These are used for the CR handling - This can be used to */
/* from 1 to 16 bits from any bit offset between 0 and 4096 */

/* Offsets are in BYTES, so bits 0-7 are at offset 0 etc.   */

INPUT_PORTS_START( cosmicguerilla_input_ports )

	PORT_START /* 4-7 */
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

	PORT_START /* 8-15 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL)
	PORT_DIPNAME( 0x30, 0x30, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2000 pts" )
	PORT_DIPSETTING(    0x20, "1500 pts" )
	PORT_DIPSETTING(    0x10, "1000 pts" )
	PORT_DIPSETTING(    0x00, "No Bonus" )
	PORT_DIPNAME( 0x40, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin 1 Play" )
	PORT_DIPSETTING(    0x40, "2 Coins 1 Play" )
	PORT_DIPNAME( 0x80, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x80, "5" )

	PORT_START      /* Hard wired settings */

	/* The coin slots are not memory mapped. Coin causes INT 4  */
	/* This fake input port is used by the interrupt handler 	*/
	/* to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check   */
	/* when the user releases the key. 							*/

	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

    /* This dip switch is not read by the program at any time   */
    /* but is wired to enable or disable the flip screen output */

	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )

    /* This odd setting is marked as shown on the schematic,    */
    /* and again, is not read by the program, but wired into    */
    /* the watchdog circuit. The book says to leave it off      */

    PORT_DIPNAME( 0x04, 0x00, "Not used by Game", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Default" )
    PORT_DIPSETTING(    0x04, "Unknown" )

INPUT_PORTS_END

static int cosmicguerillahiload(void)
{
	static int firsttime = 0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */

	if (firsttime == 0)
	{
		memset(&RAM[0x3c10],0xff,4);	/* high score */
		firsttime = 1;
	}

    /* check if the hi score table has already been initialized */

    if (memcmp(&RAM[0x3c10],"\x00\x00\x00\x00",4) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			osd_fread(f,&RAM[0x3c10],4);
			osd_fclose(f);
        }

 		firsttime = 0;
 		return 1;
    }
    else return 0;  /* we can't load the hi scores yet */
}

static void cosmicguerillahisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
       osd_fwrite(f,&RAM[0x3c10],4);
	   osd_fclose(f);
    }
}

static struct MachineDriver cosmicguerilla_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			1228500,			/* 9.828 Mhz Crystal */
			0,
			cosmicguerilla_readmem,cosmicguerilla_writemem,
			cosmicguerilla_readport,cosmicguerilla_writeport,
			cosmicguerilla_interrupt,16
		}
	},
	60, 0,		/* frames per second, vblank duration */
	1,			/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	0, 			/* no gfxdecodeinfo - bitmapped display */
    16,8*4,
    cosmicguerilla_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	cosmic_vh_start,
	cosmic_vh_stop,
	cosmic_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

ROM_START( cosmicguerilla_rom )
	ROM_REGION(0x10000)  /* 8k for code */
	ROM_LOAD( "cosmicg1.bin",  0x0000, 0x0400, 0xe1b9f894 )
	ROM_LOAD( "cosmicg2.bin",  0x0400, 0x0400, 0x35c75346 )
	ROM_LOAD( "cosmicg3.bin",  0x0800, 0x0400, 0x82a49b48 )
	ROM_LOAD( "cosmicg4.bin",  0x0C00, 0x0400, 0x1c1c934c )
	ROM_LOAD( "cosmicg5.bin",  0x1000, 0x0400, 0xb1c00fbf )
	ROM_LOAD( "cosmicg6.bin",  0x1400, 0x0400, 0xf03454ce )
	ROM_LOAD( "cosmicg7.bin",  0x1800, 0x0400, 0xf33ebae7 )
	ROM_LOAD( "cosmicg8.bin",  0x1C00, 0x0400, 0x472e4990 )

    ROM_REGION(0x0400)	/* Colour Prom */
	ROM_LOAD( "cosmicg9.bin",  0x0000, 0x0400, 0x689c2c96 )
ROM_END

struct GameDriver cosmicg_driver =
{
	__FILE__,
	0,
	"cosmicg",
	"Cosmic Guerilla",
	"1979",
	"Universal",
	"Andy Jones\nMike Coates",
	0,
    &cosmicguerilla_machine_driver,
    0,

	cosmicguerilla_rom,
	cosmicguerilla_decode, 0,
    cosmicguerilla_sample_names,
	0,      /* sound_prom */

	cosmicguerilla_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	cosmicguerillahiload, cosmicguerillahisave
};

/***************************************************************************

Magical Spot 2 memory map (preliminary)

0000-2fff ROM
6000-63ff RAM
6400-7fff Video RAM

read:

write:

***************************************************************************/

void magspot2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void magspot2_colourmap_w(int offset, int data);

static int magspot2_interrupt(void)
{
	/* Coin 1 causes an IRQ, Coin 2 an NMI */
	if (input_port_4_r(0) & 0x01)
	{
  		return interrupt();
	}

	if (input_port_4_r(0) & 0x02)
	{
		return nmi_interrupt();
	}

	return ignore_interrupt();
}


static int magspot2_coinage_dip_r(int offset)
{
	return (input_port_5_r(0) & (1 << (7 - offset))) ? 0 : 1;
}


static struct MemoryReadAddress magspot2_readmem[] =
{
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x3800, 0x3807, magspot2_coinage_dip_r },
	{ 0x5000, 0x5000, input_port_0_r },
	{ 0x5001, 0x5001, input_port_1_r },
	{ 0x5002, 0x5002, input_port_2_r },
	{ 0x5003, 0x5003, input_port_3_r },
	{ 0x6000, 0x7fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress magspot2_writemem[] =
{
	{ 0x4000, 0x401f, MWA_RAM, &spriteram, &spriteram_size},
	{ 0x4800, 0x4800, DAC_data_w },
    { 0x480D, 0x480D, magspot2_colourmap_w },
    { 0x480F, 0x480F, cosmic_flipscreen_w },
	{ 0x6000, 0x63ff, MWA_RAM },
	{ 0x6400, 0x7bff, cosmic_videoram_w, &cosmic_videoram, &videoram_size},
    { 0x7c00, 0x7fff, MWA_RAM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( magspot2_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_DIPNAME( 0xc0, 0x40, "Bonus Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "5000" )
	PORT_DIPSETTING(    0x80, "10000" )
	PORT_DIPSETTING(    0xc0, "15000" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x01, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2000" )
	PORT_DIPSETTING(    0x02, "3000" )
	PORT_DIPSETTING(    0x03, "5000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x18, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
  	PORT_BIT( 0x3e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* Fake port to handle coins */
	PORT_START	/* IN4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)

	/* Fake port to handle coinage dip switches. Each bit goes to 3800-3807 */
	PORT_START	/* IN5 */
	PORT_DIPNAME( 0x0f, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0d, "4 Coins/2 Credits" )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x09, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0e, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "4 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0a, "3 Coins/3 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0b, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0xf0, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "4 Coins/2 Credits" )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x90, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xe0, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "4 Coins/4 Credits" )
	PORT_DIPSETTING(    0xa0, "3 Coins/3 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xb0, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
INPUT_PORTS_END

static struct MachineDriver magspot2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz ???? */
			0,
			magspot2_readmem,magspot2_writemem,0,0,
			magspot2_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
  	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	cosmicalien_gfxdecodeinfo,
	16, 16*4,
	magspot2_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	cosmic_vh_start,
	cosmic_vh_stop,
	cosmic_vh_screenrefresh_sprites,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

ROM_START( magspot2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "my1",  0x0000, 0x0800, 0xc0085ade )
	ROM_LOAD( "my2",  0x0800, 0x0800, 0xd534a68b )
	ROM_LOAD( "my3",  0x1000, 0x0800, 0x25513b2a )
	ROM_LOAD( "my5",  0x1800, 0x0800, 0x8836bbc4 )
	ROM_LOAD( "my4",  0x2000, 0x0800, 0x6a08ab94 )
	ROM_LOAD( "my6",  0x2800, 0x0800, 0x77c6d109 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "my7",  0x0000, 0x0800, 0x1ab338d3 )
	ROM_LOAD( "my8",  0x0800, 0x0800, 0x9e1d63a2 )

	ROM_REGION(0x0420)	/* color proms */
	ROM_LOAD( "m13",  0x0000, 0x0020, 0x36e2aa2a )
	ROM_LOAD( "my9",  0x0020, 0x0400, 0x89f23ebd )
ROM_END


struct GameDriver magspot2_driver =
{
	__FILE__,
	0,
	"magspot2",
	"Magical Spot II",
	"1980",
	"Universal",
	"Zsolt Vasvari",
	0,
	&magspot2_machine_driver,
	0,

	magspot2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	magspot2_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

/***************************************************************************

  Devil Zone

***************************************************************************/

void devzone_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

static struct MemoryReadAddress devzone_readmem[] =
{
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x5000, 0x5000, input_port_0_r },
	{ 0x5001, 0x5001, input_port_1_r },
	{ 0x5002, 0x5002, input_port_2_r },
	{ 0x5003, 0x5003, input_port_3_r },
    { 0x6000, 0x6001, MRA_RAM },
	{ 0x6002, 0x6002, input_port_5_r },
	{ 0x6003, 0x7fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress devzone_writemem[] =
{
	{ 0x4000, 0x401f, MWA_RAM, &spriteram, &spriteram_size},
	{ 0x4800, 0x4800, DAC_data_w },
    { 0x480D, 0x480D, magspot2_colourmap_w },
    { 0x480F, 0x480F, cosmic_flipscreen_w },
	{ 0x6000, 0x63ff, MWA_RAM },
	{ 0x6400, 0x7bff, cosmic_videoram_w, &cosmic_videoram, &videoram_size},
    { 0x7c00, 0x7fff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct MachineDriver devzone_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz ???? */
			0,
			devzone_readmem,devzone_writemem,0,0,
			magspot2_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
  	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	cosmicalien_gfxdecodeinfo,
	16, 16*4,
	devzone_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	cosmic_vh_start,
	cosmic_vh_stop,
	cosmic_vh_screenrefresh_sprites,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

INPUT_PORTS_START( devzone_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x01, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "6000" )
	PORT_DIPSETTING(    0x03, "8000" )

    PORT_DIPNAME( 0x0C, 0x0C, "Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "Use Coin A & B" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )

	PORT_DIPNAME( 0x10, 0x10, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )

	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )

	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
  	PORT_BIT( 0x3e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* Fake port to handle coins */
	PORT_START	/* IN4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)

	PORT_START	/* IN5 */
	PORT_DIPNAME( 0x0f, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0d, "4 Coins/2 Credits" )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x09, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0e, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "4 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0a, "3 Coins/3 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0b, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0xf0, 0x10, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "4 Coins/2 Credits" )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x90, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xe0, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "4 Coins/4 Credits" )
	PORT_DIPSETTING(    0xa0, "3 Coins/3 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xb0, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
INPUT_PORTS_END

ROM_START( devzone_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "dv1.e3",  0x0000, 0x0800, 0xc70faf00 )
	ROM_LOAD( "dv2.e4",  0x0800, 0x0800, 0xeacfed61 )
	ROM_LOAD( "dv3.e5",  0x1000, 0x0800, 0x7973317e )
	ROM_LOAD( "dv5.e7",  0x1800, 0x0800, 0xb71a3989 )
	ROM_LOAD( "dv4.e6",  0x2000, 0x0800, 0xa58c5b8c )
	ROM_LOAD( "dv6.e8",  0x2800, 0x0800, 0x3930fb67 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dv7.n1",  0x0000, 0x0800, 0xe7562fcf )
	ROM_LOAD( "dv8.n2",  0x0800, 0x0800, 0xda1cbec1 )

	ROM_REGION(0x0400)	/* color proms */
	ROM_LOAD( "dz9.e2",  0x0000, 0x0400, 0x693855b6 )
ROM_END

struct GameDriver devzone_driver =
{
	__FILE__,
	0,
	"devzone",
	"Devil Zone",
	"1980",
	"Universal",
	"Zsolt Vasvari\nMike Coates",
	GAME_WRONG_COLORS,
	&devzone_machine_driver,
	0,

	devzone_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	devzone_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
