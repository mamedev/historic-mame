#define S2650_INT_NONE  -1
#define S2650_INT_ZSBR0 0x00

/* fake control port   M/~IO=0 D/~C=0 E/~NE=0 */
#define S2650_CTRL_PORT       0x100

/* fake data port      M/~IO=0 D/~C=1 E/~NE=0 */
#define S2650_DATA_PORT       0x101

/* extended i/o ports  M/~IO=0 D/~C=x E/~NE=1 */
#define S2650_EXT_PORT        0xff

typedef struct {
        unsigned short  page;   /* 8K page select register (A14..A13) */
        unsigned short  iar;    /* instruction address register (A12..A0) */
        unsigned short  ea;     /* effective address (A14..A0) */
        unsigned char   psl;    /* processor status lower */
        unsigned char   psu;    /* processor status upper */
        unsigned char   r;      /* absolute addressing dst/src register */
        unsigned char   reg[7]; /* 7 general purpose registers */
        unsigned char   halt;   /* 1 if cpu is halted */
        unsigned char   ir;     /* instruction register */
        int             irq;    /* interrupt request vector */
        unsigned short  ras[8]; /* 8 return address stack entries */
}       S2650_Regs;

extern	void	S2650_SetRegs(S2650_Regs * rgs);
extern	void	S2650_GetRegs(S2650_Regs * rgs);
extern  int     S2650_GetPC(void);
extern	void	S2650_set_flag(int state);
extern	int 	S2650_get_flag(void);
extern  void    S2650_set_sense(int state);
extern	int 	S2650_get_sense(void);
extern  void    S2650_Reset(void);
extern  int     S2650_Execute(int cycles);
extern  void    S2650_Cause_Interrupt(int type);
extern  void    S2650_Clear_Pending_Interrupts(void);

extern  int     S2650_ICount;

#ifdef  MAME_DEBUG
extern  int     Dasm2650(char * buff, int PC);
#endif
