/****************************************************************************

Blockade/Comotion Memory MAP
Frank Palazzolo (palazzol@tir.com)

CPU - Intel 8080A

Memory Address              (Upper/Lower)

0xxx 00aa aaaa aaaa     ROM     U2/U3    R       1K for Blockade/Comotion
0xxx 01aa aaaa aaaa     ROM     U4/U5    R       1K for Comotion Only
xxx1 xxxa aaaa aaaa     RAM              R/W     256 bytes
1xx0 xxaa aaaa aaaa    VRAM                      1K playfield

                    SPRITE ROM  U29/U43			 256 bytes for Blockade/Comotion

Ports    In            Out
1        Controls      bit 7 = Coin Latch Reset
                       bit 5 = Pin 19?
2        Controls      Sound Pitch?
4        Controls      Boom On
8        N/A           Boom Off


Note:   Blockade/Comotion support is complete with the exception of
		the square wave "music".

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* #define BLOCKADE_LOG 1 */

/* in vidhrdw */
void blockade_vh_screenrefresh(struct osd_bitmap *bitmap);
void comotion_vh_screenrefresh(struct osd_bitmap *bitmap);

void blockade_coin_latch_w(int offset, int data);
void blockade_sound_freq_w(int offset, int data);
void blockade_env_on_w(int offset, int data);
void blockade_env_off_w(int offset, int data);

void blockade_rom_init(void)
{
	int i;

	/* Merge nibble-wide roms together,
	   and load them into 0x0000-0x0400 */

	for(i=0;i<0x0400;i++)
	{
		ROM[0x0000+i] = (ROM[0x1000+i]<<4)+ROM[0x1400+i];
	}
}

void comotion_rom_init(void)
{
	int i;

	/* Merge nibble-wide roms together,
	   and load them into 0x0000-0x0800 */

	for(i=0;i<0x0400;i++)
	{
		ROM[0x0000+i] = (ROM[0x1000+i]<<4)+ROM[0x1400+i];
		ROM[0x0400+i] = (ROM[0x1800+i]<<4)+ROM[0x1c00+i];
	}

	/* They also need to show up here for comotion */

	for(i=0;i<2048;i++)
	{
		ROM[0x4000+i] = ROM[0x0000+i];
	}
}

/*************************************************************/
/*                                                           */
/* Inserting a coin should work like this:                   */
/*	1) Reset the CPU                                         */
/*	2) CPU Sets the coin latch                               */
/*	3) Finally the CPU coin latch is Cleared by the hardware */
/*	   (by the falling coin..?)                              */
/*	                                                         */
/*	I am faking this by keeping the CPU from Setting         */
/*	the coin latch if we have just been reset.               */
/*                                                           */
/*************************************************************/

static int coin_latch = 1;  /* Active Low */
static int just_been_reset = 0;

/* Need to check for a coin on the interrupt, */
/* This will reset the cpu                    */

int blockade_interrupt(void)
{
	if ((input_port_0_r(0) & 0x80) == 0)
	{
		just_been_reset = 1;
		cpu_reset(0);
	}

	return ignore_interrupt();
}

int blockade_input_port_0_r(int offset)
{
	/* coin latch is bit 7 */

	int temp = (input_port_0_r(0)&0x7f);
	return (coin_latch<<7) | (temp);
}

void blockade_coin_latch_w(int offset, int data)
{
	if (data & 0x80)
	{
	#ifdef BLOCKADE_LOG
		printf("Reset Coin Latch\n");
	#endif
		if (just_been_reset)
		{
			just_been_reset = 0;
			coin_latch = 0;
		}
		else
			coin_latch = 1;
	}

	if (data & 0x20)
	{
	#ifdef BLOCKADE_LOG
		printf("Pin 19 High\n");
	#endif
	}
	else
	{
	#ifdef BLOCKADE_LOG
		printf("Pin 19 Low\n");
	#endif
	}

	return;
}

