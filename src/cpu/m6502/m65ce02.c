/*****************************************************************************
 *
 *	 m65ce02.c
 *	 Portable 65ce02 emulator V1.0beta2
 *
 *	 Copyright (c) 1998 Juergen Buchmueller
 *	 Copyright (c) 2000 Peter Trauner
 *   all rights reserved.
 *   documentation by michael steil mist@c64.org
 *   available at ftp://ftp.funet.fi/pub/cbm/c65
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/
/* 4. February 2000 PeT fixed relative word operand */
/* 4. February 2000 PeT jsr (absolut) jsr (absolut,x) inw dew */
/* 17.February 2000 PeT phw */

/* 
  chapter3.txt lines about 5400
  descripe some rom entries

* neg is now simple 2er komplement negation with set of N and Z

* asw is arithmetic (signed) shift left
* row is rotate left

* inw has zeropage address operand! (not absolute) (c65 dos at 0xba1a)

* row, asw has absolute adressing

* phw push low order byte, push high order byte!

* cle/see
  maybe extended stack flag (real 16 bit stack pointer inc/dec)
  usage of high order byte?
  tys txs not interruptable ??

* map
  jmp/jsr bank lda # ldy #0 ldx #$e0 or bank&0xf ldy #$f0 or bank&0xf


   notes (differences to michael steil document)
   sequences in c65 

   0xc800 (interface code) (system init)
   lda #$00
   ldx #$e3
   ldy #$00
   ldz #$b3
   map
   see
   (the values in the register are not used in the following code)
    i think, map configures memory management!
    e flag set maybe means not the c64 compatible mode

   ldx #$ff
   ldy #$01
   txs
   tys 
*   makes me feel, we have a real 16bit stackpointer !
    and the tys and tsy mnemonics access the highbyte!
*   having a real stackpointer high byte makes me feel

   irq handler
   pha phx phy phz tsy tsx phy phx ldy #$05 lda ($01,x),y   
    the new lda sta (byte indexed stack), indirect, indexed )
*    is more (stack byte indexed), indirect, indexed)

   switch to c64 mode
   lda #$00 sta $d609 sta $d031 sta $d030 sta $d031 tax tay taz map
   jsr ($fff6) jsr $(fffc)

   map in monitor and call it
   lda #$a0 ldx #$82 ldy #$00 ldz #$b3 map ... jmp $6000

   get dos
   ... lda #$00 ldx #$11 ldy #$80 ldz #$31 map

   remove dos
   ... lda #$00 ldx #$00 ldy #$00 ldz #$b3 map



12. $FF6E JSRFAR  ;gosub in another bank
13. $FF71 JMPFAR  ;goto another bank

         Preparation:

               Registers:  none

               Memory:     system map, also:
                           $02 --> bank (0-FF)
                           $03 --> PC_high
                           $04 --> PC_low
                           $05 --> .S (status)
                           $06 --> .A
                           $07 --> .X
                           $08 --> .Y
                           $09 --> .Z

               Flags:      none

               Calls:      none

         Results:

               Registers:  none

               Memory:     as per call, also:
                           $05 --> .S (status)
                           $06 --> .A
                           $07 --> .X
                           $08 --> .Y
                           $09 --> .Z

               Flags:      none

The  two  routines,  JSRFAR  and  JMPFAR, enable code executing in the
system bank of memory to call (or JMP to) a routine in any other bank.
In  the case of JSRFAR, the called routine must restore the system map
before executing a return.
jsr entry:
0380: 20 9E 03 jsr  $039E
0383: 08       php  
0384: 48       pha  
0385: DA       phx  
0386: 5A       phy  
0387: DB       phz  
0388: 08       php  
0389: 20 C4 03 jsr  $03C4
038C: 68       pla  
038D: 85 05    sta  $05
038F: FB       plz  
0390: 64 09    stz  $09
0392: 7A       ply  
0393: 84 08    sty  $08
0395: FA       plx  
0396: 86 07    stx  $07
0398: 68       pla  
0399: 85 06    sta  $06
039B: 28       plp  
039C: EA       nop  
039D: 60       rts  
039E: FC 03 00 phw  $0003
03A1: A5 05    lda  $05
03A3: 48       pha  
03A4: A5 02    lda  $02
03A6: 10 05    bpl  $03AD
03A8: 20 C4 03 jsr  $03C4
03AB: 80 0C    bra  $03B9
03AD: 29 0F    and  #$0F
03AF: 09 E0    ora  #$E0
03B1: AA       tax  
03B2: 09 F0    ora  #$F0
03B4: 4B       taz  
03B5: A9 00    lda  #$00
03B7: A8       tay  
03B8: 5C       map  
03B9: A5 06    lda  $06
03BB: A6 07    ldx  $07
03BD: A4 08    ldy  $08
03BF: AB 09 00 ldz  $0009
03C2: EA       nop  
03C3: 40       rti  
03C4: A9 00    lda  #$00
03C6: A2 E3    ldx  #$E3
03C8: A0 00    ldy  #$00
03CA: A3 B3    ldz  #$B3
03CC: 5C       map  
03CD: 60       rts  
*/

