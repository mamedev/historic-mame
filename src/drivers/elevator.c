/***************************************************************************

Elevator Action memory map (preliminary)

MAIN CPU:

0000-7fff ROM
8000-87ff RAM
9000-bfff Character generator RAM
c400-c7ff Video RAM: front playfield
c800-cbff Video RAM: middle playfield
cc00-cfff Video RAM: back playfield
d100-d17f Sprites
d200-d27f Palette (64 pairs: xxxxxxxR RRGGGBBB. bits are inverted, i.e. 0x01ff = black)

read:
8800      ?
8801      ? the code stops until bit 0 and 1 are = 1
d404      returns contents of graphic ROM, pointed by d509-d50a
d408      IN0
          bit 5 = jump player 1
          bit 4 = fire player 1
          bit 3 = up player 1
          bit 2 = down player 1
          bit 1 = right player 1
          bit 0 = left player 1
d409      IN1
          bit 5 = jump player 2 (COCKTAIL only)
          bit 4 = fire player 2 (COCKTAIL only)
          bit 3 = up player 2 (COCKTAIL only)
          bit 2 = down player 2 (COCKTAIL only)
          bit 1 = right player 2 (COCKTAIL only)
          bit 0 = left player 2 (COCKTAIL only)
d40a      DSW1
          bit 7   = cocktail / upright (0 = upright)
          bit 6   = flip screen
          bit 5   = ?
          bit 3-4 = lives
		  bit 2   = free play
          bit 0-1 = bonus
d40b      IN2
          bit 7 = start 2
          bit 6 = start 1
d40c      COIN
          bit 5 = tilt
          bit 4 = coin
d40f      DSW2 (when d40e == 0x0e) and DSW3 (when d40e == 0x0f)
          DSW2
		  coins per play
          DSW3
          bit 7 = coinage (1 way/2 ways)
          bit 6 = no hit
          bit 5 = year display yes/no
          bit 4 = coin display yes/no
		  bit 2-3 ?
		  bit 0-1 difficulty

write
d000-d01f front playfield column scroll
d020-d03f middle playfield column scroll
d040-d05f back playfield column scroll
d300      playfield priority control ??
          bit 0-2 ?
		  bit 3 = 1 middle playfield has priority over sprites ??
d40e      0e/0f = control which of DSW2 and DSW3 is read from d40f; other values = ?
d40f      ?
d500      front playfield horizontal scroll
d501      front playfield vertical scroll
d502      middle playfield horizontal scroll
d503      middle playfield vertical scroll
d504      back playfield horizontal scroll
d505      back playfield vertical scroll
d506      bits 0-3 = front playfield color code
          bits 4-7 = middle playfield color code
d507      bits 0-3 = back playfield color code
          bits 4-7 = sprite color bank (1 bank = 2 color codes)
d509-d50a pointer to graphic ROM to read from d404
d50b      command for the audio CPU
d50d      watchdog reset
d50e      bootleg version: $01 -> ROM ea54.bin is mapped at 7000-7fff
                           $81 -> ROM ea52.bin is mapped at 7000-7fff
d600      bit 0-3 ?
		  bit 4 front playfield enable
		  bit 5 middle playfield (probably) enable
		  bit 6 back playfield (probably) enable
		  bit 7 sprites (probably) enable


SOUND CPU:
0000-1fff ROM
4000-43ff RAM
e000-     additional ROM?

read:
5000      command from CPU board

write:
4800      8910 #1  control
4801      8910 #1  write
4802      8910 #1  control
4803      8910 #1  write
4804      8910 #1  control
4805      8910 #1  write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *taito_dsw23_select;
extern int taito_dsw23_r(int offset);
extern int elevator_init_machine(const char *gamename);
extern int elevator_protection_r(int offset);
extern int elevator_unknown_r(int offset);
extern void elevatob_bankswitch_w(int offset,int data);

extern unsigned char *elevator_videoram2,*elevator_videoram3;
extern unsigned char *elevator_characterram;
extern unsigned char *elevator_scroll1,*elevator_scroll2,*elevator_scroll3;
extern unsigned char *elevator_gfxpointer,*elevator_paletteram;
extern unsigned char *elevator_colorbank,*elevator_video_priority;
extern unsigned char *elevator_colorbank,*elevator_video_enable;
extern void elevator_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int elevator_gfxrom_r(int offset);
extern void elevator_videoram2_w(int offset,int data);
extern void elevator_videoram3_w(int offset,int data);
extern void elevator_paletteram_w(int offset,int data);
extern void elevator_colorbank_w(int offset,int data);
extern void elevator_characterram_w(int offset,int data);
extern int elevator_vh_start(void);
extern void elevator_vh_stop(void);
extern void elevator_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int elevator_sh_interrupt(void);
extern int elevator_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc400, 0xcfff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd408, 0xd408, input_port_0_r },	/* IN0 */
	{ 0xd409, 0xd409, input_port_1_r },	/* IN1 */
	{ 0xd40b, 0xd40b, input_port_2_r },	/* IN2 */
	{ 0xd40c, 0xd40c, input_port_3_r },	/* COIN */
	{ 0xd40a, 0xd40a, input_port_4_r },	/* DSW1 */
	{ 0xd40f, 0xd40f, taito_dsw23_r },	/* DSW2 and DSW3 */
	{ 0xd404, 0xd404, elevator_gfxrom_r },
	{ 0x8801, 0x8801, elevator_unknown_r },
	{ 0x8800, 0x8800, elevator_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xc400, 0xc7ff, videoram_w, &videoram },
	{ 0xc800, 0xcbff, elevator_videoram2_w, &elevator_videoram2 },
	{ 0xcc00, 0xcfff, elevator_videoram3_w, &elevator_videoram3 },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram },
	{ 0xd000, 0xd01f, MWA_RAM, &elevator_scroll1 },
	{ 0xd020, 0xd03f, MWA_RAM, &elevator_scroll2 },
	{ 0xd040, 0xd05f, MWA_RAM, &elevator_scroll3 },
	{ 0xd506, 0xd507, elevator_colorbank_w, &elevator_colorbank },
	{ 0xd509, 0xd50a, MWA_RAM, &elevator_gfxpointer },
	{ 0xd50b, 0xd50b, sound_command_w },
	{ 0xd50d, 0xd50d, MWA_NOP },
	{ 0xd200, 0xd27f, elevator_paletteram_w, &elevator_paletteram },
	{ 0x9000, 0xbfff, elevator_characterram_w, &elevator_characterram },
	{ 0xd50e, 0xd50e, elevatob_bankswitch_w },
	{ 0xd40e, 0xd40e, MWA_RAM, &taito_dsw23_select },
	{ 0xd600, 0xd600, MWA_RAM, &elevator_video_priority },
	{ 0xd600, 0xd600, MWA_RAM, &elevator_video_enable },
{ 0x8800, 0x8800, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x5000, 0x5000, sound_command_r },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_0_w },
	{ 0x4801, 0x4801, AY8910_write_port_0_w },
	{ 0x4802, 0x4802, AY8910_control_port_1_w },
	{ 0x4803, 0x4803, AY8910_write_port_1_w },
	{ 0x4804, 0x4804, AY8910_control_port_2_w },
	{ 0x4805, 0x4805, AY8910_write_port_2_w },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_2 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* COIN */
		0xef,
		{ 0, 0, 0, 0, OSD_KEY_3, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x7f,
		{ 0, 0, 0, 0, 0, OSD_KEY_F2, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW3 */
		0x7f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct KEYSet keys[] =
{
	{ 0, 3, "MOVE UP" },
	{ 0, 0, "MOVE LEFT"  },
	{ 0, 1, "MOVE RIGHT" },
	{ 0, 2, "MOVE DOWN" },
	{ 0, 4, "FIRE" },
	{ 0, 5, "JUMP" },
	{ -1 }
};



static struct DSW dsw[] =
{
	{ 4, 0x18, "LIVES", { "6", "5", "4", "3" }, 1 },
	{ 4, 0x03, "BONUS", { "25000", "20000", "15000", "10000" }, 1 },
	{ 6, 0x03, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 4, 0x04, "FREE PLAY", { "ON", "OFF" }, 1 },
	{ 6, 0x40, "DEMO MODE", { "ON", "OFF" }, 1 },
	{ 6, 0x10, "COIN DISPLAY", { "NO", "YES" }, 1 },
	{ 6, 0x20, "YEAR DISPLAY", { "NO", "YES" }, 1 },
	{ -1 }
};



struct GfxLayout elevator_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 512*8*8, 256*8*8, 0 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
struct GfxLayout elevator_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 128*16*16, 64*16*16, 0 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0,
		8*8+7, 8*8+6, 8*8+5, 8*8+4, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x9000, &elevator_charlayout,   0, 16 },	/* the game dynamically modifies this */
	{ 0, 0x9000, &elevator_spritelayout, 0,  8 },	/* the game dynamically modifies this */
	{ 0, 0xa800, &elevator_charlayout,   0,  8 },	/* the game dynamically modifies this */
	{ 0, 0xa800, &elevator_spritelayout, 0,  8 },	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};



