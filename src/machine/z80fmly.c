/***************************************************************************

  Z80 FMLY.C   Z80 FAMILY CHIP EMURATOR for MAME Ver.0.1 alpha

  Support chip :  Z80PIO , Z80CTC

  It is not support dizzy chain.

  Copyright(C) 1997 Tatsuyuki Satoh.

  This version are tested starforce driver.

  8/21/97 -- Heavily modified by Aaron Giles to be much more accurate for the MCR games
  8/27/97 -- Rewritten a second time by Aaron Giles, with the datasheets in hand

pending:
	Z80CTC , Counter mode & Timer with Trigrt start :not support Triger level

***************************************************************************/

#include <stdio.h>
#include "driver.h"
#include "z80fmly.h"

#define DEBUG

/* these are the bits of the incoming commands to the CTC */
#define INTERRUPT			0x80
#define INTERRUPT_ON 	0x80
#define INTERRUPT_OFF	0x00

#define MODE				0x40
#define MODE_TIMER		0x00
#define MODE_COUNTER		0x40

#define PRESCALER			0x20
#define PRESCALER_256	0x20
#define PRESCALER_16		0x00

#define EDGE				0x10
#define EDGE_FALLING		0x00
#define EDGE_RISING		0x10

#define TRIGGER			0x08
#define TRIGGER_AUTO		0x00
#define TRIGGER_CLOCK	0x08

#define CONSTANT			0x04
#define CONSTANT_LOAD	0x04
#define CONSTANT_NONE	0x00

#define RESET				0x02
#define RESET_CONTINUE	0x00
#define RESET_ACTIVE		0x02

#define CONTROL			0x01
#define CONTROL_VECTOR	0x00
#define CONTROL_WORD		0x01

/* these extra bits help us keep things accurate */
#define WAITING_FOR_TRIG	0x100


void z80ctc_reset (Z80CTC *ctc, int system_clock)
{
	int ch;

	/* erase everything */
	memset (ctc, 0, sizeof (*ctc));

	/* insert default timers */
	for (ch = 0; ch < 4; ch++)
	{
		ctc->mode[ch] = RESET_ACTIVE;
		ctc->down[ch] = 0x7fffffff;
		ctc->last[ch] = cpu_gettotalcycles();
		ctc->tconst[ch] = 0x100;
	}

	/* set the system clock */
	ctc->sys_clk = system_clock;
}


void z80ctc_w (Z80CTC *ctc, int ch, int data)
{
	int mode, i;

	/* keep channel within range, and get the current mode */
	ch &= 3;
	mode = ctc->mode[ch];

	/* if we're waiting for a time constant, this is it */
	if ((mode & CONSTANT) == CONSTANT_LOAD)
	{
		/* set the time constant (0 -> 0x100) */
		ctc->tconst[ch] = data ? data : 0x100;

		/* clear the internal mode -- we're no longer waiting */
		ctc->mode[ch] &= ~CONSTANT;

		/* also clear the reset, since the constant gets it going again */
		ctc->mode[ch] &= ~RESET;

		/* if we're in timer mode.... */
		if ((mode & MODE) == MODE_TIMER)
		{
			/* if we're triggering on the time constant, reset the down counter now */
			if ((mode & TRIGGER) == TRIGGER_AUTO)
				ctc->down[ch] = ctc->tconst[ch] << 8;

			/* else set the bit indicating that we're waiting for the appropriate trigger */
			else
			{
				unsigned long delta = ctc->fall[ch] - cpu_gettotalcycles ();

				/* if we're waiting for the falling edge and we know when that is, set it up */
				if ((mode & EDGE) == EDGE_FALLING && delta < ctc->sys_clk)
					ctc->down[ch] = (ctc->tconst[ch] << 8) + delta;

				/* otherwise, just indicate that we're waiting for a trigger */
				else
					ctc->mode[ch] |= WAITING_FOR_TRIG;
			}
		}

		/* else just set the down counter now */
		else
			ctc->down[ch] = ctc->tconst[ch] << 8;

		/* all done here */
		return;
	}

	/* if we're writing the interrupt vector, handle it specially */
	if ((data & CONTROL) == CONTROL_VECTOR && ch == 0)
	{
		ctc->vector = data & 0xf8;
		if (errorlog) fprintf (errorlog, "CTC Vector = %02x\n", ctc->vector);
		return;
	}

	/* this must be a control word */
	if ((data & CONTROL) == CONTROL_WORD)
	{
		/* set the new mode */
		ctc->mode[ch] = data;
		if (errorlog) fprintf (errorlog,"CTC ch.%d mode = %02x\n", ch, data);

		/* if we're being reset, clear out any pending interrupts for this channel */
		if ((data & RESET) == RESET_ACTIVE)
		{
			for (i = 0; i < MAX_IRQ; i++)
				if (ctc->irq[i].irq == ctc->vector + (ch << 1))
					ctc->irq[i].irq = 0;
		}

		/* all done here */
		return;
	}
}


