/* 8080bw.c *********************************
 updated: 1997-04-09 08:46 TT
 updated  20-3-1998 LT Added colour changes on base explosion
 *
 * Author      : Tormod Tjaberg
 * Created     : 1997-04-09
 * Description : Sound routines for the 'invaders' games
 *
 * Note:
 * The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 * excellent space invader emulator and converted to signed samples so
 * they would work under SEAL. The port info was also gleaned from
 * his emulator. These sounds should also work on all the invader games.
 *
 * The sounds are generated using output port 3 and 5
 *
 * Port 4:
 * bit 0=UFO  (repeats)       0.raw
 * bit 1=Shot                 1.raw
 * bit 2=Base hit             2.raw
 * bit 3=Invader hit          3.raw
 *
 * Port 5:
 * bit 0=Fleet movement 1     4.raw
 * bit 1=Fleet movement 2     5.raw
 * bit 2=Fleet movement 3     6.raw
 * bit 3=Fleet movement 4     7.raw
 * bit 4=UFO 2                8.raw
 */

#include "driver.h"

void invaders_vh_flipscreen(int data);

/*
   Note: For invad2ct, the Player 1 sounds are the same as for the
         original and deluxe versions.  Player 2 sounds are all
         different, and are triggered by writes to port 1 and port 7.

*/

void invad2ct_sh_port1_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (7, 10, 1);		/* Saucer Sound - Player 2 */

	if (~data & 0x01 && Sound & 0x01)
		sample_stop (7);

	if (data & 0x02 && ~Sound & 0x02)
	{
		sample_start (8, 11, 0);		/* Missle Sound - Player 2 */
	}

	if (data & 0x04 && ~Sound & 0x04)
		sample_start (9, 12, 0);		/* Explosion - Player 2 */

	if (data & 0x08 && ~Sound & 0x08)
		sample_start (10, 13, 0);		/* Invader Hit - Player 2 */

	if (data & 0x10 && ~Sound & 0x10)
		sample_start (4, 9, 0);		    /* Bonus Missle Base - Player 2 */

	Sound = data;
}

void invaders_sh_port3_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (0, 0, 1);			/* Saucer Sound */

	if (~data & 0x01 && Sound & 0x01)
		sample_stop (0);

	if (data & 0x02 && ~Sound & 0x02)
		sample_start (1, 1, 0);			/* Missle Sound */

	if (data & 0x04 && ~Sound & 0x04)
		sample_start (2, 2, 0);			/* Explosion */

	if (data & 0x08 && ~Sound & 0x08)
		sample_start (3, 3, 0);			/* Invader Hit */

	if (data & 0x10 && ~Sound & 0x10)
		sample_start (4, 9, 0);			/* Bonus Missle Base */

	Sound = data;
}

/* LT 20-3-1998 */
void invadpt2_sh_port3_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (0, 0, 1);

	if (~data & 0x01 && Sound & 0x01)
		sample_stop (0);

	if (data & 0x02 && ~Sound & 0x02)
		sample_start (1, 1, 0);

	if (data & 0x04 && ~Sound & 0x04){
            sample_start (2, 2, 0);
    /* turn all colours red here */
			palette_change_color(1,0xff,0x00,0x00);
			palette_change_color(2,0xff,0x00,0x00);
			palette_change_color(3,0xff,0x00,0x00);
			palette_change_color(4,0xff,0x00,0x00);
			palette_change_color(5,0xff,0x00,0x00);
			palette_change_color(6,0xff,0x00,0x00);
        }

	if (~data & 0x04 && Sound & 0x04){
            sample_stop (2);
            palette_change_color(1,0xff,0x20,0x20);
            palette_change_color(2,0x20,0xff,0x20);
            palette_change_color(3,0xff,0xff,0x20);
            palette_change_color(4,0xff,0xff,0xff);
            palette_change_color(5,0x20,0xff,0xff);
            palette_change_color(6,0xff,0x20,0xff);
        }
	if (data & 0x08 && ~Sound & 0x08)
		sample_start (3, 3, 0);

	Sound = data;
}

