/* 
	Intel 386 emulator

	Written by Ville Linde
*/

#include "cpuintrf.h"
#include "mamedbg.h"
#include "osd_cpu.h"
#include "i386.h"
#include "state.h"

/*************************************************************************/

int i386_ICount;

UINT32 i386_translate(int segment, UINT32 ip)
{
	UINT32 r = 0;

	if( PROTECTED_MODE ) {
		r = I.sreg[segment].base + ip;
	} else {
		r = (I.sreg[segment].selector << 4) + (ip & 0xffff);
		/* A20 lines are initially set, but zeroed after first intra-segment jump */
		if( segment == CS && !I.performed_intersegment_jump )
			r |= 0xfff00000;
	}

	return r;
}

void i386_load_segment_descriptor( int segment )
{
	UINT32 v1,v2;
	UINT32 base, limit;
	int entry;

	if( I.sreg[segment].selector & 0x4 ) {
		base = I.ldtr.base;
		limit = I.ldtr.limit;
	} else {
		base = I.gdtr.base;
		limit = I.gdtr.limit;
	}

	if (limit == 0)
		return;
	entry = (I.sreg[segment].selector % limit) & ~0x7;

	v1 = READ32( base + entry );
	v2 = READ32( base + entry + 4 );

	I.sreg[segment].base = (v2 & 0xff000000) | ((v2 & 0xff) << 16) | ((v1 >> 16) & 0xffff);
	I.sreg[segment].limit = ((v2 << 16) & 0xf0000) | (v1 & 0xffff);
	I.sreg[segment].d = (v2 & 0x800000) ? 1 : 0;
}

UINT32 get_flags(void)
{
	UINT32 f = 0x2;
	f |= I.CF;
	f |= I.PF << 2;
	f |= I.AF << 4;
	f |= I.ZF << 6;
	f |= I.SF << 7;
	f |= I.TF << 8;
	f |= I.IF << 9;
	f |= I.DF << 10;
	f |= I.OF << 11;
	return (I.eflags & 0xFFFF0000) | (f & 0xFFFF);
}

void set_flags( UINT32 f )
{
	I.CF = (f & 0x1) ? 1 : 0;
	I.PF = (f & 0x4) ? 1 : 0;
	I.AF = (f & 0x10) ? 1 : 0;
	I.ZF = (f & 0x40) ? 1 : 0;
	I.SF = (f & 0x80) ? 1 : 0;
	I.TF = (f & 0x100) ? 1 : 0;
	I.IF = (f & 0x200) ? 1 : 0;
	I.DF = (f & 0x400) ? 1 : 0;
	I.OF = (f & 0x800) ? 1 : 0;
}

static void sib_byte(UINT8 mod, UINT32* out_ea, UINT8* out_segment)
{
	UINT32 ea = 0;
	UINT8 segment = 0;
	UINT8 scale, i, base;
	UINT8 sib = FETCH();
	scale = (sib >> 6) & 0x3;
	i = (sib >> 3) & 0x7;
	base = sib & 0x7;

	switch( base )
	{
		case 0: ea = REG32(EAX); segment = DS; break;
		case 1: ea = REG32(ECX); segment = DS; break;
		case 2: ea = REG32(EDX); segment = DS; break;
		case 3: ea = REG32(EBX); segment = DS; break;
		case 4: ea = REG32(ESP); segment = SS; break;
		case 5:
			if( mod == 0 ) {
				ea = FETCH32();
				segment = DS;
			} else if( mod == 1 ) {
				ea = REG32(EBP);
				segment = SS;
			} else if( mod == 2 ) {
				ea = REG32(EBP);
				segment = SS;
			}
			break;
		case 6: ea = REG32(ESI); segment = DS; break;
		case 7: ea = REG32(EDI); segment = DS; break;
	}
	switch( i )
	{
		case 0: ea += REG32(EAX) * (1 << scale); break;
		case 1: ea += REG32(ECX) * (1 << scale); break;
		case 2: ea += REG32(EDX) * (1 << scale); break;
		case 3: ea += REG32(EBX) * (1 << scale); break;
		case 4: break;
		case 5: ea += REG32(EBP) * (1 << scale); break;
		case 6: ea += REG32(ESI) * (1 << scale); break;
		case 7: ea += REG32(EDI) * (1 << scale); break;
	}
	*out_ea = ea;
	*out_segment = segment;
}
		
