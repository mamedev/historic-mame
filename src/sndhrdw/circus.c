/*
* Links to Generic DAC routines for Music
*
* Other effects will be added as samples
*/

#include "driver.h"
#include "generic.h"
#include "sndhrdw/dac.h"

static struct DACinterface DAinterface =
{
1,
441000,
{ 255, 255 },
{  1,  1 } /* filter rate (rate = Register(ohm)*Capaciter(F)*1000000) */
};

void circus_dac_w(int offset,int data)
{
DAC_data_w(offset , data);
}

int circus_sh_start(void)
{
if( DAC_sh_start(&DAinterface) ) return 1;

return 0;
}

void circus_sh_stop(void)
{
DAC_sh_stop();
}

void circus_sh_update(void)
{
DAC_sh_update();
}
