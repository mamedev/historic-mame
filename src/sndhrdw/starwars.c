/***************************************************************************
sndhrdw\starwars.c

STARWARS MACHINE FILE

This file created by Frank Palazzolo. (palazzol@tir.com)

Release 2.0 (6 August 1997)

See drivers\starwars.c for notes

***************************************************************************/

#include "driver.h"
#include "pokey.h"
#include "cpuintrf.h"   /* for cpu_getpc() */
#include "starwars.h"

/* These lines were modified from milliped.c */

#define emulation_rate (1500000/(28*4))
#define buffer_len (emulation_rate/Machine->drv->frames_per_second)

#define SNDDEBUG 0
#define SOUND_SELF_TEST 0

/* These three are exported so machine\starwars.c can get to them */

int sw_port_A = 0;       /* PIA port A data register */

                         /* Configured as follows:           */
                         /* d7 (in)  Main Ready Flag         */
                         /* d6 (in)  Sound Ready Flag        */
                         /* d5 (out) Mute Speech             */
                         /* d4 (in)  Not Sound Self Test     */
                         /* d3 (out) Hold Main CPU in Reset? */
                         /* d2 (in)  TMS5220 Not Ready       */
                         /* d1 (out) TMS5220 Not Read        */
                         /* d0 (out) TMS5220 Not Write       */

int sw_mainwrite = 0;    /* Main CPU write register */
int sw_mainread = 0;     /* Main CPU read register */

unsigned char timer = 0; /* # of snd_interrupts until timeout */
int timer_running = 0;   /* is the timer running ? */

int port_A_ddr = 0;      /* PIA Data Direction Register A */
int port_B_ddr = 0;      /* PIA Data Direction Register B */
                         /* for each bit, 0 = input, 1 = output */

int starwars_sh_start(void)
{
        /* Derived from milliped.c */
        Pokey_sound_init(1500000,emulation_rate,4);
	return 0;
}

void starwars_sh_stop(void)
{
}


void starwars_pokey_sound_w(int offset,int data)
{
        Update_pokey_sound(offset%8,data,offset>>3,3);
}

void starwars_pokey_ctl_w(int offset,int data)
{
        Update_pokey_sound((offset%8)|8,data,offset>>3,3);
}

void starwars_sh_update(void)
{
        /* Derived from milliped.c */

        unsigned char buffer[emulation_rate/30];
	static int playing;

	if (play_sound == 0) return;

	Pokey_process(buffer,buffer_len);
	osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
        if (!playing)
        {
	        osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
	        osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
	        playing = 1;
        }
}

/********************************************************/

int starwars_snd_interrupt(void)
{
	/* if the timer is running, decrement it, and check if it */
	/* has expired.  If so - IRQ, otherwise ignore            */
	if (timer_running)
	{
		if (--timer == 0)
		{
			timer_running = 0;
			return interrupt();
		}
	}

	return ignore_interrupt();
}
/********************************************************/

int sin_r(int offset)
{
    sw_port_A &= 0x7f;   /* reset main ready flag */
    return sw_mainwrite;
}
/********************************************************/

int PIA_port_r(int offset)
{
    static int rv;

    switch (offset)
    {
        case 0:
            sw_port_A = sw_port_A | ((SOUND_SELF_TEST ^ 1)<<4);
            rv = sw_port_A;
            break;

        case 1:
            rv = port_A_ddr;
            break;

        case 2:
            rv = 0;  /* speech data read? */
            break;

        case 3:
            rv = port_B_ddr;
            break;
    }
    return rv;
}
/********************************************************/

int PIA_timer_r(int offset)
{
    switch (offset)
    {
        case 1:
            if (timer_running)
                return timer;
            else
                return 0xff;    /* This is a hack because timer should */
                                /* be counting down below zero every PIA */
                                /* clock now */

                                /* Since we can't do this, at least make */
                                /* it less than zero so that in the IRQ  */
                                /* handler, it looks like it's > 0       */

        default:
            return 0;
    }
}
/********************************************************/

void sout_w(int offset, int data)
{
    sw_mainread = data;
    sw_port_A |= 0x40;  /* set sound ready flag */
    return;
}
/********************************************************/

void PIA_port_w(int offset, int data)
{
    switch (offset)
    {
        case 0:
            sw_port_A |= (data & port_A_ddr);
            break;

        case 1:
            port_A_ddr = data;
            break;

        case 2:
            /* TMS5220 Speech Data on port B */
            break;

        case 3:
            port_B_ddr = data;
            break;
     }
    return;

}
/********************************************************/

void PIA_timer_w(int offset, int data)
{
    switch (offset)
    {
         case 3:
             break;

         case 0x1b:

             /* Start the timer counting down */

             /* Should be decrementing every data*1024 PIA clock cycles */
             /* PIA runs at 1.5 Mhz */

             /* instead we are decrementing at half that rate, so we'll */
             /* cut the time in half */

             timer = data>>1;

             timer_running = 1;
             break;

         return;
    }
}

