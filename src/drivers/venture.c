/***************************************************************************

Venture memory map (preliminary)

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
               Press the key 7 to test

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

unsigned char *venture_ir2;


void venture_pos_change(void)
{
	int x,y;
	int cv[4];
	static int lcv0=0;
	static int lcv1=0;
	static int lcv2=0;
	static int lcv3=0;

	if (!(*venture_ir2&0x80))
	{
		x=(236 - *exidy_sprite1_xpos)>>3;
		y=(244 - *exidy_sprite1_ypos)>>3;
		cv[0]=videoram[(y<<5)+x];
		cv[1]=videoram[(y<<5)+x+1];
		cv[2]=videoram[((y+1)<<5)+x];
		cv[3]=videoram[((y+1)<<5)+x+1];

		if (cv[0] || cv[1] || cv[2] || cv[3])
		{
			if (	(cv[0]!=lcv0) ||
				(cv[1]!=lcv1) ||
				(cv[2]!=lcv2) ||
				(cv[3]!=lcv3))
			{
				*venture_ir2 = 0x80;
				lcv0=cv[0];
				lcv1=cv[1];
				lcv2=cv[2];
				lcv3=cv[3];
			}
		}
		else
			*venture_ir2 = 0x00;
	}

	return;
}

void venture_x_pos_change(int offset, int data)
{
	exidy_sprite1_xpos[offset]=data;
	venture_pos_change();
	return;
}

void venture_y_pos_change(int offset, int data)
{
	exidy_sprite1_ypos[offset]=data;
	venture_pos_change();
	return;
}


int venture_input_port_2_r(int offset)
{
	int value;

	/* Get 2 coin keys */
	value=input_port_2_r(offset) & 0x60;

	/* Combine with memory */
	value=value | (venture_ir2[offset] & 0x9F);

	/* Clear memory now that we've got it */
	venture_ir2[offset]=0x00;

	return value;
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r },	/* DSW */
	{ 0x5101, 0x5101, input_port_1_r },	/* IN0 */
	{ 0x5103, 0x5103, venture_input_port_2_r, &venture_ir2 },	/* IN1 */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fff, exidy_characterram_w, &exidy_characterram },
	{ 0x5000, 0x5000, venture_x_pos_change, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, venture_y_pos_change, &exidy_sprite1_ypos },
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


static struct InputPort input_ports[] =
{
	{	/* DSW */
		0xb0,
		{ OSD_KEY_4, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_RIGHT, OSD_KEY_LEFT,
				OSD_KEY_CONTROL, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_3 },
		{ 0, 0, OSD_JOY_RIGHT, OSD_JOY_LEFT,
				OSD_JOY_FIRE1, OSD_JOY_UP, OSD_JOY_DOWN, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, OSD_KEY_4, OSD_KEY_3, 0 },
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
	{ 1, 4, "FIRE BUTTON" },
        { -1 }
};




static struct DSW dsw[] =
{
	{ 0, 0x60, "LIVES", { "2", "3", "4", "5" } },
	{ 0, 0x06, "BONUS", { "20000", "30000", "40000", "50000" } },
	{ 0, 0x18, "COIN SELECT", { "1 COIN 4 CREDITS", "1 COIN 2 CREDITS", "2 COINS 1 CREDIT", "1 COIN 1 CREDIT" } },
	{ -1 }
};



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
	{ 0, 0x4800, &charlayout,   0,       6*2 },	/* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, (6*2)*2, 2*2 },  /* Sprites */
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
	0x80,0x80,0x80,   /* grey  */
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
		grey, orange, blue, red, purple, green, cyan, yellow, white
};

static unsigned char colortable[] =
{
	/* text */
	black, white,
	black, white,

	/* wall */
	black, purple,
	black, purple,

	/* goblin */
	black, cyan,
	black, cyan,

	/* Hall Monster */
	black, green,
	black, green,

	/* "?" box */
	black, cyan,
	black, cyan,

	/* door */
	black, white,
	black, white,

	/* Winky */
	black, red,
	black, red,

	/* Arrow */
	black, yellow,
	black, yellow,

};

