/***************************************************************************
File: drivers\starwars.c

STARWARS HARDWARE FILE

This file is Copyright 1997, Steve Baines.
Modified by Frank Palazzolo for sound support

Current e-mail contact address:  sjb@ohm.york.ac.uk

Release 2.0 (6 August 1997)

***************************************************************************

Comments:

Release 2.0:

My first contribution to MAME as well.  Added Pokey Sound Support.
Seems to cut out some music prematurely, when there are other sounds going,
but otherwise ok.  Attract mode music may not work quite right either.
No speech yet.

(If anyone has a proper data sheet for the 6532A PIA chip, please
contact me.)  Star Wars writes to some registers which I don't have
docs on.        -Frank Palazzolo
                 palazzol@tir.com


Release 1.0:

This is the first release of my Star Wars driver for MAME. This is also my
first ever driver for MAME!

Known Bugs/Issues:

1) The vector generator I'm using sometimes draws lines in the wrong place
when in the trench sequence (you'll see what I mean).  I think this is due to
a wraparound effect somewhere in the vector code and should be fixed soon.

2) On the Death Star surface run the ship banks in the correct direction
in response to left/right movements, but the surface moves in the other
direction.  I'm a bit puzzled by this, maybe I've got buggy ROMS?

3) No mouse support yet

4) No sound support yet

5) DIP setting in MAME doesn't work, but the game has internal DIP switch
setting which does work, so this doesn't really matter.  Press and hold
the SELF TEST key, and accounting info is shown. Still holding that key, press
AUX COIN, and the settings menu appears.

6) NovRAM, Mathbox self tests fail.  This doesn't matter, they work
properley for the purposes of everything but the self-test.

7) PAUSE doesn't clear itself when un-paused

That's all I can think of at the moment

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/starwars.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"
#include "machine/swmathbx.h"
#include "machine/starwars.h"
#include "machine/atari_vg.h"

#define EMPIRE 0

/* Star Wars READ memory map */
static struct MemoryReadAddress readmem[] =
{
       { 0x0000, 0x2fff, MRA_RAM, &vectorram },   /* vector_ram */
       { 0x3000, 0x3fff, MRA_ROM },  /* vector_rom */
/*       { 0x4800, 0x4fff, MRA_RAM },   cpu_ram   */
/*        { 0x5000, 0x5fff, MRA_RAM },    (math_ram_r) math_ram */
/*        { 0x0000, 0x3fff, MRA_RAM, &vectorram}, *//* Vector RAM and ROM */
        { 0x4800, 0x5fff, MRA_RAM },  /* CPU and Math RAM */
        { 0x6000, 0x7fff, banked_rom_r }, /* banked ROM */
        { 0x8000, 0xffff, MRA_ROM },  /* rest of main_rom */
        { 0x4300, 0x431f, input_bank_0_r }, /* Memory mapped input port 0 */
        { 0x4320, 0x433f, input_bank_1_r }, /* Memory mapped input port 1 */
        { 0x4340, 0x435f, opt_0_r }, /* DIP switches bank 0 */
        { 0x4360, 0x437f, opt_1_r }, /* DIP switches bank 1 */
        { 0x4380, 0x439f, adc_r },   /* ADC read */
        { 0x4400, 0x4400, main_read_r },
        { 0x4401, 0x4401, main_ready_flag_r },
        { 0x4500, 0x45ff, MRA_RAM }, /* nov_ram */
        { 0x4700, 0x4700, reh },
        { 0x4701, 0x4701, rel },
        { 0x4703, 0x4703, prng }, /* pseudo random number generator */
	{ -1 }	/* end of table */
};

/* Star Wars Sound READ memory map */
static struct MemoryReadAddress readmem2[] =
{
        { 0x0800, 0x0fff, sin_r }, /* SIN Read */
        { 0x1000, 0x107f, MRA_RAM },  /* PIA RAM */

        { 0x1080, 0x1083, PIA_port_r },
        { 0x1084, 0x109f, PIA_timer_r },

        { 0x2000, 0x27ff, MRA_RAM }, /* program RAM */
        { 0x4000, 0x7fff, MRA_ROM }, /* sound roms */
        { 0xc000, 0xffff, MRA_ROM }, /* load last rom twice */
                                     /* for proper int vec operation */

        { -1 }  /* end of table */
};

