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
d404      returns contents of background ROM, pointed by d509-d50a
d408      IN0
          bit 5 = jump player 1
          bit 4 = fire player 1
          bit 3 = up player 1
          bit 2 = down player 1
          bit 1 = right player 1
          bit 0 = left player 1
d409      IN1
          bit 5 = jump player 2 (COCKTAIL only)
          bit 4 = fire player 2 (COCKTAIL only)
          bit 3 = up player 2 (COCKTAIL only)
          bit 2 = down player 2 (COCKTAIL only)
          bit 1 = right player 2 (COCKTAIL only)
          bit 0 = left player 2 (COCKTAIL only)
d40a      DSW1
          bit 7   = cocktail / upright (0 = upright)
          bit 6   = flip screen
          bit 5   = ?
          bit 3-4 = lives
		  bit 2   = free play
          bit 0-1 = bonus
d40b      IN2
          bit 7 = start 2
          bit 6 = start 1
d40c      COIN
          bit 5 = tilt
          bit 4 = coin
d40f      DSW3
          bit 7 = coinage (1 way/2 ways)
          bit 6 = no hit
          bit 5 = year display yes/no
          bit 4 = coin display yes/no
		  bit 2-3 ?
		  bit 0-1 difficulty

write
d020-d03f playfield #2 column scroll
d040-d05f playfield #1 column scroll
d509-d50a pointer to background ROM to read from d404
d50d      watchdog reset ?
d50e      bootleg version: $01 -> ROM ea54.bin is mapped at 7000-7fff
                           $81 -> ROM ea52.bin is mapped at 7000-7fff

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int elevator_init_machine(const char *gamename);
extern int elevator_protection_r(int offset);
extern int elevator_unknown_r(int offset);
extern void elevatob_bankswitch_w(int offset,int data);

extern unsigned char *elevator_videoram2,*elevator_videoram3;
extern unsigned char *elevator_scroll1,*elevator_scroll2;
extern unsigned char *elevator_attributesram;
extern int elevator_background_r(int offset);
extern void elevator_videoram2_w(int offset,int data);
extern void elevator_videoram3_w(int offset,int data);
extern int elevator_vh_start(void);
extern void elevator_vh_stop(void);
extern void elevator_vh_screenrefresh(struct osd_bitmap *bitmap);



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
	{ 0xd40f, 0xd40f, input_port_5_r },	/* DSW3 */
	{ 0xd404, 0xd404, elevator_background_r },
	{ 0x8801, 0x8801, elevator_unknown_r },
	{ 0x8800, 0x8800, elevator_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xc400, 0xc7ff, videoram_w, &videoram },
	{ 0xc800, 0xcbff, elevator_videoram2_w, &elevator_videoram2 },
	{ 0xcc00, 0xcfff, elevator_videoram3_w, &elevator_videoram3 },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram },
	{ 0xd020, 0xd03f, MWA_RAM, &elevator_scroll2 },
	{ 0xd040, 0xd05f, MWA_RAM, &elevator_scroll1 },
	{ 0xd50d, 0xd50d, MWA_NOP },
	{ 0xd50e, 0xd50e, elevatob_bankswitch_w },
{ 0xd40e, 0xd40f, MWA_RAM },
{ 0xd509, 0xd50a, MWA_RAM },
{ 0x8800, 0x8800, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 }
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
		0xbc,
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
	{ 1, 0x3000, &charlayout,    0, 3 },
	{ 1, 0x0000, &charlayout,    0, 3 },
	{ 1, 0x0000, &spritelayout, 3*8, 2 },
	{ 1, 0x1800, &spritelayout, 3*8, 2 },
	{ 1, 0x4800, &spritelayout, 3*8, 2 },	/* seems to be unused (contains garbage in the bootleg version) */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	25,34,195,	/* BLUE */
	25,161,153,	/* CYAN */
	68,68,68, /* DKGRAY */
  0xff,0x00,0x00, /* RED */
	153,153,153,	/* GRAY */
	229,229,229, /* LTGRAY */
	195,195,195, /* LTGRAY2 */
	238,153,195,	/* PINK */
	25,110,110,	/* DKCYAN */
	195,153,110,	/* BROWN1 */
	238,195,153,	/* BROWN2 */
	170,136,51,	/* BROWN3 */
	238,238,238, /* WHITE */
	238,110,195,	/* DKPINK */

  0x00,0xff,0x00, /* GREEN */
  0xff,0xff,0x00, /* YELLOW */
  0xff,0x00,0xff, /* MAGENTA */

	0xe0,0xb0,0x70,	/* BROWN */
	0xd0,0xa0,0x60,	/* BROWN0 */
	0x54,0x40,0x14,	/* BROWN4 */

  0x54,0xa8,0xff, /* LTBLUE */
  0x00,0xa0,0x00, /* DKGREEN */
  0x00,0xe0,0x00, /* GRASSGREEN */

	0xff,0xb6,0x49,	/* DKORANGE */
	0xff,0xb6,0x92,	/* LTORANGE */
};

