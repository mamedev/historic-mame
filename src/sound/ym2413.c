/*

    Copyright (C) 2000  Charles Mac Donald

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    YM2413 (OPLL) emulator
    by Charles Mac Donald
    E-mail: cgfm2@hooked.net
    WWW: http://cgfm2.emuviews.com

    General improvements, YM2413 internal register emulation and
    a lot of soundpatches tweaking
    by Fabio Ricardo Schmidlin (FRS)
    E-mail: sd_snatcher@yahoo.com
    Thanks: Sean Young, Mauro Sans Jr, DalPoz

    Change log:

    [04212001] (by FRS)
    - Removed some compilation warnings

    [04172001] (by FRS)
    - A lot of tweaking/testing on the piano patch.
    - Some code cleanup as sugested by Sean.

    [04122001] (by FRS)
    - Fixed a bug created when coding the DAC emulation. Now Aleste2's
      music6 and sfx are playing right again.
    - A lot of debugging messages re-enabled again.

    [04082001] (by FRS)
    - PCM mode implemented, but it doesn't play...
    - The docs about the DAC DATA were wrong. The higher 4bits are output,
      instead of the lower 4bits.
    - PCM mode fixed. Now MSX's ClubGuide3 diskmagazine programs FMDEMO{1,2}
      and FMSAMP plays all samples right.

    Reg #10h (in PCM mode)

       7   6   5   4   3   2   1   0
     +---+---+---+---+---+---+---+---+
     |    DAC DATA   | - | - | - | - |
     +---+---+---+---+---+---+---+---+


    [03312001] (by FRS)
    - Thanks to Dalpoz now I have documentation describing the YM2413 PCM
      mode, which will be implemented soon. It works as decribed bellow:

    Reg #0Fh  TST

       7   6   5   4   3   2   1   0
     +---+---+---+---+---+---+---+---+
     | - | - | - | - |SND| - | - |SMP|
     +---+---+---+---+---+---+---+---+

     SND = 1 : Sound output from the chip is out
     SMP = 1 : Sample mode ON. The higher 4 bits of the register #10h are
               output to the audio out.

    [03262001] (by FRS)
    - Now I'm reading the soundbank from ym2413.rom, which is more compliant
      with MAME's filosofy. But meanwhile I'm not using standard MAME's
      functions to do that, what's surely on the TODO list.

    [03242001] (by FRS)
    - Hacked it into MESS0.37b12
    - Deleted function ym2413_init(), since MAME/MESS does the same job.
    - Now I'm detecting if MAME is using the real or emulated YM2413 and
      adjusting the drumkit volumes accordingly.
    - New piano patch, It stills not good, but at least some games now have a
      piano...  :P  (the previous patch was sounding very very low)
      MSX's "Rune Worth" introdemo uses a lot if pianos, and it sound stills
      very bad yet. But "Psycho World" now sounds very good!!!   :))

    [03182001] (by FRS)
    - More tests, a little adjstment on hihat's RR. Now MonMon Monster and
      Bit2 games sounds right.

    [03172001] (by FRS)
    - New PATCH_RHYTHM_FRS drumkit created.
    - A lot of tweaking on the drumkit. More than 12h of hacking. Lots of
      tests on SynthSaurus2.
    - Extensive MSX2 game testing. Thanks Mauro Sans Jr for helping me.

    [03162001] (by FRS)
    - Now I'm using djgpp on DOS and the real SB32 OPL3 to do the tests
    - Argh! I discovered the 1st parameter of SD and HH is swapped on
      OPL2/3. I don't know if this happens on OPL1. A software workaround
      is now done, so this 1st parameter is write swapped too.
    - A little trick will help me to adjust and test the rhythm sound without
      need a lot of debug as until now. I've pointed the SD an HH table
      to the USER instument table for now. With this I can smoothly adjust
      the SD and HH (next will be TOM and TCY) parameters on Synth Saurus2. ;)


    [03082001] (by FRS)
    - On the original load_instrument function, when the YM2413 user instrument
      was redefined, it redefined all channels with instr=0 eight times! Yes,
      it's one full YM3812 definition look per each YM2413 user instr access.
      This means a worst case of 576 YM3812 writes! Now only the register
      being modified will be send to the YM3812.
    - I much better statefull sustain mechanism is now implemented.
    - Bugfix: I was saving the SLRR from modulator when it should be from
      carrier... Ops!  :P

    [03072001] (by FRS)
    - YM2413 handles SUS on key-off! And also handles key-off for Percursive
      Tones (EG-TYP=0) too! On key off, if SUSTAIN=1 then RR=5 else RR=?

    [03062001] (by FRS)
    - Fixed a bug in volume caching routine. The test condition was very
      wrong. Note to self: Do not code after 06:30am...   :P
    - After all this fixes Okazaki's patches now sound better than Allegro's,
      so it will be the default.

    [03052001] (by FRS)
    - Removed the vol correction formula, since it was wrong.

    [03042001] (by FRS)
    - I did not notice until now that Okazaki's patches was in YM2413 format
      (stupid me :P ), now they're converted.
    - Fixed a bug in the if rhythm/norhythm volume setting.
    - Fixed a bug in rhythm mode on/off setting that caused the battery
      TL/volume to malfunction.
    - Removed bassdrum volhi setting. Real YM2413 testing reveals it does'nt
      handle it too.
    - I've put a lot of debuging messages on invalid registers and unused bits.
      The bait is set, let's wait to see if any game uses it.

    [02192001] (by FRS)
    - Made some tweaking on the Allegro instruments. Better Acoustic Bass and
      Eletric Guitar. Now its sound better than Okazaki's, so it will
      be the default.
    - Now ym2413_reset() loads the default user patch. A lot of MSX games
      get correctly played.

    [02182001] (by FRS)
    - Modified TL's of Okazaki's Snare Drum and HiHat. It was too loud.
    - Now you can select which instrument patches will be compiled using defines
      on ym2413.h.
    - Added Instrument settings from Mitsutaka Okazaki's emu2413 v0.38.
    - New ym3812_TLs' formula, using both YM2413 TLs volumes. Now a lot of
      MSX musics sounds much better.
    - Damn!!! The vi editor messed up with all indentation...  :(
      Runing indent organized it a little, but i had to reindent a lot
      by hand...  :-/
    - Bugfix: The YM3812 set for rhythm mod is now done before all other
              settings. And turning off the rhythm mode now restore the instri
              settings of channels 7,8,9. Hopefully this fixes FireHawk's
              "Moonlight Sonatta" on MSX2.
    - Bugfix: Now KSL's are preserved on rhythm volume change.
    - New rhythm_volume() function.
    - Change the opll->channel[ch].volume formula to correct the scale.
      It's now VOL=(VOL<<2)+3*(VOL&1) instead of the simple VOL=VOL<<2 formula.

    [02172001] (by FRS)
    - Bugfix: KSLc of channels 7,8,9 was not being set nor saved on rhythm mode.
    - Rhythm values are not hardcoded in rhythm_mode_init anymore. Moved
      them to rhythmtable[].
    - Bugfix: SLRR of channels 7,8,9 is now being properly saved when rhythm
      mode is set (used by sustain).

    [02162001] (by FRS)
    - Fixed a bug in my volume caching routine where changing user instrument
      doesn't update the registers. Now it's using a "dirty flag" to indicate
      that the user instrument changed.

    [02152001] (by FRS)
    - Implemented a YM3812 parameters structure. This will allow
      to implement sustain and more speed-ups.
    - Optimized YM2413 volume changing, based on the cache described above.
      This solved the clicky sound on many MSX games that modifies
      the volume constantly. Without this, the respective YM3812 channel
      instrument was fully redefined on every YM2413 volume change.

    [02142001] (by FRS)
    - At this point, this program only does an almost direct register
      mapping. The sound is almost good, but no internal YM2413 registers
      are emulated, the volumes are completely wrong (even worse for the
      drumkit) and the rhythm mode is very poorly implemented: It just
      switch the YM3812 and copy the drumkit patches. Both the drumkit
      and user defined instrument are mishandled in many ways, so MSX
      games that uses this resources sound very strangely. I.e., some Aleste-2
      musics get surprisingly good, while others plays weird a lot.
      Anyway, the sound is far more better than the old MAME's YM2413toYM3812
      translator, and the code is very well structured.

    [06132000]
    - Fixed bug where channel numbers larger than 9 could be written to
      register groups $10-18, $20-28, $30-38.

    [06082000]
    - Now the YM2413 chip number is passed to the OPL_WRITE macro,
      and the user instrument data is stored in the YM2413 context,
      both for multiple YM2413 emulation.

    [06012000]
    - Added alternate instrument table taken from Allegro's 'fm_inst.h'.
    - Changed source so it can compile seperately from SMS Plus.
    - Added 'ym2413_reset' function and changed ym2413_init.

    TODO:

    - A lot of tests and comparing against a real YM2413.
    - The undocumented YM2413 PCM mode.
    - Load YM2413.ROM using some standard MAME method, and malloc the
      buffer instead of using a predefined vector.

    Known issues:

    - The table of fixed instrument values probably need to be compared
      against a real YM2413, so they can be hand-tuned.

    - Carefully check AT/DE/RR timings against my MSX Turbo-R's real YM2413.
      (before 02:00am, obviously...  ;)

    - I must check the dB atenuation of vibrato and tremolo against
      a real YM2413, because the YM2413 documentation doesn't metion it.

    - The piano patch does not sound right yet.

 ------------------------------------------------------------------
    I based the YM2413 emulation on the following documents. If you want
    to improve it or make changes, I'd advise reading the following:

    - Yamaha's YMF-272 (OPL-3) programmer's manual. (ymf272.pdf)
      (Has useful table of how the operators map to YM3812 registers)

    - Yamaha's YM2413 programmer's manual. (ym2413.lzh)

    - Vladimir Arnost's OPL-3 programmer's guide. (opl3.txt)
      (Explains operator allocation in rhythm mode)

    - The YM2413 emulation from MAME. (ym2413.c/2413intf.h)
*/

