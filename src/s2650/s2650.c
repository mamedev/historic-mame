/*******************************************************
 *
 *      Portable Signetics 2650 cpu emulation
 *
 *      Written by Juergen Buchmueller for use with MAME
 *
 *******************************************************/

/* define this to have some interrupt information logged */
#define VERBOSE

#include "memory.h"
#include "types.h"
#include "S2650/s2650.h"
#include "S2650/s2650cpu.h"

#ifdef	VERBOSE
#include <stdio.h>
#endif

/* define this to expand all EA calculations inline */
#define INLINE_EA

int     S2650_ICount    = 0;
static  S2650_Regs      S;

/* condition code changes for a byte */
static  byte    ccc[0x200] = {
	0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x04,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
};

#define SET_CC(value) { S.psl = (S.psl & ~CC) | ccc[value]; }
#define SET_CC_OVF(value,before) { S.psl = (S.psl & ~(OVF+CC)) | ccc[value + ( ( (value^before) << 1) & 256 )]; }

/* read next opcode */
static  byte RDOP(void)
{
byte result = cpu_readop(S.page + S.iar);
	S.iar = (S.iar + 1) & PMSK;
	return result;
}

/* read next opcode argument */
static  byte RDOPARG(void)
{
byte result = cpu_readop_arg(S.page + S.iar);
	S.iar = (S.iar + 1) & PMSK;
	return result;
}

#define RDMEM(addr) cpu_readmem16(addr)

/* handy table to build relative offsets from HR */
static	int 	S2650_relative[0x100] = {
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
	-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
	-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
	-16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
	-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
	-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
	-16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
};

/* build effective address with relative addressing */
#define _REL_EA(page) { 								\
byte	hr; 											\
	hr = RDOPARG(); /* get 'holding register' */        \
	/* build effective address within current 8K page */\
	S.ea = page + ((S.iar + S2650_relative[hr]) & PMSK);\
	if (hr & 0x80) /* indirect bit set ? */ 			\
	{													\
	int 	addr = S.ea;								\
		S2650_ICount -= 2;								\
		/* build indirect 32K address */				\
		S.ea = RDMEM(addr) << 8;						\
		if (!(++addr & PMSK))							\
		addr -= PLEN;									\
		S.ea = (S.ea + RDMEM(addr)) & AMSK; 			\
	}													\
}

/* build effective address with absolute addressing */
#define _ABS_EA() { 											\
byte	hr, dr; 												\
	hr = RDOPARG(); /* get 'holding register' */                \
	dr = RDOPARG(); /* get 'data bus register' */               \
	/* build effective address within current 8K page */		\
	S.ea = S.page + (((hr << 8) + dr) & PMSK);					\
	/* indirect addressing ? */ 								\
	if (hr & 0x80)												\
	{															\
	int addr = S.ea;											\
		S2650_ICount -= 2;										\
		/* build indirect 32K address */						\
		/* build indirect 32K address */						\
		S.ea = RDMEM(addr) << 8;								\
		if (!(++addr & PMSK))									\
			addr -= PLEN;										\
		S.ea = (S.ea + RDMEM(addr)) & AMSK; 					\
	}															\
	/* check for indexed addressing mode */ 					\
	switch (hr & 0x60)											\
	{															\
		case 0x00: /* not indexed */							\
			break;												\
		case 0x20: /* auto increment indexed */ 				\
			S.reg[S.r] += 1;									\
			S.ea = (S.ea & PAGE)+((S.ea+S.reg[S.r]) & PMSK);	\
			S.r = 0; /* absolute addressing reg is R0 */		\
			break;												\
		case 0x40: /* auto decrement indexed */ 				\
			S.reg[S.r] -= 1;									\
			S.ea = (S.ea & PAGE)+((S.ea+S.reg[S.r]) & PMSK);	\
			S.r = 0; /* absolute addressing reg is R0 */		\
			break;												\
		case 0x60: /* indexed */								\
			S.ea = (S.ea & PAGE)+((S.ea+S.reg[S.r]) & PMSK);	\
			S.r = 0; /* absolute addressing reg is R0 */		\
			break;												\
	}															\
}

/* build effective address with absolute addressing (branch) */
#define _BRA_EA() { 											\
byte	hr, dr; 												\
	hr = RDOPARG(); /* get 'holding register' */                \
	dr = RDOPARG(); /* get 'data bus register' */               \
	/* build address in 32K address space */					\
	S.ea = ((hr << 8) + dr) & AMSK; 							\
	/* indirect addressing ? */ 								\
	if (hr & 0x80)												\
	{															\
	int 	addr = S.ea;										\
		S2650_ICount -= 2;										\
		/* build indirect 32K address */						\
		S.ea = RDMEM(addr) << 8;								\
		if (!(++addr & PMSK))									\
			addr -= PLEN;										\
		S.ea = (S.ea + RDMEM(addr)) & AMSK; 					\
	}															\
}

