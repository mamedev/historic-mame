/***************************************************************************

Green Berete memory map (preliminary)

0000-bfff ROM
c000-c7ff Color RAM
c800-cfff Video RAM
d000-d0c0 Sprites (bank 0)
d100-d1c0 Sprites (bank 1)
d200-dfff RAM
e000-e01f ZRAM1 line scroll registers
e020-e03f ZRAM2 bit 8 of line scroll registers

read:
f200      DSW2
          bit 0-1 lives
          bit 2   cocktail/upright cabinet (0 = upright)
          bit 3-4 bonus
          bit 5-6 difficulty
          bit 7   demo sounds
f400      DSW3
          bit 0 = screen flip
		  bit 1 = single/dual upright controls
f600      DSW1 coins per play
f601      IN1 player 2 controls
f602      IN0 player 1 controls
f603      IN2
          bit 0-1-2 coin  bit 3 1 player  start  bit 4 2 players start

write:
e043      bit 3 = sprite bank select; other bits = ?
e044      bit 0 = nmi enable, other bits = ?
f200      sound (?)
f400      sound (?)
f600      watchdog reset

interrupts:
The game uses both IRQ (mode 1) and NMI.
Vblank triggers IRQ. NMI is used for timing.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int gberet_interrupt(void);

extern unsigned char *gberet_scroll;
extern int gberet_vh_start(void);
extern void gberet_vh_stop(void);
extern void gberet_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void ladybug_sound1_w(int offset,int data);
extern void ladybug_sound2_w(int offset,int data);
extern int ladybug_sh_start(void);
extern void ladybug_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe03f, MRA_RAM },
	{ 0xf602, 0xf602, input_port_0_r },	/* IN0 */
	{ 0xf601, 0xf601, input_port_1_r },	/* IN1 */
	{ 0xf603, 0xf603, input_port_2_r },	/* IN2 */
	{ 0xf600, 0xf600, input_port_4_r },	/* DSW1 */
	{ 0xf200, 0xf200, input_port_3_r },	/* DSW2 */
	{ 0xf400, 0xf400, input_port_4_r },	/* DSW3 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xd200, 0xdfff, MWA_RAM },
	{ 0xc000, 0xc7ff, colorram_w, &colorram },
	{ 0xc800, 0xcfff, videoram_w, &videoram },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram },
	{ 0xe000, 0xe03f, MWA_RAM, &gberet_scroll },
	{ 0xf600, 0xf600, MWA_NOP },
{ 0xe043, 0xe043, MWA_RAM },
{ 0xe044, 0xe044, MWA_RAM },
	{ 0xf200, 0xf200, ladybug_sound1_w },
	{ 0xf400, 0xf400, ladybug_sound2_w },
	{ 0x0000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 },
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x7a,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW3 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 4, 0x03, "LIVES", { "7", "5", "3", "2" }, 1 },
	{ 4, 0x18, "BONUS", { "50000 200000", "50000 100000", "40000 80000", "30000 70000" }, 1 },
	{ 4, 0x60, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 4, 0x80, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		64*8+0*32, 64*8+1*32, 64*8+2*32, 64*8+3*32, 64*8+4*32, 64*8+5*32, 64*8+6*32, 64*8+7*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,       0, 16 },
	{ 1, 0x04000, &spritelayout, 16*16, 16 },
	{ 1, 0x0c000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xdb,0x00,0x00,	/* RED */
	0xdb,0x92,0x49,	/* BROWN */
	0xff,0xb6,0xdb,	/* PINK */
	0x00,0xdb,0x00,	/* UNUSED */
	0x00,0xdb,0xdb,	/* CYAN */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,0xb6,0x49,	/* DKORANGE */
	0x88,0x88,0x88,	/* UNUSED */
	0xdb,0xdb,0x00,	/* YELLOW */
	0xff,0x00,0xdb,	/* UNUSED */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xdb,0x00,	/* GREEN */
	0x49,0xb6,0x92,	/* DKGREEN */
	0xff,0xb6,0x92,	/* LTORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum {BLACK,RED,BROWN,PINK,UNUSED1,CYAN,DKCYAN,DKORANGE,
		UNUSED2,YELLOW,UNUSED3,BLUE,GREEN,DKGREEN,LTORANGE,GREY};

static unsigned char colortable[] =
{
	/* chars */
	/* most of the chars are actually two 4-color chars put together. The color */
	/* combination selects which of the two is displayed */
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,
	0,4,5,6,0,4,5,6,0,4,5,6,0,4,5,6,
	0,0,0,0,7,7,7,7,8,8,8,8,9,9,9,9,
	0,11,12,13,0,11,12,13,0,11,12,13,0,11,12,13,
	0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,
	0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	0,4,5,6,0,4,5,6,0,4,5,6,0,4,5,6,
	0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,
	0,11,12,13,0,11,12,13,0,11,12,13,0,11,12,13,
	0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,
	0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,
	0,0,0,0,7,7,7,7,8,8,8,8,9,9,9,9,
	0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,
	0,4,5,6,0,4,5,6,0,4,5,6,0,4,5,6,

	/* sprites */
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};



/* waveforms for the audio hardware */
static unsigned char samples[32] =	/* a simple sine (sort of) wave */
{
	0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x44,0x44,0x44,0x44,0x22,0x22,0x22,0x22,
	0x00,0x00,0x00,0x00,0xdd,0xdd,0xdd,0xdd,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd,0xdd,0xdd
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz?? */
			0,
			readmem,writemem,0,0,
			gberet_interrupt,8
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 2*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	gberet_vh_start,
	gberet_vh_stop,
	gberet_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	ladybug_sh_start,
	0,
	ladybug_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gberet_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "c10_l03.bin", 0x0000, 0x4000 )
	ROM_LOAD( "c08_l02.bin", 0x4000, 0x4000 )
	ROM_LOAD( "c07_l01.bin", 0x8000, 0x4000 )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "f03_l07.bin", 0x00000, 0x4000 )
	ROM_LOAD( "e05_l06.bin", 0x04000, 0x4000 )
	ROM_LOAD( "e04_l05.bin", 0x08000, 0x4000 )
	ROM_LOAD( "f04_l08.bin", 0x0c000, 0x4000 )
	ROM_LOAD( "e03_l04.bin", 0x10000, 0x4000 )
ROM_END



struct GameDriver gberet_driver =
{
	"gberet",
	&machine_driver,

	gberet_rom,
	0, 0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,	/* letters */
		0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a },
	0x02, 0x04,
	8*13, 8*16, 0x05,

	0, 0
};
