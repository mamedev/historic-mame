static void (*i386_opcode_table1_32[256])(void) =
{
	I386OP(add_rm8_r8),			/* 0x00 */
	I386OP(add_rm32_r32),		/* 0x01 */
	I386OP(add_r8_rm8),			/* 0x02 */
	I386OP(add_r32_rm32),		/* 0x03 */
	I386OP(add_al_i8),			/* 0x04 */
	I386OP(add_eax_i32),		/* 0x05 */
	I386OP(push_es32),			/* 0x06 */
	I386OP(pop_es32),			/* 0x07 */
	I386OP(or_rm8_r8),			/* 0x08 */
	I386OP(or_rm32_r32),		/* 0x09 */
	I386OP(or_r8_rm8),			/* 0x0a */
	I386OP(or_r32_rm32),		/* 0x0b */
	I386OP(or_al_i8),			/* 0x0c */
	I386OP(or_eax_i32),			/* 0x0d */
	I386OP(push_cs32),			/* 0x0e */
	I386OP(decode_two_byte),	/* 0x0f */
	I386OP(adc_rm8_r8),			/* 0x10 */
	I386OP(adc_rm32_r32),		/* 0x11 */
	I386OP(adc_r8_rm8),			/* 0x12 */
	I386OP(adc_r32_rm32),		/* 0x13 */
	I386OP(adc_al_i8),			/* 0x14 */
	I386OP(adc_eax_i32),		/* 0x15 */
	I386OP(push_ss32),			/* 0x16 */
	I386OP(pop_ss32),			/* 0x17 */
	I386OP(sbb_rm8_r8),			/* 0x18 */
	I386OP(sbb_rm32_r32),		/* 0x19 */
	I386OP(sbb_r8_rm8),			/* 0x1a */
	I386OP(sbb_r32_rm32),		/* 0x1b */
	I386OP(sbb_al_i8),			/* 0x1c */
	I386OP(sbb_eax_i32),		/* 0x1d */
	I386OP(push_ds32),			/* 0x1e */
	I386OP(pop_ds32),			/* 0x1f */
	I386OP(and_rm8_r8),			/* 0x20 */
	I386OP(and_rm32_r32),		/* 0x21 */
	I386OP(and_r8_rm8),			/* 0x22 */
	I386OP(and_r32_rm32),		/* 0x23 */
	I386OP(and_al_i8),			/* 0x24 */
	I386OP(and_eax_i32),		/* 0x25 */
	I386OP(segment_ES),			/* 0x26 */
	I386OP(daa),				/* 0x27 */
	I386OP(sub_rm8_r8),			/* 0x28 */
	I386OP(sub_rm32_r32),		/* 0x29 */
	I386OP(sub_r8_rm8),			/* 0x2a */
	I386OP(sub_r32_rm32),		/* 0x2b */
	I386OP(sub_al_i8),			/* 0x2c */
	I386OP(sub_eax_i32),		/* 0x2d */
	I386OP(segment_CS),			/* 0x2e */
	I386OP(das),				/* 0x2f */
	I386OP(xor_rm8_r8),			/* 0x30 */
	I386OP(xor_rm32_r32),		/* 0x31 */
	I386OP(xor_r8_rm8),			/* 0x32 */
	I386OP(xor_r32_rm32),		/* 0x33 */
	I386OP(xor_al_i8),			/* 0x34 */
	I386OP(xor_eax_i32),		/* 0x35 */
	I386OP(segment_SS),			/* 0x36 */
	I386OP(aaa),				/* 0x37 */
	I386OP(cmp_rm8_r8),			/* 0x38 */
	I386OP(cmp_rm32_r32),		/* 0x39 */
	I386OP(cmp_r8_rm8),			/* 0x3a */
	I386OP(cmp_r32_rm32),		/* 0x3b */
	I386OP(cmp_al_i8),			/* 0x3c */
	I386OP(cmp_eax_i32),		/* 0x3d */
	I386OP(segment_DS),			/* 0x3e */
	I386OP(aas),				/* 0x3f */
	I386OP(inc_eax),			/* 0x40 */
	I386OP(inc_ecx),			/* 0x41 */
	I386OP(inc_edx),			/* 0x42 */
	I386OP(inc_ebx),			/* 0x43 */
	I386OP(inc_esp),			/* 0x44 */
	I386OP(inc_ebp),			/* 0x45 */
	I386OP(inc_esi),			/* 0x46 */
	I386OP(inc_edi),			/* 0x47 */
	I386OP(dec_eax),			/* 0x48 */
	I386OP(dec_ecx),			/* 0x49 */
	I386OP(dec_edx),			/* 0x4a */
	I386OP(dec_ebx),			/* 0x4b */
	I386OP(dec_esp),			/* 0x4c */
	I386OP(dec_ebp),			/* 0x4d */
	I386OP(dec_esi),			/* 0x4e */
	I386OP(dec_edi),			/* 0x4f */
	I386OP(push_eax),			/* 0x50 */
	I386OP(push_ecx),			/* 0x51 */
	I386OP(push_edx),			/* 0x52 */
	I386OP(push_ebx),			/* 0x53 */
	I386OP(push_esp),			/* 0x54 */
	I386OP(push_ebp),			/* 0x55 */
	I386OP(push_esi),			/* 0x56 */
	I386OP(push_edi),			/* 0x57 */
	I386OP(pop_eax),			/* 0x58 */
	I386OP(pop_ecx),			/* 0x59 */
	I386OP(pop_edx),			/* 0x5a */
	I386OP(pop_ebx),			/* 0x5b */
	I386OP(pop_esp),			/* 0x5c */
	I386OP(pop_ebp),			/* 0x5d */
	I386OP(pop_esi),			/* 0x5e */
	I386OP(pop_edi),			/* 0x5f */
	I386OP(pushad),				/* 0x60 */
	I386OP(popad),				/* 0x61 */
	I386OP(bound_r32_m32_m32),	/* 0x62 */
	I386OP(unimplemented),		/* 0x63 */		/* TODO: ARPL */
	I386OP(segment_FS),			/* 0x64 */
	I386OP(segment_GS),			/* 0x65 */
	I386OP(operand_size),		/* 0x66 */
	I386OP(address_size),		/* 0x67 */
	I386OP(push_i32),			/* 0x68 */
	I386OP(imul_r32_rm32_i32),	/* 0x69 */
	I386OP(push_i8),			/* 0x6a */
	I386OP(imul_r32_rm32_i8),	/* 0x6b */
	I386OP(insb),				/* 0x6c */
	I386OP(insd),				/* 0x6d */
	I386OP(outsb),				/* 0x6e */
	I386OP(outsd),				/* 0x6f */
	I386OP(jo_rel8),			/* 0x70 */
	I386OP(jno_rel8),			/* 0x71 */
	I386OP(jc_rel8),			/* 0x72 */
	I386OP(jnc_rel8),			/* 0x73 */
	I386OP(jz_rel8),			/* 0x74 */
	I386OP(jnz_rel8),			/* 0x75 */
	I386OP(jbe_rel8),			/* 0x76 */
	I386OP(ja_rel8),			/* 0x77 */
	I386OP(js_rel8),			/* 0x78 */
	I386OP(jns_rel8),			/* 0x79 */
	I386OP(jp_rel8),			/* 0x7a */
	I386OP(jnp_rel8),			/* 0x7b */
	I386OP(jl_rel8),			/* 0x7c */
	I386OP(jge_rel8),			/* 0x7d */
	I386OP(jle_rel8),			/* 0x7e */
	I386OP(jg_rel8),			/* 0x7f */
	I386OP(group80_8),			/* 0x80 */
	I386OP(group81_32),			/* 0x81 */
	I386OP(group80_8),			/* 0x82 */
	I386OP(group83_32),			/* 0x83 */
	I386OP(test_rm8_r8),		/* 0x84 */
	I386OP(test_rm32_r32),		/* 0x85 */
	I386OP(xchg_r8_rm8),		/* 0x86 */
	I386OP(xchg_r32_rm32),		/* 0x87 */
	I386OP(mov_rm8_r8),			/* 0x88 */
	I386OP(mov_rm32_r32),		/* 0x89 */
	I386OP(mov_r8_rm8),			/* 0x8a */
	I386OP(mov_r32_rm32),		/* 0x8b */
	I386OP(mov_rm16_sreg),		/* 0x8c */
	I386OP(lea32),				/* 0x8d */
	I386OP(mov_sreg_rm16),		/* 0x8e */
	I386OP(pop_rm32),			/* 0x8f */
	I386OP(nop),				/* 0x90 */
	I386OP(xchg_eax_ecx),		/* 0x91 */
	I386OP(xchg_eax_edx),		/* 0x92 */
	I386OP(xchg_eax_ebx),		/* 0x93 */
	I386OP(xchg_eax_esp),		/* 0x94 */
	I386OP(xchg_eax_ebp),		/* 0x95 */
	I386OP(xchg_eax_esi),		/* 0x96 */
	I386OP(xchg_eax_edi),		/* 0x97 */
	I386OP(cwde),				/* 0x98 */
	I386OP(cdq),				/* 0x99 */
	I386OP(call_abs32),			/* 0x9a */
	I386OP(wait),				/* 0x9b */
	I386OP(pushfd),				/* 0x9c */
	I386OP(popfd),				/* 0x9d */
	I386OP(sahf),				/* 0x9e */
	I386OP(lahf),				/* 0x9f */
	I386OP(mov_al_m8),			/* 0xa0 */
	I386OP(mov_eax_m32),		/* 0xa1 */
	I386OP(mov_m8_al),			/* 0xa2 */
	I386OP(mov_m32_eax),		/* 0xa3 */
	I386OP(movsb),				/* 0xa4 */
	I386OP(movsd),				/* 0xa5 */
	I386OP(cmpsb),				/* 0xa6 */
	I386OP(cmpsd),				/* 0xa7 */
	I386OP(test_al_i8),			/* 0xa8 */
	I386OP(test_eax_i32),		/* 0xa9 */
	I386OP(stosb),				/* 0xaa */
	I386OP(stosd),				/* 0xab */
	I386OP(lodsb),				/* 0xac */
	I386OP(lodsd),				/* 0xad */
	I386OP(scasb),				/* 0xae */
	I386OP(scasd),				/* 0xaf */
	I386OP(mov_al_i8),			/* 0xb0 */
	I386OP(mov_cl_i8),			/* 0xb1 */
	I386OP(mov_dl_i8),			/* 0xb2 */
	I386OP(mov_bl_i8),			/* 0xb3 */
	I386OP(mov_ah_i8),			/* 0xb4 */
	I386OP(mov_ch_i8),			/* 0xb5 */
	I386OP(mov_dh_i8),			/* 0xb6 */
	I386OP(mov_bh_i8),			/* 0xb7 */
	I386OP(mov_eax_i32),		/* 0xb8 */
	I386OP(mov_ecx_i32),		/* 0xb9 */
	I386OP(mov_edx_i32),		/* 0xba */
	I386OP(mov_ebx_i32),		/* 0xbb */
	I386OP(mov_esp_i32),		/* 0xbc */
	I386OP(mov_ebp_i32),		/* 0xbd */
	I386OP(mov_esi_i32),		/* 0xbe */
	I386OP(mov_edi_i32),		/* 0xbf */
	I386OP(groupC0_8),			/* 0xc0 */
	I386OP(groupC1_32),			/* 0xc1 */
	I386OP(ret_near32_i16),		/* 0xc2 */
	I386OP(ret_near32),			/* 0xc3 */
	I386OP(les32),				/* 0xc4 */
	I386OP(lds32),				/* 0xc5 */
	I386OP(mov_rm8_i8),			/* 0xc6 */
	I386OP(mov_rm32_i32),		/* 0xc7 */
	I386OP(unimplemented),		/* 0xc8 */		/* TODO: ENTER */
	I386OP(leave32),			/* 0xc9 */
	I386OP(retf_i32),			/* 0xca */
	I386OP(retf32),				/* 0xcb */
	I386OP(int3),				/* 0xcc */
	I386OP(int),				/* 0xcd */
	I386OP(into),				/* 0xce */
	I386OP(iret32),				/* 0xcf */
	I386OP(groupD0_8),			/* 0xd0 */
	I386OP(groupD1_32),			/* 0xd1 */
	I386OP(groupD2_8),			/* 0xd2 */
	I386OP(groupD3_32),			/* 0xd3 */
	I386OP(aam),				/* 0xd4 */
	I386OP(aad),				/* 0xd5 */
	I386OP(setalc),				/* 0xd6 */		/* undocumented */
	I386OP(xlat32),				/* 0xd7 */
	I386OP(escape),				/* 0xd8 */
	I386OP(escape),				/* 0xd9 */
	I386OP(escape),				/* 0xda */
	I386OP(escape),				/* 0xdb */
	I386OP(escape),				/* 0xdc */
	I386OP(escape),				/* 0xdd */
	I386OP(escape),				/* 0xde */
	I386OP(escape),				/* 0xdf */
	I386OP(loopne32),			/* 0xe0 */
	I386OP(loopz32),			/* 0xe1 */
	I386OP(loop32),				/* 0xe2 */
	I386OP(jcxz32),				/* 0xe3 */
	I386OP(in_al_i8),			/* 0xe4 */
	I386OP(in_eax_i8),			/* 0xe5 */
	I386OP(out_al_i8),			/* 0xe6 */
	I386OP(out_eax_i8),			/* 0xe7 */
	I386OP(call_rel32),			/* 0xe8 */
	I386OP(jmp_rel32),			/* 0xe9 */
	I386OP(jmp_abs32),			/* 0xea */
	I386OP(jmp_rel8),			/* 0xeb */
	I386OP(in_al_dx),			/* 0xec */
	I386OP(in_eax_dx),			/* 0xed */
	I386OP(out_al_dx),			/* 0xee */
	I386OP(out_eax_dx),			/* 0xef */
	I386OP(lock),				/* 0xf0 */
	I386OP(invalid),			/* 0xf1 */
	I386OP(repne),				/* 0xf2 */
	I386OP(rep),				/* 0xf3 */
	I386OP(hlt),				/* 0xf4 */
	I386OP(cmc),				/* 0xf5 */
	I386OP(groupF6_8),			/* 0xf6 */
	I386OP(groupF7_32),			/* 0xf7 */
	I386OP(clc),				/* 0xf8 */
	I386OP(stc),				/* 0xf9 */
	I386OP(cli),				/* 0xfa */
	I386OP(sti),				/* 0xfb */
	I386OP(cld),				/* 0xfc */
	I386OP(std),				/* 0xfd */
	I386OP(groupFE_8),			/* 0xfe */
	I386OP(groupFF_32),			/* 0xff */
};



