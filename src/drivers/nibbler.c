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



int vanguard_interrupt(void);

extern unsigned char *nibbler_videoram2;
extern unsigned char *nibbler_characterram;
void nibbler_videoram2_w(int offset,int data);
void nibbler_characterram_w(int offset,int data);
void nibbler_vh_screenrefresh(struct osd_bitmap *bitmap);
void vanguard_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

void vanguard_sound0_w(int offset,int data);
void vanguard_sound1_w(int offset,int data);
int vanguard_sh_start(void);
void vanguard_sh_update(void);


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
	{ 0x2100, 0x2100, vanguard_sound0_w },
	{ 0x2101, 0x2101, vanguard_sound1_w },
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
	0x00,0x07,0xff,0xC5,0x00,0x38,0xad,0xA8,0x00,0xad,0x3f,0xC0,0x00,0xff,0x07,0xFF,
	0x00,0x3f,0xc0,0xAD,0x00,0xff,0xc5,0x3F,0x00,0xff,0x3f,0x07,0x00,0x07,0xc5,0x3F,
	/* background colors */
	0x00,0x3f,0xff,0xC0,0x00,0xc7,0x38,0x05,0x00,0x07,0xc0,0x3F,0x00,0x3f,0xe0,0x05,
	0x00,0x07,0xac,0xC0,0x00,0xff,0xc5,0x2F,0x00,0xc0,0x05,0x2F,0x00,0x3f,0x05,0xC7
};


static unsigned char samples[32] =
{
   0x88, 0x88, 0x88, 0x88, 0xaa, 0xaa, 0xaa, 0xaa,
   0xcc, 0xcc, 0xcc, 0xcc, 0xee, 0xee, 0xee, 0xee,
   0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,
   0x44, 0x44, 0x44, 0x44, 0x66, 0x66, 0x66, 0x66
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
			vanguard_interrupt,2
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16*4,16*4,
	vanguard_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	nibbler_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	vanguard_sh_start,
	0,
	vanguard_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( nibbler_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "IC12", 0x3000, 0x1000, 0x63b52ccf )
	ROM_LOAD( "IC07", 0x4000, 0x1000, 0x5de3931d )
	ROM_LOAD( "IC08", 0x5000, 0x1000, 0x4f2b5fc9 )
	ROM_LOAD( "IC09", 0x6000, 0x1000, 0x157dbec5 )
	ROM_LOAD( "IC10", 0x7000, 0x1000, 0xa04d2ef5 )
	ROM_LOAD( "IC14", 0x8000, 0x1000, 0x4d9217c2 )
	ROM_RELOAD(       0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "IC15", 0x9000, 0x1000, 0x88b3edc7 )
	ROM_LOAD( "IC16", 0xa000, 0x1000, 0x7a1d25a9 )
	ROM_LOAD( "IC17", 0xb000, 0x1000, 0xd000eafc )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC50", 0x0000, 0x1000, 0x2bc0b3f0 )
	ROM_LOAD( "IC51", 0x1000, 0x1000, 0x27bd8de9 )

	ROM_REGION(0x1000)	/* space for the sound ROMs */
	ROM_LOAD( "IC52", 0x0000, 0x0800, 0x9ad746f9 )
	ROM_LOAD( "IC53", 0x0800, 0x0800, 0xbe207f5e )
ROM_END



static int hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0290],"\x00\x50\x00\x00",4) == 0 &&
			memcmp(&RAM[0x02b4],"\x00\x05\x00\x00",4) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0290],4*10);
			osd_fread(f,&RAM[0x02d0],3*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0290],4*10);
		osd_fwrite(f,&RAM[0x02d0],3*10);
		osd_fclose(f);
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

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

