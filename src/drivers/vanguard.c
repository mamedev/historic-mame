/***************************************************************************
Note: This driver has been revised to use the sound driver.
Sound ROMS sk4_ic51.bin and sk4_ic52.bin are loaded...but don't have
the right checksums.
Andrew Scott (ascott@utkux.utcc.utk.edu)

Vanguard memory map (preliminary)

Note that this memory map is very similar to Nibbler

0000-03ff RAM
0400-07ff Video RAM 1
0800-0bff Video RAM 2
0c00-0fff Color RAM (3 bits for video RAM 1 and 3 bits for video RAM 2)
1000-1fff Character generator RAM
4000-bfff ROM

read:
3104      IN0
3105      IN1
3106      DSW ??
3107      IN2

write
3100      Sound Port 0
3101      Sound Port 1
3200      y scroll register
3300      x scroll register

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *vanguard_videoram2;
extern unsigned char *vanguard_characterram;
extern unsigned char *vanguard_scrollx,*vanguard_scrolly;

int vanguard_interrupt(void);
int vanguard_vh_start(void);
void vanguard_vh_stop(void);
void vanguard_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void vanguard_vh_screenrefresh(struct osd_bitmap *bitmap);
void vanguard_characterram_w(int offset,int data);

void vanguard_sound0_w(int offset,int data);
void vanguard_sound1_w(int offset,int data);
int vanguard_sh_start(void);
void vanguard_sh_update(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x4000, 0xbfff, MRA_ROM },
	{ 0x3104, 0x3104, input_port_0_r },	/* IN0 */
	{ 0x3105, 0x3105, input_port_1_r },	/* IN1 */
	{ 0x3106, 0x3106, input_port_2_r },	/* DSW ?? */
	{ 0x3107, 0x3107, input_port_3_r },	/* IN2 */
	{ 0xf000, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &vanguard_videoram2 },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, vanguard_characterram_w, &vanguard_characterram },
	{ 0x3300, 0x3300, MWA_RAM, &vanguard_scrollx },
	{ 0x3200, 0x3200, MWA_RAM, &vanguard_scrolly },
	{ 0x3100, 0x3100, vanguard_sound0_w },
	{ 0x3101, 0x3101, vanguard_sound1_w },
	{ 0x4000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress fantasy_readmem[] =
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

static struct MemoryWriteAddress fantasy_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &vanguard_videoram2 },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0fff, colorram_w, &colorram },
	{ 0x1000, 0x1fff, vanguard_characterram_w, &vanguard_characterram },
	{ 0x2100, 0x2100, vanguard_sound0_w },
	{ 0x2101, 0x2101, vanguard_sound1_w },
	{ 0x2300, 0x3300, MWA_RAM, &vanguard_scrollx },
	{ 0x2200, 0x3200, MWA_RAM, &vanguard_scrolly },
	{ 0x3000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_D, OSD_KEY_E, OSD_KEY_F, OSD_KEY_S,
			OSD_KEY_DOWN, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ OSD_JOY_FIRE3, OSD_JOY_FIRE2, OSD_JOY_FIRE4, OSD_JOY_FIRE1,
			OSD_JOY_DOWN, OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_LEFT }
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_D, OSD_KEY_E, OSD_KEY_F, OSD_KEY_S,
			OSD_KEY_DOWN, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ OSD_JOY_FIRE3, OSD_JOY_FIRE2, OSD_JOY_FIRE4, OSD_JOY_FIRE1,
			OSD_JOY_DOWN, OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_LEFT }
	},
	{	/* DSW ?? */
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
        { 0, 1, "FIRE UP" },
        { 0, 3, "FIRE LEFT"  },
        { 0, 2, "FIRE RIGHT" },
        { 0, 0, "FIRE DOWN" },
        { -1 }
};



