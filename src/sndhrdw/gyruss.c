#include "driver.h"
#include "I8039/I8039.h"


/*#define USE_SAMPLES*/


static unsigned char voltable[255];

static unsigned char soundcommand = 0;



void gyruss_i8039_irq_w(int offset,int data)
{
	cpu_cause_interrupt(2,I8039_EXT_INT);
}

void gyruss_i8039_command_w(int offset,int data)
{
	soundcommand = data;

#ifdef USE_SAMPLES
	if (data) sample_start(0,data-1,0);
	else sample_stop(0);
#endif
}

int gyruss_i8039_command_r(int offset)
{
	/* kludge: the code actually does
	00000013: ba 00       	mov r2,#0
	00000015: 05          	en  i
	00000016: fa          	mov a,r2
	and expects r2 to retrieve the command from an external latch. Since the
	8039 emulator doesn't allow that, we kick in later, when the code does
	00000062: 53 0f       	anl a,#$0f
	00000064: 03 32       	add a,#$32
	00000066: b3          	jmpp @a
	since r2 is always 0, it always fetches the first byte of the jump table,
	so we wedge in there and return the correct jump address */

	return ROM[0x32+(soundcommand&0x0f)];
}



void gyruss_digital_out(int offset,int data)
{
#ifndef USE_SAMPLES
	DAC_data_w(0,voltable[data]);
#endif
}



/* The timer clock which feeds the lower 4 bits of    */
/* AY-3-8910 port A is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 10240, formed by a standard divide by 1024,        */
/* followed by a divide by 10 using a 4 bit           */
/* bi-quinary count sequence. (See LS90 data sheet    */
/* for an example).                                   */
/* Bits 1-3 come directly from the upper three bits   */
/* of the bi-quinary counter. Bit 0 comes from the    */
/* output of the divide by 1024.                      */

static int gyruss_timer[20] = {
0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

int gyruss_portA_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 10240;

	last_totalcycles = current_totalcycles;

	return gyruss_timer[clock/512];
}



void gyruss_sh_irqtrigger_w(int offset,int data)
{
	/* writing to this register triggers IRQ on the sound CPU */
	cpu_cause_interrupt(1,0xff);
}




void gyruss_init_machine(void)
{
	int i,j;
	int weight[8],totweight;


	/* reproduce the resistor ladder

	   -- 200 -+
	          100
	   -- 200 -+
	          100
	   -- 200 -+
	          100
	   -- 200 -+
	          100
	   -- 200 -+
	          100
	   -- 200 -+
	          100
	   -- 200 -+
	          100
	   -- 200 -+-------- out
	*/

	totweight = 0;
	for (i = 0;i < 8;i++)
	{
		weight[i] = 252000 / (200 + (7-i) * 100);
		totweight += weight[i];
	}

	for (i = 0;i < 8;i++)
		weight[i] = (255 * weight[i] + totweight / 2) / totweight;

	for (i = 0;i < 256;i++)
	{
		voltable[i] = 0;

		for (j = 0;j < 8;j++)
		{
			if ((i >> j) & 1)
				voltable[i] += weight[j];
		}
	}
}
