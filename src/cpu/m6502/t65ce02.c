/*****************************************************************************
 *
 *	 tbl65sc02.c
 *	 65sc02 opcode functions and function pointer table
 *
 *	 Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
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

#undef	OP
#define OP(nn) INLINE void m65ce02_##nn(void)

/*****************************************************************************
 *****************************************************************************
 *
 *	 overrides for 65C02 opcodes
 *
 *****************************************************************************
 * op	 temp	  cycles			 rdmem	 opc  wrmem   ********************/
#define m6502_00 m65ce02_00									/* 7 BRK */
#define m6502_20 m65ce02_20									/* 6 JSR ABS */
#define m6502_40 m65ce02_40									/* 6 RTI */
#define m6502_60 m65ce02_60									/* 6 RTS */
#define m65c02_80 m65ce02_80                                /* 2 BRA */
#define m6502_a0 m65ce02_a0									/* 2 LDY IMM */
#define m6502_c0 m65ce02_c0									/* 2 CPY IMM */
#define m6502_e0 m65ce02_e0									/* 2 CPX IMM */

#define m6502_10 m65ce02_10									/* 2 BPL */
#define m6502_30 m65ce02_30									/* 2 BMI */
#define m6502_50 m65ce02_50									/* 2 BVC */
#define m6502_70 m65ce02_70									/* 2 BVS */
#define m6502_90 m65ce02_90									/* 2 BCC */
#define m6502_b0 m65ce02_b0									/* 2 BCS */
#define m6502_d0 m65ce02_d0									/* 2 BNE */
#define m6502_f0 m65ce02_f0									/* 2 BEQ */

#define m6502_01 m65ce02_01									/* 6 ORA IDX */
#define m6502_21 m65ce02_21									/* 6 AND IDX */
#define m6502_41 m65ce02_41									/* 6 EOR IDX */
#define m6502_61 m65ce02_61									/* 6 ADC IDX */
#define m6502_81 m65ce02_81									/* 6 STA IDX */
#define m6502_a1 m65ce02_a1									/* 6 LDA IDX */
#define m6502_c1 m65ce02_c1									/* 6 CMP IDX */
#define m6502_e1 m65ce02_e1									/* 6 SBC IDX */

#define m6502_11 m65ce02_11									/* 5 ORA IDY; */
#define m6502_31 m65ce02_31									/* 5 AND IDY; */
#define m6502_51 m65ce02_51									/* 5 EOR IDY; */
#define m6502_71 m65ce02_71									/* 5 ADC IDY; */
#define m6502_91 m65ce02_91									/* 6 STA IDY; */
#define m6502_b1 m65ce02_b1									/* 5 LDA IDY; */
#define m6502_d1 m65ce02_d1									/* 5 CMP IDY; */
#define m6502_f1 m65ce02_f1									/* 5 SBC IDY; */

OP(02) {		  m6502_ICount -= 2;		 CLE;		  } /* ? CLE */
OP(22) {          m6502_ICount -= 5;         JSR_IND;		  } /* ? JSR IND */
OP(42) {		  m6502_ICount -= 2;		 NEG;		  } /* 2 NEG */
OP(62) { int tmp; m6502_ICount -= 2;RD_IMM;	 RTS_IMM;	  } /* ? RTS IMM */
OP(82) { int tmp; m6502_ICount -= 5; RD_INSY; STA;  } /* 5 STA INSY */
#define m6502_a2 m65ce02_a2									/* 2 LDX IMM */
OP(c2) { int tmp; m6502_ICount -= 2; RD_IMM; CPZ;		  } /* 2 CPZ IMM */
OP(e2) { int tmp; m6502_ICount -= 5; RD_INSY; LDA;  } /* ? LDA INSY */

OP(12) { int tmp; m6502_ICount -= 5; RD_IDZ; ORA;		  } /* 5 ORA IDZ */
OP(32) { int tmp; m6502_ICount -= 5; RD_IDZ; AND;		  } /* 5 AND IDZ */
OP(52) { int tmp; m6502_ICount -= 5; RD_IDZ; EOR;		  } /* 5 EOR IDZ */
OP(72) { int tmp; m6502_ICount -= 5; RD_IDZ; ADC;		  } /* 5 ADC IDZ */
OP(92) { int tmp; m6502_ICount -= 5; RD_IDZ; STA;		  } /* 5 STA IDZ */
OP(b2) { int tmp; m6502_ICount -= 5; RD_IDZ; LDA;		  } /* 5 LDA IDZ */
OP(d2) { int tmp; m6502_ICount -= 5; RD_IDZ; CMP;		  } /* 5 CMP IDZ */
OP(f2) { int tmp; m6502_ICount -= 5; RD_IDZ; SBC;		  } /* 5 SBC IDZ */

