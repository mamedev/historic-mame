/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */

static const char* copyright_notice =
"DEBABELIZER\n"
"Version 1.0\n"
"A portable Motorola M68000 disassembler.\n"
"Copyright 1999 Karl Stenerud.  All rights reserved.\n"
"\n"
"This code is freeware and may be freely used as long as this copyright\n"
"notice remains unaltered in the source code and any binary files\n"
"containing this code in compiled form.\n"
"\n"
"The latest version of this code can be obtained at:\n"
"http://milliways.scas.bcit.bc.ca/~karl/musashi\n"
;


/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "d68000.h"
#include "d68kconf.h"



/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */
/* ======================================================================== */

/* unsigned int and int must be at least 32 bits wide */
#undef uint
#define uint unsigned int

/* Bit Isolation Functions */
#define BIT_0(A)  ((A) & 0x0001)
#define BIT_1(A)  ((A) & 0x0002)
#define BIT_2(A)  ((A) & 0x0004)
#define BIT_3(A)  ((A) & 0x0008)
#define BIT_4(A)  ((A) & 0x0010)
#define BIT_5(A)  ((A) & 0x0020)
#define BIT_6(A)  ((A) & 0x0040)
#define BIT_7(A)  ((A) & 0x0080)
#define BIT_8(A)  ((A) & 0x0100)
#define BIT_9(A)  ((A) & 0x0200)
#define BIT_A(A)  ((A) & 0x0400)
#define BIT_B(A)  ((A) & 0x0800)
#define BIT_C(A)  ((A) & 0x1000)
#define BIT_D(A)  ((A) & 0x2000)
#define BIT_E(A)  ((A) & 0x4000)
#define BIT_F(A)  ((A) & 0x8000)



/* ======================================================================== */
/* =============================== PROTOTYPES ============================= */
/* ======================================================================== */

uint  read_memory_32 (uint address);
uint  read_memory_16 (uint address);
uint  read_memory_8  (uint address);

/* make signed integers 100% portably */
static int make_int_8(int value);
static int make_int_16(int value);

/* make a string of a hex value */
static char* make_signed_hex_str_8(uint val);
static char* make_signed_hex_str_16(uint val);
static char* make_signed_hex_str_32(uint val);

/* make string of ea mode */
static char* get_ea_mode_str(uint instruction, uint size);

static char* get_ea_mode_str_8(uint instruction);
static char* get_ea_mode_str_16(uint instruction);
static char* get_ea_mode_str_32(uint instruction);

/* make string of immediate value */
static char* get_imm_str_s8(void);
static char* get_imm_str_s16(void);
static char* get_imm_str_s32(void);

/* Stuff to build the opcode handler jump table */
static void  build_opcode_table(void);
static int   valid_ea(uint opcode, uint mask);
static int   compare_nof_true_bits(const void *aptr, const void *bptr);


/* used to build opcode handler jump table */
typedef struct
{
   void (*opcode_handler)(void); /* handler function */
   uint mask;                    /* mask on opcode */
   uint match;                   /* what to match after masking */
   uint ea_mask;                 /* what ea modes are allowed */
} opcode_struct;



/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

/* Opcode handler jump table */
static void (*g_instruction_table[0x10000])(void);
/* Flag if disassembler initialized */
static int  g_initialized = 0;

static char g_dasm_str[100]; /* string to hold disassembly */
static uint g_cpu_pc;        /* program counter */
static uint g_cpu_ir;        /* instruction register */

/* used by ops like asr, ror, addq, etc */
static uint g_3bit_qdata_table[8] = {8, 1, 2, 3, 4, 5, 6, 7};



/* ======================================================================== */
/* =========================== UTILITY FUNCTIONS ========================== */
/* ======================================================================== */

/* Pass up the memory requests */
#define read_memory_32(address) (m68000_read_memory_32((address)&0xffffff)&0xffffffff)
#define read_memory_16(address) (m68000_read_memory_16((address)&0xffffff)&0xffff)
#define read_memory_8(address) (m68000_read_memory_8((address)&0xffffff)&0xff)

/* Fake a split interface */
#define get_ea_mode_str_8(instruction) get_ea_mode_str(instruction, 0)
#define get_ea_mode_str_16(instruction) get_ea_mode_str(instruction, 1)
#define get_ea_mode_str_32(instruction) get_ea_mode_str(instruction, 2)


/* 100% portable signed int generators */
static int make_int_8(int value)
{
   return (value & 0x80) ? value | ~0xff : value & 0xff;
}

static int make_int_16(int value)
{
   return (value & 0x8000) ? value | ~0xffff : value & 0xffff;
}


/* Get string representation of hex values */
static char* make_signed_hex_str_8(uint val)
{
   static char str[20];

   val &= 0xff;

   if(val & 0x80)
      sprintf(str, "-$%x", (0-val) & 0x7f);
   else
      sprintf(str, "$%x", val & 0x7f);

   return str;
}

static char* make_signed_hex_str_16(uint val)
{
   static char str[20];

   val &= 0xffff;

   if(val & 0x8000)
      sprintf(str, "-$%x", (0-val) & 0x7fff);
   else
      sprintf(str, "$%x", val & 0x7fff);

   return str;
}

static char* make_signed_hex_str_32(uint val)
{
   static char str[20];

   val &= 0xffffffff;

   if(val & 0x80000000)
      sprintf(str, "-$%x", (0-val) & 0x7fffffff);
   else
      sprintf(str, "$%x", val & 0x7fffffff);

   return str;
}

/* make string of immediate value */
static char* get_imm_str_s8(void)
{
   static char str[15];
   sprintf(str, "#%s", make_signed_hex_str_8(read_memory_16(g_cpu_pc)));
   g_cpu_pc += 2;
   return str;
}

static char* get_imm_str_u8(void)
{
   static char str[15];
   sprintf(str, "#$%x", read_memory_16(g_cpu_pc) & 0xff);
   g_cpu_pc += 2;
   return str;
}

static char* get_imm_str_s16(void)
{
   static char str[15];
   sprintf(str, "#%s", make_signed_hex_str_16(read_memory_16(g_cpu_pc)));
   g_cpu_pc += 2;
   return str;
}

static char* get_imm_str_u16(void)
{
   static char str[15];
   sprintf(str, "#$%x", read_memory_16(g_cpu_pc) & 0xffff);
   g_cpu_pc += 2;
   return str;
}

static char* get_imm_str_s32(void)
{
   static char str[15];
   sprintf(str, "#%s", make_signed_hex_str_32(read_memory_32(g_cpu_pc)));
   g_cpu_pc += 4;
   return str;
}

static char* get_imm_str_u32(void)
{
   static char str[15];
   uint val = read_memory_32(g_cpu_pc);
   sprintf(str, "#$%x", val & 0xffffffff);
   g_cpu_pc += 4;
   return str;
}

/* Make string of effective address mode */
static char* get_ea_mode_str(uint instruction, uint size)
{
   static char b1[50];
   static char b2[50];
   static char* mode = b2;
   uint extension;

   /* Switch buffers so we don't clobber on a double-call to this function */
   mode = mode == b1 ? b2 : b1;

   switch(instruction & 0x3f)
   {
      case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
      /* data register direct */
         sprintf(mode, "D%d", instruction&7);
         break;
      case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
      /* address register direct */
         sprintf(mode, "A%d", instruction&7);
         break;
      case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
      /* address register indirect */
         sprintf(mode, "(A%d)", instruction&7);
         break;
      case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
      /* address register indirect with postincrement */
         sprintf(mode, "(A%d)+", instruction&7);
         break;
      case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
      /* address register indirect with predecrement */
         sprintf(mode, "-(A%d)", instruction&7);
         break;
      case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
      /* address register indirect with displacement*/
         sprintf(mode, "%s(A%d)", make_signed_hex_str_16(read_memory_16(g_cpu_pc)), instruction&7);
         g_cpu_pc += 2;
         break;
      case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
      /* address register indirect with index */
         extension = read_memory_16(g_cpu_pc);
         g_cpu_pc += 2;
         sprintf(mode, "%s(A%d,%c%d.%c)", (extension & 0xff) == 0 ? "" : make_signed_hex_str_8(extension), instruction&7, BIT_F(extension) ? 'A' : 'D', (extension>>12)&7, BIT_B(extension) ? 'l' : 'w');
         break;
      case 0x38:
      /* absolute short address */
         sprintf(mode, "$%x.w", read_memory_16(g_cpu_pc));
         g_cpu_pc += 2;
         break;
      case 0x39:
      /* absolute long address */
         sprintf(mode, "$%x.l", read_memory_32(g_cpu_pc));
         g_cpu_pc += 4;
         break;
      case 0x3a:
      /* program counter with displacement */
         sprintf(mode, "%s(PC)", make_signed_hex_str_16(read_memory_16(g_cpu_pc)));
         g_cpu_pc += 2;
         break;
      case 0x3b:
      /* program counter with index */
         extension = read_memory_16(g_cpu_pc);
         g_cpu_pc += 2;
         sprintf(mode, "%s(PC,%c%d.%c)", (extension & 0xff) == 0 ? "" : make_signed_hex_str_8(extension), BIT_F(extension) ? 'A' : 'D', (extension>>12)&7, BIT_B(extension) ? 'l' : 'w');
         break;
      case 0x3c:
      /* Immediate */
         if(size == 0)
            sprintf(mode, "%s", get_imm_str_u8());
         else if(size == 1)
            sprintf(mode, "%s", get_imm_str_u16());
         else if(size == 2)
            sprintf(mode, "%s", get_imm_str_u32());
         break;
      default:
         sprintf(mode, "INVALID %x", instruction & 0x3f);
   }
   return mode;
}



