/* PowerPC common opcodes */

static void ppc_addx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);

	REG(RT) = ra + rb;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, rb);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addcx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);

	REG(RT) = ra + rb;

	SET_ADD_CA(REG(RT), ra, rb);

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, rb);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 tmp;

	tmp = rb + carry;
	REG(RT) = ra + tmp;

	if( ADD_CA(tmp, rb, carry) || ADD_CA(REG(RT), ra, tmp) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, rb);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addi(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 a = RA;

	if( a ) 
		i += REG(a);

	REG(RT) = i;
}

static void ppc_addic(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 ra = REG(RA);

	REG(RT) = ra + i;

	if( ADD_CA(REG(RT), ra, i) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;
}

static void ppc_addic_rc(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 ra = REG(RA);

	REG(RT) = ra + i;

	if( ADD_CA(REG(RT), ra, i) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	SET_CR0(REG(RT));
}

static void ppc_addis(UINT32 op)
{
	UINT32 i = UIMM16 << 16;
	UINT32 a = RA;

	if( a )
		i += REG(a);

	REG(RT) = i;
}

static void ppc_addmex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 tmp;

	tmp = ra + carry;
	REG(RT) = tmp + -1;

	if( ADD_CA(tmp, ra, carry) || ADD_CA(REG(RT), tmp, -1) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, carry - 1);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addzex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;

	REG(RT) = ra + carry;

	if( ADD_CA(REG(RT), ra, carry) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, carry);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_andx(UINT32 op)
{
	REG(RA) = REG(RS) & REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_andcx(UINT32 op)
{
	REG(RA) = REG(RS) & ~REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_andi_rc(UINT32 op)
{
	UINT32 i = UIMM16;

	REG(RA) = REG(RS) & i;

	SET_CR0(REG(RA));
}

static void ppc_andis_rc(UINT32 op)
{
	UINT32 i = UIMM16 << 16;

	REG(RA) = REG(RS) & i;

	SET_CR0(REG(RA));
}

static void ppc_bx(UINT32 op)
{
	INT32 li = op & 0x3fffffc;
	if( li & 0x2000000 )
		li |= 0xfc000000;

	if( AABIT ) {
		ppc.npc = li;
	} else {
		ppc.npc = ppc.pc + li;
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}

	change_pc(ppc.npc);
}

static void ppc_bcx(UINT32 op)
{
	int condition = check_condition_code(BO, BI);

	if( condition ) {
		if( AABIT ) {
			ppc.npc = SIMM16 & ~0x3;
		} else {
			ppc.npc = ppc.pc + (SIMM16 & ~0x3);
		}

		change_pc(ppc.npc);
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}
}

static void ppc_bcctrx(UINT32 op)
{
	int condition = check_condition_code(BO, BI);

	if( condition ) {
		ppc.npc = CTR & ~0x3;
		change_pc(ppc.npc);
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}
}

static void ppc_bclrx(UINT32 op)
{
	int condition = check_condition_code(BO, BI);

	if( condition ) {
		ppc.npc = LR & ~0x3;
		change_pc(ppc.npc);
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}
}

static void ppc_cmp(UINT32 op)
{
	INT32 ra = REG(RA);
	INT32 rb = REG(RB);
	int d = CRFD;

	if( ra < rb )
		CR(d) = 0x8;
	else if( ra > rb )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cmpi(UINT32 op)
{
	INT32 ra = REG(RA);
	INT32 i = SIMM16;
	int d = CRFD;

	if( ra < i )
		CR(d) = 0x8;
	else if( ra > i )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cmpl(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	int d = CRFD;

	if( ra < rb )
		CR(d) = 0x8;
	else if( ra > rb )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cmpli(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 i = UIMM16;
	int d = CRFD;

	if( ra < i )
		CR(d) = 0x8;
	else if( ra > i )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cntlzw(UINT32 op)
{
	int n = 0;
	int t = RT;
	UINT32 m = 0x80000000;

	while(n < 32)
	{
		if( REG(t) & m )
			break;
		m >>= 1;
		n++;
	}

	REG(RA) = n;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_crand(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) & CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crandc(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) & ~CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_creqv(UINT32 op)
{
	int bit = RT;
	int b = ~(CRBIT(RA) ^ CRBIT(RB));
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crnand(UINT32 op)
{
	int bit = RT;
	int b = ~(CRBIT(RA) & CRBIT(RB));
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crnor(UINT32 op)
{
	int bit = RT;
	int b = ~(CRBIT(RA) | CRBIT(RB));
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_cror(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) | CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crorc(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) | ~CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crxor(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) ^ CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_dcbf(UINT32 op)
{

}

static void ppc_dcbi(UINT32 op)
{

}

static void ppc_dcbst(UINT32 op)
{

}

static void ppc_dcbt(UINT32 op)
{

}

static void ppc_dcbtst(UINT32 op)
{

}

static void ppc_dcbz(UINT32 op)
{

}

static void ppc_divwx(UINT32 op)
{
	if( REG(RB) == 0 && REG(RA) < 0x80000000 )
	{
		REG(RT) = 0;
		if( OEBIT ) {
			XER |= XER_SO | XER_OV;
		}
	}
	else if( REG(RB) == 0 || (REG(RB) == 0xffffffff && REG(RA) == 0x80000000) )
	{
		REG(RT) = 0xffffffff;
		if( OEBIT ) {
			XER |= XER_SO | XER_OV;
		}
	}
	else 
	{
		REG(RT) = (INT32)REG(RA) / (INT32)REG(RB);
		if( OEBIT ) {
			XER &= ~XER_OV;
		}
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_divwux(UINT32 op)
{
	if( REG(RB) == 0 )
	{
		REG(RT) = 0;
		if( OEBIT ) {
			XER |= XER_SO | XER_OV;
		}
	}
	else
	{
		REG(RT) = (UINT32)REG(RA) / (UINT32)REG(RB);
		if( OEBIT ) {
			XER &= ~XER_OV;
		}
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_eieio(UINT32 op)
{

}

static void ppc_eqvx(UINT32 op)
{
	REG(RA) = ~(REG(RS) ^ REG(RB));

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_extsbx(UINT32 op)
{
	REG(RA) = (INT32)(INT8)REG(RS);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_extshx(UINT32 op)
{
	REG(RA) = (INT32)(INT16)REG(RS);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_icbi(UINT32 op)
{

}

static void ppc_isync(UINT32 op)
{

}

static void ppc_lbz(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ8(ea);
}

static void ppc_lbzu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ8(ea);
	REG(RA) = ea;
}

static void ppc_lbzux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ8(ea);
	REG(RA) = ea;
}

static void ppc_lbzx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ8(ea);
}

static void ppc_lha(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = (INT32)(INT16)READ16(ea);
}

static void ppc_lhau(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = (INT32)(INT16)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhaux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = (INT32)(INT16)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhax(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (INT32)(INT16)READ16(ea);
}

static void ppc_lhbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)BYTE_REVERSE16(READ16(ea));
}

static void ppc_lhz(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ16(ea);
}

static void ppc_lhzu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhzux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhzx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ16(ea);
}

static void ppc_lmw(UINT32 op)
{
	int r = RT;
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	while( r <= 31 )
	{
		REG(r) = READ32(ea);
		ea += 4;
		r++;
	}
}

static void ppc_lswi(UINT32 op)
{
	osd_die("ppc: lswi unimplemented\n");
}

static void ppc_lswx(UINT32 op)
{
	osd_die("ppc: lswx unimplemented\n");
}

static void ppc_lwarx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	ppc.reserved_address = ea;
	ppc.reserved = 1;

	REG(RT) = READ32(ea);
}

static void ppc_lwbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = BYTE_REVERSE32(READ32(ea));
}

static void ppc_lwz(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = READ32(ea);
}

static void ppc_lwzu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = READ32(ea);
	REG(RA) = ea;
}

static void ppc_lwzux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = READ32(ea);
	REG(RA) = ea;
}

static void ppc_lwzx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = READ32(ea);
}

static void ppc_mcrf(UINT32 op)
{
	CR(RT >> 2) = CR(RA >> 2);
}

static void ppc_mcrxr(UINT32 op)
{
	osd_die("ppc: mcrxr unimplemented\n");
}

static void ppc_mfcr(UINT32 op)
{
	REG(RT) = ppc_get_cr();
}

static void ppc_mfmsr(UINT32 op)
{
	REG(RT) = ppc_get_msr();
}

static void ppc_mfspr(UINT32 op)
{
	REG(RT) = ppc_get_spr(SPR);
}

static void ppc_mtcrf(UINT32 op)
{
	int fxm = FXM;
	int t = RT;

	if( fxm & 0x80 )	CR(0) = (REG(t) >> 28) & 0xf;
	if( fxm & 0x40 )	CR(1) = (REG(t) >> 24) & 0xf;
	if( fxm & 0x20 )	CR(2) = (REG(t) >> 20) & 0xf;
	if( fxm & 0x10 )	CR(3) = (REG(t) >> 16) & 0xf;
	if( fxm & 0x08 )	CR(4) = (REG(t) >> 12) & 0xf;
	if( fxm & 0x04 )	CR(5) = (REG(t) >> 8) & 0xf;
	if( fxm & 0x02 )	CR(6) = (REG(t) >> 4) & 0xf;
	if( fxm & 0x01 )	CR(7) = (REG(t) >> 0) & 0xf;
}

static void ppc_mtmsr(UINT32 op)
{
	ppc_set_msr(REG(RS));
}

static void ppc_mtspr(UINT32 op)
{
	ppc_set_spr(SPR, REG(RS));
}

static void ppc_mulhwx(UINT32 op)
{
	INT64 ra = (INT64)(INT32)REG(RA);
	INT64 rb = (INT64)(INT32)REG(RB);

	REG(RT) = (UINT32)((ra * rb) >> 32);

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_mulhwux(UINT32 op)
{
	UINT64 ra = (UINT64)REG(RA);
	UINT64 rb = (UINT64)REG(RB);

	REG(RT) = (UINT32)((ra * rb) >> 32);

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_mulli(UINT32 op)
{
	INT32 ra = (INT32)REG(RA);
	INT32 i = SIMM16;

	REG(RT) = ra * i;
}

static void ppc_mullwx(UINT32 op)
{
	INT64 ra = (INT64)(INT32)REG(RA);
	INT64 rb = (INT64)(INT32)REG(RB);
	INT64 r;

	r = ra * rb;
	REG(RT) = (UINT32)r;

	if( OEBIT ) {
		XER &= ~XER_OV;

		if( r != (INT64)(INT32)r )
			XER |= XER_OV | XER_SO;
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_nandx(UINT32 op)
{
	REG(RA) = ~(REG(RS) & REG(RB));

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_negx(UINT32 op)
{
	REG(RT) = -REG(RA);

	if( OEBIT ) {
		if( REG(RT) == 0x80000000 )
			XER |= XER_OV | XER_SO;
		else
			XER &= ~XER_OV;
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_norx(UINT32 op)
{
	REG(RA) = ~(REG(RS) | REG(RB));

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_orx(UINT32 op)
{
	REG(RA) = REG(RS) | REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_orcx(UINT32 op)
{
	REG(RA) = REG(RS) | ~REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_ori(UINT32 op)
{
	REG(RA) = REG(RS) | UIMM16;
}

static void ppc_oris(UINT32 op)
{
	REG(RA) = REG(RS) | (UIMM16 << 16);
}

static void ppc_rfi(UINT32 op)
{
	ppc.npc = ppc_get_spr(SPR_SRR0);
	ppc_set_msr( ppc_get_spr(SPR_SRR1) );

	change_pc(ppc.npc);
}

static void ppc_rlwimix(UINT32 op)
{
	UINT32 r;
	UINT32 mask = GET_ROTATE_MASK(MB, ME);
	UINT32 rs = REG(RS);
	int sh = SH;

	r = (rs << sh) | (rs >> (32-sh));
	REG(RA) = (REG(RA) & ~mask) | (r & mask);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_rlwinmx(UINT32 op)
{
	UINT32 r;
	UINT32 mask = GET_ROTATE_MASK(MB, ME);
	UINT32 rs = REG(RS);
	int sh = SH;

	r = (rs << sh) | (rs >> (32-sh));
	REG(RA) = r & mask;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_rlwnmx(UINT32 op)
{
	UINT32 r;
	UINT32 mask = GET_ROTATE_MASK(MB, ME);
	UINT32 rs = REG(RS);
	int sh = REG(RB) & 0x1f;

	r = (rs << sh) | (rs >> (32-sh));
	REG(RA) = r & mask;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_sc(UINT32 op)
{
	osd_die("ppc: sc unimplemented\n");
}

static void ppc_slwx(UINT32 op)
{
	int sh = REG(RB) & 0x3f;

	if( sh > 31 ) {
		REG(RA) = 0;
	}
	else {
		REG(RA) = REG(RS) << sh;
	}

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_srawx(UINT32 op)
{
	int sh = REG(RB) & 0x3f;

	XER &= ~XER_CA;

	if( sh > 31 ) {
		REG(RA) = (INT32)(REG(RS)) >> 31;
		if( REG(RA) )
			XER |= XER_CA;
	}
	else {
		REG(RA) = (INT32)(REG(RS)) >> sh;
		if( ((INT32)(REG(RS)) < 0) && (REG(RS) & BITMASK_0(sh)) )
			XER |= XER_CA;
	}

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_srawix(UINT32 op)
{
	int sh = SH;

	XER &= ~XER_CA;
	if( ((INT32)(REG(RS)) < 0) && (REG(RS) & BITMASK_0(sh)) )
		XER |= XER_CA;

	REG(RA) = (INT32)(REG(RS)) >> sh;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_srwx(UINT32 op)
{
	int sh = REG(RB) & 0x3f;

	if( sh > 31 ) {
		REG(RA) = 0;
	}
	else {
		REG(RA) = REG(RS) >> sh;
	}

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_stb(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	WRITE8(ea, (UINT8)REG(RS));
}

static void ppc_stbu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	WRITE8(ea, (UINT8)REG(RS));
	REG(RA) = ea;
}

static void ppc_stbux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	WRITE8(ea, (UINT8)REG(RS));
	REG(RA) = ea;
}

static void ppc_stbx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE8(ea, (UINT8)REG(RS));
}

static void ppc_sth(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	WRITE16(ea, (UINT16)REG(RS));
}

static void ppc_sthbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE16(ea, (UINT16)BYTE_REVERSE16(REG(RS)));
}

static void ppc_sthu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	WRITE16(ea, (UINT16)REG(RS));
	REG(RA) = ea;
}

static void ppc_sthux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	WRITE16(ea, (UINT16)REG(RS));
	REG(RA) = ea;
}

static void ppc_sthx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE16(ea, (UINT16)REG(RS));
}

static void ppc_stmw(UINT32 op)
{
	UINT32 ea;
	int r = RS;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	while( r <= 31 )
	{
		WRITE32(ea, REG(r));
		ea += 4;
		r++;
	}
}

static void ppc_stswi(UINT32 op)
{
	osd_die("ppc: stswi unimplemented\n");
}

static void ppc_stswx(UINT32 op)
{
	osd_die("ppc: stswx unimplemented\n");
}

static void ppc_stw(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	WRITE32(ea, REG(RS));
}

static void ppc_stwbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE32(ea, BYTE_REVERSE32(REG(RS)));
}

static void ppc_stwcx_rc(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	if( ppc.reserved ) {
		WRITE32(ea, REG(RS));

		ppc.reserved = 0;
		ppc.reserved_address = 0;

		CR(0) = 0x2;
		if( XER & XER_SO )
			CR(0) |= 0x1;
	} else {
		CR(0) = 0;
		if( XER & XER_SO )
			CR(0) |= 0x1;
	}
}

static void ppc_stwu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	WRITE32(ea, REG(RS));
	REG(RA) = ea;
}

static void ppc_stwux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	WRITE32(ea, REG(RS));
	REG(RA) = ea;
}

static void ppc_stwx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE32(ea, REG(RS));
}

static void ppc_subfx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	REG(RT) = rb - ra;

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), rb, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfcx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	REG(RT) = rb - ra;

	SET_SUB_CA(REG(RT), rb, ra);

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), rb, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 r;

	r = ~ra + carry;
	REG(RT) = rb + r;

	SET_ADD_CA(r, ~ra, carry);		/* step 1 carry */
	if( REG(RT) < r )				/* step 2 carry */
		XER |= XER_CA;

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), rb, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfic(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 ra = REG(RA);

	REG(RT) = i - ra;

	SET_SUB_CA(REG(RT), i, ra);
}

static void ppc_subfmex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 r;

	r = ~ra + carry;
	REG(RT) = r - 1;

	SET_SUB_CA(r, ~ra, carry);		/* step 1 carry */
	if( REG(RT) < r )
		XER |= XER_CA;				/* step 2 carry */

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), -1, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfzex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;
	
	REG(RT) = ~ra + carry;

	SET_ADD_CA(REG(RT), ~ra, carry);

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), 0, REG(RA));
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_sync(UINT32 op)
{

}

static void ppc_tw(UINT32 op)
{
	osd_die("ppc: tw unimplemented\n");
}

static void ppc_twi(UINT32 op)
{
	osd_die("ppc: twi unimplemented\n");
}

static void ppc_xorx(UINT32 op)
{
	REG(RA) = REG(RS) ^ REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_xori(UINT32 op)
{
	REG(RA) = REG(RS) ^ UIMM16;
}

static void ppc_xoris(UINT32 op)
{
	REG(RA) = REG(RS) ^ (UIMM16 << 16);
}



static void ppc_invalid(UINT32 op)
{
	osd_die("ppc: Invalid opcode %08X\n", op);
}
