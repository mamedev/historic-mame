/***************************************************************************

MouseTrap memory map (preliminary)

0000-03ff RAM
4000-43ff Video ram
4800-4fff Character generator RAM
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
		  bit 4  fire
		  bit 5  up
		  bit 6  down
		  bit 7  coin 1 (must activate together with IN1 bit 6)
5103      IN1
          bit 2  Must be 1 to have collision enabled?
               Probably Collision between sprite and background.

          bit 4  Probably Collision flag between sprites
               Press the key 8 to test

          bit 5  coin 2 (must activate together with DSW bit 0)
          bit 6  coin 1 (must activate together with IN0 bit 7)
          other bits ?
5200      ?
5201      ?
5202      ?
5203      ?
5213      IN2 (Mouse Trap)
          bit 3  blue button
          bit 2  free play
          bit 1  red button
          bit 0  yellow button

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



unsigned char *mtrap_i2r;

int mtrap_input_port_3_r(int offset)
{
	int res;


	/* Start with what's in memory */
	res = *mtrap_i2r & 0x8b;

	/* get coin 1 status */
	if ((readinputport(1) & 0x80) == 0) res |= 0x40;

	/* get coin 2 status */
	if ((readinputport(0) & 0x01) != 0) res |= 0x20;

	/* Character collision */
	if (!(*exidy_sprite1_xpos == 0x78 && *exidy_sprite1_ypos == 0x38))
		res |= 0x04;

	/* Sprite collision */
	if (!((*exidy_sprite1_xpos + 0x10 < *exidy_sprite2_xpos) ||
			(*exidy_sprite1_xpos > *exidy_sprite2_xpos + 0x10) ||
			(*exidy_sprite1_ypos + 0x10 < *exidy_sprite2_ypos) ||
			(*exidy_sprite1_ypos > *exidy_sprite2_ypos + 0x10) ||
			(*exidy_sprite_enable&0x40)))
		res |= 0x10;

	return res;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r },	/* DSW */
	{ 0x5101, 0x5101, input_port_1_r },	/* IN0 */
	{ 0x5103, 0x5103, mtrap_input_port_3_r, &mtrap_i2r },
	{ 0x5213, 0x5213, input_port_2_r },	/* IN1 */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fff, exidy_characterram_w, &exidy_characterram },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x7fff, MRA_ROM },
	{ 0x8000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x57ff, MWA_RAM },
	{ 0x5800, 0x7fff, MWA_ROM },
	{ 0x8000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START	/* DSW0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "30000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x02, "50000" )
	PORT_DIPSETTING(    0x00, "60000" )
	PORT_DIPNAME( 0x98, 0x98, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x98, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x88, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x00, "Coin A 2/1 Coin B 1/3" )
	PORT_DIPSETTING(    0x08, "Coin A 1/3 Coin B 2/7" )
	PORT_DIPSETTING(    0x10, "Coin A 1/1 Coin B 1/4" )
	PORT_DIPSETTING(    0x18, "Coin A 1/1 Coin B 1/5" )
	PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME, "Free Play", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bits per pixel */
	{ 0 }, /* No info needed for bit offsets */
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
	{ 0, 0x4800, &charlayout,   0,       13*2 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, (13*2)*2, 2*2 },  /* Sprites */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black      */
	0x80,0x00,0x80,   /* darkpurple */
	0x80,0x00,0x00,   /* darkred    */
	0xf8,0x64,0xd8,   /* pink       */
	0x00,0x80,0x00,   /* darkgreen  */
	0x00,0x80,0x80,   /* darkcyan   */
	0x80,0x80,0x00,   /* darkyellow */
	0x80,0x80,0x80,   /* darkwhite  */
	0xf8,0x94,0x44,   /* orange     */
	0x00,0x00,0xff,   /* blue   */
	0xff,0x00,0x00,   /* red    */
	0xff,0x00,0xff,   /* purple */
	0x00,0xff,0x00,   /* green  */
	0x00,0xff,0xff,   /* cyan   */
	0xff,0xff,0x00,   /* yellow */
	0xff,0xff,0xff    /* white  */
};

enum
{
	black, darkpurple, darkred, pink, darkgreen, darkcyan, darkyellow,
		darkwhite, orange, blue, red, purple, green, cyan, yellow, white
};

