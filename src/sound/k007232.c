/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/


#include "driver.h"
#include <math.h>


#define  KDAC_A_PCM_MAX    (2)


typedef struct kdacApcm
{
	unsigned char vol[KDAC_A_PCM_MAX];
	unsigned int  addr[KDAC_A_PCM_MAX];
	unsigned int  start[KDAC_A_PCM_MAX];
	unsigned int  step[KDAC_A_PCM_MAX];
	int play[KDAC_A_PCM_MAX];
	int loop[KDAC_A_PCM_MAX];

	unsigned char wreg[0x10]; /* write data */
} KDAC_A_PCM;

static KDAC_A_PCM    kpcm;

static int pcm_chan;

static unsigned char *pcmbuf[2];
static unsigned int  pcm_limit;


#define   BASE_SHIFT    (12)



int kdac_note[] = {
  261.63/8, 277.18/8,
  293.67/8, 311.13/8,
  329.63/8,
  349.23/8, 369.99/8,
  392.00/8, 415.31/8,
  440.00/8, 466.16/8,
  493.88/8,

  523.25/8,
};

static float kdaca_fn[][2] = {
  /* B */
  { 0x03f, 493.88/8 },		/* ?? */
  { 0x11f, 493.88/4 },		/* ?? */
  { 0x18f, 493.88/2 },		/* ?? */
  { 0x1c7, 493.88   },
  { 0x1e3, 493.88*2 },
  { 0x1f1, 493.88*4 },		/* ?? */
  { 0x1f8, 493.88*8 },		/* ?? */
  /* A+ */
  { 0x020, 466.16/8 },		/* ?? */
  { 0x110, 466.16/4 },		/* ?? */
  { 0x188, 466.16/2 },
  { 0x1c4, 466.16   },
  { 0x1e2, 466.16*2 },
  { 0x1f1, 466.16*4 },		/* ?? */
  { 0x1f8, 466.16*8 },		/* ?? */
  /* A */
  { 0x000, 440.00/8 },		/* ?? */
  { 0x100, 440.00/4 },		/* ?? */
  { 0x180, 440.00/2 },
  { 0x1c0, 440.00   },
  { 0x1e0, 440.00*2 },
  { 0x1f0, 440.00*4 },		/* ?? */
  { 0x1f8, 440.00*8 },		/* ?? */
  { 0x1fc, 440.00*16},		/* ?? */
  { 0x1fe, 440.00*32},		/* ?? */
  { 0x1ff, 440.00*64},		/* ?? */
  /* G+ */
  { 0x0f2, 415.31/4 },
  { 0x179, 415.31/2 },
  { 0x1bc, 415.31   },
  { 0x1de, 415.31*2 },
  { 0x1ef, 415.31*4 },		/* ?? */
  { 0x1f7, 415.31*8 },		/* ?? */
  /* G */
  { 0x0e2, 392.00/4 },
  { 0x171, 392.00/2 },
  { 0x1b8, 392.00   },
  { 0x1dc, 392.00*2 },
  { 0x1ee, 392.00*4 },		/* ?? */
  { 0x1f7, 392.00*8 },		/* ?? */
  /* F+ */
  { 0x0d0, 369.99/4 },		/* ?? */
  { 0x168, 369.99/2 },
  { 0x1b4, 369.99   },
  { 0x1da, 369.99*2 },
  { 0x1ed, 369.99*4 },		/* ?? */
  { 0x1f6, 369.99*8 },		/* ?? */
  /* F */
  { 0x0bf, 349.23/4 },		/* ?? */
  { 0x15f, 349.23/2 },
  { 0x1af, 349.23   },
  { 0x1d7, 349.23*2 },
  { 0x1eb, 349.23*4 },		/* ?? */
  { 0x1f5, 349.23*8 },		/* ?? */
  /* E */
  { 0x0ac, 329.63/4 },
  { 0x155, 329.63/2 },		/* ?? */
  { 0x1ab, 329.63   },
  { 0x1d5, 329.63*2 },
  { 0x1ea, 329.63*4 },		/* ?? */
  { 0x1f4, 329.63*8 },		/* ?? */
  /* D+ */
  { 0x098, 311.13/4 },		/* ?? */
  { 0x14c, 311.13/2 },
  { 0x1a6, 311.13   },
  { 0x1d3, 311.13*2 },
  { 0x1e9, 311.13*4 },		/* ?? */
  { 0x1f4, 311.13*8 },		/* ?? */
  /* D */
  { 0x080, 293.67/4 },		/* ?? */
  { 0x140, 293.67/2 },		/* ?? */
  { 0x1a0, 293.67   },
  { 0x1d0, 293.67*2 },
  { 0x1e8, 293.67*4 },		/* ?? */
  { 0x1f4, 293.67*8 },		/* ?? */
  { 0x1fa, 293.67*16},		/* ?? */
  { 0x1fd, 293.67*32},		/* ?? */
  /* C+ */
  { 0x06d, 277.18/4 },		/* ?? */
  { 0x135, 277.18/2 },		/* ?? */
  { 0x19b, 277.18   },
  { 0x1cd, 277.18*2 },
  { 0x1e6, 277.18*4 },		/* ?? */
  { 0x1f2, 277.18*8 },		/* ?? */
  /* C */
  { 0x054, 261.63/4 },
  { 0x12a, 261.63/2 },
  { 0x195, 261.63   },
  { 0x1ca, 261.63*2 },
  { 0x1e5, 261.63*4 },
  { 0x1f2, 261.63*8 },		/* ?? */

  { -1, -1 },
};
static float fncode[0x200];
/*************************************************************/
void KDAC_A_make_fncode( void ){
  int i, j, k;
  float fn;
#if 0
  for( i = 0; i < 0x200; i++ )  fncode[i] = 0;

  i = 0;
  while( (int)kdaca_fn[i][0] != -1 ){
    fncode[(int)kdaca_fn[i][0]] = kdaca_fn[i][1];
    i++;
  }

  i = j = 0;
  while( i < 0x200 ){
    if( fncode[i] != 0 ){
      if( i != j ){
	fn = (fncode[i] - fncode[j]) / (i - j);
	for( k = 1; k < (i-j); k++ )
	  fncode[k+j] = fncode[j] + fn*k;
	j = i;
      }
    }
    i++;
  }
 #if 0
 	for( i = 0; i < 0x200; i++ )
  if (errorlog) fprintf( errorlog,"fncode[%04x] = %.2f\n", i, fncode[i] );
 #endif

#else
  for( i = 0; i < 0x200; i++ ){
    fncode[i] = (0x200 * 55) / (0x200 - i);
    if (errorlog) fprintf( errorlog,"2 : fncode[%04x] = %.2f\n", i, fncode[i] );
  }

#endif
}


