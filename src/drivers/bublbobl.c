/***************************************************************************

Bobble Bobble memory map (preliminary)

CPU #1
0000-bfff ROM (8000-bfff is banked)
CPU #2
0000-7fff ROM

CPU #1 AND #2
c000-dcff Graphic RAM. This contains pointers to the video RAM columns and
          to the sprites are contained in Object RAM.
dd00-dfff Object RAM (groups of four bytes: X position, code [offset in the
          Graphic RAM], Y position, gfx bank)
e000-f7fe RAM
f800-f9ff Palette RAM
fc01-fdff RAM

read:
ff00      DSWA
ff01      DSWB
ff02      IN0
ff03      IN1

write:
fa80      watchdog reset
fc00      interrupt vector? (not needed by Bobble Bobble)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"

/* prototypes for functions in ../machine/bublbobl.c */
extern unsigned char *bublbobl_sharedram1,*bublbobl_sharedram2;
int bublbobl_interrupt(void);
int bublbobl_sharedram1_r(int offset);
int bublbobl_sharedram2_r(int offset);
void boblbobl_patch(void);
void bublbobl_patch(void);
void bublbobl_play_sound(int offset, int data);
void bublbobl_sharedram1_w(int offset, int data);
void bublbobl_sharedram2_w(int offset, int data);
void bublbobl_bankswitch_w(int offset, int data);

/* prototypes for functions in ../vidhrdw/bublbobl.c */
extern unsigned char *bublbobl_objectram;
extern unsigned char *bublbobl_paletteram;
extern int bublbobl_objectram_size;
void bublbobl_videoram_w(int offset,int data);
void bublbobl_objectram_w(int offset,int data);
void bublbobl_paletteram_w(int offset,int data);
void bublbobl_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int bublbobl_vh_start(void);
void bublbobl_vh_stop(void);
void bublbobl_vh_screenrefresh(struct osd_bitmap *bitmap);

int bublbobl_sh_interrupt(void);
int bublbobl_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x7fff, MRA_ROM },
    { 0x8000, 0xbfff, MRA_BANK1 },
    { 0xc000, 0xf9ff, bublbobl_sharedram1_r },
    { 0xfc01, 0xfdff, bublbobl_sharedram2_r },
    { 0xff00, 0xff00, input_port_0_r },
    { 0xff01, 0xff01, input_port_1_r },
    { 0xff02, 0xff02, input_port_2_r },
    { 0xff03, 0xff03, input_port_3_r },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, bublbobl_videoram_w, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, bublbobl_objectram_w, &bublbobl_objectram, &bublbobl_objectram_size },	/* handled by bublbobl_sharedram_w() */
	{ 0xf800, 0xf9ff, bublbobl_paletteram_w, &bublbobl_paletteram },
	{ 0xc000, 0xf9ff, bublbobl_sharedram1_w, &bublbobl_sharedram1 },
	{ 0xfa00, 0xfa00, sound_command_w },
	{ 0xfa80, 0xfa80, MWA_NOP },
	{ 0xfb40, 0xfb40, bublbobl_bankswitch_w },
	{ 0xfc01, 0xfdff, bublbobl_sharedram2_w, &bublbobl_sharedram2 },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_lvl[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
    { 0xc000, 0xf9ff, bublbobl_sharedram1_r },
    { 0xfc01, 0xfdff, bublbobl_sharedram2_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_lvl[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xcd00, 0xd4ff, bublbobl_videoram_w },
    { 0xc000, 0xf9ff, bublbobl_sharedram1_w },
    { 0xfc01, 0xfdff, bublbobl_sharedram2_w },
	{ -1 }  /* end of table */
};

#ifdef TRY_SOUND
static int pip(int offset)
{
	return 0x80;
}
static int pap(int offset)
{
	static int count;

	return count;
}

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, pap },
	{ 0x9001, 0x9001, AY8910_read_port_0_r },
	{ 0xa000, 0xa000, pip },
	{ 0xb000, 0xb000, sound_command_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* sound chip? */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, AY8910_control_port_0_w },
	{ 0x9001, 0x9001, AY8910_write_port_0_w },
	{ 0xa000, 0xa001, MWA_NOP },	/* sound chip? */
	{ 0xb000, 0xb001, MWA_NOP },	/* sound chip? */
	{ 0xb002, 0xb002, MWA_NOP },	/* interrupt enable/acknowledge? */
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
	{ -1 }	/* end of table */
};
#endif



