/*****************************************************************************
Reactor hardware: very similar to Q*bert with a different memory map...

Thanks to Richard Davies who provided a keyboard/joystick substitute for the
trackball...

...which is now obsolete. BW

Main processor (8088 minimum mode)  memory map.
0000-1fff RAM
2000-2fff sprite programmation (64 sprites)
3000-37ff background ram (32x30 chars)
4000-4fff background object ram
6000-67ff palette ram (palette of 16 colors)
7000-77ff i/o ports (see below)
8000-ffff ROM (reactor's prog)

memory mapped ports:

read:
7000    Dip switch
7001    Inputs 10-17
7002    trackball Horiz. input: -127 (left fastest) to +127 (right fastest), 0=stop
7003    trackball Vert. input: -127 (up fastest) to +127 (down fastest), 0=stop
7004    Inputs 40-47

write:
7000    watchdog timer clear
7001    trackball output (optional)
7002    Outputs 20-27
7003    Flipflop outputs:
		b7: F/B priority
		b6: horiz. flipflop
		b5: vert. flipflop
		b4: Output 33
		b3: coin meter
		b2: knocker
		b1: coin 1
		b0: coin lockout
7004    Outputs 40-47

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

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int reactor_vh_start(void);
void gottlieb_sh_w(int offset, int data);
void gottlieb_characterram_w(int offset, int data);
int gottlieb_sh_init(const char *gamename);
void gottlieb_video_outputs(int offset,int data);
extern unsigned char *gottlieb_paletteram;
extern unsigned char *gottlieb_characterram;
void gottlieb_paletteram_w(int offset,int data);
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);


extern struct MemoryReadAddress gottlieb_sound_readmem[];
extern struct MemoryWriteAddress gottlieb_sound_writemem[];


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x6fff, MRA_RAM },
	{ 0x7000, 0x7000, input_port_0_r },     /* DSW */
	{ 0x7001, 0x7001, input_port_1_r },		/* buttons */		/* JB 971221 */
	{ 0x7002, 0x7002, input_port_2_r },     /* trackball H */	/* JB 971221 */
	{ 0x7003, 0x7003, input_port_3_r },     /* trackball V */	/* JB 971221 */
	{ 0x7004, 0x7004, input_port_4_r },     /* joystick */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x20ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3000, 0x33ff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x4fff, gottlieb_characterram_w, &gottlieb_characterram }, /* bg object ram */
	{ 0x6000, 0x601f, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x7000, 0x7000, MWA_RAM },    /* watchdog timer clear */
	{ 0x7001, 0x7001, MWA_RAM },    /* trackball: not used */
	{ 0x7002, 0x7002, gottlieb_sh_w }, /* sound/speech command */
	{ 0x7003, 0x7003, gottlieb_video_outputs },       /* OUT1 */
	{ 0x7004, 0x7004, MWA_RAM },    /* OUT2 */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/* JB 971221 */
INPUT_PORTS_START( input_ports )
	PORT_START	/* DSW */
	PORT_DIPNAME (0x01, 0x01, "Sound with Logos", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x01, "On" )
	PORT_DIPNAME (0x02, 0x02, "Bounce Chambers Points", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "10" )
	PORT_DIPSETTING (   0x02, "15" )
	PORT_DIPNAME (0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING (   0x04, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_DIPNAME (0x08, 0x08, "Sound with Instructions", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x08, "On" )
	PORT_DIPNAME (0x10, 0x10, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING (   0x10, "Upright" )
	PORT_DIPSETTING (   0x00, "Cocktail" )
	PORT_DIPNAME (0x20, 0x20, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x20, "1 Coin/1 Credit" )
	PORT_DIPNAME (0xc0, 0xc0, "Bonus Ship", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "10000" )
	PORT_DIPSETTING (   0x40, "12000" )
	PORT_DIPSETTING (   0xc0, "15000" )
	PORT_DIPSETTING (   0x80, "20000" )

	PORT_START	/* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Select in Service Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BITX(0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x02, "Off" )
	PORT_DIPSETTING (   0x00, "On" )
	PORT_BIT ( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* trackball h */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_X | IPF_CENTER, 100, 127, 0, 0 )

	PORT_START	/* trackball v */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_Y | IPF_CENTER, 100, 127, 0, 0 )

	PORT_START	/* IN4 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
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
	128,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 0x1000*8, 0x2000*8, 0x3000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x4000, &charlayout, 0, 1 },
	{ 1, 0, &spritelayout, 0, 1 },
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
	reactor_vh_start,
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



ROM_START( reactor_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ROM7", 0x8000, 0x1000, 0xd8f16275 )
	ROM_LOAD( "ROM6", 0x9000, 0x1000, 0x7aadbabf )
	ROM_LOAD( "ROM5", 0xa000, 0x1000, 0x9e2453f0 )
	ROM_LOAD( "ROM4", 0xb000, 0x1000, 0x2f1839e2 )
	ROM_LOAD( "ROM3", 0xc000, 0x1000, 0x70123534 )
	ROM_LOAD( "ROM2", 0xd000, 0x1000, 0xb50b26f3 )
	ROM_LOAD( "ROM1", 0xe000, 0x1000, 0xeaf1c223 )
	ROM_LOAD( "ROM0", 0xf000, 0x1000, 0xe126beca )

	ROM_REGION(0x8000)      /* temporary space for graphics */
	ROM_LOAD( "FG0", 0x0000, 0x1000, 0x80076d89 )       /* sprites */
	ROM_LOAD( "FG1", 0x1000, 0x1000, 0x0577a58b )       /* sprites */
	ROM_LOAD( "FG2", 0x2000, 0x1000, 0xe1ecaede )       /* sprites */
	ROM_LOAD( "FG3", 0x3000, 0x1000, 0x50087b04 )       /* sprites */

	ROM_REGION(0x10000)      /* 64k for sound cpu */
	ROM_LOAD( "SND1", 0xf000, 0x800, 0x1367334b )
		ROM_RELOAD(0x7000, 0x800) /* A15 is not decoded */
	ROM_LOAD( "SND2", 0xf800, 0x800, 0x10e64e0a )
		ROM_RELOAD(0x7800, 0x800) /* A15 is not decoded */
ROM_END



static int hiload(void)
{
	void *f;
	unsigned char *RAM=Machine->memory_region[0];

	if (RAM[0x4D8]!=0x0A) return 0;
	f=osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
	if (f) {
		osd_fread(f,RAM+0x4D8,0x557-0x4D8);
		osd_fclose(f);
	}
	return 1;
}

static void hisave(void)
{
	void *f=osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);
	unsigned char *RAM=Machine->memory_region[0];

	if (f) {
		osd_fwrite(f,RAM+0x4D8,0x557-0x4D8);
		osd_fclose(f);
	}
}



struct GameDriver reactor_driver =
{
	"Reactor",
	"reactor",
	"Fabrice Frances",
	&machine_driver,

	reactor_rom,
	0, 0,   /* rom decode and opcode decode functions */
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload,hisave     /* hi-score load and save */
};
