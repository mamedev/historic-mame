#include "driver.h"
#include "inptport.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "vidhrdw/avgdvg.h"
#include "sndhrdw/pokyintf.h"

/* globals (to this module) */


int quantum_ram_size;

static unsigned char *ram;	/* ASG 971124 */
static unsigned char *vram;
static unsigned char nvram[256];


/*** quantum_avg_start
*
* Purpose: our own hook to the Atari Vector Graphics start
*
* Returns: avg_start()
*
* History: 11/19/97 PF Created
*
**************************/
int quantum_avg_start(void)
{
	/* dynamically allocate this to be polite -
	   running mame != always running my driver! :) */
	vram = (unsigned char *)malloc(vectorram_size);

	if (!vram)
		return 1;

	ram = malloc(quantum_ram_size);
	if (!ram)
	{
		free (vram);
		return 1;
	}
	cpu_setbank(1,ram);

	vectorram = vram;
	return avg_start_quantum();
}


/*** quantum_interrupt
*
* Purpose: do an interrupt - so many times per raster frame
*
* Returns: 0
*
* History: 11/19/97 PF Created
*
**************************/
int quantum_interrupt(void)
{
	if (cpu_getiloops() == 5)
		avgdvg_clr_busy();

	return 1; /* ipl0' == ivector 1 */
}

/*** quantum_switches_r
*
* Purpose: read switches input, which sneaks the VHALT' in
*
* Returns: byte
*
* History: 11/20/97 PF Created
*
**************************/
int quantum_switches_r(int offset)
{
	if (!(offset & 1))
		return 0xff;

	return (input_port_0_r(0) |
		(avgdvg_done() ? 1 : 0));
}

/*** quantum_led_write
*
* Purpose: turn on or off LED's.
*	Entire address range used, only D0 and D1 used.
*
* Returns: Nothing
*
* History: 11/19/97 PF Created
*
**************************/
void quantum_led_write(int offset, int value)
{
	osd_led_w(0, value & 0x10);
	osd_led_w(1, value & 0x20);
}


/*** quantum_vram_read, quantum_vram_write
*
* Purpose: read and write vector memory
*	no hokey translation is done, since the vector memory
*	was 16 bit
*
* Returns: Nothing
*
* History: 11/19/97 PF Created
*
**************************/
void quantum_vram_write(int offset, int value)
{
	vram[offset % 0x2000] = value;
}

int quantum_vram_read(int offset)
{
	return vram[offset % 0x2000];
}


/*** quantum_nvram_write, quantum_nvram_read
*
* Purpose: read and write non volitile memory.
*	D0-D3 used, weed out access on even address (D8-D15)
*
* Returns:
*
* History: 11/19/97 PF Created
*
**************************/
void quantum_nvram_write(int offset, int value)
{
	if (!(offset & 0x01))
		return;

	nvram[(offset >> 1) % sizeof nvram] = (unsigned char)(value & 0xF);
}

int quantum_nvram_read(int offset)
{
	if (!(offset & 0x01))
		return 0xff;

	return (int)(nvram[(offset >> 1) % sizeof nvram]);
}


/*** quantum_snd_read, quantum_snd_write
*
* Purpose: read and write POKEY chips -
*	need to do translation, so we don't directly map it
*
* Returns: register value, for read
*
* History: 11/19/97 PF Created
*
**************************/
void quantum_snd_write(int offset, int value)
{
	/* D8-D15 aren't hooked up in this range */
	if (!(offset & 0x01))
		return;

	if (offset & 0x20) /* A5 selects chip */
		pokey2_w((offset >> 1) % 0x10, value);
	else
		pokey1_w((offset >> 1) % 0x10, value);
}

int quantum_snd_read(int offset)
{
	if (!(offset & 0x01))
		return 0xff;

	if (offset & 0x20)
		return pokey2_r((offset >> 1) % 0x10);
	else
		return pokey1_r((offset >> 1) % 0x10);
}


/*** quantum_trackball
*
* Purpose: read trackball port.  So far, attempting theory:
*	D0-D3 - vert movement delta
*	D4-D7 - horz movement delta
*
*	if wrong, will need to pull out my 74* logic reference
*
* Returns: 8 bit value
*
* History: 11/19/97 PF Created
*
**************************/
int quantum_trackball_r (int offset)
{
	int x, y;

	if (!(offset & 1))
		return 0;

	/* we seem to need this in order to get accurate trackball movement */
	update_analog_ports ();

	x = input_port_4_r (offset) >> 4;
	y = input_port_3_r (offset) >> 4;

	return (x << 4) + y;
}


/*** quantum_input_1_r, quantum_input_2_r
*
* Purpose: POKEY input switches read
*
* Returns: in the high bit the appropriate switch value
*
* History: 12/2/97 ASG Created
*
**************************/
int quantum_input_1_r(int offset)
{
	if (!(offset & 1))
		return 0;
	return (input_port_1_r (0) << (7 - (offset - POT0_C))) & 0x80;
}

int quantum_input_2_r(int offset)
{
	if (!(offset & 1))
		return 0;
	return (input_port_2_r (0) << (7 - (offset - POT0_C))) & 0x80;
}


/*** quantum_nvram_load, quantum_nvram_save
*
* Purpose: MAME interface for storing NVRAM to disk
*
* Returns: _load() returns -1 to tell MAME to quit trying
*
* History: 11/19/97 PF Created
*
**************************/
int quantum_nvram_load(void)
{
	return -1;
}

void quantum_nvram_save(void)
{
}