static struct InputPort bublbobl_input_ports[] =
{
    {	/* 1: DSWA - FF00 */
	0xff,
	{ 0, 0, 0, 0, 0, 0, 0, 0 }, /* test mode */
	{ 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    {	/* 2: DSWB - FF01 */
	0x3f,
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    {	/* 3: IN0 - FF02 */
	0xff,
	{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_4, OSD_KEY_3,
	  OSD_KEY_ALT, OSD_KEY_LCONTROL, OSD_KEY_1, 0 },
	{ OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0,
	  OSD_JOY_UP, OSD_JOY_FIRE, 0, 0 }
    },
    {	/* 4: IN1 - FF03 */
	0xff,
	{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_T, OSD_KEY_5,
	  OSD_KEY_I, OSD_KEY_O, OSD_KEY_2, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    { -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
	{ -1 }
};

static struct KEYSet keys[] =
{
    { 2, 0, "P1 LEFT" },    { 2, 1, "P1 RIGHT" },
    { 2, 4, "P1 JUMP" },    { 2, 5, "P1 BUBBLE" },
    { 3, 0, "P2 LEFT" },    { 3, 1, "P2 RIGHT" },
    { 3, 4, "P2 JUMP" },    { 3, 5, "P2 BUBBLE" },
    { -1 }
};

static struct DSW dsw[] =
{
    { 1, 0x30, "LIVES", { "2", "1", "5", "3" } },
    { 1, 0x0c, "BONUS", { "50/250", "40/200", "20/80", "30/100" } },
    { 1, 0x03, "DIFFICULTY", { "VERY HARD", "HARD", "EASY", "NORMAL" } },
    { 0, 0x01, "LANGUAGE", { "ENGLISH", "JAPANESE" } },
    { 0, 0x30, "SLOT A", { "2C/3P", "2C/1P", "1C/2P", "1C/1P" } },
    { 0, 0xc0, "SLOT B", { "2C/3P", "2C/1P", "1C/2P", "1C/1P" } },
    { 0, 0x08, "ATTRACT SND", { "OFF", "ON" } },
    { 0, 0x04, "TEST MODE", { "ON", "OFF" },1 },
    { 0, 0x02, "REV SCREEN", { "ON", "OFF" },1 },
    { 1, 0xc0, "SPARE", { "A", "B", "C", "D" } },
    { -1 }
};

static struct DSW sboblbob_dsw[] =
{
    { 1, 0x30, "LIVES", { "2", "1", "5", "3" } },
    { 1, 0x0c, "BONUS", { "50/250", "40/200", "20/80", "30/100" } },
    { 1, 0x03, "DIFFICULTY", { "VERY HARD", "HARD", "EASY", "NORMAL" } },
    { 0, 0x01, "Game", { "Super Bobble Bobble", "Bobble Bobble" } },
    { 0, 0x30, "SLOT A", { "2C/3P", "2C/1P", "1C/2P", "1C/1P" } },
    { 0, 0xc0, "SLOT B", { "2C/3P", "2C/1P", "1C/2P", "1C/1P" } },
    { 0, 0x08, "ATTRACT SND", { "OFF", "ON" } },
    { 0, 0x04, "TEST MODE", { "ON", "OFF" },1 },
    { 0, 0x02, "REV SCREEN", { "ON", "OFF" },1 },
    { 1, 0xc0, "SPARE", { "A", "B", "C", "D" } },
    { -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* the characters are 8x8 pixels */
	256*8*6,			/* 256 characters per bank,
						* 8 banks per ROM pair,
						* 6 ROM pairs */
	4,	/* 4 bits per pixel */
	{ 0, 4, 6*0x8000*8, 6*0x8000*8+4 },
	{ 3, 2, 1, 0, 11, 10, 9, 8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 bytes in two ROMs */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* read all graphics into one big graphics region */
	{ 1, 0x00000, &charlayout, 0, 16 },
	{ -1 }	/* end of array */
};



static struct MachineDriver bublbobl_machine_driver =
{
    /* basic machine hardware */
    {				/* MachineCPU */
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			0,			/* memory_region */
			readmem,writemem,0,0,
			bublbobl_interrupt, 1
		},
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			2,			/* memory_region */
			readmem_lvl,writemem_lvl,0,0,
			interrupt, 1
		},
#ifdef TRY_SOUND
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz ??? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			bublbobl_sh_interrupt,1
		}
#endif
    },
    60,				/* frames_per_second */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
    0,		/* init_machine() */

    /* video hardware */
    32*8, 32*8,			/* screen_width, height */
    { 0, 32*8-1, 2*8, 30*8-1 }, /* visible_area */
    gfxdecodeinfo,
    256, /* total_colours */
    16*16,		/* color_table_len */
    bublbobl_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
    0,
	bublbobl_vh_start,
	bublbobl_vh_stop,
	bublbobl_vh_screenrefresh,

#ifdef TRY_SOUND
	0,
	0,
	bublbobl_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
#else
	0,0,0,0,0
#endif
};

static struct MachineDriver boblbobl_machine_driver =
{
    /* basic machine hardware */
    {				/* MachineCPU */
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			0,			/* memory_region */
			readmem,writemem,0,0,
			interrupt, 1	/* interrupt mode 1, unlike Bubble Bobble */
		},
		{
			CPU_Z80,
			6000000,		/* 6 Mhz??? */
			2,			/* memory_region */
			readmem_lvl,writemem_lvl,0,0,
			interrupt, 1
		},
#ifdef TRY_SOUND
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz ??? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			bublbobl_sh_interrupt,1
		}
#endif
    },
    60,				/* frames_per_second */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
    0,		/* init_machine() */

    /* video hardware */
    32*8, 32*8,			/* screen_width, height */
    { 0, 32*8-1, 2*8, 30*8-1 }, /* visible_area */
    gfxdecodeinfo,
    256, /* total_colours */
    16*16,		/* color_table_len */
    bublbobl_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
    0,
	bublbobl_vh_start,
	bublbobl_vh_stop,
	bublbobl_vh_screenrefresh,

#ifdef TRY_SOUND
	0,
	0,
	bublbobl_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
#else
	0,0,0,0,0
#endif
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bublbobl_rom )
    ROM_REGION(0x1c000)	/* 64k+64k for the first CPU */
    ROM_LOAD( "a78_06.bin", 0x00000, 0x8000, 0x54bbd2ad )
    ROM_LOAD( "a78_05.bin", 0x08000, 0x4000, 0x366354e5 )	/* banked at 8000-bfff. I must load */
	ROM_CONTINUE(           0x10000, 0xc000 )				/* bank 0 at 8000 because the code falls into */
															/* it from 7fff, so bank switching wouldn't work */
    ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "a78_09.bin", 0x00000, 0x8000, 0x4c2d6dc7 )
    ROM_LOAD( "a78_10.bin", 0x08000, 0x8000, 0x58e234c8 )
    ROM_LOAD( "a78_11.bin", 0x10000, 0x8000, 0x12787a3c )
    ROM_LOAD( "a78_12.bin", 0x18000, 0x8000, 0xcb0e4600 )
    ROM_LOAD( "a78_13.bin", 0x20000, 0x8000, 0x08e59a77 )
    ROM_LOAD( "a78_14.bin", 0x28000, 0x8000, 0xda100c26 )
    ROM_LOAD( "a78_15.bin", 0x30000, 0x8000, 0xe24b0f79 )
    ROM_LOAD( "a78_16.bin", 0x38000, 0x8000, 0x82f9f47b )
    ROM_LOAD( "a78_17.bin", 0x40000, 0x8000, 0x02935fdb )
    ROM_LOAD( "a78_18.bin", 0x48000, 0x8000, 0x41e639be )
    ROM_LOAD( "a78_19.bin", 0x50000, 0x8000, 0xd7b322af )
    ROM_LOAD( "a78_20.bin", 0x58000, 0x8000, 0xd2704b1e )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "a78_08.bin", 0x0000, 0x08000, 0xa11cdcf4 )

