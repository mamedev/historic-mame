/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

static int sound_ctrl;
static int fireone_sell;

/* In drivers/starfire.c */
extern unsigned char *starfire_ram;

/* In vidhrdw/starfire.c */
WRITE_HANDLER( starfire_vidctrl_w );
WRITE_HANDLER( starfire_vidctrl1_w );

WRITE_HANDLER( starfire_shadow_w )
{
    starfire_ram[offset & 0x3ff] = data;
}

WRITE_HANDLER( starfire_output_w )
{
    starfire_ram[offset & 0x3ff] = data;
    switch(offset & 0xf) {
    case 0:
		starfire_vidctrl_w(0, data);
		break;
    case 1:
		starfire_vidctrl1_w(0, data);
		break;
    case 2:
		/* Sounds */
		break;
    }
}

WRITE_HANDLER( fireone_output_w )
{
    starfire_ram[offset & 0x3ff] = data;
    switch(offset & 0xf) {
    case 0:
		starfire_vidctrl_w(0, data);
		break;
    case 1:
		starfire_vidctrl1_w(0, data);
		break;
    case 2:
		/* Sounds */
		fireone_sell = (data & 0x8) ? 0 : 1;
		break;
    }
}

READ_HANDLER( starfire_shadow_r )
{
    return starfire_ram[offset & 0x3ff];
}

READ_HANDLER( starfire_input_r )
{
    switch(offset & 0xf) {
    case 0:
		return input_port_0_r(0);
    case 1:
		/* Note : need to loopback sounds lengths on that one */
		return input_port_1_r(0);
    case 5:
		/* Throttle, should be analog too */
		return input_port_4_r(0);
    case 6:
		return input_port_2_r(0);
    case 7:
		return input_port_3_r(0);
    default:
		return 0xff;
    }
}

READ_HANDLER( fireone_input_r )
{
    switch(offset & 0xf) {
    case 0:
		return input_port_0_r(0);
    case 1:
		return input_port_1_r(0);
    case 2:
		/* Throttle, should be analog too */
		return fireone_sell ? input_port_2_r(0) : input_port_3_r(0);
    default:
		return 0xff;
    }
}

WRITE_HANDLER( starfire_soundctrl_w ) {
    sound_ctrl = data;
}

READ_HANDLER( starfire_io1_r ) {
    int in,out;

    in = readinputport(1);
    out = (in & 0x07) | 0xE0;

    if (sound_ctrl & 0x04)
        out = out | 0x08;
    else
        out = out & 0xF7;

    if (sound_ctrl & 0x08)
        out = out | 0x10;
    else
        out = out & 0xEF;

    return out;
}

int starfire_interrupt (void)
{

    return nmi_interrupt();
}
