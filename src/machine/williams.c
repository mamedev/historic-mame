/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"

int williams_init_machine(const char *gamename)
{
/*
	char *sndram;
	sndram = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	if (sndram)
	{
		sndram[0xfffe] = 0xf0;
		sndram[0xffff] = 0x00;
	}
*/
	return 0;
}

static int romenable = 1;

void williams_rom_enable(int offset,int data)
{
	if (errorlog) fprintf(errorlog, "romenable %x = %x\n", offset, data);
	romenable = (data > 0);
}

static char *get_read_addr(int offset)
{
	if ((offset <= 0x9000 && romenable) || (offset > 0xc000))
		return &RAM[offset];
	else
		return &RAM[offset + 0x10000];
}

int williams_paged_read(int offset)
{
	if (romenable)
	{
/*		if (errorlog) fprintf(errorlog, "ROM %x = %x\n", offset, RAM[offset]);*/
		return RAM[offset];
	} else {
/*		if (errorlog) fprintf(errorlog, "RAM %x = %x\n", offset, RAM[offset + 0x10000]);*/
		return RAM[offset + 0x10000];
	}
}

int williams_paged_read_ram(int offset)
{
/*	if (errorlog) fprintf(errorlog, "RAM read %x = %x\n", offset+0x9000, RAM[offset+0x19000]);*/
	return RAM[offset + 0x19000];
}

/*
 * We define dirty regions as 8x8 blocks
 * So we have a 16 x 19 dirty area, which we can define with 19 ints
 */
extern char *williams_dirty;

void williams_paged_write(int offset, int value)
{
/*	if (errorlog) fprintf(errorlog, "RAM write %x = %x\n", offset, value); */
	if (RAM[offset + 0x10000] != value)
	{
		RAM[offset + 0x10000] = value;
		williams_dirty[offset >> 8] = 1;
	}
/*	williams_dirty[(offset & 0xff)>>4] |= (offset>>8); */
}

int blit_w, blit_h, blit_src, blit_dest, blit_mask, blit_op;

#define BLIT(func) 								\
	for (y=0; y<height; y++) {					\
		for (x=0; x<width; x++) {				\
			dest = blit_dest + x*0x100 + y;		\
			if (dest < 0x9800)	{				\
				func;							\
			}									\
			src++;								\
		}										\
	}

static void do_blitting()
{
	int x,y;
	int src, dest;
	int width, height;
	int blit1 = blit_mask & 0x0f;
	int blit2 = blit_mask & 0xf0;
	int b,c;

	if (errorlog) fprintf(errorlog, "doblit op=%x, src=%x, dest=%x, w=%x, h=%x, mask=%x\n",
		blit_op, blit_src, blit_dest, blit_w, blit_h, blit_mask);
		
	width = (blit_h ^ 4);
	height = (blit_w ^ 4);
	src = blit_src;

#if 0
	BLIT(cpu_writemem(dest, blit_mask))
#else
	switch (blit_op)
	{
		case 0x12:
			BLIT(cpu_writemem(dest, blit_mask))
			break;
		case 0x0a:
		case 0x2a:	/* for drawing? */
			BLIT(
				b = cpu_readmem(src);
				c = RAM[dest + 0x10000];
				if ((b & 0x0f) != blit1)
					c = (c & 0xf0) | (b & 0x0f);
				if ((b & 0xf0) != blit2)
					c = (c & 0x0f) | (b & 0xf0);
				cpu_writemem(dest, c)
			)
			break;
		case 0x1a:	/* for erasing ? */
		case 0x3a:
			BLIT(
				b = cpu_readmem(src);
				c = RAM[dest + 0x10000];
				if ((b & 0x0f) != 0)
					c = (c & 0xf0) | blit1;
				if ((b & 0xf0) != 0)
					c = (c & 0x0f) | blit2;
				cpu_writemem(dest, c)
			)
			break;
		case 0x52:
		case 0x92:
			BLIT(
				b = cpu_readmem(src);
				cpu_writemem(dest, b)
			)
		default:
			if (errorlog) fprintf(errorlog, "Unsupported blit op %x\n", blit_op);
			break;
	}
#endif
}

