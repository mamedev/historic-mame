/*

HNZVC

? = undefined
* = affected
- = unaffected
0 = cleared
1 = set
# = CCr directly affected by instruction
@ = special - carry set if bit 7 is set

*/

INLINE void illegal( void )
{
	logerror("HD6309: illegal opcode at %04x\nVectoring to [$fff0]\n",PC);

	CC |= CC_E; 				/* save entire state */
	PUSHWORD(pPC);
	PUSHWORD(pU);
	PUSHWORD(pY);
	PUSHWORD(pX);
	PUSHBYTE(DP);

	if ( MD & MD_EM )
	{
		PUSHBYTE(F);
		PUSHBYTE(E);
		hd6309.extra_cycles += 2; /* subtract +2 cycles */
	}

	PUSHBYTE(B);
	PUSHBYTE(A);
	PUSHBYTE(CC);
	hd6309.extra_cycles += 19;	/* subtract +19 cycles next time */

	PCD = RM16(0xfff0);
	CHANGE_PC;
}

#if macintosh
#pragma mark ____0x____
#endif

/* $00 NEG direct ?**** */
INLINE void neg_di( void )
{
	UINT16 r,t;
	DIRBYTE(t);
	r = -t;
	CLR_NZVC;
	SET_FLAGS8(0,t,r);
	WM(EAD,r);
}

/* $01 OIM direct ?**** */
INLINE void oim_di( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	DIRBYTE(t);
	r = im | t;
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $02 AIM direct */
INLINE void aim_di( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	DIRBYTE(t);
	r = im & t;
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $03 COM direct -**01 */
INLINE void com_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	t = ~t;
	CLR_NZV;
	SET_NZ8(t);
	SEC;
	WM(EAD,t);
}

/* $04 LSR direct -0*-* */
INLINE void lsr_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	CLR_NZC;
	CC |= (t & CC_C);
	t >>= 1;
	SET_Z8(t);
	WM(EAD,t);
}

/* $05 EIM direct */
INLINE void eim_di( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	DIRBYTE(t);
	r = im ^ t;
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $06 ROR direct -**-* */
INLINE void ror_di( void )
{
	UINT8 t,r;
	DIRBYTE(t);
	r= (CC & CC_C) << 7;
	CLR_NZC;
	CC |= (t & CC_C);
	r |= t>>1;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $07 ASR direct ?**-* */
INLINE void asr_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	CLR_NZC;
	CC |= (t & CC_C);
	t = (t & 0x80) | (t >> 1);
	SET_NZ8(t);
	WM(EAD,t);
}

/* $08 ASL direct ?**** */
INLINE void asl_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = t << 1;
	CLR_NZVC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $09 ROL direct -**** */
INLINE void rol_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = (CC & CC_C) | (t << 1);
	CLR_NZVC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $0A DEC direct -***- */
INLINE void dec_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	--t;
	CLR_NZV;
	SET_FLAGS8D(t);
	WM(EAD,t);
}

/* $0B TIM direct */
INLINE void tim_di( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	DIRBYTE(t);
	r = im & t;
	CLR_NZV;
	SET_NZ8(r);
}

/* $OC INC direct -***- */
INLINE void inc_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	++t;
	CLR_NZV;
	SET_FLAGS8I(t);
	WM(EAD,t);
}

/* $OD TST direct -**0- */
INLINE void tst_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	CLR_NZV;
	SET_NZ8(t);
}

/* $0E JMP direct ----- */
INLINE void jmp_di( void )
{
	DIRECT;
	PCD = EAD;
	CHANGE_PC;
}

/* $0F CLR direct -0100 */
INLINE void clr_di( void )
{
	DIRECT;
	WM(EAD,0);
	CLR_NZVC;
	SEZ;
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
	/* SYNC stops processing instructions until an interrupt request happens. */
	/* This doesn't require the corresponding interrupt to be enabled: if it */
	/* is disabled, execution continues with the next instruction. */
	hd6309.int_state |= HD6309_SYNC;	 /* HJB 990227 */
	CHECK_IRQ_LINES();
	/* if HD6309_SYNC has not been cleared by CHECK_IRQ_LINES(),
	 * stop execution until the interrupt lines change. */
	if( hd6309.int_state & HD6309_SYNC )
		if (hd6309_ICount > 0) hd6309_ICount = 0;
}

/* $14 sexw inherent */
INLINE void sexw( void )
{
	UINT32 t;
	t = SIGNED_16(W);
	D = t;
	CLR_NZV;
	SET_NZ32(t);
}

/* $15 ILLEGAL */

/* $16 LBRA relative ----- */
INLINE void lbra( void )
{
	IMMWORD(ea);
	PC += EA;
	CHANGE_PC;

	if ( EA == 0xfffd )  /* EHC 980508 speed up busy loop */
		if ( hd6309_ICount > 0)
			hd6309_ICount = 0;
}

/* $17 LBSR relative ----- */
INLINE void lbsr( void )
{
	IMMWORD(ea);
	PUSHWORD(pPC);
	PC += EA;
	CHANGE_PC;
}

/* $18 ILLEGAL */

#if 1
/* $19 DAA inherent (A) -**0* */
INLINE void daa( void )
{
	UINT8 msn, lsn;
	UINT16 t, cf = 0;
	msn = A & 0xf0; lsn = A & 0x0f;
	if( lsn>0x09 || CC & CC_H) cf |= 0x06;
	if( msn>0x80 && lsn>0x09 ) cf |= 0x60;
	if( msn>0x90 || CC & CC_C) cf |= 0x60;
	t = cf + A;
	CLR_NZV; /* keep carry from previous operation */
	SET_NZ8((UINT8)t); SET_C8(t);
	A = t;
}
#else
/* $19 DAA inherent (A) -**0* */
INLINE void daa( void )
{
	UINT16 t;
	t = A;
	if (CC & CC_H) t+=0x06;
	if ((t&0x0f)>9) t+=0x06;		/* ASG -- this code is broken! $66+$99=$FF -> DAA should = $65, we get $05! */
	if (CC & CC_C) t+=0x60;
	if ((t&0xf0)>0x90) t+=0x60;
	if (t&0x100) SEC;
	A = t;
}
#endif

/* $1A ORCC immediate ##### */
INLINE void orcc( void )
{
	UINT8 t;
	IMMBYTE(t);
	CC |= t;
	CHECK_IRQ_LINES();	/* HJB 990116 */
}

/* $1B ILLEGAL */

/* $1C ANDCC immediate ##### */
INLINE void andcc( void )
{
	UINT8 t;
	IMMBYTE(t);
	CC &= t;
	CHECK_IRQ_LINES();	/* HJB 990116 */
}

/* $1D SEX inherent -**0- */
INLINE void sex( void )
{
	UINT16 t;
	t = SIGNED(B);
	D = t;
	CLR_NZV;
	SET_NZ16(t);
}

INLINE void exg( void )
{
	UINT16 t1,t2;
	UINT8 tb;
	int 	promote = FALSE;

	IMMBYTE(tb);
	if( (tb^(tb>>4)) & 0x08 )	/* HJB 990225: mixed 8/16 bit case? */
	{
		promote = TRUE;
	}

	switch(tb>>4) {
		case  0: t1 = D;  break;
		case  1: t1 = X;  break;
		case  2: t1 = Y;  break;
		case  3: t1 = U;  break;
		case  4: t1 = S;  break;
		case  5: t1 = PC; break;
		case  6: t1 = W;  break;
		case  7: t1 = V;  break;
		case  8: t1 = (promote ? D : A);  break;
		case  9: t1 = (promote ? D : B);  break;
		case 10: t1 = CC; break;
		case 11: t1 = DP; break;
		case 12: t1 = 0;  break;
		case 13: t1 = 0;  break;
		case 14: t1 = (promote ? W : E ); break;
		default: t1 = (promote ? W : F ); break;
	}
	switch(tb&15) {
		case  0: t2 = D;  break;
		case  1: t2 = X;  break;
		case  2: t2 = Y;  break;
		case  3: t2 = U;  break;
		case  4: t2 = S;  break;
		case  5: t2 = PC; break;
		case  6: t2 = W;  break;
		case  7: t2 = V;  break;
		case  8: t2 = (promote ? D : A);  break;
		case  9: t2 = (promote ? D : B);  break;
		case 10: t2 = CC; break;
		case 11: t2 = DP; break;
		case 12: t2 = 0;  break;
		case 13: t2 = 0;  break;
		case 14: t2 = (promote ? W : E); break;
		default: t2 = (promote ? W : F); break;
	}

	switch(tb>>4) {
		case  0: D = t2;  break;
		case  1: X = t2;  break;
		case  2: Y = t2;  break;
		case  3: U = t2;  break;
		case  4: S = t2;  break;
		case  5: PC = t2; CHANGE_PC; break;
		case  6: W = t2;  break;
		case  7: V = t2;  break;
		case  8: if (promote) D = t2; else A = t2; break;
		case  9: if (promote) D = t2; else B = t2; break;
		case 10: CC = t2; break;
		case 11: DP = t2; break;
		case 12: /* 0 = t2 */ break;
		case 13: /* 0 = t2 */ break;
		case 14: if (promote) W = t2; else E = t2; break;
		case 15: if (promote) W = t2; else F = t2; break;
	}
	switch(tb&15) {
		case  0: D = t1;  break;
		case  1: X = t1;  break;
		case  2: Y = t1;  break;
		case  3: U = t1;  break;
		case  4: S = t1;  break;
		case  5: PC = t1; CHANGE_PC; break;
		case  6: W = t1;  break;
		case  7: V = t1;  break;
		case  8: if (promote) D = t1; else A = t1; break;
		case  9: if (promote) D = t1; else B = t1; break;
		case 10: CC = t1; break;
		case 11: DP = t1; break;
		case 12: /* 0 = t1 */ break;
		case 13: /* 0 = t1 */ break;
		case 14: if (promote) W = t1; else E = t1; break;
		case 15: if (promote) W = t1; else F = t1; break;
	}
}

/* $1F TFR inherent ----- */
INLINE void tfr( void )
{
	UINT8 tb;
	UINT16 t;
	int 	promote = FALSE;

	IMMBYTE(tb);
	if( (tb^(tb>>4)) & 0x08 )
	{
		promote = TRUE;
	}

	switch(tb>>4) {
		case  0: t = D;  break;
		case  1: t = X;  break;
		case  2: t = Y;  break;
		case  3: t = U;  break;
		case  4: t = S;  break;
		case  5: t = PC; break;
		case  6: t = W;  break;
		case  7: t = V;  break;
		case  8: t = (promote ? D : A );  break;
		case  9: t = (promote ? D : B );  break;
		case 10: t = CC; break;
		case 11: t = DP; break;
		case 12: t = 0;  break;
		case 13: t = 0;  break;
		case 14: t = (promote ? W : E ); break;
		default: t = (promote ? W : F ); break;
	}

	switch(tb&15) {
		case  0: D = t;  break;
		case  1: X = t;  break;
		case  2: Y = t;  break;
		case  3: U = t;  break;
		case  4: S = t;  break;
		case  5: PC = t; CHANGE_PC; break;
		case  6: W = t;  break;
		case  7: V = t;  break;
		case  8: if (promote) D = t; else A = t; break;
		case  9: if (promote) D = t; else B = t; break;
		case 10: CC = t; break;
		case 11: DP = t; break;
		case 12: /* 0 = t1 */ break;
		case 13: /* 0 = t1 */ break;
		case 14: if (promote) W = t; else E = t; break;
		case 15: if (promote) W = t; else F = t; break;
	}
}

#if macintosh
#pragma mark ____2x____
#endif

/* $20 BRA relative ----- */
INLINE void bra( void )
{
	UINT8 t;
	IMMBYTE(t);
	PC += SIGNED(t);
	CHANGE_PC;
	/* JB 970823 - speed up busy loops */
	if( t == 0xfe )
		if( hd6309_ICount > 0 ) hd6309_ICount = 0;
}

/* $21 BRN relative ----- */
INLINE void brn( void )
{
	UINT8 t;
	IMMBYTE(t);
}

/* $1021 LBRN relative ----- */
INLINE void lbrn( void )
{
	IMMWORD(ea);
}

/* $22 BHI relative ----- */
INLINE void bhi( void )
{
	BRANCH( !(CC & (CC_Z|CC_C)) );
}

/* $1022 LBHI relative ----- */
INLINE void lbhi( void )
{
	LBRANCH( !(CC & (CC_Z|CC_C)) );
}

/* $23 BLS relative ----- */
INLINE void bls( void )
{
	BRANCH( (CC & (CC_Z|CC_C)) );
}

/* $1023 LBLS relative ----- */
INLINE void lbls( void )
{
	LBRANCH( (CC&(CC_Z|CC_C)) );
}

/* $24 BCC relative ----- */
INLINE void bcc( void )
{
	BRANCH( !(CC&CC_C) );
}

/* $1024 LBCC relative ----- */
INLINE void lbcc( void )
{
	LBRANCH( !(CC&CC_C) );
}

/* $25 BCS relative ----- */
INLINE void bcs( void )
{
	BRANCH( (CC&CC_C) );
}

/* $1025 LBCS relative ----- */
INLINE void lbcs( void )
{
	LBRANCH( (CC&CC_C) );
}

/* $26 BNE relative ----- */
INLINE void bne( void )
{
	BRANCH( !(CC&CC_Z) );
}

/* $1026 LBNE relative ----- */
INLINE void lbne( void )
{
	LBRANCH( !(CC&CC_Z) );
}

/* $27 BEQ relative ----- */
INLINE void beq( void )
{
	BRANCH( (CC&CC_Z) );
}

/* $1027 LBEQ relative ----- */
INLINE void lbeq( void )
{
	LBRANCH( (CC&CC_Z) );
}

/* $28 BVC relative ----- */
INLINE void bvc( void )
{
	BRANCH( !(CC&CC_V) );
}

/* $1028 LBVC relative ----- */
INLINE void lbvc( void )
{
	LBRANCH( !(CC&CC_V) );
}

/* $29 BVS relative ----- */
INLINE void bvs( void )
{
	BRANCH( (CC&CC_V) );
}

/* $1029 LBVS relative ----- */
INLINE void lbvs( void )
{
	LBRANCH( (CC&CC_V) );
}

/* $2A BPL relative ----- */
INLINE void bpl( void )
{
	BRANCH( !(CC&CC_N) );
}

/* $102A LBPL relative ----- */
INLINE void lbpl( void )
{
	LBRANCH( !(CC&CC_N) );
}

/* $2B BMI relative ----- */
INLINE void bmi( void )
{
	BRANCH( (CC&CC_N) );
}

/* $102B LBMI relative ----- */
INLINE void lbmi( void )
{
	LBRANCH( (CC&CC_N) );
}

/* $2C BGE relative ----- */
INLINE void bge( void )
{
	BRANCH( !NXORV );
}

/* $102C LBGE relative ----- */
INLINE void lbge( void )
{
	LBRANCH( !NXORV );
}

/* $2D BLT relative ----- */
INLINE void blt( void )
{
	BRANCH( NXORV );
}

/* $102D LBLT relative ----- */
INLINE void lblt( void )
{
	LBRANCH( NXORV );
}

/* $2E BGT relative ----- */
INLINE void bgt( void )
{
	BRANCH( !(NXORV || (CC&CC_Z)) );
}

/* $102E LBGT relative ----- */
INLINE void lbgt( void )
{
	LBRANCH( !(NXORV || (CC&CC_Z)) );
}

/* $2F BLE relative ----- */
INLINE void ble( void )
{
	BRANCH( (NXORV || (CC&CC_Z)) );
}

/* $102F LBLE relative ----- */
INLINE void lble( void )
{
	LBRANCH( (NXORV || (CC&CC_Z)) );
}

#if macintosh
#pragma mark ____3x____
#endif