/* Elevator Action doesn't have a color PROM, it uses a RAM to generate colors */
/* and change them during the game. Here is the list of all the colors is uses. */
static unsigned char color_prom[] =
{
	/* total: 37 colors (2 bytes per color) */
	0x01,0xFF,	/* transparent black */
	0x01,0xFF,0x01,0xFA,0x01,0xF8,0x01,0xE2,0x01,0xD8,0x01,0xD1,0x01,0xC7,0x01,0xB6,
	0x01,0xB1,0x01,0xA4,0x01,0x92,0x01,0x89,0x01,0x6D,0x01,0x24,0x01,0x20,0x00,0xDB,
	0x00,0xA7,0x00,0x9C,0x00,0x98,0x00,0x92,0x00,0x87,0x00,0x80,0x00,0x59,0x00,0x53,
	0x00,0x49,0x00,0x40,0x00,0x3F,0x00,0x1A,0x00,0x19,0x00,0x11,0x00,0x0B,0x00,0x0A,
	0x00,0x08,0x00,0x07,0x00,0x03,0x00,0x00
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz ??? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			elevator_sh_interrupt,2
		}
	},
	60,
	elevator_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	37, 16*8,
	elevator_vh_convert_color_prom,

	0,
	elevator_vh_start,
	elevator_vh_stop,
	elevator_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	elevator_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( elevator_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ea-ic69.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea-ic68.bin", 0x1000, 0x1000 )
	ROM_LOAD( "ea-ic67.bin", 0x2000, 0x1000 )
	ROM_LOAD( "ea-ic66.bin", 0x3000, 0x1000 )
	ROM_LOAD( "ea-ic65.bin", 0x4000, 0x1000 )
	ROM_LOAD( "ea-ic64.bin", 0x5000, 0x1000 )
	ROM_LOAD( "ea-ic55.bin", 0x6000, 0x1000 )
	ROM_LOAD( "ea-ic54.bin", 0x7000, 0x1000 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ea-ic4.bin",  0x0000, 0x1000 )
	ROM_LOAD( "ea-ic5.bin",  0x1000, 0x1000 )

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "ea-ic1.bin",  0x0000, 0x1000 )
	ROM_LOAD( "ea-ic2.bin",  0x1000, 0x1000 )
	ROM_LOAD( "ea-ic3.bin",  0x2000, 0x1000 )
	ROM_LOAD( "ea-ic4.bin",  0x3000, 0x1000 )
	ROM_LOAD( "ea-ic5.bin",  0x4000, 0x1000 )
	ROM_LOAD( "ea-ic6.bin",  0x5000, 0x1000 )
	ROM_LOAD( "ea-ic7.bin",  0x6000, 0x1000 )
	ROM_LOAD( "ea-ic8.bin",  0x7000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ea-ic70.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea-ic71.bin", 0x1000, 0x1000 )
/*	ROM_LOAD( "ee_ea10.bin", , 0x1000 ) ??? */
ROM_END

ROM_START( elevatob_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ea69.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea68.bin", 0x1000, 0x1000 )
	ROM_LOAD( "ea67.bin", 0x2000, 0x1000 )
	ROM_LOAD( "ea66.bin", 0x3000, 0x1000 )
	ROM_LOAD( "ea65.bin", 0x4000, 0x1000 )
	ROM_LOAD( "ea64.bin", 0x5000, 0x1000 )
	ROM_LOAD( "ea55.bin", 0x6000, 0x1000 )
	ROM_LOAD( "ea54.bin", 0x7000, 0x1000 )
	ROM_LOAD( "ea54.bin", 0xe000, 0x1000 )	/* copy for my convenience */
	ROM_LOAD( "ea52.bin", 0xf000, 0x1000 )	/* protection crack, bank switched at 7000 */

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ea04.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea05.bin", 0x1000, 0x1000 )

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "ea01.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea02.bin", 0x1000, 0x1000 )
	ROM_LOAD( "ea03.bin", 0x2000, 0x1000 )
	ROM_LOAD( "ea04.bin", 0x3000, 0x1000 )
	ROM_LOAD( "ea05.bin", 0x4000, 0x1000 )
	ROM_LOAD( "ea06.bin", 0x5000, 0x1000 )
	ROM_LOAD( "ea07.bin", 0x6000, 0x1000 )
	ROM_LOAD( "ea08.bin", 0x7000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ea70.bin", 0x0000, 0x1000 )
	ROM_LOAD( "ea71.bin", 0x1000, 0x1000 )
ROM_END



struct GameDriver elevator_driver =
{
	"Elevator Action",
	"elevator",
	"NICOLA SALMORIA",
	&machine_driver,

	elevator_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom,0,0,
	8*13, 8*16,

	0, 0
};

struct GameDriver elevatob_driver =
{
	"Elevator Action (bootleg)",
	"elevatob",
	"NICOLA SALMORIA",
	&machine_driver,

	elevatob_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom,0,0,
	8*13, 8*16,

	0, 0
};
