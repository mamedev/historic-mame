/*******************************************************************************

Data East machine functions - Bryan McPhail, mish@tendril.force9.net


* dec0_controls_read - Controls for Bad Dudes, D.Ninja, Robocop, Hippodrome
* midres_controls_read - Controls for Midnight Resistance
* slyspy_controls_read - Controls for Sly Spy

* robocop_interrupt - Interrupts for Robocop, Hippodrome, Sly Spy
* dude_interrupt - Interrupts for Bad Dudes, Dragonninja, Mid. Res, H.B.

Plus protection enable for Hippodrome, protection enable for Robocop not
needed now unprotected rom set has appeared...  Protection for Heavy Barrel
is patched out at rom loading stage in driver file.

*******************************************************************************/

#include "driver.h"

/******************************************************************************/

int dec0_controls_read(int offset)
{
	switch (offset)
	{
		case 0: /* Player 1 & 2 joystick & buttons */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2: /* Credits, start buttons in lsb, ??? in msb */
			return readinputport(2);

		case 4: /* Byte 4: Dipswitch bank 2, Byte 5: Dipswitch Bank 1 */
			return (readinputport(3) + (readinputport(4) << 8));

		case 8: /* Unknown, needed 0 for Bad Dudes */
			return 0;
	}

if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_getpc(),0x30c000+offset);

	return 0xffff;
}

/******************************************************************************/

int dec0_rotary_read(int offset)
{
	switch (offset)
	{
		case 0: /* Player 1 rotary */
			return ~(1 << (readinputport(5) * 12 / 256));

		case 8: /* Player 2 rotary */
			return ~(1 << (readinputport(6) * 12 / 256));

		default:
if (errorlog) fprintf(errorlog,"Unknown rotary read at 300000 %02x\n",offset);
	}

	return 0;
}

/******************************************************************************/

int midres_controls_read(int offset)
{
	switch (offset)
	{
		case 0: /* Player 1 Joystick + start, Player 2 Joystick + start */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2: /* Dipswitches */
			return (readinputport(3) + (readinputport(4) << 8));

		case 4: /* Player 1 rotary */
			return ~(1 << (readinputport(5) * 12 / 256));

		case 6: /* Player 2 rotary */
			return ~(1 << (readinputport(6) * 12 / 256));

		case 8: /* Credits, start buttons in lsb, ??? in msb */
			return readinputport(2);

		case 12:
			return 0;	/* ?? watchdog ?? */
	}

if (errorlog) fprintf(errorlog,"PC %06x unknown control read at %02x\n",cpu_getpc(),0x180000+offset);

	return 0xffff;
}

/******************************************************************************/

int slyspy_controls_read(int offset)
{
	switch (offset)
	{
		case 0: /* Dip Switches */
			return (readinputport(3) + (readinputport(4) << 8));

		case 2: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));

		case 4: /* Credits, ??? in msb */
			return readinputport(2);
	}

if (errorlog) fprintf(errorlog,"Unknown control read at 30c000 %d\n",offset);

	return 0xffff;
}

/******************************************************************************/

int robocop_interrupt(void)
{
 return 6;
}

int dude_interrupt(void)
{
	static int a=0;

  if (a) {a=0; return 6;}
  a=1; return 5;
}

/******************************************************************************/

int hippodrm_protection(int offset)
{
	if (errorlog) fprintf(errorlog,"PC %06x - Read %06x\n",cpu_getpc(),0x180000+offset);

	switch (offset) {
    case 8: return 4;
    case 0x14: return 0; /* Unknown.. */
  	case 0x1c: return 4;
  }
  return 0; /* Keep zero */
}

/******************************************************************************/

