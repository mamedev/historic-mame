#ifndef _GS_H
#define _GS_H

#include "cpuintrf.h"
#include "osd_cpu.h"

enum {
	GS_PC, GS_H, GS_V, GS_MAX_H, GS_MAX_V, GS_X, GS_Y,
	GS_HBLANK, GS_HSYNC, GS_VBLANK, GS_VSYNC,
	GS_HBLANK_START, GS_HSYNC_START, GS_HSYNC_END, GS_HBLANK_END,
	GS_VBLANK_START, GS_VSYNC_START, GS_VSYNC_END, GS_VBLANK_END,
	GS_1H, GS_2H, GS_4H, GS_8H,
	GS_16H, GS_32H, GS_64H, GS_128H,
	GS_256H, GS_512H,
	GS_1V, GS_2V, GS_4V, GS_8V,
	GS_16V, GS_32V, GS_64V, GS_128V,
	GS_256V, GS_512V
};

extern int gensync_ICount;

extern void gensync_reset(void *param);
extern void gensync_exit(void);
extern int gensync_execute(int cycles);
extern unsigned gensync_get_context(void *reg);
extern void gensync_set_context(void *reg);
extern unsigned gensync_get_pc(void);
extern void gensync_set_pc(unsigned val);
extern unsigned gensync_get_sp(void);
extern void gensync_set_sp(unsigned val);
extern unsigned gensync_get_reg(int regnum);
extern void gensync_set_reg(int regnum, unsigned val);
extern void gensync_set_nmi_line(int linestate);
extern void gensync_set_irq_line(int irqline, int linestate);
extern void gensync_set_irq_callback(int(*callback)(int irqline));
extern void gensync_internal_interrupt(int type);
extern void gensync_state_save(void *file);
extern void gensync_state_load(void *file);
extern const char *gensync_info(void *context,int regnum);
extern unsigned gensync_dasm(char *buffer,unsigned pc);

#ifdef MAME_DEBUG
extern unsigned gensyncd(char *dst, unsigned PC);
#endif

#endif	/* _GS_H */

