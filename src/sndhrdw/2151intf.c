/***************************************************************************

  2151intf.c

  Support interface YM2151(OPM)

***************************************************************************/

#include "driver.h"
#include "fm.h"
#include "ym2151.h"

extern unsigned char No_FM;

/* for stream system */
static int stream[MAX_2151];
static void *FMBuf[2];

#define MIN_SLICE 44

static int emulation_rate;
static int buffer_len;
static int sample_bits;

static struct YM2151interface *intf;

static int FMMode;
#define CHIP_YM2151_DAC 4	/* use Tatsuyuki's FM.C */
#define CHIP_YM2151_ALT 5	/* use Jarek's YM2151.C */
#define CHIP_YM2151_OPL 6	/* use OPL (does not supported) */

#define YM2151_NUMBUF 2

static FMSAMPLE *bufferFM[MAX_2151*YM2151_NUMBUF];
static int sample_posFM[MAX_2151];

static int volume[MAX_2151];
static int channel;

static void *Timer[MAX_2151][2];

static void timer_callback_2151(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	Timer[n][c] = 0;
	if( YM2151TimerOver(n,c) )
	{	/* IRQ is active */;
		/* User Interrupt call */
		if(intf->irqhandler[n]) intf->irqhandler[n]();
	}
}

/* TimerHandler from fm.c */
static void TimerHandler(int n,int c,double timeSec)
{
	if( timeSec == 0 )
	{	/* Reset FM Timer */
		if( Timer[n][c] )
		{
	 		timer_remove (Timer[n][c]);
			Timer[n][c] = 0;
		}
	}
	else
	{	/* Start FM Timer */
		if( Timer[n][c] == 0 )
		{
			Timer[n][c] = timer_set (timeSec, (c<<7)|n, timer_callback_2151 );
		}
	}
}

/* update request from fm.c */
void YM2151UpdateRequest(int chip)
{
	stream_update(stream[chip],0);
}

int YM2151_sh_start(struct YM2151interface *interface,int mode)
{
	int i,j;
	int rate = Machine->sample_rate;
	char buf[YM2151_NUMBUF][40];
	const char *name[YM2151_NUMBUF];
	int vol;

	if( rate == 0 ) rate = 1000;	/* kludge to prevent nasty crashes */

	intf = interface;

	buffer_len = rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;
	sample_bits = Machine->sample_bits;

	if( mode ) FMMode = CHIP_YM2151_ALT;
	else       FMMode = CHIP_YM2151_DAC;
/*	if( !No_FM ) FMMode = CHIP_YM2151_OPL;*/

	switch(FMMode)
	{
	case CHIP_YM2151_DAC:	/* Tatsuyuki's */
		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			/* stream setup */
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j] = buf[j];
				sprintf(buf[j],"YM2151 #%d %s",i,(j&1)?"Rt":"Lt");
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,rate,Machine->sample_bits,
				i,OPMUpdateOne);
			/* volume setup */
			vol = intf->volume[i];
			if( vol > 255 ) vol = 255;
			for( j=0 ; j < YM2151_NUMBUF ; j++ )
			{
				stream_set_volume(stream[i]+j,vol);
				stream_set_pan(stream[i]+j,(j&1)?100:-100);
			}
		}
		/* Set Timer handler */
		for (i = 0; i < intf->num; i++)
			Timer[i][0] =Timer[i][1] = 0;
		FMSetTimerHandler( TimerHandler , 0);
		if (OPMInit(intf->num,intf->baseclock,Machine->sample_rate,sample_bits) == 0)
		{
			/* set port handler */
			for (i = 0; i < intf->num; i++)
				OPMSetPortHander(i,intf->portwritehandler[i]);
			return 0;
		}
		/* error */
		return 1;
	case CHIP_YM2151_ALT:	/* Jarek's */
		for (i = 0;i < MAX_2151;i++)
		{
			bufferFM[i] = 0;
			sample_posFM[i] = 0;
		}
		for (i = 0;i < intf->num;i++)
		{
			if ((bufferFM[i] = malloc((sample_bits/8)*buffer_len)) == 0)
			{
				while (--i >= 0) free(bufferFM[i]);
				return 1;
			}
			memset(bufferFM[i],0, buffer_len);
		}
		if (YMInit(intf->num,intf->baseclock,emulation_rate,sample_bits,buffer_len,bufferFM) == 0)
		{
			channel=get_play_channels(intf->num);
			for (i = 0; i < intf->num; i++)
			{
				YMSetIrqHandler(i,intf->irqhandler[i]);
				YMSetPortWriteHandler(i,intf->portwritehandler[i]);
			}
			return 0;
		}
		return 1;
	case CHIP_YM2151_OPL:
		break;
	}
	return 1;
}

