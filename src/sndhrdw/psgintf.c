/***************************************************************************

  psgintf.c


  Many games use the AY-3-8910 to produce sound; the functions contained in
  this file make it easier to interface with it.

  Support interface AY-3-8910 : YM2203(DAC) : YM2203(OPL)

***************************************************************************/

#include "driver.h"
#include "psg.h"
#include "fm.h"
#include "sndhrdw/psgintf.h"
#include "sndhrdw/generic.h"

extern unsigned char No_FM;

#define MIN_SLICE_8910 10	/* minimum update step for AY-3-8910 (226usec @ 44100Hz) */
#define MIN_SLICE_2203 44	/* minimum update step for YM2203 (1msec @ 44100Hz)      */

static int emulation_rateAY;
static int buffer_lenAY;
static int emulation_rateFM;
static int buffer_lenFM;

static struct PSGinterface *intf;

static int FMMode;
#define CHIP_AY8910 1
#define CHIP_YM2203_DAC 2
#define CHIP_YM2203_OPL 3   /* YM2203 with OPL chip */


static SAMPLE *bufferAY[MAX_PSG];
#ifdef OPN_MIX
static FMSAMPLE *bufferFM[MAX_PSG];
#else
static FMSAMPLE *bufferFM[MAX_PSG*3];
static int ofstA;
static int ofstB;
static int ofstC;

#endif
static int sample_posAY[MAX_PSG];
static int sample_posFM[MAX_PSG];

static int volumeAY[MAX_PSG];
static int volumeFM[MAX_PSG];

static int channelAY;
#ifdef OPN_MIX
static int channelFM;
#else
static int channelFMa;
static int channelFMb;
static int channelFMc;
#endif

#ifdef macintosh
#include "mac8910.c"
#endif

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



int PSG_sh_start(struct PSGinterface *interface , int type , int rate)
{
	int i,j;

	intf = interface;

	buffer_lenAY = rate / Machine->drv->frames_per_second;
	emulation_rateAY = buffer_lenAY * Machine->drv->frames_per_second;

	buffer_lenFM = rate / Machine->drv->frames_per_second;
	emulation_rateFM = buffer_lenFM * Machine->drv->frames_per_second;

#ifndef OPN_MIX
	ofstA = intf->num;
	ofstB = ofstA*2;
	ofstC = ofstB*3;
#endif
	FMMode = type;

	for (i = 0;i < MAX_8910;i++)
	{
		sample_posFM[i] = 0;
		sample_posAY[i] = 0;

		bufferAY[i] = 0;
#ifdef OPN_MIX
		bufferFM[i] = 0;
#else
		bufferFM[i*3  ] = 0;
		bufferFM[i*3+1] = 0;
		bufferFM[i*3+2] = 0;
#endif
	}
	/* SSG initialize */
	for (i = 0;i < intf->num;i++)
	{
		if ((bufferAY[i] = malloc(sizeof(SAMPLE)*buffer_lenAY)) == 0)
		{
			while (--i >= 0) free(bufferAY[i]);
			return 1;
		}
//		for (j = 0;j < buffer_lenAY ;j++) bufferAY[i][j] = AUDIO_CONV(0);
	}
	if (AYInit(intf->num,intf->clock,emulation_rateAY,buffer_lenAY,bufferAY) == 0)
	{
		channelAY = get_play_channels( intf->num );

		/* volume setup (do not support YM2203) */
		for (i = 0;i < intf->num;i++)
		{
			int gain;


			/* for 8910 and SSG of YM2203 */
			volumeAY[i] = intf->volume[i] & 0xff;
			gain = (intf->volume[i] >> 8) & 0xff;
			AYSetGain(i,gain);

			/* for FM of YM2203 */
			volumeFM[i] = intf->volume[i]>>16; /* high 16 bit */
			if( volumeFM[i] > 255 )
			{
				/* do not support gain control for FM emurator */
				volumeFM[i] = 255;
			}
		}
		if( FMMode == CHIP_AY8910 ) return 0; /* not use OPN */

		/* OPN initialize */
#ifdef OPN_MIX
		for (i = 0;i < intf->num;i++)
#else
		for (i = 0;i < intf->num *3;i++)
#endif
		{
			if ((bufferFM[i] = malloc(sizeof(FMSAMPLE)*buffer_lenFM)) == 0)
			{
			while (--i >= 0) free(bufferFM[i]);
			for (i = 0;i < intf->num;i++) free(bufferAY[i]);
			return 1;
			}
			for (j = 0;j < buffer_lenFM ;j++) bufferFM[i][j] = FMOUT_0;
		}

		if (OPNInit(intf->num,intf->clock,emulation_rateFM,buffer_lenFM,bufferFM) == 0)
		{
#ifdef OPN_MIX
			channelFM = get_play_channels( intf->num );
#else
			channelFMa = get_play_channels( intf->num * 3 );
			channelFMb = channelFMa + intf->num;
			channelFMc = channelFMb + intf->num;
#endif
			return 0;
		}
		/* error */
#ifdef OPN_MIX
		for (i = 0;i < intf->num;i++) free(bufferFM[i]);
#else
		for (i = 0;i < intf->num *3;i++) free(bufferFM[i]);
#endif
	}
	for (i = 0;i < intf->num;i++) free(bufferAY[i]);
	return 1;
}

