/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/
/* ported from the mmsnd src to be stream friendly and work with */
/* newer versions of mame... 									 */

/* to do:*/
/* supposedly this chip has a stereo mode... however since every */
/* game known to support it (so far) is mono.. there  is no way  */
/* to test it.. so it hasnt been implemented (yet) 				 */


#include "driver.h"
#include <math.h>

INLINE int Limit(int v, int max, int min) { return v > max ? max : (v < min ? min : v); }

enum {
  KD_L_PAN = 0, KD_R_PAN = 1, KD_LR_PAN = 2
};

static KDAC_A_PCM    kpcm;
static int  reg_port;
static int emulation_rate;
int volume;

static int sample_pos, next_sample_pos;
static int pcm_chan;

static int emu_step;
static int emu_addr;

static unsigned char *pcmbuf;
static unsigned int  pcm_limit;


#define   BASE_SHIFT    (12)

KDAC_A_PCM *KDAC_A_GetPointer( void ){
  return &kpcm;
}

#define    KD_ON     (1<<0)
#define    KD_START  (1<<1)

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

void KDAC_A_update(int arg, void *buffer, int buffer_len){
  int i, j, k;
  unsigned int addr, old_addr;
  signed int ld, rd, cen;
  signed int dataL, dataR;
  KDAC_A_SAMPLE *datap = (KDAC_A_SAMPLE*)buffer;

  memset( datap, 0x00, buffer_len * sizeof(KDAC_A_SAMPLE) );
  for( i = 0; i < KDAC_A_PCM_MAX; i++ ){
    if( (kpcm.flag[i]&(KD_START|KD_ON)) == (KD_START|KD_ON) ){
      /**** PCM setup ****/
      addr = kpcm.start[i] + ((kpcm.addr[i]>>BASE_SHIFT)&0x000fffff);
      ld = (kpcm.pan[i]&0x0f);
      rd = ((kpcm.pan[i]>>4)&0x0f);
      cen = 0;
      ld = (kpcm.env[i]&0xff) * (ld + cen);
      rd = (kpcm.env[i]&0xff) * (rd + cen);
      for( j = 0; j < buffer_len; j++ ){
	old_addr = addr;
	addr = kpcm.start[i] + ((kpcm.addr[i]>>BASE_SHIFT)&0x000fffff);
	for(; old_addr <= addr; old_addr++ ){
	  if( (((unsigned int)pcmbuf[old_addr])&0x0080) ){
	    kpcm.flag[i] = 0;
	    break;
	  }
	}
	kpcm.addr[i] += kpcm.step[i];
	if( !kpcm.flag[i] )  break; /* skip PCM */
	kpcm.pcmx[0][i] = ((signed int)(pcmbuf[addr]&0x7f) - 0x40)*ld;
	kpcm.pcmx[1][i] = ((signed int)(pcmbuf[addr]&0x7f) - 0x40)*rd;

	*(datap + j) = (Limit( ((int)*(datap + j) + ((kpcm.pcmx[0][i])>>4)), 32767, -32768 ) +
					Limit( ((int)*(datap + j) + ((kpcm.pcmx[1][i])>>4)), 32767, -32768 )) / 2;
      }
    }
  }
}

/************************************************/
/*    Konami PCM start                          */
/************************************************/
int K007232_sh_start( struct K007232_interface *intf ){
  int i;
  int rate = Machine->sample_rate;
  char name[40] = "Konami 007232";

  pcmbuf = (unsigned char *)Machine->memory_region[intf->bank];
  if( !intf->limit )    pcm_limit = 0x00020000;	/* default limit */
  else                  pcm_limit = intf->limit;

  volume = intf->volume;
  emulation_rate = (rate / Machine->drv->frames_per_second) * Machine->drv->frames_per_second;

  for( i = 0; i < KDAC_A_PCM_MAX; i++ ){
    kpcm.env[i] = 0;
    kpcm.pan[i] = 0xcc;
    kpcm.start[i] = 0;
    kpcm.step[i] = 0;
    kpcm.flag[i] = 0;
  }
  for( i = 0; i < 0x10; i++ )  kpcm.wreg[i] = 0;

  reg_port = 0;

  pcm_chan = stream_init(name, emulation_rate, 16, 0, KDAC_A_update);
  stream_set_volume(pcm_chan, volume);
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
void K007232_WriteReg( int r, int v ){
  int  i;
  int  data;

  kpcm.wreg[r] = v;			/* stock write data */
  /**** set PCM registers ****/
  if( r == 0x000d ){
    /**** reset? (amuse) ****/
    if( v&0x02 )  kpcm.flag[0] = 0;
    if( v&0x01 )  kpcm.flag[1] = 0;
    return;
  } else if( r == 0x000c ){
    /**** volume? (amuse) ****/

    kpcm.env[1] = ((v&0x0f)<<4) ? ((v&0x0f)<<4) : 0x10;
    kpcm.env[0] = ((v&0xf0)) ? ((v&0xf0)) :0x10;
    return;
  }
  reg_port = 0;
  if( r >= 0x0006 ){
    reg_port = 1;
    r -= 0x0006;
  }

  switch( r ){
  case 0x00:
  case 0x01:
    /**** address step ****/
    data = (((((unsigned int)kpcm.wreg[reg_port*0x06 + 0x01])<<8)&0x0100) | (((unsigned int)kpcm.wreg[reg_port*0x06 + 0x00])&0x00ff));
   #if 0
	if( !reg_port && r == 1 )
	if (errorlog) fprintf( errorlog, "%04x\n" ,data );
   #endif

	kpcm.step[reg_port] = (int)(
				(
				 ( (7850.0 / (float)emulation_rate) ) *
				 ( fncode[data] / (440.00/2) ) *
				 ( (float)3580000 / (float)4000000 ) *
				 (1<<BASE_SHIFT)
				 )
				);
    break;
  case 0x02:
  case 0x03:
  case 0x04:
    /**** start address ****/
    kpcm.start[reg_port] =
      ((((unsigned int)kpcm.wreg[reg_port*0x06 + 0x04]<<16)&0x00ff0000) |
       (((unsigned int)kpcm.wreg[reg_port*0x06 + 0x03]<< 8)&0x0000ff00) |
       (((unsigned int)kpcm.wreg[reg_port*0x06 + 0x02]    )&0x000000ff));
    break;
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

int  K007232_ReadReg( int r ){
  int  ch = 0;
  if( r == 0x0005 ){
    if( kpcm.start[0] < pcm_limit ){
      kpcm.flag[0] = KD_START|KD_ON;
      kpcm.addr[0] = 0;
      //check_err_fncode( (((((unsigned int)kpcm.wreg[0x01])<<8)&0x0100) | (((unsigned int)kpcm.wreg[0x00])&0x00ff)) );
    }
  } else if( r == 0x000b ){
    if( kpcm.start[1] < pcm_limit ){
      kpcm.flag[1] = KD_START|KD_ON;
      kpcm.addr[1] = 0;
      //check_err_fncode( (((((unsigned int)kpcm.wreg[0x07])<<8)&0x0100) | (((unsigned int)kpcm.wreg[0x06])&0x00ff)) );
    }
  }
  return 0;
}

/**************** end of file ****************/
