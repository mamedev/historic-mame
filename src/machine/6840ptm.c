/**********************************************************************


    Motorola 6840 PTM interface and emulation

    This function is a simple emulation of up to 4 MC6840
    Programmable Timer Module

    Written By El Condor based on previous work by Aaron Giles,
   'Re-Animator' and Mathis Rosenhauer.

    Todo:
         Get the outputs to actually do something!
         Find out why this does not appear to work.

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
			outputenable[3],// output states
				   input[3],// input  gate states
				   clock[3],// clock  states
				  enable[3],
			   interrupt[3],
			   irqenable[3],

				 status_reg,
				 lsb_buffer,
				 msb_buffer;
	double	internal_period,
		 external_period[3];

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

#ifdef PTMVERBOSE
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
//Reload Counter                                                         //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void reload_count(int idx, int which)
{
	double period;
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
		timer_adjust(currptr->timer1, TIME_NEVER, 0, 0);
		currptr->enable[0] = 0;
		case 1:
		timer_adjust(currptr->timer2, TIME_NEVER, 0, 0);
		currptr->enable[1] = 0;
		case 2:
		timer_adjust(currptr->timer3, TIME_NEVER, 0, 0);
		currptr->enable[2] = 0;
		}
		return;
	}

	/* determine the clock period for this timer */
	if (currptr->control_reg[idx] & 0x02)
		period = currptr->internal_period;
	else
		period = currptr->external_period[idx];

	/* determine the number of clock periods before we expire */
	count = currptr->counter[idx].w;
	if (currptr->control_reg[idx] & 0x04)
		count = ((count >> 8) + 1) * ((count & 0xff) + 1);
	else
		count = count + 1;

	/* set the timer */
	PLOG(("reload_count(%d): period = %f  count = %d\n", idx, period, count));
	switch (idx)
	{
	case 0:
	timer_adjust(currptr->timer1, period * (double)count, (count << 2) + 0, 0);
	currptr->enable[0] = 1;
	case 1:
	timer_adjust(currptr->timer2, period * (double)count, (count << 2) + 1, 0);
	currptr->enable[1] = 1;
	case 2:
	timer_adjust(currptr->timer3, period * (double)count, (count << 2) + 2, 0);
	currptr->enable[2] = 1;
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//Unconfigure Timer                                                      //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_unconfig(void)
{
	int i;

	i = 0;
	while ( i < PTM_6840_MAX )
	{
		if ( ptm[i].timer1 );
		timer_adjust(ptm[i].timer1, TIME_NEVER, 0, 0);
		ptm[i].timer1 = NULL;

		if ( ptm[i].timer2 );
		timer_adjust(ptm[i].timer2, TIME_NEVER, 0, 0);
		ptm[i].timer2 = NULL;

		if ( ptm[i].timer3 );
		timer_adjust(ptm[i].timer3, TIME_NEVER, 0, 0);
		ptm[i].timer3 = NULL;

		i++;
  	}
	memset (&ptm, 0, sizeof (ptm));
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//Configure Timer                                                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_config(int which, const ptm6840_interface *intf)
{
	ptm6840 *currptr = ptm + which;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call ptm6840_config at init time!");
	assert_always((which >= 0) && (which < PTM_6840_MAX), "ptm6840_config called on an invalid PTM!");
	assert_always(intf, "ptm6840_config called with an invalid interface!");
	ptm[which].intf = intf;

	ptm[which].internal_period = currptr->intf->internal_clock;

	ptm[which].external_period[0] = currptr->intf->external_clock1;
	ptm[which].external_period[1] = currptr->intf->external_clock2;
	ptm[which].external_period[2] = currptr->intf->external_clock3;

	ptm[which].timer1 = timer_alloc(ptm6840_t1_timeout);
	ptm[which].timer2 = timer_alloc(ptm6840_t2_timeout);
	ptm[which].timer3 = timer_alloc(ptm6840_t3_timeout);
	ptm6840_reset(which);
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//Reset Timer                                                            //
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
//Read Timer                                                             //
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

//      if ( currptr->status_reg & 0x07 ) currptr->status_reg |=  0x80;
//      else                              currptr->status_reg &= ~0x80;
		return currptr->status_reg;
		break;

		case PTM_6840_MSBBUF1://2
		currptr->status_reg &= ~0x01;
		return currptr->counter[0].b.u;

		case PTM_6840_LSB1://3
		currptr->status_reg &= ~0x01;
		return currptr->counter[0].b.l;

		case PTM_6840_MSBBUF2://4
		currptr->status_reg &= ~0x02;
		PLOG(("6840PTM #%d TIMER 2 = %02X\n", which, currptr->counter[1].w));
		return currptr->counter[1].b.u;

		case PTM_6840_LSB2://5
		currptr->status_reg &= ~0x02;
		return currptr->counter[1].b.l;

		case PTM_6840_MSBBUF3://6
		currptr->status_reg &= ~0x04;
		return currptr->counter[2].b.u;

		case PTM_6840_LSB3://7
		currptr->status_reg &= ~0x04;
		return currptr->counter[2].b.l;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//Write Timer                                                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_write (int which, int offset, int data)
{
	ptm6840 *currptr = ptm + which;

	int idx=0;
	int i;

	if (offset < 2)
	{
		UINT8 diffs = data ^ currptr->control_reg[idx];
		idx = (offset == 1) ? 1 : (currptr->control_reg[1] & 0x01) ? 0 : 2;

		PLOG(("MC6840 #%d : Control register %d selected\n",which,idx));
		PLOG(("operation mode   = %s\n", opmode[ (data>>3)&0x07 ]));

		currptr->control_reg[idx] = data;

		/* reset? */
		if (idx == 0 && (diffs & 0x01))
		{
			/* holding reset down */
			if (data & 0x01)
			{
				PLOG(("MC6840 #%d : Timer reset\n",which));
				timer_adjust(currptr->timer1, TIME_NEVER, 0, 0);
				timer_adjust(currptr->timer2, TIME_NEVER, 0, 0);
				timer_adjust(currptr->timer3, TIME_NEVER, 0, 0);
				currptr->enable[0] = 0;
				currptr->enable[1] = 0;
				currptr->enable[2] = 0;
			}

			/* releasing reset */
			else
			{
				for (i = 0; i < 3; i++)
					reload_count(i,which);
			}

			currptr->status_reg = 0;

			if ( currptr->intf->irq_func )
			{
				if (currptr->irqenable[0])
				{
					currptr->intf->irq_func(1);
				}
			}

			/* changing the clock source? (e.g. Zwackery) */
			if (diffs & 0x02)
			reload_count(idx,which);
		}
	}
	/* offsets 2, 4, and 6 are MSB buffer registers */
	switch ( offset )
	{
		case PTM_6840_MSBBUF1://2

		PLOG(("6840PTM #%d msbbuf1 = %02X\n", which, data));

		currptr->status_reg &= ~0x01;
		currptr->msb_buffer = data;
		if ( currptr->intf->irq_func )
		{
			if (currptr->irqenable[0])
			{
				currptr->intf->irq_func(0);
			}
		}
		break;

		case PTM_6840_MSBBUF2://4

		PLOG(("6840PTM #%d msbbuf2 = %02X\n", which, data));

		currptr->status_reg &= ~0x02;
		currptr->msb_buffer = data;
		if ( currptr->intf->irq_func )
		{
			if (currptr->irqenable[1])
			{
				currptr->intf->irq_func(0);
			}
		}
		break;

		case PTM_6840_MSBBUF3://6

		PLOG(("6840PTM #%d msbbuf3 = %02X\n", which, data));

		currptr->status_reg &= ~0x04;
		currptr->msb_buffer = data;
		if ( currptr->intf->irq_func )
		{
			if (currptr->irqenable[2])
			{
				currptr->intf->irq_func(0);
			}
		}
		break;
	}
	/* offsets 3, 5, and 7 are Write Timer Latch commands */
	if (offset >2)
	{
		int counter = (offset - 2) / 2;
		currptr->latch[counter].w = (currptr->msb_buffer << 8) | (data & 0xff);

		/* clear the interrupt */
		currptr->status_reg &= ~(1 << counter);
		if ( currptr->intf->irq_func )
		{
			if (currptr->irqenable[counter])
			{
				currptr->intf->irq_func(0);
			}
		}

		/* reload the count if in an appropriate mode */
		if (!(currptr->control_reg[counter] & 0x10))
			reload_count(counter, which);

		PLOG(("%06X:M6840#%d: Counter %d latch = %04X\n", which, activecpu_get_previouspc(), counter, currptr->latch[counter].w));
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

	if ( p->control_reg[0] & 0x80 )
	{
		logerror("**ptm6840 %d t1 timeout**\n", which);

		if ( p->control_reg[0] & 0x40 )
		{ // interrupt enabled
			p->status_reg |= 0x01;
			if ( p->intf->irq_func  ) p->intf->irq_func(1);
		}

		if ( p->intf )
		{
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

	if ( p->control_reg[1] & 0x80 )
	{
		logerror("**ptm6840 %d t2 timeout**\n", which);

		if ( p->control_reg[1] & 0x40 )
		{ // interrupt enabled
			p->status_reg |= 0x02;
			if ( p->intf->irq_func  ) p->intf->irq_func(1);
		}

		if ( p->intf )
		{
			if ( p->intf->out2_func ) p->intf->out2_func(0, p->output[1]);
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

	if ( p->control_reg[2] & 0x80 )
	{
		logerror("**ptm6840 %d t3 timeout**\n", which);

		if ( p->control_reg[2] & 0x40 )
		{ // interrupt enabled
			p->status_reg |= 0x04;
			if ( p->intf->irq_func  ) p->intf->irq_func(1);
		}

		if ( p->intf )
		{
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

	p->input[0] = state;
}

void ptm6840_set_c1(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[0] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g2: set gate2 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g2(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[1] = state;
}

void ptm6840_set_c2(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[1] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g3: set gate3 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g3(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[2] = state;
}

void ptm6840_set_c3(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[2] = state;
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

READ16_HANDLER( ptm6840_0_msb_r ) { return ptm6840_read(0, offset) << 8; }
READ16_HANDLER( ptm6840_1_msb_r ) { return ptm6840_read(1, offset) << 8; }
READ16_HANDLER( ptm6840_2_msb_r ) { return ptm6840_read(2, offset) << 8; }
READ16_HANDLER( ptm6840_3_msb_r ) { return ptm6840_read(3, offset) << 8; }

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