OP(03) {		  m6502_ICount -= 2;		 SEE;		  } /* ? SEE */
OP(23) {          m6502_ICount -= 6;         JSR_INDX;		  } /* ? JSR INDX */
OP(43) { int tmp; m6502_ICount -= 2; RD_ACC; ASR_65CE02; WB_ACC; } /* 2 ASR A */
#define m65sc02_63 m65ce02_63								/* ? BSR */
OP(83) {							   BRA_WORD(1);	   } /* ? BRA REL WORD */
OP(a3) { int tmp; m6502_ICount -= 2; RD_IMM; LDZ;		  } /* 2 LDZ IMM */
OP(c3) { PAIR tmp; m6502_ICount -= 9; RD_ZPG_WORD; DEW; WB_EA_WORD;  } /* ? DEW ABS */
OP(e3) { PAIR tmp; m6502_ICount -= 9; RD_ZPG_WORD; INW; WB_EA_WORD;  } /* ? INW ABS */

OP(13) { 							 BPL_WORD;	   } /* ? BPL REL WORD */
OP(33) { 							 BMI_WORD;	   } /* ? BMI REL WORD */
OP(53) { 							 BVC_WORD;	   } /* ? BVC REL WORD */
OP(73) { 							 BVS_WORD;	   } /* ? BVS REL WORD */
OP(93) { 							 BCC_WORD;	   } /* ? BCC REL WORD */
OP(b3) { 							 BCS_WORD;	   } /* ? BCS REL WORD */
OP(d3) { 							 BNE_WORD;	   } /* ? BNE REL WORD */
OP(f3) { 							 BEQ_WORD;	   } /* ? BEQ REL WORD */

#define m65c02_04 m65ce02_04                                /* 3 tsb zpg */
#define m6502_24 m65ce02_24									/* 3 BIT ZPG */
OP(44) { int tmp; m6502_ICount -= 5; RD_ZPG; ASR_65CE02; WB_EA;  } /* 5 ASR ZPG */
OP(64) { int tmp; m6502_ICount -= 3;		 STZ_65CE02; WR_ZPG; } /* 3 STZ ZPG */
#define m6502_84 m65ce02_84									/* 3 STY ZPG */
#define m6502_a4 m65ce02_a4									/* 3 LDY ZPG */
#define m6502_c4 m65ce02_c4									/* 3 CPY ZPG */
#define m6502_e4 m65ce02_e4									/* 3 CPX ZPG */

#define m65c02_14 m65ce02_14                                /* 3 trb zpg */
#define m65c02_34 m65ce02_34                                /* 4 bit zpx */
OP(54) { int tmp; m6502_ICount -= 6; RD_ZPX; ASR_65CE02; WB_EA;  } /* 6 ASR ZPX */
OP(74) { int tmp; m6502_ICount -= 4;		 STZ_65CE02; WR_ZPX; } /* 4 STZ ZPX */
#define m6502_94 m65ce02_94                                /* 4 sty zpx */

#define m6502_b4 m65ce02_b4                                /* 4 ldy zpx */
OP(d4) { int tmp; m6502_ICount -= 3; RD_ZPG; CPZ;		  } /* 3 CPZ ZPG */
OP(f4) { PAIR tmp; m6502_ICount -= 6; RD_IMM_WORD; PUSH_WORD(tmp); } /* ? PHW imm16 */

#define m6502_05 m65ce02_05									/* 3 ORA ZPG */
#define m6502_25 m65ce02_25									/* 3 AND ZPG */
#define m6502_45 m65ce02_45									/* 3 EOR ZPG */
#define m6502_65 m65ce02_65									/* 3 ADC ZPG */
#define m6502_85 m65ce02_85									/* 3 STA ZPG */
#define m6502_a5 m65ce02_a5									/* 3 LDA ZPG */
#define m6502_c5 m65ce02_c5									/* 3 CMP ZPG */
#define m6502_e5 m65ce02_e5									/* 3 SBC ZPG */