#define REG_TO_REG( immediate ) 						\
{														\
	switch( immediate ) 								\
	{													\
		case 0x00:	MATH16(D,D);				break;	\
		case 0x01:	MATH16(D,X);				break;	\
		case 0x02:	MATH16(D,Y);				break;	\
		case 0x03:	MATH16(D,U);				break;	\
		case 0x04:	MATH16(D,S);				break;	\
		case 0x05:	MATH16(D,PC); CHANGE_PC;	break;	\
		case 0x06:	MATH16(D,W);				break;	\
		case 0x07:	MATH16(D,V);				break;	\
		case 0x08:	MATH16(D,D);				break;	\
		case 0x09:	MATH16(D,D);				break;	\
		case 0x0a:	MATH16(D,CC);				break;	\
		case 0x0b:	MATH16(D,DP);				break;	\
		case 0x0c:	MATH16(D,zero); 			break;	\
		case 0x0d:	MATH16(D,zero); 			break;	\
		case 0x0e:	MATH16(D,W);				break;	\
		case 0x0f:	MATH16(D,W);				break;	\
		case 0x10:	MATH16(X,D);				break;	\
		case 0x11:	MATH16(X,X);				break;	\
		case 0x12:	MATH16(X,Y);				break;	\
		case 0x13:	MATH16(X,U);				break;	\
		case 0x14:	MATH16(X,S);				break;	\
		case 0x15:	MATH16(X,PC); CHANGE_PC;	break;	\
		case 0x16:	MATH16(X,W);				break;	\
		case 0x17:	MATH16(X,V);				break;	\
		case 0x18:	MATH16(X,D);				break;	\
		case 0x19:	MATH16(X,D);				break;	\
		case 0x1a:	MATH16(X,CC);				break;	\
		case 0x1b:	MATH16(X,DP);				break;	\
		case 0x1c:	MATH16(X,zero); 			break;	\
		case 0x1d:	MATH16(X,zero); 			break;	\
		case 0x1e:	MATH16(X,W);				break;	\
		case 0x1f:	MATH16(X,W);				break;	\
		case 0x20:	MATH16(Y,D);				break;	\
		case 0x21:	MATH16(Y,X);				break;	\
		case 0x22:	MATH16(Y,Y);				break;	\
		case 0x23:	MATH16(Y,U);				break;	\
		case 0x24:	MATH16(Y,S);				break;	\
		case 0x25:	MATH16(Y,PC); CHANGE_PC;	break;	\
		case 0x26:	MATH16(Y,W);				break;	\
		case 0x27:	MATH16(Y,V);				break;	\
		case 0x28:	MATH16(Y,D);				break;	\
		case 0x29:	MATH16(Y,D);				break;	\
		case 0x2a:	MATH16(Y,CC);				break;	\
		case 0x2b:	MATH16(Y,DP);				break;	\
		case 0x2c:	MATH16(Y,zero); 			break;	\
		case 0x2d:	MATH16(Y,zero); 			break;	\
		case 0x2e:	MATH16(Y,W);				break;	\
		case 0x2f:	MATH16(Y,W);				break;	\
		case 0x30:	MATH16(U,D);				break;	\
		case 0x31:	MATH16(U,X);				break;	\
		case 0x32:	MATH16(U,Y);				break;	\
		case 0x33:	MATH16(U,U);				break;	\
		case 0x34:	MATH16(U,S);				break;	\
		case 0x35:	MATH16(U,PC); CHANGE_PC;	break;	\
		case 0x36:	MATH16(U,W);				break;	\
		case 0x37:	MATH16(U,V);				break;	\
		case 0x38:	MATH16(U,D);				break;	\
		case 0x39:	MATH16(U,D);				break;	\
		case 0x3a:	MATH16(U,CC);				break;	\
		case 0x3b:	MATH16(U,DP);				break;	\
		case 0x3c:	MATH16(U,zero); 			break;	\
		case 0x3d:	MATH16(U,zero); 			break;	\
		case 0x3e:	MATH16(U,W);				break;	\
		case 0x3f:	MATH16(U,W);				break;	\
		case 0x40:	MATH16(S,D);				break;	\
		case 0x41:	MATH16(S,X);				break;	\
		case 0x42:	MATH16(S,Y);				break;	\
		case 0x43:	MATH16(S,U);				break;	\
		case 0x44:	MATH16(S,S);				break;	\
		case 0x45:	MATH16(S,PC); CHANGE_PC;	break;	\
		case 0x46:	MATH16(S,W);				break;	\
		case 0x47:	MATH16(S,V);				break;	\
		case 0x48:	MATH16(S,D);				break;	\
		case 0x49:	MATH16(S,D);				break;	\
		case 0x4a:	MATH16(S,CC);				break;	\
		case 0x4b:	MATH16(S,DP);				break;	\
		case 0x4c:	MATH16(S,zero); 			break;	\
		case 0x4d:	MATH16(S,zero); 			break;	\
		case 0x4e:	MATH16(S,W);				break;	\
		case 0x4f:	MATH16(S,W);				break;	\
		case 0x50:	MATH16(PC,D);				break;	\
		case 0x51:	MATH16(PC,X);				break;	\
		case 0x52:	MATH16(PC,Y);				break;	\
		case 0x53:	MATH16(PC,U);				break;	\
		case 0x54:	MATH16(PC,S);				break;	\
		case 0x55:	MATH16(PC,PC); CHANGE_PC;	break;	\
		case 0x56:	MATH16(PC,W);				break;	\
		case 0x57:	MATH16(PC,V);				break;	\
		case 0x58:	MATH16(PC,D);				break;	\
		case 0x59:	MATH16(PC,D);				break;	\
		case 0x5a:	MATH16(PC,CC);				break;	\
		case 0x5b:	MATH16(PC,DP);				break;	\
		case 0x5c:	MATH16(PC,zero);			break;	\
		case 0x5d:	MATH16(PC,zero);			break;	\
		case 0x5e:	MATH16(PC,W);				break;	\
		case 0x5f:	MATH16(PC,W);				break;	\
		case 0x60:	MATH16(W,D);				break;	\
		case 0x61:	MATH16(W,X);				break;	\
		case 0x62:	MATH16(W,Y);				break;	\
		case 0x63:	MATH16(W,U);				break;	\
		case 0x64:	MATH16(W,S);				break;	\
		case 0x65:	MATH16(W,PC); CHANGE_PC;	break;	\
		case 0x66:	MATH16(W,W);				break;	\
		case 0x67:	MATH16(W,V);				break;	\
		case 0x68:	MATH16(W,D);				break;	\
		case 0x69:	MATH16(W,D);				break;	\
		case 0x6a:	MATH16(W,CC);				break;	\
		case 0x6b:	MATH16(W,DP);				break;	\
		case 0x6c:	MATH16(W,zero); 			break;	\
		case 0x6d:	MATH16(W,zero); 			break;	\
		case 0x6e:	MATH16(W,W);				break;	\
		case 0x6f:	MATH16(W,W);				break;	\
		case 0x70:	MATH16(V,D);				break;	\
		case 0x71:	MATH16(V,X);				break;	\
		case 0x72:	MATH16(V,Y);				break;	\
		case 0x73:	MATH16(V,U);				break;	\
		case 0x74:	MATH16(V,S);				break;	\
		case 0x75:	MATH16(V,PC); CHANGE_PC;	break;	\
		case 0x76:	MATH16(V,W);				break;	\
		case 0x77:	MATH16(V,V);				break;	\
		case 0x78:	MATH16(V,D);				break;	\
		case 0x79:	MATH16(V,D);				break;	\
		case 0x7a:	MATH16(V,CC);				break;	\
		case 0x7b:	MATH16(V,DP);				break;	\
		case 0x7c:	MATH16(V,zero); 			break;	\
		case 0x7d:	MATH16(V,zero); 			break;	\
		case 0x7e:	MATH16(V,W);				break;	\
		case 0x7f:	MATH16(V,W);				break;	\
		case 0x80:	MATH16(D,D);				break;	\
		case 0x81:	MATH16(D,X);				break;	\
		case 0x82:	MATH16(D,Y);				break;	\
		case 0x83:	MATH16(D,U);				break;	\
		case 0x84:	MATH16(D,S);				break;	\
		case 0x85:	MATH16(D,PC); CHANGE_PC;	break;	\
		case 0x86:	MATH16(D,W);				break;	\
		case 0x87:	MATH16(D,V);				break;	\
		case 0x88:	MATH8(A,A); 				break;	\
		case 0x89:	MATH8(A,B); 				break;	\
		case 0x8a:	MATH8(A,CC);				break;	\
		case 0x8b:	MATH8(A,DP);				break;	\
		case 0x8c:	MATH16(A,zero); 			break;	\
		case 0x8d:	MATH16(A,zero); 			break;	\
		case 0x8e:	MATH8(A,E); 				break;	\
		case 0x8f:	MATH8(A,F); 				break;	\
		case 0x90:	MATH16(D,D);				break;	\
		case 0x91:	MATH16(D,X);				break;	\
		case 0x92:	MATH16(D,Y);				break;	\
		case 0x93:	MATH16(D,U);				break;	\
		case 0x94:	MATH16(D,S);				break;	\
		case 0x95:	MATH16(D,PC); CHANGE_PC;	break;	\
		case 0x96:	MATH16(D,W);				break;	\
		case 0x97:	MATH16(D,V);				break;	\
		case 0x98:	MATH8(B,A); 				break;	\
		case 0x99:	MATH8(B,B); 				break;	\
		case 0x9a:	MATH8(B,CC);				break;	\
		case 0x9b:	MATH8(B,DP);				break;	\
		case 0x9c:	MATH16(B,zero); 			break;	\
		case 0x9d:	MATH16(B,zero); 			break;	\
		case 0x9e:	MATH8(B,E); 				break;	\
		case 0x9f:	MATH8(B,F); 				break;	\
		case 0xa0:	MATH16(CC,D);				break;	\
		case 0xa1:	MATH16(CC,X);				break;	\
		case 0xa2:	MATH16(CC,Y);				break;	\
		case 0xa3:	MATH16(CC,U);				break;	\
		case 0xa4:	MATH16(CC,S);				break;	\
		case 0xa5:	MATH16(CC,PC); CHANGE_PC;	break;	\
		case 0xa6:	MATH16(CC,W);				break;	\
		case 0xa7:	MATH16(CC,V);				break;	\
		case 0xa8:	MATH8(CC,A);				break;	\
		case 0xa9:	MATH8(CC,B);				break;	\
		case 0xaa:	MATH8(CC,CC);				break;	\
		case 0xab:	MATH8(CC,DP);				break;	\
		case 0xac:	MATH16(CC,zero);			break;	\
		case 0xad:	MATH16(CC,zero);			break;	\
		case 0xae:	MATH8(CC,E);				break;	\
		case 0xaf:	MATH8(CC,F);				break;	\
		case 0xb0:	MATH16(DP,D);				break;	\
		case 0xb1:	MATH16(DP,X);				break;	\
		case 0xb2:	MATH16(DP,Y);				break;	\
		case 0xb3:	MATH16(DP,U);				break;	\
		case 0xb4:	MATH16(DP,S);				break;	\
		case 0xb5:	MATH16(DP,PC); CHANGE_PC;	break;	\
		case 0xb6:	MATH16(DP,W);				break;	\
		case 0xb7:	MATH16(DP,V);				break;	\
		case 0xb8:	MATH8(DP,A);				break;	\
		case 0xb9:	MATH8(DP,B);				break;	\
		case 0xba:	MATH8(DP,CC);				break;	\
		case 0xbb:	MATH8(DP,DP);				break;	\
		case 0xbc:	MATH16(DP,zero);			break;	\
		case 0xbd:	MATH16(DP,zero);			break;	\
		case 0xbe:	MATH8(DP,E);				break;	\
		case 0xbf:	MATH8(DP,F);				break;	\
		case 0xc0:	MATH16(0,D);				break;	\
		case 0xc1:	MATH16(0,X);				break;	\
		case 0xc2:	MATH16(0,Y);				break;	\
		case 0xc3:	MATH16(0,U);				break;	\
		case 0xc4:	MATH16(0,S);				break;	\
		case 0xc5:	MATH16(0,PC); CHANGE_PC;	break;	\
		case 0xc6:	MATH16(0,W);				break;	\
		case 0xc7:	MATH16(0,V);				break;	\
		case 0xc8:	MATH8(0,A); 				break;	\
		case 0xc9:	MATH8(0,B); 				break;	\
		case 0xca:	MATH8(0,CC);				break;	\
		case 0xcb:	MATH8(0,DP);				break;	\
		case 0xcc:	MATH16(0,zero); 			break;	\
		case 0xcd:	MATH16(0,zero); 			break;	\
		case 0xce:	MATH8(0,E); 				break;	\
		case 0xcf:	MATH8(0,F); 				break;	\
		case 0xd0:	MATH16(0,D);				break;	\
		case 0xd1:	MATH16(0,X);				break;	\
		case 0xd2:	MATH16(0,Y);				break;	\
		case 0xd3:	MATH16(0,U);				break;	\
		case 0xd4:	MATH16(0,S);				break;	\
		case 0xd5:	MATH16(0,PC); CHANGE_PC;	break;	\
		case 0xd6:	MATH16(0,W);				break;	\
		case 0xd7:	MATH16(0,V);				break;	\
		case 0xd8:	MATH8(0,A); 				break;	\
		case 0xd9:	MATH8(0,B); 				break;	\
		case 0xda:	MATH8(0,CC);				break;	\
		case 0xdb:	MATH8(0,DP);				break;	\
		case 0xdc:	MATH16(0,zero); 			break;	\
		case 0xdd:	MATH16(0,zero); 			break;	\
		case 0xde:	MATH8(0,E); 				break;	\
		case 0xdf:	MATH8(0,F); 				break;	\
		case 0xe0:	MATH16(W,D);				break;	\
		case 0xe1:	MATH16(W,X);				break;	\
		case 0xe2:	MATH16(W,Y);				break;	\
		case 0xe3:	MATH16(W,U);				break;	\
		case 0xe4:	MATH16(W,S);				break;	\
		case 0xe5:	MATH16(W,PC); CHANGE_PC;	break;	\
		case 0xe6:	MATH16(W,W);				break;	\
		case 0xe7:	MATH16(W,V);				break;	\
		case 0xe8:	MATH8(E,A); 				break;	\
		case 0xe9:	MATH8(E,B); 				break;	\
		case 0xea:	MATH8(E,CC);				break;	\
		case 0xeb:	MATH8(E,DP);				break;	\
		case 0xec:	MATH16(E,zero); 			break;	\
		case 0xed:	MATH16(E,zero); 			break;	\
		case 0xee:	MATH8(E,E); 				break;	\
		case 0xef:	MATH8(E,F); 				break;	\
		case 0xf0:	MATH16(W,D);				break;	\
		case 0xf1:	MATH16(W,X);				break;	\
		case 0xf2:	MATH16(W,Y);				break;	\
		case 0xf3:	MATH16(W,U);				break;	\
		case 0xf4:	MATH16(W,S);				break;	\
		case 0xf5:	MATH16(W,PC); CHANGE_PC;	break;	\
		case 0xf6:	MATH16(W,W);				break;	\
		case 0xf7:	MATH16(W,V);				break;	\
		case 0xf8:	MATH8(F,A); 				break;	\
		case 0xf9:	MATH8(F,B); 				break;	\
		case 0xfa:	MATH8(F,CC);				break;	\
		case 0xfb:	MATH8(F,DP);				break;	\
		case 0xfc:	MATH16(F,zero); 			break;	\
		case 0xfd:	MATH16(F,zero); 			break;	\
		case 0xfe:	MATH8(F,E); 				break;	\
		case 0xff:	MATH8(F,F); 				break;	\
		}												\
}

/* $1030 addr_r r1 + r2 -> r2 */

