#include "driver.h"
#include "M6502/M6502.h"




/* note: pinball "thwok" sound is command 0x0d */
void gottlieb_sh_w(int offset,int data)
{
	int command = 255-data;


	if (command)
	{
		soundlatch_w(offset,command);
		cpu_cause_interrupt(1,INT_IRQ);
	}
}

void qbert_sh_w(int offset,int data)
{
	int command = 255-data;


	switch(command)
	{ /* now with all 4 samples ;) */
		case 0x12:      /* JB 980110 */
			/* play a sample here (until Votrax speech is emulated) */
			sample_start (0, 0, 0);
			break;

		case 0x16:
			/* play a sample here (until Votrax speech is emulated) */
			sample_start (0, 2, 0);
			break;

		case 0x14:
			/* play a sample here (until Votrax speech is emulated) */
			sample_start (0, 1, 0);
			break;

		case 0x11:
			/* play a sample here (until Votrax speech is emulated) */
			sample_start (0, 3, 0);
			break;
	}

	gottlieb_sh_w(offset,data);
}

void gottlieb_sh2_w(int offset,int command)
{
    if (command)
	{
		soundlatch_w(offset,command);
		cpu_cause_interrupt(1,INT_IRQ);
	}
}


void gottlieb_amplitude_DAC_w(int offset,int data)
{
	DAC_data_w (offset, data);
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
		return soundlatch_r(offset);
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