enum
{
	c_text, c_wall, c_gbln, c_mstr, c_qbox, c_door
};

static unsigned char venture_color_lookup[] =
{
	/* 0x00-0x0F */
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	/* 0x10-0x1F */
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	/* 0x20-0x2F */
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	/* 0x30-0x3F */
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln, c_gbln,
	/* 0x40-0x4F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x50-0x5F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x60-0x6F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x70-0x7F */
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	c_text, c_text, c_text, c_text, c_text, c_text, c_text, c_text,
	/* 0x80-0x8F */
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	/* 0x90-0x9F */
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	/* 0xA0-0xAF */
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	/* 0xB0-0xBF */
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall, c_wall,
	/* 0xC0-0xCF */
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
	/* 0xD0-0xDF */
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
	/* 0xE0-0xEF */
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
	/* 0xF0-0xFF */
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
	c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr, c_mstr,
};


void venture_init_machine(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* Disable ROM Check for quicker startup */
	RAM[0x8AF4]=0xEA;
	RAM[0x8AF5]=0xEA;
	RAM[0x8AF6]=0xEA;

	/* Unlike MouseTrap and Pepper II, we have to do a CLD and an SEI ourselves. */
	/* To accomplish this, we put it in unused memory and force a jump to it. */
	/* VERY ugly!  */
	/* If we ever have the chance to play with the CPU flags at init time, this can */
	/* be replaced with P Reg = P Reg | I_FLAG & (~D_FLAG) ... */

	RAM[0x7000]=0xD8;
	RAM[0x7001]=0x78;
	RAM[0x7002]=0x4C;
	RAM[0x7003]=RAM[0xFFFC];
	RAM[0x7004]=RAM[0xFFFD];
	RAM[0xFFFC]=0x00;
	RAM[0xFFFD]=0x70;

	/* Set color lookup table to point to us */
	exidy_color_lookup=venture_color_lookup;
}




static struct MachineDriver venture_machine_driver =
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
	venture_init_machine,

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

ROM_START( venture_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "13A-CPU", 0x8000, 0x1000, 0x891abe62 )
	ROM_LOAD( "12A-CPU", 0x9000, 0x1000, 0xac004ea6 )
	ROM_LOAD( "11A-CPU", 0xA000, 0x1000, 0x225ec9ee )
	ROM_LOAD( "10A-CPU", 0xB000, 0x1000, 0x4c8a0c70 )
	ROM_LOAD( "9A-CPU",  0xC000, 0x1000, 0x4ec5e145 )
	ROM_LOAD( "8A-CPU",  0xD000, 0x1000, 0x25eac9e2 )
	ROM_LOAD( "7A-CPU",  0xE000, 0x1000, 0x2173eca5 )
	ROM_LOAD( "6A-CPU",  0xF000, 0x1000, 0x1714e61c )

	ROM_REGION(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11D-CPU", 0x0000, 0x0800, 0xceb42d02 )

	ROM_REGION(0x10000)	/* 64k for audio */
	ROM_LOAD( "3a-ac", 0x5800, 0x0800, 0x6098790a )
	ROM_LOAD( "4a-ac", 0x6000, 0x0800, 0x9bd6ad80 )
	ROM_LOAD( "5a-ac", 0x6800, 0x0800, 0xee5c9752 )
	ROM_LOAD( "6a-ac", 0x7000, 0x0800, 0x9559adbb )
	ROM_LOAD( "7a-ac", 0x7800, 0x0800, 0x9c5601b0 )
	ROM_LOAD( "7a-ac", 0xF800, 0x0800, 0x9c5601b0 )
ROM_END


static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0x0380],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x03A0],"DJS",3) == 0))
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
                        fread(&RAM[0x0380],1,5+6*5,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		/* 5 bytes for score order, 6 bytes per score/initials */
                fwrite(&RAM[0x0380],1,5+6*5,f);
		fclose(f);
	}

}




struct GameDriver venture_driver =
{
	"Venture",
	"venture",
	"MARC LAFONTAINE\nNICOLA SALMORIA\nBRIAN LEVINE\nMIKE BALFOUR",
	&venture_machine_driver,

	venture_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload,hisave
};