void blockade_sound_freq_w(int offset, int data)
{
#ifdef BLOCKADE_LOG
	printf("Sound Freq: %d\n",data);
#endif
	return;
}

void blockade_env_on_w(int offset, int data)
{
#ifdef BLOCKADE_LOG
	printf("Boom Start\n");
#endif
	sample_start(0,0,0);
	return;
}

void blockade_env_off_w(int offset, int data)
{
#ifdef BLOCKADE_LOG
	printf("Boom End\n");
#endif
	return;
}

static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x07ff, MRA_ROM },
    { 0x4000, 0x47ff, MRA_ROM },  /* same image */
    { 0xe000, 0xe3ff, videoram_r, &videoram, &videoram_size },
    { 0xff00, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x07ff, MWA_ROM },
    { 0x4000, 0x47ff, MWA_ROM },  /* same image */
    { 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
    { 0xff00, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x01, 0x01, blockade_input_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x04, 0x04, input_port_2_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, blockade_coin_latch_w },
	{ 0x02, 0x02, blockade_sound_freq_w },
	{ 0x04, 0x04, blockade_env_on_w },
	{ 0x08, 0x08, blockade_env_off_w },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( blockade_input_ports )
    PORT_START  /* IN0 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
              "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
		                        /* this is really used for the coin latch,  */
								/* see blockade_interrupt()                 */
	PORT_DIPNAME ( 0x70, 0x70, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x70, "6 Crashes" )
	PORT_DIPSETTING ( 0x30, "5 Crashes" )
	PORT_DIPSETTING ( 0x50, "4 Crashes" )
	PORT_DIPSETTING ( 0x60, "3 Crashes" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME ( 0x04, 0x04, "Boom Switch", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x00, "Off" )
	PORT_DIPSETTING ( 0x04, "On" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START  /* IN1 */
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )

	PORT_START  /* IN2 */
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( comotion_input_ports )
    PORT_START  /* IN0 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
              "Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
		                        /* this is really used for the coin latch,  */
								/* see blockade_interrupt()                 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x00, "3 Crashes" )
	PORT_DIPSETTING ( 0x08, "4 Crashes" )
	PORT_DIPNAME ( 0x04, 0x04, "Boom Switch", IP_KEY_NONE )
	PORT_DIPSETTING ( 0x00, "Off" )
	PORT_DIPSETTING ( 0x04, "On" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START  /* IN1 */
	/* Right Player */
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	/* Upper Player */
    PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Upper/Left", OSD_KEY_R, IP_JOY_NONE, 0 )
    PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Upper/Down", OSD_KEY_T, IP_JOY_NONE, 0 )
    PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Upper/Right", OSD_KEY_Y, IP_JOY_NONE, 0 )
    PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Upper/Up", OSD_KEY_5, IP_JOY_NONE, 0 )
	PORT_START  /* IN2 */
	/* Left Player */
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	/* Lower Player */
    PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Lower/Left", OSD_KEY_B, IP_JOY_NONE, 0 )
    PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Lower/Down", OSD_KEY_N, IP_JOY_NONE, 0 )
    PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Lower/Right", OSD_KEY_M, IP_JOY_NONE, 0 )
    PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Lower/Up", OSD_KEY_H, IP_JOY_NONE, 0 )
INPUT_PORTS_END

static unsigned char palette[] =
{
        0x00,0x00,0x00, /* BLACK */
        0x00,0xff,0x00, /* GREEN */		/* overlay (Blockade) */
        0xff,0xff,0xff, /* WHITE */		/* Comotion */
        0xff,0x00,0x00, /* RED */		/* for the disclaimer text */
};

static unsigned char colortable[] =
{
        0x00, 0x01,			/* green on black (Blockade) */
		0x00, 0x02,			/* white on black (Comotion) */
};

static struct GfxLayout blockade_layout =
{
	8,8,	/* 8*8 characters */
	32,	/* 32 characters */
	1,	/* 1 bit per pixel */
	{ 0 },	/* no separation in 1 bpp */
	{ 4,5,6,7,256*8+4,256*8+5,256*8+6,256*8+7 },	/* Merge nibble-wide roms */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &blockade_layout,   0, 2 },
	{ -1 } /* end of array */
};

static struct Samplesinterface samples_interface =
{
	1	/* 1 channel */
};

static struct MachineDriver blockade_machine_driver =
{
	/* basic machine hardware */
	{
		{
            CPU_Z80, /* Really an 8080A */
            2079000,
			0,
			readmem,writemem,readport,writeport,
            blockade_interrupt,1
		},
	},
        60, DEFAULT_60HZ_VBLANK_DURATION,
        1,
	0,

	/* video hardware */
	32*8, 28*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	blockade_vh_screenrefresh,

	/* sound hardware */
    0,
    0,          /* Start audio  */
    0,          /* Stop audio   */
    0,          /* Update audio */
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static struct MachineDriver comotion_machine_driver =
{
	/* basic machine hardware */
	{
		{
            CPU_Z80, /* Really an 8080A */
            2079000,
			0,
			readmem,writemem,readport,writeport,
            blockade_interrupt,1
		},
	},
        60, DEFAULT_60HZ_VBLANK_DURATION,
        1,
	0,

	/* video hardware */
	32*8, 28*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	comotion_vh_screenrefresh,

	/* sound hardware */
    0,
    0,          /* Start audio  */
    0,          /* Stop audio   */
    0,          /* Update audio */
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

ROM_START( blockade_rom )
    ROM_REGION(0x10000) /* 64k for code */
	/* Note: These are being loaded into a bogus location, */
	/*       They are nibble wide rom images which will be */
	/*       merged and loaded into the proper place by    */
	/*       blockade_rom_init()                           */
	ROM_LOAD( "316-04.u2",    0x1000, 0x0400, 0xb4090e07 )
	ROM_LOAD( "316-03.u3",    0x1400, 0x0400, 0xff5a0c08 )
    ROM_REGION(0x200)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "316-02.u29",   0x0000, 0x0100, 0x2c2b0a0d )
	ROM_LOAD( "316-01.u43",   0x0100, 0x0100, 0x35720802 )
ROM_END

ROM_START( comotion_rom )
    ROM_REGION(0x10000) /* 64k for code */
	/* Note: These are being loaded into a bogus location, */
	/*       They are nibble wide rom images which will be */
	/*       merged and loaded into the proper place by    */
	/*       comotion_rom_init()                           */
	ROM_LOAD( "316-07.u2",    0x1000, 0x0400, 0x434a0d04 )
	ROM_LOAD( "316-08.u3",    0x1400, 0x0400, 0x45a40d0e )
	ROM_LOAD( "316-09.u4",    0x1800, 0x0400, 0x2d060906 )
	ROM_LOAD( "316-10.u5",    0x1c00, 0x0400, 0x43b9040d )
    ROM_REGION(0x200)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "316-06.u43",   0x0000, 0x0100, 0x6fcc050e )  /* Note: these are reversed */
	ROM_LOAD( "316-05.u29",   0x0100, 0x0100, 0x7ee00a0a )
ROM_END

static const char *blockade_sample_name[] =
{
	"*blockade",
	"BOOM.SAM",
	0	/* end of array */
};

struct GameDriver blockade_driver =
{
    "Blockade",
    "blockade",
    "Frank Palazzolo",
	&blockade_machine_driver,

    blockade_rom,
	blockade_rom_init,
	0,
	blockade_sample_name,
	0,	/* sound_prom */

    blockade_input_ports,

    0, palette, colortable,

	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver comotion_driver =
{
    "Comotion",
    "comotion",
    "Frank Palazzolo",
	&comotion_machine_driver,

    comotion_rom,
	comotion_rom_init,
	0,
	blockade_sample_name,
	0,	/* sound_prom */

    comotion_input_ports,

    0, palette, colortable,

	ORIENTATION_DEFAULT,
	0, 0
};



