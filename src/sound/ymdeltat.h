#ifndef __YMDELTAT_H_
#define __YMDELTAT_H_

#define YM_DELTAT_SHIFT    (16)

/* DELTA-T (adpcm type B) struct */
typedef struct deltat_adpcm_state {     /* AT: rearranged and tigntened structure */
	UINT8 *memory;
	UINT8 *arrivedFlagPtr;	/* pointer of arrived end address flag	*/
	INT32 *output_pointer;	/* pointer of output pointers			*/
	INT32 *pan;				/* pan : &output_pointer[pan]			*/
	double freqbase;
	int memory_size;
	int output_range;
	int portshift;			/* address shift bits	*/
	UINT32 now_addr;		/* current address		*/
	UINT32 now_step;		/* currect step			*/
	UINT32 step;			/* step					*/
	UINT32 start;			/* start address		*/
	UINT32 end;				/* end address			*/
	UINT32 delta;			/* delta scale			*/
	INT32 volume;			/* current volume		*/
	INT32 acc;				/* shift Measurement value	*/
	INT32 adpcmd;			/* next Forecast		*/
	INT32 adpcml;			/* current value		*/

	/* leveling and re-sampling state for DELTA-T */
	INT32 prev_acc;			/* leveling value		*/

	/* external flag control (for YM2610) */
	UINT8 reg[16];			/* adpcm registers		*/
	UINT8 statusflag;		/* EOS flag bit value	*/
	UINT8 now_data;			/* current rom data		*/
	UINT8 portstate;		/* port status: stop==0	*/
	UINT8 eos;      		/* AT: added EOS flag	*/
}YM_DELTAT;

/* static state */
extern UINT8 *ym_deltat_memory;       /* memory pointer */


#define YM_DELTAT_DECODE_PRESET(DELTAT) {ym_deltat_memory = DELTAT->memory;}

void YM_DELTAT_ADPCM_Write(YM_DELTAT *DELTAT,int r,int v);
void YM_DELTAT_ADPCM_Reset(YM_DELTAT *DELTAT,int pan);
void YM_DELTAT_postload(YM_DELTAT *DELTAT,UINT8 *regs);
void YM_DELTAT_savestate(const char *statename,int num,YM_DELTAT *DELTAT);


#define YM_INLINE_BLOCK
#include "ymdeltat.c" /* include inline function section */
#undef  YM_INLINE_BLOCK

#endif
