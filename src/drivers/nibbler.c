/***************************************************************************

Nibbler memory map (preliminary)

0000-03ff RAM
0400-07ff Video RAM (playfield 1)
0800-0bff Video RAM (playfield 2)
0c00-0fff Color RAM (4 bits per playfield)
1000-1fff Character generator RAM
3000-bfff ROM

read:
2104      IN0
2105      IN1
2106      DSW
2107      IN2

*
 * IN0 (bits NOT inverted)
 * bit 7 : LEFT player 1
 * bit 6 : RIGHT player 1
 * bit 5 : UP player 1
 * bit 4 : DOWN player 1
 * bit 3 : ?\
 * bit 2 : ?| debug
 * bit 1 : ?| commands?
 * bit 0 : ?/
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : LEFT player 2 (TABLE only)
 * bit 6 : RIGHT player 2 (TABLE only)
 * bit 5 : UP player 2 (TABLE only)
 * bit 4 : DOWN player 2 (TABLE only)
 * bit 3 : ?\
 * bit 2 : ?| debug
 * bit 1 : ?| commands?
 * bit 0 : ?/
 *
*
 * DSW (bits NOT inverted)
 * bit 7 :
 * bit 6 :
 * bit 5 : Free Play
 * bit 4 : RACK TEST
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :
 * bit 1 :\ nr of lives
 * bit 0 :/ 00 = 3  01 = 4  10 = 5  11 = 6
 *
*
 * IN2 (bits NOT inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : ?
 * bit 4 : ?
 * bit 3 : ?
 * bit 2 : ?
 * bit 1 : ?
 * bit 0 : ?
 *

write:
2000-2001 ?
2100-2103 ?
2200      ?
2300      ?

Interrupts: VBlank causes an IRQ. Coin insertion causes a NMI.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int nibbler_interrupt(void);

extern unsigned char *nibbler_videoram2;
extern unsigned char *nibbler_characterram;
extern void nibbler_videoram2_w(int offset,int data);
extern void nibbler_characterram_w(int offset,int data);
extern void nibbler_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void vanguard_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x3000, 0xbfff, MRA_ROM },
	{ 0xfffa, 0xffff, MRA_ROM },
	{ 0x2104, 0x2104, input_port_0_r },	/* IN0 */
	{ 0x2105, 0x2105, input_port_1_r },	/* IN1 */
	{ 0x2106, 0x2106, input_port_2_r },	/* DSW */
	{ 0x2107, 0x2107, input_port_3_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, videoram_w, &videoram, &videoram_size },
	{ 0x0800, 0x0bff, nibbler_videoram2_w, &nibbler_videoram2 },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, nibbler_characterram_w, &nibbler_characterram },
	{ 0x3000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, 0, 0, 0, OSD_KEY_DOWN, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ 0, 0, 0, 0, OSD_JOY_DOWN, OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_LEFT },
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x00,
		{ 0, 0, 0, 0, OSD_KEY_F1, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1 },
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
        { 0, 5, "MOVE UP" },
        { 0, 7, "MOVE LEFT"  },
        { 0, 6, "MOVE RIGHT" },
        { 0, 4, "MOVE DOWN" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 2, 0x04, "DIFFICULTY", { "EASY", "HARD" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xf000, &charlayout,    0, 8 },	/* the game dynamically modifies this */
	{ 1, 0x0800, &charlayout2, 8*4, 8 },
	{ -1 } /* end of array */
};


