/***************************************************************************

TODO: properly emulate the trackball.

  Coors Light Bowling from Capcom

  Memory Map:

  0000-3fff     3 Graphics ROMS mapped in using 0x4800
  4000          Scan line read or modified (Only 0-244 are displayed)
  4800          Graphics ROM select
  5000-57ff     Static RAM (Saves machine state after shut off)
                No DIP Switches
                Enter setup menu by holding down the F1 key on the
                high score screen
  5800-5842     ???? Might have something to do with video,
                See weird code starting at 0xe102, which is used for animating
                the pins, but pulsates bit 2 of location 0x582d.
                Dunno what it does.
  5b00-5bff     Graphics RAM for one scanline (0x4000)
                First 0x20 bytes provide a 16 color palette for this
                scan line. 2 bytes per color: 0000RRRR GGGGBBBB.

                The 4096 colors of the game is mapped into 256 colors, by
                taking the most significant 3 bits of the Red and Green
                components, and the most significant 2 bits of the blue
                component. This provides for a uniform palette, but I think
                the game uses a lot of colors where the Red component is
                either 0 or 0x0f. So there may be a way to improve the colors.

                Remaining 0xe0 bytes contain 2 pixels each for a total of
                448 pixels, but only 360 seem to be displayed.
                (Each scanline appears vertically on MAME)

  6000          Sound command
  6800
  7000          Input port 1    Bit 0-3 Trackball Vertical,
                                Bit 7   Left Coin
  7800          Input port 2    Bit 0-3 Trackball Horizontal
                                Bit 4   Hook Left
                                Bit 5   Hook Right
                                Bit 6   Start
                                Bit 7   Right Coin

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"



unsigned char *capbowl_firq_enable;

void capbowl_vh_screenrefresh(struct osd_bitmap *bitmap);

int  capbowl_vh_start(void);
void capbowl_vh_stop(void);

extern unsigned char *capbowl_scanline;

void capbowl_videoram_w(int offset,int data);
int  capbowl_videoram_r(int offset);

void capbowl_rom_select_w(int offset,int data);

int  capbowl_pagedrom_r(int offset);

int  capbowl_service_r(int offset);
void capbowl_service_w(int offset, int data);



void capbowl_sndcmd_w(int offset,int data)
{
	soundlatch_w(offset, data);

	cpu_cause_interrupt (1, M6809_INT_IRQ);
}


/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void firqhandler(void)
{
	cpu_cause_interrupt (1, M6809_INT_FIRQ);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1 },
	{ 0x5000, 0x57ff, MRA_RAM },
	{ 0x582d, 0x582d, MRA_RAM },
	{ 0x5836, 0x5836, MRA_NOP },	/* firq acknowledge? */
	{ 0x5b00, 0x5bff, capbowl_videoram_r },
	{ 0x7000, 0x7000, input_port_0_r },
	{ 0x7800, 0x7800, input_port_1_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x4000, MWA_RAM, &capbowl_scanline },
	{ 0x4800, 0x4800, capbowl_rom_select_w },
	{ 0x5000, 0x57ff, MWA_RAM },
	{ 0x582a, 0x582a, MWA_RAM },	/* ??? */
	{ 0x582d, 0x582d, MWA_RAM, &capbowl_firq_enable },	/* ??? */
	{ 0x5b00, 0x5bff, capbowl_videoram_w },
	{ 0x5c00, 0x5c00, MWA_NOP }, /* Off by 1 bug?? */
	{ 0x6000, 0x6000, capbowl_sndcmd_w },
	{ 0x6800, 0x6800, MWA_NOP },	/* trackball reset? */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, YM2203_status_port_0_r },
	{ 0x1001, 0x1001, YM2203_read_port_0_r },
	{ 0x7000, 0x7000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM},
	{ 0x1000, 0x1000, YM2203_control_port_0_w },
	{ 0x1001, 0x1001, YM2203_write_port_0_w },
	{ 0x2000, 0x2000, MWA_NOP },  /* ????? */
	{ 0x6000, 0x6000, DAC_data_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/***************************************************************************

  Coors Light Bowling uses NMI to trigger the self test. We use a fake input
  port to tie that event to a keypress.

***************************************************************************/
static int capbowl_interrupt(void)
{
	if (readinputport(2) & 1)	/* get status of the F2 key */
		return nmi_interrupt();	/* trigger self test */
	else if (*capbowl_firq_enable & 0x04)
		return M6809_INT_FIRQ;
	else return ignore_interrupt();
}

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_ANALOG ( 0x0f, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_CENTER, 100, 7, 0, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_ANALOG ( 0x0f, 0x00, IPT_TRACKBALL_X | IPF_CENTER, 100, 7, 0, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
INPUT_PORTS_END



static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	1500000,	/* 1.5 MHz ??? */
	{ YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ firqhandler }
};

static struct DACinterface dac_interface =
{
	1,
	441000,
	{ 64, 64 },
	{  1,  1 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz ??? */
			0,
			readmem,writemem,0,0,
			capbowl_interrupt,2	/* ?? */
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ??? */
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the sound hardware */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	/* The visible region is 245x360 */
	256, 360, { 0, 244, 0, 359 },
	0,
	256,0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	capbowl_vh_start,
	capbowl_vh_stop,
	capbowl_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( capbowl_rom )
	ROM_REGION(0x28000)   /* 160k for code and graphics */
	ROM_LOAD( "u6",  0x8000 , 0x8000, 0xb70297ae )
	ROM_LOAD( "gr0", 0x10000, 0x8000, 0xfb7d35bd )
	ROM_LOAD( "gr1", 0x18000, 0x8000, 0xe28dc4ef )
	ROM_LOAD( "gr2", 0x20000, 0x8000, 0x325fce25 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)   /* 64k for sound */
	ROM_LOAD( "sound", 0x8000, 0x8000, 0x1ec37619 )
ROM_END

ROM_START( clbowl_rom )
	ROM_REGION(0x28000)   /* 160k for code and graphics */
	ROM_LOAD( "u6",  0x8000 , 0x8000, 0x99fede6e )
	ROM_LOAD( "gr0", 0x10000, 0x8000, 0x64039867 )
	ROM_LOAD( "gr1", 0x18000, 0x8000, 0x3a758375 )
	ROM_LOAD( "gr2", 0x20000, 0x8000, 0xb63eb4f2 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)   /* 64k for sound */
	ROM_LOAD( "sound", 0x8000, 0x8000, 0xe27c494a )
ROM_END



static int hiload(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x5000],0x800);
		osd_fclose(f);
	}
	/* Invalidate the static RAM to force reset to factory settings */
	else memset(&RAM[0x5000],0xff,0x800);

	return 1;
}

static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x5000],0x0800);
		osd_fclose(f);
	}
}



struct GameDriver capbowl_driver =
{
	"Capcom Bowling",
	"capbowl",
	"Zsolt Vasvari\nMirko Buffoni\nNicola Salmoria",
	&machine_driver,

	capbowl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,

	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver clbowl_driver =
{
	"Coors Light Bowling",
	"clbowl",
	"Zsolt Vasvari\nMirko Buffoni\nNicola Salmoria",
	&machine_driver,

	clbowl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,

	ORIENTATION_DEFAULT,

	hiload, hisave
};