/* swap registers 1-3 with 4-6 */
/* this is done when the RS bit in PSL changes */
#define SWAP_REGS { 		\
byte swap;					\
	swap = S.reg[1];		\
	S.reg[1] = S.reg[4];	\
	S.reg[4] = swap;		\
	swap = S.reg[2];		\
	S.reg[2] = S.reg[5];	\
	S.reg[5] = swap;		\
	swap = S.reg[3];		\
	S.reg[3] = S.reg[6];	\
	S.reg[6] = swap;		\
}

/* BRanch Relative if cond is true */
#define S_BRR(cond) {					\
	if (cond) { 						\
		REL_EA(S.page); 				\
		S.page = S.ea & PAGE;			\
		S.iar  = S.ea & PMSK;			\
		change_pc(S.ea);				\
	} else	S.iar = (S.iar + 1) & PMSK; \
}

/* Zero Branch Relative  */
#define S_ZBRR() {			\
	REL_EA(0x0000); 		\
	S.page = S.ea & PAGE;	\
	S.iar  = S.ea & PMSK;	\
	change_pc(S.ea);		\
}

/* BRanch Absolute if cond is true */
#define S_BRA(cond) {					\
	if (cond) { 						\
		BRA_EA();						\
		S.page = S.ea & PAGE;			\
		S.iar  = S.ea & PMSK;			\
		change_pc(S.ea);				\
	} else S.iar = (S.iar + 2) & PMSK;	\
}

/* Branch indeXed Absolute (R3) */
#define S_BXA() {						\
	BRA_EA();							\
	S.ea   = (S.ea + S.reg[3]) & AMSK;	\
	S.page = S.ea & PAGE;				\
	S.iar  = S.ea & PMSK;				\
	change_pc(S.ea);					\
}

/* Branch to Subroutine Relative if cond is true */
#define S_BSR(cond) {									\
	if (cond) { 										\
		REL_EA(S.page); 								\
		S.psu  = (S.psu & ~SP) | ((S.psu + 1) & SP);	\
		S.ras[S.psu & SP] = S.iar;						\
		S.page = S.ea & PAGE;							\
		S.iar  = S.ea & PMSK;							\
		change_pc(S.ea);								\
	} else	S.iar = (S.iar + 1) & PMSK; 				\
}

/* Zero Branch to Subroutine Relative */
#define S_ZBSR() {									\
	REL_EA(0x0000); 								\
	S.psu  = (S.psu & ~SP) | ((S.psu + 1) & SP);	\
	S.ras[S.psu & SP] = S.iar;						\
	S.page = S.ea & PAGE;							\
	S.iar  = S.ea & PMSK;							\
	change_pc(S.ea);								\
}

/* Branch to Subroutine Absolute */
#define S_BSA(cond) {								\
	if (cond) { 									\
		BRA_EA();									\
		S.psu = (S.psu & ~SP) | ((S.psu + 1) & SP); \
		S.ras[S.psu & SP] = S.iar;					\
		S.page = S.ea & PAGE;						\
		S.iar  = S.ea & PMSK;						\
		change_pc(S.ea);							\
	} else S.iar = (S.iar + 2) & PMSK;				\
}

/* Branch to Subroutine indeXed Absolute (R3) */
#define S_BSXA() {								\
	BRA_EA();									\
	S.ea  = (S.ea + S.reg[3]) & AMSK;			\
	S.psu = (S.psu & ~SP) | ((S.psu + 1) & SP); \
	S.ras[S.psu & SP] = S.iar;					\
	S.page = S.ea & PAGE;						\
	S.iar  = S.ea & PMSK;						\
	change_pc(S.ea);							\
}

/* RETurn from subroutine */
#define S_RET(cond) {								\
	if (cond) { 									\
		S.ea = S.ras[S.psu & SP];					\
		S.psu = (S.psu & ~SP) | ((S.psu - 1) & SP); \
		S.page = S.ea & PAGE;						\
		S.iar  = S.ea & PMSK;						\
		change_pc(S.ea);							\
	}												\
}

/* RETurn from subroutine and Enable interrupts */
#define S_RETE(cond) {								\
	if (cond) { 									\
		S.ea = S.ras[S.psu & SP];					\
		S.psu = (S.psu & ~SP) | ((S.psu - 1) & SP); \
		S.page = S.ea & PAGE;						\
		S.iar  = S.ea & PMSK;						\
		change_pc(S.ea);							\
		S.psu &= ~II;								\
	}												\
}