#define m6502_15 m65ce02_15									/* 4 ORA ZPX */
#define m6502_35 m65ce02_35									/* 4 AND ZPX */
#define m6502_55 m65ce02_55									/* 4 EOR ZPX */
#define m6502_75 m65ce02_75									/* 4 ADC ZPX */
#define m6502_95 m65ce02_95									/* 4 STA ZPX */
#define m6502_b5 m65ce02_b5									/* 4 LDA ZPX */
#define m6502_d5 m65ce02_d5									/* 4 CMP ZPX */
#define m6502_f5 m65ce02_f5									/* 4 SBC ZPX */

#define m6502_06 m65ce02_06									/* 5 ASL ZPG */
#define m6502_26 m65ce02_26									/* 5 ROL ZPG */
#define m6502_46 m65ce02_46									/* 5 LSR ZPG */
#define m6502_66 m65ce02_66									/* 5 ROR ZPG */
#define m6502_86 m65ce02_86									/* 3 STX ZPG */
#define m6502_a6 m65ce02_a6									/* 3 LDX ZPG */
#define m6502_c6 m65ce02_c6									/* 5 DEC ZPG */
#define m6502_e6 m65ce02_e6									/* 5 INC ZPG */

#define m6502_16 m65ce02_16									/* 6 ASL ZPX */
#define m6502_36 m65ce02_36									/* 6 ROL ZPX */
#define m6502_56 m65ce02_56									/* 6 LSR ZPX */
#define m6502_76 m65ce02_76									/* 6 ROR ZPX */
#define m6502_96 m65ce02_96									/* 4 STX ZPY */
#define m6502_b6 m65ce02_b6									/* 4 LDX ZPY */
#define m6502_d6 m65ce02_d6									/* 6 DEC ZPX */
#define m6502_f6 m65ce02_f6									/* 6 INC ZPX */

#define m65c02_07 m65ce02_07                                /* 5 RMB0 ZPG */
#define m65c02_27 m65ce02_27                                /* 5 RMB2 ZPG */
#define m65c02_47 m65ce02_47                                /* 5 RMB4 ZPG */
#define m65c02_67 m65ce02_67                                /* 5 RMB6 ZPG */
#define m65c02_87 m65ce02_87                                /* 5 SMB0 ZPG */
#define m65c02_a7 m65ce02_a7                                /* 5 SMB2 ZPG */
#define m65c02_c7 m65ce02_c7                                /* 5 SMB4 ZPG */
#define m65c02_e7 m65ce02_e7                                /* 5 SMB6 ZPG */

#define m65c02_17 m65ce02_17                                /* 5 RMB1 ZPG */
#define m65c02_37 m65ce02_37                                /* 5 RMB3 ZPG */
#define m65c02_57 m65ce02_57                                /* 5 RMB5 ZPG */
#define m65c02_77 m65ce02_77                                /* 5 RMB7 ZPG */
#define m65c02_97 m65ce02_97                                /* 5 SMB1 ZPG */
#define m65c02_b7 m65ce02_b7                                /* 5 SMB3 ZPG */
#define m65c02_d7 m65ce02_d7                                /* 5 SMB5 ZPG */
#define m65c02_f7 m65ce02_f7                                /* 5 SMB7 ZPG */

#define m6502_08 m65ce02_08                                  /* 3 PHP */
#define m6502_28 m65ce02_28									/* 4 PLP */
#define m6502_48 m65ce02_48									/* 3 PHA */
#define m6502_68 m65ce02_68									/* 4 PLA */
#define m6502_88 m65ce02_88									/* 2 DEY */
#define m6502_a8 m65ce02_a8									/* 2 TAY */
#define m6502_c8 m65ce02_c8									/* 2 INY */
#define m6502_e8 m65ce02_e8									/* 2 INX */

#define m6502_18 m65ce02_18									/* 2 CLC */
#define m6502_38 m65ce02_38									/* 2 SEC */
#define m6502_58 m65ce02_58									/* 2 CLI */
#define m6502_78 m65ce02_78									/* 2 SEI */
#define m6502_98 m65ce02_98									/* 2 TYA */
#define m6502_b8 m65ce02_b8									/* 2 CLV */
#define m6502_d8 m65ce02_d8									/* 2 CLD */
#define m6502_f8 m65ce02_f8									/* 2 SED */

#define m6502_09 m65ce02_09									/* 2 ORA IMM */
#define m6502_29 m65ce02_29									/* 2 AND IMM */
#define m6502_49 m65ce02_49									/* 2 EOR IMM */
#define m6502_69 m65ce02_69									/* 2 ADC IMM */
#define m65c02_89 m65ce02_89                                /* 2 BIT IMM */
#define m6502_a9 m65ce02_a9									/* 2 LDA IMM */
#define m6502_c9 m65ce02_c9									/* 2 CMP IMM */
#define m6502_e9 m65ce02_e9									/* 2 SBC IMM */

