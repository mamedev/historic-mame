/***************************************************************************

Taito Z System [twin 68K with optional Z80]
-------------------------------------------

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime,
Nicola Salmoria. Thanks to Richard Bush and the Raine team, whose open
source was very helpful in many areas particularly the sprites.)

- Changes Log -

01-26-02 Added Enforce
10-17-01 TC0150ROD support improved (e.g. Aquajack)
09-01-01 Preliminary TC0150ROD support
08-28-01 Fixed uncentered steer inputs, Nightstr controls
05-27-01 Inputs through taitoic ioc routines, contcirc subwoofer filter
04-12-01 Centered steering AD inputs, added digital steer
02-18-01 Added Spacegun gunsights (Insideoutboy)


				*****

The Taito Z system has a number of similarities with the Taito F2 system,
and uses some of the same custom Taito components.

TaitoZ supports 5 separate layers of graphics - one 64x64 tiled scrolling
background plane of 8x8 tiles, a similar foreground plane, another optional
plane used for drawing a road (e.g. Chasehq), a sprite plane [with varying
properties], and a text plane with character definitions held in ram.

(Double Axle has four rather than two background planes, and they contain
32x32 16x16 tiles. This is because it uses a TC0480SCP rather than the
older TC0100SCN tilemap generator used in previous Taito Z games. The
hardware for Taito's Super Chase was a further development of this, with a
68020 for main CPU and Ensoniq sound - standard features of Taito's F3
system. Taito's F3 system superceded both Taito B and F2 systems, but the
Taito Z system was enhanced with F3 features and continued in games like
Super Chase and Under Fire up to the mid 1990s.)

Each Taito Z game used one of the following sprite systems - allowing the
use of big sprites with minimal CPU overhead [*]:

(i)  16x8 tiles aggregated through a spritemap rom into 128x128 sprites
(ii) 16x16 tiles aggregated through a spritemap rom into three sprite sizes:
      128 x 128
       64 x 128
       32 x 128
(iii) 16x8 tiles aggregated through a spritemap rom into 64x64 sprites

[* in Taito B/F2/F3 the CPU has to keep track of all the 16x16 tiles within
a big sprite]

The Z system has twin 68K CPUs which communicate via shared ram.
Typically they share $4000 bytes, but Spacegun / Dbleaxle share $10000.

The first 68000 handles screen, palette and sprites, and sometimes other
jobs [e.g. inputs; in one game it also handles the road].

The second 68000 may handle functions such as:
	(i)  inputs/dips, sound (through a YM2610) and/or
	(ii) the "road" that's in every TaitoZ game except Spacegun.

Most Z system games have a Z80 as well, which takes over sound duties.
Commands are written to it by the one of the 68000s.

The memory map for the Taito Z games is similar in outline but usually
shuffled around: some games have different i/o because of analogue
sticks, light guns, cockpit hardware etc.


Contcirc board (B.Troha)
--------------

Taito Sound PCB J1100137A K1100314A:

  Zilog Z0840004PSC     XTAL OSC          Yamaha
  Z80 CPU               16.000 MHz        YM2610

  TC0060DCA              B33-30
  TC0060DCA
                                            TC0140SYT

                                          B33-08
                                          B33-09
                                          B33-10

Notes: B33-30 is a OKI M27512-15


Taito Video Baord PCB J1100139A K1100316A:

 B33-03     TC0050VDZ     TC0050VDZ                       TC0050VDZ
 B33-04
 B33-05
 B33-06                      TC0020VAR

     B14-31

 B33-07

                      B14-30

Notes: B14-31 is 27HC64 (Sharp LH5763J-70)
       B14-30 is OKI M27512-15
DG:    TC0020VAR + 3xTC0050VDZ may be precursor to 370MSO/300FLA combo


Taito CPU Board J110138A K1100315A:

                                            XTAL OSC  XTAL OSC
                                            24.000MHz 26.686MHz

                                                 B33-02
B33-01
                  TC0150ROD                 TC0100SCN        NEC D43256C-10L
                                                             NEC D43256C-10L

                                                      TC0110PCR

                                          TC0070RGB
 MC6800P12 IC-25 MC68000P12 IC-35
           IC-26            IC 36           TC0040IOC
                                              DSWA  DSWB

Notes: IC-41 Is 271001 Listed as JH1 (unsocketed / unused)
       IC-42 Is 271001 Listed as JL1 (unsocketed / unused)



Aquajack top board (Guru)
------------------

68000-12 x 2
OSC: 26.686, 24.000, 16.000
I dont see any recognisable sound chips, but I do see a YM3016F

TCO110PCR
TCO220IOC
TCO100SCN
TCO140SYT
TCO3200BR
TCO150ROD
TCO020VAR
TCO050VDZ
TCO050VDZ
TCO050VDZ

ChaseHQ (Guru)
-------

Video board
-----------
                 Pal  b52-28d  b52-28b
                 Pal      17d      17b
                 Pal      28c      28a
                 Pal      77c      17a

b52-30
    34
    31
    35
    32                     b52-27  pal20       TC020VAR  b52-03  b52-127
    36                         51                        b52-126  b52-124 Pal
                               50
                               49                  Pal   b52-125
    33   Pal  b52-19                                      Pal   b52-25
    37    38                 b52-18b                      Pal   122
         Pal      Pal        b52-18a                      Pal   123
         b52-20   b52-21     b52-18


CPU board
---------
                                                    b52-119 Pal
                                                    b52-118 Pal
                                68000-12
                            b52-131    129
b52-113                     b52-130    136
b52-114
b52-115                 TC0140SYT
b52-116

             YM2610                          b52-29
                                                      26.686MHz
                                                      24 MHz
           16MHz                             TC0100SCN

       Pal b52-121
       Pal b52-120                      TC0170ABT   TC0110PCR    b52-01
          68000-12
                                        b52-06
         TC0040IOC  b52-133 b52-132  TC0150ROD    b52-28



ChaseHQ2(SCI) custom chips (Guru) (DG: same as Bshark except 0140SYT?)
--------------------------

CPU PCB:
TC0170ABT
TC0150ROD
TC0140SYT
TC0220IOC

c09-23.rom is a
PROM type AM27S21PC, location looks like this...

-------------
|   68000   |
-------------

c09-25    c09-26
c09-24

|-------|
|       |
| ABT   |
|       |
|-------|

c09-23     c09-07

|-------|
|       |
| ROD   |
|       |
|-------|

c09-32   c09-33
-------------
|   68000   |
-------------

c09-21  c09-22

Lower PCB:
TCO270MOD
TC0300FLA
TC0260DAR
TC0370MSO
TC0100SCN
TC0380BSH

c09-16.rom is located next to
c09-05, which is located next to Taito TCO370MSO.


BShark custom chips
-------------------

TC0220IOC (known io chip)
TC0260DAR (known palette chip)
TC0400YSC  substitute for TC0140SYT when 68K writes directly to YM2610 ??
TC0170ABT  = same in Dblaxle
TC0100SCN (known tilemap chip)
TC0370MSO  = same in Dblaxle, Motion Objects ?
TC0300FLA  = same in Dblaxle
TC0270MOD  ???
TC0380BSH  ???
TC0150ROD (known road generator chip)


DblAxle custom chip info
------------------------

TC0150ROD is next to road lines gfx chip [c78-09] but also
c78-15, an unused 256 byte rom. Perhaps this contains color
info for the road lines? Raine makes an artificial "pal map"
for the road, AFAICS.

TC0170ABT is between 68000 CPUA and the TC0140SYT. Next to
that is the Z80A, the YM2610, and the three adpcm roms.

On the graphics board we have the TC0480SCP next to its two
scr gfx roms: c78-10 & 11.

The STY object mapping rom is next to c78-25, an unused
0x10000 byte rom which compresses by 98%. To right of this
are TC0370MSO (motion objects?), then TC0300FLA.

Below c78-25 are two unused 1K roms: c84-10 and c84-11.
Below right is another unused 256 byte rom, c78-21.
(At the bottom are the 5 obj gfx roms.)

K11000635A
----------
 43256   c78-11 SCN1 CHR
 43256   c78-10 SCN0 CHR   TC0480SCP

 c78-04
 STY ROM
            c78-25   TC0370MSO   TC0300FLA
            c84-10
            c84-11                                      c78-21

                       43256 43256 43256 43256
                 43256 43256 43256 43256 43256
                 43256 43256 43256 43256 43256
                                   43256 43256

                             c78-05L
            c78-06 OBJ1
                             c78-05H

            c78-08 OBJ3      c78-07 OBJ2

Power Wheels
------------

Cpu PCB

CPU:	68000-16 x2
Sound:	Z80-A
	YM2610
OSC:	32.000MHz
Chips:	TC0140SYT
	TC0150ROD
	TC0170ABT
	TC0310FAM
	TC0510NIO


Video PCB

OSC:	26.686MHz
Chips:	TC0260DAR
	TC0270MOD
	TC0300FLA
	TC0370MSO
	TC0380BSH
	TC0480SCP


LAN interface board

OSC:	40.000MHz
	16.000MHz
Chips:	uPD72105C


TODO Lists
==========

Add cpu idle time skip to improve speed.

Is the no-Z80 sound handling correct: some voices in Bshark
aren't that clear.

Make taitosnd cpu-independent so we can restore Z80 to CPU3.

Cockpit hardware

DIPs - e.g. coinage

Sprite zooming - dimensions may be got from the unused 64K rom
on the video board (it's in the right place for it, both with
Contcirc video chips and the chips used on later boards). These
64K roms compare as follows - makes sense as these groups
comprise the three sprite layout types used in TaitoZ games:

   Contcirc / Enforce      =IDENTICAL
   ChaseHQ / Nightstr      =IDENTICAL
   Bshark / SCI / Dblaxle  =IDENTICAL

   Missing from Aquajack / Spacegun dumps (I would bet they are
   the same as Bshark). Can anyone dump these two along with any
   proms on the video board?


Continental Circus
------------------

Road priority incompletely understood - e.g. start barrier should
be darkening LH edge of road as well as RH edge.

The 8 level accel / brake should be possible to control with
analogue pedal. Don't think mame can do this.

Junk (?) stuff often written in high byte of sound word.

Speculative YM2610 a/b/c channel filtering as these may be
outputs to subwoofer (vibration). They sound better, anyway.


Chasehq
-------

Motor CPU: appears to be identical to one in Topspeed.

[Used to have junk sprites when you reach criminal car (the 'criminals
here' sprite): two bits above tile number are used. Are these
meaningless, or is some effect missing?]


Enforce
-------

Test mode - SHIFT: LO/HI is not understood (appears to depend on Demo
Sound DSW)

Landscape in the background can be made to scroll rapidly with DSW.
True to original?

Some layer offsets are out a little.


Battle Shark
------------

Is road on the road stages correct? Hard to tell.

Does the original have the "seeking" crosshair effect, making it a
challenge to control?


SCI
---

Road seems ok, but are the green bushes in round 1 a little too far
to the edge of the interroad verge?

Sprite frames were plotted in opposite order so flickered. Reversing
this has lost us alternate frames: probably need to buffer sprite
ram by one frame to solve this?


Night Striker
-------------

Road A/B priority problems will manifest in the choice tunnels with,
one or the other having higher priority in top and bottom halves. Can
someone provide a sequence of screenshots showing exactly what happens
at the road split point.

Strange page in test mode which lets you alter all sorts of settings,
may relate to sit-in cockpit version. Can't find a dip that disables
this.

Does a variety of writes to TC0220IOC offset 3... significant?


Aqua Jack
---------

Sprites left on screen under hiscore table. Deliberate? Or is there
a sprite disable bit somewhere.

Should road body be largely transparent as I've implemented it?

Sprite/sprite priorities often look bad. Sprites go to max size for
a frame before they explode - surely a bug.

Hangs briefly fairly often without massive cpu interleaving (500).
Keys aren't very responsive in test mode.

The problem code is this:

CPUA
$1fe02 hangs waiting for ($6002,A5) in shared ram to be zero.

CPUB
$1056 calls $11ea routine which starts by setting ($6002,A5) non-
zero. At end (after $1218 waiting for a bit from sound comm port)
it alters ($6002,A5) to zero (but this value lasts briefly!).

Unless context rapidly switches back to cpua this change is missed
because $11ea gets called again *very* rapidly at times when sounds
are being written [that's when the problem manifested].

$108a-c2 reads 0x20 bytes from unmapped area, not sure
what it's doing. Perhaps this machine had some optional
exotic input device...


Spacegun
--------

Problem with the zoomed sprites not matching up very well
when forming the background. They jerk a bit relative to
each other... probably a cpu sync thing, perhaps also some
fine-tuning required on the zoomed sprite dimension calcs.

Light gun interrupt timing arbitrary.


Double Axle
-----------

Road occasionally has incorrectly unclipped line appearing at top
(ice stage). Also road 'ghost' often remains on screen - also an
interrupt issue I presume.

Double Axle has poor sound: one ADPCM rom should be twice as long?
[In log we saw stuff like this, suggesting extra ADPCM rom needed:
YM2610: ADPCM-A end out of range: $001157ff
YM2610: ADPCM-A start out of range: $00111f00]

Various sprites go missing e.g. mountains half way through cross
country course. Fall off the ledge and crash and you will see
the explosion sprites make other mountain sprites vanish, as
though their entries in spriteram are being overwritten. (Perhaps
an int6 timing/number issue: sprites seem to be ChaseHQ2ish with
a spriteframe toggle - currently this never changes which may be
wrong.)

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "machine/eeprom.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

VIDEO_START( taitoz );
VIDEO_START( spacegun );

VIDEO_UPDATE( contcirc );
VIDEO_UPDATE( chasehq );
VIDEO_UPDATE( bshark );
VIDEO_UPDATE( sci );
VIDEO_UPDATE( aquajack );
VIDEO_UPDATE( spacegun );
VIDEO_UPDATE( dblaxle );

READ16_HANDLER ( sci_spriteframe_r );
WRITE16_HANDLER( sci_spriteframe_w );

//  These TC0150ROD prototypes will go in taitoic.h  //
READ16_HANDLER ( TC0150ROD_word_r );	/* Road generator */
WRITE16_HANDLER( TC0150ROD_word_w );

