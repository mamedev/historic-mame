/*  Z80 FMLY.H   Z80 FAMILY IC EMURATION */

#include "Z80.h"

typedef struct z80pio_f Z80PIO;
struct z80pio_f {
	int mode[2];		/* mode 00=in,01=out,02=i/o,03=bit */
	int enable[2];		/* interrupt enable */
	int mask[2];		/* mask folowers */
	int dir[2];			/* direction (bit mode) */
	int vector[2];		/* interrupt vector */
	int irq[2];			/* interrupt request */
	int rdy[2];			/* ready pin */
	int in[2];			/* input port  */
	int out[2];			/* output port */
};

#define MAX_IRQ	20

typedef struct pendirq_f PendIRQ;
struct pendirq_f {
	unsigned long time;
	int irq;
};

typedef struct z80ctc_f Z80CTC;
struct z80ctc_f {
	int vector;					/* interrupt vector */
	int sys_clk;				/* system clock */
	int mode[4];				/* current mode */
	int tconst[4];				/* time constant * 256 */
	int down[4];				/* down counter * 256 */
	unsigned long fall[4];	/* time of the next falling edge */
	unsigned long last[4];	/* time of the last update */
	PendIRQ irq[MAX_IRQ];	/* pending IRQ's */
};

void z80ctc_reset( Z80CTC *ctc , int system_clock );
void z80ctc_w( Z80CTC *ctc , int ch , int data );
int  z80ctc_r( Z80CTC *ctc , int ch );
int  z80ctc_update( Z80CTC *ctc , int ch , int cntclk , int cnthold );
int  z80ctc_irq_r( Z80CTC *ctc );

void z80pio_reset( Z80PIO *pio );
void z80pio_w( Z80PIO *pio , int ch , int cd , int data );
int  z80pio_r( Z80PIO *pio , int ch , int cd );
void z80pio_p_w( Z80PIO *pio , int ch , int data );
int  z80pio_p_r( Z80PIO *pio , int ch );
int  z80pio_irq_r( Z80PIO *pio );