/* LT 20-3-1998 */
void invaders_sh_port4_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (0, 0, 1);

	if (~data & 0x01 && Sound & 0x01)
		sample_stop (0);

	if (data & 0x02 && ~Sound & 0x02)
		sample_start (1, 1, 0);

	if (data & 0x04 && ~Sound & 0x04){
            sample_start (2, 2, 0);
    /* turn all colours red here */
            palette_change_color(1,0xff,0x00,0x00);
            palette_change_color(2,0xff,0x00,0x00);
            palette_change_color(3,0xff,0x00,0x00);
            palette_change_color(4,0xff,0x00,0x00);
            palette_change_color(5,0xff,0x00,0x00);
            palette_change_color(6,0xff,0x00,0x00);
            palette_change_color(7,0xff,0x00,0x00);
        }

	if (~data & 0x04 && Sound & 0x04){
            sample_stop (2);
    /* restore colours here */
            palette_change_color(1,0x20,0x20,0xff);
            palette_change_color(2,0x20,0xff,0x20);
            palette_change_color(3,0x20,0xff,0xff);
            palette_change_color(4,0xff,0x20,0x20);
            palette_change_color(5,0xff,0x20,0xff);
            palette_change_color(6,0xff,0xff,0x20);
            palette_change_color(7,0xff,0xff,0xff);
        }
	if (data & 0x08 && ~Sound & 0x08)
		sample_start (3, 3, 0);

	Sound = data;
}

void invaders_sh_port5_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (5, 4, 0);			/* Fleet 1 */

	if (data & 0x02 && ~Sound & 0x02)
		sample_start (5, 5, 0);			/* Fleet 2 */

	if (data & 0x04 && ~Sound & 0x04)
		sample_start (5, 6, 0);			/* Fleet 3 */

	if (data & 0x08 && ~Sound & 0x08)
		sample_start (5, 7, 0);			/* Fleet 4 */

	if (data & 0x10 && ~Sound & 0x10)
		sample_start (6, 8, 0);			/* Saucer Hit */

	invaders_vh_flipscreen(data & 0x20);

	Sound = data;
}

void invad2ct_sh_port7_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (11, 14, 0);		/* Fleet 1 - Player 2 */

	if (data & 0x02 && ~Sound & 0x02)
		sample_start (11, 15, 0);		/* Fleet 2 - Player 2 */

	if (data & 0x04 && ~Sound & 0x04)
		sample_start (11, 16, 0);		/* Fleet 3 - Player 2 */

	if (data & 0x08 && ~Sound & 0x08)
		sample_start (11, 17, 0);		/* Fleet 4 - Player 2 */

	if (data & 0x10 && ~Sound & 0x10)
		sample_start (12, 18, 0);		/* Saucer Hit - Player 2 */

	invaders_vh_flipscreen(data & 0x20);

	Sound = data;
}


/* HC 4/14/98 NOTE: *I* THINK there are sounds missing...
i dont know for sure... but that is my guess....... */

void boothill_sh_port3_w(int offset, int data)
{
	switch (data)
	{
		case 0x0c:
			sample_start (0, 0, 0);
			break;

		case 0x18:
		case 0x28:
			sample_start (1, 2, 0);
			break;

		case 0x48:
		case 0x88:
			sample_start (2, 3, 0);
			break;
	}
}

/* HC 4/14/98 */
void boothill_sh_port5_w(int offset, int data)
{
	switch (data)
	{
		case 0x3b:
			sample_start (2, 1, 0);
			break;
	}
}

/*******************************************************/
/*                                                     */
/* Taito "Balloon Bomber"                              */
/*                                                     */
/*******************************************************/

/* This only does the colour swap for the explosion */
/* We do not have correct samples so sound not done */

void ballbomb_sh_port3_w(int offset, int data)
{
	static unsigned char colorswap = 0;

	if (data & 0x04 && ~colorswap & 0x04)
	{
	    /* turn all colours red here */

        palette_change_color(1,0xff,0x00,0x00);
        palette_change_color(2,0xff,0x00,0x00);
        palette_change_color(3,0xff,0x00,0x00);
        palette_change_color(4,0xff,0x00,0x00);
        palette_change_color(5,0xff,0x00,0x00);
        palette_change_color(6,0xff,0x00,0x00);
        palette_change_color(7,0xff,0x00,0x00);
    }

	if (~data & 0x04 && colorswap & 0x04)
	{
    	/* restore colours here */

        palette_change_color(1,0x20,0xff,0xff);
        palette_change_color(2,0xff,0x20,0xff);
        palette_change_color(3,0x20,0x20,0xff);
        palette_change_color(4,0xff,0xff,0x20);
        palette_change_color(5,0x20,0xff,0x20);
        palette_change_color(6,0xff,0x20,0x20);
        palette_change_color(7,0xff,0xff,0xff);
    }

	colorswap = data;
}


