#include "driver.h"
#include <math.h>


#define MAX_CHANNELS 16

static signed char memory[MAX_CHANNELS];
static int r1[MAX_CHANNELS];
static int r2[MAX_CHANNELS];
static int r3[MAX_CHANNELS];
static int c[MAX_CHANNELS];

/*
signal  >--R1--+--R2--+---> amp
               |      |
               C      R3
               |      |
               +------+
               |
              GND
*/

/* R1, R2, R3 in Ohm; C in pF */
/* set C = 0 to disable the filter */
void set_RC_filter(int channel,int R1,int R2,int R3,int C)
{
	r1[channel] = R1;
	r2[channel] = R2;
	r3[channel] = R3;
	c[channel] = C;
}

void apply_RC_filter(int channel,signed char *buf,int len,int sample_rate)
{
	float R1,R2,R3,C;
	float Req;
	int K;
	int i;


	if (c[channel] == 0) return;	/* filter disabled */

	R1 = r1[channel];
	R2 = r2[channel];
	R3 = r3[channel];
	C = (float)c[channel] * 1E-12;	/* convert pF to F */

	/* Cut Frequency = 1/(2*Pi*Req*C) */

	Req = (R1*(R2+R3))/(R1+R2+R3);

	K = 0x10000 * exp(-1 / (Req * C) / sample_rate);

	buf[0] = buf[0] + (memory[channel] - buf[0]) * K / 0x10000;

	for (i = 1;i < len;i++)
		buf[i] = buf[i] + (buf[i-1] - buf[i]) * K / 0x10000;

	memory[channel] = buf[len-1];
}
