/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Right now, this is an *extremely* basic ticket dispenser.
  TODO:  Add sound, graphical output?
***************************************************************************/

#include "driver.h"
#include "machine/ticket.h"

#define DEBUG_TICKET

unsigned int dispensed_tickets = 0;

static int status;
static int power;
static int time_msec;
static int active;
static void *timer;

static int active_bit = 0x80;

/* Callback routine used during ticket dispensing */
static void ticket_dispenser_toggle(int parm);


/***************************************************************************
  ticket_dispenser_init

***************************************************************************/
void ticket_dispenser_init(int msec, int activehigh)
{
	time_msec = msec;
	active    = activehigh ? active_bit : 0;
	status    = active;// ^ active_bit;  /* inactive */
	power     = 0x00;
}

/***************************************************************************
  ticket_dispenser_r

  How I think this works:
  A status of 0x80 (or 0x00) means we're still dispensing the ticket.
  A status of 0x00 (or 0x80) means we're in a wait cycle after dispensing the ticket.
  A status of 0x00 (or 0x80) twice in a row means the power's off.
***************************************************************************/
int ticket_dispenser_r(int offset)
{
#ifdef DEBUG_TICKET
	fprintf(errorlog, "PC: %04X  Ticket Status Read = %02X\n", cpu_getpc(), status);
#endif
	return status;
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
	if ((data & active_bit) == active)
	{
		if (!power)
		{
#ifdef DEBUG_TICKET
			fprintf(errorlog, "PC: %04X  Ticket Power On\n", cpu_getpc());
#endif
			timer = timer_set (TIME_IN_MSEC(time_msec), 0, ticket_dispenser_toggle);
			power = 1;

			status = active;// ^ active_bit;  /* inactive */
		}
	}
	else
	{
		if (power)
		{
#ifdef DEBUG_TICKET
			fprintf(errorlog, "PC: %04X  Ticket Power Off\n", cpu_getpc());
#endif
			timer_remove(timer);
			osd_led_w(2,0);
			power = 0;
		}
	}
}


/***************************************************************************
  ticket_dispenser_toggle

  How I think this works:
  When a ticket dispenses, there is N milliseconds of status = high,
  and N milliseconds of status = low (a wait cycle?).
***************************************************************************/
static void ticket_dispenser_toggle(int parm)
{

	/* If we still have power, keep toggling ticket states. */
	if (power)
	{
		status ^= active_bit;
#ifdef DEBUG_TICKET
		fprintf(errorlog, "Ticket Status Changed to %02X\n", status);
#endif
		timer = timer_set (TIME_IN_MSEC(time_msec), 0, ticket_dispenser_toggle);
	}

	if (status == active)
	{
		/* Dispense a ticket on every time the status goes active */
		osd_led_w(2,1);
		dispensed_tickets++;

#ifdef DEBUG_TICKET
		fprintf(errorlog, "Ticket Dispensed\n");
#endif
	}
	else
	{
		osd_led_w(2,0);
	}
}



