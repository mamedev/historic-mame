/***************************************************************************

Elevator Action memory map (preliminary)

0000-7fff ROM
8000-87ff RAM
c400-c7ff Video RAM 1
c800-cbff Video RAM 2
cc00-cfff Video RAM 3
d100-d17f Sprites

read:
8800      ?
8801      ? the code stops until bit 0 and 1 are = 1

write
d50d      ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int elevator_init_machine(const char *gamename);

extern unsigned char *elevator_videoram2,*elevator_videoram3;
extern unsigned char *elevator_scroll1,*elevator_scroll2;
extern unsigned char *elevator_attributesram;
extern void elevator_videoram2_w(int offset,int data);
extern void elevator_videoram3_w(int offset,int data);
extern int elevator_vh_start(void);
extern void elevator_vh_stop(void);
extern void elevator_vh_screenrefresh(struct osd_bitmap *bitmap);
//extern void elevator_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc400, 0xcfff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd408, 0xd408, input_port_0_r },	/* IN0 */
	{ 0xd409, 0xd409, input_port_1_r },	/* IN1 */
	{ 0xd40b, 0xd40b, input_port_2_r },	/* IN2 */
	{ 0xd40c, 0xd40c, input_port_3_r },	/* COIN */
	{ 0xd40a, 0xd40a, input_port_4_r },	/* DSW1 */