INLINE void addr_r( void )
{
	UINT8	tb, z8 = 0;
	UINT16	z16 = 0, r8;
	UINT32	r16;
	UINT8	*src8Reg = NULL, *dst8Reg = NULL;
	UINT16	*src16Reg = NULL, *dst16Reg = NULL;
	int 	promote = FALSE, large = FALSE;


	IMMBYTE(tb);
	if( (tb^(tb>>4)) & 0x08 )	/* HJB 990225: mixed 8/16 bit case? */
	{
		promote = TRUE;
	}

	switch(tb>>4) {
		case  0: src16Reg = &D; large = TRUE;  break;
		case  1: src16Reg = &X; large = TRUE;  break;
		case  2: src16Reg = &Y; large = TRUE;  break;
		case  3: src16Reg = &U; large = TRUE;  break;
		case  4: src16Reg = &S; large = TRUE;  break;
		case  5: src16Reg = &PC; large = TRUE; break;
		case  6: src16Reg = &W; large = TRUE;  break;
		case  7: src16Reg = &V; large = TRUE;  break;
		case  8: if (promote) src16Reg = &D; else src8Reg = &A; break;
		case  9: if (promote) src16Reg = &D; else src8Reg = &B; break;
		case 10: if (promote) src16Reg = &z16; else src8Reg = &CC; break; /* I dont know if this is the correct promotion */
		case 11: if (promote) src16Reg = &z16; else src8Reg = &DP; break; /* I dont know if this is the correct promotion */
		case 12: if (promote) src16Reg = &z16; else src8Reg = &z8; break;
		case 13: if (promote) src16Reg = &z16; else src8Reg = &z8; break;
		case 14: if (promote) src16Reg = &W; else src8Reg = &E; break;
		default: if (promote) src16Reg = &W; else src8Reg = &F; break;
	}

	switch(tb&15) {
		case  0: dst16Reg = &D; large = TRUE;  break;
		case  1: dst16Reg = &X; large = TRUE;  break;
		case  2: dst16Reg = &Y; large = TRUE;  break;
		case  3: dst16Reg = &U; large = TRUE;  break;
		case  4: dst16Reg = &S; large = TRUE;  break;
		case  5: dst16Reg = &PC; large = TRUE; break;
		case  6: dst16Reg = &W; large = TRUE;  break;
		case  7: dst16Reg = &V; large = TRUE;  break;
		case  8: if (promote) dst16Reg = &D; else dst8Reg = &A; break;
		case  9: if (promote) dst16Reg = &D; else dst8Reg = &B; break;
		case 10: if (promote) dst16Reg = &z16; else dst8Reg = &CC; break; /* I dont know if this is the correct promotion */
		case 11: if (promote) dst16Reg = &z16; else dst8Reg = &DP; break; /* I dont know if this is the correct promotion */
		case 12: if (promote) dst16Reg = &z16; else dst8Reg = &z8; break;
		case 13: if (promote) dst16Reg = &z16; else dst8Reg = &z8; break;
		case 14: if (promote) dst16Reg = &W; else dst8Reg = &E; break;
		default: if (promote) dst16Reg = &W; else dst8Reg = &F; break;
	}


	if ( large )
	{
		r16 = *src16Reg + *dst16Reg;
		CLR_HNZVC;
		SET_FLAGS16(*src16Reg,*dst16Reg,r16);
		*dst16Reg = r16;

		if ( (tb&15) == 5 )
		{
			CHANGE_PC;
		}
	}
	else
	{
		r8 = *src8Reg + *dst8Reg;
		CLR_HNZVC;
		SET_FLAGS8(*src8Reg,*dst8Reg,r8);
		/* SET_H(*src8Reg,*src8Reg,r8);*/ /*Experimentation prooved this not to be the case */
		*dst8Reg = r8;
	}

}

#define MATH16( r1, r2 )			\
{									\
	r16 = r1 + r2 + (CC & CC_C);	\
	CLR_HNZVC;						\
	SET_FLAGS16(r1,r1,r16); 		\
	r2 = r16;						\
}

#define MATH8( r1, r2 ) 			\
{									\
	r8 = r1 + r2 + (CC & CC_C); 	\
	CLR_HNZVC;						\
	SET_FLAGS8(r1,r2,r8);			\
	SET_H(r1,r2,r8);				\
	r2 = r8;						\
}

INLINE void adcr( void )
{
	UINT16 t,r8, zero=0;
	UINT32	r16;

	IMMBYTE(t);

	REG_TO_REG( t );
}
#undef MATH16
#undef MATH8

/* $1032 SUBR r1 - r2 -> r2 */
#define MATH16( r1, r2 )					\
{											\
	r16 = (UINT32)r2 - (UINT32)r1;			\
	CLR_NZVC;								\
	SET_FLAGS16((UINT32)r2,(UINT32)r1,r16); \
	r2 = r16;								\
}

#define MATH8( r1, r2 ) 			\
{									\
	r8 = r2 - r1;					\
	CLR_NZVC;						\
	SET_FLAGS8(r2,r1,r8);			\
	r2 = r8;						\
}

INLINE void subr( void )
{
	UINT16 t,r8, zero=0;
	UINT32	r16;

	IMMBYTE(t);

	REG_TO_REG( t );
}
#undef MATH16
#undef MATH8

/* $1033 SBCR r1 - r2 - C -> r2 */
#define MATH16( r1, r2 )							\
{													\
	r16 = (UINT32)r2 - (UINT32)r1 - (CC & CC_C);	\
	CLR_NZVC;										\
	SET_FLAGS16((UINT32)r2,(UINT32)r1,r16); 		\
	r2 = r16;										\
}

#define MATH8( r1, r2 ) 			\
{									\
	r8 = r2 - r1 - (CC & CC_C); 	\
	CLR_NZVC;						\
	SET_FLAGS8(r2,r1,r8);			\
	r2 = r8;						\
}

INLINE void sbcr( void )
{
	UINT16 t,r8, zero=0;
	UINT32	r16;

	IMMBYTE(t);

	REG_TO_REG( t );
}
#undef MATH16
#undef MATH8

/* $1034 ANDR r1 & r2 -> r2 */
#define MATH16( r1, r2 )			\
{									\
	r16 = r1 & r2;					\
	CLR_NZV;						\
	SET_NZ16(r16);					\
	r2 = r16;						\
}

#define MATH8( r1, r2 ) 			\
{									\
	r8 = r1 & r2;					\
	CLR_NZV;						\
	SET_NZ8(r8);					\
	r2 = r8;						\
}

INLINE void andr( void )
{
	UINT16 t,r8, zero=0;
	UINT32	r16;

	IMMBYTE(t);

	REG_TO_REG( t );
}
#undef MATH16
#undef MATH8

/* $1035 ORR r1 | r2 -> r2 */
#define MATH16( r1, r2 )			\
{									\
	r16 = r1 | r2;					\
	CLR_NZV;						\
	SET_NZ16(r16);					\
	r2 = r16;						\
}

#define MATH8( r1, r2 ) 			\
{									\
	r8 = r1 | r2;					\
	CLR_NZV;						\
	SET_NZ8(r8);					\
	r2 = r8;						\
}

INLINE void orr( void )
{
	UINT16 t,r8, zero=0;
	UINT32	r16;

	IMMBYTE(t);

	REG_TO_REG( t );
}
#undef MATH16
#undef MATH8

/* $1036 EORR r1 ^ r2 -> r2 */
#define MATH16( r1, r2 )			\
{									\
	r16 = r1 ^ r2;					\
	CLR_NZV;						\
	SET_NZ16(r16);					\
	r2 = r16;						\
}

#define MATH8( r1, r2 ) 			\
{									\
	r8 = r1 ^ r2;					\
	CLR_NZV;						\
	SET_NZ8(r8);					\
	r2 = r8;						\
}

INLINE void eorr( void )
{
	UINT16 t,r8, zero=0;
	UINT32	r16;

	IMMBYTE(t);

	REG_TO_REG( t );
}
#undef MATH16
#undef MATH8

/* $1037 CMPR r1 - r2 */
#define MATH16( r1, r2 )					\
{											\
	r16 = (UINT32)r2 - (UINT32)r1;			\
	CLR_NZVC;								\
	SET_FLAGS16((UINT32)r2,(UINT32)r1,r16); \
}

#define MATH8( r1, r2 ) 			\
{									\
	r8 = r2 - r1;					\
	CLR_NZVC;						\
	SET_FLAGS8(r2,r1,r8);			\
}

INLINE void cmpr( void )
{
	UINT16 t,r8, zero=0;
	UINT32	r16;

	IMMBYTE(t);

	REG_TO_REG( t );
}
#undef MATH16
#undef MATH8

/* $1138 TFR R0+,R1+ */
INLINE void tfmpp( void )
{
IlegalInstructionError;
/* tfmpp unimplemented */
}

/* $1139 TFR R0-,R1- */
INLINE void tfmmm( void )
{
IlegalInstructionError;
/* #warning tfmmm unimplemented */
}

/* $113A TFR R0+,R1 */
INLINE void tfmpc( void )
{
IlegalInstructionError;
/* #warning tfmpc unimplemented */
}

/* $113B TFR R0,R1+ */
INLINE void tfmcp( void )
{
IlegalInstructionError;
/* #warning tfmcp unimplemented */
}

/* $30 LEAX indexed --*-- */
INLINE void leax( void )
{
	fetch_effective_address();
	X = EA;
	CLR_Z;
	SET_Z(X);
}

/* $31 LEAY indexed --*-- */
INLINE void leay( void )
{
	fetch_effective_address();
	Y = EA;
	CLR_Z;
	SET_Z(Y);
}

/* $32 LEAS indexed ----- */
INLINE void leas( void )
{
	fetch_effective_address();
	S = EA;
	hd6309.int_state |= HD6309_LDS;
}

/* $33 LEAU indexed ----- */
INLINE void leau( void )
{
	fetch_effective_address();
	U = EA;
}

/* $34 PSHS inherent ----- */
INLINE void pshs( void )
{
	UINT8 t;
	IMMBYTE(t);
	if( t&0x80 ) { PUSHWORD(pPC); hd6309_ICount -= 2; }
	if( t&0x40 ) { PUSHWORD(pU);  hd6309_ICount -= 2; }
	if( t&0x20 ) { PUSHWORD(pY);  hd6309_ICount -= 2; }
	if( t&0x10 ) { PUSHWORD(pX);  hd6309_ICount -= 2; }
	if( t&0x08 ) { PUSHBYTE(DP);  hd6309_ICount -= 1; }
	if( t&0x04 ) { PUSHBYTE(B);   hd6309_ICount -= 1; }
	if( t&0x02 ) { PUSHBYTE(A);   hd6309_ICount -= 1; }
	if( t&0x01 ) { PUSHBYTE(CC);  hd6309_ICount -= 1; }
}

/* $1038 PSHSW inherent ----- */
INLINE void pshsw( void )
{
	PUSHWORD(pW);
}

/* $103a PSHUW inherent ----- */
INLINE void pshuw( void )
{
	PSHUWORD(pW);
}

/* $35 PULS inherent ----- */
INLINE void puls( void )
{
	UINT8 t;
	IMMBYTE(t);
	if( t&0x01 ) { PULLBYTE(CC); hd6309_ICount -= 1; }
	if( t&0x02 ) { PULLBYTE(A);  hd6309_ICount -= 1; }
	if( t&0x04 ) { PULLBYTE(B);  hd6309_ICount -= 1; }
	if( t&0x08 ) { PULLBYTE(DP); hd6309_ICount -= 1; }
	if( t&0x10 ) { PULLWORD(XD); hd6309_ICount -= 2; }
	if( t&0x20 ) { PULLWORD(YD); hd6309_ICount -= 2; }
	if( t&0x40 ) { PULLWORD(UD); hd6309_ICount -= 2; }
	if( t&0x80 ) { PULLWORD(PCD); CHANGE_PC; hd6309_ICount -= 2; }

	/* HJB 990225: moved check after all PULLs */
	if( t&0x01 ) { CHECK_IRQ_LINES(); }
}

/* $1039 PULSW inherent ----- */
INLINE void pulsw( void )
{
	PULLWORD(W);
}

/* $103b PULUW inherent ----- */
INLINE void puluw( void )
{
	PULUWORD(W);
}

/* $36 PSHU inherent ----- */
INLINE void pshu( void )
{
	UINT8 t;
	IMMBYTE(t);
	if( t&0x80 ) { PSHUWORD(pPC); hd6309_ICount -= 2; }
	if( t&0x40 ) { PSHUWORD(pS);  hd6309_ICount -= 2; }
	if( t&0x20 ) { PSHUWORD(pY);  hd6309_ICount -= 2; }
	if( t&0x10 ) { PSHUWORD(pX);  hd6309_ICount -= 2; }
	if( t&0x08 ) { PSHUBYTE(DP);  hd6309_ICount -= 1; }
	if( t&0x04 ) { PSHUBYTE(B);   hd6309_ICount -= 1; }
	if( t&0x02 ) { PSHUBYTE(A);   hd6309_ICount -= 1; }
	if( t&0x01 ) { PSHUBYTE(CC);  hd6309_ICount -= 1; }
}

/* 37 PULU inherent ----- */
INLINE void pulu( void )
{
	UINT8 t;
	IMMBYTE(t);
	if( t&0x01 ) { PULUBYTE(CC); hd6309_ICount -= 1; }
	if( t&0x02 ) { PULUBYTE(A);  hd6309_ICount -= 1; }
	if( t&0x04 ) { PULUBYTE(B);  hd6309_ICount -= 1; }
	if( t&0x08 ) { PULUBYTE(DP); hd6309_ICount -= 1; }
	if( t&0x10 ) { PULUWORD(XD); hd6309_ICount -= 2; }
	if( t&0x20 ) { PULUWORD(YD); hd6309_ICount -= 2; }
	if( t&0x40 ) { PULUWORD(SD); hd6309_ICount -= 2; }
	if( t&0x80 ) { PULUWORD(PCD); CHANGE_PC; hd6309_ICount -= 2; }

	/* HJB 990225: moved check after all PULLs */
	if( t&0x01 ) { CHECK_IRQ_LINES(); }
}

/* $38 ILLEGAL */

/* $39 RTS inherent ----- */
INLINE void rts( void )
{
	PULLWORD(PCD);
	CHANGE_PC;
}

/* $3A ABX inherent ----- */
INLINE void abx( void )
{
	X += B;
}

/* $3B RTI inherent ##### */
INLINE void rti( void )
{
	UINT8 t;
	PULLBYTE(CC);
	t = CC & CC_E;		/* HJB 990225: entire state saved? */
	if(t)
	{
		hd6309_ICount -= 9;
		PULLBYTE(A);
		PULLBYTE(B);
		if ( MD & MD_EM )
		{
			PUSHBYTE(E);
			PUSHBYTE(F);
		}
		PULLBYTE(DP);
		PULLWORD(XD);
		PULLWORD(YD);
		PULLWORD(UD);
	}
	PULLWORD(PCD);
	CHANGE_PC;
	CHECK_IRQ_LINES();	/* HJB 990116 */
}

/* $3C CWAI inherent ----1 */
INLINE void cwai( void )
{
	UINT8 t;
	IMMBYTE(t);
	CC &= t;
	/*
	 * CWAI stacks the entire machine state on the hardware stack,
	 * then waits for an interrupt; when the interrupt is taken
	 * later, the state is *not* saved again after CWAI.
	 */
	CC |= CC_E; 		/* HJB 990225: save entire state */
	PUSHWORD(pPC);
	PUSHWORD(pU);
	PUSHWORD(pY);
	PUSHWORD(pX);
	PUSHBYTE(DP);
	if ( MD & MD_EM )
	{
		PUSHBYTE(E);
		PUSHBYTE(F);
	}
	PUSHBYTE(B);
	PUSHBYTE(A);
	PUSHBYTE(CC);
	hd6309.int_state |= HD6309_CWAI;	 /* HJB 990228 */
	CHECK_IRQ_LINES();	  /* HJB 990116 */
	if( hd6309.int_state & HD6309_CWAI )
		if( hd6309_ICount > 0 )
			hd6309_ICount = 0;
}

