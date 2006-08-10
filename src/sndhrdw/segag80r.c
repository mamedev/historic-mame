/***************************************************************************

    Sega G-80 raster hardware

    Across these games, there's a mixture of discrete sound circuitry,
    speech boards, ADPCM samples, and a TMS3617 music chip.

    08-JAN-1999 - MAB:
     - added NEC 7751 support to Monster Bash

    05-DEC-1998 - MAB:
     - completely rewrote sound code to use Samples interface.
       (It's based on the Zaxxon sndhrdw code.)

    TODO:
     - Astro Blaster needs "Attack Rate" modifiers implemented
     - Sample support for 005
     - Melody support for 005
     - Sound for Pig Newton
     - Speech for Astro Blaster

    - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "segag80r.h"
#include "machine/8255ppi.h"
#include "sound/samples.h"
#include "sound/tms36xx.h"
#include "sound/dac.h"

#define TOTAL_SOUNDS 16

struct sa
{
	int channel;
	int num;
	int looped;
	int stoppable;
	int restartable;
};

/***************************************************************************
Astro Blaster

Sound is created through two boards.
The Astro Blaster Sound Board consists solely of sounds created through
discrete circuitry.
The G-80 Speech Synthesis Board consists of an 8035 and a speech synthesizer.
It is implemented in segasnd.c.
***************************************************************************/

const char *astrob_sample_names[] =
{
	"*astrob",
	"invadr1.wav","winvadr1.wav","invadr2.wav","winvadr2.wav",
	"invadr3.wav","winvadr3.wav","invadr4.wav","winvadr4.wav",
	"asteroid.wav","refuel.wav",

	"pbullet.wav","ebullet.wav","eexplode.wav","pexplode.wav",
	"deedle.wav","sonar.wav",
	0
};

enum
{
	invadr1 = 0,winvadr1,invadr2,winvadr2,
	invadr3,winvadr3,invadr4,winvadr4,
	asteroid,refuel,
	pbullet,ebullet,eexplode,pexplode,
	deedle,sonar
};

static struct sa astrob_sa[TOTAL_SOUNDS] =
{
	/* Port 0x3E: */
	{  0, invadr1,  1, 1, 1 },      /* Line  0 - Invader 1 */
	{  1, invadr2,  1, 1, 1 },      /* Line  1 - Invader 2 */
	{  2, invadr3,  1, 1, 1 },      /* Line  2 - Invader 3 */
	{  3, invadr4,  1, 1, 1 },      /* Line  3 - Invader 4 */
	{  4, asteroid, 1, 1, 1 },      /* Line  4 - Asteroids */
	{ -1 },                                         /* Line  5 - <Mute> */
	{  5, refuel,   0, 1, 0 },      /* Line  6 - Refill */
	{ -1 },                                         /* Line  7 - <Warp Modifier> */

	/* Port 0x3F: */
	{  6, pbullet,  0, 0, 1 },      /* Line  0 - Laser #1 */
	{  7, ebullet,  0, 0, 1 },      /* Line  1 - Laser #2 */
	{  8, eexplode, 0, 0, 1 },      /* Line  2 - Short Explosion */
	{  8, pexplode, 0, 0, 1 },      /* Line  3 - Long Explosion */
	{ -1 },                                         /* Line  4 - <Attack Rate> */
	{ -1 },                                         /* Line  5 - <Rate Reset> */
	{  9, deedle,   0, 0, 1 },      /* Line  6 - Bonus */
	{ 10, sonar,    0, 0, 1 },      /* Line  7 - Sonar */
};

