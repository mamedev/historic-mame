/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


#define MCU_DEBUG 0


int arkanoid_paddle_select;

static int fromz80, toz80;
static int z80write, m68705write;
static int stickybits;

unsigned char *arkanoid_stat;

FILE *thelog;

int arkanoid_Z80_mcu_r (int value)
{
#if MCU_DEBUG
if (!thelog) thelog = fopen ("ark.log", "w");
fprintf (thelog, "Read: D00C = %02X (PC=%04X)\n", toz80, cpu_getpc());
#endif

	/* return the last value the 68705 wrote, and mark that we've read it */
	m68705write = 0;
	return toz80;
}

void arkanoid_Z80_mcu_w (int offset, int value)
{
#if MCU_DEBUG
if (!thelog) thelog = fopen ("ark.log", "w");
fprintf (thelog, "Write: D018 = %02X (PC=%04X)\n", value, cpu_getpc());
#endif

	/* a write from the Z80 has occurred, mark it and remember the value */
	z80write = 1;
	fromz80 = value;

	/* give up a little bit of time to let the 68705 detect the write */
	cpu_yielduntil_trigger (700);
}

int arkanoid_68705_mcu_r (int offset)
{
#if MCU_DEBUG
if (!thelog) thelog = fopen ("ark.log", "w");
fprintf (thelog, "*** MCU Read: %02X (PC=%04X, $15=%04X)\n", fromz80, cpu_getpc(), (RAM[0x14] << 8) + RAM[0x13]);
#endif

	/* mark that the command has been seen */
	cpu_trigger (700);

	/* return the last value the Z80 wrote */
	z80write = 0;
	return fromz80;
}

void arkanoid_68705_mcu_w (int offset, int value)
{
#if MCU_DEBUG
if (!thelog) thelog = fopen ("ark.log", "w");
fprintf (thelog, "*** MCU Write: %02X (PC=%04X)\n", value, cpu_getpc());
#endif

	/* a write from the 68705 to the Z80; remember its value */
	m68705write = 1;
	toz80 = value;
}

int arkanoid_68705_stat_r (int offset)
{
	int result = (*arkanoid_stat | 0xf0) & 0xfc;

	/* bit 0 is high on a write strobe; clear it once we've detected it */
	if (z80write) result |= 0x01;

	/* bit 1 is high if the previous write has been read */
	if (!m68705write) result |= 0x02;

	return result;
}

void arkanoid_68705_stat_w (int offset, int data)
{
	*arkanoid_stat = data;

	/* the MCU will toggle bits low for just one instruction; keep track of them until the Z80 reads */
	stickybits &= data;
}

int arkanoid_68705_input_0_r (int offset)
{
	int result = input_port_0_r (offset);

	/* bit 0x40 comes from the sticky bit */
	result |= (stickybits & 0x04) << 4;

	/* bit 0x80 comes from a write latch */
	if (!m68705write) result |= 0x80;

	/* reset the sticky bits; they've been read */
	stickybits = *arkanoid_stat;

	return result;
}

int arkanoid_input_2_r (int offset)
{
	if (arkanoid_paddle_select)
	{
		return input_port_3_r(offset);
	}
	else
	{
		return input_port_2_r(offset);
	}
}