static unsigned char color_prom[] =
{
        /* foreground colors */
        0x00, 0x07, 0xff, 0xC5, 0x00, 0x38, 0xad, 0xA8,
        0x00, 0xad, 0x3f, 0xC0, 0x00, 0xff, 0x07, 0xFF,
        0x00, 0x3f, 0xc0, 0xAD, 0x00, 0xff, 0xc5, 0x3F,
        0x00, 0xff, 0x3f, 0x07, 0x00, 0x07, 0xc5, 0x3F,
        /* background colors */
        0x00, 0x3f, 0xff, 0xC0, 0x00, 0xc7, 0x38, 0x05,
        0x00, 0x07, 0xc0, 0x3F, 0x00, 0x3f, 0xe0, 0x05,
        0x00, 0x07, 0xac, 0xC0, 0x00, 0xff, 0xc5, 0x2F,
        0x00, 0xc0, 0x05, 0x2F, 0x00, 0x3f, 0x05, 0xC7
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			nibbler_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,16*4,
	vanguard_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	nibbler_vh_screenrefresh,

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

ROM_START( nibbler_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "IC12", 0x3000, 0x1000 )
	ROM_LOAD( "IC07", 0x4000, 0x1000 )
	ROM_LOAD( "IC08", 0x5000, 0x1000 )
	ROM_LOAD( "IC09", 0x6000, 0x1000 )
	ROM_LOAD( "IC10", 0x7000, 0x1000 )
	ROM_LOAD( "IC14", 0x8000, 0x1000 )
	ROM_LOAD( "IC15", 0x9000, 0x1000 )
	ROM_LOAD( "IC16", 0xa000, 0x1000 )
	ROM_LOAD( "IC17", 0xb000, 0x1000 )
	ROM_LOAD( "IC14", 0xf000, 0x1000 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC50", 0x0000, 0x1000 )
	ROM_LOAD( "IC51", 0x1000, 0x1000 )

	/* sound? */
/*	ROM_LOAD( "IC52", 0x????, 0x0800 ) */
/*	ROM_LOAD( "IC53", 0x????, 0x0800 ) */
ROM_END

ROM_START( fantasy_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic12.cpu", 0x3000, 0x1000 )
	ROM_LOAD( "ic07.cpu", 0x4000, 0x1000 )
	ROM_LOAD( "ic08.cpu", 0x5000, 0x1000 )
	ROM_LOAD( "ic09.cpu", 0x6000, 0x1000 )
	ROM_LOAD( "ic10.cpu", 0x7000, 0x1000 )
	ROM_LOAD( "ic14.cpu", 0x8000, 0x1000 )
	ROM_LOAD( "ic15.cpu", 0x9000, 0x1000 )
	ROM_LOAD( "ic16.cpu", 0xa000, 0x1000 )
	ROM_LOAD( "ic17.cpu", 0xb000, 0x1000 )
	ROM_LOAD( "ic14.cpu", 0xf000, 0x1000 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic50.vid", 0x0000, 0x1000 )
	ROM_LOAD( "ic51.vid", 0x1000, 0x1000 )

	/* sound? */
/*	ROM_LOAD( "ic51.cpu", 0x????, 0x0800 ) */
/*	ROM_LOAD( "ic52.cpu", 0x????, 0x0800 ) */
/*	ROM_LOAD( "ic53.cpu", 0x????, 0x0800 ) */
/*	ROM_LOAD( "ic07.dau", 0x????, 0x0800 ) */
/*	ROM_LOAD( "ic08.dau", 0x????, 0x0800 ) */
/*	ROM_LOAD( "ic11.dau", 0x????, 0x0800 ) */
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0290],"\x00\x50\x00\x00",4) == 0 &&
			memcmp(&RAM[0x02b4],"\x00\x05\x00\x00",4) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x0290],1,4*10,f);
			fread(&RAM[0x02d0],1,3*10,f);
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
		fwrite(&RAM[0x0290],1,4*10,f);
		fwrite(&RAM[0x02d0],1,3*10,f);
		fclose(f);
	}
}



struct GameDriver nibbler_driver =
{
	"Nibbler",
	"nibbler",
	"NICOLA SALMORIA\nBRIAN LEVINE\nMIRKO BUFFONI",
	&machine_driver,

	nibbler_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	hiload, hisave
};

struct GameDriver fantasy_driver =
{
	"Fantasy",
	"fantasy",
	"NICOLA SALMORIA\nBRIAN LEVINE\nMIRKO BUFFONI",
	&machine_driver,

	fantasy_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};
