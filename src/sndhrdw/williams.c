#include "driver.h"
#include "sndhrdw\generic.h"
#include "M6808.h"


unsigned char *williams_dac;


#define AUDIO_CONV(A) (A+0x80)

#define TARGET_EMULATION_RATE 44100     /* will be adapted to be a multiple of buffer_len */
static int emulation_rate;
static int buffer_len;
static unsigned char *buffer;
static int sample_pos;

static unsigned char amplitude_DAC;



/* #define HARDWARE_SOUND */

/*  This define is for the few that can plug a real Williams sound
 *  board on the parallel port of their PC.
 *  You will have to recompile MAME as there is no command line option
 *  to enable that feature.
 *
 *
 *      You have to plug it correctly:
 *       Parallel  Sound board (J3)
 *     Pin  2           3
 *     Pin  3           2
 *     Pin  4           5
 *     Pin  5           4
 *     Pin  6           7
 *     Pin  7           6
 *
 *     Pin  18         GND (Any ground pin on the board)
 */



#ifdef HARDWARE_SOUND

/* Put the port of your parallel port here. */

#define Parallel 0x378
#endif


void williams_sh_w(int offset,int data)
{
#ifdef HARDWARE_SOUND
	outp(Parallel,data);
	return;
#endif

	soundlatch_w(0,data);
	cpu_cause_interrupt(1,M6808_INT_IRQ);
}



void williams_digital_out_w(int offset,int data)
{
	int totcycles,leftcycles,newpos;


	*williams_dac = data;

	totcycles = Machine->drv->cpu[1].cpu_clock / Machine->drv->frames_per_second;
	leftcycles = cpu_getfcount();
	newpos = buffer_len * (totcycles-leftcycles) / totcycles;

	while (sample_pos < newpos-1)
	    buffer[sample_pos++] = amplitude_DAC;

    amplitude_DAC=AUDIO_CONV(data);

    buffer[sample_pos++] = amplitude_DAC;
}



int williams_sh_start(void)
{
    buffer_len = TARGET_EMULATION_RATE / Machine->drv->frames_per_second;
    emulation_rate = buffer_len * Machine->drv->frames_per_second;

    if ((buffer = malloc(buffer_len)) == 0)
		return 1;
    memset(buffer,0x80,buffer_len);

    sample_pos = 0;

	return 0;
}

void williams_sh_stop(void)
{
    free(buffer);
}

void williams_sh_update(void)
{
	while (sample_pos < buffer_len)
		buffer[sample_pos++] = amplitude_DAC;

	osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);

	sample_pos=0;
}