/* LOaD destination with source */
#define S_LOD(dest,source) {  \
	dest = source;		  \
	SET_CC(dest);		  \
}

/* SToRe source to memory addr (CC unchanged) */
#define S_STR(address,source) cpu_writemem16(address, source)

/* logical and destination with source */
#define S_AND(dest,source) {	\
	dest &= source; 			\
	SET_CC(dest);				\
}

/* logical inclusive or destination with source */
#define S_IOR(dest,source) {	\
	dest |= source; 			\
	SET_CC(dest);				\
}

/* logical exclusive or destination with source */
#define S_EOR(dest,source) {	\
	dest ^= source; 			\
	SET_CC(dest);				\
}

/* add source to destination */
#define S_ADD(dest,source) {							\
byte before = dest; 									\
	/* add source; carry only if WC is set */			\
	dest = dest + source + ((S.psl >> 3) & S.psl & C);	\
	S.psl &= ~(C + OVF + IDC);							\
	if (dest < before)									\
		S.psl |= C; 									\
	if ((dest & 15) < (before & 15))					\
		S.psl |= IDC;									\
	SET_CC_OVF(dest,before);							\
}

/* subtract source from destination */
#define S_SUB(dest,source) {									\
byte before = dest; 											\
	/* add source; carry only if WC is set */					\
	dest = dest - source - ((S.psl >> 3) & (S.psl ^ C) & C);	\
	S.psl &= ~(C + OVF + IDC);									\
	if (dest <= before) 										\
		S.psl |= C; 											\
	if ((dest & 15) > (before & 15))							\
		S.psl |= IDC;											\
	SET_CC_OVF(dest,before);									\
}

#define S_COM(reg,val) {			\
int 	d;							\
	S.psl &= ~CC;					\
	if (S.psl & COM)				\
		d = (byte)reg - (byte)val;	\
	else							\
		d = (char)reg - (char)val;	\
	if (d < 0)						\
		S.psl |= 0x80;				\
	else							\
	if (d > 0)						\
		S.psl |= 0x40;				\
}

#define S_DAR(dest) {	\
	if (!(S.psl & IDC)) \
		dest += 0x0a;	\
	if (!(S.psl & C))	\
		dest += 0xa0;	\
}

/* rotate register left */
#define S_RRL(dest) {											\
byte before = dest; 											\
	if (S.psl & WC) 											\
	{															\
	byte c = S.psl & C; 										\
		S.psl &= ~(C + IDC);									\
		dest = (before << 1) | c;								\
		S.psl |= (before >> 7) + (dest & IDC);					\
	} else	dest = (before << 1) | (before >> 7);				\
		SET_CC_OVF(dest,before);								\
}

/* rotate register right */
#define S_RRR(dest) {											\
byte	before = dest;											\
	if (S.psl & WC) 											\
	{															\
	byte c = S.psl & C; 										\
		S.psl &= ~(C + IDC);									\
		dest = (before >> 1) | (c << 7);						\
		S.psl |= (before & C) + (dest & IDC);					\
	} else	dest = (before >> 1) | (before << 7);				\
	SET_CC_OVF(dest,before);									\
}

/* store processor status upper to register 0 */
#define S_SPSU() {	\
	R0 = S.psu & ~PSU34; \
	SET_CC(R0); 	\
}

/* store processor status lower to register 0 */
#define S_SPSL() {	\
	R0 = S.psl; 	\
	SET_CC(R0); 	\
}

/* clear processor status upper, selective */
#define S_CPSU() {			\
byte cpsu = RDOPARG();		\
	S.psu = S.psu & ~cpsu;	\
}

/* clear processor status lower, selective */
#define S_CPSL() {							\
byte cpsl = RDOPARG();						\
		/* select 1st register set now ? */ \
		if ((cpsl & RS) && (S.psl & RS))	\
				SWAP_REGS;					\
		S.psl = S.psl & ~cpsl;				\
}

/* preset processor status upper, selective */
#define S_PPSU() {					\
byte ppsu = RDOPARG() & ~PSU34; 	\
	S.psu = S.psu | ppsu;			\
}

/* preset processor status lower, selective */
#define S_PPSL() {							\
byte ppsl = RDOPARG();						\
	/* select 2nd register set now ? */ 	\
	if ((ppsl & RS) && !(S.psl & RS))		\
		SWAP_REGS;							\
	S.psl = S.psl | ppsl;					\
}