    ROM_REGION(0x10000)	/* 64k for the third CPU */
    ROM_LOAD( "a78_07.bin", 0x0000, 0x08000, 0x3eaa10b8 )
ROM_END

ROM_START( boblbobl_rom )
    ROM_REGION(0x1c000)	/* 64k+64k for the first CPU */
    ROM_LOAD( "bb3", 0x00000, 0x8000, 0x9202336e )
    ROM_LOAD( "bb5", 0x08000, 0x4000, 0x0d5aa1e0 )	/* banked at 8000-bfff. I must load */
	ROM_CONTINUE(    0x10000, 0x4000 )				/* bank 0 at 8000 because the code falls into */
													/* it from 7fff, so bank switching wouldn't work */
    ROM_LOAD( "bb4", 0x14000, 0x8000, 0xc6fc6192 )	/* banked at 8000-bfff */

    ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "bb6",  0x00000, 0x8000, 0x4c2d6dc7 )
    ROM_LOAD( "bb7",  0x08000, 0x8000, 0x58e234c8 )
    ROM_LOAD( "bb8",  0x10000, 0x8000, 0x12787a3c )
    ROM_LOAD( "bb9",  0x18000, 0x8000, 0xcb0e4600 )
    ROM_LOAD( "bb10", 0x20000, 0x8000, 0x08e59a77 )
    ROM_LOAD( "bb11", 0x28000, 0x8000, 0xda100c26 )
    ROM_LOAD( "bb12", 0x30000, 0x8000, 0xe24b0f79 )
    ROM_LOAD( "bb13", 0x38000, 0x8000, 0x82f9f47b )
    ROM_LOAD( "bb14", 0x40000, 0x8000, 0x02935fdb )
    ROM_LOAD( "bb15", 0x48000, 0x8000, 0x41e639be )
    ROM_LOAD( "bb16", 0x50000, 0x8000, 0xd7b322af )
    ROM_LOAD( "bb17", 0x58000, 0x8000, 0xd2704b1e )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "bb1", 0x0000, 0x08000, 0xa11cdcf4 )

    ROM_REGION(0x10000)	/* 64k for the third CPU */
    ROM_LOAD( "bb2", 0x0000, 0x08000, 0x3eaa10b8 )
