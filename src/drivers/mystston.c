/***************************************************************************

Mysterious Stones memory map (preliminary)

MAIN BOARD:

0000-0fff RAM
1000-13ff Video RAM
1400-17ff Attribute RAM
1800-19ff Background video RAM #1
1a00-1bff Background attribute RAM #1
1c00-1fff probably scratchpad RAM, contains a copy of the background without objects
4000-ffff ROM

read
2000      IN0
          bit 7 = coin 2 (must also cause a NMI)
          bit 6 = coin 1 (must also cause a NMI)
          bit 0-5 = player 1 controls
2010      IN1
          bit 7 = start 2
          bit 6 = start 1
		  bit 0-5 = player 2 controls (cocktail only)
2020      DSW1
          bit 3-7 = probably unused
          bit 2 = ?
		  bit 1 = ?
          bit 0 = lives
2030      DSW2
          bit 7 = vblank
          bit 6 = coctail/upright (0 = upright)
		  bit 4-5 = probably unused
		  bit 0-3 = coins per play

write
0780-07df sprites
2000      ?
2010      ?
2020      background scroll
2030      ?
2040      ?
2060-2077 palette

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern int mystston_DSW2_r(int offset);
extern int mystston_interrupt(void);

extern unsigned char *mystston_videoram2,*mystston_colorram2;
extern int mystston_videoram2_size;
extern unsigned char *mystston_scroll;
extern unsigned char *mystston_paletteram;
extern void mystston_paletteram_w(int offset,int data);
extern void mystston_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int mystston_vh_start(void);
void mystston_vh_stop(void);
extern void mystston_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x2030, 0x2030, mystston_DSW2_r },
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
	{ 0x2000, 0x2000, input_port_0_r },
	{ 0x2010, 0x2010, input_port_1_r },
	{ 0x2020, 0x2020, input_port_2_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x13ff, MWA_RAM, &mystston_videoram2, &mystston_videoram2_size },
	{ 0x1400, 0x17ff, MWA_RAM, &mystston_colorram2 },
	{ 0x1800, 0x19ff, videoram_w, &videoram, &videoram_size },
	{ 0x1a00, 0x1bff, colorram_w, &colorram },
	{ 0x1c00, 0x1fff, MWA_RAM },
	{ 0x2020, 0x2020, MWA_RAM, &mystston_scroll },
	{ 0x2060, 0x2077, mystston_paletteram_w, &mystston_paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ 0x0780, 0x07df, MWA_RAM, &spriteram, &spriteram_size },	/* handled by the MWA_RAM above, here */
												/* to initialize the pointer */
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_ALT, OSD_KEY_CONTROL, 0, OSD_KEY_3 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE2, OSD_JOY_FIRE1, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 1, "MOVE LEFT"  },
        { 0, 0, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 5, "FIRE" },
        { 0, 4, "KICK" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x01, "LIVES", { "5", "3" }, 1 },
	{ 2, 0x02, "SW2", { "0", "1" } },
	{ 2, 0x04, "SW3", { "0", "1" } },
	{ 2, 0x08, "SW4", { "0", "1" } },
	{ 2, 0x10, "SW5", { "0", "1" } },
	{ 2, 0x20, "SW6", { "0", "1" } },
	{ 2, 0x40, "SW7", { "0", "1" } },
	{ 2, 0x80, "SW8", { "0", "1" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 16 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	256,    /* 256 tiles */
	3,	/* 3 bits per pixel */
	{ 2*512*16*16, 512*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every tile takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   1*8, 1 },
	{ 1, 0x06000, &charlayout,   1*8, 1 },
	{ 1, 0x00000, &spritelayout,   0, 1 },
	{ 1, 0x06000, &spritelayout,   0, 1 },
	{ 1, 0x0c000, &tilelayout,   2*8, 1 },
	{ 1, 0x0e000, &tilelayout,   2*8, 1 },
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ???? */
			0,
			readmem,writemem,0,0,
			mystston_interrupt,1
		},
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256, 3*8,
	mystston_vh_convert_color_prom,

	0,
	mystston_vh_start,
	mystston_vh_stop,
	mystston_vh_screenrefresh,

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

ROM_START( mystston_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ms0", 0x4000, 0x2000 )
	ROM_LOAD( "ms1", 0x6000, 0x2000 )
	ROM_LOAD( "ms2", 0x8000, 0x2000 )
	ROM_LOAD( "ms3", 0xa000, 0x2000 )
	ROM_LOAD( "ms4", 0xc000, 0x2000 )
	ROM_LOAD( "ms5", 0xe000, 0x2000 )

	ROM_REGION(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ms6",  0x00000, 0x2000 )
	ROM_LOAD( "ms7",  0x02000, 0x2000 )
	ROM_LOAD( "ms8",  0x04000, 0x2000 )
	ROM_LOAD( "ms9",  0x06000, 0x2000 )
	ROM_LOAD( "ms10", 0x08000, 0x2000 )
	ROM_LOAD( "ms11", 0x0a000, 0x2000 )
	ROM_LOAD( "ms12", 0x0c000, 0x2000 )
	ROM_LOAD( "ms13", 0x0e000, 0x2000 )
	ROM_LOAD( "ms14", 0x10000, 0x2000 )
	ROM_LOAD( "ms15", 0x12000, 0x2000 )
	ROM_LOAD( "ms16", 0x14000, 0x2000 )
	ROM_LOAD( "ms17", 0x16000, 0x2000 )
ROM_END



struct GameDriver mystston_driver =
{
	"Mysterious Stones",
	"mystston",
	"NICOLA SALMORIA",
	&machine_driver,

	mystston_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	0, 0, 0,
	8*13, 8*16,

	0, 0
};
