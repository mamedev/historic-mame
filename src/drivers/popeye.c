/***************************************************************************

Popeye memory map (preliminary)

0000-7fff  ROM

8000-87ff  RAM
8c00-8dff  sprites
8f00-8fff  RAM (stack)

A000-A3FF  Text video ram
A400-A7FF  Text Attribute

C000-CFFF  Seem to be a Bitmap of 64 X 64


I/O 0  ;AY-3-8910 Control Reg.
I/O 1  ;AY-3-8910 Data Write Reg.
        write to port B: select bit of DSW2 to read in bit 7 of port A (0-2-4-6-8-a-c-e)
I/O 3  ;AY-3-8910 Data Read Reg.
        read from port A: bit 0-5 = DSW1  bit 7 = bit of DSW1 selected by port B

        DSW1
		bit 0-3 = coins per play (0 = free play)
		bit 4-5 = ?

		DSW2
        bit 0-1 = lives
		bit 2-3 = difficulty
		bit 4-5 = bonus
		bit 6 = demo sounds
		bit 7 = cocktail/upright (0 = upright)

I/O 2  ;bit 0 Coin in 1
        bit 1 Coin in 2
        bit 2 Coin in 3 = 5 credits
        bit 3
        bit 4 Start 2 player game
        bit 5 Start 1 player game
        bit 6
        bit 7

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *popeye_videoram,*popeye_colorram;
extern void popeye_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int popeye_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8c00, 0x8eaf, MRA_RAM },
	{ 0x8f00, 0x8fff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8c00, 0x8dff, MWA_RAM, &spriteram },
	{ 0x8f00, 0x8fff, MWA_RAM },
	{ 0xa000, 0xa3ff, MWA_RAM, &popeye_videoram },
	{ 0xa400, 0xa7ff, MWA_RAM, &popeye_colorram },
	{ 0xc000, 0xcfff, videoram_w, &videoram },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};





static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_JOY_FIRE,
				OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ OSD_KEY_8, OSD_KEY_9, OSD_KEY_1,OSD_KEY_2,
      OSD_KEY_0, OSD_KEY_5, OSD_KEY_4,OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x3d,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 4, 0x03, "LIVES", { "4", "3", "2", "1" }, 1 },
	{ 4, 0x30, "BONUS", { "NONE", "80000", "60000", "40000" }, 1 },
	{ 4, 0x0c, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 4, 0x40, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{ 3, 0x10, "SW1 5", { "ON", "OFF" }, 1 },
	{ 3, 0x20, "SW1 6", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	16,16,	/* 16*16 characters (8*8 doubled) */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7,7, 6,6, 5,5, 4,4, 3,3, 2,2, 1,1, 0,0 },	/* pretty straightforward layout */
	{ 0*8,0*8, 1*8,1*8, 2*8,2*8, 3*8,3*8, 4*8,4*8, 5*8,5*8, 6*8,6*8, 7*8,7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 0x4000*8 },	/* the two bitplanes are separated in different files */
	{7+(0x2000*8),6+(0x2000*8),5+(0x2000*8),4+(0x2000*8),
     3+(0x2000*8),2+(0x2000*8),1+(0x2000*8),0+(0x2000*8),
   7,6,5,4,3,2,1,0 },
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
    7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8, },
	16*8	/* every sprite takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 8 },	/* chars */
	{ 1, 0x1000, &spritelayout,  0, 8 },	/* sprites */
	{ 1, 0x2000, &spritelayout,  0, 8 },	/* sprites */
	{ 1, 0x0000, &tilelayout,    0, 8 },	/* tiles */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black      */
	0x94,0x00,0xd8,   /* darkpurple */
	0xd8,0x00,0x00,   /* darkred    */
	0xf8,0x64,0xd8,   /* pink       */
	0x00,0xd8,0x00,   /* darkgreen  */
	0x00,0xf8,0xd8,   /* darkcyan   */
	0xd8,0xd8,0x94,   /* darkyellow */
	0xd8,0xf8,0xd8,   /* darkwhite  */
	0xf8,0x94,0x44,   /* orange     */
	0x00,0x00,0xd8,   /* blue   */
	0xf8,0x00,0x00,   /* red    */
	0xff,0x00,0xff,   /* purple */
	0x00,0xf8,0x00,   /* green  */
	0x00,0xff,0xff,   /* cyan   */
	0xf8,0xf8,0x00,   /* yellow */
	0xff,0xff,0xff    /* white  */
};

enum
{
	black, darkpurple, darkred, pink, darkgreen, darkcyan, darkyellow,
		darkwhite, orange, blue, red, purple, green, cyan, yellow, white
};

static unsigned char colortable[] =
{
	black, darkred,   blue,       darkyellow,
	black, green,     darkpurple, orange,
	black, darkgreen, darkred,    yellow,
	black, darkred,   darkgreen,  yellow,
	black, yellow,    darkgreen,  red,
	black, green,     orange,     yellow,
	black, darkwhite, red,        pink,
	black, darkcyan,  red,        darkwhite
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz ? */
			0,
			readmem,writemem,readport,writeport,
			nmi_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*16, 30*16, { 0*16, 32*16-1, 0*16, 30*16-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	popeye_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	popeye_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( popeyebl_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "po1", 0x0000, 0x2000 )
	ROM_LOAD( "po2", 0x2000, 0x2000 )
	ROM_LOAD( "po3", 0x4000, 0x2000 )
	ROM_LOAD( "po4", 0x6000, 0x2000 )

	ROM_REGION(0x9000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "po5", 0x0000, 0x1000 )
	ROM_LOAD( "po6", 0x1000, 0x2000 )
	ROM_LOAD( "po7", 0x3000, 0x2000 )
	ROM_LOAD( "po8", 0x5000, 0x2000 )
	ROM_LOAD( "po9", 0x7000, 0x2000 )
ROM_END



struct GameDriver popeyebl_driver =
{
	"popeyebl",
	&machine_driver,

	popeyebl_rom,
	0, 0,
	0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x02,
	16*13, 16*16, 0x0c,

	0,0
};
