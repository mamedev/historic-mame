/*************************************************************************

  Food Fight sound hardware

*************************************************************************/

#include <stdlib.h>

#include "pokyintf.h"


static struct POKEYinterface pokey_interface =
{
	3,	/* 3 chips */
	600000,	/* .6 Mhz */
	255,
	USE_CLIP,
	/* The 8 pot handlers */
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0 }
};


/*
 *		Actually called by the video system to initialize memory regions.
 */

int foodf_sh_start (void)
{
	return pokey_sh_start (&pokey_interface);
}


/*
 *		Sound I/O
 */

int foodf_pokey1_r (int offset) { return (offset & 1) ? pokey1_r (offset/2) : 0; }
int foodf_pokey2_r (int offset) { return (offset & 1) ? pokey2_r (offset/2) : 0; }
int foodf_pokey3_r (int offset) { return (offset & 1) ? pokey3_r (offset/2) : 0; }

void foodf_pokey1_w (int offset, int data) { if (offset & 1) pokey1_w (offset/2, data); }
void foodf_pokey2_w (int offset, int data) { if (offset & 1) pokey2_w (offset/2, data); }
void foodf_pokey3_w (int offset, int data) { if (offset & 1) pokey3_w (offset/2, data); }
