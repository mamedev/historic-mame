/**********************************************************************


    Motorola 6840 PTM interface and emulation

    This function is a simple emulation of up to 4 MC6840
    Programmable Timer Modules

    Written By El Condor based on previous work by Aaron Giles,
   'Re-Animator' and Mathis Rosenhauer.

    Todo:
         Write handling for 'Single Shot' operation.
         Establish whether ptm6840_set_c? routines can replace
         hard coding of external clock frequencies.


    Operation:
    The interface is arranged as follows:

    Internal Clock frequency,
    Clock 1 frequency, Clock 2 frequency, Clock 3 frequency,
    Clock 1 output, Clock 2 output, Clock 3 output,
    IRQ function

    If the external clock frequencies are not fixed, they should be
    entered as '0', and the ptm6840_set_c?(which, state) functions
    should be used instead if necessary.

**********************************************************************/

#include "driver.h"
#include "timer.h"
#include "6840ptm.h"

#define PTMVERBOSE 1

#if PTMVERBOSE
#define PLOG(x)	logerror x
#else
#define PLOG(x)
#endif

#define PTM_6840_CTRL1   0
#define PTM_6840_CTRL2   1
#define PTM_6840_MSBBUF1 2
#define PTM_6840_LSB1	 3
#define PTM_6840_MSBBUF2 4
#define PTM_6840_LSB2    5
#define PTM_6840_MSBBUF3 6
#define PTM_6840_LSB3    7

typedef struct _ptm6840 ptm6840;
struct _ptm6840
{
	const ptm6840_interface *intf;

	UINT8	 control_reg[3],
				  output[3],// output states
				   input[3],// input  gate states
				   clock[3],// clock  states
			     enabled[3],
			   interrupt[3],
					mode[3],
			     t3_divisor,
			     		IRQ,
				 status_reg,
	  status_read_since_int,
				 lsb_buffer,
				 msb_buffer;
	   double internal_freq,
		   external_freq[3];

	// each PTM has 3 timers
		mame_timer	*timer1,
					*timer2,
					*timer3;
	union
	{
#ifdef LSB_FIRST
		struct { UINT8 l, u; } b;
#else
		struct { UINT8 u, l; } b;
#endif
		UINT16 w;
	} latch[3];
	union
	{
#ifdef LSB_FIRST
		struct { UINT8 l, u; } b;
#else
		struct { UINT8 u, l; } b;
#endif
		UINT16 w;
	} counter[3];

};

// local prototypes ///////////////////////////////////////////////////////

static void ptm6840_t1_timeout(int which);
static void ptm6840_t2_timeout(int which);
static void ptm6840_t3_timeout(int which);

// local vars /////////////////////////////////////////////////////////////

static ptm6840 ptm[PTM_6840_MAX];

#if PTMVERBOSE
static const char *opmode[] =
{
	"000 continous mode",
	"001 freq comparison mode",
	"010 continous mode",
	"011 pulse width comparison mode",
	"100 single shot mode",
	"101 freq comparison mode",
	"110 single shot mode",
	"111 pulse width comparison mode"
};
#endif

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Update Internal Interrupts                                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

