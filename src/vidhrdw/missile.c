/***************************************************************************

  vmissile.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "generic.h"

unsigned char *missile_videoram;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int missile_vh_start(void)
{

/* 	force video ram to be $0000-$FFFF even though only $1900-$FFFF is used */
	if ((missile_videoram = malloc(256 * 256)) == 0)
		return 1;

	memset(missile_videoram, 0, 256 * 256);

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void missile_vh_stop(void)
{
	free(missile_videoram);
}




/********************************************************************************************/
int missile_video_r(int address)
{
	return(missile_videoram[address] & 0xE0);
}



/********************************************************************************************/
void
missile_video_w(int address,int data)
{
	int wbyte, wbit;
	extern unsigned char *RAM;


  if(address < 0xF800)
		missile_videoram[address] = data;
	else{
		missile_videoram[address] = (missile_videoram[address] & 0x20) | data;
		wbyte = ((address - 0xF800) >> 2) & 0xFFFE;
		wbit = (address - 0xF800) % 8;
		if(data & 0x20)
			RAM[0x401 + wbyte] |= (1 << wbit);
		else
			RAM[0x401 + wbyte] &= ((1 << wbit) ^ 0xFF);
	}
}






/********************************************************************************************/
void
missile_video_mult_w(int address, int data)
{
	data = (data & 0x80) + ((data & 8) << 3);
	address = address << 2;
	if(address >= 0xF800)
		data |= 0x20;
  missile_videoram[address]     = data;
  missile_videoram[address + 1] = data;
  missile_videoram[address + 2] = data;
  missile_videoram[address + 3] = data;
}



/********************************************************************************************/
void
missile_video_3rd_bit_w(int address, int data)
{
	int i;
	extern unsigned char *RAM;

	RAM[address] = data;

	address = ((address - 0x401) << 2) + 0xF800;
	for(i=0;i<8;i++){
		if(data & (1 << i))
			missile_videoram[address + i] |= 0x20;
		else
			missile_videoram[address + i] &= 0xC0;
	}
}




/********************************************************************************************/
void missile_vh_update(struct osd_bitmap *bitmap)
{
	int x, y, tmp;
	int address;
	int bottom;
	/* TODO: get rid of this */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* RG - 4/11/98 - added flips and rotates... */

	for(address = 0x1900;address <= 0xFFFF;address++){
		y = (address >> 8) - 25;
		x = address & 0xFF;
		if( y < 231 - 32)
			bottom = 1;
		else
			bottom = 0;

		if (Machine->orientation & ORIENTATION_SWAP_XY){
			tmp = x;
			x = y;
			y = tmp;
		}
		if (Machine->orientation & ORIENTATION_FLIP_X)
			x = bitmap->width - 1 - x;
		if (Machine->orientation & ORIENTATION_FLIP_Y)
			y = bitmap->height - 1 - y;


		if(bottom)
			bitmap->line[y][x] = Machine->pens[RAM[0x4B00 + ((missile_videoram[address] >> 5) & 6)] + 1];
		else
			bitmap->line[y][x] = Machine->pens[RAM[0x4B00 + (missile_videoram[address] >> 5)] + 1];
	}
}
