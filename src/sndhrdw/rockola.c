/* from Andrew Scott (ascott@utkux.utcc.utk.edu) */
#include "driver.h"

static int NoSound0=1;
static int Sound0Offset;
static int Sound0Base;
static int oldSound0Base;
static int NoSound1=1;
static int Sound1Offset;
static int Sound1Base;
static int oldSound1Base;
static unsigned char LastPort1;
static unsigned char LastPort2;



static unsigned char waveform[32] =
{
   0x88, 0x88, 0x88, 0x88, 0xaa, 0xaa, 0xaa, 0xaa,
   0xcc, 0xcc, 0xcc, 0xcc, 0xee, 0xee, 0xee, 0xee,
   0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,
   0x44, 0x44, 0x44, 0x44, 0x66, 0x66, 0x66, 0x66
};



int rockola_sh_start(void)
{
   NoSound0=1;
   Sound0Offset=0x0000;
   Sound0Base=Sound0Offset;
   oldSound0Base=Sound0Base;
   NoSound1=1;
   Sound1Offset=0x0800;
   Sound1Base=Sound1Offset;
   oldSound1Base=Sound1Base;

      osd_play_sample(0, (signed char*)waveform, 32, 1000, 0, 1);
      osd_play_sample(1, (signed char*)waveform, 32, 1000, 0, 1);

        return 0;
}

void rockola_sh_update(void)
{
	static int count;


	/* only update every second call (30 Hz update) */
	count++;
    if (count & 1) return;


        /* play musical tones according to tunes stored in ROM */

     if (!NoSound0 && Machine->memory_region[2][Sound0Offset]!=0xff)
        osd_adjust_sample(0, (32000 / (256-Machine->memory_region[2][Sound0Offset])) * 16, 128);
     else
        osd_adjust_sample(0, 1000, 0);
     Sound0Offset++;

     if (!NoSound1 && Machine->memory_region[2][Sound1Offset]!=0xff)
        osd_adjust_sample(1, (32000 / (256-Machine->memory_region[2][Sound1Offset])) * 16, 128);
     else
        osd_adjust_sample(1, 1000, 0);
     Sound1Offset++;

      /* check for overflow*/
      if (Sound0Offset > Sound0Base + 255)
         Sound0Offset = Sound0Base;

      if (Sound1Offset > Sound1Base + 255)
         Sound1Offset = Sound1Base;
}

void rockola_sound0_w(int offset,int data)
{
        /* select musical tune in ROM based on sound command byte */

      Sound0Base = ((data & 0x07) << 8);
      if (Sound0Base != oldSound0Base)
         {
         oldSound0Base = Sound0Base;
         Sound0Offset = Sound0Base;
         }

      /* play noise samples requested by sound command byte */
    if (Machine->samples!=0 && Machine->samples->sample[0]!=0)
    {
      if (data & 0x20 && !(LastPort1 & 0x20))
         osd_play_sample(4,Machine->samples->sample[0]->data,
                           Machine->samples->sample[0]->length,
                           Machine->samples->sample[0]->smpfreq,
                           Machine->samples->sample[0]->volume,0);
      else if (!(data & 0x20) && LastPort1 & 0x20)
         osd_stop_sample(4);

      if (data & 0x40 && !(LastPort1 & 0x40))
         osd_play_sample(2,Machine->samples->sample[0]->data,
                           Machine->samples->sample[0]->length,
                           Machine->samples->sample[0]->smpfreq,
                           Machine->samples->sample[0]->volume,0);
      else if (!(data & 0x20) && LastPort1 & 0x20)
         osd_stop_sample(2);

      if (data & 0x80 && !(LastPort1 & 0x80))
         {
         osd_play_sample(3,Machine->samples->sample[1]->data,
                           Machine->samples->sample[1]->length,
                           Machine->samples->sample[1]->smpfreq,
                           Machine->samples->sample[1]->volume,0);
         }
    }

      if (data & 0x10)
         {
         NoSound0=0;
         }

      if (data & 0x08)
         {
         NoSound0=1;
         }

   LastPort1 = data;
}

void rockola_sound1_w(int offset,int data)
{
       /* select tune in ROM based on sound command byte */
      Sound1Base = 0x0800 + ((data & 0x07) << 8);
      if (Sound1Base != oldSound1Base)
         {
         oldSound1Base = Sound1Base;
         Sound1Offset = Sound1Base;
         }

      if (data & 0x08)
         {
         NoSound1=0;
         }
      else
         {
         NoSound1=1;
         }

   LastPort2 = data;
}