/* Star Wars WRITE memory map */
static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_RAM, &vectorram }, /* vector_ram */
	{ 0x3000, 0x3fff, MWA_ROM },  /* vector_rom */
/*        { 0x4800, 0x4fff, MWA_RAM },   cpu_ram */
/*        { 0x5000, 0x5fff, MWA_RAM },  (math_ram_w) math_ram */
	{ 0x4800, 0x5fff, MWA_RAM }, /* CPU and Math RAM */
	{ 0x6000, 0xffff, MWA_ROM }, /* main_rom */
	{ 0x4400, 0x4400, main_wr_w },
	{ 0x4500, 0x45ff, MWA_RAM }, /* nov_ram */
	{ 0x4600, 0x461f, atari_vg_go },  /* evggo(mine) or vg2_go */
	{ 0x4620, 0x463f, vg_reset }, /* evgres(mine) or vg_reset */
	{ 0x4640, 0x465f, MWA_NOP }, /*  (wdclr) Watchdog clear */
	{ 0x4660, 0x467f, irqclr },  /* clear periodic interrupt */
/*        { 0x4680, 0x4680, MWA_NOP },  (coin_ctr2) Coin counter 1 */
/*        { 0x4681, 0x4681, MWA_NOP },  (coin_ctr1) Coin counter 2 */
	{ 0x4680, 0x4681, MWA_NOP },  /*  Coin counters */
	{ 0x4682, 0x4682, led3 }, /* led3 */
	{ 0x4683, 0x4683, led2 }, /* led2 */
	{ 0x4684, 0x4684, mpage },  /* Page select for ROM0 */
	{ 0x4685, 0x4685, prngclr }, /* Reset PRNG */
	{ 0x4686, 0x4686, led1 },    /* led1 */
	{ 0x4687, 0x4687, recall },
	{ 0x46a0, 0x46bf, nstore },
	{ 0x46c0, 0x46c0, adcstart0 },
	{ 0x46c1, 0x46c1, adcstart1 },
	{ 0x46c2, 0x46c2, adcstart2 },
	{ 0x46e0, 0x46e0, soundrst },
        { 0x4700, 0x4707, swmathbx },
	{ -1 }	/* end of table */
};

/* Star Wars sound WRITE memory map */
static struct MemoryWriteAddress writemem2[] =
{
        { 0x0000, 0x07ff, sout_w },
        { 0x1000, 0x107f, MWA_RAM }, /* PIA ram */

        { 0x1080, 0x1083, PIA_port_w },
        { 0x1084, 0x109f, PIA_timer_w },

        { 0x1800, 0x181f, starwars_pokey_sound_w },
        { 0x1820, 0x183f, starwars_pokey_ctl_w },

        { 0x2000, 0x27ff, MWA_RAM }, /* program RAM */
        { 0x4000, 0x7fff, MWA_ROM }, /* sound rom */
        { 0xc000, 0xffff, MWA_ROM }, /* sound rom again, for intvecs */


        { -1 }  /* end of table */
};

