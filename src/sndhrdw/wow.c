/**************************************************************************

	Sound Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"
#include "Z80.h"

void sh_votrax_start(int Channel);
void sh_votrax_stop(void);
void votrax_w(int data);
int  votrax_status_r(void);


int wow_sh_start(void)
{
	/* Start votrax emulation (load samples) */

	sh_votrax_start(0);

    return 0;
}

void wow_sh_stop(void)
{
	/* Free the samples */

	sh_votrax_stop();
}

/* Writes to speech chip with an IN command */

int wow_speech_r(int offset)
{
	Z80_Regs regs;
    int data;

	Z80_GetRegs(&regs);
    data = regs.BC.B.h;

	votrax_w(data);

    return data;				/* Probably not used */
}

/* Read from port 2 (0x12) returns speech status as 0x80 */

int wow_port_2_r(int offset)
{
	int Ans;

    Ans = (input_port_2_r(0) & 0x7F);

    if (votrax_status_r() != 0) Ans += 128;

    return Ans;
}

void wow_sh_update(void)
{
}
