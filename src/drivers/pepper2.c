/***************************************************************************

Pepper II memory map (preliminary)

0000-03ff RAM
4000-43ff Video ram
6000-6fff Character generator RAM (512)
8000-ffff ROM

read:
5100      DSW (inverted)
          bit 0  coin 2 (NOT inverted) (must activate together with IN1 bit 5)
          bit 1-2  bonus
          bit 3-4  coins per play
          bit 5-6  lives
          bit 7  US/UK coins
5101      IN0 (inverted)
          bit 0  start 1
		  bit 1  start 2
		  bit 2  right
		  bit 3  left
		  bit 5  up
		  bit 6  down
		  bit 7  coin 1 (must activate together with IN1 bit 6)
5103      IN1

		  bit 5  coin 2 (must activate together with DSW bit 0)
		  bit 6  coin 1 (must activate together with IN0 bit 7)
5200      ?
5201      ?
5202      ?
5203      ?

write:
5000      Sprite #1 X position
5040      Sprite #1 Y position
5080      Sprite #2 X position
50c0      Sprite #2 Y position

5100      Sprite number  bits 0-3 Sprite #1  4-7 Sprite #2
5101      Sprites bank
              bit 7  Bank for sprite #1  (Not sure!)
              bit 6  Bank for sprite #2  (Sure, see the bird in Mouse Trap)

5200-5203  Probably has something to do with the colors?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/* These are defined in vidhrdw/exidy.c */
extern unsigned char *exidy_characterram;
extern unsigned char *exidy_color_lookup;
void exidy_characterram_w(int offset,int data);
void exidy_vh_screenrefresh(struct osd_bitmap *bitmap);