/* $3D MUL inherent --*-@ */
INLINE void mul( void )
{
	UINT16 t;
	t = A * B;
	CLR_ZC; SET_Z16(t); if(t&0x80) SEC;
	D = t;
}

/* $3E ILLEGAL */

/* $3F SWI (SWI2 SWI3) absolute indirect ----- */
INLINE void swi( void )
{
	CC |= CC_E; 			/* HJB 980225: save entire state */
	PUSHWORD(pPC);
	PUSHWORD(pU);
	PUSHWORD(pY);
	PUSHWORD(pX);
	PUSHBYTE(DP);
	if ( MD & MD_EM )
	{
		PUSHBYTE(F);
		PUSHBYTE(E);
	}
	PUSHBYTE(B);
	PUSHBYTE(A);
	PUSHBYTE(CC);
	CC |= CC_IF | CC_II;	/* inhibit FIRQ and IRQ */
	PCD=RM16(0xfffa);
	CHANGE_PC;
}

/* $1130 BAND */

#define decodePB_tReg(n)	((n)&3)
#define decodePB_src(n) 	(((n)>>2)&7)
#define decodePB_dst(n) 	(((n)>>5)&7)

static unsigned char *	regTable[4] = { &(CC), &(A), &(B), &(E) };

static UINT8	bitTable[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

INLINE void band( void )
{
	UINT8		pb;
	UINT16		db;

	IMMBYTE(pb);

	DIRBYTE(db);

	if ( ( *(regTable[decodePB_tReg(pb)]) & bitTable[decodePB_dst(pb)] ) && ( db & bitTable[decodePB_src(pb)] ))
		*(regTable[decodePB_tReg(pb)]) |= bitTable[decodePB_dst(pb)];
	else
		*(regTable[decodePB_tReg(pb)]) &= (~bitTable[decodePB_dst(pb)]);
}

/* $1131 BIAND */

INLINE void biand( void )
{
	UINT8		pb;
	UINT16		db;

	IMMBYTE(pb);

	DIRBYTE(db);

	if ( ( *(regTable[decodePB_tReg(pb)]) & bitTable[decodePB_dst(pb)] ) && ( (~db) & bitTable[decodePB_src(pb)] ))
		*(regTable[decodePB_tReg(pb)]) |= bitTable[decodePB_dst(pb)];
	else
		*(regTable[decodePB_tReg(pb)]) &= (~bitTable[decodePB_dst(pb)]);
}

/* $1132 BOR */

INLINE void bor( void )
{
	UINT8		pb;
	UINT16		db;

	IMMBYTE(pb);

	DIRBYTE(db);

	if ( ( *(regTable[decodePB_tReg(pb)]) & bitTable[decodePB_dst(pb)] ) || ( db & bitTable[decodePB_src(pb)] ))
		*(regTable[decodePB_tReg(pb)]) |= bitTable[decodePB_dst(pb)];
	else
		*(regTable[decodePB_tReg(pb)]) &= (~bitTable[decodePB_dst(pb)]);
}

/* $1133 BIOR */

INLINE void bior( void )
{
	UINT8		pb;
	UINT16		db;

	IMMBYTE(pb);

	DIRBYTE(db);

	if ( ( *(regTable[decodePB_tReg(pb)]) & bitTable[decodePB_dst(pb)] ) || ( (~db) & bitTable[decodePB_src(pb)] ))
		*(regTable[decodePB_tReg(pb)]) |= bitTable[decodePB_dst(pb)];
	else
		*(regTable[decodePB_tReg(pb)]) &= (~bitTable[decodePB_dst(pb)]);
}

/* $1134 BEOR */

INLINE void beor( void )
{
	UINT8		pb;
	UINT16		db;
	UINT8		tReg, tMem;

	IMMBYTE(pb);

	DIRBYTE(db);

	tReg = *(regTable[decodePB_tReg(pb)]) & bitTable[decodePB_dst(pb)];
	tMem = db & bitTable[decodePB_src(pb)];

	if ( (tReg || tMem ) && !(tReg && tMem) )
		*(regTable[decodePB_tReg(pb)]) |= bitTable[decodePB_dst(pb)];
	else
		*(regTable[decodePB_tReg(pb)]) &= (~bitTable[decodePB_dst(pb)]);
}

/* $1135 BIEOR */

INLINE void bieor( void )
{
	UINT8		pb;
	UINT16		db;
	UINT8		tReg, tMem;

	IMMBYTE(pb);

	DIRBYTE(db);

	tReg = *(regTable[decodePB_tReg(pb)]) & bitTable[decodePB_dst(pb)];
	tMem = (~db) & bitTable[decodePB_src(pb)];

	if ( (tReg || tMem ) && !(tReg && tMem) )
		*(regTable[decodePB_tReg(pb)]) |= bitTable[decodePB_dst(pb)];
	else
		*(regTable[decodePB_tReg(pb)]) &= (~bitTable[decodePB_dst(pb)]);
}

/* $1133 LDBT */

INLINE void ldbt( void )
{
	UINT8		pb;
	UINT16		db;

	IMMBYTE(pb);

	DIRBYTE(db);

	if ( ( db & bitTable[decodePB_src(pb)] ) )
		*(regTable[decodePB_tReg(pb)]) |= bitTable[decodePB_dst(pb)];
	else
		*(regTable[decodePB_tReg(pb)]) &= (~bitTable[decodePB_dst(pb)]);
}

/* $1134 STBT */

INLINE void stbt( void )
{
	UINT8		pb;
	UINT16		db;

	IMMBYTE(pb);

	DIRBYTE(db);

	if ( ( *(regTable[decodePB_tReg(pb)]) & bitTable[decodePB_dst(pb)] ) )
		WM( EAD, db | bitTable[decodePB_src(pb)] );
	else
		WM( EAD, db & (~bitTable[decodePB_src(pb)]) );
}

/* $103F SWI2 absolute indirect ----- */
INLINE void swi2( void )
{
	CC |= CC_E; 			/* HJB 980225: save entire state */
	PUSHWORD(pPC);
	PUSHWORD(pU);
	PUSHWORD(pY);
	PUSHWORD(pX);
	PUSHBYTE(DP);
	if ( MD & MD_EM )
	{
		PUSHBYTE(F);
		PUSHBYTE(E);
	}
	PUSHBYTE(B);
	PUSHBYTE(A);
	PUSHBYTE(CC);
	PCD = RM16(0xfff4);
	CHANGE_PC;
}

/* $113F SWI3 absolute indirect ----- */
INLINE void swi3( void )
{
	CC |= CC_E; 			/* HJB 980225: save entire state */
	PUSHWORD(pPC);
	PUSHWORD(pU);
	PUSHWORD(pY);
	PUSHWORD(pX);
	PUSHBYTE(DP);
	if ( MD & MD_EM )
	{
		PUSHBYTE(F);
		PUSHBYTE(E);
	}
	PUSHBYTE(B);
	PUSHBYTE(A);
	PUSHBYTE(CC);
	PCD = RM16(0xfff2);
	CHANGE_PC;
}

#if macintosh
#pragma mark ____4x____
#endif

/* $40 NEGA inherent ?**** */
INLINE void nega( void )
{
	UINT16 r;
	r = -A;
	CLR_NZVC;
	SET_FLAGS8(0,A,r);
	A = r;
}

/* $41 ILLEGAL */

/* $42 ILLEGAL */

/* $43 COMA inherent -**01 */
INLINE void coma( void )
{
	A = ~A;
	CLR_NZV;
	SET_NZ8(A);
	SEC;
}

/* $44 LSRA inherent -0*-* */
INLINE void lsra( void )
{
	CLR_NZC;
	CC |= (A & CC_C);
	A >>= 1;
	SET_Z8(A);
}

/* $45 ILLEGAL */

/* $46 RORA inherent -**-* */
INLINE void rora( void )
{
	UINT8 r;
	r = (CC & CC_C) << 7;
	CLR_NZC;
	CC |= (A & CC_C);
	r |= A >> 1;
	SET_NZ8(r);
	A = r;
}

/* $47 ASRA inherent ?**-* */
INLINE void asra( void )
{
	CLR_NZC;
	CC |= (A & CC_C);
	A = (A & 0x80) | (A >> 1);
	SET_NZ8(A);
}

/* $48 ASLA inherent ?**** */
INLINE void asla( void )
{
	UINT16 r;
	r = A << 1;
	CLR_NZVC;
	SET_FLAGS8(A,A,r);
	A = r;
}

/* $49 ROLA inherent -**** */
INLINE void rola( void )
{
	UINT16 t,r;
	t = A;
	r = (CC & CC_C) | (t<<1);
	CLR_NZVC; SET_FLAGS8(t,t,r);
	A = r;
}

/* $4A DECA inherent -***- */
INLINE void deca( void )
{
	--A;
	CLR_NZV;
	SET_FLAGS8D(A);
}

/* $4B ILLEGAL */

/* $4C INCA inherent -***- */
INLINE void inca( void )
{
	++A;
	CLR_NZV;
	SET_FLAGS8I(A);
}

/* $4D TSTA inherent -**0- */
INLINE void tsta( void )
{
	CLR_NZV;
	SET_NZ8(A);
}

/* $4E ILLEGAL */

/* $4F CLRA inherent -0100 */
INLINE void clra( void )
{
	A = 0;
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____5x____
#endif

/* $50 NEGB inherent ?**** */
INLINE void negb( void )
{
	UINT16 r;
	r = -B;
	CLR_NZVC;
	SET_FLAGS8(0,B,r);
	B = r;
}

/* $1040 NEGD inherent ?**** */
INLINE void negd( void )
{
	UINT32 r;
	r = -D;
	CLR_NZVC;
	SET_FLAGS16(0,D,r);
	D = r;
}

/* $51 ILLEGAL */

/* $52 ILLEGAL */

/* $53 COMB inherent -**01 */
INLINE void comb( void )
{
	B = ~B;
	CLR_NZV;
	SET_NZ8(B);
	SEC;
}

/* $1143 COME inherent -**01 */
INLINE void come( void )
{
	E = ~E;
	CLR_NZV;
	SET_NZ8(E);
	SEC;
}

/* $1153 COMF inherent -**01 */
INLINE void comf( void )
{
	F = ~F;
	CLR_NZV;
	SET_NZ8(F);
	SEC;
}

/* $1043 COMD inherent -**01 */
INLINE void comd( void )
{
	D = ~D;
	CLR_NZV;
	SET_NZ16(D);
	SEC;
}

/* $1053 COMW inherent -**01 */
INLINE void comw( void )
{
	W = ~W;
	CLR_NZV;
	SET_NZ16(W);
	SEC;
}

/* $54 LSRB inherent -0*-* */
INLINE void lsrb( void )
{
	CLR_NZC;
	CC |= (B & CC_C);
	B >>= 1;
	SET_Z8(B);
}

/* $1044 LSRD inherent -0*-* */
INLINE void lsrd( void )
{
	CLR_NZC;
	CC |= (B & CC_C);
	D >>= 1;
	SET_Z16(D);
}

/* $1054 LSRW inherent -0*-* */
INLINE void lsrw( void )
{
	CLR_NZC;
	CC |= (F & CC_C);
	W >>= 1;
	SET_Z16(W);
}

/* $55 ILLEGAL */

/* $56 RORB inherent -**-* */
INLINE void rorb( void )
{
	UINT8 r;
	r = (CC & CC_C) << 7;
	CLR_NZC;
	CC |= (B & CC_C);
	r |= B >> 1;
	SET_NZ8(r);
	B = r;
}

/* $1046 RORD inherent -**-* */
INLINE void rord( void )
{
	UINT16 r;
	r = (CC & CC_C) << 15;
	CLR_NZC;
	CC |= (D & CC_C);
	r |= D >> 1;
	SET_NZ16(r);
	D = r;
}

/* $1056 RORW inherent -**-* */
INLINE void rorw( void )
{
	UINT16 r;
	r = (CC & CC_C) << 15;
	CLR_NZC;
	CC |= (W & CC_C);
	r |= W >> 1;
	SET_NZ16(r);
	W = r;
}

/* $57 ASRB inherent ?**-* */
INLINE void asrb( void )
{
	CLR_NZC;
	CC |= (B & CC_C);
	B= (B & 0x80) | (B >> 1);
	SET_NZ8(B);
}

/* $1047 ASRD inherent ?**-* */
INLINE void asrd( void )
{
	CLR_NZC;
	CC |= (D & CC_C);
	D= (D & 0x8000) | (D >> 1);
	SET_NZ16(D);
}

/* $58 ASLB inherent ?**** */
INLINE void aslb( void )
{
	UINT16 r;
	r = B << 1;
	CLR_NZVC;
	SET_FLAGS8(B,B,r);
	B = r;
}

/* $1048 ASLD inherent ?**** */
INLINE void asld( void )
{
	UINT32 r;
	r = D << 1;
	CLR_NZVC;
	SET_FLAGS16(D,D,r);
	D = r;
}

/* $59 ROLB inherent -**** */
INLINE void rolb( void )
{
	UINT16 t,r;
	t = B;
	r = CC & CC_C;
	r |= t << 1;
	CLR_NZVC;
	SET_FLAGS8(t,t,r);
	B = r;
}

/* $1049 ROLD inherent -**** */
INLINE void rold( void )
{
	UINT32 t,r;
	t = D;
	r = CC & CC_C;
	r |= t << 1;
	CLR_NZVC;
	SET_FLAGS16(t,t,r);
	D = r;
}

/* $1059 ROLW inherent -**** */
INLINE void rolw( void )
{
	UINT32 t,r;
	t = W;
	r = CC & CC_C;
	r |= t << 1;
	CLR_NZVC;
	SET_FLAGS16(t,t,r);
	W = r;
}

/* $5A DECB inherent -***- */
INLINE void decb( void )
{
	--B;
	CLR_NZV;
	SET_FLAGS8D(B);
}

/* $114a DECE inherent -***- */
INLINE void dece( void )
{
	--E;
	CLR_NZV;
	SET_FLAGS8D(E);
}

/* $115a DECF inherent -***- */
INLINE void decf( void )
{
	--F;
	CLR_NZV;
	SET_FLAGS8D(F);
}

/* $104a DECD inherent -***- */
INLINE void decd( void )
{
	UINT32 r;
	r = D - 1;
	CLR_NZVC;
	SET_FLAGS16(D,D,r)
	D = r;
}

/* $105a DECW inherent -***- */
INLINE void decw( void )
{
	UINT32 r;
	r = W - 1;
	CLR_NZVC;
	SET_FLAGS16(W,W,r)
	W = r;
}

/* $5B ILLEGAL */

/* $5C INCB inherent -***- */
INLINE void incb( void )
{
	++B;
	CLR_NZV;
	SET_FLAGS8I(B);
}

/* $114c INCE inherent -***- */
INLINE void ince( void )
{
	++E;
	CLR_NZV;
	SET_FLAGS8I(E);
}

/* $115c INCF inherent -***- */
INLINE void incf( void )
{
	++F;
	CLR_NZV;
	SET_FLAGS8I(F);
}

/* $104c INCD inherent -***- */
INLINE void incd( void )
{
	UINT32 r;
	r = D + 1;
	CLR_NZVC;
	SET_FLAGS16(D,D,r)
	D = r;
}

/* $105c INCW inherent -***- */
INLINE void incw( void )
{
	UINT32 r;
	r = W + 1;
	CLR_NZVC;
	SET_FLAGS16(W,W,r)
	W = r;
}

/* $5D TSTB inherent -**0- */
INLINE void tstb( void )
{
	CLR_NZV;
	SET_NZ8(B);
}

/* $104d TSTD inherent -**0- */
INLINE void tstd( void )
{
	CLR_NZV;
	SET_NZ16(D);
}

/* $105d TSTW inherent -**0- */
INLINE void tstw( void )
{
	CLR_NZV;
	SET_NZ16(W);
}

/* $114d TSTE inherent -**0- */
INLINE void tste( void )
{
	CLR_NZV;
	SET_NZ8(E);
}

/* $115d TSTF inherent -**0- */
INLINE void tstf( void )
{
	CLR_NZV;
	SET_NZ8(F);
}

/* $5E ILLEGAL */

/* $5F CLRB inherent -0100 */
INLINE void clrb( void )
{
	B = 0;
	CLR_NZVC; SEZ;
}

/* $104f CLRD inherent -0100 */
INLINE void clrd( void )
{
	D = 0;
	CLR_NZVC; SEZ;
}

/* $114f CLRE inherent -0100 */
INLINE void clre( void )
{
	E = 0;
	CLR_NZVC; SEZ;
}

/* $115f CLRF inherent -0100 */
INLINE void clrf( void )
{
	F = 0;
	CLR_NZVC; SEZ;
}

/* $105f CLRW inherent -0100 */
INLINE void clrw( void )
{
	W = 0;
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____6x____
#endif

/* $60 NEG indexed ?**** */
INLINE void neg_ix( void )
{
	UINT16 r,t;
	fetch_effective_address();
	t = RM(EAD);
	r=-t;
	CLR_NZVC;
	SET_FLAGS8(0,t,r);
	WM(EAD,r);
}

/* $61 OIM indexed */
INLINE void oim_ix( void )
{
	UINT8	r,im;
	IMMBYTE(im);
	fetch_effective_address();
	r = im | RM(EAD);
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $62 AIM indexed */
INLINE void aim_ix( void )
{
	UINT8	r,im;
	IMMBYTE(im);
	fetch_effective_address();
	r = im & RM(EAD);
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $63 COM indexed -**01 */
INLINE void com_ix( void )
{
	UINT8 t;
	fetch_effective_address();
	t = ~RM(EAD);
	CLR_NZV;
	SET_NZ8(t);
	SEC;
	WM(EAD,t);
}

/* $64 LSR indexed -0*-* */
INLINE void lsr_ix( void )
{
	UINT8 t;
	fetch_effective_address();
	t=RM(EAD);
	CLR_NZC;
	CC |= (t & CC_C);
	t>>=1; SET_Z8(t);
	WM(EAD,t);
}

/* $65 EIM indexed */
INLINE void eim_ix( void )
{
	UINT8	r,im;
	IMMBYTE(im);
	fetch_effective_address();
	r = im ^ RM(EAD);
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}
/* $66 ROR indexed -**-* */
INLINE void ror_ix( void )
{
	UINT8 t,r;
	fetch_effective_address();
	t=RM(EAD);
	r = (CC & CC_C) << 7;
	CLR_NZC;
	CC |= (t & CC_C);
	r |= t>>1; SET_NZ8(r);
	WM(EAD,r);
}

/* $67 ASR indexed ?**-* */
INLINE void asr_ix( void )
{
	UINT8 t;
	fetch_effective_address();
	t=RM(EAD);
	CLR_NZC;
	CC |= (t & CC_C);
	t=(t&0x80)|(t>>1);
	SET_NZ8(t);
	WM(EAD,t);
}

/* $68 ASL indexed ?**** */
INLINE void asl_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t=RM(EAD);
	r = t << 1;
	CLR_NZVC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $69 ROL indexed -**** */
INLINE void rol_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t=RM(EAD);
	r = CC & CC_C;
	r |= t << 1;
	CLR_NZVC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $6A DEC indexed -***- */
INLINE void dec_ix( void )
{
	UINT8 t;
	fetch_effective_address();
	t = RM(EAD) - 1;
	CLR_NZV; SET_FLAGS8D(t);
	WM(EAD,t);
}

/* $6B TIM indexed */
INLINE void tim_ix( void )
{
	UINT8	r,im;
	IMMBYTE(im);
	fetch_effective_address();
	r = im & RM(EAD);
	CLR_NZV;
	SET_NZ8(r);
}

/* $6C INC indexed -***- */
INLINE void inc_ix( void )
{
	UINT8 t;
	fetch_effective_address();
	t = RM(EAD) + 1;
	CLR_NZV; SET_FLAGS8I(t);
	WM(EAD,t);
}

/* $6D TST indexed -**0- */
INLINE void tst_ix( void )
{
	UINT8 t;
	fetch_effective_address();
	t = RM(EAD);
	CLR_NZV;
	SET_NZ8(t);
}

/* $6E JMP indexed ----- */
INLINE void jmp_ix( void )
{
	fetch_effective_address();
	PCD = EAD;
	CHANGE_PC;
}

/* $6F CLR indexed -0100 */
INLINE void clr_ix( void )
{
	fetch_effective_address();
	WM(EAD,0);
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____7x____
#endif

/* $70 NEG extended ?**** */
INLINE void neg_ex( void )
{
	UINT16 r,t;
	EXTBYTE(t); r=-t;
	CLR_NZVC; SET_FLAGS8(0,t,r);
	WM(EAD,r);
}

/* $71 OIM extended */
INLINE void oim_ex( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	EXTBYTE(t);
	r = im | t;
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $72 AIM extended */
INLINE void aim_ex( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	EXTBYTE(t);
	r = im & t;
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $73 COM extended -**01 */
INLINE void com_ex( void )
{
	UINT8 t;
	EXTBYTE(t); t = ~t;
	CLR_NZV; SET_NZ8(t); SEC;
	WM(EAD,t);
}

/* $74 LSR extended -0*-* */
INLINE void lsr_ex( void )
{
	UINT8 t;
	EXTBYTE(t); CLR_NZC; CC |= (t & CC_C);
	t>>=1; SET_Z8(t);
	WM(EAD,t);
}

/* $75 EIM extended */
INLINE void eim_ex( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	EXTBYTE(t);
	r = im ^ t;
	CLR_NZV;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $76 ROR extended -**-* */
INLINE void ror_ex( void )
{
	UINT8 t,r;
	EXTBYTE(t); r=(CC & CC_C) << 7;
	CLR_NZC; CC |= (t & CC_C);
	r |= t>>1; SET_NZ8(r);
	WM(EAD,r);
}

/* $77 ASR extended ?**-* */
INLINE void asr_ex( void )
{
	UINT8 t;
	EXTBYTE(t); CLR_NZC; CC |= (t & CC_C);
	t=(t&0x80)|(t>>1);
	SET_NZ8(t);
	WM(EAD,t);
}

/* $78 ASL extended ?**** */
INLINE void asl_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t); r=t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $79 ROL extended -**** */
INLINE void rol_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t); r = (CC & CC_C) | (t << 1);
	CLR_NZVC; SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $7A DEC extended -***- */
INLINE void dec_ex( void )
{
	UINT8 t;
	EXTBYTE(t); --t;
	CLR_NZV; SET_FLAGS8D(t);
	WM(EAD,t);
}

/* $7B TIM extended */
INLINE void tim_ex( void )
{
	UINT8	r,t,im;
	IMMBYTE(im);
	EXTBYTE(t);
	r = im & t;
	CLR_NZV;
	SET_NZ8(r);
}

/* $7C INC extended -***- */
INLINE void inc_ex( void )
{
	UINT8 t;
	EXTBYTE(t); ++t;
	CLR_NZV; SET_FLAGS8I(t);
	WM(EAD,t);
}

/* $7D TST extended -**0- */
INLINE void tst_ex( void )
{
	UINT8 t;
	EXTBYTE(t); CLR_NZV; SET_NZ8(t);
}

/* $7E JMP extended ----- */
INLINE void jmp_ex( void )
{
	EXTENDED;
	PCD = EAD;
	CHANGE_PC;
}

/* $7F CLR extended -0100 */
INLINE void clr_ex( void )
{
	EXTENDED;
	WM(EAD,0);
	CLR_NZVC; SEZ;
}


#if macintosh
#pragma mark ____8x____
#endif

/* $80 SUBA immediate ?**** */
INLINE void suba_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $81 CMPA immediate ?**** */
INLINE void cmpa_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
}

/* $82 SBCA immediate ?**** */
INLINE void sbca_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = A - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $83 SUBD (CMPD CMPU) immediate -**** */
INLINE void subd_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $1080 SUBW immediate -**** */
INLINE void subw_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $1083 CMPD immediate -**** */
INLINE void cmpd_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $1081 CMPW immediate -**** */
INLINE void cmpw_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $1183 CMPU immediate -**** */
INLINE void cmpu_im( void )
{
	UINT32 r, d;
	PAIR b;
	IMMWORD(b);
	d = U;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $84 ANDA immediate -**0- */
INLINE void anda_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	A &= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $85 BITA immediate -**0- */
INLINE void bita_im( void )
{
	UINT8 t,r;
	IMMBYTE(t);
	r = A & t;
	CLR_NZV;
	SET_NZ8(r);
}

/* $86 LDA immediate -**0- */
INLINE void lda_im( void )
{
	IMMBYTE(A);
	CLR_NZV;
	SET_NZ8(A);
}

/* $88 EORA immediate -**0- */
INLINE void eora_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	A ^= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $89 ADCA immediate ***** */
INLINE void adca_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = A + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $8A ORA immediate -**0- */
INLINE void ora_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	A |= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $8B ADDA immediate ***** */
INLINE void adda_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = A + t;
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $8C CMPX (CMPY CMPS) immediate -**** */
INLINE void cmpx_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = X;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $108C CMPY immediate -**** */
INLINE void cmpy_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = Y;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $118C CMPS immediate -**** */
INLINE void cmps_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = S;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $8D BSR ----- */
INLINE void bsr( void )
{
	UINT8 t;
	IMMBYTE(t);
	PUSHWORD(pPC);
	PC += SIGNED(t);
	CHANGE_PC;
}

/* $8E LDX (LDY) immediate -**0- */
INLINE void ldx_im( void )
{
	IMMWORD(pX);
	CLR_NZV;
	SET_NZ16(X);
}

/* $CD LDQ immediate -**0- */
INLINE void ldq_im( void )
{
	PAIR	q;

	IMMLONG(q);
	CLR_NZV;
	SET_NZ32(q.d);
	D = q.w.h;
	W = q.w.l;
}

/* $108E LDY immediate -**0- */
INLINE void ldy_im( void )
{
	IMMWORD(pY);
	CLR_NZV;
	SET_NZ16(Y);
}

/* $118f MULD immediate */
INLINE void muld_im( void )
{
	PAIR t, q;

	IMMWORD( t );
	q.d = (signed short) D * (signed short)t.w.l;
	D = q.w.h;
	W = q.w.l;

	/* Warning: Set CC */
}

/* $118d DIVD immediate */
INLINE void divd_im( void )
{
	UINT8 t;

	IMMBYTE( t );
	if ( t == 0 )
	{
		DivisionByZeroError;
	}
	else
	{
		W = (signed short) D / (signed char) t;
		D = (signed short) D % (signed char) t;
	}

	/* Warning: Set CC */
}

/* $118e DIVQ immediate */
INLINE void divq_im( void )
{
	PAIR	t,q;
	INT16	v;

	IMMWORD( t );
	q.w.h = D;
	q.w.l = W;

	v = (signed long) q.d / (signed short) t.w.l;
	W = (signed long) q.d % (signed short) t.w.l;
	D = v;

	/* Warning: Set CC */
}

#if macintosh
#pragma mark ____9x____
#endif

/* $90 SUBA direct ?**** */
INLINE void suba_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $91 CMPA direct ?**** */
INLINE void cmpa_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
}

/* $92 SBCA direct ?**** */
INLINE void sbca_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = A - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $93 SUBD (CMPD CMPU) direct -**** */
INLINE void subd_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $1090 SUBW direct -**** */
INLINE void subw_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $1093 CMPD direct -**** */
INLINE void cmpd_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $1091 CMPW direct -**** */
INLINE void cmpw_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $1193 CMPU direct -**** */
INLINE void cmpu_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = U;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(U,b.d,r);
}

/* $94 ANDA direct -**0- */
INLINE void anda_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	A &= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $95 BITA direct -**0- */
INLINE void bita_di( void )
{
	UINT8 t,r;
	DIRBYTE(t);
	r = A & t;
	CLR_NZV;
	SET_NZ8(r);
}

/* $96 LDA direct -**0- */
INLINE void lda_di( void )
{
	DIRBYTE(A);
	CLR_NZV;
	SET_NZ8(A);
}

/* $113d LDMD direct -**0- */
INLINE void ldmd_di( void )
{
	DIRBYTE(MD);
}

/* $97 STA direct -**0- */
INLINE void sta_di( void )
{
	CLR_NZV;
	SET_NZ8(A);
	DIRECT;
	WM(EAD,A);
}

/* $98 EORA direct -**0- */
INLINE void eora_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	A ^= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $99 ADCA direct ***** */
INLINE void adca_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = A + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $9A ORA direct -**0- */
INLINE void ora_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	A |= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $9B ADDA direct ***** */
INLINE void adda_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = A + t;
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $9C CMPX (CMPY CMPS) direct -**** */
INLINE void cmpx_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = X;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $109C CMPY direct -**** */
INLINE void cmpy_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = Y;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $119C CMPS direct -**** */
INLINE void cmps_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = S;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $9D JSR direct ----- */
INLINE void jsr_di( void )
{
	DIRECT;
	PUSHWORD(pPC);
	PCD = EAD;
	CHANGE_PC;
}

/* $9E LDX (LDY) direct -**0- */
INLINE void ldx_di( void )
{
	DIRWORD(pX);
	CLR_NZV;
	SET_NZ16(X);
}

/* $119f MULD direct -**0- */
INLINE void muld_di( void )
{
	PAIR	t,q;

	DIRWORD(t);
	q.d = (signed short) D * (signed short)t.w.l;

	D = q.w.h;
	W = q.w.l;
	/* Warning: Set CC */
}

/* $119d DIVD direct -**0- */
INLINE void divd_di( void )
{
	UINT8	t;

	DIRBYTE(t);
	W = (signed short) D / (signed char) t;
	D = (signed short) D % (signed char) t;

	/* Warning: Set CC */
}

/* $119e DIVQ direct -**0- */
INLINE void divq_di( void )
{
	PAIR	t, q;
	INT16	v;

	q.w.h = D;
	q.w.l = W;

	DIRWORD(t);
	v = (signed long) q.d / (signed short) t.w.l;
	W = (signed long) q.d % (signed short) t.w.l;
	D = v;

	/* Warning: Set CC */
}

/* $10dc LDQ direct -**0- */
INLINE void ldq_di( void )
{
	PAIR	q;

	DIRLONG(q);
	CLR_NZV;
	SET_NZ32(q.d);
	D = q.w.h;
	W = q.w.l;
}

/* $109E LDY direct -**0- */
INLINE void ldy_di( void )
{
	DIRWORD(pY);
	CLR_NZV;
	SET_NZ16(Y);
}

/* $9F STX (STY) direct -**0- */
INLINE void stx_di( void )
{
	CLR_NZV;
	SET_NZ16(X);
	DIRECT;
	WM16(EAD,&pX);
}

/* $10dd STQ direct -**0- */
INLINE void stq_di( void )
{
	PAIR	q;

	q.w.h = D;
	q.w.l = W;

	CLR_NZV;
	SET_NZ32(q.d);
	DIRECT;
	WM32(EAD,&q);
}

/* $109F STY direct -**0- */
INLINE void sty_di( void )
{
	CLR_NZV;
	SET_NZ16(Y);
	DIRECT;
	WM16(EAD,&pY);
}

#if macintosh
#pragma mark ____Ax____
#endif


/* $a0 SUBA indexed ?**** */
INLINE void suba_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $a1 CMPA indexed ?**** */
INLINE void cmpa_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
}

/* $a2 SBCA indexed ?**** */
INLINE void sbca_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = A - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $a3 SUBD (CMPD CMPU) indexed -**** */
INLINE void subd_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $10a0 SUBW indexed -**** */
INLINE void subw_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $10a3 CMPD indexed -**** */
INLINE void cmpd_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $10a1 CMPW indexed -**** */
INLINE void cmpw_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $11a3 CMPU indexed -**** */
INLINE void cmpu_ix( void )
{
	UINT32 r;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	r = U - b.d;
	CLR_NZVC;
	SET_FLAGS16(U,b.d,r);
}

/* $a4 ANDA indexed -**0- */
INLINE void anda_ix( void )
{
	fetch_effective_address();
	A &= RM(EAD);
	CLR_NZV;
	SET_NZ8(A);
}

/* $a5 BITA indexed -**0- */
INLINE void bita_ix( void )
{
	UINT8 r;
	fetch_effective_address();
	r = A & RM(EAD);
	CLR_NZV;
	SET_NZ8(r);
}

/* $a6 LDA indexed -**0- */
INLINE void lda_ix( void )
{
	fetch_effective_address();
	A = RM(EAD);
	CLR_NZV;
	SET_NZ8(A);
}

/* $a7 STA indexed -**0- */
INLINE void sta_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ8(A);
	WM(EAD,A);
}

/* $a8 EORA indexed -**0- */
INLINE void eora_ix( void )
{
	fetch_effective_address();
	A ^= RM(EAD);
	CLR_NZV;
	SET_NZ8(A);
}

/* $a9 ADCA indexed ***** */
INLINE void adca_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = A + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $aA ORA indexed -**0- */
INLINE void ora_ix( void )
{
	fetch_effective_address();
	A |= RM(EAD);
	CLR_NZV;
	SET_NZ8(A);
}

/* $aB ADDA indexed ***** */
INLINE void adda_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = A + t;
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $aC CMPX (CMPY CMPS) indexed -**** */
INLINE void cmpx_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = X;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $10aC CMPY indexed -**** */
INLINE void cmpy_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = Y;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $11aC CMPS indexed -**** */
INLINE void cmps_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = S;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $aD JSR indexed ----- */
INLINE void jsr_ix( void )
{
	fetch_effective_address();
	PUSHWORD(pPC);
	PCD = EAD;
	CHANGE_PC;
}

/* $aE LDX (LDY) indexed -**0- */
INLINE void ldx_ix( void )
{
	fetch_effective_address();
	X=RM16(EAD);
	CLR_NZV;
	SET_NZ16(X);
}

/* $11af MULD indexed -**0- */
INLINE void muld_ix( void )
{
	PAIR	q;
	UINT16	t;

	fetch_effective_address();
	t=RM16(EAD);
	q.d = (signed short) D * (signed short)t;

	D = q.w.h;
	W = q.w.l;

	/* Warning: Set CC */
}

/* $11ad DIVD indexed -**0- */
INLINE void divd_ix( void )
{
	UINT8	t;

	fetch_effective_address();
	t=RM(EAD);
	W = (signed short) D / (signed char) t;
	D = (signed short) D % (signed char) t;

	/* Warning: Set CC */
}

/* $11ae DIVQ indexed -**0- */
INLINE void divq_ix( void )
{
	UINT16	t;
	PAIR	q;

	q.w.h = D;
	q.w.l = W;

	fetch_effective_address();
	t=RM16(EAD);
	D = (signed long) q.d / (signed short) t;
	W = (signed long) q.d % (signed short) t;

	/* Warning: Set CC */
}

/* $10ec LDQ indexed -**0- */
INLINE void ldq_ix( void )
{
	PAIR	q;

	fetch_effective_address();
	q.d=RM32(EAD);
	CLR_NZV;
	SET_NZ32(q.d);
	D = q.w.h;
	W = q.w.l;
}

/* $10aE LDY indexed -**0- */
INLINE void ldy_ix( void )
{
	fetch_effective_address();
	Y=RM16(EAD);
	CLR_NZV;
	SET_NZ16(Y);
}

/* $aF STX (STY) indexed -**0- */
INLINE void stx_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ16(X);
	WM16(EAD,&pX);
}

/* $10ed STQ indexed -**0- */
INLINE void stq_ix( void )
{
	PAIR	q;

	q.w.h = D;
	q.w.l = W;

	fetch_effective_address();
	CLR_NZV;
	SET_NZ32(q.d);
	WM32(EAD,&q);
}

/* $10aF STY indexed -**0- */
INLINE void sty_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ16(Y);
	WM16(EAD,&pY);
}

#if macintosh
#pragma mark ____Bx____
#endif

/* $b0 SUBA extended ?**** */
INLINE void suba_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $b1 CMPA extended ?**** */
INLINE void cmpa_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = A - t;
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
}

