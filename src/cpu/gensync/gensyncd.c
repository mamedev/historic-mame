#include <stdio.h>
#include <string.h>
#include "gensync.h"

unsigned gensyncd(char *buffer, unsigned PC)
{
	int hblank = cpu_get_reg(GS_HBLANK);
	int hsync = cpu_get_reg(GS_HSYNC);
	int vblank = cpu_get_reg(GS_VBLANK);
	int vsync = cpu_get_reg(GS_VSYNC);

	if( hblank )
	{
		buffer += sprintf(buffer, " HB");
		if( hsync )
			buffer += sprintf(buffer, " HS");
    }
	else
		buffer += sprintf(buffer, " X:%03X", cpu_get_reg(GS_X));

    if( vblank )
    {
		buffer += sprintf(buffer, " VB");
		if( vsync )
			buffer += sprintf(buffer, " VS");
    }
	else
		buffer += sprintf(buffer, " Y:%03X", cpu_get_reg(GS_Y));

    return 1;
}

