/***************************************************************************

williams (preliminary)

I still need to know:

* How does the blitter chip work?
* How often does the game generate interrupts?
* What is the value at 0xcb00?
* How do I display the fonts?
* Anybody have specs for the 6802/6808?
* Some sample sounds would be nice...

Thanks!

---------

Game CPU

0000-8fff bank-switchable between ROM & RAM.  Writes always go to RAM.
0000-97ff screen memory (304 x 256 x 4 bits/pixel)
9000-bfff RAM
c000-cbff I/O
cc00-cfff CMOS RAM, cc00-ccff write-protected
d000-ffff ROM

memory mapped ports:

read:
f000		sound ROM
c804-c807	game controller
c80c-c80f	coin door, sound board, LEDs
9800		palette RAM
da51/f60b	palette ROM
f50b		select bank 1
f507		select bank 3
f503		select bank 2
f4ff		select bank 7
f50d		select bank in acca


write:
c000 color_registers
c804 widget_pia_dataa
c805 widget_pia_ctrla
c806 widget_pia_datab
c807 widget_pia_ctrlb
c80c rom_pia_dataa
c80d rom_pia_ctrla
c80e rom_pia_datab
c80f rom_pia_ctrlb
C900 rom_enable_scr_ctrl
CA00 start_blitter
CA01 blitter_mask
CA02 blitter_source
CA04 blitter_dest
CA06 blitter_w_h
cbff	watchdog

I/O ports:


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define NO_WILLIAMS_SOUND

extern int williams_vh_start(void);
extern void williams_vh_screenrefresh(struct osd_bitmap *bitmap);
void williams_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int williams_init_machine(const char *gamename);

extern void williams_rom_enable(int offset,int data);
extern int williams_paged_read(int offset);
extern int williams_paged_read_ram(int offset);
extern void williams_paged_write(int offset,int data);
extern int williams_pia_read(int offset);
extern void williams_pia_write(int offset,int data);
extern int williams_rand(int offset);
extern void williams_watchdog(int offset,int data);
extern int williams_interrupt(void);
extern void williams_colorram_w(int offset,int data);
extern void williams_blitter_w(int offset,int data);

extern void williams_sh_update();
extern int williams_sound_interrupt(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0xd000, 0xffff, MRA_ROM },
	{ 0x0000, 0x8fff, williams_paged_read },
	{ 0x9000, 0x97ff, williams_paged_read_ram },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc80f, williams_pia_read },
	{ 0xcb00, 0xcb00, MRA_RAM /* williams_rand */ },
	{ 0xcc00, 0xcfff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x97ff, williams_paged_write },	/* RAM write */
	{ 0x9800, 0xbfff, MWA_RAM },			/* RAM write */
	{ 0xc000, 0xc00f, williams_colorram_w },	/* video and color RAM */
	{ 0xc804, 0xc80f, williams_pia_write },
        { 0xcbff, 0xcbff, williams_watchdog },		/* watchdog timer (write 0x39) */
	{ 0xca00, 0xca07, williams_blitter_w },		/* the blitter! */
	{ 0xcc00, 0xcfff, MWA_RAM },			/* CMOS (1st 256 bytes write-prot) */
	{ 0xc900, 0xc900, williams_rom_enable },	/* ROM enable */
	{ 0xd000, 0xffff, MWA_ROM },			/* ROM */
	{ -1 }	/* end of table */
};


#ifdef WILLIAMS_SOUND
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0,    0x80,   MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0,    0x80,   MWA_RAM },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};
#endif

/*
 * KEYS
 *
 * port 1: fire, thrust, ~smart bomb, hyperspace, 2 player, 1 player, reverse, down
 * port 2: up, inviso ... (smart bomb player 2??)
 * port 3: auto up, advance, right coin, high score reset, left coin, center coin, slam
 *
 */
