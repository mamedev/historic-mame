/*****************************************************************************
Reactor hardware: very similar to Q*bert with a different memory map...

Thanks to Richard Davies who provided a keyboard/joystick substitute for the
trackball (now, who wants to add mouse services in Mame ?-)

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

Nota: I've used my own 6502 emulator in order to compute the digital effects
because the clock emulation is much more precise and allows to put timestamps
on amplitude DAC writes. MAME doesn't allow to compute the digital effects in
real time like Euphoric (oops, <ad. mode end>) so the effects are provided
as precomputed samples (some of them are quite big, I should convert them
to 22kHz)  
******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int reactor_vh_start(void);
extern void gottlieb_vh_init_basic_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void gottlieb_sh_w(int offset, int data);
extern void gottlieb_characterram_w(int offset, int data);
extern int gottlieb_sh_init(const char *gamename);
extern void gottlieb_sh_update(void);
extern const char *gottlieb_sample_names[];
extern void gottlieb_output(int offset, int data);
extern int reactor_IN1_r(int offset);
extern int reactor_tb_H_r(int offset);
extern int reactor_tb_V_r(int offset);
extern unsigned char *gottlieb_videoram;
extern unsigned char *gottlieb_paletteram;
extern unsigned char *gottlieb_spriteram;
extern unsigned char *gottlieb_characterram;
extern void gottlieb_videoram_w(int offset,int data);
extern void gottlieb_paletteram_w(int offset,int data);
extern void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x6fff, MRA_RAM },
	{ 0x7000, 0x7000, input_port_0_r },     /* DSW */
	{ 0x7001, 0x7001, reactor_IN1_r },      /* buttons */
	{ 0x7002, 0x7002, reactor_tb_H_r },     /* trackball H */
	{ 0x7003, 0x7003, reactor_tb_V_r },     /* trackball V */
	{ 0x7004, 0x7004, input_port_4_r },     /* joystick */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2fff, MWA_RAM, &gottlieb_spriteram },
	{ 0x3000, 0x37ff, gottlieb_videoram_w, &gottlieb_videoram },
	{ 0x4000, 0x4fff, gottlieb_characterram_w, &gottlieb_characterram }, /* bg object ram */
	{ 0x6000, 0x67ff, gottlieb_paletteram_w, &gottlieb_paletteram },
	{ 0x7000, 0x7000, MWA_RAM },    /* watchdog timer clear */
	{ 0x7001, 0x7001, MWA_RAM },    /* trackball: not used */
	{ 0x7002, 0x7002, gottlieb_sh_w }, /* sound/speech command */
	{ 0x7003, 0x7003, gottlieb_output },       /* OUT1 */
	{ 0x7004, 0x7004, MWA_RAM },    /* OUT2 */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct InputPort input_ports[] =
{
	{       /* DSW */
		0x0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* buttons */
		0x02,   /* test mode off */
		{ OSD_KEY_F2 /* select */, 0,0,0,0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: handled by reactor_tb_H_r() */
		0x0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* trackball: handled by reactor_tb_V_r() */
		0x0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* buttons */
		0x00,
		{ OSD_KEY_1, OSD_KEY_2,         /* energy & decoy player 1. */
		  OSD_KEY_ALT, OSD_KEY_CONTROL, /* decoy & energy player 2. Also for 1 & 2 player start, long plays */
		  OSD_KEY_3, 0 /* coin 2 */, 0, 0 },
		{ 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1, 0, 0, 0, 0 }
	},
	{ -1 }  /* end of table */
};


static struct KEYSet keys[] =
{
        { 4, 1, "PLAYER 1 DECOY" },
        { 4, 0, "PLAYER 1 ENERGY" },
        { 4, 2, "PLAYER 2 DECOY" },
        { 4, 3, "PLAYER 2 ENERGY" },
        { -1 }
};



static struct DSW dsw[] =
{
	{ 0, 0x08, "SOUND WITH INSTRUCTIONS", { "NO","YES" } },
	{ 0, 0x01, "SOUND WITH LOGOS", { "NO", "YES" } },
	{ 0, 0x10, "", { "COCKTAIL", "UPRIGHT" } },
	{ 0, 0x04, "FREE PLAY", { "YES" , "NO" } },
	{ 0, 0x20, "COINS PER CREDIT", { "2", "1" } },
	{ 0, 0x02, "BOUNCE CHAMBERS PTS", { "10", "15" } },
	{ 0, 0xC0, "BONUS SHIP AT", { "10000", "12000", "20000", "15000" } },
/* the following switch must be connected to the IP16 line */
/*	{ 1, 0x2, "TEST MODE", {"ON", "OFF"} },*/
	{ -1 }
};


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
	{ 0 * 16, 1 * 16, 2 * 16, 3 * 16, 4 * 16, 5 * 16, 6 * 16, 7 * 16,
		8 * 16, 9 * 16, 10 * 16, 11 * 16, 12 * 16, 13 * 16, 14 * 16, 15 * 16 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
}; 

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x8005, &charlayout, 16, 2 }, /* characters in rom for Mame's messages in white & yellow */
	{ 0, 0x4000, &charlayout, 0, 1 },
	{ 1, 0, &spritelayout, 0, 1 },
	{ 0, 0, &fakelayout, 3*16, 16 }, /* 256 colors to pick in */
	{ -1 } /* end of array */
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
		}
	},
	60,     /* frames / second */
	0,      /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	256,256+3*16,        /* 256 for colormap, 1*16 for the game, 2*16 for the dsw menu. Silly, isn't it ? */
	gottlieb_vh_init_basic_color_palette,
	
	0,      /* init vh */
	reactor_vh_start,
	generic_vh_stop,
	gottlieb_vh_screenrefresh,

	/* sound hardware */
	0,      /* samples */
	0,
	0,
	0,
	gottlieb_sh_update
};

ROM_START( reactor_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ROM0",  0xf000, 0x1000 )
	ROM_LOAD( "ROM1",  0xe000, 0x1000 )
	ROM_LOAD( "ROM2",  0xd000, 0x1000 )
	ROM_LOAD( "ROM3",  0xc000, 0x1000 )
	ROM_LOAD( "ROM4",  0xb000, 0x1000 )
	ROM_LOAD( "ROM5",  0xa000, 0x1000 )
	ROM_LOAD( "ROM6",  0x9000, 0x1000 )
	ROM_LOAD( "ROM7",  0x8000, 0x1000 )

	ROM_REGION(0x8000)      /* temporary space for graphics */
	ROM_LOAD( "FG0",  0x0000, 0x1000 )       /* sprites */
	ROM_LOAD( "FG1",  0x1000, 0x1000 )       /* sprites */
	ROM_LOAD( "FG2",  0x2000, 0x1000 )       /* sprites */
	ROM_LOAD( "FG3",  0x3000, 0x1000 )       /* sprites */
ROM_END


struct GameDriver reactor_driver =
{
	"reactor",
	&machine_driver,

	reactor_rom,
	0, 0,   /* rom decode and opcode decode functions */
	gottlieb_sample_names,

	input_ports, dsw, keys,

	(char *)1,
	0,0,    /* palette, colortable */

	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,    /* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,       /* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0,1,      /* white & yellow for dsw menu */
	8*11,8*20,1, /* paused message displayed at X,Y and color */

	0,0     /* hi-score load and save */
};