#define m6502_19 m65ce02_19									/* 4 ORA ABY */
#define m6502_39 m65ce02_39									/* 4 AND ABY */
#define m6502_59 m65ce02_59									/* 4 EOR ABY */
#define m6502_79 m65ce02_79									/* 4 ADC ABY */
#define m6502_99 m65ce02_99									/* 5 STA ABY */
#define m6502_b9 m65ce02_b9									/* 4 LDA ABY */
#define m6502_d9 m65ce02_d9									/* 4 CMP ABY */
#define m6502_f9 m65ce02_f9									/* 4 SBC ABY */

#define m6502_0a m65ce02_0a									/* 2 ASL */
#define m6502_2a m65ce02_2a									/* 2 ROL */
#define m6502_4a m65ce02_4a									/* 2 LSR */
#define m6502_6a m65ce02_6a									/* 2 ROR */
#define m6502_8a m65ce02_8a									/* 2 TXA */
#define m6502_aa m65ce02_aa									/* 2 TAX */
#define m6502_ca m65ce02_ca									/* 2 DEX */
#define m6502_ea m65ce02_ea									/* 2 NOP */

#define m65c02_1a m65ce02_1a                             /* 2 INA */
#define m65c02_3a m65ce02_3a                             /* 2 DEA */
#define m65c02_5a m65ce02_5a                             /* 3 PHY */
#define m65c02_7a m65ce02_7a                             /* 4 PLY */
#define m6502_9a m65ce02_9a									/* 2 TXS */
#define m6502_ba m65ce02_ba									/* 2 TSX */
#define m65c02_da m65ce02_da                             /* 3 PHX */
#define m65c02_fa m65ce02_fa                             /* 4 PLX */

OP(0b) {		  m6502_ICount -= 2;		 TSY;		  } /* 2 TSY */
OP(2b) {		  m6502_ICount -= 2;		 TYS;		  } /* 2 TYS */
OP(4b) {		  m6502_ICount -= 2;		 TAZ;		  } /* 2 TAZ */
OP(6b) {		  m6502_ICount -= 2;		 TZA;		  } /* 2 TZA */
OP(8b) { int tmp; m6502_ICount -= 5;		 STY; WR_ABX; } /* 5 STY ABX */
OP(ab) { int tmp; m6502_ICount -= 4; RD_ABS; LDZ;		  } /* 4 LDZ ABS */
OP(cb) { PAIR tmp; m6502_ICount -= 8; RD_ABS_WORD; ASW; WB_EA_WORD;  } /* ? ASW ABS */
OP(eb) { PAIR tmp; m6502_ICount -= 8; RD_ABS_WORD; ROW; WB_EA_WORD;  } /* ? roW ABS */

OP(1b) {		  m6502_ICount -= 2;		 INZ;		  } /* 2 INZ */
OP(3b) {		  m6502_ICount -= 2;		 DEZ;		  } /* 2 DEZ */
OP(5b) {		  m6502_ICount -= 2;		 TAB;		  } /* 2 TAB */
OP(7b) {		  m6502_ICount -= 2;		 TBA;		  } /* 2 TBA */
OP(9b) { int tmp; m6502_ICount -= 5;		 STX; WR_ABY; } /* 5 STX ABY */
OP(bb) { int tmp; m6502_ICount -= 4; RD_ABX; LDZ;		  } /* 4 LDZ ABX */
OP(db) {		  m6502_ICount -= 2;		 PHZ;		  } /* 2 PHZ */
OP(fb) {		  m6502_ICount -= 2;		 PLZ;		  } /* 2 PLZ */

#define m65c02_0c m65ce02_0c                                /* 4 TSB ABS */
#define m6502_2c m65ce02_2c									/* 4 BIT ABS */
#define m6502_4c m65ce02_4c									/* 3 JMP ABS */
#define m6502_6c m65ce02_6c									/* 5 JMP IND */
#define m6502_8c m65ce02_8c									/* 4 STY ABS */
#define m6502_ac m65ce02_ac									/* 4 LDY ABS */
#define m6502_cc m65ce02_cc									/* 4 CPY ABS */
#define m6502_ec m65ce02_ec									/* 4 CPX ABS */