#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "m65ce02.h"

#include "ops02.h"
#include "opsce02.h"

#define CORE_M65CE02

extern FILE * errorlog;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

/* Layout of the registers in the debugger */
static UINT8 m65ce02_reg_layout[] = {
	M65CE02_A,M65CE02_X,M65CE02_Y,M65CE02_Z,M65CE02_S,M65CE02_PC,-1,
	M65CE02_EA,M65CE02_ZP,M65CE02_NMI_STATE,M65CE02_IRQ_STATE, M65CE02_B,
	M65CE02_P, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 m65ce02_win_layout[] = {
	25, 0,55, 2,	/* register window (top, right rows) */
     0, 0,24,22,    /* disassembler window (left colums) */
    25, 3,55, 9,    /* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

typedef struct
{
	UINT8	subtype;		/* currently selected cpu sub type */
    void    (**insn)(void); /* pointer to the function pointer table */
	PAIR	ppc;			/* previous program counter */
    PAIR    pc;             /* program counter */
    PAIR    sp;             /* stack pointer (always 100 - 1FF) */
    PAIR    zp;             /* zero page address */
	/* contains B register zp.b.h */
    PAIR    ea;             /* effective address */
    UINT8   a;              /* Accumulator */
    UINT8   x;              /* X index register */
    UINT8   y;              /* Y index register */
    UINT8   z;              /* Z index register */
    UINT8   p;              /* Processor status */
	UINT8	pending_irq;	/* nonzero if an IRQ is pending */
    UINT8   after_cli;      /* pending IRQ and last insn cleared I */
	UINT8	nmi_state;
	UINT8	irq_state;
    int     (*irq_callback)(int irqline);   /* IRQ callback */
}   m65ce02_Regs;


int m65ce02_ICount = 0;

static m65ce02_Regs m65ce02;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/

static void (*c65_map)(int a, int x, int y, int z);
#include "t65ce02.c"
#include "t6502.c"
#include "t65c02.c"
#include "t65sc02.c"

static void (*insn65ce02[0x100])(void) = {
	m65ce02_00,m65ce02_01,m65ce02_02,m65ce02_03,m65ce02_04,m65ce02_05,m65ce02_06,m65ce02_07,
	m65ce02_08,m65ce02_09,m65ce02_0a,m65ce02_0b,m65ce02_0c,m65ce02_0d,m65ce02_0e,m65ce02_0f,
	m65ce02_10,m65ce02_11,m65ce02_12,m65ce02_13,m65ce02_14,m65ce02_15,m65ce02_16,m65ce02_17,
	m65ce02_18,m65ce02_19,m65ce02_1a,m65ce02_1b,m65ce02_1c,m65ce02_1d,m65ce02_1e,m65ce02_1f,
	m65ce02_20,m65ce02_21,m65ce02_22,m65ce02_23,m65ce02_24,m65ce02_25,m65ce02_26,m65ce02_27,
	m65ce02_28,m65ce02_29,m65ce02_2a,m65ce02_2b,m65ce02_2c,m65ce02_2d,m65ce02_2e,m65ce02_2f,
	m65ce02_30,m65ce02_31,m65ce02_32,m65ce02_33,m65ce02_34,m65ce02_35,m65ce02_36,m65ce02_37,
	m65ce02_38,m65ce02_39,m65ce02_3a,m65ce02_3b,m65ce02_3c,m65ce02_3d,m65ce02_3e,m65ce02_3f,
	m65ce02_40,m65ce02_41,m65ce02_42,m65ce02_43,m65ce02_44,m65ce02_45,m65ce02_46,m65ce02_47,
	m65ce02_48,m65ce02_49,m65ce02_4a,m65ce02_4b,m65ce02_4c,m65ce02_4d,m65ce02_4e,m65ce02_4f,
	m65ce02_50,m65ce02_51,m65ce02_52,m65ce02_53,m65ce02_54,m65ce02_55,m65ce02_56,m65ce02_57,
	m65ce02_58,m65ce02_59,m65ce02_5a,m65ce02_5b,m65ce02_5c,m65ce02_5d,m65ce02_5e,m65ce02_5f,
	m65ce02_60,m65ce02_61,m65ce02_62,m65ce02_63,m65ce02_64,m65ce02_65,m65ce02_66,m65ce02_67,
	m65ce02_68,m65ce02_69,m65ce02_6a,m65ce02_6b,m65ce02_6c,m65ce02_6d,m65ce02_6e,m65ce02_6f,
	m65ce02_70,m65ce02_71,m65ce02_72,m65ce02_73,m65ce02_74,m65ce02_75,m65ce02_76,m65ce02_77,
	m65ce02_78,m65ce02_79,m65ce02_7a,m65ce02_7b,m65ce02_7c,m65ce02_7d,m65ce02_7e,m65ce02_7f,
	m65ce02_80,m65ce02_81,m65ce02_82,m65ce02_83,m65ce02_84,m65ce02_85,m65ce02_86,m65ce02_87,
	m65ce02_88,m65ce02_89,m65ce02_8a,m65ce02_8b,m65ce02_8c,m65ce02_8d,m65ce02_8e,m65ce02_8f,
	m65ce02_90,m65ce02_91,m65ce02_92,m65ce02_93,m65ce02_94,m65ce02_95,m65ce02_96,m65ce02_97,
	m65ce02_98,m65ce02_99,m65ce02_9a,m65ce02_9b,m65ce02_9c,m65ce02_9d,m65ce02_9e,m65ce02_9f,
	m65ce02_a0,m65ce02_a1,m65ce02_a2,m65ce02_a3,m65ce02_a4,m65ce02_a5,m65ce02_a6,m65ce02_a7,
	m65ce02_a8,m65ce02_a9,m65ce02_aa,m65ce02_ab,m65ce02_ac,m65ce02_ad,m65ce02_ae,m65ce02_af,
	m65ce02_b0,m65ce02_b1,m65ce02_b2,m65ce02_b3,m65ce02_b4,m65ce02_b5,m65ce02_b6,m65ce02_b7,
	m65ce02_b8,m65ce02_b9,m65ce02_ba,m65ce02_bb,m65ce02_bc,m65ce02_bd,m65ce02_be,m65ce02_bf,
	m65ce02_c0,m65ce02_c1,m65ce02_c2,m65ce02_c3,m65ce02_c4,m65ce02_c5,m65ce02_c6,m65ce02_c7,
	m65ce02_c8,m65ce02_c9,m65ce02_ca,m65ce02_cb,m65ce02_cc,m65ce02_cd,m65ce02_ce,m65ce02_cf,
	m65ce02_d0,m65ce02_d1,m65ce02_d2,m65ce02_d3,m65ce02_d4,m65ce02_d5,m65ce02_d6,m65ce02_d7,
	m65ce02_d8,m65ce02_d9,m65ce02_da,m65ce02_db,m65ce02_dc,m65ce02_dd,m65ce02_de,m65ce02_df,
	m65ce02_e0,m65ce02_e1,m65ce02_e2,m65ce02_e3,m65ce02_e4,m65ce02_e5,m65ce02_e6,m65ce02_e7,
	m65ce02_e8,m65ce02_e9,m65ce02_ea,m65ce02_eb,m65ce02_ec,m65ce02_ed,m65ce02_ee,m65ce02_ef,
	m65ce02_f0,m65ce02_f1,m65ce02_f2,m65ce02_f3,m65ce02_f4,m65ce02_f5,m65ce02_f6,m65ce02_f7,
	m65ce02_f8,m65ce02_f9,m65ce02_fa,m65ce02_fb,m65ce02_fc,m65ce02_fd,m65ce02_fe,m65ce02_ff
};

void m65ce02_reset (void *param)
{
	c65_map=(void(*)(int a, int x, int y, int z))param;
	m65ce02.insn = insn65ce02;

	/* wipe out the rest of the m6502 structure */
	/* read the reset vector into PC */
	/* reset z index and b bank */
    PCL = RDMEM(M65CE02_RST_VEC);
    PCH = RDMEM(M65CE02_RST_VEC+1);

	m65ce02.sp.d = 0x0100 | (m6502.sp.b.l);	/* stack pointer (always 100 - 1FF) */
	m65ce02.p = F_T|F_I|F_Z;	/* set T, I and Z flags */
	m65ce02.pending_irq = 0;	/* nonzero if an IRQ is pending */
	m65ce02.after_cli = 0;	/* pending IRQ and last insn cleared I */
	m65ce02.irq_callback = NULL;

	change_pc16(PCD);
}

void m65ce02_exit(void)
{
	/* nothing to do yet */
}

unsigned m65ce02_get_context (void *dst)
{
	if( dst )
		*(m65ce02_Regs*)dst = m6502;
	return sizeof(m65ce02_Regs);
}

void m65ce02_set_context (void *src)
{
	if( src )
	{
		m65ce02 = *(m65ce02_Regs*)src;
		change_pc(PCD);
	}
}

unsigned m65ce02_get_pc (void)
{
	return PCD;
}

void m65ce02_set_pc (unsigned val)
{
	PCW = val;
	change_pc(PCD);
}

unsigned m65ce02_get_sp (void)
{
	return S;
}

void m65ce02_set_sp (unsigned val)
{
	S = val;
}

unsigned m65ce02_get_reg (int regnum)
{
	switch( regnum )
	{
		case M65CE02_PC: return m65ce02.pc.w.l;
		case M65CE02_S: return m65ce02.sp.w.l;
		case M65CE02_P: return m65ce02.p;
		case M65CE02_A: return m65ce02.a;
		case M65CE02_X: return m65ce02.x;
		case M65CE02_Y: return m65ce02.y;
		case M65CE02_Z: return m65ce02.z;
		case M65CE02_B: return m65ce02.zp.b.h;
		case M65CE02_EA: return m65ce02.ea.w.l;
		case M65CE02_ZP: return m65ce02.zp.b.l;
		case M65CE02_NMI_STATE: return m65ce02.nmi_state;
		case M65CE02_IRQ_STATE: return m65ce02.irq_state;
		case REG_PREVIOUSPC: return m65ce02.ppc.w.l;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
					return RDMEM( offset ) | ( RDMEM( offset + 1 ) << 8 );
			}
	}
	return 0;
}

