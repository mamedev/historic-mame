/*******************************************************************************

Data East machine functions - Bryan McPhail, mish@tendril.force9.net


* dec0_controls_read - Controls for Bad Dudes, D.Ninja, Robocop, Hippodrome
* midres_controls_read - Controls for Midnight Resistance
* slyspy_controls_read - Controls for Sly Spy

* robocop_interrupt - Interrupts for Robocop, Hippodrome, Sly Spy
* dude_interrupt - Interrupts for Bad Dudes, Dragonninja, Mid. Res, H.B.

Plus protection routines for Hippodrome & Heavy Barrel

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

    {  int a=cpu_getpc();
			if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x (%04x)\n",cpu_getpc(),0x30c000+offset,hb_prot);
    }
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

		case 4: /* Credits */
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
	switch (offset) {
    case 8: return 4;
    case 0x14: return 0; /* Unknown.. */
  	case 0x1c: return 4;
  }
	if (errorlog) fprintf(errorlog,"PC %06x - Read %06x\n",cpu_getpc(),0x180000+offset);
  return 0; /* Keep zero */
}

/******************************************************************************/

/* We can catch microcontroller commands and try to guess what the results
  should be... */
void hb_8751_write(int data)
{
  static int level,state;

  int title[]={  1, 2, 5, 6, 9,10,13,14,17,18,21,22,25,26,29,30,33,34,37,38,41,42,0,
  						   3, 4, 7, 8,11,12,15,16,19,20,23,24,27,28,31,32,35,36,39,40,43,44,0,
                45,46,49,50,53,54,57,58,61,62,65,66,69,70,73,74,77,78,81,82,85,86,0,
                51,52,55,56,59,60,63,64,67,68,71,72,75,76,79,80,83,84,0,
                85,86,89,90,93,94,97,98,101,102,105,106,109,110,113,114,117,118,121,122,125,126,0,
                87,88,91,92,95,96,99,100,103,104,107,108,111,112,115,116,119,120,123,124,127,128,0,
                129,130,133,134,137,138,141,142,145,146,149,150,153,154,157,158,161,162,165,166,169,170,173,174,0,
                131,132,135,136,139,140,143,144,147,148,151,152,155,156,159,160,163,164,167,168,171,172,175,176,0,
                177,178,178,179,180,181,182,183,190,190,194,0
                };

  /* Code 0x9000 is unknown but we can use it to initialise the variables */
	if (data==0x9000 || data==0xb3b || data==0x500) hb_prot=level=0;

  /* Command 0x301 is written at end of each level */
  if (data==0x301) {level++;hb_prot=level;}
  //if (data==0x301) {level=5;hb_prot=level;} /* Example: Go to level 5 at end of 1 */

  /* 0x200 selects level */
  if (data==0x200) hb_prot=level;

  /* Stack pointer */
  if (data==7) hb_prot=0xc000;

  /* Related to appearance & placement of special weapons */
  if (data>0x5ff && data<0x700) hb_prot=rand()%0xfff;

  /* All commands in range 4xx are related to title screen.. */
  if (data==0x4ff) state=0;
  if (data>0x3ff && data<0x4ff) {
		state++;
    hb_prot=title[state-1]+128+15+0x2000;

    /* We have to use a state as the microcontroller remembers previous commands */

    /* Mark line ends */
  	if (state==23 || state==46 || state==67 || state==88 || state==111
    		|| state==134 || state==159 || state==184 || state==187) hb_prot=0xfffe;

    /* Mark title end */
  	if (state==190) hb_prot=0xffff;

  }

// if (errorlog && data!=0x600) fprintf(errorlog,"CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",cpu_getpc(),data,0x30c010+offset);
// if (errorlog) fprintf(errorlog,"warning - write %02x to 8571 (state %d return %d) %04x\n",data,state,title[state-1],hb_prot);
}

/******************************************************************************/

