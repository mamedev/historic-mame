/***************************************************************************
Warlords Driver by Lee Taylor and John Clegg
  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "sndhrdw/pokyintf.h"

#define PADDLE_SPEED 8
#define PADDLE_MIN 0x1D
#define PADDLE_MAX 0xCB


static int paddle2 = 0x80;
static int paddle3 = 0x80;
static int paddle4 = 0x80;

int warlord_pot2_r (int data) {

	int res = readinputport(5);
	
	if ((res & 0x01) && paddle2 >= PADDLE_MIN + PADDLE_SPEED)
		paddle2 -= PADDLE_SPEED ;
	if ((res & 0x02) && paddle2 <= PADDLE_MAX - PADDLE_SPEED)
		paddle2 += PADDLE_SPEED ;

	return paddle2;
	}
	
int warlord_pot3_r (int data) {

	int res = readinputport(5);
	
	if ((res & 0x04) && paddle3 >= PADDLE_MIN + PADDLE_SPEED)
		paddle3 -= PADDLE_SPEED ;
	if ((res & 0x08) && paddle3 <= PADDLE_MAX - PADDLE_SPEED)
		paddle3 += PADDLE_SPEED ;

	return paddle3;
	}
	
int warlord_pot4_r (int data) {

	int res = readinputport(5);
	
	if ((res & 0x10) && paddle4 >= PADDLE_MIN + PADDLE_SPEED)
    	paddle4 -= PADDLE_SPEED ;
  	if ((res & 0x20) && paddle4 <= PADDLE_MAX - PADDLE_SPEED)
    	paddle4 += PADDLE_SPEED ;

	return paddle4;
	}

