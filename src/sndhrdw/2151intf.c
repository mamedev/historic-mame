/***************************************************************************

  2151intf.c

  Support interface YM2151(OPM)

***************************************************************************/

#include "driver.h"
#include "fm.h"
#include "sndhrdw/2151intf.h"
#include "sndhrdw/generic.h"

extern unsigned char No_FM;

#define MIN_SLICE 44

static int emulation_rate;
static int buffer_len;

static struct YM2151interface *intf;

static int FMMode;
#define CHIP_YM2151_DAC 4
#define CHIP_YM2151_OPL 5

static FMSAMPLE *bufferFM[MAX_2151*8];
static int sample_posFM[MAX_2151];

static int volume[MAX_2151];
static int channel;

#ifdef macintosh
//#include "mac8910.c"
#endif

int YM2151_sh_start(struct YM2151interface *interface )
{
	int i,j;
/*	int rate = 22050; */
	int rate = Machine->sample_rate;

	intf = interface;

	buffer_len = rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	if( No_FM ) FMMode = CHIP_YM2151_DAC;
	else        FMMode = CHIP_YM2151_OPL;

	for (i = 0;i < MAX_2151;i++)
	{
		sample_posFM[i] = 0;
		for( j = 0 ; j < 8 ; j++ )
			bufferFM[i*8+j] = 0;
	}

	/* OPM initialize */
	for (i = 0;i < intf->num*8;i++)
	{
		if ((bufferFM[i] = malloc(sizeof(FMSAMPLE)*buffer_len)) == 0)
		{
			while (--i >= 0) free(bufferFM[i]);
			return 1;
		}
		for (j = 0;j < buffer_len ;j++) bufferFM[i][j] = FMOUT_0;
	}
	for (i = 0; i < intf->num; i++)
	volume[i] = intf->volume[i];

	if (OPMInit(intf->num,intf->clock,emulation_rate,buffer_len,bufferFM) == 0)
	{
		channel = get_play_channels(intf->num*8);
		for (i = 0; i < intf->num; i++)
			OPMSetIrqHandler (i, interface->handler[i]);
		return 0;
	}
	/* error */
	for (i = 0;i < intf->num *8;i++) free(bufferFM[i]);
	return 1;
}

void YM2151_sh_stop(void)
{
	int i;

	if( FMMode == CHIP_YM2151_DAC )
	{
		for (i = 0; i < intf->num; i++)
			OPMSetIrqHandler (i, 0);
		OPMShutdown();
		for (i = 0;i < intf->num *8;i++) free(bufferFM[i]);
	}
}

static inline void update_opm(int chip)
{
	int totcycles,leftcycles,newpos;

	leftcycles = cpu_getfcount();
	if (leftcycles < 0) leftcycles = 0;	/* ASG 971105 */
	totcycles = cpu_getfperiod();
	newpos = buffer_len * (totcycles-leftcycles) / totcycles;

	if( newpos - sample_posFM[chip] < MIN_SLICE ) return;
	OPMUpdateOne(chip , newpos );
	sample_posFM[chip] = newpos;
}

static int lastreg0,lastreg1;

int YM2151_status_port_0_r(int offset)
{
	update_opm(0);
	return OPMReadStatus(0);
}
int YM2151_status_port_1_r(int offset)
{
	update_opm(1);
	return OPMReadStatus(1);
}

void YM2151_register_port_0_w(int offset,int data)
{
	lastreg0 = data;
}
void YM2151_register_port_1_w(int offset,int data)
{
	lastreg1 = data;
}

static void opm_to_opn(int n,int r,int v)
{
	/* */
	return;
}

void YM2151_data_port_0_w(int offset,int data)
{
	if( FMMode == CHIP_YM2151_DAC ){
		update_opm(0);
		OPMWriteReg(0,lastreg0,data);
	}else if( FMMode == CHIP_YM2151_OPL ){
		opm_to_opn(0,lastreg0,data);
	}
}

void YM2151_data_port_1_w(int offset,int data)
{
	if( FMMode == CHIP_YM2151_DAC ){
		update_opm(1);
		OPMWriteReg(1,lastreg1,data);
	}else if( FMMode == CHIP_YM2151_OPL ){
		opm_to_opn(1,lastreg1,data);
	}
}

void YM2151_sh_update(void)
{
	int i;

	if (Machine->sample_rate == 0 ) return;

	if( FMMode == CHIP_YM2151_DAC ){	/* DIGITAL sound emurator */
		OPMUpdate();
		for (i = 0;i < intf->num;i++)
		{
			sample_posFM[i] = 0;
		}

		/* ASG 971107 - if only one chip, start at voice 8 so that other sfx can be used */
		for (i = 0;i < intf->num*8;i+=8)
		{
			osd_play_streamed_sample(channel+(i<<3)  ,bufferFM[i  ],buffer_len,emulation_rate,volume[i]);
			osd_play_streamed_sample(channel+(i<<3)+1,bufferFM[i+1],buffer_len,emulation_rate,volume[i]);
			osd_play_streamed_sample(channel+(i<<3)+2,bufferFM[i+2],buffer_len,emulation_rate,volume[i]);
			osd_play_streamed_sample(channel+(i<<3)+3,bufferFM[i+3],buffer_len,emulation_rate,volume[i]);
			osd_play_streamed_sample(channel+(i<<3)+4,bufferFM[i+4],buffer_len,emulation_rate,volume[i]);
			osd_play_streamed_sample(channel+(i<<3)+5,bufferFM[i+5],buffer_len,emulation_rate,volume[i]);
			osd_play_streamed_sample(channel+(i<<3)+6,bufferFM[i+6],buffer_len,emulation_rate,volume[i]);
			osd_play_streamed_sample(channel+(i<<3)+7,bufferFM[i+7],buffer_len,emulation_rate,volume[i]);
		}
	}else if( FMMode == CHIP_YM2151_OPL ){	/* OPL chip emurator */
		osd_ym2203_update();
	}
}