static void modrm_to_EA(UINT8 mod_rm, UINT32* out_ea, UINT8* out_segment)
{
	INT8 disp8;
	INT16 disp16;
	INT32 disp32;
	UINT8 mod = (mod_rm >> 6) & 0x3;
	UINT8 rm = mod_rm & 0x7;
	UINT32 ea;
	UINT8 segment;

	if( mod_rm >= 0xc0 )
		osd_die("i386: Called modrm_to_EA with modrm value %02X !\n",mod_rm);

	if( I.address_size ) {
		switch( rm )
		{
			case 0: ea = REG32(EAX); segment = DS; break;
			case 1: ea = REG32(ECX); segment = DS; break;
			case 2: ea = REG32(EDX); segment = DS; break;
			case 3: ea = REG32(EBX); segment = DS; break;
			case 4: sib_byte( mod, &ea, &segment ); break;
			case 5: 
				if( mod == 0 ) {
					ea = FETCH32(); segment = DS;
				} else {
					ea = REG32(EBP); segment = SS; 
				}
				break;
			case 6: ea = REG32(ESI); segment = DS; break;
			case 7: ea = REG32(EDI); segment = DS; break;
		}
		if( mod == 1 ) {
			disp8 = FETCH();
			ea += (INT32)disp8;
		} else if( mod == 2 ) {
			disp32 = FETCH32();
			ea += disp32;
		}

		if( I.segment_prefix )
			segment = I.segment_override;

		*out_ea = ea;
		*out_segment = segment;
	
	} else {
		switch( rm )
		{
			case 0: ea = REG16(BX) + REG16(SI); segment = DS; break;
			case 1: ea = REG16(BX) + REG16(DI); segment = DS; break;
			case 2: ea = REG16(BP) + REG16(SI); segment = SS; break;
			case 3: ea = REG16(BP) + REG16(DI); segment = SS; break;
			case 4: ea = REG16(SI); segment = DS; break;
			case 5: ea = REG16(DI); segment = DS; break;
			case 6:
				if( mod == 0 ) {
					ea = FETCH16(); segment = DS;
				} else {
					ea = REG16(BP); segment = SS; 
				}
				break;
			case 7: ea = REG16(BX); segment = DS; break;
		}
		if( mod == 1 ) {
			disp8 = FETCH();
			ea += (INT32)disp8;
		} else if( mod == 2 ) {
			disp16 = FETCH16();
			ea += (INT32)disp16;
		}

		if( I.segment_prefix )
			segment = I.segment_override;

		*out_ea = ea & 0xffff;
		*out_segment = segment;
	}
}

static UINT32 GetNonTranslatedEA(UINT8 modrm)
{
	UINT8 segment;
	UINT32 ea;
	modrm_to_EA( modrm, &ea, &segment );
	return ea;
}

static UINT32 GetEA(UINT8 modrm)
{
	UINT8 segment;
	UINT32 ea;
	modrm_to_EA( modrm, &ea, &segment );
	return i386_translate( segment, ea );
}