static unsigned char colortable[] =
{
	/* text colors */
	black, green,
	black, green,

	/* maze path colors */
	black, green,
	black, green,

	/* dot colors */
	black, yellow,
	black, yellow,

	/* cat colors */
	black, yellow,
	black, yellow,

	/* "IN" box */
	purple, white,
	purple, white,

	/* Red door */
	black, red,
	black, red,

	/* blue door */
	black, blue,
	black, blue,

	/* yellow door */
	black, yellow,
	black, yellow,

	/* bonus objects */
	black, red,
	black, red,

	/* dog biscuit */
	black, red,
	black, red,

	/* extra mouse */
	black, cyan,
	black, cyan,

	/* hawk (for instructions) */
	black, purple,
	black, purple,

	/* dog (for instructions) */
	black, red,
	black, red,

	/* Mouse/Dog */
	black, cyan,
	black, red,

	/* Hawk */
	black, purple,
	black, purple,

};

enum
{
	c_text, c_path, c_dots, c_cats, c_ibox, c_rddr, c_bldr, c_yldr,
	c_bnus, c_bsct, c_xmse, c_hawk, c_dogs
};

static unsigned char mtrap_color_lookup[] =
{
	/* 0x00-0x0F */
	c_bldr, c_bldr, c_bldr, c_bldr, c_ibox, c_ibox, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x10-0x1F */
	c_text, c_text, c_text, c_text, c_hawk, c_hawk, c_hawk, c_hawk,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x20-0x2F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x30-0x3F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x40-0x4F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x50-0x5F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_path, c_path, c_path,
	/* 0x60-0x6F */
	c_path, c_path, c_path, c_path, c_path, c_path, c_path, c_path,
	c_path, c_path, c_path, c_path, c_path, c_path, c_path, c_path,
	/* 0x70-0x7F */
	c_path, c_path, c_path, c_path, c_path, c_path, c_path, c_path,
	c_path, c_path, c_path, c_path, c_path, c_path, c_path, c_path,
	/* 0x80-0x8F */
	c_rddr, c_rddr, c_rddr, c_rddr, c_bsct, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x90-0x9F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0xA0-0xAF */
	c_text, c_text, c_bnus, c_bnus, c_bnus, c_bnus, c_dogs, c_dogs,
	c_dogs, c_dogs, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0xB0-0xBF */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0xC0-0xCF */
	c_yldr, c_yldr, c_dots, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0xD0-0xDF */
	c_text, c_text, c_yldr, c_yldr, c_xmse, c_xmse, c_xmse, c_xmse,
	c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats,
	/* 0xE0-0xEF */
	c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats,
	c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats,
	/* 0xF0-0xFF */
	c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats,
	c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats, c_cats,
};



void mtrap_init_machine(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* Disable ROM Check for quicker startup */
	RAM[0xF439]=0xEA;
	RAM[0xF43A]=0xEA;
	RAM[0xF43B]=0xEA;

	/* Set color lookup table to point to us */
	exidy_color_lookup=mtrap_color_lookup;
}




static struct MachineDriver mtrap_machine_driver =
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
			CPU_M6502 | CPU_AUDIO_CPU,
			1000000,	/* 1 Mhz ???? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	mtrap_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
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
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mtrap_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mtl11a.bin", 0xA000, 0x1000, 0xb4e109f7 )
	ROM_LOAD( "mtl10a.bin", 0xB000, 0x1000, 0xe890bac6 )
	ROM_LOAD( "mtl9a.bin",  0xC000, 0x1000, 0x06628e86 )
	ROM_LOAD( "mtl8a.bin",  0xD000, 0x1000, 0xa12b0c55 )
	ROM_LOAD( "mtl7a.bin",  0xE000, 0x1000, 0xb5c75a2f )
	ROM_LOAD( "mtl6a.bin",  0xF000, 0x1000, 0x2e7f499b )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mtl11D.bin", 0x0000, 0x0800, 0x389ef2ec )

	ROM_REGION(0x10000)	/* 64k for audio */
	ROM_LOAD( "mta5a.bin", 0x6800, 0x0800, 0xdc40685a )
	ROM_LOAD( "mta6a.bin", 0x7000, 0x0800, 0x250b2f5f )
	ROM_LOAD( "mta7a.bin", 0x7800, 0x0800, 0x3ba2700a )
	ROM_LOAD( "mta7a.bin", 0xF800, 0x0800, 0x3ba2700a )
ROM_END



static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0x0380],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x03A0],"LWH",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x0380],5+6*5);
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
                osd_fwrite(f,&RAM[0x0380],5+6*5);
		osd_fclose(f);
	}

}


struct GameDriver mtrap_driver =
{
	"Mouse Trap",
	"mtrap",
	"Marc LaFontaine\nBrian Levine\nMike Balfour\nMarco Cassili",
	&mtrap_machine_driver,

	mtrap_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	0/*TBR*/, input_ports, 0/*TBR*/, 0/*TBR*/, 0/*TBR*/,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload,hisave
};
