#define NICK_BUBLBOBL_CHANGE
/***************************************************************************

  psgintf.c


  Many games use the AY-3-8910 to produce sound; the functions contained in
  this file make it easier to interface with it.

  Support interface AY-3-8910 : YM2203(DAC) : YM2203(OPL)

***************************************************************************/

#include "driver.h"
#include "psg.h"
#include "fm.h"

extern unsigned char No_FM;

static int emulation_rateAY;
static int buffer_lenAY;
static int emulation_rateFM;
static int buffer_lenFM;
static int sample_bits;

static struct PSGinterface *intf;

static int FMMode;
#define CHIP_AY8910 1
#define CHIP_YM2203_DAC 2
#define CHIP_YM2203_OPL 3   /* YM2203 with OPL chip */
#define CHIP_YM2608_DAC 4
#define CHIP_YM2612_DAC 5

static void *bufferAY[MAX_PSG];
static FMSAMPLE *bufferFM[MAX_PSG*2];

static int volumeAY[MAX_PSG];
static int volumeFM[MAX_PSG];

static int channelAY;

static int channelFM;

static void *Timer[MAX_PSG][2];
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

	for( i = 0 ; i < MAX_PSG ; i++ )
		Timer[i][0] = Timer[i][1] = 0;
	FMSetTimerHandler( TimerHandler , 0 );
}

unsigned char AYPortHandler(int num,int port, int iswrite, unsigned char val)
{
	if (iswrite)
	{
		if (port == 0)
		{
			if (intf->portAwrite[num]) (*intf->portAwrite[num])(0,val);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - write %02x to 8910 #%d Port A\n",cpu_getpc(),val,num);
		}
		else
		{
			if (intf->portBwrite[num]) (*intf->portBwrite[num])(1,val);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - write %02x to 8910 #%d Port B\n",cpu_getpc(),val,num);
		}
	}
	else
	{
		if (port == 0)
		{
			if (intf->portAread[num]) return (*intf->portAread[num])(0);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - read 8910 #%d Port A\n",cpu_getpc(),num);
		}
		else
		{
			if (intf->portBread[num]) return (*intf->portBread[num])(1);
else if (errorlog) fprintf(errorlog,"PC %04x: warning - read 8910 #%d Port B\n",cpu_getpc(),num);
		}
	}

	return 0;
}

