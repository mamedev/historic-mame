/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

void blktiger_bankswitch_w(int offset,int data)
{
        static int olddata=0xff;
        int bankaddress;
        if (data != olddata)
        {
                bankaddress = 0x10000 + (data & 0x0f) * 0x4000 ;
                memcpy(&RAM[0x8000], &ROM[bankaddress], 0x4000);
                olddata=data;
        }
}

int blktiger_interrupt(void)
{
        if (osd_key_pressed(OSD_KEY_F))
        {
                FILE *fp=fopen("RAM.DMP", "w+b");
                if (fp)
                {
                        fwrite(RAM, 0x10000, 1, fp);
                        fclose(fp);
                }
        }

        return 0x38;
}