/* $b2 SBCA extended ?**** */
INLINE void sbca_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = A - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $b3 SUBD (CMPD CMPU) extended -**** */
INLINE void subd_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $10b0 SUBW extended -**** */
INLINE void subw_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $10b3 CMPD extended -**** */
INLINE void cmpd_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = D;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $10b1 CMPW extended -**** */
INLINE void cmpw_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = W;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $11b3 CMPU extended -**** */
INLINE void cmpu_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = U;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $b4 ANDA extended -**0- */
INLINE void anda_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	A &= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $b5 BITA extended -**0- */
INLINE void bita_ex( void )
{
	UINT8 t,r;
	EXTBYTE(t);
	r = A & t;
	CLR_NZV; SET_NZ8(r);
}

/* $b6 LDA extended -**0- */
INLINE void lda_ex( void )
{
	EXTBYTE(A);
	CLR_NZV;
	SET_NZ8(A);
}

/* $b7 STA extended -**0- */
INLINE void sta_ex( void )
{
	CLR_NZV;
	SET_NZ8(A);
	EXTENDED;
	WM(EAD,A);
}

/* $b8 EORA extended -**0- */
INLINE void eora_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	A ^= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $b9 ADCA extended ***** */
INLINE void adca_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = A + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $bA ORA extended -**0- */
INLINE void ora_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	A |= t;
	CLR_NZV;
	SET_NZ8(A);
}

