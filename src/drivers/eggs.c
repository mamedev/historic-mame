/***************************************************************************

Eggs memory map (preliminary)

0000-07ff RAM
1000-13ff Video RAM (the left column contains sprites)
1400-17ff Attributes RAM
1800-1bff Mirror address of video RAM, but x and y coordinates are swapped
1c00-1fff Mirror address of color RAM, but x and y coordinates are swapped
3000-7fff ROM

read:
2000      DSW1
2001      DSW2??
2002      IN0
2003      IN1

write:
2000      flip screen
2001      Watchdog reset? interrupt acknowledge?
2004      8910 #0 control port
2005      8910 #0 write port
2006      8910 #1 control port
2007      8910 #1 write port

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int btime_mirrorvideoram_r(int offset);
int btime_mirrorcolorram_r(int offset);
void btime_mirrorvideoram_w(int offset,int data);
void btime_mirrorcolorram_w(int offset,int data);
void eggs_vh_screenrefresh(struct osd_bitmap *bitmap);

int eggs_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_r },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_r },
	{ 0x2000, 0x2000, input_port_2_r },	/* DSW1 */
	{ 0x2001, 0x2001, input_port_3_r },	/* DSW2?? */
	{ 0x2002, 0x2002, input_port_0_r },	/* IN0 */
	{ 0x2003, 0x2003, input_port_1_r },	/* IN1 */
	{ 0x3000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },	/* reset/interrupt vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_w },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_w },
	{ 0x2001, 0x2001, MWA_NOP },
	{ 0x2004, 0x2004, AY8910_control_port_0_w },
	{ 0x2005, 0x2005, AY8910_write_port_0_w },
	{ 0x2006, 0x2006, AY8910_control_port_1_w },
	{ 0x2007, 0x2007, AY8910_write_port_1_w },
	{ 0x3000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_LCONTROL, 0, OSD_KEY_3, OSD_KEY_4 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, IPB_VBLANK },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2?? */
		0xff,
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
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
        { 0, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x01, "LIVES", { "5", "3" }, 1 },
	{ 3, 0x02, "COIN A", { "1 COIN 2 PLAYS", "1 COIN 1 PLAY" }, 1 },
	{ 3, 0x0c, "COIN B", { "2 COINS 1 PLAY",  "1 COIN 2 PLAYS",  "1 COIN 3 PLAYS", "1 COIN 1 PLAY" }, 1 },
	{ 3, 0x10, "SW5", { "0", "1" } },
	{ 3, 0x20, "SW6", { "0", "1" } },
	{ 3, 0x40, "CABINET", { "UPRIGHT", "COCKTAIL" } },
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
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x0000, &spritelayout, 0, 1 },
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
	black, darkred,   blue,       darkyellow, yellow, green,     darkpurple, orange,
	black, darkgreen, darkred,    yellow,	   green, darkred,   darkgreen,  yellow,
	black, yellow,    darkgreen,  red,	         red, green,     orange,     yellow,
	black, darkwhite, red,        pink,        green, darkcyan,  red,        darkwhite
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
			interrupt,1
		},
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	eggs_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	eggs_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( eggs_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "e14.bin", 0x3000, 0x1000, 0x996dbebf )
	ROM_LOAD( "d14.bin", 0x4000, 0x1000, 0xbbbce334 )
	ROM_LOAD( "c14.bin", 0x5000, 0x1000, 0xc89bff1d )
	ROM_LOAD( "b14.bin", 0x6000, 0x1000, 0x86e4f94e )
	ROM_LOAD( "a14.bin", 0x7000, 0x1000, 0x9beb93e9 )
	ROM_RELOAD(          0xf000, 0x1000 )	/* for reset/interrupt vectors */

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g12.bin",  0x0000, 0x1000, 0x4beb2eab )
	ROM_LOAD( "g10.bin",  0x1000, 0x1000, 0x61460352 )
	ROM_LOAD( "h12.bin",  0x2000, 0x1000, 0x9c23f42b )
	ROM_LOAD( "h10.bin",  0x3000, 0x1000, 0x77b53ac7 )
	ROM_LOAD( "j12.bin",  0x4000, 0x1000, 0x27ab70f5 )
	ROM_LOAD( "j10.bin",  0x5000, 0x1000, 0x8786e8c4 )
ROM_END



static int hiload(void)
{
	/* check if the hi score table has already been initialized */
        if (	(memcmp(&RAM[0x0400],"\x17\x25\x19",3)==0) &&
		(memcmp(&RAM[0x041B],"\x00\x47\x00",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x0400],0x1E);
			/* Fix hi score at top */
			memcpy(&RAM[0x0015],&RAM[0x0403],3);
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
                osd_fwrite(f,&RAM[0x0400],0x1E);
		osd_fclose(f);
	}

}



struct GameDriver eggs_driver =
{
	"Eggs",
	"eggs",
	"NICOLA SALMORIA\nMIKE BALFOUR",
	&machine_driver,

	eggs_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
