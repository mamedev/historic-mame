#include "driver.h"
#include "sndhrdw/generic.h"

#define AUDIO_CONV(A) (A+0x80)


#ifdef OLD_ROUTINES

/* macroscopic sound emulation from my understanding of the 6502 rom (FF) */

static int current_fx,interrupted_fx;

void gottlieb_sh_w(int offset,int data)
{
   int fx= 255-data;


   if (Machine->samples == 0) return;

   if (fx && fx<48 && Machine->samples->sample[fx])
	switch (fx) {
		case 44: /* reset */
			break;
		case 45:
		case 46:
		case 47:
			/* read expansion socket */
			break;
		default:
		     osd_play_sample(0,Machine->samples->sample[fx]->data,
					Machine->samples->sample[fx]->length,
					 Machine->samples->sample[fx]->smpfreq,
					  Machine->samples->sample[fx]->volume,0);
		     break;
	}
}

void gottlieb_sh_update(void)
{
	if (interrupted_fx && osd_get_sample_status(1)) {
		current_fx=interrupted_fx;
		interrupted_fx=0;
		osd_restart_sample(0);
	}
	if (current_fx && osd_get_sample_status(0))
		current_fx=0;
}

void qbert_sh_w(int offset,int data)
{
   static int fx_priority[]={
	0,
	3,3,3,3,3,3,3,3,3,3,3,3,5,3,4,3,
	3,3,3,3,3,5,5,3,3,3,3,3,3,3,3,3,
	15,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
   int fx= 255-data;
   int channel;

   if (Machine->samples == 0) return;

   if (fx==44) { /* reset */
	interrupted_fx=0;
	current_fx=0;
	osd_stop_sample(0);
	osd_stop_sample(1);
   }
   if (fx && fx<44 && fx_priority[fx] >= fx_priority[current_fx]) {
	if (current_fx==25 || current_fx==26 || current_fx==27)
		interrupted_fx=current_fx;
	if (fx==25 || fx==26 || fx==27 || fx==35)
		interrupted_fx=0;
	if (interrupted_fx)
		channel=1;
	else
		channel=0;
	osd_stop_sample(0);
	osd_stop_sample(1);
	if (Machine->samples->sample[fx])
		osd_play_sample(channel,Machine->samples->sample[fx]->data,
				   Machine->samples->sample[fx]->length,
				    Machine->samples->sample[fx]->smpfreq,
				      Machine->samples->sample[fx]->volume,0);
	current_fx=fx;
   }
}
#else

#define TARGET_EMULATION_RATE 44100     /* will be adapted to be a multiple of buffer_len */
static int emulation_rate;
static int buffer_len;
static unsigned char *buffer;
static int sample_pos;


static unsigned char amplitude_DAC;


void gottlieb_sh_w(int offset,int data)
{
    int command= 255-data;
    if (command) sound_command_w(offset,command);
}

void gottlieb_sh2_w(int offset,int command)
{
    if (command) sound_command_w(offset,command);
}


void gottlieb_sh_update(void)
{
	while (sample_pos < buffer_len)
		buffer[sample_pos++] = amplitude_DAC;

	osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);

	sample_pos=0;
}

void qbert_sh_w(int offset, int data)
{}
#endif

int gottlieb_sh_start(void)
{
    buffer_len = TARGET_EMULATION_RATE / Machine->drv->frames_per_second;
    emulation_rate = buffer_len * Machine->drv->frames_per_second;

    if ((buffer = malloc(buffer_len)) == 0)
	return 1;
    memset(buffer,0x80,buffer_len);

    sample_pos = 0;

    return 0;
}



void gottlieb_sh_stop(void)
{
    free(buffer);
}



void gottlieb_amplitude_DAC_w(int offset,int data)
{
	int totcycles,leftcycles,newpos;


	totcycles = Machine->drv->cpu[1].cpu_clock / Machine->drv->frames_per_second;
	leftcycles = cpu_getfcount();
	newpos = buffer_len * (totcycles-leftcycles) / totcycles;

	while (sample_pos < newpos-1)
	    buffer[sample_pos++] = amplitude_DAC;

    amplitude_DAC=AUDIO_CONV(data);

    buffer[sample_pos++] = amplitude_DAC;
}


int gottlieb_sh_interrupt(void)
{
    if (pending_commands) return interrupt();
    else return ignore_interrupt();
}

int gottlieb_sound_expansion_socket_r(int offset)
{
    return 0;
}

void gottlieb_speech_w(int offset, int data)
{}

void gottlieb_speech_clock_DAC_w(int offset, int data)
{}

void gottlieb_sound_expansion_socket_w(int offset, int data)
{}

    /* partial decoding takes place to minimize chip count in a 6502+6532
       system, so both page 0 (direct page) and 1 (stack) access the same
       128-bytes ram,
       either with the first 128 bytes of the page or the last 128 bytes */

int riot_ram_r(int offset)
{
    return RAM[offset&0x7f];
}

void riot_ram_w(int offset, int data)
{
	/* pb is that M6502.c does some memory reads directly, so we
	  repeat the writes */
    RAM[offset&0x7F]=data;
    RAM[0x80+(offset&0x7F)]=data;
    RAM[0x100+(offset&0x7F)]=data;
    RAM[0x180+(offset&0x7F)]=data;
}

static unsigned char riot_regs[32];
    /* lazy handling of the 6532's I/O, and no handling of timers at all */

int gottlieb_riot_r(int offset)
{
    switch (offset&0x1f) {
	case 0: /* port A */
		return sound_command_r(offset);
	case 2: /* port B */
		return 0x40;    /* say that PB6 is 1 (test SW1 not pressed) */
	case 5: /* interrupt register */
		return 0x40;    /* say that edge detected on PA7 */
	default:
		return riot_regs[offset&0x1f];
    }
}

void gottlieb_riot_w(int offset, int data)
{
    riot_regs[offset&0x1f]=data;
}