/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */

/* Disasemble one instruction at pc and store in str_buff */
int m68000_disassemble(char* str_buff, int pc)
{
   g_cpu_pc = pc;
   g_cpu_ir = read_memory_16(g_cpu_pc);
   g_cpu_pc += 2;

   if(!g_initialized)
   {
      build_opcode_table();
      g_initialized = 1;
   }

   g_instruction_table[g_cpu_ir]();
   strcpy(str_buff, g_dasm_str);
   return g_cpu_pc - pc;
}



/* ======================================================================== */
/* ========================= INSTRUCTION HANDLERS ========================= */
/* ======================================================================== */
/* Instruction handler function names follow this convention:
 *
 * op_NAME_EXTENSIONS(void)
 * where NAME is the name of the opcode it handles and EXTENSIONS are any
 * extensions for special instances of that opcode.
 *
 * Examples:
 *   op_add_er_8(): add opcode, from effective address to register,
 *                      size = byte
 *
 *   op_asr_s_8(): arithmetic shift right, static count, size = byte
 *
 *
 * Common extensions:
 * 8   : size = byte
 * 16  : size = word
 * 32  : size = long
 * rr  : register to register
 * mm  : memory to memory
 * r   : register
 * s   : static
 * er  : effective address -> register
 * re  : register -> effective address
 * ea  : using effective address mode of operation
 * d   : data register direct
 * a   : address register direct
 * ai  : address register indirect
 * pi  : address register indirect with postincrement
 * pd  : address register indirect with predecrement
 * di  : address register indirect with displacement
 * ix  : address register indirect with index
 * aw  : absolute word
 * al  : absolute long
 */

static void op_1010(void)
{
   sprintf(g_dasm_str, "dc.w $%04x; opcode 1010", g_cpu_ir);
}


static void op_1111(void)
{
   sprintf(g_dasm_str, "dc.w $%04x; opcode 1111", g_cpu_ir);
}


