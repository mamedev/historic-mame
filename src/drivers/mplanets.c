/***************************************************************************

Mad Planets' memory map

Thanks to Richard Davies who provided a keyboard/joystick substitute for the
dialer...
...now obsolete BW

Main processor (8088 minimum mode) memory map.
0000-0fff RAM
1000-1fff RAM
2000-2fff RAM
3000-37ff sprite programmation (64 sprites)
3800-3fff background ram (32x30 chars)
4000-4fff background object ram ?
5000-57ff palette ram (palette of 16 colors)
5800-5fff i/o ports (see below)
6000-ffff ROM (mad planets' prog)

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

Sound processor (6502) memory map:
0000-0fff RIOT (6532)
1000-1fff amplitude DAC
2000-2fff SC01 voice chip
3000-3fff voice clock DAC
4000-4fff socket expansion
5000-5fff socket expansion
6000-6fff socket expansion
7000-7fff PROM
(repeated in 8000-ffff, A15 only used in socket expansion)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int mplanets_vh_start(void);
void gottlieb_sh_w(int offset, int data);
void gottlieb_video_outputs(int offset,int data);
extern unsigned char *gottlieb_paletteram;
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);

extern struct MemoryReadAddress gottlieb_sound_readmem[];
extern struct MemoryWriteAddress gottlieb_sound_writemem[];

int gottlieb_nvram_load(void);
void gottlieb_nvram_save(void);



static int trak;

int mplanets_trak_r(int offset)
{
	return input_port_2_r(offset) - trak;
}

void mplanets_trak_w(int offset,int data)
{
	/* recenter the dial */
	trak = input_port_2_r(offset);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },     /* DSW */
	{ 0x5801, 0x5801, input_port_1_r },     /* buttons */
	{ 0x5802, 0x5802, MRA_NOP },     		/* trackball H: not used */
	{ 0x5803, 0x5803, mplanets_trak_r },     /* trackball V: dialer */
	{ 0x5804, 0x5804, input_port_3_r },     /* joystick */
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3bff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, MWA_RAM }, /* bg object ram... ? not used ? */
	{ 0x5000, 0x501f, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x5800, 0x5800, MWA_RAM },    /* watchdog timer clear */
	{ 0x5801, 0x5801, mplanets_trak_w },    /* trackball */
	{ 0x5802, 0x5802, gottlieb_sh_w }, /* sound/speech command */
	{ 0x5803, 0x5803, gottlieb_video_outputs },       /* OUT1 */
	{ 0x5804, 0x5804, MWA_RAM },    /* OUT2 */
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/* JB 971221 */
INPUT_PORTS_START( input_ports )
	PORT_START	/* DSW */
	PORT_DIPNAME (0x08, 0x00, "Round Select", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x08, "On" )
	PORT_DIPNAME (0x01, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING (   0x01, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x14, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x10, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x14, "Free Play" )
	PORT_DIPNAME (0x20, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "3" )
	PORT_DIPSETTING (   0x20, "5" )
	PORT_DIPNAME (0x02, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "10000" )
	PORT_DIPSETTING (   0x02, "12000" )
	PORT_DIPNAME (0xc0, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING (   0x40, "Easy" )
	PORT_DIPSETTING (   0x00, "Normal" )
	PORT_DIPSETTING (   0x80, "Hard" )
	PORT_DIPSETTING (   0xc0, "Hardest" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x3c, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_SERVICE, "Select in Service Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x80, "Off" )
	PORT_DIPSETTING (   0x00, "On" )

	PORT_START	/* trackball v (dial) */
	PORT_ANALOGX ( 0xff, 0x00, IPT_DIAL, 15, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 4 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )
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
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 1 },
	{ 1, 0x2000, &spritelayout, 0, 1 },
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
			3579545/4,        /* could it be /2 ? */
			2,             /* memory region #2 */
			gottlieb_sound_readmem,gottlieb_sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	16, 16,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,      /* init vh */
	mplanets_vh_start,
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



ROM_START( mplanets_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ROM4", 0x6000, 0x2000, 0xf09b30bb )
	ROM_LOAD( "ROM3", 0x8000, 0x2000, 0x52223738 )
	ROM_LOAD( "ROM2", 0xa000, 0x2000, 0xe406bbb6 )
	ROM_LOAD( "ROM1", 0xc000, 0x2000, 0x385a7fa6 )
	ROM_LOAD( "ROM0", 0xe000, 0x2000, 0x29df430b )

	ROM_REGION(0xA000)      /* temporary space for graphics */
	ROM_LOAD( "BG0", 0x0000, 0x1000, 0xb85b00c3 )
	ROM_LOAD( "BG1", 0x1000, 0x1000, 0x175bc547 )
	ROM_LOAD( "FG3", 0x2000, 0x2000, 0x7c6a72bc )       /* sprites */
	ROM_LOAD( "FG2", 0x4000, 0x2000, 0x6ab56cc7 )       /* sprites */
	ROM_LOAD( "FG1", 0x6000, 0x2000, 0x16c596b7 )       /* sprites */
	ROM_LOAD( "FG0", 0x8000, 0x2000, 0x96727f86 )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "SND1", 0xf000, 0x800, 0xca36c072 )
	ROM_RELOAD(0x7000, 0x800) /* A15 is not decoded */
	ROM_LOAD( "SND2", 0xf800, 0x800, 0x66461044 )
	ROM_RELOAD(0x7800, 0x800) /* A15 is not decoded */

ROM_END



struct GameDriver mplanets_driver =
{
	"Mad Planets",
	"mplanets",
	"Fabrice Frances",
	&machine_driver,

	mplanets_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	gottlieb_nvram_load, gottlieb_nvram_save
};