enum {BLACK,BLUE,CYAN,DKGRAY,RED,GRAY,LTGRAY,LTGRAY2,PINK,DKCYAN,
		BROWN1,BROWN2,BROWN3,WHITE,DKPINK,

		GREEN,YELLOW,MAGENTA,
       BROWN,BROWN0,BROWN4,
			 LTBLUE,DKGREEN,GRASSGREEN,DKORANGE,LTORANGE
            };

static unsigned char colortable[] =
{
        0,BLUE,CYAN,LTGRAY,GRAY,RED,LTGRAY2,DKGRAY,
        0,PINK,DKGRAY,LTGRAY2,DKCYAN,GRAY,GRAY,15,
        0,BROWN3,WHITE,BROWN2,DKPINK,BROWN1,DKGRAY,RED,
		/* sprites */
        0,BROWN2,BROWN3,11,RED,LTGRAY,BROWN1,3,
        0,CYAN,GRAY,DKGRAY,BLUE,LTGRAY,LTGRAY2,5,
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

	ROM_REGION(0x2000)	/* background graphics */
	ROM_LOAD( "ea-ic7.bin",  0x0000, 0x1000 )
	ROM_LOAD( "ea-ic8.bin",  0x1000, 0x1000 )

#if 0
	ROM_LOAD( "ea-ic70.bin", , 0x1000 )
	ROM_LOAD( "ea-ic71.bin", , 0x1000 )
	ROM_LOAD( "ee_ea10.bin", , 0x1000 )
#endif
ROM_END

ROM_START( elevatob_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ea69.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea68.bin", 0x1000, 0x1000 )
	ROM_LOAD( "ea67.bin", 0x2000, 0x1000 )
	ROM_LOAD( "ea66.bin", 0x3000, 0x1000 )
	ROM_LOAD( "ea65.bin", 0x4000, 0x1000 )
	ROM_LOAD( "ea64.bin", 0x5000, 0x1000 )
	ROM_LOAD( "ea55.bin", 0x6000, 0x1000 )
	ROM_LOAD( "ea54.bin", 0x7000, 0x1000 )
	ROM_LOAD( "ea54.bin", 0xe000, 0x1000 )	/* copy for my convenience */
	ROM_LOAD( "ea52.bin", 0xf000, 0x1000 )	/* protection crack, bank switched at 7000 */

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ea01.bin",  0x0000, 0x1000 )
	ROM_LOAD( "ea02.bin",  0x1000, 0x1000 )
	ROM_LOAD( "ea03.bin",  0x2000, 0x1000 )
	ROM_LOAD( "ea04.bin",  0x3000, 0x1000 )
	ROM_LOAD( "ea05.bin",  0x4000, 0x1000 )
	ROM_LOAD( "ea06.bin",  0x5000, 0x1000 )

	ROM_REGION(0x2000)	/* background graphics */
	ROM_LOAD( "ea07.bin",  0x0000, 0x1000 )
	ROM_LOAD( "ea08.bin",  0x1000, 0x1000 )

#if 0
	ROM_LOAD( "ea70.bin", , 0x1000 )
	ROM_LOAD( "ea71.bin", , 0x1000 )
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

struct GameDriver elevatob_driver =
{
	"elevatob",
	&machine_driver,

	elevatob_rom,
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
