#include "driver.h"
#include "M6502/M6502.h"




void gottlieb_sh_w(int offset,int data)
{
	if (data != 0xff)
	{
		if (Machine->gamedrv->samplenames)
		{
			/* if we have loaded samples, we must be Q*Bert */
			switch(data ^ 0xff)
			{
				case 0x12:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 0, 0);
					break;

				case 0x14:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 1, 0);
					break;

				case 0x16:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 2, 0);
					break;

				case 0x11:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 3, 0);
					break;
			}
		}

		soundlatch_w(offset,data);
		cpu_cause_interrupt(1,INT_IRQ);
		/* only the second sound board revision has the third CPU */
		cpu_cause_interrupt(2,INT_IRQ);
	}
}



/* callback for the timer */
void gottlieb_nmi_generate(int param)
{
	cpu_cause_interrupt(1,INT_NMI);
}

static const char *PhonemeTable[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A1" ,"A2" ,"ZH",
 "AH2","I3" ,"I2" ,"I1" ,"M"  ,"N"  ,"B"  ,"V",
 "CH" ,"SH" ,"Z"  ,"AW1","NG" ,"AH1","OO1","OO",
 "L"  ,"K"  ,"J"  ,"H"  ,"G"  ,"F"  ,"D"  ,"S",
 "A"  ,"AY" ,"Y1" ,"UH3","AH" ,"P"  ,"O"  ,"I",
 "U"  ,"Y"  ,"T"  ,"R"  ,"E"  ,"W"  ,"AE" ,"AE1",
 "AW2","UH2","UH1","UH" ,"O2" ,"O1" ,"IU" ,"U1",
 "THV","TH" ,"ER" ,"EH" ,"E1" ,"AW" ,"PA1","STOP",
 0
};

void gottlieb_speech_w(int offset, int data)
{
	data ^= 255;

if (errorlog) fprintf(errorlog,"Votrax: intonation %d, phoneme %02x %s\n",data >> 6,data & 0x3f,PhonemeTable[data & 0x3f]);

	/* generate a NMI after a while to make the CPU continue to send data */
	timer_set(TIME_IN_USEC(50),0,gottlieb_nmi_generate);
}

void gottlieb_speech_clock_DAC_w(int offset, int data)
{}



    /* partial decoding takes place to minimize chip count in a 6502+6532
       system, so both page 0 (direct page) and 1 (stack) access the same
       128-bytes ram,
       either with the first 128 bytes of the page or the last 128 bytes */

int riot_ram_r(int offset)
{
	extern unsigned char *RAM;


    return RAM[offset&0x7f];
}

void riot_ram_w(int offset, int data)
{
	extern unsigned char *RAM;


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
		return soundlatch_r(offset) ^ 0xff;	/* invert command */
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




static int psg_latch,nmi_enable;
static void *nmi_timer;


int gottlieb_sh_init (const char *gamename)
{
	nmi_timer = NULL;
	return 0;
}

int stooges_sound_input_r(int offset)
{
	/* bits 0-3 are probably unused (future expansion) */

	/* bits 4 & 5 are two dip switches. Unused? */

	/* bit 6 is the test switch. When 0, the CPU plays a pulsing tone. */

	/* bit 7 is a ready signal from the speech chip */

	return 0xc0;
}

void stooges_8910_latch_w(int offset,int data)
{
	psg_latch = data;
}

void stooges_sound_control_w(int offset,int data)
{
	static int last;


	/* bit 0 = NMI enable */
	nmi_enable = data & 1;

	/* bit 1 lights a led on the sound board */

	/* bit 2 goes to 8913 BDIR pin  */
	if ((last & 0x04) == 0x04 && (data & 0x04) == 0x00)
	{
		/* bit 3 selects which of the two 8913 to enable */
		if (data & 0x08)
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_0_w(0,psg_latch);
			else
				AY8910_write_port_0_w(0,psg_latch);
		}
		else
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_1_w(0,psg_latch);
			else
				AY8910_write_port_1_w(0,psg_latch);
		}
	}

	/* bit 5 goes to pin 7 of the speech chip */

	/* bit 6 = speech chip latch clock; high then low to make the chip read it */
	if ((last & 0x40) == 0x40 && (data & 0x40) == 0x00)
	{
	}

	/* bit 7 resets the speech chip */

	last = data & 0x44;
}

/* callback for the timer */
void stooges_nmi_generate(int param)
{
	if (nmi_enable) cpu_cause_interrupt(2,INT_NMI);
}

void stooges_sound_timer_w(int offset,int data)
{
	double freq;


	/* base clock is 250kHz divided by 256 */
	freq = TIME_IN_HZ(250000.0/256/(256-data));

	if (nmi_timer)
		timer_reset(nmi_timer,freq);
	else
		nmi_timer = timer_pulse(freq,0,stooges_nmi_generate);
}
