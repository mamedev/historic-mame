/***************************************************************************

Colony 7 is *extremely* similar to Defender in hardware.  See williams.c.

------- Colony7 -------------
0000-9800 Video RAM
C000-CFFF ROM (4 banks) + I/O
d000-ffff ROM

c000-c00f color_registers  (16 bytes of BBGGGRRR)

C3FC      WatchDog

C400-C4FF CMOS ram battery backed up

C800      6 bits of the video counters bits 2-7

cc00 pia1_dataa (widget = I/O board)
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6
  bit 7
cc01 pia1_ctrla

cc02 pia1_datab
  bit 0 \
  bit 1 |
  bit 2 |-6 bits to sound board
  bit 3 |
  bit 4 |
  bit 5 /
  bit 6 \
  bit 7 /Plus CA2 and CB2 = 4 bits to drive the LED 7 segment
cc03 pia1_ctrlb (CB2 select between player 1 and player 2 controls if Table)

cc04 pia2_dataa
  bit 0  Fire
  bit 1  Thrust
  bit 2  Smart Bomb
  bit 3  HyperSpace
  bit 4  2 Players
  bit 5  1 Player
  bit 6  Reverse
  bit 7  Down
cc05 pia2_ctrla

cc06 pia2_datab
  bit 0  Up
  bit 1
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7
cc07 pia2_ctrlb
  Control the IRQ

d000 Select bank (c000-cfff)
  0 = I/O
  1 = BANK 1
  2 = BANK 2
  3 = BANK 3
  7 = BANK 4


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"


/**** from machine/williams.h ****/
extern unsigned char *williams_port_select;
extern unsigned char *williams_video_counter;
extern unsigned char *williams_bank_select;
void williams_init_machine(void);
int williams_interrupt(void);
int williams_input_port_0_1(int offset);
int williams_input_port_2_3(int offset);

extern unsigned char *defender_mirror;
extern unsigned char *defender_irq_enable;
extern unsigned char *defender_catch;
extern unsigned char *defender_bank_base;
int defender_interrupt(void);
int defender_bank_r(int offset);
void defender_bank_w(int offset,int data);
void defender_mirror_w(int offset,int data);
int defender_catch_loop_r(int offset); /* JB 970823 */

void colony7_bank_select_w(int offset,int data);


/**** from sndhrdw/williams.h ****/
extern unsigned char *williams_dac;
void williams_sh_w(int offset,int data);
void williams_digital_out_w(int offset,int data);
int williams_sh_start(void);
void williams_sh_stop(void);
void williams_sh_update(void);


/**** from vidhrdw/williams.h ****/
extern unsigned char *williams_paletteram;
extern unsigned char *williams_blitterram;
extern int williams_paletteram_size;
void williams_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int williams_vh_start(void);
void williams_vh_stop(void);
void williams_vh_screenrefresh(struct osd_bitmap *bitmap);
int williams_videoram_r(int offset);
void williams_palette_w(int offset,int data);
void williams_videoram_w(int offset,int data);
void williams_blitter_w(int offset,int data);

void defender_videoram_w(int offset,int data);


/*
 *   Read mem for Colony7
 */

static struct MemoryReadAddress colony7_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },		/* i/o / rom */
	{ 0xcc07, 0xcc07, MRA_BANK1, &defender_irq_enable },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Colony7
 */

static struct MemoryWriteAddress colony7_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &williams_paletteram, &williams_paletteram_size },	/* here only to initialize the pointers */
	{ 0xd000, 0xd000, colony7_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};




/* memory handlers for the audio CPU */
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x8400, 0x8400, MRA_RAM },
	{ 0x8402, 0x8402, soundlatch_r },
	{ 0xf800, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x8400, 0x8400, williams_digital_out_w, &williams_dac },
	{ 0xf800, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};




/*
 *   Colony7 buttons
 */

INPUT_PORTS_START( colony7_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the background color table. */
static struct GfxLayout fakelayout =
{
	1,1,
	0,
	4,	/* 4 bits per pixel */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0, &fakelayout, 0, 1 },
	{ -1 } /* end of array */
};