static UINT16 cpua_ctrl = 0xff;
static int sci_int6 = 0;
static int dblaxle_int6 = 0;
static int ioc220_port = 0;
static data16_t eep_latch = 0;

//static data16_t *taitoz_ram;
//static data16_t *motor_ram;

static size_t taitoz_sharedram_size;
data16_t *taitoz_sharedram;	/* read externally to draw Spacegun crosshair */

static READ16_HANDLER( sharedram_r )
{
	return taitoz_sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&taitoz_sharedram[offset]);
}

static void parse_control(void)
{
	/* bit 0 enables cpu B */
	/* however this fails when recovering from a save state
	   if cpu B is disabled !! */
	cpu_set_reset_line(2,(cpua_ctrl &0x1) ? CLEAR_LINE : ASSERT_LINE);

}

static void parse_control_noz80(void)
{
	/* bit 0 enables cpu B */
	/* however this fails when recovering from a save state
	   if cpu B is disabled !! */
	cpu_set_reset_line(1,(cpua_ctrl &0x1) ? CLEAR_LINE : ASSERT_LINE);

}

static WRITE16_HANDLER( cpua_ctrl_w )	/* assumes Z80 sandwiched between 68Ks */
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;	/* for Wgp */
	cpua_ctrl = data;

	parse_control();

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",activecpu_get_pc(),data);
}

static WRITE16_HANDLER( cpua_noz80_ctrl_w )	/* assumes no Z80 */
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;	/* for Wgp */
	cpua_ctrl = data;

	parse_control_noz80();

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",activecpu_get_pc(),data);
}


/***********************************************************
                        INTERRUPTS
***********************************************************/

/* 68000 A */

static void taitoz_interrupt6(int x)
{
	cpu_set_irq_line(0,6,HOLD_LINE);
}

/* 68000 B */

#if 0
static void taitoz_cpub_interrupt5(int x)
{
	cpu_set_irq_line(2,5,HOLD_LINE);	/* assumes Z80 sandwiched between the 68Ks */
}
#endif

static void taitoz_sg_cpub_interrupt5(int x)
{
	cpu_set_irq_line(1,5,HOLD_LINE);	/* assumes no Z80 */
}

#if 0
static void taitoz_cpub_interrupt6(int x)
{
	cpu_set_irq_line(2,6,HOLD_LINE);	/* assumes Z80 sandwiched between the 68Ks */
}
#endif


/***** Routines for particular games *****/

static INTERRUPT_GEN( sci_interrupt )
{
	/* Need 2 int4's per int6 else (-$6b63,A5) never set to 1 which
	   causes all sprites to vanish! Spriteram has areas for 2 frames
	   so in theory only needs updating every other frame. */

	sci_int6 = !sci_int6;

	if (sci_int6)
		timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_interrupt6);
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

/* Double Axle seems to keep only 1 sprite frame in sprite ram,
   which is probably wrong. Game seems to work with no int 6's
   at all. Cpu control byte has 0,4,8,c poked into 2nd nibble
   and it seems possible this should be causing int6's ? */

static INTERRUPT_GEN( dblaxle_interrupt )
{
	// Unsure how many int6's per frame, copy SCI for now
	dblaxle_int6 = !dblaxle_int6;

	if (dblaxle_int6)
		timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_interrupt6);

	cpu_set_irq_line(0, 4, HOLD_LINE);
}

static INTERRUPT_GEN( dblaxle_cpub_interrupt )
{
	// Unsure how many int6's per frame
	timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_interrupt6);
	cpu_set_irq_line(2, 4, HOLD_LINE);
}


/******************************************************************
                              EEPROM
******************************************************************/

static data8_t default_eeprom[128]=
{
	0x00,0x00,0x00,0xff,0x00,0x01,0x41,0x41,0x00,0x00,0x00,0xff,0x00,0x00,0xf0,0xf0,
	0x00,0x00,0x00,0xff,0x00,0x01,0x41,0x41,0x00,0x00,0x00,0xff,0x00,0x00,0xf0,0xf0,
	0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x01,0x40,0x00,0x00,0x00,0xf0,0x00,
	0x00,0x01,0x42,0x85,0x00,0x00,0xf1,0xe3,0x00,0x01,0x40,0x00,0x00,0x00,0xf0,0x00,
	0x00,0x01,0x42,0x85,0x00,0x00,0xf1,0xe3,0xcc,0xcb,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* lock command */
	"0100111111" 	/* unlock command */
};

static NVRAM_HANDLER( spacegun )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(default_eeprom,128);  /* Default the gun setup values */
	}
}

static int eeprom_r(void)
{
	return (EEPROM_read_bit() & 0x01)<<7;
}

#if 0
static READ16_HANDLER( eep_latch_r )
{
	return eep_latch;
}
#endif

static WRITE16_HANDLER( spacegun_output_bypass_w )
{
	switch (offset)
	{
		case 0x03:

/*			0000xxxx	(unused)
			000x0000	eeprom reset (active low)
			00x00000	eeprom clock
			0x000000	eeprom data
			x0000000	(unused)                  */

			COMBINE_DATA(&eep_latch);
			EEPROM_write_bit(data & 0x40);
			EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
			EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
			break;

		default:
			TC0220IOC_w( offset,data );	/* might be a 510NIO ! */
	}
}


/**********************************************************
                       GAME INPUTS
**********************************************************/

static READ16_HANDLER( contcirc_input_bypass_r )
{
	/* Bypass TC0220IOC controller for analog input */

	data8_t port = TC0220IOC_port_r(0);	/* read port number */
	int steer = 0;
	int fake = input_port_6_word_r(0,0);

	if (!(fake &0x10))	/* Analogue steer (the real control method) */
	{
		/* center around zero and reduce span to 0xc0 */
		steer = ((input_port_5_word_r(0,0) - 0x80) * 0xc0) / 0x100;

	}
	else	/* Digital steer */
	{
		if (fake &0x4)
		{
			steer = 0x60;
		}
		else if (fake &0x8)
		{
			steer = 0xff9f;
		}
	}

	switch (port)
	{
		case 0x08:
			return steer &0xff;

		case 0x09:
			return steer >> 8;

		default:
			return TC0220IOC_portreg_r( offset );
	}
}


static READ16_HANDLER( chasehq_input_bypass_r )
{
	/* Bypass TC0220IOC controller for extra inputs */

	data8_t port = TC0220IOC_port_r(0);	/* read port number */
	int steer = 0;
	int fake = input_port_10_word_r(0,0);

	if (!(fake &0x10))	/* Analogue steer (the real control method) */
	{
		/* center around zero */
		steer = input_port_9_word_r(0,0) - 0x80;
	}
	else	/* Digital steer */
	{
		if (fake &0x4)
		{
			steer = 0xff80;
		}
		else if (fake &0x8)
		{
			steer = 0x7f;
		}
	}

	switch (port)
	{
		case 0x08:
			return input_port_5_word_r(0,mem_mask);

		case 0x09:
			return input_port_6_word_r(0,mem_mask);

		case 0x0a:
			return input_port_7_word_r(0,mem_mask);

		case 0x0b:
			return input_port_8_word_r(0,mem_mask);

		case 0x0c:
			return steer &0xff;

		case 0x0d:
			return steer >> 8;

		default:
			return TC0220IOC_portreg_r( offset );
	}
}


static READ16_HANDLER( bshark_stick_r )
{
	switch (offset)
	{
		case 0x00:
			return input_port_5_word_r(0,mem_mask);

		case 0x01:
			return input_port_6_word_r(0,mem_mask);

		case 0x02:
			return input_port_7_word_r(0,mem_mask);

		case 0x03:
			return input_port_8_word_r(0,mem_mask);
	}

logerror("CPU #0 PC %06x: warning - read unmapped stick offset %06x\n",activecpu_get_pc(),offset);

	return 0xff;
}

static data8_t nightstr_stick[128]=
{
	0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
	0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
	0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
	0xe8,0x00,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,
	0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,
	0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,
	0x46,0x47,0x48,0x49,0xb8
};

static READ16_HANDLER( nightstr_stick_r )
{
	switch (offset)
	{
		case 0x00:
			return nightstr_stick[(input_port_5_word_r(0,mem_mask) * 0x64) / 0x100];

		case 0x01:
			return nightstr_stick[(input_port_6_word_r(0,mem_mask) * 0x64) / 0x100];

		case 0x02:
			return input_port_7_word_r(0,mem_mask);

		case 0x03:
			return input_port_8_word_r(0,mem_mask);
	}

logerror("CPU #0 PC %06x: warning - read unmapped stick offset %06x\n",activecpu_get_pc(),offset);

	return 0xff;
}

static WRITE16_HANDLER( bshark_stick_w )
{
	/* Each write invites a new interrupt as soon as the
	   hardware has got the next a/d conversion ready. We set a token
	   delay of 10000 cycles; our "coords" are always ready
	   but we don't want CPUA to have an int6 before int4 is over (?)
	*/

	timer_set(TIME_IN_CYCLES(10000,0),0, taitoz_interrupt6);
}


static READ16_HANDLER( sci_steer_input_r )
{
	int steer = 0;
	int fake = input_port_6_word_r(0,0);

	if (!(fake &0x10))	/* Analogue steer (the real control method) */
	{
		/* center around zero and reduce span to 0xc0 */
		steer = ((input_port_5_word_r(0,0) - 0x80) * 0xc0) / 0x100;
	}
	else	/* Digital steer */
	{
		if (fake &0x4)
		{
			steer = 0xffa0;
		}
		else if (fake &0x8)
		{
			steer = 0x5f;
		}
	}

	switch (offset)
	{
		case 0x04:
			return (steer & 0xff);

 		case 0x05:
			return (steer & 0xff00) >> 8;
	}

logerror("CPU #0 PC %06x: warning - read unmapped steer input offset %06x\n",activecpu_get_pc(),offset);

	return 0xff;
}


static READ16_HANDLER( spacegun_input_bypass_r )
{
	switch (offset)
	{
		case 0x03:
			return eeprom_r();

		default:
			return TC0220IOC_r( offset );	/* might be a 510NIO ! */
	}
}

static READ16_HANDLER( spacegun_lightgun_r )
{
	switch (offset)
	{
		case 0x00:
			return input_port_5_word_r(0,mem_mask);	/* P1X */

		case 0x01:
			return input_port_6_word_r(0,mem_mask);	/* P1Y */

		case 0x02:
			return input_port_7_word_r(0,mem_mask);	/* P2X */

		case 0x03:
			return input_port_8_word_r(0,mem_mask);	/* P2Y */
	}

	return 0x0;
}

