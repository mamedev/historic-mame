/***************************************************************************

  sn76496.c

  Routines to emulate the Texas Instruments SN76489 and SN76496 programmable
  tone /noise generator.

  Noise emulation is not accurate due to lack of documentation. The noise
  generator uses a shift register with a XOR-feedback network, but the exact
  layout is unknown. It can be set for either period or white noise; again,
  the details are unknown.

***************************************************************************/

#include "driver.h"

//#define DEBUG_PITCH

#define MIN_SLICE 1		/* minimum update step */

#ifdef SIGNED_SAMPLES
	#define MAX_OUTPUT 0x7fff
	#define AUDIO_CONV(A) (A)
#else
	#define MAX_OUTPUT 0xffff
	#define AUDIO_CONV(A) (A)
#endif

#define STEP 0x10000


/* Formulas for noise generator */
/* bit0 = output */

/* noise feedback for white noise mode */
#define FB_WNOISE 0x12000	/* bit15.d(16bits) = bit0(out) ^ bit2 */
//#define FB_WNOISE 0x14000	/* bit15.d(16bits) = bit0(out) ^ bit1 */
//#define FB_WNOISE 0x28000	/* bit16.d(17bits) = bit0(out) ^ bit2 (same to AY-3-8910) */
//#define FB_WNOISE 0x50000	/* bit17.d(18bits) = bit0(out) ^ bit2 */

/* noise feedback for periodic noise mode */
/* it is correct maybe (it was in the Megadrive sound manual) */
#define FB_PNOISE 0x10000	/* 16bit rorate */

/* noise generator start preset (for periodic noise) */
#define NG_PRESET 0x0f35


struct SN76496
{
	int Clock;			/* chip clock in Hz     */
						/* set this before calling SN76496Reset() */

	unsigned int UpdateStep;
	int VolTable[16];	/* volume table         */
	int Register[8];	/* registers */
	int LastRegister;	/* last register written */
	int Volume[4];		/* volume of voice 0-2 and noise */
	unsigned int RNG;		/* noise generator      */
	int NoiseFB;		/* noise feedback mask */
	int Period[4];
	int Count[4];
	int Output[4];
};

static int emulation_rate;
static int buffer_len;
static int sample_16bit;

static struct SN76496interface *intf;

static struct SN76496 sn[MAX_76496];
static void *output_buffer[MAX_76496];
static int sample_pos[MAX_76496];
static int volume[MAX_76496];

static int channel;



static void SN76496Reset(struct SN76496 *R,int rate)
{
	int i;
	double divdr;


	/* the base clock for the tone generators is the chip clock divided by 16; */
	/* for the noise generator, it is clock / 256. */
	/* Here we calculate the number of steps which happen during one sample */
	/* at the given sample rate. No. of events = sample rate / (clock/16). */
	/* STEP is a multiplier used to turn the fraction into a fixed point */
	/* number. */
	R->UpdateStep = ((double)STEP * rate * 16) / R->Clock;

	for (i = 0;i < 4;i++) R->Volume[i] = 0;

	R->LastRegister = 0;
	for (i = 0;i < 8;i+=2)
	{
		R->Register[i] = 0;
		R->Register[i + 1] = 0x0f;	/* volume = 0 */
	}

	for (i = 0;i < 4;i++)
	{
		R->Output[i] = 0;
		R->Period[i] = R->Count[i] = R->UpdateStep;
	}
	R->RNG = NG_PRESET;
	R->Output[3] = R->RNG & 1;

	/* build volume table (2dB per step) */
	divdr = 1;
	R->VolTable[0] = MAX_OUTPUT / 4;
	for (i = 1;i <= 14;i++)
	{
		R->VolTable[i] = R->VolTable[0] / divdr;
		divdr *= 1.258925412;	/* = 10 ^ (2/20) = 2dB */
	}
	R->VolTable[15] = 0;
}

