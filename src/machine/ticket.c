/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Right now, this is an *extremely* basic ticket dispenser.
  TODO:  Add sound, graphical output?
***************************************************************************/

#include "driver.h"


static int dispenser_status;
static int dispenser_power;
static int dispenser_time_msec;

/* Not currently used for anything, but seems like a good thing to keep */
static long dispensed_tickets;

/* Callback routine used during ticket dispensing */
static void ticket_dispenser_toggle(int foo);


/***************************************************************************
  ticket_dispenser_init

  TODO:  Take in ACTIVE_LOW / ACTIVE_HIGH as a parameter?
***************************************************************************/
void ticket_dispenser_init(int msec)
{
	dispenser_time_msec = msec;
	dispensed_tickets = 0;
	dispenser_status = 0x00;
	dispenser_power = 0x00;
}

/***************************************************************************
  ticket_dispenser_r

  How I think this works:
  A status of 0x80 means we're still dispensing the ticket.
  A status of 0x00 means we're in a wait cycle after dispensing the ticket.
  A status of 0x00 twice in a row means the power's off.
***************************************************************************/
int ticket_dispenser_r(int offset)
{
	return dispenser_status;
}

/***************************************************************************
  ticket_dispenser_w

  How I think this works:
  A write of 0x00 means we should turn on the power and start dispensing.
  A write of 0x80 means we should turn off the power and stop.
***************************************************************************/
void ticket_dispenser_w(int offset, int data)
{
	/* On an activate signal, start dispensing! */
	if (data==0x00)
	{
		timer_set (TIME_IN_MSEC(dispenser_time_msec), 0, ticket_dispenser_toggle);
	}

	dispenser_power = data ^ 0x80;

}


/***************************************************************************
  ticket_dispenser_toggle

  How I think this works:
  When a ticket dispenses, there is N milliseconds of status = high,
  and N milliseconds of status = low (a wait cycle?).
***************************************************************************/
static void ticket_dispenser_toggle(int foo)
{

	/* If we still have power, keep toggling ticket states. */
	if (dispenser_power)
	{
		dispenser_status ^= 0x80;
		timer_set (TIME_IN_MSEC(dispenser_time_msec), 0, ticket_dispenser_toggle);
	}

	if (dispenser_status == 0x80)
	{
		/* Dispense a ticket on every low to high change */
		osd_led_w(2,1);
		dispensed_tickets++;
	}
	else
	{
		osd_led_w(2,0);
	}
}



