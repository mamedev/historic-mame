/***************************************************************************

  sn76496.c

  Routines to emulate the Texas Instruments SN76489 and SN76496 programmable
  tone /noise generator.

  This is a very simple chip with no envelope control, therefore unlike the
  AY8910 or Pokey emulators, this module does not create a sample in a
  buffer: it just keeps track of the frequency of the three voices, it's up
  to the caller to generate tone of the required frenquency.

  Noise emulation is not accurate due to lack of documentation. In the chip,
  the noise generator uses a shift register, which can shift at four
  different rates: three of them are fixed, while the fourth is connected
  to the tone generator #3 output, but it's not clear how. It is also
  unknown how exactly the shift register works.
  Since it couldn't be accurate, this module doesn't create a sample for the
  noise output. It will only return the shift rate, that is, the sample rate
  the caller should use. It's up to the caller to create a suitable sample;
  a 10K sample filled with rand() will do.

  update 1997.6.21 Tatsuyuki Satoh.
    added SN76496UpdateB() function.

***************************************************************************/

#include "driver.h"
#include "sn76496.h"
#include "sndhrdw/generic.h"

#define MIN_SLICE 1		/* minimum update step */

#define AUDIO_CONV(A) (A)

/* It is define for true noise generater */
/* bit0 = output level                   */

/* noise feedback for write noise mode */
#define FB_WNOISE 0x12000	/* bit15.d(16bits) = bit0(out) ^ bit2 */
//#define FB_WNOISE 0x14000	/* bit15.d(16bits) = bit0(out) ^ bit1 */
//#define FB_WNOISE 0x28000	/* bit16.d(17bits) = bit0(out) ^ bit2 (same to AY-3-8910) */
//#define FB_WNOISE 0x50000	/* bit17.d(18bits) = bit0(out) ^ bit2 */

/* noise feedback for periodic noise mode */
/* it is collect maybe (it was written by megadrive sound manual) */
#define FB_PNOISE 0x10000	/* 16bit rorate */

/* noise generator start preset (for periodic noise) */
#define NG_PRESET 0x0f35

static int emulation_rate;
static int buffer_len;

static struct SN76496interface *intf;

static struct SN76496 sn[MAX_76496];
static unsigned char *buffer[MAX_76496];
static int sample_pos[MAX_76496];
static int volume[MAX_76496];

static int channel;

static void SN76496Reset(struct SN76496 *R)
{
	int i;

//	R->freqStep = (double)emulation_rate * 0x10000 / R->Clock * 2;  /* SN76494 */
	R->freqStep = (double)emulation_rate * 0x10000 / R->Clock * 16; /* SN76496 */

	for (i = 0;i < 4;i++) R->Volume[i] = 0;

	R->LastRegister = 0;
	for (i = 0;i < 8;i+=2)
	{
		R->Register[i] = 0;
		R->Register[i + 1] = 0x0f;	/* volume = 0 */
	}

	for (i = 0;i < 4;i++)
	{
		R->Turn[i] = R->Counter[i] = R->Dir[i] = 0;
	}
	R->NoiseGen = NG_PRESET;
}

static void SetVolRate( struct SN76496 *R, int rate , int limit )
{
	int i;
	double div;

	/* make volume table (2dB per step) */
	div = 1;
	R->VolTable[0] = rate / 2 / 4;
	for (i = 1;i <= 14;i++)
	{
		R->VolTable[i] = R->VolTable[0] / div;
		div *= 1.258925412; /* 2dB */
	}
	R->VolTable[15] = 0;

	/* volume limit */
	for ( i = 0 ; i < 16 ; i++ )
	{
		if( R->VolTable[i] > (limit/2/4) ) R->VolTable[i] = (limit/2/4);
	}
}

int SN76496_sh_start(struct SN76496interface *interface)
{
	int i;

	intf = interface;

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	channel = get_play_channels( intf->num );

	for (i = 0;i < MAX_76496;i++)
	{
		sample_pos[i] = 0;
		buffer[i] = 0;
	}

	for (i = 0;i < intf->num;i++)
	{
		if ((buffer[i] = malloc(buffer_len)) == 0)
		{
			while (--i >= 0) free(buffer[i]);
			return 1;
		}
		memset(buffer[i],AUDIO_CONV(0),buffer_len);

		sn[i].Clock = intf->clock;
		SN76496Reset(&sn[i]);
		/* volume gain controll */
		volume[i] = intf->volume[i];
		if( volume[i] > 255 )
		{
			SetVolRate( &sn[i] , 0xffff * volume[i] / 255 , 0xffff*2 );
			volume[i] = 255;
		}
		else
		{
			SetVolRate( &sn[i] , 0xffff , 0xffff );
		}
	}
	return 0;
}

void SN76496_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++) free(buffer[i]);
}

