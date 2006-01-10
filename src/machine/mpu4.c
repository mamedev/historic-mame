
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6809/m6809.h"
#include "machine/74148.h"

#define LOG(x) logerror x

static UINT8 m6840_irq_state;
static UINT8 m6850_irq_state;
static UINT8 scn2674_irq_state;
static void update_irq(void);

/*************************************
 *
 *  Interrupt system
 *
 *************************************/

/* the interrupt system consists of a 74148 priority encoder
   with the following interrupt priorites.  A lower number
   indicates a lower priority:

    7 - Game Card
    6 - Game Card
    5 - Game Card
    4 - Game Card
    3 - AVDC
    2 - ACIA
    1 - PTM
    0 - Unused (no such IRQ on 68k)
*/

static void update_mpu68_interrupts(void)
{
	int newstate = 0;

	/* all interrupts go through an LS148, which gives priority to the highest */
	if (m6840_irq_state)//1
		newstate = 1;
	if (m6850_irq_state)//2
		newstate = 2;
	if (scn2674_irq_state == 1)//3
		newstate = 3;

	m6840_irq_state = 0;
	m6850_irq_state = 0;
	scn2674_irq_state = 0;
	
	/* set the new state of the IRQ lines */
	if (newstate)
		cpunum_set_input_line(1, newstate, HOLD_LINE);
	else
		cpunum_set_input_line(1, 7, CLEAR_LINE);
}
