/***************************************************************************

Blue Print memory map (preliminary)

CPU #1
0000-4fff ROM
5000-7fff Space for other ROMs, but which ones?
8000-87ff RAM
9000-93ff Video RAM
a000-a01f ??
b000-b0ff Sprite RAM
f000-f3ff Color RAM

read:
c000      IN0
          bit 7 = DOWN
          bit 6 = UP
          bit 5 = RIGHT
          bit 4 = LEFT
          bit 3 = FIRE
          bit 2 = TILT
          bit 1 = START 1
          bit 0 = COIN
c001      IN1
          bit 3-7 = ?
          bit 2 = TEST (1 = do the test)
          bit 1 = START 2
          bit 0 = SERVICE?
c003      when d000 = 0x11: DSW1
          bit 7 = ?
          bit 6 = ?
          bit 5 = coins per play
          bit 4 = Maze monster appears in 2nd (0) or 3rd (1) maze
          bit 3 = free play
          bit 1-2 = bonus
          bit 0 = ?

          when d000 = 0x13: DSW2
          bit 7 = ?
          bit 6 = ?
          bit 4-5 = difficulty
          bit 3 = UPRIGHT or COCKTAIL select (0 = UPRIGHT) (?)
          bit 2 = ?
          bit 0-1 = lives

e000      Watchdog reset

write:
c000      ?
d000      ?


CPU #2
0000-0fff ROM
2000-2fff ROM
4000-43ff RAM

read:
6002      8910 #0 read
8002      8910 #1 read

write:
6000      8910 #0 control
6001      8910 #0 write
8000      8910 #1 control
8001      8910 #1 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



void blueprnt_vh_screenrefresh(struct osd_bitmap *bitmap);

int blueprnt_sh_interrupt(void);
int blueprnt_sh_start(void);


static int pip(int offset)
{
if (RAM[0xd000] == 0x11) return readinputport(2);
else return readinputport(3);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0xf000, 0xf3ff, MRA_RAM },
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0xa000, 0xa01f, MRA_RAM },
	{ 0xb000, 0xb0ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc003, 0xc003, pip },
	{ 0xe000, 0xe000, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0xf000, 0xf3ff, colorram_w, &colorram },
	{ 0xa000, 0xa01f, MWA_RAM },
	{ 0xb000, 0xb0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xd000, sound_command_w },
	{ 0x0000, 0x4fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6002, 0x6002, AY8910_read_port_0_r },
	{ 0x8002, 0x8002, AY8910_read_port_1_r },
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x2fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x6000, 0x6000, AY8910_control_port_0_w },
	{ 0x6001, 0x6001, AY8910_write_port_0_w },
	{ 0x8000, 0x8000, AY8910_control_port_1_w },
	{ 0x8001, 0x8001, AY8910_write_port_1_w },
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x2fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_3, OSD_KEY_1, OSD_KEY_T, OSD_KEY_CONTROL, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN },
		{ 0, 0, 0, OSD_JOY_FIRE, OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN },
	},
	{	/* IN1 */
		0x00,
		{ OSD_KEY_4, OSD_KEY_2, OSD_KEY_F2, OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
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
        { 0, 6, "MOVE UP" },
        { 0, 4, "MOVE LEFT"  },
        { 0, 5, "MOVE RIGHT" },
        { 0, 7, "MOVE DOWN" },
        { 0, 3, "ACCELERATE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x03, "LIVES", { "2", "3", "4", "5" } },
	{ 2, 0x06, "BONUS", { "20000", "30000", "40000", "50000" } },
	{ 3, 0x30, "DIFFICULTY", { "EASY", "MEDIUM", "HARD", "HARDEST" } },
	{ 2, 0x10, "MAZE MONSTER", { "2ND MAZE", "3RD MAZE" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,8,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 128*16*16, 2*128*16*16 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	16*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,  0, 8 },
	{ 1, 0x2000, &spritelayout, 8*4, 1 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xff,0x00,0x00, /* RED */
	0x00,0xff,0x00, /* GREEN */
	0x00,0x00,0xff, /* BLUE */
	0xff,0xff,0x00, /* YELLOW */
	0xff,0x00,0xff, /* MAGENTA */
	0x00,0xff,0xff, /* CYAN */
	0xff,0xff,0xff, /* WHITE */
	0xE0,0xE0,0xE0, /* LTGRAY */
	0xC0,0xC0,0xC0, /* DKGRAY */
	0xe0,0xb0,0x70,	/* BROWN */
	0xd0,0xa0,0x60,	/* BROWN0 */
	0xc0,0x90,0x50,	/* BROWN1 */
	0xa3,0x78,0x3a,	/* BROWN2 */
	0x80,0x60,0x20,	/* BROWN3 */
	0x54,0x40,0x14,	/* BROWN4 */
	0x54,0xa8,0xff, /* LTBLUE */
	0x00,0xa0,0x00, /* DKGREEN */
	0x00,0xe0,0x00, /* GRASSGREEN */
	0xff,0xb6,0xdb,	/* PINK */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,96,0x49,	/* DKORANGE */
	0xff,128,0x00,	/* ORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};


static unsigned char colortable[] =
{
	0,1,2,3,
	0,4,5,6,
	0,7,8,9,
	0,10,11,12,
	0,13,14,15,
	0,1,3,5,
	0,7,9,11,
	0,13,15,17,

	0,1,2,3,7,4,5,6
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz ? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			blueprnt_sh_interrupt,16
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	blueprnt_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	blueprnt_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( blueprnt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1m", 0x0000, 0x1000, 0x24058e7b )
	ROM_LOAD( "1n", 0x1000, 0x1000, 0x0b2382d7 )
	ROM_LOAD( "1p", 0x2000, 0x1000, 0xc8efb4af )
	ROM_LOAD( "1r", 0x3000, 0x1000, 0xec63fbb5 )
	ROM_LOAD( "1s", 0x4000, 0x1000, 0x3b24f9f6 )

	ROM_REGION(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c3",  0x0000, 0x1000, 0xf081e14f )
	ROM_LOAD( "d3",  0x1000, 0x1000, 0xa48e0a9e )
	ROM_LOAD( "d17", 0x2000, 0x1000, 0xc334fbe2 )
	ROM_LOAD( "d18", 0x3000, 0x1000, 0x5451da09 )
	ROM_LOAD( "d20", 0x4000, 0x1000, 0x966ee1d4 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "3u", 0x0000, 0x1000, 0xfd126e6a )
	ROM_LOAD( "3v", 0x2000, 0x1000, 0x22ab6921 )
ROM_END



struct GameDriver blueprnt_driver =
{
	"Blue Print",
	"blueprnt",
	"NICOLA SALMORIA",
	&machine_driver,

	blueprnt_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	0, 0
};
