/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6502.h"

#define IN0_VBLANK (1<<5)
#define IN0_NOT_VBLANK 0xDF

extern void ccastles_sh_update(void);

int bankaddress;
int ax;
int ay;
int icount;
unsigned char xcoor;
unsigned char ycoor;

int ccastles_init_machine(const char *gamename) {
  RAM[0xED7F] = 0xEA;
  RAM[0xED80] = 0xEA;
  RAM[0xE010] = 0xEA;
  RAM[0xE011] = 0xEA;

  icount = 0;

  return(0);
}

int ccastles_interrupt(void) {
  return(INT_IRQ);
}

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

int ccastles_IN0_r(int offset) {
  int res;
  static int count = 0;

  res = readinputport(0);

  if(cpu_geticount() > (Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second) / 12) {
    if(count==3) {
      res |= IN0_VBLANK;
      count = 0;
    } else {
      res &= IN0_NOT_VBLANK;
      count++;
    }
  } else {
    res &= IN0_NOT_VBLANK;
  }

  return(res);
}

int ccastles_random_r(int offset) {
  return rand();
}

int ccastles_bitmode_r(int offset) {
  int addr;

  addr = (ycoor<<7) | (xcoor>>1);

  if(!ax) {
    if(!RAM[0x9F02]) {
      xcoor++;
    } else {
      xcoor--;
    }
  }

  if(!ay) {
    if(!RAM[0x9F03]) {
      ycoor++;
    } else {
      ycoor--;
    }
  }

  if(xcoor & 0x01) {
    return((RAM[addr] & 0x0F) << 4);
  } else {
    return(RAM[addr] & 0xF0);
  }
}

void ccastles_xy_w(int offset, int data) {
  RAM[offset] = data;

  if(offset == 0x0000) {
    xcoor = data;
  } else {
    ycoor = data;
  }
}

void ccastles_axy_w(int offset, int data) {
  RAM[0x9F00+offset] = data;

  if(offset == 0x0000) {
    if(data) {
      ax = data;
    } else {
      ax = 0x00;
    }
  } else {
    if(data) {
      ay = data;
    } else {
      ay = 0x00;
    }
  }
}

void ccastles_bitmode_w(int offset, int data) {
  int addr;
  int mode;

  RAM[0x0002] = data;
  if(xcoor & 0x01) {
    mode = (data >> 4) & 0x0F;
  } else {
    mode = (data & 0xF0);
  }

  addr = (ycoor<<7) | (xcoor>>1);

  if((addr>0x0BFF) && (addr<0x8000)) {
    if(xcoor & 0x01) {
      RAM[addr] = (RAM[addr] & 0xF0) | mode;
    } else {
      RAM[addr] = (RAM[addr] & 0x0F) | mode;
    }
  }

  if(!ax) {
    if(!RAM[0x9F02]) {
      xcoor++;
    } else {
      xcoor--;
    }
  }

  if(!ay) {
    if(!RAM[0x9F03]) {
      ycoor++;
    } else {
      ycoor--;
    }
  }
}

void ccastles_bankswitch_w(int offset, int data) {
  if(data) {
    bankaddress = 0x10000;
  } else {
    bankaddress = 0xA000;
  }
}

extern void ccastles_colorram_w(int offset,int data) {
  int r,g,b;

  r = (data & 0xC0) >> 6;
  b = (data & 0x38) >> 3;
  g = (data & 0x07);

  /*  if(errorlog) {
    if(offset>31) {
      fprintf(errorlog,"COLOR: offset=%2.2x data=%2.2x r=%d g=%d b=%d\n",offset,data,(4+r)*36,g*36,b*36);
    } else {
      fprintf(errorlog,"COLOR: offset=%2.2x data=%2.2x r=%d g=%d b=%d\n",offset,data,r*36,g*36,b*36);
    }
  } */
}
