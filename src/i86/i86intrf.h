#define I86_NMI_INT 2
void I86_Reset(unsigned char *mem, int cycles);
void I86_Execute(void);
extern int int86_pending;