extern unsigned char *exidy_sprite_no;
extern unsigned char *exidy_sprite_enable;
extern unsigned char *exidy_sprite1_xpos;
extern unsigned char *exidy_sprite1_ypos;
extern unsigned char *exidy_sprite2_xpos;
extern unsigned char *exidy_sprite2_ypos;


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r },	/* DSW */
	{ 0x5101, 0x5101, input_port_1_r },	/* IN0 */
	{ 0x5103, 0x5103, input_port_2_r },	/* IN1 */
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x7000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x6000, 0x6fff, exidy_characterram_w, &exidy_characterram },
	{ 0x7000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x67ff, MRA_RAM },
	{ 0x6800, 0x7fff, MRA_ROM },
	{ 0x8000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x67ff, MWA_RAM },
	{ 0x6800, 0x7fff, MWA_ROM },
	{ 0x8000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* DSW */
		0xb0,
		{ OSD_KEY_4, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5, OSD_KEY_5 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_RIGHT, OSD_KEY_LEFT,
				0, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_3 },
		{ 0, 0, OSD_JOY_RIGHT, OSD_JOY_LEFT,
				0, OSD_JOY_UP, OSD_JOY_DOWN, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, OSD_KEY_4, OSD_KEY_3, 0},
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
        { 1, 5, "MOVE UP" },
        { 1, 3, "MOVE LEFT"  },
        { 1, 2, "MOVE RIGHT" },
        { 1, 6, "MOVE DOWN" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 0, 0x60, "LIVES", { "5", "4", "3", "2" } },
	{ 0, 0x06, "BONUS", { "50000", "40000", "30000", "20000" } },
	{ 0, 0x18, "COIN SELECT", { "1 COIN 4 CREDITS", "1 COIN 2 CREDITS", "2 COINS 1 CREDIT", "1 COIN 1 CREDIT" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 }, /* 2 bits separated by 0x0800 bytes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	16*4,	/* 8 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	8*32	/* every char takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x6000, &charlayout,   0,       10*2 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, (10*2)*4, 2*2 },  /* Angel/Devil/Zipper Ripper */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black      */
	0x80,0x00,0x80,   /* darkpurple */
	0x80,0x00,0x00,   /* darkred    */
	0xf8,0x64,0xd8,   /* pink       */	/* bad guys */
	0x00,0x80,0x00,   /* darkgreen  */	/* "Player 1" */
	0x00,0x80,0x80,   /* darkcyan   */
	0x80,0x80,0x00,   /* darkyellow */
	0x80,0x80,0x80,   /* darkwhite  */
	0xf8,0x94,0x44,   /* orange     */
	0x00,0x00,0xff,   /* blue   */		/* maze primary, bonus objects */
	0xff,0x00,0x00,   /* red    */		/* maze secondary, bonus objects */
	0xff,0x00,0xff,   /* purple */		/* changed maze secondary */
	0x00,0xff,0x00,   /* green  */		/* pitchfork, changed maze primary */
	0x00,0xff,0xff,   /* cyan   */		/* changed bad guys */
	0xff,0xff,0x00,   /* yellow */		/* angel, bonus objects */
	0xff,0xff,0xff    /* white  */		/* text, devil */
};

enum
{
	black, darkpurple, darkred, pink, darkgreen, darkcyan, darkyellow,
		darkwhite, orange, blue, red, purple, green, cyan, yellow, white
};

static unsigned char colortable[] =
{
	/* text colors */
	black, black, black, white,
	black, black, black, white,

	/* maze path colors */
	black, black, blue, red,
	black, black, green, purple,

	/* Roaming Eyes */
	black, black, blue, red,
	black, black, green, purple,

	/* unused Angels at top of screen */
	black, black, yellow, yellow,
	black, black, yellow, yellow,

	/* box fill */
	black, black, blue, white,
	black, black, white, blue,

	/* pitchfork */
	black, black, green, yellow,
	black, black, green, yellow,

	/* "Pepper II" */
	black, black, yellow, yellow,
	black, black, yellow, yellow,

	/* Maze number boxes */
	black, red, yellow, blue,
	black, red, yellow, blue,

	/* Exit signs */
	black, red, blue, white,
	black, red, blue, white,

	/* Bonus objects */
	black, blue, red, yellow,
	black, blue, red, yellow,

	/* Angel/Devil */
	black, yellow, black, white,

	/* Zipper Ripper */
	black, green, black, green,

};

enum
{
	c_text, c_path, c_eyes, c_angl, c_fill, c_fork, c_ppr2, c_mbox,
		c_exit, c_bnus
};

static unsigned char pepper2_color_lookup[] =
{
	/* 0x00-0x0F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x10-0x1F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x20-0x2F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x30-0x3F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x40-0x4F */
	c_path, c_path, c_path, c_path, c_path, c_path, c_path, c_path,
	c_path, c_path, c_path, c_path, c_path, c_path, c_path, c_path,
	/* 0x50-0x5F */
	c_path, c_path, c_path, c_path, c_path, c_path, c_eyes, c_eyes,
	c_eyes, c_eyes, c_eyes, c_eyes, c_eyes, c_eyes, c_eyes, c_eyes,
	/* 0x60-0x6F */
	c_eyes, c_eyes, c_eyes, c_eyes, c_eyes, c_eyes, c_eyes, c_eyes,
	c_eyes, c_angl, c_angl, c_angl, c_angl, c_angl, c_angl, c_angl,
	/* 0x70-0x7F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x80-0x8F */
	c_fill, c_fork, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2,
	c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2,
	/* 0x90-0x9F */
	c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2,
	c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2, c_ppr2,
	/* 0xA0-0xAF */
	c_ppr2, c_fork, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0xB0-0xBF */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0xC0-0xCF */
	c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox,
	c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox,
	/* 0xD0-0xDF */
	c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox, c_mbox,
	c_exit, c_exit, c_exit, c_exit, c_exit, c_exit, c_bnus, c_bnus,
	/* 0xE0-0xEF */
	c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus,
	c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus,
	/* 0xF0-0xFF */
	c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus,
	c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus, c_bnus,
};



void pepper2_init_machine(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* Disable ROM Check for quicker startup */
	RAM[0xF52D]=0xEA;
	RAM[0xF52E]=0xEA;
	RAM[0xF52F]=0xEA;

	/* Set color lookup table to point to us */
	exidy_color_lookup=pepper2_color_lookup;
}


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	pepper2_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	exidy_vh_screenrefresh,

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

ROM_START( pepper2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "main_12a", 0x9000, 0x1000, 0x0a47e1e3 )
	ROM_LOAD( "main_11a", 0xA000, 0x1000, 0x120e0da0 )
	ROM_LOAD( "main_10a", 0xB000, 0x1000, 0x4ab11dc7 )
	ROM_LOAD( "main_9a",  0xC000, 0x1000, 0xa6f0e57e )
	ROM_LOAD( "main_8a",  0xD000, 0x1000, 0x04b6fd2a )
	ROM_LOAD( "main_7a",  0xE000, 0x1000, 0x9e350147 )
	ROM_LOAD( "main_6a",  0xF000, 0x1000, 0xda172fe9 )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "main_11d", 0x0000, 0x0800, 0x8ae4f8ba )

	ROM_REGION(0x10000)	/* 64k for audio */
	ROM_LOAD( "audio_5a", 0x6800, 0x0800, 0x96de524a )
	ROM_LOAD( "audio_6a", 0x7000, 0x0800, 0xf2685a2c )
	ROM_LOAD( "audio_7a", 0x7800, 0x0800, 0xe5a6f8ec )
	ROM_LOAD( "audio_7a", 0xF800, 0x0800, 0xe5a6f8ec )
ROM_END


static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0360],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x0380],"\x15\x20\x11",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x0360],5+6*5);
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
		/* 5 bytes for score order, 6 bytes per score/initials */
                osd_fwrite(f,&RAM[0x0360],5+6*5);
		osd_fclose(f);
	}

}




struct GameDriver pepper2_driver =
{
	"Pepper II",
	"pepper2",
	"MARC LAFONTAINE\nBRIAN LEVINE\nMIKE BALFOUR",
        &machine_driver,

	pepper2_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload,hisave
};
