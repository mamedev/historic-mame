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

static int stream[MAX_2610][YM2610_NUMBUF];

static FMSAMPLE *Buf[YM2610_NUMBUF];

/* Global Interface holder */
static struct YM2610interface *intf;

static void *Timer[MAX_2610][2];

static int FMpan[4];

/*------------------------- TM2610 -------------------------------*/
/* Timer overflow callback from timer.c */
static void timer_callback_2610(int param)
{
	int n=param&0x7f;
	int c=param>>7;

//	if(errorlog) fprintf(errorlog,"2610 TimerOver\n");
	Timer[n][c] = 0;
	if( YM2610TimerOver(n,c) )
	{	/* IRQ is active */;
		/* User Interrupt call */
		if(intf->handler[n]) intf->handler[n]();
	}
}

/* TimerHandler from fm.c */
static void TimerHandler(int n,int c,double timeSec)
{
	if( timeSec == 0 )
	{	/* Reset FM Timer */
		if( Timer[n][c] )
		{
//			if(errorlog) fprintf(errorlog,"2610 TimerReset\n");
	 		timer_remove (Timer[n][c]);
			Timer[n][c] = 0;
		}
	}
	else
	{	/* Start FM Timer */
		if( Timer[n][c] == 0 )
		{
//			if(errorlog) fprintf(errorlog,"2610 TimerSet %f\n",timeSec);
			Timer[n][c] = timer_set (timeSec, (c<<7)|n, timer_callback_2610 );
		}
	}
}

static void FMTimerInit( void )
{
	int i;

	for( i = 0 ; i < MAX_2610 ; i++ )
		Timer[i][0] = Timer[i][1] = 0;
	FMSetTimerHandler( TimerHandler , 0 );
}

/* update request from fm.c */
void YM2610UpdateRequest(int chip)
{
	int chan;
	/* pickup buffer pointer */
	for( chan = 0 ; chan < YM2610_NUMBUF ; chan++)
		stream_update(stream[chip][chan],100);
}

static void YM2610UpdateCallbackLatch(int chan,void *buffer,int length)
{
	Buf[chan] = buffer;
}

static void YM2610UpdateCallback(int chip,void *buffer,int length)
{
	Buf[YM2610_NUMBUF-1] = buffer;
	YM2610UpdateOne(chip,Buf,length);
}

int YM2610_sh_start(struct YM2610interface *interface ){
	int i,j;
	int rate = Machine->sample_rate;

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
		int vol;
		char name[20];
		for( j=0 ; j < YM2610_NUMBUF-1 ; j++ )
		{
			sprintf(name,"YM2610 #%d-%d",i,j);
			stream[i][j] = stream_init(name,Machine->sample_rate,Machine->sample_bits,j,YM2610UpdateCallbackLatch);
		}
		sprintf(name,"YM2610 #%d-%d",i,YM2610_NUMBUF-1);
		stream[i][YM2610_NUMBUF-1] = stream_init(name,Machine->sample_rate,Machine->sample_bits,i,YM2610UpdateCallback);
		/* volume setup */
		vol = intf->volume[i]>>16; /* high 16 bit */
		if( vol > 255 ) vol = 255;
		for( j=0 ; j < YM2610_NUMBUF ; j++ )
			stream_set_volume(stream[i][j],vol);
	}

	/**** initialize YM2610 ****/
	if (YM2610Init(intf->num,intf->clock,Machine->sample_rate,Machine->sample_bits, intf->pcmroma, intf->pcmromb) == 0)
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
int YM2610_status_port_0_A_r( int offset ){
//	if(errorlog) fprintf(errorlog,"2610 S0=%02X\n",YM2610Read(0,0));
	return YM2610Read(0,0);
}