static WRITE16_HANDLER( spacegun_lightgun_w )
{
	/* Each write invites a new lightgun interrupt as soon as the
	   hardware has got the next coordinate ready. We set a token
	   delay of 10000 cycles; our "lightgun" coords are always ready
	   but we don't want CPUB to have an int5 before int4 is over (?).

	   Four lightgun interrupts happen before the collected coords
	   are moved to shared ram where CPUA can use them. */

	timer_set(TIME_IN_CYCLES(10000,0),0, taitoz_sg_cpub_interrupt5);
}


static READ16_HANDLER( dblaxle_steer_input_r )
{
	int steer = 0;
	int fake = input_port_6_word_r(0,0);

	if (!(fake &0x10))	/* Analogue steer (the real control method) */
	{
		/* center around zero and reduce span to 0x80 */
		steer = ((input_port_5_word_r(0,0) - 0x80) * 0x80) / 0x100;
	}
	else	/* Digital steer */
	{
		if (fake &0x4)
		{
			steer = 0xffc0;
		}
		else if (fake &0x8)
		{
			steer = 0x3f;
		}
	}

	switch (offset)
	{
		case 0x04:
			return steer >> 8;

		case 0x05:
			return steer &0xff;
	}

logerror("CPU #0 PC %06x: warning - read unmapped steer input offset %02x\n",activecpu_get_pc(),offset);
	return 0x00;
}


static READ16_HANDLER( chasehq_motor_r )
{
	switch (offset)
	{
		case 0x0:
			return (rand() &0xff);	/* motor status ?? */

		case 0x101:
			return 0x55;	/* motor cpu status ? */

		default:
logerror("CPU #0 PC %06x: warning - read motor cpu %03x\n",activecpu_get_pc(),offset);
			return 0;
	}
}

static WRITE16_HANDLER( chasehq_motor_w )
{
	/* Writes $e00000-25 and $e00200-219 */

logerror("CPU #0 PC %06x: warning - write %04x to motor cpu %03x\n",activecpu_get_pc(),data,offset);

}

static READ16_HANDLER( aquajack_unknown_r )
{
	return 0xff;
}


/*****************************************************
                        SOUND
*****************************************************/

static int banknum = -1;

static void reset_sound_region(void)	/* assumes Z80 sandwiched between 68Ks */
{
	cpu_setbank( 10, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000 );
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 7;
	reset_sound_region();
}

static WRITE16_HANDLER( taitoz_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0, data & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0, data & 0xff);

#ifdef MAME_DEBUG
//	if (data & 0xff00)
//	{
//		char buf[80];
//
//		sprintf(buf,"taitoz_sound_w to high byte: %04x",data);
//		usrintf_showmessage(buf);
//	}
#endif
}

static READ16_HANDLER( taitoz_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff));
	else return 0;
}