int z80ctc_r (Z80CTC *ctc, int ch)
{
	/* keep channel within range */
	ch &= 3;

	/* return the current down counter value */
	return ctc->down[ch] >> 8;
}


int z80ctc_update (Z80CTC *ctc, int ch, int cntclk, int cnthold)
{
	unsigned long time = cpu_gettotalcycles ();
	int mode, zco = 0;
	int upclk, sysclk, i;

	/* keep channel within range, and get the current mode */
	ch &= 3;
	mode = ctc->mode[ch];

	/* increment the last timer update */
	sysclk = time - ctc->last[ch];
	ctc->last[ch] = time;

	/* if we got an external clock, handle it */
	if (cntclk)
	{
		/* set the time of the falling edge here */
		ctc->fall[ch] = time + cnthold;

		/* if this timer is waiting for a trigger, we can resolve it now */
		if (mode & WAITING_FOR_TRIG)
		{
			/* first clear the waiting flag */
			ctc->mode[ch] &= ~WAITING_FOR_TRIG;
			mode &= ~WAITING_FOR_TRIG;

			/* rising edge? */
			if ((mode & EDGE) == EDGE_RISING)
				ctc->down[ch] = ctc->tconst[ch] << 8;

			/* falling edge? */
			else
				ctc->down[ch] = (ctc->tconst[ch] << 8) + cnthold;
		}
	}

	/* if this channel is reset, we're done */
	if ((mode & RESET) == RESET_ACTIVE)
		return 0;

	/* if this channel is in timer mode waiting for a trigger, we're done */
	if (mode & WAITING_FOR_TRIG)
		return 0;

	/* select the clock to use */
	if ((mode & MODE) == MODE_TIMER)
	{
		if ((mode & PRESCALER) == PRESCALER_16)
			upclk = sysclk << 4;
		else
			upclk = sysclk;
	}
	else
		upclk = cntclk << 8;

	/* if the clock isn't updated this time, bail */
	if (!upclk)
		return 0;

	/* decrement the counter and count zero crossings */
	ctc->down[ch] -= upclk;
	while (ctc->down[ch] <= 0)
	{
		/* if we're doing interrupts, add a pending one */
		if ((mode & INTERRUPT) == INTERRUPT_ON)
		{
			for (i = 0; i < MAX_IRQ; i++)
				if (!ctc->irq[i].irq)
				{
					ctc->irq[i].irq = ctc->vector + (ch << 1);
					ctc->irq[i].time = time + ctc->down[ch];
					break;
				}
		}

		/* update the counters */
		zco += 1;
		ctc->down[ch] += ctc->tconst[ch] << 8;
	}

	/* log it */
	if (errorlog && zco)
	{
		if ((mode & MODE) == MODE_COUNTER)
			fprintf (errorlog, "CTC Ch.%d trigger\n", ch);
		else
			fprintf (errorlog, "CTC Ch.%d rollover x%d\n", ch, zco);
	}

	/* return the number of zero crossings */
	return zco;
}


int z80ctc_irq_r (Z80CTC *ctc)
{
	unsigned long basetime = cpu_gettotalcycles () - ctc->sys_clk;	/* no more than 1 second behind! */
	unsigned long time, earliestTime = 0xffffffff;
	int i, earliest = -1;

	/* find the earliest IRQ */
	for (i = 0; i < MAX_IRQ; i++)
		if (ctc->irq[i].irq)
		{
			time = ctc->irq[i].time - basetime;
			if (time < earliestTime)
			{
				earliest = i;
				earliestTime = time;
			}
		}

	/* if none, bail */
	if (earliest == -1)
		return Z80_IGNORE_INT;

	/* otherwise, return the IRQ and clear it */
	i = ctc->irq[earliest].irq;
	ctc->irq[earliest].irq = 0;
	return i;
}


void z80pio_reset( Z80PIO *pio )
{
	int i;
	for( i = 0 ; i <= 1 ; i++){
		pio->mask[i]   = 0xff;	/* mask all on */
		pio->enable[i] = 0x00;	/* disable     */
		pio->mode[i]   = 0x01;	/* mode input  */
		pio->dir[i]    = 0x01;	/* mode input  */
		pio->rdy[i]    = 0x00;	/* RDY = low   */
		pio->out[i]    = 0x00;	/* outdata = 0 */
	}
}

