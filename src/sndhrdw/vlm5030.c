/*
	vlm5030.c

	VLM5030 emurator
*/
#include "driver.h"
#include "vlm5030.h"
#include "sndhrdw/generic.h"

#define MIN_SLICE 10

#define TRY_5220	/* try to use tms5220 , but it was not match */

#ifdef TRY_5220
#include "5220intf.h"
static int VLM5030_end; /* hack end point */
#else /* TRY_5220 */

#endif /* TRY_5220 */
static int sample_pos;
static int buffer_len;
static int emulation_rate;
static struct VLM5030interface *intf;

static unsigned char *buffer;
static int channel;

static unsigned char *VLM5030_rom;
static int VLM5030_address;
static int VLM5030_BUSY;

/* decode and buffering data */
static void vlm5030_process(unsigned char *buffer, unsigned int size)
{
#ifdef TRY_5220
	int data;

	/* write data to tms5220 */
	while( VLM5030_BUSY  && tms5220_ready_r() )
	{
		/* write data */
		data = VLM5030_rom[VLM5030_address++];
		tms5220_data_w (0, data);
		/* !!!!! Warning : End point is found opecode 0x03 !!!!! */
//		if( data == 0x03 )
		if( VLM5030_address >= VLM5030_end )
		{
			 VLM5030_BUSY = 0;
		}
	}
#else
	int buf_count = 0;

	while(size>0)
	{
		buffer[buf_count] = 0x00;
		buf_count++;
		size--;
	}
#endif
}

/* realtime update */
void VLM5030_update(void)
{
	int totcycles,leftcycles,newpos;

	if( VLM5030_rom )
	{
		/* docode mode */
		totcycles = cpu_getfperiod();
		leftcycles = cpu_getfcount();
		newpos = buffer_len * (totcycles-leftcycles) / totcycles;

		if (newpos > buffer_len)
			newpos = buffer_len;
		if (newpos - sample_pos < MIN_SLICE)
			return;
		vlm5030_process (buffer + sample_pos, newpos - sample_pos);

		sample_pos = newpos;
	}
	else
	{
		/* sampling mode (set busy flag) */
		VLM5030_BUSY = osd_get_sample_status(channel) ? 0 : 1;
	}
}

int VLM5030_busy(void)
{
	VLM5030_update();
	return VLM5030_BUSY;
}

void VLM5030_w(int offset , int data )
{
	if( VLM5030_rom )
	{
		/* docode mode */
		VLM5030_address = (((int)VLM5030_rom[data&0xfffe])<<8)|VLM5030_rom[data|1];
#ifdef TRY_5220
		/* !!!!! Get the end point !!!!! */
		VLM5030_end     = (((int)VLM5030_rom[(data&0xfffe)+2])<<8)|VLM5030_rom[(data|1)+3];
#endif
		tms5220_data_w (0, 0x60 );
		VLM5030_BUSY = 1;
		VLM5030_update();
	}
	else if (Machine->samples)
	{
		/* sampling mode */
		struct GameSample *sample = Machine->samples->sample[data>>1];
		if( sample ) osd_play_sample( channel, sample->data, sample->length, sample->smpfreq, sample->volume, 0 );
	}
}

#ifdef TRY_5220
static struct TMS5220interface tms5220_interface =
{
    640000,     /* clock speed (80*samplerate) */
    255,        /* volume */
    0           /* IRQ handler */
};
#endif

/* start VLM5030 with sound rom              */
/* speech_rom == 0 -> use sampling data mode */
int VLM5030_sh_start( struct VLM5030interface *interface , unsigned char *speech_rom )
{
    intf = interface;

//	buffer_len = intf->clock / 80 / Machine->drv->frames_per_second;
	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;
	sample_pos = 0;

	VLM5030_rom = speech_rom;
	VLM5030_BUSY = 0;

	if( VLM5030_rom )
	{
		/* decode mode */
#ifdef TRY_5220
		if( tms5220_sh_start (&tms5220_interface) != 0 ) return 1;
#else
		if ((buffer = malloc(buffer_len)) == 0)
		{
			return 1;
		}
		memset(buffer,0x80,buffer_len);
#endif
	}

	channel = get_play_channels(1);
	return 0;
}

/* update VLM5030 */
void VLM5030_sh_update( void )
{
	if (Machine->sample_rate == 0) return;

	if( VLM5030_rom )
	{
#ifdef TRY_5220
		tms5220_sh_update();
		sample_pos = 0;
#else
		if (sample_pos < buffer_len)
			vlm5030_process (buffer + sample_pos, buffer_len - sample_pos);
		sample_pos = 0;
		osd_play_streamed_sample(channel,buffer,buffer_len,emulation_rate,intf->volume);
#endif
	}
}

/* update VLM5030 */
void VLM5030_sh_stop( void )
{
#ifdef TRY_5220
	tms5220_sh_stop();
#else
	if (buffer != 0 )
	{
    	free( buffer );
    	buffer = 0;
	}
#endif
}
