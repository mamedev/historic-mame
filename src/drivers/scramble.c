/***************************************************************************

Scramble memory map (preliminary)

MAIN BOARD:
0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
5000-50ff Object RAM
5000-503f  screen attributes
5040-505f  sprites
5060-507f  bullets
5080-50ff  unused?

read:
7000      Watchdog Reset (Scramble)
7800      Watchdog Reset (Battle of Atlantis)
8100      IN0
8101      IN1
8102      IN2

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : SHOOT 1 player 1
 * bit 2 : CREDIT (SERVICE)
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
 * bit 0 :/ 00 = 3  01 = 4  10 = 5  11 = 256
*
 * IN2 (all bits are inverted)
 * bit 7 : protection check?
 * bit 6 : DOWN player 1
 * bit 5 : protection check?
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/
 * bit 0 : DOWN player 2 (TABLE only)
 *

write:
6801      interrupt enable
6802      coin counter
6803      ? (POUT1)
6804      stars on
6805      ? (POUT2)
6806      screen vertical flip
6807      screen horizontal flip
8200      To AY-3-8910 port A (commands for the audio CPU)
8201      bit 3 = interrupt trigger on audio CPU  bit 4 = AMPM (?)
8202      protection check control?


SOUND BOARD:
0000-17ff ROM
8000-83ff RAM

I/0 ports:
read:
20      8910 #2  read
80      8910 #1  read

write
10      8910 #2  control
20      8910 #2  write
40      8910 #1  control
80      8910 #1  write

interrupts:
interrupt mode 1 triggered by the main CPU

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int scramble_IN2_r(int offset);
int scramble_protection_r(int offset);

extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void galaxian_attributes_w(int offset,int data);
void galaxian_stars_w(int offset,int data);
int scramble_vh_start(void);
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap);
int scramble_vh_interrupt(void);

int scramble_sh_interrupt(void);
int scramble_sh_start(void);
int frogger_sh_interrupt(void);
int frogger_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x7000, 0x7000, MRA_NOP },
	{ 0x7800, 0x7800, MRA_NOP },
	{ 0x8100, 0x8100, input_port_0_r },	/* IN0 */
	{ 0x8101, 0x8101, input_port_1_r },	/* IN1 */
	{ 0x8102, 0x8102, scramble_IN2_r },	/* IN2 */
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x8202, 0x8202, scramble_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x503f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x6801, 0x6801, interrupt_enable_w },
	{ 0x6804, 0x6804, galaxian_stars_w },
	{ 0x6802, 0x6802, MWA_NOP },
	{ 0x6806, 0x6807, MWA_NOP },
	{ 0x8200, 0x8200, sound_command_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x0000, 0x17ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x0000, 0x17ff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress froggers_sound_readmem[] =
{
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x0000, 0x17ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress froggers_sound_writemem[] =
{
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x0000, 0x17ff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort sound_readport[] =
{
	{ 0x80, 0x80, AY8910_read_port_0_r },
	{ 0x20, 0x20, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x20, 0x20, AY8910_write_port_1_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort froggers_sound_readport[] =
{
	{ 0x40, 0x40, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort froggers_sound_writeport[] =
{
	{ 0x80, 0x80, AY8910_control_port_0_w },
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_ALT, OSD_KEY_5, OSD_KEY_CONTROL,
				OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_4, OSD_KEY_3 },
		{ OSD_JOY_UP, OSD_JOY_FIRE2, 0, OSD_JOY_FIRE1,
				OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN1 */
		0xfc,
		{ 0, 0, OSD_KEY_ALT, OSD_KEY_CONTROL,
				OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1,
				OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN2 */
		0xf1,
		{ OSD_KEY_DOWN, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ OSD_JOY_DOWN, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 2, 4, "PL1 MOVE UP" },
        { 0, 5, "PL1 MOVE LEFT"  },
        { 0, 4, "PL1 MOVE RIGHT" },
        { 2, 6, "PL1 MOVE DOWN" },
        { 0, 3, "PL1 FIRE FRONT" },
        { 0, 1, "PL1 FIRE DOWN" },
        { 0, 0, "PL2 MOVE UP" },
        { 1, 5, "PL2 MOVE LEFT"  },
        { 1, 4, "PL2 MOVE RIGHT" },
        { 2, 0, "PL2 MOVE DOWN" },
        { 1, 3, "PL2 FIRE FRONT" },
        { 1, 2, "PL2 FIRE DOWN" },
        { -1 }
};


static struct DSW scramble_dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "256" } },
	{ 2, 0x06, "COINAGE", { "A 1/1 B 2/1 C 1/1", "A 1/2 B 1/1 C 1/2", "A 1/3 B 3/1 C 1/3", "A 1/4 B 4/1 C 1/4" } },
	{ -1 }
};
static struct DSW theend_dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "256" } },
	{ 2, 0x06, "COINAGE", { "1 Coin/1 Credit", "2 Coins/1 Credit", "3 Coins/1 Credit", "1 Coin/2 Credits" } },
	{ -1 }
};
static struct DSW atlantis_dsw[] =
{
	{ 1, 0x02, "LIVES", { "5", "3" }, 1 },
	{ 1, 0x01, "SW1", { "OFF", "ON" } },
	/* don't know the values for coinage, since ROM 2H is bad */
	{ 2, 0x0e, "COINAGE", { "0", "1", "2", "3", "4", "5", "6", "7" } },
	{ -1 }
};
static struct DSW froggers_dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "256" } },
	{ 2, 0x06, "COINAGE", { "A 1/1 B 1/1 C 1/1", "A 2/1 B 2/1 C 2/1", "A 2/1 B 1/3 C 2/1", "A 1/1 B 1/6 C 1/1" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	7,1,	/* it's just 1 pixel, but we use 7*1 to position it correctly */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 3, 0, 0, 0, 0, 0, 0 },	/* I "know" that this bit is 1 */
	{ 0 },	/* I "know" that this bit is 1 */
	0	/* no use */
};
static struct GfxLayout theend_bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	3,1,	/* 3*1 line */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 2, 2, 2 },	/* I "know" that this bit is 1 */
	{ 0 },	/* I "know" that this bit is 1 */
	0	/* no use */
};


