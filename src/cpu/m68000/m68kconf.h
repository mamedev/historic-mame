#ifndef M68KCONF__HEADER
#define M68KCONF__HEADER

#ifndef A68KEM

/* ======================================================================== */
/* ============================= CONFIGURATION ============================ */
/* ======================================================================== */

/* Configuration switches.  1 = ON, 0 = OFF */

/* If on, CPU will call m68000_interrupt_acknowledge() when it services an
 * interrupt.
 * If off, all interrupts will be autovectored.
 */
#define M68000_INT_ACK       0

/* If on, CPU will call m68000_breakpoint_acknowledge() when it encounters
 * a breakpoint instruction and it is running in 68010+ mode.
 */
#define M68000_BKPT_ACK      0

/* If on, the CPU will monitor the trace flags and take trace exceptions
 */
#define M68000_TRACE   0

/* If on, CPU will actually halt when m68000_input_halt is called by the host.
 * I allow it to be disabled for a very slight speed increase.
 */
#define M68000_HALT    0

/* If on, CPU will call m68000_output_reset() when it encounters a reset
 * instruction.
 */
#define M68000_OUTPUT_RESET  0

/* If on, CPU will call m68000_setting_pc() when it changes the PC by a large
 * value.  This allows host programs to be nicer when it comes to fetching
 * immediate data and instructions on a banked memory system.
 */
#define M68000_SETTING_PC    0

/* If on, CPU will call m68000_changing_fc() on every memory access to
 * differentiate between user/supervisor, program/data access like a real
 * 68000 would.  This should be enabled and m68000_changing_fc() should be
 * handled if you are going to emulate the m68010. (moves uses function codes
 * to read/write data from different address spaces)
 */
#define M68000_CHANGING_FC   0

/* If on, CPU will call m68000_debug_hook() before every instruction. */
#define M68000_DEBUG_HOOK    0

/* If on, CPU will call m68000_peek_pc_hook() before every instruction. */
#define M68000_PEEK_PC_HOOK  0

/* Set to your compiler's static inline keyword.  If your compiler doesn't
 * have inline, just set this to static.
 * If you define INLINE in the makefile, it will override this value.
 */
#ifndef INLINE
#define INLINE static __inline__
#endif


/* Turn this on if you are building MAME.
 * It will override the configuration section.
 */
#define M68000_BUILDING_MAME   1



/* ======================================================================== */
/* ============================== MAME STUFF ============================== */
/* ======================================================================== */

#if M68000_BUILDING_MAME

/* Things to make it work better with MAME */

#include "cpuintrf.h"

#undef M68000_INT_ACK
#undef M68000_BKPT_ACK
#undef M68000_ALLOW_TRACE
#undef M68000_ALLOW_HALT
#undef M68000_OUTPUT_RESET
#undef M68000_SETTING_PC
#undef M68000_CHANGING_FC
#undef M68000_DEBUG_HOOK
#undef M68000_PEEK_PC_HOOK
#define M68000_INT_ACK		 1
#define M68000_BKPT_ACK      0
#define M68000_ALLOW_TRACE   0
#define M68000_ALLOW_HALT    0
#define M68000_OUTPUT_RESET  0
#define M68000_SETTING_PC    1
#define M68000_CHANGING_FC   0 /* MAME needs a way to handle function codes */
#define M68000_PEEK_PC_HOOK  1


extern int m68000_clks_left;

extern int previouspc;
#define m68000_peek_pc_hook() previouspc = g_cpu_pc;

#ifdef MAME_DEBUG
#define M68000_DEBUG_HOOK 1
extern int mame_debug;
extern void MAME_Debug(void);
#define m68000_debug_hook()                    if(mame_debug) MAME_Debug()
#else
#define M68000_DEBUG_HOOK 0
#endif /* MAME_DEBUG */

#include "memory.h"
#define m68000_read_memory_8(address)          cpu_readmem24(address)
#define m68000_read_memory_16(address)         cpu_readmem24_word(address)
#define m68000_read_memory_32(address)         cpu_readmem24_dword(address)

#define m68000_read_immediate_8(address)       (cpu_readop16((address)-1)&0xff)
#define m68000_read_immediate_16(address)      cpu_readop16(address)
#define m68000_read_immediate_32(address)      ((cpu_readop16(address)<<16) | cpu_readop16((address)+2))

#define m68000_read_instruction(address)       cpu_readop16(address)


#define m68000_write_memory_8(address, value)  cpu_writemem24(address, value)
#define m68000_write_memory_16(address, value) cpu_writemem24_word(address, value)
#define m68000_write_memory_32(address, value) cpu_writemem24_dword(address, value)

#define m68000_setting_pc(address)             change_pc24((address)&0xffffff)


typedef struct {
	m68000_cpu_context regs;
#if NEW_INTERRUPT_SYSTEM
	int (*irq_callback)(int irqline);
#endif /* NEW_INTERRUPT_SYSTEM */
}	MC68000_Regs;


#define MC68000_ICount   m68000_clks_left

#define MC68000_Reset    m68000_pulse_reset
#define MC68000_GetPC    m68000_peek_pc
#define MC68000_Execute  m68000_execute

int m68000_disassemble(char* str_buff, int pc);

#define Dasm68000        m68000_disassemble

#if NEW_INTERRUPT_SYSTEM

void MC68000_SetRegs(MC68000_Regs *regs);
void MC68000_GetRegs(MC68000_Regs *regs);
void MC68000_set_nmi_line(int state);
void MC68000_set_irq_line(int irqline, int state);
void MC68000_set_irq_callback(int (*callback)(int irqline));

#else

#define MC68000_SetRegs(R)     m68000_set_context(&R->regs)
#define MC68000_GetRegs(R)     m68000_get_context(&R->regs)
#define MC68000_CauseInterupt  m68000_pulse_irq
void MC68000_ClearPendingInterrupts(void);

#endif /* NEW_INTERRUPT_SYSTEM */


#endif /* M68000_BUILDING_MAME */

#else
/* ======================================================================== */
/* =========================== ASSEMBLER INTERFACE ======================== */
/* ======================================================================== */

#define Dasm68000		 m68000_disassemble

typedef struct {

		m68000_cpu_context regs;

}	MC68000_Regs;

extern m68000_cpu_context regs;

void MC68000_SetRegs(MC68000_Regs *src);
void MC68000_GetRegs(MC68000_Regs *dst);
void MC68000_Reset(void);
int  MC68000_Execute(int cycles);
int  MC68000_GetPC(void);

extern int MC68000_ICount;

#if NEW_INTERRUPT_SYSTEM

void MC68000_set_nmi_line(int state);
void MC68000_set_irq_line(int irqline, int state);
void MC68000_set_irq_callback(int (*callback)(int irqline));

#else

void MC68000_Cause_Interrupt(int level)
void MC68000_ClearPendingInterrupts(void);

#endif /* NEW_INTERRUPT_SYSTEM */


#endif

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68KCONF__HEADER */
