#ifndef _UPD7810_H_
#define _UPD7810_H_

enum {
	UPD7810_PC=1, UPD7810_SP, UPD7810_PSW,
	UPD7810_EA, UPD7810_V, UPD7810_A, UPD7810_VA,
	UPD7810_BC, UPD7810_B, UPD7810_C, UPD7810_DE, UPD7810_D, UPD7810_E, UPD7810_HL, UPD7810_H, UPD7810_L,
	UPD7810_EA2, UPD7810_V2, UPD7810_A2, UPD7810_VA2,
	UPD7810_BC2, UPD7810_B2, UPD7810_C2, UPD7810_DE2, UPD7810_D2, UPD7810_E2, UPD7810_HL2, UPD7810_H2, UPD7810_L2,
	UPD7810_CNT0, UPD7810_CNT1, UPD7810_TM0, UPD7810_TM1, UPD7810_ECNT, UPD7810_ECPT, UPD7810_ETM0, UPD7810_ETM1,
	UPD7810_MA, UPD7810_MB, UPD7810_MCC, UPD7810_MC, UPD7810_MM, UPD7810_MF,
	UPD7810_TMM, UPD7810_ETMM, UPD7810_EOM, UPD7810_SML, UPD7810_SMH,
	UPD7810_ANM, UPD7810_MKL, UPD7810_MKH, UPD7810_ZCM,
	UPD7810_TXB, UPD7810_RXB, UPD7810_CR0, UPD7810_CR1, UPD7810_CR2, UPD7810_CR3,
	UPD7810_TXD, UPD7810_RXD, UPD7810_SCK, UPD7810_TI, UPD7810_TO, UPD7810_CI, UPD7810_CO0, UPD7810_CO1
};

/* port numbers for PA,PB,PC,PD and PF */
enum {
	UPD7810_PORTA, UPD7810_PORTB, UPD7810_PORTC, UPD7810_PORTD, UPD7810_PORTF
};

/* Supply an instance of this function in your driver code:
 * It will be called whenever an output signal changes or a new
 * input line state is to be sampled.
 */
typedef int (*upd7810_io_callback)(int ioline, int state);

#define UPD7810_INT_NONE	-1
#define UPD7810_INTNMI		0
#define UPD7810_INTF1		1
#define UPD7810_INTF2		2

extern int upd7810_icount;						/* cycle count */

extern void upd7810_init (void);				/* Initialize save states */
extern void upd7810_reset (void *param);		/* Reset registers to the initial values */
extern void upd7810_exit  (void);				/* Shut down CPU core */
extern int	upd7810_execute(int cycles);		/* Execute cycles - returns number of cycles actually run */
extern unsigned upd7810_get_context (void *dst);/* Get registers, return context size */
extern void upd7810_set_context (void *src);	/* Set registers */
extern unsigned upd7810_get_pc (void);			/* Get program counter */
extern void upd7810_set_pc (unsigned val);		/* Set program counter */
extern unsigned upd7810_get_sp (void);			/* Get stack pointer */
extern void upd7810_set_sp (unsigned val);		/* Set stack pointer */
extern unsigned upd7810_get_reg (int regnum);
extern void upd7810_set_reg (int regnum, unsigned val);
extern void upd7810_set_nmi_line(int state);
extern void upd7810_set_irq_line(int irqline, int state);
extern void upd7810_set_irq_callback(int (*callback)(int irqline));
extern const char *upd7810_info(void *context, int regnum);
extern unsigned upd7810_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm7810( char *dst, unsigned pc );
#endif

#endif

