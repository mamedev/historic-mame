/**************************************************************************
 *               National Semiconductor COP400 Emulator                   *
 *                                                                        *
 *                   Copyright (C) 2006 MAME Team                         *
 **************************************************************************/

#ifndef _COP400_H
#define _COP400_H

#ifndef INLINE
#define INLINE static inline
#endif

#define COP400_CLOCK_DIVIDER	4

#define COP400_PORT_L	0x100
#define COP400_PORT_G	0x101
#define COP400_PORT_D	0x102
#define	COP400_PORT_IN	0x103
#define	COP400_PORT_SK	0x104
#define	COP400_PORT_SIO	0x105

enum {
	COP400_PC=1,
	COP400_A,
	COP400_B,
	COP400_C,
	COP400_G,
	COP400_EN,
	COP400_Q,
    COP400_SA,
    COP400_SB,
    COP400_SC,
    COP400_SIO,
    COP400_SKL
};

extern void cop410_get_info(UINT32 state, union cpuinfo *info);
extern void cop420_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
int 	DasmCOP410(char *dst, unsigned pc);
int 	DasmCOP420(char *dst, unsigned pc);
#endif

#endif  /* _COP400_H */
