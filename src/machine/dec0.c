/*******************************************************************************

Data East machine functions - Bryan McPhail, mish@tendril.force9.net


* dec0_controls_read - Controls for Bad Dudes, D.Ninja, Robocop, Hippodrome
* midres_controls_read - Controls for Midnight Resistance
* slyspy_controls_read - Controls for Sly Spy

* robocop_interrupt - Interrupts for Robocop, Hippodrome, Sly Spy
* dude_interrupt - Interrupts for Bad Dudes, Dragonninja, Mid. Res, H.B.

Plus protection enable for Hippodrome, protection enable for Robocop not
needed now unprotected rom set has appeared...

*******************************************************************************/

#include "driver.h"

/******************************************************************************/

static int hb_prot;

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

		case 8: /* Unknown, needed 0 for Bad Dudes, 8751 MC read in HB */

      /* Protection return in Heavy Barrel.. Hopefully this won't break
      	any of the other games (which don't use a microcontroller).

      int a=cpu_getpc();
			if (a!=0x1024 && a!=0x1345c && errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x (%04x)\n",cpu_getpc(),0x30c000+offset,hb_prot);

      */

			return hb_prot;
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

/* We can catch microcontroller commands and try to guess what the results
  should be... */
void hb_8751_write(int data)
{
  /* Code 0x500 is unknown but we can use it to initialise the variable */
	if (data==0x500 || data==0xb3b) hb_prot=0;

  /* Command 0x301 is written at end of each level */
  if (data==0x301) hb_prot++;

  /* Many commands unknown!  Attract mode still doesn't work.. */
//  if (errorlog && data!=0x600) fprintf(errorlog,"CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",cpu_getpc(),data,0x30c010+offset);
 if (errorlog) fprintf(errorlog,"warning - write %02x to 8571\n",data);

}

/******************************************************************************/