/* $bB ADDA extended ***** */
INLINE void adda_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = A + t;
	CLR_HNZVC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $bC CMPX (CMPY CMPS) extended -**** */
INLINE void cmpx_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = X;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $10bC CMPY extended -**** */
INLINE void cmpy_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = Y;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $11bC CMPS extended -**** */
INLINE void cmps_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = S;
	r = d - b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
}

/* $bD JSR extended ----- */
INLINE void jsr_ex( void )
{
	EXTENDED;
	PUSHWORD(pPC);
	PCD = EAD;
	CHANGE_PC;
}

/* $bE LDX (LDY) extended -**0- */
INLINE void ldx_ex( void )
{
	EXTWORD(pX);
	CLR_NZV;
	SET_NZ16(X);
}

/* $11bf MULD extended -**0- */
INLINE void muld_ex( void )
{
	PAIR	t, q;

	EXTWORD(t);
	q.d = (signed short) D * (signed short)t.w.l;

	D = q.w.h;
	W = q.w.l;

	/* Warning: Set CC */
}

/* $11bd DIVD extended -**0- */
INLINE void divd_ex( void )
{
	UINT8	t;

	EXTBYTE(t);
	W = (signed short) D / (signed char) t;
	D = (signed short) D % (signed char) t;

	/* Warning: Set CC */
}

/* $11be DIVQ extended -**0- */
INLINE void divq_ex( void )
{
	PAIR	t, q;

	q.w.h = D;
	q.w.l = W;

	EXTWORD(t);
	D = (signed long) q.d / (signed short) t.w.l;
	W = (signed long) q.d % (signed short) t.w.l;

	/* Warning: Set CC */
}

/* $10fc LDQ extended -**0- */
INLINE void ldq_ex( void )
{
	PAIR	q;

	EXTLONG(q);
	CLR_NZV;
	SET_NZ32(q.d);

	D = q.w.h;
	W = q.w.l;
}

/* $10bE LDY extended -**0- */
INLINE void ldy_ex( void )
{
	EXTWORD(pY);
	CLR_NZV;
	SET_NZ16(Y);
}

/* $bF STX (STY) extended -**0- */
INLINE void stx_ex( void )
{
	CLR_NZV;
	SET_NZ16(X);
	EXTENDED;
	WM16(EAD,&pX);
}

/* $10fd STQ extended -**0- */
INLINE void stq_ex( void )
{
	PAIR	q;

	q.w.h = D;
	q.w.l = W;

	CLR_NZV;
	SET_NZ32(q.d);
	EXTENDED;
	WM32(EAD,&q);
}

/* $10bF STY extended -**0- */
INLINE void sty_ex( void )
{
	CLR_NZV;
	SET_NZ16(Y);
	EXTENDED;
	WM16(EAD,&pY);
}


#if macintosh
#pragma mark ____Cx____
#endif

/* $c0 SUBB immediate ?**** */
INLINE void subb_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = B - t;
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $1180 SUBE immediate ?**** */
INLINE void sube_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = E - t;
	CLR_NZVC;
	SET_FLAGS8(E,t,r);
	E = r;
}

/* $11C0 SUBF immediate ?**** */
INLINE void subf_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = F - t;
	CLR_NZVC;
	SET_FLAGS8(F,t,r);
	F = r;
}

/* $c1 CMPB immediate ?**** */
INLINE void cmpb_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = B - t;
	CLR_NZVC; SET_FLAGS8(B,t,r);
}

/* $1181 CMPE immediate ?**** */
INLINE void cmpe_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = E - t;
	CLR_NZVC; SET_FLAGS8(E,t,r);
}

/* $11C1 CMPF immediate ?**** */
INLINE void cmpf_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = F - t;
	CLR_NZVC; SET_FLAGS8(F,t,r);
}

/* $c2 SBCB immediate ?**** */
INLINE void sbcb_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = B - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $1082 SBCD immediate ?**** */
INLINE void sbcd_im( void )
{
	PAIR	t;
	UINT32	 r;
	IMMWORD(t);
	r = D - t.w.l - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS16(D,t.w.l,r);
	D = r;
}

/* $c3 ADDD immediate -**** */
INLINE void addd_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = D;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $108b ADDW immediate -**** */
INLINE void addw_im( void )
{
	UINT32 r,d;
	PAIR b;
	IMMWORD(b);
	d = W;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $118b ADDE immediate -**** */
INLINE void adde_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = E + t;
	CLR_HNZVC;
	SET_FLAGS8(E,t,r);
	SET_H(E,t,r);
	E = r;
}

/* $11Cb ADDF immediate -**** */
INLINE void addf_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = F + t;
	CLR_HNZVC;
	SET_FLAGS8(F,t,r);
	SET_H(F,t,r);
	F = r;
}

/* $c4 ANDB immediate -**0- */
INLINE void andb_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	B &= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $1084 ANDD immediate -**0- */
INLINE void andd_im( void )
{
	PAIR t;
	IMMWORD(t);
	D &= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $c5 BITB immediate -**0- */
INLINE void bitb_im( void )
{
	UINT8 t,r;
	IMMBYTE(t);
	r = B & t;
	CLR_NZV;
	SET_NZ8(r);
}

/* $1085 BITD immediate -**0- */
INLINE void bitd_im( void )
{
	PAIR	t;
	UINT16	r;
	IMMWORD(t);
	r = B & t.w.l;
	CLR_NZV;
	SET_NZ16(r);
}

/* $113c BITMD immediate -**0- */
INLINE void bitmd_im( void )
{
	UINT8 t,r;
	IMMBYTE(t);
	r = MD & t;
	CLR_NZV;
	SET_NZ8(r);

	CLDZ;
	CLII;
}

/* $c6 LDB immediate -**0- */
INLINE void ldb_im( void )
{
	IMMBYTE(B);
	CLR_NZV;
	SET_NZ8(B);
}

/* $113d LDMD immediate -**0- */
INLINE void ldmd_im( void )
{
	IMMBYTE(MD);
/*	CLR_NZV;	*/
/*	SET_NZ8(B); */
}

/* $1186 LDE immediate -**0- */
INLINE void lde_im( void )
{
	IMMBYTE(E);
	CLR_NZV;
	SET_NZ8(E);
}

/* $11C6 LDF immediate -**0- */
INLINE void ldf_im( void )
{
	IMMBYTE(F);
	CLR_NZV;
	SET_NZ8(F);
}

/* $c8 EORB immediate -**0- */
INLINE void eorb_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	B ^= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $1088 EORD immediate -**0- */
INLINE void eord_im( void )
{
	PAIR t;
	IMMWORD(t);
	D ^= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $c9 ADCB immediate ***** */
INLINE void adcb_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = B + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $1089 ADCD immediate ***** */
INLINE void adcd_im( void )
{
	PAIR	t;
	UINT32	r;
	IMMWORD(t);
	r = D + t.w.l + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS16(D,t.w.l,r);
	D = r;
}

/* $cA ORB immediate -**0- */
INLINE void orb_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	B |= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $108a ORD immediate -**0- */
INLINE void ord_im( void )
{
	PAIR t;
	IMMWORD(t);
	D |= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $cB ADDB immediate ***** */
INLINE void addb_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = B + t;
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $cC LDD immediate -**0- */
INLINE void ldd_im( void )
{
	PAIR	t;

	IMMWORD(t);
	D=t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $1086 LDW immediate -**0- */
INLINE void ldw_im( void )
{
	PAIR	t;
	IMMWORD(t);
	W=t.w.l;
	CLR_NZV;
	SET_NZ16(W);
}

/* $cE LDU (LDS) immediate -**0- */
INLINE void ldu_im( void )
{
	IMMWORD(pU);
	CLR_NZV;
	SET_NZ16(U);
}

/* $10cE LDS immediate -**0- */
INLINE void lds_im( void )
{
	IMMWORD(pS);
	CLR_NZV;
	SET_NZ16(S);
	hd6309.int_state |= HD6309_LDS;
}

#if macintosh
#pragma mark ____Dx____
#endif

/* $d0 SUBB direct ?**** */
INLINE void subb_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = B - t;
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $1190 SUBE direct ?**** */
INLINE void sube_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = E - t;
	CLR_NZVC;
	SET_FLAGS8(E,t,r);
	E = r;
}

/* $11d0 SUBF direct ?**** */
INLINE void subf_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = F - t;
	CLR_NZVC;
	SET_FLAGS8(F,t,r);
	F = r;
}

/* $d1 CMPB direct ?**** */
INLINE void cmpb_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = B - t;
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
}

/* $1191 CMPE direct ?**** */
INLINE void cmpe_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = E - t;
	CLR_NZVC;
	SET_FLAGS8(E,t,r);
}

/* $11D1 CMPF direct ?**** */
INLINE void cmpf_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = F - t;
	CLR_NZVC;
	SET_FLAGS8(F,t,r);
}

/* $d2 SBCB direct ?**** */
INLINE void sbcb_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = B - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $1092 SBCD direct ?**** */
INLINE void sbcd_di( void )
{
	PAIR	t;
	UINT32	r;
	DIRWORD(t);
	r = D - t.w.l - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS16(D,t.w.l,r);
	D = r;
}

/* $d3 ADDD direct -**** */
INLINE void addd_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = D;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $109b ADDW direct -**** */
INLINE void addw_di( void )
{
	UINT32 r,d;
	PAIR b;
	DIRWORD(b);
	d = W;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $119b ADDE direct -**** */
INLINE void adde_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = E + t;
	CLR_HNZVC;
	SET_FLAGS8(E,t,r);
	SET_H(E,t,r);
	E = r;
}

/* $11db ADDF direct -**** */
INLINE void addf_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = F + t;
	CLR_HNZVC;
	SET_FLAGS8(F,t,r);
	SET_H(F,t,r);
	F = r;
}

