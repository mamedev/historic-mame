/***************************************************************************

Mario Bros memory map (preliminary):

0000-5fff ROM
6000-6fff RAM
7000-73ff ?
7400-77ff Video RAM
f000-ffff ROM

read:
7c00      IN0
7c80      IN1
7f80      DSW

*
 * IN0 (bits NOT inverted)
 * bit 7 : TEST
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : JUMP player 1
 * bit 3 : ? DOWN player 1 ?
 * bit 2 : ? UP player 1 ?
 * bit 1 : LEFT player 1
 * bit 0 : RIGHT player 1
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : JUMP player 2
 * bit 3 : ? DOWN player 2 ?
 * bit 2 : ? UP player 2 ?
 * bit 1 : LEFT player 2
 * bit 0 : RIGHT player 2
 *
*
 * DSW (bits NOT inverted)
 * bit 7 : \ difficulty
 * bit 6 : / 00 = easy  01 = medium  10 = hard  11 = hardest
 * bit 5 : \ bonus
 * bit 4 : / 00 = 20000  01 = 30000  10 = 40000  11 = none
 * bit 3 : \ coins per play
 * bit 2 : /
 * bit 1 : \ 00 = 3 lives  01 = 4 lives
 * bit 0 : / 10 = 5 lives  11 = 6 lives
 *

write:
7d00      ?
7d80      ?
7e00      sound
7e80-7e82 ?
7e83      sprite palette bank select
7e84      interrupt enable
7e85      ?
7f00-7f07 sound triggers


I/O ports

write:
00        ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "I8039/I8039.h"

static int p[8] = { 0,0xf0,0,0,0,0,0,0 };
static int t[2] = { 0,0 };




void mario_gfxbank_w(int offset,int data);
void mario_palettebank_w(int offset,int data);
int  mario_vh_start(void);
void mario_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mario_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/*
 *  from sndhrdw/mario.c
 */
void   mario_sh_w(int offset,int data);
void   mario_sh1_w(int offset,int data);
void   mario_sh2_w(int offset,int data);
void   mario_sh3_w(int offset,int data);


#define ACTIVELOW_PORT_BIT(P,A,D)   ((P & (~(1 << A))) | ((D ^ 1) << A))
#define ACTIVEHIGH_PORT_BIT(P,A,D)   ((P & (~(1 << A))) | (D << A))


void mario_sh_growing(int offset, int data)    { t[1] = data; }
void mario_sh_getcoin(int offset, int data)    { t[0] = data; }
void mario_sh_crab(int offset, int data)       { p[1] = ACTIVEHIGH_PORT_BIT(p[1],0,data); }
void mario_sh_turtle(int offset, int data)     { p[1] = ACTIVEHIGH_PORT_BIT(p[1],1,data); }
void mario_sh_fly(int offset, int data)        { p[1] = ACTIVEHIGH_PORT_BIT(p[1],2,data); }
static void mario_sh_tuneselect(int offset, int data) { soundlatch_w(offset,data); }

static int  mario_sh_getp1(int offset)   { return p[1]; }
static int  mario_sh_getp2(int offset)   { return p[2]; }
static int  mario_sh_gett0(int offset)   { return t[0]; }
static int  mario_sh_gett1(int offset)   { return t[1]; }
static int  mario_sh_gettune(int offset) { return soundlatch_r(offset); }

static void mario_sh_putsound(int offset, int data)
{
	DAC_data_w(0,data);
}
static void mario_sh_putp1(int offset, int data)
{
        p[1] = data;
}
static void mario_sh_putp2(int offset, int data)
{
        p[2] = data;
}
void masao_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 1 && data == 0)
	{
		/* setting bit 0 high then low triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x7400, 0x77ff, MRA_RAM },	/* video RAM */
	{ 0x7c00, 0x7c00, input_port_0_r },	/* IN0 */
	{ 0x7c80, 0x7c80, input_port_1_r },	/* IN1 */
	{ 0x7f80, 0x7f80, input_port_2_r },	/* DSW */
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x7400, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7e80, 0x7e80, mario_gfxbank_w },
	{ 0x7e83, 0x7e83, mario_palettebank_w },
	{ 0x7e84, 0x7e84, interrupt_enable_w },
	{ 0x7c00, 0x7c00, mario_sh1_w }, /* Mario run sample */
	{ 0x7c80, 0x7c80, mario_sh2_w }, /* Luigi run sample */
	{ 0x7f01, 0x7f01, mario_sh_getcoin },
	{ 0x7f03, 0x7f03, mario_sh_crab },
	{ 0x7f04, 0x7f04, mario_sh_turtle },
	{ 0x7f05, 0x7f05, mario_sh_fly },
	{ 0x7f00, 0x7f07, mario_sh3_w }, /* Misc discrete samples */
	{ 0x7e00, 0x7e00, mario_sh_tuneselect },
	{ 0x7000, 0x73ff, MWA_NOP },	/* ??? */
