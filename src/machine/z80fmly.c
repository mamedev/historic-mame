/***************************************************************************

  Z80 FMLY.C   Z80 FAMILY CHIP EMURATOR for MAME Ver.0.1 alpha

  Support chip :  Z80PIO , Z80CTC

  It is not support dizzy chain.

  Copyright(C) 1997 Tatsuyuki Satoh.

  This version are tested starforce driver.

pending:
	Z80CTC , Counter mode & Timer with Trigrt start :not support Triger level

***************************************************************************/

#include <stdio.h>
#include "driver.h"
#include "z80fmly.h"

#define DEBUG

void z80ctc_reset( Z80CTC *ctc , int system_clock )
{
	int ch;

	for( ch = 0 ; ch <= 3 ; ch ++ ){
		ctc->mode[ch] &= 0x03;
		ctc->timec[ch] = 0x10000;
		ctc->cnt[ch] = 0;
	}
	ctc->sys_clk = system_clock;
}

void z80ctc_w( Z80CTC *ctc , int ch , int data )
{
	if( ctc->mode[ch]&0x04 ){	/* time constact */
		ctc->timec[ch] = (data?data:0x100)<<8; /* 1 - 256 */
		ctc->cnt[ch] = 0;
		ctc->mode[ch] &= 0xf9;
		if (errorlog) fprintf(errorlog,"CTC Ch.%d TIMER %02x\n" , ch , ctc->timec[ch]>>8 );
#if 1
		/* This routine are abnoemal case , used by starforce sound cpu */
		/* ctc2.tc pending & ctc0.tc write , ctc2.tc is fix. */
		/* ctc3.tc pending & ctc1.tc write , ctc3.tc is fix. */
		ch ^= 0x02;
		if( ctc->mode[ch]&0x04 ){	/* time constact */
			ctc->timec[ch] = (data?data:0x100)<<8; /* 1 - 256 */
			ctc->cnt[ch] = 0;
			ctc->mode[ch] &= 0xf9;	/* clear constant & start ctc */
			if (errorlog) fprintf(errorlog,"CTC Ch.%d TIMER %02x\n" , ch , ctc->timec[ch]>>8 );
		}
#endif
		return;
	}

	if( data & 0x01 ){			/* control word */
		ctc->mode[ch] = data;
		if (errorlog) fprintf(errorlog,"CTC ch.%d mode %02x\n" , ch , data );
	}else{					/* interrupt vector */
		if( !(ch&0x02) ){
			/* channel 0 or 2 abavle ( zilog Z80A ) */
			ctc->vector = (data&0xf8);
			if (errorlog) fprintf(errorlog,"CTC Vector %02x\n" , data );
		}
	}
}

int z80ctc_r( Z80CTC *ctc , int ch )
{
	if (errorlog) fprintf(errorlog,"CTC-%c read \n",'0'+ch );
	return ( (ctc->timec[ch&0x03]-ctc->cnt[ch&0x03]) >>8);
}

/* 
		sysclk = update system clocks.
		cntclk = update clocks in 'CLKIN' pin. ( external clock )
*/
int z80ctc_update( Z80CTC *ctc , int ch , int sysclk , int cntclk )
{
	int upclk,zco;

	ch = ch & 0x03;

	/* enable check */
	if( (ctc->mode[ch] & 0x02) ) return 0;
	/* select clock */
	if( ctc->mode[ch] & 0x40 ) upclk = cntclk<<8;
	else
	{		/* timer mode */
		if( !(ctc->mode[ch]&0x10) )
		{
			if( cntclk ) ctc->mode[ch]&= 0xef;	/* Start */
			else return 0; /* Wait Trigrt */
		}
		upclk = sysclk;
		if( !(ctc->mode[ch]&0x20) ) upclk = upclk<<4; /* priscaler = /16 */
	}
	if( !upclk ) return 0;

	/* counter update & calcrate zco plus */
	upclk += ctc->cnt[ch];
	zco = upclk / ctc->timec[ch];
	ctc->cnt[ch] = upclk % ctc->timec[ch];
	/* interrupt check */
	if( zco && (ctc->mode[ch]&0x80) ){
		ctc->irq[ch] += zco;	/* interrupt request */
	}
	return zco;
}

int z80ctc_irq_r( Z80CTC *ctc )
{
	if( ctc->irq[0] ){
		ctc->irq[0]--;
		return ctc->vector;
	}
	if( ctc->irq[1] ){
		ctc->irq[1]--;
		return ctc->vector+2;
	}
	if( ctc->irq[2] ){
		ctc->irq[2]--;
		return ctc->vector+4;
	}
	if( ctc->irq[3] ){
		ctc->irq[3]--;
		return ctc->vector+6;
	}
	return Z80_IGNORE_INT;
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
