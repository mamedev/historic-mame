
#ifndef AUDIT_H
#define AUDIT_H

/* return values from VerifyRomSet and VerifySampleSet */

#define CORRECT   0
#define NOTFOUND  1
#define INCORRECT 2

typedef void (*verify_printf_proc)(char *fmt,...);

int VerifyRomSet(int game,verify_printf_proc verify_printf);
int VerifySampleSet(int game,verify_printf_proc verify_printf);


#endif
