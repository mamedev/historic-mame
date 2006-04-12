/***************************************************************************
Amiga Computer / Arcadia Game System

Driver by:

Ernesto Corvi & Mariusz Wojcieszek

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "includes/amiga.h"
#include "cpu/m68000/m68000.h"


#define CLOCK_DIVIDER	16


#define LOG(x)
//#define LOG(x)    logerror x


typedef struct _audio_channel audio_channel;
struct _audio_channel
{
	mame_timer *	end_timer;
	UINT32			curlocation;
	UINT16			curlength;
	UINT16			curticks;
	UINT8			index;
	UINT8			dmaenabled;
	UINT8			state;
	UINT8			modper;
	UINT8			modvol;
	UINT16			data;
};


typedef struct _amiga_audio amiga_audio;
struct _amiga_audio
{
	audio_channel	channel[4];
	UINT16			ADKCON;
	UINT16			DMACON;
	sound_stream *	stream;
	double			sample_period;
};


static amiga_audio *audio_state;


static void signal_irq(int which)
{
	amiga_custom_w(REG_INTREQ, 0x8000 | (1 << which), 0);
}


static void dma_reload(audio_channel *chan)
{
	chan->curlocation = CUSTOM_REG_LONG(REG_AUD0LCH + chan->index * 8);
	chan->curlength = CUSTOM_REG(REG_AUD0LEN + chan->index * 8);
	timer_set(TIME_IN_HZ(15750), chan->index + 7, signal_irq);
	LOG(("dma_reload(%d): offs=%05X len=%04X\n", chan->index, chan->curlocation, chan->curlength));
}


static void amiga_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	amiga_audio *audio = param;
	int channum, sampoffs = 0;

	length *= CLOCK_DIVIDER;

	/* loop until done */
	while (length > 0)
	{
		int nextper, nextvol;
		int ticks = length;

		/* determine the number of ticks we can do in this chunk */
		if (ticks > audio->channel[0].curticks)
			ticks = audio->channel[0].curticks;
		if (ticks > audio->channel[1].curticks)
			ticks = audio->channel[1].curticks;
		if (ticks > audio->channel[2].curticks)
			ticks = audio->channel[2].curticks;
		if (ticks > audio->channel[3].curticks)
			ticks = audio->channel[3].curticks;

		/* loop over channels */
		nextper = nextvol = -1;
		for (channum = 0; channum < 4; channum++)
		{
			int volume = (nextvol == -1) ? CUSTOM_REG(REG_AUD0VOL + channum * 8) : nextvol;
			int period = (nextper == -1) ? CUSTOM_REG(REG_AUD0PER + channum * 8) : nextper;
			audio_channel *chan = &audio->channel[channum];
			stream_sample_t sample;
			int i;

			/* normalize the volume value */
			volume = (volume & 0x40) ? 64 : (volume & 0x3f);
			volume *= 4;

			/* are we modulating the period of the next channel? */
			if (chan->modper)
			{
				nextper = chan->data;
				nextvol = -1;
				sample = 0;
			}

			/* are we modulating the volume of the next channel? */
			else if (chan->modvol)
			{
				nextper = -1;
				nextvol = chan->data;
				sample = 0;
			}

			/* otherwise, we are generating data */
			else
			{
				nextper = nextvol = -1;
				if (!(chan->curlocation & 1))
					sample = (INT8)(chan->data >> 8) * volume;
				else
					sample = (INT8)(chan->data >> 0) * volume;
			}

			/* fill the buffer with the sample */
			for (i = 0; i < ticks; i += CLOCK_DIVIDER)
				outputs[channum][(sampoffs + i) / CLOCK_DIVIDER] = sample;

			/* account for the ticks; if we hit 0, advance */
			chan->curticks -= ticks;
			if (chan->curticks == 0)
			{
				chan->curticks = period;
				if (chan->curticks < 124)
					chan->curticks = 124;

				/* move forward one byte; if we move to an even byte, fetch new */
				if (chan->dmaenabled)
				{
					chan->curlocation++;
					if (!(chan->curlocation & 1))
					{
						chan->data = amiga_chip_ram[chan->curlocation/2];
						if (chan->curlength != 0)
							chan->curlength--;

						/* if we run out of data, reload the dma */
						if (chan->curlength == 0)
							dma_reload(chan);
					}
				}
			}
		}

		/* bump ourselves forward by the number of ticks */
		sampoffs += ticks;
		length -= ticks;
	}
}


void *amiga_sh_start(int clock, const struct CustomSound_interface *config)
{
	int i;

	/* allocate a new audio state */
	audio_state = auto_malloc(sizeof(*audio_state));
	memset(audio_state, 0, sizeof(*audio_state));
	for (i = 0; i < 4; i++)
		audio_state->channel[i].index = i;

	/* compute the sample period */
	audio_state->sample_period = TIME_IN_HZ(clock);

	/* create the stream */
	audio_state->stream = stream_create(0, 4, clock / CLOCK_DIVIDER, audio_state, amiga_stream_update);
	return audio_state;
}


