#include "driver.h"


#define MAX_OUTPUT 0x7fff

static int channel[MAX_CVSD];
static unsigned char latch[MAX_CVSD];
static unsigned char VolTable[MAX_CVSD][256];
static unsigned char SignedVolTable[MAX_CVSD][256];

static char current_databit[MAX_CVSD];
static int StepIndex[MAX_CVSD]; /* Index into the step values table */
static char ShiftReg[MAX_CVSD]; /* Shift register values for determining step size */
static int OldTicks[MAX_CVSD];
static int FullOutput[MAX_CVSD]; /* CVSD Output */
static char ByteOutput[MAX_CVSD]; /* CVSD Output (byte sized) */

/*
 * This table, and the update formula, are from Larry Bank, author of CAGE.
 * This is only an approximation to the way the chip works.
 * But it seems to do well so far....
 */
static unsigned char CVSDStepSize[64] = {
	1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31, /* 16 */
	33,34,36,37,39,40,41,42,43,44,45,46,47,48,49,50, /* 16 */
	51,52,53,53,54,54,55,55,55,56,56,56,56,56,57,57, /* 16 */
	57,57,57,57,58,58,58,58,58,59,59,59,59,59,59,59}; /* 16 */


void CVSD_clock_w(int num, int databit)
{
	/* speech clock */
	databit &=1;  /* (databit&0x01) ?  1 : 0*/;
	/* Number of E states since last change */
	/* If has been idle for a while, reset step value*/
	if (cpu_gettotalcycles() - OldTicks[num] > 50000 )
	{
		StepIndex[num] = 0;
		FullOutput[num] = 0;
	}
	if (databit) /* Speech clock changing (active on rising edge) */
	{
		ShiftReg[num]=((ShiftReg[num]<<1)+current_databit[num])&0x07;
		if (current_databit[num])
		{
			FullOutput[num] += CVSDStepSize[StepIndex[num]];
			if (FullOutput[num] > 1023)
				FullOutput[num] = 1023;
		}
		else
		{
			FullOutput[num] -= CVSDStepSize[StepIndex[num]];
			if (FullOutput[num] < -1024)
				FullOutput[num] = -1024;
		}
		/* Check 3-bit shift register for all 0's or all 1's to change step size */
		if (ShiftReg[num] == 7 || ShiftReg[num] == 0) /* Increase step size */
		{
			StepIndex[num]++;
			if (StepIndex[num] > 63)
				StepIndex[num] = 63;
		}
		else /* Decrease step size */
		{
			StepIndex[num]--;
			if (StepIndex[num] == 11)
				StepIndex[num] = 12;
			if (StepIndex[num] < 0)
				StepIndex[num] = 0;
		}
		ByteOutput[num] = ((FullOutput[num] / 8) +128) ;
		OldTicks[num] = cpu_gettotalcycles(); /* In preparation for next time */

		/* update the output buffer before changing the registers */
		if (latch[num] != SignedVolTable[num][(unsigned char) ByteOutput[num]])
			stream_update(channel[num],0);

		latch[num] = VolTable[num][(unsigned char) ByteOutput[num]];
	}
}
void CVSD_digit_w(int num, int databit)
{
	/* speech data */
	databit &= 0x01;
	current_databit[num] = databit; /* Save the current speech data bit */
}
void CVSD_dig_and_clk_w(int num, int databit)
{
	/* speech data */
	databit &= 0x01;
	current_databit[num] = databit; /* Save the current speech data bit */
	CVSD_clock_w(num,1);
}


static void CVSD_update(int num,void *buffer,int length)
{
	memset(buffer,latch[num],length);
}


static void CVSD_set_gain(int num,int gain)
{
	int i;
	int out;


	gain &= 0xff;

	/* build volume table (linear) */
	for (i = 255;i > 0;i--)
	{
		out = (i * (1.0 + (gain / 16))) * MAX_OUTPUT / 255;
		/* limit volume to avoid clipping */
		if (out > MAX_OUTPUT) VolTable[num][i] = MAX_OUTPUT / 256;
		else VolTable[num][i] = out / 256;
	}
	VolTable[num][0] = 0;

	for (i = 0;i < 128;i++)
	{
		SignedVolTable[num][0x80 + i] = VolTable[num][2*i];
		SignedVolTable[num][0x80 - i] = -VolTable[num][2*i];
	}
	SignedVolTable[num][0] = -VolTable[num][255];
}


int CVSD_sh_start(const struct MachineSound *msound)
{
	int i;
	const struct CVSDinterface *intf = msound->sound_interface;


	for (i = 0;i < intf->num;i++)
	{
		char name[40];


		sprintf(name,"CVSD #%d",i);
		channel[i] = stream_init(name,intf->volume[i] & 0xff,Machine->sample_rate,8,
				i,CVSD_update);

		if (channel[i] == -1)
			return 1;

		CVSD_set_gain(i,(intf->volume[i] >> 8) & 0xff);

		latch[i] = 0;
		current_databit[i] = 0;
		StepIndex[i] = 0;
		ShiftReg[i] = 0;
		OldTicks[i] = 0;
		FullOutput[i] = 0;
		ByteOutput[i] = 0;
	}
	return 0;
}
