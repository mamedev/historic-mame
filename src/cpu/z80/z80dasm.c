/*****************************************************************************
 *
 *	 z80dasm.c
 *	 Portable Z80 disassembler
 *
 *	 Copyright (C) 1998 Juergen Buchmueller, all rights reserved.
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

#include "driver.h"

enum e_mnemonics {
	_ADC  ,_ADD  ,_AND	,_BIT  ,_CALL ,_CCF  ,_CP	,_CPD  ,
	_CPDR ,_CPI  ,_CPIR ,_CPL  ,_DAA  ,_DB	 ,_DEC	,_DI   ,
	_DJNZ ,_EI	 ,_EX	,_EXX  ,_HALT ,_IM	 ,_IN	,_INC  ,
	_IND  ,_INDR ,_INI	,_INIR ,_JP   ,_JR	 ,_LD	,_LDD  ,
	_LDDR ,_LDI  ,_LDIR ,_NEG  ,_NOP  ,_OR	 ,_OTDR ,_OTIR ,
	_OUT  ,_OUTD ,_OUTI ,_POP  ,_PUSH ,_RES  ,_RET	,_RETI ,
	_RETN ,_RL	 ,_RLA	,_RLC  ,_RLCA ,_RLD  ,_RR	,_RRA  ,
	_RRC  ,_RRCA ,_RRD	,_RST  ,_SBC  ,_SCF  ,_SET	,_SLA  ,
	_SLL  ,_SRA  ,_SRL	,_SUB  ,_XOR
};

static char *s_mnemonic[] = {
	"adc", "add", "and", "bit", "call","ccf", "cp",  "cpd",
	"cpdr","cpi", "cpir","cpl", "daa", "db",  "dec", "di",
	"djnz","ei",  "ex",  "exx", "halt","im",  "in",  "inc",
	"ind", "indr","ini", "inir","jp",  "jr",  "ld",  "ldd",
	"lddr","ldi", "ldir","neg", "nop", "or",  "otdr","otir",
	"out", "outd","outi","pop", "push","res", "ret", "reti",
	"retn","rl",  "rla", "rlc", "rlca","rld", "rr",  "rra",
	"rrc", "rrca","rrd", "rst", "sbc", "scf", "set", "sla",
	"sll", "sra", "srl", "sub", "xor "
};

typedef struct {
	UINT8	mnemonic;
	char *	arguments;
}	z80dasm;

static z80dasm mnemonic_xx_cb[256]= {
	{_RLC,"b=Y"},   {_RLC,"c=Y"},   {_RLC,"d=Y"},   {_RLC,"e=Y"},   {_RLC,"h=Y"},   {_RLC,"l=Y"},   {_RLC,"Y"},     {_RLC,"a=Y"},
	{_RRC,"b=Y"},   {_RRC,"c=Y"},   {_RRC,"d=Y"},   {_RRC,"e=Y"},   {_RRC,"h=Y"},   {_RRC,"l=Y"},   {_RRC,"Y"},     {_RRC,"a=Y"},
	{_RL,"b=Y"},    {_RL,"c=Y"},    {_RL,"d=Y"},    {_RL,"e=Y"},    {_RL,"h=Y"},    {_RL,"l=Y"},    {_RL,"Y"},      {_RL,"a=Y"},
	{_RR,"b=Y"},    {_RR,"c=Y"},    {_RR,"d=Y"},    {_RR,"e=Y"},    {_RR,"h=Y"},    {_RR,"l=Y"},    {_RR,"Y"},      {_RR,"a=Y"},
	{_SLA,"b=Y"},   {_SLA,"c=Y"},   {_SLA,"d=Y"},   {_SLA,"e=Y"},   {_SLA,"h=Y"},   {_SLA,"l=Y"},   {_SLA,"Y"},     {_SLA,"a=Y"},
	{_SRA,"b=Y"},   {_SRA,"c=Y"},   {_SRA,"d=Y"},   {_SRA,"e=Y"},   {_SRA,"h=Y"},   {_SRA,"l=Y"},   {_SRA,"Y"},     {_SRA,"a=Y"},
	{_SLL,"b=Y"},   {_SLL,"c=Y"},   {_SLL,"d=Y"},   {_SLL,"e=Y"},   {_SLL,"h=Y"},   {_SLL,"l=Y"},   {_SLL,"Y"},     {_SLL,"a=Y"},
	{_SRL,"b=Y"},   {_SRL,"c=Y"},   {_SRL,"d=Y"},   {_SRL,"e=Y"},   {_SRL,"h=Y"},   {_SRL,"l=Y"},   {_SRL,"Y"},     {_SRL,"a=Y"},
	{_BIT,"b=0,Y"}, {_BIT,"c=0,Y"}, {_BIT,"d=0,Y"}, {_BIT,"e=0,Y"}, {_BIT,"h=0,Y"}, {_BIT,"l=0,Y"}, {_BIT,"0,Y"},   {_BIT,"a=0,Y"},
	{_BIT,"b=1,Y"}, {_BIT,"c=1,Y"}, {_BIT,"d=1,Y"}, {_BIT,"e=1,Y"}, {_BIT,"h=1,Y"}, {_BIT,"l=1,Y"}, {_BIT,"1,Y"},   {_BIT,"a=1,Y"},
	{_BIT,"b=2,Y"}, {_BIT,"c=2,Y"}, {_BIT,"d=2,Y"}, {_BIT,"e=2,Y"}, {_BIT,"h=2,Y"}, {_BIT,"l=2,Y"}, {_BIT,"2,Y"},   {_BIT,"a=2,Y"},
	{_BIT,"b=3,Y"}, {_BIT,"c=3,Y"}, {_BIT,"d=3,Y"}, {_BIT,"e=3,Y"}, {_BIT,"h=3,Y"}, {_BIT,"l=3,Y"}, {_BIT,"3,Y"},   {_BIT,"a=3,Y"},
	{_BIT,"b=4,Y"}, {_BIT,"c=4,Y"}, {_BIT,"d=4,Y"}, {_BIT,"e=4,Y"}, {_BIT,"h=4,Y"}, {_BIT,"l=4,Y"}, {_BIT,"4,Y"},   {_BIT,"a=4,Y"},
	{_BIT,"b=5,Y"}, {_BIT,"c=5,Y"}, {_BIT,"d=5,Y"}, {_BIT,"e=5,Y"}, {_BIT,"h=5,Y"}, {_BIT,"l=5,Y"}, {_BIT,"5,Y"},   {_BIT,"a=5,Y"},
	{_BIT,"b=6,Y"}, {_BIT,"c=6,Y"}, {_BIT,"d=6,Y"}, {_BIT,"e=6,Y"}, {_BIT,"h=6,Y"}, {_BIT,"l=6,Y"}, {_BIT,"6,Y"},   {_BIT,"a=6,Y"},
	{_BIT,"b=7,Y"}, {_BIT,"c=7,Y"}, {_BIT,"d=7,Y"}, {_BIT,"e=7,Y"}, {_BIT,"h=7,Y"}, {_BIT,"l=7,Y"}, {_BIT,"7,Y"},   {_BIT,"a=7,Y"},
	{_RES,"b=0,Y"}, {_RES,"c=0,Y"}, {_RES,"d=0,Y"}, {_RES,"e=0,Y"}, {_RES,"h=0,Y"}, {_RES,"l=0,Y"}, {_RES,"0,Y"},   {_RES,"a=0,Y"},
	{_RES,"b=1,Y"}, {_RES,"c=1,Y"}, {_RES,"d=1,Y"}, {_RES,"e=1,Y"}, {_RES,"h=1,Y"}, {_RES,"l=1,Y"}, {_RES,"1,Y"},   {_RES,"a=1,Y"},
	{_RES,"b=2,Y"}, {_RES,"c=2,Y"}, {_RES,"d=2,Y"}, {_RES,"e=2,Y"}, {_RES,"h=2,Y"}, {_RES,"l=2,Y"}, {_RES,"2,Y"},   {_RES,"a=2,Y"},
	{_RES,"b=3,Y"}, {_RES,"c=3,Y"}, {_RES,"d=3,Y"}, {_RES,"e=3,Y"}, {_RES,"h=3,Y"}, {_RES,"l=3,Y"}, {_RES,"3,Y"},   {_RES,"a=3,Y"},
	{_RES,"b=4,Y"}, {_RES,"c=4,Y"}, {_RES,"d=4,Y"}, {_RES,"e=4,Y"}, {_RES,"h=4,Y"}, {_RES,"l=4,Y"}, {_RES,"4,Y"},   {_RES,"a=4,Y"},
	{_RES,"b=5,Y"}, {_RES,"c=5,Y"}, {_RES,"d=5,Y"}, {_RES,"e=5,Y"}, {_RES,"h=5,Y"}, {_RES,"l=5,Y"}, {_RES,"5,Y"},   {_RES,"a=5,Y"},
	{_RES,"b=6,Y"}, {_RES,"c=6,Y"}, {_RES,"d=6,Y"}, {_RES,"e=6,Y"}, {_RES,"h=6,Y"}, {_RES,"l=6,Y"}, {_RES,"6,Y"},   {_RES,"a=6,Y"},
	{_RES,"b=7,Y"}, {_RES,"c=7,Y"}, {_RES,"d=7,Y"}, {_RES,"e=7,Y"}, {_RES,"h=7,Y"}, {_RES,"l=7,Y"}, {_RES,"7,Y"},   {_RES,"a=7,Y"},
	{_SET,"b=0,Y"}, {_SET,"c=0,Y"}, {_SET,"d=0,Y"}, {_SET,"e=0,Y"}, {_SET,"h=0,Y"}, {_SET,"l=0,Y"}, {_SET,"0,Y"},   {_SET,"a=0,Y"},
	{_SET,"b=1,Y"}, {_SET,"c=1,Y"}, {_SET,"d=1,Y"}, {_SET,"e=1,Y"}, {_SET,"h=1,Y"}, {_SET,"l=1,Y"}, {_SET,"1,Y"},   {_SET,"a=1,Y"},
	{_SET,"b=2,Y"}, {_SET,"c=2,Y"}, {_SET,"d=2,Y"}, {_SET,"e=2,Y"}, {_SET,"h=2,Y"}, {_SET,"l=2,Y"}, {_SET,"2,Y"},   {_SET,"a=2,Y"},
	{_SET,"b=3,Y"}, {_SET,"c=3,Y"}, {_SET,"d=3,Y"}, {_SET,"e=3,Y"}, {_SET,"h=3,Y"}, {_SET,"l=3,Y"}, {_SET,"3,Y"},   {_SET,"a=3,Y"},
	{_SET,"b=4,Y"}, {_SET,"c=4,Y"}, {_SET,"d=4,Y"}, {_SET,"e=4,Y"}, {_SET,"h=4,Y"}, {_SET,"l=4,Y"}, {_SET,"4,Y"},   {_SET,"a=4,Y"},
	{_SET,"b=5,Y"}, {_SET,"c=5,Y"}, {_SET,"d=5,Y"}, {_SET,"e=5,Y"}, {_SET,"h=5,Y"}, {_SET,"l=5,Y"}, {_SET,"5,Y"},   {_SET,"a=5,Y"},
	{_SET,"b=6,Y"}, {_SET,"c=6,Y"}, {_SET,"d=6,Y"}, {_SET,"e=6,Y"}, {_SET,"h=6,Y"}, {_SET,"l=6,Y"}, {_SET,"6,Y"},   {_SET,"a=6,Y"},
	{_SET,"b=7,Y"}, {_SET,"c=7,Y"}, {_SET,"d=7,Y"}, {_SET,"e=7,Y"}, {_SET,"h=7,Y"}, {_SET,"l=7,Y"}, {_SET,"7,Y"},   {_SET,"a=7,Y"}
};

static z80dasm mnemonic_cb[256] = {
	{_RLC,"b"},     {_RLC,"c"},     {_RLC,"d"},     {_RLC,"e"},     {_RLC,"h"},     {_RLC,"l"},     {_RLC,"(hl)"},  {_RLC,"a"},
	{_RRC,"b"},     {_RRC,"c"},     {_RRC,"d"},     {_RRC,"e"},     {_RRC,"h"},     {_RRC,"l"},     {_RRC,"(hl)"},  {_RRC,"a"},
	{_RL,"b"},      {_RL,"c"},      {_RL,"d"},      {_RL,"e"},      {_RL,"h"},      {_RL,"l"},      {_RL,"(hl)"},   {_RL,"a"},
	{_RR,"b"},      {_RR,"c"},      {_RR,"d"},      {_RR,"e"},      {_RR,"h"},      {_RR,"l"},      {_RR,"(hl)"},   {_RR,"a"},
	{_SLA,"b"},     {_SLA,"c"},     {_SLA,"d"},     {_SLA,"e"},     {_SLA,"h"},     {_SLA,"l"},     {_SLA,"(hl)"},  {_SLA,"a"},
	{_SRA,"b"},     {_SRA,"c"},     {_SRA,"d"},     {_SRA,"e"},     {_SRA,"h"},     {_SRA,"l"},     {_SRA,"(hl)"},  {_SRA,"a"},
	{_SLL,"b"},     {_SLL,"c"},     {_SLL,"d"},     {_SLL,"e"},     {_SLL,"h"},     {_SLL,"l"},     {_SLL,"(hl)"},  {_SLL,"a"},
	{_SRL,"b"},     {_SRL,"c"},     {_SRL,"d"},     {_SRL,"e"},     {_SRL,"h"},     {_SRL,"l"},     {_SRL,"(hl)"},  {_SRL,"a"},
	{_BIT,"0,b"},   {_BIT,"0,c"},   {_BIT,"0,d"},   {_BIT,"0,e"},   {_BIT,"0,h"},   {_BIT,"0,l"},   {_BIT,"0,(hl)"},{_BIT,"0,a"},
	{_BIT,"1,b"},   {_BIT,"1,c"},   {_BIT,"1,d"},   {_BIT,"1,e"},   {_BIT,"1,h"},   {_BIT,"1,l"},   {_BIT,"1,(hl)"},{_BIT,"1,a"},
	{_BIT,"2,b"},   {_BIT,"2,c"},   {_BIT,"2,d"},   {_BIT,"2,e"},   {_BIT,"2,h"},   {_BIT,"2,l"},   {_BIT,"2,(hl)"},{_BIT,"2,a"},
	{_BIT,"3,b"},   {_BIT,"3,c"},   {_BIT,"3,d"},   {_BIT,"3,e"},   {_BIT,"3,h"},   {_BIT,"3,l"},   {_BIT,"3,(hl)"},{_BIT,"3,a"},
	{_BIT,"4,b"},   {_BIT,"4,c"},   {_BIT,"4,d"},   {_BIT,"4,e"},   {_BIT,"4,h"},   {_BIT,"4,l"},   {_BIT,"4,(hl)"},{_BIT,"4,a"},
	{_BIT,"5,b"},   {_BIT,"5,c"},   {_BIT,"5,d"},   {_BIT,"5,e"},   {_BIT,"5,h"},   {_BIT,"5,l"},   {_BIT,"5,(hl)"},{_BIT,"5,a"},
	{_BIT,"6,b"},   {_BIT,"6,c"},   {_BIT,"6,d"},   {_BIT,"6,e"},   {_BIT,"6,h"},   {_BIT,"6,l"},   {_BIT,"6,(hl)"},{_BIT,"6,a"},
	{_BIT,"7,b"},   {_BIT,"7,c"},   {_BIT,"7,d"},   {_BIT,"7,e"},   {_BIT,"7,h"},   {_BIT,"7,l"},   {_BIT,"7,(hl)"},{_BIT,"7,a"},
	{_RES,"0,b"},   {_RES,"0,c"},   {_RES,"0,d"},   {_RES,"0,e"},   {_RES,"0,h"},   {_RES,"0,l"},   {_RES,"0,(hl)"},{_RES,"0,a"},
	{_RES,"1,b"},   {_RES,"1,c"},   {_RES,"1,d"},   {_RES,"1,e"},   {_RES,"1,h"},   {_RES,"1,l"},   {_RES,"1,(hl)"},{_RES,"1,a"},
	{_RES,"2,b"},   {_RES,"2,c"},   {_RES,"2,d"},   {_RES,"2,e"},   {_RES,"2,h"},   {_RES,"2,l"},   {_RES,"2,(hl)"},{_RES,"2,a"},
	{_RES,"3,b"},   {_RES,"3,c"},   {_RES,"3,d"},   {_RES,"3,e"},   {_RES,"3,h"},   {_RES,"3,l"},   {_RES,"3,(hl)"},{_RES,"3,a"},
	{_RES,"4,b"},   {_RES,"4,c"},   {_RES,"4,d"},   {_RES,"4,e"},   {_RES,"4,h"},   {_RES,"4,l"},   {_RES,"4,(hl)"},{_RES,"4,a"},
	{_RES,"5,b"},   {_RES,"5,c"},   {_RES,"5,d"},   {_RES,"5,e"},   {_RES,"5,h"},   {_RES,"5,l"},   {_RES,"5,(hl)"},{_RES,"5,a"},
	{_RES,"6,b"},   {_RES,"6,c"},   {_RES,"6,d"},   {_RES,"6,e"},   {_RES,"6,h"},   {_RES,"6,l"},   {_RES,"6,(hl)"},{_RES,"6,a"},
	{_RES,"7,b"},   {_RES,"7,c"},   {_RES,"7,d"},   {_RES,"7,e"},   {_RES,"7,h"},   {_RES,"7,l"},   {_RES,"7,(hl)"},{_RES,"7,a"},
	{_SET,"0,b"},   {_SET,"0,c"},   {_SET,"0,d"},   {_SET,"0,e"},   {_SET,"0,h"},   {_SET,"0,l"},   {_SET,"0,(hl)"},{_SET,"0,a"},
	{_SET,"1,b"},   {_SET,"1,c"},   {_SET,"1,d"},   {_SET,"1,e"},   {_SET,"1,h"},   {_SET,"1,l"},   {_SET,"1,(hl)"},{_SET,"1,a"},
	{_SET,"2,b"},   {_SET,"2,c"},   {_SET,"2,d"},   {_SET,"2,e"},   {_SET,"2,h"},   {_SET,"2,l"},   {_SET,"2,(hl)"},{_SET,"2,a"},
	{_SET,"3,b"},   {_SET,"3,c"},   {_SET,"3,d"},   {_SET,"3,e"},   {_SET,"3,h"},   {_SET,"3,l"},   {_SET,"3,(hl)"},{_SET,"3,a"},
	{_SET,"4,b"},   {_SET,"4,c"},   {_SET,"4,d"},   {_SET,"4,e"},   {_SET,"4,h"},   {_SET,"4,l"},   {_SET,"4,(hl)"},{_SET,"4,a"},
	{_SET,"5,b"},   {_SET,"5,c"},   {_SET,"5,d"},   {_SET,"5,e"},   {_SET,"5,h"},   {_SET,"5,l"},   {_SET,"5,(hl)"},{_SET,"5,a"},
	{_SET,"6,b"},   {_SET,"6,c"},   {_SET,"6,d"},   {_SET,"6,e"},   {_SET,"6,h"},   {_SET,"6,l"},   {_SET,"6,(hl)"},{_SET,"6,a"},
	{_SET,"7,b"},   {_SET,"7,c"},   {_SET,"7,d"},   {_SET,"7,e"},   {_SET,"7,h"},   {_SET,"7,l"},   {_SET,"7,(hl)"},{_SET,"7,a"}
};

static z80dasm mnemonic_ed[256]= {
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_IN,"b,(c)"},  {_OUT,"(c),b"}, {_SBC,"hl,bc"}, {_LD,"(W),bc"}, {_NEG,0},       {_RETN,0},      {_IM,"0"},      {_LD,"i,a"},
	{_IN,"c,(c)"},  {_OUT,"(c),c"}, {_ADC,"hl,bc"}, {_LD,"bc,(W)"}, {_NEG,"*"},     {_RETI,0},      {_IM,"0"},      {_LD,"r,a"},
	{_IN,"d,(c)"},  {_OUT,"(c),d"}, {_SBC,"hl,de"}, {_LD,"(W),de"}, {_NEG,"*"},     {_RETN,0},      {_IM,"1"},      {_LD,"a,i"},
	{_IN,"e,(c)"},  {_OUT,"(c),e"}, {_ADC,"hl,de"}, {_LD,"de,(W)"}, {_NEG,"*"},     {_RETI,0},      {_IM,"2"},      {_LD,"a,r"},
	{_IN,"h,(c)"},  {_OUT,"(c),h"}, {_SBC,"hl,hl"}, {_LD,"(W),hl"}, {_NEG,"*"},     {_RETN,0},      {_IM,"0"},      {_RRD,"(hl)"},
	{_IN,"l,(c)"},  {_OUT,"(c),l"}, {_ADC,"hl,hl"}, {_LD,"hl,(W)"}, {_NEG,"*"},     {_RETI,0},      {_IM,"0"},      {_RLD,"(hl)"},
	{_IN,"0,(c)"},  {_OUT,"(c),0"}, {_SBC,"hl,sp"}, {_LD,"(W),sp"}, {_NEG,"*"},     {_RETN,0},      {_IM,"1"},      {_DB,"?"},
	{_IN,"a,(c)"},  {_OUT,"(c),a"}, {_ADC,"hl,sp"}, {_LD,"sp,(W)"}, {_NEG,"*"},     {_RETI,0},      {_IM,"2"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
    {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
    {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
    {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_LDI,0},		{_CPI,0},		{_INI,0},		{_OUTI,0},		{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_LDD,0},		{_CPD,0},		{_IND,0},		{_OUTD,0},		{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_LDIR,0},		{_CPIR,0},		{_INIR,0},		{_OTIR,0},		{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_LDDR,0},		{_CPDR,0},		{_INDR,0},		{_OTDR,0},		{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"}
};

static z80dasm mnemonic_xx[256]= {
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_ADD,"I,bc"},  {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_ADD,"I,de"},  {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_LD,"I,W"},    {_LD,"(W),I"},  {_INC,"I"},     {_INC,"Ih"},    {_DEC,"Ih"},    {_LD,"Ih,B"},   {_DB,"?"},
	{_DB,"?"},      {_ADD,"I,I"},   {_LD,"I,(W)"},  {_DEC,"I"},     {_INC,"Il"},    {_DEC,"Il"},    {_LD,"Il,B"},   {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_INC,"X"},     {_DEC,"X"},     {_LD,"X,B"},    {_DB,"?"},
	{_DB,"?"},      {_ADD,"I,sp"},  {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_LD,"b,Ih"},   {_LD,"b,Il"},   {_LD,"b,X"},    {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_LD,"c,Ih"},   {_LD,"c,Il"},   {_LD,"c,X"},    {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_LD,"d,Ih"},   {_LD,"d,Il"},   {_LD,"d,X"},    {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_LD,"e,Ih"},   {_LD,"e,Il"},   {_LD,"e,X"},    {_DB,"?"},
	{_LD,"Ih,b"},   {_LD,"Ih,c"},   {_LD,"Ih,d"},   {_LD,"Ih,e"},   {_LD,"Ih,Ih"},  {_LD,"Ih,Il"},  {_LD,"h,X"},    {_LD,"Ih,a"},
	{_LD,"Il,b"},   {_LD,"Il,c"},   {_LD,"Il,d"},   {_LD,"Il,e"},   {_LD,"Il,Ih"},  {_LD,"Il,Il"},  {_LD,"l,X"},    {_LD,"Il,a"},
	{_LD,"X,b"},    {_LD,"X,c"},    {_LD,"X,d"},    {_LD,"X,e"},    {_LD,"X,h"},    {_LD,"X,l"},    {_DB,"?"},      {_LD,"X,a"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_LD,"a,Ih"},   {_LD,"a,Il"},   {_LD,"a,X"},    {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_ADD,"a,Ih"},  {_ADD,"a,Il"},  {_ADD,"a,X"},   {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_ADC,"a,Ih"},  {_ADC,"a,Il"},  {_ADC,"a,X"},   {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_SUB,"Ih"},    {_SUB,"Il"},    {_SUB,"X"},     {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_SBC,"a,Ih"},  {_SBC,"a,Il"},  {_SBC,"a,X"},   {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_AND,"Ih"},    {_AND,"Il"},    {_AND,"X"},     {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_XOR,"Ih"},    {_XOR,"Il"},    {_XOR,"X"},     {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_OR,"Ih"},     {_OR,"Il"},     {_OR,"X"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_CP,"Ih"},     {_CP,"Il"},     {_CP,"X"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"cb"},     {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_POP,"I"},     {_DB,"?"},      {_EX,"(sp),I"}, {_DB,"?"},      {_PUSH,"I"},    {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_JP,"(I)"},    {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},
	{_DB,"?"},      {_LD,"sp,I"},   {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"},      {_DB,"?"}
};

static z80dasm mnemonic_main[256]= {
	{_NOP,0},		{_LD,"bc,W"},   {_LD,"(bc),a"}, {_INC,"bc"},    {_INC,"b"},     {_DEC,"b"},     {_LD,"b,B"},    {_RLCA,0},
	{_EX,"af,af'"}, {_ADD,"hl,bc"}, {_LD,"a,(bc)"}, {_DEC,"bc"},    {_INC,"c"},     {_DEC,"c"},     {_LD,"c,B"},    {_RRCA,0},
	{_DJNZ,"R"},    {_LD,"de,W"},   {_LD,"(de),a"}, {_INC,"de"},    {_INC,"d"},     {_DEC,"d"},     {_LD,"d,B"},    {_RLA,0},
	{_JR,"R"},      {_ADD,"hl,de"}, {_LD,"a,(de)"}, {_DEC,"de"},    {_INC,"e"},     {_DEC,"e"},     {_LD,"e,B"},    {_RRA,0},
	{_JR,"nz,R"},   {_LD,"hl,W"},   {_LD,"(W),hl"}, {_INC,"hl"},    {_INC,"h"},     {_DEC,"h"},     {_LD,"h,B"},    {_DAA,0},
	{_JR,"z,R"},    {_ADD,"hl,hl"}, {_LD,"hl,(W)"}, {_DEC,"hl"},    {_INC,"l"},     {_DEC,"l"},     {_LD,"l,B"},    {_CPL,0},
	{_JR,"nc,R"},   {_LD,"sp,W"},   {_LD,"(W),a"},  {_INC,"sp"},    {_INC,"(hl)"},  {_DEC,"(hl)"},  {_LD,"(hl),B"}, {_SCF,0},
	{_JR,"c,R"},    {_ADD,"hl,sp"}, {_LD,"a,(W)"},  {_DEC,"sp"},    {_INC,"a"},     {_DEC,"a"},     {_LD,"a,B"},    {_CCF,0},
	{_LD,"b,b"},    {_LD,"b,c"},    {_LD,"b,d"},    {_LD,"b,e"},    {_LD,"b,h"},    {_LD,"b,l"},    {_LD,"b,(hl)"}, {_LD,"b,a"},
	{_LD,"c,b"},    {_LD,"c,c"},    {_LD,"c,d"},    {_LD,"c,e"},    {_LD,"c,h"},    {_LD,"c,l"},    {_LD,"c,(hl)"}, {_LD,"c,a"},
	{_LD,"d,b"},    {_LD,"d,c"},    {_LD,"d,d"},    {_LD,"d,e"},    {_LD,"d,h"},    {_LD,"d,l"},    {_LD,"d,(hl)"}, {_LD,"d,a"},
	{_LD,"e,b"},    {_LD,"e,c"},    {_LD,"e,d"},    {_LD,"e,e"},    {_LD,"e,h"},    {_LD,"e,l"},    {_LD,"e,(hl)"}, {_LD,"e,a"},
	{_LD,"h,b"},    {_LD,"h,c"},    {_LD,"h,d"},    {_LD,"h,e"},    {_LD,"h,h"},    {_LD,"h,l"},    {_LD,"h,(hl)"}, {_LD,"h,a"},
	{_LD,"l,b"},    {_LD,"l,c"},    {_LD,"l,d"},    {_LD,"l,e"},    {_LD,"l,h"},    {_LD,"l,l"},    {_LD,"l,(hl)"}, {_LD,"l,a"},
	{_LD,"(hl),b"}, {_LD,"(hl),c"}, {_LD,"(hl),d"}, {_LD,"(hl),e"}, {_LD,"(hl),h"}, {_LD,"(hl),l"}, {_HALT,0},      {_LD,"(hl),a"},
	{_LD,"a,b"},    {_LD,"a,c"},    {_LD,"a,d"},    {_LD,"a,e"},    {_LD,"a,h"},    {_LD,"a,l"},    {_LD,"a,(hl)"}, {_LD,"a,a"},
	{_ADD,"a,b"},   {_ADD,"a,c"},   {_ADD,"a,d"},   {_ADD,"a,e"},   {_ADD,"a,h"},   {_ADD,"a,l"},   {_ADD,"a,(hl)"},{_ADD,"a,a"},
	{_ADC,"a,b"},   {_ADC,"a,c"},   {_ADC,"a,d"},   {_ADC,"a,e"},   {_ADC,"a,h"},   {_ADC,"a,l"},   {_ADC,"a,(hl)"},{_ADC,"a,a"},
	{_SUB,"b"},     {_SUB,"c"},     {_SUB,"d"},     {_SUB,"e"},     {_SUB,"h"},     {_SUB,"l"},     {_SUB,"(hl)"},  {_SUB,"a"},
	{_SBC,"a,b"},   {_SBC,"a,c"},   {_SBC,"a,d"},   {_SBC,"a,e"},   {_SBC,"a,h"},   {_SBC,"a,l"},   {_SBC,"a,(hl)"},{_SBC,"a,a"},
	{_AND,"b"},     {_AND,"c"},     {_AND,"d"},     {_AND,"e"},     {_AND,"h"},     {_AND,"l"},     {_AND,"(hl)"},  {_AND,"a"},
	{_XOR,"b"},     {_XOR,"c"},     {_XOR,"d"},     {_XOR,"e"},     {_XOR,"h"},     {_XOR,"l"},     {_XOR,"(hl)"},  {_XOR,"a"},
	{_OR,"b"},      {_OR,"c"},      {_OR,"d"},      {_OR,"e"},      {_OR,"h"},      {_OR,"l"},      {_OR,"(hl)"},   {_OR,"a"},
	{_CP,"b"},      {_CP,"c"},      {_CP,"d"},      {_CP,"e"},      {_CP,"h"},      {_CP,"l"},      {_CP,"(hl)"},   {_CP,"a"},
	{_RET,"nz"},    {_POP,"bc"},    {_JP,"nz,W"},   {_JP,"W"},      {_CALL,"nz,W"}, {_PUSH,"bc"},   {_ADD,"a,B"},   {_RST,"00h"},
	{_RET,"z"},     {_RET,0},       {_JP,"z,W"},    {_DB,"cb"},     {_CALL,"z,W"},  {_CALL,"W"},    {_ADC,"a,B"},   {_RST,"08h"},
	{_RET,"nc"},    {_POP,"de"},    {_JP,"nc,W"},   {_OUT,"(B),a"}, {_CALL,"nc,W"}, {_PUSH,"de"},   {_SUB,"B"},     {_RST,"10h"},
	{_RET,"c"},     {_EXX,0},       {_JP,"c,W"},    {_IN,"a,(B)"},  {_CALL,"c,W"},  {_DB,"dd"},     {_SBC,"a,B"},   {_RST,"18h"},
	{_RET,"po"},    {_POP,"hl"},    {_JP,"po,W"},   {_EX,"(sp),hl"},{_CALL,"po,W"}, {_PUSH,"hl"},   {_AND,"B"},     {_RST,"20h"},
	{_RET,"pe"},    {_JP,"(hl)"},   {_JP,"pe,W"},   {_EX,"de,hl"},  {_CALL,"pe,W"}, {_DB,"ed"},     {_XOR,"B"},     {_RST,"28h"},
	{_RET,"p"},     {_POP,"af"},    {_JP,"p,W"},    {_DI,0},        {_CALL,"p,W"},  {_PUSH,"af"},   {_OR,"B"},      {_RST,"30h"},
	{_RET,"m"},     {_LD,"sp,hl"},  {_JP,"m,W"},    {_EI,0},        {_CALL,"m,W"},  {_DB,"fd"},     {_CP,"B"},      {_RST,"38h"}
};

static char sign(INT8 offset)
{
 return (offset < 0)? '-':'+';
}

static int offs(INT8 offset)
{
	if (offset < 0) return -offset;
	return offset;
}

/****************************************************************************/
/* Disassemble first opcode in buffer and return number of bytes it takes   */
/****************************************************************************/
int DasmZ80(char *dest,int PC)
{
	z80dasm *d;
	char *r, *src;
	int pc;
	INT8 offset = 0;
	UINT8 op, op1;
	pc = PC;
	r = "INTERNAL PROGRAM ERROR";

	op = cpu_readop(pc);
	op1 = 0; /* keep GCC happy */
	pc++;
	switch (op) {
		case 0xcb:
			op = cpu_readop(pc++);
			d = &mnemonic_cb[op];
			break;
		case 0xed:
			op1 = cpu_readop(pc++);
			d = &mnemonic_ed[op1];
			break;
		case 0xdd:
			r="ix";
			op1 = cpu_readop(pc++);
			switch (op1) {
				case 0xcb:
					offset = (INT8) cpu_readop_arg(pc++);
					op1 = cpu_readop(pc++);
					d = &mnemonic_xx_cb[op1];
					break;
				default:
					d = &mnemonic_xx[op1];
					break;
			}
			break;
		case 0xfd:
			r="iy";
			op1 = cpu_readop(pc++);
			switch (op1) {
				case 0xcb:
					offset = (INT8) cpu_readop_arg(pc++);
					op1 = cpu_readop(pc++);
					d = &mnemonic_xx_cb[op1];
					break;
				default:
					d = &mnemonic_xx[op1];
					break;
			}
			break;
		default:
			d = &mnemonic_main[op];
			break;
	}
	if (d->arguments) {
		dest += sprintf(dest, "%-4s ", s_mnemonic[d->mnemonic]);
		src = d->arguments;
		while (*src) {
			switch (*src) {
				case '?':   /* illegal opcode */
					dest += sprintf( dest, "$%02x,$%02x", op, op1);
					break;
				case 'B':   /* Byte op arg */
					dest += sprintf( dest, "$%02x", cpu_readop_arg(pc));
					pc++;
					break;
				case 'R':
					dest += sprintf( dest, "$%04x", (PC+2+(INT8)cpu_readop_arg(pc))&0xffff);
					pc++;
					break;
				case 'W':
					dest += sprintf( dest, "$%04x", cpu_readop_arg(pc) + (cpu_readop_arg((pc+1)&0xffff)<<8));
					pc+=2;
					break;
				case 'X':
					offset = (INT8) cpu_readop_arg(pc);
					dest += sprintf( dest,"(%s%c$%02x)", r, sign(offset), offs(offset) );
					pc++;
					break;
				case 'Y':
					dest += sprintf( dest,"(%s%c$%02x)", r, sign(offset), offs(offset) );
					break;
				case 'I':
					dest += sprintf( dest, "%s", r);
					break;
				default:
					*dest++ = *src;
			}
			src++;
		}
		*dest = '\0';
	} else {
		dest += sprintf(dest, "%s", s_mnemonic[d->mnemonic]);
    }
	return (pc - PC);
}