int SN76496_sh_start(struct SN76496interface *interface)
{
	int i;

	intf = interface;

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	if( Machine->sample_bits == 16 ) sample_16bit = 1;
	else                             sample_16bit = 0;

	channel = get_play_channels( intf->num );

	for (i = 0;i < MAX_76496;i++)
	{
		sample_pos[i] = 0;
		output_buffer[i] = 0;
	}

	for (i = 0;i < intf->num;i++)
	{
		if ((output_buffer[i] = malloc((Machine->sample_bits/8)*buffer_len)) == 0)
		{
			while (--i >= 0) free(output_buffer[i]);
			return 1;
		}
		memset(output_buffer[i],0,(Machine->sample_bits/8)*buffer_len);

		sn[i].Clock = intf->clock;
		SN76496Reset(&sn[i],emulation_rate);
		/* volume gain controll */
		volume[i] = intf->volume[i];
	}
	return 0;
}

void SN76496_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++) free(output_buffer[i]);
}

static void SN76496Write(struct SN76496 *R,int data)
{
#ifdef DEBUG_PITCH
static int base = 5000;
if (osd_key_pressed(OSD_KEY_Z))
{
	if (base > 0) base--;
}
if (osd_key_pressed(OSD_KEY_X))
{
	if (base < 10000) base++;
}
osd_on_screen_display("PITCH",base/100);
#endif
	if (data & 0x80)
	{
		int r = (data & 0x70) >> 4;
		int c = r/2;

		R->LastRegister = r;
		R->Register[r] = (R->Register[r] & 0x3f0) | (data & 0x0f);
		switch (r)
		{
			case 0:	/* tone 0 : frequency */
			case 2:	/* tone 1 : frequency */
			case 4:	/* tone 2 : frequency */
				R->Period[c] = R->UpdateStep * R->Register[r];
				if (R->Period[c] == 0) R->Period[c] = R->UpdateStep;
				if (r == 4)
				{
					if ((R->Register[6] & 0x03) == 0x03)
{
						R->Period[3] = 2 * R->Period[2];
#ifdef DEBUG_PITCH
if ((R->Register[6]&4)==0) R->Period[3] = R->Period[3] * (50+(base/100))/100;
#endif
}
				}
				break;
			case 1:	/* tone 0 : volume */
			case 3:	/* tone 1 : volume */
			case 5:	/* tone 2 : volume */
			case 7:	/* noise  : volume */
				R->Volume[c] = R->VolTable[data & 0x0f];
				break;
			case 6:	/* noise  : frequency, mode */
				{
					int n = R->Register[6];
					R->NoiseFB = (n & 4) ? FB_WNOISE : FB_PNOISE;
					n &= 3;
					/* N/512,N/1024,N/2048,Tone #3 output */
					R->Period[3] = (n == 3) ? 2 * R->Period[2] : R->UpdateStep << (5+n);
#ifdef DEBUG_PITCH
if ((R->Register[6]&4)==0) R->Period[3] = R->Period[3] * (50+(base/100))/100;
#endif
					/* reset noise shifter */
					R->RNG = NG_PRESET;
					R->Output[3] = R->RNG & 1;
				}
				break;
		}
	}
	else
	{
		int r = R->LastRegister;
		int c = r/2;

		switch (r)
		{
			case 0:	/* tone 0 : frequency */
			case 2:	/* tone 1 : frequency */
			case 4:	/* tone 2 : frequency */
				R->Register[r] = (R->Register[r] & 0x0f) | ((data & 0x3f) << 4);
				R->Period[c] = R->UpdateStep * R->Register[r];
				if (R->Period[c] == 0) R->Period[c] = R->UpdateStep;
				if (r == 4)
				{
					if ((R->Register[6] & 0x03) == 0x03)
{
						R->Period[3] = 2 * R->Period[2];
#ifdef DEBUG_PITCH
if ((R->Register[6]&4)==0) R->Period[3] = R->Period[3] * (50+(base/100))/100;
#endif
}
				}
				break;
		}
	}
}

