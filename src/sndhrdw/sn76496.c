/***************************************************************************

  sn76496.c

  Routines to emulate the Texas Instruments SN76489 and SN76496 programmable
  tone /noise generator.

  Noise emulation is not accurate due to lack of documentation. In the chip,
  the noise generator uses a shift register, which can shift at four
  different rates: three of them are fixed, while the fourth is connected
  to the tone generator #3 output, but it's not clear how. It is also
  unknown how exactly the shift register works.

***************************************************************************/

#include "driver.h"

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

struct SN76496
{
	/* set this before calling SN76496Reset() */
	int Clock;			/* chip clock in Hz     */
	int freqStep;		/* frequency count step */
	int Volume[4];		/* volume of voice 0-2 and noise. Range is 0-0x1fff */
	int NoiseFB;		/* noise feedback mask */
	int Register[8];	/* registers */
	int LastRegister;	/* last writed register */
	int Counter[4];		/* frequency counter    */
	int Turn[4];
	int Dir[4];			/* output direction     */
	int VolTable[16];	/* volume tables        */
	unsigned int NoiseGen;		/* noise generator      */
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

static void SN76496Reset(struct SN76496 *R)
{
	int i;

	R->freqStep = (double)emulation_rate * 0x10000 * 16 / R->Clock; /* SN76496 */

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
	double divdr;

	/* make volume table (2dB per step) */
	divdr = 1;
	R->VolTable[0] = rate / 2 / 4;
	for (i = 1;i <= 14;i++)
	{
		R->VolTable[i] = R->VolTable[0] / divdr;
		divdr *= 1.258925412; /* 2dB */
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
		memset(output_buffer[i],AUDIO_CONV(0),(Machine->sample_bits/8)*buffer_len);

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

	for (i = 0;i < intf->num;i++) free(output_buffer[i]);
}

static void SN76496Write(struct SN76496 *R,int data)
{
	if (data & 0x80)
	{
		R->LastRegister = (data >> 4) & 0x07;
		R->Register[R->LastRegister] = (R->Register[R->LastRegister]&0x3f0) | (data & 0x0f);
		switch( data & 0x70 ){
		case 0x00:	/* tone 0 : frequency */
			R->Turn[0] = R->freqStep * R->Register[0];
			break;
		case 0x10:	/* tone 0 : volume    */
			R->Volume[0] = R->VolTable[ data&0x0f ];
			break;
		case 0x20:	/* tone 1 : frequency */
			R->Turn[1] = R->freqStep * R->Register[2];
			break;
		case 0x30:	/* tone 1 : volume    */
			R->Volume[1] = R->VolTable[ data&0x0f ];
			break;
		case 0x40:	/* tone 2 : frequency */
			R->Turn[2] = R->freqStep * R->Register[4];
			if( (R->Register[6]&0x03) == 0x03 ) R->Turn[3] = R->Turn[2]<<1;
			break;
		case 0x50:	/* tone 2 : volume    */
			R->Volume[2] = R->VolTable[ data&0x0f ];
			break;
		case 0x70:	/* noise  : volume     */
			R->Volume[3] = R->VolTable[ data&0x0f ];
			break;
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
			break;
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
			if( (R->Register[6]&0x03) == 0x03 ) R->Turn[3] = R->Turn[2]<<1;
			break;
		}
	}
}

static void SN76496UpdateB(struct SN76496 *R , char *buffer , int start , int end)
{
	int i;
	int vb0,vb1,vb2,vbn;
	int outdata;
	int size = end - start;

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
	for (i = start;i < end;i++)
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
		if( sample_16bit ) ((unsigned short *)buffer)[i] = outdata;
		else               ((unsigned char  *)buffer)[i] = outdata >> 8; /* 16bit -> 8bit */
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
