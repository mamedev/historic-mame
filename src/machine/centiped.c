/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


#define IN0_VBLANK (1<<6)



static int vblank;



int centiped_IN0_r(int offset)
{
	int res;
	int trak;

	res = readinputport(0);
	trak = readtrakport(0);

	if (vblank) res |= IN0_VBLANK;
	else vblank = 0;

	return(res|trak);
}

int centiped_trakball_x(int data) {
  static char x = 0;
  static int res = 0;

  if(data > 7) {
    data = 7;
  }

  if(data < -7) {
    data = -7;
  }

  x -= (char)data;

  if(x<0x00) {
    x += 0x10;
  }

  if(x>0x10) {
    x -= 0x10;
  }

  if(data < 0) {
    res = x;
  }

  if(data > 0) {
    res = 0x80|x;
  }

  return(res);
}

int centiped_trakball_y(int data) {
  static char y = 0;
  static int res = 0;

  data = -data;

  if(data > 7) {
    data = 7;
  }

  if(data < -7) {
    data = -7;
  }

  y -= (char)data;

  if(y<0x00) {
    y += 0x10;
  }

  if(y>0x10) {
    y -= 0x10;
  }

  if(data < 0) {
    res = y;
  }

  if(data > 0) {
    res = 0x80|y;
  }

  return(res);
}


int centiped_init_machine(const char *gamename)
{
	/* patch the roms to pass the startup test */
	RAM[0x38ae] = 0xea;
	RAM[0x38af] = 0xea;

	return 0;
}



int centiped_rand_r(int offset)
{
	return rand();
}



int centiped_interrupt(void)
{
	vblank = 1;

	return interrupt();
}
