/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"

int defender_bank;
int bank_address;

void williams_sh_w(int offset,int data);
void Williams_Palette_w(int offset,int data);

int  video_counter;
int  Index;
int  Stargate_IntTable[] = {0,0xFF,0x00,0xff};

/*
 *  int IntTable[] = {0,0x40,0xC0,0xff};
 *  int IntTable[] = {0,0x24,0x48,0x6c,0x90,0xB4,0xD8,0xff};
 *  int IntTable[] = {0,0x55,0xAA,0xff};
 *
 *  MUST be in this order to have the Back Zone not in the playfield
 *  in Joust.
 */

int  IntTable[] = {0xAA,0xff,0,0x55};


/*
 *  int IntTable[] = {0x00,0x00,0x00,0x00};
 *  int IntTable[] = {0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xff};
 *
 *  Here is some test that I have done, This is for the value of 0xCB00
 *  It affect the color cycling and/or the speed of the game
 *  0xCB00 is the value of the video counter (6 hi bits)
 *  IRQ is supposed to be at 4 ms interval.
 *  I put IRQ 4 times by frame so we are supposed to have near 4 ms
 *
 *  So, the value of CB00 normally pass all the values from 0 to 0xFC each
 *  frame.
 *  After trying many combination, I found that Stargate have to have
 *  more time than the others games between IRQs, but will work if he see
 *  only 00 and FF at $CB00. So I call IRQ just 2 times by frame.
 *  If I do like the others, IRQ 4 time by frames, I need to boost the
 *  CPU speed, but even pass 2 mhz, the game will freeze if he dont have
 *  time to execute all he is supposed to before the next IRQ.
 *  Same thing with Defender. This thing was observed even on the arcade
 *  board. In defender, on the first wave try to catch more than 7 man,
 *  stop your ship, press DOWN to drop the mens. You will see the
 *  game slowdown. If you had the 10 men, it will freeze for a long
 *  period of time. On the emulator, it crash if there is not enough
 *  instructions executed before the video counter reached a certain
 *  value.
 *  But Robotron will not work if only 0 and FF pass at CB00, he want
 *  more value.With only 0 and FF the color cycling do not work.
 *  Joust is like Robotron, but, the values has to match the real
 *  thing. What I mean is that normally the first IRQ is supposed
 *  to be after the first Quarter of a screen refresh, the second
 *  is supposed to appen at the middle of the screen...
 *  So, the value at CB00 has to follow that rule.
 *  In Joust, when a shape is erased, it is displayed at the new position
 *  just at the next IRQ. So we have a blackout area in the screen.
 *  If the value of CB00 are not synchronized with our computer, this
 *  blackout zone can be in the middle of the screen.
 *  Thats why the values in the table are 0xAA,0xff,0,0x55 and not
 *  0,0x55,0xAA,0xff. It seem to work fine.
 */


int Williams_Interrupt(void)
{
  video_counter = IntTable[Index&0x03];
  Index++;

  return INT_IRQ;
}


int Stargate_Interrupt(void)
{
  video_counter = Stargate_IntTable[Index&0x03];
  Index++;

  return INT_IRQ;
}

int Defender_Interrupt(void)
{
  video_counter = Stargate_IntTable[Index&0x03];
  Index++;

  /*
   *  IRQ only if enabled (0x3C will it work all the time? or should
   *  I check for a bit?)
   *  I think I should check for 111 in bits 5-3 but it work anyway
   */

  if (RAM[0x10000+0xc07] == 0x3C)
    return INT_IRQ;

  return INT_NONE;
}





int video_counter_r(int offset)
{
/*
	if (errorlog)
	  fprintf(errorlog,"### Get Video counter %02X at %04X\n",video_counter,m6809_GetPC());
*/
        return video_counter;
}



/**************
 *
 */
int williams_init_machine(const char *gamename)
{
  if(strcmp(gamename,"blaster") == 0){
/*So we do not have to copy the roms in RAM[] when bank switching
   On my system its much faster.*/
    m6809_Flags = M6809_FAST_NONE;
    return 0;
  }
  if(strcmp(gamename,"defender") == 0){
/*So we do not have to copy the roms in RAM[] when bank switching*/
    m6809_Flags = M6809_FAST_NONE;
    return 0;
  }

/*The cpu will fetch its instructions directly in RAM[]*/
  m6809_Flags = M6809_FAST_OP;
  return 0;

}




/*
 *  For Joust
 */
int input_port_0_1(int offset)
{
        if((RAM[0xc807] & 0x1C) == 0x1C)
           return input_port_1_r(0);
        else
           return input_port_0_r(0);
}

/*
 *  For Splat
 */
int input_port_2_3(int offset)
{
	if((RAM[0xc807] & 0x1C) == 0x1c)
	   return input_port_3_r(0);
	else
	   return input_port_2_r(0);
}

/*
 *  For Blaster
 */
int blaster_input_port_0(int offset)
{
        int i;
        int keys;

        keys = input_port_0_r(0);

        if(keys & 0x04)
          i = 0x00;
        else if(keys & 0x08)
          i = 0x80;
        else
          i = 0x40;

        if(keys&0x02)
          i += 0x00;
        else if(keys&0x01)
          i += 0x08;
        else
          i += 0x04;

        return i;
}


/*
 *   Defender Read at C000-CFFF
 */

int defender_bank_r(int offset)
{
	if(defender_bank == 0){         /* If bank = 0 then we are in the I/O */
	  if(offset == 0xc00)           /* Buttons IN 0  */
 	    return input_port_0_r(0);
	  if(offset == 0xc04)           /* Buttons IN 1  */
	    return input_port_1_r(0);
	  if(offset == 0xc06)           /* Buttons IN 2  */
	    return input_port_2_r(0);
	  if(offset == 0x800)           /* video counter */
	    return video_counter;
/*  Log
 *    if (errorlog)
 *      fprintf(errorlog,"-- Read %04X at %04X\n",0xC000+offset,m6809_GetPC());
 *    else = RAM
 */
          return RAM[0x10000+offset];
	}

        /* If not bank 0 then read RAM[] */

        return RAM[bank_address+offset];
}


/*
 *  Defender Write at C000-CFFF
 */

void defender_bank_w(int offset,int data)
{
        if (defender_bank == 0) {
	    RAM[0x10000+offset] = data;
        /* WatchDog */
            if (offset == 0x03FC)
              return;
        /* Palette  */
  	    if (offset < 0x10)
	      Williams_Palette_w(offset,data);
        /* Sound    */
            if (offset == 0x0c02)
              williams_sh_w(offset,data);
        }
}


/*
 *  Defender Select a bank
 *  There is just data in bank 0
 */

void defender_bank_select_w(int offset,int data)
{
	if (data == 7) data = 4;      /*  more convenient for us  */
		if (defender_bank == data)
			return;

	defender_bank = data;
        bank_address = data*0x1000 + 0x10000; /*Address of the ROM */
}

/*
 *  Blaster bank select
 */

void blaster_bank_select_w(int offset,int data)
{
	if (defender_bank == data)
		return;
        bank_address = data*0x4000 + 0x10000; /*Address of the ROM */
	defender_bank = data;   /* Banks are 0x4000 byte long from 0x10000 in RAM */
}
