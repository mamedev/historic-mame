/***************************************************************************

	Atari Sprint 2 hardware

	driver by Mike Balfour

	Games supported:
		* Sprint 1
		* Sprint 2

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
	        0000-03FF       WRAM
	        0400-07FF       R: RAM and DISPLAY, W: DISPLAY
	        0800-0BFF       R: SWITCH
	        0C00-0FFF       R: SYNC
	        0C00-0C0F       W: ATTRACT
	        0C10-0C1F       W: SKID1
	        0C20-0C2F       W: SKID2
	        0C30-0C3F       W: LAMP1
	        0C40-0C4F       W: LAMP2
	        0C60-0C6F       W: SPARE
	        0C80-0CFF       W: TIMER RESET (Watchdog)
	        0D00-0D7F       W: COLLISION RESET 1
	        0D80-0DFF       W: COLLISION RESET 2
	        0E00-0E7F       W: STEERING RESET 1
	        0E80-0EFF       W: STEERING RESET 2
	        0F00-0F7F       W: NOISE RESET
	        1000-13FF       R: COLLISION1
	        1400-17FF       R: COLLISION2
	        2000-23FF       PROM1 (Playfield ROM1)
	        2400-27FF       PROM2 (Playfield ROM1)
	        2800-2BFF       PROM3 (Playfield ROM2)
	        2C00-2FFF       PROM4 (Playfield ROM2)
	        3000-33FF       PROM5 (Program ROM1)
	        3400-37FF       PROM6 (Program ROM1)
	        3800-3BFF       PROM7 (Program ROM2)
	        3C00-3FFF       PROM8 (Program ROM2)
	       (FC00-FFFF)      PROM8 (Program ROM2) - only needed for the 6502 vectors

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sprint2.h"



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x01, 0x00, /* Black playfield */
	0x01, 0x03, /* White playfield */
	0x01, 0x03, /* White car */
	0x01, 0x00, /* Black car */
	0x01, 0x02, /* Grey car 1 */
	0x01, 0x02  /* Grey car 2 */
};

static PALETTE_INIT( sprint2 )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK - oil slicks, text, black car */
	palette_set_color(1,0x55,0x55,0x55); /* DK GREY - default background */
	palette_set_color(2,0x80,0x80,0x80); /* LT GREY - grey cars */
	palette_set_color(3,0xff,0xff,0xff); /* WHITE - track, text, white car */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Sound handlers
 *
 *************************************/
WRITE_HANDLER( sprint2_skid1_w )
{
	discrete_sound_w(0, offset & 1);
}

WRITE_HANDLER( sprint2_skid2_w )
{
	discrete_sound_w(1, offset & 1);
}

WRITE_HANDLER( sprint2_attract_w )
{
	discrete_sound_w(5, !(offset & 1));
}