#include "driver.h"
#include "ym2413.h"

#define VERBOSE 1

/* Internal use */
static void load_instrument(int chip, int ch, int inst, int vol);
static void rhythm_mode_init(int chip);
static void rhythm_volume(int chip, int ch);

/* You can replace this to output to another YM3812 emulator
   or a perhaps a real OPL-2/OPL-3 sound chip */
/*
#if USE_ADLIB
	#define OPL_WRITE(c,r,d)  { outp(0x388+c*2, r); outp(0x389+c*2, d); }
#else
	#define OPL_WRITE(c,r,d)  OPLWriteReg(ym3812, r, d)
#endif
*/
#define OPL_WRITE(c,r,d) { YM3812_control_port_0_w(0, r); YM3812_write_port_0_w(0, d); }

/* YM2413 chip contexts */
t_ym2413 ym2413[MAX_YM2413];
t_ym3812 ym3812[MAX_YM2413];

/* Fixed instrument settings, select which do you want */
/* This might need some tweaking... */
static unsigned char table[16][11] = {
    /*   20    23    40    43    60    63    80    83    E0    E3    C0 */
#ifdef PATCH_MAME /* Ugly instrument settings from MAME */
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x01, 0x22, 0x23, 0x07, 0xF0, 0xF0, 0x07, 0x18, 0x00, 0x00, 0x00},
  {0x23, 0x01, 0x68, 0x05, 0xF2, 0x74, 0x6C, 0x89, 0x00, 0x00, 0x00},
  {0x13, 0x11, 0x25, 0x00, 0xD2, 0xB2, 0xF4, 0xF4, 0x00, 0x00, 0x00},
  {0x22, 0x21, 0x1B, 0x05, 0xC0, 0xA1, 0x18, 0x08, 0x00, 0x00, 0x00},
  {0x22, 0x21, 0x2C, 0x03, 0xD2, 0xA1, 0x18, 0x57, 0x00, 0x00, 0x00},
  {0x01, 0x22, 0xBA, 0x01, 0xF1, 0xF1, 0x1E, 0x04, 0x00, 0x00, 0x00},
  {0x21, 0x21, 0x28, 0x06, 0xF1, 0xF1, 0x6B, 0x3E, 0x00, 0x00, 0x00},
  {0x27, 0x21, 0x60, 0x00, 0xF0, 0xF0, 0x0D, 0x0F, 0x00, 0x00, 0x00},
  {0x20, 0x21, 0x2B, 0x06, 0x85, 0xF1, 0x6D, 0x89, 0x00, 0x00, 0x00},
  {0x01, 0x21, 0xBF, 0x02, 0x53, 0x62, 0x5F, 0xAE, 0x01, 0x00, 0x00},
  {0x23, 0x21, 0x70, 0x07, 0xD4, 0xA3, 0x4E, 0x64, 0x01, 0x00, 0x00},
  {0x2B, 0x21, 0xA4, 0x07, 0xF6, 0x93, 0x5C, 0x4D, 0x00, 0x00, 0x00},
  {0x21, 0x23, 0xAD, 0x07, 0x77, 0xF1, 0x18, 0x37, 0x00, 0x00, 0x00},
  {0x21, 0x21, 0x2A, 0x03, 0xF3, 0xE2, 0x29, 0x46, 0x00, 0x00, 0x00},
  {0x21, 0x23, 0x37, 0x03, 0xF3, 0xE2, 0x29, 0x46, 0x00, 0x00, 0x00},
