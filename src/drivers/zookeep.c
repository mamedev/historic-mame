/***************************************************************************

ZooKeeper Memory Map
--------- ------ ---

ZooKeeper uses the same hardware as Qix;  two 6809 CPUs:  one for data and
sound and the other for video.
Communication between the two CPUs is done using a 4K RAM space at $0000
which both CPUs have direct access.  FIRQs (fast interrupts) are generated
by each CPU to interrupt the other at specific times.

The coin door switches and player controls are connected to the CPUs by
Mototola 6821 PIAs.  These devices are memory mapped as shown below.

The screen is 256x256 with eight bit pixels (64K).  The screen is divided
into two halves each half mapped by the video CPU at $0000-$7FFF.  The
high order bit of the address latch at $9402 specifies which half of the
screen is being accessed.

Timing is critical in the hardware.  The data CPU must have an interrupt
signal generated externally at the right frequency to make the game play
correctly.

The address latch works as follows.  When the video CPU accesses $9400,
the screen address is computed by using the values at $9402 (high byte)
and $9403 (low byte) to get a value between $0000-$FFFF.  The value at
that location is either returned or written.

The scan line at $9800 on the video CPU records where the scan line is
on the display (0-255).  Several places in the ROM code wait until the
scan line reaches zero before continuing.

CPU #1 (Data/Sound):
    $0400 - $07FF:  Local Memory
    $0800        :  ACIA base address
    $0C00        :  Video FIRQ activation
    $0C01        :  Data FIRQ deactivation
    $1000        :  Sound PIA
    $1400        :  [76543210] Game PIA 1 (Port A)
                     o         Fast draw
                      o        1P button
                       o       2P button
                        o      Slow draw
                         o     Joystick Left
                          o    Joystick Down
                           o   Joystick Right
                            o  Joystick Up
    $1402        :  [76543210] Game PIA 1 (Port B)
                     o         Tilt
                      o        Coin sw      Unknown
                       o       Right coin
                        o      Left coin
                         o     Slew down
                          o    Slew up
                           o   Sub. test
                            o  Adv. test
    $1900        :  Game PIA 2
    $1C00        :  Game PIA 3

CPU #2 (Video):
    $0000 - $7FFF:  Video/Screen RAM
    $8400 - $87FF:  CMOS backup and local memory
    $8800        :  LED output and color RAM page select
    $8C00        :  Data FIRQ activation
    $8C01        :  Video FIRQ deactivation
    $9000        :  Color RAM
    $9400        :  Address latch screen location
    $9402        :  Address latch Hi-byte
    $9403        :  Address latch Lo-byte
    $9800        :  Scan line location
    $9C00        :  CRT controller base address

BOTH CPUS:
    $0000 - $03FF:  Dual port RAM accessible by both processors

NONVOLATILE CMOS MEMORY MAP (CPU #2 -- Video) $8400-$87ff
	$86A9 - $86AA:	When CMOS is valid, these bytes are $55AA
	$86AC - $86C3:	AUDIT TOTALS -- 4 4-bit BCD digits per setting
					(All totals default to: 0000)
					$86AC: TOTAL PAID CREDITS
					$86AE: LEFT COINS
					$86B0: CENTER COINS
					$86B2: RIGHT COINS
					$86B4: PAID CREDITS
					$86B6: AWARDED CREDITS
					$86B8: % FREE PLAYS
					$86BA: MINUTES PLAYED
					$86BC: MINUTES AWARDED
					$86BE: % FREE TIME
					$86C0: AVG. GAME [SEC]
					$86C2: HIGH SCORES
	$86C4 - $86FF:	High scores -- 10 scores/names, consecutive in memory
					Six 4-bit BCD digits followed by 3 ascii bytes
                                        (Default: 030000 zookeeper)
	$8700		 :	LANGUAGE SELECT (Default: $32)
					ENGLISH = $32  FRANCAIS = $33  ESPANOL = $34  DEUTSCH = $35
	$87D9 - $87DF:	COIN SLOT PROGRAMMING -- 2 4-bit BCD digits per setting
					$87D9: STANDARD COINAGE SETTING  (Default: 01)
					$87DA: COIN MULTIPLIERS LEFT (Default: 01)
					$87DB: COIN MULTIPLIERS CENTER (Default: 04)
					$87DC: COIN MULTIPLIERS RIGHT (Default: 01)
					$87DD: COIN UNITS FOR CREDIT (Default: 01)
					$87DE: COIN UNITS FOR BONUS (Default: 00)
					$87DF: MINIMUM COINS (Default: 00)
	$87E0 - $87EA:	LOCATION PROGRAMMING -- 2 4-bit BCD digits per setting
					$87E0: BACKUP HSTD [0000] (Default: 03)
					$87E1: MAXIMUM CREDITS (Default: 10)
					$87E2: NUMBER OF TURNS (Default: 03)
					$87E3: THRESHOLD (Default: 75)
					$87E4: TIME LINE (Default: 37)
					$87E5: DIFFICULTY 1 (Default: 01)
					$87E6: DIFFICULTY 2 (Default: 01)
					$87E7: DIFFICULTY 3 (Default: 01)
					$87E8: DIFFICULTY 4 (Default: 01)
					$87E9: ATTRACT SOUND (Default: 01)
					$87EA: TABLE MODE (Default: 00)

***************************************************************************/

