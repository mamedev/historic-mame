/*

	SCSP (YMF292-F) header
*/

#ifndef _SCSP_H_
#define _SCSP_H_

#ifndef VOL_YM3012
#define YM3012_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))
#endif

#define MAX_SCSP	(2)

struct SCSPinterface 
{
	int num;					/* # of chips to emulate */
	int region[MAX_SCSP]; 				/* region of 512k RAM */
	int roffset[MAX_SCSP];				/* offset in the region */
	int mixing_level[MAX_SCSP];			/* volume */
	void (*irq_callback[MAX_SCSP])(int state);	/* irq callback */
};

int SCSP_sh_start(const struct MachineSound *msound);
void SCSP_sh_stop(void);

// SCSP register access
READ16_HANDLER( SCSP_0_r );
WRITE16_HANDLER( SCSP_0_w );
READ16_HANDLER( SCSP_1_r );
WRITE16_HANDLER( SCSP_1_w );

// MIDI I/O access (used for comms on Model 2/3)
WRITE16_HANDLER( SCSP_MidiIn );
READ16_HANDLER( SCSP_MidiOutR );

#endif