#if 0
static WRITE16_HANDLER( taitoz_msb_sound_w )
{
	if (offset == 0)
		taitosound_port_w (0,(data >> 8) & 0xff);
	else if (offset == 1)
		taitosound_comm_w (0,(data >> 8) & 0xff);

#ifdef MAME_DEBUG
	if (data & 0xff)
	{
		char buf[80];

		sprintf(buf,"taitoz_msb_sound_w to low byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

static READ16_HANDLER( taitoz_msb_sound_r )
{
	if (offset == 1)
		return ((taitosound_comm_r (0) & 0xff) << 8);
	else return 0;
}
#endif

/***********************************************************
                   MEMORY STRUCTURES
***********************************************************/


static MEMORY_READ16_START( contcirc_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },	/* main CPUA ram */
	{ 0x084000, 0x087fff, sharedram_r },
	{ 0x100000, 0x100007, TC0110PCR_word_r },	/* palette */
	{ 0x200000, 0x20ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_r },
	{ 0x300000, 0x301fff, TC0150ROD_word_r },	/* "root ram" */
	{ 0x400000, 0x4006ff, MRA16_RAM },	/* spriteram */
MEMORY_END

static MEMORY_WRITE16_START( contcirc_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM },
	{ 0x084000, 0x087fff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
//	{ 0x090000, 0x090001, MWA16_NOP },	/* ??? */
	{ 0x100000, 0x100007, TC0110PCR_step1_rbswap_word_w },	/* palette */
	{ 0x200000, 0x20ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_w },
	{ 0x300000, 0x301fff, TC0150ROD_word_w },	/* "root ram" */
	{ 0x400000, 0x4006ff, MWA16_RAM, &spriteram16, &spriteram_size },
MEMORY_END

static MEMORY_READ16_START( contcirc_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },
	{ 0x084000, 0x087fff, sharedram_r },
	{ 0x100000, 0x100001, contcirc_input_bypass_r },
	{ 0x100002, 0x100003, TC0220IOC_halfword_port_r },	/* (actually game uses TC040IOC) */
	{ 0x200000, 0x200003, taitoz_sound_r },
MEMORY_END

static MEMORY_WRITE16_START( contcirc_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM },
	{ 0x084000, 0x087fff, sharedram_w, &taitoz_sharedram },
	{ 0x100000, 0x100001, TC0220IOC_halfword_portreg_w },
	{ 0x100002, 0x100003, TC0220IOC_halfword_port_w },
	{ 0x200000, 0x200003, taitoz_sound_w },
MEMORY_END


static MEMORY_READ16_START( chasehq_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x107fff, MRA16_RAM },	/* main CPUA ram */
	{ 0x108000, 0x10bfff, sharedram_r },
	{ 0x10c000, 0x10ffff, MRA16_RAM },	/* extra CPUA ram */
	{ 0x400000, 0x400001, chasehq_input_bypass_r },
	{ 0x400002, 0x400003, TC0220IOC_halfword_port_r },
	{ 0x820000, 0x820003, taitoz_sound_r },
	{ 0xa00000, 0xa00007, TC0110PCR_word_r },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xd00000, 0xd007ff, MRA16_RAM },	/* spriteram */
	{ 0xe00000, 0xe003ff, chasehq_motor_r },	/* motor cpu */
MEMORY_END

static MEMORY_WRITE16_START( chasehq_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x107fff, MWA16_RAM },
	{ 0x108000, 0x10bfff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
	{ 0x10c000, 0x10ffff, MWA16_RAM },
	{ 0x400000, 0x400001, TC0220IOC_halfword_portreg_w },
	{ 0x400002, 0x400003, TC0220IOC_halfword_port_w },
	{ 0x800000, 0x800001, cpua_ctrl_w },
	{ 0x820000, 0x820003, taitoz_sound_w },
	{ 0xa00000, 0xa00007, TC0110PCR_step1_word_w },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xd00000, 0xd007ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xe00000, 0xe003ff, chasehq_motor_w },	/* motor cpu */
MEMORY_END

static MEMORY_READ16_START( chq_cpub_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x108000, 0x10bfff, sharedram_r },
	{ 0x800000, 0x801fff, TC0150ROD_word_r },
MEMORY_END

static MEMORY_WRITE16_START( chq_cpub_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x108000, 0x10bfff, sharedram_w, &taitoz_sharedram },
	{ 0x800000, 0x801fff, TC0150ROD_word_w },
MEMORY_END


static MEMORY_READ16_START( enforce_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },	/* main CPUA ram */
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x300000, 0x3006ff, MRA16_RAM },	/* spriteram */
	{ 0x400000, 0x401fff, TC0150ROD_word_r },	/* "root ram" ??? */
	{ 0x500000, 0x500007, TC0110PCR_word_r },	/* palette */
	{ 0x600000, 0x60ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x620000, 0x62000f, TC0100SCN_ctrl_word_0_r },
MEMORY_END

static MEMORY_WRITE16_START( enforce_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x104000, 0x107fff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
	{ 0x200000, 0x200001, cpua_ctrl_w },	// works without?
	{ 0x300000, 0x3006ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x400000, 0x401fff, TC0150ROD_word_w },	/* "root ram" ??? */
	{ 0x500000, 0x500007, TC0110PCR_step1_rbswap_word_w },	/* palette */
	{ 0x600000, 0x60ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x620000, 0x62000f, TC0100SCN_ctrl_word_0_w },
MEMORY_END

static MEMORY_READ16_START( enforce_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x200000, 0x200003, taitoz_sound_r },
	{ 0x300000, 0x300001, TC0220IOC_halfword_portreg_r },
	{ 0x300002, 0x300003, TC0220IOC_halfword_port_r },	/* (actually game uses TC040IOC ?) */
MEMORY_END

static MEMORY_WRITE16_START( enforce_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x104000, 0x107fff, sharedram_w, &taitoz_sharedram },
	{ 0x200000, 0x200003, taitoz_sound_w },
	{ 0x300000, 0x300001, TC0220IOC_halfword_portreg_w },
	{ 0x300002, 0x300003, TC0220IOC_halfword_port_w },
MEMORY_END


static MEMORY_READ16_START( bshark_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },	/* main CPUA ram */
	{ 0x110000, 0x113fff, sharedram_r },
	{ 0x400000, 0x40000f, TC0220IOC_halfword_r },
	{ 0x800000, 0x800007, bshark_stick_r },
	{ 0xa00000, 0xa01fff, paletteram16_word_r },	/* palette */
	{ 0xc00000, 0xc00fff, MRA16_RAM },	/* spriteram */
	{ 0xd00000, 0xd0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xd20000, 0xd2000f, TC0100SCN_ctrl_word_0_r },
MEMORY_END

static MEMORY_WRITE16_START( bshark_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x110000, 0x113fff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
	{ 0x400000, 0x40000f, TC0220IOC_halfword_w },
	{ 0x600000, 0x600001, cpua_noz80_ctrl_w },
	{ 0x800000, 0x800007, bshark_stick_w },
	{ 0xa00000, 0xa01fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0xc00000, 0xc00fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xd00000, 0xd0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xd20000, 0xd2000f, TC0100SCN_ctrl_word_0_w },
MEMORY_END

static MEMORY_READ16_START( bshark_cpub_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x108000, 0x10bfff, MRA16_RAM },
	{ 0x110000, 0x113fff, sharedram_r },
//	{ 0x40000a, 0x40000b, taitoz_unknown_r },	// ???
	{ 0x600000, 0x600001, YM2610_status_port_0_A_lsb_r },
	{ 0x600002, 0x600003, YM2610_read_port_0_lsb_r },
	{ 0x600004, 0x600005, YM2610_status_port_0_B_lsb_r },
	{ 0x60000c, 0x60000d, MRA16_NOP },
	{ 0x60000e, 0x60000f, MRA16_NOP },
	{ 0x800000, 0x801fff, TC0150ROD_word_r },	/* "root ram" */
MEMORY_END

static MEMORY_WRITE16_START( bshark_cpub_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x108000, 0x10bfff, MWA16_RAM },
	{ 0x110000, 0x113fff, sharedram_w },
//	{ 0x400000, 0x400007, MWA16_NOP },   // pan ???
	{ 0x600000, 0x600001, YM2610_control_port_0_A_lsb_w },
	{ 0x600002, 0x600003, YM2610_data_port_0_A_lsb_w },
	{ 0x600004, 0x600005, YM2610_control_port_0_B_lsb_w },
	{ 0x600006, 0x600007, YM2610_data_port_0_B_lsb_w },
	{ 0x60000c, 0x60000d, MWA16_NOP },	// interrupt controller?
	{ 0x60000e, 0x60000f, MWA16_NOP },
	{ 0x800000, 0x801fff, TC0150ROD_word_w },	/* "root ram" */
MEMORY_END


static MEMORY_READ16_START( sci_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x107fff, MRA16_RAM },	/* main CPUA ram */
	{ 0x108000, 0x10bfff, sharedram_r },	/* extent ?? */
	{ 0x10c000, 0x10ffff, MRA16_RAM },	/* extra CPUA ram */
	{ 0x200000, 0x20000f, TC0220IOC_halfword_r },
	{ 0x200010, 0x20001f, sci_steer_input_r },
	{ 0x420000, 0x420003, taitoz_sound_r },
	{ 0x800000, 0x801fff, paletteram16_word_r },
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xc00000, 0xc03fff, MRA16_RAM },	/* spriteram */	// Raine draws only 0x1000
	{ 0xc08000, 0xc08001, sci_spriteframe_r },	// debugging
MEMORY_END

static MEMORY_WRITE16_START( sci_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x107fff, MWA16_RAM },
	{ 0x108000, 0x10bfff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
	{ 0x10c000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x20000f, TC0220IOC_halfword_w },
//	{ 0x400000, 0x400001, cpua_ctrl_w },	// ?? doesn't seem to fit what's written
	{ 0x420000, 0x420003, taitoz_sound_w },
	{ 0x800000, 0x801fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xc00000, 0xc03fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xc08000, 0xc08001, sci_spriteframe_w },
MEMORY_END

static MEMORY_READ16_START( sci_cpub_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x200000, 0x203fff, MRA16_RAM },
	{ 0x208000, 0x20bfff, sharedram_r },
	{ 0xa00000, 0xa01fff, TC0150ROD_word_r },
MEMORY_END

static MEMORY_WRITE16_START( sci_cpub_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x200000, 0x203fff, MWA16_RAM },
	{ 0x208000, 0x20bfff, sharedram_w, &taitoz_sharedram },
	{ 0xa00000, 0xa01fff, TC0150ROD_word_w },
MEMORY_END


static MEMORY_READ16_START( nightstr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },	/* main CPUA ram */
	{ 0x110000, 0x113fff, sharedram_r },
	{ 0x400000, 0x40000f, TC0220IOC_halfword_r },
	{ 0x820000, 0x820003, taitoz_sound_r },
	{ 0xa00000, 0xa00007, TC0110PCR_word_r },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xd00000, 0xd007ff, MRA16_RAM },	/* spriteram */
	{ 0xe40000, 0xe40007, nightstr_stick_r },
MEMORY_END

static MEMORY_WRITE16_START( nightstr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x110000, 0x113fff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
	{ 0x400000, 0x40000f, TC0220IOC_halfword_w },
	{ 0x800000, 0x800001, cpua_ctrl_w },
	{ 0x820000, 0x820003, taitoz_sound_w },
	{ 0xa00000, 0xa00007, TC0110PCR_step1_word_w },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xd00000, 0xd007ff, MWA16_RAM, &spriteram16, &spriteram_size },
//	{ 0xe00000, 0xe00001, MWA16_NOP },	/* ??? */
//	{ 0xe00008, 0xe00009, MWA16_NOP },	/* ??? */
//	{ 0xe00010, 0xe00011, MWA16_NOP },	/* ??? */
	{ 0xe40000, 0xe40007, bshark_stick_w },
MEMORY_END

static MEMORY_READ16_START( nightstr_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x800000, 0x801fff, TC0150ROD_word_r },	/* "root ram" */
MEMORY_END

static MEMORY_WRITE16_START( nightstr_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x104000, 0x107fff, sharedram_w, &taitoz_sharedram },
	{ 0x800000, 0x801fff, TC0150ROD_word_w },	/* "root ram" */
MEMORY_END


static MEMORY_READ16_START( aquajack_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },	/* main CPUA ram */
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x300000, 0x300007, TC0110PCR_word_r },	/* palette */
	{ 0x800000, 0x801fff, TC0150ROD_word_r },	/* (like Contcirc, uses CPUA for road) */
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xc40000, 0xc403ff, MRA16_RAM },	/* spriteram */
MEMORY_END

static MEMORY_WRITE16_START( aquajack_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x104000, 0x107fff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
	{ 0x200000, 0x200001, cpua_ctrl_w },	// not needed, but it's probably like the others
	{ 0x300000, 0x300007, TC0110PCR_step1_word_w },	/* palette */
	{ 0x800000, 0x801fff, TC0150ROD_word_w },
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xc40000, 0xc403ff, MWA16_RAM, &spriteram16, &spriteram_size },
MEMORY_END

static MEMORY_READ16_START( aquajack_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x200000, 0x20000f, TC0220IOC_halfword_r },
	{ 0x300000, 0x300003, taitoz_sound_r },
	{ 0x800800, 0x80083f, aquajack_unknown_r }, // Read regularly after write to 800800...
//	{ 0x900000, 0x900007, taitoz_unknown_r },
MEMORY_END

static MEMORY_WRITE16_START( aquajack_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x104000, 0x107fff, sharedram_w, &taitoz_sharedram },
	{ 0x200000, 0x20000f, TC0220IOC_halfword_w },
	{ 0x300000, 0x300003, taitoz_sound_w },
//	{ 0x800800, 0x800801, taitoz_unknown_w },
//	{ 0x900000, 0x900007, taitoz_unknown_w },
MEMORY_END


static MEMORY_READ16_START( spacegun_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x30c000, 0x30ffff, MRA16_RAM },	/* local CPUA ram */
	{ 0x310000, 0x31ffff, sharedram_r },	/* extent correct acc. to CPUB inits */
	{ 0x500000, 0x5005ff, MRA16_RAM },	/* spriteram */
	{ 0x900000, 0x90ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x920000, 0x92000f, TC0100SCN_ctrl_word_0_r },
	{ 0xb00000, 0xb00007, TC0110PCR_word_r },	/* palette */
MEMORY_END

static MEMORY_WRITE16_START( spacegun_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x30c000, 0x30ffff, MWA16_RAM },
	{ 0x310000, 0x31ffff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size  },
	{ 0x500000, 0x5005ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x90ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x920000, 0x92000f, TC0100SCN_ctrl_word_0_w },
	{ 0xb00000, 0xb00007, TC0110PCR_step1_rbswap_word_w },	/* palette */
MEMORY_END

static MEMORY_READ16_START( spacegun_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x20c000, 0x20ffff, MRA16_RAM },	/* local CPUB ram */
	{ 0x210000, 0x21ffff, sharedram_r },
	{ 0x800000, 0x80000f, spacegun_input_bypass_r },
	{ 0xc00000, 0xc00001, YM2610_status_port_0_A_lsb_r },
	{ 0xc00002, 0xc00003, YM2610_read_port_0_lsb_r },
	{ 0xc00004, 0xc00005, YM2610_status_port_0_B_lsb_r },
	{ 0xc0000c, 0xc0000d, MRA16_NOP },
	{ 0xc0000e, 0xc0000f, MRA16_NOP },
	{ 0xf00000, 0xf00007, spacegun_lightgun_r },
MEMORY_END

static MEMORY_WRITE16_START( spacegun_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x20c000, 0x20ffff, MWA16_RAM },
	{ 0x210000, 0x21ffff, sharedram_w },
	{ 0x800000, 0x80000f, spacegun_output_bypass_w },
	{ 0xc00000, 0xc00001, YM2610_control_port_0_A_lsb_w },
	{ 0xc00002, 0xc00003, YM2610_data_port_0_A_lsb_w },
	{ 0xc00004, 0xc00005, YM2610_control_port_0_B_lsb_w },
	{ 0xc00006, 0xc00007, YM2610_data_port_0_B_lsb_w },
	{ 0xc0000c, 0xc0000d, MWA16_NOP },	// interrupt controller?
	{ 0xc0000e, 0xc0000f, MWA16_NOP },
//	{ 0xc20000, 0xc20003, YM2610_???? },	/* Pan (acc. to Raine) */
//	{ 0xe00000, 0xe00001, MWA16_NOP },	/* ??? */
	{ 0xf00000, 0xf00007, spacegun_lightgun_w },
MEMORY_END


static MEMORY_READ16_START( dblaxle_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x200000, 0x203fff, MRA16_RAM },	/* main CPUA ram */
	{ 0x210000, 0x21ffff, sharedram_r },
	{ 0x400000, 0x40000f, TC0510NIO_halfword_wordswap_r },
	{ 0x400010, 0x40001f, dblaxle_steer_input_r },
	{ 0x620000, 0x620003, taitoz_sound_r },
	{ 0x800000, 0x801fff, paletteram16_word_r },	/* palette */
	{ 0xa00000, 0xa0ffff, TC0480SCP_word_r },	  /* tilemaps */
	{ 0xa30000, 0xa3002f, TC0480SCP_ctrl_word_r },
	{ 0xc00000, 0xc03fff, MRA16_RAM },	/* spriteram */
	{ 0xc08000, 0xc08001, sci_spriteframe_r },	// debugging
MEMORY_END

static MEMORY_WRITE16_START( dblaxle_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x200000, 0x203fff, MWA16_RAM },
	{ 0x210000, 0x21ffff, sharedram_w, &taitoz_sharedram, &taitoz_sharedram_size },
	{ 0x400000, 0x40000f, TC0510NIO_halfword_wordswap_w },
	{ 0x600000, 0x600001, cpua_ctrl_w },	/* could this be causing int6 ? */
	{ 0x620000, 0x620003, taitoz_sound_w },
	{ 0x800000, 0x801fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x900000, 0x90ffff, TC0480SCP_word_w },	  /* tilemap mirror */
	{ 0xa00000, 0xa0ffff, TC0480SCP_word_w },	  /* tilemaps */
	{ 0xa30000, 0xa3002f, TC0480SCP_ctrl_word_w },
	{ 0xc00000, 0xc03fff, MWA16_RAM, &spriteram16, &spriteram_size }, /* mostly unused ? */
	{ 0xc08000, 0xc08001, sci_spriteframe_w },	/* set in int6, seems to stay zero */
MEMORY_END

static MEMORY_READ16_START( dblaxle_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x110000, 0x11ffff, sharedram_r },
	{ 0x300000, 0x301fff, TC0150ROD_word_r },
	{ 0x500000, 0x503fff, MRA16_RAM },	/* network ram ? (see Gunbustr) */
MEMORY_END

static MEMORY_WRITE16_START( dblaxle_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x110000, 0x11ffff, sharedram_w, &taitoz_sharedram },
	{ 0x300000, 0x301fff, TC0150ROD_word_w },
	{ 0x500000, 0x503fff, MWA16_RAM },	/* network ram ? (see Gunbustr) */
MEMORY_END


/***************************************************************************/

static MEMORY_READ_START( z80_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( z80_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END


/***********************************************************
                   INPUT PORTS, DIPs
***********************************************************/

#define TAITO_Z_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_Z_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_Z_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

#define TAITO_Z_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Normal" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )


INPUT_PORTS_START( contcirc )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, "Cockpit" )	// analogue accelerator pedal
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_WORLD_8

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty 1 (time/speed)" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty 2 (other cars)" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Steering wheel" )	//not sure what effect this has
	PORT_DIPSETTING(    0x10, "Free" )
	PORT_DIPSETTING(    0x00, "Locked" )
	PORT_DIPNAME( 0x20, 0x00, "Enable 3d alternate frames" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* main accel key */

	PORT_START      /* IN1: b3 not mapped: standardized on holding b4=lo gear */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear shift lo/hi */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* main brake key */

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, "handle" i.e. steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 50, 15, 0x00, 0xff)

	PORT_START      /* IN4, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( contcrcu )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, "Cockpit" )	// analogue accelerator pedal
	PORT_DIPNAME( 0x02, 0x02, "Discounted continues" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_JAPAN_8		// confirmed

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty 1 (time/speed)" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty 2 (other cars)" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Steering wheel" )	//not sure what effect this has
	PORT_DIPSETTING(    0x10, "Free" )
	PORT_DIPSETTING(    0x00, "Locked" )
	PORT_DIPNAME( 0x20, 0x00, "Enable 3d alternate frames" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )	//acc. to manual
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )	//acc. to manual
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* main accel key */

	PORT_START      /* IN1: b3 not mapped: standardized on holding b4=lo gear */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear shift lo/hi */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* main brake key */

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, "handle" i.e. steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 50, 15, 0x00, 0xff)

	PORT_START      /* IN4, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( chasehq )	// IN3-6 perhaps used with cockpit setup? //
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Upright / Steering Lock" )
	PORT_DIPSETTING(    0x02, "Upright / No Steering Lock" )
	PORT_DIPSETTING(    0x01, "Full Throttle Convert, Cockpit" )
	PORT_DIPSETTING(    0x00, "Full Throttle Convert, Deluxe" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Setting" )
	PORT_DIPSETTING(    0x08, "70 Seconds" )
	PORT_DIPSETTING(    0x04, "65 Seconds" )
	PORT_DIPSETTING(    0x0c, "60 Seconds" )
	PORT_DIPSETTING(    0x00, "55 Seconds" )
	PORT_DIPNAME( 0x10, 0x10, "Turbos Stocked" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Damage Cleared at Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	/* brake */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* turbo */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* accel */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN5, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN6, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN7, steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 50, 15, 0x00, 0xff )

	PORT_START      /* IN8, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( chasehqj )	// IN3-6 perhaps used with cockpit setup? //
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Upright / Steering Lock" )
	PORT_DIPSETTING(    0x02, "Upright / No Steering Lock" )
	PORT_DIPSETTING(    0x01, "Full Throttle Convert, Cockpit" )
	PORT_DIPSETTING(    0x00, "Full Throttle Convert, Deluxe" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Setting" )
	PORT_DIPSETTING(    0x08, "70 Seconds" )
	PORT_DIPSETTING(    0x04, "65 Seconds" )
	PORT_DIPSETTING(    0x0c, "60 Seconds" )
	PORT_DIPSETTING(    0x00, "55 Seconds" )
	PORT_DIPNAME( 0x10, 0x10, "Turbos Stocked" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Damage Cleared at Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	/* brake */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* turbo */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* accel */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN5, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN6, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN7, steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 50, 15, 0x00, 0xff )

	PORT_START      /* IN8, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( enforce )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )	// Says SHIFT HI in test mode !?
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )		// Says SHIFT LO in test mode !?
	TAITO_Z_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Background scenery" )
	PORT_DIPSETTING(    0x10, "Crazy scrolling" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	/* Bomb */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* Laser */
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
INPUT_PORTS_END