int AY8910_sh_start(struct PSGinterface *interface)
{
    return PSG_sh_start(interface , CHIP_AY8910, Machine->sample_rate);
}

int YM2203_sh_start(struct PSGinterface *interface)
{
	int type;

	if( No_FM ) type = CHIP_YM2203_DAC;
	else        type = CHIP_YM2203_OPL;
    return PSG_sh_start(interface , type , Machine->sample_rate );
}

void YM2203_sh_stop(void)
{
	int i,j;

	AYShutdown();
	for (i = 0;i < intf->num;i++){
		free(bufferAY[i]);
	}

	if( FMMode == CHIP_YM2203_DAC )
	{
		OPNShutdown();
		for (i = 0;i < intf->num;i++){
#ifdef OPN_MIX
			free(bufferFM[i]);
#else
			for (j = 0;j < 3;j++){
				free(bufferFM[i*3+j]);
			}
#endif
		}
	}
}

static void update_ay(int chip)
{
	int totcycles,leftcycles,newpos;

	totcycles = cpu_getfperiod();
	leftcycles = cpu_getfcount();
	newpos = buffer_lenAY * (totcycles-leftcycles) / totcycles;

	if( newpos - sample_posAY[chip] < MIN_SLICE_8910 ) return;
	AYUpdateOne(chip, newpos );

	sample_posAY[chip] = newpos;
}

static inline void update_fm(int chip)
{
	int totcycles,leftcycles,newpos;

	leftcycles = cpu_getfcount();
	totcycles = cpu_getfperiod();
	newpos = buffer_lenFM * (totcycles-leftcycles) / totcycles;

	if( newpos - sample_posFM[chip] < MIN_SLICE_2203 ) return;
	OPNUpdateOne( chip , newpos );
	sample_posFM[chip] = newpos;
}

static int lastreg0,lastreg1,lastreg2,lastreg3,lastreg4;

int PSG_read_port_0_r(int offset)
{
	if( lastreg0<16 ) return AYReadReg(0,lastreg0);
	if( FMMode == CHIP_YM2203_DAC ) return OPNReadStatus(0);
	return 0;
}
int PSG_read_port_1_r(int offset)
{
	if( lastreg1<16 ) return AYReadReg(1,lastreg1);
	if( FMMode == CHIP_YM2203_DAC ) return OPNReadStatus(1);
	return 0;
}
int PSG_read_port_2_r(int offset)
{
	if( lastreg2<16 ) return AYReadReg(2,lastreg2);
	if( FMMode == CHIP_YM2203_DAC ) return OPNReadStatus(2);
	return 0;
}
int PSG_read_port_3_r(int offset)
{
	if( lastreg3<16 ) return AYReadReg(3,lastreg3);
	if( FMMode == CHIP_YM2203_DAC ) return OPNReadStatus(3);
	return 0;
}
int PSG_read_port_4_r(int offset)
{
	if( lastreg4<16 ) return AYReadReg(4,lastreg4);
	return 0;
}
int YM2203_status_port_0_r(int offset)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		update_fm(0);
		return OPNReadStatus(0);
	}
	return 0;
}
int YM2203_status_port_1_r(int offset)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		update_fm(1);
		return OPNReadStatus(1);
	}
	return 0;
}
int YM2203_status_port_2_r(int offset)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		update_fm(2);
		return OPNReadStatus(2);
	}
	return 0;
}
int YM2203_status_port_3_r(int offset)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		update_fm(3);
		return OPNReadStatus(3);
	}
	return 0;
}
int YM2203_status_port_4_r(int offset)
{
	if( FMMode == CHIP_YM2203_DAC )
	{
		update_fm(4);
		return OPNReadStatus(4);
	}
	return 0;
}


void PSG_control_port_0_w(int offset,int data)
{
	lastreg0 = data;
}
void PSG_control_port_1_w(int offset,int data)
{
	lastreg1 = data;
}
void PSG_control_port_2_w(int offset,int data)
{
	lastreg2 = data;
}
void PSG_control_port_3_w(int offset,int data)
{
	lastreg3 = data;
}
void PSG_control_port_4_w(int offset,int data)
{
	lastreg4 = data;
}