WRITE_HANDLER( sprint2_noise_reset_w )
{
	discrete_sound_w(6, 0);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, MRA_RAM }, /* WRAM */
	{ 0x0400, 0x07ff, MRA_RAM }, /* DISPLAY RAM */
	{ 0x0800, 0x083f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0840, 0x087f, sprint2_coins_r },
	{ 0x0880, 0x08bf, sprint2_steering1_r },
	{ 0x08c0, 0x08ff, sprint2_steering2_r },
	{ 0x0900, 0x093f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0940, 0x097f, sprint2_coins_r },
	{ 0x0980, 0x09bf, sprint2_steering1_r },
	{ 0x09c0, 0x09ff, sprint2_steering2_r },
	{ 0x0a00, 0x0a3f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0a40, 0x0a7f, sprint2_coins_r },
	{ 0x0a80, 0x0abf, sprint2_steering1_r },
	{ 0x0ac0, 0x0aff, sprint2_steering2_r },
	{ 0x0b00, 0x0b3f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0b40, 0x0b7f, sprint2_coins_r },
	{ 0x0b80, 0x0bbf, sprint2_steering1_r },
	{ 0x0bc0, 0x0bff, sprint2_steering2_r },
	{ 0x0c00, 0x0fff, sprint2_read_sync_r }, /* SYNC */
	{ 0x1000, 0x13ff, sprint2_collision1_r }, /* COLLISION 1 */
	{ 0x1400, 0x17ff, sprint2_collision2_r }, /* COLLISION 2 */
	{ 0x2000, 0x3fff, MRA_ROM }, /* PROM1-PROM8 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM8 for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM }, /* WRAM */
	{ 0x0010, 0x0013, MWA_RAM, &sprint2_horiz_ram }, /* WRAM */
	{ 0x0014, 0x0016, MWA_RAM, &sprint2_sound_ram }, /* WRAM */
	{ 0x0018, 0x001f, MWA_RAM, &sprint2_vert_car_ram }, /* WRAM */
	{ 0x0400, 0x07ff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x0c00, 0x0c0f, sprint2_attract_w }, /* ATTRACT */
	{ 0x0c10, 0x0c1f, sprint2_skid1_w }, /* SKID1 */
	{ 0x0c20, 0x0c2f, sprint2_skid2_w }, /* SKID2 */
	{ 0x0c30, 0x0c3f, sprint2_lamp1_w }, /* LAMP1 */
	{ 0x0c40, 0x0c4f, sprint2_lamp2_w }, /* LAMP2 */
	{ 0x0c60, 0x0c6f, MWA_RAM }, /* SPARE */
	{ 0x0c80, 0x0cff, MWA_NOP }, /* TIMER RESET (watchdog) */
	{ 0x0d00, 0x0d7f, sprint2_collision_reset1_w }, /* COLLISION RESET 1 */
	{ 0x0d80, 0x0dff, sprint2_collision_reset2_w }, /* COLLISION RESET 2 */
	{ 0x0e00, 0x0e7f, sprint2_steering_reset1_w }, /* STEERING RESET 1 */
	{ 0x0e80, 0x0eff, sprint2_steering_reset2_w }, /* STEERING RESET 2 */
	{ 0x0f00, 0x0f7f, sprint2_noise_reset_w }, /* NOISE RESET */
	{ 0x2000, 0x3fff, MWA_ROM }, /* PROM1-PROM8 */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( sprint2 )
	PORT_START      /* DSW - fake port, gets mapped to Sprint2 ports */
	PORT_DIPNAME( 0x01, 0x00, "Tracks on Demo" )
	PORT_DIPSETTING(    0x00, "Easy Track Only" )
	PORT_DIPSETTING(    0x01, "Cycle 12 Tracks" )
	PORT_DIPNAME( 0x02, 0x00, "Oil Slicks" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage )  )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Allow Extended Play" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0x00, "Real Game Length" )
	PORT_DIPSETTING(    0xc0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START      /* IN0 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "P2 1st gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2, "P2 2nd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2, "P2 3rd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2, "P2 4th gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2, "P1 1st gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON3, "P1 2nd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON4, "P1 3rd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_BUTTON5, "P1 4th gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN1 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1, "P1 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON6, "Track Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 100, 10, 0, 0 )

	PORT_START      /* IN5 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 100, 10, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( sprint1 )
	PORT_START      /* DSW - fake port, gets mapped to Sprint2 ports */
	PORT_DIPNAME( 0x01, 0x00, "Change Track" )
	PORT_DIPSETTING(    0x01, "Every Lap" )
	PORT_DIPSETTING(    0x00, "Every 2 Laps" )
	PORT_DIPNAME( 0x02, 0x00, "Oil Slicks" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage )  )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Allow Extended Play" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0x00, "Real Game Length" )
	PORT_DIPSETTING(    0xc0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START      /* IN0 - fake port, gets mapped to Sprint ports */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "1st gear", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3, "2nd gear", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4, "3rd gear", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5, "4th gear", KEYCODE_V, IP_JOY_DEFAULT )
	PORT_BIT (0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 - fake port, gets mapped to Sprint ports */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 100, 10, 0, 0 )

	PORT_START      /* IN5 */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	64,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout carlayout =
{
	16,8,
	32,
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8  },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 2 },
	{ REGION_GFX2, 0, &carlayout,  4, 4 },
	{ -1 }
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_INIT( sprint1 )
{
	sprintx_is_sprint2 = 0;
}

static MACHINE_INIT( sprint2 )
{
	sprintx_is_sprint2 = 1;
}


/************************************************************************/
/* sprint2 Sound System Analog emulation                               */
/************************************************************************/

