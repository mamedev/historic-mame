/***************************************************************************

Zaxxon memory map (preliminary)

0000-1fff ROM 3
2000-3fff ROM 2
4000-4fff ROM 1
6000-67ff RAM 1
6800-6fff RAM 2
8000-83ff Video RAM
a000-a0ff sprites

read:
c000      IN0
c001      IN1
c002      DSW1
c003      DSW2
c100      IN2

*
 * IN0 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : FIRE player 1
 * bit 3 : UP player 1
 * bit 2 : DOWN player 1
 * bit 1 : LEFT player 1
 * bit 0 : RIGHT player 1
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : FIRE player 2 (TABLE only)
 * bit 3 : UP player 2 (TABLE only)
 * bit 2 : DOWN player 2 (TABLE only)
 * bit 1 : LEFT player 2 (TABLE only)
 * bit 0 : RIGHT player 2 (TABLE ony)
 *
*
 * IN2 (bits NOT inverted)
 * bit 7 : CREDIT
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : ?
 * bit 3 : START 2
 * bit 2 : START 1
 * bit 1 : ?
 * bit 0 : ?
 *
*
 * DSW1 (bits NOT inverted)
 * bit 7 :  COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 6 :  sound  0 = off  1 = on
 * bit 5 :\ lives
 * bit 4 :/ 00 = infinite  01 = 4  10 = 5  11 = 3
 * bit 3 :  ?
 * bit 2 :  ?
 * bit 1 :\ bonus
 * bit 0 :/ 00 = 40000  01 = 20000  10 = 30000  11 = 10000
 *
*
 * DSW1 (bits NOT inverted)
 * bit 7 :  COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 6 :  sound  0 = off  1 = on
 * bit 5 :\ lives
 * bit 4 :/ 00 = infinite  01 = 4  10 = 5  11 = 3
 * bit 3 :  ?
 * bit 2 :  ?
 * bit 1 :\ bonus
 * bit 0 :/ 00 = 40000  01 = 20000  10 = 30000  11 = 10000
 *
*
 * DSW2 (bits NOT inverted)
 * bit 7 :\
 * bit 6 :|  right coin slot
 * bit 5 :|
 * bit 4 :/
 * bit 3 :\
 * bit 2 :|  left coin slot
 * bit 1 :|
 * bit 0 :/
 *

write:
c000-c002 ?
c006      ?
ff3c-ff3f ?
fff0      interrupt enable
fff1      ?
fff8-fff9 background playfield position
fffa      background color? (0 = standard  1 = reddish)
fffb      background enable?
fffe      ?

interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI causes a ROM/RAM test.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int zaxxon_IN2_r(int offset);

extern unsigned char *zaxxon_background_position;
extern unsigned char *zaxxon_background_color;
extern unsigned char *zaxxon_background_enable;
extern void zaxxon_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int  zaxxon_vh_start(void);
extern void zaxxon_vh_stop(void);
extern void zaxxon_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa0ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc100, 0xc100, zaxxon_IN2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc003, 0xc003, input_port_4_r },
	{ 0x0000, 0x4fff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram },
	{ 0xfff0, 0xfff0, interrupt_enable_w },
	{ 0xfff8, 0xfff9, MWA_RAM, &zaxxon_background_position },
	{ 0xfffa, 0xfffa, MWA_RAM, &zaxxon_background_color },
	{ 0xfffb, 0xfffb, MWA_RAM, &zaxxon_background_enable },
	{ 0x0000, 0x4fff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, OSD_KEY_1, OSD_KEY_2,
				0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x7f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x33,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 0, 3, "PLANE UP" },
        { 0, 1, "PLANE LEFT"  },
        { 0, 0, "PLANE RIGHT" },
        { 0, 2, "PLANE DOWN" },
        { 0, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x30, "LIVES", { "INFINITE", "4", "5", "3" }, 1 },
	{ 3, 0x03, "BONUS", { "40000", "20000", "30000", "10000" }, 1 },
	{ 3, 0x40, "SOUND", { "OFF", "ON" } },
	{ 3, 0x04, "SW3", { "OFF", "ON" } },
	{ 3, 0x08, "SW4", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 103*8, 102*8, 101*8, 100*8, 99*8, 98*8, 97*8, 96*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,          0, 16 },	/* characters */
	{ 1, 0x1000, &spritelayout,      16*4, 32 },	/* sprites */
	{ 1, 0x7000, &charlayout2,  16*4+32*8, 32 },	/* background tiles */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
  0xFF,0x00,0x00, /* RED */
	0xdb,0x92,0x49, /* BROWN */
	0xff,0xb6,0xdb, /* PINK */
	0xFF,0xFF,0xFF, /* WHITE */
	0x00,0xFF,0xFF, /* CYAN */
	0x49,0xb6,0xdb, /* DKCYAN */
  0xFF,0x60,0x00, /* DKORANGE */
	0x00,0x00,0x96, /* DKBLUE */
  0xFF,0xFF,0x00, /* YELLOW */
	0x03,0x96,0xd2, /* LTBLUE */
	0x24,0x24,0xdb, /* BLUE */
	0x00,0xdb,0x00, /* GREEN */
	0x49,0xb6,0x92, /* DKGREEN */
	0xff,0xb6,0x92, /* LTORANGE */
	0xb6,0xb6,0xb6, /* GRAY */
	0x19,0x96,0x62, /* VDKGREEN */
  0xC0,0x00,0x00, /* DKRED */

  0xFF,0x00,0xFF, /* CUSTOM1 magenta*/
  0x80,0xC0,0xFF, /* CUSTOM2 blue plane*/
  0xFF,0xE0,0x00, /* CUSTOM3 */
  0xFF,0xC0,0x40, /* CUSTOM4 */

  0xc0,0xff,0x00, /* GREEN1 */
  0x80,0xe0,0x00, /* GREEN2 */
  0x40,0xc0,0x00, /* GREEN3 */

  0x00,0x00,0x80, /* BACK1 dark blue*/
  0x00,0x00,0xC0, /* BACK2 Blue */
  0x40,0xA0,0xFF, /* BACK3 */
  0x60,0xA0,0xE0, /* BACK4 */
  0xA0,0xD0,0xF0, /* BACK5 */
  0xC0,0xE0,0xFF, /* BACK6 */
  0x00,0x60,0xC0, /* BACK7 */
  0xE0,0x80,0xE0, /* BACK8 */
  0x50,0x90,0xD0, /* BACK9 */
  0x40,0x80,0xC0, /* BACK10 */
  0x80,0xB0,0xE0, /* BACK11 */

  0x00,0x00,0xFF, /* BLUE1 */
  0x00,0x00,0xC0, /* BLUE2 */
  0x00,0x00,0x80, /* BLUE3 */

  0xFF,0xFF,0xFF, /* YELLOW1 */
  0xFF,0xFF,0x00, /* YELLOW2 */
  0xFF,0xE0,0x00, /* YELLOW3 */
  0xE0,0xC0,0x00, /* YELLOW4 */
  0xD0,0xA0,0x00, /* YELLOW5 */

  0xA0,0x00,0x00, /* DKRED2 */
  0x80,0x00,0x00, /* DKRED3 */

  0x80,0xA0,0xC0, /* GRAY1 */
  0x90,0xA0,0xC0, /* GRAY2 */
  0xC0,0xE0,0xFF, /* GRAY3 */
  0xA0,0xC0,0xE0, /* GRAY4 */
  0x80,0xA0,0xC0, /* GRAY5 */
  0x60,0x80,0xA0, /* GRAY6 */
};

