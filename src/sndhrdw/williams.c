#include "driver.h"


/* #define HARDWARE_SOUND */

/*  This define is for the few that can plug a real Williams sound
 *  board on the parallel port of their PC.
 *  You will have to recompile MAME as there is no command line option
 *  to enable that feature.
 *
 *
 *      You have to plug it correctly:
 *       Parallel  Sound board (J3)
 *     Pin  2           3
 *     Pin  3           2
 *     Pin  4           5
 *     Pin  5           4
 *     Pin  6           7
 *     Pin  7           6
 *
 *     Pin  18         GND (Any ground pin on the board)
 */



#ifdef HARDWARE_SOUND

/* Put the port of your parallel port here. */

#define Parallel 0x378
#endif

void williams_sh_w(int offset,int data)
{
int fx;
static int Snd49Flag;

#ifdef HARDWARE_SOUND
	outp(Parallel,data);
	return;
#endif

	if (Machine->samples == 0 || Machine->samples->sample[0] == 0) return;

  fx = data & 0x3F;

/*
 To Do:
  -Special numbers:
   39 = play a tune
   42 or 44? play a note on Defender only.
   ?? play a Sinistar voice sample
  -In Splat, the last samples cannot be interrupted (from 57 to 63)
     but there no way to do that here!!!

 Done:
  -In Robotron do not restart the sound 49 when wave change
*/
 if(fx == 49)
  if(Snd49Flag == 1)
   return;

  if(fx == 0x3F){
   if(Snd49Flag == 1)
    return;
    osd_stop_sample(0);
  }else{
	 if (Machine->samples->sample[fx] != 0){
    osd_play_sample(0,Machine->samples->sample[fx]->data,
     Machine->samples->sample[fx]->length,
     Machine->samples->sample[fx]->smpfreq,
     Machine->samples->sample[fx]->volume,0);
    if(fx == 49)
      Snd49Flag = 1;
    else
      Snd49Flag = 0;
   }
  }
}

void williams_sh_update(void)
{
}
