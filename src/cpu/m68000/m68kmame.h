#ifndef M68KMAME__HEADER
#define M68KMAME__HEADER

/* ======================================================================== */
/* ============================== MAME STUFF ============================== */
/* ======================================================================== */

#include "cpuintrf.h"
#include "memory.h"
#include "mamedbg.h"

/* Configuration switches (see m68kconf.h for explanation) */
#define M68K_SEPARATE_READ_IMM      OPT_ON

#define M68K_EMULATE_INT_ACK        OPT_ON
#define M68K_INT_ACK_CALLBACK(A)

#define M68K_EMULATE_BKPT_ACK       OPT_OFF
#define M68K_BKPT_ACK_CALLBACK()

#define M68K_EMULATE_TRACE          OPT_OFF

#define M68K_EMULATE_RESET          OPT_OFF
#define M68K_RESET_CALLBACK()

#define M68K_EMULATE_FC             OPT_OFF
#define M68K_SET_FC_CALLBACK(A)

#define M68K_MONITOR_PC             OPT_SPECIFY_HANDLER
#define M68K_SET_PC_CALLBACK(A)     (*m68k_memory_intf.changepc)(A)

#define M68K_INSTRUCTION_HOOK       OPT_SPECIFY_HANDLER
#define M68K_INSTRUCTION_CALLBACK() CALL_MAME_DEBUG

#define M68K_EMULATE_PREFETCH       OPT_ON

#define M68K_LOG_ENABLE             OPT_OFF
#define M68K_LOG_1010_1111          OPT_OFF
#define M68K_LOG_FILEHANDLE         errorlog

#define M68K_EMULATE_ADDRESS_ERROR  OPT_OFF

#define M68K_USE_64_BIT             OPT_OFF


/* Redirect memory calls */
struct m68k_memory_interface
{
	offs_t		opcode_xor;
	data8_t		(*read8)(offs_t);
	data16_t	(*read16)(offs_t);
	data32_t	(*read32)(offs_t);
	void		(*write8)(offs_t, data8_t);
	void		(*write16)(offs_t, data16_t);
	void		(*write32)(offs_t, data32_t);
	void		(*changepc)(offs_t);
};
extern struct m68k_memory_interface m68k_memory_intf;

#define m68k_read_memory_8(address)          (*m68k_memory_intf.read8)(address)
#define m68k_read_memory_16(address)         (*m68k_memory_intf.read16)(address)
#define m68k_read_memory_32(address)         (*m68k_memory_intf.read32)(address)

#define m68k_read_immediate_16(address)      cpu_readop_arg16((address) ^ m68k_memory_intf.opcode_xor)
#define m68k_read_immediate_32(address)		 ((m68k_read_immediate_16(address) << 16) | m68k_read_immediate_16((address)+2))

#define m68k_read_disassembler_8(address)    (*m68k_memory_intf.read8)(address)
#define m68k_read_disassembler_16(address)   (*m68k_memory_intf.read16)(address)
#define m68k_read_disassembler_32(address)   (*m68k_memory_intf.read32)(address)


#define m68k_write_memory_8(address, value)  (*m68k_memory_intf.write8)(address, value)
#define m68k_write_memory_16(address, value) (*m68k_memory_intf.write16)(address, value)
#define m68k_write_memory_32(address, value) (*m68k_memory_intf.write32)(address, value)


/* Redirect ICount */
#define m68000_ICount m68ki_remaining_cycles


/* M68K Variants */
#if HAS_M68010
#define M68K_EMULATE_010            OPT_ON
#else
#define M68K_EMULATE_010            OPT_OFF
#endif

#if HAS_M68EC020
#define M68K_EMULATE_EC020          OPT_ON
#else
#define M68K_EMULATE_EC020          OPT_OFF
#endif

#if HAS_M68020
#define M68K_EMULATE_020            OPT_ON
#else
#define M68K_EMULATE_020            OPT_OFF
#endif

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68KMAME__HEADER */
