#include "driver.h"
#include "sndhrdw\generic.h"
#include "M6808.h"
#include "dac.h"

unsigned char *williams_dac;

static struct DACinterface DAinterface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

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
	*williams_dac = data;
	DAC_data_w(0,data);
}



int williams_sh_start(void)
{
	if( DAC_sh_start(&DAinterface) ) return 1;
	return 0;
}

void williams_sh_stop(void)
{
	DAC_sh_stop();
}

void williams_sh_update(void)
{
	DAC_sh_update();
}
