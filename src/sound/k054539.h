/*********************************************************

	Konami 054539 PCM Sound Chip

*********************************************************/
#ifndef __K054539_H__
#define __K054539_H__

#define MAX_054539 2

struct K054539interface {
	int num;					/* number of chips */
	int clock;					/* clock (usually 48000) */
	int region;					/* memory region of sample ROM(s) */
	void (*irq[MAX_054539])( void );
};


int K054539_sh_start( const struct MachineSound *msound );
void K054539_sh_stop( void );
WRITE_HANDLER( K054539_0_w );
READ_HANDLER( K054539_0_r );
WRITE_HANDLER( K054539_1_w );
READ_HANDLER( K054539_1_r );

#endif /* __K054539_H__ */