void williams_blitter_w(int offset, int value)
{
/*	if (errorlog) fprintf(errorlog, "blitter write %x = %x\n", offset, value); */
	switch (offset & 7)
	{
		case 0:		/* start blitter */
			blit_op = value;
			do_blitting();
			break;
		case 1:		/* mask */
			blit_mask = value;
			break;
		case 2:		/* src-hi */
			blit_src = (blit_src & 0xff) | (value << 8);
			break;
		case 3:		/* src-lo */
			blit_src = (blit_src & 0xff00) | value;
			break;
		case 4:		/* dest-hi */
			blit_dest = (blit_dest & 0xff) | (value << 8);
			break;
		case 5:		/* dest-lo */
			blit_dest = (blit_dest & 0xff00) | value;
			break;
		case 6:		/* height */
			blit_h = value;
			break;
		case 7:		/* width */
			blit_w = value;
			break;
	}
}

static int watchdog = 0;
static int ypos = 0;

int williams_sound_interrupt(void)
{
	return INT_NONE;
}

int williams_interrupt(void)
{
	RAM[0xcb00] = ypos;
/*	if (errorlog) fprintf(errorlog, "cb00 = %x\n", RAM[0xcb00]); */
	ypos += 0x10;
	if (ypos >= 0x100)
	{
		ypos = 0x0;
		watchdog++;
/*		if (errorlog) fprintf(errorlog, "interrupt, wd = %x\n", watchdog); */
		if (watchdog > 32)
		{
			watchdog = 0;
			m6809_reset();
			return INT_NONE;
		}
	}
	return (ypos == 0x10 || ypos == 0x90) ? INT_IRQ : INT_NONE;
}

void williams_watchdog(int offset, int value)
{
/*	if (errorlog) fprintf(errorlog, "wd reset %x\n", watchdog); */
	/* only watch for this value */
	if (value == 0x39)
		watchdog = 0;
}

int williams_rand(int offset)
{
	int value = rand() & 0xff;
	if (errorlog) fprintf(errorlog, "RND %x = %x\n", offset, value);
	return value;
}

static unsigned char pia_pdr[5];		/* data registers */
static unsigned char pia_ddr[5];		/* direction registers */
static unsigned char pia_cr[5];		/* control registers */
static char offsettoport[] = { 0, 0, 1, 1, 4, 4, 4, 4, 2, 2, 3, 3, 4, 4, 4, 4 };

int williams_pia_read(int offset)
{
	int port = offsettoport[offset];
	int reg = offset&1;
	int value;
	
	if (reg) {
		/* control reg */
		value = pia_cr[port];
	} else {
		if (pia_cr[port] & 0x4) {
			/* PDR */
			value = readinputport(port) & ~pia_ddr[port];
			if (errorlog) fprintf(errorlog, "input port %x = %x\n",
				port, readinputport(port));
		} else {
			/* DDR */
			value = pia_ddr[port];
		}
	}
	if (errorlog) fprintf(errorlog, "PIA read %x:%x = %x\n", port, reg, value);
	return value;
}

extern void williams_play_sound(int n);

void williams_pia_write(int offset, int value)
{
	int port = offsettoport[offset];
	int reg = offset&1;
	if (errorlog) fprintf(errorlog, "PIA write %x:%x = %x\n", port, reg, value);
	if (reg) {
		/* control reg */
		pia_cr[port] = value;
	} else {
		if (pia_cr[port] & 0x4) {
			/* PDR */
			pia_pdr[port] = value & pia_ddr[port];
			if (port == 3)
			{
				williams_play_sound(value);
			}
		} else {
			/* DDR */
			pia_ddr[port] = value;
		}
	}
}