static void i386_trap(int irq)
{
	/*	I386 Interrupts/Traps/Faults:
	 *
	 *	0x00	Divide by zero
	 *	0x01	Debug exception
	 *	0x02	NMI
	 *	0x03	Int3
	 *	0x04	Overflow
	 *	0x05	Array bounds check
	 *	0x06	Illegal Opcode
	 *	0x07	FPU not available
	 *	0x08	Double fault
	 *	0x09	Coprocessor segment overrun
	 *	0x0a	Invalid task state
	 *	0x0b	Segment not present
	 *	0x0c	Stack exception
	 *	0x0d	General Protection Fault
	 *	0x0e	Page fault
	 *	0x0f	Reserved
	 *	0x10	Coprocessor error
	 */
	UINT32 v1, v2;
	UINT32 offset;
	UINT16 segment;
	int entry = irq * (I.sreg[CS].d ? 8 : 4);

	/* Check if IRQ is out of IDTR's bounds */
	if( entry > I.idtr.limit ) {
		osd_die("I386 Interrupt: IRQ out of IDTR bounds (IRQ: %d, IDTR Limit: %d)\n", irq, I.idtr.limit);
	}

	if( !I.sreg[CS].d )
	{
		/* 16-bit */
		PUSH16( get_flags() & 0xffff );
		PUSH16( I.sreg[CS].selector );
		PUSH16( I.eip );

		I.sreg[CS].selector = READ16( I.idtr.base + entry + 2 );
		I.eip = READ32( I.idtr.base + entry );
	}
	else
	{
		/* 32-bit */
		PUSH32( get_flags() & 0x00fcffff );
		PUSH32( I.sreg[CS].selector );
		PUSH32( I.eip );

		v1 = READ32( I.idtr.base + entry );
		v2 = READ32( I.idtr.base + entry + 4 );
		offset = (v2 & 0xffff0000) | (v1 & 0xffff);
		segment = (v1 >> 16) & 0xffff;

		I.sreg[CS].selector = segment;
		I.eip = offset;
	}
	i386_load_segment_descriptor(CS);
	CHANGE_PC(I.eip);
}

static void i386_interrupt(int irq)
{
	/* Check if the interrupts are enabled */
	if( I.IF )
		i386_trap(irq);
}



#include "i386ops.c"
#include "i386op16.c"
#include "i386op16.h"
#include "i386op32.c"
#include "i386op32.h"

static void I386OP(decode_opcode)(void)
{
	I.opcode = FETCH();
	if( I.operand_size )
		i386_opcode_table1_32[I.opcode]();
	else
		i386_opcode_table1_16[I.opcode]();
}

/* Two-byte opcode prefix */
static void I386OP(decode_two_byte)(void)
{
	I.opcode = FETCH();
	if( I.operand_size )
		i386_opcode_table2_32[I.opcode]();
	else
		i386_opcode_table2_16[I.opcode]();
}

/*************************************************************************/

static void i386_postload(void)
{
	int i;
	for (i = 0; i < 6; i++)
		i386_load_segment_descriptor(i);
	CHANGE_PC(I.eip);
}