#endif
#ifdef PATCH_ALLEGRO	/* Instrument settings from Allegro */
  {0x49, 0x4C, 0x4C, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x31, 0x21, 0x15, 0x09, 0xdd, 0x56, 0x13, 0x26, 0x01, 0x00, 0x08}, /* Violin */
  {0x03, 0x11, 0x54, 0x09, 0xf3, 0xf1, 0x9a, 0xe7, 0x01, 0x00, 0x0c}, /* Acoustic Guitar(steel) */
  {0x21, 0x21, 0x8f, 0x0c, 0xf2, 0xf2, 0x45, 0x76, 0x00, 0x00, 0x08}, /* Acoustic Grand Piano */
  {0xe1, 0xe1, 0x46, 0x09, 0x88, 0x65, 0x5f, 0x1a, 0x00, 0x00, 0x00}, /* Flute */
  {0x32, 0x21, 0x90, 0x09, 0x9b, 0x72, 0x21, 0x17, 0x00, 0x00, 0x04}, /* Clarinet */
  {0x21, 0x21, 0x4b, 0x09, 0xaa, 0x8f, 0x16, 0x0a, 0x01, 0x00, 0x08}, /* Oboe */
  {0x21, 0x21, 0x92, 0x0a, 0x85, 0x8f, 0x17, 0x09, 0x00, 0x00, 0x0c}, /* Trumpet */
  {0x23, 0xb1, 0x93, 0x09, 0x97, 0x55, 0x23, 0x14, 0x01, 0x00, 0x04}, /* Church Organ */
  {0x21, 0x21, 0x9b, 0x09, 0x61, 0x7f, 0x6a, 0x0a, 0x00, 0x00, 0x02}, /* French Horn */
  {0x71, 0x72, 0x57, 0x09, 0x54, 0x7a, 0x05, 0x05, 0x00, 0x00, 0x0c}, /* Synth Voice */
  {0x21, 0x36, 0x80, 0x17, 0xa2, 0xf1, 0x01, 0xd5, 0x00, 0x00, 0x08}, /* Harpsichord */
  {0x18, 0x81, 0x62, 0x09, 0xf3, 0xf2, 0xe6, 0xf6, 0x00, 0x00, 0x00}, /* Vibraphone */
  {0x31, 0x31, 0x8b, 0x09, 0xf4, 0xf1, 0xe8, 0x78, 0x00, 0x00, 0x0a}, /* Synth Bass 1 */
  {0x21, 0xa2, 0x1e, 0x03, 0x94, 0xc3, 0x06, 0xa6, 0x00, 0x00, 0x02}, /* Acoustic Bass */
  {0x03, 0x21, 0x87, 0x83, 0xf6, 0xf3, 0x22, 0xf8, 0x01, 0x00, 0x06}, /* Electric Guitar(clean) */
#endif
#ifdef PATCH_OKAZAKI
  /* Instrument settings from Mitsutaka Okazaki's emu2413 v0.38 */
  /* Converted to YM3812 format, plus TL corrections            */
  /* Tweaked instruments by FRS                                 */
/* {0x49, 0x4c, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04}, */
  {0x49, 0x4c, 0x4c, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04},
  {0x61, 0x61, 0x1e, 0x06, 0xf0, 0x7f, 0x07, 0x17, 0x00, 0x01, 0x0e},
  {0x13, 0x41, 0x8f, 0x14, 0xce, 0xd2, 0x43, 0x13, 0x01, 0x00, 0x0a},
/*  {0x03, 0x01, 0x99, 0x00, 0xff, 0xc3, 0x03, 0x73, 0x00, 0x00, 0x08}, */
  {0x03, 0x01, 0x4a, 0x04, 0xff, 0xd3, 0x23, 0x73, 0x00, 0x00, 0x08},