/* test processor status upper */
#define S_TPSU() {				\
byte tpsu = RDOPARG();			\
	S.psl &= ~CC;				\
	if ((S.psu & tpsu) != tpsu) \
		S.psl |= 0x80;			\
}

/* test processor status lower */
#define S_TPSL() {						\
byte tpsl = RDOPARG();					\
	if ((S.psl & tpsl) != tpsl) 		\
		S.psl = (S.psl & ~CC) | 0x80;	\
	else								\
		S.psl &= ~CC;					\
}

/* test under mask immediate */
#define S_TMI(value) {			\
byte	tmi = RDOPARG();		\
	S.psl &= ~CC;				\
	if ((value & tmi) != tmi)	\
		S.psl |= 0x80;			\
}

#ifdef  INLINE_EA
#define REL_EA(page)    _REL_EA(page)
#define ABS_EA()        _ABS_EA()
#define BRA_EA()        _BRA_EA()
#else
static  void    REL_EA(unsigned short page)
        _REL_EA(page)
static  void    ABS_EA(void)
        _ABS_EA()
static  void    BRA_EA(void)
        _BRA_EA()
#endif

void	S2650_SetRegs(S2650_Regs * rgs)
{
	S = *rgs;
}

void	S2650_GetRegs(S2650_Regs * rgs)
{
	*rgs = S;
}

int     S2650_GetPC(void)
{
	return S.page + S.iar;
}

void	S2650_set_flag(int state)
{
	if (state)
		S.psu |= FO;
	else
		S.psu &= ~FO;
}

int 	S2650_get_flag(void)
{
	return (S.psu & FO) ? 1 : 0;
}

void    S2650_set_sense(int state)
{
	if (state)
		S.psu |= SI;
	else
		S.psu &= ~SI;
}

int 	S2650_get_sense(void)
{
	return (S.psu & SI) ? 1 : 0;
}

void    S2650_Reset(void)
{
	memset(&S, 0, sizeof(S));
	S.irq = S2650_INT_NONE;
	S.psl = COM | WC;
	S.psu = SI;
}

void    S2650_Cause_Interrupt(int type)
{

	S.irq = type;			/* set interrupt request (vector) */
#ifdef	VERBOSE
	if (S.irq != S2650_INT_NONE)
	{
	extern FILE * errorlog;
		if (errorlog) fprintf(errorlog, "S2650_Cause_Interrupt $%02x\n", S.irq);
	}
#endif
}

void    S2650_Clear_Pending_Interrupts(void)
{
        S.irq = S2650_INT_NONE; /* clear interrupt request */
}

static  int S2650_Cycles[0x100] = {
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3,
        2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
        2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3
};