void i386_init(void)
{
	int i, j;
	int regs8[8] = {AL,CL,DL,BL,AH,CH,DH,BH};
	int regs16[8] = {AX,CX,DX,BX,SP,BP,SI,DI};
	int regs32[8] = {EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI};
	int cpu = cpu_getactivecpu();
	const char *state_type = "I386";

	for( i=0; i < 256; i++ ) {
		int c=0;
		for( j=0; j < 8; j++ ) {
			if( i & (1 << j) )
				c++;
		}
		parity_table[i] = ~(c & 0x1) & 0x1;
	}
	
	for( i=0; i < 256; i++ ) {
		MODRM_table[i].reg.b = regs8[(i >> 3) & 0x7];
		MODRM_table[i].reg.w = regs16[(i >> 3) & 0x7];
		MODRM_table[i].reg.d = regs32[(i >> 3) & 0x7];

		MODRM_table[i].rm.b = regs8[i & 0x7];
		MODRM_table[i].rm.w = regs16[i & 0x7];
		MODRM_table[i].rm.d = regs32[i & 0x7];
	}

	state_save_register_UINT32(state_type, cpu,	"REGS",			I.reg.d, 8);
	state_save_register_UINT16(state_type, cpu,	"ES",			&I.sreg[ES].selector, 1);
	state_save_register_UINT16(state_type, cpu,	"CS",			&I.sreg[CS].selector, 1);
	state_save_register_UINT16(state_type, cpu,	"SS",			&I.sreg[SS].selector, 1);
	state_save_register_UINT16(state_type, cpu,	"DS",			&I.sreg[DS].selector, 1);
	state_save_register_UINT16(state_type, cpu,	"FS",			&I.sreg[FS].selector, 1);
	state_save_register_UINT16(state_type, cpu,	"GS",			&I.sreg[GS].selector, 1);
	state_save_register_UINT32(state_type, cpu,	"EIP",			&I.eip, 1);
	state_save_register_UINT32(state_type, cpu,	"PREV_EIP",		&I.prev_eip, 1);
	state_save_register_UINT8(state_type, cpu,	"CF",			&I.CF, 1);
	state_save_register_UINT8(state_type, cpu,	"DF",			&I.DF, 1);
	state_save_register_UINT8(state_type, cpu,	"SF",			&I.SF, 1);
	state_save_register_UINT8(state_type, cpu,	"OF",			&I.OF, 1);
	state_save_register_UINT8(state_type, cpu,	"ZF",			&I.ZF, 1);
	state_save_register_UINT8(state_type, cpu,	"PF",			&I.PF, 1);
	state_save_register_UINT8(state_type, cpu,	"AF",			&I.AF, 1);
	state_save_register_UINT8(state_type, cpu,	"IF",			&I.IF, 1);
	state_save_register_UINT8(state_type, cpu,	"TF",			&I.TF, 1);
	state_save_register_UINT32(state_type, cpu,	"CR",			I.cr, 4);
	state_save_register_UINT32(state_type, cpu,	"DR",			I.dr, 8);
	state_save_register_UINT32(state_type, cpu,	"TR",			I.tr, 8);
	state_save_register_UINT32(state_type, cpu,	"IDTR_BASE",	&I.idtr.base, 1);
	state_save_register_UINT16(state_type, cpu,	"IDTR_LIMIT",	&I.idtr.limit, 1);
	state_save_register_UINT32(state_type, cpu,	"GDTR_BASE",	&I.gdtr.base, 1);
	state_save_register_UINT16(state_type, cpu,	"GDTR_LIMIT",	&I.gdtr.limit, 1);
	state_save_register_UINT8(state_type, cpu,	"ISEGJMP",		&I.performed_intersegment_jump, 1);
	state_save_register_func_postload(i386_postload);
}

void i386_reset(void *param)
{
	memset( &I, 0, sizeof(I386_REGS) );
	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.cr[0] = 0;
	I.eflags = 0;
	I.eip = 0xfff0;

	REG32(EAX) = 0x0308;	// Intel 386, stepping D1
	REG32(EDX) = 0;

	CHANGE_PC(I.eip);
}

void i386_exit(void)
{

}

static void i386_get_context(void *dst)
{
	if(dst) {
		*(I386_REGS *)dst = I;
	}
}

static void i386_set_context(void *src)
{
	if(src) {
		I = *(I386_REGS *)src;
	}

	CHANGE_PC(I.eip);
}

