/***************************************************************************

Midway?? Z80 board memory map (preliminary)

0000-1fff ROM
2000-23ff RAM
2400-3fff Video RAM
4000-57ff ROM (some clones use more, others less)

Games which are referenced by this driver:
------------------------------------------
Astro Invader (astinvad)
Kamikaze (kamikaze)

I/O ports:
read:
01        IN0
02        IN1
03        bit shift register read

write:
02        shift amount
03        sound
04        shift data
05        sound
06        ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int  astinvad_interrupt(void);

extern unsigned char *astinvad_videoram;

void astinvad_videoram_w(int offset,int data);   /* L.T */

int  astinvad_vh_start(void);
void astinvad_vh_stop(void);
void astinvad_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void astinvad_sh_port4_w(int offset,int data);
void astinvad_sh_port5_w(int offset,int data);
void astinvad_sh_update(void);


static struct MemoryWriteAddress astinvad_writemem[] = /* L.T */ /* Whole function */
{
	{ 0x1c00, 0x23ff, MWA_RAM },
        { 0x2400, 0x3fff, astinvad_videoram_w, &astinvad_videoram },
	{ 0x4000, 0x4fff, MWA_RAM, },
	{ 0x0000, 0x1bff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress astinvad_readmem[] =
{
	{ 0x1c00, 0x3fff, MRA_RAM },
	{ 0x0000, 0x1bff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ -1 }  /* end of table */
};

/* LT 20-3-1998 */
static struct IOReadPort astinvad_readport[] =
{
	{ 0x08, 0x08, input_port_0_r },
	{ 0x09, 0x09, input_port_1_r },
	{ -1 }  /* end of table */
};

/* LT 20-3-1998 */
static struct IOWritePort astinvad_writeport[] =
{
        { 0x04, 0x04, astinvad_sh_port4_w },
        { 0x05, 0x05, astinvad_sh_port5_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( astinvad_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Extra Live", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "1 Coin  Per 2 Plays" )
	PORT_DIPSETTING(    0x00, "1 Coin  Per Play" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Left */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Left */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Right */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Right */
INPUT_PORTS_END

static unsigned char astinvad_palette[] = /* L.T */
{
	0x00,0x00,0x00,
	0x20,0x20,0xff,
	0x20,0xff,0x20,
	0x20,0xff,0xff,
	0xff,0x20,0x20,
	0xff,0x20,0xff,
	0xff,0xff,0x20,
	0xff,0xff,0xff
};


static struct Samplesinterface samples_interface =
{
	9       /* 9 channels */
};



static struct MachineDriver astinvad_machine_driver = /* LT */
{
	/* basic machine hardware */
	{
		{
                        CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			astinvad_readmem,astinvad_writemem,astinvad_readport,astinvad_writeport,
			interrupt,1    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(astinvad_palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
        astinvad_vh_start,
        astinvad_vh_stop,
        astinvad_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( astinvad_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ai_cpu_1.rom", 0x0000, 0x0400, 0xfea7ea35 )
	ROM_LOAD( "ai_cpu_2.rom", 0x0400, 0x0400, 0x90bc4412 )
	ROM_LOAD( "ai_cpu_3.rom", 0x0800, 0x0400, 0x4ef7a34d )
	ROM_LOAD( "ai_cpu_4.rom", 0x0c00, 0x0400, 0xb49ad24c )
	ROM_LOAD( "ai_cpu_5.rom", 0x1000, 0x0400, 0x2e35e513 )
	ROM_LOAD( "ai_cpu_6.rom", 0x1400, 0x0400, 0x83a0c7c8 )
	ROM_LOAD( "ai_cpu_7.rom", 0x1800, 0x0400, 0xd9259ef3 )
ROM_END

ROM_START( kamikaze_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "km01", 0x0000, 0x0800, 0xc0183e5c )
	ROM_LOAD( "km02", 0x0800, 0x0800, 0xd8397221 )
	ROM_LOAD( "km03", 0x1000, 0x0800, 0x15f835c0 )
	ROM_LOAD( "km04", 0x1800, 0x0800, 0x856b59e5 )
ROM_END

static const char *astinvad_sample_names[] =
{
	"*invaders",
	"0.SAM",
	"1.SAM",
	"2.SAM",
	"3.SAM",
	"4.SAM",
	"5.SAM",
	"6.SAM",
	"7.SAM",
	"8.SAM",
	0       /* end of array */
};

/* LT 20-3-1998 */
struct GameDriver astinvad_driver =
{
	__FILE__,
	0,
	"astinvad",
	"Astro Invader",
	"1980",
	"Stern",
	"Lee Taylor\n",
	0,
	&astinvad_machine_driver,

	astinvad_rom,
	0, 0,
	astinvad_sample_names,
	0,      /* sound_prom */

	astinvad_input_ports,

	0, astinvad_palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

/* LT 20 - 3 19978 */
struct GameDriver kamikaze_driver =
{
	__FILE__,
	&astinvad_driver,
	"kamikaze",
	"Kamikaze",
	"1979",
	"Leijac Corporation",
	"Lee Taylor\n",
	0,
	&astinvad_machine_driver,

	kamikaze_rom,
	0, 0,
	astinvad_sample_names,
	0,      /* sound_prom */

	astinvad_input_ports,

	0, astinvad_palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