/*  {0x21, 0x21, 0x8f, 0x09, 0xff, 0xd3, 0x03, 0x73, 0x00, 0x00, 0x08}, Acoustic Grand Piano */
  {0x21, 0x61, 0x1b, 0x09, 0xaf, 0x63, 0x40, 0x28, 0x00, 0x00, 0x0e},
  {0x22, 0x21, 0x1e, 0x09, 0xf0, 0x76, 0x08, 0x28, 0x00, 0x00, 0x0c},
  {0x31, 0x22, 0x16, 0x09, 0x90, 0x7f, 0x00, 0x08, 0x00, 0x00, 0x0a},
  {0x21, 0x61, 0x1d, 0x05, 0x82, 0x81, 0x10, 0x17, 0x00, 0x00, 0x0e},
  {0x23, 0x21, 0x2d, 0x09, 0xc0, 0x70, 0x07, 0x07, 0x00, 0x01, 0x0c},
  {0x61, 0x21, 0x1b, 0x09, 0x64, 0x65, 0x18, 0x18, 0x00, 0x00, 0x0c},
  {0x61, 0x61, 0x0c, 0x08, 0x85, 0xa0, 0x79, 0x07, 0x01, 0x01, 0x00},
  {0x23, 0x21, 0x87, 0x09, 0xf0, 0xa4, 0x00, 0xf7, 0x00, 0x01, 0x02},
  {0x97, 0xe1, 0x28, 0x0E, 0xff, 0xf3, 0x02, 0xf8, 0x00, 0x00, 0x0e},
  {0x61, 0x10, 0x0c, 0x08, 0xf2, 0xc4, 0x40, 0xc8, 0x00, 0x00, 0x0a},
  {0x01, 0x01, 0x56, 0x03, 0xb4, 0xb2, 0x23, 0x58, 0x00, 0x00, 0x06},
  {0x61, 0x41, 0x89, 0x09, 0xf1, 0xf4, 0xf0, 0x13, 0x00, 0x00, 0x06},

#endif
};
static unsigned char rhythmtable[3][11] = {
#ifdef PATCH_RHYTHM_OKAZAKI
  /* Drum kit from Mitsutaka Okazaki's emu2413 v0.38 */
  /* Converted yo YM3812 format                      */
  /* with TL correction                              */
  {0x08, 0x21, 0x28, 0x00, 0xdf, 0xf8, 0xff, 0xf8, 0x00, 0x00, 0x00},
/* {0x20, 0x21, 0x00, 0x00, 0xa8, 0xf8, 0xf8, 0xf8, 0x00, 0x00, 0x00}, */
  {0x20, 0x21, 0x00, 0x00, 0xF8, 0xf8, 0xf8, 0xf8, 0x00, 0x00, 0x00},
  {0x35, 0x18, 0x00, 0x00, 0xf7, 0xa9, 0xf7, 0x55, 0x00, 0x00, 0x00},
#endif

#ifdef PATCH_RHYTHM_FRS
  {0x1E, 0x21, 0xE8, 0x00, 0xdf, 0xf9, 0xff, 0xf8, 0x01, 0x00, 0x00},
  {0x01, 0x21, 0x0F, 0x0F, 0xb8, 0xB8, 0xb7, 0xF8, 0x01, 0x00, 0x00},
  {0x35, 0x01, 0x09, 0x8F, 0xF7, 0xf6, 0xF7, 0xd5, 0x00, 0x01, 0x00},

/* {0x35, 0x01, 0x00, 0x82, 0xB8, 0xf6, 0xB5, 0xd5, 0x00, 0x01, 0x00}, */
#endif

#ifdef PATCH_RHYTHM_MAME
    /* MAME Drum kit */
  {0x13, 0x13, 0x25, 0x00, 0xD7, 0xB7, 0xF4, 0xF4, 0x00, 0x00, 0x00},
  {0x13, 0x22, 0x10, 0xD0, 0xD7, 0xB7, 0xF4, 0xF4, 0x00, 0x00, 0x00},
  {0x13, 0x11, 0x00, 0x00, 0xD7, 0xB7, 0xF4, 0xF4, 0x00, 0x00, 0x00},
#endif
};

/* Maps channels to YM3812 operator registers */
static unsigned char ch2op[] = {0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12};


/*--------------------------------------------------------------------------*/

void ym2413_reset (int chip)
{
	int n;
	unsigned char *param = &table[0][0];
#ifdef YM2413_ROM_FILE
	FILE* fym2413rom;
	size_t readsize;
#endif

	/* Point to current YM2413 context */
	t_ym2413 * opll = &ym2413[chip];

	/* Clear channel data context */
	memset (opll, 0, sizeof (t_ym2413));

	/* Clear all YM3812 registers */
	for (n = 0; n < 0xF5; n += 1)
    {
		OPL_WRITE (chip, n, 0x00);
    }

	/* Turn off rhythm mode and key-on bits */
	opll->rhythm = 0;
	OPL_WRITE (chip, 0xBD, 0x00);

	/* Enable waveform select */
	OPL_WRITE (chip, 0x01, 0x20);

	/* Enable OPL3 mode */
	/* OPL_WRITE (chip, 0x105, 0x01); */

	/* Set keyboard split NTS = ON */
	/* TODO: Must check this right... Docs I have conflicts describing this flag */
	OPL_WRITE (chip, 0x08, 0x40);

#ifdef YM2413_ROM_FILE
	/* Load instrument patches from YM2413.ROM, if it exists */
	fym2413rom = fopen( YM2413_ROM_FILE, "rb" );
	if ( fym2413rom != NULL )
	{
		readsize = fread( table, sizeof(table), 1, fym2413rom );
		readsize = fread( rhythmtable, sizeof(rhythmtable), 1, fym2413rom );
		fclose ( fym2413rom );
	}
#endif

	/* Load the initial user patch */
	for (n = 0; n<10; n++ )
		opll->user[n] = param[n];

#if 0
	/* Workaround on MAME's YM3812 digital emu */
	if ( options.use_emulated_ym3812 )
	{
		logerror("YM2413: Emulated YM3812 drumkit volume workaround enabled\n");
		rhythmtable[1][3]+=3;	/* SD  */
		rhythmtable[1][2]+=14;	/* HH  */
		rhythmtable[2][3]+=14;	/* TCY */
	}
#endif
}

