#ifndef E132XS_H
#define E132XS_H

/* Functions */
extern void e132xs_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned dasm_e132xs(char *buffer, unsigned pc, unsigned h_flag);
#endif


/* Memory access */
/* read byte */
#define READ_B(addr)           (program_read_byte_32be(addr))
/* read half-word */
#define READ_HW(addr)          (program_read_word_32be((addr) & ~1))
/* read word */
#define READ_W(addr)           (program_read_dword_32be((addr) & ~3))

/* write byte */
#define WRITE_B(addr, data)    (program_write_byte_32be(addr, data))
/* write half-word */
#define WRITE_HW(addr, data)   (program_write_word_32be((addr) & ~1, data))
/* write word */
#define WRITE_W(addr, data)    (program_write_dword_32be((addr) & ~3, data))


/* I/O access */
/* read word */
#define IO_READ_W(addr)        (io_read_dword_32be(((addr) >> 11) & 0x7ffc))
/* write word */
#define IO_WRITE_W(addr, data) (io_write_dword_32be(((addr) >> 11) & 0x7ffc, data))


#define READ_OP(addr)	       READ_HW(addr)


/* Registers Number	*/
#define PC_REGISTER			 0
#define SR_REGISTER			 1
#define BCR_REGISTER		20
#define TPR_REGISTER		21
#define ISR_REGISTER		25
#define FCR_REGISTER		26
#define MCR_REGISTER		27

#define X_CODE(val)		 ((val & 0x7000) >> 12)
#define E_BIT(val)		 ((val & 0x8000) >> 15)
#define S_BIT_CONST(val) ((val & 0x4000) >> 14)
#define DD(val)			 ((val & 0x3000) >> 12)


/* Extended DSP instructions */
#define EMUL			0x102
#define EMULU			0x104
#define EMULS			0x106
#define EMAC			0x10a
#define EMACD			0x10e
#define EMSUB			0x11a
#define EMSUBD			0x11e
#define EHMAC			0x02a
#define EHMACD			0x02e
#define EHCMULD			0x046
#define EHCMACD			0x04e
#define EHCSUMD			0x086
#define EHCFFTD			0x096
#define EHCFFTSD		0x296

/* Delay values */
#define NO_DELAY		0
#define DELAY_EXECUTE	1
#define DELAY_TAKEN		2

/* Trap numbers */
#define IO2					48
#define IO1					49
#define INT4				50
#define INT3				51
#define INT2				52
#define INT1				53
#define IO3					54
#define TIMER				55
#define RESERVED1			56
#define TRACE_EXCEPTION		57
#define PARITY_ERROR		58
#define EXTENDED_OVERFLOW	59
#define RANGE_ERROR			60
#define PRIVILEGE_ERROR		RANGE_ERROR
#define FRAME_ERROR			RANGE_ERROR
#define RESERVED2			61
#define RESET				62	// reserved if not mapped @ MEM3
#define ERROR_ENTRY			63	// for instruction code of all ones

/* Traps code */
#define	TRAPLE		4
#define	TRAPGT		5
#define	TRAPLT		6
#define	TRAPGE		7
#define	TRAPSE		8
#define	TRAPHT		9
#define	TRAPST		10
#define	TRAPHE		11
#define	TRAPE		12
#define	TRAPNE		13
#define	TRAPV		14
#define	TRAP		15

/* Entry point to get trap locations or emulated code associated */
#define	E132XS_ENTRY_MEM0	0
#define	E132XS_ENTRY_MEM1	1
#define	E132XS_ENTRY_MEM2	2
#define	E132XS_ENTRY_IRAM	3
#define	E132XS_ENTRY_MEM3	7

#endif /* E132XS_H */
