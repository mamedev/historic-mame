/***************************************************************************

  2610intf.c

  The YM2610 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include "driver.h"
#include "ay8910.h"
#include "fm.h"

#ifdef BUILD_YM2610

/* use FM.C with stream system */

extern unsigned char No_FM;

static int stream[MAX_2610];

static FMSAMPLE *Buf[YM2610_NUMBUF];

/* Global Interface holder */
static struct YM2610interface *intf;

static void *Timer[MAX_2610][2];
static double lastfired[MAX_2610][2];

static int FMpan[4];

/*------------------------- TM2610 -------------------------------*/
/* Timer overflow callback from timer.c */
static void timer_callback_2610(int param)
{
	int n=param&0x7f;
	int c=param>>7;

//	if(errorlog) fprintf(errorlog,"2610 TimerOver %d\n",c);
	Timer[n][c] = 0;
	lastfired[n][c] = timer_get_time();
	if( YM2610TimerOver(n,c) )
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
//			if(errorlog) fprintf(errorlog,"2610 TimerReset %d\n",c);
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

//			if(errorlog) fprintf(errorlog,"2610 TimerSet %d %f slack %f\n",c,timeSec,slack);

			Timer[n][c] = timer_set (timeSec - slack, (c<<7)|n, timer_callback_2610 );
		}
	}
}

static void FMTimerInit( void )
{
	int i;

	for( i = 0 ; i < MAX_2610 ; i++ )
		Timer[i][0] = Timer[i][1] = 0;
}

/* update request from fm.c */
void YM2610UpdateRequest(int chip)
{
	stream_update(stream[chip],100);
}

int YM2610_sh_start(struct YM2610interface *interface ){
	int i,j;
	int rate = Machine->sample_rate;
	char buf[YM2610_NUMBUF][40];
	const char *name[YM2610_NUMBUF];
	int mixed_vol,vol[YM2610_NUMBUF],pan[YM2610_NUMBUF];

	intf = interface;
	if( intf->num > MAX_2610 ) return 1;

	if( AY8910_sh_start((struct AY8910interface *)interface,"YM2610(SSG)") ) return 1;

	/* FM init */
#if 0
	if( No_FM ) FMMode = CHIP_YM2610_DAC;
	else        FMMode = CHIP_YM2610_OPL;
#endif

	/* Timer Handler set */
	FMTimerInit();

	FMpan[0] = 0x00;		/* left pan */
	FMpan[1] = 0xff;		/* right pan */
	FMpan[2] = 0x00;		/* left pan */
	FMpan[3] = 0xff;		/* right pan */

	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		/* stream setup */
		mixed_vol = intf->volumeFM[i];
		/* stream setup */
		for (j = 0 ; j < YM2610_NUMBUF ; j++)
		{
			char *chname;
			name[j]=buf[j];
			vol[j] = mixed_vol & 0xff;
			pan[j] = (mixed_vol>>8) & 0xff;
			mixed_vol>>=16;
			switch( pan[j] ){
			case OSD_PAN_CENTER:chname="Ct";break;
			case OSD_PAN_LEFT:  chname="Lt";break;
			case OSD_PAN_RIGHT: chname="Rt";break;
			default:            chname="??";break;
			}
			sprintf(buf[j],"YM2610 #%d Ch%d(%s)",i,j+1,chname);
		}
		stream[i] = stream_init_multi(YM2610_NUMBUF,name,rate,FM_OUTPUT_BIT,i,YM2610UpdateOne);
		/* volume setup */
		for (j = 0 ; j < YM2610_NUMBUF ; j++)
		{
			stream_set_volume(stream[i]+j,vol[j]);
			stream_set_pan(stream[i]+j,pan[j]);
		}
	}

	/**** initialize YM2610 ****/
	if (YM2610Init(intf->num,intf->baseclock,rate, intf->pcmroma, intf->pcmromb,TimerHandler,0) == 0)
		return 0;

	/* error */
	AY8910_sh_stop();
	return 1;
}

/************************************************/
/* Sound Hardware Stop							*/
/************************************************/
void YM2610_sh_stop(void)
{
	AY8910_sh_stop();
	YM2610Shutdown();
}

/************************************************/
/* Status Read for YM2610 - Chip 0				*/
/************************************************/
int YM2610_status_port_0_A_r( int offset )
{
//if(errorlog) fprintf(errorlog,"PC %04x: 2610 S0A=%02X\n",cpu_getpc(),YM2610Read(0,0));
	return YM2610Read(0,0);
}

int YM2610_status_port_0_B_r( int offset )
{
//if(errorlog) fprintf(errorlog,"PC %04x: 2610 S0B=%02X\n",cpu_getpc(),YM2610Read(0,2));
	return YM2610Read(0,2);
}

/************************************************/
/* Status Read for YM2610 - Chip 1				*/
/************************************************/
int YM2610_status_port_1_A_r( int offset ) {
	return YM2610Read(1,0);
}

int YM2610_status_port_1_B_r( int offset ) {
	return YM2610Read(1,2);
}

/************************************************/
/* Port Read for YM2610 - Chip 0				*/
/************************************************/
int YM2610_read_port_0_r( int offset ){
	return YM2610Read(0,1);
}

/************************************************/
/* Port Read for YM2610 - Chip 1				*/
/************************************************/
int YM2610_read_port_1_r( int offset ){
	return YM2610Read(1,1);
}

/************************************************/
/* Control Write for YM2610 - Chip 0			*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_control_port_0_A_w(int offset,int data)
{
//if(errorlog) fprintf(errorlog,"PC %04x: 2610 Reg A %02X",cpu_getpc(),data);
	YM2610Write(0,0,data);
}

void YM2610_control_port_0_B_w(int offset,int data)
{
//if(errorlog) fprintf(errorlog,"PC %04x: 2610 Reg B %02X",cpu_getpc(),data);
	YM2610Write(0,2,data);
}

/************************************************/
/* Control Write for YM2610 - Chip 1			*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_control_port_1_A_w(int offset,int data){
	YM2610Write(1,0,data);
}

void YM2610_control_port_1_B_w(int offset,int data){
	YM2610Write(1,2,data);
}

/************************************************/
/* Data Write for YM2610 - Chip 0				*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_data_port_0_A_w(int offset,int data)
{
//if(errorlog) fprintf(errorlog," =%02X\n",data);
	YM2610Write(0,1,data);
}

void YM2610_data_port_0_B_w(int offset,int data)
{
//if(errorlog) fprintf(errorlog," =%02X\n",data);
	YM2610Write(0,3,data);
}

/************************************************/
/* Data Write for YM2610 - Chip 1				*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_data_port_1_A_w(int offset,int data){
	YM2610Write(1,1,data);
}
void YM2610_data_port_1_B_w(int offset,int data){
	YM2610Write(1,3,data);
}

/************************************************/
/* Sound Hardware Update						*/
/************************************************/
void YM2610_sh_update(void)
{
}

/**************************************************/
/*   YM2610 left/right position change (TAITO)    */
/**************************************************/
void YM2610_pan( int lr, int v ){
	FMpan[lr] = v;
}

/**************** end of file ****************/

#endif
