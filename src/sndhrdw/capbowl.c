#include "mame.h"
#include "m6809/m6809.h"
#include "sndhrdw/generic.h"

static int trigger_int = 0;

void capbowl_sndcmd_w(int offset,int data)
{
  sound_command_w(offset, data);

  trigger_int++;
}


static int first_time = 1;

int capbowl_sndcmd_r(int offset)
{
  int ret;

  if (first_time)
  {
    ret = 0x00;
    first_time = 0;
  }
  else
  {
    ret = sound_command_r(offset);
    if (errorlog)
    {
      fprintf(errorlog, "Sound Command Read: %02x\n", ret);
    }
  }

  return ret;
}


int capbowl_sound_interrupt(void)
{
  /* This location is normally incremented by FIRQ */
  Machine->memory_region[2][0x000c]++;

  /* Generate an interrupt if there is anything in the queue */
  if (trigger_int)
  {
    trigger_int--;
    return M6809_INT_IRQ;
  }

  return M6809_INT_NONE;
}



static int sound_reg = 0;

void capbowl_sndreg_w(int offset,int data)
{
  sound_reg = data;
}


int capbowl_sndreg_r(int offset)
{
  return 0x00;
}



void capbowl_snddata_w(int offset,int data)
{
  if (errorlog)
  {
    fprintf(errorlog, "Sound Register Write: %02x - %02x\n", sound_reg, data);
  }
}


int capbowl_snddata_r(int offset)
{
  if (errorlog)
  {
    fprintf(errorlog, "Sound Register Read: %02x\n", sound_reg);
  }

  return 0x00;
}



