#include "driver.h"
#include "adpcm.h"

const int SmpOffsetTable[] = { 128, 128, 3006, 4800, 6000, 22466, 47538, 51500, 59360, -1 };

extern int signalf;




int kungfum_sh_init(const char *gamename)
{
        int i;
	struct GameSamples *samples;

	i = 0;
	while (SmpOffsetTable[i+1] != -1) i++;

	if ((samples = malloc(sizeof(struct GameSamples) + (i-1)*sizeof(struct GameSample))) == 0)
		return 0;

	samples->total = i;
	for (i = 0; i < samples->total; i++)
		samples->sample[i] = 0;

	for (i = 0; i < samples->total; i++)
	{
                long smplen;

		if ((smplen=(SmpOffsetTable[i+1]-SmpOffsetTable[i])) > 0)
		{
		   if ((samples->sample[i] = malloc(sizeof(struct GameSample) + (smplen)*sizeof(char))) != 0)
		   {
                           long j;

			   samples->sample[i]->length = smplen;
			   samples->sample[i]->volume = 255;
			   samples->sample[i]->smpfreq = 8000;   /* standard ADPCM voice freq */
			   samples->sample[i]->resolution = 8;
                           InitDecoder();
                           for (j=0; j < smplen; j++)
                           {
                             signalf += ((j%2) ? DecodeAdpcm(Machine->memory_region[2][(SmpOffsetTable[i]+j) >> 2] & 0x0f)
                                               : DecodeAdpcm(Machine->memory_region[2][(SmpOffsetTable[i]+j) >> 2] >> 4));
                             if (signalf > 2047) signalf = 2047;
                             if (signalf < -2047) signalf = -2047;
                             samples->sample[i]->data[j] = (signalf / 16);     /* for 16-bit samples multiply by 16 */
                           }
		   }
		}
	}

        Machine->samples = samples;

        return 0;
}

void kungfum_sh_port0_w(int offset, int data)
{
        int command;

	if (Machine->samples == 0) return;

        command = data;
        data    = data & 0x7f;
        if (command == 0x80)
          osd_stop_sample(7);
        else if ((command > 0x80) && (data < Machine->samples->total) && Machine->samples->sample[data])
		osd_play_sample(7,Machine->samples->sample[data]->data,
				Machine->samples->sample[data]->length,
                                Machine->samples->sample[data]->smpfreq,
                                Machine->samples->sample[data]->volume,0);
}

void kungfum_sh_update(void)
{
}