/* $d4 ANDB direct -**0- */
INLINE void andb_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	B &= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $1094 ANDD direct -**0- */
INLINE void andd_di( void )
{
	PAIR t;
	DIRWORD(t);
	D &= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $d5 BITB direct -**0- */
INLINE void bitb_di( void )
{
	UINT8 t,r;
	DIRBYTE(t);
	r = B & t;
	CLR_NZV;
	SET_NZ8(r);
}

/* $1095 BITD direct -**0- */
INLINE void bitd_di( void )
{
	PAIR	t;
	UINT16	r;
	DIRWORD(t);
	r = B & t.w.l;
	CLR_NZV;
	SET_NZ16(r);
}

/* $d6 LDB direct -**0- */
INLINE void ldb_di( void )
{
	DIRBYTE(B);
	CLR_NZV;
	SET_NZ8(B);
}

/* $1196 LDE direct -**0- */
INLINE void lde_di( void )
{
	DIRBYTE(E);
	CLR_NZV;
	SET_NZ8(E);
}

/* $11d6 LDF direct -**0- */
INLINE void ldf_di( void )
{
	DIRBYTE(F);
	CLR_NZV;
	SET_NZ8(F);
}

/* $d7 STB direct -**0- */
INLINE void stb_di( void )
{
	CLR_NZV;
	SET_NZ8(B);
	DIRECT;
	WM(EAD,B);
}

/* $1197 STE direct -**0- */
INLINE void ste_di( void )
{
	CLR_NZV;
	SET_NZ8(E);
	DIRECT;
	WM(EAD,E);
}

/* $11D7 STF direct -**0- */
INLINE void stf_di( void )
{
	CLR_NZV;
	SET_NZ8(F);
	DIRECT;
	WM(EAD,F);
}

/* $d8 EORB direct -**0- */
INLINE void eorb_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	B ^= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $1098 EORD direct -**0- */
INLINE void eord_di( void )
{
	PAIR t;
	DIRWORD(t);
	D ^= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $d9 ADCB direct ***** */
INLINE void adcb_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = B + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $1099 adcd direct ***** */
INLINE void adcd_di( void )
{
	UINT32 t,r;
	DIRBYTE(t);
	r = D + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS16(D,t,r);
	D = r;
}

/* $dA ORB direct -**0- */
INLINE void orb_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	B |= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $109a ORD direct -**0- */
INLINE void ord_di( void )
{
	PAIR t;
	DIRWORD(t);
	D |= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $dB ADDB direct ***** */
INLINE void addb_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = B + t;
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $dC LDD direct -**0- */
INLINE void ldd_di( void )
{
	PAIR t;
	DIRWORD(t);
	D=t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $1096 LDW direct -**0- */
INLINE void ldw_di( void )
{
	PAIR t;
	DIRWORD(t);
	W=t.w.l;
	CLR_NZV;
	SET_NZ16(W);
}

/* $dD STD direct -**0- */
INLINE void std_di( void )
{
	CLR_NZV;
	SET_NZ16(D);
	DIRECT;
	WM16(EAD,&pD);
}

/* $1097 STW direct -**0- */
INLINE void stw_di( void )
{
	CLR_NZV;
	SET_NZ16(W);
	DIRECT;
	WM16(EAD,&pW);
}

/* $dE LDU (LDS) direct -**0- */
INLINE void ldu_di( void )
{
	DIRWORD(pU);
	CLR_NZV;
	SET_NZ16(U);
}

/* $10dE LDS direct -**0- */
INLINE void lds_di( void )
{
	DIRWORD(pS);
	CLR_NZV;
	SET_NZ16(S);
	hd6309.int_state |= HD6309_LDS;
}

/* $dF STU (STS) direct -**0- */
INLINE void stu_di( void )
{
	CLR_NZV;
	SET_NZ16(U);
	DIRECT;
	WM16(EAD,&pU);
}

/* $10dF STS direct -**0- */
INLINE void sts_di( void )
{
	CLR_NZV;
	SET_NZ16(S);
	DIRECT;
	WM16(EAD,&pS);
}

#if macintosh
#pragma mark ____Ex____
#endif


/* $e0 SUBB indexed ?**** */
INLINE void subb_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = B - t;
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $11a0 SUBE indexed ?**** */
INLINE void sube_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = E - t;
	CLR_NZVC;
	SET_FLAGS8(E,t,r);
	E = r;
}

/* $11e0 SUBF indexed ?**** */
INLINE void subf_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = F - t;
	CLR_NZVC;
	SET_FLAGS8(F,t,r);
	F = r;
}

/* $e1 CMPB indexed ?**** */
INLINE void cmpb_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = B - t;
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
}

/* $11a1 CMPE indexed ?**** */
INLINE void cmpe_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = E - t;
	CLR_NZVC;
	SET_FLAGS8(E,t,r);
}

/* $11e1 CMPF indexed ?**** */
INLINE void cmpf_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = F - t;
	CLR_NZVC;
	SET_FLAGS8(F,t,r);
}

/* $e2 SBCB indexed ?**** */
INLINE void sbcb_ix( void )
{
	UINT16	  t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = B - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $10a2 SBCD indexed ?**** */
INLINE void sbcd_ix( void )
{
	UINT32	  t,r;
	fetch_effective_address();
	t = RM16(EAD);
	r = D - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS16(D,t,r);
	D = r;
}

/* $e3 ADDD indexed -**** */
INLINE void addd_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = D;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $10ab ADDW indexed -**** */
INLINE void addw_ix( void )
{
	UINT32 r,d;
	PAIR b;
	fetch_effective_address();
	b.d=RM16(EAD);
	d = W;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $11ab ADDE indexed -**** */
INLINE void adde_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = E + t;
	CLR_HNZVC;
	SET_FLAGS8(E,t,r);
	SET_H(E,t,r);
	E = r;
}

/* $11eb ADDF indexed -**** */
INLINE void addf_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = F + t;
	CLR_HNZVC;
	SET_FLAGS8(F,t,r);
	SET_H(F,t,r);
	F = r;
}

/* $e4 ANDB indexed -**0- */
INLINE void andb_ix( void )
{
	fetch_effective_address();
	B &= RM(EAD);
	CLR_NZV;
	SET_NZ8(B);
}

/* $10a4 ANDD indexed -**0- */
INLINE void andd_ix( void )
{
	fetch_effective_address();
	D &= RM16(EAD);
	CLR_NZV;
	SET_NZ16(D);
}

/* $e5 BITB indexed -**0- */
INLINE void bitb_ix( void )
{
	UINT8 r;
	fetch_effective_address();
	r = B & RM(EAD);
	CLR_NZV;
	SET_NZ8(r);
}

/* $10a5 BITD indexed -**0- */
INLINE void bitd_ix( void )
{
	UINT16 r;
	fetch_effective_address();
	r = D & RM16(EAD);
	CLR_NZV;
	SET_NZ16(r);
}

/* $e6 LDB indexed -**0- */
INLINE void ldb_ix( void )
{
	fetch_effective_address();
	B = RM(EAD);
	CLR_NZV;
	SET_NZ8(B);
}

/* $11a6 LDE indexed -**0- */
INLINE void lde_ix( void )
{
	fetch_effective_address();
	E = RM(EAD);
	CLR_NZV;
	SET_NZ8(E);
}

/* $11e6 LDF indexed -**0- */
INLINE void ldf_ix( void )
{
	fetch_effective_address();
	F = RM(EAD);
	CLR_NZV;
	SET_NZ8(F);
}

/* $e7 STB indexed -**0- */
INLINE void stb_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ8(B);
	WM(EAD,B);
}

/* $11a7 STE indexed -**0- */
INLINE void ste_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ8(E);
	WM(EAD,E);
}

/* $11e7 STF indexed -**0- */
INLINE void stf_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ8(F);
	WM(EAD,F);
}

/* $e8 EORB indexed -**0- */
INLINE void eorb_ix( void )
{
	fetch_effective_address();
	B ^= RM(EAD);
	CLR_NZV;
	SET_NZ8(B);
}

/* $10a8 EORD indexed -**0- */
INLINE void eord_ix( void )
{
	fetch_effective_address();
	D ^= RM16(EAD);
	CLR_NZV;
	SET_NZ16(D);
}

/* $e9 ADCB indexed ***** */
INLINE void adcb_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = B + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $10a9 ADCD indexed ***** */
INLINE void adcd_ix( void )
{
	UINT32 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = D + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS16(D,t,r);
	D = r;
}

/* $eA ORB indexed -**0- */
INLINE void orb_ix( void )
{
	fetch_effective_address();
	B |= RM(EAD);
	CLR_NZV;
	SET_NZ8(B);
}

/* $10aa ORD indexed -**0- */
INLINE void ord_ix( void )
{
	fetch_effective_address();
	D |= RM16(EAD);
	CLR_NZV;
	SET_NZ16(D);
}

/* $eB ADDB indexed ***** */
INLINE void addb_ix( void )
{
	UINT16 t,r;
	fetch_effective_address();
	t = RM(EAD);
	r = B + t;
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $eC LDD indexed -**0- */
INLINE void ldd_ix( void )
{
	fetch_effective_address();
	D=RM16(EAD);
	CLR_NZV; SET_NZ16(D);
}

/* $10a6 LDW indexed -**0- */
INLINE void ldw_ix( void )
{
	fetch_effective_address();
	W=RM16(EAD);
	CLR_NZV; SET_NZ16(W);
}

/* $eD STD indexed -**0- */
INLINE void std_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ16(D);
	WM16(EAD,&pD);
}

/* $10a7 STW indexed -**0- */
INLINE void stw_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ16(W);
	WM16(EAD,&pW);
}

/* $eE LDU (LDS) indexed -**0- */
INLINE void ldu_ix( void )
{
	fetch_effective_address();
	U=RM16(EAD);
	CLR_NZV;
	SET_NZ16(U);
}

/* $10eE LDS indexed -**0- */
INLINE void lds_ix( void )
{
	fetch_effective_address();
	S=RM16(EAD);
	CLR_NZV;
	SET_NZ16(S);
	hd6309.int_state |= HD6309_LDS;
}

/* $eF STU (STS) indexed -**0- */
INLINE void stu_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ16(U);
	WM16(EAD,&pU);
}

/* $10eF STS indexed -**0- */
INLINE void sts_ix( void )
{
	fetch_effective_address();
	CLR_NZV;
	SET_NZ16(S);
	WM16(EAD,&pS);
}

#if macintosh
#pragma mark ____Fx____
#endif

/* $f0 SUBB extended ?**** */
INLINE void subb_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = B - t;
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $11b0 SUBE extended ?**** */
INLINE void sube_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = E - t;
	CLR_NZVC;
	SET_FLAGS8(E,t,r);
	E = r;
}

/* $11f0 SUBF extended ?**** */
INLINE void subf_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = F - t;
	CLR_NZVC;
	SET_FLAGS8(F,t,r);
	F = r;
}

/* $f1 CMPB extended ?**** */
INLINE void cmpb_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = B - t;
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
}

/* $11b1 CMPE extended ?**** */
INLINE void cmpe_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = E - t;
	CLR_NZVC;
	SET_FLAGS8(E,t,r);
}

/* $11f1 CMPF extended ?**** */
INLINE void cmpf_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = F - t;
	CLR_NZVC;
	SET_FLAGS8(F,t,r);
}

/* $f2 SBCB extended ?**** */
INLINE void sbcb_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = B - t - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS8(B,t,r);
	B = r;
}

/* $10b2 SBCD extended ?**** */
INLINE void sbcd_ex( void )
{
	PAIR t = {{0,}};
	UINT32 r;

	EXTWORD(t);
	r = D - t.w.l - (CC & CC_C);
	CLR_NZVC;
	SET_FLAGS16(D,t.w.l,r);
	D = r;
}

/* $f3 ADDD extended -**** */
INLINE void addd_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = D;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	D = r;
}

/* $10bb ADDW extended -**** */
INLINE void addw_ex( void )
{
	UINT32 r,d;
	PAIR b = {{0,}};
	EXTWORD(b);
	d = W;
	r = d + b.d;
	CLR_NZVC;
	SET_FLAGS16(d,b.d,r);
	W = r;
}

/* $11bb ADDE extended -**** */
INLINE void adde_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = E + t;
	CLR_HNZVC;
	SET_FLAGS8(E,t,r);
	SET_H(E,t,r);
	E = r;
}

/* $11fb ADDF extended -**** */
INLINE void addf_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = F + t;
	CLR_HNZVC;
	SET_FLAGS8(F,t,r);
	SET_H(F,t,r);
	F = r;
}

/* $f4 ANDB extended -**0- */
INLINE void andb_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	B &= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $10b4 ANDD extended -**0- */
INLINE void andd_ex( void )
{
	PAIR t = {{0,}};
	EXTWORD(t);
	D &= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $f5 BITB extended -**0- */
INLINE void bitb_ex( void )
{
	UINT8 t,r;
	EXTBYTE(t);
	r = B & t;
	CLR_NZV;
	SET_NZ8(r);
}

/* $10b5 BITD extended -**0- */
INLINE void bitd_ex( void )
{
	PAIR t = {{0,}};
	UINT8 r;
	EXTWORD(t);
	r = B & t.w.l;
	CLR_NZV;
	SET_NZ16(r);
}

/* $f6 LDB extended -**0- */
INLINE void ldb_ex( void )
{
	EXTBYTE(B);
	CLR_NZV;
	SET_NZ8(B);
}

/* $11b6 LDE extended -**0- */
INLINE void lde_ex( void )
{
	EXTBYTE(E);
	CLR_NZV;
	SET_NZ8(E);
}

/* $11f6 LDF extended -**0- */
INLINE void ldf_ex( void )
{
	EXTBYTE(F);
	CLR_NZV;
	SET_NZ8(F);
}

/* $f7 STB extended -**0- */
INLINE void stb_ex( void )
{
	CLR_NZV;
	SET_NZ8(B);
	EXTENDED;
	WM(EAD,B);
}

/* $11b7 STE extended -**0- */
INLINE void ste_ex( void )
{
	CLR_NZV;
	SET_NZ8(E);
	EXTENDED;
	WM(EAD,E);
}

/* $11f7 STF extended -**0- */
INLINE void stf_ex( void )
{
	CLR_NZV;
	SET_NZ8(F);
	EXTENDED;
	WM(EAD,F);
}

/* $f8 EORB extended -**0- */
INLINE void eorb_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	B ^= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $10b8 EORD extended -**0- */
INLINE void eord_ex( void )
{
	PAIR t = {{0,}};
	EXTWORD(t);
	D ^= t.w.l;
	CLR_NZV;
	SET_NZ16(D);
}

/* $f9 ADCB extended ***** */
INLINE void adcb_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = B + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $10b9 ADCD extended ***** */
INLINE void adcd_ex( void )
{
	UINT32 t,r;
	EXTBYTE(t);
	r = D + t + (CC & CC_C);
	CLR_HNZVC;
	SET_FLAGS16(D,t,r);
	D = r;
}

/* $fA ORB extended -**0- */
INLINE void orb_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	B |= t;
	CLR_NZV;
	SET_NZ8(B);
}

/* $10ba ORD extended -**0- */
INLINE void ord_ex( void )
{
	PAIR t = {{0,}};
	EXTWORD(t);
	D |= t.w.l;
	CLR_NZV;
	SET_NZ8(D);
}

/* $fB ADDB extended ***** */
INLINE void addb_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = B + t;
	CLR_HNZVC;
	SET_FLAGS8(B,t,r);
	SET_H(B,t,r);
	B = r;
}

/* $fC LDD extended -**0- */
INLINE void ldd_ex( void )
{
	EXTWORD(pD);
	CLR_NZV;
	SET_NZ16(D);
}

/* $10b6 LDW extended -**0- */
INLINE void ldw_ex( void )
{
	EXTWORD(pW);
	CLR_NZV;
	SET_NZ16(W);
}

/* $fD STD extended -**0- */
INLINE void std_ex( void )
{
	CLR_NZV;
	SET_NZ16(D);
	EXTENDED;
	WM16(EAD,&pD);
}

/* $10b7 STW extended -**0- */
INLINE void stw_ex( void )
{
	CLR_NZV;
	SET_NZ16(W);
	EXTENDED;
	WM16(EAD,&pW);
}

/* $fE LDU (LDS) extended -**0- */
INLINE void ldu_ex( void )
{
	EXTWORD(pU);
	CLR_NZV;
	SET_NZ16(U);
}