static struct MachineDriver colony7_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1200000,                /* ? Mhz */ /*Defender do not like 1 mhz. Collect at least 9 humans, when you depose them, the game stuck */
			0,                      /* memory region */
			colony7_readmem,       /* MemoryReadAddress */
			colony7_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			defender_interrupt,     /* interrupt routine */
			2                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750,   /* 0.89475 Mhz (3.579 / 4) */
			2,        /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */

		}
	},
	60,                                     /* frames per second */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};


/***************************************************************************

  High score/CMOS save/load

***************************************************************************/


static int colony7_cmos_load(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}

	return 1;
}

static void colony7_cmos_save(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}
}


/***************************************************************************

  Game driver(s)

***************************************************************************/



ROM_START( colony7_rom )
	ROM_REGION(0x14000)
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "cs06.bin", 0x10000, 0x0800, 0xb4d158c1 )
	ROM_LOAD( "cs04.bin", 0x10800, 0x0800, 0x69594a21 )
	ROM_LOAD( "cs07.bin", 0x11000, 0x0800, 0xc168b5da )
	ROM_LOAD( "cs05.bin", 0x11800, 0x0800, 0x49741fe0 )
	ROM_LOAD( "cs08.bin", 0x12000, 0x0800, 0x7838bc72 )
	ROM_LOAD( "cs08.bin", 0x12800, 0x0800, 0x7838bc72 )
	ROM_LOAD( "cs03.bin", 0xd000,  0x1000, 0xd3fcbf64 )
	ROM_LOAD( "cs02.bin", 0xe000,  0x1000, 0x741ecfe4 )
	ROM_LOAD( "cs01.bin", 0xf000,  0x1000, 0xdfb0181c )

	ROM_REGION(0x1000)
	ROM_LOAD( "cs11.bin", 0x0000, 0x0800, 0x54ba49bc )

	ROM_REGION(0x10000)
	ROM_LOAD( "cs11.bin", 0xf800, 0x0800, 0x54ba49bc ) /* Sound ROM */
ROM_END

ROM_START( colony7a_rom )
	ROM_REGION(0x14000)
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "cs06.bin", 0x10000, 0x0800, 0xb4d158c1 )
	ROM_LOAD( "cs04.bin", 0x10800, 0x0800, 0x69594a21 )
	ROM_LOAD( "cs07.bin", 0x11000, 0x0800, 0xc168b5da )
	ROM_LOAD( "cs05.bin", 0x11800, 0x0800, 0x49741fe0 )
	ROM_LOAD( "cs08.bin", 0x12000, 0x0800, 0x7838bc72 )
	ROM_LOAD( "cs08.bin", 0x12800, 0x0800, 0x7838bc72 )
	ROM_LOAD( "cs03a.bin", 0xd000,  0x1000, 0x124fcfb1 )
	ROM_LOAD( "cs02a.bin", 0xe000,  0x1000, 0x9acb45f3 )
	ROM_LOAD( "cs01a.bin", 0xf000,  0x1000, 0xe3508de4 )

	ROM_REGION(0x1000)
	ROM_LOAD( "cs11.bin", 0x0000, 0x0800, 0x54ba49bc )

	ROM_REGION(0x10000)
	ROM_LOAD( "cs11.bin", 0xf800, 0x0800, 0x54ba49bc ) /* Sound ROM */
ROM_END

struct GameDriver colony7_driver =
{
	"Colony 7",
	"colony7",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour",
	&colony7_machine_driver,       /* MachineDriver * */

	colony7_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,	/* sound_prom */

	0/*TBR*/,colony7_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	colony7_cmos_load, colony7_cmos_save
};

struct GameDriver colony7a_driver =
{
	"Colony 7 (Alternate)",
	"colony7a",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour",
	&colony7_machine_driver,       /* MachineDriver * */

	colony7a_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,	/* sound_prom */

	0/*TBR*/,colony7_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	colony7_cmos_load, colony7_cmos_save
};