INLINE void update_interrupts(int which)
{
	ptm6840 *currptr = ptm + which;
	currptr->status_reg &= ~0x80;

	if ((currptr->status_reg & 0x01) && (currptr->control_reg[0] & 0x40)) currptr->status_reg |= 0x80;
	if ((currptr->status_reg & 0x02) && (currptr->control_reg[1] & 0x40)) currptr->status_reg |= 0x80;
	if ((currptr->status_reg & 0x04) && (currptr->control_reg[2] & 0x40)) currptr->status_reg |= 0x80;

	currptr->IRQ = currptr->status_reg >> 7;

	if ( currptr->intf->irq_func )
	{
		if (currptr->interrupt[0]|currptr->interrupt[1]|currptr->interrupt[2])
		{
			currptr->intf->irq_func(currptr->IRQ);
		}
	}

}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Compute Counter                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static UINT16 compute_counter(int counter, int which)
{
	ptm6840 *currptr = ptm + which;

	double freq;
	int remaining=0;

	/* if there's no timer, return the count */
	if (!currptr->enabled[counter])

	return currptr->counter[counter].w;

	/* determine the clock frequency for this timer */
	if (currptr->control_reg[counter] & 0x02)
		freq = currptr->internal_freq;
	else
		freq = currptr->external_freq[counter];

	/* see how many are left */
	switch (counter)
	{
		case 0:
		remaining = (int)(timer_timeleft(currptr->timer1) * freq);
		case 1:
		remaining = (int)(timer_timeleft(currptr->timer2) * freq);
		case 2:
		remaining = (int)((timer_timeleft(currptr->timer3) * freq)/currptr->t3_divisor);
	}

	/* adjust the count for dual byte mode */
	if (currptr->control_reg[counter] & 0x04)
	{
		int divisor = (currptr->counter[counter].w & 0xff) + 1;
		int msb = remaining / divisor;
		int lsb = remaining % divisor;
		remaining = (msb << 8) | lsb;
	}
	PLOG(("MC6840 #%d: read counter(%d): %d\n", which, counter, remaining));
	return remaining;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Reload Counter                                                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void reload_count(int idx, int which)
{
	double freq;
	int count;
	ptm6840 *currptr = ptm + which;

	/* copy the latched value in */
	currptr->counter[idx].w = currptr->latch[idx].w;

	/* counter 0 is self-updating if clocked externally */
	if (idx == 0 && !(currptr->control_reg[idx] & 0x02))
	{
		switch (idx)
		{
			case 0:
			currptr->enabled[0] = 0;
			timer_enable(currptr->timer1,FALSE);
			case 1:
			currptr->enabled[1] = 0;
			timer_enable(currptr->timer2,FALSE);
			case 2:
			currptr->enabled[2] = 0;
			timer_enable(currptr->timer3,FALSE);
		}
		return;
	}

	/* determine the clock frequency for this timer */
	if (currptr->control_reg[idx] & 0x02)
		freq = currptr->internal_freq;
	else
		freq = currptr->external_freq[idx];

	/* determine the number of clock periods before we expire */
	count = currptr->counter[idx].w;
	if (currptr->control_reg[idx] & 0x04)
		count = ((count >> 8) + 1) * ((count & 0xff) + 1);
	else
		count = count + 1;

	/* set the timer */
	PLOG(("MC6840 #%d: reload_count(%d): freq = %f  count = %d\n", which, idx, freq, count));
	switch (idx)
	{
	case 0:
	timer_adjust(currptr->timer1, freq * (double)count, which, 0);
	currptr->enabled[0] = 1;
	timer_enable(currptr->timer1,TRUE);
	PLOG(("TIMER GO(%d)\n", idx));
	break;
	case 1:
	timer_adjust(currptr->timer2, freq * (double)count, which, 0);
	currptr->enabled[1] = 1;
	timer_enable(currptr->timer2,TRUE);
	PLOG(("TIMER GO(%d)\n", idx));
	break;
	case 2:
	timer_adjust(currptr->timer3, freq * (double)count/currptr->t3_divisor, which, 0);
	currptr->enabled[2] = 1;
	timer_enable(currptr->timer3,TRUE);
	PLOG(("TIMER GO(%d)\n", idx));
	break;
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Unconfigure Timer                                                     //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_unconfig(void)
{
	int i;

	i = 0;
	while ( i < PTM_6840_MAX )
	{
		if ( ptm[i].timer1 );
		timer_adjust(ptm[i].timer1, TIME_NEVER, i, 0);
		ptm[i].enabled[0] = 0;
		timer_enable(ptm[i].timer1,FALSE);
		ptm[i].timer1 = NULL;

		if ( ptm[i].timer2 );
		timer_adjust(ptm[i].timer2, TIME_NEVER, i, 0);
		ptm[i].enabled[1] = 0;
		timer_enable(ptm[i].timer2,FALSE);
		ptm[i].timer2 = NULL;

		if ( ptm[i].timer3 );
		timer_adjust(ptm[i].timer3, TIME_NEVER, i, 0);
		ptm[i].enabled[2] = 0;
		timer_enable(ptm[i].timer3,FALSE);
		ptm[i].timer3 = NULL;

		i++;
  	}
	memset (&ptm, 0, sizeof (ptm));
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Configure Timer                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_config(int which, const ptm6840_interface *intf)
{
	ptm6840 *currptr = ptm + which;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call ptm6840_config at init time!");
	assert_always((which >= 0) && (which < PTM_6840_MAX), "ptm6840_config called on an invalid PTM!");
	assert_always(intf, "ptm6840_config called with an invalid interface!");
	ptm[which].intf = intf;

	ptm[which].internal_freq = TIME_IN_HZ(currptr->intf->internal_clock);

	if ( currptr->intf->external_clock1 )
	{
	ptm[which].external_freq[0] = TIME_IN_HZ(currptr->intf->external_clock1);
	}
	else
	{
	ptm[which].external_freq[0] = 1;
	}
	if ( currptr->intf->external_clock2 )
	{
	ptm[which].external_freq[1] = TIME_IN_HZ(currptr->intf->external_clock2);
	}
	else
	{
	ptm[which].external_freq[1] = 1;
	}
	if ( currptr->intf->external_clock3 )
	{
	ptm[which].external_freq[2] = TIME_IN_HZ(currptr->intf->external_clock3);
	}
	else
	{
	ptm[which].external_freq[2] = 1;
	}

	ptm[which].timer1 = timer_alloc(ptm6840_t1_timeout);
	ptm[which].timer2 = timer_alloc(ptm6840_t2_timeout);
	ptm[which].timer3 = timer_alloc(ptm6840_t3_timeout);

	state_save_register_item("6840ptm", which, currptr->lsb_buffer);
	state_save_register_item("6840ptm", which, currptr->msb_buffer);
	state_save_register_item("6840ptm", which, currptr->status_read_since_int);
	state_save_register_item("6840ptm", which, currptr->status_reg);
	state_save_register_item("6840ptm", which, currptr->t3_divisor);
	state_save_register_item("6840ptm", which, currptr->internal_freq);
	state_save_register_item("6840ptm", which, currptr->IRQ);

	state_save_register_item("6840ptm", which, currptr->control_reg[0]);
	state_save_register_item("6840ptm", which, currptr->output[0]);
	state_save_register_item("6840ptm", which, currptr->input[0]);
	state_save_register_item("6840ptm", which, currptr->clock[0]);
	state_save_register_item("6840ptm", which, currptr->interrupt[0]);
	state_save_register_item("6840ptm", which, currptr->mode[0]);
	state_save_register_item("6840ptm", which, currptr->enabled[0]);
	state_save_register_item("6840ptm", which, currptr->external_freq[0]);
	state_save_register_item("6840ptm", which, currptr->counter[0].w);
	state_save_register_item("6840ptm", which, currptr->latch[0].w);
	state_save_register_item("6840ptm", which, currptr->control_reg[1]);
	state_save_register_item("6840ptm", which, currptr->output[1]);
	state_save_register_item("6840ptm", which, currptr->input[1]);
	state_save_register_item("6840ptm", which, currptr->clock[1]);
	state_save_register_item("6840ptm", which, currptr->interrupt[1]);
	state_save_register_item("6840ptm", which, currptr->mode[1]);
	state_save_register_item("6840ptm", which, currptr->enabled[1]);
	state_save_register_item("6840ptm", which, currptr->external_freq[1]);
	state_save_register_item("6840ptm", which, currptr->counter[1].w);
	state_save_register_item("6840ptm", which, currptr->latch[1].w);
	state_save_register_item("6840ptm", which, currptr->control_reg[2]);
	state_save_register_item("6840ptm", which, currptr->output[2]);
	state_save_register_item("6840ptm", which, currptr->input[2]);
	state_save_register_item("6840ptm", which, currptr->clock[2]);
	state_save_register_item("6840ptm", which, currptr->interrupt[2]);
	state_save_register_item("6840ptm", which, currptr->mode[2]);
	state_save_register_item("6840ptm", which, currptr->enabled[2]);
	state_save_register_item("6840ptm", which, currptr->external_freq[2]);
	state_save_register_item("6840ptm", which, currptr->counter[2].w);
	state_save_register_item("6840ptm", which, currptr->latch[2].w);

	ptm6840_reset(which);

}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Reset Timer                                                           //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_reset(int which)
{
	int i;
	for ( i = 0; i < 3; i++ )
	{
		ptm[which].control_reg[i]	= 0x00;
		ptm[which].control_reg[0]	= 0x01;
		ptm[which].counter[i].w		= 0xffff;
		ptm[which].latch[i].w		= 0xffff;
		ptm[which].output[i]		= 0;
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Read Timer                                                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

int ptm6840_read(int which, int offset)
{
	ptm6840 *currptr = ptm + which;

	switch ( offset )
	{
		case PTM_6840_CTRL1 ://0

		break;

		case PTM_6840_CTRL2 ://1
		return currptr->status_reg;
		break;

		case PTM_6840_MSBBUF1://2
		{
			int result = compute_counter(0, which);

			/* clear the interrupt if the status has been read */
			if (currptr->status_read_since_int & (1 << 0))
				currptr->status_reg &= ~(1 << 0);
			update_interrupts(which);

			currptr->counter[0].b.l = result & 0xff;

			PLOG(("%06X: MC6840 #%d: Counter %d read = %04X\n", activecpu_get_previouspc(), which, 0, result));
			return result >> 8;
		}

		case PTM_6840_LSB1://3
		return currptr->counter[0].b.l;

		case PTM_6840_MSBBUF2://4
		{
			int result = compute_counter(1, which);

			/* clear the interrupt if the status has been read */
			if (currptr->status_read_since_int & (1 << 1))
				currptr->status_reg &= ~(1 << 1);
			update_interrupts(which);

			currptr->counter[1].b.l = result & 0xff;

			PLOG(("%06X: MC6840 #%d: Counter %d read = %04X\n", activecpu_get_previouspc(), which, 1, result));
			return result >> 8;
		}

		case PTM_6840_LSB2://5
		return currptr->counter[1].b.l;

		case PTM_6840_MSBBUF3://6
		{
			int result = compute_counter(2, which);

			/* clear the interrupt if the status has been read */
			if (currptr->status_read_since_int & (1 << 2))
				currptr->status_reg &= ~(1 << 2);
			update_interrupts(which);

			currptr->counter[2].b.l = result & 0xff;

			PLOG(("%06X: MC6840 #%d: Counter %d read = %04X\n", activecpu_get_previouspc(), which, 2, result));
			return result >> 8;
		}

		case PTM_6840_LSB3://7
		return currptr->counter[2].b.l;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Write Timer                                                           //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_write (int which, int offset, int data)
{
	ptm6840 *currptr = ptm + which;

	int idx;
	int i;
	UINT8 diffs;

	if (offset < 2)
	{
		idx = (offset == 1) ? 1 : (currptr->control_reg[1] & 0x01) ? 0 : 2;
		diffs = data ^ currptr->control_reg[idx];
		currptr->t3_divisor = (currptr->control_reg[3] & 0x01) ? 8 : 1;
		currptr->mode[idx] = (data>>3)&0x07;
		currptr->control_reg[idx] = data;

		PLOG(("MC6840 #%d : Control register %d selected\n",which,idx));
		PLOG(("operation mode   = %s\n", opmode[ currptr->mode[idx] ]));
		PLOG(("value            = %04X\n", currptr->control_reg[idx]));

		switch ( idx )
		{
			case 0:
			if (!(currptr->control_reg[0] & 0x80 ))
			{ // output cleared
				if ( currptr->intf )
				{
					if ( currptr->intf->out1_func ) currptr->intf->out1_func(0, 0);
				}
			}

			case 1:
			if (!(currptr->control_reg[1] & 0x80 ))
			{ // output cleared
				if ( currptr->intf )
				{
					if ( currptr->intf->out2_func ) currptr->intf->out2_func(0, 0);
				}
			}

			case 2:
			if (!(currptr->control_reg[2] & 0x80 ))
			{ // output cleared
				if ( currptr->intf )
				{
					if ( currptr->intf->out3_func ) currptr->intf->out3_func(0, 0);
				}
			}
		}

		/* reset? */
		if (idx == 0 && (diffs & 0x01))
		{
			/* holding reset down */
			if (data & 0x01)
			{
				PLOG(("MC6840 #%d : Timer reset\n",which));
				timer_enable(currptr->timer1,FALSE);
				timer_enable(currptr->timer2,FALSE);
				timer_enable(currptr->timer3,FALSE);
			}

			/* releasing reset */
			else
			{
				for (i = 0; i < 3; i++)
					reload_count(i,which);
			}

			currptr->status_reg = 0;

			update_interrupts(which);

			/* changing the clock source? (e.g. Zwackery) */
			if (diffs & 0x02)
			reload_count(idx,which);
		}

	}
	/* offsets 2, 4, and 6 are MSB buffer registers */

	switch ( offset )
	{
		case PTM_6840_MSBBUF1://2

		PLOG(("MC6840 #%d msbbuf1 = %02X\n", which, data));

		currptr->status_reg &= ~0x01;
		currptr->msb_buffer = data;
		if ( currptr->intf->irq_func )
		update_interrupts(which);
		break;

		case PTM_6840_MSBBUF2://4

		PLOG(("MC6840 #%d msbbuf2 = %02X\n", which, data));

		currptr->status_reg &= ~0x02;
		currptr->msb_buffer = data;
		update_interrupts(which);
		break;

		case PTM_6840_MSBBUF3://6

		PLOG(("MC6840 #%d msbbuf3 = %02X\n", which, data));

		currptr->status_reg &= ~0x04;
		currptr->msb_buffer = data;
		update_interrupts(which);
		break;

		/* offsets 3, 5, and 7 are Write Timer Latch commands */

		case PTM_6840_LSB1://3
		currptr->latch[0].b.u = currptr->msb_buffer;
		currptr->latch[0].b.l = data;
		/* clear the interrupt */
		currptr->status_reg &= ~(1 << 0);
		update_interrupts(which);

		/* reload the count if in an appropriate mode */
		if (!(currptr->control_reg[0] & 0x10))
			reload_count(0, which);

		PLOG(("%06X:MC6840 #%d: Counter %d latch = %04X\n", activecpu_get_previouspc(), which, 0, currptr->latch[0].w));
		break;

		case PTM_6840_LSB2://5
		currptr->latch[1].b.u = currptr->msb_buffer;
		currptr->latch[1].b.l = data;

		/* clear the interrupt */
		currptr->status_reg &= ~(1 << 1);
		update_interrupts(which);

		/* reload the count if in an appropriate mode */
		if (!(currptr->control_reg[1] & 0x10))
			reload_count(1, which);

		PLOG(("%06X:MC6840 #%d: Counter %d latch = %04X\n", activecpu_get_previouspc(), which, 1, currptr->latch[1].w));
		break;

		case PTM_6840_LSB3://7
		currptr->latch[2].b.u = currptr->msb_buffer;
		currptr->latch[2].b.l = data;

		/* clear the interrupt */
		currptr->status_reg &= ~(1 << 2);
		update_interrupts(which);

		/* reload the count if in an appropriate mode */
		if (!(currptr->control_reg[2] & 0x10))
			reload_count(2, which);

		PLOG(("%06X:MC6840 #%d: Counter %d latch = %04X\n", activecpu_get_previouspc(), which, 2, currptr->latch[2].w));
		break;
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t1_timeout: called if timer1 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t1_timeout(int which)
{
	ptm6840 *p = ptm + which;

	PLOG(("**ptm6840 %d t1 timeout**\n", which));

	if ( p->control_reg[0] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x01;
		p->interrupt[0] = 1;
		update_interrupts(which);
	}

	if ( p->control_reg[0] & 0x80 )
	{ // output enabled
		if ( p->intf )
		{
			p->output[0] = p->output[0]?0:1;
			PLOG(("**ptm6840 %d t1 output %d **\n", which, p->output[0]));
			if ( p->intf->out1_func ) p->intf->out1_func(0, p->output[0]);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t2_timeout: called if timer2 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t2_timeout(int which)
{
	ptm6840 *p = ptm + which;

	PLOG(("**ptm6840 %d t2 timeout**\n", which));

	if ( p->control_reg[1] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x02;
		p->interrupt[1] = 1;
		update_interrupts(which);
	}

	if ( p->control_reg[1] & 0x80 )
	{ // output enabled
		if ( p->intf )
		{
			p->output[1] = p->output[1]?0:1;
			if ( p->intf->out1_func ) p->intf->out1_func(0, p->output[0]);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t3_timeout: called if timer3 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t3_timeout(int which)
{
	ptm6840 *p = ptm + which;

	PLOG(("**ptm6840 %d t3 timeout**\n", which));

	if ( p->control_reg[2] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x04;
		p->interrupt[2] = 1;
		update_interrupts(which);
	}

	if ( p->control_reg[2] & 0x80 )
	{ // output enabled
		if ( p->intf )
		{
			p->output[2] = p->output[2]?0:1;
			if ( p->intf->out3_func ) p->intf->out3_func(0, p->output[2]);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g1: set gate1 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g1(int which, int state)
{
	ptm6840 *p = ptm + which;

	if ((p->mode[0] == 0)|(p->mode[0] == 2)|(p->mode[0] == 4)|(p->mode[0] == 6))
		{
		if (state == 0 && p->input[0])
		reload_count (0,which);
		}

	p->input[0] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_c1: set clock1 status (0 Or 1)                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_c1(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->clock[0] = state;

	if (!(p->control_reg[0] & 0x02))
	{
	timer_enable(p->timer1,state?TRUE:FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g2: set gate2 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g2(int which, int state)
{
	ptm6840 *p = ptm + which;

	if ((p->mode[1] == 0)|(p->mode[1] == 2)|(p->mode[1] == 4)|(p->mode[1] == 6))
		{
		if (state == 0 && p->input[1])
		reload_count (1,which);
		}

	p->input[1] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_c2: set clock2 status (0 Or 1)                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_c2(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->clock[1] = state;

	if (!(p->control_reg[1] & 0x02))
	{
	timer_enable(p->timer2,state?TRUE:FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g3: set gate3 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g3(int which, int state)
{
	ptm6840 *p = ptm + which;

	if ((p->mode[2] == 0)|(p->mode[2] == 2)|(p->mode[2] == 4)|(p->mode[2] == 6))
		{
		if (state == 0 && p->input[2])
		reload_count (2,which);
		}

	p->input[2] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_c3: set clock3 status (0 Or 1)                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_c3(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->clock[2] = state;
	if (!(p->control_reg[2] & 0x02))
	{
	timer_enable(p->timer3,state?TRUE:FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( ptm6840_0_r ) { return ptm6840_read(0, offset); }
READ8_HANDLER( ptm6840_1_r ) { return ptm6840_read(1, offset); }
READ8_HANDLER( ptm6840_2_r ) { return ptm6840_read(2, offset); }
READ8_HANDLER( ptm6840_3_r ) { return ptm6840_read(3, offset); }

WRITE8_HANDLER( ptm6840_0_w ) { ptm6840_write(0, offset, data); }
WRITE8_HANDLER( ptm6840_1_w ) { ptm6840_write(1, offset, data); }
WRITE8_HANDLER( ptm6840_2_w ) { ptm6840_write(2, offset, data); }
WRITE8_HANDLER( ptm6840_3_w ) { ptm6840_write(3, offset, data); }

READ16_HANDLER( ptm6840_0_msb_r ) { return ptm6840_read(0, offset); }
READ16_HANDLER( ptm6840_1_msb_r ) { return ptm6840_read(1, offset); }
READ16_HANDLER( ptm6840_2_msb_r ) { return ptm6840_read(2, offset); }
READ16_HANDLER( ptm6840_3_msb_r ) { return ptm6840_read(3, offset); }

WRITE16_HANDLER( ptm6840_0_msb_w ) { if (ACCESSING_MSB) ptm6840_write(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_1_msb_w ) { if (ACCESSING_MSB) ptm6840_write(1, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_2_msb_w ) { if (ACCESSING_MSB) ptm6840_write(2, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_3_msb_w ) { if (ACCESSING_MSB) ptm6840_write(3, offset, (data >> 8) & 0xff); }

READ16_HANDLER( ptm6840_0_lsb_r ) { return ptm6840_read(0, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_1_lsb_r ) { return ptm6840_read(1, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_2_lsb_r ) { return ptm6840_read(2, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_3_lsb_r ) { return ptm6840_read(3, offset << 8 | 0x00ff); }

WRITE16_HANDLER( ptm6840_0_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(0, offset, data & 0xff);}
WRITE16_HANDLER( ptm6840_1_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(1, offset, data & 0xff);}
WRITE16_HANDLER( ptm6840_2_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(2, offset, data & 0xff);}
WRITE16_HANDLER( ptm6840_3_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(3, offset, data & 0xff);}