const struct discrete_lfsr_desc sprint2_lfsr={
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is not inverted */
	15			/* Output bit */
};

/* Nodes - Inputs */
#define SPRINT2_MOTORSND1_DATA		NODE_01
#define SPRINT2_MOTORSND2_DATA		NODE_02
#define SPRINT2_SKIDSND1_EN		NODE_03
#define SPRINT2_SKIDSND2_EN		NODE_04
#define SPRINT2_CRASHSND_DATA		NODE_05
#define SPRINT2_ATTRACT_EN		NODE_06
#define SPRINT2_NOISE_RESET		NODE_07
/* Nodes - Sounds */
#define SPRINT2_MOTORSND1		NODE_10
#define SPRINT2_MOTORSND2		NODE_11
#define SPRINT2_CRASHSND		NODE_12
#define SPRINT2_SKIDSND1		NODE_13
#define SPRINT2_SKIDSND2		NODE_14
#define SPRINT2_NOISE			NODE_15
#define SPRINT2_FINAL_MIX1		NODE_16
#define SPRINT2_FINAL_MIX2		NODE_17

static DISCRETE_SOUND_START(sprint2_sound_interface)
	/************************************************/
	/* Sprint2 sound system: 5 Sound Sources        */
	/*                     Relative Volume          */
	/*    1/2) Motor           84.69%               */
	/*      2) Crash          100.00%               */
	/*    4/5) Screech/Skid    40.78%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Skid enable Car1                      */
	/*  $01 - Skid enable Car2                      */
	/*  $02 - Motorsound frequency Car1             */
	/*  $03 - Motorsound frequency Car2             */
	/*  $04 - Crash volume                          */
	/*  $05 - Attract                               */
	/*  $06 - Noise Reset                           */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for sprint2           */
	/************************************************/
	/*                    NODE                       ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUT      (SPRINT2_SKIDSND1_EN       , 0x00, 0x000f,                   0.0)
	DISCRETE_INPUT      (SPRINT2_SKIDSND2_EN       , 0x01, 0x000f,                   0.0)
	DISCRETE_INPUT      (SPRINT2_MOTORSND1_DATA    , 0x02, 0x000f,                   0.0)
	DISCRETE_INPUT      (SPRINT2_MOTORSND2_DATA    , 0x03, 0x000f,                   0.0)
	DISCRETE_INPUTX     (SPRINT2_CRASHSND_DATA     , 0x04, 0x000f, 1000.0/15.0, 0,   0.0)
	DISCRETE_INPUT      (SPRINT2_ATTRACT_EN        , 0x05, 0x000f,                   0.0)
	DISCRETE_INPUT_PULSE(SPRINT2_NOISE_RESET       , 0x06, 0x000f,                   1.0)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 10uf capacitor on the R-ladder.         */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, SPRINT2_MOTORSND1_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, (382.0-27.0)/12/15, DISC_LOGADJ, "Motor 1 RPM")
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND1, SPRINT2_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Car2 motor sound is basically the same as    */
	/* Car1.  But I shifted the frequencies up for  */
	/* it to sound different from car1.             */
	/************************************************/
	DISCRETE_RCFILTER(NODE_40, 1, SPRINT2_MOTORSND2_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_41, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, (522.0-27.0)/12/15, DISC_LOGADJ, "Motor 2 RPM")
	DISCRETE_MULTIPLY(NODE_42, 1, NODE_40, NODE_41)

	DISCRETE_MULTADD(NODE_43, 1, NODE_42, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_44, 1, NODE_43, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_45, 1, NODE_44, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_46, 1, NODE_42, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_47, 1, NODE_46, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_48, 1, NODE_47, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_49, 1, NODE_42, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_50, 1, NODE_49, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_51, 1, NODE_50, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND2, SPRINT2_ATTRACT_EN, NODE_45, NODE_48, NODE_51)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SPRINT2_NOISE, 1, SPRINT2_NOISE_RESET, 15750.0/4, 1.0, 0, 0, &sprint2_lfsr)

	DISCRETE_MULTIPLY(NODE_60, 1, SPRINT2_NOISE, SPRINT2_CRASHSND_DATA)
	DISCRETE_RCFILTER(SPRINT2_CRASHSND, 1, NODE_60, 545, 1e-7)

	/************************************************/
	/* Skid circuits takes the noise output from    */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, SPRINT2_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(NODE_73, 1, NODE_72, 407.8, 31.5, 0, 0.0)
	DISCRETE_ONOFF(SPRINT2_SKIDSND1, SPRINT2_SKIDSND1_EN, NODE_73)
	DISCRETE_ONOFF(SPRINT2_SKIDSND2, SPRINT2_SKIDSND2_EN, NODE_73)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, SPRINT2_MOTORSND1, SPRINT2_CRASHSND, SPRINT2_SKIDSND1)
	DISCRETE_ADDER3(NODE_91, 1, SPRINT2_MOTORSND2, SPRINT2_CRASHSND, SPRINT2_SKIDSND2)
	DISCRETE_GAIN(SPRINT2_FINAL_MIX1, NODE_90, 65534.0/(846.9+1000.0+407.8))
	DISCRETE_GAIN(SPRINT2_FINAL_MIX2, NODE_91, 65534.0/(846.9+1000.0+407.8))

	DISCRETE_OUTPUT_STEREO(SPRINT2_FINAL_MIX2, SPRINT2_FINAL_MIX1, 100)