static struct GfxDecodeInfo scramble_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 1 },	/* 1 color code instead of 2, so all */
											/* shots will be yellow */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo theend_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &theend_bulletlayout, 8*4, 2 },
	{ -1 } /* end of array */
};



static unsigned char scramble_color_prom[] =
{
	/* palette */
	0x00,0x17,0xC7,0xF6,0x00,0x17,0xC0,0x3F,0x00,0x07,0xC0,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC7,0x31,0x17,0x00,0x31,0xC7,0x3F,0x00,0xF6,0x07,0xF0,0x00,0x3F,0x07,0xC4
};



static unsigned char froggers_color_prom[] =
{
	/* palette */
	0x00,0xF6,0x79,0x4F,0x00,0xC0,0x3F,0x17,0x00,0x87,0xF8,0x7F,0x00,0xC1,0x7F,0x38,
	0x00,0x7F,0xCF,0xF9,0x00,0x57,0xB7,0xC3,0x00,0xFF,0x7F,0x87,0x00,0x79,0x4F,0xFF
};



static struct MachineDriver scramble_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 Mhz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			scramble_sh_interrupt,10
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	scramble_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	scramble_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/* same as Scramble, the only difference is the gfxdecodeinfo */
static struct MachineDriver theend_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 Mhz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			scramble_sh_interrupt,10
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	theend_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	scramble_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



