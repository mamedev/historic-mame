/***************************************************************************

Tutankham :  memory map (preliminary)

I include here the document based on Rob Jarrett's research because it's
really exaustive.



Tutankham Emu Info
------------------

By Rob Jarrett
robj@astound.com (until June 20, 1997)
or robncait@inforamp.net

Special thanks go to Pete Custerson for the schematics!!


I've recently been working on an emulator for Tutankham. Unfortunately, time and resources
are not on my side so I'd like to provide anyone with the technical information I've gathered
so far, that way someone can finish the project.

First of all, I'd like to say that I've had no prior experience in writing an emulator, and
my hardware knowledge is weak. I've managed to find out a fair amount by looking at the
schematics of the game and the disassembled ROMs. Using the USim C++ 6809 core I have the
game sort of up and running, albeit in a pathetic state. It's not playable, and crashes after
a short amount of time. I don't feel the source code is worth releasing because of the bad
design; I was using it as a testing bed and anticipated rewriting everything in the future.

Here's all the info I know about Tutankham:

Processor: 6809
Sound: Z80 slave w/2 AY3910 sound chips
Graphics: Bitmapped display, no sprites (!)
Memory Map:

Address		R/W	Bits		Function
------------------------------------------------------------------------------------------------------
$0000-$7fff				Video RAM
					- Screen is stored sideways, 256x256 pixels
					- 1 byte=2 pixels
		R/W	aaaaxxxx	- leftmost pixel palette index
		R/W	xxxxbbbb	- rightmost pixel palette index
					- **** not correct **** Looks like some of this memory is for I/O state, (I think < $0100)
					  so you might want to blit from $0100-$7fff

$8000-$800f	R/W     aaaaaaaa	Palette colors
					- Don't know how to decode them into RGB values

$8100		W			Not sure
					- Video chip function of some sort
					( split screen x pan position -- TT )

$8120		R			Not sure
					- Read from quite frequently
					- Some sort of video or interrupt thing?
					- Or a random number seed?

$8160					Dip Switch 2
					- Inverted bits (ie. 1=off)
		R	xxxxxxxa	DSWI1
		R
		R			.
		R			.
		R			.
		R
		R
		R	axxxxxxx	DSWI8

$8180					I/O: Coin slots, service, 1P/2P buttons
		R

$81a0					Player 1 I/O
		R

$81c0					Player 2 I/O
		R

$81e0					Dip Switch 1
					- Inverted bits
		R	xxxxxxxa	DSWI1
		R
		R			.
		R			.
		R			.
		R
		R
		R	axxxxxxx	DSWI8

$8200					IST on schematics
					- Enable/disable IRQ
		R/W	xxxxxxxa	- a=1 IRQ can be fired, a=0 IRQ can't be fired

$8202					OUT2 (Coin counter)
		W	xxxxxxxa	- Increment coin counter

$8203					OUT1 (Coin counter)
		W	xxxxxxxa	- Increment coin counter

$8204					Not sure - 401 on schematics
		W

$8205					MUT on schematics
		R/W	xxxxxxxa	- Sound amplification on/off?

$8206					HFF on schematics
		W			- Don't know what it does

$8207					Not sure - can't resolve on schematics
		W

$8300					Graphics bank select
		W	xxxxxaaa	- Selects graphics ROM 0-11 that appears at $9000-9fff
					- But wait! There's only 9 ROMs not 12! I think the PCB allows 12
					  ROMs for patches/mods to the game. Just make 9-11 return 0's

$8600		W			SON on schematics
$8608		R/W			SON on schematics
					- Sound on/off? i.e. Run/halt Z80 sound CPU?

$8700		W	aaaaaaaa	SDA on schematics
					- Sound data? Maybe Z80 polls here and plays the appropriate sound?
					- If so, easy to trigger samples here

$8800-$8fff				RAM
		R/W			- Memory for the program ROMs

$9000-$9fff				Graphics ROMs ra1_1i.cpu - ra1_9i.cpu
		R	aaaaaaaa	- See address $8300 for usage

$a000-$afff				ROM ra1_1h.cpu
		R	aaaaaaaa	- 6809 Code

$b000-$bfff				ROM ra1_2h.cpu
		R	aaaaaaaa	- 6809 Code

$c000-$cfff				ROM ra1_3h.cpu
		R	aaaaaaaa	- 6809 Code

$d000-$dfff				ROM ra1_4h.cpu
		R	aaaaaaaa	- 6809 Code

$e000-$efff				ROM ra1_5h.cpu
		R	aaaaaaaa	- 6809 Code

$f000-$ffff				ROM ra1_6h.cpu
		R	aaaaaaaa	- 6809 Code

Programming notes:

I found that generating an IRQ every 4096 instructions seemed to kinda work. Again, I know
little about emu writing and I think some fooling with this number might be needed.

Sorry I didn't supply the DSW and I/O bits, this info is available elsewhere on the net, I
think at tant or something. I just couldn't remember what they were at this writing!!

If there are any questions at all, please feel free to email me at robj@astound.com (until
June 20, 1997) or robncait@inforamp.net.


BTW, this information is completely free - do as you wish with it. I'm not even sure if it's
correct! (Most of it seems to be). Giving me some credit if credit is due would be nice,
and please let me know about your emulator if you release it.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


unsigned char *tut_videoram;
unsigned char *tut_paletteram;
unsigned char *tut_scrollx;

extern int timeplt_sh_start(void);

extern int tut_bankedrom_r( int offset );
extern void tut_bankselect_w( int offset,int data );
extern int tut_rnd_r( int offset );
extern int tutankhm_init_machine( const char *gamename );
extern int tutankhm_interrupt(void);

extern int  tut_vh_start( void );
extern void tut_vh_stop( void );
extern int  tut_videoram_r( int offset );
extern void tut_videoram_w( int offset, int data );
extern void tut_palette_w( int offset, int data );
extern void tut_vh_convert_color_prom( unsigned char *palette, unsigned char *colortable, const unsigned char *color_prom );
extern void tut_vh_screenrefresh( struct osd_bitmap *bitmap );


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, tut_videoram_r, &tut_videoram },
	{ 0x8800, 0x8fff, MRA_RAM },		/* Game RAM */
	{ 0xa000, 0xffff, MRA_ROM },		/* Game ROM */
	{ 0x9000, 0x9fff, tut_bankedrom_r },	/* Graphics ROMs ra1_1i.cpu - ra1_9i.cpu (See address $8300 for usage) */
	{ 0x81a0, 0x81a0, input_port_2_r },	/* Player 1 I/O */
	{ 0x81c0, 0x81c0, input_port_3_r },	/* Player 2 I/O */
	{ 0x8120, 0x8120, tut_rnd_r },
	{ 0x8180, 0x8180, input_port_1_r },	/* I/O: Coin slots, service, 1P/2P buttons */
	{ 0x8160, 0x8160, input_port_0_r },	/* DSW2 (inverted bits) */
	{ 0x81e0, 0x81e0, input_port_4_r },	/* DSW1 (inverted bits) */
	{ 0x8200, 0x8200, MRA_RAM },	        /* 1 IRQ can be fired, 0 IRQ can't be fired */
	{ 0x8205, 0x8205, MRA_RAM },            /* Sound amplification on/off? */

	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, tut_videoram_w },
	{ 0x8800, 0x8fff, MWA_RAM },		                /* Game RAM */
	{ 0x8000, 0x800f, tut_palette_w, &tut_paletteram },	/* Palette RAM */
	{ 0x8100, 0x8100, MWA_RAM, &tut_scrollx },              /* video x pan hardware reg */
	{ 0x8300, 0x8300, tut_bankselect_w },	                /* Graphics bank select */
	{ 0x8200, 0x8200, interrupt_enable_w },		        /* 1 IRQ can be fired, 0 IRQ can't be fired */
        { 0x8202, 0x8203, MWA_RAM },
        { 0x8205, 0x8207, MWA_RAM },
	{ 0x8700, 0x8700, sound_command_w },
	{ 0xa000, 0xffff, MWA_ROM },		                /* Game ROM */
	{ -1 } /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x2fff, MRA_ROM },            	/* Z80 sound ROM */
	{ 0x3000, 0x3fff, MRA_RAM },            	/* RAM ??? */
	{ 0x4000, 0x4000, AY8910_read_port_0_r },	/* Master AY3-8910 (data ???) */
	{ 0x6000, 0x6000, AY8910_read_port_1_r },	/* Other AY3-8910 (data ???) */
	{ -1 }	/* end of table */

};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x3000, 0x3fff, MWA_RAM },            	/* RAM ??? */
	{ 0x4000, 0x4000, AY8910_write_port_0_w },	/* Master AY3-8910 (data ???) */
	{ 0x5000, 0x5000, AY8910_control_port_0_w },	/* Master AY3-8910 (control ???) */
	{ 0x6000, 0x6000, AY8910_write_port_1_w },	/* Other AY3-8910 (data ???) */
	{ 0x7000, 0x7000, AY8910_control_port_1_w },	/* Other AY3-8910 (control ???) */
	{ 0x8000, 0x8000, MWA_RAM },			/* ??? */
	{ 0x0000, 0x2fff, MWA_ROM },			/* Z80 sound ROM */
	{ -1 } /* end of table */
};