void z80pio_w( Z80PIO *pio , int ch , int cd , int data )
{
	if( ch ) ch = 1;

	if( cd ){			/* controll port */
		if( pio->mode[ch] == 0x13 ){		/* direction */
			pio->dir[ch] = data;
			pio->mode[ch] = 0x03;
			return;
		}
		if( pio->enable[ch] & 0x10 ){	/* mask folows */
			pio->mask[ch] = data;
			pio->enable[ch] &= 0xef;
			return;
		}
		switch( data & 0x0f ){
		case 0x0f:	/* mode select 0=out,1=in,2=i/o,3=bit */
			pio->mode[ch] = (data >> 6 );
			if( pio->mode[ch] == 0x03 ) pio->mode[ch] = 0x13;
			if (errorlog) fprintf(errorlog,"PIO-%c Mode %02x\n",'A'+ch,data );
			break;
		case 0x07:		/* interrupt control */
			pio->enable[ch] = data;
			pio->mask[ch]   = 0x00;
			if (errorlog) fprintf(errorlog,"PIO-%c int %02x\n",'A'+ch,data );
			break;
		case 0x03:		/* interrupt enable controll */
			pio->enable[ch] = (pio->enable[ch]&0x7f)|(data &0x80);
			if (errorlog) fprintf(errorlog,"PIO-%c ena %02x\n",'A'+ch,data );
			break;
		default:
			if( !(data&1) ) pio->vector[ch] = data;
			if (errorlog) fprintf(errorlog,"PIO-%c vector %02x\n",'A'+ch,data );
		}
	}else{				/* data port */
		pio->out[ch] = data;		/* latch out data */
		switch( pio->mode[ch] ){
		case 0x00:			/* mode 0 output */
		case 0x02:			/* mode 2 i/o */
			pio->rdy[ch] = 1;	/* ready = H */
			return;
		case 0x01:			/* mode 1 intput */
		case 0x03:			/* mode 0 bit */
			return;
		default:
			if (errorlog) fprintf(errorlog,"PIO-%c data write,bad mode\n",'A'+ch );
		}
	}
}

/* starforce emurate Z80PIO subset */
/* ch.A mode 1 input handshake mode from sound command */
/* ch.b not use */
int z80pio_r( Z80PIO *pio , int ch , int cd )
{
	if( ch ) ch = 1;

	if( cd ){
		if (errorlog) fprintf(errorlog,"PIO-%c controll read\n",'A'+ch );
		return 0;
	}
	switch( pio->mode[ch] ){
	case 0x00:			/* mode 0 output */
		return pio->out[ch];
	case 0x01:			/* mode 1 intput */
		pio->rdy[ch] = 1;	/* ready = H */
		return pio->in[ch];
	case 0x02:			/* mode 2 i/o */
		if( ch ) if (errorlog) fprintf(errorlog,"PIO-B mode 2 \n");
		pio->rdy[1] = 1;	/* brdy = H */
		return pio->in[0];
	case 0x03:			/* mode 0 bit */
		return (pio->in[ch]&pio->dir[ch])|(pio->out[ch]&~pio->dir[ch]);
	}
	if (errorlog) fprintf(errorlog,"PIO-%c data read,bad mode\n",'A'+ch );
	return 0;
}

/* z80pio port write */
void z80pio_p_w( Z80PIO *pio , int ch , int data )
{
	if( ch ) ch = 1;

	pio->in[ch]  = data;
	switch( pio->mode[ch] ){
	case 0x01:
	case 0x02:
		pio->rdy[ch] = 0;
	}
}

/* z80pio port read */
int z80pio_p_r( Z80PIO *pio , int ch )
{
	if( ch ) ch = 1;

	switch( pio->mode[ch] ){
	case 0x00:
		pio->rdy[ch] = 0;
		return pio->out[ch];
		break;
	case 0x02:
		pio->rdy[1] = 0;		/* port a only */
		return pio->out[ch];
	case 0x03:
		return (pio->in[ch]&pio->dir[ch])|(pio->out[ch]&~pio->dir[ch]);
	}
	return 0;
}

int z80pio_irq_r( Z80PIO *pio )
{
	int data;

	if( pio->enable[0]&0x80 ){		/* channel A */
		switch( pio->mode[0] ){
		case 0x00:			/* mode 0 output */
		case 0x02:
			if( !(pio->rdy[1]) ) return pio->vector[1];
		case 0x01:			/* mode 1/2 intput */
			if( !(pio->rdy[0]) ) return pio->vector[0];
			break;
		case 0x03:
			data = (pio->in[0]&pio->dir[0])|(pio->out[0]&~pio->dir[0]);
			data &= ~pio->mask[0];
			if( !(pio->enable[0]&0x20) ) data ^= pio->mask[0]; /* low active */
			if( pio->enable[0]&0x40 ){	/* and */
				if( data == pio->mask[0] ) return pio->vector[0];
			}else{						/* or  */
				if( data                ) return pio->vector[0];
			}
		}
	}
	if( pio->enable[1]&0x80 ){		/* channel B */
		switch( pio->mode[1] ){
		case 0x00:			/* mode 0 output */
		case 0x01:			/* mode 1 intput */
			if( !(pio->rdy[1]) ) return pio->vector[1];
			break;
		case 0x03:
			data = (pio->in[1]&pio->dir[1])|(pio->out[1]&~pio->dir[1]);
			data &= ~pio->mask[1];
			if( !(pio->enable[1]&0x20) ) data ^= pio->mask[1]; /* low active */
			if( pio->enable[1]&0x40 ){	/* and */
				if( data == pio->mask[1] ) return pio->vector[1];
			}else{						/* or  */
				if( data                ) return pio->vector[1];
			}
		}
	}
	return Z80_IGNORE_INT;
}
