/***************************************************************************

  sndhrdw.c

  Functions to emulate the sound hardware of the machine.

***************************************************************************/
#include "driver.h"

int toki_sample_channels[4],channel=0;
unsigned char *toki_samples;

void toki_sh_stop(void)
{
 osd_stop_sample(toki_sample_channels[0]);
 osd_stop_sample(toki_sample_channels[1]);
 osd_stop_sample(toki_sample_channels[2]);
 osd_stop_sample(toki_sample_channels[3]);
 free(toki_samples);
}

int toki_sh_start(void)
{
#if 0
 int i,signalf,value;
 unsigned char *toki_soundrom = &Machine->memory_region[5][0xB500];	 // offset to first sample

 if ((toki_samples = malloc(0x4B00*2)) == 0) return 1;
 InitDecoder();

 for(i = 0; i < 0x4B00; i++)
 {
  value = toki_soundrom[i];

  signalf += DecodeAdpcm (value & 0x0f);
  if (signalf > 2047) signalf = 2047;
  if (signalf < -2047) signalf = -2047;
  toki_samples[i*2] = (signalf / 16);

  signalf += DecodeAdpcm (value >> 4);
  if (signalf > 2047) signalf = 2047;
  if (signalf < -2047) signalf = -2047;
  toki_samples[i*2+1] = (signalf / 16);
 }

 toki_sample_channels[0] = get_play_channels(1);
 toki_sample_channels[1] = get_play_channels(2);
 toki_sample_channels[2] = get_play_channels(3);
 toki_sample_channels[3] = get_play_channels(4);
#endif
 return 0;
}

void toki_soundcommand_w(int offset,int data)
{
#if 0
 unsigned char *toki_soundinfo = &Machine->memory_region[5][0x3000];
 int address,length;

 data = data & 0xFFFF;

 if ((toki_soundinfo[data*20]==7) && (data<128))	// digi-sound
 {
   length  = (toki_soundinfo[data*20+7])<<9;
   address = (toki_soundinfo[data*20+5]-0xB5)<<9;
   osd_play_sample(toki_sample_channels[channel & 3],
   			      &toki_samples[address],
			      length, 6000, 255, 0);
   channel++;
 }
#endif
}