static struct InputPort stargate_input_ports[] =
{
	{	/* Controls */
		0x00,	/* default_value */
		{ OSD_KEY_O, OSD_KEY_I, OSD_KEY_J, OSD_KEY_SPACE, OSD_KEY_2, OSD_KEY_1, OSD_KEY_A, OSD_KEY_X },
		{ OSD_JOY_FIRE, OSD_JOY_RIGHT, 0, 0, 0, 0, OSD_JOY_LEFT, OSD_JOY_DOWN }
	},
	{	/* More controls */
		0x00,	/* default_value */
		{ OSD_KEY_S, OSD_KEY_M, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_UP, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ 0, OSD_KEY_ENTER, OSD_KEY_5, 0, OSD_KEY_3, OSD_KEY_4, OSD_KEY_9, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct KEYSet stargate_keys[] =
{
        { 1, 0, "MOVE UP" },
        { 0, 7, "MOVE DOWN"  },
        { 0, 6, "REVERSE" },
        { 0, 1, "THRUST" },
        { 0, 0, "FIRE" },
        { 0, 2, "SMART BOMB" },
        { 0, 3, "HYPERSPACE" },
        { 1, 1, "INVISO" },
        { -1 }
};

/*
 * KEYS
 *
 * port 1: up, down, left, right, 2 player, 1 player, fire up, fire down,
 * port 2: fire left, fire right
 * port 3: auto up, advance, right coin, high score reset, left coin, center coin, slam
 *
 */
static struct InputPort robotron_input_ports[] =
{
	{	/* Controls */
		0x00,	/* default_value */
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_2, OSD_KEY_1, OSD_KEY_E, OSD_KEY_D },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0, 0, 0 }
	},
	{	/* More controls */
		0x00,	/* default_value */
		{ OSD_KEY_S, OSD_KEY_F, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ 0, OSD_KEY_ENTER, OSD_KEY_5, 0, OSD_KEY_3, OSD_KEY_4, OSD_KEY_9, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

/*
 * KEYS
 *
 * port 1: left, right, flap, 2left, 2right, 2flap, 2 player start, 1 player start
 * port 2: ?
 * port 3: auto up, advance, right coin, high score reset, left coin, center coin, slam
 *
 */
static struct InputPort joust_input_ports[] =
{
	{	/* Controls */
		0x00,	/* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_CONTROL, 0, OSD_KEY_2, OSD_KEY_1, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_FIRE, 0, 0, 0, 0, 0 }
	},
	{	/* More controls */
		0x00,	/* default_value */
		{ OSD_KEY_S, OSD_KEY_F, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ 0, OSD_KEY_ENTER, OSD_KEY_5, 0, OSD_KEY_3, OSD_KEY_4, OSD_KEY_9, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct InputPort williamstest_input_ports[] =
{
	{	/* Controls */
		0x00,	/* default_value */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4, OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, OSD_KEY_8 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* More controls */
		0x00,	/* default_value */
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ OSD_KEY_A, OSD_KEY_S, OSD_KEY_D, OSD_KEY_F, OSD_KEY_G, OSD_KEY_H, OSD_KEY_J, OSD_KEY_K },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* coin door */
		0x00,	/* default_value */
		{ OSD_KEY_Z, OSD_KEY_X, OSD_KEY_C, OSD_KEY_V, OSD_KEY_B, OSD_KEY_N, OSD_KEY_M, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct KEYSet keys[] =
{
        { 4, 0, "MOVE UP" },
        { 4, 3, "MOVE LEFT"  },
        { 4, 1, "MOVE RIGHT" },
        { 4, 2, "MOVE DOWN" },
        { 4, 5, "FIRE" },
        { -1 }
};


/* williams -- ROM SPV-1.3C (4K) */
static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 2 bits per pixel */
	{ 0, 4},	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 }, /* characters are rotated 90 degrees */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	16*8	/* every char takes 16 bytes */
};

/* williams -- ROM SPV-2.3F (8K) */
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 39 * 8, 38 * 8, 37 * 8, 36 * 8, 35 * 8, 34 * 8, 33 * 8, 32 * 8,
			7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3, 
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 bytes */
};


/* bumped color table to 69 entries -- see color table for details */
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 69 /*32*/ },
	{ 1, 0x1000, &spritelayout, 0, 69 /*32*/ },
	{ -1 } /* end of array */
};

static int hi_loaded = 0;

static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if (!hi_loaded)
	{
		FILE *f;

		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0xcc00],1,0x400,f);
			fclose(f);
			hi_loaded = 1;
		}
	}
	return 1;
}



static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0xcc00],1,0x400,f);
		fclose(f);
	}
}