WRITE8_HANDLER( astrob_audio_ports_w )
{
	int line;
	int noise;
	int warp = 0;

	/* First, handle special control lines: */

	/* MUTE */
	if ((offset == 0) && (data & 0x20))
	{
		/* Note that this also stops our speech from playing. */
		/* (If our speech ever gets synthesized, this will probably
           need to call some type of speech_mute function) */
		for (noise = 0; noise < TOTAL_SOUNDS; noise++)
			sample_stop(astrob_sa[noise].channel);
		return;
	}

	/* WARP */
	if ((offset == 0) && (!(data & 0x80)))
	{
		warp = 1;
	}

	/* ATTACK RATE */
	if ((offset == 1) && (!(data & 0x10)))
	{
		/* TODO: this seems to modify the speed of the invader sounds */
	}

	/* RATE RESET */
	if ((offset == 1) && (!(data & 0x20)))
	{
		/* TODO: this seems to modify the speed of the invader sounds */
	}

	/* Now, play our discrete sounds */
	for (line = 0;line < 8;line++)
	{
		noise = 8 * offset + line;

		if (astrob_sa[noise].channel != -1)
		{
			/* trigger sound */
			if ((data & (1 << line)) == 0)
			{
				/* Special case: If we're on Invaders sounds, modify with warp */
				if ((astrob_sa[noise].num >= invadr1) &&
					(astrob_sa[noise].num <= invadr4))
				{
					if (astrob_sa[noise].restartable || !sample_playing(astrob_sa[noise].channel))
						sample_start(astrob_sa[noise].channel, astrob_sa[noise].num + warp, astrob_sa[noise].looped);
				}
				/* All other sounds are normal */
				else
				{
					if (astrob_sa[noise].restartable || !sample_playing(astrob_sa[noise].channel))
						sample_start(astrob_sa[noise].channel, astrob_sa[noise].num, astrob_sa[noise].looped);
				}
			}
			else
			{
				if (sample_playing(astrob_sa[noise].channel) && astrob_sa[noise].stoppable)
					sample_stop(astrob_sa[noise].channel);
			}
		}
	}
}

/***************************************************************************
005

The Sound Board seems to consist of the following:
An 8255:
  Port A controls the sounds that use discrete circuitry
    A0 - Large Expl. Sound Trig
    A1 - Small Expl. Sound Trig
    A2 - Drop Sound Bomb Trig
    A3 - Shoot Sound Pistol Trig
    A4 - Missile Sound Trig
    A5 - Helicopter Sound Trig
    A6 - Whistle Sound Trig
    A7 - <unused>
  Port B controls the melody generator (described below)
    B0-B3 - connected to addr lines A7-A10 of the 2716 sound ROM
    B4 - connected to 1CL and 2CL on the LS393, and to RD on an LS293 just before output
    B5-B6 - connected to 1CK on the LS393
    B7 - <unused>
  Port C is apparently unused

Melody Generator:
    There's an LS393 counter hooked to addr lines A0-A6 of a 2716 ROM.
    Port B from the 8255 hooks to addr lines A7-A10 of the 2716.
    D0-D4 output from the 2716 into an 6331.
    D5 outputs from the 2716 to a 555 timer.
    D0-D7 on the 6331 output to two LS161s.
    The LS161s output to the LS293 (also connected to 8255 B4).
    The output of this feeds into a 4391, which produces "MELODY" output.
***************************************************************************/

const char *s005_sample_names[] =
{
		0
};

/***************************************************************************
Space Odyssey

The Sound Board consists solely of sounds created through discrete circuitry.
***************************************************************************/

const char *spaceod_sample_names[] =
{
		"fire.wav", "bomb.wav", "eexplode.wav", "pexplode.wav",
		"warp.wav", "birth.wav", "scoreup.wav", "ssound.wav",
		"accel.wav", "damaged.wav", "erocket.wav",
		0
};

enum
{
		sofire = 0,sobomb,soeexplode,sopexplode,
		sowarp,sobirth,soscoreup,sossound,
		soaccel,sodamaged,soerocket
};