int AY8910_sh_start(struct PSGinterface *interface)
{
	int i;/*,j;*/
	int rate = Machine->sample_rate;

	intf = interface;

	buffer_lenAY = rate / Machine->drv->frames_per_second;
	emulation_rateAY = buffer_lenAY * Machine->drv->frames_per_second;
	sample_bits = Machine->sample_bits;

	for (i = 0;i < MAX_8910;i++)
	{
		bufferAY[i] = 0;
		bufferFM[i] = 0;
	}
	/* SSG initialize */
	for (i = 0;i < intf->num;i++)
	{
		if ((bufferAY[i] = malloc(3*(sample_bits/8)*buffer_lenAY)) == 0)
		{
			while (--i >= 0) free(bufferAY[i]);
			return 1;
		}
	}
	if (AYInit(intf->num,intf->clock,emulation_rateAY,sample_bits,buffer_lenAY,bufferAY) )
	{
		for (i = 0;i < intf->num;i++) free(bufferAY[i]);
		return 1;
	}
	channelAY = get_play_channels( 3*intf->num );

	/* volume setup (do not support YM2203) */
	for (i = 0;i < intf->num;i++)
	{
		int gain;

		/* for 8910 */
		volumeAY[i] = intf->volume[i] & 0xff;
		gain = (intf->volume[i] >> 8) & 0xff;
		AYSetGain(i,gain);
	}
	return 0;

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

int YM2203_sh_start(struct PSGinterface *interface)
{
	int i;
	int rate = Machine->sample_rate;

	if( AY8910_sh_start(interface) ) return 1;

	/* FM init */
	if( No_FM ) FMMode = CHIP_YM2203_DAC;
	else        FMMode = CHIP_YM2203_OPL;

	intf = interface;
	buffer_lenFM = rate / Machine->drv->frames_per_second;
	emulation_rateFM = buffer_lenFM * Machine->drv->frames_per_second;

	/* OPN buffer initialize */
	for (i = 0;i < intf->num;i++)
	{
		if ((bufferFM[i] = malloc((sample_bits/8)*buffer_lenFM)) == 0)
		{
			while (--i >= 0) free(bufferFM[i]);
			AY8910_sh_stop();
			return 1;
		}
	}
	/* Timer Handler set */
	timer_callback = timer_callback_2203;
	FMTimerInit();
	/* Initialize FM emurator */
	if (YM2203Init(intf->num,intf->clock,emulation_rateFM,sample_bits,buffer_lenFM,bufferFM) == 0)
	{
		channelFM = get_play_channels( intf->num );
		/* volume setup */
		for (i = 0;i < intf->num;i++)
		{
			volumeFM[i] = intf->volume[i]>>16; /* high 16 bit */
			if( volumeFM[i] > 255 ) volumeFM[i] = 255;
		}
		/* Ready */
		return 0;
	}
	/* error */
	for (i = 0;i < intf->num;i++) free(bufferFM[i]);
	AY8910_sh_stop();
	return 1;
}

void AY8910_sh_stop(void)
{
	int i;

	AYShutdown();
	for (i = 0;i < intf->num;i++){
		free(bufferAY[i]);
	}
}

void YM2203_sh_stop(void)
{
	int i;

	AY8910_sh_stop();
	if( FMMode == CHIP_YM2203_DAC )
	{
		YM2203Shutdown();
		for (i = 0;i < intf->num;i++){
			free(bufferFM[i]);
		}
	}
}


/* AY8910 interface */
int AY8910_read_port_0_r(int offset) { return AY8910Read(0,1); }
int AY8910_read_port_1_r(int offset) { return AY8910Read(1,1); }
int AY8910_read_port_2_r(int offset) { return AY8910Read(2,1); }
int AY8910_read_port_3_r(int offset) { return AY8910Read(3,1); }
int AY8910_read_port_4_r(int offset) { return AY8910Read(4,1); }

void AY8910_control_port_0_w(int offset,int data) { AY8910Write(0,0,data);}
void AY8910_control_port_1_w(int offset,int data) { AY8910Write(1,0,data);}
void AY8910_control_port_2_w(int offset,int data) { AY8910Write(2,0,data);}
void AY8910_control_port_3_w(int offset,int data) { AY8910Write(3,0,data);}
void AY8910_control_port_4_w(int offset,int data) { AY8910Write(4,0,data);}

void AY8910_write_port_0_w(int offset,int data) { AY8910Write(0,1,data);}
void AY8910_write_port_1_w(int offset,int data) { AY8910Write(1,1,data);}
void AY8910_write_port_2_w(int offset,int data) { AY8910Write(2,1,data);}
void AY8910_write_port_3_w(int offset,int data) { AY8910Write(3,1,data);}
void AY8910_write_port_4_w(int offset,int data) { AY8910Write(4,1,data);}

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

void AY8910_sh_update(void)
{
	int i,ch;

	if (Machine->sample_rate == 0 ) return;
	/* update AY-3-8910 */
	AYUpdate();
	for (i = 0;i < intf->num;i++)
	{
		if( sample_bits == 16 )
		{
			osd_play_streamed_sample_16(channelAY+3*i,bufferAY[i],2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
			osd_play_streamed_sample_16(channelAY+3*i+1,(signed short *)bufferAY[i]+buffer_lenAY,2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
			osd_play_streamed_sample_16(channelAY+3*i+2,(signed short *)bufferAY[i]+2*buffer_lenAY,2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
		}
		else
		{
			osd_play_streamed_sample(channelAY+3*i,bufferAY[i],buffer_lenAY,emulation_rateAY,volumeAY[i]);
			osd_play_streamed_sample(channelAY+3*i+1,(signed char *)bufferAY[i]+buffer_lenAY,buffer_lenAY,emulation_rateAY,volumeAY[i]);
			osd_play_streamed_sample(channelAY+3*i+2,(signed char *)bufferAY[i]+2*buffer_lenAY,buffer_lenAY,emulation_rateAY,volumeAY[i]);
		}
	}
}

void YM2203_sh_update(void)
{
	int i;

	if (Machine->sample_rate == 0 ) return;
	/* update AY-3-8910 */
	AYUpdate();
	if( FMMode == CHIP_YM2203_DAC ){	/* DIGITAL sound emurator */
		/* OPN DAC */
		YM2203Update();
		for (i = 0;i < intf->num;i++)
		{
			if( sample_bits == 16 )
			{
				osd_play_streamed_sample_16(channelAY+3*i,bufferAY[i],2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample_16(channelAY+3*i+1,(signed short *)bufferAY[i]+buffer_lenAY,2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample_16(channelAY+3*i+2,(signed short *)bufferAY[i]+2*buffer_lenAY,2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample_16(channelFM+i,bufferFM[i],2*buffer_lenFM,emulation_rateFM,volumeFM[i]);
			}
			else
			{
				osd_play_streamed_sample(channelAY+3*i,bufferAY[i],buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample(channelAY+3*i+1,(signed char *)bufferAY[i]+buffer_lenAY,buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample(channelAY+3*i+2,(signed char *)bufferAY[i]+2*buffer_lenAY,buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample(channelFM+i,bufferFM[i],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			}
		}
	}else{
		osd_ym2203_update();
		for (i = 0;i < intf->num;i++)
		{
			if( sample_bits == 16 )
			{
				osd_play_streamed_sample_16(channelAY+3*i,bufferAY[i],2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample_16(channelAY+3*i+1,(signed short *)bufferAY[i]+buffer_lenAY,2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample_16(channelAY+3*i+2,(signed short *)bufferAY[i]+2*buffer_lenAY,2*buffer_lenAY,emulation_rateAY,volumeAY[i]);
			}
			else
			{
				osd_play_streamed_sample(channelAY+3*i,bufferAY[i],buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample(channelAY+3*i+1,(signed char *)bufferAY[i]+buffer_lenAY,buffer_lenAY,emulation_rateAY,volumeAY[i]);
				osd_play_streamed_sample(channelAY+3*i+2,(signed char *)bufferAY[i]+2*buffer_lenAY,buffer_lenAY,emulation_rateAY,volumeAY[i]);
			}
		}
	}
}

#ifdef BUILD_YM2608
/* YM2608 interface */
int YM2608_address0_0_r(int offset) { return (int)YM2608Read(0,0); }
int YM2608_address0_1_r(int offset) { return (int)YM2608Read(1,0); }
int YM2608_address1_0_r(int offset) { return (int)YM2608Read(0,1); }
int YM2608_address1_1_r(int offset) { return (int)YM2608Read(1,1); }
int YM2608_address2_0_r(int offset) { return (int)YM2608Read(0,2); }
int YM2608_address2_1_r(int offset) { return (int)YM2608Read(1,2); }
int YM2608_address3_0_r(int offset) { return (int)YM2608Read(0,3); }
int YM2608_address3_1_r(int offset) { return (int)YM2608Read(1,3); }

void YM2608_address0_0_w(int offset,int data) { YM2608Write(0,0,data); }
void YM2608_address0_1_w(int offset,int data) { YM2608Write(1,0,data); }
void YM2608_address1_0_w(int offset,int data) { YM2608Write(0,1,data); }
void YM2608_address1_1_w(int offset,int data) { YM2608Write(1,1,data); }
void YM2608_address2_0_w(int offset,int data) { YM2608Write(0,2,data); }
void YM2608_address2_1_w(int offset,int data) { YM2608Write(1,2,data); }
void YM2608_address3_0_w(int offset,int data) { YM2608Write(0,3,data); }
void YM2608_address3_1_w(int offset,int data) { YM2608Write(1,3,data); }


/* Timer overflow callback from timer.c */
static void timer_callback_2608(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	Timer[n][c] = 0;
	if( YM2608TimerOver(n,c) )
	{	/* IRQ is active */;
		/* User Interrupt call */
		if(intf->handler[n]) intf->handler[n]();
	}
}

int  YM2608_sh_start(struct YM2608interface *interface)
{
	int i;
	int rate = Machine->sample_rate;

	if( AY8910_sh_start(interface) ) return 1;

	intf = interface;
	buffer_lenFM = rate / Machine->drv->frames_per_second;
	emulation_rateFM = buffer_lenFM * Machine->drv->frames_per_second;

	FMMode = CHIP_YM2608_DAC;

	/* OPN buffer initialize */
	for (i = 0;i < intf->num*2;i++)
	{
		if ((bufferFM[i] = malloc((sample_bits/8)*buffer_lenFM)) == 0)
		{
			while (--i >= 0) free(bufferFM[i]);
			AY8910_sh_stop();
			return 1;
		}
	}
	/* Timer Handler set */
	timer_callback = timer_callback_2608;
	FMTimerInit();
	/* Initialize FM emurator */
	if (YM2608Init(intf->num,intf->clock,emulation_rateFM,sample_bits,buffer_lenFM,bufferFM) == 0)
	{
		channelFM = get_play_channels( intf->num*2 );
		/* volume setup */
		for (i = 0;i < intf->num;i++)
		{
			volumeFM[i] = intf->volume[i]>>16; /* high 16 bit */
			if( volumeFM[i] > 255 ) volumeFM[i] = 255;
		}
		/* Ready */
		return 0;
	}
	/* error */
	for (i = 0;i < intf->num;i++) free(bufferFM[i]);
	AY8910_sh_stop();
	return 1;
}

void YM2608_sh_stop(void)
{
	int i;

	AY8910_sh_stop();
	if( FMMode == CHIP_YM2608_DAC )
	{
		YM2608Shutdown();
		for (i = 0;i < intf->num;i++){
			free(bufferFM[i]);
		}
	}
}


void YM2608_sh_update(void)
{
	int i,ch;

	if (Machine->sample_rate == 0 ) return;
	/* update AY-3-8910 */
	AYUpdate();

	YM2608Update();
	ch = 0;
	for (i = 0;i < intf->num;i++)
	{
		if( sample_bits == 16 )
		{
			osd_play_streamed_sample_16(channelAY+i,bufferAY[i],buffer_lenFM,emulation_rateFM,volumeAY[i]);
			osd_play_streamed_sample_16(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;
			osd_play_streamed_sample_16(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;
		}
		else
		{
			osd_play_streamed_sample(channelAY+i,bufferAY[i],buffer_lenFM,emulation_rateFM,volumeAY[i]);
			osd_play_streamed_sample(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;
			osd_play_streamed_sample(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;

		}
	}
}
#endif	/* BUILD_YM2608 */


#ifdef BUILD_YM2612
/* YM2612 interface */
int YM2612_address0_0_r(int offset) { return (int)YM2612Read(0,0); }
int YM2612_address0_1_r(int offset) { return (int)YM2612Read(1,0); }
int YM2612_address1_0_r(int offset) { return (int)YM2612Read(0,1); }
int YM2612_address1_1_r(int offset) { return (int)YM2612Read(1,1); }
int YM2612_address2_0_r(int offset) { return (int)YM2612Read(0,2); }
int YM2612_address2_1_r(int offset) { return (int)YM2612Read(1,2); }
int YM2612_address3_0_r(int offset) { return (int)YM2612Read(0,3); }
int YM2612_address3_1_r(int offset) { return (int)YM2612Read(1,3); }

void YM2612_address0_0_w(int offset,int data) { YM2612Write(0,0,data); }
void YM2612_address0_1_w(int offset,int data) { YM2612Write(1,0,data); }
void YM2612_address1_0_w(int offset,int data) { YM2612Write(0,1,data); }
void YM2612_address1_1_w(int offset,int data) { YM2612Write(1,1,data); }
void YM2612_address2_0_w(int offset,int data) { YM2612Write(0,2,data); }
void YM2612_address2_1_w(int offset,int data) { YM2612Write(1,2,data); }
void YM2612_address3_0_w(int offset,int data) { YM2612Write(0,3,data); }
void YM2612_address3_1_w(int offset,int data) { YM2612Write(1,3,data); }

/* Timer overflow callback from timer.c */
static void timer_callback_2612(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	Timer[n][c] = 0;
	if( YM2612TimerOver(n,c) )
	{	/* IRQ is active */;
		/* User Interrupt call */
		if(intf->handler[n]) intf->handler[n]();
	}
}

int  YM2612_sh_start(struct YM2612interface *interface)
{
	int i;
	int rate = Machine->sample_rate;

	if( AY8910_sh_start(interface) ) return 1;

	intf = interface;
	buffer_lenFM = rate / Machine->drv->frames_per_second;
	emulation_rateFM = buffer_lenFM * Machine->drv->frames_per_second;

	FMMode = CHIP_YM2612_DAC;

	/* OPN buffer initialize */
	for (i = 0;i < intf->num*2;i++)
	{
		if ((bufferFM[i] = malloc((sample_bits/8)*buffer_lenFM)) == 0)
		{
			while (--i >= 0) free(bufferFM[i]);
			AY8910_sh_stop();
			return 1;
		}
	}
	/* Timer Handler set */
	timer_callback = timer_callback_2612;
	FMTimerInit();
	/* Initialize FM emurator */
	if (YM2612Init(intf->num,intf->clock,emulation_rateFM,sample_bits,buffer_lenFM,bufferFM) == 0)
	{
		channelFM = get_play_channels( intf->num*2 );
		/* volume setup */
		for (i = 0;i < intf->num;i++)
		{
			volumeFM[i] = intf->volume[i]>>16; /* high 16 bit */
			if( volumeFM[i] > 255 ) volumeFM[i] = 255;
		}
		/* Ready */
		return 0;
	}
	/* error */
	for (i = 0;i < intf->num;i++) free(bufferFM[i]);
	AY8910_sh_stop();
	return 1;
}

void YM2612_sh_stop(void)
{
	int i;

	AY8910_sh_stop();
	if( FMMode == CHIP_YM2612_DAC )
	{
		YM2612Shutdown();
		for (i = 0;i < intf->num;i++){
			free(bufferFM[i]);
		}
	}
}


void YM2612_sh_update(void)
{
	int i,ch;

	if (Machine->sample_rate == 0 ) return;
	/* update AY-3-8910 */
	AYUpdate();

	YM2612Update();
	ch = 0;
	for (i = 0;i < intf->num;i++)
	{
		if( sample_bits == 16 )
		{
			osd_play_streamed_sample_16(channelAY+i,bufferAY[i],buffer_lenFM,emulation_rateFM,volumeAY[i]);
			osd_play_streamed_sample_16(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;
			osd_play_streamed_sample_16(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;
		}
		else
		{
			osd_play_streamed_sample(channelAY+i,bufferAY[i],buffer_lenFM,emulation_rateFM,volumeAY[i]);
			osd_play_streamed_sample(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;
			osd_play_streamed_sample(channelFM+ch,bufferFM[ch],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			ch++;

		}
	}
}
#endif	/* BUILD_YM2612 */