static const char *sample_names[] =
{
	"0.sam", "1.sam", "2.sam", "3.sam",
	"4.sam", "5.sam", "6.sam", "7.sam", 
	"8.sam", "9.sam", "a.sam", "b.sam",
	"c.sam", "d.sam", "e.sam", "f.sam", 

	"10.sam", "11.sam", "12.sam", "13.sam",
	"14.sam", "15.sam", "16.sam", "17.sam", 
	"18.sam", "19.sam", "1a.sam", "1b.sam", 
	"1c.sam", "1d.sam", "1e.sam", "1f.sam", 

	"20.sam", "21.sam", "22.sam", "23.sam",
	"24.sam", "25.sam", "26.sam", "27.sam", 
	"28.sam", "29.sam", "2a.sam", "2b.sam", 
	"2c.sam", "2d.sam", "2e.sam", "2f.sam", 

	"30.sam", "31.sam", "32.sam", "33.sam",
	"34.sam", "35.sam", "36.sam", "37.sam", 
	"38.sam", "39.sam", "3a.sam", "3b.sam", 
	"3c.sam", "3d.sam", "3e.sam", 0,
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,			/* 1 Mhz */
			0,				/* memory region */
			readmem,			/* MemoryReadAddress */
			writemem,			/* MemoryWriteAddress */
			0,				/* IOReadPort */
			0,				/* IOWritePort */
			williams_interrupt,		/* interrupt routine */
			16				/* interrupts per frame */
		}
#ifdef WILLIAMS_SOUND
		/** future -- sound **/
		,{
			CPU_M6809 | CPU_AUDIO_CPU ,
			1000000,			/* 1 Mhz */
			1,				/* memory region */
			sound_readmem,			/* MemoryReadAddress */
			sound_writemem,			/* MemoryWriteAddress */
			0,				/* IOReadPort */
			0,				/* IOWritePort */
			williams_sound_interrupt,	/* interrupt routine */
			1				/* interrupts per frame */
		}
#endif
	},
	60,						/* frames per second */
	williams_init_machine,		                /* init machine routine */

	/* video hardware */
	304, 256,					/* screen_width, screen_height */
	 { 0, 0, 302, 255 },	                        /* struct rectangle visible_area */
	gfxdecodeinfo,				        /* GfxDecodeInfo * */
	256,						/* total colors */
	256,						/* color table length */
	williams_vh_convert_color_prom,			/* convert color prom routine */

	0,						/* vh_init routine */
	williams_vh_start,			        /* vh_start routine */
	generic_vh_stop,			        /* vh_stop routine */
	williams_vh_screenrefresh,	                /* vh_update routine */

	/* sound hardware */
	0,						/* pointer to samples */
	0,						/* sh_init routine */
	0,						/* sh_start routine */
	0,						/* sh_stop routine */
	williams_sh_update			        /* sh_update routine */
};

ROM_START( stargate_rom )
	ROM_REGION(0x20000)	/* 1st 64k is ROM, 2nd 64K is RAM */
	ROM_LOAD( "stargate.01", 0x0000, 0x1000 )
	ROM_LOAD( "stargate.02", 0x1000, 0x1000 )
	ROM_LOAD( "stargate.03", 0x2000, 0x1000 )
	ROM_LOAD( "stargate.04", 0x3000, 0x1000 )
	ROM_LOAD( "stargate.05", 0x4000, 0x1000 )
	ROM_LOAD( "stargate.06", 0x5000, 0x1000 )
	ROM_LOAD( "stargate.07", 0x6000, 0x1000 )
	ROM_LOAD( "stargate.08", 0x7000, 0x1000 )
	ROM_LOAD( "stargate.09", 0x8000, 0x1000 )
	ROM_LOAD( "stargate.10", 0xd000, 0x1000 )
	ROM_LOAD( "stargate.11", 0xe000, 0x1000 )
	ROM_LOAD( "stargate.12", 0xf000, 0x1000 )
#ifdef WILLIAMS_SOUND
	ROM_REGION(0x10000)
	ROM_LOAD( "stargate.snd", 0xf000, 0x1000 )
#endif
ROM_END

