/***************************************************************************

3 Stooges' memory map

Main processor (8088 minimum mode) memory map.
0000-0fff RAM
1000-1fff RAM
2000-2fff ROM
3000-37ff sprite programmation (64 sprites)
3800-3fff background ram (32x30 chars)
4000-4fff background object ram
5000-57ff palette ram (palette of 16 colors)
5800-5fff i/o ports (see below)
6000-ffff ROM

memory mapped ports:

read:
5800    Dip switch
5801    Inputs 10-17
5802    trackball input
5803    trackball input
5804    Inputs 40-47

write:
5800    watchdog timer clear
5801    trackball output
5802    Outputs 20-27
5803    Flipflop outputs:
		b7: F/B priority
		b6: horiz. flipflop
		b5: vert. flipflop
		b4: Output 33
		b3: coin meter
		b2: knocker
		b1: coin 1
		b0: coin lockout
5804    Outputs 40-47

interrupts:
INTR not connected
NMI connected to vertical blank

Sound processor memory map:
just some guesses for now, obtained with a fast overlook at the sound rom
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int stooges_vh_start(void);
void gottlieb_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void gottlieb_sh2_w(int offset, int data);
void gottlieb_video_outputs(int offset,int data);
extern unsigned char *gottlieb_characterram;
extern unsigned char *gottlieb_paletteram;
void gottlieb_characterram_w(int offset,int data);
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);

int riot_ram_r(int offset);
int gottlieb_riot_r(int offset);
int gottlieb_sound_expansion_socket_r(int offset);
void riot_ram_w(int offset, int data);
void gottlieb_riot_w(int offset, int data);
void gottlieb_amplitude_DAC_w(int offset, int data);
void gottlieb_speech_w(int offset, int data);
void gottlieb_speech_clock_DAC_w(int offset, int data);
void gottlieb_sound_expansion_socket_w(int offset, int data);

int gottlieb_nvram_load(void);
void gottlieb_nvram_save(void);



static int joympx;

int stooges_IN4_r(int offset)
{
	int joy;


	switch (joympx)
	{
		case 0:
		default:
			joy = ((readinputport(4) >> 0) & 0x0f);	/* joystick 1 */
			break;
		case 1:
			joy = ((readinputport(5) >> 0) & 0x0f);	/* joystick 2 */
			break;
		case 2:
			joy = ((readinputport(5) >> 4) & 0x0f);	/* joystick 3 */
			break;
	}

	return joy | (readinputport(4) & 0xf0);
}

void stooges_output(int offset,int data)
{
	joympx = (data >> 5) & 0x03;
	gottlieb_video_outputs(offset,data);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, input_port_1_r },     /* buttons */
	{ 0x5802, 0x5802, input_port_2_r },     /* trackball H: not used */
	{ 0x5803, 0x5803, input_port_3_r },     /* trackball V: not used */
	{ 0x5804, 0x5804, stooges_IN4_r },  /* joysticks demultiplexer */
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3bff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, gottlieb_characterram_w, &gottlieb_characterram },
	{ 0x5000, 0x501f, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, MWA_RAM },    /* trackball output not used */
	{ 0x5802, 0x5802, gottlieb_sh2_w }, /* sound/speech command */
	{ 0x5803, 0x5803, stooges_output },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress stooges_sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x8000, 0x801f, gottlieb_riot_r },
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

struct MemoryWriteAddress stooges_sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x4000, 0x4000, gottlieb_speech_w }, /* not sure... */
	{ 0x4001, 0x4001, gottlieb_amplitude_DAC_w },
	{ 0x8000, 0x801f, gottlieb_riot_w },
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* DSW */
	PORT_DIPNAME (0x01, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING (   0x01, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x02, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Normal" )
	PORT_DIPSETTING (   0x02, "Hard" )
	PORT_DIPNAME (0x14, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x10, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x14, "Free Play" )
	PORT_DIPNAME (0x08, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "3" )
	PORT_DIPSETTING (   0x08, "5" )
	PORT_BIT( 0x20, 0x00, IPT_UNUSED )
	PORT_DIPNAME (0x40, 0x00, "Bonus Life At", IP_KEY_NONE )
	PORT_DIPSETTING (   0x40, "10000" )
	PORT_DIPSETTING (   0x00, "20000" )
	PORT_DIPNAME (0x80, 0x00, "And Bonus Life Every", IP_KEY_NONE )
	PORT_DIPSETTING (   0x80, "10000" )
	PORT_DIPSETTING (   0x00, "20000" )

	PORT_START	/* IN1 */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x01, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_SERVICE, "Select in Service Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0xe0, 0x00, IPT_UNKNOWN )

	PORT_START	/* trackball h: not used */
	PORT_BIT( 0xff, 0x00, IPT_UNUSED )

	PORT_START	/* trackball v: not used */
	PORT_BIT( 0xff, 0x00, IPT_UNUSED )

	PORT_START	/* joystick 2 (Moe) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* the bottom four bits of the previous port are multiplexed among */
	/* three joysticks - the following port contains settings for the other two */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x2000*8, 0x4000*8, 0x6000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 }, /* 1 palette for the game */
	{ 1, 0x0000, &spritelayout, 0, 1 },
	{ -1 } /* end of array */
};



static struct DACinterface dac_interface =
{
	1,
	441000,
	{ 255 },
	{ 0 },
};



static const struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			5000000,        /* 5 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU ,
			3579545/4,
			2,             /* memory region #2 */
			stooges_sound_readmem,stooges_sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	16,256,
	gottlieb_vh_init_color_palette,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,      /* init vh */
	stooges_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



ROM_START( stooges_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "GV113RAM.4", 0x2000, 0x1000, 0x64249570 )
	ROM_LOAD( "GV113ROM.4", 0x6000, 0x2000, 0x8fdb5ff5 )
	ROM_LOAD( "GV113ROM.3", 0x8000, 0x2000, 0x8d135dd7 )
	ROM_LOAD( "GV113ROM.2", 0xa000, 0x2000, 0x093ee71e )
	ROM_LOAD( "GV113ROM.1", 0xc000, 0x2000, 0x65319da1 )
	ROM_LOAD( "GV113ROM.0", 0xe000, 0x2000, 0x20f3727b )

	ROM_REGION(0x8000)      /* temporary space for graphics */
	ROM_LOAD( "GV113FG3", 0x0000, 0x2000, 0xf3e09a2a )       /* sprites */
	ROM_LOAD( "GV113FG2", 0x2000, 0x2000, 0x5bde03f8 )       /* sprites */
	ROM_LOAD( "GV113FG1", 0x4000, 0x2000, 0x3904746a )       /* sprites */
	ROM_LOAD( "GV113FG0", 0x6000, 0x2000, 0xa2b57805 )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "DROM1", 0xe000, 0x2000, 0x3aa5d107 )
ROM_END



struct GameDriver stooges_driver =
{
	"Three Stooges",
	"3stooges",
	"Fabrice Frances (MAME driver)\nJohn Butler\nMarco Cassili",
	&machine_driver,

	stooges_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gottlieb_nvram_load, gottlieb_nvram_save
};