static struct InputPort input_ports[] =
{
        {       /* IN0  Memory mapped input at 0x4300 */
		0xdf, /* Mask out the bit marked as 'SPARE 1' */
                { OSD_KEY_4, OSD_KEY_3, OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, 0, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, OSD_JOY_FIRE, OSD_JOY_FIRE }
	},
        {       /* IN1  Memory mapped input at 0x4320 */
		0x34, /* Only 3 bits used in this port */
                { 0, 0, OSD_KEY_E, 0, OSD_KEY_ALT, OSD_KEY_CONTROL, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* OPT0  DIP switch bank 0  */
		0xe4,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* OPT1  DIP switch bank 1  */
                0xc0,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
	{
		X_AXIS,
	  	1,
	  	1,
		starwars_trakball_r
	},
	{
		Y_AXIS,
		1,
		1,
		starwars_trakball_r
	},
	{ -1 }
};

/* This is the text displayed in the settings screen when changing keys */

static struct KEYSet keys[] =
{
        { 0, 7, "LEFT F/S" },
        { 0, 6, "RIGHT F/S"  },
        { 0, 4, "SELF TEST" },
        { 0, 3, "SLAM" },
        { 0, 2, "COIN AUX" },
        { 0, 1, "COIN L" },
        { 0, 0, "COIN R" },
        { 1, 5, "LEFT THUMB" },
        { 1, 4, "RIGHT THUMB" },
        { 1, 2, "DIAGN" },
	{ -1 }
};

static struct DSW dsw[] =
{
        { 2, 0xc0, "SHIELDS", { "9", "8", "7", "6" } },
        { 2, 0x30, "DIFFICULTY", { "HARDEST", "HARD", "MODERATE", "EASY" } },
        { 2, 0x0c, "BONUS SHIELDS", { "3", "2", "1", "0" } },
        { 2, 0x02, "ATTRACT MUSIC", { "OFF", "ON" } },
        { 2, 0x01, "GAME MODE", { "NORMAL", "FREEZE" } },
        { 3, 0xc0, "CREDITS/COIN", { "1/2", "1", "2", "FREEPLAY"} },
	{ -1 }
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
        { 0, 0,      &fakelayout,     0, 256 },
        { -1 } /* end of array */
};

static unsigned char color_prom[] =
        {
          0x00,0x00,0x00,
          0x00,0x00,0x01,
          0x00,0x01,0x00,
          0x00,0x01,0x01,
          0x01,0x00,0x00,
          0x01,0x00,0x01,
          0x01,0x01,0x00,
          0x01,0x01,0x01,
          0x00,0x00,0x00,
          0x00,0x00,0x01,
          0x00,0x01,0x00,
          0x00,0x01,0x01,
          0x01,0x00,0x00,
          0x01,0x00,0x01,
          0x01,0x01,0x00,
          0x01,0x01,0x01      };

/*********************************/

/* Memory regions relate to the ROM loader memory definitions later */
/* in this file.  Note that main machine has region 0      */
/* and I've not put a sound board in yet                  */

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
                /* Main CPU */
		{
			CPU_M6809,
                        1500000,                        /* 1.5 Mhz CPU clock (Don't know what speed it should be) */
			0,  				/* Memory region #0 */
			readmem,writemem,0,0,
                        interrupt,3            /* Interrupt handler, and interrupts per frame (usually 1) */
                        /* Starwars should be 183Hz interrupts */
                        /* Increasing number of interrupts per frame speeds game up */
                },
                /* Sound CPU */
                {
                        CPU_M6809 | CPU_AUDIO_CPU,
                        1500000,                        /* 1.5 Mhz CPU clock (Don't know what speed it should be) */
      /*JB 970809 */    2,                              /* Memory region #2 */
                        readmem2,writemem2,0,0,
                        starwars_snd_interrupt,12       /* Interrupt handler, and interrupts per frame (usually 1) */
                        /* Interrupts are to attempt to get */
                        /* resolution for the PIA Timer */
                        /* Approx. 2048 PIA clocks (@1.5 Mhz) */
                }

	},
        60,                     /* Target Frames per Second */
        starwars_init_machine,  /* Name of initialisation handler */

	/* video hardware */
	288, 224, { 0, 240, 0, 280 },
	gfxdecodeinfo,
        256,256, /* Number of colours, length of colour lookup table */
	atari_vg_init_colors,

        0,  /* Handler to initialise video handware */
	atari_vg_avg_flip_start,	/* Start video hardware */
	atari_vg_stop,			/* Stop video hardware */
	atari_vg_screenrefresh,		/* Do a screen refresh */

	/* sound hardware */
	0,
        0,                              /* Initialise audio hardware */
        starwars_sh_start,              /* Start audio  */
        starwars_sh_stop,               /* Stop audio   */
        starwars_sh_update              /* Update audio */
};



/***************************************************************************

  Game driver

***************************************************************************/

ROM_START( starwars_rom )

/* ROM_REGION(x) allocates a chunk of memory of size x to be used as ROM */
/* ROM_LOAD("bibble",a,b) loads ROM file called 'bibble' starting at offset 'a' */
/* within this chunk, and of length 'b' */