static void op_abcd_rr(void)
{
   sprintf(g_dasm_str, "abcd    D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}


static void op_abcd_mm(void)
{
   sprintf(g_dasm_str, "abcd    -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_add_er_8(void)
{
   sprintf(g_dasm_str, "add.b   %s, D%d", get_ea_mode_str_8(g_cpu_ir), (g_cpu_ir>>9)&7);
}


static void op_add_er_16(void)
{
   sprintf(g_dasm_str, "add.w   %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_add_er_32(void)
{
   sprintf(g_dasm_str, "add.l   %s, D%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_add_re_8(void)
{
   sprintf(g_dasm_str, "add.b   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_add_re_16(void)
{
   sprintf(g_dasm_str, "add.w   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_16(g_cpu_ir));
}

static void op_add_re_32(void)
{
   sprintf(g_dasm_str, "add.l   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_32(g_cpu_ir));
}

static void op_adda_16(void)
{
   sprintf(g_dasm_str, "adda.w  %s, A%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_adda_32(void)
{
   sprintf(g_dasm_str, "adda.l  %s, A%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_addi_8(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "addi.b  %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_addi_16(void)
{
   char* str = get_imm_str_s16();
   sprintf(g_dasm_str, "addi.w  %s, %s", str, get_ea_mode_str_16(g_cpu_ir));
}

static void op_addi_32(void)
{
   char* str = get_imm_str_s32();
   sprintf(g_dasm_str, "addi.l  %s, %s", str, get_ea_mode_str_32(g_cpu_ir));
}

static void op_addq_8(void)
{
   sprintf(g_dasm_str, "addq.b  #%d, %s", g_3bit_qdata_table[(g_cpu_ir>>9)&7], get_ea_mode_str_8(g_cpu_ir));
}

static void op_addq_16(void)
{
   sprintf(g_dasm_str, "addq.w  #%d, %s", g_3bit_qdata_table[(g_cpu_ir>>9)&7], get_ea_mode_str_16(g_cpu_ir));
}

static void op_addq_32(void)
{
   sprintf(g_dasm_str, "addq.l  #%d, %s", g_3bit_qdata_table[(g_cpu_ir>>9)&7], get_ea_mode_str_32(g_cpu_ir));
}

static void op_addx_rr_8(void)
{
   sprintf(g_dasm_str, "addx.b  D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_addx_rr_16(void)
{
   sprintf(g_dasm_str, "addx.w  D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_addx_rr_32(void)
{
   sprintf(g_dasm_str, "addx.l  D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_addx_mm_8(void)
{
   sprintf(g_dasm_str, "addx.b  -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_addx_mm_16(void)
{
   sprintf(g_dasm_str, "addx.w  -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_addx_mm_32(void)
{
   sprintf(g_dasm_str, "addx.l  -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_and_er_8(void)
{
   sprintf(g_dasm_str, "and.b   %s, D%d", get_ea_mode_str_8(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_and_er_16(void)
{
   sprintf(g_dasm_str, "and.w   %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_and_er_32(void)
{
   sprintf(g_dasm_str, "and.l   %s, D%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_and_re_8(void)
{
   sprintf(g_dasm_str, "and.b   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_and_re_16(void)
{
   sprintf(g_dasm_str, "and.w   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_16(g_cpu_ir));
}

static void op_and_re_32(void)
{
   sprintf(g_dasm_str, "and.l   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_32(g_cpu_ir));
}

static void op_andi_8(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "andi.b  %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_andi_16(void)
{
   char* str = get_imm_str_s16();
   sprintf(g_dasm_str, "andi.w  %s, %s", str, get_ea_mode_str_16(g_cpu_ir));
}

static void op_andi_32(void)
{
   char* str = get_imm_str_s32();
   sprintf(g_dasm_str, "andi.l  %s, %s", str, get_ea_mode_str_32(g_cpu_ir));
}

static void op_andi_to_ccr(void)
{
   sprintf(g_dasm_str, "andi    %s, CCR", get_imm_str_s8());
}

static void op_andi_to_sr(void)
{
   sprintf(g_dasm_str, "andi    %s, SR", get_imm_str_s16());
}

static void op_asr_s_8(void)
{
   sprintf(g_dasm_str, "asr.b   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_asr_s_16(void)
{
   sprintf(g_dasm_str, "asr.w   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_asr_s_32(void)
{
   sprintf(g_dasm_str, "asr.l   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_asr_r_8(void)
{
   sprintf(g_dasm_str, "asr.b   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_asr_r_16(void)
{
   sprintf(g_dasm_str, "asr.w   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_asr_r_32(void)
{
   sprintf(g_dasm_str, "asr.l   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_asr_ea(void)
{
   sprintf(g_dasm_str, "asr.w   %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_asl_s_8(void)
{
   sprintf(g_dasm_str, "asl.b   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_asl_s_16(void)
{
   sprintf(g_dasm_str, "asl.w   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_asl_s_32(void)
{
   sprintf(g_dasm_str, "asl.l   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_asl_r_8(void)
{
   sprintf(g_dasm_str, "asl.b   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_asl_r_16(void)
{
   sprintf(g_dasm_str, "asl.w   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_asl_r_32(void)
{
   sprintf(g_dasm_str, "asl.l   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_asl_ea(void)
{
   sprintf(g_dasm_str, "asl.w   %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_bhi_8(void)
{
   sprintf(g_dasm_str, "bhi     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bhi_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bhi     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bls_8(void)
{
   sprintf(g_dasm_str, "bls     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bls_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bls     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bcc_8(void)
{
   sprintf(g_dasm_str, "bcc     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bcc_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bcc     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bcs_8(void)
{
   sprintf(g_dasm_str, "bcs     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bcs_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bcs     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bne_8(void)
{
   sprintf(g_dasm_str, "bne     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bne_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bne     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_beq_8(void)
{
   sprintf(g_dasm_str, "beq     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_beq_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "beq     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bvc_8(void)
{
   sprintf(g_dasm_str, "bvc     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bvc_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bvc     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bvs_8(void)
{
   sprintf(g_dasm_str, "bvs     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bvs_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bvs     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bpl_8(void)
{
   sprintf(g_dasm_str, "bpl     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bpl_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bpl     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bmi_8(void)
{
   sprintf(g_dasm_str, "bmi     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bmi_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bmi     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bge_8(void)
{
   sprintf(g_dasm_str, "bge     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bge_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bge     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_blt_8(void)
{
   sprintf(g_dasm_str, "blt     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_blt_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "blt     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bgt_8(void)
{
   sprintf(g_dasm_str, "bgt     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bgt_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bgt     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_ble_8(void)
{
   sprintf(g_dasm_str, "ble     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_ble_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "ble     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bchg_r(void)
{
   sprintf(g_dasm_str, "bchg    D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_bchg_s(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "bchg    %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_bclr_r(void)
{
   sprintf(g_dasm_str, "bclr    D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_bclr_s(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "bclr    %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_bra_8(void)
{
   sprintf(g_dasm_str, "bra     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bra_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bra     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_bset_r(void)
{
   sprintf(g_dasm_str, "bset    D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_bset_s(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "bset    %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_bsr_8(void)
{
   sprintf(g_dasm_str, "bsr     #%s; %x", make_signed_hex_str_8(g_cpu_ir), g_cpu_pc + make_int_8(g_cpu_ir));
}

static void op_bsr_16(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "bsr     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_btst_r(void)
{
   sprintf(g_dasm_str, "btst    D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_btst_s(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "btst    %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_chk(void)
{
   sprintf(g_dasm_str, "chk     %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_clr_8(void)
{
   sprintf(g_dasm_str, "clr.b   %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_clr_16(void)
{
   sprintf(g_dasm_str, "clr.w   %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_clr_32(void)
{
   sprintf(g_dasm_str, "clr.l   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_cmp_8(void)
{
   sprintf(g_dasm_str, "cmp.b   %s, D%d", get_ea_mode_str_8(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_cmp_16(void)
{
   sprintf(g_dasm_str, "cmp.w   %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_cmp_32(void)
{
   sprintf(g_dasm_str, "cmp.l   %s, D%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_cmpa_16(void)
{
   sprintf(g_dasm_str, "cmpa.w  %s, A%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_cmpa_32(void)
{
   sprintf(g_dasm_str, "cmpa.l  %s, A%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_cmpi_8(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "cmpi.b  %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_cmpi_16(void)
{
   char* str = get_imm_str_s16();
   sprintf(g_dasm_str, "cmpi.w  %s, %s", str, get_ea_mode_str_16(g_cpu_ir));
}

static void op_cmpi_32(void)
{
   char* str = get_imm_str_s32();
   sprintf(g_dasm_str, "cmpi.l  %s, %s", str, get_ea_mode_str_32(g_cpu_ir));
}

static void op_cmpm_8(void)
{
   sprintf(g_dasm_str, "cmpm.b  (A%d)+, (A%d)+", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_cmpm_16(void)
{
   sprintf(g_dasm_str, "cmpm.w  (A%d)+, (A%d)+", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_cmpm_32(void)
{
   sprintf(g_dasm_str, "cmpm.l  (A%d)+, (A%d)+", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_dbt(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbt     %s; %x", get_imm_str_s16(), new_pc);
}

static void op_dbf(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbra    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbhi(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbhi    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbls(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbls    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbcc(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbcc    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbcs(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbcs    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbne(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbne    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbeq(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbeq    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbvc(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbvc    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbvs(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbvs    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbpl(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbpl    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbmi(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbmi    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbge(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbge    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dblt(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dblt    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dbgt(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dbgt    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_dble(void)
{
   uint new_pc = g_cpu_pc + make_int_16(read_memory_16(g_cpu_pc));
   sprintf(g_dasm_str, "dble    D%d, %s; %x", g_cpu_ir & 7, get_imm_str_s16(), new_pc);
}

static void op_divs(void)
{
   sprintf(g_dasm_str, "divs    %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_divu(void)
{
   sprintf(g_dasm_str, "divu    %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_eor_8(void)
{
   sprintf(g_dasm_str, "eor.b   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_eor_16(void)
{
   sprintf(g_dasm_str, "eor.w   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_16(g_cpu_ir));
}

static void op_eor_32(void)
{
   sprintf(g_dasm_str, "eor.l   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_32(g_cpu_ir));
}

static void op_eori_8(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "eori.b  %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_eori_16(void)
{
   char* str = get_imm_str_s16();
   sprintf(g_dasm_str, "eori.w  %s, %s", str, get_ea_mode_str_16(g_cpu_ir));
}

static void op_eori_32(void)
{
   char* str = get_imm_str_s32();
   sprintf(g_dasm_str, "eori.l  %s, %s", str, get_ea_mode_str_32(g_cpu_ir));
}

static void op_eori_to_ccr(void)
{
   sprintf(g_dasm_str, "eori    %s, CCR", get_imm_str_s8());
}

static void op_eori_to_sr(void)
{
   sprintf(g_dasm_str, "eori    %s, SR", get_imm_str_s16());
}

static void op_exg_dd(void)
{
   sprintf(g_dasm_str, "exg     D%d, D%d", ((g_cpu_ir>>9)&7)+1, g_cpu_ir&7);
}

static void op_exg_aa(void)
{
   sprintf(g_dasm_str, "exg     A%d, A%d", ((g_cpu_ir>>9)&7)+1, g_cpu_ir&7);
}

static void op_exg_da(void)
{
   sprintf(g_dasm_str, "exg     D%d, A%d", ((g_cpu_ir>>9)&7)+1, g_cpu_ir&7);
}

static void op_ext_16(void)
{
   sprintf(g_dasm_str, "ext.w   D%d", g_cpu_ir&7);
}

static void op_ext_32(void)
{
   sprintf(g_dasm_str, "ext.l   D%d", g_cpu_ir&7);
}

static void op_jmp(void)
{
   sprintf(g_dasm_str, "jmp     %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_jsr(void)
{
   sprintf(g_dasm_str, "jsr     %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_lea(void)
{
   sprintf(g_dasm_str, "lea     %s, A%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_link(void)
{
   sprintf(g_dasm_str, "link    A%d, %s", g_cpu_ir&7, get_imm_str_s16());
}

static void op_lsr_s_8(void)
{
   sprintf(g_dasm_str, "lsr.b   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_lsr_s_16(void)
{
   sprintf(g_dasm_str, "lsr.w   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_lsr_s_32(void)
{
   sprintf(g_dasm_str, "lsr.l   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_lsr_r_8(void)
{
   sprintf(g_dasm_str, "lsr.b   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_lsr_r_16(void)
{
   sprintf(g_dasm_str, "lsr.w   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_lsr_r_32(void)
{
   sprintf(g_dasm_str, "lsr.l   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_lsr_ea(void)
{
   sprintf(g_dasm_str, "lsr.w   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_lsl_s_8(void)
{
   sprintf(g_dasm_str, "lsl.b   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_lsl_s_16(void)
{
   sprintf(g_dasm_str, "lsl.w   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_lsl_s_32(void)
{
   sprintf(g_dasm_str, "lsl.l   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_lsl_r_8(void)
{
   sprintf(g_dasm_str, "lsl.b   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_lsl_r_16(void)
{
   sprintf(g_dasm_str, "lsl.w   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_lsl_r_32(void)
{
   sprintf(g_dasm_str, "lsl.l   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_lsl_ea(void)
{
   sprintf(g_dasm_str, "lsl.w   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_move_dd_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_ai_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_pi_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_pd_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_di_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_ix_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_aw_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_al_8(void)
{
   char* str = get_ea_mode_str_8(g_cpu_ir);
   sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_dd_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_ai_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_pi_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_pd_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_di_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_ix_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_aw_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_al_16(void)
{
   char* str = get_ea_mode_str_16(g_cpu_ir);
   sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_dd_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_ai_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_pi_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_pd_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_di_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_ix_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_aw_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_move_al_32(void)
{
   char* str = get_ea_mode_str_32(g_cpu_ir);
   sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static void op_movea_16(void)
{
   sprintf(g_dasm_str, "movea.w %s, A%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_movea_32(void)
{
   sprintf(g_dasm_str, "movea.l %s, A%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_move_to_ccr(void)
{
   sprintf(g_dasm_str, "move    %s, CCR", get_ea_mode_str_8(g_cpu_ir));
}

static void op_move_fr_sr(void)
{
   sprintf(g_dasm_str, "move    SR, %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_move_to_sr(void)
{
   sprintf(g_dasm_str, "move    %s, SR", get_ea_mode_str_16(g_cpu_ir));
}

static void op_move_fr_usp(void)
{
   sprintf(g_dasm_str, "move    USP, A%d", g_cpu_ir&7);
}

static void op_move_to_usp(void)
{
   sprintf(g_dasm_str, "move    A%d, USP", g_cpu_ir&7);
}

static void op_movem_pd_16(void)
{
   uint data = read_memory_16(g_cpu_pc);
   char buffer[40];
   uint first;
   uint run_length;
   uint i;

   g_cpu_pc += 2;

   buffer[0] = 0;
   for(i=0;i<8;i++)
   {
      if(data&(1<<(15-i)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(15-i)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "D%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-D%d", first + run_length);
      }
   }
   for(i=0;i<8;i++)
   {
      if(data&(1<<(7-i)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(7-i)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "A%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-A%d", first + run_length);
      }
   }
   sprintf(g_dasm_str, "movem.w %s, %s", buffer, get_ea_mode_str_16(g_cpu_ir));
}

static void op_movem_pd_32(void)
{
   uint data = read_memory_16(g_cpu_pc);
   char buffer[40];
   uint first;
   uint run_length;
   uint i;

   g_cpu_pc += 2;

   buffer[0] = 0;
   for(i=0;i<8;i++)
   {
      if(data&(1<<(15-i)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(15-i)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "D%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-D%d", first + run_length);
      }
   }
   for(i=0;i<8;i++)
   {
      if(data&(1<<(7-i)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(7-i)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "A%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-A%d", first + run_length);
      }
   }
   sprintf(g_dasm_str, "movem.l %s, %s", buffer, get_ea_mode_str_32(g_cpu_ir));
}

static void op_movem_er_16(void)
{
   uint data = read_memory_16(g_cpu_pc);
   char buffer[40];
   uint first;
   uint run_length;
   uint i;

   g_cpu_pc += 2;

   buffer[0] = 0;
   for(i=0;i<8;i++)
   {
      if(data&(1<<i))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<i))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "D%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-D%d", first + run_length);
      }
   }
   for(i=0;i<8;i++)
   {
      if(data&(1<<(i+8)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(i+8)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "A%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-A%d", first + run_length);
      }
   }
   sprintf(g_dasm_str, "movem.w %s, %s", get_ea_mode_str_16(g_cpu_ir), buffer);
}

static void op_movem_er_32(void)
{
   uint data = read_memory_16(g_cpu_pc);
   char buffer[40];
   uint first;
   uint run_length;
   uint i;

   g_cpu_pc += 2;

   buffer[0] = 0;
   for(i=0;i<8;i++)
   {
      if(data&(1<<i))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<i))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "D%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-D%d", first + run_length);
      }
   }
   for(i=0;i<8;i++)
   {
      if(data&(1<<(i+8)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(i+8)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "A%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-A%d", first + run_length);
      }
   }
   sprintf(g_dasm_str, "movem.l %s, %s", get_ea_mode_str_32(g_cpu_ir), buffer);
}

static void op_movem_re_16(void)
{
   uint data = read_memory_16(g_cpu_pc);
   char buffer[40];
   uint first;
   uint run_length;
   uint i;

   g_cpu_pc += 2;

   buffer[0] = 0;
   for(i=0;i<8;i++)
   {
      if(data&(1<<i))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<i))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "D%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-D%d", first + run_length);
      }
   }
   for(i=0;i<8;i++)
   {
      if(data&(1<<(i+8)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(i+8)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "A%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-A%d", first + run_length);
      }
   }
   sprintf(g_dasm_str, "movem.w %s, %s", buffer, get_ea_mode_str_16(g_cpu_ir));
}

static void op_movem_re_32(void)
{
   uint data = read_memory_16(g_cpu_pc);
   char buffer[40];
   uint first;
   uint run_length;
   uint i;

   g_cpu_pc += 2;

   buffer[0] = 0;
   for(i=0;i<8;i++)
   {
      if(data&(1<<i))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<i))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "D%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-D%d", first + run_length);
      }
   }
   for(i=0;i<8;i++)
   {
      if(data&(1<<(i+8)))
      {
         first = i;
         run_length = 0;
         for(i++;i<8;i++)
            if(data&(1<<(i+8)))
               run_length++;
         if(buffer[0] != 0)
            strcat(buffer, "/");
         sprintf(buffer+strlen(buffer), "A%d", first);
         if(run_length > 0)
            sprintf(buffer+strlen(buffer), "-A%d", first + run_length);
      }
   }
   sprintf(g_dasm_str, "movem.l %s, %s", buffer, get_ea_mode_str_32(g_cpu_ir));
}

static void op_movep_re_16(void)
{
   sprintf(g_dasm_str, "movep.w D%d, ($%x,A%d)", (g_cpu_ir>>9)&7, read_memory_16(g_cpu_pc), g_cpu_ir&7);
   g_cpu_pc += 2;
}

static void op_movep_re_32(void)
{
   sprintf(g_dasm_str, "movep.l D%d, ($%x,A%d)", (g_cpu_ir>>9)&7, read_memory_16(g_cpu_pc), g_cpu_ir&7);
   g_cpu_pc += 2;
}

static void op_movep_er_16(void)
{
   sprintf(g_dasm_str, "movep.w ($%x,A%d), D%d", read_memory_16(g_cpu_pc), g_cpu_ir&7, (g_cpu_ir>>9)&7);
   g_cpu_pc += 2;
}

static void op_movep_er_32(void)
{
   sprintf(g_dasm_str, "movep.l ($%x,A%d), D%d", read_memory_16(g_cpu_pc), g_cpu_ir&7, (g_cpu_ir>>9)&7);
   g_cpu_pc += 2;
}

static void op_moveq(void)
{
   sprintf(g_dasm_str, "moveq   #%s, D%d", make_signed_hex_str_8(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_muls(void)
{
   sprintf(g_dasm_str, "muls    %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_mulu(void)
{
   sprintf(g_dasm_str, "mulu    %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_nbcd(void)
{
   sprintf(g_dasm_str, "nbcd    %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_neg_8(void)
{
   sprintf(g_dasm_str, "neg.b   %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_neg_16(void)
{
   sprintf(g_dasm_str, "neg.w   %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_neg_32(void)
{
   sprintf(g_dasm_str, "neg.l   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_negx_8(void)
{
   sprintf(g_dasm_str, "negx.b  %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_negx_16(void)
{
   sprintf(g_dasm_str, "negx.w  %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_negx_32(void)
{
   sprintf(g_dasm_str, "negx.l  %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_nop(void)
{
   sprintf(g_dasm_str, "nop");
}

static void op_not_8(void)
{
   sprintf(g_dasm_str, "not.b   %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_not_16(void)
{
   sprintf(g_dasm_str, "not.w   %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_not_32(void)
{
   sprintf(g_dasm_str, "not.l   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_or_er_8(void)
{
   sprintf(g_dasm_str, "or.b    %s, D%d", get_ea_mode_str_8(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_or_er_16(void)
{
   sprintf(g_dasm_str, "or.w    %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_or_er_32(void)
{
   sprintf(g_dasm_str, "or.l    %s, D%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_or_re_8(void)
{
   sprintf(g_dasm_str, "or.b    D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_or_re_16(void)
{
   sprintf(g_dasm_str, "or.w    D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_16(g_cpu_ir));
}

static void op_or_re_32(void)
{
   sprintf(g_dasm_str, "or.l    D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_32(g_cpu_ir));
}

static void op_ori_8(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "ori.b   %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_ori_16(void)
{
   char* str = get_imm_str_s16();
   sprintf(g_dasm_str, "ori.w   %s, %s", str, get_ea_mode_str_16(g_cpu_ir));
}

static void op_ori_32(void)
{
   char* str = get_imm_str_s32();
   sprintf(g_dasm_str, "ori.l   %s, %s", str, get_ea_mode_str_32(g_cpu_ir));
}

static void op_ori_to_ccr(void)
{
   sprintf(g_dasm_str, "ori     %s, CCR", get_imm_str_s8());
}

static void op_ori_to_sr(void)
{
   sprintf(g_dasm_str, "ori     %s, SR", get_imm_str_s16());
}

static void op_pea(void)
{
   sprintf(g_dasm_str, "pea     %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_reset(void)
{
   sprintf(g_dasm_str, "reset");
}

static void op_ror_s_8(void)
{
   sprintf(g_dasm_str, "ror.b   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_ror_s_16(void)
{
   sprintf(g_dasm_str, "ror.w   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7],g_cpu_ir&7);
}

static void op_ror_s_32(void)
{
   sprintf(g_dasm_str, "ror.l   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_ror_r_8(void)
{
   sprintf(g_dasm_str, "ror.b   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_ror_r_16(void)
{
   sprintf(g_dasm_str, "ror.w   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_ror_r_32(void)
{
   sprintf(g_dasm_str, "ror.l   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_ror_ea(void)
{
   sprintf(g_dasm_str, "ror.w   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_rol_s_8(void)
{
   sprintf(g_dasm_str, "rol.b   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_rol_s_16(void)
{
   sprintf(g_dasm_str, "rol.w   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_rol_s_32(void)
{
   sprintf(g_dasm_str, "rol.l   #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_rol_r_8(void)
{
   sprintf(g_dasm_str, "rol.b   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_rol_r_16(void)
{
   sprintf(g_dasm_str, "rol.w   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_rol_r_32(void)
{
   sprintf(g_dasm_str, "rol.l   D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_rol_ea(void)
{
   sprintf(g_dasm_str, "rol.w   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_roxr_s_8(void)
{
   sprintf(g_dasm_str, "roxr.b  #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_roxr_s_16(void)
{
   sprintf(g_dasm_str, "roxr.w  #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}


static void op_roxr_s_32(void)
{
   sprintf(g_dasm_str, "roxr.l  #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_roxr_r_8(void)
{
   sprintf(g_dasm_str, "roxr.b  D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_roxr_r_16(void)
{
   sprintf(g_dasm_str, "roxr.w  D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_roxr_r_32(void)
{
   sprintf(g_dasm_str, "roxr.l  D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_roxr_ea(void)
{
   sprintf(g_dasm_str, "roxr.w  %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_roxl_s_8(void)
{
   sprintf(g_dasm_str, "roxl.b  #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_roxl_s_16(void)
{
   sprintf(g_dasm_str, "roxl.w  #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_roxl_s_32(void)
{
   sprintf(g_dasm_str, "roxl.l  #%d, D%d", g_3bit_qdata_table[(g_cpu_ir>>9)&7], g_cpu_ir&7);
}

static void op_roxl_r_8(void)
{
   sprintf(g_dasm_str, "roxl.b  D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_roxl_r_16(void)
{
   sprintf(g_dasm_str, "roxl.w  D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_roxl_r_32(void)
{
   sprintf(g_dasm_str, "roxl.l  D%d, D%d", (g_cpu_ir>>9)&7, g_cpu_ir&7);
}

static void op_roxl_ea(void)
{
   sprintf(g_dasm_str, "roxl.w  %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_rte(void)
{
   sprintf(g_dasm_str, "rte");
}

static void op_rtr(void)
{
   sprintf(g_dasm_str, "rtr");
}

static void op_rts(void)
{
   sprintf(g_dasm_str, "rts");
}

static void op_sbcd_rr(void)
{
   sprintf(g_dasm_str, "sbcd    D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_sbcd_mm(void)
{
   sprintf(g_dasm_str, "sbcd    -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_st(void)
{
   sprintf(g_dasm_str, "st      %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_sf(void)
{
   sprintf(g_dasm_str, "sf      %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_shi(void)
{
   sprintf(g_dasm_str, "shi     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_sls(void)
{
   sprintf(g_dasm_str, "sls     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_scc(void)
{
   sprintf(g_dasm_str, "scc     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_scs(void)
{
   sprintf(g_dasm_str, "scs     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_sne(void)
{
   sprintf(g_dasm_str, "sne     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_seq(void)
{
   sprintf(g_dasm_str, "seq     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_svc(void)
{
   sprintf(g_dasm_str, "svc     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_svs(void)
{
   sprintf(g_dasm_str, "svs     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_spl(void)
{
   sprintf(g_dasm_str, "spl     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_smi(void)
{
   sprintf(g_dasm_str, "smi     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_sge(void)
{
   sprintf(g_dasm_str, "sge     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_slt(void)
{
   sprintf(g_dasm_str, "slt     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_sgt(void)
{
   sprintf(g_dasm_str, "sgt     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_sle(void)
{
   sprintf(g_dasm_str, "sle     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_stop(void)
{
   sprintf(g_dasm_str, "stop");
}

static void op_sub_er_8(void)
{
   sprintf(g_dasm_str, "sub.b   %s, D%d", get_ea_mode_str_8(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_sub_er_16(void)
{
   sprintf(g_dasm_str, "sub.w   %s, D%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_sub_er_32(void)
{
   sprintf(g_dasm_str, "sub.l   %s, D%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_sub_re_8(void)
{
   sprintf(g_dasm_str, "sub.b   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_8(g_cpu_ir));
}

static void op_sub_re_16(void)
{
   sprintf(g_dasm_str, "sub.w   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_16(g_cpu_ir));
}

static void op_sub_re_32(void)
{
   sprintf(g_dasm_str, "sub.l   D%d, %s", (g_cpu_ir>>9)&7, get_ea_mode_str_32(g_cpu_ir));
}

static void op_suba_16(void)
{
   sprintf(g_dasm_str, "suba.w  %s, A%d", get_ea_mode_str_16(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_suba_32(void)
{
   sprintf(g_dasm_str, "suba.l  %s, A%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static void op_subi_8(void)
{
   char* str = get_imm_str_s8();
   sprintf(g_dasm_str, "subi.b  %s, %s", str, get_ea_mode_str_8(g_cpu_ir));
}

static void op_subi_16(void)
{
   char* str = get_imm_str_s16();
   sprintf(g_dasm_str, "subi.w  %s, %s", str, get_ea_mode_str_16(g_cpu_ir));
}

static void op_subi_32(void)
{
   char* str = get_imm_str_s32();
   sprintf(g_dasm_str, "subi.l  %s, %s", str, get_ea_mode_str_32(g_cpu_ir));
}

static void op_subq_8(void)
{
   sprintf(g_dasm_str, "subq.b  #%d, %s", g_3bit_qdata_table[(g_cpu_ir>>9)&7], get_ea_mode_str_8(g_cpu_ir));
}

static void op_subq_16(void)
{
   sprintf(g_dasm_str, "subq.w  #%d, %s", g_3bit_qdata_table[(g_cpu_ir>>9)&7], get_ea_mode_str_16(g_cpu_ir));
}

static void op_subq_32(void)
{
   sprintf(g_dasm_str, "subq.l  #%d, %s", g_3bit_qdata_table[(g_cpu_ir>>9)&7], get_ea_mode_str_32(g_cpu_ir));
}

static void op_subx_rr_8(void)
{
   sprintf(g_dasm_str, "subx.b  D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_subx_rr_16(void)
{
   sprintf(g_dasm_str, "subx.w  D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_subx_rr_32(void)
{
   sprintf(g_dasm_str, "subx.l  D%d, D%d", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_subx_mm_8(void)
{
   sprintf(g_dasm_str, "subx.b  -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_subx_mm_16(void)
{
   sprintf(g_dasm_str, "subx.w  -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_subx_mm_32(void)
{
   sprintf(g_dasm_str, "subx.l  -(A%d), -(A%d)", g_cpu_ir&7, (g_cpu_ir>>9)&7);
}

static void op_swap(void)
{
   sprintf(g_dasm_str, "swap    D%d", g_cpu_ir&7);
}

static void op_tas(void)
{
   sprintf(g_dasm_str, "tas     %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_trap(void)
{
   sprintf(g_dasm_str, "trap    #$%x", g_cpu_ir&0xf);
}

static void op_trapv(void)
{
   sprintf(g_dasm_str, "trapv");
}

static void op_tst_8(void)
{
   sprintf(g_dasm_str, "tst.b   %s", get_ea_mode_str_8(g_cpu_ir));
}

static void op_tst_16(void)
{
   sprintf(g_dasm_str, "tst.w   %s", get_ea_mode_str_16(g_cpu_ir));
}

static void op_tst_32(void)
{
   sprintf(g_dasm_str, "tst.l   %s", get_ea_mode_str_32(g_cpu_ir));
}

static void op_unlk(void)
{
   sprintf(g_dasm_str, "unlk    A%d", g_cpu_ir&7);
}


static void op_illegal(void)
{
   sprintf(g_dasm_str, "dc.w $%04x; ILLEGAL", g_cpu_ir);
}

static void op_invalid_ea(void)
{
   sprintf(g_dasm_str, "dc.w $%04x; INVALID EA", g_cpu_ir);
}



/* ======================================================================== */
/* ======================= INSTRUCTION TABLE BUILDER ====================== */
/* ======================================================================== */

/* EA Masks:
800 = data register direct
400 = address register direct
200 = address register indirect
100 = ARI postincrement
 80 = ARI pre-decrement
 40 = ARI displacement
 20 = ARI index
 10 = absolute short
  8 = absolute long
  4 = immediate / sr
  2 = pc displacement
  1 = pc idx
*/

static opcode_struct g_opcode_info[] =
{
/*  opcode handler    mask    match   ea mask */
   {op_1010         , 0xf000, 0xa000, 0x000},
   {op_1111         , 0xf000, 0xf000, 0x000},
   {op_abcd_rr      , 0xf1f8, 0xc100, 0x000},
   {op_abcd_mm      , 0xf1f8, 0xc108, 0x000},
   {op_add_er_8     , 0xf1c0, 0xd000, 0xbff},
   {op_add_er_16    , 0xf1c0, 0xd040, 0xfff},
   {op_add_er_32    , 0xf1c0, 0xd080, 0xfff},
   {op_add_re_8     , 0xf1c0, 0xd100, 0x3f8},
   {op_add_re_16    , 0xf1c0, 0xd140, 0x3f8},
   {op_add_re_32    , 0xf1c0, 0xd180, 0x3f8},
   {op_adda_16      , 0xf1c0, 0xd0c0, 0xfff},
   {op_adda_32      , 0xf1c0, 0xd1c0, 0xfff},
   {op_addi_8       , 0xffc0, 0x0600, 0xbf8},
   {op_addi_16      , 0xffc0, 0x0640, 0xbf8},
   {op_addi_32      , 0xffc0, 0x0680, 0xbf8},
   {op_addq_8       , 0xf1c0, 0x5000, 0xbf8},
   {op_addq_16      , 0xf1c0, 0x5040, 0xff8},
   {op_addq_32      , 0xf1c0, 0x5080, 0xff8},
   {op_addx_rr_8    , 0xf1f8, 0xd100, 0x000},
   {op_addx_rr_16   , 0xf1f8, 0xd140, 0x000},
   {op_addx_rr_32   , 0xf1f8, 0xd180, 0x000},
   {op_addx_mm_8    , 0xf1f8, 0xd108, 0x000},
   {op_addx_mm_16   , 0xf1f8, 0xd148, 0x000},
   {op_addx_mm_32   , 0xf1f8, 0xd188, 0x000},
   {op_and_er_8     , 0xf1c0, 0xc000, 0xbff},
   {op_and_er_16    , 0xf1c0, 0xc040, 0xbff},
   {op_and_er_32    , 0xf1c0, 0xc080, 0xbff},
   {op_and_re_8     , 0xf1c0, 0xc100, 0x3f8},
   {op_and_re_16    , 0xf1c0, 0xc140, 0x3f8},
   {op_and_re_32    , 0xf1c0, 0xc180, 0x3f8},
   {op_andi_to_ccr  , 0xffff, 0x023c, 0x000},
   {op_andi_to_sr   , 0xffff, 0x027c, 0x000},
   {op_andi_8       , 0xffc0, 0x0200, 0xbf8},
   {op_andi_16      , 0xffc0, 0x0240, 0xbf8},
   {op_andi_32      , 0xffc0, 0x0280, 0xbf8},
   {op_asr_s_8      , 0xf1f8, 0xe000, 0x000},
   {op_asr_s_16     , 0xf1f8, 0xe040, 0x000},
   {op_asr_s_32     , 0xf1f8, 0xe080, 0x000},
   {op_asr_r_8      , 0xf1f8, 0xe020, 0x000},
   {op_asr_r_16     , 0xf1f8, 0xe060, 0x000},
   {op_asr_r_32     , 0xf1f8, 0xe0a0, 0x000},
   {op_asr_ea       , 0xffc0, 0xe0c0, 0x3f8},
   {op_asl_s_8      , 0xf1f8, 0xe100, 0x000},
   {op_asl_s_16     , 0xf1f8, 0xe140, 0x000},
   {op_asl_s_32     , 0xf1f8, 0xe180, 0x000},
   {op_asl_r_8      , 0xf1f8, 0xe120, 0x000},
   {op_asl_r_16     , 0xf1f8, 0xe160, 0x000},
   {op_asl_r_32     , 0xf1f8, 0xe1a0, 0x000},
   {op_asl_ea       , 0xffc0, 0xe1c0, 0x3f8},
   {op_bhi_16       , 0xffff, 0x6200, 0x000},
   {op_bhi_8        , 0xff00, 0x6200, 0x000},
   {op_bls_16       , 0xffff, 0x6300, 0x000},
   {op_bls_8        , 0xff00, 0x6300, 0x000},
   {op_bcc_16       , 0xffff, 0x6400, 0x000},
   {op_bcc_8        , 0xff00, 0x6400, 0x000},
   {op_bcs_16       , 0xffff, 0x6500, 0x000},
   {op_bcs_8        , 0xff00, 0x6500, 0x000},
   {op_bne_16       , 0xffff, 0x6600, 0x000},
   {op_bne_8        , 0xff00, 0x6600, 0x000},
   {op_beq_16       , 0xffff, 0x6700, 0x000},
   {op_beq_8        , 0xff00, 0x6700, 0x000},
   {op_bvc_16       , 0xffff, 0x6800, 0x000},
   {op_bvc_8        , 0xff00, 0x6800, 0x000},
   {op_bvs_16       , 0xffff, 0x6900, 0x000},
   {op_bvs_8        , 0xff00, 0x6900, 0x000},
   {op_bpl_16       , 0xffff, 0x6a00, 0x000},
   {op_bpl_8        , 0xff00, 0x6a00, 0x000},
   {op_bmi_16       , 0xffff, 0x6b00, 0x000},
   {op_bmi_8        , 0xff00, 0x6b00, 0x000},
   {op_bge_16       , 0xffff, 0x6c00, 0x000},
   {op_bge_8        , 0xff00, 0x6c00, 0x000},
   {op_blt_16       , 0xffff, 0x6d00, 0x000},
   {op_blt_8        , 0xff00, 0x6d00, 0x000},
   {op_bgt_16       , 0xffff, 0x6e00, 0x000},
   {op_bgt_8        , 0xff00, 0x6e00, 0x000},
   {op_ble_16       , 0xffff, 0x6f00, 0x000},
   {op_ble_8        , 0xff00, 0x6f00, 0x000},
   {op_bchg_r       , 0xf1c0, 0x0140, 0xbf8},
   {op_bchg_s       , 0xffc0, 0x0840, 0xbf8},
   {op_bclr_r       , 0xf1c0, 0x0180, 0xbf8},
   {op_bclr_s       , 0xffc0, 0x0880, 0xbf8},
   {op_bra_16       , 0xffff, 0x6000, 0x000},
   {op_bra_8        , 0xff00, 0x6000, 0x000},
   {op_bset_r       , 0xf1c0, 0x01c0, 0xbf8},
   {op_bset_s       , 0xffc0, 0x08c0, 0xbf8},
   {op_bsr_16       , 0xffff, 0x6100, 0x000},
   {op_bsr_8        , 0xff00, 0x6100, 0x000},
   {op_btst_r       , 0xf1c0, 0x0100, 0xbff},
   {op_btst_s       , 0xffc0, 0x0800, 0xbfb},
   {op_chk          , 0xf1c0, 0x4180, 0xbff},
   {op_clr_8        , 0xffc0, 0x4200, 0xbf8},
   {op_clr_16       , 0xffc0, 0x4240, 0xbf8},
   {op_clr_32       , 0xffc0, 0x4280, 0xbf8},
   {op_cmp_8        , 0xf1c0, 0xb000, 0xbff},
   {op_cmp_16       , 0xf1c0, 0xb040, 0xfff},
   {op_cmp_32       , 0xf1c0, 0xb080, 0xfff},
   {op_cmpa_16      , 0xf1c0, 0xb0c0, 0xfff},
   {op_cmpa_32      , 0xf1c0, 0xb1c0, 0xfff},
   {op_cmpi_8       , 0xffc0, 0x0c00, 0xbf8},
   {op_cmpi_16      , 0xffc0, 0x0c40, 0xbf8},
   {op_cmpi_32      , 0xffc0, 0x0c80, 0xbf8},
   {op_cmpm_8       , 0xf1f8, 0xb108, 0x000},
   {op_cmpm_16      , 0xf1f8, 0xb148, 0x000},
   {op_cmpm_32      , 0xf1f8, 0xb188, 0x000},
   {op_dbt          , 0xfff8, 0x50c8, 0x000},
   {op_dbf          , 0xfff8, 0x51c8, 0x000},
   {op_dbhi         , 0xfff8, 0x52c8, 0x000},
   {op_dbls         , 0xfff8, 0x53c8, 0x000},
   {op_dbcc         , 0xfff8, 0x54c8, 0x000},
   {op_dbcs         , 0xfff8, 0x55c8, 0x000},
   {op_dbne         , 0xfff8, 0x56c8, 0x000},
   {op_dbeq         , 0xfff8, 0x57c8, 0x000},
   {op_dbvc         , 0xfff8, 0x58c8, 0x000},
   {op_dbvs         , 0xfff8, 0x59c8, 0x000},
   {op_dbpl         , 0xfff8, 0x5ac8, 0x000},
   {op_dbmi         , 0xfff8, 0x5bc8, 0x000},
   {op_dbge         , 0xfff8, 0x5cc8, 0x000},
   {op_dblt         , 0xfff8, 0x5dc8, 0x000},
   {op_dbgt         , 0xfff8, 0x5ec8, 0x000},
   {op_dble         , 0xfff8, 0x5fc8, 0x000},
   {op_divs         , 0xf1c0, 0x81c0, 0xbff},
   {op_divu         , 0xf1c0, 0x80c0, 0xbff},
   {op_eor_8        , 0xf1c0, 0xb100, 0xbf8},
   {op_eor_16       , 0xf1c0, 0xb140, 0xbf8},
   {op_eor_32       , 0xf1c0, 0xb180, 0xbf8},
   {op_eori_to_ccr  , 0xffff, 0x0a3c, 0x000},
   {op_eori_to_sr   , 0xffff, 0x0a7c, 0x000},
   {op_eori_8       , 0xffc0, 0x0a00, 0xbf8},
   {op_eori_16      , 0xffc0, 0x0a40, 0xbf8},
   {op_eori_32      , 0xffc0, 0x0a80, 0xbf8},
   {op_exg_dd       , 0xf1f8, 0xc140, 0x000},
   {op_exg_aa       , 0xf1f8, 0xc148, 0x000},
   {op_exg_da       , 0xf1f8, 0xc188, 0x000},
   {op_ext_16       , 0xfff8, 0x4880, 0x000},
   {op_ext_32       , 0xfff8, 0x48c0, 0x000},
   {op_illegal      , 0xffff, 0x4afc, 0x000},
   {op_jmp          , 0xffc0, 0x4ec0, 0x27b},
   {op_jsr          , 0xffc0, 0x4e80, 0x27b},
   {op_lea          , 0xf1c0, 0x41c0, 0x27b},
   {op_link         , 0xfff8, 0x4e50, 0x000},
   {op_lsr_s_8      , 0xf1f8, 0xe008, 0x000},
   {op_lsr_s_16     , 0xf1f8, 0xe048, 0x000},
   {op_lsr_s_32     , 0xf1f8, 0xe088, 0x000},
   {op_lsr_r_8      , 0xf1f8, 0xe028, 0x000},
   {op_lsr_r_16     , 0xf1f8, 0xe068, 0x000},
   {op_lsr_r_32     , 0xf1f8, 0xe0a8, 0x000},
   {op_lsr_ea       , 0xffc0, 0xe2c0, 0x3f8},
   {op_lsl_s_8      , 0xf1f8, 0xe108, 0x000},
   {op_lsl_s_16     , 0xf1f8, 0xe148, 0x000},
   {op_lsl_s_32     , 0xf1f8, 0xe188, 0x000},
   {op_lsl_r_8      , 0xf1f8, 0xe128, 0x000},
   {op_lsl_r_16     , 0xf1f8, 0xe168, 0x000},
   {op_lsl_r_32     , 0xf1f8, 0xe1a8, 0x000},
   {op_lsl_ea       , 0xffc0, 0xe3c0, 0x3f8},
   {op_move_dd_8    , 0xf1c0, 0x1000, 0xbff},
   {op_move_ai_8    , 0xf1c0, 0x1080, 0xbff},
   {op_move_pi_8    , 0xf1c0, 0x10c0, 0xbff},
   {op_move_pd_8    , 0xf1c0, 0x1100, 0xbff},
   {op_move_di_8    , 0xf1c0, 0x1140, 0xbff},
   {op_move_ix_8    , 0xf1c0, 0x1180, 0xbff},
   {op_move_aw_8    , 0xffc0, 0x11c0, 0xbff},
   {op_move_al_8    , 0xffc0, 0x13c0, 0xbff},
   {op_move_dd_16   , 0xf1c0, 0x3000, 0xfff},
   {op_move_ai_16   , 0xf1c0, 0x3080, 0xfff},
   {op_move_pi_16   , 0xf1c0, 0x30c0, 0xfff},
   {op_move_pd_16   , 0xf1c0, 0x3100, 0xfff},
   {op_move_di_16   , 0xf1c0, 0x3140, 0xfff},
   {op_move_ix_16   , 0xf1c0, 0x3180, 0xfff},
   {op_move_aw_16   , 0xffc0, 0x31c0, 0xfff},
   {op_move_al_16   , 0xffc0, 0x33c0, 0xfff},
   {op_move_dd_32   , 0xf1c0, 0x2000, 0xfff},
   {op_move_ai_32   , 0xf1c0, 0x2080, 0xfff},
   {op_move_pi_32   , 0xf1c0, 0x20c0, 0xfff},
   {op_move_pd_32   , 0xf1c0, 0x2100, 0xfff},
   {op_move_di_32   , 0xf1c0, 0x2140, 0xfff},
   {op_move_ix_32   , 0xf1c0, 0x2180, 0xfff},
   {op_move_aw_32   , 0xffc0, 0x21c0, 0xfff},
   {op_move_al_32   , 0xffc0, 0x23c0, 0xfff},
   {op_movea_16     , 0xf1c0, 0x3040, 0xfff},
   {op_movea_32     , 0xf1c0, 0x2040, 0xfff},
   {op_move_to_ccr  , 0xffc0, 0x44c0, 0xbff},
   {op_move_fr_sr   , 0xffc0, 0x40c0, 0xbf8},
   {op_move_to_sr   , 0xffc0, 0x46c0, 0xbff},
   {op_move_to_usp  , 0xfff8, 0x4e60, 0x000},
   {op_move_fr_usp  , 0xfff8, 0x4e68, 0x000},
   {op_movem_pd_16  , 0xfff8, 0x48a0, 0x000},
   {op_movem_pd_32  , 0xfff8, 0x48e0, 0x000},
   {op_movem_re_16  , 0xffc0, 0x4880, 0x37b},
   {op_movem_re_32  , 0xffc0, 0x48c0, 0x37b},
   {op_movem_er_16  , 0xffc0, 0x4c80, 0x37b},
   {op_movem_er_32  , 0xffc0, 0x4cc0, 0x37b},
   {op_movep_er_16  , 0xf1f8, 0x0108, 0x000},
   {op_movep_er_32  , 0xf1f8, 0x0148, 0x000},
   {op_movep_re_16  , 0xf1f8, 0x0188, 0x000},
   {op_movep_re_32  , 0xf1f8, 0x01c8, 0x000},
   {op_moveq        , 0xf100, 0x7000, 0x000},
   {op_muls         , 0xf1c0, 0xc1c0, 0xbff},
   {op_mulu         , 0xf1c0, 0xc0c0, 0xbff},
   {op_nbcd         , 0xffc0, 0x4800, 0xbf8},
   {op_neg_8        , 0xffc0, 0x4400, 0xbf8},
   {op_neg_16       , 0xffc0, 0x4440, 0xbf8},
   {op_neg_32       , 0xffc0, 0x4480, 0xbf8},
   {op_negx_8       , 0xffc0, 0x4000, 0xbf8},
   {op_negx_16      , 0xffc0, 0x4040, 0xbf8},
   {op_negx_32      , 0xffc0, 0x4080, 0xbf8},
   {op_nop          , 0xffff, 0x4e71, 0x000},
   {op_not_8        , 0xffc0, 0x4600, 0xbf8},
   {op_not_16       , 0xffc0, 0x4640, 0xbf8},
   {op_not_32       , 0xffc0, 0x4680, 0xbf8},
   {op_or_er_8      , 0xf1c0, 0x8000, 0xbff},
   {op_or_er_16     , 0xf1c0, 0x8040, 0xbff},
   {op_or_er_32     , 0xf1c0, 0x8080, 0xbff},
   {op_or_re_8      , 0xf1c0, 0x8100, 0x3f8},
   {op_or_re_16     , 0xf1c0, 0x8140, 0x3f8},
   {op_or_re_32     , 0xf1c0, 0x8180, 0x3f8},
   {op_ori_to_ccr   , 0xffff, 0x003c, 0x000},
   {op_ori_to_sr    , 0xffff, 0x007c, 0x000},
   {op_ori_8        , 0xffc0, 0x0000, 0xbf8},
   {op_ori_16       , 0xffc0, 0x0040, 0xbf8},
   {op_ori_32       , 0xffc0, 0x0080, 0xbf8},
   {op_pea          , 0xffc0, 0x4840, 0x27b},
   {op_reset        , 0xffff, 0x4e70, 0x000},
   {op_ror_s_8      , 0xf1f8, 0xe018, 0x000},
   {op_ror_s_16     , 0xf1f8, 0xe058, 0x000},
   {op_ror_s_32     , 0xf1f8, 0xe098, 0x000},
   {op_ror_r_8      , 0xf1f8, 0xe038, 0x000},
   {op_ror_r_16     , 0xf1f8, 0xe078, 0x000},
   {op_ror_r_32     , 0xf1f8, 0xe0b8, 0x000},
   {op_ror_ea       , 0xffc0, 0xe6c0, 0x3f8},
   {op_rol_s_8      , 0xf1f8, 0xe118, 0x000},
   {op_rol_s_16     , 0xf1f8, 0xe158, 0x000},
   {op_rol_s_32     , 0xf1f8, 0xe198, 0x000},
   {op_rol_r_8      , 0xf1f8, 0xe138, 0x000},
   {op_rol_r_16     , 0xf1f8, 0xe178, 0x000},
   {op_rol_r_32     , 0xf1f8, 0xe1b8, 0x000},
   {op_rol_ea       , 0xffc0, 0xe7c0, 0x3f8},
   {op_roxr_s_8     , 0xf1f8, 0xe010, 0x000},
   {op_roxr_s_16    , 0xf1f8, 0xe050, 0x000},
   {op_roxr_s_32    , 0xf1f8, 0xe090, 0x000},
   {op_roxr_r_8     , 0xf1f8, 0xe030, 0x000},
   {op_roxr_r_16    , 0xf1f8, 0xe070, 0x000},
   {op_roxr_r_32    , 0xf1f8, 0xe0b0, 0x000},
   {op_roxr_ea      , 0xffc0, 0xe4c0, 0x3f8},
   {op_roxl_s_8     , 0xf1f8, 0xe110, 0x000},
   {op_roxl_s_16    , 0xf1f8, 0xe150, 0x000},
   {op_roxl_s_32    , 0xf1f8, 0xe190, 0x000},
   {op_roxl_r_8     , 0xf1f8, 0xe130, 0x000},
   {op_roxl_r_16    , 0xf1f8, 0xe170, 0x000},
   {op_roxl_r_32    , 0xf1f8, 0xe1b0, 0x000},
   {op_roxl_ea      , 0xffc0, 0xe5c0, 0x3f8},
   {op_rte          , 0xffff, 0x4e73, 0x000},
   {op_rtr          , 0xffff, 0x4e77, 0x000},
   {op_rts          , 0xffff, 0x4e75, 0x000},
   {op_sbcd_rr      , 0xf1f8, 0x8100, 0x000},
   {op_sbcd_mm      , 0xf1f8, 0x8108, 0x000},
   {op_st           , 0xffc0, 0x50c0, 0xbf8},
   {op_sf           , 0xffc0, 0x51c0, 0xbf8},
   {op_shi          , 0xffc0, 0x52c0, 0xbf8},
   {op_sls          , 0xffc0, 0x53c0, 0xbf8},
   {op_scc          , 0xffc0, 0x54c0, 0xbf8},
   {op_scs          , 0xffc0, 0x55c0, 0xbf8},
   {op_sne          , 0xffc0, 0x56c0, 0xbf8},
   {op_seq          , 0xffc0, 0x57c0, 0xbf8},
   {op_svc          , 0xffc0, 0x58c0, 0xbf8},
   {op_svs          , 0xffc0, 0x59c0, 0xbf8},
   {op_spl          , 0xffc0, 0x5ac0, 0xbf8},
   {op_smi          , 0xffc0, 0x5bc0, 0xbf8},
   {op_sge          , 0xffc0, 0x5cc0, 0xbf8},
   {op_slt          , 0xffc0, 0x5dc0, 0xbf8},
   {op_sgt          , 0xffc0, 0x5ec0, 0xbf8},
   {op_sle          , 0xffc0, 0x5fc0, 0xbf8},
   {op_stop         , 0xffff, 0x4e72, 0x000},
   {op_sub_er_8     , 0xf1c0, 0x9000, 0xbff},
   {op_sub_er_16    , 0xf1c0, 0x9040, 0xfff},
   {op_sub_er_32    , 0xf1c0, 0x9080, 0xfff},
   {op_sub_re_8     , 0xf1c0, 0x9100, 0x3f8},
   {op_sub_re_16    , 0xf1c0, 0x9140, 0x3f8},
   {op_sub_re_32    , 0xf1c0, 0x9180, 0x3f8},
   {op_suba_16      , 0xf1c0, 0x90c0, 0xfff},
   {op_suba_32      , 0xf1c0, 0x91c0, 0xfff},
   {op_subi_8       , 0xffc0, 0x0400, 0xbf8},
   {op_subi_16      , 0xffc0, 0x0440, 0xbf8},
   {op_subi_32      , 0xffc0, 0x0480, 0xbf8},
   {op_subq_8       , 0xf1c0, 0x5100, 0xbf8},
   {op_subq_16      , 0xf1c0, 0x5140, 0xff8},
   {op_subq_32      , 0xf1c0, 0x5180, 0xff8},
   {op_subx_rr_8    , 0xf1f8, 0x9100, 0x000},
   {op_subx_rr_16   , 0xf1f8, 0x9140, 0x000},
   {op_subx_rr_32   , 0xf1f8, 0x9180, 0x000},
   {op_subx_mm_8    , 0xf1f8, 0x9108, 0x000},
   {op_subx_mm_16   , 0xf1f8, 0x9148, 0x000},
   {op_subx_mm_32   , 0xf1f8, 0x9188, 0x000},
   {op_swap         , 0xfff8, 0x4840, 0x000},
   {op_tas          , 0xffc0, 0x4ac0, 0xbf8},
   {op_trap         , 0xfff0, 0x4e40, 0x000},
   {op_trapv        , 0xffff, 0x4e76, 0x000},
   {op_tst_8        , 0xffc0, 0x4a00, 0xbf8},
   {op_tst_16       , 0xffc0, 0x4a40, 0xbf8},
   {op_tst_32       , 0xffc0, 0x4a80, 0xbf8},
   {op_unlk         , 0xfff8, 0x4e58, 0x000},
   {0, 0, 0, 0}
};

/* Check if opcode is using a valid ea mode */
static int valid_ea(uint opcode, uint mask)
{
   if(mask == 0)
      return 1;

   switch(opcode & 0x3f)
   {
      case 0x00: case 0x01: case 0x02: case 0x03:
      case 0x04: case 0x05: case 0x06: case 0x07:
         return (mask & 0x800) != 0;
      case 0x08: case 0x09: case 0x0a: case 0x0b:
      case 0x0c: case 0x0d: case 0x0e: case 0x0f:
         return (mask & 0x400) != 0;
      case 0x10: case 0x11: case 0x12: case 0x13:
      case 0x14: case 0x15: case 0x16: case 0x17:
         return (mask & 0x200) != 0;
      case 0x18: case 0x19: case 0x1a: case 0x1b:
      case 0x1c: case 0x1d: case 0x1e: case 0x1f:
         return (mask & 0x100) != 0;
      case 0x20: case 0x21: case 0x22: case 0x23:
      case 0x24: case 0x25: case 0x26: case 0x27:
         return (mask & 0x080) != 0;
      case 0x28: case 0x29: case 0x2a: case 0x2b:
      case 0x2c: case 0x2d: case 0x2e: case 0x2f:
         return (mask & 0x040) != 0;
      case 0x30: case 0x31: case 0x32: case 0x33:
      case 0x34: case 0x35: case 0x36: case 0x37:
         return (mask & 0x020) != 0;
      case 0x38:
         return (mask & 0x010) != 0;
      case 0x39:
         return (mask & 0x008) != 0;
      case 0x3a:
         return (mask & 0x002) != 0;
      case 0x3b:
         return (mask & 0x001) != 0;
      case 0x3c:
         return (mask & 0x004) != 0;
   }
   return 0;

}

/* Used by qsort */
static int compare_nof_true_bits(const void *aptr, const void *bptr)
{
   uint a = ((opcode_struct*)aptr)->mask;
   uint b = ((opcode_struct*)bptr)->mask;
   int count_a = ((a & 0x8000) ? 1 : 0) + ((a & 0x4000) ? 1 : 0) + ((a & 0x2000) ? 1 : 0) + ((a & 0x1000) ? 1 : 0) +
                 ((a & 0x0800) ? 1 : 0) + ((a & 0x0400) ? 1 : 0) + ((a & 0x0200) ? 1 : 0) + ((a & 0x0100) ? 1 : 0) +
                 ((a & 0x0080) ? 1 : 0) + ((a & 0x0040) ? 1 : 0) + ((a & 0x0020) ? 1 : 0) + ((a & 0x0010) ? 1 : 0) +
                 ((a & 0x0008) ? 1 : 0) + ((a & 0x0004) ? 1 : 0) + ((a & 0x0002) ? 1 : 0) + ((a & 0x0001) ? 1 : 0);
   int count_b = ((b & 0x8000) ? 1 : 0) + ((b & 0x4000) ? 1 : 0) + ((b & 0x2000) ? 1 : 0) + ((b & 0x1000) ? 1 : 0) +
                 ((b & 0x0800) ? 1 : 0) + ((b & 0x0400) ? 1 : 0) + ((b & 0x0200) ? 1 : 0) + ((b & 0x0100) ? 1 : 0) +
                 ((b & 0x0080) ? 1 : 0) + ((b & 0x0040) ? 1 : 0) + ((b & 0x0020) ? 1 : 0) + ((b & 0x0010) ? 1 : 0) +
                 ((b & 0x0008) ? 1 : 0) + ((b & 0x0004) ? 1 : 0) + ((b & 0x0002) ? 1 : 0) + ((b & 0x0001) ? 1 : 0);
   return count_b - count_a; /* reversed to get greatest to least sorting */
}

/* build the opcode handler jump table */
static void build_opcode_table(void)
{
   uint i;
   uint opcode;
   opcode_struct* ostruct;
   uint opcode_info_length = 0;

   for(ostruct = g_opcode_info;ostruct->opcode_handler != 0;ostruct++)
      opcode_info_length++;

   qsort((void *)g_opcode_info, opcode_info_length, sizeof(g_opcode_info[0]), compare_nof_true_bits);

   for(i=0;i<0x10000;i++)
   {
      g_instruction_table[i] = op_illegal; /* default to illegal */
      opcode = i;
      /* search through opcode info for a match */
      for(ostruct = g_opcode_info;ostruct->opcode_handler != 0;ostruct++)
      {
         /* match opcode mask and allowed ea modes */
         if((opcode & ostruct->mask) == ostruct->match)
         {
            if(valid_ea(opcode, ostruct->ea_mask))
            {
               g_instruction_table[i] = ostruct->opcode_handler;
               break;
            }
            else
               /* I like to know if it was just an invalid ea mode */
               g_instruction_table[i] = op_invalid_ea;
         }
      }
   }
}



/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