void YM2151_sh_stop(void)
{
	int i;

	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		OPMShutdown();
		break;
	case CHIP_YM2151_ALT:
		YMShutdown();
		for (i = 0;i < intf->num *YM2151_NUMBUF;i++) free(bufferFM[i]);
		break;
	case CHIP_YM2151_OPL:
		return;
	}
}

INLINE void update_opm(int chip)
{
	int newpos;

	newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

	if( newpos - sample_posFM[chip] < MIN_SLICE ) return;
	YM2151UpdateOne(chip , newpos );
	sample_posFM[chip] = newpos;
}

static int lastreg0,lastreg1,lastreg2;

int YM2151_status_port_0_r(int offset)
{
	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		return OPMReadStatus(0);
	case CHIP_YM2151_ALT:
		update_opm(0);
		return YMReadReg(0);
	}
	return 0;
}

int YM2151_status_port_1_r(int offset)
{
	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		return OPMReadStatus(1);
	case CHIP_YM2151_ALT:
		update_opm(1);
		return YMReadReg(1);
	}
	return 0;
}

int YM2151_status_port_2_r(int offset)
{
	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		return OPMReadStatus(2);
	case CHIP_YM2151_ALT:
		update_opm(2);
		return YMReadReg(2);
	}
	return 0;
}

void YM2151_register_port_0_w(int offset,int data)
{
	lastreg0 = data;
}
void YM2151_register_port_1_w(int offset,int data)
{
	lastreg1 = data;
}
void YM2151_register_port_2_w(int offset,int data)
{
	lastreg2 = data;
}

static void opm_to_opn(int n,int r,int v)
{
	/* */
	return;
}

void YM2151_data_port_0_w(int offset,int data)
{
	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		YM2151Write(0,0,lastreg0);
		YM2151Write(0,1,data);
		break;
	case CHIP_YM2151_ALT:
		update_opm(0);
		YMWriteReg(0,lastreg0,data);
		break;
	case CHIP_YM2151_OPL:
		opm_to_opn(0,lastreg0,data);
		break;
	}
}

void YM2151_data_port_1_w(int offset,int data)
{
	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		YM2151Write(1,0,lastreg1);
		YM2151Write(1,1,data);
		break;
	case CHIP_YM2151_ALT:
		update_opm(1);
		YMWriteReg(1,lastreg1,data);
		break;
	case CHIP_YM2151_OPL:
		opm_to_opn(1,lastreg1,data);
		break;
	}
}

void YM2151_data_port_2_w(int offset,int data)
{
	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		YM2151Write(2,0,lastreg2);
		YM2151Write(2,1,data);
		break;
	case CHIP_YM2151_ALT:
		update_opm(2);
		YMWriteReg(2,lastreg2,data);
		break;
	case CHIP_YM2151_OPL:
		opm_to_opn(2,lastreg2,data);
		break;
	}
}

void YM2151_sh_update(void)
{
	int i;

	if (Machine->sample_rate == 0 ) return;

	switch(FMMode)
	{
	case CHIP_YM2151_OPL:
		osd_ym2203_update();
		return;
	case CHIP_YM2151_DAC:
		break;
	case CHIP_YM2151_ALT:
		YM2151Update();
		for (i = 0;i < intf->num;i++)
			if (sample_bits==16)
				osd_play_streamed_sample_16(channel+i,bufferFM[i],2*buffer_len,emulation_rate,intf->volume[i],0);
			else
				osd_play_streamed_sample(channel+i,bufferFM[i],buffer_len,emulation_rate,intf->volume[i],0);
		/* reset sample position */
		for (i = 0;i < intf->num;i++)
			sample_posFM[i] = 0;
		break;
	}
}