DISCRETE_SOUND_END

static DISCRETE_SOUND_START(sprint1_sound_interface)
	/************************************************/
	/* Sprint1 sound system: 3 Sound Sources        */
	/*                     Relative Volume          */
	/*      1) Motor           84.69%               */
	/*      2) Crash          100.00%               */
	/*      3) Screech/Skid    40.78%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Skid enable                           */
	/*  $02 - Motorsound frequency                  */
	/*  $04 - Crash volume                          */
	/*  $05 - Attract                               */
	/*  $06 - Noise Reset                           */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for sprint1           */
	/************************************************/
	/*                   NODE                  ADDR   MASK   INIT */
	DISCRETE_INPUT(SPRINT2_SKIDSND1_EN       , 0x00, 0x000f, 0.0)
	DISCRETE_INPUT(SPRINT2_MOTORSND1_DATA    , 0x02, 0x000f, 0.0)
	DISCRETE_INPUTX(SPRINT2_CRASHSND_DATA    , 0x04, 0x000f, 1000.0/15.0, 0, 0.0)
	DISCRETE_INPUT(SPRINT2_ATTRACT_EN        , 0x05, 0x000f, 0.0)
	DISCRETE_INPUT(SPRINT2_NOISE_RESET       , 0x06, 0x000f, 1.0)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 10uf capacitor on the R-ladder.         */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, SPRINT2_MOTORSND1_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, (382.0-27.0)/12/15, DISC_LOGADJ, "Motor RPM")
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND1, SPRINT2_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SPRINT2_NOISE, 1, SPRINT2_NOISE_RESET, 15750.0/4, 1.0, 0, 0, &sprint2_lfsr)

	DISCRETE_MULTIPLY(NODE_60, SPRINT2_CRASHSND_DATA, SPRINT2_NOISE, SPRINT2_CRASHSND_DATA)
	DISCRETE_RCFILTER(SPRINT2_CRASHSND, SPRINT2_CRASHSND_DATA, NODE_60, 545, 1e-7)

	/************************************************/
	/* Skid circuit takes the noise output from     */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, SPRINT2_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(SPRINT2_SKIDSND1, SPRINT2_SKIDSND1_EN, NODE_72, 407.8, 31.5, 0, 0.0)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, SPRINT2_MOTORSND1, SPRINT2_CRASHSND, SPRINT2_SKIDSND1)
	DISCRETE_GAIN(SPRINT2_FINAL_MIX1, NODE_90, 65534.0/(846.9+1000.0+407.8))

	DISCRETE_OUTPUT(SPRINT2_FINAL_MIX1, 100)
DISCRETE_SOUND_END


