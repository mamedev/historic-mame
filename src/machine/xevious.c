/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw\generic.h"
#include "Z80.h"


unsigned char *xevious_sharedram;
unsigned char *xevious_vlatches;
static unsigned char interrupt_enable_1,interrupt_enable_2,interrupt_enable_3;
/* static int    HiScore; */

static unsigned char *rom2a;
static unsigned char *rom2b;
static unsigned char *rom2c;
static int xevious_bs[2];


int xevious_init_machine(const char *gamename)
{
	/* halt the slave CPUs until they're reset */
	cpu_halt(1,0);
	cpu_halt(2,0);

	Machine->memory_region[0][0x8c00] = 1;
	Machine->memory_region[0][0x8c01] = 1;

	rom2a = Machine->memory_region[4];
	rom2b = Machine->memory_region[4]+0x1000;
	rom2c = Machine->memory_region[4]+0x3000;

	return 0;
}

/* emulation for schematic 9B */
void xevious_bs_w(int offset, int data)
{
	xevious_bs[offset & 0x01] = data;
}

int xevious_bb_r(int offset )
{
	int adr_2b,adr_2c;
	int dat1,dat2;


	/* get BS to 12 bit data from 2A,2B */
	adr_2b = ((xevious_bs[1]&0x7e)<<6)|((xevious_bs[0]&0xfe)>>1);
	if( adr_2b & 1 ){
		/* high bits select */
		dat1 = ((rom2a[adr_2b>>1]&0xf0)<<4)|rom2b[adr_2b];
	}else{
	    /* low bits select */
	    dat1 = ((rom2a[adr_2b>>1]&0x0f)<<8)|rom2b[adr_2b];
	}
	adr_2c = (dat1 & 0x1ff)<<2;
	if( offset & 0x01 )
		adr_2c += (1<<11);	/* signal 4H to A11 */
	if( (xevious_bs[0]&1) ^ ((dat1>>10)&1) )
		adr_2c |= 1;
	if( (xevious_bs[1]&1) ^ ((dat1>>9)&1) )
		adr_2c |= 2;
	if( offset & 0x01 ){
		/* return BB1 */
		dat2 = rom2c[adr_2c];
	}else{
		/* return BB0 */
		dat2 =rom2c[adr_2c];
		/* swap bit 6 & 7 */
		dat2 = (dat2 & 0x3f) | ((dat2 & 0x80) >> 1) | ((dat2 & 0x40) << 1);
		/* flip x & y */
		dat2 ^= (dat1 >> 4) & 0x40;
		dat2 ^= (dat1 >> 2) & 0x80;
	}
	return dat2;
}

int xevious_sharedram_r(int offset)
{
	return xevious_sharedram[offset];
}

void xevious_sharedram_w(int offset,int data)
{
	xevious_sharedram[offset] = data;
}



int xevious_dsw_r(int offset)
{
	int bit0,bit1;


	bit0 = (input_port_0_r(0) >> offset) & 1;
	bit1 = (input_port_1_r(0) >> offset) & 1;

	return bit0 | (bit1 << 1);
}



