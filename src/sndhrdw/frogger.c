#include "driver.h"
#include "Z80.h"
#include "psg.h"


#define AY8910_CLOCK 1789750000	/* 1.78975 MHZ ?? */

#define UPDATES_PER_SECOND 60
#define emulation_rate (400*UPDATES_PER_SECOND)
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)


static int command_queue[10],pending_commands;



int frogger_sh_init(const char *gamename)
{
	int j;
	unsigned char *RAM;


	/* ROM 1 has data lines D0 and D1 swapped. Decode it. */
	RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	for (j = 0;j < 0x0800;j++)
		RAM[j] = (RAM[j] & 0xfc) | ((RAM[j] & 1) << 1) | ((RAM[j] & 2) >> 1);

	return 0;
}



void frogger_soundcommand_w(int offset,int data)
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

#define TIMER_RATE (128)

			clockticks = (Z80_IPeriod - Z80_ICount);

			clock = clockticks / TIMER_RATE;

			clock = ((clock & 0x01) << 4) | ((clock & 0x02) << 6) |
					((clock & 0x10) << 2) |	((clock & 0x08) << 0);

			chip->Regs[port] = clock;
		}
	}

	return 0;
}



int frogger_sh_interrupt(void)
{
	if (pending_commands > 0)
	{
	/* actually I should trigger the interrupt when bit 3 of 8201 is set to 1 */
		return 0xff;
	}
	else return Z80_IGNORE_INT;
}



int frogger_sh_start(void)
{
	pending_commands = 0;

	if (AYInit(1,AY8910_CLOCK,emulation_rate,buffer_len,0) == 0)
	{
		AYSetPortHandler(0,AY_PORTA,porthandler);
		AYSetPortHandler(0,AY_PORTB,porthandler);

		return 0;
	}
	else return 1;
}



void frogger_sh_stop(void)
{
	AYShutdown();
}



int static lastreg;	/* AY-3-8910 register currently selected */

int frogger_sh_read_port_r(int offset)
{
	return AYReadReg(0,lastreg);
}



void frogger_sh_control_port_w(int offset,int data)
{
	lastreg = data;
}



void frogger_sh_write_port_w(int offset,int data)
{
	AYWriteReg(0,lastreg,data);
}



void frogger_sh_update(void)
{
	unsigned char buffer[buffer_len];
	static int playing;


	if (play_sound == 0) return;

	AYSetBuffer(0,buffer);
	AYUpdate();
	osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
if (!playing)
{
	osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
	osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
	playing = 1;
}
}
