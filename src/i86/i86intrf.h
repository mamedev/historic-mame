#define I86_NMI_INT 2
extern void I86_Reset(unsigned char *mem, int cycles);
extern void I86_Execute(void);
extern int int86_pending;