/************************************************/
/*    Konami PCM update                         */
/************************************************/

void KDAC_A_update(int arg, void **buffer, int buffer_len)
{
	int i;
	int sample_bits;
	signed char *buf8[2];
	signed short *buf16[2];

	sample_bits = arg >> 8;

	buf8[0] = (signed char *)buffer[0];
	buf8[1] = (signed char *)buffer[1];
	buf16[0] = (signed short *)buffer[0];
	buf16[1] = (signed short *)buffer[1];

	for( i = 0; i < KDAC_A_PCM_MAX; i++ )
	{
		if (kpcm.play[i])
		{
			int vol,j;
			unsigned int addr, old_addr;

			/**** PCM setup ****/
			addr = kpcm.start[i] + ((kpcm.addr[i]>>BASE_SHIFT)&0x000fffff);
			vol = kpcm.vol[i] * 0x22;
			for( j = 0; j < buffer_len; j++ )
			{
				old_addr = addr;
				addr = kpcm.start[i] + ((kpcm.addr[i]>>BASE_SHIFT)&0x000fffff);
				while (old_addr <= addr)
				{
					if (pcmbuf[i][old_addr] & 0x80)
					{
						/* end of sample */

						if (kpcm.loop[i])
						{
							/* loop to the beginning */
							addr = kpcm.start[i];
							kpcm.addr[i] = 0;
						}
						else
						{
							/* stop sample */
							kpcm.play[i] = 0;
						}
						break;
					}

					old_addr++;
				}

				if (kpcm.play[i] == 0)
				{
					if (sample_bits == 8)
						memset(buf8[i],0,(buffer_len - j) * sizeof(signed char));
					else
						memset(buf16[i],0,(buffer_len - j) * sizeof(signed short));
					break;
				}

				kpcm.addr[i] += kpcm.step[i];

				if (sample_bits == 8)
					buf8[i][j] = (((pcmbuf[i][addr] & 0x7f) - 0x40) * vol) >> 8;
				else
					buf16[i][j] = (((pcmbuf[i][addr] & 0x7f) - 0x40) * vol);
			}
		}
		else
		{
			if (sample_bits == 8)
				memset(buf8[i],0,buffer_len * sizeof(signed char));
			else
				memset(buf16[i],0,buffer_len * sizeof(signed short));
		}
	}
}

/************************************************/
/*    Konami PCM start                          */
/************************************************/
int K007232_sh_start( struct K007232_interface *intf )
{
	int i;
	char buf[2][40];
	const char *name[2];

	pcmbuf[0] = (unsigned char *)Machine->memory_region[intf->bankA];
	pcmbuf[1] = (unsigned char *)Machine->memory_region[intf->bankB];
	if( !intf->limit )    pcm_limit = 0x00020000;	/* default limit */
	else                  pcm_limit = intf->limit;

	for( i = 0; i < KDAC_A_PCM_MAX; i++ )
	{
		kpcm.vol[i] = 0;
		kpcm.start[i] = 0;
		kpcm.step[i] = 0;
		kpcm.play[i] = 0;
		kpcm.loop[i] = 0;
	}
	for( i = 0; i < 0x10; i++ )  kpcm.wreg[i] = 0;

	for (i = 0;i < 2;i++)
	{
		name[i] = buf[i];
		sprintf(buf[i],"Konami 007232 Ch %c",'A'+i);
	}
	pcm_chan = stream_init_multi(2,name,Machine->sample_rate,
			Machine->sample_bits,Machine->sample_bits << 8,KDAC_A_update);
	stream_set_volume(pcm_chan,intf->volume);
	stream_set_volume(pcm_chan+1,intf->volume);
	KDAC_A_make_fncode();

	return 0;
}

