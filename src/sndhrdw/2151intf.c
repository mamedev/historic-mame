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

static struct YM2151interface *intf;

static int FMMode;
#define CHIP_YM2151_DAC 4	/* use Tatsuyuki's FM.C */
#define CHIP_YM2151_ALT 5	/* use Jarek's YM2151.C */
#define CHIP_YM2151_OPL 6	/* use OPL (does not supported) */

#define YM2151_NUMBUF 2

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
static void TimerHandler(int n,int c,int count,double stepTime)
{
	if( count == 0 )
	{	/* Reset FM Timer */
		if( Timer[n][c] )
		{
	 		timer_remove (Timer[n][c]);
			Timer[n][c] = 0;
		}
	}
	else
	{	/* Start FM Timer */
		double timeSec = (double)count * stepTime;

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
				char *chname[2] = { "Lt", "Rt" };
				int ch;


				name[j] = buf[j];
				ch = j & 1;
				if (intf->volume[i] & YM2151_STEREO_REVERSE)
					ch ^= 1;
				sprintf(buf[j],"YM2151 #%d %s",i,chname[ch]);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,rate,Machine->sample_bits,
				i,OPMUpdateOne);
			/* volume setup */
			vol = intf->volume[i] & 0xff;
			for( j=0 ; j < YM2151_NUMBUF ; j++ )
			{
				int ch;

				ch = j & 1;
				if (intf->volume[i] & YM2151_STEREO_REVERSE)
					ch ^= 1;

				stream_set_volume(stream[i]+j,vol);
				stream_set_pan(stream[i]+j,ch ? OSD_PAN_RIGHT : OSD_PAN_LEFT);
			}
		}
		/* Set Timer handler */
		for (i = 0; i < intf->num; i++)
			Timer[i][0] =Timer[i][1] = 0;
		if (OPMInit(intf->num,intf->baseclock,Machine->sample_rate,Machine->sample_bits,TimerHandler,0) == 0)
		{
			/* set port handler */
			for (i = 0; i < intf->num; i++)
				OPMSetPortHander(i,intf->portwritehandler[i]);
			return 0;
		}
		/* error */
		return 1;
	case CHIP_YM2151_ALT:	/* Jarek's */
		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			/* stream setup */
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				char *chname[2] = { "Lt", "Rt" };
				int ch;


				name[j] = buf[j];
				ch = j & 1;
				if (intf->volume[i] & YM2151_STEREO_REVERSE)
					ch ^= 1;
				sprintf(buf[j],"YM2151 #%d %s",i,chname[ch]);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,rate,Machine->sample_bits,
				i,YM2151UpdateOne);
			/* volume setup */
			vol = intf->volume[i] & 0xff;
			for( j=0 ; j < YM2151_NUMBUF ; j++ )
			{
				int ch;

				ch = j & 1;
				if (intf->volume[i] & YM2151_STEREO_REVERSE)
					ch ^= 1;

				stream_set_volume(stream[i]+j,vol);
				stream_set_pan(stream[i]+j,ch ? OSD_PAN_RIGHT : OSD_PAN_LEFT);
			}
		}
		if (YM2151Init(intf->num,intf->baseclock,Machine->sample_rate,Machine->sample_bits) == 0)
		{
			for (i = 0; i < intf->num; i++)
			{
				YM2151SetIrqHandler(i,intf->irqhandler[i]);
				YM2151SetPortWriteHandler(i,intf->portwritehandler[i]);
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
		YM2151Shutdown();
		break;
	case CHIP_YM2151_OPL:
		return;
	}
}

static int lastreg0,lastreg1,lastreg2;

int YM2151_status_port_0_r(int offset)
{
	switch(FMMode)
	{
	case CHIP_YM2151_DAC:
		return OPMReadStatus(0);
	case CHIP_YM2151_ALT:
		return YM2151ReadStatus(0);
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
		return YM2151ReadStatus(1);
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
		return YM2151ReadStatus(2);
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
		YM2151UpdateRequest(0);
		YM2151WriteReg(0,lastreg0,data);
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
		YM2151UpdateRequest(1);
		YM2151WriteReg(1,lastreg1,data);
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
		YM2151UpdateRequest(2);
		YM2151WriteReg(2,lastreg2,data);
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
		break;
	}
}

