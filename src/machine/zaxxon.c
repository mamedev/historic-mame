/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


#define IN2_COIN (1<<7)


int zaxxon_IN2_r(int offset)
{
	int res;
	static int coin;


	res = readinputport(2);

	if (res & IN2_COIN)
	{
		if (coin) res &= ~IN2_COIN;
		else coin = 1;
	}
	else coin = 0;

	return res;
}
