static struct {
	struct {
		WREGS w[256];
		BREGS b[256];
	} reg;
	struct {
		WREGS w[256];
		BREGS b[256];
	} RM;
} Mod_RM;

#define RegWord(ModRM) regs.w[Mod_RM.reg.w[ModRM]]
#define RegByte(ModRM) regs.b[Mod_RM.reg.b[ModRM]]

#define GetRMWord(ModRM) \
	((ModRM) >= 0xc0 ? regs.w[Mod_RM.RM.w[ModRM]] : ( (*GetEA[ModRM])(), ReadWord( EA ) ))

#define PutbackRMWord(ModRM,val) \
{ \
    if (ModRM >= 0xc0) regs.w[Mod_RM.RM.w[ModRM]]=val; \
    else WriteWord(EA,val); \
}

#define GetnextRMWord ReadWord(EA+2)

#define PutRMWord(ModRM,val) \
{ \
    if (ModRM >= 0xc0) \
        regs.w[Mod_RM.RM.w[ModRM]]=val; \
    else { \
	(*GetEA[ModRM])(); \
	WriteWord( EA ,val); \
    } \
}

#define PutImmRMWord(ModRM) \
{ \
    WORD val; \
    if (ModRM >= 0xc0) \
        FETCHWORD(regs.w[Mod_RM.RM.w[ModRM]]) \
    else { \
	(*GetEA[ModRM])(); \
	FETCHWORD(val) \
	WriteWord( EA , val); \
    } \
}
	
#define GetRMByte(ModRM) \
	((ModRM) >= 0xc0 ? regs.b[Mod_RM.RM.b[ModRM]] : ReadByte( (*GetEA[ModRM])() ))

#define PutRMByte(ModRM,val) \
{ \
    if (ModRM >= 0xc0) \
        regs.b[Mod_RM.RM.b[ModRM]]=val; \
    else \
	WriteByte( (*GetEA[ModRM])() ,val); \
}

#define PutImmRMByte(ModRM) \
{ \
    if (ModRM >= 0xc0) \
        regs.b[Mod_RM.RM.b[ModRM]]=FETCH; \
    else { \
	(*GetEA[ModRM])(); \
	WriteByte( EA , FETCH ); \
    } \
}
	
#define PutbackRMByte(ModRM,val) \
{ \
    if (ModRM >= 0xc0) regs.b[Mod_RM.RM.b[ModRM]]=val; \
    else WriteByte(EA,val); \
}
