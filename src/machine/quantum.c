#include "driver.h"
#include "inptport.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/avgdvg.h"

/* globals */

unsigned char *quantum_ram;
unsigned char *quantum_nvram;


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
	quantum_nvram[offset >> 1] = (unsigned char)(value & 0xF);
}

int quantum_nvram_read(int offset)
{
	return (int)(quantum_nvram[offset >> 1]);
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
	if (offset & 0x20) /* A5 selects chip */
		pokey2_w((offset >> 1) % 0x10, value);
	else
		pokey1_w((offset >> 1) % 0x10, value);
}

int quantum_snd_read(int offset)
{
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

	x = input_port_4_r (offset);
	y = input_port_3_r (offset);

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
	return (input_port_1_r (0) << (7 - (offset - POT0_C))) & 0x80;
}

int quantum_input_2_r(int offset)
{
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
	static int nvram_loaded = 0;
	void *f;

	/* load the nvram contents immediately, but only once */
	if (nvram_loaded == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name, 0,
						   OSD_FILETYPE_HIGHSCORE, 0)) != 0)
		{
			osd_fread(f, quantum_nvram, sizeof(quantum_nvram));
			nvram_loaded = 1;
		}
		else
			quantum_nvram[15] = 5;
	}

	/* load the other ram contents once the highscores have been initialised */
	if (memcmp(&quantum_ram[0x1c902 - 0x18000], "\x00\x01\xbe\x16", 4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0))
			!= 0)
		{
			/* skip the nvram contents */
			osd_fseek(f, sizeof(quantum_nvram), SEEK_SET);

			/* restore all time scores */
			osd_fread(f, quantum_ram + 0x1b52e - 0x18000, 2 * 3 * 3);   /* initials */
			osd_fread(f, quantum_ram + 0x1b540 - 0x18000, 4 * 3);         /* scores */
			osd_fread(f, quantum_ram + 0x1b5aa - 0x18000, 4);	   /* the highscore */
			osd_fread(f, quantum_ram + 0x1bdbc - 0x18000, 2 * 7 * 3);     /* digits */
			osd_fread(f, quantum_ram + 0x1c158 - 0x18000, 7); /* highscore's digits */

			/* today's scores - not for saving? */
			/* osd_fread(f, quantum_ram + 0x1c726 - 0x18000, 0x1c816 - 0x1c726); */

			osd_fclose(f);
		}

		return 1;
	}
	else
	{
		return 0;  /* we can't load the hi scores yet */
	}
	return -1;
}

void quantum_nvram_save(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name, 0,
					   OSD_FILETYPE_HIGHSCORE, 1)) != 0)
	{
		/* nvram */
		osd_fwrite(f, quantum_nvram, sizeof(quantum_nvram));

		/* all time scores */
		osd_fwrite(f, quantum_ram + 0x1b52e - 0x18000, 2 * 3 * 3);		/* initials */
		osd_fwrite(f, quantum_ram + 0x1b540 - 0x18000, 4 * 3);			  /* scores */
		osd_fwrite(f, quantum_ram + 0x1b5aa - 0x18000, 4);			   /* highscore */
		osd_fwrite(f, quantum_ram + 0x1bdbc - 0x18000, 2 * 7 * 3);		  /* digits */
		osd_fwrite(f, quantum_ram + 0x1c158 - 0x18000, 7);	  /* highscore's digits */

		/* today's scores - not for saving? */
		/* osd_fwrite(f, quantum_ram + 0x1c726 - 0x18000, 0x1c816 - 0x1c726); */

		osd_fclose(f);
	}
}
