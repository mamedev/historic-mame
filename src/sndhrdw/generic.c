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
		if (errorlog) fprintf(errorlog,"warning: read command, but queue empty\n");
		res = 0;
	}

	return res;
}



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