//	{ 0x7e85, 0x7e85, MWA_RAM },	/* Sets alternative 1 and 0 */
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress masao_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x7400, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7e00, 0x7e00, soundlatch_w },
	{ 0x7e80, 0x7e80, mario_gfxbank_w },
	{ 0x7e83, 0x7e83, mario_palettebank_w },
	{ 0x7e84, 0x7e84, interrupt_enable_w },
	{ 0x7000, 0x73ff, MWA_NOP },	/* ??? */
	{ 0x7f00, 0x7f00, masao_sh_irqtrigger_w },
        { 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOWritePort mario_writeport[] =
{
	{ 0x00,   0x00,   IOWP_NOP },  /* unknown... is this a trigger? */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ -1 }	/* end of table */
};
static struct IOReadPort readport_sound[] =
{
	{ 0x00,     0xff,     mario_sh_gettune },
	{ I8039_p1, I8039_p1, mario_sh_getp1 },
	{ I8039_p2, I8039_p2, mario_sh_getp2 },
	{ I8039_t0, I8039_t0, mario_sh_gett0 },
	{ I8039_t1, I8039_t1, mario_sh_gett1 },
	{ -1 }	/* end of table */
};
static struct IOWritePort writeport_sound[] =
{
	{ 0x00,     0xff,     mario_sh_putsound },
	{ I8039_p1, I8039_p1, mario_sh_putp1 },
	{ I8039_p2, I8039_p2, mario_sh_putp2 },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x10, "30000" )
	PORT_DIPSETTING(    0x20, "40000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0xc0, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 512*8*8, 0 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8	/* every char takes 8 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },	/* the bitplanes are separated */
	{ 256*16*8+7, 256*16*8+6, 256*16*8+5, 256*16*8+4, 256*16*8+3, 256*16*8+2, 256*16*8+1, 256*16*8+0,
			7, 6, 5, 4, 3, 2, 1, 0 },	/* the two halves of the sprite are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 16 },
	{ 1, 0x2000, &spritelayout, 16*4, 32 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* 4P - palette */
	0xFF,0xD7,0x6B,0x00,0x1F,0x93,0xFB,0xFF,0xFF,0xE3,0x00,0x4F,0x00,0xFF,0x7F,0xFE,
	0xFF,0x74,0x08,0x0F,0x00,0xFF,0x7F,0xFE,0xFF,0x1F,0x00,0x0F,0x00,0xFF,0x7F,0xFE,
	0xFF,0x00,0x03,0x1F,0x7F,0x13,0x92,0xFE,0xFF,0x00,0xE0,0xF4,0xFE,0xEC,0x92,0x5F,
	0xFF,0x00,0x08,0x1C,0x9E,0x5E,0x92,0xFE,0xFF,0xFC,0xE4,0xE3,0xF3,0x00,0xB2,0xDF,
	0xFF,0x00,0xFE,0xFF,0x1C,0x1F,0x03,0x6C,0xFF,0x00,0xFE,0xFF,0xE3,0xE0,0x00,0x6C,
	0xFF,0xFE,0x9B,0x0A,0x00,0x1F,0x13,0xFF,0xFF,0xE3,0x9B,0x0E,0x00,0xDF,0xBC,0xFF,
	0xFF,0x1F,0xC8,0x00,0xFC,0xF1,0xEA,0xE3,0xFF,0x17,0x7F,0x00,0xFF,0x0F,0x1F,0x97,
	0xFF,0xE8,0xFE,0x00,0xFF,0x6F,0xFC,0x7A,0xFF,0x07,0xB7,0x00,0xFF,0xBF,0xFC,0x7A,
	0xFF,0xFE,0x9B,0x0A,0x00,0x1F,0x13,0x7A,0xFF,0xE3,0x9B,0x0E,0x00,0xDF,0xBC,0xFF,
	0xFF,0x00,0x6E,0x92,0xDB,0x1F,0x9F,0x8E,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xBF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,
	0xFF,0x00,0x92,0xFE,0xDB,0x00,0x00,0x92,0xFF,0xFF,0xFF,0xFF,0xFF,0xEC,0xFF,0xFF,
	0xFF,0x00,0xFE,0xFF,0x00,0x8C,0xF9,0xFE,0xFF,0x00,0xFE,0xFF,0xF7,0x00,0xF9,0xFE,
	0xFF,0xA0,0xFE,0xFF,0xEF,0xF7,0x00,0x8C,0xFF,0xA0,0xFE,0xFF,0x83,0xEF,0xF7,0x00,
	0xFF,0x0F,0x5F,0xFF,0x00,0x83,0xEF,0xF7,0xFF,0x0F,0x5F,0xFF,0xFE,0x00,0x83,0xEF,
	0xFF,0x00,0x1E,0xFF,0xF9,0xFE,0x00,0x83,0xFF,0x00,0x1E,0xFF,0x8C,0xF9,0xFE,0x00,
	0x00,0x28,0x94,0xFF,0xE0,0x6C,0x04,0x00,0x00,0x1C,0xFF,0xB0,0xFF,0x00,0x80,0x01,
	0x00,0x8B,0xF7,0xF0,0xFF,0x00,0x80,0x01,0x00,0xE0,0xFF,0xF0,0xFF,0x00,0x80,0x01,
	0x00,0xFF,0xFC,0xE0,0x80,0xEC,0x6D,0x01,0x00,0xFF,0x1F,0x0B,0x01,0x13,0x6D,0xA0,
	0x00,0xFF,0xF7,0xE3,0x61,0xA1,0x6D,0x01,0x00,0x03,0x1B,0x1C,0x0C,0xFF,0x4D,0x20,
	0x00,0xFF,0x01,0x00,0xE3,0xE0,0xFC,0x93,0x00,0xFF,0x01,0x00,0x1C,0x1F,0xFF,0x93,
	0x00,0x01,0x64,0xF5,0xFF,0xE0,0xEC,0x00,0x00,0x1C,0x64,0xF1,0xFF,0x20,0x43,0x00,
	0x00,0xE0,0x37,0xFF,0x03,0x0E,0x15,0x1C,0x00,0xE8,0x80,0xFF,0x00,0xF0,0xE0,0x68,
	0x00,0x17,0x01,0xFF,0x00,0x90,0x03,0x85,0x00,0xF8,0x48,0xFF,0x00,0x40,0x03,0x85,
	0x00,0x01,0x64,0xF5,0xFF,0xE0,0xEC,0x85,0x00,0x1C,0x64,0xF1,0xFF,0x20,0x43,0x00,
	0x00,0xFF,0x91,0x6D,0x24,0xE0,0x60,0x71,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
	0x00,0xFF,0x6D,0x01,0x24,0xFF,0xFF,0x6D,0x00,0x00,0x00,0x00,0x00,0x13,0x00,0x00,
	0x00,0xFF,0x01,0x00,0xFF,0x73,0x06,0x01,0x00,0xFF,0x01,0x00,0x08,0xFF,0x06,0x01,
	0x00,0x5F,0x01,0x00,0x10,0x08,0xFF,0x73,0x00,0x5F,0x01,0x00,0x7C,0x10,0x08,0xFF,
	0x00,0xF0,0xA0,0x00,0xFF,0x7C,0x10,0x08,0x00,0xF0,0xA0,0x00,0x01,0xFF,0x7C,0x10,
	0x00,0xFF,0xE1,0x00,0x06,0x01,0xFF,0x7C,0x00,0xFF,0xE1,0x00,0x73,0x06,0x01,0xFF
};



