/***************************************************************************

  Many games use a master-slave CPU setup. Typically, the main CPU writes
  a command to some register, and then writes to another register to trigger
  an interrupt on the slave CPU (the interrupt might also be triggered by
  the first write). The slave CPU, notified by the interrupt, goes and reads
  the command.

  Currently, MAME doesn't interleave the execution of the two CPUs: they run
  alternatively for one complete video frame (~50000 clock cycles for a 3Mhz
  CPU). This could lead to data loss if the main CPU sends a quick sequence
  of commands. To avoid that, we use a buffer.

***************************************************************************/

#include "driver.h"



#define QUEUE_LENGTH 10	/* hopefully enough! */

static int command_queue[QUEUE_LENGTH];
int pending_commands;



/***************************************************************************

  Add a command to the queue.

***************************************************************************/
void sound_command_w(int offset,int data)
{
	if (pending_commands < QUEUE_LENGTH)
	{
		command_queue[pending_commands] = data;
		pending_commands++;
	}
	else
	{
		if (errorlog) fprintf(errorlog,"error: sound command queue overflow!\n");
	}
}



/***************************************************************************

  This function reads a command from the sound queue. If the queue is empty,
  returns 0.

***************************************************************************/
int sound_command_r(int offset)
{
	int i,res;


	if (pending_commands > 0)
	{
		res = command_queue[0];

		pending_commands--;

		for (i = 0;i < pending_commands;i++)
			command_queue[i] = command_queue[i+1];
	}
	else
	{
		if (errorlog) fprintf(errorlog,"warning: read sound command, but queue empty\n");
		res = 0;
	}

	return res;
}



/***************************************************************************

  Similar to sound_command_r(), but if the queue is empty it returns the
  last command instead of 0. Games which continuously poll the sound command
  port should use this function.

***************************************************************************/
int sound_command_latch_r(int offset)
{
	int i,res;


	res = command_queue[0];

	if (pending_commands > 0)
	{
		pending_commands--;

		for (i = 0;i < pending_commands;i++)
			command_queue[i] = command_queue[i+1];
	}

	return res;
}



/***************************************************************************

  This function returns 0xff if there are commands waiting in the queue,
  0 otherwise.

***************************************************************************/
int sound_pending_commands_r(int offset)
{
	if (pending_commands > 0) return 0xff;
	else return 0;
}



static int latch,read_debug;

void soundlatch_w(int offset,int data)
{
if (errorlog && read_debug == 0)
	fprintf(errorlog,"Warning: sound latch written before being read. Previous: %02x, new: %02x\n",latch,data);
	latch = data;
	read_debug = 0;
}

int soundlatch_r(int offset)
{
	read_debug = 1;
	return latch;
}

/***************************************************************************

  This function returns top of reserved sound channels

***************************************************************************/
static int reserved_channel = 0;

int get_play_channels( int request )
{
	int ret_value = reserved_channel;

	reserved_channel += request;
	return ret_value;
}

void reset_play_channels(void)
{
	reserved_channel = 0;
}