static struct DSW dsw[] =
{
	{ 2, 0x30, "LIVES", { "3", "4", "5", "3" } },
	{ 2, 0x40, "COINS PER CREDIT", { "1", "2" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
        8,8,    /* 8*8 characters */
        256,    /* 256 characters */
        2,      /* 2 bits per pixel */
        { 0, 256*8*8 }, /* the two bitplanes are separated */
        { 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
        { 0, 1, 2, 3, 4, 5, 6, 7 },
        8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout3 =
{
	8,8,	/* 8*8 characters */
	512,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xf000, &charlayout,    0, 8 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &charlayout2, 8*4, 8 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo fantasy_gfxdecodeinfo[] =
{
	{ 0, 0xf000, &charlayout,    0, 8 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &charlayout3, 8*4, 8 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* foreground colors */
	0x00,0x2F,0xF4,0xFF,0xEF,0xF8,0xFF,0x07,0xFE,0xC0,0x07,0x3F,0xFF,0x3F,0xC6,0xC0,
	0x00,0x38,0xE7,0x07,0xEF,0xC0,0xF4,0xFF,0xFE,0xFF,0xF8,0xC0,0xFF,0xC6,0xE7,0xC0,
	/* background colors */
	0x00,0x80,0x3F,0xC6,0xEF,0xC6,0x2F,0xF8,0xFE,0xC6,0xE7,0xC0,0xFF,0x2F,0x38,0xC6,
	0x00,0x07,0x80,0x2F,0xEF,0x07,0xF8,0xFF,0xFE,0xFF,0xF8,0xC0,0xFF,0xE7,0xC6,0xF4
};

static unsigned char fantasy_color_prom[] =
{
   0x00, 0x07, 0x38, 0xF8, 0x00, 0x05, 0x2F, 0xF8,
   0x00, 0xC0, 0x2F, 0xF8, 0x00, 0xF8, 0x07, 0xFF,
   0x00, 0x07, 0xF4, 0xC0, 0x00, 0x38, 0xC5, 0x07,
   0x00, 0xFF, 0x2F, 0x3F, 0x00, 0xC5, 0xC5, 0xC0,
   0x00, 0x06, 0x27, 0x05, 0x7F, 0x2F, 0x38, 0x05,
   0x00, 0x38, 0x05, 0x28, 0x00, 0x07, 0xFF, 0xF4,
   0x00, 0x07, 0x38, 0x2F, 0x00, 0xF8, 0xFF, 0xC5,
   0x00, 0xC0, 0x38, 0xF4, 0x00, 0x27, 0xFF, 0x2F
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
	32*8, 32*8, { 4*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16*4,16*4,
	vanguard_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	vanguard_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	vanguard_sh_start,
	0,
	vanguard_sh_update
};

static struct MachineDriver fantasy_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			fantasy_readmem,fantasy_writemem,0,0,
			vanguard_interrupt,2
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 4*8, 32*8-1, 0*8, 32*8-1 },
	fantasy_gfxdecodeinfo,
	16*4,16*4,
	vanguard_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	vanguard_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	vanguard_sh_start,
	0,
	vanguard_sh_update
};

static const char *vanguard_sample_names[]={
        "fire.sam",
        "explsion.sam",
        0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( vanguard_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sk4_ic07.bin", 0x4000, 0x1000, 0x8dd6cf9a )
	ROM_LOAD( "sk4_ic08.bin", 0x5000, 0x1000, 0xa3740a46 )
	ROM_LOAD( "sk4_ic09.bin", 0x6000, 0x1000, 0x27c429cc )
	ROM_LOAD( "sk4_ic10.bin", 0x7000, 0x1000, 0x1e153237 )
	ROM_LOAD( "sk4_ic13.bin", 0x8000, 0x1000, 0x32820c48 )
	ROM_RELOAD(               0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "sk4_ic14.bin", 0x9000, 0x1000, 0x4b34aaea )
	ROM_LOAD( "sk4_ic15.bin", 0xa000, 0x1000, 0x64432d1d )
	ROM_LOAD( "sk4_ic16.bin", 0xb000, 0x1000, 0xb4d9810f )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sk5_ic50.bin", 0x0000, 0x0800, 0x6a6bc3f7 )
	ROM_LOAD( "sk5_ic51.bin", 0x0800, 0x0800, 0x7eb71bd1 )

	ROM_REGION(0x1000)	/* space for the sound ROMs */
	ROM_LOAD( "sk4_ic51.bin", 0x0000, 0x0800, 0x7f305f4c )  /* sound ROM 1 */
	ROM_LOAD( "sk4_ic52.bin", 0x0800, 0x0800, 0xe6fb89fb )  /* sound ROM 2 */
ROM_END

ROM_START( fantasy_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic12.cpu", 0x3000, 0x1000, 0xbe01e827 )
	ROM_LOAD( "ic07.cpu", 0x4000, 0x1000, 0x66f038dc )
	ROM_LOAD( "ic08.cpu", 0x5000, 0x1000, 0x89f3afb1 )
	ROM_LOAD( "ic09.cpu", 0x6000, 0x1000, 0x3cc9498f )
	ROM_LOAD( "ic10.cpu", 0x7000, 0x1000, 0x120a97b6 )
	ROM_LOAD( "ic14.cpu", 0x8000, 0x1000, 0xf3845850 )
	ROM_RELOAD(           0xf000, 0x1000 )	/* for the reset and interrupt vectors */
	ROM_LOAD( "ic15.cpu", 0x9000, 0x1000, 0xfa21e08f )
	ROM_LOAD( "ic16.cpu", 0xa000, 0x1000, 0x757af434 )
	ROM_LOAD( "ic17.cpu", 0xb000, 0x1000, 0xb5417a91 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic50.vid", 0x0000, 0x1000, 0xcfa87668 )
	ROM_LOAD( "ic51.vid", 0x1000, 0x1000, 0xf9730c57 )

	ROM_REGION(0x1000)	/* space for the sound ROMs */
	ROM_LOAD( "ic51.cpu", 0x0000, 0x0800, 0xf433dd21 )
	ROM_LOAD( "ic52.cpu", 0x0800, 0x0800, 0xd968fa48 )

/*	ROM_LOAD( "ic53.cpu", 0x????, 0x0800 ) ?? */
/*	ROM_LOAD( "ic07.dau", 0x????, 0x0800 ) ?? */
/*	ROM_LOAD( "ic08.dau", 0x????, 0x0800 ) ?? */
/*	ROM_LOAD( "ic11.dau", 0x????, 0x0800 ) ?? */
ROM_END




static int vanguard_hiload(void)     /* V.V */
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0025],"\x00\x10",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0025],3);
			osd_fread(f,&RAM[0x0220],112);
			osd_fread(f,&RAM[0x02a0],16);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */

}


static void vanguard_hisave(void)    /* V.V */
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0025],3);
		osd_fwrite(f,&RAM[0x0220],112);
		osd_fwrite(f,&RAM[0x02a0],16);
		osd_fclose(f);
	}
}



struct GameDriver vanguard_driver =
{
	"Vanguard",
	"vanguard",
        "BRIAN LEVINE\nBRAD OLIVER\nMIRKO BUFFONI\nANDREW SCOTT\nVALERIO VERRANDO",
	&machine_driver,

	vanguard_rom,
	0, 0,
        vanguard_sample_names,

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	vanguard_hiload, vanguard_hisave
};

struct GameDriver fantasy_driver =
{
	"Fantasy",
	"fantasy",
	"NICOLA SALMORIA\nBRIAN LEVINE\nMIRKO BUFFONI",
	&fantasy_machine_driver,

	fantasy_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	fantasy_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};