void ym2413_write (int chip, int address, int data)
{
	/* Point to current YM2413 context */
	t_ym2413 * opll = &ym2413[chip];
	t_ym3812 * opl2 = &ym3812[chip];

	if (address & 1)		/* Data port */
	{
		/* If the DAC mode is enabled */
		/* update the output buffer before changing the registers */
		if ( (opll->reg[0x0F] & 1) && (opll->latch == 0x10) && opll->reg[0x10] != data )
			stream_update(opll->DAC_stream, 0);

		/* Store register data */
		opll->reg[opll->latch] = data;

		switch (opll->latch & 0x30)
		{
		case 0x00:		/* User instrument registers */
			switch (opll->latch & 0x0F)
			{
			case 0x00:	/* Misc. ctrl. (modulator) */
			case 0x01:	/* Misc. ctrl. (carrier) */
			case 0x02:	/* Key scale level and total level (modulator) */
			case 0x04:	/* Attack / Decay (modulator) */
			case 0x05:	/* Attack / Decay (carrier) */
			case 0x06:	/* Sustain / Release (modulator) */
			case 0x07:	/* Sustain / Release (carrier) */
				opll->user[(opll->latch & 0x07)] = data;
				break;

			case 0x03:	/* Key scale level, carrier/modulator waveform, feedback */
				/* Key scale level (carrier) */
				/* Don't touch the total level */
				opll->user[3] = (opll->user[3] & 0x3F) | (data & 0xC0);

				/* Waveform select for the modulator */
				opll->user[8] = (data >> 3) & 1;

				/* Waveform select for the carrier */
				opll->user[9] = (data >> 4) & 1;

				/* Store feedback level in YM3812 format */
				opll->user[10] = ((data & 0x07) << 1) & 0x0E;

				if( data & 0x02 )
					logerror("YM2413: Warning! Undocumented REG=03 bit5 is on. Not implemented.");
				break;

			case 0x08:
			case 0x09:
			case 0x0A:
			case 0x0B:
			case 0x0C:
			case 0x0D:
				logerror("YM2413: Error! Invalid register %d\n", opll->latch);
				break;

			case 0x0E:	/* Rhythm enable and key-on bits */
				OPL_WRITE (chip, 0xBD, (data & 0x3F));
				if ((data & 0x20) && (opll->rhythm == 0))
				{
					opll->rhythm = 1;
					rhythm_mode_init (chip);
					break;
				}

				if ( ((data & 0x20) == 0) && (opll->rhythm == 1))
				{
					int x;
					opll->rhythm = 0;

					/* Restores instruments in this channels */
					for (x = 6; x < 9; x++)
					load_instrument (chip, x, opll->channel[x].instrument, opll->channel[x].volume);
				}

				if( data & 0xC0 ) /* Check for unknown bits */
					logerror("YM2413: Warning! Undocumented REG=0E value=%02x. Not implemented.", data);

				break;

			case 0x0F:  /* Test Register */
				/* log anything wrote to this register, since we don't know
				when ones ore zeroes have effect. This will log also when DAC
				mode is activated, but you can just ignore it. (^_^)  */
				logerror("YM2413: Warning! Test REG=0F write, value=%02x. Only bits 0 and 3 implemented\n", data);
				break;
			} /* case 0x00-0x0F registers */

			/* If the user instrument registers were accessed, then
			go through each channel and update the ones that were
			currently using the user instrument. We can skip the
			last three channels in rhythm mode since they can
			only use percussion sounds anyways. */
			if (opll->latch <= 0x07)
			{
				static int x;
				for (x = 0; x < ((opll->reg[0x0E] & 0x20) ? 6 : 9); x += 1)
				{
					if (opll->channel[x].instrument == 0x00)
					{
						load_instrument (chip, x, 0x00,
						                 opll->channel[x].volume);
					}
				}
			}
			break;

		case 0x10:		/* Channel Frequency (LSB) */
		{
			static int block, frequency;
			static int ch;

			/* Return if the DAC mode is enabled */
			if ((opll->latch == 0x10) && (opll->reg[0x0F] & 1))
				break;

			ch = (opll->latch & 0x0F);

			/* Ensure proper channel range */
			if (ch > 0x08)
			{
				logerror ("YM2413: Error! Channel to big: reg=%d ch=%d\n",
				         opll->latch, ch);
				break;
			}

			/* Get YM2413 channel frequency */
			frequency = ((opll->reg[0x10 + ch] & 0xFF) |
			            ((opll->reg[0x20 + ch] & 0x01) << 8));

			/* Get YM2413 block */
			block = (opll->reg[0x20 + ch] >> 1) & 7;

#if VERBOSE
			if ((ch >= 6) && (opll->reg[0x0E] & 0x20))
			{
				logerror("YM2413: rhythm: ch=%d, setting F-number=%02x\n",
				        ch, data);
			}
#endif

			/* Scale 9 bit frequency to 10 bits */
			frequency = (frequency << 1) & 0x1FFF;

			/* Add in block and KEY-ON/OFF flag, since we'll write */
			/* both registers of YM3812 because of the <<1 above */
			frequency = frequency | (block << 10) |
			            (opll->reg[0x20 + ch]<<9);

			/* Save current frequency/block/key-on_off setting */
			opll->channel[ch].frequency = (frequency & 0x3FFF);

			/* Write changes to YM3812 */
			OPL_WRITE (chip, 0xA0 + ch,
			          (opll->channel[ch].frequency >> 0) & 0xFF);
			OPL_WRITE (chip, 0xB0 + ch,
			          (opll->channel[ch].frequency >> 8) & 0xFF);
			break;
		}
		case 0x20: /* Channel Frequency (MSB) + keyon/off and sustain control */
		{
			static unsigned char RRselect;
			static int y;

			int ch = (opll->latch & 0x0F);
			unsigned char *param = (opll->channel[ch].instrument == 0) ?
			                       &opll->user[0] :
			                       &table[opll->channel[ch].instrument][0];

#if VERBOSE
			if ((ch >= 6) && (opll->reg[0x0E] & 0x20))
				logerror("YM2413: rhythm: ch=%d, setting block=%02x F-num MSB=%d\n",
					ch, (data>>1)&7, data&1 );

			if( data & 0xC0 )
				logerror( "YM2413: Warning! Set undocumented bit in REG=%02x value=%02x. Not implemented.",
				        opll->latch, data);
#endif

			/* Frequency/block/KEY handling */
			opll->channel[ch].frequency = (opll->channel[ch].frequency &
			                              0x1FF) | ( (data & 0x1F) <<9);

#if VERBOSE
			logerror("YM2413: ch=%d, data=%02x, SLRRc=%02x\n",
				ch, data, opl2->channel[ch].SLRRc );
#endif

			/* YM2413 does have a bunch of automatic functions, but other
			   OPL chips doesn't. Here is reproduced the YM2413 behaviour on
			   auto-hanling the channel's Release Rates based on the
			   EG-Type, SUS and KEY status. All other YM2413->YM3812
			   translators just ignored this important aspect, and this is
			   one of the biggest reasons them all failed to sound like a
			   real YM2413. */
			for( y=0 ; y<2 ; y++ ) /* Adjust RR for carrier and modulator */
			{
				RRselect = ((param[y]&0x20)>>1) | ((data&0xF0)>>2) | opll->channel[ch].oldSUSKEY;
				switch ( RRselect )
				{
				case 0x00: /*EGTYP=0 SUS=off KEY=off oldSUS=off oldKEY=off*/
					break;
				case 0x01: /*EGTYP=0 SUS=off KEY=off oldSUS=off oldKEY=on */
				case 0x03: /*EGTYP=0 SUS=off KEY=off oldSUS=on  oldKEY=on */
					OPL_WRITE (chip, 0x80+3*y + ch2op[ch],
					          ((opl2->channel[ch].SLRRc & 0xF0) | 7));
					break;
				case 0x02: /*EGTYP=0 SUS=off KEY=off oldSUS=on  oldKEY=off*/
				case 0x06: /*EGTYP=0 SUS=off KEY=on  oldSUS=on  oldKEY=off*/
				case 0x07: /*EGTYP=0 SUS=off KEY=on  oldSUS=on  oldKEY=on */
					OPL_WRITE (chip, 0x80+3*y + ch2op[ch],
					          opl2->channel[ch].SLRRc);
					break;
				case 0x04: /*EGTYP=0 SUS=off KEY=on  oldSUS=off oldKEY=off*/
				case 0x05: /*EGTYP=0 SUS=off KEY=on  oldSUS=off oldKEY=on */
					OPL_WRITE (chip, 0x80+3*y + ch2op[ch],
					          opl2->channel[ch].SLRRc);
					break;
				case 0x08: /*EGTYP=0 SUS=on  KEY=off oldSUS=off oldKEY=off*/
					OPL_WRITE (chip, 0x80+3*y + ch2op[ch],
					          ((opl2->channel[ch].SLRRc & 0xF0) | 5));
				case 0x09: /*EGTYP=0 SUS=on  KEY=off oldSUS=off oldKEY=on */
				case 0x0B: /*EGTYP=0 SUS=on  KEY=off oldSUS=on  oldKEY=on */
					OPL_WRITE (chip, 0x80+3*y + ch2op[ch],
					          ((opl2->channel[ch].SLRRc & 0xF0) | 5));
				case 0x0A: /*EGTYP=0 SUS=on  KEY=off oldSUS=on  oldKEY=off*/
				case 0x0D: /*EGTYP=0 SUS=on  KEY=on  oldSUS=off oldKEY=on */
					break;
				case 0x0C: /*EGTYP=0 SUS=on  KEY=on  oldSUS=off oldKEY=off*/
				case 0x0E: /*EGTYP=0 SUS=on  KEY=on  oldSUS=on  oldKEY=off*/
					OPL_WRITE (chip, 0x80+3*y + ch2op[ch],
					          opl2->channel[ch].SLRRc);
					break;
				case 0x0F: /*EGTYP=0 SUS=on  KEY=on  oldSUS=on  oldKEY=on */
					break;
				} /* switch RRselect */
			} /* for {carrier,modulator} */

			/* store old SUS and KEY  */
			opll->channel[ch].oldSUSKEY = ((RRselect>>2)&3);

			/* Write changes to YM3812 */
			OPL_WRITE (chip, 0xB0 + ch,
			          (opll->channel[ch].frequency >> 8) & 0xFF);
		}
		break;

		case 0x30:	/* Channel Volume Level and Instrument Select */
		{
			int ch = (opll->latch & 0x0F);

			if (ch > 8)  /* Ensure proper channel range */
			{
				logerror ("YM2413: Error! Channel to big: reg=%d ch=%d\n",
			opll->latch, ch);
				break;
			}

			/* If we're accessing channels 7, 8, or 9, and we're
			   in rhythm mode, then update the individual volume
			   settings. Obs: channel 7 = ch6 */

			if ((ch >= 6) && (opll->reg[0x0E] & 0x20))
			{
				logerror("YM2413: volume = battery, ch=%d\n", ch);
					rhythm_volume (chip, ch);
			}
			else  /* Set the new instrument and volume for this channel */
			{
				int inst = (data >> 4) & 0x0F;
				int vol = (data & 0x0F)<<2;
				load_instrument (chip, ch, inst, vol);
			}
			break;
		} /* case 0x30 */
		} /* switch (opll->latch & 0x30) */
	}
	else	/* Register latch */
	{
		opll->latch = (data & 0x3F);
	}
}