static struct DACinterface dac_interface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

static struct Samplesinterface samples_interface =
{
	3	/* 3 channels */
};

static struct AY8910interface ay8910_interface =
{
	1,      /* 1 chip */
	14318000/6,	/* ? */
	{ 255 },
	{ soundlatch_r },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MemoryReadAddress masao_sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress masao_sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x6000, 0x6000, AY8910_control_port_0_w },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ -1 }  /* end of table */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,mario_writeport,
			nmi_interrupt,1
		},
		{
			CPU_I8039 | CPU_AUDIO_CPU,
			670000,         /* 670 khz (?) */
			2,
			readmem_sound,writemem_sound,readport_sound,writeport_sound,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256,16*4+32*8,
	mario_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	mario_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static struct MachineDriver masao_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,        /* 4.000 Mhz (?) */
			0,
			readmem,masao_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			24576000/16,	/* ???? */
			2,
			masao_sound_readmem,masao_sound_writemem,0,0,
			ignore_interrupt,1
		}

		},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256,16*4+32*8,
	mario_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	mario_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *sample_names[] =
{
	/* 7f00 - 7f07 sounds */
	"death.sam",  /* 0x00 death */
	"ice.sam",    /* 0x02 ice appears (formerly effect0.sam) */
	"coin.sam",   /* 0x06 coin appears (formerly effect1.sam) */
	"skid.sam",   /* 0x07 skid */

	/* 7c00 */
	"run.sam",        /* 03, 02, 01 - 0x1b */

	/* 7c80 */
	"luigirun.sam",   /* 03, 02, 01 - 0x1c */

    0	/* end of array */
};