static struct MachineDriver froggers_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz?????? */
			2,	/* memory region #2 */
			froggers_sound_readmem,froggers_sound_writemem,froggers_sound_readport,froggers_sound_writeport,
			frogger_sh_interrupt,10
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	scramble_gfxdecodeinfo,
	32+64,8*4+2*2,	/* 32 for the characters, 64 for the stars */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	frogger_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scramble_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2d", 0x0000, 0x0800, 0x9b48021e )
	ROM_LOAD( "2e", 0x0800, 0x0800, 0x4d2d8657 )
	ROM_LOAD( "2f", 0x1000, 0x0800, 0xcf9b5ad1 )
	ROM_LOAD( "2h", 0x1800, 0x0800, 0x8e22964a )
	ROM_LOAD( "2j", 0x2000, 0x0800, 0x14ce448c )
	ROM_LOAD( "2l", 0x2800, 0x0800, 0x110075b0 )
	ROM_LOAD( "2m", 0x3000, 0x0800, 0x746f8605 )
	ROM_LOAD( "2p", 0x3800, 0x0800, 0xa3280a00 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x0000, 0x0800, 0x86bcba72 )
	ROM_LOAD( "5h", 0x0800, 0x0800, 0x973cedc4 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800, 0xbbc47658 )
	ROM_LOAD( "5d", 0x0800, 0x0800, 0x7b9aac98 )
	ROM_LOAD( "5e", 0x1000, 0x0800, 0x5b267ea0 )
ROM_END

ROM_START( atlantis_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c", 0x0000, 0x0800, 0x35c37f85 )
	ROM_LOAD( "2e", 0x0800, 0x0800, 0x9a1cf98e )
	ROM_LOAD( "2f", 0x1000, 0x0800, 0xc43738d5 )
	ROM_LOAD( "2h", 0x1800, 0x0800, 0x00000000 )	/* this one seems to be bad */
	ROM_LOAD( "2j", 0x2000, 0x0800, 0x08db4581 )
	ROM_LOAD( "2l", 0x2800, 0x0800, 0x88d349cb )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x0000, 0x0800, 0x7f47e177 )
	ROM_LOAD( "5h", 0x0800, 0x0800, 0x30396a17 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800, 0xbbc47658 )
	ROM_LOAD( "5d", 0x0800, 0x0800, 0x7b9aac98 )
	ROM_LOAD( "5e", 0x1000, 0x0800, 0x5b267ea0 )
ROM_END

ROM_START( theend_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "IC13", 0x0000, 0x0800, 0x34fd8a0f )
	ROM_LOAD( "IC14", 0x0800, 0x0800, 0x19c26ade )
	ROM_LOAD( "IC15", 0x1000, 0x0800, 0xe7177301 )
	ROM_LOAD( "IC16", 0x1800, 0x0800, 0x9f72edda )
	ROM_LOAD( "IC17", 0x2000, 0x0800, 0xb2a13167 )
	ROM_LOAD( "IC18", 0x2800, 0x0800, 0x253049da )
	ROM_LOAD( "IC56", 0x3000, 0x0800, 0xe8f380ab )
	ROM_LOAD( "IC55", 0x3800, 0x0800, 0xe0c27de2 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC30", 0x0000, 0x0800, 0x83c615b6 )
	ROM_LOAD( "IC31", 0x0800, 0x0800, 0xad579d45 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "IC56", 0x0000, 0x0800, 0xe8f380ab )
	ROM_LOAD( "IC55", 0x0800, 0x0800, 0xe0c27de2 )
ROM_END

ROM_START( froggers_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_d2.bin", 0x0000, 0x0800, 0xb457efef )
	ROM_LOAD( "vid_e2.bin", 0x0800, 0x0800, 0x01dc34c8 )
	ROM_LOAD( "vid_f2.bin", 0x1000, 0x0800, 0xa382c0ac )
	ROM_LOAD( "vid_h2.bin", 0x1800, 0x0800, 0xcafbf75f )
	ROM_LOAD( "vid_j2.bin", 0x2000, 0x0800, 0x0071b52f )
	ROM_LOAD( "vid_l2.bin", 0x2800, 0x0800, 0x7a83468f )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vid_f5.bin", 0x0000, 0x0800, 0x38a71739 )
	ROM_LOAD( "vid_h5.bin", 0x0800, 0x0800, 0xb474d87c )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_c5.bin", 0x0000, 0x0800, 0x57851ff5 )
	ROM_LOAD( "snd_d5.bin", 0x0800, 0x0800, 0xd77b3859 )
	ROM_LOAD( "snd_e5.bin", 0x1000, 0x0800, 0x7ec0f39e )
ROM_END



static void froggers_decode(void)
{
	int A;
	unsigned char *RAM;


	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	for (A = 0;A < 0x0800;A++)
		RAM[A] = (RAM[A] & 0xfc) | ((RAM[A] & 1) << 1) | ((RAM[A] & 2) >> 1);
}



static int scramble_hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0x4200],"\x00\x00\x01",3) == 0) &&
		(memcmp(&RAM[0x421B],"\x00\x00\x01",3) == 0))
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x4200],1,0x1E,f);
			/* copy high score */
			memcpy(&RAM[0x40A8],&RAM[0x4200],3);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void scramble_hisave(const char *name)
{
	FILE *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x4200],1,0x1E,f);
		fclose(f);
	}

}