ROM_END

ROM_START( sboblbob_rom )
    ROM_REGION(0x1c000)	/* 64k+64k for the first CPU */
    ROM_LOAD( "bbb-3.rom", 0x00000, 0x8000, 0xcbbab03c )
    ROM_LOAD( "bbb-5.rom", 0x08000, 0x4000, 0x0d5aa1e0 )	/* banked at 8000-bfff. I must load */
	ROM_CONTINUE(    0x10000, 0x4000 )				/* bank 0 at 8000 because the code falls into */
													/* it from 7fff, so bank switching wouldn't work */
    ROM_LOAD( "bbb-4.rom", 0x14000, 0x8000, 0x8e95270d )	/* banked at 8000-bfff */

    ROM_REGION(0x60000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "bbb-11.rom", 0x00000, 0x8000, 0x4c2d6dc7 )
    ROM_LOAD( "bbb-10.rom", 0x08000, 0x8000, 0x58e234c8 )
    ROM_LOAD( "bbb-9.rom",  0x10000, 0x8000, 0x12787a3c )
    ROM_LOAD( "bbb-8.rom",  0x18000, 0x8000, 0xcb0e4600 )
    ROM_LOAD( "bbb-7.rom",  0x20000, 0x8000, 0x08e59a77 )
    ROM_LOAD( "bbb-6.rom",  0x28000, 0x8000, 0xda100c26 )
    ROM_LOAD( "bbb-17.rom", 0x30000, 0x8000, 0xe24b0f79 )
    ROM_LOAD( "bbb-16.rom", 0x38000, 0x8000, 0x82f9f47b )
    ROM_LOAD( "bbb-15.rom", 0x40000, 0x8000, 0x02935fdb )
    ROM_LOAD( "bbb-14.rom", 0x48000, 0x8000, 0x41e639be )
    ROM_LOAD( "bbb-13.rom", 0x50000, 0x8000, 0xd7b322af )
    ROM_LOAD( "bbb-12.rom", 0x58000, 0x8000, 0xd2704b1e )

    ROM_REGION(0x10000)	/* 64k for the second CPU */
    ROM_LOAD( "bbb-1.rom", 0x0000, 0x08000, 0xa11cdcf4 )

    ROM_REGION(0x10000)	/* 64k for the third CPU */
    ROM_LOAD( "bbb-2.rom", 0x0000, 0x08000, 0x3eaa10b8 )
