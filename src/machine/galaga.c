/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


unsigned char *galaga_sharedram;
static unsigned char interrupt_enable_1,interrupt_enable_2,interrupt_enable_3;
static int do_nmi;
unsigned char galaga_hiscoreloaded,testdone;


int galaga_reset_r(int offset)
{
        galaga_hiscoreloaded = 0;
        testdone = 0;

	return RAM[offset];
}


int galaga_hiscore_print_r(int offset)
{
        if ((cpu_getpc() == 0x031e || cpu_getpc() == 0xe1) && galaga_hiscoreloaded) {
          if (offset == 4)
            RAM[0x83f2] = RAM[0x8a25];  /* Adjust the 6th digit */

          return RAM[0x8a20+offset];    /* return HISCORE */
        }
        else
          return RAM[0x2b9+offset];     /* bypass ROM test */
}

int galaga_sharedram_r(int offset)
{
	return galaga_sharedram[offset];
}



void galaga_sharedram_w(int offset,int data)
{
	galaga_sharedram[offset] = data;
        if (offset == 0x1ab9 && Machine->samples) {
          if (data && testdone && Machine->samples->sample[0]) {
                osd_play_sample(7,Machine->samples->sample[0]->data,
                        Machine->samples->sample[0]->length,
                        Machine->samples->sample[0]->smpfreq,
                        Machine->samples->sample[0]->volume,0);
          }
        }
}



int galaga_dsw_r(int offset)
{
	int bit0,bit1;


	bit0 = (input_port_0_r(0) >> offset) & 1;
	bit1 = (input_port_1_r(0) >> offset) & 1;

	return bit0 | (bit1 << 1);
}



/***************************************************************************

 Emulate the custom IO chip.

 In the real Galaga machine, the chip would cause an NMI on CPU #1 to ask
 for data to be transferred. We don't bother causing the NMI, we just look
 into the CPU register to see where the data has to be read/written to, and
 emulate the behaviour of the NMI interrupt.

***************************************************************************/
extern void galaga_customio_w(int offset,int data)
{
	static int mode,credits;
	Z80_Regs regs;


	Z80_GetRegs(&regs);

	switch (data)
	{
		case 0x10:	/* nop */
			break;

		case 0x71:
			{
				static int coin,start1,start2,fire;
				int in;


				/* check if the user inserted a coin */
				if (osd_key_pressed(OSD_KEY_3))
				{
					if (coin == 0 && credits < 99) credits++;
					coin = 1;
				}
				else coin = 0;

				/* check for 1 player start button */
				if (osd_key_pressed(OSD_KEY_1))
				{
					if (start1 == 0 && credits >= 1) credits--;
					start1 = 1;
				}
				else start1 = 0;

				/* check for 2 players start button */
				if (osd_key_pressed(OSD_KEY_2))
				{
					if (start2 == 0 && credits >= 2) credits -= 2;
					start2 = 1;
				}
				else start2 = 0;

				in = readinputport(2);
				/* check fire */
				if ((in & 0x10) == 0)
				{
					if (fire) in |= 0x10;
					else fire = 1;
				}
				else fire = 0;

				if (mode)	/* switch mode */
/* TODO: investigate what each bit does. bit 7 is the service switch */
					cpu_writemem(regs.DE2.D,0x80);
				else	/* credits mode: return number of credits in BCD format */
					cpu_writemem(regs.DE2.D,(credits / 10) * 16 + credits % 10);

				cpu_writemem(regs.DE2.D + 1,in);
				cpu_writemem(regs.DE2.D + 2,0xff);
			}
			break;

		case 0xb1:	/* status? */
                        testdone = 1;
			credits = 0;	/* this is a good time to reset the credits counter */
			cpu_writemem(regs.DE2.D,0);
			cpu_writemem(regs.DE2.D + 1,0);
			cpu_writemem(regs.DE2.D + 2,0);
			break;

#if 0
                case 0xa8:
                        testdone = 0;
                        if (Machine->samples->sample[0])
                         osd_play_sample(7,Machine->samples->sample[0]->data,
                                      Machine->samples->sample[0]->length,
                                      Machine->samples->sample[0]->smpfreq,
                                      Machine->samples->sample[0]->volume,0);
                        break;
#endif
		case 0xa1:	/* go into switch mode */
			mode = 1;
			break;

		case 0xe1:	/* go into credit mode */
			mode = 0;
			break;

		case 0x61:	/* generate an NMI to recover from DI/HALT */
			do_nmi = 1;
			break;

		default:
if (errorlog) fprintf(errorlog,"%04x: warning: unknown custom IO command %02x\n",cpu_getpc(),data);
			break;
	}
}



extern int galaga_customio_r(int offset)
{
	return 0x10;	/* everything is handled by customio_w() */
}



void galaga_halt_w(int offset,int data)
{
	cpu_halt(1,data);
	cpu_halt(2,data);
}



void galaga_interrupt_enable_1_w(int offset,int data)
{
	interrupt_enable_1 = data;
}



int galaga_interrupt_1(void)
{
	if (do_nmi)
	{
		do_nmi = 0;
		return Z80_NMI_INT;
	}

	if (interrupt_enable_1) return 0xff;
	else return Z80_IGNORE_INT;
}



void galaga_interrupt_enable_2_w(int offset,int data)
{
	interrupt_enable_2 = data;
}



int galaga_interrupt_2(void)
{
	if (interrupt_enable_2) return 0xff;
	else return Z80_IGNORE_INT;
}



void galaga_interrupt_enable_3_w(int offset,int data)
{
	interrupt_enable_3 = data;
}



int galaga_interrupt_3(void)
{
	if (interrupt_enable_3) return Z80_IGNORE_INT;
	else return Z80_NMI_INT;
}