static void rhythm_mode_init (int chip)
{
	/* Point to current YM2413 and YM3812 context */
	t_ym2413 * opll = &ym2413[chip];
	t_ym3812 * opl2 = &ym3812[chip];

	/* Operator offset from requested channel */
	int op, ch;
	unsigned char *param;

	logerror("YM2413: Rhythm mode init\n");

	/* Load instrument settings for channel seven. (Bass drum) */
	ch = 6;
	op = ch2op[ch];
	param = &rhythmtable[0][0];

	OPL_WRITE (chip, 0x20 + op, param[0]);
	OPL_WRITE (chip, 0x23 + op, param[1]);
	OPL_WRITE (chip, 0x40 + op, param[2]);       /* Bass Drum has TLm */
	opl2->channel[ch].KSLm = (param[2] & 0xC0);
	opl2->channel[ch].KSLc = (param[3] & 0xC0);
	rhythm_volume( chip, ch);
	OPL_WRITE (chip, 0x60 + op, param[4]);
	OPL_WRITE (chip, 0x63 + op, param[5]);
	OPL_WRITE (chip, 0x80 + op, param[6]);
	opl2->channel[ch].SLRRc = param[7];
	OPL_WRITE (chip, 0x83 + op, param[7]);
	OPL_WRITE (chip, 0xE0 + op, param[8]);
	OPL_WRITE (chip, 0xE3 + op, param[9]);
	OPL_WRITE (chip, 0xC0 + ch, param[10]);

	/* Use old frequency, but strip key-on bit */
	OPL_WRITE (chip, 0xA0 + ch, (opll->channel[6].frequency >> 0) & 0xFF);
	OPL_WRITE (chip, 0xB0 + ch, (opll->channel[6].frequency >> 8) & 0x1F);

	/* Load instrument settings for channel eight. (High hat and snare drum) */
	ch = 7;
	op = ch2op[ch];
	param = &rhythmtable[1][0];

	/* Argh! On OPL3, the first parameter seems swapped!!! */
	OPL_WRITE (chip, 0x20 + op, param[1]);
	OPL_WRITE (chip, 0x23 + op, param[0]);

	OPL_WRITE (chip, 0x40 + op, (param[2] & 0xC0) );
	opl2->channel[ch].KSLm = (param[2] & 0xC0);
	OPL_WRITE (chip, 0x43 + op, (param[3] & 0xC0) );
	opl2->channel[ch].KSLc = (param[3] & 0xC0);
	rhythm_volume( chip, ch);
	OPL_WRITE (chip, 0x60 + op, param[4]);
	OPL_WRITE (chip, 0x63 + op, param[5]);
	OPL_WRITE (chip, 0x80 + op, param[6]);
	opl2->channel[ch].SLRRc = param[7];
	OPL_WRITE (chip, 0x83 + op, param[7]);
	OPL_WRITE (chip, 0xE0 + op, param[8]);
	OPL_WRITE (chip, 0xE3 + op, param[9]);
	OPL_WRITE (chip, 0xC0 + ch, param[10]);

	/* Use old frequency, but strip key-on bit */
	OPL_WRITE (chip, 0xA0 + ch, (opll->channel[7].frequency >> 0) & 0xFF);
	OPL_WRITE (chip, 0xB0 + ch, (opll->channel[7].frequency >> 8) & 0x1F);

	/* Load instrument settings for channel nine. (Tom-tom and top cymbal) */
	ch = 8;
	op = ch2op[ch];
	param = &rhythmtable[2][0];
	OPL_WRITE (chip, 0x20 + op, param[0]);
	OPL_WRITE (chip, 0x23 + op, param[1]);

	/* OPL_WRITE(chip, 0x40 + op, ((opll->reg[0x38] >> 4) & 0x0F) << 2); */
	/* OPL_WRITE(chip, 0x43 + op, ((opll->reg[0x38] >> 0) & 0x0F) << 2); */
	OPL_WRITE (chip, 0x40 + op, (param[2] & 0xC0) );
	opl2->channel[ch].KSLm = (param[2] & 0xC0);
	OPL_WRITE (chip, 0x43 + op, (param[3] & 0xC0) );
	opl2->channel[ch].KSLc = (param[3] & 0xC0);
	rhythm_volume( chip, ch);
	OPL_WRITE (chip, 0x60 + op, param[4]);
	OPL_WRITE (chip, 0x63 + op, param[5]);
	OPL_WRITE (chip, 0x80 + op, param[6]);
	opl2->channel[ch].SLRRc = param[7];
	OPL_WRITE (chip, 0x83 + op, param[7]);
	OPL_WRITE (chip, 0xE0 + op, param[8]);
	OPL_WRITE (chip, 0xE3 + op, param[9]);
	OPL_WRITE (chip, 0xC0 + ch, param[10]);

	/* Use old frequency, but strip key-on bit */
	OPL_WRITE (chip, 0xA0 + ch, (opll->channel[8].frequency >> 0) & 0xFF);
	OPL_WRITE (chip, 0xB0 + ch, (opll->channel[8].frequency >> 8) & 0x1F);
}