int     S2650_Execute(int cycles)
{
	S2650_ICount = cycles;
	do
	{
#ifdef  MAME_DEBUG
{
	extern int mame_debug;
	extern void MAME_Debug(void);
		if (mame_debug) MAME_Debug();
}
#endif
		/* interrupt request and not interrupt inhibit set ? */
		if ((S.irq != S2650_INT_NONE) && !(S.psu & II))
		{
			if (S.halt)
			{
				S.halt = 0;
				S.iar = (S.iar + 1) & PMSK;
			}
			/* build effective address within first 8K page */
			S.ea = S2650_relative[S.irq] & PMSK;
			if (S.irq & 0x80) /* indirect bit set ? */
			{
			int 	addr = S.ea;
				S2650_ICount -= 2;
				/* build indirect 32K address */
				S.ea = RDMEM(addr) << 8;
				if (!(++addr & PMSK))
					addr -= PLEN;
				S.ea = (S.ea + RDMEM(addr)) & AMSK;
			}
#ifdef	VERBOSE
{
extern FILE * errorlog;
			if (errorlog) fprintf(errorlog, "S2650 interrupt to $%04x\n", S.ea);
}
#endif
			S.psu  = (S.psu & ~SP) | ((S.psu + 1) & SP) | II;
			S.ras[S.psu & SP] = S.iar;
			S.page = S.ea & PAGE;
			S.iar  = S.ea & PMSK;
			S.irq  = S2650_INT_NONE;
		}
		S.ir = RDOP();
		S2650_ICount -= S2650_Cycles[S.ir];
		S.r = S.ir & 3; 		/* register / value */
		switch (S.ir)
		{
			case 0x00:		/* LODZ,0 */
			case 0x01:		/* LODZ,1 */
			case 0x02:		/* LODZ,2 */
			case 0x03:		/* LODZ,3 */
				S_LOD(R0,S.reg[S.r]);
				break;

			case 0x04:		/* LODI,0 v */
			case 0x05:		/* LODI,1 v */
			case 0x06:		/* LODI,2 v */
			case 0x07:		/* LODI,3 v */
				S_LOD(S.reg[S.r],RDOPARG());
				break;

			case 0x08:		/* LODR,0 (*)a */
			case 0x09:		/* LODR,1 (*)a */
			case 0x0a:		/* LODR,2 (*)a */
			case 0x0b:		/* LODR,3 (*)a */
				REL_EA(S.page);
				S_LOD(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x0c:		/* LODA,0 (*)a(,X) */
			case 0x0d:		/* LODA,1 (*)a(,X) */
			case 0x0e:		/* LODA,2 (*)a(,X) */
			case 0x0f:		/* LODA,3 (*)a(,X) */
				ABS_EA();
				S_LOD(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x10:		/* illegal */
			case 0x11:		/* illegal */
				break;
			case 0x12:		/* SPSU */
				S_SPSU();
				break;
			case 0x13:		/* SPSL */
				S_SPSL();
				break;

			case 0x14:		/* RETC,0 */
			case 0x15:		/* RETC,1 */
			case 0x16:		/* RETC,2 */
				S_RET((S.psl >> 6) == S.r);
				break;
			case 0x17:		/* RETC,3 */
				S_RET(1);
				break;

			case 0x18:		/* BCTR,0  (*)a */
			case 0x19:		/* BCTR,1  (*)a */
			case 0x1a:		/* BCTR,2  (*)a */
				S_BRR((S.psl >> 6) == S.r);
				break;
			case 0x1b:		/* BCTR,3  (*)a */
				S_BRR(1);
				break;

			case 0x1c:		/* BCTA,0  (*)a */
			case 0x1d:		/* BCTA,1  (*)a */
			case 0x1e:		/* BCTA,2  (*)a */
				S_BRA((S.psl >> 6) == S.r);
				break;
			case 0x1f:		/* BCTA,3  (*)a */
				S_BRA(1);
				break;

			case 0x20:		/* EORZ,0 */
			case 0x21:		/* EORZ,1 */
			case 0x22:		/* EORZ,2 */
			case 0x23:		/* EORZ,3 */
				S_EOR(R0, S.reg[S.r]);
				break;

			case 0x24:		/* EORI,0 v */
			case 0x25:		/* EORI,1 v */
			case 0x26:		/* EORI,2 v */
			case 0x27:		/* EORI,3 v */
				S_EOR(S.reg[S.r],RDOPARG());
				break;

			case 0x28:		/* EORR,0 (*)a */
			case 0x29:		/* EORR,1 (*)a */
			case 0x2a:		/* EORR,2 (*)a */
			case 0x2b:		/* EORR,3 (*)a */
				REL_EA(S.page);
				S_EOR(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x2c:		/* EORA,0 (*)a(,X) */
			case 0x2d:		/* EORA,1 (*)a(,X) */
			case 0x2e:		/* EORA,2 (*)a(,X) */
			case 0x2f:		/* EORA,3 (*)a(,X) */
				ABS_EA();
				S_EOR(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x30:		/* REDC,0 */
			case 0x31:		/* REDC,1 */
			case 0x32:		/* REDC,2 */
			case 0x33:		/* REDC,3 */
				S.reg[S.r] = cpu_readport(S2650_CTRL_PORT);
				SET_CC(S.reg[S.r]);
				break;

			case 0x34:		/* RETE,0 */
			case 0x35:		/* RETE,1 */
			case 0x36:		/* RETE,2 */
				S_RETE((S.psl >> 6) == S.r);
				break;
			case 0x37:		/* RETE,3 */
				S_RETE(1);
				break;

			case 0x38:		/* BSTR,0 (*)a */
			case 0x39:		/* BSTR,1 (*)a */
			case 0x3a:		/* BSTR,2 (*)a */
				S_BSR((S.psl >> 6) == S.r);
				break;
			case 0x3b:		/* BSTR,R3 (*)a */
				S_BSR(1);
				break;

			case 0x3c:		/* BSTA,0 (*)a */
			case 0x3d:		/* BSTA,1 (*)a */
			case 0x3e:		/* BSTA,2 (*)a */
				S_BSA((S.psl >> 6) == S.r);
				break;
			case 0x3f:		/* BSTA,3 (*)a */
				S_BSA(1);
				break;

			case 0x40:		/* HALT */
				S.iar = (S.iar - 1) & PMSK;
				S.halt = 1;
				if (S2650_ICount > 0)
					S2650_ICount = 0;
				break;
			case 0x41:		/* ANDZ,1 */
			case 0x42:		/* ANDZ,2 */
			case 0x43:		/* ANDZ,3 */
				S_AND(R0,S.reg[S.r]);
				break;

			case 0x44:		/* ANDI,0 v */
			case 0x45:		/* ANDI,1 v */
			case 0x46:		/* ANDI,2 v */
			case 0x47:		/* ANDI,3 v */
				S_AND(S.reg[S.r],RDOPARG());
				break;

			case 0x48:		/* ANDR,0 (*)a */
			case 0x49:		/* ANDR,1 (*)a */
			case 0x4a:		/* ANDR,2 (*)a */
			case 0x4b:		/* ANDR,3 (*)a */
				REL_EA(S.page);
				S_AND(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x4c:		/* ANDA,0 (*)a(,X) */
			case 0x4d:		/* ANDA,1 (*)a(,X) */
			case 0x4e:		/* ANDA,2 (*)a(,X) */
			case 0x4f:		/* ANDA,3 (*)a(,X) */
				ABS_EA();
				S_AND(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x50:		/* RRR,0 */
			case 0x51:		/* RRR,1 */
			case 0x52:		/* RRR,2 */
			case 0x53:		/* RRR,3 */
				S_RRR(S.reg[S.r]);
				break;

			case 0x54:		/* REDE,0 v */
			case 0x55:		/* REDE,1 v */
			case 0x56:		/* REDE,2 v */
			case 0x57:		/* REDE,3 v */
				S.reg[S.r] = cpu_readport(RDOPARG());
				SET_CC(S.reg[S.r]);
				break;

			case 0x58:		/* BRNR,0 (*)a */
			case 0x59:		/* BRNR,1 (*)a */
			case 0x5a:		/* BRNR,2 (*)a */
			case 0x5b:		/* BRNR,3 (*)a */
				S_BRR(S.reg[S.r]);
				break;

			case 0x5c:		/* BRNA,0 (*)a */
			case 0x5d:		/* BRNA,1 (*)a */
			case 0x5e:		/* BRNA,2 (*)a */
			case 0x5f:		/* BRNA,3 (*)a */
				S_BRA(S.reg[S.r]);
				break;

			case 0x60:		/* IORZ,0 */
			case 0x61:		/* IORZ,1 */
			case 0x62:		/* IORZ,2 */
			case 0x63:		/* IORZ,3 */
				S_IOR(R0,S.reg[S.r]);
				break;

			case 0x64:		/* IORI,0 v */
			case 0x65:		/* IORI,1 v */
			case 0x66:		/* IORI,2 v */
			case 0x67:		/* IORI,3 v */
				S_IOR(S.reg[S.r],RDOPARG());
				break;

			case 0x68:		/* IORR,0 (*)a */
			case 0x69:		/* IORR,1 (*)a */
			case 0x6a:		/* IORR,2 (*)a */
			case 0x6b:		/* IORR,3 (*)a */
				REL_EA(S.page);
				S_IOR(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x6c:		/* IORA,0 (*)a(,X) */
			case 0x6d:		/* IORA,1 (*)a(,X) */
			case 0x6e:		/* IORA,2 (*)a(,X) */
			case 0x6f:		/* IORA,3 (*)a(,X) */
				ABS_EA();
				S_IOR(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x70:		/* REDD,0 */
			case 0x71:		/* REDD,1 */
			case 0x72:		/* REDD,2 */
			case 0x73:		/* REDD,3 */
				S.reg[S.r] = cpu_readport(S2650_DATA_PORT);
				SET_CC(S.reg[S.r]);
				break;

			case 0x74:		/* CPSU */
				S_CPSU();
				break;
			case 0x75:		/* CPSL */
				S_CPSL();
				break;
			case 0x76:		/* PPSU */
				S_PPSU();
				break;
			case 0x77:		/* PPSL */
				S_PPSL();
				break;

			case 0x78:		/* BSNR,0 (*)a */
			case 0x79:		/* BSNR,1 (*)a */
			case 0x7a:		/* BSNR,2 (*)a */
			case 0x7b:		/* BSNR,3 (*)a */
				S_BSR(S.reg[S.r]);
				break;

			case 0x7c:		/* BSNA,0 (*)a */
			case 0x7d:		/* BSNA,1 (*)a */
			case 0x7e:		/* BSNA,2 (*)a */
			case 0x7f:		/* BSNA,3 (*)a */
				S_BSA(S.reg[S.r]);
				break;

			case 0x80:		/* ADDZ,0 */
			case 0x81:		/* ADDZ,1 */
			case 0x82:		/* ADDZ,2 */
			case 0x83:		/* ADDZ,3 */
				S_ADD(R0,S.reg[S.r]);
				break;

			case 0x84:		/* ADDI,0 v */
			case 0x85:		/* ADDI,1 v */
			case 0x86:		/* ADDI,2 v */
			case 0x87:		/* ADDI,3 v */
				S_ADD(S.reg[S.r],RDOPARG());
				break;

			case 0x88:		/* ADDR,0 (*)a */
			case 0x89:		/* ADDR,1 (*)a */
			case 0x8a:		/* ADDR,2 (*)a */
			case 0x8b:		/* ADDR,3 (*)a */
				REL_EA(S.page);
				S_ADD(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x8c:		/* ADDA,0 (*)a(,X) */
			case 0x8d:		/* ADDA,1 (*)a(,X) */
			case 0x8e:		/* ADDA,2 (*)a(,X) */
			case 0x8f:		/* ADDA,3 (*)a(,X) */
				ABS_EA();
				S_ADD(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0x90:		/* illegal */
			case 0x91:		/* illegal */
				break;
			case 0x92:		/* LPSU */
				S.psu = R0 & ~PSU34;
				break;
			case 0x93:		/* LPSL */
				/* change register set ? */
				if ((S.psl ^ R0) & RS)
					SWAP_REGS;
				S.psl = R0;
				break;

			case 0x94:		/* DAR,0 */
			case 0x95:		/* DAR,1 */
			case 0x96:		/* DAR,2 */
			case 0x97:		/* DAR,3 */
				S_DAR(S.reg[S.r]);
				break;

			case 0x98:		/* BCFR,0 (*)a */
			case 0x99:		/* BCFR,1 (*)a */
			case 0x9a:		/* BCFR,2 (*)a */
				S_BRR((S.psl >> 6) != S.r);
				break;
			case 0x9b:		/* ZBRR    (*)a */
				S_ZBRR();
				break;

			case 0x9c:		/* BCFA,0 (*)a */
			case 0x9d:		/* BCFA,1 (*)a */
			case 0x9e:		/* BCFA,2 (*)a */
				S_BRA((S.psl >> 6) != S.r);
				break;
			case 0x9f:		/* BXA	   (*)a */
				S_BXA();
				break;

			case 0xa0:		/* SUBZ,0 */
			case 0xa1:		/* SUBZ,1 */
			case 0xa2:		/* SUBZ,2 */
			case 0xa3:		/* SUBZ,3 */
				S_SUB(R0,S.reg[S.r]);
				break;

			case 0xa4:		/* SUBI,0 v */
			case 0xa5:		/* SUBI,1 v */
			case 0xa6:		/* SUBI,2 v */
			case 0xa7:		/* SUBI,3 v */
				S_SUB(S.reg[S.r],RDOPARG());
				break;

			case 0xa8:		/* SUBR,0 (*)a */
			case 0xa9:		/* SUBR,1 (*)a */
			case 0xaa:		/* SUBR,2 (*)a */
			case 0xab:		/* SUBR,3 (*)a */
				REL_EA(S.page);
				S_SUB(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0xac:		/* SUBA,0 (*)a(,X) */
			case 0xad:		/* SUBA,1 (*)a(,X) */
			case 0xae:		/* SUBA,2 (*)a(,X) */
			case 0xaf:		/* SUBA,3 (*)a(,X) */
				ABS_EA();
				S_SUB(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0xb0:		/* WRTC,0 */
			case 0xb1:		/* WRTC,1 */
			case 0xb2:		/* WRTC,2 */
			case 0xb3:		/* WRTC,3 */
				cpu_writeport(S2650_CTRL_PORT,S.reg[S.r]);
				break;

			case 0xb4:		/* TPSU */
				S_TPSU();
				break;
			case 0xb5:		/* TPSL */
				S_TPSL();
				break;
			case 0xb6:		/* illegal */
			case 0xb7:		/* illegal */
				break;

			case 0xb8:		/* BSFR,0 (*)a */
			case 0xb9:		/* BSFR,1 (*)a */
			case 0xba:		/* BSFR,2 (*)a */
				S_BSR((S.psl >> 6) != S.r);
				break;
			case 0xbb:		/* ZBSR    (*)a */
				S_ZBSR();
				break;

			case 0xbc:		/* BSFA,0 (*)a */
			case 0xbd:		/* BSFA,1 (*)a */
			case 0xbe:		/* BSFA,2 (*)a */
				S_BSA((S.psl >> 6) != S.r);
				break;
			case 0xbf:		/* BSXA    (*)a */
				S_BSXA();
				break;

			case 0xc0:		/* NOP */
				break;
			case 0xc1:		/* STRZ,1 */
			case 0xc2:		/* STRZ,2 */
			case 0xc3:		/* STRZ,3 */
				S_LOD(S.reg[S.r],R0);
				break;

			case 0xc4:		/* illegal */
			case 0xc5:		/* illegal */
			case 0xc6:		/* illegal */
			case 0xc7:		/* illegal */
				break;

			case 0xc8:		/* STRR,0 (*)a */
			case 0xc9:		/* STRR,1 (*)a */
			case 0xca:		/* STRR,2 (*)a */
			case 0xcb:		/* STRR,3 (*)a */
				REL_EA(S.page);
				S_STR(S.ea, S.reg[S.r]);
				break;

			case 0xcc:		/* STRA,0 (*)a(,X) */
			case 0xcd:		/* STRA,1 (*)a(,X) */
			case 0xce:		/* STRA,2 (*)a(,X) */
			case 0xcf:		/* STRA,3 (*)a(,X) */
				ABS_EA();
				S_STR(S.ea, S.reg[S.r]);
				break;

			case 0xd0:		/* RRL,0 */
			case 0xd1:		/* RRL,1 */
			case 0xd2:		/* RRL,2 */
			case 0xd3:		/* RRL,3 */
				S_RRL(S.reg[S.r]);
				break;

			case 0xd4:		/* WRTE,0 v */
			case 0xd5:		/* WRTE,1 v */
			case 0xd6:		/* WRTE,2 v */
			case 0xd7:		/* WRTE,3 v */
				cpu_writeport(RDOPARG(), S.reg[S.r]);
				break;

			case 0xd8:		/* BIRR,0 (*)a */
			case 0xd9:		/* BIRR,1 (*)a */
			case 0xda:		/* BIRR,2 (*)a */
			case 0xdb:		/* BIRR,3 (*)a */
				S_BRR(++S.reg[S.r]);
				break;

			case 0xdc:		/* BIRA,0 (*)a */
			case 0xdd:		/* BIRA,1 (*)a */
			case 0xde:		/* BIRA,2 (*)a */
			case 0xdf:		/* BIRA,3 (*)a */
				S_BRA(++S.reg[S.r]);
				break;

			case 0xe0:		/* COMZ,0 */
			case 0xe1:		/* COMZ,1 */
			case 0xe2:		/* COMZ,2 */
			case 0xe3:		/* COMZ,3 */
				S_COM(R0,S.reg[S.r]);
				break;

			case 0xe4:		/* COMI,0 v */
			case 0xe5:		/* COMI,1 v */
			case 0xe6:		/* COMI,2 v */
			case 0xe7:		/* COMI,3 v */
				S_COM(S.reg[S.r],RDOPARG());
				break;

			case 0xe8:		/* COMR,0 (*)a */
			case 0xe9:		/* COMR,1 (*)a */
			case 0xea:		/* COMR,2 (*)a */
			case 0xeb:		/* COMR,3 (*)a */
				REL_EA(S.page);
				S_COM(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0xec:		/* COMA,0 (*)a(,X) */
			case 0xed:		/* COMA,1 (*)a(,X) */
			case 0xee:		/* COMA,2 (*)a(,X) */
			case 0xef:		/* COMA,3 (*)a(,X) */
				ABS_EA();
				S_COM(S.reg[S.r],RDMEM(S.ea));
				break;

			case 0xf0:		/* WRTD,0 */
			case 0xf1:		/* WRTD,1 */
			case 0xf2:		/* WRTD,2 */
			case 0xf3:		/* WRTD,3 */
				cpu_writeport(S2650_DATA_PORT, S.reg[S.r]);
				break;

			case 0xf4:		/* TMI,0  v */
			case 0xf5:		/* TMI,1  v */
			case 0xf6:		/* TMI,2  v */
			case 0xf7:		/* TMI,3  v */
				S_TMI(S.reg[S.r]);
				break;

			case 0xf8:		/* BDRR,0 (*)a */
			case 0xf9:		/* BDRR,1 (*)a */
			case 0xfa:		/* BDRR,2 (*)a */
			case 0xfb:		/* BDRR,3 (*)a */
				S_BRR(--S.reg[S.r]);
				break;

			case 0xfc:		/* BDRA,0 (*)a */
			case 0xfd:		/* BDRA,1 (*)a */
			case 0xfe:		/* BDRA,2 (*)a */
			case 0xff:		/* BDRA,3 (*)a */
				S_BRA(--S.reg[S.r]);
				break;
		}
	} while (S2650_ICount > 0);
	return cycles - S2650_ICount;
}