//	{ 0xd40f, 0xd40f, input_port_5_r },	/* DSW3 */
	{ 0x8801, 0x8801, input_port_5_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xc400, 0xc7ff, videoram_w, &videoram },
	{ 0xc800, 0xcbff, elevator_videoram2_w, &elevator_videoram2 },
	{ 0xcc00, 0xcfff, elevator_videoram3_w, &elevator_videoram3 },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram },
	{ 0xd502, 0xd502, MWA_RAM, &elevator_scroll2 },
	{ 0xd504, 0xd504, MWA_RAM, &elevator_scroll1 },
{ 0xd50d, 0xd50d, MWA_RAM },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, 0 , 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE, 0 , 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* COIN */
		0xff,
		{ 0, 0, 0, 0, OSD_KEY_3, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x7f,
		{ 0, 0, 0, 0, 0, OSD_KEY_F2, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW3 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 4, 0x18, "LIVES", { "6", "5", "4", "3" }, 1 },
	{ 4, 0x03, "BONUS", { "25000", "20000", "15000", "10000" }, 1 },
	{ 5, 0x03, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 4, 0x04, "FREE PLAY", { "ON", "OFF" }, 1 },
	{ 5, 0x40, "DEMO MODE", { "ON", "OFF" }, 1 },
	{ 5, 0x10, "COIN DISPLAY", { "NO", "YES" }, 1 },
	{ 5, 0x20, "YEAR DISPLAY", { "NO", "YES" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 0, 256*8*8, 512*8*8 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 64*16*16, 128*16*16 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0,
		8*8+7, 8*8+6, 8*8+5, 8*8+4, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x3000, &charlayout,    0, 32 },
	{ 1, 0x0000, &charlayout,    0, 32 },
	{ 1, 0x1800, &spritelayout,  0, 32 },
	{ 1, 0x4800, &spritelayout,  0, 32 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */

  0xff,0x00,0x00, /* RED */
  0x00,0xff,0x00, /* GREEN */
  0x00,0x00,0xff, /* BLUE */
  0xff,0xff,0x00, /* YELLOW */
  0xff,0x00,0xff, /* MAGENTA */
  0x00,0xff,0xff, /* CYAN */
  0xff,0xff,0xff, /* WHITE */
  0xE0,0xE0,0xE0, /* LTGRAY */
  0xC0,0xC0,0xC0, /* DKGRAY */

	0xe0,0xb0,0x70,	/* BROWN */
	0xd0,0xa0,0x60,	/* BROWN0 */
	0xc0,0x90,0x50,	/* BROWN1 */
	0xa3,0x78,0x3a,	/* BROWN2 */
	0x80,0x60,0x20,	/* BROWN3 */
	0x54,0x40,0x14,	/* BROWN4 */

  0x54,0xa8,0xff, /* LTBLUE */
  0x00,0xa0,0x00, /* DKGREEN */
  0x00,0xe0,0x00, /* GRASSGREEN */



	0xff,0xb6,0xdb,	/* PINK */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum {BLACK,RED,GREEN,BLUE,YELLOW,MAGENTA,CYAN,WHITE,LTGRAY,DKGRAY,
       BROWN,BROWN0,BROWN1,BROWN2,BROWN3,BROWN4,
			 LTBLUE,DKGREEN,GRASSGREEN,
            };

static unsigned char colortable[] =
{
        0,1,2,14,4,13,6,7,
        0,2,14,4,13,6,7,8,
        0,14,4,13,6,7,8,7,
        0,4,13,6,7,8,17,9,
        0,13,6,7,8,7,9,2,
        0,6,7,8,7,9,2,5,
        0,7,8,7,9,2,5,13,
        0,8,7,9,2,5,13,8,
        0,7,9,2,5,13,8,13,
        0,9,2,5,13,8,13,7,
        0,2,5,13,8,13,7,2,
        0,5,13,8,13,7,2,14,
        0,13,8,13,7,2,14,4,
        0,8,13,7,2,14,4,5,
        0,13,7,2,14,4,5,6,
        0,13,7,2,14,4,5,6,
        0,1,2,14,4,13,6,7,
        0,2,14,4,13,6,7,8,
        0,14,4,13,6,7,8,7,
        0,4,13,6,7,8,17,9,
        0,13,6,7,8,7,9,2,
        0,6,7,8,7,9,2,5,
        0,7,8,7,9,2,5,13,
        0,8,7,9,2,5,13,8,
        0,7,9,2,5,13,8,13,
        0,9,2,5,13,8,13,7,
        0,2,5,13,8,13,7,2,
        0,5,13,8,13,7,2,14,
        0,13,8,13,7,2,14,4,
        0,8,13,7,2,14,4,5,
        0,13,7,2,14,4,5,6,
        0,13,7,2,14,4,5,6,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
	},
	60,
	elevator_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	elevator_vh_start,
	elevator_vh_stop,
	elevator_vh_screenrefresh,

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

ROM_START( elevator_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ea-ic69.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea-ic68.bin", 0x1000, 0x1000 )
	ROM_LOAD( "ea-ic67.bin", 0x2000, 0x1000 )
	ROM_LOAD( "ea-ic66.bin", 0x3000, 0x1000 )
	ROM_LOAD( "ea-ic65.bin", 0x4000, 0x1000 )
	ROM_LOAD( "ea-ic64.bin", 0x5000, 0x1000 )
	ROM_LOAD( "ea-ic55.bin", 0x6000, 0x1000 )
	ROM_LOAD( "ea-ic54.bin", 0x7000, 0x1000 )

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ea-ic1.bin",  0x0000, 0x1000 )
	ROM_LOAD( "ea-ic2.bin",  0x1000, 0x1000 )
	ROM_LOAD( "ea-ic3.bin",  0x2000, 0x1000 )
	ROM_LOAD( "ea-ic4.bin",  0x3000, 0x1000 )
	ROM_LOAD( "ea-ic5.bin",  0x4000, 0x1000 )
	ROM_LOAD( "ea-ic6.bin",  0x5000, 0x1000 )
#if 0
	ROM_LOAD( "ea-ic7.bin",  0x6000, 0x1000 )
	ROM_LOAD( "ea-ic8.bin",  0x7000, 0x1000 )
	ROM_LOAD( "ea-ic70.bin", , 0x1000 )
	ROM_LOAD( "ea-ic71.bin", , 0x1000 )
	ROM_LOAD( "ee_ea10.bin", , 0x1000 )
#endif
ROM_END



struct GameDriver elevator_driver =
{
	"elevator",
	&machine_driver,

	elevator_rom,
	0, 0,
	0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,	/* numbers */
		0x1c,0x29,0x2e,0x30,0x1e,0x27,0x28,0x2b,0x2c,0x2c,0x21,0x1b,0x20,	/* letters */
		0x25,0x2f,0x1a,0x2f,0x1f,0x2d,0x31,0x22,0x23,0x26,0x21,0x1d,0x12 },	/* j, k, q and z are missing */
	0x06, 0x04,
	8*13, 8*16, 0x00,

	0, 0
};
