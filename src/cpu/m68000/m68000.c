/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */

static const char* copyright_notice =
"MUSASHI\n"
"Version 1.1\n"
"A portable Motorola M680x0 processor emulation engine.\n"
"Copyright 1999 Karl Stenerud.  All rights reserved.\n"
"\n"
"This code may be freely used for non-commercial purpooses as long as this\n"
"copyright notice remains unaltered in the source code and any binary files\n"
"containing this code in compiled form.\n"
"\n"
"Any commercial ventures wishing to use this code must contact the author\n"
"(Karl Stenerud) to negotiate commercial licensing terms.\n"
"\n"
"The latest version of this code can be obtained at:\n"
"http://milliways.scas.bcit.bc.ca/~karl/musashi\n"
;


/* ======================================================================== */
/* ================================= NOTES ================================ */
/* ======================================================================== */
/*
why does m68000 book allow immediate addressing mode for btst?
should I implement address error for odd branch, jsr, etc?
*/


/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include "m68000.h"

/* ======================================================================== */
/* ==================== ARCHITECTURE-DEPENDANT DEFINES ==================== */
/* ======================================================================== */

#if UCHAR_MAX == 0xff
#define M68000_HAS_8_BIT_SIZE  1
#else
#define M68000_HAS_8_BIT_SIZE  0
#endif /* UCHAR_MAX == 0xff */

#if USHRT_MAX == 0xffff
#define M68000_HAS_16_BIT_SIZE 1
#else
#define M68000_HAS_16_BIT_SIZE 0
#endif /* USHRT_MAX == 0xffff */

#if ULONG_MAX == 0xffffffff
#define M68000_HAS_32_BIT_SIZE 1
#else
#define M68000_HAS_32_BIT_SIZE 0
#endif /* ULONG_MAX == 0xffffffff */

#if UINT_MAX > 0xffffffff
#define M68000_OVER_32_BIT     1
#else
#define M68000_OVER_32_BIT     0
#endif /* UINT_MAX > 0xffffffff */

#ifndef QSORT_CALLBACK_DECL
#define QSORT_CALLBACK_DECL
#endif

#undef int8
#undef int16
#undef int32
#undef uint
#undef uint8
#undef uint16
#undef uint

#define int8   INT8
#define uint8  UINT8
#define int16  INT16
#define uint16 UINT16
#define int32  INT32

/* int and unsigned int must be at least 32 bits wide */
#define uint   unsigned int


/* Allow for architectures that don't have 8-bit sizes */
#if M68000_HAS_8_BIT_SIZE
#define make_int_8(value) (int8)(value)
#else
#undef  int8
#define int8   int
#undef  uint8
#define uint8  unsigned int
INLINE int make_int_8(int value)
{
   /* Will this work for 1's complement machines? */
   return (value & 0x80) ? value | ~0xff : value & 0xff;
}
#endif /* M68000_HAS_8_BIT_SIZE */


/* Allow for architectures that don't have 16-bit sizes */
#if M68000_HAS_16_BIT_SIZE
#define make_int_16(value) (int16)(value)
#else
#undef  int16
#define int16  int
#undef  uint16
#define uint16 unsigned int
INLINE int make_int_16(int value)
{
   /* Will this work for 1's complement machines? */
   return (value & 0x8000) ? value | ~0xffff : value & 0xffff;
}
#endif /* M68000_HAS_16_BIT_SIZE */


/* Allow for architectures that don't have 32-bit sizes */
#if M68000_HAS_32_BIT_SIZE
#define make_int_32(value) (int32)(value)
#else
#undef  int32
#define int32  int
INLINE int make_int_32(int value)
{
   /* Will this work for 1's complement machines? */
   return (value & 0x80000000) ? value | ~0xffffffff : value & 0xffffffff;
}
#endif /* M68000_HAS_32_BIT_SIZE */


/* Some 64-bit optimizations */
#if M68000_OVER_32_BIT
#define LSR_32(VALUE, SHIFT) ((VALUE) >> (SHIFT))
#define LSL_32(VALUE, SHIFT) ((VALUE) << (SHIFT))
#else
#define LSR_32(VALUE, SHIFT) (((VALUE) >> 1) >> ((SHIFT)-1))
#define LSL_32(VALUE, SHIFT) (((VALUE) << 1) << ((SHIFT)-1))
#endif /* M68000_OVER_32_BIT */


/* ======================================================================== */
/* ========================= CONFIGURATION DEFINES ======================== */
/* ======================================================================== */

/* Act on values in m68kconf.h */
#if !M68000_OUTPUT_RESET
#define m68000_output_reset()
#endif /* !M68000_OUTPUT_RESET */

#if !M68000_SETTING_PC
#define m68000_setting_pc(A) g_cpu_pc = (A)
#endif /* !M68000_SETTING_PC */

#if !M68000_CHANGING_FC
#define m68000_changing_fc(A)
#endif /* !M68000_CHANGING_FC */

#if !M68000_DEBUG_HOOK
#define m68000_debug_hook()
#endif /* M68000_DEBUG_HOOK */

#if !M68000_PEEK_PC_HOOK
#define m68000_peek_pc_hook()
#endif /* M68000_DEBUG_HOOK */

#if !M68000_INT_ACK
#define m68000_interrupt_acknowledge(A) -1
#endif /* M68000_INT_ACK */

#if !M68000_BKPT_ACK
#define m68000_breakpoint_acknowledge(A)
#endif /* M68000_BKPT_ACK */



/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */
/* ======================================================================== */

/* Exception Vectors handled by emulation */
#define EXCEPTION_ILLEGAL_INSTRUCTION      4
#define EXCEPTION_ZERO_DIVIDE              5
#define EXCEPTION_CHK                      6
#define EXCEPTION_TRAPV                    7
#define EXCEPTION_PRIVILEGE_VIOLATION      8
#define EXCEPTION_TRACE                    9
#define EXCEPTION_1010                    10
#define EXCEPTION_1111                    11
#define EXCEPTION_FORMAT_ERROR            14
#define EXCEPTION_UNINITIALIZED_INTERRUPT 15
#define EXCEPTION_SPURIOUS_INTERRUPT      24
#define EXCEPTION_INTERRUPT_AUTOVECTOR    24
#define EXCEPTION_TRAP_BASE               32


#define FUNCTION_CODE_USER_DATA          1
#define FUNCTION_CODE_USER_PROGRAM       2
#define FUNCTION_CODE_SUPERVISOR_DATA    5
#define FUNCTION_CODE_SUPERVISOR_PROGRAM 6
#define FUNCTION_CODE_CPU_SPACE          7

/* CPU types for deciding what to emulate */
#define CPU_000 M68000_CPU_000
#define CPU_010 M68000_CPU_010

#define CPU_ALL      CPU_000 | CPU_010
#define CPU_010_PLUS CPU_010

/* Bit Isolation Functions */
#define BIT_0(A)  ((A) & 0x00000001)
#define BIT_1(A)  ((A) & 0x00000002)
#define BIT_2(A)  ((A) & 0x00000004)
#define BIT_3(A)  ((A) & 0x00000008)
#define BIT_4(A)  ((A) & 0x00000010)
#define BIT_5(A)  ((A) & 0x00000020)
#define BIT_6(A)  ((A) & 0x00000040)
#define BIT_7(A)  ((A) & 0x00000080)
#define BIT_8(A)  ((A) & 0x00000100)
#define BIT_9(A)  ((A) & 0x00000200)
#define BIT_A(A)  ((A) & 0x00000400)
#define BIT_B(A)  ((A) & 0x00000800)
#define BIT_C(A)  ((A) & 0x00001000)
#define BIT_D(A)  ((A) & 0x00002000)
#define BIT_E(A)  ((A) & 0x00004000)
#define BIT_F(A)  ((A) & 0x00008000)
#define BIT_10(A) ((A) & 0x00010000)
#define BIT_11(A) ((A) & 0x00020000)
#define BIT_12(A) ((A) & 0x00040000)
#define BIT_13(A) ((A) & 0x00080000)
#define BIT_14(A) ((A) & 0x00100000)
#define BIT_15(A) ((A) & 0x00200000)
#define BIT_16(A) ((A) & 0x00400000)
#define BIT_17(A) ((A) & 0x00800000)
#define BIT_18(A) ((A) & 0x01000000)
#define BIT_19(A) ((A) & 0x02000000)
#define BIT_1A(A) ((A) & 0x04000000)
#define BIT_1B(A) ((A) & 0x08000000)
#define BIT_1C(A) ((A) & 0x10000000)
#define BIT_1D(A) ((A) & 0x20000000)
#define BIT_1E(A) ((A) & 0x40000000)
#define BIT_1F(A) ((A) & 0x80000000)

#define GET_MSB_8(A)  ((A) & 0x80)
#define GET_MSB_9(A)  ((A) & 0x100)
#define GET_MSB_16(A) ((A) & 0x8000)
#define GET_MSB_17(A) ((A) & 0x10000)
#define GET_MSB_32(A) ((A) & 0x80000000)

#define LOW_NIBBLE(A) ((A) & 0x0f)
#define HIGH_NIBBLE(A) ((A) & 0xf0)

#define MASK_OUT_ABOVE_8(A)  ((A) & 0xff)
#define MASK_OUT_ABOVE_16(A) ((A) & 0xffff)
#define MASK_OUT_BELOW_8(A)  ((A) & ~0xff)
#define MASK_OUT_BELOW_16(A) ((A) & ~0xffff)

/* No need for useless masking if we're 32-bit */
#if M68000_OVER_32_BIT
#define MASK_OUT_ABOVE_32(A) ((A) & 0xffffffff)
#define MASK_OUT_BELOW_32(A) ((A) & ~0xffffffff)
#else
#define MASK_OUT_ABOVE_32(A) (A)
#define MASK_OUT_BELOW_32(A) 0
#endif /* M68000_OVER_32_BIT */


/* Rotate Functions */

#define ROL_8(VALUE, SHIFT) (((VALUE)<<(SHIFT)) | ((VALUE)>>(8-(SHIFT))))
#define ROL_9(VALUE, SHIFT) (((VALUE)<<(SHIFT)) | ((VALUE)>>(9-(SHIFT))))
#define ROL_16(VALUE, SHIFT) (((VALUE)<<(SHIFT)) | ((VALUE)>>(16-(SHIFT))))
#define ROL_17(VALUE, SHIFT) (((VALUE)<<(SHIFT)) | ((VALUE)>>(17-(SHIFT))))
#define ROL_32(VALUE, SHIFT) (((VALUE)<<(SHIFT)) | ((VALUE)>>(32-(SHIFT))))
#define ROL_33(VALUE, SHIFT) (LSL_32(VALUE, SHIFT) | LSR_32(VALUE, 33-(SHIFT)))

#define ROR_8(VALUE, SHIFT) (((VALUE)>>(SHIFT)) | ((VALUE)<<(8-(SHIFT))))
#define ROR_9(VALUE, SHIFT) (((VALUE)>>(SHIFT)) | ((VALUE)<<(9-(SHIFT))))
#define ROR_16(VALUE, SHIFT) (((VALUE)>>(SHIFT)) | ((VALUE)<<(16-(SHIFT))))
#define ROR_17(VALUE, SHIFT) (((VALUE)>>(SHIFT)) | ((VALUE)<<(17-(SHIFT))))
#define ROR_32(VALUE, SHIFT) (((VALUE)>>(SHIFT)) | ((VALUE)<<(32-(SHIFT))))
#define ROR_33(VALUE, SHIFT) (LSR_32(VALUE, SHIFT) | LSL_32(VALUE, 33-(SHIFT)))


/*
 * The general instruction format follows this pattern:
 * .... XXX. .... .YYY
 * where XXX is register X and YYY is register Y
 */
/* Data Register Isolation */
#define DX (g_cpu_dr[(g_cpu_ir >> 9) & 7])
#define DY (g_cpu_dr[g_cpu_ir & 7])
/* Address Register Isolation */
#define AX (g_cpu_ar[(g_cpu_ir >> 9) & 7])
#define AY (g_cpu_ar[g_cpu_ir & 7])


/* Effective Address Calculations */
#define EA_AI AY                              /* address register indirect */
#define EA_PI_8 (AY++)                        /* postincrement (size = byte) */
#define EA_PI7_8 ((g_cpu_ar[7]+=2)-2)         /* postincrement (size = byte & AR = 7) */
#define EA_PI_16 ((AY+=2)-2)                  /* postincrement (size = word) */
#define EA_PI_32 ((AY+=4)-4)                  /* postincrement (size = long) */
#define EA_PD_8 (--AY)                        /* predecrement (size = byte) */
#define EA_PD7_8 (g_cpu_ar[7]-=2)             /* predecrement (size = byte & AR = 7) */
#define EA_PD_16 (AY-=2)                      /* predecrement (size = word) */
#define EA_PD_32 (AY-=4)                      /* predecrement (size = long) */
#define EA_DI (AY+make_int_16(read_imm_16())) /* displacement */
#define EA_AW make_int_16(read_imm_16())      /* absolute word */
#define EA_AL read_imm_32()                   /* absolute long */


/* Add and subtract flag calculations */
#define VFLAG_ADD_8(src, dst, res) GET_MSB_8((src & dst & ~res) | (~src & ~dst & res))
#define VFLAG_ADD_16(src, dst, res) GET_MSB_16((src & dst & ~res) | (~src & ~dst & res))
#define VFLAG_ADD_32(src, dst, res) GET_MSB_32((src & dst & ~res) | (~src & ~dst & res))

#define CFLAG_ADD_8(src, dst, res) GET_MSB_8((src & dst) | (~res & dst) | (src & ~res))
#define CFLAG_ADD_16(src, dst, res) GET_MSB_16((src & dst) | (~res & dst) | (src & ~res))
#define CFLAG_ADD_32(src, dst, res) GET_MSB_32((src & dst) | (~res & dst) | (src & ~res))


#define VFLAG_SUB_8(src, dst, res) GET_MSB_8((~src & dst & ~res) | (src & ~dst & res))
#define VFLAG_SUB_16(src, dst, res) GET_MSB_16((~src & dst & ~res) | (src & ~dst & res))
#define VFLAG_SUB_32(src, dst, res) GET_MSB_32((~src & dst & ~res) | (src & ~dst & res))

#define CFLAG_SUB_8(src, dst, res) GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res))
#define CFLAG_SUB_16(src, dst, res) GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res))
#define CFLAG_SUB_32(src, dst, res) GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res))


/* Conditions */
#define CONDITION_HI     (g_cpu_c_flag == 0 && g_cpu_z_flag == 0)
#define CONDITION_NOT_HI (g_cpu_c_flag != 0 || g_cpu_z_flag != 0)
#define CONDITION_LS     (g_cpu_c_flag != 0 || g_cpu_z_flag != 0)
#define CONDITION_NOT_LS (g_cpu_c_flag == 0 && g_cpu_z_flag == 0)
#define CONDITION_CC     (g_cpu_c_flag == 0)
#define CONDITION_NOT_CC (g_cpu_c_flag != 0)
#define CONDITION_CS     (g_cpu_c_flag != 0)
#define CONDITION_NOT_CS (g_cpu_c_flag == 0)
#define CONDITION_NE     (g_cpu_z_flag == 0)
#define CONDITION_NOT_NE (g_cpu_z_flag != 0)
#define CONDITION_EQ     (g_cpu_z_flag != 0)
#define CONDITION_NOT_EQ (g_cpu_z_flag == 0)
#define CONDITION_VC     (g_cpu_v_flag == 0)
#define CONDITION_NOT_VC (g_cpu_v_flag != 0)
#define CONDITION_VS     (g_cpu_v_flag != 0)
#define CONDITION_NOT_VS (g_cpu_v_flag == 0)
#define CONDITION_PL     (g_cpu_n_flag == 0)
#define CONDITION_NOT_PL (g_cpu_n_flag != 0)
#define CONDITION_MI     (g_cpu_n_flag != 0)
#define CONDITION_NOT_MI (g_cpu_n_flag == 0)
#define CONDITION_GE     ((g_cpu_n_flag == 0) == (g_cpu_v_flag == 0))
#define CONDITION_NOT_GE ((g_cpu_n_flag == 0) != (g_cpu_v_flag == 0))
#define CONDITION_LT     ((g_cpu_n_flag == 0) != (g_cpu_v_flag == 0))
#define CONDITION_NOT_LT ((g_cpu_n_flag == 0) == (g_cpu_v_flag == 0))
#define CONDITION_GT     (g_cpu_z_flag == 0 && (g_cpu_n_flag == 0) == (g_cpu_v_flag == 0))
#define CONDITION_NOT_GT (g_cpu_z_flag != 0 || (g_cpu_n_flag == 0) != (g_cpu_v_flag == 0))
#define CONDITION_LE     (g_cpu_z_flag != 0 || (g_cpu_n_flag == 0) != (g_cpu_v_flag == 0))
#define CONDITION_NOT_LE (g_cpu_z_flag == 0 && (g_cpu_n_flag == 0) == (g_cpu_v_flag == 0))



/* ======================================================================== */
/* =============================== PROTOTYPES ============================= */
/* ======================================================================== */

/* This is used to generate the opcode handler jump table */
typedef struct
{
   void (*opcode_handler)(void); /* handler function */
   uint mask;                 /* mask on opcode */
   uint match;                /* what to match after masking */
} opcode_struct;


#if M68000_CHANGING_FC

/* Read data from anywhere */
INLINE uint  read_memory_8  (uint address);
INLINE uint  read_memory_16 (uint address);
INLINE uint  read_memory_32 (uint address);

/* Write to memory */
INLINE void  write_memory_8 (uint address, uint value);
INLINE void  write_memory_16(uint address, uint value);
INLINE void  write_memory_32(uint address, uint value);

#else

/* Read data from anywhere */
uint         read_memory_8  (uint address);
uint         read_memory_16 (uint address);
uint         read_memory_32 (uint address);

/* Write to memory */
void         write_memory_8 (uint address, uint value);
void         write_memory_16(uint address, uint value);
void         write_memory_32(uint address, uint value);

#endif /* M68000_CHANGING_FC */


/* Read data immediately after the program counter */
INLINE uint  read_imm_8(void);
INLINE uint  read_imm_16(void);
INLINE uint  read_imm_32(void);

/* Reads the next word after the program counter */
INLINE uint  read_instruction(void);

/* Read data with specific function code */
INLINE uint  read_memory_8_fc  (uint address, uint fc);
INLINE uint  read_memory_16_fc (uint address, uint fc);
INLINE uint  read_memory_32_fc (uint address, uint fc);

/* Write data with specific function code */
INLINE void  write_memory_8_fc (uint address, uint fc, uint value);
INLINE void  write_memory_16_fc(uint address, uint fc, uint value);
INLINE void  write_memory_32_fc(uint address, uint fc, uint value);

/* Push/Pull data to/from the stack */
void         push_16(uint data);
void         push_32(uint data);
uint         pull_16(void);
uint         pull_32(void);

/* Change program flow */
void         branch_byte(uint address);
void         branch_word(uint address);
void         branch_long(uint address);

INLINE void  exception(uint vector);          /* process an exception */
INLINE void  service_interrupt(void);         /* service a pending interrupt */
uint         get_int_mask(void);              /* get the interrupt mask */
void         set_int_mask(uint value);        /* set the interrupt mask */
INLINE void  set_s_flag(int value);           /* Set the S flag */
uint         get_sr(void);                    /* get the status register */
INLINE void  set_sr(uint value);              /* set the status register */
uint         get_ccr(void);                   /* get the condition code register */
INLINE void  set_ccr(uint value);             /* set the condition code register */
INLINE void  set_pc(uint address);            /* set the program counter */

/* Functions to build the opcode handler jump table */
static void  build_opcode_table(void);
static int QSORT_CALLBACK_DECL compare_nof_true_bits(const void *aptr, const void *bptr);



/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

static void  (*g_instruction_jump_table[0x10000])(void); /* opcode handler jump table */
static uint  g_emulation_initialized = 0;                /* flag if emulation has been initialized */
int          m68000_clks_left = 0;                       /* Number of clocks remaining */

/* sp[0] is user, sp[1] is system.  Speeds up supervisor/user switches */
#define g_cpu_usp g_cpu_sp[0]
#define g_cpu_isp g_cpu_sp[1]

/* Internal CPU registers */
static uint  g_cpu_type = CPU_000;
static uint  g_cpu_dr[8];       /* Data Registers */
static uint  g_cpu_ar[8];       /* Address Registers */
static uint  g_cpu_pc;          /* Program Counter */
static uint  g_cpu_sp[2];       /* User and Interrupt Stack Pointers */
static uint  g_cpu_vbr;         /* Vector Base Register (68010+) */
static uint  g_cpu_sfc;         /* Source Function Code Register (m68010+) */
static uint  g_cpu_dfc;         /* Destination Function Code Register (m68010+) */
static uint  g_cpu_ir;          /* Instruction Register */
static uint  g_cpu_t_flag;      /* Trace */
static uint  g_cpu_s_flag;      /* Supervisor */
static uint  g_cpu_x_flag;      /* Extend */
static uint  g_cpu_n_flag;      /* Negative */
static uint  g_cpu_z_flag;      /* Zero */
static uint  g_cpu_v_flag;      /* Overflow */
static uint  g_cpu_c_flag;      /* Carry */
static uint  g_cpu_int_mask;    /* I0-I2 */
static uint  g_cpu_ints_pending;/* Pending Interrupts */
static uint  g_cpu_stopped;     /* Stopped state */
#if M68000_HALT
static uint  g_cpu_halted;      /* Halted state */
#endif /* M68000_HALT */

/* Pointers to speed up address register indirect with index calculation */
static uint* g_cpu_dar[2] = {g_cpu_dr, g_cpu_ar};

/* Pointers to speed up movem instructions */
static uint* g_movem_pi_table[16] =
{
   g_cpu_dr, g_cpu_dr+1, g_cpu_dr+2, g_cpu_dr+3, g_cpu_dr+4, g_cpu_dr+5, g_cpu_dr+6, g_cpu_dr+7,
   g_cpu_ar, g_cpu_ar+1, g_cpu_ar+2, g_cpu_ar+3, g_cpu_ar+4, g_cpu_ar+5, g_cpu_ar+6, g_cpu_ar+7
};
static uint* g_movem_pd_table[16] =
{
   g_cpu_ar+7, g_cpu_ar+6, g_cpu_ar+5, g_cpu_ar+4, g_cpu_ar+3, g_cpu_ar+2, g_cpu_ar+1, g_cpu_ar,
   g_cpu_dr+7, g_cpu_dr+6, g_cpu_dr+5, g_cpu_dr+4, g_cpu_dr+3, g_cpu_dr+2, g_cpu_dr+1, g_cpu_dr,
};

/* Used when checking for pending interrupts */
static uint8 g_int_masks[] = {0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x80};

/* Used by opcodes such as addq, subq, shift, rotate etc */
static uint8 g_3bit_qdata_table[8] = {8, 1, 2, 3, 4, 5, 6, 7};

/* Used by shift & rotate instructions */
static uint8 g_shift_8_table[65] =
{
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
static uint16 g_shift_16_table[65] =
{
    0x0000, 0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0,
    0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff
};
static uint g_shift_32_table[65] =
{
    0x00000000, 0x80000000, 0xc0000000, 0xe0000000, 0xf0000000, 0xf8000000, 0xfc000000, 0xfe000000,
    0xff000000, 0xff800000, 0xffc00000, 0xffe00000, 0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000,
    0xffff0000, 0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
    0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};


/* Number of clock cycles to use for exception processing.
 * I used 4 for any vectors that are undocumented for processing times.
 */
uint8 g_exception_cycle_table[256] =
{
   40, /*  0: Reset - should never be called                                 */
   40, /*  1: Reset - should never be called                                 */
   50, /*  2: Bus Error                                (unused in emulation) */
   50, /*  3: Address Error                            (unused in emulation) */
   34, /*  4: Illegal Instruction                                            */
   42, /*  5: Divide by Zero                                                 */
   44, /*  6: CHK                                                            */
   34, /*  7: TRAPV                                                          */
   34, /*  8: Privilege Violation                                            */
   34, /*  9: Trace                                                          */
    4, /* 10: 1010                                                           */
    4, /* 11: 1111                                                           */
    4, /* 12: RESERVED                                                       */
    4, /* 13: Coprocessor Protocol Violation           (unused in emulation) */
    4, /* 14: Format Error                             (unused in emulation) */
   44, /* 15: Uninitialized Interrupt                                        */
    4, /* 16: RESERVED                                                       */
    4, /* 17: RESERVED                                                       */
    4, /* 18: RESERVED                                                       */
    4, /* 19: RESERVED                                                       */
    4, /* 20: RESERVED                                                       */
    4, /* 21: RESERVED                                                       */
    4, /* 22: RESERVED                                                       */
    4, /* 23: RESERVED                                                       */
   44, /* 24: Spurious Interrupt                                             */
   44, /* 25: Level 1 Interrupt Autovector                                   */
   44, /* 26: Level 2 Interrupt Autovector                                   */
   44, /* 27: Level 3 Interrupt Autovector                                   */
   44, /* 28: Level 4 Interrupt Autovector                                   */
   44, /* 29: Level 5 Interrupt Autovector                                   */
   44, /* 30: Level 6 Interrupt Autovector                                   */
   44, /* 31: Level 7 Interrupt Autovector                                   */
   38, /* 32: TRAP #0                                                        */
   38, /* 33: TRAP #1                                                        */
   38, /* 34: TRAP #2                                                        */
   38, /* 35: TRAP #3                                                        */
   38, /* 36: TRAP #4                                                        */
   38, /* 37: TRAP #5                                                        */
   38, /* 38: TRAP #6                                                        */
   38, /* 39: TRAP #7                                                        */
   38, /* 40: TRAP #8                                                        */
   38, /* 41: TRAP #9                                                        */
   38, /* 42: TRAP #10                                                       */
   38, /* 43: TRAP #11                                                       */
   38, /* 44: TRAP #12                                                       */
   38, /* 45: TRAP #13                                                       */
   38, /* 46: TRAP #14                                                       */
   38, /* 47: TRAP #15                                                       */
    4, /* 48: FP Branch or Set on Unknown Condition    (unused in emulation) */
    4, /* 49: FP Inexact Result                        (unused in emulation) */
    4, /* 50: FP Divide by Zero                        (unused in emulation) */
    4, /* 51: FP Underflow                             (unused in emulation) */
    4, /* 52: FP Operand Error                         (unused in emulation) */
    4, /* 53: FP Overflow                              (unused in emulation) */
    4, /* 54: FP Signaling NAN                         (unused in emulation) */
    4, /* 55: FP Unimplemented Data Type               (unused in emulation) */
    4, /* 56: MMU Configuration Error                  (unused in emulation) */
    4, /* 57: MMU Illegal Operation Error              (unused in emulation) */
    4, /* 58: MMU Access Level Violation Error         (unused in emulation) */
    4, /* 59: RESERVED                                                       */
    4, /* 60: RESERVED                                                       */
    4, /* 61: RESERVED                                                       */
    4, /* 62: RESERVED                                                       */
    4, /* 63: RESERVED                                                       */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /*  64- 79: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /*  80- 95: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /*  96-111: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 112-127: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 128-143: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 144-159: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 160-175: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 176-191: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 192-207: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 208-223: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 224-239: User Defined */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 240-255: User Defined */
};



/* ======================================================================== */
/* =========================== UTILITY FUNCTIONS ========================== */
/* ======================================================================== */

#if M68000_CHANGING_FC

/* Pass these function calls to the ones defined in the header. */
/* Ensure a 24-bit address is requested. */
INLINE uint read_memory_8(uint address)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   return m68000_read_memory_8((address)&0xffffff);
}
INLINE uint read_memory_16(uint address)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   return m68000_read_memory_16((address)&0xffffff);
}
INLINE uint read_memory_32(uint address)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   return m68000_read_memory_32((address)&0xffffff);
}

INLINE uint write_memory_8(uint address, uint value)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   return m68000_write_memory_8((address)&0xffffff, value);
}
INLINE uint write_memory_16(uint address, uint value)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   return m68000_write_memory_16((address)&0xffffff, value);
}
INLINE uint write_memory_32(uint address, uint value)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   return m68000_write_memory_32((address)&0xffffff, value);
}

#else

/* Pass these function calls to the ones defined in the header. */
/* Ensure a 24-bit address is requested. */
#define read_memory_8(address) m68000_read_memory_8((address)&0xffffff)
#define read_memory_16(address) m68000_read_memory_16((address)&0xffffff)
#define read_memory_32(address) m68000_read_memory_32((address)&0xffffff)

#define write_memory_8(address, value) m68000_write_memory_8((address)&0xffffff, value)
#define write_memory_16(address, value) m68000_write_memory_16((address)&0xffffff, value)
#define write_memory_32(address, value) m68000_write_memory_32((address)&0xffffff, value)

#endif /* M68000_CHANGING_FC */


INLINE uint read_imm_8(void)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   g_cpu_pc += 2;
   return m68000_read_immediate_8((g_cpu_pc-1)&0xffffff);
}
INLINE uint read_imm_16(void)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   g_cpu_pc += 2;
   return m68000_read_immediate_16((g_cpu_pc-2)&0xffffff);
}
INLINE uint read_imm_32(void)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_DATA : FUNCTION_CODE_USER_DATA);
   g_cpu_pc += 4;
   return m68000_read_immediate_32((g_cpu_pc-4)&0xffffff);
}

INLINE uint read_instruction(void)
{
   m68000_changing_fc(g_cpu_s_flag ? FUNCTION_CODE_SUPERVISOR_PROGRAM : FUNCTION_CODE_USER_PROGRAM);
   g_cpu_pc += 2;
   return m68000_read_instruction((g_cpu_pc-2)&0xffffff);
}

/* Read/Write data with specific function code */
INLINE uint read_memory_8_fc(uint address, uint fc)
{
   m68000_changing_fc(fc&7);
   return m68000_read_memory_8((address)&0xffffff);
}
INLINE uint read_memory_16_fc(uint address, uint fc)
{
   m68000_changing_fc(fc&7);
   return m68000_read_memory_16((address)&0xffffff);
}
INLINE uint read_memory_32_fc(uint address, uint fc)
{
   m68000_changing_fc(fc&7);
   return m68000_read_memory_32((address)&0xffffff);
}

INLINE void write_memory_8_fc(uint address, uint fc, uint value)
{
   m68000_changing_fc(fc&7);
   m68000_write_memory_8((address)&0xffffff, value);
}
INLINE void write_memory_16_fc(uint address, uint fc, uint value)
{
   m68000_changing_fc(fc&7);
   m68000_write_memory_16((address)&0xffffff, value);
}
INLINE void write_memory_32_fc(uint address, uint fc, uint value)
{
   m68000_changing_fc(fc&7);
   m68000_write_memory_32((address)&0xffffff, value);
}

/* Push/pull data to/from the stack */
#define push_16(data) write_memory_16(g_cpu_ar[7]-=2, data)
#define push_32(data) write_memory_32(g_cpu_ar[7]-=4, data)
#define pull_16() read_memory_16((g_cpu_ar[7]+=2) - 2)
#define pull_32() read_memory_32((g_cpu_ar[7]+=4) - 4)


/* Use up clock cycles.
 * NOTE: clock cycles used in here are 99.9% correct for a 68000, not for the
 * higher processors.
 */
#define USE_CLKS(A) m68000_clks_left -= (A)

/* Status Register / Condition Code Register
 * f  e  d  c  b  a  9  8  7  6  5  4  3  2  1  0
 * T  -  S  -  -  I2 I1 I0 -  -  -  X  N  Z  V  C
 */

/* Set interrupt mask.  If there's a pending interrupt higher than the mask, service it */
#define set_int_mask(value) if(g_cpu_ints_pending & g_int_masks[g_cpu_int_mask = (value)]) service_interrupt()

/* Set the interrupt mask */
#define get_int_mask() g_cpu_int_mask

/* Set the S flag */
INLINE void set_s_flag(int value)
{
   g_cpu_sp[g_cpu_s_flag] = g_cpu_ar[7];
   g_cpu_ar[7] = g_cpu_sp[(g_cpu_s_flag = value != 0)];
}

/* Get the condition code register */
#define get_ccr() (((g_cpu_x_flag != 0) << 4) |	\
                      ((g_cpu_n_flag != 0) << 3) |	\
                      ((g_cpu_z_flag != 0) << 2) |	\
                      ((g_cpu_v_flag != 0) << 1) |	\
                       (g_cpu_c_flag != 0))

/* Set the condition code register */
INLINE void set_ccr(uint value)
{
   g_cpu_x_flag = BIT_4(value);
   g_cpu_n_flag = BIT_3(value);
   g_cpu_z_flag = BIT_2(value);
   g_cpu_v_flag = BIT_1(value);
   g_cpu_c_flag = BIT_0(value);
}

/* Get the status register */
#define get_sr() (((g_cpu_t_flag != 0) << 15) |  \
                  ((g_cpu_s_flag != 0) << 13) |  \
                   (get_int_mask()     << 8) |   \
                  ((g_cpu_x_flag != 0) << 4) |   \
                  ((g_cpu_n_flag != 0) << 3) |   \
                  ((g_cpu_z_flag != 0) << 2) |   \
                  ((g_cpu_v_flag != 0) << 1) |   \
                   (g_cpu_c_flag != 0))

/* Set the status register */
INLINE void set_sr(uint value)
{
   g_cpu_t_flag = BIT_F(value);
   g_cpu_x_flag = BIT_4(value);
   g_cpu_n_flag = BIT_3(value);
   g_cpu_z_flag = BIT_2(value);
   g_cpu_v_flag = BIT_1(value);
   g_cpu_c_flag = BIT_0(value);
   set_s_flag(BIT_D(value));
   set_int_mask((value >> 8) & 7);
}

/* Service an interrupt request */
INLINE void service_interrupt(void)
{
   uint int_level = 7;
   uint vector;

   /* Start at level 7 and then go down *
    * I assume we always find a pending interrupt.
    * This is because there are only 2 places this is called from:
    * set_int_mask() and m68000_pulse_irq().
    * Both of these functins are reliable.
    */
   for(;!(g_cpu_ints_pending & (1<<int_level));int_level--)
      ;

   /* Remove from the pending interrupts list */
   g_cpu_ints_pending &= ~(1<<int_level);

   /* Get the exception vector */
   switch(vector = m68000_interrupt_acknowledge(int_level))
   {
      case 0x00: case 0x01:
      /* vectors 0 and 1 are ignored since they are for reset only */
         return;
      case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
      case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
      case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
      case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
      case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
      case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
      case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
      case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
      case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
      case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
      case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
      case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
      case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
      case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
      case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
      case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
      case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
      case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
      case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
      case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
      case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
      case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
      case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
      case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
      case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
      case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
      case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
      case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
      case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
      case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
      case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
      case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
         /* The external peripheral has provided the interrupt vector to take */
         break;
      case M68000_INT_ACK_AUTOVECTOR:
         /* Use the autovectors.  This is the most commonly used implementation */
         vector = EXCEPTION_INTERRUPT_AUTOVECTOR+int_level;
         break;
      case M68000_INT_ACK_SPURIOUS:
         /* Called if no devices respond to the interrupt acknowledge */
         vector = EXCEPTION_SPURIOUS_INTERRUPT;
         break;
      default:
         /* Everything else is ignored */
         return;
   }

   /* If vector is uninitialized, call the uninitialized interrupt vector */
   if(read_memory_32(vector<<2) == 0)
      vector = EXCEPTION_UNINITIALIZED_INTERRUPT;

   /* Generate an exception */
   exception(vector);

   /* Set the interrupt mask to the level of the one being serviced */
   g_cpu_int_mask = int_level;
}


/* Process an exception */
INLINE void exception(uint vector)
{
   /* Save the old status register */
   uint old_sr = get_sr();

   /* Use up some clock cycles */
   USE_CLKS(g_exception_cycle_table[vector]);

   /* Turn off stopped state and trace flag */
   g_cpu_stopped = 0;
   g_cpu_t_flag = 0;
   /* Enter supervisor mode */
   set_s_flag(1);
   /* Push a stack frame */
   if(g_cpu_type & CPU_010_PLUS)
      push_16(vector<<2); /* This is format 0 */
   push_32(g_cpu_pc);
   push_16(old_sr);
   /* Generate a new program counter from the vector */
   set_pc(read_memory_32((vector<<2)+g_cpu_vbr));
}

/* I set the PC this way to let host programs be nicer.
 * This is mainly for programs running from separate ram banks.
 * If the host program knows where the PC is, it can offer faster
 * ram access times for data to be retrieved immediately following
 * the PC.
 */
INLINE void set_pc(uint address)
{
   /* Set the program counter */
   g_cpu_pc = address;
   /* Inform the host program */
   m68000_setting_pc(address&0xffffff);
}

/* branch byte and word are for branches, while long is for jumps
 * so far it's been safe to not call set_pc() for branch word.
 */
#define branch_byte(address) g_cpu_pc += make_int_8(address)
#define branch_word(address) g_cpu_pc += make_int_16(address)
#define branch_long(address) set_pc(address)



/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */

/* Peek at the internals of the M68000 */
int m68000_peek_dr(int reg_num)
{
   return (reg_num < 8) ? g_cpu_dr[reg_num] & 0xffffffff : 0;
}

int m68000_peek_ar(int reg_num)
{
   return (reg_num < 8) ? g_cpu_ar[reg_num] & 0xffffffff : 0;
}

unsigned int m68000_peek_pc()
{
   return g_cpu_pc & 0xffffff;
}

int m68000_peek_sr()
{
   return get_sr() & 0xffff;
}

int m68000_peek_ir()
{
   return g_cpu_ir & 0xffff;
}

int m68000_peek_t_flag()
{
   return g_cpu_t_flag != 0;
}

int m68000_peek_s_flag()
{
   return g_cpu_s_flag != 0;
}

int m68000_peek_int_mask()
{
   return get_int_mask();
}

int m68000_peek_x_flag()
{
   return g_cpu_x_flag != 0;
}

int m68000_peek_n_flag()
{
   return g_cpu_n_flag != 0;
}

int m68000_peek_z_flag()
{
   return g_cpu_z_flag != 0;
}

int m68000_peek_v_flag()
{
   return g_cpu_v_flag != 0;
}

int m68000_peek_c_flag()
{
   return g_cpu_c_flag != 0;
}

int m68000_peek_usp()
{
   return g_cpu_s_flag ? g_cpu_usp : g_cpu_ar[7];
}

int m68000_peek_isp()
{
   return g_cpu_s_flag ? g_cpu_ar[7] : g_cpu_isp;
}

/* Poke data into the M68000 */
void m68000_poke_dr(int reg_num, int value)
{
   if(reg_num < 8)
      g_cpu_dr[reg_num] = MASK_OUT_ABOVE_32(value);
}

void m68000_poke_ar(int reg_num, int value)
{
   if(reg_num < 8)
      g_cpu_ar[reg_num] = MASK_OUT_ABOVE_32(value);
}

void m68000_poke_pc(unsigned int value)
{
   set_pc(value & 0xffffff);
}

void m68000_poke_sr(int value)
{
   set_sr(MASK_OUT_ABOVE_16(value));
}

void m68000_poke_ir(int value)
{
   g_cpu_ir = MASK_OUT_ABOVE_16(value);
}

void m68000_poke_t_flag(int value)
{
   g_cpu_t_flag = (value != 0);
}

void m68000_poke_s_flag(int value)
{
   set_s_flag(value);
}

void m68000_poke_int_mask(int value)
{
   set_int_mask(value & 7);
}

void m68000_poke_x_flag(int value)
{
   g_cpu_x_flag = (value != 0);
}

void m68000_poke_n_flag(int value)
{
   g_cpu_n_flag = (value != 0);
}

void m68000_poke_z_flag(int value)
{
   g_cpu_z_flag = (value != 0);
}

void m68000_poke_v_flag(int value)
{
   g_cpu_v_flag = (value != 0);
}

void m68000_poke_c_flag(int value)
{
   g_cpu_c_flag = (value != 0);
}

void m68000_poke_usp(int value)
{
   if(g_cpu_s_flag)
      g_cpu_usp = MASK_OUT_ABOVE_32(value);
   else
      g_cpu_ar[7] = MASK_OUT_ABOVE_32(value);
}

void m68000_poke_isp(int value)
{
   if(g_cpu_s_flag)
      g_cpu_ar[7] = MASK_OUT_ABOVE_32(value);
   else
      g_cpu_isp = MASK_OUT_ABOVE_32(value);
}

/* Execute some instructions */
int m68000_execute(int num_clks)
{
#if M68000_TRACE
   uint trace;
#endif /* M68000_TRACE */

#if M68000_HALT
   if(!g_cpu_halted)
   {
#endif /* M68000_HALT */
   /* Make sure we're not stopped */
   if(!g_cpu_stopped)
   {
      /* Set our pool of clock cycles available */
      m68000_clks_left = num_clks;

      /* Main loop.  Keep going until we run out of clock cycles */
      do
      {
#if M68000_TRACE
         trace = g_cpu_t_flag;
#endif /* M68000_TRACE */

         /* Call the debug hook, if any */
         m68000_debug_hook();
         /* Call the peek PC hook, if any */
         m68000_peek_pc_hook();
         /* Read an instruction and call its handler */
         g_cpu_ir = read_instruction();
         g_instruction_jump_table[g_cpu_ir]();

#if M68000_TRACE
         if(trace)
            exception(EXCEPTION_TRACE);
#endif /* M68000_TRACE */

      } while(m68000_clks_left > 0);

      /* return how many clocks we used */
      return num_clks - m68000_clks_left;
   }
#if M68000_HALT
   }
#endif /* M68000_HALT */

   /* We get here if the CPU is stopped */
   m68000_clks_left = 0;

   return num_clks;
}


/* Add an interrupt to the pending list */
void m68000_pulse_irq(int int_level)
{
   /* Add to the pending list and service an interrupt if it wasn't masked out */
   if((g_cpu_ints_pending |= (1<<(int_level&7))) & g_int_masks[g_cpu_int_mask])
      service_interrupt();
}

/* Remove an interrupt from the pending list */
void m68000_clear_irq(int int_level)
{
    g_cpu_ints_pending &= ~(1<<int_level);
}

/* Reset the M68000 */
void m68000_pulse_reset(void)
{
#if M68000_HALT
   g_cpu_halted = 0;
#endif /* M68000_HALT */
   g_cpu_stopped = 0;
   g_cpu_ints_pending = 0;
   g_cpu_t_flag = 0;
   g_cpu_x_flag = g_cpu_n_flag = g_cpu_z_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_usp = 0;
   g_cpu_ir = 0;
   g_cpu_dr[0] = g_cpu_dr[1] = g_cpu_dr[2] = g_cpu_dr[3] =
   g_cpu_dr[4] = g_cpu_dr[5] = g_cpu_dr[6] = g_cpu_dr[7] =
   g_cpu_ar[0] = g_cpu_ar[1] = g_cpu_ar[2] = g_cpu_ar[3] =
   g_cpu_ar[4] = g_cpu_ar[5] = g_cpu_ar[6] = g_cpu_ar[7] = 0;
   g_cpu_vbr = 0;
   g_cpu_sfc = 0;
   g_cpu_dfc = 0;
   set_s_flag(1);
   set_int_mask(7);
   g_cpu_ar[7] = g_cpu_isp = read_memory_32(0);
   set_pc(read_memory_32(4));
   m68000_clks_left = 0;

   /* The first call to this function initializes the opcode handler jump table */
   if(g_emulation_initialized)
      return;
   else
   {
      build_opcode_table();
      g_emulation_initialized = 1;
   }
}


/* Halt the CPU */
void m68000_pulse_halt(void)
{
#if M68000_HALT
   g_cpu_halted = 1;
#endif /* M68000_HALT */
}


/* Get and set the current CPU context */
/* This is to allow for multiple CPUs */
void m68000_get_context(m68000_cpu_context* cpu)
{
   memcpy(cpu->dr, g_cpu_dr, 8*sizeof(uint));
   memcpy(cpu->ar, g_cpu_ar, 8*sizeof(uint));
   cpu->pc = g_cpu_pc;
   cpu->usp = g_cpu_usp;
   cpu->isp = g_cpu_isp;
   cpu->sfc = g_cpu_sfc;
   cpu->dfc = g_cpu_dfc;
   cpu->vbr = g_cpu_vbr;
   cpu->sr = get_sr();
   cpu->ints_pending = g_cpu_ints_pending;
   cpu->stopped = g_cpu_stopped;
#if M68000_HALT
   cpu->halted = g_cpu_halted;
#endif /* M68000_HALT */
}

void m68000_set_context(m68000_cpu_context* cpu)
{
   memcpy(g_cpu_dr, cpu->dr, 8*sizeof(uint));
   memcpy(g_cpu_ar, cpu->ar, 8*sizeof(uint));
   g_cpu_usp = cpu->usp;
   g_cpu_isp = cpu->isp;
   g_cpu_ints_pending = cpu->ints_pending;
   g_cpu_stopped = cpu->stopped;
#if M68000_HALT
   g_cpu_halted = cpu->halted;
#endif /* M68000_HALT */
   set_pc(cpu->pc);
   set_sr(cpu->sr);
}



/* ======================================================================== */
/* ========================= INSTRUCTION HANDLERS ========================= */
/* ======================================================================== */
/* Instruction handler function names follow this convention:
 *
 * m68000_NAME_EXTENSIONS(void)
 * where NAME is the name of the opcode it handles and EXTENSIONS are any
 * extensions for special instances of that opcode.
 *
 * Examples:
 *   m68000_add_er_ai_8(): add opcode, from effective address to register,
 *                         using address register indirect, size = byte
 *
 *   m68000_asr_s_8(): arithmetic shift right, static count, size = byte
 *
 *
 * Note: move uses the form m68000_move_DST_SRC_SIZE
 *
 * Common extensions:
 * 8   : size = byte
 * 16  : size = word
 * 32  : size = long
 * rr  : register to register
 * mm  : memory to memory
 * a7  : using a7 register
 * ax7 : using a7 in X part of instruction (....XXX......YYY)
 * ay7 : using a7 in Y part of instruction (....XXX......YYY)
 * axy7: using a7 in both parts of instruction (....XXX......YYY)
 * r   : register
 * s   : static
 * er  : effective address -> register
 * re  : register -> effective address
 * ea  : using effective address mode of operation
 * d   : data register direct
 * a   : address register direct
 * ai  : address register indirect
 * pi  : address register indirect with postincrement
 * pi7 : address register 7 indirect with postincrement
 * pd  : address register indirect with predecrement
 * pd7 : address register 7 indirect with predecrement
 * di  : address register indirect with displacement
 * ix  : address register indirect with index
 * aw  : absolute word
 * al  : absolute long
 * pcdi: program counter with displacement
 * pcix: program counter with index
 */

static void m68000_1010(void)
{
   exception(EXCEPTION_1010);
}


static void m68000_1111(void)
{
   exception(EXCEPTION_1111);
}


static void m68000_abcd_rr(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + (g_cpu_x_flag != 0);

   if(res > 9)
      res += 6;
   res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res -= 0xa0;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | MASK_OUT_ABOVE_8(res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(6);
}


static void m68000_abcd_mm_ax7(void)
{
   uint src = read_memory_8(--AY);
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + (g_cpu_x_flag != 0);

   if(res > 9)
      res += 6;
   res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res -= 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_abcd_mm_ay7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = --AX;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + (g_cpu_x_flag != 0);

   if(res > 9)
      res += 6;
   res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res -= 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_abcd_mm_axy7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + (g_cpu_x_flag != 0);

   if(res > 9)
      res += 6;
   res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res -= 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_abcd_mm(void)
{
   uint src = read_memory_8(--AY);
   uint ea  = --AX;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + (g_cpu_x_flag != 0);

   if(res > 9)
      res += 6;
   res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res -= 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_add_er_d_8(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4);
}


static void m68000_add_er_ai_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_AI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+4);
}


static void m68000_add_er_pi_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PI_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+4);
}


static void m68000_add_er_pi7_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PI7_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+4);
}


static void m68000_add_er_pd_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PD_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+6);
}


static void m68000_add_er_pd7_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PD7_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+6);
}


static void m68000_add_er_di_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_DI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+8);
}


static void m68000_add_er_ix_8(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+10);
}


static void m68000_add_er_aw_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_AW);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+8);
}


static void m68000_add_er_al_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_AL);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+12);
}


static void m68000_add_er_pcdi_8(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_8(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+8);
}


static void m68000_add_er_pcix_8(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+10);
}


static void m68000_add_er_i_8(void)
{
   uint* d_dst = &DX;
   uint src = read_imm_8();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+4);
}


static void m68000_add_er_d_16(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4);
}


static void m68000_add_er_a_16(void)
{
   uint* d_dst = &DX;
   uint src = AY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4);
}


static void m68000_add_er_ai_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+4);
}


static void m68000_add_er_pi_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_PI_16);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+4);
}


static void m68000_add_er_pd_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_PD_16);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+6);
}


static void m68000_add_er_di_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_DI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+8);
}


static void m68000_add_er_ix_16(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+10);
}


static void m68000_add_er_aw_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AW);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+8);
}


static void m68000_add_er_al_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AL);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+12);
}


static void m68000_add_er_pcdi_16(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_16(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+8);
}


static void m68000_add_er_pcix_16(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+10);
}


static void m68000_add_er_i_16(void)
{
   uint* d_dst = &DX;
   uint src = read_imm_16();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4+4);
}


static void m68000_add_er_d_32(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8);
}


static void m68000_add_er_a_32(void)
{
   uint* d_dst = &DX;
   uint src = AY;
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8);
}


static void m68000_add_er_ai_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_AI);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+8);
}


static void m68000_add_er_pi_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_PI_32);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+8);
}


static void m68000_add_er_pd_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_PD_32);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+10);
}


static void m68000_add_er_di_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_DI);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+12);
}


static void m68000_add_er_ix_32(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+14);
}


static void m68000_add_er_aw_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_AW);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+12);
}


static void m68000_add_er_al_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_AL);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+16);
}


static void m68000_add_er_pcdi_32(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_32(ea);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+12);
}


static void m68000_add_er_pcix_32(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+14);
}


static void m68000_add_er_i_32(void)
{
   uint* d_dst = &DX;
   uint src = read_imm_32();
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(6+8);
}


static void m68000_add_re_ai_8(void)
{
   uint ea = EA_AI;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_add_re_pi_8(void)
{
   uint ea = EA_PI_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_add_re_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_add_re_pd_8(void)
{
   uint ea = EA_PD_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+6);
}


static void m68000_add_re_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+6);
}


static void m68000_add_re_di_8(void)
{
   uint ea = EA_DI;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_add_re_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+10);
}


static void m68000_add_re_aw_8(void)
{
   uint ea = EA_AW;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_add_re_al_8(void)
{
   uint ea = EA_AL;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+12);
}


static void m68000_add_re_ai_16(void)
{
   uint ea = EA_AI;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_add_re_pi_16(void)
{
   uint ea = EA_PI_16;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_add_re_pd_16(void)
{
   uint ea = EA_PD_16;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+6);
}


static void m68000_add_re_di_16(void)
{
   uint ea = EA_DI;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_add_re_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+10);
}


static void m68000_add_re_aw_16(void)
{
   uint ea = EA_AW;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_add_re_al_16(void)
{
   uint ea = EA_AL;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+12);
}


static void m68000_add_re_ai_32(void)
{
   uint ea = EA_AI;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_add_re_pi_32(void)
{
   uint ea = EA_PI_32;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_add_re_pd_32(void)
{
   uint ea = EA_PD_32;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+10);
}


static void m68000_add_re_di_32(void)
{
   uint ea = EA_DI;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+12);
}


static void m68000_add_re_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+14);
}


static void m68000_add_re_aw_32(void)
{
   uint ea = EA_AW;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+12);
}


static void m68000_add_re_al_32(void)
{
   uint ea = EA_AL;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+16);
}


static void m68000_adda_d_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(MASK_OUT_ABOVE_16(DY)));
   USE_CLKS(8);
}


static void m68000_adda_a_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(MASK_OUT_ABOVE_16(AY)));
   USE_CLKS(8);
}


static void m68000_adda_ai_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(EA_AI)));
   USE_CLKS(8+4);
}


static void m68000_adda_pi_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(EA_PI_16)));
   USE_CLKS(8+4);
}


static void m68000_adda_pd_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(EA_PD_16)));
   USE_CLKS(8+6);
}


static void m68000_adda_di_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(EA_DI)));
   USE_CLKS(8+8);
}


static void m68000_adda_ix_16(void)
{
   uint* a_dst = &AX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(ea)));
   USE_CLKS(8+10);
}


static void m68000_adda_aw_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(EA_AW)));
   USE_CLKS(8+8);
}


static void m68000_adda_al_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(EA_AL)));
   USE_CLKS(8+12);
}


static void m68000_adda_pcdi_16(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(ea)));
   USE_CLKS(8+8);
}


static void m68000_adda_pcix_16(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_memory_16(ea)));
   USE_CLKS(8+10);
}


static void m68000_adda_i_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + make_int_16(read_imm_16()));
   USE_CLKS(8+4);
}


static void m68000_adda_d_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + DY);
   USE_CLKS(8);
}


static void m68000_adda_a_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + AY);
   USE_CLKS(8);
}


static void m68000_adda_ai_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(EA_AI));
   USE_CLKS(6+8);
}


static void m68000_adda_pi_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(EA_PI_32));
   USE_CLKS(6+8);
}


static void m68000_adda_pd_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(EA_PD_32));
   USE_CLKS(6+10);
}


static void m68000_adda_di_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(EA_DI));
   USE_CLKS(6+12);
}


static void m68000_adda_ix_32(void)
{
   uint* a_dst = &AX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(ea));
   USE_CLKS(6+14);
}


static void m68000_adda_aw_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(EA_AW));
   USE_CLKS(6+12);
}


static void m68000_adda_al_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(EA_AL));
   USE_CLKS(6+16);
}


static void m68000_adda_pcdi_32(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(ea));
   USE_CLKS(6+12);
}


static void m68000_adda_pcix_32(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_memory_32(ea));
   USE_CLKS(6+14);
}


static void m68000_adda_i_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + read_imm_32());
   USE_CLKS(6+8);
}


static void m68000_addi_d_8(void)
{
   uint* d_dst = &DY;
   uint src = read_imm_8();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8);
}


static void m68000_addi_ai_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+4);
}


static void m68000_addi_pi_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+4);
}


static void m68000_addi_pi7_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+4);
}


static void m68000_addi_pd_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+6);
}


static void m68000_addi_pd7_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+6);
}


static void m68000_addi_di_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_addi_ix_8(void)
{
   uint src = read_imm_8();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+10);
}


static void m68000_addi_aw_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_addi_al_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+12);
}


static void m68000_addi_d_16(void)
{
   uint* d_dst = &DY;
   uint src = read_imm_16();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8);
}


static void m68000_addi_ai_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_AI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+4);
}


static void m68000_addi_pi_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_PI_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+4);
}


static void m68000_addi_pd_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_PD_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+6);
}


static void m68000_addi_di_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_DI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_addi_ix_16(void)
{
   uint src = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+10);
}


static void m68000_addi_aw_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_AW;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_addi_al_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_AL;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+12);
}


static void m68000_addi_d_32(void)
{
   uint* d_dst = &DY;
   uint src = read_imm_32();
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(16);
}


static void m68000_addi_ai_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_AI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(20+8);
}


static void m68000_addi_pi_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_PI_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(20+8);
}


static void m68000_addi_pd_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_PD_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(20+10);
}


static void m68000_addi_di_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_DI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(20+12);
}


static void m68000_addi_ix_32(void)
{
   uint src = read_imm_32();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(20+14);
}


static void m68000_addi_aw_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_AW;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(20+12);
}


static void m68000_addi_al_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_AL;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(20+16);
}


static void m68000_addq_d_8(void)
{
   uint* d_dst = &DY;
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4);
}


static void m68000_addq_ai_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_addq_pi_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_addq_pi7_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_addq_pd_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+6);
}


static void m68000_addq_pd7_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+6);
}


static void m68000_addq_di_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_addq_ix_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+10);
}


static void m68000_addq_aw_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_addq_al_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+12);
}


static void m68000_addq_d_16(void)
{
   uint* d_dst = &DY;
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4);
}


static void m68000_addq_a_16(void)
{
   uint* a_dst = &AY;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + g_3bit_qdata_table[(g_cpu_ir>>9)&7]);
   USE_CLKS(4);
}


static void m68000_addq_ai_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_addq_pi_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+4);
}


static void m68000_addq_pd_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+6);
}


static void m68000_addq_di_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_DI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_addq_ix_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+10);
}


static void m68000_addq_aw_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AW;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+8);
}


static void m68000_addq_al_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AL;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8+12);
}


static void m68000_addq_d_32(void)
{
   uint* d_dst = &DY;
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8);
}


static void m68000_addq_a_32(void)
{
   uint* a_dst = &AY;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst + g_3bit_qdata_table[(g_cpu_ir>>9)&7]);
   USE_CLKS(8);
}


static void m68000_addq_ai_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_addq_pi_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+8);
}


static void m68000_addq_pd_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+10);
}


static void m68000_addq_di_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_DI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+12);
}


static void m68000_addq_ix_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+14);
}


static void m68000_addq_aw_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AW;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+12);
}


static void m68000_addq_al_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AL;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(12+16);
}


static void m68000_addx_rr_8(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(src + dst + (g_cpu_x_flag != 0));

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4);
}


static void m68000_addx_rr_16(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(src + dst + (g_cpu_x_flag != 0));

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(4);
}


static void m68000_addx_rr_32(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(src + dst + (g_cpu_x_flag != 0));

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(8);
}


static void m68000_addx_mm_8_ax7(void)
{
   uint src = read_memory_8(--AY);
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst + (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(18);
}


static void m68000_addx_mm_8_ay7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = --AX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst + (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(18);
}


static void m68000_addx_mm_8_axy7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst + (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(18);
}


static void m68000_addx_mm_8(void)
{
   uint src = read_memory_8(--AY);
   uint ea  = --AX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(src + dst + (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(18);
}


static void m68000_addx_mm_16(void)
{
   uint src = read_memory_16(AY-=2);
   uint ea  = (AX-=2);
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src + dst + (g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(18);
}


static void m68000_addx_mm_32(void)
{
   uint src = read_memory_32(AY-=4);
   uint ea  = (AX-=4);
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(src + dst + (g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32((src & dst & ~res) | (~src & ~dst & res));
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & dst) | (~res & dst) | (src & ~res));
   USE_CLKS(30);
}


static void m68000_and_er_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (DY | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_and_er_ai_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_AI) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_and_er_pi_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_PI_8) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_and_er_pi7_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_PI7_8) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_and_er_pd_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_PD_8) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_and_er_pd7_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_PD7_8) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_and_er_di_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_DI) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_and_er_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(ea) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_and_er_aw_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_AW) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_and_er_al_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(EA_AL) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_and_er_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(ea) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_and_er_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8(DX &= (read_memory_8(ea) | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_and_er_i_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DX &= (read_imm_8() | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_and_er_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (DY | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_and_er_ai_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(EA_AI) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_and_er_pi_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(EA_PI_16) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_and_er_pd_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(EA_PD_16) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_and_er_di_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(EA_DI) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_and_er_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(ea) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_and_er_aw_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(EA_AW) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_and_er_al_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(EA_AL) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_and_er_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(ea) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_and_er_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16(DX &= (read_memory_16(ea) | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_and_er_i_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DX &= (read_imm_16() | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_and_er_d_32(void)
{
   uint res = DX &= DY;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_and_er_ai_32(void)
{
   uint res = DX &= read_memory_32(EA_AI);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+8);
}


static void m68000_and_er_pi_32(void)
{
   uint res = DX &= read_memory_32(EA_PI_32);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+8);
}


static void m68000_and_er_pd_32(void)
{
   uint res = DX &= read_memory_32(EA_PD_32);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+10);
}


static void m68000_and_er_di_32(void)
{
   uint res = DX &= read_memory_32(EA_DI);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+12);
}


static void m68000_and_er_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = DX &= read_memory_32(ea);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+14);
}


static void m68000_and_er_aw_32(void)
{
   uint res = DX &= read_memory_32(EA_AW);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+12);
}


static void m68000_and_er_al_32(void)
{
   uint res = DX &= read_memory_32(EA_AL);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+16);
}


static void m68000_and_er_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = DX &= read_memory_32(ea);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+12);
}


static void m68000_and_er_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = DX &= read_memory_32(ea);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+14);
}


static void m68000_and_er_i_32(void)
{
   uint res = DX &= read_imm_32();

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+8);
}


static void m68000_and_re_ai_8(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_and_re_pi_8(void)
{
   uint ea = EA_PI_8;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_and_re_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_and_re_pd_8(void)
{
   uint ea = EA_PD_8;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_and_re_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_and_re_di_8(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_and_re_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_and_re_aw_8(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_and_re_al_8(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_8(DX & read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_and_re_ai_16(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_16(DX & read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_and_re_pi_16(void)
{
   uint ea = EA_PI_16;
   uint res = MASK_OUT_ABOVE_16(DX & read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_and_re_pd_16(void)
{
   uint ea = EA_PD_16;
   uint res = MASK_OUT_ABOVE_16(DX & read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_and_re_di_16(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_16(DX & read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_and_re_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16(DX & read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_and_re_aw_16(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_16(DX & read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_and_re_al_16(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_16(DX & read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_and_re_ai_32(void)
{
   uint ea = EA_AI;
   uint res = DX & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_and_re_pi_32(void)
{
   uint ea = EA_PI_32;
   uint res = DX & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_and_re_pd_32(void)
{
   uint ea = EA_PD_32;
   uint res = DX & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_and_re_di_32(void)
{
   uint ea = EA_DI;
   uint res = DX & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_and_re_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = DX & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_and_re_aw_32(void)
{
   uint ea = EA_AW;
   uint res = DX & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_and_re_al_32(void)
{
   uint ea = EA_AL;
   uint res = DX & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+16);
}


static void m68000_andi_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY &= (read_imm_8() | 0xffffff00));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_andi_ai_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AI;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_andi_pi_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PI_8;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_andi_pi7_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PI7_8;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_andi_pd_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PD_8;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_andi_pd7_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PD7_8;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_andi_di_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_DI;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_andi_ix_8(void)
{
   uint tmp = read_imm_8();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_andi_aw_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AW;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_andi_al_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AL;
   uint res = tmp & read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_andi_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY &= (read_imm_16() | 0xffff0000));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_andi_ai_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AI;
   uint res = tmp & read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_andi_pi_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_PI_16;
   uint res = tmp & read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_andi_pd_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_PD_16;
   uint res = tmp & read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_andi_di_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_DI;
   uint res = tmp & read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_andi_ix_16(void)
{
   uint tmp = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = tmp & read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_andi_aw_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AW;
   uint res = tmp & read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_andi_al_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AL;
   uint res = tmp & read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_andi_d_32(void)
{
   uint res = DY &= (read_imm_32());

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(14);
}


static void m68000_andi_ai_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AI;
   uint res = tmp & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_andi_pi_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_PI_32;
   uint res = tmp & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_andi_pd_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_PD_32;
   uint res = tmp & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+10);
}


static void m68000_andi_di_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_DI;
   uint res = tmp & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_andi_ix_32(void)
{
   uint tmp = read_imm_32();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = tmp & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+14);
}


static void m68000_andi_aw_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AW;
   uint res = tmp & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_andi_al_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AL;
   uint res = tmp & read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+16);
}


static void m68000_andi_to_ccr(void)
{
   set_ccr(get_ccr() & read_imm_8());
   USE_CLKS(20);
}


static void m68000_andi_to_sr(void)
{
   uint and_val = read_imm_16();

   if(g_cpu_s_flag)
   {
      set_sr(get_sr() & and_val);
      USE_CLKS(20);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_asr_s_8(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = src >> shift;

   if(GET_MSB_8(src))
      res |= g_shift_8_table[shift];

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = shift > 7 ? g_cpu_n_flag : (src >> (shift-1)) & 1;
   USE_CLKS((shift<<1)+6);
}


static void m68000_asr_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = src >> shift;

   if(GET_MSB_16(src))
      res |= g_shift_16_table[shift];

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = (src >> (shift-1)) & 1;
   USE_CLKS((shift<<1)+6);
}


static void m68000_asr_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = src >> shift;

   if(GET_MSB_32(src))
      res |= g_shift_32_table[shift];

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = (src >> (shift-1)) & 1;
   USE_CLKS((shift<<1)+8);
}


static void m68000_asr_r_8(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = src >> shift;

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift < 8)
      {
         if(GET_MSB_8(src))
            res |= g_shift_8_table[shift];

         *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

         g_cpu_c_flag = g_cpu_x_flag = (src >> (shift-1)) & 1;
         g_cpu_n_flag = GET_MSB_8(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      if(GET_MSB_8(src))
      {
         *d_dst |= 0xff;
         g_cpu_c_flag = g_cpu_x_flag = 1;
         g_cpu_n_flag = 1;
         g_cpu_z_flag = 0;
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst &= 0xffffff00;
      g_cpu_c_flag = g_cpu_x_flag = 0;
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_asr_r_16(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = src >> shift;

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift < 16)
      {
         if(GET_MSB_16(src))
            res |= g_shift_16_table[shift];

         *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

         g_cpu_c_flag = g_cpu_x_flag = (src >> (shift-1)) & 1;
         g_cpu_n_flag = GET_MSB_16(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      if(GET_MSB_16(src))
      {
         *d_dst |= 0xffff;
         g_cpu_c_flag = g_cpu_x_flag = 1;
         g_cpu_n_flag = 1;
         g_cpu_z_flag = 0;
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst &= 0xffff0000;
      g_cpu_c_flag = g_cpu_x_flag = 0;
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_asr_r_32(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = src >> shift;

   USE_CLKS((shift<<1)+8);
   if(shift != 0)
   {
      if(shift < 32)
      {
         if(GET_MSB_32(src))
            res |= g_shift_32_table[shift];

         *d_dst = res;

         g_cpu_c_flag = g_cpu_x_flag = (src >> (shift-1)) & 1;
         g_cpu_n_flag = GET_MSB_32(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      if(GET_MSB_32(src))
      {
         *d_dst = 0xffffffff;
         g_cpu_c_flag = g_cpu_x_flag = 1;
         g_cpu_n_flag = 1;
         g_cpu_z_flag = 0;
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst = 0;
      g_cpu_c_flag = g_cpu_x_flag = 0;
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_asr_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   if(GET_MSB_16(src))
      res |= 0x8000;

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   USE_CLKS(8+4);
}


static void m68000_asr_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   if(GET_MSB_16(src))
      res |= 0x8000;

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   USE_CLKS(8+4);
}


static void m68000_asr_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   if(GET_MSB_16(src))
      res |= 0x8000;

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   USE_CLKS(8+6);
}


static void m68000_asr_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   if(GET_MSB_16(src))
      res |= 0x8000;

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   USE_CLKS(8+8);
}


static void m68000_asr_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   if(GET_MSB_16(src))
      res |= 0x8000;

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   USE_CLKS(8+10);
}


static void m68000_asr_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   if(GET_MSB_16(src))
      res |= 0x8000;

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   USE_CLKS(8+8);
}


static void m68000_asr_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   if(GET_MSB_16(src))
      res |= 0x8000;

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   USE_CLKS(8+12);
}


static void m68000_asl_s_8(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(src << shift);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_x_flag = g_cpu_c_flag = (src >> (8-shift)) & 1;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   src &= g_shift_8_table[shift+1];
   g_cpu_v_flag = !(src == 0 || (src == g_shift_8_table[shift+1] && shift < 8));

   USE_CLKS((shift<<1)+6);
}


static void m68000_asl_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(src << shift);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = (src >> (16-shift)) & 1;
   src &= g_shift_16_table[shift+1];
   g_cpu_v_flag = !(src == 0 || src == g_shift_16_table[shift+1]);

   USE_CLKS((shift<<1)+6);
}


static void m68000_asl_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = *d_dst;
   uint res = MASK_OUT_ABOVE_32(src << shift);

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = (src >> (32-shift)) & 1;
   src &= g_shift_32_table[shift+1];
   g_cpu_v_flag = !(src == 0 || src == g_shift_32_table[shift+1]);

   USE_CLKS((shift<<1)+8);
}


static void m68000_asl_r_8(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(src << shift);

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift < 8)
      {
         *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (8-shift)) & 1;
         g_cpu_n_flag = GET_MSB_8(res);
         g_cpu_z_flag = (res == 0);
         src &= g_shift_8_table[shift+1];
         g_cpu_v_flag = !(src == 0 || src == g_shift_8_table[shift+1]);
         return;
      }

      *d_dst &= 0xffffff00;
      g_cpu_x_flag = g_cpu_c_flag = (shift == 8 ? src & 1 : 0);
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = !(src == 0);
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_asl_r_16(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(src << shift);

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift < 16)
      {
         *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (16-shift)) & 1;
         g_cpu_n_flag = GET_MSB_16(res);
         g_cpu_z_flag = (res == 0);
         src &= g_shift_16_table[shift+1];
         g_cpu_v_flag = !(src == 0 || src == g_shift_16_table[shift+1]);
         return;
      }

      *d_dst &= 0xffff0000;
      g_cpu_x_flag = g_cpu_c_flag = (shift == 16 ? src & 1 : 0);
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = !(src == 0);
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_asl_r_32(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = *d_dst;
   uint res = MASK_OUT_ABOVE_32(src << shift);

   USE_CLKS((shift<<1)+8);
   if(shift != 0)
   {
      if(shift < 32)
      {
         *d_dst = res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (32-shift)) & 1;
         g_cpu_n_flag = GET_MSB_32(res);
         g_cpu_z_flag = (res == 0);
         src &= g_shift_32_table[shift+1];
         g_cpu_v_flag = !(src == 0 || src == g_shift_32_table[shift+1]);
         return;
      }

      *d_dst = 0;
      g_cpu_x_flag = g_cpu_c_flag = (shift == 32 ? src & 1 : 0);
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = !(src == 0);
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_asl_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   src &= 0xc000;
   g_cpu_v_flag = !(src == 0 || src == 0xc000);
   USE_CLKS(8+4);
}


static void m68000_asl_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   src &= 0xc000;
   g_cpu_v_flag = !(src == 0 || src == 0xc000);
   USE_CLKS(8+4);
}


static void m68000_asl_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   src &= 0xc000;
   g_cpu_v_flag = !(src == 0 || src == 0xc000);
   USE_CLKS(8+6);
}


static void m68000_asl_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   src &= 0xc000;
   g_cpu_v_flag = !(src == 0 || src == 0xc000);
   USE_CLKS(8+8);
}


static void m68000_asl_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   src &= 0xc000;
   g_cpu_v_flag = !(src == 0 || src == 0xc000);
   USE_CLKS(8+10);
}


static void m68000_asl_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   src &= 0xc000;
   g_cpu_v_flag = !(src == 0 || src == 0xc000);
   USE_CLKS(8+8);
}


static void m68000_asl_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   src &= 0xc000;
   g_cpu_v_flag = !(src == 0 || src == 0xc000);
   USE_CLKS(8+12);
}


static void m68000_bhi_8(void)
{
   if(CONDITION_HI)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bhi_16(void)
{
   if(CONDITION_HI)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bls_8(void)
{
   if(CONDITION_LS)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bls_16(void)
{
   if(CONDITION_LS)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bcc_8(void)
{
   if(CONDITION_CC)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bcc_16(void)
{
   if(CONDITION_CC)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bcs_8(void)
{
   if(CONDITION_CS)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bcs_16(void)
{
   if(CONDITION_CS)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bne_8(void)
{
   if(CONDITION_NE)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bne_16(void)
{
   if(CONDITION_NE)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_beq_8(void)
{
   if(CONDITION_EQ)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_beq_16(void)
{
   if(CONDITION_EQ)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bvc_8(void)
{
   if(CONDITION_VC)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bvc_16(void)
{
   if(CONDITION_VC)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bvs_8(void)
{
   if(CONDITION_VS)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bvs_16(void)
{
   if(CONDITION_VS)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bpl_8(void)
{
   if(CONDITION_PL)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bpl_16(void)
{
   if(CONDITION_PL)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bmi_8(void)
{
   if(CONDITION_MI)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bmi_16(void)
{
   if(CONDITION_MI)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bge_8(void)
{
   if(CONDITION_GE)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bge_16(void)
{
   if(CONDITION_GE)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_blt_8(void)
{
   if(CONDITION_LT)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_blt_16(void)
{
   if(CONDITION_LT)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bgt_8(void)
{
   if(CONDITION_GT)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_bgt_16(void)
{
   if(CONDITION_GT)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_ble_8(void)
{
   if(CONDITION_LE)
   {
      branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
      USE_CLKS(10);
      return;
   }
   USE_CLKS(8);
}


static void m68000_ble_16(void)
{
   if(CONDITION_LE)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_bchg_r_d(void)
{
   uint* d_dst = &DY;
   uint mask = 1 << (DX & 0x1f);

   g_cpu_z_flag = !(*d_dst & mask);
   *d_dst ^= mask;
   USE_CLKS(8);
}


static void m68000_bchg_r_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+4);
}


static void m68000_bchg_r_pi(void)
{
   uint ea = EA_PI_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+4);
}


static void m68000_bchg_r_pi7(void)
{
   uint ea = EA_PI7_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+4);
}


static void m68000_bchg_r_pd(void)
{
   uint ea = EA_PD_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+6);
}


static void m68000_bchg_r_pd7(void)
{
   uint ea = EA_PD7_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+6);
}


static void m68000_bchg_r_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+8);
}


static void m68000_bchg_r_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+10);
}


static void m68000_bchg_r_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+8);
}


static void m68000_bchg_r_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(8+12);
}


static void m68000_bchg_s_d(void)
{
   uint* d_dst = &DY;
   uint mask = 1 << (read_imm_8() & 0x1f);

   g_cpu_z_flag = !(*d_dst & mask);
   *d_dst ^= mask;
   USE_CLKS(12);
}


static void m68000_bchg_s_ai(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AI;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+4);
}


static void m68000_bchg_s_pi(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PI_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+4);
}


static void m68000_bchg_s_pi7(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PI7_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+4);
}


static void m68000_bchg_s_pd(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PD_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+6);
}


static void m68000_bchg_s_pd7(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PD7_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+6);
}


static void m68000_bchg_s_di(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_DI;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+8);
}


static void m68000_bchg_s_ix(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+10);
}


static void m68000_bchg_s_aw(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AW;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+8);
}


static void m68000_bchg_s_al(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AL;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src ^ mask);
   USE_CLKS(12+12);
}


static void m68000_bclr_r_d(void)
{
   uint* d_dst = &DY;
   uint mask = 1 << (DX & 0x1f);

   g_cpu_z_flag = !(*d_dst & mask);
   *d_dst &= ~mask;
   USE_CLKS(10);
}


static void m68000_bclr_r_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+4);
}


static void m68000_bclr_r_pi(void)
{
   uint ea = EA_PI_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+4);
}


static void m68000_bclr_r_pi7(void)
{
   uint ea = EA_PI7_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+4);
}


static void m68000_bclr_r_pd(void)
{
   uint ea = EA_PD_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+6);
}


static void m68000_bclr_r_pd7(void)
{
   uint ea = EA_PD7_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+6);
}


static void m68000_bclr_r_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+8);
}


static void m68000_bclr_r_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+10);
}


static void m68000_bclr_r_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+8);
}


static void m68000_bclr_r_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(8+12);
}


static void m68000_bclr_s_d(void)
{
   uint* d_dst = &DY;
   uint mask = 1 << (read_imm_8() & 0x1f);

   g_cpu_z_flag = !(*d_dst & mask);
   *d_dst &= ~mask;
   USE_CLKS(14);
}


static void m68000_bclr_s_ai(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AI;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+4);
}


static void m68000_bclr_s_pi(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PI_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+4);
}


static void m68000_bclr_s_pi7(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PI7_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+4);
}


static void m68000_bclr_s_pd(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PD_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+6);
}


static void m68000_bclr_s_pd7(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PD7_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+6);
}


static void m68000_bclr_s_di(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_DI;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+8);
}


static void m68000_bclr_s_ix(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+10);
}


static void m68000_bclr_s_aw(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AW;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+8);
}


static void m68000_bclr_s_al(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AL;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src & ~mask);
   USE_CLKS(12+12);
}


static void m68010_bkpt(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      m68000_breakpoint_acknowledge(0);
      USE_CLKS(11);
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68000_bra_8(void)
{
   branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
   USE_CLKS(10);
}


static void m68000_bra_16(void)
{
   branch_word(read_memory_16(g_cpu_pc));
   USE_CLKS(10);
}


static void m68000_bset_r_d(void)
{
   uint* d_dst = &DY;
   uint mask = 1 << (DX & 0x1f);

   g_cpu_z_flag = !(*d_dst & mask);
   *d_dst |= mask;
   USE_CLKS(8);
}


static void m68000_bset_r_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+4);
}


static void m68000_bset_r_pi(void)
{
   uint ea = EA_PI_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+4);
}


static void m68000_bset_r_pi7(void)
{
   uint ea = EA_PI7_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+4);
}


static void m68000_bset_r_pd(void)
{
   uint ea = EA_PD_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+6);
}


static void m68000_bset_r_pd7(void)
{
   uint ea = EA_PD7_8;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+6);
}


static void m68000_bset_r_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+8);
}


static void m68000_bset_r_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+10);
}


static void m68000_bset_r_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+8);
}


static void m68000_bset_r_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_8(ea);
   uint mask = 1 << (DX & 7);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(8+12);
}


static void m68000_bset_s_d(void)
{
   uint* d_dst = &DY;
   uint mask = 1 << (read_imm_8() & 0x1f);

   g_cpu_z_flag = !(*d_dst & mask);
   *d_dst |= mask;
   USE_CLKS(12);
}


static void m68000_bset_s_ai(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AI;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+4);
}


static void m68000_bset_s_pi(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PI_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+4);
}


static void m68000_bset_s_pi7(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PI7_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+4);
}


static void m68000_bset_s_pd(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PD_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+6);
}


static void m68000_bset_s_pd7(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_PD7_8;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+6);
}


static void m68000_bset_s_di(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_DI;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+8);
}


static void m68000_bset_s_ix(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+10);
}


static void m68000_bset_s_aw(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AW;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+8);
}


static void m68000_bset_s_al(void)
{
   uint mask = 1 << (read_imm_8() & 7);
   uint ea = EA_AL;
   uint src = read_memory_8(ea);

   g_cpu_z_flag = !(src & mask);
   write_memory_8(ea, src | mask);
   USE_CLKS(12+12);
}


static void m68000_bsr_8(void)
{
   push_32(g_cpu_pc);
   branch_byte(MASK_OUT_ABOVE_8(g_cpu_ir));
   USE_CLKS(18);
}


static void m68000_bsr_16(void)
{
   push_32(g_cpu_pc+2);
   branch_word(read_memory_16(g_cpu_pc));
   USE_CLKS(18);
}


static void m68000_btst_r_d(void)
{
   g_cpu_z_flag = !(DY & (1 << (DX & 0x1f)));
   USE_CLKS(6);
}


static void m68000_btst_r_ai(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_AI) & (1 << (DX & 7)));
   USE_CLKS(4+4);
}


static void m68000_btst_r_pi(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_PI_8) & (1 << (DX & 7)));
   USE_CLKS(4+4);
}


static void m68000_btst_r_pi7(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_PI7_8) & (1 << (DX & 7)));
   USE_CLKS(4+4);
}


static void m68000_btst_r_pd(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_PD_8) & (1 << (DX & 7)));
   USE_CLKS(4+6);
}


static void m68000_btst_r_pd7(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_PD7_8) & (1 << (DX & 7)));
   USE_CLKS(4+6);
}


static void m68000_btst_r_di(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_DI) & (1 << (DX & 7)));
   USE_CLKS(4+8);
}


static void m68000_btst_r_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   g_cpu_z_flag = !(read_memory_8(ea) & (1 << (DX & 7)));
   USE_CLKS(4+10);
}


static void m68000_btst_r_aw(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_AW) & (1 << (DX & 7)));
   USE_CLKS(4+8);
}


static void m68000_btst_r_al(void)
{
   g_cpu_z_flag = !(read_memory_8(EA_AL) & (1 << (DX & 7)));
   USE_CLKS(4+12);
}


static void m68000_btst_r_pcdi(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   g_cpu_z_flag = !(read_memory_8(ea) & (1 << (DX & 7)));
   USE_CLKS(4+8);
}


static void m68000_btst_r_pcix(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   g_cpu_z_flag = !(read_memory_8(ea) & (1 << (DX & 7)));
   USE_CLKS(4+10);
}


static void m68000_btst_r_i(void)
{
   g_cpu_z_flag = !(read_imm_8() & (1 << (DX & 7)));
   USE_CLKS(4+4);
}


static void m68000_btst_s_d(void)
{
   g_cpu_z_flag = !(DY & (1 << (read_imm_8() & 0x1f)));
   USE_CLKS(10);
}


static void m68000_btst_s_ai(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_AI) & (1 << bit));
   USE_CLKS(8+4);
}


static void m68000_btst_s_pi(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_PI_8) & (1 << bit));
   USE_CLKS(8+4);
}


static void m68000_btst_s_pi7(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_PI7_8) & (1 << bit));
   USE_CLKS(8+4);
}


static void m68000_btst_s_pd(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_PD_8) & (1 << bit));
   USE_CLKS(8+6);
}


static void m68000_btst_s_pd7(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_PD7_8) & (1 << bit));
   USE_CLKS(8+6);
}


static void m68000_btst_s_di(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_DI) & (1 << bit));
   USE_CLKS(8+8);
}


static void m68000_btst_s_ix(void)
{
   uint bit = read_imm_8() & 7;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   g_cpu_z_flag = !(read_memory_8(ea) & (1 << bit));
   USE_CLKS(8+10);
}


static void m68000_btst_s_aw(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_AW) & (1 << bit));
   USE_CLKS(8+8);
}


static void m68000_btst_s_al(void)
{
   uint bit = read_imm_8() & 7;
   g_cpu_z_flag = !(read_memory_8(EA_AL) & (1 << bit));
   USE_CLKS(8+12);
}


static void m68000_btst_s_pcdi(void)
{
   uint bit = read_imm_8() & 7;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   g_cpu_z_flag = !(read_memory_8(ea) & (1 << bit));
   USE_CLKS(8+8);
}


static void m68000_btst_s_pcix(void)
{
   uint bit = read_imm_8() & 7;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   g_cpu_z_flag = !(read_memory_8(ea) & (1 << bit));
   USE_CLKS(8+10);
}


static void m68000_chk_d(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(MASK_OUT_ABOVE_16(DY));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_ai(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(read_memory_16(EA_AI));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+4);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_pi(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(read_memory_16(EA_PI_16));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+4);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_pd(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(read_memory_16(EA_PD_16));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+6);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_di(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(read_memory_16(EA_DI));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+8);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_ix(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   int bound = make_int_16(read_memory_16(ea));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+10);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_aw(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(read_memory_16(EA_AW));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+8);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_al(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(read_memory_16(EA_AL));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+12);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_pcdi(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   int bound = make_int_16(read_memory_16(ea));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+8);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_pcix(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   int bound = make_int_16(read_memory_16(ea));

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+10);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_chk_i(void)
{
   int src = make_int_16(MASK_OUT_ABOVE_16(DX));
   int bound = make_int_16(read_imm_16());

   if(src >= 0 && src <= bound)
   {
      USE_CLKS(10+4);
      return;
   }
   g_cpu_n_flag = src < 0;
   exception(EXCEPTION_CHK);
}


static void m68000_clr_d_8(void)
{
   DY &= 0xffffff00;

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(4);
}


static void m68000_clr_ai_8(void)
{
   write_memory_8(EA_AI, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+4);
}


static void m68000_clr_pi_8(void)
{
   write_memory_8(EA_PI_8, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+4);
}


static void m68000_clr_pi7_8(void)
{
   write_memory_8(EA_PI7_8, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+4);
}


static void m68000_clr_pd_8(void)
{
   write_memory_8(EA_PD_8, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+6);
}


static void m68000_clr_pd7_8(void)
{
   write_memory_8(EA_PD7_8, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+6);
}


static void m68000_clr_di_8(void)
{
   write_memory_8(EA_DI, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+8);
}


static void m68000_clr_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+10);
}


static void m68000_clr_aw_8(void)
{
   write_memory_8(EA_AW, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+8);
}


static void m68000_clr_al_8(void)
{
   write_memory_8(EA_AL, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+12);
}


static void m68000_clr_d_16(void)
{
   DY &= 0xffff0000;

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(4);
}


static void m68000_clr_ai_16(void)
{
   write_memory_16(EA_AI, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+4);
}


static void m68000_clr_pi_16(void)
{
   write_memory_16(EA_PI_16, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+4);
}


static void m68000_clr_pd_16(void)
{
   write_memory_16(EA_PD_16, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+6);
}


static void m68000_clr_di_16(void)
{
   write_memory_16(EA_DI, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+8);
}


static void m68000_clr_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_16(ea, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+10);
}


static void m68000_clr_aw_16(void)
{
   write_memory_16(EA_AW, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+8);
}


static void m68000_clr_al_16(void)
{
   write_memory_16(EA_AL, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(8+12);
}


static void m68000_clr_d_32(void)
{
   DY = 0;

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(6);
}


static void m68000_clr_ai_32(void)
{
   write_memory_32(EA_AI, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(12+8);
}


static void m68000_clr_pi_32(void)
{
   write_memory_32(EA_PI_32, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(12+8);
}


static void m68000_clr_pd_32(void)
{
   write_memory_32(EA_PD_32, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(12+10);
}


static void m68000_clr_di_32(void)
{
   write_memory_32(EA_DI, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(12+12);
}


static void m68000_clr_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_32(ea, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(12+14);
}


static void m68000_clr_aw_32(void)
{
   write_memory_32(EA_AW, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(12+12);
}


static void m68000_clr_al_32(void)
{
   write_memory_32(EA_AL, 0);

   g_cpu_n_flag = g_cpu_v_flag = g_cpu_c_flag = 0;
   g_cpu_z_flag = 1;
   USE_CLKS(12+16);
}


static void m68000_cmp_d_8(void)
{
   uint src = DY;
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4);
}


static void m68000_cmp_ai_8(void)
{
   uint src = read_memory_8(EA_AI);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+4);
}


static void m68000_cmp_pi_8(void)
{
   uint src = read_memory_8(EA_PI_8);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+4);
}


static void m68000_cmp_pi7_8(void)
{
   uint src = read_memory_8(EA_PI7_8);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+4);
}


static void m68000_cmp_pd_8(void)
{
   uint src = read_memory_8(EA_PD_8);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+6);
}


static void m68000_cmp_pd7_8(void)
{
   uint src = read_memory_8(EA_PD7_8);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+6);
}


static void m68000_cmp_di_8(void)
{
   uint src = read_memory_8(EA_DI);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+8);
}


static void m68000_cmp_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+10);
}


static void m68000_cmp_aw_8(void)
{
   uint src = read_memory_8(EA_AW);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+8);
}


static void m68000_cmp_al_8(void)
{
   uint src = read_memory_8(EA_AL);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+12);
}


static void m68000_cmp_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_8(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+8);
}


static void m68000_cmp_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+10);
}


static void m68000_cmp_i_8(void)
{
   uint src = read_imm_8();
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+4);
}


static void m68000_cmp_d_16(void)
{
   uint src = DY;
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4);
}


static void m68000_cmp_a_16(void)
{
   uint src = AY;
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4);
}


static void m68000_cmp_ai_16(void)
{
   uint src = read_memory_16(EA_AI);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+4);
}


static void m68000_cmp_pi_16(void)
{
   uint src = read_memory_16(EA_PI_16);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+4);
}


static void m68000_cmp_pd_16(void)
{
   uint src = read_memory_16(EA_PD_16);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+6);
}


static void m68000_cmp_di_16(void)
{
   uint src = read_memory_16(EA_DI);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+8);
}


static void m68000_cmp_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+10);
}


static void m68000_cmp_aw_16(void)
{
   uint src = read_memory_16(EA_AW);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+8);
}


static void m68000_cmp_al_16(void)
{
   uint src = read_memory_16(EA_AL);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+12);
}


static void m68000_cmp_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_16(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+8);
}


static void m68000_cmp_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+10);
}


static void m68000_cmp_i_16(void)
{
   uint src = read_imm_16();
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(4+4);
}


static void m68000_cmp_d_32(void)
{
   uint src = DY;
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6);
}


static void m68000_cmp_a_32(void)
{
   uint src = AY;
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6);
}


static void m68000_cmp_ai_32(void)
{
   uint src = read_memory_32(EA_AI);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmp_pi_32(void)
{
   uint src = read_memory_32(EA_PI_32);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmp_pd_32(void)
{
   uint src = read_memory_32(EA_PD_32);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+10);
}


static void m68000_cmp_di_32(void)
{
   uint src = read_memory_32(EA_DI);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+12);
}


static void m68000_cmp_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+14);
}


static void m68000_cmp_aw_32(void)
{
   uint src = read_memory_32(EA_AW);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+12);
}


static void m68000_cmp_al_32(void)
{
   uint src = read_memory_32(EA_AL);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+16);
}


static void m68000_cmp_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_32(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+12);
}


static void m68000_cmp_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+14);
}


static void m68000_cmp_i_32(void)
{
   uint src = read_imm_32();
   uint dst = DX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmpa_d_16(void)
{
   uint src = make_int_16(DY & 0xffff);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6);
}


static void m68000_cmpa_a_16(void)
{
   uint src = make_int_16(AY&0xffff);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6);
}


static void m68000_cmpa_ai_16(void)
{
   uint src = make_int_16(read_memory_16(EA_AI));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+4);
}


static void m68000_cmpa_pi_16(void)
{
   uint src = make_int_16(read_memory_16(EA_PI_16));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+4);
}


static void m68000_cmpa_pd_16(void)
{
   uint src = make_int_16(read_memory_16(EA_PD_16));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+6);
}


static void m68000_cmpa_di_16(void)
{
   uint src = make_int_16(read_memory_16(EA_DI));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmpa_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = make_int_16(read_memory_16(ea));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+10);
}


static void m68000_cmpa_aw_16(void)
{
   uint src = make_int_16(read_memory_16(EA_AW));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmpa_al_16(void)
{
   uint src = make_int_16(read_memory_16(EA_AL));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+12);
}


static void m68000_cmpa_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = make_int_16(read_memory_16(ea));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmpa_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = make_int_16(read_memory_16(ea));
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+10);
}


static void m68000_cmpa_i_16(void)
{
   uint src = make_int_16(read_imm_16());
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+4);
}


static void m68000_cmpa_d_32(void)
{
   uint src = DY;
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6);
}


static void m68000_cmpa_a_32(void)
{
   uint src = AY;
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6);
}


static void m68000_cmpa_ai_32(void)
{
   uint src = read_memory_32(EA_AI);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmpa_pi_32(void)
{
   uint src = read_memory_32(EA_PI_32);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmpa_pd_32(void)
{
   uint src = read_memory_32(EA_PD_32);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+10);
}


static void m68000_cmpa_di_32(void)
{
   uint src = read_memory_32(EA_DI);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+12);
}


static void m68000_cmpa_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+14);
}


static void m68000_cmpa_aw_32(void)
{
   uint src = read_memory_32(EA_AW);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+12);
}


static void m68000_cmpa_al_32(void)
{
   uint src = read_memory_32(EA_AL);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+16);
}


static void m68000_cmpa_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_32(ea);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+12);
}


static void m68000_cmpa_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+14);
}


static void m68000_cmpa_i_32(void)
{
   uint src = read_imm_32();
   uint dst = AX;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(6+8);
}


static void m68000_cmpi_d_8(void)
{
   uint src = read_imm_8();
   uint dst = DY;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8);
}


static void m68000_cmpi_ai_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_AI);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+4);
}


static void m68000_cmpi_pi_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_PI_8);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+4);
}


static void m68000_cmpi_pi7_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_PI7_8);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+4);
}


static void m68000_cmpi_pd_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_PD_8);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+6);
}


static void m68000_cmpi_pd7_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_PD7_8);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+6);
}


static void m68000_cmpi_di_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_DI);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+8);
}


static void m68000_cmpi_ix_8(void)
{
   uint src = read_imm_8();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+10);
}


static void m68000_cmpi_aw_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_AW);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+8);
}


static void m68000_cmpi_al_8(void)
{
   uint src = read_imm_8();
   uint dst = read_memory_8(EA_AL);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+12);
}


static void m68000_cmpi_d_16(void)
{
   uint src = read_imm_16();
   uint dst = DY;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8);
}


static void m68000_cmpi_ai_16(void)
{
   uint src = read_imm_16();
   uint dst = read_memory_16(EA_AI);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+4);
}


static void m68000_cmpi_pi_16(void)
{
   uint src = read_imm_16();
   uint dst = read_memory_16(EA_PI_16);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+4);
}


static void m68000_cmpi_pd_16(void)
{
   uint src = read_imm_16();
   uint dst = read_memory_16(EA_PD_16);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+6);
}


static void m68000_cmpi_di_16(void)
{
   uint src = read_imm_16();
   uint dst = read_memory_16(EA_DI);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+8);
}


static void m68000_cmpi_ix_16(void)
{
   uint src = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+10);
}


static void m68000_cmpi_aw_16(void)
{
   uint src = read_imm_16();
   uint dst = read_memory_16(EA_AW);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+8);
}


static void m68000_cmpi_al_16(void)
{
   uint src = read_imm_16();
   uint dst = read_memory_16(EA_AL);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(8+12);
}


static void m68000_cmpi_d_32(void)
{
   uint src = read_imm_32();
   uint dst = DY;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(14);
}


static void m68000_cmpi_ai_32(void)
{
   uint src = read_imm_32();
   uint dst = read_memory_32(EA_AI);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12+8);
}


static void m68000_cmpi_pi_32(void)
{
   uint src = read_imm_32();
   uint dst = read_memory_32(EA_PI_32);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12+8);
}


static void m68000_cmpi_pd_32(void)
{
   uint src = read_imm_32();
   uint dst = read_memory_32(EA_PD_32);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12+10);
}


static void m68000_cmpi_di_32(void)
{
   uint src = read_imm_32();
   uint dst = read_memory_32(EA_DI);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12+12);
}


static void m68000_cmpi_ix_32(void)
{
   uint src = read_imm_32();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12+14);
}


static void m68000_cmpi_aw_32(void)
{
   uint src = read_imm_32();
   uint dst = read_memory_32(EA_AW);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12+12);
}


static void m68000_cmpi_al_32(void)
{
   uint src = read_imm_32();
   uint dst = read_memory_32(EA_AL);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12+16);
}


static void m68000_cmpm_8_ax7(void)
{
   uint src = read_memory_8(AY++);
   uint dst = read_memory_8((g_cpu_ar[7]+=2)-2);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12);
}


static void m68000_cmpm_8_ay7(void)
{
   uint src = read_memory_8((g_cpu_ar[7]+=2)-2);
   uint dst = read_memory_8(AX++);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12);
}


static void m68000_cmpm_8_axy7(void)
{
   uint src = read_memory_8((g_cpu_ar[7]+=2)-2);
   uint dst = read_memory_8((g_cpu_ar[7]+=2)-2);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12);
}


static void m68000_cmpm_8(void)
{
   uint src = read_memory_8(AY++);
   uint dst = read_memory_8(AX++);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12);
}


static void m68000_cmpm_16(void)
{
   uint src = read_memory_16((AY+=2)-2);
   uint dst = read_memory_16((AX+=2)-2);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(12);
}


static void m68000_cmpm_32(void)
{
   uint src = read_memory_32((AY+=4)-4);
   uint dst = read_memory_32((AX+=4)-4);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   USE_CLKS(20);
}


static void m68000_dbt(void)
{
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbf(void)
{
   uint* d_reg = &DY;
   uint res = MASK_OUT_ABOVE_16(*d_reg-1);
   *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
   if(res != 0xffff)
   {
      branch_word(read_memory_16(g_cpu_pc));
      USE_CLKS(10);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(14);
}


static void m68000_dbhi(void)
{
   if(CONDITION_NOT_HI)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbls(void)
{
   if(CONDITION_NOT_LS)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbcc(void)
{
   if(CONDITION_NOT_CC)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbcs(void)
{
   if(CONDITION_NOT_CS)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbne(void)
{
   if(CONDITION_NOT_NE)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbeq(void)
{
   if(CONDITION_NOT_EQ)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbvc(void)
{
   if(CONDITION_NOT_VC)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbvs(void)
{
   if(CONDITION_NOT_VS)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbpl(void)
{
   if(CONDITION_NOT_PL)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbmi(void)
{
   if(CONDITION_NOT_MI)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbge(void)
{
   if(CONDITION_NOT_GE)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dblt(void)
{
   if(CONDITION_NOT_LT)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dbgt(void)
{
   if(CONDITION_NOT_GT)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_dble(void)
{
   if(CONDITION_NOT_LE)
   {
      uint* d_reg = &DY;
      uint res = MASK_OUT_ABOVE_16(*d_reg-1);
      *d_reg = MASK_OUT_BELOW_16(*d_reg) | res;
      if(res != 0xffff)
      {
         branch_word(read_memory_16(g_cpu_pc));
         USE_CLKS(10);
         return;
      }
      g_cpu_pc += 2;
      USE_CLKS(14);
      return;
   }
   g_cpu_pc += 2;
   USE_CLKS(12);
}


static void m68000_divs_d(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(MASK_OUT_ABOVE_16(DY));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = quotient == 0;
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_ai(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(read_memory_16(EA_AI));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+4);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+4);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_pi(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(read_memory_16(EA_PI_16));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+4);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+4);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_pd(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(read_memory_16(EA_PD_16));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+6);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+6);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_di(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(read_memory_16(EA_DI));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+8);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+8);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_ix(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   int src = make_int_16(read_memory_16(ea));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+10);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+10);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_aw(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(read_memory_16(EA_AW));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+8);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+8);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_al(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(read_memory_16(EA_AL));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+12);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+12);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_pcdi(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   int src = make_int_16(read_memory_16(ea));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+8);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+8);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_pcix(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   int src = make_int_16(read_memory_16(ea));

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+10);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+10);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divs_i(void)
{
   uint* d_dst = &DX;
   int src = make_int_16(read_imm_16());

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      int quotient = make_int_32(*d_dst) / src;
      int remainder = make_int_32(*d_dst) % src;

      if(quotient == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(158+4);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(158+4);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_d(void)
{
   uint* d_dst = &DX;
   uint src = MASK_OUT_ABOVE_16(DY);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_ai(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AI);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+4);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+4);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_pi(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_PI_16);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+4);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+4);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_pd(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_PD_16);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+6);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+6);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_di(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_DI);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+8);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+8);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_ix(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+10);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+10);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_aw(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AW);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+8);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+8);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_al(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AL);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+12);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+12);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_pcdi(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_16(ea);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+8);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+8);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_pcix(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+10);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+10);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_divu_i(void)
{
   uint* d_dst = &DX;
   uint src = read_imm_16();

   g_cpu_c_flag = 0;

   if(src != 0)
   {
      uint quotient = *d_dst / src;
      uint remainder = *d_dst % src;

      if(make_int_32(quotient) == make_int_16(quotient & 0xffff))
      {
         g_cpu_z_flag = (quotient == 0);
         g_cpu_n_flag = GET_MSB_16(quotient);
         g_cpu_v_flag = 0;
         *d_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
         USE_CLKS(140+4);
         return;
      }
      g_cpu_v_flag = 1;
      USE_CLKS(140+4);
      return;
   }
   exception(EXCEPTION_ZERO_DIVIDE);
}


static void m68000_eor_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY ^= MASK_OUT_ABOVE_8(DX));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_eor_ai_8(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_eor_pi_8(void)
{
   uint ea = EA_PI_8;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_eor_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_eor_pd_8(void)
{
   uint ea = EA_PD_8;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_eor_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_eor_di_8(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_eor_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_eor_aw_8(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_eor_al_8(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_8(DX ^ read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_eor_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY ^= MASK_OUT_ABOVE_16(DX));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_eor_ai_16(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_16(DX ^ read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_eor_pi_16(void)
{
   uint ea = EA_PI_16;
   uint res = MASK_OUT_ABOVE_16(DX ^ read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_eor_pd_16(void)
{
   uint ea = EA_PD_16;
   uint res = MASK_OUT_ABOVE_16(DX ^ read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_eor_di_16(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_16(DX ^ read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_eor_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16(DX ^ read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_eor_aw_16(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_16(DX ^ read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_eor_al_16(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_16(DX ^ read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_eor_d_32(void)
{
   uint res = DY ^= DX;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_eor_ai_32(void)
{
   uint ea = EA_AI;
   uint res = DX ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_eor_pi_32(void)
{
   uint ea = EA_PI_32;
   uint res = DX ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_eor_pd_32(void)
{
   uint ea = EA_PD_32;
   uint res = DX ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_eor_di_32(void)
{
   uint ea = EA_DI;
   uint res = DX ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_eor_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = DX ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_eor_aw_32(void)
{
   uint ea = EA_AW;
   uint res = DX ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_eor_al_32(void)
{
   uint ea = EA_AL;
   uint res = DX ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+16);
}


static void m68000_eori_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY ^= read_imm_8());

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_eori_ai_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AI;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_eori_pi_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PI_8;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_eori_pi7_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PI7_8;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_eori_pd_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PD_8;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_eori_pd7_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PD7_8;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_eori_di_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_DI;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_eori_ix_8(void)
{
   uint tmp = read_imm_8();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_eori_aw_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AW;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_eori_al_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AL;
   uint res = tmp ^ read_memory_8(ea);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_eori_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY ^= read_imm_16());

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_eori_ai_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AI;
   uint res = tmp ^ read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_eori_pi_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_PI_16;
   uint res = tmp ^ read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_eori_pd_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_PD_16;
   uint res = tmp ^ read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_eori_di_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_DI;
   uint res = tmp ^ read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_eori_ix_16(void)
{
   uint tmp = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = tmp ^ read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_eori_aw_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AW;
   uint res = tmp ^ read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_eori_al_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AL;
   uint res = tmp ^ read_memory_16(ea);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_eori_d_32(void)
{
   uint res = DY ^= read_imm_32();

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(16);
}


static void m68000_eori_ai_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AI;
   uint res = tmp ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_eori_pi_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_PI_32;
   uint res = tmp ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_eori_pd_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_PD_32;
   uint res = tmp ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+10);
}


static void m68000_eori_di_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_DI;
   uint res = tmp ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_eori_ix_32(void)
{
   uint tmp = read_imm_32();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = tmp ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+14);
}


static void m68000_eori_aw_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AW;
   uint res = tmp ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_eori_al_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AL;
   uint res = tmp ^ read_memory_32(ea);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+16);
}


static void m68000_eori_to_ccr(void)
{
   set_ccr(get_ccr() ^ read_imm_8());
   USE_CLKS(20);
}


static void m68000_eori_to_sr(void)
{
   uint eor_val = read_imm_16();

   if(g_cpu_s_flag)
   {
      set_sr(get_sr() ^ eor_val);
      USE_CLKS(20);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_exg_dd(void)
{
   uint* reg_a = &DX;
   uint* reg_b = &DY;
   uint tmp    = *reg_a;

   *reg_a = *reg_b;
   *reg_b = tmp;

   USE_CLKS(6);
}


static void m68000_exg_aa(void)
{
   uint* reg_a = &AX;
   uint* reg_b = &AY;
   uint tmp    = *reg_a;

   *reg_a = *reg_b;
   *reg_b = tmp;

   USE_CLKS(6);
}


static void m68000_exg_da(void)
{
   uint* reg_a = &DX;
   uint* reg_b = &AY;
   uint tmp    = *reg_a;

   *reg_a = *reg_b;
   *reg_b = tmp;

   USE_CLKS(6);
}


static void m68000_ext_16(void)
{
   uint* d_dst = &DY;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | (*d_dst & 0x00ff) | (GET_MSB_8(*d_dst) ? 0xff00 : 0);

   g_cpu_n_flag = GET_MSB_16(*d_dst);
   g_cpu_z_flag = MASK_OUT_ABOVE_16(*d_dst) == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_ext_32(void)
{
   uint* d_dst = &DY;

   *d_dst = (*d_dst & 0x0000ffff) | (BIT_F(*d_dst) ? 0xffff0000 : 0);

   g_cpu_n_flag = GET_MSB_32(*d_dst);
   g_cpu_z_flag = *d_dst == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_jmp_ai(void)
{
   branch_long(EA_AI);
   USE_CLKS(0+8);
}


static void m68000_jmp_di(void)
{
   branch_long(EA_DI);
   USE_CLKS(0+10);
}


static void m68000_jmp_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   branch_long(ea);
   USE_CLKS(0+14);
}


static void m68000_jmp_aw(void)
{
   branch_long(EA_AW);
   USE_CLKS(0+10);
}


static void m68000_jmp_al(void)
{
   branch_long(EA_AL);
   USE_CLKS(0+12);
}


static void m68000_jmp_pcdi(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   branch_long(ea);
   USE_CLKS(0+10);
}


static void m68000_jmp_pcix(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   branch_long(ea);
   USE_CLKS(0+14);
}


static void m68000_jsr_ai(void)
{
   uint ea = EA_AI;

   push_32(g_cpu_pc);
   branch_long(ea);
   USE_CLKS(0+16);
}


static void m68000_jsr_di(void)
{
   uint ea = EA_DI;

   push_32(g_cpu_pc);
   branch_long(ea);
   USE_CLKS(0+18);
}


static void m68000_jsr_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));

   push_32(g_cpu_pc);
   branch_long(ea);
   USE_CLKS(0+22);
}


static void m68000_jsr_aw(void)
{
   uint ea = EA_AW;

   push_32(g_cpu_pc);
   branch_long(ea);
   USE_CLKS(0+18);
}


static void m68000_jsr_al(void)
{
   uint ea = EA_AL;

   push_32(g_cpu_pc);
   branch_long(ea);
   USE_CLKS(0+20);
}


static void m68000_jsr_pcdi(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));

   push_32(g_cpu_pc);
   branch_long(ea);
   USE_CLKS(0+18);
}


static void m68000_jsr_pcix(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));

   push_32(g_cpu_pc);
   branch_long(ea);
   USE_CLKS(0+22);
}


static void m68000_lea_ai(void)
{
   AX = EA_AI;
   USE_CLKS(0+4);
}


static void m68000_lea_di(void)
{
   AX = EA_DI;
   USE_CLKS(0+8);
}


static void m68000_lea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   AX = ea;
   USE_CLKS(0+12);
}


static void m68000_lea_aw(void)
{
   AX = EA_AW;
   USE_CLKS(0+8);
}


static void m68000_lea_al(void)
{
   AX = EA_AL;
   USE_CLKS(0+12);
}


static void m68000_lea_pcdi(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   AX = ea;
   USE_CLKS(0+8);
}


static void m68000_lea_pcix(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   AX = ea;
   USE_CLKS(0+12);
}


static void m68000_link_a7(void)
{
   g_cpu_ar[7] -= 4;
   write_memory_32(g_cpu_ar[7], g_cpu_ar[7]);
   g_cpu_ar[7] = MASK_OUT_ABOVE_32(g_cpu_ar[7] + make_int_16(read_imm_16()));
   USE_CLKS(16);
}


static void m68000_link(void)
{
   uint* a_dst = &AY;

   push_32(*a_dst);
   *a_dst = g_cpu_ar[7];
   g_cpu_ar[7] = MASK_OUT_ABOVE_32(g_cpu_ar[7] + make_int_16(read_imm_16()));
   USE_CLKS(16);
}


static void m68000_lsr_s_8(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = src >> shift;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = 0;
   g_cpu_z_flag = res == 0;
   g_cpu_x_flag = g_cpu_c_flag = shift > 8 ? 0 : (src >> (shift-1)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_lsr_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = src >> shift;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = 0;
   g_cpu_z_flag = res == 0;
   g_cpu_x_flag = g_cpu_c_flag = (src >> (shift-1)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_lsr_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = *d_dst;
   uint res = src >> shift;

   *d_dst = res;

   g_cpu_n_flag = 0;
   g_cpu_z_flag = res == 0;
   g_cpu_x_flag = g_cpu_c_flag = (src >> (shift-1)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+8);
}


static void m68000_lsr_r_8(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = src >> shift;

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift <= 8)
      {
         *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (shift-1)) & 1;
         g_cpu_n_flag = 0;
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst &= 0xffffff00;
      g_cpu_x_flag = g_cpu_c_flag = 0;
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_lsr_r_16(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = src >> shift;

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift <= 16)
      {
         *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (shift-1)) & 1;
         g_cpu_n_flag = 0;
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst &= 0xffff0000;
      g_cpu_x_flag = g_cpu_c_flag = 0;
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_lsr_r_32(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = *d_dst;
   uint res = src >> shift;

   USE_CLKS((shift<<1)+8);
   if(shift != 0)
   {
      if(shift < 32)
      {
         *d_dst = res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (shift-1)) & 1;
         g_cpu_n_flag = 0;
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst = 0;
      g_cpu_x_flag = g_cpu_c_flag = (shift == 32 ? GET_MSB_32(src) : 0);
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_lsr_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   write_memory_16(ea, res);

   g_cpu_n_flag = 0;
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_lsr_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   write_memory_16(ea, res);

   g_cpu_n_flag = 0;
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_lsr_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   write_memory_16(ea, res);

   g_cpu_n_flag = 0;
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_lsr_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   write_memory_16(ea, res);

   g_cpu_n_flag = 0;
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_lsr_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   write_memory_16(ea, res);

   g_cpu_n_flag = 0;
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_lsr_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   write_memory_16(ea, res);

   g_cpu_n_flag = 0;
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_lsr_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint res = src >> 1;

   write_memory_16(ea, res);

   g_cpu_n_flag = 0;
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_lsl_s_8(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(src << shift);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = shift > 8 ? 0 : (src >> (8-shift)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_lsl_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(src << shift);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = (src >> (16-shift)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_lsl_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = *d_dst;
   uint res = MASK_OUT_ABOVE_32(src << shift);

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = (src >> (32-shift)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+8);
}


static void m68000_lsl_r_8(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(src << shift);

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift <= 8)
      {
         *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (8-shift)) & 1;
         g_cpu_n_flag = GET_MSB_8(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst &= 0xffffff00;
      g_cpu_x_flag = g_cpu_c_flag = 0;
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_lsl_r_16(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(src << shift);

   USE_CLKS((shift<<1)+6);
   if(shift != 0)
   {
      if(shift <= 16)
      {
         *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (16-shift)) & 1;
         g_cpu_n_flag = GET_MSB_16(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst &= 0xffff0000;
      g_cpu_x_flag = g_cpu_c_flag = 0;
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_lsl_r_32(void)
{
   uint* d_dst = &DY;
   uint shift = DX & 0x3f;
   uint src = *d_dst;
   uint res = MASK_OUT_ABOVE_32(src << shift);

   USE_CLKS((shift<<1)+8);
   if(shift != 0)
   {
      if(shift < 32)
      {
         *d_dst = res;
         g_cpu_x_flag = g_cpu_c_flag = (src >> (32-shift)) & 1;
         g_cpu_n_flag = GET_MSB_32(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }

      *d_dst = 0;
      g_cpu_x_flag = g_cpu_c_flag = (shift == 32 ? src & 1 : 0);
      g_cpu_n_flag = 0;
      g_cpu_z_flag = 1;
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_lsl_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_lsl_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_lsl_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_lsl_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_lsl_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_lsl_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_lsl_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(src << 1);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_dd_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_move_dd_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_move_dd_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_move_dd_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_move_dd_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_move_dd_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_move_dd_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_move_dd_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_move_dd_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_move_dd_i_8(void)
{
   uint res = read_imm_8();
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_move_ai_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_ai_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_ai_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_ai_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_ai_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_ai_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_ai_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_ai_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_ai_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_ai_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_ai_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_ai_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_ai_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi7_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pi_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pi7_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi7_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi7_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi7_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pi7_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pi7_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi7_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pi7_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi7_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_pi7_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi7_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pi7_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = (g_cpu_ar[7]+=2)-2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pi_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pi_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pi_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_pi_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pi_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = AX++;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd7_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pd_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pd7_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd7_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd7_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd7_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pd7_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pd7_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd7_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pd7_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd7_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_pd7_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd7_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pd7_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = g_cpu_ar[7]-=2;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pd_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pd_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pd_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_pd_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pd_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = --AX;

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_di_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_di_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_di_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_di_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_di_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_move_di_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_move_di_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_di_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_di_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_di_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_di_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_di_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_di_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_ix_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14);
}


static void m68000_move_ix_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+4);
}


static void m68000_move_ix_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+4);
}


static void m68000_move_ix_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+4);
}


static void m68000_move_ix_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+6);
}


static void m68000_move_ix_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+6);
}


static void m68000_move_ix_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+8);
}


static void m68000_move_ix_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+10);
}


static void m68000_move_ix_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+8);
}


static void m68000_move_ix_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+12);
}


static void m68000_move_ix_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+8);
}


static void m68000_move_ix_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+10);
}


static void m68000_move_ix_i_8(void)
{
   uint res = read_imm_8();
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+4);
}


static void m68000_move_aw_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_aw_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_aw_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_aw_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_aw_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_move_aw_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_move_aw_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_aw_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_aw_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_aw_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_aw_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_aw_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_aw_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_al_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16);
}


static void m68000_move_al_ai_8(void)
{
   uint res = read_memory_8(EA_AI);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+4);
}


static void m68000_move_al_pi_8(void)
{
   uint res = read_memory_8(EA_PI_8);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+4);
}


static void m68000_move_al_pi7_8(void)
{
   uint res = read_memory_8(EA_PI7_8);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+4);
}


static void m68000_move_al_pd_8(void)
{
   uint res = read_memory_8(EA_PD_8);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+6);
}


static void m68000_move_al_pd7_8(void)
{
   uint res = read_memory_8(EA_PD7_8);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+6);
}


static void m68000_move_al_di_8(void)
{
   uint res = read_memory_8(EA_DI);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_al_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+10);
}


static void m68000_move_al_aw_8(void)
{
   uint res = read_memory_8(EA_AW);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_al_al_8(void)
{
   uint res = read_memory_8(EA_AL);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_al_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_8(ea);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_al_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+10);
}


static void m68000_move_al_i_8(void)
{
   uint res = read_imm_8();
   uint ea_dst = read_imm_32();

   write_memory_8(ea_dst, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+4);
}


static void m68000_move_dd_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_move_dd_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_move_dd_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_move_dd_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_move_dd_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_move_dd_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_move_dd_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_move_dd_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_move_dd_i_16(void)
{
   uint res = read_imm_16();
   uint* d_dst = &DX;

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_move_ai_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_ai_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_ai_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_ai_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_ai_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_ai_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_ai_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_ai_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_ai_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_ai_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_ai_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_ai_i_16(void)
{
   uint res = read_imm_16();
   uint ea_dst = AX;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pi_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pi_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pi_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pi_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pi_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_pi_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pi_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pi_i_16(void)
{
   uint res = read_imm_16();
   uint ea_dst = (AX+=2) - 2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pd_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8);
}


static void m68000_move_pd_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_pd_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_move_pd_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pd_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_move_pd_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_move_pd_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_move_pd_i_16(void)
{
   uint res = read_imm_16();
   uint ea_dst = AX-=2;

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_move_di_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_di_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_di_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_di_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_di_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_move_di_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_di_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_di_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_di_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_di_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_di_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_di_i_16(void)
{
   uint res = read_imm_16();
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_ix_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14);
}


static void m68000_move_ix_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14);
}


static void m68000_move_ix_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+4);
}


static void m68000_move_ix_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+4);
}


static void m68000_move_ix_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+6);
}


static void m68000_move_ix_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+8);
}


static void m68000_move_ix_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+10);
}


static void m68000_move_ix_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+8);
}


static void m68000_move_ix_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+12);
}


static void m68000_move_ix_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+8);
}


static void m68000_move_ix_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+10);
}


static void m68000_move_ix_i_16(void)
{
   uint res = read_imm_16();
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(14+4);
}


static void m68000_move_aw_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_aw_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_aw_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_aw_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_aw_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_move_aw_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_aw_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_aw_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_aw_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_aw_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_aw_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_aw_i_16(void)
{
   uint res = read_imm_16();
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_move_al_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16);
}


static void m68000_move_al_a_16(void)
{
   uint res = MASK_OUT_ABOVE_16(AY);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16);
}


static void m68000_move_al_ai_16(void)
{
   uint res = read_memory_16(EA_AI);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+4);
}


static void m68000_move_al_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+4);
}


static void m68000_move_al_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+6);
}


static void m68000_move_al_di_16(void)
{
   uint res = read_memory_16(EA_DI);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_al_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+10);
}


static void m68000_move_al_aw_16(void)
{
   uint res = read_memory_16(EA_AW);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_al_al_16(void)
{
   uint res = read_memory_16(EA_AL);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_al_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_al_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+10);
}


static void m68000_move_al_i_16(void)
{
   uint res = read_imm_16();
   uint ea_dst = read_imm_32();

   write_memory_16(ea_dst, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+4);
}


static void m68000_move_dd_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_move_dd_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_move_dd_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_dd_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_move_dd_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_move_dd_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+14);
}


static void m68000_move_dd_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_move_dd_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+16);
}


static void m68000_move_dd_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_move_dd_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+14);
}


static void m68000_move_dd_i_32(void)
{
   uint res = read_imm_32();
   uint* d_dst = &DX;

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_move_ai_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_ai_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_ai_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_ai_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_ai_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_ai_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_ai_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_move_ai_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_ai_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+16);
}


static void m68000_move_ai_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_ai_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_move_ai_i_32(void)
{
   uint res = read_imm_32();
   uint ea_dst = AX;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_pi_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_pi_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_pi_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_pi_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_pi_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_pi_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_pi_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_move_pi_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_pi_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+16);
}


static void m68000_move_pi_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_pi_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_move_pi_i_32(void)
{
   uint res = read_imm_32();
   uint ea_dst = (AX+=4)-4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_pd_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_pd_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12);
}


static void m68000_move_pd_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_pd_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_pd_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_move_pd_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_pd_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_move_pd_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_pd_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+16);
}


static void m68000_move_pd_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_move_pd_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_move_pd_i_32(void)
{
   uint res = read_imm_32();
   uint ea_dst = AX-=4;

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_move_di_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16);
}


static void m68000_move_di_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16);
}


static void m68000_move_di_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_di_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_di_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+10);
}


static void m68000_move_di_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_di_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+14);
}


static void m68000_move_di_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_di_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+16);
}


static void m68000_move_di_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_di_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+14);
}


static void m68000_move_di_i_32(void)
{
   uint res = read_imm_32();
   uint ea_dst = AX + make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_ix_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18);
}


static void m68000_move_ix_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18);
}


static void m68000_move_ix_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+8);
}


static void m68000_move_ix_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+8);
}


static void m68000_move_ix_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+10);
}


static void m68000_move_ix_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+12);
}


static void m68000_move_ix_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+14);
}


static void m68000_move_ix_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+12);
}


static void m68000_move_ix_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+16);
}


static void m68000_move_ix_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+12);
}


static void m68000_move_ix_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+14);
}


static void m68000_move_ix_i_32(void)
{
   uint res = read_imm_32();
   uint extension_dst = read_imm_16();
   uint index_dst = g_cpu_dar[BIT_F(extension_dst)!=0][(extension_dst>>12) & 7];
   uint ea_dst = AX  + make_int_8(extension_dst & 0xff) + (BIT_B(extension_dst) ? index_dst : make_int_16(index_dst & 0xffff));

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(18+8);
}


static void m68000_move_aw_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16);
}


static void m68000_move_aw_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16);
}


static void m68000_move_aw_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_aw_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_aw_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+10);
}


static void m68000_move_aw_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_aw_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+14);
}


static void m68000_move_aw_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_aw_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+16);
}


static void m68000_move_aw_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+12);
}


static void m68000_move_aw_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+14);
}


static void m68000_move_aw_i_32(void)
{
   uint res = read_imm_32();
   uint ea_dst = make_int_16(read_imm_16());

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(16+8);
}


static void m68000_move_al_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20);
}


static void m68000_move_al_a_32(void)
{
   uint res = MASK_OUT_ABOVE_32(AY);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20);
}


static void m68000_move_al_ai_32(void)
{
   uint res = read_memory_32(EA_AI);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_move_al_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_move_al_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+10);
}


static void m68000_move_al_di_32(void)
{
   uint res = read_memory_32(EA_DI);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_move_al_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+14);
}


static void m68000_move_al_aw_32(void)
{
   uint res = read_memory_32(EA_AW);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_move_al_al_32(void)
{
   uint res = read_memory_32(EA_AL);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+16);
}


static void m68000_move_al_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_32(ea);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_move_al_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+14);
}


static void m68000_move_al_i_32(void)
{
   uint res = read_imm_32();
   uint ea_dst = read_imm_32();

   write_memory_32(ea_dst, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_movea_d_16(void)
{
   AX = make_int_16(MASK_OUT_ABOVE_16(DY));
   USE_CLKS(4);
}


static void m68000_movea_a_16(void)
{
   AX = make_int_16(MASK_OUT_ABOVE_16(AY));
   USE_CLKS(4);
}


static void m68000_movea_ai_16(void)
{
   AX = make_int_16(read_memory_16(EA_AI));
   USE_CLKS(4+4);
}


static void m68000_movea_pi_16(void)
{
   AX = make_int_16(read_memory_16(EA_PI_16));
   USE_CLKS(4+4);
}


static void m68000_movea_pd_16(void)
{
   AX = make_int_16(read_memory_16(EA_PD_16));
   USE_CLKS(4+6);
}


static void m68000_movea_di_16(void)
{
   AX = make_int_16(read_memory_16(EA_DI));
   USE_CLKS(4+8);
}


static void m68000_movea_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   AX = make_int_16(read_memory_16(ea));
   USE_CLKS(4+10);
}


static void m68000_movea_aw_16(void)
{
   AX = make_int_16(read_memory_16(EA_AW));
   USE_CLKS(4+8);
}


static void m68000_movea_al_16(void)
{
   AX = make_int_16(read_memory_16(EA_AL));
   USE_CLKS(4+12);
}


static void m68000_movea_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   AX = make_int_16(read_memory_16(ea));
   USE_CLKS(4+8);
}


static void m68000_movea_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   AX = make_int_16(read_memory_16(ea));
   USE_CLKS(4+10);
}


static void m68000_movea_i_16(void)
{
   AX = make_int_16(read_imm_16());
   USE_CLKS(4+4);
}


static void m68000_movea_d_32(void)
{
   AX = MASK_OUT_ABOVE_32(DY);
   USE_CLKS(4);
}


static void m68000_movea_a_32(void)
{
   AX = MASK_OUT_ABOVE_32(AY);
   USE_CLKS(4);
}


static void m68000_movea_ai_32(void)
{
   AX = read_memory_32(EA_AI);
   USE_CLKS(4+8);
}


static void m68000_movea_pi_32(void)
{
   AX = read_memory_32(EA_PI_32);
   USE_CLKS(4+8);
}


static void m68000_movea_pd_32(void)
{
   AX = read_memory_32(EA_PD_32);
   USE_CLKS(4+10);
}


static void m68000_movea_di_32(void)
{
   AX = read_memory_32(EA_DI);
   USE_CLKS(4+12);
}


static void m68000_movea_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   AX = read_memory_32(ea);
   USE_CLKS(4+14);
}


static void m68000_movea_aw_32(void)
{
   AX = read_memory_32(EA_AW);
   USE_CLKS(4+12);
}


static void m68000_movea_al_32(void)
{
   AX = read_memory_32(EA_AL);
   USE_CLKS(4+16);
}


static void m68000_movea_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   AX = read_memory_32(ea);
   USE_CLKS(4+12);
}


static void m68000_movea_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   AX = read_memory_32(ea);
   USE_CLKS(4+14);
}


static void m68000_movea_i_32(void)
{
   AX = read_imm_32();
   USE_CLKS(4+8);
}


static void m68010_move_fr_ccr_d(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      DY = MASK_OUT_BELOW_8(DY) | get_ccr();
      USE_CLKS(4);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_move_fr_ccr_ai(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      write_memory_8(EA_AI, get_ccr());
      USE_CLKS(8+4);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_move_fr_ccr_pi(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      write_memory_8(EA_PI_16, get_ccr());
      USE_CLKS(8+4);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_move_fr_ccr_pd(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      write_memory_8(EA_PD_16, get_ccr());
      USE_CLKS(8+6);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_move_fr_ccr_di(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      write_memory_8(EA_DI, get_ccr());
      USE_CLKS(8+8);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_move_fr_ccr_ix(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint extension = read_imm_16();
      uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
      uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
      write_memory_8(ea, get_ccr());
      USE_CLKS(8+10);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_move_fr_ccr_aw(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      write_memory_8(EA_AW, get_ccr());
      USE_CLKS(8+8);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_move_fr_ccr_al(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      write_memory_8(EA_AL, get_ccr());
      USE_CLKS(8+12);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68000_move_to_ccr_d(void)
{
   set_ccr(DY);
   USE_CLKS(12);
}


static void m68000_move_to_ccr_ai(void)
{
   set_ccr(read_memory_16(EA_AI));
   USE_CLKS(12+4);
}


static void m68000_move_to_ccr_pi(void)
{
   set_ccr(read_memory_16(EA_PI_16));
   USE_CLKS(12+4);
}


static void m68000_move_to_ccr_pd(void)
{
   set_ccr(read_memory_16(EA_PD_16));
   USE_CLKS(12+6);
}


static void m68000_move_to_ccr_di(void)
{
   set_ccr(read_memory_16(EA_DI));
   USE_CLKS(12+8);
}


static void m68000_move_to_ccr_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   set_ccr(read_memory_16(ea));
   USE_CLKS(12+10);
}


static void m68000_move_to_ccr_aw(void)
{
   set_ccr(read_memory_16(EA_AW));
   USE_CLKS(12+8);
}


static void m68000_move_to_ccr_al(void)
{
   set_ccr(read_memory_16(EA_AL));
   USE_CLKS(12+12);
}


static void m68000_move_to_ccr_pcdi(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   set_ccr(read_memory_16(ea));
   USE_CLKS(12+8);
}


static void m68000_move_to_ccr_pcix(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   set_ccr(read_memory_16(ea));
   USE_CLKS(12+10);
}


static void m68000_move_to_ccr_i(void)
{
   set_ccr(read_imm_16());
   USE_CLKS(12+4);
}


static void m68000_move_fr_sr_d(void)
{
   if(g_cpu_s_flag)
   {
      DY = MASK_OUT_BELOW_16(DY) | get_sr();
      USE_CLKS(6);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_sr_ai(void)
{
   uint ea = EA_AI;

   if(g_cpu_s_flag)
   {
      write_memory_16(ea, get_sr());
      USE_CLKS(8+4);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_sr_pi(void)
{
   uint ea = EA_PI_16;

   if(g_cpu_s_flag)
   {
      write_memory_16(ea, get_sr());
      USE_CLKS(8+4);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_sr_pd(void)
{
   uint ea = EA_PD_16;

   if(g_cpu_s_flag)
   {
      write_memory_16(ea, get_sr());
      USE_CLKS(8+6);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_sr_di(void)
{
   uint ea = EA_DI;

   if(g_cpu_s_flag)
   {
      write_memory_16(ea, get_sr());
      USE_CLKS(8+8);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_sr_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));

   if(g_cpu_s_flag)
   {
      write_memory_16(ea, get_sr());
      USE_CLKS(8+10);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_sr_aw(void)
{
   uint ea = EA_AW;

   if(g_cpu_s_flag)
   {
      write_memory_16(ea, get_sr());
      USE_CLKS(8+8);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_sr_al(void)
{
   uint ea = EA_AL;

   if(g_cpu_s_flag)
   {
      write_memory_16(ea, get_sr());
      USE_CLKS(8+12);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_d(void)
{
   if(g_cpu_s_flag)
   {
      set_sr(DY);
      USE_CLKS(12);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_ai(void)
{
   uint new_sr = read_memory_16(EA_AI);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+4);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_pi(void)
{
   uint new_sr = read_memory_16(EA_PI_16);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+4);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_pd(void)
{
   uint new_sr = read_memory_16(EA_PD_16);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+6);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_di(void)
{
   uint new_sr = read_memory_16(EA_DI);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+8);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint new_sr = read_memory_16(ea);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+10);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_aw(void)
{
   uint new_sr = read_memory_16(EA_AW);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+8);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_al(void)
{
   uint new_sr = read_memory_16(EA_AL);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+12);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_pcdi(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint new_sr = read_memory_16(ea);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+8);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_pcix(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint new_sr = read_memory_16(ea);

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+10);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_sr_i(void)
{
   uint new_sr = read_imm_16();

   if(g_cpu_s_flag)
   {
      set_sr(new_sr);
      USE_CLKS(12+4);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_fr_usp(void)
{
   if(g_cpu_s_flag)
   {
      AY = g_cpu_usp;
      USE_CLKS(4);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_move_to_usp(void)
{
   if(g_cpu_s_flag)
   {
      g_cpu_usp = AY;
      USE_CLKS(4);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68010_movec_cr(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      if(g_cpu_s_flag)
      {
         uint next_word = read_imm_16();
         USE_CLKS(12);
         switch(next_word & 0xfff)
         {
            case 0x000: /* SFC */
               g_cpu_dar[next_word>>15][(next_word>>12)&7] = g_cpu_sfc;
               return;
            case 0x001: /* DFC */
               g_cpu_dar[next_word>>15][(next_word>>12)&7] = g_cpu_dfc;
               return;
            case 0x800: /* USP */
               g_cpu_dar[next_word>>15][(next_word>>12)&7] = g_cpu_usp;
               return;
            case 0x801: /* VBR */
               g_cpu_dar[next_word>>15][(next_word>>12)&7] = g_cpu_vbr;
               return;
            default:
               exception(EXCEPTION_ILLEGAL_INSTRUCTION);
               return;
         }
      }
      exception(EXCEPTION_PRIVILEGE_VIOLATION);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_movec_rc(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      if(g_cpu_s_flag)
      {
         uint next_word = read_imm_16();
         USE_CLKS(10);
         switch(next_word & 0xfff)
         {
            case 0x000: /* SFC */
               g_cpu_sfc = g_cpu_dar[next_word>>15][(next_word>>12)&7] & 7;
               return;
            case 0x001: /* DFC */
               g_cpu_dfc = g_cpu_dar[next_word>>15][(next_word>>12)&7] & 7;
               return;
            case 0x800: /* USP */
               g_cpu_usp = g_cpu_dar[next_word>>15][(next_word>>12)&7];
               return;
            case 0x801: /* VBR */
               g_cpu_vbr = g_cpu_dar[next_word>>15][(next_word>>12)&7];
               return;
            default:
               exception(EXCEPTION_ILLEGAL_INSTRUCTION);
               return;
         }
      }
      exception(EXCEPTION_PRIVILEGE_VIOLATION);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68000_movem_pd_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = AY;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_16(ea -= 2, *(g_movem_pd_table[i]));
         count++;
      }
   AY = ea;
   USE_CLKS((count<<2)+8);
}


static void m68000_movem_pd_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = AY;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_32(ea -= 4, *(g_movem_pd_table[i]));
         count++;
      }
   AY = ea;
   USE_CLKS((count<<4)+8);
}


static void m68000_movem_pi_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = AY;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   AY = ea;
   USE_CLKS((count<<2)+12);
}


static void m68000_movem_pi_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = AY;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   AY = ea;
   USE_CLKS((count<<4)+12);
}


static void m68000_movem_re_ai_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_16(ea, *(g_movem_pi_table[i]));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+4+4);
}


static void m68000_movem_re_pd_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_PD_16;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_16(ea, *(g_movem_pi_table[i]));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+4+6);
}


static void m68000_movem_re_di_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_DI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_16(ea, *(g_movem_pi_table[i]));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+4+8);
}


static void m68000_movem_re_ix_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_16(ea, *(g_movem_pi_table[i]));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+4+10);
}


static void m68000_movem_re_aw_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AW;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_16(ea, *(g_movem_pi_table[i]));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+4+8);
}


static void m68000_movem_re_al_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AL;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_16(ea, *(g_movem_pi_table[i]));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+4+12);
}


static void m68000_movem_re_ai_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_32(ea, *(g_movem_pi_table[i]));
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+4+8);
}


static void m68000_movem_re_pd_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_PD_32;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_32(ea, *(g_movem_pi_table[i]));
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+4+10);
}


static void m68000_movem_re_di_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_DI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_32(ea, *(g_movem_pi_table[i]));
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+4+12);
}


static void m68000_movem_re_ix_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_32(ea, *(g_movem_pi_table[i]));
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+4+14);
}


static void m68000_movem_re_aw_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AW;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_32(ea, *(g_movem_pi_table[i]));
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+4+12);
}


static void m68000_movem_re_al_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AL;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         write_memory_32(ea, *(g_movem_pi_table[i]));
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+4+16);
}


static void m68000_movem_er_ai_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+4);
}


static void m68000_movem_er_pi_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_PI_16;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+4);
}


static void m68000_movem_er_di_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_DI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+8);
}


static void m68000_movem_er_ix_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+10);
}


static void m68000_movem_er_aw_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AW;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+8);
}


static void m68000_movem_er_al_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AL;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+12);
}


static void m68000_movem_er_pcdi_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+8);
}


static void m68000_movem_er_pcix_16(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = make_int_16(MASK_OUT_ABOVE_16(read_memory_16(ea)));
         ea += 2;
         count++;
      }
   USE_CLKS((count<<2)+8+10);
}


static void m68000_movem_er_ai_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+8);
}


static void m68000_movem_er_pi_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_PI_32;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+8);
}


static void m68000_movem_er_di_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_DI;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+12);
}


static void m68000_movem_er_ix_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+14);
}


static void m68000_movem_er_aw_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AW;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+12);
}


static void m68000_movem_er_al_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint ea = EA_AL;
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+16);
}


static void m68000_movem_er_pcdi_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+12);
}


static void m68000_movem_er_pcix_32(void)
{
   uint i = 0;
   uint register_list = read_imm_16();
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint count = 0;

   for(;i<16;i++)
      if(register_list & (1 << i))
      {
         *(g_movem_pi_table[i]) = read_memory_32(ea);
         ea += 4;
         count++;
      }
   USE_CLKS((count<<4)+8+14);
}


static void m68000_movep_re_16(void)
{
   uint ea = AY + make_int_16(MASK_OUT_ABOVE_16(read_imm_16()));
   uint src = DX;

   write_memory_8(ea   , MASK_OUT_ABOVE_8(src>>8));
   write_memory_8(ea+=2, MASK_OUT_ABOVE_8(src));
   USE_CLKS(16);
}


static void m68000_movep_re_32(void)
{
   uint ea = AY + make_int_16(MASK_OUT_ABOVE_16(read_imm_16()));
   uint src = DX;

   write_memory_8(ea,    MASK_OUT_ABOVE_8(src>>24));
   write_memory_8(ea+=2, MASK_OUT_ABOVE_8(src>>16));
   write_memory_8(ea+=2, MASK_OUT_ABOVE_8(src>>8));
   write_memory_8(ea+=2, MASK_OUT_ABOVE_8(src));
   USE_CLKS(24);
}


static void m68000_movep_er_16(void)
{
   uint ea = AY + make_int_16(MASK_OUT_ABOVE_16(read_imm_16()));
   uint* d_dst = &DX;

    *d_dst = MASK_OUT_BELOW_16(*d_dst) | ((read_memory_8(ea) << 8) + read_memory_8(ea+2));
   USE_CLKS(16);
}


static void m68000_movep_er_32(void)
{
   uint ea = AY + make_int_16(MASK_OUT_ABOVE_16(read_imm_16()));

   DX = (read_memory_8(ea) << 24)   + (read_memory_8(ea+2) << 16)
      + (read_memory_8(ea+4) <<  8) +  read_memory_8(ea+6);
   USE_CLKS(24);
}


static void m68010_moves_ai_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AI;
      USE_CLKS(0+18);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pi_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PI_8;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pi7_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PI7_8;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pd_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PD_8;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pd7_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PD7_8;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_di_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_DI;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_ix_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint extension = read_imm_16();
      uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
      uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
      USE_CLKS(0+24);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_aw_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AW;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_al_8(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AL;
      USE_CLKS(0+24);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_8_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_8(read_memory_8_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_8(g_cpu_dr[(next_word>>12)&7]) | read_memory_8_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_ai_16(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AI;
      USE_CLKS(0+18);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_16_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_16(read_memory_16_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_16(g_cpu_dr[(next_word>>12)&7]) | read_memory_16_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pi_16(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PI_16;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_16_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_16(read_memory_16_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_16(g_cpu_dr[(next_word>>12)&7]) | read_memory_16_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pd_16(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PD_16;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_16_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_16(read_memory_16_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_16(g_cpu_dr[(next_word>>12)&7]) | read_memory_16_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_di_16(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_DI;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_16_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_16(read_memory_16_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_16(g_cpu_dr[(next_word>>12)&7]) | read_memory_16_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_ix_16(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint extension = read_imm_16();
      uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
      uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
      USE_CLKS(0+24);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_16_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_16(read_memory_16_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_16(g_cpu_dr[(next_word>>12)&7]) | read_memory_16_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_aw_16(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AW;
      USE_CLKS(0+20);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_16_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_16(read_memory_16_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_16(g_cpu_dr[(next_word>>12)&7]) | read_memory_16_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_al_16(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AL;
      USE_CLKS(0+24);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_16_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      if(BIT_F(next_word)) /* Memory to address register */
      {
         g_cpu_ar[(next_word>>12)&7] = make_int_16(read_memory_16_fc(ea, g_cpu_sfc));
         return;
      }
      /* Memory to data register */
      g_cpu_dr[(next_word>>12)&7] = MASK_OUT_BELOW_16(g_cpu_dr[(next_word>>12)&7]) | read_memory_16_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_ai_32(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AI;
      USE_CLKS(0+8);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_32_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      /* Memory to register */
      g_cpu_dar[next_word>>15][(next_word>>12)&7] = read_memory_32_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pi_32(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PI_32;
      USE_CLKS(0+8);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_32_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      /* Memory to register */
      g_cpu_dar[next_word>>15][(next_word>>12)&7] = read_memory_32_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_pd_32(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_PD_32;
      USE_CLKS(0+10);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_32_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      /* Memory to register */
      g_cpu_dar[next_word>>15][(next_word>>12)&7] = read_memory_32_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_di_32(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_DI;
      USE_CLKS(0+12);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_32_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      /* Memory to register */
      g_cpu_dar[next_word>>15][(next_word>>12)&7] = read_memory_32_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_ix_32(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint extension = read_imm_16();
      uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
      uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
      USE_CLKS(0+14);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_32_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      /* Memory to register */
      g_cpu_dar[next_word>>15][(next_word>>12)&7] = read_memory_32_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_aw_32(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AW;
      USE_CLKS(0+12);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_32_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      /* Memory to register */
      g_cpu_dar[next_word>>15][(next_word>>12)&7] = read_memory_32_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68010_moves_al_32(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint next_word = read_imm_16();
      uint ea = EA_AL;
      USE_CLKS(0+16);
      if(BIT_B(next_word)) /* Register to memory */
      {
         write_memory_32_fc(ea, g_cpu_dfc, g_cpu_dar[next_word>>15][(next_word>>12)&7]);
         return;
      }
      /* Memory to register */
      g_cpu_dar[next_word>>15][(next_word>>12)&7] = read_memory_32_fc(ea, g_cpu_sfc);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68000_moveq(void)
{
   uint res = DX = make_int_8(MASK_OUT_ABOVE_8(g_cpu_ir));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_muls_d(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(MASK_OUT_ABOVE_16(DY)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54);
}


static void m68000_muls_ai(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(read_memory_16(EA_AI)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+4);
}


static void m68000_muls_pi(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(read_memory_16(EA_PI_16)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+4);
}


static void m68000_muls_pd(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(read_memory_16(EA_PD_16)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+6);
}


static void m68000_muls_di(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(read_memory_16(EA_DI)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+8);
}


static void m68000_muls_ix(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = make_int_16(read_memory_16(ea)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+10);
}


static void m68000_muls_aw(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(read_memory_16(EA_AW)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+8);
}


static void m68000_muls_al(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(read_memory_16(EA_AL)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+12);
}


static void m68000_muls_pcdi(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = make_int_16(read_memory_16(ea)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+8);
}


static void m68000_muls_pcix(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = make_int_16(read_memory_16(ea)) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+10);
}


static void m68000_muls_i(void)
{
   uint* d_dst = &DX;
   uint res = make_int_16(read_imm_16()) * make_int_16(MASK_OUT_ABOVE_16(*d_dst));

   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+4);
}


static void m68000_mulu_d(void)
{
   uint* d_dst = &DX;
   uint res = MASK_OUT_ABOVE_16(DY) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54);
}


static void m68000_mulu_ai(void)
{
   uint* d_dst = &DX;
   uint res = read_memory_16(EA_AI) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+4);
}


static void m68000_mulu_pi(void)
{
   uint* d_dst = &DX;
   uint res = read_memory_16(EA_PI_16) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+4);
}


static void m68000_mulu_pd(void)
{
   uint* d_dst = &DX;
   uint res = read_memory_16(EA_PD_16) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+6);
}


static void m68000_mulu_di(void)
{
   uint* d_dst = &DX;
   uint res = read_memory_16(EA_DI) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+8);
}


static void m68000_mulu_ix(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+10);
}


static void m68000_mulu_aw(void)
{
   uint* d_dst = &DX;
   uint res = read_memory_16(EA_AW) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+8);
}


static void m68000_mulu_al(void)
{
   uint* d_dst = &DX;
   uint res = read_memory_16(EA_AL) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+12);
}


static void m68000_mulu_pcdi(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = read_memory_16(ea) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+8);
}


static void m68000_mulu_pcix(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea) * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+10);
}


static void m68000_mulu_i(void)
{
   uint* d_dst = &DX;
   uint res = read_imm_16() * MASK_OUT_ABOVE_16(*d_dst);
   *d_dst = res;

   g_cpu_z_flag = res == 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(54+4);
}


static void m68000_nbcd_d(void)
{
   uint* d_dst = &DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(6);
}


static void m68000_nbcd_ai(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_nbcd_pi(void)
{
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_nbcd_pi7(void)
{
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_nbcd_pd(void)
{
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_nbcd_pd7(void)
{
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_nbcd_di(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_nbcd_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_nbcd_aw(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_nbcd_al(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(0x9a - dst - (g_cpu_x_flag != 0));

   if(res != 0x9a)
   {
      if((res & 0x0f) == 0xa)
         res = (res & 0xf0) + 0x10;

      write_memory_8(ea, res);

      if(res != 0)
         g_cpu_z_flag = 0;
      g_cpu_c_flag = g_cpu_x_flag = 1;
   }
   else
      g_cpu_c_flag = g_cpu_x_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_neg_d_8(void)
{
   uint *d_dst = &DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(-dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(4);
}


static void m68000_neg_ai_8(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+4);
}


static void m68000_neg_pi_8(void)
{
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+4);
}


static void m68000_neg_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+4);
}


static void m68000_neg_pd_8(void)
{
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+6);
}


static void m68000_neg_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+6);
}


static void m68000_neg_di_8(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+8);
}


static void m68000_neg_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+10);
}


static void m68000_neg_aw_8(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+8);
}


static void m68000_neg_al_8(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+12);
}


static void m68000_neg_d_16(void)
{
   uint *d_dst = &DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(-dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(4);
}


static void m68000_neg_ai_16(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+4);
}


static void m68000_neg_pi_16(void)
{
   uint ea = EA_PI_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+4);
}


static void m68000_neg_pd_16(void)
{
   uint ea = EA_PD_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+6);
}


static void m68000_neg_di_16(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+8);
}


static void m68000_neg_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+10);
}


static void m68000_neg_aw_16(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+8);
}


static void m68000_neg_al_16(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(8+12);
}


static void m68000_neg_d_32(void)
{
   uint *d_dst = &DY;
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(-dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(6);
}


static void m68000_neg_ai_32(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(12+8);
}


static void m68000_neg_pi_32(void)
{
   uint ea = EA_PI_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(12+8);
}


static void m68000_neg_pd_32(void)
{
   uint ea = EA_PD_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(12+10);
}


static void m68000_neg_di_32(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(12+12);
}


static void m68000_neg_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(12+14);
}


static void m68000_neg_aw_32(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(12+12);
}


static void m68000_neg_al_32(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = res != 0;
   USE_CLKS(12+16);
}


static void m68000_negx_d_8(void)
{
   uint *d_dst = &DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(4);
}


static void m68000_negx_ai_8(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+4);
}


static void m68000_negx_pi_8(void)
{
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+4);
}


static void m68000_negx_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+4);
}


static void m68000_negx_pd_8(void)
{
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+6);
}


static void m68000_negx_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+6);
}


static void m68000_negx_di_8(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+8);
}


static void m68000_negx_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+10);
}


static void m68000_negx_aw_8(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+8);
}


static void m68000_negx_al_8(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(-dst-(g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_8(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_8(dst | res);
   USE_CLKS(8+12);
}


static void m68000_negx_d_16(void)
{
   uint *d_dst = &DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(4);
}


static void m68000_negx_ai_16(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(8+4);
}


static void m68000_negx_pi_16(void)
{
   uint ea = EA_PI_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(8+4);
}


static void m68000_negx_pd_16(void)
{
   uint ea = EA_PD_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(8+6);
}


static void m68000_negx_di_16(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(8+8);
}


static void m68000_negx_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(8+10);
}


static void m68000_negx_aw_16(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(8+8);
}


static void m68000_negx_al_16(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(-dst-(g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_16(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_16(dst | res);
   USE_CLKS(8+12);
}


static void m68000_negx_d_32(void)
{
   uint *d_dst = &DY;
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(6);
}


static void m68000_negx_ai_32(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(12+8);
}


static void m68000_negx_pi_32(void)
{
   uint ea = EA_PI_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(12+8);
}


static void m68000_negx_pd_32(void)
{
   uint ea = EA_PD_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(12+10);
}


static void m68000_negx_di_32(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(12+12);
}


static void m68000_negx_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(12+14);
}


static void m68000_negx_aw_32(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(12+12);
}


static void m68000_negx_al_32(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(-dst-(g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_v_flag = GET_MSB_32(dst & res);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_32(dst | res);
   USE_CLKS(12+16);
}


static void m68000_nop(void)
{
   USE_CLKS(4);
}


static void m68000_not_d_8(void)
{
   uint* d_dst = &DY;
   uint res = MASK_OUT_ABOVE_8(~*d_dst);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_not_ai_8(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_not_pi_8(void)
{
   uint ea = EA_PI_8;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_not_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_not_pd_8(void)
{
   uint ea = EA_PD_8;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_not_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_not_di_8(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_not_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_not_aw_8(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_not_al_8(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_8(~read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_not_d_16(void)
{
   uint* d_dst = &DY;
   uint res = MASK_OUT_ABOVE_16(~*d_dst);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_not_ai_16(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_16(~read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_not_pi_16(void)
{
   uint ea = EA_PI_16;
   uint res = MASK_OUT_ABOVE_16(~read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_not_pd_16(void)
{
   uint ea = EA_PD_16;
   uint res = MASK_OUT_ABOVE_16(~read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_not_di_16(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_16(~read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_not_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16(~read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_not_aw_16(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_16(~read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_not_al_16(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_16(~read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_not_d_32(void)
{
   uint* d_dst = &DY;
   uint res = *d_dst = MASK_OUT_ABOVE_32(~*d_dst);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6);
}


static void m68000_not_ai_32(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_32(~read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_not_pi_32(void)
{
   uint ea = EA_PI_32;
   uint res = MASK_OUT_ABOVE_32(~read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_not_pd_32(void)
{
   uint ea = EA_PD_32;
   uint res = MASK_OUT_ABOVE_32(~read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_not_di_32(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_32(~read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_not_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_32(~read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_not_aw_32(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_32(~read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_not_al_32(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_32(~read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+16);
}


static void m68000_or_er_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= MASK_OUT_ABOVE_8(DY)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_or_er_ai_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_AI)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_or_er_pi_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_PI_8)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_or_er_pi7_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_PI7_8)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_or_er_pd_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_PD_8)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_or_er_pd7_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_PD7_8)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_or_er_di_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_DI)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_or_er_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(ea)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_or_er_aw_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_AW)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_or_er_al_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(EA_AL)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_or_er_pcdi_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(ea)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_or_er_pcix_8(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8((DX |= read_memory_8(ea)));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_or_er_i_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DX |= read_imm_8()));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_or_er_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= MASK_OUT_ABOVE_16(DY)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_or_er_ai_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(EA_AI)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_or_er_pi_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(EA_PI_16)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_or_er_pd_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(EA_PD_16)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_or_er_di_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(EA_DI)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_or_er_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(ea)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_or_er_aw_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(EA_AW)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_or_er_al_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(EA_AL)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_or_er_pcdi_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(ea)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_or_er_pcix_16(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16((DX |= read_memory_16(ea)));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_or_er_i_16(void)
{
   uint res = MASK_OUT_ABOVE_16((DX |= read_imm_16()));

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_or_er_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= DY));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_or_er_ai_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(EA_AI)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+8);
}


static void m68000_or_er_pi_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(EA_PI_32)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+8);
}


static void m68000_or_er_pd_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(EA_PD_32)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+10);
}


static void m68000_or_er_di_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(EA_DI)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+12);
}


static void m68000_or_er_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(ea)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+14);
}


static void m68000_or_er_aw_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(EA_AW)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+12);
}


static void m68000_or_er_al_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(EA_AL)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+16);
}


static void m68000_or_er_pcdi_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(ea)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+12);
}


static void m68000_or_er_pcix_32(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_32((DX |= read_memory_32(ea)));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+14);
}


static void m68000_or_er_i_32(void)
{
   uint res = MASK_OUT_ABOVE_32((DX |= read_imm_32()));

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(6+8);
}


static void m68000_or_re_ai_8(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_or_re_pi_8(void)
{
   uint ea = EA_PI_8;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_or_re_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_or_re_pd_8(void)
{
   uint ea = EA_PD_8;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_or_re_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_or_re_di_8(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_or_re_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_or_re_aw_8(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_or_re_al_8(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_8(DX | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_or_re_ai_16(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_16(DX | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_or_re_pi_16(void)
{
   uint ea = EA_PI_16;
   uint res = MASK_OUT_ABOVE_16(DX | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_or_re_pd_16(void)
{
   uint ea = EA_PD_16;
   uint res = MASK_OUT_ABOVE_16(DX | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_or_re_di_16(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_16(DX | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_or_re_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16(DX | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_or_re_aw_16(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_16(DX | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_or_re_al_16(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_16(DX | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_or_re_ai_32(void)
{
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_32(DX | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_or_re_pi_32(void)
{
   uint ea = EA_PI_32;
   uint res = MASK_OUT_ABOVE_32(DX | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_or_re_pd_32(void)
{
   uint ea = EA_PD_32;
   uint res = MASK_OUT_ABOVE_32(DX | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_or_re_di_32(void)
{
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_32(DX | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_or_re_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_32(DX | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+14);
}


static void m68000_or_re_aw_32(void)
{
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_32(DX | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_or_re_al_32(void)
{
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_32(DX | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+16);
}


static void m68000_ori_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8((DY |= read_imm_8()));

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_ori_ai_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_ori_pi_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PI_8;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_ori_pi7_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PI7_8;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_ori_pd_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PD_8;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_ori_pd7_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_PD7_8;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_ori_di_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_ori_ix_8(void)
{
   uint tmp = read_imm_8();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_ori_aw_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_ori_al_8(void)
{
   uint tmp = read_imm_8();
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_8(tmp | read_memory_8(ea));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_ori_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY |= read_imm_16());

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(8);
}


static void m68000_ori_ai_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_16(tmp | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_ori_pi_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_PI_16;
   uint res = MASK_OUT_ABOVE_16(tmp | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+4);
}


static void m68000_ori_pd_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_PD_16;
   uint res = MASK_OUT_ABOVE_16(tmp | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+6);
}


static void m68000_ori_di_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_16(tmp | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_ori_ix_16(void)
{
   uint tmp = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_16(tmp | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+10);
}


static void m68000_ori_aw_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_16(tmp | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+8);
}


static void m68000_ori_al_16(void)
{
   uint tmp = read_imm_16();
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_16(tmp | read_memory_16(ea));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(12+12);
}


static void m68000_ori_d_32(void)
{
   uint res = MASK_OUT_ABOVE_32(DY |= read_imm_32());

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(16);
}


static void m68000_ori_ai_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AI;
   uint res = MASK_OUT_ABOVE_32(tmp | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_ori_pi_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_PI_32;
   uint res = MASK_OUT_ABOVE_32(tmp | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+8);
}


static void m68000_ori_pd_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_PD_32;
   uint res = MASK_OUT_ABOVE_32(tmp | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+10);
}


static void m68000_ori_di_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_DI;
   uint res = MASK_OUT_ABOVE_32(tmp | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_ori_ix_32(void)
{
   uint tmp = read_imm_32();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = MASK_OUT_ABOVE_32(tmp | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+14);
}


static void m68000_ori_aw_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AW;
   uint res = MASK_OUT_ABOVE_32(tmp | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+12);
}


static void m68000_ori_al_32(void)
{
   uint tmp = read_imm_32();
   uint ea = EA_AL;
   uint res = MASK_OUT_ABOVE_32(tmp | read_memory_32(ea));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(20+16);
}


static void m68000_ori_to_ccr(void)
{
   set_ccr(get_ccr() | read_imm_8());
   USE_CLKS(20);
}


static void m68000_ori_to_sr(void)
{
   uint or_val = read_imm_16();

   if(g_cpu_s_flag)
   {
      set_sr(get_sr() | or_val);
      USE_CLKS(20);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_pea_ai(void)
{
   push_32(EA_AI);
   USE_CLKS(0+12);
}


static void m68000_pea_di(void)
{
   push_32(EA_DI);
   USE_CLKS(0+16);
}


static void m68000_pea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   push_32(ea);
   USE_CLKS(0+20);
}


static void m68000_pea_aw(void)
{
   push_32(EA_AW);
   USE_CLKS(0+16);
}


static void m68000_pea_al(void)
{
   push_32(EA_AL);
   USE_CLKS(0+20);
}


static void m68000_pea_pcdi(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   push_32(ea);
   USE_CLKS(0+16);
}


static void m68000_pea_pcix(void)
{
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   push_32(ea);
   USE_CLKS(0+20);
}


static void m68000_reset(void)
{
   if(g_cpu_s_flag)
   {
#if M68000_OUTPUT_RESET
      m68000_output_reset();
#endif /* M68000_OUTPUT_RESET */
      USE_CLKS(132);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_ror_s_8(void)
{
   uint* d_dst = &DY;
   uint orig_shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint shift = orig_shift & 7;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(ROR_8(src, shift));

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = (src >> ((shift-1)&7)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((orig_shift<<1)+6);
}


static void m68000_ror_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, shift));

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = (src >> (shift-1)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_ror_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = MASK_OUT_ABOVE_32(ROR_32(src, shift));

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = (src >> (shift-1)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+8);
}


static void m68000_ror_r_8(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift & 7;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(ROR_8(src, shift));

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;
      g_cpu_c_flag = (src >> ((shift-1)&7)) & 1;
      g_cpu_n_flag = GET_MSB_8(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_ror_r_16(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift & 15;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, shift));

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;
      g_cpu_c_flag = (src >> ((shift-1)&15)) & 1;
      g_cpu_n_flag = GET_MSB_16(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_ror_r_32(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift & 31;
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = MASK_OUT_ABOVE_32(ROR_32(src, shift));

   USE_CLKS((orig_shift<<1)+8);
   if(orig_shift != 0)
   {
      *d_dst = res;
      g_cpu_c_flag = (src >> ((shift-1)&31)) & 1;
      g_cpu_n_flag = GET_MSB_32(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_ror_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_ror_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_ror_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_ror_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_ror_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_ror_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_ror_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROR_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = src & 1;
   g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_rol_s_8(void)
{
   uint* d_dst = &DY;
   uint orig_shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint shift = orig_shift & 7;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(ROL_8(src, shift));

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = (src >> (8-orig_shift)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((orig_shift<<1)+6);
}


static void m68000_rol_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, shift));

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = (src >> (16-shift)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_rol_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = MASK_OUT_ABOVE_32(ROL_32(src, shift));

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = (src >> (32-shift)) & 1;
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+8);
}


static void m68000_rol_r_8(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift & 7;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint res = MASK_OUT_ABOVE_8(ROL_8(src, shift));

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      if(shift != 0)
      {
         *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;
         g_cpu_c_flag = (src >> (8-shift)) & 1;
         g_cpu_n_flag = GET_MSB_8(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }
      g_cpu_c_flag = src & 1;
      g_cpu_n_flag = GET_MSB_8(src);
      g_cpu_z_flag = (src == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_rol_r_16(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift & 15;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, shift));

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      if(shift != 0)
      {
         *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;
         g_cpu_c_flag = (src >> (16-shift)) & 1;
         g_cpu_n_flag = GET_MSB_16(res);
         g_cpu_z_flag = (res == 0);
         g_cpu_v_flag = 0;
         return;
      }
      g_cpu_c_flag = src & 1;
      g_cpu_n_flag = GET_MSB_16(src);
      g_cpu_z_flag = (src == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_rol_r_32(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift & 31;
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = MASK_OUT_ABOVE_32(ROL_32(src, shift));

   USE_CLKS((orig_shift<<1)+8);
   if(orig_shift != 0)
   {
      *d_dst = res;

      g_cpu_c_flag = (src >> (32-shift)) & 1;
      g_cpu_n_flag = GET_MSB_32(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = 0;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_rol_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_rol_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_rol_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_rol_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_rol_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_rol_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_rol_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = GET_MSB_16(src);
   g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_roxr_s_8(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint tmp = ROR_9(src  | ((g_cpu_x_flag != 0) << 8), shift);
   uint res = MASK_OUT_ABOVE_8(tmp);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_9(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_roxr_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), shift);
   uint res = MASK_OUT_ABOVE_16(tmp);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_roxr_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint a = ((src >> 16) & 0xffff) | ((g_cpu_x_flag != 0) << 16);
   uint b = src & 0xffff;
   uint shifted_a = LSR_32(a, shift) | LSL_32(a, (33 - shift)) | LSL_32(b, (17 - shift));
   uint shifted_b = LSR_32(b, shift) | LSL_32(b, (33 - shift)) | LSL_32(a, (16 - shift));
   uint res = MASK_OUT_ABOVE_32(((shifted_a << 16) & 0xffff0000) | (shifted_b & 0xffff));

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(shifted_a);
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+8);
}


static void m68000_roxr_r_8(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift % 9;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint tmp = ROR_9(src  | ((g_cpu_x_flag != 0) << 8), shift);
   uint res = MASK_OUT_ABOVE_8(tmp);

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;
      g_cpu_c_flag = g_cpu_x_flag = GET_MSB_9(tmp);
      g_cpu_n_flag = GET_MSB_8(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = g_cpu_x_flag;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_roxr_r_16(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift % 17;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), shift);
   uint res = MASK_OUT_ABOVE_16(tmp);

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;
      g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
      g_cpu_n_flag = GET_MSB_16(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = g_cpu_x_flag;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_roxr_r_32(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift % 33;
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = MASK_OUT_ABOVE_32((ROR_33(src, shift) & ~(1<<(32-shift))) | ((g_cpu_x_flag != 0)<<(32-shift)));
   uint new_x_flag = src & (1<<(shift-1));

   USE_CLKS((orig_shift<<1)+8);
   if(shift != 0)
   {
      *d_dst = res;
      g_cpu_x_flag = new_x_flag;
   }
   else
      res = src;
   g_cpu_c_flag = g_cpu_x_flag;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_roxr_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_roxr_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_roxr_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_roxr_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_roxr_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_roxr_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_roxr_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint tmp = ROR_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68000_roxl_s_8(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint tmp = ROL_9(src  | ((g_cpu_x_flag != 0) << 8), shift);
   uint res = MASK_OUT_ABOVE_8(tmp);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_9(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_roxl_s_16(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), shift);
   uint res = MASK_OUT_ABOVE_16(tmp);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_roxl_s_32(void)
{
   uint* d_dst = &DY;
   uint shift = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint a = ((src >> 16) & 0xffff) | ((g_cpu_x_flag != 0) << 16);
   uint b = src & 0xffff;
   uint shifted_a = LSL_32(a, shift) | LSR_32(a, (33 - shift)) | LSR_32(b, (16 - shift));
   uint shifted_b = LSL_32(b, shift) | LSR_32(b, (33 - shift)) | LSR_32(a, (17 - shift));
   uint res = MASK_OUT_ABOVE_32(((shifted_a << 16) & 0xffff0000) | (shifted_b & 0xffff));

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(shifted_a);
   g_cpu_v_flag = 0;
   USE_CLKS((shift<<1)+6);
}


static void m68000_roxl_r_8(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift % 9;
   uint src = MASK_OUT_ABOVE_8(*d_dst);
   uint tmp = ROL_9(src  | ((g_cpu_x_flag != 0) << 8), shift);
   uint res = MASK_OUT_ABOVE_8(tmp);

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;
      g_cpu_c_flag = g_cpu_x_flag = GET_MSB_9(tmp);
      g_cpu_n_flag = GET_MSB_8(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = g_cpu_x_flag;
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_roxl_r_16(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift % 17;
   uint src = MASK_OUT_ABOVE_16(*d_dst);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), shift);
   uint res = MASK_OUT_ABOVE_16(tmp);

   USE_CLKS((orig_shift<<1)+6);
   if(orig_shift != 0)
   {
      *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;
      g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
      g_cpu_n_flag = GET_MSB_16(res);
      g_cpu_z_flag = (res == 0);
      g_cpu_v_flag = 0;
      return;
   }

   g_cpu_c_flag = g_cpu_x_flag;
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_roxl_r_32(void)
{
   uint* d_dst = &DY;
   uint orig_shift = DX & 0x3f;
   uint shift = orig_shift % 33;
   uint src = MASK_OUT_ABOVE_32(*d_dst);
   uint res = MASK_OUT_ABOVE_32((ROL_33(src, shift) & ~(1<<(shift-1))) | ((g_cpu_x_flag != 0)<<(shift-1)));
   uint new_x_flag = src & (1<<(32-shift));

   USE_CLKS((orig_shift<<1)+8);
   if(shift != 0)
   {
      *d_dst = res;
      g_cpu_x_flag = new_x_flag;
   }
   else
      res = src;
   g_cpu_c_flag = g_cpu_x_flag;
   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_v_flag = 0;
}


static void m68000_roxl_ea_ai(void)
{
   uint ea = EA_AI;
   uint src = read_memory_16(ea);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_roxl_ea_pi(void)
{
   uint ea = EA_PI_16;
   uint src = read_memory_16(ea);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+4);
}


static void m68000_roxl_ea_pd(void)
{
   uint ea = EA_PD_16;
   uint src = read_memory_16(ea);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+6);
}


static void m68000_roxl_ea_di(void)
{
   uint ea = EA_DI;
   uint src = read_memory_16(ea);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_roxl_ea_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+10);
}


static void m68000_roxl_ea_aw(void)
{
   uint ea = EA_AW;
   uint src = read_memory_16(ea);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+8);
}


static void m68000_roxl_ea_al(void)
{
   uint ea = EA_AL;
   uint src = read_memory_16(ea);
   uint tmp = ROL_17(src  | ((g_cpu_x_flag != 0) << 16), 1);
   uint res = MASK_OUT_ABOVE_16(tmp);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_c_flag = g_cpu_x_flag = GET_MSB_17(tmp);
   g_cpu_v_flag = 0;
   USE_CLKS(8+12);
}


static void m68010_rtd(void)
{
   if(g_cpu_type & (CPU_010_PLUS))
   {
      uint new_pc = pull_32();
      g_cpu_ar[7] += make_int_16(read_imm_16());
      branch_long(new_pc);
      USE_CLKS(16);
      return;
   }
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}


static void m68000_rte(void)
{
   uint new_sr;
   uint new_pc;
   uint format_word;

   if(g_cpu_s_flag)
   {
      new_sr = pull_16();
      new_pc = pull_32();
      branch_long(new_pc);
      set_sr(new_sr);
      if(!(g_cpu_type & (CPU_010_PLUS)))
      {
         USE_CLKS(20);
         return;
      }
      format_word = (pull_16()>>12) & 0xf;
      /* I'm ignoring code 8 (bus error and address error) */
      if(format_word != 0)
         exception(EXCEPTION_FORMAT_ERROR);
      USE_CLKS(20);
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_rtr(void)
{
   set_ccr(pull_16());
   branch_long(pull_32());
   USE_CLKS(20);
}


static void m68000_rts(void)
{
   branch_long(pull_32());
   USE_CLKS(16);
}


static void m68000_sbcd_rr(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - (g_cpu_x_flag != 0);

   if(res > 9)
      res -= 6;
   res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res += 0xa0;

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | MASK_OUT_ABOVE_8(res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(6);
}


static void m68000_sbcd_mm_ax7(void)
{
   uint src = read_memory_8(--(AY));
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - (g_cpu_x_flag != 0);

   if(res > 9)
      res -= 6;
   res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res += 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_sbcd_mm_ay7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = --AX;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - (g_cpu_x_flag != 0);

   if(res > 9)
      res -= 6;
   res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res += 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_sbcd_mm_axy7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - (g_cpu_x_flag != 0);

   if(res > 9)
      res -= 6;
   res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res += 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_sbcd_mm(void)
{
   uint src = read_memory_8(--AY);
   uint ea  = --AX;
   uint dst = read_memory_8(ea);
   uint res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - (g_cpu_x_flag != 0);

   if(res > 9)
      res -= 6;
   res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
   if((g_cpu_x_flag = g_cpu_c_flag = res > 0x99) != 0)
      res += 0xa0;

   write_memory_8(ea, res);

   if(MASK_OUT_ABOVE_8(res) != 0)
      g_cpu_z_flag = 0;
   USE_CLKS(18);
}


static void m68000_st_d(void)
{
   DY |= 0xff;
   USE_CLKS(6);
}


static void m68000_st_ai(void)
{
   write_memory_8(EA_AI, 0xff);
   USE_CLKS(8+4);
}


static void m68000_st_pi(void)
{
   write_memory_8(EA_PI_8, 0xff);
   USE_CLKS(8+4);
}


static void m68000_st_pi7(void)
{
   write_memory_8(EA_PI7_8, 0xff);
   USE_CLKS(8+4);
}


static void m68000_st_pd(void)
{
   write_memory_8(EA_PD_8, 0xff);
   USE_CLKS(8+6);
}


static void m68000_st_pd7(void)
{
   write_memory_8(EA_PD7_8, 0xff);
   USE_CLKS(8+6);
}


static void m68000_st_di(void)
{
   write_memory_8(EA_DI, 0xff);
   USE_CLKS(8+8);
}


static void m68000_st_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, 0xff);
   USE_CLKS(8+10);
}


static void m68000_st_aw(void)
{
   write_memory_8(EA_AW, 0xff);
   USE_CLKS(8+8);
}


static void m68000_st_al(void)
{
   write_memory_8(EA_AL, 0xff);
   USE_CLKS(8+12);
}


static void m68000_sf_d(void)
{
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_sf_ai(void)
{
   write_memory_8(EA_AI, 0);
   USE_CLKS(8+4);
}


static void m68000_sf_pi(void)
{
   write_memory_8(EA_PI_8, 0);
   USE_CLKS(8+4);
}


static void m68000_sf_pi7(void)
{
   write_memory_8(EA_PI7_8, 0);
   USE_CLKS(8+4);
}


static void m68000_sf_pd(void)
{
   write_memory_8(EA_PD_8, 0);
   USE_CLKS(8+6);
}


static void m68000_sf_pd7(void)
{
   write_memory_8(EA_PD7_8, 0);
   USE_CLKS(8+6);
}


static void m68000_sf_di(void)
{
   write_memory_8(EA_DI, 0);
   USE_CLKS(8+8);
}


static void m68000_sf_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, 0);
   USE_CLKS(8+10);
}


static void m68000_sf_aw(void)
{
   write_memory_8(EA_AW, 0);
   USE_CLKS(8+8);
}


static void m68000_sf_al(void)
{
   write_memory_8(EA_AL, 0);
   USE_CLKS(8+12);
}


static void m68000_shi_d(void)
{
   if(CONDITION_HI)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_shi_ai(void)
{
   write_memory_8(EA_AI, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_shi_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_shi_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_shi_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_shi_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_shi_di(void)
{
   write_memory_8(EA_DI, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_shi_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_shi_aw(void)
{
   write_memory_8(EA_AW, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_shi_al(void)
{
   write_memory_8(EA_AL, CONDITION_HI ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_sls_d(void)
{
   if(CONDITION_LS)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_sls_ai(void)
{
   write_memory_8(EA_AI, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sls_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sls_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sls_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sls_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sls_di(void)
{
   write_memory_8(EA_DI, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sls_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_sls_aw(void)
{
   write_memory_8(EA_AW, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sls_al(void)
{
   write_memory_8(EA_AL, CONDITION_LS ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_scc_d(void)
{
   if(CONDITION_CC)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_scc_ai(void)
{
   write_memory_8(EA_AI, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_scc_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_scc_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_scc_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_scc_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_scc_di(void)
{
   write_memory_8(EA_DI, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_scc_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_scc_aw(void)
{
   write_memory_8(EA_AW, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_scc_al(void)
{
   write_memory_8(EA_AL, CONDITION_CC ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_scs_d(void)
{
   if(CONDITION_CS)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_scs_ai(void)
{
   write_memory_8(EA_AI, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_scs_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_scs_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_scs_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_scs_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_scs_di(void)
{
   write_memory_8(EA_DI, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_scs_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_scs_aw(void)
{
   write_memory_8(EA_AW, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_scs_al(void)
{
   write_memory_8(EA_AL, CONDITION_CS ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_sne_d(void)
{
   if(CONDITION_NE)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_sne_ai(void)
{
   write_memory_8(EA_AI, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sne_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sne_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sne_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sne_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sne_di(void)
{
   write_memory_8(EA_DI, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sne_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_sne_aw(void)
{
   write_memory_8(EA_AW, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sne_al(void)
{
   write_memory_8(EA_AL, CONDITION_NE ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_seq_d(void)
{
   if(CONDITION_EQ)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_seq_ai(void)
{
   write_memory_8(EA_AI, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_seq_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_seq_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_seq_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_seq_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_seq_di(void)
{
   write_memory_8(EA_DI, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_seq_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_seq_aw(void)
{
   write_memory_8(EA_AW, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_seq_al(void)
{
   write_memory_8(EA_AL, CONDITION_EQ ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_svc_d(void)
{
   if(CONDITION_VC)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_svc_ai(void)
{
   write_memory_8(EA_AI, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_svc_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_svc_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_svc_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_svc_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_svc_di(void)
{
   write_memory_8(EA_DI, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_svc_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_svc_aw(void)
{
   write_memory_8(EA_AW, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_svc_al(void)
{
   write_memory_8(EA_AL, CONDITION_VC ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_svs_d(void)
{
   if(CONDITION_VS)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_svs_ai(void)
{
   write_memory_8(EA_AI, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_svs_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_svs_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_svs_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_svs_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_svs_di(void)
{
   write_memory_8(EA_DI, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_svs_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_svs_aw(void)
{
   write_memory_8(EA_AW, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_svs_al(void)
{
   write_memory_8(EA_AL, CONDITION_VS ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_spl_d(void)
{
   if(CONDITION_PL)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_spl_ai(void)
{
   write_memory_8(EA_AI, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_spl_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_spl_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_spl_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_spl_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_spl_di(void)
{
   write_memory_8(EA_DI, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_spl_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_spl_aw(void)
{
   write_memory_8(EA_AW, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_spl_al(void)
{
   write_memory_8(EA_AL, CONDITION_PL ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_smi_d(void)
{
   if(CONDITION_MI)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_smi_ai(void)
{
   write_memory_8(EA_AI, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_smi_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_smi_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_smi_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_smi_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_smi_di(void)
{
   write_memory_8(EA_DI, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_smi_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_smi_aw(void)
{
   write_memory_8(EA_AW, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_smi_al(void)
{
   write_memory_8(EA_AL, CONDITION_MI ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_sge_d(void)
{
   if(CONDITION_GE)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_sge_ai(void)
{
   write_memory_8(EA_AI, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sge_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sge_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sge_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sge_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sge_di(void)
{
   write_memory_8(EA_DI, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sge_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_sge_aw(void)
{
   write_memory_8(EA_AW, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sge_al(void)
{
   write_memory_8(EA_AL, CONDITION_GE ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_slt_d(void)
{
   if(CONDITION_LT)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_slt_ai(void)
{
   write_memory_8(EA_AI, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_slt_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_slt_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_slt_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_slt_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_slt_di(void)
{
   write_memory_8(EA_DI, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_slt_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_slt_aw(void)
{
   write_memory_8(EA_AW, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_slt_al(void)
{
   write_memory_8(EA_AL, CONDITION_LT ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_sgt_d(void)
{
   if(CONDITION_GT)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_sgt_ai(void)
{
   write_memory_8(EA_AI, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sgt_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sgt_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sgt_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sgt_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sgt_di(void)
{
   write_memory_8(EA_DI, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sgt_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_sgt_aw(void)
{
   write_memory_8(EA_AW, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sgt_al(void)
{
   write_memory_8(EA_AL, CONDITION_GT ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_sle_d(void)
{
   if(CONDITION_LE)
   {
      DY |= 0xff;
      USE_CLKS(6);
      return;
   }
   DY &= 0xffffff00;
   USE_CLKS(4);
}


static void m68000_sle_ai(void)
{
   write_memory_8(EA_AI, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sle_pi(void)
{
   write_memory_8(EA_PI_8, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sle_pi7(void)
{
   write_memory_8(EA_PI7_8, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+4);
}


static void m68000_sle_pd(void)
{
   write_memory_8(EA_PD_8, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sle_pd7(void)
{
   write_memory_8(EA_PD7_8, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+6);
}


static void m68000_sle_di(void)
{
   write_memory_8(EA_DI, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sle_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   write_memory_8(ea, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+10);
}


static void m68000_sle_aw(void)
{
   write_memory_8(EA_AW, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+8);
}


static void m68000_sle_al(void)
{
   write_memory_8(EA_AL, CONDITION_LE ? 0xff : 0);
   USE_CLKS(8+12);
}


static void m68000_stop(void)
{
   uint new_sr = read_imm_16();

   if(g_cpu_s_flag)
   {
      g_cpu_stopped = 1;
      set_sr(new_sr);
      m68000_clks_left = 0;
      return;
   }
   exception(EXCEPTION_PRIVILEGE_VIOLATION);
}


static void m68000_sub_er_d_8(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4);
}


static void m68000_sub_er_ai_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_AI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+4);
}


static void m68000_sub_er_pi_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PI_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+4);
}


static void m68000_sub_er_pi7_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PI7_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+4);
}


static void m68000_sub_er_pd_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PD_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+6);
}


static void m68000_sub_er_pd7_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_PD7_8);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+6);
}


static void m68000_sub_er_di_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_DI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+8);
}


static void m68000_sub_er_ix_8(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+10);
}


static void m68000_sub_er_aw_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_AW);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+8);
}


static void m68000_sub_er_al_8(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_8(EA_AL);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+12);
}


static void m68000_sub_er_pcdi_8(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_8(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+8);
}


static void m68000_sub_er_pcix_8(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_8(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+10);
}


static void m68000_sub_er_i_8(void)
{
   uint* d_dst = &DX;
   uint src = read_imm_8();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+4);
}


static void m68000_sub_er_d_16(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4);
}


static void m68000_sub_er_a_16(void)
{
   uint* d_dst = &DX;
   uint src = AY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4);
}


static void m68000_sub_er_ai_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+4);
}


static void m68000_sub_er_pi_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_PI_16);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+4);
}


static void m68000_sub_er_pd_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_PD_16);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+6);
}


static void m68000_sub_er_di_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_DI);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+8);
}


static void m68000_sub_er_ix_16(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+10);
}


static void m68000_sub_er_aw_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AW);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+8);
}


static void m68000_sub_er_al_16(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_16(EA_AL);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+12);
}


static void m68000_sub_er_pcdi_16(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_16(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+8);
}


static void m68000_sub_er_pcix_16(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_16(ea);
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+10);
}


static void m68000_sub_er_i_16(void)
{
   uint* d_dst = &DX;
   uint src = read_imm_16();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4+4);
}


static void m68000_sub_er_d_32(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8);
}


static void m68000_sub_er_a_32(void)
{
   uint* d_dst = &DX;
   uint src = AY;
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8);
}


static void m68000_sub_er_ai_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_AI);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+8);
}


static void m68000_sub_er_pi_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_PI_32);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+8);
}


static void m68000_sub_er_pd_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_PD_32);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+10);
}


static void m68000_sub_er_di_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_DI);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+12);
}


static void m68000_sub_er_ix_32(void)
{
   uint* d_dst = &DX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+14);
}


static void m68000_sub_er_aw_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_AW);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+12);
}


static void m68000_sub_er_al_32(void)
{
   uint* d_dst = &DX;
   uint src = read_memory_32(EA_AL);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+16);
}


static void m68000_sub_er_pcdi_32(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   uint src = read_memory_32(ea);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+12);
}


static void m68000_sub_er_pcix_32(void)
{
   uint* d_dst = &DX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = read_memory_32(ea);
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+14);
}


static void m68000_sub_er_i_32(void)
{
   uint* d_dst = &DX;
   uint src = read_imm_32();
   uint dst = *d_dst;
   uint res = *d_dst = MASK_OUT_ABOVE_32(dst - src);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(6+8);
}


static void m68000_sub_re_ai_8(void)
{
   uint ea = EA_AI;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_sub_re_pi_8(void)
{
   uint ea = EA_PI_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_sub_re_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_sub_re_pd_8(void)
{
   uint ea = EA_PD_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+6);
}


static void m68000_sub_re_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+6);
}


static void m68000_sub_re_di_8(void)
{
   uint ea = EA_DI;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_sub_re_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+10);
}


static void m68000_sub_re_aw_8(void)
{
   uint ea = EA_AW;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_sub_re_al_8(void)
{
   uint ea = EA_AL;
   uint src = DX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+12);
}


static void m68000_sub_re_ai_16(void)
{
   uint ea = EA_AI;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_sub_re_pi_16(void)
{
   uint ea = EA_PI_16;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_sub_re_pd_16(void)
{
   uint ea = EA_PD_16;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+6);
}


static void m68000_sub_re_di_16(void)
{
   uint ea = EA_DI;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_sub_re_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+10);
}


static void m68000_sub_re_aw_16(void)
{
   uint ea = EA_AW;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_sub_re_al_16(void)
{
   uint ea = EA_AL;
   uint src = DX;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+12);
}


static void m68000_sub_re_ai_32(void)
{
   uint ea = EA_AI;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_sub_re_pi_32(void)
{
   uint ea = EA_PI_32;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_sub_re_pd_32(void)
{
   uint ea = EA_PD_32;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+10);
}


static void m68000_sub_re_di_32(void)
{
   uint ea = EA_DI;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+12);
}


static void m68000_sub_re_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+14);
}


static void m68000_sub_re_aw_32(void)
{
   uint ea = EA_AW;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+12);
}


static void m68000_sub_re_al_32(void)
{
   uint ea = EA_AL;
   uint src = DX;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+16);
}


static void m68000_suba_d_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(MASK_OUT_ABOVE_16(DY)));
   USE_CLKS(8);
}


static void m68000_suba_a_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(MASK_OUT_ABOVE_16(AY)));
   USE_CLKS(8);
}


static void m68000_suba_ai_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(EA_AI)));
   USE_CLKS(8+4);
}


static void m68000_suba_pi_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(EA_PI_16)));
   USE_CLKS(8+4);
}


static void m68000_suba_pd_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(EA_PD_16)));
   USE_CLKS(8+6);
}


static void m68000_suba_di_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(EA_DI)));
   USE_CLKS(8+8);
}


static void m68000_suba_ix_16(void)
{
   uint* a_dst = &AX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(ea)));
   USE_CLKS(8+10);
}


static void m68000_suba_aw_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(EA_AW)));
   USE_CLKS(8+8);
}


static void m68000_suba_al_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(EA_AL)));
   USE_CLKS(8+12);
}


static void m68000_suba_pcdi_16(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(ea)));
   USE_CLKS(8+8);
}


static void m68000_suba_pcix_16(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_memory_16(ea)));
   USE_CLKS(8+10);
}


static void m68000_suba_i_16(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - make_int_16(read_imm_16()));
   USE_CLKS(8+4);
}


static void m68000_suba_d_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - DY);
   USE_CLKS(8);
}


static void m68000_suba_a_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - AY);
   USE_CLKS(8);
}


static void m68000_suba_ai_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(EA_AI));
   USE_CLKS(6+8);
}


static void m68000_suba_pi_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(EA_PI_32));
   USE_CLKS(6+8);
}


static void m68000_suba_pd_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(EA_PD_32));
   USE_CLKS(6+10);
}


static void m68000_suba_di_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(EA_DI));
   USE_CLKS(6+12);
}


static void m68000_suba_ix_32(void)
{
   uint* a_dst = &AX;
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(ea));
   USE_CLKS(6+14);
}


static void m68000_suba_aw_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(EA_AW));
   USE_CLKS(6+12);
}


static void m68000_suba_al_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(EA_AL));
   USE_CLKS(6+16);
}


static void m68000_suba_pcdi_32(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint ea = old_pc + make_int_16(read_memory_16(old_pc));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(ea));
   USE_CLKS(6+12);
}


static void m68000_suba_pcix_32(void)
{
   uint* a_dst = &AX;
   uint old_pc = (g_cpu_pc+=2) - 2;
   uint extension = read_memory_16(old_pc);
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = old_pc + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_memory_32(ea));
   USE_CLKS(6+14);
}


static void m68000_suba_i_32(void)
{
   uint* a_dst = &AX;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - read_imm_32());
   USE_CLKS(6+8);
}


static void m68000_subi_d_8(void)
{
   uint* d_dst = &DY;
   uint src = read_imm_8();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8);
}


static void m68000_subi_ai_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+4);
}


static void m68000_subi_pi_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+4);
}


static void m68000_subi_pi7_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+4);
}


static void m68000_subi_pd_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+6);
}


static void m68000_subi_pd7_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+6);
}


static void m68000_subi_di_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_subi_ix_8(void)
{
   uint src = read_imm_8();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+10);
}


static void m68000_subi_aw_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_subi_al_8(void)
{
   uint src = read_imm_8();
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+12);
}


static void m68000_subi_d_16(void)
{
   uint* d_dst = &DY;
   uint src = read_imm_16();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8);
}


static void m68000_subi_ai_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_AI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+4);
}


static void m68000_subi_pi_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_PI_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+4);
}


static void m68000_subi_pd_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_PD_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+6);
}


static void m68000_subi_di_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_DI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_subi_ix_16(void)
{
   uint src = read_imm_16();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+10);
}


static void m68000_subi_aw_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_AW;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_subi_al_16(void)
{
   uint src = read_imm_16();
   uint ea = EA_AL;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);
   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+12);
}


static void m68000_subi_d_32(void)
{
   uint* d_dst = &DY;
   uint src = read_imm_32();
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(16);
}


static void m68000_subi_ai_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_AI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(20+8);
}


static void m68000_subi_pi_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_PI_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(20+8);
}


static void m68000_subi_pd_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_PD_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(20+10);
}


static void m68000_subi_di_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_DI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(20+12);
}


static void m68000_subi_ix_32(void)
{
   uint src = read_imm_32();
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(20+14);
}


static void m68000_subi_aw_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_AW;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(20+12);
}


static void m68000_subi_al_32(void)
{
   uint src = read_imm_32();
   uint ea = EA_AL;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(20+16);
}


static void m68000_subq_d_8(void)
{
   uint* d_dst = &DY;
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src);

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4);
}


static void m68000_subq_ai_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_subq_pi_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_subq_pi7_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_subq_pd_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+6);
}


static void m68000_subq_pd7_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+6);
}


static void m68000_subq_di_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_subq_ix_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+10);
}


static void m68000_subq_aw_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_subq_al_8(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src);

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+12);
}


static void m68000_subq_d_16(void)
{
   uint* d_dst = &DY;
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src);

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4);
}


static void m68000_subq_a_16(void)
{
   uint* a_dst = &AY;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - g_3bit_qdata_table[(g_cpu_ir>>9)&7]);
   USE_CLKS(8);
}


static void m68000_subq_ai_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_subq_pi_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+4);
}


static void m68000_subq_pd_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD_16;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+6);
}


static void m68000_subq_di_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_DI;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_subq_ix_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+10);
}


static void m68000_subq_aw_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AW;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+8);
}


static void m68000_subq_al_16(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AL;
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src);

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8+12);
}


static void m68000_subq_d_32(void)
{
   uint* d_dst = &DY;
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_32(dst - src);

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8);
}


static void m68000_subq_a_32(void)
{
   uint* a_dst = &AY;
   *a_dst = MASK_OUT_ABOVE_32(*a_dst - g_3bit_qdata_table[(g_cpu_ir>>9)&7]);
   USE_CLKS(8);
}


static void m68000_subq_ai_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_subq_pi_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PI_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+8);
}


static void m68000_subq_pd_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_PD_32;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+10);
}


static void m68000_subq_di_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_DI;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+12);
}


static void m68000_subq_ix_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+14);
}


static void m68000_subq_aw_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AW;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+12);
}


static void m68000_subq_al_32(void)
{
   uint src = g_3bit_qdata_table[(g_cpu_ir>>9)&7];
   uint ea = EA_AL;
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src);

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = (res == 0);
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(12+16);
}


static void m68000_subx_rr_8(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_8(dst - src - (g_cpu_x_flag != 0));

   *d_dst = MASK_OUT_BELOW_8(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4);
}


static void m68000_subx_rr_16(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_16(dst - src - (g_cpu_x_flag != 0));

   *d_dst = MASK_OUT_BELOW_16(*d_dst) | res;

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(4);
}


static void m68000_subx_rr_32(void)
{
   uint* d_dst = &DX;
   uint src = DY;
   uint dst = *d_dst;
   uint res = MASK_OUT_ABOVE_32(dst - src - (g_cpu_x_flag != 0));

   *d_dst = res;

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(8);
}


static void m68000_subx_mm_8_ax7(void)
{
   uint src = read_memory_8(--AY);
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src - (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(18);
}


static void m68000_subx_mm_8_ay7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = --AX;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src - (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(18);
}


static void m68000_subx_mm_8_axy7(void)
{
   uint src = read_memory_8(g_cpu_ar[7]-=2);
   uint ea  = g_cpu_ar[7]-=2;
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src - (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(18);
}


static void m68000_subx_mm_8(void)
{
   uint src = read_memory_8(--(AY));
   uint ea  = --(AX);
   uint dst = read_memory_8(ea);
   uint res = MASK_OUT_ABOVE_8(dst - src - (g_cpu_x_flag != 0));

   write_memory_8(ea, res);

   g_cpu_n_flag = GET_MSB_8(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_8((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_8((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(18);
}


static void m68000_subx_mm_16(void)
{
   uint src = read_memory_16(AY-=2);
   uint ea  = (AX-=2);
   uint dst = read_memory_16(ea);
   uint res = MASK_OUT_ABOVE_16(dst - src - (g_cpu_x_flag != 0));

   write_memory_16(ea, res);

   g_cpu_n_flag = GET_MSB_16(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_16((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_16((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(18);
}


static void m68000_subx_mm_32(void)
{
   uint src = read_memory_32(AY-=4);
   uint ea  = (AX-=4);
   uint dst = read_memory_32(ea);
   uint res = MASK_OUT_ABOVE_32(dst - src - (g_cpu_x_flag != 0));

   write_memory_32(ea, res);

   g_cpu_n_flag = GET_MSB_32(res);
   if(res != 0) g_cpu_z_flag = 0;
   g_cpu_x_flag = g_cpu_c_flag = GET_MSB_32((src & ~dst) | (res & ~dst) | (src & res));
   g_cpu_v_flag = GET_MSB_32((~src & dst & ~res) | (src & ~dst & res));
   USE_CLKS(30);
}


static void m68000_swap(void)
{
   uint* d_dst = &DY;

   *d_dst ^= (*d_dst>>16) & 0x0000ffff;
   *d_dst ^= (*d_dst<<16) & 0xffff0000;
   *d_dst ^= (*d_dst>>16) & 0x0000ffff;

   g_cpu_n_flag = GET_MSB_32(*d_dst);
   g_cpu_z_flag = *d_dst == 0;
   g_cpu_c_flag = g_cpu_v_flag = 0;
   USE_CLKS(4);
}


static void m68000_tas_d(void)
{
   uint* d_dst = &DY;

   g_cpu_z_flag = MASK_OUT_ABOVE_8(*d_dst) == 0;
   g_cpu_n_flag = GET_MSB_8(*d_dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   *d_dst |= 0x80;
   USE_CLKS(4);
}


static void m68000_tas_ai(void)
{
   uint ea = EA_AI;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+4);
}


static void m68000_tas_pi(void)
{
   uint ea = EA_PI_8;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+4);
}


static void m68000_tas_pi7(void)
{
   uint ea = EA_PI7_8;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+4);
}


static void m68000_tas_pd(void)
{
   uint ea = EA_PD_8;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+6);
}


static void m68000_tas_pd7(void)
{
   uint ea = EA_PD7_8;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+6);
}


static void m68000_tas_di(void)
{
   uint ea = EA_DI;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+8);
}


static void m68000_tas_ix(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+10);
}


static void m68000_tas_aw(void)
{
   uint ea = EA_AW;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+8);
}


static void m68000_tas_al(void)
{
   uint ea = EA_AL;
   uint dst = read_memory_8(ea);

   g_cpu_z_flag = dst == 0;
   g_cpu_n_flag = GET_MSB_8(dst);
   g_cpu_v_flag = g_cpu_c_flag = 0;
   write_memory_8(ea, dst | 0x80);
   USE_CLKS(14+12);
}


static void m68000_trap(void)
{
   exception(EXCEPTION_TRAP_BASE+(g_cpu_ir&0xf));
}


static void m68000_trapv(void)
{
   if(!g_cpu_v_flag)
   {
      USE_CLKS(4);
      return;
   }
   exception(EXCEPTION_TRAPV);
}


static void m68000_tst_d_8(void)
{
   uint res = MASK_OUT_ABOVE_8(DY);

   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_tst_ai_8(void)
{
   uint ea = EA_AI;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_tst_pi_8(void)
{
   uint ea = EA_PI_8;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_tst_pi7_8(void)
{
   uint ea = EA_PI7_8;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_tst_pd_8(void)
{
   uint ea = EA_PD_8;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_tst_pd7_8(void)
{
   uint ea = EA_PD7_8;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_tst_di_8(void)
{
   uint ea = EA_DI;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_tst_ix_8(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_tst_aw_8(void)
{
   uint ea = EA_AW;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_tst_al_8(void)
{
   uint ea = EA_AL;
   uint res = read_memory_8(ea);
   g_cpu_n_flag = GET_MSB_8(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_tst_d_16(void)
{
   uint res = MASK_OUT_ABOVE_16(DY);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_tst_ai_16(void)
{
   uint res = read_memory_16(EA_AI);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_tst_pi_16(void)
{
   uint res = read_memory_16(EA_PI_16);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+4);
}


static void m68000_tst_pd_16(void)
{
   uint res = read_memory_16(EA_PD_16);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+6);
}


static void m68000_tst_di_16(void)
{
   uint res = read_memory_16(EA_DI);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_tst_ix_16(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_16(ea);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_tst_aw_16(void)
{
   uint res = read_memory_16(EA_AW);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_tst_al_16(void)
{
   uint res = read_memory_16(EA_AL);

   g_cpu_n_flag = GET_MSB_16(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_tst_d_32(void)
{
   uint res = DY;

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4);
}


static void m68000_tst_ai_32(void)
{
   uint res = read_memory_32(EA_AI);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_tst_pi_32(void)
{
   uint res = read_memory_32(EA_PI_32);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+8);
}


static void m68000_tst_pd_32(void)
{
   uint res = read_memory_32(EA_PD_32);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+10);
}


static void m68000_tst_di_32(void)
{
   uint res = read_memory_32(EA_DI);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_tst_ix_32(void)
{
   uint extension = read_imm_16();
   uint index = g_cpu_dar[BIT_F(extension)!=0][(extension>>12) & 7];
   uint ea = AY + make_int_8(extension & 0xff) + (BIT_B(extension) ? index : make_int_16(index & 0xffff));
   uint res = read_memory_32(ea);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+14);
}


static void m68000_tst_aw_32(void)
{
   uint res = read_memory_32(EA_AW);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+12);
}


static void m68000_tst_al_32(void)
{
   uint res = read_memory_32(EA_AL);

   g_cpu_n_flag = GET_MSB_32(res);
   g_cpu_z_flag = res == 0;
   g_cpu_v_flag = g_cpu_c_flag = 0;
   USE_CLKS(4+16);
}


static void m68000_unlk_a7(void)
{
   g_cpu_ar[7] = read_memory_32(g_cpu_ar[7]);
   USE_CLKS(12);
}


static void m68000_unlk(void)
{
   uint* a_dst = &AY;
   g_cpu_ar[7] = *a_dst;
   *a_dst = pull_32();
   USE_CLKS(12);
}


static void m68000_illegal(void)
{
   exception(EXCEPTION_ILLEGAL_INSTRUCTION);
}



/* ======================================================================== */
/* ======================= INSTRUCTION TABLE BUILDER ====================== */
/* ======================================================================== */

/* Opcode handler table */
static opcode_struct g_opcode_handler_table[] =
{
/*  opcode handler              mask   match */
   {m68000_1010             , 0xf000, 0xa000},
   {m68000_1111             , 0xf000, 0xf000},
   {m68000_abcd_rr          , 0xf1f8, 0xc100},
   {m68000_abcd_mm_ax7      , 0xfff8, 0xcf08},
   {m68000_abcd_mm_ay7      , 0xf1ff, 0xc10f},
   {m68000_abcd_mm_axy7     , 0xffff, 0xcf0f},
   {m68000_abcd_mm          , 0xf1f8, 0xc108},
   {m68000_add_er_d_8       , 0xf1f8, 0xd000},
   {m68000_add_er_ai_8      , 0xf1f8, 0xd010},
   {m68000_add_er_pi_8      , 0xf1f8, 0xd018},
   {m68000_add_er_pi7_8     , 0xf1ff, 0xd01f},
   {m68000_add_er_pd_8      , 0xf1f8, 0xd020},
   {m68000_add_er_pd7_8     , 0xf1ff, 0xd027},
   {m68000_add_er_di_8      , 0xf1f8, 0xd028},
   {m68000_add_er_ix_8      , 0xf1f8, 0xd030},
   {m68000_add_er_aw_8      , 0xf1ff, 0xd038},
   {m68000_add_er_al_8      , 0xf1ff, 0xd039},
   {m68000_add_er_pcdi_8    , 0xf1ff, 0xd03a},
   {m68000_add_er_pcix_8    , 0xf1ff, 0xd03b},
   {m68000_add_er_i_8       , 0xf1ff, 0xd03c},
   {m68000_add_er_d_16      , 0xf1f8, 0xd040},
   {m68000_add_er_a_16      , 0xf1f8, 0xd048},
   {m68000_add_er_ai_16     , 0xf1f8, 0xd050},
   {m68000_add_er_pi_16     , 0xf1f8, 0xd058},
   {m68000_add_er_pd_16     , 0xf1f8, 0xd060},
   {m68000_add_er_di_16     , 0xf1f8, 0xd068},
   {m68000_add_er_ix_16     , 0xf1f8, 0xd070},
   {m68000_add_er_aw_16     , 0xf1ff, 0xd078},
   {m68000_add_er_al_16     , 0xf1ff, 0xd079},
   {m68000_add_er_pcdi_16   , 0xf1ff, 0xd07a},
   {m68000_add_er_pcix_16   , 0xf1ff, 0xd07b},
   {m68000_add_er_i_16      , 0xf1ff, 0xd07c},
   {m68000_add_er_d_32      , 0xf1f8, 0xd080},
   {m68000_add_er_a_32      , 0xf1f8, 0xd088},
   {m68000_add_er_ai_32     , 0xf1f8, 0xd090},
   {m68000_add_er_pi_32     , 0xf1f8, 0xd098},
   {m68000_add_er_pd_32     , 0xf1f8, 0xd0a0},
   {m68000_add_er_di_32     , 0xf1f8, 0xd0a8},
   {m68000_add_er_ix_32     , 0xf1f8, 0xd0b0},
   {m68000_add_er_aw_32     , 0xf1ff, 0xd0b8},
   {m68000_add_er_al_32     , 0xf1ff, 0xd0b9},
   {m68000_add_er_pcdi_32   , 0xf1ff, 0xd0ba},
   {m68000_add_er_pcix_32   , 0xf1ff, 0xd0bb},
   {m68000_add_er_i_32      , 0xf1ff, 0xd0bc},
   {m68000_add_re_ai_8      , 0xf1f8, 0xd110},
   {m68000_add_re_pi_8      , 0xf1f8, 0xd118},
   {m68000_add_re_pi7_8     , 0xf1ff, 0xd11f},
   {m68000_add_re_pd_8      , 0xf1f8, 0xd120},
   {m68000_add_re_pd7_8     , 0xf1ff, 0xd127},
   {m68000_add_re_di_8      , 0xf1f8, 0xd128},
   {m68000_add_re_ix_8      , 0xf1f8, 0xd130},
   {m68000_add_re_aw_8      , 0xf1ff, 0xd138},
   {m68000_add_re_al_8      , 0xf1ff, 0xd139},
   {m68000_add_re_ai_16     , 0xf1f8, 0xd150},
   {m68000_add_re_pi_16     , 0xf1f8, 0xd158},
   {m68000_add_re_pd_16     , 0xf1f8, 0xd160},
   {m68000_add_re_di_16     , 0xf1f8, 0xd168},
   {m68000_add_re_ix_16     , 0xf1f8, 0xd170},
   {m68000_add_re_aw_16     , 0xf1ff, 0xd178},
   {m68000_add_re_al_16     , 0xf1ff, 0xd179},
   {m68000_add_re_ai_32     , 0xf1f8, 0xd190},
   {m68000_add_re_pi_32     , 0xf1f8, 0xd198},
   {m68000_add_re_pd_32     , 0xf1f8, 0xd1a0},
   {m68000_add_re_di_32     , 0xf1f8, 0xd1a8},
   {m68000_add_re_ix_32     , 0xf1f8, 0xd1b0},
   {m68000_add_re_aw_32     , 0xf1ff, 0xd1b8},
   {m68000_add_re_al_32     , 0xf1ff, 0xd1b9},
   {m68000_adda_d_16        , 0xf1f8, 0xd0c0},
   {m68000_adda_a_16        , 0xf1f8, 0xd0c8},
   {m68000_adda_ai_16       , 0xf1f8, 0xd0d0},
   {m68000_adda_pi_16       , 0xf1f8, 0xd0d8},
   {m68000_adda_pd_16       , 0xf1f8, 0xd0e0},
   {m68000_adda_di_16       , 0xf1f8, 0xd0e8},
   {m68000_adda_ix_16       , 0xf1f8, 0xd0f0},
   {m68000_adda_aw_16       , 0xf1ff, 0xd0f8},
   {m68000_adda_al_16       , 0xf1ff, 0xd0f9},
   {m68000_adda_pcdi_16     , 0xf1ff, 0xd0fa},
   {m68000_adda_pcix_16     , 0xf1ff, 0xd0fb},
   {m68000_adda_i_16        , 0xf1ff, 0xd0fc},
   {m68000_adda_d_32        , 0xf1f8, 0xd1c0},
   {m68000_adda_a_32        , 0xf1f8, 0xd1c8},
   {m68000_adda_ai_32       , 0xf1f8, 0xd1d0},
   {m68000_adda_pi_32       , 0xf1f8, 0xd1d8},
   {m68000_adda_pd_32       , 0xf1f8, 0xd1e0},
   {m68000_adda_di_32       , 0xf1f8, 0xd1e8},
   {m68000_adda_ix_32       , 0xf1f8, 0xd1f0},
   {m68000_adda_aw_32       , 0xf1ff, 0xd1f8},
   {m68000_adda_al_32       , 0xf1ff, 0xd1f9},
   {m68000_adda_pcdi_32     , 0xf1ff, 0xd1fa},
   {m68000_adda_pcix_32     , 0xf1ff, 0xd1fb},
   {m68000_adda_i_32        , 0xf1ff, 0xd1fc},
   {m68000_addi_d_8         , 0xfff8, 0x0600},
   {m68000_addi_ai_8        , 0xfff8, 0x0610},
   {m68000_addi_pi_8        , 0xfff8, 0x0618},
   {m68000_addi_pi7_8       , 0xffff, 0x061f},
   {m68000_addi_pd_8        , 0xfff8, 0x0620},
   {m68000_addi_pd7_8       , 0xffff, 0x0627},
   {m68000_addi_di_8        , 0xfff8, 0x0628},
   {m68000_addi_ix_8        , 0xfff8, 0x0630},
   {m68000_addi_aw_8        , 0xffff, 0x0638},
   {m68000_addi_al_8        , 0xffff, 0x0639},
   {m68000_addi_d_16        , 0xfff8, 0x0640},
   {m68000_addi_ai_16       , 0xfff8, 0x0650},
   {m68000_addi_pi_16       , 0xfff8, 0x0658},
   {m68000_addi_pd_16       , 0xfff8, 0x0660},
   {m68000_addi_di_16       , 0xfff8, 0x0668},
   {m68000_addi_ix_16       , 0xfff8, 0x0670},
   {m68000_addi_aw_16       , 0xffff, 0x0678},
   {m68000_addi_al_16       , 0xffff, 0x0679},
   {m68000_addi_d_32        , 0xfff8, 0x0680},
   {m68000_addi_ai_32       , 0xfff8, 0x0690},
   {m68000_addi_pi_32       , 0xfff8, 0x0698},
   {m68000_addi_pd_32       , 0xfff8, 0x06a0},
   {m68000_addi_di_32       , 0xfff8, 0x06a8},
   {m68000_addi_ix_32       , 0xfff8, 0x06b0},
   {m68000_addi_aw_32       , 0xffff, 0x06b8},
   {m68000_addi_al_32       , 0xffff, 0x06b9},
   {m68000_addq_d_8         , 0xf1f8, 0x5000},
   {m68000_addq_ai_8        , 0xf1f8, 0x5010},
   {m68000_addq_pi_8        , 0xf1f8, 0x5018},
   {m68000_addq_pi7_8       , 0xf1ff, 0x501f},
   {m68000_addq_pd_8        , 0xf1f8, 0x5020},
   {m68000_addq_pd7_8       , 0xf1ff, 0x5027},
   {m68000_addq_di_8        , 0xf1f8, 0x5028},
   {m68000_addq_ix_8        , 0xf1f8, 0x5030},
   {m68000_addq_aw_8        , 0xf1ff, 0x5038},
   {m68000_addq_al_8        , 0xf1ff, 0x5039},
   {m68000_addq_d_16        , 0xf1f8, 0x5040},
   {m68000_addq_a_16        , 0xf1f8, 0x5048},
   {m68000_addq_ai_16       , 0xf1f8, 0x5050},
   {m68000_addq_pi_16       , 0xf1f8, 0x5058},
   {m68000_addq_pd_16       , 0xf1f8, 0x5060},
   {m68000_addq_di_16       , 0xf1f8, 0x5068},
   {m68000_addq_ix_16       , 0xf1f8, 0x5070},
   {m68000_addq_aw_16       , 0xf1ff, 0x5078},
   {m68000_addq_al_16       , 0xf1ff, 0x5079},
   {m68000_addq_d_32        , 0xf1f8, 0x5080},
   {m68000_addq_a_32        , 0xf1f8, 0x5088},
   {m68000_addq_ai_32       , 0xf1f8, 0x5090},
   {m68000_addq_pi_32       , 0xf1f8, 0x5098},
   {m68000_addq_pd_32       , 0xf1f8, 0x50a0},
   {m68000_addq_di_32       , 0xf1f8, 0x50a8},
   {m68000_addq_ix_32       , 0xf1f8, 0x50b0},
   {m68000_addq_aw_32       , 0xf1ff, 0x50b8},
   {m68000_addq_al_32       , 0xf1ff, 0x50b9},
   {m68000_addx_rr_8        , 0xf1f8, 0xd100},
   {m68000_addx_rr_16       , 0xf1f8, 0xd140},
   {m68000_addx_rr_32       , 0xf1f8, 0xd180},
   {m68000_addx_mm_8_ax7    , 0xfff8, 0xdf08},
   {m68000_addx_mm_8_ay7    , 0xf1ff, 0xd10f},
   {m68000_addx_mm_8_axy7   , 0xffff, 0xdf0f},
   {m68000_addx_mm_8        , 0xf1f8, 0xd108},
   {m68000_addx_mm_16       , 0xf1f8, 0xd148},
   {m68000_addx_mm_32       , 0xf1f8, 0xd188},
   {m68000_and_er_d_8       , 0xf1f8, 0xc000},
   {m68000_and_er_ai_8      , 0xf1f8, 0xc010},
   {m68000_and_er_pi_8      , 0xf1f8, 0xc018},
   {m68000_and_er_pi7_8     , 0xf1ff, 0xc01f},
   {m68000_and_er_pd_8      , 0xf1f8, 0xc020},
   {m68000_and_er_pd7_8     , 0xf1ff, 0xc027},
   {m68000_and_er_di_8      , 0xf1f8, 0xc028},
   {m68000_and_er_ix_8      , 0xf1f8, 0xc030},
   {m68000_and_er_aw_8      , 0xf1ff, 0xc038},
   {m68000_and_er_al_8      , 0xf1ff, 0xc039},
   {m68000_and_er_pcdi_8    , 0xf1ff, 0xc03a},
   {m68000_and_er_pcix_8    , 0xf1ff, 0xc03b},
   {m68000_and_er_i_8       , 0xf1ff, 0xc03c},
   {m68000_and_er_d_16      , 0xf1f8, 0xc040},
   {m68000_and_er_ai_16     , 0xf1f8, 0xc050},
   {m68000_and_er_pi_16     , 0xf1f8, 0xc058},
   {m68000_and_er_pd_16     , 0xf1f8, 0xc060},
   {m68000_and_er_di_16     , 0xf1f8, 0xc068},
   {m68000_and_er_ix_16     , 0xf1f8, 0xc070},
   {m68000_and_er_aw_16     , 0xf1ff, 0xc078},
   {m68000_and_er_al_16     , 0xf1ff, 0xc079},
   {m68000_and_er_pcdi_16   , 0xf1ff, 0xc07a},
   {m68000_and_er_pcix_16   , 0xf1ff, 0xc07b},
   {m68000_and_er_i_16      , 0xf1ff, 0xc07c},
   {m68000_and_er_d_32      , 0xf1f8, 0xc080},
   {m68000_and_er_ai_32     , 0xf1f8, 0xc090},
   {m68000_and_er_pi_32     , 0xf1f8, 0xc098},
   {m68000_and_er_pd_32     , 0xf1f8, 0xc0a0},
   {m68000_and_er_di_32     , 0xf1f8, 0xc0a8},
   {m68000_and_er_ix_32     , 0xf1f8, 0xc0b0},
   {m68000_and_er_aw_32     , 0xf1ff, 0xc0b8},
   {m68000_and_er_al_32     , 0xf1ff, 0xc0b9},
   {m68000_and_er_pcdi_32   , 0xf1ff, 0xc0ba},
   {m68000_and_er_pcix_32   , 0xf1ff, 0xc0bb},
   {m68000_and_er_i_32      , 0xf1ff, 0xc0bc},
   {m68000_and_re_ai_8      , 0xf1f8, 0xc110},
   {m68000_and_re_pi_8      , 0xf1f8, 0xc118},
   {m68000_and_re_pi7_8     , 0xf1ff, 0xc11f},
   {m68000_and_re_pd_8      , 0xf1f8, 0xc120},
   {m68000_and_re_pd7_8     , 0xf1ff, 0xc127},
   {m68000_and_re_di_8      , 0xf1f8, 0xc128},
   {m68000_and_re_ix_8      , 0xf1f8, 0xc130},
   {m68000_and_re_aw_8      , 0xf1ff, 0xc138},
   {m68000_and_re_al_8      , 0xf1ff, 0xc139},
   {m68000_and_re_ai_16     , 0xf1f8, 0xc150},
   {m68000_and_re_pi_16     , 0xf1f8, 0xc158},
   {m68000_and_re_pd_16     , 0xf1f8, 0xc160},
   {m68000_and_re_di_16     , 0xf1f8, 0xc168},
   {m68000_and_re_ix_16     , 0xf1f8, 0xc170},
   {m68000_and_re_aw_16     , 0xf1ff, 0xc178},
   {m68000_and_re_al_16     , 0xf1ff, 0xc179},
   {m68000_and_re_ai_32     , 0xf1f8, 0xc190},
   {m68000_and_re_pi_32     , 0xf1f8, 0xc198},
   {m68000_and_re_pd_32     , 0xf1f8, 0xc1a0},
   {m68000_and_re_di_32     , 0xf1f8, 0xc1a8},
   {m68000_and_re_ix_32     , 0xf1f8, 0xc1b0},
   {m68000_and_re_aw_32     , 0xf1ff, 0xc1b8},
   {m68000_and_re_al_32     , 0xf1ff, 0xc1b9},
   {m68000_andi_to_ccr      , 0xffff, 0x023c},
   {m68000_andi_to_sr       , 0xffff, 0x027c},
   {m68000_andi_d_8         , 0xfff8, 0x0200},
   {m68000_andi_ai_8        , 0xfff8, 0x0210},
   {m68000_andi_pi_8        , 0xfff8, 0x0218},
   {m68000_andi_pi7_8       , 0xffff, 0x021f},
   {m68000_andi_pd_8        , 0xfff8, 0x0220},
   {m68000_andi_pd7_8       , 0xffff, 0x0227},
   {m68000_andi_di_8        , 0xfff8, 0x0228},
   {m68000_andi_ix_8        , 0xfff8, 0x0230},
   {m68000_andi_aw_8        , 0xffff, 0x0238},
   {m68000_andi_al_8        , 0xffff, 0x0239},
   {m68000_andi_d_16        , 0xfff8, 0x0240},
   {m68000_andi_ai_16       , 0xfff8, 0x0250},
   {m68000_andi_pi_16       , 0xfff8, 0x0258},
   {m68000_andi_pd_16       , 0xfff8, 0x0260},
   {m68000_andi_di_16       , 0xfff8, 0x0268},
   {m68000_andi_ix_16       , 0xfff8, 0x0270},
   {m68000_andi_aw_16       , 0xffff, 0x0278},
   {m68000_andi_al_16       , 0xffff, 0x0279},
   {m68000_andi_d_32        , 0xfff8, 0x0280},
   {m68000_andi_ai_32       , 0xfff8, 0x0290},
   {m68000_andi_pi_32       , 0xfff8, 0x0298},
   {m68000_andi_pd_32       , 0xfff8, 0x02a0},
   {m68000_andi_di_32       , 0xfff8, 0x02a8},
   {m68000_andi_ix_32       , 0xfff8, 0x02b0},
   {m68000_andi_aw_32       , 0xffff, 0x02b8},
   {m68000_andi_al_32       , 0xffff, 0x02b9},
   {m68000_asr_s_8          , 0xf1f8, 0xe000},
   {m68000_asr_s_16         , 0xf1f8, 0xe040},
   {m68000_asr_s_32         , 0xf1f8, 0xe080},
   {m68000_asr_r_8          , 0xf1f8, 0xe020},
   {m68000_asr_r_16         , 0xf1f8, 0xe060},
   {m68000_asr_r_32         , 0xf1f8, 0xe0a0},
   {m68000_asr_ea_ai        , 0xfff8, 0xe0d0},
   {m68000_asr_ea_pi        , 0xfff8, 0xe0d8},
   {m68000_asr_ea_pd        , 0xfff8, 0xe0e0},
   {m68000_asr_ea_di        , 0xfff8, 0xe0e8},
   {m68000_asr_ea_ix        , 0xfff8, 0xe0f0},
   {m68000_asr_ea_aw        , 0xffff, 0xe0f8},
   {m68000_asr_ea_al        , 0xffff, 0xe0f9},
   {m68000_asl_s_8          , 0xf1f8, 0xe100},
   {m68000_asl_s_16         , 0xf1f8, 0xe140},
   {m68000_asl_s_32         , 0xf1f8, 0xe180},
   {m68000_asl_r_8          , 0xf1f8, 0xe120},
   {m68000_asl_r_16         , 0xf1f8, 0xe160},
   {m68000_asl_r_32         , 0xf1f8, 0xe1a0},
   {m68000_asl_ea_ai        , 0xfff8, 0xe1d0},
   {m68000_asl_ea_pi        , 0xfff8, 0xe1d8},
   {m68000_asl_ea_pd        , 0xfff8, 0xe1e0},
   {m68000_asl_ea_di        , 0xfff8, 0xe1e8},
   {m68000_asl_ea_ix        , 0xfff8, 0xe1f0},
   {m68000_asl_ea_aw        , 0xffff, 0xe1f8},
   {m68000_asl_ea_al        , 0xffff, 0xe1f9},
   {m68000_bhi_16           , 0xffff, 0x6200},
   {m68000_bhi_8            , 0xff00, 0x6200},
   {m68000_bls_16           , 0xffff, 0x6300},
   {m68000_bls_8            , 0xff00, 0x6300},
   {m68000_bcc_16           , 0xffff, 0x6400},
   {m68000_bcc_8            , 0xff00, 0x6400},
   {m68000_bcs_16           , 0xffff, 0x6500},
   {m68000_bcs_8            , 0xff00, 0x6500},
   {m68000_bne_16           , 0xffff, 0x6600},
   {m68000_bne_8            , 0xff00, 0x6600},
   {m68000_beq_16           , 0xffff, 0x6700},
   {m68000_beq_8            , 0xff00, 0x6700},
   {m68000_bvc_16           , 0xffff, 0x6800},
   {m68000_bvc_8            , 0xff00, 0x6800},
   {m68000_bvs_16           , 0xffff, 0x6900},
   {m68000_bvs_8            , 0xff00, 0x6900},
   {m68000_bpl_16           , 0xffff, 0x6a00},
   {m68000_bpl_8            , 0xff00, 0x6a00},
   {m68000_bmi_16           , 0xffff, 0x6b00},
   {m68000_bmi_8            , 0xff00, 0x6b00},
   {m68000_bge_16           , 0xffff, 0x6c00},
   {m68000_bge_8            , 0xff00, 0x6c00},
   {m68000_blt_16           , 0xffff, 0x6d00},
   {m68000_blt_8            , 0xff00, 0x6d00},
   {m68000_bgt_16           , 0xffff, 0x6e00},
   {m68000_bgt_8            , 0xff00, 0x6e00},
   {m68000_ble_16           , 0xffff, 0x6f00},
   {m68000_ble_8            , 0xff00, 0x6f00},
   {m68000_bchg_r_d         , 0xf1f8, 0x0140},
   {m68000_bchg_r_ai        , 0xf1f8, 0x0150},
   {m68000_bchg_r_pi        , 0xf1f8, 0x0158},
   {m68000_bchg_r_pi7       , 0xf1ff, 0x015f},
   {m68000_bchg_r_pd        , 0xf1f8, 0x0160},
   {m68000_bchg_r_pd7       , 0xf1ff, 0x0167},
   {m68000_bchg_r_di        , 0xf1f8, 0x0168},
   {m68000_bchg_r_ix        , 0xf1f8, 0x0170},
   {m68000_bchg_r_aw        , 0xf1ff, 0x0178},
   {m68000_bchg_r_al        , 0xf1ff, 0x0179},
   {m68000_bchg_s_d         , 0xfff8, 0x0840},
   {m68000_bchg_s_ai        , 0xfff8, 0x0850},
   {m68000_bchg_s_pi        , 0xfff8, 0x0858},
   {m68000_bchg_s_pi7       , 0xffff, 0x085f},
   {m68000_bchg_s_pd        , 0xfff8, 0x0860},
   {m68000_bchg_s_pd7       , 0xffff, 0x0867},
   {m68000_bchg_s_di        , 0xfff8, 0x0868},
   {m68000_bchg_s_ix        , 0xfff8, 0x0870},
   {m68000_bchg_s_aw        , 0xffff, 0x0878},
   {m68000_bchg_s_al        , 0xffff, 0x0879},
   {m68000_bclr_r_d         , 0xf1f8, 0x0180},
   {m68000_bclr_r_ai        , 0xf1f8, 0x0190},
   {m68000_bclr_r_pi        , 0xf1f8, 0x0198},
   {m68000_bclr_r_pi7       , 0xf1ff, 0x019f},
   {m68000_bclr_r_pd        , 0xf1f8, 0x01a0},
   {m68000_bclr_r_pd7       , 0xf1ff, 0x01a7},
   {m68000_bclr_r_di        , 0xf1f8, 0x01a8},
   {m68000_bclr_r_ix        , 0xf1f8, 0x01b0},
   {m68000_bclr_r_aw        , 0xf1ff, 0x01b8},
   {m68000_bclr_r_al        , 0xf1ff, 0x01b9},
   {m68000_bclr_s_d         , 0xfff8, 0x0880},
   {m68000_bclr_s_ai        , 0xfff8, 0x0890},
   {m68000_bclr_s_pi        , 0xfff8, 0x0898},
   {m68000_bclr_s_pi7       , 0xffff, 0x089f},
   {m68000_bclr_s_pd        , 0xfff8, 0x08a0},
   {m68000_bclr_s_pd7       , 0xffff, 0x08a7},
   {m68000_bclr_s_di        , 0xfff8, 0x08a8},
   {m68000_bclr_s_ix        , 0xfff8, 0x08b0},
   {m68000_bclr_s_aw        , 0xffff, 0x08b8},
   {m68000_bclr_s_al        , 0xffff, 0x08b9},
   {m68010_bkpt             , 0xfff8, 0x4848},
   {m68000_bra_16           , 0xffff, 0x6000},
   {m68000_bra_8            , 0xff00, 0x6000},
   {m68000_bset_r_d         , 0xf1f8, 0x01c0},
   {m68000_bset_r_ai        , 0xf1f8, 0x01d0},
   {m68000_bset_r_pi        , 0xf1f8, 0x01d8},
   {m68000_bset_r_pi7       , 0xf1ff, 0x01df},
   {m68000_bset_r_pd        , 0xf1f8, 0x01e0},
   {m68000_bset_r_pd7       , 0xf1ff, 0x01e7},
   {m68000_bset_r_di        , 0xf1f8, 0x01e8},
   {m68000_bset_r_ix        , 0xf1f8, 0x01f0},
   {m68000_bset_r_aw        , 0xf1ff, 0x01f8},
   {m68000_bset_r_al        , 0xf1ff, 0x01f9},
   {m68000_bset_s_d         , 0xfff8, 0x08c0},
   {m68000_bset_s_ai        , 0xfff8, 0x08d0},
   {m68000_bset_s_pi        , 0xfff8, 0x08d8},
   {m68000_bset_s_pi7       , 0xffff, 0x08df},
   {m68000_bset_s_pd        , 0xfff8, 0x08e0},
   {m68000_bset_s_pd7       , 0xffff, 0x08e7},
   {m68000_bset_s_di        , 0xfff8, 0x08e8},
   {m68000_bset_s_ix        , 0xfff8, 0x08f0},
   {m68000_bset_s_aw        , 0xffff, 0x08f8},
   {m68000_bset_s_al        , 0xffff, 0x08f9},
   {m68000_bsr_16           , 0xffff, 0x6100},
   {m68000_bsr_8            , 0xff00, 0x6100},
   {m68000_btst_r_d         , 0xf1f8, 0x0100},
   {m68000_btst_r_ai        , 0xf1f8, 0x0110},
   {m68000_btst_r_pi        , 0xf1f8, 0x0118},
   {m68000_btst_r_pi7       , 0xf1ff, 0x011f},
   {m68000_btst_r_pd        , 0xf1f8, 0x0120},
   {m68000_btst_r_pd7       , 0xf1ff, 0x0127},
   {m68000_btst_r_di        , 0xf1f8, 0x0128},
   {m68000_btst_r_ix        , 0xf1f8, 0x0130},
   {m68000_btst_r_aw        , 0xf1ff, 0x0138},
   {m68000_btst_r_al        , 0xf1ff, 0x0139},
   {m68000_btst_r_pcdi      , 0xf1ff, 0x013a},
   {m68000_btst_r_pcix      , 0xf1ff, 0x013b},
   {m68000_btst_r_i         , 0xf1ff, 0x013c},
   {m68000_btst_s_d         , 0xfff8, 0x0800},
   {m68000_btst_s_ai        , 0xfff8, 0x0810},
   {m68000_btst_s_pi        , 0xfff8, 0x0818},
   {m68000_btst_s_pi7       , 0xffff, 0x081f},
   {m68000_btst_s_pd        , 0xfff8, 0x0820},
   {m68000_btst_s_pd7       , 0xffff, 0x0827},
   {m68000_btst_s_di        , 0xfff8, 0x0828},
   {m68000_btst_s_ix        , 0xfff8, 0x0830},
   {m68000_btst_s_aw        , 0xffff, 0x0838},
   {m68000_btst_s_al        , 0xffff, 0x0839},
   {m68000_btst_s_pcdi      , 0xffff, 0x083a},
   {m68000_btst_s_pcix      , 0xffff, 0x083b},
   {m68000_chk_d            , 0xf1f8, 0x4180},
   {m68000_chk_ai           , 0xf1f8, 0x4190},
   {m68000_chk_pi           , 0xf1f8, 0x4198},
   {m68000_chk_pd           , 0xf1f8, 0x41a0},
   {m68000_chk_di           , 0xf1f8, 0x41a8},
   {m68000_chk_ix           , 0xf1f8, 0x41b0},
   {m68000_chk_aw           , 0xf1ff, 0x41b8},
   {m68000_chk_al           , 0xf1ff, 0x41b9},
   {m68000_chk_pcdi         , 0xf1ff, 0x41ba},
   {m68000_chk_pcix         , 0xf1ff, 0x41bb},
   {m68000_chk_i            , 0xf1ff, 0x41bc},
   {m68000_clr_d_8          , 0xfff8, 0x4200},
   {m68000_clr_ai_8         , 0xfff8, 0x4210},
   {m68000_clr_pi_8         , 0xfff8, 0x4218},
   {m68000_clr_pi7_8        , 0xffff, 0x421f},
   {m68000_clr_pd_8         , 0xfff8, 0x4220},
   {m68000_clr_pd7_8        , 0xffff, 0x4227},
   {m68000_clr_di_8         , 0xfff8, 0x4228},
   {m68000_clr_ix_8         , 0xfff8, 0x4230},
   {m68000_clr_aw_8         , 0xffff, 0x4238},
   {m68000_clr_al_8         , 0xffff, 0x4239},
   {m68000_clr_d_16         , 0xfff8, 0x4240},
   {m68000_clr_ai_16        , 0xfff8, 0x4250},
   {m68000_clr_pi_16        , 0xfff8, 0x4258},
   {m68000_clr_pd_16        , 0xfff8, 0x4260},
   {m68000_clr_di_16        , 0xfff8, 0x4268},
   {m68000_clr_ix_16        , 0xfff8, 0x4270},
   {m68000_clr_aw_16        , 0xffff, 0x4278},
   {m68000_clr_al_16        , 0xffff, 0x4279},
   {m68000_clr_d_32         , 0xfff8, 0x4280},
   {m68000_clr_ai_32        , 0xfff8, 0x4290},
   {m68000_clr_pi_32        , 0xfff8, 0x4298},
   {m68000_clr_pd_32        , 0xfff8, 0x42a0},
   {m68000_clr_di_32        , 0xfff8, 0x42a8},
   {m68000_clr_ix_32        , 0xfff8, 0x42b0},
   {m68000_clr_aw_32        , 0xffff, 0x42b8},
   {m68000_clr_al_32        , 0xffff, 0x42b9},
   {m68000_cmp_d_8          , 0xf1f8, 0xb000},
   {m68000_cmp_ai_8         , 0xf1f8, 0xb010},
   {m68000_cmp_pi_8         , 0xf1f8, 0xb018},
   {m68000_cmp_pi7_8        , 0xf1ff, 0xb01f},
   {m68000_cmp_pd_8         , 0xf1f8, 0xb020},
   {m68000_cmp_pd7_8        , 0xf1ff, 0xb027},
   {m68000_cmp_di_8         , 0xf1f8, 0xb028},
   {m68000_cmp_ix_8         , 0xf1f8, 0xb030},
   {m68000_cmp_aw_8         , 0xf1ff, 0xb038},
   {m68000_cmp_al_8         , 0xf1ff, 0xb039},
   {m68000_cmp_pcdi_8       , 0xf1ff, 0xb03a},
   {m68000_cmp_pcix_8       , 0xf1ff, 0xb03b},
   {m68000_cmp_i_8          , 0xf1ff, 0xb03c},
   {m68000_cmp_d_16         , 0xf1f8, 0xb040},
   {m68000_cmp_a_16         , 0xf1f8, 0xb048},
   {m68000_cmp_ai_16        , 0xf1f8, 0xb050},
   {m68000_cmp_pi_16        , 0xf1f8, 0xb058},
   {m68000_cmp_pd_16        , 0xf1f8, 0xb060},
   {m68000_cmp_di_16        , 0xf1f8, 0xb068},
   {m68000_cmp_ix_16        , 0xf1f8, 0xb070},
   {m68000_cmp_aw_16        , 0xf1ff, 0xb078},
   {m68000_cmp_al_16        , 0xf1ff, 0xb079},
   {m68000_cmp_pcdi_16      , 0xf1ff, 0xb07a},
   {m68000_cmp_pcix_16      , 0xf1ff, 0xb07b},
   {m68000_cmp_i_16         , 0xf1ff, 0xb07c},
   {m68000_cmp_d_32         , 0xf1f8, 0xb080},
   {m68000_cmp_a_32         , 0xf1f8, 0xb088},
   {m68000_cmp_ai_32        , 0xf1f8, 0xb090},
   {m68000_cmp_pi_32        , 0xf1f8, 0xb098},
   {m68000_cmp_pd_32        , 0xf1f8, 0xb0a0},
   {m68000_cmp_di_32        , 0xf1f8, 0xb0a8},
   {m68000_cmp_ix_32        , 0xf1f8, 0xb0b0},
   {m68000_cmp_aw_32        , 0xf1ff, 0xb0b8},
   {m68000_cmp_al_32        , 0xf1ff, 0xb0b9},
   {m68000_cmp_pcdi_32      , 0xf1ff, 0xb0ba},
   {m68000_cmp_pcix_32      , 0xf1ff, 0xb0bb},
   {m68000_cmp_i_32         , 0xf1ff, 0xb0bc},
   {m68000_cmpa_d_16        , 0xf1f8, 0xb0c0},
   {m68000_cmpa_a_16        , 0xf1f8, 0xb0c8},
   {m68000_cmpa_ai_16       , 0xf1f8, 0xb0d0},
   {m68000_cmpa_pi_16       , 0xf1f8, 0xb0d8},
   {m68000_cmpa_pd_16       , 0xf1f8, 0xb0e0},
   {m68000_cmpa_di_16       , 0xf1f8, 0xb0e8},
   {m68000_cmpa_ix_16       , 0xf1f8, 0xb0f0},
   {m68000_cmpa_aw_16       , 0xf1ff, 0xb0f8},
   {m68000_cmpa_al_16       , 0xf1ff, 0xb0f9},
   {m68000_cmpa_pcdi_16     , 0xf1ff, 0xb0fa},
   {m68000_cmpa_pcix_16     , 0xf1ff, 0xb0fb},
   {m68000_cmpa_i_16        , 0xf1ff, 0xb0fc},
   {m68000_cmpa_d_32        , 0xf1f8, 0xb1c0},
   {m68000_cmpa_a_32        , 0xf1f8, 0xb1c8},
   {m68000_cmpa_ai_32       , 0xf1f8, 0xb1d0},
   {m68000_cmpa_pi_32       , 0xf1f8, 0xb1d8},
   {m68000_cmpa_pd_32       , 0xf1f8, 0xb1e0},
   {m68000_cmpa_di_32       , 0xf1f8, 0xb1e8},
   {m68000_cmpa_ix_32       , 0xf1f8, 0xb1f0},
   {m68000_cmpa_aw_32       , 0xf1ff, 0xb1f8},
   {m68000_cmpa_al_32       , 0xf1ff, 0xb1f9},
   {m68000_cmpa_pcdi_32     , 0xf1ff, 0xb1fa},
   {m68000_cmpa_pcix_32     , 0xf1ff, 0xb1fb},
   {m68000_cmpa_i_32        , 0xf1ff, 0xb1fc},
   {m68000_cmpi_d_8         , 0xfff8, 0x0c00},
   {m68000_cmpi_ai_8        , 0xfff8, 0x0c10},
   {m68000_cmpi_pi_8        , 0xfff8, 0x0c18},
   {m68000_cmpi_pi7_8       , 0xffff, 0x0c1f},
   {m68000_cmpi_pd_8        , 0xfff8, 0x0c20},
   {m68000_cmpi_pd7_8       , 0xffff, 0x0c27},
   {m68000_cmpi_di_8        , 0xfff8, 0x0c28},
   {m68000_cmpi_ix_8        , 0xfff8, 0x0c30},
   {m68000_cmpi_aw_8        , 0xffff, 0x0c38},
   {m68000_cmpi_al_8        , 0xffff, 0x0c39},
   {m68000_cmpi_d_16        , 0xfff8, 0x0c40},
   {m68000_cmpi_ai_16       , 0xfff8, 0x0c50},
   {m68000_cmpi_pi_16       , 0xfff8, 0x0c58},
   {m68000_cmpi_pd_16       , 0xfff8, 0x0c60},
   {m68000_cmpi_di_16       , 0xfff8, 0x0c68},
   {m68000_cmpi_ix_16       , 0xfff8, 0x0c70},
   {m68000_cmpi_aw_16       , 0xffff, 0x0c78},
   {m68000_cmpi_al_16       , 0xffff, 0x0c79},
   {m68000_cmpi_d_32        , 0xfff8, 0x0c80},
   {m68000_cmpi_ai_32       , 0xfff8, 0x0c90},
   {m68000_cmpi_pi_32       , 0xfff8, 0x0c98},
   {m68000_cmpi_pd_32       , 0xfff8, 0x0ca0},
   {m68000_cmpi_di_32       , 0xfff8, 0x0ca8},
   {m68000_cmpi_ix_32       , 0xfff8, 0x0cb0},
   {m68000_cmpi_aw_32       , 0xffff, 0x0cb8},
   {m68000_cmpi_al_32       , 0xffff, 0x0cb9},
   {m68000_cmpm_8_ax7       , 0xfff8, 0xbf08},
   {m68000_cmpm_8_ay7       , 0xf1ff, 0xb10f},
   {m68000_cmpm_8_axy7      , 0xffff, 0xbf0f},
   {m68000_cmpm_8           , 0xf1f8, 0xb108},
   {m68000_cmpm_16          , 0xf1f8, 0xb148},
   {m68000_cmpm_32          , 0xf1f8, 0xb188},
   {m68000_dbt              , 0xfff8, 0x50c8},
   {m68000_dbf              , 0xfff8, 0x51c8},
   {m68000_dbhi             , 0xfff8, 0x52c8},
   {m68000_dbls             , 0xfff8, 0x53c8},
   {m68000_dbcc             , 0xfff8, 0x54c8},
   {m68000_dbcs             , 0xfff8, 0x55c8},
   {m68000_dbne             , 0xfff8, 0x56c8},
   {m68000_dbeq             , 0xfff8, 0x57c8},
   {m68000_dbvc             , 0xfff8, 0x58c8},
   {m68000_dbvs             , 0xfff8, 0x59c8},
   {m68000_dbpl             , 0xfff8, 0x5ac8},
   {m68000_dbmi             , 0xfff8, 0x5bc8},
   {m68000_dbge             , 0xfff8, 0x5cc8},
   {m68000_dblt             , 0xfff8, 0x5dc8},
   {m68000_dbgt             , 0xfff8, 0x5ec8},
   {m68000_dble             , 0xfff8, 0x5fc8},
   {m68000_divs_d           , 0xf1f8, 0x81c0},
   {m68000_divs_ai          , 0xf1f8, 0x81d0},
   {m68000_divs_pi          , 0xf1f8, 0x81d8},
   {m68000_divs_pd          , 0xf1f8, 0x81e0},
   {m68000_divs_di          , 0xf1f8, 0x81e8},
   {m68000_divs_ix          , 0xf1f8, 0x81f0},
   {m68000_divs_aw          , 0xf1ff, 0x81f8},
   {m68000_divs_al          , 0xf1ff, 0x81f9},
   {m68000_divs_pcdi        , 0xf1ff, 0x81fa},
   {m68000_divs_pcix        , 0xf1ff, 0x81fb},
   {m68000_divs_i           , 0xf1ff, 0x81fc},
   {m68000_divu_d           , 0xf1f8, 0x80c0},
   {m68000_divu_ai          , 0xf1f8, 0x80d0},
   {m68000_divu_pi          , 0xf1f8, 0x80d8},
   {m68000_divu_pd          , 0xf1f8, 0x80e0},
   {m68000_divu_di          , 0xf1f8, 0x80e8},
   {m68000_divu_ix          , 0xf1f8, 0x80f0},
   {m68000_divu_aw          , 0xf1ff, 0x80f8},
   {m68000_divu_al          , 0xf1ff, 0x80f9},
   {m68000_divu_pcdi        , 0xf1ff, 0x80fa},
   {m68000_divu_pcix        , 0xf1ff, 0x80fb},
   {m68000_divu_i           , 0xf1ff, 0x80fc},
   {m68000_eor_d_8          , 0xf1f8, 0xb100},
   {m68000_eor_ai_8         , 0xf1f8, 0xb110},
   {m68000_eor_pi_8         , 0xf1f8, 0xb118},
   {m68000_eor_pi7_8        , 0xf1ff, 0xb11f},
   {m68000_eor_pd_8         , 0xf1f8, 0xb120},
   {m68000_eor_pd7_8        , 0xf1ff, 0xb127},
   {m68000_eor_di_8         , 0xf1f8, 0xb128},
   {m68000_eor_ix_8         , 0xf1f8, 0xb130},
   {m68000_eor_aw_8         , 0xf1ff, 0xb138},
   {m68000_eor_al_8         , 0xf1ff, 0xb139},
   {m68000_eor_d_16         , 0xf1f8, 0xb140},
   {m68000_eor_ai_16        , 0xf1f8, 0xb150},
   {m68000_eor_pi_16        , 0xf1f8, 0xb158},
   {m68000_eor_pd_16        , 0xf1f8, 0xb160},
   {m68000_eor_di_16        , 0xf1f8, 0xb168},
   {m68000_eor_ix_16        , 0xf1f8, 0xb170},
   {m68000_eor_aw_16        , 0xf1ff, 0xb178},
   {m68000_eor_al_16        , 0xf1ff, 0xb179},
   {m68000_eor_d_32         , 0xf1f8, 0xb180},
   {m68000_eor_ai_32        , 0xf1f8, 0xb190},
   {m68000_eor_pi_32        , 0xf1f8, 0xb198},
   {m68000_eor_pd_32        , 0xf1f8, 0xb1a0},
   {m68000_eor_di_32        , 0xf1f8, 0xb1a8},
   {m68000_eor_ix_32        , 0xf1f8, 0xb1b0},
   {m68000_eor_aw_32        , 0xf1ff, 0xb1b8},
   {m68000_eor_al_32        , 0xf1ff, 0xb1b9},
   {m68000_eori_to_ccr      , 0xffff, 0x0a3c},
   {m68000_eori_to_sr       , 0xffff, 0x0a7c},
   {m68000_eori_d_8         , 0xfff8, 0x0a00},
   {m68000_eori_ai_8        , 0xfff8, 0x0a10},
   {m68000_eori_pi_8        , 0xfff8, 0x0a18},
   {m68000_eori_pi7_8       , 0xffff, 0x0a1f},
   {m68000_eori_pd_8        , 0xfff8, 0x0a20},
   {m68000_eori_pd7_8       , 0xffff, 0x0a27},
   {m68000_eori_di_8        , 0xfff8, 0x0a28},
   {m68000_eori_ix_8        , 0xfff8, 0x0a30},
   {m68000_eori_aw_8        , 0xffff, 0x0a38},
   {m68000_eori_al_8        , 0xffff, 0x0a39},
   {m68000_eori_d_16        , 0xfff8, 0x0a40},
   {m68000_eori_ai_16       , 0xfff8, 0x0a50},
   {m68000_eori_pi_16       , 0xfff8, 0x0a58},
   {m68000_eori_pd_16       , 0xfff8, 0x0a60},
   {m68000_eori_di_16       , 0xfff8, 0x0a68},
   {m68000_eori_ix_16       , 0xfff8, 0x0a70},
   {m68000_eori_aw_16       , 0xffff, 0x0a78},
   {m68000_eori_al_16       , 0xffff, 0x0a79},
   {m68000_eori_d_32        , 0xfff8, 0x0a80},
   {m68000_eori_ai_32       , 0xfff8, 0x0a90},
   {m68000_eori_pi_32       , 0xfff8, 0x0a98},
   {m68000_eori_pd_32       , 0xfff8, 0x0aa0},
   {m68000_eori_di_32       , 0xfff8, 0x0aa8},
   {m68000_eori_ix_32       , 0xfff8, 0x0ab0},
   {m68000_eori_aw_32       , 0xffff, 0x0ab8},
   {m68000_eori_al_32       , 0xffff, 0x0ab9},
   {m68000_exg_dd           , 0xf1f8, 0xc140},
   {m68000_exg_aa           , 0xf1f8, 0xc148},
   {m68000_exg_da           , 0xf1f8, 0xc188},
   {m68000_ext_16           , 0xfff8, 0x4880},
   {m68000_ext_32           , 0xfff8, 0x48c0},
   {m68000_jmp_ai           , 0xfff8, 0x4ed0},
   {m68000_jmp_di           , 0xfff8, 0x4ee8},
   {m68000_jmp_ix           , 0xfff8, 0x4ef0},
   {m68000_jmp_aw           , 0xffff, 0x4ef8},
   {m68000_jmp_al           , 0xffff, 0x4ef9},
   {m68000_jmp_pcdi         , 0xffff, 0x4efa},
   {m68000_jmp_pcix         , 0xffff, 0x4efb},
   {m68000_jsr_ai           , 0xfff8, 0x4e90},
   {m68000_jsr_di           , 0xfff8, 0x4ea8},
   {m68000_jsr_ix           , 0xfff8, 0x4eb0},
   {m68000_jsr_aw           , 0xffff, 0x4eb8},
   {m68000_jsr_al           , 0xffff, 0x4eb9},
   {m68000_jsr_pcdi         , 0xffff, 0x4eba},
   {m68000_jsr_pcix         , 0xffff, 0x4ebb},
   {m68000_lea_ai           , 0xf1f8, 0x41d0},
   {m68000_lea_di           , 0xf1f8, 0x41e8},
   {m68000_lea_ix           , 0xf1f8, 0x41f0},
   {m68000_lea_aw           , 0xf1ff, 0x41f8},
   {m68000_lea_al           , 0xf1ff, 0x41f9},
   {m68000_lea_pcdi         , 0xf1ff, 0x41fa},
   {m68000_lea_pcix         , 0xf1ff, 0x41fb},
   {m68000_link_a7          , 0xffff, 0x4e57},
   {m68000_link             , 0xfff8, 0x4e50},
   {m68000_lsr_s_8          , 0xf1f8, 0xe008},
   {m68000_lsr_s_16         , 0xf1f8, 0xe048},
   {m68000_lsr_s_32         , 0xf1f8, 0xe088},
   {m68000_lsr_r_8          , 0xf1f8, 0xe028},
   {m68000_lsr_r_16         , 0xf1f8, 0xe068},
   {m68000_lsr_r_32         , 0xf1f8, 0xe0a8},
   {m68000_lsr_ea_ai        , 0xfff8, 0xe2d0},
   {m68000_lsr_ea_pi        , 0xfff8, 0xe2d8},
   {m68000_lsr_ea_pd        , 0xfff8, 0xe2e0},
   {m68000_lsr_ea_di        , 0xfff8, 0xe2e8},
   {m68000_lsr_ea_ix        , 0xfff8, 0xe2f0},
   {m68000_lsr_ea_aw        , 0xffff, 0xe2f8},
   {m68000_lsr_ea_al        , 0xffff, 0xe2f9},
   {m68000_lsl_s_8          , 0xf1f8, 0xe108},
   {m68000_lsl_s_16         , 0xf1f8, 0xe148},
   {m68000_lsl_s_32         , 0xf1f8, 0xe188},
   {m68000_lsl_r_8          , 0xf1f8, 0xe128},
   {m68000_lsl_r_16         , 0xf1f8, 0xe168},
   {m68000_lsl_r_32         , 0xf1f8, 0xe1a8},
   {m68000_lsl_ea_ai        , 0xfff8, 0xe3d0},
   {m68000_lsl_ea_pi        , 0xfff8, 0xe3d8},
   {m68000_lsl_ea_pd        , 0xfff8, 0xe3e0},
   {m68000_lsl_ea_di        , 0xfff8, 0xe3e8},
   {m68000_lsl_ea_ix        , 0xfff8, 0xe3f0},
   {m68000_lsl_ea_aw        , 0xffff, 0xe3f8},
   {m68000_lsl_ea_al        , 0xffff, 0xe3f9},
   {m68000_move_dd_d_8      , 0xf1f8, 0x1000},
   {m68000_move_dd_ai_8     , 0xf1f8, 0x1010},
   {m68000_move_dd_pi_8     , 0xf1f8, 0x1018},
   {m68000_move_dd_pi7_8    , 0xf1ff, 0x101f},
   {m68000_move_dd_pd_8     , 0xf1f8, 0x1020},
   {m68000_move_dd_pd7_8    , 0xf1ff, 0x1027},
   {m68000_move_dd_di_8     , 0xf1f8, 0x1028},
   {m68000_move_dd_ix_8     , 0xf1f8, 0x1030},
   {m68000_move_dd_aw_8     , 0xf1ff, 0x1038},
   {m68000_move_dd_al_8     , 0xf1ff, 0x1039},
   {m68000_move_dd_pcdi_8   , 0xf1ff, 0x103a},
   {m68000_move_dd_pcix_8   , 0xf1ff, 0x103b},
   {m68000_move_dd_i_8      , 0xf1ff, 0x103c},
   {m68000_move_ai_d_8      , 0xf1f8, 0x1080},
   {m68000_move_ai_ai_8     , 0xf1f8, 0x1090},
   {m68000_move_ai_pi_8     , 0xf1f8, 0x1098},
   {m68000_move_ai_pi7_8    , 0xf1ff, 0x109f},
   {m68000_move_ai_pd_8     , 0xf1f8, 0x10a0},
   {m68000_move_ai_pd7_8    , 0xf1ff, 0x10a7},
   {m68000_move_ai_di_8     , 0xf1f8, 0x10a8},
   {m68000_move_ai_ix_8     , 0xf1f8, 0x10b0},
   {m68000_move_ai_aw_8     , 0xf1ff, 0x10b8},
   {m68000_move_ai_al_8     , 0xf1ff, 0x10b9},
   {m68000_move_ai_pcdi_8   , 0xf1ff, 0x10ba},
   {m68000_move_ai_pcix_8   , 0xf1ff, 0x10bb},
   {m68000_move_ai_i_8      , 0xf1ff, 0x10bc},
   {m68000_move_pi_d_8      , 0xf1f8, 0x10c0},
   {m68000_move_pi_ai_8     , 0xf1f8, 0x10d0},
   {m68000_move_pi_pi_8     , 0xf1f8, 0x10d8},
   {m68000_move_pi_pi7_8    , 0xf1ff, 0x10df},
   {m68000_move_pi_pd_8     , 0xf1f8, 0x10e0},
   {m68000_move_pi_pd7_8    , 0xf1ff, 0x10e7},
   {m68000_move_pi_di_8     , 0xf1f8, 0x10e8},
   {m68000_move_pi_ix_8     , 0xf1f8, 0x10f0},
   {m68000_move_pi_aw_8     , 0xf1ff, 0x10f8},
   {m68000_move_pi_al_8     , 0xf1ff, 0x10f9},
   {m68000_move_pi_pcdi_8   , 0xf1ff, 0x10fa},
   {m68000_move_pi_pcix_8   , 0xf1ff, 0x10fb},
   {m68000_move_pi_i_8      , 0xf1ff, 0x10fc},
   {m68000_move_pi7_d_8     , 0xfff8, 0x1ec0},
   {m68000_move_pi7_ai_8    , 0xfff8, 0x1ed0},
   {m68000_move_pi7_pi_8    , 0xfff8, 0x1ed8},
   {m68000_move_pi7_pi7_8   , 0xffff, 0x1edf},
   {m68000_move_pi7_pd_8    , 0xfff8, 0x1ee0},
   {m68000_move_pi7_pd7_8   , 0xffff, 0x1ee7},
   {m68000_move_pi7_di_8    , 0xfff8, 0x1ee8},
   {m68000_move_pi7_ix_8    , 0xfff8, 0x1ef0},
   {m68000_move_pi7_aw_8    , 0xffff, 0x1ef8},
   {m68000_move_pi7_al_8    , 0xffff, 0x1ef9},
   {m68000_move_pi7_pcdi_8  , 0xffff, 0x1efa},
   {m68000_move_pi7_pcix_8  , 0xffff, 0x1efb},
   {m68000_move_pi7_i_8     , 0xffff, 0x1efc},
   {m68000_move_pd_d_8      , 0xf1f8, 0x1100},
   {m68000_move_pd_ai_8     , 0xf1f8, 0x1110},
   {m68000_move_pd_pi_8     , 0xf1f8, 0x1118},
   {m68000_move_pd_pi7_8    , 0xf1ff, 0x111f},
   {m68000_move_pd_pd_8     , 0xf1f8, 0x1120},
   {m68000_move_pd_pd7_8    , 0xf1ff, 0x1127},
   {m68000_move_pd_di_8     , 0xf1f8, 0x1128},
   {m68000_move_pd_ix_8     , 0xf1f8, 0x1130},
   {m68000_move_pd_aw_8     , 0xf1ff, 0x1138},
   {m68000_move_pd_al_8     , 0xf1ff, 0x1139},
   {m68000_move_pd_pcdi_8   , 0xf1ff, 0x113a},
   {m68000_move_pd_pcix_8   , 0xf1ff, 0x113b},
   {m68000_move_pd_i_8      , 0xf1ff, 0x113c},
   {m68000_move_pd7_d_8     , 0xfff8, 0x1f00},
   {m68000_move_pd7_ai_8    , 0xfff8, 0x1f10},
   {m68000_move_pd7_pi_8    , 0xfff8, 0x1f18},
   {m68000_move_pd7_pi7_8   , 0xffff, 0x1f1f},
   {m68000_move_pd7_pd_8    , 0xfff8, 0x1f20},
   {m68000_move_pd7_pd7_8   , 0xffff, 0x1f27},
   {m68000_move_pd7_di_8    , 0xfff8, 0x1f28},
   {m68000_move_pd7_ix_8    , 0xfff8, 0x1f30},
   {m68000_move_pd7_aw_8    , 0xffff, 0x1f38},
   {m68000_move_pd7_al_8    , 0xffff, 0x1f39},
   {m68000_move_pd7_pcdi_8  , 0xffff, 0x1f3a},
   {m68000_move_pd7_pcix_8  , 0xffff, 0x1f3b},
   {m68000_move_pd7_i_8     , 0xffff, 0x1f3c},
   {m68000_move_di_d_8      , 0xf1f8, 0x1140},
   {m68000_move_di_ai_8     , 0xf1f8, 0x1150},
   {m68000_move_di_pi_8     , 0xf1f8, 0x1158},
   {m68000_move_di_pi7_8    , 0xf1ff, 0x115f},
   {m68000_move_di_pd_8     , 0xf1f8, 0x1160},
   {m68000_move_di_pd7_8    , 0xf1ff, 0x1167},
   {m68000_move_di_di_8     , 0xf1f8, 0x1168},
   {m68000_move_di_ix_8     , 0xf1f8, 0x1170},
   {m68000_move_di_aw_8     , 0xf1ff, 0x1178},
   {m68000_move_di_al_8     , 0xf1ff, 0x1179},
   {m68000_move_di_pcdi_8   , 0xf1ff, 0x117a},
   {m68000_move_di_pcix_8   , 0xf1ff, 0x117b},
   {m68000_move_di_i_8      , 0xf1ff, 0x117c},
   {m68000_move_ix_d_8      , 0xf1f8, 0x1180},
   {m68000_move_ix_ai_8     , 0xf1f8, 0x1190},
   {m68000_move_ix_pi_8     , 0xf1f8, 0x1198},
   {m68000_move_ix_pi7_8    , 0xf1ff, 0x119f},
   {m68000_move_ix_pd_8     , 0xf1f8, 0x11a0},
   {m68000_move_ix_pd7_8    , 0xf1ff, 0x11a7},
   {m68000_move_ix_di_8     , 0xf1f8, 0x11a8},
   {m68000_move_ix_ix_8     , 0xf1f8, 0x11b0},
   {m68000_move_ix_aw_8     , 0xf1ff, 0x11b8},
   {m68000_move_ix_al_8     , 0xf1ff, 0x11b9},
   {m68000_move_ix_pcdi_8   , 0xf1ff, 0x11ba},
   {m68000_move_ix_pcix_8   , 0xf1ff, 0x11bb},
   {m68000_move_ix_i_8      , 0xf1ff, 0x11bc},
   {m68000_move_aw_d_8      , 0xfff8, 0x11c0},
   {m68000_move_aw_ai_8     , 0xfff8, 0x11d0},
   {m68000_move_aw_pi_8     , 0xfff8, 0x11d8},
   {m68000_move_aw_pi7_8    , 0xffff, 0x11df},
   {m68000_move_aw_pd_8     , 0xfff8, 0x11e0},
   {m68000_move_aw_pd7_8    , 0xffff, 0x11e7},
   {m68000_move_aw_di_8     , 0xfff8, 0x11e8},
   {m68000_move_aw_ix_8     , 0xfff8, 0x11f0},
   {m68000_move_aw_aw_8     , 0xffff, 0x11f8},
   {m68000_move_aw_al_8     , 0xffff, 0x11f9},
   {m68000_move_aw_pcdi_8   , 0xffff, 0x11fa},
   {m68000_move_aw_pcix_8   , 0xffff, 0x11fb},
   {m68000_move_aw_i_8      , 0xffff, 0x11fc},
   {m68000_move_al_d_8      , 0xfff8, 0x13c0},
   {m68000_move_al_ai_8     , 0xfff8, 0x13d0},
   {m68000_move_al_pi_8     , 0xfff8, 0x13d8},
   {m68000_move_al_pi7_8    , 0xffff, 0x13df},
   {m68000_move_al_pd_8     , 0xfff8, 0x13e0},
   {m68000_move_al_pd7_8    , 0xffff, 0x13e7},
   {m68000_move_al_di_8     , 0xfff8, 0x13e8},
   {m68000_move_al_ix_8     , 0xfff8, 0x13f0},
   {m68000_move_al_aw_8     , 0xffff, 0x13f8},
   {m68000_move_al_al_8     , 0xffff, 0x13f9},
   {m68000_move_al_pcdi_8   , 0xffff, 0x13fa},
   {m68000_move_al_pcix_8   , 0xffff, 0x13fb},
   {m68000_move_al_i_8      , 0xffff, 0x13fc},
   {m68000_move_dd_d_16     , 0xf1f8, 0x3000},
   {m68000_move_dd_a_16     , 0xf1f8, 0x3008},
   {m68000_move_dd_ai_16    , 0xf1f8, 0x3010},
   {m68000_move_dd_pi_16    , 0xf1f8, 0x3018},
   {m68000_move_dd_pd_16    , 0xf1f8, 0x3020},
   {m68000_move_dd_di_16    , 0xf1f8, 0x3028},
   {m68000_move_dd_ix_16    , 0xf1f8, 0x3030},
   {m68000_move_dd_aw_16    , 0xf1ff, 0x3038},
   {m68000_move_dd_al_16    , 0xf1ff, 0x3039},
   {m68000_move_dd_pcdi_16  , 0xf1ff, 0x303a},
   {m68000_move_dd_pcix_16  , 0xf1ff, 0x303b},
   {m68000_move_dd_i_16     , 0xf1ff, 0x303c},
   {m68000_move_ai_d_16     , 0xf1f8, 0x3080},
   {m68000_move_ai_a_16     , 0xf1f8, 0x3088},
   {m68000_move_ai_ai_16    , 0xf1f8, 0x3090},
   {m68000_move_ai_pi_16    , 0xf1f8, 0x3098},
   {m68000_move_ai_pd_16    , 0xf1f8, 0x30a0},
   {m68000_move_ai_di_16    , 0xf1f8, 0x30a8},
   {m68000_move_ai_ix_16    , 0xf1f8, 0x30b0},
   {m68000_move_ai_aw_16    , 0xf1ff, 0x30b8},
   {m68000_move_ai_al_16    , 0xf1ff, 0x30b9},
   {m68000_move_ai_pcdi_16  , 0xf1ff, 0x30ba},
   {m68000_move_ai_pcix_16  , 0xf1ff, 0x30bb},
   {m68000_move_ai_i_16     , 0xf1ff, 0x30bc},
   {m68000_move_pi_d_16     , 0xf1f8, 0x30c0},
   {m68000_move_pi_a_16     , 0xf1f8, 0x30c8},
   {m68000_move_pi_ai_16    , 0xf1f8, 0x30d0},
   {m68000_move_pi_pi_16    , 0xf1f8, 0x30d8},
   {m68000_move_pi_pd_16    , 0xf1f8, 0x30e0},
   {m68000_move_pi_di_16    , 0xf1f8, 0x30e8},
   {m68000_move_pi_ix_16    , 0xf1f8, 0x30f0},
   {m68000_move_pi_aw_16    , 0xf1ff, 0x30f8},
   {m68000_move_pi_al_16    , 0xf1ff, 0x30f9},
   {m68000_move_pi_pcdi_16  , 0xf1ff, 0x30fa},
   {m68000_move_pi_pcix_16  , 0xf1ff, 0x30fb},
   {m68000_move_pi_i_16     , 0xf1ff, 0x30fc},
   {m68000_move_pd_d_16     , 0xf1f8, 0x3100},
   {m68000_move_pd_a_16     , 0xf1f8, 0x3108},
   {m68000_move_pd_ai_16    , 0xf1f8, 0x3110},
   {m68000_move_pd_pi_16    , 0xf1f8, 0x3118},
   {m68000_move_pd_pd_16    , 0xf1f8, 0x3120},
   {m68000_move_pd_di_16    , 0xf1f8, 0x3128},
   {m68000_move_pd_ix_16    , 0xf1f8, 0x3130},
   {m68000_move_pd_aw_16    , 0xf1ff, 0x3138},
   {m68000_move_pd_al_16    , 0xf1ff, 0x3139},
   {m68000_move_pd_pcdi_16  , 0xf1ff, 0x313a},
   {m68000_move_pd_pcix_16  , 0xf1ff, 0x313b},
   {m68000_move_pd_i_16     , 0xf1ff, 0x313c},
   {m68000_move_di_d_16     , 0xf1f8, 0x3140},
   {m68000_move_di_a_16     , 0xf1f8, 0x3148},
   {m68000_move_di_ai_16    , 0xf1f8, 0x3150},
   {m68000_move_di_pi_16    , 0xf1f8, 0x3158},
   {m68000_move_di_pd_16    , 0xf1f8, 0x3160},
   {m68000_move_di_di_16    , 0xf1f8, 0x3168},
   {m68000_move_di_ix_16    , 0xf1f8, 0x3170},
   {m68000_move_di_aw_16    , 0xf1ff, 0x3178},
   {m68000_move_di_al_16    , 0xf1ff, 0x3179},
   {m68000_move_di_pcdi_16  , 0xf1ff, 0x317a},
   {m68000_move_di_pcix_16  , 0xf1ff, 0x317b},
   {m68000_move_di_i_16     , 0xf1ff, 0x317c},
   {m68000_move_ix_d_16     , 0xf1f8, 0x3180},
   {m68000_move_ix_a_16     , 0xf1f8, 0x3188},
   {m68000_move_ix_ai_16    , 0xf1f8, 0x3190},
   {m68000_move_ix_pi_16    , 0xf1f8, 0x3198},
   {m68000_move_ix_pd_16    , 0xf1f8, 0x31a0},
   {m68000_move_ix_di_16    , 0xf1f8, 0x31a8},
   {m68000_move_ix_ix_16    , 0xf1f8, 0x31b0},
   {m68000_move_ix_aw_16    , 0xf1ff, 0x31b8},
   {m68000_move_ix_al_16    , 0xf1ff, 0x31b9},
   {m68000_move_ix_pcdi_16  , 0xf1ff, 0x31ba},
   {m68000_move_ix_pcix_16  , 0xf1ff, 0x31bb},
   {m68000_move_ix_i_16     , 0xf1ff, 0x31bc},
   {m68000_move_aw_d_16     , 0xfff8, 0x31c0},
   {m68000_move_aw_a_16     , 0xfff8, 0x31c8},
   {m68000_move_aw_ai_16    , 0xfff8, 0x31d0},
   {m68000_move_aw_pi_16    , 0xfff8, 0x31d8},
   {m68000_move_aw_pd_16    , 0xfff8, 0x31e0},
   {m68000_move_aw_di_16    , 0xfff8, 0x31e8},
   {m68000_move_aw_ix_16    , 0xfff8, 0x31f0},
   {m68000_move_aw_aw_16    , 0xffff, 0x31f8},
   {m68000_move_aw_al_16    , 0xffff, 0x31f9},
   {m68000_move_aw_pcdi_16  , 0xffff, 0x31fa},
   {m68000_move_aw_pcix_16  , 0xffff, 0x31fb},
   {m68000_move_aw_i_16     , 0xffff, 0x31fc},
   {m68000_move_al_d_16     , 0xfff8, 0x33c0},
   {m68000_move_al_a_16     , 0xfff8, 0x33c8},
   {m68000_move_al_ai_16    , 0xfff8, 0x33d0},
   {m68000_move_al_pi_16    , 0xfff8, 0x33d8},
   {m68000_move_al_pd_16    , 0xfff8, 0x33e0},
   {m68000_move_al_di_16    , 0xfff8, 0x33e8},
   {m68000_move_al_ix_16    , 0xfff8, 0x33f0},
   {m68000_move_al_aw_16    , 0xffff, 0x33f8},
   {m68000_move_al_al_16    , 0xffff, 0x33f9},
   {m68000_move_al_pcdi_16  , 0xffff, 0x33fa},
   {m68000_move_al_pcix_16  , 0xffff, 0x33fb},
   {m68000_move_al_i_16     , 0xffff, 0x33fc},
   {m68000_move_dd_d_32     , 0xf1f8, 0x2000},
   {m68000_move_dd_a_32     , 0xf1f8, 0x2008},
   {m68000_move_dd_ai_32    , 0xf1f8, 0x2010},
   {m68000_move_dd_pi_32    , 0xf1f8, 0x2018},
   {m68000_move_dd_pd_32    , 0xf1f8, 0x2020},
   {m68000_move_dd_di_32    , 0xf1f8, 0x2028},
   {m68000_move_dd_ix_32    , 0xf1f8, 0x2030},
   {m68000_move_dd_aw_32    , 0xf1ff, 0x2038},
   {m68000_move_dd_al_32    , 0xf1ff, 0x2039},
   {m68000_move_dd_pcdi_32  , 0xf1ff, 0x203a},
   {m68000_move_dd_pcix_32  , 0xf1ff, 0x203b},
   {m68000_move_dd_i_32     , 0xf1ff, 0x203c},
   {m68000_move_ai_d_32     , 0xf1f8, 0x2080},
   {m68000_move_ai_a_32     , 0xf1f8, 0x2088},
   {m68000_move_ai_ai_32    , 0xf1f8, 0x2090},
   {m68000_move_ai_pi_32    , 0xf1f8, 0x2098},
   {m68000_move_ai_pd_32    , 0xf1f8, 0x20a0},
   {m68000_move_ai_di_32    , 0xf1f8, 0x20a8},
   {m68000_move_ai_ix_32    , 0xf1f8, 0x20b0},
   {m68000_move_ai_aw_32    , 0xf1ff, 0x20b8},
   {m68000_move_ai_al_32    , 0xf1ff, 0x20b9},
   {m68000_move_ai_pcdi_32  , 0xf1ff, 0x20ba},
   {m68000_move_ai_pcix_32  , 0xf1ff, 0x20bb},
   {m68000_move_ai_i_32     , 0xf1ff, 0x20bc},
   {m68000_move_pi_d_32     , 0xf1f8, 0x20c0},
   {m68000_move_pi_a_32     , 0xf1f8, 0x20c8},
   {m68000_move_pi_ai_32    , 0xf1f8, 0x20d0},
   {m68000_move_pi_pi_32    , 0xf1f8, 0x20d8},
   {m68000_move_pi_pd_32    , 0xf1f8, 0x20e0},
   {m68000_move_pi_di_32    , 0xf1f8, 0x20e8},
   {m68000_move_pi_ix_32    , 0xf1f8, 0x20f0},
   {m68000_move_pi_aw_32    , 0xf1ff, 0x20f8},
   {m68000_move_pi_al_32    , 0xf1ff, 0x20f9},
   {m68000_move_pi_pcdi_32  , 0xf1ff, 0x20fa},
   {m68000_move_pi_pcix_32  , 0xf1ff, 0x20fb},
   {m68000_move_pi_i_32     , 0xf1ff, 0x20fc},
   {m68000_move_pd_d_32     , 0xf1f8, 0x2100},
   {m68000_move_pd_a_32     , 0xf1f8, 0x2108},
   {m68000_move_pd_ai_32    , 0xf1f8, 0x2110},
   {m68000_move_pd_pi_32    , 0xf1f8, 0x2118},
   {m68000_move_pd_pd_32    , 0xf1f8, 0x2120},
   {m68000_move_pd_di_32    , 0xf1f8, 0x2128},
   {m68000_move_pd_ix_32    , 0xf1f8, 0x2130},
   {m68000_move_pd_aw_32    , 0xf1ff, 0x2138},
   {m68000_move_pd_al_32    , 0xf1ff, 0x2139},
   {m68000_move_pd_pcdi_32  , 0xf1ff, 0x213a},
   {m68000_move_pd_pcix_32  , 0xf1ff, 0x213b},
   {m68000_move_pd_i_32     , 0xf1ff, 0x213c},
   {m68000_move_di_d_32     , 0xf1f8, 0x2140},
   {m68000_move_di_a_32     , 0xf1f8, 0x2148},
   {m68000_move_di_ai_32    , 0xf1f8, 0x2150},
   {m68000_move_di_pi_32    , 0xf1f8, 0x2158},
   {m68000_move_di_pd_32    , 0xf1f8, 0x2160},
   {m68000_move_di_di_32    , 0xf1f8, 0x2168},
   {m68000_move_di_ix_32    , 0xf1f8, 0x2170},
   {m68000_move_di_aw_32    , 0xf1ff, 0x2178},
   {m68000_move_di_al_32    , 0xf1ff, 0x2179},
   {m68000_move_di_pcdi_32  , 0xf1ff, 0x217a},
   {m68000_move_di_pcix_32  , 0xf1ff, 0x217b},
   {m68000_move_di_i_32     , 0xf1ff, 0x217c},
   {m68000_move_ix_d_32     , 0xf1f8, 0x2180},
   {m68000_move_ix_a_32     , 0xf1f8, 0x2188},
   {m68000_move_ix_ai_32    , 0xf1f8, 0x2190},
   {m68000_move_ix_pi_32    , 0xf1f8, 0x2198},
   {m68000_move_ix_pd_32    , 0xf1f8, 0x21a0},
   {m68000_move_ix_di_32    , 0xf1f8, 0x21a8},
   {m68000_move_ix_ix_32    , 0xf1f8, 0x21b0},
   {m68000_move_ix_aw_32    , 0xf1ff, 0x21b8},
   {m68000_move_ix_al_32    , 0xf1ff, 0x21b9},
   {m68000_move_ix_pcdi_32  , 0xf1ff, 0x21ba},
   {m68000_move_ix_pcix_32  , 0xf1ff, 0x21bb},
   {m68000_move_ix_i_32     , 0xf1ff, 0x21bc},
   {m68000_move_aw_d_32     , 0xfff8, 0x21c0},
   {m68000_move_aw_a_32     , 0xfff8, 0x21c8},
   {m68000_move_aw_ai_32    , 0xfff8, 0x21d0},
   {m68000_move_aw_pi_32    , 0xfff8, 0x21d8},
   {m68000_move_aw_pd_32    , 0xfff8, 0x21e0},
   {m68000_move_aw_di_32    , 0xfff8, 0x21e8},
   {m68000_move_aw_ix_32    , 0xfff8, 0x21f0},
   {m68000_move_aw_aw_32    , 0xffff, 0x21f8},
   {m68000_move_aw_al_32    , 0xffff, 0x21f9},
   {m68000_move_aw_pcdi_32  , 0xffff, 0x21fa},
   {m68000_move_aw_pcix_32  , 0xffff, 0x21fb},
   {m68000_move_aw_i_32     , 0xffff, 0x21fc},
   {m68000_move_al_d_32     , 0xfff8, 0x23c0},
   {m68000_move_al_a_32     , 0xfff8, 0x23c8},
   {m68000_move_al_ai_32    , 0xfff8, 0x23d0},
   {m68000_move_al_pi_32    , 0xfff8, 0x23d8},
   {m68000_move_al_pd_32    , 0xfff8, 0x23e0},
   {m68000_move_al_di_32    , 0xfff8, 0x23e8},
   {m68000_move_al_ix_32    , 0xfff8, 0x23f0},
   {m68000_move_al_aw_32    , 0xffff, 0x23f8},
   {m68000_move_al_al_32    , 0xffff, 0x23f9},
   {m68000_move_al_pcdi_32  , 0xffff, 0x23fa},
   {m68000_move_al_pcix_32  , 0xffff, 0x23fb},
   {m68000_move_al_i_32     , 0xffff, 0x23fc},
   {m68000_movea_d_16       , 0xf1f8, 0x3040},
   {m68000_movea_a_16       , 0xf1f8, 0x3048},
   {m68000_movea_ai_16      , 0xf1f8, 0x3050},
   {m68000_movea_pi_16      , 0xf1f8, 0x3058},
   {m68000_movea_pd_16      , 0xf1f8, 0x3060},
   {m68000_movea_di_16      , 0xf1f8, 0x3068},
   {m68000_movea_ix_16      , 0xf1f8, 0x3070},
   {m68000_movea_aw_16      , 0xf1ff, 0x3078},
   {m68000_movea_al_16      , 0xf1ff, 0x3079},
   {m68000_movea_pcdi_16    , 0xf1ff, 0x307a},
   {m68000_movea_pcix_16    , 0xf1ff, 0x307b},
   {m68000_movea_i_16       , 0xf1ff, 0x307c},
   {m68000_movea_d_32       , 0xf1f8, 0x2040},
   {m68000_movea_a_32       , 0xf1f8, 0x2048},
   {m68000_movea_ai_32      , 0xf1f8, 0x2050},
   {m68000_movea_pi_32      , 0xf1f8, 0x2058},
   {m68000_movea_pd_32      , 0xf1f8, 0x2060},
   {m68000_movea_di_32      , 0xf1f8, 0x2068},
   {m68000_movea_ix_32      , 0xf1f8, 0x2070},
   {m68000_movea_aw_32      , 0xf1ff, 0x2078},
   {m68000_movea_al_32      , 0xf1ff, 0x2079},
   {m68000_movea_pcdi_32    , 0xf1ff, 0x207a},
   {m68000_movea_pcix_32    , 0xf1ff, 0x207b},
   {m68000_movea_i_32       , 0xf1ff, 0x207c},
   {m68010_move_fr_ccr_d    , 0xfff8, 0x42c0},
   {m68010_move_fr_ccr_ai   , 0xfff8, 0x42d0},
   {m68010_move_fr_ccr_pi   , 0xfff8, 0x42d8},
   {m68010_move_fr_ccr_pd   , 0xfff8, 0x42e0},
   {m68010_move_fr_ccr_di   , 0xfff8, 0x42e8},
   {m68010_move_fr_ccr_ix   , 0xfff8, 0x42f0},
   {m68010_move_fr_ccr_aw   , 0xffff, 0x42f8},
   {m68010_move_fr_ccr_al   , 0xffff, 0x42f9},
   {m68000_move_to_ccr_d    , 0xfff8, 0x44c0},
   {m68000_move_to_ccr_ai   , 0xfff8, 0x44d0},
   {m68000_move_to_ccr_pi   , 0xfff8, 0x44d8},
   {m68000_move_to_ccr_pd   , 0xfff8, 0x44e0},
   {m68000_move_to_ccr_di   , 0xfff8, 0x44e8},
   {m68000_move_to_ccr_ix   , 0xfff8, 0x44f0},
   {m68000_move_to_ccr_aw   , 0xffff, 0x44f8},
   {m68000_move_to_ccr_al   , 0xffff, 0x44f9},
   {m68000_move_to_ccr_pcdi , 0xffff, 0x44fa},
   {m68000_move_to_ccr_pcix , 0xffff, 0x44fb},
   {m68000_move_to_ccr_i    , 0xffff, 0x44fc},
   {m68000_move_fr_sr_d     , 0xfff8, 0x40c0},
   {m68000_move_fr_sr_ai    , 0xfff8, 0x40d0},
   {m68000_move_fr_sr_pi    , 0xfff8, 0x40d8},
   {m68000_move_fr_sr_pd    , 0xfff8, 0x40e0},
   {m68000_move_fr_sr_di    , 0xfff8, 0x40e8},
   {m68000_move_fr_sr_ix    , 0xfff8, 0x40f0},
   {m68000_move_fr_sr_aw    , 0xffff, 0x40f8},
   {m68000_move_fr_sr_al    , 0xffff, 0x40f9},
   {m68000_move_to_sr_d     , 0xfff8, 0x46c0},
   {m68000_move_to_sr_ai    , 0xfff8, 0x46d0},
   {m68000_move_to_sr_pi    , 0xfff8, 0x46d8},
   {m68000_move_to_sr_pd    , 0xfff8, 0x46e0},
   {m68000_move_to_sr_di    , 0xfff8, 0x46e8},
   {m68000_move_to_sr_ix    , 0xfff8, 0x46f0},
   {m68000_move_to_sr_aw    , 0xffff, 0x46f8},
   {m68000_move_to_sr_al    , 0xffff, 0x46f9},
   {m68000_move_to_sr_pcdi  , 0xffff, 0x46fa},
   {m68000_move_to_sr_pcix  , 0xffff, 0x46fb},
   {m68000_move_to_sr_i     , 0xffff, 0x46fc},
   {m68000_move_fr_usp      , 0xfff8, 0x4e68},
   {m68000_move_to_usp      , 0xfff8, 0x4e60},
   {m68010_movec_cr         , 0xffff, 0x4e7a},
   {m68010_movec_rc         , 0xffff, 0x4e7b},
   {m68000_movem_pd_16      , 0xfff8, 0x48a0},
   {m68000_movem_pd_32      , 0xfff8, 0x48e0},
   {m68000_movem_pi_16      , 0xfff8, 0x4c98},
   {m68000_movem_pi_32      , 0xfff8, 0x4cd8},
   {m68000_movem_re_ai_16   , 0xfff8, 0x4890},
//   {m68000_movem_re_pd_16   , 0xfff8, 0x48a0},
   {m68000_movem_re_di_16   , 0xfff8, 0x48a8},
   {m68000_movem_re_ix_16   , 0xfff8, 0x48b0},
   {m68000_movem_re_aw_16   , 0xffff, 0x48b8},
   {m68000_movem_re_al_16   , 0xffff, 0x48b9},
   {m68000_movem_re_ai_32   , 0xfff8, 0x48d0},
//   {m68000_movem_re_pd_32   , 0xfff8, 0x48e0},
   {m68000_movem_re_di_32   , 0xfff8, 0x48e8},
   {m68000_movem_re_ix_32   , 0xfff8, 0x48f0},
   {m68000_movem_re_aw_32   , 0xffff, 0x48f8},
   {m68000_movem_re_al_32   , 0xffff, 0x48f9},
   {m68000_movem_er_ai_16   , 0xfff8, 0x4c90},
//   {m68000_movem_er_pi_16   , 0xfff8, 0x4c98},
   {m68000_movem_er_di_16   , 0xfff8, 0x4ca8},
   {m68000_movem_er_ix_16   , 0xfff8, 0x4cb0},
   {m68000_movem_er_aw_16   , 0xffff, 0x4cb8},
   {m68000_movem_er_al_16   , 0xffff, 0x4cb9},
   {m68000_movem_er_pcdi_16 , 0xffff, 0x4cba},
   {m68000_movem_er_pcix_16 , 0xffff, 0x4cbb},
   {m68000_movem_er_ai_32   , 0xfff8, 0x4cd0},
//   {m68000_movem_er_pi_32   , 0xfff8, 0x4cd8},
   {m68000_movem_er_di_32   , 0xfff8, 0x4ce8},
   {m68000_movem_er_ix_32   , 0xfff8, 0x4cf0},
   {m68000_movem_er_aw_32   , 0xffff, 0x4cf8},
   {m68000_movem_er_al_32   , 0xffff, 0x4cf9},
   {m68000_movem_er_pcdi_32 , 0xffff, 0x4cfa},
   {m68000_movem_er_pcix_32 , 0xffff, 0x4cfb},
   {m68000_movep_er_16      , 0xf1f8, 0x0108},
   {m68000_movep_er_32      , 0xf1f8, 0x0148},
   {m68000_movep_re_16      , 0xf1f8, 0x0188},
   {m68000_movep_re_32      , 0xf1f8, 0x01c8},
   {m68010_moves_ai_8       , 0xfff8, 0x0e10},
   {m68010_moves_pi_8       , 0xfff8, 0x0e18},
   {m68010_moves_pi7_8      , 0xffff, 0x0e1f},
   {m68010_moves_pd_8       , 0xfff8, 0x0e20},
   {m68010_moves_pd7_8      , 0xffff, 0x0e27},
   {m68010_moves_di_8       , 0xfff8, 0x0e28},
   {m68010_moves_ix_8       , 0xfff8, 0x0e30},
   {m68010_moves_aw_8       , 0xffff, 0x0e38},
   {m68010_moves_al_8       , 0xffff, 0x0e39},
   {m68010_moves_ai_16      , 0xfff8, 0x0e50},
   {m68010_moves_pi_16      , 0xfff8, 0x0e58},
   {m68010_moves_pd_16      , 0xfff8, 0x0e60},
   {m68010_moves_di_16      , 0xfff8, 0x0e68},
   {m68010_moves_ix_16      , 0xfff8, 0x0e70},
   {m68010_moves_aw_16      , 0xffff, 0x0e78},
   {m68010_moves_al_16      , 0xffff, 0x0e79},
   {m68010_moves_ai_32      , 0xfff8, 0x0e90},
   {m68010_moves_pi_32      , 0xfff8, 0x0e98},
   {m68010_moves_pd_32      , 0xfff8, 0x0ea0},
   {m68010_moves_di_32      , 0xfff8, 0x0ea8},
   {m68010_moves_ix_32      , 0xfff8, 0x0eb0},
   {m68010_moves_aw_32      , 0xffff, 0x0eb8},
   {m68010_moves_al_32      , 0xffff, 0x0eb9},
   {m68000_moveq            , 0xf100, 0x7000},
   {m68000_muls_d           , 0xf1f8, 0xc1c0},
   {m68000_muls_ai          , 0xf1f8, 0xc1d0},
   {m68000_muls_pi          , 0xf1f8, 0xc1d8},
   {m68000_muls_pd          , 0xf1f8, 0xc1e0},
   {m68000_muls_di          , 0xf1f8, 0xc1e8},
   {m68000_muls_ix          , 0xf1f8, 0xc1f0},
   {m68000_muls_aw          , 0xf1ff, 0xc1f8},
   {m68000_muls_al          , 0xf1ff, 0xc1f9},
   {m68000_muls_pcdi        , 0xf1ff, 0xc1fa},
   {m68000_muls_pcix        , 0xf1ff, 0xc1fb},
   {m68000_muls_i           , 0xf1ff, 0xc1fc},
   {m68000_mulu_d           , 0xf1f8, 0xc0c0},
   {m68000_mulu_ai          , 0xf1f8, 0xc0d0},
   {m68000_mulu_pi          , 0xf1f8, 0xc0d8},
   {m68000_mulu_pd          , 0xf1f8, 0xc0e0},
   {m68000_mulu_di          , 0xf1f8, 0xc0e8},
   {m68000_mulu_ix          , 0xf1f8, 0xc0f0},
   {m68000_mulu_aw          , 0xf1ff, 0xc0f8},
   {m68000_mulu_al          , 0xf1ff, 0xc0f9},
   {m68000_mulu_pcdi        , 0xf1ff, 0xc0fa},
   {m68000_mulu_pcix        , 0xf1ff, 0xc0fb},
   {m68000_mulu_i           , 0xf1ff, 0xc0fc},
   {m68000_nbcd_d           , 0xfff8, 0x4800},
   {m68000_nbcd_ai          , 0xfff8, 0x4810},
   {m68000_nbcd_pi          , 0xfff8, 0x4818},
   {m68000_nbcd_pi7         , 0xffff, 0x481f},
   {m68000_nbcd_pd          , 0xfff8, 0x4820},
   {m68000_nbcd_pd7         , 0xffff, 0x4827},
   {m68000_nbcd_di          , 0xfff8, 0x4828},
   {m68000_nbcd_ix          , 0xfff8, 0x4830},
   {m68000_nbcd_aw          , 0xffff, 0x4838},
   {m68000_nbcd_al          , 0xffff, 0x4839},
   {m68000_neg_d_8          , 0xfff8, 0x4400},
   {m68000_neg_ai_8         , 0xfff8, 0x4410},
   {m68000_neg_pi_8         , 0xfff8, 0x4418},
   {m68000_neg_pi7_8        , 0xffff, 0x441f},
   {m68000_neg_pd_8         , 0xfff8, 0x4420},
   {m68000_neg_pd7_8        , 0xffff, 0x4427},
   {m68000_neg_di_8         , 0xfff8, 0x4428},
   {m68000_neg_ix_8         , 0xfff8, 0x4430},
   {m68000_neg_aw_8         , 0xffff, 0x4438},
   {m68000_neg_al_8         , 0xffff, 0x4439},
   {m68000_neg_d_16         , 0xfff8, 0x4440},
   {m68000_neg_ai_16        , 0xfff8, 0x4450},
   {m68000_neg_pi_16        , 0xfff8, 0x4458},
   {m68000_neg_pd_16        , 0xfff8, 0x4460},
   {m68000_neg_di_16        , 0xfff8, 0x4468},
   {m68000_neg_ix_16        , 0xfff8, 0x4470},
   {m68000_neg_aw_16        , 0xffff, 0x4478},
   {m68000_neg_al_16        , 0xffff, 0x4479},
   {m68000_neg_d_32         , 0xfff8, 0x4480},
   {m68000_neg_ai_32        , 0xfff8, 0x4490},
   {m68000_neg_pi_32        , 0xfff8, 0x4498},
   {m68000_neg_pd_32        , 0xfff8, 0x44a0},
   {m68000_neg_di_32        , 0xfff8, 0x44a8},
   {m68000_neg_ix_32        , 0xfff8, 0x44b0},
   {m68000_neg_aw_32        , 0xffff, 0x44b8},
   {m68000_neg_al_32        , 0xffff, 0x44b9},
   {m68000_negx_d_8         , 0xfff8, 0x4000},
   {m68000_negx_ai_8        , 0xfff8, 0x4010},
   {m68000_negx_pi_8        , 0xfff8, 0x4018},
   {m68000_negx_pi7_8       , 0xffff, 0x401f},
   {m68000_negx_pd_8        , 0xfff8, 0x4020},
   {m68000_negx_pd7_8       , 0xffff, 0x4027},
   {m68000_negx_di_8        , 0xfff8, 0x4028},
   {m68000_negx_ix_8        , 0xfff8, 0x4030},
   {m68000_negx_aw_8        , 0xffff, 0x4038},
   {m68000_negx_al_8        , 0xffff, 0x4039},
   {m68000_negx_d_16        , 0xfff8, 0x4040},
   {m68000_negx_ai_16       , 0xfff8, 0x4050},
   {m68000_negx_pi_16       , 0xfff8, 0x4058},
   {m68000_negx_pd_16       , 0xfff8, 0x4060},
   {m68000_negx_di_16       , 0xfff8, 0x4068},
   {m68000_negx_ix_16       , 0xfff8, 0x4070},
   {m68000_negx_aw_16       , 0xffff, 0x4078},
   {m68000_negx_al_16       , 0xffff, 0x4079},
   {m68000_negx_d_32        , 0xfff8, 0x4080},
   {m68000_negx_ai_32       , 0xfff8, 0x4090},
   {m68000_negx_pi_32       , 0xfff8, 0x4098},
   {m68000_negx_pd_32       , 0xfff8, 0x40a0},
   {m68000_negx_di_32       , 0xfff8, 0x40a8},
   {m68000_negx_ix_32       , 0xfff8, 0x40b0},
   {m68000_negx_aw_32       , 0xffff, 0x40b8},
   {m68000_negx_al_32       , 0xffff, 0x40b9},
   {m68000_nop              , 0xffff, 0x4e71},
   {m68000_not_d_8          , 0xfff8, 0x4600},
   {m68000_not_ai_8         , 0xfff8, 0x4610},
   {m68000_not_pi_8         , 0xfff8, 0x4618},
   {m68000_not_pi7_8        , 0xffff, 0x461f},
   {m68000_not_pd_8         , 0xfff8, 0x4620},
   {m68000_not_pd7_8        , 0xffff, 0x4627},
   {m68000_not_di_8         , 0xfff8, 0x4628},
   {m68000_not_ix_8         , 0xfff8, 0x4630},
   {m68000_not_aw_8         , 0xffff, 0x4638},
   {m68000_not_al_8         , 0xffff, 0x4639},
   {m68000_not_d_16         , 0xfff8, 0x4640},
   {m68000_not_ai_16        , 0xfff8, 0x4650},
   {m68000_not_pi_16        , 0xfff8, 0x4658},
   {m68000_not_pd_16        , 0xfff8, 0x4660},
   {m68000_not_di_16        , 0xfff8, 0x4668},
   {m68000_not_ix_16        , 0xfff8, 0x4670},
   {m68000_not_aw_16        , 0xffff, 0x4678},
   {m68000_not_al_16        , 0xffff, 0x4679},
   {m68000_not_d_32         , 0xfff8, 0x4680},
   {m68000_not_ai_32        , 0xfff8, 0x4690},
   {m68000_not_pi_32        , 0xfff8, 0x4698},
   {m68000_not_pd_32        , 0xfff8, 0x46a0},
   {m68000_not_di_32        , 0xfff8, 0x46a8},
   {m68000_not_ix_32        , 0xfff8, 0x46b0},
   {m68000_not_aw_32        , 0xffff, 0x46b8},
   {m68000_not_al_32        , 0xffff, 0x46b9},
   {m68000_or_er_d_8        , 0xf1f8, 0x8000},
   {m68000_or_er_ai_8       , 0xf1f8, 0x8010},
   {m68000_or_er_pi_8       , 0xf1f8, 0x8018},
   {m68000_or_er_pi7_8      , 0xf1ff, 0x801f},
   {m68000_or_er_pd_8       , 0xf1f8, 0x8020},
   {m68000_or_er_pd7_8      , 0xf1ff, 0x8027},
   {m68000_or_er_di_8       , 0xf1f8, 0x8028},
   {m68000_or_er_ix_8       , 0xf1f8, 0x8030},
   {m68000_or_er_aw_8       , 0xf1ff, 0x8038},
   {m68000_or_er_al_8       , 0xf1ff, 0x8039},
   {m68000_or_er_pcdi_8     , 0xf1ff, 0x803a},
   {m68000_or_er_pcix_8     , 0xf1ff, 0x803b},
   {m68000_or_er_i_8        , 0xf1ff, 0x803c},
   {m68000_or_er_d_16       , 0xf1f8, 0x8040},
   {m68000_or_er_ai_16      , 0xf1f8, 0x8050},
   {m68000_or_er_pi_16      , 0xf1f8, 0x8058},
   {m68000_or_er_pd_16      , 0xf1f8, 0x8060},
   {m68000_or_er_di_16      , 0xf1f8, 0x8068},
   {m68000_or_er_ix_16      , 0xf1f8, 0x8070},
   {m68000_or_er_aw_16      , 0xf1ff, 0x8078},
   {m68000_or_er_al_16      , 0xf1ff, 0x8079},
   {m68000_or_er_pcdi_16    , 0xf1ff, 0x807a},
   {m68000_or_er_pcix_16    , 0xf1ff, 0x807b},
   {m68000_or_er_i_16       , 0xf1ff, 0x807c},
   {m68000_or_er_d_32       , 0xf1f8, 0x8080},
   {m68000_or_er_ai_32      , 0xf1f8, 0x8090},
   {m68000_or_er_pi_32      , 0xf1f8, 0x8098},
   {m68000_or_er_pd_32      , 0xf1f8, 0x80a0},
   {m68000_or_er_di_32      , 0xf1f8, 0x80a8},
   {m68000_or_er_ix_32      , 0xf1f8, 0x80b0},
   {m68000_or_er_aw_32      , 0xf1ff, 0x80b8},
   {m68000_or_er_al_32      , 0xf1ff, 0x80b9},
   {m68000_or_er_pcdi_32    , 0xf1ff, 0x80ba},
   {m68000_or_er_pcix_32    , 0xf1ff, 0x80bb},
   {m68000_or_er_i_32       , 0xf1ff, 0x80bc},
   {m68000_or_re_ai_8       , 0xf1f8, 0x8110},
   {m68000_or_re_pi_8       , 0xf1f8, 0x8118},
   {m68000_or_re_pi7_8      , 0xf1ff, 0x811f},
   {m68000_or_re_pd_8       , 0xf1f8, 0x8120},
   {m68000_or_re_pd7_8      , 0xf1ff, 0x8127},
   {m68000_or_re_di_8       , 0xf1f8, 0x8128},
   {m68000_or_re_ix_8       , 0xf1f8, 0x8130},
   {m68000_or_re_aw_8       , 0xf1ff, 0x8138},
   {m68000_or_re_al_8       , 0xf1ff, 0x8139},
   {m68000_or_re_ai_16      , 0xf1f8, 0x8150},
   {m68000_or_re_pi_16      , 0xf1f8, 0x8158},
   {m68000_or_re_pd_16      , 0xf1f8, 0x8160},
   {m68000_or_re_di_16      , 0xf1f8, 0x8168},
   {m68000_or_re_ix_16      , 0xf1f8, 0x8170},
   {m68000_or_re_aw_16      , 0xf1ff, 0x8178},
   {m68000_or_re_al_16      , 0xf1ff, 0x8179},
   {m68000_or_re_ai_32      , 0xf1f8, 0x8190},
   {m68000_or_re_pi_32      , 0xf1f8, 0x8198},
   {m68000_or_re_pd_32      , 0xf1f8, 0x81a0},
   {m68000_or_re_di_32      , 0xf1f8, 0x81a8},
   {m68000_or_re_ix_32      , 0xf1f8, 0x81b0},
   {m68000_or_re_aw_32      , 0xf1ff, 0x81b8},
   {m68000_or_re_al_32      , 0xf1ff, 0x81b9},
   {m68000_ori_to_ccr       , 0xffff, 0x003c},
   {m68000_ori_to_sr        , 0xffff, 0x007c},
   {m68000_ori_d_8          , 0xfff8, 0x0000},
   {m68000_ori_ai_8         , 0xfff8, 0x0010},
   {m68000_ori_pi_8         , 0xfff8, 0x0018},
   {m68000_ori_pi7_8        , 0xffff, 0x001f},
   {m68000_ori_pd_8         , 0xfff8, 0x0020},
   {m68000_ori_pd7_8        , 0xffff, 0x0027},
   {m68000_ori_di_8         , 0xfff8, 0x0028},
   {m68000_ori_ix_8         , 0xfff8, 0x0030},
   {m68000_ori_aw_8         , 0xffff, 0x0038},
   {m68000_ori_al_8         , 0xffff, 0x0039},
   {m68000_ori_d_16         , 0xfff8, 0x0040},
   {m68000_ori_ai_16        , 0xfff8, 0x0050},
   {m68000_ori_pi_16        , 0xfff8, 0x0058},
   {m68000_ori_pd_16        , 0xfff8, 0x0060},
   {m68000_ori_di_16        , 0xfff8, 0x0068},
   {m68000_ori_ix_16        , 0xfff8, 0x0070},
   {m68000_ori_aw_16        , 0xffff, 0x0078},
   {m68000_ori_al_16        , 0xffff, 0x0079},
   {m68000_ori_d_32         , 0xfff8, 0x0080},
   {m68000_ori_ai_32        , 0xfff8, 0x0090},
   {m68000_ori_pi_32        , 0xfff8, 0x0098},
   {m68000_ori_pd_32        , 0xfff8, 0x00a0},
   {m68000_ori_di_32        , 0xfff8, 0x00a8},
   {m68000_ori_ix_32        , 0xfff8, 0x00b0},
   {m68000_ori_aw_32        , 0xffff, 0x00b8},
   {m68000_ori_al_32        , 0xffff, 0x00b9},
   {m68000_pea_ai           , 0xfff8, 0x4850},
   {m68000_pea_di           , 0xfff8, 0x4868},
   {m68000_pea_ix           , 0xfff8, 0x4870},
   {m68000_pea_aw           , 0xffff, 0x4878},
   {m68000_pea_al           , 0xffff, 0x4879},
   {m68000_pea_pcdi         , 0xffff, 0x487a},
   {m68000_pea_pcix         , 0xffff, 0x487b},
   {m68000_reset            , 0xffff, 0x4e70},
   {m68000_ror_s_8          , 0xf1f8, 0xe018},
   {m68000_ror_s_16         , 0xf1f8, 0xe058},
   {m68000_ror_s_32         , 0xf1f8, 0xe098},
   {m68000_ror_r_8          , 0xf1f8, 0xe038},
   {m68000_ror_r_16         , 0xf1f8, 0xe078},
   {m68000_ror_r_32         , 0xf1f8, 0xe0b8},
   {m68000_ror_ea_ai        , 0xfff8, 0xe6d0},
   {m68000_ror_ea_pi        , 0xfff8, 0xe6d8},
   {m68000_ror_ea_pd        , 0xfff8, 0xe6e0},
   {m68000_ror_ea_di        , 0xfff8, 0xe6e8},
   {m68000_ror_ea_ix        , 0xfff8, 0xe6f0},
   {m68000_ror_ea_aw        , 0xffff, 0xe6f8},
   {m68000_ror_ea_al        , 0xffff, 0xe6f9},
   {m68000_rol_s_8          , 0xf1f8, 0xe118},
   {m68000_rol_s_16         , 0xf1f8, 0xe158},
   {m68000_rol_s_32         , 0xf1f8, 0xe198},
   {m68000_rol_r_8          , 0xf1f8, 0xe138},
   {m68000_rol_r_16         , 0xf1f8, 0xe178},
   {m68000_rol_r_32         , 0xf1f8, 0xe1b8},
   {m68000_rol_ea_ai        , 0xfff8, 0xe7d0},
   {m68000_rol_ea_pi        , 0xfff8, 0xe7d8},
   {m68000_rol_ea_pd        , 0xfff8, 0xe7e0},
   {m68000_rol_ea_di        , 0xfff8, 0xe7e8},
   {m68000_rol_ea_ix        , 0xfff8, 0xe7f0},
   {m68000_rol_ea_aw        , 0xffff, 0xe7f8},
   {m68000_rol_ea_al        , 0xffff, 0xe7f9},
   {m68000_roxr_s_8         , 0xf1f8, 0xe010},
   {m68000_roxr_s_16        , 0xf1f8, 0xe050},
   {m68000_roxr_s_32        , 0xf1f8, 0xe090},
   {m68000_roxr_r_8         , 0xf1f8, 0xe030},
   {m68000_roxr_r_16        , 0xf1f8, 0xe070},
   {m68000_roxr_r_32        , 0xf1f8, 0xe0b0},
   {m68000_roxr_ea_ai       , 0xfff8, 0xe4d0},
   {m68000_roxr_ea_pi       , 0xfff8, 0xe4d8},
   {m68000_roxr_ea_pd       , 0xfff8, 0xe4e0},
   {m68000_roxr_ea_di       , 0xfff8, 0xe4e8},
   {m68000_roxr_ea_ix       , 0xfff8, 0xe4f0},
   {m68000_roxr_ea_aw       , 0xffff, 0xe4f8},
   {m68000_roxr_ea_al       , 0xffff, 0xe4f9},
   {m68000_roxl_s_8         , 0xf1f8, 0xe110},
   {m68000_roxl_s_16        , 0xf1f8, 0xe150},
   {m68000_roxl_s_32        , 0xf1f8, 0xe190},
   {m68000_roxl_r_8         , 0xf1f8, 0xe130},
   {m68000_roxl_r_16        , 0xf1f8, 0xe170},
   {m68000_roxl_r_32        , 0xf1f8, 0xe1b0},
   {m68000_roxl_ea_ai       , 0xfff8, 0xe5d0},
   {m68000_roxl_ea_pi       , 0xfff8, 0xe5d8},
   {m68000_roxl_ea_pd       , 0xfff8, 0xe5e0},
   {m68000_roxl_ea_di       , 0xfff8, 0xe5e8},
   {m68000_roxl_ea_ix       , 0xfff8, 0xe5f0},
   {m68000_roxl_ea_aw       , 0xffff, 0xe5f8},
   {m68000_roxl_ea_al       , 0xffff, 0xe5f9},
   {m68010_rtd              , 0xffff, 0x4e74},
   {m68000_rte              , 0xffff, 0x4e73},
   {m68000_rtr              , 0xffff, 0x4e77},
   {m68000_rts              , 0xffff, 0x4e75},
   {m68000_sbcd_rr          , 0xf1f8, 0x8100},
   {m68000_sbcd_mm_ax7      , 0xfff8, 0x8f08},
   {m68000_sbcd_mm_ay7      , 0xf1ff, 0x810f},
   {m68000_sbcd_mm_axy7     , 0xffff, 0x8f0f},
   {m68000_sbcd_mm          , 0xf1f8, 0x8108},
   {m68000_st_d             , 0xfff8, 0x50c0},
   {m68000_st_ai            , 0xfff8, 0x50d0},
   {m68000_st_pi            , 0xfff8, 0x50d8},
   {m68000_st_pi7           , 0xffff, 0x50df},
   {m68000_st_pd            , 0xfff8, 0x50e0},
   {m68000_st_pd7           , 0xffff, 0x50e7},
   {m68000_st_di            , 0xfff8, 0x50e8},
   {m68000_st_ix            , 0xfff8, 0x50f0},
   {m68000_st_aw            , 0xffff, 0x50f8},
   {m68000_st_al            , 0xffff, 0x50f9},
   {m68000_sf_d             , 0xfff8, 0x51c0},
   {m68000_sf_ai            , 0xfff8, 0x51d0},
   {m68000_sf_pi            , 0xfff8, 0x51d8},
   {m68000_sf_pi7           , 0xffff, 0x51df},
   {m68000_sf_pd            , 0xfff8, 0x51e0},
   {m68000_sf_pd7           , 0xffff, 0x51e7},
   {m68000_sf_di            , 0xfff8, 0x51e8},
   {m68000_sf_ix            , 0xfff8, 0x51f0},
   {m68000_sf_aw            , 0xffff, 0x51f8},
   {m68000_sf_al            , 0xffff, 0x51f9},
   {m68000_shi_d            , 0xfff8, 0x52c0},
   {m68000_shi_ai           , 0xfff8, 0x52d0},
   {m68000_shi_pi           , 0xfff8, 0x52d8},
   {m68000_shi_pi7          , 0xffff, 0x52df},
   {m68000_shi_pd           , 0xfff8, 0x52e0},
   {m68000_shi_pd7          , 0xffff, 0x52e7},
   {m68000_shi_di           , 0xfff8, 0x52e8},
   {m68000_shi_ix           , 0xfff8, 0x52f0},
   {m68000_shi_aw           , 0xffff, 0x52f8},
   {m68000_shi_al           , 0xffff, 0x52f9},
   {m68000_sls_d            , 0xfff8, 0x53c0},
   {m68000_sls_ai           , 0xfff8, 0x53d0},
   {m68000_sls_pi           , 0xfff8, 0x53d8},
   {m68000_sls_pi7          , 0xffff, 0x53df},
   {m68000_sls_pd           , 0xfff8, 0x53e0},
   {m68000_sls_pd7          , 0xffff, 0x53e7},
   {m68000_sls_di           , 0xfff8, 0x53e8},
   {m68000_sls_ix           , 0xfff8, 0x53f0},
   {m68000_sls_aw           , 0xffff, 0x53f8},
   {m68000_sls_al           , 0xffff, 0x53f9},
   {m68000_scc_d            , 0xfff8, 0x54c0},
   {m68000_scc_ai           , 0xfff8, 0x54d0},
   {m68000_scc_pi           , 0xfff8, 0x54d8},
   {m68000_scc_pi7          , 0xffff, 0x54df},
   {m68000_scc_pd           , 0xfff8, 0x54e0},
   {m68000_scc_pd7          , 0xffff, 0x54e7},
   {m68000_scc_di           , 0xfff8, 0x54e8},
   {m68000_scc_ix           , 0xfff8, 0x54f0},
   {m68000_scc_aw           , 0xffff, 0x54f8},
   {m68000_scc_al           , 0xffff, 0x54f9},
   {m68000_scs_d            , 0xfff8, 0x55c0},
   {m68000_scs_ai           , 0xfff8, 0x55d0},
   {m68000_scs_pi           , 0xfff8, 0x55d8},
   {m68000_scs_pi7          , 0xffff, 0x55df},
   {m68000_scs_pd           , 0xfff8, 0x55e0},
   {m68000_scs_pd7          , 0xffff, 0x55e7},
   {m68000_scs_di           , 0xfff8, 0x55e8},
   {m68000_scs_ix           , 0xfff8, 0x55f0},
   {m68000_scs_aw           , 0xffff, 0x55f8},
   {m68000_scs_al           , 0xffff, 0x55f9},
   {m68000_sne_d            , 0xfff8, 0x56c0},
   {m68000_sne_ai           , 0xfff8, 0x56d0},
   {m68000_sne_pi           , 0xfff8, 0x56d8},
   {m68000_sne_pi7          , 0xffff, 0x56df},
   {m68000_sne_pd           , 0xfff8, 0x56e0},
   {m68000_sne_pd7          , 0xffff, 0x56e7},
   {m68000_sne_di           , 0xfff8, 0x56e8},
   {m68000_sne_ix           , 0xfff8, 0x56f0},
   {m68000_sne_aw           , 0xffff, 0x56f8},
   {m68000_sne_al           , 0xffff, 0x56f9},
   {m68000_seq_d            , 0xfff8, 0x57c0},
   {m68000_seq_ai           , 0xfff8, 0x57d0},
   {m68000_seq_pi           , 0xfff8, 0x57d8},
   {m68000_seq_pi7          , 0xffff, 0x57df},
   {m68000_seq_pd           , 0xfff8, 0x57e0},
   {m68000_seq_pd7          , 0xffff, 0x57e7},
   {m68000_seq_di           , 0xfff8, 0x57e8},
   {m68000_seq_ix           , 0xfff8, 0x57f0},
   {m68000_seq_aw           , 0xffff, 0x57f8},
   {m68000_seq_al           , 0xffff, 0x57f9},
   {m68000_svc_d            , 0xfff8, 0x58c0},
   {m68000_svc_ai           , 0xfff8, 0x58d0},
   {m68000_svc_pi           , 0xfff8, 0x58d8},
   {m68000_svc_pi7          , 0xffff, 0x58df},
   {m68000_svc_pd           , 0xfff8, 0x58e0},
   {m68000_svc_pd7          , 0xffff, 0x58e7},
   {m68000_svc_di           , 0xfff8, 0x58e8},
   {m68000_svc_ix           , 0xfff8, 0x58f0},
   {m68000_svc_aw           , 0xffff, 0x58f8},
   {m68000_svc_al           , 0xffff, 0x58f9},
   {m68000_svs_d            , 0xfff8, 0x59c0},
   {m68000_svs_ai           , 0xfff8, 0x59d0},
   {m68000_svs_pi           , 0xfff8, 0x59d8},
   {m68000_svs_pi7          , 0xffff, 0x59df},
   {m68000_svs_pd           , 0xfff8, 0x59e0},
   {m68000_svs_pd7          , 0xffff, 0x59e7},
   {m68000_svs_di           , 0xfff8, 0x59e8},
   {m68000_svs_ix           , 0xfff8, 0x59f0},
   {m68000_svs_aw           , 0xffff, 0x59f8},
   {m68000_svs_al           , 0xffff, 0x59f9},
   {m68000_spl_d            , 0xfff8, 0x5ac0},
   {m68000_spl_ai           , 0xfff8, 0x5ad0},
   {m68000_spl_pi           , 0xfff8, 0x5ad8},
   {m68000_spl_pi7          , 0xffff, 0x5adf},
   {m68000_spl_pd           , 0xfff8, 0x5ae0},
   {m68000_spl_pd7          , 0xffff, 0x5ae7},
   {m68000_spl_di           , 0xfff8, 0x5ae8},
   {m68000_spl_ix           , 0xfff8, 0x5af0},
   {m68000_spl_aw           , 0xffff, 0x5af8},
   {m68000_spl_al           , 0xffff, 0x5af9},
   {m68000_smi_d            , 0xfff8, 0x5bc0},
   {m68000_smi_ai           , 0xfff8, 0x5bd0},
   {m68000_smi_pi           , 0xfff8, 0x5bd8},
   {m68000_smi_pi7          , 0xffff, 0x5bdf},
   {m68000_smi_pd           , 0xfff8, 0x5be0},
   {m68000_smi_pd7          , 0xffff, 0x5be7},
   {m68000_smi_di           , 0xfff8, 0x5be8},
   {m68000_smi_ix           , 0xfff8, 0x5bf0},
   {m68000_smi_aw           , 0xffff, 0x5bf8},
   {m68000_smi_al           , 0xffff, 0x5bf9},
   {m68000_sge_d            , 0xfff8, 0x5cc0},
   {m68000_sge_ai           , 0xfff8, 0x5cd0},
   {m68000_sge_pi           , 0xfff8, 0x5cd8},
   {m68000_sge_pi7          , 0xffff, 0x5cdf},
   {m68000_sge_pd           , 0xfff8, 0x5ce0},
   {m68000_sge_pd7          , 0xffff, 0x5ce7},
   {m68000_sge_di           , 0xfff8, 0x5ce8},
   {m68000_sge_ix           , 0xfff8, 0x5cf0},
   {m68000_sge_aw           , 0xffff, 0x5cf8},
   {m68000_sge_al           , 0xffff, 0x5cf9},
   {m68000_slt_d            , 0xfff8, 0x5dc0},
   {m68000_slt_ai           , 0xfff8, 0x5dd0},
   {m68000_slt_pi           , 0xfff8, 0x5dd8},
   {m68000_slt_pi7          , 0xffff, 0x5ddf},
   {m68000_slt_pd           , 0xfff8, 0x5de0},
   {m68000_slt_pd7          , 0xffff, 0x5de7},
   {m68000_slt_di           , 0xfff8, 0x5de8},
   {m68000_slt_ix           , 0xfff8, 0x5df0},
   {m68000_slt_aw           , 0xffff, 0x5df8},
   {m68000_slt_al           , 0xffff, 0x5df9},
   {m68000_sgt_d            , 0xfff8, 0x5ec0},
   {m68000_sgt_ai           , 0xfff8, 0x5ed0},
   {m68000_sgt_pi           , 0xfff8, 0x5ed8},
   {m68000_sgt_pi7          , 0xffff, 0x5edf},
   {m68000_sgt_pd           , 0xfff8, 0x5ee0},
   {m68000_sgt_pd7          , 0xffff, 0x5ee7},
   {m68000_sgt_di           , 0xfff8, 0x5ee8},
   {m68000_sgt_ix           , 0xfff8, 0x5ef0},
   {m68000_sgt_aw           , 0xffff, 0x5ef8},
   {m68000_sgt_al           , 0xffff, 0x5ef9},
   {m68000_sle_d            , 0xfff8, 0x5fc0},
   {m68000_sle_ai           , 0xfff8, 0x5fd0},
   {m68000_sle_pi           , 0xfff8, 0x5fd8},
   {m68000_sle_pi7          , 0xffff, 0x5fdf},
   {m68000_sle_pd           , 0xfff8, 0x5fe0},
   {m68000_sle_pd7          , 0xffff, 0x5fe7},
   {m68000_sle_di           , 0xfff8, 0x5fe8},
   {m68000_sle_ix           , 0xfff8, 0x5ff0},
   {m68000_sle_aw           , 0xffff, 0x5ff8},
   {m68000_sle_al           , 0xffff, 0x5ff9},
   {m68000_stop             , 0xffff, 0x4e72},
   {m68000_sub_er_d_8       , 0xf1f8, 0x9000},
   {m68000_sub_er_ai_8      , 0xf1f8, 0x9010},
   {m68000_sub_er_pi_8      , 0xf1f8, 0x9018},
   {m68000_sub_er_pi7_8     , 0xf1ff, 0x901f},
   {m68000_sub_er_pd_8      , 0xf1f8, 0x9020},
   {m68000_sub_er_pd7_8     , 0xf1ff, 0x9027},
   {m68000_sub_er_di_8      , 0xf1f8, 0x9028},
   {m68000_sub_er_ix_8      , 0xf1f8, 0x9030},
   {m68000_sub_er_aw_8      , 0xf1ff, 0x9038},
   {m68000_sub_er_al_8      , 0xf1ff, 0x9039},
   {m68000_sub_er_pcdi_8    , 0xf1ff, 0x903a},
   {m68000_sub_er_pcix_8    , 0xf1ff, 0x903b},
   {m68000_sub_er_i_8       , 0xf1ff, 0x903c},
   {m68000_sub_er_d_16      , 0xf1f8, 0x9040},
   {m68000_sub_er_a_16      , 0xf1f8, 0x9048},
   {m68000_sub_er_ai_16     , 0xf1f8, 0x9050},
   {m68000_sub_er_pi_16     , 0xf1f8, 0x9058},
   {m68000_sub_er_pd_16     , 0xf1f8, 0x9060},
   {m68000_sub_er_di_16     , 0xf1f8, 0x9068},
   {m68000_sub_er_ix_16     , 0xf1f8, 0x9070},
   {m68000_sub_er_aw_16     , 0xf1ff, 0x9078},
   {m68000_sub_er_al_16     , 0xf1ff, 0x9079},
   {m68000_sub_er_pcdi_16   , 0xf1ff, 0x907a},
   {m68000_sub_er_pcix_16   , 0xf1ff, 0x907b},
   {m68000_sub_er_i_16      , 0xf1ff, 0x907c},
   {m68000_sub_er_d_32      , 0xf1f8, 0x9080},
   {m68000_sub_er_a_32      , 0xf1f8, 0x9088},
   {m68000_sub_er_ai_32     , 0xf1f8, 0x9090},
   {m68000_sub_er_pi_32     , 0xf1f8, 0x9098},
   {m68000_sub_er_pd_32     , 0xf1f8, 0x90a0},
   {m68000_sub_er_di_32     , 0xf1f8, 0x90a8},
   {m68000_sub_er_ix_32     , 0xf1f8, 0x90b0},
   {m68000_sub_er_aw_32     , 0xf1ff, 0x90b8},
   {m68000_sub_er_al_32     , 0xf1ff, 0x90b9},
   {m68000_sub_er_pcdi_32   , 0xf1ff, 0x90ba},
   {m68000_sub_er_pcix_32   , 0xf1ff, 0x90bb},
   {m68000_sub_er_i_32      , 0xf1ff, 0x90bc},
   {m68000_sub_re_ai_8      , 0xf1f8, 0x9110},
   {m68000_sub_re_pi_8      , 0xf1f8, 0x9118},
   {m68000_sub_re_pi7_8     , 0xf1ff, 0x911f},
   {m68000_sub_re_pd_8      , 0xf1f8, 0x9120},
   {m68000_sub_re_pd7_8     , 0xf1ff, 0x9127},
   {m68000_sub_re_di_8      , 0xf1f8, 0x9128},
   {m68000_sub_re_ix_8      , 0xf1f8, 0x9130},
   {m68000_sub_re_aw_8      , 0xf1ff, 0x9138},
   {m68000_sub_re_al_8      , 0xf1ff, 0x9139},
   {m68000_sub_re_ai_16     , 0xf1f8, 0x9150},
   {m68000_sub_re_pi_16     , 0xf1f8, 0x9158},
   {m68000_sub_re_pd_16     , 0xf1f8, 0x9160},
   {m68000_sub_re_di_16     , 0xf1f8, 0x9168},
   {m68000_sub_re_ix_16     , 0xf1f8, 0x9170},
   {m68000_sub_re_aw_16     , 0xf1ff, 0x9178},
   {m68000_sub_re_al_16     , 0xf1ff, 0x9179},
   {m68000_sub_re_ai_32     , 0xf1f8, 0x9190},
   {m68000_sub_re_pi_32     , 0xf1f8, 0x9198},
   {m68000_sub_re_pd_32     , 0xf1f8, 0x91a0},
   {m68000_sub_re_di_32     , 0xf1f8, 0x91a8},
   {m68000_sub_re_ix_32     , 0xf1f8, 0x91b0},
   {m68000_sub_re_aw_32     , 0xf1ff, 0x91b8},
   {m68000_sub_re_al_32     , 0xf1ff, 0x91b9},
   {m68000_suba_d_16        , 0xf1f8, 0x90c0},
   {m68000_suba_a_16        , 0xf1f8, 0x90c8},
   {m68000_suba_ai_16       , 0xf1f8, 0x90d0},
   {m68000_suba_pi_16       , 0xf1f8, 0x90d8},
   {m68000_suba_pd_16       , 0xf1f8, 0x90e0},
   {m68000_suba_di_16       , 0xf1f8, 0x90e8},
   {m68000_suba_ix_16       , 0xf1f8, 0x90f0},
   {m68000_suba_aw_16       , 0xf1ff, 0x90f8},
   {m68000_suba_al_16       , 0xf1ff, 0x90f9},
   {m68000_suba_pcdi_16     , 0xf1ff, 0x90fa},
   {m68000_suba_pcix_16     , 0xf1ff, 0x90fb},
   {m68000_suba_i_16        , 0xf1ff, 0x90fc},
   {m68000_suba_d_32        , 0xf1f8, 0x91c0},
   {m68000_suba_a_32        , 0xf1f8, 0x91c8},
   {m68000_suba_ai_32       , 0xf1f8, 0x91d0},
   {m68000_suba_pi_32       , 0xf1f8, 0x91d8},
   {m68000_suba_pd_32       , 0xf1f8, 0x91e0},
   {m68000_suba_di_32       , 0xf1f8, 0x91e8},
   {m68000_suba_ix_32       , 0xf1f8, 0x91f0},
   {m68000_suba_aw_32       , 0xf1ff, 0x91f8},
   {m68000_suba_al_32       , 0xf1ff, 0x91f9},
   {m68000_suba_pcdi_32     , 0xf1ff, 0x91fa},
   {m68000_suba_pcix_32     , 0xf1ff, 0x91fb},
   {m68000_suba_i_32        , 0xf1ff, 0x91fc},
   {m68000_subi_d_8         , 0xfff8, 0x0400},
   {m68000_subi_ai_8        , 0xfff8, 0x0410},
   {m68000_subi_pi_8        , 0xfff8, 0x0418},
   {m68000_subi_pi7_8       , 0xffff, 0x041f},
   {m68000_subi_pd_8        , 0xfff8, 0x0420},
   {m68000_subi_pd7_8       , 0xffff, 0x0427},
   {m68000_subi_di_8        , 0xfff8, 0x0428},
   {m68000_subi_ix_8        , 0xfff8, 0x0430},
   {m68000_subi_aw_8        , 0xffff, 0x0438},
   {m68000_subi_al_8        , 0xffff, 0x0439},
   {m68000_subi_d_16        , 0xfff8, 0x0440},
   {m68000_subi_ai_16       , 0xfff8, 0x0450},
   {m68000_subi_pi_16       , 0xfff8, 0x0458},
   {m68000_subi_pd_16       , 0xfff8, 0x0460},
   {m68000_subi_di_16       , 0xfff8, 0x0468},
   {m68000_subi_ix_16       , 0xfff8, 0x0470},
   {m68000_subi_aw_16       , 0xffff, 0x0478},
   {m68000_subi_al_16       , 0xffff, 0x0479},
   {m68000_subi_d_32        , 0xfff8, 0x0480},
   {m68000_subi_ai_32       , 0xfff8, 0x0490},
   {m68000_subi_pi_32       , 0xfff8, 0x0498},
   {m68000_subi_pd_32       , 0xfff8, 0x04a0},
   {m68000_subi_di_32       , 0xfff8, 0x04a8},
   {m68000_subi_ix_32       , 0xfff8, 0x04b0},
   {m68000_subi_aw_32       , 0xffff, 0x04b8},
   {m68000_subi_al_32       , 0xffff, 0x04b9},
   {m68000_subq_d_8         , 0xf1f8, 0x5100},
   {m68000_subq_ai_8        , 0xf1f8, 0x5110},
   {m68000_subq_pi_8        , 0xf1f8, 0x5118},
   {m68000_subq_pi7_8       , 0xf1ff, 0x511f},
   {m68000_subq_pd_8        , 0xf1f8, 0x5120},
   {m68000_subq_pd7_8       , 0xf1ff, 0x5127},
   {m68000_subq_di_8        , 0xf1f8, 0x5128},
   {m68000_subq_ix_8        , 0xf1f8, 0x5130},
   {m68000_subq_aw_8        , 0xf1ff, 0x5138},
   {m68000_subq_al_8        , 0xf1ff, 0x5139},
   {m68000_subq_d_16        , 0xf1f8, 0x5140},
   {m68000_subq_a_16        , 0xf1f8, 0x5148},
   {m68000_subq_ai_16       , 0xf1f8, 0x5150},
   {m68000_subq_pi_16       , 0xf1f8, 0x5158},
   {m68000_subq_pd_16       , 0xf1f8, 0x5160},
   {m68000_subq_di_16       , 0xf1f8, 0x5168},
   {m68000_subq_ix_16       , 0xf1f8, 0x5170},
   {m68000_subq_aw_16       , 0xf1ff, 0x5178},
   {m68000_subq_al_16       , 0xf1ff, 0x5179},
   {m68000_subq_d_32        , 0xf1f8, 0x5180},
   {m68000_subq_a_32        , 0xf1f8, 0x5188},
   {m68000_subq_ai_32       , 0xf1f8, 0x5190},
   {m68000_subq_pi_32       , 0xf1f8, 0x5198},
   {m68000_subq_pd_32       , 0xf1f8, 0x51a0},
   {m68000_subq_di_32       , 0xf1f8, 0x51a8},
   {m68000_subq_ix_32       , 0xf1f8, 0x51b0},
   {m68000_subq_aw_32       , 0xf1ff, 0x51b8},
   {m68000_subq_al_32       , 0xf1ff, 0x51b9},
   {m68000_subx_rr_8        , 0xf1f8, 0x9100},
   {m68000_subx_rr_16       , 0xf1f8, 0x9140},
   {m68000_subx_rr_32       , 0xf1f8, 0x9180},
   {m68000_subx_mm_8_ax7    , 0xfff8, 0x9f08},
   {m68000_subx_mm_8_ay7    , 0xf1ff, 0x910f},
   {m68000_subx_mm_8_axy7   , 0xffff, 0x9f0f},
   {m68000_subx_mm_8        , 0xf1f8, 0x9108},
   {m68000_subx_mm_16       , 0xf1f8, 0x9148},
   {m68000_subx_mm_32       , 0xf1f8, 0x9188},
   {m68000_swap             , 0xfff8, 0x4840},
   {m68000_tas_d            , 0xfff8, 0x4ac0},
   {m68000_tas_ai           , 0xfff8, 0x4ad0},
   {m68000_tas_pi           , 0xfff8, 0x4ad8},
   {m68000_tas_pi7          , 0xffff, 0x4adf},
   {m68000_tas_pd           , 0xfff8, 0x4ae0},
   {m68000_tas_pd7          , 0xffff, 0x4ae7},
   {m68000_tas_di           , 0xfff8, 0x4ae8},
   {m68000_tas_ix           , 0xfff8, 0x4af0},
   {m68000_tas_aw           , 0xffff, 0x4af8},
   {m68000_tas_al           , 0xffff, 0x4af9},
   {m68000_trap             , 0xfff0, 0x4e40},
   {m68000_trapv            , 0xffff, 0x4e76},
   {m68000_tst_d_8          , 0xfff8, 0x4a00},
   {m68000_tst_ai_8         , 0xfff8, 0x4a10},
   {m68000_tst_pi_8         , 0xfff8, 0x4a18},
   {m68000_tst_pi7_8        , 0xffff, 0x4a1f},
   {m68000_tst_pd_8         , 0xfff8, 0x4a20},
   {m68000_tst_pd7_8        , 0xffff, 0x4a27},
   {m68000_tst_di_8         , 0xfff8, 0x4a28},
   {m68000_tst_ix_8         , 0xfff8, 0x4a30},
   {m68000_tst_aw_8         , 0xffff, 0x4a38},
   {m68000_tst_al_8         , 0xffff, 0x4a39},
   {m68000_tst_d_16         , 0xfff8, 0x4a40},
   {m68000_tst_ai_16        , 0xfff8, 0x4a50},
   {m68000_tst_pi_16        , 0xfff8, 0x4a58},
   {m68000_tst_pd_16        , 0xfff8, 0x4a60},
   {m68000_tst_di_16        , 0xfff8, 0x4a68},
   {m68000_tst_ix_16        , 0xfff8, 0x4a70},
   {m68000_tst_aw_16        , 0xffff, 0x4a78},
   {m68000_tst_al_16        , 0xffff, 0x4a79},
   {m68000_tst_d_32         , 0xfff8, 0x4a80},
   {m68000_tst_ai_32        , 0xfff8, 0x4a90},
   {m68000_tst_pi_32        , 0xfff8, 0x4a98},
   {m68000_tst_pd_32        , 0xfff8, 0x4aa0},
   {m68000_tst_di_32        , 0xfff8, 0x4aa8},
   {m68000_tst_ix_32        , 0xfff8, 0x4ab0},
   {m68000_tst_aw_32        , 0xffff, 0x4ab8},
   {m68000_tst_al_32        , 0xffff, 0x4ab9},
   {m68000_unlk_a7          , 0xffff, 0x4e5f},
   {m68000_unlk             , 0xfff8, 0x4e58},
   {m68000_illegal          , 0xffff, 0x4afc},
   {0, 0, 0}
};


/* Comparison function for qsort() */
static int QSORT_CALLBACK_DECL compare_nof_true_bits(const void* aptr, const void* bptr)
{
   uint a = ((opcode_struct*)aptr)->mask;
   uint b = ((opcode_struct*)bptr)->mask;

   a = ((a & 0xAAAA) >> 1) + (a & 0x5555);
   a = ((a & 0xCCCC) >> 2) + (a & 0x3333);
   a = ((a & 0xF0F0) >> 4) + (a & 0x0F0F);
   a = ((a & 0xFF00) >> 8) + (a & 0x00FF);

   b = ((b & 0xAAAA) >> 1) + (b & 0x5555);
   b = ((b & 0xCCCC) >> 2) + (b & 0x3333);
   b = ((b & 0xF0F0) >> 4) + (b & 0x0F0F);
   b = ((b & 0xFF00) >> 8) + (b & 0x00FF);

   return b - a; /* reversed to get greatest to least sorting */
}

/* Build the opcode handler jump table */
static void build_opcode_table(void)
{
   uint opcode;
   opcode_struct* ostruct;
   uint table_length = 0;

   for(ostruct = g_opcode_handler_table;ostruct->opcode_handler != 0;ostruct++)
      table_length++;

   qsort((void *)g_opcode_handler_table, table_length, sizeof(g_opcode_handler_table[0]), compare_nof_true_bits);

   for(opcode=0;opcode<0x10000;opcode++)
   {
      /* default to illegal */
      g_instruction_jump_table[opcode] = m68000_illegal;
      /* search through opcode handler table for a match */
      for(ostruct = g_opcode_handler_table;ostruct->opcode_handler != 0;ostruct++)
      {
         if((opcode & ostruct->mask) == ostruct->match)
         {
            g_instruction_jump_table[opcode] = ostruct->opcode_handler;
            break;
         }
      }
   }
}



/* Check if the instruction is a valid one */
int m68000_is_valid_instruction(int instruction)
{
   if(g_instruction_jump_table[instruction & 0xffff] == m68000_illegal)
      return 0;
   if(!(g_cpu_type & CPU_010_PLUS))
   {
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_bkpt)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_d)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_ai)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_pi)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_pd)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_di)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_ix)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_aw)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_move_fr_ccr_al)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_movec_cr)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_movec_rc)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_ai_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pi_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pi7_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pd_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pd7_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_di_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_ix_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_aw_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_al_8)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_ai_16)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pi_16)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pd_16)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_di_16)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_ix_16)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_aw_16)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_al_16)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_ai_32)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pi_32)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_pd_32)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_di_32)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_ix_32)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_aw_32)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_moves_al_32)
         return 0;
      if(g_instruction_jump_table[instruction & 0xffff] == m68010_rtd)
         return 0;
   }
   return 1;
}

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