static struct sa spaceod_sa[TOTAL_SOUNDS] =
{
		/* Port 0x0E: */
//      {  0, sossound,   1, 1, 1 },    /* Line  0 - background noise */
		{ -1 },                                                 /* Line  0 - background noise */
		{ -1 },                                                 /* Line  1 - unused */
		{  1, soeexplode, 0, 0, 1 },    /* Line  2 - Short Explosion */
		{ -1 },                                                 /* Line  3 - unused */
		{  2, soaccel,    0, 0, 1 },    /* Line  4 - Accelerate */
		{  3, soerocket,  0, 0, 1 },    /* Line  5 - Battle Star */
		{  4, sobomb,     0, 0, 1 },    /* Line  6 - D. Bomb */
		{  5, sopexplode, 0, 0, 1 },    /* Line  7 - Long Explosion */
		/* Port 0x0F: */
		{  6, sofire,     0, 0, 1 },    /* Line  0 - Shot */
		{  7, soscoreup,  0, 0, 1 },    /* Line  1 - Bonus Up */
		{ -1 },                                                 /* Line  2 - unused */
		{  8, sowarp,     0, 0, 1 },    /* Line  3 - Warp */
		{ -1 },                                                 /* Line  4 - unused */
		{ -1 },                                                 /* Line  5 - unused */
		{  9, sobirth,    0, 0, 1 },    /* Line  6 - Appearance UFO */
		{ 10, sodamaged,  0, 0, 1 },    /* Line  7 - Black Hole */
};

WRITE8_HANDLER( spaceod_audio_ports_w )
{
		int line;
		int noise;

		for (line = 0;line < 8;line++)
		{
				noise = 8 * offset + line;

				if (spaceod_sa[noise].channel != -1)
				{
						/* trigger sound */
						if ((data & (1 << line)) == 0)
						{
								if (spaceod_sa[noise].restartable || !sample_playing(spaceod_sa[noise].channel))
										sample_start(spaceod_sa[noise].channel, spaceod_sa[noise].num, spaceod_sa[noise].looped);
						}
						else
						{
								if (sample_playing(spaceod_sa[noise].channel) && spaceod_sa[noise].stoppable)
										sample_stop(spaceod_sa[noise].channel);
						}
				}
		}
}

/***************************************************************************
Monster Bash

The Sound Board is a fairly complex mixture of different components.
An 8255A-5 controls the interface to/from the sound board.
Port A connects to a TMS3617 (basic music synthesizer) circuit.
Port B connects to two sounds generated by discrete circuitry.
Port C connects to a NEC7751 (8048 CPU derivative) to control four "samples".
***************************************************************************/

static UINT32 n7751_rom_address = 0;
static UINT8 n7751_command;
static UINT8 n7751_busy;

enum
{
	mbzap = 0,
	mbjumpdown
};



static WRITE8_HANDLER( monsterb_sound_a_w )
{
	int enable_val;

	/* Lower four data lines get decoded into 13 control lines */
	tms36xx_note_w(0, 0, data & 15);

	/* Top four data lines address an 82S123 ROM that enables/disables voices */
	enable_val = memory_region(REGION_SOUND2)[(data & 0xF0) >> 4];
	tms3617_enable_w(0, enable_val >> 2);
}

static WRITE8_HANDLER( monsterb_sound_b_w )
{
	if (!(data & 0x01))
		sample_start(0, mbzap, 0);

	if (!(data & 0x02))
		sample_start(1, mbjumpdown, 0);

    /* TODO: D7 on Port B might affect TMS3617 output (mute?) */
}

static READ8_HANDLER( n7751_status_r )
{
	return n7751_busy << 4;
}


static WRITE8_HANDLER( n7751_command_w )
{
	/*
        Z80 7751 control port

        D0-D2 = connected to 7751 port C
        D3    = /INT line
    */
	n7751_command = data & 0x07;
	cpunum_set_input_line(1, 0, ((data & 0x08) == 0) ? ASSERT_LINE : CLEAR_LINE);
	cpu_boost_interleave(0, TIME_IN_USEC(100));
}


