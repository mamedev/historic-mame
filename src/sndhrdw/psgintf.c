#define NICK_BUBLBOBL_CHANGE

#include "driver.h"
#include "ay8910.h"
#include "fm.h"

extern unsigned char No_FM;

static int stream[MAX_2203];

static struct YM2203interface *intf;

static int FMMode;
#define CHIP_YM2203_DAC 2
#define CHIP_YM2203_OPL 3   /* YM2203 with OPL chip */

static void *Timer[MAX_2203][2];
static void (*timer_callback)(int param);

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
			Timer[n][c] = timer_set (timeSec, (c<<7)|n, timer_callback );
		}
	}
}

static void FMTimerInit( void )
{
	int i;

	for( i = 0 ; i < MAX_2203 ; i++ )
		Timer[i][0] = Timer[i][1] = 0;
	FMSetTimerHandler( TimerHandler , 0 );
}



/*------------------------- TM2203 -------------------------------*/
/* Timer overflow callback from timer.c */
static void timer_callback_2203(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	Timer[n][c] = 0;
	if( YM2203TimerOver(n,c) )
	{	/* IRQ is active */;
		/* User Interrupt call */
		if(intf->handler[n]) intf->handler[n]();
	}
#ifdef NICK_BUBLBOBL_CHANGE
	/* Sync another timer */
	c ^= 1;
	if( Timer[n][c] )
	{
		if( timer_timeleft(Timer[n][c]) < 0.008 )
		{
			timer_remove (Timer[n][c]);
			Timer[n][c] = 0;
			if( YM2203TimerOver(n,c) )
			{	/* IRQ is active */;
				/* User Interrupt call */
				if(intf->handler[n]) intf->handler[n]();
			}
		}
	}
#endif
}
/* update request from fm.c */
void YM2203UpdateRequest(int chip)
{
	stream_update(stream[chip],0);
}

#if 0
/* update callback from stream.c */
static void YM2203UpdateCallback(int chip,void *buffer,int length)
{
	YM2203UpdateOne(chip,buffer,length);
}
#endif

int YM2203_sh_start(struct YM2203interface *interface)
{
	int i;
	int rate = Machine->sample_rate;

	if( AY8910_sh_start(interface,"YM2203") ) return 1;

	/* FM init */
	if( No_FM ) FMMode = CHIP_YM2203_DAC;
	else        FMMode = CHIP_YM2203_OPL;

	intf = interface;

	/* Timer Handler set */
	timer_callback = timer_callback_2203;
	FMTimerInit();
	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		int volume;
		char name[20];
		sprintf(name,"YM2203 #%d FM",i);
		stream[i] = stream_init(name,Machine->sample_rate,Machine->sample_bits,i,YM2203UpdateOne/*YM2203UpdateCallback*/);
		/* volume setup */
		volume = intf->volume[i]>>16; /* high 16 bit */
		if( volume > 255 ) volume = 255;
		stream_set_volume(stream[i],volume);
	}
	/* Initialize FM emurator */
	if (YM2203Init(intf->num,intf->baseclock,Machine->sample_rate,Machine->sample_bits) == 0)
	{
		/* Ready */
		return 0;
	}
	/* error */
	/* stream close */
	AY8910_sh_stop();
	return 1;
}

void YM2203_sh_stop(void)
{
	int i;

	AY8910_sh_stop();
///////	if( FMMode == CHIP_YM2203_DAC )
	{
		YM2203Shutdown();
	}
}



static int lastreg0,lastreg1,lastreg2,lastreg3,lastreg4;

int YM2203_status_port_0_r(int offset) { return YM2203Read(0,0); }
int YM2203_status_port_1_r(int offset) { return YM2203Read(1,0); }
int YM2203_status_port_2_r(int offset) { return YM2203Read(2,0); }
int YM2203_status_port_3_r(int offset) { return YM2203Read(3,0); }
int YM2203_status_port_4_r(int offset) { return YM2203Read(4,0); }

int YM2203_read_port_0_r(int offset) { return YM2203Read(0,1); }
int YM2203_read_port_1_r(int offset) { return YM2203Read(1,1); }
int YM2203_read_port_2_r(int offset) { return YM2203Read(2,1); }
int YM2203_read_port_3_r(int offset) { return YM2203Read(3,1); }
int YM2203_read_port_4_r(int offset) { return YM2203Read(4,1); }

void YM2203_control_port_0_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC ) YM2203Write(0,0,data);
	else
	{
		AY8910Write(0,0,data);
		lastreg0 = data;
	}
}
void YM2203_control_port_1_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC ) YM2203Write(1,0,data);
	else
	{
		AY8910Write(1,0,data);
		lastreg1 = data;
	}
}
void YM2203_control_port_2_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC ) YM2203Write(2,0,data);
	else
	{
		AY8910Write(2,0,data);
		lastreg2 = data;
	}
}
void YM2203_control_port_3_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC ) YM2203Write(3,0,data);
	else
	{
		AY8910Write(3,0,data);
		lastreg3 = data;
	}
}
void YM2203_control_port_4_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC ) YM2203Write(4,0,data);
	else
	{
		AY8910Write(4,0,data);
		lastreg4 = data;
	}
}

void YM2203_write_port_0_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		YM2203Write(0,1,data);
	}
	else
	{
		if( lastreg0<16 ) AY8910Write(0,1,data);
		else osd_ym2203_write(0,lastreg0,data);
	}
}
void YM2203_write_port_1_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		YM2203Write(1,1,data);
	}
	else
	{
		if( lastreg1<16 ) AY8910Write(1,1,data);
		else osd_ym2203_write(1,lastreg1,data);
	}
}
void YM2203_write_port_2_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC ) YM2203Write(2,1,data);
	else
	{
		if( lastreg2<16 ) AY8910Write(2,1,data);
		else osd_ym2203_write(2,lastreg2,data);
	}
}
void YM2203_write_port_3_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		YM2203Write(3,1,data);
	}
	else
	{
		if( lastreg3<16 ) AY8910Write(3,1,data);
		else osd_ym2203_write(3,lastreg3,data);
	}
}
void YM2203_write_port_4_w(int offset,int data)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		YM2203Write(4,1,data);
	}
	else
	{
		if( lastreg4<16 ) AY8910Write(4,1,data);
		else              osd_ym2203_write(4,lastreg4,data);
	}
}

void YM2203_sh_update(void)
{
	if (Machine->sample_rate == 0 ) return;

	if( FMMode == CHIP_YM2203_OPL )
		osd_ym2203_update();
}
