/***************************************************************************

Wild Western memory map (preliminary)
same as Jungle King - I haven't verified it yet

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
8800      ? (Front Line only)
8801      ? the code stops until bit 0 is 1 (Front Line only)
d404      returns contents of graphic ROM, pointed by d509-d50a
d408      IN0
          bit 4 = fire player 1
          bit 3 = up player 1
          bit 2 = down player 1
          bit 1 = right player 1
          bit 0 = left player 1
d409      IN1
          bit 4 = fire player 2 (COCKTAIL only)
          bit 3 = up player 2 (COCKTAIL only)
          bit 2 = down player 2 (COCKTAIL only)
          bit 1 = right player 2 (COCKTAIL only)
          bit 0 = left player 2 (COCKTAIL only)
d40a      DSW1
          bit 7   = cocktail / upright (0 = upright)
          bit 6   = flip screen
          bit 5   = RAM check
          bit 3-4 = lives
		  bit 2   = high score names
          bit 0-1 = bonus
d40b      IN2
          bit 7 = start 2
          bit 6 = start 1
d40c      COIN
          bit 5 = tilt
          bit 4 = coin
d40f      8910 #0 read
            port A DSW2
              coins per play
            port B DSW3
              bit 7 = coinage (1 way/2 ways)
              bit 6 = demo mode
              bit 5 = year display yes/no
              bit 0-4 ?

write
d000-d01f front playfield column scroll
d020-d03f middle playfield column scroll
d040-d05f back playfield column scroll
d300      playfield priority control ??
          bit 0-2 ?
		  bit 3 = 1 middle playfield has priority over sprites ??
d40e      8910 #0 control
d40f      8910 #0 write
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
d600      bit 0-3 ?
		  bit 4 front playfield enable
		  bit 5 middle playfield (probably) enable
		  bit 6 back playfield (probably) enable
		  bit 7 sprites (probably) enable


SOUND CPU:
0000-0fff ROM (1fff for Front Line?)
4000-43ff RAM

read:
5000      command from CPU board

write:
4800      8910 #1  control
4801      8910 #1  write
4802      8910 #2  control
4803      8910 #2  write
4804      8910 #3  control
4805      8910 #3  write
            port B bit 0 SOUND CPU NMI disable

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"

extern unsigned char *elevator_protection;
int elevator_protection_r(int offset);
int elevator_protection_t_r(int offset);

extern unsigned char *taito_videoram2,*taito_videoram3;
extern unsigned char *taito_characterram;
extern unsigned char *taito_scrollx1,*taito_scrollx2,*taito_scrollx3;
extern unsigned char *taito_scrolly1,*taito_scrolly2,*taito_scrolly3;
extern unsigned char *taito_colscrolly1,*taito_colscrolly2,*taito_colscrolly3;
extern unsigned char *taito_gfxpointer,*taito_paletteram;
extern unsigned char *taito_colorbank,*taito_video_priority,*taito_video_enable;
void taito_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int taito_gfxrom_r(int offset);
void taito_videoram2_w(int offset,int data);
void taito_videoram3_w(int offset,int data);
void taito_paletteram_w(int offset,int data);
void taito_colorbank_w(int offset,int data);
void taito_characterram_w(int offset,int data);
int wwestern_vh_start(void);
void taito_vh_stop(void);
void taito_vh_screenrefresh(struct osd_bitmap *bitmap);

int elevator_sh_interrupt(void);
int elevator_sh_start(void);


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
	{ 0xd40f, 0xd40f, AY8910_read_port_0_r },	/* DSW2 and DSW3 */
	{ 0xd404, 0xd404, taito_gfxrom_r },
	{ 0x8801, 0x8801, elevator_protection_t_r },
	{ 0x8800, 0x8800, elevator_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xc400, 0xc7ff, videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcbff, taito_videoram2_w, &taito_videoram2 },
	{ 0xcc00, 0xcfff, taito_videoram3_w, &taito_videoram3 },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xd01f, MWA_RAM, &taito_colscrolly1 },
	{ 0xd020, 0xd03f, MWA_RAM, &taito_colscrolly2 },
	{ 0xd040, 0xd05f, MWA_RAM, &taito_colscrolly3 },
	{ 0xd500, 0xd500, MWA_RAM, &taito_scrollx1 },
	{ 0xd501, 0xd501, MWA_RAM, &taito_scrolly1 },
	{ 0xd502, 0xd502, MWA_RAM, &taito_scrollx2 },
	{ 0xd503, 0xd503, MWA_RAM, &taito_scrolly2 },
	{ 0xd504, 0xd504, MWA_RAM, &taito_scrollx3 },
	{ 0xd505, 0xd505, MWA_RAM, &taito_scrolly3 },
	{ 0xd506, 0xd507, taito_colorbank_w, &taito_colorbank },
	{ 0xd509, 0xd50a, MWA_RAM, &taito_gfxpointer },
	{ 0xd50b, 0xd50b, sound_command_w },
	{ 0xd50d, 0xd50d, MWA_NOP },
	{ 0xd200, 0xd27f, taito_paletteram_w, &taito_paletteram },
	{ 0x9000, 0xbfff, taito_characterram_w, &taito_characterram },
	{ 0xd40e, 0xd40e, AY8910_control_port_0_w },
	{ 0xd40f, 0xd40f, AY8910_write_port_0_w },
	{ 0xd300, 0xd300, MWA_RAM, &taito_video_priority },
	{ 0xd600, 0xd600, MWA_RAM, &taito_video_enable },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x5000, 0x5000, sound_command_r },
	{ 0x0000, 0x0fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_1_w },
	{ 0x4801, 0x4801, AY8910_write_port_1_w },
	{ 0x4802, 0x4802, AY8910_control_port_2_w },
	{ 0x4803, 0x4803, AY8910_write_port_2_w },
	{ 0x4804, 0x4804, AY8910_control_port_3_w },
	{ 0x4805, 0x4805, AY8910_write_port_3_w },
	{ 0x0000, 0x0fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, 0 , 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE, 0 , 0, 0 }
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
		0xff,
		{ 0, 0, 0, 0, OSD_KEY_3, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x7b,
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

static struct TrakPort trak_ports[] =
{
        { -1 }
};



static struct KEYSet keys[] =
{
        { 0, 3, "MOVE UP" },
        { 0, 0, "MOVE LEFT"  },
        { 0, 1, "MOVE RIGHT" },
        { 0, 2, "MOVE DOWN" },
        { 0, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 4, 0x18, "LIVES", { "6", "5", "4", "3" }, 1 },
	{ 4, 0x03, "BONUS", { "70000", "50000", "30000", "10000" }, 1 },
	{ 6, 0x40, "DEMO MODE", { "ON", "OFF" }, 1 },
	{ 6, 0x20, "YEAR DISPLAY", { "NO", "YES" }, 1 },
	{ 4, 0x04, "HIGH SCORE NAME", { "YES", "NO" } },
	{ 6, 0x01, "SW C 1", { "ON", "OFF" }, 1 },
	{ 6, 0x02, "SW C 2", { "ON", "OFF" }, 1 },
	{ 6, 0x04, "SW C 3", { "ON", "OFF" }, 1 },
	{ 6, 0x08, "SW C 4", { "ON", "OFF" }, 1 },
	{ 6, 0x10, "SW C 5", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 512*8*8, 256*8*8, 0 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
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
	{ 0, 0x9000, &charlayout,   0, 16 },	/* the game dynamically modifies this */
	{ 0, 0x9000, &spritelayout, 0,  8 },	/* the game dynamically modifies this */
	{ 0, 0xa800, &charlayout,   0,  8 },	/* the game dynamically modifies this */
	{ 0, 0xa800, &spritelayout, 0,  8 },	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};



/* Wild Western doesn't have a color PROM, it uses a RAM to generate colors */
/* and change them during the game. Here is the list of all the colors is uses. */
static unsigned char color_prom[] =
{
	/* total: 33 colors (2 bytes per color) */
	0x01,0xFF,	/* transparent black */
	0x01,0xFF,0x01,0xFE,0x01,0xFD,0x01,0xFC,0x01,0xFB,0x01,0xFA,0x01,0xF9,0x01,0xF8,
	0x01,0xF0,0x01,0xE8,0x01,0xE0,0x01,0xD8,0x01,0xD0,0x01,0xC8,0x01,0xC7,0x01,0xC3,
	0x01,0xC0,0x01,0x80,0x01,0x40,0x01,0x00,0x00,0xC0,0x00,0xBF,0x00,0x93,0x00,0x8F,
	0x00,0x80,0x00,0x57,0x00,0x40,0x00,0x3F,0x00,0x38,0x00,0x24,0x00,0x07,0x00,0x00
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
			elevator_sh_interrupt,5
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	33, 16*8,
	taito_vh_convert_color_prom,

	0,
	wwestern_vh_start,
	taito_vh_stop,
	taito_vh_screenrefresh,

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

ROM_START( wwestern_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ww01.bin",  0x0000, 0x1000, 0x9643808b )
	ROM_LOAD( "ww02d.bin", 0x1000, 0x1000, 0x2600df90 )
	ROM_LOAD( "ww03d.bin", 0x2000, 0x1000, 0xc48cf79e )
	ROM_LOAD( "ww04d.bin", 0x3000, 0x1000, 0x056c2194 )
	ROM_LOAD( "ww05d.bin", 0x4000, 0x1000, 0x09bfef11 )
	ROM_LOAD( "ww06d.bin", 0x5000, 0x1000, 0x84cb873f )
	ROM_LOAD( "ww07.bin",  0x6000, 0x1000, 0xfa9f8521 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_OBSOLETELOAD( "ww08.bin",  0x0000, 0x1000 )	/* not needed - could be removed */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "ww08.bin", 0x0000, 0x1000, 0xa03f7275 )
	ROM_LOAD( "ww09.bin", 0x1000, 0x1000, 0x3179c0b1 )
	ROM_LOAD( "ww10.bin", 0x2000, 0x1000, 0xc957ac33 )
	ROM_LOAD( "ww11.bin", 0x3000, 0x1000, 0x80e293be )
	ROM_LOAD( "ww12.bin", 0x4000, 0x1000, 0xc053509d )
	ROM_LOAD( "ww13.bin", 0x5000, 0x1000, 0xbc0fba87 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ww14.bin", 0x0000, 0x1000, 0xc7424c46 )
ROM_END

ROM_START( frontlin_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "aa1.05", 0x0000, 0x2000, 0x0f3ae054 )
	ROM_LOAD( "aa1.06", 0x2000, 0x2000, 0xcc3889f8 )
	ROM_LOAD( "aa1.07", 0x4000, 0x2000, 0x90402de4 )
	ROM_LOAD( "aa1.08", 0x6000, 0x2000, 0xd0239e27 )
	ROM_LOAD( "aa1.10", 0xe000, 0x1000, 0xe8cfe99f )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_OBSOLETELOAD( "aa1.09",  0x0000, 0x2000 )	/* not needed - could be removed */

	ROM_REGION(0x8000)	/* graphic ROMs */
#if 0
	ROM_LOAD( "aa1.01", 0x0000, 0x2000, 0x7e801008 )
	ROM_LOAD( "aa1.02", 0x2000, 0x2000, 0x7ed0424c )
	ROM_LOAD( "aa1.03", 0x4000, 0x2000, 0xfee60766 )
	ROM_LOAD( "aa1.04", 0x6000, 0x2000, 0xd64f3991 )
	ROM_LOAD( "aa1.09", 0x0000, 0x2000, 0xaebb291b )
#endif
	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "aa1.11", 0x0000, 0x1000, 0x047afe22 )
	ROM_LOAD( "aa1.12", 0x1000, 0x1000, 0xd09423e4 )	/* ? */
ROM_END



struct GameDriver wwestern_driver =
{
	"Wild Western",
	"wwestern",
	"NICOLA SALMORIA",
	&machine_driver,

	wwestern_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};

struct GameDriver frontlin_driver =
{
	"Front Line",
	"frontlin",
	"NICOLA SALMORIA",
	&machine_driver,

	frontlin_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};