#include "driver.h"

extern unsigned char *zoo_sharedram;
extern int qix_addresslatch_r(int offset);
extern void qix_addresslatch_w(int offset, int data);
void qix_paletteram_w(int offset,int data);
void qix_palettebank_w(int offset,int data);
extern void qix_colorram_w(int offset, int data);
extern void qix_data_firq_w(int offset, int data);
extern int qix_videoram_r(int offset);
extern void qix_videoram_w(int offset, int data);
extern void qix_video_firq_w(int offset, int data);

void qix_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int zoo_sharedram_r(int offset);

extern unsigned char *qix_paletteram,*qix_palettebank;
extern unsigned char *qix_videoaddress;

extern void zoo_sharedram_w(int offset, int data);
extern int qix_data_interrupt(void);
extern void qix_vh_screenrefresh(struct osd_bitmap *bitmap); /* JB 970524 */
extern int qix_vh_start(void);
extern void qix_vh_stop(void);

extern void zookeeper_bankswitch_w(int offset, int data);
extern int zookeeper_scanline_r(int offset);
extern void zookeeper_init_machine(void);



static struct MemoryReadAddress readmem_cpu_data[] =
{
	{ 0x0000, 0x03ff, zoo_sharedram_r, &zoo_sharedram },
	{ 0x0400, 0x07ff, MRA_RAM },
	{ 0x1400, 0x1400, input_port_0_r }, /* PIA 1 PORT A -- Player controls */
	{ 0x1402, 0x1402, input_port_1_r }, /* PIA 1 PORT B -- Coin door switches */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 } /* end of table */
};