/* channel (0-9), instrument (0-F), volume (0-3F) */
static void load_instrument (int chip, int ch, int inst, int vol)
{
	/* Point to current YM2413 and YM3812 context */
	t_ym2413 * opll = &ym2413[chip];
	t_ym3812 * opl2 = &ym3812[chip];

	/* Point to fixed instrument or user table */
	unsigned char *param = (inst == 0) ? &opll->user[0] : &table[inst][0];

	/* Make operator offset from requested channel */
	int op = ch2op[ch];
	unsigned int TLc_opl2;

	/* Store volume level */
	opll->channel[ch].volume = (vol &= 0x3F);

	/* Calculates the YM3812 Total Level Modulator */
	TLc_opl2 = vol+(63-vol)*(param[3]&0x3F)/63;
	opl2->channel[ch].TLc = (char)TLc_opl2;
	logerror("YM2413: ch=%d inst=%2d TLc_opl2=%d TLc_opll=%2d vol=%2d\n",
	         ch, inst, TLc_opl2, (param[3]&0x3F), vol);

	inst = (inst & 0x0F);

	/* MSX games modifies the volume very constantly, so we must check
	if only the volume is being changed and not the instrument.
	Or else the sound will become "clicky" */

	if( opll->channel[ch].instrument != inst )
	{
		/* Store instrument number */
		opll->channel[ch].instrument = inst;

		/* Update instrument settings, except frequency registers */
		OPL_WRITE (chip, 0x20 + op, param[0]);
		OPL_WRITE (chip, 0x23 + op, param[1]);
		opl2->channel[ch].KSLm = (param[2] & 0xC0);
		OPL_WRITE (chip, 0x40 + op, param[2]);
		opl2->channel[ch].KSLc = (param[3] & 0xC0);
		OPL_WRITE (chip, 0x43 + op,
		           (param[3] & 0xC0) | opl2->channel[ch].TLc );
		OPL_WRITE (chip, 0x60 + op, param[4]);
		OPL_WRITE (chip, 0x63 + op, param[5]);
		OPL_WRITE (chip, 0x80 + op, param[6]);
		opl2->channel[ch].SLRRc = param[7];
		OPL_WRITE (chip, 0x83 + op, param[7]);
		OPL_WRITE (chip, 0xE0 + op, param[8]);
		OPL_WRITE (chip, 0xE3 + op, param[9]);
		OPL_WRITE (chip, 0xC0 + ch, param[10]);
		logerror("YM2413: Instrument change ch=%d instr=%2d vol=%2d\n", ch, inst, vol>>2 );
	}
	else
	{
		if( opll->latch <=7 )
			logerror("YM2413: Userinstr parameter change. ch=%d reg=%02x data=%02x (YM3812 format)\n",
				ch, opll->latch, param[opll->latch] );

		switch ( opll->latch )
		{
		case 0x00:
			OPL_WRITE (chip, 0x20 + op, param[0]);
			break;
		case 0x01:
			OPL_WRITE (chip, 0x23 + op, param[1]);
		case 0x02:
			opl2->channel[ch].KSLm = (param[2] & 0xC0);
			OPL_WRITE (chip, 0x40 + op, param[2]);
			break;
		case 0x03:
			opl2->channel[ch].KSLc = (param[3] & 0xC0);
			OPL_WRITE (chip, 0x43 + op,
				(param[3] & 0xC0) | opl2->channel[ch].TLc );
			break;
		case 0x04:
			OPL_WRITE (chip, 0x60 + op, param[4]);
		case 0x05:
			OPL_WRITE (chip, 0x63 + op, param[5]);
		case 0x06:
			OPL_WRITE (chip, 0x80 + op, param[6]);
			break;
		case 0x07:
			opl2->channel[ch].SLRRc = param[7];
			OPL_WRITE (chip, 0x83 + op, param[7]);
			break;
		case 0x08:
			OPL_WRITE (chip, 0xE0 + op, param[8]);
			break;
		case 0x09:
			OPL_WRITE (chip, 0xE3 + op, param[9]);
			break;
		case 0x0A:
			OPL_WRITE (chip, 0xC0 + ch, param[10]);
			break;
		default:   /* Only volume change */
			OPL_WRITE (chip, 0x43 + op,
			opl2->channel[ch].KSLc | opl2->channel[ch].TLc );
#if VERBOSE
			logerror ("YM2413: Volcache: ch=%d vol=%2d inst=%2d oldinst=%2d ym3812_TLc=%2d \n", ch,
			          opll->channel[ch].volume, inst,
			          opll->channel[ch].instrument, opl2->channel[ch].TLc);
#endif
		} /* switch  */
    } /* else */
}