static WRITE8_HANDLER( n7751_rom_offset_w )
{
	/* P4 - address lines 0-3 */
	/* P5 - address lines 4-7 */
	/* P6 - address lines 8-11 */
	int mask = (0xf << (4 * offset)) & 0x3fff;
	int newdata = (data << (4 * offset)) & mask;
	n7751_rom_address = (n7751_rom_address & ~mask) | newdata;
}


static WRITE8_HANDLER( n7751_rom_select_w )
{
	/* P7 - ROM selects */
	int numroms = memory_region_length(REGION_SOUND1) / 0x1000;
	n7751_rom_address &= 0xfff;
	if (!(data & 0x01) && numroms >= 1) n7751_rom_address |= 0x0000;
	if (!(data & 0x02) && numroms >= 2) n7751_rom_address |= 0x1000;
	if (!(data & 0x04) && numroms >= 3) n7751_rom_address |= 0x2000;
	if (!(data & 0x08) && numroms >= 4) n7751_rom_address |= 0x3000;
}


static READ8_HANDLER( n7751_rom_r )
{
	/* read from BUS */
	return memory_region(REGION_SOUND1)[n7751_rom_address];
}


static READ8_HANDLER( n7751_command_r )
{
	/* read from P2 - 8255's PC0-2 connects to 7751's S0-2 (P24-P26 on an 8048) */
	/* bit 0x80 is an alternate way to control the sample on/off; doesn't appear to be used */
	return 0x80 | ((n7751_command & 0x07) << 4);
}


static WRITE8_HANDLER( n7751_busy_w )
{
	/* write to P2 */
	/* output of bit $80 indicates we are ready (1) or busy (0) */
	/* no other outputs are used */
	n7751_busy = data >> 7;
}


static READ8_HANDLER( n7751_t1_r )
{
	/* T1 - labelled as "TEST", connected to ground */
	return 0;
}


static SOUND_START( monsterb )
{
	static const ppi8255_interface ppi_intf =
	{
		1,
		{ 0 },
		{ 0 },
		{ n7751_status_r },
		{ monsterb_sound_a_w },
		{ monsterb_sound_b_w },
		{ n7751_command_w }
	};
	ppi8255_init(&ppi_intf);
	return 0;
}


static ADDRESS_MAP_START( monsterb_7751_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( monsterb_7751_portmap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(n7751_t1_r)
	AM_RANGE(I8039_p2, I8039_p2) AM_READ(n7751_command_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ(n7751_rom_r)
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(DAC_0_data_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(n7751_busy_w)
	AM_RANGE(I8039_p4, I8039_p6) AM_WRITE(n7751_rom_offset_w)
	AM_RANGE(I8039_p7, I8039_p7) AM_WRITE(n7751_rom_select_w)
ADDRESS_MAP_END


static const char *monsterb_sample_names[] =
{
	"*monsterb",
	"zap.wav",
	"jumpdown.wav",
	0
};

static struct Samplesinterface monsterb_samples_interface =
{
	2,    /* 2 channels */
	monsterb_sample_names
};


static struct TMS36XXinterface monsterb_tms3617_interface =
{
	TMS3617,	/* TMS36xx subtype(s) */
	{0.5,0.5,0.5,0.5,0.5,0.5}  /* decay times of voices */
};


MACHINE_DRIVER_START( monsterb_sound )

	/* basic machine hardware */
	MDRV_CPU_ADD(N7751, 6000000/15)
	MDRV_CPU_PROGRAM_MAP(monsterb_7751_map,0)
	MDRV_CPU_IO_MAP(monsterb_7751_portmap,0)

	/* sound hardware */
	MDRV_SOUND_START(monsterb)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(monsterb_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(TMS36XX, 247)
	MDRV_SOUND_CONFIG(monsterb_tms3617_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END