unsigned i386_get_reg(int regnum)
{
	switch(regnum)
	{
		case REG_PC:		return I.pc;
		case I386_EIP:		return I.eip;
		case I386_EAX:		return REG32(EAX);
		case I386_EBX:		return REG32(EBX);
		case I386_ECX:		return REG32(ECX);
		case I386_EDX:		return REG32(EDX);
		case I386_EBP:		return REG32(EBP);
		case I386_ESP:		return REG32(ESP);
		case I386_ESI:		return REG32(ESI);
		case I386_EDI:		return REG32(EDI);
		case I386_EFLAGS:	return I.eflags;
		case I386_CS:		return I.sreg[CS].selector;
		case I386_SS:		return I.sreg[SS].selector;
		case I386_DS:		return I.sreg[DS].selector;
		case I386_ES:		return I.sreg[ES].selector;
		case I386_FS:		return I.sreg[FS].selector;
		case I386_GS:		return I.sreg[GS].selector;
		case I386_CR0:		return I.cr[0];
		case I386_CR1:		return I.cr[1];
		case I386_CR2:		return I.cr[2];
		case I386_CR3:		return I.cr[3];
		case I386_DR0:		return I.dr[0];
		case I386_DR1:		return I.dr[1];
		case I386_DR2:		return I.dr[2];
		case I386_DR3:		return I.dr[3];
		case I386_DR4:		return I.dr[4];
		case I386_DR5:		return I.dr[5];
		case I386_DR6:		return I.dr[6];
		case I386_DR7:		return I.dr[7];
		case I386_TR6:		return I.tr[6];
		case I386_TR7:		return I.tr[7];
	}
	return 0;
}

void i386_set_reg(int regnum, unsigned value)
{
	switch(regnum)
	{
		case I386_EIP:		I.eip = value; break;
		case I386_EAX:		REG32(EAX) = value; break;
		case I386_EBX:		REG32(EBX) = value; break;
		case I386_ECX:		REG32(ECX) = value; break;
		case I386_EDX:		REG32(EDX) = value; break;
		case I386_EBP:		REG32(EBP) = value; break;
		case I386_ESP:		REG32(ESP) = value; break;
		case I386_ESI:		REG32(ESI) = value; break;
		case I386_EDI:		REG32(EDI) = value; break;
		case I386_EFLAGS:	I.eflags = value; break;
		case I386_CS:		I.sreg[CS].selector = value & 0xffff; break;
		case I386_SS:		I.sreg[SS].selector = value & 0xffff; break;
		case I386_DS:		I.sreg[DS].selector = value & 0xffff; break;
		case I386_ES:		I.sreg[ES].selector = value & 0xffff; break;
		case I386_FS:		I.sreg[FS].selector = value & 0xffff; break;
		case I386_GS:		I.sreg[GS].selector = value & 0xffff; break;
		case I386_CR0:		I.cr[0] = value; break;
		case I386_CR1:		I.cr[1] = value; break;
		case I386_CR2:		I.cr[2] = value; break;
		case I386_CR3:		I.cr[3] = value; break;
		case I386_DR0:		I.dr[0] = value; break;
		case I386_DR1:		I.dr[1] = value; break;
		case I386_DR2:		I.dr[2] = value; break;
		case I386_DR3:		I.dr[3] = value; break;
		case I386_DR4:		I.dr[4] = value; break;
		case I386_DR5:		I.dr[5] = value; break;
		case I386_DR6:		I.dr[6] = value; break;
		case I386_DR7:		I.dr[7] = value; break;
		case I386_TR6:		I.tr[6] = value; break;
		case I386_TR7:		I.tr[7] = value; break;
	}
}

void i386_set_irq_line(int irqline, int state)
{
	if ( state )
		i386_interrupt( irqline );
}

void i386_set_irq_callback(int (*callback)(int))
{

}

int i386_execute(int num_cycles)
{
	I.cycles = num_cycles;
	CHANGE_PC(I.eip);

	while( I.cycles > 0 )
	{
		I.operand_size = I.sreg[CS].d;
		I.address_size = I.sreg[CS].d;
		I.segment_prefix = 0;
		
		CALL_MAME_DEBUG;
		
		I386OP(decode_opcode)();
	}

	return num_cycles - I.cycles;
}

/*************************************************************************/

static UINT8 i386_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1, 
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		0
};

static UINT8 i386_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/*************************************************************************/

unsigned i386_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return i386_dasm_one(buffer, pc, I.sreg[CS].d, I.sreg[CS].d);
#else
	sprintf( buffer, "$%02X", READ8(pc) );
	return 1;
