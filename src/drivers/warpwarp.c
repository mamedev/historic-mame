/***************************************************************************

Warp Warp memory map (preliminary)

  Memory Map figured out by Chris Hardy (chrish@kcbbs.gen.nz)
  Initial Driver code by Mirko


0000-37FF ROM		Code
4800-4FFF ROM		Graphics rom which must be included in memory space

memory mapped ports:

read:

All Read ports only use bit 0

C000      Coin slot 2
C001      ???
C002      Start P1
C003      Start P2
C004      Fire
C005      Test Mode
C006      ???
C007      Coin Slot 2
C020-C027 Dipswitch 1->8 in bit 0

C010      Analog Joystick
			 0->62 = DOWN
			 63->110 = LEFT
			 111->166 = RIGHT
			 167->255 = UP

write:

C003			WatchDog reset (ignore)
C036			re-enable interrupts (ignore)
C010			Sound Port (??)
C020			Sound Port 2 (??)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *warpwarp_bulletsram;
void warpwarp_vh_screenrefresh(struct osd_bitmap *bitmap);

int warpwarp_input_c000_7_r(int offset);
int warpwarp_input_c020_27_r(int offset);
int warpwarp_input_controller_r(int offset);
int warpwarp_interrupt(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xc000, 0xc007, warpwarp_input_c000_7_r },
	{ 0xc010, 0xc010, warpwarp_input_controller_r },
	{ 0xc020, 0xc027, warpwarp_input_c020_27_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4800, 0x4fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xC000, 0xC001, MWA_RAM, &warpwarp_bulletsram },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Freeplay" )
	PORT_DIPNAME( 0x0c, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
/* The bonus setting changes depending on the number of lives */
	PORT_DIPNAME( 0x30, 0x10, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Setting 1" )
	PORT_DIPSETTING(    0x10, "Setting 2" )
	PORT_DIPSETTING(    0x20, "Setting 3" )
	PORT_DIPSETTING(    0x30, "Setting 4" )
	PORT_DIPNAME( 0x40, 0x40, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE ) /* Probably unused */
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* FAKE - used by input_controller_r to simulate an analog stick */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 2 bits per pixel */
	{ 0 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 }, /* characters are rotated 90 degrees */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* bits are packed in groups of four */
	8*8	/* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	1,	/* 1 bits per pixel */
	{ 0 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 23 * 8, 22 * 8, 21 * 8, 20 * 8, 19 * 8, 18 * 8, 17 * 8, 16 * 8 ,
           7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{  0, 1, 2, 3, 4, 5, 6, 7 ,
         8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x4800, &charlayout,   0, 16 },
	{ 0, 0x4800, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0, 0, 0,
	0, 165, 255,
	231, 231, 0,
	165, 0, 0,
	181, 181, 181,
	239, 0, 239,
	247, 0, 156,
	255, 0, 0,
	255, 132, 0,
	255, 181, 115,
	255, 255, 255,
	255, 0, 255,
	0, 255, 255,
	0, 0, 255,
	255, 0, 0
};

static unsigned char colortable[] =
{
	0,0,
	0,1,
	0,2,
	0,3,
	0,4,
	0,5,
	0,6,
	0,7,
	0,8,
	0,9,
	0,9,
	0,10,
	0,11,
	0,12,
	0,13,
	0,14,
	0,15
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2048000,	/* 3 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	31*8, 34*8, { 0*8, 31*8-1, 0*8, 34*8-1 },
	gfxdecodeinfo,

	sizeof(palette)/3 ,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	warpwarp_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};



ROM_START( warpwarp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "warp_2r.bin",  0x0000, 0x1000, 0x9a51d92b )
	ROM_LOAD( "warp_2m.bin",  0x1000, 0x1000, 0xdeb96ecf )
	ROM_LOAD( "warp_1p.bin",  0x2000, 0x1000, 0x30ad0f77 )
	ROM_LOAD( "warp_1t.bin",  0x3000, 0x0800, 0x3426c0b0 )
	ROM_LOAD( "warp_s12.bin", 0x4800, 0x0800, 0xb0468bf8 )
ROM_END



static int hiload(void)
{

        void *f;

        if (memcmp(&RAM[0x8358],"\x00\x30\x00",3) == 0 &&
                memcmp(&RAM[0x8373],"\x18\x18\x18",3) == 0)

        {
                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0x8358],3*10);
                        RAM[0x831d] = RAM[0x8364];
                        RAM[0x831e] = RAM[0x8365];
                        RAM[0x831f] = RAM[0x8366];
                        osd_fclose(f);
                }
                return 1;
        }
        else
                return 0;
}

static void hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x8358],3*10);
		osd_fclose(f);
	}
}



struct GameDriver warpwarp_driver =
{
	"Warp Warp",
	"warpwarp",
	"Chris Hardy (MAME driver)\nJuan Carlos Lorente (high score)\nMarco Cassili",
	&machine_driver,

	warpwarp_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

