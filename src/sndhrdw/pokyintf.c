/***************************************************************************

	pokyintf.c

	Interface the routines in pokey.c to MAME

	The pseudo-random number algorithm is (C) Keith Gerdes.
	Please contact him if you wish to use this same algorithm in your programs.
	His e-mail is mailto:kgerdes@worldnet.att.net

***************************************************************************/

#include "driver.h"
#include "sndhrdw/pokyintf.h"

#define TARGET_EMULATION_RATE 44100	/* will be adapted to be a multiple of buffer_len */
static int buffer_len;
static int emulation_rate;

extern uint8 rng[MAXPOKEYS];
static uint8 pokey_random[MAXPOKEYS];     /* The random number for each pokey */

static struct POKEYinterface *intf;
static unsigned char *buffer;

#ifdef macintosh
#include "macpokey.c"
#endif

int pokey_sh_start (struct POKEYinterface *interface)
{

	int i;

	intf = interface;

	buffer_len = TARGET_EMULATION_RATE / Machine->drv->frames_per_second / intf->updates_per_frame;
	emulation_rate = buffer_len * Machine->drv->frames_per_second * intf->updates_per_frame;

	if ((buffer = malloc(buffer_len * intf->updates_per_frame)) == 0)
	{
		free(buffer);
		return 1;
	}
	memset(buffer,0x80,buffer_len * intf->updates_per_frame);

	Pokey_sound_init (intf->clock, emulation_rate, intf->num, intf->clip);

	for (i = 0; i < intf->num; i ++)
		/* Seed the values */
		pokey_random[i] = rand ();

	return 0;
}


void pokey_sh_stop (void)
{
}


/*****************************************************************************/
/* Module:  Read_pokey_regs()                                                */
/* Purpose: To return the values of the Pokey registers. Currently, only the */
/*          random number generator register is returned.                    */
/*                                                                           */
/* Author:  Keith Gerdes, Brad Oliver & Eric Smith                           */
/* Date:    August 8, 1997                                                   */
/*                                                                           */
/* Inputs:  addr - the address of the parameter to be changed                */
/*          chip - the pokey chip to read                                    */
/*                                                                           */
/* Outputs: Adjusts local globals, returns the register in question          */
/*                                                                           */
/*****************************************************************************/

int Read_pokey_regs (uint16 addr, uint8 chip)
{

    switch (addr & 0x0f)
    {
    	case POT0_C:
    		if (intf->pot0_r[chip]) return (*intf->pot0_r[chip])(addr);
    		break;
    	case POT1_C:
    		if (intf->pot1_r[chip]) return (*intf->pot1_r[chip])(addr);
    		break;
    	case POT2_C:
    		if (intf->pot2_r[chip]) return (*intf->pot2_r[chip])(addr);
    		break;
    	case POT3_C:
    		if (intf->pot3_r[chip]) return (*intf->pot3_r[chip])(addr);
    		break;
    	case POT4_C:
    		if (intf->pot4_r[chip]) return (*intf->pot4_r[chip])(addr);
    		break;
    	case POT5_C:
    		if (intf->pot5_r[chip]) return (*intf->pot5_r[chip])(addr);
    		break;
    	case POT6_C:
    		if (intf->pot6_r[chip]) return (*intf->pot6_r[chip])(addr);
    		break;
    	case POT7_C:
    		if (intf->pot7_r[chip]) return (*intf->pot7_r[chip])(addr);
    		break;
    	case ALLPOT_C:
    		if (intf->allpot_r[chip]) return (*intf->allpot_r[chip])(addr);
    		break;
		case RANDOM_C:
			/* If the random number generator is enabled, get a new random number */
			/* The pokey apparently shifts the high nibble down on consecutive reads */
			if (rng[chip]) {
				pokey_random[chip] = (pokey_random[chip]>>4) | (rand()&0xf0);
				}
			return pokey_random[chip];
			break;
		default:
#ifdef DEBUG
			if (errorlog) fprintf (errorlog, "Pokey #%d read from register %02x\n", chip, addr);
#endif
			return 0;
			break;
	}

	return 0;
}

int pokey1_r (int offset)
{
	return Read_pokey_regs (offset,0);
}

int pokey2_r (int offset)
{
	return Read_pokey_regs (offset,1);
}

int pokey3_r (int offset)
{
	return Read_pokey_regs (offset,2);
}

int pokey4_r (int offset)
{
	return Read_pokey_regs (offset,3);
}


void pokey1_w (int offset,int data)
{
	Update_pokey_sound (offset,data,0,16/intf->num);
}

void pokey2_w (int offset,int data)
{
	Update_pokey_sound (offset,data,1,16/intf->num);
}

void pokey3_w (int offset,int data)
{
	Update_pokey_sound (offset,data,2,16/intf->num);
}

void pokey4_w (int offset,int data)
{
	Update_pokey_sound (offset,data,3,16/intf->num);
}

static int updatecount;

void pokey_update(void)
{
	if (updatecount >= intf->updates_per_frame) return;

	Pokey_process (&buffer[updatecount * buffer_len],buffer_len);

	updatecount++;
}


void pokey_sh_update (void)
{

	if (play_sound == 0) return;

	if (intf->updates_per_frame == 1) pokey_update();

if (errorlog && updatecount != intf->updates_per_frame)
	fprintf(errorlog,"Error: pokey_update() has not been called %d times in a frame\n",intf->updates_per_frame);

	updatecount = 0;	/* must be zeroed here to keep in sync in case of a reset */

	osd_play_streamed_sample(0,buffer,buffer_len * intf->updates_per_frame,emulation_rate,intf->volume);

}
