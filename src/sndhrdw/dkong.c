#include "driver.h"
#include "pokey.h"



#define UPDATES_PER_SECOND 60
#define emulation_rate 11025
#define buffer_len 69000

char samp_buf[25][buffer_len];
int buf_size[25];

int dkong_sh_init(const char *gamename)
{
    FILE *infile;
	int x;
	static char *names[] =
	{
	   "walk.sam",     "jump.sam",     "boom.sam",     "coin.sam",
	   "drop.sam",     "prize.sam",    "",             "",
	   "",             "intro.sam",    "howhigh.sam",  "time.sam",
	   "hammer.sam",   "rivet2a.sam",  "hamhit.sam",   "lvl1end.sam",
	   "back1.sam",    "",             "back3.sam",    "back2.sam",
	   "rivet1a.sam",  "",             "rivet1.sam",   "gorilla.sam",
	   "death.sam"
	};

    for (x=0; x<25; x++)
	{
	   buf_size[x] = 0;
	   if (names[x][0] != 0)
	   {
		   char buf[32];

		   sprintf(buf,"%s/%s",gamename,names[x]);
    	  infile = fopen (buf,"rb");

          if (infile)
		  {
		     buf_size[x] = fread (samp_buf[x], 1, buffer_len, infile);
	      	 fclose (infile);
	      }
	   }
	}

	return 0;
}


void dkong_sh1_w(int offset,int data)
{
   static state[8];

   if (state[offset] != data)
   {
      if ((buf_size[offset] != 0) && (data))
	  {
         osd_play_sample(offset,samp_buf[offset],
            buf_size[offset],emulation_rate, 255,0);
	  }
	  state[offset] = data;
   }
}


void dkong_sh3_w(int offset,int data)
{
   static last_dead;

      if (last_dead != data)
      {
         last_dead = data;
		 if (data)
		 {
		    osd_play_sample(0,samp_buf[24],buf_size[24],emulation_rate,255,0);
		 	osd_stop_sample(1);		  /* kill other samples */
		 	osd_stop_sample(2);
		 	osd_stop_sample(3);
		 	osd_stop_sample(4);
		 	osd_stop_sample(5);
		 }
      }
}


void dkong_sh2_w(int offset,int data)
{
   static last;

   if (last != data)
   {
      switch (data)
      {
         case 8:		  /* these samples repeat */
		 case 9:
		 case 10:
		 case 11:
		 case 4:
		    if (buf_size[data+8] != 0)
			{
              osd_play_sample(4,samp_buf[data+8],
                 buf_size[data+8],emulation_rate,255,1);
	     	}
	     break;

	  	 default:		  /* the rest do not */
		    if (data != 0)
			{
			   if (buf_size[data+8] == 0)
			   {
			      /*show (data,data);	  for debug */
			   }
			   else
			   {
			      osd_play_sample(4,samp_buf[data+8],
				     buf_size[data+8],emulation_rate,255,0);
			   }
			}
         break;
	  }
      last = data;
   }
}



void dkong_sh_update(void)
{
}