INPUT_PORTS_START( bshark )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Mirror screen" )	// manual says first two must be off
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_US_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x04, "Speed of Sight" )
	PORT_DIPSETTING(    0x0c, "Slow" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x04, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	// manual says all these must be off
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1, unused */

	PORT_START      /* IN2, b2-5 affect sound num in service mode but otherwise useless (?) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Fire" */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )	/* same as "Fire" */

	PORT_START	/* values chosen to match allowed crosshair area */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 20, 4, 0xcc, 0x35)

	PORT_START	/* "X adjust" */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* values chosen to match allowed crosshair area */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 20, 4, 0xd5, 0x32)

	PORT_START	/* "Y adjust" */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( bsharkj )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Mirror screen" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x04, "Speed of Sight" )
	PORT_DIPSETTING(    0x0c, "Slow" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x04, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1, unused */

	PORT_START      /* IN2, b2-5 affect sound num in service mode but otherwise useless (?) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Fire" */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )	/* same as "Fire" */

	PORT_START	/* values chosen to match allowed crosshair area */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 20, 4, 0xcc, 0x35)

	PORT_START	/* "X adjust" */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* values chosen to match allowed crosshair area */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 20, 4, 0xd5, 0x32)

	PORT_START	/* "Y adjust" */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( sci )	// dsws may be slightly wrong
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, "Cockpit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Setting" )
	PORT_DIPSETTING(    0x08, "70 Seconds" )
	PORT_DIPSETTING(    0x04, "65 Seconds" )
	PORT_DIPSETTING(    0x0c, "60 Seconds" )
	PORT_DIPSETTING(    0x00, "55 Seconds" )
	PORT_DIPNAME( 0x10, 0x10, "Turbos Stocked" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, "Respond to Controls" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Damage Cleared at Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Siren Volume" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Low" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* fire */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* brake */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON5 | IPF_PLAYER1 )	/* turbo */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON6 | IPF_PLAYER1 )	/* "center" */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* accel */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 50, 15, 0x00, 0xff )

	PORT_START      /* IN4, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( sciu )	// dsws may be slightly wrong
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, "Cockpit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_US_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Setting" )
	PORT_DIPSETTING(    0x08, "70 Seconds" )
	PORT_DIPSETTING(    0x04, "65 Seconds" )
	PORT_DIPSETTING(    0x0c, "60 Seconds" )
	PORT_DIPSETTING(    0x00, "55 Seconds" )
	PORT_DIPNAME( 0x10, 0x10, "Turbos Stocked" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, "Respond to Controls" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Damage Cleared at Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Siren Volume" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Low" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* fire */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* brake */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON5 | IPF_PLAYER1 )	/* turbo */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON6 | IPF_PLAYER1 )	/* "center" */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* accel */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 50, 15, 0x00, 0xff )

	PORT_START      /* IN4, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( nightstr )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_US_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Shields" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, "Shields" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START      /* IN1, unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 60, 15, 0x00, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER1, 60, 15, 0x00, 0xff)

	PORT_START	/* X offset */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Y offset */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( aquajack )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, "Cockpit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30k" )
	PORT_DIPSETTING(    0x30, "50k" )
	PORT_DIPSETTING(    0x10, "80k" )
	PORT_DIPSETTING(    0x20, "100k" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) ) /* Dips 7 & 8 shown as "Do Not Touch" in manual */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2, what is it ??? */
	PORT_ANALOG( 0xff, 0x80, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( aquajckj )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, "Cockpit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30k" )
	PORT_DIPSETTING(    0x30, "50k" )
	PORT_DIPSETTING(    0x10, "80k" )
	PORT_DIPSETTING(    0x20, "100k" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) ) /* Dips 7 & 8 shown as "Do Not Touch" in manual */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2, what is it ??? */
	PORT_ANALOG( 0xff, 0x80, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( spacegun )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )	// Manual says Always Off
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Always have gunsight power up" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )	// Manual lists dips 3 through 6 and 8 as Always off
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Disable Pedal (?)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN1, unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER1, 20, 22, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 20, 22, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER2, 20, 22, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 20, 22, 0, 0xff)
INPUT_PORTS_END

INPUT_PORTS_START( dblaxle )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Gear shift" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Inverted" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_US_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x00, "Multi-machine hookup ?" )	// doesn't boot if on
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Player Truck" )
	PORT_DIPSETTING(    0x08, "Red" )
	PORT_DIPSETTING(    0x00, "Blue" )
	PORT_DIPNAME( 0x10, 0x10, "Back button" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Inverted" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	// causes "Root CPU Error" on "Icy Road" (Tourniquet)
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	/* shift */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* brake */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )	/* "back" */

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* nitro */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )	/* "center" */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* accel */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 40, 10, 0x00, 0xff )

	PORT_START      /* IN4, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( pwheelsj )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Gear shift" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Inverted" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_Z_COINAGE_JAPAN_8

	PORT_START /* DSW B */
	TAITO_Z_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x00, "Multi-machine hookup ?" )	// doesn't boot if on
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Player Truck" )
	PORT_DIPSETTING(    0x08, "Red" )
	PORT_DIPSETTING(    0x00, "Blue" )
	PORT_DIPNAME( 0x10, 0x10, "Back button" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Inverted" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	// causes "Root CPU Error" on "Icy Road" (Tourniquet)
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	/* shift */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* brake */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )	/* "back" */

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* nitro */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )	/* "center" */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* accel */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2, unused */

	PORT_START      /* IN3, steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 40, 10, 0x00, 0xff )

	PORT_START      /* IN4, fake allowing digital steer */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END


/***********************************************************
                       GFX DECODING
***********************************************************/

static struct GfxLayout tile16x8_layout =
{
	16,8,	/* 16*8 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout tile16x16_layout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout dblaxle_charlayout =
{
	16,16,    /* 16*16 characters */
	RGN_FRAC(1,1),
	4,        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 5*4, 4*4, 3*4, 2*4, 7*4, 6*4, 9*4, 8*4, 13*4, 12*4, 11*4, 10*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo taitoz_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },		/* sprites & playfield */
	{ -1 } /* end of array */
};

/* taitoic.c TC0100SCN routines expect scr stuff to be in second gfx
   slot, so 2nd batch of obj must be placed third */

static struct GfxDecodeInfo chasehq_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x16_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },		/* sprites & playfield */
	{ REGION_GFX4, 0x0, &tile16x16_layout,  0, 256 },	/* sprite parts */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo dblaxle_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &dblaxle_charlayout,  0, 256 },	/* sprites & playfield */
	{ -1 } /* end of array */
};



/**************************************************************
                         YM2610 (SOUND)

The first interface is for game boards with twin 68000 and Z80.
Interface B is for games which lack a Z80 (Spacegun, Bshark).
**************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)	// assumes Z80 sandwiched between 68Ks
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandlerb(int irq)
{
	// DG: this is probably specific to Z80 and wrong?
//	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz ?? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static struct YM2610interface ym2610_interfaceb =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz ?? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandlerb },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};


/**************************************************************
                         SUBWOOFER (SOUND)
**************************************************************/

static int subwoofer_sh_start(const struct MachineSound *msound)
{
	/* Adjust the lowpass filter of the first three YM2610 channels */

	/* 150 Hz is a common top frequency played by a generic */
	/* subwoofer, the real Arcade Machine may differs */

	mixer_set_lowpass_frequency(0,20);
	mixer_set_lowpass_frequency(1,20);
	mixer_set_lowpass_frequency(2,20);

	return 0;
}

static struct CustomSound_interface subwoofer_interface =
{
	subwoofer_sh_start,
	0, /* none */
	0 /* none */
};


/***********************************************************
                      MACHINE DRIVERS

CPU Interleaving
----------------

Chasehq2 needs high interleaving to have sound (not checked
since May 2001 - may have changed).

Enforce with interleave of 1 sometimes lets you take over from
the demo game when you coin up! Set to 10 seems to cure this.

Bshark needs the high cpu interleaving to run test mode.

Nightstr needs the high cpu interleaving to get through init.

Aquajack has it VERY high to cure frequent sound-related
hangs.

Dblaxle has 10 to boot up reliably but very occasionally gets
a "root cpu error" still.

Mostly it's the 2nd 68K which writes to road chip, so syncing
between it and the master 68K may be important. Contcirc
and ChaseHQ have interleave of only 1 - possible cause of
Contcirc road glitchiness in attract?

***********************************************************/

/* Contcirc vis area seems narrower than the other games... */