ROM_START( robotron_rom )
	ROM_REGION(0x20000)	/* 1st 64k is ROM, 2nd 64K is RAM */
	ROM_LOAD( "robotron.01", 0x0000, 0x1000 )
	ROM_LOAD( "robotron.02", 0x1000, 0x1000 )
	ROM_LOAD( "robotron.03", 0x2000, 0x1000 )
	ROM_LOAD( "robotron.04", 0x3000, 0x1000 )
	ROM_LOAD( "robotron.05", 0x4000, 0x1000 )
	ROM_LOAD( "robotron.06", 0x5000, 0x1000 )
	ROM_LOAD( "robotron.07", 0x6000, 0x1000 )
	ROM_LOAD( "robotron.08", 0x7000, 0x1000 )
	ROM_LOAD( "robotron.09", 0x8000, 0x1000 )
	ROM_LOAD( "robotron.10", 0xd000, 0x1000 )
	ROM_LOAD( "robotron.11", 0xe000, 0x1000 )
	ROM_LOAD( "robotron.12", 0xf000, 0x1000 )
#ifdef WILLIAMS_SOUND
	ROM_REGION(0x10000)
	ROM_LOAD( "robotron.snd", 0xf000, 0x1000 )
#endif
ROM_END

ROM_START( joust_rom )
	ROM_REGION(0x20000)	/* 1st 64k is ROM, 2nd 64K is RAM */
	ROM_LOAD( "joust.01", 0x0000, 0x1000 )
	ROM_LOAD( "joust.02", 0x1000, 0x1000 )
	ROM_LOAD( "joust.03", 0x2000, 0x1000 )
	ROM_LOAD( "joust.04", 0x3000, 0x1000 )
	ROM_LOAD( "joust.05", 0x4000, 0x1000 )
	ROM_LOAD( "joust.06", 0x5000, 0x1000 )
	ROM_LOAD( "joust.07", 0x6000, 0x1000 )
	ROM_LOAD( "joust.08", 0x7000, 0x1000 )
	ROM_LOAD( "joust.09", 0x8000, 0x1000 )
	ROM_LOAD( "joust.10", 0xd000, 0x1000 )
	ROM_LOAD( "joust.11", 0xe000, 0x1000 )
	ROM_LOAD( "joust.12", 0xf000, 0x1000 )
#ifdef WILLIAMS_SOUND
	ROM_REGION(0x10000)
	ROM_LOAD( "joust.snd", 0xf000, 0x1000 )
#endif
ROM_END

ROM_START( splat_rom )
	ROM_REGION(0x20000)	/* 1st 64k is ROM, 2nd 64K is RAM */
	ROM_LOAD( "splat.01", 0x0000, 0x1000 )
	ROM_LOAD( "splat.02", 0x1000, 0x1000 )
	ROM_LOAD( "splat.03", 0x2000, 0x1000 )
	ROM_LOAD( "splat.04", 0x3000, 0x1000 )
	ROM_LOAD( "splat.05", 0x4000, 0x1000 )
	ROM_LOAD( "splat.06", 0x5000, 0x1000 )
	ROM_LOAD( "splat.07", 0x6000, 0x1000 )
	ROM_LOAD( "splat.08", 0x7000, 0x1000 )
	ROM_LOAD( "splat.09", 0x8000, 0x1000 )
	ROM_LOAD( "splat.10", 0xd000, 0x1000 )
	ROM_LOAD( "splat.11", 0xe000, 0x1000 )
	ROM_LOAD( "splat.12", 0xf000, 0x1000 )
ROM_END

ROM_START( sinistar_rom )
	ROM_REGION(0x20000)	/* 1st 64k is ROM, 2nd 64K is RAM */
	ROM_LOAD( "sinistar.01", 0x0000, 0x1000 )
	ROM_LOAD( "sinistar.02", 0x1000, 0x1000 )
	ROM_LOAD( "sinistar.03", 0x2000, 0x1000 )
	ROM_LOAD( "sinistar.04", 0x3000, 0x1000 )
	ROM_LOAD( "sinistar.05", 0x4000, 0x1000 )
	ROM_LOAD( "sinistar.06", 0x5000, 0x1000 )
	ROM_LOAD( "sinistar.07", 0x6000, 0x1000 )
	ROM_LOAD( "sinistar.08", 0x7000, 0x1000 )
	ROM_LOAD( "sinistar.09", 0x8000, 0x1000 )
	ROM_LOAD( "sinistar.10", 0xe000, 0x1000 )
	ROM_LOAD( "sinistar.11", 0xf000, 0x1000 )
ROM_END