/************************************************/
/*    Konami PCM stop                           */
/************************************************/
void K007232_sh_stop( void )
{
}

void K007232_sh_update( void )
{
}

/************************************************/
/*    Konami PCM write register                 */
/************************************************/
void K007232_WriteReg( int r, int v )
{
	int  i;
	int  data;

	if (Machine->sample_rate == 0) return;

	stream_update(pcm_chan,0);

	kpcm.wreg[r] = v;			/* stock write data */

	if (r == 0x0d)
	{
		/* select if sample plays once or looped */
		kpcm.loop[0] = v & 0x01;
		kpcm.loop[1] = v & 0x02;
		return;
	}
	else if (r == 0x0c)
	{
		/* volume */
		/* volume is externally controlled so it could be anything, but I guess */
		/* this is a standard setup. It's the one used in TMNT. */
		/* the pin is marked BLEV so it could be intended to control only channel B */

		kpcm.vol[0] = (v >> 4) & 0x0f;
		kpcm.vol[1] = (v >> 0) & 0x0f;
		return;
	}
	else
	{
		int  reg_port;

		reg_port = 0;
		if (r >= 0x06)
		{
			reg_port = 1;
			r -= 0x06;
		}

		switch (r)
		{
			case 0x00:
			case 0x01:
				/**** address step ****/
				data = (((((unsigned int)kpcm.wreg[reg_port*0x06 + 0x01])<<8)&0x0100) | (((unsigned int)kpcm.wreg[reg_port*0x06 + 0x00])&0x00ff));
				#if 0
				if( !reg_port && r == 1 )
				if (errorlog) fprintf( errorlog, "%04x\n" ,data );
				#endif

				kpcm.step[reg_port] =
					( (7850.0 / (float)Machine->sample_rate) ) *
					( fncode[data] / (440.00/2) ) *
					( (float)3580000 / (float)4000000 ) *
					(1<<BASE_SHIFT);
				break;

			case 0x02:
			case 0x03:
			case 0x04:
				/**** start address ****/
				kpcm.start[reg_port] =
					((((unsigned int)kpcm.wreg[reg_port*0x06 + 0x04]<<16)&0x00010000) |
					(((unsigned int)kpcm.wreg[reg_port*0x06 + 0x03]<< 8)&0x0000ff00) |
					(((unsigned int)kpcm.wreg[reg_port*0x06 + 0x02]    )&0x000000ff));
			break;
		}
	}
}

/************************************************/
/*    Konami PCM read register                  */
/************************************************/
void check_err_fncode( int data ){
  /**** not make keycode ****/
  if( data < 0x054 )
  {
  	if (errorlog) fprintf( errorlog,"err!! KDAC_A param %04x\n", data );
  }
  else if( data > 0x1e5 )
  {
  	if (errorlog) fprintf( errorlog,"err!! KDAC_A param %04x\n", data );
  }
  else if( data > 0x054 && data < 0x0ac )
  {
  	if (errorlog) fprintf( errorlog,"err!! KDAC_A param %04x\n", data );
  }
  else if( data > 0x0ac && data < 0x0e2 )
  {
  	if (errorlog) fprintf( errorlog,"err!! KDAC_A param %04x\n", data );
  }
  else if( data > 0x0f2 && data < 0x12a )
  {
  	if (errorlog) fprintf( errorlog,"err!! KDAC_A param %04x\n", data );
  }
}

int  K007232_ReadReg( int r )
{
	int  ch = 0;

	if (r == 0x0005)
	{
		if (kpcm.start[0] < pcm_limit)
		{
			kpcm.play[0] = 1;
			kpcm.addr[0] = 0;
			//check_err_fncode( (((((unsigned int)kpcm.wreg[0x01])<<8)&0x0100) | (((unsigned int)kpcm.wreg[0x00])&0x00ff)) );
		}
	}
	else if (r == 0x000b)
	{
		if (kpcm.start[1] < pcm_limit)
		{
			kpcm.play[1] = 1;
			kpcm.addr[1] = 0;
			//check_err_fncode( (((((unsigned int)kpcm.wreg[0x07])<<8)&0x0100) | (((unsigned int)kpcm.wreg[0x06])&0x00ff)) );
		}
	}
	return 0;
}