static struct InputPort input_ports[] =
{
	{	/* DSW2 */
		0xff,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 },	/* not affected by keyboard */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* I/O: Coin slots, service, 1P/2P buttons */
		0xff,	/* default_value */
		{ OSD_KEY_3, 0, 0, OSD_KEY_1, OSD_KEY_2, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* Player 1 I/O */
		0xff,	/* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_SPACE, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_FIRE1, OSD_JOY_FIRE2, OSD_JOY_FIRE3, 0 }
	},
	{	/* Player 2 I/O */
		0xff,	/* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_SPACE, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_FIRE1, OSD_JOY_FIRE2, OSD_JOY_FIRE3, 0 }
	},
	{	/* Dip Switch 1 */
		0xff,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 },	/* not affected by keyboard */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct DSW dsw[] =
{

	{ 4, 0x0f, "COIN1", { "FREE PLAY",
			    "4 COINS  3 CREDITS",
			    "4 COINS  1 CREDIT",
			    "3 COINS  4 CREDITS",
			    "3 COINS  2 CREDITS",
			    "3 COINS  1 CREDITS",
			    "2 COINS  5 CREDITS",
			    "2 COINS  3 CREDITS",
			    "2 COINS  1 CREDIT",
			    "1 COIN  7 CREDITS",
			    "1 COIN  6 CREDITS",
			    "1 COIN  5 CREDITS",
			    "1 COIN  4 CREDITS",
			    "1 COIN  3 CREDITS",
			    "1 COIN  2 CREDITS",
			    "1 COIN  1 CREDIT", }, 1 },

	{ 0, 0x03, "LIVES", { "256", "4", "5", "3" }, 1 },
	{ 0, 0x04, "CABINET", { "TABLE", "UPRIGHT" }, 0 },
	{ 0, 0x08, "BONUS", { "30000", "40000" }, 0 },
	{ 0, 0x30, "DIFFICULTY", { "HARDEST", "MEDIUM", "HARD", "EASY" }, 1 },
	{ 0, 0x40, "BOMB", { "1 PER GAME", "3 PER GAME" }, 0 },
	{ 0, 0x80, "ATTRACT MUSIC", { "OFF", "ON" }, 0 },
	{ -1 }
};

static struct KEYSet keys[] =
{
	{ 2, 2, "P1 MOVE UP" },
	{ 2, 0, "P1 MOVE LEFT"  },
	{ 2, 1, "P1 MOVE RIGHT" },
	{ 2, 3, "P1 MOVE DOWN" },
	{ 2, 4, "P1 FIRE LEFT" },
	{ 2, 5, "P1 FIRE RIGHT" },
	{ 2, 6, "P1 FLASH BOMB" },
	{ 3, 2, "P2 MOVE UP" },
	{ 3, 0, "P2 MOVE LEFT"  },
	{ 3, 1, "P2 MOVE RIGHT" },
	{ 3, 3, "P2 MOVE DOWN" },
	{ 3, 4, "P2 FIRE LEFT" },
	{ 3, 5, "P2 FIRE RIGHT" },
	{ 3, 6, "P2 FLASH BOMB" },
	{ -1 }
};

/* Tutankhm's video is all bitmapped, no characters ... */
/* this is here to decode the chartable for MAME's use  */
static struct GfxLayout charlayout =
{
	8,8,    	/* 8*8 characters */
	48,		/* 43 characters */
	4,      	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* bits are consecutive */
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 }, /* x bit positions */
	{ 24, 28, 16, 20, 8, 12, 0, 4},                     /* y bit positions */
	32*8    /* every char takes 32 consecutive bytes */
};