static void SN76496UpdateB(struct SN76496 *R , char *buffer , int start , int end)
{
	int i;
	unsigned char  *buffer_8;
	unsigned short *buffer_16;
	int length = end - start;

	if (length <= 0) return;

	buffer_8  = &((unsigned char  *)buffer)[start];
	buffer_16 = &((unsigned short *)buffer)[start];

	/* If the volume is 0, increase the counter */
	for (i = 0;i < 4;i++)
	{
		if (R->Volume[i] == 0)
		{
			/* note that I do count += length, NOT count = length + 1. You might think */
			/* it's the same since the volume is 0, but doing the latter could cause */
			/* interferencies when the program is rapidly modulating the volume. */
			if (R->Count[i] <= length*STEP) R->Count[i] += length*STEP;
		}
	}

	while (length)
	{
		int vol[4];
		unsigned int out;
		int left;


		/* vol[] keeps track of how long each square wave stays */
		/* in the 1 position during the sample period. */
		vol[0] = vol[1] = vol[2] = vol[3] = 0;

		for (i = 0;i < 3;i++)
		{
			if (R->Output[i]) vol[i] += R->Count[i];
			R->Count[i] -= STEP;
			/* Period[i] is the half period of the square wave. Here, in each */
			/* loop I add Period[i] twice, so that at the end of the loop the */
			/* square wave is in the same status (0 or 1) it was at the start. */
			/* vol[i] is also incremented by Period[i], since the wave has been 1 */
			/* exactly half of the time, regardless of the initial position. */
			/* If we exit the loop in the middle, Output[i] has to be inverted */
			/* and vol[i] incremented only if the exit status of the square */
			/* wave is 1. */
			while (R->Count[i] <= 0)
			{
				R->Count[i] += R->Period[i];
				if (R->Count[i] > 0)
				{
					R->Output[i] ^= 1;
					if (R->Output[i]) vol[i] += R->Period[i];
					break;
				}
				R->Count[i] += R->Period[i];
				vol[i] += R->Period[i];
			}
			if (R->Output[i]) vol[i] -= R->Count[i];
		}

		left = STEP;
		do
		{
			int nextevent;


			if (R->Count[3] < left) nextevent = R->Count[3];
			else nextevent = left;

			if (R->Output[3]) vol[3] += R->Count[3];
			R->Count[3] -= nextevent;
			if (R->Count[3] <= 0)
			{
				if (R->RNG & 1) R->RNG ^= R->NoiseFB;
				R->RNG >>= 1;
				R->Output[3] = R->RNG & 1;
				R->Count[3] += R->Period[3];
				if (R->Output[3]) vol[3] += R->Period[3];
			}
			if (R->Output[3]) vol[3] -= R->Count[3];

			left -= nextevent;
		} while (left > 0);

		out = vol[0] * R->Volume[0] + vol[1] * R->Volume[1] +
				vol[2] * R->Volume[2] + vol[3] * R->Volume[3];

		if( sample_16bit ) *(buffer_16++) = out / STEP;
		else               *(buffer_8++) = AUDIO_CONV(out / (STEP * 256));

		length--;
	}
}

static void doupdate(int chip)
{
	int newpos;


	newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

	if( newpos - sample_pos[chip] < MIN_SLICE ) return;
	SN76496UpdateB(&sn[chip],output_buffer[chip] ,sample_pos[chip],newpos);
	sample_pos[chip] = newpos;
}

void SN76496_0_w(int offset,int data)
{
	doupdate(0);
	SN76496Write(&sn[0],data);
}

void SN76496_1_w(int offset,int data)
{
	doupdate(1);
	SN76496Write(&sn[1],data);
}

void SN76496_2_w(int offset,int data)
{
	doupdate(2);
	SN76496Write(&sn[2],data);
}

void SN76496_3_w(int offset,int data)
{
	doupdate(3);
	SN76496Write(&sn[3],data);
}

void SN76496_sh_update(void)
{
	int i;


	if (Machine->sample_rate == 0) return;

	for (i = 0;i < intf->num;i++)
	{
		SN76496UpdateB(&sn[i],output_buffer[i],sample_pos[i],buffer_len);
		sample_pos[i] = 0;
	}

	for (i = 0;i < intf->num;i++)
		if( sample_16bit ) osd_play_streamed_sample_16(channel+i,output_buffer[i],2*buffer_len,emulation_rate,volume[i]);
		else               osd_play_streamed_sample(channel+i,output_buffer[i],buffer_len,emulation_rate,volume[i]);
}

/**********************************************/
void SN76496_change_clock( int num, int clock ){
  sn[num].Clock = clock;
  SN76496Reset(&sn[num],emulation_rate);
}
/*********************************************/
