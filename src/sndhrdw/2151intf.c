/***************************************************************************

  2151intf.c

  Support interface YM2151(OPM)

***************************************************************************/

#include "driver.h"
#include "fm.h"
#include "ym2151.h"

extern unsigned char No_FM;



#define MIN_SLICE 44

static int emulation_rate;
static int buffer_len;
static int sample_bits;

static int emu_mode;		/* 0 = Tatsuyuki's emurator */
							/* 1 = Jarek's emurator     */

static struct YM2151interface *intf;

static int FMMode;
#define CHIP_YM2151_DAC 4
#define CHIP_YM2151_OPL 5

static FMSAMPLE *bufferFM[MAX_2151*YM2151_NUMBUF];
static int sample_posFM[MAX_2151];

static int volume[MAX_2151];
static int channel;


int YM2151_sh_start(struct YM2151interface *interface,int mode)
{
	int i,j;
/*	int rate = 22050; */
	int rate = Machine->sample_rate;

	if( rate == 0 ) rate = 1000;	/* kludge to prevent nasty crashes */

	intf = interface;

	emu_mode = mode;

	buffer_len = rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;
	sample_bits = Machine->sample_bits;

	FMMode = CHIP_YM2151_DAC;
/*	if( No_FM ) FMMode = CHIP_YM2151_DAC;*/
/*	else        FMMode = CHIP_YM2151_OPL;*/

	if( emu_mode == 0 )
	{	/* Tatsuyuki's */
		for (i = 0;i < MAX_2151;i++)
		{
			sample_posFM[i] = 0;
			for( j = 0 ; j < YM2151_NUMBUF ; j++ )
				bufferFM[i*YM2151_NUMBUF+j] = 0;
		}
		/* OPM initialize */
		for (i = 0;i < intf->num*YM2151_NUMBUF;i++)
		{
			if ((bufferFM[i] = malloc((sample_bits/8)*buffer_len)) == 0)
			{
				while (--i >= 0) free(bufferFM[i]);
				return 1;
			}
/*			for (j = 0;j < buffer_len ;j++) bufferFM[i][j] = FMOUT_0;*/
		}
		if (OPMInit(intf->num,intf->clock,emulation_rate,sample_bits,buffer_len,bufferFM) == 0)
		{
			int vol_max = 255;
			int gain = 0xffff;

			channel = get_play_channels(intf->num*YM2151_NUMBUF);
			/* volume calcration */
			for (i = 0; i < intf->num; i++)
			{
				if( vol_max < intf->volume[i] ) vol_max = intf->volume[i];
			}
			if( vol_max > 255 ) gain = gain * vol_max / 255;
			/* gain set */
/*			FMSetGain( gain );*/
			for (i = 0; i < intf->num; i++)
			{
				volume[i] = intf->volume[i] * 0xffff / gain;
			}
			for (i = 0; i < intf->num; i++)
				OPMSetIrqHandler (i, interface->handler[i]);
			return 0;
		}
		/* error */
		for (i = 0;i < intf->num *YM2151_NUMBUF;i++) free(bufferFM[i]);
		return 1;
	}
	else
	{	/* Jarek's */
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
		if (YMInit(intf->num,intf->clock,emulation_rate,sample_bits,buffer_len,bufferFM) == 0)
		{
			channel=get_play_channels(intf->num);
			for (i = 0; i < intf->num; i++)
				YMSetIrqHandler(i,intf->handler[i]);
			return 0;
		}
		else return 1;
	}
}

void YM2151_sh_stop(void)
{
	int i;

	if( FMMode == CHIP_YM2151_DAC )
	{
		if (emu_mode == 0)
		{
			for (i = 0; i < intf->num; i++)
				OPMSetIrqHandler (i, 0);
			OPMShutdown();
		}
		else
			YMShutdown();
		for (i = 0;i < intf->num *YM2151_NUMBUF;i++) free(bufferFM[i]);
	}
}