ROM_START( mario_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mario.7f", 0x0000, 0x2000, 0x978dfc01 )
	ROM_LOAD( "mario.7e", 0x2000, 0x2000, 0x587539fd )
	ROM_LOAD( "mario.7d", 0x4000, 0x2000, 0x33b5e2c5 )
	ROM_LOAD( "mario.7c", 0xf000, 0x1000, 0x7d313723 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mario.3f", 0x0000, 0x1000, 0xfd014e45 )
	ROM_LOAD( "mario.3j", 0x1000, 0x1000, 0xfb393def )
	ROM_LOAD( "mario.7m", 0x2000, 0x1000, 0x8b6216de )
	ROM_LOAD( "mario.7n", 0x3000, 0x1000, 0xb5b1ef57 )
	ROM_LOAD( "mario.7p", 0x4000, 0x1000, 0x97149ed8 )
	ROM_LOAD( "mario.7s", 0x5000, 0x1000, 0x28de7ec8 )
	ROM_LOAD( "mario.7t", 0x6000, 0x1000, 0x13f5f925 )
	ROM_LOAD( "mario.7u", 0x7000, 0x1000, 0x43e11755 )

	ROM_REGION(0x1000)	/* sound */
	ROM_LOAD( "mario.6k", 0x0000, 0x1000, 0x0c278aa3 )
ROM_END

ROM_START( masao_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "masao-4.rom", 0x0000, 0x2000, 0x50b1c9b1 )
	ROM_LOAD( "masao-3.rom", 0x2000, 0x2000, 0x268c1db2 )
	ROM_LOAD( "masao-2.rom", 0x4000, 0x2000, 0xffe2203e )
	ROM_LOAD( "masao-1.rom", 0xf000, 0x1000, 0x3d37dab5 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "masao-6.rom",  0x0000, 0x1000, 0xd1b430f8 )
	ROM_LOAD( "masao-7.rom",  0x1000, 0x1000, 0x963d7329 )
	ROM_LOAD( "masao-8.rom",  0x2000, 0x1000, 0x897916c7 )
	ROM_LOAD( "masao-9.rom",  0x3000, 0x1000, 0xbe350605 )
	ROM_LOAD( "masao-10.rom", 0x4000, 0x1000, 0x97149ed8 )
	ROM_LOAD( "masao-11.rom", 0x5000, 0x1000, 0xde10dbe8 )
	ROM_LOAD( "masao-12.rom", 0x6000, 0x1000, 0x10f4f824 )
	ROM_LOAD( "masao-13.rom", 0x7000, 0x1000, 0x4bc14527 )

	ROM_REGION(0x10000) /* 64k for sound */
	ROM_LOAD( "masao-5.rom", 0x0000, 0x1000, 0xb4d14c8f )
ROM_END

static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6b1d],"\x00\x20\x01",3) == 0 &&
			memcmp(&RAM[0x6ba5],"\x00\x32\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x6b00],34*5);	/* hi scores */
			RAM[0x6823] = RAM[0x6b1f];
			RAM[0x6824] = RAM[0x6b1e];
			RAM[0x6825] = RAM[0x6b1d];
			osd_fread(f,&RAM[0x6c00],0x3c);	/* distributions */
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x6b00],34*5);	/* hi scores */
		osd_fwrite(f,&RAM[0x6c00],0x3c);	/* distributions */
		osd_fclose(f);
	}
}



struct GameDriver mario_driver =
{
	__FILE__,
	0,
	"mario",
	"Mario Bros.",
	"1983",
	"Nintendo of America",
	"Mirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)\nDan Boris (8039 info)\nRon Fries (Audio Info)\nMarco Cassili",
	0,
	&machine_driver,

	mario_rom,
	0, 0,
	sample_names,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver masao_driver =
{
	__FILE__,
	&mario_driver,
	"masao",
	"Masao",
	"1983",
	"bootleg",
	"Hugh McLenaghan (MAME driver)\nMirko Buffoni (sound info)",
	0,
	&masao_machine_driver,

	masao_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
