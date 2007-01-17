/*
 * Berzerk/Frenzy Soundhardware Driver
 * Copyright Alex Judd 1997/98
 * V1.1 for Mame 0.31 13March98
 *
 * these were the original values but I have adjusted them to take account of the
 * default sample rate of 22050. I guess the original samples were at 24000.
 * case 0 : samplefrequency = 19500;
 * case 1 : samplefrequency = 20800;
 * case 2 : samplefrequency = 22320;
 * case 3 : samplefrequency = 24000;
 * case 4 : samplefrequency = 26000;
 * case 5 : samplefrequency = 28400;
 * case 6 : samplefrequency = 31250;
 * case 7 : samplefrequency = 34700;
 *
 */

#include "driver.h"
#include "includes/berzerk.h"
#include "sound/samples.h"
#include "sound/s14001a.h"

static const char *sample_names[] =
{
	"*berzerk", /* universal samples directory */
	"",
	"01.wav", // "kill"
	"02.wav", // "attack"
	"03.wav", // "charge"
	"04.wav", // "got"
	"05.wav", // "to"
	"06.wav", // "get"
	"",
	"08.wav", // "alert"
	"09.wav", // "detected"
	"10.wav", // "the"
	"11.wav", // "in"
	"12.wav", // "it"
	"",
	"",
	"15.wav", // "humanoid"
	"16.wav", // "coins"
	"17.wav", // "pocket"
	"18.wav", // "intruder"
	"",
	"20.wav", // "escape"
	"21.wav", // "destroy"
	"22.wav", // "must"
	"23.wav", // "not"
	"24.wav", // "chicken"
	"25.wav", // "fight"
	"26.wav", // "like"
	"27.wav", // "a"
	"28.wav", // "robot"
	"",
	"30.wav", // player fire
	"31.wav", // baddie fire
	"32.wav", // kill baddie
	"33.wav", // kill human (real)
	"34.wav", // kill human (cheat)
	0	/* end of array */
};


#define VOICE_PITCH_MODULATION	/* remove this to disable voice pitch modulation */

/* Note: We have the ability to replace the real death sound (33) and the
 * real voices with sample (34) as no good death sample could be obtained.
 */
//#define FAKE_DEATH_SOUND


/* volume and channel controls */
#define VOICE_VOLUME 100
#define SOUND_VOLUME 80
#define SOUND_CHANNEL 1    /* channel for sound effects */
#define VOICE_CHANNEL 5    /* special channel for voice sounds only */
#define DEATH_CHANNEL 6    /* special channel for fake death sound only */

/* berzerk sound */
static int lastnoise = 0;

/* sound controls */
static int berzerknoisemulate = 0;
static int voice_playing;
static int deathsound = 0;          /* trigger for playing collision sound */

static void berzerk_sh_update(int param)
{
	voice_playing = !sample_playing(VOICE_CHANNEL);
	if (deathsound==3 && sample_playing(DEATH_CHANNEL) == 0 && lastnoise != 70)
		deathsound=0;               /* reset death sound */
}


static void berzerk_sh_start(void)
{
	int i;

	S14001A_rst_0_w(0);

	berzerknoisemulate = 1;

	for (i = 0;i < 5;i++)
	{
		if (sample_loaded(i))
			berzerknoisemulate = 0;
	}

	timer_pulse(TIME_IN_HZ(Machine->screen[0].refresh), 0, berzerk_sh_update);
}

WRITE8_HANDLER( berzerk_sound_w )
{
	switch(offset)
	{
	case 0: case 1: case 2: case 3: /*printf("6840 write ignored\n");*/ break;
	case 4: /* Decode message for voice samples */
	      if ((data&0xC0)==0x00) /* select word input */
		{
		  if (!S14001A_bsy_0_r()) /* if s14001a is not busy... */
		    {
#if 0
		      mame_printf_debug("not busy, triggering S14001A core with %x\n", data);
		      mame_printf_debug("S14001a word play command: ");
		      switch(data)
			{
			case 0: mame_printf_debug("help\n"); break;
			case 1: mame_printf_debug("kill\n"); break;
			case 2: mame_printf_debug("attack\n"); break;
			case 3: mame_printf_debug("charge\n"); break;
			case 4: mame_printf_debug("got\n"); break;
			case 5: mame_printf_debug("shoot\n"); break;
			case 6: mame_printf_debug("get\n"); break;
			case 7: mame_printf_debug("is\n"); break;
			case 8: mame_printf_debug("alert\n"); break;
			case 9: mame_printf_debug("detected\n"); break;
			case 10: mame_printf_debug("the\n"); break;
			case 11: mame_printf_debug("in\n"); break;
			case 12: mame_printf_debug("it\n"); break;
			case 13: mame_printf_debug("there\n"); break;
			case 14: mame_printf_debug("where\n"); break;
			case 15: mame_printf_debug("humanoid\n"); break;
			case 16: mame_printf_debug("coins\n"); break;
			case 17: mame_printf_debug("pocket\n"); break;
			case 18: mame_printf_debug("intruder\n"); break;
			case 19: mame_printf_debug("no\n"); break;
			case 20: mame_printf_debug("escape\n"); break;
			case 21: mame_printf_debug("destroy\n"); break;
			case 22: mame_printf_debug("must\n"); break;
			case 23: mame_printf_debug("not\n"); break;
			case 24: mame_printf_debug("chicken\n"); break;
			case 25: mame_printf_debug("fight\n"); break;
			case 26: mame_printf_debug("like\n"); break;
			case 27: mame_printf_debug("a\n"); break;
			case 28: mame_printf_debug("robot\n"); break;
			default: mame_printf_debug("ERROR: data %2x; you should NOT see this!\n", data); break;
			}
#endif
		      S14001A_reg_0_w(data & 0x3f);
		      S14001A_rst_0_w(1);
		      S14001A_rst_0_w(0);
		      break;
		    }
//        else { printf("S14001A busy, ignoring write\n"); break; }
		}
	      else if ((data&0xC0)==0x40) /* VSU-1000 control write */
		{
		  /* volume and frequency control goes here */
		  mame_printf_debug("TODO: VSU-1000 Control write (ignored for now)\n");
		  break;
		}
	      else { mame_printf_debug("bogus write ignored\n"); break; } /* vsu-1000 ignores these writes entirely */
	case 5: /*printf("6840 write ignored\n");*/ break;
	case 6: /*printf("SB-1000 control write ignored\n");*/ break;
	case 7: /*printf("6840 write ignored\n");*/ break;
	default: mame_printf_debug("you should never see this/n"); break;
	}
}

READ8_HANDLER( berzerk_sound_r )
{
	if ( offset == 4 && !S14001A_bsy_0_r() )
	{
		return 0x40;
	}
	return 0x00;
}


struct Samplesinterface berzerk_samples_interface =
{
	8,	/* 8 channels */
	sample_names,
	berzerk_sh_start
};
