/***************************************************************************

Frogger memory map (preliminary)

0000-3fff ROM
8000-87ff RAM
a800-abff Video RAM
b000-b0ff Object RAM
b000-b03f screen attributes
b040-b05f sprites
b060-b0ff unused?

read:
8800      Watchdog Reset
e000      IN0
e002      IN1
e004      IN2

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : SHOOT 1 player 1
 * bit 2 : CREDIT
 * bit 1 : SHOOT 2 player 1
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : LEFT player 2 (TABLE only)
 * bit 4 : RIGHT player 2 (TABLE only)
 * bit 3 : SHOOT 1 player 2 (TABLE only)
 * bit 2 : SHOOT 2 player 2 (TABLE only)
 * bit 1 :\ nr of lives
 * bit 0 :/ 00 = 3  01 = 5  10 = 7  11 = 256
*
 * IN2 (all bits are inverted)
 * bit 7 : unused
 * bit 6 : DOWN player 1
 * bit 5 : unused
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/
 * bit 0 : DOWN player 2 (TABLE only)
 *

write:
b808      interrupt enable
b80c      screen horizontal flip
b810      screen vertical flip
b818      coin counter 1
b81c      coin counter 2
d000      To AY-3-8910 port A (commands for the second Z80)
d002      trigger interrupt on sound CPU


SOUND BOARD:
0000-17ff ROM
4000-43ff RAM

I/0 ports:
read:
40      8910 #1  read

write
40      8910 #1  write
80      8910 #1  control

interrupts:
interrupt mode 1 triggered by the main CPU

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *frogger_attributesram;
extern void frogger_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void frogger_attributes_w(int offset,int data);
extern void frogger_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int frogger_sh_interrupt(void);
extern int frogger_sh_init(const char *gamename);
extern int frogger_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa800, 0xabff, MRA_RAM },	/* video RAM */
	{ 0xb000, 0xb05f, MRA_RAM },	/* screen attributes, sprites */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8800, 0x8800, MRA_NOP },
	{ 0xe000, 0xe000, input_port_0_r },	/* IN0 */
	{ 0xe002, 0xe002, input_port_1_r },	/* IN1 */
	{ 0xe004, 0xe004, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa800, 0xabff, videoram_w, &videoram },
	{ 0xb000, 0xb03f, frogger_attributes_w, &frogger_attributesram },
	{ 0xb040, 0xb05f, MWA_RAM, &spriteram },
	{ 0xb808, 0xb808, interrupt_enable_w },
	{ 0xd000, 0xd000, sound_command_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x0000, 0x17ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x0000, 0x17ff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort sound_readport[] =
{
	{ 0x40, 0x40, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x80, 0x80, AY8910_control_port_0_w },
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ 0, 0, OSD_KEY_3, 0, OSD_KEY_RIGHT, OSD_KEY_LEFT, 0, 0 },
		{ 0, 0, 0, 0, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN1 */
		0xfc,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xf7,
		{ 0, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ 0, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 2, 4, "MOVE UP" },
        { 0, 5, "MOVE LEFT"  },
        { 0, 4, "MOVE RIGHT" },
        { 2, 6, "MOVE DOWN" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "5", "7", "256" } },
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
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 16 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



#define BLACK 0x00
#define LTGREEN 0x3c
#define DKRED 0x17
#define DKBROWN 0x5c
#define DKPINK 0xd7
#define LTBROWN 0x5e
#define PURPLE 0xc4
#define BLUE 0xc0
#define RED 0x07
#define MAGENTA 0xc7
#define GREEN 0x39
#define CYAN 0xf8
#define YELLOW 0x3f
#define WHITE 0xf6

static unsigned char color_prom[] =
{
	/* palette */
	BLACK,BLUE,RED,WHITE,
	BLACK,DKRED,GREEN,CYAN,
	BLACK,LTBROWN,WHITE,DKBROWN,
	BLACK,MAGENTA,GREEN,YELLOW,
	BLACK,LTGREEN,CYAN,DKPINK,
	BLACK,RED,WHITE,GREEN,
	BLACK,PURPLE,BLUE,RED,
	BLACK,RED,YELLOW,PURPLE
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
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			frogger_sh_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32,64,
	frogger_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	frogger_vh_screenrefresh,

	/* sound hardware */
	0,
	frogger_sh_init,
	frogger_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( frogger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "frogger.ic5", 0x0000, 0x1000 )
	ROM_LOAD( "frogger.ic6", 0x1000, 0x1000 )
	ROM_LOAD( "frogger.ic7", 0x2000, 0x1000 )
	ROM_LOAD( "frogger.ic8", 0x3000, 0x1000 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "frogger.606", 0x0000, 0x0800 )
	ROM_LOAD( "frogger.607", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608", 0x0000, 0x0800 )
	ROM_LOAD( "frogger.609", 0x0800, 0x0800 )
	ROM_LOAD( "frogger.610", 0x1000, 0x0800 )
ROM_END

ROM_START( frogsega_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "frogger.ic5", 0x0000, 0x1000 )
	ROM_LOAD( "frogger.ic6", 0x1000, 0x1000 )
	ROM_LOAD( "frogger.ic7", 0x2000, 0x1000 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "frogger.606", 0x0000, 0x0800 )
	ROM_LOAD( "frogger.607", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608", 0x0000, 0x0800 )
	ROM_LOAD( "frogger.609", 0x0800, 0x0800 )
	ROM_LOAD( "frogger.610", 0x1000, 0x0800 )
ROM_END



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x83f1],"\x63\x04",2) == 0 &&
			memcmp(&RAM[0x83f9],"\x27\x01",2) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x83f1],1,2*5,f);
			RAM[0x83ef] = RAM[0x83f1];
			RAM[0x83f0] = RAM[0x83f2];
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
		fwrite(&RAM[0x83f1],1,2*5,f);
		fclose(f);
	}
}



struct GameDriver frogger_driver =
{
	"frogger",
	&machine_driver,

	frogger_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,	/* letters */
		0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a },
	0x00, 0x03,
	8*13, 8*16, 0x06,

	hiload, hisave
};



struct GameDriver frogsega_driver =
{
	"frogsega",
	&machine_driver,

	frogsega_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,	/* letters */
		0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a },
	0x00, 0x03,
	8*13, 8*16, 0x06,

	hiload, hisave
};
