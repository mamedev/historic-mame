/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

extern void cps1_dump_video(void);

int cps1_ram_size;
unsigned char *cps1_ram;

void cps1_dump_driver(void)
{
        FILE *fp=fopen("RAM.DMP", "w+b");
        if (fp)
        {
                fwrite(cps1_ram, cps1_ram_size, 1, fp);
                fclose(fp);
        }
        fp=fopen("ROM.DMP", "w+b");
        if (fp)
        {
                fwrite(RAM, 0x80000, 1, fp);
                fclose(fp);
        }

}

int cps1_input_r(int offset)
{
       return (readinputport (offset) << 8) + readinputport (offset+1);
}


int cps1_interrupt(void)
{
        if (osd_key_pressed(OSD_KEY_F))
        {
                cps1_dump_video();
                cps1_dump_driver();
        }
        return 6;
}


int cps1_interrupt2(void)
{
        static int s;
        s^=1;

        if (osd_key_pressed(OSD_KEY_F))
        {
                cps1_dump_video();
                cps1_dump_driver();
        }
        if (s)
                return 2;
        else
                return 4;
}

