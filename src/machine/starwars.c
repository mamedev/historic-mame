/***************************************************************************
machine\starwars.c

STARWARS MACHINE FILE

This file is Copyright 1997, Steve Baines.
Modified by Frank Palazzolo for sound support

Release 2.0 (6 August 1997)

See drivers\starwars.c for notes

***************************************************************************/

#include "driver.h"
#include "cpuintrf.h"
#include "swmathbx.h"
#include "vidhrdw/avgdvg.h"


/*   If 1 then log functions called */
#define MACHDEBUG 0
#define SNDDEBUG 0

/* control select values for ADC_R */
#define kPitch	0
#define kYaw	1
#define kThrust	2
static unsigned char control_num = kPitch;


/**********************************************************/
/**********************************************************/
/************** Read Handlers *****************************/

/*
int math_ram_r(int offset)
   {
   printf("MRAM_RD: (%x), %x\n",0x5000+offset, RAM[0x5000+offset]);
   return RAM[0x5000+offset];
   }
*/

/********************************************************/

/*
void math_ram_w(int offset, int data)
   {
   printf("MRAM_WR: (%x), %x\n",0x5000+offset, data);
   RAM[0x5000+offset]=data;
   }
*/

/********************************************************/

/* Read from ROM 0. Use bankaddress as base address
int banked_rom_r(int offset)
   {
   return RAM[bankaddress + offset];
   }*/

#if 0
/********************************************************/
int input_bank_0_r(int offset)
   {
   int x;
   x=input_port_0_r(0); /* Read memory mapped port 1 */
   x=x&0xdf; /* Clear out bit 5 (SPARE 1) */
   #if(MACHDEBUG==1)
   printf("(%x)input_bank_0_r   (returning %xh)\n", cpu_getpc(), x);
   #endif
   return x;
   }
#endif

/********************************************************/
int input_bank_1_r(int offset)
   {
   int x;
   x=input_port_1_r(0); // Read memory mapped port 2

#if 0
   x=x&0x34; /* Clear out bit 3 (SPARE 2), and 0 and 1 (UNUSED) */
             /* MATH_RUN (bit 7) set to 0 */
   x=x|(0x40);  /* Set bit 6 to 1 (VGHALT) */
#endif

/* Kludge to enable Starwars Mathbox Self-test                  */
/* The mathbox looks like it's running, from this address... :) */
   if (cpu_getpc() == 0xf978)
        x|=0x80;

   if (avgdvg_done())
	x|=0x40;
   else
	x&=~0x40;
   #if(MACHDEBUG==1)
   printf("(%x)input_bank_1_r   (returning %xh)\n", cpu_getpc(), x);
   #endif
   return x;
   }
/*********************************************************/
/********************************************************/
int control_r (int offset) {

	if (control_num == kPitch)
		return readinputport (4);
	else if (control_num == kYaw)
		return readinputport (5);
	/* default to unused thrust */
	else return 0;
	}

void control_w (int offset, int data) {

	control_num = offset;
	}

/************************************************************/
/************************************************************/
/************** Write Handlers ******************************/

/********************************************************/
void irqclr(int offset, int data)
   {
   #if(MACHDEBUG==1)
   printf("irqclr\n");
   #endif
   }
/**************************************************/
void led3(int offset, int data)
   {
   #if(MACHDEBUG==1)
   printf("led3 [%d]\n",((data&0x80)>>7) );
   #endif
   }
/**************************************************/
void led2(int offset, int data)
   {
   #if(MACHDEBUG==1)
   printf("led2 [%d]\n",((data&0x80)>>7) );
   #endif
   }
/**************************************************/

/* Switch bank of ROM 0 */
void mpage(int offset, int data) /* MSB toggles bank */
{
	if ((data & 0x80)==0)
		cpu_setbank (1, &RAM[0x6000])
	else
		cpu_setbank (1, &RAM[0x10000]);

   #if(MACHDEBUG==1)
   printf("mpage [Page %d]\n",((data&0x80)>>7) );
   #endif
}

/**************************************************/
void led1(int offset, int data)
   {
   #if(MACHDEBUG==1)
   printf("led1 [%d]\n",((data&0x80)>>7) );
   #endif
   }
/**************************************************/
void recall(int offset, int data)
   {
   #if(MACHDEBUG==1)
   printf("recall\n");
   #endif
   }
/********************************************************/
void nstore(int offset, int data)
   {
   #if(MACHDEBUG==1)
   printf("nstore\n");
   #endif
   }

int starwars_interrupt(void)
{
	if (cpu_getiloops() == 5)
		avgdvg_clr_busy();
	return interrupt();
}

