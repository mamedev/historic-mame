#include "driver.h"
#include "Z80.h"
#include "psg.h"


#define AY8910_CLOCK 1789750000	/* 1.78975 MHZ ?? */

#define UPDATES_PER_SECOND 60
#define emulation_rate (400*UPDATES_PER_SECOND)
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)


static int command_queue[10],pending_commands;



void pooyan_soundcommand_w(int offset,int data)
{
	command_queue[pending_commands] = data;
	pending_commands++;
}



static unsigned char porthandler(AY8910 *chip, int port, int iswrite, unsigned char val)
{
	if (iswrite)
	{
	}
	else
	{
		if (port == 0x0e)
		{
			int i;


			if (pending_commands > 0)	/* should always be true */
			{
				chip->Regs[port] = command_queue[0];

				pending_commands--;

				for (i = 0;i < pending_commands;i++)
					command_queue[i] = command_queue[i+1];
			}
			else chip->Regs[port] = 0;
		}
		else if (port == 0x0f)
		{
			int clockticks,clock;

#define TIMER_RATE (32)

			clockticks = (Z80_IPeriod - Z80_ICount);

			clock = clockticks / TIMER_RATE;

			chip->Regs[port] = clock;
		}
	}

	return 0;
}



int pooyan_sh_interrupt(void)
{
	if (pending_commands > 0)
	{
	/* actually I should trigger the interrupt when bit 3 of 8201 is set to 1 */
		return 0xff;
	}
	else return Z80_IGNORE_INT;
}



int pooyan_sh_start(void)
{
	pending_commands = 0;

	if (AYInit(2,AY8910_CLOCK,emulation_rate,buffer_len,0) == 0)
	{
		AYSetPortHandler(0,AY_PORTA,porthandler);
		AYSetPortHandler(0,AY_PORTB,porthandler);

		return 0;
	}
	else return 1;
}



void pooyan_sh_stop(void)
{
	AYShutdown();
}



int static lastreg1,lastreg2;	/* AY-3-8910 register currently selected */

int pooyan_sh_read_port1_r(int offset)
{
	return AYReadReg(0,lastreg1);
}



void pooyan_sh_control_port1_w(int offset,int data)
{
	lastreg1 = data;
}



void pooyan_sh_write_port1_w(int offset,int data)
{
	AYWriteReg(0,lastreg1,data);
}



int pooyan_sh_read_port2_r(int offset)
{
	return AYReadReg(1,lastreg2);
}



void pooyan_sh_control_port2_w(int offset,int data)
{
	lastreg2 = data;
}



void pooyan_sh_write_port2_w(int offset,int data)
{
	AYWriteReg(1,lastreg2,data);
}



void pooyan_sh_update(void)
{
	unsigned char buffer[2][buffer_len];
	static int playing;


	if (play_sound == 0) return;

	AYSetBuffer(0,buffer[0]);
	AYSetBuffer(1,buffer[1]);
	AYUpdate();
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
if (!playing)
{
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(0,buffer[0],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(1,buffer[1],buffer_len,emulation_rate,255);
	playing = 1;
}
}