/* $10fE LDS extended -**0- */
INLINE void lds_ex( void )
{
	EXTWORD(pS);
	CLR_NZV;
	SET_NZ16(S);
	hd6309.int_state |= HD6309_LDS;
}

/* $fF STU (STS) extended -**0- */
INLINE void stu_ex( void )
{
	CLR_NZV;
	SET_NZ16(U);
	EXTENDED;
	WM16(EAD,&pU);
}

/* $10fF STS extended -**0- */
INLINE void sts_ex( void )
{
	CLR_NZV;
	SET_NZ16(S);
	EXTENDED;
	WM16(EAD,&pS);
}

/* $10xx opcodes */
INLINE void pref10( void )
{
	UINT8 ireg2 = ROP(PCD);
	PC++;
	switch( ireg2 )
	{
		case 0x21: lbrn();		hd6309_ICount-=5;	break;
		case 0x22: lbhi();		hd6309_ICount-=5;	break;
		case 0x23: lbls();		hd6309_ICount-=5;	break;
		case 0x24: lbcc();		hd6309_ICount-=5;	break;
		case 0x25: lbcs();		hd6309_ICount-=5;	break;
		case 0x26: lbne();		hd6309_ICount-=5;	break;
		case 0x27: lbeq();		hd6309_ICount-=5;	break;
		case 0x28: lbvc();		hd6309_ICount-=5;	break;
		case 0x29: lbvs();		hd6309_ICount-=5;	break;
		case 0x2a: lbpl();		hd6309_ICount-=5;	break;
		case 0x2b: lbmi();		hd6309_ICount-=5;	break;
		case 0x2c: lbge();		hd6309_ICount-=5;	break;
		case 0x2d: lblt();		hd6309_ICount-=5;	break;
		case 0x2e: lbgt();		hd6309_ICount-=5;	break;
		case 0x2f: lble();		hd6309_ICount-=5;	break;

		case 0x30: addr_r();	hd6309_ICount-=4;	break;
		case 0x31: adcr();		hd6309_ICount-=4;	break;
		case 0x32: subr();		hd6309_ICount-=4;	break;
		case 0x33: sbcr();		hd6309_ICount-=4;	break;
		case 0x34: andr();		hd6309_ICount-=4;	break;
		case 0x35: orr();		hd6309_ICount-=4;	break;
		case 0x36: eorr();		hd6309_ICount-=4;	break;
		case 0x37: cmpr();		hd6309_ICount-=4;	break;
		case 0x38: pshsw(); 	hd6309_ICount-=6;	break;
		case 0x39: pulsw(); 	hd6309_ICount-=6;	break;
		case 0x3a: pshuw(); 	hd6309_ICount-=6;	break;
		case 0x3b: puluw(); 	hd6309_ICount-=6;	break;
		case 0x3f: swi2();		hd6309_ICount-=20;	break;

		case 0x40: negd();		hd6309_ICount-=3;	break;
		case 0x43: comd();		hd6309_ICount-=3;	break;
		case 0x44: lsrd();		hd6309_ICount-=3;	break;
		case 0x46: rord();		hd6309_ICount-=3;	break;
		case 0x47: asrd();		hd6309_ICount-=3;	break;
		case 0x48: asld();		hd6309_ICount-=3;	break;
		case 0x49: rold();		hd6309_ICount-=3;	break;
		case 0x4a: decd();		hd6309_ICount-=3;	break;
		case 0x4c: incd();		hd6309_ICount-=3;	break;
		case 0x4d: tstd();		hd6309_ICount-=3;	break;
		case 0x4f: clrd();		hd6309_ICount-=3;	break;

		case 0x53: comw();		hd6309_ICount-=3;	break;
		case 0x54: lsrw();		hd6309_ICount-=3;	break;
		case 0x56: rorw();		hd6309_ICount-=3;	break;
		case 0x59: rolw();		hd6309_ICount-=3;	break;
		case 0x5a: decw();		hd6309_ICount-=3;	break;
		case 0x5c: incw();		hd6309_ICount-=3;	break;
		case 0x5d: tstw();		hd6309_ICount-=3;	break;
		case 0x5f: clrw();		hd6309_ICount-=3;	break;

		case 0x80: subw_im();	hd6309_ICount-=5;	break;
		case 0x81: cmpw_im();	hd6309_ICount-=5;	break;
		case 0x82: sbcd_im();	hd6309_ICount-=5;	break;
		case 0x83: cmpd_im();	hd6309_ICount-=5;	break;
		case 0x84: andd_im();	hd6309_ICount-=5;	break;
		case 0x85: bitd_im();	hd6309_ICount-=5;	break;
		case 0x86: ldw_im();	hd6309_ICount-=4;	break;
		case 0x88: eord_im();	hd6309_ICount-=5;	break;
		case 0x89: adcd_im();	hd6309_ICount-=5;	break;
		case 0x8a: ord_im();	hd6309_ICount-=5;	break;
		case 0x8b: addw_im();	hd6309_ICount-=5;	break;
		case 0x8c: cmpy_im();	hd6309_ICount-=5;	break;
		case 0x8e: ldy_im();	hd6309_ICount-=4;	break;

		case 0x90: subw_di();	hd6309_ICount-=7;	break;
		case 0x91: cmpw_di();	hd6309_ICount-=7;	break;
		case 0x92: sbcd_di();	hd6309_ICount-=7;	break;
		case 0x93: cmpd_di();	hd6309_ICount-=7;	break;
		case 0x94: andd_di();	hd6309_ICount-=7;	break;
		case 0x95: bitd_di();	hd6309_ICount-=7;	break;
		case 0x96: ldw_di();	hd6309_ICount-=6;	break;
		case 0x97: stw_di();	hd6309_ICount-=6;	break;
		case 0x98: eord_di();	hd6309_ICount-=7;	break;
		case 0x99: adcd_di();	hd6309_ICount-=7;	break;
		case 0x9a: ord_di();	hd6309_ICount-=7;	break;
		case 0x9b: addw_di();	hd6309_ICount-=7;	break;
		case 0x9c: cmpy_di();	hd6309_ICount-=7;	break;
		case 0x9e: ldy_di();	hd6309_ICount-=6;	break;
		case 0x9f: sty_di();	hd6309_ICount-=6;	break;

		case 0xa0: subw_ix();	hd6309_ICount-=7;	break;
		case 0xa1: cmpw_ix();	hd6309_ICount-=7;	break;
		case 0xa2: sbcd_ix();	hd6309_ICount-=7;	break;
		case 0xa3: cmpd_ix();	hd6309_ICount-=7;	break;
		case 0xa4: andd_ix();	hd6309_ICount-=7;	break;
		case 0xa5: bitd_ix();	hd6309_ICount-=7;	break;
		case 0xa6: ldw_ix();	hd6309_ICount-=6;	break;
		case 0xa7: stw_ix();	hd6309_ICount-=6;	break;
		case 0xa8: eord_ix();	hd6309_ICount-=7;	break;
		case 0xa9: adcd_ix();	hd6309_ICount-=7;	break;
		case 0xaa: ord_ix();	hd6309_ICount-=7;	break;
		case 0xab: addw_ix();	hd6309_ICount-=7;	break;
		case 0xac: cmpy_ix();	hd6309_ICount-=7;	break;
		case 0xae: ldy_ix();	hd6309_ICount-=6;	break;
		case 0xaf: sty_ix();	hd6309_ICount-=6;	break;

		case 0xb0: subw_ex();	hd6309_ICount-=8;	break;
		case 0xb1: cmpw_ex();	hd6309_ICount-=8;	break;
		case 0xb2: sbcd_ex();	hd6309_ICount-=8;	break;
		case 0xb3: cmpd_ex();	hd6309_ICount-=8;	break;
		case 0xb4: andd_ex();	hd6309_ICount-=8;	break;
		case 0xb5: bitd_ex();	hd6309_ICount-=8;	break;
		case 0xb6: ldw_ex();	hd6309_ICount-=7;	break;
		case 0xb7: stw_ex();	hd6309_ICount-=7;	break;
		case 0xb8: eord_ex();	hd6309_ICount-=8;	break;
		case 0xb9: adcd_ex();	hd6309_ICount-=8;	break;
		case 0xba: ord_ex();	hd6309_ICount-=8;	break;
		case 0xbb: addw_ex();	hd6309_ICount-=8;	break;
		case 0xbc: cmpy_ex();	hd6309_ICount-=8;	break;
		case 0xbe: ldy_ex();	hd6309_ICount-=7;	break;
		case 0xbf: sty_ex();	hd6309_ICount-=7;	break;

		case 0xce: lds_im();	hd6309_ICount-=4;	break;

		case 0xdc: ldq_di();	hd6309_ICount-=8;	break;
		case 0xdd: stq_di();	hd6309_ICount-=8;	break;
		case 0xde: lds_di();	hd6309_ICount-=6;	break;
		case 0xdf: sts_di();	hd6309_ICount-=6;	break;

		case 0xec: ldq_ix();	hd6309_ICount-=8;	break;
		case 0xed: stq_ix();	hd6309_ICount-=8;	break;
		case 0xee: lds_ix();	hd6309_ICount-=6;	break;
		case 0xef: sts_ix();	hd6309_ICount-=6;	break;

		case 0xfc: ldq_ex();	hd6309_ICount-=9;	break;
		case 0xfd: stq_ex();	hd6309_ICount-=9;	break;
		case 0xfe: lds_ex();	hd6309_ICount-=7;	break;
		case 0xff: sts_ex();	hd6309_ICount-=7;	break;

		default:   IlegalInstructionError;			break;
	}
}

/* $11xx opcodes */
INLINE void pref11( void )
{
	UINT8 ireg2 = ROP(PCD);
	PC++;
	switch( ireg2 )
	{
		case 0x30: band();		hd6309_ICount-=7;	break;
		case 0x31: biand(); 	hd6309_ICount-=7;	break;
		case 0x32: bor();		hd6309_ICount-=7;	break;
		case 0x33: bior();		hd6309_ICount-=7;	break;
		case 0x34: beor();		hd6309_ICount-=7;	break;
		case 0x35: bieor(); 	hd6309_ICount-=7;	break;
		case 0x36: ldbt();		hd6309_ICount-=7;	break;
		case 0x37: stbt();		hd6309_ICount-=8;	break;
		case 0x38: tfmpp(); 	hd6309_ICount-=6;	break;
		case 0x39: tfmmm(); 	hd6309_ICount-=6;	break;
		case 0x3a: tfmpc(); 	hd6309_ICount-=6;	break;
		case 0x3b: tfmcp(); 	hd6309_ICount-=6;	break;
		case 0x3c: bitmd_im();	hd6309_ICount-=4;	break;
		case 0x3d: ldmd_im();	hd6309_ICount-=5;	break;
		case 0x3f: swi3();		hd6309_ICount-=20;	break;

		case 0x43: come();		hd6309_ICount-=3;	break;
		case 0x4a: dece();		hd6309_ICount-=3;	break;
		case 0x4c: ince();		hd6309_ICount-=3;	break;
		case 0x4d: tste();		hd6309_ICount-=3;	break;
		case 0x4f: clre();		hd6309_ICount-=3;	break;

		case 0x53: comf();		hd6309_ICount-=3;	break;
		case 0x5a: decf();		hd6309_ICount-=3;	break;
		case 0x5c: incf();		hd6309_ICount-=3;	break;
		case 0x5d: tstf();		hd6309_ICount-=3;	break;
		case 0x5f: clrf();		hd6309_ICount-=3;	break;

		case 0x80: sube_im();	hd6309_ICount-=3;	break;
		case 0x81: cmpe_im();	hd6309_ICount-=3;	break;
		case 0x83: cmpu_im();	hd6309_ICount-=5;	break;
		case 0x86: lde_im();	hd6309_ICount-=3;	break;
		case 0x8b: adde_im();	hd6309_ICount-=3;	break;
		case 0x8c: cmps_im();	hd6309_ICount-=5;	break;
		case 0x8d: divd_im();	hd6309_ICount-=25;	break;
		case 0x8e: divq_im();	hd6309_ICount-=34;	break;
		case 0x8f: muld_im();	hd6309_ICount-=28;	break;

		case 0x90: sube_di();	hd6309_ICount-=5;	break;
		case 0x91: cmpe_di();	hd6309_ICount-=5;	break;
		case 0x93: cmpu_di();	hd6309_ICount-=7;	break;
		case 0x96: lde_di();	hd6309_ICount-=5;	break;
		case 0x97: ste_di();	hd6309_ICount-=5;	break;
		case 0x9b: adde_di();	hd6309_ICount-=5;	break;
		case 0x9c: cmps_di();	hd6309_ICount-=7;	break;
		case 0x9d: divd_di();	hd6309_ICount-=27;	break;
		case 0x9e: divq_di();	hd6309_ICount-=36;	break;
		case 0x9f: muld_di();	hd6309_ICount-=30;	break;

		case 0xa0: sube_ix();	hd6309_ICount-=5;	break;
		case 0xa1: cmpe_ix();	hd6309_ICount-=5;	break;
		case 0xa3: cmpu_ix();	hd6309_ICount-=7;	break;
		case 0xa6: lde_ix();	hd6309_ICount-=5;	break;
		case 0xa7: ste_ix();	hd6309_ICount-=5;	break;
		case 0xab: adde_ix();	hd6309_ICount-=5;	break;
		case 0xac: cmps_ix();	hd6309_ICount-=7;	break;
		case 0xad: divd_ix();	hd6309_ICount-=27;	break;
		case 0xae: divq_ix();	hd6309_ICount-=36;	break;
		case 0xaf: muld_ix();	hd6309_ICount-=30;	break;

		case 0xb0: sube_ex();	hd6309_ICount-=6;	break;
		case 0xb1: cmpe_ex();	hd6309_ICount-=6;	break;
		case 0xb3: cmpu_ex();	hd6309_ICount-=8;	break;
		case 0xb6: lde_ex();	hd6309_ICount-=6;	break;
		case 0xb7: ste_ex();	hd6309_ICount-=6;	break;
		case 0xbb: adde_ex();	hd6309_ICount-=6;	break;
		case 0xbc: cmps_ex();	hd6309_ICount-=8;	break;
		case 0xbd: divd_ex();	hd6309_ICount-=28;	break;
		case 0xbe: divq_ex();	hd6309_ICount-=37;	break;
		case 0xbf: muld_ex();	hd6309_ICount-=31;	break;

		case 0xc0: subf_im();	hd6309_ICount-=3;	break;
		case 0xc1: cmpf_im();	hd6309_ICount-=3;	break;
		case 0xc6: ldf_im();	hd6309_ICount-=3;	break;
		case 0xcb: addf_im();	hd6309_ICount-=3;	break;

		case 0xd0: subf_di();	hd6309_ICount-=5;	break;
		case 0xd1: cmpf_di();	hd6309_ICount-=5;	break;
		case 0xd6: ldf_di();	hd6309_ICount-=5;	break;
		case 0xd7: stf_di();	hd6309_ICount-=5;	break;
		case 0xdb: addf_di();	hd6309_ICount-=5;	break;

		case 0xe0: subf_ix();	hd6309_ICount-=5;	break;
		case 0xe1: cmpf_ix();	hd6309_ICount-=5;	break;
		case 0xe6: ldf_ix();	hd6309_ICount-=5;	break;
		case 0xe7: stf_ix();	hd6309_ICount-=5;	break;
		case 0xeb: addf_ix();	hd6309_ICount-=5;	break;

		case 0xf0: subf_ex();	hd6309_ICount-=6;	break;
		case 0xf1: cmpf_ex();	hd6309_ICount-=6;	break;
		case 0xf6: ldf_ex();	hd6309_ICount-=6;	break;
		case 0xf7: stf_ex();	hd6309_ICount-=6;	break;
		case 0xfb: addf_ex();	hd6309_ICount-=6;	break;

		default:   IlegalInstructionError;			break;
	}
}

