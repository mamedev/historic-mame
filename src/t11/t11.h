/*** T-11: Portable DEC T-11 emulator ******************************************/

#ifndef _T11_H
#define _T11_H

#include "memory.h"

/****************************************************************************/
/* sizeof(char)=1, sizeof(short)=2, sizeof(long)>=4                         */
/****************************************************************************/

typedef union {
#ifdef LSB_FIRST
	struct {unsigned char l,h,h2,h3;} b;
	struct {unsigned short l,h; } w;
	unsigned long d;
#else
	struct {unsigned char h3,h2,h,l;} b;
	struct {unsigned short h,l;} w;
	unsigned long d;
#endif
} t11_Register;

/* T-11 Registers */
typedef struct
{
	t11_Register reg[8];
	t11_Register psw;
	int  op;
	int  pending_interrupts;
	unsigned char *bank[8];
} t11_Regs;

#ifndef INLINE
#define INLINE static inline
#endif

#define T11_INT_NONE    -1      /* No interrupt requested */
#define T11_IRQ0        0      /* IRQ0 */
#define T11_IRQ1        1      /* IRQ0 */
#define T11_IRQ2        2      /* IRQ0 */
#define T11_IRQ3        3      /* IRQ0 */

#define T11_RESERVED    0x000   /* Reserved vector */
#define T11_TIMEOUT     0x004   /* Time-out/system error vector */
#define T11_ILLINST     0x008   /* Illegal and reserved instruction vector */
#define T11_BPT         0x00C   /* BPT instruction vector */
#define T11_IOT         0x010   /* IOT instruction vector */
#define T11_PWRFAIL     0x014   /* Power fail vector */
#define T11_EMT         0x018   /* EMT instruction vector */
#define T11_TRAP        0x01C   /* TRAP instruction vector */

/* PUBLIC FUNCTIONS */
void t11_SetRegs(t11_Regs *Regs);
void t11_GetRegs(t11_Regs *Regs);
unsigned t11_GetPC(void);
void t11_reset(void);
int t11_execute(int cycles);	/* NS 970908 */
void t11_Cause_Interrupt(int type);	/* NS 970908 */
void t11_Clear_Pending_Interrupts(void);	/* NS 970908 */

void t11_SetBank(int banknum, unsigned char *address);

/* PUBLIC GLOBALS */
extern int	t11_ICount;


/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
#define T11_RDMEM(A) ((unsigned)cpu_readmem16lew(A))
#define T11_RDMEM_WORD(A) ((unsigned)cpu_readmem16lew_word(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define T11_WRMEM(A,V) (cpu_writemem16lew(A,V))
#define T11_WRMEM_WORD(A,V) (cpu_writemem16lew_word(A,V))

#endif /* _T11_H */