ROM_END



/* High Score memory, etc
 *
 *  E64C-E64E - the high score - initialised with points for first extra life
 *
 *  [ E64F-E653 - memory for copy protection routine ]
 *
 *  E654/5/6 score 1     E657 level     E658/9/A initials
 *  E65B/C/D score 2     E65E level     E65f/0/1 initials
 *  E662/3/4 score 3     E665 level     E666/7/8 initials
 *  E669/A/B score 4     E66C level     E66d/E/F initials
 *  E670/1/2 score 5     E673 level     E674/5/6 initials
 *
 *  E677 - position we got to in the high scores
 *  E67B - initialised to 1F - record level number reached
 *  E67C - initialised to 1F - p1's best level reached
 *  E67D - initialised to 13 - p2's best level reached
 */
static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe654],&RAM[0xe670],3) == 0 &&
			memcmp(&RAM[0xe654],&RAM[0xe64c],3) == 0 &&	/* high score */
			memcmp(&RAM[0xe67b],"\x1f\x1f\x13",3) == 0 &&	/* level number */
			(memcmp(&RAM[0xe654],"\x00\x20\x00",3) == 0 ||
			memcmp(&RAM[0xe654],"\x00\x30\x00",3) == 0 ||
			memcmp(&RAM[0xe654],"\x00\x40\x00",3) == 0 ||
			memcmp(&RAM[0xe654],"\x00\x50\x00",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xe654],7*5);
			osd_fread(f,&RAM[0xe67b],3);
			RAM[0xe64c] = RAM[0xe654];
			RAM[0xe64d] = RAM[0xe655];
			RAM[0xe64e] = RAM[0xe656];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xe654],7*5);
		osd_fwrite(f,&RAM[0xe67b],3);
		osd_fclose(f);
	}
}



struct GameDriver bublbobl_driver =
{
	"Bubble Bobble",
	"bublbobl",
	"CHRIS MOORE\nOLIVER WHITE\nNICOLA SALMORIA",
	&bublbobl_machine_driver,

	bublbobl_rom,
	bublbobl_patch, 0,	/* remove protection */
	0,

	bublbobl_input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver boblbobl_driver =
{
	"Bobble Bobble (bootleg Bubble Bobble)",
	"boblbobl",
	"CHRIS MOORE\nOLIVER WHITE\nNICOLA SALMORIA",
	&boblbobl_machine_driver,

	boblbobl_rom,
	boblbobl_patch, 0,	/* remove protection */
	0,

	bublbobl_input_ports, 0, trak_ports, dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver sboblbob_driver =
{
	"Bobble Bobble (bootleg Bubble Bobble, alternate version)",
	"sboblbob",
	"CHRIS MOORE\nOLIVER WHITE\nNICOLA SALMORIA",
	&boblbobl_machine_driver,

	sboblbob_rom,
	0, 0,
	0,

	bublbobl_input_ports, 0, trak_ports, sboblbob_dsw, keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
