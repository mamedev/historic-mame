/***************************************************************************

Jump Bug memory map (preliminary)

0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
4c00-4fff mirror address for video RAM
5000-50ff Object RAM
  5000-503f  screen attributes
  5040-505f  sprites
  5060-507f  bullets?
  5080-50ff  unused?
8000-afff ROM

read:
6000      IN0
6800      IN1
7000      IN2 ?
*
 * IN0 (all bits are inverted)
 * bit 7 : DOWN player 1
 * bit 6 : UP player 1
 * bit 5 : ?
 * bit 4 : SHOOT player 1
 * bit 3 : LEFT player 1
 * bit 2 : RIGHT player 1
 * bit 1 : DOWN player 2
 * bit 0 : COIN A
*
 * IN1 (bits 2-7 are inverted)
 * bit 7 : UP player 2
 * bit 6 : Difficulty: 1 Easy, 0 Hard (according to JBE v0.5)
 * bit 5 : COIN B
 * bit 4 : SHOOT player 2
 * bit 3 : LEFT player 2
 * bit 2 : RIGHT player 2
 * bit 1 : START 2 player
 * bit 0 : START 1 player
*
 * DSW (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : ?
 * bit 3 : \ coins: 11; coin a 1 credit, coin b 6 credits
 * bit 2 : /        10; coin a 1/2 credits, coin b 3 credits credits
                    01; coin a 1/2 credits, coin b 1/2
                    00; coin a 1 credit, coin b 1 credit
 * bit 1 : \ lives: 11; 5 cars, 10; 4 cars,
 * bit 0 : /        01; 3 cars, 00; Free Play


write:
5800      8910 write port
5900      8910 control port
6002-6006 gfx bank select - see vidhrdw/jumpbug.c for details (6005 seems to be unused)
7001      interrupt enable
7002      coin counter ????
7003      ?
7004      stars on (?)
7005      ?
7006      screen vertical flip ????
7007      screen horizontal flip ????
7800      watchdog reset

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *jumpbug_attributesram;
extern unsigned char *jumpbug_gfxbank;
extern unsigned char *jumpbug_stars;
void jumpbug_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void jumpbug_attributes_w(int offset,int data);
void jumpbug_gfxbank_w(int offset,int data);
void jumpbug_stars_w(int offset,int data);
int jumpbug_vh_start(void);
void jumpbug_vh_screenrefresh(struct osd_bitmap *bitmap);

int jumpbug_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM, Video RAM */
	{ 0x4c00, 0x4fff, videoram_r },	/* mirror address for Video RAM*/
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xafff, MRA_ROM },
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* IN2 */
        { 0xeff0, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4c00, 0x4fff, videoram_w },	/* mirror address for Video RAM */
	{ 0x5000, 0x503f, jumpbug_attributes_w, &jumpbug_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7004, 0x7004, MWA_RAM, &jumpbug_stars },
	{ 0x6002, 0x6006, jumpbug_gfxbank_w, &jumpbug_gfxbank },
	{ 0x5900, 0x5900, AY8910_control_port_0_w },
	{ 0x5800, 0x5800, AY8910_write_port_0_w },
	{ 0x7800, 0x7800, MWA_NOP },
        { 0xeff0, 0xefff, MWA_RAM },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0xafff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ 0, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_LCONTROL, OSD_KEY_ALT, OSD_KEY_DOWN, OSD_KEY_UP },
		{ 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, OSD_JOY_DOWN, OSD_JOY_UP }
	},
	{	/* IN1 */
		0x40,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, OSD_KEY_3, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x01,
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
        { 0, 7, "MOVE UP" },
        { 0, 2, "MOVE LEFT"  },
        { 0, 3, "MOVE RIGHT" },
        { 0, 6, "MOVE DOWN" },
        { 0, 4, "FIRE" },
        { 0, 5, "JUMP" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "UNLIMITED", "3", "4", "5" } },
	{ 1, 0x40, "DIFFICULTY", { "HARD", "EASY" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	768,	/* 768 characters */
	2,	/* 2 bits per pixel */
	{ 0, 768*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	192,	/* 192 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 192*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0x17,0xC7,0xF6,0x00,0x17,0xC0,0x3F,0x00,0x07,0xC0,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC7,0x31,0x17,0x00,0x31,0xC7,0x3F,0x00,0xF6,0x07,0xF0,0x00,0x3F,0x07,0xC4
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
			nmi_interrupt,1
		}
	},
	60,
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32+64, 32,	/* 32 for the characters, 64 for the stars */
	jumpbug_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	jumpbug_vh_start,
	generic_vh_stop,
	jumpbug_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	jumpbug_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jumpbug_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jb1", 0x0000, 0x1000, 0x79963f12 )
	ROM_LOAD( "jb2", 0x1000, 0x1000, 0xa43443aa )
	ROM_LOAD( "jb3", 0x2000, 0x1000, 0xbaf5db29 )
	ROM_LOAD( "jb4", 0x3000, 0x1000, 0x48f5b817 )
	ROM_LOAD( "jb5", 0x8000, 0x1000, 0x817144dd )
	ROM_LOAD( "jb6", 0x9000, 0x1000, 0x58440d82 )
	ROM_LOAD( "jb7", 0xa000, 0x1000, 0xef8ff5ab )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jbi", 0x0000, 0x0800, 0x5857c05d )
	ROM_LOAD( "jbj", 0x0800, 0x0800, 0xdd2b5d5b )
	ROM_LOAD( "jbk", 0x1000, 0x0800, 0x53ddcbb5 )
	ROM_LOAD( "jbl", 0x1800, 0x0800, 0x66b19093 )
	ROM_LOAD( "jbm", 0x2000, 0x0800, 0x7bac88f8 )
	ROM_LOAD( "jbn", 0x2800, 0x0800, 0x3bdc2b52 )
ROM_END

ROM_START( jbugsega_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jb1.prg", 0x0000, 0x4000, 0xecb42886 )
	ROM_LOAD( "jb2.prg", 0x8000, 0x2800, 0x14302ed6 )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jb3.gfx", 0x0000, 0x3000, 0xa798658a )
ROM_END



static void jumpbug_decode(void)
{
	/* this is not a "decryption", it is just a protection removal */
	RAM[0x265a] = 0xc9;
	RAM[0x8a16] = 0xc9;
	RAM[0x8dae] = 0xc9;
	RAM[0x8dbe] = 0xc3;
	RAM[0x8dd7] = 0x18;
	RAM[0x9f3d] = 0x18;
	RAM[0x9f53] = 0xc3;
}



static int hiload(void)
{

        if (memcmp(&RAM[0x4208],"\x00\x00\x00\x05",4) == 0 &&
            memcmp(&RAM[0x4233],"\x97\x97\x97\x97",4) ==0)

        {

                void *f;

                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0x4208],6);
                        osd_fread(f,&RAM[0x4222],3*7);
                        osd_fclose(f);
                }
                return 1;
        }

        else return 0;  /* we can't load the hi scores yet */

}


static void hisave(void)
{
	void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0x4208],6);
                osd_fwrite(f,&RAM[0x4222],3*7);
                osd_fclose(f);
        }
}



struct GameDriver jumpbug_driver =
{
	"Jump Bug",
	"jumpbug",
	"RICHARD DAVIES\nBRAD OLIVER\nNICOLA SALMORIA\nJuan Carlos Lorente",
	&machine_driver,

	jumpbug_rom,
	jumpbug_decode, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver jbugsega_driver =
{
	"Jump Bug (bootleg)",
	"jbugsega",
	"RICHARD DAVIES\nBRAD OLIVER\nNICOLA SALMORIA\nJuan Carlos Lorente",
	&machine_driver,

	jbugsega_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