void amiga_audio_update(void)
{
	stream_update(audio_state->stream, 0);
}


WRITE16_HANDLER( amiga_audio_w )
{
	int channel = (offset - REG_AUD0LCH) / 0x0008;
	audio_channel *chan = &audio_state->channel[channel];

	stream_update(audio_state->stream, 0);

	switch (offset)
	{
		case REG_DMACON:
			LOG(("DMACON = %04X\n", data));

			if (data & 0x8000)
				audio_state->DMACON |= data & 0x7fff;
			else
				audio_state->DMACON &= ~(data & 0x7fff);

			/* if all DMA off, disable all channels */
			if (!(audio_state->DMACON & 0x0200))
			{
				audio_state->channel[0].dmaenabled =
				audio_state->channel[1].dmaenabled =
				audio_state->channel[2].dmaenabled =
				audio_state->channel[3].dmaenabled = FALSE;
			}

			/* otherwise, set based on the channel */
			else
			{
				if (!audio_state->channel[0].dmaenabled && ((audio_state->DMACON >> 0) & 1))
					dma_reload(&audio_state->channel[0]);
				audio_state->channel[0].dmaenabled = (audio_state->DMACON >> 0) & 1;

				if (!audio_state->channel[1].dmaenabled && ((audio_state->DMACON >> 1) & 1))
					dma_reload(&audio_state->channel[1]);
				audio_state->channel[1].dmaenabled = (audio_state->DMACON >> 1) & 1;

				if (!audio_state->channel[2].dmaenabled && ((audio_state->DMACON >> 2) & 1))
					dma_reload(&audio_state->channel[2]);
				audio_state->channel[2].dmaenabled = (audio_state->DMACON >> 2) & 1;

				if (!audio_state->channel[3].dmaenabled && ((audio_state->DMACON >> 3) & 1))
					dma_reload(&audio_state->channel[3]);
				audio_state->channel[3].dmaenabled = (audio_state->DMACON >> 3) & 1;
			}
			break;
/*
NAME   rev ADDR type chip Description
--------------------------------------------------------------------------------
DMACON  096  W  A D P DMA control write (clear or set)
DMACONR 002  R  A   P DMA control (and blitter status) read

    This register controls all of the DMA channels, and contains
    blitter DMA status bits.

         +------+----------+--------------------------------------------+
         | BIT# | FUNCTION | DESCRIPTION                                |
         +------+----------+--------------------------------------------+
         | 15   | SET/CLR  | Set/Clear control bit. Determines if bits  |
         |      |          | written wit a 1 get set or cleared.        |
         |      |          | Bits written witn a zero are unchanged.    |
         | 09   | DMAEN    | Enable all DMA below (also UHRES DMA)      |
         | 03   | AUD3EN   | Audio chanel 3 DMA enable                  |
         | 02   | AUD2EN   | Audio chanel 2 DMA enable                  |
         | 01   | AUD1EN   | Audio chanel 1 DMA enable                  |
         | 00   | AUD0EN   | Audio chanel 0 DMA enable                  |
         +------+----------+--------------------------------------------+
*/

		case REG_ADKCON:
			LOG(("ADKCON = %04X\n", data));

			if (data & 0x8000)
				audio_state->ADKCON |= data & 0x7fff;
			else
				audio_state->ADKCON &= ~(data & 0x7fff);

			audio_state->channel[3].modper = (audio_state->ADKCON >> 7) & 1;
			audio_state->channel[2].modper = (audio_state->ADKCON >> 6) & 1;
			audio_state->channel[1].modper = (audio_state->ADKCON >> 5) & 1;
			audio_state->channel[0].modper = (audio_state->ADKCON >> 4) & 1;
			audio_state->channel[3].modvol = (audio_state->ADKCON >> 3) & 1;
			audio_state->channel[2].modvol = (audio_state->ADKCON >> 2) & 1;
			audio_state->channel[1].modvol = (audio_state->ADKCON >> 1) & 1;
			audio_state->channel[0].modvol = (audio_state->ADKCON >> 0) & 1;
/*
07 USE3PN Use audio channel 3 to modulate nothing
06 USE2P3 Use audio channel 2 to modulate period of channel 3.
05 USE1P2 Use audio channel 1 to modulate period of channel 2
04 USE0P1 Use audio channel 0 to modulate period of channel 1.
03 USE3VN Use audio channel 3 to modulate nothing
02 USE2V3 Use audio channel 2 to modulate volume of channel 3.
01 USE1V2 Use audio channel 1 to modulate volume of channel 2.
00 USE0V1 Use audio channel 0 to modulate volume of channel 1.
*/
			break;

		case REG_AUD0DAT:
		case REG_AUD1DAT:
		case REG_AUD2DAT:
		case REG_AUD3DAT:
			LOG(("AUD%dDAT = %04X\n", chan->index, data));
			chan->data = data;
			break;
	}
}
