/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



int bankaddress;


int ccastles_trakball_x(int data) {
  return((unsigned char)data);
}

int ccastles_trakball_y(int data) {
  return(0xFF-(unsigned char)data);
}

int ccastles_trakball_r(int offset) {
  static unsigned char x,y;
  int trak_x, trak_y;

  switch(offset) {
  case 0x00:
    if(osd_key_pressed(OSD_KEY_DOWN) || osd_joy_pressed(OSD_JOY_DOWN)) {
      x-=2;
      y-=4;
    }
    if(osd_key_pressed(OSD_KEY_UP) || osd_joy_pressed(OSD_JOY_UP)) {
      x+=2;
      y+=4;
    }

    trak_y = input_trak_0_r(0);

    if(trak_y == NO_TRAK) {
      RAM[0x9500] = y;
      return(y);
    } else {
      RAM[0x9500] = trak_y;
      return(trak_y);
    }

    break;
  case 0x01:
    if(osd_key_pressed(OSD_KEY_LEFT) || osd_joy_pressed(OSD_JOY_LEFT)) {
      x-=4;
      y+=2;
    }
    if(osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_pressed(OSD_JOY_RIGHT)) {
      x+=4;
      y-=2;
    }

    trak_x = input_trak_1_r(0);

    if(trak_x == NO_TRAK) {
      RAM[0x9501] = x;
      return(x);
    } else {
      RAM[0x9501] = trak_x;
      return(trak_x);
    }

    break;
  }

  return(0);
}

int ccastles_rom_r(int offset) {
  return(RAM[bankaddress+offset]);
}


int ccastles_random_r(int offset) {
  return rand();
}


void ccastles_bankswitch_w(int offset, int data) {
  if(data) {
    bankaddress = 0x10000;
  } else {
    bankaddress = 0xA000;
  }
}