void m65ce02_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
		case M65CE02_PC: m65ce02.pc.w.l = val; break;
		case M65CE02_S: m65ce02.sp.w.l = val; break;
		case M65CE02_P: m65ce02.p = val; break;
		case M65CE02_A: m65ce02.a = val; break;
		case M65CE02_X: m65ce02.x = val; break;
		case M65CE02_Y: m65ce02.y = val; break;
		case M65CE02_Z: m65ce02.z = val; break;
		case M65CE02_B: m65ce02.zp.b.h = val; break;
		case M65CE02_EA: m65ce02.ea.w.l = val; break;
		case M65CE02_ZP: m65ce02.zp.b.l = val; break;
		case M65CE02_NMI_STATE: m65ce02_set_nmi_line( val ); break;
		case M65CE02_IRQ_STATE: m65ce02_set_irq_line( 0, val ); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
				{
					WRMEM( offset, val & 0xfff );
					WRMEM( offset + 1, (val >> 8) & 0xff );
				}
			}
    }
}

INLINE void m65ce02_take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = M65CE02_IRQ_VEC;
		m65ce02_ICount -= 7;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		LOG((errorlog,"M6502#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD));
		/* call back the cpuintrf to let it clear the line */
		if (m65ce02.irq_callback) (*m65ce02.irq_callback)(0);
	    change_pc16(PCD);
	}
	m65ce02.pending_irq = 0;
}

