
/*

HNZVC

? = undefined
* = affected
- = unaffected
0 = cleared
1 = set
# = ccr directly affected by instruction
@ = special - carry set if bit 7 is set

*/

#ifdef NEW
static void illegal( void )
#else
INLINE void illegal( void )
#endif
{
	if(errorlog)fprintf(errorlog, "M6809: illegal opcode at %04x\n",pcreg);
}

#if macintosh
#pragma mark ____0x____
#endif

/* $00 NEG direct ?**** */
INLINE void neg_di( void )
{
	word r,t;
	DIRBYTE(t); r=-t;
	CLR_NZVC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $01 ILLEGAL */

/* $02 ILLEGAL */

/* $03 COM direct -**01 */
INLINE void com_di( void )
{
	byte t;
	DIRBYTE(t); t = ~t;
	CLR_NZV; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $04 LSR direct -0*-* */
INLINE void lsr_di( void )
{
	byte t;
	DIRBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $05 ILLEGAL */

/* $06 ROR direct -**-* */
INLINE void ror_di( void )
{
	byte t,r;
	DIRBYTE(t); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $07 ASR direct ?**-* */
INLINE void asr_di( void )
{
	byte t;
	DIRBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $08 ASL direct ?**** */
INLINE void asl_di( void )
{
	word t,r;
	DIRBYTE(t); r=t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $09 ROL direct -**** */
INLINE void rol_di( void )
{
	word t,r;
	DIRBYTE(t); r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $0A DEC direct -***- */
INLINE void dec_di( void )
{
	byte t;
	DIRBYTE(t); --t;
	CLR_NZV; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $0B ILLEGAL */

/* $OC INC direct -***- */
INLINE void inc_di( void )
{
	byte t;
	DIRBYTE(t); ++t;
	CLR_NZV; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $OD TST direct -**0- */
INLINE void tst_di( void )
{
	byte t;
	DIRBYTE(t); CLR_NZV; SET_NZ8(t);
}

/* $0E JMP direct ----- */
INLINE void jmp_di( void )
{
	DIRECT; pcreg=eaddr;change_pc(pcreg);/* ASG 971005 */
}

/* $0F CLR direct -0100 */
INLINE void clr_di( void )
{
	DIRECT; M_WRMEM(eaddr,0);
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____1x____
#endif

/* $10 FLAG */

/* $11 FLAG */

/* $12 NOP inherent ----- */
INLINE void nop( void )
{
	;
}

/* $13 SYNC inherent ----- */
INLINE void sync( void )
{
	/* SYNC should stop processing instructions until an interrupt occurs.
	   A decent fake is probably to force an immediate IRQ. */
	m6809_ICount = 0;
	pending_interrupts |= M6809_SYNC;	/* NS 980101 */
}

/* $14 ILLEGAL */

/* $15 ILLEGAL */

/* $16 LBRA relative ----- */
INLINE void lbra( void )
{
	IMMWORD(eaddr);

	pcreg+=eaddr;change_pc(pcreg);/* ASG 971005 */

	if ( eaddr == 0xfffd ) /* EHC 980508 speed up busy loop */
		m6809_ICount = 0;
}

/* $17 LBSR relative ----- */
INLINE void lbsr( void )
{
	IMMWORD(eaddr); PUSHWORD(pcreg); pcreg+=eaddr;change_pc(pcreg);/* ASG 971005 */
}

/* $18 ILLEGAL */

#if 1
/* $19 DAA inherent (areg) -**0* */
INLINE void daa( void )
{
	byte msn, lsn;
	word t, cf = 0;
	msn=areg & 0xf0; lsn=areg & 0x0f;
	if( lsn>0x09 || cc&0x20 ) cf |= 0x06;
	if( msn>0x80 && lsn>0x09 ) cf |= 0x60;
	if( msn>0x90 || cc&0x01 ) cf |= 0x60;
	t = cf + areg;
	CLR_NZV; /* keep carry from previous operation */
	SET_NZ8((byte)t); SET_C8(t);
	areg = t;
}
#else
/* $19 DAA inherent (areg) -**0* */
INLINE void daa( void )
{
	word t;
	t=areg;
	if (cc&0x20) t+=0x06;
	if ((t&0x0f)>9) t+=0x06;		/* ASG -- this code is broken! $66+$99=$FF -> DAA should = $65, we get $05! */
	if (cc&0x01) t+=0x60;
	if ((t&0xf0)>0x90) t+=0x60;
	if (t&0x100) SEC;
	areg=t;
}
#endif

/* $1A ORCC immediate ##### */
INLINE void orcc( void )
{
	byte t;
	IMMBYTE(t); cc|=t;
}

/* $1B ILLEGAL */

/* $1C ANDCC immediate ##### */
INLINE void andcc( void )
{
	byte t;
	IMMBYTE(t); cc&=t;
}

/* $1D SEX inherent -**0- */
INLINE void sex( void )
{
	word t;
	t = SIGNED(breg); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* $1E EXG inherent ----- */
INLINE void exg( void )
{
	word t1=0,t2=0;
	byte tb;

	IMMBYTE(tb); GETREG(t1,tb>>4); GETREG(t2,tb&15);
	SETREG(t2,tb>>4); SETREG(t1,tb&15);
}

/* $1F TFR inherent ----- */
INLINE void tfr( void )
{
	byte tb;
	word t=0;

	IMMBYTE(tb); GETREG(t,tb>>4); SETREG(t,tb&15);
}

#if macintosh
#pragma mark ____2x____
#endif

/* $20 BRA relative ----- */
INLINE void bra( void )
{
	byte t;
	IMMBYTE(t);pcreg+=SIGNED(t);change_pc(pcreg);	/* TS 971002 */
	/* JB 970823 - speed up busy loops */
	if (t==0xfe) m6809_ICount = 0;
}

/* $21 BRN relative ----- */
INLINE void brn( void )
{
	byte t;
	IMMBYTE(t);
}

/* $1021 LBRN relative ----- */
INLINE void lbrn( void )
{
	word t;
	IMMWORD(t);
}

/* $22 BHI relative ----- */
INLINE void bhi( void )
{
	byte t;
	BRANCH(!(cc&0x05));
}

/* $1022 LBHI relative ----- */
INLINE void lbhi( void )
{
	word t;
	LBRANCH(!(cc&0x05));
}

/* $23 BLS relative ----- */
INLINE void bls( void )
{
	byte t;
	BRANCH(cc&0x05);
}

/* $1023 LBLS relative ----- */
INLINE void lbls( void )
{
	word t;
	LBRANCH(cc&0x05);
}

/* $24 BCC relative ----- */
INLINE void bcc( void )
{
	byte t;
	BRANCH(!(cc&0x01));
}

/* $1024 LBCC relative ----- */
INLINE void lbcc( void )
{
	word t;
	LBRANCH(!(cc&0x01));
}

/* $25 BCS relative ----- */
INLINE void bcs( void )
{
	byte t;
	BRANCH(cc&0x01);
}

/* $1025 LBCS relative ----- */
INLINE void lbcs( void )
{
	word t;
	LBRANCH(cc&0x01);
}

/* $26 BNE relative ----- */
INLINE void bne( void )
{
	byte t;
	BRANCH(!(cc&0x04));
}

/* $1026 LBNE relative ----- */
INLINE void lbne( void )
{
	word t;
	LBRANCH(!(cc&0x04));
}

/* $27 BEQ relative ----- */
INLINE void beq( void )
{
	byte t;
	BRANCH(cc&0x04);
}

/* $1027 LBEQ relative ----- */
INLINE void lbeq( void )
{
	word t;
	LBRANCH(cc&0x04);
}

/* $28 BVC relative ----- */
INLINE void bvc( void )
{
	byte t;
	BRANCH(!(cc&0x02));
}

/* $1028 LBVC relative ----- */
INLINE void lbvc( void )
{
	word t;
	LBRANCH(!(cc&0x02));
}

/* $29 BVS relative ----- */
INLINE void bvs( void )
{
	byte t;
	BRANCH(cc&0x02);
}

/* $1029 LBVS relative ----- */
INLINE void lbvs( void )
{
	word t;
	LBRANCH(cc&0x02);
}

/* $2A BPL relative ----- */
INLINE void bpl( void )
{
	byte t;
	BRANCH(!(cc&0x08));
}

/* $102A LBPL relative ----- */
INLINE void lbpl( void )
{
	word t;
	LBRANCH(!(cc&0x08));
}

/* $2B BMI relative ----- */
INLINE void bmi( void )
{
	byte t;
	BRANCH(cc&0x08);
}

/* $102B LBMI relative ----- */
INLINE void lbmi( void )
{
	word t;
	LBRANCH(cc&0x08);
}

/* $2C BGE relative ----- */
INLINE void bge( void )
{
	byte t;
	BRANCH(!NXORV);
}

/* $102C LBGE relative ----- */
INLINE void lbge( void )
{
	word t;
	LBRANCH(!NXORV);
}

/* $2D BLT relative ----- */
INLINE void blt( void )
{
	byte t;
	BRANCH(NXORV);
}

/* $102D LBLT relative ----- */
INLINE void lblt( void )
{
	word t;
	LBRANCH(NXORV);
}

/* $2E BGT relative ----- */
INLINE void bgt( void )
{
	byte t;
	BRANCH(!(NXORV||cc&0x04));
}

/* $102E LBGT relative ----- */
INLINE void lbgt( void )
{
	word t;
	LBRANCH(!(NXORV||cc&0x04));
}

/* $2F BLE relative ----- */
INLINE void ble( void )
{
	byte t;
	BRANCH(NXORV||cc&0x04);
}

/* $102F LBLE relative ----- */
INLINE void lble( void )
{
	word t;
	LBRANCH(NXORV||cc&0x04);
}

#if macintosh
#pragma mark ____3x____
#endif

/* $30 LEAX indexed --*-- */
INLINE void leax( void )
{
	xreg=eaddr; CLR_Z; SET_Z(xreg);
}

/* $31 LEAY indexed --*-- */
INLINE void leay( void )
{
	yreg=eaddr; CLR_Z; SET_Z(yreg);
}

/* $32 LEAS indexed ----- */
INLINE void leas( void )
{
	sreg=eaddr;
}

/* $33 LEAU indexed ----- */
INLINE void leau( void )
{
	ureg=eaddr;
}

/* $34 PSHS inherent ----- */
INLINE void pshs( void )
{
	byte t;
	IMMBYTE(t);
	if(t&0x80) PUSHWORD(pcreg);
	if(t&0x40) PUSHWORD(ureg);
	if(t&0x20) PUSHWORD(yreg);
	if(t&0x10) PUSHWORD(xreg);
	if(t&0x08) PUSHBYTE(dpreg);
	if(t&0x04) PUSHBYTE(breg);
	if(t&0x02) PUSHBYTE(areg);
	if(t&0x01) PUSHBYTE(cc);
}

/* 35 PULS inherent ----- */
INLINE void puls( void )
{
	byte t;
	IMMBYTE(t);
	if(t&0x01) PULLBYTE(cc);
	if(t&0x02) PULLBYTE(areg);
	if(t&0x04) PULLBYTE(breg);
	if(t&0x08) PULLBYTE(dpreg);
	if(t&0x10) PULLWORD(xreg);
	if(t&0x20) PULLWORD(yreg);
	if(t&0x40) PULLWORD(ureg);
	if(t&0x80){ PULLWORD(pcreg);change_pc(pcreg); }	/* TS 971002 */
}

/* $36 PSHU inherent ----- */
INLINE void pshu( void )
{
	byte t;
	IMMBYTE(t);
	if(t&0x80) PSHUWORD(pcreg);
	if(t&0x40) PSHUWORD(sreg);
	if(t&0x20) PSHUWORD(yreg);
	if(t&0x10) PSHUWORD(xreg);
	if(t&0x08) PSHUBYTE(dpreg);
	if(t&0x04) PSHUBYTE(breg);
	if(t&0x02) PSHUBYTE(areg);
	if(t&0x01) PSHUBYTE(cc);
}

/* 37 PULU inherent ----- */
INLINE void pulu( void )
{
	byte t;
	IMMBYTE(t);
	if(t&0x01) PULUBYTE(cc);
	if(t&0x02) PULUBYTE(areg);
	if(t&0x04) PULUBYTE(breg);
	if(t&0x08) PULUBYTE(dpreg);
	if(t&0x10) PULUWORD(xreg);
	if(t&0x20) PULUWORD(yreg);
	if(t&0x40) PULUWORD(sreg);
	if(t&0x80) { PULUWORD(pcreg);change_pc(pcreg); }	/* TS 971002 */
}

/* $38 ILLEGAL */

/* $39 RTS inherent ----- */
INLINE void rts( void )
{
	PULLWORD(pcreg);change_pc(pcreg);	/* TS 971002 */
}

/* $3A ABX inherent ----- */
INLINE void abx( void )
{
	xreg += breg;
}

/* $3B RTI inherent ##### */
INLINE void rti( void )
{
	byte t;
/*	ASG 971016 t=cc&0x80; */
	PULLBYTE(cc);
	t=cc&0x80;	/*	ASG 971016 */
	if(t)
	{
		m6809_ICount -= 9;
		PULLBYTE(areg);
		PULLBYTE(breg);
		PULLBYTE(dpreg);
		PULLWORD(xreg);
		PULLWORD(yreg);
		PULLWORD(ureg);
	}
	PULLWORD(pcreg);change_pc(pcreg);	/* TS 971002 */
}

/* $3C CWAI inherent ----1 */
INLINE void cwai( void )
{
	byte t;
	IMMBYTE(t); cc&=t;
	/* CWAI should stack the entire machine state on the hardware stack,
		then wait for an interrupt. We just wait for an interrupt. */
	if ((pending_interrupts & M6809_INT_IRQ) != 0 && (cc & 0x10) == 0)
		return;
	else if ((pending_interrupts & M6809_INT_FIRQ) != 0 && (cc & 0x40) == 0)
		return;
	m6809_ICount = 0;
	pending_interrupts |= M6809_CWAI;	/* NS 980101 */
}

/* $3D MUL inherent --*-@ */
INLINE void mul( void )
{
	word t;
	t=areg*breg;
	CLR_ZC; SET_Z16(t); if(t&0x80) SEC;
	SETDREG(t);
}

/* $3E ILLEGAL */

/* $3F SWI (SWI2 SWI3) absolute indirect ----- */
INLINE void swi( void )
{
	cc|=0x80;
	PUSHWORD(pcreg);
	PUSHWORD(ureg);
	PUSHWORD(yreg);
	PUSHWORD(xreg);
	PUSHBYTE(dpreg);
	PUSHBYTE(breg);
	PUSHBYTE(areg);
	PUSHBYTE(cc);
	cc|=0x50;
	pcreg = M_RDMEM_WORD(0xfffa);change_pc(pcreg);	/* TS 971002 */
}

/* $103F SWI2 absolute indirect ----- */
INLINE void swi2( void )
{
	cc|=0x80;
	PUSHWORD(pcreg);
	PUSHWORD(ureg);
	PUSHWORD(yreg);
	PUSHWORD(xreg);
	PUSHBYTE(dpreg);
	PUSHBYTE(breg);
	PUSHBYTE(areg);
	PUSHBYTE(cc);
	pcreg = M_RDMEM_WORD(0xfff4);change_pc(pcreg);	/* TS 971002 */
}

/* $113F SWI3 absolute indirect ----- */
INLINE void swi3( void )
{
	cc|=0x80;
	PUSHWORD(pcreg);
	PUSHWORD(ureg);
	PUSHWORD(yreg);
	PUSHWORD(xreg);
	PUSHBYTE(dpreg);
	PUSHBYTE(breg);
	PUSHBYTE(areg);
	PUSHBYTE(cc);
	pcreg = M_RDMEM_WORD(0xfff2);change_pc(pcreg);	/* TS 971002 */
}

#if macintosh
#pragma mark ____4x____
#endif

/* $40 NEGA inherent ?**** */
INLINE void nega( void )
{
	word r;
	r=-areg;
	CLR_NZVC; SET_FLAGS8(0,areg,r);
	areg=r;
}

/* $41 ILLEGAL */

/* $42 ILLEGAL */

/* $43 COMA inherent -**01 */
INLINE void coma( void )
{
	areg = ~areg;
	CLR_NZV; SET_NZ8(areg); SEC;
}

/* $44 LSRA inherent -0*-* */
INLINE void lsra( void )
{
	CLR_NZC; cc|=(areg&0x01);
	areg>>=1; SET_Z8(areg);
}

/* $45 ILLEGAL */

/* $46 RORA inherent -**-* */
INLINE void rora( void )
{
	byte r;
	r=(cc&0x01)<<7;
	CLR_NZC; cc|=(areg&0x01);
	r |= areg>>1; SET_NZ8(r);
	areg=r;
}

/* $47 ASRA inherent ?**-* */
INLINE void asra( void )
{
	CLR_NZC; cc|=(areg&0x01);
	areg>>=1; areg|=((areg&0x40)<<1);
	SET_NZ8(areg);
}

/* $48 ASLA inherent ?**** */
INLINE void asla( void )
{
	word r;
	r=areg<<1;
	CLR_NZVC; SET_FLAGS8(areg,areg,r);
	areg=r;
}

/* $49 ROLA inherent -**** */
INLINE void rola( void )
{
	word t,r;
	t = areg; r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	areg=r;
}

/* $4A DECA inherent -***- */
INLINE void deca( void )
{
	--areg;
	CLR_NZV; SET_FLAGS8D(areg);
}

/* $4B ILLEGAL */

/* $4C INCA inherent -***- */
INLINE void inca( void )
{
	++areg;
	CLR_NZV; SET_FLAGS8I(areg);
}

/* $4D TSTA inherent -**0- */
INLINE void tsta( void )
{
	CLR_NZV; SET_NZ8(areg);
}

/* $4E ILLEGAL */

/* $4F CLRA inherent -0100 */
INLINE void clra( void )
{
	areg=0;
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____5x____
#endif

/* $50 NEGB inherent ?**** */
INLINE void negb( void )
{
	word r;
	r=-breg;
	CLR_NZVC; SET_FLAGS8(0,breg,r);
	breg=r;
}

/* $51 ILLEGAL */

/* $52 ILLEGAL */

/* $53 COMB inherent -**01 */
INLINE void comb( void )
{
	breg = ~breg;
	CLR_NZV; SET_NZ8(breg); SEC;
}

/* $54 LSRB inherent -0*-* */
INLINE void lsrb( void )
{
	CLR_NZC; cc|=(breg&0x01);
	breg>>=1; SET_Z8(breg);
}

/* $55 ILLEGAL */

/* $56 RORB inherent -**-* */
INLINE void rorb( void )
{
	byte r;
	r=(cc&0x01)<<7;
	CLR_NZC; cc|=(breg&0x01);
	r |= breg>>1; SET_NZ8(r);
	breg=r;
}

/* $57 ASRB inherent ?**-* */
INLINE void asrb( void )
{
	CLR_NZC; cc|=(breg&0x01);
	breg>>=1; breg|=((breg&0x40)<<1);
	SET_NZ8(breg);
}

/* $58 ASLB inherent ?**** */
INLINE void aslb( void )
{
	word r;
	r=breg<<1;
	CLR_NZVC; SET_FLAGS8(breg,breg,r);
	breg=r;
}

/* $59 ROLB inherent -**** */
INLINE void rolb( void )
{
	word t,r;
	t = breg; r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	breg=r;
}

/* $5A DECB inherent -***- */
INLINE void decb( void )
{
	--breg;
	CLR_NZV; SET_FLAGS8D(breg);
}

/* $5B ILLEGAL */

/* $5C INCB inherent -***- */
INLINE void incb( void )
{
	++breg;
	CLR_NZV; SET_FLAGS8I(breg);
}

/* $5D TSTB inherent -**0- */
INLINE void tstb( void )
{
	CLR_NZV; SET_NZ8(breg);
}

/* $5E ILLEGAL */

/* $5F CLRB inherent -0100 */
INLINE void clrb( void )
{
	breg=0;
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____6x____
#endif

/* $60 NEG indexed ?**** */
INLINE void neg_ix( void )
{
	word r,t;
	t=M_RDMEM(eaddr); r=-t;
	CLR_NZVC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $61 ILLEGAL */

/* $62 ILLEGAL */

/* $63 COM indexed -**01 */
INLINE void com_ix( void )
{
	byte t;
	t = ~M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $64 LSR indexed -0*-* */
INLINE void lsr_ix( void )
{
	byte t;
	t=M_RDMEM(eaddr); CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $65 ILLEGAL */

/* $66 ROR indexed -**-* */
INLINE void ror_ix( void )
{
	byte t,r;
	t=M_RDMEM(eaddr); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $67 ASR indexed ?**-* */
INLINE void asr_ix( void )
{
	byte t;
	t=M_RDMEM(eaddr); CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $68 ASL indexed ?**** */
INLINE void asl_ix( void )
{
	word t,r;
	t=M_RDMEM(eaddr); r=t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $69 ROL indexed -**** */
INLINE void rol_ix( void )
{
	word t,r;
	t=M_RDMEM(eaddr); r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $6A DEC indexed -***- */
INLINE void dec_ix( void )
{
	byte t;
	t=M_RDMEM(eaddr)-1;
	CLR_NZV; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $6B ILLEGAL */

/* $6C INC indexed -***- */
INLINE void inc_ix( void )
{
	byte t;
	t=M_RDMEM(eaddr)+1;
	CLR_NZV; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $6D TST indexed -**0- */
INLINE void tst_ix( void )
{
	byte t;
	t=M_RDMEM(eaddr); CLR_NZV; SET_NZ8(t);
}

/* $6E JMP indexed ----- */
INLINE void jmp_ix( void )
{
	pcreg=eaddr;change_pc(pcreg);	/* TS 971002 */
}

/* $6F CLR indexed -0100 */
INLINE void clr_ix( void )
{
	M_WRMEM(eaddr,0);
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____7x____
#endif

/* $70 NEG extended ?**** */
INLINE void neg_ex( void )
{
	word r,t;
	EXTBYTE(t); r=-t;
	CLR_NZVC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $71 ILLEGAL */

/* $72 ILLEGAL */

/* $73 COM extended -**01 */
INLINE void com_ex( void )
{
	byte t;
	EXTBYTE(t); t = ~t;
	CLR_NZV; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $74 LSR extended -0*-* */
INLINE void lsr_ex( void )
{
	byte t;
	EXTBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $75 ILLEGAL */

/* $76 ROR extended -**-* */
INLINE void ror_ex( void )
{
	byte t,r;
	EXTBYTE(t); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $77 ASR extended ?**-* */
INLINE void asr_ex( void )
{
	byte t;
	EXTBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $78 ASL extended ?**** */
INLINE void asl_ex( void )
{
	word t,r;
	EXTBYTE(t); r=t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $79 ROL extended -**** */
INLINE void rol_ex( void )
{
	word t,r;
	EXTBYTE(t); r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $7A DEC extended -***- */
INLINE void dec_ex( void )
{
	byte t;
	EXTBYTE(t); --t;
	CLR_NZV; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $7B ILLEGAL */

/* $7C INC extended -***- */
INLINE void inc_ex( void )
{
	byte t;
	EXTBYTE(t); ++t;
	CLR_NZV; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $7D TST extended -**0- */
INLINE void tst_ex( void )
{
	byte t;
	EXTBYTE(t); CLR_NZV; SET_NZ8(t);
}

/* $7E JMP extended ----- */
INLINE void jmp_ex( void )
{
	EXTENDED; pcreg=eaddr;change_pc(pcreg);	/* TS 971002 */
}

/* $7F CLR extended -0100 */
INLINE void clr_ex( void )
{
	EXTENDED; M_WRMEM(eaddr,0);
	CLR_NZVC; SEZ;
}


#if macintosh
#pragma mark ____8x____
#endif

/* $80 SUBA immediate ?**** */
INLINE void suba_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $81 CMPA immediate ?**** */
INLINE void cmpa_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $82 SBCA immediate ?**** */
INLINE void sbca_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $83 SUBD (CMPD CMPU) immediate -**** */
INLINE void subd_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $1083 CMPD immediate -**** */
INLINE void cmpd_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $1183 CMPU immediate -**** */
INLINE void cmpu_im( void )
{
	dword r,b;
	IMMWORD(b); r = ureg-b;
	CLR_NZVC; SET_FLAGS16(ureg,b,r);
}

/* $84 ANDA immediate -**0- */
INLINE void anda_im( void )
{
	byte t;
	IMMBYTE(t); areg &= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $85 BITA immediate -**0- */
INLINE void bita_im( void )
{
	byte t,r;
	IMMBYTE(t); r = areg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $86 LDA immediate -**0- */
INLINE void lda_im( void )
{
	IMMBYTE(areg);
	CLR_NZV; SET_NZ8(areg);
}

/* is this a legal instruction? */
/* $87 STA immediate -**0- */
INLINE void sta_im( void )
{
	CLR_NZV; SET_NZ8(areg);
	IMM8; M_WRMEM(eaddr,areg);
}

/* $88 EORA immediate -**0- */
INLINE void eora_im( void )
{
	byte t;
	IMMBYTE(t); areg ^= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $89 ADCA immediate ***** */
INLINE void adca_im( void )
{
	word t,r;
	IMMBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $8A ORA immediate -**0- */
INLINE void ora_im( void )
{
	byte t;
	IMMBYTE(t); areg |= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $8B ADDA immediate ***** */
INLINE void adda_im( void )
{
	word t,r;
	IMMBYTE(t); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $8C CMPX (CMPY CMPS) immediate -**** */
INLINE void cmpx_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $108C CMPY immediate -**** */
INLINE void cmpy_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = yreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $118C CMPS immediate -**** */
INLINE void cmps_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = sreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $8D BSR ----- */
INLINE void bsr( void )
{
	byte t;
	IMMBYTE(t); PUSHWORD(pcreg); pcreg += SIGNED(t);change_pc(pcreg);	/* TS 971002 */
}

/* $8E LDX (LDY) immediate -**0- */
INLINE void ldx_im( void )
{
	IMMWORD(xreg);
	CLR_NZV; SET_NZ16(xreg);
}

/* $108E LDY immediate -**0- */
INLINE void ldy_im( void )
{
	IMMWORD(yreg);
	CLR_NZV; SET_NZ16(yreg);
}

/* is this a legal instruction? */
/* $8F STX (STY) immediate -**0- */
INLINE void stx_im( void )
{
	CLR_NZV; SET_NZ16(xreg);
	IMM16; M_WRMEM_WORD(eaddr,xreg);
}

/* is this a legal instruction? */
/* $108F STY immediate -**0- */
INLINE void sty_im( void )
{
	CLR_NZV; SET_NZ16(yreg);
	IMM16; M_WRMEM_WORD(eaddr,yreg);
}

#if macintosh
#pragma mark ____9x____
#endif

/* $90 SUBA direct ?**** */
INLINE void suba_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $91 CMPA direct ?**** */
INLINE void cmpa_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $92 SBCA direct ?**** */
INLINE void sbca_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $93 SUBD (CMPD CMPU) direct -**** */
INLINE void subd_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $1093 CMPD direct -**** */
INLINE void cmpd_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $1193 CMPU direct -**** */
INLINE void cmpu_di( void )
{
	dword r,b;
	DIRWORD(b); r = ureg-b;
	CLR_NZVC; SET_FLAGS16(ureg,b,r);
}

/* $94 ANDA direct -**0- */
INLINE void anda_di( void )
{
	byte t;
	DIRBYTE(t); areg &= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $95 BITA direct -**0- */
INLINE void bita_di( void )
{
	byte t,r;
	DIRBYTE(t); r = areg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $96 LDA direct -**0- */
INLINE void lda_di( void )
{
	DIRBYTE(areg);
	CLR_NZV; SET_NZ8(areg);
}

/* $97 STA direct -**0- */
INLINE void sta_di( void )
{
	CLR_NZV; SET_NZ8(areg);
	DIRECT; M_WRMEM(eaddr,areg);
}

/* $98 EORA direct -**0- */
INLINE void eora_di( void )
{
	byte t;
	DIRBYTE(t); areg ^= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $99 ADCA direct ***** */
INLINE void adca_di( void )
{
	word t,r;
	DIRBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $9A ORA direct -**0- */
INLINE void ora_di( void )
{
	byte t;
	DIRBYTE(t); areg |= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $9B ADDA direct ***** */
INLINE void adda_di( void )
{
	word t,r;
	DIRBYTE(t); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $9C CMPX (CMPY CMPS) direct -**** */
INLINE void cmpx_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $109C CMPY direct -**** */
INLINE void cmpy_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = yreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $119C CMPS direct -**** */
INLINE void cmps_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = sreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $9D JSR direct ----- */
INLINE void jsr_di( void )
{
	DIRECT; PUSHWORD(pcreg);
	pcreg = eaddr;change_pc(pcreg);	/* TS 971002 */
}

/* $9E LDX (LDY) direct -**0- */
INLINE void ldx_di( void )
{
	DIRWORD(xreg);
	CLR_NZV; SET_NZ16(xreg);
}

/* $109E LDY direct -**0- */
INLINE void ldy_di( void )
{
	DIRWORD(yreg);
	CLR_NZV; SET_NZ16(yreg);
}

/* $9F STX (STY) direct -**0- */
INLINE void stx_di( void )
{
	CLR_NZV; SET_NZ16(xreg);
	DIRECT; M_WRMEM_WORD(eaddr,xreg);
}

/* $109F STY direct -**0- */
INLINE void sty_di( void )
{
	CLR_NZV; SET_NZ16(yreg);
	DIRECT; M_WRMEM_WORD(eaddr,yreg);
}

#if macintosh
#pragma mark ____Ax____
#endif


/* $a0 SUBA indexed ?**** */
INLINE void suba_ix( void )
{
	word	t,r;
	t = M_RDMEM(eaddr); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $a1 CMPA indexed ?**** */
INLINE void cmpa_ix( void )
{
	word	t,r;
	t = M_RDMEM(eaddr); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $a2 SBCA indexed ?**** */
INLINE void sbca_ix( void )
{
	word	t,r;
	t = M_RDMEM(eaddr); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $a3 SUBD (CMPD CMPU) indexed -**** */
INLINE void subd_ix( void )
{
	dword r,d,b;
	b = M_RDMEM_WORD(eaddr); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $10a3 CMPD indexed -**** */
INLINE void cmpd_ix( void )
{
	dword r,d,b;
	b = M_RDMEM_WORD(eaddr); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $11a3 CMPU indexed -**** */
INLINE void cmpu_ix( void )
{
	dword r,b;
	b = M_RDMEM_WORD(eaddr); r = ureg-b;
	CLR_NZVC; SET_FLAGS16(ureg,b,r);
}

/* $a4 ANDA indexed -**0- */
INLINE void anda_ix( void )
{
	areg &= M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(areg);
}

/* $a5 BITA indexed -**0- */
INLINE void bita_ix( void )
{
	byte r;
	r = areg&M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(r);
}

/* $a6 LDA indexed -**0- */
INLINE void lda_ix( void )
{
	areg = M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(areg);
}

/* $a7 STA indexed -**0- */
INLINE void sta_ix( void )
{
	CLR_NZV; SET_NZ8(areg);
	M_WRMEM(eaddr,areg);
}

/* $a8 EORA indexed -**0- */
INLINE void eora_ix( void )
{
	areg ^= M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(areg);
}

/* $a9 ADCA indexed ***** */
INLINE void adca_ix( void )
{
	word t,r;
	t = M_RDMEM(eaddr); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $aA ORA indexed -**0- */
INLINE void ora_ix( void )
{
	areg |= M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(areg);
}

/* $aB ADDA indexed ***** */
INLINE void adda_ix( void )
{
	word t,r;
	t = M_RDMEM(eaddr); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $aC CMPX (CMPY CMPS) indexed -**** */
INLINE void cmpx_ix( void )
{
	dword r,d,b;
	b = M_RDMEM_WORD(eaddr); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $10aC CMPY indexed -**** */
INLINE void cmpy_ix( void )
{
	dword r,d,b;
	b = M_RDMEM_WORD(eaddr); d = yreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $11aC CMPS indexed -**** */
INLINE void cmps_ix( void )
{
	dword r,d,b;
	b = M_RDMEM_WORD(eaddr); d = sreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $aD JSR indexed ----- */
INLINE void jsr_ix( void )
{
	PUSHWORD(pcreg);
	pcreg = eaddr;change_pc(pcreg);	/* TS 971002 */
}

/* $aE LDX (LDY) indexed -**0- */
INLINE void ldx_ix( void )
{
	xreg = M_RDMEM_WORD(eaddr);
	CLR_NZV; SET_NZ16(xreg);
}

/* $10aE LDY indexed -**0- */
INLINE void ldy_ix( void )
{
	yreg = M_RDMEM_WORD(eaddr);
	CLR_NZV; SET_NZ16(yreg);
}

/* $aF STX (STY) indexed -**0- */
INLINE void stx_ix( void )
{
	CLR_NZV; SET_NZ16(xreg);
	M_WRMEM_WORD(eaddr,xreg);
}

/* $10aF STY indexed -**0- */
INLINE void sty_ix( void )
{
	CLR_NZV; SET_NZ16(yreg);
	M_WRMEM_WORD(eaddr,yreg);
}

#if macintosh
#pragma mark ____Bx____
#endif

/* $b0 SUBA extended ?**** */
INLINE void suba_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $b1 CMPA extended ?**** */
INLINE void cmpa_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $b2 SBCA extended ?**** */
INLINE void sbca_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $b3 SUBD (CMPD CMPU) extended -**** */
INLINE void subd_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $10b3 CMPD extended -**** */
INLINE void cmpd_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $11b3 CMPU extended -**** */
INLINE void cmpu_ex( void )
{
	dword r,b;
	EXTWORD(b); r = ureg-b;
	CLR_NZVC; SET_FLAGS16(ureg,b,r);
}

/* $b4 ANDA extended -**0- */
INLINE void anda_ex( void )
{
	byte t;
	EXTBYTE(t); areg &= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $b5 BITA extended -**0- */
INLINE void bita_ex( void )
{
	byte t,r;
	EXTBYTE(t); r = areg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $b6 LDA extended -**0- */
INLINE void lda_ex( void )
{
	EXTBYTE(areg);
	CLR_NZV; SET_NZ8(areg);
}

/* $b7 STA extended -**0- */
INLINE void sta_ex( void )
{
	CLR_NZV; SET_NZ8(areg);
	EXTENDED; M_WRMEM(eaddr,areg);
}

/* $b8 EORA extended -**0- */
INLINE void eora_ex( void )
{
	byte t;
	EXTBYTE(t); areg ^= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $b9 ADCA extended ***** */
INLINE void adca_ex( void )
{
	word t,r;
	EXTBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $bA ORA extended -**0- */
INLINE void ora_ex( void )
{
	byte t;
	EXTBYTE(t); areg |= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $bB ADDA extended ***** */
INLINE void adda_ex( void )
{
	word t,r;
	EXTBYTE(t); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $bC CMPX (CMPY CMPS) extended -**** */
INLINE void cmpx_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $10bC CMPY extended -**** */
INLINE void cmpy_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = yreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $11bC CMPS extended -**** */
INLINE void cmps_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = sreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $bD JSR extended ----- */
INLINE void jsr_ex( void )
{
	EXTENDED; PUSHWORD(pcreg);
	pcreg = eaddr;change_pc(pcreg);	/* TS 971002 */
}

/* $bE LDX (LDY) extended -**0- */
INLINE void ldx_ex( void )
{
	EXTWORD(xreg);
	CLR_NZV; SET_NZ16(xreg);
}

/* $10bE LDY extended -**0- */
INLINE void ldy_ex( void )
{
	EXTWORD(yreg);
	CLR_NZV; SET_NZ16(yreg);
}

/* $bF STX (STY) extended -**0- */
INLINE void stx_ex( void )
{
	CLR_NZV; SET_NZ16(xreg);
	EXTENDED; M_WRMEM_WORD(eaddr,xreg);
}

/* $10bF STY extended -**0- */
INLINE void sty_ex( void )
{
	CLR_NZV; SET_NZ16(yreg);
	EXTENDED; M_WRMEM_WORD(eaddr,yreg);
}


#if macintosh
#pragma mark ____Cx____
#endif

/* $c0 SUBB immediate ?**** */
INLINE void subb_im( void )
{
	word	t,r;
	IMMBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $c1 CMPB immediate ?**** */
INLINE void cmpb_im( void )
{
	word	t,r;
	IMMBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $c2 SBCB immediate ?**** */
INLINE void sbcb_im( void )
{
	word	t,r;
	IMMBYTE(t); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $c3 ADDD immediate -**** */
INLINE void addd_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $c4 ANDB immediate -**0- */
INLINE void andb_im( void )
{
	byte t;
	IMMBYTE(t); breg &= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $c5 BITB immediate -**0- */
INLINE void bitb_im( void )
{
	byte t,r;
	IMMBYTE(t); r = breg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $c6 LDB immediate -**0- */
INLINE void ldb_im( void )
{
	IMMBYTE(breg);
	CLR_NZV; SET_NZ8(breg);
}

/* is this a legal instruction? */
/* $c7 STB immediate -**0- */
INLINE void stb_im( void )
{
	CLR_NZV; SET_NZ8(breg);
	IMM8; M_WRMEM(eaddr,breg);
}

/* $c8 EORB immediate -**0- */
INLINE void eorb_im( void )
{
	byte t;
	IMMBYTE(t); breg ^= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $c9 ADCB immediate ***** */
INLINE void adcb_im( void )
{
	word t,r;
	IMMBYTE(t); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $cA ORB immediate -**0- */
INLINE void orb_im( void )
{
	byte t;
	IMMBYTE(t); breg |= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $cB ADDB immediate ***** */
INLINE void addb_im( void )
{
	word t,r;
	IMMBYTE(t); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $cC LDD immediate -**0- */
INLINE void ldd_im( void )
{
	word t;
	IMMWORD(t); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* is this a legal instruction? */
/* $cD STD immediate -**0- */
INLINE void std_im( void )
{
	word t;
	IMM16; t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $cE LDU (LDS) immediate -**0- */
INLINE void ldu_im( void )
{
	IMMWORD(ureg);
	CLR_NZV; SET_NZ16(ureg);
}

/* $10cE LDS immediate -**0- */
INLINE void lds_im( void )
{
	IMMWORD(sreg);
	CLR_NZV; SET_NZ16(sreg);
}

/* is this a legal instruction? */
/* $cF STU (STS) immediate -**0- */
INLINE void stu_im( void )
{
	CLR_NZV; SET_NZ16(ureg);
	IMM16; M_WRMEM_WORD(eaddr,ureg);
}

/* is this a legal instruction? */
/* $10cF STS immediate -**0- */
INLINE void sts_im( void )
{
	CLR_NZV; SET_NZ16(sreg);
	IMM16; M_WRMEM_WORD(eaddr,sreg);
}


#if macintosh
#pragma mark ____Dx____
#endif

/* $d0 SUBB direct ?**** */
INLINE void subb_di( void )
{
	word	t,r;
	DIRBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $d1 CMPB direct ?**** */
INLINE void cmpb_di( void )
{
	word	t,r;
	DIRBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $d2 SBCB direct ?**** */
INLINE void sbcb_di( void )
{
	word	t,r;
	DIRBYTE(t); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $d3 ADDD direct -**** */
INLINE void addd_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $d4 ANDB direct -**0- */
INLINE void andb_di( void )
{
	byte t;
	DIRBYTE(t); breg &= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $d5 BITB direct -**0- */
INLINE void bitb_di( void )
{
	byte t,r;
	DIRBYTE(t); r = breg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $d6 LDB direct -**0- */
INLINE void ldb_di( void )
{
	DIRBYTE(breg);
	CLR_NZV; SET_NZ8(breg);
}

/* $d7 STB direct -**0- */
INLINE void stb_di( void )
{
	CLR_NZV; SET_NZ8(breg);
	DIRECT; M_WRMEM(eaddr,breg);
}

/* $d8 EORB direct -**0- */
INLINE void eorb_di( void )
{
	byte t;
	DIRBYTE(t); breg ^= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $d9 ADCB direct ***** */
INLINE void adcb_di( void )
{
	word t,r;
	DIRBYTE(t); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $dA ORB direct -**0- */
INLINE void orb_di( void )
{
	byte t;
	DIRBYTE(t); breg |= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $dB ADDB direct ***** */
INLINE void addb_di( void )
{
	word t,r;
	DIRBYTE(t); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $dC LDD direct -**0- */
INLINE void ldd_di( void )
{
	word t;
	DIRWORD(t); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* $dD STD direct -**0- */
INLINE void std_di( void )
{
	word t;
	DIRECT; t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $dE LDU (LDS) direct -**0- */
INLINE void ldu_di( void )
{
	DIRWORD(ureg);
	CLR_NZV; SET_NZ16(ureg);
}

/* $10dE LDS direct -**0- */
INLINE void lds_di( void )
{
	DIRWORD(sreg);
	CLR_NZV; SET_NZ16(sreg);
}

/* $dF STU (STS) direct -**0- */
INLINE void stu_di( void )
{
	CLR_NZV; SET_NZ16(ureg);
	DIRECT; M_WRMEM_WORD(eaddr,ureg);
}

/* $10dF STS direct -**0- */
INLINE void sts_di( void )
{
	CLR_NZV; SET_NZ16(sreg);
	DIRECT; M_WRMEM_WORD(eaddr,sreg);
}

#if macintosh
#pragma mark ____Ex____
#endif


/* $e0 SUBB indexed ?**** */
INLINE void subb_ix( void )
{
	word	t,r;
	t = M_RDMEM(eaddr); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $e1 CMPB indexed ?**** */
INLINE void cmpb_ix( void )
{
	word	t,r;
	t = M_RDMEM(eaddr); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $e2 SBCB indexed ?**** */
INLINE void sbcb_ix( void )
{
	word	t,r;
	t = M_RDMEM(eaddr); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $e3 ADDD indexed -**** */
INLINE void addd_ix( void )
{
	dword r,d,b;
	b = M_RDMEM_WORD(eaddr); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $e4 ANDB indexed -**0- */
INLINE void andb_ix( void )
{
	breg &= M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(breg);
}

/* $e5 BITB indexed -**0- */
INLINE void bitb_ix( void )
{
	byte r;
	r = breg&M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(r);
}

/* $e6 LDB indexed -**0- */
INLINE void ldb_ix( void )
{
	breg = M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(breg);
}

/* $e7 STB indexed -**0- */
INLINE void stb_ix( void )
{
	CLR_NZV; SET_NZ8(breg);
	M_WRMEM(eaddr,breg);
}

/* $e8 EORB indexed -**0- */
INLINE void eorb_ix( void )
{
	breg ^= M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(breg);
}

/* $e9 ADCB indexed ***** */
INLINE void adcb_ix( void )
{
	word t,r;
	t = M_RDMEM(eaddr); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $eA ORB indexed -**0- */
INLINE void orb_ix( void )
{
	breg |= M_RDMEM(eaddr);
	CLR_NZV; SET_NZ8(breg);
}

/* $eB ADDB indexed ***** */
INLINE void addb_ix( void )
{
	word t,r;
	t = M_RDMEM(eaddr); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $eC LDD indexed -**0- */
INLINE void ldd_ix( void )
{
	word t;
	t = M_RDMEM_WORD(eaddr); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* $eD STD indexed -**0- */
INLINE void std_ix( void )
{
	word t;
	t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $eE LDU (LDS) indexed -**0- */
INLINE void ldu_ix( void )
{
	ureg = M_RDMEM_WORD(eaddr);
	CLR_NZV; SET_NZ16(ureg);
}

/* $10eE LDS indexed -**0- */
INLINE void lds_ix( void )
{
	sreg = M_RDMEM_WORD(eaddr);
	CLR_NZV; SET_NZ16(sreg);
}

/* $eF STU (STS) indexed -**0- */
INLINE void stu_ix( void )
{
	CLR_NZV; SET_NZ16(ureg);
	M_WRMEM_WORD(eaddr,ureg);
}

/* $10eF STS indexed -**0- */
INLINE void sts_ix( void )
{
	CLR_NZV; SET_NZ16(sreg);
	M_WRMEM_WORD(eaddr,sreg);
}

#if macintosh
#pragma mark ____Fx____
#endif

/* $f0 SUBB extended ?**** */
INLINE void subb_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $f1 CMPB extended ?**** */
INLINE void cmpb_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $f2 SBCB extended ?**** */
INLINE void sbcb_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $f3 ADDD extended -**** */
INLINE void addd_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $f4 ANDB extended -**0- */
INLINE void andb_ex( void )
{
	byte t;
	EXTBYTE(t); breg &= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $f5 BITB extended -**0- */
INLINE void bitb_ex( void )
{
	byte t,r;
	EXTBYTE(t); r = breg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $f6 LDB extended -**0- */
INLINE void ldb_ex( void )
{
	EXTBYTE(breg);
	CLR_NZV; SET_NZ8(breg);
}

/* $f7 STB extended -**0- */
INLINE void stb_ex( void )
{
	CLR_NZV; SET_NZ8(breg);
	EXTENDED; M_WRMEM(eaddr,breg);
}

/* $f8 EORB extended -**0- */
INLINE void eorb_ex( void )
{
	byte t;
	EXTBYTE(t); breg ^= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $f9 ADCB extended ***** */
INLINE void adcb_ex( void )
{
	word t,r;
	EXTBYTE(t); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $fA ORB extended -**0- */
INLINE void orb_ex( void )
{
	byte t;
	EXTBYTE(t); breg |= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $fB ADDB extended ***** */
INLINE void addb_ex( void )
{
	word t,r;
	EXTBYTE(t); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $fC LDD extended -**0- */
INLINE void ldd_ex( void )
{
	word t;
	EXTWORD(t); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* $fD STD extended -**0- */
INLINE void std_ex( void )
{
	word t;
	EXTENDED; t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $fE LDU (LDS) extended -**0- */
INLINE void ldu_ex( void )
{
	EXTWORD(ureg);
	CLR_NZV; SET_NZ16(ureg);
}

/* $10fE LDS extended -**0- */
INLINE void lds_ex( void )
{
	EXTWORD(sreg);
	CLR_NZV; SET_NZ16(sreg);
}

/* $fF STU (STS) extended -**0- */
INLINE void stu_ex( void )
{
	CLR_NZV; SET_NZ16(ureg);
	EXTENDED; M_WRMEM_WORD(eaddr,ureg);
}

/* $10fF STS extended -**0- */
INLINE void sts_ex( void )
{
	CLR_NZV; SET_NZ16(sreg);
	EXTENDED; M_WRMEM_WORD(eaddr,sreg);
}
