/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/
#ifndef __KDAC_A_H__
#define __KDAC_A_H__

/******************************************/
#define  KDAC_A_PCM_MAX    (2)

typedef signed short KDAC_A_SAMPLE;

typedef struct kdacApcm{
  unsigned char env[KDAC_A_PCM_MAX];
  unsigned char pan[KDAC_A_PCM_MAX];
  unsigned int  addr[KDAC_A_PCM_MAX];
  unsigned int  start[KDAC_A_PCM_MAX];
  unsigned int  step[KDAC_A_PCM_MAX];
  unsigned int  loop[KDAC_A_PCM_MAX];
  int pcmx[2][KDAC_A_PCM_MAX];
  int flag[KDAC_A_PCM_MAX];

  unsigned char wreg[0x10]; /* write data */
} KDAC_A_PCM;

struct K007232_interface {
  int  bank;
  int volume;
  int limit;

};


/******************************************/
int K007232_sh_start( struct K007232_interface *intf );
void K007232_sh_stop( void );
void K007232_sh_update( void );
void K007232_WriteReg( int r, int v );
int  K007232_ReadReg( int r );

#endif
/**************** end of file ****************/
