/*
	vlm5030.c

	VLM5030 emurator
*/
#include "driver.h"
#include "vlm5030.h"

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

static unsigned char *output_buffer;
static int channel;

static unsigned char *VLM5030_rom;
static int VLM5030_address;
static int pin_BSY;
static int pin_ST;
static int pin_RST;
static int latch_data = 0;

static int table_h;

/* decode and buffering data */
static void vlm5030_process(unsigned char *buffer, unsigned int size)
{
#ifdef TRY_5220
	int data;

	/* write data to tms5220 */
	while( pin_BSY  && tms5220_ready_r() )
	{
		/* write data */
		data = VLM5030_rom[VLM5030_address++];
		tms5220_data_w (0, data);
		/* !!!!! Warning : End point is found opecode 0x03 !!!!! */
//		if( data == 0x03 )
		if( VLM5030_address >= VLM5030_end )
		{
			 pin_BSY = 0;
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
	int newpos;

	if (Machine->sample_rate == 0) return;

	if( VLM5030_rom )
	{
		/* docode mode */
		newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

		if (newpos - sample_pos < MIN_SLICE)
			return;
		vlm5030_process (output_buffer + sample_pos, newpos - sample_pos);

		sample_pos = newpos;
	}
	else
	{
		/* sampling mode (check  busy flag) */
		if( pin_ST == 0 && pin_BSY == 1 )
		{
			if( osd_get_sample_status(channel) )
				pin_BSY = 0;
		}
	}
}

/* get BSY pin level */
int VLM5030_BSY(void)
{
	VLM5030_update();
	return pin_BSY;
}

/* latch contoll data */
void VLM5030_data_w(int offset,int data)
{
	latch_data = data;
}

/* set RST pin level : reset / set table address A8-A15 */
void VLM5030_RST (int pin )
{
	if( pin_RST )
	{
		if( !pin )
		{	/* H -> L : latch high address table */
			pin_RST = 0;
/*			table_h = latch_data * 256; */
			table_h = 0;
		}
	}
	else
	{
		if( pin )
		{	/* L -> H : reset chip */
			pin_RST = 1;
			if( pin_BSY )
			{
				osd_stop_sample( channel );
				pin_BSY = 0;
			}
		}
	}
}

/* set VCU pin level : ?? unknown */
void VLM5030_VCU(int pin)
{
	/* unknown */
	intf->vcu = pin;
	return;
}

/* set ST pin level  : set table address A0-A7 / start speech */
void VLM5030_ST(int pin )
{
	int table = table_h | latch_data;

	if( pin_ST )
	{
		if( !pin )
		{	/* H -> L */
			pin_ST = 0;
			/* start speech */

			if (Machine->sample_rate == 0)
			{
				pin_BSY = 0;
				return;
			}
			if( VLM5030_rom )
			{
				/* docode mode */
				VLM5030_address = (((int)VLM5030_rom[table&0xfffe])<<8)|VLM5030_rom[table|1];
#ifdef TRY_5220
				/* !!!!! Get the end point !!!!! */
				VLM5030_end     = (((int)VLM5030_rom[(table&0xfffe)+2])<<8)|VLM5030_rom[(table|1)+3];
				tms5220_data_w (0, 0x60 );
#endif
				VLM5030_update();
			}
			else if (Machine->samples)
			{
				/* sampling mode */
				int num = table>>1;

				if (Machine->samples == 0) return;
				if (Machine->samples->total <= num ) return;
				if (Machine->samples->sample[num] == 0) return;

				osd_play_sample(channel,
					Machine->samples->sample[num]->data,
					Machine->samples->sample[num]->length,
					Machine->samples->sample[num]->smpfreq,
					Machine->samples->sample[num]->volume,
					0);
			}
		}
	}
	else
	{
		if( pin )
		{	/* L -> H */
			pin_ST = 1;
			/* setup speech , BSY on after 30ms? */
			pin_BSY = 1;
		}
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
int VLM5030_sh_start( struct VLM5030interface *interface )
{
    intf = interface;

//	buffer_len = intf->clock / 80 / Machine->drv->frames_per_second;
	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;
	sample_pos = 0;
	pin_BSY = pin_RST = pin_ST  = 0;
/*	VLM5030_setVCU(intf->vcu); */

	if( intf->memory_region == -1 )
	{	/* sampling file mode */
		VLM5030_rom = 0;
	}
	else
	{	/* decode mode */
		VLM5030_rom = Machine->memory_region[intf->memory_region];
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