/* Tutankhm's video is all bitmapped, no characters ... */
/* this is here to decode the chartable for MAME's use  */
/* and for the color table for the game bitmap		*/
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* Characters for MAME's use */
	{ 0, 0xf000, &charlayout, 0, 4 },
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			2500000,			/* 2.5 Mhz ??? */
			0,				/* memory region # 0 */
			readmem,			/* MemoryReadAddress */
			writemem,			/* MemoryWriteAddress */
			0,				/* IOReadPort */
			0,				/* IOWritePort */
			tutankhm_interrupt,			/* interrupt routine */
			1				/* interrupts per frame */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1000000,			/* 1.000 Mhz ??? */
			2,				/* memory region # 2 */
			sound_readmem,			/* MemoryReadAddress */
			sound_writemem,			/* MemoryWriteAddress */
			0,				/* IOReadPort */
			0,				/* IOWritePort */
			interrupt,        		/* interrupt routine */
			1			        /* interrupts per frame */
		}
	},
	60,						/* frames per second */
	tutankhm_init_machine,				/* init machine routine */

	/* video hardware */
	32*8, 32*8,			                /* screen_width, screen_height */
	{ 2, 8*32-3, 0*32, 8*32-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,					/* GfxDecodeInfo * */
	256,						/* total colors */
	256 + 16, 					/* color table length ( 256 for text, 16 for bitmap ) */
	tut_vh_convert_color_prom,			/* convert color prom routine */

	0,						/* vh_init routine */
	tut_vh_start,					/* vh_start routine */
	tut_vh_stop,					/* vh_stop routine */
	tut_vh_screenrefresh,				/* vh_update routine */

	/* sound hardware */
	0,						/* pointer to samples */
	0,						/* sh_init routine */
	timeplt_sh_start,				/* sh_start routine */
	AY8910_sh_stop,					/* sh_stop routine */
	AY8910_sh_update				/* sh_update routine */
};