static struct MemoryReadAddress readmem_cpu_video[] =
{
	{ 0x0000, 0x7fff, qix_videoram_r },
	{ 0x8000, 0x83ff, zoo_sharedram_r },
	{ 0x9400, 0x9400, qix_addresslatch_r },
	{ 0x9800, 0x9800, zookeeper_scanline_r },
	{ 0xa000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem_cpu_data[] =
{
	{ 0x0000, 0x03ff, zoo_sharedram_w },
	{ 0x0400, 0x07ff, MWA_RAM },
	{ 0x0c00, 0x0c00, qix_video_firq_w },
	{ 0x1000, 0x1003, MWA_RAM },
	{ 0x1400, 0x1403, MWA_RAM },
	{ 0x1900, 0x1903, MWA_RAM },
	{ 0x1c00, 0x1c03, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem_cpu_video[] =
{
	{ 0x0000, 0x7fff, qix_videoram_w },
	{ 0x8000, 0x83ff, zoo_sharedram_w },
	{ 0x8400, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, qix_palettebank_w, &qix_palettebank },
	{ 0x8801, 0x8801, zookeeper_bankswitch_w },
	{ 0x8c00, 0x8c00, qix_data_firq_w },
	{ 0x9000, 0x93ff, MWA_RAM },
	{ 0x9000, 0x93ff, qix_paletteram_w, &qix_paletteram },
	{ 0x9400, 0x9400, qix_addresslatch_w },
	{ 0x9402, 0x9403, MWA_RAM, &qix_videoaddress },
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 } /* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* PORT 0 -- PIA 1 Port A -- Controls [01234567] */
		0xff,	/* default_value */

		/* keyboard controls */
		{ OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_LEFT,
		  OSD_KEY_ALT /* slow draw */, OSD_KEY_2 /* 2P start */,
		  OSD_KEY_1 /* 1P start */,  OSD_KEY_LCONTROL /* fast draw */ },

		/* joystick controls */
		{ OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_LEFT,
		  OSD_JOY_FIRE2 /* slow draw */, 0, 0, OSD_JOY_FIRE1 /* fast draw */ } /* EBM 970517 */
	},
	{	/* PORT 1 -- PIA 1 Port B -- Coin door switches */
		0xff,	/* default_value */

		/* keyboard controls */
		{ OSD_KEY_F1 /* adv. test */, OSD_KEY_F2 /* sub. test */,/* EBM 970519 */
		  OSD_KEY_F5 /* slew up */, OSD_KEY_F6 /* slew down */,  /* EBM 970519 */
		  OSD_KEY_3 /* left coin */, OSD_KEY_4 /* right coin */, /* EBM 970517 */
		  0 /* coin sw */,  OSD_KEY_F9 /* tilt */ },

		/* joystick controls (not used)  */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

/* These are only here to allow user to change the key settings */
/* Note:  Also need keys for the coin door switches. */
static struct KEYSet keys[] =
{
	/* port, bit num., setting name */
        { 0, 0, "MOVE UP" },
        { 0, 1, "MOVE RIGHT" },
        { 0, 2, "MOVE DOWN" },
        { 0, 3, "MOVE LEFT" },
        { 0, 4, "SLOW DRAW" },
        { 0, 7, "FAST DRAW" },
        { -1 }
};


/* ZooKeeper has no DIP switches */
static struct DSW zookeeper_dsw[] =
{
	/* Fake a switch to configure CMOS. Pressing adv. test (F1) causes
	   machine to reset and show config screens, so we flip the adv. test
	   bit here to force it. */ /* JB 970525 */
	{ 1, 0x01, "CONFIGURE CMOS", { "YES", "NO" } },
	{ -1 }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,		/* 1.25 Mhz */
			0,			/* memory region */
			readmem_cpu_data,	/* MemoryReadAddress */
			writemem_cpu_data,	/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			interrupt,	/* interrupt routine */
			1			/* interrupts per frame */
		},
		{
			CPU_M6809,
			1250000,		/* 1.25 Mhz */
			2,			/* memory region #2 */
			readmem_cpu_video, writemem_cpu_video, 0, 0,
			ignore_interrupt,
            1
		}
	},
	60,							/* frames per second */
	100,								/* 100 cpu slices per frame */ /* JB 971011 */
	zookeeper_init_machine,		/* init machine routine */ /* JB 970526 */

	/* video hardware */
	256, 256,					/* screen_width, screen_height */
	{ 0, 256-1, 0, 256-1 }, 	/* struct rectangle visible_area */
	0,							/* GfxDecodeInfo * */
	256,						/* total colors */
	0,							/* color table length */
	qix_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,							/* vh_init routine */
	qix_vh_start,				/* vh_start routine */ /* JB 970524 */
	qix_vh_stop,				/* vh_stop routine */ /* JB 970524 */
	qix_vh_screenrefresh,		        /* vh_update routine */	/* JB 970524 */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	0					/* sh_update routine */
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zookeeper_rom )
	ROM_REGION(0x10000)	/* 64k for code for the first CPU (Data) */
	ROM_LOAD( "ZB12", 0x8000, 0x1000, 0x04506034 )
	ROM_LOAD( "ZA13", 0x9000, 0x1000, 0x91f9297d )
	ROM_LOAD( "ZA14", 0xA000, 0x1000, 0xa9a036e4 )
	ROM_LOAD( "ZA15", 0xB000, 0x1000, 0x56a14af7 )
	ROM_LOAD( "ZA16", 0xC000, 0x1000, 0x01f7597d )
	ROM_LOAD( "ZA17", 0xD000, 0x1000, 0x3dd0c4e0 )
	ROM_LOAD( "ZA18", 0xE000, 0x1000, 0xdc96af3a )
	ROM_LOAD( "ZA19", 0xF000, 0x1000, 0xfd5cd200 )

	/* This is temporary space not really used but necessary because MAME
	 * always throws away memory region 1.
	 */
	ROM_REGION(0x1000)
	ROM_OBSOLETELOAD( "ZA10", 0x0000, 0x1000 )

	ROM_REGION(0x12000)     /* 64k for code + 2 ROM banks for the second CPU (Video) */
	ROM_LOAD(  "ZA3", 0x0A000, 0x1000, 0x8f54bbe8 )
	ROM_LOAD(  "ZA5", 0x10000, 0x1000, 0x05e61772 )
	ROM_LOAD(  "ZA4", 0x0B000, 0x1000, 0x8979a0b3 )
	ROM_LOAD(  "ZA6", 0x11000, 0x1000, 0x3b0092ac )

	ROM_LOAD(  "ZA7", 0x0C000, 0x1000, 0xe01d57bd )
	ROM_LOAD(  "ZA8", 0x0D000, 0x1000, 0x62a73b67 )
	ROM_LOAD(  "ZA9", 0x0E000, 0x1000, 0x7feb3005 )
	ROM_LOAD( "ZA10", 0x0F000, 0x1000, 0x0729e957 )
ROM_END


/* Loads high scores and all other CMOS settings */
static int hiload(void)
{
	/* get RAM pointer (data is in second CPU's memory region) */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x8400],0x400);
		osd_fclose(f);
	}

	return 1;
}



static void hisave(void)
{
	/* get RAM pointer (data is in second CPU's memory region) */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8400],0x400);
		osd_fclose(f);
	}
}


struct GameDriver zookeep_driver =
{
	"Zookeeper",
	"zookeep",
	"John Butler\nEd. Mueller\nAaron Giles",
	&machine_driver,

    zookeeper_rom,
	0, 0,   /* ROM decode and opcode decode functions */
	0,      /* Sample names */

    input_ports, 0, trak_ports, zookeeper_dsw, keys,

    0, 0, 0,   			/* colors, palette, colortable */
	ORIENTATION_DEFAULT,

	0,0/*hiload, hisave*/	       /* High score load and save */ /* JB 970525 */
};