#endif
}

static void i386_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_IRQ_STATE && state <= CPUINFO_INT_IRQ_STATE + 32)
	{
		i386_set_irq_line(state-CPUINFO_INT_IRQ_STATE, info->i);
		return;
	}

	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_PC:							I.pc = info->i;break;
		case CPUINFO_INT_REGISTER + I386_EIP:			I.eip = info->i; CHANGE_PC(I.eip); break;
		case CPUINFO_INT_REGISTER + I386_EAX:			REG32(EAX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EBX:			REG32(EBX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_ECX:			REG32(ECX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EDX:			REG32(EDX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EBP:			REG32(EBP) = info->i; break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + I386_ESP:			REG32(ESP) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_ESI:			REG32(ESI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EDI:			REG32(EDI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EFLAGS:		I.eflags = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CS:			I.sreg[CS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_SS:			I.sreg[SS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_DS:			I.sreg[DS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_ES:			I.sreg[ES].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_FS:			I.sreg[FS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_GS:			I.sreg[GS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_CR0:			I.cr[0] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR1:			I.cr[1] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR2:			I.cr[2] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR3:			I.cr[3] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR0:			I.dr[0] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR1:			I.dr[1] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR2:			I.dr[2] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR3:			I.dr[3] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR4:			I.dr[4] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR5:			I.dr[5] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR6:			I.dr[6] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR7:			I.dr[7] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_TR6:			I.tr[6] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_TR7:			I.tr[7] = info->i; break;
		
		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:				 break;	/* we don't have one */
	}
}

void i386_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:				info->i = sizeof(I386_REGS);			break;
		case CPUINFO_INT_IRQ_LINES:					info->i = 32;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:		info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:				info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:				info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:		info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:		info->i = 8;							break;
		case CPUINFO_INT_MIN_CYCLES:				info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:				info->i = 40;							break;
		
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_IRQ_STATE:				info->i = CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:				/* not implemented */					break;

		case CPUINFO_INT_PC:					info->i = I.pc; break;
		case CPUINFO_INT_REGISTER + I386_EIP:			info->i = I.eip; break;
		case CPUINFO_INT_REGISTER + I386_EAX:			info->i = REG32(EAX); break;
		case CPUINFO_INT_REGISTER + I386_EBX:			info->i = REG32(EBX); break;
		case CPUINFO_INT_REGISTER + I386_ECX:			info->i = REG32(ECX); break;
		case CPUINFO_INT_REGISTER + I386_EDX:			info->i = REG32(EDX); break;
		case CPUINFO_INT_REGISTER + I386_EBP:			info->i = REG32(EBP); break;
		case CPUINFO_INT_REGISTER + I386_ESP:			info->i = REG32(ESP); break;
		case CPUINFO_INT_REGISTER + I386_ESI:			info->i = REG32(ESI); break;
		case CPUINFO_INT_REGISTER + I386_EDI:			info->i = REG32(EDI); break;
		case CPUINFO_INT_REGISTER + I386_EFLAGS:		info->i = I.eflags; break;
		case CPUINFO_INT_REGISTER + I386_CS:			info->i = I.sreg[CS].selector; break;
		case CPUINFO_INT_REGISTER + I386_SS:			info->i = I.sreg[SS].selector; break;
		case CPUINFO_INT_REGISTER + I386_DS:			info->i = I.sreg[DS].selector; break;
		case CPUINFO_INT_REGISTER + I386_ES:			info->i = I.sreg[ES].selector; break;
		case CPUINFO_INT_REGISTER + I386_FS:			info->i = I.sreg[FS].selector; break;
		case CPUINFO_INT_REGISTER + I386_GS:			info->i = I.sreg[GS].selector; break;
		case CPUINFO_INT_REGISTER + I386_CR0:			info->i = I.cr[0]; break;
		case CPUINFO_INT_REGISTER + I386_CR1:			info->i = I.cr[1]; break;
		case CPUINFO_INT_REGISTER + I386_CR2:			info->i = I.cr[2]; break;
		case CPUINFO_INT_REGISTER + I386_CR3:			info->i = I.cr[3]; break;
		case CPUINFO_INT_REGISTER + I386_DR0:			info->i = I.dr[0]; break;
		case CPUINFO_INT_REGISTER + I386_DR1:			info->i = I.dr[1]; break;
		case CPUINFO_INT_REGISTER + I386_DR2:			info->i = I.dr[2]; break;
		case CPUINFO_INT_REGISTER + I386_DR3:			info->i = I.dr[3]; break;
		case CPUINFO_INT_REGISTER + I386_DR4:			info->i = I.dr[4]; break;
		case CPUINFO_INT_REGISTER + I386_DR5:			info->i = I.dr[5]; break;
		case CPUINFO_INT_REGISTER + I386_DR6:			info->i = I.dr[6]; break;
		case CPUINFO_INT_REGISTER + I386_DR7:			info->i = I.dr[7]; break;
		case CPUINFO_INT_REGISTER + I386_TR6:			info->i = I.tr[6]; break;
		case CPUINFO_INT_REGISTER + I386_TR7:			info->i = I.tr[7]; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = i386_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = i386_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = i386_set_context;	break;
		case CPUINFO_PTR_INIT:		      				info->init = i386_init;			break;
		case CPUINFO_PTR_RESET:		      				info->reset = i386_reset;		break;
		case CPUINFO_PTR_EXIT:		      				info->exit = i386_exit;			break;
		case CPUINFO_PTR_EXECUTE:	      				info->execute = i386_execute;		break;
		case CPUINFO_PTR_BURN:		      				info->burn = NULL;			break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = i386_dasm;		break;
		case CPUINFO_PTR_IRQ_CALLBACK:					break;	/* not supported */
		case CPUINFO_PTR_INSTRUCTION_COUNTER: 			info->icount = &I.cycles;		break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = i386_reg_layout;		break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = i386_win_layout;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "I386"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel 386"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2003-2004 Ville Linde"); break;

		case CPUINFO_STR_FLAGS:	   				sprintf(info->s = cpuintrf_temp_str(), "%08X", get_flags()); break; 

		case CPUINFO_STR_REGISTER + I386_EIP:			sprintf(info->s = cpuintrf_temp_str(), "EIP: %08X", I.eip); break;
		case CPUINFO_STR_REGISTER + I386_EAX:			sprintf(info->s = cpuintrf_temp_str(), "EAX: %08X", I.reg.d[EAX]); break;
		case CPUINFO_STR_REGISTER + I386_EBX:			sprintf(info->s = cpuintrf_temp_str(), "EBX: %08X", I.reg.d[EBX]); break;
		case CPUINFO_STR_REGISTER + I386_ECX:			sprintf(info->s = cpuintrf_temp_str(), "ECX: %08X", I.reg.d[ECX]); break;
		case CPUINFO_STR_REGISTER + I386_EDX:			sprintf(info->s = cpuintrf_temp_str(), "EDX: %08X", I.reg.d[EDX]); break;
		case CPUINFO_STR_REGISTER + I386_EBP:			sprintf(info->s = cpuintrf_temp_str(), "EBP: %08X", I.reg.d[EBP]); break;
		case CPUINFO_STR_REGISTER + I386_ESP:			sprintf(info->s = cpuintrf_temp_str(), "ESP: %08X", I.reg.d[ESP]); break;
		case CPUINFO_STR_REGISTER + I386_ESI:			sprintf(info->s = cpuintrf_temp_str(), "ESI: %08X", I.reg.d[ESI]); break;
		case CPUINFO_STR_REGISTER + I386_EDI:			sprintf(info->s = cpuintrf_temp_str(), "EDI: %08X", I.reg.d[EDI]); break;
		case CPUINFO_STR_REGISTER + I386_EFLAGS:		sprintf(info->s = cpuintrf_temp_str(), "EFLAGS: %08X", I.eflags); break;
		case CPUINFO_STR_REGISTER + I386_CS:			sprintf(info->s = cpuintrf_temp_str(), "CS: %04X:%08X", I.sreg[CS].selector, I.sreg[CS].base); break;
		case CPUINFO_STR_REGISTER + I386_SS:			sprintf(info->s = cpuintrf_temp_str(), "SS: %04X:%08X", I.sreg[SS].selector, I.sreg[SS].base); break;
		case CPUINFO_STR_REGISTER + I386_DS:			sprintf(info->s = cpuintrf_temp_str(), "DS: %04X:%08X", I.sreg[DS].selector, I.sreg[DS].base); break;
		case CPUINFO_STR_REGISTER + I386_ES:			sprintf(info->s = cpuintrf_temp_str(), "ES: %04X:%08X", I.sreg[ES].selector, I.sreg[ES].base); break;
		case CPUINFO_STR_REGISTER + I386_FS:			sprintf(info->s = cpuintrf_temp_str(), "FS: %04X:%08X", I.sreg[FS].selector, I.sreg[FS].base); break;
		case CPUINFO_STR_REGISTER + I386_GS:			sprintf(info->s = cpuintrf_temp_str(), "GS: %04X:%08X", I.sreg[GS].selector, I.sreg[GS].base); break;
		case CPUINFO_STR_REGISTER + I386_CR0:			sprintf(info->s = cpuintrf_temp_str(), "CR0: %08X", I.cr[0]); break;
		case CPUINFO_STR_REGISTER + I386_CR1:			sprintf(info->s = cpuintrf_temp_str(), "CR1: %08X", I.cr[1]); break;
		case CPUINFO_STR_REGISTER + I386_CR2:			sprintf(info->s = cpuintrf_temp_str(), "CR2: %08X", I.cr[2]); break;
		case CPUINFO_STR_REGISTER + I386_CR3:			sprintf(info->s = cpuintrf_temp_str(), "CR3: %08X", I.cr[3]); break;
		case CPUINFO_STR_REGISTER + I386_DR0:			sprintf(info->s = cpuintrf_temp_str(), "DR0: %08X", I.dr[0]); break;
		case CPUINFO_STR_REGISTER + I386_DR1:			sprintf(info->s = cpuintrf_temp_str(), "DR1: %08X", I.dr[1]); break;
		case CPUINFO_STR_REGISTER + I386_DR2:			sprintf(info->s = cpuintrf_temp_str(), "DR2: %08X", I.dr[2]); break;
		case CPUINFO_STR_REGISTER + I386_DR3:			sprintf(info->s = cpuintrf_temp_str(), "DR3: %08X", I.dr[3]); break;
		case CPUINFO_STR_REGISTER + I386_DR4:			sprintf(info->s = cpuintrf_temp_str(), "DR4: %08X", I.dr[4]); break;
		case CPUINFO_STR_REGISTER + I386_DR5:			sprintf(info->s = cpuintrf_temp_str(), "DR5: %08X", I.dr[5]); break;
		case CPUINFO_STR_REGISTER + I386_DR6:			sprintf(info->s = cpuintrf_temp_str(), "DR6: %08X", I.dr[6]); break;
		case CPUINFO_STR_REGISTER + I386_DR7:			sprintf(info->s = cpuintrf_temp_str(), "DR7: %08X", I.dr[7]); break;
		case CPUINFO_STR_REGISTER + I386_TR6:			sprintf(info->s = cpuintrf_temp_str(), "TR6: %08X", I.tr[6]); break;
		case CPUINFO_STR_REGISTER + I386_TR7:			sprintf(info->s = cpuintrf_temp_str(), "TR7: %08X", I.tr[7]); break;
	}
}