static MACHINE_DRIVER_START( sprint2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 333333)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(sprint2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 29*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))

	MDRV_PALETTE_INIT(sprint2)
	MDRV_VIDEO_START(sprint2)
	MDRV_VIDEO_UPDATE(sprint2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, sprint2_sound_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sprint1 )

	MDRV_IMPORT_FROM(sprint2)

	/* basic machine hardware */
	MDRV_MACHINE_INIT(sprint1)

	/* video hardware */
	MDRV_VIDEO_UPDATE(sprint1)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_REPLACE("discrete", DISCRETE, sprint1_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( sprint1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6290-01.b1",   0x2000, 0x0800, CRC(41fc985e) SHA1(7178846480cbf8d15955ccd987d0b0e902ab9f90) )
	ROM_LOAD( "6291-01.c1",   0x2800, 0x0800, CRC(07f7a920) SHA1(845f65d2bd290eb295ca6bae2575f27aaa08c0dd) )
	ROM_LOAD( "6442-01.d1",   0x3000, 0x0800, CRC(e9ff0124) SHA1(42fe028e2e595573ccc0821de3bb6970364c585d) )
	ROM_RELOAD(               0xf000, 0x0800 )
	ROM_LOAD( "6443-01.e1",   0x3800, 0x0800, CRC(d6bb00d0) SHA1(cdcd4bb7b32be7a11480d3312fcd8d536e2d0caf) )
	ROM_RELOAD(               0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6396-01.p4",   0x0000, 0x0200, CRC(801b42dd) SHA1(1db58390d803f404253cbf36d562016441ca568d) )
	ROM_LOAD_NIB_LOW ( "6397-01.r4",   0x0000, 0x0200, CRC(135ba1aa) SHA1(0465259440f73e1a2c8d8101f29e99b4885420e4) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6399-01.j6",   0x0000, 0x0200, CRC(63d685b2) SHA1(608746163e25dbc14cde43c17aecbb9a14fac875) )
	ROM_LOAD_NIB_LOW ( "6398-01.k6",   0x0000, 0x0200, CRC(c9e1017e) SHA1(e7279a13e4a812d2e0218be0bc5162f2e56c6b66) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6401-01.e2",   0x0000, 0x0020, CRC(857df8db) SHA1(06313d5bde03220b2bc313d18e50e4bb1d0cfbbb) )	/* Address PROM, not used. Put here for posterity. */
ROM_END

ROM_START( sprint2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6290-01.b1",   0x2000, 0x0800, CRC(41fc985e) SHA1(7178846480cbf8d15955ccd987d0b0e902ab9f90) )
	ROM_LOAD( "6291-01.c1",   0x2800, 0x0800, CRC(07f7a920) SHA1(845f65d2bd290eb295ca6bae2575f27aaa08c0dd) )
	ROM_LOAD( "6404sp2.d1",   0x3000, 0x0800, CRC(d2878ff6) SHA1(b742a8896c1bf1cfacf48d06908920d88a2c9ea8) )
	ROM_RELOAD(               0xf000, 0x0800 )
	ROM_LOAD( "6405sp2.e1",   0x3800, 0x0800, CRC(6c991c80) SHA1(c30a5b340f05dd702c7a186eb62607a48fa19f72) )
	ROM_RELOAD(               0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6396-01.p4",   0x0000, 0x0200, CRC(801b42dd) SHA1(1db58390d803f404253cbf36d562016441ca568d) )
	ROM_LOAD_NIB_LOW ( "6397-01.r4",   0x0000, 0x0200, CRC(135ba1aa) SHA1(0465259440f73e1a2c8d8101f29e99b4885420e4) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6399-01.j6",   0x0000, 0x0200, CRC(63d685b2) SHA1(608746163e25dbc14cde43c17aecbb9a14fac875) )
	ROM_LOAD_NIB_LOW ( "6398-01.k6",   0x0000, 0x0200, CRC(c9e1017e) SHA1(e7279a13e4a812d2e0218be0bc5162f2e56c6b66) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6401-01.e2",   0x0000, 0x0020, CRC(857df8db) SHA1(06313d5bde03220b2bc313d18e50e4bb1d0cfbbb) )	/* Address PROM, not used. Put here for posterity. */
ROM_END



/*    YEAR  NAME      PARENT   MACHINE   INPUT  INIT MONITOR  */
GAME( 1978, sprint1,  0,       sprint1,  sprint1, 0, ROT0, "Atari", "Sprint 1" )
GAME( 1976, sprint2,  sprint1, sprint2,  sprint2, 0, ROT0, "Atari", "Sprint 2" )
