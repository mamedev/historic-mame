/* invaders.c ********************************* updated: 1997-04-09 08:46 TT
 *
 * Author      : Tormod Tjaberg
 * Created     : 1997-04-09
 * Description : Sound routines for the 'invaders' games
 *
 * Note:
 * The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 * excellent space invader emulator and converted to signed samples so
 * they would work under SEAL. The port info was also gleaned from
 * his emulator. These sounds should also work on all the invader games.
 *
 * The sounds are generated using output port 3 and 5
 *
 * Port 4:
 * bit 0=UFO  (repeats)       0.raw
 * bit 1=Shot                 1.raw
 * bit 2=Base hit             2.raw
 * bit 3=Invader hit          3.raw
 *
 * Port 5:
 * bit 0=Fleet movement 1     4.raw
 * bit 1=Fleet movement 2     5.raw
 * bit 2=Fleet movement 3     6.raw
 * bit 3=Fleet movement 4     7.raw
 * bit 4=UFO 2                8.raw
 */

#include "driver.h"

#define emulation_rate 11025
#define buffer_len 30000

/* if no files found sound will be zero */
char samp_buf[9][buffer_len];
int buf_size[9];

int invaders_sh_init(const char *gamename)
{
   FILE *infile;
   int x;
   char buf[32];
   static char *names[] =
   {
      "0.raw",     "1.raw",     "2.raw",     "3.raw",
      "4.raw",     "5.raw",    "6.raw",      "7.raw",
      "8.raw"
   };

   for (x = 0; x < 9; x++)
   {
      buf_size[x] = 0;
      if (names[x][0] != 0)
      {
         sprintf(buf, "%s/%s", gamename, names[x]);
         infile = fopen(buf, "rb");

         if (infile)
         {
            buf_size[x] = fread(samp_buf[x], 1, buffer_len, infile);
            fclose(infile);
         }
      }
   }

   return 0;
}


void invaders_sh_port3_w(int offset, int data)
{
   static unsigned char Sound = 0;

   if (data & 0x01 && ~Sound & 0x01)
      osd_play_sample(0, samp_buf[0], buf_size[0], emulation_rate, 255, 1);

   if (~data & 0x01 && Sound & 0x01)
      osd_stop_sample(0);

   if (data & 0x02 && ~Sound & 0x02)
      osd_play_sample(1, samp_buf[1], buf_size[1], emulation_rate, 255, 0);

   if (~data & 0x02 && Sound & 0x02)
      osd_stop_sample(1);

   if (data & 0x04 && ~Sound & 0x04)
      osd_play_sample(2, samp_buf[2], buf_size[2], emulation_rate, 255, 0);

   if (~data & 0x04 && Sound & 0x04)
      osd_stop_sample(2);

   if (data & 0x08 && ~Sound & 0x08)
      osd_play_sample(3, samp_buf[3], buf_size[3], emulation_rate, 255, 0);

   if (~data & 0x08 && Sound & 0x08)
      osd_stop_sample(3);

   Sound = data;
}


void invaders_sh_port5_w(int offset, int data)
{
   static unsigned char Sound = 0;

   if (data & 0x01 && ~Sound & 0x01)
      osd_play_sample(4, samp_buf[4], buf_size[4], emulation_rate, 255, 0);

   if (data & 0x02 && ~Sound & 0x02)
      osd_play_sample(4, samp_buf[5], buf_size[5], emulation_rate, 255, 0);

   if (data & 0x04 && ~Sound & 0x04)
      osd_play_sample(4, samp_buf[6], buf_size[6], emulation_rate, 255, 0);

   if (data & 0x08 && ~Sound & 0x08)
      osd_play_sample(4, samp_buf[7], buf_size[7], emulation_rate, 255, 0);

   if (data & 0x10 && ~Sound & 0x10)
      osd_play_sample(5, samp_buf[8], buf_size[8], emulation_rate, 255, 0);

   if (~data & 0x10 && Sound & 0x10)
      osd_stop_sample(5);

   Sound = data;
}

void invaders_sh_update(void)
{
}