void PSG_write_port_0_w(int offset,int data)
{
	if( lastreg0<16 )
	{
		update_ay(0);
		AYWriteReg(0,lastreg0,data);
	}
	else
	{
		if( FMMode == CHIP_YM2203_DAC ){
			update_fm(0);
			OPNWriteReg(0,lastreg0,data);
		}else if( FMMode == CHIP_YM2203_OPL ){
			osd_ym2203_write(0,lastreg0,data);
		}
		else
		{
if (errorlog) fprintf(errorlog,"warning: write to 8910 #0 register #%d. It could be a 2203 or another compatible chip.\n",lastreg0);
		}
	}
}
void PSG_write_port_1_w(int offset,int data)
{
	if( lastreg1<16 )
	{
		update_ay(1);
		AYWriteReg(1,lastreg1,data);
	}
	else
	{
		if( FMMode == CHIP_YM2203_DAC ){
			update_fm(1);
			OPNWriteReg(1,lastreg1,data);
		}else if( FMMode == CHIP_YM2203_OPL ){
			osd_ym2203_write(1,lastreg1,data);
		}
		else
		{
if (errorlog) fprintf(errorlog,"warning: write to 8910 #1 register #%d. It could be a 2203 or another compatible chip.\n",lastreg1);
		}
	}
}
void PSG_write_port_2_w(int offset,int data)
{
	if( lastreg2<16 )
	{
		update_ay(2);
		AYWriteReg(2,lastreg2,data);
	}
	else
	{
		if( FMMode == CHIP_YM2203_DAC ){
			update_fm(2);
			OPNWriteReg(2,lastreg2,data);
		}else if( FMMode == CHIP_YM2203_OPL ){
			osd_ym2203_write(2,lastreg2,data);
		}
		else
		{
if (errorlog) fprintf(errorlog,"warning: write to 8910 #2 register #%d. It could be a 2203 or another compatible chip.\n",lastreg2);
		}
	}
}
void PSG_write_port_3_w(int offset,int data)
{
	if( lastreg3<16 )
	{
		update_ay(3);
		AYWriteReg(3,lastreg3,data);
	}
	else
	{
		if( FMMode == CHIP_YM2203_DAC ){
			update_fm(3);
			OPNWriteReg(3,lastreg3,data);
		}else if( FMMode == CHIP_YM2203_OPL ){
			osd_ym2203_write(3,lastreg3,data);
		}
		else
		{
if (errorlog) fprintf(errorlog,"warning: write to 8910 #3 register #%d. It could be a 2203 or another compatible chip.\n",lastreg3);
		}
	}
}

void PSG_write_port_4_w(int offset,int data)
{
	if( lastreg4<16 )
	{
		update_ay(4);
		AYWriteReg(4,lastreg4,data);
	}
	else
	{
		if( FMMode == CHIP_YM2203_DAC ){
			update_fm(4);
			OPNWriteReg(4,lastreg4,data);
		}else if( FMMode == CHIP_YM2203_OPL ){
			osd_ym2203_write(4,lastreg4,data);
		}
		else
		{
if (errorlog) fprintf(errorlog,"warning: write to 8910 #4 register #%d. It could be a 2203 or another compatible chip.\n",lastreg4);
		}
	}
}

void PSG_sh_update(void)
{
	int i,ch;

	if (Machine->sample_rate == 0 ) return;
	/* update AY-3-8910 */
	AYUpdate();

	if( FMMode == CHIP_YM2203_DAC ){	/* DIGITAL sound emurator */
		/* OPN DAC */
		OPNUpdate();
		ch = 0;
		for (i = 0;i < intf->num;i++)
		{
			sample_posAY[i] = sample_posFM[i] = 0;
#ifdef OPN_MIX
			osd_play_streamed_sample(channelAY+i,bufferAY[i],buffer_lenFM,emulation_rateFM,volumeAY[i]>>2);
			osd_play_streamed_sample(channelFM+i,bufferFM[i],buffer_lenFM,emulation_rateFM,volumeFM[i]);
#else
			osd_play_streamed_sample(channelAY +i,bufferAY[i]    ,buffer_lenFM,emulation_rateFM,volumeAY[i]);
			osd_play_streamed_sample(channelFMa+i,bufferFM[i*3  ],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			osd_play_streamed_sample(channelFMb+i,bufferFM[i*3+1],buffer_lenFM,emulation_rateFM,volumeFM[i]);
			osd_play_streamed_sample(channelFMc+i,bufferFM[i*3+2],buffer_lenFM,emulation_rateFM,volumeFM[i]);
#endif
		}
	}else{
		if( FMMode == CHIP_YM2203_OPL ){	/* OPL chip emurator */
			osd_ym2203_update();
		}
		/* SSG without OPN DAC */
		for (i = 0;i < intf->num;i++)
		{
			sample_posAY[i] = 0;
			osd_play_streamed_sample(channelAY+i,bufferAY[i],buffer_lenAY,emulation_rateAY,volumeAY[i]);
		}
	}

}