#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mame.h"
#include "driver.h"
#include "osdepend.h"



void mooncrst_sound_freq_w(int offset,int data)
{
	if (data && data != 0xff) osd_adjust_sample(0,24000/(256-data)*32,255);
/*
	if (data && data != 0xff) osd_adjust_sample(0,(data*8)*32,255);
*/
	else osd_adjust_sample(0,1000,0);
}



int mooncrst_sh_start(void)
{
	osd_play_sample(0,Machine->drv->samples,32,1000,0,1);
	return 0;
}



void mooncrst_sh_update(void)
{
}