static void (*i386_opcode_table2_32[256])(void) =
{
	I386OP(group0F00_32),		/* 0x00 */
	I386OP(group0F01_32),		/* 0x01 */
	I386OP(unimplemented),		/* 0x02 */		/* TODO: LAR */
	I386OP(unimplemented),		/* 0x03 */		/* TODO: LSL */
	I386OP(invalid),			/* 0x04 */
	I386OP(invalid),			/* 0x05 */
	I386OP(clts),				/* 0x06 */
	I386OP(invalid),			/* 0x07 */
	I386OP(invalid),			/* 0x08 */		/* INVD (486) */
	I386OP(invalid),			/* 0x09 */		/* WBINVD (486) */
	I386OP(invalid),			/* 0x0a */
	I386OP(unimplemented),		/* 0x0b */		/* TODO: UD2 */
	I386OP(invalid),			/* 0x0c */
	I386OP(invalid),			/* 0x0d */
	I386OP(invalid),			/* 0x0e */
	I386OP(invalid),			/* 0x0f */
	I386OP(invalid),			/* 0x10 */		/* SSE */
	I386OP(invalid),			/* 0x11 */		/* SSE */
	I386OP(invalid),			/* 0x12 */		/* SSE */
	I386OP(invalid),			/* 0x13 */		/* SSE */
	I386OP(invalid),			/* 0x14 */		/* SSE */
	I386OP(invalid),			/* 0x15 */		/* SSE */
	I386OP(invalid),			/* 0x16 */		/* SSE */
	I386OP(invalid),			/* 0x17 */		/* SSE */
	I386OP(invalid),			/* 0x18 */		/* SSE */
	I386OP(invalid),			/* 0x19 */
	I386OP(invalid),			/* 0x1a */
	I386OP(invalid),			/* 0x1b */
	I386OP(invalid),			/* 0x1c */
	I386OP(invalid),			/* 0x1d */
	I386OP(invalid),			/* 0x1e */
	I386OP(invalid),			/* 0x1f */
	I386OP(mov_r32_cr),			/* 0x20 */
	I386OP(mov_r32_dr),			/* 0x21 */
	I386OP(mov_cr_r32),			/* 0x22 */
	I386OP(mov_dr_r32),			/* 0x23 */
	I386OP(unimplemented),		/* 0x24 */		/* TODO: MOV R32, TR */
	I386OP(invalid),			/* 0x25 */
	I386OP(unimplemented),		/* 0x26 */		/* TODO: MOV TR, R32 */
	I386OP(invalid),			/* 0x27 */
	I386OP(invalid),			/* 0x28 */		/* SSE */
	I386OP(invalid),			/* 0x29 */		/* SSE */
	I386OP(invalid),			/* 0x2a */		/* SSE */
	I386OP(invalid),			/* 0x2b */		/* SSE */
	I386OP(invalid),			/* 0x2c */		/* SSE */
	I386OP(invalid),			/* 0x2d */		/* SSE */
	I386OP(invalid),			/* 0x2e */		/* SSE */
	I386OP(invalid),			/* 0x2f */		/* SSE */
	I386OP(invalid),			/* 0x30 */		/* WRMSR (Pentium) */
	I386OP(invalid),			/* 0x31 */		/* RDTSC (Pentium) */
	I386OP(invalid),			/* 0x32 */		/* RDMSR (Pentium) */
	I386OP(invalid),			/* 0x33 */		/* RDPMC (Pentium MMX+) */
	I386OP(invalid),			/* 0x34 */		/* SYSENTER (Pentium II) */
	I386OP(invalid),			/* 0x35 */		/* SYSEXIT (Pentium II) */
	I386OP(invalid),			/* 0x36 */
	I386OP(invalid),			/* 0x37 */
	I386OP(invalid),			/* 0x38 */
	I386OP(invalid),			/* 0x39 */
	I386OP(invalid),			/* 0x3a */
	I386OP(invalid),			/* 0x3b */
	I386OP(invalid),			/* 0x3c */		/* SSE */
	I386OP(invalid),			/* 0x3d */
	I386OP(invalid),			/* 0x3e */
	I386OP(invalid),			/* 0x3f */
	I386OP(invalid),			/* 0x40 */		/* CMOVO (Pentium Pro) */
	I386OP(invalid),			/* 0x41 */		/* CMOVNO (Pentium Pro) */
	I386OP(invalid),			/* 0x42 */		/* CMOVC (Pentium Pro) */
	I386OP(invalid),			/* 0x43 */		/* CMOVNC (Pentium Pro) */
	I386OP(invalid),			/* 0x44 */		/* CMOVZ (Pentium Pro) */
	I386OP(invalid),			/* 0x45 */		/* CMOVNZ (Pentium Pro) */
	I386OP(invalid),			/* 0x46 */		/* CMOVBE (Pentium Pro) */
	I386OP(invalid),			/* 0x47 */		/* CMOVA (Pentium Pro) */
	I386OP(invalid),			/* 0x48 */		/* CMOVS (Pentium Pro) */
	I386OP(invalid),			/* 0x49 */		/* CMOVNS (Pentium Pro) */
	I386OP(invalid),			/* 0x4a */		/* CMOVP (Pentium Pro) */
	I386OP(invalid),			/* 0x4b */		/* CMOVNP (Pentium Pro) */
	I386OP(invalid),			/* 0x4c */		/* CMOVL (Pentium Pro) */
	I386OP(invalid),			/* 0x4d */		/* CMOVGE (Pentium Pro) */
	I386OP(invalid),			/* 0x4e */		/* CMOVLE (Pentium Pro) */
	I386OP(invalid),			/* 0x4f */		/* CMOVG (Pentium Pro) */
	I386OP(invalid),			/* 0x50 */		/* SSE */
	I386OP(invalid),			/* 0x51 */		/* SSE */
	I386OP(invalid),			/* 0x52 */		/* SSE */
	I386OP(invalid),			/* 0x53 */		/* SSE */
	I386OP(invalid),			/* 0x54 */		/* SSE */
	I386OP(invalid),			/* 0x55 */		/* SSE */
	I386OP(invalid),			/* 0x56 */		/* SSE */
	I386OP(invalid),			/* 0x57 */		/* SSE */
	I386OP(invalid),			/* 0x58 */		/* SSE */
	I386OP(invalid),			/* 0x59 */		/* SSE */
	I386OP(invalid),			/* 0x5a */		/* SSE */
	I386OP(invalid),			/* 0x5b */		/* SSE */
	I386OP(invalid),			/* 0x5c */		/* SSE */
	I386OP(invalid),			/* 0x5d */		/* SSE */
	I386OP(invalid),			/* 0x5e */		/* SSE */
	I386OP(invalid),			/* 0x5f */		/* SSE */
	I386OP(invalid),			/* 0x60 */		/* MMX */
	I386OP(invalid),			/* 0x61 */		/* MMX */
	I386OP(invalid),			/* 0x62 */		/* MMX */
	I386OP(invalid),			/* 0x63 */		/* MMX */
	I386OP(invalid),			/* 0x64 */		/* MMX */
	I386OP(invalid),			/* 0x65 */		/* MMX */
	I386OP(invalid),			/* 0x66 */		/* MMX */
	I386OP(invalid),			/* 0x67 */		/* MMX */
	I386OP(invalid),			/* 0x68 */		/* MMX */
	I386OP(invalid),			/* 0x69 */		/* MMX */
	I386OP(invalid),			/* 0x6a */		/* MMX */
	I386OP(invalid),			/* 0x6b */		/* MMX */
	I386OP(invalid),			/* 0x6c */		/* MMX */
	I386OP(invalid),			/* 0x6d */		/* MMX */
	I386OP(invalid),			/* 0x6e */		/* MMX */
	I386OP(invalid),			/* 0x6f */		/* MMX */
	I386OP(invalid),			/* 0x70 */		/* MMX */
	I386OP(invalid),			/* 0x71 */		/* MMX */
	I386OP(invalid),			/* 0x72 */		/* MMX */
	I386OP(invalid),			/* 0x73 */		/* MMX */
	I386OP(invalid),			/* 0x74 */		/* MMX */
	I386OP(invalid),			/* 0x75 */		/* MMX */
	I386OP(invalid),			/* 0x76 */		/* MMX */
	I386OP(invalid),			/* 0x77 */		/* MMX */
	I386OP(invalid),			/* 0x78 */		/* MMX UD */
	I386OP(invalid),			/* 0x79 */		/* MMX UD */
	I386OP(invalid),			/* 0x7a */		/* MMX UD */
	I386OP(invalid),			/* 0x7b */		/* MMX UD */
	I386OP(invalid),			/* 0x7c */		/* MMX UD */
	I386OP(invalid),			/* 0x7d */		/* MMX UD */
	I386OP(invalid),			/* 0x7e */		/* MMX */
	I386OP(invalid),			/* 0x7f */		/* MMX */
	I386OP(jo_rel32),			/* 0x80 */
	I386OP(jno_rel32),			/* 0x81 */
	I386OP(jc_rel32),			/* 0x82 */
	I386OP(jnc_rel32),			/* 0x83 */
	I386OP(jz_rel32),			/* 0x84 */
	I386OP(jnz_rel32),			/* 0x85 */
	I386OP(jbe_rel32),			/* 0x86 */
	I386OP(ja_rel32),			/* 0x87 */
	I386OP(js_rel32),			/* 0x88 */
	I386OP(jns_rel32),			/* 0x89 */
	I386OP(jp_rel32),			/* 0x8a */
	I386OP(jnp_rel32),			/* 0x8b */
	I386OP(jl_rel32),			/* 0x8c */
	I386OP(jge_rel32),			/* 0x8d */
	I386OP(jle_rel32),			/* 0x8e */
	I386OP(jg_rel32),			/* 0x8f */
	I386OP(seto_rm8),			/* 0x90 */
	I386OP(setno_rm8),			/* 0x91 */
	I386OP(setc_rm8),			/* 0x92 */
	I386OP(setnc_rm8),			/* 0x93 */
	I386OP(setz_rm8),			/* 0x94 */
	I386OP(setnz_rm8),			/* 0x95 */
	I386OP(setbe_rm8),			/* 0x96 */
	I386OP(seta_rm8),			/* 0x97 */
	I386OP(sets_rm8),			/* 0x98 */
	I386OP(setns_rm8),			/* 0x99 */
	I386OP(setp_rm8),			/* 0x9a */
	I386OP(setnp_rm8),			/* 0x9b */
	I386OP(setl_rm8),			/* 0x9c */
	I386OP(setge_rm8),			/* 0x9d */
	I386OP(setle_rm8),			/* 0x9e */
	I386OP(setg_rm8),			/* 0x9f */
	I386OP(push_fs32),			/* 0xa0 */
	I386OP(pop_fs32),			/* 0xa1 */
	I386OP(invalid),			/* 0xa2 */		/* CPUID (486) */
	I386OP(bt_rm32_r32),		/* 0xa3 */
	I386OP(shld32_i8),			/* 0xa4 */
	I386OP(shld32_cl),			/* 0xa5 */
	I386OP(invalid),			/* 0xa6 */
	I386OP(invalid),			/* 0xa7 */
	I386OP(push_gs32),			/* 0xa8 */
	I386OP(pop_gs32),			/* 0xa9 */
	I386OP(unimplemented),		/* 0xaa */		/* TODO: RSM */
	I386OP(bts_rm32_r32),		/* 0xab */
	I386OP(shrd32_i8),			/* 0xac */
	I386OP(shrd32_cl),			/* 0xad */
	I386OP(invalid),			/* 0xae */		/* SSE */
	I386OP(imul_r32_rm32),		/* 0xaf */
	I386OP(invalid),			/* 0xb0 */		/* CMPXCHG (486) */
	I386OP(invalid),			/* 0xb1 */		/* CMPXCHG (486) */
	I386OP(lss32),				/* 0xb2 */
	I386OP(btr_rm32_r32),		/* 0xb3 */
	I386OP(lfs32),				/* 0xb4 */
	I386OP(lgs32),				/* 0xb5 */
	I386OP(movzx_r32_rm8),		/* 0xb6 */
	I386OP(movzx_r32_rm16),		/* 0xb7 */
	I386OP(invalid),			/* 0xb8 */
	I386OP(invalid),			/* 0xb9 */
	I386OP(group0FBA_32),		/* 0xba */
	I386OP(btc_rm32_r32),		/* 0xbb */
	I386OP(bsf_r32_rm32),		/* 0xbc */
	I386OP(bsr_r32_rm32),		/* 0xbd */
	I386OP(movsx_r32_rm8),		/* 0xbe */
	I386OP(movsx_r32_rm16),		/* 0xbf */
	I386OP(invalid),			/* 0xc0 */		/* XADD (486) */
	I386OP(invalid),			/* 0xc1 */		/* XADD (486) */
	I386OP(invalid),			/* 0xc2 */		/* SSE */
	I386OP(invalid),			/* 0xc3 */		/* SSE */
	I386OP(invalid),			/* 0xc4 */		/* SSE */
	I386OP(invalid),			/* 0xc5 */		/* SSE */
	I386OP(invalid),			/* 0xc6 */		/* SSE */
	I386OP(invalid),			/* 0xc7 */		/* CMPXCHG8B (Pentium) */
	I386OP(invalid),			/* 0xc8 */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xc9 */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xca */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xcb */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xcc */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xcd */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xce */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xcf */		/* BSWAP (486) */
	I386OP(invalid),			/* 0xd0 */
	I386OP(invalid),			/* 0xd1 */		/* MMX */
	I386OP(invalid),			/* 0xd2 */		/* MMX */
	I386OP(invalid),			/* 0xd3 */		/* MMX */
	I386OP(invalid),			/* 0xd4 */		/* MMX */
	I386OP(invalid),			/* 0xd5 */		/* MMX */
	I386OP(invalid),			/* 0xd6 */		/* MMX */
	I386OP(invalid),			/* 0xd7 */		/* MMX */
	I386OP(invalid),			/* 0xd8 */		/* MMX */
	I386OP(invalid),			/* 0xd9 */		/* MMX */
	I386OP(invalid),			/* 0xda */		/* MMX */
	I386OP(invalid),			/* 0xdb */		/* MMX */
	I386OP(invalid),			/* 0xdc */		/* MMX */
	I386OP(invalid),			/* 0xdd */		/* MMX */
	I386OP(invalid),			/* 0xde */		/* MMX */
	I386OP(invalid),			/* 0xdf */		/* MMX */
	I386OP(invalid),			/* 0xe0 */		/* MMX */
	I386OP(invalid),			/* 0xe1 */		/* MMX */
	I386OP(invalid),			/* 0xe2 */		/* MMX */
	I386OP(invalid),			/* 0xe3 */		/* MMX */
	I386OP(invalid),			/* 0xe4 */		/* MMX */
	I386OP(invalid),			/* 0xe5 */		/* MMX */
	I386OP(invalid),			/* 0xe6 */		/* SSE */
	I386OP(invalid),			/* 0xe7 */		/* SSE */
	I386OP(invalid),			/* 0xe8 */		/* MMX */
	I386OP(invalid),			/* 0xe9 */		/* MMX */
	I386OP(invalid),			/* 0xea */		/* MMX */
	I386OP(invalid),			/* 0xeb */		/* MMX */
	I386OP(invalid),			/* 0xec */		/* MMX */
	I386OP(invalid),			/* 0xed */		/* MMX */
	I386OP(invalid),			/* 0xee */		/* MMX */
	I386OP(invalid),			/* 0xef */		/* MMX */
	I386OP(invalid),			/* 0xf0 */		/* MMX */
	I386OP(invalid),			/* 0xf1 */		/* MMX */
	I386OP(invalid),			/* 0xf2 */		/* MMX */
	I386OP(invalid),			/* 0xf3 */		/* MMX */
	I386OP(invalid),			/* 0xf4 */		/* MMX */
	I386OP(invalid),			/* 0xf5 */		/* MMX */
	I386OP(invalid),			/* 0xf6 */		/* MMX */
	I386OP(invalid),			/* 0xf7 */		/* MMX */
	I386OP(invalid),			/* 0xf8 */		/* MMX */
	I386OP(invalid),			/* 0xf9 */		/* MMX */
	I386OP(invalid),			/* 0xfa */		/* MMX */
	I386OP(invalid),			/* 0xfb */		/* MMX */
	I386OP(invalid),			/* 0xfc */		/* MMX */
	I386OP(invalid),			/* 0xfd */		/* MMX */
	I386OP(invalid),			/* 0xfe */		/* MMX */
	I386OP(invalid),			/* 0xff */
};