static MACHINE_DRIVER_START( contcirc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(contcirc_readmem,contcirc_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(contcirc_cpub_readmem,contcirc_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 3*8, 31*8-1)
	MDRV_GFXDECODE(taitoz_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(contcirc)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
	MDRV_SOUND_ADD(CUSTOM, subwoofer_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( chasehq )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(chasehq_readmem,chasehq_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(chq_cpub_readmem,chq_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(chasehq_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(chasehq)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( enforce )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(enforce_readmem,enforce_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(enforce_cpub_readmem,enforce_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 31*8-1)
	MDRV_GFXDECODE(taitoz_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(contcirc)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
	MDRV_SOUND_ADD(CUSTOM, subwoofer_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bshark )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(bshark_readmem,bshark_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(bshark_cpub_readmem,bshark_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(taitoz_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(bshark)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interfaceb)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sci )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(sci_readmem,sci_writemem)
	MDRV_CPU_VBLANK_INT(sci_interrupt,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(sci_cpub_readmem,sci_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(50)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(taitoz_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(sci)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nightstr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(nightstr_readmem,nightstr_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(nightstr_cpub_readmem,nightstr_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(chasehq_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(chasehq)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( aquajack )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(aquajack_readmem,aquajack_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(aquajack_cpub_readmem,aquajack_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(500)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(taitoz_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(aquajack)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spacegun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz ??? */
	MDRV_CPU_MEMORY(spacegun_readmem,spacegun_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz ??? */
	MDRV_CPU_MEMORY(spacegun_cpub_readmem,spacegun_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(spacegun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(taitoz_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(spacegun)
	MDRV_VIDEO_UPDATE(spacegun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interfaceb)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dblaxle )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz ??? */
	MDRV_CPU_MEMORY(dblaxle_readmem,dblaxle_writemem)
	MDRV_CPU_VBLANK_INT(dblaxle_interrupt,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_sound_readmem,z80_sound_writemem)

	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz ??? */
	MDRV_CPU_MEMORY(dblaxle_cpub_readmem,dblaxle_cpub_writemem)
	MDRV_CPU_VBLANK_INT(dblaxle_cpub_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(dblaxle_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(taitoz)
	MDRV_VIDEO_UPDATE(dblaxle)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END


/***************************************************************************
                                 DRIVERS

Contcirc, Dblaxle sound sample rom order is uncertain as sound imperfect
***************************************************************************/

ROM_START( contcirc )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "ic25",      0x00000, 0x20000, 0xf5c92e42 )
	ROM_LOAD16_BYTE( "cc_26.bin", 0x00001, 0x20000, 0x1345ebe6 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "ic35",      0x00000, 0x20000, 0x16522f2d )
	ROM_LOAD16_BYTE( "cc_36.bin", 0x00001, 0x20000, 0xa1732ea5 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b33-30",   0x00000, 0x04000, 0xd8746234 )
	ROM_CONTINUE(         0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b33-02", 0x00000, 0x80000, 0xf6fb3ba2 )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b33-06", 0x000000, 0x080000, 0x2cb40599 )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "b33-05", 0x000001, 0x080000, 0xbddf9eea )
	ROM_LOAD32_BYTE( "b33-04", 0x000002, 0x080000, 0x8df866a2 )
	ROM_LOAD32_BYTE( "b33-03", 0x000003, 0x080000, 0x4f6c36d9 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b33-01", 0x00000, 0x80000, 0xf11f2be8 )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b33-07", 0x00000, 0x80000, 0x151e1f52 )	/* STY spritemap */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b33-09", 0x00000, 0x80000, 0x1e6724b5 )
	ROM_LOAD( "b33-10", 0x80000, 0x80000, 0xe9ce03ab )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b33-08", 0x00000, 0x80000, 0xcaa1c4c8 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "b14-30", 0x00000, 0x10000, 0xdccb0c7f )	/* unused roms */
	ROM_LOAD( "b14-31", 0x00000, 0x02000, 0x5c6b013d )
ROM_END

ROM_START( contcrcu )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "ic25", 0x00000, 0x20000, 0xf5c92e42 )
	ROM_LOAD16_BYTE( "ic26", 0x00001, 0x20000, 0xe7c1d1fa )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "ic35", 0x00000, 0x20000, 0x16522f2d )
	ROM_LOAD16_BYTE( "ic36", 0x00001, 0x20000, 0xd6741e33 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b33-30",   0x00000, 0x04000, 0xd8746234 )
	ROM_CONTINUE(         0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b33-02", 0x00000, 0x80000, 0xf6fb3ba2 )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b33-06", 0x000000, 0x080000, 0x2cb40599 )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "b33-05", 0x000001, 0x080000, 0xbddf9eea )
	ROM_LOAD32_BYTE( "b33-04", 0x000002, 0x080000, 0x8df866a2 )
	ROM_LOAD32_BYTE( "b33-03", 0x000003, 0x080000, 0x4f6c36d9 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b33-01", 0x00000, 0x80000, 0xf11f2be8 )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b33-07", 0x00000, 0x80000, 0x151e1f52 )	/* STY spritemap */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b33-09", 0x00000, 0x80000, 0x1e6724b5 )
	ROM_LOAD( "b33-10", 0x80000, 0x80000, 0xe9ce03ab )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b33-08", 0x00000, 0x80000, 0xcaa1c4c8 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "b14-30", 0x00000, 0x10000, 0xdccb0c7f )	/* unused roms */
	ROM_LOAD( "b14-31", 0x00000, 0x02000, 0x5c6b013d )
ROM_END

ROM_START( chasehq )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b52-130.36", 0x00000, 0x20000, 0x4e7beb46 )
	ROM_LOAD16_BYTE( "b52-136.29", 0x00001, 0x20000, 0x2f414df0 )
	ROM_LOAD16_BYTE( "b52-131.37", 0x40000, 0x20000, 0xaa945d83 )
	ROM_LOAD16_BYTE( "b52-129.30", 0x40001, 0x20000, 0x0eaebc08 )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b52-132.39", 0x00000, 0x10000, 0xa2f54789 )
	ROM_LOAD16_BYTE( "b52-133.55", 0x00001, 0x10000, 0x12232f95 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b52-137.51",   0x00000, 0x04000, 0x37abb74a )
	ROM_CONTINUE(             0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b52-29.27", 0x00000, 0x80000, 0x8366d27c )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b52-34.5",  0x000000, 0x080000, 0x7d8dce36 )
	ROM_LOAD32_BYTE( "b52-35.7",  0x000001, 0x080000, 0x78eeec0d )	/* OBJ A 16x16 */
	ROM_LOAD32_BYTE( "b52-36.9",  0x000002, 0x080000, 0x61e89e91 )
	ROM_LOAD32_BYTE( "b52-37.11", 0x000003, 0x080000, 0xf02e47b9 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b52-28.4", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b52-30.4",  0x000000, 0x080000, 0x1b8cc647 )
	ROM_LOAD32_BYTE( "b52-31.6",  0x000001, 0x080000, 0xf1998e20 )	/* OBJ B 16x16 */
	ROM_LOAD32_BYTE( "b52-32.8",  0x000002, 0x080000, 0x8620780c )
	ROM_LOAD32_BYTE( "b52-33.10", 0x000003, 0x080000, 0xe6f4b8c4 )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b52-38.34", 0x00000, 0x80000, 0x5b5bf7f6 )	/* STY spritemap */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b52-115.71", 0x000000, 0x080000, 0x4e117e93 )
	ROM_LOAD( "b52-114.72", 0x080000, 0x080000, 0x3a73d6b1 )
	ROM_LOAD( "b52-113.73", 0x100000, 0x080000, 0x2c6a3a05 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b52-116.70", 0x00000, 0x80000, 0xad46983c )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "b52-01.7",    0x00000, 0x00100, 0x89719d17 )	/* unused roms */
	ROM_LOAD( "b52-03.135",  0x00000, 0x00400, 0xa3f8490d )
	ROM_LOAD( "b52-06.24",   0x00000, 0x00100, 0xfbf81f30 )
	ROM_LOAD( "b52-18.93",   0x00000, 0x00100, 0x60bdaf1a )	// identical to b52-18b
	ROM_LOAD( "b52-18a",     0x00000, 0x00100, 0x6271be0d )
	ROM_LOAD( "b52-49.68",   0x00000, 0x02000, 0x60dd2ed1 )
	ROM_LOAD( "b52-50.66",   0x00000, 0x10000, 0xc189781c )
	ROM_LOAD( "b52-51.65",   0x00000, 0x10000, 0x30cc1f79 )
	ROM_LOAD( "b52-126.136", 0x00000, 0x00400, 0xfa2f840e )
	ROM_LOAD( "b52-127.156", 0x00000, 0x00400, 0x77682a4f )

	// Various pals are listed in Malcor's notes: b52-118 thru 125,
	// b52-16 thru 21, b52-25 thru 27
ROM_END

ROM_START( chasehqj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b52-140.36", 0x00000, 0x20000, 0xc1298a4b )
	ROM_LOAD16_BYTE( "b52-139.29", 0x00001, 0x20000, 0x997f732e )
	ROM_LOAD16_BYTE( "b52-131.37", 0x40000, 0x20000, 0xaa945d83 )
	ROM_LOAD16_BYTE( "b52-129.30", 0x40001, 0x20000, 0x0eaebc08 )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b52-132.39", 0x00000, 0x10000, 0xa2f54789 )
	ROM_LOAD16_BYTE( "b52-133.55", 0x00001, 0x10000, 0x12232f95 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b52-134.51",    0x00000, 0x04000, 0x91faac7f )
	ROM_CONTINUE(           0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b52-29.27", 0x00000, 0x80000, 0x8366d27c )	/* SCR 8x8*/

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b52-34.5",  0x000000, 0x080000, 0x7d8dce36 )
	ROM_LOAD32_BYTE( "b52-35.7",  0x000001, 0x080000, 0x78eeec0d )	/* OBJ A 16x16 */
	ROM_LOAD32_BYTE( "b52-36.9",  0x000002, 0x080000, 0x61e89e91 )
	ROM_LOAD32_BYTE( "b52-37.11", 0x000003, 0x080000, 0xf02e47b9 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b52-28.4", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b52-30.4",  0x000000, 0x080000, 0x1b8cc647 )
	ROM_LOAD32_BYTE( "b52-31.6",  0x000001, 0x080000, 0xf1998e20 )	/* OBJ B 16x16 */
	ROM_LOAD32_BYTE( "b52-32.8",  0x000002, 0x080000, 0x8620780c )
	ROM_LOAD32_BYTE( "b52-33.10", 0x000003, 0x080000, 0xe6f4b8c4 )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b52-38.34", 0x00000, 0x80000, 0x5b5bf7f6 )	/* STY spritemap */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b52-41.71", 0x000000, 0x80000, 0x8204880c )
	ROM_LOAD( "b52-40.72", 0x080000, 0x80000, 0xf0551055 )
	ROM_LOAD( "b52-39.73", 0x100000, 0x80000, 0xac9cbbd3 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b52-42.70", 0x00000, 0x80000, 0x6e617df1 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "b52-01.7",    0x00000, 0x00100, 0x89719d17 )	/* unused roms */
	ROM_LOAD( "b52-03.135",  0x00000, 0x00400, 0xa3f8490d )
	ROM_LOAD( "b52-06.24",   0x00000, 0x00100, 0xfbf81f30 )
	ROM_LOAD( "b52-18.93",   0x00000, 0x00100, 0x60bdaf1a )	// identical to b52-18b
	ROM_LOAD( "b52-18a",     0x00000, 0x00100, 0x6271be0d )
	ROM_LOAD( "b52-49.68",   0x00000, 0x02000, 0x60dd2ed1 )
	ROM_LOAD( "b52-50.66",   0x00000, 0x10000, 0xc189781c )
	ROM_LOAD( "b52-51.65",   0x00000, 0x10000, 0x30cc1f79 )
	ROM_LOAD( "b52-126.136", 0x00000, 0x00400, 0xfa2f840e )
	ROM_LOAD( "b52-127.156", 0x00000, 0x00400, 0x77682a4f )
ROM_END

ROM_START( enforce )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b58-27.27", 0x00000, 0x20000, 0xa1aa0191 )
	ROM_LOAD16_BYTE( "b58-19.19", 0x00001, 0x20000, 0x40f43da3 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b58-26.26", 0x00000, 0x20000, 0xe823c85c )
	ROM_LOAD16_BYTE( "b58-18.18", 0x00001, 0x20000, 0x65328a3e )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b58-32.41",   0x00000, 0x04000, 0xf3fd8eca )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b58-09.13", 0x00000, 0x80000, 0x9ffd5b31 )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b58-04.7",  0x000000, 0x080000, 0x9482f08d )
	ROM_LOAD32_BYTE( "b58-03.6",  0x000001, 0x080000, 0x158bc440 )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "b58-02.2",  0x000002, 0x080000, 0x6a6e307c )
	ROM_LOAD32_BYTE( "b58-01.1",  0x000003, 0x080000, 0x01e9f0a8 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b58-06.116", 0x00000, 0x80000, 0xb3495d70 )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b58-05.71", 0x00000, 0x80000, 0xd1f4991b )	/* STY spritemap */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b58-07.11", 0x000000, 0x080000, 0xeeb5ba08 )
	ROM_LOAD( "b58-08.12", 0x080000, 0x080000, 0x049243cf )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples ??? */
	ROM_LOAD( "b58-10.14", 0x00000, 0x80000, 0xedce0cc1 )	/* ??? */

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "b58-26a.104", 0x00000, 0x10000, 0xdccb0c7f )	/* unused roms */
	ROM_LOAD( "b58-27.56",   0x00000, 0x02000, 0x5c6b013d )
	ROM_LOAD( "b58-23.52",   0x00000, 0x00100, 0x7b7d8ff4 )
	ROM_LOAD( "b58-24.51",   0x00000, 0x00100, 0xfbf81f30 )
	ROM_LOAD( "b58-25.75",   0x00000, 0x00100, 0xde547342 )
// Add pals...
ROM_END

ROM_START( bshark )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c34_71.98",    0x00000, 0x20000, 0xdf1fa629 )
	ROM_LOAD16_BYTE( "c34_69.75",    0x00001, 0x20000, 0xa54c137a )
	ROM_LOAD16_BYTE( "c34_70.97",    0x40000, 0x20000, 0xd77d81e2 )
	ROM_LOAD16_BYTE( "bshark67.bin", 0x40001, 0x20000, 0x39307c74 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 512K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c34_74.128", 0x00000, 0x20000, 0x6869fa99 )
	ROM_LOAD16_BYTE( "c34_72.112", 0x00001, 0x20000, 0xc09c0f91 )
	ROM_LOAD16_BYTE( "c34_75.129", 0x40000, 0x20000, 0x6ba65542 )
	ROM_LOAD16_BYTE( "c34_73.113", 0x40001, 0x20000, 0xf2fe62b5 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c34_05.3", 0x00000, 0x80000, 0x596b83da )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c34_04.17", 0x000000, 0x080000, 0x2446b0da )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c34_03.16", 0x000001, 0x080000, 0xa18eab78 )
	ROM_LOAD32_BYTE( "c34_02.15", 0x000002, 0x080000, 0x8488ba10 )
	ROM_LOAD32_BYTE( "c34_01.14", 0x000003, 0x080000, 0x3ebe8c63 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "c34_07.42", 0x00000, 0x80000, 0xedb07808 )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c34_06.12", 0x00000, 0x80000, 0xd200b6eb )	/* STY spritemap */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c34_08.127", 0x00000, 0x80000, 0x89a30450 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c34_09.126", 0x00000, 0x80000, 0x39d12b50 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "c34_18.22", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "c34_19.72", 0x00000, 0x00100, 0x2ee9c404 )
	ROM_LOAD( "c34_20.89", 0x00000, 0x00100, 0xfbf81f30 )
	ROM_LOAD( "c34_21.7",  0x00000, 0x00400, 0x10728853 )
	ROM_LOAD( "c34_22.8",  0x00000, 0x00400, 0x643e8bfc )
ROM_END

ROM_START( bsharkj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c34_71.98", 0x00000, 0x20000, 0xdf1fa629 )
	ROM_LOAD16_BYTE( "c34_69.75", 0x00001, 0x20000, 0xa54c137a )
	ROM_LOAD16_BYTE( "c34_70.97", 0x40000, 0x20000, 0xd77d81e2 )
	ROM_LOAD16_BYTE( "c34_66.74", 0x40001, 0x20000, 0xa0392dce )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 512K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c34_74.128", 0x00000, 0x20000, 0x6869fa99 )
	ROM_LOAD16_BYTE( "c34_72.112", 0x00001, 0x20000, 0xc09c0f91 )
	ROM_LOAD16_BYTE( "c34_75.129", 0x40000, 0x20000, 0x6ba65542 )
	ROM_LOAD16_BYTE( "c34_73.113", 0x40001, 0x20000, 0xf2fe62b5 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c34_05.3", 0x00000, 0x80000, 0x596b83da )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c34_04.17", 0x000000, 0x080000, 0x2446b0da )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c34_03.16", 0x000001, 0x080000, 0xa18eab78 )
	ROM_LOAD32_BYTE( "c34_02.15", 0x000002, 0x080000, 0x8488ba10 )
	ROM_LOAD32_BYTE( "c34_01.14", 0x000003, 0x080000, 0x3ebe8c63 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "c34_07.42", 0x00000, 0x80000, 0xedb07808 )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c34_06.12", 0x00000, 0x80000, 0xd200b6eb )	/* STY spritemap */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c34_08.127", 0x00000, 0x80000, 0x89a30450 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c34_09.126", 0x00000, 0x80000, 0x39d12b50 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "c34_18.22", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "c34_19.72", 0x00000, 0x00100, 0x2ee9c404 )
	ROM_LOAD( "c34_20.89", 0x00000, 0x00100, 0xfbf81f30 )
	ROM_LOAD( "c34_21.7",  0x00000, 0x00400, 0x10728853 )
	ROM_LOAD( "c34_22.8",  0x00000, 0x00400, 0x643e8bfc )
ROM_END

ROM_START( sci )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c09-37.rom", 0x00000, 0x20000, 0x0fecea17 )
	ROM_LOAD16_BYTE( "c09-40.rom", 0x00001, 0x20000, 0xe46ebd9b )
	ROM_LOAD16_BYTE( "c09-38.rom", 0x40000, 0x20000, 0xf4404f87 )
	ROM_LOAD16_BYTE( "c09-41.rom", 0x40001, 0x20000, 0xde87bcb9 )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c09-33.rom", 0x00000, 0x10000, 0xcf4e6c5b )
	ROM_LOAD16_BYTE( "c09-32.rom", 0x00001, 0x10000, 0xa4713719 )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c09-34.rom",   0x00000, 0x04000, 0xa21b3151 )
	ROM_CONTINUE(             0x10000, 0x1c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c09-05.rom", 0x00000, 0x80000, 0x890b38f0 )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c09-04.rom", 0x000000, 0x080000, 0x2cbb3c9b )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c09-02.rom", 0x000001, 0x080000, 0xa83a0389 )
	ROM_LOAD32_BYTE( "c09-03.rom", 0x000002, 0x080000, 0xa31d0e80 )
	ROM_LOAD32_BYTE( "c09-01.rom", 0x000003, 0x080000, 0x64bfea10 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "c09-07.rom", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c09-06.rom", 0x00000, 0x80000, 0x12df6d7b )	/* STY spritemap */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c09-14.rom", 0x000000, 0x080000, 0xad78bf46 )
	ROM_LOAD( "c09-13.rom", 0x080000, 0x080000, 0xd57c41d3 )
	ROM_LOAD( "c09-12.rom", 0x100000, 0x080000, 0x56c99fa5 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c09-15.rom", 0x00000, 0x80000, 0xe63b9095 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "c09-16.rom", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "c09-23.rom", 0x00000, 0x00100, 0xfbf81f30 )
//	ROM_LOAD( "c09-21.rom", 0x00000, 0x00???, 0x00000000 )	/* pals (Guru dump) */
//	ROM_LOAD( "c09-22.rom", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c09-24.rom", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c09-25.rom", 0x00000, 0x00???, 0x00000000 )
//	ROM_LOAD( "c09-26.rom", 0x00000, 0x00???, 0x00000000 )
ROM_END

ROM_START( scia )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c09-28.bin",  0x00000, 0x20000, 0x630dbaad )
	ROM_LOAD16_BYTE( "c09-30.bin",  0x00001, 0x20000, 0x68b1a97d )
	ROM_LOAD16_BYTE( "c09-36.bin",  0x40000, 0x20000, 0x59e47cba )
	ROM_LOAD16_BYTE( "c09-31.bin",  0x40001, 0x20000, 0x962b1fbf )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c09-33.rom", 0x00000, 0x10000, 0xcf4e6c5b )
	ROM_LOAD16_BYTE( "c09-32.rom", 0x00001, 0x10000, 0xa4713719 )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c09-34.rom",   0x00000, 0x04000, 0xa21b3151 )
	ROM_CONTINUE(             0x10000, 0x1c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c09-05.rom", 0x00000, 0x80000, 0x890b38f0 )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c09-04.rom", 0x000000, 0x080000, 0x2cbb3c9b )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c09-02.rom", 0x000001, 0x080000, 0xa83a0389 )
	ROM_LOAD32_BYTE( "c09-03.rom", 0x000002, 0x080000, 0xa31d0e80 )
	ROM_LOAD32_BYTE( "c09-01.rom", 0x000003, 0x080000, 0x64bfea10 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "c09-07.rom", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c09-06.rom", 0x00000, 0x80000, 0x12df6d7b )	/* STY spritemap */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c09-14.rom", 0x000000, 0x080000, 0xad78bf46 )
	ROM_LOAD( "c09-13.rom", 0x080000, 0x080000, 0xd57c41d3 )
	ROM_LOAD( "c09-12.rom", 0x100000, 0x080000, 0x56c99fa5 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c09-15.rom", 0x00000, 0x80000, 0xe63b9095 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "c09-16.rom", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "c09-23.rom", 0x00000, 0x00100, 0xfbf81f30 )
ROM_END

ROM_START( sciu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c09-43.37",  0x00000, 0x20000, 0x20a9343e )
	ROM_LOAD16_BYTE( "c09-44.40",  0x00001, 0x20000, 0x7524338a )
	ROM_LOAD16_BYTE( "c09-41.38",  0x40000, 0x20000, 0x83477f11 )
	ROM_LOAD16_BYTE( "c09-41.rom", 0x40001, 0x20000, 0xde87bcb9 )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c09-33.rom", 0x00000, 0x10000, 0xcf4e6c5b )
	ROM_LOAD16_BYTE( "c09-32.rom", 0x00001, 0x10000, 0xa4713719 )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "c09-34.rom",   0x00000, 0x04000, 0xa21b3151 )
	ROM_CONTINUE(             0x10000, 0x1c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c09-05.rom", 0x00000, 0x80000, 0x890b38f0 )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c09-04.rom", 0x000000, 0x080000, 0x2cbb3c9b )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c09-02.rom", 0x000001, 0x080000, 0xa83a0389 )
	ROM_LOAD32_BYTE( "c09-03.rom", 0x000002, 0x080000, 0xa31d0e80 )
	ROM_LOAD32_BYTE( "c09-01.rom", 0x000003, 0x080000, 0x64bfea10 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "c09-07.rom", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c09-06.rom", 0x00000, 0x80000, 0x12df6d7b )	/* STY spritemap */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c09-14.rom", 0x000000, 0x080000, 0xad78bf46 )
	ROM_LOAD( "c09-13.rom", 0x080000, 0x080000, 0xd57c41d3 )
	ROM_LOAD( "c09-12.rom", 0x100000, 0x080000, 0x56c99fa5 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c09-15.rom", 0x00000, 0x80000, 0xe63b9095 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "c09-16.rom", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "c09-23.rom", 0x00000, 0x00100, 0xfbf81f30 )
ROM_END

ROM_START( nightstr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b91-45.bin", 0x00000, 0x20000, 0x7ad63421 )
	ROM_LOAD16_BYTE( "b91-44.bin", 0x00001, 0x20000, 0x4bc30adf )
	ROM_LOAD16_BYTE( "b91-43.bin", 0x40000, 0x20000, 0x3e6f727a )
	ROM_LOAD16_BYTE( "b91-46.bin", 0x40001, 0x20000, 0xe870be95 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b91-39.bin", 0x00000, 0x20000, 0x725b23ae )
	ROM_LOAD16_BYTE( "b91-40.bin", 0x00001, 0x20000, 0x81fb364d )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b91-41.bin",   0x00000, 0x04000, 0x2694bb42 )
	ROM_CONTINUE(             0x10000, 0x1c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b91-11.bin", 0x00000, 0x80000, 0xfff8ce31 )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b91-04.bin", 0x000000, 0x080000, 0x8ca1970d )	/* OBJ A 16x16 */
	ROM_LOAD32_BYTE( "b91-03.bin", 0x000001, 0x080000, 0xcd5fed39 )
	ROM_LOAD32_BYTE( "b91-02.bin", 0x000002, 0x080000, 0x457c64b8 )
	ROM_LOAD32_BYTE( "b91-01.bin", 0x000003, 0x080000, 0x3731d94f )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b91-10.bin", 0x00000, 0x80000, 0x1d8f05b4 )	/* ROD, road lines */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b91-08.bin", 0x000000, 0x080000, 0x66f35c34 )	/* OBJ B 16x16 */
	ROM_LOAD32_BYTE( "b91-07.bin", 0x000001, 0x080000, 0x4d8ec6cf )
	ROM_LOAD32_BYTE( "b91-06.bin", 0x000002, 0x080000, 0xa34dc839 )
	ROM_LOAD32_BYTE( "b91-05.bin", 0x000003, 0x080000, 0x5e72ac90 )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b91-09.bin", 0x00000, 0x80000, 0x5f247ca2 )	/* STY spritemap */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b91-13.bin", 0x00000, 0x80000, 0x8c7bf0f5 )
	ROM_LOAD( "b91-12.bin", 0x80000, 0x80000, 0xda77c7af )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b91-14.bin", 0x00000, 0x80000, 0x6bc314d3 )

	ROM_REGION( 0x10000, REGION_USER2, 0 )
	ROM_LOAD( "b91-26.bin", 0x00000, 0x400,   0x77682a4f )	/* unused roms */
	ROM_LOAD( "b91-27.bin", 0x00000, 0x400,   0xa3f8490d )
	ROM_LOAD( "b91-28.bin", 0x00000, 0x400,   0xfa2f840e )
	ROM_LOAD( "b91-29.bin", 0x00000, 0x2000,  0xad685be8 )
	ROM_LOAD( "b91-30.bin", 0x00000, 0x10000, 0x30cc1f79 )
	ROM_LOAD( "b91-31.bin", 0x00000, 0x10000, 0xc189781c )
	ROM_LOAD( "b91-32.bin", 0x00000, 0x100,   0xfbf81f30 )
	ROM_LOAD( "b91-33.bin", 0x00000, 0x100,   0x89719d17 )
ROM_END

ROM_START( aquajack )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b77-22.rom", 0x00000, 0x20000, 0x67400dde )
	ROM_LOAD16_BYTE( "34.17",      0x00001, 0x20000, 0xcd4d0969 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b77-24.rom", 0x00000, 0x20000, 0x95e643ed )
	ROM_LOAD16_BYTE( "b77-23.rom", 0x00001, 0x20000, 0x395a7d1c )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "b77-20.rom",   0x00000, 0x04000, 0x84ba54b7 )
	ROM_CONTINUE(             0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b77-05.rom", 0x00000, 0x80000, 0x7238f0ff )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b77-04.rom", 0x000000, 0x80000, 0xbed0be6c )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "b77-03.rom", 0x000001, 0x80000, 0x9a3030a7 )
	ROM_LOAD32_BYTE( "b77-02.rom", 0x000002, 0x80000, 0xdaea0d2e )
	ROM_LOAD32_BYTE( "b77-01.rom", 0x000003, 0x80000, 0xcdab000d )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b77-07.rom", 0x000000, 0x80000, 0x7db1fc5e )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b77-06.rom", 0x00000, 0x80000, 0xce2aed00 )	/* STY spritemap */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b77-09.rom", 0x00000, 0x80000, 0x948e5ad9 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b77-08.rom", 0x00000, 0x80000, 0x119b9485 )

/*	(no unused roms in my set, there should be an 0x10000 one like the rest) */
ROM_END

ROM_START( aquajckj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b77-22.rom", 0x00000, 0x20000, 0x67400dde )
	ROM_LOAD16_BYTE( "b77-21.rom", 0x00001, 0x20000, 0x23436845 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b77-24.rom", 0x00000, 0x20000, 0x95e643ed )
	ROM_LOAD16_BYTE( "b77-23.rom", 0x00001, 0x20000, 0x395a7d1c )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "b77-20.rom",   0x00000, 0x04000, 0x84ba54b7 )
	ROM_CONTINUE(             0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b77-05.rom", 0x00000, 0x80000, 0x7238f0ff )	/* SCR 8x8 */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b77-04.rom", 0x000000, 0x80000, 0xbed0be6c )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "b77-03.rom", 0x000001, 0x80000, 0x9a3030a7 )
	ROM_LOAD32_BYTE( "b77-02.rom", 0x000002, 0x80000, 0xdaea0d2e )
	ROM_LOAD32_BYTE( "b77-01.rom", 0x000003, 0x80000, 0xcdab000d )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "b77-07.rom", 0x000000, 0x80000, 0x7db1fc5e )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "b77-06.rom", 0x00000, 0x80000, 0xce2aed00 )	/* STY spritemap */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b77-09.rom", 0x00000, 0x80000, 0x948e5ad9 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b77-08.rom", 0x00000, 0x80000, 0x119b9485 )

/*	(no unused roms in my set, there should be an 0x10000 one like the rest) */
ROM_END

ROM_START( spacegun )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c57-18.62", 0x00000, 0x20000, 0x19d7d52e )
	ROM_LOAD16_BYTE( "c57-20.74", 0x00001, 0x20000, 0x2e58253f )
	ROM_LOAD16_BYTE( "c57-17.59", 0x40000, 0x20000, 0xe197edb8 )
	ROM_LOAD16_BYTE( "c57-22.73", 0x40001, 0x20000, 0x5855fde3 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c57-15.27", 0x00000, 0x20000, 0xb36eb8f1 )
	ROM_LOAD16_BYTE( "c57-16.29", 0x00001, 0x20000, 0xbfb5d1e7 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c57-06.52", 0x00000, 0x80000, 0x4ebadd5b )		/* SCR 8x8 */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c57-01.25", 0x000000, 0x100000, 0xf901b04e )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c57-02.24", 0x000001, 0x100000, 0x21ee4633 )
	ROM_LOAD32_BYTE( "c57-03.12", 0x000002, 0x100000, 0xfafca86f )
	ROM_LOAD32_BYTE( "c57-04.11", 0x000003, 0x100000, 0xa9787090 )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c57-05.36", 0x00000, 0x80000, 0x6a70eb2e )	/* STY spritemap */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c57-07.76", 0x00000, 0x80000, 0xad653dc1 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c57-08.75", 0x00000, 0x80000, 0x22593550 )

/*	(no unused 0x10000 rom like the rest?) */
	ROM_REGION( 0x10000, REGION_USER2, 0 )
//	ROM_LOAD( "c57-09.9",  0x00000, 0xada, 0x306f130b )	/* pals */
//	ROM_LOAD( "c57-10.47", 0x00000, 0xcd5, 0xf11474bd )
//	ROM_LOAD( "c57-11.48", 0x00000, 0xada, 0xb33be19f )
//	ROM_LOAD( "c57-12.61", 0x00000, 0xcd5, 0xf1847096 )
//	ROM_LOAD( "c57-13.72", 0x00000, 0xada, 0x795f0a85 )
//	ROM_LOAD( "c57-14.96", 0x00000, 0xada, 0x5b3c40b7 )
ROM_END

ROM_START( dblaxle )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c78-41.3",  0x00000, 0x20000, 0xcf297fe4 )
	ROM_LOAD16_BYTE( "c78-43.5",  0x00001, 0x20000, 0x38a8bad6 )
	ROM_LOAD16_BYTE( "c78-42.4",  0x40000, 0x20000, 0x4124ab2b )
	ROM_LOAD16_BYTE( "c78-44.6",  0x40001, 0x20000, 0x50a55b6e )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c78-30-1.35", 0x00000, 0x20000, 0x026aac18 )
	ROM_LOAD16_BYTE( "c78-31-1.36", 0x00001, 0x20000, 0x67ce23e8 )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD    ( "ic42", 0x00000, 0x04000, 0xf2186943 )
	ROM_CONTINUE(         0x10000, 0x1c000 )	/* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c78-10.12", 0x00000, 0x80000, 0x44b1897c )	/* SCR 8x8 */
	ROM_LOAD16_BYTE( "c78-11.11", 0x00001, 0x80000, 0x7db3d4a3 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c78-08.25", 0x000000, 0x100000, 0x6c725211 )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c78-07.33", 0x000001, 0x100000, 0x9da00d5b )
	ROM_LOAD32_BYTE( "c78-06.23", 0x000002, 0x100000, 0x8309e91b )
	ROM_LOAD32_BYTE( "c78-05.31", 0x000003, 0x100000, 0x90001f68 )
//	ROMX_LOAD      ( "c78-05l.1", 0x000003, 0x080000, 0xf24bf972, ROM_SKIP(7) )
//	ROMX_LOAD      ( "c78-05h.2", 0x000007, 0x080000, 0xc01039b5, ROM_SKIP(7) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "c78-09.12", 0x000000, 0x80000, 0x0dbde6f5 )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c78-04.3", 0x00000, 0x80000, 0xcc1aa37c )	/* STY spritemap */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c78-12.33", 0x000000, 0x80000, 0xfbb39585 )	// Half size ??
	// ??? gap 0x80000-fffff //
	ROM_LOAD( "c78-13.46", 0x100000, 0x80000, 0x1b363aa2 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c78-14.31",  0x00000, 0x80000, 0x9cad4dfb )

	ROM_REGION( 0x10000, REGION_USER2, 0 )	/* unused roms */
	ROM_LOAD( "c78-25.15",  0x00000, 0x10000, 0x7245a6f6 )	// 98% compression
	ROM_LOAD( "c78-15.22",  0x00000, 0x00100, 0xfbf81f30 )
	ROM_LOAD( "c78-21.74",  0x00000, 0x00100, 0x2926bf27 )
	ROM_LOAD( "c84-10.16",  0x00000, 0x00400, 0x643e8bfc )
	ROM_LOAD( "c84-11.17",  0x00000, 0x00400, 0x10728853 )
ROM_END

ROM_START( pwheelsj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "c78-26-2.2",  0x00000, 0x20000, 0x25c8eb2e )
	ROM_LOAD16_BYTE( "c78-28-2.4",  0x00001, 0x20000, 0xa9500eb1 )
	ROM_LOAD16_BYTE( "c78-27-2.3",  0x40000, 0x20000, 0x08d2cffb )
	ROM_LOAD16_BYTE( "c78-29-2.5",  0x40001, 0x20000, 0xe1608004 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "c78-30-1.35", 0x00000, 0x20000, 0x026aac18 )
	ROM_LOAD16_BYTE( "c78-31-1.36", 0x00001, 0x20000, 0x67ce23e8 )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD    ( "c78-32.42",    0x00000, 0x04000, 0x1494199c )
	ROM_CONTINUE(                 0x10000, 0x1c000 )	/* banked stuff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c78-10.12", 0x00000, 0x80000, 0x44b1897c )	/* SCR 8x8 */
	ROM_LOAD16_BYTE( "c78-11.11", 0x00001, 0x80000, 0x7db3d4a3 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c78-08.25", 0x000000, 0x100000, 0x6c725211 )	/* OBJ 16x8 */
	ROM_LOAD32_BYTE( "c78-07.33", 0x000001, 0x100000, 0x9da00d5b )
	ROM_LOAD32_BYTE( "c78-06.23", 0x000002, 0x100000, 0x8309e91b )
	ROM_LOAD32_BYTE( "c78-05.31", 0x000003, 0x100000, 0x90001f68 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* don't dispose */
	ROM_LOAD( "c78-09.12", 0x000000, 0x80000, 0x0dbde6f5 )	/* ROD, road lines */

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c78-04.3", 0x00000, 0x80000, 0xcc1aa37c )	/* STY spritemap */

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c78-01.33", 0x000000, 0x100000, 0x90ff1e72 )
	ROM_LOAD( "c78-02.46", 0x100000, 0x080000, 0x8882d2b7 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c78-03.31",  0x00000, 0x80000, 0x9b926a2f )

	ROM_REGION( 0x10000, REGION_USER2, 0 )	/* unused roms */
	ROM_LOAD( "c78-25.15",  0x00000, 0x10000, 0x7245a6f6 )	// 98% compression
	ROM_LOAD( "c78-15.22",  0x00000, 0x00100, 0xfbf81f30 )
	ROM_LOAD( "c78-21.74",  0x00000, 0x00100, 0x2926bf27 )
	ROM_LOAD( "c84-10.16",  0x00000, 0x00400, 0x643e8bfc )
	ROM_LOAD( "c84-11.17",  0x00000, 0x00400, 0x10728853 )
ROM_END


static DRIVER_INIT( taitoz )
{
//	taitosnd_setz80_soundcpu( 2 );

	cpua_ctrl = 0xff;
	state_save_register_UINT16("main1", 0, "control", &cpua_ctrl, 1);
	state_save_register_func_postload(parse_control);

	/* these are specific to various games: we ought to split the inits */
	state_save_register_int   ("main2", 0, "control", &sci_int6);
	state_save_register_int   ("main3", 0, "control", &dblaxle_int6);
	state_save_register_int   ("main4", 0, "register", &ioc220_port);

	state_save_register_int   ("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}

static DRIVER_INIT( bshark )
{
	cpua_ctrl = 0xff;
	state_save_register_UINT16("main1", 0, "control", &cpua_ctrl, 1);
	state_save_register_func_postload(parse_control_noz80);

	state_save_register_UINT16("main2", 0, "control", &eep_latch, 1);
}



/* Release date order: contcirc 1989 (c) date is bogus */

GAMEX( 1989, contcirc, 0,        contcirc, contcirc, taitoz,   ROT0,               "Taito Corporation Japan", "Continental Circus (World)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1987, contcrcu, contcirc, contcirc, contcrcu, taitoz,   ROT0,               "Taito America Corporation", "Continental Circus (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1988, chasehq,  0,        chasehq,  chasehq,  taitoz,   ROT0,               "Taito Corporation Japan", "Chase HQ (World)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1988, chasehqj, chasehq,  chasehq,  chasehqj, taitoz,   ROT0,               "Taito Corporation", "Chase HQ (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1988, enforce,  0,        enforce,  enforce,  taitoz,   ROT0,               "Taito Corporation", "Enforce (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, bshark,   0,        bshark,   bshark,   bshark,   ORIENTATION_FLIP_X, "Taito America Corporation", "Battle Shark (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, bsharkj,  bshark,   bshark,   bsharkj,  bshark,   ORIENTATION_FLIP_X, "Taito Corporation", "Battle Shark (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, sci,      0,        sci,      sci,      taitoz,   ROT0,               "Taito Corporation Japan", "Special Criminal Investigation (World set 1)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, scia,     sci,      sci,      sci,      taitoz,   ROT0,               "Taito Corporation Japan", "Special Criminal Investigation (World set 2)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, sciu,     sci,      sci,      sciu,     taitoz,   ROT0,               "Taito America Corporation", "Special Criminal Investigation (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1989, nightstr, 0,        nightstr, nightstr, taitoz,   ROT0,               "Taito America Corporation", "Night Striker (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1990, aquajack, 0,        aquajack, aquajack, taitoz,   ROT0,               "Taito Corporation Japan", "Aqua Jack (World)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1990, aquajckj, aquajack, aquajack, aquajckj, taitoz,   ROT0,               "Taito Corporation", "Aqua Jack (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME ( 1990, spacegun, 0,        spacegun, spacegun, bshark,   ORIENTATION_FLIP_X, "Taito Corporation Japan", "Space Gun (World)" )
GAMEX( 1991, dblaxle,  0,        dblaxle,  dblaxle,  taitoz,   ROT0,               "Taito America Corporation", "Double Axle (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1991, pwheelsj, dblaxle,  dblaxle,  pwheelsj, taitoz,   ROT0,               "Taito Corporation", "Power Wheels (Japan)", GAME_IMPERFECT_GRAPHICS )