ROM_START( tutankhm_rom )
	ROM_REGION( 0x10000 + 0x1000 * 12 )      /* 64k for M6809 CPU code + ( 4k * 12 ) for ROM banks */
	ROM_LOAD( "ra1_1h.cpu", 0xa000, 0x1000 ) /* program ROMs */
	ROM_LOAD( "ra1_2h.cpu", 0xb000, 0x1000 )
	ROM_LOAD( "ra1_3h.cpu", 0xc000, 0x1000 )
	ROM_LOAD( "ra1_4h.cpu", 0xd000, 0x1000 )
	ROM_LOAD( "ra1_5h.cpu", 0xe000, 0x1000 )
	ROM_LOAD( "ra1_6h.cpu", 0xf000, 0x1000 )
	ROM_LOAD( "ra1_1i.cpu", 0x10000, 0x1000 ) /* graphic ROMs (banked) -- only 9 of 12 are filled */
	ROM_LOAD( "ra1_2i.cpu", 0x11000, 0x1000 )
	ROM_LOAD( "ra1_3i.cpu", 0x12000, 0x1000 )
	ROM_LOAD( "ra1_4i.cpu", 0x13000, 0x1000 )
	ROM_LOAD( "ra1_5i.cpu", 0x14000, 0x1000 )
	ROM_LOAD( "ra1_6i.cpu", 0x15000, 0x1000 )
	ROM_LOAD( "ra1_7i.cpu", 0x16000, 0x1000 )
	ROM_LOAD( "ra1_8i.cpu", 0x17000, 0x1000 )
	ROM_LOAD( "ra1_9i.cpu", 0x18000, 0x1000 )

	ROM_REGION( 0x1000 ) /* ROM Region 1 -- discarded */
	ROM_LOAD( "ra1_6h.cpu", 0x0000, 0x1000 )

	ROM_REGION( 0x10000 ) /* 64k for Z80 sound CPU code */
	ROM_LOAD( "ra1_7a.snd", 0x0000, 0x1000 )
	ROM_LOAD( "ra1_8a.snd", 0x1000, 0x1000 )
ROM_END


static int hiload(const char *name)
{
   FILE *f;

   /* get RAM pointer (this game is multiCPU, we can't assume the global */
   /* RAM pointer is pointing to the right place) */
   unsigned char *RAM = Machine->memory_region[0];

   /* check if the hi score table has already been initialized */
   if (memcmp(&RAM[0x88d9],"\x01\x00",2) == 0)
   {
      if ((f = fopen(name,"rb")) != 0)
      {
         fread(&RAM[0x88a6],1,52,f);
         fclose(f);
      }

      return 1;
   }
   else
      return 0; /* we can't load the hi scores yet */
}

static void hisave(const char *name)
{
	FILE *f;
        /* get RAM pointer (this game is multiCPU, we can't assume the global */
        /* RAM pointer is pointing to the right place) */
        unsigned char *RAM = Machine->memory_region[0];

	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x88a6],1,52,f);
		fclose(f);
	}
}


struct GameDriver tutankhm_driver =
{
        "Tutankham",
	"tutankhm",
        "MIRKO BUFFONI\nDAVID DAHL",
	&machine_driver,

	tutankhm_rom,
	0, 0,   /* ROM decode and opcode decode functions */
	0,      /* Sample names */

	input_ports, trak_ports, dsw, keys,

	0, 0, 0,   /* colors, palette, colortable */
	128-(8*3), 128-4+(8*3),
	hiload, hisave		        /* High score load and save */
};
