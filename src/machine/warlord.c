/***************************************************************************
Warlords Driver by Lee Taylor and John Clegg
  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "sndhrdw/pokyintf.h"

#define PADDLE_SPEED 1
#define PADDLE_MIN 0x1D
#define PADDLE_MAX 0xCB


static int paddle1 = 0x80 ;
static int paddle2 = 0x80 ;
static int paddle3 = 0x80 ;
static int paddle4 = 0x80 ;

int warlord_pots(int offset)
{
	int trak;

  if ( osd_key_pressed(OSD_KEY_Q) && paddle1 >= PADDLE_MIN + PADDLE_SPEED )
    paddle1 -= PADDLE_SPEED ;
  if ( osd_key_pressed(OSD_KEY_W) && paddle1 <= PADDLE_MAX - PADDLE_SPEED )
    paddle1 += PADDLE_SPEED ;

	trak = readtrakport(0);
	if (trak != NO_TRAK) {
		paddle1 += trak;
		if (paddle1 < PADDLE_MIN)
			paddle1 = PADDLE_MIN;
		else if (paddle1 > PADDLE_MAX)
			paddle1 = PADDLE_MAX;
		}

  if ( osd_key_pressed(OSD_KEY_I) && paddle2 >= PADDLE_MIN + PADDLE_SPEED )
    paddle2 -= PADDLE_SPEED ;
  if ( osd_key_pressed(OSD_KEY_O) && paddle2 <= PADDLE_MAX - PADDLE_SPEED )
    paddle2 += PADDLE_SPEED ;

  if ( osd_key_pressed(OSD_KEY_A) && paddle3 >= PADDLE_MIN + PADDLE_SPEED )
    paddle3 -= PADDLE_SPEED ;
   if ( osd_key_pressed(OSD_KEY_S) && paddle3 <= PADDLE_MAX - PADDLE_SPEED )
   paddle3 += PADDLE_SPEED ;

  if ( osd_key_pressed(OSD_KEY_J) && paddle4 >= PADDLE_MIN + PADDLE_SPEED )
    paddle4 -= PADDLE_SPEED ;
  if ( osd_key_pressed(OSD_KEY_K) && paddle4 <= PADDLE_MAX - PADDLE_SPEED )
    paddle4 += PADDLE_SPEED ;


  switch ( offset )
  {
    case 0x00 : return paddle1 ;
    case 0x01 : return paddle2 ;
    case 0x02 : return paddle3 ;
    case 0x03 : return paddle4 ;
    case 0x0A : return pokey1_r (offset);
    default   : if (errorlog)
                  fprintf(errorlog,"%02X\n" , offset ) ;
                return 0 ;
  }
}

int warlord_trakball_r (int data)
{
	#define MAXMOVE 32

	data = data >> 1;
	if(data > MAXMOVE)
		data = MAXMOVE;
	else if(data < -MAXMOVE)
		data = -MAXMOVE;
	return data;
}