/* channel (7-9), volumes are get from the YM2413 registers */
static void rhythm_volume(int chip, int ch)
{
	/* Point to current YM2413 and YM3812 context */
	t_ym2413 * opll = &ym2413[chip];
	t_ym3812 * opl2 = &ym3812[chip];
	unsigned char *param;

	/* Make operator offset from requested channel */
	int op = ch2op[ch];
	unsigned int TLc_opl2, TLm_opl2;
	unsigned char volhi;
	unsigned char vollo = opll->channel[ch].volume;

	/* Store volume level */
	vollo = ((opll->reg[0x30 + ch] >> 0) & 0x0F );
	opll->channel[ch].volume = vollo = (vollo<<2); /* +3*(vollo&1); */
	opll->channel[ch].instrument = ((opll->reg[0x30 + ch] >> 4) & 0x0F );
	volhi = opll->channel[ch].instrument;
	volhi = (volhi<<2); /* +3*(volhi&1); */

	param = &rhythmtable[ch-6][0];

	/* Calculates the YM3812 Total Level Modulator */
	TLm_opl2 = volhi+(63-volhi)*(param[2]&0x3F)/63;
	TLc_opl2 = vollo+(63-vollo)*(param[3]&0x3F)/63;
	opl2->channel[ch].TLm = (char)TLm_opl2;
	opl2->channel[ch].TLc = (char)TLc_opl2;
	logerror("YM2413: rhythmvol. ch=%d TLm_opl2=%2d TLm_opll=%2d volhi=%2d\n",
	         ch, TLm_opl2, (param[2]&0x3F), volhi/4);
	logerror("YM2413: rhythmvol. ch=%d TLc_opl2=%2d TLc_opll=%2d vollo=%2d\n",
	         ch, TLc_opl2, (param[3]&0x3F), vollo/4);

#if VERBOSE
	logerror("YM2413: Rhythm volume change: ch=%2d volhi=%2d vollo=%2d, KSLm=%d KSLc=%d\n",
	         ch, opll->channel[ch].instrument,
	         ((opll->reg[0x30 + ch] >> 0) & 0x0F ), opl2->channel[ch].KSLm>>6,
	         opl2->channel[ch].KSLc>>6);
#endif

	/* There's no vol-hi for bass drum */
	if ( ch > 6 )
		OPL_WRITE (chip, 0x40 + op,
		           (opl2->channel[ch].KSLm) | opl2->channel[ch].TLm );

		OPL_WRITE (chip, 0x43 + op,
		           (opl2->channel[ch].KSLc) | opl2->channel[ch].TLc );
}


