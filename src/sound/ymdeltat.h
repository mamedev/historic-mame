#ifndef __YMDELTAT_H_
#define __YMDELTAT_H_

#define YM_DELTAT_SHIFT    (16)

/* adpcm type A and type B struct */
typedef struct deltat_adpcm_state {
	unsigned char *memory;
	int memory_size;
	double freqbase;
	int *output_pointer;
	int output_range;

	unsigned char reg[16];
	int portstate, portcontrol, portshift;

	unsigned char flag;          /* port state        */
	unsigned char flagMask;      /* arrived flag mask */
	unsigned char now_data;
	unsigned int now_addr;
	unsigned int now_step;
	unsigned int step;
	unsigned int start;
	unsigned int end;
	unsigned int delta;
	int volume;
	int *pan;     /* &output_pointer[pan] */
	int /*adpcmm,*/ adpcmx, adpcmd;
	int adpcml;			/* hiro-shi!! */

	/* leveling and re-sampling state for DELTA-T */
	int volume_w_step;   /* volume with step rate */
	int next_leveling;   /* leveling value        */
	int sample_step;     /* step of re-sampling   */
}YM_DELTAT;

/* static state */
extern unsigned char *ym_deltat_memory;       /* memory pointer */
extern unsigned char ym_deltat_arrived_flag;  /* arrived flag   */

/* before YM_DELTAT_ADPCM_CALC(YM_DELTAT *DELTAT); */
#define YM_DELTAT_DECODE_PRESET(DELTAT) {ym_deltat_memory = DELTAT->memory;}

void YM_DELTAT_ADPCM_Write(YM_DELTAT *DELTAT,int r,int v);
void YM_DELTAT_ADPCM_Reset(YM_DELTAT *DELTAT,int pan);

/* INLINE void YM_DELTAT_ADPCM_CALC(YM_DELTAT *DELTAT); */
#define YM_INLINE_BLOCK
#include "ymdeltat.c" /* include inline function section */
#undef  YM_INLINE_BLOCK

#endif
