/*****************************************************************************
 * Generic Video Synchronization CPU replacement for non-CPU games
 * It does nothing but count horizontal and vertical synchronization,
 * so a driver can use cpu_getscanline() and the provided registers to
 * access the bits of generic horizontal and vertical counters.
 * A driver defines it's video layout by specifying ten parameters:
 * h_max			horizontal modulo value (including blanking/sync)
 * v_max			vertical modulo value (including blanking/sync)
 * hblank_start 	start of horizontal blanking
 * hsync_start		start of horizontal sync
 * hsync_end		end of horizontal sync
 * hblank_end		end of horizontal blanking
 * vblank_start 	start of vertical blanking
 * vsync_start		start of vertical sync
 * vsync_end		end of vertical sync
 * vblank_end		end of vertical blanking
 *****************************************************************************/

#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "gensync.h"

/* Layout of the registers in the debugger */
static UINT8 gensync_reg_layout[] = {
	GS_1H, GS_2H, GS_4H, GS_8H, GS_16H,
	GS_32H, GS_64H, GS_128H, GS_256H, GS_512H, -1,
	GS_1V, GS_2V, GS_4V, GS_8V, GS_16V,
	GS_32V, GS_64V, GS_128V, GS_256V, GS_512V, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 gensync_win_layout[] = {
     0, 0,80, 2,    /* register window (top rows) */
     0, 3,24,19,    /* disassembler window (left colums) */
    25, 3,55, 9,    /* memory #1 window (right, upper middle) */
    25,13,55, 9,    /* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};

typedef struct {
	int pc;
	int h_max, v_max, size;
	int hblank_start, hsync_start, hsync_end, hblank_end;
	int vblank_start, vsync_start, vsync_end, vblank_end;
}	GENSYNC;

GENSYNC gensync;

int gensync_ICount;

/*
 * Call this function with an ponter to an array of ten ints:
 * The horizontal and vertical maximum values, the horizontal
 * blanking start, sync start, sync end and blanking end followed
 * by the vertical blanking start, sync start, sync end counter values.
 * In your machine driver add a pointer to an array like:
 *		int video[] = {454,262, 0,32,64,80, 0,4,8,16 };
 * for the reset_param to the CPU_GENSYNC entry.
 */
void gensync_reset(void *param)
{
	int *video = param;
	gensync.pc = 0;
	gensync.h_max = video[0];
	gensync.v_max = video[1];
	gensync.size = gensync.h_max * gensync.v_max;
	gensync.hblank_start = video[2];
	gensync.hsync_start = video[3];
	gensync.hsync_end = video[4];
	gensync.hblank_end = video[5];
	gensync.vblank_start = video[6];
	gensync.vsync_start = video[7];
	gensync.vsync_end = video[8];
	gensync.vblank_end = video[9];
}

void gensync_exit(void)
{
}

int gensync_execute(int cycles)
{
	gensync_ICount = cycles;

#ifdef  MAME_DEBUG
	do
	{
        CALL_MAME_DEBUG;
		if( ++gensync.pc == gensync.size )
			gensync.pc = 0;
	} while( --gensync_ICount > 0 );
#else
	gensync.pc += gensync_ICount;
	gensync_ICount = 0;
#endif

	return cycles - gensync_ICount;
}

unsigned gensync_get_context(void *reg)
{
	if( reg )
		*(GENSYNC *)reg = gensync;
	return sizeof(gensync);
}

void gensync_set_context(void *reg)
{
	if( reg )
		gensync = *(GENSYNC *)reg;
}

unsigned gensync_get_pc(void)
{
	return gensync.pc;
}

void gensync_set_pc(unsigned val)
{
	gensync.pc = val % (gensync.h_max * gensync.v_max);
}

unsigned gensync_get_sp(void)
{
	return 0;
}

void gensync_set_sp(unsigned val)
{
}

unsigned gensync_get_reg(int regnum)
{
	int h = gensync.pc % gensync.h_max;
	int v = (gensync.pc / gensync.h_max) % gensync.v_max;

    switch( regnum )
	{
	case GS_H:
		return h;
	case GS_V:
		return v;
	case GS_MAX_H:
		return gensync.h_max;
	case GS_MAX_V:
		return gensync.v_max;
	case GS_X:
		if( gensync.hblank_start < gensync.hblank_end )
		{
			if( h >= gensync.hblank_start && h < gensync.hblank_end )
				return -1;
			return h - gensync.hblank_end;
		}
		if( h >= gensync.hblank_start || h < gensync.hblank_end )
			return -1;
		return h - gensync.hblank_start;
	case GS_Y:
		if( gensync.vblank_start < gensync.vblank_end )
		{
			if( v >= gensync.vblank_start && v < gensync.vblank_end )
				return -1;
			return v - gensync.vblank_end;
		}
		if( v >= gensync.vblank_start || v < gensync.vblank_end )
			return -1;
		return v - gensync.vblank_start;
    case GS_HBLANK_START:
		return gensync.hblank_start;
	case GS_HSYNC_START:
		return gensync.hsync_start;
	case GS_HSYNC_END:
		return gensync.hsync_end;
	case GS_HBLANK_END:
		return gensync.hblank_end;
	case GS_VBLANK_START:
		return gensync.vblank_start;
	case GS_VSYNC_START:
		return gensync.vsync_start;
	case GS_VSYNC_END:
		return gensync.vsync_end;
	case GS_VBLANK_END:
		return gensync.vblank_end;
	case GS_HBLANK:
		if( gensync.hblank_start < gensync.hblank_end )
			return (h >= gensync.hblank_start && h < gensync.hblank_end);
		return (h >= gensync.hblank_start || h < gensync.hblank_end);
	case GS_HSYNC:
		if( gensync.hsync_start < gensync.hsync_end )
			return (h >= gensync.hsync_start && h < gensync.hsync_end);
		return (h >= gensync.hsync_start || h < gensync.hsync_end);
	case GS_VBLANK:
		if( gensync.vblank_start < gensync.vblank_end )
			return (v >= gensync.vblank_start && v < gensync.vblank_end);
		return (v >= gensync.vblank_start || v < gensync.vblank_end);
	case GS_VSYNC:
		if( gensync.vsync_start < gensync.vsync_end )
			return (v >= gensync.vsync_start && v < gensync.vsync_end);
		return (v >= gensync.vsync_start || v < gensync.vsync_end);
	case GS_1H:
		return (h >> 0) & 1;
	case GS_2H:
		return (h >> 1) & 1;
	case GS_4H:
		return (h >> 2) & 1;
	case GS_8H:
		return (h >> 3) & 1;
	case GS_16H:
		return (h >> 4) & 1;
	case GS_32H:
		return (h >> 5) & 1;
	case GS_64H:
		return (h >> 6) & 1;
	case GS_128H:
		return (h >> 7) & 1;
	case GS_256H:
		return (h >> 8) & 1;
	case GS_512H:
		return (h >> 9) & 1;
	case GS_1V:
		return (v >> 0) & 1;
	case GS_2V:
		return (v >> 1) & 1;
	case GS_4V:
		return (v >> 2) & 1;
	case GS_8V:
		return (v >> 3) & 1;
	case GS_16V:
		return (v >> 4) & 1;
	case GS_32V:
		return (v >> 5) & 1;
	case GS_64V:
		return (v >> 6) & 1;
	case GS_128V:
		return (v >> 7) & 1;
	case GS_256V:
		return (v >> 8) & 1;
	case GS_512V:
		return (v >> 9) & 1;
    }
	return 0;
}

void gensync_set_reg(int regnum, unsigned val)
{
	int h = gensync.pc % gensync.h_max;
    int v = (gensync.pc / gensync.h_max) % gensync.v_max;

    switch( regnum )
	{
	case GS_PC:
		gensync.pc = val % gensync.size;
		return;
    case GS_H:
		h = val % gensync.h_max;
		break;
	case GS_V:
		v = val % gensync.v_max;
		break;
	case GS_HBLANK_START:
		gensync.hblank_start = val;
	case GS_HSYNC_START:
		gensync.hsync_start = val;
	case GS_HSYNC_END:
		gensync.hsync_end = val;
	case GS_HBLANK_END:
		gensync.hblank_end = val;
	case GS_VBLANK_START:
		gensync.vblank_start = val;
	case GS_VSYNC_START:
		gensync.vsync_start = val;
	case GS_VSYNC_END:
		gensync.vsync_end = val;
	case GS_VBLANK_END:
		gensync.vblank_end = val;
    case GS_1H:
		h = (h & ~0x001) | ((val & 1) << 0);
		break;
	case GS_2H:
		h = (h & ~0x002) | ((val & 1) << 1);
		break;
	case GS_4H:
		h = (h & ~0x004) | ((val & 1) << 2);
		break;
	case GS_8H:
		h = (h & ~0x008) | ((val & 1) << 3);
		break;
	case GS_16H:
		h = (h & ~0x010) | ((val & 1) << 4);
		break;
	case GS_32H:
		h = (h & ~0x020) | ((val & 1) << 5);
		break;
	case GS_64H:
		h = (h & ~0x040) | ((val & 1) << 6);
		break;
	case GS_128H:
		h = (h & ~0x080) | ((val & 1) << 7);
		break;
	case GS_256H:
		h = (h & ~0x100) | ((val & 1) << 8);
		break;
	case GS_512H:
		h = (h & ~0x200) | ((val & 1) << 9);
		break;
	case GS_1V:
		v = (v & ~0x001) | ((val & 1) << 0);
		break;
	case GS_2V:
		v = (v & ~0x002) | ((val & 1) << 1);
		break;
	case GS_4V:
		v = (v & ~0x004) | ((val & 1) << 2);
		break;
	case GS_8V:
		v = (v & ~0x008) | ((val & 1) << 3);
		break;
	case GS_16V:
		v = (v & ~0x010) | ((val & 1) << 4);
		break;
	case GS_32V:
		v = (v & ~0x020) | ((val & 1) << 5);
		break;
	case GS_64V:
		v = (v & ~0x040) | ((val & 1) << 6);
		break;
	case GS_128V:
		v = (v & ~0x080) | ((val & 1) << 7);
		break;
	case GS_256V:
		v = (v & ~0x100) | ((val & 1) << 8);
		break;
	case GS_512V:
		v = (v & ~0x200) | ((val & 1) << 9);
		break;
    }
	gensync.pc = v * gensync.h_max + h;
}

void gensync_set_nmi_line(int linestate)
{
}

void gensync_set_irq_line(int irqline, int linestate)
{
}

void gensync_set_irq_callback(int(*callback)(int irqline))
{
}

void gensync_internal_interrupt(int type)
{
}

void gensync_state_save(void *file)
{
}

void gensync_state_load(void *file)
{
}

const char* gensync_info(void *context,int regnum)
{
	static char buffer[64][7+1];
    static int which = 0;
	int h = 0, v = 0;
    GENSYNC *r = context;

    which = ++which % 64;
    buffer[which][0] = '\0';
    if( !context )
		r = &gensync;
	if( r->h_max )
	{
		h = r->pc % r->h_max;
		if( r->v_max )
			v = (r->pc / r->h_max) % r->v_max;
	}

    switch( regnum )
    {
		case CPU_INFO_REG+GS_PC: sprintf(buffer[which], "%03X:%03X", h, v); break;
		case CPU_INFO_REG+GS_HBLANK_START: sprintf(buffer[which], "HBS:%03X", r->hblank_start); break;
		case CPU_INFO_REG+GS_HSYNC_START: sprintf(buffer[which], "HSS:%03X", r->hsync_start); break;
		case CPU_INFO_REG+GS_HSYNC_END: sprintf(buffer[which], "HSE:%03X", r->hsync_end); break;
		case CPU_INFO_REG+GS_HBLANK_END: sprintf(buffer[which], "HBE:%03X", r->hblank_end); break;
		case CPU_INFO_REG+GS_VBLANK_START: sprintf(buffer[which], "VBS:%03X", r->hblank_start); break;
		case CPU_INFO_REG+GS_VSYNC_START: sprintf(buffer[which], "VSS:%03X", r->hsync_start); break;
		case CPU_INFO_REG+GS_VSYNC_END: sprintf(buffer[which], "VSE:%03X", r->hsync_end); break;
		case CPU_INFO_REG+GS_VBLANK_END: sprintf(buffer[which], "VBE:%03X", r->hblank_end); break;
        case CPU_INFO_REG+GS_1H: sprintf(buffer[which], "1H:%X", (h >> 0) & 1); break;
		case CPU_INFO_REG+GS_2H: sprintf(buffer[which], "2H:%X", (h >> 1) & 1); break;
		case CPU_INFO_REG+GS_4H: sprintf(buffer[which], "4H:%X", (h >> 2) & 1); break;
		case CPU_INFO_REG+GS_8H: sprintf(buffer[which], "8H:%X", (h >> 3) & 1); break;
		case CPU_INFO_REG+GS_16H: sprintf(buffer[which], "16H:%X", (h >> 4) & 1); break;
		case CPU_INFO_REG+GS_32H: sprintf(buffer[which], "32H:%X", (h >> 5) & 1); break;
		case CPU_INFO_REG+GS_64H: sprintf(buffer[which], "64H:%X", (h >> 6) & 1); break;
		case CPU_INFO_REG+GS_128H: sprintf(buffer[which], "128H:%X", (h >> 7) & 1); break;
		case CPU_INFO_REG+GS_256H: sprintf(buffer[which], "256H:%X", (h >> 8) & 1); break;
		case CPU_INFO_REG+GS_512H: sprintf(buffer[which], "512H:%X", (h >> 8) & 1); break;
		case CPU_INFO_REG+GS_1V: sprintf(buffer[which], "1V:%X", (v >> 0) & 1); break;
		case CPU_INFO_REG+GS_2V: sprintf(buffer[which], "2V:%X", (v >> 1) & 1); break;
		case CPU_INFO_REG+GS_4V: sprintf(buffer[which], "4V:%X", (v >> 2) & 1); break;
		case CPU_INFO_REG+GS_8V: sprintf(buffer[which], "8V:%X", (v >> 3) & 1); break;
		case CPU_INFO_REG+GS_16V: sprintf(buffer[which], "16V:%X", (v >> 4) & 1); break;
		case CPU_INFO_REG+GS_32V: sprintf(buffer[which], "32V:%X", (v >> 5) & 1); break;
		case CPU_INFO_REG+GS_64V: sprintf(buffer[which], "64V:%X", (v >> 6) & 1); break;
		case CPU_INFO_REG+GS_128V: sprintf(buffer[which], "128V:%X", (v >> 7) & 1); break;
		case CPU_INFO_REG+GS_256V: sprintf(buffer[which], "256V:%X", (v >> 8) & 1); break;
		case CPU_INFO_REG+GS_512V: sprintf(buffer[which], "512V:%X", (v >> 8) & 1); break;
		case CPU_INFO_FLAGS: sprintf(buffer[which], "%4d:%4d", v, h); break;
		case CPU_INFO_NAME: return "GENSYNC";
		case CPU_INFO_FAMILY: return "GENSYNC generic video synchronization";
        case CPU_INFO_VERSION: return "0.1";
        case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 1999, The MAMEDEV team.";
		case CPU_INFO_REG_LAYOUT: return (const char*)gensync_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)gensync_win_layout;
    }
    return buffer[which];
}

unsigned gensync_dasm(char *buffer,unsigned pc)
{
#ifdef MAME_DEBUG
	return gensyncd(buffer, pc);
#else
	sprintf(buffer, "%3d", pc);
	return 1;
#endif
}
