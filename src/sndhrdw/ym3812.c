#include "driver.h"



int YM3812_status_port_0_r(int offset)
{
	return osd_ym3812_status();
}

int YM3812_read_port_0_r(int offset)
{
	return osd_ym3812_read();
}

void YM3812_control_port_0_w(int offset,int data)
{
	osd_ym3812_control(data);
}

void YM3812_write_port_0_w(int offset,int data)
{
	osd_ym3812_write(data);
}


int YM3812_sh_start(struct YM3812interface *interface)
{
	return 0;
}

void YM3812_sh_stop(void)
{
}

void YM3812_sh_update(void)
{
}