static int atlantis_hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x403D],"\x00\x00\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x403D],1,4*11,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void atlantis_hisave(const char *name)
{
	FILE *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x403D],1,4*11,f);
		fclose(f);
	}

}

static int theend_hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x43C0],"\x00\x00\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			/* This seems to have more than 5 scores in memory. */
			/* If this DISPLAYS more than 5 scores, change 3*5 to 3*10 or */
			/* however many it should be. */
			fread(&RAM[0x43C0],1,3*5,f);
			/* copy high score */
			memcpy(&RAM[0x40A8],&RAM[0x43C0],3);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void theend_hisave(const char *name)
{
	FILE *f;

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		/* This seems to have more than 5 scores in memory. */
		/* If this DISPLAYS more than 5 scores, change 3*5 to 3*10 or */
		/* however many it should be. */
		fwrite(&RAM[0x43C0],1,3*5,f);
		fclose(f);
	}

}


static int froggers_hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x43f1],"\x63\x04",2) == 0 &&
			memcmp(&RAM[0x43f8],"\x27\x01",2) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x43f1],1,2*5,f);
			RAM[0x43ef] = RAM[0x43f1];
			RAM[0x43f0] = RAM[0x43f2];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void froggers_hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x43f1],1,2*5,f);
		fclose(f);
	}
}



struct GameDriver scramble_driver =
{
	"Scramble",
	"scramble",
	"NICOLA SALMORIA\nMIKE BALFOUR",
	&scramble_machine_driver,

	scramble_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, scramble_dsw, keys,

	scramble_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	scramble_hiload, scramble_hisave
};

struct GameDriver atlantis_driver =
{
	"Battle of Atlantis",
	"atlantis",
	"NICOLA SALMORIA\nMIKE BALFOUR",
	&scramble_machine_driver,

	atlantis_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, atlantis_dsw, keys,

	scramble_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	atlantis_hiload, atlantis_hisave
};

struct GameDriver theend_driver =
{
	"The End",
	"theend",
	"NICOLA SALMORIA\nVILLE LAITINEN\nMIKE BALFOUR",
	&theend_machine_driver,

	theend_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, theend_dsw, keys,

	scramble_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	theend_hiload, theend_hisave
};

struct GameDriver froggers_driver =
{
	"Frog",
	"froggers",
	"NICOLA SALMORIA",
	&froggers_machine_driver,

	froggers_rom,
	froggers_decode, 0,
	0,

	input_ports, 0, trak_ports, froggers_dsw, keys,

	froggers_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	froggers_hiload, froggers_hisave
};