static void SN76496Write(struct SN76496 *R,int data)
{
	if (data & 0x80)
	{
		R->LastRegister = (data >> 4) & 0x07;
		R->Register[R->LastRegister] = (R->Register[R->LastRegister]&0x3f0) | (data & 0x0f);
		switch( data & 0x70 ){
		case 0x10:	/* tone 0 : volume    */
			R->Volume[0] = R->VolTable[ data&0x0f ];
			break;
		case 0x30:	/* tone 1 : volume    */
			R->Volume[1] = R->VolTable[ data&0x0f ];
			break;
		case 0x50:	/* tone 2 : volume    */
			R->Volume[2] = R->VolTable[ data&0x0f ];
			break;
		case 0x70:	/* noise  : vokume     */
			R->Volume[3] = R->VolTable[ data&0x0f ];
		case 0x60:	/* noise  : frequency, mode */
			{
				int n = R->Register[6];
				R->NoiseFB = (n & 4) ? FB_WNOISE : FB_PNOISE;
				n &= 3;
				/* N/512,N/1024,N/2048,Tone #3 output */
				R->Turn[3] = (n==3) ? R->Turn[2]<<1 : R->freqStep<<(5+n);
				/* reset noise shifter */
				R->NoiseGen = NG_PRESET;
			}
		}
	}
	else
	{
		R->Register[R->LastRegister] = (R->Register[R->LastRegister]&0x0f) | ((data&0x3f)<<4);
		switch( R->LastRegister ){
		case 0x00:	/* tone 0 : frequency */
			R->Turn[0] = R->freqStep * R->Register[0];
			break;
		case 0x02:	/* tone 1 : frequency */
			R->Turn[1] = R->freqStep * R->Register[2];
			break;
		case 0x04:	/* tone 2 : frequency */
			R->Turn[2] = R->freqStep * R->Register[4];
			break;
		}
	}
}

static void SN76496UpdateB(struct SN76496 *R , char *buffer , int size)
{
	int i;
	int vb0,vb1,vb2,vbn;
	int outdata;

	if (size <= 0) return;

	vb0 = R->Dir[0] ? -R->Volume[0]:R->Volume[0];
	vb1 = R->Dir[1] ? -R->Volume[1]:R->Volume[1];
	vb2 = R->Dir[2] ? -R->Volume[2]:R->Volume[2];
	vbn = (R->NoiseGen & 1) ? -R->Volume[3]:R->Volume[3];

	/* bypass if volume 0 or frecuency 0 */
	if( !vb0 || !R->Turn[0] ) R->Counter[0]+=size<<16;
	if( !vb1 || !R->Turn[1] ) R->Counter[1]+=size<<16;
	if( !vb2 || !R->Turn[2] ) R->Counter[2]+=size<<16;
	if( !vbn || !R->Turn[3] ) R->Counter[3]+=size<<16;

	/* make buffer */
	for (i = 0;i < size;i++)
	{
		outdata = AUDIO_CONV( vb0 + vb1 + vb2 + vbn );

		R->Counter[0] -= 0x10000;
		while( R->Counter[0] < 0 ){
			R->Dir[0]^=1; vb0 = -vb0;
			outdata -= (vb0 * R->Counter[0])>>15;
			R->Counter[0] += R->Turn[0];
		}
		R->Counter[1] -= 0x10000;
		while( R->Counter[1] < 0 ){
			R->Dir[1]^=1; vb1 = -vb1;
			outdata -= (vb1 * R->Counter[1])>>15;
			R->Counter[1] += R->Turn[1];
		}
		R->Counter[2] -= 0x10000;
		while( R->Counter[2] < 0 ){
			R->Dir[2]^=1; vb2 = -vb2;
			outdata -= (vb2 * R->Counter[2])>>15;
			R->Counter[2] += R->Turn[2];
		}
		R->Counter[3] -= 0x10000;
		while( R->Counter[3] < 0 ){
			if( R->NoiseGen & 1 ) R->NoiseGen ^= R->NoiseFB;
			if( (R->NoiseGen+1)&2 ){ /* if(bit0^bit1) */
				vbn = -vbn;
				outdata -= (vbn * R->Counter[3])>>15;
			}
			R->NoiseGen>>=1;
			R->Counter[3] += R->Turn[3];
		}
		buffer[i] = outdata >>8; /* 16bit -> 8bit */
	}
}

static void doupdate(int chip)
{
	int totcycles,leftcycles,newpos;

	totcycles = cpu_getfperiod();
	leftcycles = cpu_getfcount();
	newpos = buffer_len * (totcycles-leftcycles) / totcycles;
	if (newpos >= buffer_len) newpos = buffer_len - 1;

	if( newpos - sample_pos[chip] < MIN_SLICE ) return;
	SN76496UpdateB(&sn[chip],&buffer[chip][sample_pos[chip]],newpos - sample_pos[chip]);
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
		SN76496UpdateB(&sn[i],&buffer[i][sample_pos[i]],buffer_len - sample_pos[i]);
		sample_pos[i] = 0;
	}

	for (i = 0;i < intf->num;i++)
		osd_play_streamed_sample(channel+i,buffer[i],buffer_len,emulation_rate,volume[i]);
}
