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
Space Intruder (spaceint)

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
void spaceint_videoram_w (int offset,int data);

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
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Extra Live" )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Coins" )
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
	9,       /* 9 channels */
	25	/* volume */
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
	ROM_LOAD( "ai_cpu_1.rom", 0x0000, 0x0400, 0x20e3ec41 )
	ROM_LOAD( "ai_cpu_2.rom", 0x0400, 0x0400, 0xe8f1ab55 )
	ROM_LOAD( "ai_cpu_3.rom", 0x0800, 0x0400, 0xa0092553 )
	ROM_LOAD( "ai_cpu_4.rom", 0x0c00, 0x0400, 0xbe14185c )
	ROM_LOAD( "ai_cpu_5.rom", 0x1000, 0x0400, 0xfee681ec )
	ROM_LOAD( "ai_cpu_6.rom", 0x1400, 0x0400, 0xeb338863 )
	ROM_LOAD( "ai_cpu_7.rom", 0x1800, 0x0400, 0x16dcfea4 )
ROM_END

ROM_START( kamikaze_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "km01",         0x0000, 0x0800, 0x8aae7414 )
	ROM_LOAD( "km02",         0x0800, 0x0800, 0x6c7a2beb )
	ROM_LOAD( "km03",         0x1000, 0x0800, 0x3e8dedb6 )
	ROM_LOAD( "km04",         0x1800, 0x0800, 0x494e1f6d )
ROM_END


static const char *astinvad_sample_names[] =
{
	"*invaders",
	"0.wav",
	"1.wav",
	"2.wav",
	"3.wav",
	"4.wav",
	"5.wav",
	"6.wav",
	"7.wav",
	"8.wav",
	0       /* end of array */
};


/*****************************************************************************/
/* Highscore save and load HSC 11/04/98										 */


static int astinvad_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

/*		RAM[0x1fc9]=1;
		RAM[0x1fca]=1;
		RAM[0x1fcb]=1;
*/




	if (memcmp(&RAM[0x1cff],"\x08\x20\x03",3) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
            osd_fread(f,&RAM[0x1fc9],19);
            osd_fclose(f);
        }

        return 1;
    }

    else
    return 0;  /* we can't load the hi scores yet */
}

static void astinvad_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
		osd_fwrite(f,&RAM[0x1fc9],19);
        //osd_fwrite(f,&RAM[0x1c00],2047);
		//{ 0x4000, 0x4fff, MWA_RAM, },
		//osd_fwrite(f,&RAM[0x4000],4095);
        osd_fclose(f);
    }
}

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
	0,

	astinvad_rom,
	0, 0,
	astinvad_sample_names,
	0,      /* sound_prom */

	astinvad_input_ports,

	0, astinvad_palette, 0,
	ORIENTATION_ROTATE_270,

	astinvad_hiload,astinvad_hisave
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
	0,

	kamikaze_rom,
	0, 0,
	astinvad_sample_names,
	0,      /* sound_prom */

	astinvad_input_ports,

	0, astinvad_palette, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

/*------------------------------------------------------------------------------
 Shoei Space Intruder
 Added By Lee Taylor (lee@defender.demon.co.uk)
 December 1998
------------------------------------------------------------------------------*/


static int spaceint_interrupt(void)
{
	if (readinputport(2) & 1)	/* Coin */
		return nmi_interrupt();
	else return interrupt();
}


/* LT 23-12-1998 */
static struct MemoryWriteAddress spaceint_writemem[] = /* L.T */ /* Whole function */
{
	{ 0x2000, 0x23ff, MWA_RAM },
        { 0x4100, 0x5fff, spaceint_videoram_w, &astinvad_videoram },
	{ 0x4000, 0x40ff, MWA_RAM, },
	{ 0x0000, 0x1bff, MWA_ROM },
	{ -1 }  /* end of table */
};


/* LT 23-12-1998 */
static struct MemoryReadAddress spaceint_readmem[] =
{
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x4100, 0x5fff, MRA_RAM },
	{ 0x0000, 0x1bff, MRA_ROM },
	{ -1 }  /* end of table */
};


/* LT 23-12-1998 */
static struct IOReadPort spaceint_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ -1 }  /* end of table */
};


/* LT 23-12-1998 */
static struct IOWritePort spaceint_writeport[] =
{
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( spaceint_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY  )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Extra Live" )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Coins" )
	PORT_DIPSETTING(    0x08, "1 Coin  Per 2 Plays" )
	PORT_DIPSETTING(    0x00, "1 Coin  Per Play" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Left */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Left */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Right */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Right */
	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	/* The cabinet selector is not memory mapped, but just disables the */
	/* screen flip logic */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
INPUT_PORTS_END


static struct MachineDriver spaceint_machine_driver = /* 20-12-1998 LT */
{
	/* basic machine hardware */
	{
		{
                        CPU_Z80,
			2000000,        /* 2 Mhz? */
			0,
			spaceint_readmem,spaceint_writemem,spaceint_readport,spaceint_writeport,
			spaceint_interrupt,1    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 30*8-1 },
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


ROM_START( spaceint_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "1", 0x0000, 0x0400, 0x184314d2 )
	ROM_LOAD( "2", 0x0400, 0x0400, 0x55459aa1 )
	ROM_LOAD( "3", 0x0800, 0x0400, 0x9d6819be )
	ROM_LOAD( "4", 0x0c00, 0x0400, 0x432052d4 )
	ROM_LOAD( "5", 0x1000, 0x0400, 0xc6cfa650 )
	ROM_LOAD( "6", 0x1400, 0x0400, 0xc7ccf40f )
	ROM_REGION(0x0100)     /* dummy space for graphics*/
	ROM_REGION(0x0100)     /* color prom*/
	ROM_LOAD( "clr", 0x0000, 0x0100, 0x13c1803f )
ROM_END


static int spaceint_hiload(void)
{
        return 1;
}

static void spaceint_hisave(void)
{

}


/* LT 21-12-1998 */
struct GameDriver spaceint_driver =
{
	__FILE__,
	0,
	"spaceint",
	"Space Intruder",
	"1980",
	"Shoei",
	"Lee Taylor\n",
	GAME_WRONG_COLORS,
	&spaceint_machine_driver,
	0,

	spaceint_rom,
	0, 0,
	astinvad_sample_names,
	0,      /* sound_prom */

	spaceint_input_ports,

	0, astinvad_palette, 0,
	ORIENTATION_DEFAULT,
	spaceint_hiload,spaceint_hisave
};


