/***************************************************************************

  2612intf.c

  The YM2612 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include "driver.h"
#include "sndhrdw/ay8910.h"
#include "sndhrdw/fm.h"
#include "sndhrdw/2612intf.h"


#ifdef BUILD_YM2612

/* use FM.C with stream system */

extern unsigned char No_FM;

static int stream[MAX_2612];

static FMSAMPLE *Buf[YM2612_NUMBUF];

/* Global Interface holder */
static struct YM2612interface *intf;

static void *Timer[MAX_2612][2];
static double lastfired[MAX_2612][2];

/*------------------------- TM2612 -------------------------------*/
/* Timer overflow callback from timer.c */
static void timer_callback_2612(int param)
{
	int n=param&0x7f;
	int c=param>>7;

//	if(errorlog) fprintf(errorlog,"2612 TimerOver %d\n",c);
	Timer[n][c] = 0;
	lastfired[n][c] = timer_get_time();
	if( YM2612TimerOver(n,c) )
	{	/* IRQ is active */;
		/* User Interrupt call */
		if(intf->handler[n]) intf->handler[n]();
	}
}

/* TimerHandler from fm.c */
static void TimerHandler(int n,int c,int count,double stepTime)
{
	if( count == 0 )
	{	/* Reset FM Timer */
		if( Timer[n][c] )
		{
//			if(errorlog) fprintf(errorlog,"2612 TimerReset %d\n",c);
	 		timer_remove (Timer[n][c]);
			Timer[n][c] = 0;
		}
	}
	else
	{	/* Start FM Timer */
		double timeSec = (double)count * stepTime;

		if( Timer[n][c] == 0 )
		{
			double slack;

			slack = timer_get_time() - lastfired[n][c];
			/* hackish way to make bstars intro sync without */
			/* breaking sonicwi2 command 0x35 */
			if (slack < 0.000050) slack = 0;

//			if(errorlog) fprintf(errorlog,"2612 TimerSet %d %f slack %f\n",c,timeSec,slack);

			Timer[n][c] = timer_set (timeSec - slack, (c<<7)|n, timer_callback_2612 );
		}
	}
}

static void FMTimerInit( void )
{
	int i;

	for( i = 0 ; i < MAX_2612 ; i++ )
		Timer[i][0] = Timer[i][1] = 0;
}

/* update request from fm.c */
void YM2612UpdateRequest(int chip)
{
	stream_update(stream[chip],100);
}

/***********************************************************/
/*    YM2612 (fm4ch type)                                  */
/***********************************************************/
int YM2612_sh_start(struct YM2612interface *interface ){
	int i,j;
	int rate = Machine->sample_rate;
	char buf[YM2612_NUMBUF][40];
	const char *name[YM2612_NUMBUF];

	intf = interface;
	if( intf->num > MAX_2612 ) return 1;

	/* FM init */
	/* Timer Handler set */
	FMTimerInit();
	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		int vol;
		/* stream setup */
		for (j = 0 ; j < YM2612_NUMBUF ; j++)
		{
			name[j] = buf[j];
			sprintf(buf[j],"YM2612(%s) #%d %s",j < 2 ? "FM" : "ADPCM",i,(j&1)?"Rt":"Lt");
		}
		stream[i] = stream_init_multi(YM2612_NUMBUF,
			name,rate,Machine->sample_bits,
			i,YM2612UpdateOne);
		/* volume setup */
		vol = intf->volume[i]; /* high 16 bit */
		if( vol > 255 ) vol = 255;
		for( j=0 ; j < YM2612_NUMBUF ; j++ )
		  {
		    stream_set_volume(stream[i]+j,vol);
		    stream_set_pan(stream[i]+j,(j&1)?OSD_PAN_RIGHT:OSD_PAN_LEFT);
		  }
	}

	/**** initialize YM2612 ****/
	if (YM2612Init(intf->num,intf->baseclock,rate,TimerHandler,0) == 0)
	  return 0;
	/* error */
	return 1;
}

/************************************************/
/* Sound Hardware Stop							*/
/************************************************/
void YM2612_sh_stop(void)
{
  YM2612Shutdown();
}

/************************************************/
/* Status Read for YM2612 - Chip 0				*/
/************************************************/
int YM2612_status_port_0_A_r( int offset )
{
  return YM2612Read(0,0);
}

int YM2612_status_port_0_B_r( int offset )
{
  return YM2612Read(0,2);
}

/************************************************/
/* Status Read for YM2612 - Chip 1				*/
/************************************************/
int YM2612_status_port_1_A_r( int offset ) {
  return YM2612Read(1,0);
}

int YM2612_status_port_1_B_r( int offset ) {
  return YM2612Read(1,2);
}

/************************************************/
/* Port Read for YM2612 - Chip 0				*/
/************************************************/
int YM2612_read_port_0_r( int offset ){
  return YM2612Read(0,1);
}

/************************************************/
/* Port Read for YM2612 - Chip 1				*/
/************************************************/
int YM2612_read_port_1_r( int offset ){
  return YM2612Read(1,1);
}

/************************************************/
/* Control Write for YM2612 - Chip 0			*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2612_control_port_0_A_w(int offset,int data)
{
  YM2612Write(0,0,data);
}

void YM2612_control_port_0_B_w(int offset,int data)
{
  YM2612Write(0,2,data);
}

/************************************************/
/* Control Write for YM2612 - Chip 1			*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2612_control_port_1_A_w(int offset,int data){
  YM2612Write(1,0,data);
}

void YM2612_control_port_1_B_w(int offset,int data){
  YM2612Write(1,2,data);
}

/************************************************/
/* Data Write for YM2612 - Chip 0				*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2612_data_port_0_A_w(int offset,int data)
{
  YM2612Write(0,1,data);
}

void YM2612_data_port_0_B_w(int offset,int data)
{
  YM2612Write(0,3,data);
}

/************************************************/
/* Data Write for YM2612 - Chip 1				*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2612_data_port_1_A_w(int offset,int data){
  YM2612Write(1,1,data);
}
void YM2612_data_port_1_B_w(int offset,int data){
  YM2612Write(1,3,data);
}

/************************************************/
/* Sound Hardware Update						*/
/************************************************/
void YM2612_sh_update(void)
{
}

/**************** end of file ****************/

#endif