/* This is the first ROM region defined, i.e. memory region 0 */
#if(EMPIRE==0)
        ROM_REGION(0x14000)     /* 64k for code, and another 16k on the end for banked ROMS */

        ROM_LOAD( "136021.105", 0x3000, 0x1000, 0x46f9a4f9 ) /* 3000-3fff is 4k vector rom */

        ROM_LOAD( "136021.102", 0x8000, 0x2000, 0x7ad298ba ) /*  8k ROM 1 bank */
        ROM_LOAD( "136021.203", 0xa000, 0x2000, 0x58c5d6a1 ) /*  8k ROM 2 bank */
        ROM_LOAD( "136021.104", 0xc000, 0x2000, 0x38d07312 ) /*  8k ROM 3 bank */
        ROM_LOAD( "136021.206", 0xe000, 0x2000, 0x199428a2 ) /*  8k ROM 4 bank */

        /* Load the Mathbox PROM's temporarily into the Vector RAM area */
        /* During initialisation they will be converted into useable form */
        /* and stored elsewhere. */
        ROM_LOAD( "136021.110",0x0000,0x0400, 0x3757bfd5 ) /* PROM 0 */
        ROM_LOAD( "136021.111",0x0400,0x0400, 0x465db5db ) /* PROM 1 */
        ROM_LOAD( "136021.112",0x0800,0x0400, 0xcb5ab0da ) /* PROM 2 */
        ROM_LOAD( "136021.113",0x0c00,0x0400, 0x4d20bcd2 ) /* PROM 3 */

/* Now load all 16k of ROM 0 starting at address 0x10000 */
/* Note that 64k is 0xffff, so this is outside of normal range. */

        ROM_LOAD( "136021.114", 0x10000, 0x4000, 0x8fddcf2b )   /* ROM 0 bank pages 0 and 1 */

		ROM_REGION(0x100) /* JB 970809 - MAME always throws away region 1 in vh_open() */

        /* Sound ROMS */
        ROM_REGION(0x10000)     /* Really only 32k, but it looks like 64K */

        ROM_LOAD("136021.107",0x4000,0x2000, 0xc0006994) /* Sound ROM 0 */
        ROM_RELOAD(           0xc000,0x2000) /* Copied again for */
        ROM_LOAD("136021.208",0x6000,0x2000, 0xa000bc38) /* Sound ROM 0 */
        ROM_RELOAD(           0xe000,0x2000) /* proper int vecs */
ROM_END  
#endif

/* ****************** EMPIRE *********************************** */
/* This doesn't work */
#if(EMPIRE==1)
        ROM_REGION(0x14000)     /* 64k for code, and another 16k on the end for banked ROMS */

        ROM_OBSOLETELOAD( "136031.111", 0x3000, 0x1000 )    /* 3000-3fff is 4k vector rom */

/* Expansion board location   ROM_LOAD( "136021.102", 0x8000, 0x2000 )   8k ROM 1 bank */
        ROM_OBSOLETELOAD( "136031.102", 0xa000, 0x2000 ) /*  8k ROM 2 bank */
        ROM_OBSOLETELOAD( "136021.103", 0xc000, 0x2000 ) /*  8k ROM 3 bank */
        ROM_OBSOLETELOAD( "136031.104", 0xe000, 0x2000 ) /*  8k ROM 4 bank */
        ROM_OBSOLETELOAD( "136031.101", 0x10000, 0x4000 )   /* Paged ROM 0 */
ROM_END
#endif


/* NovRAM Load/Save.  In-game DIP switch setting, and Hi-scores */

#if(EMPIRE==0)
static int novram_load(const char *name)
{
/* get RAM pointer (if game is multiCPU, we can't assume the global */
/* RAM pointer is pointing to the right place) */
unsigned char *RAM = Machine->memory_region[0];

FILE *f;
     if ((f = fopen(name,"rb")) != 0)
        {
        fread(&RAM[0x4500],1,256,f);
        fclose(f);
        }
     return 1;
}

static void novram_save(const char *name)
{
	FILE *f;
        /* get RAM pointer (if game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = fopen(name,"wb")) != 0)
	{
                fwrite(&RAM[0x4500],1,256,f);
		fclose(f);
	}
}
#endif
#if (EMPIRE==1)
static int novram_load(const char *name)
   {
   return 0;
   }
static void novram_save(const char *name)
   {
   }
#endif


struct GameDriver starwars_driver =
{
	"Star Wars",
	"starwars",
        "STEVE BAINES\nBRAD OLIVER\nFRANK PALAZZOLO",
	&machine_driver,

        starwars_rom,
        0, 0,  /* ROM decryption, Opcode decryption */
        0,     /* Sample Array (optional) */

        input_ports, trak_ports, dsw, keys,

        color_prom, /* Colour PROM */
        0,          /* palette */
        0,          /* colourtable */

        128, 128,  /* `Paused' x, y */
        novram_load, novram_save /* Highscore load, save */
};

