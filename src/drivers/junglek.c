/***************************************************************************

Jungle King memory map (preliminary)

0000-7fff ROM
8000-87ff RAM
c400-c7ff Video RAM 1
c800-cbff Video RAM 2
cc00-cfff Video RAM 3
d100-d17f Sprites

read:
d404      ?
d408      IN0
          bit 4 = fire player 1
          bit 3 = up player 1
          bit 2 = down player 1
          bit 1 = right player 1
          bit 0 = left player 1
d409      IN1
          bit 4 = fire player 2 (COCKTAIL only)
          bit 3 = up player 2 (COCKTAIL only)
          bit 2 = down player 2 (COCKTAIL only)
          bit 1 = right player 2 (COCKTAIL only)
          bit 0 = left player 2 (COCKTAIL only)
d40a      DSW1
          bit 7   = cocktail / upright (0 = upright)
          bit 6   = flip screen
          bit 5   = RAM check
          bit 3-4 = lives
		  bit 2   = ?
          bit 0-1 = finish bonus
d40b      IN2
          bit 7 = start 2
          bit 6 = start 1
d40c      COIN
          bit 5 = tilt
          bit 4 = coin
d40f      DSW3
          bit 7 = coinage (1 way/2 ways)
          bit 6 = infinite lives
          bit 5 = year display yes/no
		  bit 2-4 ?
		  bit 0-1 bonus  none /10000 / 20000 /30000

write:
d502      playfield 2 scroll
d504      playfield 1 scroll
d509-d50a ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *junglek_videoram2,*junglek_videoram3;
extern unsigned char *junglek_scroll1,*junglek_scroll2;
extern unsigned char *junglek_attributesram;
extern void junglek_videoram2_w(int offset,int data);
extern void junglek_videoram3_w(int offset,int data);
extern int junglek_vh_start(void);
extern void junglek_vh_stop(void);
extern void junglek_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int junglek_sh_interrupt(void);
extern int junglek_sh_start(void);



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
{ 0xd404, 0xd404, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xc400, 0xc7ff, videoram_w, &videoram },
	{ 0xc800, 0xcbff, junglek_videoram2_w, &junglek_videoram2 },
	{ 0xcc00, 0xcfff, junglek_videoram3_w, &junglek_videoram3 },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram },
	{ 0xd502, 0xd502, MWA_RAM, &junglek_scroll2 },
	{ 0xd504, 0xd504, MWA_RAM, &junglek_scroll1 },
{ 0xd509, 0xd50a, MWA_RAM },
{ 0x9000, 0xbfff, MWA_RAM },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x0000, 0x0fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_0_w },
	{ 0x4801, 0x4801, AY8910_write_port_0_w },
	{ 0x4802, 0x4802, AY8910_control_port_1_w },
	{ 0x4803, 0x4803, AY8910_write_port_1_w },
	{ 0x4804, 0x4804, AY8910_control_port_2_w },
	{ 0x4805, 0x4805, AY8910_write_port_2_w },
	{ 0x0000, 0x0fff, MWA_ROM },
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
		0x3f,
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
	{ 5, 0x03, "BONUS", { "30000", "20000", "10000", "NONE" }, 1 },
	{ 4, 0x03, "FINISH BONUS", { "TIMER X3", "TIMER X2", "TIMER X1", "NONE" }, 1 },
	{ 5, 0x40, "DEMO MODE", { "ON", "OFF" }, 1 },
	{ 5, 0x20, "YEAR DISPLAY", { "NO", "YES" }, 1 },
	{ 4, 0x04, "SW A 3", { "ON", "OFF" }, 1 },
	{ 5, 0x04, "SW C 3", { "ON", "OFF" }, 1 },
	{ 5, 0x08, "SW C 4", { "ON", "OFF" }, 1 },
	{ 5, 0x10, "SW C 5", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	160,	/* 160 characters */
	3,	/* 3 bits per pixel */
	{ 0, 160*8*8, 320*8*8 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout bgcharlayout =
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
	{ 0, 256*8*8, 512*8*8 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0,
		8*8+7, 8*8+6, 8*8+5, 8*8+4, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x6000, &charlayout,    0, 32 },
	{ 1, 0x0000, &bgcharlayout,     0, 32 },
	{ 1, 0x1800, &bgcharlayout,     0, 32 },
	{ 1, 0x3000, &bgcharlayout,     0, 32 },
	{ 1, 0x4800, &bgcharlayout,     0, 32 },
	{ 1, 0x7000, &charlayout,    0, 32 },
	{ 1, 0x0000, &spritelayout,     0, 32 },
	{ 1, 0x1800, &spritelayout,     0, 32 },
	{ 1, 0x3000, &spritelayout,     0, 32 },
	{ 1, 0x4800, &spritelayout,     0, 32 },
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
#if 0
		{
			CPU_Z80,
			2000000,	/* 2 Mhz ???? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			junglek_sh_interrupt,1
		}
#endif
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	junglek_vh_start,
	junglek_vh_stop,
	junglek_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	junglek_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( junglek_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "kn41.bin",  0x0000, 0x1000 )
	ROM_LOAD( "kn42.bin",  0x1000, 0x1000 )
	ROM_LOAD( "kn43.bin",  0x2000, 0x1000 )
	ROM_LOAD( "kn44.bin",  0x3000, 0x1000 )
	ROM_LOAD( "kn45.bin",  0x4000, 0x1000 )
	ROM_LOAD( "kn46.bin",  0x5000, 0x1000 )
	ROM_LOAD( "kn47.bin",  0x6000, 0x1000 )
	ROM_LOAD( "kn48.bin",  0x7000, 0x1000 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "kn49.bin",  0x0000, 0x1000 )
	ROM_LOAD( "kn50.bin",  0x1000, 0x1000 )
	ROM_LOAD( "kn51.bin",  0x2000, 0x1000 )
	ROM_LOAD( "kn52.bin",  0x3000, 0x1000 )
	ROM_LOAD( "kn53.bin",  0x4000, 0x1000 )
	ROM_LOAD( "kn54.bin",  0x5000, 0x1000 )
	ROM_LOAD( "kn55.bin",  0x6000, 0x1000 )
	ROM_LOAD( "kn56.bin",  0x7000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "kn57-1.bin", 0x0000, 0x1000 )
	ROM_LOAD( "kn58-1.bin", 0x1000, 0x1000 )	/* ??? */
	ROM_LOAD( "kn59-1.bin", 0x2000, 0x1000 )	/* ??? */
	ROM_LOAD( "kn60.bin",   0x3000, 0x1000 )	/* ??? */
ROM_END

ROM_START( jungleh_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "kn41a",  0x0000, 0x1000 )
	ROM_LOAD( "kn42",   0x1000, 0x1000 )
	ROM_LOAD( "kn43",   0x2000, 0x1000 )
	ROM_LOAD( "kn44",   0x3000, 0x1000 )
	ROM_LOAD( "kn45",   0x4000, 0x1000 )
	ROM_LOAD( "kn46a",  0x5000, 0x1000 )
	ROM_LOAD( "kn47",   0x6000, 0x1000 )
	ROM_LOAD( "kn48a",  0x7000, 0x1000 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "kn49a",  0x0000, 0x1000 )
	ROM_LOAD( "kn50a",  0x1000, 0x1000 )
	ROM_LOAD( "kn51a",  0x2000, 0x1000 )
	ROM_LOAD( "kn52a",  0x3000, 0x1000 )
	ROM_LOAD( "kn53a",  0x4000, 0x1000 )
	ROM_LOAD( "kn54a",  0x5000, 0x1000 )
	ROM_LOAD( "kn55",   0x6000, 0x1000 )
	ROM_LOAD( "kn56a",  0x7000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "kn57-1", 0x0000, 0x1000 )
	ROM_LOAD( "kn58-1", 0x1000, 0x1000 )	/* ??? */
	ROM_LOAD( "kn59-1", 0x2000, 0x1000 )	/* ??? */
	ROM_LOAD( "kn60",   0x3000, 0x1000 )	/* ??? */
ROM_END



struct GameDriver junglek_driver =
{
	"junglek",
	&machine_driver,

	junglek_rom,
	0, 0,
	0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0x06, 0x04,
	8*13, 8*16, 0x00,

	0, 0
};



struct GameDriver jungleh_driver =
{
	"jungleh",
	&machine_driver,

	jungleh_rom,
	0, 0,
	0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0x06, 0x04,
	8*13, 8*16, 0x00,

	0, 0
};