/***************************************************************************

 Emulate the custom IO chip.

 In the real Xevious machine, the chip would cause an NMI on CPU #1 to ask
 for data to be transferred. We don't bother causing the NMI, we just look
 into the CPU register to see where the data has to be read/written to, and
 emulate the behaviour of the NMI interrupt.

***************************************************************************/
void xevious_customio_w(int offset,int data)
{
	static int mode,credits;
	static int protect;
	Z80_Regs regs;


	Z80_GetRegs(&regs);

        if (errorlog) fprintf(errorlog,"%04x: custom IO command %02x, HL = %04x DE = %04x BC = %04x  %02x %02x %02x\n",
	   cpu_getpc(),data,regs.HL2.D,regs.DE2.D,regs.BC2.D, RAM[regs.HL2.D], RAM[(regs.HL2.D)+1], RAM[(regs.HL2.D)+2]);

	/* read data function */
	/* set data to memory 7000- */
	switch (data)
	{
		case 0x10:	/* nop */
			return;

		case 0x71:
			{
				static int coin,start1,start2,fire1,fire2;
				int in, dir,stsw;

				in = readinputport(0);
				/* check if the user inserted a coin */
				if ( !(in&0x10) )
				{
					if (coin == 0 && credits < 99) credits++;
					coin = 1;
				}
				else coin = 0;

				stsw = 0;
				/* check for 1 player start button */
				if ( !(in&0x01) )
				{
					if (start1 == 0 && credits >= 1)
					{
					 credits--;
					 stsw = 0xff; /* ?? */
					}
					start1 = 1;
				}
				else start1 = 0;

				/* check for 2 players start button */
				if ( !(in&0x02) )
				{
					if (start2 == 0 && credits >= 2)
					{
					 credits -= 2;
					 stsw = 0xfe;	/* ?? */
					}
					start2 = 1;
				}
				else start2 = 0;

				in = readinputport(1);

			/*
				  Direction is returned as shown below:
				      0
				    7   1
				  6   8   2
				    5   3
				      4
				  For the previous direction return 8.
			 */
				dir = 8;
				if ((in & 0x08) == 0)		/* up */
				{
					if ((in & 0x01) == 0)	/* right */
						dir = 1;
					else if ((in & 0x02) == 0) /* left */
						dir = 7;
					else
						dir = 0;
				}
				else if ((in & 0x04) == 0)	/* down */
				{
					if ((in & 0x01) == 0)	/* right */
						dir = 3;
					else if ((in & 0x02) == 0) /* left */
						dir = 5;
					else
						dir = 4;
				}
				else if ((in & 0x01) == 0)	/* right */
					dir = 2;
				else if ((in & 0x02) == 0) /* left */
					dir = 6;

				/* check fire */
				dir |= 0x10;
				if ((in & 0x10) == 0)
				{
					if (!fire1)
						dir &= ~0x10;	/* blaster */
					else
						fire1 = 1;
				}
				else
				{
					fire1 = 0;
				}

				/* check fire2 */
				dir |= 0xe0;
				if ((in & 0x20) == 0)
				{
					if (!fire2)
						dir &= ~0x20;		/*zapper */
					else
						fire2 = 1;
				}
				else
				{
					fire2 = 0;
				}

				if( stsw )
				{
					cpu_writemem(0x7000, 0x7e);
					cpu_writemem(0x7000 + 1,stsw);	/* PL1 */
					cpu_writemem(0x7000 + 2,stsw);	/* PL2 */
				}
				else
				{
					cpu_writemem(0x7000, 0x80);
					cpu_writemem(0x7000 + 1,dir);	/* PL1 */
					cpu_writemem(0x7000 + 2,dir);	/* PL2 */
				}
			}
			break;

		case 0x61:
			mode = 1;
			break;

		case 0x91:
			cpu_writemem(0x7000,0);
			cpu_writemem(0x7000 + 1,0);
			cpu_writemem(0x7000 + 2,0);
			mode = 0;
			break;
		case 0x74:		/* protect data read ? */
			if( protect == 0x80 ){
				cpu_writemem(0x7000 + 3,0x05);	/* 1st check */
			}else{
				cpu_writemem(0x7000 + 3,0x95);  /* 2nd check */
			}
			cpu_writemem(0x7000,0);
			cpu_writemem(0x7000 + 1,0);
			cpu_writemem(0x7000 + 2,0);
			mode = 0;
			break;

		default:
                        if (errorlog) fprintf(errorlog,"%04x: custom IO command %02x, HL = %04x DE = %04x BC = %04x\n",
		           cpu_getpc(),data,regs.HL2.D,regs.DE2.D,regs.BC2.D);
			break;
	}

	/* copy all of the data into the destination, just like the NMI */
	Z80_GetRegs(&regs);
	while (regs.BC2.D > 0)
	{
		cpu_writemem(regs.DE2.D,cpu_readmem(regs.HL2.D));
		++regs.DE2.W.l;
		++regs.HL2.W.l;
		--regs.BC2.W.l;
	}
	Z80_SetRegs(&regs);
	/* data write functions */
	switch (data)
	{
		case 0x64:		/* protect data calculate ? */
			protect = cpu_readmem(0x7000);
			break;
		case 0x68:
			/* case 1:30,40,00,02,df,10,10 */
			/* case 1:40,40,40,01,ff,20,20 */
			break;
		case 0xa1:
			/* write 6 byte 5,5,5,5,5,5 */
			/* write 8 byte 1,1,1,1,1,4,2,2 */
			mode = 1;
			break;
		default:
			break;
	}
}


int xevious_customio_r(int offset)
{
        /* return 00h -> free  custom_io */
        /* return e0h -> busy  cusrom_io */
	return 0x00;	/* everything is handled by customio_w() */
}



void xevious_halt_w(int offset,int data)
{
	cpu_halt(1,data&1);
	cpu_halt(2,data&1);
}



void xevious_interrupt_enable_1_w(int offset,int data)
{
	interrupt_enable_1 = (data&1);
}



int xevious_interrupt_1(void)
{
	if (interrupt_enable_1) return 0xff;
	else return Z80_IGNORE_INT;
}



void xevious_interrupt_enable_2_w(int offset,int data)
{
	interrupt_enable_2 = (data&1);
}



int xevious_interrupt_2(void)
{
	if (interrupt_enable_2) return 0xff;
	else return Z80_IGNORE_INT;
}



void xevious_interrupt_enable_3_w(int offset,int data)
{
	interrupt_enable_3 = (data&1);
}



int xevious_interrupt_3(void)
{
	if (interrupt_enable_3) return Z80_IGNORE_INT;
	else return Z80_NMI_INT;
}