int m65ce02_execute(int cycles)
{
	m65ce02_ICount = cycles;

	change_pc16(PCD);

	do
	{
		UINT8 op;
        PPC = PCD;

        CALL_MAME_DEBUG;

		op = RDOP();
        /* if an irq is pending, take it now */
		if( m65ce02.pending_irq && op == 0x78 )
            m65ce02_take_irq();

		(*m65ce02.insn[op])();

		/* check if the I flag was just reset (interrupts enabled) */
        if( m65ce02.after_cli )
        {
            LOG((errorlog,"M6502#%d after_cli was >0", cpu_getactivecpu()));
            m65ce02.after_cli = 0;
            if (m65ce02.irq_state != CLEAR_LINE)
            {
                LOG((errorlog,": irq line is asserted: set pending IRQ\n"));
                m65ce02.pending_irq = 1;
            }
            else
            {
                LOG((errorlog,": irq line is clear\n"));
            }
        }
		else
		if( m65ce02.pending_irq )
            m65ce02_take_irq();

    } while (m65ce02_ICount > 0);

	return cycles - m65ce02_ICount;
}

void m65ce02_set_nmi_line(int state)
{
	if (m65ce02.nmi_state == state) return;
	m65ce02.nmi_state = state;
	if( state != CLEAR_LINE )
	{
		LOG((errorlog, "M6502#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
		EAD = M6502_NMI_VEC;
		m65ce02_ICount -= 7;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		LOG((errorlog,"M6502#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD));
		change_pc16(PCD);
    }
}

void m65ce02_set_irq_line(int irqline, int state)
{
	m65ce02.irq_state = state;
	if( state != CLEAR_LINE )
	{
		LOG((errorlog, "M6502#%d set_irq_line(ASSERT)\n", cpu_getactivecpu()));
		m6502.pending_irq = 1;
	}
}

void m65ce02_set_irq_callback(int (*callback)(int))
{
	m65ce02.irq_callback = callback;
}

void m65ce02_state_save(void *file)
{
	int cpu = cpu_getactivecpu();
	/* insn is set at restore since it's a pointer */
	state_save_UINT16(file,"m65ce02",cpu,"PC",&m65ce02.pc.w.l,2);
	state_save_UINT16(file,"m65ce02",cpu,"SP",&m65ce02.sp.w.l,2);
	state_save_UINT8(file,"m65ce02",cpu,"P",&m65ce02.p,1);
	state_save_UINT8(file,"m65ce02",cpu,"A",&m65ce02.a,1);
	state_save_UINT8(file,"m65ce02",cpu,"X",&m65ce02.x,1);
	state_save_UINT8(file,"m65ce02",cpu,"Y",&m65ce02.y,1);
	state_save_UINT8(file,"m65ce02",cpu,"Z",&m65ce02.z,1);
	state_save_UINT8(file,"m65ce02",cpu,"B",&m65ce02.zp.b.h,1);
	state_save_UINT8(file,"m65ce02",cpu,"PENDING",&m65ce02.pending_irq,1);
	state_save_UINT8(file,"m65ce02",cpu,"AFTER_CLI",&m65ce02.after_cli,1);
	state_save_UINT8(file,"m65ce02",cpu,"NMI_STATE",&m65ce02.nmi_state,1);
	state_save_UINT8(file,"m65ce02",cpu,"IRQ_STATE",&m65ce02.irq_state,1);
}

void m65ce02_state_load(void *file)
{
	int cpu = cpu_getactivecpu();
	m65ce02.insn = insn65ce02;
	state_load_UINT16(file,"m65ce02",cpu,"PC",&m65ce02.pc.w.l,2);
	state_load_UINT16(file,"m65ce02",cpu,"SP",&m65ce02.sp.w.l,2);
	state_load_UINT8(file,"m65ce02",cpu,"P",&m65ce02.p,1);
	state_load_UINT8(file,"m65ce02",cpu,"A",&m65ce02.a,1);
	state_load_UINT8(file,"m65ce02",cpu,"X",&m65ce02.x,1);
	state_load_UINT8(file,"m65ce02",cpu,"Y",&m65ce02.y,1);
	state_load_UINT8(file,"m65ce02",cpu,"Z",&m65ce02.z,1);
	state_load_UINT8(file,"m65ce02",cpu,"B",&m65ce02.zp.b.h,1);
	state_load_UINT8(file,"m65ce02",cpu,"PENDING",&m65ce02.pending_irq,1);
	state_load_UINT8(file,"m65ce02",cpu,"AFTER_CLI",&m65ce02.after_cli,1);
	state_load_UINT8(file,"m65ce02",cpu,"NMI_STATE",&m65ce02.nmi_state,1);
	state_load_UINT8(file,"m65ce02",cpu,"IRQ_STATE",&m65ce02.irq_state,1);
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m65ce02_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m65ce02_Regs *r = context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &m65ce02;

    switch( regnum )
	{
		case CPU_INFO_REG+M65CE02_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+M65CE02_S: sprintf(buffer[which], "S:%04X", r->sp.w.l); break;
		case CPU_INFO_REG+M65CE02_P: sprintf(buffer[which], "P:%02X", r->p); break;
		case CPU_INFO_REG+M65CE02_A: sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+M65CE02_X: sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+M65CE02_Y: sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_REG+M65CE02_Z: sprintf(buffer[which], "Z:%02X", r->z); break;
		case CPU_INFO_REG+M65CE02_B: sprintf(buffer[which], "B:%02X", r->zp.b.h); break;
		case CPU_INFO_REG+M65CE02_EA: sprintf(buffer[which], "EA:%04X", r->ea.w.l); break;
		case CPU_INFO_REG+M65CE02_ZP: sprintf(buffer[which], "ZP:%04X", r->zp.w.l); break;
		case CPU_INFO_REG+M65CE02_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+M65CE02_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
        case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->p & 0x80 ? 'N':'.',
				r->p & 0x40 ? 'V':'.',
				r->p & 0x20 ? 'R':'.',
				r->p & 0x10 ? 'B':'.',
				r->p & 0x08 ? 'D':'.',
				r->p & 0x04 ? 'I':'.',
				r->p & 0x02 ? 'Z':'.',
				r->p & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "M65ce02";
		case CPU_INFO_FAMILY: return "CBM Semiconductor Group CSG 65ce02";
		case CPU_INFO_VERSION: return "1.0beta";
		case CPU_INFO_CREDITS: 
			return "Copyright (c) 1998 Juergen Buchmueller\n"
				"Copyright (c) 2000 Peter Trauner\n"
				"all rights reserved.";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_REG_LAYOUT: return (const char*)m65ce02_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m65ce02_win_layout;
	}
	return buffer[which];
}

unsigned m65ce02_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return Dasm65ce02( buffer, pc );
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}



