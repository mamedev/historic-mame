#include <math.h>
#include "driver.h"
#include "ay8910.h"
#include "fm.h"


static int stream[MAX_2203];

static double syncTime[MAX_2203];

static const struct YM2203interface *intf;

static void *Timer[MAX_2203][2];
static void (*timer_callback)(int param);

/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if(intf->handler[n]) intf->handler[n](irq);
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
		if( Timer[n][c] == 0 )
		{
			double timeNow;
			double timeSec;
			/* Syncronus Start Timming */
			timeSec = ( (double)count * stepTime );
			timeNow = timer_get_time();
			if (syncTime[n] < timeNow)
			{
				syncTime[n] =  timeNow + (stepTime-fmod(timeNow-syncTime[n],stepTime));
			}
			Timer[n][c] = timer_set (timeSec+(syncTime[n]-timeNow), (c<<7)|n, timer_callback );
		}
	}
}

static void FMTimerInit( void )
{
	int i;

	for( i = 0 ; i < MAX_2203 ; i++ )
	{
		Timer[i][0] = Timer[i][1] = 0;
		syncTime[i] = 0.0;
	}
}



/*------------------------- TM2203 -------------------------------*/
/* Timer overflow callback from timer.c */
static void timer_callback_2203(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	Timer[n][c] = 0;
	YM2203TimerOver(n,c);
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

int YM2203_sh_start(const struct MachineSound *msound)
{
	int i;

	if (AY8910_sh_start(msound)) return 1;

	intf = msound->sound_interface;

	/* Timer Handler set */
	timer_callback = timer_callback_2203;
	FMTimerInit();
	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		int volume;
		char name[20];
		sprintf(name,"%s #%d FM",sound_name(msound),i);
		volume = intf->mixing_level[i]>>16; /* high 16 bit */
		stream[i] = stream_init(name,volume,Machine->sample_rate,FM_OUTPUT_BIT,i,YM2203UpdateOne/*YM2203UpdateCallback*/);
	}
	/* Initialize FM emurator */
	if (YM2203Init(intf->num,intf->baseclock,Machine->sample_rate,TimerHandler,IRQHandler) == 0)
	{
		/* Ready */
		return 0;
	}
	/* error */
	/* stream close */
	return 1;
}

void YM2203_sh_stop(void)
{
	YM2203Shutdown();
}

void YM2203_sh_reset(void)
{
	int i;

	for (i = 0;i < intf->num;i++)
		YM2203ResetChip(i);
}



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
	YM2203Write(0,0,data);
}
void YM2203_control_port_1_w(int offset,int data)
{
	YM2203Write(1,0,data);
}
void YM2203_control_port_2_w(int offset,int data)
{
	YM2203Write(2,0,data);
}
void YM2203_control_port_3_w(int offset,int data)
{
	YM2203Write(3,0,data);
}
void YM2203_control_port_4_w(int offset,int data)
{
	YM2203Write(4,0,data);
}

void YM2203_write_port_0_w(int offset,int data)
{
	YM2203Write(0,1,data);
}
void YM2203_write_port_1_w(int offset,int data)
{
	YM2203Write(1,1,data);
}
void YM2203_write_port_2_w(int offset,int data)
{
	YM2203Write(2,1,data);
}
void YM2203_write_port_3_w(int offset,int data)
{
	YM2203Write(3,1,data);
}
void YM2203_write_port_4_w(int offset,int data)
{
	YM2203Write(4,1,data);
}