#define m65c02_1c m65ce02_1c                                /* 4 TRB ABS */
#define m65c02_3c m65ce02_3c                                /* 4 BIT ABX */
OP(5c) {		  m6502_ICount -= 2;		 MAP;		  } /* ? MAP */
#define m65c02_7c m65ce02_7c                                /* 6 JMP IAX */
OP(9c) { int tmp; m6502_ICount -= 4;		 STZ_65CE02; WR_ABS; } /* 4 STZ ABS */
#define m6502_bc m65ce02_bc                                /* 4 LDY ABX */
OP(dc) { int tmp; m6502_ICount -= 3; RD_ABS; CPZ;		  } /* 3 CPZ ABS */
OP(fc) { PAIR tmp; m6502_ICount -= 6; RD_ABS_WORD; PUSH_WORD(tmp); } /* ? PHW ab */

#define m6502_0d m65ce02_0d									/* 4 ORA ABS */
#define m6502_2d m65ce02_2d									/* 4 AND ABS */
#define m6502_4d m65ce02_4d									/* 4 EOR ABS */
#define m6502_6d m65ce02_6d									/* 4 ADC ABS */
#define m6502_8d m65ce02_8d									/* 4 STA ABS */
#define m6502_ad m65ce02_ad									/* 4 LDA ABS */
#define m6502_cd m65ce02_cd									/* 4 CMP ABS */
#define m6502_ed m65ce02_ed									/* 4 SBC ABS */

#define m6502_1d m65ce02_1d									/* 4 ORA ABX */
#define m6502_3d m65ce02_3d									/* 4 AND ABX */
#define m6502_5d m65ce02_5d									/* 4 EOR ABX */
#define m6502_7d m65ce02_7d									/* 4 ADC ABX */
#define m6502_9d m65ce02_9d									/* 5 STA ABX */
#define m6502_bd m65ce02_bd									/* 4 LDA ABX */
#define m6502_dd m65ce02_dd									/* 4 CMP ABX */
#define m6502_fd m65ce02_fd									/* 4 SBC ABX */

#define m6502_0e m65ce02_0e									/* 6 ASL ABS */
#define m6502_2e m65ce02_2e									/* 6 ROL ABS */
#define m6502_4e m65ce02_4e									/* 6 LSR ABS */
#define m6502_6e m65ce02_6e									/* 6 ROR ABS */
#define m6502_8e m65ce02_8e									/* 4 STX ABS */
#define m6502_ae m65ce02_ae									/* 4 LDX ABS */
#define m6502_ce m65ce02_ce									/* 6 DEC ABS */
#define m6502_ee m65ce02_ee									/* 6 INC ABS */

#define m6502_1e m65ce02_1e									/* 7 ASL ABX */
#define m6502_3e m65ce02_3e									/* 7 ROL ABX */
#define m6502_5e m65ce02_5e									/* 7 LSR ABX */
#define m6502_7e m65ce02_7e									/* 7 ROR ABX */
OP(9e) { int tmp; m6502_ICount -= 5;		 STZ_65CE02; WR_ABX; } /* 5 STZ ABX */
#define m6502_be m65ce02_be									/* 4 LDX ABY */
#define m6502_de m65ce02_de									/* 7 DEC ABX */
#define m6502_fe m65ce02_fe									/* 7 INC ABX */

#define m65c02_0f m65ce02_0f                             /* 5 BBR0 ZPG */
#define m65c02_2f m65ce02_2f                             /* 5 BBR2 ZPG */
#define m65c02_4f m65ce02_4f                             /* 5 BBR4 ZPG */
#define m65c02_6f m65ce02_6f                             /* 5 BBR6 ZPG */
#define m65c02_8f m65ce02_8f                             /* 5 BBS0 ZPG */
#define m65c02_af m65ce02_af                             /* 5 BBS2 ZPG */
#define m65c02_cf m65ce02_cf                             /* 5 BBS4 ZPG */
#define m65c02_ef m65ce02_ef                             /* 5 BBS6 ZPG */

#define m65c02_1f m65ce02_1f                             /* 5 BBR1 ZPG */
#define m65c02_3f m65ce02_3f                             /* 5 BBR3 ZPG */
#define m65c02_5f m65ce02_5f                             /* 5 BBR5 ZPG */
#define m65c02_7f m65ce02_7f                             /* 5 BBR7 ZPG */
#define m65c02_9f m65ce02_9f                             /* 5 BBS1 ZPG */
#define m65c02_bf m65ce02_bf                             /* 5 BBS3 ZPG */
#define m65c02_df m65ce02_df                             /* 5 BBS5 ZPG */
#define m65c02_ff m65ce02_ff                             /* 5 BBS7 ZPG */