INLINE void update_opm(int chip)
{
	int newpos;


	newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

	if( newpos - sample_posFM[chip] < MIN_SLICE ) return;
	if( emu_mode == 0 ) OPMUpdateOne(chip , newpos );
	else                YM2151UpdateOne(chip , newpos );
	sample_posFM[chip] = newpos;
}

static int lastreg0,lastreg1,lastreg2;

int YM2151_status_port_0_r(int offset)
{
	update_opm(0);
	if( emu_mode == 0 ) return OPMReadStatus(0);
	else                return YMReadReg(0);
}

int YM2151_status_port_1_r(int offset)
{
	update_opm(1);
	if( emu_mode == 0 ) return OPMReadStatus(1);
	else                return YMReadReg(1);
}

int YM2151_status_port_2_r(int offset)
{
	update_opm(2);
	if( emu_mode == 0 ) return OPMReadStatus(2);
	else                return YMReadReg(2);
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
	if( FMMode == CHIP_YM2151_DAC )
	{
		update_opm(0);
		if( emu_mode == 0 ) OPMWriteReg(0,lastreg0,data);
		else                YMWriteReg(0,lastreg0,data);
	} else if( FMMode == CHIP_YM2151_OPL )
	{
		opm_to_opn(0,lastreg0,data);
	}
}

void YM2151_data_port_1_w(int offset,int data)
{
	if( FMMode == CHIP_YM2151_DAC ){
		update_opm(1);
		if( emu_mode == 0 ) OPMWriteReg(1,lastreg1,data);
		else                YMWriteReg(1,lastreg1,data);
	}else if( FMMode == CHIP_YM2151_OPL ){
		opm_to_opn(1,lastreg1,data);
	}
}

void YM2151_data_port_2_w(int offset,int data)
{
	if( FMMode == CHIP_YM2151_DAC ){
		update_opm(2);
		if( emu_mode == 0 ) OPMWriteReg(2,lastreg2,data);
		else                YMWriteReg(2,lastreg2,data);
	}else if( FMMode == CHIP_YM2151_OPL ){
		opm_to_opn(2,lastreg2,data);
	}
}

void YM2151_sh_update(void)
{
	int i;

	if (Machine->sample_rate == 0 ) return;

	if( FMMode == CHIP_YM2151_DAC )
	{	/* DIGITAL sound emurator */
		if( emu_mode == 0 )
		{	/* Tatsuyuki's */
			OPMUpdate();
			for (i = 0;i < intf->num;i++)
			{
				if( sample_bits == 16 )
				{
					/* Left channel  */
					osd_play_streamed_sample_16(channel+i*2   ,bufferFM[i*2  ],2*buffer_len,emulation_rate,volume[i]);
					/* Right channel */
					osd_play_streamed_sample_16(channel+i*2+1 ,bufferFM[i*2+1],2*buffer_len,emulation_rate,volume[i]);
				}
				else
				{
					/* Left channel  */
					osd_play_streamed_sample(channel+i*2   ,bufferFM[i*2  ],buffer_len,emulation_rate,volume[i]);
					/* Right channel */
					osd_play_streamed_sample(channel+i*2+1 ,bufferFM[i*2+1],buffer_len,emulation_rate,volume[i]);
				}
			}
		}
		else
		{	/* Jarek's */
			YM2151Update();
			for (i = 0;i < intf->num;i++)
				if (sample_bits==16)
					osd_play_streamed_sample_16(channel+i,bufferFM[i],2*buffer_len,emulation_rate,intf->volume[i]);
				else
					osd_play_streamed_sample(channel+i,bufferFM[i],buffer_len,emulation_rate,intf->volume[i]);
		}
		/* reset sample position */
		for (i = 0;i < intf->num;i++)
		{
			sample_posFM[i] = 0;
		}
	}else if( FMMode == CHIP_YM2151_OPL ){	/* OPL chip emurator */
		osd_ym2203_update();
	}
}

