#include "driver.h"

static unsigned char tmpbuf[400000];
static unsigned char *samp_buf[48];
static int buf_size[48];

int qbert_sh_init(const char *gamename)
{
    FILE *infile;
    char name[20];
    int i;
    
    for (i=0; i<48; i++) {
	  buf_size[i] = 0;
	  sprintf(name,"%s/fx_%02d",gamename,i);
	  infile = fopen (name,"rb");
	  if (infile) {
		buf_size[i] = fread (tmpbuf, 1, 400000, infile);
		fclose (infile);
		samp_buf[i]=malloc(buf_size[i]);
		if (samp_buf[i]) {
			int j;
			for(j=0;j<buf_size[i];j++)
				samp_buf[i][j]=tmpbuf[j];
		}
	  }
    }

    return 0;
}


void qbert_sh_w(int offset,int data)
{
   int fx= 255-data;

#if 0

/*   ok, emulating the priority of fx's does not seem to work great */
   
   static int current_fx=0;
   static int fx_priority[]={
	0,
	3,3,3,3,3,3,3,3,3,3,3,3,5,3,4,3,
	3,3,3,3,3,5,5,3,3,3,3,3,3,3,3,3,
	15,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
   
   if (fx && fx<48 && buf_size[fx] && samp_buf[fx]) {
	switch (fx) {
		case 44: /* reset */
/* it seems I can't do this, otherwise the fx are cut too early */
/*                      
			osd_stop_sample(0);
*/
			break;
		case 45:
		case 46:
		case 47:
			/* read expansion socket */
			break;
		default:
/* does this test work? it is only correct if the previous fx has not
   finished.... or if a 0 priority (fx #0) is sent between others */
			if (fx_priority[fx] >= fx_priority[current_fx])
			{
			  /*      osd_stop_sample(0);    */
				osd_play_sample(0,samp_buf[fx],buf_size[fx],44100,255,0);

			}
			break;
	}
   }
   if (fx<48) current_fx=fx;


#else

/* so, why not improve over the original machine and play several sounds */
/* simultaneously ?                                                      */
   if (fx && fx<48 && buf_size[fx] && samp_buf[fx])
	switch (fx) {
		case 44: /* reset */
			break;
		case 45:
		case 46:
		case 47:
			/* read expansion socket */
			break;
		case 11: /* noser hop */
		case 23: /* noser fall */
		case 25: /* noser on disk */
		     osd_play_sample(1,samp_buf[fx],buf_size[fx],44100,255,0);
		     break;
		default:
		     osd_play_sample(0,samp_buf[fx],buf_size[fx],44100,255,0);
		     break;
	}
#endif
}

void qbert_sh_update(void)
{
}