ROM_START( bubbles_rom )
	ROM_REGION(0x20000)	/* 1st 64k is ROM, 2nd 64K is RAM */
	ROM_LOAD( "bubbles.01", 0x0000, 0x1000 )
	ROM_LOAD( "bubbles.02", 0x1000, 0x1000 )
	ROM_LOAD( "bubbles.03", 0x2000, 0x1000 )
	ROM_LOAD( "bubbles.04", 0x3000, 0x1000 )
	ROM_LOAD( "bubbles.05", 0x4000, 0x1000 )
	ROM_LOAD( "bubbles.06", 0x5000, 0x1000 )
	ROM_LOAD( "bubbles.07", 0x6000, 0x1000 )
	ROM_LOAD( "bubbles.08", 0x7000, 0x1000 )
	ROM_LOAD( "bubbles.09", 0x8000, 0x1000 )
	ROM_LOAD( "bubbles.10", 0xd000, 0x1000 )
	ROM_LOAD( "bubbles.11", 0xe000, 0x1000 )
	ROM_LOAD( "bubbles.12", 0xf000, 0x1000 )
ROM_END

struct GameDriver stargate_driver =
{
	"stargate",
	&machine_driver,		/* MachineDriver * */

	stargate_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	sample_names,			/* samplenames */

	stargate_input_ports,	        /* InputPort  */
	0,		                /* DSW        */
        stargate_keys,                  /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x02, 0x01,			/* white_text, yellow_text for DIP switch menu */
	8*11, 8*20, 0x01,		/* paused_x, paused_y, paused_color for PAUSED */

	hiload,				/* highscore load routine */
	hisave				/* highscore save routine */
};

struct GameDriver robotron_driver =
{
	"robotron",
	&machine_driver,		/* MachineDriver * */

	robotron_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	sample_names,			/* samplenames */

	robotron_input_ports,	        /* InputPort  */
	0,		                /* DSW        */
        keys,                           /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x02, 0x01,			/* white_text, yellow_text for DIP switch menu */
	8*11, 8*20, 0x01,		/* paused_x, paused_y, paused_color for PAUSED */

	hiload,				/* highscore load routine */
	hisave				/* highscore save routine */
};

struct GameDriver joust_driver =
{
	"joust",
	&machine_driver,		/* MachineDriver * */

	joust_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	sample_names,			/* samplenames */

	joust_input_ports,	        /* InputPort  */
	0,		                /* DSW        */
        keys,                           /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x02, 0x01,			/* white_text, yellow_text for DIP switch menu */
	8*11, 8*20, 0x01,		/* paused_x, paused_y, paused_color for PAUSED */

	hiload,				/* highscore load routine */
	hisave				/* highscore save routine */
};

struct GameDriver splat_driver =
{
	"splat",
	&machine_driver,		/* MachineDriver * */

	splat_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	sample_names,			/* samplenames */

	williamstest_input_ports,	/* InputPort  */
	0,		                /* DSW        */
        keys,                           /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x02, 0x01,			/* white_text, yellow_text for DIP switch menu */
	8*11, 8*20, 0x01,		/* paused_x, paused_y, paused_color for PAUSED */

	hiload,				/* highscore load routine */
	hisave				/* highscore save routine */
};

struct GameDriver sinistar_driver =
{
	"sinistar",
	&machine_driver,		/* MachineDriver * */

	sinistar_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	sample_names,			/* samplenames */

	williamstest_input_ports,	/* InputPort  */
	0,		                /* DSW        */
        keys,                           /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x02, 0x01,			/* white_text, yellow_text for DIP switch menu */
	8*11, 8*20, 0x01,		/* paused_x, paused_y, paused_color for PAUSED */

	hiload,				/* highscore load routine */
	hisave				/* highscore save routine */
};

struct GameDriver bubbles_driver =
{
	"bubbles",
	&machine_driver,		/* MachineDriver * */

	bubbles_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	sample_names,			/* samplenames */

	williamstest_input_ports,	/* InputPort  */
	0,		                /* DSW        */
        keys,                           /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x02, 0x01,			/* white_text, yellow_text for DIP switch menu */
	8*11, 8*20, 0x01,		/* paused_x, paused_y, paused_color for PAUSED */

	hiload,				/* highscore load routine */
	hisave				/* highscore save routine */
};