int YM2610_status_port_0_B_r( int offset ){
//	if(errorlog) fprintf(errorlog,"2610 S0=%02X\n",YM2610Read(0,2));
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
void YM2610_control_port_0_A_w(int offset,int data){
//	if(errorlog) fprintf(errorlog,"2610 A0,Reg %02X",data);
	YM2610Write(0,0,data);
}

void YM2610_control_port_0_B_w(int offset,int data){
//	if(errorlog) fprintf(errorlog,"2610 A1,Reg %02X",data);
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
void YM2610_data_port_0_A_w(int offset,int data){
//	if(errorlog) fprintf(errorlog,"=%02X\n",data);
	YM2610Write(0,1,data);
}

void YM2610_data_port_0_B_w(int offset,int data){
//	if(errorlog) fprintf(errorlog,"=%02X\n",data);
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

#else








/* use YM2610.C */

#include "ym2610.h"

extern unsigned char No_FM;

#define MIN_SLICE_2610 44	/* minimum update step for YM2610 (1msec @ 44100Hz)      */

/* Global Interface holder */
static struct YM2610interface *intf;

static int emulation_rateFM;
static int buffer_lenFM;
static int sample_bits;

static FMSAMPLE *bufferFM[MAX_2610*YM2610_NUMBUF];
static int sample_posFM[MAX_2610];
static int volumeFM[MAX_2610];
static int channelFM;
static int FMpan[4];

void AYWriteReg(int chip, int r, int v);
unsigned char AYReadReg(int n, int r);

/************************************************/
/* Sound Hardware Start							*/
/************************************************/
int YM2610_sh_start(struct YM2610interface *interface ){
	int i,j;
	int rate = Machine->sample_rate;

	intf = interface;
	if( intf->num > MAX_2610 )    return 1;

	if (AY8910_sh_start((struct AY8910interface *)interface,"YM2610") ) return 1;

	buffer_lenFM = rate / Machine->drv->frames_per_second;
	emulation_rateFM = buffer_lenFM * Machine->drv->frames_per_second;
	sample_bits = Machine->sample_bits;

	FMpan[0] = 0x00;		/* left pan */
	FMpan[1] = 0xff;		/* right pan */
	FMpan[2] = 0x00;		/* left pan */
	FMpan[3] = 0xff;		/* right pan */

	for( i = 0; i < MAX_2610; i++ ) {
		sample_posFM[i] = 0;

		for( j = 0; j < YM2610_NUMBUF; j++ )  bufferFM[i*YM2610_NUMBUF+j] = 0;
	}

	/**** get YM2610 buffer ****/
	for( i = 0; i < intf->num*YM2610_NUMBUF; i++ ){
	if((bufferFM[i] = malloc((sample_bits/8)*buffer_lenFM)) == 0){
	  while (--i >= 0)  free(bufferFM[i]);
	  return 1;
	}
	}

	/**** initialize YM2610 ****/
	if( OPNBInit( intf->num, intf->clock, emulation_rateFM, sample_bits, buffer_lenFM, bufferFM, intf->pcmroma, intf->pcmromb ) == 0 ) {
		channelFM = get_play_channels( intf->num*YM2610_NUMBUF );
		for ( i = 0; i < intf->num; i++ ) {
			volumeFM[i] = intf->volume[i];
			if( volumeFM[i] > 255 ) {
				/* do not support gain control for FM emurator */
				volumeFM[i] = 255;
			}
			OPNBSetIrqHandler (i, intf->handler[i]);
		}
		return 0;
	}

	/* error */
	for( i = 0; i < intf->num*YM2610_NUMBUF; i++ )
	  	free( bufferFM[i] );

	return 1;
}

/************************************************/
/* Sound Hardware Stop							*/
/************************************************/
void YM2610_sh_stop(void){
	int i;

	AY8910_sh_stop();
	OPNBShutdown();

	for( i = 0; i < intf->num*YM2610_NUMBUF; i++ )
		free(bufferFM[i]);
}

/************************************************/
/* Update FM									*/
/************************************************/

INLINE void update_fm( int chip ) {
	int newpos;
	newpos = cpu_scalebyfcount(buffer_lenFM);	/* get current position based on the timer */

	if( newpos - sample_posFM[chip] < MIN_SLICE_2610 ) return;
	OPNBUpdateOne( chip , newpos );
	sample_posFM[chip] = newpos;
}

static int lastreg0, lastreg1, lastreg2, lastreg3;

/************************************************/
/* Status Read for YM2610 - Chip 0				*/
/************************************************/
int YM2610_status_port_0_A_r( int offset ) {
	return OPNBReadStatus( 0 ) & 0x83;
}

int YM2610_status_port_0_B_r( int offset ) {
	return OPNBReadADPCMStatus( 0 );
}

/************************************************/
/* Status Read for YM2610 - Chip 1				*/
/************************************************/
int YM2610_status_port_1_A_r( int offset ) {
	return OPNBReadStatus( 1 );
}

int YM2610_status_port_1_B_r( int offset ) {
	return OPNBReadADPCMStatus( 1 );
}

/************************************************/
/* Port Read for YM2610 - Chip 0				*/
/************************************************/
int YM2610_read_port_0_r( int offset ){
	if ( lastreg0 < 0x10 )
		return  AYReadReg(0,lastreg0);
	else
		if ( lastreg0 == 0xff )
			return 1;
	return 0;
}

/************************************************/
/* Port Read for YM2610 - Chip 1				*/
/************************************************/
int YM2610_read_port_1_r( int offset ){
	if ( lastreg2 < 0x10 )
		return  AYReadReg( 1, lastreg2 );
	else
		if ( lastreg2 == 0xff )
			return 1;
	return 0;
}

/************************************************/
/* Control Write for YM2610 - Chip 0			*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_control_port_0_A_w(int offset,int data){
	lastreg0 = data;
}

void YM2610_control_port_0_B_w(int offset,int data){
	lastreg1 = data;
}

/************************************************/
/* Control Write for YM2610 - Chip 1			*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_control_port_1_A_w(int offset,int data){
	lastreg2 = data;
}

void YM2610_control_port_1_B_w(int offset,int data){
	lastreg3 = data;
}

/************************************************/
/* Data Write for YM2610 - Chip 0				*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_data_port_0_A_w(int offset,int data){
	if( lastreg0 < 0x10 ){
		AYWriteReg( 0, lastreg0, data );
	} else{
		update_fm( 0 );
		OPNBWriteReg( 0, 0, lastreg0, data );
	}
}

void YM2610_data_port_0_B_w(int offset,int data){
	  update_fm( 0 );
	  OPNBWriteReg( 0, 1, lastreg1, data );
}

/************************************************/
/* Data Write for YM2610 - Chip 1				*/
/* Consists of 2 addresses						*/
/************************************************/
void YM2610_data_port_1_A_w(int offset,int data){
	if ( lastreg0 < 0x10 ) {
		AYWriteReg( 1, lastreg2, data );
	} else {
		update_fm( 1 );
		OPNBWriteReg( 1, 0, lastreg2, data );
	}
}
void YM2610_data_port_1_B_w(int offset,int data){
	update_fm( 1 );
	OPNBWriteReg( 1, 1, lastreg3, data );
}

/************************************************/
/* Sound Hardware Update						*/
/************************************************/
void YM2610_sh_update(void){
	int i, j;

	if (Machine->sample_rate == 0 ) return;

	OPNBUpdate();
	for( i = 0; i < intf->num; i++ ){
		sample_posFM[i] = 0;
		if( sample_bits == 16 ){
			/**** FM ch ****/
			osd_play_streamed_sample_16(channelFM+i*YM2610_NUMBUF   ,bufferFM[i*YM2610_NUMBUF  ],buffer_lenFM*2,emulation_rateFM,volumeFM[i] );
			osd_play_streamed_sample_16(channelFM+i*YM2610_NUMBUF+1 ,bufferFM[i*YM2610_NUMBUF+1],buffer_lenFM*2,emulation_rateFM,volumeFM[i] );
			/**** delta-T ADPCM ****/
			osd_play_streamed_sample_16(channelFM+i*YM2610_NUMBUF+2 ,bufferFM[i*YM2610_NUMBUF+2],buffer_lenFM*2,emulation_rateFM,volumeFM[i] );
			osd_play_streamed_sample_16(channelFM+i*YM2610_NUMBUF+3 ,bufferFM[i*YM2610_NUMBUF+3],buffer_lenFM*2,emulation_rateFM,volumeFM[i] );
			/**** ADPCM ****/
#ifdef OPNB_ADPCM_MIX
			osd_play_streamed_sample_16(channelFM+i*YM2610_NUMBUF+4 ,bufferFM[i*YM2610_NUMBUF+4],buffer_lenFM*2,emulation_rateFM,volumeFM[i] );
			osd_play_streamed_sample_16(channelFM+i*YM2610_NUMBUF+5 ,bufferFM[i*YM2610_NUMBUF+5],buffer_lenFM*2,emulation_rateFM,volumeFM[i] );
#else
			for( j = 4; j < YM2610_NUMBUF; j++ ){
				osd_play_streamed_sample_16(channelFM+i*YM2610_NUMBUF+j ,bufferFM[i*YM2610_NUMBUF+j],buffer_lenFM*2,emulation_rateFM,volumeFM[i] );
			}
#endif

		} else{
			/**** FM ch ****/
			osd_play_streamed_sample(channelFM+i*YM2610_NUMBUF   ,bufferFM[i*YM2610_NUMBUF  ],buffer_lenFM,emulation_rateFM,volumeFM[i] );
			osd_play_streamed_sample(channelFM+i*YM2610_NUMBUF+1 ,bufferFM[i*YM2610_NUMBUF+1],buffer_lenFM,emulation_rateFM,volumeFM[i] );
			/**** delta-T ADPCM ****/
			osd_play_streamed_sample(channelFM+i*YM2610_NUMBUF+2 ,bufferFM[i*YM2610_NUMBUF+2],buffer_lenFM,emulation_rateFM,volumeFM[i] );
			osd_play_streamed_sample(channelFM+i*YM2610_NUMBUF+3 ,bufferFM[i*YM2610_NUMBUF+3],buffer_lenFM,emulation_rateFM,volumeFM[i] );
			/**** ADPCM ****/
#ifdef OPNB_ADPCM_MIX
			osd_play_streamed_sample(channelFM+i*YM2610_NUMBUF+4 ,bufferFM[i*YM2610_NUMBUF+4],buffer_lenFM,emulation_rateFM,volumeFM[i] );
			osd_play_streamed_sample(channelFM+i*YM2610_NUMBUF+5 ,bufferFM[i*YM2610_NUMBUF+5],buffer_lenFM,emulation_rateFM,volumeFM[i] );
#else
			for( j = 4; j < YM2610_NUMBUF; j++ ){
				osd_play_streamed_sample(channelFM+i*YM2610_NUMBUF+j ,bufferFM[i*YM2610_NUMBUF+j],buffer_lenFM,emulation_rateFM,volumeFM[i] );
			}
#endif
		}
	}
}

/**************************************************/
/*   YM2610 left/right position change (TAITO)    */
/**************************************************/
void YM2610_pan( int lr, int v ){
	FMpan[lr] = v;
}

/**************** end of file ****************/


#endif