enum {BLACK,RED,BROWN,PINK,WHITE,CYAN,DKCYAN,DKORANGE,
		DKBLUE,YELLOW,LTBLUE,BLUE,GREEN,DKGREEN,LTORANGE,GRAY,
		VDKGREEN,DKRED,
		CUSTOM1,CUSTOM2,CUSTOM3,CUSTOM4,
		GREEN1,GREEN2,GREEN3,
		BACK1,BACK2,BACK3,BACK4,BACK5,BACK6,BACK7,BACK8,BACK9,BACK10,BACK11,
		BLUE1,BLUE2,BLUE3,
		YELLOW1,YELLOW2,YELLOW3,YELLOW4,YELLOW5,
		DKRED2,DKRED3,GRAY1,GRAY2,GRAY3,GRAY4,GRAY5,GRAY6};

static unsigned char colortable[] =
{
	/* chars */
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,YELLOW,RED,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,WHITE,WHITE,
	0,BLACK,BACK3,WHITE,
	0,BLACK,BACK3,BLACK,
	0,BLACK,YELLOW,RED,


	/* sprites */
	0,YELLOW3,YELLOW1,YELLOW2,YELLOW2,YELLOW1,YELLOW5,DKORANGE,   /* Explosion Space */
	0,RED,WHITE,CYAN,YELLOW,BLUE,GREEN,DKORANGE,     /* Explosion plane 1 */
	0,DKRED,YELLOW,CYAN,RED,BLUE,GREEN,DKORANGE,     /* Explosion plane 2 */
	0,RED,DKRED,RED,DKORANGE,RED,DKRED,DKRED2,      /* Explosion Rocket 1 and NMI bullet in space (low level)*/
	0,BLACK,BLACK,GREEN,DKORANGE,RED,DKRED,DKRED2,  /* Player Bullet and NMI in space */
	0,0,0,0,0,0,0,0,
	0,RED,RED,RED,RED,RED,RED,RED,           /* NMI Bullet */
	0,BLUE2,RED,BACK5,BACK4,BLUE1,BLUE3,BLACK,      /* Satellite 1/2 */
	0,DKRED,BLACK,DKCYAN,RED,RED,RED,RED,        /* Plane When Hit and Cannon bullet */
	0,BLACK,GRAY5,GRAY1,GRAY2,GRAY3,GRAY4,GRAY6, /* Cannon 2 */
	0,WHITE,BACK3,BACK5,BACK6,BACK9,DKORANGE,BLACK,/* NMI Plane 1 (and Standing planes) */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BLACK,WHITE,BLACK,BLACK,BLACK,BLACK,BLACK,       /* Cross-Target */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,CUSTOM1,RED,BACK5,CUSTOM3,CUSTOM4,BLACK,WHITE,     /* Plane and Force field */
	0,YELLOW1,YELLOW2,YELLOW3,YELLOW4,YELLOW5,BLACK,BLACK,    /* NMI Plane 2 */
	0,BLACK,GREEN3,RED,CUSTOM3,GREEN1,GREEN2,GREEN2, /* Cannon 1 */
	0,WHITE,BLACK,BACK6,BACK5,BACK4,LTBLUE,BLACK, /* Wall and Junk pile from shooting radar,fuel...(with score) */
	0,GRAY5,BLACK,RED,DKRED,DKORANGE,CUSTOM3,WHITE,  /* Radar & Fuel */
	0,RED,DKRED,RED,DKORANGE,RED,CUSTOM3,LTORANGE,      /* Rocket 1 */
	0,WHITE,BACK6,BACK5,BACK4,BACK11,BACK10,BLACK,      /* Zaxxon 1 */
	0,BLUE2,CYAN,BACK5,BACK4,BLUE1,BLUE3,BLACK,      /* Satellite 2/2 */
	0,WHITE,PINK,DKORANGE,CYAN,RED,DKRED,BLACK,      /* Zaxxon 2 (RED) */
	0,RED,DKRED,RED,DKORANGE,YELLOW,LTORANGE,LTORANGE,/* Rocket 2 */
	0,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,     /* Shadow of Plane */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BROWN,DKORANGE,DKRED2,RED,DKRED,DKRED2,BLACK,      /* Zaxxon Dead (more RED) */
	0,0,0,0,0,0,0,0,

	/* background tiles (normal) */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BLACK,BACK7,BACK8,BACK2,BACK4,BLACK,BLACK,        /* Zaxxon Arena */
	0,DKBLUE,LTBLUE,WHITE,DKCYAN,BLACK,BLUE,CYAN,    /* Space BG */
	0,BACK1,WHITE,BACK5,BLACK,BACK6,BACK2,BACK4,      /* City BG 2 */
	0,BACK2,BACK5,BACK4,BACK6,BACK1,BACK3,BLACK,  /* City BG 1 */
	0,WHITE,BACK6,BLACK,BACK5,BACK4,YELLOW,BACK2,      /* Buildings */
	/* background tiles (during death sequencel) */
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,BLACK,BLACK,RED,DKRED2,DKRED3,BLACK,BLACK,        /* Zaxxon Arena */
	0,DKBLUE,LTBLUE,WHITE,DKCYAN,DKRED2,BLUE,CYAN,    /* Space BG */
	0,DKRED3,WHITE,BACK4,BLACK,WHITE,DKRED2,BACK10,      /* City BG 2 */
	0,DKRED2,BACK4,BACK10,WHITE,DKRED3,BACK3,BLACK,  /* City BG 1 */
	0,WHITE,WHITE,BLACK,BACK4,BACK10,YELLOW,DKRED2      /* Buildings */
};




