/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"

static unsigned int rom_offset;

void capbowl_rom_select_w(int offset,int data)
{
  switch (data & 0x1f)
  {
  case 0:
  default:
      rom_offset = 0x10000;
      break;
  case 1:
      rom_offset = 0x14000;
      break;
  case 4:
      rom_offset = 0x18000;
      break;
  case 5:
      rom_offset = 0x1c000;
      break;
  case 8:
      rom_offset = 0x20000;
      break;
  case 9:
      rom_offset = 0x24000;
      break;
  }
}


int capbowl_pagedrom_r(int offset)
{
  return Machine->memory_region[0][offset + rom_offset];
}


static int capbowl_service = -1;

int capbowl_service_r(int offset)
{
  if (capbowl_service != -1)
  {
    /* This is to pass memory test */
    int ret = capbowl_service;
    capbowl_service = -1;
    return ret;
  }

  /* Service key. Hold down during high-score screen */
  if (osd_key_pressed(OSD_KEY_F1))
  {
    return 0x0b;
  }
  else
  {
    return 0x00;
  }
}

void capbowl_service_w(int offset, int data)
{
  capbowl_service = data;
}


int capbowl_track_y_r(int offset)
{
  int ret = input_port_0_r(offset);

  if (!(ret & 0x01))
  {
    /* Trackball up */
    ret = (ret & 0xf0) | 0x07;
  }
  else if (!(ret & 0x02))
  {
    /* Trackball down */
    ret = (ret & 0xf0) | 0x0f;
  }
  else
  {
    ret &= 0xf0;
  }

  return ret;
}


int capbowl_track_x_r(int offset)
{
  int ret = input_port_1_r(offset);

  if (!(ret & 0x01))
  {
    /* Trackball left */
    ret = (ret & 0xf0) | 0x08;
  }
  else if (!(ret & 0x02))
  {
    /* Trackball right */
    ret = (ret & 0xf0) | 0x07;
  }
  else
  {
    ret &= 0xf0;
  }

  return ret;
}


static int firq_enabled = 0;

void capbowl_firq_enable_w(int offset,int data)
{
  firq_enabled = data & 0x04;
}

void capbowl_interrupt(void)
{
  if (firq_enabled) cpu_cause_interrupt(0,M6809_INT_FIRQ);
  cpu_cause_interrupt(0,M6809_INT_NONE);
}