static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	zaxxon_vh_start,
	zaxxon_vh_stop,
	zaxxon_vh_screenrefresh,

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

ROM_START( zaxxon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "%s.3",  0x0000, 0x2000 )
	ROM_LOAD( "%s.2",  0x2000, 0x2000 )
	ROM_LOAD( "%s.1",  0x4000, 0x1000 )

	ROM_REGION(0xd000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "%s.15", 0x0000, 0x0800 )
	ROM_LOAD( "%s.14", 0x0800, 0x0800 )
	ROM_LOAD( "%s.13", 0x1000, 0x2000 )
	ROM_LOAD( "%s.12", 0x3000, 0x2000 )
	ROM_LOAD( "%s.11", 0x5000, 0x2000 )
	ROM_LOAD( "%s.6",  0x7000, 0x2000 )
	ROM_LOAD( "%s.5",  0x9000, 0x2000 )
	ROM_LOAD( "%s.4",  0xb000, 0x2000 )

	ROM_REGION(0x8000)	/* background graphics */
	ROM_LOAD( "%s.8",  0x0000, 0x2000 )
	ROM_LOAD( "%s.7",  0x2000, 0x2000 )
	ROM_LOAD( "%s.10", 0x4000, 0x2000 )
	ROM_LOAD( "%s.9",  0x6000, 0x2000 )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6110],"\x00\x89\x00",3) == 0 &&
			memcmp(&RAM[0x6179],"\x00\x37\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x6100],1,21*6,f);
			RAM[0x6038] = RAM[0x6110];
			RAM[0x6039] = RAM[0x6111];
			RAM[0x603a] = RAM[0x6112];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x6100],1,21*6,f);
		fclose(f);
	}
}



struct GameDriver zaxxon_driver =
{
	"Zaxxon",
	"zaxxon",
	"MIRKO BUFFONI\nNICOLA SALMORIA\nMARC VERGOOSSEN\nMARC LAFONTAINE",
	&machine_driver,

	zaxxon_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, colortable,
	{ 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,	/* numbers */
		0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,	/* letters */
		0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a },
	0x00, 0x0f,
	8*13, 8*16, 0x0f,

	hiload, hisave
};
